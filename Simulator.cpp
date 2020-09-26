#include "Simulator.h"

Simulator& Simulator::getInstance()
{
    static Simulator instance;
    return instance;
}

Simulator::Simulator()
{
}
// dispatch data from simulator
void Simulator::dispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    // check SimConnect event ID
    switch (pData->dwID)
    {
    case SIMCONNECT_RECV_ID_OPEN:
        // connection process is complete
        break;

    case SIMCONNECT_RECV_ID_QUIT:
        // connection closed
        break;

    default:
        break;
    }
}
