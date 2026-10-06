// src/game/driverghost.cpp
// The SDriver ghost: the frames that run ahead of the unit, their steps,
// turns and speeds, collisions with nearby units, arrival tests, and the
// per-tick move of the unit onto the oldest frame. OWNER: P.
// HD 0x553700..0x55c3e0 (see driver.h for the full list).

#include <math.h>
#include <string.h>
#include "driver.h"
#include "driverunit.h"
#include "drivermath.h"
#include "manoeuvre.h"
#include "target.h"
#include "iunit.h"
#include "world.h"
#include "worldapi.h"
#include "blockmap.h"
#include "stub_log.h"

namespace pz {

// (SSE2 intrinsics: a plain "return d - kHdTwoPi" can become an x87 fsub.)
static __forceinline double WrapPi(double d)
{
    __m128d v = _mm_set_sd(d);
    if (d > kHdPi)
        return _mm_cvtsd_f64(_mm_sub_sd(v, _mm_set_sd(kHdTwoPi)));
    if (kHdMinusPi > d)
        return _mm_cvtsd_f64(_mm_add_sd(v, _mm_set_sd(kHdTwoPi)));
    return d;
}

// |a - b| folded into [0, pi] (the inline idiom; SSE double).
static __forceinline double AbsDiff(double a, double b)
{
    double d = _mm_cvtsd_f64(_mm_sub_sd(_mm_set_sd(a), _mm_set_sd(b)));
    d = fabs(d);
    if (d > kHdPi)
        return _mm_cvtsd_f64(_mm_sub_sd(_mm_set_sd(kHdTwoPi), _mm_set_sd(d)));
    return d;
}

static __forceinline float RevDir(float dir)
{
    return (float)DWrapAdd((double)dir, kHdPi);
}

// ---------------------------------------------------------------------------
// Steps (vtable +0x24/+0x28/+0x2c; HD ignores "this")

// PANZERS 0x557b30
bool SDriver::GhostStepStraight(SGhostFrame* f, float x, float z)
{
    (void)x;
    (void)z;
    int s = (f->Reverse == 0) ? 1 : -1;
    float nx = (float)(DSin((double)f->Dir) * (double)f->Speed * (double)s + (double)f->X);
    f->X = nx;
    float nz = (float)(DCos((double)f->Dir) * (double)f->Speed * (double)s + (double)f->Z);
    f->Z = nz;
    f->Y = g_DriverEnv.TerrainHeight(nx, nz);
    return true;
}

// PANZERS 0x559360
bool SDriver::GhostStepTowards(SGhostFrame* f, float x, float z)
{
    float dx = x - f->X;
    float dz = z - f->Z;
    float d2 = dz * dz + dx * dx;
    if (d2 > f->Speed * f->Speed) {
        int s = (f->Reverse == 0) ? 1 : -1;
        f->X = (float)(DSin((double)f->Dir) * (double)f->Speed * (double)s + (double)f->X);
        f->Z = (float)(DCos((double)f->Dir) * (double)f->Speed * (double)s + (double)f->Z);
    } else {
        f->X = x;
        f->Z = z;
        f->Speed = (float)sqrt((double)d2);
    }
    f->Y = g_DriverEnv.TerrainHeight(f->X, f->Z);
    return d2 == 0.0f;
}

// PANZERS 0x55c230
bool SDriver::GhostTurnTowards(SGhostFrame* f, float x, float z)
{
    float dx = x - f->X;
    float dz = z - f->Z;
    float a = DAtan2f((double)dx, (double)dz);
    if (1e-4 > fabs((double)dx) && 1e-4 > fabs((double)dz))
        a = f->Dir;
    float d2 = dz * dz + dx * dx;
    if (d2 > f->Speed * f->Speed) {
        int s = (f->Reverse == 0) ? 1 : -1;
        f->X = (float)(DSin((double)a) * (double)f->Speed * (double)s + (double)f->X);
        f->Z = (float)(DCos((double)a) * (double)f->Speed * (double)s + (double)f->Z);
    } else {
        f->X = x;
        f->Z = z;
        f->Speed = (float)sqrt((double)d2);
    }
    f->Y = g_DriverEnv.TerrainHeight(f->X, f->Z);
    return d2 == 0.0f;
}

// ---------------------------------------------------------------------------
// Speed and spin

// PANZERS 0x55b2d0
void SDriver::SetUnitSpeed(SGhostFrame* f)
{
    float m = GetMaxSpeed();
    if (SpeedSteps == 1) {
        float ws = WantedSpeed;
        if (ws == -1.0f) {
            f->Speed = m;
            return;
        }
        if (ws == 0.0f) {
            f->Speed = 0.0f;
            return;
        }
        f->Speed = (m > ws) ? ws : m;
        return;
    }
    float ws = WantedSpeed;
    float maxs = PDriver->MaxSpeed;
    if ((ws == -1.0f && m >= f->Speed) || ws > f->Speed) {
        ++f->SpeedStep;
        if (((float)f->SpeedStep / (float)SpeedSteps) * maxs > m) {
            --f->SpeedStep;
            if (f->SpeedStep <= 0)
                f->SpeedStep = 0;
        }
    } else {
        --f->SpeedStep;
        if (f->SpeedStep <= 0)
            f->SpeedStep = 0;
    }
    f->Speed = ((float)f->SpeedStep / (float)SpeedSteps) * maxs;
}

// PANZERS 0x55b3e0 SDriver::SetUnitSpinSpeed
void SDriver::SetUnitSpinSpeed(SGhostFrame* f)
{
    SPDriver* pd = PDriver;
    int type = pd->Type;
    if (type == 1 || type == 3) {
        float ws = WantedSpin;
        if (ws == 99999.0f || ws > f->SpinSpeed) {
            ++f->SpinStep;
            if (f->SpinStep > SpinSteps)
                f->SpinStep = f->SpinStep - 1;
        } else if (ws == -99999.0f || f->SpinSpeed > ws) {
            --f->SpinStep;
            if (f->SpinStep < -SpinSteps)
                f->SpinStep = f->SpinStep + 1;
        }
        float k = (float)f->SpinStep;
        float sp = f->Speed;
        f->SpinSpeed = (((k / (float)SpinSteps) * sp) * pd->WheelTurnAngle) / pd->Wheelbase;
        if (sp == 0.0f)
            f->SpinSpeed = (((k / (float)SpinSteps) * pd->MaxSpeed) * pd->WheelTurnAngle) / pd->Wheelbase;
        return;
    }
    if (type != 0) {
        DrvPanic("SDriver::SetUnitSpinSpeed - bad DriverType");
        return;
    }
    float ts = GetTurnSpeed();
    float ws = WantedSpin;
    if (ws == 99999.0f || ws > f->SpinSpeed) {
        ++f->SpinStep;
        if (((float)f->SpinStep / (float)SpinSteps) * pd->SpinSpeed > ts)
            f->SpinStep = f->SpinStep - 1;
    } else if (ws == -99999.0f || f->SpinSpeed > ws) {
        --f->SpinStep;
        if (-ts > ((float)f->SpinStep / (float)SpinSteps) * pd->SpinSpeed)
            f->SpinStep = f->SpinStep + 1;
    }
    f->SpinSpeed = ((float)f->SpinStep / (float)SpinSteps) * pd->SpinSpeed;
}

// PANZERS 0x55b5f0: turn towards dir with stepped spin; returns the angle
// left before the step.
float SDriver::TurnTowardsInSteps(SGhostFrame* f, float dir)
{
    if (f->Reverse)
        f->Dir = (float)WrapPi((double)f->Dir + kHdPi);
    double d = (double)dir - (double)f->Dir;
    float left = (float)AbsDiff((double)dir, (double)f->Dir);
    if (1e-4 > (double)left) {
        WantedSpin = 0.0f;
    } else {
        double dw = WrapPi(d);
        double s = 0.0;
        if (0.0 > dw)
            s = -1.0;
        else if (dw > 0.0)
            s = 1.0;
        WantedSpin = ((int)s > 0) ? 99999.0f : -99999.0f;
    }
    int step = f->SpinStep;
    if (step != 0) {
        float a = (float)fabs((double)f->SpinSpeed);
        float n = (float)(step < 0 ? -step : step);
        float v = ((a / n + a) * 0.5f) * n - a * 0.5f;
        if (v >= left && IsSpinStopNecessary(f))
            WantedSpin = 0.0f;
    }
    SetUnitSpinSpeed(f);
    float fd = f->Dir;
    float ad = (float)AbsDiff((double)dir, (double)fd);
    SPDriver* pd = PDriver;
    float r;
    if (pd->Type == 1 || pd->Type == 3) {
        float inv = 1.0f / (float)SpinSteps;
        r = ((f->Speed * inv) * pd->WheelTurnAngle) / pd->Wheelbase;
        if (r == 0.0f)
            r = ((pd->MaxSpeed * inv) * pd->WheelTurnAngle) / pd->Wheelbase;
    } else {
        r = (1.0f / (float)SpinSteps) * pd->SpinSpeed;
    }
    if ((double)ad > fabs((double)r))
        f->Dir = (float)WrapPi((double)f->SpinSpeed + (double)fd);
    else
        f->Dir = dir;
    if (f->Reverse)
        f->Dir = (float)WrapPi((double)f->Dir - kHdPi);
    return ad;
}

// PANZERS 0x55b9c0: turn towards dir at the full turn speed.
float SDriver::TurnTowardsDirect(SGhostFrame* f, float dir)
{
    if (f->Reverse)
        f->Dir = (float)WrapPi((double)f->Dir + kHdPi);
    double dw = WrapPi((double)dir - (double)f->Dir);
    double s = 0.0;
    if (0.0 > dw)
        s = -1.0;
    else if (dw > 0.0)
        s = 1.0;
    float spin = (float)((double)GetTurnSpeed() * s);
    f->SpinSpeed = spin;
    float fd = f->Dir;
    float ad = (float)AbsDiff((double)dir, (double)fd);
    if ((double)ad > fabs((double)spin))
        f->Dir = (float)WrapPi((double)spin + (double)fd);
    else
        f->Dir = dir;
    if (f->Reverse)
        f->Dir = (float)WrapPi((double)f->Dir - kHdPi);
    if (1e-4 > (double)ad) {
        f->SpinSpeed = 0.0f;
        f->SpinStep = 0;
    }
    return ad;
}

// PANZERS 0x55b8d0
float SDriver::TurnTowardsDir(SGhostFrame* f, float dir)
{
    if (SpinSteps == 1)
        return TurnTowardsDirect(f, dir);
    return TurnTowardsInSteps(f, dir);
}

// PANZERS 0x55b900
void SDriver::TurnTowardsPoint(SGhostFrame* f, float x, float z)
{
    float dx = x - f->X;
    float dz = z - f->Z;
    float a = DAtan2f((double)dx, (double)dz);
    float d = (1e-4 > fabs((double)dx) && 1e-4 > fabs((double)dz)) ? f->Dir : a;
    if (SpinSteps == 1)
        TurnTowardsDirect(f, d);
    else
        TurnTowardsInSteps(f, d);
}

// PANZERS 0x557260
bool SDriver::IsSpinStopNecessary(const SGhostFrame* f)
{
    if (DU_Ghosts(Unit)->Count == 0)
        return true;
    SWayPoint wp;
    GetWayPoint(&wp, f);
    if (wp.Type == 1 || wp.Type == 3 || wp.Type == 4 || wp.Type == 5)
        return true;
    if (wp.Type != 2) {
        DrvPanic("SDriver::IsSpinStopNecessary - Unknown waypoint type (%d)", wp.Type);
        return true;
    }
    int i = f->WayPointIdx;
    SManoeuvre& m = LocalWayPoints.At(i).Manoeuvres.At(f->ManoeuvreIdx);
    int p = f->PointIdx + 1;
    if (p >= m.Points.Size)
        return i + 1 >= LocalWayPoints.Size;
    SWayPoint& np = m.Points.At(p);
    if (np.Type == 2)
        return false;
    float a = (float)DAtan2((double)(np.X - f->X), (double)(np.Z - f->Z));
    double d = DAngleDist((double)a, (double)wp.Dir);
    return 0.1745329350233078 > d;                              // 0x7f59f8
}

// PANZERS 0x557400
bool SDriver::IsTheGhostInStoppingDistance(const SGhostFrame* f)
{
    SWayPoint wp;
    GetWayPoint(&wp, f);
    if (wp.Type == 1 || wp.Type == 3 || wp.Type == 4 || wp.Type == 5) {
        float dx = wp.X - f->X;
        float dz = wp.Z - f->Z;
        float d2 = dx * dx + dz * dz;
        if (1e-4 > (double)d2)
            return true;
        float sp = f->Speed;
        if (sp == 0.0f)
            return false;
        if (SpeedSteps == 1) {
            float h = sp * 0.5f;
            return h * h >= d2;
        }
        float v = (((PDriver->MaxSpeed / (float)SpeedSteps + sp) * 0.5f) * (float)f->SpeedStep)
            - sp * 0.5f;
        return v * v >= d2;
    }
    if (wp.Type == 2)
        return true;
    DrvPanic("SDriver::IsTheGhostInStoppingDistance: Unknown waypoint type (%d)", wp.Type);
    return true;
}

// PANZERS 0x5594a0: may the ghost drop the current manoeuvre (stop on the
// way) at this frame?
bool SDriver::CanCancelManoeuvre(const SGhostFrame* f)
{
    int i = f->WayPointIdx;
    SWayPointWithManoeuvres& w = LocalWayPoints.At(i);
    if (w.PointType == kLwpLocal)
        return true;
    SManoeuvre& m = w.Manoeuvres.At(f->ManoeuvreIdx);
    int p = f->PointIdx;
    bool rev = m.Points.At(p).Reverse;
    int np = p + 1;
    if (np >= m.Points.Size) {
        if (LocalWayPoints.At(i).PointType == kLwpLastPass)
            return false;
        if (LocalWayPoints.At(f->WayPointIdx).PointType == kLwpLastStop) {
            STarget* t = Target;
            if (t && t->HasPathAhead() && !t->IsKind14())
                return false;
            return true;
        }
        if (LocalWayPoints.At(f->WayPointIdx).Manoeuvres.At(f->ManoeuvreIdx).Type != 0xd)
            return false;
        if (LocalWayPoints.At(i + 1).PointType != kLwpLastStop)
            return false;
        STarget* t = Target;
        if (t && t->HasPathAhead())
            return false;
        return true;
    }
    if (m.Points.At(f->PointIdx).Type == 2)
        return true;
    if (m.Points.At(np).Type == 2)
        return true;
    return rev != m.Points.At(np).Reverse;
}

// PANZERS 0x55b0b0 SDriver::SetNextWP
bool SDriver::SetNextWP(SGhostFrame* f, bool keep, bool create)
{
    if (!keep) {
        if (f->WayPointIdx < 0) {
            DrvPanic("SDriver::SetNextWP - frame.WayPointIdx < 0");
            return false;
        }
        if (f->ManoeuvreIdx < 0) {
            DrvPanic("SDriver::SetNextWP - 1. frame.ManoeuvreIdx < 0");
            return false;
        }
        if (LocalWayPoints.At(f->WayPointIdx).Manoeuvres.At(f->ManoeuvreIdx).Type == 0xd)
            ++f->WayPointIdx;
        ++f->WayPointIdx;
    }
    if (f->WayPointIdx >= LocalWayPoints.Size)
        return false;
    if (create) {
        SWayPointWithManoeuvres* w = &LocalWayPoints.At(f->WayPointIdx);
        Wpm_CreateManoeuvres(w, PDriver->Type, Target, TurnRadius, f);
        Wpm_FilterByStart(&LocalWayPoints.At(f->WayPointIdx), f->X, f->Z);
        f->ManoeuvreIdx = Wpm_PopBest(&LocalWayPoints.At(f->WayPointIdx));
        if (f->ManoeuvreIdx < 0) {
            DrvPanic("SDriver::SetNextWP - 2. frame.ManoeuvreIdx < 0");
            return false;
        }
    }
    f->PointIdx = 0;
    return true;
}

// PANZERS 0x55af20: the frame's next waypoint point (and manoeuvres);
// resets the wanted speed and the stopping-distance flag.
bool SDriver::NextWayPoint(SGhostFrame* f, bool setNext, bool create)
{
    if (setNext) {
        if (!SetNextWP(f, setNext, create))
            return false;
    } else {
        ++f->PointIdx;
    }
    SManoeuvre& m = LocalWayPoints.At(f->WayPointIdx).Manoeuvres.At(f->ManoeuvreIdx);
    if (f->PointIdx >= m.Points.Size) {
        if (!SetNextWP(f, false, true))
            return false;
    }
    WantedSpeed = -1.0f;
    StoppingDistance = false;
    bool old = f->Reverse;
    SWayPoint& p = LocalWayPoints.At(f->WayPointIdx).Manoeuvres.At(f->ManoeuvreIdx)
                       .Points.At(f->PointIdx);
    f->Reverse = p.Reverse;
    if (f->Reverse != old) {
        f->SpinStep = 0;
        f->SpinSpeed = 0.0f;
    }
    return true;
}

// PANZERS 0x550650 (not executed in the menu): after a collision the local
// path becomes a stop at the frame.
void SDriver::SetLocalAfterCollision(SGhostFrame* f)
{
    for (int i = 0; i < LocalWayPoints.Size; ++i)                 // 0x550aa0
        Wpm_Free(&LocalWayPoints.Array[i]);
    LocalWayPoints.Clear(f->WayPointIdx);
    int k = LocalWayPoints.Add();
    SWayPointWithManoeuvres& w = LocalWayPoints.Array[k];
    w.X = f->X;
    w.Z = f->Z;
    w.PointType = kLwpLocal;
    Wpm_AddStop(&w, DU_B(Unit, 0xcc) != 0);
    f->ManoeuvreIdx = 0;
    f->PointIdx = 0;
    ArrivalActive = false;
    ArrivalDist2 = 0.0f;
    Arrival80 = 0.0f;
    ArrivalDir = 0.0f;
}

// ---------------------------------------------------------------------------
// MoveTowardNextWayPoint (vtable +0x18)

// PANZERS 0x5582a0 SDriver::MoveTowardNextWayPoint
bool SDriver::MoveTowardNextWayPoint(SGhostFrame* f)
{
    SWayPoint wp;
    GetWayPoint(&wp, f);
    if (wp.Type == kWpFollowUnit) {
        ManoeuvreOpeningFrame = 0;
        ManoeuvreFrame = DU_Ghosts(Unit)->Top;
        WaitForBoss = true;
        if (!Target) {
            DrvPanic("SDriver::MoveTowardNextWayPoint(): Target is NULL");
            return false;
        }
        if (Target->Type != kTargetUnit) {
            DrvPanic("SDriver::MoveTowardNextWayPoint - Target->Type != TARGET_UNIT");
            return false;
        }
        SIUnit* tu = HdUnit(Target->Unit);
        if (!tu)
            return false;
        float r;
        int k = Target->Kind;
        if (k == 6 || k == 7)
            r = 10.0f;
        else if (SpeedSteps == 1)
            r = DU_F(Unit, 0x54) * 0.5f + DU_F(tu, 0x54) * 0.5f + 0.5f;
        else
            r = DU_F(Unit, 0x54) * 0.5f + DU_F(tu, 0x54) * 0.5f + 1.75f;
        float dx = wp.X - f->X;
        float dz = wp.Z - f->Z;
        float d2 = dx * dx + dz * dz;
        if (r * r > d2) {
            WantedSpeed = 0.0f;
        } else {
            float r12 = r * 1.2f;
            float r14 = r * 1.4f;
            if (d2 > r12 * r12 && r14 * r14 > d2) {
                float k2 = DU_ClassType(Unit) == 5 ? 0.7f : 0.3f;
                float v = GetMaxSpeed() * k2;
                float ts = DU_F(tu, 0xc8);
                if (ts > v) {
                    if (ts > GetMaxSpeed())
                        ts = GetMaxSpeed();
                    WantedSpeed = ts;
                }
            } else if (d2 >= r14 * r14) {
                WantedSpeed = -1.0f;
            }
        }
        SetUnitSpeed(f);
        if (!(f->Speed > 0.0f))
            return false;
        TurnTowardsPoint(f, wp.X, wp.Z);
        GhostStepStraight(f, wp.X, wp.Z);
        return true;
    }
    if (wp.Type == kWpPoint || wp.Type == kWpNear) {
        SetUnitSpeed(f);
        float dx = wp.X - f->X;
        float dz = wp.Z - f->Z;
        float d;
        if (ArrivalActive) {
            double a = (double)(float)DAtan2((double)dx, (double)dz);
            double diff = AbsDiff(a, (double)ArrivalDir);
            if (1.5707963705062866 > diff) {
                TurnTowardsPoint(f, wp.X, wp.Z);
                GhostStepStraight(f, wp.X, wp.Z);
                return true;
            }
            d = ArrivalDir;
        } else {
            float a = DAtan2f((double)dx, (double)dz);
            d = (1e-4 > fabs((double)dx) && 1e-4 > fabs((double)dz)) ? f->Dir : a;
        }
        if (SpinSteps == 1)
            TurnTowardsDirect(f, d);
        else
            TurnTowardsInSteps(f, d);
        GhostStepStraight(f, wp.X, wp.Z);
        return true;
    }
    if (wp.Type == kWpPointDir) {
        SetUnitSpeed(f);
        if (ArrivalActive)
            TurnTowardsDir(f, wp.Dir);
        else
            TurnTowardsPoint(f, wp.X, wp.Z);
        GhostStepStraight(f, wp.X, wp.Z);
        return true;
    }
    if (wp.Type == kWpTurn) {
        f->Speed = 0.0f;
        TurnTowardsDir(f, wp.Dir);
        return true;
    }
    DrvPanic("SDriver::MoveTowardNextWayPoint: Waypoint type unimplemented (%d)", wp.Type);
    return false;
}

// PANZERS 0x5588e0: the squad frame also carries its members' relative
// positions (frame +0x30) and directions (+0x58).
bool SPanzersSquadDriver::MoveTowardNextWayPoint(SGhostFrame* f)
{
    bool r = SDriver::MoveTowardNextWayPoint(f);
    if (f->Speed > 0.0f) {
        if (g_DriverEnv.GetFrame() % 2 == 0)
            g_DriverEnv.SquadMembersStep(Unit, f->Dir);
        if (g_DriverEnv.GetFrame() % 2 == 0)
            g_DriverEnv.SquadMembersStep2(Unit, f->Dir);
    }
    int n = DU_I(Unit, 0x17c);
    for (int i = 0; i < n; ++i) {
        float rel[2];
        g_DriverEnv.SquadMemberRelativePos(Unit, rel, i);
        double a = (double)f->Field6c;
        f->Gear[i][0] = (float)(DCos(a) * (double)rel[0] + DSin(a) * (double)rel[1]);
        f->Gear[i][1] = (float)(DCos(a) * (double)rel[1] - DSin(a) * (double)rel[0]);
        f->Gear2[i] = f->Dir;
    }
    return r;
}

// PANZERS 0x558a60: a squad member walks to its formation point (target
// type 3), with a random catch-up "wobble" of the speed.
bool SPanzersSquadMemberDriver::MoveTowardNextWayPoint(SGhostFrame* f)
{
    STarget* t = Target;
    if (t->Type == kTargetPosDir) {
        float dz = t->Pos[2] - f->Z;
        float dx = t->Pos[0] - f->X;
        float d2 = dz * dz + dx * dx;
        SIUnit* boss = HdUnit(DU_I(Unit, 0x78));
        if (!boss)
            return true;
        if (DU_ClassType(boss) == 11)
            f->Speed = boss->GetMoveSpeed(-1);
        else
            f->Speed = GetMaxSpeed();
        if (Wobble == 0.0f) {
            float sp = f->Speed;
            float s5 = sp * 5.0f;
            float s4 = sp * 4.0f;
            if (d2 > s5 * s5)
                f->Speed = sp * 1.2f;
            else if (d2 > s4 * s4)
                f->Speed = sp * 1.1f;
            else
                f->Speed = sp * 1.05f;
        } else if (DU_ClassType(boss) != 11 && DU_I(boss, 0x17c) > 1) {
            float w = Wobble + WobbleSpeed;
            Wobble = w;
            if (w > 16.0f)
                WobbleSpeed = -0.75f;
            if (0.0f > Wobble)
                Wobble = 0.0f;
            f->Speed = (float)((1.0 - (double)Wobble * 0.01) * (double)f->Speed);
        }
        if (DU_ClassType(boss) != 11 && DU_I(boss, 0xf8) == 2 && Wobble == 0.0f) {
            int fr = g_DriverEnv.GetFrame();
            if ((fr + DU_I(Unit, 0x74) * 20) % 30 == 0 && HdRandInt(2) == 0 && 0.25f > d2) {
                Wobble = Wobble + 0.22f;
                WobbleSpeed = 0.22f;
            }
        }
        float thr;
        if (DU_I(boss, 0xf8) == 1)
            thr = 3.1415927f;
        else if (0.19634954631328583 > AbsDiff((double)DU_F(boss, 0xb0), (double)f->Dir))
            thr = 3.1415927f;
        else
            thr = 1.5707964f;
        float a = DAtan2f((double)dx, (double)dz);
        float a2;
        float thr2;
        if (1e-4 > fabs((double)dx) && 1e-4 > fabs((double)dz)) {
            a2 = Target->Dir;
            thr2 = 6.2831855f;
        } else {
            a2 = a;
            thr2 = thr;
        }
        float fdir = f->Dir;
        float diff2 = (float)AbsDiff((double)a2, (double)fdir);
        if ((thr2 > diff2 && 0.0625f > d2) || DU_ClassType(boss) == 11) {
            bool reached = GhostTurnTowards(f, Target->Pos[0], Target->Pos[2]);
            float ad = (SpinSteps == 1) ? TurnTowardsDirect(f, Target->Dir)
                                        : TurnTowardsInSteps(f, Target->Dir);
            if (reached && 1e-4 > (double)ad)
                return false;
            return true;
        }
        float dx2 = Target->Pos[0] - f->X;
        float dz2 = Target->Pos[2] - f->Z;
        float a3 = DAtan2f((double)dx2, (double)dz2);
        float d = (1e-4 > fabs((double)dx2) && 1e-4 > fabs((double)dz2)) ? fdir : a3;
        float ad = (SpinSteps == 1) ? TurnTowardsDirect(f, d) : TurnTowardsInSteps(f, d);
        if (0.7853982f > ad) {
            if (!GhostStepTowards(f, Target->Pos[0], Target->Pos[2]))
                return true;
            double ad2 = (double)TurnTowardsDir(f, Target->Dir);
            if (1e-4 > ad2)
                return false;
            return true;
        }
        f->Speed = 0.0f;
        return true;
    }
    if (t->Type == kTargetTurn) {
        float r = TurnToDir(t->Dir);
        f->X = DU_F(Unit, 0x8c);
        f->Y = DU_F(Unit, 0x90);
        f->Z = DU_F(Unit, 0x94);
        f->Dir = DU_F(Unit, 0xb0);
        f->Speed = 0.0f;
        f->SpinSpeed = DU_F(Unit, 0xd0);
        return r != 0.0f;
    }
    if (t->Type != kTargetUnit) {
        DrvPanic("SPanzersSquadMemberDriver::MoveTowardNextWayPoint - bad target type (%d)", t->Type);
        return true;
    }
    TurnToPoint(t->Pos[0], t->Pos[1], t->Pos[2]);
    f->X = DU_F(Unit, 0x8c);
    f->Y = DU_F(Unit, 0x90);
    f->Z = DU_F(Unit, 0x94);
    f->Dir = DU_F(Unit, 0xb0);
    f->Speed = 0.0f;
    f->SpinSpeed = DU_F(Unit, 0xd0);
    return true;
}

// ---------------------------------------------------------------------------
// Turning in place while stationary (vtable +0x1c/+0x20; not executed in
// the menu)

// PANZERS 0x55c3f0
float SDriver::TurnToDir(float dir)
{
    DU_B(Unit, 0xcc) = 0;
    SGhostFrame f;
    memset(&f, 0, sizeof(f));
    FillGhostFrame(&f);
    float r = (SpinSteps == 1) ? TurnTowardsDirect(&f, dir) : TurnTowardsInSteps(&f, dir);
    DU_F(Unit, 0xb0) = f.Dir;
    DU_F(Unit, 0xd0) = f.SpinSpeed;
    DU_F(Unit, 0xd4) = ((float)f.SpinStep / (float)SpinSteps) * PDriver->WheelTurnAngle;
    DrvUnit_MoveTowedChain(Unit);
    if (r == 0.0f)
        DU_F(Unit, 0xd0) = 0.0f;
    return r;
}

// PANZERS 0x55c530
float SDriver::TurnToPoint(float x, float y, float z)
{
    (void)y;
    DU_B(Unit, 0xcc) = 0;
    SGhostFrame f;
    memset(&f, 0, sizeof(f));
    FillGhostFrame(&f);
    float a = DAtan2f((double)(x - f.X), (double)(z - f.Z));
    if (1e-4 > fabs((double)(x - f.X)) && 1e-4 > fabs((double)(z - f.Z)))
        a = f.Dir;
    float r = (SpinSteps == 1) ? TurnTowardsDirect(&f, a) : TurnTowardsInSteps(&f, a);
    DU_F(Unit, 0xb0) = f.Dir;
    DU_F(Unit, 0xd0) = f.SpinSpeed;
    DU_F(Unit, 0xd4) = ((float)f.SpinStep / (float)SpinSteps) * PDriver->WheelTurnAngle;
    DrvUnit_MoveTowedChain(Unit);
    if (r == 0.0f)
        DU_F(Unit, 0xd0) = 0.0f;
    return r;
}

// PANZERS 0x55c4d0
float SPanzersSquadDriver::TurnToDir(float dir)
{
    float r = SDriver::TurnToDir(dir);
    int n = DU_I(Unit, 0x17c);
    for (int i = 0; i < n; ++i)
        DU_F(Unit, 0x238 + i * 4) = DU_F(Unit, 0xb0);
    return r;
}

// PANZERS 0x55c6a0: the member offsets (unit +0x210) follow the squad
// direction (+0x24c); each member faces the point.
float SPanzersSquadDriver::TurnToPoint(float x, float y, float z)
{
    float r = SDriver::TurnToPoint(x, y, z);
    int n = DU_I(Unit, 0x17c);
    for (int i = 0; i < n; ++i) {
        float rel[2];
        g_DriverEnv.SquadMemberRelativePos(Unit, rel, i);
        double a = (double)DU_F(Unit, 0x24c);
        DU_F(Unit, 0x210 + i * 8) = (float)(DCos(a) * (double)rel[0] + DSin(a) * (double)rel[1]);
        DU_F(Unit, 0x214 + i * 8) = (float)(DCos(a) * (double)rel[1] - DSin(a) * (double)rel[0]);
        float mx = x - (DU_F(Unit, 0x8c) + DU_F(Unit, 0x210 + i * 8));
        float mz = z - (DU_F(Unit, 0x214 + i * 8) + DU_F(Unit, 0x94));
        DU_F(Unit, 0x238 + i * 4) = DAtan2f((double)mx, (double)mz);
    }
    return r;
}

// ---------------------------------------------------------------------------
// Arrival

// PANZERS 0x554fc0 SDriver::HasTheUnitArrivedAtTheWayPoint(frame, prev)
bool SDriver::HasArrived(SGhostFrame* B, const SGhostFrame* A)
{
    if (0.0f >= ArrivalRadius) {
        DrvPanic("SDriver::HasTheUnitArrivedAtTheWayPoint - Arrival.Radius <= 0");
        return false;
    }
    SWayPoint wp;
    GetWayPoint(&wp, B);
    float R = ArrivalRadius;
    switch (wp.Type) {
    case kWpFollowUnit:
        return false;
    case kWpPoint:
    case kWpPointDir: {
        float d2 = (wp.X - B->X) * (wp.X - B->X) + (wp.Z - B->Z) * (wp.Z - B->Z);
        if (!(R * R > d2)) {
            if (B->Speed == 0.0f)
                goto arrived;
            if (!ArrivalActive)
                return false;
            goto arrived;
        }
        float ws = WantedSpeed;
        if (ws == 0.0f && B->Speed == 0.0f)
            goto arrived;
        if (!ArrivalActive) {
            float e2 = (wp.X - A->X) * (wp.X - A->X) + (wp.Z - A->Z) * (wp.Z - A->Z);
            if (1e-4 > (double)e2) {
                if (A->Speed == 0.0f)
                    goto arrived;
                if (ws != 0.0f)
                    goto arrived;
            }
            ArrivalDist2 = d2;
            ArrivalActive = true;
            ArrivalDir = B->Reverse ? RevDir(B->Dir) : B->Dir;
            return false;
        }
        if (!(d2 >= ArrivalDist2)) {
            ArrivalDist2 = d2;
            return false;
        }
        if (ws != 0.0f)
            goto arrived;
        if (wp.Type == kWpPoint) {
            if (!(B->Speed > 0.0f))
                goto arrived;
            return false;
        }
        // kWpPointDir: also the final direction.
        {
            float d = A->Reverse ? RevDir(A->Dir) : A->Dir;
            float ad = (float)DAngleDist((double)wp.Dir, (double)d);
            if (SpinSteps <= 1) {
                if (!(0.001 > (double)ad))
                    return false;
            } else if (WantedSpin == 0.0f) {
                if (B->SpinStep != 0)
                    return false;
            } else {
                if (fabs((double)B->SpinSpeed) < (double)ad)
                    return false;
            }
            if (WantedSpeed != 0.0f)
                goto arrived;
            if (!(B->Speed > 0.0f))
                goto arrived;
            return false;
        }
    }
    case kWpTurn: {
        float d = A->Reverse ? RevDir(A->Dir) : A->Dir;
        float ad = (float)AbsDiff((double)wp.Dir, (double)d);
        if (SpinSteps <= 1) {
            if (0.001 > (double)ad)
                goto arrived;
            return false;
        }
        if (WantedSpin == 0.0f) {
            if (B->SpinStep != 0)
                return false;
            goto arrived;
        }
        if (fabs((double)B->SpinSpeed) < (double)ad)
            return false;
        goto arrived;
    }
    case kWpNear: {
        float d2 = (wp.X - B->X) * (wp.X - B->X) + (wp.Z - B->Z) * (wp.Z - B->Z);
        if (!(R * R > d2))
            return false;
        if (WantedSpeed != 0.0f)
            goto arrived;
        DrvPanic("SDriver::HasTheUnitArrivedAtTheWayPoint - We should stop at WAYPOINT_NEAR_TO_POINT.");
        return false;
    }
    default:
        DrvPanic("SDriver::HasTheUnitArrivedAtTheWayPoint - unknown waypoint type (%d)", wp.Type);
        return false;
    }
arrived:
    ArrivalActive = false;
    ArrivalDist2 = 0.0f;
    Arrival80 = 0.0f;
    ArrivalDir = 0.0f;
    return true;
}

// ---------------------------------------------------------------------------
// Collisions

// HD SUnit 0x5bc8d0: the heading from the unit to its active driver's first
// global waypoint (only the +0x28 driver).
static bool UnitHeadingToGlobalWp(SIUnit* unit, float* out)
{
    int i = DU_I(unit, 0x28);
    if (i < 0)
        return false;
    if (i >= DU_I(unit, 0x3c)) {
        DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SDriver*", i);
        return false;
    }
    SDriver* d = ((SDriver**)DU_P(unit, 0x38))[i];
    if (!d->Target || d->GlobalWayPoints.Size == 0)
        return false;
    float dx = d->GlobalWayPoints.Array[0].X - DU_F(unit, 0x8c);
    float dz = d->GlobalWayPoints.Array[0].Z - DU_F(unit, 0x94);
    *out = DAtan2f((double)dx, (double)dz);
    return true;
}

// The near-unit filter shared by 0x554870 and 0x555450.
static SIUnit* NearCandidate(SDriver* d, int i)
{
    SIUnit* u = d->Unit;
    char* arr = (char*)DU_P(u, 0x1a8);
    int idx = *(int*)(arr + i * 8);
    if (!HdUnitLive(idx))
        return nullptr;
    if (*(arr + i * 8 + 4) != 0)
        return nullptr;
    SIUnit* o = HdUnit(idx);
    if (DU_B(o, 0x7c) != 0 && DU_I(o, 0x78) == DU_I(u, 0x74))
        return nullptr;
    if (DU_B(o, 0x168) != 0 || DU_B(o, 0x150) != 0)
        return nullptr;
    if (d->Target->Kind == 1 && d->Target->Unit == DU_I(o, 0x74))
        return nullptr;
    if (DU_ClassType(u) == 5 && DU_ClassType(o) == 5)
        return nullptr;
    return o;
}

// PANZERS 0x554870 (vtable +0x3c): is the frame blocked (static map or a
// nearby unit's predicted position)? Lower-priority stopped units in the
// way are sent aside.
bool SDriver::PredictGhost(SGhostFrame* f, int* outUnit, float* outPos, float* outDir)
{
    *outUnit = -1;
    SIUnit* u = Unit;
    if (DrvUnit_TestBlockMap(u, f->X, f->Z, f->Dir, DU_I(u, 0x5c), (short)DU_S(u, 0xd8)))
        return true;
    if (!(DU_S(u, 0xd8) & 0x800))
        return false;
    int next = DU_Ghosts(u)->Top + 1;
    for (int i = 0; i < DU_I(Unit, 0x1ac); ++i) {
        SIUnit* o = NearCandidate(this, i);
        if (!o)
            continue;
        u = Unit;
        SGhostQueue* oq = DU_Ghosts(o);
        if (oq->Count != 0) {
            int m = next < oq->Top ? next : oq->Top;
            int gi = m > oq->Bottom ? m : oq->Bottom;
            SGhostFrame g = *GhostAt(o, gi);
            outPos[0] = g.X;
            outPos[1] = g.Z;
            *outDir = g.Dir;
            if (g.Reverse)
                *outDir = RevDir(g.Dir);
        } else {
            outPos[0] = DU_F(o, 0x8c);
            outPos[1] = DU_F(o, 0x94);
            *outDir = DU_F(o, 0xb0);
            if (DU_B(o, 0xcc) != 0)
                *outDir = RevDir(DU_F(o, 0xb0));
        }
        float dx = outPos[0] - f->X;
        float dz = outPos[1] - f->Z;
        double r = (double)(DU_F(u, 0x54) + DU_F(o, 0x54)) + 0.5;
        if (!(r * r * 0.25 > (double)(dx * dx + dz * dz)))
            continue;
        DrvUnit_MarkBlockMap(o, true, outPos[0], outPos[1], *outDir, 0x40);
        bool blocked = DrvUnit_TestBlockMap(Unit, f->X, f->Z, f->Dir, DU_I(Unit, 0x5c), 0x40);
        DrvUnit_MarkBlockMap(o, false, outPos[0], outPos[1], *outDir, 0x40);
        if (!blocked)
            continue;
        if (DU_I(o, 0x2e0) >= DU_I(Unit, 0x2e0)) {
            *outUnit = *(int*)((char*)DU_P(Unit, 0x1a8) + i * 8);
            return true;
        }
        if (DrvUnit_IsMoving(o))
            continue;
        // Send the stopped unit aside.
        float fx = f->X;
        float fz = f->Z;
        float ddx = outPos[0] - fx;
        float ddz = outPos[1] - fz;
        float nx, nz;
        float a;
        if (0.0010000000474974513 > fabs((double)ddx) && 0.0010000000474974513 > fabs((double)ddz)) {
            nx = 1.0f;
            nz = 0.0f;
            a = 0.0f;
        } else {
            double inv = 1.0 / sqrt((double)(ddx * ddx + ddz * ddz));
            nx = (float)((double)ddx * inv);
            nz = (float)((double)ddz * inv);
            a = DAtan2f((double)nx, (double)nz);
        }
        float ex = fx - DU_F(Unit, 0x8c);
        float ez = fz - DU_F(Unit, 0x94);
        if (!(0.0010000000474974513 > fabs((double)ex) && 0.0010000000474974513 > fabs((double)ez))) {
            double inv = 1.0 / sqrt((double)(ex * ex + ez * ez));
            float ux = (float)((double)ex * inv);
            float uz = (float)((double)ez * inv);
            float b = DAtan2f((double)ux, (double)uz);
            double s = DSign(DWrapSub((double)a, (double)b));
            if ((int)s < 0) {
                nx = -uz;
                nz = ux;
            } else {
                nx = uz;
                nz = -ux;
            }
        }
        float px = DU_F(o, 0x8c) + (nx * DU_F(Unit, 0x54)) * 0.5f;
        float pz = (nz * DU_F(Unit, 0x54)) * 0.5f + DU_F(o, 0x94);
        STarget* t = STarget::Create(1);
        float p[2] = { px, pz };
        t->SetGroundPos(p);
        t->Unit = DU_I(Unit, 0x74);
        o->SetCurrentTarget(t, 0);
    }
    return false;
}

// PANZERS 0x555450 (vtable +0x38): is the oldest frame (the one the unit
// moves onto next) blocked by a nearby unit standing there?
bool SDriver::RefreshGhost()
{
    if (PDriver->Type == 3)
        return false;
    SIUnit* u = Unit;
    SGhostQueue* q = DU_Ghosts(u);
    SGhostFrame* b = GhostAt(u, q->Bottom);
    float bx = b->X;
    float bz = b->Z;
    float bdir = GhostAt(u, q->Bottom)->Dir;
    if (!BlockMap_CheckDynamic(g_World, bx, bz, DU_I(u, 0x58)))
        return false;
    if (!(DU_S(u, 0xd8) & 0x800))
        return false;
    for (int i = 0; i < DU_I(Unit, 0x1ac); ++i) {
        SIUnit* o = NearCandidate(this, i);
        if (!o)
            continue;
        u = Unit;
        float ox = DU_F(o, 0x8c);
        float oz = DU_F(o, 0x94);
        float od = DU_F(o, 0xb0);
        if (DU_B(o, 0xcc) != 0)
            od = RevDir(od);
        float dz = oz - bz;
        float dx = ox - bx;
        double r = (double)(DU_F(u, 0x54) + DU_F(o, 0x54)) + 0.5;
        if (!(r * r * 0.25 > (double)(dx * dx + dz * dz)))
            continue;
        DrvUnit_MarkBlockMap(o, true, ox, oz, od, 0x40);
        bool blocked = DrvUnit_TestBlockMap(Unit, bx, bz, bdir, DU_I(Unit, 0x5c), 0x40);
        DrvUnit_MarkBlockMap(o, false, ox, oz, od, 0x40);
        if (!blocked)
            continue;
        if (DU_I(o, 0x2e0) >= DU_I(Unit, 0x2e0))
            return true;
        if (DU_B(o, 0x150) != 0)
            continue;
        if (DrvUnit_IsMoving(o)) {
            STarget* ot = (STarget*)DU_P(o, 0x1f4);
            if (!ot || ot->Kind == 1)
                continue;
        }
        float nx, nz;
        if (0.0010000000474974513 > fabs((double)dx) && 0.0010000000474974513 > fabs((double)dz)) {
            nx = 1.0f;
            nz = 0.0f;
        } else {
            double inv = 1.0 / sqrt((double)(dx * dx + dz * dz));
            nx = (float)((double)dx * inv);
            nz = (float)((double)dz * inv);
        }
        float px = (nx * DU_F(Unit, 0x54)) * 0.25f + ox;
        float pz = (nz * DU_F(Unit, 0x54)) * 0.25f + oz;
        STarget* t = STarget::Create(1);
        float p[2] = { px, pz };
        t->SetGroundPos(p);
        t->Unit = DU_I(Unit, 0x74);
        o->SetCurrentTarget(t, 0);
    }
    return false;
}

// ---------------------------------------------------------------------------
// Ghost_NextStep (vtable +0x44) and FollowGhost

// PANZERS 0x553c00 SDriver::Ghost_NextStep
void SDriver::Ghost_NextStep()
{
    PZ_M2_TRACE("SDriver::Ghost_NextStep (0x553c00)");
    SGhostFrame A;
    SGhostFrame B;
    memset(&A, 0, sizeof(A));
    memset(&B, 0, sizeof(B));
    {
        SGhostQueue* q = DU_Ghosts(Unit);
        A = *GhostAt(Unit, q->Top);
    }
    for (;;) {
        B = A;
        if (!StoppingDistance && IsTheGhostInStoppingDistance(&B)) {
            StoppingDistance = true;
            if (B.PointIdx == 0)
                ManoeuvreOpeningFrame = DU_Ghosts(Unit)->Top;
            if (B.Speed > 0.0f && CanCancelManoeuvre(&B))
                WantedSpeed = 0.0f;
        }
        if (ManoeuvreOpeningFrame > 0 && DU_Ghosts(Unit)->Bottom > ManoeuvreOpeningFrame) {
            DrvPanic("SDriver::Ghost_NextStep() - Unit->GhostFrames.GetBottomIndex() > GhostState.ManoeuvreOpeningFrame");
            return;
        }
        bool moved = MoveTowardNextWayPoint(&B);
        if (HasArrived(&B, &A)) {
            int wi = B.WayPointIdx;
            SManoeuvre& m = LocalWayPoints.At(wi).Manoeuvres.At(B.ManoeuvreIdx);
            if (B.PointIdx == m.Points.Size - 1) {
                ManoeuvreOpeningFrame = 0;
                ManoeuvreFrame = DU_Ghosts(Unit)->Top;
            }
            if (!NextWayPoint(&A, false, true)) {
                // The local path is done.
                int i = A.WayPointIdx - 1;
                GhostActive = false;
                if (LocalWayPoints.At(i).PointType == kLwpLocal)
                    return;
                if (GlobalWayPoints.Size == 0) {
                    DrvPanic("SDriver::Ghost_NextStep() - GlobalWayPointsArray.Remove(0) but it is an empty array.");
                    return;
                }
                GlobalWayPoints.Remove(0);
                i = A.WayPointIdx - 1;
                int pt = LocalWayPoints.At(i).PointType;
                if (pt == kLwpLastPass) {
                    GhostRestart = true;
                    return;
                }
                if (pt != kLwpLastStop)
                    DrvPanic("SDriver::Ghost_NextStep() - Invalid PointType when a unit reached the end of a LocalWayPointsArray.");
                LocalPathEnd = true;
                return;
            }
            *GhostAt(Unit, DU_Ghosts(Unit)->Top) = A;            // the top frame takes A
            continue;
        }
        float predDir = 0.0f;
        bool blocked = PredictGhost(&B, &CollisionUnit, CollisionPos, &predDir);
        if (!blocked && DU_Ghosts(Unit)->Count <= 500) {
            if (moved)
                UnitGhostPush(Unit, &B);
            return;
        }
        int wpIdx = B.WayPointIdx;
        if (wpIdx == LocalWayPoints.Size - 1) {
            SManoeuvre& m = LocalWayPoints.At(wpIdx).Manoeuvres.At(B.ManoeuvreIdx);
            if (B.PointIdx == m.Points.Size - 1 && ArrivalActive) {
                // Blocked at the final point while arriving: done.
                GhostActive = false;
                if (LocalWayPoints.At(wpIdx).PointType == kLwpLocal)
                    return;
                if (GlobalWayPoints.Size == 0) {
                    DrvPanic("SDriver::Ghost_NextStep() - GlobalWayPointsArray.Remove(0) but it is an empty array.");
                    return;
                }
                GlobalWayPoints.Remove(0);
                int pt = LocalWayPoints.At(B.WayPointIdx).PointType;
                if (pt == kLwpLastPass) {
                    GhostRestart = true;
                    return;
                }
                if (pt == kLwpLastStop) {
                    LocalPathEnd = true;
                    return;
                }
                DrvPanic("SDriver::Ghost_NextStep() - Invalid PointType when a unit reached the end of a LocalWayPointsArray and there was a collision at the last point.");
                return;
            }
        }
        SIUnit* collider = nullptr;
        if (CollisionUnit >= 0) {
            collider = HdUnit(CollisionUnit);
            if (collider && DU_B(collider, 0xcd) != 0) {
                WaitForBoss = true;
                SkipFollow = true;
                return;
            }
        }
        if (ManoeuvreOpeningFrame > 0) {
            // Back to the manoeuvre opening frame and try the next manoeuvre.
            int top = DU_Ghosts(Unit)->Top;
            while (top > ManoeuvreOpeningFrame) {
                if (DU_Ghosts(Unit)->Top != top) {
                    DrvPanic("SDriver::Ghost_NextStep() - Unit->GhostFrames.GetTopIndex() != i");
                    return;
                }
                UnitGhostRemoveTopAll(Unit);
                --top;
            }
            A = *GhostAt(Unit, DU_Ghosts(Unit)->Top);
            int best = Wpm_PopBest(&LocalWayPoints.At(B.WayPointIdx));
            if (best > -1) {
                A.ManoeuvreIdx = best;
                A.PointIdx = 0;
                *GhostAt(Unit, DU_Ghosts(Unit)->Top) = A;
                ArrivalActive = false;
                ArrivalDist2 = 0.0f;
                Arrival80 = 0.0f;
                ArrivalDir = 0.0f;
                if (!NextWayPoint(&A, true, false)) {
                    DrvPanic("SDriver::Ghost_NextStep() - No WP after changing manoeuvre!");
                    return;
                }
                continue;
            }
        }
        if (collider && DrvUnit_IsMoving(collider)) {
            float myHead = 0.0f;
            float colHead = 0.0f;
            UnitHeadingToGlobalWp(Unit, &myHead);
            float x1, x0;
            if (UnitHeadingToGlobalWp(collider, &colHead)) {
                x1 = colHead;
                x0 = myHead;
            } else {
                x1 = predDir;
                x0 = B.Dir;
            }
            float f1 = (float)AbsDiff((double)x1, (double)x0);
            double a = (double)(float)DAtan2((double)(DU_F(collider, 0x8c) - DU_F(Unit, 0x8c)),
                                              (double)(DU_F(collider, 0x94) - DU_F(Unit, 0x94)));
            double d2 = AbsDiff(a, (double)myHead);
            if (1.5707963705062866 > (double)f1 && 1.5707963705062866 > (double)(float)d2) {
                if (CollisionUnit != LastCollisionUnit) {
                    LastCollisionUnit = CollisionUnit;
                    CollisionCount = 0;
                }
                ++CollisionCount;
                if (CollisionCount < 8) {
                    AvoidUnit = -1;
                    int top = DU_Ghosts(Unit)->Top;
                    while (top > ManoeuvreFrame && DU_Ghosts(Unit)->Count > 1) {
                        if (DU_Ghosts(Unit)->Top != top) {
                            DrvPanic("SDriver::Ghost_NextStep() 2. - Unit->GhostFrames.GetTopIndex() != i");
                            return;
                        }
                        UnitGhostRemoveTopAll(Unit);
                        --top;
                    }
                    SetLocalAfterCollision(&A);
                    NextWayPoint(&A, true, false);
                    SGhostFrame* tf = GhostAt(Unit, DU_Ghosts(Unit)->Top);
                    tf->WayPointIdx = A.WayPointIdx;
                    tf->ManoeuvreIdx = A.ManoeuvreIdx;
                    tf->PointIdx = A.PointIdx;
                    WaitTicks = 10;
                    GhostRestart = true;
                    return;
                }
                AvoidUnit = CollisionUnit;
                LastCollisionUnit = -1;
                StartStoppedGhost(false);
                WaitTicks = 10;
                GhostRestart = true;
                return;
            }
            AvoidUnit = CollisionUnit;
            StartStoppedGhost(false);
            GhostRestart = true;
            int g = DU_I(Unit, 0x254);
            if (g != -1) {
                int cg = DU_I(collider, 0x254);
                if (cg != -1) {
                    if (cg != g) {
                        WaitTicks = (g < cg) ? 5 : 15;
                        return;
                    }
                    float da = g_DriverEnv.GetMovementGroupDistance(Unit);
                    float db = g_DriverEnv.GetMovementGroupDistance(collider);
                    WaitTicks = (db > da) ? 5 : 15;
                    return;
                }
                if (g > -1) {
                    WaitTicks = 5;
                    return;
                }
            }
            if (DU_I(collider, 0x254) > -1) {
                WaitTicks = 15;
                return;
            }
            WaitTicks = (DU_I(Unit, 0x74) < DU_I(collider, 0x74)) ? 5 : 15;
            return;
        }
        // Blocked by the map or a stopped unit: wait and restart.
        if (LastGhostPos[0] != B.X || LastGhostPos[1] != B.Y || LastGhostPos[2] != B.Z)
            UnitGhostClearAll(Unit);
        LastGhostPos[0] = B.X;
        LastGhostPos[1] = B.Y;
        LastGhostPos[2] = B.Z;
        WaitTicks = 2;
        GhostActive = false;
        GhostRestart = true;
        return;
    }
}

// PANZERS 0x553960: once per tick, the unit takes the oldest ghost frame.
void SDriver::FollowGhost()
{
    int frame = g_DriverEnv.GetFrame();
    if (LastFollowFrame == frame || SkipFollow)
        return;
    SGhostQueue* q = DU_Ghosts(Unit);
    if (q->Count == 0)
        return;
    if (q->Count == 1) {
        if (!GhostHold)
            UnitGhostRemoveBottomAll(Unit);
        return;
    }
    DrvUnit_ClearNear(Unit);
    UnitGhostRemoveBottomAll(Unit);
    int b = DU_Ghosts(Unit)->Bottom;
    Unit->SetOnBlockMap(false);
    if (RefreshGhost()) {
        UnitGhostClearAll(Unit);
        GhostRestart = true;
        WaitTicks = 2;
        GhostActive = false;
        Unit->SetOnBlockMap(true);
        return;
    }
    LastFollowFrame = g_DriverEnv.GetFrame();
    SIUnit* u = Unit;
    const SGhostFrame* g = GhostAt(u, b);
    DU_F(u, 0x8c) = g->X;
    DU_F(u, 0x90) = g->Y;
    DU_F(u, 0x94) = g->Z;
    DU_F(u, 0xb0) = g->Dir;
    DU_F(u, 0xc8) = g->Speed;
    DU_B(u, 0xcc) = g->Reverse ? 1 : 0;
    DU_F(u, 0xd0) = g->SpinSpeed;
    DU_F(u, 0xd4) = ((float)g->SpinStep / (float)SpinSteps) * PDriver->WheelTurnAngle;
    for (int i = 0; i < 5; ++i) {
        DU_F(u, 0x210 + i * 8) = g->Gear[i][0];
        DU_F(u, 0x214 + i * 8) = g->Gear[i][1];
        DU_F(u, 0x238 + i * 4) = g->Gear2[i];
    }
    DU_F(u, 0x24c) = g->Field6c;
    if (DU_ClassType(u) == 10)
        DU_F(u, 0x300) = g->Field70;
    DrvUnit_CopyPosToTowed(u);
    DrvUnit_SetMoveState(u);
    u->SetOnBlockMap(true);
    if (DU_Ghosts(u)->Count == 1 && !GhostHold)
        UnitGhostRemoveBottomAll(u);
    g_DriverEnv.WorldUnitMoved(DU_I(u, 0x74), WantedSpeed);
    DrvUnit_CheckOnMap(u);
    DrvUnit_CheckStaticMap(u);
}

} // namespace pz
