from typing import List

class Pos:
    """A position."""
    x:float = 0.0
    y:float = 0.0
    def dist(self, other):
        """Returns the distance to another position."""
        pass



class AntGameClient:
    """The central class within which all the game's data is stored."""
    @property
    def nests(self) -> List[Nest]:
        """An array of every nest in the map."""
        pass
    @property
    def ants(self) -> List[Ant]:
        """An array of every ant in the map."""
        pass
    @property
    def enemies(self) -> List[Ant]:
        """An array of every enemy ant in the map."""
        pass
    @property
    def meID(self) -> int:
        """The index of this player's nest in the nests."""
        pass
    @property
    def me(self) -> Nest:
        """This player's nest."""
        pass
    @property
    def addr(self) -> str:
        """The address we should aim to connect to."""
        pass
    port:int = 55521
    """The port through which we should aim to connect to the server."""
    @property
    def name(self) -> str:
        """The name of this bot."""
        pass
    @property
    def mapsize(self) -> Pos:
        """The size of the map."""
        pass


    def connect(self) -> None:

        """Connect to the server and start the game. Only returns once the game is over. Returns None."""
        pass

    def setCallback(self, callback : function, type : str) -> None:

        """Set a function as a callback during game running. There are two types of callbacks; game callbacks and ant callbacks. The former take no arguments and the latter take a singular ant which is the subject of the callback. callback should be a callable function with the correct argument list according to type and type should be one of the following:
    gameStart - Called at game start, no arguments.
    gameFrame - Called every frame, no arguments.
    antNew - Called every time a friendly ant is created, one argument (created ant).
    antGrab - Called every time a friendly ant grabs food, one argument (grabber ant).
    antDeliver - Called every time a friendly ant delivers food to the nest, one argument (deliverer ant).
    antHurt - Called every time a friendly ant is hurt, one argument (hurt ant).
    antHit - Called every time a friendly ant hits another, one argument (hitter ant).
    antFull - Called every time a friendly ant has maximum food, one argument (full ant).
    antWait - Called every frame for friendly ants that have no commands, one argument (waiting ant).
    antDeath - Called every time a friendly ant dies, one argument (dead ant)."""
        pass

    def newAnt(self, type : AntType) -> None:
        """Creates a new ant with the type given as argument."""
        pass

    def nearestFood(self) -> Pos:
        """Gets the closest food object to the nest."""
        pass

    def nearestFreeFood(self) -> Pos:
        """Gets the closest food object to the nest that no friendly ant is currently going after."""
        pass



class Nest:
    """A nest within the map."""
    def __eq__(self, other):
        """Checks if this nest is the same as another nest."""
        pass

    def __ne__(self, other):
        """Checks if this nest is not the same as another nest."""
        pass

    @property
    def ants(self) -> List[Ant]:
        """An array of each ant in this nest."""
        pass

    @property
    def pos(self) -> Pos:
        """The nest's position."""
        pass

    @property
    def food(self) -> float:
        """The nest's food."""
        pass



