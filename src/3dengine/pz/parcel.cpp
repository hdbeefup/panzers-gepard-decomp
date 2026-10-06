// src/3dengine/pz/parcel.cpp
// Terrain parcels and the fixed-function draw state of the HD terrain
// passes. OWNER: agent B. See parcel.h.

#include <math.h>
#include <string.h>
#include "parcel.h"
#include "pzterrain.h"
#include "igepardhd.h"
#include "gepard.h"
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
// SParcel2 (compact mode only)

SParcel2::SParcel2(int x0, int z0, const float* heights, const unsigned char* blend,
                   const float* normals, const unsigned* diffuse, int stride, int baseLayer,
                   const int* textures, const int* layerFlags, void* overlays, int parcel,
                   STerrain* terrain)
    : X0(x0), Z0(z0), Stride(stride), BaseLayer(baseLayer), Parcel(parcel), Heights(heights),
      BlendMap(blend), Normals(normals), Diffuse(diffuse), Textures(textures), Terrain(terrain)
{
    STUB_LOG("SParcel2::SParcel2 (0x704fd0)");
    PZ_TRACE("SParcel2::SParcel2 (0x704fd0)");
    (void)layerFlags; (void)overlays;
}

SParcel2::~SParcel2()
{
}

void SParcel2::DrawCompact(bool shadowPass)
{
    STUB_LOG("SParcel2::DrawCompact (0x7089e0)");
    PZ_TRACE("SParcel2::DrawCompact (0x7089e0)");
    // Compact mode is a debug toggle; draw the parcel the layered way.
    if (Terrain && Terrain->ParcelRenderer && Parcel >= 0 && Parcel < Terrain->ParcelCount)
        Terrain->ParcelRenderer->DrawLayered(X0, Z0, Heights, Diffuse, BlendMap, Normals, Stride,
                                             BaseLayer, Textures, Terrain->Parcels[Parcel].Layers,
                                             shadowPass);
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
