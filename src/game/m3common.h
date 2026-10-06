// src/game/m3common.h
// Shared definitions for M3 (main menu -> Training Camp mission -> main menu,
// docs/M3_INTERFACES.md): HD object sizes, the -m3 switch and the M3 trace.
//
// SHARED HEADER (owner P0). Changes are additive and go through P0 or the
// integrator.

#ifndef PZ_M3COMMON_H
#define PZ_M3COMMON_H

#include "m2common.h"

namespace pz {

// HD object sizes (operator new at the creation sites in PANZERS.exe).
enum M3HdSize : unsigned {
    kHdSizeSPanzersCampaign   = 0xb8c,    // OnAction 0x659250 / Play 0x65b470, ctor 0x590ec0
    kHdSizeSGameView          = 0x3e98,   // LoadNextCampaignView 0x658b10 case 2, ctor 0x6181f0
    kHdSizeSMarket            = 0x25b4,   // 0x658b10 case 1, ctor 0x63f2f0 (delete size in 0x6400b0)
    kHdSizeSTrainingMenu      = 0x288,    // OnAction 0x4d4d4, ctor 0x633b30
    kHdSizeSBriefingMenu      = 0x340,    // 0x658b10 case 0, ctor 0x632f40
    kHdSizeSResultsMenu       = 0x1228,   // 0x658b10 case 3, ctor 0x633540
    kHdSizeSStreamBuffer      = 0x30,     // ctor 0x65cd20 (packet frames, replay)
    kHdSizeSDXWidget          = 0x58,     // HD SDXWidget ctor 0x5398f0 writes up to +0x54
};

// Runtime switch (src/game/m3trace.cpp). Recompile-only test hook, default OFF.
//   Enabled: the Training Camp button runs the M3 path (STrainingMenu ->
//            SPanzersCampaign -> SGameView -> SMarket -> mission start ->
//            Esc menu -> End Mission -> main menu). Off: the button is the
//            logged stub it was before M3. "-m3" or PZ_M3=1 turns it on.
//   Trace:   call trace of the M3 skeleton ("PZM3 <what>"). "-m3" turns it
//            on; PZ_M3_TRACE=0/1 overrides it.
//   LoadMap: SGameView::LoadMap really loads the map into an SWorld (the M1/M2
//            loader). Default on with -m3; PZ_M3_LOADMAP=0 skips the load and
//            keeps an empty view (for UI work on the screens only).
struct SM3Switches {
    bool Enabled;
    bool Trace;
    bool LoadMap;
};
extern SM3Switches g_M3;

// Reads the environment and removes "-m3" from argv. Call once, before
// SSettings parses the command line.
void M3ParseCommandLine(int* argc, char** argv);

// M3 trace: each call site logs its first 4 calls, then every 500th call
// ("PZM3 #<n>: <what>"). The mission UI calls these per frame, so the
// per-tick scheme of PZ_M2_TRACE would flood the log.
void M3TraceCall(STraceSite* site, const char* what);

} // namespace pz

#define PZ_M3_TRACE(WHAT)                                             \
    do {                                                              \
        if (pz::g_M3.Trace) {                                         \
            static pz::STraceSite s_pzM3Site = { 0u, 0 };             \
            pz::M3TraceCall(&s_pzM3Site, WHAT);                       \
        }                                                             \
    } while (0)

// An M3 skeleton stub starts with two lines (tools/census.py counts the
// STUB_LOG line of the M3 files as "m3 skeleton"):
//     STUB_LOG("Class::Name (0xADDR)");
//     PZ_M3_TRACE("Class::Name (0xADDR)");
// When the body is lifted, drop the STUB_LOG line, keep the trace and put
// "// PANZERS 0xADDR" above the function.

#endif // PZ_M3COMMON_H
