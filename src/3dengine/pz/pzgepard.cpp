// src/3dengine/pz/pzgepard.cpp
// The Gepard facade: HD SGepard slots (vftable 0x816da8) over the SWINE
// SGepard, plus the HD SGepard members the HD 3D path calls. OWNER: agent A.
//
// Shared with the SWINE 2D path:
//   - the D3D9 device (SGepard::lpD3DDev): the HD scene draws inside the
//     SWINE frame through SGepard::PanzersScenePass (pzviewport.cpp);
//   - textures: LoadTexture/ReleaseTexture forward to the SWINE texture table,
//     so a handle from the 3D path is valid for the board and the reverse.
// Owned here: the primary viewport facade, the SPixie (HD SGepard+0x7f4),
// the model prototype heap (+0x550), the animation heap (+0x564) and the
// dynamic vertex buffer (+0x5d4).
//
// Device creation is unchanged (SWINE SGepard::Initialize). HD creates the
// device with BehaviorFlags 0x40 (HARDWARE_VERTEXPROCESSING) or 0x20
// (SOFTWARE), never D3DCREATE_FPU_PRESERVE (0x67d1c0 writes SGepard+0x458,
// 0x68ada0 passes it to CreateDevice); SWINE does the same. See
// docs/MENU3D_INTERFACES.md "FPU".

#include <d3d9.h>
#include <string.h>
#include <stdlib.h>
#include "pzgepard.h"
#include "pzviewport.h"
#include "pzscene.h"
#include "pzpixie.h"
#include "pzterrain.h"
#include "panim.h"
#include "pzmodel.h"
#include <sys/stat.h>
#include "mesh.h"
#include "gepard.h"
#include "stream.h"
#include "core_common.h"
#include "logger.h"
#include "stub_log.h"

extern SIGepard* Gepard;   // SWINE renderer (window/widget.h)

