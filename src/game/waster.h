// src/game/waster.h
// SWasterUnit (ClassType 7, vftable 0x7ffc74, 0x364 bytes): mines,
// explosives, molotov fire, grenades and wrecks that go off after their
// LifeTime (SPWasterUnit +0x13c ticks), on contact (Sensor) or on command
// (RemoteControl). The explosion is gunner 0's shot (SGunner 0x587250, C1).
// OWNER: agent M3-C sub-agent C3.

#ifndef PZ_GAME_WASTER_H
#define PZ_GAME_WASTER_H

#include "unit.h"

namespace pz {

struct SWasterUnit : SUnit {
    SWasterUnit(SPWasterUnit* proto, int worldIndex);            // 0x5d1cd0
    ~SWasterUnit() override;                                     // 0x5d1db0
    void Uninit() override;                                      // 0x5d2070
    void CreateBoardElements();                                  // the board part of Init 0x5d22b0 / Slot_14 0x5d23d0 (unitboard.cpp)
    void ReleaseBoardElements();                                 // the board part of Uninit 0x5d2070
    void Init(SUnitDef* def) override;                           // 0x5d22b0
    void InitNew(int player, const float* pos, float dir, int p4, float hp) override;   // 0x5d2330
    void Slot_14() override;                                     // 0x5d23d0 (the board elements)
    void GetClassDescriptor(void** obj, const SUnitClassDesc** desc) override;   // 0x5d2290
    void RefreshDead() override;                                     // 0x5d2460 the dead waster's countdown
    void RefreshTargeting() override;                            // 0x5d2440
    void RefreshMisc() override;                                 // 0x5d24a0
    void UpdateVisuals(SIViewport* vp) override;                 // 0x5d1e30 (the timer bar on the board)
    void EC_AttackPos(int xBits, int zBits, int p3) override;    // 0x5d21a0
    void EC_Attack(int unit, int p2) override;                   // 0x5d2090
    void EC_Die() override;                                      // 0x5d2280

    void SetAttachedUnit(int unit) { AttachedUnit = unit; }      // 0x5d2870
    void Explode();                                              // 0x5d2880

    SPWasterUnit* P;                 // +0x340
    int      AttachedUnit;           // +0x344 the unit it sticks to / went off on (-1)
    int      Activated;              // +0x348 went off
    int      LifeTime;               // +0x34c ticks left (SPWasterUnit +0x13c)
    int      FireEffect;             // +0x350 molotov: pixie effect (-1)
    int      BoardBar;               // +0x354 board element: the timer bar (-1)
    int      BoardFrame;             // +0x358 board element: its frame (-1)
    int      OwnerSquad;             // +0x35c the squad that laid it (-1)
    int      OwnerRank;              // +0x360 that squad's rank (+0x88)
};

PZ_HD_SIZE(SWasterUnit, kHdSizeSWasterUnit);

} // namespace pz

#endif // PZ_GAME_WASTER_H
