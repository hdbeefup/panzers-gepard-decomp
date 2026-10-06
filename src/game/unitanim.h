// src/game/unitanim.h
// pz::SUnitAnimation and pz::SPUnitAnimation (0x5c6300..0x5cfb00). OWNER: agent A.
// Logged skeleton from M2-P0: every slot is STUB_LOG + PZ_M2_TRACE. Lift
// from the HD exe; mark lifted bodies with // PANZERS 0xADDR and drop the
// STUB_LOG line (docs/M2_INTERFACES.md).

#ifndef PZ_GAME_UNITANIM_H
#define PZ_GAME_UNITANIM_H

#include "iunitanim.h"

namespace pz {

// HD SUnitAnimation base (vftable 0x7fd7e4); +0x04, +0x08 and +0x10 are pure in HD.
struct SUnitAnimation : SIUnitAnimation {
    SUnitAnimation();
    ~SUnitAnimation() override;
    void InitModel(int p1) override;
    void UpdateModel() override;
    void Slot_0C() override;
    void Slot_10() override;
    void Slot_14() override;
    float GetStateMoveSpeed(int state) override;
    float GetStateTurnSpeed(int state) override;
    void* GetRunningGear() override;
    void Slot_24() override;
    void Slot_28() override;
    void Slot_2C() override;
    void Slot_30() override;
    void Slot_34() override;
    void Slot_38() override;
    int GetAttachNode() override;
    SIModel* GetModel() override;
};

// HD SPUnitAnimation base (vftable 0x7fd724); +0x04 and +0x10 are pure in HD.
struct SPUnitAnimation : SIPUnitAnimation {
    SPUnitAnimation();
    ~SPUnitAnimation() override;
    void Load(SProperties* props) override;
    void LoadResources(int p1, int p2) override;
    void Slot_0C() override;
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override;
};

} // namespace pz

#endif // PZ_GAME_UNITANIM_H
