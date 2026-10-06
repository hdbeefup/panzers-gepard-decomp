// src/3dengine/pz/pzscene.cpp
// pz::SScene skeleton. OWNER: agent A.
// Stubs log their first call (STUB_LOG) and every call when -menu3d tracing
// is on (PZ_TRACE). Replace a stub with the lifted body and mark it
// "// PANZERS 0xADDR" (drop its STUB_LOG line, so the census moves).

#include <string.h>
#include "pzscene.h"
#include "pzterrain.h"
#include "stub_log.h"

namespace pz {

SScene::SScene(int param)
{
    STUB_LOG("SScene::SScene (0x69faf0)");
    PZ_TRACE("SScene::SScene (0x69faf0)");
    (void)param;
    memset(static_cast<void*>(&RefCount), 0, sizeof(SScene) - offsetof(SScene, RefCount));
    RefCount = 1;   // ctor 0x69faf0: param_1[1] = 1
}

SScene::~SScene()
{
    STUB_LOG("SScene::~SScene (0x6a0440)");
    PZ_TRACE("SScene::~SScene (0x6a0440)");
    delete[] AtmosphereName;
    AtmosphereName = nullptr;
}

// PANZERS 0x6a1f70
void SScene::AddRef()
{
    PZ_TRACE("SScene::AddRef (0x6a1f70)");
    ++RefCount;
}

// PANZERS 0x6ac7f0
void SScene::Release()
{
    PZ_TRACE("SScene::Release (0x6ac7f0)");
    if (--RefCount == 0)
        delete this;   // ~SScene 0x6a0440 + operator delete(0x2b0)
}

// PANZERS 0x6baa80
void SScene::SetAtmosphere(const char* name)
{
    PZ_TRACE("SScene::SetAtmosphere (0x6baa80)");
    // HD: SString::operator=(const char*) 0x52c320 on +0x170.
    delete[] AtmosphereName;
    AtmosphereName = nullptr;
    AtmosphereLen = name ? (int)strlen(name) : 0;
    if (AtmosphereLen) {
        AtmosphereName = new char[AtmosphereLen + 1];
        memcpy(AtmosphereName, name, AtmosphereLen + 1);
    }
}

// PANZERS 0x6a1f80
void SScene::AdvanceTime(int milliseconds)
{
    PZ_TRACE("SScene::AdvanceTime (0x6a1f80)");
    TimeMs += milliseconds;
}

// PANZERS 0x6bab40
void SScene::SetInterpolation(double t)
{
    PZ_TRACE("SScene::SetInterpolation (0x6bab40)");
    Interpolation = t;
}

// PANZERS 0x6baed0
void SScene::SetFocusHeight(float height)
{
    PZ_TRACE("SScene::SetFocusHeight (0x6baed0)");
    FocusHeight = height;
}

void SScene::PrepareViewport(SViewport* vp)
{
    STUB_LOG("SScene::PrepareViewport (0x6bbc40)");
    PZ_TRACE("SScene::PrepareViewport (0x6bbc40)");
    (void)vp;
}

void SScene::UpdateViewport(SViewport* vp)
{
    STUB_LOG("SScene::UpdateViewport (0x6a24c0)");
    PZ_TRACE("SScene::UpdateViewport (0x6a24c0)");
    (void)vp;
}

void SScene::RenderViewport(SViewport* vp)
{
    STUB_LOG("SScene::RenderViewport (0x6acaf0)");
    PZ_TRACE("SScene::RenderViewport (0x6acaf0)");
    (void)vp;
    ++FrameCount;   // HD: *(scene + 0xa0) += 1 at the end
}

void SScene::RenderScene()
{
    STUB_LOG("SScene::RenderScene (0x6b7760)");
    PZ_TRACE("SScene::RenderScene (0x6b7760)");
}

// ---- generated slot stubs (HD vtable order) ----

// HD SScene vtbl +0x0c -> 0x6b0000 (3 arg dwords)
int SScene::Slot_0C_CreateMinimapTarget(int p1, int w2, int h2)
{
    STUB_LOG("SScene::Slot_0C_CreateMinimapTarget (0x6b0000)");
    PZ_TRACE("SScene::Slot_0C_CreateMinimapTarget (0x6b0000)");
    (void)p1; (void)w2; (void)h2;
    return 0;
}

// HD SScene vtbl +0x10 -> 0x6ac7d0 (2 arg dwords)
void SScene::Slot_10()
{
    STUB_LOG("SScene::Slot_10 (0x6ac7d0)");
    PZ_TRACE("SScene::Slot_10 (0x6ac7d0)");
}

// HD SScene vtbl +0x14 -> 0x6ac5b0 (4 arg dwords)
void SScene::Slot_14()
{
    STUB_LOG("SScene::Slot_14 (0x6ac5b0)");
    PZ_TRACE("SScene::Slot_14 (0x6ac5b0)");
}

// HD SScene vtbl +0x18 -> 0x6ac5e0 (3 arg dwords)
void SScene::Slot_18()
{
    STUB_LOG("SScene::Slot_18 (0x6ac5e0)");
    PZ_TRACE("SScene::Slot_18 (0x6ac5e0)");
}

// HD SScene vtbl +0x28 -> 0x6ac6e0 (0 arg dwords)
void SScene::Slot_28()
{
    STUB_LOG("SScene::Slot_28 (0x6ac6e0)");
    PZ_TRACE("SScene::Slot_28 (0x6ac6e0)");
}

// HD SScene vtbl +0x2c -> 0x6a76f0 (7 arg dwords)
int SScene::CreatePointLight(float x, float y, float z, float range, double p5, int p6)
{
    STUB_LOG("SScene::CreatePointLight (0x6a76f0)");
    PZ_TRACE("SScene::CreatePointLight (0x6a76f0)");
    (void)x; (void)y; (void)z; (void)range; (void)p5; (void)p6;
    return -1;
}

// HD SScene vtbl +0x30 -> 0x6a8ee0 (9 arg dwords)
int SScene::CreateSpotLight(float x, float y, float z, float range, double p5, int p6, int p7, int p8)
{
    STUB_LOG("SScene::CreateSpotLight (0x6a8ee0)");
    PZ_TRACE("SScene::CreateSpotLight (0x6a8ee0)");
    (void)x; (void)y; (void)z; (void)range; (void)p5; (void)p6; (void)p7; (void)p8;
    return -1;
}

// HD SScene vtbl +0x34 -> 0x6baca0 (4 arg dwords)
void SScene::SetLightPosition(int light, float x, float y, float z)
{
    STUB_LOG("SScene::SetLightPosition (0x6baca0)");
    PZ_TRACE("SScene::SetLightPosition (0x6baca0)");
    (void)light; (void)x; (void)y; (void)z;
}

// HD SScene vtbl +0x38 -> 0x6aa890 (1 arg dword)
void SScene::DestroyLight(int light)
{
    STUB_LOG("SScene::DestroyLight (0x6aa890)");
    PZ_TRACE("SScene::DestroyLight (0x6aa890)");
    (void)light;
}

// HD SScene vtbl +0x3c -> 0x6ba970 (1 arg dword)
void SScene::SetAmbientLight(const float* rgba)
{
    STUB_LOG("SScene::SetAmbientLight (0x6ba970)");
    PZ_TRACE("SScene::SetAmbientLight (0x6ba970)");
    (void)rgba;
}

// HD SScene vtbl +0x40 -> 0x6abc60 (0 arg dwords)
void SScene::Slot_40()
{
    STUB_LOG("SScene::Slot_40 (0x6abc60)");
    PZ_TRACE("SScene::Slot_40 (0x6abc60)");
}

// HD SScene vtbl +0x44 -> 0x6baef0 (3 arg dwords)
void SScene::SetSunLight(const float* color, float p2, float p3)
{
    STUB_LOG("SScene::SetSunLight (0x6baef0)");
    PZ_TRACE("SScene::SetSunLight (0x6baef0)");
    (void)color; (void)p2; (void)p3;
}

// HD SScene vtbl +0x48 -> 0x6ac1e0 (3 arg dwords)
void SScene::Slot_48()
{
    STUB_LOG("SScene::Slot_48 (0x6ac1e0)");
    PZ_TRACE("SScene::Slot_48 (0x6ac1e0)");
}

// HD SScene vtbl +0x4c -> 0x6baa90 (6 arg dwords)
void SScene::SetFog(float r, float g, float b, float start, float end, float alpha)
{
    STUB_LOG("SScene::SetFog (0x6baa90)");
    PZ_TRACE("SScene::SetFog (0x6baa90)");
    (void)r; (void)g; (void)b; (void)start; (void)end; (void)alpha;
}

// HD SScene vtbl +0x50 -> 0x6a8d50 (4 arg dwords)
SIModel* SScene::CreateModelFromFile(const char* file, float scale, int p3, int p4)
{
    STUB_LOG("SScene::CreateModelFromFile (0x6a8d50)");
    PZ_TRACE("SScene::CreateModelFromFile (0x6a8d50)");
    (void)file; (void)scale; (void)p3; (void)p4;
    return nullptr;
}

// HD SScene vtbl +0x54 -> 0x6a89d0 (3 arg dwords)
SIModel* SScene::CreateModel(int proto, int proto2, bool flag)
{
    STUB_LOG("SScene::CreateModel (0x6a89d0)");
    PZ_TRACE("SScene::CreateModel (0x6a89d0)");
    (void)proto; (void)proto2; (void)flag;
    return nullptr;
}

// HD SScene vtbl +0x58 -> 0x6a8bc0 (2 arg dwords)
SIModel* SScene::CreateModelFromPrototype(int proto, int flag)
{
    STUB_LOG("SScene::CreateModelFromPrototype (0x6a8bc0)");
    PZ_TRACE("SScene::CreateModelFromPrototype (0x6a8bc0)");
    (void)proto; (void)flag;
    return nullptr;
}

// HD SScene vtbl +0x5c -> 0x6ba8a0 (4 arg dwords)
void SScene::Slot_5C()
{
    STUB_LOG("SScene::Slot_5C (0x6ba8a0)");
    PZ_TRACE("SScene::Slot_5C (0x6ba8a0)");
}

// HD SScene vtbl +0x60 -> 0x6ba810 (2 arg dwords)
void SScene::ReplaceModel(SIModel* model, int proto)
{
    STUB_LOG("SScene::ReplaceModel (0x6ba810)");
    PZ_TRACE("SScene::ReplaceModel (0x6ba810)");
    (void)model; (void)proto;
}

// HD SScene vtbl +0x64 -> 0x6a9dd0 (3 arg dwords)
SITerrain* SScene::CreateTerrain(int width, int height, int p3)
{
    STUB_LOG("SScene::CreateTerrain (0x6a9dd0)");
    PZ_TRACE("SScene::CreateTerrain (0x6a9dd0)");
    (void)width; (void)height; (void)p3;
    return nullptr;
}

// HD SScene vtbl +0x68 -> 0x6aab20 (0 arg dwords)
void SScene::DestroyTerrain()
{
    STUB_LOG("SScene::DestroyTerrain (0x6aab20)");
    PZ_TRACE("SScene::DestroyTerrain (0x6aab20)");
}

// HD SScene vtbl +0x6c -> 0x6a9000 (5 arg dwords)
void SScene::Slot_6C()
{
    STUB_LOG("SScene::Slot_6C (0x6a9000)");
    PZ_TRACE("SScene::Slot_6C (0x6a9000)");
}

// HD SScene vtbl +0x70 -> 0x6aa8e0 (1 arg dword)
void SScene::Slot_70()
{
    STUB_LOG("SScene::Slot_70 (0x6aa8e0)");
    PZ_TRACE("SScene::Slot_70 (0x6aa8e0)");
}

// HD SScene vtbl +0x74 -> 0x6bad60 (2 arg dwords)
void SScene::Slot_74()
{
    STUB_LOG("SScene::Slot_74 (0x6bad60)");
    PZ_TRACE("SScene::Slot_74 (0x6bad60)");
}

// HD SScene vtbl +0x78 -> 0x6badd0 (5 arg dwords)
void SScene::Slot_78()
{
    STUB_LOG("SScene::Slot_78 (0x6badd0)");
    PZ_TRACE("SScene::Slot_78 (0x6badd0)");
}

// HD SScene vtbl +0x7c -> 0x6a9850 (15 arg dwords)
void SScene::Slot_7C()
{
    STUB_LOG("SScene::Slot_7C (0x6a9850)");
    PZ_TRACE("SScene::Slot_7C (0x6a9850)");
}

// HD SScene vtbl +0x80 -> 0x6bb680 (6 arg dwords)
void SScene::Slot_80()
{
    STUB_LOG("SScene::Slot_80 (0x6bb680)");
    PZ_TRACE("SScene::Slot_80 (0x6bb680)");
}

// HD SScene vtbl +0x84 -> 0x6aaad0 (0 arg dwords)
void SScene::Slot_84()
{
    STUB_LOG("SScene::Slot_84 (0x6aaad0)");
    PZ_TRACE("SScene::Slot_84 (0x6aaad0)");
}

// HD SScene vtbl +0x88 -> 0x6a2780 (1 arg dword)
void SScene::Slot_88()
{
    STUB_LOG("SScene::Slot_88 (0x6a2780)");
    PZ_TRACE("SScene::Slot_88 (0x6a2780)");
}

// HD SScene vtbl +0x8c -> 0x6a7790 (6 arg dwords)
void SScene::Slot_8C()
{
    STUB_LOG("SScene::Slot_8C (0x6a7790)");
    PZ_TRACE("SScene::Slot_8C (0x6a7790)");
}

// HD SScene vtbl +0x90 -> 0x6bb320 (4 arg dwords)
void SScene::Slot_90()
{
    STUB_LOG("SScene::Slot_90 (0x6bb320)");
    PZ_TRACE("SScene::Slot_90 (0x6bb320)");
}

// HD SScene vtbl +0x94 -> 0x6a2710 (1 arg dword)
void SScene::Slot_94()
{
    STUB_LOG("SScene::Slot_94 (0x6a2710)");
    PZ_TRACE("SScene::Slot_94 (0x6a2710)");
}

// HD SScene vtbl +0x98 -> 0x6aa320 (1 arg dword)
void SScene::Slot_98()
{
    STUB_LOG("SScene::Slot_98 (0x6aa320)");
    PZ_TRACE("SScene::Slot_98 (0x6aa320)");
}

// HD SScene vtbl +0x9c -> 0x6a7940 (8 arg dwords)
int SScene::CreateLake(const char* p1, const char* p2, float p3, float p4, short* p5, int p6, short* p7, int p8)
{
    STUB_LOG("SScene::CreateLake (0x6a7940)");
    PZ_TRACE("SScene::CreateLake (0x6a7940)");
    (void)p1; (void)p2; (void)p3; (void)p4; (void)p5; (void)p6; (void)p7; (void)p8;
    return -1;
}

// HD SScene vtbl +0xa0 -> 0x6aa3f0 (1 arg dword)
void SScene::DestroyLake(int lake)
{
    STUB_LOG("SScene::DestroyLake (0x6aa3f0)");
    PZ_TRACE("SScene::DestroyLake (0x6aa3f0)");
    (void)lake;
}

// HD SScene vtbl +0xa4 -> 0x6a1f90 (2 arg dwords)
void SScene::Slot_A4()
{
    STUB_LOG("SScene::Slot_A4 (0x6a1f90)");
    PZ_TRACE("SScene::Slot_A4 (0x6a1f90)");
}

// HD SScene vtbl +0xa8 -> 0x6aabe0 (0 arg dwords)
void SScene::Slot_A8()
{
    STUB_LOG("SScene::Slot_A8 (0x6aabe0)");
    PZ_TRACE("SScene::Slot_A8 (0x6aabe0)");
}

// HD SScene vtbl +0xac -> 0x6abcb0 (5 arg dwords)
void SScene::Slot_AC()
{
    STUB_LOG("SScene::Slot_AC (0x6abcb0)");
    PZ_TRACE("SScene::Slot_AC (0x6abcb0)");
}

// HD SScene vtbl +0xb0 -> 0x6a90e0 (16 arg dwords)
void SScene::Slot_B0()
{
    STUB_LOG("SScene::Slot_B0 (0x6a90e0)");
    PZ_TRACE("SScene::Slot_B0 (0x6a90e0)");
}

// HD SScene vtbl +0xb4 -> 0x6aa910 (1 arg dword)
void SScene::Slot_B4()
{
    STUB_LOG("SScene::Slot_B4 (0x6aa910)");
    PZ_TRACE("SScene::Slot_B4 (0x6aa910)");
}

// HD SScene vtbl +0xb8 -> 0x6ac0b0 (1 arg dword)
void SScene::Slot_B8()
{
    STUB_LOG("SScene::Slot_B8 (0x6ac0b0)");
    PZ_TRACE("SScene::Slot_B8 (0x6ac0b0)");
}

// HD SScene vtbl +0xbc -> 0x6ac150 (2 arg dwords)
void SScene::Slot_BC()
{
    STUB_LOG("SScene::Slot_BC (0x6ac150)");
    PZ_TRACE("SScene::Slot_BC (0x6ac150)");
}

// HD SScene vtbl +0xc0 -> 0x6abfc0 (5 arg dwords)
void SScene::Slot_C0()
{
    STUB_LOG("SScene::Slot_C0 (0x6abfc0)");
    PZ_TRACE("SScene::Slot_C0 (0x6abfc0)");
}

// HD SScene vtbl +0xc4 -> 0x6a9ee0 (3 arg dwords)
void SScene::Slot_C4()
{
    STUB_LOG("SScene::Slot_C4 (0x6a9ee0)");
    PZ_TRACE("SScene::Slot_C4 (0x6a9ee0)");
}

// HD SScene vtbl +0xc8 -> 0x6aa050 (4 arg dwords)
void SScene::Slot_C8()
{
    STUB_LOG("SScene::Slot_C8 (0x6aa050)");
    PZ_TRACE("SScene::Slot_C8 (0x6aa050)");
}

// HD SScene vtbl +0xcc -> 0x6c0b40 (2 arg dwords)
void SScene::UpdateWire(int wire, int* p2)
{
    STUB_LOG("SScene::UpdateWire (0x6c0b40)");
    PZ_TRACE("SScene::UpdateWire (0x6c0b40)");
    (void)wire; (void)p2;
}

// HD SScene vtbl +0xd0 -> 0x6aaba0 (1 arg dword)
void SScene::Slot_D0()
{
    STUB_LOG("SScene::Slot_D0 (0x6aaba0)");
    PZ_TRACE("SScene::Slot_D0 (0x6aaba0)");
}

// HD SScene vtbl +0xd4 -> 0x6aa280 (0 arg dwords)
void SScene::Slot_D4()
{
    STUB_LOG("SScene::Slot_D4 (0x6aa280)");
    PZ_TRACE("SScene::Slot_D4 (0x6aa280)");
}

// HD SScene vtbl +0xd8 -> 0x6a1d80 (5 arg dwords)
void SScene::Slot_D8()
{
    STUB_LOG("SScene::Slot_D8 (0x6a1d80)");
    PZ_TRACE("SScene::Slot_D8 (0x6a1d80)");
}

// HD SScene vtbl +0xdc -> 0x6a1cd0 (8 arg dwords)
void SScene::Slot_DC()
{
    STUB_LOG("SScene::Slot_DC (0x6a1cd0)");
    PZ_TRACE("SScene::Slot_DC (0x6a1cd0)");
}

// HD SScene vtbl +0xe0 -> 0x6a14f0 (6 arg dwords)
void SScene::Slot_E0()
{
    STUB_LOG("SScene::Slot_E0 (0x6a14f0)");
    PZ_TRACE("SScene::Slot_E0 (0x6a14f0)");
}

// HD SScene vtbl +0xe4 -> 0x6a1b00 (5 arg dwords)
void SScene::Slot_E4()
{
    STUB_LOG("SScene::Slot_E4 (0x6a1b00)");
    PZ_TRACE("SScene::Slot_E4 (0x6a1b00)");
}

// HD SScene vtbl +0xe8 -> 0x6a1790 (6 arg dwords)
void SScene::Slot_E8()
{
    STUB_LOG("SScene::Slot_E8 (0x6a1790)");
    PZ_TRACE("SScene::Slot_E8 (0x6a1790)");
}

// HD SScene vtbl +0xec -> 0x6a24e0 (0 arg dwords)
void SScene::Slot_EC()
{
    STUB_LOG("SScene::Slot_EC (0x6a24e0)");
    PZ_TRACE("SScene::Slot_EC (0x6a24e0)");
}

// HD SScene vtbl +0xf0 -> 0x6a24f0 (1 arg dword)
void SScene::Slot_F0()
{
    STUB_LOG("SScene::Slot_F0 (0x6a24f0)");
    PZ_TRACE("SScene::Slot_F0 (0x6a24f0)");
}

// HD SScene vtbl +0xf4 -> 0x6aabf0 (1 arg dword)
void SScene::Slot_F4()
{
    STUB_LOG("SScene::Slot_F4 (0x6aabf0)");
    PZ_TRACE("SScene::Slot_F4 (0x6aabf0)");
}

// HD SScene vtbl +0xf8 -> 0x6abca0 (0 arg dwords)
void SScene::Slot_F8()
{
    STUB_LOG("SScene::Slot_F8 (0x6abca0)");
    PZ_TRACE("SScene::Slot_F8 (0x6abca0)");
}

// HD SScene vtbl +0xfc -> 0x6a96c0 (2 arg dwords)
void SScene::SetSkybox(const char* file, float radius)
{
    STUB_LOG("SScene::SetSkybox (0x6a96c0)");
    PZ_TRACE("SScene::SetSkybox (0x6a96c0)");
    (void)file; (void)radius;
}

// HD SScene vtbl +0x100 -> 0x6aa9a0 (0 arg dwords)
void SScene::ClearSkybox()
{
    STUB_LOG("SScene::ClearSkybox (0x6aa9a0)");
    PZ_TRACE("SScene::ClearSkybox (0x6aa9a0)");
}

// HD SScene vtbl +0x104 -> 0x6ba950 (0 arg dwords)
void SScene::Slot_104_RecreateShadowBuffer()
{
    STUB_LOG("SScene::Slot_104_RecreateShadowBuffer (0x6ba950)");
    PZ_TRACE("SScene::Slot_104_RecreateShadowBuffer (0x6ba950)");
}

} // namespace pz