namespace pz {

void ViewportScenePass(IDirect3DDevice9* dev);   // pzviewport.cpp
void ViewportPostBoardPass(IDirect3DDevice9* dev);   // pzviewport.cpp
void ReleaseMeshVertexDecls();                   // mesh.cpp
void SetExtension(SString* s, const char* ext);  // pmodel.cpp

static SPzGepard* s_Facade = nullptr;
static SGepardHDState s_HD;

static SGepard* SwineGepard() { return static_cast<SGepard*>(::Gepard); }

// ---------------------------------------------------------------------------
// HD shaders (SGepard::CreatePixelShader 0x678e60, CreateVertexShader
// 0x679eb0). The token streams are the ones PANZERS.exe creates.
// ---------------------------------------------------------------------------

struct SHdShader {
    int          Id;
    const DWORD* Tokens;
};
#include "hdshaders.inc"

// PANZERS 0x678e60
static void CreatePixelShader(int id, const DWORD* tokens)
{
    SGepardHDState& hd = s_HD;
    if (id < 1 || id > 0x22 || hd.PixelShaders[id]) {
        Logger.g->Panic("SGepard::CreatePixelShader: Invalid id: %d", id);
        return;
    }
    HRESULT hr = hd.Device->CreatePixelShader(tokens, &hd.PixelShaders[id]);
    if (FAILED(hr)) {
        Logger.g->Panic("%s: %08x", "SGepard::CreatePixelShader: CreatePixelShader failed", (unsigned)hr);
        hd.PixelShaders[id] = nullptr;
    }
}

static void CreatePixelShaders(const SHdShader* set, int count)
{
    for (int i = 0; i < count; ++i)
        CreatePixelShader(set[i].Id, set[i].Tokens);
}

// PANZERS 0x679eb0
static void CreateVertexShader(int id, const DWORD* tokens)
{
    SGepardHDState& hd = s_HD;
    if (id < 1 || id > 0x42 || hd.VertexShaders[id]) {
        Logger.g->Panic("SGepard::CreateVertexShader: Invalid id: %d", id);
        return;
    }
    HRESULT hr = hd.Device->CreateVertexShader(tokens, &hd.VertexShaders[id]);
    if (FAILED(hr)) {
        Logger.g->Panic("%s: %08x", "SGepard::CreateVertexShader: CreateVertexShader failed", (unsigned)hr);
        hd.VertexShaders[id] = nullptr;
    }
}

// The two vs_2_0 shaders SGepard::Initialize builds inline when the vertex
// shader version is at least 2.0 (0x67d1c0, again after a reset in
// 0x67fde0). The shadow-buffer pass of technique 4 draws model meshes with
// them (SMesh::DrawShadow 0x6cdb70): c0..c3 world*view*projection,
// c12..c15 world*view, c8..c11 the texture 0 matrix (all transposed).
static const DWORD kVsShadow41[] = {
    0xfffe0200,                                  // vs_2_0
    0x0200001f, 0x80000000, 0x900f0000,          // dcl_position v0
    0x03000014, 0xc00f0000, 0x90e40000, 0xa0e40000,  // m4x4 oPos, v0, c0
    0x03000014, 0x800f0000, 0x90e40000, 0xa0e4000c,  // m4x4 r0, v0, c12
    0x03000014, 0xe00f0000, 0x80e40000, 0xa0e40008,  // m4x4 oT0, r0, c8
    0x02000001, 0xd00f0000, 0xa0000005,          // mov oD0, c5.x
    0x0000ffff,
};
static const DWORD kVsShadow42[] = {
    0xfffe0200,                                  // vs_2_0
    0x0200001f, 0x80000000, 0x900f0000,          // dcl_position v0
    0x0200001f, 0x80000005, 0x900f0001,          // dcl_texcoord v1
    0x03000014, 0xc00f0000, 0x90e40000, 0xa0e40000,  // m4x4 oPos, v0, c0
    0x03000014, 0x800f0000, 0x90e40000, 0xa0e4000c,  // m4x4 r0, v0, c12
    0x03000014, 0xe00f0000, 0x80e40000, 0xa0e40008,  // m4x4 oT0, r0, c8
    0x02000001, 0xd00f0000, 0xa0000005,          // mov oD0, c5.x
    0x02000001, 0xe00f0001, 0x90e40001,          // mov oT1, v1
    0x0000ffff,
};

// PANZERS 0x67d1c0 (the shader and shadow-technique part, after the device
// is created): PS 1.x reflection shaders on PS > 1.0, the two inline
// vertex shaders on VS >= 2.0, and the shadow technique by PS version.
static void InitShadowCaps(IDirect3DDevice9* dev)
{
    SGepardHDState& hd = s_HD;
    D3DCAPS9 caps;
    memset(&caps, 0, sizeof(caps));
    dev->GetDeviceCaps(&caps);
    hd.VSVersion = caps.VertexShaderVersion & 0xffff;
    hd.PSVersion = caps.PixelShaderVersion & 0xffff;
    hd.DepthTextureFlag = true;
    hd.DebugShadowTexture = -1;
    IDirect3D9* d3d = nullptr;
    D3DDEVICE_CREATION_PARAMETERS cp;
    memset(&cp, 0, sizeof(cp));
    dev->GetCreationParameters(&cp);
    D3DDISPLAYMODE mode;
    memset(&mode, 0, sizeof(mode));
    dev->GetDisplayMode(0, &mode);
    if (SUCCEEDED(dev->GetDirect3D(&d3d)) && d3d) {
        D3DADAPTER_IDENTIFIER9 id;
        if (SUCCEEDED(d3d->GetAdapterIdentifier(cp.AdapterOrdinal, 0, &id))) {
            hd.VendorId = id.VendorId;
            hd.DeviceId = id.DeviceId;
        }
    }
    if (0x1ff < hd.VSVersion) {
        CreateVertexShader(0x41, kVsShadow41);
        CreateVertexShader(0x42, kVsShadow42);
    }
    if (0x100 < hd.PSVersion)
        CreatePixelShaders(kPs1x, sizeof(kPs1x) / sizeof(kPs1x[0]));   // 0x67df80
    Logger.g->Log(0, "SGepard::Initialize: Detected pixel shader version: %d", hd.PSVersion);
    if (hd.PSVersion < 0x200) {
        HRESULT hr = d3d ? d3d->CheckDeviceFormat(cp.AdapterOrdinal, cp.DeviceType, mode.Format,
                                                  D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, D3DFMT_D24X8)
                         : E_FAIL;
        if (hr == D3D_OK) {
            hd.ShadowCap = 2;
            hd.DepthTextureFlag = false;
            Logger.g->Log(0, "SGepard::Initialize: Choosing depth texture shadow buffering.");
            CreatePixelShaders(kPsDepth, sizeof(kPsDepth) / sizeof(kPsDepth[0]));   // 0x67dff0
        } else if (hd.PSVersion < 0x104) {
            hd.ShadowCap = 1;
            Logger.g->Log(0, "SGepard::Initialize: Choosing compatible shadow buffering.");
        } else {
            hd.ShadowCap = 3;
            Logger.g->Log(0, "SGepard::Initialize: Choosing PS1.4 based shadow buffering.");
            CreatePixelShaders(kPs14, sizeof(kPs14) / sizeof(kPs14[0]));   // 0x67e1c0
        }
    } else {
        hd.ShadowCap = 4;
        Logger.g->Log(0, "SGepard::Initialize: Choosing PS2.0 based shadow buffering.");
        CreatePixelShaders(kPs20, sizeof(kPs20) / sizeof(kPs20[0]));   // 0x67e3b0
    }
    hd.PixelShader1x = hd.PixelShaders[2] != nullptr;
    if (d3d)
        d3d->Release();
}

static void ReleaseShaders()
{
    for (auto*& p : s_HD.PixelShaders)
        if (p) {
            p->Release();
            p = nullptr;
        }
    for (auto*& v : s_HD.VertexShaders)
        if (v) {
            v->Release();
            v = nullptr;
        }
}

// ---------------------------------------------------------------------------
// Offscreen viewports (render targets)
// ---------------------------------------------------------------------------

struct SHdRenderTarget {
    bool               Used;
    int                Width, Height;     // vp +0x128 / +0x12c
    D3DFORMAT          Format;            // vp +0x68
    D3DFORMAT          DepthFormat;       // vp +0x6c (0x689d50)
    unsigned           Flags;             // vp +0x74: 1 depth, 2 colour texture, 4 depth texture, 8 stencil
    IDirect3DTexture9* ColorTexture;      // vp +0x18
    IDirect3DTexture9* DepthTexture;      // vp +0x1c
    IDirect3DSurface9* Color;             // vp +0x10
    IDirect3DSurface9* Depth;             // vp +0x14
    unsigned           Filter[2];         // texture records +0x24 of +0x20 / +0x24
};
static const int kMaxRenderTargets = 8;
static SHdRenderTarget s_Rt[kMaxRenderTargets];
static const int kRtTextureBase = 0x40000000;   // texture handle base + index * 2 (+1: depth)
static int s_RtSelected = -1;
static IDirect3DSurface9* s_SavedColor = nullptr;
static IDirect3DSurface9* s_SavedDepth = nullptr;
static D3DVIEWPORT9 s_SavedViewport;

static void ReleaseRenderTarget(SHdRenderTarget& rt)
{
    if (rt.Color) rt.Color->Release();
    if (rt.Depth) rt.Depth->Release();
    if (rt.ColorTexture) rt.ColorTexture->Release();
    if (rt.DepthTexture) rt.DepthTexture->Release();
    memset(&rt, 0, sizeof(rt));
}

// PANZERS 0x689d50 (SViewport::ChooseDepthStencilFormat)
static D3DFORMAT ChooseDepthFormat(IDirect3DDevice9* dev, D3DFORMAT color, unsigned flags)
{
    if (!(flags & 1))
        return D3DFMT_UNKNOWN;
    IDirect3D9* d3d = nullptr;
    if (FAILED(dev->GetDirect3D(&d3d)) || !d3d)
        return D3DFMT_UNKNOWN;
    D3DDEVICE_CREATION_PARAMETERS cp;
    dev->GetCreationParameters(&cp);
    D3DDISPLAYMODE mode;
    dev->GetDisplayMode(0, &mode);
    DWORD usage = D3DUSAGE_DEPTHSTENCIL;
    D3DRESOURCETYPE type = (flags & 4) ? D3DRTYPE_TEXTURE : D3DRTYPE_SURFACE;
    auto ok = [&](D3DFORMAT f) {   // 0x68c990
        return SUCCEEDED(d3d->CheckDeviceFormat(cp.AdapterOrdinal, cp.DeviceType, mode.Format, usage, type, f)) &&
               SUCCEEDED(d3d->CheckDepthStencilMatch(cp.AdapterOrdinal, cp.DeviceType, mode.Format, color, f));
    };
    static const D3DFORMAT kStencil[] = { D3DFMT_D24S8, D3DFMT_D24X4S4, D3DFMT_D15S1 };
    static const D3DFORMAT kNv[] = { D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24X4S4, D3DFMT_D16 };
    static const D3DFORMAT kOther[] = { D3DFMT_D16, D3DFMT_D32, D3DFMT_D24X8, D3DFMT_D24X4S4 };
    const D3DFORMAT* list;
    int n;
    if (flags & 8) {
        list = kStencil;
        n = 3;
    } else if ((color == D3DFMT_X8R8G8B8 || color == D3DFMT_A8R8G8B8) && s_HD.VendorId == 0x10de) {
        list = kNv;
        n = 4;
    } else {
        list = kOther;
        n = 4;
    }
    D3DFORMAT r = D3DFMT_UNKNOWN;
    for (int i = 0; i < n && r == D3DFMT_UNKNOWN; ++i)
        if (ok(list[i]))
            r = list[i];
    d3d->Release();
    if (r == D3DFMT_UNKNOWN && (flags & 8))
        Logger.g->Panic("SViewport::ChooseDepthStencilFormat: Stecil-buffer is not available");
    return r;
}

// PANZERS 0x689710 (SViewport::AllocOffscreenBuffers)
static bool AllocRenderTarget(SHdRenderTarget& rt)
{
    IDirect3DDevice9* dev = s_HD.Device;
    HRESULT hr;
    if (rt.Flags & 2) {
        hr = dev->CreateTexture(rt.Width, rt.Height, 1, D3DUSAGE_RENDERTARGET, rt.Format, D3DPOOL_DEFAULT,
                                &rt.ColorTexture, nullptr);
        if (FAILED(hr)) {
            Logger.g->Log(0, "%s: %08x", "SViewport::AllocOffscreenBuffers: CreateRenderTargetTexture failed", (unsigned)hr);
            return false;
        }
        rt.ColorTexture->GetSurfaceLevel(0, &rt.Color);
    } else {
        hr = dev->CreateRenderTarget(rt.Width, rt.Height, rt.Format, D3DMULTISAMPLE_NONE, 0, TRUE, &rt.Color, nullptr);
        if (FAILED(hr)) {
            Logger.g->Log(0, "%s: %08x", "SViewport::CreateSurface: CreateRenderTarget failed", (unsigned)hr);
            return false;
        }
    }
    if (!(rt.Flags & 1) || rt.DepthFormat == D3DFMT_UNKNOWN)
        return true;
    if (rt.Flags & 4) {
        hr = dev->CreateTexture(rt.Width, rt.Height, 1, D3DUSAGE_DEPTHSTENCIL, rt.DepthFormat, D3DPOOL_DEFAULT,
                                &rt.DepthTexture, nullptr);
        if (FAILED(hr)) {
            Logger.g->Log(0, "%s: %08x", "SViewport::AllocOffscreenBuffers: CreateDepthStencilTexture failed", (unsigned)hr);
            return false;
        }
        rt.DepthTexture->GetSurfaceLevel(0, &rt.Depth);
    } else {
        hr = dev->CreateDepthStencilSurface(rt.Width, rt.Height, rt.DepthFormat, D3DMULTISAMPLE_NONE, 0, TRUE,
                                            &rt.Depth, nullptr);
        if (FAILED(hr)) {
            Logger.g->Log(0, "%s: %08x", "SViewport::AllocOffscreenBuffers: CreateDepthStencilSurface failed", (unsigned)hr);
            return false;
        }
    }
    return true;
}

// PANZERS 0x678c70
// new SViewport(0x244) in the viewport heap, then 0x68aa60: format, flags,
// depth format, size (texture sizes clamped to the device), buffers.
int GepardCreateRenderTarget(int width, int height, unsigned format, unsigned flags)
{
    IDirect3DDevice9* dev = HD().Device;
    if (!dev)
        return -1;
    int index = -1;
    for (int i = 0; i < kMaxRenderTargets && index < 0; ++i)
        if (!s_Rt[i].Used)
            index = i;
    if (index < 0) {
        Logger.g->Log(0, "SGepard::CreateViewport: no free offscreen viewport");
        return -1;
    }
    SHdRenderTarget& rt = s_Rt[index];
    memset(&rt, 0, sizeof(rt));
    rt.Used = true;
    rt.Format = (D3DFORMAT)format;
    rt.Flags = flags;
    rt.DepthFormat = ChooseDepthFormat(dev, rt.Format, flags);   // 0x689d50
    rt.Width = width;
    rt.Height = height;
    if (flags & 6) {   // 0x680230: a texture size the device takes
        D3DCAPS9 caps;
        dev->GetDeviceCaps(&caps);
        if ((DWORD)rt.Width > caps.MaxTextureWidth) rt.Width = (int)caps.MaxTextureWidth;
        if ((DWORD)rt.Height > caps.MaxTextureHeight) rt.Height = (int)caps.MaxTextureHeight;
    }
    rt.Filter[0] = rt.Filter[1] = (GepardOption(9) != 0) + 1 | (GepardOption(8) != 0 ? 4u : 0u) | 8u;
    if (!AllocRenderTarget(rt)) {
        ReleaseRenderTarget(rt);
        return -1;
    }
    return index;
}

// PANZERS 0x67a3f0
void GepardDestroyRenderTarget(int index)
{
    if (index < 0 || index >= kMaxRenderTargets || !s_Rt[index].Used)
        return;
    if (s_RtSelected == index)
        GepardUnselectRenderTarget();
    ReleaseRenderTarget(s_Rt[index]);
}

// PANZERS 0x67cb60 (+0x20 colour texture, +0x24 depth texture)
int GepardRenderTargetTexture(int index, bool depth)
{
    if (index < 0 || index >= kMaxRenderTargets || !s_Rt[index].Used)
        return -1;
    return kRtTextureBase + index * 2 + (depth ? 1 : 0);
}

static SHdRenderTarget* RtFromTexture(int texture, bool* depth)
{
    if (texture < kRtTextureBase)
        return nullptr;
    int i = (texture - kRtTextureBase) >> 1;
    if (i < 0 || i >= kMaxRenderTargets || !s_Rt[i].Used)
        return nullptr;
    *depth = (texture & 1) != 0;
    return &s_Rt[i];
}

// PANZERS 0x680bf0 (flags 8: from options 8/9)
void GepardSetTextureFilter(int texture, unsigned flags)
{
    bool depth;
    SHdRenderTarget* rt = RtFromTexture(texture, &depth);
    if (!rt)
        return;   // SWINE textures take the option filter in GepardSetTexture
    if (flags == 8)
        flags = (GepardOption(9) != 0) + 1 | (GepardOption(8) != 0 ? 4u : 0u) | 8u;
    rt->Filter[depth ? 1 : 0] = flags;
}

// PANZERS 0x6803e0
// Pushes the current target and selects the offscreen one: render target,
// depth surface and the D3D viewport (0x68c770 / 0x68d620).
void GepardSelectRenderTarget(int index)
{
    IDirect3DDevice9* dev = HD().Device;
    if (!dev || index < 0 || index >= kMaxRenderTargets || !s_Rt[index].Used || s_RtSelected >= 0)
        return;
    SHdRenderTarget& rt = s_Rt[index];
    dev->GetRenderTarget(0, &s_SavedColor);
    dev->GetDepthStencilSurface(&s_SavedDepth);
    dev->GetViewport(&s_SavedViewport);
    if (FAILED(dev->SetRenderTarget(0, rt.Color)))
        Logger.g->Log(0, "SViewport::Select: SetRenderTarget failed");
    if (FAILED(dev->SetDepthStencilSurface(rt.Depth)))
        Logger.g->Log(0, "SViewport::Select: SetDepthStencilSurface failed");
    D3DVIEWPORT9 v = { 0, 0, (DWORD)rt.Width, (DWORD)rt.Height, 0.0f, 1.0f };
    dev->SetViewport(&v);
    s_RtSelected = index;
}

// PANZERS 0x6814b0
void GepardUnselectRenderTarget()
{
    IDirect3DDevice9* dev = HD().Device;
    if (s_RtSelected < 0) {
        Logger.g->Log(0, "SGepard::UnselectViewport: called without SelectViewport");
        return;
    }
    s_RtSelected = -1;
    if (!dev)
        return;
    dev->SetRenderTarget(0, s_SavedColor);
    dev->SetDepthStencilSurface(s_SavedDepth);
    dev->SetViewport(&s_SavedViewport);
    if (s_SavedColor) s_SavedColor->Release();
    if (s_SavedDepth) s_SavedDepth->Release();
    s_SavedColor = nullptr;
    s_SavedDepth = nullptr;
}

// PANZERS 0x689f10 (on the selected offscreen viewport)
void GepardClearRenderTarget(unsigned color, float z, unsigned stencil)
{
    IDirect3DDevice9* dev = HD().Device;
    if (!dev || s_RtSelected < 0)
        return;
    const SHdRenderTarget& rt = s_Rt[s_RtSelected];
    DWORD flags = D3DCLEAR_TARGET;
    if (rt.Depth) {
        flags |= D3DCLEAR_ZBUFFER;
        if (rt.DepthFormat == D3DFMT_D24S8 || rt.DepthFormat == D3DFMT_D24X4S4 || rt.DepthFormat == D3DFMT_D15S1)
            flags |= D3DCLEAR_STENCIL;
    }
    dev->Clear(0, nullptr, flags, color, z, stencil);
}

// Device reset: HD SGepard::ResetDevice 0x67fde0 frees every scene's shadow
// buffer (0x6a2670) before Reset; the scenes recreate it on their next
// shadow pass. The recompile hooks the SWINE reset the same way.
static void PreDeviceReset(void*)
{
    ReleaseAllShadowBuffers();
    if (s_RtSelected >= 0)
        GepardUnselectRenderTarget();
    for (auto& rt : s_Rt)
        if (rt.Used)
            ReleaseRenderTarget(rt);
    Logger.g->Log(0, "pz: offscreen viewports released for the device reset");
}

static void PostDeviceReset(void*)
{
    InvalidateRenderPassCache();
}

SGepardHDState& HD()
{
    if (!s_HD.Device && ::Gepard) {
        IDirect3DDevice9* dev = SwineGepard()->lpD3DDev;
        if (dev) {
            s_HD.Device = dev;
            D3DCAPS9 caps;
            memset(&caps, 0, sizeof(caps));
            dev->GetDeviceCaps(&caps);
            unsigned blend = caps.MaxTextureBlendStages ? caps.MaxTextureBlendStages : 1;
            unsigned tex = caps.MaxSimultaneousTextures ? caps.MaxSimultaneousTextures : 1;
            s_HD.TransformStages = blend < 5 ? blend : 5;
            s_HD.BlendStages = blend < 5 ? blend : 5;
            s_HD.SamplerStages = tex < 5 ? tex : 5;
            // HD: PS id 2 exists on PS 1.1+ cards (0x67df80); then
            // SMaterial::Begin draws reflections in one PS 1.x pass.
            InitShadowCaps(dev);
            s_HD.DynVBSize = 0x1fffe0;   // 0x678730
            s_HD.DynType112 = 0;
            s_HD.DynTypeFvf[0] = 0x112;
            s_HD.DynTypeStride[0] = 0x20;
            s_HD.DynTypeCount = 1;
            // SYSTEMMEM: survives SWINE's device reset (HD uses DEFAULT).
            if (FAILED(dev->CreateVertexBuffer(s_HD.DynVBSize, D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY, 0,
                                               D3DPOOL_SYSTEMMEM, &s_HD.DynVB, nullptr)))
                Logger.g->Log(0, "SGepard::CreateDynamicVB: CreateVertexBuffer failed");
        }
    }
    return s_HD;
}

// PANZERS 0x6f33c0 (flora instance part, 0x6f3880..0x6f3a40)
// Grass instances (agent B's flora pass). HD draws only node 0's mesh
// (prototype -> Nodes[0] +0x44, mesh vtbl +4) with
//   node 0 transform (+0x14, 3x4) [x Y<->Z when FlyZ] x scale 0.005
//   (the constant 3x4 at 0x886ed0) x the instance rotation and position.
// Without the node transform and the scale the grass was drawn 200 times
// too large and covered the whole menu scene.
static void DrawFloraInstance(SScene* scene, int proto, const float world[16])
{
    SPModel* p = GepardModelPrototype(proto);
    if (!p || !HD().Device || p->NodeCount < 1 || !p->Nodes[0].Mesh)
        return;
    float m[12];
    memcpy(m, p->Nodes[0].Transform, sizeof(m));
    if (p->FlyZ) {
        static const float kFlyZ[12] = { 1, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0 };   // 0x8df834 (Y<->Z)
        Mat34Mul(m, m, kFlyZ);
    }
    static const float kScale[12] = { 0.005f, 0, 0, 0, 0.005f, 0, 0, 0, 0.005f, 0, 0, 0 };   // 0x886ed0
    Mat34Mul(m, m, kScale);
    const float inst[12] = { world[0], world[1], world[2], world[4], world[5], world[6],
                             world[8], world[9], world[10], world[12], world[13], world[14] };
    Mat34Mul(m, m, inst);
    float w44[16];
    Mat34To44(w44, m);
    HD().Device->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)w44);
    p->Nodes[0].Mesh->Draw(scene);
}

