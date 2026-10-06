// src/game/unitextern.cpp
// Weak defaults of the cross-agent hooks in unitextern.h and punit.h. OWNER:
// agent U. Each default is bound with /alternatename, so the owner's real
// definition (anywhere in the link) wins without touching this file.
//
// The directives only reach the linker when this object is linked: SUnit's
// ctor references UnitExternLinked() for that.

#include <stdlib.h>
#include <string.h>
#include "unitextern.h"
#include "punit.h"
#include "driver.h"
#include "unitprops.h"
#include "logger.h"
#include "doodad.h"
#include "world.h"
#include "worldapi.h"

#define PZ_WEAK(NAME) __pragma(comment(linker, "/alternatename:_" #NAME "=_" #NAME "_Default"))

namespace pz {
void UnitExternLinked() {}
}

extern "C" {

// ---- P ------------------------------------------------------------------

// HD operator new(0x38) + STarget::STarget 0x5b27c0(kind).
pz::STarget* PzTargetNew_Default(int kind)
{
    using namespace pz;
    STarget* t = (STarget*)operator new(0x38);
    memset(t, 0, 0x38);
    tgt::I(t, tgt::kKind) = kind;
    tgt::I(t, tgt::kUnit) = -1;
    tgt::I(t, tgt::kUnit2) = -1;
    tgt::I(t, tgt::kPath) = -1;
    tgt::I(t, tgt::kPathPt) = -1;
    return t;
}
PZ_WEAK(PzTargetNew)

void PzTargetRelease_Default(pz::STarget* t)
{
    using namespace pz;
    if (t && --tgt::I(t, tgt::kRefCount) == 0)
        operator delete(t);                                  // 0x76654a(t, 0x38)
}
PZ_WEAK(PzTargetRelease)

// HD STarget::ConsumePath 0x5b7c50 (P owns it; the default is that body):
// the next point of the PATH at +0x30 (wrapping on a closed path) into
// +0x10..+0x18 with the terrain height; false past the end of an open path.
bool PzTargetConsumePath_Default(pz::STarget* t)
{
    using namespace pz;
    int path = tgt::I(t, tgt::kPath);
    if (path < 0)
        Logger.g->Panic("STarget::ConsumePath: No path");
    if (!g_World->Paths.IsLive(path))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "struct SPath", path);
    const SPath& p = g_World->Paths.Array[path].Data;
    int& pt = tgt::I(t, tgt::kPathPt);
    pt++;
    if (p.Points.Size <= pt) {
        if (!p.Closed)
            return false;
        pt = 0;
    }
    if (pt < 0 || pt >= p.Points.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SPathPoint", pt);
    float x = p.Points.Array[pt].X, z = p.Points.Array[pt].Z;
    tgt::F(t, tgt::kPos) = x;
    tgt::F(t, tgt::kPos + 4) = g_World->GetTerrainHeight(x, z);
    tgt::F(t, tgt::kPos + 8) = z;
    return true;
}
PZ_WEAK(PzTargetConsumePath)

// Without P's STarget::Refresh the target stays valid (units keep their order).
bool PzTargetRefresh_Default(pz::STarget* t, int unitIndex)
{
    (void)t; (void)unitIndex;
    return true;
}
PZ_WEAK(PzTargetRefresh)

void PzBlockMapSetUnit_Default(float x, float z, int size, bool on)
{
    (void)x; (void)z; (void)size; (void)on;
}
PZ_WEAK(PzBlockMapSetUnit)

void PzBlockMapMark_Default(int p1, int p2, int size, bool on, short flags)
{
    (void)p1; (void)p2; (void)size; (void)on; (void)flags;
}
PZ_WEAK(PzBlockMapMark)

int PzBlockMapTestPath_Default(int p1, int p2, int p4, short flags, int p6, int p7)
{
    (void)p1; (void)p2; (void)p4; (void)flags; (void)p6; (void)p7;
    return 0;
}
PZ_WEAK(PzBlockMapTestPath)

int PzBlockMapTest_Default(int p1, int p2, int size, short flags)
{
    (void)p1; (void)p2; (void)size; (void)flags;
    return 0;
}
PZ_WEAK(PzBlockMapTest)

bool PzBlockMapTestUnits_Default(float x, float z, int size)
{
    (void)x; (void)z; (void)size;
    return false;
}
PZ_WEAK(PzBlockMapTestUnits)

void PzDriverReset_Default(void* driver, bool dropTargets)
{
    (void)driver; (void)dropTargets;
}
PZ_WEAK(PzDriverReset)

} // extern "C"

namespace pz {

// Stand-in driver prototype and driver until agent P's driver classes land
// (recompile only). They derive from P's logged skeleton SPDriver / SDriver,
// keep the HD prototype fields the unit and animation code read (+0x04
// DriverType, +0x08 MoveSpeed, +0x0c SpinSpeed) and do not move the unit.
struct SStandInPDriver : SPDriver {
    int   DriverType;    // +0x04
    float MoveSpeed;     // +0x08
    float SpinSpeed;     // +0x0c
    SIDriver* CreateDriver(SIUnit* unit) override;
};

struct SStandInDriver : SDriver {
    SStandInPDriver* P;
    SIPDriver* GetPDriver() override { return P; }
    void Init() override {}
    void Refresh() override {}
    void RefreshTarget(int) override {}
};

SIDriver* SStandInPDriver::CreateDriver(SIUnit* unit)
{
    (void)unit;
    SStandInDriver* d = new SStandInDriver();
    d->P = this;
    // The SPanzersSquadMemberDriver ctor (0x54f970) draws the world seed
    // twice (its speed factor); kept so the seed sequence, the formations
    // drawn after it and the world CRC stay HD's until P's driver lands.
    if (DriverType == 11) {
        for (int k = 0; k < 2; ++k)
            g_World->RandomSeed = g_World->RandomSeed * 0x343fdu + 0x269ec3u;
    }
    return d;
}

} // namespace pz

extern "C" {

pz::SIPDriver* PzCreatePDriver_Default(int driverType, pz::SUPropStruct* driverStruct, const char* unitName)
{
    using namespace pz;
    (void)unitName;
    if (driverType == 2 || driverType == 4 || driverType > 13) {
        if (Logger.g)
            Logger.g->Warning("SPUnit::InitDrivers - Unknown drivertype");
        return nullptr;
    }
    SStandInPDriver* p = new SStandInPDriver();
    p->DriverType = driverType;
    p->MoveSpeed = 0.0f;
    p->SpinSpeed = 0.0f;
    SUPropStruct* sub = driverStruct->GetMultiSubStruct("DriverType");
    for (SUProp* c : sub->Children) {
        if (c && c->Type() == UPROP_EXPR && c->Name == "MoveSpeed")
            p->MoveSpeed = static_cast<SUPropExpr*>(c)->Value;
        if (c && c->Type() == UPROP_EXPR && c->Name == "SpinSpeed")
            p->SpinSpeed = static_cast<SUPropExpr*>(c)->Value;
    }
    return p;
}
PZ_WEAK(PzCreatePDriver)

// ---- L ------------------------------------------------------------------

void PzGameLogicUnitCreated_Default(int unitIndex)
{
    (void)unitIndex;
}
PZ_WEAK(PzGameLogicUnitCreated)

} // extern "C"
