// src/3dengine/pz/pzmodel.h
// pz::SModel: HD SModel (0x150 bytes, ctor 0x6d4ae0), created by SScene
// CreateModel*. Two vptrs: SIModel at +0x00, SIAttachable at +0x04 (MSVC
// puts them in declaration order of the bases, as HD does).
// OWNER: agent A. Skeleton from P0.

#ifndef PZ_PZMODEL_H
#define PZ_PZMODEL_H

#include "imodel.h"

namespace pz {

struct SScene;

struct SModel : SIModel, SIAttachable {
    SModel(SScene* scene, SPModel* proto, SPModel* proto2, int flag, int index);   // 0x6d4ae0
    ~SModel() override;                                                           // via SIAttachable +0x00

    void AddRef() override;
    void Release() override;
    void Slot_08() override;
    void Slot_0C() override;
    void GetPosition(float* xyz) override;
    void GetRenderPosition(float* xyz) override;
    void SetPosition(float x, float y, float z) override;
    void SetRotation(float angle, float tiltX, float tiltZ) override;
    void SetRotationYawPitchRoll(float pitch, float yaw, float roll) override;
    void RotateAxis(float x, float y, float z, float angle) override;
    void SetScale(float scale) override;
    float GetScale() override;
    void SetSequence(unsigned sequence, bool p2) override;
    void Slot_34() override;
    void Slot_38() override;
    void StoreInterpolationState() override;
    int FindNode(const char* name) override;
    void Slot_44() override;
    void Slot_48() override;
    void Slot_4C() override;
    void Slot_50() override;
    void Slot_54() override;
    void Slot_58() override;
    void Slot_5C() override;
    void SetNodeVisible(int node, bool visible) override;
    void Slot_64() override;
    void Slot_68() override;
    void Slot_6C() override;
    void Slot_70() override;
    void Slot_74() override;
    void Slot_78() override;
    void Slot_7C() override;
    void Slot_80() override;
    void Slot_84() override;
    void Slot_88() override;
    void Slot_8C() override;
    void Slot_90() override;
    void SetFlags(unsigned flags) override;
    void Slot_98() override;
    void Slot_9C() override;
    void Slot_A0() override;
    void Slot_A4() override;
    void Slot_A8() override;
    void Slot_AC() override;
    void Slot_B0() override;
    void Slot_B4() override;
    void Slot_B8() override;
    void Slot_BC() override;
    void Slot_C0() override;
    void Slot_C4() override;
    void Slot_C8() override;
    void Slot_CC() override;
    void Slot_D0() override;
    void Slot_D4() override;
    void Slot_D8() override;
    void Slot_DC() override;
    void Slot_E0() override;
    void Slot_E4() override;
    void SetSway(float phase, float p2, float p3) override;
    void Slot_EC() override;
    void Slot_F0() override;
    void Slot_F4() override;
    void Slot_F8() override;
    void Slot_FC() override;
    void Slot_100() override;
    void Slot_104() override;

    // SIAttachable (+0x04 vtable 0x883910)
    int Update(int p1, int p2) override;
    void Attach_08() override;
    void Attach_0C() override;

    // --- HD layout (offsets asserted below) ---
    unsigned char _08[0x10 - 0x08];
    int           RefCount;          // +0x10 (ctor sets 1)
    SScene*       Scene;             // +0x14
    unsigned char _18[0x150 - 0x18];
};
PZ_HD_SIZE(SModel, kHdSizeSModel);
static_assert(offsetof(SModel, RefCount) == 0x10, "SModel layout");
static_assert(offsetof(SModel, Scene) == 0x14, "SModel layout");

} // namespace pz

#endif // PZ_PZMODEL_H
