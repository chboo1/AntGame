#include <iostream>
#include <vector>
#include <deque>
#include <chrono>
#include <string>
#include <csignal>
#include "network.hpp"
#ifndef MAP_HPP
#define MAP_HPP

class DPos;
class Pos
{
    public:
    unsigned short x;
    unsigned short y;
    Pos(){}
    Pos(DPos);
    Pos(unsigned short, unsigned short);
    Pos& operator=(DPos);
    bool operator==(Pos);
    bool operator!=(Pos);
    Pos operator-(Pos);
    Pos operator+(Pos);
};


class DPos
{
    public:
    double x;
    double y;
    DPos(){}
    DPos(Pos);
    DPos(double, double);
    DPos& operator=(Pos);
    DPos operator+(DPos);
    DPos operator-(DPos);
    double magnitude();
};

class Map;
class Nest;
class Ant;


class NestStats
{
    public:
    std::string name;
    double foodTaken;
    unsigned int antsMade;
    unsigned int peakAnts;
    double peakFood;
    unsigned char rank;
    double timeLasted;
    unsigned int kills;
};



class Round
{
    public:
    static Round*instance;
    ConnectionManager cm;
    double secondsRunning;
    std::chrono::steady_clock::time_point phaseEndTime; // Not affected by time scale
    std::chrono::steady_clock::time_point timeAtStart;
    std::chrono::steady_clock::time_point timeLastFrame;
    double deltaTime;
    bool logging = false;
    bool statsKeeping = false;
    std::vector<NestStats> deadNestStats;
    volatile static std::sig_atomic_t signalFlag;
#ifdef ERROR
#undef ERROR // This is problematic on some windows implementations, apparently
#endif
    enum Phase {INIT, WAIT, RUNNING, DONE, CLOSED, ERROR};
    Phase phase = INIT;
    Map*map;
    Round();
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
    enum class Tile : unsigned char {EMPTY, WALL, FOOD, NEST, UNKNOWN=255};
    static bool tileWalkable(Tile);
    bool tileWalkable(Pos);
    static bool tileEdible(Tile);
    bool tileEdible(Pos);
    std::vector<Nest*> nests;
    std::vector<Ant*> antPermanents;
    Pos size;
    unsigned char* map; // The raw map data. Should be size.x*size.y bytes. To access data at X, Y, index into this by [x+y*size.x].
    Map();
    void init(); // Uses RoundSettings::instance to get values
    std::string encode(); // Returns a string that can be passed to decode() to copy this map.
    void decode(std::string); // Takes a string returned from encode() and copies that map to this instance.
    void _decode(std::istream*);
    void freeMap();
    void cleanup();
    Tile operator[](Pos);
    ~Map();
};


class Nest
{
    public:
    class NestCommand
    {
        public:
        enum class ID : unsigned char {NEWANT};
        ID cmd;
        enum class State : unsigned char {ONGOING, SUCCESS, FAIL};
        State state;
        std::uint64_t arg;
    };
    Map*parent;
    Pos p;
    std::vector<Ant*> ants;
    std::deque<NestCommand> commands;
    double foodCount;
    std::string name;
    NestStats stats;
    Nest();
    Nest(Map*, Pos); // Takes a parent ptr and a position
    Nest(Map*, Pos, int); // Takes a parent ptr, a position and an ant count
    void init(Map*, Pos, int); // Takes a parent ptr, a position and an ant count
    void giveCommand(NestCommand);
    void step(double);
    void salute();
    void cleanup();
    Ant* createAnt(unsigned char);
    void killAnt(int);
    ~Nest();
};


class Ant
{
    public:
    struct AntCommand
    {
        enum class ID : unsigned char {MOVE, TINTERACT, AINTERACT, FOLLOW, CFOLLOW}; // FOLLOW is a utility for clients to use. The server never assigns it or uses it for behaviour
        ID cmd;
        enum class State : unsigned char {ONGOING, SUCCESS, FAIL};
        State state;
        std::uint64_t arg;
    };
    Nest*parent;
    DPos p;
    unsigned char type;
    unsigned int pid;
    std::deque<AntCommand> commands;
    double health;
    double foodCarry;
    Ant();
    Ant(Nest*, DPos); // Takes a parent ptr and a position. Defaults to type = 0
    Ant(Nest*, DPos, unsigned char); // Takes a parent ptr, a position and a type
    void init(Nest*, DPos, unsigned char); // Takes a parent ptr, a position and a type
    void _init(Nest*, DPos, unsigned char); // Takes a parent ptr, a position and a type
    void giveCommand(AntCommand);
    void step(double);

    struct AntType
    {
        double damageMod;
        double costMod;
        double healthMod;
        double speedMod;
        double capacity;
        double rangeMod;
    };

    static const constexpr AntType antTypes[] = {
        {1.0, 1.0, 1.0, 1.0, 3.0, 1.0}, // 0 -> BASE
        {2.0, 1.5, 0.5, 1.0, 3.0, 1.0}, // 1 -> GLASS CANON
        {0.5, 1.5, 2.0, 1.0, 3.0, 1.0} // 2 -> TANK
    };

    static const unsigned char antTypec = 3;
};
#endif
