#include <string>
#ifndef NETWORK_HPP
#define NETWORK_HPP


class Player // TODO... eventually, I guess
{
};


class RoundSettings
{
    public:
    static RoundSettings* instance;
    std::string mapFile;
    bool isPlayer;
    RoundSettings();
};
#endif
