#include "map.hpp"
#include "network.hpp"
#include "sockets.hpp"
#include <iostream>
#include <string>
#include <thread>
#include <signal.h>


#ifdef ANTNET_UNIX
void interruptHandler(int a)
{
    Round::signalFlag = 1;
    return;
}
#endif


int main(int argc, char*args[])
{
#ifdef ANTNET_UNIX
    struct sigaction sa;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = interruptHandler;
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);
#endif
    char prevArg = '\0';
    std::string configFile = "";
    int port = -1;
    bool logging = false;
    for (int i = 1; i < argc; i++)
    {
        std::string arg = args[i];
        switch (prevArg)
        {
            case '\0':
                if (arg[0] == '-')
                {
                    for (int j = 1; j < arg.length(); j++)
                    {
                        switch (arg[j])
                        {
                            case 'p':
                                if (prevArg != '\0')
                                {
                                    std::cerr << "Please provide an argument to the parameter '-" << prevArg << "' before invoking '-p'." << std::endl;
                                    return 2;
                                }
                                if (port != -1)
                                {
                                    std::cerr << "Please only use the parameter '-p' once." << std::endl;
                                    return 2;
                                }
                                prevArg = arg[j];
                                break;
                            case 'f':
                                if (prevArg != '\0')
                                {
                                    std::cerr << "Please provide an argument to the parameter '-" << prevArg << "' before invoking '-" << arg[j] << "'." << std::endl;
                                    return 2;
                                }
                                if (configFile != "")
                                {
                                    std::cerr << "Please only use the parameter '-f' once." << std::endl;
                                    return 2;
                                }
                                prevArg = arg[j];
                                break;
                            case 'l':
                                logging = true;
                                break;
                            case 'L':
                                logging = false;
                                break;
                        }
                    }
                }
                break;
            case 'p':
                prevArg = '\0';
                port = std::stoi(arg);
                if (port < 0 || port > 65535)
                {
                    std::cerr << "Invalid port number, should be between 0 and 65535." << std::endl;
                    return 2;
                }
                break;
            case 'f':
                prevArg = '\0';
                configFile = arg;
                break;
        }
    }
    if (configFile.empty())
    {
        configFile = "antgame.cfg";
    }
    Round r;
    r.logging = logging;
    if (r.open())
    {
        while (r.phase != Round::CLOSED && r.phase != Round::INIT)
        {
            auto start = std::chrono::steady_clock::now();
            r.step();
            auto end = std::chrono::steady_clock::now();
            for (;std::chrono::duration<double>(end-start).count() < 1.0/60.0;end = std::chrono::steady_clock::now())
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
        return 0;
    }
    else
    {
        std::cerr << "Failed to open server!" << std::endl;
        return 1;
    }
}
