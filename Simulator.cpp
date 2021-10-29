#include "Simulator.h"
#include "Console.h"
#include "Convert.h"
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
        // manage connection to simulator
        if (hSimConnect == nullptr)
        {
            // not connected to simulator - try to connect
            hResult = SimConnect_Open(&hSimConnect, "MsSimConnect", nullptr, 0, 0, 0);
            if (hResult == S_OK)
            {
                Console::getInstance().log(LogLevel::Info, "connecting to SimConnect server");
                threadSleepTime = std::chrono::milliseconds(NormalSleep);
                simConnectResponseError = false;
            }
            else
            {
                if (!simConnectResponseError)
                {
                    Console::getInstance().log(LogLevel::Warning, "no response from SimConnect server");
                    simConnectResponseError = true;
                }
            }
        }
        else
        {
            // connected to simulator - dispatch
            SimConnect_CallDispatch(hSimConnect, &Simulator::dispatchWrapper, nullptr);
        }

        //send data to joystick
        if (pJoystickLink &&
            (std::chrono::duration<double>(std::chrono::steady_clock::now() - lastJoystickSendTime).count() > 0.02))
        {
            uint8_t* pBuffer = joySendBuffer;
            placeData<uint8_t>(static_cast<uint8_t>(simDataRead.flapsNumHandlePositions), pBuffer);
            placeData<uint8_t>(static_cast<uint8_t>(simDataRead.flapsHandleIndex), pBuffer);
            placeData<float>(static_cast<float>(simDataRead.aileronPosition - simDataRead.yokeXindicator), pBuffer);
            placeData<uint32_t>(simDataFlags, pBuffer);
            placeData<float>(static_cast<float>(simDataRead.throttleLever1Pos), pBuffer);
            placeData<char>('S', pBuffer);
            placeData<char>('I', pBuffer);
            placeData<char>('M', pBuffer);
            pJoystickLink->sendData(joySendBuffer);
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
        setSimdataFlag(0, false);    //SimConnect data invalid
        threadSleepTime = std::chrono::milliseconds(LongSleep);
        break;

    case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
        // sim data received
        procesSimData(pData);
        setSimdataFlag(0, true);    //SimConnect data valid
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
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "AILERON POSITION", "Position");    // used for yoke X zero position calculations (w/o vibrations)
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "YOKE X INDICATOR", "Position");    // used for yoke X zero position calculations (w/o vibrations)
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "Elevator Trim PCT", "Percent Over 100");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "Rudder Trim PCT", "Percent Over 100");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "NUMBER OF ENGINES", "Number");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "PROP MAX RPM PERCENT:1", "Percent");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "PROP MAX RPM PERCENT:2", "Percent");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "ESTIMATED CRUISE SPEED", "Knots");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "AIRSPEED INDICATED", "Knots");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "ROTATION VELOCITY BODY X", "Radians per second");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "ROTATION VELOCITY BODY Y", "Radians per second");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "ROTATION VELOCITY BODY Z", "Radians per second");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "FLAPS NUM HANDLE POSITIONS", "Number");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "FLAPS HANDLE INDEX", "Number");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "AUTOPILOT MASTER", "Bool");        // autopilot master on/off
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "GENERAL ENG THROTTLE LEVER POSITION:1", "Number");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "GENERAL ENG THROTTLE LEVER POSITION:2", "Number");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "GENERAL ENG THROTTLE LEVER POSITION:3", "Number");
    addToDataDefinition(hSimConnect, SimDataReadDefinition, "GENERAL ENG THROTTLE LEVER POSITION:4", "Number");

    // simconnect variables for testing
    addToDataDefinition(hSimConnect, SimDataTestDefinition, "YOKE Y POSITION", "Position");
    addToDataDefinition(hSimConnect, SimDataTestDefinition, "YOKE Y POSITION WITH AP", "Position");
    addToDataDefinition(hSimConnect, SimDataTestDefinition, "YOKE Y INDICATOR", "Position");
    addToDataDefinition(hSimConnect, SimDataTestDefinition, "ELEVATOR DEFLECTION PCT", "Percent Over 100");
    addToDataDefinition(hSimConnect, SimDataTestDefinition, "ELEVATOR POSITION", "Position");
    addToDataDefinition(hSimConnect, SimDataTestDefinition, "ELEVATOR TRIM INDICATOR", "Position");
    addToDataDefinition(hSimConnect, SimDataTestDefinition, "ELEVATOR TRIM PCT", "Percent Over 100");

    // simconnect variables for setting
    addToDataDefinition(hSimConnect, SimDataWriteDefinition, "FLAPS HANDLE INDEX", "Number");
    addToDataDefinition(hSimConnect, SimDataWriteDefinition, "YOKE X POSITION", "Position");    // write to simulator as yoke current X position

    // simconnect variables for setting
    addToDataDefinition(hSimConnect, SimDataSetThrottleDefinition, "GENERAL ENG THROTTLE LEVER POSITION:1", "Number");   // throttle lever 1 position
    addToDataDefinition(hSimConnect, SimDataSetThrottleDefinition, "GENERAL ENG THROTTLE LEVER POSITION:2", "Number");   // throttle lever 2 position
    addToDataDefinition(hSimConnect, SimDataSetThrottleDefinition, "GENERAL ENG THROTTLE LEVER POSITION:3", "Number");   // throttle lever 3 position
    addToDataDefinition(hSimConnect, SimDataSetThrottleDefinition, "GENERAL ENG THROTTLE LEVER POSITION:4", "Number");   // throttle lever 4 position
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
    requestDataOnSimObject(SimDataReadRequest, SimDataReadDefinition, SIMCONNECT_PERIOD_SIM_FRAME);
    requestDataOnSimObject(SimDataTestRequest, SimDataTestDefinition, SIMCONNECT_PERIOD_SECOND);
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
    case SimDataReadRequest:
        {
            auto simDataTime = std::chrono::steady_clock::now();
            SimDataRead* pSimDataRead = reinterpret_cast<SimDataRead*>(&pObjData->dwData);
            memcpy(&simDataRead, pSimDataRead, sizeof(SimDataRead));
            simDataInterval = std::chrono::duration<double>(simDataTime - lastSimDataTime).count();
            lastSimDataTime = simDataTime;

            setSimdataFlag(1, simDataRead.autopilotMaster != 0);    //flag of autopilot master on/off

            angularAccelerationX = simDataInterval != 0 ? (simDataRead.rotationVelocityBodyX - lastRotationVelocityBodyX) / simDataInterval : 0;
            angularAccelerationY = simDataInterval != 0 ? (simDataRead.rotationVelocityBodyY - lastRotationVelocityBodyY) / simDataInterval : 0;
            angularAccelerationZ = simDataInterval != 0 ? (simDataRead.rotationVelocityBodyZ - lastRotationVelocityBodyZ) / simDataInterval : 0;
            lastRotationVelocityBodyX = simDataRead.rotationVelocityBodyX;
            lastRotationVelocityBodyY = simDataRead.rotationVelocityBodyY;
            lastRotationVelocityBodyZ = simDataRead.rotationVelocityBodyZ;
        }
        break;

    case SimDataTestRequest:
        // XXX print parameters for test
        {
            SimDataTest* pVariableCheck = reinterpret_cast<SimDataTest*>(&pObjData->dwData);
            //ss << "yYp=" << pVariableCheck->yokeYposition << "  ";
            //ss << "yYpAP=" << pVariableCheck->yokeYpositionAP << "  ";
            //ss << "yYi=" << pVariableCheck->yokeYindicator << "  ";
            //ss << "edPCT=" << pVariableCheck->elevatorDeflectionPCT << "  ";
            //ss << "ep=" << pVariableCheck->elevatorPosition << "  ";
            //ss << "eti=" << pVariableCheck->elevatorTrimIndicator << "  ";
            //ss << "etPCT=" << pVariableCheck->elevatorTrimPCT << "  ";
            ss << "aP=" << simDataRead.aileronPosition << "  ";
            ss << "yXi=" << simDataRead.yokeXindicator << "  ";
            ss << "zero=" << simDataRead.aileronPosition - simDataRead.yokeXindicator  << "  ";
            ss << "pil=" << simDataWriteGen.yokeXposition << "  ";
            //Console::getInstance().log(LogLevel::Info, ss.str());
        }
        break;

    default:
        // unexpected data received
        ss << "unexpected data received, id=" << pObjData->dwRequestID;
        Console::getInstance().log(LogLevel::Warning, ss.str());
        break;
    }
}

