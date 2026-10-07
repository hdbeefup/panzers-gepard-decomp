// src/3dengine/pz/pzgepard.h
// pz::SPzGepard: the HD SGepard slots the world calls, on top of the SWINE
// SGepard (one D3D9 device, one texture table). OWNER: agent A.
//
// Besides the 24 interface slots this header carries the non-virtual HD
// SGepard members the HD 3D path calls (render helpers, dynamic vertex
// buffer, prototype/animation heaps). They act on the facade singleton.

#ifndef PZ_PZGEPARD_H
#define PZ_PZGEPARD_H

#include "igepardhd.h"
#include "pmodel.h"
#include "hdbitmap.h"

struct IDirect3DDevice9;
struct IDirect3DVertexBuffer9;
struct IDirect3DPixelShader9;
struct IDirect3DVertexShader9;

namespace pz {

struct SViewport;
struct SPixie;
struct SPAnim;

// HD SBitmap: hdbitmap.h

// The HD SGepard state the HD scene path reads (HD offsets in comments).
struct SGepardHDState {
    IDirect3DDevice9*       Device;         // +0x478 (the SWINE device)
    unsigned                TransformStages;// +0x47c texture-transform stages handled (<= 5)
    unsigned                SamplerStages;  // +0x480 sampler stages handled (<= 5)
    unsigned                BlendStages;    // +0x484 texture-blend stages handled (<= 5)
    bool                    PixelShader1x;  // HD: PS id 2 created (+0x75c); see SMaterial::Begin
    int                     PolyCount;      // +0x4ac per-frame stats
    int                     VertexCount;    // +0x4b0
    int                     PassCount;      // +0x4b4
    int                     LightCount;     // +0x54c
    // Dynamic vertex buffer (+0x5d4..+0x5e4) and its vertex types (+0x5e8).
    IDirect3DVertexBuffer9* DynVB;          // +0x5d4
    unsigned                DynVBSize;      // +0x5d8 (0x1fffe0)
    bool                    DynVBLocked;    // +0x5dc
    int                     DynVBType;      // +0x5e0
    unsigned                DynVBOffset;    // +0x5e4
    unsigned                DynTypeFvf[8];
    unsigned                DynTypeStride[8];
    int                     DynTypeCount;
    int                     DynType112;     // +0x5fc vertex type of FVF 0x112 (32 bytes)
    // Shader caps and the shadow technique (SGepard::Initialize 0x67d1c0).
    unsigned                VSVersion;      // +0x488 D3DCAPS9 VertexShaderVersion & 0xffff
    unsigned                PSVersion;      // +0x48c D3DCAPS9 PixelShaderVersion & 0xffff
    unsigned                VendorId;       // +0x434 adapter identifier
    unsigned                DeviceId;       // +0x438
    bool                    DepthTextureFlag; // +0x499 (0 after the depth-texture choice)
    int                     ShadowCap;      // +0x540 GetCap(0): 1 compatible, 2 depth texture, 3 PS 1.4, 4 PS 2.0
    int                     DebugShadowTexture; // +0x548 option 4 (editor/shadow_buffer_256_hq.tga)
    IDirect3DPixelShader9*  PixelShaders[0x23];   // +0x754 + id*4, ids 1..0x22
    IDirect3DVertexShader9* VertexShaders[0x43];  // +0x648 + id*4, ids 1..0x42 (HD creates 0x41/0x42)
};
SGepardHDState& HD();

// HD SGepard non-virtual members used by the 3D path.
void GepardSetTexture(unsigned stage, int texture);                // 0x680c90
void GepardSetVertexFormat(int decl, unsigned fvf);                // 0x680e80
void GepardSetWorld(const float* m3x4);                            // 0x680fb0
void GepardSetWorldIdentity();                                     // 0x680fe0
void GepardSetVertexShader(int id);                                // 0x680f40
void GepardSetPixelShader(int id);                                 // 0x680ac0
int  GepardGetTextureAlpha(int texture);                           // 0x67c9e0
void* GepardLockDynamicVB(int type, int count);                    // 0x67f150
void GepardUnlockDynamicVB();                                      // 0x681440
void GepardDrawDynamicVB(int prim, int minIndex, int numVerts, int startIndex,
                         int primCount, int withIndices);          // 0x67a650
void GepardAdvanceDynamicVB(int count);                            // 0x677fc0
void GepardSetAmbient(const float* rgba);                          // 0x680510
unsigned GepardArgb(const float* rgba);                            // 0x5aa900 clamped float4 -> ARGB
SPModel* GepardModelPrototype(int index);                          // 0x6778a0
SPAnim*  GepardAnim(int index);                                    // 0x67a760
int      GepardLoadAnim(const char* file);                         // 0x67d920
void     GepardReleaseAnim(int index);
int      GepardOption(unsigned option);                            // 0x67c380 (no trace)
SIPixie* GepardPixie();                                            // SGepard+0x7f4, no AddRef
void     GepardSetPixelShaderConstant(unsigned reg, const float* v, unsigned count); // device +0x1b4
void     GepardSetVertexShaderConstant(unsigned reg, const float* v, unsigned count); // device +0x178
void     GepardEnableLights(bool on);                              // 0x67a720 (the scene's lights)

// Offscreen viewports (HD SViewport 0x244 with a render-target texture in
// the SGepard viewport heap +0x578). The recompile keeps only what the
// shadow buffer needs: a colour render-target texture and a depth surface
// (or a depth texture), both D3DPOOL_DEFAULT, released before a device
// reset (HD: SGepard::ResetDevice 0x67fde0 frees every scene's shadow
// buffer through 0x6a2670) and recreated on the next use.
int  GepardCreateRenderTarget(int width, int height, unsigned format, unsigned flags); // 0x678c70 (0x68aa60, 0x689710)
void GepardDestroyRenderTarget(int index);                         // Gepard +0x40 0x67a3f0
int  GepardRenderTargetTexture(int index, bool depth);             // 0x67cb60 +0x20 / +0x24
void GepardSetTextureFilter(int texture, unsigned flags);          // 0x680bf0
void GepardSelectRenderTarget(int index);                          // 0x6803e0 SGepard::SelectViewport
void GepardUnselectRenderTarget();                                 // 0x6814b0 SGepard::UnselectViewport
void GepardClearRenderTarget(unsigned color, float z, unsigned stencil); // 0x689f10 on the selected one
SHdBitmap* GepardReadRenderTarget(int index, int format);            // 0x6c0e50 lock + 0x669be0 copy + 0x6c1030 unlock (lockable targets, flags 1)

struct SPzGepard : SIGepardHD {
    SPzGepard();
    ~SPzGepard();

