// src/stubs/stubs.cpp
// Function stubs for symbols referenced by the imported SWINE engine code
// whose real definitions are NOT in this repo: they live in SWINE gameplay
// code (game/game.cpp, world/world.cpp) or in libraries we do not build.
// None of this is copied from those files; each body is a minimal stand-in.
// See docs/IMPORT_NOTES.md for the full cut list.
//
// Rule: every function stub begins with STUB_LOG("Name") (logs once, and is
// what tools/census.py counts). Data stubs live in stub_globals.cpp.

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "stub_log.h"
#include "string2.h"
#include "logger.h"

struct IDirect3DDevice9;

void StubLogFirstCall(const char* name)
{
    char buf[256];
    _snprintf(buf, sizeof(buf) - 1, "STUB: %s called (not implemented)\n", name);
    buf[sizeof(buf) - 1] = '\0';
    OutputDebugStringA(buf);
    fputs(buf, stderr);
    // Also into the game log, so a run's stub hits can be read from the log.
    if (Logger.g)
        Logger.g->Log(0, "STUB: %s called (not implemented)", name);
}

// ---------------------------------------------------------------------------
// zstd — used only by 3dengine/animation.cpp for the SWINE-HD-only "ANI2"
// compressed animation format. Panzers ships classic data, so zstd is not
// built; returning an error makes the caller treat the ANI2 file as corrupt.
// ---------------------------------------------------------------------------
extern "C" int ZSTD_decompress(void* dst, int dstCapacity, const void* src, int srcSize)
{
    STUB_LOG("ZSTD_decompress");
    (void)dst; (void)dstCapacity; (void)src; (void)srcSize;
    return -1;
}

// ---------------------------------------------------------------------------
// DXGetErrorStringA — declared extern "C" (cdecl) by sound/concert.h, so the
// June 2010 dxerr.lib (__stdcall, _DXGetErrorStringA@4) does not satisfy it.
// SWINE defined it in game/game.cpp. Here: the HRESULT as hex.
// ---------------------------------------------------------------------------
extern "C" const char* DXGetErrorStringA(HRESULT hr)
{
    STUB_LOG("DXGetErrorStringA");
    static char buf[32];
    _snprintf(buf, sizeof(buf) - 1, "HRESULT 0x%08X", (unsigned)hr);
    buf[sizeof(buf) - 1] = '\0';
    return buf;
}

// ---------------------------------------------------------------------------
// MpegAudioPrecalculate — builds the mdec MP3 decoder tables (mdec/decode.h).
// SWINE kept it and its *Precalculate helpers in game/game.cpp. Here: no-op;
// mdec's PrecalculateCalled stays 0, so creating an SMpegAudioDecoder
// (streamed MP3 music) will Panic until the real routine is lifted.
// ---------------------------------------------------------------------------
void MpegAudioPrecalculate()
{
    STUB_LOG("MpegAudioPrecalculate");
}

// ---------------------------------------------------------------------------
// DrawDebugPickerOverlayFromGepard — SWINE editor/debug overlay bridge,
// defined in world/world.cpp, called from SGepard::RenderScene. No-op.
// ---------------------------------------------------------------------------
extern "C" void DrawDebugPickerOverlayFromGepard(IDirect3DDevice9* dev)
{
    STUB_LOG("DrawDebugPickerOverlayFromGepard");
    (void)dev;
}
