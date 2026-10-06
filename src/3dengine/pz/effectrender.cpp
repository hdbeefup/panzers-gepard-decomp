// src/3dengine/pz/effectrender.cpp
// Draw helpers for the effects (see effectrender.h). OWNER: agent C.

#include <d3d9.h>
#include <math.h>
#include <string.h>
#include <vector>
#include "effectrender.h"
#include "igepard.h"
#include "gepard.h"
#include "logger.h"

extern SIGepard* Gepard;   // SWINE renderer (window/widget.h)

namespace pz {

static IDirect3DDevice9* Dev()
{
    return ::Gepard ? static_cast<SGepard*>(::Gepard)->lpD3DDev : nullptr;
}

void* EffectDevice()
{
    return Dev();
}

// ---------------------------------------------------------------------------
// Dynamic vertex buffers
// ---------------------------------------------------------------------------

static std::vector<float> s_VB;
static unsigned s_VBFvf = 0;
static bool s_VBLocked = false;

static int FvfStride(unsigned fvf)
{
    // 0x6787a0: XYZ 0xc, XYZRHW 0x10, NORMAL 0xc, DIFFUSE 4, SPECULAR 4,
    // PSIZE 4, plus 8 per texture coordinate set.
    int s = 0;
    if (fvf & 2) s += 0xc;
    if (fvf & 4) s += 0x10;
    if (fvf & 0x10) s += 0xc;
    if (fvf & 0x20) s += 4;
    if (fvf & 0x40) s += 4;
    if (fvf & 0x80) s += 4;
    return s + ((fvf >> 8) & 0xf) * 8;
}

int EffectCreateDynamicVB(unsigned fvf)
{
    return (int)fvf;
}

float* EffectLockDynamicVB(int vb, int vertices)
{
    if (s_VBLocked)
        Logger.g->Panic("SGepard::LockDynamicVB: Already locked.");
    s_VBFvf = (unsigned)vb;
    size_t n = (size_t)vertices * FvfStride(s_VBFvf) / 4;
    if (s_VB.size() < n)
        s_VB.resize(n);
    s_VBLocked = true;
    return s_VB.data();
}

void EffectUnlockDynamicVB()
{
    if (!s_VBLocked)
        Logger.g->Panic("SGepard::UnlockDynamicVB: Not locked.");
    s_VBLocked = false;
}

void EffectDrawDynamicVB(int primType, int primCount)
{
    if (s_VBLocked)
        Logger.g->Panic("SGepard::DrawDynamicVB: Buffer is locked.");
    IDirect3DDevice9* d = Dev();
    if (!d || primCount <= 0)
        return;
    d->SetVertexShader(nullptr);
    d->SetPixelShader(nullptr);
    d->SetFVF(s_VBFvf);
    d->DrawPrimitiveUP((D3DPRIMITIVETYPE)primType, primCount, s_VB.data(), FvfStride(s_VBFvf));
}

void EffectAdvanceDynamicVB(int vertices)
{
    (void)vertices;   // the CPU buffer restarts at 0 on every lock
}

// ---------------------------------------------------------------------------
// Material
// ---------------------------------------------------------------------------

void SEffectMaterial::Reset()
{
    Texture = -1;
    Address = 1;
    ColorOp = D3DTOP_MODULATE;
    AlphaBlend = false;
    SrcBlend = D3DBLEND_ONE;
    DestBlend = D3DBLEND_ZERO;
    AlphaTest = false;
    AlphaRef = 0;
    AlphaFunc = D3DCMP_ALWAYS;
    ZEnable = true;
    ZFunc = D3DCMP_LESSEQUAL;
    ZWrite = true;
    CullMode = D3DCULL_CCW;
    FogMode = 0;
}

void SEffectMaterial::SetTexture(int tex, bool wrap)
{
    Texture = tex;
    Address = wrap ? D3DTADDRESS_WRAP : D3DTADDRESS_CLAMP;
}

void SEffectMaterial::SetBlendFromAlphaType(int alphaType)
{
    if (alphaType == 2) {
        AlphaBlend = true;
        SrcBlend = D3DBLEND_SRCALPHA;
        DestBlend = D3DBLEND_INVSRCALPHA;
        AlphaTest = true;
        AlphaRef = 4;
        AlphaFunc = D3DCMP_GREATER;
        ZWrite = false;
        return;
    }
    AlphaBlend = false;
    SrcBlend = D3DBLEND_ONE;
    DestBlend = D3DBLEND_ZERO;
    ZWrite = true;
    if (alphaType == 1) {
        AlphaTest = true;
        AlphaRef = 0x80;
        AlphaFunc = D3DCMP_GREATER;
        return;
    }
    AlphaTest = false;
    AlphaRef = 0;
    AlphaFunc = D3DCMP_ALWAYS;
}

void SEffectMaterial::SetBlend(int mode, int tex)
{
    if (mode == 1) {           // additive
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
        AlphaBlend = true; SrcBlend = D3DBLEND_ZERO; DestBlend = D3DBLEND_INVSRCCOLOR;
    } else if (mode == 3) {
        AlphaBlend = true; SrcBlend = D3DBLEND_ZERO; DestBlend = D3DBLEND_SRCCOLOR;
    } else if (mode == 4) {
        AlphaBlend = true; SrcBlend = D3DBLEND_DESTCOLOR; DestBlend = D3DBLEND_SRCCOLOR;
    } else {
        SetBlendFromAlphaType(EffectTextureAlphaType(tex));
        return;
    }
    AlphaTest = false; AlphaRef = 0; AlphaFunc = D3DCMP_ALWAYS; ZWrite = false;
}

static DWORD s_SceneFogColor = 0;
static bool  s_SceneFog = false;

void SEffectMaterial::Apply()
{
    IDirect3DDevice9* d = Dev();
    if (!d)
        return;
    if (::Gepard)
        static_cast<SGepard*>(::Gepard)->SetTexture(0, Texture, false);
    d->SetSamplerState(0, D3DSAMP_ADDRESSU, Address);
    d->SetSamplerState(0, D3DSAMP_ADDRESSV, Address);
    d->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    d->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    d->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
    d->SetTextureStageState(0, D3DTSS_COLOROP, ColorOp);
    d->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    d->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    d->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    d->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    d->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_CURRENT);
    d->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
    d->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    d->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
    d->SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
    d->SetRenderState(D3DRS_LIGHTING, FALSE);
    d->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
    d->SetRenderState(D3DRS_CULLMODE, CullMode);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, AlphaBlend);
    d->SetRenderState(D3DRS_SRCBLEND, SrcBlend);
    d->SetRenderState(D3DRS_DESTBLEND, DestBlend);
    d->SetRenderState(D3DRS_ALPHATESTENABLE, AlphaTest);
    d->SetRenderState(D3DRS_ALPHAREF, AlphaRef);
    d->SetRenderState(D3DRS_ALPHAFUNC, AlphaFunc);
    d->SetRenderState(D3DRS_ZENABLE, ZEnable ? D3DZB_TRUE : D3DZB_FALSE);
    d->SetRenderState(D3DRS_ZFUNC, ZFunc);
    d->SetRenderState(D3DRS_ZWRITEENABLE, ZWrite);
    // 0x687a50 fog modes: 0 = the scene fog colour, 1 = black (additive).
    d->SetRenderState(D3DRS_FOGENABLE, s_SceneFog);
    if (s_SceneFog) {
        d->SetRenderState(D3DRS_FOGCOLOR, FogMode == 1 ? 0 : s_SceneFogColor);
        d->SetRenderState(D3DRS_FOGTABLEMODE, D3DFOG_NONE);
        d->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
        d->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);
    }
}

