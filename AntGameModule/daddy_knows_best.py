from AntGame import *
from AntTypes import *
import os
import sys
import math

AI_NAME = os.path.basename(__file__)
AI_NAME_WITHOUT_EXT = os.path.splitext(AI_NAME)[0]
AI_NAME = AI_NAME_WITHOUT_EXT.replace(" ", "_")
agc = AntGameClient()

os.makedirs(os.path.join(os.path.dirname(__file__), '..', 'logs'), exist_ok=True)
LOGFILE = open(os.path.join(os.path.dirname(__file__), '..', 'logs', f'{AI_NAME_WITHOUT_EXT}_debug_{os.getpid()}.log'), 'w')

DEFAULT = AntTypes.DEFAULT_ANT.value
PEASANT = AntTypes.PEASANT_ANT.value
GLASS = AntTypes.GLASS_CANON_ANT.value
TANK = AntTypes.TANK_ANT.value

# Defense ranges (3x attack range)
GLASS_RANGE = 42
TANK_RANGE = 24
DEFAULT_RANGE = 30

defaultCount = 5
speedyCount = 0
glassCount = 0
tankCount = 0
flagDict = {}
elapsedTime = 0.0
totalBuilt = 0
totalDeaths = 0
allInActive = False
highAntTime = 0.0  # game-seconds with 150+ ants (for timeout trigger)
peakAnts = 5


def log(msg):
    LOGFILE.write(f"[T{elapsedTime:.2f}] {msg}\n")
    LOGFILE.flush()


def defenseRange(ant):
    if ant.type == GLASS:
        return GLASS_RANGE
    elif ant.type == TANK:
        return TANK_RANGE
    return DEFAULT_RANGE


def findPriorityTarget(ant: Ant):
    """Find best enemy to attack: prioritize PEASANTs (one-shot) and low-HP enemies."""
    nearest = ant.nearestEnemy()
    if nearest is None:
        return None
    if nearest.type == PEASANT:
        return nearest
    baseDist = ant.pos.dist(nearest.pos)
    searchRadius = baseDist * 1.5
    bestPeasant = None
    bestPeasantDist = searchRadius
    bestWounded = None
    bestWoundedDist = searchRadius
    for enemy in agc.enemies:
        d = ant.pos.dist(enemy.pos)
        if d > searchRadius:
            continue
        if enemy.type == PEASANT and d < bestPeasantDist:
            bestPeasant = enemy
            bestPeasantDist = d
        elif enemy.health <= 2.0 and d < bestWoundedDist:
            bestWounded = enemy
            bestWoundedDist = d
    if bestPeasant is not None:
        return bestPeasant
    if bestWounded is not None:
        return bestWounded
    return nearest


def nextFood(ant: Ant):
    if ant.isFull:
        ant.goDeliver()
    else:
        ant.goTake(ant.nearestFreeFood())


def onStart():
    global flagDict
    log("v26: delta-based timing, onHit callback, food theft fix")
    log(f"START: nest=({agc.me.pos.x:.0f},{agc.me.pos.y:.0f})")
    for ant in agc.me.ants:
        ant.goTake(agc.nearestFreeFood())
        flagDict[ant.id] = {"attacking": False}


def onFrame():
    global flagDict, elapsedTime, allInActive
    global highAntTime, peakAnts
    dt = agc.delta
    elapsedTime += dt

    numAnts = len(agc.me.ants)
    numEnemies = len(agc.enemies)

    if numAnts > peakAnts:
        peakAnts = numAnts

    # Track time at high ant count (for timeout)
    if numAnts >= 150 and numEnemies > 0:
        highAntTime += dt

    # All-in trigger: 180+ ants,
    # OR timeout after ~150 game-seconds at 150+ ants
    normalTrigger = (numAnts >= 180 and numEnemies > 0)
    timeoutTrigger = (highAntTime >= 150.0 and numAnts >= 150 and numEnemies > 0)
    if not allInActive and (normalTrigger or timeoutTrigger):
        allInActive = True
        reason = "TIMEOUT" if timeoutTrigger and not normalTrigger else "THRESHOLD"
        log(f"ALL-IN ACTIVATED ({reason}): ants={numAnts} food={agc.me.food:.1f} enemies={numEnemies} highAntTime={highAntTime:.1f}s peak={peakAnts}")

    if allInActive:
        for ant in agc.me.ants:
            if flagDict.get(ant.id) is None:
                flagDict[ant.id] = {"attacking": False}
            # Only combat types attack; PEASANTs keep gathering for replacements
            if ant.type != PEASANT:
                flagDict[ant.id]["attacking"] = True
                ant.followAttack(ant.nearestEnemy())

    # Emergency rebuild when all ants dead
    if numAnts < 1 and agc.me.food >= PEASANT.cost:
        for _ in range(math.floor(agc.me.food / PEASANT.cost)):
            agc.newAnt(PEASANT)

    # Per-ant self-defense (all non-attacking ants, tight range)
    for ant in agc.me.ants:
        if flagDict.get(ant.id) is None:
            flagDict[ant.id] = {"attacking": False}
        if not flagDict[ant.id]["attacking"]:
            enemy = ant.nearestEnemy()
            dr = defenseRange(ant)
            if enemy is not None and ant.pos.dist(enemy.pos) < dr:
                if ant.target != enemy:
                    ant.followAttack(enemy)
            elif ant.target is not None:
                # Enemy moved beyond defense range — disengage and resume gathering
                nextFood(ant)

    # Logging (every ~12.5 game-seconds)
    if int(elapsedTime / 12.5) != int((elapsedTime - dt) / 12.5):
        mode = "ALLIN" if allInActive else "gather"
        log(f"STATUS: food={agc.me.food:.1f} ants={numAnts}(p{speedyCount}/d{defaultCount}/g{glassCount}/t{tankCount}) enemies={numEnemies} built={totalBuilt} deaths={totalDeaths} mode={mode} peak={peakAnts} highTime={highAntTime:.1f}s elapsed={elapsedTime:.1f}s")

    if agc.me.food < 15:
        log(f"LOW FOOD: {agc.me.food:.1f} (ants={numAnts}, built={totalBuilt})")


