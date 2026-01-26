import AntGame
agc = AntGame.AntGameClient()

defaultAnt = AntGame.AntType(0)
attackerAnt = AntGame.AntType(1)


def onStart():
    global antFoodCost
    antFoodCost = defaultAnt.cost
    for ant in agc.me.ants:
        ant.goTake(agc.nearestFreeFood())


def onWait(ma):
    if ma.type == attackerAnt:
        ma.followAttack(ma.nearestEnemy())


def onDeliver(ma):
    ma.goTake(agc.nearestFreeFood())
    if len(agc.me.ants) % 10 == 0:
        if agc.me.food > 60 + defaultAnt.cost:
            agc.newAnt(defaultAnt)
    else:
        if agc.me.food > 60 + attackerAnt.cost:
            agc.newAnt(attackerAnt)


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
agc.name = "Python Client"
agc.setCallback(onWait, "antWait")
agc.setCallback(onDeliver, "antDeliver")
agc.setCallback(onGrab, "antGrab")
agc.setCallback(onNewAnt, "antNew")
agc.setCallback(onStart, "gameStart")
agc.connect()
