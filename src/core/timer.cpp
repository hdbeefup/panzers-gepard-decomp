// core/timer.cpp
// High-resolution timer
// Decompiled from: gameSplit/globals.c

#include "timer.h"
#include "logger.h"

//----- (004FF9A0) --------------------------------------------------------

STimer::STimer()
{
    LARGE_INTEGER Frequency;

    if (!QueryPerformanceFrequency(&Frequency))
        Logger.g->Panic("STimer::GetHiresFreq: QueryPerformanceFrequency failed");

    this->hiresFreq = Frequency.QuadPart;

    LARGE_INTEGER Count;
    QueryPerformanceCounter(&Count);
    this->hiresStartTime = Count.QuadPart;
}

//----- (004FFA20) --------------------------------------------------------

LARGE_INTEGER STimer::GetHiresFreq()
{
    LARGE_INTEGER freq;

    if (!QueryPerformanceFrequency(&freq))
        Logger.g->Panic("STimer::GetHiresFreq: QueryPerformanceFrequency failed");

    return freq;
}

//----- (004FFA70) --------------------------------------------------------

LARGE_INTEGER STimer::GetHiresTime()
{
    LARGE_INTEGER count;
    QueryPerformanceCounter(&count);
    return count;
}

//----- (004FFAA0) --------------------------------------------------------

unsigned int STimer::GetTickValue24bit()
{
    LARGE_INTEGER PerformanceCount;
    QueryPerformanceCounter(&PerformanceCount);
    return (unsigned int)((1000 * (PerformanceCount.QuadPart - this->hiresStartTime) / this->hiresFreq) & 0xFFFFFF);
}

//----- (004FFB00) --------------------------------------------------------

__int64 STimer::GetTickValue()
{
    LARGE_INTEGER PerformanceCount;
    QueryPerformanceCounter(&PerformanceCount);
    return 1000 * (PerformanceCount.QuadPart - this->hiresStartTime) / this->hiresFreq;
}
