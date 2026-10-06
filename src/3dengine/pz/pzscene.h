// src/3dengine/pz/pzscene.h
// pz::SScene: HD SScene (0x2b0 bytes, ctor 0x69faf0, dtor 0x6a0440).
// OWNER: agent A.

#ifndef PZ_PZSCENE_H
#define PZ_PZSCENE_H

#include "iscene.h"
#include "pmodel.h"

namespace pz {

struct SViewport;
struct STerrain;
struct SModel;

// Scene light (HD SHeap entry value, 0x44 bytes).
struct SSceneLight {
    int   Type;        // +0x00 (entry +0x04) 1 point, 2 spot, 3 directional
    float Color[4];    // +0x04 diffuse = specular
    float Dir[3];      // +0x14
    float Pos[3];      // +0x20
    float Range;       // +0x2c
    float Atten[3];    // +0x30
    float Theta;       // +0x3c
    float Phi;         // +0x40
};
static_assert(sizeof(SSceneLight) == 0x44, "HD light entry 0x48");

// Terrain-cell model link (scene +0x1ac SDArray, 8 bytes).
struct SCellLink {
    int Model;
    int Next;
};

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
    int CreateDirectionalLight(float r, float g, float b, float a, float dx, float dy, float dz) override;
    int CreatePointLight(float r, float g, float b, float a, float x, float y, float z, float range, float atten2) override;
    void SetLightPosition(int light, float x, float y, float z) override;
    void DestroyLight(int light) override;
    void SetAmbientLight(const float* rgba) override;
    const float* GetAmbientLight() override;
    void SetSunLight(const float* color, float p2, float p3) override;
    void GetSunLight(float* color, float* p2, float* p3) override;
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

    // Other non-virtual HD members.
    void UpdateModels(SViewport* vp, bool all);     // 0x6bbcb0
    void SortModelsIntoCells();                     // 0x6ac210
    void DrawModels(SViewport* vp, int p2);         // 0x6b02a0
    void DrawRivers(SViewport* vp);                 // 0x6b0920
    void ModelsMoved() { ModelsSorted = false; }    // 0x6bbbc0
    void AddDeferredModel(SModel* m);               // 0x6d5820 on +0x2a0
    void RemoveModel(SModel* m);
    void SetLightColor(int light, const float* rgba);           // 0x6bab60
    void SetLightDirection(int light, const float* dir);        // 0x6babe0
    int  CreateDirectionalLight(const float* rgba, const float* dir);
    void SetLights();                                           // 0x680620 (SGepard)
    void GenerateShadowBuffer(SViewport* vp);                   // 0x6aac20
    void ReleaseShadowBuffer();                                 // 0x6a2670

