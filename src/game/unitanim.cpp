// src/game/unitanim.cpp
// pz::SUnitAnimation and pz::SPUnitAnimation (0x5c6300..0x5cfb00). OWNER: agent A.

#include "unitanim.h"
#include "stub_log.h"

namespace pz {

SUnitAnimation::SUnitAnimation()
{
    PZ_M2_TRACE("SUnitAnimation::SUnitAnimation");
}

SUnitAnimation::~SUnitAnimation()
{
    STUB_LOG("SUnitAnimation::~SUnitAnimation (0x5c74c0)");
    PZ_M2_TRACE("SUnitAnimation::~SUnitAnimation (0x5c74c0)");
}

void SUnitAnimation::InitModel(int p1)
{
    STUB_LOG("SUnitAnimation::InitModel (pure in HD SUnitAnimation)");
    PZ_M2_TRACE("SUnitAnimation::InitModel (pure in HD SUnitAnimation)");
    (void)p1;
}

void SUnitAnimation::UpdateModel()
{
    STUB_LOG("SUnitAnimation::UpdateModel (pure in HD SUnitAnimation)");
    PZ_M2_TRACE("SUnitAnimation::UpdateModel (pure in HD SUnitAnimation)");
}

void SUnitAnimation::Slot_0C()
{
    STUB_LOG("SUnitAnimation::Slot_0C (0x5c76c0)");
    PZ_M2_TRACE("SUnitAnimation::Slot_0C (0x5c76c0)");
}

void SUnitAnimation::Slot_10()
{
    STUB_LOG("SUnitAnimation::Slot_10 (pure in HD SUnitAnimation)");
    PZ_M2_TRACE("SUnitAnimation::Slot_10 (pure in HD SUnitAnimation)");
}

void SUnitAnimation::Slot_14()
{
    STUB_LOG("SUnitAnimation::Slot_14 (0x5cb420)");
    PZ_M2_TRACE("SUnitAnimation::Slot_14 (0x5cb420)");
}

float SUnitAnimation::GetStateMoveSpeed(int state)
{
    STUB_LOG("SUnitAnimation::GetStateMoveSpeed (0x5c7e90)");
    PZ_M2_TRACE("SUnitAnimation::GetStateMoveSpeed (0x5c7e90)");
    (void)state;
    return 0.0f;
}

float SUnitAnimation::GetStateTurnSpeed(int state)
{
    STUB_LOG("SUnitAnimation::GetStateTurnSpeed (0x5c7f50)");
    PZ_M2_TRACE("SUnitAnimation::GetStateTurnSpeed (0x5c7f50)");
    (void)state;
    return 0.0f;
}

void* SUnitAnimation::GetRunningGear()
{
    STUB_LOG("SUnitAnimation::GetRunningGear (0x5c7c60)");
    PZ_M2_TRACE("SUnitAnimation::GetRunningGear (0x5c7c60)");
    return nullptr;
}

void SUnitAnimation::Slot_24()
{
    STUB_LOG("SUnitAnimation::Slot_24 (0x5c8410)");
    PZ_M2_TRACE("SUnitAnimation::Slot_24 (0x5c8410)");
}

void SUnitAnimation::Slot_28()
{
    STUB_LOG("SUnitAnimation::Slot_28 (0x5c8350)");
    PZ_M2_TRACE("SUnitAnimation::Slot_28 (0x5c8350)");
}

void SUnitAnimation::Slot_2C()
{
    STUB_LOG("SUnitAnimation::Slot_2C (0x5c83b0)");
    PZ_M2_TRACE("SUnitAnimation::Slot_2C (0x5c83b0)");
}

void SUnitAnimation::Slot_30()
{
    STUB_LOG("SUnitAnimation::Slot_30 (0x5c8430)");
    PZ_M2_TRACE("SUnitAnimation::Slot_30 (0x5c8430)");
}

void SUnitAnimation::Slot_34()
{
    STUB_LOG("SUnitAnimation::Slot_34 (0x5c8370)");
    PZ_M2_TRACE("SUnitAnimation::Slot_34 (0x5c8370)");
}

void SUnitAnimation::Slot_38()
{
    STUB_LOG("SUnitAnimation::Slot_38 (0x5c83d0)");
    PZ_M2_TRACE("SUnitAnimation::Slot_38 (0x5c83d0)");
}

int SUnitAnimation::GetAttachNode()
{
    STUB_LOG("SUnitAnimation::GetAttachNode (0x5c8250)");
    PZ_M2_TRACE("SUnitAnimation::GetAttachNode (0x5c8250)");
    return 0;
}

SIModel* SUnitAnimation::GetModel()
{
    STUB_LOG("SUnitAnimation::GetModel (0x5c8240)");
    PZ_M2_TRACE("SUnitAnimation::GetModel (0x5c8240)");
    return nullptr;
}

SPUnitAnimation::SPUnitAnimation()
{
    PZ_M2_TRACE("SPUnitAnimation::SPUnitAnimation");
}

SPUnitAnimation::~SPUnitAnimation()
{
    STUB_LOG("SPUnitAnimation::~SPUnitAnimation (0x5c7290)");
    PZ_M2_TRACE("SPUnitAnimation::~SPUnitAnimation (0x5c7290)");
}

void SPUnitAnimation::Load(SProperties* props)
{
    STUB_LOG("SPUnitAnimation::Load (pure in HD SPUnitAnimation)");
    PZ_M2_TRACE("SPUnitAnimation::Load (pure in HD SPUnitAnimation)");
    (void)props;
}

void SPUnitAnimation::LoadResources(int p1, int p2)
{
    STUB_LOG("SPUnitAnimation::LoadResources (0x5ca680)");
    PZ_M2_TRACE("SPUnitAnimation::LoadResources (0x5ca680)");
    (void)p1;
    (void)p2;
}

void SPUnitAnimation::Slot_0C()
{
    STUB_LOG("SPUnitAnimation::Slot_0C (0x5cb470)");
    PZ_M2_TRACE("SPUnitAnimation::Slot_0C (0x5cb470)");
}

SIUnitAnimation* SPUnitAnimation::CreateAnimation(SIUnit* unit)
{
    STUB_LOG("SPUnitAnimation::CreateAnimation (pure in HD SPUnitAnimation)");
    PZ_M2_TRACE("SPUnitAnimation::CreateAnimation (pure in HD SPUnitAnimation)");
    (void)unit;
    return nullptr;
}

} // namespace pz
