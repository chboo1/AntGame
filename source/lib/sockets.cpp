#include "sockets.hpp"
#include <iostream>


int Connection::port = ANTNET_DEFAULT_PORT;
Connection::ErrState Connection::serrorState = Connection::ErrState::OK;

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
    serrorState = OK;
    closeListen();
    listenSockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSockfd < 0)
    {
        std::cerr << "Failed to open the listening socket! Errno: " << errno << std::endl;
        serrorState = SYS;
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
        switch (errno)
        {
            case EACCES:
            case EAFNOSUPPORT:
            case EBADF:
            case EDESTADDRREQ:
            case EFAULT:
            case EINVAL:
            case ENOTSOCK:
            case EOPNOTSUPP:
            case EADDRNOTAVAIL:
                serrorState = SYS;
                break;
            case EADDRINUSE:
                serrorState = ADDR;
                break;
            default:
                serrorState = UNKNOWN;
                break;
        }
        std::cerr << "Failed to bind the listening socket! Errno: " << errno << std::endl;
        close(listenSockfd);
        listenSockfd = -1;
        return false;
    }
    if (listen(listenSockfd, ANTNET_LISTEN_BACKLOG) < 0)
    {
        serrorState = SYS;
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
    serrorState = OK;
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
    serrorState = OK;
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
        serrorState = SYS;
        closeListen(); // The only possible errno here is EBADF which means listenSockfd is not a socket
        return -1; // Can't set to nonblock
    }
    if (fcntl(listenSockfd, F_SETFL, fl | O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to set flags of listening socket to set it to nonblock mode" << std::endl;
        serrorState = SYS;
        closeListen(); // The only possible errno here is EBADF which means listenSockfd is not a socket
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
        if (list)
        {
            fcntl(newfd, F_SETFL, fl); 
            Connection* newConn = new Connection(newfd);
            list->push_back(newConn);
        }
        else
        {
            close(newfd);
        }
        acceptedc++;
    }
    if (newfd < 0) // There was an error or there aren't any more sockets to read
    {
        if (errno != EWOULDBLOCK && errno != ECONNABORTED)
        {
            switch (errno)
            {
                case EBADF:
                case EINVAL:
                case ENOTSOCK:
                    closeListen(); // These imply we don't have a valid sock
                
                case EFAULT:
                case EMFILE:
                case ENFILE:
                case ENOMEM:
                    serrorState = SYS;
                    break;
                case EINTR:
                default:
                    serrorState = UNKNOWN;
                    break;
            }
            std::cerr << "Failed to accept connections! Errno: " << errno << std::endl;
        }
    }
    fcntl(listenSockfd, F_SETFL, fl);
    return acceptedc;
}
#endif // ifndef CLIENT


bool Connection::connectTo(std::string nip, int nport)
{
    errorState = OK;
    finish();
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        errorState = SYS;
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
        switch (errno)
        {
            case EACCES:
            case EAFNOSUPPORT:
            case EALREADY:
            case EBADF:
            case EFAULT:
            case EINTR:
            case EINVAL:
            case EISCONN:
            case ENOBUFS:
            case ENOTSOCK:
            case EOPNOTSUPP:
                errorState = SYS;
                break;

            case EADDRINUSE:
            case EADDRNOTAVAIL:
            case ECONNRESET:
                errorState = RETRY;
                break;

            case ECONNREFUSED:
            case EHOSTUNREACH:
            case EPROTOTYPE:
                errorState = ADDR;
                break;

            case ENETDOWN:
            case ENETUNREACH:
            case ETIMEDOUT:
                errorState = NET;
                break;

            default:
                errorState = UNKNOWN;
                break;
        }
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
    return sockfd >= 0 && sockfd <= RLIMIT_NOFILE;
}


