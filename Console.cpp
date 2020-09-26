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
    registerCommand("quit", "quit from program", std::bind(&Console::quit, this));
}

// console handler
void Console::handler(void)
{
    while (!quitRequest)
    {
        std::cout << "\n>" << std::flush;

        std::string command;
        std::cin >> command;

        if (commands.find(command) != commands.end())
        {
            // command exists - execute it
            commands[command].second();
        }
        else
        {
            //command not found
            std::cout << "\ninvalid command: " << command.c_str();
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

//register console command
void Console::registerCommand(std::string command, std::string description, std::function<void(void)> action)
{
    commands[command] = std::pair < std::string, std::function<void(void)>>(description, action);
}