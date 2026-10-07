// src/game/unit.cpp
// pz::SUnit: the slots not lifted yet (logged stubs). OWNER: agent U.
// The lifted bodies are in unitbase.cpp.

#include <string.h>
#include "unit.h"
#include "doodad.h"
#include "target.h"
#include "world.h"
#include "worldapi.h"
#include "stub_log.h"
#include "packets.h"

namespace pz {

// M3-C5: the class records of SIUnit +0x1c (unit.h).
const SUnitClassDesc kUnitClassDesc_8dc540 = { "Special", nullptr, 0x8dc540 };
const SUnitClassDesc kUnitClassDesc_8dc0b0 = { "SUnit", &kUnitClassDesc_8dc540, 0x8dc0b0 };
const SUnitClassDesc kUnitClassDesc_8dbff0 = { "SUnit", &kUnitClassDesc_8dc540, 0x8dbff0 };
const SUnitClassDesc kUnitClassDesc_8da7f8 = { "SUnit", &kUnitClassDesc_8dc540, 0x8da7f8 };

void SUnit::Slot_10()
{
    STUB_LOG("SUnit::Slot_10 (0x5bc2d0)");
    PZ_M2_TRACE("SUnit::Slot_10 (0x5bc2d0)");
}

// SUnit::Slot_14 0x5bb1c0 / Slot_18 0x5baf30 (after load): unitsave.cpp (agent S, M4).

// PANZERS 0x5ba220
void SUnit::GetClassDescriptor(void** obj, const SUnitClassDesc** desc)
{
    *obj = this;
    *desc = &kUnitClassDesc_8dc540;                           // "Special"
}

// PANZERS 0x5bd900
void SUnit::RefreshDead()
{
}

// PANZERS 0x546ab0
bool SUnit::Slot_54()
{
    return false;
}

// PANZERS 0x5468d0
bool SUnit::Slot_58(int p1)
{
    (void)p1;
    return false;
}

// PANZERS 0x5468c0
void SUnit::Slot_60(int p1)
{
    (void)p1;
}

void SUnit::Slot_6C()
{
    STUB_LOG("SUnit::Slot_6C (0x5c2390)");
    PZ_M2_TRACE("SUnit::Slot_6C (0x5c2390)");
}

void SUnit::Slot_74()
{
    STUB_LOG("SUnit::Slot_74 (0x5b5c10)");
    PZ_M2_TRACE("SUnit::Slot_74 (0x5b5c10)");
}

// PANZERS 0x55ce70 (typed by M3-C3, the slot owner)
float* SUnit::GetEntrance(float* out3)
{
    out3[0] = Pos[0];
    out3[1] = Pos[1];
    out3[2] = Pos[2];
    return out3;
}

// PANZERS 0x55ce60 (typed by M3-C3, the slot owner)
float SUnit::GetEntranceDir()
{
    return Dir;
}

void SUnit::Slot_84()
{
    STUB_LOG("SUnit::Slot_84 (0x5c0fa0)");
    PZ_M2_TRACE("SUnit::Slot_84 (0x5c0fa0)");
}

// PANZERS 0x55cee0
void SUnit::AddXP(int victim, float xp, int p3)
{
    (void)victim; (void)xp; (void)p3;
}

void SUnit::Slot_9C()
{
    STUB_LOG("SUnit::Slot_9C (0x5c1d40)");
    PZ_M2_TRACE("SUnit::Slot_9C (0x5c1d40)");
}

void SUnit::EC_Default(int p1, int p2)
{
    STUB_LOG("SUnit::EC_Default (0x5b8ab0)");
    PZ_M2_TRACE("SUnit::EC_Default (0x5b8ab0)");
}

void SUnit::Slot_C8()
{
    STUB_LOG("SUnit::Slot_C8 (0x5b9170)");
    PZ_M2_TRACE("SUnit::Slot_C8 (0x5b9170)");
}

void SUnit::EC_AssaultBuilding(int p1, int p2)
{
    STUB_LOG("SUnit::EC_AssaultBuilding (0x5b82b0)");
    PZ_M2_TRACE("SUnit::EC_AssaultBuilding (0x5b82b0)");
}

// PANZERS 0x547b90
void SUnit::Slot_E0(int p1, int p2)
{
    (void)p1;
    (void)p2;
}

// PANZERS 0x5b8570 (typed and lifted by M3-C3, the +0xe4 slot owner)
void SUnit::EC_AttackPos(int xBits, int zBits, int p3)
{
    PZ_M2_TRACE("SUnit::EC_AttackPos (0x5b8570)");
    if (*(int*)(g_World->Players[Player] + 0x08) == 2 || !Proto->AttackGround)   // World+0x178, proto +0xb4
        return;
    float x, z;
    memcpy(&x, &xBits, 4);
    memcpy(&z, &zBits, 4);
    STarget* t = STarget::Create(2);                              // new 0x38, 0x5b27c0(2)
    t->Type = kTargetPath;                                        // 2
    t->Pos[0] = x;
    t->Pos[1] = g_World->GetTerrainHeight(x, z);                  // 0x5e7730
    t->Pos[2] = z;
    t->Mode = 1;                                                  // +0x20
    SetTarget(&PrimaryTarget, t);                                 // +0x1f8
    SetCurrentTarget(t, p3);                                      // +0xa0
}

// PANZERS 0x5478c0
void SUnit::EC_ThrowGrenade(int unit, int p2)
{
    (void)unit;
    (void)p2;
}

// PANZERS 0x5478e0
void SUnit::EC_ThrowMolotov(int unit, int p2)
{
    (void)unit;
    (void)p2;
}

// PANZERS 0x5478d0
void SUnit::EC_ThrowMagneticMine(int unit, int p2)
{
    (void)unit;
    (void)p2;
}

// PANZERS 0x547b30
void SUnit::Slot_FC(int p1, int p2, int p3)
{
    (void)p1;
    (void)p2;
    (void)p3;
}

// PANZERS 0x548180
void SUnit::Slot_100(int p1, int p2, int p3)
{
    (void)p1;
    (void)p2;
    (void)p3;
}

// PANZERS 0x547e80
void SUnit::Slot_104()
{
}

// PANZERS 0x5478f0
void SUnit::Slot_108()
{
}

// PANZERS 0x547ba0
void SUnit::Slot_10C(int p1, int p2, int p3)
{
    (void)p1;
    (void)p2;
    (void)p3;
}

// PANZERS 0x547b80
void SUnit::Slot_110(int p1, int p2)
{
    (void)p1;
    (void)p2;
}

// PANZERS 0x547b40
void SUnit::Slot_114(int p1, int p2)
{
    (void)p1;
    (void)p2;
}

// PANZERS 0x547b50
void SUnit::Slot_118(int p1)
{
    (void)p1;
}

// PANZERS 0x547b60
void SUnit::Slot_11C()
{
}

// PANZERS 0x547b70
void SUnit::Slot_120(int p1)
{
    (void)p1;
}

// PANZERS 0x54cd50
void SUnit::SetHealthPercent(float percent)
{
    HP = percent / 100.0f;                                   // DAT_007ee558
}

// PANZERS 0x55cdf0 (typed by M3-C3, the slot owner)
void SUnit::Slot_134(int p1)
{
    (void)p1;
}

// PANZERS 0x547900
void SUnit::EC_ChangeActiveDriver(int driver)
{
    (void)driver;
}

// PANZERS 0x547e70
void SUnit::Slot_148(int player)
{
    (void)player;
}

// PANZERS 0x5481a0
void SUnit::Slot_14C(int player)
{
    (void)player;
}

// PANZERS 0x548190
void SUnit::Slot_150(int p1)
{
    (void)p1;
}

// PANZERS 0x5481b0
void SUnit::Slot_154()
{
}

// PANZERS 0x55ce50 (typed by M3-C3, the slot owner)
void SUnit::Slot_158(int p1)
{
    (void)p1;
}

// PANZERS 0x547e60
void SUnit::Slot_15C(int p1)
{
    (void)p1;
}

// PANZERS 0x5478b0
void SUnit::Slot_160(int p1)
{
    (void)p1;
}

// PANZERS 0x55ce40 (typed by M3-C3, the slot owner)
void SUnit::Slot_164(int p1)
{
    (void)p1;
}

// PANZERS 0x5b9e30
float SUnit::GetHealth()
{
    return HP;
}

// PANZERS 0x5b9ec0
float SUnit::GetHitPoints()
{
    return Proto->HP;                                              // prototype +0x98
}

// PANZERS 0x5b9d90
float SUnit::GetLowestMaxRange()
{
    return GetMaxRange(0);                                        // +0x17c(0)
}

// PANZERS 0x548380
float SUnit::Slot_18C()
{
    return 0.0f;
}

// PANZERS 0x55e330
void SUnit::OnAttackedBy(int attacker)
{
    (void)attacker;
}

// PANZERS 0x55cf70
void SUnit::OnMemberDied(int unit)
{
    (void)unit;
}

// PANZERS 0x5c1da0
void SUnit::SetSpecialAnimation(const char* name)
{
    FreeSString((SString*)&_160);                             // operator delete +0x160
    int len = name ? (int)strlen(name) : 0;
    if (len == 0) {
        _164 = 0;
        _160 = nullptr;
    } else {
        _164 = len;
        char* s = new char[len + 1];                          // 0x766b87
        memcpy(s, name, len + 1);                             // 0x76b3a0
        _160 = s;
    }
}

// PANZERS 0x55cf60 (typed by M3-C3, the slot owner)
bool SUnit::IsCapturable()
{
    return false;
}

// PANZERS 0x5ba3e0
// An enemy of this unit's player (relation -1) gives 3 (attack), any other
// targetable unit 2 (follow); itself or an untargetable unit 0.
int SUnit::ActionOn(int target)
{
    if (IsTargetable(target, true) && target != WorldIndex)       // 0x5bb6b0
        return 3 - (GetUnitRelation(target, Player) != -1);       // 0x56d2a0
    return 0;
}

} // namespace pz
