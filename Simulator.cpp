#include "Simulator.h"
#include "Console.h"
#include <thread>
#include <sstream>

Simulator& Simulator::getInstance()
{
    static Simulator instance;
    return instance;
}

Simulator::Simulator() :
    joystickLink(VENDOR_ID, PRODUCT_ID, REPORT_ID)
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
            else
            {
                putchar('.');   //XXX test
            }
        }
        else
        {
            // connected to simulator - dispatch
            SimConnect_CallDispatch(hSimConnect, &Simulator::dispatchWrapper, nullptr);
        }

        if (joystickLink.isConnectionOpen())
        {
            // send simulator data here
        }
        else
        {
            // no USB connection - try to connect
            if (joystickLink.openConnection())
            {
                // connection has been opened
                // enable reception
                joystickLink.enableReception();
            }
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

    if (joystickLink.isConnectionOpen())
    {
        joystickLink.disableReception();
        joystickLink.closeConnection();
    }
}

void Simulator::dispatchWrapper(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    Simulator::getInstance().dispatch(pData, cbData, pContext);
}

// dispatch data from simulator
void Simulator::dispatch(SIMCONNECT_RECV* pData, DWORD cbData, void* pContext)
{
    std::stringstream ss;
    // check SimConnect message ID
    switch (pData->dwID)
    {
    case SIMCONNECT_RECV_ID_OPEN:
        // connection process is complete
        {
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
        if (dwIDs.find(pData->dwID) != dwIDs.end())
        {
            // a new unknown dwID received
            ss << "unknown dwID=" << pData->dwID << " received";
            Console::getInstance().log(LogLevel::Debug, ss.str());
            dwIDs.insert(pData->dwID);
        }
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
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Elevator Trim Position", "Position");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Elevator Trim Indicator", "Position");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Elevator Trim PCT", "Percent Over 100");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Sigma SQRT", "Number");
    addToDataDefinition(hSimConnect, AircraftParametersDefinition, "Dynamic Pressure", "Pounds Per Square Foot");
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
    std::stringstream ss;
    HRESULT hr = SimConnect_RequestDataOnSimObject(hSimConnect, RequestID, DefineID, SIMCONNECT_OBJECT_ID_USER, Period);
    if (hr == S_OK)
    {
        ss << "request data: def=" << DefineID << ", req=" << RequestID << ", period=" << Period;
        Console::getInstance().log(LogLevel::Debug, ss.str());
    }
    else
    {
        ss << "data request error: def=" << DefineID << ", req=" << RequestID << ", period=" << Period;
        Console::getInstance().log(LogLevel::Error, ss.str());
    }
}

// process data received from simulator
void Simulator::procesSimData(SIMCONNECT_RECV* pData)
{
    SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = static_cast<SIMCONNECT_RECV_SIMOBJECT_DATA*>(pData);
    std::stringstream ss;
    switch (pObjData->dwRequestID)
    {
    case AircraftParametersRequest:
        // XXX print parameters for test
        {
            SimData* pSimData = reinterpret_cast<SimData*>(&pObjData->dwData);
            //ss << pSimData->yokePositionX;
            ss << " " << pSimData->yokePositionY;
            //ss << " " << pSimData->rudderPedalPosition;
            //ss << " " << pSimData->yokeIndicatorX;
            ss << " " << pSimData->yokeIndicatorY;
            //ss << " " << pSimData->rudderPedalIndicator;
            ss << " " << pSimData->elevatorPosition;
            ss << " " << pSimData->elevatorDeflection;
            ss << " " << pSimData->elevatorDeflectionPCT;
            ss << " " << pSimData->elevatorTrimPosition;
            ss << " " << pSimData->elevatorTrimIndicator;
            ss << " " << pSimData->elevatorTrimPCT;
            ss << " " << pSimData->sigmaSQRT;
            ss << " " << pSimData->dynamicPressure;
            Console::getInstance().log(LogLevel::Info, ss.str());
        }
        break;
    default:
        // unexpected data received
        ss << "unexpected data received, id=" << pObjData->dwRequestID;
        Console::getInstance().log(LogLevel::Warning, ss.str());
        break;
    }
}
