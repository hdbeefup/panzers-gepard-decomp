// 3dengine/glow.cpp
// Glow post-processing effect
// Decompiled from: gameSplit/sgepard.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <d3d9.h>

#include "glow.h"
#include "gepard.h"
#include "terrain.h"
#include "effects.h"
#include "properties.h"
#include "logger.h"
#include "hdbeefup.h"

// Classes: SGlow
// Function count: 5

// Local vertex struct for transformed+lit glow quads (28 bytes)
struct GlowVertex {
    float x;
    float y;
    float z;
    float rhw;
    unsigned int diffuse;
    float u;
    float v;
};

//----- (00446480) --------------------------------------------------------

SGlow::SGlow(char *classname, SEffect *effect, SGepard *gepard)
{
    this->data.array = 0;
    this->data.size = 0;
    this->data.maxsize = 0;
    strcpy(this->ClassName, classname);
    this->Effect = effect;
    this->Gepard = gepard;
    this->FirstTime = true;
    this->StopNow = false;

    int dt = effect->EffectsINI->GetInt(this->ClassName, "DrawType", -1);
    this->DrawType = (SDrawType)dt;
    if ( dt == -1 )
        Logger.g->Panic("SGlow::Initializations: A DrawType nincs megadva, vagy nulla van megadva.");

    char tmpstr[260];
    char *String = this->Effect->EffectsINI->GetString(this->ClassName, "Texture", "error");
    strcpy(tmpstr, String);
    if ( strcmp(tmpstr, "error") == 0 )
        Logger.g->Panic("SGlow::SGlow: cannot load texture in class: \"%s\"", this->ClassName);

    char fn[260];
    strcpy(fn, "effects\\");
    strcat(fn, tmpstr);

#ifdef HDB_MISSING_ASSET_FALLBACK
    int v15 = this->Gepard->LoadTextureOrPlaceholder(fn, 1, 1);
#else
    int v15 = this->Gepard->LoadTexture(fn, 1, 1);
#endif
    this->THandle = v15;
    if ( v15 == -1 )
        Logger.g->Panic("SGlow::SGlow: cannot load texture \"%s\"", fn);

    this->Darabszam = this->Effect->EffectsINI->GetInt(this->ClassName, "Darabszam", 5);
    this->Scale = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "Scale", 1.0);
    this->Variations = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "Variations", 1.0);
    float v22 = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "Variations", 1.0);
    this->sz = 0;
    this->RecVariations = 1.0f / v22;
}

//----- (00446700) --------------------------------------------------------

SGlow::~SGlow()
{
    this->Gepard->ReleaseTexture(this->THandle, 0);
    if ( this->data.array )
    {
        free(this->data.array);
        this->data.array = 0;
    }
}

//----- (00446760) --------------------------------------------------------

void SGlow::AddGlow(float x, float y, float z, int r, int g, int b, int scale, float fadeing)
{
    int v10 = 0;
    int size = this->data.size;
    if ( size > 0 )
    {
        for ( v10 = 0; v10 < size; ++v10 )
        {
            if ( !this->data.array[v10].active )
                break;
        }
    }
    if ( v10 >= size )
    {
        int maxsize = this->data.maxsize;
        v10 = size;
        if ( size == maxsize )
        {
            int v14;
            if ( maxsize >= 16 )
                v14 = 6 * maxsize / 5;
            else
                v14 = 16;
            SGlowData *v15 = (SGlowData *)realloc(this->data.array, sizeof(SGlowData) * v14);
            this->data.array = v15;
            memset(&v15[this->data.maxsize], 0, sizeof(SGlowData) * (v14 - this->data.maxsize));
            v10 = this->data.size;
            this->data.maxsize = v14;
        }
        this->data.size = v10 + 1;
    }
    this->data.array[v10].active = 1;
    this->data.array[v10].x = x;
    this->data.array[v10].y = y;
    this->data.array[v10].z = z;
    this->data.array[v10].whichtexture = 0;
    this->data.array[v10].r = r;
    this->data.array[v10].g = g;
    this->data.array[v10].b = b;
    this->data.array[v10].scale = scale;
    this->data.array[v10].needtofadeout = 0;
    this->data.array[v10].fadeing = fadeing;
}

//----- (00446870) --------------------------------------------------------

void SGlow::ClearGlow()
{
    for ( int i = 0; i < this->data.size; ++i )
    {
        if ( !this->data.array[i].needtofadeout )
            this->data.array[i].active = 0;
    }
}

//----- (004468A0) --------------------------------------------------------

char SGlow::DrawParticles()
{
    if ( this->StopNow )
        return 0;

    SGepard *Gepard = this->Gepard;
    IDirect3DDevice9 *lpD3DDev = Gepard->lpD3DDev;
    float dt = (float)((double)Gepard->ElapsedTime * 0.001);

    lpD3DDev->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    Gepard->SetDrawType(DT_ADD);
    Gepard->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 0);
    Gepard->SetTexture(0, this->THandle, 1);
    // x64: glow particles on snowy maps were rendering with wrong colors —
    // close-up stuck on blue, distance flashing red/yellow — because the
    // sampler / texture-stage state was being inherited from prior effects
    // (snow, terrain mip-filtered draws). The glow.tga is a grayscale star
    // and its DXT1 mip 0 is correct, but if MIPFILTER != NONE the GPU
    // samples lower mips at distance, and the texture-stage COLOROP /
    // COLORARG defaults differ between drivers. Force a clean known state.
    lpD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
    lpD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
    lpD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
    lpD3DDev->SetTextureStageState(0, D3DTSS_COLOROP,   D3DTOP_MODULATE);
    lpD3DDev->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
    lpD3DDev->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
    lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP,   D3DTOP_SELECTARG1);
    lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    lpD3DDev->SetTextureStageState(1, D3DTSS_COLOROP,   D3DTOP_DISABLE);
    lpD3DDev->SetTextureStageState(1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE);
