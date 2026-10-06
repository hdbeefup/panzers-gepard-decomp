// src/game/buildingunit.h
// SBuildingUnit (vftable 0x7f4184, 0x470 bytes): houses, towers, capturable
// buildings (0x545940..0x5500a0). OWNER: agent U.
//
// The menu has one, the "FR village long stone" house (UNDS slot 0). Only
// what it executes is lifted: construction, Init (node hiding), the
// interpolation store of the second model, the per-tick slots of an empty
// building. Entering, occupants, capture flags and productive buildings
// are logged stubs.

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
    void RefreshTargeting() override;                            // 0x54a250
    void RefreshMisc() override;                                 // 0x54b910
    void UpdateVisuals(SIViewport* vp) override;                 // 0x546d20
    float GetMaxRange(int weapon) override;                      // 0x548240
    void AI_Heartbeat() override;                                // 0x546500

    // Non-virtual (HD thiscall).
    bool IsOccupiedByTeam(int player);                           // 0x5468e0 (SBuildingAnimation 0x5cb650)

    // M3 agent F (buildingunit_store.cpp): squads in buildings at map load.
    bool StoreUnit(int unit, int mode) override;                 // vtbl +0x5c 0x54d380
    void FindWindowPoints();                                     // 0x548660 (Init) +0x358, +0x364..+0x3e8, +0x3f4
    bool ReadNodePoint(float* dst, const char* node);            // 0x548560 {x, z, dir} of a model node

    SPBuildingUnit* P;               // +0x340
    void*    BlockNode;              // +0x344 model +0xa8(4, "Block") footprint
    void*    IndoorNode;             // +0x348 (building type 6)
    void*    ProductiveIndoorNode;   // +0x34c (unit type 0x1a)
    int      Board350;               // +0x350 board elements (not created)
    int      Board354;               // +0x354
    float    InsideY;                // +0x358 y of the units inside
    unsigned char _35c[0x3e8 - 0x35c]; // +0x370 / +0x3ac: 5 window points {x, z, dir} each
    SUnitArray<unsigned char[0xc]> WindowSets;   // +0x3e8 (RefreshMisc, occupied)
    float    RangeBonus;             // +0x3f4 added to GetMaxRange (RefreshTargeting) (name guessed)
    int      WindowSet;              // +0x3f8 RefreshMisc: -1 or the chosen +0x3e8 set
    SUnitArray<int> WindowSlots;     // +0x3fc
    SUnitArray<SUnitMember> Members2;// +0x408 second occupant list (SStoredMemberProperties)
    SUnitArray<int> WindowSlots2;    // +0x414
    SUnitArray<int> Inside;          // +0x420 units refreshed with the building (dead ones removed)
    unsigned char _42c[0x43c - 0x42c];
    bool     _43c[12];               // +0x43c per player (hangar: IsOccupiedByTeam); ctor 0
    SIModel* Model448;               // +0x448
    int      FlagNode;               // +0x44c (capturable: "flag" node)
    unsigned char _450[0x454 - 0x450];
    bool     OnStaticBlock;          // +0x454 a "Block" point is on the static block map
    unsigned char _455[0x470 - 0x455];
};

PZ_HD_SIZE(SBuildingUnit, kHdSizeSBuildingUnit);
#if defined(_M_IX86)
static_assert(offsetof(SBuildingUnit, RangeBonus) == 0x3f4, "0x54a250 +0x3f4");
static_assert(offsetof(SBuildingUnit, Members2) == 0x408, "0x54b910 +0x408");
static_assert(offsetof(SBuildingUnit, Inside) == 0x420, "0x54b910 +0x420");
static_assert(offsetof(SBuildingUnit, _43c) == 0x43c, "0x5468e0 +0x43c");
static_assert(offsetof(SBuildingUnit, Model448) == 0x448, "0x5468a0 +0x448");
#endif

} // namespace pz

#endif // PZ_GAME_BUILDINGUNIT_H
