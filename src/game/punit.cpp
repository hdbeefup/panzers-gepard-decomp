// src/game/punit.cpp
// pz::SPUnit skeleton (0x5a3810..0x5aa7b0). OWNER: agent U.

#include "punit.h"
#include "stub_log.h"

namespace pz {

SPUnit::SPUnit()
{
    PZ_M2_TRACE("SPUnit::SPUnit");
}

SPUnit::~SPUnit()
{
    STUB_LOG("SPUnit::~SPUnit (0x5a5900)");
    PZ_M2_TRACE("SPUnit::~SPUnit (0x5a5900)");
}

void SPUnit::LoadHeader(SProperties* props)
{
    STUB_LOG("SPUnit::LoadHeader (0x5a6650)");
    PZ_M2_TRACE("SPUnit::LoadHeader (0x5a6650)");
    (void)props;
}

void SPUnit::LoadResources(int p1)
{
    STUB_LOG("SPUnit::LoadResources (0x5a8780)");
    PZ_M2_TRACE("SPUnit::LoadResources (0x5a8780)");
    (void)p1;
}

void SPUnit::Slot_0C()
{
    STUB_LOG("SPUnit::Slot_0C (0x5a9950)");
    PZ_M2_TRACE("SPUnit::Slot_0C (0x5a9950)");
}

SIUnit* SPUnit::CreateUnit(int worldIndex)
{
    STUB_LOG("SPUnit::CreateUnit (pure in HD SPUnit)");
    PZ_M2_TRACE("SPUnit::CreateUnit (pure in HD SPUnit)");
    (void)worldIndex;
    return nullptr;
}

void SPUnit::LoadGunners(int p1)
{
    STUB_LOG("SPUnit::LoadGunners (0x5a7390)");
    PZ_M2_TRACE("SPUnit::LoadGunners (0x5a7390)");
    (void)p1;
}

} // namespace pz