SIGepardHD* PzGepard()
{
    if (!s_Facade) {
        s_Facade = new SPzGepard();
        SGepard::PanzersScenePass = &ViewportScenePass;
        SGepard::PanzersPostBoardPass = &ViewportPostBoardPass;
        g_FloraDraw = &DrawFloraInstance;
        if (SwineGepard())
            SwineGepard()->RegisterResetCallbacks(&PreDeviceReset, &PostDeviceReset, nullptr);
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
    SGepard::PanzersPostBoardPass = nullptr;
    g_FloraDraw = nullptr;
    if (SwineGepard())
        SwineGepard()->RegisterResetCallbacks(nullptr, nullptr, nullptr);
    if (s_HD.DebugShadowTexture >= 0)
        s_Facade->ReleaseTexture(s_HD.DebugShadowTexture);
    delete s_Facade;
    s_Facade = nullptr;
    ReleaseMeshVertexDecls();
    if (s_RtSelected >= 0)
        GepardUnselectRenderTarget();
    for (auto& rt : s_Rt)
        if (rt.Used)
            ReleaseRenderTarget(rt);
    ReleaseShaders();
    if (s_HD.DynVB)
        s_HD.DynVB->Release();
    memset(&s_HD, 0, sizeof(s_HD));
}

SPzGepard::SPzGepard()
    : RefCount(1), Primary(nullptr), Pixie(nullptr)
{
    memset(Options, 0, sizeof(Options));
}

SPzGepard::~SPzGepard()
{
    for (int i = Prototypes.Next(-1); i >= 0; i = Prototypes.Next(i)) {
        Prototypes[i]->RefCount = 0;
        delete Prototypes[i];
    }
    Prototypes.Free();
    for (int i = Anims.Next(-1); i >= 0; i = Anims.Next(i))
        delete Anims[i];
    Anims.Free();
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
    // ctor 0x69faf0 logs "Scene created"); stored in the heap. The heap is
    // not kept here.
    HD();
    return new SScene(0);
}

// PANZERS 0x680910
// Options (SGepard +0x4f8): 2 shadow technique (0 off, else GetCap(0) or
// 1), 3 shadow buffer size, 4 shadow-buffer debug texture, 5 shadows of
// free-heap models with a terrain shadow decal, 6 animated-mesh shadows,
// 8/9 texture filter, 10/11 texture detail, 0x10 MODULATE2X lighting.
void SPzGepard::SetOption(unsigned option, int value)
{
    PZ_TRACE("SGepard::SetOption (0x680910)");
    if (0x11 < option) {
        Logger.g->Panic("SGepard::SetOption: Invalid option %d", option);
        return;
    }
    int old = Options[option];
    Options[option] = value;
    switch (option) {
    case 2:
        // Every scene drops its shadow buffer (0x6a2670) and rebuilds it
        // for the new technique on its next frame.
        if (old != value)
            ReleaseAllShadowBuffers();
        break;
    case 4:
        if (old != value) {
            ReleaseAllShadowBuffers();
            if (value != 0) {
                s_HD.DebugShadowTexture = LoadTexture("editor/shadow_buffer_256_hq.tga", 1, true);
                return;
            }
            ReleaseTexture(s_HD.DebugShadowTexture);
            s_HD.DebugShadowTexture = -1;
            return;
        }
        break;
    case 8:
    case 9:
        // HD re-derives every texture's filter flags (0x680bf0(i, 8)); the
        // recompile reads options 8/9 when it binds a texture.
        if (old != value)
            for (auto& rt : s_Rt)
                if (rt.Used)
                    rt.Filter[0] = rt.Filter[1] = (Options[9] != 0) + 1 | (Options[8] != 0 ? 4u : 0u) | 8u;
        break;
    case 10:
    case 11:
        if (old != value)
            Slot_54();   // texture detail reload (0x67fb50)
        break;
    case 0xc:
    case 0xd:
    case 0xf:
        // 0x67cc30 re-applies the default render states; the recompile
        // resets them at every scene pass (pzscene.cpp).
        break;
    default:
        break;
    }
}

// PANZERS 0x67c380
int SPzGepard::GetOption(unsigned option)
{
    PZ_TRACE("SGepard::GetOption (0x67c380)");
    return option < 0x12 ? Options[option] : 0;
}

int GepardOption(unsigned option)
{
    return s_Facade && option < 0x12 ? s_Facade->Options[option] : 0;
}

// PANZERS 0x67a7d0
// SGepard +0x540[cap]. Cap 0 = the shadow technique SGepard::Initialize
// picked by pixel shader version (1..4); cap 1 is never set by HD.
int SPzGepard::GetCap(unsigned cap)
{
    PZ_TRACE("SGepard::GetCap (0x67a7d0)");
    HD();
    return cap == 0 ? s_HD.ShadowCap : 0;
}

// PANZERS 0x67db20
// Returns the prototype index of (file, scale[, sequence file]); loads it
// on first use (.4d -> SPModel::Load4DFile with the file's directory as the
// texture prefix; other files -> the .geo loader 0x693190, not lifted).
int SPzGepard::LoadModelPrototype(const char* file, float scale, const char* p3, int p4)
{
    PZ_TRACE("SGepard::LoadModelPrototype (0x67db20)");
    if (!file || !*file)
        return -1;
    for (int i = Prototypes.Next(-1); i >= 0; i = Prototypes.Next(i)) {
        SPModel* p = Prototypes[i];
        const char* n = p->FileName.buf ? p->FileName.buf : "";
        if (strcmp(n, file) != 0)
            continue;
        if (p3) {
            const char* s = p->SequenceFile.buf ? p->SequenceFile.buf : "";
            if (strcmp(s, p3) != 0 || p->Scale != scale)
                continue;
        } else if (p->SequenceFile.size != 0 || p->Scale != scale) {
            continue;
        }
        p->AddRef();   // 0x68eb50
        return i;
    }
    SPModel* p = new SPModel(p4);   // 0x68e250
    // Directory of the file with a trailing '/' (0x5e4e00 + 0x52c580).
    char dir[512];
    strncpy(dir, file, sizeof(dir) - 2);
    dir[sizeof(dir) - 2] = 0;
    int slash = -1;
    for (int i = 0; dir[i] && dir[i + 1]; ++i)
        if (dir[i] == '/' || dir[i] == '\\')
            slash = i;
    if (slash >= 0)
        dir[slash] = 0;
    else
        strcpy(dir, ".");
    strcat(dir, "/");
    bool ok = false;
    if (strstr(file, "4D") || strstr(file, "4d") || strstr(file, "4DA") || strstr(file, "4da")) {   // 0x8183a8..
        try {
            ok = p->Load4DFile(dir, file, scale);
        } catch (const char* e) {
            Logger.g->Log(0, "SPModel::Load4DFile: %s: %s", file, e);
            ok = false;
        }
    } else {
        STUB_LOG("SPModel::LoadGeoFile (0x693190)");
    }
    if (!ok) {
        p->RefCount = 0;
        delete p;
        return -1;
    }
    p->FileName = file;
    p->SequenceFile = p3;
    p->Scale = scale;
    int i = Prototypes.Add();   // 0x677bc0
    Prototypes[i] = p;
    return i;
}

// PANZERS 0x67f6d0
void SPzGepard::ReleaseModelPrototype(int proto)
{
    PZ_TRACE("SPzGepard::ReleaseModelPrototype (0x67f6d0)");
    if (Prototypes.Valid(proto) && Prototypes[proto]->RefCount > 0)
        --Prototypes[proto]->RefCount;
}

// PANZERS 0x678210
// Deletes the prototypes nobody references any more.
int SPzGepard::PurgeModelPrototypes()
{
    PZ_TRACE("SPzGepard::PurgeModelPrototypes (0x678210)");
    int n = 0;
    for (int i = Prototypes.Next(-1); i >= 0; i = Prototypes.Next(i)) {
        if (Prototypes[i]->RefCount == 0) {
            delete Prototypes[i];
            Prototypes.Remove(i);
            ++n;
        }
    }
    return n;
}

SPModel* GepardModelPrototype(int index)   // 0x6778a0
{
    if (!s_Facade || !s_Facade->Prototypes.Valid(index))
        return nullptr;
    return s_Facade->Prototypes[index];
}

// PANZERS 0x67d920
int GepardLoadAnim(const char* file)
{
    SPzGepard* g = static_cast<SPzGepard*>(PzGepard());
    if (!file || !*file)
        return -1;
    for (int i = g->Anims.Next(-1); i >= 0; i = g->Anims.Next(i)) {
        SPAnim* a = g->Anims[i];
        if (a->Name.buf && !strcmp(a->Name.buf, file)) {
            ++a->RefCount;
            return i;
        }
    }
    SPAnim* a = new SPAnim();
    bool ok;
    try {
        ok = a->LoadAnimFile(file);
    } catch (const char* e) {
        Logger.g->Log(0, "SPAnim::LoadAnimFile: %s: %s", file, e);
        ok = false;
    }
    if (!ok) {
        delete a;
        return -1;
    }
    a->Name = file;
    int i = g->Anims.Add();
    g->Anims[i] = a;
    return i;
}

// PANZERS 0x67a760
SPAnim* GepardAnim(int index)
{
    if (!s_Facade || !s_Facade->Anims.Valid(index))
        return nullptr;
    return s_Facade->Anims[index];
}

void GepardReleaseAnim(int index)
{
    if (s_Facade && s_Facade->Anims.Valid(index) && s_Facade->Anims[index]->RefCount > 0)
        --s_Facade->Anims[index]->RefCount;
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

// PANZERS 0x67ea30 (name handling)
// HD: with the alpha flag the extension becomes ".tga" (0x597390 appends it
// when the name has none); a missing file is retried as ".dxt", then logged
// "Texture is not found" and -1 returned. The decoding and the _hq DXT cache
// stay SWINE's (docs/ENGINE_DIFF.md section 2).
int SPzGepard::LoadTexture(const char* file, int mipmap, bool alpha)
{
    PZ_TRACE("SGepard::LoadTexture (0x67ea30)");
    if (!::Gepard || !file || !*file)
        return -1;
    SString name;
    name = file;
    if (alpha)
        SetExtension(&name, "tga");
    struct _stat st;
    if (FileSystem.Stat(name.buf, &st) != 0) {
        SString dxt;
        dxt = name;
        SetExtension(&dxt, "dxt");
        bool found = FileSystem.Stat(dxt.buf, &st) == 0;
        delete[] dxt.buf;
        if (!found) {
            Logger.g->Log(0, "SGepard::LoadTexture: %s: Texture is not found", name.buf);
            delete[] name.buf;
            return -1;
        }
    }
    bool hasDot = false;
    for (int i = 0; i < name.size; ++i) {
        if (name.buf[i] == '/' || name.buf[i] == '\\')
            hasDot = false;
        else if (name.buf[i] == '.')
            hasDot = true;
    }
    if (!hasDot)
        SetExtension(&name, "tga");   // the SWINE loader needs an extension to swap
    int h = ::Gepard->LoadTexture(name.buf, mipmap != 0, alpha);
    delete[] name.buf;
    return h;
}

void SPzGepard::ReleaseTexture(int texture)
{
    PZ_TRACE("SGepard::ReleaseTexture (0x67f740)");
    if (texture >= kRtTextureBase)
        return;   // offscreen viewport textures go with their viewport
    if (::Gepard && texture >= 0)
        ::Gepard->ReleaseTexture(texture, false);
}

// PANZERS 0x67ca50
void SPzGepard::GetTextureSize(int texture, int* width, int* height)
{
    PZ_TRACE("SPzGepard::GetTextureSize (0x67ca50)");
    bool depth;
    if (const SHdRenderTarget* rt = RtFromTexture(texture, &depth)) {
        *width = rt->Width;
        *height = rt->Height;
        return;
    }
    SGepard* g = SwineGepard();
    if (g && texture >= 0 && texture < g->Textures.size && g->Textures.array[texture].use == 0x7FFFFFFF) {
        *width = (int)g->Textures.array[texture].data.Width;
        *height = (int)g->Textures.array[texture].data.Height;
        return;
    }
    *width = 0;
    *height = 0;
}

// PANZERS 0x67c9e0
// HD texture record +0x20: 0 opaque, 1 1-bit alpha (test), 2 alpha blend.
int GepardGetTextureAlpha(int texture)
{
    SGepard* g = SwineGepard();
    if (!g || texture < 0 || texture >= kRtTextureBase)
        return 0;
    return g->GetTextureAlpha(texture);
}

// PANZERS 0x680c90
// Binds a texture handle; the sampler filters come from the texture's
// filter flags (HD record +0x24, from options 8/9 via 0x680bf0: bit 0-1
// min/mag 0 point, 1 linear, 2 anisotropic; bit 2 linear mip).
void GepardSetTexture(unsigned stage, int texture)
{
    IDirect3DDevice9* dev = HD().Device;
    SGepard* g = SwineGepard();
    if (!dev)
        return;
    unsigned flags;
    bool depth;
    if (const SHdRenderTarget* rt = RtFromTexture(texture, &depth)) {
        dev->SetTexture(stage, depth ? rt->DepthTexture : rt->ColorTexture);
        flags = rt->Filter[depth ? 1 : 0];
    } else if (!g || texture < 0 || texture >= g->Textures.size || g->Textures.array[texture].use != 0x7FFFFFFF) {
        dev->SetTexture(stage, nullptr);
        return;
    } else {
        dev->SetTexture(stage, g->Textures.array[texture].data.lpTexture);
        flags = (GepardOption(9) != 0) + 1 | (GepardOption(8) != 0 ? 4u : 0u) | 8u;   // 0x680bf0(idx, 8)
    }
    switch (flags & 3) {
    case 0:
        dev->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
        dev->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_POINT);
        break;
    case 1:
        dev->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        dev->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
        break;
    case 2:
        dev->SetSamplerState(stage, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
        dev->SetSamplerState(stage, D3DSAMP_MINFILTER, D3DTEXF_ANISOTROPIC);
        dev->SetSamplerState(stage, D3DSAMP_MAXANISOTROPY, 4);
        break;
    }
    dev->SetSamplerState(stage, D3DSAMP_MIPFILTER, (flags & 4) ? D3DTEXF_LINEAR : D3DTEXF_POINT);
}

IDirect3DVertexDeclaration9* MeshVertexDecl(int i);   // mesh.cpp

// PANZERS 0x680e80
void GepardSetVertexFormat(int decl, unsigned fvf)
{
    IDirect3DDevice9* dev = HD().Device;
    if (decl == -1) {
        dev->SetFVF(fvf);
        return;
    }
    IDirect3DVertexDeclaration9* d = MeshVertexDecl(decl);
    if (!d) {
        Logger.g->Panic("SGepard::SetVertexDeclaration: invalid index");
        return;
    }
    dev->SetVertexDeclaration(d);
}

// PANZERS 0x680fb0
void GepardSetWorld(const float* m)
{
    float w[16];
    Mat34To44(w, m);
    HD().Device->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)w);
}

