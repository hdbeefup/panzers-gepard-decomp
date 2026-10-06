// src/world/ai.cpp
// SWorld AI and the mission start / map load extras (world.h block "M3").
// OWNER: agent L (docs/M3_INTERFACES.md). Skeleton stubs.

#include "world.h"
#include "m3common.h"
#include "stub_log.h"

namespace pz {

void SWorld::RefreshAI()
{
    STUB_LOG("SWorld::RefreshAI (0x5f5c70)");
    PZ_M3_TRACE("SWorld::RefreshAI (0x5f5c70)");
}

void SWorld::StartEffects()
{
    STUB_LOG("SWorld::StartEffects (0x5f5b50)");
    PZ_M3_TRACE("SWorld::StartEffects (0x5f5b50)");
}

void SWorld::InitCameraSpline(const char* file)
{
    STUB_LOG("SGameWorld::InitCameraSpline (0x609760)");
    PZ_M3_TRACE("SGameWorld::InitCameraSpline (0x609760)");
    (void)file;
}

void SWorld::LoadMapExtra_5e2d70()
{
    STUB_LOG("SWorld::LoadMapExtra_5e2d70 (0x5e2d70)");
    PZ_M3_TRACE("SWorld::LoadMapExtra_5e2d70 (0x5e2d70)");
}

void SWorld::LoadMapExtra_5debb0()
{
    STUB_LOG("SWorld::LoadMapExtra_5debb0 (0x5debb0)");
    PZ_M3_TRACE("SWorld::LoadMapExtra_5debb0 (0x5debb0)");
}

void SWorld::LoadMapExtra_607ad0()
{
    STUB_LOG("SWorld::LoadMapExtra_607ad0 (0x607ad0)");
    PZ_M3_TRACE("SWorld::LoadMapExtra_607ad0 (0x607ad0)");
}

void SWorld::FixBridges()
{
    STUB_LOG("SWorld::FixBridges (0x5e65f0)");
    PZ_M3_TRACE("SWorld::FixBridges (0x5e65f0)");
}

} // namespace pz
