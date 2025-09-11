#include <string>
#include <vector>
#ifndef SOCKETS_HPP
#define SOCKETS_HPP

#ifndef ANTNET_LISTEN_BACKLOG
#define ANTNET_LISTEN_BACKLOG 16
#endif

#ifndef ANTNET_DEFAULT_PORT
#define ANTNET_DEFAULT_PORT 55521
#endif

#if defined(unix) || defined(__APPLE__)
#define ANTNET_UNIX
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

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
    private:
#ifdef ANTNET_WIN
#ifndef CLIENT
    static SOCKET listenSock;
    static bool listenOpen;
#endif
    static int instances;
    static bool started;
    static WSADATA wsadata;
    SOCKET sock;
#elif defined(ANTNET_UNIX)
#ifndef CLIENT
    static int listenSockfd;
    public:
    Connection(int);
    private:
#endif
    int sockfd = -1;
#endif    
    public:
    Connection();
    ~Connection();
    static int port;
    #ifndef CLIENT
    static int fetchConnections(std::vector<Connection>*);
    static void closeListen();
    static bool openListen(int);
    static bool listening();
    #endif
    bool connectTo(std::string, int);
    bool send(char*, size_t);
    int receive(char*, size_t);
    std::string readall();
    void finish();
    bool connected();
};
#endif
