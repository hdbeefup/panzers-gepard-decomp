// src/3dengine/pz/pzterrain.h
// pz::STerrain: HD STerrain (0x11c88 bytes, ctor 0x6efde0, dtor 0x6f06c0).
// Created only by SScene CreateTerrain (+0x64) with (scene, w, h, p); the
// TERR loader passes (168, 184, 4) for menu.map. 16 blend layers (SWINE has 9).
// OWNER: agent B. Skeleton from P0.
//
// Called by SScene (agent A) on the render path, non-virtually:
//   0x6f8610 (visibility, from SScene::RenderScene), 0x6f2aa0 terrain draw,
//   0x6f46e0 roads, 0x6f33c0 (from SScene::RenderViewport 0x6acaf0).
// Agent B declares those here when lifting them; agent A calls them.

#ifndef PZ_PZTERRAIN_H
#define PZ_PZTERRAIN_H

#include "iterrain.h"

namespace pz {

struct SScene;

struct STerrain : SITerrain {
    STerrain(SScene* scene, int width, int height, int p4);   // 0x6efde0
    ~STerrain();                                              // 0x6f06c0

    void Acquire(STerrainBuffers* out) override;
    void Slot_04_LoadLayerTexture(int layer, int p2) override;
    void Slot_08() override;
    void LoadSketchTexture(const char* file) override;
    void Slot_10() override;
    void Slot_14() override;
    void Slot_18() override;
    void Slot_1C() override;
    void Slot_20() override;
    void Slot_24() override;
    void Slot_28() override;
    void Slot_2C() override;
    void Slot_30() override;
    void Slot_34() override;
    void Slot_38() override;
    void Slot_3C() override;
    void Slot_40() override;
    void Slot_44() override;
    void Slot_48() override;
    void Slot_4C() override;
    void Slot_50() override;
    void Slot_54() override;
    void Slot_58() override;
    void Slot_5C() override;
    void Slot_60() override;
    void Slot_64() override;
    void Slot_68() override;
    void Slot_6C() override;
    void Slot_70() override;
    void Slot_74() override;
    void Slot_78() override;
    void Slot_7C() override;
    void Slot_80() override;
    void Slot_84() override;
    void UpdateRoad(int road) override;
    void Slot_8C() override;
    void Slot_90() override;
    void Slot_94() override;
    void UpdateRoadJunction(int junction) override;

    // --- HD layout ---
    unsigned char _04[0x11c88 - 0x04];
};
PZ_HD_SIZE(STerrain, kHdSizeSTerrain);

} // namespace pz

#endif // PZ_PZTERRAIN_H
