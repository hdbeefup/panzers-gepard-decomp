// src/game/movementgroup.cpp
// SGameLogic orders and movement groups (convoys, formations).
// OWNER: agent L. Lifted from the HD exe.
//
// A movement group (SGameLogic+0x2dc, SHeap element 0x28) holds the units
// one order moves together: their offsets from the group centre, the
// slowest move speed (drivers read it through GetMovementGroupMoveSpeed),
// the boss and the biggest unit, and whether it is a convoy (the boss drives
// the path, every other unit follows the one in front).
//
// Unit orders: HD calls SUnit 0x5bb980 / 0x5bbb60 / 0x5bb8a0 (agent U, not
// in SIUnit yet). Their non-queued path clears the unit's order queue
// (+0x19c SDArray, element 0x1c) and runs SUnit::ExecuteCommand 0x5b95a0,
// which for the commands used here calls EC_Move (+0xac, command 1),
// EC_MoveAlongPath (+0xb4, command 6) or EC_Follow (+0xb8, command 7).
// UnitOrder below does the same through SIUnit.

#include <stdlib.h>
#include <string.h>
#include "gamelogic.h"
#include "trigger.h"
#include "triggersunits.h"
#include "worldapi.h"
#include "world.h"
#include "logger.h"
#include "pz/imodel.h"
#include <math.h>
#include "stub_log.h"

