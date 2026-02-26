from AntGame import *
import os
import math
agc = AntGameClient()

defaultAnt = AntType(0)
speedyAnt = AntType(3)

defaultAntAmount = 5
speedyAntAmount = 0

flagDict = {}


def nextFood(a: Ant):
    if a.isFull:
        a.goDeliver()
    else:
        a.goTake(a.nearestFreeFood())


def onStart():
    for ant in agc.me.ants:
        ant.goTake(agc.nearestFreeFood())
        flagDict[ant.id] = {"attacking":False}


def onFrame():
    global flagDict
    if len(agc.me.ants) > 250 and agc.me.food > 300 and len(agc.enemies) > 0:
        for ant in agc.me.ants:
            if flagDict.get(ant.id) is None:
                flagDict[ant.id] = {"attacking":False}
            if ant.type == defaultAnt:
                flagDict[ant.id]["attacking"] = True
                ant.followAttack(ant.nearestEnemy())

    if len(agc.me.ants) < 1:
        for i in range(0, math.floor((agc.me.food-60) / speedyAnt.cost)):
            agc.newAnt(speedyAnt)

    for ant in agc.me.ants:
        if flagDict.get(ant.id) is None:
            flagDict[ant.id] = {"attacking":False}
        if not flagDict.get(ant.id)["attacking"]:
            onFrameNormal(ant)
        else:
            onFrameAttack(ant)


def onFrameNormal(ant):
    if ant.nearestEnemy() is not None and ant.pos.dist(ant.nearestEnemy().pos) < defaultAnt.attackrange * 3:
        if ant.target != ant.nearestEnemy():
            ant.followAttack(ant.nearestEnemy())


def onFrameAttack(ant):
    pass


def onWait(ma):
    if flagDict[ma.id]["attacking"]:
        onWaitAttack(ma)
    else:
        onWaitNormal(ma)


def onWaitNormal(ma):
    nextFood(ma)


def onWaitAttack(ma):
    global flagDict
    ma.followAttack(ma.nearestEnemy())
    if ma.nearestEnemy() is None:
        for ant in agc.me.ants:
            if flagDict[ma.id]["attacking"]:
                flagDict[ma.id]["attacking"] = False
                if ant.isFull:
                    ant.goDeliver()
                else:
                    ant.goTake(agc.nearestFreeFood())


def onDeliver(ma: Ant):
    global defaultAntAmount
    global speedyAntAmount
    ma.goTake(agc.nearestFreeFood())
    if agc.me.food > 60 + defaultAnt.cost:
        if speedyAntAmount > defaultAntAmount:
            agc.newAnt(defaultAnt)
        else:
            agc.newAnt(speedyAnt)


def onGrab(ma: Ant):
    if not ma.isFull:
        ma.goTake(ma.nearestFreeFood())
    else:
        ma.goDeliver()


def onDeath(ma : Ant):
    global defaultAntAmount
    global speedyAntAmount
    global flagDict
    if flagDict.get(ma.id) is not None:
        del flagDict[ma.id]
    if ma.type == defaultAnt:
        defaultAntAmount -= 1
    else:
        speedyAntAmount -= 1


def onNewAnt(ma : Ant):
    global defaultAntAmount
    global speedyAntAmount
    global flagDict
    flagDict[ma.id] = {"attacking":False}
    ma.goTake(agc.nearestFreeFood())
    if ma.type == defaultAnt:
        defaultAntAmount += 1
    else:
        speedyAntAmount += 1


def onHurt(ma: Ant):
    ma.followAttack(ma.nearestEnemy())


def onHit(ma : Ant):
    print("onHit")
    global flagDict
    if not flagDict[ma.id]["attacking"]:
        if ma.nearestEnemy() is None:
            ma.goDeliver()
        elif ant.pos.dist(ant.nearestEnemy().pos) > defaultAnt.attackRange * 3:
            ma.goDeliver()
        else:
            ma.followAttack(ma.nearestEnemy())





agc.port = 42069
agc.name = os.path.basename(__file__)
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
