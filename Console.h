#pragma once
class Console
{
public:
    Console(Console const&) = delete;
    Console& operator=(Console const&) = delete;
    Console& getInstance();
private:
    Console();
};
