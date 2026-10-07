// src/game/unit_afterload.cpp
// The after-load slots of the unit classes (SIUnit +0x14, called by the UNIS
// loader 0x5f3820 once every unit is loaded; SUnit's own is in
// unitsave.cpp). The board elements HD makes again here (the unit labels
// and icons, board +0x08 / +0x18 / +0x24 / +0x34) are not created by the
// recompile, as in the classes' Init. OWNER: agent S (M4).

#include <stdlib.h>
#include <string.h>
#include "unit.h"
#include "buildingunit.h"
#include "squadunit.h"
#include "world.h"
#include "worldapi.h"
#include "blockmap.h"
#include "blockmaprefresh.h"
#include "pz/imodel.h"
#include "logger.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {

// SIModel +0xac (0x6d8090), as buildingunit.cpp's static helper.
static int ModelNodePoints(SIModel* model, const char* node, float (**out)[3])
{
    SVec3Array a = { nullptr, 0, 0 };
    model->GetNodePoints(node, &a);
    *out = a.Array;
    return a.Size;
}

// HD 0x661b30 + operator delete(0x1c), as buildingunit.cpp's static helper.
static void BlockBitmapFree(SBlockBitmap* bm)
{
    delete[] bm->Bits;
    bm->Bits = nullptr;
    delete bm;
}

// PANZERS 0x549500
// The footprints go on the static block map again (an old "Block" bitmap is
// taken off and freed first), then the view points, the capture flag node
// and +0x454, as SBuildingUnit::Init 0x548f20 does.
void SBuildingUnit::Slot_14()
{
    SUnit::Slot_14();                                             // 0x5bb1c0
    // HD: board +0x08(4, ...) -> +0x354, +0x350 (not created).
    if (BlockNode) {
        BlockMap_ApplyBitmap(g_World, BlockNode, false, 4);       // 0x5f4910(+0x344, 0, 4)
        BlockBitmapFree(BlockNode);                               // 0x661b30 + delete 0x1c
        BlockNode = nullptr;
    }
    BlockNode = Model->BuildNodeBlockBitmap(4, "Block");          // +0xa8
    BlockMap_ApplyBitmap(g_World, BlockNode, true, 4);
    Model->SetNodeVisible(Model->FindNode("Block"), false);       // +0x40 / +0x60
    if (P->BuildingType == 6) {
        IndoorNode = Model->BuildNodeBlockBitmap(4, "Indoor");
        BlockMap_ApplyBitmap(g_World, IndoorNode, true, 0x8000);
        Model->SetNodeVisible(Model->FindNode("Indoor"), false);
    }
    if (P->UnitType == 0x1a) {
        ProductiveIndoorNode = Model->BuildNodeBlockBitmap(4, "Indoor");
        BlockMap_ApplyBitmap(g_World, ProductiveIndoorNode, true, 0x800);
        Model->SetNodeVisible(Model->FindNode("Indoor"), false);
    }
    if (P->BuildingType == 3) {
        FlagNode = Model->FindNode("flag");                       // +0x44c (0x7f436c)
        STUB_LOG("SBuildingUnit 0x5471d0 (the capture flag model), called by SBuildingUnit after load 0x549500");
    }
    InitViewPoints();                                             // 0x548660
    OnStaticBlock = false;                                        // +0x454
    float (*pts)[3] = nullptr;
    int n = ModelNodePoints(Model, "Block", &pts);                // +0xac
    for (int i = 0; i < n; ++i) {
        int x, z;
        memcpy(&x, &pts[i][0], 4);
        memcpy(&z, &pts[i][2], 4);
        if (SUnit::TestBlockMap(x, z, 0, 1, 1)) {                 // 0x5b7500
            OnStaticBlock = true;
            break;
        }
    }
    free(pts);
}

void SquadBoardAfterLoad(SPanzersSquadUnit* s);                  // unitboard.cpp (0x59cf80 board part)

// PANZERS 0x59cf80
// After load: the unit size, SUnit 0x5bb1c0, then the squad's board elements
// again (+0x380, +0x360.., +0x370..; HD does not release old ones).
void SPanzersSquadUnit::Slot_14()
{
    SetUnitSize();                                                // +0x1c4 (0x5a0f30)
    SUnit::Slot_14();                                             // 0x5bb1c0
    SquadBoardAfterLoad(this);
}

// PANZERS 0x598980
// A member that was already gone (+0x156) is frozen and removed.
void SPanzersSquadMemberUnit::Slot_14()
{
    SUnit::Slot_14();                                             // 0x5bb1c0
    if (_156) {
        Frozen = true;                                            // +0x153
        g_World->RemoveUnit(WorldIndex);                          // 0x5f8060(+0x74)
    }
}

} // namespace pz