def onWait(ant: Ant):
    if flagDict.get(ant.id) and flagDict[ant.id]["attacking"]:
        target = findPriorityTarget(ant)
        if target is not None:
            ant.followAttack(target)
        else:
            flagDict[ant.id]["attacking"] = False
            nextFood(ant)
    else:
        nextFood(ant)


def onDeliver(ant: Ant):
    ant.goTake(agc.nearestFreeFood())
    if allInActive:
        # During all-in: replace PEASANTs if economy is collapsing
        totalCombat = glassCount + tankCount
        if speedyCount < totalCombat * 0.5 and agc.me.food > 80 + PEASANT.cost:
            agc.newAnt(PEASANT)
        # Build combat replacements, keep food reserve
        elif agc.me.food > 80 + GLASS.cost:
            if glassCount <= tankCount:
                agc.newAnt(GLASS)
            else:
                agc.newAnt(TANK)
    elif agc.me.food > 60 + DEFAULT.cost:
        totalCombat = glassCount + tankCount
        if speedyCount < totalCombat * 1.3:
            agc.newAnt(PEASANT)
        elif glassCount <= tankCount:
            agc.newAnt(GLASS)
        else:
            agc.newAnt(TANK)


def onGrab(ant: Ant):
    if not ant.isFull:
        ant.goTake(ant.nearestFreeFood())
    else:
        ant.goDeliver()


def onDeath(ant: Ant):
    global defaultCount, speedyCount, glassCount, tankCount, totalDeaths
    totalDeaths += 1
    if ant.id in flagDict:
        del flagDict[ant.id]
    if ant.type == DEFAULT:
        defaultCount -= 1
    elif ant.type == PEASANT:
        speedyCount -= 1
    elif ant.type == GLASS:
        glassCount -= 1
    elif ant.type == TANK:
        tankCount -= 1


def onNewAnt(ant: Ant):
    global defaultCount, speedyCount, glassCount, tankCount, totalBuilt
    totalBuilt += 1
    flagDict[ant.id] = {"attacking": False}
    ant.goTake(agc.nearestFreeFood())
    if ant.type == DEFAULT:
        defaultCount += 1
    elif ant.type == PEASANT:
        speedyCount += 1
    elif ant.type == GLASS:
        glassCount += 1
    elif ant.type == TANK:
        tankCount += 1


def onHurt(ant: Ant):
    target = findPriorityTarget(ant)
    if target is not None:
        ant.followAttack(target)


def onHit(ant: Ant):
    """Called when our ant's attack lands. Re-acquire best target (finish wounded, pick peasants)."""
    target = findPriorityTarget(ant)
    if target is not None:
        ant.followAttack(target)


agc.port = int(sys.argv[1]) if len(sys.argv) > 1 else 42069
agc.name = f'{AI_NAME}_{os.getpid()}'
agc.setCallback(onStart, "gameStart")
agc.setCallback(onFrame, "gameFrame")
agc.setCallback(onWait, "antWait")
agc.setCallback(onDeliver, "antDeliver")
agc.setCallback(onGrab, "antGrab")
agc.setCallback(onNewAnt, "antNew")
agc.setCallback(onHurt, "antHurt")
agc.setCallback(onHit, "antHit")
agc.setCallback(onDeath, "antDeath")
agc.connect()
