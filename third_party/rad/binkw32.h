// third_party/rad/binkw32.h
//
// HAND-WRITTEN declarations for the subset of the Bink API (binkw32.dll,
// 1.5q shipped with Panzers HD) that PANZERS.exe imports. This is NOT the RAD
// Game Tools SDK header and contains no SDK code. See README.md.
//
// Names and stdcall argument byte counts come from the decorated IAT names in
// PANZERS.exe (HD) / the binkw32.dll exports (e.g. `_BinkCopyToBuffer@28`);
// argument meaning comes from the call sites in SSuperWindow::LoadBinkVideo
// 0x657ef0, the frame step 0x65ba00 and the close 0x65b830.
//
// As with mss.h, the DLL exports the names with a leading underscore, so the
// functions are declared as `_BinkX` and #defined back to `BinkX`.

#ifndef THIRD_PARTY_RAD_BINKW32_H
#define THIRD_PARTY_RAD_BINKW32_H

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int          S32;
typedef unsigned int U32;

// Only the leading fields Panzers reads are declared. Offsets are from the
// PANZERS.exe accesses: BINK +0x8 (frame count) is compared with +0xC (current
// frame) in 0x65ba00 to detect the end of the video; BINKBUFFER +0x4/+0x10/
// +0x14/+0x18 are passed to BinkCopyToBuffer in 0x65ba00.
typedef struct BINK {
    U32 Width;      // +0x00 (logged "[BINK] resolutions: Bink %dx%d")
    U32 Height;     // +0x04
    U32 Frames;     // +0x08
    U32 FrameNum;   // +0x0C (1-based current frame)
    // ... more fields owned by binkw32.dll
} BINK, *HBINK;

typedef struct BINKBUFFER {
    U32   Width;         // +0x00
    U32   Height;        // +0x04
    U32   WindowWidth;   // +0x08
    U32   WindowHeight;  // +0x0C
    U32   SurfaceType;   // +0x10
    void* Buffer;        // +0x14 (valid between BinkBufferLock/Unlock)
    S32   BufferPitch;   // +0x18
    // ... more fields owned by binkw32.dll
} BINKBUFFER, *HBINKBUFFER;

// BinkSetSoundSystem(BinkOpenMiles, (U32)HDIGDRIVER)
typedef void* (__stdcall* BINKSNDSYSOPEN)(U32 param);

#define RADEXPLINK __stdcall
#define BINKIMPORT __declspec(dllimport)

BINKIMPORT HBINK       RADEXPLINK _BinkOpen(char const* name, U32 flags);
BINKIMPORT void        RADEXPLINK _BinkClose(HBINK bink);
BINKIMPORT S32         RADEXPLINK _BinkDoFrame(HBINK bink);
BINKIMPORT void        RADEXPLINK _BinkNextFrame(HBINK bink);
BINKIMPORT S32         RADEXPLINK _BinkWait(HBINK bink);
BINKIMPORT S32         RADEXPLINK _BinkCopyToBuffer(HBINK bink, void* dest, S32 destpitch, U32 destheight,
                                                    U32 destx, U32 desty, U32 flags);
BINKIMPORT S32         RADEXPLINK _BinkSetSoundSystem(BINKSNDSYSOPEN open, U32 param);
BINKIMPORT void*       RADEXPLINK _BinkOpenMiles(U32 param);

BINKIMPORT HBINKBUFFER RADEXPLINK _BinkBufferOpen(HWND wnd, U32 width, U32 height, U32 bufferflags);
BINKIMPORT void        RADEXPLINK _BinkBufferClose(HBINKBUFFER buf);
BINKIMPORT S32         RADEXPLINK _BinkBufferLock(HBINKBUFFER buf);
BINKIMPORT S32         RADEXPLINK _BinkBufferUnlock(HBINKBUFFER buf);
BINKIMPORT void        RADEXPLINK _BinkBufferSetResolution(S32 w, S32 h, S32 bits);
BINKIMPORT S32         RADEXPLINK _BinkBufferSetScale(HBINKBUFFER buf, U32 w, U32 h);
BINKIMPORT void        RADEXPLINK _BinkBufferBlit(HBINKBUFFER buf, void* rects, U32 numrects);

#ifdef __cplusplus
}
#endif

#define BinkOpen                _BinkOpen
#define BinkClose               _BinkClose
#define BinkDoFrame             _BinkDoFrame
#define BinkNextFrame           _BinkNextFrame
#define BinkWait                _BinkWait
#define BinkCopyToBuffer        _BinkCopyToBuffer
#define BinkSetSoundSystem      _BinkSetSoundSystem
#define BinkOpenMiles           _BinkOpenMiles
#define BinkBufferOpen          _BinkBufferOpen
#define BinkBufferClose         _BinkBufferClose
#define BinkBufferLock          _BinkBufferLock
#define BinkBufferUnlock        _BinkBufferUnlock
#define BinkBufferSetResolution _BinkBufferSetResolution
#define BinkBufferSetScale      _BinkBufferSetScale
#define BinkBufferBlit          _BinkBufferBlit

#endif // THIRD_PARTY_RAD_BINKW32_H
