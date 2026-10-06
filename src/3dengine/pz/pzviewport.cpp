// src/3dengine/pz/pzviewport.cpp
// pz::SViewport skeleton and the 2D/3D frame order. OWNER: agent A.
//
// Frame order (HD SViewport::Render 0x68c220, per subport):
//   TestCooperativeLevel (lost -> skip; not reset -> SGepard::ResetDevice
//     0x67fde0, board +0xc4)
//   if (scene && viewport+0x241) { scene 0x6bbc40(vp); scene 0x6a24c0(vp);
//     BeginScene; scene 0x6acaf0(vp) (it clears with SViewport::Clear
//     0x689f10 inside SScene::RenderScene 0x6b7760) }
//   else { Clear(TARGET|ZBUFFER[|STENCIL], clearColor, 1.0, 0); BeginScene }
//   board: viewport+0x240 ? SBoard render 0x6c7150 : software cursor 0x6ca240
//   EndScene; Present 0x68bfe0
// So the 3D scene and the 2D board share one BeginScene/EndScene, scene
// first. Callers: SDXWindow::OnIdle 0x53a0d0 (scene = SDXWindow+0xe0) every
// frame, SWorld::LoadMap 0x5f1990 (scene 0: loading frame).
//
// Recompile: SWINE SGepard::RenderScene already does the device-lost check,
// Clear, BeginScene, board, EndScene and Present. Render() runs the two
// pre-scene calls, then SGepard::RenderScene with SGepard::PanzersScenePass
// set to ScenePass below, which runs scene 0x6acaf0 right before the board.
// ScenePass saves and restores all device state around the HD scene
// (D3DSBT_ALL state block), so HD fixed-function state cannot leak into the
// SWINE board.

#include <d3d9.h>
#include "pzviewport.h"
#include "pzscene.h"
#include "igepard.h"
#include "core_common.h"
#include "logger.h"
#include "stub_log.h"

extern SIGepard* Gepard;   // SWINE renderer (window/widget.h)
extern SIBoard* Board;

namespace pz {

static SScene*    s_FrameScene = nullptr;
static SViewport* s_FrameViewport = nullptr;

// Installed as SGepard::PanzersScenePass by the Gepard facade.
void ViewportScenePass(IDirect3DDevice9* dev)
{
    if (!s_FrameScene)
        return;
    IDirect3DStateBlock9* saved = nullptr;
    if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &saved)))
        saved = nullptr;
    s_FrameScene->RenderViewport(s_FrameViewport);   // 0x6acaf0
    if (saved) {
        saved->Apply();
        saved->Release();
    }
}

SViewport::SViewport()
    : Camera(), ProjectionSet(false)
{
}

SViewport::~SViewport()
{
}

void SViewport::SetCamera(float x, float y, float z, float yaw, float pitch)
{
    STUB_LOG("SViewport::SetCamera (0x68d370)");
    PZ_TRACE("SViewport::SetCamera (0x68d370)");
    // HD also builds the view matrix here (camera offset +0xc0.. added).
    Camera.X = x;
    Camera.Y = y;
    Camera.Z = z;
    Camera.Yaw = yaw;
    Camera.Pitch = pitch;
}

void SViewport::GetCamera(float* x, float* y, float* z, float* yaw, float* pitch)
{
    STUB_LOG("SViewport::GetCamera (0x68be30)");
    PZ_TRACE("SViewport::GetCamera (0x68be30)");
    if (x) *x = Camera.X;
    if (y) *y = Camera.Y;
    if (z) *z = Camera.Z;
    if (yaw) *yaw = Camera.Yaw;
    if (pitch) *pitch = Camera.Pitch;
}

void SViewport::SetProjection(float fovRadians, float nearZ, float farZ)
{
    STUB_LOG("SViewport::SetProjection (0x68cfd0)");
    PZ_TRACE("SViewport::SetProjection (0x68cfd0)");
    // HD: +0xcc fov, +0xd0 near, +0xd4 far, projection 1/tan(fov/2), aspect
    // 4:3 in full-screen mode 1, else back buffer w/h.
    Camera.Fov = fovRadians;
    Camera.NearZ = nearZ;
    Camera.FarZ = farZ;
    ProjectionSet = true;
}

SIBoard* SViewport::GetBoard()
{
    PZ_TRACE("SViewport::GetBoard (0x68b730)");
    // HD 0x68b730 AddRefs SViewport+0x28; the SWINE board has no refcount.
    return ::Board;
}

