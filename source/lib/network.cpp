#include "network.hpp"
#include "map.hpp"
#include "antTypes.hpp"
#include <fstream>
#include <climits>
#include <cstdint>
#include <cmath>
#include <deque>
#include <sstream>


RoundSettings* RoundSettings::instance = nullptr;


RoundSettings::RoundSettings()
{
     if (instance == nullptr)
     {
         instance = this;
         isPlayer = true; // Reset this manually if you are the server. Don't do it otherwise.
         port = ANTNET_DEFAULT_PORT;
         mapFile = "defaultMap";
         configFile = "";
         gameStartDelay = 3.0;
     }
}


void RoundSettings::loadConfig(std::string config)
{
    std::ifstream configF;
    if (config != "")
    {
        configF.open(config, std::ios::in);
        if (!configF.is_open())
        {
            std::cerr << "Cannot open file `" << config << "'. It might not exist or it might not be readable." << std::endl;
            if (config != "antgame.cfg")
            {
                configF.open("antgame.cfg", std::ios::in);
            }
            configFile = "antgame.cfg";
        }
        else {configFile = config;}
    }
    // Default settings. These are set here even if there is a config file, in case not all configs are set in there.
    mapFile = "defaultMap";
    port = ANTNET_DEFAULT_PORT;
    gameStartDelay = 3.0;
    timeScale = 1.0;
    startingFood = 60;
    movementSpeed = 5.0;
    hungerRate = 1.0;
    foodYield = 1.0;
    foodTheftYield = 3.0;
    antCost = 5.0;
    attackRange = 10.0;
    attackDamage = 1.0;
    antHealth = 5.0;
    pickupRange = 10.0;
    capacityMod = 1.0;
    if (configF.is_open())
    {
        std::string line;
        while (getline(configF, line))
        {
            std::string identifier = line.substr(0, line.find(':'));
            std::string data = line.find(':') >= line.length() - 1 ? "": line.substr(line.find(':')+1);
            configLine(identifier, data);
        }
    }
    configF.close();
}


void RoundSettings::_loadConfig(std::string config)
{
    std::stringstream configF(config);
    mapFile = "defaultMap";
    port = ANTNET_DEFAULT_PORT;
    gameStartDelay = 3.0;
    timeScale = 1.0;
    startingFood = 60;
    movementSpeed = 5.0;
    hungerRate = 1.0;
    foodYield = 1.0;
    foodTheftYield = 3.0;
    antCost = 5.0;
    attackRange = 10.0;
    attackDamage = 1.0;
    antHealth = 5.0;
    pickupRange = 10.0;
    capacityMod = 1.0;
    std::string line;
    while (getline(configF, line))
    {
        std::string identifier = line.substr(0, line.find(':'));
        std::string data = line.find(':') >= line.length() - 1 ? "": line.substr(line.find(':')+1);
        configLine(identifier, data);
    }
}


