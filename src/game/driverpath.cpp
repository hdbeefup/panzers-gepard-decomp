// src/game/driverpath.cpp
// SDriver path finding: the global path (A* over the block map), the local
// path (local A* + SWayPointWithManoeuvres) and the formation goal.
// OWNER: P. HD 0x550b40..0x553100, 0x556ac0, 0x55aa80, 0x55b1d0.

#include <math.h>
#include "driver.h"
#include "driverunit.h"
#include "drivermath.h"
#include "manoeuvre.h"
#include "target.h"
#include "iunit.h"
#include "world.h"
#include "worldapi.h"
#include "astar.h"
#include "blockmap.h"

namespace pz {

// The HD "wrap once into (-pi, pi]" idiom, in double.
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

// PANZERS 0x550b40: clips the segment s = {x0, z0, x1, z1} to the circle
// (centre c, radius r): p1 moves to the far crossing when it lies outside,
// p0 to the near crossing. False when the segment does not cross the disc.
static bool ClipSegmentToCircle(float* s, const float* c, float r)
{
    float cz = c[1];
    float cx = c[0];
    float z0 = s[1];
    float x0 = s[0];
    float dz = s[3] - z0;
    float dx = s[2] - x0;
    float a = dz * dz + dx * dx;
    float b = ((x0 - cx) * dx + (z0 - cz) * dz) * 2.0f;
    float k = (((z0 * z0 + x0 * x0 + cx * cx + cz * cz) - (x0 * cx + z0 * cz) * 2.0f) - r * r);
    float disc = b * b - k * (a * 4.0f);
    if (0.0f >= disc)
        return false;
    float sq = (float)sqrt((double)disc);
    float t1 = (-b - sq) / (a * 2.0f);
    float t2 = (sq - b) / (a * 2.0f);
    if (t1 >= 1.0f)
        return false;
    if (0.0f >= t2)
        return false;
    if (1.0f > t2) {
        float nx = s[0] + t2 * dx;
        float nz = s[1] + t2 * dz;
        s[2] = nx;
        s[3] = nz;
    }
    if (t1 > 0.0f) {
        float nz = t1 * dz + s[1];
        float nx = t1 * dx + s[0];
        s[0] = nx;
        s[1] = nz;
    }
    return true;
}

// PANZERS 0x5514b0: the global waypoints from the A* points, skipping the
// points closer than the unit radius (SPUnit +0xec) to the previous waypoint
// and cutting the segments at that radius.
void SDriver::SetGlobalPathFromAStar(const void* astarp)
{
    const SAStar* astar = (const SAStar*)astarp;
    GlobalWayPoints.Clear(0);
    float cur[2] = { DU_F(Unit, 0x8c), DU_F(Unit, 0x94) };
    float startX = cur[0];
    float startZ = cur[1];
    int n = AStar_GetPointCount(astar);
    int i = 0;
    for (; i < AStar_GetPointCount(astar); ++i) {
        const float* p = AStar_GetPoint(astar, i);
        float dz = p[1] - cur[1];
        float dx = p[0] - cur[0];
        float r = DU_F(DU_P(Unit, 0x04), 0xec);
        if (r * r > dx * dx + dz * dz) {
            startX = p[0];
            startZ = p[1];
            continue;
        }
        float seg[4] = { startX, startZ, p[0], p[1] };
        ClipSegmentToCircle(seg, cur, r);
        int k = GlobalWayPoints.Add();
        GlobalWayPoints.Array[k].Z = seg[3];
        GlobalWayPoints.Array[k].X = seg[2];
        --i;
        startX = seg[2];
        startZ = seg[3];
        cur[0] = seg[2];
        cur[1] = seg[3];
    }
    (void)n;
    int last = i - 1;
    if (last < 0 || last >= AStar_GetPointCount(astar)) {
        DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SVector2", last);
        return;
    }
    const float* lp = AStar_GetPoint(astar, last);
    int k = GlobalWayPoints.Add();
    GlobalWayPoints.Array[k].X = lp[0];
    GlobalWayPoints.Array[k].Z = lp[1];
}

// PANZERS 0x551780: the local waypoints = the ghost position, then the A*
// points; resets the A*.
void SDriver::SetLocalPathFromAStar(const void* astarp, const float* start)
{
    SAStar* astar = (SAStar*)astarp;
    for (int i = -1; i < AStar_GetPointCount(astar); ++i) {
        int k = LocalWayPoints.Add();
        if (i == -1) {
            LocalWayPoints.Array[k].X = start[0];
            LocalWayPoints.Array[k].Z = start[2];
        } else {
            const float* p = AStar_GetPoint(astar, i);
            LocalWayPoints.Array[k].X = p[0];
            LocalWayPoints.Array[k].Z = p[1];
        }
    }
    AStar_Reset(astar);
}

// PANZERS 0x550d20 SDriver::CopyGlobalPathFromBossDriver
bool SDriver::CopyGlobalPathFromBossDriver()
{
    if (!Target)
        return false;
    SIUnit* boss = g_DriverEnv.GetMovementGroupBoss(DU_I(Unit, 0x254));
    SDriver* bd = boss ? DrvUnit_ActiveDriver(boss) : nullptr;
    if (!bd) {
        DrvPanic("SDriver::CopyGlobalPathFromBossDriver - no boss driver");
        return false;
    }
    if (bd->GlobalWayPoints.Size == 0)
        return false;
    ResetGhost();
    GlobalWayPoints.Clear(bd->GlobalWayPoints.Size);              // 0x54ff30
    for (int i = 0; i < GlobalWayPoints.Size; ++i)
        GlobalWayPoints.Array[i] = bd->GlobalWayPoints.Array[i];
    HasGlobalPath = bd->HasGlobalPath;
    float delta[2];
    ComputeGoal(delta);
    float fp[2];
    g_DriverEnv.GetMovementGroupUnitFormationPos(fp, boss);
    for (int i = 0; i < GlobalWayPoints.Size; ++i) {
        SVec2& g = GlobalWayPoints.Array[i];
        float x = delta[0] + (g.X - fp[0]);
        float z = delta[1] + (g.Z - fp[1]);
        float c[3];
        World_ClampToMap(g_World, c, x, 0.0f, z);
        g.X = c[0];
        g.Z = c[2];
    }
    return true;
}

// PANZERS 0x551c00: true when the driver must run FindGlobalPath (false
// when it has a global path or copied the boss driver's).
bool SDriver::NeedsGlobalPath()
{
    if (GlobalWayPoints.Size > 0)
        return false;
    if (SavedTarget)
        return true;
    int g = DU_I(Unit, 0x254);
    if (g > -1 && g_DriverEnv.GetMovementGroupConvoy(g))
        return true;
    SIUnit* boss = g_DriverEnv.GetMovementGroupBoss(g);
    STarget* t = Target;
    if (t && t->Path > -1) {
        if (!boss)
            return true;
        STarget* bt = (STarget*)DU_P(boss, 0x1f8);
        if (bt && bt->Path > -1 && t->PathPoint != bt->PathPoint)
            return true;
    }
    if (!boss || boss == Unit)
        return true;
    if (DU_P(Unit, 0x1f8) != DU_P(Unit, 0x1f4))
        return true;
    if (CopyGlobalPathFromBossDriver())
        return false;
    return true;
}

// PANZERS 0x551cb0 SDriver::FindGlobalPath
bool SDriver::FindGlobalPath(int size)
{
    PZ_M2_TRACE("SDriver::FindGlobalPath (0x551cb0)");
    if (!Target) {
        DrvPanic("SDriver::FindGlobalPath(): Target is NULL");
        return false;
    }
    if (Target->Type == kTargetTurn) {
        DrvPanic("SDriver::FindGlobalPath(): Target type not implemented");
        return false;
    }
    if (size == -1)
        size = DU_I(Unit, 0x5c);
    float delta[2];
    ComputeGoal(delta);
    Unit->SetOnBlockMap(false);
    float goal[2];
    g_DriverEnv.FindEmptySpace(goal, Goal[0], Goal[2], DU_F(Unit, 0x8c), DU_F(Unit, 0x94), size,
                               DU_S(Unit, 0xd8), false);
    Unit->SetOnBlockMap(true);
    float dz = goal[1] - DU_F(Unit, 0x94);
    float dx = goal[0] - DU_F(Unit, 0x8c);
    float r = DU_F(DU_P(Unit, 0x04), 0xec);
    if (r * r > dz * dz + dx * dx
        && BlockMap_LineFree(g_World, DU_F(Unit, 0x8c), DU_F(Unit, 0x94), goal[0], goal[1],
                             DU_I(Unit, 0x5c), DU_S(Unit, 0xd8))) {
        GlobalWayPoints.Clear(0);
        int k = GlobalWayPoints.Add();
        GlobalWayPoints.Array[k].X = goal[0];
        GlobalWayPoints.Array[k].Z = goal[1];
        HasGlobalPath = 1;
        return true;
    }
    int result = 0;
    AStar_FindPath(World_GetGlobalAStar(g_World), Unit, &result, DU_F(Unit, 0x8c),
                   DU_F(Unit, 0x94), goal[0], goal[1], size, DU_S(Unit, 0xd8), false);
    switch (result) {
    case 0:
    case 1:
        HasGlobalPath = 0;
        return false;
    case 2:
        HasGlobalPath = 1;
        SetGlobalPathFromAStar(World_GetGlobalAStar(g_World));
        return true;
    default:
        return true;
    }
}

// PANZERS 0x553100 SDriver::GetLocalPathGoalPoint: the first global
// waypoint; for the last one of a unit order, the refreshed target.
bool SDriver::GetLocalPathGoalPoint(float* out)
{
    if (GlobalWayPoints.Size == 0) {
        DrvPanic("SDriver::GetLocalPathGoalPoint - GlobalWayPointsArray.GetSize() == 0");
        return false;
    }
    out[0] = GlobalWayPoints.Array[0].X;
    out[1] = GlobalWayPoints.Array[0].Z;
    if (GlobalWayPoints.Size != 1)
        return false;
    if (Target->IsUnitOrder()) {
        Target->Refresh(DU_I(Unit, 0x74));
        out[0] = Target->Pos[0];
        out[1] = Target->Pos[2];
    }
    return true;
}

// PANZERS 0x552050 SDriver::FindLocalPath (vtable +0x34)
bool SDriver::FindLocalPath(SGhostFrame* f)
{
    PZ_M2_TRACE("SDriver::FindLocalPath (0x552050)");
    UpdateTurnRadius();
    float goal[2] = { 0.0f, 0.0f };
    bool last = GetLocalPathGoalPoint(goal);
    unsigned short mask = (unsigned short)(DU_S(Unit, 0xd8) | 0x80);
    DrvUnit_MarkNear(Unit, true, AvoidUnit, CollisionPos[0], CollisionPos[1]);
    Unit->SetOnBlockMap(false);
    float out[2];
    if (LocalPathFailures > 0 && last && Target->Type == kTargetUnit) {
        float a = DAtan2f((double)(Target->Pos[0] - f->X), (double)(Target->Pos[2] - f->Z));
        int seed = (int)DU_U(g_World, 0x7518) * 0x343fd + 0x269ec3;
        DU_I(g_World, 0x7518) = seed;
        int r = (seed >> 0x10) & 0x7fff;
        float dir = (float)((double)r * 3.0517578125e-05 * kHdTwoPi + (double)a);
        g_DriverEnv.FindEmptySpaceDir(out, Target->Pos[0], Target->Pos[2], dir, DU_I(Unit, 0x5c),
                                      mask, true);
    } else if (Target->Type == kTargetUnit) {
        g_DriverEnv.FindEmptySpace(out, goal[0], goal[1], f->X, f->Z, DU_I(Unit, 0x5c), mask, true);
    } else {
        double a = DAtan2((double)(goal[0] - f->X), (double)(goal[1] - f->Z));
        double d = (double)(float)a + (double)(LocalPathFailures - 1) * 1.5707963705062866;
        d = WrapPi(d);
        g_DriverEnv.FindEmptySpaceDir(out, goal[0], goal[1], (float)d, DU_I(Unit, 0x5c), mask,
                                      true);
    }
    goal[0] = out[0];
    goal[1] = out[1];
    Unit->SetOnBlockMap(true);
    bool ok = true;
    if (BlockMap_LineFree(g_World, f->X, f->Z, goal[0], goal[1], DU_I(Unit, 0x5c), mask)) {
        int k = LocalWayPoints.Add();
        LocalWayPoints.Array[k].X = f->X;
        LocalWayPoints.Array[k].Z = f->Z;
        k = LocalWayPoints.Add();
        LocalWayPoints.Array[k].X = goal[0];
        LocalWayPoints.Array[k].Z = goal[1];
        InitLocalWayPointsArray(f, last);
        HasLocalPath = 1;
    } else {
        int result = 0;
        AStar_FindPath(World_GetLocalAStar(g_World), Unit, &result, f->X, f->Z, goal[0], goal[1],
                       DU_I(Unit, 0x5c), mask, false);
        if (result >= 0) {
            if (result <= 1) {
                ok = false;
            } else if (result == 2) {
                HasLocalPath = 1;
                SetLocalPathFromAStar(World_GetLocalAStar(g_World), &f->X);
                InitLocalWayPointsArray(f, last);
            }
        }
    }
    DrvUnit_MarkNear(Unit, false, AvoidUnit, CollisionPos[0], CollisionPos[1]);
    AvoidUnit = -1;
    return ok;
}

// PANZERS 0x556ac0 SDriver::InitLocalWayPointsArray: point types, the in
// and out directions and the next-point data of the local waypoints from
// the ghost frame's waypoint on, then drops duplicated points.
void SDriver::InitLocalWayPointsArray(const SGhostFrame* f, bool lastStops)
{
    if (!Target) {
        DrvPanic("SDriver::InitLocalWayPointsArray(): Target is NULL");
        return;
    }
    SHdDArray<SWayPointWithManoeuvres>& L = LocalWayPoints;
    for (int i = f->WayPointIdx; i < L.Size; ++i) {
        SWayPointWithManoeuvres& w = L.At(i);
        if (i == f->WayPointIdx)
            w.PointType = kLwpFirst;
        else if (i == L.Size - 1)
            w.PointType = lastStops ? kLwpLastStop : kLwpLastPass;
        else if (i == L.Size - 2)
            w.PointType = kLwpBeforeLast;
        else
            w.PointType = kLwpMiddle;

        if (i < L.Size - 1) {
            float dx = L.At(i + 1).X - w.X;
            float dz = L.At(i + 1).Z - w.Z;
            if (1e-4 > fabs((double)dx) && 1e-4 > fabs((double)dz)) {
                if (i == 0) {
                    double d = (double)f->Dir;
                    if (Target->Reverse)
                        d = WrapPi(d + kHdPi);
                    L.At(0).OutDir = (float)d;
                } else {
                    L.At(i).OutDir = L.At(i - 1).OutDir;
                }
            } else {
                w.OutDir = DAtan2f((double)dx, (double)dz);
            }
        } else if (lastStops && Target->Type == kTargetPosDir) {
            double d = (double)Target->Dir;
            if (Target->Reverse)
                d = WrapPi(d + kHdPi);
            w.OutDir = (float)d;
        } else {
            w.OutDir = L.At(i - 1).OutDir;
        }

        if (i == f->WayPointIdx) {
            double d = (double)f->Dir;
            if (Target->Reverse)
                d = WrapPi(d + kHdPi);
            w.InDir = (float)d;
        } else {
            w.InDir = L.At(i - 1).OutDir;
        }
    }
    for (int i = f->WayPointIdx; i < L.Size - 1; ++i) {
        L.At(i).NextX = L.At(i + 1).X;
        L.At(i).NextZ = L.At(i + 1).Z;
        L.At(i).NextOutDir = L.At(i + 1).OutDir;
    }
    int i = 0;
    while (i < L.Size - 1) {
        if (L.At(i).X == L.At(i + 1).X && L.At(i).Z == L.At(i + 1).Z) {
            Wpm_Free(&L.Array[i]);                                // 0x559810
            L.Remove(i);
        } else {
            ++i;
        }
    }
}

// PANZERS 0x55aa80: Goal (+0xa8) = the target position, offset by the
// unit's formation position when it moves in a (non-convoy) movement group;
// outDelta = Goal - TargetPos (x, z).
void SDriver::ComputeGoal(float* outDelta)
{
    STarget* t = Target;
    TargetPos[0] = t->Pos[0];
    TargetPos[1] = t->Pos[1];
    TargetPos[2] = t->Pos[2];
    int g = DU_I(Unit, 0x254);
    bool simple = g == -1 || SavedTarget != nullptr;
    if (!simple && g_DriverEnv.GetMovementGroupConvoy(g))
        simple = true;
    if (!simple) {
        int k = t->Kind;
        if (k == 9 || k == 10 || k == 0x10 || k == 0xe || k == 0xf)
            simple = true;
    }
    if (simple) {
        Goal[0] = t->Pos[0];
        Goal[1] = t->Pos[1];
        Goal[2] = t->Pos[2];
        outDelta[0] = 0.0f;
        outDelta[1] = 0.0f;
        return;
    }
    float fp[2];
    g_DriverEnv.GetMovementGroupUnitFormationPos(fp, Unit);
    t = Target;
    float gx, gy, gz;
    if (t->Path > -1) {
        float a;
        if (!t->GetPathNodeDir(&a))
            a = DAtan2f((double)(Target->Pos[0] - DU_F(Unit, 0x8c)),
                               (double)(Target->Pos[2] - DU_F(Unit, 0x94)));
        if (t->PathPoint == 0) {
            float fd = g_DriverEnv.GetMovementGroupFormationDir(DU_I(Unit, 0x254));
            if (fd == 0.0f)
                g_DriverEnv.SetMovementGroupFormationDir(DU_I(Unit, 0x254), a);
            t = Target;
            gy = t->Pos[1] + 0.0f;
            gx = t->Pos[0] + fp[0];
            gz = t->Pos[2] + fp[1];
        } else {
            float fd = g_DriverEnv.GetMovementGroupFormationDir(DU_I(Unit, 0x254));
            float d = a - fd;
            float c = (float)DCos((double)d);
            double s = DSin((double)d);
            float fs = (float)s;
            float ns = (float)(-s);
            t = Target;
            float rx = fs * fp[1] + c * fp[0];
            float rz = ns * fp[0] + c * fp[1];
            gx = t->Pos[0] + rx;
            gz = t->Pos[2] + rz;
            gy = 0.0f;
        }
    } else if (t->Type == kTargetPosDir) {
        // HD builds a 2D rotation (0x557a40) and multiplies it (0x661bb0);
        // the branch did not run in the menu. Rotate fp by the target dir
        // relative to the unit, as HD's matrices do.
        DrvPanic("SDriver::ComputeGoal: formation goal for TARGET_POS_DIR not lifted (0x557a40/0x661bb0)");
        gx = t->Pos[0] + fp[0];
        gy = t->Pos[1] + 0.0f;
        gz = t->Pos[2] + fp[1];
    } else {
        gy = t->Pos[1] + 0.0f;
        gx = t->Pos[0] + fp[0];
        gz = t->Pos[2] + fp[1];
    }
    Goal[0] = gx;
    Goal[1] = gy;
    Goal[2] = gz;
    float c[3];
    World_ClampToMap(g_World, c, Goal[0], Goal[1], Goal[2]);
    Goal[0] = c[0];
    Goal[1] = c[1];
    Goal[2] = c[2];
    outDelta[0] = Goal[0] - TargetPos[0];
    outDelta[1] = Goal[2] - TargetPos[2];
}

// PANZERS 0x55b1d0: the turning radius (+0xcc) and the arrival radius
// (+0x74) of the driver type.
void SDriver::UpdateTurnRadius()
{
    SPDriver* pd = PDriver;
    int type = pd->Type;
    float tr;
    if (type == 1) {
        tr = pd->Wheelbase / pd->WheelTurnAngle;
    } else if (type == 3) {
        tr = (pd->Wheelbase / pd->WheelTurnAngle) * 2.0f;
        TurnRadius = tr;
        ArrivalRadius = tr / 3.0f;
        return;
    } else if (type == 10) {
        TurnRadius = 1.0f;
        ArrivalRadius = 1.0f;
        return;
    } else if (type == 13) {
        TurnRadius = 1.0f;
        ArrivalRadius = 10.0f;
        return;
    } else {
        float ts = GetTurnSpeed();
        if (ts == 0.0f) {
            TurnRadius = 1.0f;
        } else {
            float ms = GetMaxSpeed();
            TurnRadius = ms / GetTurnSpeed();
        }
        tr = TurnRadius;
    }
    ArrivalRadius = 1.0f;
    TurnRadius = tr * 1.1f;
}

} // namespace pz
