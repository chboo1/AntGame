from AntGame import *
import os
import math
agc = AntGameClient()

defaultAnt = AntType(0)
speedyAnt = AntType(3)


def nextFood(a: Ant):
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


def onDeliver(ma: Ant):
    ma.goTake(agc.nearestFreeFood())
    if agc.me.food > 60 + defaultAnt.cost:
        if len(agc.me.ants) % 2 == 0:
            agc.newAnt(defaultAnt)
        else:
            agc.newAnt(speedyAnt)


def onGrab(ma: Ant):
    if not ma.isFull:
        ma.goTake(ma.nearestFreeFood())
    else:
        ma.goDeliver()


def onNewAnt(ma: Ant):
    ma.goTake(agc.nearestFreeFood())


def onWait(ma: Ant):
    nextFood(ma)


def onHurt(ma: Ant):
    ma.followAttack(ma.nearestEnemy())


def onHit(ma: Ant):
    if ma.nearestEnemy() is None:
        ma.goDeliver()
    elif ma.pos.dist(ma.nearestEnemy().pos) > defaultAnt.attackRange * 3:
        ma.goDeliver()
    else:
        ma.followAttack(ma.nearestEnemy())




def onWaitAttack(ma: Ant):
    print("Waiting")
    ma.followAttack(ma.nearestEnemy())
    if ma.nearestEnemy() is None:
        agc.setCallback(onDeliver, "antDeliver")
        agc.setCallback(onGrab, "antGrab")
        agc.setCallback(onNewAnt, "antNew")
        agc.setCallback(onWait, "antWait")
        agc.setCallback(onFrame, "gameFrame")
        agc.setCallback(onHurt, "antHurt")
        agc.setCallback(onHit, "antHit")
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
