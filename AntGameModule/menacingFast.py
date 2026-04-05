from AntGame import *
from AntTypes import *
import os
import math
agc = AntGameClient()

defaultAntAmount = 5
speedyAntAmount = 0

flagDict = dict()


def nextFood(a: Ant):
    if a.isFull:
        a.goDeliver()
    else:
        a.goTake(a.nearestFreeFood())


def onStart():
    global flagDict
    for ant in agc.me.ants:
        ant.goTake(agc.nearestFreeFood())
        flagDict[ant.id] = {"attacking":False}


def onFrame():
    global flagDict
    if len(agc.me.ants) > 250 and agc.me.food > 300 and len(agc.enemies) > 0:
        for ant in agc.me.ants:
            if flagDict.get(ant.id) is None:
                flagDict[ant.id] = {"attacking":False}
            if ant.type == AntTypes.DEFAULT_ANT.value:
                flagDict[ant.id]["attacking"] = True
                ant.followAttack(ant.nearestEnemy())

    if len(agc.me.ants) < 1:
        for _ in range(0, math.floor((agc.me.food-60) / AntTypes.PEASANT_ANT.value.cost)):
            agc.newAnt(AntTypes.PEASANT_ANT.value)

    for ant in agc.me.ants:
        if flagDict.get(ant.id) is None:
            flagDict[ant.id] = {"attacking":False}
        if not flagDict[ant.id]["attacking"]:
            onFrameNormal(ant)
        else:
            onFrameAttack(ant)


def onFrameNormal(ant: Ant):
    if ant.nearestEnemy() is not None and ant.pos.dist(ant.nearestEnemy().pos) < AntTypes.DEFAULT_ANT.value.attackrange * 3:
        if ant.target != ant.nearestEnemy():
            ant.followAttack(ant.nearestEnemy())


def onFrameAttack(ant: Ant):
    pass


def onWait(ma: Ant):
    global flagDict
    if flagDict[ma.id]["attacking"]:
        onWaitAttack(ma)
    else:
        onWaitNormal(ma)


def onWaitNormal(ma: Ant):
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
    if agc.me.food > 60 + AntTypes.DEFAULT_ANT.value.cost:
        if speedyAntAmount > defaultAntAmount:
            agc.newAnt(AntTypes.DEFAULT_ANT.value)
        else:
            agc.newAnt(AntTypes.PEASANT_ANT.value)


def onGrab(ma: Ant):
    if not ma.isFull:
        ma.goTake(ma.nearestFreeFood())
    else:
        ma.goDeliver()


def onDeath(ma: Ant):
    global defaultAntAmount
    global speedyAntAmount
    global flagDict
    if ma.id in flagDict:
        del flagDict[ma.id]
    if ma.type == AntTypes.DEFAULT_ANT.value:
        defaultAntAmount -= 1
    else:
        speedyAntAmount -= 1


def onNewAnt(ma : Ant):
    global defaultAntAmount
    global speedyAntAmount
    global flagDict
    flagDict[ma.id] = {"attacking":False}
    ma.goTake(agc.nearestFreeFood())
    if ma.type == AntTypes.DEFAULT_ANT.value:
        defaultAntAmount += 1
    else:
        speedyAntAmount += 1


def onHurt(ma: Ant):
    ma.followAttack(ma.nearestEnemy())


def onHit(ma: Ant):
    print("onHit")
    global flagDict
    if not flagDict[ma.id]["attacking"]:
        if ma.nearestEnemy() is None:
            ma.goDeliver()
        elif ma.pos.dist(ma.nearestEnemy().pos) > AntTypes.DEFAULT_ANT.value.attackRange * 3:
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
