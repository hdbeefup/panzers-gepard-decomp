// src/3dengine/pz/parcel.cpp
// Terrain parcels and the fixed-function draw state of the HD terrain
// passes. OWNER: agent B. See parcel.h.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "parcel.h"
#include "pzterrain.h"
#include "igepardhd.h"
#include "gepard.h"
#include "pzgepard.h"
#include "logger.h"
#include "stub_log.h"

extern SIGepard* Gepard;   // SWINE renderer (window/widget.h)

namespace pz {

SHdFog g_HdFog = { false, 0, 0.0f, 0.0f };

static SGepard* SwineGepard()
{
    return static_cast<SGepard*>(::Gepard);
}

IDirect3DDevice9* TerrainDevice()
{
    SGepard* g = SwineGepard();
    return g ? g->lpD3DDev : nullptr;
}

static const STextureProp* TextureProp(int texture)
{
    SGepard* g = SwineGepard();
    if (!g || texture < 0 || texture >= g->Textures.size || g->Textures.array[texture].use != 0x7FFFFFFF)
        return nullptr;
    return &g->Textures.array[texture].data;
}

int TerrainTextureAlpha(int texture)
{
    // HD 0x67c9e0 returns the texture record +0x20 (0 opaque, 1 alpha test,
    // 2 alpha blend), 0 for a bad handle. SWINE keeps the same classes.
    const STextureProp* t = TextureProp(texture);
    return t ? t->Alpha : 0;
}

bool TerrainTextureSize(int texture, int* w, int* h)
{
    // Gepard +0x50 (HD 0x67ca50); the facade slot is still a stub, so read
    // the SWINE table directly.
    const STextureProp* t = TextureProp(texture);
    *w = t ? (int)t->Width : 0;
    *h = t ? (int)t->Height : 0;
    return t != nullptr;
}

int TerrainAddRefTexture(int texture)
{
    // HD 0x677f20: ++refcount of a live texture, returns the handle.
    SGepard* g = SwineGepard();
    if (g && TextureProp(texture))
        g->AddRefTexture(texture);
    return texture;
}

void TerrainReleaseTexture(int texture)
{
    if (texture >= 0)
        PzGepard()->ReleaseTexture(texture);
}

int TerrainLoadTexture(const char* file, int mipmap, bool alpha)
{
    return PzGepard()->LoadTexture(file, mipmap, alpha);
}

int TerrainOption(unsigned option)
{
    return PzGepard()->GetOption(option);
}

void TerrainSetWorldIdentity(IDirect3DDevice9* dev)
{
    // PANZERS 0x680fe0
    D3DMATRIX m;
    memset(&m, 0, sizeof(m));
    m._11 = m._22 = m._33 = m._44 = 1.0f;
    dev->SetTransform(D3DTS_WORLD, &m);
}

bool g_TerrainDrawKeepShaders = false;

void TerrainDraw(IDirect3DDevice9* dev, DWORD fvf, const void* verts, int nverts, int stride,
                 const unsigned short* idx, int nidx)
{
    // HD: Gepard dynamic VB/IB (0x67f150 lock, 0x67a650 / 0x67a550 draw).
    // UP draw here: nothing in D3DPOOL_DEFAULT to rebuild on device reset.
    if (!dev || nverts <= 0 || nidx < 3)
        return;
    if (!g_TerrainDrawKeepShaders) {
        dev->SetVertexShader(nullptr);
        dev->SetPixelShader(nullptr);
    }
    dev->SetFVF(fvf);
    dev->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, nverts, nidx / 3, idx, D3DFMT_INDEX16,
                                verts, stride);
}

// ------------------------------------------------------------------------
// STerrainDrawState

// PANZERS 0x687730
void STerrainDrawState::Reset()
{
    // 0x687730 clears the texture transforms, then 0x6883a0 sets the rest.
    Lighting = true;
    memset(&Material, 0, sizeof(Material));
    Material.Diffuse.r = Material.Diffuse.g = Material.Diffuse.b = Material.Diffuse.a = 1.0f;
    Material.Ambient.r = Material.Ambient.g = Material.Ambient.b = Material.Ambient.a = 1.0f;
    AmbientSource = D3DMCS_COLOR1;
    DiffuseSource = D3DMCS_COLOR1;
    SpecularSource = D3DMCS_COLOR2;
    EmissiveSource = D3DMCS_MATERIAL;
    ShadeMode = D3DSHADE_GOURAUD;
    CullMode = D3DCULL_CW;
    for (int i = 0; i < 2; ++i) {
        Textures[i].Handle = -1;
        Textures[i].AddrU = D3DTADDRESS_WRAP;
        Textures[i].AddrV = D3DTADDRESS_WRAP;
    }
    TextureFactor = 0xffffffff;
    Stages[0].ColorOp = D3DTOP_MODULATE;
    Stages[0].ColorArg1 = D3DTA_TEXTURE;
    Stages[0].ColorArg2 = D3DTA_CURRENT;
    Stages[0].ColorArg0 = D3DTA_CURRENT;
    Stages[0].AlphaOp = D3DTOP_SELECTARG1;
    Stages[0].AlphaArg1 = D3DTA_TEXTURE;
    Stages[0].AlphaArg2 = D3DTA_CURRENT;
    Stages[1].ColorOp = D3DTOP_DISABLE;
    Stages[1].ColorArg1 = D3DTA_TEXTURE;
    Stages[1].ColorArg2 = D3DTA_CURRENT;
    Stages[1].ColorArg0 = D3DTA_CURRENT;
    Stages[1].AlphaOp = D3DTOP_DISABLE;
    Stages[1].AlphaArg1 = D3DTA_TEXTURE;
    Stages[1].AlphaArg2 = D3DTA_CURRENT;
    ColorWrite = 0x0f;
    Blend = false;
    SrcBlend = D3DBLEND_ONE;
    DestBlend = D3DBLEND_ZERO;
    AlphaTest = false;
    AlphaRef = 0;
    AlphaFunc = D3DCMP_ALWAYS;
    ZEnable = true;
    ZFunc = D3DCMP_LESSEQUAL;
    ZWrite = true;
    FogMode = 0;
    FogColor = 0;
}

// PANZERS 0x688d80
void STerrainDrawState::SetTexture(unsigned stage, int texture, bool wrap)
{
    Textures[stage].Handle = texture;
    Textures[stage].AddrU = Textures[stage].AddrV = wrap ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
}