// parse received data from joystick link
void Simulator::parseReceivedData(std::vector<uint8_t> receivedData)
{
    lastJoystickDataTime = std::chrono::steady_clock::now();
    uint8_t* pData = &receivedData.data()[1];
    joyData.yokeXposition = parseData<float>(pData);
    joyData.commandedThrottle = parseData<float>(pData);

    //prepare data for simulator
    if (simDataRead.autopilotMaster != 0)
    {
        // autopilot is on
        simDataWriteGen.yokeXposition = 0;
    }
    else
    {
        // autopilot is off
        simDataWriteGen.yokeXposition = joyData.yokeXposition;
    }

    std::stringstream ss;
    HRESULT hr = SimConnect_SetDataOnSimObject(hSimConnect, SimDataWriteDefinition, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(SimDataWriteGen), &simDataWriteGen);
    if ((hr != S_OK) && (!simConnectSetError))
    {
        ss << "failed to set in simConnect server";
        Console::getInstance().log(LogLevel::Error, ss.str());
        simConnectSetError = true;
    }
    if ((hr == S_OK) && (simConnectSetError))
    {
        ss << "sucseeded to set in simConnect server";
        Console::getInstance().log(LogLevel::Info, ss.str());
        simConnectSetError = false;
    }

    if (throttleArbiter.setRequested(joyData.commandedThrottle, simDataRead.throttleLever1Pos, 10))
    {
        // request for setting throttle in simulator
        simDataWriteThr.commandedThrottle1 = simDataWriteThr.commandedThrottle2 = simDataWriteThr.commandedThrottle3 = simDataWriteThr.commandedThrottle4 = joyData.commandedThrottle;
        SimConnect_SetDataOnSimObject(hSimConnect, SimDataSetThrottleDefinition, SIMCONNECT_OBJECT_ID_USER, 0, 0, sizeof(SimDataWriteThr), &simDataWriteThr);
    }
}

