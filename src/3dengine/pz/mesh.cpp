// src/3dengine/pz/mesh.cpp
// HD SMesh / SAnimesh / SSkinnedMesh, SMaterial and SRenderPass.
// OWNER: agent A. See mesh.h.

#include <d3d9.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "mesh.h"
#include "pzgepard.h"
#include "pzscene.h"
#include "shadowmath.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

SMeshDrawGlobals g_MeshDraw;

static inline DWORD F2DW(float f) { DWORD d; memcpy(&d, &f, 4); return d; }

// ---------------------------------------------------------------------------
// The last applied pass (HD globals 0x92f0a8..0x92f0ec, written through the
// fog setter 0x688ac0 with ECX = 0x92f0a8).
// ---------------------------------------------------------------------------
struct SPassCache {
    int      VertexShader;   // 0x92f0a8
    bool     Lighting;       // 0x92f0ac
    bool     MaterialDirty;  // 0x92f0ad
    bool     SourceDirty;    // 0x92f0ae
    unsigned TransformMask;  // 0x92f0b0
    int      ShadeMode;      // 0x92f0b4
    int      CullMode;       // 0x92f0b8
    unsigned SamplerMask;    // 0x92f0bc
    int      PixelShader;    // 0x92f0c0
    int      TextureFactor;  // 0x92f0c4
    unsigned StageMask;      // 0x92f0c8
    unsigned char ColorWrite;// 0x92f0cc
    bool     AlphaBlend;     // 0x92f0cd
    bool     AlphaTest;      // 0x92f0ce
    bool     ZEnable;        // 0x92f0cf
    int      ZFunc;          // 0x92f0d0
    bool     ZWrite;         // 0x92f0d4
    bool     Stencil;        // 0x92f0d5
    int      FogMode;        // 0x92f0d8
    unsigned FogTint;        // 0x92f0dc
    unsigned FogColor;       // 0x92f0e0
    float    FogStart;       // 0x92f0e4
    float    FogEnd;         // 0x92f0e8
    bool     FogEnabled;     // 0x92f0ec
    float    FogStart2, FogEnd2, FogInvRange, FogInvRange2;   // 0x92f0f0..
};
static SPassCache s_Cache;

void InvalidateRenderPassCache()
{
    // Values no pass produces, so the next Apply sets every state; all stage
    // masks set, so every stage is rewritten.
    float fs = s_Cache.FogStart, fe = s_Cache.FogEnd;
    unsigned fc = s_Cache.FogColor;
    bool fen = s_Cache.FogEnabled;
    memset(&s_Cache, 0, sizeof(s_Cache));
    s_Cache.VertexShader = -1;
    s_Cache.Lighting = true;
    s_Cache.MaterialDirty = true;
    s_Cache.SourceDirty = true;
    s_Cache.TransformMask = 0x1f;
    s_Cache.ShadeMode = -1;
    s_Cache.CullMode = -1;
    s_Cache.SamplerMask = 0x1f;
    s_Cache.PixelShader = -1;
    s_Cache.TextureFactor = 0x12345678;
    s_Cache.StageMask = 0x1f;
    s_Cache.ColorWrite = 0xee;
    s_Cache.AlphaBlend = true;
    s_Cache.AlphaTest = true;
    s_Cache.ZEnable = false;
    s_Cache.ZFunc = -1;
    s_Cache.ZWrite = false;
    s_Cache.Stencil = true;
    s_Cache.FogMode = -1;
    s_Cache.FogStart = fs;
    s_Cache.FogEnd = fe;
    s_Cache.FogColor = fc;
    s_Cache.FogEnabled = fen;
}

bool GetPassFog(float* start, float* end, float* start2, float* end2, float* inv)
{
    *start = s_Cache.FogStart;
    *end = s_Cache.FogEnd;
    *start2 = s_Cache.FogStart2;
    *end2 = s_Cache.FogEnd2;
    *inv = s_Cache.FogInvRange2;
    return s_Cache.FogEnabled;
}

// PANZERS 0x688ac0
void SetPassFog(float start, float end, unsigned color)
{
    IDirect3DDevice9* dev = HD().Device;
    s_Cache.FogColor = color;
    s_Cache.FogMode = 2;
    s_Cache.FogTint = 0;
    s_Cache.FogStart2 = start * start;
    s_Cache.FogEnd = end;
    s_Cache.FogStart = start;
    s_Cache.FogEnd2 = end * end;
    s_Cache.FogEnabled = start < end;
    s_Cache.FogInvRange = 1.0f / (end - start);
    s_Cache.FogInvRange2 = (float)(1.0 / (double)(end - start));   // HD: constant 0x878780 / (end - start)
    if (dev)
        dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
}

// ---------------------------------------------------------------------------
// SRenderPass
// ---------------------------------------------------------------------------

// PANZERS 0x687730
void SRenderPass::Init()
{
    for (int i = 0; i < 5; ++i) {
        memset(Transforms[i].Matrix, 0, sizeof(Transforms[i].Matrix));
        Transforms[i].Matrix[0] = 1.0f;
        Transforms[i].Matrix[5] = 1.0f;
        Transforms[i].Matrix[10] = 1.0f;
        Transforms[i].Matrix[15] = 1.0f;
    }
    Reset();
}

// PANZERS 0x6883a0
void SRenderPass::Reset()
{
    VertexShader = 0;
    Lighting = true;
    memset(Material, 0, sizeof(Material));
    for (int i = 0; i < 8; ++i)   // Diffuse and Ambient = 1
        Material[i] = 1.0f;
    MaterialDirty = false;
    AmbientSource = 1;
    DiffuseSource = 1;
    SpecularSource = 2;
    EmissiveSource = 0;
    SourceDirty = false;
    for (int i = 0; i < 5; ++i) {
        Transforms[i].TexCoordIndex = i;
        Transforms[i].TransformFlags = 0;
    }
    TransformMask = 0;
    ShadeMode = D3DSHADE_GOURAUD;
    CullMode = D3DCULL_CW;
    for (int i = 0; i < 5; ++i) {
        Samplers[i].Texture = -1;
        Samplers[i].AddressU = D3DTADDRESS_WRAP;
        Samplers[i].AddressV = D3DTADDRESS_WRAP;
    }
    SamplerMask = 0;
    PixelShader = 0;
    TextureFactor = -1;
    Stages[0].ColorOp = D3DTOP_MODULATE;
    Stages[0].ColorArg1 = D3DTA_TEXTURE;
    Stages[0].ColorArg2 = D3DTA_CURRENT;
    Stages[0].ColorArg0 = D3DTA_CURRENT;
    Stages[0].AlphaOp = D3DTOP_SELECTARG1;
    Stages[0].AlphaArg1 = D3DTA_TEXTURE;
    Stages[0].AlphaArg2 = D3DTA_CURRENT;
    Stages[0].ResultTemp = false;
    for (int i = 1; i < 5; ++i) {
        Stages[i].ColorOp = D3DTOP_DISABLE;
        Stages[i].ColorArg1 = D3DTA_TEXTURE;
        Stages[i].ColorArg2 = D3DTA_CURRENT;
        Stages[i].ColorArg0 = D3DTA_CURRENT;
        Stages[i].AlphaOp = D3DTOP_DISABLE;
        Stages[i].AlphaArg1 = D3DTA_TEXTURE;
        Stages[i].AlphaArg2 = D3DTA_CURRENT;
        Stages[i].ResultTemp = false;
    }
    StageMask = 0;
    ColorWrite = 0x0f;
    AlphaBlend = false;
    SrcBlend = D3DBLEND_ONE;
    DestBlend = D3DBLEND_ZERO;
    AlphaTest = false;
    AlphaRef = 0;
    AlphaFunc = D3DCMP_ALWAYS;
    ZEnable = true;
    ZFunc = D3DCMP_LESSEQUAL;
    ZWrite = true;
    Stencil = false;
    StencilState[0] = 1;
    StencilState[1] = 1;
    StencilState[2] = 1;
    StencilState[3] = 8;
    StencilState[4] = 0;
    StencilState[5] = -1;
    StencilState[6] = -1;
    FogMode = 0;
    ++HD().PassCount;   // SGepard +0x4b4
}

