// src/game/unitai.cpp
// SUnit AI, targeting, order queue and effect timers (0x5b2800..0x5c6300),
// and the SSingleUnit per-tick slots that use them. OWNER: M2-I sub-agent UB.
//
// Bodies follow the HD disassembly: float expressions keep the SSE scalar
// order of the original (float unless HD converts to double), every world
// unit access goes through WorldUnit (the SHeapTRB panic), and the branches
// the menu never reaches end in STUB_LOG with the HD control flow around them.

#include <float.h>
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "unit.h"
#include "singleunit.h"
#include "squadunit.h"
#include "gunner.h"
#include "gunnermath.h"
#include "projectile.h"
#include "idriver.h"
#include "driver.h"
#include "manoeuvre.h"
#include "unitextern.h"
#include "drivermath.h"
#include "gamelogic.h"
#include "world.h"
#include "aigroup.h"
#include "target.h"
#include "worldapi.h"
#include "pz/imodel.h"
#include "pz/ipixie.h"
#include "pz/iviewport.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

// ---------------------------------------------------------------------------
// Helpers

static int PlayerType(int player)                             // World+0x178 + player * 0x48
{
    return *(int*)(g_World->Players[player] + 8);
}

static int FBits(float f)
{
    int i;
    memcpy(&i, &f, 4);
    return i;
}

static float BitsF(int i)
{
    float f;
    memcpy(&f, &i, 4);
    return f;
}

static int TKind(STarget* t) { return tgt::I(t, tgt::kKind); }
static int TType(STarget* t) { return tgt::I(t, tgt::kType); }

// HD inline: --RefCount, sized delete at 0, slot = 0.
static void DropTarget(STarget** slot)
{
    if (*slot) {
        PzTargetRelease(*slot);
        *slot = nullptr;
    }
}

// PANZERS 0x549ab0
// Allied players: the same team id (World+0x17c + player * 0x48), or the
// same player when the team id is 0.
static bool SameSide(int a, int b)
{
    int team = *(int*)(g_World->Players[a] + 0x0c);
    if (team != 0)
        return team == *(int*)(g_World->Players[b] + 0x0c);
    return a == b;
}

static void ArrayPanic(const char* type, int i)
{
    Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", type, i);
}

// HD SDArray<T>::Add (0x5b5930 for 8 bytes, 0x5b59a0 for 0x1c, inline for
// 0x10): grows by 6/5 (at least 0x10), zeroes the new tail.
template <typename T>
static int AddElem(SUnitArray<T>* a)
{
    if (a->Size == a->Max) {
        int nmax = a->Max < 0x10 ? 0x10 : (a->Max * 6) / 5;
        a->Array = (T*)realloc(a->Array, nmax * sizeof(T));
        memset((void*)&a->Array[a->Max], 0, (nmax - a->Max) * sizeof(T));
        a->Max = nmax;
    }
    return a->Size++;
}

