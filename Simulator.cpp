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
    lastSimDataTime = lastJoystickDataTime = lastJoystickSendTime = std::chrono::steady_clock::now();
    Console::getInstance().registerCommand("simdata", "display last simulator data", std::bind(&Simulator::displaySimData, this));
    Console::getInstance().registerCommand("joydata", "display last joystick data", std::bind(&Simulator::displayReceivedJoystickData, this));
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
                Console::getInstance().log(LogLevel::Warning, "no response from SimConnect server");
            }
        }
        else
        {
            // connected to simulator - dispatch
            SimConnect_CallDispatch(hSimConnect, &Simulator::dispatchWrapper, nullptr);
        }

        //XXX simulation of flaps change
        static std::chrono::steady_clock::time_point lastFlapsChange;
        if (std::chrono::duration<double>(std::chrono::steady_clock::now() - lastFlapsChange).count() > 3)
        {
            static const uint8_t noOfFlaps = 4;
            static int flapsIndx = 0;
            int prevFlapsIndx = flapsIndx;
            flapsIndx += (rand() & 1) ? 1 : -1;
            if (flapsIndx < 0)
            {
                flapsIndx = 1;
            }
            if (flapsIndx == noOfFlaps)
            {
                flapsIndx = noOfFlaps - 2;
            }
            std::stringstream sss;
            sss << "flaps changed " << prevFlapsIndx << "->" << flapsIndx;
            Console::getInstance().log(LogLevel::Info, sss.str());
            simData.flapsNumHandlePositions = noOfFlaps;
            simData.flapsHandleIndex = static_cast<uint8_t>(flapsIndx);
            lastFlapsChange = std::chrono::steady_clock::now();
        }

        if (pJoystickLink &&
            (std::chrono::duration<double>(std::chrono::steady_clock::now() - lastJoystickSendTime).count() > 0.02))
        {
            uint8_t testData[64] =
            { 
                static_cast<uint8_t>(simData.flapsNumHandlePositions),
                static_cast<uint8_t>(simData.flapsHandleIndex),
                'F',
                'L',
                'A',
                'P',
                'S'
            };
            pJoystickLink->sendData(testData);
            lastJoystickSendTime = std::chrono::steady_clock::now();
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
    // aircraft parameters
    addToDataDefinition(hSimConnect, SimDataDefinition, "Yoke X Indicator", "Position");
    addToDataDefinition(hSimConnect, SimDataDefinition, "Aileron Position", "Position");
    addToDataDefinition(hSimConnect, SimDataDefinition, "Aileron Trim PCT", "Percent Over 100");
    addToDataDefinition(hSimConnect, SimDataDefinition, "Elevator Trim PCT", "Percent Over 100");
    addToDataDefinition(hSimConnect, SimDataDefinition, "Rudder Trim PCT", "Percent Over 100");
    addToDataDefinition(hSimConnect, SimDataDefinition, "NUMBER OF ENGINES", "Number");
    addToDataDefinition(hSimConnect, SimDataDefinition, "PROP MAX RPM PERCENT:1", "Percent");
    addToDataDefinition(hSimConnect, SimDataDefinition, "PROP MAX RPM PERCENT:2", "Percent");
    addToDataDefinition(hSimConnect, SimDataDefinition, "ESTIMATED CRUISE SPEED", "Knots");
    addToDataDefinition(hSimConnect, SimDataDefinition, "AIRSPEED INDICATED", "Knots");
    addToDataDefinition(hSimConnect, SimDataDefinition, "ROTATION VELOCITY BODY X", "Radians per second");
    addToDataDefinition(hSimConnect, SimDataDefinition, "ROTATION VELOCITY BODY Y", "Radians per second");
    addToDataDefinition(hSimConnect, SimDataDefinition, "ROTATION VELOCITY BODY Z", "Radians per second");
    addToDataDefinition(hSimConnect, SimDataDefinition, "FLAPS NUM HANDLE POSITIONS", "Number");
    addToDataDefinition(hSimConnect, SimDataDefinition, "FLAPS HANDLE INDEX", "Number");

    // simconnect variables for testing
    addToDataDefinition(hSimConnect, VariableCheckDefinition, "FLAPS NUM HANDLE POSITIONS", "Number");
    addToDataDefinition(hSimConnect, VariableCheckDefinition, "FLAPS HANDLE INDEX", "Number");

    // simconnect variables for setting
    addToDataDefinition(hSimConnect, SimDataSetDefinition, "FLAPS HANDLE INDEX", "Number");
};

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
    requestDataOnSimObject(SimDataRequest, SimDataDefinition, SIMCONNECT_PERIOD_SIM_FRAME);
    requestDataOnSimObject(VariableCheckRequest, VariableCheckDefinition, SIMCONNECT_PERIOD_SECOND);
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
    case SimDataRequest:
        {
            putchar('.');
            auto simDataTime = std::chrono::steady_clock::now();
            SimData* pSimData = reinterpret_cast<SimData*>(&pObjData->dwData);
            memcpy(&simData, pSimData, sizeof(SimData));
            simDataInterval = std::chrono::duration<double>(simDataTime - lastSimDataTime).count();
            lastSimDataTime = simDataTime;

            angularAccelerationX = simDataInterval != 0 ? (simData.rotationVelocityBodyX - lastRotationVelocityBodyX) / simDataInterval : 0;
            angularAccelerationY = simDataInterval != 0 ? (simData.rotationVelocityBodyY - lastRotationVelocityBodyY) / simDataInterval : 0;
            angularAccelerationZ = simDataInterval != 0 ? (simData.rotationVelocityBodyZ - lastRotationVelocityBodyZ) / simDataInterval : 0;
            lastRotationVelocityBodyX = simData.rotationVelocityBodyX;
            lastRotationVelocityBodyY = simData.rotationVelocityBodyY;
            lastRotationVelocityBodyZ = simData.rotationVelocityBodyZ;
        }
        break;

    case VariableCheckRequest:
        // XXX print parameters for test
        {
            VariableCheck* pVariableCheck = reinterpret_cast<VariableCheck*>(&pObjData->dwData);
            ss << "flaps numb pos " << pVariableCheck->flapsNumHandlePositions << "  ";
            ss << "flaps index " << pVariableCheck->flapsHandleIndex;
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

// parse received data fron joystick link
void Simulator::parseReceivedData(std::vector<uint8_t> receivedData)
{
    lastJoystickDataTime = std::chrono::steady_clock::now();

    if (receivedData[1] != joyData.flapsPositionIndex)
    {
        std::stringstream ss;
        ss << "joystick flaps change request " << static_cast<int>(joyData.flapsPositionIndex) << " -> " << static_cast<int>(receivedData[1]) << " : ";
        simDataSet.flapsHandleIndex = static_cast<double>(receivedData[1]);
        HRESULT hr = SimConnect_SetDataOnSimObject(hSimConnect, SimDataSetDefinition, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(SimDataSet), &simDataSet);
        if (hr == S_OK)
        {
            ss << "success";
            Console::getInstance().log(LogLevel::Debug, ss.str());
        }
        else
        {
            ss << "failed to set in simConnect server";
            Console::getInstance().log(LogLevel::Error, ss.str());
        }
        joyData.flapsPositionIndex = receivedData[1];
    }
}

// display current data received from SimConnect server
void Simulator::displaySimData()
{
    std::cout << "time from last SimData [s] = " << std::chrono::duration<double>(std::chrono::steady_clock::now() - lastSimDataTime).count() << std::endl;
    std::cout << "last SimData interval [s] = " << simDataInterval << std::endl;
    std::cout << "========== SimData ==========" << std::endl;
    std::cout << "yoke indicator X = " << simData.yokeXIndicator << std::endl;
    std::cout << "aileron position = " << simData.aileronPosition << std::endl;
    std::cout << "aileron trim % = " << simData.ailreronTrimPCT << std::endl;
    std::cout << "elevator trim % = " << simData.elevatorTrimPCT << std::endl;
    std::cout << "rudder trim % = " << simData.rudderTrimPCT << std::endl;
    std::cout << "number of engines = " << simData.numberOfEngines << std::endl;
    std::cout << "propeller 1 % = " << simData.prop1Percent << std::endl;
    std::cout << "propeller 2 % = " << simData.prop2Percent << std::endl;
    std::cout << "estimated cruise speed [kts] = " << simData.estimatedCruiseSpeed << std::endl;
    std::cout << "indicated airspeed [kts] = " << simData.indicatedAirspeed << std::endl;
    std::cout << "rotation velocity body X [rad/s] = " << simData.rotationVelocityBodyX << std::endl;
    std::cout << "rotation velocity body Y [rad/s] = " << simData.rotationVelocityBodyY << std::endl;
    std::cout << "rotation velocity body Z [rad/s] = " << simData.rotationVelocityBodyZ << std::endl;
    std::cout << "number of flaps positions = " << simData.flapsNumHandlePositions << std::endl;
    std::cout << "flaps lever position = " << simData.flapsHandleIndex << std::endl;
    //std::cout << " = " << simData << std::endl;
    std::cout << "========== calculated data ==========" << std::endl;
    std::cout << "angular acceleration X [rad/s2] = " << angularAccelerationX << std::endl;
    std::cout << "angular acceleration Y [rad/s2] = " << angularAccelerationX << std::endl;
    std::cout << "angular acceleration Z [rad/s2] = " << angularAccelerationZ << std::endl;
}

// display current data received from Joystick
void Simulator::displayReceivedJoystickData()
{
    std::cout << "time from last joystick reception [s] = " << std::chrono::duration<double>(std::chrono::steady_clock::now() - lastJoystickDataTime).count() << std::endl;
    std::cout << "========== Joystick Data ==========" << std::endl;
    std::cout << "flaps lever position index = " << static_cast<int>(joyData.flapsPositionIndex) << std::endl;
}