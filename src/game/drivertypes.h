// src/game/drivertypes.h
// Data types shared by the drivers, the manoeuvres and the unit ghost-frame
// queue, at their HD layouts. OWNER: P.
//
// HD containers (the S.W.I.N.E./Gepard templates):
//   SDArray<T>  {T* a; int size; int max}; Add grows to 16, then max*6/5
//               (realloc + zero the new tail); Clear(n) sets size = n,
//               grows max to n and zeroes the whole buffer.
//   SDEQueue<T> {T* a; int count; int max; int base; int top; int bottom}:
//               indices bottom..top are live; slot = i - base, minus max
//               when >= max.

#ifndef PZ_GAME_DRIVERTYPES_H
#define PZ_GAME_DRIVERTYPES_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "m2common.h"

namespace pz {

// Fatal HD consistency error (FUN_0065c970). HD shows a message box and
// exits; the recompile logs it (once per call site text) and the caller
// returns a safe value.
void DrvPanic(const char* fmt, ...);
// HD warning (FUN_0065cac0, log level 1).
void DrvWarn(const char* fmt, ...);

// HD SDArray<T>.
template <typename T>
struct SHdDArray {
    T*  Array;
    int Size;
    int Max;

    void Init() { Array = nullptr; Size = 0; Max = 0; }
    void Free()
    {
        if (Array)
            ::free(Array);
        Array = nullptr;
        Size = 0;
        Max = 0;
    }
    // HD Add (grow by one, zeroed); returns the new index.
    int Add()
    {
        if (Size == Max) {
            int n = Max < 0x10 ? 0x10 : (Max * 6) / 5;
            Array = (T*)::realloc(Array, (size_t)n * sizeof(T));
            memset(Array + Max, 0, (size_t)(n - Max) * sizeof(T));
            Max = n;
        }
        return Size++;
    }
    // HD Clear(n) (0x550a30 for 8-byte elements): size = n, max >= n, zero all.
    void Clear(int n)
    {
        if (Size != 0 && !Array)
            DrvPanic("SDArray<%s>::Clear: array is damaged", "?");
        Size = n;
        if (Max < n) {
            Max = n;
            Array = (T*)::realloc(Array, (size_t)n * sizeof(T));
        }
        if (Array)
            memset(Array, 0, (size_t)Max * sizeof(T));
    }
    // HD Remove(i): shift down, zero the freed tail element.
    void Remove(int i)
    {
        if (i < 0 || i >= Size) {
            DrvPanic("SDArray<%s>::Remove: invalid index (%d) size = %d", "?", i, Size);
            return;
        }
        --Size;
        if (Size - i != 0)
            memmove(Array + i, Array + i + 1, (size_t)(Size - i) * sizeof(T));
        memset(Array + Size, 0, sizeof(T));
    }
    T& At(int i)
    {
        if (i < 0 || i >= Size) {
            DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "?", i);
            static T s_dummy;
            memset(&s_dummy, 0, sizeof(T));
            return s_dummy;
        }
        return Array[i];
    }
};

struct SVec2 {
    float X;
    float Z;
};

// HD SGhostFrame (0x74): the state of the "ghost" that runs ahead of the
// unit. Built from the unit by 0x550ec0, written back by 0x553960.
struct SGhostFrame {
    float X;                 // +0x00 unit +0x8c
    float Y;                 // +0x04 unit +0x90
    float Z;                 // +0x08 unit +0x94
    float Dir;               // +0x0c unit +0xb0
    float Speed;             // +0x10 unit +0xc8
    bool  Reverse;           // +0x14 unit +0xcc
    unsigned char _15[3];
    float SpinSpeed;         // +0x18 unit +0xd0
    int   SpeedStep;         // +0x1c speed / PDriver->MaxSpeed * driver +0xd4 (-1 when +0xd4 == 1)
    int   SpinStep;          // +0x20 spin / PDriver spin * driver +0xd8
    int   WayPointIdx;       // +0x24 LocalWayPointsArray index
    int   ManoeuvreIdx;      // +0x28 manoeuvre of that waypoint
    int   PointIdx;          // +0x2c point of that manoeuvre
    float Gear[5][2];        // +0x30 unit +0x210..+0x237
    float Gear2[5];          // +0x58 unit +0x238..+0x24b
    float Field6c;           // +0x6c unit +0x24c
    float Field70;           // +0x70 unit +0x300 (ClassType 10 only)
};

// HD waypoint of a manoeuvre (0x14).
enum {
    kWpPoint = 1,            // go to the point
    kWpTurn = 2,             // turn to Dir in place
    kWpPointDir = 3,         // go to the point, then Dir
    kWpNear = 4,             // stop near the point
    kWpFollowUnit = 5,       // follow the target unit
};
struct SWayPoint {
    float X;                 // +0x00
    float Z;                 // +0x04
    float Dir;               // +0x08
    int   Type;              // +0x0c kWp*
    bool  Reverse;           // +0x10
    unsigned char _11[3];
};

// HD SManoeuvre (0x40), one candidate way of passing a waypoint.
struct SManoeuvre {
    int   Type;              // +0x00 1..0x11 (see manoeuvre.cpp)
    int   Index;             // +0x04 index after sorting (SortManoeuvres)
    SHdDArray<SWayPoint> Points; // +0x08
    float StartX;            // +0x14
    float StartZ;            // +0x18
    float Dist2;             // +0x1c (start - waypoint)^2
    float EndX;              // +0x20
    float EndZ;              // +0x24
    float Circle0[2];        // +0x28 (0x58ce90 out)
    float Circle1[2];        // +0x30 (0x58ce90 out)
    const char* Name;        // +0x38 debug name (HD: heap copy, FUN_0052c320)
    unsigned char _3c[4];
};

// HD SWayPointWithManoeuvres (0x3c): a LocalWayPointsArray entry.
enum {
    kLwpFirst = -1,          // the waypoint the ghost starts from
    kLwpMiddle = 0,
    kLwpBeforeLast = 1,
    kLwpLastStop = 2,
    kLwpLastPass = 3,
    kLwpLocal = 4,           // a lone local point (0x550650 / 0x5518c0)
};
struct SWayPointWithManoeuvres {
    SHdDArray<SManoeuvre> Manoeuvres; // +0x00
    unsigned Flags;          // +0x0c bit i = manoeuvre i still possible
    bool  Reverse;           // +0x10 target +0x2c
    unsigned char _11[3];
    float Radius;            // +0x14 turning radius (driver +0xcc)
    float X;                 // +0x18
    float Z;                 // +0x1c
    int   PointType;         // +0x20 kLwp*
    float InDir;             // +0x24
    float NextX;             // +0x28
    float NextZ;             // +0x2c
    float NextOutDir;        // +0x30
    float DirToWp;           // +0x34 atan2(from the ghost to the waypoint)
    float OutDir;            // +0x38
};

#if defined(_M_IX86)
static_assert(sizeof(SGhostFrame) == 0x74, "HD SGhostFrame");
static_assert(sizeof(SWayPoint) == 0x14, "HD waypoint");
static_assert(sizeof(SManoeuvre) == 0x40, "HD SManoeuvre");
static_assert(sizeof(SWayPointWithManoeuvres) == 0x3c, "HD SWayPointWithManoeuvres");
#endif

} // namespace pz

#endif // PZ_GAME_DRIVERTYPES_H
