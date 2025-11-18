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
    if (!conn.send("\0\0\0\x09\0\0\0\x01\x00", 9))
    {
        std::cerr << "Failed to send join request over..." << std::endl;
        conn.finish();
        return 1;
    }
    std::string ideal;
    ideal.append("\0\0\0\x0a\0\0\0\x01\x00", 9);
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
    std::cout << "Successfully connected to server." << std::endl;
    ideal.clear();
    ideal.append(ConnectionManager::makeAGNPuint(name.length() + 12));
    ideal.append(ConnectionManager::makeAGNPuint(1));
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
                responses.erase(0);
                rc--;
                switch (responses[0])
                {
                    case '\x02': // PING
                        conn.send("\0\0\0\x09\0\0\0\x01\x01", 9);
                        std::cout << "Pinged..." << std::endl;
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
                responses.erase(0);
            }
        }
    }
    conn.readall();
    if (conn.connected())
    {
        std::cout << "Server started game!" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        conn.send("\0\0\0\x09\0\0\0\x01\x02", 9);
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
    else
    {
        std::cerr << "Lost connection to server while waiting for start of game!" << std::endl;
        return 1;
    }

    conn.finish();
    return 0;
}