void RoundSettings::configLine(std::string identifier, std::string data)
{
    if (identifier == "map")
    {
	mapFile = data;
    }
    else if (identifier == "timescale")
    {
	try{
	timeScale = std::stod(data);
	}catch(...){
	timeScale = 1.0;
	std::cerr << "The `timescale' argument of the configuration file is not a valid floating point number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "port")
    {
	try{
	port = (unsigned int)std::stoi(data);
	}catch(...){
	port = ANTNET_DEFAULT_PORT;
	std::cerr << "The `port' argument of the configuration file is not a valid integer! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "gamestartdelay")
    {
	try{
	gameStartDelay = std::stod(data);
	}catch(...){
        gameStartDelay = 3.0;
	std::cerr << "The `gamestartdelay' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "startingfood")
    {
	try{
	startingFood = std::stod(data);
	}catch(...){
        startingFood = 60;
	std::cerr << "The `startingfood' argument of the configuration file is not a valid integer! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "movementspeed")
    {
	try{
	movementSpeed = std::stod(data);
	}catch(...){
        movementSpeed = 5.0;
	std::cerr << "The `movementspeed' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "hungerrate")
    {
	try{
        hungerRate = std::stod(data);
	}catch(...){
        hungerRate = 1.0;
	std::cerr << "The `hungerrate' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "foodyield")
    {
	try{
	foodYield = std::stod(data);
	}catch(...){
        foodYield = 1.0;
	std::cerr << "The `gamestartdelay' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "foodtheftyield")
    {
	try{
	foodTheftYield = std::stod(data);
	}catch(...){
        foodTheftYield = 3.0;
	std::cerr << "The `foodtheftyield' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "antcost")
    {
	try{
	antCost = std::stod(data);
	}catch(...){
        antCost = 5.0;
	std::cerr << "The `antcost' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "attackrange")
    {
	try{
	attackRange = std::stod(data);
	}catch(...){
        attackRange = 10.0;
	std::cerr << "The `attackrange' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "attackdmg")
    {
	try{
	attackDamage = std::stod(data);
	}catch(...){
        attackDamage = 1.0;
	std::cerr << "The `attackdmg' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "anthealth")
    {
	try{
	antHealth = std::stod(data);
	}catch(...){
        antHealth = 5.0;
	std::cerr << "The `anthealth' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "pickuprange")
    {
	try{
	pickupRange = std::stod(data);
	}catch(...){
        pickupRange = 10.0;
	std::cerr << "The `pickuprange' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
    else if (identifier == "capacitymod")
    {
	try{
	capacityMod = std::stod(data);
	}catch(...){
        capacityMod = 1.0;
	std::cerr << "The `capacitymod' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
	}
    }
}


Player::Player()
{
    timeAtLastMessage = std::chrono::steady_clock::now();
    name = "Unnamed nest";
}


Player::Player(Connection*nconn)
{
    conn = nconn;
    timeAtLastMessage = std::chrono::steady_clock::now();
    name = "Unnamed nest";
}


Player::Player(Viewer*v)
{
    conn = v->conn;
    timeAtLastMessage = std::chrono::steady_clock::now();
    unusedData = v->unusedData;
    name = "Unnamed nest";
}


Player::~Player()
{
    if (conn != nullptr)
    {
        delete conn;
    }
}


Viewer::Viewer()
{
    timeAtLastMessage = std::chrono::steady_clock::now();
}


Viewer::Viewer(Connection*nconn)
{
    conn = nconn;
    timeAtLastMessage = std::chrono::steady_clock::now();
}


Viewer::~Viewer()
{
    if (conn)
    {
        delete conn;
    }
}


ConnectionManager::ConnectionManager()
{
    AntEvent ae;
    ae.pid = 0xffffffff;
    ae.health = 0;
    ae.foodCarry = 0;
    antEventQueue.push_back(ae);
    MapEvent me;
    me.x = 0xffff;
    me.y = 0xffff;
    me.tile = 0xff;
    mapEventQueue.push_back(me);
}


void ConnectionManager::start()
{
    for (Player* p : players)
    {
        if (isValid(p))
        {
            p->conn->send("\0\0\0\x0a\0\0\0\x01\0\x04", 10); // START
        }
        if (isValid(p)) // Running check again to detect if it changed when sending data
        {
            if (Round::instance->map->nests.size() > p->nestID && Round::instance->map->nests[p->nestID] != nullptr)
            {
                Round::instance->map->nests[p->nestID]->name = p->name;
            }
        }
    }
}


void ConnectionManager::step()
{
    if (!Round::instance)
    {
        return;
    }
    if (Round::instance->phase != Round::Phase::WAIT && Round::instance->phase != Round::Phase::RUNNING && Round::instance->phase != Round::Phase::DONE)
    {
        return;
    }
    if (Connection::listening())
    {
	std::vector<Connection*> newConnections;
	if (Connection::fetchConnections(&newConnections) >= 0)
	{
	    for (int i = 0; i < newConnections.size(); i++)
	    {
                if (newConnections[i] == nullptr) {continue;}
                if (!newConnections[i]->connected())
                {
                    newConnections[i]->finish();
                    delete newConnections[i];
                }
                else
                {
                    viewers.push_front(new Viewer(newConnections[i]));
                }
            }
	}
    }
    else
    {
	if (players.size() < Round::instance->map->nests.size())
	{
	    if (!Connection::openListen(Connection::port) && Connection::serrorState != Connection::RETRY)
	    {
		std::cerr << "Cannot listen for connections!" << std::endl;
		Round::instance->phase = Round::Phase::ERROR;
	    }
	}
    }
    handleViewers();
    if (Round::instance->phase == Round::Phase::DONE) {return;}
    handlePlayers();
}


void ConnectionManager::reset()
{
    for (Viewer* v : viewers)
    {
        if (v)
        {
	    delete v;
        }
    }
    viewers.clear();
    for (Player* p : players)
    {
        if (p)
        {
            if (Round::instance->logging)
            {
                std::cout << "Kicking out player at nest ID " << (int)p->nestID << std::endl;
            }
            delete p;
        }
    }
    players.clear();
}


void ConnectionManager::preclose()
{
    for (Player* p : players)
    {
        if (p && p->conn && p->conn->connected())
        {
            p->conn->send("\0\0\0\x0a\0\0\0\x01\0\x03", 10); // BYE
            p->bye = true;
        }
    }
    players.clear();
}


void ConnectionManager::handlePlayers()
{
    for (Player* p : players)
    {
	p->toClose = !interpretRequests(p) || p->toClose;
        if (!p->toClose)
        {
            std::string feedbackString = "";
            unsigned int feedbackc = 0;
            for (Nest::NestCommand ncmd : Round::instance->map->nests[p->nestID]->commands)
            {
                if (ncmd.state == Nest::NestCommand::State::ONGOING)
                {
                    continue;
                }
                if (ncmd.state == Nest::NestCommand::State::SUCCESS)
                {
                    feedbackString.append("\x00\x07", 2);
                }
                if (ncmd.state == Nest::NestCommand::State::FAIL)
                {
                    feedbackString.append("\x00\x08", 2);
                }
                switch (ncmd.cmd)
                {
                    case Nest::NestCommand::ID::NEWANT:
                        feedbackString.push_back((char)RequestID::NEWANT);
                        feedbackString.push_back((char)ncmd.arg);
                        feedbackc++;
                        break;
                    default:
                        feedbackString.pop_back();
                        feedbackString.pop_back();
                        break;
                }
            }
            bool cleared = false;
            while (!cleared)
            {
                cleared = true;
                for (std::deque<Nest::NestCommand>::iterator it = Round::instance->map->nests[p->nestID]->commands.begin(); it != Round::instance->map->nests[p->nestID]->commands.end();it++)
                {
                    if ((*it).state != Nest::NestCommand::State::ONGOING)
                    {
                        Round::instance->map->nests[p->nestID]->commands.erase(it);
                        cleared = false;
                        break;
                    }
                }
            }
            for (Ant* a : Round::instance->map->nests[p->nestID]->ants)
            {
                for (Ant::AntCommand acmd : a->commands)
                {
                    if (acmd.state == Ant::AntCommand::State::ONGOING)
                    {
                        continue;
                    }
                    if (acmd.state == Ant::AntCommand::State::SUCCESS)
                    {
                        feedbackString.append("\x00\x07", 2);
                    }
                    if (acmd.state == Ant::AntCommand::State::FAIL)
                    {
                        feedbackString.append("\x00\x08", 2);
                    }
                    switch (acmd.cmd)
                    {
                        case Ant::AntCommand::ID::MOVE:
                            feedbackString.push_back((char)RequestID::WALK);
                            feedbackString.append(makeAGNPuint(a->pid));
                            feedbackString.append(makeAGNPuint(acmd.arg>>32));
                            feedbackString.append(makeAGNPuint(acmd.arg&0xffffffff));
                            feedbackc++;
                            break;
                        case Ant::AntCommand::ID::TINTERACT:
                            feedbackString.push_back((char)RequestID::TINTERACT);
                            feedbackString.append(makeAGNPuint(a->pid));
                            feedbackString.append(makeAGNPuint(acmd.arg));
                            feedbackc++;
                            break;
                        case Ant::AntCommand::ID::AINTERACT:
                            feedbackString.push_back((char)RequestID::AINTERACT);
                            feedbackString.append(makeAGNPuint(a->pid));
                            feedbackString.append(makeAGNPuint(acmd.arg));
                            feedbackc++;
                            break;
                        default:
                            feedbackString.pop_back();
                            feedbackString.pop_back();
                            break;
                    }
                }
                cleared = false;
                while (!cleared)
                {
                    cleared = true;
                    for (std::deque<Ant::AntCommand>::iterator it = a->commands.begin(); it != a->commands.end();it++)
                    {
                        if ((*it).state != Ant::AntCommand::State::ONGOING)
                        {
                            a->commands.erase(it);
                            cleared = false;
                            break;
                        }
                    }
                }
            }
            if (!feedbackString.empty())
            {
                feedbackString.insert(0, makeAGNPuint(feedbackc));
                feedbackString.insert(0, makeAGNPuint(feedbackString.length()+4));
                p->conn->send(feedbackString.c_str(), feedbackString.length());
            }
        }
    }
    std::chrono::steady_clock::time_point timpont = std::chrono::steady_clock::now();
    for (int i = 0; i < players.size(); i++)
    {
        Player* p = players[i];
        double timeSinceMessage = std::chrono::duration<double>(timpont-p->timeAtLastMessage).count();
        if (!isValid(p) || timeSinceMessage >= (p->bye?1.0:3.0))
        {
            if (Round::instance->logging)
            {
                std::cout << "Kicking out player at nest ID " << (int)p->nestID << std::endl;
            }
            if (p && p->conn && p->conn->connected())
            {
                p->conn->send("\0\0\0\x0a\0\0\0\x01\0\x03", 10); // BYE
            }
            delete p;
            players.erase(players.begin() + i);i--;
            if (Round::instance->phase == Round::Phase::WAIT)
            {
                for (int i = 0; i < players.size(); i++)
                {
                    players[i]->nestID = i;
                }
            }
        }
        else if (timeSinceMessage >= 1.0 && !p->pinged)
        {
            p->pinged = true;
            p->conn->send("\0\0\0\x0a\0\0\0\x01\0\x02", 10); // PING REQ
        }
    }
    for (;!antEventQueue.empty(); antEventQueue.pop_front())
    {
        bool ok = true;
        for (Player* p : players)
        {
            if (p->antEventPos == antEventQueue.begin())
            {
                ok = false;
                break;
            }
        }
        if (!ok)
        {
            break;
        }
    }
    if (antEventQueue.empty())
    {
        AntEvent ae;
        ae.pid = -1;
        ae.foodCarry = 0;
        ae.health = 0;
        antEventQueue.push_back(ae);
        for (Player*p : players)
        {
            p->antEventPos = antEventQueue.begin();
        }
    }
}


void ConnectionManager::handleViewers()
{
    for (Viewer* v : viewers)
    {
        if (v->confirmed)
        {
            httpResponse(v);
        }
        else if (httpResponse(v))
        {
            v->confirmed = true;
            v->timeAtLastMessage = std::chrono::steady_clock::now();
        }
        else if (playerGreeting(v))
        {
            if (Round::instance->phase == Round::Phase::WAIT)
            {
                Player* p = new Player(v);
                p->nestID = players.size();
                p->mapEventPos = mapEventQueue.begin();
                p->antEventPos = antEventQueue.begin();
                players.push_back(p);
                v->conn = nullptr; // So the destructor doesn't kill it
                if (Round::instance->logging)
                {
                    std::cout << "Accepting new player to nest ID " << (int)p->nestID << std::endl;
                }
            }
            v->toClose = true;
        }
    }
    auto prev = viewers.before_begin();
    bool del = false;
    std::chrono::steady_clock::time_point timpont = std::chrono::steady_clock::now();
    for (auto it = viewers.begin();;it++)
    {
        if (del)
        {
            auto it2 = prev;
            it2++;
            delete *(it2);
            viewers.erase_after(prev);
        }
        else if (it != viewers.begin())
        {
            prev++;
        }
        if (it == viewers.end()) {break;}
        del = false;
        Viewer* v = *it;
        if (!isValid(v) || std::chrono::duration<double>(timpont - v->timeAtLastMessage).count() >= 3.0)
        {
            del = true;
        }
    }
}


bool ConnectionManager::isValid(Viewer* v)
{
    return v != nullptr && !v->toClose && v->conn != nullptr && v->conn->errorState != Connection::CLOSED && v->conn->connected();
}


bool ConnectionManager::isValid(Player* p)
{
    return p != nullptr && !p->toClose && p->conn != nullptr && p->conn->errorState != Connection::CLOSED && p->conn->connected() && Round::instance != nullptr && p->nestID < Round::instance->map->nests.size() && Round::instance->map->nests[p->nestID] != nullptr;
}


bool ConnectionManager::httpResponse(Viewer* v)
{
    if (!isValid(v))
    {
        return false;
    }
    std::string data = v->unusedData;
    data += v->conn->readall();
    if (!isValid(v))
    {
        return false;
    }
    if (v->confirmed) // Deleting any leftover data before the next request (i.e. the message body)
    {
        size_t p = data.find("HTTP/1.1\r\n");
        if (p != std::string::npos)
        {
            p = data.rfind("\n", p);
            if (p != std::string::npos)
            {
                data.erase(0, p + 1);
            }
        }
        else
        {
            data.clear();
        }
    }
    if (data.find("\r\n") == std::string::npos)
    {
        v->unusedData = data;
        return false;
    }
    if (data.compare(data.find("\r\n") - 8, 8, "HTTP/1.1") == 0)
    {
        signed char flag = 0;
        if (data.compare(0, 4, "GET ") == 0)
        {
            flag = 2;
        }
        else if (data.compare(0, 5, "HEAD ") == 0)
        {
            flag = 1;
        }
	    if (data.compare(0, 5, "POST ") == 0 || data.compare(0, 4, "PUT ") == 0 || data.compare(0, 7, "DELETE ") == 0 || data.compare(0, 8, "CONNECT ") == 0 || data.compare(0, 8, "OPTIONS") == 0 || data.compare(0, 6, "TRACE ") == 0 || data.compare(0, 6, "PATCH ") == 0)
        {
            sendResponse(v, "HTTP/1.1 405 Method Not Allowed\r\nConnection: keep-alive\r\n", "HTTPFiles/err405.html");
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            v->unusedData = data;
            return true;
        }
        if (flag == 0)
        {
            sendResponse(v, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n", "HTTPFiles/err400.html");
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            v->unusedData = data;
            v->toClose = true;
            return false;
        }
        if (data.find(' ') == std::string::npos || data.length() - data.find(' ') < 8)
        {
            sendResponse(v, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n", "HTTPFiles/err400.html");
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            v->unusedData = data;
            v->toClose = true;
            return false;
        }
        if (data.find(' ', data.find(' ') + 1) == std::string::npos)
        {
            sendResponse(v, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n", "HTTPFiles/err400.html");
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            v->unusedData = data;
            v->toClose = true;
            return false;
        }
        if (data.compare(data.find(' ') + 1, 12, "/favicon.ico") == 0)
        {
            sendResponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", flag == 2 ? "HTTPFiles/favicon.ico" : "");
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            v->unusedData = data;
        }
        else if (data.compare(data.find(' ') + 1, 20, "/waitingInternalData") == 0)
        {
            if (Round::instance->phase != Round::Phase::WAIT)
            {
                _sendResponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", "SIR_IT_IS_TIME_TO_REFRESH");
            }
            else
            {
                std::string playersData = "";
                for (Player* p : players)
                {
                    playersData.append("<li>");
                    playersData.append(p->name);
                    playersData.append(" (Nest ");
                    playersData.append(std::to_string(p->nestID));
                    playersData.append(")</li>\n");
                }
                if (playersData.empty()) {playersData = "None!";}
                _sendResponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", playersData);
            }
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            v->unusedData = data;
        }
        else if (data.compare(data.find(' ') + 1, 17, "/changelogMapData") == 0)
        {
            std::string changelogData = "";
            unsigned int clientAt = 0;
            changelogData = data.substr(data.find(' ') + 1, data.find(' ', data.find(' ') + 1) - data.find(' '));
            changelogData = changelogData.substr(17, std::string::npos);
            try
            {
                clientAt = std::stoul(changelogData, nullptr, 10);
            }
            catch (...)
            {
                sendResponse(v, "HTTP/1.1 422 Unprocessable Content\r\nConnection: keep-alive\r\n", "err422.html");
                data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
                v->unusedData = data;
                v->toClose = true;
                return false;
            }
            if (clientAt > mapEventQueue.size())
            {
                sendResponse(v, "HTTP/1.1 422 Unprocessable Content\r\nConnection: keep-alive\r\n", "err422.html");
                data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
                v->unusedData = data;
                v->toClose = true;
                return false;
            }
            changelogData.clear();
            unsigned short antc = 0;
            for (Ant* a : Round::instance->map->antPermanents)
            {
                if (a) {antc++;}
            }
            changelogData.reserve(10 + antc*4 + (mapEventQueue.size() - clientAt)*5);
            changelogData.append(makeAGNPuint(mapEventQueue.size() - clientAt));
            unsigned int index = 0;
            for (MapEvent me : mapEventQueue)
            {
                if (index >= clientAt)
                {
                    changelogData.append(makeAGNPushort(me.x));
                    changelogData.append(makeAGNPushort(me.y));
                    changelogData.push_back((char)me.tile);
                }
                index++;
            }
            changelogData.append(makeAGNPushort(antc));
            for (Ant* a : Round::instance->map->antPermanents)
            {
                if (a)
                {
                    changelogData.append(makeAGNPushort((unsigned short)std::floor(a->p.x)));
                    changelogData.append(makeAGNPushort((unsigned short)std::floor(a->p.y)));
                }
            }
            changelogData.append(makeAGNPuint(mapEventQueue.size()));
            _sendResponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", changelogData);
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            v->unusedData = data;
        }
        else if (data.compare(data.find(' ') + 1, 16, "/internalMapData") == 0)
        {
            std::string mapData = "";
            const char hexCharacterString[] = "0123456789abcdef";
            if (Round::instance->phase == Round::Phase::RUNNING)
            {
                mapData.reserve((unsigned int)Round::instance->map->size.x*(unsigned int)Round::instance->map->size.y*2 + 8);
                mapData.push_back(hexCharacterString[(unsigned char)((mapEventQueue.size()>>28) & 0xf)]);
                mapData.push_back(hexCharacterString[(unsigned char)((mapEventQueue.size()>>24) & 0xf)]);
                mapData.push_back(hexCharacterString[(unsigned char)((mapEventQueue.size()>>20) & 0xf)]);
                mapData.push_back(hexCharacterString[(unsigned char)((mapEventQueue.size()>>16) & 0xf)]);
                mapData.push_back(hexCharacterString[(unsigned char)((mapEventQueue.size()>>12) & 0xf)]);
                mapData.push_back(hexCharacterString[(unsigned char)((mapEventQueue.size()>>8) & 0xf)]);
                mapData.push_back(hexCharacterString[(unsigned char)((mapEventQueue.size()>>4) & 0xf)]);
                mapData.push_back(hexCharacterString[(unsigned char)(mapEventQueue.size() & 0xf)]);
            }
            else
            {
                mapData.reserve((unsigned int)Round::instance->map->size.x*(unsigned int)Round::instance->map->size.y*2);
            }
            for (unsigned int i = 0; i < (unsigned int)Round::instance->map->size.x*(unsigned int)Round::instance->map->size.y; i++)
            {
                mapData.push_back(hexCharacterString[(unsigned char)Round::instance->map->map[i]>>4]);
                mapData.push_back(hexCharacterString[(unsigned char)Round::instance->map->map[i]&0xf]);
            }
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            v->unusedData = data;
            _sendResponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", mapData);
        }
        else if (data.compare(data.find(' ') + 1, 11, "/playerData") == 0)
        {
            if (Round::instance->phase != Round::Phase::RUNNING && Round::instance->phase != Round::Phase::DONE)
            {
                sendResponse(v, "HTTP/1.1 404 Not Found\r\nConnection: closed\r\n", "err404.html");
                v->toClose = true;
            }
            else
            {
                std::string playersData = "";
                for (int i = 0; i < Round::instance->map->nests.size(); i++)
                {
                    Nest* n = Round::instance->map->nests[i];
                    if (n)
                    {
                        playersData.append("<li>");
                        playersData.append(n->name);
                        playersData.append(" (Nest ");
                        playersData.append(std::to_string(i));
                        playersData.append(", ");
                        playersData.append(n->foodCount > 0 ? std::to_string((unsigned int)std::ceil(n->foodCount)) : "DEAD");
                        playersData.append(")</li>");
                    }
                    else
                    {
                        playersData.append("<li>DEAD NEST (Nest ");
                        playersData.append(std::to_string(i));
                        playersData.append(")</li>");
                    }
                }
                if (playersData.empty()) {playersData = "None!";}
                _sendResponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", playersData);
                data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
                v->unusedData = data;
            }
        }
        else if (data.compare(data.find(' ') + 1, 2, "/ ") != 0)
        {
            sendResponse(v, "HTTP/1.1 302 Found\r\nLocation: /\r\nConnection: keep-alive\r\n", "");
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            v->unusedData = data;
        }
        else
        {
            v->timeAtLastMessage = std::chrono::steady_clock::now();
	    data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
	    v->unusedData = data;
            switch (Round::instance->phase)
            {
                case Round::Phase::ERROR:
                case Round::Phase::INIT: break;
                case Round::Phase::WAIT:
                    sendResponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", flag == 2 ? "HTTPFiles/waiting.html" : "");
                    break;
                case Round::Phase::RUNNING:
                case Round::Phase::DONE:
                    // TODO better (RUNNING & DONE http response)
                    sendResponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", flag == 2 ? "HTTPFiles/running.html" : "");
                    break;
                case Round::Phase::CLOSED: break; // TODO (CLOSED http response), not necessary
            }
        }
        return true;
    }
    v->unusedData = data;
    return false;
}


bool ConnectionManager::sendResponse(Viewer* v, std::string header, std::string filename)
{
    if (!isValid(v))
    {
        return false;
    }
    if (filename != "")
    {
        std::ifstream f(filename, std::ios::binary | std::ios::ate);
        if (!f.is_open())
        {
            return true;
        }
        int filesize = (int)f.tellg();
        std::string fileData = "";
        f.seekg(0);
        fileData.append(filesize, '\0');
        char* buf = (char*)fileData.data();
        f.read(buf, filesize);
        f.close();
        if (filename.compare(filename.length()-5, 5, ".html") == 0)
        {
            for (size_t widthpos = fileData.find("__WIDTH"); widthpos != std::string::npos; widthpos = fileData.find("__WIDTH"))
            {
                fileData.replace(widthpos, 7, std::to_string(Round::instance->map->size.x));
            }
            for (size_t widthpos = fileData.find("__HEIGHT"); widthpos != std::string::npos; widthpos = fileData.find("__HEIGHT"))
            {
                fileData.replace(widthpos, 8, std::to_string(Round::instance->map->size.y));
            }
            filesize = fileData.length();
        }
        std::string contentSize = "Server: Toilet\r\nContent-Length: " + std::to_string(filesize) + "\r\n\r\n";
        header.append(contentSize);
        header.append(fileData);
        if (!v->conn->send(header.data(), header.size()))
        {
            return true;
        }
        return true;
    }
    else
    {
        if (!v->conn->send(header.data(), header.size()))
        {
            return v->conn->connected();
        }
    }
    v->conn->send("Content-Length: 0\r\n\r\n", 21);
    return true;
}


bool ConnectionManager::_sendResponse(Viewer* v, std::string header, std::string content)
{
    if (!isValid(v))
    {
        return false;
    }
    std::string contentSize = "Server: Toilet\r\nContent-Length: " + std::to_string(content.length()) + "\r\n\r\n";
    header.append(contentSize);
    header.append(content);
    if (!v->conn->send(header.data(), header.size()))
    {
        return true;
    }
    v->conn->send("Content-Length: 0\r\n\r\n", 21);
    return true;
}


bool ConnectionManager::playerGreeting(Viewer* v)
{
    if (!isValid(v))
    {
        return false;
    }
    std::string data = v->unusedData;
    data += v->conn->readall();
    if (!isValid(v))
    {
        return false;
    }
    std::string comp;
    comp.append("\0\0\0\x09\0\0\0\x01\0", 9);
    size_t p = data.find(comp);
    if (p == std::string::npos)
    {
        v->unusedData = data;
        return false;
    }
    data.erase(0, p+9);
    v->unusedData = data;
    if (Round::instance->phase == Round::Phase::WAIT && Round::instance->map->nests.size() > players.size())
    {
        v->conn->send("\0\0\0\x0a\0\0\0\x01\0\0", 10); // OK
	    return true;
    }
    v->conn->send("\0\0\0\x0c\0\0\0\x02\0\x01\0\x03", 12); // DENY
    v->toClose = true;
    return false;
}


unsigned int ConnectionManager::getAGNPuint(std::string data)
{
    return ((unsigned int)(unsigned char)data[0]<<24) + ((unsigned int)(unsigned char)data[1]<<16) + ((unsigned int)(unsigned char)data[2]<<8) + (unsigned int)(unsigned char)data[3];
}


std::string ConnectionManager::makeAGNPuint(std::uint32_t num)
{
    std::string str = "";
    str.push_back((char)(num>>24));
    str.push_back((char)(num>>16 & 0xff));
    str.push_back((char)(num>>8 & 0xff));
    str.push_back((char)(num & 0xff));
    return str;
}


unsigned short ConnectionManager::getAGNPushort(std::string data)
{
    return ((unsigned int)(unsigned char)data[0]<<8) + (unsigned int)(unsigned char)data[1];
}


std::string ConnectionManager::makeAGNPushort(std::uint16_t num)
{
    std::string str;
    str.push_back((char)(num>>8));
    str.push_back((char)(num & 0xff));
    return str;
}


double ConnectionManager::getAGNPshortdouble(unsigned int num)
{
    double ret;
    ret = num>>16;
    ret += (double)(num & 0xffff) / (double)0x10000;
    return ret;
}


unsigned int ConnectionManager::makeAGNPshortdouble(double num)
{
    unsigned int ret;
    ret = (unsigned int)std::floor(num)<<16;
    ret += (unsigned int)((num - std::floor(num))*(double)0x10000);
    return ret;
}


std::uint64_t ConnectionManager::makeAGNPdouble(double num)
{
    std::uint64_t ret;
    ret = (std::uint64_t)std::floor(num)<<32;
    ret += (std::uint64_t)((num - std::floor(num))*(double)0x100000000);
    return ret;
}


double ConnectionManager::getAGNPdouble(std::uint64_t num)
{
    double ret;
    ret = num>>32;
    ret += (double)(num & 0xffffffff) / (double)0x100000000;
    return ret;
}


double ConnectionManager::getAGNPdoublestr(std::string str)
{
    std::uint64_t a = getAGNPuint(str);
    a<<=32;
    a += getAGNPuint(str.substr(4, 4));
    return getAGNPdouble(a);
}


std::string ConnectionManager::makeAGNPdoublestr(double num)
{
    std::uint64_t ret = 0;
    ret += (((std::uint64_t)std::floor(num))<<32);
    ret += (std::uint64_t)((num - std::floor(num))*(double)0x100000000);
    std::string str;
    str.push_back((char)(ret>>56));
    str.push_back((char)((ret>>48) & 0xff));
    str.push_back((char)((ret>>40) & 0xff));
    str.push_back((char)((ret>>32) & 0xff));
    str.push_back((char)((ret>>24) & 0xff));
    str.push_back((char)((ret>>16) & 0xff));
    str.push_back((char)((ret>>8) & 0xff));
    str.push_back((char)(ret & 0xff));
    return str;
}


std::string ConnectionManager::DEBUGstringToHex(std::string str)
{
    const char hexCharacterString[] = "0123456789abcdef";
    std::string ret = "";
    for (char c : str)
    {
        ret.push_back(hexCharacterString[(unsigned char)c>>4]);
        ret.push_back(hexCharacterString[(unsigned char)c&0xf]);
    }
    return ret;
}


bool ConnectionManager::interpretRequests(Player* p)
{
    if (Round::instance == nullptr || Round::instance->map == nullptr)
    {
        return false;
    }
    if (!isValid(p))
    {
        return false;
    }
    if (Round::instance->phase == Round::Phase::ERROR || Round::instance->phase == Round::Phase::CLOSED || Round::instance->phase == Round::Phase::INIT) 
    {
        return false;
    }
    std::string data = p->unusedData;
    data += p->conn->readall();
    if (!isValid(p))
    {
        return false;
    }
    if (p->messageSizeLeft == 0)
    {
        p->messageRequestsLeft = 0;
        if (data.length() >= 8)
        {
            p->messageSizeLeft = getAGNPuint(data) - 8;
            data.erase(0, 4);
            p->messageRequestsLeft = getAGNPuint(data);
            data.erase(0, 4);
        }
    }
    if (p->messageSizeLeft > 0 && data.length() >= p->messageSizeLeft)
    {
        p->timeAtLastMessage = std::chrono::steady_clock::now();
        std::string responses = "";
        std::uint32_t rq = 0;
        unsigned char unfinishedReq = 0;
        for (;p->messageRequestsLeft > 0; p->messageRequestsLeft--)
        {
            if (p->messageSizeLeft < 1)
            {
                break;
            }
            unsigned char id = data[0];
            data.erase(0, 1);
            p->messageSizeLeft -= 1;
            if (id == (unsigned char)RequestID::BYE)
            {
                if (!p->bye)
                {
                    if (responses.length() > UINT32_MAX-10)
                    {
                        std::string msg = "";
                        msg.append(makeAGNPuint(responses.length() + 8));
                        msg.append(makeAGNPuint(rq));
                        msg.append(responses);
                        p->conn->send(msg.c_str(), msg.length());
                        rq = 0;
                        responses = "";
                    }
                    responses.append("\x02\x03", 2);
                }
                p->toClose = true;
                break;
            }
            else if (id == (unsigned char)RequestID::JOIN)
            {
                if (responses.length() > UINT32_MAX-10)
                {
                    std::string msg = "";
                    msg.append(makeAGNPuint(responses.length() + 8));
                    msg.append(makeAGNPuint(rq));
                    msg.append(responses);
                    p->conn->send(msg.c_str(), msg.length());
                    rq = 0;
                    responses = "";
                }
                responses.append("\0\x01", 2);
            }
            else if (id == (unsigned char)RequestID::PING)
            {
                if (p->pinged)
                {
                    p->pinged = false; // This is of the only things the server does not respond to
                }
                else
                {
                    if (responses.length() > UINT32_MAX-10)
                    {
                        std::string msg = "";
                        msg.append(makeAGNPuint(responses.length()+8));
                        msg.append(makeAGNPuint(rq));
                        msg.append(responses);
                        p->conn->send(msg.c_str(), msg.length());
                        rq = 0;
                        responses = "";
                    }
                    responses.append("\x01\x02", 2);
                }
            }
            else if (id == (unsigned char)RequestID::SETTINGS)
            {
                std::string msg = "";
                msg.append(makeAGNPuint(responses.length()+8));
                msg.append(makeAGNPuint(rq));
                msg.append(responses);
                p->conn->send(msg.c_str(), msg.length());
                msg.clear();
                rq = 0;
                responses = "";
                std::ifstream inf(RoundSettings::instance->configFile, std::ios::in | std::ios::ate);
                if (!inf.is_open())
                {
                    p->conn->send("\0\0\0\x0e\0\0\0\x01\x05\x05\0\0\0\0", 14); // OK DATA [""]
                }
                else
                {
                    unsigned int configFileSize = inf.tellg();
                    inf.seekg(0);
                    std::cout << "Sending response to settings" << std::endl;
                    p->conn->send((makeAGNPuint(configFileSize + 14) + "\0\0\0\x01\x05\x05" + makeAGNPuint(configFileSize)).c_str(), 14); // OK DATA
                    char buf[4096];
                    unsigned int bytesSent = 0;
                    for(;;)
                    {
                        if (!inf.get(buf, 4096).fail())
                        {
                            p->conn->send(buf, inf.gcount());
                            bytesSent += inf.gcount();
                        }
                        else {break;}
                        if (inf.gcount() < 4096) {break;}
                    }
		    inf.close();
                }
            }
            else if (id == (unsigned char)RequestID::MAP)
            {
                std::string msg = "";
                msg.append(makeAGNPuint(responses.length()+8));
                msg.append(makeAGNPuint(rq));
                msg.append(responses);
                p->conn->send(msg.c_str(), msg.length());
                msg.clear();
                rq = 0;
                responses = "";
                std::uint64_t antc = 0;
                for (Nest* n : Round::instance->map->nests) {if (n) {antc += n->ants.size();}}
                msg.reserve(5UL + (std::uint64_t)Round::instance->map->nests.size() * 13UL + antc * 29UL);
                msg.append(makeAGNPushort(Round::instance->map->size.x));
                msg.append(makeAGNPushort(Round::instance->map->size.y));
                msg.push_back((char)(unsigned char)Round::instance->map->nests.size());
                for (Nest* n : Round::instance->map->nests)
                {
                    if (n)
                    {
                        msg.append(makeAGNPdoublestr(n->foodCount));
                        msg.append(makeAGNPushort(n->p.x));
                        msg.append(makeAGNPushort(n->p.y));
                        msg.push_back((char)(unsigned char)n->ants.size());
                        for (Ant * a : n->ants)
                        {
                            msg.append(makeAGNPuint(makeAGNPshortdouble(a->p.x)));
                            msg.append(makeAGNPuint(makeAGNPshortdouble(a->p.y)));
                            msg.push_back((char)a->type);
                            msg.append(makeAGNPuint(a->pid));
                            msg.append(makeAGNPdoublestr(a->health));
                            msg.append(makeAGNPdoublestr(a->foodCarry));
                        }
                    }
                    else
                    {
                        msg.append("\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\0", 13);
                    }
                }
                std::string head = "";
                head.append(makeAGNPuint(10+msg.length()+(unsigned int)Round::instance->map->size.x*(unsigned int)Round::instance->map->size.y));
                head.append(makeAGNPuint(1));
                head.append("\x09\x05", 2);
                p->conn->send(head.c_str(), head.length());
                p->conn->send(msg.c_str(), msg.length());
                p->conn->send((char*)Round::instance->map->map, (unsigned int)Round::instance->map->size.x * (unsigned int)Round::instance->map->size.y);
                p->antEventPos = --antEventQueue.end();
                p->mapEventPos = --mapEventQueue.end();
            }
            else
            {
                bool endLoop = false;
                switch (Round::instance->phase)
                {
                    case Round::Phase::INIT:
                    case Round::Phase::DONE:
                    case Round::Phase::CLOSED:
                    case Round::Phase::ERROR:
                        endLoop = true;
                        break;
                    case Round::Phase::WAIT:
                        switch (id)
                        {
                            case (unsigned char)RequestID::NAME:{
                                if (p->messageSizeLeft < 1)
                                {
                                    unfinishedReq = id;
				                    break;
                                }
                                unsigned char nameSize = (unsigned char)data[0];
                                if (nameSize > 128) {nameSize = 128;}
                                p->messageSizeLeft -= 1;
                                data.erase(0, 1);
                                if (p->messageSizeLeft < nameSize)
                                {
                                    unfinishedReq = id;
                                    break;
                                }
                                p->name = "";
                                p->name.reserve(nameSize);
                                for (int i = 0; i < nameSize; i++)
                                {
                                    if ((int)data[i] >= 32 && (int)data[i] <= 126)
                                    {
                                        switch(data[i])
                                        {
                                            case '<':
                                                p->name.append("&lt;");
                                                break;
                                            case '>':
                                                p->name.append("&gt;");
                                                break;
                                            case '&':
                                                p->name.append("&amp;");
                                                break;
                                            case '\'':
                                                p->name.append("&apos;");
                                                break;
                                            case '"':
                                                p->name.append("&quot;");
                                                break;
                                            default:
                                                p->name.push_back(data[i]);
                                                break;
                                        }
                                    }
                                }
                                data.erase(0, nameSize);
                                p->messageSizeLeft -= nameSize;
                                if (responses.length() > UINT32_MAX-10)
                                {
                                    std::string msg = "";
                                    msg.append(makeAGNPuint(responses.length()+8));
                                    msg.append(makeAGNPuint(rq));
                                    msg.append(responses);
                                    p->conn->send(msg.c_str(), msg.length());
                                    rq = 0;
                                    responses = "";
                                }
                                responses.append("\x03\0", 2);
                                break;}
                            default:{
                                if (responses.length() > UINT32_MAX-10)
                                {
                                    std::string msg = "";
                                    msg.append(makeAGNPuint(responses.length()+8));
                                    msg.append(makeAGNPuint(rq));
                                    msg.append(responses);
                                    p->conn->send(msg.c_str(), msg.length());
                                    rq = 0;
                                    responses = "";
                                }
                                responses.push_back((char)id);
                                responses.append("\x01", 1);
                                break;}
                        }
                        break;
                    case Round::Phase::RUNNING:
                        switch (id)
                        {
                            case (unsigned char)RequestID::TINTERACT:{
                                if (p->messageSizeLeft < 8)
                                {
                                    unfinishedReq = id;
                                    break;
                                }
                                if (responses.length() > UINT32_MAX-10)
                                {
                                    std::string msg = "";
                                    msg.append(makeAGNPuint(responses.length()+8));
                                    msg.append(makeAGNPuint(rq));
                                    msg.append(responses);
                                    p->conn->send(msg.c_str(), msg.length());
                                    rq = 0;
                                    responses = "";
                                }
                                p->messageSizeLeft -= 8;
                                Command cmd;
                                cmd.nestID = p->nestID;
                                cmd.antID = getAGNPuint(data);
                                data.erase(0, 4);
                                cmd.cmd = Command::ID::TINTERACT;
                                cmd.arg = getAGNPuint(data);
                                data.erase(0, 4);
                                if (cmd.antID > Round::instance->map->antPermanents.size() || !Round::instance->map->antPermanents[cmd.antID] || Round::instance->map->antPermanents[cmd.antID]->commands.size() >= 0xff || Round::instance->map->antPermanents[cmd.antID]->parent != Round::instance->map->nests[p->nestID])
                                {
                                    responses.append("\x06\x01", 2);
                                }
                                else
                                {
                                    commands.push_back(cmd);
                                    responses.append("\x06\0", 2);
                                }
                                break;}
                            case (unsigned char)RequestID::WALK:{
                                if (p->messageSizeLeft < 12)
                                {
                                    unfinishedReq = id;
                                    break;
                                }
                                if (responses.length() > UINT32_MAX-10)
                                {
                                    std::string msg = "";
                                    msg.append(makeAGNPuint(responses.length()+8));
                                    msg.append(makeAGNPuint(rq));
                                    msg.append(responses);
                                    p->conn->send(msg.c_str(), msg.length());
                                    rq = 0;
                                    responses = "";
                                }
                                p->messageSizeLeft -= 12;
                                Command cmd;
                                cmd.nestID = p->nestID;
                                cmd.antID = getAGNPuint(data);
                                data.erase(0, 4);
                                cmd.cmd = Command::ID::MOVE;
                                cmd.arg = getAGNPuint(data);
                                cmd.arg<<=32;
                                data.erase(0, 4);
                                cmd.arg += getAGNPuint(data);
                                data.erase(0, 4);
                                if (cmd.antID > Round::instance->map->antPermanents.size() || !Round::instance->map->antPermanents[cmd.antID] || Round::instance->map->antPermanents[cmd.antID]->commands.size() >= 0xff || Round::instance->map->antPermanents[cmd.antID]->parent != Round::instance->map->nests[p->nestID])
                                {
                                    responses.append("\x04\x01", 2);
                                }
                                else
                                {
                                    commands.push_back(cmd);
                                    responses.append("\x04\0", 2);
                                }
                                break;}
                            case (unsigned char)RequestID::AINTERACT:{
                                if (p->messageSizeLeft < 8)
                                {
                                    unfinishedReq = id;
                                    break;
                                }
                                if (responses.length() > UINT32_MAX-10)
                                {
                                    std::string msg = "";
                                    msg.append(makeAGNPuint(responses.length()+8));
                                    msg.append(makeAGNPuint(rq));
                                    msg.append(responses);
                                    p->conn->send(msg.c_str(), msg.length());
                                    rq = 0;
                                    responses = "";
                                }
                                p->messageSizeLeft -= 8;
                                Command cmd;
                                cmd.nestID = p->nestID;
                                cmd.antID = getAGNPuint(data);
                                data.erase(0, 4);
                                cmd.cmd = Command::ID::AINTERACT;
                                cmd.arg = getAGNPuint(data);
                                data.erase(0, 4);
                                if (cmd.antID > Round::instance->map->antPermanents.size() || !Round::instance->map->antPermanents[cmd.antID] || Round::instance->map->antPermanents[cmd.antID]->commands.size() >= 0xff || Round::instance->map->antPermanents[cmd.antID]->parent != Round::instance->map->nests[p->nestID])
                                {
                                    responses.append("\x07\x01", 2);
                                }
                                else
                                {
                                    commands.push_back(cmd);
                                    responses.append("\x07\0", 2);
                                }
                                break;}
                            case (unsigned char)RequestID::NEWANT:{
                                if (p->messageSizeLeft < 1)
                                {
                                    unfinishedReq = id;
                                    break;
                                }
                                if (responses.length() > UINT32_MAX-10)
                                {
                                    std::string msg = "";
                                    msg.append(makeAGNPuint(responses.length()+8));
                                    msg.append(makeAGNPuint(rq));
                                    msg.append(responses);
                                    p->conn->send(msg.c_str(), msg.length());
                                    rq = 0;
                                    responses = "";
                                }
                                p->messageSizeLeft -= 1;
                                Command cmd;
                                cmd.nestID = p->nestID;
                                cmd.antID = UINT32_MAX;
                                cmd.cmd = Command::ID::NEWANT;
                                cmd.arg = (unsigned char)data[0];
                                data.erase(0, 1);
                                if (cmd.arg >= antTypec || Round::instance->map->nests[cmd.nestID]->commands.size() >= 0xff)
                                {
                                    responses.append("\x08\x01", 2);
                                }
                                else
                                {
                                    commands.push_back(cmd);
                                    responses.append("\x08\0", 2);
                                }
                                break;}
                            case (unsigned char)RequestID::CHANGELOG:{
                                std::string msg = "";
                                msg.append(makeAGNPuint(responses.length()+8));
                                msg.append(makeAGNPuint(rq));
                                msg.append(responses);
                                p->conn->send(msg.c_str(), msg.length());
                                msg.clear();
                                rq = 0;
                                responses = "";
                                std::uint64_t antc = 0;
                                for (Nest* n : Round::instance->map->nests) {if (n) {antc += n->ants.size();}}
                                msg.reserve((std::uint64_t)Round::instance->map->nests.size() * 9UL + antc * 13UL);
                                std::cout << "Nests: " << Round::instance->map->nests.size() << std::endl;
                                for (Nest* n : Round::instance->map->nests)
                                {
                                    if (n)
                                    {
                                        std::cout << "Nest w/ " << n->ants.size() << " ants at " << msg.size() << std::endl;
                                        msg.append(makeAGNPdoublestr(n->foodCount));
                                        msg.push_back((char)(unsigned char)std::min(n->ants.size(), (std::size_t)0xfe));
                                        unsigned char anti = 0;
                                        for (Ant * a : n->ants)
                                        {
                                            msg.append(makeAGNPuint(makeAGNPshortdouble(a->p.x)));
                                            msg.append(makeAGNPuint(makeAGNPshortdouble(a->p.y)));
                                            msg.push_back((char)a->type);
                                            msg.append(makeAGNPuint(a->pid));
                                            anti++;
                                            if (anti == 0) {break;}
                                        }
                                    }
                                    else
                                    {
                                        msg.append("\xff\xff\xff\xff\xff\xff\xff\xff\0", 9);
                                    }
                                }
                                std::cout << "Done at " << msg.size() << std::endl;
                                std::string events;
                                unsigned int ec = 0;
                                for (;!mapEventQueue.empty();)
                                {
                                    if (p->mapEventPos != --mapEventQueue.end())
                                    {
                                        p->mapEventPos++;
                                        events.append(makeAGNPushort((*p->mapEventPos).x));
                                        events.append(makeAGNPushort((*p->mapEventPos).y));
                                        events.push_back((char)(*p->mapEventPos).tile);
                                        ec++;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                events.insert(0, makeAGNPuint(ec));
                                ec = 0;
                                std::size_t insertPos = events.size();
                                for (;!antEventQueue.empty();)
                                {
                                    if (p->antEventPos != --antEventQueue.end())
                                    {
                                        p->antEventPos++;
                                        events.append(makeAGNPuint((*p->antEventPos).pid));
                                        events.append(makeAGNPdoublestr((*p->antEventPos).health));
                                        events.append(makeAGNPdoublestr((*p->antEventPos).foodCarry));
                                        ec++;
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                events.insert(insertPos, makeAGNPuint(ec));
                                std::string head = "";
                                head.append(makeAGNPuint(10+msg.length()+events.length()));
                                head.append(makeAGNPuint(1));
                                head.append("\x0a\x05", 2);
                                std::cout << head.length() + msg.length() << ", " << events.length() << ", " << head.length() + msg.length() + events.length() << std::endl;
                                p->conn->send(head.c_str(), head.length());
                                p->conn->send(msg.c_str(), msg.length());
                                p->conn->send(events.c_str(), events.length());
                                break;}
                            case (unsigned char)RequestID::ME:
                                if (responses.length() > UINT32_MAX - 11)
                                {
                                    std::string msg = "";
                                    msg.append(makeAGNPuint(responses.length()+8));
                                    msg.append(makeAGNPuint(rq));
                                    msg.append(responses);
                                    p->conn->send(msg.c_str(), msg.length());
                                    rq = 0;
                                    responses = "";
                                }
                                responses.append("\x0b\x05");
                                responses.push_back((char)p->nestID);
                                break;
                            default:{
                                if (responses.length() > UINT32_MAX-10)
                                {
                                    std::string msg = "";
                                    msg.append(makeAGNPuint(responses.length()+8));
                                    msg.append(makeAGNPuint(rq));
                                    msg.append(responses);
                                    p->conn->send(msg.c_str(), msg.length());
                                    rq = 0;
                                    responses = "";
                                }
                                responses.push_back((char)id);
                                responses.push_back('\x01');
                                break;}
                        }
                        break;
                }
                if (endLoop) {break;}
            }
            rq++;
        }
        data.erase(0, p->messageSizeLeft);
        p->messageSizeLeft = 0;
        if (rq > 0 && responses.length() > 0)
        {
            std::string msg = "";
            msg.append(makeAGNPuint(responses.length()+8));
            msg.append(makeAGNPuint(rq));
            msg.append(responses);
            p->conn->send(msg.c_str(), msg.length());
        }
        if (unfinishedReq != 0)
        {
            std::string msg = "";
            msg.append("\0\0\0\x0a\0\0\0\x01", 8); // Msg size 10, 1 response
            msg.push_back((char)unfinishedReq);
            msg.push_back('\x01');
            p->conn->send(msg.data(), msg.length());
        }
        if (p->bye)
        {
            p->toClose = true;
        }
    }
    p->unusedData = data;
    return isValid(p);
}