// PANZERS 0x688d80
void SRenderPass::SetTexture(unsigned stage, int texture, bool wrap)
{
    Samplers[stage].Texture = texture;
    int a = wrap ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
    Samplers[stage].AddressV = a;
    Samplers[stage].AddressU = a;
    SamplerMask |= 1u << (stage & 31);
}

// PANZERS 0x688c40
void SRenderPass::SetTexCoordIndex(unsigned stage, int index)
{
    Transforms[stage].TransformFlags = 0;
    Transforms[stage].TexCoordIndex = index;
    TransformMask |= 1u << (stage & 31);
}

// PANZERS 0x688c70
void SRenderPass::SetTexTransform(unsigned stage, int index, int flags, const float* m)
{
    Transforms[stage].TexCoordIndex = index;
    Transforms[stage].TransformFlags = flags;
    memcpy(Transforms[stage].Matrix, m, 64);
    TransformMask |= 1u << (stage & 31);
}

// PANZERS 0x688cd0
void SRenderPass::SetTexMatrix(unsigned stage, int flags, const float* m)
{
    Transforms[stage].TransformFlags = flags;
    memcpy(Transforms[stage].Matrix, m, 64);
    TransformMask |= 1u << (stage & 31);
}

// PANZERS 0x6888f0
void SRenderPass::SetColorOp(unsigned stage, int op, int arg1, int arg2, int arg0)
{
    Stages[stage].ColorOp = op;
    Stages[stage].ColorArg1 = arg1;
    Stages[stage].ColorArg2 = arg2;
    Stages[stage].ColorArg0 = arg0;
    StageMask |= 1u << (stage & 31);
}

// PANZERS 0x688870
void SRenderPass::SetAlphaOp(unsigned stage, int op, int arg1, int arg2)
{
    Stages[stage].AlphaOp = op;
    Stages[stage].AlphaArg1 = arg1;
    Stages[stage].AlphaArg2 = arg2;
    StageMask |= 1u << (stage & 31);
}

// PANZERS 0x6887d0
// 2 = alpha blend (SRCALPHA, INVSRCALPHA, alpha test > 0, no z write),
// 1 = alpha test (ref 0x80), else opaque.
void SRenderPass::SetAlphaMode(int mode)
{
    if (mode == 2) {
        AlphaBlend = true;
        SrcBlend = D3DBLEND_SRCALPHA;
        DestBlend = D3DBLEND_INVSRCALPHA;
        AlphaTest = true;
        AlphaRef = 4;   // HD writes the word 0x401: test on, ref 4
        AlphaFunc = D3DCMP_GREATEREQUAL;
        ZWrite = false;
        return;
    }
    AlphaBlend = false;
    SrcBlend = D3DBLEND_ONE;
    DestBlend = D3DBLEND_ZERO;
    ZWrite = true;
    if (mode == 1) {
        AlphaTest = true;   // word 0x8001: test on, ref 0x80
        AlphaRef = 0x80;
        AlphaFunc = D3DCMP_GREATEREQUAL;
        return;
    }
    AlphaTest = false;
    AlphaRef = 0;
    AlphaFunc = D3DCMP_ALWAYS;
}

// PANZERS 0x688980
// 1 additive (ONE, ONE, black fog), 2 multiply (ZERO, SRCCOLOR),
// 3 (ONE, INVSRCALPHA... dest 3 = SRCCOLOR), 4 (DESTCOLOR 9, SRCCOLOR 3);
// anything else takes the alpha mode of the texture.
void SRenderPass::SetBlendMode(int mode, int texture)
{
    if (mode == 1) {
        AlphaBlend = true;
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
        DestBlend = D3DBLEND_SRCCOLOR;
    } else if (mode == 3) {
        SrcBlend = D3DBLEND_ZERO;
        DestBlend = D3DBLEND_SRCCOLOR;
    } else if (mode == 4) {
        SrcBlend = D3DBLEND_DESTCOLOR;
        DestBlend = D3DBLEND_SRCCOLOR;
    } else {
        SetAlphaMode(GepardGetTextureAlpha(texture));
        return;
    }
    if (mode == 3)
        SrcBlend = D3DBLEND_ZERO;
    AlphaBlend = true;
    AlphaTest = false;
    AlphaRef = 0;
    AlphaFunc = D3DCMP_ALWAYS;
    ZWrite = false;
}

// PANZERS 0x688aa0
void SRenderPass::SetFogMode(int mode, unsigned color)
{
    FogMode = mode;
    FogColor = color;
}

