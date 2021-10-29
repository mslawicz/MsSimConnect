#pragma once

template <class T>
class Arbiter
{
public:
    bool setRequested(T remote, T local, uint16_t counterLoad);
private:
    T lastRemote;
    T lastLocal;
    uint16_t remoteCounter;
    uint16_t localCounter;
};

template <class T>
bool Arbiter<T>::setRequested(T remote, T local, uint16_t counterLoad)
{
    bool setRequest{ false };

    if ((remote != lastRemote) && (0 == localCounter))
    {
        remoteCounter = counterLoad;
        setRequest = true;
    }

    if ((local != lastLocal) && (0 == remoteCounter))
    {
        localCounter = counterLoad;
    }

    lastRemote = remote;
    lastLocal = local;

    if (0 < remoteCounter)
    {
        remoteCounter--;
    }

    if (0 < localCounter)
    {
        localCounter--;
    }

    return setRequest;
}
