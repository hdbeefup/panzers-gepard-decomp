// src/3dengine/pz/pzscene.h
// pz::SScene: HD SScene (0x2b0 bytes, ctor 0x69faf0, dtor 0x6a0440).
// OWNER: agent A.

#ifndef PZ_PZSCENE_H
#define PZ_PZSCENE_H

#include "iscene.h"
#include "pmodel.h"
#include "propertystruct.h"

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

// Ground trail segment (track marks; HD SHeap value 0x38, entry 0x3c). One
// quad from the previous track point to the current one, each widened by
// the trail's half width across its direction, on the terrain.
struct SGroundTrailSegment {
    float V[4][3];     // +0x00 prev +w, cur +w, prev -w, cur -w (triangle strip)
    int   StartTime;   // +0x30 scene +0xa4 when made (ms)
    bool  Noticed;     // +0x34 seen once on the visibility map (then always drawn)
};
static_assert(sizeof(SGroundTrailSegment) == 0x38, "HD segment entry 0x3c");

// Ground trail (HD SHeap value 0x30, entry 0x34, scene +0x1f8).
struct SGroundTrail {
    SHeap<SGroundTrailSegment>* Segments;   // +0x00 new 0x14
    int   Texture;      // +0x04 (Gepard AddRef'd)
    bool  Closed;       // +0x08 (+0x94): removed once no segment is left
    float Strength;     // +0x0c strength * 256 (alpha at age 0)
    float FadeMs;       // +0x10 segment lifetime
    float HalfWidth;    // +0x14
    float VScale;       // +0x18 (not read)
    float LastX;        // +0x1c
    float LastZ;        // +0x20
    float LastDir;      // +0x24
    bool  HaveLast;     // +0x28
    int   DrawType;     // +0x2c blend mode (0x688980)
};
static_assert(sizeof(SGroundTrail) == 0x30, "HD trail entry 0x34");

// Terrain-cell model link (scene +0x1ac SDArray, 8 bytes).
struct SCellLink {
    int Model;
    int Next;
};

// Smoke trail point (HD SDArray stride 0x1c).
struct SSmokeTrailPoint {
    float Pos[3];      // +0x00
    float V;           // +0x0c texture V (VScale per unit along the trail)
    float Alpha;       // +0x10 alpha scale (TrackSmokeTrail p5)
    float Width;       // +0x14 half width added to the second track (p6)
    int   Time;        // +0x18 scene +0xa4 when placed (ms)
};
static_assert(sizeof(SSmokeTrailPoint) == 0x1c, "HD smoke-trail point 0x1c");
struct SSmokeTrailPoints {     // HD SDArray (new 0xc)
    SSmokeTrailPoint* Data;
    int Count;
    int Max;
};

// Rectangle outline (HD SHeap value 0x14, entry 0x18, scene +0x1cc): a
// line strip around (X0, Z0) - (X1, Z1) on the terrain grid (0x6b04d0).
struct SOutlineRect {
    unsigned Color;   // +0x00
    int X0, Z0;       // +0x04
    int X1, Z1;       // +0x0c
};

// Smoke trail (HD SHeap value 0x38, entry 0x3c, scene +0x1e0).
struct SSmokeTrail {
    SSmokeTrailPoints* Points;          // +0x00
    int   Texture;                      // +0x04 (Gepard AddRef'd)
    bool  Closed;                       // +0x08 (+0x88): removed once every point faded
    unsigned Color;                     // +0x0c RGB (draw type 0)
    float Strength;                     // +0x10 strength * 256 (not read by the draw)
    float FadeSpeed;                    // +0x14 (not read)
    float UScale;                       // +0x18 (not read)
    float VScale;                       // +0x1c V per unit of length
    int   DrawType;                     // +0x20 blend mode: 0 alpha (Color), 1 additive (grey)
    const STrackFloat* AlphaTrack;      // +0x24 the SPTrailEffect's "Alpha"
    const STrackFloat* WidthTrack;      // +0x28 its second track (width over age)
    int   AlphaCursor;                  // +0x2c
    int   WidthCursor;                  // +0x30
    float InvDuration;                  // +0x34 1 / Duration (age in s * this = track time)
};
static_assert(sizeof(SSmokeTrail) == 0x38, "HD smoke-trail entry 0x3c");

// Wire point (HD SDArray stride 0x18).
struct SWirePoint {
    float    Pos[3];   // +0x00
    unsigned Color;    // +0x0c
    float    Dist;     // +0x10 length along the wire (texture U)
    float    Sag;      // +0x14 cosh(sag) - cosh(sag * t): scales the wind sway
};
static_assert(sizeof(SWirePoint) == 0x18, "HD wire point 0x18");

