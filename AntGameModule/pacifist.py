import AntGame
import os
agc = AntGame.AntGameClient()

defaultAnt = AntGame.AntType(0)


def onStart():
    global antFoodCost
    antFoodCost = defaultAnt.cost
    for ant in agc.me.ants:
        ant.goTake(agc.nearestFreeFood())


def onDeliver(ma):
    ma.goTake(agc.nearestFreeFood())
    if agc.me.food > 60 + defaultAnt.cost:
        agc.newAnt(defaultAnt)


def onGrab(ma):
    if not ma.isFull:
        target = ma.nearestFreeFood()
        if target is not None:
            ma.goTake(target)
    else:
        ma.goDeliver()


def onNewAnt(ma):
    if ma.type == defaultAnt:
        ma.goTake(agc.nearestFreeFood())



agc.port = 42069
agc.name = os.path.basename(__file__)
agc.setCallback(onDeliver, "antDeliver")
agc.setCallback(onGrab, "antGrab")
agc.setCallback(onNewAnt, "antNew")
agc.setCallback(onStart, "gameStart")
agc.connect()
