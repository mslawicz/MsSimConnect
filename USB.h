#pragma once
#pragma comment(lib, "hid.lib")

#include <Windows.h>
#include <cstdint>
#include <hidsdi.h>
#include <string>

#define HID_BUFFER_SIZE    64   // report id + 63 bytes of payload

class USBHID
{
public:
    USBHID(USHORT VID, USHORT PID, uint8_t collection);
    ~USBHID();
    bool openConnection();
    void closeConnection();
    bool isConnectionOpen() const { return isOpen; }
    void enableReception(void);
    void disableReception(void); // clears the reception event (no signals until enabled again)
private:
    USHORT VID;
    USHORT PID;
    uint8_t collection;
    bool isOpen{ false };        // true if the device is found and open
    GUID hidGuid{ 0 };           // pointer to a caller-allocated GUID buffer that the routine uses to return the device interface GUID for HIDClass devices
    HANDLE fileHandle{ nullptr };
    std::string VidPid;
    std::wstring collectionStr;
    static const size_t ReceiveBufferSize = 260;
    uint8_t receiveBuffer[ReceiveBufferSize];
    LPDWORD receivedDataCount;
    OVERLAPPED receiveOverlappedData;
};