    void AddRef() override;
    void Release() override;
    int* Slot_08_Stats(int* out) override;
    SIScene* CreateScene() override;
    void SetOption(unsigned option, int value) override;
    int GetOption(unsigned option) override;
    int GetCap(unsigned cap) override;
    void SetBrightness(int level) override;
    int LoadModelPrototype(const char* file, float scale, const char* p3, int p4) override;
    void ReleaseModelPrototype(int proto) override;
    int PurgeModelPrototypes() override;
    void Slot_2C() override;
    void Slot_30() override;
    void Slot_34() override;
    void SwitchModelPrototypeNodes(int proto, int node1, int node2) override;
    SIViewport* GetViewport(int index) override;
    void DestroyViewport(int index) override;
    int LoadTexture(const char* file, int mipmap, bool alpha) override;
    void ReleaseTexture(int texture) override;
    void UpdateTexture(int texture, int p2, int p3, void* data) override;
    void GetTextureSize(int texture, int* width, int* height) override;
    void Slot_54() override;
    void SetCachePath(const char* path) override;
    SIPixie* GetPixie() override;

    int        RefCount;
    SViewport* Primary;        // HD SGepard viewport list [0] (SViewport*, created with the device)
    SPixie*    Pixie;          // HD SGepard+0x7f4
    int        Options[0x20];  // HD SGepard SetOption/GetOption storage (+0x4f8, 0x12 used)
    SHeap<SPModel*> Prototypes;   // HD +0x550
    SHeap<SPAnim*>  Anims;        // HD +0x564
};

} // namespace pz

#endif // PZ_PZGEPARD_H
