// src/3dengine/pz/pzgepard.cpp
// The Gepard facade: HD SGepard slots (vftable 0x816da8) over the SWINE
// SGepard. OWNER: agent A. Skeleton and facade wiring from P0.
//
// Shared with the SWINE 2D path:
//   - the D3D9 device (SGepard::lpD3DDev): the HD scene draws inside the
//     SWINE frame through SGepard::PanzersScenePass (pzviewport.cpp);
//   - textures: LoadTexture/ReleaseTexture forward to the SWINE texture table,
//     so a handle from the 3D path is valid for the board and the reverse.
// Owned here: the primary viewport facade and the SPixie (HD SGepard+0x7f4).
//
// Device creation is unchanged (SWINE SGepard::Initialize). HD creates the
// device with BehaviorFlags 0x40 (HARDWARE_VERTEXPROCESSING) or 0x20
// (SOFTWARE), never D3DCREATE_FPU_PRESERVE (0x67d1c0 writes SGepard+0x458,
// 0x68ada0 passes it to CreateDevice); SWINE does the same. See
// docs/MENU3D_INTERFACES.md "FPU".

#include <string.h>
#include "pzgepard.h"
#include "pzviewport.h"
#include "pzscene.h"
#include "pzpixie.h"
#include "gepard.h"
#include "core_common.h"
#include "logger.h"
#include "stub_log.h"

extern SIGepard* Gepard;   // SWINE renderer (window/widget.h)

