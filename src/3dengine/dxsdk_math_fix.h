// dxsdk_math_fix.h
// Workaround: June 2010 DirectX SDK d3dx9math.inl uses sqrtf/sinf/cosf/etc.
// in the global namespace. MSVC 14.50+ UCRT corecrt_math.h defines them only
// under C compilation or specific arch macros. We declare them explicitly.
#pragma once

#include <corecrt_math.h>

// Verify sqrtf is available; if not, provide inline wrappers
#if !defined(sqrtf) && !defined(_M_X64) && !defined(_M_ARM) && !defined(_M_ARM64)
// On x86, corecrt_math.h provides sqrtf as inline wrapping sqrt(double).
// If it's somehow still missing, declare it:
extern "C" {
    float __cdecl sqrtf(float);
    float __cdecl sinf(float);
    float __cdecl cosf(float);
    float __cdecl tanf(float);
    float __cdecl acosf(float);
    float __cdecl asinf(float);
    float __cdecl atanf(float);
    float __cdecl atan2f(float, float);
    float __cdecl fabsf(float);
    float __cdecl floorf(float);
    float __cdecl ceilf(float);
    float __cdecl fmodf(float, float);
    float __cdecl powf(float, float);
    float __cdecl expf(float);
    float __cdecl logf(float);
    float __cdecl log10f(float);
    float __cdecl sinhf(float);
    float __cdecl coshf(float);
    float __cdecl tanhf(float);
    float __cdecl modff(float, float*);
}
#endif