// PANZERS 0x680fe0
void GepardSetWorldIdentity()
{
    float w[16];
    memset(w, 0, sizeof(w));
    w[0] = w[5] = w[10] = w[15] = 1.0f;
    HD().Device->SetTransform(D3DTS_WORLD, (const D3DMATRIX*)w);
}

// PANZERS 0x680f40
// HD creates only ids 0x41/0x42 (vs_2_0); ids 1..0x40 stay null.
void GepardSetVertexShader(int id)
{
    IDirect3DVertexShader9* vs = nullptr;
    if (id != 0) {
        if (id < 1 || id > 0x42 || !HD().VertexShaders[id]) {
            Logger.g->Panic("SGepard::SetVertexShader: invalid index");
            return;
        }
        vs = HD().VertexShaders[id];
    }
    HD().Device->SetVertexShader(vs);
}

// PANZERS 0x680ac0
void GepardSetPixelShader(int id)
{
    IDirect3DPixelShader9* ps = nullptr;
    if (id != 0) {
        if (0x21 < (unsigned)(id - 1) || !HD().PixelShaders[id]) {
            Logger.g->Panic("SGepard::SetPixelShader: invalid index");
            return;
        }
        ps = HD().PixelShaders[id];
    }
    HRESULT hr = HD().Device->SetPixelShader(ps);
    if (FAILED(hr))
        Logger.g->Panic("%s: %08x", "SGepard::SetPixelShader: SetPixelShader failed", (unsigned)hr);
}

