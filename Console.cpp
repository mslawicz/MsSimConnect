#include "Console.h"
#include <conio.h>
#include <string>
#include <iostream>


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
    std::string userInput;

    while (!quitRequest)
    {
        std::cout << "\n>" << std::flush;

        std::getline(std::cin, userInput);
        std::cout << "\n--> " << userInput.c_str(); //XXX test

        if (userInput[0] == 'q')
        {
            quitRequest = true;
        }

        std::cin.clear();
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