int EffectTextureAlphaType(int tex)
{
    if (!::Gepard)
        return 0;
    SGepard* g = static_cast<SGepard*>(::Gepard);
    if (tex < 0 || tex >= g->Textures.size || g->Textures.array[tex].use != 0x7FFFFFFF)
        return 0;
    return g->Textures.array[tex].data.Alpha;
}

float EffectSinDeg(int deg)
{
    if (deg < 0 || deg > 0x2cf)
        Logger.g->Panic("SGepard::GetSinusValue: invalid degree");
    return (float)sin(deg * 3.14159265358979323846 / 180.0);
}

float EffectCosDeg(int deg)
{
    if (deg < 0 || deg > 0x2cf)
        Logger.g->Panic("SGepard::GetCosinusValue: invalid degree");
    return (float)cos(deg * 3.14159265358979323846 / 180.0);
}

// ---------------------------------------------------------------------------
// Projection (SViewport +0x3c 0x68dda0)
// ---------------------------------------------------------------------------

static D3DXMATRIX s_View;        // HD SViewport+0x90
static D3DXMATRIX s_ToScreen;    // HD SViewport+0x170 (view * projection * viewport)
static float s_SizeFactor = 1.0f;// HD SViewport+0x1f0
static float s_FogStart = 0.0f, s_FogEnd = 0.0f;