bool Connection::send(const char* message, size_t len)
{
    errorState = OK;
    if (!connected())
    {
        std::cerr << "Cannot send because this connection is not open to anything!" << std::endl;
        errorState = EARLY;
        return false;
    }
    if (len <= 0)
    {
        return true;
    }
    int ret;
    if ((ret = write(sockfd, message, (int)len)) < len)
    {
        if (ret < 0)
        {
            switch (errno)
            {
                case EFAULT:
                    errorState = FAULT;
                    break;
                case EFBIG:
                case EINTR:
                case EIO:
                case ENXIO: // How
                    errorState = SYS;
                    break;
                case EINVAL:
                case EBADF:
                    errorState = SYS;
                    finish();
                    break;
#ifdef EAGAIN
                case EAGAIN:
#endif
#ifdef EWOULDBLOCK
#if !defined(EAGAIN) || EAGAIN != EWOULDBLOCK
                case EWOULDBLOCK:
#endif
#endif
                    errorState = RETRY;
                    break;
                case ECONNRESET:
                case EPIPE:
                    errorState = CLOSED;
                    finish();
                    break;
                case ENETDOWN:
                case ENETUNREACH:
                    errorState = NET;
                    break;
                default:
                    errorState = UNKNOWN;
                    break;
            }
            std::cerr << "Error while sending data!" << std::endl;
            return false;
        }
        std::cerr << "Only sent partial data." << std::endl;
        return false;
    }
    return true;
}


int Connection::receive(char* buf, size_t size)
{
    if (!connected())
    {
        std::cerr << "Cannot receive data because this connection is not connected!" << std::endl;
        errorState = EARLY;
        return -1;
    }
    int ret = read(sockfd, buf, (int)size);
    if (ret < 0)
    {
            switch (errno)
            {
                case EFAULT:
                    errorState = FAULT;
                    break;
                case EINTR:
                case EIO:
                case ENXIO: // How
                case ENOMEM:
                    errorState = SYS;
                    break;
                case EINVAL:
                case EBADF:
                    errorState = SYS;
                    finish();
                    break;
#ifdef EAGAIN
                case EAGAIN:
#endif
#ifdef EWOULDBLOCK
#if !defined(EAGAIN) || EAGAIN != EWOULDBLOCK
                case EWOULDBLOCK:
#endif
#endif
                case ETIMEDOUT:
                    errorState = RETRY;
                    break;
                case ECONNRESET:
                case ENOTCONN:
                    errorState = CLOSED;
                    finish();
                    break;
                case ENETDOWN:
                case ENETUNREACH:
                    errorState = NET;
                    break;
                default:
                    errorState = UNKNOWN;
                    break;
            }
        std::cerr << "Failed while reading! Errno: " << errno << std::endl;
        return -1;
    }
    return ret;
}


std::string Connection::readall()
{
    errorState = OK;
    if (!connected())
    {
        std::cerr << "Cannot receive data because this connection is not connected!" << std::endl;
        errorState = EARLY;
        return "";
    }
    errno = 0;
    int fl = fcntl(sockfd, F_GETFL);
    if (fl == -1)
    {
        std::cerr << "Failed to get file flags before receiving data! Errno: " << errno << std::endl;
        errorState = SYS;
        finish();
        return "";
    }
    if (fcntl(sockfd, F_SETFL, fl | O_NONBLOCK) == -1)
    {
        std::cerr << "Failed to set file flags before receiving data! Errno: " << errno << std::endl;
        errorState = SYS;
        finish();
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
            switch (errno)
            {
                case EFAULT:
                case EINTR:
                case EIO:
                case ENXIO: // How
                case ENOMEM:
                    errorState = SYS;
                    break;
                case EINVAL:
                case EBADF:
                    errorState = SYS;
                    finish();
                    break;
                case ECONNRESET:
                case ENOTCONN:
                    errorState = CLOSED;
                    finish();
                    break;
                case ENETDOWN:
                case ENETUNREACH:
                case ETIMEDOUT:
                    errorState = NET;
                    break;
                default:
                    errorState = UNKNOWN;
                    break;
            }
            if (errno != ECONNRESET)
            {
                std::cerr << "Error while reading! Errno: " << errno << std::endl;
            }
            if (sockfd >= 0)
            {
                fcntl(sockfd, F_SETFL, fl);
            }
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
    errorState = OK;
    if (sockfd >= 0)
    {
        close(sockfd);
    }
    sockfd = -1;
}


#elif defined(ANTNET_WIN) // ifdef ANTNET_UNIX
bool Connection::started = false;
unsigned int Connection::instances = 0;
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
        started = false;
    }
}


