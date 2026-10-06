// src/game/unit.cpp
// pz::SUnit, the HD unit base class (0x5b2800..0x5c6300). OWNER: agent U.

#include "unit.h"
#include "stub_log.h"

namespace pz {

SUnit::SUnit()
{
    PZ_M2_TRACE("SUnit::SUnit");
}

SUnit::~SUnit()
{
    STUB_LOG("SUnit::~SUnit (0x5b3730)");
    PZ_M2_TRACE("SUnit::~SUnit (0x5b3730)");
}

void SUnit::Uninit()
{
    STUB_LOG("SUnit::Uninit (0x5b7e40)");
    PZ_M2_TRACE("SUnit::Uninit (0x5b7e40)");
}

void SUnit::Init(SUnitDef* def)
{
    STUB_LOG("SUnit::Init (0x5ba8e0)");
    PZ_M2_TRACE("SUnit::Init (0x5ba8e0)");
    (void)def;
}

void SUnit::InitNew(int p1, int p2, int p3, int p4, int p5)
{
    STUB_LOG("SUnit::InitNew (0x5bace0)");
    PZ_M2_TRACE("SUnit::InitNew (0x5bace0)");
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
}

void SUnit::Slot_10()
{
    STUB_LOG("SUnit::Slot_10 (0x5bc2d0)");
    PZ_M2_TRACE("SUnit::Slot_10 (0x5bc2d0)");
}

void SUnit::Slot_14()
{
    STUB_LOG("SUnit::Slot_14 (0x5bb1c0)");
    PZ_M2_TRACE("SUnit::Slot_14 (0x5bb1c0)");
}

void SUnit::Slot_18()
{
    STUB_LOG("SUnit::Slot_18 (0x5baf30)");
    PZ_M2_TRACE("SUnit::Slot_18 (0x5baf30)");
}

void SUnit::Slot_1C()
{
    STUB_LOG("SUnit::Slot_1C (0x5ba220)");
    PZ_M2_TRACE("SUnit::Slot_1C (0x5ba220)");
}

void SUnit::Hook20(int p1)
{
    STUB_LOG("SUnit::Hook20 (0x54cd40)");
    PZ_M2_TRACE("SUnit::Hook20 (0x54cd40)");
    (void)p1;
}

void SUnit::SetPosition(float x, float z, int p3, int p4)
{
    STUB_LOG("SUnit::SetPosition (0x5c1980)");
    PZ_M2_TRACE("SUnit::SetPosition (0x5c1980)");
    (void)x;
    (void)z;
    (void)p3;
    (void)p4;
}

void SUnit::Slot_28()
{
    STUB_LOG("SUnit::Slot_28 (0x5be2b0)");
    PZ_M2_TRACE("SUnit::Slot_28 (0x5be2b0)");
}

void SUnit::ServerRefresh(int frame)
{
    STUB_LOG("SUnit::ServerRefresh (0x5bee90)");
    PZ_M2_TRACE("SUnit::ServerRefresh (0x5bee90)");
    (void)frame;
}

void SUnit::Slot_30()
{
    STUB_LOG("SUnit::Slot_30 (0x5bd900)");
    PZ_M2_TRACE("SUnit::Slot_30 (0x5bd900)");
}

void SUnit::RefreshTargeting()
{
    STUB_LOG("SUnit::RefreshTargeting (0x5bd600)");
    PZ_M2_TRACE("SUnit::RefreshTargeting (0x5bd600)");
}

void SUnit::RefreshMisc()
{
    STUB_LOG("SUnit::RefreshMisc (0x5bdee0)");
    PZ_M2_TRACE("SUnit::RefreshMisc (0x5bdee0)");
}

void SUnit::RefreshModel()
{
    STUB_LOG("SUnit::RefreshModel (0x5c6130)");
    PZ_M2_TRACE("SUnit::RefreshModel (0x5c6130)");
}

void SUnit::UpdateVisuals(SIViewport* vp)
{
    STUB_LOG("SUnit::UpdateVisuals (0x5b76c0)");
    PZ_M2_TRACE("SUnit::UpdateVisuals (0x5b76c0)");
    (void)vp;
}

void SUnit::Slot_44()
{
    STUB_LOG("SUnit::Slot_44 (0x5bce20)");
    PZ_M2_TRACE("SUnit::Slot_44 (0x5bce20)");
}

void SUnit::Slot_48()
{
    STUB_LOG("SUnit::Slot_48 (0x5bcb60)");
    PZ_M2_TRACE("SUnit::Slot_48 (0x5bcb60)");
}

void SUnit::Unplace()
{
    STUB_LOG("SUnit::Unplace (0x5ba850)");
    PZ_M2_TRACE("SUnit::Unplace (0x5ba850)");
}

void SUnit::Place(float x, float z, float dir)
{
    STUB_LOG("SUnit::Place (0x5c5160)");
    PZ_M2_TRACE("SUnit::Place (0x5c5160)");
    (void)x;
    (void)z;
    (void)dir;
}

void SUnit::Slot_54()
{
    STUB_LOG("SUnit::Slot_54 (0x546ab0)");
    PZ_M2_TRACE("SUnit::Slot_54 (0x546ab0)");
}

void SUnit::Slot_58()
{
    STUB_LOG("SUnit::Slot_58 (0x5468d0)");
    PZ_M2_TRACE("SUnit::Slot_58 (0x5468d0)");
}

void SUnit::StoreUnit(int unit, int p2)
{
    STUB_LOG("SUnit::StoreUnit (0x5c30d0)");
    PZ_M2_TRACE("SUnit::StoreUnit (0x5c30d0)");
    (void)unit;
    (void)p2;
}

void SUnit::Slot_60()
{
    STUB_LOG("SUnit::Slot_60 (0x5468c0)");
    PZ_M2_TRACE("SUnit::Slot_60 (0x5468c0)");
}

void SUnit::Slot_64()
{
    STUB_LOG("SUnit::Slot_64 (0x5c51d0)");
    PZ_M2_TRACE("SUnit::Slot_64 (0x5c51d0)");
}

void SUnit::Slot_68()
{
    STUB_LOG("SUnit::Slot_68 (0x5c6000)");
    PZ_M2_TRACE("SUnit::Slot_68 (0x5c6000)");
}

void SUnit::Slot_6C()
{
    STUB_LOG("SUnit::Slot_6C (0x5c2390)");
    PZ_M2_TRACE("SUnit::Slot_6C (0x5c2390)");
}

void SUnit::Remove(bool p1)
{
    STUB_LOG("SUnit::Remove (0x5c2df0)");
    PZ_M2_TRACE("SUnit::Remove (0x5c2df0)");
    (void)p1;
}

void SUnit::Slot_74()
{
    STUB_LOG("SUnit::Slot_74 (0x5b5c10)");
    PZ_M2_TRACE("SUnit::Slot_74 (0x5b5c10)");
}

void SUnit::Slot_78()
{
    STUB_LOG("SUnit::Slot_78 (0x55ce70)");
    PZ_M2_TRACE("SUnit::Slot_78 (0x55ce70)");
}

void SUnit::Slot_7C()
{
    STUB_LOG("SUnit::Slot_7C (0x55ce60)");
    PZ_M2_TRACE("SUnit::Slot_7C (0x55ce60)");
}

bool SUnit::HasWoundedMember()
{
    STUB_LOG("SUnit::HasWoundedMember (0x54a240)");
    PZ_M2_TRACE("SUnit::HasWoundedMember (0x54a240)");
    return false;
}

void SUnit::Slot_84()
{
    STUB_LOG("SUnit::Slot_84 (0x5c0fa0)");
    PZ_M2_TRACE("SUnit::Slot_84 (0x5c0fa0)");
}

int SUnit::GetRank()
{
    STUB_LOG("SUnit::GetRank (0x5b9e60)");
    PZ_M2_TRACE("SUnit::GetRank (0x5b9e60)");
    return 0;
}

void SUnit::Slot_8C()
{
    STUB_LOG("SUnit::Slot_8C (0x55cee0)");
    PZ_M2_TRACE("SUnit::Slot_8C (0x55cee0)");
}

void SUnit::Slot_90()
{
    STUB_LOG("SUnit::Slot_90 (0x5b6040)");
    PZ_M2_TRACE("SUnit::Slot_90 (0x5b6040)");
}

void SUnit::Slot_94()
{
    STUB_LOG("SUnit::Slot_94 (0x5c4080)");
    PZ_M2_TRACE("SUnit::Slot_94 (0x5c4080)");
}

void SUnit::Slot_98()
{
    STUB_LOG("SUnit::Slot_98 (0x5c5050)");
    PZ_M2_TRACE("SUnit::Slot_98 (0x5c5050)");
}

void SUnit::Slot_9C()
{
    STUB_LOG("SUnit::Slot_9C (0x5c1d40)");
    PZ_M2_TRACE("SUnit::Slot_9C (0x5c1d40)");
}

void SUnit::SetCurrentTarget(STarget* target)
{
    STUB_LOG("SUnit::SetCurrentTarget (0x5c0d10)");
    PZ_M2_TRACE("SUnit::SetCurrentTarget (0x5c0d10)");
    (void)target;
}

void SUnit::Slot_A4()
{
    STUB_LOG("SUnit::Slot_A4 (0x5b8ab0)");
    PZ_M2_TRACE("SUnit::Slot_A4 (0x5b8ab0)");
}

void SUnit::Slot_A8()
{
    STUB_LOG("SUnit::Slot_A8 (0x5ba3e0)");
    PZ_M2_TRACE("SUnit::Slot_A8 (0x5ba3e0)");
}

void SUnit::EC_Move(int p1, int p2, int p3, bool p4, int p5)
{
    STUB_LOG("SUnit::EC_Move (0x5b8ea0)");
    PZ_M2_TRACE("SUnit::EC_Move (0x5b8ea0)");
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
}

void SUnit::Slot_B0()
{
    STUB_LOG("SUnit::Slot_B0 (0x5b8d20)");
    PZ_M2_TRACE("SUnit::Slot_B0 (0x5b8d20)");
}

void SUnit::EC_MoveAlongPath(int path, int p2, int p3)
{
    STUB_LOG("SUnit::EC_MoveAlongPath (0x5b8e20)");
    PZ_M2_TRACE("SUnit::EC_MoveAlongPath (0x5b8e20)");
    (void)path;
    (void)p2;
    (void)p3;
}

void SUnit::EC_Follow(int unit, int p2)
{
    STUB_LOG("SUnit::EC_Follow (0x5b8ba0)");
    PZ_M2_TRACE("SUnit::EC_Follow (0x5b8ba0)");
    (void)unit;
    (void)p2;
}

void SUnit::Slot_BC()
{
    STUB_LOG("SUnit::Slot_BC (0x5b9370)");
    PZ_M2_TRACE("SUnit::Slot_BC (0x5b9370)");
}

void SUnit::Stop()
{
    STUB_LOG("SUnit::Stop (0x5b91b0)");
    PZ_M2_TRACE("SUnit::Stop (0x5b91b0)");
}

void SUnit::ClearTargets()
{
    STUB_LOG("SUnit::ClearTargets (0x5b90b0)");
    PZ_M2_TRACE("SUnit::ClearTargets (0x5b90b0)");
}

void SUnit::Slot_C8()
{
    STUB_LOG("SUnit::Slot_C8 (0x5b9170)");
    PZ_M2_TRACE("SUnit::Slot_C8 (0x5b9170)");
}

void SUnit::Slot_CC()
{
    STUB_LOG("SUnit::Slot_CC (0x5b8660)");
    PZ_M2_TRACE("SUnit::Slot_CC (0x5b8660)");
}

void SUnit::Slot_D0()
{
    STUB_LOG("SUnit::Slot_D0 (0x5b8740)");
    PZ_M2_TRACE("SUnit::Slot_D0 (0x5b8740)");
}

void SUnit::Slot_D4()
{
    STUB_LOG("SUnit::Slot_D4 (0x5b82b0)");
    PZ_M2_TRACE("SUnit::Slot_D4 (0x5b82b0)");
}

void SUnit::Slot_D8()
{
    STUB_LOG("SUnit::Slot_D8 (0x5b8fa0)");
    PZ_M2_TRACE("SUnit::Slot_D8 (0x5b8fa0)");
}

void SUnit::Slot_DC()
{
    STUB_LOG("SUnit::Slot_DC (0x5b8c90)");
    PZ_M2_TRACE("SUnit::Slot_DC (0x5b8c90)");
}

void SUnit::Slot_E0()
{
    STUB_LOG("SUnit::Slot_E0 (0x547b90)");
    PZ_M2_TRACE("SUnit::Slot_E0 (0x547b90)");
}

void SUnit::Slot_E4()
{
    STUB_LOG("SUnit::Slot_E4 (0x5b8570)");
    PZ_M2_TRACE("SUnit::Slot_E4 (0x5b8570)");
}

void SUnit::Slot_E8()
{
    STUB_LOG("SUnit::Slot_E8 (0x5b8440)");
    PZ_M2_TRACE("SUnit::Slot_E8 (0x5b8440)");
}

void SUnit::StopGunners()
{
    STUB_LOG("SUnit::StopGunners (0x5b9110)");
    PZ_M2_TRACE("SUnit::StopGunners (0x5b9110)");
}

void SUnit::Slot_F0()
{
    STUB_LOG("SUnit::Slot_F0 (0x5478c0)");
    PZ_M2_TRACE("SUnit::Slot_F0 (0x5478c0)");
}

void SUnit::Slot_F4()
{
    STUB_LOG("SUnit::Slot_F4 (0x5478e0)");
    PZ_M2_TRACE("SUnit::Slot_F4 (0x5478e0)");
}

void SUnit::Slot_F8()
{
    STUB_LOG("SUnit::Slot_F8 (0x5478d0)");
    PZ_M2_TRACE("SUnit::Slot_F8 (0x5478d0)");
}

void SUnit::Slot_FC()
{
    STUB_LOG("SUnit::Slot_FC (0x547b30)");
    PZ_M2_TRACE("SUnit::Slot_FC (0x547b30)");
}

void SUnit::Slot_100()
{
    STUB_LOG("SUnit::Slot_100 (0x548180)");
    PZ_M2_TRACE("SUnit::Slot_100 (0x548180)");
}

void SUnit::Slot_104()
{
    STUB_LOG("SUnit::Slot_104 (0x547e80)");
    PZ_M2_TRACE("SUnit::Slot_104 (0x547e80)");
}

void SUnit::Slot_108()
{
    STUB_LOG("SUnit::Slot_108 (0x5478f0)");
    PZ_M2_TRACE("SUnit::Slot_108 (0x5478f0)");
}

void SUnit::Slot_10C()
{
    STUB_LOG("SUnit::Slot_10C (0x547ba0)");
    PZ_M2_TRACE("SUnit::Slot_10C (0x547ba0)");
}

void SUnit::Slot_110()
{
    STUB_LOG("SUnit::Slot_110 (0x547b80)");
    PZ_M2_TRACE("SUnit::Slot_110 (0x547b80)");
}

void SUnit::Slot_114()
{
    STUB_LOG("SUnit::Slot_114 (0x547b40)");
    PZ_M2_TRACE("SUnit::Slot_114 (0x547b40)");
}

void SUnit::Slot_118()
{
    STUB_LOG("SUnit::Slot_118 (0x547b50)");
    PZ_M2_TRACE("SUnit::Slot_118 (0x547b50)");
}

void SUnit::Slot_11C()
{
    STUB_LOG("SUnit::Slot_11C (0x547b60)");
    PZ_M2_TRACE("SUnit::Slot_11C (0x547b60)");
}

void SUnit::Slot_120()
{
    STUB_LOG("SUnit::Slot_120 (0x547b70)");
    PZ_M2_TRACE("SUnit::Slot_120 (0x547b70)");
}

void SUnit::Slot_124()
{
    STUB_LOG("SUnit::Slot_124 (0x5b8b10)");
    PZ_M2_TRACE("SUnit::Slot_124 (0x5b8b10)");
}

void SUnit::Slot_128()
{
    STUB_LOG("SUnit::Slot_128 (0x5b88d0)");
    PZ_M2_TRACE("SUnit::Slot_128 (0x5b88d0)");
}

void SUnit::SetBehavior(int behavior)
{
    STUB_LOG("SUnit::SetBehavior (0x5b8960)");
    PZ_M2_TRACE("SUnit::SetBehavior (0x5b8960)");
    (void)behavior;
}

void SUnit::Slot_130()
{
    STUB_LOG("SUnit::Slot_130 (0x54cd50)");
    PZ_M2_TRACE("SUnit::Slot_130 (0x54cd50)");
}

void SUnit::Slot_134()
{
    STUB_LOG("SUnit::Slot_134 (0x55cdf0)");
    PZ_M2_TRACE("SUnit::Slot_134 (0x55cdf0)");
}

void SUnit::Slot_138()
{
    STUB_LOG("SUnit::Slot_138 (0x547900)");
    PZ_M2_TRACE("SUnit::Slot_138 (0x547900)");
}

void SUnit::Slot_13C()
{
    STUB_LOG("SUnit::Slot_13C (0x5b8920)");
    PZ_M2_TRACE("SUnit::Slot_13C (0x5b8920)");
}

void SUnit::Slot_140()
{
    STUB_LOG("SUnit::Slot_140 (0x5b87f0)");
    PZ_M2_TRACE("SUnit::Slot_140 (0x5b87f0)");
}

void SUnit::Slot_144()
{
    STUB_LOG("SUnit::Slot_144 (0x5b93f0)");
    PZ_M2_TRACE("SUnit::Slot_144 (0x5b93f0)");
}

void SUnit::Slot_148()
{
    STUB_LOG("SUnit::Slot_148 (0x547e70)");
    PZ_M2_TRACE("SUnit::Slot_148 (0x547e70)");
}

void SUnit::Slot_14C()
{
    STUB_LOG("SUnit::Slot_14C (0x5481a0)");
    PZ_M2_TRACE("SUnit::Slot_14C (0x5481a0)");
}

void SUnit::Slot_150()
{
    STUB_LOG("SUnit::Slot_150 (0x548190)");
    PZ_M2_TRACE("SUnit::Slot_150 (0x548190)");
}

void SUnit::Slot_154()
{
    STUB_LOG("SUnit::Slot_154 (0x5481b0)");
    PZ_M2_TRACE("SUnit::Slot_154 (0x5481b0)");
}

void SUnit::Slot_158()
{
    STUB_LOG("SUnit::Slot_158 (0x55ce50)");
    PZ_M2_TRACE("SUnit::Slot_158 (0x55ce50)");
}

void SUnit::Slot_15C()
{
    STUB_LOG("SUnit::Slot_15C (0x547e60)");
    PZ_M2_TRACE("SUnit::Slot_15C (0x547e60)");
}

void SUnit::Slot_160()
{
    STUB_LOG("SUnit::Slot_160 (0x5478b0)");
    PZ_M2_TRACE("SUnit::Slot_160 (0x5478b0)");
}

void SUnit::Slot_164()
{
    STUB_LOG("SUnit::Slot_164 (0x55ce40)");
    PZ_M2_TRACE("SUnit::Slot_164 (0x55ce40)");
}

void SUnit::Slot_168()
{
    STUB_LOG("SUnit::Slot_168 (0x5b9360)");
    PZ_M2_TRACE("SUnit::Slot_168 (0x5b9360)");
}

void SUnit::StoreInterpolationState()
{
    STUB_LOG("SUnit::StoreInterpolationState (0x5b5ad0)");
    PZ_M2_TRACE("SUnit::StoreInterpolationState (0x5b5ad0)");
}

void SUnit::Slot_170()
{
    STUB_LOG("SUnit::Slot_170 (0x5b9e30)");
    PZ_M2_TRACE("SUnit::Slot_170 (0x5b9e30)");
}

void SUnit::Slot_174()
{
    STUB_LOG("SUnit::Slot_174 (0x5b9ec0)");
    PZ_M2_TRACE("SUnit::Slot_174 (0x5b9ec0)");
}

void SUnit::Slot_178()
{
    STUB_LOG("SUnit::Slot_178 (0x5b9d90)");
    PZ_M2_TRACE("SUnit::Slot_178 (0x5b9d90)");
}

float SUnit::GetMaxRange(int weapon)
{
    STUB_LOG("SUnit::GetMaxRange (0x5b9f60)");
    PZ_M2_TRACE("SUnit::GetMaxRange (0x5b9f60)");
    (void)weapon;
    return 0.0f;
}

float SUnit::GetMinRange(int weapon)
{
    STUB_LOG("SUnit::GetMinRange (0x5b9f90)");
    PZ_M2_TRACE("SUnit::GetMinRange (0x5b9f90)");
    (void)weapon;
    return 0.0f;
}

float SUnit::GetSightRange()
{
    STUB_LOG("SUnit::GetSightRange (0x5ba240)");
    PZ_M2_TRACE("SUnit::GetSightRange (0x5ba240)");
    return 0.0f;
}

float SUnit::GetExtra188()
{
    STUB_LOG("SUnit::GetExtra188 (0x548230)");
    PZ_M2_TRACE("SUnit::GetExtra188 (0x548230)");
    return 0.0f;
}

void SUnit::Slot_18C()
{
    STUB_LOG("SUnit::Slot_18C (0x548380)");
    PZ_M2_TRACE("SUnit::Slot_18C (0x548380)");
}

void SUnit::AI_Heartbeat()
{
    STUB_LOG("SUnit::AI_Heartbeat (0x5b37d0)");
    PZ_M2_TRACE("SUnit::AI_Heartbeat (0x5b37d0)");
}

void SUnit::Slot_194()
{
    STUB_LOG("SUnit::Slot_194 (0x55e330)");
    PZ_M2_TRACE("SUnit::Slot_194 (0x55e330)");
}

void SUnit::SetOnBlockMap(bool on)
{
    STUB_LOG("SUnit::SetOnBlockMap (0x5bc660)");
    PZ_M2_TRACE("SUnit::SetOnBlockMap (0x5bc660)");
    (void)on;
}

void SUnit::Slot_19C()
{
    STUB_LOG("SUnit::Slot_19C (0x5b74c0)");
    PZ_M2_TRACE("SUnit::Slot_19C (0x5b74c0)");
}

void SUnit::MarkBlockMap(bool on, int p2, int p3, int p4, short p5)
{
    STUB_LOG("SUnit::MarkBlockMap (0x5bc6d0)");
    PZ_M2_TRACE("SUnit::MarkBlockMap (0x5bc6d0)");
    (void)on;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
}

int SUnit::TestBlockMapPath(int p1, int p2, int p3, int p4, short p5, int p6, int p7)
{
    STUB_LOG("SUnit::TestBlockMapPath (0x5b7520)");
    PZ_M2_TRACE("SUnit::TestBlockMapPath (0x5b7520)");
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    (void)p6;
    (void)p7;
    return 0;
}

int SUnit::TestBlockMap(int p1, int p2, int p3, int p4, short p5)
{
    STUB_LOG("SUnit::TestBlockMap (0x5b7500)");
    PZ_M2_TRACE("SUnit::TestBlockMap (0x5b7500)");
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    return 0;
}

void SUnit::RestoreBehavior()
{
    STUB_LOG("SUnit::RestoreBehavior (0x546ac0)");
    PZ_M2_TRACE("SUnit::RestoreBehavior (0x546ac0)");
}

float SUnit::GetMoveSpeed(int p1)
{
    STUB_LOG("SUnit::GetMoveSpeed (0x5b9ed0)");
    PZ_M2_TRACE("SUnit::GetMoveSpeed (0x5b9ed0)");
    (void)p1;
    return 0.0f;
}

void SUnit::Slot_1B4()
{
    STUB_LOG("SUnit::Slot_1B4 (0x55cf70)");
    PZ_M2_TRACE("SUnit::Slot_1B4 (0x55cf70)");
}

void SUnit::Slot_1B8()
{
    STUB_LOG("SUnit::Slot_1B8 (0x5c1da0)");
    PZ_M2_TRACE("SUnit::Slot_1B8 (0x5c1da0)");
}

void SUnit::Slot_1BC()
{
    STUB_LOG("SUnit::Slot_1BC (0x55cf60)");
    PZ_M2_TRACE("SUnit::Slot_1BC (0x55cf60)");
}

void SUnit::ServerRefreshMedic(float dt)
{
    STUB_LOG("SUnit::ServerRefreshMedic (0x5bfa50)");
    PZ_M2_TRACE("SUnit::ServerRefreshMedic (0x5bfa50)");
    (void)dt;
}

void SUnit::SetUnitSize()
{
    STUB_LOG("SUnit::SetUnitSize (0x5c21d0)");
    PZ_M2_TRACE("SUnit::SetUnitSize (0x5c21d0)");
}

void SUnit::GetCenterPosition(float* out)
{
    STUB_LOG("SUnit::GetCenterPosition (0x5b9d40)");
    PZ_M2_TRACE("SUnit::GetCenterPosition (0x5b9d40)");
    (void)out;
}

} // namespace pz
