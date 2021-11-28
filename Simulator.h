#pragma once

#include <Windows.h>
#include "SimConnect.h"
#include "USB.h"
#include "Arbiter.h"
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
    void setSimdataFlag(uint8_t bitPosition, bool value);
    void processNewData();  // process received data from SimConnect and prepare for joystick
    HANDLE hSimConnect{ nullptr };
    const uint8_t ShortSleep = 1;
    const uint8_t NormalSleep = 10;
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
        SimDataReadRequest,
        SimDataTestRequest
    };
    struct SimDataRead      // SimConnect data to be send or compute for HID joystick
    {
        double aileronPosition;     // used for yoke X zero position calculations (w/o vibratiobs)
        double yokeXindicator;      // used for yoke X zero position calculations (w/o vibratiobs)
        double elevatorTrimPCT;
        double rudderTrimPCT;
        double numberOfEngines;
        double prop1Percent;
        double prop2Percent;
        double estimatedCruiseSpeed;
        double indicatedAirspeed;
        double flapsNumHandlePositions; // number of flaps positions excluding position 0 (extracted)
        double flapsHandleIndex;        // flaps lever position 0..flapsNumHandlePositions
        double autopilotMaster;         // is autopilot on?
        double rotationAccBodyX;        // rotation acceleration body X
        double rotationAccBodyY;        // rotation acceleration body Y
        double rotationAccBodyZ;        // rotation acceleration body Z
        double accelerationBodyX;       // acceleration in X axis
        double accelerationBodyY;       // acceleration in Y axis
    };
    struct JoyData  // data received from joystick device
    {
        float xxx;      // not specified yet
    };
    struct SimDataWriteGen   // general data to set in simulator
    {
        double xxx;
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
    struct SimDataCalculated    // calculated simulation data
    {
        double normalizedSpeed;     // speed referenced to cruise speed
    };
    std::set<DWORD> dwIDs;  // set of received SimConnect dwIDs
    USBHID* pJoystickLink{ nullptr };   // pointer to USB HID joystick device
    std::chrono::steady_clock::time_point lastSimDataTime;  // remembers time of last simData reception from server
    std::chrono::steady_clock::time_point lastJoystickDataTime;  // remembers time of last joystick data reception
    SimDataRead simDataRead;    // current state of simData
    double simDataInterval{ 0 };    // time between last two simData readouts [s]
    JoyData joyData{ 0 };    // data received from joystick
    SimDataWriteGen simDataWriteGen;      // general data to be written to simulator
    std::chrono::steady_clock::time_point lastJoystickSendTime;  // remembers time of last joystick data sending
    static const size_t JoySendBufferSize = 64;
    uint8_t joySendBuffer[JoySendBufferSize];
    uint32_t simDataFlags{ 0 };     //bit flags received from simulator
    bool simConnectSetError{ false };   //last attempt to set in SimConnect failed?
    bool simConnectResponseError{ false };  //last connection to SimConnect failed?
    SimDataCalculated simDataCalculated;    // calculated parameters
};

