// src/game/drivermath.cpp
// HD maths helpers for the drivers (see drivermath.h): angle helpers and the
// world random seed. The CRT routines are in src/3dengine/pz/hdmath.cpp.
//
// The x87 part (the 2pi - d of 0x551bc0) uses MSVC x86 inline asm so that
// the x87 rounding (precision control) is exactly the HD one (x86 only).

#include "drivermath.h"
#include "world.h"
#include "worldapi.h"
#include "gamelogic.h"
#include "logger.h"
#include <intrin.h>
#include <stdlib.h>

#include <emmintrin.h>
#include <string.h>

namespace pz {


// ---------------------------------------------------------------------------
// Angle helpers and the world random seed
// ---------------------------------------------------------------------------

const double kHdPi = 3.1415927410125732;        // 0x7f4560 (0x400921fb60000000)
const double kHdTwoPi = 6.2831854820251465;     // 0x7f4570 (0x401921fb60000000)
const double kHdMinusPi = -3.1415927410125732;  // 0x7f5aa0 (0xc00921fb60000000)
static const double kHdInv32768 = 3.0517578125e-05;   // 0x7f4540

// The angle helpers use intrinsics on purpose: MSVC compiles a returned
// "s - kHdTwoPi" as an x87 fsub (it returns through ST0), which the in-game
// control word 0x007F would round to 24 bits. HD does the arithmetic in SSE2.

// PANZERS 0x550600
double HdWrapAdd(double a, double b)
{
    __m128d s = _mm_add_sd(_mm_set_sd(a), _mm_set_sd(b));
    if (_mm_comigt_sd(s, _mm_set_sd(kHdPi)))
        return _mm_cvtsd_f64(_mm_sub_sd(s, _mm_set_sd(kHdTwoPi)));
    if (_mm_comigt_sd(_mm_set_sd(kHdMinusPi), s))
        s = _mm_add_sd(s, _mm_set_sd(kHdTwoPi));
    return _mm_cvtsd_f64(s);
}

// PANZERS 0x55c1e0
double HdWrapSub(double a, double b)
{
    __m128d s = _mm_sub_sd(_mm_set_sd(a), _mm_set_sd(b));
    if (_mm_comigt_sd(s, _mm_set_sd(kHdPi)))
        return _mm_cvtsd_f64(_mm_sub_sd(s, _mm_set_sd(kHdTwoPi)));
    if (_mm_comigt_sd(_mm_set_sd(kHdMinusPi), s))
        s = _mm_add_sd(s, _mm_set_sd(kHdTwoPi));
    return _mm_cvtsd_f64(s);
}

// PANZERS 0x551bc0
// |a - b| in SSE2 (andps with the abs mask); above pi the HD code returns
// fld 2pi; fsub qword |a-b|, an x87 subtraction rounded to the in-game
// precision (CW 0x007F: 24-bit). It is done here in x87 under CW 0x007F too,
// so the value is the one HD hands to its caller (exact in a double; callers
// that store it to a float get the same float).
double HdAngleDist(double a, double b)
{
    double diff = a - b;
    unsigned long long bits;
    memcpy(&bits, &diff, 8);
    bits &= 0x7fffffffffffffffull;
    double d;
    memcpy(&d, &bits, 8);
    if (d > kHdPi) {
        unsigned short oldcw, cw = 0x007f;
        double r;
        const double* tp = &kHdTwoPi;
        __asm {
            fnstcw  oldcw
            fldcw   cw
            mov     ecx, tp
            fld     qword ptr [ecx]
            fsub    d
            fstp    r
            fldcw   oldcw
        }
        return r;
    }
    return d;
}

// PANZERS 0x55b5c0
double HdSign(double v)
{
    if (0.0 > v)
        return -1.0;       // 0x7ea780
    if (v > 0.0)
        return 1.0;
    return 0.0;
}

// Recompile-only trace of the world random draws (PZ_M2_RNGLOG=1): logs
// "PZM2 RNG <frame> <caller address> <new seed>"; the exe links with
// /DYNAMICBASE:NO, so tools/addr2func.py --va resolves the caller.
void HdRngTrace(void* caller)
{
    static int s_On = -1;
    if (s_On < 0)
        s_On = getenv("PZ_M2_RNGLOG") ? 1 : 0;
    if (!s_On || !Logger.g)
        return;
    Logger.g->Log(0, "PZM2 RNG %d %p %08x", g_GameLogic ? g_GameLogic->Frame : -1, caller, g_World->RandomSeed);
}

// PANZERS 0x555a00
int HdRandInt(int range)
{
    int r = HdRandScale(HdLcg15(HdLcgStep(&g_World->RandomSeed)), range);
    HdRngTrace(_ReturnAddress());
    return r;
}

// PANZERS 0x559740
double HdRandDouble(double range)
{
    int r = HdLcg15(HdLcgStep(&g_World->RandomSeed));
    HdRngTrace(_ReturnAddress());
    __m128d v = _mm_mul_sd(_mm_set_sd((double)r), _mm_set_sd(kHdInv32768));
    return _mm_cvtsd_f64(_mm_mul_sd(v, _mm_set_sd(range)));
}

} // namespace pz
