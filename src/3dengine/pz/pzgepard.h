// src/3dengine/pz/pzgepard.h
// pz::SPzGepard: the HD SGepard slots the world calls, on top of the SWINE
// SGepard (one D3D9 device, one texture table). OWNER: agent A.
// Skeleton and facade wiring from P0; see docs/MENU3D_INTERFACES.md.

#ifndef PZ_PZGEPARD_H
#define PZ_PZGEPARD_H

#include "igepardhd.h"

namespace pz {

struct SViewport;
struct SPixie;

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
    int        Options[0x20];  // HD SGepard SetOption/GetOption storage
};

} // namespace pz

#endif // PZ_PZGEPARD_H