// PANZERS 0x688d20
void STerrainDrawState::SetTexture(unsigned stage, int texture, bool wrapU, bool wrapV)
{
    Textures[stage].Handle = texture;
    Textures[stage].AddrU = wrapU ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
    Textures[stage].AddrV = wrapV ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
}

// PANZERS 0x6888f0
void STerrainDrawState::SetColorOp(unsigned stage, DWORD op, DWORD arg1, DWORD arg2, DWORD arg0)
{
    Stages[stage].ColorOp = op;
    Stages[stage].ColorArg1 = arg1;
    Stages[stage].ColorArg2 = arg2;
    Stages[stage].ColorArg0 = arg0;
}

// PANZERS 0x688870
void STerrainDrawState::SetAlphaOp(unsigned stage, DWORD op, DWORD arg1, DWORD arg2)
{
    Stages[stage].AlphaOp = op;
    Stages[stage].AlphaArg1 = arg1;
    Stages[stage].AlphaArg2 = arg2;
}

// PANZERS 0x688aa0
void STerrainDrawState::SetFog(int mode, unsigned color)
{
    FogMode = mode;
    FogColor = color;
}

// PANZERS 0x6887d0
void STerrainDrawState::SetBlendMode(int mode)
{
    if (mode == 2) {
        Blend = true;
        SrcBlend = D3DBLEND_SRCALPHA;
        DestBlend = D3DBLEND_INVSRCALPHA;
        AlphaTest = true;
        AlphaRef = 4;
        AlphaFunc = D3DCMP_GREATER;
        ZWrite = false;
        return;
    }
    Blend = false;
    SrcBlend = D3DBLEND_ONE;
    DestBlend = D3DBLEND_ZERO;
    ZWrite = true;
    if (mode == 1) {
        AlphaTest = true;
        AlphaRef = 0x80;
        AlphaFunc = D3DCMP_GREATER;
        return;
    }
    AlphaTest = false;
    AlphaRef = 0;
    AlphaFunc = D3DCMP_ALWAYS;
}

// PANZERS 0x688980
void STerrainDrawState::SetBlendModeEx(int mode, int texture)
{
    if (mode == 1) {
        Blend = true;
        SrcBlend = D3DBLEND_ONE;
        DestBlend = D3DBLEND_ONE;
        AlphaTest = false;
        AlphaRef = 0;
        AlphaFunc = D3DCMP_ALWAYS;
        ZWrite = false;
        FogMode = 1;
        return;
    }
    if (mode == 2) {
        SrcBlend = D3DBLEND_ZERO;
        DestBlend = D3DBLEND_INVSRCCOLOR;
    } else if (mode == 3) {
        SrcBlend = D3DBLEND_ZERO;
        DestBlend = D3DBLEND_SRCCOLOR;
    } else if (mode == 4) {
        SrcBlend = D3DBLEND_DESTCOLOR;
        DestBlend = D3DBLEND_SRCCOLOR;
    } else {
        SetBlendMode(TerrainTextureAlpha(texture));
        return;
    }
    Blend = true;
    AlphaTest = false;
    AlphaRef = 0;
    AlphaFunc = D3DCMP_ALWAYS;
    ZWrite = false;
}

static DWORD FloatBits(float f)
{
    DWORD d;
    memcpy(&d, &f, sizeof(d));
    return d;
}

// PANZERS 0x687a50
void STerrainDrawState::Apply(IDirect3DDevice9* dev) const
{
    // HD caches every state in globals and sends only changes; the
    // recompile sends them all (the SWINE board shares the device).
    if (!dev)
        return;
    dev->SetVertexShader(nullptr);
    dev->SetPixelShader(nullptr);
    dev->SetRenderState(D3DRS_LIGHTING, Lighting);
    if (Lighting) {
        dev->SetMaterial(&Material);
        dev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, AmbientSource);
        dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, DiffuseSource);
        dev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, SpecularSource);
        dev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, EmissiveSource);
        dev->SetRenderState(D3DRS_COLORVERTEX, TRUE);
    }
    dev->SetRenderState(D3DRS_SHADEMODE, ShadeMode);
    dev->SetRenderState(D3DRS_CULLMODE, CullMode);
    dev->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
    for (DWORD i = 0; i < 2; ++i) {
        dev->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i);
        dev->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
        const STextureProp* t = TextureProp(Textures[i].Handle);
        dev->SetTexture(i, t ? t->lpTexture : nullptr);
        dev->SetSamplerState(i, D3DSAMP_ADDRESSU, Textures[i].AddrU);
        dev->SetSamplerState(i, D3DSAMP_ADDRESSV, Textures[i].AddrV);
    }
    dev->SetRenderState(D3DRS_TEXTUREFACTOR, TextureFactor);
    for (DWORD i = 0; i < 2; ++i) {
        const Stage& s = Stages[i];
        dev->SetTextureStageState(i, D3DTSS_COLOROP, s.ColorOp);
        if (s.ColorOp != D3DTOP_DISABLE) {
            dev->SetTextureStageState(i, D3DTSS_COLORARG1, s.ColorArg1);
            dev->SetTextureStageState(i, D3DTSS_COLORARG2, s.ColorArg2);
            dev->SetTextureStageState(i, D3DTSS_COLORARG0, s.ColorArg0);
            dev->SetTextureStageState(i, D3DTSS_RESULTARG, D3DTA_CURRENT);
        }
        dev->SetTextureStageState(i, D3DTSS_ALPHAOP, s.AlphaOp);
        if (s.AlphaOp != D3DTOP_DISABLE) {
            dev->SetTextureStageState(i, D3DTSS_ALPHAARG1, s.AlphaArg1);
            dev->SetTextureStageState(i, D3DTSS_ALPHAARG2, s.AlphaArg2);
        }
    }
    dev->SetTextureStageState(2, D3DTSS_COLOROP, D3DTOP_DISABLE);
    dev->SetTextureStageState(2, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, ColorWrite);
    dev->SetRenderState(D3DRS_ALPHABLENDENABLE, Blend);
    if (Blend) {
        dev->SetRenderState(D3DRS_SRCBLEND, SrcBlend);
        dev->SetRenderState(D3DRS_DESTBLEND, DestBlend);
    }
    dev->SetRenderState(D3DRS_ALPHATESTENABLE, AlphaTest);
    if (AlphaTest) {
        dev->SetRenderState(D3DRS_ALPHAREF, AlphaRef);
        dev->SetRenderState(D3DRS_ALPHAFUNC, AlphaFunc);
    }
    dev->SetRenderState(D3DRS_ZENABLE, ZEnable ? D3DZB_TRUE : D3DZB_FALSE);
    if (ZEnable) {
        dev->SetRenderState(D3DRS_ZFUNC, ZFunc);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, ZWrite);
    }
    dev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    // Fog: modes 0/1 use the scene fog (HD globals 0x92f0e0..0x92f0ec),
    // linear vertex range fog; mode 3 a fixed colour whose alpha byte sets
    // the density (start -10 * a, end 10 * (255 - a)).
    switch (FogMode) {
    case 0:
    case 1:
        if (g_HdFog.Enabled) {
            dev->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);
            dev->SetRenderState(D3DRS_FOGCOLOR, FogMode == 0 ? g_HdFog.Color : 0);
            dev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
            dev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            dev->SetRenderState(D3DRS_FOGSTART, FloatBits(g_HdFog.Start));
            dev->SetRenderState(D3DRS_FOGEND, FloatBits(g_HdFog.End));
            dev->SetRenderState(D3DRS_FOGENABLE, TRUE);
        } else {
            dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
        }
        break;
    case 3: {
        int a = (int)(FogColor >> 24);
        dev->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);
        dev->SetRenderState(D3DRS_FOGCOLOR, FogColor);
        dev->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
        dev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        dev->SetRenderState(D3DRS_FOGSTART, FloatBits((float)(a * -10)));
        dev->SetRenderState(D3DRS_FOGEND, FloatBits((float)((0xff - a) * 10)));
        dev->SetRenderState(D3DRS_FOGENABLE, TRUE);
        break;
    }
    default:
        dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
        break;
    }
}

