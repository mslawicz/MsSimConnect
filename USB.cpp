#include "USB.h"

USBHID::USBHID(USHORT VID, USHORT PID, uint8_t collection) :
    VID(VID),
    PID(PID),
    collection(collection)
{
}

USBHID::~USBHID()
{
}
