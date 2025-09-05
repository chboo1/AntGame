#include "../headers/map.hpp"
#include <chrono>
#include <iostream>
#include <vector>


Round* Round::instance = nullptr;



Round::Round()
{
    if (Round::instance == nullptr)
    {
        Round::instance = this;
    }
    secondsRunning = 0;
}
void Round::open()
{
}
void Round::start()
{
}
void Round::step()
{
    std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
    double oldRunningTime = secondsRunning;
    secondsRunning = ((std::chrono::duration<double>)(t - timeAtStart)).count() * timeScale; 
    double delta = secondsRunning - oldRunningTime;
}
void Round::end()
{
}
void Round::reset() // Goes back to state after constructor but before open
{
}


Map::Map()
{
}
void Map::init() // Uses RoundSettings::instance to get values
{
    if (RoundSettings::instance == nullptr)
    {
        return;
    }
    if (RoundSettings::instance->mapFile == "")
    {
         return; // Neither of these statements are errors. Setting mapFile to an empty string is the proper way to cause this function to leave without changing anything. 
    }
    std::ifstream mapFile(RoundSettings::instance->mapFile, std::ios::in);
    if (!mapFile)
    {
        std::cerr << "The mapFile string points to a non-existent or protected file `" << mapFile << "'" << std::endl;
        return;
    }
    std::string line = "";
    getline(mapFile, line);
    if (line.size() < 18)
    {
        std::cerr << "Map string format invalid: Header line too small" << std::endl;
        return;
    }
    size.x = (unsigned int)std::stoul(line.substr(0, 8), nullptr, 16);
    size.y = (unsigned int)std::stoul(line.substr(8, 8), nullptr, 16);
    nestc = (unsigned char)std::stoul(line.substr(16, 2), nullptr, 16);
    nests.reserve(nestc);
    for (unsigned char i = 0; i < nestc; i++)
    {
        getline(mapFile, line);
        
    }
}
std::string Map::encode() // Returns a string that can be passed to decode() to copy this map.
{
}
void Map::decode(std::string) // Takes a string returned from encode() and copies that map to this instance.
{
}
void Map::prep(int) // Preps map before game. Assumes RoundSettings::instance is set.
{
}

Nest::Nest()
{
}
Nest::Nest(Map*, Pos) // Takes a parent ptr and a position
{
}
Nest::Nest(Map*, Pos, int) // Takes a parent ptr, a position and an ant count
{
}
void Nest::init(Map*, Pos, int) // Takes a parent ptr, a position and an ant count
{
}

Ant::Ant()
{
}
Ant::Ant(Nest* nparent, Pos np) // Takes a parent ptr and a position. Defaults to type = 0
{
    init(nparent, np, 0);
}
Ant::Ant(Nest* nparent, Pos np, unsigned char ntype) // Takes a parent ptr, a position and a type
{
    init(nparent, np, ntype);
}
void Ant::init(Nest* nparent, Pos npos, unsigned char ntype) // Takes a parent ptr, a position and a type
{
    parent = nparent;
    p = npos;
    type = ntype;
}
void Ant::giveCommand(AntCommand);
{
}
void Ant::step(double);
{
}
