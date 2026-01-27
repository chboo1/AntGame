import AntGame
import os
agc = AntGame.AntGameClient()

defaultAnt = AntGame.AntType(0)
attackMode = False


def onStart():
    global antFoodCost
    antFoodCost = defaultAnt.cost
    for ant in agc.me.ants:
        ant.goTake(agc.nearestFreeFood())


def onFrame():
    global attackMode
    if len(agc.me.ants) > 250 and agc.me.food > 300:
        attackMode = True
        for ant in agc.me.ants:
            ant.stop()
            ant.followAttack(ant.nearestEnemy())
    for ant in agc.me.ants:
        if attackMode or ant.pos.dist(ant.nearestEnemy().pos) < defaultAnt.attackrange * 1.5:
            if ant.target != ant.nearestEnemy():
                ant.followAttack(ant.nearestEnemy())


def onWait(ma):
    global attackMode
    if attackMode:
        ma.followAttack(ma.nearestEnemy())


def onDeliver(ma):
    ma.goTake(agc.nearestFreeFood())
    if agc.me.food > 60 + defaultAnt.cost:
        agc.newAnt(defaultAnt)


def onGrab(ma):
    if not ma.isFull:
        ma.goTake(ma.nearestFreeFood())
    else:
        ma.goDeliver()


def onNewAnt(ma):
    if ma.type == defaultAnt:
        ma.goTake(agc.nearestFreeFood())


def onHurt(ma):
    ma.followAttack(ma.nearestEnemy())



agc.port = 42069
agc.name = os.path.basename(__file__)
agc.setCallback(onDeliver, "antDeliver")
agc.setCallback(onGrab, "antGrab")
agc.setCallback(onNewAnt, "antNew")
agc.setCallback(onStart, "gameStart")
agc.setCallback(onFrame, "gameFrame")
agc.setCallback(onWait, "antWait")
agc.setCallback(onHurt, "antHurt")
agc.connect()
