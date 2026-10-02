// core/timer.h
// STimer — high-resolution timer
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef CORE_TIMER_H
#define CORE_TIMER_H

#include "core_common.h"

struct STimer {
    __int64 hiresFreq;
    __int64 hiresStartTime;

    STimer();
    LARGE_INTEGER GetHiresTime();
    LARGE_INTEGER GetHiresFreq();
    unsigned int GetTickValue24bit();
    __int64 GetTickValue();
};

extern STimer Timer;

#endif // CORE_TIMER_H
