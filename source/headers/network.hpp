#include <string>
#include <vector>
#include <forward_list>
#include <chrono>
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
    unsigned char nestID;
    std::chrono::steady_clock::time_point timeAtLastMessage;
    std::string unusedData = "";
    Player();
    Player(Connection*);
    Player(Viewer);
    ~Player();
};


class ConnectionManager
{
    std::vector<Player*> players;
    std::forward_list<Viewer*> viewers;
    void handleViewers();
    bool httpResponse(Viewer*);
    bool playerGreeting(Viewer*);
    bool sendResponse(std::string, std::string);
    bool isValid(Viewer*);
    public:
    struct Command
    {
        unsigned char nestID;
        unsigned int antID;
        unsigned char cmd;
        unsigned long arg;
    };
    ConnectionManager();
    void step();
    void reset();
};


class RoundSettings
{
    public:
    static RoundSettings* instance;
    std::string mapFile;
    bool isPlayer;
    int port;
    RoundSettings();
};
#endif