// device +0x1b4 / +0x178 (the HD call sites pass SGepard +0x478 directly)
void GepardSetPixelShaderConstant(unsigned reg, const float* v, unsigned count)
{
    HD().Device->SetPixelShaderConstantF(reg, v, count);
}

void GepardSetVertexShaderConstant(unsigned reg, const float* v, unsigned count)
{
    HD().Device->SetVertexShaderConstantF(reg, v, count);
}

// PANZERS 0x67a720
void GepardEnableLights(bool on)
{
    for (int i = 0; i < HD().LightCount; ++i)
        HD().Device->LightEnable(i, on);
}

// PANZERS 0x5aa900 (clamped float4 -> ARGB)
unsigned GepardArgb(const float* c)
{
    auto ch = [](float v) -> unsigned {
        if (1.0f <= v) return 0xff;
        if (v < 0.0f) return 0;
        return (unsigned)lrintf(v * 255.0f);
    };
    unsigned a = ch(c[3]), r = ch(c[0]);
    unsigned g = c[3] < 0.0f ? 0 : ch(c[1]);
    unsigned b = c[3] < 0.0f ? 0 : ch(c[2]);
    return b | ((a << 8 | r) << 8 | g) << 8;
}

// PANZERS 0x680510
void GepardSetAmbient(const float* c)
{
    HD().Device->SetRenderState(D3DRS_AMBIENT, GepardArgb(c));
}

