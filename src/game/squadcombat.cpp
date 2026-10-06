// src/game/squadcombat.cpp
// SPanzersSquadUnit / SPanzersSquadMemberUnit in combat (0x598070..0x59f580):
// attack orders, the equipment items, the reactions to an attacker, the
// member bookkeeping when a member dies, the dead member's countdown, the
// parachute drop. OWNER: M3-C5. See squadunit.h.
//
// Every draw of the world seed (World+0x7518) is where HD draws it:
// GetARandomMemberIdx 0x59b3d0 (one), ParachuteDrop 0x598cb0 (four),
// OnAttackedBy 0x59ef00 (one per member through SWalkerAnimation 0x5cb0a0).

#include <math.h>
#include <string.h>
#include "squadunit.h"
#include "drivermath.h"
#include "gamelogic.h"
#include "gunner.h"
#include "idriver.h"
#include "target.h"
#include "unitanim.h"
#include "unitextern.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/imodel.h"
#include "logger.h"
#include "doodad.h"
#include "m3common.h"
#include "stub_log.h"

namespace pz {

// ---------------------------------------------------------------------------
// Helpers (HD inline code)

static SUnit* MemberUnit(SUnit* squad, int i)                 // SDArray<SUnitMember>[i] + SHeapTRB[]
{
    if (i < 0 || i >= squad->Members.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", i);
    return WorldUnit(squad->Members.Array[i].Unit);
}

// World+0x178 + player * 0x48: the player's kind (1 human, 2 passive, 3, 4).
static int PlayerKind(int player)
{
    return *(int*)(g_World->Players[player] + 0x08);
}

// World+0x17c + player * 0x48: the player's alliance (0 = none).
static int PlayerAlliance(int player)
{
    return *(int*)(g_World->Players[player] + 0x0c);
}

// HD 0x549ab0 (inline copy): a and b are allies.
static bool PlayersAllied(int a, int b)
{
    int t = PlayerAlliance(a);
    return t != 0 ? t == PlayerAlliance(b) : a == b;
}

// SHeap<AIGP>::operator[] 0x55ccc0 (World+0x4f4): the AI group record; its
// +0x08 is the group's mode (2 assault, 4, 5 passive?).
static int AIGroupMode(int group)
{
    SWorld* w = g_World;
    if (!w->AIGroups.IsLive(group))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SAIGroup", group);
    int v;
    memcpy(&v, &w->AIGroups.Array[group].Data[8], 4);
    return v;
}

static int FloatBits(float f)
{
    int b;
    memcpy(&b, &f, 4);
    return b;
}

// SGunner::CanTargetUnit 0x583b60 (gunner.cpp): can the gunner attack
// `target` (p3: the caller may move).
bool GunnerCanAttackUnit(SGunner* g, SUnit* target, bool p3)
{
    return g->CanTargetUnit(target, p3);
}

// SWasterUnit 0x5870f0 WasterSetOwner: waster.cpp (M3-C C3).

// SWorld 0x5d68e0 (agent C4: the AI group of `unit` answers an attack; one
// world RNG draw on some paths). M3-C: owned by C4, stub until merged.
void SWorld::AIGroupUnitAttacked(int unit, int attacker)
{
    (void)unit;
    (void)attacker;
    STUB_LOG("SWorld 0x5d68e0 (AI group attacked), called by SPanzersSquadUnit::OnAttackedBy 0x59ef00");
    PZ_M3_TRACE("SWorld 0x5d68e0");
}

// ---------------------------------------------------------------------------
// SUnit helpers the squads need (unit.h block C5)

// PANZERS 0x5c1d50
// The unit's driver left: stop, remember the active driver (+0x30 while a
// target is set, +0x2c), no active driver, out of the movement group.
void SUnit::LeaveDriverSeat()
{
    Stop();                                                   // +0xc0
    if (CurrentTarget)
        RefreshDriver = ActiveDriver > -1 ? ActiveDriver : PrevDriver;
    PrevDriver = ActiveDriver;
    ActiveDriver = -1;
    if (g_GameLogic)
        g_GameLogic->RemoveUnitFromMovementGroup(WorldIndex);   // 0x579510
}

// PANZERS 0x5be140
void SUnit::RemoveStoredMember(int i)
{
    if (i < 0 || i >= Members.Size)
        Logger.g->Panic("SUnit::RemoveStoredMember - bad member idx:%d", i);
    if (Members.Array[i].Seat == 1 || Members.Array[i].Seat == 2)   // 0x5991e0
        LeaveDriverSeat();                                    // 0x5c1d50
    if (i >= Members.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", i);
    if (Members.Array[i].Seat == 3 || Members.Array[i].Seat == 2) {
        SGunner* g = GetGunner(Members.Array[i].Gunner);      // 0x55cc40
        g->Stop();                                            // +0x28
        GetGunner(Members.Array[i].Gunner)->Active = 0;       // +0x1c
    }
    // 0x54c7b0 SDArray<SUnitMember>::Remove
    if (i < 0 || i >= Members.Size)
        Logger.g->Panic("SDArray<%s>::Remove: invalid index (%d) size = %d", "struct SUnitMember", i, Members.Size);
    Members.Size--;
    int rest = Members.Size - i;
    if (rest != 0)
        memmove(&Members.Array[i], &Members.Array[i + 1], rest * sizeof(SUnitMember));
    memset(&Members.Array[Members.Size], 0, sizeof(SUnitMember));
}

// ---------------------------------------------------------------------------
// SPanzersSquadUnit

// PANZERS 0x59bfd0
void SPanzersSquadUnit::GetClassDescriptor(void** obj, const SUnitClassDesc** desc)
{
    *obj = this;
    *desc = &kUnitClassDesc_8dc0b0;
}

// PANZERS 0x59b3d0
int SPanzersSquadUnit::GetARandomMemberIdx()
{
    int n = Members.Size;
    if (n < 1)
        Logger.g->Panic("SPanzersSquadUnit::GetARandomMemberIdx (IHaveToDie = %s, Hidden = %s)",
                        Wrecked ? "True" : "False", Unplaced ? "True" : "False");
    int r = WorldRand();
    int i = (int)((double)r * 3.0517578125e-05 * (double)n);   // DAT_007f4540, cvttsd2si
    if (i < 0 || i >= Members.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", i);
    return Members.Array[i].Unit;
}

// PANZERS 0x59b650
// The shortest range of the members' main weapons (1000 at most); 0 when
// the first member has no main gunner. A member on a thrown item (main
// gunner 1 or 2) answers the grenade range of the squad's rank.
float SPanzersSquadUnit::GetLowestMaxRange()
{
    int n = Members.Size;
    if (n == 0)
        Logger.g->Panic("SPanzersSquadUnit::GetLowestMaxRange() - StoredMembers.GetSize() == 0");
    if (MemberUnit(this, 0)->MainGunner == -1)
        return 0.0f;
    float lowest = 1000.0f;                                   // DAT_007f1b94
    for (int i = 0; i < Members.Size; ++i) {
        SUnit* m = MemberUnit(this, i);
        int g = m->MainGunner;
        if (g < 0 || g >= m->Gunners.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", g);
        float r = m->Gunners.Array[g]->GetPGunner()->MaxRange;   // +0x2c, +0x38
        if (lowest > r)
            lowest = r;
        int mg = MemberUnit(this, i)->MainGunner;
        if (mg == 1 || mg == 2) {
            switch (GetRank()) {                              // +0x88
            case 0: return (float)g_UnitRegistry->GrenadeMaxRange[0];
            case 1: return (float)g_UnitRegistry->GrenadeMaxRange[1];
            case 2: return (float)g_UnitRegistry->GrenadeMaxRange[2];
            case 3: return (float)g_UnitRegistry->GrenadeMaxRange[3];
            case 4: return (float)g_UnitRegistry->GrenadeMaxRange[4];
            default: break;                                   // rank > 4: next member
            }
        }
    }
    return lowest;
}

// PANZERS 0x599d60
// Every member back on its default gunner; a building the squad's first
// gunner cannot shoot at is assaulted (+0xd4), anything else attacked as
// SUnit::EC_Attack does.
void SPanzersSquadUnit::EC_Attack(int unit, int p2)
{
    if (PlayerKind(Player) == 2)
        return;
    if (!IsTargetable(unit, true))                            // 0x5bb6b0
        return;
    for (int i = 0; i < Members.Size; ++i)
        MemberUnit(this, i)->MainGunner = 0;
    SUnit* t = WorldUnit(unit);
    if (t->Parent == WorldIndex)
        Logger.g->Panic("SPanzersSquadUnit::EC_Attack: PanzersSquad is trying to attack it's own member.");
    if (WorldUnit(unit)->Proto->ClassType == 9) {
        bool canShoot = false;
        if (Gunners.Size > 0) {
            SUnit* b = WorldUnit(unit);
            if (Gunners.Size < 1)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
            canShoot = GunnerCanAttackUnit(Gunners.Array[0], b, true);   // 0x583b60
        }
        if (!canShoot) {
            EC_AssaultBuilding(unit, p2);                     // +0xd4
            return;
        }
    }
    SUnit::EC_Attack(unit, p2);                               // 0x5b8440
    if (CurrentTarget && CurrentTarget->Mode != 1)
        RestoreBehavior();                                    // +0x1ac
    UpdateMovingMembersRelPos();                              // 0x5a0d30
}

// The equipment orders 0x599f80 / 0x59a280 (and 0x59a0f0): the squad
// attacks, and one random member takes the item's gunner (a full load) and
// attacks too.
static void SquadUseItem(SPanzersSquadUnit* s, int item, int unit, int p2)
{
    s->EC_Attack(unit, p2);                                   // +0xe8
    int m = s->GetARandomMemberIdx();                         // 0x59b3d0
    SUnit* mu = WorldUnit(m);
    if (mu->Gunners.Size <= item)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", item);
    SGunner* g = mu->Gunners.Array[item];
    SUnit* mu2 = WorldUnit(m);
    if (mu2->Gunners.Size <= item)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", item);
    SGunner* g2 = mu2->Gunners.Array[item];
    g2->AmmoLeft = 1.0f / (float)g->GetPGunner()->Ammo;      // DAT_007f1b58 / +0x48
    WorldUnit(m)->MainGunner = item;
    WorldUnit(m)->EC_Attack(unit, p2);                        // member +0xe8 (0x597e80)
}

// PANZERS 0x599f80
void SPanzersSquadUnit::EC_ThrowGrenade(int unit, int p2)
{
    if (unit == WorldIndex || !IsTargetable(unit, true))
        return;
    if (HasSlotWeapon(1))                                     // 0x5ba820
        SquadUseItem(this, 1, unit, p2);
    if (CurrentTarget && CurrentTarget->Mode != 1)
        RestoreBehavior();                                    // +0x1ac
    UpdateMovingMembersRelPos();                              // 0x5a0d30
}

// PANZERS 0x59a280
void SPanzersSquadUnit::EC_ThrowMolotov(int unit, int p2)
{
    if (unit == WorldIndex || !IsTargetable(unit, true))
        return;
    if (HasSlotWeapon(2))                                     // 0x5ba820
        SquadUseItem(this, 2, unit, p2);
    if (CurrentTarget && CurrentTarget->Mode != 1)
        RestoreBehavior();                                    // +0x1ac
    UpdateMovingMembersRelPos();                              // 0x5a0d30
}

// PANZERS 0x59a3f0
void SPanzersSquadUnit::EC_AttackMove(float x, float z, int p3)
{
    // HD passes AL unchanged (garbage) when neither holds; only bit 0 is read.
    int flag = ((char)p3 == 0 && RequestedState != 0) ? 0 : 1;
    SUnit::EC_AttackMove(x, z, flag);                 // 0x5b8660
    if (RequestedState == -1 && CurrentTarget && CurrentTarget->Mode != 1)
        RestoreBehavior();                                    // +0x1ac
    UpdateMovingMembersRelPos();                              // 0x5a0d30
}

// PANZERS 0x59c2a0
// XP for hitting `victim` (crews x0.2 against squads and members); a kill
// adds a bonus: 20 for an armoured victim (50 / 100 and the flag +0x6c when
// it is set), else 10 (50 and +0x68 when that flag is set). Support squads
// get no kill bonus. A unit of an observer (kind 4) player moves to the
// local player first.
void SPanzersSquadUnit::AddXP(int victim, float xp, int p3)
{
    (void)p3;
    float* XPf = reinterpret_cast<float*>(&XP);               // +0x64 is a float in HD
    if (PlayerKind(Player) == 4)
        Player = g_World->LocalPlayer;                        // World+0x16c
    if (victim < 0) {
        *XPf = *XPf + xp;
        return;
    }
    float v;
    if (P->UnitType == 0xe &&
        (WorldUnit(victim)->Proto->ClassType == 5 || WorldUnit(victim)->Proto->ClassType == 6))
        v = xp * 0.2f + *XPf;                                 // DAT_007f1b4c
    else
        v = *XPf + xp;
    *XPf = v;
    if (!(0.0f >= WorldUnit(victim)->HP))
        return;
    bool flag;
    if (P->UnitType == 0xe) {
        float x = *XPf;
        if (WorldUnit(victim)->Proto->ArmourType == 2) {
            if (FirstArmouredVehicleKill) {
                FirstArmouredVehicleKill = true;
                *XPf = x + 50.0f;                             // DAT_007f4594
                return;
            }
            *XPf = x + 20.0f;                                 // DAT_007f35d8
            return;
        }
        flag = FirstKill;
    } else {
        if (P->SupportPlace)
            return;
        float x = *XPf;
        if (WorldUnit(victim)->Proto->ArmourType == 2) {
            if (FirstArmouredVehicleKill) {
                FirstArmouredVehicleKill = true;
                *XPf = x + 100.0f;                            // DAT_007ee558
                return;
            }
            *XPf = x + 20.0f;
            return;
        }
        flag = FirstKill;
    }
    if (!flag) {
        *XPf = *XPf + 10.0f;                                  // DAT_007f5a7c
        return;
    }
    FirstKill = true;
    *XPf = *XPf + 50.0f;
}

// PANZERS 0x59ca40
// A squad created during the game (SWorld::CreateUnit with a position):
// members created around `pos` at full health, no stored / global state.
void SPanzersSquadUnit::InitNew(int player, const float* pos, float dir, int p4, float hp)
{
    int n = P->MaxNumberOfUnits;
    // 0x546ca0 / 0x546af0: Members and MemberOrderDelay sized to n and cleared.
    Members.Size = n;
    if (Members.Max < n) {
        Members.Max = n;
        Members.Array = (SUnitMember*)realloc(Members.Array, n * sizeof(SUnitMember));
    }
    if (Members.Max)
        memset(Members.Array, 0, Members.Max * sizeof(SUnitMember));
    SetUnitSize();                                            // +0x1c4
    MemberOrderDelay.Size = n;
    if (MemberOrderDelay.Max < n) {
        MemberOrderDelay.Max = n;
        MemberOrderDelay.Array = (int*)realloc(MemberOrderDelay.Array, n * sizeof(int));
    }
    if (MemberOrderDelay.Max)
        memset(MemberOrderDelay.Array, 0, MemberOrderDelay.Max * sizeof(int));
    SetRelativePositions();                                   // inline 0x5a09d0
    int dirBits = FloatBits(dir);
    for (int i = 0; i < RelPos.Size; ++i) {
        float ox, oz;
        GetFormationOffset(i, &ox, &oz);
        if (i < 5) {                                          // HD writes +0x210 + 8 * i unchecked
            MemberOffset[i][0] = ox;
            MemberOffset[i][1] = oz;
        }
        int mdir = RelPosAt(i)->HasDir ? RelPosAt(i)->Dir : dirBits;
        if (i < 5)
            MemberDir[i] = mdir;
        if (i >= Members.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", i);
        Members.Array[i].Owner = WorldIndex;
        float mpos[3] = { ox + pos[0], pos[1], pos[2] + oz };
        float md;
        memcpy(&md, &mdir, 4);
        int m = g_World->CreateUnit(player, SStr(P->SquadMemberName), mpos, md, 0, 1.0f,
                                    WorldIndex, true, "");    // 0x5e3170
        Members.Array[i].Unit = m;
        if (m < 0) {
            Logger.g->Warning("SPanzersSquadUnit::Init - StoredMembers[i] < 0");
            if (Members.Size != 0 && !Members.Array)
                Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "struct SUnitMember");
            Members.Size = 0;
            if (Members.Max > 0)
                memset(Members.Array, 0, Members.Max * sizeof(SUnitMember));
            return;
        }
    }
    // HD: the squad's board elements (+0x380, +0x360..+0x37c); the recompile
    // creates none.
    _ec = GlobalState;                                        // +0xec = +0xe0
    SUnit::InitNew(player, pos, dir, p4, hp);                 // 0x5bace0
    if (g_GameLogic)
        RefreshTargeting();                                   // +0x34
}

// PANZERS 0x59d4e0
// A member left the squad (it died): its formation place goes, the squad
// shrinks (SetUnitSize) or dies with its last member (SUnit::EC_Die).
void SPanzersSquadUnit::RemoveMember(int unit)
{
    for (int i = 0; i < Members.Size; ++i) {
        if (Members.Array[i].Unit != unit)
            continue;
        WorldUnit(unit)->Parent = -1;                         // +0x78
        SetOnBlockMap(false);                                 // +0x198
        if (i >= RelPos.Size)
            Logger.g->Panic("SDArray<%s>::Remove: invalid index (%d) size = %d", "struct SSquadRelPos", i, RelPos.Size);
        RelPos.Size--;
        int rest = RelPos.Size - i;
        if (rest != 0)
            memmove(&RelPos.Array[i], &RelPos.Array[i + 1], rest * sizeof(SSquadRelPos));
        memset(&RelPos.Array[RelPos.Size], 0, sizeof(SSquadRelPos));
        RemoveStoredMember(i);                                // 0x5be140
        _268 = g_GameLogic->GetFrame();                       // +0x268 (0x56d1a0)
        if (Members.Size == 0) {
            SUnit::EC_Die();                                  // 0x5b8b10 (direct call)
            SetOnBlockMap(true);
            return;
        }
        SetUnitSize();                                        // +0x1c4
        SetOnBlockMap(true);
        return;
    }
}

// PANZERS 0x59d720
// Kind 9 / 10: the squad got into the vehicle / building (StoreUnit +0x5c);
// a human player's queued order goes on to the carrier. Kinds 0xe / 0xf /
// 0x10: delayed actions 2 (lay a tank mine, member 0 plays the animation),
// 3 and 4 (explosives) in 20 ticks (RefreshMisc 0x59e0d0).
void SPanzersSquadUnit::OnDriverReachedTarget()
{
    STarget* t = CurrentTarget;
    if (!t)
        Logger.g->Panic("SPanzersSquadUnit::OnDriverReachedTarget - No Currenttarget, unit:%s, idx:%d",
                        SStr(Proto->Name), WorldIndex);
    int kind = t->Kind;
    if (kind == 9 || kind == 10) {
        bool stored = WorldUnit(t->Unit)->StoreUnit(WorldIndex, kind == 9 ? 0 : 1);   // +0x5c
        if (stored) {
            STarget* pt = PrimaryTarget;
            STarget* ct = CurrentTarget;
            bool pass = pt && ct != pt && PlayerKind(Player) == 1;
            if (pass && kind == 9)
                pass = pt->Type != 0 || ct->Type != 0 || pt->Unit != ct->Unit;
            if (pass) {
                SUnit* carrier = WorldUnit(CurrentTarget->Unit);
                if (carrier->PrimaryTarget)
                    WorldUnit(CurrentTarget->Unit)->PrimaryTarget->Release();   // 0x5bdef0
                if (PrimaryTarget)
                    PrimaryTarget->AddRef();                  // 0x5b5a30
                WorldUnit(CurrentTarget->Unit)->PrimaryTarget = PrimaryTarget;
                WorldUnit(CurrentTarget->Unit)->SetCurrentTarget(PrimaryTarget, 0);   // +0xa0
                if (kind == 10) {
                    SUnit::OnDriverReachedTarget();           // 0x5bcb60
                    return;
                }
            }
            if (kind == 9 && PrimaryTarget) {
                PrimaryTarget->Release();                     // 0x5bdef0
                PrimaryTarget = nullptr;
                SUnit::OnDriverReachedTarget();               // 0x5bcb60
                return;
            }
        }
    } else if (kind == 0xe) {
        _18[0] = 2;                                           // +0x18 delayed action: tank mine
        _18[3] = 0x14;                                        // +0x24 ticks
        if (Members.Size > 0) {
            if (Members.Size < 1)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", 0);
            SUnit* m = WorldUnit(Members.Array[0].Unit);
            m->SetSpecialAnimation(GlobalState == 2 ? "lay_mine1" : "kneel_mine1");   // +0x1b8
            SUnit::OnDriverReachedTarget();
            return;
        }
    } else if (kind == 0xf) {
        _18[0] = 3;
        _18[3] = 0x14;
        SUnit::OnDriverReachedTarget();
        return;
    } else if (kind == 0x10) {
        _18[0] = 4;                                           // explosives
        _18[3] = 0x14;
    }
    SUnit::OnDriverReachedTarget();                           // 0x5bcb60
}

// PANZERS 0x59ef00
// An idle squad hit by `attacker` fights back: in weapon range it attack-
// moves to the attacker, beyond it attacks; a human player's AI-group
// squad assaults an attacking building; without a gunner that can answer it
// steps 10 m away from the attacker. Idle human allies in sight that can
// shoot come along (HD tests dx + dz * dz <= 100, dx not squared). Squads
// of an AI group alarm the group (0x5d68e0). An idle (or holding) squad
// goes prone (state 2) unless it is a support squad on its way, and every
// member's walker animation restarts its relax timer (a world RNG draw per
// member).
void SPanzersSquadUnit::OnAttackedBy(int attacker)
{
    if (!PrimaryTarget && !CurrentTarget && attacker > -1) {
        if (WorldUnit(attacker)->Proto->ClassType != 8 &&
            (Behavior == 0 || IsAIDefault()) &&                // 0x5bb470
            !PlayersAllied(Player, WorldUnit(attacker)->Player) &&   // 0x549ab0
            (AIGroup == -1 || AIGroupMode(AIGroup) != 5)) {
            SUnit* a = WorldUnit(attacker);
            bool canShoot = false;
            if (Gunners.Size > 0) {
                SUnit* a2 = WorldUnit(attacker);
                canShoot = GunnerCanAttackUnit(GetGunner(0), a2, true);   // 0x55cc40, 0x583b60
            }
            if (canShoot) {
                float dz = a->Pos[2] - Pos[2];
                float dx = a->Pos[0] - Pos[0];
                double r1 = (double)GetMaxRange(0) + 0.1;     // +0x17c, DAT_007f83e0 (x87)
                float d2 = dx * dx + dz * dz;
                double lim = ((double)GetMaxRange(0) + 0.1) * r1;
                if ((double)d2 > lim)
                    EC_Attack(attacker, 0);                   // +0xe8
                else
                    EC_AttackMove(a->Pos[0], a->Pos[2], 0);   // +0xcc
            } else if (PlayerKind(Player) == 1 && a->Proto->ClassType == 9 &&
                       static_cast<SPBuildingUnit*>(a->Proto)->BuildingType != 1 &&
                       AIGroup > -1 && AIGroupMode(AIGroup) == 2) {
                EC_AssaultBuilding(attacker, 1);              // +0xd4
            } else {
                float x = Pos[0], z = Pos[2];
                x = x > a->Pos[0] ? x + 10.0f : x - 10.0f;    // DAT_007f5a7c
                z = z > a->Pos[2] ? z + 10.0f : z - 10.0f;
                EC_AttackMove(x, z, 0); // +0xcc
            }
        }
    }
    if (attacker > -1) {
        if (AIGroup == -1) {
            if (WorldUnit(attacker)->Proto->ClassType != 8) {
                SUnit* a = WorldUnit(attacker);
                for (int i = 0; i < SightUnits.Size; ++i) {
                    if (!IsTargetable(SightUnits.Array[i].Unit, false))   // 0x5bb6b0
                        continue;
                    if (i >= SightUnits.Size)
                        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SNearUnit", i);
                    SUnit* o = WorldUnit(SightUnits.Array[i].Unit);
                    if (!PlayersAllied(o->Player, Player))
                        continue;
                    if (PlayerKind(o->Player) != 1 || o->PrimaryTarget || o->CurrentTarget)
                        continue;
                    if (o->Behavior != 0 && !o->IsAIDefault())
                        continue;
                    if (o->AIGroup != -1 || o->Gunners.Size <= 0)
                        continue;
                    if (!GunnerCanAttackUnit(o->GetGunner(0), a, true))
                        continue;
                    if (o->IsHiddenInBlockMap())              // 0x5bb5c0
                        continue;
                    float dz = o->Pos[2] - Pos[2];
                    float dx = o->Pos[0] - Pos[0];
                    if (dx + dz * dz > 100.0f)                // DAT_007ee558 (HD: dx not squared)
                        continue;
                    o->EC_AttackMove(a->Pos[0], a->Pos[2], 0);   // +0xcc
                }
            }
        } else if (AIGroup > -1) {
            if (WorldUnit(attacker)->Proto->ClassType != 8 && PlayerKind(Player) == 1 &&
                !PlayersAllied(Player, WorldUnit(attacker)->Player))
                g_World->AIGroupUnitAttacked(WorldIndex, attacker);   // 0x5d68e0
        }
    }
    STarget* t = CurrentTarget;
    if (t && !((t->Kind == 2 || t->Kind == 3) && t->Mode == 1))
        return;
    if (GlobalState != 2) {
        bool keep = (P->UnitType == 0x15 || P->SupportPlace) && t &&
                    (t->Kind == 8 || t->Kind == 2 || t->Kind == 3);
        if (!keep)
            SetSquadBehavior(2);                              // 0x599450
    }
    for (int i = 0; i < Members.Size; ++i)
        static_cast<SWalkerAnimation*>(MemberUnit(this, i)->Anim)->ResetRelax();   // 0x5cb0a0
}

// ---------------------------------------------------------------------------
// SPanzersSquadMemberUnit

// PANZERS 0x5988f0
void SPanzersSquadMemberUnit::GetClassDescriptor(void** obj, const SUnitClassDesc** desc)
{
    *obj = this;
    *desc = &kUnitClassDesc_8dbff0;
}

// PANZERS 0x598770
int SPanzersSquadMemberUnit::GetRank()
{
    return WorldUnit(Parent)->GetRank();                      // +0x78, tail call +0x88
}

// PANZERS 0x5987c0
// The member's hit points: the prototype's plus the squad rank's
// SquadHpLevel; above rank 4 HD answers HearingRange[3] (+0x74).
float SPanzersSquadMemberUnit::GetHitPoints()
{
    const SUnitRegistry* r = g_UnitRegistry;
    switch (GetRank()) {                                      // +0x88 (0x598770)
    case 0: return (float)r->SquadHpLevel[0] + P->HP;
    case 1: return (float)r->SquadHpLevel[1] + P->HP;
    case 2: return (float)r->SquadHpLevel[2] + P->HP;
    case 3: return (float)r->SquadHpLevel[3] + P->HP;
    case 4: return (float)r->SquadHpLevel[4] + P->HP;
    default: return (float)r->HearingRange[3];
    }
}

// PANZERS 0x598a20
// The dead member's tick: targets dropped, +0x104 / +0x108 and the script
// id cleared; then +0x15c ticks until it is removed from the world (0:
// +0x155 set and kept).
void SPanzersSquadMemberUnit::RefreshDead()
{
    if (CurrentTarget)
        ClearTargets();                                       // +0xc4
    _104 = 0;
    _108 = 0;
    FreeSString(&ScriptID);                                   // operator delete +0x194, +0x198 = 0
    if (_15c == 0) {
        _154 = (unsigned short)((_154 & 0x00ff) | 0x0100);    // +0x155 = 1
        return;
    }
    if (--_15c == 0) {
        Frozen = true;                                        // +0x153
        g_World->RemoveUnit(WorldIndex);                      // 0x5f8060
    }
}

// PANZERS 0x59dfe0
// The squad after its last member died (SUnit::EC_Die from RemoveMember):
// the vehicle it was going to enter (+0x18c) is released (+0x190), the
// script id cleared; the first tick starts the death (+0x155, the squad
// animation sets +0x15c = 1), the next removes the squad from the world.
// The recompile had no override (SUnit 0x5bd900 is empty), so dead squads
// stayed in the world.
void SPanzersSquadUnit::RefreshDead()
{
    int v = _18c;
    if (v > -1 && g_World->Units.IsLive(v)) {
        WorldUnit(v)->_190 = false;
        _18c = -1;
    }
    FreeSString(&ScriptID);                                   // operator delete +0x194, +0x198 = 0
    if (_15c == 0) {
        if (CurrentTarget)
            ClearTargets();                                   // +0xc4
        _154 = (unsigned short)((_154 & 0x00ff) | 0x0100);    // +0x155 = 1
        return;
    }
    if (--_15c == 0) {
        SetWreckModel();                                      // +0x28
        Frozen = true;                                        // +0x153
        g_World->RemoveUnit(WorldIndex);                      // 0x5f8060
    }
}

// PANZERS 0x598070
void SPanzersSquadMemberUnit::EC_ChangeActiveDriver(int driver)
{
    Stop();                                                   // +0xc0
    if (driver < Drivers.Size) {
        SetActiveDriver(driver);                              // 0x5c0cb0
        return;
    }
    Logger.g->Panic("SPanzersSquadMemberUnit::EC_ChangeActiveDriver: bad active_driver");
}

// PANZERS 0x598cb0
// A parachutist leaves the plane at height `y`: invulnerable, the
// "parachute" state, a random spot within 2 m (x, z), a random heading
// (+0..pi), pushed out of blocked space (0x5e58d0), a random height step
// (0..4 m below y), the parachute driver (1) and the canopy model shown.
void SPanzersSquadMemberUnit::ParachuteDrop(float y)
{
    Invulnerable = true;                                      // +0x111
    int st = Anim ? static_cast<SUnitAnimation*>(Anim)->Proto->FindState("parachute") : 0;   // 0x5c7ed0
    SetGlobalState(st, 0);                                    // 0x5b7390
    int r = WorldRand();
    Pos[0] = (float)((2.0 - (double)r * 3.0517578125e-05 * 4.0) + (double)Pos[0]);   // 0x7ea768, 0x7ea770
    r = WorldRand();
    Pos[2] = (float)((2.0 - (double)r * 3.0517578125e-05 * 4.0) + (double)Pos[2]);
    r = WorldRand();
    Dir = (float)((double)r * 3.0517578125e-05 * kHdPi + (double)Dir);   // 0x7f4560
    float out[2];
    g_World->FindEmptySpace(out, Pos[0], Pos[2], Dir, UnitSizeBlocks * 2, 0x1017, true);   // 0x5e58d0
    float a = Dir - 1.5707964f;                               // DAT_007f5a00
    Pos[0] = (float)((double)out[0] - DSin((double)a) * (double)UnitSizeBlocks * 0.25);   // 0x78d640, 0x7f5a10
    Pos[2] = (float)((double)out[1] - DCos((double)a) * (double)UnitSizeBlocks * 0.25);   // 0x78d480
    r = WorldRand();
    Pos[1] = y - (float)(int)((double)r * 3.0517578125e-05 * 5.0);   // 0x7f5a40, cvttsd2si
    SetActiveDriver(1);                                       // 0x5c0cb0
    RefreshModel();                                           // +0x3c
    StoreInterpolationState();                                // +0x16c
    Parachute->SetPosition(Pos[0], Pos[1], Pos[2]);           // +0x18
    Parachute->SetVisible(true, false);                       // +0x30
    Parachute->StoreInterpolationState();                     // +0x3c
}

} // namespace pz