void SViewport::Render(SIScene* scene, unsigned clearColor)
{
    PZ_TRACE("SViewport::Render (0x68c220)");
    // clearColor: HD clears with it only when there is no scene. SWINE clears
    // with its fog colour (black in the menu) every frame.
    (void)clearColor;
    SScene* s = static_cast<SScene*>(scene);
    if (s) {
        s->PrepareViewport(this);   // 0x6bbc40
        s->UpdateViewport(this);    // 0x6a24c0
    }
    s_FrameScene = s;
    s_FrameViewport = this;
    if (::Gepard)
        ::Gepard->RenderScene(false);   // Clear, BeginScene, [ScenePass], board, EndScene, Present
    s_FrameScene = nullptr;
    s_FrameViewport = nullptr;
    TraceNextFrame();
}

// ---- generated slot stubs (HD vtable order) ----

// HD SViewport vtbl +0x00 -> 0x68d0d0 (4 arg dwords)
void SViewport::SetPosition(int x, int y, int w, int h)
{
    STUB_LOG("SViewport::SetPosition (0x68d0d0)");
    PZ_TRACE("SViewport::SetPosition (0x68d0d0)");
    (void)x; (void)y; (void)w; (void)h;
}

// HD SViewport vtbl +0x04 -> 0x68c4e0 (2 arg dwords)
void SViewport::Resize(int width, int height)
{
    STUB_LOG("SViewport::Resize (0x68c4e0)");
    PZ_TRACE("SViewport::Resize (0x68c4e0)");
    (void)width; (void)height;
}

// HD SViewport vtbl +0x08 -> 0x68ca00 (8 arg dwords)
void SViewport::SetFullScreenMode(int w, int h, int bpp, bool vsync, int refresh, int aa, int aaq, bool p8)
{
    STUB_LOG("SViewport::SetFullScreenMode (0x68ca00)");
    PZ_TRACE("SViewport::SetFullScreenMode (0x68ca00)");
    (void)w; (void)h; (void)bpp; (void)vsync; (void)refresh; (void)aa; (void)aaq; (void)p8;
}

// HD SViewport vtbl +0x0c -> 0x68d6d0 (4 arg dwords)
void SViewport::SetWindowedMode(int x, int y, int w, int h)
{
    STUB_LOG("SViewport::SetWindowedMode (0x68d6d0)");
    PZ_TRACE("SViewport::SetWindowedMode (0x68d6d0)");
    (void)x; (void)y; (void)w; (void)h;
}

// HD SViewport vtbl +0x10 -> 0x68b800 (4 arg dwords)
void SViewport::Slot_10()
{
    STUB_LOG("SViewport::Slot_10 (0x68b800)");
    PZ_TRACE("SViewport::Slot_10 (0x68b800)");
}

// HD SViewport vtbl +0x14 -> 0x68b8b0 (3 arg dwords)
bool SViewport::NextFullScreenMode(int* w, int* h, int p3)
{
    STUB_LOG("SViewport::NextFullScreenMode (0x68b8b0)");
    PZ_TRACE("SViewport::NextFullScreenMode (0x68b8b0)");
    (void)w; (void)h; (void)p3;
    return false;
}

// HD SViewport vtbl +0x18 -> 0x68bcc0 (3 arg dwords)
bool SViewport::PrevFullScreenMode(int* w, int* h, int p3)
{
    STUB_LOG("SViewport::PrevFullScreenMode (0x68bcc0)");
    PZ_TRACE("SViewport::PrevFullScreenMode (0x68bcc0)");
    (void)w; (void)h; (void)p3;
    return false;
}

// HD SViewport vtbl +0x1c -> 0x68d310 (1 arg dword)
void SViewport::Slot_1C()
{
    STUB_LOG("SViewport::Slot_1C (0x68d310)");
    PZ_TRACE("SViewport::Slot_1C (0x68d310)");
}

// HD SViewport vtbl +0x2c -> 0x68cf00 (6 arg dwords)
void SViewport::Slot_2C()
{
    STUB_LOG("SViewport::Slot_2C (0x68cf00)");
    PZ_TRACE("SViewport::Slot_2C (0x68cf00)");
}

// HD SViewport vtbl +0x30 -> 0x68ce50 (4 arg dwords)
void SViewport::Slot_30()
{
    STUB_LOG("SViewport::Slot_30 (0x68ce50)");
    PZ_TRACE("SViewport::Slot_30 (0x68ce50)");
}

// HD SViewport vtbl +0x34 -> 0x689c20 (3 arg dwords)
void SViewport::Slot_34()
{
    STUB_LOG("SViewport::Slot_34 (0x689c20)");
    PZ_TRACE("SViewport::Slot_34 (0x689c20)");
}

// HD SViewport vtbl +0x38 -> 0x6898b0 (5 arg dwords)
void SViewport::Slot_38()
{
    STUB_LOG("SViewport::Slot_38 (0x6898b0)");
    PZ_TRACE("SViewport::Slot_38 (0x6898b0)");
}

