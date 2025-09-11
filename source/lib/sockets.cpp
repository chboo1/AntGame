#include "sockets.hpp"
#include <iostream>


int Connection::port = ANTNET_DEFAULT_PORT;

#ifdef ANTNET_UNIX
int Connection::listenSockfd = -1;


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
    int yesiwantyoutoreuseaddr = 1;
    if (setsockopt(listenSockfd, SOL_SOCKET, SO_REUSEADDR, &yesiwantyoutoreuseaddr, sizeof(int)) < 0)
    {
        std::cerr << "Failed to allow socket to reuse address! This may cause you to be unable to use the same port for a few minutes after the program ends. Proceeding anyways. Errno: " << errno << std::endl;
    }
    struct sockaddr_in peerAddress;
    peerAddress.sin_family = AF_INET;
    peerAddress.sin_port = htons(nport);
    peerAddress.sin_addr.s_addr = INADDR_ANY;
    if (bind(listenSockfd, (struct sockaddr*)&peerAddress, (socklen_t)sizeof(peerAddress)) < 0)
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
        fcntl(newfd, F_SETFL, fl);
        Connection newConn(newfd);
        list->push_back(newConn);
        acceptedc++;
    }
    fcntl(listenSockfd, F_SETFL, fl);
    return acceptedc;
}


bool Connection::connectTo(std::string nip, int nport)
{
     if (sockfd >= 0)
     {
         std::cerr << "Already connected with this connection!" << std::endl;
         return false;
     }
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
    remote.sin_addr.s_addr = inet_addr(nip.c_str());
    if (connect(sockfd, (struct sockaddr*)&remote, (socklen_t)sizeof(remote)) < 0)
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


bool Connection::send(char* message, size_t len)
{
    if (sockfd < 0)
    {
        std::cerr << "Cannot send because this connection is not open to anything!" << std::endl;
        return false;
    }
    int ret;
    if ((ret = write(sockfd, message, (int)len)) < len)
    {
        if (ret < 0)
        {
            std::cerr << "Error while sending data!" << std::endl;
            return false;
        }
        std::cerr << "Only sent partial data." << std::endl;
        return false;
    }
    return true;
}


int Connection::receive(char* buf, size_t len)
{
    if (sockfd < 0)
    {
        std::cerr << "Cannot receive data because this connection is not connected!" << std::endl;
        return -1;
    }
    int ret = read(sockfd, buf, (int)len);
    if (ret < 0)
    {
        std::cerr << "Failed while reading! Errno: " << errno << std::endl;
        return -1;
    }
    return ret;
}


std::string Connection::readall()
{
    if (sockfd < 0)
    {
        std::cerr << "Cannot receive data because this connection is not connected!" << std::endl;
        return "";
    }
    int fl = fcntl(sockfd, F_GETFL);
    if (fl == -1)
    {
        std::cerr << sockfd << std::endl;
        std::cerr << "Failed to get file flags before receiving data! Errno: " << errno << std::endl;
        return "";
    }
    if (fcntl(sockfd, F_SETFL, fl | O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to set file flags before receiving data! Errno: " << errno << std::endl;
        return "";
    }
    std::string ret = "";
    char buf[512];
    int sizeRead = read(sockfd, buf, 512);
    while (true)
    {
        if (sizeRead < 0) // Either an error or EAGAIN/EWOULDBLOCK, meaning nothing is left.
        {
            #ifdef EAGAIN
            if (errno == EAGAIN)
            {
                break;
            }
            #endif
            #ifdef EWOULDBLOCK
            if (errno == EWOULDBLOCK)
            {
                break;
            }
            #endif
            std::cerr << "Error while reading! Errno: " << errno << std::endl;
            fcntl(sockfd, F_SETFL, fl);
            return "";
        }
        ret.append(buf, sizeRead);
        if (sizeRead < 512) // End of transmission!
        {
            break;
        }
    }
    fcntl(sockfd, F_SETFL, fl);
    return ret;
}


void Connection::finish()
{
    if (sockfd >= 0)
    {
        close(sockfd);
    }
    sockfd = -1;
}


#elif defined(ANTNET_WIN)
bool Connection::listenOpen = false;
bool Connection::started = false;
int Connection::instances = 0;
#else
#error sockets.cpp: No ANT OS macros defined! Has sockets.hpp been modified?
#endif
