#pragma once
#pragma comment(lib, "hid.lib")

#include <Windows.h>
#include <cstdint>
#include <hidsdi.h>
#include <string>


class USBHID
{
public:
    USBHID(USHORT VID, USHORT PID, uint8_t collection);
    ~USBHID();
    void handler();
    bool openConnection();
    void closeConnection();
    bool isConnectionOpen() const { return isOpen; }
    bool enableReception(void);
    void disableReception(void); // clears the reception event (no signals until enabled again)
    bool isDataReceived(void);
private:
    USHORT VID;
    USHORT PID;
    uint8_t collection;
    bool isOpen{ false };        // true if the device is found and open
    GUID hidGuid{ CLSID_NULL };  // pointer to a caller-allocated GUID buffer that the routine uses to return the device interface GUID for HIDClass devices
    HANDLE fileHandle{ INVALID_HANDLE_VALUE };
    std::string VidPid;
    std::wstring collectionStr;
    static const size_t ReceiveBufferSize = 260;
    const DWORD HidBufferSize = collection ? 64 : 65; // report id (!=0) + 63 bytes of payload or report id (==0) + 64 bytes of payload
    uint8_t receiveBuffer[ReceiveBufferSize];
    DWORD receivedDataCount;
    OVERLAPPED receiveOverlappedData;
};