// ------------------------------------------------------------------------
// Shared parcel index list (SParcel ctor 0x708180).

unsigned short g_ParcelIndices[0x180];

static void BuildParcelIndices()
{
    static bool built = false;
    if (built)
        return;
    built = true;
    int n = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            unsigned short v = (unsigned short)(r * 9 + c);
            g_ParcelIndices[n++] = v;
            g_ParcelIndices[n++] = (unsigned short)(v + 1);
            g_ParcelIndices[n++] = (unsigned short)(v + 9);
            g_ParcelIndices[n++] = (unsigned short)(v + 9);
            g_ParcelIndices[n++] = (unsigned short)(v + 1);
            g_ParcelIndices[n++] = (unsigned short)(v + 10);
        }
    }
}

// ------------------------------------------------------------------------
// SParcel

// PANZERS 0x708180
SParcel::SParcel(STerrain* terrain)
    : Terrain(terrain)
{
    PZ_TRACE("SParcel::SParcel (0x708180)");
    BuildParcelIndices();
}

SParcel::~SParcel()
{
}

// Fills the 9x9 vertex block of a parcel (DrawLayered / DrawSketch inner loop).
static void FillParcel(STerrainVertex* v, int x0, int z0, const float* heights,
                       const float* normals, int stride)
{
    for (int r = 0; r < 9; ++r) {
        const float* h = heights + r * stride;
        const float* n = normals + r * stride * 3;
        for (int c = 0; c < 9; ++c, ++v) {
            v->X = (float)(x0 + c);
            v->Y = h[c];
            v->Z = (float)(z0 + r);
            v->NX = n[c * 3 + 0];
            v->NY = n[c * 3 + 1];
            v->NZ = n[c * 3 + 2];
        }
    }
}

// PANZERS 0x708ee0
void SParcel::DrawLayered(int x0, int z0, const float* heights, const unsigned* diffuse,
                          const unsigned char* blend, const float* normals, int stride,
                          int baseLayer, const int* textures, const unsigned char* layers,
                          bool shadowPass)
{
    PZ_TRACE("SParcel::DrawLayered (0x708ee0)");
    IDirect3DDevice9* dev = TerrainDevice();
    if (!dev)
        return;

    // Option 0x11 (debug): colour parcels by layer count (>3 green, 5 magenta, >5 red).
    bool debug = false;
    unsigned debugColor = 0;
    if (TerrainOption(0x11) != 0 && baseLayer < 0x10) {
        int count = 1;
        for (int k = baseLayer; k < 0x10; ++k)
            if (layers[k])
                ++count;
        if (count > 3) {
            debug = true;
            debugColor = count > 5 ? 0xff0000 : (count > 4 ? 0xff00ff : 0xff00);
        }
    }

    STerrainVertex v[0x51];
    FillParcel(v, x0, z0, heights, normals, stride);

    // Base layer: opaque, vertex colour = DIFF with alpha 0xff.
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) {
            STerrainVertex& o = v[r * 9 + c];
            o.Color = (debug ? debugColor : diffuse[r * stride + c]) | 0xff000000;
            o.U = (float)c * 0.125f;
            o.V = (float)((double)r * 0.125);
        }
    STerrainDrawState st;
    if (!shadowPass && TerrainOption(0x10) != 0)
        st.SetColorOp(0, D3DTOP_MODULATE2X, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
    st.SetAlphaOp(0, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_CURRENT);
    st.SetTexture(0, textures[baseLayer], true);
    if (shadowPass)
        { st.SetFog(2, 0); st.Lighting = false; }   // 0x709572: unlit, the shadow pass lights it
    st.Apply(dev);
    TerrainDraw(dev, kTerrainFVF, v, 0x51, sizeof(STerrainVertex), g_ParcelIndices, 0x180);

    // Layers above the base: texture k+1, vertex alpha = blend byte k. A
    // layer whose texture carries its own alpha (0x67c9e0 != 0) is skipped.
    for (int k = baseLayer; k < 0x10; ++k) {
        int tex = textures[k + 1];
        if (!layers[k] || TerrainTextureAlpha(tex) != 0)
            continue;
        for (int r = 0; r < 9; ++r)
            for (int c = 0; c < 9; ++c) {
                unsigned rgb = debug ? debugColor : (diffuse[r * stride + c] & 0xffffff);
                v[r * 9 + c].Color = ((unsigned)blend[(r * stride + c) * 0x10 + k] << 24) | rgb;
            }
        st.Reset();
        if (!shadowPass && TerrainOption(0x10) != 0)
            st.SetColorOp(0, D3DTOP_MODULATE2X, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
        st.SetAlphaOp(0, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_CURRENT);
        st.SetTexture(0, tex, true);
        st.SetBlendMode(2);
        if (shadowPass)
            { st.SetFog(2, 0); st.Lighting = false; }   // 0x709c8f: unlit, the shadow pass lights it
        st.Apply(dev);
        TerrainDraw(dev, kTerrainFVF, v, 0x51, sizeof(STerrainVertex), g_ParcelIndices, 0x180);
    }
}

// PANZERS 0x70a930
void SParcel::DrawSketch(int x0, int z0, const float* heights, const unsigned* diffuse,
                         unsigned color, const float* normals, int stride, float uScale,
                         float vScale, bool fixedFunction)
{
    PZ_TRACE("SParcel::DrawSketch (0x70a930)");
    IDirect3DDevice9* dev = TerrainDevice();
    if (!dev)
        return;
    STerrainVertex v[0x51];
    FillParcel(v, x0, z0, heights, normals, stride);
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) {
            STerrainVertex& o = v[r * 9 + c];
            o.Color = diffuse ? (diffuse[r * stride + c] | 0xff000000) : color;
            o.U = (float)(x0 + c) * uScale;
            o.V = 1.0f - (float)(z0 + r) * vScale;
        }
    // Gepard option 2 == 4 (and !fixedFunction) uses vertex shader 0x41 with
    // the world/view/projection and texture matrices; not ported (no HD
    // shader on the PS 2.0 path).
    (void)fixedFunction;
    TerrainDraw(dev, kTerrainFVF, v, 0x51, sizeof(STerrainVertex), g_ParcelIndices, 0x180);
}

