#include "map.hpp"
#include <chrono>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>
#include <cstdint>
#include <thread> // For sleeping
#include <cstdlib>
#include <cmath>
#include <algorithm>


Round* Round::instance = nullptr;
volatile std::sig_atomic_t Round::signalFlag = 0;


const char ANTGAME_hexarray[] = "0123456789abcdef";
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


Pos::Pos(DPos o)
{
    x = std::max(std::floor(o.x), 0.0);
    y = std::max(std::floor(o.y), 0.0);
}


Pos::Pos(unsigned short nx, unsigned short ny)
{
    x = nx;
    y = ny;
}


Pos& Pos::operator=(DPos o)
{
    x = std::max(std::floor(o.x), 0.0);
    y = std::max(std::floor(o.y), 0.0);
    return *this;
}


bool Pos::operator==(Pos o)
{
    return x == o.x && y == o.y;
}


Pos Pos::operator+(Pos o)
{
    Pos r;
    r.x = x + o.x;
    r.y = y + o.y;
    return r;
}


Pos Pos::operator-(Pos o)
{
    Pos r;
    r.x = x - o.x;
    r.y = y - o.y;
    return r;
}


bool Pos::operator!=(Pos o)
{
    return x != o.x || y != o.y;
}


DPos::DPos(Pos o)
{
    x = o.x;
    y = o.y;
}


DPos::DPos(double nx, double ny)
{
    x = nx;
    y = ny;
}


DPos& DPos::operator=(Pos o)
{
    x = o.x;
    y = o.y;
    return *this;
}


DPos DPos::operator+(DPos o)
{
    DPos r = o;
    r.x += x;
    r.y += y;
    return r;
}


DPos DPos::operator-(DPos o)
{
    DPos r{x, y};
    r.x -= o.x;
    r.y -= o.y;
    return r;
}


double DPos::magnitude()
{
    return sqrt(x*x + y*y);
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
    RoundSettings::instance->loadConfig(config);
    if (port >= 0)
    {
        RoundSettings::instance->port = port;
    }
    if (logging)
    {
        std::cout << "Opening server to connections using config file located at '" << config << "' and on port " << RoundSettings::instance->port << "." << std::endl;
    }
    map = new Map;
    if (!map)
    {
        std::cerr << "Could not allocate space for a map object!" << std::endl;
        return false;
    }
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
    if (phase != WAIT)
    {
        std::cerr << "Error: Trying to start the round before the waiting state!" << std::endl;
        return;
    }
    if (!map)
    {
        std::cerr << "Error: Trying to start the game without a map!" << std::endl;
        phase = ERROR;
        return;
    }
    timeAtStart = std::chrono::steady_clock::now();
    timeLastFrame = std::chrono::steady_clock::now();
    secondsRunning = 0;
    phase = RUNNING;
    cm.start();
    for (Nest* n : map->nests)
    {
        if (n)
        {
            n->foodCount = RoundSettings::instance->startingFood;
        }
    }
}


