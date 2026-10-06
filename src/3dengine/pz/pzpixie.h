// src/3dengine/pz/pzpixie.h
// pz::SPixie: HD SPixie, the effect manager (0x7c bytes, ctor 0x6941e0,
// dtor 0x69cb90). One instance, owned by the Gepard facade (HD SGepard+0x7f4).
// OWNER: agent C. Skeleton from P0.
//
// Called by SScene::RenderViewport 0x6acaf0 (agent A): pixie +0x48 with the
// scene frame counter, then 0x69ea50(scene, ...) non-virtually to draw the
// scene's effects. Agent C declares 0x69ea50 here when lifting it.

#ifndef PZ_PZPIXIE_H
#define PZ_PZPIXIE_H

#include "ipixie.h"

namespace pz {

struct SPixie : SIPixie {
    SPixie();    // 0x6941e0
    ~SPixie();   // 0x69cb90 (non-virtual: deleted by Release)

    void AddRef() override;
    void Release() override;
    void Slot_08() override;
    int RefreshEffectPrototype(int proto) override;
    int LoadEffectPrototype(const char* file, bool p2, bool p3, int p4, int p5) override;
    void Slot_14() override;
    void Slot_18() override;
    void Slot_1C() override;
    void Slot_20() override;
    void PlayEffect(SIScene* scene, int proto, const float* pos, const float* dir, int p5) override;
    void Slot_28() override;
    void Slot_2C() override;
    void Slot_30() override;
    void StopEffect(int effect) override;
    void Slot_38() override;
    void Slot_3C() override;
    void UnregisterEffect(int effect) override;
    void Slot_44() override;
    void UpdateFrame(int frame) override;
    void Slot_4C() override;
    void Slot_50() override;
    void Slot_54() override;
    void Slot_58() override;
    void Slot_5C() override;
    void Slot_60() override;
    void Slot_64() override;
    virtual int InitEffectPrototype(int p1, int p2, int p3); // +0x68 HD 0x69d6a0 (3 arg dwords) SPixie only (not in SIPixie)

    // --- HD layout (offsets asserted below) ---
    unsigned char _04[0x78 - 0x04];
    int           RefCount;          // +0x78
};
PZ_HD_SIZE(SPixie, kHdSizeSPixie);
static_assert(offsetof(SPixie, RefCount) == 0x78, "SPixie layout");

} // namespace pz

#endif // PZ_PZPIXIE_H