void EffectBeginRender(SIViewport* vp)
{
    (void)vp;
    IDirect3DDevice9* d = Dev();
    if (!d)
        return;
    D3DXMATRIX ident;
    D3DXMatrixIdentity(&ident);
    d->SetTransform(D3DTS_WORLD, &ident);      // 0x680fe0

    D3DXMATRIX proj;
    D3DVIEWPORT9 v;
    d->GetTransform(D3DTS_VIEW, &s_View);
    d->GetTransform(D3DTS_PROJECTION, &proj);
    d->GetViewport(&v);
    D3DXMATRIX screen;
    D3DXMatrixIdentity(&screen);
    screen._11 = v.Width * 0.5f;
    screen._22 = -(float)v.Height * 0.5f;
    screen._41 = v.X + v.Width * 0.5f;
    screen._42 = v.Y + v.Height * 0.5f;
    screen._33 = v.MaxZ - v.MinZ;
    screen._43 = v.MinZ;
    s_ToScreen = s_View * proj * screen;
    s_SizeFactor = proj._22 * v.Height * 0.5f;

    DWORD fog = 0, fs = 0, fe = 0, fc = 0;
    d->GetRenderState(D3DRS_FOGENABLE, &fog);
    d->GetRenderState(D3DRS_FOGSTART, &fs);
    d->GetRenderState(D3DRS_FOGEND, &fe);
    d->GetRenderState(D3DRS_FOGCOLOR, &fc);
    memcpy(&s_FogStart, &fs, 4);
    memcpy(&s_FogEnd, &fe, 4);
    s_SceneFog = fog && s_FogStart < s_FogEnd;
    s_SceneFogColor = fc;
}

void EffectEndRender()
{
    IDirect3DDevice9* d = Dev();
    if (!d)
        return;
    d->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    d->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
    d->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
}

bool EffectProject(SIViewport* vp, const float* p, float size,
                   float* sx, float* sy, float* ssize, float* z, unsigned* fog)
{
    (void)vp;
    const D3DXMATRIX& m = s_ToScreen;
    float w = p[1] * m._24 + p[0] * m._14 + p[2] * m._34 + m._44;
    if (w <= 0.0f) {
        *sx = 0.0f;
        *sy = 0.0f;
        *ssize = -1.0f;
        return false;
    }
    float iw = 1.0f / w;
    *sx = (p[1] * m._21 + p[0] * m._11 + p[2] * m._31 + m._41) * iw;
    *sy = (p[1] * m._22 + p[0] * m._12 + p[2] * m._32 + m._42) * iw;
    *z  = (p[1] * m._23 + p[0] * m._13 + p[2] * m._33 + m._43) * iw;
    *ssize = s_SizeFactor * size * iw;
    if (s_SceneFog) {
        const D3DXMATRIX& v = s_View;
        float vx = v._21 * p[1] + v._11 * p[0] + v._31 * p[2] + v._41;
        float vy = v._12 * p[0] + v._22 * p[1] + v._32 * p[2] + v._42;
        float vz = v._13 * p[0] + v._23 * p[1] + v._33 * p[2] + v._43;
        double d2 = (double)(vy * vy + vx * vx + vz * vz);
        if ((double)(s_FogStart * s_FogStart) <= d2) {
            if ((double)(s_FogEnd * s_FogEnd) <= d2) {
                *fog = 0;
                return true;
            }
            int a = (int)lrint((s_FogEnd - sqrt(d2)) * (255.0 / (s_FogEnd - s_FogStart)));
            *fog = (unsigned)a << 24;
            return true;
        }
    }
    *fog = 0xff000000u;
    return true;
}

} // namespace pz
