#include "Console.h"
#include <iostream>


Console& Console::getInstance()
{
    static Console instance;
    return instance;
}

Console::Console()
{
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