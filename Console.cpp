#include "Console.h"
#include <conio.h>
#include <string>
#include <iostream>
#include <sstream>


Console& Console::getInstance()
{
    static Console instance;
    return instance;
}

Console::Console()
{
}

// console handler
void Console::handler(void)
{
    while (!quitRequest)
    {
        std::cout << "\n>" << std::flush;

        std::string command;
        std::cin >> command;

        std::cout << " command=" << command.c_str(); //XXX test

        if (command[0] == 'q')
        {
            quitRequest = true;
        }

        std::cin.clear();
        std::cin.ignore(INT_MAX, '\n');
    }
}

// log message in console window
void Console::log(LogLevel level, std::string message)
{
    int iCurrentLevel = static_cast<int>(currentLevel);
    int iLevel = static_cast<int>(level);
    if ((iLevel <= iCurrentLevel) && (iCurrentLevel > 0))
    {
        std::cout << levelText.find(level)->second.c_str() << ": " << message.c_str() << std::endl;
    }
}