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

struct IDirect3DDevice9;
struct IDirect3DVertexBuffer9;

namespace pz {

struct SViewport;
struct SPixie;
struct SPAnim;

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
SPModel* GepardModelPrototype(int index);                          // 0x6778a0
SPAnim*  GepardAnim(int index);                                    // 0x67a760
int      GepardLoadAnim(const char* file);                         // 0x67d920
void     GepardReleaseAnim(int index);
int      GepardOption(unsigned option);                            // 0x67c380 (no trace)
SIPixie* GepardPixie();                                            // SGepard+0x7f4, no AddRef

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
    void Slot_40() override;
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
