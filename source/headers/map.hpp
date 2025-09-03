#include <iostream>
#include <vector>
#include "network.hpp"
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

class Round
{
    public:
    static Round*instance;
    double secondsRunning = 0.0;
    double timeScale = 1.0; // Every non-native measure of seconds counts up by this number every real second
    unsigned char phase = 0; // 0 means not started
    Map*map;
    void open();
    void start();
    void step();
    void end();
};

class Map
{
    public:
    std::vector<Nest> nests;
    unsigned char nestc;
    Pos size;
    unsigned char** map;
    std::string encode(); // Returns a string that can be passed to decode() to copy this map.
    void decode(std::string); // Takes a string returned from encode() and copies that map to this instance.
    void prep(int); // Preps map before game. Assumes RoundSettings::instance is set.
};


class Nest
{
    public:
    Map*parent;
    unsigned int x;
    unsigned int y;
    std::vector<Ant*> ants;
};

class AntCommand
{
    unsigned char cmd;
    unsigned long arg;
};

class Ant
{
    public:
    Nest*parent;
    unsigned int x;
    unsigned int y;
    std::vector<AntCommand> commands;
    void giveCommand(AntCommand);
};
#endif