// PANZERS 0x687a50
void SRenderPass::Apply()
{
    SGepardHDState& hd = HD();
    IDirect3DDevice9* dev = hd.Device;
    if (!dev)
        return;
    // (HD starts an STimer period here, 0x661950, for its profile overlay.)
    if (s_Cache.VertexShader != VertexShader) {
        GepardSetVertexShader(VertexShader);
        s_Cache.VertexShader = VertexShader;
    }
    if (VertexShader == 0) {
        if (s_Cache.Lighting != Lighting) {
            dev->SetRenderState(D3DRS_LIGHTING, Lighting);
            s_Cache.Lighting = Lighting;
        }
        if (Lighting) {
            if (s_Cache.MaterialDirty || MaterialDirty) {
                dev->SetMaterial((const D3DMATERIAL9*)Material);
                s_Cache.MaterialDirty = MaterialDirty;
            }
            if (s_Cache.SourceDirty || SourceDirty) {
                dev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, AmbientSource);
                dev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, DiffuseSource);
                dev->SetRenderState(D3DRS_SPECULARMATERIALSOURCE, SpecularSource);
                dev->SetRenderState(D3DRS_EMISSIVEMATERIALSOURCE, EmissiveSource);
                s_Cache.SourceDirty = SourceDirty;
            }
        }
        unsigned mask = TransformMask | s_Cache.TransformMask;
        for (unsigned i = 0; i < hd.TransformStages; ++i, mask >>= 1) {
            if (!(mask & 1))
                continue;
            dev->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, Transforms[i].TexCoordIndex);
            dev->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, Transforms[i].TransformFlags);
            if ((char)Transforms[i].TransformFlags)
                dev->SetTransform((D3DTRANSFORMSTATETYPE)(D3DTS_TEXTURE0 + i),
                                  (const D3DMATRIX*)Transforms[i].Matrix);
        }
        s_Cache.TransformMask = TransformMask;
    } else {
        unsigned mask = s_Cache.TransformMask;
        for (unsigned i = 0; i < hd.TransformStages; ++i, mask >>= 1) {
            if (!(mask & 1))
                continue;
            dev->SetTextureStageState(i, D3DTSS_TEXCOORDINDEX, i);
            dev->SetTextureStageState(i, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
        }
        s_Cache.TransformMask = 0;
    }
    if (s_Cache.ShadeMode != ShadeMode) {
        dev->SetRenderState(D3DRS_SHADEMODE, ShadeMode);
        s_Cache.ShadeMode = ShadeMode;
    }
    if (s_Cache.CullMode != CullMode) {
        dev->SetRenderState(D3DRS_CULLMODE, CullMode);
        s_Cache.CullMode = CullMode;
    }
    unsigned smask = SamplerMask | s_Cache.SamplerMask;
    for (unsigned i = 0; i < hd.SamplerStages; ++i, smask >>= 1) {
        if (!(smask & 1))
            continue;
        if (Samplers[i].Texture < 0) {
            GepardSetTexture(i, -1);
        } else {
            GepardSetTexture(i, Samplers[i].Texture);
            dev->SetSamplerState(i, D3DSAMP_ADDRESSU, Samplers[i].AddressU);
            dev->SetSamplerState(i, D3DSAMP_ADDRESSV, Samplers[i].AddressV);
        }
    }
    s_Cache.SamplerMask = SamplerMask;
    if (s_Cache.PixelShader != PixelShader) {
        GepardSetPixelShader(PixelShader);
        s_Cache.PixelShader = PixelShader;
    }
    if (PixelShader == 0) {
        if (s_Cache.TextureFactor != TextureFactor) {
            dev->SetRenderState(D3DRS_TEXTUREFACTOR, TextureFactor);
            s_Cache.TextureFactor = TextureFactor;
        }
        unsigned tmask = StageMask | s_Cache.StageMask;
        for (unsigned i = 0; i < hd.BlendStages; ++i, tmask >>= 1) {
            if (!(tmask & 1))
                continue;
            const Stage& st = Stages[i];
            dev->SetTextureStageState(i, D3DTSS_COLOROP, st.ColorOp);
            if (st.ColorOp != D3DTOP_DISABLE) {
                dev->SetTextureStageState(i, D3DTSS_COLORARG1, st.ColorArg1);
                dev->SetTextureStageState(i, D3DTSS_COLORARG2, st.ColorArg2);
                if (st.ColorOp == D3DTOP_MULTIPLYADD)
                    dev->SetTextureStageState(i, D3DTSS_COLORARG0, st.ColorArg0);
                dev->SetTextureStageState(i, D3DTSS_RESULTARG, st.ResultTemp ? D3DTA_TEMP : D3DTA_CURRENT);
            }
            dev->SetTextureStageState(i, D3DTSS_ALPHAOP, st.AlphaOp);
            if (st.AlphaOp != D3DTOP_DISABLE) {
                dev->SetTextureStageState(i, D3DTSS_ALPHAARG1, st.AlphaArg1);
                dev->SetTextureStageState(i, D3DTSS_ALPHAARG2, st.AlphaArg2);
            }
        }
        s_Cache.StageMask = StageMask;
    }
    if (s_Cache.ColorWrite != ColorWrite) {
        dev->SetRenderState(D3DRS_COLORWRITEENABLE, ColorWrite);
        s_Cache.ColorWrite = ColorWrite;
    }
    if (ColorWrite) {
        if (s_Cache.AlphaBlend != AlphaBlend) {
            dev->SetRenderState(D3DRS_ALPHABLENDENABLE, AlphaBlend);
            s_Cache.AlphaBlend = AlphaBlend;
        }
        if (AlphaBlend) {
            dev->SetRenderState(D3DRS_SRCBLEND, SrcBlend);
            dev->SetRenderState(D3DRS_DESTBLEND, DestBlend);
        }
        if (s_Cache.AlphaTest != AlphaTest) {
            dev->SetRenderState(D3DRS_ALPHATESTENABLE, AlphaTest);
            s_Cache.AlphaTest = AlphaTest;
        }
        if (AlphaTest) {
            dev->SetRenderState(D3DRS_ALPHAREF, AlphaRef);
            dev->SetRenderState(D3DRS_ALPHAFUNC, AlphaFunc);
        }
    }
    if (s_Cache.ZEnable != ZEnable) {
        dev->SetRenderState(D3DRS_ZENABLE, ZEnable);
        s_Cache.ZEnable = ZEnable;
    }
    if (ZEnable) {
        if (s_Cache.ZFunc != ZFunc) {
            dev->SetRenderState(D3DRS_ZFUNC, ZFunc);
            s_Cache.ZFunc = ZFunc;
        }
        if (s_Cache.ZWrite != ZWrite) {
            dev->SetRenderState(D3DRS_ZWRITEENABLE, ZWrite);
            s_Cache.ZWrite = ZWrite;
        }
    }
    if (s_Cache.Stencil != Stencil) {
        dev->SetRenderState(D3DRS_STENCILENABLE, Stencil);
        s_Cache.Stencil = Stencil;
    }
    if (Stencil) {
        for (int i = 0; i < 7; ++i)
            dev->SetRenderState((D3DRENDERSTATETYPE)(D3DRS_STENCILFAIL + i), StencilState[i]);
    }
    if (s_Cache.FogMode != FogMode || (FogMode == 3 && s_Cache.FogTint != FogColor)) {
        switch (FogMode) {
        case 0:
        case 1:
            if (s_Cache.FogEnabled) {
                dev->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);
                dev->SetRenderState(D3DRS_FOGCOLOR, FogMode == 0 ? s_Cache.FogColor : 0);
                dev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
                dev->SetRenderState(D3DRS_FOGSTART, F2DW(s_Cache.FogStart));
                dev->SetRenderState(D3DRS_FOGEND, F2DW(s_Cache.FogEnd));
                dev->SetRenderState(D3DRS_FOGENABLE, TRUE);
            } else {
                dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
            }
            break;
        case 2:
            dev->SetRenderState(D3DRS_FOGENABLE, FALSE);
            break;
        case 3: {
            unsigned a = FogColor >> 24;
            dev->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);
            dev->SetRenderState(D3DRS_FOGCOLOR, FogColor);
            dev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
            dev->SetRenderState(D3DRS_FOGSTART, F2DW((float)(int)(a * -10)));
            dev->SetRenderState(D3DRS_FOGEND, F2DW((float)(int)((0xff - a) * 10)));
            dev->SetRenderState(D3DRS_FOGENABLE, TRUE);
            s_Cache.FogTint = FogColor;
            break;
        }
        }
        s_Cache.FogMode = FogMode;
    }
    // (HD ends the STimer period and adds it to SGepard +0x4b8.)
}

// ---------------------------------------------------------------------------
// SMaterial
// ---------------------------------------------------------------------------

// PANZERS 0x6cbd50
void SMaterial::Init()
{
    memset(this, 0, sizeof(*this));
    Passes[0].Init();
    Passes[1].Init();
    for (int i = 0; i < 5; ++i)
        Textures[i] = -1;
}

void SMaterial::Release()
{
    SIGepardHD* g = PzGepard();
    for (int i = 0; i < 5; ++i) {
        if (Textures[i] >= 0)
            g->ReleaseTexture(Textures[i]);
        Textures[i] = -1;
    }
}

// PANZERS 0x6cccc0
void SMaterial::SetTexture(unsigned slot, int texture)
{
    if (slot >= 5) {
        Logger.g->Panic("SMaterial::SetTexture: Invalid texture index: %d", slot);
        return;
    }
    PzGepard()->ReleaseTexture(Textures[slot]);   // Gepard +0x48
    Textures[slot] = texture;
}

// PANZERS 0x6c27f0 (ARGB -> r, g, b, a in 0..1)
static void ArgbToFloat4(float* o, unsigned c)
{
    const double k = 1.0 / 255.0;   // 0x80ca20
    o[0] = (float)((double)(c >> 16 & 0xff) * k);
    o[1] = (float)((double)(c >> 8 & 0xff) * k);
    o[2] = (float)((double)(c & 0xff) * k);
    o[3] = (float)((double)(c >> 24) * k);
}

static const float kSphereMap[16] = {   // 0x883080 / 0x8830a0 / 0 / 0x883090
    0.5f, 0.0f, 0.0f, 0.0f,
    0.0f, -0.5f, 0.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 0.0f,
};