void SParcel::Draw(int p1) { (void)p1; }
void SParcel::DrawShadow(int p1) { (void)p1; }
void SParcel::Slot_0C() {}
void SParcel::Slot_10() {}
void SParcel::Slot_14() {}
void SParcel::Slot_18() {}
void SParcel::Slot_1C() {}
void SParcel::Slot_20() {}
void SParcel::Slot_24() {}
void SParcel::Slot_28() {}
void SParcel::CreateVertexBuffer(int p1, int p2) { (void)p1; (void)p2; }
void SParcel::Slot_30() {}
void SParcel::Slot_34() {}

// ------------------------------------------------------------------------
// SWireframeParcel

// PANZERS 0x708400
SWireframeParcel::SWireframeParcel(STerrain* terrain, bool outline)
    : Terrain(terrain), Outline(outline), IndexCount(0)
{
    if (!outline) {
        // Grid: per row r and column c the edges (v, v+1) and (v, v+9).
        for (int r = 0; r < 8; ++r)
            for (int c = 0; c < 8; ++c) {
                unsigned short v = (unsigned short)(r * 9 + c);
                Indices[IndexCount++] = v;
                Indices[IndexCount++] = (unsigned short)(v + 1);
                Indices[IndexCount++] = v;
                Indices[IndexCount++] = (unsigned short)(v + 9);
            }
    } else {
        // Outline: top edge 0..8 and left edge 0..72.
        for (int c = 0; c < 8; ++c) {
            Indices[IndexCount++] = (unsigned short)c;
            Indices[IndexCount++] = (unsigned short)(c + 1);
        }
        for (int r = 0; r < 8; ++r) {
            Indices[IndexCount++] = (unsigned short)(r * 9);
            Indices[IndexCount++] = (unsigned short)(r * 9 + 9);
        }
    }
}

SWireframeParcel::~SWireframeParcel()
{
}

// HD 0x70b050 (reimplemented: same vertices, line list; HD lifts the lines
// by its own offset, not checked)
void SWireframeParcel::DrawWireframe(int x0, int z0, const float* heights, int stride, unsigned color)
{
    IDirect3DDevice9* dev = TerrainDevice();
    if (!dev)
        return;
    struct V { float X, Y, Z; unsigned C; } v[0x51];
    for (int r = 0; r < 9; ++r)
        for (int c = 0; c < 9; ++c) {
            V& o = v[r * 9 + c];
            o.X = (float)(x0 + c);
            o.Y = heights[r * stride + c];
            o.Z = (float)(z0 + r);
            o.C = color;
        }
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    dev->DrawIndexedPrimitiveUP(D3DPT_LINELIST, 0, 0x51, IndexCount / 2, Indices, D3DFMT_INDEX16,
                                v, sizeof(V));
}

void SWireframeParcel::Draw(int p1) { (void)p1; }
void SWireframeParcel::DrawShadow(int p1) { (void)p1; }
void SWireframeParcel::Slot_0C() {}
void SWireframeParcel::Slot_10() {}
void SWireframeParcel::Slot_14() {}
void SWireframeParcel::Slot_18() {}
void SWireframeParcel::Slot_1C() {}
void SWireframeParcel::Slot_20() {}
void SWireframeParcel::Slot_24() {}
void SWireframeParcel::Slot_28() {}
void SWireframeParcel::CreateVertexBuffer(int p1, int p2) { (void)p1; (void)p2; }
void SWireframeParcel::Slot_30() {}
void SWireframeParcel::Slot_34() {}

// ------------------------------------------------------------------------
// SBlockMapParcel

// PANZERS 0x704e70
SBlockMapParcel::SBlockMapParcel(STerrain* terrain, int p)
    : Terrain(terrain), P(p), Markers(nullptr), MarkerCount(0), MarkerCap(0)
{
    int n = p * 8 + 1;
    NVerts = n * n;
    NTris = p * p * 0x80;
    Indices = new unsigned short[p * p * 0x180];
    int k = 0;
    for (int r = 0; r < p * 8; ++r)
        for (int c = 0; c < p * 8; ++c) {
            unsigned short a = (unsigned short)(n * r + c);
            unsigned short b = (unsigned short)(n + a);
            Indices[k++] = b;
            Indices[k++] = a;
            Indices[k++] = (unsigned short)(a + 1);
            Indices[k++] = b;
            Indices[k++] = (unsigned short)(a + 1);
            Indices[k++] = (unsigned short)(b + 1);
        }
}

SBlockMapParcel::~SBlockMapParcel()
{
    delete[] Indices;
    free(Markers);
}

