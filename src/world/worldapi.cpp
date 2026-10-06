// src/world/worldapi.cpp
// Definitions of the globals in worldapi.h (owner P0).

#include "worldapi.h"

namespace pz {

SWorld*     g_World = nullptr;        // HD 0x929a50
SIScene*    g_Scene = nullptr;        // HD 0x929a54
SIPixie*    g_Pixie = nullptr;        // HD 0x929f14
SGameLogic* g_GameLogic = nullptr;    // HD 0x8f2078
SIScene*    g_WindowScene = nullptr;  // HD SDXWindow+0xe0

} // namespace pz
