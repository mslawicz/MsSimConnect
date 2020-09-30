#pragma once

#include <Windows.h>
#include "SimConnect.h"
#include <iostream>
#include <chrono>


class Simulator
{
public:
    Simulator(Simulator const&) = delete;
    Simulator& operator=(Simulator const&) = delete;
    static Simulator& getInstance();
    void doNothing(void) {} // XXX for test
    void handler(void);
    static void CALLBACK dispatchWrapper(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);
private:
    Simulator();
    ~Simulator();
    void dispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);
    void subscribe(void);
    void addToDataDefinition(HANDLE  hSimConnect, SIMCONNECT_DATA_DEFINITION_ID  defineID, std::string datumName, std::string unitsName, SIMCONNECT_DATATYPE  datumType = SIMCONNECT_DATATYPE_FLOAT64);
    void dataRequest(void);
    HANDLE hSimConnect{ nullptr };
    const uint8_t ShortSleep = 1;
    const uint8_t NormalSleep = 10;
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
};

