import AntGame
import os
import math
agc = AntGame.AntGameClient()

defaultAnt = AntGame.AntType(0)


def nextFood(a):
    if a.isFull:
        a.goDeliver()
    else:
        a.goTake(a.nearestFreeFood())


def onStart():
    for ant in agc.me.ants:
        ant.goTake(agc.nearestFreeFood())


def onFrame():
    if len(agc.me.ants) > 250 and agc.me.food > 300 and len(agc.enemies) > 0:
        for ant in agc.me.ants:
            ant.followAttack(ant.nearestEnemy())
        agc.setCallback(onFrameAttack, "gameFrame")
        agc.setCallback(None, "antDeliver")
        agc.setCallback(None, "antGrab")
        agc.setCallback(None, "antNew")
        agc.setCallback(None, "antHit")
        agc.setCallback(onWaitAttack, "antWait")

    for ant in agc.me.ants:
        if ant.nearestEnemy() is not None and ant.pos.dist(ant.nearestEnemy().pos) < defaultAnt.attackrange * 3:
            if ant.target != ant.nearestEnemy():
                ant.followAttack(ant.nearestEnemy())


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
    ma.goTake(agc.nearestFreeFood())


def onWait(ma):
    nextFood(ma)


def onHurt(ma):
    ma.followAttack(ma.nearestEnemy())


def onHit(ma):
    if ant.nearestEnemy() is None:
        ma.goDeliver()
    elif ant.pos.dist(ant.nearestEnemy().pos) > defaultAnt.attackRange * 3:
        ma.goDeliver()
    else:
        ma.followAttack(ma.nearestEnemy())




def onWaitAttack(ma):
    print("Waiting")
    ma.followAttack(ma.nearestEnemy())
    if ma.nearestEnemy() is None:
        agc.setCallback(onDeliver, "antDeliver")
        agc.setCallback(onGrab, "antGrab")
        agc.setCallback(onNewAnt, "antNew")
        agc.setCallback(onWait, "antWait")
        agc.setCallback(onFrame, "gameFrame")
        for ant in agc.me.ants:
            if ant.isFull:
                ant.goDeliver()
            else:
                ant.goTake(agc.nearestFreeFood())


def onFrameAttack():
    for ant in agc.me.ants:
        ant.followAttack(ant.nearestEnemy())
    if len(agc.me.ants) < 1:
        agc.setCallback(onDeliver, "antDeliver")
        agc.setCallback(onGrab, "antGrab")
        agc.setCallback(onNewAnt, "antNew")
        agc.setCallback(onWait, "antWait")
        agc.setCallback(onFrame, "gameFrame")
        agc.setCallback(onHurt, "antHurt")
        agc.setCallback(onHit, "antHit")
        for i in range(0, math.floor((agc.me.food-60) / defaultAnt.cost)):
            agc.newAnt(defaultAnt)



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
agc.connect()
