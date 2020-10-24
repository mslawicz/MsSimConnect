#pragma once

#include <Windows.h>
#include "SimConnect.h"
#include "USB.h"
#include <iostream>
#include <chrono>
#include <set>

class Simulator
{
public:
    Simulator(Simulator const&) = delete;
    Simulator& operator=(Simulator const&) = delete;
    static Simulator& getInstance();
    void handler(void);
    static void CALLBACK dispatchWrapper(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);
    void setJoystickLink(USBHID* pLink) { pJoystickLink = pLink; }
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
        AircraftParametersDefinition
    };
    enum DataRequestID
    {
        AircraftParametersRequest
    };
    struct SimData
    {
        double yokePositionX;
        double yokePositionY;
        double rudderPedalPosition;
        double yokeIndicatorX;
        double yokeIndicatorY;
        double rudderPedalIndicator;
        double elevatorPosition;
        double elevatorDeflection;
        double elevatorDeflectionPCT;
        double elevatorTrimPosition;
        double elevatorTrimIndicator;
        double elevatorTrimPCT;
        double sigmaSQRT;
        double dynamicPressure;
    };
    std::set<DWORD> dwIDs;  // set of received SimConnect dwIDs
    USBHID* pJoystickLink{ nullptr };
};

