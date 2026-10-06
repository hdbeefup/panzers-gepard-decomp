// src/3dengine/pz/iscene.h
// SIScene: the HD scene interface (RTTI SIScene vftable 0x87bf6c, SScene
// vftable 0x87c078, 66 slots). The world reaches the scene only
// through these slots, through the global g_Scene (HD 0x929a54).
//
// SHARED HEADER (owner P0, see docs/MENU3D_INTERFACES.md):
//   - Slot order is the HD vtable order. Never reorder, insert or remove a
//     slot, and never overload a virtual name (MSVC groups overloads in the
//     vtable). To name a Slot_XX, rename it in place and keep the comment.
//   - Comments: +offset, HD implementation address, number of argument
//     dwords taken from the RET imm16 of that function (a double counts 2),
//     then notes. "(name guessed)" = semantics from one caller only.
//   - Lifetime: refcounted. SGepard CreateScene returns RefCount 1 (ctor
//     0x69faf0 sets +0x04 = 1); Release at 0 runs ~SScene 0x6a0440 and
//     deletes 0x2b0 bytes.

#ifndef PZ_ISCENE_H
#define PZ_ISCENE_H

#include "pzcommon.h"

namespace pz {

struct SIModel;
struct SITerrain;

struct SIScene {
    virtual void AddRef() = 0;                              // +0x00 HD 0x6a1f70 (0 arg dwords)
    virtual void Release() = 0;                             // +0x04 HD 0x6ac7f0 (0 arg dwords)
    virtual void SetAtmosphere(const char* name) = 0;       // +0x08 HD 0x6baa80 (tail call) SString at +0x170; ATMS chunk / SWorld ctor
    virtual int Slot_0C_CreateMinimapTarget(int p1, int w2, int h2) = 0; // +0x0c HD 0x6b0000 (3 arg dwords) SGameLogic ctor: (0, w*2, h*2) -> handle (name guessed)
    virtual void Slot_10() = 0;                             // +0x10 HD 0x6ac7d0 (2 arg dwords)
    virtual void Slot_14() = 0;                             // +0x14 HD 0x6ac5b0 (4 arg dwords)
    virtual void Slot_18() = 0;                             // +0x18 HD 0x6ac5e0 (3 arg dwords)
    virtual void AdvanceTime(int milliseconds) = 0;         // +0x1c HD 0x6a1f80 (1 arg dword) +0xa4 += ms; SSuperWindow::OnIdle
    virtual void SetInterpolation(double t) = 0;            // +0x20 HD 0x6bab40 (2 arg dwords) +0xb8; (MenuNextTick - now) * 20
    virtual void SetFocusHeight(float height) = 0;          // +0x24 HD 0x6baed0 (1 arg dword) +0x98; SWorld::ComputeCamera
    virtual void Slot_28() = 0;                             // +0x28 HD 0x6ac6e0 (0 arg dwords)
    virtual int CreatePointLight(float x, float y, float z, float range, double p5, int p6) = 0; // +0x2c HD 0x6a76f0 (7 arg dwords) (name guessed)
    virtual int CreateSpotLight(float x, float y, float z, float range, double p5, int p6, int p7, int p8) = 0; // +0x30 HD 0x6a8ee0 (9 arg dwords) (name guessed)
    virtual void SetLightPosition(int light, float x, float y, float z) = 0; // +0x34 HD 0x6baca0 (4 arg dwords) SScene::SetLightPosition
    virtual void DestroyLight(int light) = 0;               // +0x38 HD 0x6aa890 (1 arg dword) SScene::DestroyLight
    virtual void SetAmbientLight(const float* rgba) = 0;    // +0x3c HD 0x6ba970 (1 arg dword) weather 0x6088f0
    virtual void Slot_40() = 0;                             // +0x40 HD 0x6abc60 (0 arg dwords)
    virtual void SetSunLight(const float* color, float p2, float p3) = 0; // +0x44 HD 0x6baef0 (3 arg dwords) weather 0x6088f0 (param names guessed)
    virtual void Slot_48() = 0;                             // +0x48 HD 0x6ac1e0 (3 arg dwords)
    virtual void SetFog(float r, float g, float b, float start, float end, float alpha) = 0; // +0x4c HD 0x6baa90 (6 arg dwords) weather 0x6088f0
    virtual SIModel* CreateModelFromFile(const char* file, float scale, int p3, int p4) = 0; // +0x50 HD 0x6a8d50 (4 arg dwords) SDoodad::Initialize 0x5ee040 (file, 0.005f, 0, 0)
    virtual SIModel* CreateModel(int proto, int proto2, bool flag) = 0; // +0x54 HD 0x6a89d0 (3 arg dwords) SScene::CreateModel (new SModel 0x150)
    virtual SIModel* CreateModelFromPrototype(int proto, int flag) = 0; // +0x58 HD 0x6a8bc0 (2 arg dwords) SScene::CreateModel; SGameLogic::CreateAnimatedModel 0x5649e0
    virtual void Slot_5C() = 0;                             // +0x5c HD 0x6ba8a0 (4 arg dwords)
    virtual void ReplaceModel(SIModel* model, int proto) = 0; // +0x60 HD 0x6ba810 (2 arg dwords) SScene::ReplaceModel
    virtual SITerrain* CreateTerrain(int width, int height, int p3) = 0; // +0x64 HD 0x6a9dd0 (3 arg dwords) new STerrain(0x11c88); TERR loader 0x5f2fc0 (w, h, 4)
    virtual void DestroyTerrain() = 0;                      // +0x68 HD 0x6aab20 (0 arg dwords) TERR loader on size change
    virtual void Slot_6C() = 0;                             // +0x6c HD 0x6a9000 (5 arg dwords)
    virtual void Slot_70() = 0;                             // +0x70 HD 0x6aa8e0 (1 arg dword)
    virtual void Slot_74() = 0;                             // +0x74 HD 0x6bad60 (2 arg dwords)
    virtual void Slot_78() = 0;                             // +0x78 HD 0x6badd0 (5 arg dwords)
    virtual void Slot_7C() = 0;                             // +0x7c HD 0x6a9850 (15 arg dwords)
    virtual void Slot_80() = 0;                             // +0x80 HD 0x6bb680 (6 arg dwords)
    virtual void Slot_84() = 0;                             // +0x84 HD 0x6aaad0 (0 arg dwords)
    virtual void Slot_88() = 0;                             // +0x88 HD 0x6a2780 (1 arg dword)
    virtual void Slot_8C() = 0;                             // +0x8c HD 0x6a7790 (6 arg dwords)
    virtual void Slot_90() = 0;                             // +0x90 HD 0x6bb320 (4 arg dwords)
    virtual void Slot_94() = 0;                             // +0x94 HD 0x6a2710 (1 arg dword)
    virtual void Slot_98() = 0;                             // +0x98 HD 0x6aa320 (1 arg dword)
    virtual int CreateLake(const char* p1, const char* p2, float p3, float p4, short* p5, int p6, short* p7, int p8) = 0; // +0x9c HD 0x6a7940 (8 arg dwords) SScene::CreateLake (menu.map has none)
    virtual void DestroyLake(int lake) = 0;                 // +0xa0 HD 0x6aa3f0 (1 arg dword) world 0x607ad0 (name guessed)
    virtual void Slot_A4() = 0;                             // +0xa4 HD 0x6a1f90 (2 arg dwords)
    virtual void Slot_A8() = 0;                             // +0xa8 HD 0x6aabe0 (0 arg dwords)
    virtual void Slot_AC() = 0;                             // +0xac HD 0x6abcb0 (5 arg dwords)
    virtual void Slot_B0() = 0;                             // +0xb0 HD 0x6a90e0 (16 arg dwords)
    virtual void Slot_B4() = 0;                             // +0xb4 HD 0x6aa910 (1 arg dword)
    virtual void Slot_B8() = 0;                             // +0xb8 HD 0x6ac0b0 (1 arg dword)
    virtual void Slot_BC() = 0;                             // +0xbc HD 0x6ac150 (2 arg dwords)
    virtual void Slot_C0() = 0;                             // +0xc0 HD 0x6abfc0 (5 arg dwords)
    virtual void Slot_C4() = 0;                             // +0xc4 HD 0x6a9ee0 (3 arg dwords)
    virtual void Slot_C8() = 0;                             // +0xc8 HD 0x6aa050 (4 arg dwords)
    virtual void UpdateWire(int wire, int* p2) = 0;         // +0xcc HD 0x6c0b40 (2 arg dwords) SScene::UpdateWire
    virtual void Slot_D0() = 0;                             // +0xd0 HD 0x6aaba0 (1 arg dword)
    virtual void Slot_D4() = 0;                             // +0xd4 HD 0x6aa280 (0 arg dwords)
    virtual void Slot_D8() = 0;                             // +0xd8 HD 0x6a1d80 (5 arg dwords)
    virtual void Slot_DC() = 0;                             // +0xdc HD 0x6a1cd0 (8 arg dwords)
    virtual void Slot_E0() = 0;                             // +0xe0 HD 0x6a14f0 (6 arg dwords)
    virtual void Slot_E4() = 0;                             // +0xe4 HD 0x6a1b00 (5 arg dwords)
    virtual void Slot_E8() = 0;                             // +0xe8 HD 0x6a1790 (6 arg dwords)
    virtual void Slot_EC() = 0;                             // +0xec HD 0x6a24e0 (0 arg dwords)
    virtual void Slot_F0() = 0;                             // +0xf0 HD 0x6a24f0 (1 arg dword)
    virtual void Slot_F4() = 0;                             // +0xf4 HD 0x6aabf0 (1 arg dword)
    virtual void Slot_F8() = 0;                             // +0xf8 HD 0x6abca0 (0 arg dwords)
    virtual void SetSkybox(const char* file, float radius) = 0; // +0xfc HD 0x6a96c0 (2 arg dwords) KSYB 0x5fec10 (radius 500)
    virtual void ClearSkybox() = 0;                         // +0x100 HD 0x6aa9a0 (0 arg dwords) KSYB 0x5fec10
    virtual void Slot_104_RecreateShadowBuffer() = 0;       // +0x104 HD 0x6ba950 (0 arg dwords) after Gepard SetOption(3) in Graphics Apply (name guessed)

protected:
    ~SIScene() {}
};

} // namespace pz

#endif // PZ_ISCENE_H