Connection::Connection(SOCKET nsock)
{
    instances++;
    sock = nsock;
}


#ifndef CLIENT
bool Connection::openListen(int nport)
{
    serrorState = OK;
    closeListen();
    if (!started)
    {
        int ret = WSAStartup(MAKEWORD(2, 2), &wsadata);
        if (ret != 0)
        {
            serrorState = SYS;
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
    int ret = getaddrinfo(nullptr, std::to_string(nport).c_str(), &hints, &address);
    if (ret != 0)
    {
        switch (ret)
        {
            case WSAEINVAL:
            case WSANO_RECOVERY:
            case WSAEAFNOSUPPORT:
            case WSA_NOT_ENOUGH_MEMORY:
            case WSAESOCKTNOSUPPORT:
                serrorState = SYS;
                break;
            case WSATRY_AGAIN:
                serrorState = RETRY;
                break;
            case WSAHOST_NOT_FOUND:
            case WSATYPE_NOT_FOUND:
                serrorState = ADDR;
                break;
            default:
                serrorState = UNKNOWN;
                break;
        }
        std::cerr << "Failed to get address for listen socket! Error code: " << ret << std::endl;
        if (instances <= 0)
        {
            WSACleanup();
            started = false;
        }
        return false;
    }
    listenSock = socket(address->ai_family, address->ai_socktype, address->ai_protocol);
    if (listenSock == INVALID_SOCKET)
    {
        int e = WSAGetLastError();
        switch (e)
        {
            case WSANOTINITIALISED:
                serrorState = RETRY;
                started = false; // This error was likely caused by a false started tag
                break;
            case WSAENETDOWN:
                serrorState = NET;
                break;
            case WSAEAFNOSUPPORT:
            case WSAEINPROGRESS: // Shouldn't happen considering the version we're in
            case WSAEMFILE:
            case WSAEINVAL:
            case WSAEINVALIDPROVIDER:
            case WSAEINVALIDPROCTABLE:
            case WSAENOBUFS:
            case WSAEPROTONOSUPPORT:
            case WSAEPROTOTYPE:
            case WSAEPROVIDERFAILEDINIT:
            case WSAESOCKTNOSUPPORT:
                serrorState = SYS;
                break;
            default:
                serrorState = UNKNOWN;
                break;
        }
        std::cerr << "Failed to create listen socket! Error code: " << e << std::endl;
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
        int e = WSAGetLastError();
        std::cerr << "Failed to bind listen socket to port! Error code: " << e << std::endl;
        switch (e)
        {
            case WSANOTINITIALISED:
                serrorState = RETRY;
                started = false;
                break;
            case WSAENETDOWN:
                serrorState = NET;
                break;
            case WSAEACCES:
            case WSAEADDRINUSE:
            case WSAEADDRNOTAVAIL:
                serrorState = ADDR;
                break;
            case WSAEFAULT:
            case WSAEINPROGRESS:
            case WSAEINVAL:
            case WSAENOBUFS:
            case WSAENOTSOCK: // Dunno how this would happen
                serrorState = SYS;
                break;
            default:
                serrorState = UNKNOWN;
                break;
        }
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
        int e = WSAGetLastError();
        std::cerr << "Failed to listen on socket! Error code: " << e << std::endl;
        switch (e)
        {
            case WSANOTINITIALISED:
                serrorState = RETRY;
                started = false;
                break;
            case WSAENETDOWN:
                serrorState = NET;
                break;
            case WSAEADDRINUSE: // Is essentially thrown by bind but is encountered on listen because it is when wildcard addresses get crystallized
                serrorState = ADDR;
                break;
            case WSAEINPROGRESS:
            case WSAEINVAL: // How
            case WSAEISCONN: // How
            case WSAEMFILE:
            case WSAENOBUFS:
            case WSAENOTSOCK: // How
            case EOPNOTSUPP:
                serrorState = SYS;
                break;
            default:
                serrorState = UNKNOWN;
                break;
        }
        closesocket(listenSock);
        listenSock = INVALID_SOCKET;
        if (instances <= 0)
        {
            WSACleanup();
            started = false;
        }
        return false;
    }
    port = nport;
    return true;
}


void Connection::closeListen()
{
    serrorState = OK;
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
    serrorState = OK;
    if (listenSock == INVALID_SOCKET || !started)
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
        int e = WSAGetLastError();
        std::cerr << "Failed to set listening socket to nonblocking mode before attempting to accept connections! Error: " << e << std::endl;
        serrorState = SYS;
        return -1;
    }
    yesiwantyoutousenonblock = 0; // No more. Goodbye.
    SOCKET newsock = INVALID_SOCKET;
    int acceptedc = 0;
    int err = 0;
    while ( (newsock = accept(listenSock, nullptr, nullptr)) != INVALID_SOCKET || (err = WSAGetLastError()) == WSAECONNRESET) // We ignore ECONNRESET because this does not indicate listenSock is in a failstate
    {
        if (newsock == INVALID_SOCKET)
        {
            continue;
        }
        if (list)
        {
            ioctlsocket(newsock, FIONBIO, &yesiwantyoutousenonblock);
            Connection* newConn = new Connection(newsock);
            list->push_back(newConn);
        }
        else
        {
            closesocket(newsock);
        }
        acceptedc++;
    }
    if (newsock == INVALID_SOCKET)
    {
        if (err != WSAEWOULDBLOCK && err != WSAECONNRESET)
        {
            switch (err)
            {
                case WSANOTINITIALISED:
                    started = false;
                case WSAEINVAL:
                case WSAENOTSOCK:
                    closeListen();
                case WSAEFAULT:
                case WSAEINTR:
                case WSAEINPROGRESS:
                case WSAEMFILE:
                case WSAENOBUFS:
                case WSAEOPNOTSUPP:
                    serrorState = SYS;
                    break;
                case WSAENETDOWN:
                    serrorState = NET;
                    break;
                default:
                    serrorState = UNKNOWN;
                    break;
            }
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
    errorState = OK;
    if (sock != INVALID_SOCKET)
    {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}


bool Connection::connectTo(std::string nip, int nport)
{
    finish();
    errorState = OK;
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
    int ret = getaddrinfo(nip.c_str(), std::to_string(nport).c_str(), &hints, &result);
    if (ret != 0)
    {
        switch (ret)
        {
            case WSAEINVAL:
            case WSANO_RECOVERY:
            case WSAEAFNOSUPPORT:
            case WSA_NOT_ENOUGH_MEMORY:
            case WSAESOCKTNOSUPPORT:
                errorState = SYS;
                break;
            case WSATRY_AGAIN:
                errorState = RETRY;
                break;
            case WSAHOST_NOT_FOUND:
            case WSATYPE_NOT_FOUND:
                errorState = ADDR;
                break;
            default:
                errorState = UNKNOWN;
                break;
        }
        std::cerr << "Failed to get address information of host `" << nip << "' on port " << nport << ". Make sure the server is running and is at the specified address! Errno: " << ret << std::endl;
        return false;
    }
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET)
    {
        int e = WSAGetLastError();
        switch (e)
        {
            case WSANOTINITIALISED:
                errorState = RETRY;
                started = false; // This error was likely caused by a false started tag
                break;
            case WSAENETDOWN:
                errorState = NET;
                break;
            case WSAEAFNOSUPPORT:
            case WSAEINPROGRESS: // Shouldn't happen considering the version we're in
            case WSAEMFILE:
            case WSAEINVAL:
            case WSAEINVALIDPROVIDER:
            case WSAEINVALIDPROCTABLE:
            case WSAENOBUFS:
            case WSAEPROTONOSUPPORT:
            case WSAEPROTOTYPE:
            case WSAEPROVIDERFAILEDINIT:
            case WSAESOCKTNOSUPPORT:
                errorState = SYS;
                break;
            default:
                errorState = UNKNOWN;
                break;
        }
        std::cerr << "Failed to create socket! Errno: " << e << std::endl;
        freeaddrinfo(result);
        return false;
    }
    bool success = false;
    errorState = OK;
    for (struct addrinfo*temp = result; temp != nullptr; temp = temp->ai_next)
    {
        if (temp == nullptr)
        {
            break;
        }
        if (connect(sock, temp->ai_addr, (int)temp->ai_addrlen) == SOCKET_ERROR)
        {
            int e = WSAGetLastError();
            bool done = false;
            switch(e)
            {
                case WSANOTINITIALISED:
                    done = true;
                    errorState = RETRY;
                    started = false;
                    break;
                case WSAENETDOWN:
                case WSAENETUNREACH:
                    done = true;
                    errorState = NET;
                    break;
                case WSAEINVAL:
                case WSAENOTSOCK:
                    done = true;
                case WSAEADDRINUSE: // Is not flagged as ADDR because that would imply the remote address is invalid, when it isn't.
                case WSAEINTR:
                case WSAEINPROGRESS:
                case WSAEALREADY:
                case WSAEAFNOSUPPORT:
                case WSAEFAULT:
                case WSAENOBUFS:
                    errorState = errorState != ADDR ? SYS : ADDR; // If there is an address error already we want to show that to the client more than we do this random sys error.
                    break;
                case WSAEADDRNOTAVAIL:
                case WSAECONNREFUSED:
                case WSAEHOSTUNREACH:
                case WSAETIMEDOUT:
                    errorState = ADDR;
                    break;
                case WSAEISCONN:
                    done = true;
                    errorState = OK;
                    break; // We're already connected so just leave
                case WSAEWOULDBLOCK:
                case WSAEACCES:
                default:
                    errorState = UNKNOWN;
                    break;
            }
            if (done) {break;}
            continue;
        }
        // Connected successfully
        success = true;
        errorState = OK;
        break;
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


bool Connection::send(const char* message, size_t len)
{
    errorState = OK;
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Cannot send data with a connection that isn't connected yet!" << std::endl;
        errorState = EARLY;
        return false;
    }
    if (size <= 0)
    {
        return true;
    }
    if (::send(sock, message, (int)len, 0) == SOCKET_ERROR)
    {
        int e = WSAGetLastError();
        switch (e)
        {
            case WSAEFAULT:
                errorState = FAULT;
                break;
            case WSANOTINITIALISED:
                started = false;
            case WSAENOTCONN:
            case WSAENOTSOCK:
            case WSAEINVAL:
                finish();
            case WSAEINTR:
            case WSAEINPROGRESS:
            case WSAENOBUFS:
            case WSAEOPNOTSUPP:
                errorState = SYS;
                break;
            case WSAENETDOWN:
                errorState = NET;
                break;
            case WSAENETRESET:
            case WSAEHOSTUNREACH:
            case WSAECONNABORTED:
            case WSAECONNRESET:
            case WSAETIMEDOUT:
                finish();
            case WSAESHUTDOWN:
                errorState = CLOSED;
                break;
            case WSAEACCES:
            case WSAEWOULDBLOCK:
            case WSAEMSGSIZE:
            default:
                errorState = UNKNOWN;
                break;
        }
        std::cerr << "Failed to send data to peer! Errno: " << e << std::endl;
        return false;
    }
    return true;
}


int Connection::receive(char* buf, size_t size)
{
    errorState = OK;
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Cannot receive data from a connection that isn't connected yet!" << std::endl;
        errorState = EARLY;
        return -1;
    }
    int r;
    if ((r = recv(sock, buf, (int)size, 0)) == SOCKET_ERROR)
    {
        int e = WSAGetLastError();
        switch (e)
        {
            case WSAEFAULT:
                errorState = FAULT;
                break;
            case WSANOTINITIALISED:
                started = false;
            case WSAENOTCONN:
            case WSAENOTSOCK:
            case WSAEINVAL:
                finish();
            case WSAEINTR:
            case WSAEINPROGRESS:
            case WSAENOBUFS:
            case WSAEOPNOTSUPP:
                errorState = SYS;
                break;
            case WSAENETDOWN:
                errorState = NET;
                break;
            case WSAENETRESET:
            case WSAEHOSTUNREACH:
            case WSAECONNABORTED:
            case WSAECONNRESET:
            case WSAETIMEDOUT:
                finish();
            case WSAESHUTDOWN:
                errorState = CLOSED;
                break;
            case WSAEACCES:
            case WSAEWOULDBLOCK:
            case WSAEMSGSIZE:
            default:
                errorState = UNKNOWN;
                break;
        }
        std::cerr << "Failed to receive data from peer! Errno: " << e << std::endl;
        return -1;
    }
    return r;
}


std::string Connection::readall()
{
    errorState = OK;
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Cannot read data from a connection that isn't connected yet!" << std::endl;
        errorState = EARLY;
        return "";
    }
    unsigned long yesiwantyoutousenonblock = 1;
    if (ioctlsocket(sock, FIONBIO, &yesiwantyoutousenonblock) != 0)
    {
        std::cerr << "Failed to set socket to nonblocking mode before attempting to read data!" << std::endl;
        serrorState = SYS;
        return "";
    }
    std::string ret = "";
    char buf[512];
    int sizeRead = recv(sock, buf, 512, 0);
    while (true)
    {
        if (sizeRead == SOCKET_ERROR)
        {
            int err = WSAGetLastError();
            if (err == WSAEWOULDBLOCK)
            {
                break;
            }
            switch (err)
            {
                case WSANOTINITIALISED:
                    started = false;
                case WSAENOTCONN:
                case WSAENOTSOCK:
                case WSAEINVAL:
                    finish();
                case WSAEINTR:
                case WSAEINPROGRESS:
                case WSAENOBUFS:
                case WSAEOPNOTSUPP:
                case WSAEFAULT:
                    errorState = SYS;
                    break;
                case WSAENETDOWN:
                    errorState = NET;
                    break;
                case WSAENETRESET:
                case WSAEHOSTUNREACH:
                case WSAECONNABORTED:
                case WSAECONNRESET:
                case WSAETIMEDOUT:
                    finish();
                case WSAESHUTDOWN:
                    errorState = CLOSED;
                    break;
                case WSAEACCES:
                case WSAEWOULDBLOCK:
                case WSAEMSGSIZE:
                default:
                    errorState = UNKNOWN;
                    break;
            }
            std::cerr << "Error while reading! Errno: " << err << std::endl;
            yesiwantyoutousenonblock = 0; // No more. Goodbye.
            ioctlsocket(sock, FIONBIO, &yesiwantyoutousenonblock);
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
    ioctlsocket(sock, FIONBIO, &yesiwantyoutousenonblock);
    return ret;
}


#else // elif defined(ANTNET_WIN)
#error sockets.cpp: No ANT OS macros defined! Has sockets.hpp been modified?
#endif // else