// PANZERS 0x67f150
void* GepardLockDynamicVB(int type, int count)
{
    SGepardHDState& hd = HD();
    if (type < 0 || type >= hd.DynTypeCount) {
        Logger.g->Panic("SGepard::LockDynamicVB: Bad index (%d).", type);
        return nullptr;
    }
    if (hd.DynVBLocked) {
        Logger.g->Panic("SGepard::LockDynamicVB: Already locked.");
        return nullptr;
    }
    unsigned size = hd.DynTypeStride[type] * count;
    if (hd.DynVBSize < size) {
        Logger.g->Panic("SGepard::LockDynamicVB: VertexBuffer is too small.");
        return nullptr;
    }
    if (hd.DynVBSize < hd.DynVBOffset + size)
        hd.DynVBOffset = 0;
    DWORD lockFlags = hd.DynVBOffset ? D3DLOCK_NOOVERWRITE : D3DLOCK_DISCARD;
    void* p = nullptr;
    if (!hd.DynVB || FAILED(hd.DynVB->Lock(hd.DynVBOffset, size, &p, lockFlags))) {
        Logger.g->Panic("SGepard::LockDynamicVB: IDirect3DVertexBuffer8::Lock");
        return nullptr;
    }
    hd.DynVBType = type;
    hd.DynVBLocked = true;
    return p;
}

