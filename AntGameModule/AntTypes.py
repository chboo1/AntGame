import enum
from AntGame import *

class AntTypes(enum.Enum):
    # normal Ant
    DEFAULT_ANT = AntType(0)

    # Ant with lower health but higher attack
    GLASS_CANON_ANT = AntType(1)

    # Ant with higher health but lower attack
    TANK_ANT = AntType(2)

    # Ant with practically no health but much higher speed and higher food carrying capacity
    PEASANT_ANT = AntType(3)
