#pragma once
class Simulator
{
public:
    Simulator(Simulator const&) = delete;
    Simulator& operator=(Simulator const&) = delete;
    Simulator& getInstance();
private:
    Simulator();
};