// PANZERS 0x681440
void GepardUnlockDynamicVB()
{
    SGepardHDState& hd = HD();
    if (!hd.DynVBLocked) {
        Logger.g->Panic("SGepard::UnlockDynamicVB: Not locked.");
        return;
    }
    hd.DynVB->Unlock();
    hd.DynVBLocked = false;
}

// PANZERS 0x67a650
void GepardDrawDynamicVB(int prim, int minIndex, int numVerts, int startIndex, int primCount, int withFvf)
{
    SGepardHDState& hd = HD();
    if (hd.DynVBLocked) {
        Logger.g->Panic("SGepard::DrawIndexedDynamicVB: Buffer is locked.");
        return;
    }
    if (primCount == 0)
        return;
    IDirect3DDevice9* dev = hd.Device;
    dev->SetStreamSource(0, hd.DynVB, hd.DynVBOffset, hd.DynTypeStride[hd.DynVBType]);
    if (withFvf)
        dev->SetFVF(hd.DynTypeFvf[hd.DynVBType]);
    dev->DrawIndexedPrimitive((D3DPRIMITIVETYPE)prim, 0, minIndex, numVerts, startIndex, primCount);
    hd.PolyCount += primCount;
    hd.VertexCount += numVerts;
}

// PANZERS 0x677fc0
void GepardAdvanceDynamicVB(int count)
{
    SGepardHDState& hd = HD();
    if (hd.DynVBLocked) {
        Logger.g->Panic("SGepard::AdvanceDynamicVB: Buffer is locked.");
        return;
    }
    hd.DynVBOffset += hd.DynTypeStride[hd.DynVBType] * count;
}

