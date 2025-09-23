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

#ifndef CLIENT
bool Connection::openListen(int nport)
{
    closeListen();
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


int Connection::fetchConnections(std::vector<Connection*>* list)
{
    if (listenSockfd < 0)
    {
        if (!openListen(port))
        {
            std::cerr << "Failed to open the listening socket!" << std::endl;
            return -1;
        }
    }
    int fl = fcntl(listenSockfd, F_GETFL);
    if (fl == -1)
    {
        std::cerr << "Failed to get flags of listening socket to set it to nonblock mode" << std::endl;
        return -1; // Can't set to nonblock
    }
    if (fcntl(listenSockfd, F_SETFL, fl | O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to set flags of listening socket to set it to nonblock mode" << std::endl;
        return -1; // Can't set to nonblock
    }
    int newfd = -1;
    int acceptedc = 0;
    while ((newfd = accept(listenSockfd, nullptr, nullptr)) >= 0 || errno == ECONNABORTED)
    {
        if (newfd < 0)
        {
            continue;
        }
        fcntl(newfd, F_SETFL, fl); 
        Connection* newConn = new Connection(newfd);
        list->push_back(newConn);
        acceptedc++;
    }
    if (newfd < 0)
    {
        if (errno != EWOULDBLOCK)
        {
        }
        std::cerr << "Failed to accept connections! Errno: " << errno << std::endl;
    }
    fcntl(listenSockfd, F_SETFL, fl);
    return acceptedc;
}
#endif // ifndef CLIENT


bool Connection::connectTo(std::string nip, int nport)
{
    finish();
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
    errno = 0;
    if (connect(sockfd, (struct sockaddr*)&remote, (socklen_t)sizeof(remote)) < 0)
    {
        std::cerr << "Failed to connect! Errno: " << errno << std::endl;
        close(sockfd);
        sockfd = -1;
        return false;
    }
    std::cout << errno << std::endl;
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
    errno = 0;
    int fl = fcntl(sockfd, F_GETFL);
    if (fl == -1)
    {
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
    errno = 0;
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
        sizeRead = read(sockfd, buf, 512);
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


#elif defined(ANTNET_WIN) // ifdef ANTNET_UNIX
bool Connection::started = false;
int Connection::instances = 0;
WSADATA Connection::wsadata;
SOCKET Connection::listenSock = INVALID_SOCKET;



Connection::Connection()
{
    instances++;
}


Connection::~Connection()
{
    finish();
    instances--;
    if (started && instances == 0 && !listening())
    {
        WSACleanup();
    }
}


Connection::Connection(SOCKET nsock)
{
    instances++;
    sock = nsock;
}


#ifndef CLIENT
bool Connection::openListen(int port)
{
    closeListen();
    if (!started)
    {
        int ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
        if (ret != 0)
        {
            std::cerr << "Failed to initialize winsock! Error code: " << ret << std::endl;
            return false;
        }
        started = true;
    }
    struct addrinfo hints;
    struct addrinfo* address = nullptr;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;
    int ret = getaddrinfo(nullptr, std::to_string(port), &hints, &address);
    if (ret != 0)
    {
        std::cerr << "Failed to get address for listen socket! Error code: " << ret << std::endl;
        if (instances <= 0)
        {
            WSACleanup();
            started = false;
        }
        return false;
    }
    listenSock = socket(address->ai_family, address->ai_socktype, address_ai_protocol);
    if (listenSock == INVALID_SOCKET)
    {
        std::cerr << "Failed to create listen socket! Error code: " << WSAGetLastError() << std::endl;
        freeaddrinfo(address);
        if (instances <= 0)
        {
            WSACleanup();
            started = false;
        }
        return false;
    }
    if (bind(listenSock, address->ai_addr, (int)address->ai_addrlen) == SOCKET_ERROR)
    {
        std::cerr << "Failed to bind listen socket to port! Error code: " << WSAGetLastError() << std::endl;
        freeaddrinfo(address);
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
        if (instances <= 0)
        {
            WSACleanup();
            started = false;
        }
        return false;
    }
    freeaddrinfo(address);
    if (listen(listenSock, ANTNET_LISTEN_BACKLOG) == SOCKET_ERROR)
    {
        std::cerr << "Failed to listen on socket! Error code: " << WSAGetLastError() << std::endl;
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
        if (instances <= 0)
        {
            WSACleanup();
            started = false;
        }
        return false;
    }
}


void Connection::closeListen()
{
    if (listenSock != INVALID_SOCKET)
    {
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
    }
    if (instances == 0)
    {
        WSACleanup();
        started = false;
    }
}



bool Connection::listening()
{
    if (listenSock != INVALID_SOCKET)
    {
        return true;
    }
    return false;
}


int Connection::fetchConnections(std::vector<Connection*>* list)
{
    if (listenSock == INVALID_SOCKET)
    {
        if (!openListen(port))
        {
            std::cerr << "Failed to open the listening socket!" << std::endl;
            return -1;
        }
    }
    unsigned long yesiwantyoutousenonblock = 1;
    if (ioctlsocket(listenSock, FIONBIO, &yesiwantyoutousenonblock) != 0)
    {
        std::cerr << "Failed to set listening socket to nonblocking mode before attempting to accept connections!" << std::endl;
        return -1;
    }
    yesiwantyoutousenonblock = 0; // No more. Goodbye.
    SOCKET newsock = INVALID_SOCKET;
    int acceptedc = 0;
    while ( (newsock = accept(listenSock, nullptr, nullptr)) != INVALID_SOCKET || WSAGetLastError() == WSAECONNRESET) // We ignore ECONNRESET because this does not indicate listenSock is in a failstate
    {
        if (newsock == INVALID_SOCKET)
        {
            continue;
        }
        ioctlsocket(newsock, FIONBIO, &yesiwantyoutousenonblock);
        Connection* newConn = new Connection(newsock);
        list->push_back(newConn);
        acceptedc++;
    }
    if (newsock == INVALID_SOCKET)
    {
        int err = WSAGetLastError();
        if (err != WSAEWOULDBLOCK)
        {
            std::cerr << "Failed to accept any connections using accept! Errno: " << err << std::endl;
            return -1;
        }
    }
    ioctlsocket(listenSock, FIONBIO, &yesiwantyoutousenonblock);
    return acceptedc;
}
#endif // ifndef CLIENT


bool Connection::connected()
{
    return sock != INVALID_SOCKET;
}


void Connection::finish()
{
    if (sock != INVALID_SOCKET)
    {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}


bool Connection::connectTo(std::string nhost, int nport)
{
    finish();
    if (!started)
    {
        int ret = WSAStartup(MAKEWORD(2,2), &wsadata);
        if (ret != 0)
        {
            std::cerr << "Failed to initialize winsock! Errno: " << ret << std::endl;
            return false;
        }
        started = true;
    }
    struct addrinfo *result;
    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    int ret = getaddrinfo(nhost.c_str(), std::to_string(nport), &hints, &result);
    if (ret != 0)
    {
        std::cerr << "Failed to get address information of host `" << nhost << "' on port " << nport << ". Make sure the server is running and is at the specified address! Errno: " << ret << std::endl;
        return false;
    }
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Failed to create socket! Errno: " << WSAGetLastError() << std::endl;
        freeaddrinfo(result);
        return false;
    }
    bool success = false;
    for (struct addrinfo*temp = result; temp != nullptr; temp = temp->ai_next)
    {
        if (temp == nullptr)
        {
            break;
        }
        if (connect(sock, temp->ai_addr, (int)temp->ai_addrlen) == SOCKET_ERROR)
        {
            continue;
        }
        // Connected successfully
        success = true;
    }
    freeaddrinfo(result);
    if (!success)
    {
        std::cerr << "Failed to connect to the specified address. This could be because the address is wrong or because the server is not running. Errno: " << WSAGetLastError() << std::endl;
        closesocket(sock);
        return false;
    }
    return true;
}


bool Connection::send(char* buf, size_t size)
{
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Cannot send data with a connection that isn't connected yet!" << std::endl;
        return false;
    }
    if (send(sock, buf, (int)size, 0) == SOCKET_ERROR)
    {
        std::cerr << "Failed to send data to peer! Errno: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}


bool Connection::receive(char* buf, size_t size)
{
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Cannot receive data from a connection that isn't connected yet!" << std::endl;
        return -1;
    }
    if ((int r = recv(sock, buf, (int)size, 0)) == SOCKET_ERROR)
    {
        std::cerr << "Failed to receive data from peer! Errno: " << WSAGetLastError() << std::endl;
        return -1;
    }
    return r;
}


std::string Connection::readall()
{
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Cannot read data from a connection that isn't connected yet!" << std::endl;
        return "";
    }
    unsigned long yesiwantyoutousenonblock = 1;
    if (ioctlsocket(sock, FIONBIO, &yesiwantyoutousenonblock) != 0)
    {
        std::cerr << "Failed to set socket to nonblocking mode before attempting to read data!" << std::endl;
        return -1;
    }
    std::string ret = "";
    char buf[512];
    int sizeRead = recv(sock, buf, 512, 0);
    while (true)
    {
        if (sizeRead == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err = WSAEWOULDBLOCK)
            {
                break;
            }
            std::cerr << "Error while reading! Errno: " << errno << std::endl;
            yesiwantyoutousenonblock = 0; // No more. Goodbye.
            ioctlsocket(sock, FIONBIO, yesiwantyoutousenonblock);
            return "";
        }
        ret.append(buf, sizeRead);
        if (sizeRead < 512)
        {
            break;
        }
        sizeRead = recv(sock, buf, 512, 0);
    }
    yesiwantyoutousenonblock = 0; // No more. Goodbye.
    ioctlsocket(sock, FIONBIO, yesiwantyoutousenonblock);
    return ret;
}


#else // elif defined(ANTNET_WIN)
#error sockets.cpp: No ANT OS macros defined! Has sockets.hpp been modified?
#endif // else
