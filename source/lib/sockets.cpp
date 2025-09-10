#include "sockets.hpp"


#ifdef ANTNET_UNIX
int Connection::listenSockfd = -1;
struct sockaddr_in Connection::peerAddress;

void Connection::init()
{
    if (initiated)
    {
        this->close(); // Doing this to prevent ambiguity with the linux syscall close
    }
}


void Connection::_initServer(int port)
{
    listenSockfd = socket(AF_INET, SOCK_STREAM, 0);
    peerAddress.sin_family = AF_INET;
    peerAddress.sin_port = htons(port);
}

#elif defined(ANTNET_WIN)
#else
#error sockets.cpp: No ANT OS macros defined! Has sockets.hpp been modified?
#endif
