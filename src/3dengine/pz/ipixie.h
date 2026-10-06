// src/3dengine/pz/ipixie.h
// SIPixie: the HD effect manager interface (RTTI SIPixie vftable 0x8797e4,
// 26 slots; SPixie vftable 0x879898 has 27, the last one is SPixie
// only). HD SPixie is 0x7c bytes (ctor 0x6941e0), created in
// SGepard::Initialize and kept at SGepard+0x7f4. Gepard slot +0x5c returns it
// AddRef'd; SSuperWindow::Initialize 0x657910 stores it in HD 0x929f14
// (pz::g_Pixie) and OnDestroy releases it.
//
// Effects are prototypes loaded from .fx property files
// (LoadEffectPrototype) and played into a scene (PlayEffect). Per-effect
// objects implement SIEffect (SEffect vftable 0x883c24, 13 slots; SParticles,
// SEffectSet, SLiteEffect, STrailEffect, SDecalEffect, SSoundEffect,
// SCameraShake, SRain, SSnowfall, SAtmosphere ... share it). Prototype classes
// (SPEffect 0x879850, 6 slots) are internal to agent C.
//
// SHARED HEADER (owner P0): same rules as iscene.h.

#ifndef PZ_IPIXIE_H
#define PZ_IPIXIE_H

#include "pzcommon.h"

namespace pz {

struct SIScene;

struct SIPixie {
    virtual void AddRef() = 0;                              // +0x00 HD 0x69d1d0 (0 arg dwords) RefCount +0x78
    virtual void Release() = 0;                             // +0x04 HD 0x69e8a0 (0 arg dwords) delete 0x7c
    virtual void ReloadResources(int unused) = 0;           // +0x08 HD 0x69edb0 (1 arg dword, not read) every prototype's SPEffect +0x08 (name guessed)
    virtual int RefreshEffectPrototype(int proto) = 0;      // +0x0c HD 0x69e5a0 (1 arg dword) SPixie::RefreshEffectPrototype
    virtual int LoadEffectPrototype(const char* file, bool p2, bool p3, int p4, int p5) = 0; // +0x10 HD 0x69dc80 (5 arg dwords) SPixie::LoadEffectPrototype (.fx)
    virtual void Slot_14() = 0;                             // +0x14 HD 0x69ee50 (2 arg dwords)
    virtual int CreateEffectPrototype() = 0;                // +0x18 HD 0x69d2a0 (0 arg dwords) empty "**untitled.fx" prototype (editor)
    virtual void* GetEffectProperties(int proto) = 0;       // +0x1c HD 0x69d5b0 (1 arg dword) the prototype's SPropertyArray tree
    virtual void ReleaseEffectPrototype(int proto) = 0;     // +0x20 HD 0x69e8c0 (1 arg dword) drops one reference of a LoadEffectPrototype handle
    virtual void PlayEffect(SIScene* scene, int proto, const float* pos, const float* dir, int p5) = 0; // +0x24 HD 0x69e400 (5 arg dwords) one-shot effect, deletes itself when done; p5 = float lifetime override bits (0 = .fx LifeTime). World 0x568bc0/0x57e8b0
    virtual void Slot_28() = 0;                             // +0x28 HD 0x69e510 (5 arg dwords)
    virtual int CreateEffect(SIScene* scene, int proto, const float* pos, const float* dir) = 0; // +0x2c HD 0x69f540 (4 arg dwords) persistent effect; returns the effect handle (-1 on error). World: EEFS 0x5ee9f0, weather 0x6088f0
    virtual void Slot_30() = 0;                             // +0x30 HD 0x69f610 (4 arg dwords)
    virtual void StopEffect(int effect) = 0;                // +0x34 HD 0x69f6b0 (1 arg dword) stop births, the effect dies when its particles do. Weather 0x6088f0
    virtual void ReleaseEffect(int effect) = 0;             // +0x38 HD 0x69d1e0 (1 arg dword) clears SEffectSet+0x30 (persistent): the effect deletes itself once empty
    virtual void DestroyEffect(int effect) = 0;             // +0x3c HD 0x69d540 (1 arg dword) delete now (no fade-out)
    virtual void UnregisterEffect(int effect) = 0;          // +0x40 HD 0x69f710 (1 arg dword) SPixie::UnregisterEffect
    virtual void DestroySceneEffects(SIScene* scene) = 0;   // +0x44 HD 0x69d410 (1 arg dword) every effect of that scene
    virtual void UpdateFrame(int frame) = 0;                // +0x48 HD 0x69f7b0 (1 arg dword) called from scene render 0x6acaf0 with scene +0xa0: simulates every effect once per frame
    virtual void SetEffectPosition(int effect, const float* pos) = 0; // +0x4c HD 0x69f370 (2 arg dwords)
    virtual void SetEffectDirection(int effect, const float* dir) = 0; // +0x50 HD 0x69f2d0 (2 arg dwords)
    virtual void Slot_54() = 0;                             // +0x54 HD 0x69f460 (2 arg dwords)
    virtual void Slot_58() = 0;                             // +0x58 HD 0x69f320 (2 arg dwords)
    virtual bool IsBulletIndicator(int proto) = 0;          // +0x5c HD 0x69dc20 (1 arg dword) "General.Bullet Indicator"
    virtual void SetEffectEnabled(int effect, bool on) = 0; // +0x60 HD 0x69f410 (2 arg dwords) SEffectSet +0x0c
    virtual void Slot_64() = 0;                             // +0x64 HD 0x69f4e0 (3 arg dwords)

protected:
    ~SIPixie() {}
};

// Per-effect instance interface (SEffect vftable 0x883c24). Slot +0x00 is the
// scalar deleting dtor. Names are not known yet; the world never calls these
// directly (it goes through SIPixie), so agent C may name them freely.
struct SIEffect {
    virtual ~SIEffect() {}                // +0x00 HD 0x6de620
    virtual void Slot_04() = 0;           // +0x04 HD 0x6de7a0 (1 arg dword)
    virtual void Slot_08() = 0;           // +0x08 HD 0x6de6d0 (1 arg dword)
    virtual void Slot_0C() = 0;           // +0x0c HD 0x6de6c0 (1 arg dword)
    virtual void Slot_10() = 0;           // +0x10 HD 0x6de7c0 (1 arg dword)
    virtual void Slot_14() = 0;           // +0x14 HD 0x6de7f0 (1 arg dword)
    virtual void Slot_18() = 0;           // +0x18 HD 0x6de810 (2 arg dwords)
    virtual void Slot_1C() = 0;           // +0x1c HD 0x6de6a0 (0 arg dwords) returns bool
    virtual void Slot_20() = 0;           // +0x20 HD 0x6de6b0 (1 arg dword)
    virtual void Slot_24() = 0;           // +0x24 HD 0x6de690 (1 arg dword)
    virtual void Slot_28() = 0;           // +0x28 HD 0x6de7e0 (1 arg dword)
    virtual void Slot_2C() = 0;           // +0x2c HD 0x6de800 (0 arg dwords)
    virtual void Slot_30() = 0;           // +0x30 HD 0x6de7d0 (2 arg dwords)
};

} // namespace pz

#endif // PZ_IPIXIE_H
