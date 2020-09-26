// MsSimConnect.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Console.h"
#include <conio.h>
#include <iostream>


int main()
{
    Console::getInstance().log(LogLevel::Always, "MS SimConnect v1.0");
    Console::getInstance().log(LogLevel::Always, "press Q to quit");

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
