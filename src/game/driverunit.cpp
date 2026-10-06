// src/game/driverunit.cpp
// Unit / world / game-logic access for the drivers (see driverunit.h).
// OWNER: P.

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include "driverunit.h"
#include "driver.h"
#include "target.h"
#include "iunit.h"
#include "gamelogic.h"
#include "world.h"
#include "worldapi.h"
#include "logger.h"

namespace pz {

// ---------------------------------------------------------------------------
// Logging

void DrvPanic(const char* fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    // One line per distinct message (a broken state can repeat every tick).
    static char s_seen[32][96];
    static int s_count = 0;
    for (int i = 0; i < s_count; ++i)
        if (strncmp(s_seen[i], buf, sizeof(s_seen[i]) - 1) == 0)
            return;
    if (s_count < 32) {
        strncpy(s_seen[s_count], buf, sizeof(s_seen[s_count]) - 1);
        s_seen[s_count][sizeof(s_seen[s_count]) - 1] = 0;
        ++s_count;
    }
    if (Logger.g)
        Logger.g->Log(0, "PZM2 HD PANIC (continuing): %s", buf);
}

void DrvWarn(const char* fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    if (Logger.g)
        Logger.g->Log(1, "%s", buf);
}

// ---------------------------------------------------------------------------
// Unit heap and paths (raw HD layouts, independent of U's SUnit type)

bool HdUnitLive(int index)
{
    if (!g_World || index < 0)
        return false;
    char* w = (char*)g_World;
    int size = DU_I(w, 0x4d8);
    if (index >= size)
        return false;
    int* arr = (int*)DU_P(w, 0x4d4);
    return arr[index * 2] == kHeapLive;
}

SIUnit* HdUnitOrNull(int index)
{
    if (!HdUnitLive(index))
        return nullptr;
    int* arr = (int*)DU_P(g_World, 0x4d4);
    return (SIUnit*)(size_t)arr[index * 2 + 1];
}

// PANZERS 0x546490 (SHeapTRB::operator[] for the unit heap)
SIUnit* HdUnit(int index)
{
    SIUnit* u = HdUnitOrNull(index);
    if (!u)
        DrvPanic("SHeapTRB::operator[]: invalid index (%d)", index);
    return u;
}

SHdPath* HdPath(int index)
{
    if (g_World && index >= 0) {
        char* w = (char*)g_World;
        int size = DU_I(w, 0x7498);
        SHdPath* arr = (SHdPath*)DU_P(w, 0x7494);
        if (index < size && arr && arr[index].Next == kHeapLive)
            return &arr[index];
    }
    DrvPanic("SHeap<%s>::operator[]: invalid index (%d)", "SPath", index);
    return nullptr;
}

// ---------------------------------------------------------------------------
// Ghost-frame queue (unit +0x1d8)

// PANZERS 0x5500a0
SGhostFrame* GhostAt(void* unit, int index)
{
    SGhostQueue* q = DU_Ghosts(unit);
    if (index < q->Bottom || index > q->Top) {
        DrvPanic("SDEQueue<%s>::operator[]: invalid index (%d)", "SGhostFrame", index);
        static SGhostFrame s_dummy;
        memset(&s_dummy, 0, sizeof(s_dummy));
        return &s_dummy;
    }
    int slot = index - q->Base;
    if (slot >= q->Max)
        slot -= q->Max;
    return &q->Array[slot];
}

// PANZERS 0x5ba790 (SDEQueue grow)
static void GhostGrow(SGhostQueue* q)
{
    int n = q->Max < 0x10 ? 0x10 : (q->Max * 6) / 5;
    SGhostFrame* a = (SGhostFrame*)::realloc(q->Array, (size_t)n * sizeof(SGhostFrame));
    int old = q->Max;
    q->Array = a;
    if (old <= q->Top - q->Base) {
        int tail = (q->Base - q->Bottom) + old;
        memmove(a + (n - tail), a + (old - tail), (size_t)tail * sizeof(SGhostFrame));
        q->Base += q->Max - n;
    }
    q->Max = n;
}

// PANZERS 0x5b5a40
int GhostAddTop(void* unit)
{
    SGhostQueue* q = DU_Ghosts(unit);
    if (q->Count == q->Max)
        GhostGrow(q);
    ++q->Top;
    ++q->Count;
    memset(GhostAt(unit, q->Top), 0, sizeof(SGhostFrame));
    return q->Top;
}

// PANZERS 0x5b7640
void GhostClear(void* unit)
{
    SGhostQueue* q = DU_Ghosts(unit);
    q->Count = 0;
    q->Base = 0;
    q->Top = -1;
    q->Bottom = 0;
}

// PANZERS 0x5be0b0
void GhostRemoveBottom(void* unit)
{
    SGhostQueue* q = DU_Ghosts(unit);
    if (q->Count == 0) {
        DrvPanic("SDEQueue<%s>::RemoveBottom: queue is already empty!", "SGhostFrame");
        return;
    }
    int b = q->Bottom;
    --q->Count;
    if (b - q->Base == q->Max - 1)
        q->Base += q->Max;
    if (b > q->Top) {
        DrvPanic("SDEQueue<%s>::operator[]: invalid index (%d)", "SGhostFrame", b);
        return;
    }
    q->Bottom = b + 1;
}

// PANZERS 0x5be240 (not executed in the menu)
void GhostRemoveTop(void* unit)
{
    SGhostQueue* q = DU_Ghosts(unit);
    if (q->Count == 0) {
        DrvPanic("SDEQueue<%s>::RemoveTop: queue is already empty!", "SGhostFrame");
        return;
    }
    --q->Count;
    --q->Top;
}

static SIUnit* TowedOf(SIUnit* unit)
{
    int t = DU_I(unit, 0x2d8);
    return t < 0 ? nullptr : HdUnit(t);
}

// PANZERS 0x5ba430
void UnitGhostPush(SIUnit* unit, const SGhostFrame* frame)
{
    int i = GhostAddTop(unit);
    *GhostAt(unit, i) = *frame;
    int towed = DU_I(unit, 0x2d8);
    if (towed >= 0) {
        SIUnit* t = HdUnit(towed);
        if (!t)
            return;
        SGhostFrame f = *frame;
        g_DriverEnv.UnitTowedFollow(unit, frame->X, frame->Y, frame->Z, frame->Dir,
                                    frame->Field70, t, &f.X, &f.Dir, &f.Field70);
        UnitGhostPush(t, &f);
    }
}

// PANZERS 0x5ba5a0
void UnitGhostClearAll(SIUnit* unit)
{
    GhostClear(unit);
    for (SIUnit* t = TowedOf(unit); t; t = TowedOf(t))
        GhostClear(t);
}

// PANZERS 0x5ba610
void UnitGhostRemoveBottomAll(SIUnit* unit)
{
    GhostRemoveBottom(unit);
    for (SIUnit* t = TowedOf(unit); t; t = TowedOf(t))
        GhostRemoveBottom(t);
}

// PANZERS 0x5ba680
void UnitGhostRemoveTopAll(SIUnit* unit)
{
    GhostRemoveTop(unit);
    for (SIUnit* t = TowedOf(unit); t; t = TowedOf(t))
        GhostRemoveTop(t);
}

// PANZERS 0x5ba6f0
void UnitGhostSetBottomAll(SIUnit* unit, int bottom)
{
    SGhostQueue* q = DU_Ghosts(unit);
    int old = q->Bottom;
    q->Bottom = bottom;
    q->Top += bottom - old;
    q->Base += bottom - old;
    for (SIUnit* t = TowedOf(unit); t; t = TowedOf(t)) {
        SGhostQueue* tq = DU_Ghosts(t);
        tq->Top += bottom - tq->Bottom;
        tq->Base += bottom - tq->Bottom;
        tq->Bottom = bottom;
    }
}

// ---------------------------------------------------------------------------
// SIUnit block-map slots with float arguments

static int FBits(float f)
{
    int i;
    memcpy(&i, &f, 4);
    return i;
}

bool DrvUnit_TestBlockMap(SIUnit* unit, float x, float z, float dir, int size, short mask)
{
    return unit->TestBlockMap(FBits(x), FBits(z), FBits(dir), size, mask) != 0;
}

void DrvUnit_MarkBlockMap(SIUnit* unit, bool on, float x, float z, float dir, short mask)
{
    unit->MarkBlockMap(on, FBits(x), FBits(z), FBits(dir), mask);
}

// ---------------------------------------------------------------------------
// Small SUnit functions on the movement path

// PANZERS 0x5b9bf0
SDriver* DrvUnit_ActiveDriver(SIUnit* unit)
{
    int count = DU_I(unit, 0x3c);
    SDriver** drivers = (SDriver**)DU_P(unit, 0x38);
    int over = DU_I(unit, 0x30);
    if (over >= 0) {
        if (over < count)
            return drivers[over];
        DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SDriver*", over);
        return nullptr;
    }
    int i = DU_I(unit, 0x28);
    if (i >= 0 && i < count)
        return drivers[i];
    return nullptr;
}

// PANZERS 0x5bb500
bool DrvUnit_IsMoving(SIUnit* unit)
{
    if (DU_B(unit, 0xcd) != 0)
        return true;
    SDriver* d = DrvUnit_ActiveDriver(unit);
    if (!d)
        return false;
    STarget* t = d->Target;
    if (!t)
        return false;
    float speed = DU_F(unit, 0xc8);
    if (speed == 0.0f && t->IsUnitOrder())
        return false;
    int ghosts = DU_Ghosts(unit)->Count;
    if ((ghosts == 0 || ghosts == 1) && !d->GhostRestart)
        return false;
    if (t->Mode == 1)
        return false;
    if (speed > 0.0f)
        return true;
    if (DU_F(unit, 0xd0) != 0.0f)
        return true;
    return d->GlobalWayPoints.Size > 0;
}

// PANZERS 0x5bb5c0
bool DrvUnit_IsOffStaticMap(SIUnit* unit)
{
    if (DU_S(unit, 0xd8) & 1)
        return false;
    int size = DU_I(unit, 0x58);
    if (size < 1)
        size = 1;
    return DrvUnit_TestBlockMap(unit, DU_F(unit, 0x8c), DU_F(unit, 0x94), DU_F(unit, 0xb0), size, 1);
}

// PANZERS 0x5c10f0
void DrvUnit_SetMoveState(SIUnit* unit)
{
    if (DU_B(unit, 0xf0) != 0) {
        DU_I(unit, 0xf8) = 6;
        return;
    }
    if (DU_F(unit, 0xc8) > 0.0f) {
        DU_I(unit, 0xf8) = 2;
        return;
    }
    float spin = DU_F(unit, 0xd0);
    if ((double)spin > 1e-05) {                   // 0x7fd6e8
        DU_I(unit, 0xf8) = 4;
        return;
    }
    DU_I(unit, 0xf8) = (-1e-05 > (double)spin) ? 5 : 1;   // 0x7fd718
}

// PANZERS 0x5b76a0
void DrvUnit_ClearNear(SIUnit* unit)
{
    DU_I(unit, 0x1d0) = 0;
    DU_I(unit, 0x1cc) = -1;
}

// PANZERS 0x5b9a20: (un)marks the nearby units (+0x1a8 {index, bool}) on
// the static block map with bit 0x80; the unit "except" is (un)marked at
// (x, z) instead of its own position.
void DrvUnit_MarkNear(SIUnit* unit, bool on, int except, float x, float z)
{
    if (!(DU_S(unit, 0xd8) & 0x800))
        return;
    int n = DU_I(unit, 0x1ac);
    char* arr = (char*)DU_P(unit, 0x1a8);
    for (int i = 0; i < n; ++i) {
        int idx = *(int*)(arr + i * 8);
        if (!HdUnitLive(idx))
            continue;
        if (*(arr + i * 8 + 4) != 0)
            continue;
        SIUnit* o = HdUnit(idx);
        if ((DU_B(o, 0x7c) == 0 || DU_I(o, 0x78) != DU_I(unit, 0x74))
            && DU_B(o, 0x168) == 0 && DU_B(o, 0x150) == 0
            && DU_I(unit, 0x2e0) <= DU_I(o, 0x2e0)) {
            if (!DrvUnit_IsMoving(o))
                DrvUnit_MarkBlockMap(o, on, DU_F(o, 0x8c), DU_F(o, 0x94), DU_F(o, 0xb0), 0x80);
            if (idx == except)
                DrvUnit_MarkBlockMap(o, on, x, z, 0.0f, 0x80);
        }
    }
}

// PANZERS 0x5c1220
void DrvUnit_CheckStaticMap(SIUnit* unit)
{
    char* pu = (char*)DU_P(unit, 0x04);
    if (DU_B(pu, 0x8e) != 0) {
        int player = DU_I(unit, 0xfc);
        if (DU_I(g_World, 0x178 + player * 0x48) != 0) {
            DU_S(unit, 0xd8) &= 0xfffe;
            return;
        }
    }
    if (DU_S(unit, 0xd8) & 1)
        return;
    int ct = DU_I(pu, 0x40);
    if (ct == 3 || ct == 8 || ct == 13)
        return;
    if (DrvUnit_TestBlockMap(unit, DU_F(unit, 0x8c), DU_F(unit, 0x94), DU_F(unit, 0xb0),
                             DU_I(unit, 0x58), 1))
        return;
    DU_S(unit, 0xd8) |= 1;
    int nCrew = DU_I(unit, 0x170);
    for (int i = 0; i < nCrew; ++i) {
        int idx = *(int*)((char*)DU_P(unit, 0x16c) + i * 8);
        if (SIUnit* c = HdUnit(idx))
            DU_S(c, 0xd8) |= 1;
    }
    int nMem = DU_I(unit, 0x17c);
    for (int i = 0; i < nMem; ++i) {
        int idx = *(int*)((char*)DU_P(unit, 0x178) + i * 0x18);
        if (SIUnit* m = HdUnit(idx))
            DU_S(m, 0xd8) |= 1;
    }
    int ai = DU_I(unit, 0x80);
    if (ai >= 0) {
        float out[2];
        g_DriverEnv.FindEmptySpace(out, DU_F(unit, 0x8c), DU_F(unit, 0x94),
                                   DU_F(unit, 0x8c), DU_F(unit, 0x94), DU_I(unit, 0x58),
                                   DU_S(unit, 0xd8) | 0x80, true);
        // World+0x4f4 AI groups (SHeap, 0x54 elements): +0x28/+0x2c position.
        char* w = (char*)g_World;
        int* arr = (int*)DU_P(w, 0x4f4);
        if (ai < DU_I(w, 0x4f8) && arr[ai * 0x15] == kHeapLive) {
            char* g = (char*)arr + ai * 0x54 + 4;
            DU_F(g, 0x28) = out[0];
            DU_F(g, 0x2c) = out[1];
        } else {
            DrvPanic("SHeap<%s>::operator[]: invalid index (%d)", "SAIGroup", ai);
        }
    }
}

// PANZERS 0x5c1460
void DrvUnit_CheckOnMap(SIUnit* unit)
{
    if (DU_S(unit, 0xd8) & 0x800)
        return;
    if (DrvUnit_TestBlockMap(unit, DU_F(unit, 0x8c), DU_F(unit, 0x94), DU_F(unit, 0xb0),
                             DU_I(unit, 0x58), 0x800))
        return;
    DU_S(unit, 0xd8) |= 0x800;
    g_DriverEnv.UnitSlot9C(unit, 1);
    DU_I(unit, 0x250) = 1;
    DU_B(unit, 0x33c) = 0;
    DU_B(unit, 0x111) = 0;
    int nCrew = DU_I(unit, 0x170);
    for (int i = 0; i < nCrew; ++i) {
        int idx = *(int*)((char*)DU_P(unit, 0x16c) + i * 8);
        if (SIUnit* c = HdUnit(idx))
            DU_S(c, 0xd8) |= 0x800;
    }
    int nMem = DU_I(unit, 0x17c);
    for (int i = 0; i < nMem; ++i) {
        int idx = *(int*)((char*)DU_P(unit, 0x178) + i * 0x18);
        if (SIUnit* m = HdUnit(idx))
            DU_S(m, 0xd8) |= 0x800;
    }
}

// PANZERS 0x5c1f60: each towed unit takes its next ghost frame from the
// unit pulling it (unit +0x74 GhostFrames_AddTop).
void DrvUnit_MoveTowedChain(SIUnit* unit)
{
    SIUnit* u = unit;
    while (DU_I(u, 0x2d8) >= 0) {
        SIUnit* t = HdUnit(DU_I(u, 0x2d8));
        if (!t)
            return;
        t->SetOnBlockMap(false);
        g_DriverEnv.UnitTowedFollow(u, DU_F(u, 0x8c), DU_F(u, 0x90), DU_F(u, 0x94), DU_F(u, 0xb0),
                                    DU_F(u, 0x300), t, &DU_F(t, 0x8c), &DU_F(t, 0xb0),
                                    &DU_F(t, 0x300));
        t->SetOnBlockMap(true);
        u = t;
    }
}

// PANZERS 0x5c2040: the towed unit (+0x2d8) jumps to its bottom ghost frame.
void DrvUnit_CopyPosToTowed(SIUnit* unit)
{
    int ti = DU_I(unit, 0x2d8);
    if (ti < 0)
        return;
    SIUnit* t = HdUnit(ti);
    if (!t)
        return;
    int b = DU_Ghosts(t)->Bottom;
    t->SetOnBlockMap(false);
    SGhostFrame* f = GhostAt(t, b);
    DU_F(t, 0x8c) = f->X;
    DU_F(t, 0x94) = GhostAt(t, b)->Z;
    DU_F(t, 0x90) = g_DriverEnv.TerrainHeight(DU_F(t, 0x8c), DU_F(t, 0x94));
    DU_F(t, 0xb0) = GhostAt(t, b)->Dir;
    if (DU_ClassType(t) == 10)
        DU_F(t, 0x300) = GhostAt(t, b)->Field70;
    DrvUnit_MoveTowedChain(t);
    t->SetOnBlockMap(true);
}

// ---------------------------------------------------------------------------
// Cross-agent defaults

static int Env_GetFrame()
{
    return g_GameLogic ? g_GameLogic->GetFrame() : 0;
}
static SMovementGroupElem* EnvGroup(int group)
{
    SGameLogic* gl = g_GameLogic;
    if (!gl || group < 0 || group >= gl->MovementGroupSize || gl->MovementGroups[group].Next != kHeapLive)
        return nullptr;
    return &gl->MovementGroups[group];
}
// 0x56adc0: the group's boss unit (0 when the group or the unit is gone).
static SIUnit* Env_GetMovementGroupBoss(int group)
{
    if (group < 0 || !g_GameLogic)
        return nullptr;
    return HdUnitOrNull(g_GameLogic->GetMovementGroupBossUnit(group));
}
// 0x56acf0: the group's biggest unit.
static SIUnit* Env_GetMovementGroupSecond(int group)
{
    if (group < 0 || !g_GameLogic)
        return nullptr;
    return HdUnitOrNull(g_GameLogic->GetMovementGroupBiggestUnit(group));
}
static bool Env_GetMovementGroupConvoy(int group)
{
    return g_GameLogic ? g_GameLogic->GetMovementGroupConvoy(group) != 0 : false;
}
static float Env_GetMovementGroupMoveSpeed(SIUnit* unit)
{
    return g_GameLogic ? g_GameLogic->GetMovementGroupMoveSpeed(unit) : 0.0f;
}
static void Env_GetMovementGroupUnitFormationPos(float* out, SIUnit* unit)
{
    out[0] = out[1] = 0.0f;
    // HD 0x56b240(out, unit); L takes the world index.
    if (g_GameLogic)
        g_GameLogic->GetMovementGroupUnitFormationPos(DU_I(unit, 0x74), out);
}
// 0x56af90: the group formation direction (+0x20, float bits).
static float Env_GetMovementGroupFormationDir(int group)
{
    SMovementGroupElem* mg = EnvGroup(group);
    if (!mg) {
        DrvPanic("SGameLogic::GetMovementGroupFormationDir: Invalid movement group %d", group);
        return 0.0f;
    }
    float f;
    memcpy(&f, &mg->FormationDir2, 4);
    return f;
}
static void Env_SetMovementGroupFormationDir(int group, float dir)
{
    // HD 0x57ffc0(group, float dir); L stores the bits at +0x20.
    if (g_GameLogic)
        g_GameLogic->SetMovementGroupFormationDir2(group, FBits(dir));
}
// 0x56b110 GetMovementGroupUnitDistSqrFromTarget: the member's +0x0c, -1.0
// when the unit is not a member.
static float Env_GetMovementGroupDistance(SIUnit* unit)
{
    int group = DU_I(unit, 0x254);
    SMovementGroupElem* mg = EnvGroup(group);
    if (!mg) {
        DrvPanic("SGameLogic::GetMovementGroupUnitDistSqrFromTarget: Invalid movement group %d", group);
        return -1.0f;
    }
    for (int k = 0; k < mg->MemberCount; ++k)
        if (mg->Members[k].Unit == DU_I(unit, 0x74))
            return mg->Members[k].DistSq;
    return -1.0f;
}
static bool Env_CanSeeGroundUnit(int player, SIUnit* unit)
{
    return g_GameLogic ? g_GameLogic->CanSeeGroundUnit(player, unit) : true;
}
static float* Env_FindEmptySpace(float* out, float x, float z, float, float, int, int, bool)
{
    // U's SWorld::FindEmptySpace 0x5e5700; without U: the point itself.
    out[0] = x;
    out[1] = z;
    return out;
}
static float* Env_FindEmptySpaceDir(float* out, float x, float z, float, int, int, bool)
{
    out[0] = x;
    out[1] = z;
    return out;
}
static void Env_WorldUnitMoved(int, float) {}
static float* Env_UnitGetEntrance(SIUnit* unit, float* out)
{
    out[0] = DU_F(unit, 0x8c);
    out[1] = DU_F(unit, 0x90);
    out[2] = DU_F(unit, 0x94);
    return out;
}
static void Env_UnitOnDriverReachedTarget(SIUnit* unit) { unit->Slot_48(); }
static void Env_UnitSlot9C(SIUnit* unit, int) { unit->Slot_9C(); }
static void Env_UnitTowedFollow(SIUnit*, float, float, float, float, float, SIUnit*, float*,
                                float*, float*)
{
    DrvPanic("SUnit::GhostFrames_AddTop (unit +0x74) for a towed unit: not lifted (agent U)");
}
static void Env_UnitOnDriverStucked(SIUnit*)
{
    DrvPanic("SUnit::OnDriverStucked (0x5bcd20): not lifted (agent U)");
}
static bool Env_UnitIsPosInRange(SIUnit*, float, float, float)
{
    DrvPanic("SUnit 0x5b6fd0: not lifted (agent U)");
    return false;
}
static void* Env_UnitGetAimer(SIUnit*) { return nullptr; }
static float Env_SquadUnitMoveSpeed(SIUnit* squad) { return squad->GetMoveSpeed(-1); }
static void Env_SquadMemberRelativePos(SIUnit*, float* out, int)
{
    out[0] = out[1] = 0.0f;
}
static void Env_SquadMembersStep(SIUnit*, float) {}
static float Env_TerrainHeight(float x, float z)
{
    return g_World ? g_World->GetTerrainHeight(x, z) : 0.0f;
}
static float Env_WaterHeight(float x, float z)
{
    return g_World ? g_World->GetWaterHeight(x, z) : 0.0f;
}

SDriverEnv g_DriverEnv = {
    Env_GetFrame,
    Env_GetMovementGroupBoss,
    Env_GetMovementGroupSecond,
    Env_GetMovementGroupConvoy,
    Env_GetMovementGroupMoveSpeed,
    Env_GetMovementGroupUnitFormationPos,
    Env_GetMovementGroupFormationDir,
    Env_SetMovementGroupFormationDir,
    Env_GetMovementGroupDistance,
    Env_CanSeeGroundUnit,
    Env_FindEmptySpace,
    Env_FindEmptySpaceDir,
    Env_WorldUnitMoved,
    Env_UnitGetEntrance,
    Env_UnitOnDriverReachedTarget,
    Env_UnitSlot9C,
    Env_UnitTowedFollow,
    Env_UnitOnDriverStucked,
    Env_UnitIsPosInRange,
    Env_UnitGetAimer,
    Env_SquadUnitMoveSpeed,
    Env_SquadMemberRelativePos,
    Env_SquadMembersStep,
    Env_SquadMembersStep,
    Env_TerrainHeight,
    Env_WaterHeight,
};

} // namespace pz
