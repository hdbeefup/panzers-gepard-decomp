// src/game/unit.h
// pz::SUnit, the HD unit base class (0x5b2800..0x5c6300). OWNER: agent U.
// Logged skeleton from M2-P0: every slot is STUB_LOG + PZ_M2_TRACE. Lift
// from the HD exe; mark lifted bodies with // PANZERS 0xADDR and drop the
// STUB_LOG line (docs/M2_INTERFACES.md).

#ifndef PZ_GAME_UNIT_H
#define PZ_GAME_UNIT_H

#include "iunit.h"

namespace pz {

// HD SUnit base (vftable 0x7fcc24). Base size unknown (<= 0x354).
struct SUnit : SIUnit {
    SUnit();
    ~SUnit() override;
    void Uninit() override;
    void Init(SUnitDef* def) override;
    void InitNew(int p1, int p2, int p3, int p4, int p5) override;
    void Slot_10() override;
    void Slot_14() override;
    void Slot_18() override;
    void Slot_1C() override;
    void Hook20(int p1) override;
    void SetPosition(float x, float z, int p3, int p4) override;
    void Slot_28() override;
    void ServerRefresh(int frame) override;
    void Slot_30() override;
    void RefreshTargeting() override;
    void RefreshMisc() override;
    void RefreshModel() override;
    void UpdateVisuals(SIViewport* vp) override;
    void Slot_44() override;
    void Slot_48() override;
    void Unplace() override;
    void Place(float x, float z, float dir) override;
    void Slot_54() override;
    void Slot_58() override;
    void StoreUnit(int unit, int p2) override;
    void Slot_60() override;
    void Slot_64() override;
    void Slot_68() override;
    void Slot_6C() override;
    void Remove(bool p1) override;
    void Slot_74() override;
    void Slot_78() override;
    void Slot_7C() override;
    bool HasWoundedMember() override;
    void Slot_84() override;
    int GetRank() override;
    void Slot_8C() override;
    void Slot_90() override;
    void Slot_94() override;
    void Slot_98() override;
    void Slot_9C() override;
    void SetCurrentTarget(STarget* target) override;
    void Slot_A4() override;
    void Slot_A8() override;
    void EC_Move(int p1, int p2, int p3, bool p4, int p5) override;
    void Slot_B0() override;
    void EC_MoveAlongPath(int path, int p2, int p3) override;
    void EC_Follow(int unit, int p2) override;
    void Slot_BC() override;
    void Stop() override;
    void ClearTargets() override;
    void Slot_C8() override;
    void Slot_CC() override;
    void Slot_D0() override;
    void Slot_D4() override;
    void Slot_D8() override;
    void Slot_DC() override;
    void Slot_E0() override;
    void Slot_E4() override;
    void Slot_E8() override;
    void StopGunners() override;
    void Slot_F0() override;
    void Slot_F4() override;
    void Slot_F8() override;
    void Slot_FC() override;
    void Slot_100() override;
    void Slot_104() override;
    void Slot_108() override;
    void Slot_10C() override;
    void Slot_110() override;
    void Slot_114() override;
    void Slot_118() override;
    void Slot_11C() override;
    void Slot_120() override;
    void Slot_124() override;
    void Slot_128() override;
    void SetBehavior(int behavior) override;
    void Slot_130() override;
    void Slot_134() override;
    void Slot_138() override;
    void Slot_13C() override;
    void Slot_140() override;
    void Slot_144() override;
    void Slot_148() override;
    void Slot_14C() override;
    void Slot_150() override;
    void Slot_154() override;
    void Slot_158() override;
    void Slot_15C() override;
    void Slot_160() override;
    void Slot_164() override;
    void Slot_168() override;
    void StoreInterpolationState() override;
    void Slot_170() override;
    void Slot_174() override;
    void Slot_178() override;
    float GetMaxRange(int weapon) override;
    float GetMinRange(int weapon) override;
    float GetSightRange() override;
    float GetExtra188() override;
    void Slot_18C() override;
    void AI_Heartbeat() override;
    void Slot_194() override;
    void SetOnBlockMap(bool on) override;
    void Slot_19C() override;
    void MarkBlockMap(bool on, int p2, int p3, int p4, short p5) override;
    int TestBlockMapPath(int p1, int p2, int p3, int p4, short p5, int p6, int p7) override;
    int TestBlockMap(int p1, int p2, int p3, int p4, short p5) override;
    void RestoreBehavior() override;
    float GetMoveSpeed(int p1) override;
    void Slot_1B4() override;
    void Slot_1B8() override;
    void Slot_1BC() override;
    void ServerRefreshMedic(float dt) override;
    void SetUnitSize() override;
    void GetCenterPosition(float* out) override;
};

} // namespace pz

#endif // PZ_GAME_UNIT_H
