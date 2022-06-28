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

template<typename iType, typename oType> oType scale(iType iMin, iType iMax, iType input, oType oMin, oType oMax, bool limit = true)
{
    if (limit)
    {
        if (input < iMin)
        {
            input = iMin;
        }
        else if (input > iMax)
        {
            input = iMax;
        }
    }
    auto result = static_cast<oType>(1.0 * (input - iMin) / (iMax - iMin) * (oMax - oMin) + oMin);
    return result;
}