// HD SViewport vtbl +0x3c -> 0x68dda0 (7 arg dwords)
void SViewport::Slot_3C()
{
    STUB_LOG("SViewport::Slot_3C (0x68dda0)");
    PZ_TRACE("SViewport::Slot_3C (0x68dda0)");
}

// HD SViewport vtbl +0x40 -> 0x68da50 (8 arg dwords)
void SViewport::Slot_40()
{
    STUB_LOG("SViewport::Slot_40 (0x68da50)");
    PZ_TRACE("SViewport::Slot_40 (0x68da50)");
}

// HD SViewport vtbl +0x44 -> 0x68b940 (2 arg dwords)
void SViewport::Slot_44()
{
    STUB_LOG("SViewport::Slot_44 (0x68b940)");
    PZ_TRACE("SViewport::Slot_44 (0x68b940)");
}

// HD SViewport vtbl +0x48 -> 0x68bbf0 (tail call)
void SViewport::Slot_48()
{
    STUB_LOG("SViewport::Slot_48 (0x68bbf0)");
    PZ_TRACE("SViewport::Slot_48 (0x68bbf0)");
}

// HD SViewport vtbl +0x54 -> 0x68ab40 (4 arg dwords)
int SViewport::Slot_54_SelectSubport(int p1, int p2, int p3, int p4)
{
    STUB_LOG("SViewport::Slot_54_SelectSubport (0x68ab40)");
    PZ_TRACE("SViewport::Slot_54_SelectSubport (0x68ab40)");
    (void)p1; (void)p2; (void)p3; (void)p4;
    return 0;
}

// HD SViewport vtbl +0x58 -> 0x68b0f0 (1 arg dword)
void SViewport::Slot_58_SelectSubport(int index)
{
    STUB_LOG("SViewport::Slot_58_SelectSubport (0x68b0f0)");
    PZ_TRACE("SViewport::Slot_58_SelectSubport (0x68b0f0)");
    (void)index;
}

// HD SViewport vtbl +0x5c -> 0x68bde0 (1 arg dword)
void SViewport::Slot_5C()
{
    STUB_LOG("SViewport::Slot_5C (0x68bde0)");
    PZ_TRACE("SViewport::Slot_5C (0x68bde0)");
}

// HD SViewport vtbl +0x60 -> 0x68b930 (0 arg dwords)
void SViewport::Slot_60()
{
    STUB_LOG("SViewport::Slot_60 (0x68b930)");
    PZ_TRACE("SViewport::Slot_60 (0x68b930)");
}

// HD SViewport vtbl +0x64 -> 0x68b470 (1 arg dword)
void SViewport::Slot_64()
{
    STUB_LOG("SViewport::Slot_64 (0x68b470)");
    PZ_TRACE("SViewport::Slot_64 (0x68b470)");
}

// HD SViewport vtbl +0x68 -> 0x68b220 (3 arg dwords)
void SViewport::Slot_68()
{
    STUB_LOG("SViewport::Slot_68 (0x68b220)");
    PZ_TRACE("SViewport::Slot_68 (0x68b220)");
}

// HD SViewport vtbl +0x6c -> 0x68b3a0 (4 arg dwords)
void SViewport::Slot_6C()
{
    STUB_LOG("SViewport::Slot_6C (0x68b3a0)");
    PZ_TRACE("SViewport::Slot_6C (0x68b3a0)");
}

// HD SViewport vtbl +0x70 -> 0x68b350 (4 arg dwords)
void SViewport::Slot_70()
{
    STUB_LOG("SViewport::Slot_70 (0x68b350)");
    PZ_TRACE("SViewport::Slot_70 (0x68b350)");
}

// HD SViewport vtbl +0x74 -> 0x68b520 (1 arg dword)
void SViewport::Slot_74()
{
    STUB_LOG("SViewport::Slot_74 (0x68b520)");
    PZ_TRACE("SViewport::Slot_74 (0x68b520)");
}

// HD SViewport vtbl +0x78 -> 0x68b5b0 (4 arg dwords)
void SViewport::FrontBufferScreenshot(int p1, int p2, int p3, int p4)
{
    STUB_LOG("SViewport::FrontBufferScreenshot (0x68b5b0)");
    PZ_TRACE("SViewport::FrontBufferScreenshot (0x68b5b0)");
    (void)p1; (void)p2; (void)p3; (void)p4;
}

// HD SViewport vtbl +0x7c -> 0x68b200 (1 arg dword)
void SViewport::Slot_7C()
{
    STUB_LOG("SViewport::Slot_7C (0x68b200)");
    PZ_TRACE("SViewport::Slot_7C (0x68b200)");
}

// HD SViewport vtbl +0x80 -> 0x68b210 (1 arg dword)
void SViewport::Slot_80()
{
    STUB_LOG("SViewport::Slot_80 (0x68b210)");
    PZ_TRACE("SViewport::Slot_80 (0x68b210)");
}

} // namespace pz
