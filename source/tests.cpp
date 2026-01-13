#include "sockets.hpp"
#include <iostream>


int main()
{
    if (!Connection::openListen(42069))
    {
        std::cerr << "Failed to open server." << std::endl;
        return 1;
    }
    if (!Connection::listening())
    {
        std::cerr << "Server says it's not listening when it should be." << std::endl;
        Connection::closeListen();
        return 2;
    }
    Connection conn;
    if (!conn.connectTo("127.0.0.1", 42069))
    {
        std::cerr << "Failed to connect through loopback" << std::endl;
        Connection::closeListen();
        return 3;
    }
    std::cout << "Wait start" << std::endl;
    for (int i = 0; i < 1000000000; i++) {} // Waiting
    std::cout << "Wait end" << std::endl;
    std::vector<Connection*> acceptedStuff;
    while (Connection::fetchConnections(&acceptedStuff) != 1 || acceptedStuff.size() < 1)
    {
        std::cerr << "Failed to accept connection." << std::endl;
        //Connection::closeListen();
        //return 4;
    }
    char message[8] = "Hi me!\n";
    if (!conn.send(message, 7))
    {
        std::cerr << "Client failed to send data" << std::endl;
        delete acceptedStuff[0];
        Connection::closeListen();
        return 5;
    }
    std::cout << "Wait start" << std::endl;
    for (int i = 0; i < 1000000000; i++) {} // Waiting
    std::cout << "Wait end" << std::endl;
    std::string data = acceptedStuff[0]->readall();
    while (data != "Hi me!\n")
    {
        std::cerr << "Server read the wrong data: `" << data << "'" << std::endl;
        data = acceptedStuff[0]->readall();
    }
    delete acceptedStuff[0];
    Connection::closeListen();
}