// HD SDArray<T>::Clear(n) (0x5b7550 for 8 bytes, 0x5b75c0 for 0x1c,
// 0x550a30 for the 8-byte driver points): size = n, zero the whole array.
template <typename T>
static void ClearElems(SUnitArray<T>* a, int n, const char* type)
{
    if (a->Size != 0 && a->Array == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", type);
    a->Size = n;
    if (a->Max < n) {
        a->Max = n;
        a->Array = (T*)realloc(a->Array, n * sizeof(T));
    }
    memset((void*)a->Array, 0, a->Max * sizeof(T));
}

// HD SDArray<T>::Remove (0x5bdfa0 for 0x1c, 0x5bdf10 for 0x10).
template <typename T>
static void RemoveElem(SUnitArray<T>* a, int i, const char* type)
{
    if (i < 0 || i >= a->Size)
        Logger.g->Panic("SDArray<%s>::Remove: invalid index (%d) size = %d", type, i, a->Size);
    a->Size--;
    int n = a->Size - i;
    if (n != 0)
        memmove(&a->Array[i], &a->Array[i + 1], n * sizeof(T));
    memset((void*)&a->Array[a->Size], 0, sizeof(T));
}

static int MemberUnit(SUnit* u, int i)                        // 0x5991e0 (SDArray<SUnitMember>)
{
    if (i < 0 || i >= u->Members.Size)
        ArrayPanic("struct SPanzersSquadMember", i);
    return u->Members.Array[i].Unit;
}

static int StoredUnit(SUnit* u, int i)                        // 0x546450 (SDArray<SStoredUnit>)
{
    if (i < 0 || i >= u->Stored.Size)
        ArrayPanic("struct SStoredUnit", i);
    return u->Stored.Array[i].Unit;
}

static SDriver* AsDriver(SIDriver* d) { return static_cast<SDriver*>(d); }

// ---------------------------------------------------------------------------
// Small queries

// PANZERS 0x5bb400
// A unit whose driver got stuck twice or more waits StuckCount * 5 ticks.
bool SUnit::IsWaitingAfterStuck()
{
    if (StuckCount < 2)
        return false;
    int frame = g_GameLogic->GetFrame();                      // 0x56d1a0
    return StuckCount * 5 + StuckFrame > frame;
}

// PANZERS 0x5bb5c0
bool SUnit::IsHiddenInBlockMap()
{
    if (MoveFlags & 1)
        return false;
    int size = UnitSizeBlocks < 1 ? 1 : UnitSizeBlocks;
    return (unsigned char)TestBlockMap(FBits(Pos[0]), FBits(Pos[2]), FBits(Dir), size, 1) != 0;   // +0x1a8
}

// PANZERS 0x5bb470
bool SUnit::IsAIDefault()
{
    return PlayerType(Player) == 1 && AIGroup == -1 && Behavior == 1;
}

// PANZERS 0x5ba820
bool SUnit::HasSlotWeapon(int weapon)
{
    return _138 == weapon || _144 == weapon;
}

// PANZERS 0x5bb6b0
// A live, placed unit that is not marked invalid (+0x1c0) for attacking
// (follow = false, flag +0x0c) or following (follow = true, flag +0x0d).
bool SUnit::IsTargetable(int unit, bool follow)
{
    if (!g_World->Units.IsLive(unit))
        return false;
    if (WorldUnit(unit)->Unplaced)
        return false;
    for (int i = 0; i < InvalidTargets.Size; ++i) {
        const SInvalidTarget& e = InvalidTargets.Array[i];
        if (e.Unit != unit)
            continue;
        if (!(follow ? e.FollowOk : e.AttackOk))
            return false;
    }
    return true;
}

// PANZERS 0x5b9bf0
bool SUnit::GetMovingDriver(SIDriver** out)
{
    if (RefreshDriver >= 0) {
        if (RefreshDriver >= Drivers.Size)
            ArrayPanic("class SDriver *", RefreshDriver);
        *out = Drivers.Array[RefreshDriver];
        return true;
    }
    if (ActiveDriver >= 0 && ActiveDriver < Drivers.Size) {
        *out = GetDriver(ActiveDriver);                       // 0x55cc00
        return true;
    }
    *out = nullptr;
    return false;
}

// PANZERS 0x5b9c70
bool SUnit::GetActivePDriver(SIPDriver** out)
{
    int i = RefreshDriver;
    if (i < 0) {
        i = ActiveDriver;
        if (i < 0) {
            *out = nullptr;
            return false;
        }
    }
    if (i >= Drivers.Size)
        ArrayPanic("class SDriver *", i);
    *out = Drivers.Array[i]->GetPDriver();                    // driver +0x04
    return true;
}

// PANZERS 0x5b9ce0
bool SUnit::GetMainPGunner(SPGunner** out)
{
    if (MainGunner < 0) {
        *out = nullptr;
        return false;
    }
    if (MainGunner >= Gunners.Size)
        ArrayPanic("class SGunner *", MainGunner);
    *out = Gunners.Array[MainGunner]->GetPGunner();           // gunner +0x2c
    return true;
}

// PANZERS 0x5b6fd0
bool SUnit::IsPosInGunnerArc(float x, float y, float z)
{
    if (MainGunner <= -1)
        return false;
    if (MainGunner >= Gunners.Size)
        ArrayPanic("class SGunner *", MainGunner);
    return Gunners.Array[MainGunner]->IsInArc(x, y, z);       // 0x583990
}

// ---------------------------------------------------------------------------
// Near-unit lists

// PANZERS 0x5b78a0
// Rebuilds +0x1b4 (every unit of a fighting class within sightRange) and
// +0x1a8 (ground units within 2 * nearRange; Blocked = their footprint,
// marked on the dynamic block map for the test, overlaps this unit's).
void SUnit::FillNearUnits(float sightRange, float nearRange)
{
    if (_33c)
        return;
    ClearElems(&NearUnits, 0, "struct SUnit::SNearUnit");    // 0x5b7550(0)
    ClearElems(&SightUnits, 0, "struct SUnit::SNearUnit");
    for (int i = 0; i < g_World->Units.Size; ++i) {
        if (g_World->Units.Array[i].Next != kHeapLive)
            continue;
        SUnit* u = g_World->Units.Array[i].Unit;
        if (i == WorldIndex)
            continue;
        int ct = u->Proto->ClassType;
        if (ct == 0 || ct == 0xb || ct == 5 || ct == 0xc || ct == 10 || ct == 7 || ct == 8 || ct == 9) {
            float dx = Pos[0] - u->Pos[0];
            float dy = Pos[1] - u->Pos[1];
            float dz = Pos[2] - u->Pos[2];
            float d = dx * dx + dy * dy;
            d = d + dz * dz;
            if (sightRange * sightRange > d) {
                int k = AddElem(&SightUnits);                 // 0x5b5930
                if (k < 0 || k >= SightUnits.Size)
                    ArrayPanic("struct SUnit::SNearUnit", k);
                SightUnits.Array[k].Unit = i;
            }
        }
        if (i == WorldIndex)
            continue;
        ct = u->Proto->ClassType;
        if (!(ct == 0 || ct == 0xb || ct == 5 || ct == 0xc || ct == 10))
            continue;
        float dx = Pos[0] - u->Pos[0];
        float dy = Pos[1] - u->Pos[1];
        float dz = Pos[2] - u->Pos[2];
        float r = nearRange * 2.0f;                           // DAT_007f4558
        float d = dx * dx + dy * dy;
        r = r * r;
        d = d + dz * dz;
        if (!(r > d))
            continue;
        int k = AddElem(&NearUnits);
        if (k < 0 || k >= NearUnits.Size)
            ArrayPanic("struct SUnit::SNearUnit", k);
        NearUnits.Array[k].Unit = i;
        float ox = u->Pos[0] - Pos[0];
        float oy = u->Pos[1] - Pos[1];
        float oz = u->Pos[2] - Pos[2];
        double rr = (double)(u->UnitSize + UnitSize) + 0.5;   // DAT_007ea760
        float od = ox * ox + oy * oy;
        rr = rr * rr;
        od = od + oz * oz;
        rr = rr * 0.25;                                       // DAT_007f5a10
        if (!(rr > (double)od))
            continue;
        u->MarkBlockMap(true, FBits(u->Pos[0]), FBits(u->Pos[2]), FBits(u->Dir), 0x40);          // +0x1a0
        unsigned char blocked = (unsigned char)TestBlockMap(FBits(Pos[0]), FBits(Pos[2]), FBits(Dir),
                                                            UnitSizeBlocks2, 0x40);              // +0x1a8
        u->MarkBlockMap(false, FBits(u->Pos[0]), FBits(u->Pos[2]), FBits(u->Dir), 0x40);
        if (blocked) {
            if (k >= NearUnits.Size)                          // 0x5463d0
                ArrayPanic("struct SUnit::SNearUnit", k);
            NearUnits.Array[k].Blocked = true;
        }
    }
}

// ---------------------------------------------------------------------------
// Targeting

// PANZERS 0x5b4720
// Picks the target for a gunner among the units in sight (+0x1b4): mode 0 the
// nearest in [minRange, maxRange], 1 the nearest unless a candidate kills this
// unit in fewer shots (+0x90 ShotsToKill, 99) than the current pick, 2 the
// lowest (shots it takes * shots it gives * its reload time). canMove lets the
// main gunner of a mobile unit take targets out of its line of fire.
int SUnit::FindTarget(int mode, SGunner* gunner, float minRange, float maxRange, bool canMove)
{
    float bestScore = FLT_MAX;                                // DAT_007fa570
    SUnit* best = nullptr;
    float bestD = maxRange * maxRange;
    for (int i = 0; i < SightUnits.Size; ++i) {
        if (!IsTargetable(SightUnits.Array[i].Unit, false))   // 0x5bb6b0
            continue;
        SUnit* u = WorldUnit(SightUnits.Array[i].Unit);
        if (u == this || u->Wrecked || u->Unplaced || u->_110 || u->_2cc[Player] || u->Invulnerable)
            continue;
        if (SameSide(Player, u->Player))                      // 0x549ab0
            continue;
        int pt = PlayerType(u->Player);
        if (pt == 3 || pt == 2 || pt == 4)
            continue;
        if (!IsTargetable(u->WorldIndex, false))
            continue;
        if (!g_GameLogic->CanSeeGroundUnit(Player, u))        // 0x562760
            continue;
        if (!gunner->CanTargetUnit(u, canMove))               // 0x583b60
            continue;
        bool los = gunner->HasLineOfFire(u->WorldIndex);      // 0x5847e0
        if (!los && !canMove)
            continue;
        bool mobile = ActiveDriver > -1 && (!PrimaryTarget || TKind(PrimaryTarget) == 4);
        if (!los && ((Behavior != 0 && !IsAIDefault()) || !mobile))
            continue;
        float dx = u->Pos[0] - Pos[0];
        float dy = u->Pos[1] - Pos[1];
        float dz = u->Pos[2] - Pos[2];
        float d = dx * dx + dy * dy + dz * dz;
        if (mode == 0) {
            if (bestD <= d)
                continue;
            if (d <= minRange * minRange && u->Proto->ClassType != 8)
                continue;
            bestD = d;
            best = u;
        } else if (mode == 1) {
            if (!best) {
                bestD = d;
                best = u;
                continue;
            }
            float from1[3] = { u->Pos[0], u->Pos[1], u->Pos[2] };
            float byBest = (float)ShotsToKill(from1, best->WorldIndex);   // +0x90
            float from2[3] = { u->Pos[0], u->Pos[1], u->Pos[2] };
            int byThis = ShotsToKill(from2, u->WorldIndex);
            if (byBest <= 99.0f || 99.0f < (float)byThis) {   // DAT_007fd708
                if (d < bestD && (minRange * minRange < d || u->Proto->ClassType == 8)) {
                    bestD = d;
                    best = u;
                }
            } else {
                bestD = d;
                best = u;
            }
        } else if (mode == 2) {
            float ez = u->Pos[2] - Pos[2];
            float ex = u->Pos[0] - Pos[0];
            float d2 = ex * ex + ez * ez;
            if (!(d2 < maxRange * maxRange && (minRange * minRange < d2 || u->Proto->ClassType == 8)))
                continue;
            float from1[3] = { u->Pos[0], u->Pos[1], u->Pos[2] };
            float taken = (float)ShotsToKill(from1, u->WorldIndex);   // +0x90
            float given;
            if (taken == 0.0f) {
                given = 0.0f;
            } else {
                float from2[3] = { Pos[0], Pos[1], Pos[2] };
                given = (float)u->ShotsToKill(from2, WorldIndex);
            }
            float reload = 10000.0f;                          // DAT_007fd710
            if (u->MainGunner != -1) {
                if (u->Proto->ClassType == 9)
                    (void)WorldUnit(StoredUnit(u, 0));        // HD reads it (heap check only)
                reload = (float)u->GetGunner(0)->GetPGunner()->ReloadTime;   // SPGunner +0x50
            }
            if (u->Proto->ClassType == 5 && u->Members.Size > 0) {
                SUnit* m = WorldUnit(MemberUnit(u, 0));
                if (m->MainGunner != -1) {
                    (void)WorldUnit(MemberUnit(u, 0));
                    reload = (float)m->GetGunner(0)->GetPGunner()->ReloadTime;
                }
            }
            float score = given * taken * reload;
            if (score < bestScore || (score == bestScore && d2 < bestD)) {
                bestD = d;
                bestScore = score;
                best = u;
            }
        }
    }
    return best ? best->WorldIndex : -1;
}

// PANZERS 0x5b5090
// An AI squad without a path order looks for an empty armed vehicle (class 0
// / 0xb) of its own side, near the map origin test of HD (0.75 * sight^2
// against the squad's distance from (0, 0)), that it can enter: the remembered
// one (+0x18c) if it still stands, else the nearest in sight.
int SUnit::FindVehicleToEnter()
{
    if (PlayerType(Player) != 1 || Proto->ClassType != 5)
        return -1;
    if (PrimaryTarget && tgt::I(PrimaryTarget, tgt::kPath) >= 0 && PrimaryTarget->HasPathAhead()) {   // 0x5bb630
        SHeap<SPath>& paths = g_World->Paths;                 // 0x560960
        int path = tgt::I(PrimaryTarget, tgt::kPath);
        if (!paths.IsLive(path))
            Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SPath", path);
        if (paths.Array[path].Data.Closed)
            return -1;
    }
    if (_18c != -1 && g_World->Units.IsLive(_18c) && !WorldUnit(_18c)->Wrecked)
        return _18c;
    float s1 = GetSightRange();                               // +0x184
    float s2 = GetSightRange();
    if (!((float)(s2 * 0.75f * (float)(s1 * 0.75f)) <= Pos[0] * Pos[0] + Pos[2] * Pos[2]))   // DAT_007f2fcc
        return -1;
    SUnit* best = nullptr;
    float a = GetSightRange();
    float bestD = (float)(GetSightRange() * a);
    for (int i = 0; i < SightUnits.Size; ++i) {
        if (!IsTargetable(SightUnits.Array[i].Unit, false))
            continue;
        SUnit* v = WorldUnit(SightUnits.Array[i].Unit);
        if (v == this || v->Wrecked || v->Unplaced || v->CurrentTarget)
            continue;
        if (!v->_110 && v->Player != Player)
            continue;
        int ct = v->Proto->ClassType;
        if (ct != 0 && ct != 0xb)
            continue;
        int pt = PlayerType(v->Player);
        if (pt == 3 || pt == 4)
            continue;
        if (!v->CanStoreUnit(WorldIndex))                     // 0x5b7040
            continue;
        if (v->_190 || v->Gunners.Size <= 0)
            continue;
        if (!IsTargetable(v->WorldIndex, false))
            continue;
        if (!g_GameLogic->CanSeeGroundUnit(Player, v))        // 0x562760
            continue;
        float d = (v->Pos[0] - Pos[0]) * (v->Pos[0] - Pos[0]) + (v->Pos[2] - Pos[2]) * (v->Pos[2] - Pos[2]);
        if (d < bestD) {
            bestD = d;
            best = v;
        }
    }
    return best ? best->WorldIndex : -1;
}

// PANZERS 0x5b53d0
// Attacks `unit` with gunner `gunnerIdx`: a single gunner of a vehicle / gun
// gets its own target (STarget kind 3, unit order); the main gunner of a
// mobile unit makes the unit attack (kind 2 for AI defenders and passive
// units: they remember where they stood, +0x2f4..+0x2fc), then a squad throws
// its item (+0xf4 molotov, +0xf0 grenade).
void SUnit::AttackUnit(int unit, int gunnerIdx, bool canMove)
{
    if (gunnerIdx != MainGunner || !canMove) {
        int ct = Proto->ClassType;
        if (ct != 0 && ct != 0xb)
            return;
        if (gunnerIdx < 0 || gunnerIdx >= Gunners.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", gunnerIdx);
        STarget* gt = Gunners.Array[gunnerIdx]->Target;       // +0x14
        if (gt && TType(gt) == 0 && GetGunner(gunnerIdx)->Target->Unit == unit)
            return;
        STarget* t = STarget::Create(3);                      // new 0x38, 0x5b27c0(3)
        t->Type = 0;
        t->Unit = unit;
        t->Mode = 1;
        Gunners.Array[gunnerIdx]->SetTarget(t);               // gunner +0x18
        return;
    }
    STarget* cur = CurrentTarget;
    if (cur) {
        if (TType(cur) == 0 && cur->Unit == unit) {
            if (Behavior == 0) {
                if (TKind(cur) == 2)
                    return;
            } else if (TKind(cur) == 3) {
                return;
            }
        }
        if (TType(cur) == 0 && cur->Unit == unit)
            return;
    }
    STarget* t;
    if (PlayerType(Player) == 1 && AIGroup == -1 && Behavior == 1 && WorldUnit(unit)->Proto->ClassType != 8) {
        t = STarget::Create(2);
        if (!HasReturnPos) {
            ReturnX = Pos[0];
            ReturnZ = Pos[2];
            HasReturnPos = true;
        }
    } else if ((Behavior == 0 || (PlayerType(Player) == 1 && PrimaryTarget && TKind(PrimaryTarget) == 4)) &&
               WorldUnit(unit)->Proto->ClassType != 8) {
        t = STarget::Create(2);
    } else {
        t = STarget::Create(3);
    }
    t->Type = 0;
    t->Unit = unit;
    t->Mode = 1;
    SetCurrentTarget(t, 0);                                   // +0xa0
    if (PlayerType(Player) == 1 && AIGroup == -1 && Behavior == 1) {
        bool keep = false;
        if (PrimaryTarget) {
            if (tgt::I(PrimaryTarget, tgt::kPath) != -1)
                keep = true;                                  // HD keeps a path order
            else
                DropTarget(&PrimaryTarget);
        }
        if (!keep) {
            if (CurrentTarget)
                CurrentTarget->AddRef();
            PrimaryTarget = CurrentTarget;
        }
    }
    bool item2 = (_138 == 2 && _140) || (_144 == 2 && _14c);
    bool item1 = (_138 == 1 && _140) || (_144 == 1 && _14c);
    if (!item2 && !item1)
        return;
    float lr = GetLowestMaxRange();                           // +0x178
    bool farAway = false;
    if (GlobalState != 0) {                                   // +0xe0
        float dz = Pos[2] - CurrentTarget->Pos[2];
        float dx = Pos[0] - CurrentTarget->Pos[0];
        if ((float)((int)lr * (int)lr) < dx * dx + dz * dz)   // cvttss2si
            farAway = true;
    }
    if (item2)
        EC_ThrowMolotov(unit, farAway);                           // +0xf4
    else
        EC_ThrowGrenade(unit, farAway);                           // +0xf0
}

// PANZERS 0x5b5810
// Orders this squad into vehicle `unit` (STarget kind 9), or, when the vehicle
// takes it at once (+0x58), lets the vehicle pick it up (+0xd8) and stops.
// The vehicle is marked taken (+0x190) and remembered (+0x18c).
void SUnit::EnterVehicle(int unit)
{
    if (CurrentTarget && CurrentTarget->Unit == unit)
        return;
    if (!g_World->Units.IsLive(unit))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", unit);
    // +0x58 gets the vehicle's unit pointer; it answers false for every
    // class but SSingleUnit (0x5aaa30), and only squads come here.
    if (!Slot_58((int)(intptr_t)WorldUnit(unit))) {
        STarget* t = STarget::Create(9);
        t->Type = 0;
        t->Unit = unit;
        SetCurrentTarget(t, 0);                               // +0xa0
    } else {
        STUB_LOG("SUnit::EnterVehicle (0x5b5810) vehicle +0xd8(unit) pick-up, then +0xc4");
        ClearTargets();                                       // +0xc4
    }
    WorldUnit(unit)->_190 = true;
    _18c = unit;
}

// PANZERS 0x5bc840
bool SUnit::NeedsRepair()
{
    STUB_LOG("SUnit::NeedsRepair (0x5bc840)");
    return false;
}

// PANZERS 0x5bc700
bool SUnit::NeedsSupply(float level)
{
    if (Gunners.Size == 0)
        return false;
    if (Gunners.Size < 1)
        ArrayPanic("class SGunner *", 0);
    if (Gunners.Array[0]->GetWeaponType() == 0)               // 0x584240
        return false;
    int ct = Proto->ClassType;
    if (ct != 0 && ct != 0xb)
        return false;
    float extra = 0.0f;
    SGunner* g = Gunners.Array[0];
    if (g->Loaded)
        extra = 1.0f / (float)g->GetPGunner()->Ammo;          // DAT_007f1b58 / (float)Ammo
    return level > g->AmmoLeft + extra;
}

// PANZERS 0x5bd610
// Repair and supply vehicles look for an allied unit in the sight list that
// needs them, unless busy with an order.
void SUnit::AutoRepairSupply(float supplyLevel)
{
    if (!_2eb)
        return;
    if (!(Cargo > 0.0f))
        return;
    STarget* ct = CurrentTarget;
    if (ct) {
        int k = TKind(ct);
        if (k == 6 || k == 7 || k == 8 || k == 1)
            return;
    }
    STarget* pt = PrimaryTarget;
    bool primFollow = pt && TKind(pt) == 0 && TType(pt) == 0;
    bool curFollow = ct && TKind(ct) == 0 && TType(ct) == 0;
    bool primKind4 = pt && TKind(pt) == 4;
    bool none = !pt && !ct;
    if (!none && !primKind4 && !(primFollow && curFollow))
        return;
    for (int i = 0; i < SightUnits.Size; ++i) {
        if (!IsTargetable(SightUnits.Array[i].Unit, false))
            continue;
        SUnit* u = WorldUnit(SightUnits.Array[i].Unit);
        int uc = u->Proto->ClassType;
        if (uc == 7 || uc == 8 || uc == 9)
            continue;
        if (!SameSide(Player, u->Player) || u->_110 || !Proto->Repairer)
            continue;
        if (u->NeedsRepair()) {                               // 0x5bc840
            STUB_LOG("SUnit::AutoRepairSupply (0x5bd610) EC_Repair 0x5c1ca0");
            return;
        }
    }
    for (int i = 0; i < SightUnits.Size; ++i) {
        if (!IsTargetable(SightUnits.Array[i].Unit, false))
            continue;
        SUnit* u = WorldUnit(SightUnits.Array[i].Unit);
        int uc = u->Proto->ClassType;
        if (uc == 7 || uc == 8 || uc == 9)
            continue;
        if (!SameSide(Player, u->Player) || u->_110 || !Proto->Supporter)
            continue;
        if (u->NeedsSupply(supplyLevel)) {                    // 0x5bc700
            STUB_LOG("SUnit::AutoRepairSupply (0x5bd610) EC_Supply 0x5c1e60");
            return;
        }
    }
}

// PANZERS 0x5ba2b0 (entry tests)
// What a unit ordered into a building does there (1 enter, 3 attack, 4 get
// in). Only buildings answer; the building part (0x56d2a0, 0x583b60,
// 0x5b7040) is not lifted: no menu order targets a building.
int SUnit::GetBuildingAction(int unit)
{
    if (!g_GameLogic)
        return -1;
    if (!g_World->Units.IsLive(unit))
        return -1;
    if (WorldUnit(unit)->Proto->ClassType != 9)
        return -1;
    STUB_LOG("SUnit::GetBuildingAction (0x5ba2b0) building");
    return -1;
}

// ---------------------------------------------------------------------------
// AI heartbeat

// PANZERS 0x5b37d0
// The unit's own decisions: take up the primary order again, AI units walk
// back to their post, then each automatic gunner looks for a target
// (0x5b4720); without one the main gunner tries the special weapons and the
// AI assault, and an idle unit resumes its primary order.
void SUnit::AI_Heartbeat()
{
    PZ_M2_TRACE("SUnit::AI_Heartbeat (0x5b37d0)");
    if (StuckCount >= 2 && StuckCount * 5 + StuckFrame > g_GameLogic->GetFrame())
        return;
    if (Unplaced || Wrecked)
        return;
    if (g_GameLogic && g_GameLogic->IsPaused() && !_1d4)
        return;
    if (Behavior == 2 && !CurrentTarget && PrimaryTarget) {
        int k = TKind(PrimaryTarget);
        if (k != 4 && k != 5 && k != 0xd)
            SetCurrentTarget(PrimaryTarget, 0);               // +0xa0
    }
    int ptype = PlayerType(Player);
    if (ptype == 2 || _110)
        return;
    if (Behavior == 2 && !(PrimaryTarget && TKind(PrimaryTarget) == 4))
        return;
    if (ptype == 1 && CurrentTarget && TKind(CurrentTarget) == 9)
        return;
    if (IsAIDefault() && PrimaryTarget && tgt::I(PrimaryTarget, tgt::kPath) > -1)
        HasReturnPos = false;
    if (IsAIDefault() && HasReturnPos) {
        float rz = ReturnZ;
        float rx = ReturnX;
        float dz = Pos[2] - rz;
        float dx = Pos[0] - rx;
        dz = dz * dz;
        dx = dx * dx;
        if (dx + dz > 400.0f) {                               // DAT_007fd70c
            HasReturnPos = false;
            EC_Move(FBits(rx), FBits(rz), 0, false, 0);       // +0xac
        }
    }
    bool canMove = ActiveDriver > -1 && (!PrimaryTarget || TKind(PrimaryTarget) == 4);
    bool noMoveOrder = true;
    if (PrimaryTarget) {
        int k = TKind(PrimaryTarget);
        if (k == 2 || k == 3 || k == 5)
            noMoveOrder = false;
    }
    if (MainGunner == -1 && Proto->ClassType == 0 && PlayerType(Player) == 1 && Members.Size > 0 &&
        WorldUnit(MemberUnit(this, 0))->Gunners.Size > 0) {
        // An unarmed AI vehicle fights with its crew's weapon.
        float minR = WorldUnit(MemberUnit(this, 0))->GetGunner(0)->GetPGunner()->MinRange;
        float maxR = WorldUnit(MemberUnit(this, 0))->GetGunner(0)->GetPGunner()->MaxRange;
        if (Towed >= 0 && WorldUnit(Towed)->Gunners.Size > 0)
            maxR = WorldUnit(Towed)->GetGunner(0)->GetPGunner()->MaxRange;
        if (WorldUnit(StoredUnit(this, 0))->Behavior == 0 || IsAIDefault()) {
            float s = WorldUnit(StoredUnit(this, 0))->GetSightRange();   // +0x184
            if (!(maxR > s))
                maxR = WorldUnit(StoredUnit(this, 0))->GetSightRange();
            minR = 0.0f;
        }
        maxR = maxR * 0.8f;                                   // DAT_007f83dc
        int mode = PlayerType(Player) == 1 ? 2 : 1;
        SGunner* g = WorldUnit(MemberUnit(this, 0))->GetGunner(0);
        if (FindTarget(mode, g, minR, maxR, true) >= 0) {
            if (Towed < 0)
                Unload(-1);                                   // +0x144(-1): the crew gets out
            else
                Slot_154();                                   // +0x154 (towed gun)
        }
    }
    for (int i = 0; i < Gunners.Size; ++i) {
        if (GetGunner(i)->Active == 0)
            continue;
        if (!GetGunner(i)->GetPGunner()->AutoShot)
            continue;
        int mode = PlayerType(Player) == 1 ? 2 : 1;
        float minR = GetMinRange(i);                          // +0x180
        float maxR = GetMaxRange(i);                          // +0x17c
        if (i == MainGunner) {
            if (!noMoveOrder)
                continue;
            if ((Behavior == 0 || IsAIDefault()) && canMove) {
                float s = GetSightRange();                    // +0x184
                if (!(maxR > s))
                    maxR = GetSightRange();
                minR = 0.0f;
            }
        }
        bool main = i == MainGunner && canMove;
        int found = FindTarget(mode, GetGunner(i), minR, maxR, main);
        if (found >= 0) {
            SUnit* t = WorldUnit(found);
            if (PlayerType(Player) == 1) {
                if (Proto->ClassType == 5) {
                    // An AI squad may rather man an empty armed vehicle that
                    // survives the target longer (0x5b5090 / 0x5b5810).
                    int v = FindVehicleToEnter();
                    if (v >= 0 && WorldUnit(v)->Gunners.Size > 0 && _18c != v) {
                        float p1[3] = { Pos[0], Pos[1], Pos[2] };
                        float p2[3] = { Pos[0], Pos[1], Pos[2] };
                        int withVehicle = t->ShotsToKill(p1, WorldUnit(v)->WorldIndex);   // +0x90
                        int withSelf = t->ShotsToKill(p2, WorldIndex);
                        if (withVehicle < withSelf) {
                            AttackUnit(found, i, canMove);    // 0x5b53d0
                            EnterVehicle(v);                  // 0x5b5810
                            return;
                        }
                    }
                } else if (Proto->ClassType == 0xb && Members.Size > 0) {
                    // A gun crew may rather use the gun's own carrier weapon.
                    SUnit* stored = WorldUnit(StoredUnit(this, 0));
                    SUnit* m1 = WorldUnit(MemberUnit(this, 0));
                    SUnit* m2 = WorldUnit(MemberUnit(this, 0));
                    float d = (t->Pos[0] - Pos[0]) * (t->Pos[0] - Pos[0]) + (t->Pos[1] - Pos[1]) * (t->Pos[1] - Pos[1]) +
                              (t->Pos[2] - Pos[2]) * (t->Pos[2] - Pos[2]);
                    float r = m1->GetMaxRange(0);             // +0x17c(0)
                    r = m2->GetMaxRange(0) * r;
                    if (d < r) {
                        float p1[3] = { Pos[0], Pos[1], Pos[2] };
                        float p2[3] = { Pos[0], Pos[1], Pos[2] };
                        int withStored = t->ShotsToKill(p1, stored->WorldIndex);
                        int withSelf = t->ShotsToKill(p2, WorldIndex);
                        if (withStored < withSelf) {
                            Unload(-1);                       // +0x144(-1)
                            return;
                        }
                    }
                }
            }
            AttackUnit(found, i, canMove);                    // 0x5b53d0
            continue;
        }
        if (i != MainGunner)
            continue;
        if (Proto->ClassType == 5 && Members.Size > 0 &&
            (((_138 == 2 || _138 == 3 || _138 == 1) && _140) ||
             ((_144 == 2 || _144 == 3 || _144 == 1) && _14c))) {
            SGunner* g = WorldUnit(MemberUnit(this, 0))->GetGunner(1);
            int e = FindTarget(0, g, 0.0f, maxR, canMove);
            if (e >= 0) {
                // Item 2 (+0xf4), then 3 (+0xf8), then 1 (+0xf0); "far" when
                // the target is beyond the lowest weapon range (int squared).
                int s1 = _138, s2 = _144;
                int kind;
                if ((s1 == 2 && _140) || (s2 == 2 && _14c))
                    kind = 2;
                else if ((s1 == 3 && _140) || (s2 == 3 && _14c))
                    kind = 3;
                else if ((s1 == 1 && _140) || (s2 == 1 && _14c))
                    kind = 1;
                else
                    return;
                float lr = GetLowestMaxRange();               // +0x178
                bool farAway = false;
                if (GlobalState != 0) {                       // +0xe0
                    SUnit* tu = WorldUnit(e);
                    float dz = Pos[2] - tu->Pos[2];
                    float dz2 = dz * dz;
                    float dx = Pos[0] - tu->Pos[0];
                    if ((float)((int)lr * (int)lr) < dx * dx + dz2)   // cvttss2si
                        farAway = true;
                }
                if (kind == 2)
                    EC_ThrowMolotov(e, farAway);                  // +0xf4
                else if (kind == 3)
                    EC_ThrowMagneticMine(e, farAway);             // +0xf8
                else
                    EC_ThrowGrenade(e, farAway);                  // +0xf0
                return;
            }
        }
        if (!CurrentTarget && Proto->ClassType == 5 && PlayerType(Player) == 1 && AIGroup > -1 &&
            AIGroupAt(AIGroup)->Tactic == 2) {
            // An assaulting AI squad takes the nearest enemy building it sees.
            SUnit* best = nullptr;
            float s = GetSightRange();                        // +0x184
            float bestD = GetSightRange() * s;
            for (int k = 0; k < SightUnits.Size; ++k) {
                if (!IsTargetable(SightUnits.Array[k].Unit, false))
                    continue;
                SUnit* b = WorldUnit(SightUnits.Array[k].Unit);
                if (b == this || b->Wrecked || b->Unplaced || b->CurrentTarget || b->_110)
                    continue;
                if (SameSide(Player, b->Player))
                    continue;
                if (b->Proto->ClassType != 9)
                    continue;
                if (*(const int*)(*(const unsigned char* const*)((const unsigned char*)b + 0x340) + 0x13c) == 1)   // SPBuildingUnit +0x13c
                    continue;
                int pt = PlayerType(b->Player);
                if (pt == 3 || pt == 4)
                    continue;
                if (!b->CanStoreUnit(WorldIndex))             // 0x5b7040
                    continue;
                if (!IsTargetable(b->WorldIndex, false))
                    continue;
                if (!g_GameLogic->CanSeeGroundUnit(Player, b))    // 0x562760
                    continue;
                float d = (b->Pos[0] - Pos[0]) * (b->Pos[0] - Pos[0]) + (b->Pos[2] - Pos[2]) * (b->Pos[2] - Pos[2]);
                if (d < bestD) {
                    bestD = d;
                    best = b;
                }
            }
            if (best) {
                EC_AssaultBuilding(best->WorldIndex, 1);      // +0xd4
                return;
            }
        }
        if (PrimaryTarget && !CurrentTarget && ActiveDriver > -1) {
            if (TKind(PrimaryTarget) == 4)
                RestoreBehavior();                            // +0x1ac
            SetCurrentTarget(PrimaryTarget, 0);
        }
    }
    if (PrimaryTarget && !CurrentTarget && ActiveDriver > -1)
        SetCurrentTarget(PrimaryTarget, 0);
}

// ---------------------------------------------------------------------------
// Per-tick helpers of ServerRefresh

// PANZERS 0x5c2200
// The static effects come back on (pixie +0x60 SetEffectEnabled(h, 1)).
// The recompile does not create them yet (handles -1).
void SUnit::EnableStaticEffects()
{
    for (int i = 0; i < StaticEffects.Size; ++i) {
        int h = StaticEffects.Array[i];
        if (h >= 0 && g_Pixie)
            g_Pixie->SetEffectEnabled(h, true);
    }
}

// PANZERS 0x55c900
// SDriver: the speed of the move and water effects (pixie +0x64(h, s, s)).
static void DriverSetEffectsSpeed(SDriver* d, float s)
{
    if (!g_Pixie)
        return;
    for (int i = 0; i < d->MoveEffects.Size; ++i)
        if (d->MoveEffects.Array[i] >= 0)
            g_Pixie->SetEffectSpeed(d->MoveEffects.Array[i], s, s);
    for (int i = 0; i < d->WaterEffects.Size; ++i)
        if (d->WaterEffects.Array[i] >= 0)
            g_Pixie->SetEffectSpeed(d->WaterEffects.Array[i], s, s);
}

// PANZERS 0x5bd910
// Driver effects: switch between the land and water sets when the unit
// crosses the shore, stop them when the unit stands still, start them when
// it moves off, and scale them by speed (linear or turning).
void SUnit::RefreshDriverEffects()
{
    SIDriver* id = nullptr;
    GetMovingDriver(&id);                                     // 0x5b9bf0
    if (!id) {
        if (PrevDriver <= -1)
            return;
        if (PrevDriver >= Drivers.Size)
            ArrayPanic("class SDriver *", PrevDriver);
        id = Drivers.Array[PrevDriver];
        if (!id)
            return;
    }
    SDriver* d = AsDriver(id);
    if (_338 != 0) {
        float water = g_World->GetWaterHeight(Pos[0], Pos[2]);       // 0x5ec490
        water = water - 0.01f;                                       // DAT_007f1b48
        float ground = g_World->GetTerrainHeight(Pos[0], Pos[2]);    // 0x5e7730
        int state = (water > ground) + 1;
        if (_338 != state) {
            if (state == 2) {
                d->StopAllEffects();                          // 0x55c0e0
                d->StartWaterEffects();                       // 0x55be10
            } else {
                d->StopWaterEffects();                        // 0x55c160
                d->StartMoveEffects();                        // 0x55bc50
            }
        }
    }
    bool skipStart = false;
    {
        float dx = PrevPos[0] - Pos[0];
        float dy = PrevPos[1] - Pos[1];
        float dz = PrevPos[2] - Pos[2];
        float d2 = dx * dx + dy * dy;
        d2 = d2 + dz * dz;
        if (!((double)d2 > 1e-8)) {                           // DAT_007fd6e0
            double a = fabs((double)PrevDir - (double)Dir);
            if (a > 3.1415927410125732)
                a = 6.2831854820251465 - a;
            if (!(a > 0.001)) {                               // DAT_007f59b8
                d->StopAllEffects();
                skipStart = true;
            }
        }
    }
    if (!skipStart) {
        float dx = PrevPos2[0] - PrevPos[0];
        float dy = PrevPos2[1] - PrevPos[1];
        float dz = PrevPos2[2] - PrevPos[2];
        float d2 = dx * dx + dy * dy;
        d2 = d2 + dz * dz;
        if (1e-8 > (double)d2) {
            double a = fabs((double)PrevDir2 - (double)PrevDir);
            if (a > 3.1415927410125732)
                a = 6.2831854820251465 - a;
            if (0.001 > a) {
                d->StartMoveEffects();                        // 0x55bc50
                d->StartEffects();                            // 0x55bb80
            }
        }
    }
    SPDriver* pd = static_cast<SPDriver*>(d->GetPDriver());   // driver +0x04
    if (!(pd->MaxSpeed > 0.0f) && !(static_cast<SPDriver*>(d->GetPDriver())->SpinSpeed > 0.0f))
        return;
    float lin = 0.0f;
    if (static_cast<SPDriver*>(d->GetPDriver())->MaxSpeed > 0.0f)
        lin = Speed / static_cast<SPDriver*>(d->GetPDriver())->MaxSpeed;
    if (!(static_cast<SPDriver*>(d->GetPDriver())->SpinSpeed > 0.0f)) {
        DriverSetEffectsSpeed(d, lin);
        return;
    }
    double spin = fabs((double)BitsF(_d0)) * 0.6000000238418579;  // DAT_007fd6f8
    double turn = spin / (double)static_cast<SPDriver*>(d->GetPDriver())->SpinSpeed;
    if ((double)lin > turn) {
        DriverSetEffectsSpeed(d, lin);
        return;
    }
    double turn2 = fabs((double)BitsF(_d0)) * 0.6000000238418579 /
                   (double)static_cast<SPDriver*>(d->GetPDriver())->SpinSpeed;
    DriverSetEffectsSpeed(d, (float)turn2);
}

// PANZERS 0x5bdcd0
// Invalid targets age: dead units drop out, a fully cleared entry is removed
// 40 ticks after its mark, following is allowed again after 60 ticks and
// attacking after Count * 60.
void SUnit::RefreshInvalidTargets()
{
    for (int i = 0; i < InvalidTargets.Size; ++i) {
        SInvalidTarget* e = &InvalidTargets.Array[i];
        if (!g_World->Units.IsLive(e->Unit)) {
            RemoveElem(&InvalidTargets, i, "struct SUnit::SInvalidTarget");
            return;
        }
        if (e->AttackOk && e->FollowOk && e->Frame + 0x28 == g_GameLogic->GetFrame()) {
            RemoveElem(&InvalidTargets, i, "struct SUnit::SInvalidTarget");   // 0x5bdf10
            return;
        }
        if (e->Frame + 0x3c == g_GameLogic->GetFrame())
            e->FollowOk = true;
        if (e->Frame + e->Count * 60 == g_GameLogic->GetFrame()) {
            e->Frame = g_GameLogic->GetFrame();
            e->AttackOk = true;
        }
    }
}

// PANZERS 0x5b76e0
// The current target's unit becomes invalid for attacking and following
// (Count up to 5); the unit waits (StuckFrame / StuckCount, 0x5bb400).
void SUnit::CollectInvalidTargets()
{
    if (!CurrentTarget)
        Logger.g->Panic("SUnit::CollectInvalidTargets() - CurrentTarget is null.");
    int unit = tgt::I(CurrentTarget, tgt::kUnit);
    int i = 0;
    for (; i < InvalidTargets.Size; ++i)
        if (InvalidTargets.Array[i].Unit == unit)
            break;
    if (i == InvalidTargets.Size)
        i = AddElem(&InvalidTargets);
    SInvalidTarget* e = &InvalidTargets.Array[i];
    e->Unit = unit;
    e->Frame = g_GameLogic->GetFrame();
    if (e->Count < 5)
        e->Count++;
    e->AttackOk = false;
    e->FollowOk = false;
    StuckFrame = g_GameLogic->GetFrame();
    if (StuckCount < 8)
        StuckCount++;
}

// PANZERS 0x5bcd20
void SUnit::OnDriverStucked()
{
    if (!CurrentTarget)
        Logger.g->Panic("SUnit::OnDriverStucked - No Currenttarget, unit:%s, idx:%d", SStr(Proto->Name), WorldIndex);
    g_World->UnitSpeech(WorldIndex, 4, false);                // 0x5fff20 "CantMove"
    STarget* ct = CurrentTarget;
    if ((ct == PrimaryTarget || TType(ct) == 2 || TType(ct) == 3) && PrimaryTarget)
        DropTarget(&PrimaryTarget);
    CollectInvalidTargets();                                  // 0x5b76e0
    DropTarget(&CurrentTarget);
    if (Proto->ClassType != 6)
        AI_Heartbeat();                                       // +0x190
}

// PANZERS 0x5bcb60
// SUnit::OnDriverReachedTarget: a path target moves on to its next point;
// otherwise the orders are done and the unit thinks again.
void SUnit::OnDriverReachedTarget()
{
    STarget* ct = CurrentTarget;
    if (!ct)
        Logger.g->Panic("SUnit::OnDriverReachedTarget - No Currenttarget, unit:%s, idx:%d", SStr(Proto->Name), WorldIndex);
    if (ActiveDriver < 0 || tgt::I(ct, tgt::kPath) < 0) {
        if (ct == PrimaryTarget && PrimaryTarget)
            DropTarget(&PrimaryTarget);
        if (PrimaryTarget && TKind(PrimaryTarget) == 5 && CurrentTarget && TKind(CurrentTarget) == 0)
            DropTarget(&PrimaryTarget);
        if (PrimaryTarget && TKind(PrimaryTarget) == 0xd && CurrentTarget && TKind(CurrentTarget) == 0)
            STUB_LOG("SUnit::OnDriverReachedTarget (0x5bcb60) +0x150 (enter the vehicle)");
        DropTarget(&CurrentTarget);
        if (Proto->ClassType != 6)
            AI_Heartbeat();                                   // +0x190
        return;
    }
    if (!PzTargetConsumePath(ct)) {                           // STarget::ConsumePath 0x5b7c50
        if (CurrentTarget == PrimaryTarget && PrimaryTarget)
            DropTarget(&PrimaryTarget);
        DropTarget(&CurrentTarget);
        if (PrimaryTarget)
            Logger.g->Warning("SUnit::OnDriverReachedTarget: CurrentTarget was path, PrimaryTarget is different");
    } else if (TKind(CurrentTarget) != 0xe) {
        GetDriver(ActiveDriver)->SetTarget(CurrentTarget);    // driver +0x08
    }
}

// PANZERS 0x5c2d30
// Stops the unit dead (trigger teleport): no targets, gunners and driver
// stopped, speed and spin zero.
void SUnit::StopUnit()
{
    DropTarget(&CurrentTarget);
    DropTarget(&PrimaryTarget);
    StopGunners();                                            // +0xec
    if (ActiveDriver >= 0) {
        if (ActiveDriver >= Drivers.Size)
            ArrayPanic("class SDriver *", ActiveDriver);
        PzDriverReset(Drivers.Array[ActiveDriver], true);     // 0x55bf50(1)
    }
    _f8 = 1;
    Speed = 0.0f;
    _d0 = 0;
}

// PANZERS 0x550730
// SDriver: drop the global path (the ghost restarts from the unit).
static void DriverDropGlobal(SDriver* d)
{
    if (!d->Target)
        return;
    ((SUnit*)d->Unit)->_cd[0] = 1;                            // unit +0xcd
    d->ResetGhost();                                          // 0x554770
    d->LocalPathFailures = 0;
    d->CollisionCount = 0;
    d->AvoidUnit = -1;
    d->LastCollisionUnit = -1;
    d->GlobalWayPoints.Clear(0);                              // 0x550a30
}

// PANZERS 0x550780
// SDriver: drop the local path.
static void DriverDropLocal(SDriver* d)
{
    if (!d->Target)
        return;
    ((SUnit*)d->Unit)->_cd[0] = 1;
    d->ResetGhost();
    d->LocalPathFailures = 0;
    d->CollisionCount = 0;
    d->AvoidUnit = -1;
    d->LastCollisionUnit = -1;
    for (int i = 0; i < d->LocalWayPoints.Size; ++i)          // 0x550aa0
        Wpm_Free(&d->LocalWayPoints.Array[i]);
    d->LocalWayPoints.Clear(0);
}

// PANZERS 0x5b5b50
void SUnit::DriverDropGlobalPath()
{
    if (!CurrentTarget || ActiveDriver < 0)
        return;
    if (ActiveDriver >= Drivers.Size)
        ArrayPanic("class SDriver *", ActiveDriver);
    DriverDropGlobal(AsDriver(Drivers.Array[ActiveDriver]));
}

// PANZERS 0x5b5ba0
void SUnit::DriverDropLocalPath()
{
    if (!CurrentTarget || ActiveDriver < 0)
        return;
    if (ActiveDriver >= Drivers.Size)
        ArrayPanic("class SDriver *", ActiveDriver);
    if (!AsDriver(Drivers.Array[ActiveDriver])->Target)
        return;
    DriverDropLocal(AsDriver(GetDriver(ActiveDriver)));
}

// PANZERS 0x5bee10
// A kind-1 target keeps the driver's global path in +0x308 (0x54ff30).
void SUnit::CopyDriverWayPoints()
{
    if (ActiveDriver < 0)
        return;
    if (ActiveDriver >= Drivers.Size)
        ArrayPanic("class SDriver *", ActiveDriver);
    SDriver* d = AsDriver(Drivers.Array[ActiveDriver]);
    if (!d->Target || d->GlobalWayPoints.Size == 0)
        return;
    d = AsDriver(GetDriver(ActiveDriver));
    ClearElems(&WayPoints, d->GlobalWayPoints.Size, "struct SVector2");   // 0x550a30
    for (int i = 0; i < WayPoints.Size; ++i) {
        WayPoints.Array[i].X = d->GlobalWayPoints.Array[i].X;
        WayPoints.Array[i].Z = d->GlobalWayPoints.Array[i].Z;
    }
}

// ---------------------------------------------------------------------------
// Order queue

// PANZERS 0x5bb980
void SUnit::OrderAt(int command, const float* xz, bool queue, bool add)
{
    SOrder o;
    memset(&o, 0, sizeof(o));
    o.Command = command;
    o.X = xz[0];
    o.Z = xz[1];
    o.Queue = queue;
    if (!add) {
        ClearElems(&Orders, 0, "struct SUnit::SOrder");       // 0x5b75c0(0)
        ExecuteCommand(o);                                    // 0x5b95a0
        return;
    }
    int i = AddElem(&Orders);                                 // 0x5b59a0
    if (i < 0 || i >= Orders.Size)
        ArrayPanic("struct SUnit::SOrder", i);
    Orders.Array[i] = o;
}

// PANZERS 0x5bbb60
void SUnit::OrderParam(int command, int param, bool queue, bool add)
{
    SOrder o;
    memset(&o, 0, sizeof(o));
    o.Command = command;
    o.Param = param;
    o.Queue = queue;
    if (!add) {
        ClearElems(&Orders, 0, "struct SUnit::SOrder");
        ExecuteCommand(o);
        return;
    }
    int i = AddElem(&Orders);
    if (i < 0 || i >= Orders.Size)
        ArrayPanic("struct SUnit::SOrder", i);
    Orders.Array[i] = o;
}

// PANZERS 0x5bb8a0
void SUnit::OrderUnit(int command, int unit, bool queue, bool add)
{
    SOrder o;
    memset(&o, 0, sizeof(o));
    o.Command = command;
    o.Unit = unit;
    o.Queue = queue;
    if (!add) {
        ClearElems(&Orders, 0, "struct SUnit::SOrder");
        ExecuteCommand(o);
        return;
    }
    int i = AddElem(&Orders);
    if (i < 0 || i >= Orders.Size)
        ArrayPanic("struct SUnit::SOrder", i);
    Orders.Array[i] = o;
}

// PANZERS 0x5b95a0
// The EC_ slot of a command. Cases whose slot is not typed in iunit.h log.
void SUnit::ExecuteCommand(const SOrder& o)
{
    int q = o.Queue ? 1 : 0;
    switch (o.Command) {
    case 1:
    case 0x1f:
        EC_Move(FBits(o.X), FBits(o.Z), q, false, 0);         // +0xac
        return;
    case 2:
        EC_Move(FBits(o.X), FBits(o.Z), q, true, o.Param2);
        return;
    case 6:
        EC_MoveAlongPath(o.Param, -1, q);                     // +0xb4
        return;
    case 7:
        EC_Follow(o.Unit, q);                                 // +0xb8
        return;
    case 8:
        ClearTargets();                                       // +0xc4
        return;
    case 0x28:
        SetBehavior(o.Param);                                 // +0x12c
        return;
    case 0:
        EC_Default(o.Unit, q);                                // +0xa4
        return;
    case 9:
        EC_AttackMove(o.X, o.Z, q);                           // +0xcc
        return;
    case 10:
        EC_AttackAlongPath(o.Param, -1, q);                   // +0xd0
        return;
    case 0xc:
        Slot_E0(o.Unit, q);                                   // +0xe0
        return;
    case 0xd:
        Slot_158(o.Unit);                                     // +0x158
        return;
    case 0xe:
        Slot_160(o.Unit);                                     // +0x160
        return;
    case 0xf:
        Slot_15C(o.Unit);                                     // +0x15c
        return;
    case 0x10:
        EC_AttackPos(FBits(o.X), FBits(o.Z), q);              // +0xe4
        return;
    case 0x11:
    case 0x20:
        EC_Attack(o.Unit, q);                                 // +0xe8
        return;
    case 0x12:
        EC_ThrowGrenade(o.Unit, q);                           // +0xf0
        return;
    case 0x14:
        EC_ThrowMolotov(o.Unit, q);                           // +0xf4
        return;
    case 0x16:
        EC_ThrowMagneticMine(o.Unit, q);                      // +0xf8
        return;
    case 0x17:
        Slot_FC(FBits(o.X), FBits(o.Z), q);                   // +0xfc
        return;
    case 0x18:
        Slot_100(FBits(o.X), FBits(o.Z), q);                  // +0x100
        return;
    case 0x19:
        Slot_104();                                           // +0x104
        return;
    case 0x1a:
        Slot_108();                                           // +0x108
        return;
    case 0x1b:
        Slot_10C(FBits(o.X), FBits(o.Z), q);                  // +0x10c
        return;
    case 0x1e:
        Slot_110(o.Param, q);                                 // +0x110
        return;
    case 0x21:
        SetFireBehavior(o.Param);                             // +0x13c
        return;
    case 0x22:
        Slot_114(FBits(o.X), FBits(o.Z));                     // +0x114
        return;
    case 0x23:
        Slot_118(o.Unit);                                     // +0x118
        return;
    case 0x24:
        Slot_11C();                                           // +0x11c
        return;
    case 0x25:
        Slot_120(o.Param2);                                   // +0x120
        return;
    case 0x26:
        EC_Die();                                             // +0x124
        return;
    case 0x29:
        Slot_134(o.Param);                                    // +0x134
        return;
    case 0x2a:
        EC_Enter(o.Unit, q);                                  // +0x140
        return;
    case 0x2b:
        Unload(o.Param);                                      // +0x144
        return;
    case 0x2d:
        Slot_154();                                           // +0x154
        return;
    case 0x2e:
        Slot_164(o.Param);                                    // +0x164
        return;
    case 3:
        EC_MoveReverse(FBits(o.X), FBits(o.Z), q, false, 0);  // +0xb0 (p5 = 0.0f)
        return;
    case 4:
        EC_MoveReverse(FBits(o.X), FBits(o.Z), q, true, o.Param2);   // +0xb0
        return;
    case 5:
        EC_TurnTo(o.Param2);                                  // +0xbc (Param2 float bits)
        return;
    case 0xb:
        EC_AttackMoveUnit(o.Unit);                            // +0xdc
        return;
    case 0x1c:
        SetItemAutoUse(0, o.Param != 0);                      // 0x5b88f0(0, ..)
        return;
    case 0x1d:
        SetItemAutoUse(1, o.Param != 0);                      // 0x5b88f0(1, ..)
        return;
    case 0x27:
        EC_Destroy();                                         // +0x128
        return;
    case 0x2c:
        EC_Tow(o.Unit);                                       // +0xd8
        return;
    case 0x2f:
        Slot_168(o.Param != 0);                               // +0x168
        return;
    default:
        return;
    }
}

// ---------------------------------------------------------------------------
// Medic

// PANZERS 0x5bfa50
// Healing (current target of kind 8 on a unit): within `range` (2D) and
// while the target has a wounded member (+0x80), the weakest member (the
// last with the lowest HP, its index kept as a float) gets Heal(0.1) once
// per member of this unit; the order ends when it is whole and the lowest
// was 1.0. Every 9th frame a "Projectile medic" flies to it (ballistic at
// pi/4, no damage, homing on it). Out of range: wait while the driver is
// active, else (or when nobody is wounded) go back to the primary order.
void SUnit::ServerRefreshMedic(float range)
{
    STarget* t = CurrentTarget;
    if (!t || TKind(t) != 8)
        return;
    int tu = tgt::I(t, tgt::kUnit);
    SUnit* tgtUnit = WorldUnit(tu);
    float dz = Pos[2] - tgtUnit->Pos[2];
    float dx = Pos[0] - tgtUnit->Pos[0];
    if (dz * dz + dx * dx > range * range) {
        if (ActiveDriver != -1)
            return;
    } else if (tgtUnit->HasWoundedMember()) {                 // +0x80
        float lowest = 1.0f;                                  // 0x7f1b58
        float best = -1.0f;                                   // 0x7f5a98 (member unit index as a float)
        for (int i = 0;; ++i) {
            SUnit* sq = WorldUnit(tgt::I(CurrentTarget, tgt::kUnit));
            if (i >= sq->Members.Size)
                break;
            SUnit* m = WorldUnit(sq->Members.Array[i].Unit);
            if (lowest >= m->HP) {
                best = (float)sq->Members.Array[i].Unit;      // 0x5991e0, cvtdq2ps
                lowest = WorldUnit((int)best)->HP;
            }
        }
        if (best < 0.0f)
            return;
        int b = (int)best;                                    // cvttss2si
        WorldUnit(b)->Heal(0.1f);                       // +0x98 (0x3dcccccd)
        for (int k = 1; k < Members.Size; ++k)
            WorldUnit(b)->Heal(0.1f);
        if (WorldUnit(b)->HP >= 1.0f && lowest == 1.0f)
            ClearTargets();                                   // +0xc4
        if (tgt::I(CurrentTarget, tgt::kUnit) == WorldIndex || g_GameLogic->GetFrame() % 9 != 0)
            return;
        float from[3] = { Pos[0], Pos[1] + 1.0f, Pos[2] };
        SUnit* bu = WorldUnit(b);
        float to[3] = { bu->Pos[0], bu->Pos[1], bu->Pos[2] };
        float dir = DAtan2f((double)(to[0] - from[0]), (double)(to[2] - from[2]));   // 0x78d07a, fstp float
        int proj = g_World->CreateUnit(Player, "Projectile medic", from, dir, 0, 1.0f, -1, true, "");   // 0x5e3170
        float vel[3];
        GunnerBallisticVelocity(to[0] - from[0], to[1] - from[1], to[2] - from[2], 0.7853981852531433f, vel);   // 0x7f7f50
        if (!_finite((double)vel[0]) || !_finite((double)vel[1]) || !_finite((double)vel[2]))   // 0x793d6c
            Logger.g->Panic("SUnit::ServerRefreshMedic: Projectile speed is not finite!");
        SProjectileUnit* p = (SProjectileUnit*)WorldUnit(proj);
        p->SetVelocity(vel[0], vel[1], vel[2]);               // 0x5a4450
        p->SetDamage(0.0f, 0.0f, 0);                          // 0x5a4410
        p->Shooter = WorldIndex;                              // +0x354
        p->SetHomingTarget(b);                                // 0x5a4440
        return;
    }
    STarget* pt = PrimaryTarget;
    if (!pt || pt == CurrentTarget) {
        ClearTargets();                                       // +0xc4
        return;
    }
    SetCurrentTarget(pt, 0);                                  // +0xa0
    AI_Heartbeat();                                           // +0x190
}

// ---------------------------------------------------------------------------
// SSingleUnit

// PANZERS 0x5aef40
// Once a second (ServerRefresh, (frame + index) % 20 == 0): rebuild the near
// lists in weapon (or sight) range, let repair and supply vehicles look for
// work, follow up building and vehicle-entry orders, then AI_Heartbeat.
void SSingleUnit::RefreshTargeting()
{
    if (IsWaitingAfterStuck())                                // 0x5bb400
        return;
    if (g_GameLogic && g_GameLogic->IsPaused() && !_1d4)
        return;
    if (PlayerType(Player) == 2)
        return;
    if (IsHiddenInBlockMap())                                 // 0x5bb5c0
        return;
    if (!P->Repairer && !P->Supporter) {
        float range = GetMaxRange(MainGunner);                // +0x17c
        if (range == 0.0f)                                    // DAT_007f1038
            range = GetSightRange();                          // +0x184
        FillNearUnits(range, P->_ec);                         // 0x5b78a0
    } else {
        FillNearUnits(10.0f, P->_ec);
        AutoRepairSupply(0.5f);                               // 0x5bd610
    }
    STarget* pt = PrimaryTarget;
    if (pt && TKind(pt) == 5) {
        int action = GetBuildingAction(WorldUnit(tgt::I(pt, tgt::kUnit))->WorldIndex);   // 0x5ba2b0
        STarget* t = nullptr;
        if (action == 1) {
            if (!CurrentTarget || TKind(CurrentTarget) != 0) {
                t = PzTargetNew(0);
                tgt::I(t, tgt::kType) = 2;                    // 0x5c18c0
                tgt::F(t, tgt::kPos) = tgt::F(PrimaryTarget, tgt::kPos);
                tgt::F(t, tgt::kPos + 4) = tgt::F(PrimaryTarget, tgt::kPos + 4);
                tgt::F(t, tgt::kPos + 8) = tgt::F(PrimaryTarget, tgt::kPos + 8);
            }
        } else if (action == 3) {
            if (!CurrentTarget || TKind(CurrentTarget) != 2) {
                t = PzTargetNew(2);
                tgt::I(t, tgt::kType) = 0;                    // 0x5c21b0
                tgt::I(t, tgt::kUnit) = tgt::I(PrimaryTarget, tgt::kUnit);
                tgt::I(t, tgt::kP20) = 1;
            }
        }
        if (t)
            SetCurrentTarget(t, 0);                           // +0xa0
    }
    if (PrimaryTarget && TKind(PrimaryTarget) == 0xd) {
        if (PzTargetRefresh(PrimaryTarget, WorldIndex))       // 0x5bd210
            STUB_LOG("SSingleUnit::RefreshTargeting (0x5aef40) vehicle entry (+0x58, +0x150)");
        else
            ClearTargets();                                   // +0xc4
    }
    AI_Heartbeat();                                           // +0x190
}

// PANZERS 0x5acbd0
float SSingleUnit::GetMaxRange(int weapon)
{
    if (weapon >= 0 && Gunners.Size > 0) {
        if (weapon >= Gunners.Size)
            ArrayPanic("class SGunner *", weapon);
        return Gunners.Array[weapon]->GetPGunner()->MaxRange;
    }
    if (Members.Size < 1) {
        if (P->Repairer || P->Supporter)
            return 10.0f;                                     // DAT_007f5a7c
        return 0.0f;
    }
    SUnit* crew = WorldUnit(Members.Array[0].Unit);
    if (crew->Gunners.Size <= 0)
        return 0.0f;
    return crew->Gunners.Array[0]->GetPGunner()->MaxRange;
}

// PANZERS 0x5acd30
float SSingleUnit::GetMinRange(int weapon)
{
    if (weapon >= 0 && Gunners.Size > 0) {
        if (weapon >= Gunners.Size)
            ArrayPanic("class SGunner *", weapon);
        return Gunners.Array[weapon]->GetPGunner()->MinRange;
    }
    if (Members.Size <= 0)
        return 0.0f;
    SUnit* crew = WorldUnit(Members.Array[0].Unit);
    if (crew->Gunners.Size <= 0)
        return 0.0f;
    return crew->Gunners.Array[0]->GetPGunner()->MinRange;
}

// PANZERS 0x5acb30
// A vehicle's rank is its best stored unit's.
int SSingleUnit::GetRank()
{
    int best = 0;
    for (int i = 0; i < Stored.Size; ++i) {
        int r = WorldUnit(Stored.Array[i].Unit)->GetRank();   // +0x88
        if (best < r)
            best = r;
    }
    return best;
}

// PANZERS 0x5ac4b0
// A vehicle ordered to a point close behind it backs up (+0xb0) instead of
// turning round, unless the final direction asked for is roughly its own.
void SSingleUnit::EC_Move(int xBits, int zBits, int p3, bool p4, int p5)
{
    float x = BitsF(xBits);
    float z = BitsF(zBits);
    float fdir = BitsF(p5);
    float dx = x - Pos[0];
    float dz = z - Pos[2];
    float to = (float)DAtan2((double)dx, (double)dz);         // 0x78d07a, fstp qword then cvtpd2ps
    if (0.0001 > (double)(float)fabs((double)dx) && 0.0001 > (double)(float)fabs((double)dz))   // DAT_007f1b50
        to = Dir;
    bool back = false;
    if (P->ClassType != 0xc) {
        double a = fabs((double)Dir - (double)to);
        if (a > 3.1415927410125732)
            a = 6.2831854820251465 - a;
        if (a > 1.9634954631328583 && 36.0f > dz * dz + dx * dx) {   // DAT_007fb6b0, DAT_007fb6bc
            back = true;
            if (p4) {
                double b = fabs((double)to - (double)fdir);
                if (b > 3.1415927410125732)
                    b = 6.2831854820251465 - b;
                if (!(b > 1.5707963705062866))               // DAT_007f5a38
                    back = false;
            }
        }
    }
    if (back) {
        STUB_LOG("SSingleUnit::EC_Move (0x5ac4b0) +0xb0 move backwards");
        PZ_M2_TRACE("SSingleUnit::EC_Move (0x5ac4b0) +0xb0");
        return;
    }
    SUnit::EC_Move(xBits, zBits, p3, p4, p5);                 // 0x5b8ea0
}

// PANZERS 0x5abde0
// Releases the board elements (+0x344..+0x3a4) and the armour decals
// (+0x37c..+0x38c, World+0xe4 +0x64), then SUnit::Uninit. The recompile
// creates neither (all -1).
void SSingleUnit::Uninit()
{
    for (int i = 0; i < 0x19; ++i)
        if (Board[i] != -1)
            STUB_LOG("SSingleUnit::Uninit (0x5abde0) board element / decal release");
    SUnit::Uninit();                                          // 0x5b7e40
}

// PANZERS 0x5aaaa0
// Per frame: the model's damage / heat glow (model +0xec / +0xf0), the board
// above the vehicle (selection, health bar, rank, weapons, ammo) and the
// armour decals under it, placed from the screen projection of the model.
// None of it feeds the logic; the board and the untyped model slots log.
void SSingleUnit::UpdateVisuals(SIViewport* vp)
{
    if (!Model)
        return;
    float pos[3] = { 0.0f, 0.0f, 0.0f };
    Model->GetRenderPosition(pos);                            // model +0x14
    float sx = 0.0f, sy = 0.0f, size = 0.0f, sz = 0.0f;
    int fog = 0;
    vp->ProjectToScreen(pos, 1.0f, &sx, &sy, &size, &sz, &fog);   // viewport +0x3c
    if (_118 > 0.0f)
        STUB_LOG("SSingleUnit::UpdateVisuals (0x5aaaa0) heat glow (model +0xec)");
    if (!(HP > 0.5f))                                         // DAT_007f453c
        STUB_LOG("SSingleUnit::UpdateVisuals (0x5aaaa0) damage glow (model +0xf0)");
    STUB_LOG("SSingleUnit::UpdateVisuals (0x5aaaa0) board and armour decals");
}

// ---------------------------------------------------------------------------
// Callable forms for the driver environment (g_DriverEnv, driverunit.cpp).

// PANZERS 0x5bcd20 (through the unit)
void SUnit::EnvOnDriverStucked(SIUnit* unit)
{
    static_cast<SUnit*>(unit)->OnDriverStucked();
}

// PANZERS 0x5b6fd0 (through the unit)
bool SUnit::EnvIsPosInRange(SIUnit* unit, float x, float y, float z)
{
    return static_cast<SUnit*>(unit)->IsPosInGunnerArc(x, y, z);
}

// PANZERS 0x5b9ce0 (through the unit): the main gunner's SPGunner (+0x28 =
// its fire start arc), nullptr without one.
void* SUnit::EnvGetAimer(SIUnit* unit)
{
    SPGunner* pg = nullptr;
    static_cast<SUnit*>(unit)->GetMainPGunner(&pg);
    return pg;
}

// PANZERS 0x55ce70 (SUnit +0x78) / 0x548200 (SBuildingUnit +0x78): the
// position a unit drives to when it enters this one.
float* SUnit::EnvGetEntrance(SIUnit* unit, float* out)
{
    SUnit* u = static_cast<SUnit*>(unit);
    if (u->Proto->ClassType == 9) {
        const unsigned char* b = (const unsigned char*)u;
        out[0] = *(const float*)(b + 0x364);                  // SBuildingUnit +0x364 / +0x368
        out[1] = u->Pos[1];
        out[2] = *(const float*)(b + 0x368);
        return out;
    }
    out[0] = u->Pos[0];
    out[1] = u->Pos[1];
    out[2] = u->Pos[2];
    return out;
}

// Unit +0x48 SUnit::OnDriverReachedTarget (0x5bcb60; SSingleUnit 0x5aed30
// and the squads override it).
void SUnit::EnvOnDriverReachedTarget(SIUnit* unit)
{
    unit->OnDriverReachedTarget();
}

// PANZERS 0x5c1d40 (SUnit +0x9c): +0x112 = p1. SPanzersSquadUnit overrides
// it (0x5a0a10, not lifted).
void SUnit::EnvSlot9C(SIUnit* unit, int p1)
{
    SUnit* u = static_cast<SUnit*>(unit);
    if (u->Proto->ClassType == 5) {
        static_cast<SPanzersSquadUnit*>(u)->SetFlag112(p1 != 0);  // squad +0x9c 0x5a0a10
        return;
    }
    u->_112 = (unsigned char)p1 != 0;
}

} // namespace pz