    // --- HD layout (offsets asserted below) ---
    int           RefCount;          // +0x04
    int           Param;             // +0x08
    void*         Device;            // +0x0c SGepard +0x478
    int           ShadowViewport;    // +0x10 offscreen viewport of the shadow buffer (-1: none yet)
    int           ShadowTexture;     // +0x14 its texture (-1: no shadow buffer)
    float         ShadowProjection[16]; // +0x18 world -> shadow buffer (the device PROJECTION of the shadow pass)
    float         ShadowMatrix[16];  // +0x58 camera space -> shadow texture (shadow pass: -> height)
    float         FocusHeight;       // +0x98 (+0x24 SetFocusHeight; the shadow pass: 2 * lowest visible ground)
    int           ShadowCull;        // +0x9c 2 CW / 3 CCW (camera above / below the focus height)
    int           FrameCount;        // +0xa0 (RenderViewport ++; passed to pixie +0x48)
    int           TimeMs;            // +0xa4 (+0x1c AdvanceTime)
    int           LastTimeMs;        // +0xa8
    int           FrameMs;           // +0xac
    float         Seconds;           // +0xb0 timer at PrepareViewport
    int           _b4;
    double        Interpolation;     // +0xb8 (+0x20 SetInterpolation; SModel +0x14 blends with it)
    SHeap<SSceneLight> Lights;       // +0xc0
    int           SunLight;          // +0xd4
    float         Ambient[4];        // +0xd8
    float         SunColor[4];       // +0xe8
    float         LightColor[4];     // +0xf8 ambient + sun, clamped (effects read it)
    float         SunAzimuth;        // +0x108
    float         SunElevation;      // +0x10c
    float         SunDir[3];         // +0x110
    float         FogRGB[3];         // +0x11c
    float         _128;
    float         SunMatrix[12];     // +0x12c
    unsigned      FogColor;          // +0x15c
    unsigned      FogColorAlpha;     // +0x160
    float         FogStart;          // +0x164
    float         FogEnd;            // +0x168
    float         FogInvRange;       // +0x16c
    char*         AtmosphereName;    // +0x170 SString {buf, size} (+0x08 SetAtmosphere)
    int           AtmosphereLen;     // +0x174
    SHeap<SModel*> Models;           // +0x178 flag-0 models (doodads, units)
    SHeap<SModel*> FreeModels;       // +0x18c flag-1 models (drawn without terrain cells)
    bool          ModelsSorted;      // +0x1a0
    unsigned char _1a1[3];
    int*          CellHeads;         // +0x1a4
    int*          CellTails;         // +0x1a8
    SCellLink*    CellLinks;         // +0x1ac
    int           CellLinkCount;     // +0x1b0
    int           CellLinkMax;       // +0x1b4
    int           TerrainW;          // +0x1b8
    int           TerrainH;          // +0x1bc
    int           CellsX;            // +0x1c0
    int           CellsZ;            // +0x1c4
    STerrain*     Terrain;           // +0x1c8 (+0x64 CreateTerrain)
    unsigned char _1cc[0x220 - 0x1cc];   // decal/trail/lake heaps (+0x1cc, +0x1e0, +0x1f8, +0x20c)
    SHeap<unsigned char[0x74]> Rivers;   // +0x220 water courses (Slot_B0), drawn by 0x6b0920
    int           RiverTexture;      // +0x234
    unsigned char _238[0x2a0 - 0x238];
    SModel**      Deferred;          // +0x2a0 alpha models drawn last
    int           DeferredCount;     // +0x2a4
    int           DeferredMax;       // +0x2a8
    int           _2ac;
};
PZ_HD_SIZE(SScene, kHdSizeSScene);
static_assert(offsetof(SScene, RefCount) == 0x04, "SScene layout");
static_assert(offsetof(SScene, ShadowMatrix) == 0x58, "SScene layout");
static_assert(offsetof(SScene, ShadowCull) == 0x9c, "SScene layout");
static_assert(offsetof(SScene, FocusHeight) == 0x98, "SScene layout");
static_assert(offsetof(SScene, FrameCount) == 0xa0, "SScene layout");
static_assert(offsetof(SScene, TimeMs) == 0xa4, "SScene layout");
static_assert(offsetof(SScene, FrameMs) == 0xac, "SScene layout");
static_assert(offsetof(SScene, Interpolation) == 0xb8, "SScene layout");
static_assert(offsetof(SScene, Lights) == 0xc0, "SScene layout");
static_assert(offsetof(SScene, Ambient) == 0xd8, "SScene layout");
static_assert(offsetof(SScene, LightColor) == 0xf8, "SScene layout");
static_assert(offsetof(SScene, SunMatrix) == 0x12c, "SScene layout");
static_assert(offsetof(SScene, FogColor) == 0x15c, "SScene layout");
static_assert(offsetof(SScene, AtmosphereName) == 0x170, "SScene layout");
static_assert(offsetof(SScene, Models) == 0x178, "SScene layout");
static_assert(offsetof(SScene, FreeModels) == 0x18c, "SScene layout");
static_assert(offsetof(SScene, CellLinks) == 0x1ac, "SScene layout");
static_assert(offsetof(SScene, Terrain) == 0x1c8, "SScene layout");
static_assert(offsetof(SScene, Rivers) == 0x220, "SScene layout");
static_assert(offsetof(SScene, RiverTexture) == 0x234, "SScene layout");
static_assert(offsetof(SScene, Deferred) == 0x2a0, "SScene layout");

// Every live scene (HD: the SGepard scene heap +0x4e4) drops its shadow
// buffer: Gepard option 2 / 4 changes and device resets (0x680910, 0x67fde0).
void ReleaseAllShadowBuffers();

} // namespace pz

#endif // PZ_PZSCENE_H