SIPixie* GepardPixie()
{
    SPzGepard* g = static_cast<SPzGepard*>(PzGepard());
    if (!g->Pixie) {
        g->GetPixie();
        g->Pixie->Release();
    }
    return g->Pixie;
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

// PANZERS 0x67a3f0
// Deletes a viewport of the heap; the recompile's heap holds only the
// offscreen ones (the primary viewport is the facade's).
void SPzGepard::DestroyViewport(int index)
{
    PZ_TRACE("SGepard::DestroyViewport (0x67a3f0)");
    GepardDestroyRenderTarget(index);
}

// HD SPzGepard vtbl +0x4c -> 0x681540 (4 arg dwords)
int HdBitmapBpp(int format)
{
    switch (format) {
    case 1: return 3;
    case 2: case 3: case 0x21: case 0x23: return 4;
    case 4: case 5: case 6: case 7: case 10: case 0xb: case 0xf: case 0x1d: case 0x20: return 2;
    case 8: case 9: case 0xe: case 0x10: return 1;
    default: return 0;
    }
}

// PANZERS 0x681540
// Blits the bitmap into the texture at (x, y): HD locks the texture as an
// SBitmap (0x6c0f20), BitBlt 0x669d60 converts, unlock 0x6c1080. The
// recompile locks the SWINE texture's level 0 and converts the 24/32-bit
// formats the minimap uses (1 -> 32 bit; 2/3 copied as they are).
void SPzGepard::UpdateTexture(int texture, int x, int y, void* data)
{
    PZ_TRACE("SPzGepard::UpdateTexture (0x681540)");
    SGepard* g = SwineGepard();
    const SHdBitmap* b = (const SHdBitmap*)data;
    if (!g || texture < 0 || texture >= g->Textures.size || g->Textures.array[texture].use != 0x7FFFFFFF ||
        !g->Textures.array[texture].data.lpTexture || !b) {
        Logger.g->Log(0, "SGepard::UpdateTexture: Invalid texture");   // HD panics
        return;
    }
    IDirect3DTexture9* t = g->Textures.array[texture].data.lpTexture;
    D3DSURFACE_DESC desc;
    if (FAILED(t->GetLevelDesc(0, &desc)))
        return;
    if (desc.Format != D3DFMT_A8R8G8B8 && desc.Format != D3DFMT_X8R8G8B8) {
        Logger.g->Log(0, "SGepard::UpdateTexture: texture format %d not handled", (int)desc.Format);
        return;
    }
    int bpp = HdBitmapBpp(b->Format);
    if (bpp != 3 && bpp != 4) {
        Logger.g->Log(0, "SGepard::UpdateTexture: bitmap format %d not handled", b->Format);
        return;
    }
    // 0x669d60: clip to both bitmaps.
    int w = b->Width, h = b->Height;
    if ((int)desc.Width - x < w) w = (int)desc.Width - x;
    if ((int)desc.Height - y < h) h = (int)desc.Height - y;
    if (w <= 0 || h <= 0 || x < 0 || y < 0)
        return;
    RECT r = { x, y, x + w, y + h };
    D3DLOCKED_RECT lr;
    if (FAILED(t->LockRect(0, &lr, &r, 0)))
        return;
    for (int row = 0; row < h; ++row) {
        const unsigned char* src = b->Data + b->Start + row * b->Pitch;
        unsigned char* dst = (unsigned char*)lr.pBits + row * lr.Pitch;
        if (bpp == 4) {
            memcpy(dst, src, (size_t)w * 4);                    // 0x66a460 (3 <- 2 / same format)
        } else {
            for (int i = 0; i < w; ++i) {                       // 0x66e210 (1 -> 32 bit)
                dst[i * 4 + 0] = src[i * 3 + 0];
                dst[i * 4 + 1] = src[i * 3 + 1];
                dst[i * 4 + 2] = src[i * 3 + 2];
                dst[i * 4 + 3] = 0xff;
            }
        }
    }
    t->UnlockRect(0);
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
