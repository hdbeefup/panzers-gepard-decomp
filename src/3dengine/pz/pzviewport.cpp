// src/3dengine/pz/pzviewport.cpp
// pz::SViewport: camera, projection, screen matrix and the 2D/3D frame
// order. OWNER: agent A.
//
// Frame order (HD SViewport::Render 0x68c220, per subport):
//   TestCooperativeLevel (lost -> skip; not reset -> SGepard::ResetDevice
//     0x67fde0, board +0xc4)
//   if (scene && viewport+0x241) { scene 0x6bbc40(vp); scene 0x6a24c0(vp)
//     (SViewport::Clear 0x689f10 with the scene fog colour); BeginScene;
//     scene 0x6acaf0(vp) }
//   else { Clear(TARGET|ZBUFFER[|STENCIL], clearColor, 1.0, 0); BeginScene }
//   board: viewport+0x240 ? SBoard render 0x6c7150 : software cursor 0x6ca240
//   EndScene; Present 0x68bfe0
// So the 3D scene and the 2D board share one BeginScene/EndScene, scene
// first. Callers: SDXWindow::OnIdle 0x53a0d0 (scene = SDXWindow+0xe0) every
// frame, SWorld::LoadMap 0x5f1990 (scene 0: loading frame).
//
// Recompile: SWINE SGepard::RenderScene does the device-lost check, its own
// Clear, BeginScene, board, EndScene and Present. Render() runs
// PrepareViewport before it; the scene pass hook (inside BeginScene) runs
// UpdateViewport (the HD clear, after SWINE's) and RenderViewport. A
// D3DSBT_ALL state block restores the device state for the SWINE board.

#include <d3d9.h>
#include <math.h>
#include <string.h>
#include "pzviewport.h"
#include "pzscene.h"
#include "pzgepard.h"
#include "pzmodel.h"
#include "mesh.h"
#include "igepard.h"
#include "core_common.h"
#include "logger.h"
#include "stub_log.h"
#include "mods.h"

extern SIGepard* Gepard;   // SWINE renderer (window/widget.h)
extern SIBoard* Board;

#if PANZERS_MOD_WIDESCREEN
bool g_ModWidescreen = true;   // see mods.h
#endif

namespace pz {

#if PANZERS_MOD_WIDESCREEN
// MOD_WIDESCREEN (Hor+): on a window wider than 4:3, keep the vertical field
// of view of a 4:3 view with the same fov (m11 = 4/3 / tan(fov/2)) and widen
// the horizontal one (m00 = m11 * h / w). Narrower windows keep HD's fixed
// horizontal fov. Called after HD's projection maths, so the faithful values
// are only replaced, never mixed.
static void ModWidescreenProjection(SViewport* vp)
{
    if (!g_ModWidescreen || vp->Camera.Fov == 0.0f || vp->Width <= 0 || vp->Height <= 0)
        return;
    float base = (float)(1.0 / tan((double)vp->Camera.Fov * 0.5));
    if (vp->Width * 3 > vp->Height * 4) {
        vp->Proj[5] = base * (4.0f / 3.0f);
        vp->Proj[0] = vp->Proj[5] * (float)vp->Height / (float)vp->Width;
    } else {
        // HD's values (Proj[0] may still hold a widened m00 from before).
        vp->Proj[0] = base;
        vp->Proj[5] = (base / (float)vp->Height) * (float)vp->Width;
    }
}
#endif

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
    s_FrameScene->UpdateViewport(s_FrameViewport);   // 0x6a24c0
    s_FrameScene->RenderViewport(s_FrameViewport);   // 0x6acaf0
    if (saved) {
        saved->Apply();
        saved->Release();
    }
}

// 4x4 helpers (HD 0x7c5d20 3x4*4x4, 0x7c5fb0 4x4*4x4).
static void Mul44(float* o, const float* a, const float* b)
{
    float r[16];
    for (int i = 0; i < 4; ++i)
        for (int j = 0; j < 4; ++j)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
    memcpy(o, r, 64);
}

SViewport::SViewport()
    : Camera(), ProjectionSet(false)
{
    memset(CamOffset, 0, sizeof(CamOffset));
    memset(View, 0, sizeof(View));
    View[0] = View[4] = View[8] = 1.0f;
    memset(Proj, 0, sizeof(Proj));
    Proj[0] = Proj[5] = Proj[10] = Proj[15] = 1.0f;
    ZBias = 0.0f;
    ZBiasOff = false;
    Left = Top = 0;
    Width = 1024;
    Height = 768;
    memset(ScreenM, 0, sizeof(ScreenM));
    memset(ViewProjScreen, 0, sizeof(ViewProjScreen));
    SizeScale = 1.0f;
}

SViewport::~SViewport()
{
}

static void ReadDeviceViewport(SViewport* vp)
{
    IDirect3DDevice9* dev = HD().Device;
    D3DVIEWPORT9 v;
    if (dev && SUCCEEDED(dev->GetViewport(&v)) && v.Width && v.Height) {
        vp->Left = (int)v.X;
        vp->Top = (int)v.Y;
        vp->Width = (int)v.Width;
        vp->Height = (int)v.Height;
    }
}

