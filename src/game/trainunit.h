// src/game/trainunit.h
// STrainUnit (vftable 0x7fb780, 0x3b4 bytes): locomotives, carriages and
// platforms (.unit ClassType 10). OWNER: agent TR.
//
// A train is an SSingleUnit that stands on a ROD2 road (rail.h): +0x3ac is
// the road, SUnit +0x300 the path length on it. The locomotive drives along
// the road (STrainDriver, traindriver.h); the carriages are its towed chain
// (SUnit +0x2d8, the map's towed units, SUnit::Tow 0x5c2390) and stand
// behind it on the same road (GhostFramesAddTop 0x5b0f00). The block-map
// footprint is the model's "Block" node (+0x3b0), not the unit square.

#ifndef PZ_GAME_TRAINUNIT_H
#define PZ_GAME_TRAINUNIT_H

#include "singleunit.h"
#include "punit.h"

namespace pz {

struct SBlockBitmap;

struct STrainUnit : SSingleUnit {
    STrainUnit(SPTrainUnit* proto, int worldIndex);              // 0x5b0e70
    ~STrainUnit() override;                                      // 0x5b0ed0
    void Uninit() override;                                      // +0x04 0x5b15e0
    void Init(SUnitDef* def) override;                           // +0x08 0x5b1650
    void InitNew(int player, const float* pos, float dir, int p4, float hp) override;   // +0x0c 0x5b1680
    void Slot_14() override;                                     // +0x14 0x5b16d0
    void SetPosition(float x, float z, int dirBits, int yrelBits) override;   // +0x24 0x5b1980
    void GhostFramesAddTop(float x, float y, float z, float dir, float dist, SIUnit* towed,
                           float* outPos, float* outDir, float* outDist) override;   // +0x74 0x5b0f00
    void EC_Follow(int unit, int p2) override;                   // +0xb8 0x5b1620 (empty)
    void SetOnBlockMap(bool on) override;                        // +0x198 0x5b16f0
    void Slot_19C() override;                                    // +0x19c 0x5b1400
    void MarkBlockMap(bool on, int p2, int p3, int p4, short p5) override;   // +0x1a0 0x5b17c0
    int TestBlockMap(int p1, int p2, int p3, int p4, short p5) override;      // +0x1a8 0x5b1420
    void SetUnitSize() override;                                 // +0x1c4 0x5b1b70

    void InitRail();                                             // 0x5b1be0
    // Recompile test hook (not an HD override: HD +0x38 is SSingleUnit's
    // 0x5af890): PZ_TR_TEST=1 logs the trains' track position every 10 s and
    // makes the camera follow the first locomotive once.
    void RefreshMisc() override;

    SPTrainUnit*  TP;                // +0x3a8
    int           Road;              // +0x3ac World+0x73fc road the train stands on, -1
    SBlockBitmap* BlockFootprint;    // +0x3b0 model "Block" bitmap while on the block map
};

PZ_HD_SIZE(STrainUnit, kHdSizeSTrainUnit);

} // namespace pz

#endif // PZ_GAME_TRAINUNIT_H
