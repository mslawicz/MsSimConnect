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
        SimDataDefinition,
        VariableCheckDefinition
    };
    enum DataRequestID      // SimConnect data request sets
    {
        SimDataRequest,
        VariableCheckRequest
    };
    struct SimData      // SimConnect data to be send or compute for HID joystick
    {
        double yokeXIndicator;
        double aileronPosition;
        double ailreronTrimPCT;
        double elevatorTrimPCT;
        double rudderTrimPCT;
        double numberOfEngines;
        double prop1Percent;
        double prop2Percent;
        double estimatedCruiseSpeed;
        double indicatedAirspeed;
        double rotationVelocityBodyX;
        double rotationVelocityBodyZ;
    };
    struct VariableCheck    // SimConnect data for verification and test
    {
        double rotVelBodyY;
    };
    std::set<DWORD> dwIDs;  // set of received SimConnect dwIDs
    USBHID* pJoystickLink{ nullptr };   // pointer to USB HID joystick device
    std::chrono::steady_clock::time_point lastSimDataTime;  // remembers time of last simData reception from server
    double lastRotationVelocityBodyX{ 0 };
    double lastRotationVelocityBodyZ{ 0 };
    double angularAccelerationX{ 0 };
    double angularAccelerationZ{ 0 };
    SimData simData;    // current state of simData
    double simDataInterval{ 0 };    // time between last two simData readouts [s]
};

