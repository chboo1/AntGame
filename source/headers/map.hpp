#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
#include <string>
#include "network.hpp"
#ifndef MAP_HPP
#define MAP_HPP


class Pos
{
    public:
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
    ConnectionManager cm;
    double secondsRunning;
    std::chrono::steady_clock::time_point phaseEndTime; // Not affected by time scale
    std::chrono::steady_clock::time_point timeAtStart;
    enum Phase {INIT, WAIT, RUNNING, DONE, CLOSED, ERROR};
    Phase phase = INIT;
    Map*map;
    Round();
    Round(std::string); // Uses a config file
    ~Round();
    bool open();
    bool open(int);
    bool open(std::string);
    bool open(std::string, int); // Start to allow people to join. Arguments: std::string config - A string representing the pathname to the configuration file to use for this round. int port - The port to run the server on. Both are optional
    void start(); // Start the game
    void step(); // Progress a frame
    void end(); // End the game
    void reset(); // Goes back to state after constructor but before open
};

class Map
{
    public:
    std::vector<Nest*> nests;
    Pos size;
    unsigned char* map; // The raw map data. Should be size.x*size.y bytes. To access data at X, Y, index into this by [x+y*size.x].
    Map();
    void init(); // Uses RoundSettings::instance to get values
    std::string encode(); // Returns a string that can be passed to decode() to copy this map.
    void decode(std::string); // Takes a string returned from encode() and copies that map to this instance.
    void _decode(std::istream);
    void freeMap();
    void cleanup();
    ~Map();
};


class Nest
{
    public:
    class NestCommand
    {
        enum class ID : unsigned char {DONE, NEWANT};
        ID cmd;
        unsigned long arg;
    };
    Map*parent;
    Pos p;
    std::vector<Ant*> ants;
    std::deque<NestCommand> commands;
    double foodCount;
    Nest();
    Nest(Map*, Pos); // Takes a parent ptr and a position
    Nest(Map*, Pos, int); // Takes a parent ptr, a position and an ant count
    void init(Map*, Pos, int); // Takes a parent ptr, a position and an ant count
    void giveCommand(NestCommand);
    void step(double);
    void cleanup();
    ~Nest();
};


class Ant
{
    public:
    class AntCommand
    {
        enum class ID : unsigned char {DONE, MOVE, TINTERACT, AINTERACT};
        ID cmd;
        unsigned long arg;
    };
    Nest*parent;
    Pos p;
    unsigned char type;
    std::deque<AntCommand> commands;
    Ant();
    Ant(Nest*, Pos); // Takes a parent ptr and a position. Defaults to type = 0
    Ant(Nest*, Pos, unsigned char); // Takes a parent ptr, a position and a type
    void init(Nest*, Pos, unsigned char); // Takes a parent ptr, a position and a type
    void giveCommand(AntCommand);
    void step(double);
};
#endif
