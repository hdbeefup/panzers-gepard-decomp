// src/game/driver.cpp
// pz::SDriver core, the menu driver classes and the SPDriver prototypes.
// OWNER: P. HD 0x54f4b0..0x5560a0, 0x559ab0, 0x55a640, 0x55bb80..0x55c160.
// The path finding is in driverpath.cpp, the ghost in driverghost.cpp.

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "driver.h"
#include "driverunit.h"
#include "drivermath.h"
#include "manoeuvre.h"
#include "target.h"
#include "iunit.h"
#include "iunitanim.h"
#include "world.h"
#include "worldapi.h"
#include "unitprops.h"
#include "pz/ipixie.h"
#include "stub_log.h"

namespace pz {

// An x87 "fadd qword" under the HD control word 0x007F (24-bit precision):
// the sum is rounded once to a 24-bit mantissa, then stored as a double.
static double X87AddPC24(double a, double b)
{
    unsigned short cw;
    unsigned short ncw = 0x007f;
    double r;
    __asm {
        fnstcw cw
        fldcw ncw
        fld a
        fadd b
        fstp r
        fldcw cw
    }
    return r;
}

// ---------------------------------------------------------------------------
// Construction

// PANZERS 0x54f4b0
SDriver::SDriver(SPDriver* pdriver, SIUnit* unit)
{
    MoveEffects.Init();
    WaterEffects.Init();
    LastGhostPos[0] = LastGhostPos[1] = LastGhostPos[2] = 0.0f;
    ArrivalRadius = 0.0f;
    GhostActive = true;
    LocalPathEnd = false;
    WaitForBoss = false;
    SkipFollow = false;
    MaxSpeedZero = false;
    GhostRestart = false;
    _05e[0] = _05e[1] = 0;
    ManoeuvreOpeningFrame = 0;
    ManoeuvreFrame = 0;
    GhostHold = false;
    WaitTicks = 0;
    Waiting = false;
    ArrivalActive = false;
    ArrivalDist2 = 0.0f;
    Arrival80 = 0.0f;
    ArrivalDir = 0.0f;
    LocalWayPoints.Init();
    GlobalWayPoints.Init();
    Goal[0] = Goal[1] = Goal[2] = 0.0f;
    TargetPos[0] = TargetPos[1] = TargetPos[2] = 0.0f;
    PDriver = pdriver;
    Unit = unit;
    WantedSpeed = 0.0f;
    WantedSpin = 0.0f;
    StoppingDistance = false;
    _02d[0] = _02d[1] = _02d[2] = 0;
    CollisionUnit = -1;
    LastCollisionUnit = -1;
    CollisionPos[0] = CollisionPos[1] = 0.0f;
    LocalPathFailures = 0;
    CollisionCount = 0;
    AvoidUnit = -1;
    HasGlobalPath = 0;
    HasLocalPath = 0;
    Target = nullptr;
    SavedTarget = nullptr;
    TurnTarget = nullptr;
    TurnRadius = 0.0f;
    TurnSpeedFactor = 1.0f;
    SpeedSteps = 1;
    SpinSteps = 1;
    MaxGhostFrames = 0x46;
    LastFollowFrame = -1;
    PDriver2 = nullptr;
    PZ_M2_TRACE("SDriver::SDriver (0x54f4b0)");
}

// PANZERS 0x54fdc0
SDriver::~SDriver()
{
    PZ_M2_TRACE("SDriver::~SDriver (0x54fdc0)");
    StopAllEffects();
    if (SavedTarget) {
        TargetAssign(Target, SavedTarget);
        TargetClear(SavedTarget);
    }
    TargetClear(Target);
    GlobalWayPoints.Free();
    for (int i = 0; i < LocalWayPoints.Size; ++i)                 // 0x54fd70
        Wpm_Free(&LocalWayPoints.Array[i]);
    LocalWayPoints.Free();
    WaterEffects.Free();
    MoveEffects.Free();
}

// PANZERS 0x551400 (factory) + vftable 0x7f4bf0
STurnInPlaceDriver::STurnInPlaceDriver(SPDriver* pd, SIUnit* unit) : SDriver(pd, unit)
{
    PDriver2 = pd;
    SpeedSteps = 10;
    SpinSteps = 5;
}
// PANZERS 0x550420
STurnInPlaceDriver::~STurnInPlaceDriver() {}
// PANZERS 0x559340
bool STurnInPlaceDriver::MoveTowardNextWayPoint(SGhostFrame* frame)
{
    return SDriver::MoveTowardNextWayPoint(frame);
}

// PANZERS 0x5513a0 (factory) + vftable 0x7f4c48
STurnInAngleDriver::STurnInAngleDriver(SPDriver* pd, SIUnit* unit) : SDriver(pd, unit)
{
    PDriver2 = pd;
    SpeedSteps = 10;
    SpinSteps = 5;
}
// PANZERS 0x5503f0
STurnInAngleDriver::~STurnInAngleDriver() {}
// PANZERS 0x559330
bool STurnInAngleDriver::MoveTowardNextWayPoint(SGhostFrame* frame)
{
    return SDriver::MoveTowardNextWayPoint(frame);
}

// PANZERS 0x551460 (factory) + vftable 0x7f4b40
SWalkerDriver::SWalkerDriver(SPDriver* pd, SIUnit* unit) : SDriver(pd, unit)
{
    PDriver2 = pd;
}
// PANZERS 0x550450
SWalkerDriver::~SWalkerDriver() {}
// PANZERS 0x559350
bool SWalkerDriver::MoveTowardNextWayPoint(SGhostFrame* frame)
{
    return SDriver::MoveTowardNextWayPoint(frame);
}

// PANZERS 0x5511a0 (factory) + vftable 0x7f4cf8
SPanzersSquadDriver::SPanzersSquadDriver(SPDriver* pd, SIUnit* unit) : SDriver(pd, unit)
{
    PDriver2 = pd;
}
// PANZERS 0x5502d0
SPanzersSquadDriver::~SPanzersSquadDriver() {}

// PANZERS 0x54f970
SPanzersSquadMemberDriver::SPanzersSquadMemberDriver(SPDriver* pd, SIUnit* unit) : SDriver(pd, unit)
{
    PDriver3 = pd;
    PDriver2 = nullptr;
    int r1 = (int)DU_U(g_World, 0x7518) * 0x343fd + 0x269ec3;
    DU_I(g_World, 0x7518) = r1;
    int r2 = r1 * 0x343fd + 0x269ec3;
    DU_I(g_World, 0x7518) = r2;
    Wobble = 0.0f;
    WobbleSpeed = 0.0f;
    // A random turn-speed factor in about [0.68, 1.37] per squad member.
    double a = (double)((float)(int)((double)((r2 >> 0x10) & 0x7fff) * 3.0517578125e-05 * 5.0)
                        / 100.0f) - 0.02;                       // 0x7f5a40 0x7ee558 0x7f59e8
    double b = (double)((float)(int)((double)((r1 >> 0x10) & 0x7fff) * 3.0517578125e-05 * 7.0)
                        / 10.0f) + 0.7;                         // 0x7f5a48 0x7f5a7c 0x7f5a28
    TurnSpeedFactor = (float)(a + b);
}
// PANZERS 0x550300
SPanzersSquadMemberDriver::~SPanzersSquadMemberDriver() {}

// ---------------------------------------------------------------------------
// Small slots

// PANZERS 0x5531d0
SIPDriver* SDriver::GetPDriver()
{
    return PDriver;
}

// PANZERS 0x555a50: one -1 effect handle per Move / Move-in-water effect.
void SDriver::Init()
{
    PZ_M2_TRACE("SDriver::Init (0x555a50)");
    for (int i = 0; i < PDriver->MoveEffects.Size; ++i) {
        int k = MoveEffects.Add();
        MoveEffects.Array[k] = -1;
    }
    for (int i = 0; i < PDriver->WaterEffects.Size; ++i) {
        int k = WaterEffects.Add();
        WaterEffects.Array[k] = -1;
    }
}

int SDriver::RandomInt(int range)
{
    return HdRandInt(range);
}

// PANZERS 0x557bf0
void SDriver::SetTarget(STarget* t)
{
    PZ_M2_TRACE("SDriver::SetTarget (0x557bf0)");
    if (!t->Refresh(DU_I(Unit, 0x74)))
        return;
    if (SavedTarget) {
        TargetAssign(Target, SavedTarget);
        TargetClear(SavedTarget);
    }
    TargetAssign(Target, t);
    DU_B(Unit, 0xcd) = 1;
    ResetGhost();
    LocalPathFailures = 0;
    CollisionCount = 0;
    AvoidUnit = -1;
    LastCollisionUnit = -1;
    SHdDArray<SVec2>* preset = (SHdDArray<SVec2>*)((char*)Unit + 0x308);
    if (Target->Kind != 1 && preset->Size > 0) {
        GlobalWayPoints.Clear(preset->Size);                    // 0x54ff30
        for (int i = 0; i < GlobalWayPoints.Size; ++i)
            GlobalWayPoints.Array[i] = preset->Array[i];
        preset->Clear(0);
        return;
    }
    GlobalWayPoints.Clear(0);
}

// PANZERS 0x557da0
void SPanzersSquadMemberDriver::SetTarget(STarget* t)
{
    PZ_M2_TRACE("SPanzersSquadMemberDriver::SetTarget (0x557da0)");
    if (!t->Refresh(DU_I(Unit, 0x74)))
        return;
    if (SavedTarget) {
        TargetAssign(Target, SavedTarget);
        TargetClear(SavedTarget);
    }
    TargetAssign(Target, t);
    DU_B(Unit, 0xcd) = 1;
}

// PANZERS 0x55c020 (not executed in the menu): Stop(true), SetTarget, then
// a stopped ghost that keeps the global path.
void SDriver::SetTargetStopped(STarget* t)
{
    PZ_M2_TRACE("SDriver::SetTargetStopped (0x55c020)");
    Stop(true);
    if (!t->Refresh(DU_I(Unit, 0x74)))
        return;
    if (SavedTarget) {
        TargetAssign(Target, SavedTarget);
        TargetClear(SavedTarget);
    }
    TargetAssign(Target, t);
    ResetGhost();
    LocalPathFailures = 0;
    CollisionCount = 0;
    AvoidUnit = -1;
    LastCollisionUnit = -1;
    StartStoppedGhost(true);
}

void SDriver::Slot_50()
{
    // HD 0x5531e0 returns the class reflection descriptor (debug only).
    STUB_LOG("SDriver::Slot_50 (0x5531e0)");
    PZ_M2_TRACE("SDriver::Slot_50 (0x5531e0)");
}

void SPanzersSquadMemberDriver::Slot_50()
{
    STUB_LOG("SPanzersSquadMemberDriver::Slot_50 (0x553220)");
    PZ_M2_TRACE("SPanzersSquadMemberDriver::Slot_50 (0x553220)");
}

// PANZERS 0x553260
float SDriver::GetMaxSpeed()
{
    PZ_M2_TRACE("SDriver::GetMaxSpeed (0x553260)");
    float s = Unit->GetMoveSpeed(-1);
    SIUnit* u = Unit;
    if (DU_I(u, 0x254) > -1) {
        float g = g_DriverEnv.GetMovementGroupMoveSpeed(u);
        if (g > 0.0f && s > g)
            s = g;
    }
    STarget* t = (STarget*)DU_P(u, 0x1f4);
    if (t && t->Kind == 1)
        s = s * 1.2f;                                           // 0x7f1b5c
    return s;
}

// PANZERS 0x5532e0
float SPanzersSquadDriver::GetMaxSpeed()
{
    PZ_M2_TRACE("SPanzersSquadDriver::GetMaxSpeed (0x5532e0)");
    float s = g_DriverEnv.SquadUnitMoveSpeed(Unit);
    SIUnit* u = Unit;
    if (DU_I(u, 0x254) > -1) {
        float g = g_DriverEnv.GetMovementGroupMoveSpeed(u);
        if (g > 0.0f && s > g)
            s = g;
    }
    STarget* t = (STarget*)DU_P(u, 0x1f4);
    if (t && t->Kind == 1)
        s = s * 1.2f;
    return s;
}

// PANZERS 0x557a80: the group speed limit and the 1.2 bonus, for the unit
// or (useBoss) its boss unit (+0x78).
float SDriver::ClampSpeedToGroup(float speed, bool useBoss)
{
    SIUnit* u = Unit;
    if (useBoss) {
        u = HdUnit(DU_I(Unit, 0x78));
        if (!u)
            return speed;
    }
    if (DU_I(u, 0x254) > -1) {
        float g = g_DriverEnv.GetMovementGroupMoveSpeed(u);
        if (g > 0.0f && g < speed)
            speed = g;
    }
    STarget* t = (STarget*)DU_P(u, 0x1f4);
    if (t && t->Kind == 1)
        return speed * 1.2f;
    return speed;
}

// PANZERS 0x553360
float SPanzersSquadMemberDriver::GetMaxSpeed()
{
    PZ_M2_TRACE("SPanzersSquadMemberDriver::GetMaxSpeed (0x553360)");
    SIUnit* u = Unit;
    if (DU_B(u, 0xf0) != 0 && DU_I(u, 0xe0) < DU_I(u, 0xe4))
        return 0.0f;
    if (DU_I(u, 0xf8) == 1 && DU_B(u, 0xf1) != 0)
        return 0.0f;
    SIUnitAnimation* anim = (SIUnitAnimation*)DU_P(u, 0x14);
    float s = anim->GetStateMoveSpeed(DU_I(u, 0xe0));
    return ClampSpeedToGroup(s, true);
}

// PANZERS 0x5533d0
float SDriver::GetTurnSpeed()
{
    PZ_M2_TRACE("SDriver::GetTurnSpeed (0x5533d0)");
    SIUnitAnimation* anim = (SIUnitAnimation*)DU_P(Unit, 0x14);
    float ts = anim->GetStateTurnSpeed(DU_I(Unit, 0xe0));
    if (ts == 0.0f)
        ts = PDriver->SpinSpeed;
    return TurnSpeedFactor * ts;
}

// PANZERS 0x553420
float SPanzersSquadDriver::GetTurnSpeed()
{
    PZ_M2_TRACE("SPanzersSquadDriver::GetTurnSpeed (0x553420)");
    SGhostQueue* q = DU_Ghosts(Unit);
    if (q->Count == 0)
        return PDriver2->SpinSpeed;
    SWayPoint wp;
    GetWayPoint(&wp, GhostAt(Unit, q->Top));
    float v = GetMaxSpeed();
    if (wp.Type == kWpTurn)
        v = v * 6.0f;                                           // 0x7f5a70
    return v / TurnRadius;
}

// ---------------------------------------------------------------------------
// Ghost bookkeeping

// PANZERS 0x554770
void SDriver::ResetGhost()
{
    UnitGhostClearAll(Unit);
    ArrivalActive = false;
    ArrivalDist2 = 0.0f;
    Arrival80 = 0.0f;
    ArrivalDir = 0.0f;
    GhostActive = true;          // HD writes the dword +0x58 = 1 (clears +0x59..+0x5b)
    LocalPathEnd = false;
    WaitForBoss = false;
    SkipFollow = false;
    MaxSpeedZero = false;        // word +0x5c = 0
    GhostRestart = false;
    ManoeuvreOpeningFrame = 0;
    ManoeuvreFrame = 0;
    GhostHold = false;
    WaitTicks = 0;
    Waiting = false;
}

// PANZERS 0x55bf50
void SDriver::Stop(bool clearTarget)
{
    if (clearTarget) {
        if (SavedTarget) {
            TargetAssign(Target, SavedTarget);
            TargetClear(SavedTarget);
        }
        TargetClear(Target);
    }
    ResetGhost();
}

// PANZERS 0x550ec0: a ghost frame from the unit's current state.
void SDriver::FillGhostFrame(SGhostFrame* f)
{
    SIUnit* u = Unit;
    f->X = DU_F(u, 0x8c);
    f->Y = DU_F(u, 0x90);
    f->Z = DU_F(u, 0x94);
    f->Dir = DU_F(u, 0xb0);
    f->Speed = DU_F(u, 0xc8);
    if (DU_F(u, 0xc8) == 0.0f)
        DU_B(u, 0xcc) = 0;
    f->Reverse = DU_B(u, 0xcc) != 0;
    if (SpeedSteps == 1)
        f->SpeedStep = -1;
    else
        f->SpeedStep = (int)((f->Speed / PDriver->MaxSpeed) * (float)SpeedSteps);
    f->SpinSpeed = DU_F(u, 0xd0);
    float r;
    if (PDriver->Type == 1 || PDriver->Type == 3)
        r = DU_F(u, 0xd4) / PDriver->WheelTurnAngle;
    else
        r = DU_F(u, 0xd0) / PDriver->SpinSpeed;
    f->SpinStep = (int)(r * (float)SpinSteps);
    f->Field6c = DU_F(u, 0x24c);
    for (int i = 0; i < 5; ++i) {
        f->Gear[i][0] = DU_F(u, 0x210 + i * 8);
        f->Gear[i][1] = DU_F(u, 0x214 + i * 8);
        f->Gear2[i] = DU_F(u, 0x238 + i * 4);
    }
    f->Field70 = DU_ClassType(u) == 10 ? DU_F(u, 0x300) : 0.0f;
}

// PANZERS 0x5534d0 SDriver::GetWayPoint: the frame's waypoint (16 bytes:
// x, z, dir, type; HD does not copy the reverse byte); type 5 takes the
// target unit's position.
SWayPoint* SDriver::GetWayPoint(SWayPoint* out, const SGhostFrame* f)
{
    int i = f->WayPointIdx;
    if (i < 0 || i >= LocalWayPoints.Size) {
        DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SWayPointWithManoeuvres", i);
        memset(out, 0, sizeof(*out));
        out->Type = kWpFollowUnit + 1;
        return out;
    }
    SWayPointWithManoeuvres& w = LocalWayPoints.Array[i];
    int m = f->ManoeuvreIdx;
    if (m < 0 || m >= w.Manoeuvres.Size) {
        DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SManoeuvre", m);
        memset(out, 0, sizeof(*out));
        out->Type = kWpFollowUnit + 1;
        return out;
    }
    SManoeuvre& man = w.Manoeuvres.Array[m];
    int p = f->PointIdx;
    if (p < 0 || p >= man.Points.Size) {
        DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SWayPoint", p);
        memset(out, 0, sizeof(*out));
        out->Type = kWpFollowUnit + 1;
        return out;
    }
    const SWayPoint& src = man.Points.Array[p];
    out->X = src.X;
    out->Z = src.Z;
    out->Dir = src.Dir;
    out->Type = src.Type;
    if (out->Type == kWpFollowUnit) {
        if (!Target) {
            DrvPanic("SDriver::GetWayPoint(): Target is NULL");
            return out;
        }
        if (Target->Type != kTargetUnit) {
            DrvPanic("SDriver::GetWayPoint - Target->Type != TARGET_UNIT");
            return out;
        }
        SIUnit* tu = HdUnit(Target->Unit);
        if (tu) {
            out->X = DU_F(tu, 0x8c);
            out->Z = DU_F(tu, 0x94);
        }
    }
    return out;
}

// PANZERS 0x5507d0: when a unit target moved far enough (relative to its
// distance), drops the ghost and the global path.
bool SDriver::CheckTargetMoved()
{
    STarget* t = Target;
    if (!t || t->Type != kTargetUnit || t->Kind == 3)
        return false;
    int g = DU_I(Unit, 0x254);
    if (g > -1 && g_DriverEnv.GetMovementGroupBoss(g) != Unit)
        return false;
    if (Target->IsUnitOrder()) {
        SGhostQueue* q = DU_Ghosts(Unit);
        if (q->Count == 0)
            return false;
        SGhostFrame f = *GhostAt(Unit, q->Top);
        SWayPoint wp;
        GetWayPoint(&wp, &f);
        if (wp.Type == kWpFollowUnit)
            return false;
    }
    Target->Refresh(DU_I(Unit, 0x74));
    const float* p = Target->Pos;
    float dx = TargetPos[0] - p[0];
    float dy = TargetPos[1] - p[1];
    float dz = TargetPos[2] - p[2];
    float d2 = dx * dx + dy * dy + dz * dz;
    if (!(d2 > 6.25f))                                          // 0x7f5a74
        return false;
    float ex = TargetPos[0] - DU_F(Unit, 0x8c);
    float ey = TargetPos[1] - DU_F(Unit, 0x90);
    float ez = TargetPos[2] - DU_F(Unit, 0x94);
    float e2 = ex * ex + ey * ey + ez * ez;
    float thr;
    if (e2 > 156.25f)                                           // 0x7f5a8c
        thr = 7.5f;
    else if (e2 > 56.25f)                                       // 0x7f5a88
        thr = 5.0f;
    else
        thr = 2.5f;
    if (d2 > thr * thr) {
        ResetGhost();
        GlobalWayPoints.Clear(0);
        return true;
    }
    return false;
}

// PANZERS 0x5518c0 (not executed in the menu): a one-point local path at
// the unit, turning to its direction, with a fresh ghost frame.
void SDriver::StartStoppedGhost(bool keepGlobal)
{
    ResetGhost();
    SGhostFrame f;
    memset(&f, 0, sizeof(f));
    FillGhostFrame(&f);
    UnitGhostSetBottomAll(Unit, g_DriverEnv.GetFrame());
    UnitGhostPush(Unit, &f);
    for (int i = 0; i < LocalWayPoints.Size; ++i)
        Wpm_Free(&LocalWayPoints.Array[i]);
    LocalWayPoints.Clear(0);
    int k = LocalWayPoints.Add();
    SWayPointWithManoeuvres& w = LocalWayPoints.Array[k];
    w.X = DU_F(Unit, 0x8c);
    w.Z = DU_F(Unit, 0x94);
    if (!keepGlobal) {
        w.PointType = kLwpLocal;
    } else {
        GlobalWayPoints.Clear(0);
        int g = 0;                                              // 0x557180 Insert(0)
        GlobalWayPoints.Add();
        memmove(GlobalWayPoints.Array + 1, GlobalWayPoints.Array,
                (size_t)(GlobalWayPoints.Size - 1) * sizeof(SVec2));
        GlobalWayPoints.Array[g].X = DU_F(Unit, 0x8c);
        GlobalWayPoints.Array[g].Z = DU_F(Unit, 0x94);
        UpdateTurnRadius();
        w.PointType = kLwpLastStop;
    }
    Wpm_AddStop(&LocalWayPoints.Array[k], DU_B(Unit, 0xcc) != 0);
    HasGlobalPath = 1;
    HasLocalPath = 1;
    WantedSpeed = 0.0f;
    ArrivalActive = true;
    ArrivalDist2 = 0.0f;
    Arrival80 = 0.0f;
    if (DU_B(Unit, 0xcc) == 0) {
        ArrivalDir = DU_F(Unit, 0xb0);
        return;
    }
    double d = (double)DU_F(Unit, 0xb0) + kHdPi;
    if (d > kHdPi)
        ArrivalDir = (float)(d - kHdTwoPi);
    else
        ArrivalDir = (float)(kHdMinusPi > d ? d + kHdTwoPi : d);
}

// PANZERS 0x553620: runs the ghost ahead (up to +0xdc frames).
void SDriver::GhostStep()
{
    CheckTargetMoved();
    if (!GhostActive)
        return;
    if (DU_Ghosts(Unit)->Count == 0)
        Ghost_FirstStep();
    MaxSpeedZero = GetMaxSpeed() == 0.0f;
    while (GhostActive) {
        if (!(DU_Ghosts(Unit)->Count < MaxGhostFrames || ManoeuvreOpeningFrame > 0))
            break;
        if (WaitForBoss || MaxSpeedZero)
            break;
        Ghost_NextStep();
    }
    if (WaitForBoss || MaxSpeedZero)
        GhostHold = true;
    else if (Target && Target->IsUnitOrder() && !GhostRestart && !LocalPathEnd)
        GhostHold = true;
    else
        GhostHold = false;
    if (WaitForBoss)
        WaitForBoss = false;
}

// PANZERS 0x553700 SDriver::Ghost_FirstStep
void SDriver::Ghost_FirstStep()
{
    PZ_M2_TRACE("SDriver::Ghost_FirstStep (0x553700)");
    if (DU_Ghosts(Unit)->Count != 0) {
        DrvPanic("SDriver::Ghost_FirstStep() - Unit->GhostFrames.GetSize() != 0");
        return;
    }
    if (NeedsGlobalPath()) {
        SIUnit* boss = g_DriverEnv.GetMovementGroupBoss(DU_I(Unit, 0x254));
        if (boss && boss != Unit && DU_B(boss, 0xcd) != 0) {
            WaitForBoss = true;
            SkipFollow = true;
            DU_B(Unit, 0xcd) = 1;
            return;
        }
        int size = -1;
        if (boss && boss == Unit) {
            SIUnit* second = g_DriverEnv.GetMovementGroupSecond(DU_I(Unit, 0x254));
            if (second) {
                size = DU_I(second, 0x5c);
                if (DrvUnit_TestBlockMap(Unit, DU_F(Unit, 0x8c), DU_F(Unit, 0x94),
                                         DU_F(Unit, 0xb0), size, (short)DU_S(Unit, 0xd8)))
                    size = -1;
            }
        }
        if (!FindGlobalPath(size)) {
            GhostActive = false;
            return;
        }
    }
    SGhostFrame f;
    memset(&f, 0, sizeof(f));
    FillGhostFrame(&f);
    f.WayPointIdx = 0;
    for (int i = 0; i < LocalWayPoints.Size; ++i)                 // 0x550aa0
        Wpm_Free(&LocalWayPoints.Array[i]);
    LocalWayPoints.Clear(0);
    if (!FindLocalPath(&f)) {
        ++LocalPathFailures;
        if (LocalPathFailures < 4) {
            int r = HdRandInt(10);
            int wait = LocalPathFailures == 1 ? 0x28 : (LocalPathFailures == 2 ? 100 : 200);
            GhostRestart = true;
            WaitTicks = r + wait;
            GhostActive = false;
            return;
        }
        LocalPathFailures = 0;
        GhostActive = false;
        return;
    }
    if (!NextWayPoint(&f, true, true)) {
        GhostActive = false;
        return;
    }
    UnitGhostSetBottomAll(Unit, g_DriverEnv.GetFrame());
    UnitGhostPush(Unit, &f);
    SGhostQueue* q = DU_Ghosts(Unit);
    if (q->Top != q->Bottom)
        DrvPanic("SDriver::Ghost_FirstStep() - Unit->GhostFrames.GetTopIndex() != Unit->GhostFrames.GetBottomIndex()");
}

// ---------------------------------------------------------------------------
// Refresh (vtable +0x14)

// PANZERS 0x559ab0 SDriver::Refresh
void SDriver::Refresh()
{
    PZ_M2_TRACE("SDriver::Refresh (0x559ab0)");
    SIUnit* u = Unit;
    DU_B(u, 0xcd) = 0;
    if (!Target) {
        DU_I(u, 0xf8) = 1;
        DU_F(u, 0xc8) = 0.0f;
        DU_F(u, 0xd0) = 0.0f;
        SPDriver* pd = PDriver;
        if (pd->Type != 1 && pd->Type != 3)
            return;
        float v = DU_F(u, 0xd4);
        int k;
        if (v > 0.0f)
            k = (int)((v / pd->WheelTurnAngle) * (float)SpinSteps) - 1;
        else if (0.0f > v)
            k = (int)((v / pd->WheelTurnAngle) * (float)SpinSteps) + 1;
        else
            return;
        DU_F(u, 0xd4) = ((float)k / (float)SpinSteps) * pd->WheelTurnAngle;
        return;
    }
    bool member = DU_ClassType(u) == 11 && DU_I(u, 0x17c) == 1;
    STarget* t = Target;
    if (t->Mode == 2) {
        if (t->Kind == 3) {
            Stop(true);
            DU_F(Unit, 0xc8) = 0.0f;
            g_DriverEnv.UnitOnDriverStucked(Unit);
            return;
        }
        if (!SavedTarget && !member) {
            if (DU_F(u, 0xc8) == 0.0f && DU_Ghosts(u)->Count == 0) {
                // Step back from the target: a new target at range * (1.05..1.45).
                Target->AddRef();
                SavedTarget = Target;
                TargetClear(Target);
                Target = STarget::Create(0);
                Target->AddRef();
                SIUnit* su = Unit;
                STarget* s = SavedTarget;
                float dz = DU_F(su, 0x94) - s->Pos[2];
                float dx = DU_F(su, 0x8c) - s->Pos[0];
                double len = sqrt((double)(dz * dz + dx * dx));
                double inv = 1.0 / len;
                float nx = (float)((double)dx * inv);
                float nz = (float)((double)dz * inv);
                float range = su->GetMinRange(DU_I(su, 0x44));
                nx = nx * range;
                nz = nz * range;
                double q = X87AddPC24(DRandDouble(0.4), 1.05);  // 0x7f5a18, x87 fadd 0x7f5a30
                float p[2];
                p[0] = (float)((double)nx * q) + DU_F(Unit, 0x8c);
                p[1] = DU_F(Unit, 0x94) + (float)((double)nz * q);
                Target->SetGroundPos(p);
                Target->Reverse = true;
            } else {
                if (Waiting)
                    goto after_mode2;
                StartStoppedGhost(false);
            }
            Waiting = true;
        }
    }
after_mode2:
    if (SavedTarget) {
        if (SavedTarget->Mode == 1 || member) {
            TargetAssign(Target, SavedTarget);
            TargetClear(SavedTarget);
            Waiting = false;
        } else {
            goto check_target;
        }
    }
    if (member)
        goto stationary;
check_target:
    t = Target;
    if (!t || !(t->Mode == 1 || t->Type == kTargetTurn || t->Kind == 3))
        goto ghost;
stationary:
    if (DU_F(Unit, 0xc8) == 0.0f && DU_Ghosts(Unit)->Count == 0) {
        t = Target;
        if (t->Type == kTargetTurn) {
            float r = TurnToDir(t->Dir);
            if (r == 0.0f) {
                Stop(true);
                DU_F(Unit, 0xc8) = 0.0f;
                g_DriverEnv.UnitOnDriverReachedTarget(Unit);
                DrvUnit_SetMoveState(Unit);
                return;
            }
            DrvUnit_SetMoveState(Unit);
            return;
        }
        if (DU_ClassType(Unit) == 5) {
            TurnToPoint(t->Pos[0], t->Pos[1], t->Pos[2]);
            DrvUnit_SetMoveState(Unit);
            return;
        }
        if (member) {
            float r = TurnToPoint(t->Pos[0], t->Pos[1], t->Pos[2]);
            if (r == 0.0f && Target->Mode != 1) {
                Stop(true);
                DU_F(Unit, 0xc8) = 0.0f;
                g_DriverEnv.UnitOnDriverStucked(Unit);
                DrvUnit_SetMoveState(Unit);
                return;
            }
            DrvUnit_SetMoveState(Unit);
            return;
        }
        if (!(TurnTarget && TurnTarget == t)) {
            if (g_DriverEnv.UnitIsPosInRange(Unit, t->Pos[0], t->Pos[1], t->Pos[2])) {
                DrvUnit_SetMoveState(Unit);
                return;
            }
        }
        float a = DAtan2f((double)(Target->Pos[0] - DU_F(Unit, 0x8c)),
                                 (double)(Target->Pos[2] - DU_F(Unit, 0x94)));
        void* aimer = g_DriverEnv.UnitGetAimer(Unit);
        if (!aimer) {
            TurnToDir(a);
            DrvUnit_SetMoveState(Unit);
            return;
        }
        float d = (float)DWrapSub((double)a, (double)DU_F(aimer, 0x28));
        float r = TurnToDir(d);
        if (r == 0.0f) {
            if (TurnTarget) {
                TurnTarget->Release();
                TurnTarget = nullptr;
            }
            DrvUnit_SetMoveState(Unit);
            return;
        }
        if (TurnTarget)
            TurnTarget->Release();
        if (Target)
            Target->AddRef();
        TurnTarget = Target;
        DrvUnit_SetMoveState(Unit);
        return;
    }
    if (!Waiting) {
        StartStoppedGhost(false);
        Waiting = true;
    }
ghost:
    GhostStep();
    while (!SkipFollow) {
        FollowGhost();
        if (WaitTicks > 0 && DU_Ghosts(Unit)->Count <= 1) {
            --WaitTicks;
            DU_F(Unit, 0xc8) = 0.0f;
            DU_I(Unit, 0xf8) = 1;
            DU_F(Unit, 0xd0) = 0.0f;
            DU_F(Unit, 0xd4) = 0.0f;
            if (WaitTicks != 0)
                return;
            GhostActive = true;
            if (GhostRestart)
                UnitGhostClearAll(Unit);
        }
        if (DU_Ghosts(Unit)->Count != 0)
            return;
        if (!GhostRestart)
            goto ghost_empty;
        UnitGhostClearAll(Unit);
        ArrivalActive = false;
        ArrivalDist2 = 0.0f;
        Arrival80 = 0.0f;
        ArrivalDir = 0.0f;
        GhostActive = true;      // dword +0x58 = 1
        LocalPathEnd = false;
        WaitForBoss = false;
        SkipFollow = false;
        MaxSpeedZero = false;
        GhostRestart = false;
        ManoeuvreOpeningFrame = 0;
        ManoeuvreFrame = 0;
        GhostHold = false;
        WaitTicks = 0;
        Waiting = false;
        GhostStep();
    }
    SkipFollow = false;
    return;
ghost_empty:
    if (Waiting) {
        if (SavedTarget) {
            TargetAssign(Target, SavedTarget);
            TargetClear(SavedTarget);
        }
        DU_F(Unit, 0xc8) = 0.0f;
        DU_I(Unit, 0xf8) = 1;
        DU_F(Unit, 0xd0) = 0.0f;
        DU_F(Unit, 0xd4) = 0.0f;
        Stop(false);
        return;
    }
    if (!Target) {
        Stop(true);
        return;
    }
    if (LocalPathEnd && HasGlobalPath && HasLocalPath) {
        Stop(true);
        DU_F(Unit, 0xc8) = 0.0f;
        g_DriverEnv.UnitOnDriverReachedTarget(Unit);
        return;
    }
    Stop(true);
    DU_F(Unit, 0xc8) = 0.0f;
    g_DriverEnv.UnitOnDriverStucked(Unit);
}

// PANZERS 0x55a640: a squad member moves straight along its target each
// tick (no ghost queue).
void SPanzersSquadMemberDriver::Refresh()
{
    PZ_M2_TRACE("SPanzersSquadMemberDriver::Refresh (0x55a640)");
    if (!Target) {
        DU_I(Unit, 0xf8) = 1;
        DU_F(Unit, 0xc8) = 0.0f;
        return;
    }
    SGhostFrame f;
    memset(&f, 0, sizeof(f));
    SIUnit* u = Unit;
    f.Dir = DU_F(u, 0xb0);
    f.X = DU_F(u, 0x8c);
    f.Y = DU_F(u, 0x90);
    f.Z = DU_F(u, 0x94);
    bool moved = MoveTowardNextWayPoint(&f);
    u = Unit;
    DU_F(u, 0x8c) = f.X;
    DU_F(u, 0x90) = f.Y;
    DU_F(u, 0x94) = f.Z;
    DU_F(u, 0xb0) = f.Dir;
    DU_F(u, 0xc8) = f.Speed;
    DU_F(u, 0xd0) = f.SpinSpeed;
    DrvUnit_SetMoveState(Unit);
    if (!moved)
        Stop(true);
}

// ---------------------------------------------------------------------------
// Effects (dust, exhaust, wakes). The pixie slots +0x28/+0x30/+0x64 and the
// model slot +0x34 are not typed yet in pz/ipixie.h / pz/imodel.h, so the
// effects are not created; the state the logic reads (unit +0x338) and the
// handle bookkeeping follow HD.

// PANZERS 0x55bb80 (departure effects: one-shot pixie +0x28 per entry)
void SDriver::StartEffects()
{
    STUB_LOG("SDriver::StartEffects (0x55bb80) [pixie +0x28 not typed]");
}

// PANZERS 0x55bc50: move effects, or the water ones when the unit stands
// deeper than 1.0 (0x7f1b48) in water.
void SDriver::StartMoveEffects()
{
    float water = g_DriverEnv.WaterHeight(DU_F(Unit, 0x8c), DU_F(Unit, 0x94));
    float ground = g_DriverEnv.TerrainHeight(DU_F(Unit, 0x8c), DU_F(Unit, 0x94));
    if (ground < water - 0.01f) {                               // 0x7f1b48 (x87 PC24 = float)
        StartWaterEffects();
        return;
    }
    STUB_LOG("SDriver::StartMoveEffects (0x55bc50) [pixie +0x30/+0x64 not typed]");
    DU_I(Unit, 0x338) = 1;
}

// PANZERS 0x55be10
void SDriver::StartWaterEffects()
{
    STUB_LOG("SDriver::StartWaterEffects (0x55be10) [pixie +0x30/+0x64 not typed]");
    DU_I(Unit, 0x338) = 2;
}

// PANZERS 0x55c160
void SDriver::StopWaterEffects()
{
    for (int i = 0; i < WaterEffects.Size; ++i) {
        if (WaterEffects.Array[i] != -1) {
            if (g_Pixie)
                g_Pixie->StopEffect(WaterEffects.Array[i]);          // pixie +0x34
            WaterEffects.Array[i] = -1;
        }
    }
    DU_I(Unit, 0x338) = 0;
}

// PANZERS 0x55c0e0
void SDriver::StopAllEffects()
{
    StopWaterEffects();
    for (int i = 0; i < MoveEffects.Size; ++i) {
        if (MoveEffects.Array[i] != -1) {
            if (g_Pixie)
                g_Pixie->StopEffect(MoveEffects.Array[i]);
            MoveEffects.Array[i] = -1;
        }
    }
    DU_I(Unit, 0x338) = 0;
}

// ---------------------------------------------------------------------------
// SPDriver prototypes

static void FreeEffectDescs(SHdDArray<SPDriver::SEffectDesc>& a)  // 0x54fd00
{
    for (int i = 0; i < a.Size; ++i)
        free(a.Array[i].Node);
    a.Free();
}

static char* DupName(const char* s, int len)
{
    if (!s || len <= 0)
        return nullptr;
    char* d = (char*)malloc((size_t)len + 1);
    memcpy(d, s, (size_t)len);
    d[len] = 0;
    return d;
}

// PANZERS 0x54f760
SPDriver::SPDriver()
{
    Type = -1;
    MaxSpeed = 0.0f;
    SpinSpeed = 0.0f;
    WheelTurnAngle = 0.0f;
    Wheelbase = 0.0f;
    Name = nullptr;
    NameLen = 0;
    MoveEffects.Init();
    WaterEffects.Init();
    DepartureEffects.Init();
}

// PANZERS 0x550180
SPDriver::~SPDriver()
{
    FreeEffectDescs(MoveEffects);
    FreeEffectDescs(WaterEffects);
    FreeEffectDescs(DepartureEffects);
    free(Name);
    Name = nullptr;
}

// PANZERS 0x555cc0: SPDriver::Init stores the name (FUN_0052c320).
void SPDriver::Load(SProperties* props, const char* name, int nameLen)
{
    (void)props;
    free(Name);
    Name = DupName(name, nameLen);
    NameLen = Name ? nameLen : 0;
}

// PANZERS 0x557520: "Departure_Effects" (+0x38), "Move_Effects" (+0x20),
// "Move_In_Water_Effects" (+0x2c): {"Effect" prototype, "MeshName"}.
void SPDriver::LoadSubProperties(SProperties* propsp)
{
    PZ_M2_TRACE("SPDriver::LoadSubProperties (0x557520)");
    SUPropStruct* props = (SUPropStruct*)propsp;
    if (!props)
        return;
    struct { const char* key; SHdDArray<SEffectDesc>* arr; } lists[3] = {
        { "Departure_Effects", &DepartureEffects },
        { "Move_Effects", &MoveEffects },
        { "Move_In_Water_Effects", &WaterEffects },
    };
    for (int l = 0; l < 3; ++l) {
        int n = props->GetArraySize(lists[l].key);
        for (int i = 0; i < n; ++i) {
            SUPropStruct* item = props->GetArrayItem(lists[l].key, i);
            int k = lists[l].arr->Add();
            SEffectDesc& d = lists[l].arr->Array[k];
            const char* fx = item ? item->GetString("Effect") : nullptr;
            d.Proto = (g_Pixie && fx) ? g_Pixie->LoadEffectPrototype(fx, false, false, 0, 0) : -1;
            const char* mesh = item ? item->GetString("MeshName") : nullptr;
            free(d.Node);
            d.Node = mesh ? DupName(mesh, (int)strlen(mesh)) : nullptr;
            d.NodeLen = d.Node ? (int)strlen(d.Node) : 0;
        }
    }
}

// PANZERS 0x55c820
void SPDriver::Slot_0C()
{
    // HD 0x55c820 releases the effect prototypes (pixie +0x20) and clears
    // the three arrays (resource unload).
    for (int i = 0; i < MoveEffects.Size; ++i)
        if (g_Pixie && MoveEffects.Array[i].Proto >= 0)
            g_Pixie->ReleaseEffectPrototype(MoveEffects.Array[i].Proto);
    for (int i = 0; i < WaterEffects.Size; ++i)
        if (g_Pixie && WaterEffects.Array[i].Proto >= 0)
            g_Pixie->ReleaseEffectPrototype(WaterEffects.Array[i].Proto);
    for (int i = 0; i < DepartureEffects.Size; ++i)
        if (g_Pixie && DepartureEffects.Array[i].Proto >= 0)
            g_Pixie->ReleaseEffectPrototype(DepartureEffects.Array[i].Proto);
    FreeEffectDescs(MoveEffects);
    FreeEffectDescs(WaterEffects);
    FreeEffectDescs(DepartureEffects);
}

SIDriver* SPDriver::CreateDriver(SIUnit* unit)
{
    STUB_LOG("SPDriver::CreateDriver (driver type not used in the menu)");
    (void)unit;
    return nullptr;
}

// The "DriverType" sub struct of the .unit "Driver" entry.
static SUPropStruct* DriverSub(SProperties* propsp, int type, const char* who)
{
    SUPropStruct* props = (SUPropStruct*)propsp;
    if (props->GetMultiIndex("DriverType") != type)
        DrvPanic("%s::Init - bad DriverType", who);
    return props->GetMultiSubStruct("DriverType");
}

static float HdMoveSpeed(float v)
{
    return (((v / 3.6f) * 100.0f) / 20.0f) * 0.005f;           // 0x7f5a68 0x7ee558 0x7f35d8 0x7f5994
}

// PANZERS 0x556800
void SPTurnInPlaceDriver::Load(SProperties* props, const char* name, int nameLen)
{
    Type = 0;
    SUPropStruct* s = DriverSub(props, Type, "SPTurnInPlaceDriver");
    MaxSpeed = HdMoveSpeed(s->GetFloat("MoveSpeed"));
    SpinSpeed = (s->GetFloat("SpinSpeed") * 0.05f) * 0.017453292f;  // 0x7f4534 0x7f59a0 (x87, PC24)
    MaxFuel = (int)s->GetFloat("MaxFuel");
    Consume = (float)(100.0 / (double)s->GetFloat("Consume"));
    SPDriver::Load(props, name, nameLen);
}

// PANZERS 0x556680
void SPTurnInAngleDriver::Load(SProperties* props, const char* name, int nameLen)
{
    Type = 1;
    SUPropStruct* s = DriverSub(props, Type, "SPTurnInAngleDriver");
    MaxSpeed = HdMoveSpeed(s->GetFloat("MoveSpeed"));
    WheelTurnAngle = s->GetFloat("WheelTurnAngle") * 0.017453292f;
    Wheelbase = (s->GetFloat("Wheelbase") * 100.0f) * 0.005f;
    MaxFuel = (int)s->GetFloat("MaxFuel");
    Consume = (float)(100.0 / (double)s->GetFloat("Consume"));
    SPDriver::Load(props, name, nameLen);
}

// PANZERS 0x556970
void SPWalkerDriver::Load(SProperties* props, const char* name, int nameLen)
{
    Type = 5;
    SUPropStruct* s = DriverSub(props, Type, "SPWalkerDriver");
    MaxSpeed = HdMoveSpeed(s->GetFloat("MoveSpeed"));
    SpinSpeed = (s->GetFloat("SpinSpeed") * 0.05f) * 0.017453292f;
    SPDriver::Load(props, name, nameLen);
}

// PANZERS 0x555fc0
void SPPanzersSquadDriver::Load(SProperties* props, const char* name, int nameLen)
{
    Type = 10;
    DriverSub(props, Type, "SPPanzersSquadDriver");
    SpinSpeed = 50.0f;                                          // 0x42480000
    SPDriver::Load(props, name, nameLen);
}

// PANZERS 0x5560a0
void SPPanzersSquadMemberDriver::Load(SProperties* props, const char* name, int nameLen)
{
    Type = 11;
    DriverSub(props, Type, "SPPanzersSquadMemberDriver");
    SPDriver::Load(props, name, nameLen);
}

// PANZERS 0x551400
SIDriver* SPTurnInPlaceDriver::CreateDriver(SIUnit* unit) { return new STurnInPlaceDriver(this, unit); }
// PANZERS 0x5513a0
SIDriver* SPTurnInAngleDriver::CreateDriver(SIUnit* unit) { return new STurnInAngleDriver(this, unit); }
// PANZERS 0x551460
SIDriver* SPWalkerDriver::CreateDriver(SIUnit* unit) { return new SWalkerDriver(this, unit); }
// PANZERS 0x5511a0
SIDriver* SPPanzersSquadDriver::CreateDriver(SIUnit* unit) { return new SPanzersSquadDriver(this, unit); }
// PANZERS 0x5511f0
SIDriver* SPPanzersSquadMemberDriver::CreateDriver(SIUnit* unit)
{
    return new SPanzersSquadMemberDriver(this, unit);
}

// PANZERS 0x5a6f10 (the DriverType switch of SPUnit::InitDrivers)
SIPDriver* CreatePDriver(int driverType)
{
    switch (driverType) {
    case 0: return new SPTurnInPlaceDriver();
    case 1: return new SPTurnInAngleDriver();
    case 5: return new SPWalkerDriver();
    case 10: return new SPPanzersSquadDriver();
    case 11: return new SPPanzersSquadMemberDriver();
    case 3: case 6: case 7: case 8: case 9: case 12: case 13: {
        // SPFlying/Projectile/Squad/SquadMember/ChildUnit/Parachute/Train:
        // not used by the menu.
        SPDriver* p = new SPDriver();
        p->Type = driverType;
        return p;
    }
    default:
        DrvWarn("SPUnit::InitDrivers - Unknown drivertype");
        return nullptr;
    }
}

} // namespace pz
