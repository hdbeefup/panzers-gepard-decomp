// src/3dengine/pz/pztrace.cpp
// -menu3d switches and the rate-limited per-call trace (pzcommon.h).
// Recompile-only test hook; nothing here exists in the HD exe.

#include <stdlib.h>
#include <string.h>
#include "pzcommon.h"
#include "core_common.h"
#include "logger.h"

#ifndef PZ_MENU_WORLD
#define PZ_MENU_WORLD 0
#endif

namespace pz {

SMenu3DSwitches g_Menu3D = { PZ_MENU_WORLD != 0, false };
unsigned g_TraceFrame = 0;

static bool EnvFlag(const char* name, bool* value)
{
    const char* v = getenv(name);
    if (!v || !*v)
        return false;
    *value = !(v[0] == '0' && v[1] == 0);
    return true;
}

void Menu3DParseCommandLine(int* argc, char** argv)
{
    bool menu3d = false, nomenu3d = false;
    int w = 1;
    for (int r = 1; r < *argc; ++r) {
        if (argv[r] && !_stricmp(argv[r], "-menu3d")) {
            menu3d = true;
            continue;
        }
        if (argv[r] && !_stricmp(argv[r], "-nomenu3d")) {
            nomenu3d = true;
            continue;
        }
        argv[w++] = argv[r];
    }
    *argc = w;

    bool v;
    if (menu3d) {
        g_Menu3D.World = true;
        g_Menu3D.Trace = true;
    }
    if (nomenu3d)
        g_Menu3D.World = false;
    if (EnvFlag("PZ_MENU3D", &v))
        g_Menu3D.World = v;
    if (EnvFlag("PZ_MENU3D_TRACE", &v))
        g_Menu3D.Trace = v;
}

static bool IsTraceFrame(unsigned frame)
{
    return frame < 3 || frame % 300 == 0;
}

void TraceCall(STraceSite* site, const char* what)
{
    unsigned frame = g_TraceFrame;
    if (!IsTraceFrame(frame))
        return;
    if (site->Frame != frame) {
        site->Frame = frame;
        site->Count = 0;
    }
    if (++site->Count > 4)
        return;
    if (Logger.g)
        Logger.g->Log(0, "PZ3D f%u: %s%s", frame, what, site->Count == 4 ? " (more this frame not logged)" : "");
}

void TraceNextFrame()
{
    ++g_TraceFrame;
}

} // namespace pz
