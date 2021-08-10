#pragma once

#include <cstdint>

template<typename T> void placeData(T data, uint8_t*& pBuffer)
{
    //memcpy(pBuffer, &variable, sizeof(T));
    *reinterpret_cast<T*>(pBuffer) = data;
    pBuffer += sizeof(T);
}

template<typename T> T parseData(uint8_t*& pBuffer)
{
    T data = *reinterpret_cast<T*>(pBuffer);
    pBuffer += sizeof(T);
    return data;
}