namespace pz {

void ViewportScenePass(IDirect3DDevice9* dev);   // pzviewport.cpp

static SPzGepard* s_Facade = nullptr;

SIGepardHD* PzGepard()
{
    if (!s_Facade) {
        s_Facade = new SPzGepard();
        SGepard::PanzersScenePass = &ViewportScenePass;
        if (Logger.g)
            Logger.g->Log(0, "pz: Gepard facade created (HD scene pass hooked into SGepard::RenderScene)");
    }
    return s_Facade;
}

void PzGepardShutdown()
{
    if (!s_Facade)
        return;
    SGepard::PanzersScenePass = nullptr;
    delete s_Facade;
    s_Facade = nullptr;
}

SPzGepard::SPzGepard()
    : RefCount(1), Primary(nullptr), Pixie(nullptr)
{
    memset(Options, 0, sizeof(Options));
}

SPzGepard::~SPzGepard()
{
    if (Pixie) {
        Pixie->Release();   // the facade's own reference (HD SGepard dtor)
        Pixie = nullptr;
    }
    delete Primary;
    Primary = nullptr;
}

void SPzGepard::AddRef()
{
    PZ_TRACE("SGepard::AddRef (0x677f10)");
    ++RefCount;   // HD 0x677f10; the facade is never deleted by Release
}

void SPzGepard::Release()
{
    PZ_TRACE("SGepard::Release (0x67f640)");
    // HD 0x67f640 deletes SGepard at 0; the SWINE Gepard is released by
    // SDXWindow::OnDestroy, the facade by PzGepardShutdown.
    if (RefCount > 0)
        --RefCount;
}

SIScene* SPzGepard::CreateScene()
{
    PZ_TRACE("SGepard::CreateScene (0x678ed0)");
    // HD 0x678ed0: index = scene heap +0x4e4 alloc; new SScene(index) (0x2b0,
    // ctor 0x69faf0); stored in the heap. The heap is not kept here.
    SScene* scene = new SScene(0);
    if (Logger.g)
        Logger.g->Log(0, "Scene created");
    return scene;
}

void SPzGepard::SetOption(unsigned option, int value)
{
    PZ_TRACE("SGepard::SetOption (0x680910)");
    // HD 0x680910 also applies options (3 = shadow buffer size reloads
    // editor/shadow_buffer_*.tga ...). Stored only; the SWINE renderer has
    // no counterpart (see PzStub_ApplyGraphicsOptions).
    if (option < sizeof(Options) / sizeof(Options[0]))
        Options[option] = value;
}

int SPzGepard::GetOption(unsigned option)
{
    PZ_TRACE("SGepard::GetOption (0x67c380)");
    return option < sizeof(Options) / sizeof(Options[0]) ? Options[option] : 0;
}

int SPzGepard::GetCap(unsigned cap)
{
    PZ_TRACE("SGepard::GetCap (0x67a7d0)");
    // HD: SGepard+0x540[cap]. Cap 0 = shadow technique, 2 on a PS 2.0 card
    // (the value optionsmenu.cpp assumes too).
    return cap == 0 ? 2 : 0;
}

int SPzGepard::LoadModelPrototype(const char* file, float scale, const char* p3, int p4)
{
    STUB_LOG("SGepard::LoadModelPrototype (0x67db20)");
    PZ_TRACE("SGepard::LoadModelPrototype (0x67db20)");
    (void)scale; (void)p3; (void)p4;
    if (g_Menu3D.Trace && Logger.g)
        Logger.g->Log(0, "  model prototype: %s", file ? file : "(null)");
    return -1;   // HD: index into SGepard+0x550 (SPModel, 0x54) or -1
}

SIViewport* SPzGepard::GetViewport(int index)
{
    PZ_TRACE("SGepard::GetViewport (0x67cb00)");
    if (index != 0) {
        if (Logger.g)
            Logger.g->Log(0, "pz: GetViewport(%d): only the primary viewport exists", index);
        return nullptr;
    }
    if (!Primary)
        Primary = new SViewport();
    return Primary;
}

int SPzGepard::LoadTexture(const char* file, int mipmap, bool alpha)
{
    PZ_TRACE("SGepard::LoadTexture (0x67ea30)");
    // Same texture table as the board (SWINE SGepard::LoadTexture). HD adds
    // the _hq DXT cache (docs/ENGINE_DIFF.md section 2).
    return ::Gepard ? ::Gepard->LoadTexture(file, mipmap != 0, alpha) : -1;
}

void SPzGepard::ReleaseTexture(int texture)
{
    PZ_TRACE("SGepard::ReleaseTexture (0x67f740)");
    if (::Gepard && texture >= 0)
        ::Gepard->ReleaseTexture(texture, false);
}

SIPixie* SPzGepard::GetPixie()
{
    PZ_TRACE("SGepard::GetPixie (0x67c3b0)");
    // HD: SPixie is created in SGepard::Initialize 0x67d1c0 (new 0x7c); the
    // facade creates it on first use. 0x67c3b0 AddRefs before returning.
    if (!Pixie)
        Pixie = new SPixie();
    Pixie->AddRef();
    return Pixie;
}

// ---- generated slot stubs (HD vtable order) ----

// HD SPzGepard vtbl +0x08 -> 0x67c3d0 (1 arg dword)
int* SPzGepard::Slot_08_Stats(int* out)
{
    STUB_LOG("SPzGepard::Slot_08_Stats (0x67c3d0)");
    PZ_TRACE("SPzGepard::Slot_08_Stats (0x67c3d0)");
    (void)out;
    return nullptr;
}

// HD SPzGepard vtbl +0x1c -> 0x680540 (1 arg dword)
void SPzGepard::SetBrightness(int level)
{
    STUB_LOG("SPzGepard::SetBrightness (0x680540)");
    PZ_TRACE("SPzGepard::SetBrightness (0x680540)");
    (void)level;
}

// HD SPzGepard vtbl +0x24 -> 0x67f6d0 (1 arg dword)
void SPzGepard::ReleaseModelPrototype(int proto)
{
    STUB_LOG("SPzGepard::ReleaseModelPrototype (0x67f6d0)");
    PZ_TRACE("SPzGepard::ReleaseModelPrototype (0x67f6d0)");
    (void)proto;
}

// HD SPzGepard vtbl +0x28 -> 0x678210 (0 arg dwords)
int SPzGepard::PurgeModelPrototypes()
{
    STUB_LOG("SPzGepard::PurgeModelPrototypes (0x678210)");
    PZ_TRACE("SPzGepard::PurgeModelPrototypes (0x678210)");
    return 0;
}

// HD SPzGepard vtbl +0x2c -> 0x67bf90 (2 arg dwords)
void SPzGepard::Slot_2C()
{
    STUB_LOG("SPzGepard::Slot_2C (0x67bf90)");
    PZ_TRACE("SPzGepard::Slot_2C (0x67bf90)");
}

// HD SPzGepard vtbl +0x30 -> 0x67c0a0 (2 arg dwords)
void SPzGepard::Slot_30()
{
    STUB_LOG("SPzGepard::Slot_30 (0x67c0a0)");
    PZ_TRACE("SPzGepard::Slot_30 (0x67c0a0)");
}

// HD SPzGepard vtbl +0x34 -> 0x67c170 (2 arg dwords)
void SPzGepard::Slot_34()
{
    STUB_LOG("SPzGepard::Slot_34 (0x67c170)");
    PZ_TRACE("SPzGepard::Slot_34 (0x67c170)");
}

// HD SPzGepard vtbl +0x38 -> 0x681010 (3 arg dwords)
void SPzGepard::SwitchModelPrototypeNodes(int proto, int node1, int node2)
{
    STUB_LOG("SPzGepard::SwitchModelPrototypeNodes (0x681010)");
    PZ_TRACE("SPzGepard::SwitchModelPrototypeNodes (0x681010)");
    (void)proto; (void)node1; (void)node2;
}

// HD SPzGepard vtbl +0x40 -> 0x67a3f0 (1 arg dword)
void SPzGepard::Slot_40()
{
    STUB_LOG("SPzGepard::Slot_40 (0x67a3f0)");
    PZ_TRACE("SPzGepard::Slot_40 (0x67a3f0)");
}

// HD SPzGepard vtbl +0x4c -> 0x681540 (4 arg dwords)
void SPzGepard::UpdateTexture(int texture, int p2, int p3, void* data)
{
    STUB_LOG("SPzGepard::UpdateTexture (0x681540)");
    PZ_TRACE("SPzGepard::UpdateTexture (0x681540)");
    (void)texture; (void)p2; (void)p3; (void)data;
}

// HD SPzGepard vtbl +0x50 -> 0x67ca50 (3 arg dwords)
void SPzGepard::GetTextureSize(int texture, int* width, int* height)
{
    STUB_LOG("SPzGepard::GetTextureSize (0x67ca50)");
    PZ_TRACE("SPzGepard::GetTextureSize (0x67ca50)");
    (void)texture; (void)width; (void)height;
}

// HD SPzGepard vtbl +0x54 -> 0x67fb50 (0 arg dwords)
void SPzGepard::Slot_54()
{
    STUB_LOG("SPzGepard::Slot_54 (0x67fb50)");
    PZ_TRACE("SPzGepard::Slot_54 (0x67fb50)");
}

// HD SPzGepard vtbl +0x58 -> 0x680b30 (1 arg dword)
void SPzGepard::SetCachePath(const char* path)
{
    STUB_LOG("SPzGepard::SetCachePath (0x680b30)");
    PZ_TRACE("SPzGepard::SetCachePath (0x680b30)");
    (void)path;
}

} // namespace pz
