// src/game/gamelogic.cpp
// pz::SGameLogic skeleton. OWNER: agent L. Lift from the HD exe only.
// Moved from src/world in M2-P0. The default (M1) Refresh path is unchanged;
// -m2 / PZ_M2=1 runs RefreshM2, the logged skeleton of the HD single-player
// tick (see gamelogic.h for the order).

#include <string.h>
#include "gamelogic.h"
#include "worldapi.h"
#include "world.h"
#include "core_common.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

SGameLogic::SGameLogic(int p1, int p2, int p3)
{
    STUB_LOG("SGameLogic::SGameLogic (0x55e440)");
    PZ_TRACE("SGameLogic::SGameLogic (0x55e440)");
    (void)p1; (void)p2; (void)p3;
    memset(this, 0, sizeof(*this));
    g_GameLogic = this;   // HD 0x55e440: DAT_008f2078 = this
    if (g_M2.Enabled && Logger.g)
        Logger.g->Log(0, "PZM2: -m2 on, SGameLogic::Refresh runs the M2 skeleton (trace %s, crc %s)",
                      g_M2.Trace ? "on" : "off", g_M2.Crc ? "on" : "off");
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
    if (g_M2.Enabled) {
        RefreshM2();
        return 1;
    }
    // M1: only the model part of the tick (unit animations, doodad
    // interpolation state). Triggers, units, AI and the rest are M2.
    if (g_World)
        g_World->RefreshModels();
    return 0;
}

// Skeleton of the single-player path of 0x576d80. The unit loops stay out
// until agent U replaces the M1 stand-in units (they do not implement
// SIUnit); SWorld::RefreshModels keeps the M1 visuals running meanwhile.
void SGameLogic::RefreshM2()
{
    PZ_M2_TRACE("SGameLogic::RefreshM2 (0x576d80 single-player path)");
    Tick_578b00();
    Tick_578a70();
    ProcessPacket(Frame, 0);
    BeginFrame();
    Tick_579390();
    Tick_57dfe0();
    if (Frame % 20 == 0)
        DispatchEverySecond();
    RunTriggers();
    Tick_568af0();
    Tick_565e10(0);
    // HD: per live unit +0x16c StoreInterpolationState, +0x2c ServerRefresh(frame);
    // scene +0xf0(2); SWorld::RefreshFlyingFox; per unit +0x3c RefreshModel.
    if (g_World)
        g_World->RefreshModels();
    Tick_5822a0();
    if (g_M2.Crc && Logger.g)
        Logger.g->Log(0, "PZM2 CRC %d %08x %08x %d", Frame, ComputeWorldCRC(), 0u, 0);
    ++Frame;
    M2NextTick();
}

void SGameLogic::UpdateUnitVisuals(SIViewport* vp, double interpolation)
{
    STUB_LOG("SGameLogic::UpdateUnitVisuals (0x5638f0)");
    PZ_TRACE("SGameLogic::UpdateUnitVisuals (0x5638f0)");
    (void)vp; (void)interpolation;
}

void SGameLogic::Tick_578b00()
{
    STUB_LOG("SGameLogic::Tick_578b00 (0x578b00)");
    PZ_M2_TRACE("SGameLogic::Tick_578b00 (0x578b00)");
}

void SGameLogic::Tick_578a70()
{
    STUB_LOG("SGameLogic::Tick_578a70 (0x578a70)");
    PZ_M2_TRACE("SGameLogic::Tick_578a70 (0x578a70)");
}

void SGameLogic::ProcessPacket(int frame, int p2)
{
    STUB_LOG("SGameLogic::ProcessPacket (0x5737c0)");
    PZ_M2_TRACE("SGameLogic::ProcessPacket (0x5737c0)");
    (void)frame; (void)p2;
}

void SGameLogic::BeginFrame()
{
    STUB_LOG("SGameLogic::BeginFrame (0x571840)");
    PZ_M2_TRACE("SGameLogic::BeginFrame (0x571840)");
}

unsigned SGameLogic::ComputeWorldCRC()
{
    STUB_LOG("SGameLogic::ComputeWorldCRC (0x56aa10)");
    PZ_M2_TRACE("SGameLogic::ComputeWorldCRC (0x56aa10)");
    return 0;
}

void SGameLogic::Tick_579390()
{
    STUB_LOG("SGameLogic::Tick_579390 (0x579390)");
    PZ_M2_TRACE("SGameLogic::Tick_579390 (0x579390)");
}

void SGameLogic::Tick_57dfe0()
{
    STUB_LOG("SGameLogic::Tick_57dfe0 (0x57dfe0)");
    PZ_M2_TRACE("SGameLogic::Tick_57dfe0 (0x57dfe0)");
}

void SGameLogic::Tick_568af0()
{
    STUB_LOG("SGameLogic::Tick_568af0 (0x568af0)");
    PZ_M2_TRACE("SGameLogic::Tick_568af0 (0x568af0)");
}

void SGameLogic::Tick_565e10(int p1)
{
    STUB_LOG("SGameLogic::Tick_565e10 (0x565e10)");
    PZ_M2_TRACE("SGameLogic::Tick_565e10 (0x565e10)");
    (void)p1;
}

void SGameLogic::Tick_5822a0()
{
    STUB_LOG("SGameLogic::Tick_5822a0 (0x5822a0)");
    PZ_M2_TRACE("SGameLogic::Tick_5822a0 (0x5822a0)");
}

int SGameLogic::GetFrame()
{
    STUB_LOG("SGameLogic::GetFrame (0x56d1a0)");
    PZ_M2_TRACE("SGameLogic::GetFrame (0x56d1a0)");
    return Frame;
}

bool SGameLogic::IsPaused()
{
    STUB_LOG("SGameLogic::IsPaused (0x56e150)");
    PZ_M2_TRACE("SGameLogic::IsPaused (0x56e150)");
    return Flag288 || Flag2b8;
}

bool SGameLogic::CanSeeGroundUnit(int player, SIUnit* unit)
{
    STUB_LOG("SGameLogic::CanSeeGroundUnit (0x562760)");
    PZ_M2_TRACE("SGameLogic::CanSeeGroundUnit (0x562760)");
    (void)player; (void)unit;
    return true;
}

} // namespace pz
