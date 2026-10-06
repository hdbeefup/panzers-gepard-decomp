// src/3dengine/pz/igepardhd.h
// SIGepardHD: the HD SGepard interface as the world sees it (RTTI SIGepard
// vftable 0x816d44, SGepard vftable 0x816da8, 24 slots).
//
// The HD world calls the global Gepard (HD 0x8f1c58) through these slots.
// In the recompile the global Gepard is the SWINE SGepard (SIGepard, 170
// virtuals, no slot correspondence), so the HD view is a separate facade
// object, pz::SPzGepard (pzgepard.cpp), on top of the SWINE device and
// texture table. Get it with pz::PzGepard().
//
// SHARED HEADER (owner P0): same rules as iscene.h.

#ifndef PZ_IGEPARDHD_H
#define PZ_IGEPARDHD_H

#include "pzcommon.h"

namespace pz {

struct SIScene;
struct SIViewport;
struct SIPixie;

struct SIGepardHD {
    virtual void AddRef() = 0;                              // +0x00 HD 0x677f10 (0 arg dwords)
    virtual void Release() = 0;                             // +0x04 HD 0x67f640 (0 arg dwords)
    virtual int* Slot_08_Stats(int* out) = 0;               // +0x08 HD 0x67c3d0 (1 arg dword) FPS/polygon text ("FPS: %8.2f /   Polygons")
    virtual SIScene* CreateScene() = 0;                     // +0x0c HD 0x678ed0 (0 arg dwords) new SScene(0x2b0), logs "Scene created"
    virtual void SetOption(unsigned option, int value) = 0; // +0x10 HD 0x680910 (2 arg dwords) SGepard::SetOption
    virtual int GetOption(unsigned option) = 0;             // +0x14 HD 0x67c380 (1 arg dword) SGepard::GetOption
    virtual int GetCap(unsigned cap) = 0;                   // +0x18 HD 0x67a7d0 (1 arg dword) SGepard::GetCap
    virtual void SetBrightness(int level) = 0;              // +0x1c HD 0x680540 (1 arg dword) gamma ramp 0x680540 (PzSetBrightness)
    virtual int LoadModelPrototype(const char* file, float scale, const char* p3, int p4) = 0; // +0x20 HD 0x67db20 (4 arg dwords) .4d -> prototype index; world passes (file, 0.005f, 0, 0)
    virtual void ReleaseModelPrototype(int proto) = 0;      // +0x24 HD 0x67f6d0 (1 arg dword)
    virtual int PurgeModelPrototypes() = 0;                 // +0x28 HD 0x678210 (0 arg dwords) frees prototypes with no users; end of SWorld::LoadMap
    virtual void Slot_2C() = 0;                             // +0x2c HD 0x67bf90 (2 arg dwords)
    virtual void Slot_30() = 0;                             // +0x30 HD 0x67c0a0 (2 arg dwords)
    virtual void Slot_34() = 0;                             // +0x34 HD 0x67c170 (2 arg dwords)
    virtual void SwitchModelPrototypeNodes(int proto, int node1, int node2) = 0; // +0x38 HD 0x681010 (3 arg dwords) SGepard::SwitchModelPrototypeNodes
    virtual SIViewport* GetViewport(int index) = 0;         // +0x3c HD 0x67cb00 (1 arg dword) no AddRef
    virtual void Slot_40() = 0;                             // +0x40 HD 0x67a3f0 (1 arg dword)
    virtual int LoadTexture(const char* file, int mipmap, bool alpha) = 0; // +0x44 HD 0x67ea30 (3 arg dwords) SGepard::LoadTexture; shared with the SWINE 2D path
    virtual void ReleaseTexture(int texture) = 0;           // +0x48 HD 0x67f740 (1 arg dword)
    virtual void UpdateTexture(int texture, int p2, int p3, void* data) = 0; // +0x4c HD 0x681540 (4 arg dwords) SGepard::UpdateTexture
    virtual void GetTextureSize(int texture, int* width, int* height) = 0; // +0x50 HD 0x67ca50 (3 arg dwords) world 0x601c10
    virtual void Slot_54() = 0;                             // +0x54 HD 0x67fb50 (0 arg dwords)
    virtual void SetCachePath(const char* path) = 0;        // +0x58 HD 0x680b30 (1 arg dword) [Paths] Cache (texture DXT cache)
    virtual SIPixie* GetPixie() = 0;                        // +0x5c HD 0x67c3b0 (0 arg dwords) AddRefs the SPixie at SGepard+0x7f4

protected:
    ~SIGepardHD() {}
};

// The facade over the SWINE Gepard. Created on first use, after
// SDXWindow::Create made the SWINE device; never null after that.
SIGepardHD* PzGepard();

// Releases what the facade owns (primary viewport, pixie). Call from
// SSuperWindow::OnDestroy before SDXWindow::OnDestroy releases the device.
void PzGepardShutdown();

} // namespace pz

#endif // PZ_IGEPARDHD_H
