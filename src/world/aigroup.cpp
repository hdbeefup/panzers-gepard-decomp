// src/world/aigroup.cpp
// SAIGroup (World+0x4f4, AIGP): loading, membership and the per-second AI
// refresh 0x5f5c70. OWNER: M3-C (taken over from sub-agent C4, whose layout
// analysis is aigroup.h).

#include <stdlib.h>
#include <string.h>
#include "aigroup.h"
#include "worldapi.h"
#include "astar.h"
#include "blockmap.h"
#include "stream.h"
#include "logger.h"
#include "../game/m3common.h"
#include "pzunitregistry.h"
#include "../game/unit.h"
#include "../game/gunner.h"
#include "../game/target.h"
#include "../game/gamelogic.h"
#include "../world/trigger.h"
#include "../game/drivermath.h"

namespace pz {

void FreeSString(SString* s);
void GroupStats(SFoundUnits* g);                                       // 0x582770 (triggers.cpp)

// The AIGP element descriptor (HD 0x8dde98) for gLoadVariables 0x670440.
// MaxHelpRange is a vec2 in HD: its second float lands on PathIdx (+0x34);
// whichever comes later in the file wins, as in HD.
static const SVarDesc kAIGroupDesc[] = {
    { "ID",               5,  0x00, nullptr, 0, 0 },
    { "Tactic",           2,  0x08, nullptr, 0, 0 },
    { "Status",           2,  0x0c, nullptr, 0, 0 },
    { "Timer",            2,  0x10, nullptr, 0, 0 },
    { "AttackMoveTimer",  2,  0x14, nullptr, 0, 0 },
    { "PathIdx",          2,  0x34, nullptr, 0, 0 },
    { "HasCalledForHelp", 1,  0x18, nullptr, 0, 0 },
    { "StartPos",         8,  0x1c, nullptr, 0, 0 },
    { "AttackTarget",     1,  0x24, nullptr, 0, 0 },
    { "TargetPos",        8,  0x28, nullptr, 0, 0 },
    { "MaxHelpRange",     8,  0x30, nullptr, 0, 0 },
    { "DisabledAIGroups", 10, 0x44, nullptr, 4, 2 },
    { nullptr,            0,  0,    nullptr, 0, 0 },
};

static int ReadI(::SStream* s) { return s->ReadInt(); }                 // 0x65d4d0

static SUnit* U(int i) { return WorldUnit(i); }                          // SHeapTRB::operator[] (panics)

static int UnitAt(const SHdArray<int>& a, int i)
{
    if (i < 0 || i >= a.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "int", i);
    return a.Array[i];
}

static int AddInt(SHdArray<int>& a)                                      // 0x560a00 SDArray<int>::AddEmpty
{
    if (a.Size == a.Max) {
        int n = a.Max < 0x10 ? 0x10 : (a.Max * 6) / 5;
        a.Array = (int*)realloc(a.Array, n * sizeof(int));
        memset(a.Array + a.Max, 0, (n - a.Max) * sizeof(int));
        a.Max = n;
    }
    return a.Size++;
}

static int BitsOf(float f)
{
    int i;
    memcpy(&i, &f, 4);
    return i;
}

static bool PlayersAllied(int p1, int p2)                                // World+0x17c teams (inline)
{
    const unsigned char* pl = g_World->Players[0];
    int team = *(const int*)(pl + p1 * 0x48 + 0x0c);
    if (team == 0)
        return p1 == p2;
    return team == *(const int*)(pl + p2 * 0x48 + 0x0c);
}

// PANZERS 0x55ccc0
SAIGroup* AIGroupAt(int index)
{
    SHeap<SAIGroup>& h = AIGroupHeap(g_World);
    if (!h.IsLive(index))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SAIGroup", index);
    return &h.Array[index].Data;
}

// PANZERS 0x5b3380
void SAIGroup::Release()
{
    free(DisabledAIGroups.Array);
    DisabledAIGroups.Array = nullptr;
    DisabledAIGroups.Size = DisabledAIGroups.Max = 0;
    free(Units.Array);
    Units.Array = nullptr;
    Units.Size = Units.Max = 0;
    FreeSString(&ID);
}

// PANZERS 0x5636a0
void AIGroupsClear(SWorld* w)
{
    SHeap<SAIGroup>& h = AIGroupHeap(w);
    for (int i = 0; i < h.Size; ++i)
        if (h.Array[i].Next == kHeapLive)
            h.Array[i].Data.Release();
    h.Size = 0;
    h.Free = -1;
    h.Count = 0;
}

// PANZERS 0x56e2c0 (+ the per-group 0x601060 of the map loader 0x5f2a50)
void AIGroupsLoad(SWorld* w, ::SStream* s)
{
    SHeap<SAIGroup>& h = AIGroupHeap(w);
    AIGroupsClear(w);
    unsigned n = (unsigned)ReadI(s);
    if (n > 0x1000000)
        throw "Invalid array size";
    h.Size = (int)n;
    if (h.Max < (int)n) {
        h.Max = (int)n;
        h.Array = (SHeapElem<SAIGroup>*)realloc(h.Array, n * sizeof(SHeapElem<SAIGroup>));   // 0x78b864
    }
    memset(h.Array, 0, h.Max * sizeof(SHeapElem<SAIGroup>));
    h.Free = ReadI(s);
    h.Count = ReadI(s);
    for (int i = 0; i < h.Size; ++i) {
        h.Array[i].Next = ReadI(s);
        if (h.Array[i].Next == kHeapLive)
            LoadVariables(s, &h.Array[i].Data, kAIGroupDesc);       // 0x5f0e10
    }
    for (int i = 0; i < h.Size; ++i)
        if (h.Array[i].Next == kHeapLive)
            AIGroupAt(i)->ComputeStartPos();
}

// PANZERS 0x5be040
void AIGroupsRemove(SWorld* w, int index)
{
    SHeap<SAIGroup>& h = AIGroupHeap(w);
    if (!h.IsLive(index))
        Logger.g->Panic("SHeap<%s>::Remove: invalid index (%d)", "SAIGroup", index);
    h.Array[index].Next = h.Free;
    h.Array[index].Data.Release();
    --h.Count;
    h.Free = index;
}

// PANZERS 0x601060
// The mean position of the group's top-level units (MaxHelpRange 600 when 0).
void SAIGroup::ComputeStartPos()
{
    if (Units.Size == 0)
        return;
    if (MaxHelpRange == 0.0f)
        MaxHelpRange = 600.0f;                                    // 0x44160000
    int n = 0;
    StartPos[0] = 0.0f;
    StartPos[1] = 0.0f;
    for (int i = 0; i < Units.Size; ++i) {
        SUnit* u = U(UnitAt(Units, i));
        if (u->Parent != -1)
            continue;
        StartPos[0] = u->Pos[0] + StartPos[0];
        StartPos[1] = u->Pos[2] + StartPos[1];
        ++n;
    }
    if (n == 0)
        return;
    StartPos[0] = StartPos[0] / (float)n;
    StartPos[1] = StartPos[1] / (float)n;
}

// PANZERS 0x5d9560
void SAIGroup::AddUnit(int unit)
{
    int i = AddInt(Units);
    Units.Array[i] = unit;
}

// PANZERS 0x5f7990
void SAIGroup::RemoveUnit(int unit)
{
    for (int i = 0; i < Units.Size; ++i) {
        if (Units.Array[i] != unit)
            continue;
        --Units.Size;
        int tail = Units.Size - i;
        if (tail != 0)
            memmove(&Units.Array[i], &Units.Array[i + 1], tail * sizeof(int));   // 0x76bb40
        Units.Array[Units.Size] = 0;
        return;
    }
}

// PANZERS 0x570720 (SGameLogic): a found-unit list of the live units, its
// stats (0x582770) and a movement group to (x, z) (0x56ff30(0, 0, .., p5)).
static void GroupOrderUnits(const SHdArray<int>& units, bool p5, float x, float z)
{
    SFoundUnits f;
    memset(&f, 0, sizeof(f));
    for (int i = 0; i < units.Size; ++i) {
        int u = UnitAt(units, i);
        if (!g_World->Units.IsLive(u))
            continue;
        if (f.Count == f.Max) {
            int n = f.Max < 0x10 ? 0x10 : (f.Max * 6) / 5;
            f.Units = (SFoundUnit*)realloc(f.Units, n * sizeof(SFoundUnit));
            memset(f.Units + f.Max, 0, (n - f.Max) * sizeof(SFoundUnit));
            f.Max = n;
        }
        f.Units[f.Count++].Unit = u;
    }
    GroupStats(&f);                                               // 0x582770
    g_GameLogic->GroupOrder(false, 0, &f, p5, x, z);              // 0x56ff30
    free(f.Units);
}

// PANZERS 0x5f5000
// Status 1: every top-level unit moves to (x, z).
void SAIGroup::MoveTo(float x, float z)
{
    GroupOrderUnits(Units, true, x, z);
    for (int i = 0; i < Units.Size; ++i) {
        SUnit* u = U(UnitAt(Units, i));
        if (u->Parent < 0)
            u->EC_Move(BitsOf(x), BitsOf(z), 0, false, 0);        // +0xac
    }
    Status = 1;
}

// PANZERS 0x5d95b0
// Status 2: every top-level unit not already busy with an order of kind 5
// attack-moves to (x, z).
void SAIGroup::AttackMoveTo(float x, float z)
{
    GroupOrderUnits(Units, true, x, z);
    for (int i = 0; i < Units.Size; ++i) {
        SUnit* u = U(UnitAt(Units, i));
        if (u->Parent >= 0)
            continue;
        if (u->PrimaryTarget && u->PrimaryTarget->Kind == 5)
            continue;
        u->EC_AttackMove(x, z, 0);                // +0xcc (x, z, queue)
    }
    TargetPos[0] = x;
    Status = 2;
    TargetPos[1] = z;
    AttackTarget = true;
}

// PANZERS 0x5e7990 SWorld::GetNearestPathNode
static int NearestPathNode(int path, float x, float z)
{
    if (path < 0)
        Logger.g->Panic("SWorld::GetNearestPathNode: Invalid path");
    SHeap<SPath>& paths = g_World->Paths;
    if (!paths.IsLive(path))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SPath", path);
    const SPath& p = paths.Array[path].Data;
    int best = -1;
    int bestD = 0x7fffffff;
    for (int i = 0; i < p.Points.Size; ++i) {
        float dz = p.Points.Array[i].Z - z;
        float dx = p.Points.Array[i].X - x;
        if (dx * dx + dz * dz < (float)bestD) {
            dz = p.Points.Array[i].Z - z;
            dx = p.Points.Array[i].X - x;
            bestD = (int)(dx * dx + dz * dz);                     // cvttss2si
            best = i;
        }
    }
    return best;
}

// PANZERS 0x5d9750
// The group follows the path from the node nearest its first unit.
void SAIGroup::MoveAlongPath(int path)
{
    SUnit* first = U(UnitAt(Units, 0));
    int node = NearestPathNode(path, first->Pos[0], first->Pos[2]);
    SHeap<SPath>& paths = g_World->Paths;
    if (!paths.IsLive(path))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SPath", path);
    const SPath& p = paths.Array[path].Data;
    if (node < 0 || node >= p.Points.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SPathPoint", node);
    GroupOrderUnits(Units, true, p.Points.Array[node].X, p.Points.Array[node].Z);
    for (int i = 0; i < Units.Size; ++i) {
        SUnit* u = U(UnitAt(Units, i));
        if (u->Parent < 0)
            u->EC_AttackAlongPath(path, node - 1, 0);             // +0xd0
    }
    PathIdx = path;
}

// PANZERS 0x5ef440
// Whether the first units of both groups belong to allied players.
bool SAIGroup::IsAlliedWith(int group)
{
    if (Units.Size < 1)
        return false;
    SHeap<SAIGroup>& h = AIGroupHeap(g_World);
    if (!h.IsLive(group))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SAIGroup", group);
    if (h.Array[group].Data.Units.Size < 1)
        return false;
    SAIGroup* o = AIGroupAt(group);
    int other = U(UnitAt(o->Units, 0))->Player;
    int mine = U(UnitAt(Units, 0))->Player;
    return PlayersAllied(mine, other);
}

// PANZERS 0x5b6e70 (SUnit): the unit reaches (x, z): the free spot near it is
// within its prototype +0xec range on a free straight line, or A* finds a path.
static bool UnitCanReach(SUnit* u, float x, float z)
{
    u->SetOnBlockMap(false);                                      // +0x198(0)
    float out[2];
    const float* p = g_World->FindEmptySpaceNear(out, x, z, u->Pos[0], u->Pos[2], u->UnitSizeBlocks2,
                                                 (unsigned)u->MoveFlags, false);   // 0x5e5700
    float fx = p[0], fz = p[1];
    u->SetOnBlockMap(true);
    float r = u->Proto->_ec;                                      // prototype +0xec
    if ((fx - u->Pos[0]) * (fx - u->Pos[0]) + (fz - u->Pos[2]) * (fz - u->Pos[2]) < r * r &&
        BlockMap_LineFree(g_World, u->Pos[0], u->Pos[2], fx, fz, u->UnitSizeBlocks2, (unsigned)u->MoveFlags))   // 0x6007d0
        return true;
    int result = 0;
    AStar_FindPath(World_GetGlobalAStar(g_World), u, &result, u->Pos[0], u->Pos[2], fx, fz, u->UnitSizeBlocks2,
                   (unsigned)u->MoveFlags, false);                // 0x5e70e0, 0x5a3270
    return result == 2;
}

// PANZERS 0x5e63e0
// Asks the group's biggest unit (largest +0x5c).
bool SAIGroup::CanReach(float x, float z)
{
    if (Units.Size == 0)
        return false;
    float biggest = -1.0f;                                        // DAT_007f5a98
    float idx = -1.0f;                                            // HD keeps the index in a float
    for (int i = 0; i < Units.Size; ++i) {
        SUnit* u = U(UnitAt(Units, i));
        if (biggest < (float)u->UnitSizeBlocks2) {
            biggest = (float)u->UnitSizeBlocks2;
            idx = (float)UnitAt(Units, i);
        }
    }
    return UnitCanReach(U((int)idx), x, z);
}

// PANZERS 0x5f5c70
// Once a second (every 20th frame) per group: defend groups pull their idle
// units back home, attacking groups wait for their units, attack groups pick a
// random enemy (world LCG), support groups go and help allied groups in range.
void SAIGroup::Refresh()
{
    PZ_M3_TRACE("SAIGroup::Refresh (0x5f5c70)");
    if (Timer > 0)
        --Timer;
    if (AttackMoveTimer > 0)
        --AttackMoveTimer;

    if (Tactic == 0) {
        // Defend: idle units (behaviour 0) away from home walk back.
        for (int i = 0; i < Units.Size; ++i) {
            SUnit* u = U(UnitAt(Units, i));
            STarget* t = u->CurrentTarget;                        // +0x1f4
            bool home = t && t->Kind == 9 && t->Type == 2 && t->Pos[0] == StartPos[0] && t->Pos[2] == StartPos[1];
            if (!home && u->Behavior == 0) {
                float dz = u->Pos[2] - StartPos[1];
                float dx = u->Pos[0] - StartPos[0];
                if (dx * dx + dz * dz > 400.0f)                   // _DAT_007fd70c
                    u->EC_Move(BitsOf(StartPos[0]), BitsOf(StartPos[1]), 0, false, 0);   // +0xac
            }
        }
    }
    if (Tactic == 0 && HasCalledForHelp) {
        for (int i = 0; i < Units.Size; ++i)
            if (U(UnitAt(Units, i))->CurrentTarget) {
                Timer = 5;
                return;
            }
        if (Timer != 0)
            return;
        HasCalledForHelp = false;
        // 0x568ae0 (an empty debug hook) and the log, both with the same text.
        if (Logger.g)
            Logger.g->Log(1, "AI [ %s]: DEFEND SUCCESS! ", SStr(ID));
        return;
    }

    if (Status == 1) {
        // Moving: done once no unit has an order left.
        for (int i = 0; i < Units.Size; ++i)
            if (U(UnitAt(Units, i))->PrimaryTarget)
                return;
        Status = 0;
        return;
    }
    if (Status != 2) {
        if (Tactic == 3 && Status == 0)
            RefreshSupport();
        return;
    }

    // Status 2: attacking.
    for (int i = 0; i < Units.Size; ++i) {
        SUnit* u = U(UnitAt(Units, i));
        if (u->PrimaryTarget || (Tactic == 3 && u->CurrentTarget)) {
            Timer = 5;
            return;
        }
    }
    bool attackTarget = AttackTarget;
    if (attackTarget && Units.Size > 0) {
        SUnit* u = U(UnitAt(Units, 0));
        if (u->ActiveDriver > -1) {
            float dx = u->Pos[0] - TargetPos[0];
            float dz = u->Pos[2] - TargetPos[1];
            if (dx * dx + dz * dz > 1600.0f) {                    // _DAT_00801b14
                AttackMoveTo(TargetPos[0], TargetPos[1]);
                return;
            }
        }
    }
    if (Timer != 0)
        return;
    if (Tactic != 4) {
        if (!attackTarget) {
            Status = 0;
            if (Logger.g)
                Logger.g->Log(1, "AIGroup: Timer 0 Status == AI_STAND");
            return;
        }
        AttackTarget = false;
        if (PathIdx > -1) {
            MoveAlongPath(PathIdx);
            return;
        }
        MoveTo(StartPos[0], StartPos[1]);
        return;
    }
    // Attack: the enemy top-level unit (class 0, 0xb or 5) with the lowest
    // random draw (one world LCG draw per candidate, in heap order).
    int best = 0x7fffffff;
    int bestUnit = -1;
    SUnitHeap& h = g_World->Units;
    for (int k = 0; k < h.Size; ++k) {
        if (h.Array[k].Next != kHeapLive)
            continue;
        SUnit* c = h.Array[k].Unit;
        if (c->Parent >= 0 || c->Unplaced || c->Wrecked)          // +0x78, +0x168, +0x150
            continue;
        int ct = c->Proto->ClassType;
        if (ct != 0 && ct != 0xb && ct != 5)
            continue;
        int mine = U(UnitAt(Units, 0))->Player;
        if (PlayersAllied(mine, c->Player))
            continue;
        int r = WorldRand();
        int v = (int)((double)r * 3.0517578125e-05 * 1000.0);     // cvttsd2si (DAT_007f4540, DAT_007f7f80)
        if (v < best) {
            best = v;
            bestUnit = k;
        }
    }
    if (bestUnit < 0)
        return;
    SUnit* e = U(bestUnit);
    AttackMoveTo(e->Pos[0], e->Pos[2]);
}

// The Tactic 3 / Status 0 part of 0x5f5c70: a support group (repairers,
// suppliers, medics) goes to an allied group in range whose units need it.
void SAIGroup::RefreshSupport()
{
    bool supply = false, repair = false, medic = false;
    for (int i = 0; i < Units.Size; ++i) {
        SUnit* u = U(UnitAt(Units, i));
        if (u->Proto->ClassType == 0 && u->Proto->Supporter) {    // SPSingleUnit +0xe0
            if (0.1f < U(UnitAt(Units, i))->Cargo)                // 0x5d6370, +0x2ec, DAT_007f59a8
                supply = true;
        }
        u = U(UnitAt(Units, i));
        if (u->Proto->ClassType == 0 && u->Proto->Repairer) {     // +0xdf
            if (0.1f < U(UnitAt(Units, i))->Cargo)
                repair = true;
        }
        if (U(UnitAt(Units, i))->Proto->SupportPlace)             // +0xde (medics)
            medic = true;
    }
    if (!supply && !repair && !medic)
        return;
    SHeap<SAIGroup>& gh = AIGroupHeap(g_World);
    for (int gi = 0; gi < gh.Size; ++gi) {
        if (gh.Array[gi].Next != kHeapLive)
            continue;
        if (!IsAlliedWith(gi))
            continue;
        bool disabled = false;
        for (int d = 0; d < DisabledAIGroups.Size; ++d)
            if (DisabledAIGroups.Array[d] == gi) {
                disabled = true;
                break;
            }
        if (disabled)
            continue;
        SAIGroup* o = &gh.Array[gi].Data;
        if (o->ID.size == ID.size) {
            if (o->ID.size == 0 || _stricmp(o->ID.buf, ID.buf) == 0)
                continue;
        }
        float dx = StartPos[0] - o->StartPos[0];
        float dz = StartPos[1] - o->StartPos[1];
        if (dx * dx + dz * dz > MaxHelpRange * 0.5f * MaxHelpRange * 0.5f)
            continue;
        dz = StartPos[1] - o->StartPos[1];
        dx = StartPos[0] - o->StartPos[0];
        if (dx * dx + dz * dz > o->MaxHelpRange * 0.5f * o->MaxHelpRange * 0.5f)
            continue;
        int score = 0;
        for (int j = 0; j < o->Units.Size; ++j) {
            SUnit* v = U(UnitAt(o->Units, j));
            if (v->Proto->ArmourType != 0) {
                if (v->HP < 0.5f && repair)
                    score += 2;
                if (v->Gunners.Size > 0 && v->Gunners.Array[0]->GetWeaponType() != 0) {   // 0x584240
                    if (v->Gunners.Array[0]->AmmoLeft < 0.3f && supply)
                        score += 2;
                }
            }
            if (v->HasWoundedMember() && medic)                   // +0x80
                score += 2;
        }
        if (score < 2)
            continue;
        SUnit* first = U(UnitAt(o->Units, 0));
        float x = first->Pos[0], z = first->Pos[2];
        if (CanReach(x, z)) {
            GroupOrderUnits(Units, true, x, z);
            for (int i = 0; i < Units.Size; ++i)
                U(UnitAt(Units, i))->EC_Move(BitsOf(x), BitsOf(z), 0, false, 0);   // +0xac
            Status = 2;
            return;
        }
        int k = AddInt(DisabledAIGroups);                         // 0x560a00
        DisabledAIGroups.Array[k] = gi;
    }
}

// x87 tail of 0x5e74e0 / 0x5e7110 / 0x5d68e0: GetHitPoints (+0x174) * HP
// (fmul), + the sum as a float (cvtdq2ps, fadd), fstp dword, cvttss2si.
static int AddHitPoints(SUnit* u, int sum)
{
    float hp = u->GetHitPoints();                                 // +0x174
    float t = (float)((double)hp * (double)u->HP + (double)(float)sum);
    return (int)t;
}

// PANZERS 0x5e74e0
// The group's strength: every armed unit's hit points (a squad: 100 per
// member), +600 for class 0xb, +500 for armour type 2.
int SAIGroup::Strength()
{
    int s = 0;
    for (int i = 0; i < Units.Size; ++i) {
        SUnit* u = U(UnitAt(Units, i));
        if (u->Gunners.Size > 0) {
            if (u->Proto->ClassType == 5)
                s += u->Members.Size * 100;
            else
                s = AddHitPoints(u, s);
        }
        u = U(UnitAt(Units, i));
        if (u->Proto->ClassType == 0xb)
            s += 600;
        if (u->Proto->ArmourType == 2)
            s += 500;
    }
    return s;
}

// PANZERS 0x5e7110
// The group's anti-tank strength: the hit points of units whose first gunner
// fires AT (type 1); a squad whose first member's gunner fires type 1 or 3
// counts 100 per member.
int SAIGroup::AntiTankStrength()
{
    int s = 0;
    for (int i = 0; i < Units.Size; ++i) {
        SUnit* u = U(UnitAt(Units, i));
        if (u->Gunners.Size > 0 && u->Gunners.Array[0]->GetWeaponType() == 1)   // 0x584240
            s = AddHitPoints(u, s);
        u = U(UnitAt(Units, i));
        if (u->Proto->ClassType != 5 || u->Members.Size <= 0)
            continue;
        SUnit* m = U(u->Members.Array[0].Unit);
        if (m->Gunners.Size < 1)
            continue;
        if (m->Gunners.Array[0]->GetWeaponType() != 1 &&
            m->Gunners.Array[0]->GetWeaponType() != 3)
            continue;
        s += u->Members.Size * 100;
    }
    return s;
}

// PANZERS 0x5642d0 (SGameLogic): the live units within `r` of (x, z), in heap
// order (HD keeps {unit, distance^2} pairs; only the units are read here).
static void UnitsInRadius(float x, float z, float r, SHdArray<int>& out)
{
    SUnitHeap& h = g_World->Units;
    for (int k = 0; k < h.Size; ++k) {
        if (h.Array[k].Next != kHeapLive)
            continue;
        SUnit* u = h.Array[k].Unit;
        float dx = x - u->Pos[0];
        float dz = z - u->Pos[2];
        float d = dx * dx + dz * dz;
        if (d < r * r) {
            int i = AddInt(out);
            out.Array[i] = k;
        }
    }
}

// 0x5335c0("AI [ ", group) + 0x52c580(text) handed to SGameLogic 0x568ae0,
// an empty function in HD. Recompile only: PZ_M6_AILOG=1 logs the text.
static bool AiLogOn()
{
    static int on = -1;
    if (on < 0) {
        const char* e = getenv("PZ_M6_AILOG");
        on = (e && *e && *e != '0' && Logger.g) ? 1 : 0;
    }
    return on == 1;
}

static void AiDebugText(SAIGroup* g, const char* text)
{
    if (AiLogOn())
        Logger.g->Log(0, "PZM6 AI [ %s%s", SStr(g->ID), text);
}

static int& PlayerInt(SWorld* w, int player, unsigned off)        // World+0x170 + player * 0x48 + off
{
    return *(int*)(w->Players[player] + off);
}

// The squared distance of `v` to group k's first unit, truncated (0x5d6370,
// 0x546490; HD computes dz * dz first).
static int HelperScore(SUnit* v, int k)
{
    SUnit* f = U(UnitAt(AIGroupAt(k)->Units, 0));
    float dz = v->Pos[2] - f->Pos[2];
    float a = (v->Pos[2] - f->Pos[2]) * dz;
    float dx = v->Pos[0] - f->Pos[0];
    return (int)((v->Pos[0] - f->Pos[0]) * dx + a);               // cvttss2si
}

// Group k is in help range of group `mine` (each one's MaxHelpRange / 2).
static bool InHelpRange(int mine, int k)
{
    SAIGroup* c = AIGroupAt(k);
    SAIGroup* v = AIGroupAt(mine);
    float dx = v->StartPos[0] - c->StartPos[0];
    float dz = v->StartPos[1] - c->StartPos[1];
    float d = dx * dx + dz * dz;
    float h = AIGroupAt(mine)->MaxHelpRange * 0.5f;               // DAT_007f453c
    if (d > AIGroupAt(mine)->MaxHelpRange * 0.5f * h)
        return false;
    c = AIGroupAt(k);
    v = AIGroupAt(mine);
    dx = v->StartPos[0] - c->StartPos[0];
    dz = v->StartPos[1] - c->StartPos[1];
    d = dx * dx + dz * dz;
    h = AIGroupAt(k)->MaxHelpRange * 0.5f;
    if (d > AIGroupAt(k)->MaxHelpRange * 0.5f * h)
        return false;
    return true;
}

// PANZERS 0x5d68e0
// An AI unit (`unit`, of an AI group) was attacked by `attacker` (callers:
// SSingleUnit::OnAttackedBy 0x5b0420, SPanzersSquadUnit::OnAttackedBy
// 0x59ef00). Attack groups (tactic 2 / 4) attack-move to the attacker every
// 15 refreshes; tactic 4 calls the player's support (1 in 20, one world
// draw); then, once per group (HasCalledForHelp), the enemy strength within
// 40 m of the attacker is weighed against the group's: tactic 1 groups pull
// back to the nearest allied group (a tactic 2 one attacks) or attack;
// others retreat to an allied group (enemy 3x stronger), call an allied
// tactic-2 group (stronger), or hold.
void SWorld::AIGroupUnitAttacked(int unit, int attacker)
{
    PZ_M3_TRACE("SWorld 0x5d68e0");
    SUnit* victim = WorldUnit(unit);
    SUnit* att = WorldUnit(attacker);
    SHeap<SAIGroup>& gh = AIGroupHeap(this);
    int gi = victim->AIGroup;
    if (!gh.IsLive(gi))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SAIGroup", gi);
    if (AiLogOn())
        Logger.g->Log(0, "PZM6 AI 0x5d68e0 frame %d unit %d attacker %d group %d '%s' tactic %d status %d called %d timer %d",
                      g_GameLogic ? g_GameLogic->GetFrame() : -1, unit, attacker, gi, SStr(gh.Array[gi].Data.ID),
                      gh.Array[gi].Data.Tactic, gh.Array[gi].Data.Status, (int)gh.Array[gi].Data.HasCalledForHelp,
                      gh.Array[gi].Data.AttackMoveTimer);
    if (gh.Array[gi].Data.Tactic == 5)
        return;
    if ((gh.Array[gi].Data.Tactic == 2 || AIGroupAt(gi)->Tactic == 4) &&
        AIGroupAt(victim->AIGroup)->AttackMoveTimer == 0) {
        AIGroupAt(victim->AIGroup)->AttackMoveTimer = 15;
        float x = att->Pos[0], z = att->Pos[2];
        AIGroupAt(victim->AIGroup)->AttackMoveTo(x, z);           // 0x5d95b0
        AiDebugText(AIGroupAt(victim->AIGroup), "]: Attack move!");
    }

    if (AIGroupAt(victim->AIGroup)->Tactic == 4) {
        int r = WorldRand();
        if ((int)((double)r * 3.0517578125e-05 * 20.0) == 0) {     // DAT_007f4540, DAT_007f5a58
            int p = victim->Player;
            if (PlayerInt(this, p, 0x34) > 0 && att->Proto->ArmourType == 2 && att->Speed == 0.0f) {
                g_GameLogic->SupportTacBomber(false, att->Pos[0], att->Pos[2], p);   // 0x568740
                AiDebugText(AIGroupAt(victim->AIGroup), "]: Call AI (TacBomber) support!");
            } else if (PlayerInt(this, p, 0x34) == 0 && PlayerInt(this, p, 0x3c) > 0 && att->Speed == 0.0f) {
                do {
                    g_GameLogic->SupportParatroopers(false, att->Pos[0], att->Pos[2], p, -1.0f, true, false, 0.0f);   // 0x567d40
                    AiDebugText(AIGroupAt(victim->AIGroup), "]: Call AI (Parachute) support!");
                    p = victim->Player;
                } while (PlayerInt(this, p, 0x3c) != 0);
            }
            p = victim->Player;
            if (PlayerInt(this, p, 0x2c) > 0 && att->Proto->ArmourType == 0 && att->Speed == 0.0f) {
                g_GameLogic->SupportArtillery(false, att->Pos[0], att->Pos[2], p);   // 0x5674c0
                AiDebugText(AIGroupAt(victim->AIGroup), "]: Call AI (Cannonade) support!");
            } else if (PlayerInt(this, p, 0x38) > 0 && att->Proto->ArmourType == 0 && att->Speed == 0.0f) {
                g_GameLogic->SupportHeavyBomber(false, att->Pos[0], att->Pos[2], p, -1.0f, true, false, 0.0f);   // 0x567760
                AiDebugText(AIGroupAt(victim->AIGroup), "]: Call AI (HeavyBomber) support!");
            }
        }
    }

    if (AIGroupAt(victim->AIGroup)->HasCalledForHelp)
        return;
    if (AIGroupAt(victim->AIGroup)->Tactic == 2 && AIGroupAt(victim->AIGroup)->Status == 2)
        return;
    if (AIGroupAt(victim->AIGroup)->Tactic == 4 && PlayerInt(this, victim->Player, 0x30) > 0) {
        g_GameLogic->SupportRecon(false, att->Pos[0], att->Pos[2], victim->Player, -1.0f, true);   // 0x568300
        AiDebugText(AIGroupAt(victim->AIGroup), "]: Call AI (Recon) support!");
    }
    AIGroupAt(victim->AIGroup)->HasCalledForHelp = true;

    // The enemy strength within 40 m of the attacker.
    int enemy = 0;
    bool armoured = false;
    SHdArray<int> found;
    memset(&found, 0, sizeof(found));
    UnitsInRadius(att->Pos[0], att->Pos[2], 40.0f, found);        // 0x5642d0
    for (int i = 0; i < found.Size; ++i) {
        SUnit* c = U(found.Array[i]);
        if (c->Wrecked || c->Unplaced || c->_110)
            continue;
        int ct = c->Proto->ClassType;
        if (ct != 0 && ct != 0xb && ct != 0xc && ct != 5)
            continue;
        if (PlayersAllied(WorldUnit(unit)->Player, c->Player))    // 0x546490, 0x549ab0
            continue;
        int kind = PlayerInt(this, c->Player, 0x08);
        if (kind == 3 || kind == 2 || kind == 4)
            continue;
        if (c->Gunners.Size <= 0)
            continue;
        if (c->Proto->ClassType == 5)
            enemy += c->Members.Size * 100;
        else
            enemy = AddHitPoints(c, enemy);
        if (c->Proto->UnitType == 0x17)
            enemy += 500;
        if (c->Proto->ClassType == 0xb)
            enemy += 600;
        if (c->Proto->ArmourType == 2) {
            enemy += 500;
            armoured = true;
        }
    }
    int own = AIGroupAt(victim->AIGroup)->Strength();             // 0x5e74e0
    if (AiLogOn())
        Logger.g->Log(0, "PZM6 AI   enemy %d own %d armoured %d", enemy, own, (int)armoured);

    if (AIGroupAt(victim->AIGroup)->Tactic == 1) {
        if (enemy <= own) {
            AIGroupAt(victim->AIGroup)->AttackMoveTo(att->Pos[0], att->Pos[2]);
            AiDebugText(AIGroupAt(victim->AIGroup), "]: Last man standing!");
            free(found.Array);
            return;
        }
        // The nearest reachable allied group in range; a tactic 2 group
        // (then tactic 4) wins over the others.
        int best = -1;
        int bestScore = 10000;
        for (int k = 0; k < gh.Size; ++k) {
            if (gh.Array[k].Next != kHeapLive)
                continue;
            if (!AIGroupAt(victim->AIGroup)->IsAlliedWith(k) || k == victim->AIGroup)   // 0x5ef440
                continue;
            if (!InHelpRange(victim->AIGroup, k))
                continue;
            if (!AIGroupAt(k)->CanReach(victim->Pos[0], victim->Pos[2]))   // 0x5e63e0
                continue;
            if (armoured && AIGroupAt(k)->AntiTankStrength() == 0)     // 0x5e7110
                continue;
            if (best > -1) {
                if (AIGroupAt(best)->Tactic == 2 && AIGroupAt(k)->Tactic != 2)
                    continue;
                if (AIGroupAt(best)->Tactic != 2 &&
                    (AIGroupAt(k)->Tactic == 2 || AIGroupAt(k)->Tactic == 4)) {
                    best = k;
                    bestScore = HelperScore(victim, k);
                    continue;
                }
            }
            int s = HelperScore(victim, k);
            if (s < bestScore) {
                bestScore = s;
                best = k;
            }
        }
        if (best <= -1) {
            free(found.Array);
            return;
        }
        // The attacked group falls back to the helper's first unit.
        SUnit* f = U(UnitAt(AIGroupAt(best)->Units, 0));
        for (int i = 0;; ++i) {
            SAIGroup* g = AIGroupAt(victim->AIGroup);
            if (i >= g->Units.Size)
                break;
            U(UnitAt(g->Units, i))->EC_Move(BitsOf(f->Pos[0]), BitsOf(f->Pos[2]), 1, false, 0);   // +0xac
        }
        if (AIGroupAt(best)->Tactic != 2) {
            free(found.Array);
            return;
        }
        AIGroupAt(best)->AttackMoveTo(att->Pos[0], att->Pos[2]);
        AIGroupAt(victim->AIGroup)->Status = 2;
        AIGroupAt(victim->AIGroup)->Timer = 50;
        AiDebugText(AIGroupAt(victim->AIGroup), "]: Call support!");
        free(found.Array);
        return;
    }

    if (enemy > own * 3 && AIGroupAt(victim->AIGroup)->Tactic != 0) {
        // Retreat to the nearest reachable allied group in range.
        int best = -1;
        int bestScore = 1000000;
        for (int k = 0; k < gh.Size; ++k) {
            if (gh.Array[k].Next != kHeapLive)
                continue;
            if (!AIGroupAt(victim->AIGroup)->IsAlliedWith(k) || k == victim->AIGroup)
                continue;
            if (AIGroupAt(k)->Units.Size == 0)
                continue;
            if (!InHelpRange(victim->AIGroup, k))
                continue;
            if (!AIGroupAt(k)->CanReach(victim->Pos[0], victim->Pos[2]))
                continue;
            int s = HelperScore(victim, k);
            if (s < bestScore) {
                bestScore = s;
                best = k;
            }
        }
        if (best > -1) {
            SUnit* f = U(UnitAt(AIGroupAt(best)->Units, 0));
            for (int i = 0;; ++i) {
                SAIGroup* g = AIGroupAt(victim->AIGroup);
                if (i >= g->Units.Size)
                    break;
                U(UnitAt(g->Units, i))->EC_Move(BitsOf(f->Pos[0]), BitsOf(f->Pos[2]), 0, false, 0);   // +0xac
            }
            AiDebugText(AIGroupAt(victim->AIGroup), "]: Retreat!!");
        } else {
            AiDebugText(AIGroupAt(victim->AIGroup), "]: OMG! WTF?!");
        }
    } else if (enemy > own) {
        // Call the nearest reachable allied tactic-2 group in range.
        int best = -1;
        int bestScore = 1000000;
        for (int k = 0; k < gh.Size; ++k) {
            if (gh.Array[k].Next != kHeapLive)
                continue;
            if (!AIGroupAt(victim->AIGroup)->IsAlliedWith(k) || k == victim->AIGroup)
                continue;
            if (AIGroupAt(k)->Units.Size == 0 || AIGroupAt(k)->Tactic != 2)
                continue;
            if (!InHelpRange(victim->AIGroup, k))
                continue;
            if (!AIGroupAt(k)->CanReach(victim->Pos[0], victim->Pos[2]))
                continue;
            if (armoured && AIGroupAt(k)->AntiTankStrength() == 0)
                continue;
            int s = HelperScore(victim, k);
            if (s < bestScore) {
                bestScore = s;
                best = k;
            }
        }
        if (best <= -1) {
            AiDebugText(AIGroupAt(victim->AIGroup), "]: Defending");
        } else {
            GroupOrderUnits(AIGroupAt(best)->Units, true, att->Pos[0], att->Pos[2]);   // 0x570720
            AIGroupAt(best)->AttackMoveTo(att->Pos[0], att->Pos[2]);
            AiDebugText(AIGroupAt(victim->AIGroup), "]: Call support!");
        }
    } else {
        AiDebugText(AIGroupAt(victim->AIGroup), "]: Enemy is too weak");
    }
    free(found.Array);
}

// SGameLogic::Refresh 0x576d80 (0x577958..0x5779b4): every live group.
void RefreshAIGroups()
{
    if (!g_World)
        return;
    SHeap<SAIGroup>& h = AIGroupHeap(g_World);
    for (int i = 0; i < h.Size; ++i)
        if (h.Array[i].Next == kHeapLive)
            h.Array[i].Data.Refresh();
}

} // namespace pz
