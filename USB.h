#pragma once

#include <Windows.h>
#include <cstdint>

class USBHID
{
public:
    USBHID(USHORT VID, USHORT PID, uint8_t collection);
    ~USBHID();
private:
    USHORT VID;
    USHORT PID;
    uint8_t collection
};

