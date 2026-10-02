// src/panzers/widgetglue.cpp
// Small engine glue functions the imported SWINE code expects the game layer
// to define (they were SWINE-era stubs in src/stubs/stubs.cpp). Lifted from
// the HD PANZERS.exe equivalents.

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "widget.h"
#include "string2.h"

// PANZERS 0x543cf0
// Win32 timer callback installed by SWidget::SetTimer (0x543b10, SetTimer
// with this as TIMERPROC). Walks the TimerList SHeap (HD 0x8da774 array,
// 0x8da778 size; element stride 0xc = {use, id, target}) and calls
// target->OnTimer(index, time) (vtbl +0x50) for the entry with this id. The
// array is re-read after every call (the handler may add or remove timers).
void CALLBACK TimerProc(HWND hwnd, UINT msg, UINT_PTR idEvent, DWORD time)
{
    (void)hwnd; (void)msg;
    int i = -1;
    for (;;) {
        ++i;
        if (i >= TimerList.size)
            return;
        while (TimerList.array[i].use != 0x7FFFFFFF) {
            ++i;
            if (i >= TimerList.size)
                return;
        }
        if (TimerList.array[i].data.IDEvent == idEvent)
            TimerList.array[i].data.Target->OnTimer(i, time);
    }
}

// PANZERS 0x51ee20
// SString printf: vsprintf into a 0x400-byte stack buffer; a result of
// 1..0x3ff characters becomes the string (0x51de30), anything else (error,
// empty or truncated) an empty SString.
SString* Format(SString* result, const char* fmt, ...)
{
    char buf[0x400];
    va_list args;
    va_start(args, fmt);
    int len = _vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    if (len < 0)
        len = -1;
    if ((unsigned)(len - 1) < 0x3ff) {
        result->size = len;
        result->buf = (char*)operator new[](len + 1);
        memcpy(result->buf, buf, len + 1);
    } else {
        result->buf = nullptr;
        result->size = 0;
    }
    return result;
}
