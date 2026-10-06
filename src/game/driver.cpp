// src/game/driver.cpp
// pz::SDriver and pz::SPDriver (0x5500f0..0x55c900). OWNER: agent P.

#include "driver.h"
#include "stub_log.h"

namespace pz {

SDriver::SDriver()
{
    PZ_M2_TRACE("SDriver::SDriver");
}

SDriver::~SDriver()
{
    STUB_LOG("SDriver::~SDriver (0x550120)");
    PZ_M2_TRACE("SDriver::~SDriver (0x550120)");
}

SIPDriver* SDriver::GetPDriver()
{
    STUB_LOG("SDriver::GetPDriver (0x5531d0)");
    PZ_M2_TRACE("SDriver::GetPDriver (0x5531d0)");
    return nullptr;
}

void SDriver::RefreshTarget(int p1)
{
    STUB_LOG("SDriver::RefreshTarget (0x557bf0)");
    PZ_M2_TRACE("SDriver::RefreshTarget (0x557bf0)");
    (void)p1;
}

void SDriver::Slot_0C()
{
    STUB_LOG("SDriver::Slot_0C (0x55c020)");
    PZ_M2_TRACE("SDriver::Slot_0C (0x55c020)");
}

void SDriver::Init()
{
    STUB_LOG("SDriver::Init (0x555a50)");
    PZ_M2_TRACE("SDriver::Init (0x555a50)");
}

void SDriver::Refresh()
{
    STUB_LOG("SDriver::Refresh (0x559ab0)");
    PZ_M2_TRACE("SDriver::Refresh (0x559ab0)");
}

void SDriver::MoveTowardNextWayPoint(int p1)
{
    STUB_LOG("SDriver::MoveTowardNextWayPoint (0x5582a0)");
    PZ_M2_TRACE("SDriver::MoveTowardNextWayPoint (0x5582a0)");
    (void)p1;
}

void SDriver::Slot_1C()
{
    STUB_LOG("SDriver::Slot_1C (0x55c3f0)");
    PZ_M2_TRACE("SDriver::Slot_1C (0x55c3f0)");
}

void SDriver::Slot_20()
{
    STUB_LOG("SDriver::Slot_20 (0x55c530)");
    PZ_M2_TRACE("SDriver::Slot_20 (0x55c530)");
}

bool SDriver::GhostStepStraight(SGhostFrame* frame, int p2, int p3)
{
    STUB_LOG("SDriver::GhostStepStraight (0x557b30)");
    PZ_M2_TRACE("SDriver::GhostStepStraight (0x557b30)");
    (void)frame;
    (void)p2;
    (void)p3;
    return false;
}

bool SDriver::GhostStepTowards(SGhostFrame* frame, float x, float z)
{
    STUB_LOG("SDriver::GhostStepTowards (0x559360)");
    PZ_M2_TRACE("SDriver::GhostStepTowards (0x559360)");
    (void)frame;
    (void)x;
    (void)z;
    return false;
}

int SDriver::GhostTurnTowards(SGhostFrame* frame, float x, float z)
{
    STUB_LOG("SDriver::GhostTurnTowards (0x55c230)");
    PZ_M2_TRACE("SDriver::GhostTurnTowards (0x55c230)");
    (void)frame;
    (void)x;
    (void)z;
    return 0;
}

int SDriver::FindGlobalPath(STarget* target)
{
    STUB_LOG("SDriver::FindGlobalPath (0x551cb0)");
    PZ_M2_TRACE("SDriver::FindGlobalPath (0x551cb0)");
    (void)target;
    return 0;
}

int SDriver::FindLocalPath(STarget* target)
{
    STUB_LOG("SDriver::FindLocalPath (0x552050)");
    PZ_M2_TRACE("SDriver::FindLocalPath (0x552050)");
    (void)target;
    return 0;
}

unsigned SDriver::RefreshGhost()
{
    STUB_LOG("SDriver::RefreshGhost (0x555450)");
    PZ_M2_TRACE("SDriver::RefreshGhost (0x555450)");
    return 0;
}

int SDriver::PredictGhost(float* pos, int* p2, float* dir, float* speed)
{
    STUB_LOG("SDriver::PredictGhost (0x554870)");
    PZ_M2_TRACE("SDriver::PredictGhost (0x554870)");
    (void)pos;
    (void)p2;
    (void)dir;
    (void)speed;
    return 0;
}

void SDriver::Ghost_FirstStep()
{
    STUB_LOG("SDriver::Ghost_FirstStep (0x553700)");
    PZ_M2_TRACE("SDriver::Ghost_FirstStep (0x553700)");
}

void SDriver::Ghost_NextStep()
{
    STUB_LOG("SDriver::Ghost_NextStep (0x553c00)");
    PZ_M2_TRACE("SDriver::Ghost_NextStep (0x553c00)");
}

float SDriver::GetMaxSpeed()
{
    STUB_LOG("SDriver::GetMaxSpeed (0x553260)");
    PZ_M2_TRACE("SDriver::GetMaxSpeed (0x553260)");
    return 0.0f;
}

float SDriver::GetTurnSpeed()
{
    STUB_LOG("SDriver::GetTurnSpeed (0x5533d0)");
    PZ_M2_TRACE("SDriver::GetTurnSpeed (0x5533d0)");
    return 0.0f;
}

void SDriver::Slot_50()
{
    STUB_LOG("SDriver::Slot_50 (0x5531e0)");
    PZ_M2_TRACE("SDriver::Slot_50 (0x5531e0)");
}

SPDriver::SPDriver()
{
    PZ_M2_TRACE("SPDriver::SPDriver");
}

SPDriver::~SPDriver()
{
    STUB_LOG("SPDriver::~SPDriver (0x550180)");
    PZ_M2_TRACE("SPDriver::~SPDriver (0x550180)");
}

void SPDriver::Load(SProperties* props, int p2, int p3)
{
    STUB_LOG("SPDriver::Load (0x555cc0)");
    PZ_M2_TRACE("SPDriver::Load (0x555cc0)");
    (void)props;
    (void)p2;
    (void)p3;
}

void SPDriver::LoadSubProperties()
{
    STUB_LOG("SPDriver::LoadSubProperties (0x557520)");
    PZ_M2_TRACE("SPDriver::LoadSubProperties (0x557520)");
}

void SPDriver::Slot_0C()
{
    STUB_LOG("SPDriver::Slot_0C (0x55c820)");
    PZ_M2_TRACE("SPDriver::Slot_0C (0x55c820)");
}

SIDriver* SPDriver::CreateDriver(SIUnit* unit)
{
    STUB_LOG("SPDriver::CreateDriver (pure in HD SPDriver)");
    PZ_M2_TRACE("SPDriver::CreateDriver (pure in HD SPDriver)");
    (void)unit;
    return nullptr;
}

} // namespace pz
