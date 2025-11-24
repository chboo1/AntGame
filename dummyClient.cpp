#include "map.hpp"
#include "sockets.hpp"
#include "network.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>


int main(int argc, char*args[])
{
    Connection conn;
    std::string name = "Nest with no conceivable name.";
    if (argc >= 2)
    {
        name = args[1];
        if (name.length() > 127)
        {
            name.erase(127, std::string::npos);
        }
    }
    if (!conn.connectTo("127.0.0.1", 42069))
    {
        std::cerr << "Start the server first silly." << std::endl;
        return 1;
    }
    if (!conn.send("\0\0\0\x09\0\0\0\x01\0", 9))
    {
        std::cerr << "Failed to send join request over..." << std::endl;
        conn.finish();
        return 1;
    }
    std::string ideal;
    ideal.append("\0\0\0\x0a\0\0\0\x01\0", 9);
    std::string recvData;
    for (recvData = ""; recvData.empty(); recvData = conn.readall())
    {
        if (!conn.connected())
        {
            std::cerr << "Kicked out by the server before reading any data." << std::endl;
            return 1;
        }
    }
    if (recvData.length() != 10 || recvData.compare(0, 9, ideal) != 0)
    {
        std::cerr << "Got response from server, but doesn't conform to AGNP" << std::endl;
        conn.finish();
        return 2;
    }
    recvData.erase(0, 9);
    if (recvData[0] == '\x01')
    {
        std::cerr << "Denied by server." << std::endl;
        conn.finish();
        return 3;
    }
    else if (recvData[1] != '\0')
    {
        std::cerr << "Got response from server, but inappropriate response ID" << std::endl;
        conn.finish();
        return 2;
    }
    std::cout << "Successfully connected to server. Using name " << name << " " << name.length() << std::endl;
    ideal.clear();
    ideal.append(ConnectionManager::makeAGNPuint(name.length() + 10));
    ideal.append(ConnectionManager::makeAGNPuint(1));
    ideal.push_back('\x03');
    ideal.push_back((char)name.length());
    ideal.append(name);
    if (!conn.send(ideal.c_str(), ideal.length()))
    {
        std::cerr << "Network error while sending cool requests" << std::endl;
        conn.finish();
        return 1;
    }
    ideal.clear();
    bool foundStart = false;
    bool gotNameOk = false;
    for (recvData.clear();!foundStart && conn.connected();recvData = conn.readall())
    {
        if (recvData.length() >= 10)
        {
            unsigned int responsesLen = ConnectionManager::getAGNPuint(recvData);
            std::string responses = recvData.substr(8, responsesLen-8);
            unsigned int rc = ConnectionManager::getAGNPuint(recvData.substr(4, 4));
            recvData.clear();
            while (responses.length() >= 2 && rc > 0)
            {
                rc--;
                switch (responses[1])
                {
                    case '\x02': // PING
                        conn.send("\0\0\0\x09\0\0\0\x01\x01", 9);
                        break;
                    case '\x03': // BYE
                        std::cout << "Peacefully kicked tf out by the server." << std::endl;
                        conn.send("\0\0\0\x09\0\0\0\x01\x02", 9);
                        conn.finish();
                        return 4;
                    case '\x04': // START
                        foundStart = true;
                        break;
                    case '\0': // OK
                        if (!gotNameOk)
                        {
                            gotNameOk = true;
                            break;
                        }
                    default:
                        std::cerr << "Invalid response ID from server!" << std::endl;
                        conn.finish();
                        return 2;
                }
                responses.erase(0, 2);
            }
        }
    }
    if (conn.connected())
    {
        std::cout << "Server started game!" << std::endl;
    }
    else
    {
        std::cerr << "Lost connection to server while waiting for start of game!" << std::endl;
        conn.finish();
        return 1;
    }
    Map map;
    conn.send("\0\0\0\x0a\0\0\0\x02\x0b\x09", 10);
    bool dead = false;
    unsigned char selfNestID = 0xff;
    bool hasMap = false;
    std::deque<ConnectionManager::RequestID> reqs;
    std::deque<ConnectionManager::RequestID> cmdIDs;
    std::deque<unsigned int> cmdpids;
    std::deque<std::uint64_t> cmdargs;
    std::deque<Pos> targetedFood;
    reqs.push_back(ConnectionManager::RequestID::MAP);
    reqs.push_back(ConnectionManager::RequestID::ME);
    for (recvData.clear(); !dead; recvData = conn.readall())
    {
        if (!conn.connected())
        {
            std::cerr << "Connection was closed unexpectedly!" << std::endl;
            conn.finish();
            map.cleanup();
            return 1;
        }
        bool updateCommands = false;
        bool sendChangelogReq = false;
        while (recvData.length() >= 10)
        {
            unsigned int responsesLen = ConnectionManager::getAGNPuint(recvData);
            recvData.erase(0, 4);
            unsigned int rq = ConnectionManager::getAGNPuint(recvData);
            recvData.erase(0, 4);
            responsesLen -= 8;
            while (recvData.length() < responsesLen)
            {
                recvData.append(conn.readall());
            }
            while (responsesLen >= 2 && recvData.length() >= 2)
            {
                std::cout << "Decyphering... length of data:" << recvData.length() << ", amount of data (according to server):" << responsesLen << std::endl;
                std::cout << (unsigned int)(unsigned char)recvData[0] << ", " << (unsigned int)(unsigned char)recvData[1] << std::endl;
                bool corresponds = false;
                if (recvData[0] != (char)ConnectionManager::RequestID::NONE)
                {
                    for (auto it = reqs.begin(); it != reqs.end(); it++)
                    {
                        if ((char)(*it) == recvData[0])
                        {
                            reqs.erase(it);
                            corresponds = true;
                            break;
                        }
                    }
                    if (!corresponds)
                    {
                        std::cerr << "Server sent a response to a non-existent message: " << (unsigned int)(unsigned char)recvData[0] << std::endl;
                        conn.finish();
                        map.cleanup();
                        return 2;
                    }
                }
                switch (recvData[0])
                {
                    case (char)ConnectionManager::RequestID::MAP:{
                        if (recvData[1] != (char)ConnectionManager::ResponseID::OKDATA)
                        {
                            std::cerr << "Server sent wrong response code to map request: " << (unsigned int)(unsigned char)recvData[1] << std::endl;
                            conn.finish();
                            map.cleanup();
                            return 2;
                        }
                        map.cleanup();
                        map.size.x = ConnectionManager::getAGNPushort(recvData.substr(2, 2));
                        map.size.y = ConnectionManager::getAGNPushort(recvData.substr(4, 2));
                        unsigned char nestc = (unsigned char)recvData[6];
                        map.nests.reserve(nestc);
                        unsigned int index = 7;
                        unsigned int biggestPid = 0;
                        for (unsigned char i = 0; i < nestc; i++)
                        {
                            if (recvData.length() < index + 13 || responsesLen < index + 13)
                            {
                                std::cerr << "Ran out of data while trying to get data of a nest in a MAP object." << std::endl;
                                map.cleanup();
                                conn.finish();
                                return 2;
                            }
                            else if (recvData.compare(index, 8, "\xff\xff\xff\xff\xff\xff\xff\xff") == 0 || recvData.compare(index+8, 2, "\xff\xff") == 0 || recvData.compare(index+10, 2, "\xff\xff") == 0 || recvData[index+11] == '\xff')
                            {
                                map.nests.push_back(nullptr);
                                index += 13;
                            }
                            else
                            {
                                Nest*n = new Nest;
                                n->foodCount = ConnectionManager::getAGNPdoublestr(recvData.substr(index, 8));
                                n->p.x = ConnectionManager::getAGNPushort(recvData.substr(index + 8, 2));
                                n->p.y = ConnectionManager::getAGNPushort(recvData.substr(index + 10, 2));
                                unsigned char antc = (unsigned char)recvData[index+12];
                                n->ants.reserve(antc);
                                n->parent = &map;
                                index += 13;
                                if (recvData.length() < index + 29 * antc || responsesLen < index + 29 * antc)
                                {
                                    std::cerr << "Ran out of data while trying to get data of an ant in a MAP object." << std::endl;
                                    conn.finish();
                                    delete n;
                                    map.cleanup();
                                    return 2;
                                }
                                for (unsigned char j = 0; j < antc; j++)
                                {
                                    Ant*a = new Ant;
                                    a->p.x = ConnectionManager::getAGNPshortdouble(ConnectionManager::getAGNPuint(recvData.substr(index, 4)));
                                    a->p.y = ConnectionManager::getAGNPshortdouble(ConnectionManager::getAGNPuint(recvData.substr(index + 4, 4)));
                                    a->type = recvData[index+8];
                                    a->pid = ConnectionManager::getAGNPuint(recvData.substr(index+9, 4));
                                    a->health = ConnectionManager::getAGNPdoublestr(recvData.substr(index+13, 8));
                                    a->foodCarry = ConnectionManager::getAGNPdoublestr(recvData.substr(index+21, 8));
                                    a->parent = n;
                                    index += 29;
                                    if (a->pid != 0xffffffff)
                                    {
                                        biggestPid = std::max(biggestPid, a->pid);
                                    }
                                    n->ants.push_back(a);
                                }
                                map.nests.push_back(n);
                            }
                        }
                        map.antPermanents.resize(biggestPid + 1, nullptr);
                        for (Nest*n : map.nests)
                        {
                            if (n)
                            {
                                for (Ant*a : n->ants)
                                {
                                    map.antPermanents[a->pid] = a;
                                }
                            }
                        }
                        if (recvData.length() < index + (unsigned int)map.size.x * (unsigned int)map.size.y || responsesLen < index + (unsigned int)map.size.x * (unsigned int)map.size.y)
                        {
                            std::cerr << "Ran out of data while trying to get map data in a MAP object." << std::endl;
                            conn.finish();
                            map.cleanup();
                            return 2;
                        }
                        map.map = new unsigned char[(unsigned int)map.size.x * (unsigned int)map.size.y];
                        recvData.copy((char*)map.map, index, (unsigned int)map.size.x * (unsigned int)map.size.y);
                        recvData.erase(0, index + (unsigned int)map.size.x * (unsigned int)map.size.y);
                        responsesLen -= index + (unsigned int)map.size.x * (unsigned int)map.size.y;
                        sendChangelogReq = true;
                        hasMap = true;
                        break;}
                    case (char)ConnectionManager::RequestID::CHANGELOG:{
                        if (recvData[1] != (char)ConnectionManager::ResponseID::OKDATA)
                        {
                            std::cerr << "Server sent wrong response code to changelog request: " << (unsigned int)(unsigned char)recvData[1] << std::endl;
                            conn.finish();
                            map.cleanup();
                            return 2;
                        }
                        unsigned int index = 2;
                        std::vector<bool> antExists;
                        antExists.resize(map.antPermanents.size(), false);
                        Ant*a = new Ant;
                        unsigned char nid = 0;
                        for (Nest*n:map.nests)
                        {
                            if (n)
                            {
                                if (recvData.length() < index + 9 || responsesLen < index + 9)
                                {
                                    std::cerr << "Ran out of data while trying to get nest data in a CHANGELOG object." << std::endl;
                                    conn.finish();
                                    map.cleanup();
                                    delete a;
                                    return 2;
                                }
                                else if (recvData.compare(0, 8, "\xff\xff\xff\xff\xff\xff\xff\xff") == 0 || recvData[8] == '\xff')
                                {
                                    delete n;
                                    map.nests[nid] = nullptr;
                                    continue;
                                }
                                n->ants.clear();
                                n->foodCount = ConnectionManager::getAGNPdoublestr(recvData.substr(index, 8));
                                unsigned char antc = (unsigned char)recvData[index+8];
                                n->ants.reserve(antc);
                                index += 9;
                                if (recvData.length() < index + antc * 13 || responsesLen < index + antc * 13)
                                {
                                    std::cerr << "Ran out of data while trying to get ant data in a CHANGELOG object." << std::endl;
                                    conn.finish();
                                    map.cleanup();
                                    delete a;
                                    return 2;
                                }
                                for (unsigned char i = 0; i < antc; i++)
                                {
                                    a->p.x = ConnectionManager::getAGNPshortdouble(ConnectionManager::getAGNPuint(recvData.substr(index, 4)));
                                    a->p.y = ConnectionManager::getAGNPshortdouble(ConnectionManager::getAGNPuint(recvData.substr(index+4, 4)));
                                    a->type = recvData[index+8];
                                    a->pid = ConnectionManager::getAGNPuint(recvData.substr(index+9, 4));
                                    index += 13;
                                    if (a->pid >= map.antPermanents.size())
                                    {
                                        map.antPermanents.resize(a->pid + 1, nullptr);
                                        antExists.resize(a->pid + 1, false);
                                        map.antPermanents[a->pid] = a;
                                        antExists[a->pid] = true;
                                        n->ants.push_back(a);
                                        a = new Ant;
                                    }
                                    else
                                    {
                                        unsigned int thisPid = a->pid;
                                        if (!map.antPermanents[a->pid])
                                        {
                                            map.antPermanents[a->pid] = a;
                                            a = new Ant;
                                        }
                                        else
                                        {
                                            map.antPermanents[a->pid]->p.x = a->p.x;
                                            map.antPermanents[a->pid]->p.y = a->p.y;
                                            map.antPermanents[a->pid]->type = a->type;
                                        }
                                        antExists[thisPid] = true;
                                        n->ants.push_back(map.antPermanents[thisPid]);
                                    }
                                }
                            }
                            else
                            {
                                index += 9;
                            }
                            nid++;
                        }
                        delete a;
                        for (unsigned int i = 0; i < map.antPermanents.size(); i++)
                        {
                            if (!antExists[i])
                            {
                                if (map.antPermanents[i])
                                {
                                    delete map.antPermanents[i];
                                    map.antPermanents[i] = nullptr;
                                }
                            }
                        }
                        if (recvData.length() < index + 4 || responsesLen < index + 4)
                        {
                            std::cerr << "Ran out of data while trying to get map event amount in CHANGELOG object." << std::endl;
                            conn.finish();
                            map.cleanup();
                            delete a;
                            return 2;
                        }
                        unsigned int mec = ConnectionManager::getAGNPuint(recvData.substr(index, 4));
                        index += 4;
                        if (recvData.length() < index + 5*mec || responsesLen < index + 5*mec)
                        {
                            std::cerr << "Ran out of data while trying to get map events in CHANGELOG object." << std::endl;
                            conn.finish();
                            map.cleanup();
                            return 2;
                        }
                        for (unsigned int i = 0; i < mec; i++)
                        {
                            unsigned short x = ConnectionManager::getAGNPushort(recvData.substr(index, 2));
                            unsigned short y = ConnectionManager::getAGNPushort(recvData.substr(index+2, 2));
                            unsigned char tile = (unsigned char)recvData[index+4];
                            map.map[x+y*map.size.x] = tile;
                            index += 5;
                        }
                        if (recvData.length() < index + 4 || responsesLen < index + 4)
                        {
                            std::cerr << "Ran out of data while trying to get ant event amount in CHANGELOG object." << std::endl;
                            conn.finish();
                            map.cleanup();
                            return 2;
                        }
                        unsigned int aec = ConnectionManager::getAGNPuint(recvData.substr(index, 4));
                        index += 4;
                        if (recvData.length() < index + 20*aec || responsesLen < index + 20*aec)
                        {
                            std::cerr << "Ran out of data while trying to get map events in CHANGELOG object." << std::endl;
                            conn.finish();
                            map.cleanup();
                            return 2;
                        }
                        for (unsigned int i = 0; i < aec; i++)
                        {
                            unsigned int pid = ConnectionManager::getAGNPuint(recvData.substr(index, 4));
                            double health = ConnectionManager::getAGNPdoublestr(recvData.substr(index+4, 8));
                            double food = ConnectionManager::getAGNPdoublestr(recvData.substr(index+12, 8));
                            if (map.antPermanents.size() > pid && map.antPermanents[pid])
                            {
                                map.antPermanents[pid]->foodCarry = food;
                                map.antPermanents[pid]->health = health;
                            }
                            index += 20;
                        }
                        recvData.erase(0, index);
                        responsesLen -= index;
                        sendChangelogReq = true;
                        break;}
                    case (char)ConnectionManager::RequestID::ME:
                        if (recvData[1] != (char)ConnectionManager::ResponseID::OKDATA)
                        {
                            std::cerr << "Server sent wrong response code to me request: " << (unsigned int)(unsigned char)recvData[1] << std::endl;
                            conn.finish();
                            map.cleanup();
                            return 2;
                        }
                        selfNestID = recvData[2];
                        responsesLen -= 3;
                        recvData.erase(0, 3);
                        updateCommands = true;
                        break;
                    case (char)ConnectionManager::RequestID::WALK:
                    case (char)ConnectionManager::RequestID::TINTERACT:
                        recvData.erase(0, 2);
                        responsesLen -= 2;
                        break;
                    case (char)ConnectionManager::RequestID::NONE:
                        switch (recvData[1])
                        {
                            case (char)ConnectionManager::ResponseID::PING:
                                conn.send("\0\0\0\x09\0\0\0\x01\x01", 9);
                                recvData.erase(0, 2);
                                responsesLen -= 2;
                                break;
                            case (char)ConnectionManager::ResponseID::BYE:
                                conn.send("\0\0\0\x09\0\0\0\x01\x02", 9);
                                recvData.erase(0, 2);
                                responsesLen -= 2;
                                std::cout << "Kicked out by server." << std::endl;
                                conn.finish();
                                map.cleanup();
                                return 0;
                            case (char)ConnectionManager::ResponseID::CMDFAIL:
                            case (char)ConnectionManager::ResponseID::CMDSUCCESS:{
                                updateCommands = true;
                                unsigned char serverCmdId = (unsigned char)recvData[2];
                                unsigned int antPid = ConnectionManager::getAGNPuint(recvData.substr(3, 4));
                                std::uint64_t serverCmdArg = 0;
                                switch (serverCmdId)
                                {
                                    case (char)ConnectionManager::RequestID::WALK:
                                        serverCmdArg = ConnectionManager::getAGNPuint(recvData.substr(7, 4));
                                        serverCmdArg <<= 32;
                                        serverCmdArg += ConnectionManager::getAGNPuint(recvData.substr(11, 4));
                                        recvData.erase(0, 15);
                                        responsesLen -= 15;
                                        break;
                                    case (char)ConnectionManager::RequestID::TINTERACT:
                                        serverCmdArg = ConnectionManager::getAGNPuint(recvData.substr(7, 4));
                                        recvData.erase(0, 11);
                                        responsesLen -= 11;
                                        break;
                                    default:
                                        std::cerr << "Server sent a CMDFAIL/SUCCESS with impossible previous data (ID)" << (unsigned int)serverCmdId << std::endl;
                                        conn.finish();
                                        map.cleanup();
                                        return 2;
                                }
                                auto IDit = cmdIDs.begin();
                                auto PIDit = cmdpids.begin();
                                auto ARGit = cmdargs.begin();
                                bool found = false;
                                std::cout << (unsigned int)serverCmdId << ", " << antPid << ", " << serverCmdArg << std::endl;
                                for (;IDit != cmdIDs.end();)
                                {
                                    std::cout << "A: " << (unsigned int)(*IDit) << ", " << (*PIDit) << ", " << (*ARGit) << std::endl;
                                    if ((*IDit) == (ConnectionManager::RequestID)serverCmdId && (*PIDit) == antPid && (*ARGit) == serverCmdArg)
                                    {
                                        cmdIDs.erase(IDit);
                                        cmdpids.erase(PIDit);
                                        cmdargs.erase(ARGit);
                                        found = true;
                                        break;
                                    }
                                    IDit++;
                                    PIDit++;
                                    ARGit++;
                                }
                                if (!found)
                                {
                                    std::cerr << "Server sent a CMDFAIL/SUCCESS with impossible previous data" << std::endl;
                                    conn.finish();
                                    map.cleanup();
                                    return 2;
                                }
                                break;}
                            default:
                                std::cerr << "Server sent an invalid unsolicited message: " << (unsigned int)(unsigned char)recvData[1] << std::endl;
                                conn.finish();
                                map.cleanup();
                                return 2;
                        }
                        break;
                    default:
                        std::cerr << "Server responded to a message correctly, but I have no logic to process it. Idfk why. Id: " << (unsigned int)(unsigned char)recvData[0] << std::endl;
                        conn.finish();
                        map.cleanup();
                        return 2;
                }
            }
        }
        updateCommands = updateCommands || sendChangelogReq;
        if (sendChangelogReq)
        {
            conn.send("\0\0\0\x09\0\0\0\x01\x0a", 9);
            reqs.push_back(ConnectionManager::RequestID::CHANGELOG);
        }
        if (updateCommands && selfNestID != 0xff && hasMap)
        {
            for (Ant* a : map.nests[selfNestID]->ants)
            {
                bool moveCommand = false;
                bool tinterCommand = false;
                auto cmdIDit = cmdIDs.begin();
                for (auto antIDit = cmdpids.begin(); antIDit != cmdpids.end(); antIDit++)
                {
                    if ((*antIDit) != a->pid)
                    {
                        cmdIDit++;
                        continue;
                    }
                    if ((*cmdIDit) == ConnectionManager::RequestID::WALK)
                    {
                        moveCommand = true;
                    }
                    else if ((*cmdIDit) == ConnectionManager::RequestID::TINTERACT)
                    {
                        tinterCommand = true;
                    }
                    cmdIDit++;
                }
                Pos target;
                target.x = 0xffff;
                target.y = 0xffff;
                std::cout << a->foodCarry << std::endl;
                if (a->foodCarry > 0)
                {
                    target = a->parent->p;
                }
                else
                {
                    Pos center = a->p;
                    for (unsigned int size = 1; size < 50; size++)
                    {
                        for (int ox = -size + 1; ox < size; ox++)
                        {
                            if (ox + (int)center.x < 0)
                            {
                                continue;
                            }
                            if (ox + (int)center.x >= map.size.x)
                            {
                                break;
                            }
                            Pos check = center;
                            check.x += ox;
                            if (check.y >= size)
                            {
                                check.y -= size;
                                if (map[check] == Map::Tile::FOOD)
                                {
                                    bool ok = true;
                                    for (Pos p : targetedFood)
                                    {
                                        if (p == check)
                                        {
                                            ok = false;
                                            break;
                                        }
                                    }
                                    if (ok)
                                    {
                                        target = check;
                                        break;
                                    }
                                }
                                check.y += size;
                            }
                            if (map.size.y - check.y > size)
                            {
                                check.y += size;
                                if (map[check] == Map::Tile::FOOD)
                                {
                                    bool ok = true;
                                    for (Pos p : targetedFood)
                                    {
                                        if (p == check)
                                        {
                                            ok = false;
                                            break;
                                        }
                                    }
                                    if (ok)
                                    {
                                        target = check;
                                        break;
                                    }
                                }
                                check.y -= size;
                            }
                        }
                        if (target.x != 0xffff && target.y != 0xffff)
                        {
                            break;
                        }
                        for (int oy = -size; oy <= size; oy++)
                        {
                            if (oy + (int)center.y < 0)
                            {
                                continue;
                            }
                            if (oy + (int)center.y >= map.size.y)
                            {
                                break;
                            }
                            Pos check = center;
                            check.x += oy;
                            if (check.x >= size)
                            {
                                check.x -= size;
                                if (map[check] == Map::Tile::FOOD)
                                {
                                    bool ok = true;
                                    for (Pos p : targetedFood)
                                    {
                                        if (p == check)
                                        {
                                            ok = false;
                                            break;
                                        }
                                    }
                                    if (ok)
                                    {
                                        target = check;
                                        break;
                                    }
                                }
                                check.x += size;
                            }
                            if (map.size.x - check.x > size)
                            {
                                check.x += size;
                                if (map[check] == Map::Tile::FOOD)
                                {
                                    bool ok = true;
                                    for (Pos p : targetedFood)
                                    {
                                        if (p == check)
                                        {
                                            ok = false;
                                            break;
                                        }
                                    }
                                    if (ok)
                                    {
                                        target = check;
                                        break;
                                    }
                                }
                                check.x -= size;
                            }
                        }
                        if (target.x != 0xffff && target.y != 0xffff)
                        {
                            break;
                        }
                    }
                }
                if (target.x != 0xffff && target.y != 0xffff)
                {
                    std::cout << "Targeting: " << target.x << ", " << target.y << std::endl;
                    if (!moveCommand)
                    {
                        std::string msg;
                        msg.append("\0\0\0\x15\0\0\0\x01\x04", 9);
                        msg.append(ConnectionManager::makeAGNPuint(a->pid));
                        msg.append(ConnectionManager::makeAGNPushort(target.x));
                        msg.append("\x7f\0", 2);
                        msg.append(ConnectionManager::makeAGNPushort(target.y));
                        msg.append("\x7f\0", 2);
                        cmdIDs.push_back(ConnectionManager::RequestID::WALK);
                        cmdpids.push_back(a->pid);
                        std::uint64_t arg = (unsigned int)target.x<<48 + (unsigned int)target.y + 0x7f0000007f00;
                        std::cout << "move: " << arg << std::endl;
                        cmdargs.push_back(arg);
                        conn.send(msg.c_str(), msg.length());
                        reqs.push_back(ConnectionManager::RequestID::WALK);
                    }
                    if (!tinterCommand)
                    {
                        std::string msg;
                        msg.append("\0\0\0\x11\0\0\0\x01\x06", 9);
                        msg.append(ConnectionManager::makeAGNPuint(a->pid));
                        msg.append(ConnectionManager::makeAGNPushort(target.x));
                        msg.append(ConnectionManager::makeAGNPushort(target.y));
                        cmdIDs.push_back(ConnectionManager::RequestID::TINTERACT);
                        cmdpids.push_back(a->pid);
                        std::uint64_t arg = (unsigned int)target.x<<16 + (unsigned int)target.y;
                        std::cout << "tinter: " << arg << std::endl;
                        cmdargs.push_back(arg);
                        conn.send(msg.c_str(), msg.length());
                        reqs.push_back(ConnectionManager::RequestID::TINTERACT);
                    }
                }
            }
        }
    }

    conn.finish();
    map.cleanup();
    return 0;
}