// HD 0x709d20 (reimplemented: blockmap cells coloured by their passability
// bits; the exact HD colour table was not lifted)
void SBlockMapParcel::DrawParcel(int x0, int z0, const float* heights, int stride,
                                 const unsigned* blockmap, const unsigned char* aux, int bmStride)
{
    (void)aux;
    IDirect3DDevice9* dev = TerrainDevice();
    if (!dev)
        return;
    int n = P * 8 + 1;
    struct V { float X, Y, Z; unsigned C; };
    V* v = new V[n * n];
    float inv = 1.0f / (float)P;
    for (int r = 0; r < n; ++r)
        for (int c = 0; c < n; ++c) {
            V& o = v[r * n + c];
            o.X = (float)x0 + c * inv;
            o.Z = (float)z0 + r * inv;
            o.Y = Terrain->HeightAt(o.X, o.Z) + 0.05f;
            int br = r < n - 1 ? r : r - 1, bc = c < n - 1 ? c : c - 1;
            unsigned b = blockmap[br * bmStride + bc];
            o.C = (b & 0xff) == 0 ? 0x4000ff00u : 0x40ff0000u;
        }
    (void)heights; (void)stride;
    dev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
    dev->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, n * n, NTris, Indices, D3DFMT_INDEX16,
                                v, sizeof(V));
    delete[] v;
}

void SBlockMapParcel::Draw(int p1) { (void)p1; }
void SBlockMapParcel::DrawShadow(int p1) { (void)p1; }
void SBlockMapParcel::Slot_0C() {}
void SBlockMapParcel::Slot_10() {}
void SBlockMapParcel::Slot_14() {}
void SBlockMapParcel::Slot_18() {}
void SBlockMapParcel::Slot_1C() {}
void SBlockMapParcel::Slot_20() {}
void SBlockMapParcel::Slot_24() {}
void SBlockMapParcel::Slot_28() {}
void SBlockMapParcel::CreateVertexBuffer(int p1, int p2) { (void)p1; (void)p2; }
void SBlockMapParcel::Slot_30() {}
void SBlockMapParcel::Slot_34() {}

// ------------------------------------------------------------------------
// SParcel2 (compact terrain)

namespace {

// The ctor's growing SDArrays (HD 0x78b864 realloc: 16, then * 6 / 5).
template <class T> struct SGrowArray {
    T*  Data = nullptr;
    int Count = 0;
    int Cap = 0;
    ~SGrowArray() { free(Data); }
    T& Add()
    {
        if (Count == Cap) {
            int cap = Cap < 0x10 ? 0x10 : (Cap * 6) / 5;
            Data = (T*)realloc(Data, cap * sizeof(T));
            memset(Data + Cap, 0, (cap - Cap) * sizeof(T));
            Cap = cap;
        }
        return Data[Count++];
    }
};

typedef SParcel2::SVertex   SCompactVertex;
typedef SParcel2::SMaterial SCompactMaterial;

// The layer weight of each grid vertex (HD static table 0x956550, 17 floats
// per vertex, filled through the work rows 0x9564c0 (A) / 0x956508 (B)).
float s_LayerWeights[81][17];

// fistp / cvtss2si: round to nearest (DAT_0092e350 = ROUND(x)).
inline int RoundInt(float x)
{
    return (int)lrintf(x);
}

} // namespace

