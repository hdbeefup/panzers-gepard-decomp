// src/game/m3trace.cpp
// -m3 switch and the M3 call trace (m3common.h). Recompile-only test hook;
// nothing here exists in the HD exe. OWNER: P0.

#include <stdlib.h>
#include <string.h>
#include "m3common.h"
#include "core_common.h"
#include "logger.h"

namespace pz {

// Off by default: the default boot (3D menu + M2) must not change until the
// Training Camp path is playable (docs/M3_INTERFACES.md).
SM3Switches g_M3 = { false, false, true };

static bool EnvFlag(const char* name, bool* value)
{
    const char* v = getenv(name);
    if (!v || !*v)
        return false;
    *value = !(v[0] == '0' && v[1] == 0);
    return true;
}

void M3ParseCommandLine(int* argc, char** argv)
{
    bool m3 = false;
    int w = 1;
    for (int r = 1; r < *argc; ++r) {
        if (argv[r] && !_stricmp(argv[r], "-m3")) {
            m3 = true;
            continue;
        }
        argv[w++] = argv[r];
    }
    *argc = w;

    bool v;
    if (m3) {
        g_M3.Enabled = true;
        g_M3.Trace = true;
    }
    if (EnvFlag("PZ_M3", &v))
        g_M3.Enabled = v;
    if (EnvFlag("PZ_M3_TRACE", &v))
        g_M3.Trace = v;
    if (EnvFlag("PZ_M3_LOADMAP", &v))
        g_M3.LoadMap = v;
}

void M3TraceCall(STraceSite* site, const char* what)
{
    unsigned n = ++site->Count;
    if (n > 4 && n % 500 != 0)
        return;
    if (Logger.g)
        Logger.g->Log(0, "PZM3 #%u: %s", n, what);
}

} // namespace pz
