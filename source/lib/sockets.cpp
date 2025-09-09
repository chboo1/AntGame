#include "../headers/sockets.hpp"


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

#elif defined(ANTNET_WIN)
#else
#error sockets.cpp: No ANT OS macros defined! Has sockets.hpp been modified?
#endif
