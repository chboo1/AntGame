#include "map.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>


Round* Round::instance = nullptr;



Round::Round()
{
    if (Round::instance == nullptr)
    {
        Round::instance = this;
    }
    secondsRunning = 0;
}
bool Round::open()
{
    return open("antgame.cfg", -1);
}
bool Round::open(int port)
{
    return open("antgame.cfg", port);
}
bool Round::open(std::string config)
{
    return open(config, -1);
}
bool Round::open(std::string config, int port)
{
    if (RoundSettings::instance == nullptr)
    {
        (new RoundSettings)->isPlayer = false; // We don't need this pointer since it's automatically becoming RoundSettings::instance
    }
    std::ifstream configFile;
    if (config != "")
    {
        configFile.open(config, std::ios::in);
        if (!configFile.is_open())
        {
            std::cerr << "Cannot open file `" << config << "'. It might not exist or it might not be readable." << std::endl;
            if (config != "antgame.cfg")
            {
                configFile.open("antgame.cfg", std::ios::in);
            }
        }
    }
    // Default settings. These are set here even if there is a config file, in case not all configs are set in there.
    RoundSettings::instance->mapFile = "defaultMap";
    RoundSettings::instance->port = ANTNET_DEFAULT_PORT;
    timeScale = 1.0;
    if (configFile.is_open())
    {
        // TODO: Read config file
    }
    if (port >= 0)
    {
        RoundSettings::instance->port = port;
    }
    if (!Connection::openListen(RoundSettings::instance->port))
    {
        std::cerr << "Failed to start listening for connections!" << std::endl;
        return false;
    }
    phase = WAIT;
    secondsRunning = 0;
    return true;
}
void Round::start()
{
    // TODO
}
void Round::step()
{
    cm.step();
    switch (phase)
    {
        case INIT:
            return;
        case WAIT:
            break;
        case RUNNING:{ // Oddly threatening.
            std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
            double oldRunningTime = secondsRunning;
            secondsRunning = ((std::chrono::duration<double>)(t - timeAtStart)).count() * timeScale; 
            double delta = secondsRunning - oldRunningTime;
            break;}
        case DONE:
            break;
        case CLOSED:
            break;
    
    }
}


void Round::end()
{
    // TODO
}
void Round::reset() // Goes back to state after constructor but before open
{
    // TODO
}


