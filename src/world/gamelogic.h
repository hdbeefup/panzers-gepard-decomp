// src/world/gamelogic.h
// pz::SGameLogic: HD SGameLogic (0x318 bytes, no vtable). OWNER: agent D.
// Skeleton from P0. In HD, SSuperWindow+0x1a0 holds it (was "MenuCamera").

#ifndef PZ_GAMELOGIC_H
#define PZ_GAMELOGIC_H

#include "pz/pzcommon.h"

namespace pz {

struct SIViewport;

struct SGameLogic {
    SGameLogic(int p1, int p2, int p3);   // 0x55e440 (menu: 0, -1, 0); sets g_GameLogic
    ~SGameLogic();                        // 0x55fe00 (non-virtual; caller deletes 0x318)

    void SetRunning(int running);         // 0x5802f0 (+0x04 = running; Concert +0x64 resume)
    int  Refresh();                       // 0x576d80, 20 Hz fixed step
    void UpdateUnitVisuals(SIViewport* vp, double interpolation);   // 0x5638f0 (per frame)

    unsigned char _00[0x318];
};
PZ_HD_SIZE(SGameLogic, kHdSizeSGameLogic);

} // namespace pz

#endif // PZ_GAMELOGIC_H
