// src/3dengine/pz/hdmath.h
// The HD CRT maths routines and the world random generator, shared by the
// engine (SModel node transforms, which feed the logic through the seat
// nodes) and the game logic (drivers, units, animations).
//
// Determinism (docs/M2_INTERFACES.md §7): the HD code is SSE2 scalar code
// (VS2005 /arch:SSE2) calling the VS2005 CRT, whose results differ from the
// modern UCRT in the last bits, so the recompile carries its own copies:
//   0x78d640 sin, 0x78d480 cos, 0x78d810 tan, 0x78d190 atan: __libm_sse2_*
//     SSE2 table code.
//     They take the SSE2 path when MXCSR is the default and the x87 control
//     word has all exceptions masked (CW & 0x7f == 0x7f), which is always
//     true in the game (CW 0x007F).
//   0x78d07a _CIatan2: x87 fpatan. The CRT dispatcher (0x7b5aa7) loads its
//     own control word 0x133F (64-bit precision) whatever the caller's is,
//     so the result is the extended-precision fpatan, rounded once when the
//     caller stores it.
//   0x78d090 sqrt: sqrtsd (std::sqrt on a double compiles to sqrtsd).
//   0x661440 fast atan (engine).
// Never use the CRT sin/cos/atan/atan2 in code whose results reach the logic.

#ifndef PZ_HDMATH_H
#define PZ_HDMATH_H

namespace pz {

double HdSin(double x);                 // PANZERS 0x78d640
double HdCos(double x);                 // PANZERS 0x78d480
double HdTan(double x);                 // PANZERS 0x78d810
double HdAtan(double x);                // PANZERS 0x78d190
double HdAtan2(double y, double x);     // PANZERS 0x78d07a (_CIatan2: ST1 = y, ST0 = x)
// The same, for callers that store the result with fstp dword (most of
// them): rounds the extended x87 result straight to float (HdAtan2 rounds
// it to double; converting that to float could double-round).
float  HdAtan2f(double y, double x);    // PANZERS 0x78d07a
float  HdFastAtan(float x);             // PANZERS 0x661440 x / (1 + 0.280872 x^2), folded beyond +-1

// The world random seed World+0x7518: the MSVC rand() LCG that HD inlines
// in every user, and the scaling of 0x555a00.
inline unsigned HdLcgStep(unsigned* seed)
{
    *seed = *seed * 0x343fdu + 0x269ec3u;
    return *seed;
}
inline int HdLcg15(unsigned seed)
{
    return (int)(seed >> 16) & 0x7fff;
}
inline int HdRandScale(int r15, int range)   // (int)(r15 * (1/32768) (0x7f4540) * range), in double
{
    return (int)((double)r15 * 3.0517578125e-05 * (double)range);
}

} // namespace pz

#endif // PZ_HDMATH_H
