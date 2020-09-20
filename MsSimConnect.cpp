// MsSimConnect.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <Windows.h>
#include <conio.h>
#include "SimConnect.h"
#include <iostream>


int main()
{
    std::cout << "MS SimConnect v1.0\n";
    std::cout << "press Q to quit\n";

    int key = 0;
    do
    {
        if (_kbhit())
        {
            key = toupper(_getch());
            std::cout << std::hex << key << ",";
        }
    } while(key != 0x51);   //'Q'
}