// display current data received from SimConnect server
void Simulator::displaySimData()
{
    std::cout << "time from last SimData [s] = " << std::chrono::duration<double>(std::chrono::steady_clock::now() - lastSimDataTime).count() << std::endl;
    std::cout << "last SimData interval [s] = " << simDataInterval << std::endl;
    std::cout << "========== SimDataRead ==========" << std::endl;
    std::cout << "aileron position = " << simDataRead.aileronPosition << std::endl;
    std::cout << "yoke X indicator = " << simDataRead.yokeXindicator << std::endl;
    std::cout << "elevator trim % = " << simDataRead.elevatorTrimPCT << std::endl;
    std::cout << "rudder trim % = " << simDataRead.rudderTrimPCT << std::endl;
    std::cout << "number of engines = " << simDataRead.numberOfEngines << std::endl;
    std::cout << "propeller 1 % = " << simDataRead.prop1Percent << std::endl;
    std::cout << "propeller 2 % = " << simDataRead.prop2Percent << std::endl;
    std::cout << "estimated cruise speed [kts] = " << simDataRead.estimatedCruiseSpeed << std::endl;
    std::cout << "indicated airspeed [kts] = " << simDataRead.indicatedAirspeed << std::endl;
    std::cout << "rotation velocity body X [rad/s] = " << simDataRead.rotationVelocityBodyX << std::endl;
    std::cout << "rotation velocity body Y [rad/s] = " << simDataRead.rotationVelocityBodyY << std::endl;
    std::cout << "rotation velocity body Z [rad/s] = " << simDataRead.rotationVelocityBodyZ << std::endl;
    std::cout << "number of flaps positions = " << simDataRead.flapsNumHandlePositions << std::endl;
    std::cout << "flaps lever position = " << simDataRead.flapsHandleIndex << std::endl;
    std::cout << "autopilot master = " << simDataRead.autopilotMaster << std::endl;
    std::cout << "throttle lever = " << simDataRead.throttleLever1Pos << std::endl;
    std::cout << "========== SimDataWrite ==========" << std::endl;
    std::cout << "yoke X position = " << simDataWriteGen.yokeXposition << std::endl;
    std::cout << "flaps handle index = " << simDataWriteGen.flapsHandleIndex << std::endl;
    std::cout << "commanded throttle = " << simDataWriteThr.commandedThrottle1 << std::endl;
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
    std::cout << "yoke X position = " << joyData.yokeXposition << std::endl;
    std::cout << "commanded throttle = " << joyData.commandedThrottle << std::endl;
}

//set/reset sim data flag
void Simulator::setSimdataFlag(uint8_t bitPosition, bool value)
{
    if (value)
    {
        simDataFlags |= (1 << bitPosition);
    }
    else
    {
        simDataFlags &= ~(1 << bitPosition);
    }
}