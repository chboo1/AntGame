#include "network.hpp"
#include "map.hpp"
#include "antTypes.hpp"
#include <fstream>
#include <climits>
#include <cstdint>
#include <deque>


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
	startingFood = (unsigned int)std::stoi(data);
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
    else if (identifier == "basehealth")
    {
	try{
	antHealth = std::stod(data);
	}catch(...){
        antHealth = 5.0;
	std::cerr << "The `basehealth' argument of the configuration file is not a valid number! Remember not to include any spaces." << std::endl;
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
}


Player::Player(Connection*nconn)
{
    conn = nconn;
    timeAtLastMessage = std::chrono::steady_clock::now();
}


Player::Player(Viewer v)
{
    conn = v.conn;
    timeAtLastMessage = std::chrono::steady_clock::now();
    unusedData = v.unusedData;
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
    if (conn != nullptr)
    {
        delete conn;
    }
}


ConnectionManager::ConnectionManager() {}


void ConnectionManager::start()
{
    for (Player* p : players)
    {
        if (isValid(p))
        {
            p->conn->send("\0\0\0\x0a\0\0\0\x01\0\x04", 10); // START
        }
        if (!isValid(p)) // Running check again to detect if it changed when sending data
        {
            p->toClose = true;
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
            // TODO handlePlayers ; Send command feedback
        }
    }
    std::chrono::steady_clock::time_point timpont = std::chrono::steady_clock::now();
    for (int i = 0; i < players.size(); i++)
    {
        Player* p = players[i];
        double timeSinceMessage = std::chrono::duration<double>(timpont-p->timeAtLastMessage).count();
        if (!isValid(p) || timeSinceMessage >= (p->bye?1.0:3.0)) {delete p; players.erase(players.begin() + i);i--;}
        else if (timeSinceMessage >= 1.0 && !p->pinged)
        {
            p->pinged = true;
            p->conn->send("\0\0\0\x0a\0\0\0\x01\0\x02", 10); // PING REQ
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
    	        Player* p = new Player(*v);
	        p->nestID = players.size();
	        players.push_back(p);
	        v->conn = nullptr; // So the destructor doesn't kill it
            }
	    v->toClose = true;
	}
    }
    auto prev = viewers.before_begin();
    bool del = false;
    std::chrono::steady_clock::time_point timpont = std::chrono::steady_clock::now();
    for (auto it = viewers.begin(); it != viewers.end(); it++)
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
        del = false;
        Viewer* v = *it;
        if (!isValid(v) || std::chrono::duration<double>(timpont - v->timeAtLastMessage).count() >= 3.0) {del = true;}
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
        if (data.compare(data.find(' ') + 1, 12, "/favicon.ico") == 0)
        {
            sendResponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", flag == 2 ? "HTTPFiles/favicon.ico" : "");
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            v->unusedData = data;
        }
        else if (data.compare(data.find(' ') + 1, 2, "/ ") != 0)
        {
            sendResponse(v, "HTTP/1.1 301 Moved Permanently\r\nLocation: /\r\nConnection: keep-alive\r\n", "");
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
                    // TODO better (WAIT http response)
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
        std::string contentSize = "Server: Toilet\r\nContent-Length: " + std::to_string(filesize) + "\r\n\r\n";
        f.seekg(0);
        long pos = contentSize.length();
        contentSize.append(filesize, '\0');
        char* buf = (char*)contentSize.data();
        buf += pos;
        f.read(buf, filesize);
        f.close();
        header.append(contentSize);
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


bool ConnectionManager::playerGreeting(Viewer* v)
{
    if (!isValid(v))
    {
        return false;
    }
    std::string data = v->unusedData;
    data += v->conn->readall();
    std::string comp;
    comp.append("\0\0\0\x11\0\0\0\x01\0", 9);
    size_t p = data.find(comp);
    if (p == std::string::npos)
    {
        v->unusedData = data;
        return false;
    }
    data.erase(0, p+9);
    v->unusedData = data;
    if (Round::instance->phase == Round::Phase::WAIT || Round::instance->map->nests.size() < players.size())
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
    return ((unsigned int)data[0]<<24) + ((unsigned int)data[1]<<16) + ((unsigned int)data[2]<<8) + (unsigned int)data[3];
}


std::string ConnectionManager::makeAGNPuint(std::uint32_t num)
{
    std::string str;
    str.push_back((char)(num & 0xff));
    str.push_back((char)(num>>8 & 0xff));
    str.push_back((char)(num>>16 & 0xff));
    str.push_back((char)(num>>24 & 0xff));
    return str;
}


bool ConnectionManager::interpretRequests(Player* p)
{
    if (Round::instance == nullptr)
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
            rq++;
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
                responses.append("\x00\x01", 2);
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
				if (p->messageSizeLeft < 4)
				{
                                    unfinishedReq = id;
				    break;
				}
				std::uint32_t nameSize = getAGNPuint(data);
				p->messageSizeLeft -= 4;
				data.erase(0, 4);
				if (p->messageSizeLeft < nameSize)
				{
                                    unfinishedReq = id;
				    break;
				}
				p->name = data.substr(0, nameSize);
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
                                responses.append("\x03\x00", 2);
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
                                cmd.cmd = Command::ID::TINTERACT;
                                cmd.arg = getAGNPuint(data);
                                cmd.arg<<=32;
                                data.erase(0, 4);
                                cmd.arg += getAGNPuint(data);
                                data.erase(0, 4);
                                if (cmd.antID > Round::instance->map->nests[cmd.nestID]->ants.size())
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
                                if (cmd.antID > Round::instance->map->nests[cmd.nestID]->ants.size())
                                {
                                    responses.append("\x06\x01", 2);
                                }
                                else
                                {
                                    commands.push_back(cmd);
                                    responses.append("\x06\0", 2);
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
                                if (cmd.antID > Round::instance->map->nests[cmd.nestID]->ants.size())
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
                                if (cmd.arg >= antTypec)
                                {
                                    responses.append("\x07\x01", 2);
                                }
                                else
                                {
                                    commands.push_back(cmd);
                                    responses.append("\x07\0", 2);
                                }
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
				responses.push_back('\x01');
                                break;}
                        }
                        break;
                }
                if (endLoop) {break;}
            }
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
