#include "network.hpp"


RoundSettings* RoundSettings::instance = nullptr;


RoundSettings::RoundSettings()
{
     if (instance == nullptr)
     {
         instance = this;
         isPlayer = true; // Reset this manually if you are the server. Don't do it otherwise.
         port = ANTNET_DEFAULT_PORT;
     }
}
