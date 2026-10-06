// src/game/buildingunit.cpp
// SBuildingUnit (0x545940..0x5500a0). OWNER: agent U. See buildingunit.h.

#include <string.h>
#include "buildingunit.h"
#include "unitextern.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/imodel.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"

namespace pz {

// PANZERS 0x545940
// HD also loads (GetPUnit(name, true)) the unit types every production slot
// of a productive building names (prototype +0x154, 0x68 bytes each).
SBuildingUnit::SBuildingUnit(SPBuildingUnit* proto, int worldIndex)
    : SUnit(proto, worldIndex)
{
    memset(&P, 0, sizeof(SBuildingUnit) - offsetof(SBuildingUnit, P));
    P = proto;
    for (int i = 0; i < 12; ++i)
        _2cc[i] = true;                                       // +0x2cc[i] = 1, +0x43c[i] = 0
    if (proto->Products.Size > 0)
        STUB_LOG("SBuildingUnit::SBuildingUnit (0x545940) productive building");
}

// PANZERS 0x5460c0
SBuildingUnit::~SBuildingUnit()
{
}

// PANZERS 0x548f20
// Lifted: base Init, the hidden "Block" / "Indoor" nodes. Not lifted: the
// node footprints on the static block map (model +0xa8, world 0x5f4910),
// the capture flag (type 3, 0x5471d0), the "Platform" rebuild and the
// "Block" point test (+0x454); the board elements.
void SBuildingUnit::Init(SUnitDef* def)
{
    SUnit::Init(def);                                         // 0x5ba8e0
    _110 = true;
    if (Model) {
        Model->SetNodeVisible(Model->FindNode("Block"), false);   // +0x40 / +0x60
        if (P->BuildingType == 6 || P->UnitType == 0x1a)
            Model->SetNodeVisible(Model->FindNode("Indoor"), false);
    }
    if (P->BuildingType == 3)
        STUB_LOG("SBuildingUnit::Init (0x548f20) capture flag");
    if (g_GameLogic)
        RefreshTargeting();                                   // +0x34
}

// PANZERS 0x5468a0
void SBuildingUnit::StoreInterpolationState()
{
    SUnit::StoreInterpolationState();                         // 0x5b5ad0
    if (Model448)
        Model448->StoreInterpolationState();                  // +0x3c
}

// PANZERS 0x549b10
void SBuildingUnit::SetOnBlockMap(bool on)
{
    (void)on;
}

// PANZERS 0x549b20
void SBuildingUnit::MarkBlockMap(bool on, int p2, int p3, int p4, short p5)
{
    (void)on; (void)p2; (void)p3; (void)p4; (void)p5;
}

// PANZERS 0x546ad0
int SBuildingUnit::TestBlockMap(int p1, int p2, int p3, int p4, short p5)
{
    (void)p1; (void)p2; (void)p3; (void)p4; (void)p5;
    return 0;
}

} // namespace pz
