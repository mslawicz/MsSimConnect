#pragma once

#include <Windows.h>
#include "SimConnect.h"
#include <iostream>


class Simulator
{
public:
    Simulator(Simulator const&) = delete;
    Simulator& operator=(Simulator const&) = delete;
    static Simulator& getInstance();
    void doNothing(void) {} // XXX for test
    void handler(void);
private:
    Simulator();
    ~Simulator();
    void dispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);
};

