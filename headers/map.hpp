#include <iostream>
#include <vector>
#ifndef MAP_HPP
#define MAP_HPP

class Pos
{
    unsigned int x;
    unsigned int y;
};

class Map;
class Nest;
class Ant;

class RoundInfo
{
    double secondsRunning = 0.0;
    double timeScale = 1.0; // Every non-native measure of seconds counts up by this number every real second
    unsigned char phase = 0; // 0 means not started
    Map*map;
};

class Map
{
    public:
    RoundInfo*parent;
    std::vector<Nest> nests;
    unsigned char nestc;
    Pos size;
    unsigned char** map;
};


class Nest
{
    public:
    Map*parent;
    unsigned int x;
    unsigned int y;
    std::vector<Ant> ants;
};

class AntCommand
{
    unsigned char cmd;
    unsigned long arg;
};

class Ant
{
    public:
    unsigned int x;
    unsigned int y;
    std::vector<AntCommand> commands;
};
#endif