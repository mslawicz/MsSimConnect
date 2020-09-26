#pragma once

#include <Windows.h>
#include "SimConnect.h"
#include <iostream>
#include <thread>

class Simulator
{
public:
    Simulator(Simulator const&) = delete;
    Simulator& operator=(Simulator const&) = delete;
    static Simulator& getInstance();
    void doNothing(void) {} // XXX for test
private:
    Simulator();
    ~Simulator();
    void handler(void);
    void dispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);
    std::thread handlerThread;
};

