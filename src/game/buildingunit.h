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
#include "blockmaprefresh.h"

namespace pz {

// +0x3e8 element (0x48): a "viewN" node and its five "pN0".."pN4" points
// (InitViewPoints 0x548660), {x, z, dir} each.
struct SBuildingView {
    float Pos[3];            // +0x00
    float Points[5][3];      // +0x0c
};

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
    float GetMinRange(int weapon) override;                      // 0x5482e0 (M3-I)
    float GetSightRange() override;                              // 0x548b40 (M3-I)
    void AI_Heartbeat() override;                                // 0x546500
    void OnMemberDied(int unit) override;                        // +0x1b4 0x549b70 (stub) an occupant died
    int ActionOn(int target) override;                           // +0xa8 0x548c10
    void AddXP(int victim, float xp, int p3) override;           // +0x8c 0x548d30 to the occupants
    // --- M3-C5
    void Uninit() override;                                      // 0x547690
    void CreateBoardElements();                                  // the board part of Init 0x548f20 (unitboard.cpp)
    void Slot_14() override;                                     // 0x549500 after load (unit_afterload.cpp, agent S)
    void GetClassDescriptor(void** obj, const SUnitClassDesc** desc) override;   // 0x548b20
    void SetCurrentTarget(STarget* target, int p2) override;     // 0x54ca80
    bool StoreUnit(int unit, int mode) override;                 // 0x54d380
    void TakeDamage(float damage, int weaponType, int attacker, float x, float y, float z, int hitMode) override;   // 0x54de30
    bool GetNodePoint(float* out, const char* node);             // 0x548560 {x, z, dir} of a model node (+0x358 = its y)
    void InitViewPoints();                                       // 0x548660 entrance, window and view points
    void InitBlockCells();                                       // 0x5497a0 +0x458 cells of the "Block" points
    void RefreshOccupants();                                     // HD inline in 0x54b910: the occupants at the windows
    bool IsCapturable();                                         // 0x549af0 (+0x1bc override; the slot is untyped, C3)
    // --- M5-MS (capturable buildings, hangars)
    void UpdateCaptureFlag();                                    // 0x5471d0 CreateCaptureFlag: the flag model in the owner's colour
    void RefreshCapture();                                       // 0x54cd70 (name guessed) the owner from the units around
    void RefreshRadar();                                         // 0x54ac00 (name guessed) "multi radar": capture + player counters
    void RefreshSupportPlace();                                  // 0x54acd0 (name guessed) "Support place": capture, repair, supply
    void HealNearUnits();                                        // 0x54a4a0 (name guessed) order a heal (kind 8) for a squad with wounded
    void RefreshHangar();                                        // 0x54a380 (name guessed) +0x43c: players with a unit indoors
    // --- end M3-C5

    // Non-virtual (HD thiscall).
    bool IsOccupiedByTeam(int player);                           // 0x5468e0 (SBuildingAnimation 0x5cb650)

    SPBuildingUnit* P;               // +0x340
    SBlockBitmap* BlockNode;         // +0x344 model +0xa8(4, "Block") footprint (static block bit 4)
    SBlockBitmap* IndoorNode;        // +0x348 (building type 6, bit 0x8000)
    SBlockBitmap* ProductiveIndoorNode; // +0x34c (unit type 0x1a, bit 0x800)
    int      Board350;               // +0x350 board elements (not created)
    int      Board354;               // +0x354
    float    InsideY;                // +0x358 y of the units inside (the last GetNodePoint)
    int      _35c;                   // +0x35c
    int      _360;                   // +0x360
    float    Entrance[3];            // +0x364 "entrance" node {x, z, dir}
    float    WindowA[5][3];          // +0x370 "pa0".."pa4" (the second squad inside)
    float    WindowB[5][3];          // +0x3ac "p00".."p04" (the first squad inside)
    SUnitArray<SBuildingView> WindowSets;   // +0x3e8 the "viewN" sets (RefreshMisc, occupied)
    float    RangeBonus;             // +0x3f4 the farthest view node from the building (added to GetMaxRange)
    int      WindowSet;              // +0x3f8 RefreshMisc: -1 or the chosen +0x3e8 set
    SUnitArray<int> WindowSlots;     // +0x3fc
    SUnitArray<SUnitMember> Members2;// +0x408 second occupant list (SStoredMemberProperties)
    SUnitArray<int> WindowSlots2;    // +0x414
    SUnitArray<int> Inside;          // +0x420 units refreshed with the building (dead ones removed)
    int      _42c;                   // +0x42c
    SUnitArray<int> Wires;           // +0x430 world wire links (0x5f82b0 in Uninit)
    bool     _43c[12];               // +0x43c per player (hangar: IsOccupiedByTeam); ctor 0
    SIModel* Model448;               // +0x448 the capture flag model (0x5471d0)
    int      FlagNode;               // +0x44c (capturable: "flag" node)
    int      _450;                   // +0x450
    bool     OnStaticBlock;          // +0x454 a "Block" point is on the static block map
    unsigned char _455[3];
    SUnitArray<int> BlockCells;      // +0x458 vis-map cells of the "Block" points (0x5497a0)
    unsigned char _464[0x470 - 0x464];
};

// 0x548bf0 (name guessed): SGameLogic +0x2d9 (unlimited cargo cheat) && the logic is not
// paused; a supplier for which it holds (and that is the local player's)
// does not pay cargo.
bool UnitSuppliesForFree(SUnit* u);

PZ_HD_SIZE(SBuildingUnit, kHdSizeSBuildingUnit);
#if defined(_M_IX86)
static_assert(offsetof(SBuildingUnit, Entrance) == 0x364, "0x548660 +0x364");
static_assert(offsetof(SBuildingUnit, WindowSets) == 0x3e8, "0x548660 +0x3e8");
static_assert(sizeof(SBuildingView) == 0x48, "+0x3e8 element stride 0x48 (0x548660, ShowUnitRange 0x5fee00)");
static_assert(offsetof(SBuildingUnit, RangeBonus) == 0x3f4, "0x54a250 +0x3f4");
static_assert(offsetof(SBuildingUnit, Members2) == 0x408, "0x54b910 +0x408");
static_assert(offsetof(SBuildingUnit, Inside) == 0x420, "0x54b910 +0x420");
static_assert(offsetof(SBuildingUnit, Wires) == 0x430, "0x547690 +0x430");
static_assert(offsetof(SBuildingUnit, _43c) == 0x43c, "0x5468e0 +0x43c");
static_assert(offsetof(SBuildingUnit, Model448) == 0x448, "0x5468a0 +0x448");
static_assert(offsetof(SBuildingUnit, BlockCells) == 0x458, "0x5497a0 +0x458");
static_assert(sizeof(SBuildingView) == 0x48, "0x548660 element 0x48");
#endif

} // namespace pz

#endif // PZ_GAME_BUILDINGUNIT_H
