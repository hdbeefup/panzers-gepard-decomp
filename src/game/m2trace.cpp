// src/game/m2trace.cpp
// -m2 switch and the per-tick M2 trace (m2common.h). Recompile-only test
// hook; nothing here exists in the HD exe. OWNER: P0.

#include <stdlib.h>
#include <string.h>
#include "m2common.h"
#include "core_common.h"
#include "logger.h"

namespace pz {

// M2 is on by default since M2-I (the menu world CRC matches the original
// over three 90 s convoy cycles); "-nom2" or PZ_M2=0 turns it off.
SM2Switches g_M2 = { true, false, false };
unsigned g_M2Tick = 0;

static bool EnvFlag(const char* name, bool* value)
{
    const char* v = getenv(name);
    if (!v || !*v)
        return false;
    *value = !(v[0] == '0' && v[1] == 0);
    return true;
}

void M2ParseCommandLine(int* argc, char** argv)
{
    bool m2 = false, nom2 = false;
    int w = 1;
    for (int r = 1; r < *argc; ++r) {
        if (argv[r] && !_stricmp(argv[r], "-m2")) {
            m2 = true;
            continue;
        }
        if (argv[r] && !_stricmp(argv[r], "-nom2")) {
            nom2 = true;
            continue;
        }
        argv[w++] = argv[r];
    }
    *argc = w;

    bool v;
    if (m2) {
        g_M2.Enabled = true;
        g_M2.Trace = true;
    }
    if (nom2)
        g_M2.Enabled = false;
    if (EnvFlag("PZ_M2", &v))
        g_M2.Enabled = v;
    if (EnvFlag("PZ_M2_TRACE", &v))
        g_M2.Trace = v;
    if (EnvFlag("PZ_M2_CRC", &v))
        g_M2.Crc = v;
}

static bool IsTraceTick(unsigned tick)
{
    return tick < 3 || tick % 200 == 0;
}

void M2TraceCall(STraceSite* site, const char* what)
{
    unsigned tick = g_M2Tick;
    if (!IsTraceTick(tick))
        return;
    if (site->Frame != tick) {
        site->Frame = tick;
        site->Count = 0;
    }
    if (++site->Count > 4)
        return;
    if (Logger.g)
        Logger.g->Log(0, "PZM2 t%u: %s%s", tick, what, site->Count == 4 ? " (more this tick not logged)" : "");
}

void M2NextTick()
{
    ++g_M2Tick;
}

} // namespace pz