// PANZERS 0x6ccb20
// Texture-coordinate animation of the node being drawn (SModel::Render sets
// g_MeshDraw.TexAnim*): 1 = scroll by (u, v), 2 = rotate by the angle about
// the texture centre (aspect-corrected).
void SMaterial::SetTexAnim(int slot, int pass, unsigned stage)
{
    float m[16];
    memset(m, 0, sizeof(m));
    m[0] = m[5] = m[10] = m[15] = 1.0f;   // 0x67cbc0 identity
    if (g_MeshDraw.TexAnimType == 1) {
        m[8] = g_MeshDraw.TexAnim[0];
        m[9] = g_MeshDraw.TexAnim[1];
        Passes[pass].SetTexMatrix(stage, D3DTTFF_COUNT2, m);
        return;
    }
    if (g_MeshDraw.TexAnimType == 2) {
        // 0x6ccb20 type 2: Gepard GetTextureSize, then a 2D rotation by
        // TexAnim[2] about (0.5, 0.5) scaled by the texture aspect, built
        // with 0x7c5fb0 and translated by (u, v). Lifted from the
        // decompilation; the 0x7c5fb0 helper is approximated by the
        // equivalent rotation-about-centre matrix (not byte-checked).
        int w = 1, h = 1;
        PzGepard()->GetTextureSize(Textures[slot], &w, &h);
        double a = (double)g_MeshDraw.TexAnim[2];
        float c = (float)cos(a);
        float s = (float)sin(a);
        float sx = s, sy = s;
        if (w != h && w && h) {
            sx = (float)(((double)w / (double)h) * s);
            sy = (float)(((double)h / (double)w) * s);
        }
        m[0] = c;   m[1] = -sx;
        m[4] = sy;  m[5] = c;
        m[8] = 0.5f - (c * 0.5f + sy * 0.5f) + g_MeshDraw.TexAnim[0];
        m[9] = 0.5f - (-sx * 0.5f + c * 0.5f) + g_MeshDraw.TexAnim[1];
        Passes[pass].SetTexMatrix(stage, D3DTTFF_COUNT2, m);
    }
}

