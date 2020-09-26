#pragma once

#include <Windows.h>
#include "SimConnect.h"

class Simulator
{
public:
    Simulator(Simulator const&) = delete;
    Simulator& operator=(Simulator const&) = delete;
    static Simulator& getInstance();
private:
    Simulator();
    void dispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext);
};

