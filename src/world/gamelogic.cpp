// src/world/gamelogic.cpp
// pz::SGameLogic skeleton. OWNER: agent D. Lift from the HD exe only.

#include <string.h>
#include "gamelogic.h"
#include "worldapi.h"
#include "stub_log.h"

namespace pz {

SGameLogic::SGameLogic(int p1, int p2, int p3)
{
    STUB_LOG("SGameLogic::SGameLogic (0x55e440)");
    PZ_TRACE("SGameLogic::SGameLogic (0x55e440)");
    (void)p1; (void)p2; (void)p3;
    memset(_00, 0, sizeof(_00));
    g_GameLogic = this;   // HD 0x55e440: DAT_008f2078 = this
}

SGameLogic::~SGameLogic()
{
    STUB_LOG("SGameLogic::~SGameLogic (0x55fe00)");
    PZ_TRACE("SGameLogic::~SGameLogic (0x55fe00)");
    if (g_GameLogic == this)
        g_GameLogic = nullptr;   // HD 0x55fe00: DAT_008f2078 = 0
}

void SGameLogic::SetRunning(int running)
{
    STUB_LOG("SGameLogic::SetRunning (0x5802f0)");
    PZ_TRACE("SGameLogic::SetRunning (0x5802f0)");
    (void)running;
}

int SGameLogic::Refresh()
{
    STUB_LOG("SGameLogic::Refresh (0x576d80)");
    PZ_TRACE("SGameLogic::Refresh (0x576d80)");
    return 0;
}

void SGameLogic::UpdateUnitVisuals(SIViewport* vp, double interpolation)
{
    STUB_LOG("SGameLogic::UpdateUnitVisuals (0x5638f0)");
    PZ_TRACE("SGameLogic::UpdateUnitVisuals (0x5638f0)");
    (void)vp; (void)interpolation;
}

} // namespace pz