// PANZERS 0x704fd0
// Bakes the parcel (see parcel.h). Steps in HD order:
//  1. per grid vertex the 17 layer weights: A[k] = blend byte k-1 / 255 (0
//     for layers with flag 0x80, 1 above 0.9), B[k] = A's visible part
//     under the layers above; then from the top down a layer whose visible
//     part is below 1e-4 is dropped, and one below 0.1 too, giving its share
//     back to the layers below. The vertex alpha of layer k is A[k].
//  2. the cells under opaque map decals (texture alpha class 0) are masked;
//  3. per layer k = 0..16 and pass 0 (opaque) / 1 (blended): the two
//     triangles of each unmasked cell where layer k is visible (>= 1e-4 at a
//     corner); a triangle's first visible layer draws in pass 0, the others
//     in pass 1; one material per (k, pass) that got triangles;
//  4. the junction meshes (terrain +0x30), then the road meshes (+0x1c) of
//     this parcel, one material each (types 2 / 3);
//  5. the map decals over the parcel: two triangles per covered cell, the UV
//     by the decal rotation (0..7); consecutive decals with the same texture
//     share a material.
// The vertex positions are grid positions (road vertices truncated to the
// grid); heights and normals are read at draw time.
SParcel2::SParcel2(int x0, int z0, const float* heights, const unsigned char* blend,
                   const float* normals, const unsigned* diffuse, int stride, int baseLayer,
                   const int* textures, const int* layerFlags, void* overlays, int parcel,
                   STerrain* terrain)
    : VertexCount(0), Indices(nullptr), IndexCount(0), Vertices(nullptr), Materials(nullptr),
      MaterialCount(0), DebugColor(0), Stride(stride), X0(x0), Z0(z0), Heights(heights),
      Normals(normals), Terrain(terrain)
{
    PZ_TRACE("SParcel2::SParcel2 (0x704fd0)");
    (void)diffuse;   // HD reads the diffuse per frame (UpdateColors 0x70b240)
    for (int i = 0; i < 81; ++i)
        Colors[i] = 0xffffff;

    // 1. Layer weights.
    for (int r = 0; r < 9; ++r) {
        for (int c = 0; c < 9; ++c) {
            const unsigned char* bl = blend + (r * stride + c) * 0x10;
            float A[17], B[17];
            for (int j = baseLayer - 1; j >= 0; --j) {
                A[j] = 0.0f;
                B[j] = 0.0f;
            }
            A[baseLayer] = 1.0f;
            B[baseLayer] = 1.0f;
            for (int k = baseLayer + 1; k < 0x11; ++k) {
                float w = (float)((double)bl[k - 1] * (1.0 / 255.0));   // 0x80ca20
                A[k] = w;
                B[k] = w;
                if (k != 0 && (layerFlags[k - 1] & 0x80) != 0)
                    A[k] = 0.0f;
                if ((double)A[k] > 0.9)                                 // 0x7fe0c0
                    A[k] = 1.0f;
                float keep = 1.0f - A[k];
                for (int j = k - 1; j >= 0; --j)                        // HD: 4 wide, then scalar
                    B[j] = B[j] * keep;
            }
            for (int k = 16; k >= 0; --k) {
                if ((double)B[k] < 0.0001) {                            // 0x7f1b50
                    A[k] = 0.0f;
                } else if ((double)B[k] < 0.1) {                        // 0x7f83e0
                    float keep = 1.0f - A[k];
                    for (int j = k - 1; j >= 0; --j)                    // HD: divps, then divss
                        B[j] = B[j] / keep;
                    A[k] = 0.0f;
                }
            }
            memcpy(s_LayerWeights[r * 9 + c], A, sizeof(A));
        }
    }

    // 2. Cells under opaque map decals (HD 0x957e20).
    const SMapDecal* decals = *(SMapDecal* const*)overlays;            // SDArray {data, count}
    const int decalCount = ((const int*)overlays)[1];
    unsigned char masked[8][8];
    memset(masked, 0, sizeof(masked));
    for (int i = 0; i < decalCount; ++i) {
        const SMapDecal& d = decals[i];
        int dx0 = d.X0 - x0, dz0 = d.Z0 - z0, dx1 = d.X1 - x0, dz1 = d.Z1 - z0;
        if (dx0 < 8 && dx1 > 0 && dz0 < 8 && dz1 > 0 && TerrainTextureAlpha(d.Texture) == 0) {   // 0x67d8a0
            if (dx0 < 0) dx0 = 0;
            if (dx1 > 8) dx1 = 8;
            if (dz0 < 0) dz0 = 0;
            if (dz1 > 8) dz1 = 8;
            for (int z = dz0; z < dz1; ++z)
                for (int x = dx0; x < dx1; ++x)
                    masked[z][x] = 1;
        }
    }

    SGrowArray<SCompactVertex>   verts;
    SGrowArray<unsigned short>   idx;
    SGrowArray<SCompactMaterial> mats;
    auto addMaterial = [&](int texture, int type, int firstIndex, int firstVertex) -> int {
        SCompactMaterial& m = mats.Add();
        m.Texture = TerrainAddRefTexture(texture);   // 0x677f20
        m.Type = type;
        m.PrimCount = (idx.Count - firstIndex) / 3;
        m.MinIndex = firstVertex;
        m.NumVertices = verts.Count - firstVertex;
        m.PrimType = D3DPT_TRIANGLELIST;
        return mats.Count - 1;
    };

    // 3. Layers.
    int firstLayer[64][2];                                // HD 0x957c20: per cell and triangle
    memset(firstLayer, 0xff, sizeof(firstLayer));
    int layersUsed = 0, lastLayer = -1;
    for (int k = 0; k <= 0x10; ++k) {
        for (int pass = 0; pass < 2; ++pass) {
            int cache[81];                                // HD 0x957ad8
            memset(cache, 0xff, sizeof(cache));
            const int groupVertex = verts.Count;
            const int groupIndex = idx.Count;
            auto vertex = [&](int x, int z) {
                int v = z * 9 + x;
                if (cache[v] == -1) {
                    SCompactVertex& o = verts.Add();
                    o.X = (unsigned char)x;
                    o.Z = (unsigned char)z;
                    o.U = (float)((double)x * 0.125);         // 0x7fe0b0
                    o.V = (float)((double)z * 0.125);
                    o.Alpha = (unsigned char)RoundInt(s_LayerWeights[v][k] * 255.0f);   // 0x7fb6c8
                    cache[v] = verts.Count - 1 - groupVertex;
                }
                idx.Add() = (unsigned short)(cache[v] + groupVertex);
            };
            auto visible = [&](int x, int z) { return 0.0001 <= (double)s_LayerWeights[z * 9 + x][k]; };
            for (int z = 0; z < 8; ++z) {
                for (int x = 0; x < 8; ++x) {
                    if (masked[z][x])
                        continue;
                    int* first = firstLayer[z * 8 + x];
                    if (visible(x, z) || visible(x + 1, z) || visible(x, z + 1)) {
                        if (first[0] == -1)
                            first[0] = k;
                        if ((first[0] == k) == (pass == 0)) {
                            vertex(x, z);
                            vertex(x + 1, z);
                            vertex(x, z + 1);
                        }
                    }
                    if (visible(x + 1, z) || visible(x, z + 1) || visible(x + 1, z + 1)) {
                        if (first[1] == -1)
                            first[1] = k;
                        if ((first[1] == k) == (pass == 0)) {
                            vertex(x, z + 1);
                            vertex(x + 1, z);
                            vertex(x + 1, z + 1);
                        }
                    }
                }
            }
            if (idx.Count != groupIndex) {
                addMaterial(textures[k], pass, groupIndex, groupVertex);
                if (lastLayer != k) {
                    ++layersUsed;
                    lastLayer = k;
                }
            }
        }
    }
    if (TerrainOption(0x11) != 0 && layersUsed >= 4)
        DebugColor = layersUsed < 6 ? (layersUsed > 4 ? 0xff00ffu : 0xff00u) : 0xff0000u;

    // 4. Junctions (+0x30, type 2), then roads (+0x1c, type 3).
    auto addMeshes = [&](const SHdHeap<SRoadMesh>& meshes, int texture, int type) {
        for (int i = 0; i < meshes.Size; ++i) {
            const SRoadMesh& m = meshes.Data[i];
            if (m.Use != kHeapLive || m.Parcel != parcel)
                continue;
            const int groupVertex = verts.Count;
            const int groupIndex = idx.Count;
            for (int v = 0; v < m.NVerts; ++v) {
                SCompactVertex& o = verts.Add();
                o.X = (unsigned char)(int)(m.Verts[v].X - (float)x0);
                o.Z = (unsigned char)(int)(m.Verts[v].Z - (float)z0);
                o.U = m.Verts[v].U;
                o.V = m.Verts[v].V;
                o.Alpha = 0xff;
            }
            for (int n = 0; n < m.NIndices; ++n)
                idx.Add() = (unsigned short)(m.Indices[n] + groupVertex);
            addMaterial(texture, type, groupIndex, groupVertex);
        }
    };
    for (int j = 0; j < terrain->Junctions.Size; ++j) {
        const SRoadJunction& jn = terrain->Junctions.Data[j];
        if (jn.Use == kHeapLive)
            addMeshes(jn.Meshes, jn.Texture, 2);
    }
    for (int j = 0; j < terrain->Roads.Size; ++j) {
        const SRoad& rd = terrain->Roads.Data[j];
        if (rd.Use == kHeapLive)
            addMeshes(rd.Meshes, rd.Texture, 3);
    }

    // 5. Map decals.
    int lastTexture = -1, lastMaterial = -1;
    for (int i = 0; i < decalCount; ++i) {
        const SMapDecal& d = decals[i];
        int cx0 = d.X0 - x0, cx1 = d.X1 - x0, cz0 = d.Z0 - z0, cz1 = d.Z1 - z0;
        if (cx0 > 7 || cx1 < 1 || cz0 > 7 || cz1 < 1)
            continue;
        if (cx0 < 0) cx0 = 0;
        if (cx1 > 8) cx1 = 8;
        if (cz0 < 0) cz0 = 0;
        if (cz1 > 8) cz1 = 8;
        int cache[81];
        memset(cache, 0xff, sizeof(cache));
        // u = dx * ua + ub + dz * uc, v = dx * vd + ve + dz * vf (dx, dz from
        // the decal corner, in tiles; 0x7eed98 = 1, 0x7ea780 = -1).
        float ua = 0.0f, ub = 0.0f, uc = 0.0f, vd = 0.0f, ve = 0.0f, vf = 0.0f;
        const int w = d.X1 - d.X0, h = d.Z1 - d.Z0;
        switch (d.Rotation) {
        case 0: ub = 0.0f; ve = 1.0f; ua = (float)(1.0 / w);  vf = (float)(-1.0 / h); break;
        case 1: ve = 0.0f; ub = 0.0f; vd = (float)(1.0 / w);  uc = (float)(1.0 / h);  break;
        case 2: ub = 1.0f; ve = 0.0f; ua = (float)(-1.0 / w); vf = (float)(1.0 / h);  break;
        case 3: ve = 1.0f; ub = 1.0f; vd = (float)(-1.0 / w); uc = (float)(-1.0 / h); break;
        case 4: ub = 1.0f; ve = 1.0f; ua = (float)(-1.0 / w); vf = (float)(-1.0 / h); break;
        case 5: ve = 0.0f; ub = 1.0f; vd = (float)(1.0 / w);  uc = (float)(-1.0 / h); break;
        case 6: ub = 0.0f; ve = 0.0f; ua = (float)(1.0 / w);  vf = (float)(1.0 / h);  break;
        case 7: ve = 1.0f; ub = 0.0f; vd = (float)(-1.0 / w); uc = (float)(1.0 / h);  break;
        default: break;
        }
        const int groupVertex = verts.Count;
        const int groupIndex = idx.Count;
        auto vertex = [&](int x, int z) {
            int v = z * 9 + x;
            if (cache[v] == -1) {
                SCompactVertex& o = verts.Add();
                o.X = (unsigned char)x;
                o.Z = (unsigned char)z;
                float fx = (float)(x - d.X0 + x0), fz = (float)(z - d.Z0 + z0);
                o.U = fx * ua + ub + fz * uc;
                o.V = fx * vd + ve + fz * vf;
                o.Alpha = 0xff;
                cache[v] = verts.Count - 1 - groupVertex;
            }
            idx.Add() = (unsigned short)(cache[v] + groupVertex);
        };
        for (int z = cz0; z < cz1; ++z)
            for (int x = cx0; x < cx1; ++x) {
                vertex(x, z);
                vertex(x + 1, z);
                vertex(x, z + 1);
                vertex(x, z + 1);
                vertex(x + 1, z);
                vertex(x + 1, z + 1);
            }
        if (lastTexture == d.Texture && lastMaterial >= 0) {
            mats.Data[lastMaterial].PrimCount += (idx.Count - groupIndex) / 3;
            mats.Data[lastMaterial].NumVertices += verts.Count - groupVertex;
        } else {
            lastMaterial = addMaterial(d.Texture, 2, groupIndex, groupVertex);
            lastTexture = d.Texture;
        }
    }

    if (verts.Count < 1 || idx.Count < 1 || mats.Count < 1) {
        // HD panics here (SDArray operator[] on an empty array).
        Logger.g->Log(1, "SParcel2::SParcel2: parcel %d is empty", parcel);
    }
    VertexCount = verts.Count;
    Vertices = (SCompactVertex*)malloc((verts.Count ? verts.Count : 1) * sizeof(SCompactVertex));
    memcpy(Vertices, verts.Data, verts.Count * sizeof(SCompactVertex));
    IndexCount = idx.Count;                                   // SMesh::CreateIndexBuffer 0x6cd070
    Indices = (unsigned short*)malloc((idx.Count ? idx.Count : 1) * sizeof(unsigned short));
    memcpy(Indices, idx.Data, idx.Count * sizeof(unsigned short));
    MaterialCount = mats.Count;
    Materials = (SCompactMaterial*)malloc((mats.Count ? mats.Count : 1) * sizeof(SCompactMaterial));
    memcpy(Materials, mats.Data, mats.Count * sizeof(SCompactMaterial));
}

