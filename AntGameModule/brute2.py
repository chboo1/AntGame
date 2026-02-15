from AntGame import *
import os
agc = AntGameClient()

defaultAnt = AntType(0)
attackerAnt = AntType(1)


def onFrame():
    for ant in agc.me.ants:
        if ant.type == attackerAnt:
            ant.followAttack(ant.nearestEnemy())


def onStart():
    global antFoodCost
    antFoodCost = defaultAnt.cost
    for ant in agc.me.ants:
        ant.goTake(agc.nearestFreeFood())


def onWait(ma: Ant):
    if ma.type == defaultAnt:
        if not ma.isFull:
            ma.goTake(ma.nearestFreeFood())
        else:
            ma.goDeliver()
#    else:
#        ma.followAttack(ma.nearestEnemy())


def onDeliver(ma: Ant):
    ma.goTake(agc.nearestFreeFood())
    if len(agc.me.ants) % 10 == 0:
        if agc.me.food > 60 + defaultAnt.cost:
            agc.newAnt(defaultAnt)
    else:
        if agc.me.food > 60 + attackerAnt.cost:
            agc.newAnt(attackerAnt)


def onGrab(ma: Ant):
    if not ma.isFull:
        ma.goTake(ma.nearestFreeFood())
    else:
        ma.goDeliver()


def onNewAnt(ma: Ant):
    if ma.type == defaultAnt:
        ma.goTake(agc.nearestFreeFood())


def onHurt(ma: Ant):
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
agc.connect()
