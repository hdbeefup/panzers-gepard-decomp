// src/game/movementgroup.cpp
// SGameLogic orders and movement groups (convoys, formations).
// OWNER: agent L. Logged skeleton from M2-P0.

#include "gamelogic.h"
#include "stub_log.h"

namespace pz {

void SGameLogic::GroupOrder(int p1, int p2, int p3, int p4, int p5, int p6)
{
    STUB_LOG("SGameLogic::GroupOrder (0x56ff30)");
    PZ_M2_TRACE("SGameLogic::GroupOrder (0x56ff30)");
    (void)p1; (void)p2; (void)p3; (void)p4; (void)p5; (void)p6;
}

void SGameLogic::MoveFoundUnitsToLocation(int p1, int p2, int p3, int p4, int p5, int p6)
{
    STUB_LOG("SGameLogic::MoveFoundUnitsToLocation (0x57efd0)");
    PZ_M2_TRACE("SGameLogic::MoveFoundUnitsToLocation (0x57efd0)");
    (void)p1; (void)p2; (void)p3; (void)p4; (void)p5; (void)p6;
}

void SGameLogic::ConvoyAlongPath(int p1, int p2)
{
    STUB_LOG("SGameLogic::ConvoyAlongPath (0x57e600)");
    PZ_M2_TRACE("SGameLogic::ConvoyAlongPath (0x57e600)");
    (void)p1; (void)p2;
}

void SGameLogic::SendConvoyMovementGroupFollowers(int group)
{
    STUB_LOG("SGameLogic::SendConvoyMovementGroupFollowers (0x57e6f0)");
    PZ_M2_TRACE("SGameLogic::SendConvoyMovementGroupFollowers (0x57e6f0)");
    (void)group;
}

int SGameLogic::GetMovementGroupConvoy(int group)
{
    STUB_LOG("SGameLogic::GetMovementGroupConvoy (0x56af10)");
    PZ_M2_TRACE("SGameLogic::GetMovementGroupConvoy (0x56af10)");
    (void)group;
    return 0;
}

float SGameLogic::GetMovementGroupMoveSpeed(SIUnit* unit)
{
    STUB_LOG("SGameLogic::GetMovementGroupMoveSpeed (0x56b010)");
    PZ_M2_TRACE("SGameLogic::GetMovementGroupMoveSpeed (0x56b010)");
    (void)unit;
    return 0.0f;
}

int SGameLogic::GetMovementGroupSquadsGlobalState(int group)
{
    STUB_LOG("SGameLogic::GetMovementGroupSquadsGlobalState (0x56b090)");
    PZ_M2_TRACE("SGameLogic::GetMovementGroupSquadsGlobalState (0x56b090)");
    (void)group;
    return 0;
}

void SGameLogic::GetMovementGroupUnitFormationPos(int unit, float* out)
{
    STUB_LOG("SGameLogic::GetMovementGroupUnitFormationPos (0x56b240)");
    PZ_M2_TRACE("SGameLogic::GetMovementGroupUnitFormationPos (0x56b240)");
    (void)unit; (void)out;
}

void SGameLogic::SetMovementGroupBiggestUnit(int group)
{
    STUB_LOG("SGameLogic::SetMovementGroupBiggestUnit (0x57faf0)");
    PZ_M2_TRACE("SGameLogic::SetMovementGroupBiggestUnit (0x57faf0)");
    (void)group;
}

void SGameLogic::SetMovementGroupBossUnit(int group, int p2, int p3, int p4)
{
    STUB_LOG("SGameLogic::SetMovementGroupBossUnit (0x57fcb0)");
    PZ_M2_TRACE("SGameLogic::SetMovementGroupBossUnit (0x57fcb0)");
    (void)group; (void)p2; (void)p3; (void)p4;
}

void SGameLogic::SetMovementGroupFormationDir(int group, int p2)
{
    STUB_LOG("SGameLogic::SetMovementGroupFormationDir (0x57ff40)");
    PZ_M2_TRACE("SGameLogic::SetMovementGroupFormationDir (0x57ff40)");
    (void)group; (void)p2;
}

void SGameLogic::SetMovementGroupFormationDir2(int group, int p2)
{
    STUB_LOG("SGameLogic::SetMovementGroupFormationDir2 (0x57ffc0)");
    PZ_M2_TRACE("SGameLogic::SetMovementGroupFormationDir2 (0x57ffc0)");
    (void)group; (void)p2;
}

void SGameLogic::RefreshMovementGroup(int group, int p2)
{
    STUB_LOG("SGameLogic::RefreshMovementGroup (0x5800d0)");
    PZ_M2_TRACE("SGameLogic::RefreshMovementGroup (0x5800d0)");
    (void)group; (void)p2;
}

void SGameLogic::UpdateMovementGroupSlowestMoveSpeed(int group)
{
    STUB_LOG("SGameLogic::UpdateMovementGroupSlowestMoveSpeed (0x5824b0)");
    PZ_M2_TRACE("SGameLogic::UpdateMovementGroupSlowestMoveSpeed (0x5824b0)");
    (void)group;
}

} // namespace pz