class Ant:
    """An ant within the map."""
    @property
    def pos(self) -> Pos:
        """The ant's position."""
        pass

    @property
    def health(self) -> float:
        """The ant's remaining health points."""
        pass

    @property
    def food(self) -> float:
        """The ant's current carried food."""
        pass

    @property
    def type(self) -> AntType:
        """The ant's type."""
        pass

    @property
    def nest(self) -> Nest:
        """The ant's parent nest."""
        pass

    @property
    def target(self) -> Ant:
        """When this ant is currently following, following to attack or moving to attack another ant, this is that ant."""
        pass

    @property
    def isFriend(self) -> bool:
        """Whether this ant is friendly (from your nest)."""
        pass

    @property
    def isEnemy(self) -> bool:
        """Whether this ant is an enemy (not from your nest)."""
        pass

    @property
    def isFull(self) -> bool:
        """Whether this ant has as much food as it could possibly have."""
        pass

    @property
    def isAlive(self) -> bool:
        """Whether this ant is alive."""
        pass

    @property
    def isFrozen(self) -> bool:
        """Whether this ant is a frozen snapshot. (This will always be true of ants returned by antDeath callbacks, but can be manually enabled with the freeze() method.)"""
        pass

    @property
    def isSafe(self) -> bool:
        """Of a frozen ant, whether or not it is safe to call unfreeze on it (i.e., if the ant that was frozen still exists.) Of a non-frozen ant, same as isAlive."""
        pass


    def __eq__(self, other) -> bool:
        """Compares two ants to see if they are the same."""
        pass

    def __ne__(self, other) -> bool:
        """Compares two ants to see if they are not the same."""
        pass


    def move(self, pos : Pos) -> None:
        """Send an instruction to an ant to move to the specified position."""
        pass

    def take(self, pos : Pos) -> None:
        """Send an instruction to an ant to take food from the specified position."""
        pass

    def goTake(self, pos : Pos) -> None:
        """Send an instruction to an ant to move to and take food from the specified position."""
        pass

    def attack(self, ant : Ant) -> None:
        """Send an instruction to an ant to attack the specified ant."""
        pass

    def goAttack(self, ant : Ant) -> None:
        """Send an instruction to an ant to go to the current position of and try to attack the specified ant."""
        pass

    def follow(self, ant : Ant) -> None:
        """Send an instruction to an ant follow the specified ant (move towards it until told to stop)."""
        pass

    def followAttack(self, ant : Ant) -> None:
        """Send an instruction to an ant follow and attack the specified ant (move towards it until the attack succeeds)."""
        pass

    def deliver(self) -> None:
        """Send an instruction to an ant to try to deliver its food to the nest (without moving)."""
        pass

    def goDeliver(self) -> None:
        """Send an instruction to an ant to go to and deliver its food to the nest."""
        pass

    def stop(self) -> None:
        """Stop all instructions of this ant. Should never fail."""
        pass

    def nearestFood(self) -> Pos:
        """Find the nearest food to this ant."""
        pass

    def nearestFreeFood(self) -> Pos:
        """Find the nearest food to this ant that no friendly ant is targetting."""
        pass

    def nearestAnt(self) -> Pos:
        """Find the nearest other ant to this ant."""
 
    def nearestEnemy(self) -> Pos:
        """Find the nearest enemy ant to this ant."""
        pass
 
    def nearestFriend(self) -> Pos:
        """Find the nearest friendly ant to this ant."""
        pass

    def freeze(self) -> None:
        """Take this python object and freeze it to represent this exact point in time of the ant. By default, an ant python object refers to an ant and will always refer to the most recent position and values of that ant, but not when frozen. It constitutes an error to send an instruction to a frozen ant or to unfreeze an ant that has died. To check for the latter, call isSafe(). By default, all ants passed through the antDeath callback are frozen, and since they are always then dead, they should not be unfrozen."""
        pass

    def unfreeze(self) -> None:
        """Unfreeze a frozen ant, such that it once more refers to the present version of that ant. A warning will be raised if this ant is now dead, and this function will do nothing. See freeze() for more info on frozen ants."""
        pass



class AntType:
    """A type of ant."""

    def __init__(self, type: int):
        """Initialize ant of specified type"""

    def __eq__(self, other) -> bool:
        """Checks whether this and another type are the same."""
        pass

    def __ne__(self, other) -> bool:
        """Checks whether this and another type are not the same."""
        pass

    @property
    def maxfood(self) -> float:
        """The max carried food of this ant type."""
        pass

    @property
    def maxhealth(self) -> float:
        """The maximum health of this ant type."""
        pass

    @property
    def damage(self) -> float:
        """The damage dealt by this ant type."""
        pass

    @property
    def cost(self) -> float:
        """The amount of food needed to make this ant type."""
        pass

    @property
    def speed(self) -> float:
        """The speed of this ant type."""
        pass

    @property
    def attackrange(self) -> float:
        """The attack range of this ant type."""
        pass

    @property
    def pickuprange(self) -> float:
        """The pick up range of this ant type."""
        pass

    @property
    def attackrate(self) -> float:
        """The minimum time in seconds between attacks for this ant type."""
        pass