#ifdef HDB_LIGHT_OCCLUSION
    Gepard->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
#else
    Gepard->lpD3DDev->SetRenderState(D3DRS_ZENABLE, 0);
#endif

    for ( int i = 0; i < this->data.size; ++i )
    {
        SGlowData *array = this->data.array;
        if ( array[i].active )
        {
            if ( array[i].needtofadeout )
            {
                array[i].fadeing = array[i].fadeing - dt;
                array = this->data.array;
                if ( array[i].fadeing <= 0.0f )
                {
                    array[i].fadeing = 0.0f;
                    this->data.array[i].active = 0;
                    array = this->data.array;
                }
            }

            float xpos, ypos, scale, zbuf;
            unsigned int fog;
            Gepard->TransformScaledPointAdd(
                array[i].x, array[i].y, array[i].z, this->Scale,
                &xpos, &ypos, &scale, &zbuf, &fog);

            SGlowData *v9 = this->data.array;
            float v10 = (float)(((double)(unsigned char)fog + 0.0) * 0.00196078431372549) * v9[i].fadeing;

            GlowVertex vert[4];
            vert[0].x = xpos - scale;
            vert[0].y = ypos - scale;
            vert[0].z = zbuf;
            vert[0].rhw = 1.0f;

            vert[1].x = xpos - scale;
            vert[1].y = ypos + scale;
            vert[1].z = zbuf;
            vert[1].rhw = 1.0f;

            vert[2].x = xpos + scale;
            vert[2].y = ypos - scale;
            vert[2].z = zbuf;
            vert[2].rhw = 1.0f;

            vert[3].x = xpos + scale;
            vert[3].y = ypos + scale;
            vert[3].z = zbuf;
            vert[3].rhw = 1.0f;

            int whichtexture = v9[i].whichtexture;
            float u0 = (float)whichtexture * this->RecVariations;
            float u1 = (float)(whichtexture + 1) * this->RecVariations;
            vert[0].u = u0;
            vert[1].u = u0;
            vert[2].u = u1;
            vert[3].u = u1;
            vert[0].v = 0.0f;
            vert[2].v = 0.0f;
            vert[1].v = 1.0f;
            vert[3].v = 1.0f;

            // x64: original IDA decomp packed ARGB via a chained
            //   v14 = ((uint)(g*v10) + ((uint)(r*v10) << 8)) << 8;
            //   diffuse = v14 + (uint)(b*v10);
            // The float→unsigned-int casts are undefined on x64 MSVC when
            // the float is out of [0, UINT_MAX] range (NaN, negative, large
            // accumulator). On snowy maps this fired with `v10` corrupted
            // — likely from a chain-unlink in SEffect::MoveEffects
            // following a failed Clone_ShaderLight (see Clone_ShaderLight
            // failures in the journal) that wrote into a neighbouring
            // SGlowData entry's `fadeing` slot. White (255,255,255) became
            // blue/yellow because the high-bit-set unsigned-int from
            // (uint)NaN landed in unintended channel positions after the
            // <<8 shifts.
            // Switch to a safe int-only ARGB pack with explicit clamping.
            SGlowData *v15 = this->data.array;
            SGepard *v16 = this->Gepard;
            float vv10 = v10;
            if (!(vv10 >= 0.0f)) vv10 = 0.0f;   // catches NaN
            if (vv10 > 1.0f) vv10 = 1.0f;
            int rr = (int)((float)v9[i].r * vv10);
            int gg = (int)((float)v9[i].g * vv10);
            int bb = (int)((float)v9[i].b * vv10);
            if (rr < 0) rr = 0; else if (rr > 255) rr = 255;
            if (gg < 0) gg = 0; else if (gg > 255) gg = 255;
            if (bb < 0) bb = 0; else if (bb > 255) bb = 255;
            vert[0].diffuse = ((unsigned int)rr << 16)
                            | ((unsigned int)gg << 8)
                            |  (unsigned int)bb;
            vert[1].diffuse = vert[0].diffuse;
            vert[2].diffuse = vert[0].diffuse;
            vert[3].diffuse = vert[0].diffuse;

            if ( v15[i].needtofadeout
              || (v16->Terrain == 0)
              || (v16->Terrain->VisMap == 0)
              || v16->Terrain->GodMode
              || v16->Terrain->VisMap[(int)(float)(v15[i].x + v15[i].x)
                 - 2 * (v16->Terrain->XSize + 1) * (int)(float)(v15[i].z * -2.0f)] )
            {
                v16->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(GlowVertex));
            }
        }
    }

#ifdef HDB_LIGHT_OCCLUSION
    this->Gepard->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 1);
#else
    this->Gepard->lpD3DDev->SetRenderState(D3DRS_ZENABLE, 1);
#endif
    Gepard->EnableFog();
    return 1;
}
