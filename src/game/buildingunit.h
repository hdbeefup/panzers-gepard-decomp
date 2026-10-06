// src/game/buildingunit.h
// SBuildingUnit (vftable 0x7f4184, 0x470 bytes): houses, towers, capturable
// buildings (0x545940..0x5500a0). OWNER: agent U.
//
// The menu has one, the "FR village long stone" house (UNDS slot 0). Only
// what it executes is lifted: construction, Init (node hiding), the
// interpolation store of the second model. Entering, occupants, capture
// flags and productive buildings are not.

#ifndef PZ_GAME_BUILDINGUNIT_H
#define PZ_GAME_BUILDINGUNIT_H

#include "unit.h"

namespace pz {

struct SBuildingUnit : SUnit {
    SBuildingUnit(SPBuildingUnit* proto, int worldIndex);        // 0x545940
    ~SBuildingUnit() override;                                   // 0x5464d0 / 0x5460c0
    void Init(SUnitDef* def) override;                           // 0x548f20
    void StoreInterpolationState() override;                     // 0x5468a0
    void SetOnBlockMap(bool on) override;                        // 0x549b10 (buildings use the static block map)
    void MarkBlockMap(bool on, int p2, int p3, int p4, short p5) override;   // 0x549b20
    int TestBlockMap(int p1, int p2, int p3, int p4, short p5) override;    // 0x546ad0

    SPBuildingUnit* P;               // +0x340
    void*    BlockNode;              // +0x344 model +0xa8(4, "Block") footprint
    void*    IndoorNode;             // +0x348 (building type 6)
    void*    ProductiveIndoorNode;   // +0x34c (unit type 0x1a)
    int      Board350;               // +0x350 board elements (not created)
    int      Board354;               // +0x354
    float    InsideY;                // +0x358 y of the units inside
    unsigned char _35c[0x448 - 0x35c];
    SIModel* Model448;               // +0x448
    int      FlagNode;               // +0x44c (capturable: "flag" node)
    unsigned char _450[0x454 - 0x450];
    bool     OnStaticBlock;          // +0x454 a "Block" point is on the static block map
    unsigned char _455[0x470 - 0x455];
};

PZ_HD_SIZE(SBuildingUnit, kHdSizeSBuildingUnit);

} // namespace pz

#endif // PZ_GAME_BUILDINGUNIT_H