Map::Map()
{
}
void Map::init() // Uses RoundSettings::instance to get values
{
    cleanup();
    if (RoundSettings::instance == nullptr || RoundSettings::instance->isPlayer || RoundSettings::instance->mapFile == "")
    {
        return;
    }
    std::ifstream mapFile(RoundSettings::instance->mapFile, std::ios::in | std::ios::binary);
    if (!mapFile)
    {
        std::cerr << "The mapFile string points to a non-existent or protected file `" << RoundSettings::instance->mapFile << "'" << std::endl;
        return;
    }
    std::string line = "";
    getline(mapFile, line);
    if (line.size() < 18)
    {
        std::cerr << "Map string format invalid: Header line too small." << std::endl;
        return;
    }
    Nest*n = nullptr;
    Ant*a = nullptr;
    try {
    size.x = (unsigned int)std::stoul(line.substr(0, 8), nullptr, 16);
    size.y = (unsigned int)std::stoul(line.substr(8, 8), nullptr, 16);
    unsigned char nestc = (unsigned char)std::stoul(line.substr(16, 2), nullptr, 16);
    nests.reserve(nestc);
    for (unsigned char i = 0; i < nestc; i++)
    {
        getline(mapFile, line);
        if (line.length() < 18)
        {
            std::cerr << "Map string format invalid: Nest header line too small." << std::endl;
            cleanup();
            return;
        }
        n = new Nest;
        n->p.x = (unsigned int)std::stoul(line.substr(0, 8), nullptr, 16);
        n->p.y = (unsigned int)std::stoul(line.substr(8, 8), nullptr, 16);
        unsigned char antc = (unsigned char)std::stoul(line.substr(16, 2), nullptr, 16);
        n->parent = this;
        n->ants.reserve(antc);
        for (int j = 0; j < antc; j++)
        {
            getline(mapFile, line);
            if (line.length() < 18)
            {
                std::cerr << "Map string format invalid: Ant data line is too small." << std::endl;
                cleanup();
                return;
            }
            a = new Ant;
            a->p.x = (unsigned int)std::stoul(line.substr(0, 8), nullptr, 16);
            a->p.y = (unsigned int)std::stoul(line.substr(8, 8), nullptr, 16);
            a->type = (unsigned int)std::stoul(line.substr(16, 2), nullptr, 16);
            a->parent = n;
            n->ants.push_back(a);
            a = nullptr;
        }
        nests.push_back(n);
        n = nullptr;
    }
    // Actual map data
    getline(mapFile, line);
    if (line.length() != size.x * size.y)
    {
        std::cerr << "Map string format invalid: Raw map data is of the wrong size (should be " << size.x * size.y << ")" << std::endl;
        cleanup();
        return;
    }
    map = new unsigned char[size.x*size.y];
    line.copy((char*)map, std::string::npos);

    } catch (std::invalid_argument const& error) {
        std::cerr << "Map string format invalid: Hexadecimal numbers expected! Exception is " << error.what() << std::endl;
        if (n != nullptr)
        {
            delete n;
        }
        if (a != nullptr)
        {
             delete a;
        }
        cleanup();
        return;
    }
}
std::string Map::encode() // Returns a string that can be passed to decode() to copy this map.
{
    // TODO
    return "";
}
void Map::decode(std::string string) // Takes a string returned from encode() and copies that map to this instance.
{
    cleanup();
    std::stringstream mapFile(string);
    if (!mapFile)
    {
        std::cerr << "The mapFile string points to a non-existent or protected file `" << RoundSettings::instance->mapFile << "'" << std::endl;
        return;
    }
    std::string line = "";
    getline(mapFile, line);
    if (line.size() < 18)
    {
        std::cerr << "Map string format invalid: Header line too small." << std::endl;
        return;
    }
    Nest*n = nullptr;
    Ant*a = nullptr;
    try {
    size.x = (unsigned int)std::stoul(line.substr(0, 8), nullptr, 16);
    size.y = (unsigned int)std::stoul(line.substr(8, 8), nullptr, 16);
    unsigned char nestc = (unsigned char)std::stoul(line.substr(16, 2), nullptr, 16);
    nests.reserve(nestc);
    for (unsigned char i = 0; i < nestc; i++)
    {
        getline(mapFile, line);
        if (line.length() < 18)
        {
            std::cerr << "Map string format invalid: Nest header line too small." << std::endl;
            cleanup();
            return;
        }
        n = new Nest;
        n->p.x = (unsigned int)std::stoul(line.substr(0, 8), nullptr, 16);
        n->p.y = (unsigned int)std::stoul(line.substr(8, 8), nullptr, 16);
        unsigned char antc = (unsigned char)std::stoul(line.substr(16, 2), nullptr, 16);
        n->parent = this;
        n->ants.reserve(antc);
        for (int j = 0; j < antc; j++)
        {
            getline(mapFile, line);
            if (line.length() < 18)
            {
                std::cerr << "Map string format invalid: Ant data line is too small." << std::endl;
                cleanup();
                return;
            }
            a = new Ant;
            a->p.x = (unsigned int)std::stoul(line.substr(0, 8), nullptr, 16);
            a->p.y = (unsigned int)std::stoul(line.substr(8, 8), nullptr, 16);
            a->type = (unsigned int)std::stoul(line.substr(16, 2), nullptr, 16);
            a->parent = n;
            n->ants.push_back(a);
            a = nullptr;
        }
        nests.push_back(n);
        n = nullptr;
    }
    // Actual map data
    getline(mapFile, line);
    if (line.length() != size.x * size.y)
    {
        std::cerr << "Map string format invalid: Raw map data is of the wrong size (should be " << size.x * size.y << ")" << std::endl;
        cleanup();
        return;
    }
    map = new unsigned char[size.x*size.y];
    line.copy((char*)map, std::string::npos);

    } catch (std::invalid_argument const& error) {
        std::cerr << "Map string format invalid: Hexadecimal numbers expected! Exception is " << error.what() << std::endl;
        if (n != nullptr)
        {
            delete n;
        }
        if (a != nullptr)
        {
             delete a;
        }
        cleanup();
        return;
    }
}
void Map::freeMap()
{
    if (map == nullptr)
    {
        return;
    }
    delete[] map;
    map = nullptr;
}
void Map::cleanup()
{
    freeMap();
    for (int i = 0; i < nests.size(); i++)
    {
        if (nests[i] == nullptr)
        {
            continue;
        }
        delete nests[i];
    }
    nests.clear();
    size.x = 0;
    size.y = 0;
}
Map::~Map()
{
    cleanup();
}

Nest::Nest()
{
}
Nest::Nest(Map* nparent, Pos npos) // Takes a parent ptr and a position
{
    init(nparent, npos, 0);
}
Nest::Nest(Map* nparent, Pos npos, int antc) // Takes a parent ptr, a position and an ant count
{
    init(nparent, npos, antc);
}
void Nest::init(Map* nparent, Pos npos, int antc) // Takes a parent ptr, a position and an ant count
{
    parent = nparent;
    p = npos;
    if (antc != 0)
    {
        ants.reserve(antc);
        for (int i = 0; i < antc; i++)
        {
            Ant*a = new Ant;
            a->init(this, p, 0);
            ants.push_back(a);
        }
    }
}
void Nest::cleanup()
{
    for (int i = 0; i < ants.size(); i++)
    {
         if (ants[i] == nullptr)
         {
             continue;
         }
         delete ants[i];
    }
    ants.clear();
}
Nest::~Nest()
{
    cleanup();
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
void Ant::giveCommand(AntCommand com)
{
    // May need to be changed
    commands.push_back(com); 
}
void Ant::step(double)
{
    // TODO
}
