#include <iostream>
#include <fstream>


int main()
{
    std::ofstream f("outMap", std::ios::trunc | std::ios::binary);
    f.write("000003e8000003e804\n", 19);

    f.write("000000000000000005\n", 19);
    for (int i = 0; i < 5; i++)
    {
        f.write("000000000000000000\n", 19);
    }

    f.write("000003e80000000005\n", 19);
    for (int i = 0; i < 5; i++)
    {
        f.write("000003e80000000000\n", 19);
    }

    f.write("000003e8000003e805\n", 19);
    for (int i = 0; i < 5; i++)
    {
        f.write("000003e8000003e800\n", 19);
    }

    f.write("00000000000003e805\n", 19);
    for (int i = 0; i < 5; i++)
    {
        f.write("00000000000003e800\n", 19);
    }
    for (int i = 0; i < 1000*50; i++)
    {
        f.write("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 20);
    }
    f.close();
    return 0;
}
