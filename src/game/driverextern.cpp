// src/game/driverextern.cpp
// Agent P's definitions of the unit hooks declared in unitextern.h (agent
// U). Defining them replaces U's weak defaults. OWNER: P.

#include <stddef.h>
#include <string.h>
#include "unitextern.h"
#include "driver.h"
#include "target.h"
#include "world.h"
#include "worldapi.h"
#include "blockmap.h"

namespace pz {
struct SUPropStruct;

static float BitsF(int i)
{
    float f;
    memcpy(&f, &i, 4);
    return f;
}

static_assert(offsetof(STarget, Type) == tgt::kType, "tgt layout");
static_assert(offsetof(STarget, Pos) == tgt::kPos, "tgt layout");
static_assert(offsetof(STarget, Kind) == tgt::kKind, "tgt layout");
static_assert(offsetof(STarget, Reverse) == tgt::kFlag2C, "tgt layout");
static_assert(offsetof(STarget, Path) == tgt::kPath, "tgt layout");
static_assert(offsetof(STarget, PathPoint) == tgt::kPathPt, "tgt layout");
} // namespace pz

extern "C" {

// HD SPUnit::InitDrivers 0x5a6f10: new SP*Driver by DriverType, then +0x04 Load.
pz::SIPDriver* PzCreatePDriver(int driverType, pz::SUPropStruct* driverStruct, const char* unitName)
{
    pz::SIPDriver* p = pz::CreatePDriver(driverType);
    if (p)
        p->Load((pz::SProperties*)driverStruct, unitName, unitName ? (int)strlen(unitName) : 0);
    return p;
}

pz::STarget* PzTargetNew(int kind)
{
    return pz::STarget::Create(kind);
}

void PzTargetRelease(pz::STarget* t)
{
    if (t)
        t->Release();
}

bool PzTargetConsumePath(pz::STarget* t)
{
    return t->ConsumePath();
}

bool PzTargetRefresh(pz::STarget* t, int unitIndex)
{
    return t->Refresh(unitIndex);
}

void PzBlockMapSetUnit(float x, float z, int size, bool on)
{
    pz::BlockMap_MarkDynamic(pz::g_World, x, z, size, on);              // 0x5f4430
}

void PzBlockMapMark(int p1, int p2, int size, bool on, short flags)
{
    pz::BlockMap_MarkStatic(pz::g_World, pz::BitsF(p1), pz::BitsF(p2), size, on,
                            (unsigned)(unsigned short)flags);           // 0x5f4720
}

int PzBlockMapTestPath(int p1, int p2, int p4, short flags, int p6, int p7)
{
    return pz::BlockMap_CheckStaticCell(pz::g_World, pz::BitsF(p1), pz::BitsF(p2), p4,
                                        (unsigned)(unsigned short)flags, (int*)(size_t)p6,
                                        (int*)(size_t)p7) ? 1 : 0;      // 0x5d9eb0
}

int PzBlockMapTest(int p1, int p2, int size, short flags)
{
    return pz::BlockMap_CheckStatic(pz::g_World, pz::BitsF(p1), pz::BitsF(p2), size,
                                    (unsigned)(unsigned short)flags) ? 1 : 0;   // 0x5d9e00
}

bool PzBlockMapTestUnits(float x, float z, int size)
{
    return pz::BlockMap_CheckDynamic(pz::g_World, x, z, size);          // 0x5d99f0
}

void PzDriverReset(void* driver, bool dropTargets)
{
    if (driver)
        static_cast<pz::SDriver*>(static_cast<pz::SIDriver*>(driver))->Stop(dropTargets);   // 0x55bf50
}

} // extern "C"