// PANZERS 0x68d370
// Eye position (plus the one-shot offset +0xc0), yaw about Y, pitch about X:
// View = Translate(-eye) * RotY(-yaw) * RotX(-pitch).
void SViewport::SetCamera(float x, float y, float z, float yaw, float pitch)
{
    PZ_TRACE("SViewport::SetCamera (0x68d370)");
    z = z + CamOffset[2];
    x = x + CamOffset[0];
    y = y + CamOffset[1];
    Camera.Yaw = yaw;
    Camera.Z = z;
    CamOffset[0] = CamOffset[1] = CamOffset[2] = 0.0f;
    Camera.Pitch = pitch;
    Camera.X = x;
    Camera.Y = y;
    double p = (double)-pitch;
    float cp = (float)cos(p);
    float sp = (float)sin(p);
    float P[12] = { 1, 0, 0, 0, cp, sp, 0, -sp, cp, 0, 0, 0 };
    double w = (double)-yaw;
    float cy = (float)cos(w);
    float sy = (float)sin(w);
    float Y[12] = { cy, 0, -sy, 0, 1, 0, sy, 0, cy, 0, 0, 0 };
    float T[12] = { 1, 0, 0, 0, 1, 0, 0, 0, 1, -x, -y, -z };
    float TY[12];
    Mat34Mul(TY, T, Y);
    Mat34Mul(View, TY, P);
    ApplyTransforms();
    UpdateScreenMatrix();   // 0x68c070
}

// PANZERS 0x68be30
void SViewport::GetCamera(float* x, float* y, float* z, float* yaw, float* pitch)
{
    PZ_TRACE("SViewport::GetCamera (0x68be30)");
    if (x) *x = Camera.X;
    if (y) *y = Camera.Y;
    if (z) *z = Camera.Z;
    if (yaw) *yaw = Camera.Yaw;
    if (pitch) *pitch = Camera.Pitch;
}

// PANZERS 0x68cfd0
// Horizontal fov: m00 = 1/tan(fov/2), m11 = m00 * w/h (4:3 in full-screen
// mode 1), m22 = f/(f-n), m23 = 1, m32 = -n f/(f-n).
void SViewport::SetProjection(float fovRadians, float nearZ, float farZ)
{
    PZ_TRACE("SViewport::SetProjection (0x68cfd0)");
    Camera.Fov = fovRadians;
    Camera.FarZ = farZ;
    Camera.NearZ = nearZ;
    float q = farZ / (farZ - nearZ);
    memset(Proj, 0, sizeof(Proj));
    float m00 = (float)(1.0 / tan((double)fovRadians * 0.5));
    Proj[0] = m00;
    ReadDeviceViewport(this);
    float m11 = (m00 / (float)Height) * (float)Width;   // windowed: back buffer aspect
    Proj[10] = q;
    Proj[11] = 1.0f;
    Proj[5] = m11;
    Proj[14] = -(q * nearZ);
#if PANZERS_MOD_WIDESCREEN
    ModWidescreenProjection(this);
#endif
    ProjectionSet = true;
    ApplyTransforms();   // 0x68d160
    UpdateScreenMatrix();
}

// PANZERS 0x68d160 (+ the VIEW part of 0x68d370)
void SViewport::ApplyTransforms()
{
    IDirect3DDevice9* dev = HD().Device;
    if (!dev)
        return;
    float v[16];
    Mat34To44(v, View);
    dev->SetTransform(D3DTS_VIEW, (const D3DMATRIX*)v);
    float p[16];
    memcpy(p, Proj, 64);
    if (!ZBiasOff) {
        p[10] = Proj[10] - ZBias;
    } else {
        p[10] = 1.0f;
        p[14] = 0.0f;
    }
    dev->SetTransform(D3DTS_PROJECTION, (const D3DMATRIX*)p);
}

// PANZERS 0x68c070
void SViewport::UpdateScreenMatrix()
{
    ReadDeviceViewport(this);
    memset(ScreenM, 0, sizeof(ScreenM));
    ScreenM[0] = ScreenM[5] = ScreenM[10] = ScreenM[15] = 1.0f;
    double hw = (double)Width * 0.5;
    ScreenM[0] = (float)hw;
    ScreenM[12] = (float)((double)Left + hw);
    double hh = (double)Height * 0.5;
    ScreenM[5] = (float)((double)Height * -0.5);
    ScreenM[13] = (float)((double)Top + hh);
    float v[16], vp[16];
    Mat34To44(v, View);
    Mul44(vp, v, Proj);
    Mul44(ViewProjScreen, vp, ScreenM);
    SizeScale = Proj[0] * ScreenM[0];
}

