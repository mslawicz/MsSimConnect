// MsSimConnect.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Console.h"
#include "Simulator.h"
#include <iostream>
#include <thread>


int main()
{
    Console::getInstance().log(LogLevel::Always, "MS SimConnect v1.0");
    Console::getInstance().log(LogLevel::Always, "press Q to quit");

    Simulator::getInstance().doNothing();
    std::thread simulatorThread(&Simulator::handler, &Simulator::getInstance());

    Console::getInstance().handler();

    simulatorThread.join();
}
