#ifndef TILES_HPP
#define TILES_HPP

#define TILE_EMPTY 0
#define TILE_WALL 1
#define TILE_NEST 2
#define TILE_UNKNOWN 255


bool isWalkable(unsigned char tile)
{
    if (tile == TILE_UNKNOWN)
    {
        throw std::invalid_argument("Can't determine if tile is walkable");
    }
    if (tile == TILE_WALL)
    {
        return false;
    }
    return false;
}


#endif
