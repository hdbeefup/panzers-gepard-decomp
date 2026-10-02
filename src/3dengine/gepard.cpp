// 3dengine/gepard.cpp
// Main renderer (SGepard), materials, shaders
// Decompiled from: gameSplit/sgepard.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <d3dx9.h>

#include "gepard.h"
#include "effects.h"
#include "terrain.h"
#include "texture.h"

// Bridge to the editor debug picker overlay (defined in world.cpp). Resolved
// at link time when world is linked into editor.exe. The function is a no-op
// when the overlay toggle is off, so leaving the call in shipped builds is
// safe.
extern "C" void DrawDebugPickerOverlayFromGepard(IDirect3DDevice9 *dev);

// Verify critical struct offsets match the original binary (x86 Release only —
// _DEBUG adds extra fields that shift the layout; on non-x86 archs pointer-size
// changes legitimately shift offsets, so the shipped-binary offsets no longer apply)
#if !defined(_DEBUG) && defined(_M_IX86)
static_assert(offsetof(SGepard, CameraMatrix) == 0xB08, "CameraMatrix offset mismatch — SGepard layout is wrong");
static_assert(offsetof(SGepard, ProjectionMatrix) == 0xB48, "ProjectionMatrix offset mismatch — SGepard layout is wrong");
static_assert(offsetof(SGepard, Terrain) == 0x1044, "Terrain offset mismatch — SGepard layout is wrong");
static_assert(offsetof(SGepard, Interpolation) == 0xD90, "Interpolation offset mismatch — SGepard layout is wrong");
#endif

// Helper: round up to next power of 2
static unsigned int RoundUp2Pow2(unsigned int v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}
#include "group.h"
#include "mesh.h"
#include "glow.h"
#include "spline.h"
#include "logger.h"
#include "properties.h"
#include "core_common.h"
#include "string2.h"
#include "board.h"
#include "stream.h"
#include "timer.h"
#include <dxerr.h>
#include <stdlib.h>
#include <new>
#include <xmmintrin.h>
#include <emmintrin.h>
#include "hdbeefup.h"

#define LT_NORMAL 0
#define LT_AMBIENT 1
#define LT_PRELIT 2

// Local vertex type for snowfall rendering
struct TLVertSnow {
    float x, y, z, rhw;
    unsigned int diffuse;
    unsigned int specular;
    float u, v;
};

// Classes: SGepard, SMaterial, SShader, SGlow
// Function count: 296

// Static data
static int dword_59778C[50000]; // path Z coordinates buffer
static struct { int x; } path_buffer[50000]; // path X coordinates buffer
static unsigned char block_grid[400]; // lake block grid
static unsigned char vertex_grid[400]; // lake vertex grid
extern SIConcert *Concert;
extern SIBoard *Board;
extern STimer Timer;

//----- (00422300) --------------------------------------------------------

SSnowfall::SSnowfall(SGepard *gepard, SEffect *effect, char *classname)

{
  int v5;
  float *p_y; // esi
  int v7;
  int v8;
  int v9;
  double Float; // st7
  SEffect *v11; // ecx
  int geparda;
  this->Gepard = gepard;
  this->Effect = effect;
  v5 = gepard->LoadTexture("effects\\hopelyhek_a.tga", 1, 0);
  this->THandle = v5;
  if ( v5 == -1 )
    Logger.g->Panic("SSnowFall::SSnowFall: cannot load hopelyhek_a.tga!");
  p_y = &this->Hopelyhek[0][0].y;
  geparda = 100;
  do
  {
    v7 = 160;
    do
    {
      *(p_y - 1) = (float)((float)rand() / 32767.0f) * 8.0f;
      *p_y = (float)((float)rand() / 32767.0f) * 12.0f;
      p_y[1] = (float)((float)rand() / 32767.0f) * 8.0f;
      p_y[2] = (float)((float)rand() / 32767.0f) + 2.0f;
      *((_DWORD *)p_y + 3) = (int)(float)((float)((float)rand() * 0.000030517578) * 4.0);
      p_y[8] = (float)((float)((float)rand() / 32767.0f) * 0.2f) + 0.2f;
      p_y[9] = (float)((float)((float)rand() / 32767.0f) * 0.2f) + 0.2f;
      p_y[4] = (float)((float)rand() / 32767.0) * 6.2831855f;
      p_y[5] = (float)((float)rand() / 32767.0) * 6.2831855f;
      v8 = rand();
      p_y[6] = (float)((float)((float)v8 / 32767.0f) + (float)((float)v8 / 32767.0f)) + 2.0f;
      v9 = rand();
      p_y[7] = (float)((float)((float)v9 / 32767.0f) + (float)((float)v9 / 32767.0f)) + 2.0f;
      p_y += 11;
      --v7;
    }
    while ( v7 );
    --geparda;
  }
  while ( geparda );
  Float = this->Effect->EffectsINI->GetFloat(classname, "Variations", 1.0);
  v11 = this->Effect;
  this->Variations = (float)(Float);

  this->RecVariations = 1.0f / v11->EffectsINI->GetFloat(classname, "Variations", 1.0f);
}

//----- (00422C10) --------------------------------------------------------

SSnowfall::~SSnowfall()

{
  this->Gepard->ReleaseTexture(this->THandle, 0);
}

//----- (00425F20) --------------------------------------------------------

char SSnowfall::MoveSnow()

{
  SSnowfall *v1; // esi
  SGepard *Gepard; // ecx
  SGepard *v3; // edi
  int v4;
  SGepard *v5; // esi
  int v6;
  int ZSize;
  float v8; // xmm0_4
  float v9; // xmm2_4
  SSnowfall *v10; // edi
  float v11; // xmm3_4
  int v12;
  int v13;
  float *p_sinvaluex; // esi
  float z; // xmm1_4
  SGepard *v16; // ecx
  float x; // xmm0_4
  STerrain *Terrain; // ecx
  double Height; // st7
  float v20; // xmm0_4
  float v21; // xmm0_4
  float v22;
  SGepard *v23; // ecx
  float *p_y; // eax
  float v25; // xmm0_4
  int v26;
  float v27; // xmm1_4
  float v28; // xmm2_4
  SGepard *v29; // eax
  float v30; // xmm2_4
  STerrain *v31; // edx
  unsigned char *VisMap; // edi
  int v33;
  int v34;
  int v35;
  int v36;
  int XSize;
  float y;
  int v46;
  int v47;
  int v48;
  SSnowfall *v49;
  float v50;
  float v51;
  float v52;
  float v53;
  float scale;
  float rhw;
  int v56;
  float xpos;
  float ypos;
  float zbuf;
  unsigned int fog;
  float v61;
  float v62;
  TLVertSnow vert[4];
  v1 = this;
  v49 = this;
  Gepard = this->Gepard;
  v51 = (float)((double)Gepard->ElapsedTime * 0.001f);

  // x64 fix: SetLightingType(LT_AMBIENT) and SetAmbientGray(255) were never
  // restored on exit — they leak into the next effect's draw (SGlow,
  // SReflektor, etc.), where with D3DFVF_XYZRHW + DIFFUSE the residual
  // D3DRS_LIGHTING=1 + D3DRS_AMBIENT=0xFFFFFF mixes ambient into vertex
  // diffuse and shifts white(255,255,255) glows toward yellow/blue depending
  // on which texture-stage settings the preceding draw inherited. Surfaces
  // visibly only on snowy maps where MoveSnow runs each frame. Save here,
  // restore at function exit alongside the existing CULLMODE / ALPHAOP
  // restores.
  DWORD _saved_lighting = 0, _saved_ambient = 0;
  Gepard->lpD3DDev->GetRenderState(D3DRS_LIGHTING, &_saved_lighting);
  Gepard->lpD3DDev->GetRenderState(D3DRS_AMBIENT,  &_saved_ambient);

  Gepard->SetLightingType(LT_AMBIENT);
  v1->Gepard->SetDrawType(DT_NORMAL);
  v1->Gepard->SetTexture(0, v1->THandle, 1);
  v1->Gepard->lpD3DDev->SetFVF(452u);
  v1->Gepard->SetAmbientGray(255);
  v1->Gepard->lpD3DDev->SetRenderState(D3DRS_CULLMODE, 1u);
  v1->Gepard->lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, 4u);
  v3 = v1->Gepard;
  v4 = 0;
  v47 = 0;
  if ( v3->XSize / 8 > 0 )
  {
    v5 = v3;
    v48 = 0;
    do
    {
      v6 = 0;
      ZSize = v5->ZSize;
      v3 = v5;
      v56 = 0;
      if ( ZSize / 8 > 0 )
      {
        v46 = 0;
        do
        {
          v3 = v5;
          if ( v5->Terrain->Parcels[v4 + v6 * (v5->XSize / 8)].Visible )
          {
            { float _dx = v5->CameraXPos - (float)(v48 + 4);
              float _dz = v5->CameraZPos - (float)(v46 + 4);
              v8 = sqrtf(_dx * _dx + _dz * _dz); }
            if ( v8 < 40.0 )
            {
              v9 = (float)v48;
              v10 = v49;
              v61 = (float)v48;
              v11 = (float)v46;
              v62 = (float)v46;
              v12 = v6 + v47 / -10 - 10 * (v6 / 10);
              v13 = 160;
              p_sinvaluex = &v49->Hopelyhek[10 * v12][160 * v47].sinvaluex;
              do
              {
                z = *(p_sinvaluex - 3) + v11;
                *(p_sinvaluex - 4) = *(p_sinvaluex - 4) - (float)(*(p_sinvaluex - 2) * v51);
                v16 = v10->Gepard;
                x = *(p_sinvaluex - 5) + v9;
                v53 = z;
                Terrain = v16->Terrain;
                v50 = x;
                Height = Terrain->GetHeight(x, z);
                v20 = (float)(v51 * p_sinvaluex[2]) + *p_sinvaluex;
                y = (float)(Height + *(p_sinvaluex - 4));

                *p_sinvaluex = v20;
                v21 = sinf(v20);
                v52 = (float)(v21 * p_sinvaluex[4]) + v50;
                p_sinvaluex[1] = (float)(p_sinvaluex[3] * v51) + p_sinvaluex[1];
                v22 = sinf(v20);
                v23 = v10->Gepard;
                *(float *)&v22 = v22;
                v50 = (float)(*(float *)&v22 * p_sinvaluex[5]) + v53;
 v23->TransformScaledPointBlend(v52, y, v50, 0.050000001f, &xpos, &ypos, &scale, &zbuf, &rhw, &fog);
                if ( scale != 0.0 && rhw <= 0.125 )
                {
                  p_y = &y;
                  v53 = 1.0;
                  y = (float)((float)(1.0f / rhw) - 8.0f) * 0.5f;
                  if ( y >= 1.0 )
                    p_y = &v53;
                  v25 = *p_y;
                  if ( *p_y <= 0.0 )
                    v25 = 0.0;
                  *(unsigned int*)&y = (unsigned int)(float)(v25 * 255.0) << 24;
                  v26 = *((_DWORD *)p_sinvaluex - 1);
                  vert[0].v = 0.0;
                  vert[0].x = xpos - scale;
                  vert[1].x = xpos - scale;
                  vert[2].x = xpos + scale;
                  vert[3].x = xpos + scale;
                  vert[2].v = 0.0;
                  vert[1].v = 1.0;
                  vert[3].v = 1.0;
                  vert[1].y = ypos + scale;
                  vert[3].y = ypos + scale;
                  v27 = (float)v26 * v10->RecVariations;
                  vert[3].specular = fog;
                  v28 = (float)(v26 + 1);
                  vert[2].specular = fog;
                  vert[1].specular = fog;
                  vert[0].specular = fog;
                  v29 = v10->Gepard;
                  vert[0].y = ypos - scale;
                  v30 = v28 * v10->RecVariations;
                  vert[2].y = ypos - scale;
                  vert[3].z = zbuf;
                  vert[2].z = zbuf;
                  vert[1].z = zbuf;
                  vert[0].z = zbuf;
                  vert[0].u = v27;
                  vert[1].u = v27;
                  vert[2].u = v30;
                  vert[3].u = v30;
                  vert[3].rhw = rhw;
                  vert[2].rhw = rhw;
                  vert[1].rhw = rhw;
                  vert[0].rhw = rhw;
                  v31 = v29->Terrain;
                  VisMap = v31->VisMap;
                  if ( v29->Glow )
                  {
                    if ( !VisMap
                      || v31->GodMode
                      || VisMap[(int)(float)(v52 + v52) - 2 * (int)(float)(v50 * -2.0) * (v31->XSize + 1)]
                      || v31->FogMode != 1 )
                    {
                      v33 = 13158600;
                    }
                    else
                    {
                      v33 = 7895160;
                    }
                  }
                  else if ( !VisMap
                         || v31->GodMode
                         || VisMap[(int)(float)(v52 + v52) - 2 * (int)(float)(v50 * -2.0) * (v31->XSize + 1)]
                         || (v33 = 12500670, v31->FogMode != 1) )
                  {
                    v33 = 0xFFFFFF;
                  }
                  v10 = v49;
                  {
                    unsigned int alpha_color = (fog & 0xFF000000) | v33;
                    vert[3].diffuse = alpha_color;
                    vert[2].diffuse = alpha_color;
                    vert[1].diffuse = alpha_color;
                    vert[0].diffuse = alpha_color;
                  }
                  v49->Gepard->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2u, vert, 32u);
                  if ( *(p_sinvaluex - 4) + 0.1 < 0.0 )
                  {
                    v34 = rand();
                    *(p_sinvaluex - 4) = 12.0;
                    *(p_sinvaluex - 5) = (float)((float)v34 / 32767.0f) * 8.0f;
                    v35 = rand();
                    v10 = v49;
                    *(p_sinvaluex - 3) = (float)((float)v35 / 32767.0f) * 8.0f;
                  }
                }
                v9 = v61;
                p_sinvaluex += 11;
                v11 = v62;
                --v13;
              }
              while ( v13 );
              v3 = v10->Gepard;
              v6 = v56;
            }
          }
          v36 = v3->ZSize;
          ++v6;
          v46 += 8;
          v5 = v3;
          v4 = v47;
          v56 = v6;
        }
        while ( v6 < v36 / 8 );
      }
      XSize = v3->XSize;
      ++v4;
      v48 += 8;
      v5 = v3;
      v47 = v4;
    }
    while ( v4 < XSize / 8 );
    v1 = v49;
  }
  v3->lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, 2u);
  v1->Gepard->lpD3DDev->SetRenderState(D3DRS_CULLMODE, 2u);
  v1->Gepard->lpD3DDev->SetRenderState(D3DRS_LIGHTING, _saved_lighting);
  v1->Gepard->lpD3DDev->SetRenderState(D3DRS_AMBIENT,  _saved_ambient);
  return 1;
}

//----- (004265F0) --------------------------------------------------------

char SWindowEffect::MoveWindows()

{
  float v3; // xmm1_4
  float v4; // xmm4_4
  float v5; // xmm0_4
  int v6;
  SDArray<SAblak> *Ablakok; // ebx
  int AblakCounter;
  int v9;
  SAblak *array; // edx
  int type;
  SDArray<SAblak> *v12; // eax
  char v13; // cl
  int v14;
  float v15; // xmm6_4
  int v16;
  float v17; // xmm7_4
  SAblak *v18; // ecx
  float param; // xmm5_4
  float variable; // xmm3_4
  float v21; // xmm2_4
  float v22; // xmm0_4
  float v23; // xmm4_4
  float v24; // xmm0_4
  SAblak *v25; // eax
  float v26; // xmm0_4
  float v27; // xmm4_4
  float v28; // xmm0_4
  SAblak *v29; // eax
  float v30;
  char v31;
  if ( !this->Object )
    return 0;
  v3 = 0.0;
  v4 = (float)((double)this->Gepard->ElapsedTime * 0.001f);

  v30 = v4;
  if ( this->AblakCounter < this->Ablakok->size )
  {
    v5 = this->NextRandom - v4;
    this->NextRandom = v5;
    if ( v5 <= 0.0 )
    {
      v6 = rand();
      Ablakok = this->Ablakok;
      AblakCounter = this->AblakCounter;
      v3 = 0.0;
      this->NextRandom = (float)((float)v6 / 32767.0f) + 0.5f;
      v9 = (Ablakok->array[AblakCounter].type != 1) + 1 + AblakCounter;
      this->AblakCounter = v9;
      if ( v9 < Ablakok->size )
      {
        array = Ablakok->array;
        type = array[v9].type;
        array[v9].active = 1;
        if ( type != 1 )
          this->Ablakok->array[this->AblakCounter + 1].active = 1;
      }
    }
  }
  v12 = this->Ablakok;
  v13 = 0;
  v14 = 0;
  v31 = 0;
  if ( v12->size > 0 )
  {
    v15 = (-1.0f);
    v16 = 0;
    v17 = 1.0f;
    do
    {
      v18 = v12->array;
      if ( v18[v16].active )
      {
        param = v18[v16].param;
        variable = v18[v16].variable;
        v31 = 1;
        v21 = param - variable;
        if ( v18[v16].type == 1 )
        {
          if ( v21 >= 0.0 )
          {
            if ( v21 <= 0.0 )
              v22 = 0.0;
            else
              v22 = v17;
          }
          else
          {
            v22 = v15;
          }
          v23 = (float)(v22 * v4) * 0.8f;
          if ( (float)(param - (float)(v23 + variable)) >= 0.0 )
          {
            if ( (float)(param - (float)(v23 + variable)) <= 0.0 )
              v24 = 0.0;
            else
              v24 = v17;
          }
          else
          {
            v24 = v15;
          }
          if ( v21 >= 0.0 )
          {
            if ( v21 > 0.0 )
              v3 = v17;
          }
          else
          {
            v3 = v15;
          }
          v18[v16].active = v24 == v3;
          v25 = this->Ablakok->array;
          if ( v25[v16].active )
            v25[v16].variable = v25[v16].variable + v23;
          else
            v25[v16].variable = param;
          this->Object->SetMeshProperties(this->Ablakok->array[v16].meshidx,
            0,
            0,
            this->Ablakok->array[v16].variable,
            0,
            0,
            0);
        }
        else
        {
          if ( v21 >= 0.0 )
          {
            if ( v21 <= 0.0 )
              v26 = 0.0;
            else
              v26 = v17;
          }
          else
          {
            v26 = v15;
          }
          v27 = (float)(v26 * v4) * 0.8f;
          if ( (float)(param - (float)(v27 + variable)) >= 0.0 )
          {
            if ( (float)(param - (float)(v27 + variable)) <= 0.0 )
              v28 = 0.0;
            else
              v28 = v17;
          }
          else
          {
            v28 = v15;
          }
          if ( v21 >= 0.0 )
          {
            if ( v21 > 0.0 )
              v3 = v17;
          }
          else
          {
            v3 = v15;
          }
          v18[v16].active = v28 == v3;
          v29 = this->Ablakok->array;
          if ( v29[v16].active )
            v29[v16].variable = v29[v16].variable + v27;
          else
            v29[v16].variable = param;
          this->Object->SetMeshProperties(this->Ablakok->array[v16].meshidx,
            0,
            0,
            0,
            this->Ablakok->array[v16].variable,
            0,
            0);
        }
        v17 = 1.0f;
        v15 = (-1.0f);
      }
      v12 = this->Ablakok;
      ++v14;
      v4 = v30;
      ++v16;
      v3 = 0.0;
    }
    while ( v14 < v12->size );
    return v31;
  }
  return v13;
}

//----- (00428720) --------------------------------------------------------

SGepard::SGepard()

{
  SProperties *v2; // eax
  this->BoardVisible = 1;
  this->HeightFrom = 1;
  this->ShaderInfos.first = 0;
  this->ShaderInfos.last = 0;
  this->ShaderInfos.current = 0;
  this->ShaderInfos.Closed = 0;
  this->ShaderInfos.NumItems = 0;
  this->Shader2Infos.first = 0;
  this->Shader2Infos.last = 0;
  this->Shader2Infos.current = 0;
  this->Shader2Infos.Closed = 0;
  this->Shader2Infos.NumItems = 0;
  this->SelectedShader = 0;
  this->ShaderFolders.first = 0;
  this->ShaderFolders.last = 0;
  this->ShaderFolders.current = 0;
  this->ShaderFolders.Closed = 0;
  this->ShaderFolders.NumItems = 0;
  this->GlobalHoleList.first = 0;
  this->GlobalHoleList.last = 0;
  this->GlobalHoleList.current = 0;
  this->GlobalHoleList.Closed = 0;
  this->GlobalHoleList.NumItems = 0;
  this->NodeInfos.first = 0;
  this->NodeInfos.last = 0;
  this->NodeInfos.current = 0;
  this->NodeInfos.Closed = 0;
  this->NodeInfos.NumItems = 0;
  this->gpSelHillRing = 0;
  this->gpSelSplineRing = 0;
  this->HillChain.first = 0;
  this->HillChain.last = 0;
  this->HillChain.current = 0;
  this->HillChain.Closed = 0;
  this->HillChain.NumItems = 0;
  this->SplineDisplay = 0;
  this->Editor_LastTime = 0;
  this->RefCount = 1;
  this->ShadowMapTexture.Ptr = 0;
  this->ShadowMapDepthTexture.Ptr = 0;
  this->ShadowMapWidth = 2048;
  this->ShadowDistance = 40.0;
  this->Glow = 0;
  this->ScreenshotFile = 0;
  this->Board = 0;
  this->FogColor = 0;
  this->FrameCount = 0;
  this->RenderCounter = 1;
  this->WorldTime = 0;
  this->LastWorldTime = 0;
  this->AnimTime = 0;
  this->AnimElapsedTime = 0;
  this->Interpolation = 0.0;
  this->shadowCB.Vector4fCount = 4;
  this->lightCB.Vector4fCount = 5;
  this->ObjectCache.array = 0;
  this->ObjectCache.size = 0;
  this->ObjectCache.maxsize = 0;
  this->ObjectCache.nextempty = -1;
  this->ObjectCache.occupied = 0;
  this->StaticObjects.array = 0;
  this->StaticObjects.size = 0;
  this->StaticObjects.maxsize = 0;
  this->StaticObjects.nextempty = -1;
  this->StaticObjects.occupied = 0;
  this->DynamicObjects.array = 0;
  this->DynamicObjects.size = 0;
  this->DynamicObjects.maxsize = 0;
  this->DynamicObjects.nextempty = -1;
  this->DynamicObjects.occupied = 0;
  this->ObjectHashChain.array = 0;
  this->ObjectHashChain.size = 0;
  this->ObjectHashChain.maxsize = 0;
  this->ShadowQuality = 2;
  this->MaxShadowQuality = 1;
  this->ShadowRenderTarget = 0;
  this->ShadowRenderTargetZ = 0;
  this->Terrain = 0;
  this->debugLines.array = 0;
  this->debugLines.count = 0;
  this->debugLines.capacity = 0;
  for (int i = 0; i < 7; i++) this->standardVertexShaders[i].Ptr = 0;
  for (int i = 0; i < 7; i++) this->standardPixelShaders[i].Ptr = 0;
  for (int i = 0; i < 8; i++) this->depthWriteVertexShaders[i].Ptr = 0;
  for (int i = 0; i < 8; i++) this->depthWritePixelShaders[i].Ptr = 0;
  for (int i = 0; i < 2; i++) this->terrainVertexShader[i].Ptr = 0;
  for (int i = 0; i < 2; i++) this->terrainPixelShader[i].Ptr = 0;
  /* wireframePS removed — not in original binary */
  this->terrainLightingVertexShader.Ptr = 0;
  this->ambientLitVertexShader.Ptr = 0;
  this->decalVertexShader.Ptr = 0;
  this->unlitDecalVertexShader.Ptr = 0;
  this->sepiaPixelShader.Ptr = 0;
  this->terrainLightingPixelShader.Ptr = 0;
  this->ambientLitPixelShader.Ptr = 0;
  this->decalPixelShader.Ptr = 0;
  this->unlitDecalPixelShader.Ptr = 0;
  this->Textures.array = 0;
  this->Textures.size = 0;
  this->Textures.maxsize = 0;
  this->Textures.nextempty = -1;
  this->Textures.occupied = 0;
  this->DrawType = DT_NORMAL;
  this->DynamicShaders.array = 0;
  this->DynamicShaders.size = 0;
  this->DynamicShaders.maxsize = 0;
  this->DynamicShaders.nextempty = -1;
  this->DynamicShaders.occupied = 0;
  this->Rects.array = 0;
  this->Rects.size = 0;
  this->Rects.maxsize = 0;
  this->Rects.nextempty = -1;
  this->Rects.occupied = 0;
  this->SmokeTrails.array = 0;
  this->SmokeTrails.size = 0;
  this->SmokeTrails.maxsize = 0;
  this->SmokeTrails.nextempty = -1;
  this->SmokeTrails.occupied = 0;
  this->GroundTrails.array = 0;
  this->GroundTrails.size = 0;
  this->GroundTrails.maxsize = 0;
  this->GroundTrails.nextempty = -1;
  this->GroundTrails.occupied = 0;
  this->Lakes.array = 0;
  this->Lakes.size = 0;
  this->Lakes.maxsize = 0;
  this->Lakes.nextempty = -1;
  this->Lakes.occupied = 0;
  this->DynamicVBs.array = 0;
  this->DynamicVBs.size = 0;
  this->DynamicVBs.maxsize = 0;
  this->DynamicVBs.nextempty = -1;
  this->DynamicVBs.occupied = 0;
  this->Effect = 0;
  this->FullBright = 0;
  this->FirstDrawWater = 0;
  this->ResolutionScaleSurface.Ptr = 0;
  this->ResolutionScaleDepthSurface.Ptr = 0;
  this->Flags[0] = true;
  this->Flags[1] = false;
  #ifdef HD_DEBUG_RENDERER
  Logger.g->Log(0, "SGepard ctor: Flags[0]=%d Flags[1]=%d", (int)this->Flags[0], (int)this->Flags[1]);
  #endif
  this->DebugMode = 0;
  this->ResolutionScale = 1.0;
  this->PreResetCallback = nullptr;
  this->PostResetCallback = nullptr;
  this->ResetCallbackUserData = nullptr;
  this->_folyo = -1;
  this->_folyo_csillogas = -1;
  this->_gray_texture = -1;
  this->FPSTextFrame = -1;
  this->DebugTextFrame = -1;
  this->DebugInfoFrame = -1;
  this->DebugMouseX = 0.0f;
  this->DebugMouseY = 0.0f;
  this->DebugMouseZ = 0.0f;
#ifdef _DEBUG
  // Debug-game build: enable extended overlay (Textures/Tiles/CameraDir/
  // mouse XYZ) so the "show fps" cheat creates DebugInfoFrame and the
  // RenderScene gate (line ~11333) paints. DebugMouseX/Y/Z is also under
  // _DEBUG in SGameView::MouseUpdate. Release game leaves this false.
  this->ShowDebugInfo = true;
#else
  this->ShowDebugInfo = false;
#endif
  this->HoverShader = nullptr;
  this->EditorPreviewGhost = 0;
  this->EditorPreviewGhostX = 0.0f;
  this->EditorPreviewGhostY = 0.0f;
  this->EditorPreviewGhostZ = 0.0f;
  this->lpD3DDev = 0;
  this->lpD3D = 0;
  this->IdentityMatrix._43 = 0.0;
  this->IdentityMatrix._42 = 0.0;
  this->IdentityMatrix._41 = 0.0;
  this->IdentityMatrix._34 = 0.0;
  this->IdentityMatrix._32 = 0.0;
  this->IdentityMatrix._31 = 0.0;
  this->IdentityMatrix._24 = 0.0;
  this->IdentityMatrix._23 = 0.0;
  this->IdentityMatrix._21 = 0.0;
  this->IdentityMatrix._14 = 0.0;
  this->IdentityMatrix._13 = 0.0;
  this->IdentityMatrix._12 = 0.0;
  this->IdentityMatrix._44 = 1.0;
  this->IdentityMatrix._33 = 1.0;
  this->IdentityMatrix._22 = 1.0;
  this->IdentityMatrix._11 = 1.0;
  v2 = new SProperties("decals.ini", 1, 1);
  this->DecalsIni = v2;
}

//----- (00428E30) --------------------------------------------------------

SMaterial::SMaterial()

{
  this->Specular.r = 0.0;
  this->Specular.g = 0.0;
  this->Specular.b = 0.0;
  this->Specular.a = 0.0;
  this->Emissive.r = 0.0;
  this->Emissive.g = 0.0;
  this->Emissive.b = 0.0;
  this->Emissive.a = 0.0;
  this->Power = 0.0;
  this->Diffuse.r = 1.0;
  this->Diffuse.g = 1.0;
  this->Diffuse.b = 1.0;
  this->Diffuse.a = 1.0;
  this->Ambient.r = 1.0;
  this->Ambient.g = 1.0;
  this->Ambient.b = 1.0;
  this->Ambient.a = 1.0;
}

//----- (00428EB0) --------------------------------------------------------

// Array<SGepardDebugLine> destructor — inlined from template
// void Array<SGepardDebugLine>::~Array() { count = 0; if (array) free(array); }

//----- (00429210) --------------------------------------------------------

SGepard::~SGepard()

{
  IDirect3DSurface9 *Ptr; // ecx
  IDirect3DSurface9 *v3; // ecx
  IDirect3DTexture9 *v4; // ecx
  IDirect3DTexture9 *v5; // ecx
  SBoard *Board; // edi
  int v7;
  int size;
  SHeap<SObjectCacheProp>::__Tstruct *v9; // eax
  SGroup *node1; // ecx
  SGroup *node2; // ecx
  IDirect3DDevice9 *lpD3DDev; // ecx
  IDirect3D9 *lpD3D; // ecx
  SProperties *DecalsIni; // edi
  IDirect3DSurface9 *v15; // ecx
  IDirect3DSurface9 *v16; // ecx
  IDirect3DPixelShader9 *v17; // ecx
  IDirect3DPixelShader9 *v18; // ecx
  IDirect3DPixelShader9 *v19; // ecx
  IDirect3DPixelShader9 *v20; // ecx
  IDirect3DPixelShader9 *v21; // ecx
  IDirect3DVertexShader9 *v22; // ecx
  IDirect3DVertexShader9 *v23; // ecx
  IDirect3DVertexShader9 *v24; // ecx
  IDirect3DVertexShader9 *v25; // ecx
  SGepardDebugLine *v26; // eax
  IDirect3DTexture9 *v27; // ecx
  IDirect3DTexture9 *v28; // ecx
  SHeap<SObjectCacheProp>::__Tstruct *array; // eax
  SGroup *Group; // ecx
  char *FileName; // eax
  SHeap<SObjectCacheProp>::__Tstruct *v32; // ecx
  int folyo;
  folyo = this->_folyo;
  this->ReleaseTexture(folyo, 0);
  this->ReleaseTexture(this->_folyo_csillogas, 0);
  this->ReleaseTexture(this->_gray_texture, 0);
  Ptr = this->ResolutionScaleSurface.Ptr;
  if ( Ptr )
  {
    Ptr->Release();
    this->ResolutionScaleSurface.Ptr = 0;
  }
  v3 = this->ResolutionScaleDepthSurface.Ptr;
  if ( v3 )
  {
    v3->Release();
    this->ResolutionScaleDepthSurface.Ptr = 0;
  }
  v4 = this->ShadowMapTexture.Ptr;
  if ( v4 )
  {
    v4->Release();
    this->ShadowMapTexture.Ptr = 0;
  }
  v5 = this->ShadowMapDepthTexture.Ptr;
  if ( v5 )
  {
    v5->Release();
    this->ShadowMapDepthTexture.Ptr = 0;
  }
  this->ClearObjectShadows(1, 0);
  if ( this->FPSTextFrame >= 0 )
    this->Board->DestroyFrame(this->FPSTextFrame);
  if ( this->DebugTextFrame >= 0 )
    this->Board->DestroyFrame(this->DebugTextFrame);
  Board = this->Board;
  if ( Board )
  {
    delete this->Board;
    this->Board = 0;
  }
  ::Board = 0;
  this->ShaderFolders.DeleteAll();
  if ( this->RefCount )
    Logger.g->Panic("SGepard::~SGepard: Object deleted instead of release");
  v7 = -1;
  while ( 1 )
  {
    size = this->ObjectCache.size;
    if ( ++v7 >= size )
      break;
    v9 = &this->ObjectCache.array[v7];
    while ( v9->use != 0x7FFFFFFF )
    {
      ++v7;
      ++v9;
      if ( v7 >= size )
        goto LABEL_21;
    }
    if ( v7 < 0 )
      break;
    array = this->ObjectCache.array;
    if ( !array[v7].data.RefCount )
    {
      Group = array[v7].data.Group;
      if ( Group )
      {
        Group->Release();
        this->ObjectCache.array[v7].data.Group = 0;
        array = this->ObjectCache.array;
      }
      FileName = array[v7].data.FileName;
      if ( FileName )
      {
        ::operator delete(FileName);
        this->ObjectCache.array[v7].data.FileName = 0;
      }
      if ( v7 >= this->ObjectCache.size || (v32 = this->ObjectCache.array, v32[v7].use != 0x7FFFFFFF) )
        Logger.g->Panic("SHeap::Remove: invalid index (%d)", v7);
      v32[v7].use = this->ObjectCache.nextempty;
      --this->ObjectCache.occupied;
      this->ObjectCache.nextempty = v7;
    }
  }
LABEL_21:
  node1 = this->node1;
  if ( node1 )
  {
    node1->Release();
    this->node1 = 0;
  }
  node2 = this->node2;
  if ( node2 )
  {
    node2->Release();
    this->node2 = 0;
  }
  this->ClearShaders();
  lpD3DDev = this->lpD3DDev;
  if ( lpD3DDev )
  {
    lpD3DDev->Release();
    this->lpD3DDev = 0;
  }
  lpD3D = this->lpD3D;
  if ( lpD3D )
  {
    lpD3D->Release();
    this->lpD3D = 0;
  }
  DecalsIni = this->DecalsIni;
  if ( DecalsIni )
  {
    delete DecalsIni;
    this->DecalsIni = 0;
  }
  v15 = this->ResolutionScaleDepthSurface.Ptr;
  if ( v15 )
    v15->Release();
  v16 = this->ResolutionScaleSurface.Ptr;
  if ( v16 )
    v16->Release();
  if ( this->DynamicVBs.array )
    free(this->DynamicVBs.array);
  this->Lakes.~SHeap<SGLake>();
  if ( this->GroundTrails.array )
    free(this->GroundTrails.array);
  if ( this->SmokeTrails.array )
    free(this->SmokeTrails.array);
  if ( this->Rects.array )
    free(this->Rects.array);
  if ( this->DynamicShaders.array )
    free(this->DynamicShaders.array);
  if ( this->Textures.array )
    free(this->Textures.array);
  v17 = this->unlitDecalPixelShader.Ptr;
  if ( v17 )
    v17->Release();
  v18 = this->decalPixelShader.Ptr;
  if ( v18 )
    v18->Release();
  v19 = this->ambientLitPixelShader.Ptr;
  if ( v19 )
    v19->Release();
  v20 = this->terrainLightingPixelShader.Ptr;
  if ( v20 )
    v20->Release();
  v21 = this->sepiaPixelShader.Ptr;
  if ( v21 )
    v21->Release();
  v22 = this->unlitDecalVertexShader.Ptr;
  if ( v22 )
    v22->Release();
  v23 = this->decalVertexShader.Ptr;
  if ( v23 )
    v23->Release();
  v24 = this->ambientLitVertexShader.Ptr;
  if ( v24 )
    v24->Release();
  v25 = this->terrainLightingVertexShader.Ptr;
  if ( v25 )
    v25->Release();
  for (int i = 1; i >= 0; i--) { if (this->terrainPixelShader[i].Ptr) this->terrainPixelShader[i].Ptr->Release(); }
  for (int i = 1; i >= 0; i--) { if (this->terrainVertexShader[i].Ptr) this->terrainVertexShader[i].Ptr->Release(); }
  for (int i = 7; i >= 0; i--) { if (this->depthWritePixelShaders[i].Ptr) this->depthWritePixelShaders[i].Ptr->Release(); }
  for (int i = 7; i >= 0; i--) { if (this->depthWriteVertexShaders[i].Ptr) this->depthWriteVertexShaders[i].Ptr->Release(); }
  for (int i = 6; i >= 0; i--) { if (this->standardPixelShaders[i].Ptr) this->standardPixelShaders[i].Ptr->Release(); }
  for (int i = 6; i >= 0; i--) { if (this->standardVertexShaders[i].Ptr) this->standardVertexShaders[i].Ptr->Release(); }
  v26 = this->debugLines.array;
  this->debugLines.count = 0;
  if ( v26 )
    free(v26);
  if ( this->ObjectHashChain.array )
  {
    free(this->ObjectHashChain.array);
    this->ObjectHashChain.array = 0;
  }
  if ( this->DynamicObjects.array )
    free(this->DynamicObjects.array);
  if ( this->StaticObjects.array )
    free(this->StaticObjects.array);
  if ( this->ObjectCache.array )
    free(this->ObjectCache.array);
  v27 = this->ShadowMapDepthTexture.Ptr;
  if ( v27 )
    v27->Release();
  v28 = this->ShadowMapTexture.Ptr;
  if ( v28 )
    v28->Release();
  this->HillChain.~SChain<SHillRing>();
  this->NodeInfos.DeleteAll();
  this->GlobalHoleList.DeleteAll();
  this->ShaderFolders.DeleteAll();
  this->Shader2Infos.~SChain<SShader2Info>();
  this->ShaderInfos.DeleteAll();
}

//----- (00429A40) --------------------------------------------------------

void SGepard::AddDebugLine(SVector start, SVector end, unsigned int color, unsigned int owner)

{
  unsigned int capacity;
  unsigned int count;
  unsigned int v8;
  SGepardDebugLine *array; // edi
  unsigned int v10;
  int v11;
  SGepardDebugLine *v12; // eax
  unsigned int v13;
  unsigned int v14;
  _BYTE v15[32];
  *(SVector *)v15 = start;
  *(SVector *)&v15[12] = end;
  { unsigned int _tmp[] = { color, owner }; memcpy(&v15[24] , _tmp, 8); }
  capacity = this->debugLines.capacity;
  count = this->debugLines.count;
  if ( count < capacity )
  {
    array = this->debugLines.array;
    goto LABEL_14;
  }
  if ( !capacity )
  {
    v8 = 1;
    goto LABEL_6;
  }
  v8 = 2 * capacity;
  if ( v8 )
  {
LABEL_6:
    array = (SGepardDebugLine *)malloc(32 * v8);
    count = this->debugLines.count;
    goto LABEL_7;
  }
  array = 0;
LABEL_7:
  v10 = 0;
  if ( count )
  {
    v11 = 0;
    do
    {
      v12 = this->debugLines.array;
      ++v11;
      ++v10;
      memcpy(&array[v11 - 1].start.x , &v12[v11 - 1].start.x, 16);
      memcpy(&array[v11 - 1].end.y , &v12[v11 - 1].end.y, 16);
    }
    while ( v10 < this->debugLines.count );
  }
  if ( this->debugLines.array )
    free(this->debugLines.array);
  this->debugLines.array = array;
  this->debugLines.capacity = v8;
LABEL_14:
  v13 = this->debugLines.count;
  v14 = v13 + 1;
  v13 *= 32;
  this->debugLines.count = v14;
  memcpy(((char *)&array->start.x + v13) , v15, 16);
  memcpy(((char *)&array->end.y + v13) , &v15[16], 16);
}

//----- (00429B40) --------------------------------------------------------

void SGepard::AddGlow(float x, float y, float z, int r, int g, int b, int scale, float fadeing)

{
  SEffect *Effect; // eax
  Effect = this->Effect;
  if ( Effect )
    Effect->AddGlow((SGlow *)this->Glow, x, y, z, r, g, b, scale, fadeing);
}

//----- (00429BA0) --------------------------------------------------------

void SGepard::AddNewSplineNode(SHillRing *_hillring, float x, float y, float z)

{
  SChain<SSplineRing> *i; // eax
  SChain<SSplineRing> *SplineChain; // ecx
  SSplineRing *current; // eax
  SSplineRing *next; // eax
  // SSpline::RefreshSubNodes asserts each ControlPoint's (x,z) lies on the
  // integer parcel grid (within 0.02f). The editor's terrain raycast returns
  // free-floating coords, so snap here before the node enters the chain.
  x = floorf(x + 0.5f);
  z = floorf(z + 0.5f);
  _hillring->SplineChain->current = _hillring->SplineChain->first;
  for ( i = _hillring->SplineChain; i->current; i = _hillring->SplineChain )
  {
    i->current->spline->AddNode(x, y, z, 0);
    SplineChain = _hillring->SplineChain;
    current = SplineChain->current;
    if ( SplineChain->Closed )
    {
      if ( !current )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      next = current->next;
      if ( !next )
        next = SplineChain->first;
    }
    else if ( current )
    {
      next = current->next;
    }
    else
    {
      next = 0;
    }
    SplineChain->current = next;
  }
  this->RefreshHill(_hillring);
}

//----- (00429C50) --------------------------------------------------------

void SGepard::AddNodesFromSplineNodes(SChain<SNode> *snodechain, SChain<SNodeInfo2> *nodechain, float x1, float x2, float z1, float z2)

{
  SNode *first; // eax
  long long v8; // xmm4_8
  float v9; // xmm5_4
  float v10; // xmm6_4
  float v11; // xmm7_4
  float v12; // xmm1_4
  float x; // xmm3_4
  float z; // xmm2_4
  bool v15; // al
  float v16; // xmm0_4
  float v17; // xmm0_4
  float v18; // xmm0_4
  SNodeInfo2 *v19; // esi
  SNodeInfo2 *last; // eax
  SNode *current; // eax
  first = snodechain->first;
  snodechain->current = snodechain->first;
  if ( first )
  {
    v8 = 0x80000000u;
    v9 = z1;
    v10 = x2;
    v11 = x1;
    v12 = 0.001f;
    do
    {
      x = first->x;
      z = first->z;
      v15 = 0;
      v16 = fabsf(x);
      if ( v12 > v16 && z > (float)(v9 - v12) )
        v15 = (float)(z2 + v12) > z;
      v17 = fabsf(x - v10);
      if ( v12 > v17 && z > (float)(v9 - v12) && (float)(z2 + v12) > z )
        v15 = 1;
      v18 = fabsf(z - v9);
      if ( v12 > v18 && x > (float)(v11 - v12) && (float)(v10 + v12) > x )
        v15 = 1;
      if ( v12 > fabsf(z - z2)
        && x > (float)(v11 - v12)
        && (float)(v10 + v12) > x
        || v15 )
      {
        v19 = (SNodeInfo2 *)operator new(sizeof(SNodeInfo2));
        memset(&v19->prev , 0, 16);
        memset(&v19->z , 0, 8);
        last = nodechain->last;
        if ( last )
        {
          last->next = v19;
          v19->prev = nodechain->last;
          nodechain->last = v19;
          v19->next = 0;
        }
        else
        {
          nodechain->last = v19;
          nodechain->first = v19;
        }
        ++nodechain->NumItems;
        v19->x = x;
        v12 = 0.001f;
        v8 = 0x80000000u;
        v9 = z1;
        v10 = x2;
        v11 = x1;
        v19->y = (float)(this->Terrain->GetHeight(x, z));

        v19->z = z;
      }
      current = snodechain->current;
      if ( snodechain->Closed )
      {
        if ( !current )
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        first = current->next;
        if ( !first )
          first = snodechain->first;
      }
      else if ( current )
      {
        first = current->next;
      }
      else
      {
        first = 0;
      }
      snodechain->current = first;
    }
    while ( first );
  }
}

//----- (00429E80) --------------------------------------------------------

void SGepard::AddRef()

{
  ++this->RefCount;
}

//----- (00429E90) --------------------------------------------------------

int SGepard::AddRefTexture(int idx)

{
  int result;
  SHeap<STextureProp>::__Tstruct *array; // edx
  result = idx;
  if ( idx >= 0 && idx < this->Textures.size )
  {
    array = this->Textures.array;
    if ( array[idx].use == 0x7FFFFFFF )
      ++array[idx].data.RefCount;
  }
  return result;
}

//----- (00429EC0) --------------------------------------------------------

void SGepard::AddToHoleList(int x, int z)

{
  SHoleInfo *first; // eax
  SHoleInfo *v5; // ecx
  SHoleInfo *v6; // ecx
  SHoleInfo *last; // eax
  first = this->GlobalHoleList.first;
  v5 = first;
  this->GlobalHoleList.current = first;
  if ( first )
  {
    while ( v5->x != x || v5->z != z )
    {
      if ( this->GlobalHoleList.Closed )
      {
        first = first->next;
        if ( !first )
          first = this->GlobalHoleList.first;
      }
      else
      {
        first = first->next;
      }
      v5 = first;
      this->GlobalHoleList.current = first;
      if ( !first )
        goto LABEL_9;
    }
  }
  else
  {
LABEL_9:
    v6 = (SHoleInfo *)operator new(sizeof(SHoleInfo));
    memset(&v6->next , 0, 16);
    v6->nodechain = 0;
    last = this->GlobalHoleList.last;
    if ( last )
    {
      last->next = v6;
      v6->prev = this->GlobalHoleList.last;
      this->GlobalHoleList.last = v6;
      v6->next = 0;
    }
    else
    {
      this->GlobalHoleList.last = v6;
      this->GlobalHoleList.first = v6;
    }
    ++this->GlobalHoleList.NumItems;
    this->GlobalHoleList.current = v6;
    v6->x = x;
    this->GlobalHoleList.current->z = z;
  }
}

//----- (00429FA0) --------------------------------------------------------

void SGepard::AddToShaderFolderList(const char *foldername)

{
  const char *v2; // esi
  SFolders *v4; // edi
  SFolders *last; // eax
  v2 = foldername;
  v4 = (SFolders *)operator new(sizeof(SFolders));
  memset(v4, 0, sizeof(SFolders));
  last = this->ShaderFolders.last;
  if ( last )
  {
    last->next = v4;
    v4->prev = this->ShaderFolders.last;
    this->ShaderFolders.last = v4;
    v4->next = 0;
  }
  else
  {
    this->ShaderFolders.last = v4;
    this->ShaderFolders.first = v4;
  }
  ++this->ShaderFolders.NumItems;
  strcpy((char *)v4 + 8, foldername);
}

//----- (0042A020) --------------------------------------------------------

void SGepard::AdvanceTime(unsigned int elapsed, unsigned int anim_elapsed)

{
  this->WorldTime += elapsed;
  this->AnimTime += anim_elapsed;
  this->AnimElapsedTime = anim_elapsed;
}

//----- (0042A040) --------------------------------------------------------

void SGepard::BackbufferScreenshot(const char *filename)

{
  this->ScreenshotFile = filename;
}

//----- (0042A050) --------------------------------------------------------

void SGepard::BringToFront()

{
  SShaderInfo *SelectedShader; // edx
  SShaderInfo *prev; // eax
  SShaderInfo *next; // esi
  SShaderInfo *last; // eax
  SelectedShader = this->SelectedShader;
  if ( SelectedShader )
  {
    prev = SelectedShader->prev;
    next = SelectedShader->next;
    if ( SelectedShader->next )
      next->prev = prev;
    if ( prev )
      prev->next = next;
    if ( !next )
      this->ShaderInfos.last = prev;
    if ( !prev )
      this->ShaderInfos.first = next;
    last = this->ShaderInfos.last;
    if ( last )
    {
      last->next = SelectedShader;
      SelectedShader->prev = this->ShaderInfos.last;
      SelectedShader->next = 0;
    }
    else
    {
      this->ShaderInfos.first = SelectedShader;
    }
    this->ShaderInfos.last = SelectedShader;
  }
}

//----- (0042A0C0) --------------------------------------------------------

void SGepard::ChangeLakeHeight(int idx, float y)

{
  SHeap<SGLake>::__Tstruct *array; // eax
  int v5;
  int v6;
  SHeap<SGLake>::__Tstruct *v7; // eax
  int v8;
  if ( idx >= 0 && idx < this->Lakes.size )
  {
    array = this->Lakes.array;
    v5 = idx;
    if ( array[idx].use == 0x7FFFFFFF )
    {
      v6 = 0;
      array[v5].data.Y = y;
      v7 = &this->Lakes.array[v5];
      if ( v7->data.Blocks.size > 0 )
      {
        v8 = 0;
        do
        {
          ++v8;
          ++v6;
          v7->data.Blocks.array[v8 - 1].RecalcHeightTransparency = 1;
          v7 = &this->Lakes.array[v5];
        }
        while ( v6 < v7->data.Blocks.size );
      }
    }
  }
}

//----- (0042A130) --------------------------------------------------------

void SGepard::ChangeTextureAlphaType(int idx, int alpha)

{
  SHeap<STextureProp>::__Tstruct *array; // ecx
  if ( idx >= 0 && idx < this->Textures.size )
  {
    array = this->Textures.array;
    if ( array[idx].use == 0x7FFFFFFF )
      array[idx].data.Alpha = alpha;
  }
}

//----- (0042A160) --------------------------------------------------------

bool SGepard::CheckClickBox(D3DXVECTOR3 *vec, float click_x1, float click_y1, float click_x2, float click_y2)

{
  float v7; // xmm3_4
  float v8; // xmm2_4
  bool result; // al
  D3DXVECTOR3 vec2;
  D3DXVec3TransformCoord(&vec2, vec, &this->CameraMatrix);
  result = 0;
  if ( vec2.z > 0.0 )
  {
    v7 = this->ProjectionMatrix._11 * vec2.x;
    v8 = this->ProjectionMatrix._22 * vec2.y;
    if ( v7 >= (float)(vec2.z * click_x1)
      && (float)(vec2.z * click_x2) >= v7
      && v8 >= (float)(vec2.z * click_y1)
      && (float)(vec2.z * click_y2) >= v8 )
    {
      return 1;
    }
  }
  return result;
}

//----- (0042A210) --------------------------------------------------------

bool SGepard::CheckDepthStencilFormat(D3DFORMAT format)

{
  return this->lpD3D->CheckDeviceFormat(
           this->Adapter,
           this->DeviceType,
           this->AdapterFormat,
           2u,
           D3DRTYPE_SURFACE,
           format) == 0;
}

//----- (0042A250) --------------------------------------------------------

bool SGepard::CheckTextureFormat(D3DFORMAT format, bool rendertarget)

{
  return this->lpD3D->CheckDeviceFormat(
           this->Adapter,
           this->DeviceType,
           this->AdapterFormat,
           rendertarget,
           D3DRTYPE_TEXTURE,
           format) == 0;
}

//----- (0042A290) --------------------------------------------------------

int SGepard::CleanupObjectCache()

{
  int v1;
  int v3;
  int size;
  SHeap<SObjectCacheProp>::__Tstruct *i; // eax
  SHeap<SObjectCacheProp>::__Tstruct *array; // eax
  SGroup *Group; // ecx
  char *FileName; // eax
  SHeap<SObjectCacheProp>::__Tstruct *v10; // ecx
  int v11;
  v1 = 0;
  v3 = -1;
LABEL_2:
  v11 = v1;
  while ( 1 )
  {
    size = this->ObjectCache.size;
    if ( ++v3 >= size )
      return v1;
    for ( i = &this->ObjectCache.array[v3]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v3 >= size )
        return v1;
    }
    if ( v3 < 0 )
      return v1;
    array = this->ObjectCache.array;
    if ( array[v3].data.RefCount )
    {
      v1 = v11 + 1;
      goto LABEL_2;
    }
    Group = array[v3].data.Group;
    if ( Group )
    {
      Group->Release();
      this->ObjectCache.array[v3].data.Group = 0;
      array = this->ObjectCache.array;
    }
    FileName = array[v3].data.FileName;
    if ( FileName )
    {
      ::operator delete(FileName);
      this->ObjectCache.array[v3].data.FileName = 0;
    }
    if ( v3 >= this->ObjectCache.size || (v10 = this->ObjectCache.array, v10[v3].use != 0x7FFFFFFF) )
      Logger.g->Panic("SHeap::Remove: invalid index (%d)", v3);
    v10[v3].use = this->ObjectCache.nextempty;
    --this->ObjectCache.occupied;
    v1 = v11;
    this->ObjectCache.nextempty = v3;
  }
}

//----- (0042A380) --------------------------------------------------------

void SGepard::CleanupSplines()

{
  SHillRing *first; // eax
  SHillRing *v3; // ebx
  SChain<SSplineRing> *SplineChain; // ecx
  bool Closed; // dl
  SSplineRing *next; // ecx
  first = this->HillChain.first;
  v3 = first;
  for ( this->HillChain.current = first; first; this->HillChain.current = first )
  {
    SplineChain = v3->SplineChain;
    Closed = SplineChain->Closed;
    if ( SplineChain->first )
    {
      next = SplineChain->first->next;
      if ( Closed && !next )
        Logger.g->Panic("GetSecond: In a closed chain <first->next> is NULL");
      if ( next && next->spline->Nodes.NumItems < 4 )
      {
        this->DestroyHill(v3);
        first = this->HillChain.current;
      }
    }
    else if ( Closed )
    {
      Logger.g->Panic("GetSecond: <first> is NULL");
    }
    if ( this->HillChain.Closed )
    {
      if ( !first )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      first = first->next;
      if ( !first )
        first = this->HillChain.first;
    }
    else if ( first )
    {
      first = first->next;
    }
    else
    {
      first = 0;
    }
    v3 = first;
  }
  this->gpSelSplineRing = 0;
  this->gpSelHillRing = 0;
}

//----- (0042A460) --------------------------------------------------------

void SGepard::CleanupTextureCache()

{
  int v2;
  int size;
  SHeap<STextureProp>::__Tstruct *i; // eax
  SHeap<STextureProp>::__Tstruct *array; // eax
  int v6;
  char *FileName; // ecx
  IDirect3DTexture9 *lpTexture; // ecx
  SHeap<STextureProp>::__Tstruct *v9; // ecx
  v2 = -1;
  while ( 1 )
  {
    size = this->Textures.size;
    if ( ++v2 >= size )
      break;
    for ( i = &this->Textures.array[v2]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v2 >= size )
        return;
    }
    if ( v2 < 0 )
      break;
    array = this->Textures.array;
    v6 = v2;
    if ( !array[v2].data.RefCount )
    {
      FileName = array[v6].data.FileName;
      if ( FileName )
      {
        ::operator delete(FileName);
        this->Textures.array[v6].data.FileName = 0;
        array = this->Textures.array;
      }
      lpTexture = array[v6].data.lpTexture;
      if ( lpTexture )
      {
        lpTexture->Release();
        this->Textures.array[v6].data.lpTexture = 0;
      }
      if ( v2 >= this->Textures.size || (v9 = this->Textures.array, v9[v2].use != 0x7FFFFFFF) )
        Logger.g->Panic("SHeap::Remove: invalid index (%d)", v2);
      v9[v2].use = this->Textures.nextempty;
      --this->Textures.occupied;
      this->Textures.nextempty = v2;
    }
  }
}

//----- (0042A540) --------------------------------------------------------

void SGepard::ClearDebugLines(unsigned int color, int owner)

{
  unsigned int count;
  int v5;
  SGepardDebugLine *array; // ecx
  unsigned int v7;
  unsigned int v8;
  int v9;
  SGepardDebugLine *v10; // ecx
  count = this->debugLines.count;
  if ( count )
  {
    v5 = count;
    do
    {
      --count;
      --v5;
      if ( count >= this->debugLines.count )
LABEL_12:
        Logger.g->Panic("Assert (%s) failed", "index < count");
      array = this->debugLines.array;
      if ( array[v5].color == color )
      {
        v7 = this->debugLines.count;
        if ( array[v5].owner == owner )
        {
          v8 = count;
          if ( count >= v7 )
            goto LABEL_12;
          this->debugLines.count = v7 - 1;
          if ( count < v7 - 1 )
          {
            v9 = v5 * 32;
            do
            {
              v10 = this->debugLines.array;
              v9 += 32;
              ++v8;
              memcpy(((char *)&v10[-1].start.x + v9) , ((char *)&v10->start.x + v9), 16);
              memcpy(((char *)v10 + v9 - 16) , ((char *)&v10->end.y + v9), 16);
            }
            while ( v8 < this->debugLines.count );
          }
        }
      }
    }
    while ( count );
  }
}

//----- (0042A5F0) --------------------------------------------------------

void SGepard::ClearDynamicVBs()

{
  int v2;
  int size;
  SHeap<SDynamicVB>::__Tstruct *i; // eax
  IDirect3DVertexBuffer9 *lpVertexBuffer; // ecx
  v2 = -1;
  while ( 1 )
  {
    size = this->DynamicVBs.size;
    if ( ++v2 >= size )
      break;
    for ( i = &this->DynamicVBs.array[v2]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v2 >= size )
        return;
    }
    if ( v2 < 0 )
      break;
    lpVertexBuffer = this->DynamicVBs.array[v2].data.lpVertexBuffer;
    if ( lpVertexBuffer )
    {
      lpVertexBuffer->Release();
      this->DynamicVBs.array[v2].data.lpVertexBuffer = 0;
    }
  }
}

//----- (0042A670) --------------------------------------------------------

void SGepard::ClearGlow()

{
  SEffect *Effect; // eax
  Effect = this->Effect;
  if ( Effect )
    Effect->ClearGlow((SGlow *)this->Glow);
}

//----- (0042A690) --------------------------------------------------------

void SGepard::ClearHoleList()

{
  SChain<SHoleInfo> *p_GlobalHoleList; // ebx
  SHoleInfo *i; // eax
  SChain<SNodeInfo2> *nodechain; // edi
  SNodeInfo2 *first; // eax
  SNodeInfo2 *next; // esi
  SHoleInfo *current; // eax
  SHoleInfo *v7; // eax
  SGepard *v8;
  p_GlobalHoleList = &this->GlobalHoleList;
  v8 = this;
  this->GlobalHoleList.current = this->GlobalHoleList.first;
  for ( i = this->GlobalHoleList.current; i; i = this->GlobalHoleList.current )
  {
    nodechain = i->nodechain;
    first = nodechain->first;
    if ( nodechain->first )
    {
      do
      {
        next = first->next;
        ::operator delete(first);
        first = next;
      }
      while ( next );
      //this = v8;
    }
    nodechain->first = 0;
    nodechain->last = 0;
    nodechain->current = 0;
    nodechain->NumItems = 0;
    current = p_GlobalHoleList->current;
    if ( p_GlobalHoleList->Closed )
    {
      if ( !current )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      v7 = current->next;
      if ( !v7 )
        v7 = p_GlobalHoleList->first;
    }
    else if ( current )
    {
      v7 = current->next;
    }
    else
    {
      v7 = 0;
    }
    p_GlobalHoleList->current = v7;
  }
  p_GlobalHoleList->DeleteAll();
}

//----- (0042A740) --------------------------------------------------------

void SGepard::ClearObjectShadows(bool reset, bool keepmask)

{
  int i;
  int size;
  SHeap<SObjectProp>::__Tstruct *v6; // eax
  IDirect3DSurface9 *ShadowRenderTarget; // ecx
  IDirect3DSurface9 *ShadowRenderTargetZ; // ecx
  for ( i = -1; ; this->DynamicObjects.array[i].data.Group->ClearShadow(0) )
  {
    size = this->DynamicObjects.size;
    if ( ++i < size )
    {
      v6 = &this->DynamicObjects.array[i];
      while ( v6->use != 0x7FFFFFFF )
      {
        ++i;
        ++v6;
        if ( i >= size )
          goto LABEL_6;
      }
      if ( i >= 0 )
        continue;
    }
    break;
  }
LABEL_6:
  ShadowRenderTarget = this->ShadowRenderTarget;
  if ( ShadowRenderTarget )
  {
    ShadowRenderTarget->Release();
    this->ShadowRenderTarget = 0;
  }
  ShadowRenderTargetZ = this->ShadowRenderTargetZ;
  if ( ShadowRenderTargetZ )
  {
    ShadowRenderTargetZ->Release();
    this->ShadowRenderTargetZ = 0;
  }
}

//----- (0042A7D0) --------------------------------------------------------

void SGepard::ClearResolutionScale()

{
  IDirect3DSurface9 *Ptr; // ecx
  IDirect3DSurface9 *v3; // ecx
  Ptr = this->ResolutionScaleSurface.Ptr;
  if ( Ptr )
  {
    Ptr->Release();
    this->ResolutionScaleSurface.Ptr = 0;
  }
  v3 = this->ResolutionScaleDepthSurface.Ptr;
  if ( v3 )
  {
    v3->Release();
    this->ResolutionScaleDepthSurface.Ptr = 0;
  }
}

//----- (0042A810) --------------------------------------------------------

void SGepard::ClearShaderFolderList()

{
  this->ShaderFolders.DeleteAll();
}

//----- (0042A820) --------------------------------------------------------

void SGepard::ClearShaders()

{
  int v2;
  SmartPtr<IDirect3DPixelShader9> *standardPixelShaders; // esi
  IDirect3DPixelShader9 *Ptr; // ecx
  SmartPtr<IDirect3DPixelShader9> *depthWritePixelShaders; // esi
  int v6;
  IDirect3DPixelShader9 *v7; // ecx
  SmartPtr<IDirect3DPixelShader9> *terrainPixelShader; // esi
  int v9;
  IDirect3DPixelShader9 *v10; // ecx
  IDirect3DPixelShader9 *v11; // ecx
  IDirect3DVertexShader9 *v12; // ecx
  IDirect3DPixelShader9 *v13; // ecx
  IDirect3DVertexShader9 *v14; // ecx
  IDirect3DPixelShader9 *v15; // ecx
  IDirect3DVertexShader9 *v16; // ecx
  IDirect3DPixelShader9 *v17; // ecx
  IDirect3DVertexShader9 *v18; // ecx
  IDirect3DPixelShader9 *v19; // ecx
  v2 = 7;
  standardPixelShaders = this->standardPixelShaders;
  do
  {
    Ptr = standardPixelShaders[-7].Ptr;
    if ( Ptr )
    {
      Ptr->Release();
      standardPixelShaders[-7].Ptr = 0;
    }
    if ( standardPixelShaders->Ptr )
    {
      standardPixelShaders->Ptr->Release();
      standardPixelShaders->Ptr = 0;
    }
    ++standardPixelShaders;
    --v2;
  }
  while ( v2 );
  depthWritePixelShaders = this->depthWritePixelShaders;
  v6 = 8;
  do
  {
    v7 = depthWritePixelShaders[-8].Ptr;
    if ( v7 )
    {
      v7->Release();
      depthWritePixelShaders[-8].Ptr = 0;
    }
    if ( depthWritePixelShaders->Ptr )
    {
      depthWritePixelShaders->Ptr->Release();
      depthWritePixelShaders->Ptr = 0;
    }
    ++depthWritePixelShaders;
    --v6;
  }
  while ( v6 );
  terrainPixelShader = this->terrainPixelShader;
  v9 = 2;
  do
  {
    v10 = terrainPixelShader[-2].Ptr;
    if ( v10 )
    {
      v10->Release();
      terrainPixelShader[-2].Ptr = 0;
    }
    if ( terrainPixelShader->Ptr )
    {
      terrainPixelShader->Ptr->Release();
      terrainPixelShader->Ptr = 0;
    }
    ++terrainPixelShader;
    --v9;
  }
  while ( v9 );
  v11 = this->sepiaPixelShader.Ptr;
  if ( v11 )
  {
    v11->Release();
    this->sepiaPixelShader.Ptr = 0;
  }
  v12 = this->terrainLightingVertexShader.Ptr;
  if ( v12 )
  {
    v12->Release();
    this->terrainLightingVertexShader.Ptr = 0;
  }
  v13 = this->terrainLightingPixelShader.Ptr;
  if ( v13 )
  {
    v13->Release();
    this->terrainLightingPixelShader.Ptr = 0;
  }
  v14 = this->ambientLitVertexShader.Ptr;
  if ( v14 )
  {
    v14->Release();
    this->ambientLitVertexShader.Ptr = 0;
  }
  v15 = this->ambientLitPixelShader.Ptr;
  if ( v15 )
  {
    v15->Release();
    this->ambientLitPixelShader.Ptr = 0;
  }
  v16 = this->decalVertexShader.Ptr;
  if ( v16 )
  {
    v16->Release();
    this->decalVertexShader.Ptr = 0;
  }
  v17 = this->decalPixelShader.Ptr;
  if ( v17 )
  {
    v17->Release();
    this->decalPixelShader.Ptr = 0;
  }
  v18 = this->unlitDecalVertexShader.Ptr;
  if ( v18 )
  {
    v18->Release();
    this->unlitDecalVertexShader.Ptr = 0;
  }
  v19 = this->unlitDecalPixelShader.Ptr;
  if ( v19 )
  {
    v19->Release();
    this->unlitDecalPixelShader.Ptr = 0;
  }
}

//----- (0042A9D0) --------------------------------------------------------

void SGepard::ClearShadowMap()

{
  IDirect3DTexture9 *Ptr; // ecx
  IDirect3DTexture9 *v3; // ecx
  Ptr = this->ShadowMapTexture.Ptr;
  if ( Ptr )
  {
    Ptr->Release();
    this->ShadowMapTexture.Ptr = 0;
  }
  v3 = this->ShadowMapDepthTexture.Ptr;
  if ( v3 )
  {
    v3->Release();
    this->ShadowMapDepthTexture.Ptr = 0;
  }
}

//----- (0042AA10) --------------------------------------------------------

void SGepard::CloseGroundTrail(int idx)

{
  SHeap<SGroundTrail>::__Tstruct *array; // ecx
  if ( idx >= 0 && idx < this->GroundTrails.size )
  {
    array = this->GroundTrails.array;
    if ( array[idx].use == 0x7FFFFFFF )
      array[idx].data.AutoDestruct = 1;
  }
}

//----- (0042AA40) --------------------------------------------------------

void SGepard::CloseSmokeTrail(int idx)

{
  SHeap<SSmokeTrail>::__Tstruct *array; // ecx
  if ( idx >= 0 && idx < this->SmokeTrails.size )
  {
    array = this->SmokeTrails.array;
    if ( array[idx].use == 0x7FFFFFFF )
      array[idx].data.AutoDestruct = 1;
  }
}

//----- (0042AA70) --------------------------------------------------------

void SGepard::CloseSpline(SHillRing *_hillring)

{
  SChain<SSplineRing> *i; // eax
  SChain<SSplineRing> *SplineChain; // ecx
  SSplineRing *current; // eax
  SSplineRing *next; // eax
  _hillring->SplineChain->current = _hillring->SplineChain->first;
  for ( i = _hillring->SplineChain; i->current; i = _hillring->SplineChain )
  {
    i->current->spline->Close();
    SplineChain = _hillring->SplineChain;
    current = SplineChain->current;
    if ( SplineChain->Closed )
    {
      if ( !current )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      next = current->next;
      if ( !next )
        next = SplineChain->first;
    }
    else if ( current )
    {
      next = current->next;
    }
    else
    {
      next = 0;
    }
    SplineChain->current = next;
  }
  this->RefreshHill(_hillring);
}

//----- (0042AAF0) --------------------------------------------------------

void SGepard::CollectNode(SHillRing *_hillring, SSplineRing *_splinering, SNode *_node, int _nodeindex, float _magassag)

{
  SNodeInfo *v7; // ecx
  SNodeInfo *last; // eax
  v7 = (SNodeInfo *)operator new(sizeof(SNodeInfo));
  last = this->NodeInfos.last;
  if ( last )
  {
    last->next = v7;
    v7->prev = this->NodeInfos.last;
    this->NodeInfos.last = v7;
    v7->next = 0;
  }
  else
  {
    this->NodeInfos.first = v7;
    this->NodeInfos.last = v7;
    v7->next = 0;
    this->NodeInfos.first->prev = 0;
  }
  ++this->NodeInfos.NumItems;
  this->NodeInfos.current = v7;
  v7->hillring = _hillring;
  this->NodeInfos.current->splinering = _splinering;
  this->NodeInfos.current->node = _node;
  this->NodeInfos.current->nodeindex = _nodeindex;
  this->NodeInfos.current->magassag = _magassag;
}

//----- (0042AB90) --------------------------------------------------------

SChain<SNodeInfo2> *SGepard::CollectTileNodes(int _x, int _z, SChain<SSplineRing> *_splinechain)

{
  SChain<SNodeInfo2> *v5; // eax
  SChain<SNodeInfo2> *v6; // esi
  SNodeInfo2 *v7; // edi
  SNodeInfo2 *last; // eax
  double Height; // st7
  SNodeInfo2 *v10; // edi
  SNodeInfo2 *v11; // eax
  double v12; // st7
  SNodeInfo2 *v13; // edi
  SNodeInfo2 *v14; // eax
  double v15; // st7
  SNodeInfo2 *v16; // edi
  SNodeInfo2 *v17; // eax
  double v18; // st7
  int z;
  // x64 fix: literal 0x14 (= 20) was x86 sizeof(SChain<...>); x64 is bigger.
  v5 = (SChain<SNodeInfo2> *)operator new(sizeof(SChain<SNodeInfo2>));
  v6 = v5;
  if ( v5 )
  {
    v5->first = 0;
    v5->last = 0;
    v5->current = 0;
    v5->Closed = 0;
    v5->NumItems = 0;
  }
  else
  {
    v6 = 0;
  }
  v7 = (SNodeInfo2 *)operator new(sizeof(SNodeInfo2));
  memset(&v7->prev , 0, 16);
  memset(&v7->z , 0, 8);
  last = v6->last;
  if ( last )
  {
    last->next = v7;
    v7->prev = v6->last;
    v6->last = v7;
    v7->next = 0;
  }
  else
  {
    v6->last = v7;
    v6->first = v7;
  }
  ++v6->NumItems;
  z = _z + 1;
  v7->x = (float)_x;
  Height = this->Terrain->GetHeight(_x, _z + 1);
  v7->nodetype = 0;
  v7->y = (float)(Height);

  v7->z = (float)(_z + 1);
  v10 = (SNodeInfo2 *)operator new(sizeof(SNodeInfo2));
  memset(&v10->prev , 0, 16);
  memset(&v10->z , 0, 8);
  v11 = v6->last;
  if ( v11 )
  {
    v11->next = v10;
    v10->prev = v6->last;
    v6->last = v10;
    v10->next = 0;
  }
  else
  {
    v6->last = v10;
    v6->first = v10;
  }
  ++v6->NumItems;
  v10->x = (float)(_x + 1);
  v12 = this->Terrain->GetHeight(_x + 1, z);
  v10->nodetype = 0;
  v10->y = (float)(v12);

  v10->z = (float)z;
  v13 = (SNodeInfo2 *)operator new(sizeof(SNodeInfo2));
  memset(&v13->prev , 0, 16);
  memset(&v13->z , 0, 8);
  v14 = v6->last;
  if ( v14 )
  {
    v14->next = v13;
    v13->prev = v6->last;
    v6->last = v13;
    v13->next = 0;
  }
  else
  {
    v6->last = v13;
    v6->first = v13;
  }
  ++v6->NumItems;
  v13->x = (float)(_x + 1);
  v15 = this->Terrain->GetHeight(_x + 1, _z);
  v13->nodetype = 0;
  v13->y = (float)(v15);

  v13->z = (float)_z;
  v16 = (SNodeInfo2 *)operator new(sizeof(SNodeInfo2));
  memset(&v16->prev , 0, 16);
  memset(&v16->z , 0, 8);
  v17 = v6->last;
  if ( v17 )
  {
    v17->next = v16;
    v16->prev = v6->last;
    v6->last = v16;
    v16->next = 0;
  }
  else
  {
    v6->last = v16;
    v6->first = v16;
  }
  ++v6->NumItems;
  v16->x = (float)_x;
  v18 = this->Terrain->GetHeight(_x, _z);
  v16->nodetype = 0;
  v16->y = (float)(v18);

  v16->z = (float)_z;
  if ( _splinechain->Closed && !_splinechain->first )
    Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
 this->AddNodesFromSplineNodes(&_splinechain->first->spline->Nodes, v6, (float)_x,
    (float)(_x + 1),
    (float)_z,
    (float)z);
  if ( _splinechain->Closed && !_splinechain->last )
    Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
 this->AddNodesFromSplineNodes(&_splinechain->last->spline->Nodes, v6, (float)_x,
    (float)(_x + 1),
    (float)_z,
    (float)z);
  v6->Closed = 1;
  return v6;
}

//----- (0042AE80) --------------------------------------------------------

void SGepard::CopyShaders(SDArray<SShaderClipboard> *shaders, int x0, int z0, int x1, int z1)

{
  SGepard *v6; // ebx
  SShaderInfo *first; // edi
  int v8;
  SDArray<SShaderClipboard> *v9; // esi
  int xpos;
  int zpos;
  int size;
  int maxsize;
  int v14;
  SShaderClipboard *v15; // eax
  const char *filename; // edx
  void *v19; // eax
  SShaderInfo *current; // eax
  SShaderInfo *next; // eax
  int v23;
  v6 = this;
  first = this->ShaderInfos.first;
  this->ShaderInfos.current = first;
  if ( first )
  {
    v8 = z0;
    v9 = shaders;
    do
    {
      xpos = (int)first->xpos;
      if ( xpos >= x0 && xpos + first->XVertices - 1 <= x1 )
      {
        zpos = (int)first->zpos;
        if ( zpos >= v8 && zpos + first->ZVertices - 1 <= z1 )
        {
          size = v9->size;
          maxsize = v9->maxsize;
          if ( v9->size == maxsize )
          {
            if ( maxsize >= 16 )
              v14 = 6 * maxsize / 5;
            else
              v14 = 16;
            // x64 fix: literal 20-byte stride was sizeof(SShaderClipboard)
            // on x86 (4-byte filename pointer + 4×int). On x64 the filename
            // pointer is 8 bytes so the struct is 24 bytes; the old realloc
            // was undersized by 4 bytes per element and the memset cleared
            // only part of each new slot.
            v15 = (SShaderClipboard *)realloc(v9->array, sizeof(SShaderClipboard) * v14);
            shaders->array = v15;
            memset(&v15[shaders->maxsize], 0, sizeof(SShaderClipboard) * (v14 - shaders->maxsize));
            v9 = shaders;
            size = shaders->size;
            shaders->maxsize = v14;
            v6 = this;
          }
          v9->size = size + 1;
          filename = first->filename;
          // x64 fix: original used `v17 = (int)&array[size]` then byte-offset
          // writes (`*(_DWORD*)v17 = filename_ptr`, `*(_DWORD*)(v17+4) = length`).
          // On x86 the offsets aligned with {char* filename, int length} (4+4);
          // on x64 the (int) cast truncates the heap pointer and the +4 offset
          // lands in the high half of the now-8-byte filename. Replaced with
          // typed access so the malloc'd filename pointer stores all 8 bytes
          // and `length` lands at its real offset.
          {
            SShaderClipboard *entry = &v9->array[size];
            v23 = size;
            if ( entry->filename )
            {
              operator delete(entry->filename);
              entry->filename = nullptr;
              filename = first->filename;
            }
            if ( filename )
            {
              entry->length = (int)strlen(filename);
              v19 = malloc(entry->length + 1);
              entry->filename = (char *)v19;
              memcpy(v19, first->filename, entry->length + 1);
            }
            else
            {
              entry->length = 0;
              entry->filename = nullptr;
            }
          }
          v9 = shaders;
          shaders->array[v23].X = (int)first->xpos - x0;
          shaders->array[v23].Z = (int)first->zpos - z0;
          shaders->array[v23].Rotation = first->Rotation;
          v8 = z0;
        }
      }
      current = v6->ShaderInfos.current;
      if ( v6->ShaderInfos.Closed )
      {
        if ( !current )
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        next = current->next;
        if ( !next )
          next = v6->ShaderInfos.first;
      }
      else if ( current )
      {
        next = current->next;
      }
      else
      {
        next = 0;
      }
      first = next;
      v6->ShaderInfos.current = next;
    }
    while ( next );
  }
}

//----- (0042B070) --------------------------------------------------------

int SGepard::CreateDirectionalLight(float colorr, float colorg, float colorb, float vectorx, float vectory, float vectorz)

{
  SLightProp *LightProps; // eax
  signed int v8;
  _D3DLIGHT9 *p_d3dlight; // esi
  LightProps = this->LightProps;
  v8 = 0;
  while ( LightProps->type )
  {
    ++v8;
    ++LightProps;
    if ( v8 >= 4 )
      Logger.g->Panic("SGepard::GetLightHandle: Fatal error: No more light avaiable! Exiting...");
  }
  p_d3dlight = &this->LightProps[v8].d3dlight;
  memset(&this->LightProps[v8].d3dlight.Diffuse, 0, 0x64u);
  p_d3dlight->Diffuse.b = colorb;
  p_d3dlight->Specular.b = colorb;
  p_d3dlight->Direction.x = vectorx;
  p_d3dlight->Direction.y = vectory;
  p_d3dlight->Type = D3DLIGHT_DIRECTIONAL;
  p_d3dlight->Diffuse.r = colorr;
  p_d3dlight->Diffuse.g = colorg;
  p_d3dlight->Specular.r = colorr;
  p_d3dlight->Specular.g = colorg;
  p_d3dlight->Direction.z = vectorz;
  this->lpD3DDev->SetLight(v8, p_d3dlight);
  this->lpD3DDev->LightEnable(v8, 1);
  this->LightProps[v8].type = 1;
  return v8;
}

//----- (0042B160) --------------------------------------------------------

int SGepard::CreateDynamicShader(int texture, float x, float z, float size, unsigned int color)

{
  int nextempty;
  SHeap<SDynamicShaderProp>::__Tstruct *array; // ecx
  int v9;
  SHeap<SDynamicShaderProp>::__Tstruct *v10; // eax
  int v11;
  int maxsize;
  int v13;
  SHeap<SDynamicShaderProp>::__Tstruct *v14; // eax
  int v15;
  int v16;
  SHeap<STextureProp>::__Tstruct *v17; // edx
  int v18;
  int result;
  ++this->DynamicShaders.occupied;
  nextempty = this->DynamicShaders.nextempty;
  if ( nextempty < 0 )
  {
    v11 = this->DynamicShaders.size;
    maxsize = this->DynamicShaders.maxsize;
    if ( v11 == maxsize )
    {
      if ( maxsize >= 16 )
        v13 = 6 * maxsize / 5;
      else
        v13 = 16;
      // x64: literal `24` is x86 sizeof(SHeap<SDynamicShaderProp>::__Tstruct). Same family as group.cpp:1884.
      v14 = (SHeap<SDynamicShaderProp>::__Tstruct *)realloc(this->DynamicShaders.array, sizeof(SHeap<SDynamicShaderProp>::__Tstruct) * v13);
      v15 = this->DynamicShaders.maxsize;
      this->DynamicShaders.array = v14;
      memset(&v14[v15], 0, sizeof(SHeap<SDynamicShaderProp>::__Tstruct) * (v13 - v15));
      v11 = this->DynamicShaders.size;
      this->DynamicShaders.maxsize = v13;
    }
    this->DynamicShaders.array[v11].use = 0x7FFFFFFF;
    nextempty = this->DynamicShaders.size;
    this->DynamicShaders.size = nextempty + 1;
  }
  else
  {
    array = this->DynamicShaders.array;
    v9 = nextempty;
    this->DynamicShaders.nextempty = array[nextempty].use;
    array[v9].use = 0x7FFFFFFF;
    v10 = this->DynamicShaders.array;
    memset(&v10[v9].data.TextureIndex , 0, 16);
    v10[v9].data.Color = 0;
  }
  if ( texture < 0
    || (v16 = this->Textures.size, texture >= v16)
    || (v17 = this->Textures.array, v17[texture].use != 0x7FFFFFFF) )
  {
    Logger.g->Panic("SGepard::CreateDynamicShader: Invalid texture index");
  }
  if ( texture < v16 && v17[texture].use == 0x7FFFFFFF )
    ++v17[texture].data.RefCount;
  v18 = nextempty;
  this->DynamicShaders.array[v18].data.TextureIndex = texture;
  this->DynamicShaders.array[v18].data.X = x;
  result = nextempty;
  this->DynamicShaders.array[v18].data.Z = z;
  this->DynamicShaders.array[v18].data.Size = size;
  this->DynamicShaders.array[v18].data.Color = color;
  return result;
}

//----- (0042B2F0) --------------------------------------------------------

int SGepard::CreateDynamicVB(unsigned int fvf, unsigned int numvertices)

{
  int nextempty;
  SHeap<SDynamicVB>::__Tstruct *array; // ecx
  int v6;
  SHeap<SDynamicVB>::__Tstruct *v7; // eax
  int size;
  int maxsize;
  int v10;
  SHeap<SDynamicVB>::__Tstruct *v11; // eax
  int v12;
  int v13;
  unsigned int v14;
  int v15;
  int v16;
  unsigned int v17;
  unsigned int v18;
  int v19;
  int v20;
  unsigned int v21;
  int v22;
  int v23;
  signed int v24;
  int v25;
  HRESULT v26;
  const char *v28; // eax
  int v29;
  int v30;
  char atmstr[200];
  nextempty = this->DynamicVBs.nextempty;
  ++this->DynamicVBs.occupied;
  v30 = nextempty;
  if ( nextempty < 0 )
  {
    size = this->DynamicVBs.size;
    maxsize = this->DynamicVBs.maxsize;
    if ( size == maxsize )
    {
      if ( maxsize >= 16 )
        v10 = 6 * maxsize / 5;
      else
        v10 = 16;
      v11 = (SHeap<SDynamicVB>::__Tstruct *)realloc(this->DynamicVBs.array, sizeof(SHeap<SDynamicVB>::Element) * v10);
      v12 = this->DynamicVBs.maxsize;
      this->DynamicVBs.array = v11;
      memset(&v11[v12], 0, sizeof(SHeap<SDynamicVB>::Element) * (v10 - v12));
      size = this->DynamicVBs.size;
      this->DynamicVBs.maxsize = v10;
    }
    this->DynamicVBs.array[size].use = 0x7FFFFFFF;
    nextempty = this->DynamicVBs.size;
    v30 = nextempty;
    this->DynamicVBs.size = nextempty + 1;
  }
  else
  {
    array = this->DynamicVBs.array;
    v6 = nextempty;
    this->DynamicVBs.nextempty = array[nextempty].use;
    array[v6].use = 0x7FFFFFFF;
    v7 = this->DynamicVBs.array;
    memset(&v7[v6].data.lpVertexBuffer , 0, 16);
    memset(&v7[v6].data.Locked , 0, 8);
  }
  v13 = nextempty;
  v29 = v13 * (int)sizeof(SHeap<SDynamicVB>::Element);
  this->DynamicVBs.array[v13].data.NumVertices = numvertices;
  v14 = fvf & 0xFFFFFFFD;
  if ( (fvf & 2) == 0 )
    v14 = fvf;
  this->DynamicVBs.array[v13].data.VertexFormat = fvf;
  v15 = (fvf & 2) != 0 ? 0xC : 0;
  if ( (v14 & 4) != 0 )
  {
    v15 += 16;
    v14 &= ~4u;
  }
  v16 = v15 + 12;
  v17 = v14 & 0xFFFFFFEF;
  if ( (v14 & 0x10) == 0 )
  {
    v17 = v14;
    v16 = v15;
  }
  v18 = v17 & 0xFFFFFFDF;
  v19 = v17 & 0x20;
  if ( (v17 & 0x20) == 0 )
    v18 = v17;
  v20 = v16 + 4;
  if ( !v19 )
    v20 = v16;
  v21 = v18 & 0xFFFFFFBF;
  v22 = v18 & 0x40;
  if ( (v18 & 0x40) == 0 )
    v21 = v18;
  v23 = v20 + 4;
  if ( !v22 )
    v23 = v20;
  v24 = v21 & 0xFFFFFF7F;
  if ( (v21 & 0x80) == 0 )
    v24 = v21;
  if ( (v24 & 0xFFFFF0FF) != 0 )
    Logger.g->Panic("SGepard::CreateDynamicVB: Unsupported FVF");
  v25 = v23 + 4;
  if ( (v21 & 0x80) == 0 )
    v25 = v23;
  *(int *)((char *)&this->DynamicVBs.array->data.VertexSize + v29) = v25 + 8 * ((v24 >> 8) & 0xF);
  *(&this->DynamicVBs.array->data.Locked + v29) = 0;
  *(int *)((char *)&this->DynamicVBs.array->data.LockPosition + v29) = 0;
  v26 = this->lpD3DDev->CreateVertexBuffer(
          *(unsigned int *)((char *)&this->DynamicVBs.array->data.NumVertices + v29)
        * *(int *)((char *)&this->DynamicVBs.array->data.VertexSize + v29),
          520u,
          *(unsigned int *)((char *)&this->DynamicVBs.array->data.VertexFormat + v29),
          D3DPOOL_DEFAULT,
          (IDirect3DVertexBuffer9 **)((char *)&this->DynamicVBs.array->data + v29),
          0);
  if ( v26 )
  {
    v28 = DXGetErrorStringA(v26);
    sprintf(atmstr, "%s: %s", "SGepard::CreateDynamicVB\\IDirect3DDevice9::CreateVertexBuffer", v28);
    Logger.g->Panic(atmstr);
  }
  return v30;
}

//----- (0042B580) --------------------------------------------------------

void SGepard::CreateEffectsHandler()
{
  char rtname[200];

  this->Tileset = 0;
  this->GlobalisSzelirany = ((float)rand() / 32767.0f) * 6.283f;

  SEffect *fx = new SEffect(this);
  this->Effect = fx;

  this->_izzo_robbanas_darabok = fx->LoadEffect("izzo_robbanas_darabok", 0, -1.0f, -1.0f);
  this->_izzo_robbanas_darabok2 = fx->LoadEffect("izzo_robbanas_darabok2", 0, -1.0f, -1.0f);
  this->_izzo_robbanas_darabok3 = fx->LoadEffect("izzo_robbanas_darabok3", 0, -1.0f, -1.0f);
  this->_kis_robbanas_darabok = fx->LoadEffect("kis_robbanas_darabok", 0, -1.0f, -1.0f);
  this->_shader_light = fx->LoadEffect("shader_light", 0, -1.0f, -1.0f);
  this->_shader_light_long = fx->LoadEffect("shader_light_long", 0, -1.0f, -1.0f);
  this->_becsapodas_anim_small = fx->LoadEffect("becsapodas_anim_small", 0, -1.0f, -1.0f);
  this->_becsapodas_anim_medium = fx->LoadEffect("becsapodas_anim_medium", 0, -1.0f, -1.0f);
  this->_robbanas_anim = fx->LoadEffect("robbanas_anim", 0, -1.0f, -1.0f);
  this->_robbanas_anim2 = fx->LoadEffect("robbanas_anim2", 0, -1.0f, -1.0f);
  this->_lovegtorony_darabok = fx->LoadEffect("lovegtorony_darabok", 0, -1.0f, -1.0f);
  this->_lovegtorony_robbanas_anim = fx->LoadEffect("lovegtorony_robbanas_anim", 0, -1.0f, -1.0f);
  this->_lovegtorony_robbanas_anim2 = fx->LoadEffect("lovegtorony_robbanas_anim2", 0, -1.0f, -1.0f);
  this->_robbanas_anim_fuel = fx->LoadEffect("robbanas_anim_fuel", 0, -1.0f, -1.0f);
  this->_tuzijatek_robbanas_anim = fx->LoadEffect("tuzijatek_robbanas_anim", 0, -1.0f, -1.0f);
  this->_tuzijatek_robbanas_anim2 = fx->LoadEffect("tuzijatek_robbanas_anim2", 0, -1.0f, -1.0f);
  this->_nagy_robbanas_darabok = fx->LoadEffect("nagy_robbanas_darabok", 0, -1.0f, -1.0f);
  this->_egyseg_alatti_talaj_koszolas1 = fx->LoadEffect("egyseg_alatti_talaj_koszolas1", 0, -1.0f, -1.0f);
  this->_egyseg_alatti_talaj_koszolas2 = fx->LoadEffect("egyseg_alatti_talaj_koszolas2", 0, -1.0f, -1.0f);
  this->_egyseg_alatti_talaj_koszolas3 = fx->LoadEffect("egyseg_alatti_talaj_koszolas3", 0, -1.0f, -1.0f);
  this->_egyseg_alatti_talaj_koszolas_rabbit = fx->LoadEffect("egyseg_alatti_talaj_koszolas_rabbit", 0, -1.0f, -1.0f);
  this->_egyseg_alatti_talaj_koszolas_pig = fx->LoadEffect("egyseg_alatti_talaj_koszolas_pig", 0, -1.0f, -1.0f);
  this->_tank_foldbeloves_particles = fx->LoadEffect("tank_foldbeloves_particles", 0, -1.0f, -1.0f);
  this->_tank_foldbeloves_kis_darabok = fx->LoadEffect("tank_foldbeloves_kis_darabok", 0, -1.0f, -1.0f);
  this->_tankloves_alatti_talaj_koszolas1 = fx->LoadEffect("tankloves_alatti_talaj_koszolas1", 0, -1.0f, -1.0f);
  this->_tankloves_alatti_talaj_koszolas2 = fx->LoadEffect("tankloves_alatti_talaj_koszolas2", 0, -1.0f, -1.0f);
  this->_tankloves_alatti_talaj_koszolas3 = fx->LoadEffect("tankloves_alatti_talaj_koszolas3", 0, -1.0f, -1.0f);
  this->_raketas_foldbeloves_particles = fx->LoadEffect("raketas_foldbeloves_particles", 0, -1.0f, -1.0f);
  this->_raketas_foldbeloves_kis_darabok = fx->LoadEffect("raketas_foldbeloves_kis_darabok", 0, -1.0f, -1.0f);
  this->_raketas_foldbeloves_nagy_darabok = fx->LoadEffect("raketas_foldbeloves_nagy_darabok", 0, -1.0f, -1.0f);
  this->_raketa_alatti_talaj_koszolas1 = fx->LoadEffect("raketa_alatti_talaj_koszolas1", 0, -1.0f, -1.0f);
  this->_raketa_alatti_talaj_koszolas2 = fx->LoadEffect("raketa_alatti_talaj_koszolas2", 0, -1.0f, -1.0f);
  this->_rocket_boom = fx->LoadEffect("raketa_robbanas_anim", 0, -1.0f, -1.0f);
  this->_rocket_boom2 = fx->LoadEffect("raketa_robbanas_anim2", 0, -1.0f, -1.0f);
  this->_becsapodas_fust = fx->LoadEffect("becsapodas_fust", 0, -1.0f, -1.0f);
  this->_heli_celfust = fx->LoadEffect("heli_celfust", 0, -1.0f, -1.0f);
  this->_becsapodo_darab_nyom1 = fx->LoadEffect("becsapodo_darab_nyom1", 0, -1.0f, -1.0f);
  this->_becsapodo_darab_nyom2 = fx->LoadEffect("becsapodo_darab_nyom2", 0, -1.0f, -1.0f);
  this->_becsapodo_darab_nyom3 = fx->LoadEffect("becsapodo_darab_nyom3", 0, -1.0f, -1.0f);
  this->_becsapodo_darab_nyom4 = fx->LoadEffect("becsapodo_darab_nyom4", 0, -1.0f, -1.0f);
  this->_rain = fx->LoadEffect("rain", 0, -1.0f, -1.0f);
  this->_snowfall = fx->LoadEffect("snowfall", 0, -1.0f, -1.0f);
  this->_lighting = fx->LoadEffect("Lighting", 0, -1.0f, -1.0f);
  this->_egysegfust_gyenge = fx->LoadEffect("egysegfust_gyenge", 0, -1.0f, -1.0f);
  this->_egysegfust_eros = fx->LoadEffect("egysegfust_eros", 0, -1.0f, -1.0f);
  this->_raketa_robbanas_talaj_feny = fx->LoadEffect("raketa_robbanas_talaj_feny", 0, -1.0f, -1.0f);
  this->_torkolattuz_talaj_feny = fx->LoadEffect("torkolattuz_talaj_feny", 0, -1.0f, -1.0f);
  this->_torkolattuz_talaj_feny2 = fx->LoadEffect("torkolattuz_talaj_feny2", 0, -1.0f, -1.0f);
  this->_gepfegyveres_kozepso_torkolattuz = fx->LoadEffect("gepfegyveres_kozepso_torkolattuz", 0, -1.0f, -1.0f);
  this->_gepfegyveres_oldalso_torkolattuz = fx->LoadEffect("gepfegyveres_oldalso_torkolattuz", 0, -1.0f, -1.0f);
  this->_loveg_torkolattuz = fx->LoadEffect("loveg_torkolattuz", 0, -1.0f, -1.0f);
  this->_tank_torkolattuz = fx->LoadEffect("tank_torkolattuz", 0, -1.0f, -1.0f);
  this->_torkolat_fust = fx->LoadEffect("torkolat_fust", 0, -1.0f, -1.0f);
  this->_normal_porzas = fx->LoadEffect("normal porzas", 0, -1.0f, -1.0f);
  this->_desert_porzas = fx->LoadEffect("desert porzas", 0, -1.0f, -1.0f);
  this->_rock_porzas = fx->LoadEffect("rock porzas", 0, -1.0f, -1.0f);
  this->_a_foldbe_csapodo_darabok_fustje = fx->LoadEffect("a_foldbe_csapodo_darabok_fustje", 0, -1.0f, -1.0f);
  this->_kis_kor = fx->LoadEffect("kis_kor", 0, -1.0f, -1.0f);
  this->_bomba_lokeshullam = fx->LoadEffect("bomba_lokeshullam", 0, -1.0f, -1.0f);
  this->_uzemanyag_lokeshullam = fx->LoadEffect("uzemanyag_lokeshullam", 0, -1.0f, -1.0f);
  this->_glow = fx->LoadEffect("glow", 0, -1.0f, -1.0f);
  this->_unit_reflektor = fx->LoadEffect("unit_reflektor", 0, -1.0f, -1.0f);
  this->_szintlepes_csillagok = fx->LoadEffect("szintlepes_csillagok", 0, -1.0f, -1.0f);
  this->_szintlepes_csillagok2 = fx->LoadEffect("szintlepes_csillagok2", 0, -1.0f, -1.0f);
  this->_levelup_lens = fx->LoadEffect("levelup_lens", 0, -1.0f, -1.0f);
  this->_ablak_mozgas = fx->LoadEffect("ablak_mozgas", 0, -1.0f, -1.0f);
  this->_eryngo = fx->LoadEffect("eryngo", 0, -1.0f, -1.0f);
  this->_eryngo_porzas = fx->LoadEffect("eryngo porzas", 0, -1.0f, -1.0f);
  this->_objektum_fust = fx->LoadEffect("objektum_fust", 0, -1.0f, -1.0f);

  this->_robbanastipusok[0].index = -1;
  for (int i = 1; i < 100; ++i) {
    sprintf(rtname, "robbanastipus %d", i);
    const char *mesh1 = fx->EffectsINI->GetString(rtname, "Mesh1", "nincs");
    if (strcmp(mesh1, "nincs") != 0)
      this->_robbanastipusok[i].index = fx->LoadEffect(rtname, 0, -1.0f, -1.0f);
    else
      this->_robbanastipusok[i].index = -1;
    this->_robbanastipusok[i].hangindex = fx->EffectsINI->GetInt(rtname, "Hang", 0);
    this->_robbanastipusok[i].fust = fx->EffectsINI->GetInt(rtname, "Fust", 0);
  }
}

//----- (0042C240) --------------------------------------------------------

int SGepard::CreateEmptyTexture(unsigned int width, unsigned int height, int depth, bool mipmaps, bool forcePow2)

{
  SHeap<STextureProp> *p_Textures; // ebx
  int v7;
  SHeap<STextureProp>::__Tstruct *array; // esi
  int v9;
  SGepard *v10; // ecx
  D3DFORMAT TFHiAlpha;
  HRESULT v12;
  const char *v14; // eax
  int v15;
  char atmstr[200];
  if ( width == 0 || height == 0 )
  {
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "SGepard::CreateEmptyTexture: skipping 0x0 texture (depth=%d)", depth);
    #endif
    return -1;
  }
  p_Textures = &this->Textures;
  v7 = this->Textures.Add();
  array = p_Textures->array;
  v15 = v7;
  v9 = v7;
  array[v9].data.FileName = _strdup("**unique**");
  p_Textures->array[v9].data.RefCount = 1;
  p_Textures->array[v9].data.Width = width;
  p_Textures->array[v9].data.Height = height;
  if ( forcePow2 )
  {
    if ( ((width - 1) & width) != 0 )
 Logger.g->Panic(
        "SGepard::CreateEmptyTexture: (%d x %d): Width must be power of 2",
        width,
        height);
    if ( ((height - 1) & height) != 0 )
 Logger.g->Panic(
        "SGepard::CreateEmptyTexture: (%d x %d): Height must be power of 2",
        width,
        height);
  }
  switch ( depth )
  {
    case 32:
      v10 = this;
      p_Textures->array[v9].data.Alpha = 2;
      TFHiAlpha = this->TFHiAlpha;
      break;
    case 24:
      v10 = this;
      p_Textures->array[v9].data.Alpha = 0;
      TFHiAlpha = this->TFHiOpaque;
      break;
    case 8:
      v10 = this;
      p_Textures->array[v9].data.Alpha = 2;
      TFHiAlpha = this->TFAlpha;
      break;
    case 5:
      v10 = this;
      p_Textures->array[v9].data.Alpha = 1;
      TFHiAlpha = this->TF1Bit;
      break;
    case 4:
      v10 = this;
      p_Textures->array[v9].data.Alpha = 0;
      TFHiAlpha = this->TFOpaque;
      break;
    default:
      Logger.g->Panic("SGepard::CreateEmptyTexture: %d: Depth unsupported", depth);
  }
  v12 = v10->lpD3DDev->CreateTexture(width,
          height,
          !mipmaps,
          0,
          TFHiAlpha,
          D3DPOOL_MANAGED,
          &p_Textures->array[v9].data.lpTexture,
          0);
  if ( v12 )
  {
    v14 = DXGetErrorStringA(v12);
    sprintf(atmstr, "%s: %s", "SGepard::CreateEmptyTexture: CreateTexture", v14);
    Logger.g->Panic(atmstr);
  }
  return v15;
}

//----- (0042C4D0) --------------------------------------------------------

int SGepard::CreateGroundTrail(int texture_idx, float strength, float fade_time, float u_scale, float v_scale, SDrawType drawtype)

{
  int nextempty;
  SHeap<SGroundTrail>::__Tstruct *array; // ecx
  int size;
  int maxsize;
  int v12;
  SHeap<SGroundTrail>::__Tstruct *v13; // eax
  int v14;
  SHeap<STextureProp>::__Tstruct *v15; // edx
  int v16;
  SHeap<SGroundTrailSegment> *v17; // eax
  SHeap<SGroundTrailSegment> *v18; // ecx
  int result;
  ++this->GroundTrails.occupied;
  nextempty = this->GroundTrails.nextempty;
  if ( nextempty < 0 )
  {
    size = this->GroundTrails.size;
    maxsize = this->GroundTrails.maxsize;
    if ( size == maxsize )
    {
      if ( maxsize >= 16 )
        v12 = 6 * maxsize / 5;
      else
        v12 = 16;
      // x64: literal `52` is x86 sizeof(SHeap<SGroundTrail>::__Tstruct). Same family as group.cpp:1884.
      v13 = (SHeap<SGroundTrail>::__Tstruct *)realloc(this->GroundTrails.array, sizeof(SHeap<SGroundTrail>::__Tstruct) * v12);
      v14 = this->GroundTrails.maxsize;
      this->GroundTrails.array = v13;
      memset(&v13[v14], 0, sizeof(SHeap<SGroundTrail>::__Tstruct) * (v12 - v14));
      size = this->GroundTrails.size;
      this->GroundTrails.maxsize = v12;
    }
    this->GroundTrails.array[size].use = 0x7FFFFFFF;
    nextempty = this->GroundTrails.size;
    this->GroundTrails.size = nextempty + 1;
  }
  else
  {
    array = this->GroundTrails.array;
    this->GroundTrails.nextempty = array[nextempty].use;
    array[nextempty].use = 0x7FFFFFFF;
    memset(&this->GroundTrails.array[nextempty].data, 0, sizeof(this->GroundTrails.array[nextempty].data));
  }
  if ( texture_idx >= 0 && texture_idx < this->Textures.size )
  {
    v15 = this->Textures.array;
    if ( v15[texture_idx].use == 0x7FFFFFFF )
      ++v15[texture_idx].data.RefCount;
  }
  v16 = nextempty;
  this->GroundTrails.array[nextempty].data.TextureIndex = texture_idx;
  // x64 fix: literal 0x14 (= 20) was x86 sizeof(SHeap); x64 sizeof grows
  // because the leading T* array pointer is 8 bytes instead of 4.
  v17 = (SHeap<SGroundTrailSegment> *)operator new(sizeof(SHeap<SGroundTrailSegment>));
  v18 = v17;
  if ( v17 )
  {
    v17->array = 0;
    v17->size = 0;
    v17->maxsize = 0;
    v17->nextempty = -1;
    v17->occupied = 0;
  }
  else
  {
    v18 = 0;
  }
  this->GroundTrails.array[v16].data.Segments = v18;
  this->GroundTrails.array[v16].data.AutoDestruct = 0;
  this->GroundTrails.array[v16].data.DrawType = drawtype;
  this->GroundTrails.array[v16].data.Strength = strength * 256.0f;
  result = nextempty;
  this->GroundTrails.array[v16].data.FadeTime = fade_time;
  this->GroundTrails.array[v16].data.UScale = u_scale;
  this->GroundTrails.array[v16].data.VScale = v_scale;
  this->GroundTrails.array[v16].data.HaveFirst = 0;
  return result;
}

//----- (0042C6A0) --------------------------------------------------------

void SGepard::CreateHill(SHillRing **_hillring)

{
  SHillRing *v3; // esi
  SHillRing *last; // eax
  _DWORD *v5; // eax
  v3 = (SHillRing *)operator new(sizeof(SHillRing));
  memset(&v3->prev , 0, 16);
  memset(&v3->WaterEnabled , 0, 16);
  last = this->HillChain.last;
  if ( last )
  {
    last->next = v3;
    v3->prev = this->HillChain.last;
    this->HillChain.last = v3;
    v3->next = 0;
  }
  else
  {
    this->HillChain.last = v3;
    this->HillChain.first = v3;
  }
  ++this->HillChain.NumItems;
  // x64 fix: original `malloc(0x14u)` allocated 20 bytes (x86 sizeof of
  // SChain<SSplineRing>) and the byte-offset writes (`*v5=0; v5[1]=0;
  // v5[2]=0; *((_BYTE*)v5+12)=0; v5[4]=0`) assumed x86 layout. On x64 the
  // 3 pointers grow to 8 bytes and Closed/NumItems shift, so the writes
  // landed on wrong fields and the chain was undersized. Use typed alloc
  // + typed init.
  {
    SChain<SSplineRing> *sc = new SChain<SSplineRing>();
    if ( sc )
    {
      sc->first = nullptr;
      sc->last = nullptr;
      sc->current = nullptr;
      sc->Closed = false;
      sc->NumItems = 0;
    }
    v3->SplineChain = sc;
  }
  v3->RiverSize = 3.0;
  *_hillring = v3;
}

//----- (0042C740) --------------------------------------------------------

int SGepard::CreateLake(int texture_idx, float x, float y, float z, int flow, int sparkle)

{
  int nextempty;
  SHeap<SGLake>::__Tstruct *array; // ecx
  int v10;
  SHeap<SGLake>::__Tstruct *v11; // eax
  int maxsize;
  int size;
  int v14;
  SHeap<SGLake>::__Tstruct *v15; // eax
  int v16;
  SHeap<STextureProp>::__Tstruct *v17; // edx
  int v18;
  SHeap<SGLake>::__Tstruct *v19; // esi
  float v20;
  SGepard *v21; // edx
  int v22;
  int i;
  int v24;
  int v25;
  bool v26; // cc
  char *v27; // edx
  int v28;
  double v29; // st7
  int v30;
  int v31;
  int v32;
  double v33; // st7
  int v34;
  int XSize;
  int v36;
  int v37;
  int v38;
  unsigned char *v39; // eax
  int v40;
  int v41;
  int v42;
  int v43;
  bool v44;
  unsigned int v45;
  unsigned char *v46; // esi
  unsigned char *v47; // edx
  unsigned char v48; // ah
  unsigned char v49; // ah
  unsigned char v50; // ah
  unsigned char v51; // ah
  SMesh *v52; // eax
  SMesh *v53; // eax
  SMesh *v54; // esi
  SMesh *v55; // edi
  SMesh *v56; // eax
  SMesh *v57; // eax
  int v58;
  unsigned char *v59; // eax
  int v60;
  int v61;
  SMesh *v62; // edi
  int v63;
  int v64;
  unsigned char *v65; // eax
  int v66;
  int v67;
  int v68;
  int v69;
  int v70;
  int v71;
  unsigned char *v72; // eax
  int v73;
  int v74;
  unsigned char *v75; // ecx
  double v76; // st7
  float v77; // xmm3_4
  float v78; // xmm0_4
  float v79; // xmm2_4
  unsigned int v80;
  float v81; // xmm0_4
  float v82; // xmm0_4
  SHeap<SGLake>::__Tstruct *v83; // edx
  int v84;
  int v85;
  int v86;
  SLakeBlock *v87; // eax
  int v88;
  int v89;
  SGepard *v90; // edx
  int height;
  unsigned short heighta;
  int heightb;
  int heightc;
  int heightd;
  int heighte;
  int heightf;
  int heightg;
  float heighth;
  unsigned short v101;
  int v102;
  int v103;
  int v104;
  int v105;
  int v106;
  int v107;
  int v108;
  int v109;
  float v110;
  int v111;
  int v112;
  int v113;
  int v114;
  int v115;
  int v116;
  int v117;
  int v118;
  int v119;
  unsigned char *v120;
  int v121;
  int v122;
  int v123;
  int v124;
  int v125;
  int v126;
  int v127;
  int v128;
  int v129;
  int v130;
  int v131;
  int v132;
  int v133;
  int v134;
  SMesh *v135;
  unsigned int numvertices;
  unsigned char *numverticesa;
  int v138;
  unsigned char *v139;
  char *block;
  unsigned char *v141;
  int v142;
  int texture_idxa;
  unsigned char *texture_idxb;
  SMesh *texture_idxc;
  char texture_idx_3;
  unsigned char texture_idx_3a;
  unsigned char texture_idx_3b;
  unsigned char texture_idx_3c;
  int xa;
  int xb;
  int xc;
  int xd;
  SHeap<SGLake>::__Tstruct *xe;
  unsigned char x_3;
  unsigned char x_3a;
  unsigned char x_3b;
  int za;
  float zc;
  float zd;
  float ze;
  int zb;
  unsigned char z_3;
  unsigned char z_3a;
  int flowa;
  int flowb;
  int flowc;
  unsigned char flow_3;
  unsigned char flow_3a;
  unsigned char flow_3b;
  nextempty = this->Lakes.nextempty;
  ++this->Lakes.occupied;
  v109 = nextempty;
  if ( nextempty < 0 )
  {
    maxsize = this->Lakes.maxsize;
    size = this->Lakes.size;
    if ( size == maxsize )
    {
      if ( maxsize >= 16 )
        v14 = 6 * maxsize / 5;
      else
        v14 = 16;
      // x64: literal `40` is x86 sizeof(SHeap<SGLake>::__Tstruct). Same family as group.cpp:1884.
      v15 = (SHeap<SGLake>::__Tstruct *)realloc(this->Lakes.array, sizeof(SHeap<SGLake>::__Tstruct) * v14);
      v16 = this->Lakes.maxsize;
      this->Lakes.array = v15;
      memset(&v15[v16], 0, sizeof(SHeap<SGLake>::__Tstruct) * (v14 - v16));
      size = this->Lakes.size;
      this->Lakes.maxsize = v14;
    }
    this->Lakes.array[size].use = 0x7FFFFFFF;
    nextempty = this->Lakes.size;
    v109 = nextempty;
    this->Lakes.size = nextempty + 1;
  }
  else
  {
    array = this->Lakes.array;
    v10 = nextempty;
    this->Lakes.nextempty = array[nextempty].use;
    array[v10].use = 0x7FFFFFFF;
    v11 = this->Lakes.array;
    memset(&v11[v10].data.TextureIndex , 0, 16);
    memset(&v11[v10].data.Blocks.size , 0, 16);
    v11[v10].data.Sparkle = 0;
  }
  if ( texture_idx >= 0 && texture_idx < this->Textures.size )
  {
    v17 = this->Textures.array;
    if ( v17[texture_idx].use == 0x7FFFFFFF )
      ++v17[texture_idx].data.RefCount;
  }
  v18 = nextempty;
  // x64: literal `40` is x86 sizeof(SHeap<SGLake>::__Tstruct). On x64 SGLake
  // grew (SDArray<SLakeBlock> went 12→16, +4 padding before that), so the
  // Element is 48. v134 is consumed below as a byte offset into Lakes.array.
  v134 = v18 * (int)sizeof(SHeap<SGLake>::__Tstruct);
  this->Lakes.array[v18].data.TextureIndex = texture_idx;
  this->Lakes.array[v18].data.Y = y;
  this->Lakes.array[v18].data.Flow = flow;
  this->Lakes.array[v18].data.Sparkle = sparkle;
  if ( sparkle )
  {
    v19 = &this->Lakes.array[v18];
    v19->data.TextureIndex2 = this->LoadTexture("fx/ocean/water2.tga", 1, 1);
  }
  else
  {
    this->Lakes.array[v18].data.TextureIndex2 = -1;
  }
  v20 = x;
  flowa = 0;
  xa = 0;
  block = new char[4 * (this->XSize + 1) * (this->ZSize + 1)];
  memset(block, 0, 4 * (this->XSize + 1) * (this->ZSize + 1));
  v21 = this;
  dword_59778C[0] = (int)(z + 0.5);
  path_buffer[0].x = (int)(v20 + 0.5);
  v22 = 1;
  *(_DWORD *)&block[4 * path_buffer[0].x + 4 * dword_59778C[0] * (this->XSize + 1)] = 1;
  for ( i = 0; i != v22; flowa = i )
  {
    if ( ++xa > 25000 )
      break;
    v24 = path_buffer[i].x;
    v25 = dword_59778C[2 * i];
    texture_idxa = v25;
    if ( v24 > 0 )
    {
      v26 = v24 < v21->XSize;
      v21 = this;
      if ( v26 && v25 > 0 && v25 < this->ZSize )
      {
        za = v25 - 1;
        v27 = block;
        if ( !*(_DWORD *)&block[4 * v24 + 4 * (v25 - 1) * (this->XSize + 1)] )
        {
          v110 = (float)(this->Terrain->GetHeight(v24, v25 - 1));

          if ( y >= v110 )
          {
            dword_59778C[2 * v22] = za;
            path_buffer[v22].x = v24;
            v22 = ((_WORD)v22 + 1) & 0x7FF;
            *(_DWORD *)&block[4 * v24 + 4 * za * (this->XSize + 1)] = 1;
            if ( v22 == flowa )
LABEL_146:
              Logger.g->Panic("SGepard::CreateLake: Buffer overrun");
          }
          v27 = block;
        }
        v28 = texture_idxa;
        if ( !*(_DWORD *)&v27[4 * v24 + 4 + 4 * texture_idxa * (this->XSize + 1)] )
        {
          v29 = this->Terrain->GetHeight(v24 + 1, texture_idxa);
          v28 = texture_idxa;
          zc = (float)(v29);

          if ( y >= zc )
          {
            path_buffer[v22].x = v24 + 1;
            dword_59778C[2 * v22] = texture_idxa;
            v22 = ((_WORD)v22 + 1) & 0x7FF;
            *(_DWORD *)&block[4 * v24 + 4 + 4 * texture_idxa * (this->XSize + 1)] = 1;
            if ( v22 == flowa )
              goto LABEL_146;
          }
        }
        v30 = v28 + 1;
        v111 = v28 + 1;
        v31 = 4 * v24 + 4 * (v28 + 1) * (this->XSize + 1);
        v32 = texture_idxa;
        if ( !*(_DWORD *)&block[v31] )
        {
          zd = (float)(this->Terrain->GetHeight(v24, v30));

          if ( y >= zd )
          {
            dword_59778C[2 * v22] = v111;
            path_buffer[v22].x = v24;
            v22 = ((_WORD)v22 + 1) & 0x7FF;
            *(_DWORD *)&block[4 * v24 + 4 * v111 * (this->XSize + 1)] = 1;
            if ( v22 == flowa )
              goto LABEL_146;
          }
          v32 = texture_idxa;
        }
        if ( *(_DWORD *)&block[4 * v24 - 4 + 4 * v32 * (this->XSize + 1)] )
        {
          v21 = this;
        }
        else
        {
          v33 = this->Terrain->GetHeight(v24 - 1, v32);
          v21 = this;
          ze = (float)(v33);

          if ( y >= ze )
          {
            path_buffer[v22].x = v24 - 1;
            dword_59778C[2 * v22] = texture_idxa;
            v22 = ((_WORD)v22 + 1) & 0x7FF;
            *(_DWORD *)&block[4 * v24 - 4 + 4 * texture_idxa * (this->XSize + 1)] = 1;
            *(unsigned short*)&i = (unsigned short)(flowa);
            if ( v22 == flowa )
              goto LABEL_146;
            goto LABEL_39;
          }
        }
        *(unsigned short*)&i = (unsigned short)(flowa);
      }
    }
LABEL_39:
    i = ((_WORD)i + 1) & 0x7FF;
  }
  v34 = 0;
  v121 = 0;
  v123 = (int)(float)(fminf(1.0, v21->AmbientColorVal.b - (float)(v21->SunColorVal.b * v21->SunDir.y)) * 255.0) | (((int)(float)(fminf(1.0, v21->AmbientColorVal.g - (float)(v21->SunColorVal.g * v21->SunDir.y)) * 255.0) | ((int)(float)(fminf(1.0, v21->AmbientColorVal.r - (float)(v21->SunColorVal.r * v21->SunDir.y)) * 255.0) << 8)) << 8);
  *(unsigned int *)((char *)&v21->Lakes.array->data.Color + v134) = v123;
  XSize = v21->XSize;
  if ( this->ZSize / 8 * (XSize / 8) <= 0 )
    goto LABEL_145;
  while ( 2 )
  {
    v122 = 8 * (v34 / (XSize / 8));
    v133 = 8 * (v34 % (XSize / 8));
    v138 = 0;
    memset(block_grid, 0, sizeof(block_grid));
    v36 = v122;
    xb = v122;
    // IDA __OFSUB__(v122, v122+8) = signed overflow of -8 = always false; removed
    v37 = v133;
    v38 = v122 + 8;
    v39 = &block_grid[-v133 + 11];
    texture_idxb = &block_grid[-v133 + 11];
    v40 = v133 + 8;
    while ( 1 )
    {
      if ( v133 < v40 )
      {
        do
        {
          v41 = this->XSize + 1;
          v42 = v37 + (v36 + 1) * v41;
          v43 = *(_DWORD *)&block[4 * v37 + 4 * v36 * v41] | *(_DWORD *)&block[4 * v42 + 4] | *(_DWORD *)&block[4 * v37 + 4 + 4 * v36 * v41];
          v36 = xb;
          v44 = (((unsigned char)block[4 * v42] | (unsigned char)v43) & 1) == 0;
          v39 = texture_idxb;
          if ( !v44 )
          {
            ++v138;
            texture_idxb[v37] = 1;
          }
          ++v37;
          v40 = v133 + 8;
        }
        while ( v37 < v133 + 8 );
        v38 = v122 + 8;
      }
      ++v36;
      v39 += 10;
      xb = v36;
      texture_idxb = v39;
      if ( v36 >= v38 )
        break;
      v37 = v133;
    }
    if ( !v138 )
    {
      v90 = this;
      goto LABEL_144;
    }
    v45 = 0;
    v46 = &vertex_grid[1];
    v47 = &block_grid[11];
    do
    {
      v48 = *v47;
      texture_idx_3 = *(v47 - 10);
      if ( *v47 | (unsigned char)(texture_idx_3 | *(v47 - 1) | *(v47 - 11)) )
        *(v46 - 1) = v45++;
      flow_3 = *(v47 - 9);
      x_3 = v47[1];
      if ( v48 | (unsigned char)(texture_idx_3 | flow_3 | x_3) )
        *v46 = v45++;
      v49 = *(v47 - 8);
      z_3 = v47[2];
      if ( flow_3 | (unsigned char)(x_3 | v49 | z_3) )
        v46[1] = v45++;
      texture_idx_3a = *(v47 - 7);
      flow_3a = v47[3];
      if ( v49 | (unsigned char)(z_3 | texture_idx_3a | flow_3a) )
        v46[2] = v45++;
      v50 = *(v47 - 6);
      x_3a = v47[4];
      if ( texture_idx_3a | (unsigned char)(flow_3a | v50 | x_3a) )
        v46[3] = v45++;
      texture_idx_3b = *(v47 - 5);
      flow_3b = v47[5];
      if ( v50 | (unsigned char)(x_3a | texture_idx_3b | flow_3b) )
        v46[4] = v45++;
      x_3b = v47[6];
      z_3a = *(v47 - 4);
      if ( texture_idx_3b | (unsigned char)(flow_3b | z_3a | x_3b) )
        v46[5] = v45++;
      v51 = *(v47 - 3);
      texture_idx_3c = v47[7];
      if ( z_3a | (unsigned char)(x_3b | v51 | texture_idx_3c) )
        v46[6] = v45++;
      if ( v51 | (unsigned char)(texture_idx_3c | *(v47 - 2) | v47[8]) )
        v46[7] = v45++;
      v47 += 10;
      v46 += 9;
    }
    while ( (int)v47 <= (int)&block_grid[91] );
    numvertices = v45;
    v52 = (SMesh *)operator new(sizeof(SMesh));
    if ( v52 )
    {
      v53 = new (v52) SMesh(this, 0x41u);
      v54 = v53;
      v135 = v53;
    }
    else
    {
      v54 = 0;
      v135 = 0;
    }
    v55 = 0;
    texture_idxc = 0;
    if ( sparkle )
    {
      v56 = (SMesh *)operator new(sizeof(SMesh));
      if ( v56 )
      {
        v57 = new (v56) SMesh(this, 0x42u);
        v55 = v57;
        texture_idxc = v57;
      }
      else
      {
        v55 = 0;
        texture_idxc = 0;
      }
    }
    v54->CreateIndexBuffer(0, 6 * v138);
    if ( sparkle )
      v55->CreateIndexBuffer(0, 6 * v138);
    v142 = 10;
    v58 = 0;
    zb = 4;
    xc = 3;
    flowb = 2;
    v141 = &vertex_grid[10];
    v139 = &block_grid[12];
    do
    {
      v59 = v141;
      if ( *(v139 - 1) )
      {
        v112 = *(v141 - 1);
        v54->lpIndices[0][v58] = v112;
        v60 = *(v141 - 10);
        v54->lpIndices[0][v58 + 1] = v60;
        v101 = v60;
        v124 = *(v141 - 9);
        v54->lpIndices[0][flowb] = v124;
        v54->lpIndices[0][xc] = v112;
        v54->lpIndices[0][zb] = v124;
        v61 = v142;
        height = *v141;
        *(unsigned short *)((char *)v54->lpIndices[0] + v142) = *v141;
        if ( sparkle )
        {
          texture_idxc->lpIndices[0][v58] = v112;
          texture_idxc->lpIndices[0][v58 + 1] = v101;
          texture_idxc->lpIndices[0][flowb] = v124;
          texture_idxc->lpIndices[0][xc] = v112;
          v61 = v142;
          texture_idxc->lpIndices[0][zb] = v124;
          *(unsigned short *)((char *)texture_idxc->lpIndices[0] + v142) = height;
          v54 = v135;
        }
        flowb += 6;
        v58 += 6;
        xc += 6;
        zb += 6;
        v59 = v141;
        v142 = v61 + 12;
      }
      v62 = texture_idxc;
      if ( *v139 )
      {
        v113 = *v59;
        v54->lpIndices[0][v58] = *v59;
        v63 = *(v141 - 9);
        v54->lpIndices[0][v58 + 1] = v63;
        v125 = *(v141 - 8);
        heighta = v63;
        v54->lpIndices[0][flowb] = v125;
        v54->lpIndices[0][xc] = v113;
        v54 = v135;
        v135->lpIndices[0][zb] = v125;
        v64 = v142;
        v102 = v141[1];
        *(unsigned short *)((char *)v135->lpIndices[0] + v142) = v102;
        v62 = texture_idxc;
        if ( sparkle )
        {
          texture_idxc->lpIndices[0][v58] = v113;
          texture_idxc->lpIndices[0][v58 + 1] = heighta;
          texture_idxc->lpIndices[0][flowb] = v125;
          texture_idxc->lpIndices[0][xc] = v113;
          v64 = v142;
          texture_idxc->lpIndices[0][zb] = v125;
          v62 = texture_idxc;
          *(unsigned short *)((char *)texture_idxc->lpIndices[0] + v142) = v102;
          v54 = v135;
        }
        flowb += 6;
        v58 += 6;
        xc += 6;
        zb += 6;
        v142 = v64 + 12;
      }
      v65 = v139;
      if ( v139[1] )
      {
        v114 = v141[1];
        v54->lpIndices[0][v58] = v114;
        heightb = *(v141 - 8);
        v54->lpIndices[0][v58 + 1] = heightb;
        v126 = *(v141 - 7);
        v54->lpIndices[0][flowb] = v126;
        v54->lpIndices[0][xc] = v114;
        v54->lpIndices[0][zb] = v126;
        v66 = v142;
        v103 = v141[2];
        *(unsigned short *)((char *)v54->lpIndices[0] + v142) = v103;
        v62 = texture_idxc;
        if ( sparkle )
        {
          texture_idxc->lpIndices[0][v58] = v114;
          texture_idxc->lpIndices[0][v58 + 1] = heightb;
          texture_idxc->lpIndices[0][flowb] = v126;
          texture_idxc->lpIndices[0][xc] = v114;
          v66 = v142;
          texture_idxc->lpIndices[0][zb] = v126;
          v62 = texture_idxc;
          *(unsigned short *)((char *)texture_idxc->lpIndices[0] + v142) = v103;
          v54 = v135;
        }
        flowb += 6;
        v58 += 6;
        xc += 6;
        zb += 6;
        v65 = v139;
        v142 = v66 + 12;
      }
      if ( v65[2] )
      {
        v115 = v141[2];
        v54->lpIndices[0][v58] = v115;
        heightc = *(v141 - 7);
        v54->lpIndices[0][v58 + 1] = heightc;
        v127 = *(v141 - 6);
        v54->lpIndices[0][flowb] = v127;
        v54->lpIndices[0][xc] = v115;
        v54->lpIndices[0][zb] = v127;
        v67 = v142;
        v104 = v141[3];
        *(unsigned short *)((char *)v54->lpIndices[0] + v142) = v104;
        v62 = texture_idxc;
        if ( sparkle )
        {
          texture_idxc->lpIndices[0][v58] = v115;
          texture_idxc->lpIndices[0][v58 + 1] = heightc;
          texture_idxc->lpIndices[0][flowb] = v127;
          texture_idxc->lpIndices[0][xc] = v115;
          v67 = v142;
          texture_idxc->lpIndices[0][zb] = v127;
          v62 = texture_idxc;
          *(unsigned short *)((char *)texture_idxc->lpIndices[0] + v142) = v104;
          v54 = v135;
        }
        flowb += 6;
        v58 += 6;
        xc += 6;
        zb += 6;
        v65 = v139;
        v142 = v67 + 12;
      }
      if ( v65[3] )
      {
        v116 = v141[3];
        v54->lpIndices[0][v58] = v116;
        heightd = *(v141 - 6);
        v54->lpIndices[0][v58 + 1] = heightd;
        v128 = *(v141 - 5);
        v54->lpIndices[0][flowb] = v128;
        v54->lpIndices[0][xc] = v116;
        v54->lpIndices[0][zb] = v128;
        v68 = v142;
        v105 = v141[4];
        *(unsigned short *)((char *)v54->lpIndices[0] + v142) = v105;
        v62 = texture_idxc;
        if ( sparkle )
        {
          texture_idxc->lpIndices[0][v58] = v116;
          texture_idxc->lpIndices[0][v58 + 1] = heightd;
          texture_idxc->lpIndices[0][flowb] = v128;
          texture_idxc->lpIndices[0][xc] = v116;
          v68 = v142;
          texture_idxc->lpIndices[0][zb] = v128;
          v62 = texture_idxc;
          *(unsigned short *)((char *)texture_idxc->lpIndices[0] + v142) = v105;
          v54 = v135;
        }
        flowb += 6;
        v58 += 6;
        xc += 6;
        zb += 6;
        v65 = v139;
        v142 = v68 + 12;
      }
      if ( v65[4] )
      {
        v117 = v141[4];
        v54->lpIndices[0][v58] = v117;
        heighte = *(v141 - 5);
        v54->lpIndices[0][v58 + 1] = heighte;
        v129 = *(v141 - 4);
        v54->lpIndices[0][flowb] = v129;
        v54->lpIndices[0][xc] = v117;
        v54->lpIndices[0][zb] = v129;
        v69 = v142;
        v106 = v141[5];
        *(unsigned short *)((char *)v54->lpIndices[0] + v142) = v106;
        v62 = texture_idxc;
        if ( sparkle )
        {
          texture_idxc->lpIndices[0][v58] = v117;
          texture_idxc->lpIndices[0][v58 + 1] = heighte;
          texture_idxc->lpIndices[0][flowb] = v129;
          texture_idxc->lpIndices[0][xc] = v117;
          v69 = v142;
          texture_idxc->lpIndices[0][zb] = v129;
          v62 = texture_idxc;
          *(unsigned short *)((char *)texture_idxc->lpIndices[0] + v142) = v106;
          v54 = v135;
        }
        flowb += 6;
        v58 += 6;
        xc += 6;
        zb += 6;
        v65 = v139;
        v142 = v69 + 12;
      }
      if ( v65[5] )
      {
        v118 = v141[5];
        v54->lpIndices[0][v58] = v118;
        heightf = *(v141 - 4);
        v54->lpIndices[0][v58 + 1] = heightf;
        v130 = *(v141 - 3);
        v54->lpIndices[0][flowb] = v130;
        v54->lpIndices[0][xc] = v118;
        v54->lpIndices[0][zb] = v130;
        v70 = v142;
        v107 = v141[6];
        *(unsigned short *)((char *)v54->lpIndices[0] + v142) = v107;
        v62 = texture_idxc;
        if ( sparkle )
        {
          texture_idxc->lpIndices[0][v58] = v118;
          texture_idxc->lpIndices[0][v58 + 1] = heightf;
          texture_idxc->lpIndices[0][flowb] = v130;
          texture_idxc->lpIndices[0][xc] = v118;
          v70 = v142;
          texture_idxc->lpIndices[0][zb] = v130;
          v62 = texture_idxc;
          *(unsigned short *)((char *)texture_idxc->lpIndices[0] + v142) = v107;
          v54 = v135;
        }
        flowb += 6;
        v58 += 6;
        xc += 6;
        zb += 6;
        v65 = v139;
        v142 = v70 + 12;
      }
      if ( v65[6] )
      {
        v119 = v141[6];
        v54->lpIndices[0][v58] = v119;
        heightg = *(v141 - 3);
        v54->lpIndices[0][v58 + 1] = heightg;
        v131 = *(v141 - 2);
        v54->lpIndices[0][flowb] = v131;
        v54->lpIndices[0][xc] = v119;
        v54->lpIndices[0][zb] = v131;
        v71 = v142;
        v108 = v141[7];
        *(unsigned short *)((char *)v54->lpIndices[0] + v142) = v108;
        v62 = texture_idxc;
        if ( sparkle )
        {
          texture_idxc->lpIndices[0][v58] = v119;
          texture_idxc->lpIndices[0][v58 + 1] = heightg;
          texture_idxc->lpIndices[0][flowb] = v131;
          texture_idxc->lpIndices[0][xc] = v119;
          v71 = v142;
          texture_idxc->lpIndices[0][zb] = v131;
          v62 = texture_idxc;
          *(unsigned short *)((char *)texture_idxc->lpIndices[0] + v142) = v108;
          v54 = v135;
        }
        flowb += 6;
        v58 += 6;
        xc += 6;
        zb += 6;
        v65 = v139;
        v142 = v71 + 12;
      }
      v141 += 9;
      v139 = v65 + 10;
    }
    while ( (int)(v65 + 10) < (int)&block_grid[92] );
    v54->UnlockIndexBuffer(0);
    if ( sparkle )
      v62->UnlockIndexBuffer(0);
    v54->CreateVertexBuffer(numvertices);
    if ( sparkle )
      v62->CreateVertexBuffer(numvertices);
    v72 = &block_grid[11];
    v73 = 0;
    numverticesa = &block_grid[11];
    while ( 2 )
    {
      v74 = v133;
      v75 = v72;
      xd = v133;
      v120 = v72;
      v132 = 9;
      while ( 2 )
      {
        if ( *v75 | (unsigned char)(*(v75 - 1) | *(v75 - 10) | *(v75 - 11)) )
        {
          v76 = this->Terrain->GetHeight(v74, v122);
          v77 = y;
          v78 = (float)xd;
          v79 = (float)v122;
          *(float *)((char *)v54->lpVertices + v54->OffsetXYZ + v73 * v54->VertexSize) = (float)xd;
          heighth = (float)(v76);

          *(float *)((char *)v54->lpVertices + v54->OffsetXYZ + v73 * v54->VertexSize + 4) = y;
          *(float *)((char *)v54->lpVertices + v54->OffsetXYZ + v73 * v54->VertexSize + 8) = (float)v122;
          if ( heighth > y )
          {
            v80 = v123;
            goto LABEL_130;
          }
          v81 = y - heighth;
          if ( (float)(y - heighth) < 1.0 )
          {
            v82 = v81 * 192.0f;
            goto LABEL_125;
          }
          if ( v81 >= 3.0 )
          {
            v80 = v123 | 0xFF000000;
          }
          else
          {
            v82 = (v81 - 1.0f) * 32.0f + 191.0f;
LABEL_125:
            v79 = (float)v122;
            v77 = y;
            v80 = v123 | ((unsigned int)v82 << 24);
          }
          v78 = (float)xd;
LABEL_130:
          *(_DWORD *)((char *)v54->lpVertices + v54->OffsetDiffuse + v73 * v54->VertexSize) = v80;
          *(_DWORD *)((char *)v54->lpVertices + v73 * v54->VertexSize + v54->OffsetTexture1) = 0;
          *(_DWORD *)((char *)v54->lpVertices + v73 * v54->VertexSize + v54->OffsetTexture1 + 4) = 0;
          if ( sparkle )
          {
            *(float *)((char *)texture_idxc->lpVertices + texture_idxc->OffsetXYZ + v73 * texture_idxc->VertexSize) = v78;
            *(float *)((char *)texture_idxc->lpVertices + texture_idxc->OffsetXYZ + v73 * texture_idxc->VertexSize + 4) = v77;
            *(float *)((char *)texture_idxc->lpVertices + texture_idxc->OffsetXYZ + v73 * texture_idxc->VertexSize + 8) = v79;
            *(_DWORD *)((char *)texture_idxc->lpVertices + texture_idxc->OffsetDiffuse + v73 * texture_idxc->VertexSize) = -1;
            *(_DWORD *)((char *)texture_idxc->lpVertices + v73 * texture_idxc->VertexSize + texture_idxc->OffsetTexture1) = 0;
            *(_DWORD *)((char *)texture_idxc->lpVertices
                      + v73 * texture_idxc->VertexSize
                      + texture_idxc->OffsetTexture1
                      + 4) = 0;
            *(_DWORD *)((char *)texture_idxc->lpVertices + v73 * texture_idxc->VertexSize + texture_idxc->OffsetTexture2) = 0;
            *(_DWORD *)((char *)texture_idxc->lpVertices
                      + v73 * texture_idxc->VertexSize
                      + texture_idxc->OffsetTexture2
                      + 4) = 0;
          }
          v75 = v120;
          ++v73;
          v74 = xd;
        }
        ++v75;
        ++v74;
        v44 = v132-- == 1;
        v120 = v75;
        xd = v74;
        if ( !v44 )
          continue;
        break;
      }
      ++v122;
      v72 = numverticesa + 10;
      numverticesa = v72;
      if ( (int)v72 <= (int)&block_grid[91] )
        continue;
      break;
    }
    v54->UnlockVertexBuffer();
    if ( sparkle )
      texture_idxc->UnlockVertexBuffer();
    v83 = this->Lakes.array;
    xe = v83;
    v84 = *(int *)((char *)&v83->data.Blocks.maxsize + v134);
    v85 = *(int *)((char *)&v83->data.Blocks.size + v134);
    if ( v85 == v84 )
    {
      if ( v84 >= 16 )
        v86 = 6 * v84 / 5;
      else
        v86 = 16;
      flowc = v86;
      // x64: literal `16` is x86 sizeof(SLakeBlock); on x64 padding + 2 SMesh*
      // pointers push it to 32. Same family as group.cpp:1884.
      v87 = (SLakeBlock *)realloc(*(void **)((char *)&v83->data.Blocks.array + v134), sizeof(SLakeBlock) * v86);
      *(SLakeBlock **)((char *)&xe->data.Blocks.array + v134) = v87;
      memset(
        &v87[*(int *)((char *)&xe->data.Blocks.maxsize + v134)],
        0,
        sizeof(SLakeBlock) * (flowc - *(int *)((char *)&xe->data.Blocks.maxsize + v134)));
      v83 = xe;
      *(int *)((char *)&xe->data.Blocks.maxsize + v134) = flowc;
      v85 = *(int *)((char *)&xe->data.Blocks.size + v134);
    }
    v88 = v85 + 1;
    v89 = v85;
    *(int *)((char *)&v83->data.Blocks.size + v134) = v88;
    v90 = this;
    (*(SLakeBlock **)((char *)&this->Lakes.array->data.Blocks.array + v134))[v89].BlockIdx = v121;
    (*(SLakeBlock **)((char *)&this->Lakes.array->data.Blocks.array + v134))[v89].Mesh = v135;
    (*(SLakeBlock **)((char *)&this->Lakes.array->data.Blocks.array + v134))[v89].Mesh2 = texture_idxc;
LABEL_144:
    XSize = v90->XSize;
    v34 = v121 + 1;
    v121 = v34;
    if ( v34 < this->ZSize / 8 * (XSize / 8) )
      continue;
    break;
  }
LABEL_145:
  ::operator delete(block);
  return v109;
}

//----- (0042DA40) --------------------------------------------------------

SGroup *SGepard::CreateObject(int cacheidx, bool dynamic)

{
  SHeap<SObjectCacheProp>::__Tstruct *array; // eax
  SGroup *v6; // eax
  SGroup *v7; // eax
  SGroup *v9; // eax
  SGroup *v11; // esi
  SHeap<SObjectProp>::__Tstruct *v12; // eax
  int v13;
  int cacheidxa;
  int cacheidxb;
  int dynamica;
  if ( cacheidx < 0
    || cacheidx >= this->ObjectCache.size
    || (array = this->ObjectCache.array, array[cacheidx].use != 0x7FFFFFFF) )
  {
    Logger.g->Panic("SGepard::CreateObject: Invalid cache index");
  }
  ++array[cacheidx].data.RefCount;
  if ( dynamic )
  {
    cacheidxa = this->DynamicObjects.Add();
    v13 = cacheidxa;
    this->DynamicObjects.array[cacheidxa].data.CacheIdx = cacheidx;
    v6 = (SGroup *)operator new(sizeof(SGroup));
    if ( v6 )
      v7 = new (v6) SGroup(this->ObjectCache.array[cacheidx].data.Group, dynamic, cacheidxa);
    else
      v7 = 0;
    this->DynamicObjects.array[v13].data.Group = v7;
    return this->DynamicObjects.array[v13].data.Group;
  }
  else
  {
    dynamica = this->StaticObjects.Add();
    cacheidxb = dynamica;
    this->StaticObjects.array[dynamica].data.CacheIdx = cacheidx;
    v9 = (SGroup *)operator new(sizeof(SGroup));
    if ( v9 )
    {
      v11 = new (v9) SGroup(this->ObjectCache.array[cacheidx].data.Group, 0, dynamica);
    }
    else
    {
      v11 = 0;
    }
    this->StaticObjects.array[cacheidxb].data.Group = v11;
    this->StaticObjects.array[cacheidxb].data.LastRendered = -1;
    v12 = this->StaticObjects.array;
    this->ObjectsHashed = 0;
    return v12[cacheidxb].data.Group;
  }
}

//----- (0042DBD0) --------------------------------------------------------

SIObject *SGepard::CreateObject(const char *filename, float scale, BOOL dynamic)

{
  int v5;
  v5 = this->PrecacheObject(filename, scale, (SDrawType)0);
  if ( v5 >= 0 )
  {
    SIObject *obj = this->CreateObject(v5, dynamic);
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "CreateObject: '%s' → obj=%p vtbl=%p", filename, obj, obj ? *(void**)obj : 0);
    #endif
    return obj;
  }
  else
    return 0;
}

//----- (0042DC10) --------------------------------------------------------

SIPlane *SGepard::CreatePlane(int xsize, int zsize)

{
  STerrain *v4; // eax
  STerrain *v5; // eax
  STerrain *result; // eax
  this->XSize = xsize;
  this->ZSize = zsize;
  v4 = (STerrain *)operator new(sizeof(STerrain));
  if ( v4 )
    v5 = new (v4) STerrain(this, xsize, zsize);
  else
    v5 = 0;
  this->Terrain = v5;
  this->ObjectHash = (int *)operator new[](4 * xsize / 8 * (zsize / 8));
  result = this->Terrain;
  this->ObjectsHashed = 0;
  return result;
}

//----- (0042DCE0) --------------------------------------------------------

int SGepard::CreatePointLight(float colorr, float colorg, float colorb, float posx, float posy, float posz, float range, unsigned int attenuation)

{
  SLightProp *LightProps; // eax
  signed int v10;
  _D3DLIGHT9 *p_d3dlight; // esi
  LightProps = this->LightProps;
  v10 = 0;
  while ( LightProps->type )
  {
    ++v10;
    ++LightProps;
    if ( v10 >= 4 )
      Logger.g->Panic("SGepard::GetLightHandle: Fatal error: No more light avaiable! Exiting...");
  }
  p_d3dlight = &this->LightProps[v10].d3dlight;
  memset(&this->LightProps[v10].d3dlight.Diffuse, 0, 0x64u);
  p_d3dlight->Diffuse.r = colorr;
  p_d3dlight->Diffuse.g = colorg;
  p_d3dlight->Diffuse.b = colorb;
  p_d3dlight->Position.x = posx;
  p_d3dlight->Position.y = posy;
  p_d3dlight->Position.z = posz;
  p_d3dlight->Range = range;
  p_d3dlight->Type = D3DLIGHT_POINT;
  p_d3dlight->Attenuation0 = 0.0;
  memcpy(&p_d3dlight->Attenuation1 , &attenuation, 8);
  this->lpD3DDev->SetLight(v10, p_d3dlight);
  this->lpD3DDev->LightEnable(v10, 1);
  this->LightProps[v10].type = 2;
  return v10;
}

//----- (0042DDE0) --------------------------------------------------------

int SGepard::CreateRect(unsigned int color, int x0, int z0, int x1, int z1)

{
  int v7;
  int v8;
  int v9;
  int v10;
  int nextempty;
  SHeap<SRectProp>::__Tstruct *array; // ecx
  int v13;
  SHeap<SRectProp>::__Tstruct *v14; // eax
  int size;
  int maxsize;
  int v17;
  SHeap<SRectProp>::__Tstruct *v18; // eax
  int v19;
  SMesh *v21; // eax
  int v25;
  int x0a;
  int z0a;
  int z0b;
  int x1a;
  int z1a;
  v7 = x0;
  v8 = x1;
  if ( x0 <= x1 )
  {
    v7 = x1;
    v8 = x0;
  }
  v9 = z1;
  x1a = v8;
  v10 = z0;
  if ( z0 <= z1 )
    v10 = z1;
  v25 = v7;
  nextempty = this->Rects.nextempty;
  if ( z0 <= z1 )
    v9 = z0;
  ++this->Rects.occupied;
  x0a = v10;
  z1a = v9;
  z0a = nextempty;
  if ( nextempty < 0 )
  {
    size = this->Rects.size;
    maxsize = this->Rects.maxsize;
    if ( size == maxsize )
    {
      if ( maxsize >= 16 )
        v17 = 6 * maxsize / 5;
      else
        v17 = 16;
      z0b = v17;
      v18 = (SHeap<SRectProp>::__Tstruct *)realloc(this->Rects.array, sizeof(SHeap<SRectProp>::Element) * v17);
      v19 = this->Rects.maxsize;
      this->Rects.array = v18;
      memset(&v18[v19], 0, sizeof(SHeap<SRectProp>::Element) * (z0b - v19));
      size = this->Rects.size;
      this->Rects.maxsize = z0b;
    }
    this->Rects.array[size].use = 0x7FFFFFFF;
    nextempty = this->Rects.size;
    z0a = nextempty;
    this->Rects.size = nextempty + 1;
  }
  else
  {
    array = this->Rects.array;
    v13 = nextempty;
    this->Rects.nextempty = array[nextempty].use;
    array[v13].use = 0x7FFFFFFF;
    v14 = this->Rects.array;
    memset(&v14[v13].data.Mesh , 0, 16);
    memset(&v14[v13].data.X1 , 0, 8);
  }
  // x64: was `v20 = 8 * nextempty - z0a` (= 7*idx in DWORD-units, x86 sizeof
  // (Element) = 28). On x64 sizeof(Element) = 40 (4 use + 4 pad + 8 SMesh* +
  // 4*5 ints + 4 tail pad), so the stride drifted. The Mesh write was also
  // a bare `(_DWORD*)` 4-byte store into an 8-byte ptr field. Rewrote with
  // typed array indexing.
  {
    SRectProp &rect = this->Rects.array[nextempty].data;
    rect.Color = color;
    rect.X0 = x1a;
    rect.Z0 = z1a;
    rect.X1 = v25;
    rect.Z1 = x0a;
    v21 = (SMesh *)operator new(sizeof(SMesh));
    rect.Mesh = v21 ? new (v21) SMesh(this, 0) : nullptr;
    rect.Mesh->CreateVertexBuffer(2 * (x0a + v25 - z1a - x1a) + 1);
    this->RefreshRect(rect.Mesh, x1a, z1a, v25, x0a);
    rect.Mesh->UnlockVertexBuffer();
  }
  return z0a;
}

//----- (0042E030) --------------------------------------------------------

void SGepard::CreateScene()

{
  D3DXVECTOR3 vec2;
  D3DXVECTOR3 vec;
  vec.x = 1.0;
  vec.y = -1.0;
  vec.z = 1.0;
  D3DXVec3Normalize(&vec2, &vec);
  this->SunLight = this->CreateDirectionalLight(0.6f, 0.6f, 0.6f, vec2.x, vec2.y, vec2.z);
  this->LastSunDirection = -1000.0;
  this->LastSunElevation = -1000.0;
  this->DestroySplines();
}

//----- (0042E0E0) --------------------------------------------------------

SShader2Info *SGepard::CreateShader2(float _x, float _z, int _thandle, int _scale, SDrawType drawtype, bool _snaptocenter, bool _appendtolist, bool _forcebright)

{
  SHeap<STextureProp>::__Tstruct *array; // edx
  float v11; // xmm3_4
  float v12; // xmm2_4
  unsigned int v13;
  float v14; // xmm4_4
  int v15;
  float v16; // xmm0_4
  float v17; // xmm0_4
  SMesh *v18; // eax
  SMesh *v19; // eax
  SMesh *v20; // edi
  int v21;
  signed int v22;
  int v23;
  int v24;
  unsigned short v25; // ax
  bool v26;
  int v27;
  int v28;
  float v29; // xmm0_4
  int v30;
  int v31;
  int v32;
  D3DXVECTOR3 *v34; // edi
  int v35;
  _DWORD *v36; // eax
  SGepard *v37; // esi
  SShader2Info *v38; // edx
  SShader2Info *last; // eax
  SVertexLight *v40; // eax
  SShader2Info *v41; // edx
  int v42;
  int v43;
  int v44;
  STerrain *Terrain; // edx
  float v46; // xmm0_4
  float *HeightMap; // eax
  float v48; // xmm0_4
  float v49; // xmm0_4
  float v50; // xmm1_4
  float r; // xmm2_4
  float g; // xmm3_4
  float b; // xmm1_4
  bool v54; // cc
  signed int v56;
  float v57;
  int v58;
  float v59;
  int v60;
  int v61;
  int v63;
  int v64;
  SMesh *v65;
  int v66;
  int z;
  int za;
  int v69;
  int v70;
  int v71;
  int v72;
  int v73;
  int v74;
  int v75;
  int v76;
  int v77;
  signed int v78;
  int v79;
  int v80;
  SShader2Info *v81;
  D3DXVECTOR3 result;
  D3DXVECTOR3 nv;
  int v84;
  array = this->Textures.array;
  v11 = _x;
  v12 = _z;
  v13 = ((_scale * array[_thandle].data.Width) >> 6) + 2;
  v56 = (_scale * array[_thandle].data.Height) >> 6;
  v78 = v13;
  v69 = v56 + 2;
  v14 = (float)v56;
  if ( _snaptocenter )
  {
    v13 = ((_scale * array[_thandle].data.Width) >> 6) + 2;
    v11 = _x - (float)((float)((_scale * array[_thandle].data.Width) >> 6) * 0.5);
    v12 = _z - (float)(v14 * 0.5);
  }
  v15 = v13 - 2;
  v58 = v13 - 2;
  if ( (float)((float)(int)(v13 - 2) + v11) > (float)this->XSize
    || (float)(v14 + v12) > (float)this->ZSize
    || v11 < 0.0
    || v12 < 0.0 )
  {
    return 0;
  }
  v16 = (float)(floor(v11));

  v61 = (int)v16;
  v17 = (float)(floor(v12));

  v60 = (int)v17;
  v57 = v12 - (float)(int)v17;
  v18 = (SMesh *)operator new(sizeof(SMesh));
  v84 = 0;
  if ( v18 )
  {
    v19 = new (v18) SMesh(this, 0x51u);
    v20 = v19;
    v65 = v19;
  }
  else
  {
    v20 = 0;
    v65 = 0;
  }
  v84 = -1;
  v20->CreateVertexBuffer(v78 * v69);
  v20->CreateIndexBuffer(0, (v78 - 1) * (6 * v69 - 6));
  v21 = 0;
  v79 = 0;
  z = v56 + 1;
  v22 = v78;
  if ( v56 + 1 > 0 )
  {
    v23 = 0;
    v24 = v56 + 1;
    v70 = 0;
    do
    {
      if ( v78 - 1 > 0 )
      {
        v25 = v23;
        v63 = v23;
        v73 = v78 - 1;
        do
        {
          v65->lpIndices[0][v21] = v25 + v78;
          v65->lpIndices[0][v21 + 1] = v25;
          v65->lpIndices[0][v79 + 2] = v25 + 1;
          v65->lpIndices[0][v79 + 3] = v25 + v78;
          v65->lpIndices[0][v79 + 4] = v25 + 1;
          v65->lpIndices[0][v79 + 5] = v78 + 1 + v25;
          v21 = v79 + 6;
          v25 = v63 + 1;
          v79 += 6;
          v26 = v73-- == 1;
          ++v63;
        }
        while ( !v26 );
        v22 = v78;
        v23 = v70;
        v24 = z;
        v79 = v21;
      }
      v23 += v22;
      --v24;
      v70 = v23;
      z = v24;
    }
    while ( v24 );
    v15 = v58;
    v20 = v65;
  }
  v27 = 0;
  v28 = 0;
  v80 = 0;
  v74 = 0;
  if ( v56 != -2 )
  {
    do
    {
      v76 = 0;
      if ( v22 )
      {
        v29 = (float)v27;
        v30 = v27 + v60;
        v31 = v61 - v28;
        za = v30;
        v71 = v61 - v28;
        do
        {
          v32 = v31 + v28;
          *(float *)((char *)v20->lpVertices + v20->OffsetXYZ + v80 * v20->VertexSize) = (float)v32;
          // x64: was `v33 = (int)v65->lpVertices + ...` truncating an 8-byte
          // ptr to int, then `(float*)(v33 + 4)` reconstructing — sign-extend
          // from low 32 bits zeros the high half. Use char* arithmetic.
          { char *v33 = (char *)v65->lpVertices + v65->OffsetXYZ + v80 * v20->VertexSize;
          *(float *)(v33 + 4) = (float)(this->Terrain->GetHeight(v32, za)); }

          *(float *)((char *)v65->lpVertices + v65->OffsetXYZ + v80 * v65->VertexSize + 8) = (float)za;
          v34 = (D3DXVECTOR3 *)((char *)v65->lpVertices + v80 * v65->VertexSize + v65->OffsetNormal);
          *v34 = *this->Terrain->GetNormal(&result, v80 + v71, za);
          v20 = v65;
          *(float *)((char *)v65->lpVertices + v80 * v65->VertexSize + v65->OffsetTexture1) = (float)((float)v76 - (float)(v11 - (float)v61))
                                                                                            / (float)v15;
          v35 = v80 * v65->VertexSize;
          v28 = ++v80;
          v59 = 1.0f - (float)((float)(v29 - v57) / (float)v56);
          *(float *)((char *)v65->lpVertices + v35 + v65->OffsetTexture1 + 4) = v59;
          v22 = v78;
          ++v76;
          v31 = v71;
        }
        while ( v76 < v78 );
        v27 = v74;
      }
      v74 = ++v27;
    }
    while ( v27 < v69 );
  }
  v20->UnlockVertexBuffer();
  v20->UnlockIndexBuffer(0);
  // x64 fix: original `malloc(0x3Cu)` allocated 60 bytes — the x86
  // sizeof(SShader2Info). On x64 the 4 embedded pointers (next, prev, mesh,
  // vertexlight) grow from 4 to 8 bytes each, so the struct is ~80 bytes.
  // Allocating only 60 bytes meant later field writes ran past the heap
  // block; the linked-list nodes were also smaller than the struct so
  // RenderDecals walked corrupted next pointers and skipped most decals.
  v36 = (_DWORD *)malloc(sizeof(SShader2Info));
  if ( _appendtolist )
  {
    v81 = (SShader2Info *)v36;
    memset(v36, 0, sizeof(SShader2Info));
    v37 = this;
    v38 = v81;
    last = this->Shader2Infos.last;
    if ( last )
    {
      last->next = v81;
      v81->prev = this->Shader2Infos.last;
      this->Shader2Infos.last = v81;
      v81->next = 0;
      ++this->Shader2Infos.NumItems;
    }
    else
    {
      ++this->Shader2Infos.NumItems;
      this->Shader2Infos.last = v81;
      this->Shader2Infos.first = v81;
    }
  }
  else
  {
    v37 = this;
    v38 = (SShader2Info *)v36;
    v81 = (SShader2Info *)v36;
    // x64 fix: original cleared next, prev, vertexlight via byte-offset writes
    // (offset 0, 4, 53, 57, 59) that assumed x86 SShader2Info layout. Use
    // typed assignments — also covers all bits of the 8-byte pointers on x64.
    v81->next = nullptr;
    v81->prev = nullptr;
    v81->vertexlight = nullptr;
  }
  v38->XVertices = v78;
  v38->xpos = v11;
  v38->ShaderTHandle = _thandle;
  v38->ForceBright = _forcebright;
  v38->mesh = v20;
  v38->DrawType = drawtype;
  v38->zpos = v12;
  v38->ZVertices = v69;
  v38->Erosseg = 1.0;
  v38->R = 1.0;
  v38->G = 1.0;
  v38->B = 1.0;
  v40 = (SVertexLight *)operator new[](12 * v78 * v69);
  v41 = v81;
  v66 = 0;
  v77 = 0;
  v81->vertexlight = v40;
  if ( v56 != -2 )
  {
    v42 = v60;
    do
    {
      v75 = 0;
      v43 = v61 + v42 * v37->Terrain->Stride;
      if ( v78 )
      {
        v72 = v66;
        v44 = v61;
        v64 = v61;
        do
        {
          Terrain = v37->Terrain;
          nv.y = 1.0;
          if ( v44 )
          {
            v26 = v44 == v37->XSize;
            HeightMap = Terrain->HeightMap;
            v48 = HeightMap[v43 - 1];
            if ( v26 )
              v46 = v48 - HeightMap[v43];
            else
              v46 = (float)(v48 - HeightMap[v43 + 1]) * 0.5f;
          }
          else
          {
            v46 = Terrain->HeightMap[v43] - Terrain->HeightMap[v43 + 1];
          }
          nv.x = v46;
          if ( v42 )
          {
            if ( v42 == v37->ZSize )
              v49 = Terrain->HeightMap[v43 - Terrain->Stride] - Terrain->HeightMap[v43];
            else
              v49 = (float)(Terrain->HeightMap[v43 - Terrain->Stride] - Terrain->HeightMap[v43 + Terrain->Stride]) * 0.5f;
          }
          else
          {
            v49 = Terrain->HeightMap[v43] - Terrain->HeightMap[v43 + Terrain->Stride];
          }
          nv.z = v49;
          D3DXVec3Normalize(&nv, &nv);
          v50 = -((float)((float)(v37->SunDir.y * nv.y) + (float)(nv.x * v37->SunDir.x))
                         + (float)(v37->SunDir.z * nv.z));
          if ( v50 <= 0.0 )
          {
            r = v37->AmbientColorVal.r;
            g = v37->AmbientColorVal.g;
            b = v37->AmbientColorVal.b;
          }
          else
          {
            r = fminf(1.0, (float)(v37->SunColorVal.r * v50) + v37->AmbientColorVal.r);
            g = fminf(1.0, (float)(v37->SunColorVal.g * v50) + v37->AmbientColorVal.g);
            b = fminf(1.0, (float)(v37->SunColorVal.b * v50) + v37->AmbientColorVal.b);
          }
          if ( v43 < 0 || v43 >= (v37->ZSize + 1) * (v37->XSize + 1) )
            Logger.g->Panic("SPlane::UpdateColorMap: Invalid address");
          v41 = v81;
          ++v43;
          ++v66;
          v81->vertexlight[v72].r = r;
          v81->vertexlight[v72].g = g;
          v81->vertexlight[v72++].b = b;
          v44 = ++v64;
          v54 = ++v75 < v78;
          v42 = v60;
        }
        while ( v54 );
      }
      v60 = ++v42;
      ++v77;
    }
    while ( v77 < v69 );
  }
  return v41;
}

//----- (0042E860) --------------------------------------------------------

SShaderInfo *SGepard::CreateShader(float _x, float _z, int thandle, float texscale, int _rotation, bool _appendtolist, bool _snaptocenter)

{
  SMesh *v9; // eax
  SMesh *v10; // eax
  SMesh *v11; // ebx
  SHeap<STextureProp>::__Tstruct *array; // ecx
  float Width; // xmm0_4
  int v14; // kr04_4
  float v15; // xmm0_4
  int v16;
  float v17; // xmm1_4
  SShaderInfo *first; // esi
  SShaderInfo *v19; // eax
  float v20; // xmm0_4
  float v21; // xmm1_4
  float v22; // xmm0_4
  float v23; // xmm1_4
  SShaderInfo *current; // eax
  int v25;
  int v26;
  float v27; // xmm0_4
  float v28; // xmm0_4
  int v29;
  int v30;
  int v31;
  int v32;
  bool v33;
  int v34;
  int v35;
  signed int v36;
  float v37; // xmm0_4
  int v38;
  int v39;
  D3DXVECTOR3 *v41; // edi
  SShaderInfo *v42; // eax
  __m128i v43; // xmm1
  __m128i v44; // xmm0
  __m128i v45; // xmm1
  SShaderInfo *v46; // esi
  SShaderInfo *last; // eax
  int v48;
  double X;
  double v50;
  int v51;
  int v52;
  float v53;
  double v55;
  double v56;
  int v57;
  signed int v58;
  double v59;
  int v60;
  signed int v61;
  int v62;
  unsigned int v63;
  signed int v64;
  int v65;
  signed int v66;
  int v67;
  signed int v68;
  int v69;
  D3DXVECTOR3 result;
  int v71;
  v9 = (SMesh *)operator new(sizeof(SMesh));
  v71 = 0;
  if ( v9 )
  {
    v10 = new (v9) SMesh(this, 0x51u);
    v11 = v10;
  }
  else
  {
    v11 = 0;
  }
  array = this->Textures.array;
  v48 = thandle;
  v71 = -1;
  Width = (float)array[thandle].data.Width;
  v14 = (int)(float)(Width * texscale);
  v67 = v14 / 64 + 1;
  v15 = (float)array[thandle].data.Height * texscale;
  v16 = (int)v15 / 64 + 1;
  v57 = v16;
  if ( _rotation == 1 || _rotation == 3 )
  {
    v67 = (int)v15 / 64 + 1;
    v16 = v14 / 64 + 1;
    v57 = v16;
  }
  v17 = _z;
  if ( _snaptocenter )
    v17 = _z - (float)((float)v16 - 2.0);
  v53 = v17;
  if ( _appendtolist )
  {
    first = this->ShaderInfos.first;
    this->ShaderInfos.current = first;
    if ( first )
    {
      X = v17;
      v55 = floor(_x);
      v19 = first;
      while ( 1 )
      {
        v20 = (float)(floor(v19->xpos));

        v21 = (float)(v55);

        if ( v20 == v21 )
        {
          v50 = floor(this->ShaderInfos.current->zpos);
          v22 = (float)(floor(X));

          v23 = (float)(v50);

          if ( v23 == v22 )
          {
            current = this->ShaderInfos.current;
            if ( current->Rotation == _rotation )
            {
              v25 = strcmp(this->Textures.array[v48].data.FileName, current->filename);
              if ( v25 )
                v25 = v25 < 0 ? -1 : 1;
              if ( !v25 )
                break;
            }
          }
        }
        if ( this->ShaderInfos.Closed )
        {
          if ( !first )
            Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
          first = first->next;
          if ( !first )
            first = this->ShaderInfos.first;
          this->ShaderInfos.current = first;
        }
        else if ( first )
        {
          first = first->next;
          this->ShaderInfos.current = first;
        }
        else
        {
          this->ShaderInfos.current = 0;
          first = 0;
        }
        v19 = this->ShaderInfos.current;
        if ( !v19 )
          goto LABEL_27;
      }
      this->ReleaseTexture(thandle, 0);
      return 0;
    }
  }
LABEL_27:
  v26 = v67;
  v64 = v67 - 1;
  v59 = floor(_x);
  v27 = (float)(v59);

  if ( floor((double)(v67 - 1)) + v27 > (double)this->XSize )
    return 0;
  v68 = v57 - 1;
  v56 = floor(v53);
  v28 = (float)(v56);

  if ( floor((double)(v57 - 1)) + v28 > (double)this->ZSize || _x < 0.0 || v53 < 0.0 )
    return 0;
  v11->CreateVertexBuffer(v67 * v57);
  v11->CreateIndexBuffer(0, (v67 - 1) * (6 * v57 - 6));
  v29 = v57 - 1;
  v30 = 0;
  if ( v68 > 0 )
  {
    v31 = 0;
    v65 = v57 - 1;
    v62 = 0;
    v32 = v67 - 1;
    do
    {
      if ( v32 > 0 )
      {
        v51 = v31;
        v60 = v32;
        do
        {
          v11->lpIndices[0][v30] = v31 + v67;
          v11->lpIndices[0][v30 + 1] = v31;
          v11->lpIndices[0][v30 + 2] = v31 + 1;
          v11->lpIndices[0][v30 + 3] = v31 + v67;
          v11->lpIndices[0][v30 + 4] = v31 + 1;
          v11->lpIndices[0][v30 + 5] = v31 + v67 + 1;
          v30 += 6;
          *(unsigned short*)&v31 = (unsigned short)(v51 + 1);
          v33 = v60-- == 1;
          ++v51;
        }
        while ( !v33 );
        v26 = v67;
        v32 = v67 - 1;
        v31 = v62;
        v29 = v65;
      }
      v31 += v26;
      --v29;
      v62 = v31;
      v65 = v29;
    }
    while ( v29 );
    v29 = v57 - 1;
  }
  v34 = v57;
  v35 = 0;
  v36 = 0;
  v69 = 0;
  v61 = 0;
  if ( v57 > 0 )
  {
    v66 = v29;
    do
    {
      v58 = 0;
      if ( v26 > 0 )
      {
        v37 = (float)(v56);

        v38 = v36 + (int)v37;
        v63 = v67 - 1;
        v39 = (int)(float)v59;
        v52 = v39;
        do
        {
          *(float *)((char *)v11->lpVertices + v11->OffsetXYZ + v35 * v11->VertexSize) = (float)v39;
          // x64: same lpVertices-as-int truncation as the sibling at ~4303.
          { char *v40 = (char *)v11->lpVertices + v11->OffsetXYZ + v69 * v11->VertexSize;
          *(float *)(v40 + 4) = (float)(this->Terrain->GetHeight((int)(float)v39, (int)(float)v38)); }

          *(float *)((char *)v11->lpVertices + v11->OffsetXYZ + v69 * v11->VertexSize + 8) = (float)v38;
          *(_DWORD *)((char *)v11->lpVertices + v11->OffsetDiffuse + v69 * v11->VertexSize) = -1;
          v41 = (D3DXVECTOR3 *)((char *)v11->lpVertices + v11->OffsetNormal + v69 * v11->VertexSize);
          *v41 = *this->Terrain->GetEdgeNormal(&result, (float)v52, (float)v38);
          switch ( _rotation )
          {
            case 0:
              *(float *)((char *)v11->lpVertices + v69 * v11->VertexSize + v11->OffsetTexture1) = (float)v58
                                                                                                / (float)v64;
              v43 = _mm_cvtsi32_si128(v61);
              goto LABEL_52;
            case 1:
              v44 = _mm_cvtsi32_si128(v64);
              *(float *)((char *)v11->lpVertices + v69 * v11->VertexSize + v11->OffsetTexture1) = (float)v61
                                                                                                / (float)v68;
              v43 = _mm_cvtsi32_si128(v63);
              goto LABEL_53;
            case 2:
              v45 = _mm_cvtsi32_si128(v63);
              goto LABEL_51;
            case 3:
              v44 = _mm_cvtsi32_si128(v64);
              *(float *)((char *)v11->lpVertices + v69 * v11->VertexSize + v11->OffsetTexture1) = (float)v66
                                                                                                / (float)v68;
              v43 = _mm_cvtsi32_si128(v58);
              goto LABEL_53;
            case 4:
              v45 = _mm_cvtsi32_si128(v58);
LABEL_51:
              *(float *)((char *)v11->lpVertices + v69 * v11->VertexSize + v11->OffsetTexture1) = _mm_cvtepi32_ps(v45).m128_f32[0]
                                                                                                / (float)v64;
              v43 = _mm_cvtsi32_si128(v66);
LABEL_52:
              v44 = _mm_cvtsi32_si128(v68);
LABEL_53:
              *(float *)((char *)v11->lpVertices + v69 * v11->VertexSize + v11->OffsetTexture1 + 4) = _mm_cvtepi32_ps(v43).m128_f32[0] / _mm_cvtepi32_ps(v44).m128_f32[0];
              break;
            default:
              break;
          }
          v35 = v69 + 1;
          --v63;
          v39 = v52 + 1;
          ++v69;
          ++v58;
          ++v52;
        }
        while ( v58 < v67 );
        v26 = v67;
        v36 = v61;
        v29 = v66;
        v34 = v57;
      }
      ++v36;
      --v29;
      v61 = v36;
      v66 = v29;
    }
    while ( v36 < v34 );
  }
  v11->UnlockVertexBuffer();
  v11->UnlockIndexBuffer(0);
  v46 = (SShaderInfo *)operator new(sizeof(SShaderInfo));
  memset(v46, 0, sizeof(SShaderInfo));
  if ( _appendtolist )
  {
    last = this->ShaderInfos.last;
    if ( last )
    {
      last->next = v46;
      v46->prev = this->ShaderInfos.last;
      this->ShaderInfos.last = v46;
      v46->next = 0;
    }
    else
    {
      this->ShaderInfos.last = v46;
      this->ShaderInfos.first = v46;
    }
    ++this->ShaderInfos.NumItems;
  }
  strcpy(v46->filename, this->Textures.array[v48].data.FileName);
  v46->XVertices = v67;
  v46->xpos = _x;
  v46->Rotation = _rotation;
  v42 = v46;
  v46->zpos = v53;
  v46->mesh = v11;
  v46->ZVertices = v34;
  v46->ShaderTHandle = thandle;
  return v42;
}

//----- (0042F020) --------------------------------------------------------

SShaderInfo *SGepard::CreateShader(float _x, float _z, const char *_filename, int _rotation, bool _appendtolist, bool _snaptocenter)

{
  unsigned int v15;
  char *NextProperty; // edi
  int v17;
  SProperties *DecalsIni; // ecx
  int v19;
  SFolders *first; // eax
  char *v21; // ebx
  char *filename; // esi
  SFolders *current; // eax
  SFolders *next; // eax
  int v34;
  char v35; // al
  SGepard *v38;
  float texscale;
  char newfilenamewithoutextension[260];
  char pathandfilename[260];
  char newfilename[260];
  v38 = this;
  strcpy(newfilename, _filename);
  if ( !newfilename[0] )
    return 0;
  texscale = 1.0f;
  strncpy(newfilenamewithoutextension, newfilename, strlen(newfilename) - 4);
  v15 = &newfilename[strlen(newfilename) + 1] - &newfilename[1] - 4;
  if ( v15 >= 0x78 )
    ((void)0);
  newfilenamewithoutextension[v15] = '\0';
  this->DecalsIni->EnumProperties("Decal scale overrides");
  NextProperty = this->DecalsIni->GetNextProperty();
  if ( NextProperty )
  {
    while ( 1 )
    {
      v17 = _stricmp(newfilenamewithoutextension, NextProperty);
      DecalsIni = this->DecalsIni;
      if ( !v17 )
        break;
      NextProperty = DecalsIni->GetNextProperty();
      if ( !NextProperty )
        goto LABEL_11;
    }
    texscale = DecalsIni->GetFloat("Decal scale overrides", NextProperty, 1.0);
  }
LABEL_11:
  v19 = this->LoadTexture(newfilename, 1, 1);
  if ( v19 != -1 )
    goto LABEL_34;
  Logger.g->Log(2, "   Failded to load %s  Cycling thought the directories...", newfilename);
  first = this->ShaderFolders.first;
  this->ShaderFolders.current = first;
  if ( first )
  {
    while ( v19 == -1 )
    {
      if ( strrchr(newfilename, 92) )
        v21 = strrchr(newfilename, 92) + 1;
      else
        v21 = newfilename;
      filename = this->ShaderFolders.current->filename;
      strcpy(pathandfilename, "normal\\shadertextures\\");
      strcat(pathandfilename, filename);
      strcat(pathandfilename, "\\");
      strcat(pathandfilename, v21);
      Logger.g->Log(2, "   Trying to load: %s", pathandfilename);
      v19 = v38->LoadTexture(pathandfilename, 1, 1);
      current = v38->ShaderFolders.current;
      if ( v38->ShaderFolders.Closed )
      {
        if ( !current )
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        next = current->next;
        if ( !next )
          next = v38->ShaderFolders.first;
      }
      else if ( current )
      {
        next = current->next;
      }
      else
      {
        next = 0;
      }
      v38->ShaderFolders.current = next;
      if ( !next )
      {
        if ( v19 == -1 )
          goto LABEL_35;
        break;
      }
    }
    v34 = 0;
    do
    {
      v35 = pathandfilename[v34];
      newfilename[v34++] = v35;
    }
    while ( v35 );
LABEL_34:
    Logger.g->Log(2, "Texture loaded: %s", newfilename);
    return this->CreateShader(_x, _z, v19, texscale, _rotation, _appendtolist, _snaptocenter);
  }
LABEL_35:
  Logger.g->Warning("SGepard::CreateShader: texture cannot be loaded! (%s)", newfilename);
  return 0;
}

//----- (0042F3F0) --------------------------------------------------------

int SGepard::CreateShadowTexture(unsigned int width, unsigned int height)

{
  int v5;
  SHeap<STextureProp>::__Tstruct *array; // esi
  int v7;
  HRESULT v8;
  const char *v9; // eax
  int v10;
  int v11;
  char atmstr[200];
  if ( this->TFShadow == D3DFMT_UNKNOWN )
    return -1;
  v5 = this->Textures.Add();
  array = this->Textures.array;
  v7 = v5;
  v10 = v5;
  v11 = v5;
  array[v7].data.FileName = _strdup("**unique**");
  this->Textures.array[v7].data.RefCount = 1;
  this->Textures.array[v11].data.Width = width;
  this->Textures.array[v11].data.Height = height;
  if ( ((width - 1) & width) != 0 )
 Logger.g->Panic(
      "SGepard::CreateShadowTexture: (%d x %d): Width must be power of 2",
      width,
      height);
  if ( ((height - 1) & height) != 0 )
 Logger.g->Panic(
      "SGepard::CreateShadowTexture: (%d x %d): Height must be power of 2",
      width,
      height);
  v8 = this->lpD3DDev->CreateTexture(width,
         height,
         1u,
         1u,
         this->TFShadow,
         D3DPOOL_DEFAULT,
         &this->Textures.array[v11].data.lpTexture,
         0);
  if ( v8 )
  {
    v9 = DXGetErrorStringA(v8);
    sprintf(atmstr, "%s: %s", "SGepard::CreateShadowTexture: CreateTexture", v9);
    Logger.g->Panic(atmstr);
  }
  return v10;
}

//----- (0042F540) --------------------------------------------------------

int SGepard::CreateSmokeTrail(int texture_idx, float x, float y, float z, int color, float strength, float fade_speed, float u_scale, float v_scale, SDrawType drawtype)

{
  int nextempty;
  SHeap<SSmokeTrail>::__Tstruct *array; // ecx
  int v14;
  SHeap<SSmokeTrail>::__Tstruct *v15; // eax
  int size;
  int maxsize;
  int v18;
  SHeap<SSmokeTrail>::__Tstruct *v19; // eax
  int v20;
  SHeap<STextureProp>::__Tstruct *v22; // edx
  int v23;
  SDArray<SSmokeTrailPoint> *v24; // eax
  SDArray<SSmokeTrailPoint> *v25; // ecx
  SHeap<SSmokeTrail>::__Tstruct *v26; // esi
  int v27;
  SSmokeTrailPoint *v28; // ecx
  SHeap<SSmokeTrail>::__Tstruct *v29; // esi
  int v30;
  int v31;
  int v32;
  int result;
  int v34;
  int texture_idxa;
  ++this->SmokeTrails.occupied;
  nextempty = this->SmokeTrails.nextempty;
  if ( nextempty < 0 )
  {
    size = this->SmokeTrails.size;
    maxsize = this->SmokeTrails.maxsize;
    if ( size == maxsize )
    {
      if ( maxsize >= 16 )
        v18 = 6 * maxsize / 5;
      else
        v18 = 16;
      // x64: literal `40` is x86 sizeof(SHeap<SSmokeTrail>::__Tstruct). Same family as group.cpp:1884.
      v19 = (SHeap<SSmokeTrail>::__Tstruct *)realloc(this->SmokeTrails.array, sizeof(SHeap<SSmokeTrail>::__Tstruct) * v18);
      v20 = this->SmokeTrails.maxsize;
      this->SmokeTrails.array = v19;
      memset(&v19[v20], 0, sizeof(SHeap<SSmokeTrail>::__Tstruct) * (v18 - v20));
      size = this->SmokeTrails.size;
      this->SmokeTrails.maxsize = v18;
    }
    this->SmokeTrails.array[size].use = 0x7FFFFFFF;
    nextempty = this->SmokeTrails.size;
    this->SmokeTrails.size = nextempty + 1;
  }
  else
  {
    array = this->SmokeTrails.array;
    v14 = nextempty;
    this->SmokeTrails.nextempty = array[nextempty].use;
    array[v14].use = 0x7FFFFFFF;
    v15 = this->SmokeTrails.array;
    memset(&v15[v14].data.Points , 0, 16);
    memset(&v15[v14].data.Strength , 0, 16);
    v15[v14].data.DrawType = DT_NORMAL;
  }
  if ( texture_idx >= 0 && texture_idx < this->Textures.size )
  {
    v22 = this->Textures.array;
    if ( v22[texture_idx].use == 0x7FFFFFFF )
      ++v22[texture_idx].data.RefCount;
  }
  v23 = nextempty;
  // x64: literal `40` is x86 sizeof(SHeap<SSmokeTrail>::__Tstruct). On x64 the
  // leading SDArray<SSmokeTrailPoint>* grew 4→8 plus padding, so Element is 56.
  // texture_idxa is consumed below as a byte offset into SmokeTrails.array.
  texture_idxa = v23 * (int)sizeof(SHeap<SSmokeTrail>::__Tstruct);
  this->SmokeTrails.array[v23].data.TextureIndex = texture_idx;
  // x64 fix: literal 0xC (= 12) was x86 sizeof(SDArray); x64 grows to 16.
  v24 = (SDArray<SSmokeTrailPoint> *)operator new(sizeof(SDArray<SSmokeTrailPoint>));
  v25 = v24;
  if ( v24 )
  {
    v24->array = 0;
    v24->size = 0;
    v24->maxsize = 0;
  }
  else
  {
    v25 = 0;
  }
  this->SmokeTrails.array[v23].data.Points = v25;
  this->SmokeTrails.array[v23].data.AutoDestruct = 0;
  this->SmokeTrails.array[v23].data.DrawType = drawtype;
  this->SmokeTrails.array[v23].data.Color = color;
  this->SmokeTrails.array[v23].data.Strength = strength * 256.0f;
  this->SmokeTrails.array[v23].data.FadeSpeed = fade_speed;
  this->SmokeTrails.array[v23].data.UScale = u_scale;
  this->SmokeTrails.array[v23].data.VScale = v_scale;
  v26 = this->SmokeTrails.array;
  v27 = (*(SDArray<SSmokeTrailPoint> **)((char *)&v26->data.Points + texture_idxa))->Add();
  v28 = (*(SDArray<SSmokeTrailPoint> **)((char *)&v26->data.Points + texture_idxa))->array;
  v28[v27].Vector.x = x;
  v28[v27].Vector.y = y;
  v28[v27].Vector.z = z;
  v28[v27].Scale = 0.5;
  v28[v27].TextureV = 0.0;
  v28[v27].StartTime = this->WorldTime;
  v28[v27].Alpha = 1.0;
  // x64 fix: original second-point write used `v31 = *(int*)&data.Points`
  // (truncating the 8-byte Points pointer to 4 bytes on x64), then
  // `v34 = *(_DWORD*)(v31 + 8)` re-truncated the inner array pointer,
  // and finally `*(float*)(v34 + 4*v32 + N)` wrote SSmokeTrailPoint
  // fields to whatever address that truncated chain produced. The
  // result was a smoke-trail quad with one valid corner (first point,
  // typed) and one corner pointing into garbage — the muzzle-flash
  // billboard rendered as a white rectangle without texture/UV. Mirror
  // the typed first-point write above.
  SDArray<SSmokeTrailPoint> *pts = this->SmokeTrails.array[v23].data.Points;
  int idx2 = pts->Add();
  SSmokeTrailPoint *p2 = &pts->array[idx2];
  p2->Vector.x = x;
  p2->Vector.y = y;
  p2->Vector.z = z;
  p2->Scale = 0.5f;
  p2->TextureV = 0.0f;
  p2->Alpha = 1.0f;
  p2->StartTime = this->WorldTime;
  return nextempty;
}

//----- (0042F7F0) --------------------------------------------------------

void SGepard::CreateSpline(SHillRing *_hillring, SSplineRing **_splinering, int _altitude2, int _checkboxnr)

{
  bool v7; // bl
  SChain<SSplineRing> *SplineChain; // eax
  bool Closed; // dl
  SSplineRing *first; // ecx
  SSplineRing *last; // eax
  SSpline *v12; // eax
  SSplineRing *v14; // esi
  SChain<SSplineRing> *v15; // ecx
  SSplineRing *v16; // eax
  SSplineRing *current; // edx
  SChain<SSplineRing> *v18; // eax
  SChain<SSplineRing> *v19; // ecx
  SSplineRing *v20; // eax
  bool minharmadik;
  SHillRing *_hillringa;
  v7 = 0;
  minharmadik = 0;
  SplineChain = _hillring->SplineChain;
  Closed = SplineChain->Closed;
  if ( Closed && !SplineChain->first )
    Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
  first = SplineChain->first;
  if ( Closed && !SplineChain->last )
    Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
  last = SplineChain->last;
  if ( first && last )
  {
    v7 = first != last;
    minharmadik = first != last;
  }
  v12 = (SSpline *)operator new(sizeof(SSpline));
  if ( v12 )
  {
    _hillringa = (SHillRing *)new (v12) SSpline(this, minharmadik, _checkboxnr);
  }
  else
  {
    _hillringa = 0;
  }
  v14 = (SSplineRing *)operator new(sizeof(SSplineRing));
  v14->spline = (SSpline *)_hillringa;
  if ( !v7 )
  {
    v19 = _hillring->SplineChain;
    v20 = v19->last;
    if ( v20 )
    {
      v20->next = v14;
      v14->prev = v19->last;
      v19->last = v14;
      v14->next = 0;
    }
    else
    {
      v19->first = v14;
      v19->last = v14;
      v14->next = 0;
      v19->first->prev = 0;
    }
    ++v19->NumItems;
    goto LABEL_25;
  }
  _hillring->SplineChain->current = _hillring->SplineChain->first;
  v15 = _hillring->SplineChain;
  v16 = v15->last;
  if ( v16 )
  {
    current = v15->current;
    if ( current != v16 )
    {
      v14->prev = current;
      v14->next = v15->current->next;
      v15->current->next = v14;
      v14->next->prev = v14;
      goto LABEL_18;
    }
    v16->next = v14;
    v14->prev = v15->last;
    v15->last = v14;
  }
  else
  {
    v15->last = v14;
    v15->first = v14;
    v14->prev = 0;
  }
  v14->next = 0;
LABEL_18:
  ++v15->NumItems;
  _hillringa[2].Type = _altitude2;
  v18 = _hillring->SplineChain;
  if ( v18->Closed && !v18->first )
    Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
  *(unsigned char*)&(_hillringa[2].next) = (unsigned char)(v18->first->spline->Closed);
  this->RecalculateCentralNodes(_hillring);
LABEL_25:
  *_splinering = v14;
}

//----- (0042F9D0) --------------------------------------------------------

int SGepard::CreateTextureFromBitmap(const char *filename, SBitmap *bmap, bool mipmaps)

{
  SHeap<STextureProp> *p_Textures; // ebx
  int v5;
  SHeap<STextureProp>::__Tstruct *array; // esi
  int v7;
  D3DFORMAT Format;
  D3DFORMAT v10;
  char *v11; // eax
  bool v12;
  SHeap<STextureProp>::__Tstruct *v13; // eax
  SBitmap *v16; // esi
  HRESULT v17;
  int v20;
  const char *v22; // eax
  STextureBitmap levelbitmap;
  SBitmap bmap32;
  int v27;
  SBitmap *source;
  SGepard *v29;
  const char *String;
  char atmstr[200];
  int i;
  v29 = this;
  p_Textures = &this->Textures;
  String = filename;
  source = bmap;
  v5 = this->Textures.Add();
  array = p_Textures->array;
  v27 = v5;
  v7 = v5;
  array[v7].data.FileName = _strdup(filename);
  p_Textures->array[v7].data.RefCount = 1;
  p_Textures->array[v7].data.Width = bmap->Width;
  p_Textures->array[v7].data.Height = bmap->Height;
  p_Textures->array[v7].data.MipMaps = mipmaps;
  if ( mipmaps || !v29->TFSupportNonPow2 )
  {
    if ( ((bmap->Width - 1) & bmap->Width) != 0 )
      Logger.g->Panic("SGepard::CreateTextureFromBitmap: %s: Width must be power of 2", String);
    if ( ((bmap->Height - 1) & bmap->Height) != 0 )
      Logger.g->Panic("SGepard::CreateTextureFromBitmap: %s: Height must be power of 2", String);
  }
  Format = bmap->Format;
  if ( Format == D3DFMT_A8R8G8B8 )
  {
    if ( strstr(String, "_hq.tga") || strstr(String, ".png") )
    {
      p_Textures->array[v7].data.Alpha = 2;
      v10 = v29->TFHiAlpha;
    }
    else if ( strstr(String, "_a.tga") )
    {
      p_Textures->array[v7].data.Alpha = 2;
      v10 = v29->TFAlpha;
    }
    else
    {
      v11 = (char *)strstr(String, "skinned");
      v12 = v11 == 0;
      v13 = p_Textures->array;
      if ( v12 )
      {
        v13[v7].data.Alpha = 1;
        v10 = v29->TF1Bit;
      }
      else
      {
        v13[v7].data.Alpha = 3;
        v10 = v29->TFAlpha;
      }
    }
  }
  else
  {
    if ( Format != D3DFMT_R8G8B8 )
      Logger.g->Panic("SGepard::CreateTextureFromBitmap: %s: Bitmap format unsupported", String);
    if ( strstr(String, "_hq.tga") || strstr(String, ".png") )
    {
      p_Textures->array[v7].data.Alpha = 0;
      v10 = v29->TFHiOpaque;
    }
    else
    {
      p_Textures->array[v7].data.Alpha = 0;
      v10 = v29->TFOpaque;
    }
  }
  if ( v29->TextureDetail || !mipmaps )
  {
    String = (char *)1;
    v16 = source;
    v17 = v29->lpD3DDev->CreateTexture(
            source->Width,
            source->Height,
            mipmaps ? 0 : 1,
            0,
            v10,
            D3DPOOL_MANAGED,
            &p_Textures->array[v7].data.lpTexture,
            0);
  }
  else
  {
    String = 0;
    v16 = source;
    v17 = v29->lpD3DDev->CreateTexture(
            source->Width >> 1,
            source->Height >> 1,
            mipmaps ? 0 : 1,
            0,
            v10,
            D3DPOOL_MANAGED,
            &p_Textures->array[v7].data.lpTexture,
            0);
  }
  if ( v17 )
  {
    v22 = DXGetErrorStringA(v17);
    sprintf(atmstr, "%s: %s", "SGepard::CreateTextureFromBitmap: CreateTexture", v22);
    Logger.g->Panic(atmstr);
  }
  source = (SBitmap *)p_Textures->array[v7].data.lpTexture->GetLevelCount();
  new (&bmap32) SBitmap(v16, D3DFMT_A8R8G8B8);
  v20 = 0;
  for ( i = 0; v20 < (int)source; ++v20 )
  {
    if ( v20 || !String )
      bmap32.NextMipLevel();
    new (&levelbitmap) STextureBitmap(p_Textures->array[v7].data.lpTexture, v20);
    *(unsigned char*)&i = (unsigned char)(1);
    levelbitmap.BitBlt(0, 0, bmap32.Width, bmap32.Height, &bmap32, 0, 0);
    *(unsigned char*)&i = (unsigned char)(0);
    levelbitmap.~STextureBitmap();
  }
  (&bmap32)->~SBitmap();
  return v27;
}

//----- (0042FD80) --------------------------------------------------------

int SGepard::CreateTextureFromDXT(const char *dxt_filename, const char *filename, bool mipmaps)
{
  // STUBBED — original uses heavy raw pointer offset math for Textures heap access
  // Needs full rewrite to properly access STextureProp fields
  SStream *is;
  STextureBitmap levelbitmap;
  SBitmap bmap;
  char atmstr[200];

  is = FileSystem.OpenRead(dxt_filename, "SGepard::CreateTextureFromDXT");
  int idx = this->Textures.Add();
  auto &tex = this->Textures.array[idx].data;
  tex.FileName = _strdup(filename);
  tex.RefCount = 1;
  tex.MipMaps = mipmaps;

  is->ReadSignature();
  if ( is->ReadChunkHeader() != 1415071060 )
  {
    throw "Not a texture file";
  }

  int alpha_type = is->ReadInt();
  tex.Width = is->ReadInt();
  tex.Height = is->ReadInt();
  D3DFORMAT dxt_format = (D3DFORMAT)is->ReadInt();

  D3DFORMAT target_format;
  if ( alpha_type == 2 || alpha_type == 3 )
    target_format = this->TFAlpha;
  else if ( alpha_type == 1 )
    target_format = this->TF1Bit;
  else
    target_format = this->TFOpaque;
  tex.Alpha = alpha_type;

  bool halfSize = (this->TextureDetail == 0) && mipmaps;
  HRESULT hr;
  if ( halfSize )
  {
    hr = this->lpD3DDev->CreateTexture(tex.Width >> 1, tex.Height >> 1,
            mipmaps ? 0 : 1, 0, target_format, D3DPOOL_MANAGED, &tex.lpTexture, 0);
    if ( hr )
    {
      sprintf(atmstr, "%s: %s", "SGepard::CreateTextureFromDXT: CreateTexture", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    // Skip first mip level
    SBitmap skipbmap(tex.Width, tex.Height, dxt_format, 0);
    int skipSize = skipbmap.GetLogicalSize();
    is->Seek(skipSize, 1);
    skipbmap.~SBitmap();
  }
  else
  {
    hr = this->lpD3DDev->CreateTexture(tex.Width, tex.Height,
            mipmaps ? 0 : 1, 0, target_format, D3DPOOL_MANAGED, &tex.lpTexture, 0);
    if ( hr )
    {
      sprintf(atmstr, "%s: %s", "SGepard::CreateTextureFromDXT: CreateTexture", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
  }

  int numLevels = tex.lpTexture->GetLevelCount();
  for ( int i = 0; i < numLevels; ++i )
  {
    new (&levelbitmap) STextureBitmap(tex.lpTexture, i);
    if ( dxt_format == target_format )
    {
      int sz = levelbitmap.GetLogicalSize();
      is->Read(levelbitmap.Data, sz);
    }
    else
    {
      new (&bmap) SBitmap(levelbitmap.Width, levelbitmap.Height, dxt_format, 0);
      int sz = bmap.GetLogicalSize();
      is->Read(bmap.Data, sz);
      levelbitmap.BitBlt(0, 0, bmap.Width, bmap.Height, &bmap, 0, 0);
      (&bmap)->~SBitmap();
    }
    levelbitmap.~STextureBitmap();
  }
  if ( !mipmaps )
    is->ReadChunkSkip();
  is->ReadChunkValidate(numLevels);
  is->Release();
  return idx;
}

//----- (004301D0) --------------------------------------------------------

int SGepard::CreateTextureFromLevelBitmaps(SDArray<SBitmap *> *bitmaps, const char *name)

{
  SHeap<STextureProp> *p_Textures; // esi
  int size;
  SBitmap *v5; // eax
  int Height;
  D3DFORMAT Format;
  int v8;
  SHeap<STextureProp>::__Tstruct *array; // esi
  SHeap<STextureProp>::__Tstruct *v10; // esi
  char *v11; // eax
  SGepard *v12; // ecx
  int v13;
  SHeap<STextureProp> *v14; // edi
  bool v16;
  HRESULT v19;
  int i;
  int TextureDetail;
  int v22;
  const char *v24; // eax
  STextureBitmap levelbitmap;
  int v27;
  SDArray<SBitmap *> *v28;
  int v29;
  SGepard *v30;
  D3DFORMAT v31;
  int Width;
  int v33;
  char atmstr[200];
  int v35;
  v30 = this;
  p_Textures = &this->Textures;
  v28 = bitmaps;
  size = bitmaps->size;
  v5 = *bitmaps->array;
  Width = v5->Width;
  Height = v5->Height;
  Format = v5->Format;
  v33 = Height;
  v31 = Format;
  v8 = this->Textures.Add();
  array = p_Textures->array;
  v27 = v8;
  v29 = 32 * v8;
  v10 = &array[v8];
  v11 = _strdup(name);
  v12 = v30;
  v13 = Width;
  v14 = &v30->Textures;
  v10->data.FileName = v11;
  v14->array[v29 / 0x20u].data.RefCount = 1;
  v14->array[v29 / 0x20u].data.Width = v13;
  v14->array[v29 / 0x20u].data.Height = v33;
  v14->array[v29 / 0x20u].data.MipMaps = 1;
  v16 = v12->TextureDetail == 0;
  if ( v16 )
    v19 = v12->lpD3DDev->CreateTexture(
            Width >> 1,
            v33 >> 1,
            --size,
            0,
            v31,
            D3DPOOL_MANAGED,
            &v14->array[v29 / 0x20u].data.lpTexture,
            0);
  else
    v19 = v12->lpD3DDev->CreateTexture(
            Width,
            v33,
            size,
            0,
            v31,
            D3DPOOL_MANAGED,
            &v14->array[v29 / 0x20u].data.lpTexture,
            0);
  if ( v19 )
  {
    v24 = DXGetErrorStringA(v19);
    sprintf(atmstr, "%s: %s", "SGepard::CreateTextureFromBitmap: CreateTexture", v24);
    Logger.g->Panic(atmstr);
  }
  for ( i = 0; i < size; ++i )
  {
    TextureDetail = v30->TextureDetail;
    new (&levelbitmap) STextureBitmap(v30->Textures.array[v29 / 0x20u].data.lpTexture, i);
    v35 = 0;
    v22 = i;
    if ( !TextureDetail )
      v22 = i + 1;
    levelbitmap.BitBlt(0, 0, levelbitmap.Width, levelbitmap.Height, v28->array[v22], 0, 0);
    v35 = -1;
    levelbitmap.~STextureBitmap();
  }
  return v27;
}

//----- (004303E0) --------------------------------------------------------

void SGepard::CreateTriangles(SHillRing *_hillring)

{
  SMesh *HillMesh2; // ecx
  SMesh *v4; // eax
  SMesh *v5; // eax
  SBaseVertex *v6; // edi
  int v7;
  SGepard *v8; // ecx
  SHoleInfo *first; // eax
  S3Vertex *Next3Vertex; // eax
  int v11;
  // __int128 v12 removed — SSE copy replaced with memcpy
  int size;
  int v14;
  int v15;
  int v16;
  int v17;
  int v18;
  SGepard *v19; // ecx
  int v20;
  S3Vertex *v21; // eax
  SChain<SNodeInfo2> *v22;
  S3Vertex result;
  S3Vertex _3vertex;
  SDArray<SBaseVertex> TriBuf;
  SChain<SNodeInfo2> *_nodechain;
  SGepard *v27;
  int v28;
  int v29;
  SHillRing *_hillringa;
  SHillRing *_hillringb;
  SHillRing *_hillringc;
  v27 = this;
  HillMesh2 = _hillring->HillMesh2;
  if ( HillMesh2 )
  {
    HillMesh2->Release();
    _hillring->HillMesh2 = 0;
  }
  v4 = (SMesh *)operator new(sizeof(SMesh));
  v29 = 0;
  if ( v4 )
    v5 = new (v4) SMesh(this, 0x11u);
  else
    v5 = 0;
  _hillring->HillMesh2 = v5;
  v6 = 0;
  v7 = 0;
  memset(&TriBuf, 0, sizeof(TriBuf));
  v8 = v27;
  v29 = 1;
  first = v27->GlobalHoleList.first;
  v27->GlobalHoleList.current = first;
  if ( first )
  {
    _nodechain = first->nodechain;
    Next3Vertex = v8->GetNext3Vertex(&result, _nodechain, 0);
    v11 = 0;
    memcpy(&_3vertex.v1.x , &Next3Vertex->v1.x, 16);
    memcpy(&_3vertex.v2.y, &Next3Vertex->v2.y, 16);
    _3vertex.v3.z = Next3Vertex->v3.z;
    size = 0;
    while ( 1 )
    {
      if ( v11 == v7 )
      {
        if ( v7 >= 16 )
          v14 = 6 * v7 / 5;
        else
          v14 = 16;
        v28 = v14;
        _hillringa = (SHillRing *)v11;
        v6 = (SBaseVertex *)realloc(v6, 36 * v14);
        TriBuf.array = v6;
        memset(&v6[v7], 0, 36 * (v28 - v7));
        v7 = v28;
        v11 = (int)_hillringa;
        size = TriBuf.size;
        TriBuf.maxsize = v28;
      }
      v15 = size;
      v6[v15].x = _3vertex.v1.x;
      v16 = v11 + 1;
      v6[v15].y = _3vertex.v1.y;
      v28 = v11 + 1;
      v6[v15].z = _3vertex.v1.z;
      if ( v11 + 1 == v7 )
      {
        if ( v7 >= 16 )
          v17 = 6 * v7 / 5;
        else
          v17 = 16;
        _hillringb = (SHillRing *)v17;
        v6 = (SBaseVertex *)realloc(v6, 36 * v17);
        TriBuf.array = v6;
        memset(&v6[v7], 0, 36 * ((_DWORD)_hillringb - v7));
        v7 = (int)_hillringb;
        v16 = v28;
        TriBuf.maxsize = (int)_hillringb;
      }
      *(D3DXVECTOR3 *)&v6[v16].x = _3vertex.v2;
      if ( v16 + 1 == v7 )
      {
        if ( v7 >= 16 )
          v18 = 6 * v7 / 5;
        else
          v18 = 16;
        _hillringc = (SHillRing *)v18;
        v6 = (SBaseVertex *)realloc(v6, 36 * v18);
        TriBuf.array = v6;
        memset(&v6[v7], 0, 36 * ((_DWORD)_hillringc - v7));
        v7 = (int)_hillringc;
        v16 = v28;
        TriBuf.maxsize = (int)_hillringc;
      }
      v19 = v27;
      TriBuf.size = v16 + 2;
      v20 = v16;
      v6[v20 + 1].x = _3vertex.v3.x;
      v6[v20 + 1].y = _3vertex.v3.y;
      v22 = _nodechain;
      v6[v20 + 1].z = _3vertex.v3.z;
      v21 = v19->GetNext3Vertex(&result, v22, 1);
      memcpy(&_3vertex.v1.x , &v21->v1.x, 16);
      memcpy(&_3vertex.v2.y, &v21->v2.y, 16);
      _3vertex.v3.z = v21->v3.z;
      size = TriBuf.size;
      v11 = TriBuf.size;
    }
  }
}

//----- (004307D0) --------------------------------------------------------

void SGepard::DeleteAllNodeInfos()

{
  this->NodeInfos.DeleteAll();
}

//----- (004307E0) --------------------------------------------------------

void SGepard::DeleteSelectedNodes(SHillRing *_hillring, SSplineRing *_splinering)

{
  SChain<SSplineRing> *SplineChain; // ecx
  SChain<SSplineRing> *v5; // ecx
  SChain<SSplineRing> *v6; // ecx
  SChain<SSplineRing> *v7; // ecx
  SplineChain = _hillring->SplineChain;
  if ( SplineChain->Closed && !SplineChain->first )
    Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
  SplineChain->first->spline->CopySelectionFrom(_splinering->spline);
  v5 = _hillring->SplineChain;
  if ( v5->Closed && !v5->last )
    Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
  v5->last->spline->CopySelectionFrom(_splinering->spline);
  v6 = _hillring->SplineChain;
  if ( v6->Closed && !v6->first )
    Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
  v6->first->spline->DeleteSelectedNodes();
  v7 = _hillring->SplineChain;
  if ( v7->Closed && !v7->last )
    Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
  v7->last->spline->DeleteSelectedNodes();
  this->RecalculateCentralNodes(_hillring);
  this->RefreshSubNodes(_hillring);
}

//----- (004308C0) --------------------------------------------------------

void SGepard::DeleteSelectedShader()

{
  SShaderInfo *SelectedShader; // ecx
  SShaderInfo *v3; // eax
  SMesh *mesh; // ecx
  SShaderInfo *prev; // ebx
  SShaderInfo *next; // edi
  SelectedShader = this->SelectedShader;
  if ( SelectedShader )
  {
    this->ReleaseTexture(SelectedShader->ShaderTHandle, 0);
    v3 = this->SelectedShader;
    mesh = v3->mesh;
    if ( mesh )
    {
      mesh->Release();
      this->SelectedShader->mesh = 0;
      v3 = this->SelectedShader;
    }
    if ( v3 )
    {
      prev = v3->prev;
      next = v3->next;
      if ( v3->next )
        next->prev = prev;
      if ( prev )
        prev->next = next;
      ::operator delete(v3);
      if ( !next )
        this->ShaderInfos.last = prev;
      if ( prev )
        next = this->ShaderInfos.first;
      else
        this->ShaderInfos.first = next;
      --this->ShaderInfos.NumItems;
      this->ShaderInfos.current = next;
    }
    this->SelectedShader = 0;
  }
}

//----- (00430970) --------------------------------------------------------

void SGepard::DeselectAllNode(SSplineRing *_splinering)

{
  if ( !_splinering )
    Logger.g->Panic("SGepard::SelectAllNode: incoming parameter, _splinering is NULL");
  _splinering->spline->DeselectAllNode();
}

//----- (004309A0) --------------------------------------------------------

void SGepard::DestroyEffectsHandler()

{
  SEffect *Effect; // esi
  Effect = this->Effect;
  if ( Effect )
  {
    delete this->Effect;
    this->Effect = 0;
  }
}

//----- (004309D0) --------------------------------------------------------

void SGepard::DestroyHill(SHillRing *_hillring)

{
  SHillRing *v3; // ecx
  SChain<SSplineRing> *SplineChain; // esi
  SSplineRing *first; // eax
  SSplineRing *current; // ebx
  SSpline *spline; // edi
  SSplineRing *next; // edi
  SSplineRing *prev; // ebx
  SSplineRing *v10; // eax
  SHillRing *v11; // esi
  SHillRing *v12; // edi
  SGepard *v13;
  v3 = _hillring;
  v13 = this;
  if ( _hillring )
  {
    SplineChain = _hillring->SplineChain;
    first = SplineChain->first;
    SplineChain->current = SplineChain->first;
    if ( first )
    {
      current = first;
      do
      {
        spline = first->spline;
        if ( spline )
        {
          delete first->spline;
          current = SplineChain->current;
        }
        if ( current )
        {
          next = current->next;
          prev = current->prev;
          if ( next )
            next->prev = prev;
          if ( prev )
            prev->next = next;
          operator delete(SplineChain->current);
          if ( next )
          {
            v10 = next;
          }
          else
          {
            SplineChain->last = prev;
            v10 = 0;
          }
          SplineChain->current = v10;
          if ( !prev )
            SplineChain->first = next;
          --SplineChain->NumItems;
        }
        first = SplineChain->current;
        current = first;
      }
      while ( first );
      // this = v13;  // IDA artifact
      v3 = _hillring;
    }
    v11 = v3->next;
    v12 = v3->prev;
    if ( v11 )
      v11->prev = v12;
    if ( v12 )
      v12->next = v11;
    ::operator delete(v3);
    if ( !v11 )
      this->HillChain.last = v12;
    if ( v12 )
      v11 = this->HillChain.first;
    else
      this->HillChain.first = v11;
    --this->HillChain.NumItems;
    this->HillChain.current = v11;
    this->gpSelSplineRing = 0;
    this->gpSelHillRing = 0;
  }
}

//----- (00430AD0) --------------------------------------------------------

void SGepard::DestroyLight(unsigned int handle)

{
  this->lpD3DDev->LightEnable(handle, 0);
  this->LightProps[handle].type = 0;
}

//----- (00430B00) --------------------------------------------------------

void SGepard::DestroyPlane()

{
  int *ObjectHash; // eax
  STerrain *Terrain; // edi
  ObjectHash = this->ObjectHash;
  if ( ObjectHash )
  {
    ::operator delete(ObjectHash);
    this->ObjectHash = 0;
  }
  Terrain = this->Terrain;
  if ( Terrain )
  {
    delete this->Terrain;
    this->Terrain = 0;
  }
}

//----- (00430B50) --------------------------------------------------------

void SGepard::DestroyScene()

{
  int i;
  int size;
  SHeap<SSmokeTrail>::__Tstruct *v4; // eax
  int j;
  int v6;
  SHeap<SGroundTrail>::__Tstruct *v7; // eax
  SEffect *Effect; // ecx
  this->DestroyLight(this->SunLight);
  this->DestroySplines();
  this->DestroyShaders(0);
  for ( i = -1; ; this->RemoveSmokeTrail(i) )
  {
    size = this->SmokeTrails.size;
    if ( ++i < size )
    {
      v4 = &this->SmokeTrails.array[i];
      while ( v4->use != 0x7FFFFFFF )
      {
        ++i;
        ++v4;
        if ( i >= size )
          goto LABEL_6;
      }
      if ( i >= 0 )
        continue;
    }
    break;
  }
LABEL_6:
  for ( j = -1; ; this->RemoveGroundTrail(j) )
  {
    v6 = this->GroundTrails.size;
    if ( ++j < v6 )
    {
      v7 = &this->GroundTrails.array[j];
      while ( v7->use != 0x7FFFFFFF )
      {
        ++j;
        ++v7;
        if ( j >= v6 )
          goto LABEL_11;
      }
      if ( j >= 0 )
        continue;
    }
    break;
  }
LABEL_11:
  Effect = this->Effect;
  if ( Effect )
  {
    Effect->DestroyPlayingEffects();
    if ( this->Glow )
      this->Glow = 0;
  }
}

//----- (00430C20) --------------------------------------------------------

void SGepard::DestroyShader2(SShader2Info *s2i)

{
  SVertexLight *vertexlight; // eax
  SShader2Info *next; // esi
  SShader2Info *prev; // ebx
  // F10/F12 brute Shader2Info sweep can leave dangling back-refs in
  // SEffect::EffectsPlaying type-2 entries; gameview.cpp neutralizes
  // those to NULL before sweeping, so the deferred case-2 cleanup
  // funnels NULL into here. Make that path a no-op.
  if ( !s2i )
    return;
  s2i->mesh->Release();
  vertexlight = s2i->vertexlight;
  if ( vertexlight )
  {
    ::operator delete(vertexlight);
    s2i->vertexlight = 0;
  }
  next = s2i->next;
  prev = s2i->prev;
  if ( s2i->next )
    next->prev = prev;
  if ( prev )
    prev->next = next;
  ::operator delete(s2i);
  if ( !next )
    this->Shader2Infos.last = prev;
  if ( prev )
  {
    next = this->Shader2Infos.first;
    --this->Shader2Infos.NumItems;
  }
  else
  {
    --this->Shader2Infos.NumItems;
    this->Shader2Infos.first = next;
  }
  this->Shader2Infos.current = next;
}

//----- (00430CB0) --------------------------------------------------------

void SGepard::DestroyShaders(BOOL dont_remove_textures)

{
  SChain<SShaderInfo> *p_ShaderInfos; // esi
  SShaderInfo *i; // ecx
  SMesh *mesh; // ecx
  SShaderInfo *current; // eax
  SShaderInfo *next; // eax
  p_ShaderInfos = &this->ShaderInfos;
  this->ShaderInfos.current = this->ShaderInfos.first;
  for ( i = this->ShaderInfos.current; i; i = this->ShaderInfos.current )
  {
    this->ReleaseTexture(i->ShaderTHandle, dont_remove_textures);
    mesh = this->ShaderInfos.current->mesh;
    if ( mesh )
    {
      mesh->Release();
      this->ShaderInfos.current->mesh = 0;
    }
    current = p_ShaderInfos->current;
    if ( p_ShaderInfos->Closed )
    {
      if ( !current )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      next = current->next;
      if ( !next )
        next = p_ShaderInfos->first;
    }
    else if ( current )
    {
      next = current->next;
    }
    else
    {
      next = 0;
    }
    p_ShaderInfos->current = next;
  }
  p_ShaderInfos->DeleteAll();
}

//----- (00430D60) --------------------------------------------------------

void SGepard::DestroySpline(SHillRing *_hillring, SSplineRing *_splinering)

{
  SChain<SSplineRing> *SplineChain; // esi
  SSplineRing *prev; // ebx
  SSplineRing *next; // edi
  if ( _hillring )
  {
    SplineChain = _hillring->SplineChain;
    if ( SplineChain )
    {
      if ( _splinering )
      {
        prev = _splinering->prev;
        next = _splinering->next;
        if ( next )
          next->prev = prev;
        if ( prev )
          prev->next = next;
        ::operator delete(_splinering);
        if ( !next )
          SplineChain->last = prev;
        if ( prev )
        {
          next = SplineChain->first;
          --SplineChain->NumItems;
        }
        else
        {
          --SplineChain->NumItems;
          SplineChain->first = next;
        }
        SplineChain->current = next;
      }
    }
  }
}

//----- (00430DD0) --------------------------------------------------------

void SGepard::DestroySplines()

{
  SGepard *v1; // ebx
  SHillRing *first; // eax
  SChain<SSplineRing> *SplineChain; // ebx
  SSplineRing *v4; // eax
  SChain<SRiverStripRing> *RiverStripChain; // esi
  SRiverStripRing *next; // eax
  SVertDiff *strip; // eax
  SRiverStripRing *current; // eax
  SChain<SRiverStripRing> *v9; // edi
  SRiverStripRing *v10; // eax
  SRiverStripRing *v11; // esi
  SSplineRing *v12; // eax
  SSpline *spline; // esi
  SSplineRing *v14; // esi
  SSplineRing *prev; // edi
  SSplineRing *v16; // eax
  SSplineRing *v17; // esi
  SHillRing *v18; // edi
  SHillRing *v19; // esi
  SHillRing *v20; // edi
  SHillRing *v21; // eax
  v1 = this;
  first = this->HillChain.first;
  for ( this->HillChain.current = first; first; first = this->HillChain.current )
  {
    SplineChain = first->SplineChain;
    v4 = SplineChain->first;
    for ( SplineChain->current = SplineChain->first; v4; --SplineChain->NumItems )
    {
      RiverStripChain = v4->spline->RiverStripChain;
      if ( RiverStripChain )
      {
        next = RiverStripChain->first;
        for ( RiverStripChain->current = RiverStripChain->first; next; RiverStripChain->current = next )
        {
          strip = next->strip;
          if ( strip )
          {
            ::operator delete(strip);
            RiverStripChain->current->strip = 0;
          }
          current = RiverStripChain->current;
          if ( RiverStripChain->Closed )
          {
            if ( !current )
              Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
            next = current->next;
            if ( !next )
              next = RiverStripChain->first;
          }
          else if ( current )
          {
            next = current->next;
          }
          else
          {
            next = 0;
          }
        }
      }
      v9 = SplineChain->current->spline->RiverStripChain;
      if ( v9 )
      {
        v10 = v9->first;
        if ( v9->first )
        {
          do
          {
            v11 = v10->next;
            ::operator delete(v10);
            v10 = v11;
          }
          while ( v11 );
        }
        v9->first = 0;
        v9->last = 0;
        v9->current = 0;
        v9->NumItems = 0;
        ::operator delete(v9);
        SplineChain->current->spline->RiverStripChain = 0;
      }
      v12 = SplineChain->current;
      spline = v12->spline;
      if ( spline )
      {
        delete v12->spline;
        v12 = SplineChain->current;
      }
      if ( !v12 )
        break;
      v14 = v12->next;
      prev = v12->prev;
      if ( v14 )
        v14->prev = prev;
      if ( prev )
        prev->next = v14;
      operator delete(SplineChain->current);
      if ( v14 )
      {
        v4 = v14;
      }
      else
      {
        SplineChain->last = prev;
        v4 = 0;
      }
      SplineChain->current = v4;
      if ( !prev )
        SplineChain->first = v14;
    }
    v16 = SplineChain->first;
    if ( SplineChain->first )
    {
      do
      {
        v17 = v16->next;
        ::operator delete(v16);
        v16 = v17;
      }
      while ( v17 );
    }
    SplineChain->first = 0;
    SplineChain->last = 0;
    SplineChain->current = 0;
    SplineChain->NumItems = 0;
    ::operator delete(SplineChain);
    v1 = this;
    v18 = this->HillChain.current;
    if ( v18 )
    {
      v19 = v18->next;
      v20 = v18->prev;
      if ( v19 )
        v19->prev = v20;
      if ( v20 )
        v20->next = v19;
      operator delete(this->HillChain.current);
      if ( v19 )
      {
        v21 = v19;
      }
      else
      {
        this->HillChain.last = v20;
        v21 = 0;
      }
      this->HillChain.current = v21;
      if ( !v20 )
        this->HillChain.first = v19;
      --this->HillChain.NumItems;
    }
  }
  v1->gpSelSplineRing = 0;
  v1->gpSelHillRing = 0;
}

//----- (00431010) --------------------------------------------------------

void SGepard::DoAltitude1(SHillRing *selhillring)

{
  SChain<SSplineRing> *SplineChain; // ecx
  SSplineRing *current; // eax
  SSplineRing *next; // eax
  SSpline *spline; // ecx
  SNode *first; // eax
  SNode *v7; // eax
  if ( selhillring )
  {
    this->HeightFrom = 1;
    selhillring->SplineChain->current = selhillring->SplineChain->first;
    SplineChain = selhillring->SplineChain;
    current = SplineChain->current;
    if ( SplineChain->Closed )
    {
      if ( !current )
LABEL_21:
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      next = current->next;
      if ( !next )
        next = SplineChain->first;
    }
    else if ( current )
    {
      next = current->next;
    }
    else
    {
      next = 0;
    }
    SplineChain->current = next;
    spline = selhillring->SplineChain->current->spline;
    first = spline->Nodes.first;
    for ( spline->Nodes.current = first; first; spline->Nodes.current = first )
    {
      if ( first->Selected )
        first->HeightFrom = 1;
      v7 = spline->Nodes.current;
      if ( spline->Nodes.Closed )
      {
        if ( !v7 )
          goto LABEL_21;
        first = v7->next;
        if ( !first )
          first = spline->Nodes.first;
      }
      else if ( v7 )
      {
        first = v7->next;
      }
      else
      {
        first = 0;
      }
    }
  }
}

//----- (004310C0) --------------------------------------------------------

void SGepard::DoAltitude2(SHillRing *selhillring)

{
  SChain<SSplineRing> *SplineChain; // ecx
  SSplineRing *current; // eax
  SSplineRing *next; // eax
  SSpline *spline; // ecx
  SNode *first; // eax
  SNode *v7; // eax
  if ( selhillring )
  {
    this->HeightFrom = 2;
    selhillring->SplineChain->current = selhillring->SplineChain->first;
    SplineChain = selhillring->SplineChain;
    current = SplineChain->current;
    if ( SplineChain->Closed )
    {
      if ( !current )
LABEL_21:
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      next = current->next;
      if ( !next )
        next = SplineChain->first;
    }
    else if ( current )
    {
      next = current->next;
    }
    else
    {
      next = 0;
    }
    SplineChain->current = next;
    spline = selhillring->SplineChain->current->spline;
    first = spline->Nodes.first;
    for ( spline->Nodes.current = first; first; spline->Nodes.current = first )
    {
      if ( first->Selected )
        first->HeightFrom = 2;
      v7 = spline->Nodes.current;
      if ( spline->Nodes.Closed )
      {
        if ( !v7 )
          goto LABEL_21;
        first = v7->next;
        if ( !first )
          first = spline->Nodes.first;
      }
      else if ( v7 )
      {
        first = v7->next;
      }
      else
      {
        first = 0;
      }
    }
  }
}

//----- (00431170) --------------------------------------------------------

void SGepard::DrawDynamicVB(int idx, unsigned int triangles)

{
  SHeap<SDynamicVB>::__Tstruct *array; // ecx
  if ( idx < 0 || idx >= this->DynamicVBs.size || (array = this->DynamicVBs.array, array[idx].use != 0x7FFFFFFF) )
    Logger.g->Panic("SGepard::DrawDynamicVB: Bad index (%d).", idx);
  if ( array[idx].data.Locked )
    Logger.g->Panic("SGepard::DrawDynamicVB: Buffer is locked.");
  if ( triangles )
  {
    this->lpD3DDev->SetStreamSource(0, array[idx].data.lpVertexBuffer, 0, array[idx].data.VertexSize);
    this->lpD3DDev->SetFVF(this->DynamicVBs.array[idx].data.VertexFormat);
    this->lpD3DDev->DrawPrimitive(
      D3DPT_TRIANGLELIST,
      this->DynamicVBs.array[idx].data.LockPosition,
      triangles);
    this->DynamicVBs.array[idx].data.LockPosition += 3 * triangles;
  }
}

//----- (00431240) --------------------------------------------------------

void SGepard::DrawLakesToWaterMap()

{
  if ( g_HeadlessMode ) return;
  SGepard *v1; // edx
  int v2;
  int size;
  SHeap<SGLake>::__Tstruct *i; // eax
  SHeap<SGLake>::__Tstruct *array; // eax
  int v6;
  int v7;
  SMesh *v8; // esi
  unsigned int j;
  int v10;
  float height;
  int v13;
  int v14;
  int v15;
  v1 = this;
  v2 = -1;
  while ( 1 )
  {
    size = v1->Lakes.size;
    v13 = ++v2;
    if ( v2 >= size )
      break;
    for ( i = &v1->Lakes.array[v2]; i->use != 0x7FFFFFFF; ++i )
    {
      v13 = ++v2;
      if ( v2 >= size )
        return;
    }
    if ( v2 < 0 )
      break;
    array = v1->Lakes.array;
    v6 = v2;
    v15 = 0;
    height = array[v2].data.Y - 0.30000001f;
    if ( array[v2].data.Blocks.size > 0 )
    {
      // x64: original walked Blocks.array via byte stride `v7 += 16` — x86-only.
      // sizeof(SLakeBlock) is 32 on x64 (BlockIdx + padding + 2 SMesh* + int + tail
      // padding), so v8 read garbage like 0x1 (the tail RecalcHeightTransparency).
      // Replaced with typed array indexing.
      v15 = 0;
      do
      {
        v8 = array[v6].data.Blocks.array[v15].Mesh;
        v8->LockVertexBuffer();
        j = 0;
        while ( j < v8->GetNumVertices() )
        {
          v10 = v8->OffsetXYZ + j * v8->VertexSize;
          this->Terrain->RaiseHeight((int)*(float *)((char *)v8->lpVertices + v10),
            (int)*(float *)((char *)v8->lpVertices + v10 + 8),
            height);
          ++j;
        }
        v8->UnlockVertexBuffer();
        array = this->Lakes.array;
        ++v15;
        v6 = v2;
      }
      while ( v15 < array[v2].data.Blocks.size );
      v2 = v13;
      v1 = this;
    }
  }
}

//----- (00431360) --------------------------------------------------------

void SGepard::DrawNode(bool cp, float x, float y, float z, _D3DCOLORVALUE color)
{
  SGroup *node = cp ? this->node1 : this->node2;
  node->SetPosition(x, y, z);
  node->ForceUpdate();
  node->Precalculate();
  node->SetAmbient(color);
  node->Draw();
}

//----- (00431420) --------------------------------------------------------

void SGepard::DrawWater(SHillRing *_hillring, int drawmode, float _scroll, SStream *is)

{
  SGepard *v5; // edi
  SChain<SSplineRing> *SplineChain; // eax
  SSplineRing *first; // ecx
  bool Closed; // al
  bool v9;
  SSplineRing *next; // eax
  SSpline *spline; // esi
  SRiverStripRing *v12; // esi
  float *p_x; // edx
  int v14;
  float v15; // xmm5_4
  float v16; // xmm3_4
  float v17; // xmm6_4
  float v18; // xmm2_4
  float v19; // xmm4_4
  float v20; // xmm7_4
  float v21; // xmm0_4
  float v22; // xmm6_4
  float *v23; // ecx
  float v24; // xmm5_4
  float v25; // xmm0_4
  float v26; // xmm4_4
  float v27; // xmm7_4
  float v28; // xmm1_4
  int v29;
  int v30;
  float *p_z; // eax
  float v32; // xmm1_4
  float v33; // xmm0_4
  SEffect *Effect; // ecx
  float v37; // xmm2_4
  float _randomx; // xmm4_4
  float v39; // xmm0_4
  int v40;
  float v75;
  float v76;
  float v79;
  float v80;
  float v81;
  float z1;
  float z1a;
  float z1_4;
  float z1_4a;
  float v88;
  float v89;
  float v90;
  float v91;
  float v92;
  SChain<SRiverStripRing> *RiverStripChain;
  float x2;
  float v97;
  float v98;
  int v99;
  v5 = this;
  SplineChain = _hillring->SplineChain;
  first = SplineChain->first;
  Closed = SplineChain->Closed;
  if ( !first )
  {
    if ( Closed )
      Logger.g->Panic("GetSecond: <first> is NULL");
    goto LABEL_65;
  }
  v9 = !Closed;
  next = first->next;
  if ( !v9 )
  {
    if ( !next )
      Logger.g->Panic("GetSecond: In a closed chain <first->next> is NULL");
    goto LABEL_6;
  }
  if ( !next )
LABEL_65:
    Logger.g->Panic("SGepard::DrawWater: A megadott spline sorozat masodik tagja ures!");
LABEL_6:
  spline = next->spline;
  v5->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, 1u);
  v5->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, 1u);
  if ( drawmode )
  {
    RiverStripChain = spline->RiverStripChain;
    if ( !RiverStripChain )
      goto LABEL_63;
    v12 = RiverStripChain->first;
    v98 = (-2.0f);
    v91 = (-2.0f);
    v88 = (-2.0f);
    v92 = (-2.0f);
    v81 = (-2.0f);
    RiverStripChain->current = RiverStripChain->first;
    if ( !v12 )
      goto LABEL_63;
    while ( 1 )
    {
      p_x = &v12->strip->x;
      z1 = p_x[6];
      z1_4 = p_x[8];
      v79 = p_x[6 * v12->numvertices - 6];
      v75 = p_x[6 * v12->numvertices - 4];
      v89 = *p_x;
      v90 = p_x[2];
      x2 = p_x[6 * v12->numvertices - 12];
      v97 = p_x[6 * v12->numvertices - 10];
      v5->WaterLine(*p_x, v90, x2, v97, p_x[1] - 0.30000001f);
      v5->WaterLine(z1, z1_4, v79, v75, v12->strip->y - 0.30000001f);
      v14 = 0;
      v15 = v90;
      v16 = z1_4;
      v17 = v75 - z1_4;
      v18 = z1;
      v19 = v89;
      v20 = v79 - z1;
      do
      {
        v99 = v14 + 1;
 v5->WaterLine((float)((float)(x2 - v89) * (float)((float)(v14 + 1) * 0.25)) + v19,
          (float)((float)(v97 - v90) * (float)((float)(v14 + 1) * 0.25)) + v15,
          (float)(v20 * (float)((float)(v14 + 1) * 0.25)) + v18,
          (float)(v17 * (float)((float)(v14 + 1) * 0.25)) + v16,
          v12->strip->y - 0.30000001f);
        v14 = v99;
        v18 = z1;
        v16 = z1_4;
        v19 = v89;
        v15 = v90;
        v17 = v75 - z1_4;
        v20 = v79 - z1;
      }
      while ( v99 < 4 );
      z1a = (float)(z1 + v89) * 0.5f;
      z1_4a = (float)(z1_4 + v90) * 0.5f;
      v80 = (float)(v79 + x2) * 0.5f;
      v76 = (float)(v75 + v97) * 0.5f;
      v21 = (float)((double)sqrtf((z1a - v80) * (z1a - v80) + (z1_4a - v76) * (z1_4a - v76)));

      if ( v21 > 8.0 )
      {
        v28 = 2.0f;
        v29 = (int)(float)((float)((float)(v21 - 7.9000001) * 0.25) + 2.0);
        if ( v29 > 14 )
          v29 = 14;
        v12->numverify = v29;
        v12->verify[0].x = z1a;
        v12->verify[0].z = z1_4a;
        v12->verify[1].x = v80;
        v12->verify[1].z = v76;
        if ( v12->numverify > 2 )
        {
          v30 = 2;
          p_z = &v12->verify[2].z;
          do
          {
            v32 = (float)(1.0 / (float)((float)v12->numverify - v28)) * (float)++v30;
            *(p_z - 1) = (float)((float)(v80 - z1a) * v32) + z1a;
            v33 = (float)(v76 - z1_4a) * v32;
            v28 = 2.0f;
            *p_z = v33 + z1_4a;
            p_z += 2;
          }
          while ( v30 < v12->numverify );
        }
      }
      else
      {
        v12->numverify = 2;
        v12->verify[0].x = z1a;
        v12->verify[0].z = z1_4a;
        v12->verify[1].x = v80;
        v12->verify[1].z = v76;
      }
      v22 = 1.0f;
      v23 = &v12->strip->x;
      if ( v23[1] <= v23[7] )
      {
        v24 = v98;
        v26 = v88;
        v27 = v81;
        v25 = v91;
      }
      else
      {
        v24 = v23[7];
        v25 = v23[6];
        v91 = v25;
        v92 = v23[8];
        v26 = v23[6 * v12->numvertices - 6];
        v27 = v23[6 * v12->numvertices - 4];
        v88 = v26;
        v81 = v27;
        v98 = v24;
      }
      // x64: was `current = (int*)RiverStripChain->current; v12 =
      // (SRiverStripRing*)*current` — reading `next` as a 4-byte int from an
      // 8-byte ptr field truncates the high half.
      {
        SRiverStripRing *cur = RiverStripChain->current;
        if ( RiverStripChain->Closed )
        {
          if ( !cur )
            goto LABEL_63;
          v12 = cur->next;
          if ( !v12 )
            v12 = RiverStripChain->first;
        }
        else if ( cur )
        {
          v12 = cur->next;
        }
        else
        {
          v12 = 0;
        }
      }
      RiverStripChain->current = v12;
      if ( !v12 )
      {
        // "vizeses" = waterfall spray emitter at the bottom of a water spline.
        // IDA + Ghidra agree: y is `v24 - v22` (strip top minus 1.0, i.e. a
        // unit below the top of the falling strip), and the trailing
        // scalespeed is 0.5f (0x3F000000 in both decompiles), not 0.0f.
        // Guarded by Effect != NULL because the editor build never calls
        // CreateEffectsHandler (no SSuperWindow), so Gepard->Effect is null
        // there. Original editor.exe's DrawWater has the same guard.
        Effect = v5->Effect;
        if ( v25 != -2.0 && Effect )
        {
          v37 = (float)(v26 + v25) * 0.5f;
          _randomx = fabsf(v26 - v91);
          v39 = fabsf(v27 - v92);
          if ( _randomx <= v39 )
            v40 = Effect->LoadEffect("vizeses", 0, 2.0, v39);
          else
            v40 = Effect->LoadEffect("vizeses", 0, _randomx, 2.0);
          Effect->PlayEffect(v40, v37, v24 - v22, (float)(v27 + v92) * 0.5f, 0.5f);
        }
        goto LABEL_63;
      }
    }
  }
  if ( !drawmode )  // drawmode == 0: render river water surface
  {
    v5->lpD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    v5->lpD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
    v5->lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

    // Populate-on-empty: if the spline has no cached strips (e.g. user
    // just authored a spline with no save+reload, or a stock map's saved
    // strip data was missing/truncated for this spline), compute fresh
    // strips from the spline's nodes once and render those. The existing
    // render path below reads canonical V from the cache and adds _scroll
    // (line 6701), and the helper writes canonical V — so the math
    // matches. Only fires when is==NULL (we're in the no-stream render
    // path here) AND in editor mode (this->Effect == NULL — the game
    // calls CreateEffectsHandler at startup, the editor never does).
    //
    // Game-mode gate added because firing this in game mode regresses
    // SParticles3 effects (heli_celfust, egysegfust*, vizeses) — they
    // submit valid geometry to D3D but render nothing visible. Bisect
    // identified 635467d as the trigger; root cause inside the populate
    // path not yet pinned. Editor mode does not use SParticles3 at all
    // (Gepard->Effect == NULL there) so this gate has no editor cost.
    if (!this->Effect && !is && (!spline->RiverStripChain || !spline->RiverStripChain->first)) {
      v5->DrawWater_ComputeAndStream(_hillring, _scroll, 0);
    }

    SChain<SRiverStripRing> *riverChain = spline->RiverStripChain;
    if (!riverChain)
      goto LABEL_63;
    SRiverStripRing *strip = riverChain->first;
    riverChain->current = strip;
    if (!strip)
      goto LABEL_63;

    do
    {
      // Visibility check: at least one verify point must be in a visible parcel
      bool visible = false;
      for (int vi = 0; vi < strip->numverify; ++vi)
      {
        float vx = strip->verify[vi].x;
        float vz = strip->verify[vi].z;
        if (vx < 0.0f || vx >= (float)v5->Terrain->XSize)
          continue;
        if (vz < 0.0f || vz >= (float)v5->Terrain->ZSize)
          continue;
        int col = (int)(vx * 0.125f);
        int row = (int)(vz * 0.125f);
        int parcelIdx = col + v5->Terrain->XParcels * row;
        if (v5->Terrain->Parcels[parcelIdx].Visible)
        {
          visible = true;
          break;
        }
      }

      if (visible)
      {
        // Pass 1: Base water surface
        struct WaterVert { float x, y, z; unsigned int diff; float u, v; };
        WaterVert waterVerts[100];

        v5->lpD3DDev->SetFVF(0x142); // D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1
        v5->DrawType = DT_NORMAL;
        v5->SetTexture(0, v5->_folyo, 1);
        // Force the Alpha==2 (alpha-blend + alpha-test) state for water Pass 1
        // regardless of what _folyo's texture metadata reports. Some shipped /
        // repacked effects/teszt_folyo_a.dxt headers carry alpha_type=0, which
        // sends SetTexture down the opaque branch (gepard.cpp:8653-8656:
        // ALPHABLENDENABLE=0, ZWRITE=1). Per-vertex SVertDiff alpha (encoded by
        // DrawWater_ComputeAndStream as a shoreline fade) is then ignored — the
        // river loses its blue tint over terrain — and ZWRITE=1 makes the
        // opaque water Z-fight the bridge mesh, visibly narrowing the river
        // under bridges. Pass 2 (sparkle, DT_ADD) sets its own blend so it
        // survives — which exactly matches the user-visible symptom.
        v5->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
        v5->lpD3DDev->SetRenderState(D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA);
        v5->lpD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
        v5->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 1u);
        v5->lpD3DDev->SetRenderState(D3DRS_ALPHAREF, 5u);
        v5->lpD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
        v5->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 0u);

        unsigned int numVerts = strip->numvertices;
        // Recompute per-vertex shoreline-fade alpha each frame from the
        // *pre-FixBridges* heightmap. Stock GOG maps ship strips with
        // diff=0x00FFFFFF (alphaByte=0) — without this re-bake, Pass 1
        // contributes nothing and only Pass 2's additive sparkle remains
        // visible (matches the regression report).
        //
        // SGepard::SetTessFolyoAlpha (gepard.cpp:8578) computes the same
        // formula but uses Terrain->GetHeight, which returns the
        // *post-FixBridges* HeightMap. FixBridges raises HeightMap cells
        // under and around bridges to bridge-deck height, so calling
        // SetTessFolyoAlpha here would clamp alpha near zero across most
        // of the river (river shows as nearly transparent — exactly what
        // we observed even after wiring the call up). Retail water is
        // deep blue across the full width, so retail must compute against
        // the unmodified riverbed elevation. GetOldHeight (terrain.h)
        // queries OldHeightMap, the snapshot taken before FixBridges runs
        // (gameworld.cpp:5447), which preserves the original riverbed Y.
        // Inlining the formula here so we pick the right heightmap; this
        // bypasses the leftover SetTessFolyoAlpha utility entirely.
        for (unsigned int i = 0; i < numVerts; ++i) {
          SVertDiff &sv = strip->strip[i];
          float h = (float)v5->Terrain->GetOldHeight(sv.x, sv.z);
          int a = (int)((sv.y - h) * 100.0f);
          if (a < 0) a = 0;
          if (a > 160) a = 160;
          sv.diff = ((unsigned int)a << 24) | 0xFFFFFFu;
        }
        for (unsigned int i = 0; i < numVerts; ++i)
        {
          waterVerts[i].x = strip->strip[i].x;
          waterVerts[i].y = strip->strip[i].y;
          waterVerts[i].z = strip->strip[i].z;
          waterVerts[i].diff = strip->strip[i].diff;
          waterVerts[i].u = strip->strip[i].u;
          waterVerts[i].v = strip->strip[i].v + _scroll;
        }
        v5->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, numVerts - 2, waterVerts, 24);

        // Pass 2: Sparkle overlay
        struct SparkleVert { float x, y, z; unsigned int diff; float u1, v1, u2, v2; };
        SparkleVert sparkleVerts[100];
        float sparkleScroll = _scroll * 0.025f;

        for (unsigned int i = 0; i < numVerts; ++i)
        {
          sparkleVerts[i].x = waterVerts[i].x;
          sparkleVerts[i].y = waterVerts[i].y;
          sparkleVerts[i].z = waterVerts[i].z;
          sparkleVerts[i].diff = 0x00FFFFFF;
          float bu = waterVerts[i].u;
          float bv = waterVerts[i].v;
          // SSE args recovered from Ghidra v1.7
          sparkleVerts[i].u1 = sinf(bu * 3.352f + _scroll) * 0.02f + bu + sparkleScroll;
          sparkleVerts[i].v1 = cosf(bv * 4.0f + _scroll) * 0.02f + bv;
          sparkleVerts[i].u2 = bu - sinf(bu * 4.0f + _scroll) * 0.02f;
          sparkleVerts[i].v2 = bv - cosf(bv * 4.152f + _scroll) * 0.02f + sparkleScroll;
        }

        v5->lpD3DDev->SetFVF(0x242); // D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX2
        v5->lpD3DDev->SetRenderState(D3DRS_FOGCOLOR, 0);
        v5->DrawType = DT_ADD;
        v5->SetTexture(0, v5->_folyo_csillogas, 1);
        v5->SetTexture(1, v5->_folyo_csillogas, 1);
        v5->lpD3DDev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        v5->lpD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
        v5->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, numVerts - 2, sparkleVerts, 32);
        v5->lpD3DDev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        v5->lpD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);
        v5->EnableFog();
      }

      // StepToNext in RiverStripChain
      SRiverStripRing *cur = riverChain->current;
      if (riverChain->Closed)
      {
        if (!cur)
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        strip = cur->next;
        if (!strip)
          strip = riverChain->first;
      }
      else
      {
        strip = cur ? cur->next : NULL;
      }
      riverChain->current = strip;
    }
    while (strip);
  }
LABEL_63:
  v5->lpD3DDev->SetRenderState(D3DRS_CULLMODE, 2u);
}

// Compute-from-spline-nodes branch ported from the original editor.exe
// DrawWater (swinehd17/idapro16-donotuse/editorSplit/sgepard.c:6503-8997).
// Walks the second spline ring of `_hillring` pairwise over its nodes,
// derives Altitude/Altitude2 per node, computes perpendicular tangents,
// probes terrain to find left/right edge extents, subdivides each segment
// along the spline direction (count v249), and emits each sub-segment as
// one SRiverStripRing. Per-vertex alpha fade (clamp((y - terrain_y)*100,
// 0, 160)) matches the original's shoreline blend. With is != NULL writes
// strips directly to the stream in LoadSplines format
// (int numvertices + 24*N bytes, then int 0 terminator). With is == NULL
// (Commit 2) appends strips to spline->RiverStripChain. The full HillChain
// joining-prologue (IDA 6872-7072) is intentionally stubbed: joining
// fields stay at -1.0 so the inline `!= -1.0` guards at IDA 7661/7697/7705
// short-circuit and the spline computes in isolation. Visual seams at
// joined splines are an accepted MVP defect.
void SGepard::DrawWater_ComputeAndStream(SHillRing *_hillring, float _scroll, SStream *is)
{
  if (!_hillring) return;
  SChain<SSplineRing> *splineChain = _hillring->SplineChain;
  if (!splineChain || !splineChain->first || !splineChain->first->next) {
    if (is) { int zero = 0; is->Write(&zero, 4); }
    return;
  }
  SSpline *spline = splineChain->first->next->spline;
  if (!spline) {
    if (is) { int zero = 0; is->Write(&zero, 4); }
    return;
  }

  // HillChain joining prologue. Ported from retail editor.exe DrawWater
  // (IDA editorSplit/sgepard.c:6861-7072). Walks Gepard->HillChain looking
  // for OTHER splines whose endpoints touch THIS spline's endpoints — when
  // a match is found (corner-to-corner distance < 2.0) the corresponding
  // FirstLeft/FirstRight/LastLeft/LastRight Joined fields are pulled to
  // overlap the neighbour, so the strip-gen loop below stitches the two
  // splines into one continuous polygon. Without this, Y/T-junctions
  // render as visible triangular gaps where two rivers should merge.
  spline->FirstLeftJoinedX  = -1.0f;
  spline->FirstLeftJoinedZ  = -1.0f;
  spline->FirstRightJoinedX = -1.0f;
  spline->FirstRightJoinedZ = -1.0f;
  spline->LastLeftJoinedX   = -1.0f;
  spline->LastLeftJoinedZ   = -1.0f;
  spline->LastRightJoinedX  = -1.0f;
  spline->LastRightJoinedZ  = -1.0f;
  spline->UAlign            = -1;
  spline->URatio            = -1.0f;
  spline->JoinedVPos        = -1.0f;

  // Walk every other hill's second-spline ring and try to attach.
  // First pass: THIS.FirstLeft/Right ↔ OTHER.LastLeft/Center/Right.
  // Second pass: THIS.LastLeft/Right ↔ OTHER.FirstLeft/Center/Right.
  {
    SHillRing *savedHill = HillChain.current;

    // ----- First-endpoint joining: THIS.FirstLeft/Right ↔ OTHER.LastLeft/Center/Right -----
    // Geometry: this spline's START is a tributary feeding into OTHER's END.
    //   Config B (left tributary):  FirstLeft↔LastLeft + FirstRight↔LastCenter
    //                               → FirstLeftJoined=LastLeft, FirstRightJoined=LastCenter, UAlign=0
    //   Config C (right tributary): FirstLeft↔LastCenter + FirstRight↔LastRight
    //                               → FirstLeftJoined=LastCenter, FirstRightJoined=LastRight, UAlign=1
    for (SHillRing *otherHill = HillChain.first; otherHill; otherHill = otherHill->next) {
      if (otherHill == _hillring) continue;
      SChain<SSplineRing> *otherChain = otherHill->SplineChain;
      if (!otherChain || !otherChain->first || !otherChain->first->next) continue;
      SSpline *otherSpline = otherChain->first->next->spline;
      if (!otherSpline) continue;
      if (spline->FirstLeftX == -1.0f || otherSpline->LastLeftX == -1.0f) continue;

      float dx, dz;
      dx = spline->FirstLeftX  - otherSpline->LastLeftX;
      dz = spline->FirstLeftZ  - otherSpline->LastLeftZ;
      float dFL_LL = sqrtf(dx*dx + dz*dz);
      dx = spline->FirstLeftX  - otherSpline->LastCenterX;
      dz = spline->FirstLeftZ  - otherSpline->LastCenterZ;
      float dFL_LC = sqrtf(dx*dx + dz*dz);
      dx = spline->FirstRightX - otherSpline->LastCenterX;
      dz = spline->FirstRightZ - otherSpline->LastCenterZ;
      float dFR_LC = sqrtf(dx*dx + dz*dz);
      dx = spline->FirstRightX - otherSpline->LastRightX;
      dz = spline->FirstRightZ - otherSpline->LastRightZ;
      float dFR_LR = sqrtf(dx*dx + dz*dz);

      if (dFL_LL < 2.0f && dFR_LC < 2.0f) {
        spline->FirstLeftJoinedX  = otherSpline->LastLeftX;
        spline->FirstLeftJoinedZ  = otherSpline->LastLeftZ;
        spline->FirstRightJoinedX = otherSpline->LastCenterX;
        spline->FirstRightJoinedZ = otherSpline->LastCenterZ;
        spline->UAlign     = 0;
        spline->URatio     = dFL_LL / (dFR_LC + dFL_LL + 0.0001f);
        spline->JoinedVPos = otherSpline->VEndPos;
        Logger.g->Log(0, "Joining: hill=%p first→other=%p (left-trib)  dFL_LL=%.2f dFR_LC=%.2f",
          _hillring, otherHill, dFL_LL, dFR_LC);
        break;
      }
      if (dFL_LC < 2.0f && dFR_LR < 2.0f) {
        spline->FirstLeftJoinedX  = otherSpline->LastCenterX;
        spline->FirstLeftJoinedZ  = otherSpline->LastCenterZ;
        spline->FirstRightJoinedX = otherSpline->LastRightX;
        spline->FirstRightJoinedZ = otherSpline->LastRightZ;
        spline->UAlign     = 1;
        spline->URatio     = dFL_LC / (dFR_LR + dFL_LC + 0.0001f);
        spline->JoinedVPos = otherSpline->VEndPos;
        Logger.g->Log(0, "Joining: hill=%p first→other=%p (right-trib) dFL_LC=%.2f dFR_LR=%.2f",
          _hillring, otherHill, dFL_LC, dFR_LR);
        break;
      }
    }

    // ----- Last-endpoint joining: THIS.LastLeft/Right ↔ OTHER.FirstLeft/Center/Right -----
    // Geometry: this spline's END is a tributary feeding into OTHER's START.
    //   Config B (left tributary):  LastLeft↔FirstLeft + LastRight↔FirstCenter
    //                               → LastLeftJoined=FirstLeft, LastRightJoined=FirstCenter, UAlign=0
    //   Config C (right tributary): LastLeft↔FirstCenter + LastRight↔FirstRight
    //                               → LastLeftJoined=FirstCenter, LastRightJoined=FirstRight, UAlign=1
    for (SHillRing *otherHill = HillChain.first; otherHill; otherHill = otherHill->next) {
      if (otherHill == _hillring) continue;
      SChain<SSplineRing> *otherChain = otherHill->SplineChain;
      if (!otherChain || !otherChain->first || !otherChain->first->next) continue;
      SSpline *otherSpline = otherChain->first->next->spline;
      if (!otherSpline) continue;
      if (spline->LastLeftX == -1.0f || otherSpline->FirstLeftX == -1.0f) continue;

      float dx, dz;
      dx = spline->LastLeftX  - otherSpline->FirstLeftX;
      dz = spline->LastLeftZ  - otherSpline->FirstLeftZ;
      float dLL_FL = sqrtf(dx*dx + dz*dz);
      dx = spline->LastLeftX  - otherSpline->FirstCenterX;
      dz = spline->LastLeftZ  - otherSpline->FirstCenterZ;
      float dLL_FC = sqrtf(dx*dx + dz*dz);
      dx = spline->LastRightX - otherSpline->FirstCenterX;
      dz = spline->LastRightZ - otherSpline->FirstCenterZ;
      float dLR_FC = sqrtf(dx*dx + dz*dz);
      dx = spline->LastRightX - otherSpline->FirstRightX;
      dz = spline->LastRightZ - otherSpline->FirstRightZ;
      float dLR_FR = sqrtf(dx*dx + dz*dz);

      if (dLL_FL < 2.0f && dLR_FC < 2.0f) {
        spline->LastLeftJoinedX  = otherSpline->FirstLeftX;
        spline->LastLeftJoinedZ  = otherSpline->FirstLeftZ;
        spline->LastRightJoinedX = otherSpline->FirstCenterX;
        spline->LastRightJoinedZ = otherSpline->FirstCenterZ;
        spline->UAlign     = 0;
        spline->URatio     = dLL_FL / (dLR_FC + dLL_FL + 0.0001f);
        spline->JoinedVPos = otherSpline->VStartPos - spline->RiverLength;
        Logger.g->Log(0, "Joining: hill=%p last→other=%p (left-trib)  dLL_FL=%.2f dLR_FC=%.2f",
          _hillring, otherHill, dLL_FL, dLR_FC);
        break;
      }
      if (dLL_FC < 2.0f && dLR_FR < 2.0f) {
        spline->LastLeftJoinedX  = otherSpline->FirstCenterX;
        spline->LastLeftJoinedZ  = otherSpline->FirstCenterZ;
        spline->LastRightJoinedX = otherSpline->FirstRightX;
        spline->LastRightJoinedZ = otherSpline->FirstRightZ;
        spline->UAlign     = 1;
        spline->URatio     = dLL_FC / (dLR_FR + dLL_FC + 0.0001f);
        spline->JoinedVPos = otherSpline->VStartPos - spline->RiverLength;
        Logger.g->Log(0, "Joining: hill=%p last→other=%p (right-trib) dLL_FC=%.2f dLR_FR=%.2f",
          _hillring, otherHill, dLL_FC, dLR_FR);
        break;
      }
    }

    HillChain.current = savedHill;
  }

  SNode *firstNode = spline->Nodes.first;
  if (!firstNode) {
    if (is) { int zero = 0; is->Write(&zero, 4); }
    return;
  }
  SNode *secondNode = firstNode->next;
  if (!secondNode && spline->Nodes.Closed) secondNode = firstNode;
  if (!secondNode) {
    if (is) { int zero = 0; is->Write(&zero, 4); }
    return;
  }

  // accumV is canonical V (no _scroll baked in). The render path in
  // existing DrawWater drawmode==0 adds _scroll on top
  // (gepard.cpp:6701), and the IDA stream-write also produces canonical V
  // by subtracting _scroll right before is->Write (IDA 8025-8033). Since
  // SaveSplines passes _scroll=0 we just write canonical directly.
  spline->VStartPos = 0.0f;
  float accumV = 0.0f;
  const float startV = 0.0f;

  spline->Nodes.current = secondNode;
  SNode *startNode = secondNode;
  SNode *cur = startNode;
  bool firstIter = true;

  while (cur) {
    if (!firstIter && cur == startNode) break;  // closed-loop completion
    firstIter = false;

    SNode *n_a = cur;
    SNode *n_b;
    if (spline->Nodes.Closed) {
      n_b = n_a->next;
      if (!n_b) n_b = firstNode;
    } else {
      n_b = n_a->next;
    }
    if (!n_b) break;

    SNode *n_prev;
    if (spline->Nodes.Closed) {
      n_prev = n_a->prev;
      if (!n_prev) n_prev = spline->Nodes.last;
    } else {
      n_prev = n_a->prev;
    }
    if (!n_prev) break;

    SNode *n_c;
    if (spline->Nodes.Closed) {
      n_c = n_b->next;
      if (!n_c) n_c = firstNode;
    } else {
      n_c = n_b->next;
    }
    // IDA exits the outer loop when n_c is NULL (line 7180), so a
    // non-closed spline of N nodes processes pairs (n_a, n_b) for n_a in
    // [first->next .. last->prev->prev], producing N-2 sub-segment
    // groups. Closed splines never produce NULL n_c (always wrap).
    if (!n_c) break;

    const float n_prev_x = n_prev->x, n_prev_z = n_prev->z;
    const float n_a_x = n_a->x, n_a_z = n_a->z;
    const float n_b_x = n_b->x, n_b_z = n_b->z;
    const float n_c_x = n_c->x, n_c_z = n_c->z;

    // Altitude/Altitude2 derivation (IDA 7194-7235).
    float Altitude;
    if (n_a->ControlPoint) {
      Altitude = (n_a->HeightFrom == 1) ? spline->Altitude : spline->Altitude2;
    } else {
      Altitude = n_a->y;
    }
    float Altitude2;
    if (n_b->ControlPoint) {
      Altitude2 = (n_b->HeightFrom == 1) ? spline->Altitude : spline->Altitude2;
    } else {
      Altitude2 = n_b->y;
    }

    // Perpendicular tangents (IDA 7306-7317). At n_a the tangent direction
    // is approximately n_prev->n_b (smoothed), at n_b it's n_a->n_c.
    // left = (-dz, dx); right = (dz, -dx).
    D3DXVECTOR2 left_at_a, right_at_a, left_at_b, right_at_b;
    left_at_a.x  = -(n_b_z - n_prev_z); left_at_a.y  =  (n_b_x - n_prev_x);
    right_at_a.x =  (n_b_z - n_prev_z); right_at_a.y = -(n_b_x - n_prev_x);
    left_at_b.x  = -(n_c_z - n_a_z);    left_at_b.y  =  (n_c_x - n_a_x);
    right_at_b.x =  (n_c_z - n_a_z);    right_at_b.y = -(n_c_x - n_a_x);
    D3DXVec2Normalize(&left_at_a,  &left_at_a);
    D3DXVec2Normalize(&right_at_a, &right_at_a);
    D3DXVec2Normalize(&left_at_b,  &left_at_b);
    D3DXVec2Normalize(&right_at_b, &right_at_b);

    // Edge-extent terrain probing (IDA 7356-7449). Walks outward in 0.2
    // unit steps until terrain rises above water altitude. Initial 1.0
    // default fires when no terrain hit within 30 units.
    n_a->waterleftsize = 1.0f; n_a->waterrightsize = 1.0f;
    n_b->waterleftsize = 1.0f; n_b->waterrightsize = 1.0f;
    for (int i = 0; i < 150; ++i) {
      float t = (float)i * 0.2f;
      float h = (float)Terrain->GetOldHeight(n_a_x + t * left_at_a.x, n_a_z + t * left_at_a.y);
      if (h > Altitude) { n_a->waterleftsize = t; break; }
    }
    for (int i = 0; i < 150; ++i) {
      float t = (float)i * 0.2f;
      float h = (float)Terrain->GetOldHeight(n_b_x + t * left_at_b.x, n_b_z + t * left_at_b.y);
      if (h > Altitude2) { n_b->waterleftsize = t; break; }
    }
    for (int i = 0; i < 150; ++i) {
      float t = (float)i * 0.2f;
      float h = (float)Terrain->GetOldHeight(n_a_x + t * right_at_a.x, n_a_z + t * right_at_a.y);
      if (h > Altitude) { n_a->waterrightsize = t; break; }
    }
    for (int i = 0; i < 150; ++i) {
      float t = (float)i * 0.2f;
      float h = (float)Terrain->GetOldHeight(n_b_x + t * right_at_b.x, n_b_z + t * right_at_b.y);
      if (h > Altitude2) { n_b->waterrightsize = t; break; }
    }

    // Apply edge extents.
    left_at_a.x  *= n_a->waterleftsize;  left_at_a.y  *= n_a->waterleftsize;
    left_at_b.x  *= n_b->waterleftsize;  left_at_b.y  *= n_b->waterleftsize;
    right_at_a.x *= n_a->waterrightsize; right_at_a.y *= n_a->waterrightsize;
    right_at_b.x *= n_b->waterrightsize; right_at_b.y *= n_b->waterrightsize;

    float FirstLeftJoinedX  = n_a_x + left_at_a.x;
    float FirstLeftJoinedZ  = n_a_z + left_at_a.y;
    float LastLeftJoinedX   = n_b_x + left_at_b.x;
    float LastLeftJoinedZ   = n_b_z + left_at_b.y;
    float FirstRightJoinedX = n_a_x + right_at_a.x;
    float FirstRightJoinedZ = n_a_z + right_at_a.y;
    float LastRightJoinedX  = n_b_x + right_at_b.x;
    float LastRightJoinedZ  = n_b_z + right_at_b.y;

    // Joining override: when this is the spline's FIRST sub-segment and the
    // prologue found a neighbour for the start endpoint, pull the start
    // corners onto that neighbour's matching corners. Symmetric for LAST
    // sub-segment. Without these overrides the prologue's work is dead and
    // adjacent splines render as disconnected triangles at junctions.
    SNode *lastButOne_join = spline->Nodes.last ? spline->Nodes.last->prev : 0;
    if (cur == startNode && spline->FirstLeftJoinedX != -1.0f) {
      FirstLeftJoinedX  = spline->FirstLeftJoinedX;
      FirstLeftJoinedZ  = spline->FirstLeftJoinedZ;
      FirstRightJoinedX = spline->FirstRightJoinedX;
      FirstRightJoinedZ = spline->FirstRightJoinedZ;
    }
    if (n_b == lastButOne_join && spline->LastLeftJoinedX != -1.0f) {
      LastLeftJoinedX  = spline->LastLeftJoinedX;
      LastLeftJoinedZ  = spline->LastLeftJoinedZ;
      LastRightJoinedX = spline->LastRightJoinedX;
      LastRightJoinedZ = spline->LastRightJoinedZ;
    }

    // Segment length along spline; V increments by length/12 per node-pair
    // (IDA 7546).
    const float seg_dx = n_b_x - n_a_x;
    const float seg_dz = n_b_z - n_a_z;
    const float seg_len = sqrtf(seg_dx * seg_dx + seg_dz * seg_dz);
    const float thisV = accumV;
    const float endV = accumV + seg_len * (1.0f / 12.0f);

    // Spline first/last cap positions (IDA 7561-7644). Used by joining
    // (which is stubbed), but match original for completeness.
    if (cur == startNode) {
      spline->FirstLeftX  = FirstLeftJoinedX;
      spline->FirstLeftZ  = FirstLeftJoinedZ;
      spline->FirstRightX = FirstRightJoinedX;
      spline->FirstRightZ = FirstRightJoinedZ;
      spline->FirstCenterX = n_a_x;
      spline->FirstCenterZ = n_a_z;
    }
    SNode *lastButOne = spline->Nodes.last ? spline->Nodes.last->prev : 0;
    if (n_b == lastButOne) {  // last processed iter for non-closed
      spline->LastLeftX  = LastLeftJoinedX;
      spline->LastLeftZ  = LastLeftJoinedZ;
      spline->LastRightX = LastRightJoinedX;
      spline->LastRightZ = LastRightJoinedZ;
      spline->LastCenterX = n_b_x;
      spline->LastCenterZ = n_b_z;
      spline->VEndPos = endV;
    }

    // Default U range (joining stub: URatio==-1 means uniform 0..1).
    const float u_left_start  = 0.0f, u_left_end  = 0.0f;
    const float u_right_start = 1.0f, u_right_end = 1.0f;

    // Subdivision count along the spline (IDA 7727-7729).
    int subdivCount = (int)(seg_len + 0.5f);
    if (subdivCount <= 0) subdivCount = 1;

    for (int i = 0; i < subdivCount; ++i) {
      const float t0 = (float)i / (float)subdivCount;
      const float t1 = (float)(i + 1) / (float)subdivCount;

      // Y interpolated linearly Altitude -> Altitude2 across the
      // node-pair: this is how the waterfall drop is implicit (IDA 7763).
      const float yStart = Altitude  + (Altitude2 - Altitude) * t0;
      const float yEnd   = Altitude  + (Altitude2 - Altitude) * t1;

      const float left_x_s  = FirstLeftJoinedX  + (LastLeftJoinedX  - FirstLeftJoinedX ) * t0;
      const float left_z_s  = FirstLeftJoinedZ  + (LastLeftJoinedZ  - FirstLeftJoinedZ ) * t0;
      const float right_x_s = FirstRightJoinedX + (LastRightJoinedX - FirstRightJoinedX) * t0;
      const float right_z_s = FirstRightJoinedZ + (LastRightJoinedZ - FirstRightJoinedZ) * t0;
      const float left_x_e  = FirstLeftJoinedX  + (LastLeftJoinedX  - FirstLeftJoinedX ) * t1;
      const float left_z_e  = FirstLeftJoinedZ  + (LastLeftJoinedZ  - FirstLeftJoinedZ ) * t1;
      const float right_x_e = FirstRightJoinedX + (LastRightJoinedX - FirstRightJoinedX) * t1;
      const float right_z_e = FirstRightJoinedZ + (LastRightJoinedZ - FirstRightJoinedZ) * t1;

      const float segV = endV - thisV;
      const float v_at_t0 = thisV + segV * t0;
      const float v_at_t1 = thisV + segV * t1;
      const float u_l_t0 = u_left_start  + (u_left_end  - u_left_start ) * t0;
      const float u_l_t1 = u_left_start  + (u_left_end  - u_left_start ) * t1;
      const float u_r_t0 = u_right_start + (u_right_end - u_right_start) * t0;
      const float u_r_t1 = u_right_start + (u_right_end - u_right_start) * t1;

      // Lateral subdivision count: average of river widths at start and end
      // of sub-segment, ceil'd (IDA 7874-7879).
      const float w1_dx = right_x_s - left_x_s;
      const float w1_dz = right_z_s - left_z_s;
      const float w1 = sqrtf(w1_dx * w1_dx + w1_dz * w1_dz);
      const float w2_dx = right_x_e - left_x_e;
      const float w2_dz = right_z_e - left_z_e;
      const float w2 = sqrtf(w2_dx * w2_dx + w2_dz * w2_dz);
      int latCount = (int)ceilf((w1 + w2) * 0.5f);
      if (latCount < 0) latCount = 0;

      // Skip degenerate sub-segments. Retail editor.exe at IDA
      // editorSplit/sgepard.c:7909 has an explicit `if (latCount > 0)`
      // guard around the entire lateral-subdivision + cap-fill block —
      // when probed waterleftsize/waterrightsize collapse to ~0 (e.g. the
      // spline runs through a bridge cell whose heightmap was raised by
      // FixBridges or by JezusAtmentAVizenMiMiertNe→WaterLine→RaiseHeight),
      // retail simply emits no strip for that sub-segment. Without this
      // guard our port wrote a degenerate 4-vertex strip at exactly the
      // bridge XZ — render-time `(y - terrain) * 100` then clamps alpha to
      // 0 across all 4 verts, polygon area is zero, and the river visibly
      // pinches off under the bridge. Match retail: skip the sub-segment.
      if (latCount <= 0) continue;

      const int numverts = 2 * latCount + 4;
      SVertDiff *strip = (SVertDiff*)::operator new[](sizeof(SVertDiff) * (size_t)numverts);

      // Helper to fill one vertex with alpha-fade based on terrain delta.
      // GetOldHeight reads the pre-FixBridges / pre-WaterLine snapshot —
      // same reason we use it for runtime per-vertex alpha rebake in
      // DrawWater Pass 1 (commit 45818b1). Probing modified HeightMap
      // here would bake artificially-low alpha into disk strips that
      // would survive future loads.
      auto fillVert = [&](SVertDiff *v, float x, float y, float z, float u, float vc) {
        v->x = x; v->y = y; v->z = z; v->u = u; v->v = vc;
        float h = (float)Terrain->GetOldHeight(x, z);
        int a = (int)((y - h) * 100.0f);
        if (a < 0) a = 0;
        if (a > 160) a = 160;
        v->diff = ((unsigned int)a << 24) | 0xFFFFFFu;
      };

      // Cap 0: left edge, sub-segment START.
      fillVert(&strip[0], left_x_s, yStart, left_z_s, u_l_t0, v_at_t0);
      // Cap 1: left edge, sub-segment END.
      fillVert(&strip[1], left_x_e, yEnd,   left_z_e, u_l_t1, v_at_t1);

      // Inner lateral interp pairs at lt = (j+1)/(latCount+1), j = 0..latCount-1.
      for (int j = 0; j < latCount; ++j) {
        const float lt = (float)(j + 1) / (float)(latCount + 1);
        const int vs = 2 + 2 * j;
        const int ve = vs + 1;
        fillVert(&strip[vs],
                 left_x_s + lt * (right_x_s - left_x_s),
                 yStart,
                 left_z_s + lt * (right_z_s - left_z_s),
                 u_l_t0  + lt * (u_r_t0    - u_l_t0   ),
                 v_at_t0);
        fillVert(&strip[ve],
                 left_x_e + lt * (right_x_e - left_x_e),
                 yEnd,
                 left_z_e + lt * (right_z_e - left_z_e),
                 u_l_t1  + lt * (u_r_t1    - u_l_t1   ),
                 v_at_t1);
      }

      // Final cap: right edge.
      fillVert(&strip[2 + 2 * latCount],     right_x_s, yStart, right_z_s, u_r_t0, v_at_t0);
      fillVert(&strip[2 + 2 * latCount + 1], right_x_e, yEnd,   right_z_e, u_r_t1, v_at_t1);

      if (is) {
        // Stream mode (called from SaveSplines): write strip and free.
        // Format matches LoadSplines reader (gepard.cpp:12581-12614):
        // unsigned int numvertices, then 24*N bytes of SVertDiff. Caller
        // SaveSplines passes _scroll=0, so we don't need IDA's de-scroll
        // pass (IDA 8025-8033) — vertices are already canonical.
        unsigned int nv = (unsigned int)numverts;
        is->Write(&nv, 4);
        is->Write(strip, (size_t)24 * (size_t)numverts);
        ::operator delete[](strip);
      } else {
        // Cache-populate mode (called from DrawWater drawmode==0 when
        // RiverStripChain is empty). Append a fresh SRiverStripRing to the
        // chain; existing render path adds _scroll on top of canonical V
        // (gepard.cpp:6701) and recomputes verify[] on first drawmode==1
        // call (matches LoadSplines's lazy-numverify behavior at
        // gepard.cpp:12615).
        if (!spline->RiverStripChain) {
          spline->RiverStripChain = new SChain<SRiverStripRing>();
          spline->RiverStripChain->first = 0;
          spline->RiverStripChain->last = 0;
          spline->RiverStripChain->current = 0;
          spline->RiverStripChain->Closed = false;
          spline->RiverStripChain->NumItems = 0;
        }
        SRiverStripRing *rsr = (SRiverStripRing*)::operator new(sizeof(SRiverStripRing));
        memset(rsr, 0, sizeof(SRiverStripRing));
        rsr->numvertices = (unsigned int)numverts;
        rsr->strip = strip;        // takes ownership of the allocation
        rsr->numverify = 0;
        SChain<SRiverStripRing> *rsc = spline->RiverStripChain;
        if (rsc->last) {
          rsc->last->next = rsr;
          rsr->prev = rsc->last;
          rsc->last = rsr;
          rsr->next = 0;
        } else {
          rsc->first = rsr;
          rsc->last = rsr;
        }
        ++rsc->NumItems;
        rsc->current = rsr;
      }
    }

    accumV = endV;

    SNode *next_cur;
    if (spline->Nodes.Closed) {
      next_cur = n_a->next;
      if (!next_cur) next_cur = firstNode;
    } else {
      next_cur = n_a->next;
    }
    cur = next_cur;
    spline->Nodes.current = cur;
  }

  spline->RiverLength = accumV - startV;

  if (is) {
    int zero = 0;
    is->Write(&zero, 4);  // strip-list terminator (IDA 8092-8097)
  }
}

//----- (00441CD0) --------------------------------------------------------

void SGepard::ReplaceObject(SIObject *original, int cacheidx)

{
  int v3 = cacheidx;
  if ( cacheidx < 0
    || cacheidx >= this->ObjectCache.size
    || this->ObjectCache.array[cacheidx].use != 0x7FFFFFFF )
  {
    Logger.g->Panic("SGepard::CreateObject: Invalid cache index");
  }
  ++this->ObjectCache.array[cacheidx].data.RefCount;

  // IDA used (bool *)&cacheidx + 3 hack — clean replacement
  bool isDynamic;
  int i;
  original->GetInternalIndex(&isDynamic, &i);

  if ( isDynamic )
  {
    int oldIdx = this->DynamicObjects.array[i].data.CacheIdx;
    --this->ObjectCache.array[oldIdx].data.RefCount;
    this->DynamicObjects.array[i].data.CacheIdx = v3;
    this->DynamicObjects.array[i].data.Group->Replace(this->ObjectCache.array[v3].data.Group);
  }
  else
  {
    int oldIdx = this->StaticObjects.array[i].data.CacheIdx;
    --this->ObjectCache.array[oldIdx].data.RefCount;
    this->StaticObjects.array[i].data.CacheIdx = v3;
    this->StaticObjects.array[i].data.Group->Replace(this->ObjectCache.array[v3].data.Group);
  }
}

//----- (00441DE0) --------------------------------------------------------

void SGepard::ReplaceObject(SIObject *original, const char *filename, float scale)

{
  int v5;
  v5 = this->PrecacheObject(filename, scale, (SDrawType)0);
  if ( v5 >= 0 )
    this->ReplaceObject(original, v5);
}

//----- (00441E20) --------------------------------------------------------

void SGepard::RegisterResetCallbacks(void (*pre)(void *), void (*post)(void *), void *userData)
{
  this->PreResetCallback = pre;
  this->PostResetCallback = post;
  this->ResetCallbackUserData = userData;
}

char SGepard::ResetDevice(int a2)

{
  IDirect3DTexture9 *Ptr; // ecx
  IDirect3DTexture9 *v4; // ecx
  int v5;
  int size;
  SHeap<SDynamicVB>::__Tstruct *v7; // eax
  IDirect3DSurface9 *v8; // ecx
  IDirect3DSurface9 *v9; // ecx
  IDirect3DVertexBuffer9 *lpVertexBuffer; // ecx
  int v12;
  int v13;
  SHeap<SDynamicVB>::__Tstruct *i; // eax
  SHeap<SDynamicVB>::__Tstruct *v15; // ebx
  HRESULT v16;
  const char *v17; // eax
  char _Buffer[200];
  int saved_a2 = a2; // IDA reuses a2 as loop temp — preserve original
  Ptr = this->ShadowMapTexture.Ptr;
  if ( Ptr )
  {
    Ptr->Release();
    this->ShadowMapTexture.Ptr = 0;
  }
  v4 = this->ShadowMapDepthTexture.Ptr;
  if ( v4 )
  {
    v4->Release();
    this->ShadowMapDepthTexture.Ptr = 0;
  }
  this->ClearObjectShadows(1, 0);
  v5 = -1;
  while ( 1 )
  {
    size = this->DynamicVBs.size;
    if ( ++v5 >= size )
      break;
    v7 = &this->DynamicVBs.array[v5];
    while ( v7->use != 0x7FFFFFFF )
    {
      ++v5;
      ++v7;
      if ( v5 >= size )
        goto LABEL_10;
    }
    if ( v5 < 0 )
      break;
    lpVertexBuffer = this->DynamicVBs.array[v5].data.lpVertexBuffer;
    if ( lpVertexBuffer )
    {
      lpVertexBuffer->Release();
      this->DynamicVBs.array[v5].data.lpVertexBuffer = 0;
    }
  }
LABEL_10:
  v8 = this->ResolutionScaleSurface.Ptr;
  if ( v8 )
  {
    v8->Release();
    this->ResolutionScaleSurface.Ptr = 0;
  }
  v9 = this->ResolutionScaleDepthSurface.Ptr;
  if ( v9 )
  {
    v9->Release();
    this->ResolutionScaleDepthSurface.Ptr = 0;
  }
  this->SetUpPresentation();
  // Let any non-engine consumer (e.g. SMarket's HQ 3D preview) drop its
  // D3DPOOL_DEFAULT resources before Reset, otherwise Reset returns
  // D3DERR_INVALIDCALL and the device is left in a broken state.
  if ( this->PreResetCallback )
    this->PreResetCallback(this->ResetCallbackUserData);
  Logger.g->Log(0, "ResetDevice: calling Reset...");
  HRESULT resetHr = this->lpD3DDev->Reset(&this->PresentationParameters);
  if ( resetHr < 0 )
  {
    Logger.g->Log(0, "ResetDevice: Reset failed (hr=0x%08X)", (unsigned)resetHr);
    return 0;
  }
  Logger.g->Log(0, "ResetDevice: Reset OK, calling InitTextureFormats...");
  this->InitTextureFormats();
  Logger.g->Log(0, "ResetDevice: calling InitRenderStates...");
  this->InitRenderStates();
  Logger.g->Log(0, "ResetDevice: calling InitResolutionScale...");
  this->InitResolutionScale(saved_a2, (int)this);
  // Now the device is fully ready — let the consumer recreate its resources.
  if ( this->PostResetCallback )
    this->PostResetCallback(this->ResetCallbackUserData);
  Logger.g->Log(0, "ResetDevice: InitResolutionScale done, restoring VBs...");
  v12 = -1;
  while ( 1 )
  {
    v13 = this->DynamicVBs.size;
    if ( ++v12 >= v13 )
      break;
    for ( i = &this->DynamicVBs.array[v12]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v12 >= v13 )
        return 1;
    }
    if ( v12 < 0 )
      break;
    v15 = &this->DynamicVBs.array[v12];
    if ( !v15->data.lpVertexBuffer )
    {
      v16 = this->lpD3DDev->CreateVertexBuffer(
                            v15->data.NumVertices * v15->data.VertexSize,
              520u,
              v15->data.VertexFormat,
              D3DPOOL_DEFAULT,
              (IDirect3DVertexBuffer9 **)&v15->data,
              0);
      if ( v16 )
      {
        v17 = DXGetErrorStringA(v16);
        sprintf(_Buffer, "%s: %s", "SGepard::RestoreDynamicVBs: IDirect3DDevice9::CreateVertexBuffer", v17);
        Logger.g->Panic(_Buffer);
      }
    }
  }
  return 1;
}

//----- (00442020) --------------------------------------------------------

void SGepard::ResetShadowMapTexture(unsigned int stage)

{
  this->lpD3DDev->SetSamplerState(stage, D3DSAMP_ADDRESSU, 1u);
  this->lpD3DDev->SetSamplerState(stage, D3DSAMP_ADDRESSV, 1u);
}

//----- (00442060) --------------------------------------------------------

void SGepard::Resize(int a2, unsigned int width, unsigned int height)

{
  SBoard *Board; // ecx
  if ( !this->FullScreen )
  {
    Board = this->Board;
    this->ViewWidth = width;
    this->ViewHeight = height;
    Board->ResizeFrame(0, width, height);
    this->ResetDevice(a2);
  }
}

//----- (004420A0) --------------------------------------------------------

void SGepard::RestoreDynamicVBs()

{
  int v2;
  int size;
  SHeap<SDynamicVB>::__Tstruct *i; // eax
  SHeap<SDynamicVB>::__Tstruct *v5; // edi
  HRESULT v6;
  const char *v7; // eax
  char atmstr[200];
  v2 = -1;
  while ( 1 )
  {
    size = this->DynamicVBs.size;
    if ( ++v2 >= size )
      break;
    for ( i = &this->DynamicVBs.array[v2]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v2 >= size )
        return;
    }
    if ( v2 < 0 )
      break;
    v5 = &this->DynamicVBs.array[v2];
    if ( !v5->data.lpVertexBuffer )
    {
      v6 = this->lpD3DDev->CreateVertexBuffer(
                          v5->data.NumVertices * v5->data.VertexSize,
             520u,
             v5->data.VertexFormat,
             D3DPOOL_DEFAULT,
             (IDirect3DVertexBuffer9 **)&v5->data,
             0);
      if ( v6 )
      {
        v7 = DXGetErrorStringA(v6);
        sprintf(atmstr, "%s: %s", "SGepard::RestoreDynamicVBs: IDirect3DDevice9::CreateVertexBuffer", v7);
        Logger.g->Panic(atmstr);
      }
    }
  }
}

//----- (00442180) --------------------------------------------------------

void SGepard::RotateShaderToLeft()

{
  SShaderInfo *SelectedShader; // ecx
  int v3;
  SShaderInfo **v4; // ebx
  SShaderInfo *v5; // eax
  SMesh *mesh; // ecx
  SelectedShader = this->SelectedShader;
  if ( SelectedShader )
  {
    v3 = 0;
    if ( SelectedShader->Rotation + 1 <= 3 )
      v3 = SelectedShader->Rotation + 1;
    v4 = (SShaderInfo **)this->CreateShader(SelectedShader->xpos, SelectedShader->zpos,
                           SelectedShader->filename,
                           v3,
                           false,
                           false);
    if ( v4 )
    {
      this->ReleaseTexture(this->SelectedShader->ShaderTHandle, 0);
      v5 = this->SelectedShader;
      mesh = v5->mesh;
      if ( mesh )
      {
        mesh->Release();
        this->SelectedShader->mesh = 0;
        v5 = this->SelectedShader;
      }
      *v4 = v5->next;
      v4[1] = this->SelectedShader->prev;
      memcpy(this->SelectedShader, v4, sizeof(SShaderInfo));
      free(v4);
    }
  }
}

//----- (00442250) --------------------------------------------------------

void SGepard::RotateShaderToRight()

{
  SShaderInfo *SelectedShader; // ecx
  int v3;
  SShaderInfo **v4; // ebx
  SShaderInfo *v5; // eax
  SMesh *mesh; // ecx
  SelectedShader = this->SelectedShader;
  if ( SelectedShader )
  {
    v3 = SelectedShader->Rotation - 1;
    if ( v3 < 0 )
      v3 = 3;
    v4 = (SShaderInfo **)this->CreateShader(SelectedShader->xpos, SelectedShader->zpos,
                           SelectedShader->filename,
                           v3,
                           false,
                           false);
    if ( v4 )
    {
      this->ReleaseTexture(this->SelectedShader->ShaderTHandle, 0);
      v5 = this->SelectedShader;
      mesh = v5->mesh;
      if ( mesh )
      {
        mesh->Release();
        this->SelectedShader->mesh = 0;
        v5 = this->SelectedShader;
      }
      *v4 = v5->next;
      v4[1] = this->SelectedShader->prev;
      memcpy(this->SelectedShader, v4, sizeof(SShaderInfo));
      free(v4);
    }
  }
}

//----- (00442320) --------------------------------------------------------

void SGepard::RoundToTextureSize(unsigned int width, unsigned int height, int *tex_width, int *tex_height)

{
  if ( this->TFSupportNonPow2 )
  {
    *tex_width = width;
    *tex_height = height;
  }
  else
  {
    *tex_width = RoundUp2Pow2(width);
    *tex_height = RoundUp2Pow2(height);
  }
}

//----- (004424D0) --------------------------------------------------------

void SGepard::SaveShaders(SStream *is)

{
  SShaderInfo *current; // edi
  SShaderInfo *v4; // eax
  SShaderInfo *next; // eax
  this->ShaderInfos.current = this->ShaderInfos.first;
  is->WriteInt(this->ShaderInfos.NumItems);
  current = this->ShaderInfos.current;
  if ( current )
  {
    do
    {
      is->Write(current->filename, 120);
      is->WriteFloat(current->xpos);
      is->WriteFloat(current->zpos);
      is->WriteInt(current->Rotation);
      v4 = this->ShaderInfos.current;
      if ( this->ShaderInfos.Closed )
      {
        if ( !v4 )
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        next = v4->next;
        if ( !next )
          next = this->ShaderInfos.first;
      }
      else if ( v4 )
      {
        next = v4->next;
      }
      else
      {
        next = 0;
      }
      current = next;
      this->ShaderInfos.current = next;
    }
    while ( next );
  }
}

//----- (004425A0) --------------------------------------------------------

void SGepard::SaveSplines(SStream *is, unsigned int internal)

{
  SGepard *v3; // ebx
  SHillRing *first; // eax
  SChain<SSplineRing> *SplineChain; // ebx
  SSplineRing *v6; // eax
  SSpline *spline; // esi
  bool Closed; // cl
  SSplineRing *next; // eax
  SNode *v16; // eax
  SNode *v17; // eax
  SNode *v18; // eax
  SSplineRing *v19; // eax
  SSplineRing *v20; // eax
  SHillRing *v21; // eax
  SHillRing *v22; // eax
  v3 = this;
  if ( !(_BYTE)internal )
    this->CleanupSplines();
  v3->Temp = 0;
  first = v3->HillChain.first;
  v3->HillChain.current = first;
  if ( first )
  {
    do
    {
      is->WriteInt(123456);
      is->WriteInt(1);
      // x64 fix: mirror LoadSplines — write 24 bytes in the canonical x86
      // SHillRing layout regardless of the host arch's struct size. Matches
      // the format the editor (x86) emits and that LoadSplines now expects.
      {
        struct OnDiskHillRing {
          int   SplineChainPtr_unused;
          int   Type;
          int   WaterEnabled_packed;
          float RiverSize;
          int   HillMesh_unused;
          int   HillMesh2_unused;
        };
        OnDiskHillRing diskHill = {};
        diskHill.Type                 = v3->HillChain.current->Type;
        diskHill.WaterEnabled_packed  = v3->HillChain.current->WaterEnabled ? 1 : 0;
        diskHill.RiverSize            = v3->HillChain.current->RiverSize;
        is->Write(&diskHill, sizeof(diskHill));
      }
      SplineChain = v3->HillChain.current->SplineChain;
      v6 = SplineChain->first;
      SplineChain->current = SplineChain->first;
      if ( v6 )
      {
        do
        {
          is->WriteInt(123456);
          is->WriteInt(1);
          is->Write(&SplineChain->current->spline, 4);
          spline = SplineChain->current->spline;
          is->Write(&spline->Altitude, 4);
          is->Write(&spline->Altitude2, 4);
          is->Write(&spline->CheckBoxNr, 4);
          is->Write(&spline->Closed, 1);
          is->Write(&spline->MiddleSpline, 1);
          is->Write(&spline->VisibilityState, 1);
          is->Write(&spline->URatio, 4);
          is->Write(&spline->UAlign, 4);
          Closed = SplineChain->Closed;
          if ( SplineChain->first )
          {
            next = SplineChain->first->next;
            if ( Closed && !next )
              Logger.g->Panic("GetSecond: In a closed chain <first->next> is NULL");
          }
          else
          {
            if ( Closed )
              Logger.g->Panic("GetSecond: <first> is NULL");
            next = 0;
          }
          if ( SplineChain->current == next )
          {
            // Compute strip geometry from spline nodes and stream it,
            // matching the original editor.exe's DrawWater(_, 0, 0.0f, is)
            // behavior. The helper writes `int numvertices + 24*N bytes`
            // per strip and a trailing `int 0` terminator, which is the
            // format LoadSplines reads at gepard.cpp:12581-12614.
            v3->DrawWater_ComputeAndStream(v3->HillChain.current, 0.0f, is);
          }
          v16 = spline->Nodes.first;
          spline->Nodes.current = v16;
          if ( v16 )
          {
            do
            {
              is->WriteInt(123456);
              is->WriteInt(1);
              is->Write(&spline->Nodes.current->ControlPoint, 28);
              v17 = spline->Nodes.current;
              if ( spline->Nodes.Closed )
              {
                if ( !v17 )
                  goto LABEL_51;
                v18 = v17->next;
                if ( !v18 )
                  v18 = spline->Nodes.first;
              }
              else if ( v17 )
              {
                v18 = v17->next;
              }
              else
              {
                v18 = 0;
              }
              spline->Nodes.current = v18;
            }
            while ( v18 );
          }
          is->WriteInt(123456);
          is->WriteInt(0);
          v19 = SplineChain->current;
          if ( SplineChain->Closed )
          {
            if ( !v19 )
              goto LABEL_51;
            v20 = v19->next;
            if ( !v20 )
              v20 = SplineChain->first;
          }
          else if ( v19 )
          {
            v20 = v19->next;
          }
          else
          {
            v20 = 0;
          }
          SplineChain->current = v20;
        }
        while ( v20 );
      }
      is->WriteInt(123456);
      is->WriteInt(0);
      v3 = this;
      ++this->Temp;
      v21 = this->HillChain.current;
      if ( this->HillChain.Closed )
      {
        if ( !v21 )
LABEL_51:
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        v22 = v21->next;
        if ( !v22 )
          v22 = this->HillChain.first;
      }
      else if ( v21 )
      {
        v22 = v21->next;
      }
      else
      {
        v22 = 0;
      }
      this->HillChain.current = v22;
    }
    while ( v22 );
  }
  is->WriteInt(123456);
  is->WriteInt(0);
}

//----- (00442900) --------------------------------------------------------

void SGepard::SaveSplinesSel(SStream *is)

{
  int v2;
  SHillRing *first; // eax
  SHillRing *gpSelHillRing; // esi
  SHillRing *i; // eax
  int v7;
  SSplineRing *v8; // eax
  SSplineRing *gpSelSplineRing; // edx
  SSplineRing *next; // eax
  int SplineIndex;
  int HillIndex;
  v2 = 0;
  first = this->HillChain.first;
  gpSelHillRing = this->gpSelHillRing;
  if ( first )
  {
    if ( first == gpSelHillRing )
      goto LABEL_7;
    for ( i = first->next; i; i = i->next )
    {
      ++v2;
      if ( i == gpSelHillRing )
        goto LABEL_7;
    }
  }
  v2 = -1;
LABEL_7:
  HillIndex = v2;
  if ( !gpSelHillRing )
    goto LABEL_13;
  v7 = 0;
  v8 = gpSelHillRing->SplineChain->first;
  if ( !v8 )
    goto LABEL_13;
  gpSelSplineRing = this->gpSelSplineRing;
  if ( v8 == gpSelSplineRing )
    goto LABEL_14;
  next = v8->next;
  if ( !next )
  {
LABEL_13:
    v7 = -1;
    goto LABEL_14;
  }
  while ( 1 )
  {
    ++v7;
    if ( next == gpSelSplineRing )
      break;
    next = next->next;
    if ( !next )
      goto LABEL_13;
  }
LABEL_14:
  SplineIndex = v7;
  is->Write(&HillIndex, 4);
  is->Write(&SplineIndex, 4);
}

//----- (004429A0) --------------------------------------------------------

void SGepard::SelectAllNode(SSplineRing *_splinering)

{
  if ( !_splinering )
    Logger.g->Panic("SGepard::SelectAllNode: incoming parameter, _splinering is NULL");
  _splinering->spline->SelectAllNode();
}

//----- (004429D0) --------------------------------------------------------

void SGepard::SelectHillRing(SHillRing *_hillring)

{
  this->gpSelHillRing = _hillring;
}

//----- (004429E0) --------------------------------------------------------

void SGepard::SelectNode(SSplineRing *_splinering, SNode *_node)

{
  _splinering->spline->SelectNode(_node);
}

//----- (00442A00) --------------------------------------------------------

void SGepard::SelectSplineRing(SSplineRing *_splinering)

{
  this->gpSelSplineRing = _splinering;
}

//----- (00442A10) --------------------------------------------------------

void SGepard::SelectionFog(unsigned int color)

{
  float f_end;
  float f_start;
  f_end = (float)(2550 - 10 * BYTE3(color));
  f_start = (float)(int)(-10 * BYTE3(color));
  this->lpD3DDev->SetRenderState(D3DRS_RANGEFOGENABLE, 1u);
  this->lpD3DDev->SetRenderState(D3DRS_FOGCOLOR, color);
  this->lpD3DDev->SetRenderState(D3DRS_FOGVERTEXMODE, 3u);
  this->lpD3DDev->SetRenderState(D3DRS_FOGSTART, *(DWORD*)&f_start);
  this->lpD3DDev->SetRenderState(D3DRS_FOGEND, LODWORD(f_end));
  this->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 1u);

  // Vertex-shader-driven fog override. standard.hlsl / terrain.hlsl /
  // ambientlit.hlsl all output VS oFog computed by GetFog(viewPos) =
  // length(viewPos) * lcb.FogParams.x + lcb.FogParams.y, where
  // lcb.FogParams sits at constant register c20 and is normally set
  // from the main map fog by SetLightCB. When that's writing main-fog
  // values, the selection FOGCOLOR set above only blends at distances
  // where main fog already kicked in -- close-up gets near-zero blend
  // and the tint is invisible. Override c20 with the selection start
  // and end so the shader-side blend matches the FFP setup.
  float fogRange = f_start - f_end;
  float vsFogParams[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  if (fogRange != 0.0f) {
    vsFogParams[0] = 1.0f / fogRange;
    vsFogParams[1] = -(f_end / fogRange);
  }
  this->lpD3DDev->SetVertexShaderConstantF(20, vsFogParams, 1);
}

// Depth-independent variant: writes c20 such that oFog is constant across
// all vertices (oFog = 1 - intensity), producing a uniform tint of `color`
// at fraction `intensity`. Used for the Selection==2 hover wash so it looks
// the same on near and far doodads. FFP fog is configured to a degenerate
// range so any non-shader path falls back to no-fog rather than a gradient.
void SGepard::SelectionFogFlat(unsigned int color, float intensity)
{
  if (intensity < 0.0f) intensity = 0.0f;
  if (intensity > 1.0f) intensity = 1.0f;
  float oFogConst = 1.0f - intensity;

  this->lpD3DDev->SetRenderState(D3DRS_FOGCOLOR, color);
  this->lpD3DDev->SetRenderState(D3DRS_FOGVERTEXMODE, 3u);  // D3DFOG_LINEAR
  this->lpD3DDev->SetRenderState(D3DRS_RANGEFOGENABLE, 1u);
  // Degenerate FFP range — only the VS-side oFog override matters here, but
  // make the FFP path produce a similar flat factor by collapsing the range
  // around 0 so (FOGEND - dist) / (FOGEND - FOGSTART) clamps to 1 (no fog
  // applied via FFP) for typical positive distances.
  float f_start = -1.0f, f_end = 0.0f;
  this->lpD3DDev->SetRenderState(D3DRS_FOGSTART, *(DWORD*)&f_start);
  this->lpD3DDev->SetRenderState(D3DRS_FOGEND,   *(DWORD*)&f_end);
  this->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 1u);

  // VS oFog = length(viewPos) * c20.x + c20.y. Flat: c20.x = 0.
  float vsFogParams[4] = { 0.0f, oFogConst, 0.0f, 0.0f };
  this->lpD3DDev->SetVertexShaderConstantF(20, vsFogParams, 1);
}

//----- (00442AE0) --------------------------------------------------------

void SGepard::SendToBack()

{
  SShaderInfo *SelectedShader; // edx
  SShaderInfo *next; // eax
  SShaderInfo *prev; // esi
  SelectedShader = this->SelectedShader;
  if ( SelectedShader )
  {
    next = SelectedShader->next;
    prev = SelectedShader->prev;
    if ( SelectedShader->next )
      next->prev = prev;
    if ( prev )
      prev->next = next;
    if ( !next )
      this->ShaderInfos.last = prev;
    if ( prev )
      next = this->ShaderInfos.first;
    else
      this->ShaderInfos.first = next;
    if ( next )
    {
      next->prev = SelectedShader;
      SelectedShader->prev = 0;
      SelectedShader->next = this->ShaderInfos.first;
    }
    else
    {
      this->ShaderInfos.last = SelectedShader;
    }
    this->ShaderInfos.first = SelectedShader;
  }
}

// SMaterial::SetAmbient defined inline in terrain.h

//----- (00442B80) --------------------------------------------------------

void SGepard::SetAmbientColor(unsigned int ambient)

{
  HRESULT v2;
  const char *v3; // eax
  char atmstr[200];
  v2 = this->lpD3DDev->SetRenderState(D3DRS_AMBIENT, ambient);
  if ( v2 )
  {
    v3 = DXGetErrorStringA(v2);
    sprintf(atmstr, "%s: %s", "SGepard::SetAmbientColor: IDirect3DDevice7::SetRenderState (D3DRS_AMBIENT)", v3);
    Logger.g->Panic(atmstr);
  }
}

//----- (00442C00) --------------------------------------------------------

void SGepard::SetAmbientGray(int ambient)

{
  unsigned int v2;
  HRESULT v3;
  const char *v4; // eax
  char atmstr[200];
  if ( ambient >= 0 )
  {
    v2 = 0xFFFFFF;
    if ( ambient <= 255 )
      v2 = 65793 * ambient;
  }
  else
  {
    v2 = 0;
  }
  v3 = this->lpD3DDev->SetRenderState(D3DRS_AMBIENT, v2);
  if ( v3 )
  {
    v4 = DXGetErrorStringA(v3);
    sprintf(atmstr, "%s: %s", "SGepard::SetAmbientColor: IDirect3DDevice7::SetRenderState (D3DRS_AMBIENT)", v4);
    Logger.g->Panic(atmstr);
  }
}

//----- (00442C90) --------------------------------------------------------

void SGepard::SetAmbientLight(float r, float g, float b)

{
  this->AmbientColorVal.r = r;
  this->AmbientColorVal.g = g;
  this->AmbientColorVal.b = b;
  this->DefAmbient = (int)(float)(b * 255.0) | (((int)(float)(g * 255.0) | ((int)(float)(r * 255.0) << 8)) << 8);
}

//----- (00442CF0) --------------------------------------------------------

void SGepard::SetBoardVisibility(bool b)

{
  this->BoardVisible = b;
}

//----- (00442D00) --------------------------------------------------------

void SGepard::SetBottomAmbientLight(float r, float g, float b)

{
  this->BottomAmbientColorVal.r = r;
  this->BottomAmbientColorVal.g = g;
  this->BottomAmbientColorVal.b = b;
}

//----- (00442DD0) --------------------------------------------------------

void SGepard::SetDebugMode(int mode)

{
  this->ClearShaders();
  this->DebugMode = mode;
  this->InitShaders();
}

// SMaterial::SetDiffuse defined inline in terrain.h

//----- (00442E20) --------------------------------------------------------

void SGepard::SetDrawType(SDrawType drawtype)

{
  this->DrawType = drawtype;
}

//----- (00442E30) --------------------------------------------------------

void SGepard::SetDynamicShaderPosition(int idx, float x, float z)

{
  SHeap<SDynamicShaderProp>::__Tstruct *array; // eax
  if ( idx >= 0 && idx < this->DynamicShaders.size )
  {
    array = this->DynamicShaders.array;
    if ( array[idx].use == 0x7FFFFFFF )
    {
      array[idx].data.X = x;
      this->DynamicShaders.array[idx].data.Z = z;
    }
  }
}

// SMaterial::SetEmissive defined inline in terrain.h

//----- (00442EB0) --------------------------------------------------------

void SGepard::SetEnvironmentLight()

{
  float v1; // xmm0_4
  float v2; // xmm0_4
  float v3; // xmm0_4
  v1 = this->SunColorVal.r + this->AmbientColorVal.r;
  this->EnvironmentColorVal.r = v1;
  if ( v1 > 1.0 )
    this->EnvironmentColorVal.r = 1.0;
  v2 = this->SunColorVal.g + this->AmbientColorVal.g;
  this->EnvironmentColorVal.g = v2;
  if ( v2 > 1.0 )
    this->EnvironmentColorVal.g = 1.0;
  v3 = this->SunColorVal.b + this->AmbientColorVal.b;
  this->EnvironmentColorVal.b = v3;
  if ( v3 > 1.0 )
    this->EnvironmentColorVal.b = 1.0;
}

//----- (00442F30) --------------------------------------------------------

void SGepard::SetFlag(GepardFlags flag, bool enable)

{
  this->Flags[flag] = enable;
  if ( flag == 0 )
  {
    this->Terrain->UpdateColorMap(0, 0, this->Terrain->XSize, this->Terrain->ZSize);
    this->Terrain->Update(0, 0, this->Terrain->XSize, this->Terrain->ZSize);
    if ( !enable && this->ShadowQuality > 1 )
      this->SetShadowQuality(1);
  }
}

//----- (00442FA0) --------------------------------------------------------

void SGepard::SetFog(float r, float g, float b, float start, float end)

{
  this->FogColorVal.r = r;
  this->FogColorVal.g = g;
  this->FogColorVal.b = b;
  this->FogStart2 = start * start;
  this->FogEnd = end;
  this->FogStart = start;
  this->FogEnd2 = end * end;
  this->FogMultiplier = 1.0f / (float)(end - start);
  this->FogColor = (int)(float)(b * 255.0) | (((int)(float)(g * 255.0) | ((int)(float)(r * 255.0) << 8)) << 8);
  this->FogMultiplier256 = 255.0f / (float)(end - start);
}

//----- (00443070) --------------------------------------------------------

void SGepard::SetFullBright(bool fullbright)

{
  this->FullBright = fullbright;
}

//----- (00443080) --------------------------------------------------------

void SGepard::SetInterpolation(long double interpolation)

{
  this->Interpolation = interpolation;
}

//----- (004430A0) --------------------------------------------------------

void SGepard::SetLightCB()

{
  this->lpD3DDev->SetVertexShaderConstantF(
        16u,
    (const float *)&this->lightCB,
    this->lightCB.Vector4fCount);
}

//----- (004430D0) --------------------------------------------------------

void SGepard::SetLightPosition(unsigned int handle, float posx, float posy, float posz)

{
  _D3DLIGHT9 *p_d3dlight;
  p_d3dlight = &this->LightProps[handle].d3dlight;
  p_d3dlight->Position.x = posx;
  p_d3dlight->Position.y = posy;
  p_d3dlight->Position.z = posz;
  this->lpD3DDev->SetLight(handle, p_d3dlight);
}

//----- (00443120) --------------------------------------------------------

void SGepard::SetLightingType(SLightingType lighttype)

{
  signed int v3;
  SLightProp *v4; // edi
  HRESULT v5;
  signed int v6;
  SLightProp *LightProps; // edi
  const char *v8; // eax
  char _Buffer[200];
  if ( lighttype )
  {
    if ( lighttype == LT_AMBIENT )
    {
      this->lpD3DDev->SetRenderState(D3DRS_LIGHTING, 1u);
      v6 = 0;
      LightProps = this->LightProps;
      do
      {
        if ( LightProps->type )
          this->lpD3DDev->LightEnable(v6, 0);
        ++v6;
        ++LightProps;
      }
      while ( v6 < 4 );
    }
    else if ( lighttype == LT_PRELIT )
    {
      this->lpD3DDev->SetRenderState(D3DRS_LIGHTING, 0);
    }
  }
  else
  {
    this->lpD3DDev->SetRenderState(D3DRS_LIGHTING, 1u);
    v3 = 0;
    v4 = this->LightProps;
    do
    {
      if ( v4->type )
        this->lpD3DDev->LightEnable(v3, 1);
      ++v3;
      ++v4;
    }
    while ( v3 < 4 );
    if ( this->FullBright )
    {
      v5 = this->lpD3DDev->SetRenderState(D3DRS_AMBIENT, 0xFFFFFFu);
      if ( v5 )
      {
        v8 = DXGetErrorStringA(v5);
        sprintf(_Buffer, "%s: %s", "SGepard::SetAmbientColor: IDirect3DDevice7::SetRenderState (D3DRS_AMBIENT)", v8);
        Logger.g->Panic(_Buffer);
      }
    }
    else
    {
      this->SetAmbientColor(this->DefAmbient);
    }
  }
}

//----- (00443290) --------------------------------------------------------

void SGepard::SetMaterial(SMaterial *mat)

{
  HRESULT v2;
  const char *v3; // eax
  char atmstr[200];
  v2 = this->lpD3DDev->SetMaterial(mat);
  if ( v2 )
  {
    v3 = DXGetErrorStringA(v2);
    sprintf(atmstr, "%s: %s", "SGepard::ApplyMaterial:  IDirect3DDevice7::SetMaterial", v3);
    Logger.g->Panic(atmstr);
  }
}

//----- (00443300) --------------------------------------------------------

void SGepard::SetMode(int a2, bool isfullscreen, bool vsync, _D3DMULTISAMPLE_TYPE msaa, int width, int height)

{
  SBoard *Board; // ecx
  this->VSync = vsync;
  this->FullScreen = isfullscreen;
  this->MSAALevel = msaa;
  if ( isfullscreen )
  {
    this->ModeWidth = width;
    this->ModeHeight = height;
  }
  Board = this->Board;
  this->ViewWidth = width;
  this->ViewHeight = height;
  Board->ResizeFrame(0, width, height);
  this->ResetDevice(a2);
}

//----- (00443360) --------------------------------------------------------

void SGepard::SetOrthogonalProjection(float scale, float near_plane, float far_plane)

{
  this->FarPlane = far_plane;
  this->OrthoScale = scale;
  this->NearPlane = near_plane;
  memset(&this->ProjectionMatrix.m[0][1] , 0, 8);
  memset(&this->ProjectionMatrix.m[0][3] , 0, 8);
  memset(&this->ProjectionMatrix.m[1][2] , 0, 8);
  memset(&this->ProjectionMatrix.m[2][0] , 0, 8);
  memset(&this->ProjectionMatrix.m[2][3] , 0, 8);
  this->ProjectionMatrix._42 = 0.0;
  this->ProjectionMatrix._44 = 1.0;
  this->ProjectionMatrix._11 = scale;
  this->ProjectionMatrix._33 = 1.0f / (float)(far_plane - near_plane);
  this->ProjectionMatrix._22 = scale;
  this->ProjectionMatrix._43 = -((float)(1.0 / (float)(far_plane - near_plane)) * near_plane);
  this->lpD3DDev->SetTransform(D3DTS_PROJECTION, &this->ProjectionMatrix);
}

//----- (00443460) --------------------------------------------------------

void SGepard::SetPerspectiveProjection(float fov, float near_plane, float far_plane)

{
  D3DXMATRIX *p_ProjectionMatrix; // esi
  float v6; // xmm0_4
  float v7; // xmm0_4
  float v8; // xmm2_4
  float ViewHeight; // xmm0_4
  float v10;
  float far_planea;
  this->FOV = fov;
  p_ProjectionMatrix = &this->ProjectionMatrix;
  this->FarPlane = far_plane;
  this->OrthoScale = 0.0;
  this->NearPlane = near_plane;
  far_planea = far_plane / (float)(far_plane - near_plane);
  memset(&this->ProjectionMatrix, 0, sizeof(this->ProjectionMatrix));
  v6 = sqrtf((float)(this->ViewWidth * this->ViewWidth + this->ViewHeight * this->ViewHeight));
  v10 = v6;
  v7 = tanf(fov * 0.5f);
  v8 = v10 * (float)(1.0 / v7);
  p_ProjectionMatrix->_11 = v8 / (float)this->ViewWidth;
  ViewHeight = (float)this->ViewHeight;
  this->ProjectionMatrix._34 = 1.0;
  this->ProjectionMatrix._33 = far_planea;
  this->ProjectionMatrix._22 = v8 / ViewHeight;
  this->ProjectionMatrix._43 = -(far_planea * near_plane);
  this->lpD3DDev->SetTransform(D3DTS_PROJECTION, p_ProjectionMatrix);
}

//----- (004435A0) --------------------------------------------------------

void SGepard::SetRectColor(int idx, unsigned int color)

{
  SHeap<SRectProp>::__Tstruct *array; // edx
  if ( idx >= 0 && idx < this->Rects.size )
  {
    array = this->Rects.array;
    if ( array[idx].use == 0x7FFFFFFF )
      array[idx].data.Color = color;
  }
}

//----- (004435E0) --------------------------------------------------------

void SGepard::SetRectPosition(int idx, int x0, int z0, int x1, int z1)

{
  int v8;
  int v9;
  int v10;
  SMesh *Mesh; // ecx
  SMesh *v12; // eax
  SMesh *v13; // eax
  SMesh *v14; // ecx
  int x0a;
  int x1a;
  int z1a;
  if ( idx >= 0 && idx < this->Rects.size && this->Rects.array[idx].use == 0x7FFFFFFF )
  {
    v8 = x1;
    v9 = x0;
    if ( x0 <= x1 )
    {
      v9 = x1;
      v8 = x0;
    }
    z1a = v9;
    v10 = z0;
    x1a = v8;
    if ( z0 <= z1 )
    {
      v10 = z1;
      z1 = z0;
    }
    x0a = v10;
    this->Rects.array[idx].data.X0 = v8;
    this->Rects.array[idx].data.Z0 = z1;
    this->Rects.array[idx].data.X1 = v9;
    this->Rects.array[idx].data.Z1 = v10;
    Mesh = this->Rects.array[idx].data.Mesh;
    if ( Mesh )
    {
      Mesh->Release();
      this->Rects.array[idx].data.Mesh = 0;
    }
    v12 = (SMesh *)operator new(sizeof(SMesh));
    if ( v12 )
    {
      v13 = new (v12) SMesh(this, 0);
      v14 = v13;
    }
    else
    {
      v14 = 0;
    }
    this->Rects.array[idx].data.Mesh = v14;
    (this->Rects.array[idx].data.Mesh)->CreateVertexBuffer(2 * (x0a + z1a - z1 - x1a) + 1);
    this->RefreshRect(this->Rects.array[idx].data.Mesh, x1a, z1, z1a, x0a);
    (this->Rects.array[idx].data.Mesh)->UnlockVertexBuffer();
  }
}

//----- (00443750) --------------------------------------------------------

void SGepard::SetResolution(int w, int h)

{
  this->ViewWidth = w;
  this->ViewHeight = h;
}

//----- (00443770) --------------------------------------------------------

void SGepard::SetResolutionScale(int a2, float scale)

{
  if ( this->ResolutionScale != scale )
  {
    this->ResolutionScale = scale;
    this->ResetDevice(a2);
  }
}

//----- (004437A0) --------------------------------------------------------

void SGepard::SetRiverSize(SHillRing *hillring, float size)

{
  if ( hillring )
    hillring->RiverSize = size;
}

//----- (004437C0) --------------------------------------------------------

void SGepard::SetSSAA(bool enabled)

{
  IDirect3DDevice9 *lpD3DDev; // ecx
  int v3;
  if ( this->AdapterID.VendorId == 4318 )
  {
    lpD3DDev = this->lpD3DDev;
    v3 = 0;
    if ( enabled )
      v3 = 1094800211;
    lpD3DDev->SetRenderState(D3DRS_ADAPTIVETESS_Y, v3);
  }
}

//----- (00443800) --------------------------------------------------------

void SGepard::SetSelectedShader(SShaderInfo *_si)

{
  this->SelectedShader = _si;
}

//----- (00443810) --------------------------------------------------------

void SGepard::SetShadowCB()

{
  this->lpD3DDev->SetVertexShaderConstantF(
        12u,
    (const float *)&this->shadowCB,
    this->shadowCB.Vector4fCount);
}

//----- (00443840) --------------------------------------------------------

void SGepard::SetShadowDistance(float dist)

{
  this->ShadowDistance = dist;
}

//----- (00443860) --------------------------------------------------------

void SGepard::SetShadowMapTexture(unsigned int stage)

{
  IDirect3DTexture9 *Ptr; // edx
  Ptr = this->ShadowMapDepthTexture.Ptr;
  if ( !Ptr )
    Ptr = this->ShadowMapTexture.Ptr;
  this->lpD3DDev->SetTexture(stage, Ptr);
  this->lpD3DDev->SetSamplerState(stage, D3DSAMP_ADDRESSU, 4u);
  this->lpD3DDev->SetSamplerState(stage, D3DSAMP_ADDRESSV, 4u);
  this->lpD3DDev->SetSamplerState(stage, D3DSAMP_BORDERCOLOR, 0xFFFFFFFF);
  this->lpD3DDev->SetSamplerState(stage, D3DSAMP_MAGFILTER, 2u);
  this->lpD3DDev->SetSamplerState(stage, D3DSAMP_MINFILTER, 2u);
  this->lpD3DDev->SetSamplerState(stage, D3DSAMP_MIPFILTER, 2u);
}

//----- (00443910) --------------------------------------------------------

void SGepard::SetShadowQuality(int q)

{
  int v3;
  IDirect3DTexture9 *Ptr; // ecx
  IDirect3DTexture9 *v5; // ecx
  int v6;
  v3 = q;
  if ( q > this->GetMaxShadowQuality() )
    v3 = this->GetMaxShadowQuality();
  if ( v3 != this->ShadowQuality )
  {
    Ptr = this->ShadowMapTexture.Ptr;
    if ( Ptr )
    {
      Ptr->Release();
      this->ShadowMapTexture.Ptr = 0;
    }
    v5 = this->ShadowMapDepthTexture.Ptr;
    if ( v5 )
    {
      v5->Release();
      this->ShadowMapDepthTexture.Ptr = 0;
    }
    this->ClearShaders();
    this->ShadowQuality = v3;
    v6 = 2048;
    if ( v3 > 2 )
      v6 = 4096;
    this->ShadowMapWidth = v6;
    this->InitShaders();
    if ( !v3 )
      this->ClearObjectShadows(0, 1);
  }
}

// SMaterial::SetSpecular(5-arg) defined inline in terrain.h

//----- (004439F0) --------------------------------------------------------

void SGepard::SetSplineAltitude2(float _altitude, SSplineRing *_splinering)

{
  if ( _splinering )
    _splinering->spline->Altitude2 = _altitude;
}

//----- (00443A10) --------------------------------------------------------

void SGepard::SetSplineDisplay(bool kibe)

{
  this->SplineDisplay = kibe;
}

//----- (00443A20) --------------------------------------------------------

void SGepard::SetSplineSize(float _size, SSplineRing *_splinering)

{
  if ( _splinering )
    _splinering->spline->Size = _size;
}

//----- (00443A40) --------------------------------------------------------

void SGepard::SetSplineVisibility(SSplineRing *_splinering, bool _visibilitystate)

{
  if ( _splinering )
    _splinering->spline->VisibilityState = _visibilitystate;
}

//----- (00443A60) --------------------------------------------------------

void SGepard::SetSunLight(int a2, int a3, float r, float g, float b, float direction, float elevation)

{
  int SunLight;
  _D3DLIGHT9 *p_d3dlight; // esi
  float v11; // xmm0_4
  float v12; // xmm1_4
  float v13; // xmm0_4
  float v14; // xmm1_4
  bool v15; // al
  float ra;
  float ga;
  float ba;
  SunLight = this->SunLight;
  this->SunColorVal.r = r;
  this->SunColorVal.g = g;
  this->SunColorVal.b = b;
  p_d3dlight = &this->LightProps[SunLight].d3dlight;
  p_d3dlight->Diffuse = this->SunColorVal;
  v11 = sinf(elevation);
  ra = v11;
  v12 = cosf(direction);
  ga = v12;
  this->SunDir.x = v12 * ra;
  v13 = sinf(elevation);
  ba = v13;
  this->SunDir.y = v13;
  v14 = cosf(elevation);
  this->SunDir.z = v14 * ga;
  this->SunProjection._43 = 0.0;
  this->SunProjection._42 = 0.0;
  this->SunProjection._41 = 0.0;
  this->SunProjection._34 = 0.0;
  this->SunProjection._32 = 0.0;
  this->SunProjection._31 = 0.0;
  this->SunProjection._24 = 0.0;
  this->SunProjection._23 = 0.0;
  this->SunProjection._21 = 0.0;
  this->SunProjection._14 = 0.0;
  this->SunProjection._13 = 0.0;
  this->SunProjection._12 = 0.0;
  this->SunProjection._44 = 1.0;
  this->SunProjection._33 = 1.0;
  this->SunProjection._22 = 1.0;
  this->SunProjection._11 = v14;
  this->SunProjection._12 = -ra;
  this->SunProjection._13 = 0.0;
  this->SunProjection._31 = -ra;
  this->SunProjection.m[2][1] = -v14; this->SunProjection.m[2][2] = 0.0f;
  this->SunProjection._21 = 0.0;
  this->SunProjection._22 = ga / ba;
  this->SunProjection._23 = this->SunDir.y;
  p_d3dlight->Direction = (_D3DVECTOR)this->SunDir;
  this->lpD3DDev->SetLight(this->SunLight, p_d3dlight);
  this->lpD3DDev->LightEnable(this->SunLight, 1);
  v15 = this->LastSunDirection == direction && this->LastSunElevation == elevation;
  this->ClearObjectShadows(v15, 0);
  this->LastSunDirection = direction;
  this->LastSunElevation = elevation;
}

//----- (00443CD0) --------------------------------------------------------

void SGepard::SetTessFolyoAlpha(SVertDiff *folyo, int idx)

{
  int v3;
  double Height; // st7
  int v5;
  float aa;
  float a;
  v3 = idx;
  aa = folyo[idx].y;
  Height = this->Terrain->GetHeight(folyo[v3].x, folyo[v3].z);
  v5 = 0;
  a = (float)((aa - Height) * 100.0f);

  if ( (int)a >= 0 )
    v5 = (int)a;
  if ( v5 > 160 )
    v5 = 160;
  folyo[v3].diff = (v5 << 24) + 0xFFFFFF;
}

//----- (00443D50) --------------------------------------------------------

void SGepard::SetTexture(unsigned int stage, int idx, bool setblend)

{
  bool v5; // dl
  IDirect3DDevice9 *lpD3DDev; // ecx
  int Alpha;
  SDrawType DrawType;
  v5 = idx >= 0 && idx < this->Textures.size && this->Textures.array[idx].use == 0x7FFFFFFF;
  lpD3DDev = this->lpD3DDev;
  if ( v5 )
  {
    lpD3DDev->SetTexture(stage, this->Textures.array[idx].data.lpTexture);
    Alpha = this->Textures.array[idx].data.Alpha;
  }
  else
  {
    lpD3DDev->SetTexture(stage, 0);
    Alpha = 0;
  }
  if ( setblend )
  {
    DrawType = this->DrawType;
    if ( DrawType == DT_ADD )
    {
      this->lpD3DDev->SetRenderState(D3DRS_SRCBLEND, 2u);
      this->lpD3DDev->SetRenderState(D3DRS_DESTBLEND, 2u);
      this->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
      this->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
      this->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    }
    else if ( DrawType == DT_SHADOW )
    {
      this->lpD3DDev->SetRenderState(D3DRS_SRCBLEND, 1u);
      this->lpD3DDev->SetRenderState(D3DRS_DESTBLEND, 4u);
      this->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
      this->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
      this->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    }
    else if ( Alpha == 2 )
    {
      this->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
      this->lpD3DDev->SetRenderState(D3DRS_SRCBLEND, 5u);
      this->lpD3DDev->SetRenderState(D3DRS_DESTBLEND, 6u);
      this->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 1u);
      this->lpD3DDev->SetRenderState(D3DRS_ALPHAREF, 5u);
      this->lpD3DDev->SetRenderState(D3DRS_ALPHAFUNC, 5u);
      this->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    }
    else
    {
      if ( Alpha == 4 )
      {
        this->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
        this->lpD3DDev->SetRenderState(D3DRS_SRCBLEND, 2u);
        this->lpD3DDev->SetRenderState(D3DRS_DESTBLEND, 6u);
        this->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
        this->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
      }
      else
      {
        if ( Alpha == 1 )
        {
          this->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
          this->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 1u);
          this->lpD3DDev->SetRenderState(D3DRS_ALPHAREF, 128u);
          this->lpD3DDev->SetRenderState(D3DRS_ALPHAFUNC, 5u);
        }
        else
        {
          this->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
          this->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
        }
        this->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 1u);
      }
    }
  }
}

//----- (00444060) --------------------------------------------------------

void SGepard::SetTextureDetail(int detail)

{
  int v3;
  int size;
  SHeap<STextureProp>::__Tstruct *v5; // eax
  STerrain *Terrain; // ecx
  SHeap<STextureProp>::__Tstruct *v7; // eax
  int v8;
  if ( this->TextureDetail != detail )
  {
    this->TextureDetail = detail;
    v3 = -1;
    while ( 1 )
    {
      size = this->Textures.size;
      if ( ++v3 >= size )
        break;
      v5 = &this->Textures.array[v3];
      while ( v5->use != 0x7FFFFFFF )
      {
        ++v3;
        ++v5;
        if ( v3 >= size )
          goto LABEL_7;
      }
      if ( v3 < 0 )
        break;
      v7 = &this->Textures.array[v3];
      if ( v7->data.MipMaps )
      {
        v8 = strcmp(v7->data.FileName, "TerrainLayer");
        if ( v8 )
          v8 = v8 < 0 ? -1 : 1;
        if ( v8 )
          this->ReloadTexture(v3);
      }
    }
LABEL_7:
    Terrain = this->Terrain;
    if ( Terrain )
    {
      Terrain->ReleaseTileset();
      this->Terrain->InitTileset2();
      if ( this->Terrain->GetCompactMode() )
      {
        this->Terrain->SetCompactMode(0);
        this->Terrain->SetCompactMode(1);
      }
    }
  }
}

//----- (00444160) --------------------------------------------------------

void SGepard::SetTextureFilter(int filter)

{
  unsigned int v3;
  unsigned int v4;
  v3 = (filter & 2 | 4u) >> 1;
  v4 = (filter & 1) + 1;
  this->TextureFilter = filter;
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_MAGFILTER, v3);
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_MINFILTER, v3);
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_MIPFILTER, v4);
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_MAXANISOTROPY, 16u);
  this->lpD3DDev->SetSamplerState(1u, D3DSAMP_MAGFILTER, v3);
  this->lpD3DDev->SetSamplerState(1u, D3DSAMP_MINFILTER, v3);
  this->lpD3DDev->SetSamplerState(1u, D3DSAMP_MIPFILTER, v4);
  this->lpD3DDev->SetSamplerState(1u, D3DSAMP_MAXANISOTROPY, 16u);
}

//----- (00444230) --------------------------------------------------------

void SGepard::SetTileset(int tileset)

{
  this->Tileset = tileset;
  this->FirstDrawWater = 1;
}

//----- (00444250) --------------------------------------------------------

void SGepard::SetTopSplineAltitude2(SHillRing *_hillring, float _altitude)

{
  SChain<SSplineRing> *SplineChain; // eax
  SSplineRing *last; // ecx
  SplineChain = _hillring->SplineChain;
  if ( SplineChain )
  {
    if ( SplineChain->Closed && !SplineChain->last )
      Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
    last = SplineChain->last;
    if ( last )
      last->spline->SetAltitude2(_altitude);
  }
}

//----- (004442A0) --------------------------------------------------------

void SGepard::SetTopSplineAltitude(SHillRing *_hillring, float _altitude)

{
  SChain<SSplineRing> *SplineChain; // eax
  SSplineRing *last; // ecx
  SplineChain = _hillring->SplineChain;
  if ( SplineChain )
  {
    if ( SplineChain->Closed && !SplineChain->last )
      Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
    last = SplineChain->last;
    if ( last )
      last->spline->SetAltitude(_altitude);
  }
}

//----- (004442F0) --------------------------------------------------------

void SGepard::SetUpObjectEffects()

{
  int Effect;
  SEffect *v3; // ecx
  int v4;
  SEffect *v5; // ecx
  int v6;
  int size;
  SHeap<SObjectProp>::__Tstruct *i; // eax
  SGroup *Group; // ecx
  int v10;
  int v11;
  SGroup *v12; // ecx
  int v13;
  int v14;
  SGroup *v15; // ecx
  SGroup *v16; // ecx
  SGroup *v17; // ecx
  char *_randomx;
  char *_randomxa;
  char *_randomxb;
  int x;
  float y;
  int z;
  if ( !this->Effect )
    return;
  Effect = this->Effect->LoadEffect("haz_fust", 0, -1.0, -1.0);
  v3 = this->Effect;
  this->_haz_fust = Effect;
  v4 = v3->LoadEffect("haz_fust2", 0, -1.0, -1.0);
  v5 = this->Effect;
  this->_haz_fust2 = v4;
  this->_haz_fust3 = v5->LoadEffect("haz_fust3", 0, -1.0, -1.0);
  v6 = -1;
  while ( 1 )
  {
    size = this->StaticObjects.size;
    if ( ++v6 >= size )
      break;
    for ( i = &this->StaticObjects.array[v6]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v6 >= size )
        return;
    }
    if ( v6 < 0 )
      break;
    Group = this->StaticObjects.array[v6].data.Group;
    if ( !Group )
      continue;
    if ( Group->GetMeshIndex("ps") == -1 )
    {
      v12 = this->StaticObjects.array[v6].data.Group;
      if ( v12->GetMeshIndex("_ps") == -1 )
        goto LABEL_25;
      v13 = (int)(float)((float)((float)rand() * 0.000030517578) * 3.0f);
      if ( !v13 )
      {
        _randomxb = "_ps";
        goto LABEL_24;
      }
      v14 = v13 - 1;
      if ( !v14 )
      {
        _randomxa = "_ps";
        goto LABEL_15;
      }
      if ( v14 != 1 )
        goto LABEL_25;
      _randomx = "_ps";
      goto LABEL_13;
    }
    v10 = (int)(float)((float)((float)rand() * 0.000030517578) * 3.0f);
    if ( !v10 )
    {
      _randomxb = "ps";
LABEL_24:
      this->Effect->PlayEffect(this->_haz_fust, this->StaticObjects.array[v6].data.Group, _randomxb, 0);
      goto LABEL_25;
    }
    v11 = v10 - 1;
    if ( !v11 )
    {
      _randomxa = "ps";
LABEL_15:
      this->Effect->PlayEffect(this->_haz_fust2, this->StaticObjects.array[v6].data.Group, _randomxa, 0);
      goto LABEL_25;
    }
    if ( v11 == 1 )
    {
      _randomx = "ps";
LABEL_13:
      this->Effect->PlayEffect(this->_haz_fust3, this->StaticObjects.array[v6].data.Group, _randomx, 0);
    }
LABEL_25:
    v15 = this->StaticObjects.array[v6].data.Group;
    if ( v15 && v15->GetMeshIndex("Ordogszeker") != -1 )
    {
      v16 = this->StaticObjects.array[v6].data.Group;
      v16->GetPosition((float *)&x, &y, (float *)&z);
      v17 = this->StaticObjects.array[v6].data.Group;
      v17->SetPosition((float)x, (float)(y + 0.2f), (float)z);
    }
  }
}

//----- (00444540) --------------------------------------------------------

void SGepard::SetUpPresentation()

{
  bool v2;
  unsigned int v3;
  int v4;
  D3DFORMAT Format;
  float ResolutionScale; // xmm0_4
  unsigned int v7;
  _D3DMULTISAMPLE_TYPE v8;
  _D3DMULTISAMPLE_TYPE MSAALevel;
  this->PresentationParameters.BackBufferWidth = 0;
  this->PresentationParameters.BackBufferHeight = 0;
  this->PresentationParameters.BackBufferCount = 0;
  this->PresentationParameters.MultiSampleType = D3DMULTISAMPLE_NONE;
  this->PresentationParameters.MultiSampleQuality = 0;
  this->PresentationParameters.hDeviceWindow = 0;
  this->PresentationParameters.Windowed = 0;
  this->PresentationParameters.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
  this->PresentationParameters.Flags = 0;
  this->PresentationParameters.FullScreen_RefreshRateInHz = 0;
  if ( this->FullScreen )
  {
    v2 = !this->VSync;
    this->PresentationParameters.BackBufferWidth = this->ModeWidth;
    this->PresentationParameters.BackBufferHeight = this->ModeHeight;
    this->AdapterFormat = D3DFMT_X8R8G8B8;
    this->PresentationParameters.BackBufferFormat = D3DFMT_X8R8G8B8;
    if ( v2 || (v3 = 3, this->ResolutionScale != 1.0) )
      v3 = 2;
    this->PresentationParameters.BackBufferCount = v3;
    v4 = 0;
  }
  else
  {
    Format = this->d3ddm.Format;
    this->AdapterFormat = Format;
    this->PresentationParameters.BackBufferFormat = Format;
    v4 = 1;
  }
  this->PresentationParameters.Windowed = v4;
  ResolutionScale = this->ResolutionScale;
  this->PresentationParameters.SwapEffect = D3DSWAPEFFECT_DISCARD;
  v2 = !this->VSync;
  this->PresentationParameters.EnableAutoDepthStencil = ResolutionScale == 1.0;
  v7 = 0x80000000;
  if ( !v2 )
    v7 = 1;
  this->PresentationParameters.PresentationInterval = v7;
  if ( ResolutionScale != 1.0
    || (v8 = this->MSAALevel, v8 < D3DMULTISAMPLE_2_SAMPLES)
    || this->lpD3D->CheckDeviceMultiSampleType(
                  this->Adapter,
         this->DeviceType,
         this->AdapterFormat,
         this->PresentationParameters.Windowed,
         v8,
         0) )
  {
    MSAALevel = D3DMULTISAMPLE_NONE;
  }
  else
  {
    MSAALevel = this->MSAALevel;
  }
  this->PresentationParameters.MultiSampleType = MSAALevel;
  if ( this->lpD3D->CheckDeviceFormat(
                  this->Adapter,
         this->DeviceType,
         this->AdapterFormat,
         2u,
         D3DRTYPE_SURFACE,
         D3DFMT_D24S8) )
  {
    Logger.g->Panic("SGepard::SetUpPresentation: No available depth-stencil format were found");
  }
  this->PresentationParameters.AutoDepthStencilFormat = D3DFMT_D24S8;
}

//----- (00444710) --------------------------------------------------------

void SGepard::SetViewProperties(float xpos, float ypos, float zpos, float hrot, float vrot)

{
  D3DXMATRIX *p_CameraMatrix; // esi
  D3DXMATRIX temp_mx;
  p_CameraMatrix = &this->CameraMatrix;
  this->CameraHRot = hrot;
  this->CameraVRot = vrot;
  this->CameraZPos = zpos;
  this->CameraYPos = ypos;
  this->CameraXPos = xpos;
  memset(&this->CameraMatrix.m[3][1] , 0, 8);
  memset(&this->CameraMatrix.m[2][3] , 0, 8);
  memset(&this->CameraMatrix.m[2][0] , 0, 8);
  memset(&this->CameraMatrix.m[1][2] , 0, 8);
  memset(&this->CameraMatrix.m[0][3] , 0, 8);
  memset(&this->CameraMatrix.m[0][1] , 0, 8);
  this->CameraMatrix._44 = 1.0;
  this->CameraMatrix._33 = 1.0;
  this->CameraMatrix._22 = 1.0;
  this->CameraMatrix._11 = 1.0;
  D3DXMatrixTranslation(&this->CameraMatrix, -xpos, -ypos, -zpos);
  D3DXMatrixRotationY(&temp_mx, -hrot);
  D3DXMatrixMultiply(p_CameraMatrix, p_CameraMatrix, &temp_mx);
  // NOTE: IDA original has LODWORD(vrot) ^ _xmm (negation) here, but DO NOT add -vrot.
  // SetViewProperties is called by RenderShadowMap with sun angles — negating vrot
  // breaks shadow map projection (dark diagonal overlay). The missing negation here
  // is compensated by the call site in world.cpp also omitting its negation.
  D3DXMatrixRotationX(&temp_mx, vrot);
  D3DXMatrixMultiply(p_CameraMatrix, p_CameraMatrix, &temp_mx);
  this->lpD3DDev->SetTransform(D3DTS_VIEW, p_CameraMatrix);
}

//----- (00444880) --------------------------------------------------------

struct SWorldViewProjCB { float WorldMatrix[4][4]; float ViewMatrix[4][4]; float ProjMatrix[4][4]; int Vector4fCount; };

void SGepard::SetWorldViewProjVertexShaderConstantBuffer(D3DXMATRIX *worldMatrix)

{
  IDirect3DDevice9 *lpD3DDev;
  SWorldViewProjCB cb;
  cb.Vector4fCount = 12;
  memcpy(&cb.WorldMatrix[0][0] , &worldMatrix->_11, 16);
  memcpy(&cb.WorldMatrix[1][0] , &worldMatrix->m[1][0], 16);
  memcpy(&cb.WorldMatrix[2][0] , &worldMatrix->m[2][0], 16);
  lpD3DDev = this->lpD3DDev;
  memcpy(&cb.WorldMatrix[3][0] , &worldMatrix->m[3][0], 16);
  *(D3DXMATRIX *)&cb.ViewMatrix[0][0] = this->CameraMatrix;
  *(D3DXMATRIX *)&cb.ProjMatrix[0][0] = this->ProjectionMatrix;
  lpD3DDev->SetVertexShaderConstantF(0, (const float *)&cb, 12u);
}

//----- (00444950) --------------------------------------------------------

void SGepard::SkipObjectShadowMask(SIObject *object, SStream *is)

{
  int v3;
  v3 = is->ReadInt() - 1;
  if ( v3 )
  {
    if ( v3 == 1 )
    {
      is->ReadFloat();
      is->ReadFloat();
      is->ReadFloat();
      is->ReadFloat();
      { SShadowMask2 *_m2 = new SShadowMask2(is); delete _m2; }
    }
  }
  else
  {
    is->ReadFloat();
    is->ReadFloat();
    is->ReadFloat();
    is->ReadFloat();
    { SShadowMask *_m1 = new SShadowMask(is); delete _m1; }
  }
}

//----- (004449F0) --------------------------------------------------------

void SGepard::SortSplines(SHillRing *_hillring)

{
  SHillRing *v2; // esi
  int v3;
  int v4;
  int v5;
  int v6;
  SSplineRing *first; // esi
  SSplineRing *v8; // eax
  int v9;
  float Altitude2; // xmm0_4
  SSplineRing *v11; // eax
  int v12;
  SSplineRing *v13; // edx
  int v14;
  int v15;
  SSplineRing **p_prev; // eax
  SSplineRing *next; // eax
  SSplineRing *v18; // ecx
  SSplineRing *prev; // eax
  SSplineRing *v20; // ecx
  SSplineRing *v21; // ecx
  int v22;
  int v23;
  v2 = _hillring;
  if ( !_hillring )
    return;
  v3 = 2;
  v23 = 2;
  v4 = _hillring->SplineChain->NumItems - 1;
  v22 = v4;
  if ( v4 <= 2 )
    return;
  v5 = 1;
  do
  {
    v6 = v3;
    if ( v3 >= v4 )
      goto LABEL_28;
    do
    {
      first = v2->SplineChain->first;
      v8 = first;
      if ( !first )
        Logger.g->Panic("GetPointer: SChain is empty!");
      v9 = 0;
      if ( v5 > 0 )
      {
        while ( 1 )
        {
          v8 = v8->next;
          if ( !v8 )
            break;
          if ( ++v9 >= v5 )
            goto LABEL_9;
        }
LABEL_31:
        Logger.g->Panic("GetPointer: the given index is invalid!");
      }
LABEL_9:
      Altitude2 = v8->spline->Altitude2;
      v11 = first;
      v12 = 0;
      if ( v6 > 0 )
      {
        do
        {
          v11 = v11->next;
          if ( !v11 )
            goto LABEL_31;
        }
        while ( ++v12 < v6 );
      }
      if ( Altitude2 > v11->spline->Altitude2 )
      {
        v13 = first;
        v14 = 0;
        if ( v6 > 0 )
        {
          do
          {
            v13 = v13->next;
            if ( !v13 )
              goto LABEL_31;
          }
          while ( ++v14 < v6 );
        }
        v15 = 0;
        if ( v5 > 0 )
        {
          do
          {
            first = first->next;
            if ( !first )
              goto LABEL_31;
          }
          while ( ++v15 < v5 );
        }
        if ( first->prev )
          first->prev->next = v13;
        p_prev = &v13->next->prev;
        if ( p_prev )
          *p_prev = first;
        next = first->next;
        if ( next == v13 )
        {
          v18 = v13->next;
          prev = first->prev;
          v13->next = first;
          first->prev = v13;
          v13->prev = prev;
          first->next = v18;
        }
        else
        {
          next->prev = v13;
          v13->prev->next = first;
          v20 = first->next;
          first->next = v13->next;
          v13->next = v20;
          v21 = v13->prev;
          v13->prev = first->prev;
          first->prev = v21;
        }
      }
      v4 = v22;
      ++v6;
      v2 = _hillring;
    }
    while ( v6 < v22 );
    v3 = v23;
LABEL_28:
    ++v3;
    ++v5;
    v23 = v3;
  }
  while ( v3 < v4 );
}

//----- (00444BE0) --------------------------------------------------------

void SGepard::StopEffect(void *handle)

{
  this->Effect->StopEffect((SEffectDesc2 *)handle);
}

//----- (00444BF0) --------------------------------------------------------

void SGepard::TrackEffect(void *handle, float _x, float _y, float _z, float _borningspeed)

{
  this->Effect->TrackEffect((SEffectDesc2 *)handle, _x, _y, _z, _borningspeed);
}

//----- (00444C40) --------------------------------------------------------

// Rewritten from Ghidra output — IDA used heap byte offset (52*idx) as the
// cos/sin angle instead of the actual direction, producing garbled trail geometry.
void SGepard::TrackGroundTrail(int idx, float x, float z, float dir)
{
  if (idx < 0 || idx >= this->GroundTrails.size)
    return;

  auto *entry = &this->GroundTrails.array[idx];
  if (entry->use != 0x7FFFFFFF)
    return;

  auto &trail = entry->data;

  if (!trail.HaveFirst) {
    trail.HaveFirst = true;
    trail.LastX = x;
    trail.LastZ = z;
    trail.LastDir = dir;
    return;
  }

  float lastX = trail.LastX;
  float lastZ = trail.LastZ;
  float lastDir = trail.LastDir;

  // Only add a new segment if moved at least 0.25 units (0.0625 squared)
  float dx = lastX - x;
  float dz = lastZ - z;
  if (dx * dx + dz * dz < 0.0625f)
    return;

  // Allocate a new segment from the segments heap
  auto *segs = trail.Segments;
  int segIdx = segs->nextempty;
  ++segs->occupied;

  if (segIdx < 0) {
    // No free slot — append at end, grow if needed
    if (segs->size == segs->maxsize) {
      int newMax = (segs->maxsize >= 16) ? (segs->maxsize * 6 / 5) : 16;
      segs->array = (SHeap<SGroundTrailSegment>::__Tstruct *)realloc(
          segs->array, 60 * newMax);
      memset(&segs->array[segs->maxsize], 0, 60 * (newMax - segs->maxsize));
      segs->maxsize = newMax;
    }
    segs->array[segs->size].use = 0x7FFFFFFF;
    segIdx = segs->size;
    segs->size = segIdx + 1;
  } else {
    // Reuse a free slot from the free list
    segs->nextempty = segs->array[segIdx].use;
    segs->array[segIdx].use = 0x7FFFFFFF;
    memset(&segs->array[segIdx].data, 0, sizeof(segs->array[segIdx].data));
  }

  auto &seg = segs->array[segIdx].data;
  float uScale = trail.UScale;

  // Vertices 0 and 2: at PREVIOUS position, perpendicular using LastDir
  float cosLast = cosf(lastDir) * uScale;
  float sinLast = sinf(lastDir) * uScale;

  float v0x = lastX + cosLast;
  float v0z = lastZ - sinLast;
  seg.Vertices[0].x = v0x;
  seg.Vertices[0].z = v0z;
  seg.Vertices[0].y = (float)(this->Terrain->GetHeight(v0x, v0z) + 0.025f);


  float v2x = lastX - cosLast;
  float v2z = lastZ + sinLast;
  seg.Vertices[2].x = v2x;
  seg.Vertices[2].z = v2z;
  seg.Vertices[2].y = (float)(this->Terrain->GetHeight(v2x, v2z) + 0.025f);


  // Vertices 1 and 3: at CURRENT position, perpendicular using dir
  float cosCur = cosf(dir) * uScale;
  float sinCur = sinf(dir) * uScale;

  float v1x = x + cosCur;
  float v1z = z - sinCur;
  seg.Vertices[1].x = v1x;
  seg.Vertices[1].z = v1z;
  seg.Vertices[1].y = (float)(this->Terrain->GetHeight(v1x, v1z) + 0.025f);


  float v3x = x - cosCur;
  float v3z = z + sinCur;
  seg.Vertices[3].x = v3x;
  seg.Vertices[3].z = v3z;
  seg.Vertices[3].y = (float)(this->Terrain->GetHeight(v3x, v3z) + 0.025f);


  seg.StartTime = this->WorldTime;
  seg.Noticed = false;

  trail.LastX = x;
  trail.LastZ = z;
  trail.LastDir = dir;
}

//----- (00445040) --------------------------------------------------------

void SGepard::TrackSmokeTrail(int idx, float x, float y, float z, float alpha)

{
  // x64 fix: original took addresses of trail points via `v11 = (int)&v10[N]`
  // and `v22 = (int)&v10[N-1]` — `(int)` truncated 8-byte pointers to 4
  // bytes on x64. Every subsequent `*(float*)v11` / `*(float*)(v22+20)`
  // dereferenced the truncated address, scattering writes across arbitrary
  // heap memory. The smoke trail's intermediate interpolated points landed
  // in garbage, leaving the rendered billboard with corrupt geometry — the
  // muzzle-flash white-rectangle artifact. Use typed pointers throughout.
  if ( idx < 0 || idx >= this->SmokeTrails.size )
    return;
  if ( this->SmokeTrails.array[idx].use != 0x7FFFFFFF )
    return;
  SSmokeTrail *trail = &this->SmokeTrails.array[idx].data;
  SDArray<SSmokeTrailPoint> *Points = trail->Points;
  int size = Points->size;
  SSmokeTrailPoint *last = &Points->array[size - 1];
  SSmokeTrailPoint *prev = &Points->array[size - 2];
  float dx = x - prev->Vector.x;
  float dy = y - prev->Vector.y;
  float dz = z - prev->Vector.z;
  float dist = sqrtf(dx * dx + dy * dy + dz * dz);
  float t = 0.25f;
  if ( dist > 0.25 )
  {
    do
    {
      float ratio = t / dist;
      last->Vector.x = (x - prev->Vector.x) * ratio + prev->Vector.x;
      last->Vector.y = (y - prev->Vector.y) * ratio + prev->Vector.y;
      last->Vector.z = (z - prev->Vector.z) * ratio + prev->Vector.z;
      last->Scale = 0.5f;
      last->TextureV = trail->VScale * t + prev->TextureV;
      last->StartTime = (unsigned int)((float)(this->WorldTime - prev->StartTime) * ratio
                                       + (float)prev->StartTime);
      last->Alpha = (alpha - prev->Alpha) * ratio + prev->Alpha;
      // Add a new slot — may realloc, so re-fetch pointers via cached `size`.
      int new_idx = Points->Add();
      last = &Points->array[new_idx];
      prev = &Points->array[size - 2];   // intentionally cached size; prev is fixed across iterations
      t += 0.25f;
    } while ( dist > t );
  }
  // Final write: the very last allocated slot gets the exact target position.
  last->Vector.x = x;
  last->Vector.y = y;
  last->Vector.z = z;
  last->Scale = 0.5f;
  last->TextureV = trail->VScale * dist + prev->TextureV;
  last->StartTime = this->WorldTime;
  last->Alpha = alpha;
}

//----- (00445340) --------------------------------------------------------

void SGepard::TransformGroundPoint(int a2, float x, float z, float *s_x, float *s_y, float *s_zbuf, float *s_rhw, unsigned int *s_fog, float h_bias, float z_bias)

{
  STerrain *Terrain; // ecx
  float v13; // xmm5_4
  float v14; // xmm6_4
  float v15;
  float y; // xmm4_4
  float v17; // xmm0_4
  float v18; // xmm0_4
  float v19; // xmm0_4
  float v20;
  float FogEnd;
  D3DXVECTOR3 vec;
  Terrain = this->Terrain;
  vec.x = x;
  vec.y = (float)(Terrain->GetHeight(x, z) + h_bias);

  vec.z = z;
  D3DXVec3TransformCoord(&vec, &vec, &this->CameraMatrix);
  v13 = vec.z;
  if ( vec.z <= 0.0 )
  {
    *s_x = 0.0;
    *s_y = 0.0;
    *s_zbuf = -1.0;
    *s_rhw = -1.0;
LABEL_3:
    *s_fog = -16777216;
    return;
  }
  v14 = vec.x;
  v15 = (float)(1.0 / vec.z);
  y = vec.y;
  v17 = (float)((this->ProjectionMatrix._11 * 0.5f * vec.x * v15 + 0.5f) * (double)this->ViewWidthScaled);

  *s_x = v17;
  v18 = (float)((0.5f - this->ProjectionMatrix._22 * 0.5f * y * v15) * (double)this->ViewHeightScaled);

  *s_y = v18;
  v19 = this->ProjectionMatrix._43 * v15 + this->ProjectionMatrix._33 - z_bias * v15;
  *s_zbuf = v19;
  *s_rhw = v15;
  if ( this->FogEnd <= this->FogStart )
    goto LABEL_3;
  v20 = (float)((float)((float)(y * y) + (float)(v14 * v14)) + (float)(v13 * v13));
  if ( this->FogStart2 <= v20 )
  {
    if ( v20 < this->FogEnd2 )
    {
      FogEnd = this->FogEnd;
      *s_fog = (unsigned int)(this->FogMultiplier256 * (FogEnd - sqrtf(v20))) << 24;
    }
    else
    {
      *s_fog = 0;
    }
  }
  else
  {
    *s_fog = -16777216;
  }
}

//----- (004455B0) --------------------------------------------------------

void SGepard::TransformPoint(float x, float y, float z, float *s_x, float *s_y, float *s_z)

{
  float v7; // xmm4_4
  float v8;
  float v9; // xmm0_4
  D3DXVECTOR3 vec;
  vec.x = x;
  vec.y = y;
  vec.z = z;
  D3DXVec3TransformCoord(&vec, &vec, &this->CameraMatrix);
  v7 = vec.z;
  if ( vec.z > 0.0 )
  {
    v8 = (float)(1.0 / vec.z);
    v9 = this->ProjectionMatrix._11 * 0.5f * vec.x * v8 + 0.5f;
    *s_x = v9;
    *s_y = 0.5f - this->ProjectionMatrix._22 * 0.5f * vec.y * v8;
    *s_z = v7;
  }
  else
  {
    *s_x = -1.0;
    *s_y = -1.0;
    *s_z = 0.0;
  }
}

//----- (004456C0) --------------------------------------------------------

void SGepard::TransformPoint(float x, float y, float z, float *s_x, float *s_y)

{
  float v7; // xmm0_4
  float v8;
  float v9; // xmm0_4
  D3DXVECTOR3 vec;
  vec.x = x;
  vec.y = y;
  vec.z = z;
  D3DXVec3TransformCoord(&vec, &vec, &this->CameraMatrix);
  if ( vec.z > 0.0 )
  {
    v8 = (float)(1.0 / vec.z);
    v9 = this->ProjectionMatrix._11 * 0.5f * vec.x * v8 + 0.5f;
    *s_x = v9;
    v7 = 0.5f - this->ProjectionMatrix._22 * 0.5f * vec.y * v8;
  }
  else
  {
    v7 = (-1.0f);
    *s_x = -1.0;
  }
  *s_y = v7;
}

//----- (004457B0) --------------------------------------------------------

void SGepard::TransformScaledPointAdd(float x, float y, float z, float size, float *s_x, float *s_y, float *s_size, float *s_zbuf, unsigned int *s_fog)

{
  float v11; // xmm6_4
  float v12; // xmm7_4
  float v13;
  float v14; // xmm5_4
  float v15; // xmm0_4
  float v16; // xmm0_4
  float v17; // xmm0_4
  float v18;
  float FogEnd;
  D3DXVECTOR3 vec;
  vec.x = x;
  vec.y = y;
  vec.z = z;
  D3DXVec3TransformCoord(&vec, &vec, &this->CameraMatrix);
  v11 = vec.z;
  if ( vec.z > 0.0 )
  {
    v12 = vec.x;
    v13 = (float)(1.0 / vec.z);
    v14 = vec.y;
    v15 = (float)((this->ProjectionMatrix._11 * 0.5f * vec.x * v13 + 0.5f) * (double)this->ViewWidthScaled);

    *s_x = v15;
    v16 = (float)((0.5f - this->ProjectionMatrix._22 * 0.5f * v14 * v13) * (double)this->ViewHeightScaled);

    *s_y = v16;
    v17 = (float)((double)this->ViewWidthScaled * 0.5f * this->ProjectionMatrix._11 * size * v13);

    *s_size = v17;
    *s_zbuf = this->ProjectionMatrix._43 * v13 + this->ProjectionMatrix._33;
    if ( this->Glow )
      *s_fog = 0xFFFFFF;
    if ( this->FogEnd <= this->FogStart )
    {
      *s_fog = 0xFFFFFF;
    }
    else
    {
      v18 = (float)((float)((float)(v14 * v14) + (float)(v12 * v12)) + (float)(v11 * v11));
      if ( this->FogStart2 <= v18 )
      {
        if ( v18 < this->FogEnd2 )
        {
          FogEnd = this->FogEnd;
          *s_fog = 65793
                 * (unsigned int)(this->FogMultiplier256 * (FogEnd - sqrtf(v18)));
        }
        else
        {
          *s_fog = 0;
        }
      }
      else
      {
        *s_fog = 0xFFFFFF;
      }
    }
  }
  else
  {
    *s_x = 0.0;
    *s_y = 0.0;
    *s_size = 0.0;
  }
}

//----- (00445A40) --------------------------------------------------------

void SGepard::TransformScaledPointBlend(float x, float y, float z, float size, float *s_x, float *s_y, float *s_size, float *s_zbuf, float *s_rhw, unsigned int *s_fog)

{
  float v12; // xmm6_4
  float v13; // xmm7_4
  float v14;
  float v15; // xmm5_4
  float v16; // xmm0_4
  float v17; // xmm0_4
  float v18; // xmm0_4
  float v19; // xmm0_4
  float v20;
  float FogEnd;
  D3DXVECTOR3 vec;
  vec.x = x;
  vec.y = y;
  vec.z = z;
  D3DXVec3TransformCoord(&vec, &vec, &this->CameraMatrix);
  v12 = vec.z;
  if ( vec.z > 0.0 )
  {
    v13 = vec.x;
    v14 = (float)(1.0 / vec.z);
    v15 = vec.y;
    v16 = (float)((this->ProjectionMatrix._11 * 0.5f * vec.x * v14 + 0.5f) * (double)this->ViewWidthScaled);

    *s_x = v16;
    v17 = (float)((0.5f - this->ProjectionMatrix._22 * 0.5f * v15 * v14) * (double)this->ViewHeightScaled);

    *s_y = v17;
    v18 = (float)((double)this->ViewWidthScaled * 0.5f * this->ProjectionMatrix._11 * size * v14);

    *s_size = v18;
    v19 = this->ProjectionMatrix._43 * v14 + this->ProjectionMatrix._33;
    *s_zbuf = v19;
    *s_rhw = 1.0f / v12;
    if ( this->FogEnd <= this->FogStart )
    {
      *s_fog = -16777216;
    }
    else
    {
      v20 = (float)((float)((float)(v15 * v15) + (float)(v13 * v13)) + (float)(v12 * v12));
      if ( this->FogStart2 <= v20 )
      {
        if ( v20 < this->FogEnd2 )
        {
          FogEnd = this->FogEnd;
          *s_fog = (unsigned int)(this->FogMultiplier256 * (FogEnd - sqrtf(v20))) << 24;
        }
        else
        {
          *s_fog = 0;
        }
      }
      else
      {
        *s_fog = -16777216;
      }
    }
  }
  else
  {
    *s_x = 0.0;
    *s_y = 0.0;
    *s_size = 0.0;
  }
}

//----- (00445CE0) --------------------------------------------------------

void SGepard::TransformScaledPointUi(float x, float y, float z, float size, float *s_x, float *s_y, float *s_size)

{
  float v9; // xmm0_4
  float v10;
  float v11; // xmm0_4
  float v12; // xmm0_4
  D3DXVECTOR3 vec;
  vec.x = x;
  vec.y = y;
  vec.z = z;
  D3DXVec3TransformCoord(&vec, &vec, &this->CameraMatrix);
  v9 = 0.0;
  if ( vec.z > 0.0 )
  {
    v10 = (float)(1.0 / vec.z);
    v11 = (float)((this->ProjectionMatrix._11 * 0.5f * vec.x * v10 + 0.5f) * (double)this->ViewWidth);

    *s_x = v11;
    v12 = (float)((0.5f - this->ProjectionMatrix._22 * 0.5f * vec.y * v10) * (double)this->ViewHeight);

    *s_y = v12;
    v9 = (float)((double)this->ViewWidth * 0.5f * this->ProjectionMatrix._11 * size * v10);

  }
  else
  {
    *s_x = 0.0;
    *s_y = 0.0;
  }
  *s_size = v9;
}

//----- (00445E30) --------------------------------------------------------

void SGepard::TransformScreenToCamera(float *x, float *y)

{
  float v3; // xmm0_4
  v3 = (float)((*x - (double)this->ViewWidth * 0.5) / (double)this->ViewWidth / this->ProjectionMatrix._11);

  *x = v3;
  *y = (float)((*y - (double)this->ViewHeight * 0.5) / (double)this->ViewHeight / this->ProjectionMatrix._22);
}

//----- (00445EC0) --------------------------------------------------------

void SGepard::TriggerRehash()

{
  this->ObjectsHashed = 0;
}

//----- (00445ED0) --------------------------------------------------------

void SGepard::TurnOffGlow()

{
  if ( this->Glow )
    this->Glow = 0;
}

//----- (00445EF0) --------------------------------------------------------

void SGepard::TurnOnGlow()

{
  this->Glow = this->PlayEffect(this->_glow, 0.0f, 0.0f, 0.0f, 0.0f);
}

//----- (00445F40) --------------------------------------------------------

void SGepard::UnlockDynamicVB(int idx)

{
  SHeap<SDynamicVB>::__Tstruct *array; // ecx
  if ( idx < 0 || idx >= this->DynamicVBs.size || (array = this->DynamicVBs.array, array[idx].use != 0x7FFFFFFF) )
    Logger.g->Panic("SGepard::UnlockDynamicVB: Bad index (%d).", idx);
  if ( !array[idx].data.Locked )
    Logger.g->Panic("SGepard::UnlockDynamicVB: Not locked.");
  array[idx].data.lpVertexBuffer->Unlock();
  this->DynamicVBs.array[idx].data.Locked = 0;
}

//----- (00445FC0) --------------------------------------------------------

void SGepard::UpdateMinimap(int width, int height, int texture, unsigned char *bits)

{
  SHeap<STextureProp>::__Tstruct *array; // eax
  STextureBitmap target;
  SBitmap bmap;
  int v9;
  if ( texture < 0 || texture >= this->Textures.size || this->Textures.array[texture].use != 0x7FFFFFFF )
    Logger.g->Panic("SGepard::UpdateMinimap: Invalid texture");
  new (&bmap) SBitmap(width, height, D3DFMT_A8R8G8B8, 0);
  array = this->Textures.array;
  v9 = 0;
  new (&target) STextureBitmap(array[texture].data.lpTexture, 0);
  *(unsigned char*)&v9 = (unsigned char)(1);
  if ( bits )
    memcpy(bmap.Data, bits, 4 * height * width);
  bmap.MakeOpaque();
  target.BitBlt(0, 0, width, height, &bmap, 0, 0);
  target.~STextureBitmap();
  (&bmap)->~SBitmap();
}

//----- (004460C0) --------------------------------------------------------

void SGepard::UpdateRects()

{
  if ( !this->Terrain )
    return;
  int i;
  int size;
  SHeap<SRectProp>::__Tstruct *j;
  SHeap<SRectProp>::__Tstruct *array;
  for ( i = -1; ; )
  {
    size = this->Rects.size;
    if ( ++i >= size )
      break;
    for ( j = &this->Rects.array[i]; j->use != 0x7FFFFFFF; ++j )
    {
      if ( ++i >= size )
        return;
    }
    if ( i < 0 )
      break;
    array = this->Rects.array;
    SMesh *mesh = array[i].data.Mesh;
    if ( mesh )
    {
      mesh->LockVertexBuffer();
      this->RefreshRect(mesh, array[i].data.X0, array[i].data.Z0, array[i].data.X1, array[i].data.Z1);
      mesh->UnlockVertexBuffer();
    }
  }
}

//----- (00446160) --------------------------------------------------------

void SGepard::UpdateShaders()

{
  SShaderInfo *first; // ecx
  SShaderInfo *current; // eax
  SShaderInfo *next; // eax
  first = this->ShaderInfos.first;
  this->ShaderInfos.current = first;
  if ( first )
  {
    do
    {
      this->RefreshShader(first);
      current = this->ShaderInfos.current;
      if ( this->ShaderInfos.Closed )
      {
        if ( !current )
          Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
        next = current->next;
        if ( !next )
          next = this->ShaderInfos.first;
      }
      else if ( current )
      {
        next = current->next;
      }
      else
      {
        next = 0;
      }
      first = next;
      this->ShaderInfos.current = next;
    }
    while ( next );
  }
}

//----- (004461D0) --------------------------------------------------------

void SGepard::UpdateTextureFromBitmap(int idx, SBitmap *bmap)

{
  SHeap<STextureProp>::__Tstruct *array; // edx
  int Height;
  STextureBitmap txt;
  int v6;
  if ( idx < 0 || idx >= this->Textures.size || (array = this->Textures.array, array[idx].use != 0x7FFFFFFF) )
    Logger.g->Panic("SGepard::UpdateTextureFromBitmap: Invalid texture index %d.", idx);
  new (&txt) STextureBitmap(array[idx].data.lpTexture, 0);
  Height = bmap->Height;
  v6 = 0;
  txt.BitBlt(0, 0, bmap->Width, Height, bmap, 0, 0);
  txt.~STextureBitmap();
}

//----- (00446280) --------------------------------------------------------

void SGepard::WaterLine(float x1, float z1, float x2, float z2, float y)

{
  // Editor mode: skip the heightmap mutation entirely. WaterLine exists for
  // in-game unit pathfinding — it conforms HeightMap cells along river edges
  // to water-surface height so units swim/walk on water properly. The editor
  // doesn't need this and gets actively harmed by it: along WATERFALL
  // splines (Alt ≫ Alt2) the strip y values reach the top of the fall
  // (e.g. +9.88), and RaiseHeight propagates that into HeightMap cells
  // underneath any bridge spanning the waterfall — bridge mesh Y is then
  // GetHeight(x,z) + yrel, so bridges visibly jump 10 units up the moment
  // the user edits any terrain (which retriggers per-frame doodad
  // re-positioning against the now-raised HeightMap).
  //
  // Gate is on this->Effect: SGameWorld::ServerInit always wires up an
  // SEffect via CreateEffectsHandler before JezusAtmentAVizenMiMiertNe runs;
  // SEditorWorld::Initialize never does. So Effect==NULL ⇔ editor build.
  if (!this->Effect) return;

  float v6; // xmm7_4
  float v7; // xmm4_4
  float v8; // xmm5_4
  float v9; // xmm3_4
  float v10; // xmm6_4
  float v12; // xmm2_4
  float v13; // xmm2_4
  float v14; // xmm5_4
  float v15; // xmm1_4
  float v16; // xmm2_4
  bool v17;
  float v18; // xmm6_4
  float v19; // xmm0_4
  float v20; // xmm1_4
  float x1a;
  float z1a;
  float x2a;
  float z2a;
  v6 = x2;
  v7 = x1;
  v8 = x2 - x1;
  v9 = z1;
  v10 = z2 - z1;
  v12 = fabsf(x2 - x1);
  if ( fabsf(z2 - z1) < v12 )
  {
    if ( x1 > x2 )
    {
      v9 = z2;
      v7 = x2;
      v6 = x1;
      x1 = x2;
      x2 = v6;
    }
    v18 = v10 / v8;
    v19 = (float)((float)(v7 - v7) * v18) + v9;
    z2a = v19;
    if ( v6 >= v7 )
    {
      v20 = 0.5f;
      z1a = v18 * 0.2f;
      do
      {
        this->Terrain->RaiseHeight((int)(float)(v7 + v20), (int)(float)(v19 + v20), y);
        v7 = x1 + 0.2f;
        v19 = z1a + z2a;
        v17 = x2 < (float)(x1 + 0.2);
        v20 = 0.5f;
        z2a = z1a + z2a;
        x1 = x1 + 0.2f;
      }
      while ( !v17 );
    }
  }
  else
  {
    v13 = z2;
    if ( z1 > z2 )
    {
      v7 = x2;
      v9 = z2;
      v13 = z1;
      z1 = z2;
      z2 = v13;
    }
    v14 = v8 / v10;
    v15 = (float)((float)(v9 - v9) * v14) + v7;
    x1a = v15;
    if ( v13 >= v9 )
    {
      v16 = 0.5f;
      x2a = v14 * 0.2f;
      do
      {
        this->Terrain->RaiseHeight((int)(float)(v15 + v16), (int)(float)(v9 + v16), y);
        v9 = z1 + 0.2f;
        v15 = x1a + x2a;
        v16 = 0.5f;
        v17 = z2 < (float)(z1 + 0.2);
        x1a = x1a + x2a;
        z1 = z1 + 0.2f;
      }
      while ( !v17 );
    }
  }
}

//----- (00446460) --------------------------------------------------------

int SGepard::getViewHeight()

{
  return this->ViewHeight;
}

//----- (00446470) --------------------------------------------------------

int SGepard::getViewWidth()

{
  return this->ViewWidth;
}

//----- (00422530) --------------------------------------------------------

SWindowEffect::SWindowEffect(SWindowEffect *p, SIObject *object, SDArray<SAblak> *ablakok)

{
  SWindowEffect *v5; // edi
  int v6;
  float v7; // xmm0_4
  SDArray<SAblak> *v8; // eax
  SAblak *array; // eax
  int NagyajtoHang;
  v5 = p;
  this->Gepard = p->Gepard;
  this->Effect = v5->Effect;
  this->Object = object;
  this->Ablakok = ablakok;
  v6 = rand();
  this->AblakCounter = 0;
  v7 = (float)v6;
  v8 = this->Ablakok;
  this->NextRandom = (float)(v7 / 32767.0f) + 0.5f;
  array = v8->array;
  array->active = 1;
  if ( array->type == 1 )
  {
    this->Object->GetPosition((float *)&ablakok, (float *)&object, (float *)&p);
    NagyajtoHang = v5->NagyajtoHang;
    if ( NagyajtoHang != -1 )
      Concert->PlaySoundById(NagyajtoHang, 0.0f, 0.0f, 0); // IDA: PlaySound(handle, 1092616192, ablakok, object)
  }
  else
  {
    this->Ablakok->array[this->AblakCounter + 1].active = 1;
  }
}

//----- (00422610) --------------------------------------------------------

SWindowEffect::SWindowEffect(SGepard *gepard, SEffect *effect, char *classname)

{
  int v5;
  this->Gepard = gepard;
  this->Effect = effect;
  v5 = Concert->PrecacheSound("effects/nagydisznoajto.wav", 1);
  this->NagyajtoHang = v5;
  Concert->AddRefToCachedSound(v5);
}


//----- (00435830) --------------------------------------------------------

int SGepard::Initialize(HWND _hwnd, bool isfullscreen, bool vsync, D3DMULTISAMPLE_TYPE msaa, int width, int height, bool enable_tl)
{
  HRESULT hr;
  D3DCAPS9 caps;
  char atmstr[200];

  this->hWnd = _hwnd;
  this->lpD3D = Direct3DCreate9(D3D_SDK_VERSION);
  if ( !this->lpD3D )
  {
    Logger.g->Log(0, "SGepard::Initialize: Direct3DCreate9 failed");
    return 1;
  }
  this->Adapter = 0;
  this->DeviceType = D3DDEVTYPE_HAL;
  hr = this->lpD3D->GetAdapterDisplayMode(0, &this->d3ddm);
  if ( hr )
  {
    sprintf(atmstr, "%s: %s", "SGepard::Initialize: GetAdapterDisplayMode", DXGetErrorStringA(hr));
    Logger.g->Log(0, atmstr);
    return 2;
  }
  this->VSync = vsync;
  this->FullScreen = isfullscreen;
  this->MSAALevel = msaa;
  if ( isfullscreen )
  {
    this->ModeWidth = width;
    this->ModeHeight = height;
  }
  this->ViewHeight = height;
  this->ViewWidth = width;
  hr = this->lpD3D->GetAdapterIdentifier(this->Adapter, 0, &this->AdapterID);
  if ( hr )
  {
    sprintf(atmstr, "%s: %s", "SGepard::Initialize: GetAdapterIdentifier", DXGetErrorStringA(hr));
    Logger.g->Log(0, atmstr);
    return 2;
  }
  Logger.g->Log(0, "  Driver %s", this->AdapterID.Driver);
  Logger.g->Log(0, "  Description %s", this->AdapterID.Description);
  Logger.g->Log(0, "  VendorId 0x%08X", this->AdapterID.VendorId);
  Logger.g->Log(0, "  DeviceId 0x%08X", this->AdapterID.DeviceId);
  Logger.g->Log(0, "  SubSysId 0x%08X", this->AdapterID.SubSysId);
  Logger.g->Log(0, "  Revision 0x%08X", this->AdapterID.Revision);
  hr = this->lpD3D->GetDeviceCaps(this->Adapter, this->DeviceType, &caps);
  if ( hr )
  {
    sprintf(atmstr, "%s: %s", "SGepard::Initialize: GetDeviceCaps", DXGetErrorStringA(hr));
    Logger.g->Log(0, atmstr);
    return 2;
  }
  if ( caps.MaxTextureWidth < 0x800 || caps.MaxTextureHeight < 0x800 )
  {
    Logger.g->Log(0,
      "SGepard::Initialize: Maximum texture size is too small (%d x %d)",
      caps.MaxTextureWidth, caps.MaxTextureHeight);
    return 4;
  }
  SetUpPresentation();
  this->PresentationParameters.hDeviceWindow = this->hWnd;
  #ifdef HD_DEBUG_RENDERER
  Logger.g->Log(0, "PresentParams: %dx%d fmt=%d windowed=%d swap=%d depth=%d msaa=%d interval=0x%X hwnd=0x%X",
    this->PresentationParameters.BackBufferWidth,
    this->PresentationParameters.BackBufferHeight,
    this->PresentationParameters.BackBufferFormat,
    this->PresentationParameters.Windowed,
    this->PresentationParameters.SwapEffect,
    this->PresentationParameters.AutoDepthStencilFormat,
    this->PresentationParameters.MultiSampleType,
    this->PresentationParameters.PresentationInterval,
    (unsigned int)(uintptr_t)this->PresentationParameters.hDeviceWindow);
  #endif
  #ifdef HD_DEBUG_RENDERER
  Logger.g->Log(0, "FullScreen=%d AdapterFormat=%d EnableAutoDepthStencil=%d",
    this->FullScreen, this->AdapterFormat,
    this->PresentationParameters.EnableAutoDepthStencil);
  #endif

  // Try CreateDevice with multiple fallback configurations
  {
    DWORD behaviorFlags = ((caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) && enable_tl)
      ? D3DCREATE_HARDWARE_VERTEXPROCESSING : D3DCREATE_SOFTWARE_VERTEXPROCESSING;
    if (behaviorFlags == D3DCREATE_HARDWARE_VERTEXPROCESSING)
      Logger.g->Log(0, "Gepard: Using hardware vertexprocessing");
    else
      Logger.g->Log(0, "Gepard: Using software vertexprocessing");
    const char *vpName = (behaviorFlags == D3DCREATE_HARDWARE_VERTEXPROCESSING) ? "HW" : "SW";
    D3DPRESENT_PARAMETERS savedPP = this->PresentationParameters;

    // Attempt 1: original settings
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "Gepard: CreateDevice attempt 1 (%s VP, D24S8)", vpName);
    #endif
    hr = this->lpD3D->CreateDevice(this->Adapter, this->DeviceType, this->hWnd,
      behaviorFlags, &this->PresentationParameters, &this->lpD3DDev);

    // Attempt 2: SW vertex processing
    if ( hr && behaviorFlags == D3DCREATE_HARDWARE_VERTEXPROCESSING )
    {
      #ifdef HD_DEBUG_RENDERER
      Logger.g->Log(0, "Gepard: CreateDevice attempt 2 (SW VP, D24S8)");
      #endif
      this->PresentationParameters = savedPP;
      behaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
      hr = this->lpD3D->CreateDevice(this->Adapter, this->DeviceType, this->hWnd,
        behaviorFlags, &this->PresentationParameters, &this->lpD3DDev);
    }

    // Attempt 3: D3DFMT_D16 depth
    if ( hr )
    {
      #ifdef HD_DEBUG_RENDERER
      Logger.g->Log(0, "Gepard: CreateDevice attempt 3 (SW VP, D16)");
      #endif
      this->PresentationParameters = savedPP;
      this->PresentationParameters.AutoDepthStencilFormat = D3DFMT_D16;
      behaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
      hr = this->lpD3D->CreateDevice(this->Adapter, this->DeviceType, this->hWnd,
        behaviorFlags, &this->PresentationParameters, &this->lpD3DDev);
    }

    // Attempt 4: no auto depth stencil
    if ( hr )
    {
      #ifdef HD_DEBUG_RENDERER
      Logger.g->Log(0, "Gepard: CreateDevice attempt 4 (SW VP, no depth stencil)");
      #endif
      this->PresentationParameters = savedPP;
      this->PresentationParameters.EnableAutoDepthStencil = FALSE;
      this->PresentationParameters.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
      behaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
      hr = this->lpD3D->CreateDevice(this->Adapter, this->DeviceType, this->hWnd,
        behaviorFlags, &this->PresentationParameters, &this->lpD3DDev);
    }

    // Attempt 5: REF device (software rasterizer)
    if ( hr )
    {
      #ifdef HD_DEBUG_RENDERER
      Logger.g->Log(0, "Gepard: CreateDevice attempt 5 (REF device)");
      #endif
      this->PresentationParameters = savedPP;
      this->PresentationParameters.EnableAutoDepthStencil = FALSE;
      this->PresentationParameters.AutoDepthStencilFormat = D3DFMT_UNKNOWN;
      this->DeviceType = D3DDEVTYPE_REF;
      behaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
      hr = this->lpD3D->CreateDevice(this->Adapter, this->DeviceType, this->hWnd,
        behaviorFlags, &this->PresentationParameters, &this->lpD3DDev);
    }

    if ( hr )
    {
      sprintf(atmstr, "%s: %s", "SGepard::Initialize: CreateDevice (all attempts failed)", DXGetErrorStringA(hr));
      Logger.g->Log(0, atmstr);
      return 3;
    }
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "Gepard: CreateDevice succeeded");
    #endif
  }
  LogCardInfo();
  InitTextureFormats();
  this->LightProps[0].type = 0;
  this->LightProps[1].type = 0;
  this->LightProps[2].type = 0;
  this->LightProps[3].type = 0;
  this->TextureFilter = 3;
  this->TextureDetail = 1;
  this->OrthoScale = 0.0f;
  this->FOV = 1.2502674f;
  this->NearPlane = 1.0f;
  this->FarPlane = 600.0f;
  InitRenderStates();
  InitShaders();
  SetViewProperties(0, 0, 0, 0, 0);
  this->DefAmbient = 0x00505050;
  this->node2 = 0;
  this->node1 = 0;
  this->FPSTextFrame = -1;
  this->DebugInfoFrame = -1;
  this->DebugMouseX = 0.0f;
  this->DebugMouseY = 0.0f;
  this->DebugMouseZ = 0.0f;
#ifdef _DEBUG
  this->ShowDebugInfo = true;  // see ctor at ~line 748 for rationale
#else
  this->ShowDebugInfo = false;
#endif
  this->HoverShader = nullptr;
  this->DebugTextFrame = -1;
  Logger.g->Log(0, "Gepard: 3D Engine sucessfully initialized");
  #ifdef HD_DEBUG_RENDERER
  Logger.g->Log(0, "");
  #endif
  this->_folyo = LoadTexture("effects\\teszt_folyo_a.tga", 1, 0);
  this->_folyo_csillogas = LoadTexture("fx/ocean/water2.tga", 1, 0);
  this->_gray_texture = LoadTexture("zpatch/gray.tga", 1, 0);
  SBoard *newBoard = new SBoard(this);
  this->Board = newBoard;
  this->Board->ResizeFrame(0, this->ViewWidth, this->ViewHeight);
  ::Board = this->Board;
  #ifdef HD_DEBUG_RENDERER
  Logger.g->Log(0, "SGepard::Initialize: Board set to %p (global Board=%p, &Board=%p)", newBoard, ::Board, &::Board);
  #endif
  return 0;
}

//----- (00436640) --------------------------------------------------------

void SGepard::LogCardInfo()
{
  D3DCAPS9 caps;
  this->lpD3DDev->GetDeviceCaps(&caps);
  Logger.g->Log(1, "  -----------------------");
  if (caps.DeviceType == D3DDEVTYPE_HAL)
    Logger.g->Log(1, "  Device = HAL");
  else if (caps.DeviceType == D3DDEVTYPE_REF)
    Logger.g->Log(1, "  Device = Reference");
  else
    Logger.g->Log(1, "  Device = Unknown");
  Logger.g->Log(1, "  Full screen gamma = %d", (caps.Caps2 >> 17) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Hardware color cursor (hires) = %d", caps.CursorCaps & 1);
  Logger.g->Log(1, "  Hardware color cursor (lores) = %d", (caps.CursorCaps >> 1) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Can BLT from System Memory to Non-Local Video Memory = %d", (caps.DevCaps >> 17) & 1);
  Logger.g->Log(1, "  Can render after page flip = %d", (caps.DevCaps >> 11) & 1);
  Logger.g->Log(1, "  DrawPrimitive T & L support = %d", (caps.DevCaps >> 10) & 1);
  Logger.g->Log(1, "  Execute buffers in System Memory = %d", (caps.DevCaps >> 4) & 1);
  Logger.g->Log(1, "  Execute buffers in Video Memory = %d", (caps.DevCaps >> 5) & 1);
  Logger.g->Log(1, "  Hardware rasterization = %d", (caps.DevCaps >> 19) & 1);
  Logger.g->Log(1, "  Hardware T & L = %d", HIWORD(caps.DevCaps) & 1);
  Logger.g->Log(1, "  Patches (N-patches) = %d", BYTE3(caps.DevCaps) & 1);
  Logger.g->Log(1, "  Patches (quintic patches) = %d", (caps.DevCaps >> 21) & 1);
  Logger.g->Log(1, "  Patches (rectangular and triangular patches) = %d", (caps.DevCaps >> 22) & 1);
  Logger.g->Log(1, "  Patches need not to be cached = %d", (caps.DevCaps >> 23) & 1);
  Logger.g->Log(1, "  Texturing from Non-Local Video Memory = %d", (caps.DevCaps >> 12) & 1);
  Logger.g->Log(1, "  Texturing from System Memory = %d", (caps.DevCaps >> 8) & 1);
  Logger.g->Log(1, "  Texturing from Video Memory = %d", (caps.DevCaps >> 9) & 1);
  Logger.g->Log(1, "  T & L from System Memory = %d", (caps.DevCaps >> 6) & 1);
  Logger.g->Log(1, "  T & L from Video Memory = %d", (caps.DevCaps >> 7) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Blend operations = %d", (caps.PrimitiveMiscCaps >> 11) & 1);
  Logger.g->Log(1, "  Clips transformed/lit primitives = %d", (caps.PrimitiveMiscCaps >> 9) & 1);
  Logger.g->Log(1, "  Can disable Color Buffer writes = %d", (caps.PrimitiveMiscCaps >> 7) & 1);
  Logger.g->Log(1, "  Can disable Z-buffer writes = %d", (caps.PrimitiveMiscCaps >> 1) & 1);
  Logger.g->Log(1, "  Can use TMP register in texture stages = %d", (caps.PrimitiveMiscCaps >> 10) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Anisotropy = %d", (caps.RasterCaps >> 17) & 1);
  Logger.g->Log(1, "  Color pespective correction = %d", (caps.RasterCaps >> 22) & 1);
  Logger.g->Log(1, "  Dithering = %d", caps.RasterCaps & 1);
  Logger.g->Log(1, "  Range fog = %d", HIWORD(caps.RasterCaps) & 1);
  Logger.g->Log(1, "  Table fog = %d", (caps.RasterCaps >> 8) & 1);
  Logger.g->Log(1, "  Vertex fog = %d", (caps.RasterCaps >> 7) & 1);
  Logger.g->Log(1, "  Mipmap LOD bias = %d", (caps.RasterCaps >> 13) & 1);
  Logger.g->Log(1, "  W-buffering = %d", (caps.RasterCaps >> 18) & 1);
  Logger.g->Log(1, "  W based fog = %d", (caps.RasterCaps >> 20) & 1);
  Logger.g->Log(1, "  Hidden surface removal (without Z buffer) = %d", (caps.RasterCaps >> 15) & 1);
  Logger.g->Log(1, "  Z based fog = %d", (caps.RasterCaps >> 21) & 1);
  Logger.g->Log(1, "  Z test = %d", (caps.RasterCaps >> 4) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Z compare supports: always = %d", (caps.ZCmpCaps >> 7) & 1);
  Logger.g->Log(1, "  Z compare supports:   ==   = %d", (caps.ZCmpCaps >> 2) & 1);
  Logger.g->Log(1, "  Z compare supports:   >    = %d", (caps.ZCmpCaps >> 4) & 1);
  Logger.g->Log(1, "  Z compare supports:   >=   = %d", (caps.ZCmpCaps >> 6) & 1);
  Logger.g->Log(1, "  Z compare supports:   <    = %d", (caps.ZCmpCaps >> 1) & 1);
  Logger.g->Log(1, "  Z compare supports:   <=   = %d", (caps.ZCmpCaps >> 3) & 1);
  Logger.g->Log(1, "  Z compare supports: never  = %d", caps.ZCmpCaps & 1);
  Logger.g->Log(1, "  Z compare supports:   !=   = %d", (caps.ZCmpCaps >> 5) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Source blend supports: Destination alpha = %d", (caps.SrcBlendCaps >> 6) & 1);
  Logger.g->Log(1, "  Source blend supports: Destination color = %d", (caps.SrcBlendCaps >> 8) & 1);
  Logger.g->Log(1, "  Source blend supports: Destination inverse alpha = %d", (caps.SrcBlendCaps >> 7) & 1);
  Logger.g->Log(1, "  Source blend supports: Destination inverse color = %d", (caps.SrcBlendCaps >> 9) & 1);
  Logger.g->Log(1, "  Source blend supports: Source alpha = %d", (caps.SrcBlendCaps >> 4) & 1);
  Logger.g->Log(1, "  Source blend supports: Source color = %d", (caps.SrcBlendCaps >> 2) & 1);
  Logger.g->Log(1, "  Source blend supports: Source inverse alpha = %d", (caps.SrcBlendCaps >> 5) & 1);
  Logger.g->Log(1, "  Source blend supports: Source inverse color = %d", (caps.SrcBlendCaps >> 3) & 1);
  Logger.g->Log(1, "  Source blend supports: 1 = %d", (caps.SrcBlendCaps >> 1) & 1);
  Logger.g->Log(1, "  Source blend supports: 0 = %d", caps.SrcBlendCaps & 1);
  Logger.g->Log(1, "  Source blend supports: Source alpha sat = %d", (caps.SrcBlendCaps >> 10) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Destination blend supports: Destination alpha = %d", (caps.DestBlendCaps >> 6) & 1);
  Logger.g->Log(1, "  Destination blend supports: Destination color = %d", (caps.DestBlendCaps >> 8) & 1);
  Logger.g->Log(1, "  Destination blend supports: Destination inverse alpha = %d", (caps.DestBlendCaps >> 7) & 1);
  Logger.g->Log(1, "  Destination blend supports: Destination inverse color = %d", (caps.DestBlendCaps >> 9) & 1);
  Logger.g->Log(1, "  Destination blend supports: Source alpha = %d", (caps.DestBlendCaps >> 4) & 1);
  Logger.g->Log(1, "  Destination blend supports: Source color = %d", (caps.DestBlendCaps >> 2) & 1);
  Logger.g->Log(1, "  Destination blend supports: Source inverse alpha = %d", (caps.DestBlendCaps >> 5) & 1);
  Logger.g->Log(1, "  Destination blend supports: Source inverse color = %d", (caps.DestBlendCaps >> 3) & 1);
  Logger.g->Log(1, "  Destination blend supports: 1 = %d", (caps.DestBlendCaps >> 1) & 1);
  Logger.g->Log(1, "  Destination blend supports: 0 = %d", caps.DestBlendCaps & 1);
  Logger.g->Log(1, "  Destination blend supports: Source alpha sat = %d", (caps.DestBlendCaps >> 10) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Alpha compare supports: always = %d", (caps.AlphaCmpCaps >> 7) & 1);
  Logger.g->Log(1, "  Alpha compare supports:   ==   = %d", (caps.AlphaCmpCaps >> 2) & 1);
  Logger.g->Log(1, "  Alpha compare supports:   >    = %d", (caps.AlphaCmpCaps >> 4) & 1);
  Logger.g->Log(1, "  Alpha compare supports:   >=   = %d", (caps.AlphaCmpCaps >> 6) & 1);
  Logger.g->Log(1, "  Alpha compare supports:   <    = %d", (caps.AlphaCmpCaps >> 1) & 1);
  Logger.g->Log(1, "  Alpha compare supports:   <=   = %d", (caps.AlphaCmpCaps >> 3) & 1);
  Logger.g->Log(1, "  Alpha compare supports: never  = %d", caps.AlphaCmpCaps & 1);
  Logger.g->Log(1, "  Alpha compare supports:   !=   = %d", (caps.AlphaCmpCaps >> 5) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Gouraud RGB = %d", (caps.ShadeCaps >> 3) & 1);
  Logger.g->Log(1, "  Gouraud Alpha = %d", (caps.ShadeCaps >> 14) & 1);
  Logger.g->Log(1, "  Gouraud Specular RGB = %d", (caps.ShadeCaps >> 9) & 1);
  Logger.g->Log(1, "  Gouraud Fog = %d", (caps.ShadeCaps >> 19) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Texture Alpha = %d", (caps.TextureCaps >> 2) & 1);
  Logger.g->Log(1, "  Texture Alpha in Palette = %d", (caps.TextureCaps >> 7) & 1);
  Logger.g->Log(1, "  Texture MipMap = %d", (caps.TextureCaps >> 14) & 1);
  Logger.g->Log(1, "  Texture can be non power of 2 = %d", (caps.TextureCaps >> 8) & 1);
  Logger.g->Log(1, "  Texture perspective correction = %d", caps.TextureCaps & 1);
  Logger.g->Log(1, "  Texture projection = %d", (caps.TextureCaps >> 10) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Textures can be bordered = %d", (caps.TextureAddressCaps >> 3) & 1);
  Logger.g->Log(1, "  Textures can be clamped = %d", (caps.TextureAddressCaps >> 2) & 1);
  Logger.g->Log(1, "  Textures can be mirrored = %d", (caps.TextureAddressCaps >> 1) & 1);
  Logger.g->Log(1, "  Textures can be once mirrored = %d", (caps.TextureAddressCaps >> 5) & 1);
  Logger.g->Log(1, "  Textures can be wrapped = %d", caps.TextureAddressCaps & 1);
  Logger.g->Log(1, "  Texture wrapping can be set independently for U & V = %d", (caps.TextureAddressCaps >> 4) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Maximum Texture Size = %d x %d", caps.MaxTextureWidth, caps.MaxTextureHeight);
  Logger.g->Log(1, "  Maximum Texture Repeat = %d", caps.MaxTextureRepeat);
  Logger.g->Log(1, "  Maximum Texture Aspect Ratio = %d", caps.MaxTextureAspectRatio);
  Logger.g->Log(1, "  Maximum Texture Anisotropy = %d", caps.MaxAnisotropy);
  Logger.g->Log(1, "  Maximum Texture Blend Stages = %d", caps.MaxTextureBlendStages);
  Logger.g->Log(1, "  Maximum Texture can be used similtaneously = %d", caps.MaxSimultaneousTextures);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Texture operation can be: disable           = %d", caps.TextureOpCaps & 1);
  Logger.g->Log(1, "  Texture operation can be: select argument 1 = %d", (caps.TextureOpCaps >> 1) & 1);
  Logger.g->Log(1, "  Texture operation can be: select argument 2 = %d", (caps.TextureOpCaps >> 2) & 1);
  Logger.g->Log(1, "  Texture operation can be: modulate     = %d", (caps.TextureOpCaps >> 3) & 1);
  Logger.g->Log(1, "  Texture operation can be: modulate * 2 = %d", (caps.TextureOpCaps >> 4) & 1);
  Logger.g->Log(1, "  Texture operation can be: modulate * 4 = %d", (caps.TextureOpCaps >> 5) & 1);
  Logger.g->Log(1, "  Texture operation can be: add            = %d", (caps.TextureOpCaps >> 6) & 1);
  Logger.g->Log(1, "  Texture operation can be: substract      = %d", (caps.TextureOpCaps >> 9) & 1);
  Logger.g->Log(1, "  Texture operation can be: add signed     = %d", (caps.TextureOpCaps >> 7) & 1);
  Logger.g->Log(1, "  Texture operation can be: add 2 * signed = %d", (caps.TextureOpCaps >> 8) & 1);
  Logger.g->Log(1, "  Texture operation can be: add smooth     = %d", (caps.TextureOpCaps >> 10) & 1);
  Logger.g->Log(1, "  Texture operation can be: blend with current alpha    = %d", (caps.TextureOpCaps >> 15) & 1);
  Logger.g->Log(1, "  Texture operation can be: blend with diffuse alpha    = %d", (caps.TextureOpCaps >> 11) & 1);
  Logger.g->Log(1, "  Texture operation can be: blend with factor alpha     = %d", (caps.TextureOpCaps >> 13) & 1);
  Logger.g->Log(1, "  Texture operation can be: blend with texture alpha    = %d", (caps.TextureOpCaps >> 12) & 1);
  Logger.g->Log(1, "  Texture operation can be: blend with texture PM alpha = %d", (caps.TextureOpCaps >> 14) & 1);
  Logger.g->Log(1, "  Texture operation can be: premodulate                       = %d", HIWORD(caps.TextureOpCaps) & 1);
  Logger.g->Log(1, "  Texture operation can be: modulate alpha, add color         = %d", (caps.TextureOpCaps >> 17) & 1);
  Logger.g->Log(1, "  Texture operation can be: modulate color, add alpha         = %d", (caps.TextureOpCaps >> 18) & 1);
  Logger.g->Log(1, "  Texture operation can be: modulate inverse alpha, add color = %d", (caps.TextureOpCaps >> 19) & 1);
  Logger.g->Log(1, "  Texture operation can be: modulate inverse color, add alpha = %d", (caps.TextureOpCaps >> 20) & 1);
  Logger.g->Log(1, "  Texture operation can be: bump envmap                = %d", (caps.TextureOpCaps >> 21) & 1);
  Logger.g->Log(1, "  Texture operation can be: bump envmap with luminance = %d", (caps.TextureOpCaps >> 22) & 1);
  Logger.g->Log(1, "  Texture operation can be: bump dotproduct            = %d", (caps.TextureOpCaps >> 23) & 1);
  Logger.g->Log(1, "  Texture operation can be: multiply add         = %d", BYTE3(caps.TextureOpCaps) & 1);
  Logger.g->Log(1, "  Texture operation can be: linear interpolation = %d", (caps.TextureOpCaps >> 25) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Vertex processing supports: Directional Lights = %d", (caps.VertexProcessingCaps >> 3) & 1);
  Logger.g->Log(1, "  Vertex processing supports: Local Viewer = %d", (caps.VertexProcessingCaps >> 5) & 1);
  Logger.g->Log(1, "  Vertex processing supports: Material color sources = %d", (caps.VertexProcessingCaps >> 1) & 1);
  Logger.g->Log(1, "  Vertex processing supports: Positional Lights = %d", (caps.VertexProcessingCaps >> 4) & 1);
  Logger.g->Log(1, "  Vertex processing supports: Texture coordinate generation = %d", caps.VertexProcessingCaps & 1);
  Logger.g->Log(1, "  Vertex processing supports: Tweening ??? = %d", (caps.VertexProcessingCaps >> 6) & 1);
  Logger.g->Log(1, "  -----------------------");
  Logger.g->Log(1, "  Maximum Active Lights = %d", caps.MaxActiveLights);
  Logger.g->Log(1, "  Maximum User Clip Planes = %d", caps.MaxUserClipPlanes);
  Logger.g->Log(1, "  Maximum Vertex Blend Matrices = %d", caps.MaxVertexBlendMatrices);
  Logger.g->Log(1, "  Maximum Point Size = %f", caps.MaxPointSize);
  Logger.g->Log(1, "  Maximum Primitive Count = %d", caps.MaxPrimitiveCount);
  Logger.g->Log(1, "  Maximum Vertex Index = %d", caps.MaxVertexIndex);
  Logger.g->Log(1, "  Maximum Vertex Streams = %d", caps.MaxStreams);
  Logger.g->Log(1, "  Maximum Vertex Stream Stride = %d", caps.MaxStreamStride);
  Logger.g->Log(1, "  Vertex Shader Version = %d.%d", BYTE1(caps.VertexShaderVersion), LOBYTE(caps.VertexShaderVersion));
  Logger.g->Log(1, "  Maximum Vertex Shader Constants = %d", caps.MaxVertexShaderConst);
  Logger.g->Log(1, "  Pixel Shader Version = %d.%d", BYTE1(caps.PixelShaderVersion), LOBYTE(caps.PixelShaderVersion));
  Logger.g->Log(1, "  Maximum Pixel Shader Value = %f", caps.PixelShader1xMaxValue);
  Logger.g->Log(1, "  -----------------------");

  // Resource type / format enumeration
  struct FormatEntry { D3DFORMAT format; const char *name; };
  struct ResourceEntry { D3DRESOURCETYPE type; unsigned int usage; const char *name; };

  static FormatEntry AdapterFormats[] = {
    { D3DFMT_R8G8B8, "R8G8B8" },
    { D3DFMT_A8R8G8B8, "A8R8G8B8" },
    { D3DFMT_X8R8G8B8, "X8R8G8B8" },
    { D3DFMT_R5G6B5, "R5G6B5" },
    { D3DFMT_X1R5G5B5, "X1R5G5B5" },
    { D3DFMT_A1R5G5B5, "A1R5G5B5" },
    { D3DFMT_A4R4G4B4, "A4R4G4B4" },
    { D3DFMT_R3G3B2, "R3G3B2" },
    { D3DFMT_A8, "A8" },
    { D3DFMT_A8R3G3B2, "A8R3G3B2" },
    { D3DFMT_X4R4G4B4, "X4R4G4B4" },
    { D3DFMT_A2B10G10R10, "A2B10G10R10" },
    { D3DFMT_A8B8G8R8, "A8B8G8R8" },
    { D3DFMT_X8B8G8R8, "X8B8G8R8" },
    { D3DFMT_G16R16, "G16R16" },
    { D3DFMT_A2R10G10B10, "A2R10G10B10" },
    { D3DFMT_A16B16G16R16, "A16B16G16R16" },
    { D3DFMT_A8P8, "A8P8" },
    { D3DFMT_P8, "P8" },
    { D3DFMT_L8, "L8" },
    { D3DFMT_A8L8, "A8L8" },
    { D3DFMT_A4L4, "A4L4" },
    { D3DFMT_V8U8, "V8U8" },
    { D3DFMT_L6V5U5, "L6V5U5" },
    { D3DFMT_X8L8V8U8, "X8L8V8U8" },
    { D3DFMT_Q8W8V8U8, "Q8W8V8U8" },
    { D3DFMT_V16U16, "V16U16" },
    { D3DFMT_A2W10V10U10, "A2W10V10U10" },
    { D3DFMT_UYVY, "UYVY" },
    { D3DFMT_R8G8_B8G8, "R8G8_B8G8" },
    { D3DFMT_YUY2, "YUY2" },
    { D3DFMT_G8R8_G8B8, "G8R8_G8B8" },
    { D3DFMT_DXT1, "DXT1" },
    { D3DFMT_DXT2, "DXT2" },
    { D3DFMT_DXT3, "DXT3" },
    { D3DFMT_DXT4, "DXT4" },
    { D3DFMT_DXT5, "DXT5" },
    { D3DFMT_D16_LOCKABLE, "D16_LOCKABLE" },
    { D3DFMT_D32, "D32" },
    { D3DFMT_D15S1, "D15S1" },
    { D3DFMT_D24S8, "D24S8" },
    { D3DFMT_D24X8, "D24X8" },
    { D3DFMT_D24X4S4, "D24X4S4" },
    { D3DFMT_D16, "D16" },
    { D3DFMT_D32F_LOCKABLE, "D32F_LOCKABLE" },
    { D3DFMT_D24FS8, "D24FS8" },
    { D3DFMT_L16, "L16" },
    { D3DFMT_VERTEXDATA, "VERTEXDATA" },
    { D3DFMT_INDEX16, "INDEX16" },
    { D3DFMT_INDEX32, "INDEX32" },
    { D3DFMT_Q16W16V16U16, "Q16W16V16U16" },
    { D3DFMT_MULTI2_ARGB8, "MULTI2_ARGB8" },
    { D3DFMT_R16F, "R16F" },
    { D3DFMT_G16R16F, "G16R16F" },
    { D3DFMT_A16B16G16R16F, "A16B16G16R16F" },
    { D3DFMT_R32F, "R32F" },
    { D3DFMT_G32R32F, "G32R32F" },
    { D3DFMT_A32B32G32R32F, "A32B32G32R32F" },
    { D3DFMT_CxV8U8, "CxV8U8" },
    { D3DFMT_UNKNOWN, NULL }
  };

  static ResourceEntry ResourceTypes[] = {
    { D3DRTYPE_SURFACE, 1, "render target surface" },
    { D3DRTYPE_SURFACE, 2, "depth-stencil surface" },
    { D3DRTYPE_TEXTURE, 0, "texture" },
    { D3DRTYPE_CUBETEXTURE, 0, "cube texture" },
    { D3DRTYPE_VOLUMETEXTURE, 0, "volume texture" },
    { (D3DRESOURCETYPE)0, 0, NULL }
  };

  for (int r = 0; ResourceTypes[r].type; r++)
  {
    Logger.g->Log(1, "  Supported %s types:", ResourceTypes[r].name);
    for (int f = 0; AdapterFormats[f].format; f++)
    {
      if ( !this->lpD3D->CheckDeviceFormat(
              this->Adapter, this->DeviceType, this->AdapterFormat,
              ResourceTypes[r].usage, ResourceTypes[r].type,
              AdapterFormats[f].format) )
        Logger.g->Log(1, "    %s", AdapterFormats[f].name);
    }
    Logger.g->Log(1, "  -----------------------");
  }
}

//----- (004355B0) --------------------------------------------------------

void SGepard::InitTextureFormats()
{
  D3DCAPS9 caps;
  this->lpD3DDev->GetDeviceCaps(&caps);

  // TFOpaque: prefer DXT1, fallback R5G6B5
  if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT1) )
    this->TFOpaque = D3DFMT_DXT1;
  else if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_R5G6B5) )
    this->TFOpaque = D3DFMT_R5G6B5;
  else
    Logger.g->Panic("SGepard::InitTextureFormats: No texture formats available for opaque textures");

  // TF1Bit: prefer DXT1, fallback A1R5G5B5
  if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT1) )
    this->TF1Bit = D3DFMT_DXT1;
  else if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_A1R5G5B5) )
    this->TF1Bit = D3DFMT_A1R5G5B5;
  else
    Logger.g->Panic("SGepard::InitTextureFormats: No texture formats available for 1bit-alpha textures");

  // TFAlpha: prefer DXT5, fallback A8R8G8B8, then A4R4G4B4
  if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_DXT5) )
    this->TFAlpha = D3DFMT_DXT5;
  else if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) )
    this->TFAlpha = D3DFMT_A8R8G8B8;
  else if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_A4R4G4B4) )
    this->TFAlpha = D3DFMT_A4R4G4B4;
  else
    Logger.g->Panic("SGepard::InitTextureFormats: No texture formats available for full-alpha textures");

  // TFHiOpaque: prefer X8R8G8B8, fallback A8R8G8B8
  if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8) )
    this->TFHiOpaque = D3DFMT_X8R8G8B8;
  else if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) )
    this->TFHiOpaque = D3DFMT_A8R8G8B8;
  else
    Logger.g->Panic("SGepard::InitTextureFormats: No texture formats available for hi-quality opaque textures");

  // TFHiAlpha: must be A8R8G8B8
  if ( this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_A8R8G8B8) )
    Logger.g->Panic("SGepard::InitTextureFormats: No texture formats available for hi-quality alpha textures");
  this->TFHiAlpha = D3DFMT_A8R8G8B8;

  // TFShadow: render target format - prefer R5G6B5, then X8R8G8B8, then A8R8G8B8
  if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_R5G6B5) )
    this->TFShadow = D3DFMT_R5G6B5;
  else if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_X8R8G8B8) )
    this->TFShadow = D3DFMT_X8R8G8B8;
  else if ( CheckTextureFormat(D3DFMT_A8R8G8B8, 1) )
    this->TFShadow = D3DFMT_A8R8G8B8;
  else
    this->TFShadow = D3DFMT_UNKNOWN;

  // TFShadowNull: D3DFMT_NULL if supported, else fall back to TFShadow
  D3DFORMAT D3DFMT_NULL_FORMAT = (D3DFORMAT)MAKEFOURCC('N','U','L','L');
  if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, D3DFMT_NULL_FORMAT) )
    this->TFShadowNull = D3DFMT_NULL_FORMAT;
  else
    this->TFShadowNull = this->TFShadow;

  // TFShadowDepth: D3DFMT_D24X8 or D3DFMT_D16
  if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, (D3DRESOURCETYPE)3, D3DFMT_D24X8) )
  {
    this->TFShadowDepth = D3DFMT_D24X8;
    this->MaxShadowQuality = (caps.MaxTextureWidth >= 0x1000) ? 3 : 2;
  }
  else if ( !this->lpD3D->CheckDeviceFormat(this->Adapter, this->DeviceType, this->AdapterFormat, 0, D3DRTYPE_TEXTURE, D3DFMT_D16) )
  {
    this->TFShadowDepth = D3DFMT_D16;
    this->MaxShadowQuality = (caps.MaxTextureWidth >= 0x1000) ? 3 : 2;
  }
  else
  {
    this->TFShadowDepth = D3DFMT_UNKNOWN;
    this->MaxShadowQuality = 1;
  }
  if ( this->ShadowQuality > this->MaxShadowQuality )
    this->ShadowQuality = this->MaxShadowQuality;

  // TFSupportNonPow2
  bool nonPow2 = (caps.TextureCaps & D3DPTEXTURECAPS_POW2) == 0 || (caps.TextureCaps & D3DPTEXTURECAPS_NONPOW2CONDITIONAL) != 0;
  this->TFSupportNonPow2 = nonPow2;
  if ( nonPow2 )
    Logger.g->Log(0, "SGepard::InitTextureFormats: Card supports non-power-of-2 textures");
  else
    Logger.g->Log(0, "SGepard::InitTextureFormats: Only power-of-2 textures supported");
}

//----- (00434740) --------------------------------------------------------

void SGepard::InitRenderStates()
{
  SetTextureFilter(this->TextureFilter);
  this->lpD3DDev->SetRenderState(D3DRS_DITHERENABLE, TRUE);
  this->lpD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_CW);
  for ( int i = 0; i < 4; i++ )
  {
    if ( this->LightProps[i].type != 0 )
    {
      this->lpD3DDev->SetLight(i, &this->LightProps[i].d3dlight);
      this->lpD3DDev->LightEnable(i, TRUE);
    }
  }
  if ( this->OrthoScale == 0.0f )
    SetPerspectiveProjection(this->FOV, this->NearPlane, this->FarPlane);
  else
    SetOrthogonalProjection(this->OrthoScale, this->NearPlane, this->FarPlane);
}

//----- (00434A10) --------------------------------------------------------

void SGepard::InitShaders()
{
  char shadowMapResolutionStr[8];
  char debugModeStr[8];

  // Build base defines
  const char *selfShadowVal = (this->ShadowQuality >= 2) ? "1" : "0";
  const char *enableShadowVal = (this->ShadowQuality >= 2) ? "1" : "0";
  sprintf(shadowMapResolutionStr, "%d", this->ShadowMapWidth);
  sprintf(debugModeStr, "%d", this->DebugMode);

  // Max defines needed: 4 base + 5 per-shader + 1 null = 10
  D3DXMACRO defines[16];
  int baseCount;

  // Base defines (always present)
  defines[0].Name = "SELF_SHADOW";     defines[0].Definition = selfShadowVal;
  defines[1].Name = "ENABLE_SHADOW";   defines[1].Definition = enableShadowVal;
  defines[2].Name = "SHADOWMAP_RESOLUTION"; defines[2].Definition = shadowMapResolutionStr;
  defines[3].Name = "DEBUG_MODE";      defines[3].Definition = debugModeStr;
  baseCount = 4;

  // Depth write shaders: 8 variants (3 bits: SHADOW_ALPHA_TEST, TREE_BENDING, TREE_WOBBLE)
  for ( int i = 0; i < 8; i++ )
  {
    int n = baseCount;
    defines[n].Name = "SHADOW_ALPHA_TEST";
    defines[n].Definition = (i & 1) ? "1" : "0";
    n++;
    defines[n].Name = "TREE_BENDING";
    defines[n].Definition = (i & 2) ? "1" : "0";
    n++;
    defines[n].Name = "TREE_WOBBLE";
    defines[n].Definition = (i & 4) ? "1" : "0";
    n++;
    defines[n].Name = NULL;
    defines[n].Definition = NULL;
    InitVertexShader("shaders/depthwrite.hlsl", &this->depthWriteVertexShaders[i], defines);
    InitPixelShader("shaders/depthwrite.hlsl", &this->depthWritePixelShaders[i], defines);
  }

  // Standard shaders: 7 variants
  for ( int i = 0; i < 7; i++ )
  {
    int n = baseCount;
    defines[n].Name = "SELF_ILLUMINATION";
    defines[n].Definition = (i == 1) ? "1" : "0";
    n++;
    defines[n].Name = "REFLECTION";
    defines[n].Definition = (i == 2) ? "1" : "0";
    n++;
    defines[n].Name = "COLORIZE";
    defines[n].Definition = (i == 3) ? "1" : "0";
    n++;
    defines[n].Name = "TREE_BENDING";
    defines[n].Definition = (i == 4 || i == 6) ? "1" : "0";
    n++;
    defines[n].Name = "TREE_WOBBLE";
    defines[n].Definition = (i == 5 || i == 6) ? "1" : "0";
    n++;
    defines[n].Name = NULL;
    defines[n].Definition = NULL;
    InitVertexShader("shaders/standard.hlsl", &this->standardVertexShaders[i], defines);
    InitPixelShader("shaders/standard.hlsl", &this->standardPixelShaders[i], defines);
    #ifdef HD_DEBUG_SHADERS
    Logger.g->Log(0, "InitShaders: standard[%d] VS=%p PS=%p", i, standardVertexShaders[i].Ptr, standardPixelShaders[i].Ptr);
    #endif
  }

  // Terrain shaders: 2 variants
  for ( int i = 0; i < 2; i++ )
  {
    int n = baseCount;
    defines[n].Name = "SKETCH";
    defines[n].Definition = (i == 1) ? "1" : "0";
    n++;
    defines[n].Name = NULL;
    defines[n].Definition = NULL;
    InitVertexShader("shaders/terrain.hlsl", &this->terrainVertexShader[i], defines);
    InitPixelShader("shaders/terrain.hlsl", &this->terrainPixelShader[i], defines);
  }

  // Base defines with NULL terminator for remaining shaders
  defines[baseCount].Name = NULL;
  defines[baseCount].Definition = NULL;

  /* wireframePS shader removed — not in original binary */

  InitPixelShader("shaders/sepia.hlsl", &this->sepiaPixelShader, NULL);
  InitVertexShader("shaders/terrainlighting.hlsl", &this->terrainLightingVertexShader, defines);
  InitPixelShader("shaders/terrainlighting.hlsl", &this->terrainLightingPixelShader, defines);
  InitVertexShader("shaders/ambientlit.hlsl", &this->ambientLitVertexShader, defines);
  InitPixelShader("shaders/ambientlit.hlsl", &this->ambientLitPixelShader, defines);
  InitVertexShader("shaders/decal.hlsl", &this->decalVertexShader, defines);
  InitPixelShader("shaders/decal.hlsl", &this->decalPixelShader, defines);
  InitVertexShader("shaders/unlitdecal.hlsl", &this->unlitDecalVertexShader, defines);
  InitPixelShader("shaders/unlitdecal.hlsl", &this->unlitDecalPixelShader, defines);
}

//----- (0043F550) --------------------------------------------------------

static bool semaphor = false;
static unsigned int lasttime = 0;
static unsigned int cycles = 0;
static float _scroll = 0.0f;
void SGepard::RenderScene(int a2, bool minimapmode)
{
  HRESULT hr;
  char atmstr[200];

  if ( g_HeadlessMode )
    return;

  if ( semaphor )
  {
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "SGepard::RenderScene: Nested call of RenderScene (skipping)");
    #endif
    return;
  }
  semaphor = true;

  // Update timing
  unsigned int WorldTime = this->WorldTime;
  this->ElapsedTime = WorldTime - this->LastWorldTime;
  this->LastWorldTime = WorldTime;

  // Test cooperative level / device loss
  int coop = this->lpD3DDev->TestCooperativeLevel();
  if ( coop == D3DERR_DEVICELOST || (coop == D3DERR_DEVICENOTRESET && !ResetDevice(a2)) )
    goto done;

  // Pre-render: terrain visibility, hashing, shadow map
  {
    static int rsLogCount = 0;
    static bool loggedWithTerrain = false;
    bool shouldLog = (rsLogCount < 5) || (this->Terrain && !loggedWithTerrain);
    if (shouldLog) {
      #ifdef HD_DEBUG_RENDERER
      Logger.g->Log(0, "RenderScene[%s]: Terrain=%p FOV=%.4f Near=%.4f Far=%.4f OrthoScale=%.4f",
        this->Terrain ? "GAMEPLAY" : "MENU",
        this->Terrain, this->FOV, this->NearPlane, this->FarPlane, this->OrthoScale);
      #endif
      #ifdef HD_DEBUG_RENDERER
      Logger.g->Log(0, "RenderScene[%s]: CamPos=(%.2f,%.2f,%.2f) CamH=%.4f CamV=%.4f",
        this->Terrain ? "GAMEPLAY" : "MENU",
        this->CameraXPos, this->CameraYPos, this->CameraZPos, this->CameraHRot, this->CameraVRot);
      #endif
      #ifdef HD_DEBUG_RENDERER
      Logger.g->Log(0, "RenderScene[%s]: ProjMtx _11=%.4f _22=%.4f _33=%.4f _43=%.4f _34=%.4f",
        this->Terrain ? "GAMEPLAY" : "MENU",
        this->ProjectionMatrix._11, this->ProjectionMatrix._22, this->ProjectionMatrix._33,
        this->ProjectionMatrix._43, this->ProjectionMatrix._34);
      #endif
      #ifdef HD_DEBUG_RENDERER
      Logger.g->Log(0, "RenderScene[%s]: BoardVisible=%d Board=%p Effect=%p",
        this->Terrain ? "GAMEPLAY" : "MENU",
        (int)this->BoardVisible, this->Board, this->Effect);
      #endif
      if (this->Terrain) loggedWithTerrain = true;
      rsLogCount++;
    }
  }
  if ( this->Terrain )
  {
    this->Terrain->CalculateVisibleParcels(&this->CameraMatrix, &this->ProjectionMatrix);
    {
      static int cvpLogCount = 0;
      if (cvpLogCount < 3) {
        int vis = 0;
        for (int i = 0; i < this->Terrain->NumParcels; i++)
          if (this->Terrain->Parcels[i].Visible) vis++;
        #ifdef HD_DEBUG_RENDERER
        Logger.g->Log(0, "CVP result: %d/%d parcels visible, CamMtx _41=%.4f _42=%.4f _43=%.4f",
          vis, this->Terrain->NumParcels,
          this->CameraMatrix._41, this->CameraMatrix._42, this->CameraMatrix._43);
        #endif
        cvpLogCount++;
      }
    }
    if ( !this->ObjectsHashed )
      HashObjects();
    RenderShadowMap(minimapmode);
    if ( this->ShadowQuality == 1 && !minimapmode )
      RegenerateObjectShadowDecals();
  }

  {
  // Resolution scale render target
  IDirect3DSurface9 *old_rt = NULL;
  // Determine clear flags based on actual depth/stencil availability
  int clearFlags = D3DCLEAR_TARGET;
  if ( this->PresentationParameters.EnableAutoDepthStencil )
  {
    clearFlags |= D3DCLEAR_ZBUFFER;
    if ( !minimapmode && this->PresentationParameters.AutoDepthStencilFormat == D3DFMT_D24S8 )
      clearFlags |= D3DCLEAR_STENCIL;
  }
  if ( this->ResolutionScaleSurface.Ptr )
  {
    hr = this->lpD3DDev->GetRenderTarget(0, &old_rt);
    if ( hr )
    {
      sprintf(atmstr, "%s: %s", "SGepard::RenderScene: GetRenderTarget failed", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    hr = this->lpD3DDev->SetRenderTarget(0, this->ResolutionScaleSurface.Ptr);
    if ( hr )
    {
      sprintf(atmstr, "%s: %s", "SGepard::RenderScene: SetRenderTarget failed", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    hr = this->lpD3DDev->SetDepthStencilSurface(this->ResolutionScaleDepthSurface.Ptr);
    if ( hr )
    {
      sprintf(atmstr, "%s: %s", "SGepard::RenderScene: SetDepthStencilSurface failed", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
  }

  // Clear and begin scene
  hr = this->lpD3DDev->Clear(0, NULL, clearFlags, this->FogColor, 1.0f, 0);
  if ( hr && (clearFlags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL)) )
  {
    // Retry with just target clear (driver may not support depth/stencil clear)
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "SGepard::RenderScene: Clear(flags=%d) failed, retrying TARGET-only", clearFlags);
    #endif
    hr = this->lpD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, this->FogColor, 1.0f, 0);
  }
  if ( hr )
  {
    sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::Clear", DXGetErrorStringA(hr));
    Logger.g->Panic(atmstr);
  }
  hr = this->lpD3DDev->BeginScene();
  if ( hr )
  {
    sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::BeginScene", DXGetErrorStringA(hr));
    Logger.g->Panic(atmstr);
  }

  this->PolyCount = 0;
  this->lpD3DDev->SetRenderState(D3DRS_ZENABLE, TRUE);
  EnableFog();
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  this->lpD3DDev->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  this->lpD3DDev->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);

  // Set shadow map texture on stage 2
  IDirect3DTexture9 *shadowTex = this->ShadowMapDepthTexture.Ptr;
  if ( !shadowTex )
    shadowTex = this->ShadowMapTexture.Ptr;
  this->lpD3DDev->SetTexture(2, shadowTex);
  this->lpD3DDev->SetSamplerState(2, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
  this->lpD3DDev->SetSamplerState(2, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
  this->lpD3DDev->SetSamplerState(2, D3DSAMP_BORDERCOLOR, 0xFFFFFFFF);
  this->lpD3DDev->SetSamplerState(2, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
  this->lpD3DDev->SetSamplerState(2, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  this->lpD3DDev->SetSamplerState(2, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);

  // Copy shadow matrix to shadowCB and upload
  memcpy(&this->shadowCB.ViewProjMatrix[0][0], &this->ShadowMatrix._11, 64);
  this->lpD3DDev->SetVertexShaderConstantF(12, (const float *)&this->shadowCB, this->shadowCB.Vector4fCount);

  // Set up light constant buffer
  float FogStart = this->FogStart;
  float FogEnd = this->FogEnd;
  this->lightCB.Ambient[0] = this->AmbientColorVal.r;
  this->lightCB.Ambient[1] = this->AmbientColorVal.g;
  this->lightCB.Ambient[2] = this->AmbientColorVal.b;
  this->lightCB.Ambient[3] = 1.0f;
  this->lightCB.BottomAmbient[0] = this->BottomAmbientColorVal.r;
  this->lightCB.BottomAmbient[1] = this->BottomAmbientColorVal.g;
  this->lightCB.BottomAmbient[2] = this->BottomAmbientColorVal.b;
  this->lightCB.BottomAmbient[3] = 1.0f;
  this->lightCB.SunlightDir[0] = this->SunDir.x;
  this->lightCB.SunlightDir[1] = this->SunDir.y;
  this->lightCB.SunlightDir[2] = this->SunDir.z;
  this->lightCB.SunlightDir[3] = 0.0f;
  this->lightCB.SunlightColor[0] = this->SunColorVal.r;
  this->lightCB.SunlightColor[1] = this->SunColorVal.g;
  this->lightCB.SunlightColor[2] = this->SunColorVal.b;
  this->lightCB.SunlightColor[3] = 1.0f;
  if ( FogEnd <= FogStart )
  {
    this->lightCB.FogParams[0] = 0.0f;
    this->lightCB.FogParams[1] = 0.0f;
  }
  else
  {
    float fogRange = FogStart - FogEnd; // negative
    this->lightCB.FogParams[0] = 1.0f / fogRange;
    this->lightCB.FogParams[1] = -(FogEnd / fogRange);
  }
  this->lightCB.FogParams[2] = 0.0f;
  this->lightCB.FogParams[3] = 0.0f;
  this->lpD3DDev->SetVertexShaderConstantF(16, (const float *)&this->lightCB, this->lightCB.Vector4fCount);

  // Draw terrain
  if ( this->Terrain )
  {
    int polyBefore = this->PolyCount;
    this->Terrain->Draw();

    // Stencil shadow box
    if ( this->ShadowQuality >= 2 )
      RenderShadowBoxToStencil();

    // Terrain shaders (rects)
    if ( !minimapmode && !this->Terrain->GetCompactMode() && !this->Flags[1] )
    {
      SMaterial rectMat;
      rectMat.Diffuse.r = 1.0f; rectMat.Diffuse.g = 1.0f; rectMat.Diffuse.b = 1.0f; rectMat.Diffuse.a = 1.0f;
      rectMat.Ambient.r = 1.0f; rectMat.Ambient.g = 1.0f; rectMat.Ambient.b = 1.0f; rectMat.Ambient.a = 1.0f;
      SetMaterial(&rectMat);
      this->lpD3DDev->SetTransform(D3DTS_WORLD, &this->IdentityMatrix);

      for ( int idx = 0; idx < this->Rects.size; idx++ )
      {
        if ( this->Rects.array[idx].use == 0x7FFFFFFF )
        {
          this->lpD3DDev->SetRenderState(D3DRS_AMBIENT, this->Rects.array[idx].data.Color);
          this->Rects.array[idx].data.Mesh->Draw(1);
        }
      }
    }

    // Terrain shader infos
    this->lpD3DDev->SetTransform(D3DTS_WORLD, &this->IdentityMatrix);
    if ( !this->Terrain->GetCompactMode() )
    {
      this->ShaderInfos.current = this->ShaderInfos.first;
      this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
      this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
      this->lpD3DDev->SetVertexShader(this->terrainVertexShader[0].Ptr);
      this->lpD3DDev->SetPixelShader(this->terrainPixelShader[0].Ptr);
      SetWorldViewProjVertexShaderConstantBuffer(&this->IdentityMatrix);
      SetLightingType(LT_NORMAL);
      for ( SShaderInfo *si = this->ShaderInfos.current; si; si = this->ShaderInfos.current )
      {
        // Editor decal feedback: yellow tint when actively selected, white
        // tint when only hovered (selected wins if both apply). The shipped
        // editor only had one state (cyan-ish 0x4040FFFF for selected); the
        // recompile splits it so the user can tell hover from click.
        if ( si == this->SelectedShader || si == this->HoverShader )
        {
          unsigned int fogColor = (si == this->SelectedShader) ? 0x40FFFF00u  // yellow (active)
                                                               : 0x40FFFFFFu; // white  (hover)
          this->lpD3DDev->SetRenderState(D3DRS_RANGEFOGENABLE, TRUE);
          this->lpD3DDev->SetRenderState(D3DRS_FOGCOLOR, fogColor);
          this->lpD3DDev->SetRenderState(D3DRS_FOGVERTEXMODE, D3DFOG_LINEAR);
          // 0xC4480000 = -800.0f (FOGSTART) ; 0x44FA0000 = 2000.0f (FOGEND).
          float fogStart = -800.0f, fogEnd = 2000.0f;
          this->lpD3DDev->SetRenderState(D3DRS_FOGSTART, *(DWORD*)&fogStart);
          this->lpD3DDev->SetRenderState(D3DRS_FOGEND,   *(DWORD*)&fogEnd);
          this->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, TRUE);
          // The terrain vertex shader (active here for the decal pass)
          // computes oFog as length(viewPos) * lcb.FogParams.x +
          // lcb.FogParams.y reading c20. The FFP FOGSTART/END above are
          // ignored when oFog comes from the VS, so without overriding
          // c20 the tint blend uses main-map fog params and is invisible
          // up close. Match the FFP setup.
          float fogRange = fogStart - fogEnd;  // -2800
          float vsFogParams[4] = { 1.0f / fogRange, -(fogEnd / fogRange), 0.0f, 0.0f };
          this->lpD3DDev->SetVertexShaderConstantF(20, vsFogParams, 1);
        }
        else
        {
          EnableFog();  // restores FFP fog AND c20 from main map fog
        }
        SetTexture(0, this->ShaderInfos.current->ShaderTHandle, 1);
        this->ShaderInfos.current->mesh->Draw(0, 0, 0.0f);
        this->ShaderInfos.StepToNext();
      }
      this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
      this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
      this->lpD3DDev->SetVertexShader(NULL);
      this->lpD3DDev->SetPixelShader(NULL);
      EnableFog();
    }

    this->Terrain->DrawLighting(a2);
    this->Terrain->DrawLimits();
    if ( !minimapmode )
      RenderGroundTrails();
  }
  SetLightingType(LT_NORMAL);
  {
    SMaterial mat;
    SetMaterial(&mat);
  }
  this->lpD3DDev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
  this->lpD3DDev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
  SetTexture(0, -1, 1);
  RenderObjects(minimapmode, false);
  // HDB_PREVIEW_GHOST: editor placement ghost. Drawn after opaque objects so
  // it z-tests against terrain + units; ZWRITE off so the alpha pass doesn't
  // poison depth for water/effects/UI passes that follow. State block isolates
  // all D3D state so render-state mutations (alpha-blend, fog tint, etc.)
  // don't leak into subsequent frames. Mirrors market.cpp's preview pattern.
  if ( this->EditorPreviewGhost && !minimapmode )
  {
    SGroup *gh = (SGroup *)this->EditorPreviewGhost;
    // One-shot per pointer change so we can confirm the render branch fires.
    static SIObject *lastLoggedGhost = (SIObject *)1;
    if ( lastLoggedGhost != this->EditorPreviewGhost )
    {
      Logger.g->Log(0, "PreviewGhost: render pass pos=(%.1f,%.1f,%.1f) meshes=%d visible=%d",
        this->EditorPreviewGhostX, this->EditorPreviewGhostY, this->EditorPreviewGhostZ,
        gh->MeshArray.size, (int)gh->Visible);
      lastLoggedGhost = this->EditorPreviewGhost;
    }
    // Phase 1: prove the ghost is visible by drawing it OPAQUELY with the
    // editor's white-hover tint. Transparency is deferred to phase 2 once
    // we confirm the geometry shows up at all — the per-material SetTexture
    // calls inside SMesh::Draw rebuild ALPHABLENDENABLE based on each texture's
    // alpha type, wiping out any blend-factor setup we apply before Draw().
    int savedSelection = gh->Selection;
    gh->Selection = 2;  // SelectionFogFlat(0xFFFFFFFF, 0.45) inside Draw
    gh->Precalculate();
    gh->Draw();
    gh->Selection = savedSelection;
  }
  EnableFog();
  // Scene2
  SetLightingType(LT_NORMAL);
  {
    SMaterial mat2;
    SetMaterial(&mat2);
  }
  this->lpD3DDev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
  this->lpD3DDev->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
  RenderScene2(a2, minimapmode);

  // Debug lines
  if ( this->Terrain && this->Terrain->GetBlockMapMode() )
    RenderDebugLines();

  // Smoke trails
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  RenderSmokeTrails();
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);

  // FPS counter
  if ( this->FPSTextFrame >= 0 )
  {
    unsigned int newtime = timeGetTime();
    unsigned int elapsed = newtime - lasttime;
    ++cycles;
    if ( elapsed > 1000 )
    {
      char fpsStr[256];
      int polyCount = this->PolyCount;
      double fps = 1000.0 / (double)elapsed * (double)cycles;
      if ( polyCount <= 1000 )
        sprintf(fpsStr, "FPS: %8.2f  Polygons: %d", fps, polyCount);
      else
        sprintf(fpsStr, "FPS: %8.2f  Polygons: %d %03d", fps, polyCount / 1000, polyCount % 1000);
      this->Board->SetText(this->FPSTextFrame, this->FPSFont, 0, fpsStr);
      lasttime = newtime;
      cycles = 0;
    }
  }

  // Debug info overlay: texture memory, tiles, camera position/direction.
  // Gated by ShowDebugInfo runtime flag (editor sets it true; game leaves it
  // false in release). Only paints when a map is loaded (Terrain non-null).
  if ( this->ShowDebugInfo && this->DebugInfoFrame >= 0 && this->Terrain )
  {
    float texMb = 0.0f;
    for (int ti = 0; ti < this->Textures.size; ti++) {
      if (this->Textures.array[ti].use == 0x7FFFFFFF)
        texMb += (float)(this->Textures.array[ti].data.Width * this->Textures.array[ti].data.Height * 4);
    }
    texMb /= (1024.0f * 1024.0f);

    int tiles = this->Terrain ? this->Terrain->NumParcels : 0;

    float deg = fmodf(this->CameraHRot * (180.0f / 3.14159265f), 360.0f);
    if (deg < 0.0f) deg += 360.0f;
    const char* compass;
    if      (deg < 22.5f  || deg >= 337.5f) compass = "North";
    else if (deg < 67.5f)  compass = "NorthEast";
    else if (deg < 112.5f) compass = "East";
    else if (deg < 157.5f) compass = "SouthEast";
    else if (deg < 202.5f) compass = "South";
    else if (deg < 247.5f) compass = "SouthWest";
    else if (deg < 292.5f) compass = "West";
    else                    compass = "NorthWest";

    char dbgbuf[512];
    sprintf(dbgbuf, "Textures: %.1f Mb  Tiles: %d  CameraDir: %.2f %s    X: %.4f, Z: %.4f, Y: %.4f",
      texMb, tiles, deg, compass,
      this->DebugMouseX, this->DebugMouseZ, this->DebugMouseY);
    this->Board->SetText(this->DebugInfoFrame, this->FPSFont, 0, dbgbuf);
  }

  // Water drawing
  float scrollVal = _scroll;
  // Game ticks ElapsedTime via SGepard::AdvanceTime in SGameView. The editor
  // doesn't call AdvanceTime, so ElapsedTime stays 0 there and water never
  // animates. The original editor.exe handled this with its own Editor_LastTime
  // member fed by STimer::GetTickValue (editorSplit/sgepard.c:17738-17747);
  // mirror that fallback so river/waterfall scrolling matches the original.
  unsigned int delta = this->ElapsedTime;
  if ( delta == 0 )
  {
    unsigned int now = (unsigned int)Timer.GetTickValue();
    if ( this->Editor_LastTime )
    {
      delta = now - this->Editor_LastTime;
      if ( delta > 1000 ) delta = 1000;  // clamp matches original (>0x3E8)
    }
    this->Editor_LastTime = now;
  }
  double elapsedD = (double)delta;
  this->Temp = 0;
  scrollVal -= (float)((float)(elapsedD * 0.001) * 0.1f);
  _scroll = scrollVal;

  this->HillChain.current = this->HillChain.first;
  for ( SHillRing *hill = this->HillChain.current; hill; hill = this->HillChain.current )
  {
    if ( !hill->Type && hill->WaterEnabled )
    {
      DrawWater(hill, 0, scrollVal, NULL);
      ++this->Temp;
      scrollVal = _scroll;
    }
    // StepToNext
    SHillRing *cur = this->HillChain.current;
    if ( this->HillChain.Closed )
    {
      if ( !cur )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      SHillRing *next = cur->next;
      if ( !next )
        next = this->HillChain.first;
      this->HillChain.current = next;
    }
    else
    {
      this->HillChain.current = cur ? cur->next : NULL;
    }
  }

  RenderLakes();
  // Effects
  if ( !minimapmode && this->Effect )
    this->Effect->MoveEffects(NULL, 0);

  // Resolution scale blit-back
  if ( this->ResolutionScaleSurface.Ptr )
  {
    hr = this->lpD3DDev->SetRenderTarget(0, old_rt);
    if ( hr )
    {
      sprintf(atmstr, "%s: %s", "SGepard::RenderScene: SetRenderTarget failed", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    hr = this->lpD3DDev->SetDepthStencilSurface(NULL);
    if ( hr )
    {
      sprintf(atmstr, "%s: %s", "SGepard::RenderScene: SetDepthStencilSurface failed", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    hr = this->lpD3DDev->StretchRect(this->ResolutionScaleSurface.Ptr, NULL, old_rt, NULL, D3DTEXF_LINEAR);
    if ( hr )
    {
      sprintf(atmstr, "%s: %s", "SGepard::RenderScene: StretchRect failed", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    if ( old_rt )
    {
      old_rt->Release();
      old_rt = NULL;
    }
  }

  // Board — Board::Render sets up its own 2D render states internally
  if ( !minimapmode && this->BoardVisible )
    this->Board->Render(0.0f, a2, 0);

  // Editor diagnostic overlay (debug picker rects/discs). No-op in shipped
  // builds and when toggle is off. Defined in world.cpp; resolved at link
  // time when world is linked into editor.exe / swineHD.exe.
  if ( !minimapmode )
    DrawDebugPickerOverlayFromGepard(this->lpD3DDev);

  this->lpD3DDev->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  hr = this->lpD3DDev->EndScene();
  if ( hr )
  {
    sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::EndScene", DXGetErrorStringA(hr));
    Logger.g->Panic(atmstr);
  }

  // Screenshot
  if ( this->ScreenshotFile )
  {
    IDirect3DSurface9 *bbuf = NULL;
    hr = this->lpD3DDev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bbuf);
    if ( hr )
    {
      sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::GetBackBuffer", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    SSurfaceBitmap surface(bbuf);
    surface.SaveTGA((char *)this->ScreenshotFile, 0);
    surface.~SSurfaceBitmap();
    if ( bbuf )
    {
      bbuf->Release();
      bbuf = NULL;
    }
    this->ScreenshotFile = NULL;
  }

  // Present
  if ( !minimapmode )
    this->lpD3DDev->Present(NULL, NULL, NULL, NULL);
  ++this->FrameCount;
  }

done:
  semaphor = false;
}

//----- (0043EE30) --------------------------------------------------------

void SGepard::RenderRects()
{
  SMaterial mat;
  SetMaterial(&mat);
  this->lpD3DDev->SetTransform((D3DTRANSFORMSTATETYPE)256, &this->IdentityMatrix);

  int i = -1;
  for (;;)
  {
    int size = this->Rects.size;
    if (++i >= size)
      break;
    auto* j = &this->Rects.array[i];
    while (j->use != 0x7FFFFFFF)
    {
      if (++i >= size)
        return;
      ++j;
    }
    if (i < 0)
      break;
    HRESULT v5 = this->lpD3DDev->SetRenderState(D3DRS_AMBIENT, this->Rects.array[i].data.Color);
    if (v5)
    {
      const char* v6 = DXGetErrorStringA(v5);
      char _Buffer[200];
      sprintf(_Buffer, "%s: %s", "SGepard::SetAmbientColor: IDirect3DDevice7::SetRenderState (D3DRS_AMBIENT)", v6);
      Logger.g->Panic(_Buffer);
    }
    this->Rects.array[i].data.Mesh->Draw(1);
  }
}

//----- (0043EF60) --------------------------------------------------------

void SGepard::RenderScene2(int a2, bool minimapmode)
{
  // Identity world matrix
  D3DXMATRIX d3dm;
  D3DXMatrixIdentity(&d3dm);
  this->lpD3DDev->SetTransform(D3DTS_WORLD, &d3dm);

  SetTexture(0, 2, 1);

  // Draw hills
  this->HillChain.current = this->HillChain.first;
  for ( SHillRing *hill = this->HillChain.current; hill; hill = this->HillChain.current )
  {
    if ( hill->HillMesh )
    {
      hill->HillMesh->Draw(0, 0, 0.0f);
      hill = this->HillChain.current;
    }
    if ( hill->HillMesh2 )
      hill->HillMesh2->Draw(0, 0, 0.0f);

    // StepToNext
    SHillRing *cur = this->HillChain.current;
    if ( this->HillChain.Closed )
    {
      if ( !cur )
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      SHillRing *next = cur->next;
      if ( !next )
        next = this->HillChain.first;
      this->HillChain.current = next;
    }
    else
    {
      this->HillChain.current = cur ? cur->next : NULL;
    }
  }

  // Decals
  RenderDecals(1);
  RenderDecals(0);

  // Dynamic shaders
  if ( !minimapmode && this->Terrain && !this->Flags[1] )
    RenderDynamicShaders();

  // Fog and shadow decals
  EnableFog();
  if ( !minimapmode && !this->FullBright && this->ShadowQuality > 0 )
    RenderObjectShadowDecals();

  // Reset sampler states
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  SetTexture(0, -1, 1);

  // Cleanup
  this->NodeInfos.DeleteAll();
  this->DrawType = DT_NORMAL;
  SetLightingType(LT_NORMAL);

  // Spline-mode overlay (editor only). Toggled by SetSplineDisplay from the
  // editor's spline tab. Renders sphere markers at every node and colored
  // lines along every spline. Mirrors editorSplit/sgepard.c:17060-17284.
  if ( this->SplineDisplay )
  {
    // Pass 1 — node markers. For non-selected hills, draw only the canonical
    // ring (idx = Type==1 ? 0 : 1). For the selected hill, draw every ring
    // so the user can interact with all of them.
    for ( SHillRing *hill = this->HillChain.first; hill; hill = hill->next )
    {
      int startIdx = (hill->Type != 1) ? 1 : 0;
      int numItems = hill->SplineChain ? hill->SplineChain->NumItems : 0;
      int endIdx = (hill == this->gpSelHillRing) ? numItems : (startIdx + 1);
      if ( endIdx > numItems ) endIdx = numItems;

      for ( int idx = startIdx; idx < endIdx; ++idx )
      {
        SSplineRing *sr = hill->SplineChain->first;
        for ( int k = 0; k < idx && sr; ++k ) sr = sr->next;
        if ( !sr || !sr->spline || !sr->spline->VisibilityState ) continue;
        bool top = (idx + 1 == numItems);
        sr->spline->DrawNodes(top, hill, sr);
      }
    }

    // Pass 2 — spline lines. Default is white; the selected spline is drawn
    // in gray and leaves the ambient state at red so any splines after it in
    // the chain render red — visual hint about selection ordering.
    {
      SMaterial defMat;
      defMat.Diffuse.r = defMat.Diffuse.g = defMat.Diffuse.b = defMat.Diffuse.a = 0.0f;
      SetMaterial(&defMat);
      this->lpD3DDev->SetRenderState(D3DRS_AMBIENT, 0xFFFFFFFF);
    }

    for ( SHillRing *hill = this->HillChain.first; hill; hill = hill->next )
    {
      int startIdx = (hill->Type != 1) ? 1 : 0;
      int numItems = hill->SplineChain ? hill->SplineChain->NumItems : 0;
      int endIdx = (hill == this->gpSelHillRing) ? numItems : (startIdx + 1);
      if ( endIdx > numItems ) endIdx = numItems;

      for ( int idx = startIdx; idx < endIdx; ++idx )
      {
        SSplineRing *sr = hill->SplineChain->first;
        for ( int k = 0; k < idx && sr; ++k ) sr = sr->next;
        if ( !sr || !sr->spline || !sr->spline->VisibilityState ) continue;

        SSplineRing *sel = this->gpSelSplineRing;
        if ( sel && sr->spline == sel->spline )
        {
          SMaterial selMat;
          selMat.Diffuse.r = selMat.Diffuse.g = selMat.Diffuse.b = selMat.Diffuse.a = 0.0f;
          SetMaterial(&selMat);
          this->lpD3DDev->SetRenderState(D3DRS_AMBIENT, 0x00808080);  // gray
          sr->spline->DrawLines();
          SetMaterial(&selMat);
          this->lpD3DDev->SetRenderState(D3DRS_AMBIENT, 0x00FF0000);  // red
        }
        else
        {
          sr->spline->DrawLines();
        }
      }
    }
  }
}

// ============================================================
// SIGepard virtual override stubs
// These bridge the SIGepard pure virtual signatures to the existing
// SGepard implementations which have slightly different parameter
// types/counts due to IDA __usercall artifacts.
// ============================================================

void SGepard::SetMode(bool isfullscreen, bool vsync, int msaa, int width, int height)
{
  this->SetMode(0, isfullscreen, vsync, (D3DMULTISAMPLE_TYPE)msaa, width, height);
}

void SGepard::SetResolutionScale(float scale)
{
  this->SGepard::SetResolutionScale(0, scale);
}

void SGepard::Resize(unsigned int width, unsigned int height)
{
  this->SGepard::Resize(0, width, height);
}

void SGepard::RenderScene(bool minimapmode)
{
  this->SGepard::RenderScene(0, minimapmode);
}

//----- (0043BCD0) --------------------------------------------------------
void SGepard::RenderBoardOnly()
{
  if ( g_HeadlessMode )
    return;
  if (semaphor)
  {
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "SGepard::RenderBoardOnly: Nested call (skipping)");
    #endif
    return;
  }
  semaphor = 1;
  HRESULT hr = lpD3DDev->TestCooperativeLevel();
  if (hr != D3DERR_DEVICELOST && (hr != D3DERR_DEVICENOTRESET || ResetDevice(0))) {
    DWORD flags = ResolutionScaleSurface.Ptr ? D3DCLEAR_TARGET : (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL);
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "RenderBoardOnly: Clear flags=%d ResScaleSurf=%p", (int)flags, ResolutionScaleSurface.Ptr);
    #endif
    hr = lpD3DDev->Clear(0, nullptr, flags, 0, 1.0f, 0);
    if (FAILED(hr)) {
      char atmstr[200];
      sprintf(atmstr, "%s: %s", "SGepard::RenderBoardOnly\\IDirect3DDevice::Clear", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    hr = lpD3DDev->BeginScene();
    if (FAILED(hr)) {
      char atmstr[200];
      sprintf(atmstr, "%s: %s", "SGepard::RenderBoardOnly\\IDirect3DDevice::BeginScene", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    Board->Render(0.0f, 0, (int)this);
    hr = lpD3DDev->EndScene();
    if (FAILED(hr)) {
      char atmstr[200];
      sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::EndScene", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    lpD3DDev->Present(nullptr, nullptr, nullptr, nullptr);
  }
  semaphor = 0;
}

void SGepard::SetSunLight(float r, float g, float b, float direction, float elevation)
{
  this->SGepard::SetSunLight(0, 0, r, g, b, direction, elevation);
}

// TrackGroundTrail virtual override — implementation is at address 00444C40 above

void SGepard::MakeScreenshot(int w, int h, char* filename, unsigned int flags)
{
  this->SGepard::MakeScreenshot((unsigned int)w, (unsigned int)h, filename, flags);
}

void SGepard::SetLightPosition(int light, float x, float y, float z)
{
  this->SGepard::SetLightPosition((unsigned int)light, x, y, z);
}

void SGepard::DestroyLight(int light)
{
  this->SGepard::DestroyLight((unsigned int)light);
}

int SGepard::CreatePointLight(float x, float y, float z, float r, float g, float b, float range, float atten)
{
  return this->SGepard::CreatePointLight(x, y, z, r, g, b, range, (unsigned int)(int)atten);
}

void SGepard::SaveSplines(SStream* stream, bool flag)
{
  this->SGepard::SaveSplines(stream, (unsigned int)flag);
}

void SGepard::CreateSpline(SHillRing* hill, SSplineRing** spline, float size, int type)
{
  this->SGepard::CreateSpline(hill, spline, (int)size, type);
}

void SGepard::SetInterpolation(double interp)
{
  this->SGepard::SetInterpolation((long double)interp);
}

SIObject* SGepard::CreateObject(const char* filename, float scale, bool flag)
{
  return this->SGepard::CreateObject(filename, scale, (BOOL)flag);
}

SIObject* SGepard::CreateObjectByIndex(int index, bool dynamic)
{
  return (SIObject*)this->SGepard::CreateObject(index, dynamic);
}

void SGepard::ReplaceObjectByIndex(SIObject* obj, int index)
{
  this->SGepard::ReplaceObject(obj, index);
}

SShaderInfo* SGepard::CreateShader2(float x, float z, const char* filename, int type, bool a, bool b)
{
  return this->SGepard::CreateShader(x, z, filename, type, a, b);
}

SShader2Info* SGepard::CreateShader2Info(float x, float z, int texture, int type, SDrawType dt, bool a, bool b, bool c)
{
  return this->SGepard::CreateShader2(x, z, texture, type, dt, a, b, c);
}

// Empty in original binary (confirmed via IDA + Ghidra on both game and editor PDBs)
void SGepard::DrawLine(float x1, float y1, float z1, float x2, float y2, float z2)
{
}

// Empty in original binary (confirmed via IDA + Ghidra on both game and editor PDBs)
void SGepard::RedrawHill(SHillRing* hill)
{
}

void SGepard::PlayEffectStr(int effect, SIObject* obj, char* str, int val)
{
  PlayEffect(effect, obj, str, val);
}

void* SGepard::PlayEffectPos(int effect, float x, float y, float z, float scale)
{
  return PlayEffect(effect, x, y, z, scale);
}

void SGepard::PlayEffectFull(int effect, float x, float y, float z, float a, float b, float c, float d, float e, float f, float g)
{
  PlayEffect(effect, x, y, z, a, b, c, d, e, f, g);
}

// =============================================================================
// Stub implementations for unresolved SGepard methods
// =============================================================================

// --- Simple getters (virtual) ---

bool SGepard::GetFPSEnabled()
{
  return FPSTextFrame >= 0;
}

float SGepard::GetCameraHRot()
{
  return CameraHRot;
}

float SGepard::GetFOV()
{
  return FOV;
}

float SGepard::GetShadowDistance()
{
  return ShadowDistance;
}

int SGepard::GetShadowQuality()
{
  return ShadowQuality;
}

int SGepard::GetMaxShadowQuality()
{
  if (Flags[0])
    return MaxShadowQuality;
  else
    return 1;
}

int SGepard::GetTextureDetail()
{
  return TextureDetail;
}

int SGepard::GetTextureFilter()
{
  return TextureFilter;
}

int SGepard::GetTileset()
{
  return Tileset;
}

int SGepard::GetDebugTextFrame()
{
  return DebugTextFrame;
}

unsigned int SGepard::GetAnimElapsedTime()
{
  return AnimElapsedTime;
}

bool SGepard::IsShaderSelected()
{
  return SelectedShader != nullptr;
}

SShaderInfo* SGepard::GetSelectedShader()
{
  return SelectedShader;
}

void SGepard::GetResolution(int *w, int *h)
{
  *w = ViewWidth;
  *h = ViewHeight;
}

//----- (004335D0) --------------------------------------------------------
int SGepard::GetHillSideFaceCount(SHillRing *_hillring)
{
  SChain<SSplineRing> *chain = _hillring->SplineChain;
  if (chain->Closed && !chain->first)
    Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
  return (chain->NumItems - 1) * (2 * chain->first->spline->Nodes.NumItems - 2);
}

//----- (00433610) --------------------------------------------------------
int SGepard::GetHillSideVertexCount(SHillRing *_hillring)
{
  SChain<SSplineRing> *chain = _hillring->SplineChain;
  if (chain->Closed && !chain->first)
    Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
  return chain->NumItems * chain->first->spline->Nodes.NumItems;
}

//----- (00433780) --------------------------------------------------------
unsigned int SGepard::GetLightHandle()
{
  unsigned int result = 0;
  SLightProp *light = LightProps;
  while (light->type) {
    if ((int)++result >= 4)
      Logger.g->Panic("SGepard::GetLightHandle: Fatal error: No more light avaiable! Exiting...");
    ++light;
  }
  return result;
}

SVector* SGepard::GetCameraForward(SVector *result)
{
  result->x = CameraMatrix._13;
  result->y = CameraMatrix._23;
  result->z = CameraMatrix._33;
  return result;
}

// --- Texture queries ---

bool SGepard::IsTextureOpaque(int idx)
{
  if (idx >= 0 && idx < Textures.size) {
    if (Textures.array[idx].use == 0x7FFFFFFF) {
      if (Textures.array[idx].data.Alpha)
        return false;
    }
  }
  return true;
}

int SGepard::GetTextureAlpha(int idx)
{
  if (idx >= 0 && idx < Textures.size && Textures.array[idx].use == 0x7FFFFFFF)
    return Textures.array[idx].data.Alpha;
  return 0;
}

// --- EnableFPS ---

void SGepard::EnableFPS(bool enable, int font, int parentFrame)
{
  if (enable) {
    if (FPSTextFrame < 0)
      FPSTextFrame = Board->CreateFrame(FT_TEXT, parentFrame, 8, 48, 0, 0);
    FPSFont = font;
    if (ShowDebugInfo && DebugInfoFrame < 0)
      DebugInfoFrame = Board->CreateFrame(FT_TEXT, parentFrame, 8, 64, 0, 0);
  } else {
    if (FPSTextFrame >= 0) {
      Board->DestroyFrame(FPSTextFrame);
      FPSTextFrame = -1;
    }
    if (DebugInfoFrame >= 0) {
      Board->DestroyFrame(DebugInfoFrame);
      DebugInfoFrame = -1;
    }
  }
}

// --- EnableFog ---

void SGepard::EnableFog()
{
  if (FogEnd <= FogStart) {
    lpD3DDev->SetRenderState(D3DRS_FOGENABLE, FALSE);
  } else {
    lpD3DDev->SetRenderState(D3DRS_RANGEFOGENABLE, 1);
    lpD3DDev->SetRenderState(D3DRS_FOGCOLOR, FogColor);
    lpD3DDev->SetRenderState(D3DRS_FOGVERTEXMODE, 3);
    lpD3DDev->SetRenderState(D3DRS_FOGSTART, *(DWORD*)&FogStart);
    lpD3DDev->SetRenderState(D3DRS_FOGEND, *(DWORD*)&FogEnd);
    lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 1);
  }

  // Restore vertex-shader fog params (c20) from the main map fog so
  // anything drawn after a selection-fog override gets normal fog
  // behaviour. Mirrors the math in SetLightCB.
  float vsFogParams[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
  if (FogEnd > FogStart) {
    float fogRange = FogStart - FogEnd;
    vsFogParams[0] = 1.0f / fogRange;
    vsFogParams[1] = -(FogEnd / fogRange);
  }
  this->lpD3DDev->SetVertexShaderConstantF(20, vsFogParams, 1);
}

// --- Release ---

void SGepard::Release()
{
  #ifdef HD_DEBUG_RENDERER
  Logger.g->Log(0, "SGepard::Release: RefCount=%d", RefCount);
  #endif
  if (RefCount-- == 1) {
    this->~SGepard();
    operator delete(this);
  }
}

// --- ReleaseTexture ---

void SGepard::ReleaseTexture(int idx, bool noremove)
{
  if (idx >= 0 && idx < Textures.size && Textures.array[idx].use == 0x7FFFFFFF) {
    int rc = --Textures.array[idx].data.RefCount;
    if (!noremove && !rc) {
      char *fn = Textures.array[idx].data.FileName;
      if (fn) {
        free(fn);
        Textures.array[idx].data.FileName = 0;
      }
      IDirect3DTexture9 *tex = Textures.array[idx].data.lpTexture;
      if (tex) {
        tex->Release();
        Textures.array[idx].data.lpTexture = 0;
      }
      Textures.Remove(idx);
    }
  } else if (idx != -1) {
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "SGepard::ReleaseTexture: Invalid texture index %d (size=%d) — skipping", idx, Textures.size);
    #endif
  }
}

// --- ReleaseMesh ---

void SGepard::ReleaseMesh(SHillRing *_hillring)
{
  if (_hillring) {
    SMesh *m = _hillring->HillMesh;
    if (m)
      ((SIObject*)m)->Release();
  }
}

// --- LoadTexture ---

int SGepard::LoadTexture(const char *filename, bool mipmaps, bool convert)
{
  if (!filename || !strlen(filename))
    return -1;

  // Convert .bmp extension to .tga if convert flag set
  char *fn = const_cast<char*>(filename);
  if (convert) {
    char *dot = strrchr(fn, '.');
    if (dot && !_stricmp(dot, ".bmp"))
      strcpy(dot, ".tga");
  }

  // Search for existing texture with same filename
  int size = Textures.size;
  int idx = -1;
  while (++idx < size) {
    if (Textures.array[idx].use != 0x7FFFFFFF) continue;
    if (idx < 0) break;
    if (strcmp(filename, Textures.array[idx].data.FileName) == 0) {
      ++Textures.array[idx].data.RefCount;
      return idx;
    }
  }

  // Build DXT filename
  char dxt_filename[260];
  strcpy(dxt_filename, filename);
  strcpy(strrchr(dxt_filename, '.'), ".dxt");

  struct _stat stat_orig, stat_dxt;
  if (FileSystem.Stat(filename, &stat_orig)) {
    // Original file not found — try DXT only
    if (!FileSystem.Stat(dxt_filename, &stat_dxt))
      return CreateTextureFromDXT(dxt_filename, filename, mipmaps);
    return -1;
  }

  // Original exists — prefer DXT if timestamps match
  if (!FileSystem.Stat(dxt_filename, &stat_dxt)
    && abs(stat_orig.st_mtime - stat_dxt.st_mtime) <= 2) {
    return CreateTextureFromDXT(dxt_filename, filename, mipmaps);
  }

  Logger.g->Log(0, "NO COMPRESSED TEXTURE FOR: %s", filename);
  SBitmap bmap;
  if (bmap.LoadTGA(const_cast<char*>(filename), 0))
    return CreateTextureFromBitmap(filename, &bmap, mipmaps);
  return -1;
}

#ifdef HDB_MISSING_ASSET_FALLBACK
int SGepard::LoadTextureOrPlaceholder(const char *filename, bool mipmaps, bool convert)
{
  int h = LoadTexture(filename, mipmaps, convert);
  if (h != -1) return h;
  Logger.g->Log(0, "MISSING TEXTURE: %s -> editor/missing.dxt", filename);
  return LoadTexture("editor/missing.dxt", mipmaps, convert);
}
#endif

// --- LoadEffect ---

int SGepard::LoadEffect(char *effectname, const char *texturename)
{
  if (Effect)
    return Effect->LoadEffect(effectname, texturename, -1.0f, -1.0f);
  return -1;
}

// --- PrecacheObject ---

int SGepard::PrecacheObject(const char *filename, float scale, SDrawType drawtype)
{
  // Search for existing cached object with same filename and scale
  int size = ObjectCache.size;
  int idx = -1;
  while (++idx < size) {
    if (ObjectCache.array[idx].use != 0x7FFFFFFF) continue;
    if (idx < 0) break;
    if (strcmp(filename, ObjectCache.array[idx].data.FileName) == 0
        && ObjectCache.array[idx].data.Scale == scale)
      return idx;
  }

  // Create new group and load the 4D file
  SGroup *group = new SGroup(this, drawtype);
  if (!group) return -1;

  // Find path prefix (last slash)
  const char *lastSlash = nullptr;
  const char *p = filename;
  while (*p) {
    if (*p == '\\' || *p == '/')
      lastSlash = p;
    ++p;
  }

  bool loaded;
  if (lastSlash) {
    char prefix[260];
    memset(prefix, 0, sizeof(prefix));
    strncpy(prefix, filename, lastSlash - filename + 1);
    loaded = group->Load4DFile(prefix, filename, scale);
  } else {
    loaded = group->Load4DFile(nullptr, filename, scale);
  }
  if (!loaded) {
    ((SIObject*)group)->Release();
#ifdef HDB_MISSING_ASSET_FALLBACK
    // Retry once with editor/missing.4d before giving up. Covers cases
    // where Load4DFile fails for reasons other than a missing top-level
    // file (e.g. corrupt header) that the OpenRead-level fallback in
    // stream.cpp can't catch. Cache under the original filename so
    // callers see a valid handle.
    if (strcmp(filename, "editor/missing.4d") != 0) {
      Logger.g->Log(0, "MISSING MODEL: %s -> editor/missing.4d", filename);
      SGroup *fb = new SGroup(this, drawtype);
      if (fb && fb->Load4DFile("editor/", "editor/missing.4d", scale)) {
        int newIdx = ObjectCache.Add();
        ObjectCache.array[newIdx].data.FileName = _strdup(filename);
        ObjectCache.array[newIdx].data.Scale = scale;
        ObjectCache.array[newIdx].data.RefCount = 0;
        ObjectCache.array[newIdx].data.Group = fb;
        return newIdx;
      }
      if (fb) ((SIObject*)fb)->Release();
    }
#endif
    return -1;
  }

  // Add to ObjectCache heap
  int newIdx = ObjectCache.Add();
  ObjectCache.array[newIdx].data.FileName = _strdup(filename);
  ObjectCache.array[newIdx].data.Scale = scale;
  ObjectCache.array[newIdx].data.RefCount = 0;
  ObjectCache.array[newIdx].data.Group = group;
  return newIdx;
}

// --- RenderToWinBitmap ---

HBITMAP SGepard::RenderToWinBitmap(int rotate, int width, int height)
{
  return nullptr;
}

// --- RenderMinimap ---

unsigned char* SGepard::RenderMinimap(int width, int height)
{
  char atmstr[200];
  HRESULT hr;

  // Release old shadow render targets
  if (ShadowRenderTarget) { ShadowRenderTarget->Release(); ShadowRenderTarget = nullptr; }
  if (ShadowRenderTargetZ) { ShadowRenderTargetZ->Release(); ShadowRenderTargetZ = nullptr; }

  // Create 4x oversampled render target
  IDirect3DSurface9 *rt = nullptr, *dss = nullptr, *oldRT = nullptr, *oldDSS = nullptr;
  hr = lpD3DDev->CreateRenderTarget(4 * width, 4 * height, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &rt, nullptr);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderMinimap: CreateRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->CreateDepthStencilSurface(4 * width, 4 * height, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &dss, nullptr);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderMinimap: CreateDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->GetRenderTarget(0, &oldRT);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderMinimap: GetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  if (!ResolutionScaleSurface.Ptr) {
    hr = lpD3DDev->GetDepthStencilSurface(&oldDSS);
    if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderMinimap: GetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  }
  hr = lpD3DDev->SetRenderTarget(0, rt);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderMinimap: SetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->SetDepthStencilSurface(dss);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderMinimap: SetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // Compute map size
  int mapSize;
  if (XSize > 96 && ZSize > 96) {
    mapSize = (XSize - 96 > ZSize - 96) ? XSize - 96 : ZSize - 96;
  } else {
    mapSize = (XSize > ZSize) ? XSize : ZSize;
  }

  float scale = 2.0f / (float)mapSize;
  SetOrthogonalProjection(scale, NearPlane, FarPlane);
  // vrot pre-negated: SetViewProperties drops the IDA-original negation,
  // so direct callers (bypassing ComputeCamera) must compensate or the
  // camera looks straight up at the skybox instead of down at the terrain.
  SetViewProperties((float)(XSize / 2), 400.0f, (float)(ZSize / 2), 0.0f, -1.5707963f);

  // Render with increased fog distance and max shadow quality
  FogStart += 1000.0f;
  int savedSQ = GetShadowQuality();
  SetShadowQuality(3);
  RenderScene(true);
  SetShadowQuality(savedSQ);
  FogStart -= 1000.0f;

  SetPerspectiveProjection(FOV, NearPlane, FarPlane);

  // Restore render targets
  hr = lpD3DDev->SetRenderTarget(0, oldRT);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderMinimap: SetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->SetDepthStencilSurface(oldDSS);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderMinimap: SetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // Read rendered image and downsample
  SSurfaceBitmap surfBmp(rt);
  SBitmap bmap(&surfBmp, D3DFMT_A8R8G8B8);
  surfBmp.~SSurfaceBitmap();

  // Two mip levels: 4x -> 2x -> 1x
  bmap.NextMipLevel();
  bmap.NextMipLevel();

  unsigned int dataSize = 4 * width * height;
  unsigned char *result = new unsigned char[dataSize];
  memcpy(result, bmap.Data, dataSize);

  if (rt) { rt->Release(); rt = nullptr; }
  if (dss) { dss->Release(); dss = nullptr; }
  if (oldRT) { oldRT->Release(); oldRT = nullptr; }
  if (oldDSS) oldDSS->Release();

  return result;
}

// --- Shadow generation helpers ---

//----- (00438370) --------------------------------------------------------
void SGepard::MakeObjectShadowMask(SGroup *object)
{
  char atmstr[200];

  // Delete old ShadowMask2
  SShadowMask2 *oldMask = object->ShadowMask2;
  if (oldMask)
  {
    delete oldMask;
    object->ShadowMask2 = 0;
  }

  // Get projected shadow bounds
  float umin, umax, vmin, vmax;
  object->ComputeProjectedShadowBounding(&umin, &umax, &vmin, &vmax);

  // Scale bounds to texels (32 pixels per unit)
  int u0 = (int)floor(umin * 32.0);
  int u1 = (int)ceil(umax * 32.0);
  int v0 = (int)floor(vmin * 32.0);
  int v1 = (int)ceil(vmax * 32.0);
  if (v1 - v0 > 1020 || u1 - u0 > 1020)
  {
    v1 = v0 + 1020;
    u1 = u0 + 1020;
  }

  // Create/reuse 1024x1024 R5G6B5 render target
  HRESULT hr;
  if (!ShadowRenderTarget)
  {
    hr = lpD3DDev->CreateRenderTarget(1024, 1024, D3DFMT_R5G6B5, D3DMULTISAMPLE_NONE, 0, TRUE, &ShadowRenderTarget, NULL);
    if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask: CreateRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  }
  if (!ShadowRenderTargetZ)
  {
    hr = lpD3DDev->CreateDepthStencilSurface(1024, 1024, D3DFMT_D16, D3DMULTISAMPLE_NONE, 0, TRUE, &ShadowRenderTargetZ, NULL);
    if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask: CreateDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  }

  // Save original render targets
  IDirect3DSurface9 *old_rt = NULL, *old_dss = NULL;
  hr = lpD3DDev->GetRenderTarget(0, &old_rt);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask: GetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  if (!ResolutionScaleSurface.Ptr)
  {
    hr = lpD3DDev->GetDepthStencilSurface(&old_dss);
    if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask: GetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  }

  // Set shadow render target
  hr = lpD3DDev->SetRenderTarget(0, ShadowRenderTarget);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask: SetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->SetDepthStencilSurface(ShadowRenderTargetZ);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask: SetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // Build orthographic projection
  float zrange = 30.0f - NearPlane;
  ProjectionMatrix._12 = 0.0f; ProjectionMatrix._13 = 0.0f; ProjectionMatrix._14 = 0.0f;
  ProjectionMatrix._21 = 0.0f; ProjectionMatrix._23 = 0.0f; ProjectionMatrix._24 = 0.0f;
  ProjectionMatrix._31 = 0.0f; ProjectionMatrix._32 = 0.0f; ProjectionMatrix._34 = 0.0f;
  ProjectionMatrix._41 = 0.0f; ProjectionMatrix._42 = 0.0f; ProjectionMatrix._43 = 0.0f;
  ProjectionMatrix._44 = 0.0f;
  ProjectionMatrix._11 = 0.0625f;
  ProjectionMatrix._22 = -0.0625f;
  ProjectionMatrix._33 = 1.0f / zrange;
  ProjectionMatrix._44 = 1.0f;
  ProjectionMatrix._43 = -(NearPlane * (1.0f / zrange));
  ProjectionMatrix._41 = -1.0f - (umin * 32.0f - 2.0f) * 0.001953125f;
  ProjectionMatrix._42 = (float)((vmin * 32.0 - 2.0) * 0.001953125 + 1.0);
  lpD3DDev->SetTransform(D3DTS_PROJECTION, &ProjectionMatrix);

  // Save camera state
  float savedCamX = CameraXPos;
  float savedCamY = CameraYPos;
  float savedCamZ = CameraZPos;
  float savedCamH = CameraHRot;
  float savedCamV = CameraVRot;

  // Set view matrix: translate by -objectPos, multiply by SunProjection, offset Z+10
  float x, y, z;
  object->GetInterpolatedPosition(&x, &y, &z);
  D3DXMatrixTranslation(&CameraMatrix, -x, -y, -z);
  D3DXMatrixMultiply(&CameraMatrix, &CameraMatrix, &SunProjection);
  CameraMatrix._43 = CameraMatrix._43 + 10.0f;
  lpD3DDev->SetTransform(D3DTS_VIEW, &CameraMatrix);

  // Clear and render
  hr = lpD3DDev->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0x00000000, 1.0f, 0);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask\\IDirect3DDevice::Clear", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->BeginScene();
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask\\IDirect3DDevice::BeginScene", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  lpD3DDev->SetRenderState(D3DRS_ZENABLE, TRUE);
  lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
  lpD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

  // Render terrain in black, object in white
  SetLightingType(LT_AMBIENT);
  SetAmbientColor(0x000000);
  Terrain->DrawShadow();
  SetAmbientColor(0xFFFFFF);
  object->DrawShadow();

  hr = lpD3DDev->EndScene();
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask\\IDirect3DDevice::EndScene", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // Restore projection and view
  SetPerspectiveProjection(FOV, NearPlane, FarPlane);
  SetViewProperties(savedCamX, savedCamY, savedCamZ, savedCamH, savedCamV);
  InitRenderStates();

  // Restore render targets
  hr = lpD3DDev->SetRenderTarget(0, old_rt);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask: SetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->SetDepthStencilSurface(old_dss);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeObjectShadowMask: SetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // Create mask from rendered surface
  SSurfaceBitmap surface(ShadowRenderTarget);
  SShadowMask mask1(u1 - u0 + 4, v1 - v0 + 4, &surface);

  SShadowMask2 *newMask = new SShadowMask2(&mask1);
  object->ShadowMaskUMin = umin;
  object->ShadowMaskUMax = umax;
  object->ShadowMaskVMin = vmin;
  object->ShadowMaskVMax = vmax;
  object->ShadowMask2 = newMask;

  mask1.~SShadowMask();
  surface.~SSurfaceBitmap();

  if (old_rt)
  {
    old_rt->Release();
    old_rt = NULL;
  }
  if (old_dss)
    old_dss->Release();
}

//----- (00438D40) --------------------------------------------------------
void SGepard::MakeObjectShadowShader(SGroup *object)
{
  SShadowMask2 *mask = object->ShadowMask2;
  if (!mask)
    return;

  // Compute texture size from mask dimensions (round up to power of 2)
  unsigned int rawW = mask->Width + 2;
  unsigned int rawH = mask->Height + 2;
  int texW = rawW <= 16 ? 16 : rawW <= 32 ? 32 : rawW <= 64 ? 64 : rawW <= 128 ? 128 : rawW <= 256 ? 256 : rawW <= 512 ? 512 : 1024;
  int texH = rawH <= 16 ? 16 : rawH <= 32 ? 32 : rawH <= 64 ? 64 : rawH <= 128 ? 128 : rawH <= 256 ? 256 : rawH <= 512 ? 512 : 1024;

  // Create/reuse shadow texture at correct size
  int shadowTex = object->ShadowTexture;
  if (shadowTex >= 0)
  {
    if (Textures.array[shadowTex].data.Width != (unsigned int)texW || Textures.array[shadowTex].data.Height != (unsigned int)texH)
    {
      SMesh *sm = object->ShadowMesh;
      if (sm)
      {
        sm->Release();
        object->ShadowMesh = 0;
        shadowTex = object->ShadowTexture;
      }
      ReleaseTexture(shadowTex, false);
      object->ShadowTexture = -1;
      shadowTex = -1;
    }
  }
  if (shadowTex < 0)
  {
    shadowTex = CreateEmptyTexture(texW, texH, 4, false, true);
    object->ShadowTexture = shadowTex;
  }

  // Render mask to texture
  {
    STextureBitmap tex(Textures.array[shadowTex].data.lpTexture, 0);
    object->ShadowMask2->MakeBitmap(&tex);
    tex.~STextureBitmap();
  }

  // Get object position
  float xobj, yobj, zobj;
  object->GetInterpolatedPosition(&xobj, &yobj, &zobj);

  // Project 4 corners of shadow mask to screen space, compute bounding box
  float screenX, screenZ;
  float bboxMinX = 10000.0f, bboxMaxX = -10000.0f;
  float bboxMinZ = 10000.0f, bboxMaxZ = -10000.0f;

  ProjectPoint(object->ShadowMaskUMin, object->ShadowMaskVMin, xobj, yobj, zobj, &screenX, &screenZ);
  bboxMinX = fminf(screenX, bboxMinX); bboxMaxX = fmaxf(screenX, bboxMaxX);
  bboxMinZ = fminf(screenZ, bboxMinZ); bboxMaxZ = fmaxf(screenZ, bboxMaxZ);

  ProjectPoint(object->ShadowMaskUMin, object->ShadowMaskVMax, xobj, yobj, zobj, &screenX, &screenZ);
  bboxMinX = fminf(screenX, bboxMinX); bboxMaxX = fmaxf(screenX, bboxMaxX);
  bboxMinZ = fminf(screenZ, bboxMinZ); bboxMaxZ = fmaxf(screenZ, bboxMaxZ);

  ProjectPoint(object->ShadowMaskUMax, object->ShadowMaskVMin, xobj, yobj, zobj, &screenX, &screenZ);
  bboxMinX = fminf(screenX, bboxMinX); bboxMaxX = fmaxf(screenX, bboxMaxX);
  bboxMinZ = fminf(screenZ, bboxMinZ); bboxMaxZ = fmaxf(screenZ, bboxMaxZ);

  ProjectPoint(object->ShadowMaskUMax, object->ShadowMaskVMax, xobj, yobj, zobj, &screenX, &screenZ);
  bboxMinX = fminf(screenX, bboxMinX); bboxMaxX = fmaxf(screenX, bboxMaxX);
  bboxMinZ = fminf(screenZ, bboxMinZ); bboxMaxZ = fmaxf(screenZ, bboxMaxZ);

  // Add 2-pixel margin
  bboxMinX -= 2.0f;
  bboxMaxX += 2.0f;
  bboxMinZ -= 2.0f;
  bboxMaxZ += 2.0f;

  // Grid bounds
  int gx0 = (int)floor(bboxMinX);
  int gx1 = (int)ceil(bboxMaxX);
  int gz0 = (int)floor(bboxMinZ);
  int gz1 = (int)ceil(bboxMaxZ);
  int gridW = gx1 - gx0 + 1;
  int gridH = gz1 - gz0 + 1;

  // Per-point data: [U, V, flags, vertexIndex] — 4 ints (16 bytes) per point
  struct GridPoint {
    float u, v;
    int flags;    // bit 0 = inside mask, bit 1 = used by quad
    int vertIdx;
  };
  GridPoint *grid = (GridPoint *)operator new[](sizeof(GridPoint) * gridW * gridH);

  // Sun direction projection
  float sunVX = SunDir.x / SunDir.y;
  float sunVZ = SunDir.z / SunDir.y;

  // Pass 1: For each grid point, compute UV in shadow space and check mask bounds
  for (int iz = gz0; iz <= gz1; iz++)
  {
    for (int ix = gx0; ix <= gx1; ix++)
    {
      GridPoint *pt = &grid[(iz - gz0) * gridW + (ix - gx0)];
      double terrainH = Terrain->GetOldHeight(ix, iz);
      float dy = yobj - (float)terrainH;
      float projX = (float)(ix - xobj) + dy * sunVX;
      float projZ = (float)(iz - zobj) + dy * sunVZ;
      pt->u = SunProjection._11 * projX + SunProjection._12 * projZ;
      pt->v = SunProjection._31 * projX + SunProjection._32 * projZ;
      pt->flags = (pt->u > object->ShadowMaskUMin && object->ShadowMaskUMax > pt->u &&
                   pt->v > object->ShadowMaskVMin && object->ShadowMaskVMax > pt->v) ? 1 : 0;
      pt->vertIdx = 0;
    }
  }

  // Pass 2: Mark quad vertices — if any corner of a quad is inside mask, mark all 4 for use
  int numQuads = 0;
  for (int iz = gz0; iz < gz1; iz++)
  {
    for (int ix = gx0; ix < gx1; ix++)
    {
      GridPoint *p00 = &grid[(iz - gz0) * gridW + (ix - gx0)];
      GridPoint *p10 = &grid[(iz - gz0) * gridW + (ix - gx0 + 1)];
      GridPoint *p01 = &grid[(iz - gz0 + 1) * gridW + (ix - gx0)];
      GridPoint *p11 = &grid[(iz - gz0 + 1) * gridW + (ix - gx0 + 1)];
      if ((p00->flags | p10->flags | p01->flags | p11->flags) & 1)
      {
        p00->flags |= 2;
        p10->flags |= 2;
        p01->flags |= 2;
        p11->flags |= 2;
        numQuads++;
      }
    }
  }

  // Pass 3: Assign vertex indices to marked points
  int numVerts = 0;
  for (int iz = gz0; iz <= gz1; iz++)
  {
    for (int ix = gx0; ix <= gx1; ix++)
    {
      GridPoint *pt = &grid[(iz - gz0) * gridW + (ix - gx0)];
      if (pt->flags & 2)
        pt->vertIdx = numVerts++;
    }
  }

  // Release old shadow mesh
  SMesh *oldMesh = object->ShadowMesh;
  if (oldMesh)
  {
    oldMesh->Release();
    object->ShadowMesh = 0;
  }

  // Create mesh if we have quads
  if (numQuads > 0 && numVerts > 0)
  {
    SMesh *mesh = new SMesh(this, 1);
    mesh->CreateIndexBuffer(0, 6 * numQuads);

    // Fill index buffer
    int idx = 0;
    for (int iz = gz0; iz < gz1; iz++)
    {
      for (int ix = gx0; ix < gx1; ix++)
      {
        GridPoint *p00 = &grid[(iz - gz0) * gridW + (ix - gx0)];
        GridPoint *p10 = &grid[(iz - gz0) * gridW + (ix - gx0 + 1)];
        GridPoint *p01 = &grid[(iz - gz0 + 1) * gridW + (ix - gx0)];
        GridPoint *p11 = &grid[(iz - gz0 + 1) * gridW + (ix - gx0 + 1)];
        if ((p00->flags | p10->flags | p01->flags | p11->flags) & 1)
        {
          mesh->lpIndices[0][idx + 0] = (unsigned short)p01->vertIdx;
          mesh->lpIndices[0][idx + 1] = (unsigned short)p00->vertIdx;
          mesh->lpIndices[0][idx + 2] = (unsigned short)p10->vertIdx;
          mesh->lpIndices[0][idx + 3] = (unsigned short)p01->vertIdx;
          mesh->lpIndices[0][idx + 4] = (unsigned short)p10->vertIdx;
          mesh->lpIndices[0][idx + 5] = (unsigned short)p11->vertIdx;
          idx += 6;
        }
      }
    }
    mesh->UnlockIndexBuffer(0);

    // Fill vertex buffer
    mesh->CreateVertexBuffer(numVerts);
    int vi = 0;
    for (int iz = gz0; iz <= gz1; iz++)
    {
      for (int ix = gx0; ix <= gx1; ix++)
      {
        GridPoint *pt = &grid[(iz - gz0) * gridW + (ix - gx0)];
        if (pt->flags & 2)
        {
          char *vtx = (char *)mesh->lpVertices + vi * mesh->VertexSize;
          *(float *)(vtx + mesh->OffsetXYZ + 0) = (float)ix;
          *(float *)(vtx + mesh->OffsetXYZ + 4) = (float)Terrain->GetOldHeight(ix, iz);
          *(float *)(vtx + mesh->OffsetXYZ + 8) = (float)iz;
          *(float *)(vtx + mesh->OffsetTexture1 + 0) = ((pt->u - object->ShadowMaskUMin) * 16.0f + 1.0f) / (float)texW;
          *(float *)(vtx + mesh->OffsetTexture1 + 4) = ((pt->v - object->ShadowMaskVMin) * 16.0f + 1.0f) / (float)texH;
          vi++;
        }
      }
    }
    mesh->UnlockVertexBuffer();
    object->ShadowMesh = mesh;
  }

  operator delete[](grid);
}

//----- (00432370) --------------------------------------------------------
void SGepard::GenerateObjectShadow(SGroup *object, bool dynamic)
{
  char atmstr[200];
  HRESULT hr;

  object->Precalculate();
  float xmin, xmax, zmin, zmax;
  object->ComputeShadowBounding(&xmin, &xmax, &zmin, &zmax);

  // Compute texture dimensions (ceil + 2, round up to power of 2)
  unsigned int rawW = (unsigned int)(ceil((double)(xmax - xmin) * 32.0) + 2.0);
  unsigned int rawH = (unsigned int)(ceil((double)(zmax - zmin) * 32.0) + 2.0);

  int width = rawW <= 32 ? 32 : rawW <= 64 ? 64 : rawW <= 128 ? 128 : rawW <= 256 ? 256 : rawW <= 512 ? 512 : 1024;
  int height = rawH <= 32 ? 32 : rawH <= 64 ? 64 : rawH <= 128 ? 128 : rawH <= 256 ? 256 : rawH <= 512 ? 512 : 1024;

  // Texture management: reuse if same size, else release and create
  int shadowTex = object->ShadowTexture;
  if (shadowTex >= 0)
  {
    if (Textures.array[shadowTex].data.Width != (unsigned int)width || Textures.array[shadowTex].data.Height != (unsigned int)height)
    {
      SMesh *sm = object->ShadowMesh;
      if (sm)
      {
        sm->Release();
        object->ShadowMesh = 0;
        shadowTex = object->ShadowTexture;
      }
      ReleaseTexture(shadowTex, false);
      object->ShadowTexture = -1;
      shadowTex = -1;
    }
  }

  if (shadowTex < 0)
  {
    if (dynamic)
    {
      if (TFShadow == D3DFMT_UNKNOWN)
      {
        object->ShadowTexture = -1;
      }
      else
      {
        int idx = Textures.Add();
        int off = idx;
        Textures.array[off].data.FileName = _strdup("**unique**");
        Textures.array[off].data.RefCount = 1;
        Textures.array[off].data.Width = width;
        Textures.array[off].data.Height = height;
        if ((width & (width - 1)) != 0)
          Logger.g->Panic("SGepard::CreateShadowTexture: (%d x %d): Width must be power of 2", width, height);
        if ((height & (height - 1)) != 0)
          Logger.g->Panic("SGepard::CreateShadowTexture: (%d x %d): Height must be power of 2", width, height);
        hr = lpD3DDev->CreateTexture(width, height, 1, D3DUSAGE_RENDERTARGET, TFShadow, D3DPOOL_DEFAULT, &Textures.array[off].data.lpTexture, NULL);
        if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::CreateShadowTexture: CreateTexture", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
        object->ShadowTexture = idx;
      }
    }
    else
    {
      object->ShadowTexture = CreateEmptyTexture(width, height, 4, false, true);
    }
  }

  if (object->ShadowTexture < 0)
    return;

  // Get render target surface
  IDirect3DSurface9 *rt = NULL, *old_rt = NULL, *old_dss = NULL;
  if (dynamic)
  {
    IDirect3DTexture9 *lpTex = Textures.array[object->ShadowTexture].data.lpTexture;
    hr = lpTex->GetSurfaceLevel(0, &rt);
    if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: GetSurfaceLevel failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  }
  else
  {
    hr = lpD3DDev->CreateRenderTarget(width, height, D3DFMT_R5G6B5, D3DMULTISAMPLE_NONE, 0, TRUE, &rt, NULL);
    if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: CreateRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  }

  // Save and set render target
  hr = lpD3DDev->GetRenderTarget(0, &old_rt);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: GetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  if (!ResolutionScaleSurface.Ptr)
  {
    hr = lpD3DDev->GetDepthStencilSurface(&old_dss);
    if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: GetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  }
  hr = lpD3DDev->SetRenderTarget(0, rt);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: SetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->SetDepthStencilSurface(NULL);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: SetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // Build orthographic projection
  float zrange = 30.0f - NearPlane;
  float invZrange = 1.0f / zrange;
  ProjectionMatrix._12 = 0.0f; ProjectionMatrix._13 = 0.0f; ProjectionMatrix._14 = 0.0f;
  ProjectionMatrix._21 = 0.0f; ProjectionMatrix._23 = 0.0f; ProjectionMatrix._24 = 0.0f;
  ProjectionMatrix._31 = 0.0f; ProjectionMatrix._32 = 0.0f; ProjectionMatrix._34 = 0.0f;
  ProjectionMatrix._41 = 0.0f; ProjectionMatrix._42 = 0.0f; ProjectionMatrix._43 = 0.0f;
  ProjectionMatrix._44 = 0.0f;
  ProjectionMatrix._33 = invZrange;
  ProjectionMatrix._11 = 64.0f / (float)width;
  ProjectionMatrix._22 = 64.0f / (float)height;
  ProjectionMatrix._44 = 1.0f;
  ProjectionMatrix._43 = -(NearPlane * invZrange);
  lpD3DDev->SetTransform(D3DTS_PROJECTION, &ProjectionMatrix);

  // Save camera state
  float savedCamX = CameraXPos;
  float savedCamY = CameraYPos;
  float savedCamZ = CameraZPos;
  float savedCamH = CameraHRot;
  float savedCamV = CameraVRot;

  // Set view: center of bounds, Y=40, looking straight down
  float viewX = (float)(width - 2) * 0.015625f + xmin;
  float viewZ = (float)(height - 2) * 0.015625f + zmin;
  SetViewProperties(viewX, 40.0f, viewZ, 0.0f, 1.5707963f);

  // Clear and render
  hr = lpD3DDev->Clear(0, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::Clear", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->BeginScene();
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::BeginScene", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  SetLightingType(LT_AMBIENT);
  SetAmbientColor(0xFFFFFF);
  lpD3DDev->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
  object->DrawShadowHack2();

  hr = lpD3DDev->EndScene();
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::EndScene", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // Restore camera/projection/render states
  SetPerspectiveProjection(FOV, NearPlane, FarPlane);
  SetViewProperties(savedCamX, savedCamY, savedCamZ, savedCamH, savedCamV);
  InitRenderStates();

  // Restore render targets
  hr = lpD3DDev->SetRenderTarget(0, old_rt);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: SetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->SetDepthStencilSurface(old_dss);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: SetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // If not dynamic, blit render target to texture
  if (!dynamic)
  {
    SSurfaceBitmap surface(rt);
    STextureBitmap target(Textures.array[object->ShadowTexture].data.lpTexture, 0);
    surface.Format = (D3DFORMAT)257;  // force R5G6B5 interpretation
    target.BitBlt(0, 0, width, height, &surface, 0, 0);
    target.~STextureBitmap();
    surface.~SSurfaceBitmap();
  }

  // Release temporary surfaces
  if (rt) { rt->Release(); rt = NULL; }
  if (old_rt) { old_rt->Release(); old_rt = NULL; }
  if (old_dss) { old_dss->Release(); old_dss = NULL; }

  // Build grid mesh for shadow projection
  int ix0 = (int)floor(xmin);
  int iz0 = (int)floor(zmin);
  int ix1 = (int)ceil(xmax);
  int iz1 = (int)ceil(zmax);
  int gridW = ix1 - ix0;
  int meshW = gridW + 1;
  int gridH = iz1 - iz0;
  int meshH = gridH + 1;

  SMesh *shadowMesh = object->ShadowMesh;
  if (shadowMesh)
  {
    if (object->SMX0 != ix0 || object->SMZ0 != iz0 || object->SMX1 != ix1 || object->SMZ1 != iz1)
    {
      // Bounds changed — recreate mesh
      shadowMesh->Release();
      object->ShadowMesh = NULL;
      shadowMesh = NULL;
    }
  }

  if (!shadowMesh)
  {
    // Create new shadow mesh
    shadowMesh = new SMesh(this, 1);
    shadowMesh->CreateIndexBuffer(0, gridW * (6 * meshH - 6));

    // Fill index buffer: quads for each grid cell
    int idx = 0;
    unsigned short base = 0;
    for (int gz = 0; gz < gridH; gz++)
    {
      for (int gx = 0; gx < gridW; gx++)
      {
        unsigned short v0 = base;
        unsigned short v1 = base + meshW;
        shadowMesh->lpIndices[0][idx + 0] = v1;
        shadowMesh->lpIndices[0][idx + 1] = v0;
        shadowMesh->lpIndices[0][idx + 2] = v0 + 1;
        shadowMesh->lpIndices[0][idx + 3] = v1;
        shadowMesh->lpIndices[0][idx + 4] = v0 + 1;
        shadowMesh->lpIndices[0][idx + 5] = v1 + 1;
        idx += 6;
        base++;
      }
      base++;  // skip to next row
    }
    shadowMesh->UnlockIndexBuffer(0);

    // Fill vertex buffer
    shadowMesh->CreateVertexBuffer(meshW * meshH);
    int vi = 0;
    double texScaleW = 32.0 / (double)width;
    double texScaleH = 32.0 / (double)height;
    for (int gz = 0; gz < meshH; gz++)
    {
      int wz = iz0 + gz;
      for (int gx = 0; gx < meshW; gx++)
      {
        int wx = ix0 + gx;
        char *vtx = (char *)shadowMesh->lpVertices + vi * shadowMesh->VertexSize;
        *(float *)(vtx + shadowMesh->OffsetXYZ + 0) = (float)wx;
        *(float *)(vtx + shadowMesh->OffsetXYZ + 4) = (float)Terrain->GetHeight(wx, wz);
        *(float *)(vtx + shadowMesh->OffsetXYZ + 8) = (float)wz;
        *(float *)(vtx + shadowMesh->OffsetTexture1 + 0) = (float)((float)(wx - xmin) * texScaleW);
        *(float *)(vtx + shadowMesh->OffsetTexture1 + 4) = (float)(1.0 - (float)(wz - zmin) * texScaleH);
        vi++;
      }
    }
    shadowMesh->UnlockVertexBuffer();

    object->SMX0 = ix0;
    object->SMZ0 = iz0;
    object->SMX1 = ix1;
    object->SMZ1 = iz1;
    object->ShadowMesh = shadowMesh;
  }
  else
  {
    // Mesh exists, just update texture coordinates
    shadowMesh->LockVertexBuffer();
    double texScaleW = 32.0 / (double)width;
    double texScaleH = 32.0 / (double)height;
    int vi = 0;
    for (int gz = 0; gz < meshH; gz++)
    {
      int wz = iz0 + gz;
      for (int gx = 0; gx < meshW; gx++)
      {
        int wx = ix0 + gx;
        char *vtx = (char *)shadowMesh->lpVertices + vi * shadowMesh->VertexSize;
        *(float *)(vtx + shadowMesh->OffsetTexture1 + 0) = (float)((float)(wx - xmin) * texScaleW);
        *(float *)(vtx + shadowMesh->OffsetTexture1 + 4) = (float)(1.0 - (float)(wz - zmin) * texScaleH);
        vi++;
      }
    }
    shadowMesh->UnlockVertexBuffer();
  }
}

// --- GenerateAllShadowShaders ---

void SGepard::GenerateAllShadowShaders()
{
  if ( g_HeadlessMode ) return;
  int idx = -1;
  while (true) {
    int size = StaticObjects.size;
    if (++idx >= size) break;
    while (StaticObjects.array[idx].use != 0x7FFFFFFF) {
      if (++idx >= size) return;
    }
    if (idx < 0) break;
    SGroup *group = StaticObjects.array[idx].data.Group;
    if (group->Visible && group->ShadowMask2 && !group->ShadowMesh)
      MakeObjectShadowShader(StaticObjects.array[idx].data.Group);
  }
}

// --- Editor-only stubs ---

//----- (00434570) --------------------------------------------------------
void SGepard::InitEditor()
{
  SGroup *g1 = new SGroup(this, DT_NORMAL);
  node1 = g1;
  SGroup *g2 = new SGroup(this, DT_NORMAL);
  node2 = g2;
  if (!node1->Load4DFile(0, "fx/sphere.4d", 0.1f))
  {
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "WARNING: InitEditor: Couldn't load fx/sphere.4d (node1) — spline editing may not work");
    #endif
  }
  if (!node2->Load4DFile(0, "fx/sphere.4d", 0.06f))
  {
    #ifdef HD_DEBUG_RENDERER
    Logger.g->Log(0, "WARNING: InitEditor: Couldn't load fx/sphere.4d (node2) — spline editing may not work");
    #endif
  }
}

void SGepard::GetNodePosition(SSplineRing *_splinering, int index, float *x, float *y, float *z)
{
  SNode *node = _splinering->spline->Nodes.first;
  if (!node)
    Logger.g->Panic("GetPointer: SChain is empty!");
  for (int i = 0; i < index; ++i) {
    node = node->next;
    if (!node)
      Logger.g->Panic("GetPointer: the given index is invalid!");
  }
  if (x) *x = node->x;
  if (y) *y = node->y;
  if (z) *z = node->z;
}

void SGepard::GetSelections(SHillRing **_hillring, SSplineRing **_splinering)
{
  if (_hillring) *_hillring = gpSelHillRing;
  if (_splinering) *_splinering = gpSelSplineRing;
}

bool SGepard::GetSplineVisibility(SSplineRing *_splinering)
{
  return !_splinering || _splinering->spline->VisibilityState;
}

float SGepard::GetSplineAltitude2(SSplineRing *_splinering)
{
  return _splinering->spline->Altitude2;
}

float SGepard::GetSplineSize(SSplineRing *_splinering)
{
  return _splinering->spline->Size;
}

float SGepard::GetTopSplineAltitude(SHillRing *_hillring)
{
  SChain<SSplineRing> *chain = _hillring->SplineChain;
  if (chain->Closed && !chain->first)
    Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
  return chain->first->spline->Altitude;
}

float SGepard::GetTopSplineAltitude2(SHillRing *_hillring)
{
  SChain<SSplineRing> *chain = _hillring->SplineChain;
  if (chain->Closed && !chain->first)
    Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
  return chain->first->spline->Altitude2;
}

int SGepard::GetCheckBoxNr(SSplineRing *_splinering)
{
  return _splinering->spline->CheckBoxNr;
}

float SGepard::GetRiverSize(SHillRing *hillring)
{
  if (hillring)
    return hillring->RiverSize;
  return 0.0f;
}

SShaderInfo* SGepard::GetIndexFromCoordinates(float _x, float _z)
{
  SShaderInfo *si = ShaderInfos.last;
  ShaderInfos.current = si;
  if (!si)
    return nullptr;
  while (true) {
    double fx = floor(si->xpos);
    if (_x >= (float)fx) {
      double fz = floor(si->zpos);
      float fzf = (float)fz;
      if (_z > fzf
        && (float)((float)(si->XVertices - 1) + (float)fx) > _x
        && (float)((float)(si->ZVertices - 1) + fzf) > _z) {
        return si;
      }
    }
    // StepToPrev
    if (ShaderInfos.Closed) {
      if (!si)
        Logger.g->Panic("StepToPrev: In a closed chain <current> is NULL");
      si = si->prev;
      if (!si)
        si = ShaderInfos.last;
    } else if (si) {
      si = si->prev;
    } else {
      si = nullptr;
    }
    ShaderInfos.current = si;
    if (!si)
      return nullptr;
  }
}

//----- (00431F20) --------------------------------------------------------
void SGepard::EgysegAlattiTalajKoszolas(float x, float z, int race)
{
  int rnd = (int)((float)((float)rand() * 0.000030517578f) * 10.0f);
  if (rnd) {
    int v = (int)((float)((float)rand() * 0.000030517578f) * 3.0f);
    float y = (float)(Terrain->GetHeight(x, z) + 1.0f);

    if (v == 0)
      PlayEffect(_egyseg_alatti_talaj_koszolas1, x, y, z, 0.0f);
    else if (v == 1)
      PlayEffect(_egyseg_alatti_talaj_koszolas2, x, y, z, 0.0f);
    else
      PlayEffect(_egyseg_alatti_talaj_koszolas3, x, y, z, 0.0f);
  } else {
    float y = (float)(Terrain->GetHeight(x, z) + 1.0f);

    if (race)
      PlayEffect(_egyseg_alatti_talaj_koszolas_pig, x, y, z, 0.0f);
    else
      PlayEffect(_egyseg_alatti_talaj_koszolas_rabbit, x, y, z, 0.0f);
  }
}

// --- Spline loading/saving ---

void SGepard::LoadShaders(SStream *is)
{
  DestroyShaders(1);
  ShaderInfos.current = ShaderInfos.first;

  int numitems;
  is->Read(&numitems, 4);
  #ifdef HD_DEBUG_SHADERS
  Logger.g->Log(0, "SGepard::LoadShaders: numitems=%d", numitems);
  #endif
  for (int i = 0; i < numitems; ++i) {
    char filename[120];
    float xpos, zpos;
    int rotation;
    is->Read(filename, 120);
    is->Read(&xpos, 4);
    is->Read(&zpos, 4);
    is->Read(&rotation, 4);
    if (i < 5)
    {
      #ifdef HD_DEBUG_SHADERS
      Logger.g->Log(0, "  Shader[%d]: '%s' pos=(%.2f,%.2f) rot=%d", i, filename, xpos, zpos, rotation);
      #endif
    }
    SShaderInfo *result = CreateShader(xpos, zpos, filename, rotation, 1, 0);
    if (i < 5)
    {
      #ifdef HD_DEBUG_SHADERS
      Logger.g->Log(0, "  Shader[%d]: CreateShader returned %p", i, result);
      #endif
    }
  }
  #ifdef HD_DEBUG_SHADERS
  Logger.g->Log(0, "SGepard::LoadShaders: done, ShaderInfos.first=%p", ShaderInfos.first);
  #endif

  // Clean up unreferenced textures
  int idx = -1;
  while (true) {
    int size = Textures.size;
    if (++idx >= size) break;
    while (Textures.array[idx].use != 0x7FFFFFFF) {
      if (++idx >= size) return;
    }
    if (idx < 0) break;
    if (!Textures.array[idx].data.RefCount) {
      if (Textures.array[idx].data.FileName) {
        ::operator delete(Textures.array[idx].data.FileName);
        Textures.array[idx].data.FileName = nullptr;
      }
      if (Textures.array[idx].data.lpTexture) {
        Textures.array[idx].data.lpTexture->Release();
        Textures.array[idx].data.lpTexture = nullptr;
      }
      // Remove from heap
      Textures.array[idx].use = Textures.nextempty;
      --Textures.occupied;
      Textures.nextempty = idx;
    }
  }
}

void SGepard::LoadSplines(SStream *is)
{
  DestroySplines();

  if (is->ReadInt() != 123456)
    Logger.g->Panic("SGepard::ReadSign: Attempt to read sign, but no sign here!");

  while (is->ReadInt()) {
    // Create new HillRing
    SHillRing *hill = new SHillRing();
    memset(hill, 0, sizeof(SHillRing));

    // Append to HillChain
    if (HillChain.last) {
      HillChain.last->next = hill;
      hill->prev = HillChain.last;
      HillChain.last = hill;
      hill->next = nullptr;
    } else {
      HillChain.last = hill;
      HillChain.first = hill;
    }
    ++HillChain.NumItems;
    HillChain.current = hill;

    // x64 fix: original `is->Read(&hill->SplineChain, 24)` dumped 24 raw bytes
    // from the x86 SHillRing layout (4-byte SplineChain ptr + Type + WaterEnabled
    // + RiverSize + 4-byte HillMesh ptr + 4-byte HillMesh2 ptr). On x64 the
    // struct's pointer fields are 8 bytes so those 24 bytes land on the wrong
    // fields. Read the 6 on-disk DWORDs into local typed values that match
    // the x86 layout, then assign to the runtime fields. Pointer slots
    // (SplineChain/HillMesh/HillMesh2) are runtime-only — they get set
    // separately below or after the .scene load completes.
    {
      struct OnDiskHillRing {
        int   SplineChainPtr_unused;
        int   Type;
        int   WaterEnabled_packed;   // bool in low byte + 3 bytes pad on x86
        float RiverSize;
        int   HillMesh_unused;
        int   HillMesh2_unused;
      };
      OnDiskHillRing diskHill;
      is->Read(&diskHill, sizeof(diskHill));
      hill->Type         = diskHill.Type;
      hill->WaterEnabled = (diskHill.WaterEnabled_packed & 0xFF) != 0;
      hill->RiverSize    = diskHill.RiverSize;
      // SplineChain / HillMesh / HillMesh2 stay zeroed from the prior memset.
    }

    // Create SplineChain
    auto *splineChain = new SChain<SSplineRing>();
    splineChain->first = nullptr;
    splineChain->last = nullptr;
    splineChain->current = nullptr;
    splineChain->Closed = false;
    splineChain->NumItems = 0;
    HillChain.current->SplineChain = splineChain;

    while (true) {
      if (is->ReadInt() != 123456)
        Logger.g->Panic("SGepard::ReadSign: Attempt to read sign, but no sign here!");
      if (!is->ReadInt()) break;

      // Create new SplineRing
      SSplineRing *sr = (SSplineRing*)::operator new(sizeof(SSplineRing));
      memset(sr, 0, sizeof(SSplineRing));

      // Append to splineChain
      if (splineChain->last) {
        splineChain->last->next = sr;
        sr->prev = splineChain->last;
        splineChain->last = sr;
        sr->next = nullptr;
      } else {
        splineChain->last = sr;
        splineChain->first = sr;
      }
      ++splineChain->NumItems;
      splineChain->current = sr;

      // Read spline properties
      float altitude, altitude2, uratio;
      int checkboxnr, ualign;
      bool closed, middlespline, visibilitystate;

      is->Read(&sr->spline, 4); // placeholder value
      is->Read(&altitude, 4);
      is->Read(&altitude2, 4);
      is->Read(&checkboxnr, 4);
      is->Read(&closed, 1);
      is->Read(&middlespline, 1);
      is->Read(&visibilitystate, 1);
      is->Read(&uratio, 4);
      is->Read(&ualign, 4);

      // Create SSpline
      SSpline *spline = new SSpline(this, middlespline, checkboxnr);
      splineChain->current->spline = spline;
      spline->Altitude = altitude;
      spline->Altitude2 = altitude2;
      spline->Closed = closed;
      spline->VisibilityState = visibilitystate;
      spline->URatio = uratio;
      spline->UAlign = ualign;

      // Read river strip data for second spline ring
      bool isSecondRing = false;
      if (splineChain->first && splineChain->first->next) {
        if (splineChain->Closed) {
          // closed chain check
        } else if (splineChain->current == splineChain->first->next) {
          isSecondRing = true;
        }
      }

      if (isSecondRing) {
        unsigned int numvertices = 0;
        while (true) {
          is->Read(&numvertices, 4);
          if (!numvertices) break;

          if (!spline->RiverStripChain) {
            spline->RiverStripChain = new SChain<SRiverStripRing>();
            spline->RiverStripChain->first = nullptr;
            spline->RiverStripChain->last = nullptr;
            spline->RiverStripChain->current = nullptr;
            spline->RiverStripChain->Closed = false;
            spline->RiverStripChain->NumItems = 0;
          }

          auto *rsr = (SRiverStripRing*)::operator new(sizeof(SRiverStripRing));
          memset(rsr, 0, sizeof(SRiverStripRing));

          auto *rsc = spline->RiverStripChain;
          if (rsc->last) {
            rsc->last->next = rsr;
            rsr->prev = rsc->last;
            rsc->last = rsr;
            rsr->next = nullptr;
          } else {
            rsc->last = rsr;
            rsc->first = rsr;
          }
          ++rsc->NumItems;
          rsc->current = rsr;

          rsr->numvertices = numvertices;
          rsr->strip = (SVertDiff*)::operator new[](24 * numvertices);
          is->Read(rsr->strip, 24 * numvertices);
          rsr->numverify = 0;
        }
      }

      // Read nodes
      while (true) {
        if (is->ReadInt() != 123456)
          Logger.g->Panic("SGepard::ReadSign: Attempt to read sign, but no sign here!");
        if (!is->ReadInt()) break;

        SNode *node = new SNode();
        memset(node, 0, sizeof(SNode));

        if (spline->Nodes.last) {
          spline->Nodes.last->next = node;
          node->prev = spline->Nodes.last;
          spline->Nodes.last = node;
          node->next = nullptr;
        } else {
          spline->Nodes.last = node;
          spline->Nodes.first = node;
        }
        ++spline->Nodes.NumItems;
        spline->Nodes.current = node;

        is->Read(&node->ControlPoint, 28);
      }
    }

    if (is->ReadInt() != 123456)
      Logger.g->Panic("SGepard::ReadSign: Attempt to read sign, but no sign here!");
  }
}

void SGepard::LoadSplinesSel(SStream *is)
{
  int hillIndex, splineIndex;
  is->Read(&hillIndex, 4);
  is->Read(&splineIndex, 4);

  SHillRing *hill = nullptr;
  if (HillChain.NumItems) {
    hill = HillChain.first;
    if (!hill) Logger.g->Panic("GetPointer: SChain is empty!");
    for (int i = 0; i < hillIndex; ++i) {
      hill = hill->next;
      if (!hill) Logger.g->Panic("GetPointer: the given index is invalid!");
    }
  }
  gpSelHillRing = hill;

  if (hill && hill->SplineChain && hill->SplineChain->NumItems) {
    SSplineRing *sr = hill->SplineChain->first;
    if (!sr) Logger.g->Panic("GetPointer: SChain is empty!");
    for (int i = 0; i < splineIndex; ++i) {
      sr = sr->next;
      if (!sr) Logger.g->Panic("GetPointer: the given index is invalid!");
    }
    gpSelSplineRing = sr;
  } else {
    gpSelSplineRing = nullptr;
  }
}

// --- Terrain/spline editing ---

void SGepard::MoveShader2(SShader2Info *s2i, float _x, float _z, bool _snaptocenter)
{
  if (!s2i || !s2i->mesh) return;

  if (_snaptocenter) {
    _x = floorf(_x) + 0.5f;
    _z = floorf(_z) + 0.5f;
  }

  // Validate bounds
  if (_x < 0.0f || _z < 0.0f) return;
  if (_x + (float)(s2i->XVertices - 1) > (float)XSize) return;
  if (_z + (float)(s2i->ZVertices - 1) > (float)ZSize) return;

  s2i->xpos = _x;
  s2i->zpos = _z;

  SMesh *mesh = s2i->mesh;
  mesh->LockVertexBuffer();
  int vi = 0;
  for (int z = 0; z < s2i->ZVertices; ++z) {
    for (int x = 0; x < s2i->XVertices; ++x) {
      float wx = _x + (float)x;
      float wz = _z + (float)z;
      float *pos = (float*)((char*)mesh->lpVertices + mesh->OffsetXYZ + vi * mesh->VertexSize);
      pos[0] = wx;
      pos[1] = (float)(Terrain->GetHeight((int)wx, (int)wz) + 0.01f);

      pos[2] = wz;

      // Set normal from terrain
      float *nrm = (float*)((char*)mesh->lpVertices + mesh->OffsetNormal + vi * mesh->VertexSize);
      D3DXVECTOR3 normal;
      Terrain->GetNormal(&normal, (int)wx, (int)wz);
      nrm[0] = normal.x;
      nrm[1] = normal.y;
      nrm[2] = normal.z;
      ++vi;
    }
  }
  mesh->UnlockVertexBuffer();
}

//----- (0043A040) --------------------------------------------------------
void SGepard::MoveSplineNode(SHillRing *_hillring, SSplineRing *_splinering, SNode *_node, float x, float y, float z)
{
  _splinering->spline->MoveNode(_node, x, y, z);
  RecalculateCentralNodes(_hillring);
  RefreshSubNodes(_hillring);
}

//----- (0043A0B0) --------------------------------------------------------
void SGepard::PasteShaders(SDArray<SShaderClipboard> *shaders, int x0, int z0)
{
  for (int i = 0; i < shaders->size; ++i) {
    const char *buf = shaders->array[i].filename ? shaders->array[i].filename : "";
    CreateShader((float)(x0 + shaders->array[i].X), (float)(z0 + shaders->array[i].Z),
                 buf, shaders->array[i].Rotation, 1, 0);
  }
}

//----- (0043A6A0) --------------------------------------------------------
void SGepard::RecalculateCentralNodes(SHillRing *_hillring)
{
  if (!_hillring)
    return;
  SChain<SSplineRing> *chain = _hillring->SplineChain;
  if (chain->NumItems <= 2)
    return;

  // Start at second element
  chain->current = chain->first;
  // StepToNext
  if (chain->Closed) {
    if (!chain->current)
      Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
    SSplineRing *n = chain->current->next;
    chain->current = n ? n : chain->first;
  } else if (chain->current) {
    chain->current = chain->current->next;
  } else {
    chain->current = nullptr;
  }

  while (chain->current) {
    SSplineRing *last = chain->Closed && !chain->last ?
      (Logger.g->Panic("GetLast: In a closed chain <last> is NULL"), (SSplineRing*)nullptr) : chain->last;
    if (chain->current == last)
      return;

    SSplineRing *first;
    if (chain->Closed) {
      if (!chain->last)
        Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
      first = chain->first;
      if (!first)
        Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
    } else {
      first = chain->first;
    }
    chain->current->spline->ReplaceNodesFrom(first->spline, chain->last->spline);

    // StepToNext
    SSplineRing *cur = chain->current;
    if (chain->Closed) {
      if (!cur)
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      SSplineRing *n = cur->next;
      chain->current = n ? n : chain->first;
    } else if (cur) {
      chain->current = cur->next;
    } else {
      chain->current = nullptr;
    }
  }
}

//----- (0043A760) --------------------------------------------------------
void SGepard::RecalculateControlPoints(SHillRing *_hillring)
{
  SChain<SSplineRing> *chain = _hillring->SplineChain;
  chain->current = chain->first;
  while (chain->current) {
    chain->current->spline->RecalculateControlPoints();
    // StepToNext
    SSplineRing *cur = chain->current;
    if (chain->Closed) {
      if (!cur)
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      SSplineRing *n = cur->next;
      chain->current = n ? n : chain->first;
    } else if (cur) {
      chain->current = cur->next;
    } else {
      chain->current = nullptr;
    }
  }
}

//----- (0043A7E0) --------------------------------------------------------
void SGepard::RecalculateNodesAltitude()
{
  SHillRing *hill = HillChain.first;
  HillChain.current = hill;
  while (hill) {
    RecalculateControlPoints(hill);
    RecalculateCentralNodes(HillChain.current);
    RefreshSubNodes(HillChain.current);
    // StepToNext
    SHillRing *cur = HillChain.current;
    if (HillChain.Closed) {
      if (!cur)
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      SHillRing *n = cur->next;
      hill = n ? n : HillChain.first;
    } else if (cur) {
      hill = cur->next;
    } else {
      hill = nullptr;
    }
    HillChain.current = hill;
  }
}

//----- (0043A880) --------------------------------------------------------
void SGepard::RefreshHill(SHillRing *_hillring)
{
  RecalculateCentralNodes(_hillring);
  RefreshSubNodes(_hillring);
  RedrawHill(_hillring);
}

//----- (0043A8B0) --------------------------------------------------------
void SGepard::RefreshIndices(void *handle)
{
  if (Effect)
    Effect->RefreshIndices((SParticles**)handle);
}

void SGepard::RefreshShader(SShaderInfo *si)
{
  float xEnd = floorf((float)(si->XVertices - 1)) + floorf(si->xpos);
  if (xEnd > (float)XSize) return;
  float zEnd = floorf((float)(si->ZVertices - 1)) + floorf(si->zpos);
  if (zEnd > (float)ZSize) return;
  if (si->xpos < 0.0f || si->zpos < 0.0f) return;

  SMesh *mesh = si->mesh;
  mesh->LockVertexBuffer();
  int vi = 0;
  for (int z = 0; z < si->ZVertices; ++z) {
    for (int x = 0; x < si->XVertices; ++x) {
      float wx = floorf(si->xpos) + (float)x;
      float wz = floorf(si->zpos) + (float)z;
      float height = (float)(Terrain->GetHeight((int)wx, (int)wz));

      float *pos = (float*)((char*)mesh->lpVertices + mesh->OffsetXYZ + vi * mesh->VertexSize);
      pos[1] = height;
      pos[0] = wx;
      pos[2] = wz;
      D3DXVECTOR3 result;
      D3DXVECTOR3 *nrm = (D3DXVECTOR3*)((char*)mesh->lpVertices + mesh->OffsetNormal + vi * mesh->VertexSize);
      *nrm = *Terrain->GetEdgeNormal(&result, wx, wz);
      ++vi;
    }
  }
  mesh->UnlockVertexBuffer();
}

//----- (0043B2B0) --------------------------------------------------------
void SGepard::RelitShader2(SShader2Info *s2i)
{
  SMesh *mesh = s2i->mesh;
  int xVerts = s2i->XVertices;
  int zVerts = s2i->ZVertices;

  mesh->LockVertexBuffer();
  int vi = 0;
  if (s2i->ForceBright) {
    unsigned char alpha = (unsigned char)(s2i->Erosseg * 255.0f);
    unsigned char r = (unsigned char)(s2i->R * 255.0f);
    unsigned char g = (unsigned char)(s2i->G * 255.0f);
    unsigned char b = (unsigned char)(s2i->B * 255.0f);
    DWORD color = (alpha << 24) | (r << 16) | (g << 8) | b;
    for (int z = 0; z < zVerts; ++z) {
      for (int x = 0; x < xVerts; ++x) {
        *(DWORD *)((char *)mesh->lpVertices + mesh->OffsetDiffuse + vi * mesh->VertexSize) = color;
        ++vi;
      }
    }
  } else {
    for (int z = 0; z < zVerts; ++z) {
      for (int x = 0; x < xVerts; ++x) {
        unsigned char alpha = (unsigned char)(s2i->Erosseg * 255.0f);
        unsigned char r = (unsigned char)(s2i->vertexlight[vi].r * 255.0f);
        unsigned char g = (unsigned char)(s2i->vertexlight[vi].g * 255.0f);
        unsigned char b = (unsigned char)(s2i->vertexlight[vi].b * 255.0f);
        *(DWORD *)((char *)mesh->lpVertices + mesh->OffsetDiffuse + vi * mesh->VertexSize) = (alpha << 24) | (r << 16) | (g << 8) | b;
        ++vi;
      }
    }
  }
  mesh->UnlockVertexBuffer();
}

// --- Node selection ---

void SGepard::GetSelectedNode(float x, float y, float z, SHillRing **_hillring, SSplineRing **_level, SNode **_node, bool _lockhillselection)
{
  SSplineRing *savedLevel = *_level;

  // Transform click position to screen space
  D3DXVECTOR3 clickVec(x, y, z);
  D3DXVec3TransformCoord(&clickVec, &clickVec, &CameraMatrix);
  float clickSX = -1.0f, clickSY = -1.0f;
  if (clickVec.z > 0.0f) {
    float invZ = 1.0f / clickVec.z;
    clickSX = ProjectionMatrix._11 * 0.5f * clickVec.x * invZ + 0.5f;
    clickSY = 0.5f - ProjectionMatrix._22 * 0.5f * clickVec.y * invZ;
  }

  SNodeInfo *ni = NodeInfos.first;
  NodeInfos.current = ni;
  while (ni) {
    SNode *node = ni->node;
    D3DXVECTOR3 nodeVec(node->x, ni->magassag, node->z);
    D3DXVec3TransformCoord(&nodeVec, &nodeVec, &CameraMatrix);
    float nodeSX = -1.0f, nodeSY = -1.0f;
    if (nodeVec.z > 0.0f) {
      float invZ = 1.0f / nodeVec.z;
      nodeSX = ProjectionMatrix._11 * 0.5f * nodeVec.x * invZ + 0.5f;
      nodeSY = 0.5f - ProjectionMatrix._22 * 0.5f * nodeVec.y * invZ;
    }

    if (fabsf(clickSX - nodeSX) < 0.015f && fabsf(clickSY - nodeSY) < 0.015f
      && (!_lockhillselection || ni->splinering == savedLevel)
      && !ni->splinering->spline->MiddleSpline
      && ni->node->ControlPoint)
    {
      *_hillring = ni->hillring;
      *_level = ni->splinering;
      *_node = ni->node;
      return;
    }

    // Step to next
    SNodeInfo *cur = NodeInfos.current;
    SNodeInfo *next;
    if (NodeInfos.Closed) {
      if (!cur) Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      next = cur->next;
      if (!next) next = (SNodeInfo*)HillChain.first; // wrap around
    } else {
      next = cur ? cur->next : nullptr;
    }
    ni = next;
    NodeInfos.current = next;
  }
  *_hillring = nullptr;
  *_level = nullptr;
  *_node = nullptr;
}

void SGepard::GetSelectedNode2(float clickx, float clicky, SHillRing **_hillring, SSplineRing **_level, SNode **_node, bool _lockhillselection)
{
  SSplineRing *savedLevel = *_level;

  SNodeInfo *ni = NodeInfos.first;
  NodeInfos.current = ni;
  while (ni) {
    SNode *node = ni->node;
    D3DXVECTOR3 nodeVec(node->x, ni->magassag, node->z);
    D3DXVec3TransformCoord(&nodeVec, &nodeVec, &CameraMatrix);
    float nodeSX = -1.0f, nodeSY = -1.0f;
    if (nodeVec.z > 0.0f) {
      float invZ = 1.0f / nodeVec.z;
      nodeSX = ProjectionMatrix._11 * 0.5f * nodeVec.x * invZ + 0.5f;
      nodeSY = 0.5f - ProjectionMatrix._22 * 0.5f * nodeVec.y * invZ;
    }

    if (fabsf(clickx - nodeSX) < 0.015f && fabsf(clicky - nodeSY) < 0.015f
      && (!_lockhillselection || ni->splinering == savedLevel)
      && !ni->splinering->spline->MiddleSpline
      && ni->node->ControlPoint)
    {
      *_hillring = ni->hillring;
      *_level = ni->splinering;
      *_node = ni->node;
      return;
    }

    // Step to next
    SNodeInfo *cur = NodeInfos.current;
    SNodeInfo *next;
    if (NodeInfos.Closed) {
      if (!cur) Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      next = cur->next;
      if (!next) next = (SNodeInfo*)NodeInfos.first;
    } else {
      next = cur ? cur->next : nullptr;
    }
    ni = next;
    NodeInfos.current = next;
  }
  *_hillring = nullptr;
  *_level = nullptr;
  *_node = nullptr;
}

// --- Effects ---

void SGepard::PlayEffect(int handle, SIObject *object, SDArray<SAblak> *ablakok)
{
  if (Effect)
    Effect->PlayEffect(handle, object, ablakok);
}

void SGepard::PlayEffect(int handle, SIObject *object, char *meshname, int race)
{
  if (Effect)
    Effect->PlayEffect(handle, object, meshname, race);
}

void SGepard::PlayEffect(int handle, float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, float strength, float fade_speed, float u_scale, float v_scale)
{
  if (Effect)
    Effect->PlayEffect(handle, _x1, _y1, _z1, _x2, _y2, _z2, strength, fade_speed, u_scale, v_scale);
}

// --- JezusAtmentAVizenMiMiertNe (river water rendering) ---

// Pre-pass: any river spline whose loaded strip chain contains a degenerate
// entry (numvertices <= 4, i.e. waterleftsize/rightsize collapsed at save
// time because the saving editor probed a HeightMap that had already been
// raised by FixBridges or by JezusAtmentAVizenMiMiertNe→WaterLine→RaiseHeight)
// is rebuilt from spline nodes via DrawWater_ComputeAndStream against
// OldHeightMap. This makes under-bridge water continuous regardless of which
// editor (retail or any vintage of ours) wrote the .scene — no re-save
// required. Pristine retail .scene files have no degenerate strips and skip
// this rebuild, so there's no fidelity cost in the common case.
static bool RiverChain_HasDegenerateStrip(SChain<SRiverStripRing> *rsc)
{
  if (!rsc) return false;
  for (SRiverStripRing *s = rsc->first; s; s = s->next) {
    if (s->numvertices <= 4u) return true;
  }
  return false;
}

static void RiverChain_FreeAndNull(SSpline *sp)
{
  SChain<SRiverStripRing> *rsc = sp->RiverStripChain;
  if (!rsc) return;
  SRiverStripRing *s = rsc->first;
  while (s) {
    SRiverStripRing *next = s->next;
    if (s->strip) ::operator delete[](s->strip);
    ::operator delete(s);
    s = next;
  }
  delete rsc;
  sp->RiverStripChain = 0;
}

void SGepard::JezusAtmentAVizenMiMiertNe()
{
  if ( g_HeadlessMode ) return;
  // Rebuild river chains before computing verify[]. Two triggers:
  //   (a) any strip has collapsed to <=4 verts ("degenerate") — happens on
  //       maps saved by a buggy editor that probed against a modified
  //       HeightMap; regenerating against OldHeightMap restores width.
  //   (b) the map has 2+ WaterEnabled hills — potential Y/T junction;
  //       even if strips are wide individually, adjacent splines may not
  //       share corners (e.g. on mod maps not saved with full joining).
  // The 2-pass sequence always runs joining: pass 1 populates
  // spline->FirstLeftX / LastLeftX with terrain-probed corners (the
  // joining prologue inside compute-and-stream early-outs because every
  // other spline's First/LastLeftX is still -1.0); pass 2 clears the
  // chain and re-emits — this time the prologue finds matches and the
  // strip-gen loop substitutes the joined corners onto neighbour
  // corners, closing the visible junction gap.
  int waterHillCount = 0;
  bool anyDegenerate = false;
  for (SHillRing *hill = HillChain.first; hill; hill = hill->next) {
    if (hill->Type != 0 || !hill->WaterEnabled) continue;
    ++waterHillCount;
    SChain<SSplineRing> *sc = hill->SplineChain;
    if (!sc || !sc->first || !sc->first->next) continue;
    SSpline *sp = sc->first->next->spline;
    if (!sp) continue;
    if (RiverChain_HasDegenerateStrip(sp->RiverStripChain))
      anyDegenerate = true;
  }
  bool needRegen = anyDegenerate || waterHillCount >= 2;
  if (needRegen) {
    Logger.g->Log(0, "RiverChain rebuild: %d WaterEnabled hill(s), degenerate=%d — running 2-pass regen with joining",
      waterHillCount, (int)anyDegenerate);
    // Pass 1: populate FirstLeftX / LastLeftX (joining no-ops because
    // neighbours' endpoints are still -1).
    for (SHillRing *hill = HillChain.first; hill; hill = hill->next) {
      if (hill->Type != 0 || !hill->WaterEnabled) continue;
      SChain<SSplineRing> *sc = hill->SplineChain;
      if (!sc || !sc->first || !sc->first->next) continue;
      SSpline *sp = sc->first->next->spline;
      if (!sp) continue;
      RiverChain_FreeAndNull(sp);
      DrawWater_ComputeAndStream(hill, 0.0f, 0);
    }
    // Pass 2: re-emit with joining prologue finding matches.
    for (SHillRing *hill = HillChain.first; hill; hill = hill->next) {
      if (hill->Type != 0 || !hill->WaterEnabled) continue;
      SChain<SSplineRing> *sc = hill->SplineChain;
      if (!sc || !sc->first || !sc->first->next) continue;
      SSpline *sp = sc->first->next->spline;
      if (!sp) continue;
      RiverChain_FreeAndNull(sp);
      DrawWater_ComputeAndStream(hill, 0.0f, 0);
    }
  }

  SHillRing *first = HillChain.first;
  HillChain.current = first;
  while (first) {
    if (!first->Type && first->WaterEnabled) {
      DrawWater(first, 1, 0.0f, 0);
      first = HillChain.current;
    }
    if (HillChain.Closed) {
      if (!first)
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      first = first->next;
      if (!first)
        first = HillChain.first;
    } else if (first) {
      first = first->next;
    } else {
      first = 0;
    }
    HillChain.current = first;
  }
}

// --- Remove methods ---

void SGepard::RemoveDynamicShader(int idx)
{
  if (idx >= 0 && idx < DynamicShaders.size && DynamicShaders.array[idx].use == 0x7FFFFFFF) {
    ReleaseTexture(DynamicShaders.array[idx].data.TextureIndex, false);
    DynamicShaders.Remove(idx);
  }
}

void SGepard::RemoveGroundTrail(int idx)
{
  if (idx >= 0 && idx < GroundTrails.size && GroundTrails.array[idx].use == 0x7FFFFFFF) {
    ReleaseTexture(GroundTrails.array[idx].data.TextureIndex, false);
    SHeap<SGroundTrailSegment> *segs = GroundTrails.array[idx].data.Segments;
    if (segs) {
      if (segs->array)
        free(segs->array);
      delete segs;
      GroundTrails.array[idx].data.Segments = 0;
    }
    GroundTrails.Remove(idx);
  }
}

void SGepard::RemoveLake(int idx)
{
  if (idx >= 0 && idx < Lakes.size && Lakes.array[idx].use == 0x7FFFFFFF) {
    ReleaseTexture(Lakes.array[idx].data.TextureIndex, false);
    ReleaseTexture(Lakes.array[idx].data.TextureIndex2, false);
    // Release block meshes
    for (int i = 0; i < Lakes.array[idx].data.Blocks.size; ++i) {
      SMesh *m = Lakes.array[idx].data.Blocks.array[i].Mesh;
      if (m) { ((SIObject*)m)->Release(); Lakes.array[idx].data.Blocks.array[i].Mesh = 0; }
      SMesh *m2 = Lakes.array[idx].data.Blocks.array[i].Mesh2;
      if (m2) { ((SIObject*)m2)->Release(); Lakes.array[idx].data.Blocks.array[i].Mesh2 = 0; }
    }
    Lakes.Remove(idx);
  }
}

void SGepard::RemoveRect(int idx)
{
  if (idx >= 0 && idx < Rects.size && Rects.array[idx].use == 0x7FFFFFFF) {
    SMesh *m = Rects.array[idx].data.Mesh;
    if (m)
      ((SIObject*)m)->Release();
    Rects.Remove(idx);
  }
}

void SGepard::RemoveDynamicObject(int idx)
{
  if (idx >= 0 && idx < DynamicObjects.size && DynamicObjects.array[idx].use == 0x7FFFFFFF) {
    int ci = DynamicObjects.array[idx].data.CacheIdx;
    --ObjectCache.array[ci].data.RefCount;
    DynamicObjects.Remove(idx);
  }
}

void SGepard::RemoveHashedObject(int idx)
{
  if (idx >= 0 && idx < StaticObjects.size && StaticObjects.array[idx].use == 0x7FFFFFFF) {
    int ci = StaticObjects.array[idx].data.CacheIdx;
    --ObjectCache.array[ci].data.RefCount;
    StaticObjects.Remove(idx);
    ObjectsHashed = false;
  }
}

void SGepard::RemoveDynamicVB(int idx)
{
  if (idx >= 0 && idx < DynamicVBs.size && DynamicVBs.array[idx].use == 0x7FFFFFFF) {
    IDirect3DVertexBuffer9 *vb = DynamicVBs.array[idx].data.lpVertexBuffer;
    if (vb) {
      vb->Release();
      DynamicVBs.array[idx].data.lpVertexBuffer = 0;
    }
    DynamicVBs.Remove(idx);
  }
}

void SGepard::RemoveSmokeTrail(int idx)
{
  if (idx >= 0 && idx < SmokeTrails.size && SmokeTrails.array[idx].use == 0x7FFFFFFF) {
    ReleaseTexture(SmokeTrails.array[idx].data.TextureIndex, false);
    SDArray<SSmokeTrailPoint> *pts = SmokeTrails.array[idx].data.Points;
    if (pts) {
      if (pts->array)
        free(pts->array);
      delete pts;
      SmokeTrails.array[idx].data.Points = 0;
    }
    SmokeTrails.Remove(idx);
  }
}

// Shadow box cube geometry for stencil rendering
static const float cubeVertices[24] = {
  -1.0f, -1.0f,  1.0f,   1.0f, -1.0f,  1.0f,
  -1.0f,  1.0f,  1.0f,   1.0f,  1.0f,  1.0f,
  -1.0f, -1.0f, -1.0f,   1.0f, -1.0f, -1.0f,
  -1.0f,  1.0f, -1.0f,   1.0f,  1.0f, -1.0f,
};
static const unsigned short cubeIndices[14] = { 0, 1, 2, 3, 7, 1, 5, 4, 7, 6, 2, 4, 0, 1 };

// --- Render sub-methods ---

void SGepard::RenderDebugLines()
{
  int count = debugLines.count;
  if (!count) return;

  // Vertex format: position + diffuse color = D3DFVF_XYZ | D3DFVF_DIFFUSE = 0x42
  struct DebugLineVertex {
    D3DXVECTOR3 v;
    unsigned int color;
  };
  static SDArray<DebugLineVertex> debugLineVertices = {0};

  unsigned int needed = 2 * count;
  // Resize if needed
  if (needed > (unsigned int)debugLineVertices.maxsize) {
    auto *arr = (DebugLineVertex*)realloc(debugLineVertices.array, sizeof(DebugLineVertex) * needed);
    debugLineVertices.array = arr;
    debugLineVertices.maxsize = needed;
  }
  debugLineVertices.size = needed;

  // Fill vertex pairs from debug lines
  for (int i = 0; i < count; ++i) {
    int vi = i * 2;
    debugLineVertices.array[vi].v.x = debugLines.array[i].start.x;
    debugLineVertices.array[vi].v.y = debugLines.array[i].start.y;
    debugLineVertices.array[vi].v.z = debugLines.array[i].start.z;
    debugLineVertices.array[vi].color = debugLines.array[i].color;
    debugLineVertices.array[vi + 1].v.x = debugLines.array[i].end.x;
    debugLineVertices.array[vi + 1].v.y = debugLines.array[i].end.y;
    debugLineVertices.array[vi + 1].v.z = debugLines.array[i].end.z;
    debugLineVertices.array[vi + 1].color = debugLines.array[i].color;
  }

  lpD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
  DrawType = DT_NORMAL;
  SetTexture(0, -1, 1);
  lpD3DDev->SetFVF(D3DFVF_XYZ | D3DFVF_DIFFUSE);
  lpD3DDev->DrawPrimitiveUP(D3DPT_LINELIST, count, debugLineVertices.array, 16);
}

void SGepard::RenderDecals(bool lit)
{
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);
  SetWorldViewProjVertexShaderConstantBuffer(&IdentityMatrix);

  if (lit) {
    EnableFog();
    lpD3DDev->SetVertexShader(decalVertexShader.Ptr);
    lpD3DDev->SetPixelShader(decalPixelShader.Ptr);
  } else {
    lpD3DDev->SetVertexShader(unlitDecalVertexShader.Ptr);
    lpD3DDev->SetPixelShader(unlitDecalPixelShader.Ptr);
  }

  SShader2Info *s2i = Shader2Infos.first;
  Shader2Infos.current = s2i;
  while (s2i) {
    // Guard against corrupt shader2 nodes (dangling pointers from chain management bugs)
    if (!s2i->mesh || !s2i->vertexlight) {
      SShader2Info *cur = Shader2Infos.current;
      SShader2Info *next = cur ? cur->next : nullptr;
      s2i = next;
      Shader2Infos.current = next;
      continue;
    }
    if ((s2i->DrawType == DT_NORMAL) == lit) {
      DrawType = s2i->DrawType;
      SetTexture(0, s2i->ShaderTHandle, 1);

      if (lit) {
        // Update vertex diffuse colors
        SMesh *mesh = s2i->mesh;
        int xv = s2i->XVertices;
        int zv = s2i->ZVertices;
        mesh->LockVertexBuffer();
        int vi = 0;
        for (int z = zv; z > 0; --z) {
          for (int x = 0; x < xv; ++x) {
            unsigned int color;
            unsigned char alpha = (unsigned char)(int)(s2i->Erosseg * 255.0f);
            if (s2i->ForceBright) {
              color = (alpha << 24) | ((int)(s2i->R * 255.0f) << 16) | ((int)(s2i->G * 255.0f) << 8) | (int)(s2i->B * 255.0f);
            } else {
              color = (alpha << 24)
                | ((int)(s2i->vertexlight[vi].r * 255.0f) << 16)
                | ((int)(s2i->vertexlight[vi].g * 255.0f) << 8)
                | (int)(s2i->vertexlight[vi].b * 255.0f);
            }
            *(unsigned int*)((char*)mesh->lpVertices + mesh->OffsetDiffuse + vi * mesh->VertexSize) = color;
            ++vi;
          }
        }
        mesh->UnlockVertexBuffer();
        s2i->mesh->Draw(0, 0, 0.0f);
      } else {
        // Unlit decals
        if (s2i->DrawType == DT_ADD)
          lpD3DDev->SetRenderState(D3DRS_FOGENABLE, FALSE);
        else
          EnableFog();

        float erosseg = s2i->Erosseg;
        if (!s2i->ForceBright && Terrain) {
          unsigned char *visMap = Terrain->VisMap;
          if (visMap && !Terrain->GodMode) {
            int vx = (int)(s2i->xpos * 2.0f);
            int vz = (int)(s2i->zpos * -2.0f);
            if (!visMap[vx - 2 * (Terrain->XSize + 1) * vz])
              erosseg *= 0.625f;
          }
        }

        float decalColor[4] = { erosseg, erosseg, erosseg, 1.0f };
        lpD3DDev->SetPixelShaderConstantF(0, decalColor, 1);
        s2i->mesh->Draw(0, 0, 0.0f);
      }
    }

    // Step to next in chain
    SShader2Info *cur = Shader2Infos.current;
    SShader2Info *next;
    if (Shader2Infos.Closed) {
      next = cur->next;
      if (!next) next = Shader2Infos.first;
    } else {
      next = cur ? cur->next : nullptr;
    }
    s2i = next;
    Shader2Infos.current = next;
  }

  if (lit)
    lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
  lpD3DDev->SetVertexShader(nullptr);
  lpD3DDev->SetPixelShader(nullptr);
}

void SGepard::RenderDynamicShaders()
{
  // Vertex: D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_SPECULAR | D3DFVF_TEX1 = 0x1C4 = 452
  struct DynShaderVert {
    float x, y, z;
    float rhw;
    unsigned int color;
    unsigned int specular;
    float u, v;
  };

  lpD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
  DrawType = DT_NORMAL;
  lpD3DDev->SetFVF(452);
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

  int idx = -1;
  while (true) {
    int size = DynamicShaders.size;
    if (++idx >= size) break;
    while (DynamicShaders.array[idx].use != 0x7FFFFFFF) {
      if (++idx >= size) goto done;
    }
    if (idx < 0) break;

    SetTexture(0, DynamicShaders.array[idx].data.TextureIndex, 1);

    DynShaderVert vert[4];
    float dsX = DynamicShaders.array[idx].data.X;
    float dsZ = DynamicShaders.array[idx].data.Z;
    float dsSize = DynamicShaders.array[idx].data.Size;

    TransformGroundPoint(0, dsX - dsSize, dsZ - dsSize, &vert[0].x, &vert[0].y, &vert[0].z, &vert[0].rhw, (unsigned int*)&vert[0].specular, 0.025f, 0.0025f);
    TransformGroundPoint(0, dsX + dsSize, dsZ - dsSize, &vert[1].x, &vert[1].y, &vert[1].z, &vert[1].rhw, (unsigned int*)&vert[1].specular, 0.025f, 0.0025f);
    TransformGroundPoint(0, dsX - dsSize, dsZ + dsSize, &vert[2].x, &vert[2].y, &vert[2].z, &vert[2].rhw, (unsigned int*)&vert[2].specular, 0.025f, 0.0025f);
    TransformGroundPoint(0, dsX + dsSize, dsZ + dsSize, &vert[3].x, &vert[3].y, &vert[3].z, &vert[3].rhw, (unsigned int*)&vert[3].specular, 0.025f, 0.0025f);

    if (vert[0].rhw >= 0.0f && vert[1].rhw >= 0.0f && vert[2].rhw >= 0.0f && vert[3].rhw >= 0.0f) {
      vert[0].u = 0.0f; vert[0].v = 0.0f;
      vert[1].u = 0.0f; vert[1].v = 1.0f;
      vert[2].u = 1.0f; vert[2].v = 0.0f;
      vert[3].u = 1.0f; vert[3].v = 1.0f;
      unsigned int color = DynamicShaders.array[idx].data.Color;
      vert[0].color = color;
      vert[1].color = color;
      vert[2].color = color;
      vert[3].color = color;
      lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, 32);
    }
  }
done:
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
}

void SGepard::RenderGroundTrails()
{
  struct TrailVert {
    D3DXVECTOR3 vec;
    unsigned int color;
    float u, v;
  };

  lpD3DDev->SetRenderState(D3DRS_FOGCOLOR, 0);
  lpD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
  lpD3DDev->SetFVF(322);

  D3DXMATRIX identity;
  D3DXMatrixIdentity(&identity);
  lpD3DDev->SetTransform(D3DTS_WORLD, &identity);
  lpD3DDev->SetRenderState(D3DRS_DEPTHBIAS, 0xBB200000u); // small negative bias

  int trailIdx = -1;
  while (true) {
    int size = GroundTrails.size;
    int nextIdx = trailIdx + 1;
    if (nextIdx >= size) break;
    while (GroundTrails.array[nextIdx].use != 0x7FFFFFFF) {
      if (++nextIdx >= size) goto done;
    }
    if (nextIdx < 0) break;
    trailIdx = nextIdx;

    auto &trail = GroundTrails.array[trailIdx].data;
    DrawType = trail.DrawType;
    SetTexture(0, trail.TextureIndex, 1);

    auto *segs = trail.Segments;
    int segIdx = -1;
    while (true) {
      ++segIdx;
      int segSize = segs->size;
      if (segIdx >= segSize) segIdx = -1;
      else {
        while (segs->array[segIdx].use != 0x7FFFFFFF) {
          if (++segIdx >= segSize) { segIdx = -1; break; }
        }
      }
      if (segIdx < 0) break;

      auto &seg = segs->array[segIdx].data;
      float elapsed = (float)(WorldTime - seg.StartTime);
      if (elapsed > trail.FadeTime) {
        // Remove expired segment
        segs->array[segIdx].use = segs->nextempty;
        --segs->occupied;
        segs->nextempty = segIdx;
        continue;
      }

      // Visibility check
      if (!seg.Noticed) {
        if (Terrain) {
          unsigned char *visMap = Terrain->VisMap;
          if (visMap && !Terrain->GodMode) {
            float sx = seg.Vertices[0].x;
            float sz = seg.Vertices[0].z;
            if (!visMap[(int)(sx + sx) - 2 * (Terrain->XSize + 1) * (int)(sz * -2.0f)])
              continue;
          }
        }
        seg.Noticed = true;
      }

      int alpha = (int)((1.0f - elapsed / trail.FadeTime) * trail.Strength);
      if (alpha > 255) alpha = 255;
      if (alpha < 0) alpha = 0;

      TrailVert vert[4];
      vert[0].vec = seg.Vertices[0];
      vert[1].vec = seg.Vertices[1];
      vert[2].vec = seg.Vertices[2];
      vert[3].vec = seg.Vertices[3];

      unsigned int col = 65793 * alpha; // 0x010101 * alpha
      vert[0].color = col;
      vert[1].color = col;
      vert[2].color = col;
      vert[3].color = col;
      vert[0].u = 0.0f; vert[0].v = 0.0f;
      vert[1].u = 0.0f; vert[1].v = 1.0f;
      vert[2].u = 1.0f; vert[2].v = 0.0f;
      vert[3].u = 1.0f; vert[3].v = 1.0f;

      lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, 24);
    }

    // Auto-destruct if empty
    if (trail.AutoDestruct && segs->occupied == 0)
      RemoveGroundTrail(trailIdx);
  }
done:
  lpD3DDev->SetRenderState(D3DRS_DEPTHBIAS, 0);
  EnableFog();
}

void SGepard::RenderLakes()
{
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
  lpD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
  DrawType = DT_NORMAL;

  D3DXMATRIX d3dm;
  D3DXMatrixIdentity(&d3dm);
  lpD3DDev->SetTransform((D3DTRANSFORMSTATETYPE)256, &d3dm);

  int lakeIdx = -1;
  while (1)
  {
    int size = Lakes.size;
    ++lakeIdx;
    if (lakeIdx >= size)
      break;

    // Skip unused heap entries
    SHeap<SGLake>::__Tstruct *entry = &Lakes.array[lakeIdx];
    while (entry->use != 0x7FFFFFFF)
    {
      ++lakeIdx;
      ++entry;
      if (lakeIdx >= size)
        goto done;
    }
    if (lakeIdx < 0)
      break;

    SGLake &lake = Lakes.array[lakeIdx].data;
    if (lake.Blocks.size <= 0)
      continue;

    for (int blockIdx = 0; blockIdx < lake.Blocks.size; ++blockIdx)
    {
      SLakeBlock &block = lake.Blocks.array[blockIdx];

      if (!Terrain->Parcels[block.BlockIdx].Visible)
        continue;

      SMesh *mesh = block.Mesh;
      SMesh *sparkleMesh = block.Mesh2;
      float Y = lake.Y;
      float t = (float)(int)Timer.GetTickValue24bit() * 0.001f;

      mesh->LockVertexBuffer();

      // Height-based transparency recalculation
      if (block.RecalcHeightTransparency)
      {
        float waterY = lake.Y;
        unsigned int baseColor = lake.Color;
        unsigned int numVerts = mesh->GetNumVertices();
        for (unsigned int v = 0; v < numVerts; ++v)
        {
          int vtxOfs = mesh->OffsetXYZ + v * mesh->VertexSize;
          char *verts = (char *)mesh->lpVertices;
          float vx = *(float *)(verts + vtxOfs);
          float vz = *(float *)(verts + vtxOfs + 8);
          float terrainH = (float)Terrain->GetOldHeight(vx, vz);

          unsigned int color;
          if (terrainH > waterY)
          {
            color = baseColor; // fully transparent (alpha = 0)
          }
          else
          {
            float depth = waterY - terrainH;
            if (depth >= 1.0f)
            {
              if (depth >= 3.0f)
                color = baseColor | 0xFF000000;
              else
                color = baseColor | ((unsigned int)((depth - 1.0f) * 32.0f + 191.0f) << 24);
            }
            else
            {
              color = baseColor | ((unsigned int)(depth * 192.0f) << 24);
            }
          }
          *(unsigned int *)((char *)mesh->lpVertices + mesh->OffsetDiffuse + v * mesh->VertexSize) = color;
        }
        block.RecalcHeightTransparency = 0;
      }

      // Primary mesh vertex animation — 5 flow types
      // SSE arguments recovered from Ghidra v1.7 decompile
      unsigned int numVerts = mesh->GetNumVertices();
      switch (lake.Flow)
      {
      case 0:
        for (unsigned int v = 0; v < numVerts; ++v)
        {
          char *verts = (char *)mesh->lpVertices;
          int ofs = v * mesh->VertexSize;
          float vx = *(float *)(verts + mesh->OffsetXYZ + ofs);
          float vz = *(float *)(verts + mesh->OffsetXYZ + ofs + 8);
          *(float *)(verts + mesh->OffsetXYZ + ofs + 4) = Y;
          *(float *)(verts + mesh->OffsetTexture1 + ofs) = sinf(vx * 0.5f + t) * 0.01f + vx * 0.0625f;
          *(float *)(verts + mesh->OffsetTexture1 + ofs + 4) = cosf(vz * 0.5f + t) * 0.01f + vz * 0.0625f;
        }
        break;
      case 1:
        for (unsigned int v = 0; v < numVerts; ++v)
        {
          char *verts = (char *)mesh->lpVertices;
          int ofs = v * mesh->VertexSize;
          float vx = *(float *)(verts + mesh->OffsetXYZ + ofs);
          float vz = *(float *)(verts + mesh->OffsetXYZ + ofs + 8);
          *(float *)(verts + mesh->OffsetXYZ + ofs + 4) = sinf(vz * 0.5f - t) * 0.125f + Y;
          *(float *)(verts + mesh->OffsetTexture1 + ofs) = vx * 0.0625f;
          *(float *)(verts + mesh->OffsetTexture1 + ofs + 4) = vz * 0.0625f - t * 0.02f;
        }
        break;
      case 2:
        for (unsigned int v = 0; v < numVerts; ++v)
        {
          char *verts = (char *)mesh->lpVertices;
          int ofs = v * mesh->VertexSize;
          float vx = *(float *)(verts + mesh->OffsetXYZ + ofs);
          float vz = *(float *)(verts + mesh->OffsetXYZ + ofs + 8);
          *(float *)(verts + mesh->OffsetXYZ + ofs + 4) = sinf(vx * 0.5f - t) * 0.125f + Y;
          *(float *)(verts + mesh->OffsetTexture1 + ofs) = vz * 0.0625f;
          *(float *)(verts + mesh->OffsetTexture1 + ofs + 4) = vx * 0.0625f - t * 0.02f;
        }
        break;
      case 3:
        for (unsigned int v = 0; v < numVerts; ++v)
        {
          char *verts = (char *)mesh->lpVertices;
          int ofs = v * mesh->VertexSize;
          float vx = *(float *)(verts + mesh->OffsetXYZ + ofs);
          float vz = *(float *)(verts + mesh->OffsetXYZ + ofs + 8);
          *(float *)(verts + mesh->OffsetXYZ + ofs + 4) = sinf(vz * 0.5f + t) * 0.125f + Y;
          *(float *)(verts + mesh->OffsetTexture1 + ofs) = vx * 0.0625f;
          *(float *)(verts + mesh->OffsetTexture1 + ofs + 4) = vz * 0.0625f + t * 0.02f;
        }
        break;
      case 4:
        for (unsigned int v = 0; v < numVerts; ++v)
        {
          char *verts = (char *)mesh->lpVertices;
          int ofs = v * mesh->VertexSize;
          float vx = *(float *)(verts + mesh->OffsetXYZ + ofs);
          float vz = *(float *)(verts + mesh->OffsetXYZ + ofs + 8);
          *(float *)(verts + mesh->OffsetXYZ + ofs + 4) = sinf(vx * 0.5f + t) * 0.125f + Y;
          *(float *)(verts + mesh->OffsetTexture1 + ofs) = vz * 0.0625f;
          *(float *)(verts + mesh->OffsetTexture1 + ofs + 4) = vx * 0.0625f + t * 0.02f;
        }
        break;
      }
      mesh->UnlockVertexBuffer();

      // Sparkle mesh animation
      if (lake.Sparkle)
      {
        sparkleMesh->LockVertexBuffer();
        unsigned int sparkleVerts = mesh->GetNumVertices();
        float sparkleScroll = t * 0.025f;

        for (unsigned int v = 0; v < sparkleVerts; ++v)
        {
          char *sv = (char *)sparkleMesh->lpVertices;
          int ofs = v * sparkleMesh->VertexSize;
          float svx = *(float *)(sv + sparkleMesh->OffsetXYZ + ofs);
          float svz = *(float *)(sv + sparkleMesh->OffsetXYZ + ofs + 8);

          // Y position — flat for flow 0, wave for flows 1-4
          switch (lake.Flow)
          {
          case 0:
            *(float *)(sv + sparkleMesh->OffsetXYZ + ofs + 4) = Y;
            break;
          case 1:
            *(float *)(sv + sparkleMesh->OffsetXYZ + ofs + 4) = sinf(svz * 0.5f - t) * 0.125f + Y;
            break;
          case 2:
            *(float *)(sv + sparkleMesh->OffsetXYZ + ofs + 4) = sinf(svx * 0.5f - t) * 0.125f + Y;
            break;
          case 3:
            *(float *)(sv + sparkleMesh->OffsetXYZ + ofs + 4) = sinf(svz * 0.5f + t) * 0.125f + Y;
            break;
          case 4:
            *(float *)(sv + sparkleMesh->OffsetXYZ + ofs + 4) = sinf(svx * 0.5f + t) * 0.125f + Y;
            break;
          }

          // tex1.u: sin(vtx.x * 0.419 + t) * 0.02 + vtx.x * 0.125 + sparkleScroll
          *(float *)(sv + sparkleMesh->OffsetTexture1 + ofs) =
            sinf(svx * 0.419f + t) * 0.02f + svx * 0.125f + sparkleScroll;
          // tex1.v: cos(vtx.z * 0.5f + t) * 0.02 + vtx.z * 0.125
          *(float *)(sv + sparkleMesh->OffsetTexture1 + ofs + 4) =
            cosf(svz * 0.5f + t) * 0.02f + svz * 0.125f;
          // tex2.u: vtx.x * 0.125 - sin(vtx.x * 0.5f + t) * 0.02
          *(float *)(sv + sparkleMesh->OffsetTexture2 + ofs) =
            svx * 0.125f - sinf(svx * 0.5f + t) * 0.02f;
          // tex2.v: vtx.z * 0.125 - cos(vtx.z * 0.519 + t) * 0.02 + sparkleScroll
          *(float *)(sv + sparkleMesh->OffsetTexture2 + ofs + 4) =
            svz * 0.125f - cosf(svz * 0.519f + t) * 0.02f + sparkleScroll;
        }
        sparkleMesh->UnlockVertexBuffer();
      }

      // Draw primary lake mesh
      DrawType = DT_NORMAL;
      SetTexture(0, lake.TextureIndex, 1);
      lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
      lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
      lpD3DDev->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
      lpD3DDev->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
      lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
      lpD3DDev->SetRenderState(D3DRS_ALPHAREF, 0);
      lpD3DDev->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);
      lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
      mesh->Draw(0, 0, 0.0f);

      // Draw sparkle overlay
      if (lake.Sparkle)
      {
        DrawType = DT_ADD;
        SetTexture(0, lake.TextureIndex2, 1);
        SetTexture(1, lake.TextureIndex2, 1);
        lpD3DDev->SetRenderState(D3DRS_FOGCOLOR, 0);
        lpD3DDev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
        lpD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
        sparkleMesh->Draw(0, 0, 0.0f);
        EnableFog();
        lpD3DDev->SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
        lpD3DDev->SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 0);
      }
    }
  }
done:
  lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
}

void SGepard::RenderObjectShadowDecals()
{
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_BORDER);
  lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_BORDER);

  if (ShadowQuality >= 2) {
    lpD3DDev->SetRenderState(D3DRS_STENCILENABLE, TRUE);
    lpD3DDev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL);
    lpD3DDev->SetRenderState(D3DRS_STENCILREF, 1);
  }

  lpD3DDev->SetRenderState(D3DRS_FOGCOLOR, 0);
  SetLightingType(LT_AMBIENT);
  DrawType = DT_SHADOW;
  lpD3DDev->SetVertexShader(unlitDecalVertexShader.Ptr);
  lpD3DDev->SetPixelShader(unlitDecalPixelShader.Ptr);
  SetWorldViewProjVertexShaderConstantBuffer(&IdentityMatrix);

  // Compute shadow color from ambient/sun
  float v3 = SunDir.y;
  float r = AmbientColorVal.r;
  float g = AmbientColorVal.g;
  float b = AmbientColorVal.b;
  float shadowColor[4];
  shadowColor[0] = 1.0f - (r / (r - SunColorVal.r * v3));
  shadowColor[1] = 1.0f - (g / (g - SunColorVal.g * v3));
  shadowColor[2] = 1.0f - (b / (b - SunColorVal.b * v3));
  shadowColor[3] = 1.0f;
  lpD3DDev->SetPixelShaderConstantF(0, shadowColor, 1);

  if (Terrain) {
    if (!ObjectsHashed)
      HashObjects();

    // Static objects via hash
    int totalCells = (XSize / 8) * (ZSize / 8);
    int cellIdx = 0;

    while (cellIdx < totalCells) {
      if (Terrain->Parcels[cellIdx].Visible) {
        int next = ObjectHash[cellIdx];
        while (next >= 0) {
          int chainIdx = next;
          int objIdx = ObjectHashChain.array[chainIdx].ObjectIdx;
          if (StaticObjects.array[objIdx].data.LastRendered != RenderCounter) {
            StaticObjects.array[objIdx].data.LastRendered = RenderCounter;
            SGroup *group = StaticObjects.array[objIdx].data.Group;
            if (group->ShadowTexture >= 0 && group->ShadowMesh) {
              SetTexture(0, group->ShadowTexture, 1);
              group->ShadowMesh->Draw(0, 0, 0.0f);
            }
          }
          next = ObjectHashChain.array[chainIdx].Next;
        }
      }
      ++cellIdx;

    }
    ++RenderCounter;

    // Dynamic objects
    int idx = -1;
    while (true) {
      int size = DynamicObjects.size;
      if (++idx >= size) break;
      while (DynamicObjects.array[idx].use != 0x7FFFFFFF) {
        if (++idx >= size) goto cleanup;
      }
      if (idx < 0) break;

      float x, y, z;
      SGroup *group = DynamicObjects.array[idx].data.Group;
      group->GetPosition(&x, &y, &z);

      if (x >= 0.0f
        && (float)Terrain->XSize > x
        && z >= 0.0f
        && (float)Terrain->ZSize > z
        && Terrain->Parcels[-(int)(x * -0.125f) - Terrain->XParcels * (int)(z * -0.125f)].Visible
        && (float)XSize > x
        && (float)ZSize > z
        && Terrain->IsVisible(x, z, group->VisClass))
      {
        if (group->ShadowTexture >= 0 && group->ShadowMesh) {
          SetTexture(0, group->ShadowTexture, 1);
          DynamicObjects.array[idx].data.Group->ShadowMesh->Draw(0, 0, 0.0f);
        }
      }
    }
  }

cleanup:
  EnableFog();
  lpD3DDev->SetVertexShader(nullptr);
  lpD3DDev->SetPixelShader(nullptr);
  if (ShadowQuality >= 2) {
    lpD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    lpD3DDev->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS);
    lpD3DDev->SetRenderState(D3DRS_STENCILREF, 0);
  }
}

//----- (0043EB70) --------------------------------------------------------
void SGepard::RenderObjectShadows(bool minimapmode)
{
  RenderObjects(minimapmode, true);
}

void SGepard::RenderObjects(bool minimapmode, bool shadows)
{
  if (Terrain) {
    int totalCells = (XSize / 8) * (ZSize / 8);
    int cellIdx = 0;

    if (totalCells > 0) {
      do {
        if (Terrain->Parcels[cellIdx].Visible) {
          int next = ObjectHash[cellIdx];
          if (next >= 0) {
            do {
              int chainIdx = next;
              int objIdx = ObjectHashChain.array[chainIdx].ObjectIdx;
              if (StaticObjects.array[objIdx].data.LastRendered != RenderCounter) {
                StaticObjects.array[objIdx].data.LastRendered = RenderCounter;
                SGroup *group = StaticObjects.array[objIdx].data.Group;
                group->Precalculate();
                if (shadows)
                  group->DrawShadow2();
                else
                  group->Draw();
              }
              next = ObjectHashChain.array[chainIdx].Next;
            } while (next >= 0);
          }
        }
        ++cellIdx;
  
      } while (cellIdx < totalCells);
    }
    ++RenderCounter;
  }

  if (!minimapmode) {
    /* Diagnostic: count dynamic objects and track rendering */
    static int dynLogCounter = 0;
    bool dynLog = (++dynLogCounter % 120 == 1);
    int dynTotal = 0, dynRendered = 0, dynBoundsSkip = 0, dynParcelSkip = 0, dynVisSkip = 0;
    int dynFirstIdx = -1;

    int idx = -1;
    while (true) {
      int size = DynamicObjects.size;
      if (++idx >= size) break;
      while (DynamicObjects.array[idx].use != 0x7FFFFFFF) {
        if (++idx >= size) goto dyn_done;
      }
      if (idx < 0) break;

      float x, y, z;
      SGroup *group = DynamicObjects.array[idx].data.Group;
      group->GetPosition(&x, &y, &z);
      dynTotal++;
      if (dynFirstIdx < 0) dynFirstIdx = idx;

      /* Log first 3 objects in detail */
      if (dynLog && dynTotal <= 3) {
        int parcelIdx = (Terrain) ? (-(int)(x * -0.125f) - Terrain->XParcels * (int)(z * -0.125f)) : -1;
        bool parcelVis = (Terrain && parcelIdx >= 0 && parcelIdx < Terrain->NumParcels) ? Terrain->Parcels[parcelIdx].Visible : false;
        bool isVis = Terrain ? Terrain->IsVisible(x, z, group->VisClass) : true;
        #ifdef HD_DEBUG_RENDERER
        Logger.g->Log(0, "  DynObj[%d]: pos=(%.1f,%.1f,%.1f) vc=%d meshes=%d vis=%d type=%d flags=%x parcel=%d(%d) isVis=%d bounds=(%.0f,%.0f)",
          idx, x, y, z, group->VisClass, group->MeshArray.size, (int)group->Visible, group->Type, group->Flags,
          parcelIdx, (int)parcelVis, (int)isVis, (float)XSize, (float)ZSize);
        #endif
      }

      if (Terrain) {
        // Visibility check
        if (x < 0.0f || (float)Terrain->XSize <= x || z < 0.0f || (float)Terrain->ZSize <= z
            || (float)XSize <= x || (float)ZSize <= z) {
          dynBoundsSkip++;
        } else {
          int pi = -(int)(x * -0.125f) - Terrain->XParcels * (int)(z * -0.125f);
          if (!Terrain->Parcels[pi].Visible) {
            dynParcelSkip++;
          } else if (!Terrain->IsVisible(x, z, group->VisClass)) {
            dynVisSkip++;
          } else {
            group->Precalculate();
            if (shadows)
              group->DrawShadow2();
            else
              group->Draw();
            dynRendered++;
          }
        }
      } else {
        group->Precalculate();
        if (shadows)
          group->DrawShadow2();
        else
          group->Draw();
        dynRendered++;
      }
    }
dyn_done:
    if (dynLog) {
      #ifdef HD_DEBUG_RENDERER
      Logger.g->Log(0, "RenderObjects DYNAMIC: heapSize=%d total=%d rendered=%d boundsSkip=%d parcelSkip=%d visSkip=%d shadows=%d",
        DynamicObjects.size, dynTotal, dynRendered, dynBoundsSkip, dynParcelSkip, dynVisSkip, (int)shadows);
      #endif
    }
  }
}

//----- (00440500) --------------------------------------------------------
void SGepard::RenderShadowBoxToStencil()
{
  if (ShadowQuality >= 2) {
    lpD3DDev->SetFVF(D3DFVF_XYZ);
    lpD3DDev->SetTransform(D3DTS_WORLD, &InverseShadowCameraMatrix);
    lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
    lpD3DDev->SetRenderState(D3DRS_STENCILENABLE, TRUE);
    lpD3DDev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE);
    lpD3DDev->SetRenderState(D3DRS_STENCILREF, 1);
    lpD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, 0);
    lpD3DDev->DrawIndexedPrimitiveUP(D3DPT_TRIANGLESTRIP, 0, 8, 12,
      cubeIndices, D3DFMT_INDEX16, cubeVertices, 12);
    lpD3DDev->SetRenderState(D3DRS_STENCILENABLE, FALSE);
    lpD3DDev->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_KEEP);
    lpD3DDev->SetRenderState(D3DRS_STENCILREF, 0);
    lpD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
  }
}

void SGepard::RenderShadowMap(bool minimap)
{
  if (ShadowQuality < 2) {
    // Release shadow textures if quality dropped
    if (ShadowMapTexture.Ptr) {
      ShadowMapTexture.Ptr->Release();
      ShadowMapTexture.Ptr = nullptr;
    }
    if (ShadowMapDepthTexture.Ptr) {
      ShadowMapDepthTexture.Ptr->Release();
      ShadowMapDepthTexture.Ptr = nullptr;
    }
    return;
  }

  // Create shadow map textures if needed
  if (!ShadowMapTexture.Ptr) {
    HRESULT hr;
    char atmstr[200];
    hr = lpD3DDev->CreateTexture(ShadowMapWidth, ShadowMapWidth, 1, D3DUSAGE_RENDERTARGET,
                                  TFShadowNull, D3DPOOL_DEFAULT, &ShadowMapTexture.Ptr, nullptr);
    if (FAILED(hr)) {
      sprintf(atmstr, "%s: %s", "SGepard::RenderShadowMap: CreateTexture failed", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    if (ShadowMapDepthTexture.Ptr) {
      ShadowMapDepthTexture.Ptr->Release();
      ShadowMapDepthTexture.Ptr = nullptr;
    }
    hr = lpD3DDev->CreateTexture(ShadowMapWidth, ShadowMapWidth, 1, D3DUSAGE_DEPTHSTENCIL,
                                  TFShadowDepth, D3DPOOL_DEFAULT, &ShadowMapDepthTexture.Ptr, nullptr);
    if (FAILED(hr)) {
      sprintf(atmstr, "%s: %s", "SGepard::RenderShadowMap: CreateTexture failed", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
  }

  if (!Terrain) return;

  char atmstr[200];

  // Save camera state
  float saveCamX = CameraXPos, saveCamY = CameraYPos, saveCamZ = CameraZPos;
  float saveCamH = CameraHRot, saveCamV = CameraVRot;
  float saveNear = NearPlane, saveFar = FarPlane;
  float saveOrtho = OrthoScale;
  D3DXMATRIX saveProj = ProjectionMatrix;

  // Compute shadow camera position
  float halfDist;
  D3DXVECTOR3 shadowCenter;
  if (minimap) {
    shadowCenter.x = saveCamX;
    shadowCenter.y = saveCamY - 30.0f;
    shadowCenter.z = saveCamZ;
    halfDist = 1.4142f / saveOrtho;
  } else {
    float invP11 = 1.0f / ProjectionMatrix._11;
    float invP22 = 1.0f / ProjectionMatrix._22;
    float invSumSq = invP11 * invP11 + invP22 * invP22;
    float inner = (ShadowDistance + 1.0f) * 0.5f * (invSumSq + 1.0f);
    float shadowCenterDist = fminf(ShadowDistance, inner);
    float diff = shadowCenterDist - ShadowDistance;
    halfDist = sqrtf(ShadowDistance * ShadowDistance * invSumSq + diff * diff);
    SVector fwd;
    GetCameraForward(&fwd);
    shadowCenter.x = fwd.x * shadowCenterDist + saveCamX;
    shadowCenter.y = fwd.y * shadowCenterDist + saveCamY;
    shadowCenter.z = fwd.z * shadowCenterDist + saveCamZ;
  }

  // Set orthogonal projection for shadow
  float invHalf = 1.0f / halfDist;
  SetOrthogonalProjection(invHalf, -halfDist, halfDist);

  // Compute sun angles
  // NOTE: Original sunV was atan(-SunDir.y / sqrt(x²+z²)), then negated by SetViewProperties.
  // Since our SetViewProperties omits vrot negation (see line 8604 comment), we use
  // atan(SunDir.y / sqrt(x²+z²)) directly to produce the same effective angle.
  float sunH = atan2f(SunDir.x, SunDir.z);
  float sunV = atanf(SunDir.y / sqrtf(SunDir.x * SunDir.x + SunDir.z * SunDir.z));
  SetViewProperties(shadowCenter.x, shadowCenter.y, shadowCenter.z, sunH, sunV);

  // Snap to texel grid to reduce swimming
  D3DXMATRIX camT;
  D3DXMatrixTranspose(&camT, &CameraMatrix);
  float texelSize = (halfDist + halfDist) / (float)ShadowMapWidth;
  D3DXVECTOR3 shadowCenterView;
  D3DXVec3TransformNormal(&shadowCenterView, &shadowCenter, &CameraMatrix);
  shadowCenterView.x = roundf(shadowCenterView.x / texelSize) * texelSize;
  shadowCenterView.y = roundf(shadowCenterView.y / texelSize) * texelSize;
  shadowCenterView.z = roundf(shadowCenterView.z / texelSize) * texelSize;
  D3DXVec3TransformNormal(&shadowCenter, &shadowCenterView, &camT);
  SetViewProperties(shadowCenter.x, shadowCenter.y, shadowCenter.z, sunH, sunV);

  // Compute inverse shadow camera matrix and shadow matrix
  D3DXMatrixMultiply(&InverseShadowCameraMatrix, &CameraMatrix, &ProjectionMatrix);
  D3DXMatrixInverse(&InverseShadowCameraMatrix, nullptr, &InverseShadowCameraMatrix);

  // Bias matrix: maps [-1,1] to [0,1] for shadow map lookup
  D3DXMATRIX biasMatrix;
  memset(&biasMatrix, 0, sizeof(biasMatrix));
  biasMatrix._11 = 0.5f;  biasMatrix._22 = -0.5f;  biasMatrix._33 = 1.0f;  biasMatrix._44 = 1.0f;
  biasMatrix._41 = 0.5f;  biasMatrix._42 = 0.5f;

  D3DXMATRIX projBias;
  D3DXMatrixMultiply(&projBias, &ProjectionMatrix, &biasMatrix);
  D3DXMatrixMultiply(&ShadowMatrix, &CameraMatrix, &projBias);

  // Texel offset
  ProjectionMatrix._41 -= 1.0f / (float)ShadowMapWidth;
  ProjectionMatrix._42 += 1.0f / (float)ShadowMapWidth;

  // Get shadow render targets
  IDirect3DSurface9 *shadowRT = nullptr, *shadowDS = nullptr;
  IDirect3DSurface9 *oldRT = nullptr, *oldDS = nullptr;
  HRESULT hr;

  hr = ShadowMapTexture.Ptr->GetSurfaceLevel(0, &shadowRT);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: GetSurfaceLevel failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = ShadowMapDepthTexture.Ptr->GetSurfaceLevel(0, &shadowDS);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: GetSurfaceLevel failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->GetRenderTarget(0, &oldRT);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: GetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  if (!ResolutionScaleSurface.Ptr) {
    hr = lpD3DDev->GetDepthStencilSurface(&oldDS);
    if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: GetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  }

  // Switch to shadow render target
  hr = lpD3DDev->SetRenderTarget(0, shadowRT);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: SetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->SetDepthStencilSurface(shadowDS);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: SetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // Set viewport to cover the full shadow map texture (D3D9 Clear and rendering
  // are limited to the viewport rectangle, which defaults to back buffer size)
  D3DVIEWPORT9 oldVP, shadowVP = { 0, 0, (DWORD)ShadowMapWidth, (DWORD)ShadowMapWidth, 0.0f, 1.0f };
  lpD3DDev->GetViewport(&oldVP);
  lpD3DDev->SetViewport(&shadowVP);

  hr = lpD3DDev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xFFFFFFFF, 1.0f, 0);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::Clear", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->BeginScene();
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::BeginScene", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  lpD3DDev->SetRenderState(D3DRS_ZENABLE, TRUE);
  float depthBias = 0.001f;
  float slopeScale = 2048.0f / (float)ShadowMapWidth * 0.75f;
  lpD3DDev->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&depthBias);
  lpD3DDev->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&slopeScale);
  lpD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, 0);

  // Render shadow casters
  if (ShadowQuality != 2)
    Terrain->DrawShadow2();
  RenderObjects(minimap, true);

  lpD3DDev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xF);
  hr = lpD3DDev->EndScene();
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderScene\\IDirect3DDevice::EndScene", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // Restore viewport before camera/projection restore
  lpD3DDev->SetViewport(&oldVP);

  // Restore camera
  SetViewProperties(saveCamX, saveCamY, saveCamZ, saveCamH, saveCamV);
  if (saveOrtho == 0.0f)
    SetPerspectiveProjection(FOV, saveNear, saveFar);
  else
    SetOrthogonalProjection(saveOrtho, saveNear, saveFar);

  InitRenderStates();

  // Restore render targets
  hr = lpD3DDev->SetRenderTarget(0, oldRT);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: SetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->SetDepthStencilSurface(oldDS);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::RenderToSurface: SetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  lpD3DDev->SetRenderState(D3DRS_DEPTHBIAS, 0);
  lpD3DDev->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);

  if (shadowRT) { shadowRT->Release(); shadowRT = nullptr; }
  if (ShadowMapDepthTexture.Ptr && shadowDS) { shadowDS->Release(); shadowDS = nullptr; }
  if (oldRT) { oldRT->Release(); oldRT = nullptr; }
  if (oldDS) oldDS->Release();
}

void SGepard::RenderSmokeTrails()
{
  // Vertex: pos + diffuse + tex1 = D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1 = 322
  struct SmokeVert {
    float x, y, z;
    unsigned int color;
    float u, v;
  };

  lpD3DDev->SetRenderState(D3DRS_LIGHTING, FALSE);
  lpD3DDev->SetFVF(322);

  float camX = CameraXPos, camY = CameraYPos, camZ = CameraZPos;

  // TriangleFan buffer: 6 vertices per segment
  // [0]=prev_center, [1]=prev_right, [2]=cur_right, [3]=cur_center, [4]=cur_left, [5]=prev_left
  SmokeVert fanVerts[6];

  int trailIdx = -1;
  while (true) {
    int heapSize = SmokeTrails.size;
    int nextIdx = trailIdx + 1;
    if (nextIdx >= heapSize) break;
    while (SmokeTrails.array[nextIdx].use != 0x7FFFFFFF) {
      if (++nextIdx >= heapSize) goto done;
    }
    if (nextIdx < 0) break;
    trailIdx = nextIdx;

    auto &trail = SmokeTrails.array[trailIdx].data;

    if (!trail.Points || !trail.Points->array) {
      RemoveSmokeTrail(trailIdx);
      continue;
    }

    int numPoints = trail.Points->size;
    if (numPoints < 2)
      Logger.g->Panic("SGepard::RenderSmokeTrails: Structure is damaged");

    lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    DrawType = trail.DrawType;
    if (trail.DrawType == DT_ADD)
      lpD3DDev->SetRenderState(D3DRS_FOGENABLE, FALSE);
    else
      EnableFog();
    SetTexture(0, trail.TextureIndex, 1);

    int lastAlpha = 0;
    for (int i = 0; i < numPoints; ++i) {
      SSmokeTrailPoint *pt = &trail.Points->array[i];
      float elapsed = (float)(WorldTime - pt->StartTime);
      float decay = (trail.Strength + 20.0f) / (elapsed * trail.FadeSpeed + 1.0f) - 20.0f;
      int alpha = (int)((float)(int)decay * pt->Alpha);
      if (alpha > 255) alpha = 255;
      if (alpha < 0) alpha = 0;

      // Compute scale: grows with elapsed time to create tapered dart shape
      float scale;
      if (trail.UScale >= 0.0f) {
        unsigned int elapsed = WorldTime - pt->StartTime;
        scale = sqrtf((float)elapsed) * trail.UScale;
      } else {
        scale = -trail.UScale;
      }
      pt->Scale = scale;

      // Compute direction vector for billboard
      float dirX, dirY, dirZ;
      if (i == 0) {
        SSmokeTrailPoint *next = &trail.Points->array[1];
        dirX = next->Vector.x - pt->Vector.x;
        dirY = next->Vector.y - pt->Vector.y;
        dirZ = next->Vector.z - pt->Vector.z;
      } else {
        SSmokeTrailPoint *prev = &trail.Points->array[i - 1];
        if (i == numPoints - 1) {
          dirX = pt->Vector.x;
          dirY = pt->Vector.y;
          dirZ = pt->Vector.z;
        } else {
          SSmokeTrailPoint *next = &trail.Points->array[i + 1];
          dirX = next->Vector.x;
          dirY = next->Vector.y;
          dirZ = next->Vector.z;
        }
        dirX -= prev->Vector.x;
        dirY -= prev->Vector.y;
        dirZ -= prev->Vector.z;
      }

      // Cross product for billboard normal, then normalize via D3DXVec3Normalize
      D3DXVECTOR3 cross;
      cross.x = (pt->Vector.z - camZ) * dirY - (pt->Vector.y - camY) * dirZ;
      cross.y = (pt->Vector.x - camX) * dirZ - (pt->Vector.z - camZ) * dirX;
      cross.z = (pt->Vector.y - camY) * dirX - (pt->Vector.x - camX) * dirY;
      D3DXVec3Normalize(&cross, &cross);
      cross.x *= scale;
      cross.y *= scale;
      cross.z *= scale;

      // Build 3 vertices for current cross-section: left (u=0), center (u=0.5), right (u=1)
      float texV = pt->TextureV;
      unsigned int color;
      if (trail.DrawType == DT_NORMAL)
        color = trail.Color + alpha * 0x1000000;
      else if (trail.DrawType == DT_ADD)
        color = alpha * 0x10101;
      else
        color = trail.Color + alpha * 0x1000000;

      // Current right (vertex 2)
      fanVerts[2].x = pt->Vector.x + cross.x;
      fanVerts[2].y = pt->Vector.y + cross.y;
      fanVerts[2].z = pt->Vector.z + cross.z;
      fanVerts[2].color = color;
      fanVerts[2].u = 1.0f;
      fanVerts[2].v = texV;

      // Current center (vertex 3)
      fanVerts[3].x = pt->Vector.x;
      fanVerts[3].y = pt->Vector.y;
      fanVerts[3].z = pt->Vector.z;
      fanVerts[3].color = color;
      fanVerts[3].u = 0.5f;
      fanVerts[3].v = texV;

      // Current left (vertex 4)
      fanVerts[4].x = pt->Vector.x - cross.x;
      fanVerts[4].y = pt->Vector.y - cross.y;
      fanVerts[4].z = pt->Vector.z - cross.z;
      fanVerts[4].color = color;
      fanVerts[4].u = 0.0f;
      fanVerts[4].v = texV;

      // Draw TriangleFan connecting prev and current cross-sections
      if (i > 0 && alpha != 0)
        lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 4, fanVerts, 24);

      // Carry forward: copy current vertices into prev slots for next iteration
      fanVerts[1] = fanVerts[2]; // prev right
      fanVerts[0] = fanVerts[3]; // prev center (fan hub)
      fanVerts[5] = fanVerts[4]; // prev left

      lastAlpha = alpha;
    }

    // AutoDestruct: remove trail when last point's alpha reaches 0
    if (lastAlpha == 0 && trail.AutoDestruct)
      RemoveSmokeTrail(trailIdx);
  }
done:
  EnableFog();
  lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
}

// --- Other complex stubs ---

void SGepard::HashObjects()
{
  memset(ObjectHash, -1, 4 * (XSize / 8) * (ZSize / 8));

  // Clear ObjectHashChain
  ObjectHashChain.size = 0;
  if (ObjectHashChain.maxsize > 0)
    memset(ObjectHashChain.array, 0, sizeof(SObjectHashChain) * ObjectHashChain.maxsize);

  int idx = -1;
  while (true) {
    int size = StaticObjects.size;
    if (++idx >= size) break;
    while (StaticObjects.array[idx].use != 0x7FFFFFFF) {
      if (++idx >= size) goto done;
    }
    if (idx < 0) break;

    if (!StaticObjects.array[idx].data.Group->Visible)
      continue;

    SGroup *group = StaticObjects.array[idx].data.Group;
    group->Precalculate();
    float xmin, xmax, zmin, zmax;
    group->ComputeVisBounding(&xmin, &xmax, &zmin, &zmax);

    if (xmin < 0.0f) xmin = 0.0f;
    if (xmax > (float)XSize) xmax = (float)XSize;
    if (zmin < 0.0f) zmin = 0.0f;
    if (zmax > (float)ZSize) zmax = (float)ZSize;

    int zCell = (int)(zmin * 0.125f);
    int zWorld = 8 * zCell;
    if (zmax > (float)zWorld) {
      do {
        int xCell = (int)(xmin * 0.125f);
        int xWorld = 8 * xCell;
        if (xmax > (float)xWorld) {
          do {
            // Grow ObjectHashChain if needed
            if (ObjectHashChain.size == ObjectHashChain.maxsize) {
              int newmax = (ObjectHashChain.maxsize >= 16) ? 6 * ObjectHashChain.maxsize / 5 : 16;
              auto *arr = (SObjectHashChain*)realloc(ObjectHashChain.array, sizeof(SObjectHashChain) * newmax);
              int oldmax = ObjectHashChain.maxsize;
              ObjectHashChain.array = arr;
              memset(&arr[oldmax], 0, sizeof(SObjectHashChain) * (newmax - oldmax));
              ObjectHashChain.maxsize = newmax;
            }

            int hashIdx = xCell + zCell * (XSize / 8);
            int chainIdx = ObjectHashChain.size;
            ObjectHashChain.size = chainIdx + 1;
            ObjectHashChain.array[chainIdx].ObjectIdx = idx;
            ObjectHashChain.array[chainIdx].Next = ObjectHash[hashIdx];
            ObjectHash[hashIdx] = chainIdx;

            ++xCell;
            xWorld += 8;
          } while (xmax > (float)xWorld);
        }
        ++zCell;
        zWorld += 8;
      } while (zmax > (float)zWorld);
    }
  }
done:
  ObjectsHashed = true;
}

void SGepard::RegenerateObjectShadowDecals()
{
  int updateCount = 0;
  int cellIdx = 0;

  int totalCells = (XSize / 8) * (ZSize / 8);

  while (cellIdx < totalCells && updateCount < 4) {
    if (Terrain->Parcels[cellIdx].Visible) {
      int next = ObjectHash[cellIdx];
      while (next >= 0 && updateCount < 4) {
        int chainIdx = next;
        int objIdx = ObjectHashChain.array[chainIdx].ObjectIdx;
        if (StaticObjects.array[objIdx].data.LastRendered != RenderCounter) {
          StaticObjects.array[objIdx].data.LastRendered = RenderCounter;
          SGroup *group = StaticObjects.array[objIdx].data.Group;
          if (group->Visible) {
            if ((group->Flags & 1) && group->NeedNewShadow) {
              MakeObjectShadowMask(StaticObjects.array[objIdx].data.Group);
              if (StaticObjects.array[objIdx].data.Group->ShadowMask2)
                MakeObjectShadowShader(StaticObjects.array[objIdx].data.Group);
              StaticObjects.array[objIdx].data.Group->NeedNewShadow = 0;
              ++updateCount;
            } else if (group->ShadowMask2 && !group->ShadowMesh) {
              MakeObjectShadowShader(StaticObjects.array[objIdx].data.Group);
            }
          }
        }
        next = ObjectHashChain.array[chainIdx].Next;
      }
    }
    ++cellIdx;

  }

  ++RenderCounter;

  // Dynamic objects
  int idx = -1;
  while (true) {
    int size = DynamicObjects.size;
    if (++idx >= size) break;
    while (DynamicObjects.array[idx].use != 0x7FFFFFFF) {
      if (++idx >= size) return;
    }
    if (idx < 0) break;

    SGroup *group = DynamicObjects.array[idx].data.Group;

    float x, y, z;
    group->GetPosition(&x, &y, &z);
    if (!group->Visible) { group->ClearShadow(0); continue; }

    if (x < 0.0f || (float)Terrain->XSize <= x || z < 0.0f || (float)Terrain->ZSize <= z) {
      group->ClearShadow(0);
      continue;
    }

    if (!Terrain->Parcels[-(int)(x * -0.125f) - Terrain->XParcels * (int)(z * -0.125f)].Visible) {
      group->ClearShadow(0);
      continue;
    }

    bool visible = true;
    if (Terrain->VisMap && !Terrain->GodMode) {
      int vx = (int)(x * 2.0f);
      int vz = (int)(z * -2.0f);
      int visIdx = vx - 2 * (Terrain->XSize + 1) * vz;
      if (group->VisClass == 1) {
        if (!Terrain->VisMap[visIdx]) visible = false;
      } else if (group->VisClass == 2) {
        if (!Terrain->VisMap2[visIdx]) visible = false;
      }
    }

    if (visible)
      GenerateObjectShadow(group, true);
    else
      group->ClearShadow(0);
  }
}

void SGepard::InitPixelShader(const char *path, SmartPtr<IDirect3DPixelShader9> *shader, D3DXMACRO *defines)
{
  ID3DXBuffer *shaderBuffer = nullptr;
  ID3DXBuffer *errors = nullptr;
  if (D3DXCompileShaderFromFileA(path, defines, 0, "PSMain", "ps_2_0", 0, &shaderBuffer, &errors, 0)) {
    if (errors) {
      const char *msg = (const char*)errors->GetBufferPointer();
      Logger.g->Warning("Shader error: %s", msg);
    }
  } else {
    if (shader->Ptr)
      shader->Ptr->Release();
    shader->Ptr = 0;
    DWORD *code = (DWORD*)shaderBuffer->GetBufferPointer();
    HRESULT hr = lpD3DDev->CreatePixelShader(code, &shader->Ptr);
    if (hr) {
      const char *errstr = DXGetErrorStringA(hr);
      char atmstr[200];
      sprintf(atmstr, "%s: %s", "CreatePixelShader failed", errstr);
      Logger.g->Panic(atmstr);
    }
  }
  if (shaderBuffer) shaderBuffer->Release();
  if (errors) errors->Release();
}

void SGepard::InitVertexShader(const char *path, SmartPtr<IDirect3DVertexShader9> *shader, D3DXMACRO *defines)
{
  ID3DXBuffer *shaderBuffer = nullptr;
  ID3DXBuffer *errors = nullptr;
  if (D3DXCompileShaderFromFileA(path, defines, 0, "VSMain", "vs_2_0", 0, &shaderBuffer, &errors, 0)) {
    if (errors) {
      const char *msg = (const char*)errors->GetBufferPointer();
      Logger.g->Warning("Shader error: %s", msg);
    }
  } else {
    if (shader->Ptr)
      shader->Ptr->Release();
    shader->Ptr = 0;
    DWORD *code = (DWORD*)shaderBuffer->GetBufferPointer();
    HRESULT hr = lpD3DDev->CreateVertexShader(code, &shader->Ptr);
    if (hr) {
      const char *errstr = DXGetErrorStringA(hr);
      char atmstr[200];
      sprintf(atmstr, "%s: %s", "CreateVertexShader failed", errstr);
      Logger.g->Panic(atmstr);
    }
  }
  if (shaderBuffer) shaderBuffer->Release();
  if (errors) errors->Release();
}

void SGepard::MakeScreenshot(unsigned int width, unsigned int height, char *pathandname, unsigned int sernumber)
{
  char atmstr[200];
  HRESULT hr;

  // Create off-screen render target
  IDirect3DSurface9 *rt = nullptr, *dss = nullptr, *oldRT = nullptr, *oldDSS = nullptr;
  hr = lpD3DDev->CreateRenderTarget(width, height, D3DFMT_A8R8G8B8, D3DMULTISAMPLE_NONE, 0, TRUE, &rt, nullptr);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeScreenshot: CreateRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->CreateDepthStencilSurface(width, height, D3DFMT_D24S8, D3DMULTISAMPLE_NONE, 0, TRUE, &dss, nullptr);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeScreenshot: CreateDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->GetRenderTarget(0, &oldRT);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeScreenshot: GetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  if (!ResolutionScaleSurface.Ptr) {
    hr = lpD3DDev->GetDepthStencilSurface(&oldDSS);
    if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeScreenshot: GetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  }
  hr = lpD3DDev->SetRenderTarget(0, rt);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeScreenshot: SetRenderTarget failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }
  hr = lpD3DDev->SetDepthStencilSurface(dss);
  if (FAILED(hr)) { sprintf(atmstr, "%s: %s", "SGepard::MakeScreenshot: SetDepthStencilSurface failed", DXGetErrorStringA(hr)); Logger.g->Panic(atmstr); }

  // Hide board, render scene
  bool savedBoardVis = BoardVisible;
  BoardVisible = false;
  RenderScene(false);
  BoardVisible = savedBoardVis;

  // Restore render targets
  hr = lpD3DDev->SetRenderTarget(0, oldRT);
  hr = lpD3DDev->SetDepthStencilSurface(oldDSS);

  // Save to TGA
  SSurfaceBitmap surfBmp(rt);
  SBitmap bmap(&surfBmp, D3DFMT_A8R8G8B8);
  surfBmp.~SSurfaceBitmap();

  // Force alpha = 0xFF so dust/particle blends and non-recolorable decal
  // pixels (logos etc.) don't leave transparent holes in the saved TGA.
  // Format is A8R8G8B8 (BGRA in memory little-endian); alpha byte is at +3.
  if (bmap.Data && bmap.Format == D3DFMT_A8R8G8B8 && bmap.Pixel == 4)
  {
    unsigned char *row = bmap.Data + bmap.Start;
    for (int y = 0; y < bmap.Height; ++y)
    {
      for (int x = 0; x < bmap.Width; ++x)
        row[x * 4 + 3] = 0xFF;
      row += bmap.Pitch;
    }
  }

  char filename[260];
  sprintf(filename, "%s%04d.tga", pathandname, sernumber);
  bmap.SaveTGA(filename, "SGepard::MakeScreenshot");

  if (rt) rt->Release();
  if (dss) dss->Release();
  if (oldRT) oldRT->Release();
  if (oldDSS) oldDSS->Release();
}

void SGepard::RefreshRect(SMesh *mesh, int x0, int z0, int x1, int z1)
{
  int vi = 0;

  // Bottom edge: left to right (z=z0)
  for (int x = x0; x < x1; ++x) {
    float *pos = (float*)((char*)mesh->lpVertices + mesh->OffsetXYZ + vi * mesh->VertexSize);
    pos[0] = (float)x;
    pos[2] = (float)z0;
    pos[1] = (float)(Terrain->GetHeight(x, z0) + 0.01f);

    ++vi;
  }

  // Right edge: bottom to top (x=x1)
  for (int z = z0; z < z1; ++z) {
    float *pos = (float*)((char*)mesh->lpVertices + mesh->OffsetXYZ + vi * mesh->VertexSize);
    pos[0] = (float)x1;
    pos[2] = (float)z;
    pos[1] = (float)(Terrain->GetHeight(x1, z) + 0.01f);

    ++vi;
  }

  // Top edge: right to left (z=z1)
  for (int x = x1; x > x0; --x) {
    float *pos = (float*)((char*)mesh->lpVertices + mesh->OffsetXYZ + vi * mesh->VertexSize);
    pos[0] = (float)x;
    pos[2] = (float)z1;
    pos[1] = (float)(Terrain->GetHeight(x, z1) + 0.01f);

    ++vi;
  }

  // Left edge: top to bottom (x=x0)
  for (int z = z1; z >= z0; --z) {
    float *pos = (float*)((char*)mesh->lpVertices + mesh->OffsetXYZ + vi * mesh->VertexSize);
    pos[0] = (float)x0;
    pos[2] = (float)z;
    pos[1] = (float)(Terrain->GetHeight(x0, z) + 0.01f);

    ++vi;
  }
}

//----- (0043AE80) --------------------------------------------------------
void SGepard::RefreshSubNodes(SHillRing *_hillring)
{
  SChain<SSplineRing> *chain = _hillring->SplineChain;
  if (_hillring->Type == 1) {
    if (chain->Closed && !chain->first)
      Logger.g->Panic("GetFirst: In a closed chain <first> is NULL");
    chain->first->spline->RefreshSubNodes();
    SChain<SSplineRing> *c2 = _hillring->SplineChain;
    if (c2->Closed && !c2->last)
      Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
    c2->last->spline->RefreshSubNodes();
  } else {
    if (chain->Closed && !chain->last)
      Logger.g->Panic("GetLast: In a closed chain <last> is NULL");
    chain->last->spline->RefreshSubNodes(); // RefreshSubNodes2 is idb, using RefreshSubNodes
  }
}

void SGepard::ReloadTexture(int idx)
{
  // Save filename and refcount before removing
  char *savedName = _strdup(Textures.array[idx].data.FileName);
  int savedRefCount = Textures.array[idx].data.RefCount;

  // Free old texture data
  if (Textures.array[idx].data.FileName) {
    ::operator delete(Textures.array[idx].data.FileName);
    Textures.array[idx].data.FileName = nullptr;
  }
  if (Textures.array[idx].data.lpTexture) {
    Textures.array[idx].data.lpTexture->Release();
    Textures.array[idx].data.lpTexture = nullptr;
  }

  // Remove from heap
  Textures.array[idx].use = Textures.nextempty;
  --Textures.occupied;
  Textures.nextempty = idx;

  // Try DXT first, then TGA
  char dxt_filename[260];
  strcpy(dxt_filename, savedName);
  strcpy(strrchr(dxt_filename, '.'), ".dxt");

  int newIdx;
  struct _stat stat_orig, stat_dxt;
  if (FileSystem.Stat(savedName, &stat_orig)) {
    if (!FileSystem.Stat(dxt_filename, &stat_dxt))
      newIdx = CreateTextureFromDXT(dxt_filename, savedName, true);
    else
      Logger.g->Panic("SGepard::ReloadTexture: Reload failed");
  } else if (!FileSystem.Stat(dxt_filename, &stat_dxt)
    && abs(stat_orig.st_mtime - stat_dxt.st_mtime) <= 2) {
    newIdx = CreateTextureFromDXT(dxt_filename, savedName, true);
  } else {
    Logger.g->Log(0, "NO COMPRESSED TEXTURE FOR: %s", savedName);
    SBitmap bmap;
    if (!bmap.LoadTGA(savedName, 0))
      Logger.g->Panic("SGepard::ReloadTexture: Reload failed");
    newIdx = CreateTextureFromBitmap(savedName, &bmap, true);
  }

  if (idx != newIdx)
    Logger.g->Panic("SGepard::ReloadTexture: Index mismatch");

  Textures.array[idx].data.RefCount = savedRefCount;
  ::operator delete(savedName);
}

void SGepard::LoadDXTToLevelBitmaps(const char *dxt_filename, SDArray<SBitmap *> *bitmaps)
{
  SStream *is = FileSystem.OpenRead(dxt_filename, "SGepard::LoadDXTToLevelBitmaps");
  is->ReadSignature();
  if (is->ReadChunkHeader() != 0x54584554) { // 'TEXT'
    throw "Not a texture file";
  }

  is->ReadInt(); // skip
  int width = is->ReadInt();
  int height = is->ReadInt();
  D3DFORMAT format = (D3DFORMAT)is->ReadInt();

  int numLevels = (int)ceil(log((double)width) / log(2.0)) + 1;

  for (int level = 0; level < numLevels; ++level) {
    SBitmap *bmp = new SBitmap(width, height, format, 0);
    int dataSize = bmp->GetLogicalSize();
    is->Read(bmp->Data, dataSize);

    // Add to array
    if (bitmaps->size == bitmaps->maxsize) {
      int newmax = (bitmaps->maxsize >= 16) ? 6 * bitmaps->maxsize / 5 : 16;
      auto *arr = (SBitmap**)realloc(bitmaps->array, sizeof(SBitmap*) * newmax);
      int oldmax = bitmaps->maxsize;
      bitmaps->array = arr;
      memset(&arr[oldmax], 0, sizeof(SBitmap*) * (newmax - oldmax));
      bitmaps->maxsize = newmax;
    }
    bitmaps->array[bitmaps->size++] = bmp;

    width /= 2;
    height /= 2;
  }

  is->ReadChunkValidate(width);
  is->Release();
}

void* SGepard::PlayEffect(int handle, float _x, float _y, float _z, float _scalespeed)
{
  if (Effect) {
    if ((uintptr_t)Effect >= 0x80000000u || ((uintptr_t)Effect & 0xFFFF0000u) == 0xBAAD0000u) {
      Logger.g->Warning("SGepard::PlayEffect: corrupt Effect=%p (this SGepard=%p, handle=%d)", Effect, this, handle);
      return nullptr;
    }
    return Effect->PlayEffect(handle, _x, _y, _z, _scalespeed);
  }
  return nullptr;
}

float SGepard::GetClickDistanceSquare(D3DXVECTOR3 *vec, float click_x, float click_y)
{
  D3DXVECTOR3 vec2;
  D3DXVec3TransformCoord(&vec2, vec, &CameraMatrix);
  if (vec2.z <= 0.0f)
    return 3.4028235e38f;
  float dx = (ProjectionMatrix._11 * vec2.x) - (vec2.z * click_x);
  float dy = (ProjectionMatrix._22 * vec2.y) - (vec2.z * click_y);
  return dx * dx + dy * dy;
}

S3Vertex* SGepard::GetNext3Vertex(S3Vertex *result, SChain<SNodeInfo2> *_nodechain, bool plus3)
{
  SNodeInfo2 *cur = _nodechain->current;
  if (!cur) {
    memset(result, 0, sizeof(S3Vertex));
    return result;
  }

  // Get current node position
  result->v1.x = cur->x;
  result->v1.y = cur->y;
  result->v1.z = cur->z;

  // Get next node
  SNodeInfo2 *next1 = cur->next;
  if (!next1) {
    if (_nodechain->Closed)
      next1 = _nodechain->first;
  }
  if (next1) {
    result->v2.x = next1->x;
    result->v2.y = next1->y;
    result->v2.z = next1->z;
  } else {
    result->v2 = result->v1;
  }

  // Get next-next node
  SNodeInfo2 *next2 = next1 ? next1->next : nullptr;
  if (!next2 && _nodechain->Closed && next1)
    next2 = _nodechain->first;
  if (next2) {
    result->v3.x = next2->x;
    result->v3.y = next2->y;
    result->v3.z = next2->z;
  } else {
    result->v3 = result->v2;
  }

  // Advance current
  if (plus3) {
    // Advance 3 steps
    SNodeInfo2 *n = cur;
    for (int i = 0; i < 3 && n; ++i) {
      n = n->next;
      if (!n && _nodechain->Closed) n = _nodechain->first;
    }
    _nodechain->current = n;
  } else {
    SNodeInfo2 *n = cur->next;
    if (!n && _nodechain->Closed) n = _nodechain->first;
    _nodechain->current = n;
  }

  return result;
}

unsigned char* SGepard::LockDynamicVB(int a2, int a3, int a4, int idx, unsigned int needed_vertices)
{
  SHeap<SDynamicVB>::__Tstruct *array;
  if ( idx < 0 || idx >= this->DynamicVBs.size || (array = this->DynamicVBs.array, array[idx].use != 0x7FFFFFFF) )
    Logger.g->Panic("SGepard::LockDynamicVB: Bad index (%d).", idx);
  if ( array[idx].data.Locked )
    Logger.g->Panic("SGepard::LockDynamicVB: Already locked.");
  unsigned int NumVertices = array[idx].data.NumVertices;
  if ( needed_vertices > NumVertices )
    Logger.g->Panic("SGepard::LockDynamicVB: VertexBuffer is too small.");
  DWORD lockFlags = D3DLOCK_NOOVERWRITE;
  if ( needed_vertices + array[idx].data.LockPosition > NumVertices )
  {
    array[idx].data.LockPosition = 0;
    array = this->DynamicVBs.array;
    lockFlags = D3DLOCK_DISCARD;
  }
  unsigned char *lpVertices = nullptr;
  HRESULT hr = array[idx].data.lpVertexBuffer->Lock(
    array[idx].data.LockPosition * array[idx].data.VertexSize,
    needed_vertices * array[idx].data.VertexSize,
    (void **)&lpVertices,
    lockFlags);
  if ( hr )
  {
    const char *v10 = DXGetErrorStringA(hr);
    char atmstr[200];
    sprintf(atmstr, "%s: %s", "SGepard::LockDynamicVB: IDirect3DVertexBuffer9::Lock", v10);
    Logger.g->Panic(atmstr);
  }
  this->DynamicVBs.array[idx].data.Locked = 1;
  return lpVertices;
}

//----- (004340D0) --------------------------------------------------------
void SGepard::GetViewBoundaries(D3DXVECTOR2 *bound)
{
  float h = (float)(Terrain->GetHeight(CameraXPos, CameraZPos) - CameraYPos);

  D3DXMATRIX mx;
  D3DXMatrixInverse(&mx, nullptr, &CameraMatrix);
  D3DXVECTOR3 p[4];
  float inv11 = 1.0f / ProjectionMatrix._11;
  float inv22 = 1.0f / ProjectionMatrix._22;
  p[0] = D3DXVECTOR3(-inv11,  inv22, 1.0f);
  p[1] = D3DXVECTOR3( inv11,  inv22, 1.0f);
  p[2] = D3DXVECTOR3( inv11, -inv22, 1.0f);
  p[3] = D3DXVECTOR3(-inv11, -inv22, 1.0f);
  for (int i = 0; i < 4; ++i) {
    D3DXVec3TransformNormal(&p[i], &p[i], &mx);
    bound[i].x = ((p[i].x / p[i].y * h + CameraXPos) - 48.0f) / (float)(XSize - 96);
    bound[i].y = ((p[i].z / p[i].y * h + CameraZPos) - 48.0f) / (float)(ZSize - 96);
  }
}

//----- (0043A8C0) --------------------------------------------------------
void SGepard::RefreshLights()
{
  for (int i = 0; i < 4; ++i) {
    if (LightProps[i].type) {
      lpD3DDev->SetLight(i, &LightProps[i].d3dlight);
      lpD3DDev->LightEnable(i, TRUE);
    }
  }
}

//----- (00439C90) --------------------------------------------------------
int SGepard::MemChk(void *memptr)
{
  if (!memptr) {
    Logger.g->Log(0, "SGepard::MemChk: Out of memory, waiting for user response...");
    if (FullScreen)
      Logger.g->Panic("SGepard::MemChk: We are in fullscreen, exiting...");
    int result = ::MessageBoxA(hWnd,
      "Out of memory! Select Retry to try again, or select Cancel to Quit the program.",
      nullptr, MB_RETRYCANCEL);
    if (!result)
      Logger.g->Panic("An out of memory occurd when tryed to created the out of memory dialog box (?!)");
    if (result == IDCANCEL) {
      Logger.g->Log(0, "SGepard::MemChk: Cancel was choosen, exiting...");
      _exit(1);
    }
    Logger.g->Log(0, "SGepard::MemChk: Retry was choosen, retry to alloc the memory...");
  }
  return 0;
}

//----- (0043A550) --------------------------------------------------------
void SGepard::ProjectPoint(float u, float v, float xobj, float yobj, float zobj, float *x, float *z)
{
  float vx = SunDir.x / SunDir.y;
  float vz = SunDir.z / SunDir.y;
  *x = (SunProjection._11 * u + SunProjection._12 * v) + xobj;
  float zi = (SunProjection._12 * u - SunProjection._11 * v) + zobj;
  for (int iter = 0; ; zi = *z - (yobj - (float)Terrain->GetOldHeight(*x, *z)) * vz) {
    *z = zi;
    float ground = (float)(Terrain->GetOldHeight(*x, zi));

    if (ground + 0.0001f > yobj) {
      if (yobj > ground - 0.0001f)
        break;
      if (++iter > 2)
        break;
    }
    yobj = ground;
    *x = *x - (yobj - ground) * vx;
  }
}

//----- (004361E0) --------------------------------------------------------
void SGepard::LerpFolyo(SVertDiff *folyo, int folyoidx, SVert *vert, int vertidx1, int vertidx2, float ratio)
{
  folyo[folyoidx].x = (vert[vertidx2].x - vert[vertidx1].x) * ratio + vert[vertidx1].x;
  folyo[folyoidx].y = (vert[vertidx2].y - vert[vertidx1].y) * ratio + vert[vertidx1].y;
  folyo[folyoidx].z = (vert[vertidx2].z - vert[vertidx1].z) * ratio + vert[vertidx1].z;
  folyo[folyoidx].u = (vert[vertidx2].u - vert[vertidx1].u) * ratio + vert[vertidx1].u;
  folyo[folyoidx].v = (vert[vertidx2].v - vert[vertidx1].v) * ratio + vert[vertidx1].v;
}

//----- (00434840) --------------------------------------------------------
void SGepard::InitResolutionScale(int a2, int a3)
{
  if (ResolutionScale == 1.0f) {
    ViewWidthScaled = ViewWidth;
    ViewHeightScaled = ViewHeight;
  } else {
    int scaledW = (int)((float)ViewWidth * ResolutionScale);
    int scaledH = (int)((float)ViewHeight * ResolutionScale);
    D3DMULTISAMPLE_TYPE msaa = MSAALevel;
    if (msaa < D3DMULTISAMPLE_2_SAMPLES ||
        FAILED(lpD3D->CheckDeviceMultiSampleType(Adapter, DeviceType, AdapterFormat,
               PresentationParameters.Windowed, msaa, nullptr))) {
      msaa = D3DMULTISAMPLE_NONE;
    }
    if (ResolutionScaleSurface.Ptr) {
      ResolutionScaleSurface.Ptr->Release();
      ResolutionScaleSurface.Ptr = nullptr;
    }
    HRESULT hr = lpD3DDev->CreateRenderTarget(scaledW, scaledH, D3DFMT_X8R8G8B8,
                                               msaa, 0, FALSE, &ResolutionScaleSurface.Ptr, nullptr);
    if (FAILED(hr)) {
      char atmstr[200];
      sprintf(atmstr, "%s: %s", "SGepard::InitResolutionScale: CreateRenderTarget failed", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    if (ResolutionScaleDepthSurface.Ptr) {
      ResolutionScaleDepthSurface.Ptr->Release();
      ResolutionScaleDepthSurface.Ptr = nullptr;
    }
    hr = lpD3DDev->CreateDepthStencilSurface(scaledW, scaledH, D3DFMT_D24S8,
                                              msaa, 0, TRUE, &ResolutionScaleDepthSurface.Ptr, nullptr);
    if (FAILED(hr)) {
      char atmstr[200];
      sprintf(atmstr, "%s: %s", "SGepard::InitResolutionScale: CreateDepthStencilSurface failed", DXGetErrorStringA(hr));
      Logger.g->Panic(atmstr);
    }
    ViewWidthScaled = scaledW;
    ViewHeightScaled = scaledH;
  }
}

// ---------------------------------------------------------------------------
// CreateGepard — factory function
// ---------------------------------------------------------------------------
int __cdecl CreateGepard(HWND hwnd, bool fullscreen, bool vsync, int msaa,
                         int width, int height, int a7, SIGepard **ppGepard)
{
    SGepard *g = new SGepard();
    if (!g)
        Logger.g->Panic("::CreateGepard: Out of memory!");
    int result = g->Initialize(hwnd, fullscreen, vsync, (D3DMULTISAMPLE_TYPE)msaa, width, height, a7);
    if (result)
    {
        g->Release();
        return result;
    }
    *ppGepard = g;
    return 0;
}

int __cdecl CreateGepard(HWND hwnd, int a2, int a3, D3DMULTISAMPLE_TYPE a4,
                         int a5, int a6, int a7, SIGepard **ppGepard)
{
    return CreateGepard(hwnd, (bool)a2, (bool)a3, (int)a4, a5, a6, a7, ppGepard);
}
