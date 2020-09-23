#include "Simulator.h"

Simulator& Simulator::getInstance()
{
    static Simulator instance;
    return instance;
}

Simulator::Simulator()
{
}
