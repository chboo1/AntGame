#include "sockets.hpp"


int Connection::port = ANTNET_DEFAULT_PORT;

#ifdef ANTNET_UNIX
int Connection::listenSockfd = -1;
struct sockaddr_in Connection::peerAddress;


Connection::Connection(){}
Connection::~Connection()
{
    finish();
}
Connection::Connection(int fd)
{
    sockfd = fd;
}


bool Connection::openListen(int nport)
{
    listenSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSockfd < 0)
    {
        std::cerr << "Failed to open the listening socket! Errno: " << errno << std::endl;
        listenSockfd = -1;
        return false;
    }
    struct sockaddr_in peerAddress;
    peerAddress.sin_family = AF_INET;
    peerAddress.sin_port = htons(nport);
    peerAddress.sin_addr.s_addr = INADDR_ANY;
    if (bind(listenSockfd, &peerAddress, (socklen_t)sizeof(peerAddress)) < 0)
    {
        std::cerr << "Failed to bind the listening socket! Errno: " << errno << std::endl;
        close(listenSockfd);
        listenSockfd = -1;
        return false;
    }
    if (listen(listenSockfd, ANTNET_LISTEN_BACKLOG) < 0)
    {
        std::cerr << "Failed to listen on the listening socket! Errno: " << errno << std::endl;
        close(listenSockfd);
        listenSockfd = -1;
        return false;
    }
    port = nport;
    flags |= ANTNET_CONNFLAG_INITIATED;
    return true;
}


void Connection::closeListen()
{
    if (listenSockfd >= 0)
    {
        close(listenSockfd);
        listenSockfd = -1;
    }
    port = ANTNET_DEFAULT_PORT;
}


bool Connection::listening()
{
    return listenSockfd >= 0;
}


int Connection::fetchConnections(std::vector<Connection>* list)
{
    if (listenSockfd < 0)
    {
        openListen(port);
    }
    int fl = fcntl(listenSockfd, F_GETFL);
    if (fl == -1)
    {
        return 0; // Can't set to nonblock
    }
    if (fcntl(listenSockfd, F_SETFL, fl | O_NONBLOCK) == -1)
    {
        return 0; // Can't set to nonblock
    }
    struct sockaddr addr;
    socklen_t addrlen = (socklen_t) sizeof(addr);
    int newfd = -1;
    int acceptedc = 0;
    while ((newfd = accept(listenSockfd, &addr, &addrlen)) >= 0 || errno == ECONNABORTED)
    {
        if (newfd < 0)
        {
            continue;
        }
        Connection newConn(newfd);
        list->push_back(newConn);
        acceptedc++;
    }
    fcntl(listenSockfd, F_SETFL, fl);
    return acceptedc;
}


bool Connection::connectTo(std::string nip, int nport)
{
#ifndef CLIENT
     if (Connection::listening())
     {
         std::cerr << "Already listening on this client! Can't connect!" << std::endl;
         return false;
     }
#endif
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        std::cerr << "Failed to create socket! Errno: " << errno << std::endl;
        sockfd = -1;
        return false;
    }
    struct sockaddr_in remote;
    remote.sin_port = htons(nport);
    remote.sin_family = AF_INET;
    remote.sin_addr = inet_addr(nip.c_str());
    if (connect(sockfd, &remote, (socklen_t)sizeof(remote)) < 0)
    {
        std::cerr << "Failed to connect! Errno: " << errno << std::endl;
        close(sockfd);
        sockfd = -1;
        return false;
    }
    port = nport;
    return true;
}


bool Connection::connected()
{
    return sockfd >= 0;
}


void Connection::finish()
{
    if (sockfd >= 0)
    {
        close(sockfd);
    }
}

#elif defined(ANTNET_WIN)
bool Connection::listening = false;
bool Connection::started = false;
int Connection::instances = 0;
#else
#error sockets.cpp: No ANT OS macros defined! Has sockets.hpp been modified?
#endif
