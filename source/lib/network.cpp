#include "network.hpp"
#include "map.hpp"


RoundSettings* RoundSettings::instance = nullptr;


RoundSettings::RoundSettings()
{
     if (instance == nullptr)
     {
         instance = this;
         isPlayer = true; // Reset this manually if you are the server. Don't do it otherwise.
         port = ANTNET_DEFAULT_PORT;
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


void ConnectionManager::step()
{
    switch (Round::instance->phase)
    {
        case Round::Phase::INIT:
            return;
        case Round::Phase::WAIT:
            if (Connection::listening())
            {
                std::vector<Connection*> newConnections = Connection::fetchConnections();
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
            break;
    }
    handleViewers();
}


void ConnectionManager::reset()
{
}


void ConnectionManager::handleViewers()
{
    switch (Round::instance->phase)
    {
        case Round::Phase::WAIT:{
            for (auto it = viewers.begin(); it != viewers.end(); it++)
            {
                Viewer* v = *it;
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
                    players.push_back(new Player(*v);
                    v->conn = nullptr; // So the destructor doesn't kill it
                    v->toClose = true;
                }
            }
            if (del)
            {
                viewers.erase_after(prev);
            }
            break;}
    }
    auto prev = viewers.before_begin();
    bool del = false;
    std::chrono::steady_clock::time_point timpont = std::chrono::steady_clock::now();
    for (auto it = viewers.begin(); it != viewers.end(); i++)
    {
        if (del)
        {
            viewers.erase_after(prev);
        }
        else
        {
            prev++;
        }
        del = false;
        Viewer* v = *it;
        if (!isValid(v) || std::chrono::duration<double>(timpont - v->timeAtLastMessage).count() >= 1.0) {del = true;}
    }
}


bool ConnectionManager::isValid(Viewer* v)
{
    return v != nullptr && !v->toClose && v->conn != nullptr && v->conn->errorState != CLOSED && v->conn->connected();
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
            unusedData = data;
            return true;
        }
        if (flag == 0)
        {
            sendResponse(v, "HTTP/1.1 400 Bad Request\r\nConnection: close\r\n", "HTTPFiles/err400.html");
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            unusedData = data;
            v->toClose = true;
            return false;
        }
        if (data.compare(data.find(' ') + 1, 12, "/favicon.ico") == 0)
        {
            sendReponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", flag == 2 ? "HTTPFiles/favicon.ico" : "");
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            unusedData = data;
        }
        else if (data.compare(data.find(' ') + 1, 2, "/ ") != 0)
        {
            sendResponse(v, "HTTP/1.1 301 Moved Permanently\r\nLocation: /\r\nConnection: keep-alive\r\n", "");
            data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
            unusedData = data;
        }
        else
        {
            switch (Round::instance->phase)
            {
                case Round::Phase::INIT: break;
                case Round::Phase::WAIT:
                    // TODO better
                    sendResponse(v, "HTTP/1.1 200 OK\r\nConnection: keep-alive\r\n", flag == 2 ? "HTTPFiles/waiting.html" : "");
                    v->timeAtLastMessage = std::chrono::steady_clock::now();
                    data.erase(data.begin(), data.begin() + data.find("\r\n\r\n") + 4);
                    unusedData = data;
                    break;
                case Round::Phase::RUNNING:
                case Round::Phase::DONE: break; // TODO
                case Round::Phase::CLOSED: break; // TODO
            }
        }
        return true;
    }
    return false;
}


bool sendResponse(Viewer* v, std::string header, std::string filename)
{
    if (!isValid(v))
    {
        return false;
    }
    if (!v->conn->send(header.data(), header.size()))
    {
        return !v->conn->connected();
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
        if (!v->conn->send(contentSize.data(), contentSize.size()))
        {
            return true;
        }
        f.seekg(0);
        for (char buf[1024]; f.eof(); )
        {
            f.read(buf, 1024);
            if (!v->conn->send(buf, f.gcount()) && !v->conn->connected())
            {
                break;
            }
        }
        f.close();
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
    std::string comp;
    comp.append("\0\0\0\x11\0\0\0\x01\0", 9);
    size_t p = data.find(comp);
    if (p == std::string::npos || data.length() - p < 8)
    {
        return false;
    }
    data.erase(0, p+17);
    v->unusedData = data;
    if (true /* TODO conditions */ )
    {
        v->send("\0\0\0\x0a\0\0\0\x01\0\0"); // OK
    }
    return true;
}