// PANZERS 0x708830
SParcel2::~SParcel2()
{
    for (int i = 0; i < MaterialCount; ++i)
        TerrainReleaseTexture(Materials[i].Texture);   // Gepard +0x48
    free(Materials);
    Materials = nullptr;
    free(Vertices);
    Vertices = nullptr;
    free(Indices);                                     // SMesh dtor 0x6ccdc0: the index buffer
    Indices = nullptr;
}

// PANZERS 0x70b240
void SParcel2::UpdateColors(const unsigned* diffuse, const unsigned char* overlay, int stride, int mode)
{
    if (DebugColor != 0) {
        for (int i = 0; i < 81; ++i)
            Colors[i] = DebugColor;
        return;
    }
    if (mode == 0) {
        for (int r = 0; r < 9; ++r)
            for (int c = 0; c < 9; ++c)
                Colors[r * 9 + c] = diffuse[r * stride + c] & 0xffffff;
        return;
    }
    if (mode == 1) {
        for (int r = 0; r < 9; ++r)
            for (int c = 0; c < 9; ++c) {
                const unsigned char* b = (const unsigned char*)&diffuse[r * stride + c];
                Colors[r * 9 + c] = ((unsigned)b[2] * 0xa000 & 0xffff00ff) |
                                    (((unsigned)b[1] * 5 & 0x7fffff8) << 5) |
                                    ((unsigned)b[0] * 0x50 >> 7);
            }
        return;
    }
    if (mode == 2 || mode == 3) {
        // The overlay has two cells per tile (row pitch 2 * stride); the
        // pointer is the cell above-left of the parcel's first vertex.
        for (int r = 0; r < 9; ++r) {
            const unsigned char* R0 = overlay + r * stride * 4;
            const unsigned char* R1 = R0 + stride * 2;
            const unsigned char* R2 = R0 + stride * 4;
            const unsigned char* Rm = R0 - stride * 2;
            for (int c = 0; c < 9; ++c) {
                const int k = c * 2;
                int centre = (R1[k + 1] & 1) + (R1[k] & 1) + (R0[k] & 1) + (R0[k + 1] & 1);
                int around = (Rm[k + 1] & 1) + (R0[k + 2] & 1) + (R0[k - 1] & 1) + (R2[k] & 1) +
                             (Rm[k] & 1) + (R1[k + 2] & 1) + (R1[k - 1] & 1) + (R2[k + 2] & 1);
                const unsigned char* b = (const unsigned char*)&diffuse[r * stride + c];
                unsigned R = b[2], G = b[1], B = b[0];
                if (mode == 2) {
                    unsigned f = (unsigned)((centre + 10) * 4 + around) * 2;
                    Colors[r * 9 + c] = ((((R * f) & 0xffffff80) << 8 | ((G * f) & 0xffffff80)) * 2) |
                                        ((B * f) >> 7);
                } else {
                    unsigned f = (unsigned)(0x80 - ((int)((centre * 4 + around) * 3) >> 1));
                    Colors[r * 9 + c] = (((f * R) & 0xffffff80) * 2 | G) << 8 | B;
                }
            }
        }
        return;
    }
    for (int i = 0; i < 81; ++i)
        Colors[i] = 0xffffff;
}

