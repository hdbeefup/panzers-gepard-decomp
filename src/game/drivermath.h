// src/game/drivermath.h
// HD maths helpers used by the drivers, manoeuvres, formations and units.
//
// The HD CRT routines (sin 0x78d640, cos 0x78d480, tan 0x78d810, atan2
// 0x78d07a, the fast atan 0x661440) and the world random LCG live in the
// engine's pz/hdmath.h, shared with the SModel node transforms; see the
// determinism notes there. The "x87 (_CIsin style)" note in M2_COVERAGE.md
// is about the dispatch stub; the executed path is SSE2.

#ifndef PZ_GAME_DRIVERMATH_H
#define PZ_GAME_DRIVERMATH_H

#include <emmintrin.h>
#include "pz/hdmath.h"

namespace pz {

// HD double constants (the float-precision pi values stored as doubles).
extern const double kHdPi;              // 0x7f4560 3.1415927410125732
extern const double kHdTwoPi;           // 0x7f4570 6.2831854820251465
extern const double kHdMinusPi;         // 0x7f5aa0 -3.1415927410125732

// Angle helpers (all return through ST0 as double in HD).
double HdWrapAdd(double a, double b);   // PANZERS 0x550600 a+b wrapped once into (-pi, pi]
double HdWrapSub(double a, double b);   // PANZERS 0x55c1e0 a-b wrapped once
double HdAngleDist(double a, double b); // PANZERS 0x551bc0 |a-b|, 2pi-|a-b| above pi (x87 fsub)
double HdSign(double v);                // PANZERS 0x55b5c0 -1 / 0 / 1

// World random seed (World+0x7518, MSVC rand LCG).
void   HdRngTrace(void* caller);      // recompile only: PZ_M2_RNGLOG=1 logs each world random draw
int    HdRandInt(int range);            // PANZERS 0x555a00 (int)(r15 * (1/32768) * range)
double HdRandDouble(double range);      // PANZERS 0x559740 r15 * (1/32768) * range

// x86 returns doubles in ST0, and MSVC may keep computing on the x87 stack
// after such a call; the game's control word 0x007F would round that double
// arithmetic to 24 bits where HD used SSE2. Callers use these wrappers,
// which move the result into an SSE register first.
__forceinline double SseD(double v) { return _mm_cvtsd_f64(_mm_set_sd(v)); }
__forceinline float  SseF(float v) { return _mm_cvtss_f32(_mm_set_ss(v)); }
__forceinline double DSin(double x) { return SseD(HdSin(x)); }
__forceinline double DCos(double x) { return SseD(HdCos(x)); }
__forceinline double DAtan2(double y, double x) { return SseD(HdAtan2(y, x)); }
__forceinline float  DAtan2f(double y, double x) { return SseF(HdAtan2f(y, x)); }
__forceinline double DWrapAdd(double a, double b) { return SseD(HdWrapAdd(a, b)); }
__forceinline double DWrapSub(double a, double b) { return SseD(HdWrapSub(a, b)); }
__forceinline double DAngleDist(double a, double b) { return SseD(HdAngleDist(a, b)); }
__forceinline double DSign(double v) { return SseD(HdSign(v)); }
__forceinline double DRandDouble(double r) { return SseD(HdRandDouble(r)); }

} // namespace pz

#endif // PZ_GAME_DRIVERMATH_H
