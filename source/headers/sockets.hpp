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
    Connection(SOCKET);
#endif
    static unsigned int instances;
    static bool started;
    static WSADATA wsadata;
    SOCKET sock = INVALID_SOCKET;
#elif defined(ANTNET_UNIX)
#ifndef CLIENT
    static int listenSockfd;
    Connection(int);
#endif
    int sockfd = -1;
#endif    
    public:
    /*
    	Constructs a connection object. It is possible that this function is empty.
    */
    Connection();

    /*
	Destructs a connection object. This WILL close the socket, so always pass a pointer when you suspect this might be called and you don't want it to close, e.g. in a list that frequently reallocates.
    */
    ~Connection();

    /*
    	The port on which the listening socket is listening. Setting this does not do anything.
    */
    static int port;

    enum ErrState {OK, RETRY, CLOSED, SYS, ADDR, NET, EARLY, FAULT, UNKNOWN}; // OK means no error has occured in the last call, RETRY means the call failed in a one-time way, i.e. you can retry later, CLOSED means the call failed because the connection was closed by the peer, SYS means the error is caused by system shenanigans (probably the bind call's fault), ADDR means either the remote address is unknown or the local address (when binding) is used (i.e. your port is taken up), NET means the network is down, EARLY means this request is too early (e.g. the socket isn't connected yet), FAULT means a user-provided buffer is not in user space (e.g. is NULL), only returned by send and receive and UNKNOWN means it happened for an unknown reason (woah what really)
    /*
    	Represents the state of error on the last client socket-related call. This is set by every non-static member function but connected().
    */
    ErrState errorState = OK;

    #ifndef CLIENT

    /*
    	Represents the state of error on the last server socket-related call. This is set by every static member function but listening().
    */
    static ErrState serrorState;

    /*
  	Closes listening socket.
    */
    static void closeListen();

    /*
	Starts listening on the provided port. 
    */
    static bool openListen(int nport);

    /*
	On a listening socket, accepts all pending connection requests and adds them to list. Takes one argument, a pointer to a list of pointers to connection instances, to which the accepted connections will be added. If it is nullptr, the connections are accepted then immediately closed.
    */
    static int fetchConnections(std::vector<Connection*>* list);

    /*
	Checks if the listening socket is open.
    */
    static bool listening();

    #endif

    /*
	Connects to the provided address and port. Should be an IPv4 address.
    */
    bool connectTo(std::string nip, int nport);

    /*
	Sends a message across a connected socket. Takes a pointer to data and the size of that data.
    */
    bool send(const char* message, size_t len);

    /*
	Receives a certain amount of data from a connected socket. Takes a pointer in which to store the data and the amount of data to restore.
    */
    int receive(char* buf, size_t size);

    /*
   	Reads all the data from a connected socket into a returned standard string.
    */
    std::string readall();

    /*
	Closes a connected socket.
    */
    void finish();

    /*
	Checks if a connection object is connected. Note: if the connection is independently closed by the peer, it will not show up from this function until a send or receive is attempted, which will fail with CLOSED and the connection will be closed on this side.
    */
    bool connected();
};
#endif
