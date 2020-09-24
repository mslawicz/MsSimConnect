#include "Console.h"

Console& Console::getInstance()
{
    static Console instance;
    return instance;
}

Console::Console()
{
}