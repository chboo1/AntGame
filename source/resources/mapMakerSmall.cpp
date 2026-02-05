#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cmath>


int main()
{
    std::ofstream f("outMap", std::ios::trunc | std::ios::binary);
    f.write("01f401f404\n", 11);

    f.write("0000000005\n", 11);
    for (int i = 0; i < 5; i++)
    {
        f.write("0000000000\n", 11);
    }

    f.write("01f3000005\n", 11);
    for (int i = 0; i < 5; i++)
    {
        f.write("01f3000000\n", 11);
    }

    f.write("01f301f305\n", 11);
    for (int i = 0; i < 5; i++)
    {
        f.write("01f301f300\n", 11);
    }

    f.write("000001f305\n", 11);
    for (int i = 0; i < 5; i++)
    {
        f.write("000001f300\n", 11);
    }
    std::string str;
    str.reserve(500);
    unsigned long foodC = 0;
    unsigned long wallC = 0;
    for (int x = 0; x < 500; x++)
    {
        for (int y = 0; y < 500; y++)
        {
            double d = sqrt(x*x+y*y);
            d = std::min(d, sqrt((500-x)*(500-x)+y*y));
            d = std::min(d, sqrt((500-x)*(500-x)+(500-y)*(500-y)));
            d = std::min(d, sqrt(x*x+(500-y)*(500-y)));
            d = std::min(d, sqrt((250-x)*(250-x)+(250-y)*(250-y)));
            /*
            if (((x == 249 || x == 250) && (y < 200 || y > 300)) || ((y == 249 || y == 250) && (x < 200 || x > 300)))
            {
                str.push_back('\x01');
                wallC++;
            }
            else*/ if (rand() % (int)std::floor(std::max(d / 10.0, 10.0)) == 0)
            {
                str.push_back('\x02');
                foodC++;
            }
            else
            {
                str.push_back('\0');
            }
        }
        f.write(str.c_str(), 500);
        str.clear();
        str.reserve(500);
    }
    f.close();
    std::cout << "Map has " << foodC << " food and " << wallC << " wall tiles." << std::endl;
    return 0;
}