namespace pz {

using m2u::UV;

void GroupStats(SFoundUnits* g);   // triggers.cpp (0x582770)

static int Bits(float f)
{
    int i;
    memcpy(&i, &f, 4);
    return i;
}

// PANZERS 0x5bb980 / 0x5bbb60 / 0x5bb8a0 (non-queued path) + 0x5b95a0
static void UnitOrder(int unit, int command, float x, float z, int target, int param, bool flag)
{
    if (!UV::kReal) {
        // M1 stand-ins: remembered for the test mover (StandInMove).
        m2u::StandInState* s = m2u::StandIn(unit);
        if (command == 1) {
            s->Order = 1;
            s->TX = x;
            s->TZ = z;
        } else if (command == 6 && g_World->Paths.IsLive(param) && g_World->Paths.Array[param].Data.Points.Size > 0) {
            const SHdArray<SPathPoint>& pts = g_World->Paths.Array[param].Data.Points;
            s->Order = 1;
            s->TX = pts.Array[pts.Size - 1].X;
            s->TZ = pts.Array[pts.Size - 1].Z;
        } else if (command == 7) {
            s->Order = 7;
            s->Follow = target;
        }
        return;
    }
    // 0x5b75c0(0): the order queue (+0x19c {array, size, max}) is emptied.
    int* queue = (int*)((unsigned char*)(void*)UV::Iface(unit) + 0x19c);
    queue[1] = 0;
    if (queue[2] > 0 && queue[0])
        memset((void*)(size_t)queue[0], 0, queue[2] * 0x1c);
    SIUnit* u = UV::Iface(unit);
    switch (command) {
    case 1:
        u->EC_Move(Bits(x), Bits(z), flag, false, 0);             // +0xac(x, z, flag, 0, 0)
        break;
    case 6:
        u->EC_MoveAlongPath(param, -1, flag);                     // +0xb4(path, -1, flag)
        break;
    case 7:
        u->EC_Follow(target, flag);                               // +0xb8(unit, flag)
        break;
    default:
        Logger.g->Warning("STUB: UnitOrder command %d not mapped (0x5b95a0)", command);
        break;
    }
}

static SMovementGroupElem* Group(SGameLogic* gl, int g, const char* panic)
{
    if (g < 0 || g >= gl->MovementGroupSize || gl->MovementGroups[g].Next != kHeapLive)
        Logger.g->Panic(panic, g);
    return &gl->MovementGroups[g];
}

static bool GroupLive(SGameLogic* gl, int g)
{
    return g >= 0 && g < gl->MovementGroupSize && gl->MovementGroups[g].Next == kHeapLive;
}

static int MemberUnit(const SMovementGroupElem* mg, int k)
{
    if (k < 0 || k >= mg->MemberCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SMovementGroupMember", k);
    int u = mg->Members[k].Unit;
    if (!m2u::IsLive(u))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", u);
    return u;
}

// PANZERS 0x5b9d70 (SUnit, inline): rank of a unit for the boss choice.
static int UnitRank(int u)
{
    int ct = UV::ClassType(u);
    if (ct != 0 && ct != 0xb)
        return 0;
    return UV::PUnitInt(u, 0x148);
}

// PANZERS 0x560df0 (SHeap<SMovementGroup>::Add)
static int AllocGroup(SGameLogic* gl)
{
    gl->MovementGroupCount++;
    int i = gl->MovementGroupFree;
    if (i >= 0) {
        gl->MovementGroupFree = gl->MovementGroups[i].Next;
        SMovementGroupElem& e = gl->MovementGroups[i];
        e.Next = kHeapLive;
        memset((unsigned char*)&e + 4, 0, sizeof(e) - 4);
        return i;
    }
    if (gl->MovementGroupSize == gl->MovementGroupMax) {
        int nmax = gl->MovementGroupMax < 0x10 ? 0x10 : (gl->MovementGroupMax * 6) / 5;
        gl->MovementGroups = (SMovementGroupElem*)realloc(gl->MovementGroups, nmax * sizeof(SMovementGroupElem));
        memset(gl->MovementGroups + gl->MovementGroupMax, 0, (nmax - gl->MovementGroupMax) * sizeof(SMovementGroupElem));
        gl->MovementGroupMax = nmax;
    }
    gl->MovementGroups[gl->MovementGroupSize].Next = kHeapLive;
    return gl->MovementGroupSize++;
}

// PANZERS 0x579300 (SHeap<SMovementGroup>::Remove)
static void FreeGroup(SGameLogic* gl, int g)
{
    if (!GroupLive(gl, g))
        Logger.g->Panic("SHeap<%s>::Remove: invalid index (%d)", "SMovementGroup", g);
    SMovementGroupElem& e = gl->MovementGroups[g];
    e.Next = gl->MovementGroupFree;
    free(e.Members);
    e.Members = nullptr;
    e.MemberCount = 0;
    e.MemberMax = 0;
    gl->MovementGroupCount--;
    gl->MovementGroupFree = g;
}

// PANZERS 0x560b50 (SDArray<SMovementGroupMember>::Add)
static int AddMember(SMovementGroupElem* mg)
{
    if (mg->MemberCount == mg->MemberMax) {
        int nmax = mg->MemberMax < 0x10 ? 0x10 : (mg->MemberMax * 6) / 5;
        mg->Members = (SMovementGroupMember*)realloc(mg->Members, nmax * sizeof(SMovementGroupMember));
        memset(mg->Members + mg->MemberMax, 0, (nmax - mg->MemberMax) * sizeof(SMovementGroupMember));
        mg->MemberMax = nmax;
    }
    return mg->MemberCount++;
}

// PANZERS 0x5649c0 (qsort comparator: farthest from the target first)
static int __cdecl CompareMemberDist(const void* a, const void* b)
{
    float da = ((const SMovementGroupMember*)a)->DistSq, db = ((const SMovementGroupMember*)b)->DistSq;
    return db <= da && da != db;
}

// PANZERS 0x56ff30
// Builds a movement group from a found-unit group: the units leave their old
// groups; single units and squads with an active driver that are not stored
// join (without convoy only those within 12.5 m of the group centre, and not
// class 0xb). Fewer than two members: no group (-1).
int SGameLogic::GroupOrder(bool convoy, int p3, SFoundUnits* g, bool p5, float x, float z)
{
    PZ_M2_TRACE("SGameLogic::GroupOrder (0x56ff30)");
    for (int j = 0; j < g->Count; ++j) {
        int u = g->Units[j].Unit;
        if (!m2u::IsLive(u))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", u);
        RemoveUnitFromMovementGroup(u);                           // 0x579510
    }
    if ((convoy && g->Count == 0) || g->Count < 2)
        return -1;
    int mgi = AllocGroup(this);                                   // 0x560df0
    for (int j = 0; j < g->Count; ++j) {
        int u = g->Units[j].Unit;
        if (!m2u::IsLive(u))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", u);
        if (UV::ActiveDriver(u) < 0)
            continue;
        if (UV::Container(u) >= 0)                                // (unsigned)+0x78 > 0x7fffffff
            continue;
        if (!convoy) {
            float dz = UV::Z(u) - g->Z;
            float dx = UV::X(u) - g->X;
            if (!(dz * dz + dx * dx <= 156.25f))                  // _DAT_007f5a8c
                continue;
            if (UV::ClassType(u) == 0xb)
                continue;
        }
        int ct = UV::ClassType(u);
        if (ct != 0 && ct != 5)
            continue;
        SMovementGroupElem* mg = Group(this, mgi, "SHeap<SMovementGroup>::operator[]: invalid index (%d)");
        int k = AddMember(mg);
        mg->Members[k].Unit = u;
        mg->Members[k].DX = UV::X(u) - g->X;
        mg->Members[k].DZ = UV::Z(u) - g->Z;
        float d = 0.0f;
        if (p5) {
            float ex = x - UV::X(u), ez = z - UV::Z(u);
            d = ex * ex + ez * ez;
        }
        mg->Members[k].DistSq = d;
        UV::MovementGroup(u) = mgi;
    }
    SMovementGroupElem* mg = Group(this, mgi, "SHeap<SMovementGroup>::operator[]: invalid index (%d)");
    if (mg->MemberCount == 0)
        return -1;                                                // HD keeps the empty group allocated
    if (mg->MemberCount == 1) {
        RemoveUnitFromMovementGroup(MemberUnit(mg, 0));
        return -1;
    }
    mg->Convoy = convoy;
    RefreshMovementGroup(p3, mgi);                                // 0x5800d0
    SetMovementGroupBiggestUnit(mgi);                             // 0x57faf0
    if (convoy) {
        mg = Group(this, mgi, "SHeap<SMovementGroup>::operator[]: invalid index (%d)");
        qsort(mg->Members, mg->MemberCount, sizeof(SMovementGroupMember), CompareMemberDist);
    }
    SetMovementGroupBossUnit(mgi, p5, x, z);                      // 0x57fcb0
    SetMovementGroupFormationDir2(mgi, 0);                        // 0x57ffc0
    SetMovementGroupFormationDir(mgi, 0);                         // 0x57ff40
    return mgi;
}

// PANZERS 0x57efd0
// Moves a found-unit group: a movement group first (0x56ff30), then each
// unit is ordered to the target plus its offset from the group centre, or
// to the target itself when it joined the movement group.
void SGameLogic::MoveFoundUnitsToLocation(SFoundUnits* g, int command, const float* target, bool p4, bool queue,
                                          bool marker)
{
    PZ_M2_TRACE("SGameLogic::MoveFoundUnitsToLocation (0x57efd0)");
    GroupOrder(false, p4, g, true, target[0], target[1]);
    for (int j = 0; j < g->Count; ++j) {
        int u = g->Units[j].Unit;
        if (!m2u::IsLive(u))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", u);
        float dest[2];
        dest[0] = target[0] + (UV::X(u) - g->X);
        dest[1] = target[1] + (UV::Z(u) - g->Z);
        const float* p = UV::MovementGroup(u) < 0 ? dest : target;
        if (queue)
            Logger.g->Warning("STUB: queued unit order (0x5bb980 queue path) not implemented");
        UnitOrder(u, command, p[0], p[1], 0, 0, p4);              // 0x5bb980(command, p, p4, queue)
        if (marker) {
            // HD: pixie +0x24 PlayEffect(scene, TargetRingFx, (dest x, 0, dest z), (0, 1, 0), 0).
            Logger.g->Warning("STUB: MoveFoundUnitsToLocation target marker (0x57efd0)");
        }
    }
}

// PANZERS 0x57e600
// The convoy's boss (first member) drives the path, the others follow.
void SGameLogic::ConvoyAlongPath(int group, int path)
{
    PZ_M2_TRACE("SGameLogic::ConvoyAlongPath (0x57e600)");
    if (!GroupLive(this, group))
        return;
    SMovementGroupElem* mg = &MovementGroups[group];
    if (mg->MemberCount < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SMovementGroupMember", 0);
    int boss = mg->Members[0].Unit;
    if (!m2u::IsLive(boss))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", boss);
    UnitOrder(boss, 6, 0.0f, 0.0f, 0, path, false);              // 0x5bbb60(6, path, 0, 0)
    SendConvoyMovementGroupFollowers(group);                      // 0x57e6f0
}

// PANZERS 0x57e6f0
void SGameLogic::SendConvoyMovementGroupFollowers(int group)
{
    PZ_M2_TRACE("SGameLogic::SendConvoyMovementGroupFollowers (0x57e6f0)");
    SMovementGroupElem* mg = Group(this, group, "SGameLogic::SendConvoyMovementGroupFollowers: Invalid movement group %d");
    for (int k = 1; k < mg->MemberCount; ++k) {
        int u = MemberUnit(mg, k);
        int front = mg->Members[k - 1].Unit;
        UnitOrder(u, 7, 0.0f, 0.0f, front, 0, false);            // 0x5bb8a0(7, front, 0, 0): EC_Follow
        mg = Group(this, group, "SHeap<SMovementGroup>::operator[]: invalid index (%d)");
    }
}

// PANZERS 0x579510
// Takes a unit out of its movement group. A group left with one unit is
// dissolved. In a convoy the boss's target goes to the new boss and the
// followers are re-sent; otherwise the remaining members are told when the
// boss rank, the biggest unit or the slowest speed changed.
void SGameLogic::RemoveUnitFromMovementGroup(int unit)
{
    int g = UV::MovementGroup(unit);
    if (!GroupLive(this, g)) {
        UV::MovementGroup(unit) = -1;
        return;
    }
    SMovementGroupElem* mg = &MovementGroups[g];
    bool convoy = mg->Convoy;
    float speed = mg->MoveSpeed;
    bool wasBoss = unit == GetMovementGroupBossUnit(g);
    int biggest = GetMovementGroupBiggestUnit(g);
    int last = -1;
    int wi = UV::WorldIndex(unit);
    for (int j = 0; j < mg->MemberCount; ++j) {
        if (mg->Members[j].Unit != wi)
            continue;
        // PANZERS 0x5790f0 (SDArray::Remove)
        --mg->MemberCount;
        if (mg->MemberCount - j != 0)
            memmove(&mg->Members[j], &mg->Members[j + 1], (mg->MemberCount - j) * sizeof(SMovementGroupMember));
        memset(&mg->Members[mg->MemberCount], 0, sizeof(SMovementGroupMember));
        if (mg->MemberCount > 1) {
            UpdateMovementGroupSlowestMoveSpeed(g);
            SetMovementGroupBiggestUnit(g);
            SetMovementGroupBossUnit(g, 0, 0.0f, 0.0f);
        } else {
            if (mg->MemberCount == 1) {
                last = MemberUnit(mg, 0);
                UV::MovementGroup(last) = -1;                     // +0x254 = -1
            }
            FreeGroup(this, g);                                   // 0x579300
        }
        break;
    }
    if (convoy) {
        // HD: if the unit holds a target (+0x1f8) the new boss (or the last
        // unit) takes it over (STarget refcount 0x5bdef0 / 0x5b5a30, then
        // +0xa0 SetCurrentTarget), and the followers are sent again.
        bool hasTarget = UV::kReal && UV::RawInt(unit, 0x1f8) != 0;
        if (hasTarget) {
            if (GroupLive(this, g)) {
                if (wasBoss)
                    Logger.g->Warning("STUB: convoy boss target hand-over (0x579510: 0x5bdef0 / 0x5b5a30, agent U)");
                SendConvoyMovementGroupFollowers(g);
            } else if (last >= 0 && wasBoss) {
                Logger.g->Warning("STUB: convoy target hand-over to the last unit (0x579510, agent U)");
            }
        }
    } else if (GroupLive(this, g)) {
        mg = &MovementGroups[g];
        int boss = GetMovementGroupBossUnit(g);
        int big = GetMovementGroupBiggestUnit(g);
        for (int k = 0; k < mg->MemberCount; ++k) {
            int m = MemberUnit(mg, k);
            bool notify = false;
            if (wasBoss && boss >= 0 && UnitRank(boss) != UnitRank(unit))
                notify = true;                                    // 0x5b5b50(m)
            else if (unit == biggest && big >= 0 && UV::RawInt(unit, 0x5c) != UV::RawInt(big, 0x5c))
                notify = true;                                    // 0x5b5b50(m)
            else if (speed != mg->MoveSpeed)
                notify = true;                                    // 0x5b5ba0(m)
            if (notify && UV::kReal)
                Logger.g->Warning("STUB: movement group member update (0x5b5b50 / 0x5b5ba0, agent U)");
            (void)m;
        }
    } else if (last >= 0 && UV::kReal) {
        Logger.g->Warning("STUB: last unit of a dissolved group (0x5b5b50, agent U)");
    }
    UV::MovementGroup(unit) = -1;                                 // +0x254 = -1
}

// PANZERS 0x56af10
int SGameLogic::GetMovementGroupConvoy(int group)
{
    return Group(this, group, "SGameLogic::GetMovementGroupConvoy: Invalid movement group %d")->Convoy;
}

// PANZERS 0x56b010 (drivers: SIDriver +0x48)
float SGameLogic::GetMovementGroupMoveSpeed(SIUnit* unit)
{
    int g = UV::kReal ? *(int*)((unsigned char*)(void*)unit + 0x254) : -1;
    if (!GroupLive(this, g))
        Logger.g->Panic("SGameLogic::GetMovementGroupMoveSpeed: Invalid movement group %d", g);
    return MovementGroups[g].MoveSpeed;
}

// PANZERS 0x56adc0 (returns the unit index; HD returns the unit pointer, 0 if gone)
int SGameLogic::GetMovementGroupBossUnit(int group)
{
    if (group < 0)
        return -1;
    SMovementGroupElem* mg = Group(this, group, "SHeap<SMovementGroup>::operator[]: invalid index (%d)");
    return m2u::IsLive(mg->BossUnit) ? mg->BossUnit : -1;
}

// PANZERS 0x56acf0
int SGameLogic::GetMovementGroupBiggestUnit(int group)
{
    if (group < 0)
        return -1;
    SMovementGroupElem* mg = Group(this, group, "SHeap<SMovementGroup>::operator[]: invalid index (%d)");
    return m2u::IsLive(mg->BiggestUnit) ? mg->BiggestUnit : -1;
}

// PANZERS 0x56b090 (HD takes the unit)
int SGameLogic::GetMovementGroupSquadsGlobalState(int unit)
{
    int g = UV::MovementGroup(unit);
    if (!GroupLive(this, g))
        Logger.g->Panic("SGameLogic::GetMovementGroupSquadsGlobalState: Invalid movement group %d", g);
    return MovementGroups[g].SquadsGlobalState;
}

// PANZERS 0x56b240 (HD takes the unit)
void SGameLogic::GetMovementGroupUnitFormationPos(int unit, float* out)
{
    int g = UV::MovementGroup(unit);
    if (!GroupLive(this, g))
        Logger.g->Panic("SGameLogic::GetMovementGroupUnitFormationPos: Invalid movement group %d", g);
    SMovementGroupElem* mg = &MovementGroups[g];
    int wi = UV::WorldIndex(unit);
    for (int k = 0; k < mg->MemberCount; ++k) {
        if (mg->Members[k].Unit == wi) {
            out[0] = mg->Members[k].DX;
            out[1] = mg->Members[k].DZ;
            return;
        }
    }
    out[0] = 0.0f;
    out[1] = 0.0f;
}

// PANZERS 0x57faf0
// The member with the largest unit size (+0x5c); the first wins ties.
void SGameLogic::SetMovementGroupBiggestUnit(int group)
{
    SMovementGroupElem* mg = Group(this, group, "SGameLogic::SetMovementGroupBiggestUnit: Invalid movement group %d");
    int best = -1, size = 0;
    for (int k = 0; k < mg->MemberCount; ++k) {
        int u = MemberUnit(mg, k);
        int s = UV::RawInt(u, 0x5c);
        if (k == 0 || size < s) {
            size = s;
            best = u;
        }
    }
    mg->BiggestUnit = best >= 0 ? UV::WorldIndex(best) : -1;
    if (Logger.g)
        Logger.g->Log(1, "SGameLogic::SetMovementGroupBiggestUnit - BiggestUnitIdx=%d, Name=%s", mg->BiggestUnit,
                      best >= 0 ? UV::TypeName(best) : "");
}

// PANZERS 0x57fcb0
// Convoys: the first member. Otherwise the member with the highest rank
// (0x5b9d70), the nearest to (x, z) on ties when p2 is set.
void SGameLogic::SetMovementGroupBossUnit(int group, int p2, float x, float z)
{
    SMovementGroupElem* mg = Group(this, group, "SGameLogic::SetMovementGroupBossUnit: Invalid movement group %d");
    if (mg->Convoy) {
        if (mg->MemberCount < 1)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SMovementGroupMember", 0);
        mg->BossUnit = mg->Members[0].Unit;
        return;
    }
    int best = -1, rank = 0;
    float bestD = 0.0f;
    for (int k = 0; k < mg->MemberCount; ++k) {
        int u = MemberUnit(mg, k);
        int r = UnitRank(u);
        float d = 0.0f;
        if (p2) {
            float dx = UV::X(u) - x, dz = UV::Z(u) - z;
            d = dx * dx + dz * dz;
        }
        if (k == 0 || rank < r || (r == rank && d < bestD)) {
            rank = r;
            best = u;
            bestD = d;
        }
    }
    mg->BossUnit = best >= 0 ? UV::WorldIndex(best) : -1;
    if (Logger.g)
        Logger.g->Log(1, "SGameLogic::SetMovementGroupBossUnit - BossUnitIdx=%d, Name=%s", mg->BossUnit,
                      best >= 0 ? UV::TypeName(best) : "");
}

// PANZERS 0x57ff40
void SGameLogic::SetMovementGroupFormationDir(int group, int p2)
{
    Group(this, group, "SGameLogic::SetMovementGroupFormationDir: Invalid movement group %d")->B24 = p2 != 0;
}

// PANZERS 0x57ffc0
void SGameLogic::SetMovementGroupFormationDir2(int group, int p2)
{
    Group(this, group, "SGameLogic::SetMovementGroupFormationDir: Invalid movement group %d")->FormationDir2 = p2;
}

// PANZERS 0x5800d0
// SquadsGlobalState (+0x14): with p3 0, the lowest global state (+0xec) of
// the members if they are all squads (999 start), else 0. Then the slowest
// speed.
void SGameLogic::RefreshMovementGroup(int p3, int group)
{
    SMovementGroupElem* mg = Group(this, group, "SGameLogic::RefreshMovementGroup: Invalid movement group %d");
    int state = 999;
    if (p3 == 0) {
        for (int k = 0; k < mg->MemberCount; ++k) {
            int u = MemberUnit(mg, k);
            if (UV::ClassType(u) != 5) {
                state = 0;
                break;
            }
            int s = UV::RawInt(u, 0xec);
            if (s < state)
                state = s;
        }
    } else {
        state = 0;
    }
    mg->SquadsGlobalState = state;
    UpdateMovementGroupSlowestMoveSpeed(group);                   // 0x5824b0
}

// PANZERS 0x5824b0
void SGameLogic::UpdateMovementGroupSlowestMoveSpeed(int group)
{
    SMovementGroupElem* mg = Group(this, group, "SHeap<SMovementGroup>::operator[]: invalid index (%d)");
    mg->MoveSpeed = 0.0f;
    for (int k = 0; k < mg->MemberCount; ++k) {
        int u = MemberUnit(mg, k);
        float s = UV::kReal ? UV::Iface(u)->GetMoveSpeed(-1) : 0.0f;   // +0x1b0
        mg = Group(this, group, "SHeap<SMovementGroup>::operator[]: invalid index (%d)");
        if (s != 0.0f && (mg->MoveSpeed == 0.0f || s < mg->MoveSpeed))
            mg->MoveSpeed = s;
    }
    if (Logger.g)
        Logger.g->Log(1, "SGameLogic::UpdateMovementGroupSlowestMoveSpeed - SlowestMoveSpeed=%f", (double)mg->MoveSpeed);
}

// ---------------------------------------------------------------------------
// Recompile-only test mover for the M1 stand-in units (no HD counterpart).
// Until agent U's units and agent P's drivers move the units, the stand-ins
// walk straight to their last order (vehicles 4 m/s, infantry 1.5 m/s;
// followers keep 8 m behind the unit in front) and SUnit::ServerRefresh's
// call of UpdateActiveLocations (0x5bee90 -> 0x582080) is made here, so the
// location events of the menu triggers fire and the trigger loop can be
// checked. Real units never come here.
void StandInMove(SGameLogic* gl)
{
    if (UV::kReal)
        return;
    PZ_FOR_EACH_UNIT(i) {
        int ct = UV::ClassType(i);
        if (ct == UC_SQUAD_MEMBER || UV::Unplaced(i))
            continue;
        m2u::StandInState* s = m2u::StandIn(i);
        SMenuUnit* u = (SMenuUnit*)(void*)g_World->Units.Array[i].Unit;
        float tx, tz, keep = 0.0f;
        if (s->Order == 1) {
            tx = s->TX;
            tz = s->TZ;
        } else if (s->Order == 7 && m2u::IsLive(s->Follow)) {
            tx = UV::X(s->Follow);
            tz = UV::Z(s->Follow);
            keep = 8.0f;
        } else {
            continue;
        }
        float dx = tx - u->Pos[0], dz = tz - u->Pos[2];
        float d = sqrtf(dx * dx + dz * dz);
        float step = (ct == UC_SQUAD || (u->Type && u->Type->AnimationType == 2)) ? 0.075f : 0.2f;
        if (d <= keep + 0.01f) {
            if (s->Order == 1)
                s->Order = 0;
            continue;
        }
        if (step > d - keep)
            step = d - keep;
        float mx = dx / d * step, mz = dz / d * step;
        u->Pos[0] += mx;
        u->Pos[2] += mz;
        u->Pos[1] = g_World->GetTerrainHeight(u->Pos[0], u->Pos[2]);
        u->Dir = atan2f(dx, dz);
        if (u->Model) {
            u->Model->SetPosition(u->Pos[0], u->Pos[1], u->Pos[2]);
            u->Model->SetRotation(u->Dir, 0.0f, 0.0f);
        }
        for (int m = 0; m < u->MemberCount; ++m) {
            int mi = u->Members[m];
            if (!m2u::IsLive(mi))
                continue;
            SMenuUnit* mm = (SMenuUnit*)(void*)g_World->Units.Array[mi].Unit;
            mm->Pos[0] += mx;
            mm->Pos[2] += mz;
            mm->Pos[1] = g_World->GetTerrainHeight(mm->Pos[0], mm->Pos[2]);
            if (mm->Model)
                mm->Model->SetPosition(mm->Pos[0], mm->Pos[1], mm->Pos[2]);
        }
    }
    PZ_FOR_EACH_UNIT(i) {
        int ct = UV::ClassType(i);
        if ((ct == 0 || ct == 0xb || ct == 0xc || ct == 10 || ct == 5) && !UV::Unplaced(i))
            gl->UpdateActiveLocationsAt(i, true);
    }
}

} // namespace pz