// PANZERS 0x7089e0
// The parcel's vertices go to the Gepard dynamic VB (0x67f150 / 0x681440;
// here a user-pointer draw); per material the texture (0x680c90), the
// address mode and the blend / alpha test / z-write states, then
// DrawIndexedPrimitive (0x67a650). Types: 0 opaque layer, 1 blended layer
// (SRCALPHA, alpha > 4), 2 junction / decal (clamped), 3 road (U wrapped,
// V clamped); 2 and 3 take the alpha class of the texture (1 test > 0x80,
// 2 blend) and the texture's alpha.
void SParcel2::DrawCompact(bool shadowPass)
{
    PZ_TRACE("SParcel2::DrawCompact (0x7089e0)");
    IDirect3DDevice9* dev = TerrainDevice();
    if (!dev || VertexCount <= 0 || !Indices || !Materials)
        return;
    STerrainVertex* v = (STerrainVertex*)malloc(VertexCount * sizeof(STerrainVertex));
    for (int i = 0; i < VertexCount; ++i) {
        const SVertex& s = Vertices[i];
        int g = s.Z * Stride + s.X;
        STerrainVertex& o = v[i];
        o.X = (float)(int)(X0 + s.X);
        o.Y = Heights[g];
        o.Z = (float)(int)(s.Z + Z0);
        o.NX = Normals[g * 3 + 0];
        o.NY = Normals[g * 3 + 1];
        o.NZ = Normals[g * 3 + 2];
        o.Color = ((unsigned)s.Alpha << 24) + Colors[s.Z * 9 + s.X];
        o.U = s.U;
        o.V = s.V;
    }
    if (!shadowPass && TerrainOption(0x10) != 0)
        dev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE2X);
    if (!g_TerrainDrawKeepShaders) {
        dev->SetVertexShader(nullptr);
        dev->SetPixelShader(nullptr);
    }
    dev->SetFVF(kTerrainFVF);
    int start = 0;
    for (int i = 0; i < MaterialCount; ++i) {
        const SMaterial& m = Materials[i];
        bool states = true;
        DWORD alphaOp = D3DTOP_SELECTARG2;
        switch (m.Type) {
        case 0:
            GepardSetTexture(0, m.Texture);
            dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
            dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
            dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
            dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
            dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
            break;
        case 1:
            GepardSetTexture(0, m.Texture);
            dev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
            dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
            dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
            dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            dev->SetRenderState(D3DRS_ALPHAREF, 4);
            dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
            dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            break;
        case 2:
        case 3: {
            GepardSetTexture(0, m.Texture);
            dev->SetSamplerState(0, D3DSAMP_ADDRESSU, m.Type == 2 ? D3DTADDRESS_CLAMP : D3DTADDRESS_WRAP);
            dev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
            int alpha = TerrainTextureAlpha(m.Texture);   // 0x67c9e0
            if (alpha == 1) {
                dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
                dev->SetRenderState(D3DRS_ALPHAREF, 0x80);
                dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
                dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
            } else if (alpha == 2) {
                dev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
                dev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
                dev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
                dev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
                dev->SetRenderState(D3DRS_ALPHAREF, 4);
                dev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATER);
                dev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            } else {
                dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
                dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
                dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
            }
            alphaOp = D3DTOP_SELECTARG1;
            break;
        }
        default:
            states = false;   // HD sets nothing and still draws
            break;
        }
        if (states)
            dev->SetTextureStageState(0, D3DTSS_ALPHAOP, alphaOp);
        if (m.PrimCount > 0)
            dev->DrawIndexedPrimitiveUP((D3DPRIMITIVETYPE)m.PrimType, m.MinIndex, m.NumVertices,
                                        m.PrimCount, Indices + start, D3DFMT_INDEX16, v,
                                        sizeof(STerrainVertex));
        start += m.PrimType == D3DPT_TRIANGLELIST ? m.PrimCount * 3 : m.PrimCount + 2;
    }
    free(v);
}

void SParcel2::Draw(int p1) { (void)p1; }
void SParcel2::DrawShadow(int p1) { (void)p1; }
void SParcel2::Slot_0C() {}
void SParcel2::Slot_10() {}
void SParcel2::Slot_14() {}
void SParcel2::Slot_18() {}
void SParcel2::Slot_1C() {}
void SParcel2::Slot_20() {}
void SParcel2::Slot_24() {}
void SParcel2::Slot_28() {}
void SParcel2::CreateVertexBuffer(int p1, int p2) { (void)p1; (void)p2; }
void SParcel2::Slot_30() {}
void SParcel2::Slot_34() {}

} // namespace pz
