#include "Simulator.h"
#include "Console.h"
#include <thread>
#include <sstream>

Simulator& Simulator::getInstance()
{
    static Simulator instance;
    return instance;
}

Simulator::Simulator()
{
    Console::getInstance().log(LogLevel::Debug, "Simulator object created");
}

Simulator::~Simulator()
{
}

// simulator handler function
void Simulator::handler(void)
{
    HRESULT hResult;

    while (!Console::getInstance().isQuitRequest())
    {
        if (hSimConnect == nullptr)
        {
            // not connected to simulator - try to connect
            hResult = SimConnect_Open(&hSimConnect, "MsSimConnect", nullptr, 0, 0, 0);
            if (hResult == S_OK)
            {
                Console::getInstance().log(LogLevel::Info, "connecting to SimConnect server");
                threadSleepTime = std::chrono::milliseconds(NormalSleep);
            }
        }
        else
        {
            // connected to simulator - dispatch
            auto fn = std::bind(&Simulator::dispatch, this);
            SimConnect_CallDispatch(hSimConnect, &Simulator::dispatchWrapper, nullptr);
        }
        std::this_thread::sleep_for(threadSleepTime);
    }

    if (hSimConnect)
    {
        // request closing connection with server
        hResult = SimConnect_Close(hSimConnect);
        if (hResult == S_OK)
        {
            Console::getInstance().log(LogLevel::Info, "connection to Simconnect server closed");
        }
        else
        {
            Console::getInstance().log(LogLevel::Error, "failed to disconnect from Simconnect server");
        }
    }
}

void Simulator::dispatchWrapper(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    Simulator::getInstance().dispatch(pData, cbData, pContext);
}

// dispatch data from simulator
void Simulator::dispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    // check SimConnect event ID
    switch (pData->dwID)
    {
    case SIMCONNECT_RECV_ID_OPEN:
        // connection process is complete
        {
            std::stringstream ss;
            SIMCONNECT_RECV_OPEN* pOpenData = static_cast<SIMCONNECT_RECV_OPEN*>(pData);
            ss << "connected to SimConnect v" << pOpenData->dwSimConnectVersionMajor << "." << pOpenData->dwSimConnectVersionMinor;
            Console::getInstance().log(LogLevel::Info, ss.str());

            subscribe();
            dataRequest();
        }
        break;

    case SIMCONNECT_RECV_ID_QUIT:
        // connection closed
        Console::getInstance().log(LogLevel::Info, "SimConnect server connection closed");
        hSimConnect = nullptr;
        threadSleepTime = std::chrono::milliseconds(LongSleep);
        break;

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
        // sim data received
        procesSimData(pData);
        threadSleepTime = std::chrono::milliseconds(ShortSleep);
        break;

    case SIMCONNECT_RECV_ID_NULL:
        // no more data
        threadSleepTime = std::chrono::milliseconds(NormalSleep);
        break;

    default:
        break;
    }
}

// subscribe to simulator data reception
void Simulator::subscribe(void)
{
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Yoke X Position", "Position");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Yoke Y Position", "Position");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Rudder Pedal Position", "Position");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Yoke X Indicator", "Position");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Yoke Y Indicator", "Position");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Rudder Pedal Indicator", "Position");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Elevator Position", "Position");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Elevator Deflection", "Radians");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Elevator Deflection PCT", "Percent Over 100");
}

// add data definition for reception from SimConnect server
void Simulator::addToDataDefinition(HANDLE hSimConnect, SIMCONNECT_DATA_DEFINITION_ID defineID, std::string datumName, std::string unitsName, SIMCONNECT_DATATYPE datumType)
{
    HRESULT hr = SimConnect_AddToDataDefinition(hSimConnect, defineID, datumName.c_str(), unitsName.c_str(), datumType);
    if (hr == S_OK)
    {
        std::string text("subscribed to data: ");
        text += datumName;
        Console::getInstance().log(LogLevel::Debug, text);
    }
    else
    {
        std::string text("failed to subscribe to data: ");
        text += datumName;
        Console::getInstance().log(LogLevel::Error, text);
    }
}

// request all subscribed data from SimConnect server
void Simulator::dataRequest(void)
{
    requestDataOnSimObject(AircraftParametersRequest, AircraftParametersDefinition, SIMCONNECT_PERIOD_SECOND);
}

// request data from SimConnect server - called from Simulator::dataRequest
void Simulator::requestDataOnSimObject(SIMCONNECT_DATA_REQUEST_ID RequestID, SIMCONNECT_DATA_DEFINITION_ID DefineID, SIMCONNECT_PERIOD Period)
{
    HRESULT hr = SimConnect_RequestDataOnSimObject(hSimConnect, RequestID, DefineID, SIMCONNECT_OBJECT_ID_USER, Period);
    if (hr == S_OK)
    {
        std::stringstream ss;
        ss << "request data: def=" << DefineID << ", req=" << RequestID << ", period=" << Period;
        Console::getInstance().log(LogLevel::Debug, ss.str());
    }
    else
    {
        std::stringstream ss;
        ss << "data request error: def=" << DefineID << ", req=" << RequestID << ", period=" << Period;
        Console::getInstance().log(LogLevel::Error, ss.str());
    }
}

// process data received from simulator
void Simulator::procesSimData(SIMCONNECT_RECV* pData)
{
    SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = static_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(pData);
    switch (pObjData->dwRequestID)
    {
    case AircraftParametersRequest:
        // XXX print parameters for test
        break;
    default:
        // error
        break;
    }
}
