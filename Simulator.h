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
private:
    Simulator();
    ~Simulator();
    void dispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);
    void subscribe(void);
    void addToDataDefinition(HANDLE  hSimConnect, SIMCONNECT_DATA_DEFINITION_ID  defineID, std::string datumName, std::string unitsName, SIMCONNECT_DATATYPE  datumType = SIMCONNECT_DATATYPE_FLOAT64);
    void dataRequest(void);
    void requestDataOnSimObject(SIMCONNECT_DATA_REQUEST_ID  RequestID, SIMCONNECT_DATA_DEFINITION_ID  DefineID, SIMCONNECT_PERIOD  Period);
    void procesSimData(SIMCONNECT_RECV* pData);
    HANDLE hSimConnect{ nullptr };
    const uint8_t ShortSleep = 1;
    const uint8_t NormalSleep = 5;
    const uint16_t LongSleep = 1000;
    std::chrono::milliseconds threadSleepTime{ std::chrono::milliseconds(LongSleep) };
    enum  DataDefineID
    {
        SimDataDefinition,
        VariableCheckDefinition
    };
    enum DataRequestID
    {
        SimDataRequest,
        VariableCheckRequest
    };
    struct SimData
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
    };
    struct VariableCheck
    {
        double rotVelBodyX;
        double rotVelBodyY;
        double rotVelBodyZ;
    };
    std::set<DWORD> dwIDs;  // set of received SimConnect dwIDs
    USBHID* pJoystickLink{ nullptr };
};

