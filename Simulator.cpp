#include "Simulator.h"
#include "Console.h"
#include <chrono>

Simulator& Simulator::getInstance()
{
    static Simulator instance;
    return instance;
}

Simulator::Simulator() :
    handlerThread(&Simulator::handler, this)
{
    Console::getInstance().log(LogLevel::Debug, "Simulator object created");
}

Simulator::~Simulator()
{
    handlerThread.join();
}

// simulator handler function
void Simulator::handler(void)
{
    static int cnt = 0; //XXX for test only
    while (cnt++ < 10)
    {
        std::cout << "." << std::flush;
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
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