// PANZERS 0x6cc9e0
// Second pass of a reflective material on hardware without the PS 1.x
// path: gloss (SPEC) * sphere-mapped REFL, added on top.
void SMaterial::SetupReflectionPass(int pass)
{
    SRenderPass& p = Passes[pass];
    p.SetTexture(0, Textures[1], true);
    SetTexAnim(1, pass, 0);
    p.SetColorOp(0, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
    p.SetAlphaOp(0, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_CURRENT);
    p.SetTexture(1, Textures[4], false);
    p.SetTexTransform(1, D3DTSS_TCI_CAMERASPACENORMAL, D3DTTFF_COUNT2, kSphereMap);
    p.SetColorOp(1, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
    p.SetAlphaOp(1, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_CURRENT);
}

// PANZERS 0x6cbe00
// type: 0 fixed function / CPU-transformed vertices, 1..4 the vertex-shader
// variants (skinning, frame tweening...), never reached in HD (see mesh.h).
void SMaterial::Begin(SScene* scene, int type)
{
    if ((g_MeshDraw.DeferAlpha && GepardGetTextureAlpha(Textures[0]) == 2) ||
        (g_MeshDraw.DrawingDeferred && GepardGetTextureAlpha(Textures[0]) != 2)) {
        PassCount = 0;
        return;
    }
    SRenderPass& p0 = Passes[0];
    p0.Reset();
    if (g_MeshDraw.FogTint != 0)
        p0.SetFogMode(3, (unsigned)g_MeshDraw.FogTint);
    bool mod2x = GepardOption(0x10) != 0;
    int colorMod = mod2x ? D3DTOP_MODULATE2X : D3DTOP_MODULATE;

    // Reflective (REFL + SPEC + DIFF) without PS 1.x: two passes.
    if (Textures[4] >= 0 && Textures[1] >= 0 && Textures[0] >= 0 && !HD().PixelShader1x) {
        PassCount = 2;
        p0.SetBlendMode(0, Textures[0]);
        p0.SetTexture(0, Textures[0], true);
        SetTexAnim(0, 0, 0);
        p0.SetColorOp(0, colorMod, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
        p0.SetAlphaOp(0, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_CURRENT);
        Passes[1].Reset();
        Passes[1].SetBlendMode(1, -1);
        SetupReflectionPass(1);
        return;
    }

    // Shadow-receiving materials (type 0, a diffuse texture or no
    // reflection/illumination map, scene shadow texture +0x14 present): the
    // shadow buffer on stage 0 (and 1 for PS 1.4), texture coordinates from
    // the camera-space position through scene +0x58.
    bool shadowed = type == 0 && (Textures[0] >= 0 || (Textures[4] < 0 && Textures[2] < 0)) &&
                    scene && scene->ShadowTexture >= 0;
    PassCount = 1;
    unsigned stage = 0;
    if (!shadowed) {
        if (g_MeshDraw.ColorOverride)
            p0.TextureFactor = g_MeshDraw.Color;
        else if (g_MeshDraw.Color2Override)
            p0.TextureFactor = g_MeshDraw.Color2;
    } else {
        int tech = GepardOption(2);
        if (tech == 2) {
            p0.SetTexture(0, scene->ShadowTexture, false);
            p0.SetTexTransform(0, D3DTSS_TCI_CAMERASPACEPOSITION, 0x104, scene->ShadowMatrix);
            stage = 1;
        } else if (tech == 4) {
            p0.SetTexture(0, scene->ShadowTexture, false);
            p0.SetTexTransform(0, D3DTSS_TCI_CAMERASPACEPOSITION, D3DTTFF_COUNT4, scene->ShadowMatrix);
            stage = 1;
        } else if (tech == 3) {
            p0.SetTexture(0, scene->ShadowTexture, false);
            p0.SetTexTransform(0, D3DTSS_TCI_CAMERASPACEPOSITION, D3DTTFF_COUNT4, scene->ShadowMatrix);
            p0.SetTexTransform(1, D3DTSS_TCI_CAMERASPACEPOSITION, D3DTTFF_COUNT4, scene->ShadowMatrix);
            stage = 2;
        } else {
            shadowed = false;
            if (g_MeshDraw.ColorOverride)
                p0.TextureFactor = g_MeshDraw.Color;
            else if (g_MeshDraw.Color2Override)
                p0.TextureFactor = g_MeshDraw.Color2;
        }
    }
    unsigned base = stage;
    unsigned next = stage;
    unsigned cur = stage;
    if (Textures[0] < 0) {
        if (!g_MeshDraw.AlphaOverride) {
            p0.SetAlphaMode(0);
        } else {
            p0.SetAlphaMode(2);
            p0.ZWrite = GepardGetTextureAlpha(Textures[0]) != 2;
        }
    } else {
        if (!g_MeshDraw.AlphaOverride) {
            p0.SetBlendMode(0, Textures[0]);
        } else {
            p0.SetAlphaMode(2);
            p0.ZWrite = GepardGetTextureAlpha(Textures[0]) != 2;
        }
        p0.SetTexture(stage, Textures[0], true);
        p0.SetTexCoordIndex(stage, 0);
        SetTexAnim(0, 0, stage);
        cur = stage + 1;
        next = stage + 1;
    }
    if (Textures[4] < 0) {
        if (Textures[2] >= 0) {
            p0.SetTexture(next, Textures[2], true);
            p0.SetTexCoordIndex(cur, 0);
            SetTexAnim(2, 0, cur);
        }
    } else {
        if (Textures[1] >= 0) {
            p0.SetTexture(next, Textures[1], true);
            p0.SetTexCoordIndex(cur, 0);
            SetTexAnim(1, 0, cur);
            ++cur;
            next = cur;
        }
        p0.SetTexture(next, Textures[4], false);
        p0.SetTexTransform(cur, D3DTSS_TCI_CAMERASPACENORMAL, D3DTTFF_COUNT2, kSphereMap);
    }
    if (type >= 1 && type <= 4) {
        // 0x6cbe00: vertex shader id per technique (1..0x3c by type, fog
        // range, which textures). The HD ids are never created (mesh.h).
        STUB_LOG("SMaterial::Begin vertex-shader technique (0x6cbe00 types 1..4)");
    }
    if (GepardOption(2) > 1 && shadowed) {
        // Shadow-buffer combiners (pixel shaders 0xb..0x22): texture * (lit
        // diffuse, or the ambient c1 where the shadow buffer holds a higher
        // caster), c1.w the alpha; the colour overrides tint with c2.
        static float c[8];   // 0x93cf04 (a static: c2 keeps its last tint)
        c[0] = scene->Ambient[0];
        c[1] = scene->Ambient[1];
        c[2] = scene->Ambient[2];
        c[3] = g_MeshDraw.AlphaOverride ? g_MeshDraw.Alpha : 1.0f;
        int m2x = mod2x ? 1 : 0;
        if (g_MeshDraw.ColorOverride || g_MeshDraw.Color2Override) {
            ArgbToFloat4(c + 4, (unsigned)(g_MeshDraw.ColorOverride ? g_MeshDraw.Color : g_MeshDraw.Color2));   // 0x6c27f0
            GepardSetPixelShaderConstant(1, c, 2);
            int base0 = g_MeshDraw.ColorOverride ? 0x1b : 0x13;
            if (Textures[4] < 0)
                p0.PixelShader = (Textures[2] < 0 ? base0 : base0 + 2) + m2x;
            else
                p0.PixelShader = (Textures[1] < 0 ? base0 + 4 : base0 + 6) + m2x;
            return;
        }
        GepardSetPixelShaderConstant(1, c, 1);
        if (Textures[4] < 0)
            p0.PixelShader = (Textures[2] < 0 ? 0xb : 0xd) + m2x;
        else
            p0.PixelShader = (Textures[1] < 0 ? 0xf : 0x11) + m2x;
        return;
    }
    int alphaOp = D3DTOP_SELECTARG1;
    int alphaArg2 = D3DTA_CURRENT;
    if (g_MeshDraw.AlphaOverride) {
        alphaArg2 = D3DTA_TFACTOR;
        alphaOp = D3DTOP_MODULATE;
        p0.TextureFactor = ((int)(g_MeshDraw.Alpha * 255.0f) << 24) | (p0.TextureFactor & 0xffffff);
    }
    p0.SetAlphaOp(base, alphaOp, D3DTA_TEXTURE, alphaArg2);
    int op2;
    if (Textures[4] < 0) {
        if (Textures[2] < 0) {
            p0.SetColorOp(base, colorMod, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
            if (Textures[0] >= 0) {
                if (g_MeshDraw.ColorOverride)
                    p0.SetColorOp(base + 1, D3DTOP_ADD, D3DTA_TFACTOR, D3DTA_CURRENT, D3DTA_CURRENT);
                else if (g_MeshDraw.Color2Override)
                    p0.SetColorOp(base + 1, D3DTOP_MODULATE, D3DTA_TFACTOR, D3DTA_CURRENT, D3DTA_CURRENT);
            }
            return;
        }
    } else if (Textures[1] >= 0) {
        if (Textures[0] >= 0) {
            // PS 1.x reflection combiner (pixel shaders 2..7, PixelShader1x):
            // c1 = the colour override tint (a static, kept when none is
            // set) and the alpha.
            static float c[4];   // 0x93cef0
            if (g_MeshDraw.ColorOverride || g_MeshDraw.Color2Override) {
                float t[4];
                ArgbToFloat4(t, (unsigned)(g_MeshDraw.ColorOverride ? g_MeshDraw.Color : g_MeshDraw.Color2));
                c[0] = t[0];
                c[1] = t[1];
                c[2] = t[2];
            }
            c[3] = g_MeshDraw.AlphaOverride ? g_MeshDraw.Alpha : 1.0f;
            GepardSetPixelShaderConstant(1, c, 1);
            int m2x = mod2x ? 1 : 0;
            if (g_MeshDraw.ColorOverride)
                p0.PixelShader = 6 + m2x;
            else if (g_MeshDraw.Color2Override)
                p0.PixelShader = 4 + m2x;
            else
                p0.PixelShader = 2 + m2x;
            return;
        }
        p0.SetColorOp(base, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
        op2 = D3DTOP_MODULATE;
        p0.SetColorOp(base + 1, op2, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
        p0.SetAlphaOp(base + 1, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT);
        return;
    }
    if (Textures[0] < 0) {
        p0.SetColorOp(base, D3DTOP_SELECTARG1, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
        return;
    }
    p0.SetColorOp(base, colorMod, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
    op2 = D3DTOP_ADD;
    p0.SetColorOp(base + 1, op2, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
    p0.SetAlphaOp(base + 1, D3DTOP_DISABLE, D3DTA_TEXTURE, D3DTA_CURRENT);
}

// ---------------------------------------------------------------------------
// SMesh
// ---------------------------------------------------------------------------

// PANZERS 0x6ccd00
SMesh::SMesh()
{
    Device = HD().Device;   // SGepard +0x478
    VertexBuffer = nullptr;
    Vertices = nullptr;
    VertexCount = 0;
    Fvf = 0;
    Stride = 0;
    IndexBuffer = nullptr;
    Indices = nullptr;
    IndexCount = 0;
    Materials = nullptr;
    MaterialCount = 0;
    Decl = -1;
    OffPosition = OffNormal = OffColor0 = OffColor1 = -1;
    OffTex0 = OffTex1 = OffTex2 = OffTangents = OffBlend = -1;
}

// PANZERS 0x6ccdc0 (dtor body of 0x6ccfd0)
SMesh::~SMesh()
{
    if (VertexBuffer) {
        VertexBuffer->Release();
        VertexBuffer = nullptr;
    }
    if (IndexBuffer) {
        IndexBuffer->Release();
        IndexBuffer = nullptr;
    }
    if (Materials) {
        for (unsigned i = 0; i < MaterialCount; ++i)
            Materials[i].Release();
        free(Materials);
        Materials = nullptr;
    }
}

// Vertex declarations HD keeps in the SGepard heap +0x624 (0x6cd000 /
// 0x6ccee0); one per distinct non-FVF layout.
static IDirect3DVertexDeclaration9* s_Decls[8];
static unsigned s_DeclKeys[8];
static int s_DeclCount;

static int FindOrCreateDecl(unsigned key, const D3DVERTEXELEMENT9* el)
{
    for (int i = 0; i < s_DeclCount; ++i)
        if (s_DeclKeys[i] == key)
            return i;
    if (s_DeclCount == 8 || !HD().Device)
        return -1;
    IDirect3DVertexDeclaration9* d = nullptr;
    if (FAILED(HD().Device->CreateVertexDeclaration(el, &d)))
        return -1;
    s_Decls[s_DeclCount] = d;
    s_DeclKeys[s_DeclCount] = key;
    return s_DeclCount++;
}

IDirect3DVertexDeclaration9* MeshVertexDecl(int i)
{
    return (i >= 0 && i < s_DeclCount) ? s_Decls[i] : nullptr;
}

void ReleaseMeshVertexDecls()
{
    for (int i = 0; i < s_DeclCount; ++i)
        if (s_Decls[i])
            s_Decls[i]->Release();
    s_DeclCount = 0;
}

// PANZERS 0x6cd290
// HD vertex types: D3DFVF bits plus 0x2000 (three float3 tangents) and
// 0x4000 (UBYTE4 blend indices + float4 weights). Builds the element list,
// the stride and the component offsets; a type with 0x2000/0x4000 gets a
// vertex declaration instead of an FVF.
void SMesh::SetVertexFormat(unsigned type)
{
    Fvf = 0;
    Stride = 0;
    OffPosition = OffNormal = OffColor0 = OffColor1 = -1;
    OffTex0 = OffTex1 = OffTex2 = OffTangents = OffBlend = -1;
    bool fvfOnly = true;
    D3DVERTEXELEMENT9 el[16];
    int n = 0;
    auto add = [&](BYTE t, BYTE usage, BYTE index, int size) {
        D3DVERTEXELEMENT9 e = { 0, (WORD)Stride, t, D3DDECLMETHOD_DEFAULT, usage, index };
        el[n++] = e;
        Stride += size;
    };
    if (!(type & D3DFVF_XYZ)) {
        Logger.g->Panic("SMesh::CreateVertexBuffer: Invalid vertex type no XYZ.");
        return;
    }
    Fvf = D3DFVF_XYZ;
    unsigned rest = type & ~2u;
    OffPosition = 0;
    add(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_POSITION, 0, 12);
    if (type & D3DFVF_NORMAL) {
        Fvf |= D3DFVF_NORMAL;
        rest &= ~(unsigned)D3DFVF_NORMAL;
        OffNormal = Stride;
        add(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_NORMAL, 0, 12);
    }
    if (rest & 0x2000) {
        fvfOnly = false;
        rest &= ~0x2000u;
        OffTangents = Stride;
        add(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TANGENT, 0, 12);
        add(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TANGENT, 1, 12);
        add(D3DDECLTYPE_FLOAT3, D3DDECLUSAGE_TANGENT, 2, 12);
    }
    if (rest & D3DFVF_DIFFUSE) {
        Fvf |= D3DFVF_DIFFUSE;
        rest &= ~(unsigned)D3DFVF_DIFFUSE;
        OffColor0 = Stride;
        add(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 0, 4);
    }
    if (rest & D3DFVF_SPECULAR) {
        Fvf |= D3DFVF_SPECULAR;
        rest &= ~(unsigned)D3DFVF_SPECULAR;
        OffColor1 = Stride;
        add(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_COLOR, 1, 4);
    }
    unsigned tex = rest & 0xf00;
    if (tex >= 0x100 && tex <= 0x300) {
        Fvf |= tex;
        rest &= ~0xf00u;
        OffTex0 = Stride;
        add(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 0, 8);
        if (tex >= 0x200) {
            OffTex1 = Stride;
            add(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 1, 8);
        }
        if (tex == 0x300) {
            OffTex2 = Stride;
            add(D3DDECLTYPE_FLOAT2, D3DDECLUSAGE_TEXCOORD, 2, 8);
        }
    }
    if (rest & 0x4000) {
        rest &= ~0x4000u;
        fvfOnly = false;
        OffBlend = Stride;
        add(D3DDECLTYPE_D3DCOLOR, D3DDECLUSAGE_BLENDINDICES, 0, 4);
        add(D3DDECLTYPE_FLOAT4, D3DDECLUSAGE_BLENDWEIGHT, 0, 16);
    }
    if (rest != 0) {
        Logger.g->Panic("SMesh::CreateVertexBuffer: Invalid vertex type.");
        return;
    }
    if (fvfOnly)
        return;
    Fvf = 0;
    D3DVERTEXELEMENT9 end = D3DDECL_END();
    el[n] = end;
    Decl = FindOrCreateDecl(type, el);
}

// PANZERS 0x6cd1f0
void SMesh::CreateVertexBuffer(unsigned type, int count)
{
    SetVertexFormat(type);
    VertexCount = count;
    HRESULT hr = Device->CreateVertexBuffer(Stride * count, D3DUSAGE_WRITEONLY, Fvf,
                                            D3DPOOL_MANAGED, &VertexBuffer, nullptr);
    if (FAILED(hr)) {
        Logger.g->Panic("%s: %08x", "SMesh::CreateVertexBuffer: CreateVertexBuffer failed", (unsigned)hr);
        return;
    }
    Lock();
    memset(Vertices, 0, Stride * VertexCount);
}

// PANZERS 0x6ce9c0
void SMesh::Lock()
{
    VertexBuffer->Lock(0, 0, (void**)&Vertices, 0);
}

// PANZERS 0x6cea50
void SMesh::Unlock()
{
    VertexBuffer->Unlock();
    Vertices = nullptr;
}

// PANZERS 0x6cd070
unsigned short* SMesh::CreateIndexBuffer(int count)
{
    IndexCount = count;
    HRESULT hr = Device->CreateIndexBuffer(count * 2, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
                                           D3DPOOL_MANAGED, &IndexBuffer, nullptr);
    if (FAILED(hr)) {
        Logger.g->Panic("%s: %08x", "SMesh::CreateIndexBuffer: CreateIndexBuffer failed", (unsigned)hr);
        return nullptr;
    }
    IndexBuffer->Lock(0, 0, (void**)&Indices, 0);
    memset(Indices, 0, IndexCount * 2);
    return Indices;
}

// PANZERS 0x6cea30
void SMesh::UnlockIndexBuffer()
{
    IndexBuffer->Unlock();
    Indices = nullptr;
}

// PANZERS 0x6cd0f0
SMaterial* SMesh::CreateMaterials(unsigned count)
{
    MaterialCount = count;
    Materials = (SMaterial*)malloc(sizeof(SMaterial) * (count ? count : 1));
    for (unsigned i = 0; i < count; ++i) {
        Materials[i].Init();          // 0x6ccd70
        Materials[i].PrimCount = 0;
        Materials[i].MinIndex = 0;
        Materials[i].NumVertices = 0;
        Materials[i].PrimType = D3DPT_TRIANGLELIST;
    }
    return Materials;
}

static inline int AdvanceStart(const SMaterial& m, int start)
{
    return m.PrimType == D3DPT_TRIANGLELIST ? start + m.PrimCount * 3 : start + 2 + m.PrimCount;
}

// PANZERS 0x6cd890
void SMesh::Draw(SScene* scene)
{
    if (!VertexBuffer) { Logger.g->Panic("SMesh::Draw: pVertexBuffer is NULL"); return; }
    if (!IndexBuffer) { Logger.g->Panic("SMesh::Draw: pIndexBuffer is NULL"); return; }
    if (!Materials) { Logger.g->Panic("SMesh::Draw: Materials is NULL"); return; }
    Device->SetIndices(IndexBuffer);
    HRESULT hr = Device->SetStreamSource(0, VertexBuffer, 0, Stride);
    if (FAILED(hr)) {
        Logger.g->Panic("%s: %08x", "SMesh::Draw: SetStreamSource failed", (unsigned)hr);
        return;
    }
    GepardSetVertexFormat(Decl, Fvf);
    int start = 0;
    for (unsigned i = 0; i < MaterialCount; ++i) {
        SMaterial& m = Materials[i];
        m.Begin(scene, 0);
        int passes = m.PassCount;   // 0x6cc9a0
        for (int p = 0; p < passes; ++p) {
            m.BeginPass(p);
            Device->DrawIndexedPrimitive((D3DPRIMITIVETYPE)m.PrimType, 0, m.MinIndex, m.NumVertices,
                                         start, m.PrimCount);
            HD().PolyCount += m.PrimCount;
            HD().VertexCount += m.NumVertices;
        }
        start = AdvanceStart(m, start);
    }
}

// The material loop of the shadow-buffer draws (0x6cdb70 static vertex
// buffer, 0x6ce170 dynamic one). One pass for the whole mesh: no
// lighting-dependent colour (stage 0 SELECTARG2 = diffuse, black with the
// lights off), no fog, no culling; alpha-blended materials cast nothing.
//  4 (PS 2.0): t0 = camera-space position * scene +0x58 (the height), PS 8
//    writes it; alpha-tested materials use PS 9 (texkill below 0.5) with
//    the diffuse texture on s0 and its uv on t1. With a vs_2_0 device the
//    mesh goes through vertex shader 0x41 / 0x42 instead of fixed function.
//  3 (PS 1.4): the same with the texture on stage 1 and the alpha mode.
//  1, 2: colour only (alpha test from the texture).
void SMesh::DrawShadowMaterials(SScene* scene, bool dynamic)
{
    SRenderPass pass;
    pass.Init();                                      // 0x687730
    pass.SetColorOp(0, D3DTOP_SELECTARG2, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
    pass.SetFogMode(2, 0);
    pass.CullMode = scene->ShadowCull;
    int start = 0;
    for (unsigned i = 0; i < MaterialCount; ++i) {
        SMaterial& m = Materials[i];
        int tex = m.Textures[0];                      // 0x6cc9b0(0)
        if (GepardGetTextureAlpha(tex) != 2) {
            int tech = GepardOption(2);
            if (tech == 4) {
                pass.SetTexTransform(0, D3DTSS_TCI_CAMERASPACEPOSITION, D3DTTFF_COUNT3, scene->ShadowMatrix);
                if (GepardGetTextureAlpha(tex) == 1) {
                    pass.PixelShader = 9;
                    pass.SetTexCoordIndex(1, 0);
                    pass.SetTexture(0, tex, true);
                } else {
                    pass.PixelShader = 8;
                    pass.SetTexCoordIndex(1, 0);
                    pass.SetTexture(0, -1, true);
                }
            } else {
                int alphaMode;
                if (tech == 3) {
                    pass.SetTexTransform(0, D3DTSS_TCI_CAMERASPACEPOSITION, D3DTTFF_COUNT3, scene->ShadowMatrix);
                    pass.SetTexCoordIndex(1, 0);
                    if (GepardGetTextureAlpha(tex) == 1) {
                        pass.PixelShader = 9;
                        pass.SetTexture(1, tex, true);
                        alphaMode = 1;
                    } else {
                        pass.PixelShader = 8;
                        pass.SetTexture(1, -1, true);
                        alphaMode = 0;
                    }
                } else if (GepardGetTextureAlpha(tex) == 1) {
                    pass.SetTexture(0, tex, true);
                    alphaMode = 1;
                } else {
                    pass.SetTexture(0, -1, true);
                    alphaMode = 0;
                }
                pass.SetAlphaMode(alphaMode);
            }
            pass.CullMode = D3DCULL_NONE;
            pass.Apply();                             // 0x687a50
            bool vs = GepardOption(2) == 4 && pass.VertexShader == 0 && HD().VertexShaders[0x41];
            if (vs) {
                // c0 = (W V P)^T, c12 = (W V)^T, c8 = (T0)^T from the device.
                float W[16], V[16], P[16], T[16], WV[16], WVP[16], c[16];
                Device->GetTransform(D3DTS_WORLD, (D3DMATRIX*)W);
                Device->GetTransform(D3DTS_VIEW, (D3DMATRIX*)V);
                Device->GetTransform(D3DTS_PROJECTION, (D3DMATRIX*)P);
                Device->GetTransform(D3DTS_TEXTURE0, (D3DMATRIX*)T);
                GepardSetVertexFormat(Decl, Fvf);
                GepardSetVertexShader(pass.PixelShader == 9 ? 0x42 : 0x41);
                M44Mul(WV, W, V);
                M44Mul(WVP, WV, P);
                M44Transpose(c, WVP);
                GepardSetVertexShaderConstant(0, c, 4);
                M44Transpose(c, WV);
                GepardSetVertexShaderConstant(0xc, c, 4);
                M44Transpose(c, T);
                GepardSetVertexShaderConstant(8, c, 4);
            }
            if (dynamic) {
                GepardDrawDynamicVB(m.PrimType, m.MinIndex, m.NumVertices, start, m.PrimCount, 1);
            } else {
                Device->DrawIndexedPrimitive((D3DPRIMITIVETYPE)m.PrimType, 0, m.MinIndex, m.NumVertices, start,
                                             m.PrimCount);
                HD().PolyCount += m.PrimCount;
                HD().VertexCount += m.NumVertices;
            }
            if (vs)
                GepardSetVertexShader(0);
        }
        start = AdvanceStart(m, start);
    }
}

// PANZERS 0x6cdb70
void SMesh::DrawShadow(SScene* scene)
{
    if (!VertexBuffer) { Logger.g->Panic("SMesh::DrawShadow: pVertexBuffer is NULL"); return; }
    if (!IndexBuffer) { Logger.g->Panic("SMesh::DrawShadow: pIndexBuffer is NULL"); return; }
    if (!Materials) { Logger.g->Panic("SMesh::DrawShadow: Materials is NULL"); return; }
    Device->SetIndices(IndexBuffer);
    HRESULT hr = Device->SetStreamSource(0, VertexBuffer, 0, Stride);
    if (FAILED(hr)) {
        Logger.g->Panic("%s: %08x", "SMesh::Draw: SetStreamSource failed", (unsigned)hr);
        return;
    }
    GepardSetVertexFormat(Decl, Fvf);
    DrawShadowMaterials(scene, false);
}

// PANZERS 0x6ce170
// The dynamic-buffer variant (animated and skinned meshes): the vertices
// the caller filled, then the buffer advances.
void SMesh::DrawShadowDynamic(SScene* scene)
{
    Device->SetIndices(IndexBuffer);
    DrawShadowMaterials(scene, true);
    GepardAdvanceDynamicVB(VertexCount);
}

// PANZERS 0x6cda70
// Draws the CPU-filled dynamic vertex buffer (FVF 0x112) with this mesh's
// index buffer and materials, then advances the dynamic buffer.
void SMesh::DrawDynamic(SScene* scene)
{
    Device->SetIndices(IndexBuffer);
    int start = 0;
    for (unsigned i = 0; i < MaterialCount; ++i) {
        SMaterial& m = Materials[i];
        m.Begin(scene, 0);
        int passes = m.PassCount;
        for (int p = 0; p < passes; ++p) {
            m.BeginPass(p);
            GepardDrawDynamicVB(m.PrimType, m.MinIndex, m.NumVertices, start, m.PrimCount, 1);
        }
        start = AdvanceStart(m, start);
    }
    GepardAdvanceDynamicVB(VertexCount);
}

// ---------------------------------------------------------------------------
// SAnimesh
// ---------------------------------------------------------------------------

// PANZERS 0x6cea70
SAnimesh::SAnimesh()
{
    FrameBuffers = nullptr;
    FrameBufferCount = 0;
    FrameBufferMax = 0;
    Frames = nullptr;
    FrameCount = 0;
    FrameMax = 0;
}

// PANZERS 0x6ceac0 (dtor body of 0x6cec90)
SAnimesh::~SAnimesh()
{
    for (int i = 0; i < FrameBufferCount; ++i)
        if (FrameBuffers[i])
            FrameBuffers[i]->Release();
    free(FrameBuffers);
    for (int i = 0; i < FrameCount; ++i)
        operator delete(Frames[i]);
    free(Frames);
    VertexBuffer = nullptr;   // never created for an animesh
}

// PANZERS 0x6ceda0
// One call per VERT chunk: each adds a frame (system memory in HD, because
// the frame-tweening vertex shader 0x11 does not exist; see mesh.h).
void SAnimesh::CreateVertexBuffer(unsigned type, int count)
{
    if (type != 0x112) {
        Logger.g->Panic("SAnimesh::CreateVertexBuffer: Unsupported vertex type");
        return;
    }
    SetVertexFormat(0x112);
    VertexCount = count;
    unsigned char* block = (unsigned char*)operator new((size_t)Stride * count);
    if (FrameCount == FrameMax) {   // 0x6cecc0
        int n = FrameMax < 0x10 ? 0x10 : (FrameMax * 6) / 5;
        Frames = (unsigned char**)realloc(Frames, n * sizeof(*Frames));
        memset(Frames + FrameMax, 0, (n - FrameMax) * sizeof(*Frames));
        FrameMax = n;
    }
    Frames[FrameCount++] = block;
    Lock();
    memset(Vertices, 0, Stride * VertexCount);
}

// PANZERS 0x6d0b40
void SAnimesh::Lock()
{
    Vertices = Frames[FrameCount - 1];
}

// PANZERS 0x6d0bd0
void SAnimesh::Unlock()
{
    Vertices = nullptr;
}

// PANZERS 0x6cef00 (vertex fill)
// HD locks twice the vertex count here and copies one frame.
bool SAnimesh::FillFrame(int frame)
{
    float* dst = (float*)GepardLockDynamicVB(HD().DynType112, VertexCount * 2);
    if (frame < 0 || frame >= FrameCount) {
        GepardUnlockDynamicVB();
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "unsigned char*", frame);
        return false;
    }
    memcpy(dst, Frames[frame], (size_t)VertexCount * 32);
    GepardUnlockDynamicVB();
    return true;
}

// PANZERS 0x6cef00
void SAnimesh::DrawFrame(SScene* scene, int frame)
{
    if (FillFrame(frame))
        DrawDynamic(scene);
}

// PANZERS 0x6cf140 (vertex fill)
bool SAnimesh::FillFramesLerp(int a0, int a1, float t)
{
    float* dst = (float*)GepardLockDynamicVB(HD().DynType112, VertexCount);
    if (a0 < 0 || a0 >= FrameCount || a1 < 0 || a1 >= FrameCount) {
        GepardUnlockDynamicVB();
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "unsigned char*", a0);
        return false;
    }
    const float* A = (const float*)Frames[a0];
    const float* B = (const float*)Frames[a1];
    for (int i = 0; i < VertexCount; ++i, A += 8, B += 8, dst += 8) {
        for (int k = 0; k < 6; ++k)
            dst[k] = (B[k] - A[k]) * t + A[k];
        dst[6] = A[6];
        dst[7] = A[7];
    }
    GepardUnlockDynamicVB();
    return true;
}

// PANZERS 0x6cf140
void SAnimesh::DrawFramesLerp(SScene* scene, int a0, int a1, float t)
{
    if (FillFramesLerp(a0, a1, t))
        DrawDynamic(scene);
}

// PANZERS 0x6cf500 (vertex fill)
bool SAnimesh::FillFramesBlend(int a0, int a1, float t, int b, float w)
{
    float* dst = (float*)GepardLockDynamicVB(HD().DynType112, VertexCount);
    if (a0 < 0 || a0 >= FrameCount || a1 < 0 || a1 >= FrameCount || b < 0 || b >= FrameCount) {
        GepardUnlockDynamicVB();
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "unsigned char*", a0);
        return false;
    }
    const float* A = (const float*)Frames[a0];
    const float* B = (const float*)Frames[a1];
    const float* C = (const float*)Frames[b];
    for (int i = 0; i < VertexCount; ++i, A += 8, B += 8, C += 8, dst += 8) {
        for (int k = 0; k < 6; ++k) {
            float l = (B[k] - A[k]) * t + A[k];
            dst[k] = (C[k] - l) * w + l;
        }
        dst[6] = A[6];
        dst[7] = A[7];
    }
    GepardUnlockDynamicVB();
    return true;
}

// PANZERS 0x6cf500
void SAnimesh::DrawFramesBlend(SScene* scene, int a0, int a1, float t, int b, float w)
{
    if (FillFramesBlend(a0, a1, t, b, w))
        DrawDynamic(scene);
}

// The shadow-buffer draws of an animated mesh (Gepard option 6). HD only
// takes its vertex-shader branch with SGepard vertex shader 0x11, which it
// never creates; the CPU branch fills the dynamic buffer as the colour
// draws do and draws it with SMesh::DrawShadowDynamic 0x6ce170.

// PANZERS 0x6d0400
void SAnimesh::DrawShadowFramesBlend(SScene* scene, int a0, int a1, float t, int b, float w)
{
    if (GepardOption(6) == 0)
        return;
    if (FillFramesBlend(a0, a1, t, b, w))
        DrawShadowDynamic(scene);
}

// PANZERS 0x6cfe00
void SAnimesh::DrawShadowFramesLerp(SScene* scene, int a0, int a1, float t)
{
    if (GepardOption(6) == 0)
        return;
    if (FillFramesLerp(a0, a1, t))
        DrawShadowDynamic(scene);
}

// PANZERS 0x6cf9c0
void SAnimesh::DrawShadowFrame(SScene* scene, int frame)
{
    if (GepardOption(6) == 0)
        return;
    if (FillFrame(frame))
        DrawShadowDynamic(scene);
}

// ---------------------------------------------------------------------------
// SSkinnedMesh
// ---------------------------------------------------------------------------

// PANZERS 0x6d0c30
SSkinnedMesh::SSkinnedMesh()
{
    SkinVertices = nullptr;
    SkinBuffer = nullptr;
}

// PANZERS 0x6d0d00 (dtor)
SSkinnedMesh::~SSkinnedMesh()
{
    if (SkinBuffer)
        SkinBuffer->Release();
    operator delete(SkinVertices);
    SkinVertices = nullptr;
}

// PANZERS 0x6d0d30
// Without SGepard vertex shader 1 the skin vertices stay in system memory.
void SSkinnedMesh::CreateVertexBuffer(unsigned type, int count)
{
    if (type != 0x4112) {
        Logger.g->Panic("SSkinnedMesh::CreateVertexBuffer: Unsupported vertex type");
        return;
    }
    SetVertexFormat(0x4112);
    VertexCount = count;
    SkinVertices = (unsigned char*)operator new((size_t)Stride * count);
    Lock();
    memset(Vertices, 0, Stride * VertexCount);
}

// PANZERS 0x6d33d0
void SSkinnedMesh::Lock()
{
    Vertices = SkinVertices;
}

// PANZERS 0x6d3400
void SSkinnedMesh::Unlock()
{
    Vertices = nullptr;
}

// PANZERS 0x6d0e30
// Shadow-buffer draw of a skinned mesh (Gepard option 6): without SGepard
// vertex shader 1 (HD never creates it) the CPU skinning of 0x6d2200, then
// SMesh::DrawShadowDynamic 0x6ce170.
void SSkinnedMesh::DrawShadowSkinned(SScene* scene, int bones, const float* m)
{
    (void)bones;
    if (GepardOption(6) == 0)
        return;
    FillSkinned(m);
    DrawShadowDynamic(scene);
}

// PANZERS 0x6d2200
// CPU skinning into the dynamic vertex buffer (FVF 0x112). Skin vertex (13
// floats): position, normal, uv, UBYTE4 bone indices, 4 weights. A weight
// of 0.0 ends the bone list (w3, then w2, then w1 tested, as HD does). Bone
// matrices are 3x4 (row vectors), normals renormalised in double.
void SSkinnedMesh::FillSkinned(const float* m)
{
    float* dst = (float*)GepardLockDynamicVB(HD().DynType112, VertexCount);
    const float* v = (const float*)SkinVertices;
    for (int i = 0; i < VertexCount; ++i, v += 13, dst += 8) {
        const unsigned char* idx = (const unsigned char*)(v + 8);
        const float* w = v + 9;
        int count = w[3] != 0.0f ? 4 : (w[2] != 0.0f ? 3 : (w[1] != 0.0f ? 2 : 1));
        float px = 0, py = 0, pz = 0, nx = 0, ny = 0, nz = 0;
        for (int b = 0; b < count; ++b) {
            const float* M = m + idx[b] * 12;
            float x = v[0], y = v[1], z = v[2];
            float tx = M[3] * y + M[0] * x + M[6] * z + M[9];
            float ty = M[1] * x + M[4] * y + M[7] * z + M[10];
            float tz = M[2] * x + M[5] * y + M[8] * z + M[11];
            float a = v[3], c = v[4], d = v[5];
            float qx = M[3] * c + a * M[0] + M[6] * d;
            float qy = M[1] * a + M[4] * c + M[7] * d;
            float qz = M[2] * a + M[5] * c + M[8] * d;
            if (count == 1) {
                px = tx; py = ty; pz = tz;
                nx = qx; ny = qy; nz = qz;
            } else {
                px += tx * w[b]; py += ty * w[b]; pz += tz * w[b];
                nx += qx * w[b]; ny += qy * w[b]; nz += qz * w[b];
            }
        }
        dst[0] = px;
        dst[1] = py;
        dst[2] = pz;
        double inv = 1.0 / sqrt((double)(ny * ny + nx * nx + nz * nz));
        dst[3] = (float)((double)nx * inv);
        dst[4] = (float)((double)ny * inv);
        dst[5] = (float)((double)nz * inv);
        dst[6] = v[6];
        dst[7] = v[7];
    }
    GepardUnlockDynamicVB();
}

// PANZERS 0x6d2200
void SSkinnedMesh::DrawSkinned(SScene* scene, int bones, const float* m)
{
    (void)bones;
    FillSkinned(m);
    DrawDynamic(scene);
}

} // namespace pz
