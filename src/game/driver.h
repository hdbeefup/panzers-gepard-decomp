// src/game/driver.h
// pz::SDriver and pz::SPDriver (0x5500f0..0x55c900). OWNER: agent P.
// Logged skeleton from M2-P0: every slot is STUB_LOG + PZ_M2_TRACE. Lift
// from the HD exe; mark lifted bodies with // PANZERS 0xADDR and drop the
// STUB_LOG line (docs/M2_INTERFACES.md).

#ifndef PZ_GAME_DRIVER_H
#define PZ_GAME_DRIVER_H

#include "idriver.h"

namespace pz {

// HD SDriver base (vftable 0x7f49e0). Base size <= 0xe8.
struct SDriver : SIDriver {
    SDriver();
    ~SDriver() override;
    SIPDriver* GetPDriver() override;
    void RefreshTarget(int p1) override;
    void Slot_0C() override;
    void Init() override;
    void Refresh() override;
    void MoveTowardNextWayPoint(int p1) override;
    void Slot_1C() override;
    void Slot_20() override;
    bool GhostStepStraight(SGhostFrame* frame, int p2, int p3) override;
    bool GhostStepTowards(SGhostFrame* frame, float x, float z) override;
    int GhostTurnTowards(SGhostFrame* frame, float x, float z) override;
    int FindGlobalPath(STarget* target) override;
    int FindLocalPath(STarget* target) override;
    unsigned RefreshGhost() override;
    int PredictGhost(float* pos, int* p2, float* dir, float* speed) override;
    void Ghost_FirstStep() override;
    void Ghost_NextStep() override;
    float GetMaxSpeed() override;
    float GetTurnSpeed() override;
    void Slot_50() override;
};

// HD SPDriver base (vftable 0x7f4998). HD sizes 0x44..0x58; +0x10 is pure in HD.
struct SPDriver : SIPDriver {
    SPDriver();
    ~SPDriver() override;
    void Load(SProperties* props, int p2, int p3) override;
    void LoadSubProperties() override;
    void Slot_0C() override;
    SIDriver* CreateDriver(SIUnit* unit) override;
};

} // namespace pz

#endif // PZ_GAME_DRIVER_H
