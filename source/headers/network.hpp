#include <string>
#include <vector>
#include <forward_list>
#include <chrono>
#include <cstdint>
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
    Player();
    Player(Connection*);
    Player(Viewer);
    ~Player();
};


class ConnectionManager
{
    public:
    class Command
    {
        enum class ID : unsigned char {MOVE, TINTERACT, AINTERACT, NEWANT};
        unsigned char nestID;
        signed short antID;
        ID cmd;
        unsigned long arg;
    };
    private:
    std::vector<Player*> players;
    std::forward_list<Viewer*> viewers;
    std::deque<Command> commands;

    void handleViewers();
    void handlePlayers();
    unsigned int getAGNPuint(std::string);
    std::string makeAGNPuint(std::uint32_t);
    bool httpResponse(Viewer*);
    bool sendResponse(Viewer*, std::string, std::string);
    bool isValid(Viewer*);
    bool isValid(Player*);
    bool playerGreeting(Viewer*);
    enum class RequestID : unsigned char {JOIN, NONE=0, PING, BYE, NAME, WALK, SETTINGS, TINTERACT, AINTERACT, NEWANT};
    enum class ResponseID : unsigned char {OK, DENY, PING, BYE, START, OKDATA, FAILURE};
    bool interpretRequests(Player*);
    public:
    ConnectionManager();
    void start();
    void step();
    void preclose();
    void reset();
};


class RoundSettings
{
    void Round::configLine(std::string identifier, std::string data);
    public:
    static RoundSettings* instance;
    std::string configFile;
    std::string mapFile;
    bool isPlayer;
    unsigned int port;
    double gameStartDelay;
    double timeScale;
    RoundSettings();
    void loadConfig(std::string config);
};
#endif
