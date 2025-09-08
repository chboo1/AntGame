#include <iostream>
#include <vector>
#include <chrono>
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
    double secondsRunning;
    std::chrono::steady_clock::time_point timeAtStart;
    double timeScale = 1.0; // Every non-native measure of seconds counts up by this number every real second
    unsigned char phase = 0; // 0 means not started
    Map*map;
    Round();
    void open();
    void start();
    void step();
    void end();
    void reset(); // Goes back to state after constructor but before open
};

class Map
{
    public:
    std::vector<Nest*> nests;
    Pos size;
    unsigned char* map; // The raw map data. Should be size.x*size.y bytes. To access data at X, Y, index into this by [x+y*size.x].
    Map(); // Uses RoundSettings::instance to get values
    void init(); // Uses RoundSettings::instance to get values
    std::string encode(); // Returns a string that can be passed to decode() to copy this map.
    void decode(std::string); // Takes a string returned from encode() and copies that map to this instance.
    void freeMap();
    ~Map();
};


class Nest
{
    public:
    Map*parent;
    Pos p;
    std::vector<Ant*> ants;
    Nest();
    Nest(Map*, Pos); // Takes a parent ptr and a position
    Nest(Map*, Pos, int); // Takes a parent ptr, a position and an ant count
    void init(Map*, Pos, int); // Takes a parent ptr, a position and an ant count
    void cleanup();
    ~Nest();
};

struct AntCommand
{
    unsigned char cmd;
    unsigned long arg;
};

class Ant
{
    public:
    Nest*parent;
    Pos p;
    unsigned char type;
    Ant();
    Ant(Nest*, Pos); // Takes a parent ptr and a position. Defaults to type = 0
    Ant(Nest*, Pos, unsigned char); // Takes a parent ptr, a position and a type
    void init(Nest*, Pos, unsigned char); // Takes a parent ptr, a position and a type
    std::vector<AntCommand> commands;
    void giveCommand(AntCommand);
    void step(double);
};
#endif