// Wire (HD SHeap value 0x34, entry 0x38, scene +0x238).
struct SWire {
    float From[3];        // +0x00
    float To[3];          // +0x0c
    float Length;         // +0x18 |To - From|; (int)Length + 1 segments
    float SagParam;       // +0x1c catenary parameter
    float Width;          // +0x20 half width of the ribbon
    SWirePoint* Points;   // +0x24 SDArray {data, count, max}
    int   Count;          // +0x28
    int   Max;            // +0x2c
    bool  Visible;        // +0x30 this frame (DrawWires)
};
static_assert(sizeof(SWire) == 0x34, "HD wire entry 0x38");

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
    int CreateSmokeTrail(int texture, float x, float y, float z, unsigned color, float strength,
                         float fadeSpeed, float uScale, float vScale, bool additive,
                         const void* alphaTrack, const void* track2, float duration,
                         float p14, float p15) override;
    void TrackSmokeTrail(int trail, float x, float y, float z, float p5, float p6) override;
    void DestroyAllSmokeTrails() override;
    void CloseSmokeTrail(int trail) override;
    int CreateGroundTrail(int texture, float strength, float fadeMs, float halfWidth, float vScale, int drawType) override;
    void TrackGroundTrail(int trail, float x, float z, float dir) override;
    void CloseGroundTrail(int trail) override;
    void RemoveGroundTrail(int trail) override;
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
    int CreateWire(const float* from, const float* to, float sag, float width) override;
    void UpdateWire(int wire, int* p2) override;
    void DestroyWire(int wire) override;
    void Slot_D4() override;
    void Slot_D8() override;
    void Slot_DC() override;
    void Slot_E0() override;
    void Slot_E4() override;
    void Slot_E8() override;
    void ClearLines() override;
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
    void RenderScene(SViewport* vp);       // 0x6b7760 (HD: the current viewport 0x67bf40)

    // Other non-virtual HD members.
    void UpdateModels(SViewport* vp, bool all);     // 0x6bbcb0
    void SortModelsIntoCells();                     // 0x6ac210
    void DrawModels(SViewport* vp, int p2);         // 0x6b02a0
    void DrawRivers(SViewport* vp);                 // 0x6b0920
    void DrawOutlines(SViewport* vp);               // 0x6b04d0 (pzscene_wx.cpp)
    void DrawLines(SViewport* vp);                  // 0x6acf20 (pzscene_wx.cpp)
    void DrawGroundTrails(SViewport* vp);           // 0x6ad0e0
    void DestroySmokeTrail(int trail);              // 0x6aa9e0 (SParticles dtor, DestroyAllSmokeTrails)
    void DrawSmokeTrails(SViewport* vp);            // 0x6b7b30
    void DrawSkybox(SViewport* vp, unsigned fogColor); // 0x6b7920
    void DrawWires(SViewport* vp, bool shadowPass); // 0x6b8b10
    void RebuildWire(int wire);                     // 0x6c05d0 catenary points
    float TerrainHeight2(float x, float z) const;   // terrain 0x6f4d10 (0 without a terrain)
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
    SHeap<SOutlineRect> Outlines;        // +0x1cc rectangle outlines (drawn by 0x6b04d0)
    SHeap<SSmokeTrail> SmokeTrails;      // +0x1e0 (+0x7c..+0x88, drawn by 0x6b7b30)
    int           SmokeTrailVB;      // +0x1f4 dynamic VB format 0x142 (-1: not made yet)
    SHeap<SGroundTrail> GroundTrails;    // +0x1f8 track marks (+0x8c..+0x98, drawn by 0x6ad0e0)
    unsigned char _20c[0x220 - 0x20c];   // lake heap (+0x20c)
    SHeap<unsigned char[0x74]> Rivers;   // +0x220 water courses (Slot_B0), drawn by 0x6b0920
    int           RiverTexture;      // +0x234
    SHeap<SWire>  Wires;             // +0x238 (+0xc8..+0xd0, drawn by 0x6b8b10)
    unsigned char _24c[0x260 - 0x24c];   // heap +0x24c (not on the M3 path)
    int           WireVB;            // +0x260 dynamic VB format 0x142 (-1: not made yet)
    int           WireTexture;       // +0x264 wire/wire_a.tga
    int           WireShadowTexture; // +0x268 wire/wire_shadow.tga
    int           WireDecl;          // +0x26c vertex declaration for the shadow shader
    void*         Lines;             // +0x270 SDArray of 0x20-byte debug lines (0x6acf20)
    int           LineCount;         // +0x274
    int           LineMax;           // +0x278
    int           _27c;              // +0x27c (ctor 3)
    bool          SkyboxOn;          // +0x280
    unsigned char _281[3];
    int           SkyboxTextures[6]; // +0x284 front right back left top bottom
    float         SkyboxRadius;      // +0x29c
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
static_assert(offsetof(SScene, SmokeTrails) == 0x1e0, "SScene layout");
static_assert(offsetof(SScene, SmokeTrailVB) == 0x1f4, "SScene layout");
static_assert(offsetof(SScene, GroundTrails) == 0x1f8, "SScene layout");
static_assert(offsetof(SScene, Wires) == 0x238, "SScene layout");
static_assert(offsetof(SScene, WireVB) == 0x260, "SScene layout");
static_assert(offsetof(SScene, Lines) == 0x270, "SScene layout");
static_assert(offsetof(SScene, SkyboxOn) == 0x280, "SScene layout");
static_assert(offsetof(SScene, SkyboxRadius) == 0x29c, "SScene layout");
static_assert(offsetof(SScene, Rivers) == 0x220, "SScene layout");
static_assert(offsetof(SScene, RiverTexture) == 0x234, "SScene layout");
static_assert(offsetof(SScene, Deferred) == 0x2a0, "SScene layout");

// Every live scene (HD: the SGepard scene heap +0x4e4) drops its shadow
// buffer: Gepard option 2 / 4 changes and device resets (0x680910, 0x67fde0).
void ReleaseAllShadowBuffers();

} // namespace pz

#endif // PZ_PZSCENE_H
