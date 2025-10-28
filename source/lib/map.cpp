#include "map.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <thread> // For sleeping
#include <cstdlib>


Round* Round::instance = nullptr;


const char[16] ANTGAME_hexarray = "0123456789abcdef";
std::string ANTGAME_uchartohex(unsigned char num)
{
    std::string out;
    for (int i = 0; i < 2; i++)
    {
        out += ANTGAME_hexarray[num & 15];
        num>>=4;
    }
    return out;
}
std::string ANTGAME_ushorttohex(unsigned short num)
{
    std::string out;
    for (int i = 0; i < 4; i++)
    {
        out += ANTGAME_hexarray[num & 15];
        num>>=4;
    }
    return out;
}
std::string ANTGAME_uinttohex(unsigned int num)
{
    std::string out;
    for (int i = 0; i < 8; i++)
    {
        out += ANTGAME_hexarray[num & 15];
        num>>=4;
    }
    return out;
}



Round::Round()
{
    if (Round::instance == nullptr)
    {
        Round::instance = this;
    }
    secondsRunning = 0;
}


Round::~Round()
{
    if (Round::instance == this)
    {
        Round::instance = nullptr;
    }
    if (map)
    {
        delete map;
    }
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
    loadConfig(config);
    if (port >= 0)
    {
        RoundSettings::instance->port = port;
    }
    map = new Map;
    map->init();
    if (map->nests.size() <= 0)
    {
        std::cerr << "Failed to create map!" << std::endl;
        delete map;
        map = nullptr;
        return false;
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
    timeAtStart = std::chrono::steady_clock::now();
    secondsRunning = 0;
    phase = RUNNING;
    cm.start();
}


void Round::step()
{
    cm.step();
    switch (phase)
    {
        case DEAD:
        case INIT:
            return;
        case WAIT:
            if (!map) {phase = ERROR; break;}
            if (cm.players.size() == map->nests.size())
            {
                if (phaseEndTime == std::chrono::steady_clock::time_point::max())
                {
                    phaseEndTime = std::chrono::steady_clock::now() + std::chrono::steady_clock::duration<double, std::seconds>(RoundSettings::instance->gamestartdelay);
                }
                else if (phaseEndTime < std::chrono::steady_clock::now())
                {
                    phaseEndTime = std::chrono::steady_clock::time_point::max();
                    start();
                }
            }
            else
            {
                phaseEndTime = std::chrono::steady_clock::time_point::max();
            }
            break;
        case RUNNING:{ // Oddly threatening.
            if (!map) {phase = ERROR; break;}
            std::chrono::steady_clock::time_point t = std::chrono::steady_clock::now();
            double oldRunningTime = secondsRunning;
            secondsRunning = ((std::chrono::duration<double>)(t - timeAtStart)).count() * RoundSettings::instance->timeScale; 
            double delta = secondsRunning - oldRunningTime;
            for (;!cm.commands.empty();cm.commands.pop_front())
            {
                Command cmd = cm.commands.front();
                switch (cmd.cmd)
                {
                    case ConnectionManager::Command::ID::MOVE:{
                        if (cmd.nestID > map->nests.size() || !map->nests[cmd.nestID] || cmd.antID > map->nests[cmd.nestID]->ants.size() || !map->nests[cmd.nestID]->ants[cmd.antID])
                        {
                            break;
                        }
                        Ant::AntCommand acmd;
                        acmd.cmd = Ant::AntCommand::ID::MOVE;
                        acmd.arg = cmd.arg;
                        map->nests[cmd.nestID]->ants[cmd.antID]->giveCommand(acmd);
                        break;}
                    case ConnectionManager::Command::ID::TINTERACT:{
                        if (cmd.nestID > map->nests.size() || !map->nests[cmd.nestID] || cmd.antID > map->nests[cmd.nestID]->ants.size() || !map->nests[cmd.nestID]->ants[cmd.antID])
                        {
                            break;
                        }
                        Ant::AntCommand acmd;
                        acmd.cmd = Ant::AntCommand::ID::TINTERACT;
                        acmd.arg = cmd.arg;
                        map->nests[cmd.nestID]->ants[cmd.antID]->giveCommand(acmd);
                        break;}
                    case ConnectionManager::Command::ID::AINTERACT:{
                        if (cmd.nestID > map->nests.size() || !map->nests[cmd.nestID] || cmd.antID > map->nests[cmd.nestID]->ants.size() || !map->nests[cmd.nestID]->ants[cmd.antID])
                        {
                            break;
                        }
                        Ant::AntCommand acmd;
                        acmd.cmd = Ant::AntCommand::ID::AINTERACT;
                        acmd.arg = cmd.arg;
                        map->nests[cmd.nestID]->ants[cmd.antID]->giveCommand(acmd);
                        break;}
                    case ConnectionManager::Command::ID::NEWANT:{
                        if (cmd.nestID > map->nests.size() || !map->nests[cmd.nestID])
                        {
                            break;
                        }
                        Nest::NestCommand ncmd;
                        ncmd.cmd = Nest::NestCommand::ID::NEWANT;
                        ncmd.arg = cmd.arg;
                        map->nests[cmd.nestID]->giveCommand(ncmd);
                        break;}
                }
            }
            break;}
        case DONE:
            break;
        case CLOSED:
            break;
        case ERROR:
            std::cerr << "Encountered an unrecoverable error in server function! Shutting down imminently." << std::endl;
            cm.preclose();
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            reset();
            std::exit(1);
            break;
    }
}


void Round::end()
{
    // TODO
    cm.preclose();
    phase = DONE;
}
void Round::reset() // Goes back to state after constructor but before open
{
    if (map)
    {
        delete map;
    }
    if (RoundSettings::instance)
    {
        delete RoundSettings::instance;
    }
    if (Connection::listening())
    {
        Connection::closeListen();
    }
    cm.reset();
    phase = INIT;
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
    if (!mapFile.is_open())
    {
        mapFile.open("defaultMap", std::ios::in | std::ios::binary);
        if (!mapFile.is_open())
        {
            std::cerr << "The mapFile string points to a non-existent or protected file `" << RoundSettings::instance->mapFile << "'" << std::endl;
            return;
        }
    }
    _decode(mapFile);
}
std::string Map::encode() // Returns a string that can be passed to decode() to copy this map.
{
    std::string out;
    unsigned short tantc = 0;
    for (int i = 0; i < std::min(nests.size(), 256); i++) {tantc += std::min(nests[i].ants.size(), 256);}
    out.reserve(9UL + (unsigned long)std::min(nests.size(), 256) * 9UL + (unsigned long)tantc * 9UL + (unsigned long)size.x*(unsigned long)size.y);
    out += ANTGAME_uinttohex(size.x) + ANTGAME_uinttohex(size.y) + ANTGAME_uchartohex(nests.size()) + '\n';
    for (int i = 0; i < nests.size(); i++)
    {
        out += ANTGAME_uinttohex(nests[i]->p.x);
        out += ANTGAME_uinttohex(nests[i]->p.y);
        out += ANTGAME_uchartohex((unsigned char)nests[i]->ants.size());
        for (int j = 0; j < nests[i]->ants.size(); j++)
        {
            out += ANTGAME_uinttohex(nests[i]->ants[j]->p.x;
            out += ANTGAME_uinttohex(nests[i]->ants[j]->p.y;
            out += ANTGAME_uchartohex(nests[i]->ants[j]->type;
        }
    }
    return out;
}


void Map::decode(std::string string) // Takes a string returned from encode() and copies that map to this instance.
{
    cleanup();
    std::stringstream mapFile(string);
    if (!mapFile)
    {
        std::cerr << "Failed to create a stringstream object when decoding map string!" << std::endl;
        return;
    }
    _decode(mapFile);
}


void Map::_decode(std::istream mapFile)
{
    if (!mapFile)
    {
        return;
    }
    char headerLine[18];
    std::string line = "";
    mapFile.get(headerLine, 19);
    if (mapFile.gcount() < 19 || headerLine[18] != '\n')
    {
        std::cerr << "Map string format invalid: Header line too small." << std::endl;
        return;
    }
    line.append(headerLine, 18);
    Nest*n = nullptr;
    Ant*a = nullptr;
    try {
    size.x = (unsigned int)std::stoul(line.substr(0, 8), nullptr, 16);
    size.y = (unsigned int)std::stoul(line.substr(8, 8), nullptr, 16);
    unsigned char nestc = (unsigned char)std::stoul(line.substr(16, 2), nullptr, 16);
    nests.reserve(nestc);
    for (unsigned char i = 0; i < nestc; i++)
    {
        mapFile.get(headerLine, 19);
        if (mapFile.gcount() < 19 || headerLine[18] != '\n')
        {
            std::cerr << "Map string format invalid: Nest header line too small." << std::endl;
            cleanup();
            return;
        }
        line = "";
        line.append(headerLine, 18);
        n = new Nest;
        n->p.x = (unsigned int)std::stoul(line.substr(0, 8), nullptr, 16);
        n->p.y = (unsigned int)std::stoul(line.substr(8, 8), nullptr, 16);
        unsigned char antc = (unsigned char)std::stoul(line.substr(16, 2), nullptr, 16);
        n->parent = this;
        n->ants.reserve(antc);
        for (int j = 0; j < antc; j++)
        {
            mapFile.get(headerLine, 19);
            if (mapFile.gcount() < 19 || headerLine[18] != '\n')
            {
                std::cerr << "Map string format invalid: Ant data line is too small." << std::endl;
                cleanup();
                return;
            }
            line = "";
            line.append(headerLine, 18);
            a = new Ant;
            a->p.x = (unsigned int)std::stoul(line.substr(0, 8), nullptr, 16);
            a->p.y = (unsigned int)std::stoul(line.substr(8, 8), nullptr, 16);
            a->type = (unsigned char)std::stoul(line.substr(16, 2), nullptr, 16);
            a->parent = n;
            n->ants.push_back(a);
            a = nullptr;
        }
        nests.push_back(n);
        n = nullptr;
    }
    // Actual map data
    map = new unsigned char[(unsigned int)size.x*(unsigned int)size.y];
    mapFile.get(map, (unsigned int)size.x*(unsigned int)size.y);
    if (mapFile.gcount() < (unsigned int)size.x * (unsigned int)size.y)
    {
        std::cerr << "Map string format invalid: Raw map data is of the wrong size (should be " << size.x * size.y << ")" << std::endl;
        cleanup();
        return;
    }

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


void Nest::step(double delta)
{
    for (NestCommand& cmd : commands)
    {
        switch (cmd.cmd)
        {
            case DONE:
                break;
            case NEWANT:
                // TODO
                cmd.cmd = DONE;
                break;
        }
    }
    bool del = false;
    for (auto it = commands.begin(); it != commands.end(); it++)
    {
        if (del)
        {
            commands.erase(it-1);
        }
        del = false;
        if (*it.cmd == NestCommand::ID::DONE) {del = true;}
    }
    if (del && !commands.empty()) {commands.pop_back();}
}


void Nest::giveCommand(NestCommand cmd)
{
    // TODO? May need to be changed
    commands.push_back(cmd);
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
    // TODO? May need to be changed
    commands.push_back(com); 
}
void Ant::step(double)
{
    // TODO
}
