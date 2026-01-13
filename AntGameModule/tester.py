import AntGame
agc = AntGame.AntGameClient()


def startFunction():
    print("Starting game, from python code!");
    print(agc.mapsize.x, agc.mapsize.y);
    print(agc.meID)
    print(agc.me)
    


agc.port = 42069
agc.name = "Python Client"
agc.setCallback(startFunction, "gameStart")
agc.connect()
