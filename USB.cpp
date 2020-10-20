#pragma comment(lib, "SetupAPI.lib")

#include "USB.h"
#include "Console.h"
#include <SetupAPI.h>
#include <string>

USBHID::USBHID(USHORT VID, USHORT PID, uint8_t collection) :
    VID(VID),
    PID(PID),
    collection(collection)
{
}

USBHID::~USBHID()
{
}

// find the USB device and open the connection to it
bool USBHID::openConnection()
{
    isOpen = false;
    bool found = false;     // mark that the device has not been found yet
    HidD_GetHidGuid(&hidGuid);      // get the device interfaceGUID for HIDClass devices

    // SetupDiGetClassDevs function returns a handle to a device information set that contains requested device information elements for a local computer
    HDEVINFO deviceInfoSet = SetupDiGetClassDevs(&hidGuid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE)
    {
        Console::getInstance().log(LogLevel::Error, "Invalid handle to device information set, error code=" + std::to_string(GetLastError()));
        return isOpen;
    }

    SP_DEVINFO_DATA deviceInfoData;     //  structure defines a device instance that is a member of a device information set
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    // SetupDiEnumDeviceInfo function returns a SP_DEVINFO_DATA structure that specifies a device information element in a device information set
    for (int deviceIndex = 0; SetupDiEnumDeviceInfo(deviceInfoSet, deviceIndex, &deviceInfoData) && !found; deviceIndex++)
    {
        SP_DEVICE_INTERFACE_DATA devInterfaceData;  // structure defines a device interface in a device information set
        devInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        // SetupDiEnumDeviceInterfaces function enumerates the device interfaces that are contained in a device information set
        for (int interfaceIndex = 0; SetupDiEnumDeviceInterfaces(deviceInfoSet, &deviceInfoData, &hidGuid, interfaceIndex, &devInterfaceData) && !found; interfaceIndex++)
        {
            DWORD bufferSize = 0;
            // SetupDiGetDeviceInterfaceDetail function returns details about a device interface
            // this first call is for getting required size of the DeviceInterfaceDetailData buffer
            SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &devInterfaceData, NULL, 0, &bufferSize, &deviceInfoData);

            // pointer to buffer that receives information about the device that supports the requested interface
            PSP_DEVICE_INTERFACE_DETAIL_DATA pDeviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(bufferSize);
            if (pDeviceInterfaceDetailData != nullptr)
            {
                pDeviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
            }
            else
            {
                Console::getInstance().log(LogLevel::Error, "Memory allocation error");
                continue;
            }

            // SetupDiGetDeviceInterfaceDetail function returns details about a device interface
            // this second call is for getting actual information about the device that supports the requested interface
            if (SetupDiGetDeviceInterfaceDetail(deviceInfoSet, &devInterfaceData, pDeviceInterfaceDetailData, bufferSize, &bufferSize, &deviceInfoData))
            {
                SECURITY_ATTRIBUTES securityAttributes;
                memset(&securityAttributes, 0, sizeof(SECURITY_ATTRIBUTES));
                securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
                securityAttributes.bInheritHandle = true;

                // Creates or opens a file or I/O device
                // query metadata such as file, directory, or device attributes without accessing device 
                fileHandle = CreateFile(pDeviceInterfaceDetailData->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
                    &securityAttributes, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

                if (fileHandle != INVALID_HANDLE_VALUE)
                {
                    HIDD_ATTRIBUTES	attributes;
                    memset(&attributes, 0, sizeof(HIDD_ATTRIBUTES));
                    attributes.Size = sizeof(HIDD_ATTRIBUTES);
                    if (HidD_GetAttributes(fileHandle, &attributes))
                    {
                        CloseHandle(fileHandle);
                        std::wstring collectionStr = L"&col";
                        if (collection < 10)
                        {
                            collectionStr += L"0";
                        }
                        collectionStr += std::to_wstring(collection);

                        if ((attributes.VendorID == VID) &&
                            (attributes.ProductID == PID) &&
                            ((collection == 0) || wcsstr(pDeviceInterfaceDetailData->DevicePath, collectionStr.c_str())))
                        {
                            // device with proper VID, PID and collection found (or collection == 0)
                            // Creates or opens a file or I/O device - this time for read/write operations in asynchronous mode
                            fileHandle = CreateFile(pDeviceInterfaceDetailData->DevicePath, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                                &securityAttributes, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
                            if (fileHandle != INVALID_HANDLE_VALUE)
                            {
                                isOpen = true;
                                Console::getInstance().log(LogLevel::Info, "Connection to USB HID device with VID=" + std::to_string(VID) + " PID=" + std::to_string(PID) + " opened");
                            }
                            else
                            {
                                Console::getInstance().log(LogLevel::Error, "USB device found, but cannot be opened for read/write operations, error code=" + std::to_string(GetLastError()));
                            }
                            // mark that the device has been found regardless if it has been opened
                            found = true;
                        }
                    }
                }
            }
            free(pDeviceInterfaceDetailData);
        }
    }
    SetupDiDestroyDeviceInfoList(deviceInfoSet);

    if (!isOpen)
    {
        Console::getInstance().log(LogLevel::Debug, "USB HID device with VID=" + std::to_string(VID) + " PID=" + std::to_string(PID) + " not found");
    }

    return isOpen;
}


// closes connection to the device
void USBHID::closeConnection()
{
    if (fileHandle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(fileHandle);
        fileHandle = INVALID_HANDLE_VALUE;
        Console::getInstance().log(LogLevel::Info, "closing connection to HID device with VID=" + std::to_string(VID) + " PID=" + std::to_string(PID));
    }
    else
    {
        Console::getInstance().log(LogLevel::Debug, "Invalid handle to HID device");
    }
    isOpen = false;
}