// PANZERS 0x68dda0
void SViewport::ProjectToScreen(const float* pos, float size, float* x, float* y, float* screenSize, float* z, int* fogAlpha)
{
    const float* m = ViewProjScreen;
    float px = pos[0], py = pos[1], pz = pos[2];
    float w = m[7] * py + m[3] * px + m[11] * pz + m[15];
    if (w <= 0.0f) {
        *x = 0.0f;
        *y = 0.0f;
        *screenSize = -1.0f;
        return;
    }
    w = 1.0f / w;
    *x = (m[4] * py + px * m[0] + m[8] * pz + m[12]) * w;
    *y = (m[5] * py + m[1] * px + m[9] * pz + m[13]) * w;
    *z = (m[6] * py + m[2] * px + m[10] * pz + m[14]) * w;
    *screenSize = SizeScale * size * w;
    // Fog visibility from the view-space distance (render-pass fog cache).
    float fs, fe, fs2, fe2, inv;
    if (GetPassFog(&fs, &fe, &fs2, &fe2, &inv) && fs < fe) {
        const float* V = View;
        float vx = V[3] * py + V[0] * px + V[6] * pz + V[9];
        float vy = V[1] * px + V[4] * py + V[7] * pz + V[10];
        float vz = V[2] * px + V[5] * py + V[8] * pz + V[11];
        double d2 = (double)(vy * vy + vx * vx + vz * vz);
        if ((double)fs2 <= d2) {
            if ((double)fe2 <= d2) {
                *fogAlpha = 0;
                return;
            }
            double d = sqrt(d2);
            *fogAlpha = (int)(((double)fe - d) * (double)inv) << 24;   // FISTP, RC = truncate
            return;
        }
    }
    *fogAlpha = (int)0xff000000;
}

// PANZERS 0x689f10
// HD viewport+0x6c depth format: z buffer when present, stencil for
// D24S8 / D24X4S4 / D15S1 (0x4b / 0x4f / 0x49).
void SViewport::Clear(unsigned color, float z, unsigned stencil)
{
    IDirect3DDevice9* dev = HD().Device;
    if (!dev)
        return;
    DWORD flags = D3DCLEAR_TARGET;
    IDirect3DSurface9* ds = nullptr;
    if (SUCCEEDED(dev->GetDepthStencilSurface(&ds)) && ds) {
        D3DSURFACE_DESC d;
        if (SUCCEEDED(ds->GetDesc(&d))) {
            flags |= D3DCLEAR_ZBUFFER;
            if (d.Format == D3DFMT_D24S8 || d.Format == D3DFMT_D24X4S4 || d.Format == D3DFMT_D15S1)
                flags |= D3DCLEAR_STENCIL;
        }
        ds->Release();
    }
    HRESULT hr = dev->Clear(0, nullptr, flags, color, z, stencil);
    if (FAILED(hr))
        Logger.g->Log(0, "SViewport::Clear: Clear failed (%08x)", (unsigned)hr);
}

SIBoard* SViewport::GetBoard()
{
    PZ_TRACE("SViewport::GetBoard (0x68b730)");
    // HD 0x68b730 AddRefs SViewport+0x28; the SWINE board has no refcount.
    return ::Board;
}

// PANZERS 0x68c220 (recompile frame, see the file comment)
void SViewport::Render(SIScene* scene, unsigned clearColor)
{
    PZ_TRACE("SViewport::Render (0x68c220)");
    // clearColor: HD clears with it only when there is no scene. SWINE clears
    // with its fog colour (black in the menu) every frame.
    (void)clearColor;
    HD().PolyCount = 0;   // SGepard +0x4b4/+0x4b8 frame stats
    HD().VertexCount = 0;
    SScene* s = static_cast<SScene*>(scene);
    if (s)
        s->PrepareViewport(this);   // 0x6bbc40
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

// PANZERS 0x68c4e0
// Primary windowed viewport (mode +0x78 == 0), called by SDXWindow::OnSize
// 0x53a1b0 with the new client size. HD scales its subports (none in the
// menu), stores the size at +0x128/+0x12c, zeroes the back-buffer size in the
// present parameters (+0x30/+0x34: D3D takes the client size) and resets the
// device (SGepard::ResetDevice 0x67fde0), then resizes board frame 0 (board
// +0x14) and re-applies the hardware cursor (board +0xc4). In the recompile
// the SWINE SDXWindow::OnSize has already done the device reset and the
// board (Gepard->Resize). The rest is HD's: with a projection set (fov +0xcc
// != 0.0) the y scale follows the new aspect, m11 = m00 / h * w, so the
// horizontal field of view stays and the vertical one narrows on a wide
// window; then SetViewport (0x68d620), the transforms (0x68d160) and the
// screen matrix (0x68c070).
void SViewport::Resize(int width, int height)
{
    PZ_TRACE("SViewport::Resize (0x68c4e0)");
    Width = width;
    Height = height;
    if (Camera.Fov != 0.0f)
        Proj[5] = (Proj[0] / (float)Height) * (float)Width;
#if PANZERS_MOD_WIDESCREEN
    ModWidescreenProjection(this);
#endif
    ApplyTransforms();
    UpdateScreenMatrix();
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
