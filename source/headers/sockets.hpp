#include <string>
#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#if defined(unix) || defined(__APPLE__)
#define ANTNET_UNIX
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

#elif defined(_WIN32)
#define ANTNET_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")

#else
#warning "OS macros unix, apple and windows could not be found (I don't know what OS you use!). Proceeding under unix function. If you use windows, it will fail to compile."
#define ANTNET_UNIX

#endif


class Connection
{
#ifdef ANTNET_WIN
#ifndef CLIENT
    static SOCKET listenSock;
#endif
    static WSADATA wsadata;
    static struct addrinfo peerAddress;
    static std::string port;
    SOCKET sock;
#elif defined(ANTNET_UNIX)
#ifndef CLIENT
    static int listenSockfd;
#endif
    static struct sockaddr_in peerAddress;
    int sockfd;
    int port;
#endif    
    bool initiated = false;
    bool connected = false;
    public:
    void init();
    #ifndef CLIENT
    void _initServer(int);
    #endif
    static void listen(int);
    void connect(std::string, int);
    void send(char*, size_t);
    void recv(char*, size_t);
    std::string readall();
    void close();
};
#endif
