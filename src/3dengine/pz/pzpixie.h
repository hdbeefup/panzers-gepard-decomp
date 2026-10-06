// src/3dengine/pz/pzpixie.h
// pz::SPixie: HD SPixie, the effect manager (0x7c bytes, ctor 0x6941e0,
// dtor 0x69cb90). One instance, owned by the Gepard facade (HD SGepard+0x7f4,
// HD global 0x92f104). OWNER: agent C.
//
// Prototypes come from .fx property files (LoadEffectPrototype +0x10) and are
// played into a scene (PlayEffect +0x24 one-shot, CreateEffect +0x2c
// persistent). Handles are heap indices: prototype handles index
// Prototypes (+0x08), effect handles index Effects (+0x1c).
//
// Per frame, SScene::RenderViewport 0x6acaf0 (agent A) calls
//   pixie->UpdateFrame(scene->FrameCount);         // vtbl +0x48, 0x69f7b0
//   pixie->Render(scene, viewport);                // 0x69ea50, non-virtual
// after the models, with the world transform set to identity by Render.

#ifndef PZ_PZPIXIE_H
#define PZ_PZPIXIE_H

#include "ipixie.h"
#include "effect.h"

namespace pz {

struct SIViewport;
struct SFlare;   // HD SFlare (lens flares, EffectType 1): not ported

// HD deferred play request (0x28, list at SPixie+0x64), drained by UpdateFrame.
struct SPixieDeferredPlay {
    SPixieDeferredPlay* Next;   // +0x00
    SPixieDeferredPlay* Prev;   // +0x04
    SIScene* Scene;             // +0x08
    int   Proto;                // +0x0c
    float Pos[3];               // +0x10
    float Dir[3];               // +0x1c
};

struct SPixie : SIPixie {
    SPixie();    // 0x6941e0
    ~SPixie();   // 0x69cb90 (non-virtual: deleted by Release)

    static SPixie* Instance();   // HD 0x92f104 (set by the ctor)

    void AddRef() override;
    void Release() override;
    void ReloadResources(int unused) override;
    int RefreshEffectPrototype(int proto) override;
    int LoadEffectPrototype(const char* file, bool keepProperties, bool checkTimeStamp, int unitFile, int unitFileLen) override;
    void Slot_14() override;
    int CreateEffectPrototype() override;
    void* GetEffectProperties(int proto) override;
    void ReleaseEffectPrototype(int proto) override;
    void PlayEffect(SIScene* scene, int proto, const float* pos, const float* dir, int p5) override;
    void PlayEffectOnNode(SIScene* scene, int proto, SIModel* model, int node, int p5) override;
    int CreateEffect(SIScene* scene, int proto, const float* pos, const float* dir) override;
    int CreateEffectOnNode(SIScene* scene, int proto, SIModel* model, int node) override;
    void StopEffect(int effect) override;
    void ReleaseEffect(int effect) override;
    void DestroyEffect(int effect) override;
    void UnregisterEffect(int effect) override;
    void DestroySceneEffects(SIScene* scene) override;
    void UpdateFrame(int frame) override;
    void SetEffectPosition(int effect, const float* pos) override;
    void SetEffectDirection(int effect, const float* dir) override;
    void SetEffectParam18(int effect, int p) override;
    void SetEffectModel(int effect, int model) override;
    void SetEffectAlphaScale(int effect, float s);   // 0x69f280 (sub-effect particles)
    void SetEffectSizeScale(int effect, float s);    // 0x69f3c0
    bool IsBulletIndicator(int proto) override;
    void SetEffectEnabled(int effect, bool on) override;
    void SetEffectSpeed(int effect, float p1, float p2) override;
    // +0x68 HD 0x69d6a0 (3 arg dwords) SPixie only (not in SIPixie).
    // Builds one SPEffect from an "N.Data" struct; returns its index in the
    // prototype set or -1.
    virtual int InitEffectPrototype(int proto, const char* name, SPropertyStruct* data);

    // HD 0x69ea50 (thiscall, RET 8): draws every effect of `scene`, priority
    // layer 0, 1 then 2, then the lens flares. Called by SScene::RenderViewport
    // 0x6acaf0 right after pixie +0x48. Agent A calls this.
    void Render(SIScene* scene, SIViewport* vp);

    void RemovePrototype(int proto);   // 0x69f780 (-> 0x69e9f0)

    // --- HD layout (offsets asserted below) ---
    void*        Device;           // +0x04 SGepard+0x478 (IDirect3DDevice)
    SPtrHeap<SPEffectSet> Prototypes; // +0x08
    SPtrHeap<SEffectSet>  Effects;    // +0x1c
    bool         _30;              // +0x30
    int          ParticleVB;       // +0x34 SGepard dynamic VB, FVF 0x1c4 (XYZRHW|DIFFUSE|SPECULAR|TEX1)
    int          WorldVB;          // +0x38 FVF 0x142 (XYZ|DIFFUSE|TEX1)
    SPtrHeap<SFlare> Flares;       // +0x3c
    int          FlareVB;          // +0x50 FVF 0x1c4
    int          _54;              // +0x54
    float        RainIntensity;    // +0x58 (births of "Birth in rain = 0" effects stop above 15)
    float        _5c;              // +0x5c
    float        WindPhase;        // +0x60 rand() / 32768 * 6.283
    SPixieDeferredPlay* DeferredHead;   // +0x64
    SPixieDeferredPlay* DeferredTail;   // +0x68
    SPixieDeferredPlay* DeferredCursor; // +0x6c
    bool         _70;              // +0x70
    int          DeferredCount;    // +0x74
    int          RefCount;         // +0x78
};
PZ_HD_SIZE(SPixie, kHdSizeSPixie);
static_assert(offsetof(SPixie, Prototypes) == 0x08, "SPixie layout");
static_assert(offsetof(SPixie, Effects) == 0x1c, "SPixie layout");
static_assert(offsetof(SPixie, Flares) == 0x3c, "SPixie layout");
static_assert(offsetof(SPixie, DeferredHead) == 0x64, "SPixie layout");
static_assert(offsetof(SPixie, RefCount) == 0x78, "SPixie layout");

} // namespace pz

#endif // PZ_PZPIXIE_H
