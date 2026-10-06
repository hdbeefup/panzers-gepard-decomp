// src/game/drivermath.h
// HD maths helpers used by the drivers, manoeuvres and formations. OWNER: P.
//
// Determinism (docs/M2_INTERFACES.md §7): the HD logic is SSE2 scalar code
// (VS2005 /arch:SSE2) plus a few CRT helpers. The CRT helpers are NOT the
// modern UCRT ones, so the recompile carries its own copies:
//   0x78d640 sin, 0x78d480 cos, 0x78d810 tan: __libm_sse2_* SSE2 table code.
//     They take the SSE2 path when MXCSR is the default and the x87 control
//     word has all exceptions masked (CW & 0x7f == 0x7f), which is always
//     true in the menu (CW 0x007F). HdSin/HdCos/HdTan port that SSE2 path.
//   0x78d07a _CIatan2: x87 fpatan. The CRT dispatcher (0x7b5aa7) loads its
//     own control word 0x133F (64-bit precision) whatever the caller's is
//     (HD otherwise runs at 0x007F, forced after every float->int
//     conversion by fldcw 0x8de158), so the result is the extended-precision
//     fpatan, rounded once when the caller stores it. HdAtan2/HdAtan2f do
//     the same and restore the caller's word.
//   0x78d090 sqrt: sqrtsd (std::sqrt on a double compiles to sqrtsd).
// The "x87 (_CIsin style)" note in M2_COVERAGE.md is about the dispatch
// stub; the executed path is SSE2.

#ifndef PZ_GAME_DRIVERMATH_H
#define PZ_GAME_DRIVERMATH_H

#include <emmintrin.h>

namespace pz {

double HdSin(double x);                 // PANZERS 0x78d640
double HdCos(double x);                 // PANZERS 0x78d480
double HdTan(double x);                 // PANZERS 0x78d810
double HdAtan2(double y, double x);     // PANZERS 0x78d07a (_CIatan2: ST1 = y, ST0 = x)
// The same, for callers that store the result with fstp dword (most of
// them): rounds the extended x87 result straight to float (HdAtan2 rounds
// it to double; converting that to float could double-round).
float  HdAtan2f(double y, double x);    // PANZERS 0x78d07a

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
