#pragma once

#include <Windows.h>
#include "SimConnect.h"
#include "USB.h"
#include <iostream>
#include <chrono>
#include <set>
#include <vector>

class Simulator
{
public:
    Simulator(Simulator const&) = delete;
    Simulator& operator=(Simulator const&) = delete;
    static Simulator& getInstance();
    void handler(void);
    static void CALLBACK dispatchWrapper(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);
    void setJoystickLink(USBHID* pLink) { pJoystickLink = pLink; }
    void parseReceivedData(std::vector<uint8_t> receivedData);      // parse received data fron joystick link
    void displaySimData();
    void displayReceivedJoystickData();
private:
    Simulator();
    ~Simulator();
    void dispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);    // dispatch messages from SimConnect server
    void subscribe(void);       // subscribes to SimConnect data
    void addToDataDefinition(HANDLE  hSimConnect, SIMCONNECT_DATA_DEFINITION_ID  defineID, std::string datumName, std::string unitsName, SIMCONNECT_DATATYPE  datumType = SIMCONNECT_DATATYPE_FLOAT64);
    void dataRequest(void);     // requests data from SimConnect server
    void requestDataOnSimObject(SIMCONNECT_DATA_REQUEST_ID  RequestID, SIMCONNECT_DATA_DEFINITION_ID  DefineID, SIMCONNECT_PERIOD  Period);
    void procesSimData(SIMCONNECT_RECV* pData);     // processes data received from SimConnect server
    HANDLE hSimConnect{ nullptr };
    const uint8_t ShortSleep = 1;
    const uint8_t NormalSleep = 5;
    const uint16_t LongSleep = 1000;
    std::chrono::milliseconds threadSleepTime{ std::chrono::milliseconds(LongSleep) };      // idle time between handler calls
    enum  DataDefineID      // SimConnect data subscription sets
    {
        SimDataReadDefinition,
        SimDataTestDefinition,
        SimDataWriteDefinition
    };
    enum DataRequestID      // SimConnect data request sets
    {
        SimDataRequest,
        VariableCheckRequest
    };
    struct SimDataRead      // SimConnect data to be send or compute for HID joystick
    {
        double aileronPosition;
        double elevatorTrimPCT;
        double rudderTrimPCT;
        double numberOfEngines;
        double prop1Percent;
        double prop2Percent;
        double estimatedCruiseSpeed;
        double indicatedAirspeed;
        double rotationVelocityBodyX;   // Rotation relative to aircraft X axis (pitch / elevator)
        double rotationVelocityBodyY;   // Rotation relative to aircraft Y axis (vertical axis, yaw / rudder)
        double rotationVelocityBodyZ;   // Rotation relative to aircraft Z axis (roll / aileron)
        double flapsNumHandlePositions; // number of flaps positions excluding position 0 (extracted)
        double flapsHandleIndex;        // flaps lever position 0..flapsNumHandlePositions
    };
    struct JoyData  // data received from joystick device
    {
        uint8_t flapsPositionIndex;
    };
    struct SimDataWrite   // data to set in simulator
    {
        double flapsHandleIndex;
        double yokeXposition;
    };
    struct SimDataTest    // SimConnect data for verification and test
    {
        double yokeYposition;
        double yokeYpositionAP;
        double yokeYindicator;
        double elevatorDeflectionPCT;
        double elevatorPosition;
        double elevatorTrimIndicator;
        double elevatorTrimPCT;
    };
    std::set<DWORD> dwIDs;  // set of received SimConnect dwIDs
    USBHID* pJoystickLink{ nullptr };   // pointer to USB HID joystick device
    std::chrono::steady_clock::time_point lastSimDataTime;  // remembers time of last simData reception from server
    std::chrono::steady_clock::time_point lastJoystickDataTime;  // remembers time of last joystick data reception
    double lastRotationVelocityBodyX{ 0 };
    double lastRotationVelocityBodyY{ 0 };
    double lastRotationVelocityBodyZ{ 0 };
    double angularAccelerationX{ 0 };       // for vibrations on joystick Y axis (pitch)
    double angularAccelerationY{ 0 };       // for vibrations on rudder pedals (yaw)
    double angularAccelerationZ{ 0 };       // for vibrations on joystick X axis (roll)
    SimDataRead simDataRead;    // current state of simData
    double simDataInterval{ 0 };    // time between last two simData readouts [s]
    JoyData joyData;    // data received from joystick
    SimDataWrite simDataWrite;      // data to be write in simulator
    std::chrono::steady_clock::time_point lastJoystickSendTime;  // remembers time of last joystick data sending
};

