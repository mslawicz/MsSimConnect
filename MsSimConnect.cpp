// MsSimConnect.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "Console.h"
#include "USB.h"
#include "Simulator.h"
#include <iostream>
#include <thread>
#include <functional>

#define VENDOR_ID   0x483
#define PRODUCT_ID  0x5712  // HID joystick + 2
#define REPORT_ID   0x02

int main()
{
    Console::getInstance().log(LogLevel::Always, "MS SimConnect client v1.0");
    Console::getInstance().log(LogLevel::Always, "type 'help' for the list of commands");

    USBHID joystickLink(VENDOR_ID, PRODUCT_ID, REPORT_ID);
    Simulator::getInstance().setJoystickLink(&joystickLink);
    joystickLink.setParseFunction(std::bind(&Simulator::parseReceivedData, &Simulator::getInstance(), std::placeholders::_1));

    std::thread joystickLinkThread(&USBHID::handler, &joystickLink);
    std::thread simulatorThread(&Simulator::handler, &Simulator::getInstance());

    Console::getInstance().handler();

    simulatorThread.join();
    joystickLinkThread.join();
}
