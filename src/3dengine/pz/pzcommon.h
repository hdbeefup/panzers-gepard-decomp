// src/3dengine/pz/pzcommon.h
// Shared definitions for the HD 3D path behind the main menu (M1, see
// docs/MENU3D_INTERFACES.md): HD object sizes, the -menu3d switches and the
// per-call trace used by every interface stub.
//
// Everything in src/3dengine/pz and src/world lives in namespace pz, because
// SWINE's 3dengine already has global STerrain, SParcel, SMesh, SEffect, ...
// with different layouts.

#ifndef PZ_PZCOMMON_H
#define PZ_PZCOMMON_H

#include <stddef.h>

namespace pz {

// HD object sizes (operator new / operator delete sizes in PANZERS.exe).
enum HdSize : unsigned {
    kHdSizeSGepard    = 0x810,   // ::CreateGepard 0x678ba0
    kHdSizeSViewport  = 0x244,   // SGepard::Initialize 0x67d1c0
    kHdSizeSScene     = 0x2b0,   // SGepard CreateScene 0x678ed0, SScene::Release 0x6ac7f0
    kHdSizeSModel     = 0x150,   // SScene::CreateModel 0x6a89d0
    kHdSizeSPModel    = 0x54,    // SGepard PurgeModelPrototypes 0x678210
    kHdSizeSTerrain   = 0x11c88, // SScene CreateTerrain 0x6a9dd0 / DestroyTerrain 0x6aab20
    kHdSizeSPixie     = 0x7c,    // SGepard::Initialize 0x67d1c0, SPixie::Release 0x69e8a0
    kHdSizeSWorld     = 0x7538,  // SSuperWindow::LoadMenuBackground 0x658690
    kHdSizeSGameLogic = 0x318,   // SSuperWindow::LoadMenuBackground 0x658690
};

// Runtime switches (src/3dengine/pz/pztrace.cpp).
//   World: run the menu world (SWorld + LoadMap + SGameLogic) in
//          SSuperWindow::LoadMenuBackground, as HD always does. Default =
//          the PZ_MENU_WORLD CMake option (ON); "-nomenu3d" or PZ_MENU3D=0
//          turns it off, "-menu3d" or PZ_MENU3D=1 turns it on.
//   Trace: log every 3D interface call (rate-limited). "-menu3d" or
//          PZ_MENU3D_TRACE=1 turns it on; PZ_MENU3D_TRACE=0 turns it off.
struct SMenu3DSwitches {
    bool World;
    bool Trace;
};
extern SMenu3DSwitches g_Menu3D;

// Reads the environment and removes "-menu3d" / "-nomenu3d" from argv (recompile-only
// switch; HD would read an unknown argument as a map path). Call once,
// before SSettings parses the command line.
void Menu3DParseCommandLine(int* argc, char** argv);

// Per-call trace. Frame = one SViewport::Render. A call site is logged on
// trace frames only (the first 3 frames, then every 300th) and at most 4
// times per frame, so per-frame calls stay readable.
struct STraceSite {
    unsigned Frame;
    unsigned Count;
};
extern unsigned g_TraceFrame;
void TraceCall(STraceSite* site, const char* what);
void TraceNextFrame();

} // namespace pz

#define PZ_TRACE(WHAT)                                                \
    do {                                                              \
        if (pz::g_Menu3D.Trace) {                                     \
            static pz::STraceSite s_pzTraceSite = { 0xffffffffu, 0 }; \
            pz::TraceCall(&s_pzTraceSite, WHAT);                      \
        }                                                             \
    } while (0)

// Size check for a class that mirrors an HD object. Always on: the HD size is
// the contract; keep unknown bytes as padding until they are lifted.
#define PZ_HD_SIZE(T, SIZE) \
    static_assert(sizeof(T) == (SIZE), #T " must keep the HD object size")

#endif // PZ_PZCOMMON_H
