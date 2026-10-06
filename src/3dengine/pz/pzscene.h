// src/3dengine/pz/pzscene.h
// pz::SScene: HD SScene (0x2b0 bytes, ctor 0x69faf0, dtor 0x6a0440).
// OWNER: agent A. Skeleton from P0: every slot is a logged stub except the
// trivial ones lifted below; unknown bytes are padding at their HD offsets.

#ifndef PZ_PZSCENE_H
#define PZ_PZSCENE_H

#include "iscene.h"

namespace pz {

struct SViewport;
struct STerrain;

struct SScene : SIScene {
    explicit SScene(int param);   // 0x69faf0
    ~SScene();                    // 0x6a0440 (non-virtual: deleted by Release)

    void AddRef() override;
    void Release() override;
    void SetAtmosphere(const char* name) override;
    int Slot_0C_CreateMinimapTarget(int p1, int w2, int h2) override;
    void Slot_10() override;
    void Slot_14() override;
    void Slot_18() override;
    void AdvanceTime(int milliseconds) override;
    void SetInterpolation(double t) override;
    void SetFocusHeight(float height) override;
    void Slot_28() override;
    int CreatePointLight(float x, float y, float z, float range, double p5, int p6) override;
    int CreateSpotLight(float x, float y, float z, float range, double p5, int p6, int p7, int p8) override;
    void SetLightPosition(int light, float x, float y, float z) override;
    void DestroyLight(int light) override;
    void SetAmbientLight(const float* rgba) override;
    void Slot_40() override;
    void SetSunLight(const float* color, float p2, float p3) override;
    void Slot_48() override;
    void SetFog(float r, float g, float b, float start, float end, float alpha) override;
    SIModel* CreateModelFromFile(const char* file, float scale, int p3, int p4) override;
    SIModel* CreateModel(int proto, int proto2, bool flag) override;
    SIModel* CreateModelFromPrototype(int proto, int flag) override;
    void Slot_5C() override;
    void ReplaceModel(SIModel* model, int proto) override;
    SITerrain* CreateTerrain(int width, int height, int p3) override;
    void DestroyTerrain() override;
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
    void Slot_94() override;
    void Slot_98() override;
    int CreateLake(const char* p1, const char* p2, float p3, float p4, short* p5, int p6, short* p7, int p8) override;
    void DestroyLake(int lake) override;
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
    void UpdateWire(int wire, int* p2) override;
    void Slot_D0() override;
    void Slot_D4() override;
    void Slot_D8() override;
    void Slot_DC() override;
    void Slot_E0() override;
    void Slot_E4() override;
    void Slot_E8() override;
    void Slot_EC() override;
    void Slot_F0() override;
    void Slot_F4() override;
    void Slot_F8() override;
    void SetSkybox(const char* file, float radius) override;
    void ClearSkybox() override;
    void Slot_104_RecreateShadowBuffer() override;

    // Non-virtual HD members SViewport::Render 0x68c220 calls for each
    // subport, all three with (SViewport*) (Ghidra shows 0x6a24c0 and
    // 0x6acaf0 as fastcall without it; the caller pushes the viewport).
    void PrepareViewport(SViewport* vp);   // 0x6bbc40 before BeginScene (visibility)
    void UpdateViewport(SViewport* vp);    // 0x6a24c0 before BeginScene
    void RenderViewport(SViewport* vp);    // 0x6acaf0 after BeginScene: lights, camera, terrain, roads, models, pixie
    void RenderScene();                    // 0x6b7760

    // --- HD layout (offsets asserted below) ---
    int           RefCount;          // +0x04
    unsigned char _08[0x98 - 0x08];
    float         FocusHeight;       // +0x98 (+0x24 SetFocusHeight)
    unsigned char _9c[0xa0 - 0x9c];
    int           FrameCount;        // +0xa0 (RenderViewport ++; passed to pixie +0x48)
    int           TimeMs;            // +0xa4 (+0x1c AdvanceTime)
    unsigned char _a8[0xb8 - 0xa8];
    double        Interpolation;     // +0xb8 (+0x20 SetInterpolation; SModel +0x14 blends with it)
    unsigned char _c0[0x170 - 0xc0];
    char*         AtmosphereName;    // +0x170 SString {buf, size} (+0x08 SetAtmosphere)
    int           AtmosphereLen;     // +0x174
    unsigned char _178[0x1c8 - 0x178];
    STerrain*     Terrain;           // +0x1c8 (+0x64 CreateTerrain)
    unsigned char _1cc[0x2b0 - 0x1cc];
};
PZ_HD_SIZE(SScene, kHdSizeSScene);
static_assert(offsetof(SScene, RefCount) == 0x04, "SScene layout");
static_assert(offsetof(SScene, FocusHeight) == 0x98, "SScene layout");
static_assert(offsetof(SScene, FrameCount) == 0xa0, "SScene layout");
static_assert(offsetof(SScene, TimeMs) == 0xa4, "SScene layout");
static_assert(offsetof(SScene, Interpolation) == 0xb8, "SScene layout");
static_assert(offsetof(SScene, AtmosphereName) == 0x170, "SScene layout");
static_assert(offsetof(SScene, Terrain) == 0x1c8, "SScene layout");

} // namespace pz

#endif // PZ_PZSCENE_H
