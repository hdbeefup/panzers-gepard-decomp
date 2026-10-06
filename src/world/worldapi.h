// src/world/worldapi.h
// Globals that connect the Panzers world layer (src/world, lifted from the HD
// exe only; SWINE world/ and game/ are never used) to the 3D interfaces
// (src/3dengine/pz) and to SSuperWindow.
//
// SHARED HEADER (owner P0, see docs/MENU3D_INTERFACES.md).

#ifndef PZ_WORLDAPI_H
#define PZ_WORLDAPI_H

#include "pz/pzcommon.h"

namespace pz {

struct SIScene;
struct SIPixie;
struct SWorld;
struct SGameLogic;

extern SWorld*     g_World;        // HD 0x929a50 (SWorld ctor 0x5d2f90 sets it)
extern SIScene*    g_Scene;        // HD 0x929a54 (SWorld ctor: Gepard CreateScene)
extern SIPixie*    g_Pixie;        // HD 0x929f14 (SSuperWindow::Initialize: Gepard +0x5c)
extern SGameLogic* g_GameLogic;    // HD 0x8f2078 (SGameLogic ctor 0x55e440 sets it)

// HD SDXWindow+0xe0: the scene SDXWindow::OnIdle 0x53a0d0 renders every
// frame (viewport +0x50). The SWINE SDXWindow has no such field, so the
// recompile keeps it here. Holds a reference (AddRef/Release).
extern SIScene*    g_WindowScene;

} // namespace pz

#endif // PZ_WORLDAPI_H
