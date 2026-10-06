// src/game/singleunit.h
// SSingleUnit (vftable 0x7fb2a4, 0x3a8 bytes): vehicles and guns
// (0x5aa7b0..0x5b1d00). OWNER: agent U.

#ifndef PZ_GAME_SINGLEUNIT_H
#define PZ_GAME_SINGLEUNIT_H

#include "unit.h"

namespace pz {

struct SSingleUnit : SUnit {
    SSingleUnit(SPSingleUnit* proto, int worldIndex);            // 0x5aa7b0
    ~SSingleUnit() override;                                     // 0x5aaa00
    void Init(SUnitDef* def) override;                           // 0x5ad150
    void InitNew(int player, const float* pos, float dir, int p4, float hp) override;   // 0x5ad9f0
    void Place(float x, float z, float dir) override;            // 0x5b0950
    void SetOnBlockMap(bool on) override;                        // 0x5ae950

    void InitCrewAndChildren(int player, float dir, bool fromDef, float cargo);   // the shared part of 0x5ad150 / 0x5ad9f0

    SPSingleUnit* P;                 // +0x340
    int      Board[0x19];            // +0x344..+0x3a4 board elements (health, rank, selection; not created)
};

PZ_HD_SIZE(SSingleUnit, kHdSizeSSingleUnit);

} // namespace pz

#endif // PZ_GAME_SINGLEUNIT_H
