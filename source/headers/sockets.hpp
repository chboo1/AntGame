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

#if !defined(ANTNET_UNIX) && !defined(ANTNET_WIN)
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
#endif


class Connection
{
    private:
#ifdef ANTNET_WIN
#ifndef CLIENT
    static SOCKET listenSock;
    public:
    Connection(SOCKET);
    private:
#endif
    static unsigned int instances;
    static bool started;
    static WSADATA wsadata;
    SOCKET sock = INVALID_SOCKET;
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
    enum ErrState {OK, RETRY, CLOSED, SYS, ADDR, NET, EARLY, UNKNOWN}; // OK means no error has occured in the last call, RETRY means the call failed in a one-time way, i.e. you can retry later, CLOSED means the call failed because the connection was closed by the peer, SYS means the error is caused by system shenanigans (probably the bind call's fault), ADDR means either the remote address is unknown or the local address (when binding) is used (i.e. your port is taken up), NET means the network is down, EARLY means this request is too early (e.g. the socket isn't connected yet) and UNKNOWN means it happened for an unknown reason (woah what really)
    ErrState errorState = OK; // TODO: Make this do something
    #ifndef CLIENT
    static ErrState serrorState;
    static int fetchConnections(std::vector<Connection*>*);
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
