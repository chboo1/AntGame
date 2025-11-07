#ifndef ANTTYPE_HPP
#define ANTTYPE_HPP

struct AntType
{
    double damageMod;
    double costMod;
    double healthMod;
    double speedMod;
    double capacity;
};

const AntType antTypes[] = {
    {1.0, 1.0, 1.0, 1.0, 3.0}, // 0 -> BASE
    {2.0, 1.5, 0.5, 1.0, 3.0}, // 1 -> GLASS CANON
    {0.5, 1.5, 2.0, 1.0, 3.0} // 2 -> TANK
};

const unsigned char antTypec = 3;

#endif
