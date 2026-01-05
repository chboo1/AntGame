#include <string>
#include <vector>
#include <forward_list>
#include <list>
#include <chrono>
#include <cstdint>
#include <deque>
#include "sockets.hpp"
#ifndef NETWORK_HPP
#define NETWORK_HPP
 

class Viewer
{
    public:
    Connection*conn;
    bool confirmed = false;
    bool toClose = false;
    std::chrono::steady_clock::time_point timeAtLastMessage;
    std::string unusedData = "";
    Viewer();
    Viewer(Connection*);
    ~Viewer();
};


class Player;
class Round;
class Nest;
class Ant;
class ConnectionManager
{
    friend Round;
    friend Nest;
    friend Ant;
    public:
    struct Command
    {
        enum class ID : unsigned char {MOVE, TINTERACT, AINTERACT, NEWANT};
        unsigned char nestID;
        unsigned int antID;
        ID cmd;
        std::uint64_t arg;
    };
    struct AntEvent
    {
        unsigned int pid;
        double health;
        double foodCarry;
    };
    struct MapEvent
    {
        unsigned short x;
        unsigned short y;
        unsigned char tile;
    };

    static unsigned int getAGNPuint(std::string);
    static std::string makeAGNPuint(std::uint32_t);
    static unsigned short getAGNPushort(std::string);
    static std::string makeAGNPushort(std::uint16_t);
    static double getAGNPshortdouble(unsigned int num);
    static unsigned int makeAGNPshortdouble(double num);
    static std::uint64_t makeAGNPdouble(double num);
    static double getAGNPdouble(std::uint64_t num);
    static std::string makeAGNPdoublestr(double num);
    static double getAGNPdoublestr(std::string);
    static std::string DEBUGstringToHex(std::string);

    private:
    std::vector<Player*> players;
    std::forward_list<Viewer*> viewers;
    std::deque<Command> commands;
    std::list<AntEvent> antEventQueue;
    std::list<MapEvent> mapEventQueue;

    void handleViewers();
    void handlePlayers();
    bool httpResponse(Viewer*);
    bool sendResponse(Viewer*, std::string, std::string);
    bool _sendResponse(Viewer*, std::string, std::string);
    bool isValid(Viewer*);
    bool isValid(Player*);
    bool playerGreeting(Viewer*);
    bool interpretRequests(Player*);
    public:
    enum class RequestID : unsigned char {JOIN, NONE=0, PING, BYE, NAME, WALK, SETTINGS, TINTERACT, AINTERACT, NEWANT, MAP, CHANGELOG, ME, CANCEL};
    enum class ResponseID : unsigned char {OK, DENY, PING, BYE, START, OKDATA, FAILURE, CMDSUCCESS, CMDFAIL};
    ConnectionManager();
    void start();
    void step();
    void preclose();
    void reset();
};


class Player
{
    public:
    Connection*conn;
    bool toClose = false;
    bool pinged = false;
    bool bye = false;
    unsigned char nestID;
    std::chrono::steady_clock::time_point timeAtLastMessage;
    std::string unusedData = "";
    std::string name = "";
    unsigned int messageSizeLeft = 0;
    unsigned int messageRequestsLeft = 0;
    std::list<ConnectionManager::AntEvent>::iterator antEventPos;
    std::list<ConnectionManager::MapEvent>::iterator mapEventPos;
    Player();
    Player(Connection*);
    Player(Viewer*);
    ~Player();
};


class RoundSettings
{
    void configLine(std::string identifier, std::string data);
    public:
    static RoundSettings* instance;
    std::string configFile;
    std::string mapFile;
    bool isPlayer;
    unsigned int port;
    double gameStartDelay;
    double timeScale;
    double startingFood;
    double movementSpeed;
    double hungerRate;
    double foodYield;
    double foodTheftYield;
    double antCost;
    double attackRange;
    double attackDamage;
    double antHealth;
    double pickupRange;
    double capacityMod;
    RoundSettings();
    void loadConfig(std::string config);
    void _loadConfig(std::string config);
};
#endif
