#include <iostream>
#include <fstream>
#include <cstdlib>


int main()
{
    std::ofstream f("outMap", std::ios::trunc | std::ios::binary);
    f.write("03e803e804\n", 11);

    f.write("0000000005\n", 11);
    for (int i = 0; i < 5; i++)
    {
        f.write("0000000000\n", 11);
    }

    f.write("03e7000005\n", 11);
    for (int i = 0; i < 5; i++)
    {
        f.write("03e7000000\n", 11);
    }

    f.write("03e703e705\n", 11);
    for (int i = 0; i < 5; i++)
    {
        f.write("03e703e700\n", 11);
    }

    f.write("000003e705\n", 11);
    for (int i = 0; i < 5; i++)
    {
        f.write("000003e700\n", 11);
    }
    std::string str;
    str.reserve(1000);
    unsigned long foodC = 0;
    unsigned long wallC = 0;
    for (int x = 0; x < 1000; x++)
    {
        for (int y = 0; y < 1000; y++)
        {
            double d = sqrt(x*x+y*y);
            d = std::min(d, sqrt((1000-x)*(1000-x)+y*y));
            d = std::min(d, sqrt((1000-x)*(1000-x)+(1000-y)*(1000-y)));
            d = std::min(d, sqrt(x*x+(1000-y)*(1000-y)));
            d = std::min(d, sqrt((500-x)*(500-x)+(500-y)*(500-y)));
            if (((x == 499 || x == 500) && (y < 400 || y > 600)) || ((y == 499 || y == 500) && (x < 400 || x > 600)))
            {
                str.push_back('\x01');
                wallC++;
            }
            else if (rand() % (int)std::floor(std::max(d / 10.0, 10.0)) == 0)
            {
                str.push_back('\x02');
                foodC++;
            }
            else
            {
                str.push_back('\0');
            }
        }
        f.write(str.c_str(), 1000);
        str.clear();
        str.reserve(1000);
    }
    f.close();
    std::cout << "Map has " << foodC << " food and " << wallC << " wall tiles." << std::endl;
    return 0;
}
