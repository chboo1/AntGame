#include "../headers/network.cpp"


RoundSettings* RoundSettings::instance = nullptr;


RoundSettings::RoundSettings()
{
     if (instance == nullptr)
     {
         instance = this;
         isClient = true; // Reset this manually if you are the server. Don't do it otherwise.
     }
}
