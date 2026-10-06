// src/game/target.cpp
// pz::STarget (HD 0x38 bytes). OWNER: P.

#include <math.h>
#include <float.h>
#include "target.h"
#include "driverunit.h"
#include "iunit.h"
#include "world.h"
#include "worldapi.h"
#include "blockmap.h"
#include "drivermath.h"

namespace pz {

// PANZERS 0x5b27c0 (with the new 0x38 of the callers)
STarget* STarget::Create(int kind)
{
    STarget* t = (STarget*)::operator new(sizeof(STarget));
    t->Pos[0] = 0.0f;
    t->Pos[1] = 0.0f;
    t->Pos[2] = 0.0f;
    t->Kind = kind;
    t->RefCount = 0;
    t->Unit = -1;
    t->Unit2 = -1;
    t->Dir = 0.0f;
    t->Mode = 0;
    t->Type = 0;     // HD leaves +0x04 as allocated; the callers set it
    t->Field28 = 0;
    t->Reverse = false;
    t->_2d[0] = t->_2d[1] = t->_2d[2] = 0;
    t->Path = -1;
    t->PathPoint = -1;
    return t;
}

// PANZERS 0x5bdef0
void STarget::Release()
{
    if (--RefCount == 0)
        ::operator delete(this);
}

// PANZERS 0x5bb440
bool STarget::IsUnitOrder() const
{
    return Type == kTargetUnit && (Kind == 0 || Kind == 6 || Kind == 8 || Kind == 7);
}

// PANZERS 0x5c1860
void STarget::SetGroundPos(const float* xz)
{
    Type = kTargetPath;
    float x = xz[0];
    float y = g_DriverEnv.TerrainHeight(x, xz[1]);
    Pos[0] = x;
    Pos[1] = y;
    Pos[2] = xz[1];
}

// PANZERS 0x5ba0b0 STarget::GetPathNodeDir: the direction of the path
// segment that ends at PathPoint.
bool STarget::GetPathNodeDir(float* out) const
{
    if (Path < 0) {
        DrvPanic("STarget::GetPathNodeDir: No path");
        return false;
    }
    SHdPath* p = HdPath(Path);
    if (!p)
        return false;
    float prevf = (float)(PathPoint - 1);
    if (0.0f > prevf)
        return false;
    int prev = (int)prevf;
    if (prev < 0 || prev >= p->Count || PathPoint < 0 || PathPoint >= p->Count) {
        DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SVector2", PathPoint);
        return false;
    }
    float dx = p->Points[PathPoint].X - p->Points[prev].X;
    float dz = p->Points[PathPoint].Z - p->Points[prev].Z;
    *out = DAtan2f((double)dx, (double)dz);
    return true;
}

// PANZERS 0x5bb630
bool STarget::HasPathAhead() const
{
    if (Path < 0)
        return false;
    SHdPath* p = HdPath(Path);
    if (!p)
        return false;
    return PathPoint != p->Count - 1 || p->Loop;
}

// PANZERS 0x5b7c50
bool STarget::ConsumePath()
{
    PZ_M2_TRACE("STarget::ConsumePath (0x5b7c50)");
    if (Path < 0) {
        DrvPanic("STarget::ConsumePath: No path");
        return false;
    }
    SHdPath* p = HdPath(Path);
    if (!p)
        return false;
    ++PathPoint;
    if (PathPoint >= p->Count) {
        if (!p->Loop)
            return false;
        PathPoint = 0;
    }
    if (PathPoint < 0 || PathPoint >= p->Count) {
        DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SVector2", PathPoint);
        return false;
    }
    float x = p->Points[PathPoint].X;
    float z = p->Points[PathPoint].Z;
    float y = g_DriverEnv.TerrainHeight(x, z);
    Pos[0] = x;
    Pos[1] = y;
    Pos[2] = z;
    return true;
}

// PANZERS 0x5bd210 STarget::Refresh(viewer): moves Pos to the target unit
// (or a building's entrance) while that unit can be seen; false when the
// target is gone or hidden.
bool STarget::Refresh(int viewerUnit)
{
    PZ_M2_TRACE("STarget::Refresh (0x5bd210)");
    int use = -1;
    if (Type == kTargetUnit) {
        if (!HdUnitLive(Unit))
            return false;
        SIUnit* t = HdUnit(Unit);
        if (DU_B(t, 0x150) != 0)
            return false;
        if (DU_B(t, 0x168) != 0)
            return false;
        SIUnit* viewer = HdUnit(viewerUnit);
        bool ok = g_DriverEnv.CanSeeGroundUnit(DU_I(viewer, 0xfc), t)
            || DU_ClassType(viewer) == 7
            || DrvUnit_IsOffStaticMap(t)
            || DU_ClassType(t) == 6
            || DU_ClassType(t) == 9;
        if (!ok)
            return false;
        if (DU_ClassType(t) != 9) {
            use = Unit;
        } else if (Kind == 5 || Kind == 9 || Kind == 0) {
            float tmp[3];
            float* e = g_DriverEnv.UnitGetEntrance(t, tmp);
            Pos[0] = e[0];
            Pos[1] = e[1];
            Pos[2] = e[2];
        } else {
            int mode = DU_I(DU_P(t, 0x340), 0x13c);
            if (mode != 0 && mode != 2) {
                use = Unit;
            } else {
                Pos[0] = DU_F(t, 0x3ac);
                Pos[1] = DU_F(t, 0x358);
                Pos[2] = DU_F(t, 0x3b0);
            }
        }
    } else if (Type == kTargetUnitByUnit) {
        if (!HdUnitLive(Unit2))
            return false;
        SIUnit* t2 = HdUnit(Unit2);
        if (DU_B(t2, 0x150) != 0)
            return false;
        if (DU_B(t2, 0x168) != 0)
            return false;
        use = Unit2;
        if (HdUnitLive(Unit)) {                         // 0x573790 SHeapTRB::IsLive
            SIUnit* t = HdUnit(Unit);
            if (DU_B(t, 0x150) == 0 && DU_B(t, 0x168) == 0)
                use = Unit;
        }
    }
    if (use >= 0) {
        SIUnit* u = HdUnit(use);
        Pos[0] = DU_F(u, 0x8c);
        Pos[1] = DU_F(u, 0x90);
        Pos[2] = DU_F(u, 0x94);
    }
    if (!_finite((double)Pos[0]) || !_finite((double)Pos[1]) || !_finite((double)Pos[2])) {
        DrvPanic("STarget::Refresh(): Target Point  is not finite!");
        return false;
    }
    float c[3];
    World_ClampToMap(g_World, c, Pos[0], Pos[1], Pos[2]);
    Pos[0] = c[0];
    Pos[1] = c[1];
    Pos[2] = c[2];
    return true;
}

} // namespace pz
