// src/game/singleunit.h
// SSingleUnit (vftable 0x7fb2a4, 0x3a8 bytes): vehicles and guns
// (0x5aa7b0..0x5b1d00). OWNER: agent U.

#ifndef PZ_GAME_SINGLEUNIT_H
#define PZ_GAME_SINGLEUNIT_H

#include "unit.h"

namespace pz {

struct SSingleUnit : SUnit {
    int ActionOn(int target) override;                           // +0xa8 0x5ace90 (STrainUnit too)
    SSingleUnit(SPSingleUnit* proto, int worldIndex);            // 0x5aa7b0
    ~SSingleUnit() override;                                     // 0x5aaa00
    void Init(SUnitDef* def) override;                           // 0x5ad150
    void InitNew(int player, const float* pos, float dir, int p4, float hp) override;   // 0x5ad9f0
    void Place(float x, float z, float dir) override;            // 0x5b0950
    void SetOnBlockMap(bool on) override;                        // 0x5ae950
    void RefreshMisc() override;                                 // 0x5af890
    void OnMemberDied(int unit) override;                        // +0x1b4 0x5ae970 a crew member died
    void AddXP(int victim, float xp, int p3) override;           // +0x8c 0x5ad070 to the stored units
    void Uninit() override;                                      // 0x5abde0 (unitai.cpp)
    void RefreshTargeting() override;                            // 0x5aef40 (unitai.cpp)
    void UpdateVisuals(SIViewport* vp) override;                 // 0x5aaaa0 (unitai.cpp)
    void EC_Move(int xBits, int zBits, int p3, bool p4, int p5) override;   // 0x5ac4b0 (unitai.cpp)
    float GetMaxRange(int weapon) override;                      // 0x5acbd0 (unitai.cpp)
    float GetMinRange(int weapon) override;                      // 0x5acd30 (unitai.cpp)
    int GetRank() override;                                      // 0x5acb30 (unitai.cpp)
    // --- M3 C2 (combat / death overrides, singleunit.cpp)
    float GetLowestMaxRange() override;                          // 0x5aca80
    void OnAttackedBy(int attacker) override;                    // 0x5b0420
    void OnDriverReachedTarget() override;                       // 0x5aed30
    void RefreshDead() override;                                 // 0x5af2c0
    // --- end M3 C2

    void InitCrewAndChildren(int player, float dir, bool fromDef, float cargo);   // the shared part of 0x5ad150 / 0x5ad9f0
    void PlaceAttachedUnit(SUnit* unit, int node);               // 0x5b02e0

    SPSingleUnit* P;                 // +0x340
    int      Board[0x19];            // +0x344..+0x3a4 board elements (health, rank, selection; not created)
};

PZ_HD_SIZE(SSingleUnit, kHdSizeSSingleUnit);

} // namespace pz

#endif // PZ_GAME_SINGLEUNIT_H