void Round::step()
{
    cm.step();
    switch (phase)
    {
        case INIT:
            return;
        case WAIT:
            if (!map) {phase = ERROR; break;}
            if (cm.players.size() == map->nests.size())
            {
                if (phaseEndTime == std::chrono::steady_clock::time_point::max())
                {
                    phaseEndTime = std::chrono::steady_clock::now();
                    phaseEndTime += std::chrono::duration_cast<std::chrono::steady_clock::duration, double>((std::chrono::duration<double>)RoundSettings::instance->gameStartDelay);
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
            RoundSettings::instance->timeScale = std::max(RoundSettings::instance->timeScale, 0.0);
            secondsRunning = ((std::chrono::duration<double>)(t - timeAtStart)).count() * RoundSettings::instance->timeScale;
            double delta = ((std::chrono::duration<double>)(t - timeLastFrame)).count() * RoundSettings::instance->timeScale;
            timeLastFrame = t;
            deltaTime = delta;
            if (delta <= 0)
            {
                break;
            }
            for (;!cm.commands.empty();cm.commands.pop_front())
            {
                ConnectionManager::Command cmd = cm.commands.front();
                switch (cmd.cmd)
                {
                    case ConnectionManager::Command::ID::MOVE:{
                        if (cmd.nestID > map->nests.size() || !map->nests[cmd.nestID] || cmd.antID > map->antPermanents.size() || !map->antPermanents[cmd.antID] || map->antPermanents[cmd.antID]->parent != map->nests[cmd.nestID])
                        {
                            break;
                        }
                        Ant::AntCommand acmd;
                        acmd.cmd = Ant::AntCommand::ID::MOVE;
                        acmd.state = Ant::AntCommand::State::ONGOING;
                        acmd.arg = cmd.arg;
                        map->antPermanents[cmd.antID]->giveCommand(acmd);
                        break;}
                    case ConnectionManager::Command::ID::TINTERACT:{
                        if (cmd.nestID > map->nests.size() || !map->nests[cmd.nestID] || cmd.antID > map->antPermanents.size() || !map->antPermanents[cmd.antID] || map->antPermanents[cmd.antID]->parent != map->nests[cmd.nestID])
                        {
                            break;
                        }
                        Ant::AntCommand acmd;
                        acmd.cmd = Ant::AntCommand::ID::TINTERACT;
                        acmd.state = Ant::AntCommand::State::ONGOING;
                        acmd.arg = cmd.arg;
                        map->antPermanents[cmd.antID]->giveCommand(acmd);
                        break;}
                    case ConnectionManager::Command::ID::AINTERACT:{
                        if (cmd.nestID > map->nests.size() || !map->nests[cmd.nestID] || cmd.antID > map->antPermanents.size() || !map->antPermanents[cmd.antID] || map->antPermanents[cmd.antID]->parent != map->nests[cmd.nestID])
                        {
                            break;
                        }
                        Ant::AntCommand acmd;
                        acmd.cmd = Ant::AntCommand::ID::AINTERACT;
                        acmd.state = Ant::AntCommand::State::ONGOING;
                        acmd.arg = cmd.arg;
                        map->antPermanents[cmd.antID]->giveCommand(acmd);
                        break;}
                    case ConnectionManager::Command::ID::NEWANT:{
                        if (cmd.nestID > map->nests.size() || !map->nests[cmd.nestID])
                        {
                            break;
                        }
                        Nest::NestCommand ncmd;
                        ncmd.cmd = Nest::NestCommand::ID::NEWANT;
                        ncmd.state = Nest::NestCommand::State::ONGOING;
                        ncmd.arg = cmd.arg;
                        map->nests[cmd.nestID]->giveCommand(ncmd);
                        break;}
                }
            }
            for (Nest*n : map->nests)
            {
                if (!n) {continue;}
                for (int i = 0; i < n->ants.size(); i++)
                {
                    if (n->ants[i] == nullptr || n->ants[i]->health <= 0)
                    {
                        n->killAnt(i);
                    }
                }
            }
            unsigned int c = 0;
            for (int i = 0; i < map->nests.size(); i++)
            {
                Nest*n = map->nests[i];
                if (n)
                {
                    n->step(delta);
                    if (n->foodCount <= 0)
                    {
                        NestStats stat = n->stats;
                        stat.rank = map->nests.size() - deadNestStats.size();
                        stat.name = n->name;
                        deadNestStats.push_back(stat);
                        n->salute();
                        delete n;
                        map->nests[i] = nullptr; // Notice we're not shrinking the array, to preserve IDs
                    }
                    else {c++;} // That seems familiar
                }
            }
            if (c <= 1)
            {
                end();
            }
            break;}
        case DONE:
            if (phaseEndTime < std::chrono::steady_clock::now())
            {
                reset();
                phase = CLOSED;
            }
            break;
        case CLOSED: break;
        case ERROR:
            std::cerr << "Encountered an unrecoverable error in server function! Shutting down imminently." << std::endl;
            cm.preclose();
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
            reset();
            std::exit(1);
            break;
    }
    if (signalFlag != 0)
    {
        switch (signalFlag)
        {
            case 1:
                std::cerr << "Keyboard interrupt!" << std::endl;
                break;
            default:
                std::cerr << "Unknown signal interrupt!" << std::endl;
                break;
        }
        phase = ERROR;
    }
}


void Round::end()
{
    // TODO (Round::end) probably OK
    if (logging)
    {
        std::cout << "\nThe round is over!";
        Nest*winner = nullptr;
        unsigned char winneri = 0;
        for (int i = 0; i < map->nests.size(); i++)
        {
            if (map->nests[i])
            {
                winner = map->nests[i];
                winneri = i;
            }
        }
        if (winner)
        {
            std::cout << " The winner is " << winner->name << " (Nest " << (unsigned int)winneri << ")" << std::endl;
            NestStats stat = winner->stats;
            stat.rank = 1;
            stat.name = winner->name;
            stat.timeLasted += winner->foodCount;
            deadNestStats.push_back(stat);
        }
        else
        {
            std::cout << " There is no winner (TIE)" << std::endl;
        }
        std::cout << "\n";
        if (statsKeeping)
        {
            for (NestStats stat : deadNestStats)
            {
                std::cout << stat.name << " is in rank " << (unsigned int)stat.rank << ", lasting for " << stat.timeLasted << " seconds.\nIt took " << stat.foodTaken << " food, made " << stat.antsMade << " ants and killed " << stat.kills << ".\nAt most, it had " << stat.peakAnts << " ants and " << stat.peakFood << " food.\n" << std::endl;
            }
        }
        std::cout << "Game ran for " << secondsRunning / RoundSettings::instance->timeScale << " seconds (" << secondsRunning << " in-game seconds.)" << std::endl;
    }
    cm.preclose();
    phase = DONE;
    timeAtStart = std::chrono::steady_clock::now();
    phaseEndTime = timeAtStart;
    phaseEndTime += std::chrono::duration_cast<std::chrono::steady_clock::duration, double>((std::chrono::duration<double>)10.0);
}
void Round::reset() // Goes back to state after constructor but before open
{
    if (map)
    {
        delete map;
        map = nullptr;
    }
    if (RoundSettings::instance)
    {
        delete RoundSettings::instance;
        RoundSettings::instance = nullptr;
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
    map = nullptr;
}


void Map::init() // Uses RoundSettings::instance to get values
{
    cleanup();
    if (RoundSettings::instance == nullptr || RoundSettings::instance->isPlayer || RoundSettings::instance->mapFile == "")
    {
        return;
    }
    if (Round::instance->logging)
    {
        std::cout << "Initializing map using file '" << RoundSettings::instance->mapFile << "'." << std::endl;
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
    _decode(&mapFile);
    mapFile.close();
}
std::string Map::encode() // Returns a string that can be passed to decode() to copy this map.
{
    std::string out;
    unsigned short tantc = 0;
    for (int i = 0; i < std::min((std::uint64_t)nests.size(), (std::uint64_t)256); i++) {if (nests[i]) {tantc += std::min((std::uint64_t)nests[i]->ants.size(), (std::uint64_t)256);}}
    out.reserve((std::uint64_t)10 + (std::uint64_t)std::min((std::uint64_t)nests.size(), (std::uint64_t)256) * (std::uint64_t)10 + (std::uint64_t)tantc * (std::uint64_t)10 + (std::uint64_t)size.x*(std::uint64_t)size.y);
    out += ANTGAME_ushorttohex(size.x) + ANTGAME_ushorttohex(size.y) + ANTGAME_uchartohex(nests.size()) + '\n';
    for (int i = 0; i < nests.size() && i < 256; i++)
    {
        if (nests[i])
        {
            out += ANTGAME_ushorttohex(nests[i]->p.x);
            out += ANTGAME_ushorttohex(nests[i]->p.y);
            out += ANTGAME_uchartohex((unsigned char)nests[i]->ants.size());
            for (int j = 0; j < nests[i]->ants.size() && j < 256; j++)
            {
                out += ANTGAME_ushorttohex((unsigned short)std::floor(nests[i]->ants[j]->p.x));
                out += ANTGAME_ushorttohex((unsigned short)std::floor(nests[i]->ants[j]->p.y));
                out += ANTGAME_uchartohex(nests[i]->ants[j]->type);
            }
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
    _decode(&mapFile);
}


void Map::_decode(std::istream* mapFile)
{
    if (!(*mapFile))
    {
        return;
    }
    char headerLine[11];
    std::string line = "";
    mapFile->read(headerLine, 11);
    if (mapFile->gcount() < 11 || headerLine[10] != '\n')
    {
        std::cerr << "Map string format invalid: Header line too small." << std::endl;
        return;
    }
    line.append(headerLine, 10);
    Nest*n = nullptr;
    Ant*a = nullptr;
    try {
    size.x = std::min((unsigned short)std::stoul(line.substr(0, 4), nullptr, 16), (unsigned short)65500);
    size.y = std::min((unsigned short)std::stoul(line.substr(4, 4), nullptr, 16), (unsigned short)65500);
    unsigned char nestc = (unsigned char)std::stoul(line.substr(8, 2), nullptr, 16);
    nests.reserve(nestc);
    for (unsigned char i = 0; i < nestc; i++)
    {
        mapFile->read(headerLine, 11);
        if (mapFile->gcount() < 11 || headerLine[10] != '\n')
        {
            std::cerr << "Map string format invalid: Nest header line too small." << std::endl;
            cleanup();
            return;
        }
        line = "";
        line.append(headerLine, 10);
        n = new Nest;
        n->p.x = (unsigned int)std::stoul(line.substr(0, 4), nullptr, 16);
        n->p.y = (unsigned int)std::stoul(line.substr(4, 4), nullptr, 16);
        unsigned char antc = (unsigned char)std::stoul(line.substr(8, 2), nullptr, 16);
        n->parent = this;
        n->ants.reserve(antc);
        n->foodCount = RoundSettings::instance->startingFood;
        for (int j = 0; j < antc; j++)
        {
            mapFile->read(headerLine, 11);
            if (mapFile->gcount() < 11 || headerLine[10] != '\n')
            {
                std::cerr << "Map string format invalid: Ant data line is too small." << std::endl;
                cleanup();
                return;
            }
            line = "";
            line.append(headerLine, 10);
            a = new Ant;
            DPos position;
            position.x = (unsigned int)std::stoul(line.substr(0, 4), nullptr, 16);
            position.y = (unsigned int)std::stoul(line.substr(4, 4), nullptr, 16);
            a->init(n, position, (unsigned char)std::stoul(line.substr(8, 2), nullptr, 16));
            n->ants.push_back(a);
            a = nullptr;
        }
        nests.push_back(n);
        n = nullptr;
    }
    // Actual map data
    map = new unsigned char[(unsigned int)size.x*(unsigned int)size.y];
    mapFile->read((char*)map, (unsigned int)size.x*(unsigned int)size.y);
    if (mapFile->gcount() < (unsigned int)size.x * (unsigned int)size.y)
    {
        std::cerr << "Map string format invalid: Raw map data is of the wrong size (should be " << size.x * size.y << ")" << std::endl;
        cleanup();
        return;
    }
    for (Nest* nn : nests)
    {
        if (nn->p.x >= size.x || nn->p.y >= size.y)
        {
            std::cerr << "A nest is out of bounds!" << std::endl;
            cleanup();
            return;
        }
        map[nn->p.x + nn->p.y * size.x] = (unsigned char)Tile::NEST;
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


Map::Tile Map::operator[](Pos p)
{
    if (p.x >= size.x || p.y >= size.y)
    {
        return Tile::UNKNOWN;
    }
    return (Tile)map[p.x+p.y*size.x];
}


bool Map::tileWalkable(Pos p)
{
    if (p.x >= size.x || p.y >= size.y) {return false;}
    return tileWalkable((*this)[p]);
}


bool Map::tileEdible(Pos p)
{
    if (p.x >= size.x || p.y >= size.y) {return false;}
    return tileEdible((*this)[p]);
}


bool Map::tileWalkable(Tile t)
{
    switch (t)
    {
        case Tile::WALL:
            return false;
        default:
            return true;
    }
}


bool Map::tileEdible(Tile t)
{
    switch (t)
    {
        case Tile::FOOD:
            return true;
        default:
            return false;
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
        nests[i]->salute();
        delete nests[i];
        nests[i] = nullptr;
    }
    nests.clear();
    size.x = 0;
    size.y = 0;
    antPermanents.clear();
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
            createAnt(0);
        }
    }
}


void Nest::step(double delta)
{
    foodCount -= delta;
    for (NestCommand& cmd : commands)
    {
        if (cmd.state != NestCommand::State::ONGOING) {continue;}
        switch (cmd.cmd)
        {
            case NestCommand::ID::NEWANT:
                if (foodCount < Ant::antTypes[cmd.arg].costMod * RoundSettings::instance->antCost || ants.size() >= 254)
                {
                    cmd.state = NestCommand::State::FAIL;
                    break;
                }
                stats.antsMade++;
                foodCount -= Ant::antTypes[cmd.arg].costMod * RoundSettings::instance->antCost;
                Ant* a = createAnt(cmd.arg);
                cmd.state = NestCommand::State::SUCCESS;
                ConnectionManager::AntEvent ae;
                ae.pid = a->pid;
                ae.health = a->health;
                ae.foodCarry = a->foodCarry;
                Round::instance->cm.antEventQueue.push_back(ae);
                break;
        }
    }
    if (ants.size() > stats.peakAnts)
    {
        stats.peakAnts = ants.size();
    }
    if (foodCount > stats.peakFood)
    {
        stats.peakFood = foodCount;
    }
    stats.rank = 1;
    for (Nest*n : parent->nests)
    {
        if (!n || n == this)
        {
            continue;
        }
        if (n->foodCount > foodCount)
        {
            stats.rank++;
        }
    }
    stats.timeLasted = Round::instance->secondsRunning;
    for (int i = 0; i < ants.size(); i++)
    {
        Ant* a = ants[i];
        if (a)
        {
            a->step(delta);
        }
    }
}


Ant* Nest::createAnt(unsigned char type)
{
    Ant*a = new Ant;
    a->init(this, (DPos)p, type);
    ants.push_back(a);
    return a;
}


void Nest::killAnt(int index)
{
    Ant*a = ants[index];
    if (a)
    {
        if (a->pid < parent->antPermanents.size())
        {
            parent->antPermanents[a->pid] = nullptr;
            delete a;
            ants[index] = nullptr;
        }
    }
    ants.erase(ants.begin() + index);
}


void Nest::giveCommand(NestCommand cmd)
{
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
void Nest::salute()
{
    for (;!ants.empty();)
    {
        killAnt(0);
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
Ant::Ant(Nest* nparent, DPos np) // Takes a parent ptr and a position. Defaults to type = 0
{
    init(nparent, np, 0);
}
Ant::Ant(Nest* nparent, DPos np, unsigned char ntype) // Takes a parent ptr, a position and a type
{
    init(nparent, np, ntype);
}
void Ant::init(Nest* nparent, DPos npos, unsigned char ntype) // Takes a parent ptr, a position and a type
{
    parent = nparent;
    p = npos;
    type = ntype;
    health = Ant::antTypes[type].healthMod * RoundSettings::instance->antHealth;
    foodCarry = 0;
    pid = parent->parent->antPermanents.size();
    parent->parent->antPermanents.push_back(this);
}
void Ant::_init(Nest* nparent, DPos npos, unsigned char ntype)
{
    parent = nparent;
    p = npos;
    type = ntype;
    health = Ant::antTypes[type].healthMod * RoundSettings::instance->antHealth;
    foodCarry = 0;
}
void Ant::giveCommand(AntCommand com)
{
    commands.push_back(com);
}
void Ant::step(double delta)
{
    bool moved = false;
    for (AntCommand& cmd : commands)
    {
        switch (cmd.cmd)
        {
            case AntCommand::ID::MOVE:{
                moved = true;
                DPos dest;
                dest.x = ConnectionManager::getAGNPshortdouble(cmd.arg>>32);
                dest.y = ConnectionManager::getAGNPshortdouble(cmd.arg & 0xffffffff);
                dest.x -= p.x;
                dest.y -= p.y;
                double destLen = dest.magnitude();
                if (destLen < 0.5)
                {
                    cmd.state = AntCommand::State::SUCCESS;
                    break;
                }
                else if (destLen > delta * Ant::antTypes[type].speedMod * RoundSettings::instance->movementSpeed)
                {
                    dest.x = dest.x / destLen * delta * Ant::antTypes[type].speedMod * RoundSettings::instance->movementSpeed;
                    dest.y = dest.y / destLen * delta * Ant::antTypes[type].speedMod * RoundSettings::instance->movementSpeed;
                }
                Pos npos = p + dest;
                Pos v = p;
                for (int i = 0; i < std::ceil(std::max(abs(dest.x), abs(dest.y))); i++)
                {
                    unsigned short x = p.x + (double)i * dest.x / std::max(abs(dest.x), abs(dest.y));
                    unsigned short y = p.y + (double)i * dest.y / std::max(abs(dest.x), abs(dest.y));
                    Pos ip{x, y};
                    if (x < 0)
                    {
                        npos.x = -1;
                        npos.y = y;
                        break;
                    }
                    else if (y < 0)
                    {
                        npos.x = x;
                        npos.y = -1;
                        break;
                    }
                    else if (x >= parent->parent->size.x)
                    {
                        npos.x = parent->parent->size.x;
                        npos.y = y;
                        break;
                    }
                    else if (y >= parent->parent->size.y)
                    {
                        npos.y = parent->parent->size.y;
                        npos.x = x;
                        break;
                    }
                    else if (!parent->parent->tileWalkable(ip))
                    {
                        npos = ip;
                        break;
                    }
                    v = ip;
                }
                if (parent->parent->tileWalkable(npos))
                {
                    p = p + dest;
                }
                else
                {
                    cmd.state = AntCommand::State::FAIL;
                    p = v;
                    p.x += 0.5;
                    p.y += 0.5;
                }
                if (abs(p.x - ConnectionManager::getAGNPshortdouble(cmd.arg>>32)) < 0.5 && abs(p.y - ConnectionManager::getAGNPshortdouble(cmd.arg&0xffffffff)) < 0.5)
                {
                    cmd.state = AntCommand::State::SUCCESS;
                }
                break;}
            case AntCommand::ID::TINTERACT:{
                Pos target = {(unsigned short)(cmd.arg>>16), (unsigned short)(cmd.arg&0xffff)};
                if (target.x >= parent->parent->size.x || target.y >= parent->parent->size.y)
                {
                    cmd.state = AntCommand::State::FAIL;
                }
                else if (((DPos)target - p).magnitude() > RoundSettings::instance->pickupRange * Ant::antTypes[type].rangeMod)
                {
                    if (!moved) // This basically means if you're moving somewhere it'll let you retry next frame
                    {
                        cmd.state = AntCommand::State::FAIL;
                    }
                }
                else
                {
                    switch ((*parent->parent)[target])
                    {
                        case Map::Tile::FOOD:
                            if (Ant::antTypes[type].capacity * RoundSettings::instance->capacityMod < foodCarry + RoundSettings::instance->foodYield)
                            {
                                cmd.state = AntCommand::State::FAIL;
                            }
                            else
                            {
                                foodCarry += RoundSettings::instance->foodYield;
                                parent->parent->map[target.x+target.y*parent->parent->size.x] = (unsigned char)Map::Tile::EMPTY;
                                cmd.state = AntCommand::State::SUCCESS;
                                ConnectionManager::MapEvent me;
                                me.x = target.x;
                                me.y = target.y;
                                me.tile = (unsigned char)Map::Tile::EMPTY;
                                ConnectionManager::AntEvent ae;
                                ae.pid = pid;
                                ae.foodCarry = foodCarry;
                                ae.health = health;
                                Round::instance->cm.mapEventQueue.push_back(me);
                                Round::instance->cm.antEventQueue.push_back(ae);
                            }
                            break;
                        case Map::Tile::NEST:
                            cmd.state = AntCommand::State::FAIL;
                            for (Nest* n : parent->parent->nests)
                            {
                                if (n == nullptr || n->p != target)
                                {
                                    continue;
                                }
                                if (n == parent)
                                {
                                    if (foodCarry > 0)
                                    {
                                        parent->stats.foodTaken += foodCarry;
                                        parent->foodCount += foodCarry;
                                        foodCarry = 0;
                                        ConnectionManager::AntEvent ae;
                                        ae.pid = pid;
                                        ae.foodCarry = foodCarry;
                                        ae.health = health;
                                        Round::instance->cm.antEventQueue.push_back(ae);
                                    }
                                }
                                else
                                {
                                    n->foodCount -= std::min(RoundSettings::instance->foodTheftYield, Ant::antTypes[type].capacity * RoundSettings::instance->capacityMod - foodCarry);
                                    foodCarry += std::min(RoundSettings::instance->foodTheftYield, Ant::antTypes[type].capacity * RoundSettings::instance->capacityMod - foodCarry);
                                    ConnectionManager::AntEvent ae;
                                    ae.pid = pid;
                                    ae.foodCarry = foodCarry;
                                    ae.health = health;
                                    Round::instance->cm.antEventQueue.push_back(ae);
                                }
                                cmd.state = AntCommand::State::SUCCESS;
                            }
                            break;
                        default:
                            cmd.state = AntCommand::State::FAIL;
                            break;
                    }
                }
                break;}
            case AntCommand::ID::AINTERACT:{
                if (cmd.arg >= parent->parent->antPermanents.size() || !parent->parent->antPermanents[cmd.arg] || parent->parent->antPermanents[cmd.arg]->health <= 0)
                {
                    cmd.state = AntCommand::State::FAIL;
                    break;
                }
                if (std::chrono::duration<double>(std::chrono::high_resolution_clock::now()-lastAINTER).count() * RoundSettings::instance->timeScale < RoundSettings::instance->attackRate * Ant::antTypes[type].rateMod)
                {
                    break;
                }
                if ((parent->parent->antPermanents[cmd.arg]->p - p).magnitude() > RoundSettings::instance->attackRange * Ant::antTypes[type].rangeMod)
                {
                    if (!moved)
                    {
                        cmd.state = AntCommand::State::FAIL;
                    }
                    break;
                }
                cmd.state = AntCommand::State::SUCCESS;
                Ant* ea = parent->parent->antPermanents[cmd.arg];
                ea->health -= Ant::antTypes[type].damageMod * RoundSettings::instance->attackDamage;
                ConnectionManager::AntEvent ae;
                ae.pid = ea->pid;
                ae.health = ea->health;
                ae.foodCarry = ea->foodCarry;
                Round::instance->cm.antEventQueue.push_back(ae);
                if (ea->health <= 0)
                {
                    parent->stats.kills++;
                }
                break;}
            default:{
                cmd.state = AntCommand::State::FAIL;
                break;}
        }
    }
    p.x = std::min(std::max(p.x, 0.0), (double)parent->parent->size.x);
    p.y = std::min(std::max(p.y, 0.0), (double)parent->parent->size.y);
}
