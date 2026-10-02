// 3dengine/terrain.cpp
// Terrain rendering and collision
// Decompiled from: gameSplit/sterrain.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "terrain.h"
#include "parcel.h"
#include "gepard.h"
#include "logger.h"
#include "core_common.h"
#include <new>
#include <xmmintrin.h>
#include <emmintrin.h>
#include "properties.h"
#include "bitmap.h"
#include "hdbeefup.h"
#ifdef HD_HDBEEFUP_SETTINGS
#include "options.h"
extern SOptions *Options;
#endif

#define LT_AMBIENT 1
#define LT_PRELIT 2

static const double DOUBLE_0_25 = 0.25;
static const double DOUBLE_0_375 = 0.375;
static const float FLOAT_4_5 = 4.5f;
static const float FLOAT_N3_5 = -3.5f;
static const float FLOAT_0_33333334 = 0.33333334f;
static const float FLOAT_100_0 = 100.0f;
static char _fullpath_buf[4] = {0};
static const char *fullpath = _fullpath_buf;

// Classes: STerrain
// Function count: 39

//----- (004360B0) --------------------------------------------------------

char STerrain::IsVisible(float x, float z, int visclass)

{
  unsigned char *VisMap; // edx
  bool v5;
  VisMap = this->VisMap;
  if ( VisMap && !this->GodMode && visclass )
  {
    if ( visclass == 1 )
    {
      v5 = VisMap[(int)(float)(x + x) - 2 * (this->XSize + 1) * (int)(float)(z * -2.0)] == 0;
    }
    else
    {
      if ( visclass != 2 )
        return 0;
      v5 = this->VisMap2[(int)(float)(x + x) - 2 * (this->XSize + 1) * (int)(float)(z * -2.0)] == 0;
    }
    if ( v5 )
      return 0;
  }
  return 1;
}

//----- (00450A10) --------------------------------------------------------

// SParcel2 vector deleting destructor removed (IDA artifact)

//----- (00450A60) --------------------------------------------------------

// SParcel *SParcel::`vector deleting destructor'(SParcel *this, char a2) -- removed (IDA artifact)

//----- (00457330) --------------------------------------------------------

bool STerrain::IsParcelVisible(float x, float z)

{
  return x >= 0.0
      && (float)this->XSize > x
      && z >= 0.0
      && (float)this->ZSize > z
      && this->Parcels[-(int)(x * -0.125) - this->XParcels * (int)(z * -0.125)].Visible;
}

//----- (0045C370) --------------------------------------------------------

STerrain::STerrain(SGepard *gepard, int xsize, int zsize)

{
  int v5;
  float *v6; // eax
  float *v7; // eax
  unsigned char *v8; // eax
  int NumVertices;
  unsigned int *v10; // eax
  int v11;
  int v12;
  unsigned char *v13; // eax
  unsigned char *v14; // eax
  int v15;
  int v16;
  int v17;
  int NumParcels;
  int v19;
  int v20;
  SParcel *v21; // ecx
  SParcel *v23; // ecx
  STile *v24; // eax
  size_t v25;
  int i;
  size_t v27;
  size_t v28;
  size_t v29;
  size_t b;
  SGepard *geparda;
  this->TilePrefix.buf = 0;
  this->TilePrefix.size = 0;
  memset(&this->WireframeMaterial, 0, sizeof(SMaterial));
  memset(&this->Wireframe2Material, 0, sizeof(SMaterial));
  this->Gepard = gepard;
  this->lpD3DDev = gepard->lpD3DDev;
  this->XSize = xsize;
  this->ZSize = zsize;
  v5 = (xsize + 1) * (zsize + 1);
  this->Stride = xsize + 1;
  this->NumTiles = zsize * xsize;
  this->NumVertices = v5;
  v6 = (float *)operator new[](4 * v5);
  b = 4 * this->NumVertices;
  this->HeightMap = v6;
  memset(v6, 0, b);
  v7 = (float *)operator new[](4 * this->NumVertices);
  v28 = 4 * this->NumVertices;
  this->OldHeightMap = v7;
  memset(v7, 0, v28);
  v8 = new unsigned char[9 * this->NumVertices];
  NumVertices = this->NumVertices;
  this->BlendMap = (unsigned char (*)[9])v8;
  memset(v8, 0, 9 * NumVertices);
  v10 = (unsigned int *)operator new[](4 * this->NumVertices);
  v11 = this->NumVertices;
  this->ColorMap = v10;
  memset(v10, 0, 4 * v11);
  geparda = (SGepard *)operator new[](12 * this->NumVertices);
  v12 = this->NumVertices;
  this->NormalMap = (D3DXVECTOR3 *)geparda;
  memset(geparda, 0, 12 * v12);
  v13 = new unsigned char[this->NumVertices];
  v29 = this->NumVertices;
  this->ShadowMap = v13;
  memset(v13, 128, v29);
  v14 = new unsigned char[this->NumVertices];
  v27 = this->NumVertices;
  this->WaterMap = v14;
  memset(v14, 0, v27);
  v15 = this->XSize;
  if ( (v15 & 7) != 0 || (v16 = this->ZSize, (v16 & 7) != 0) )
 Logger.g->Panic(
      "STerrain::STerrain: Plane should be multiple of 8x8 units (%dx%d)",
      v15,
      this->ZSize);
  this->XParcels = v15 / 8;
  this->ZParcels = v16 / 8;
  v17 = v15 / 8 * (v16 / 8);
  this->NumParcels = v17;
  this->Parcels = (SParcelInfo *)operator new[](sizeof(SParcelInfo) * v17);
  NumParcels = this->NumParcels;
  v19 = 0;
  if ( NumParcels > 0 )
  {
    v20 = 0;
    do
    {
      v21 = (SParcel *)operator new(sizeof(SParcel));
      if ( v21 )
      {
        new (v21) SParcel(this->Gepard, 8 * (v19 % this->XParcels), 8 * (v19 / this->XParcels));
        v23 = v21;
      }
      else
      {
        v23 = 0;
      }
      ++v19;
      this->Parcels[v20].Mesh = v23;
      this->Parcels[v20].Mesh2 = 0;
      this->Parcels[v20].BlockMapMesh = 0;
      this->Parcels[v20].Visible = 1;
      this->Parcels[v20++].LowPoint = 0.0;
      NumParcels = this->NumParcels;
    }
    while ( v19 < NumParcels );
  }
  v24 = (STile *)operator new[](2 * NumParcels);
  v25 = 2 * this->NumParcels;
  this->TileMap = v24;
  memset(v24, 0, v25);
  for ( i = 0; i < this->NumParcels; ++i )
    this->TileMap[i].Variation = (int)((double)rand() * 60.0 / 32767.0);
  this->ViewType = 21;
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
  D3DXMatrixTranslation(&this->WireframeLiftupMatrix, 0.0f, 0.02f, 0.0f);
  this->WireframeMaterial.SetDiffuse( 1.0, 1.0, 1.0, 0.5);
  this->WireframeMaterial.SetAmbient( 0.5, 1.0, 0.5, 1.0);
  this->Wireframe2Material.SetDiffuse( 1.0, 1.0, 1.0, 1.0);
  this->Wireframe2Material.SetAmbient( 0.5, 1.0, 0.5, 1.0);
  this->SketchTexture = -1;
  this->VisMap = 0;
  this->VisMap2 = 0;
  this->FogMode = 0;
  *(_WORD *)&this->GodMode = 0;
  this->CompactMode = 0;
  this->BlockMap = 0;
  this->CurrentBlockMap = 0;
  this->LimitMesh = 0;
  this->LimitTexture = -1;
}

//----- (0045C870) --------------------------------------------------------

STerrain::~STerrain()

{
  int *TileTextures; // edi
  int v3;
  float *HeightMap; // eax
  float *OldHeightMap; // eax
  unsigned int *ColorMap; // eax
  D3DXVECTOR3 *NormalMap; // eax
  unsigned char *ShadowMap; // eax
  unsigned char *WaterMap; // eax
  int v10;
  int v11;
  SParcelInfo *Parcels; // eax
  SParcel *Mesh; // ecx
  SParcel2 *Mesh2; // ecx
  SMesh *BlockMapMesh; // ecx
  SParcelInfo *v16; // eax
  STile *TileMap; // eax
  SMesh *LimitMesh; // ecx
  SGepard *Gepard; // ecx
  char *buf; // eax
  TileTextures = this->TileTextures;
  v3 = 10;
  do
  {
    this->Gepard->ReleaseTexture(*TileTextures++, 0);
    --v3;
  }
  while ( v3 );
  HeightMap = this->HeightMap;
  if ( HeightMap )
  {
    delete[] HeightMap;
    this->HeightMap = 0;
  }
  OldHeightMap = this->OldHeightMap;
  if ( OldHeightMap )
  {
    delete[] OldHeightMap;
    this->OldHeightMap = 0;
  }
  if ( this->BlendMap )
  {
    operator delete[](this->BlendMap);
    this->BlendMap = 0;
  }
  ColorMap = this->ColorMap;
  if ( ColorMap )
  {
    delete[] ColorMap;
    this->ColorMap = 0;
  }
  NormalMap = this->NormalMap;
  if ( NormalMap )
  {
    delete[] NormalMap;
    this->NormalMap = 0;
  }
  ShadowMap = this->ShadowMap;
  if ( ShadowMap )
  {
    delete[] ShadowMap;
    this->ShadowMap = 0;
  }
  WaterMap = this->WaterMap;
  if ( WaterMap )
  {
    delete[] WaterMap;
    this->WaterMap = 0;
  }
  v10 = 0;
  if ( this->NumParcels > 0 )
  {
    v11 = 0;
    do
    {
      Parcels = this->Parcels;
      Mesh = Parcels[v11].Mesh;
      if ( Mesh )
      {
        Mesh->Release();
        this->Parcels[v11].Mesh = 0;
        Parcels = this->Parcels;
      }
      Mesh2 = Parcels[v11].Mesh2;
      if ( Mesh2 )
      {
        Mesh2->Release();
        this->Parcels[v11].Mesh2 = 0;
        Parcels = this->Parcels;
      }
      BlockMapMesh = Parcels[v11].BlockMapMesh;
      if ( BlockMapMesh )
      {
        BlockMapMesh->Release();
        this->Parcels[v11].BlockMapMesh = 0;
      }
      ++v10;
      ++v11;
    }
    while ( v10 < this->NumParcels );
  }
  v16 = this->Parcels;
  if ( v16 )
  {
    delete[] v16;
    this->Parcels = 0;
  }
  TileMap = this->TileMap;
  if ( TileMap )
  {
    delete[] TileMap;
    this->TileMap = 0;
  }
  LimitMesh = this->LimitMesh;
  if ( LimitMesh )
  {
    LimitMesh->Release();
    this->LimitMesh = 0;
  }
  this->Gepard->ReleaseTexture(this->LimitTexture, 0);
  Gepard = this->Gepard;
  this->LimitTexture = -1;
  Gepard->ReleaseTexture(this->SketchTexture, 0);
  buf = this->TilePrefix.buf;
  if ( buf )
  {
    delete[] buf;
    this->TilePrefix.buf = 0;
  }
}

//----- (0045CB40) --------------------------------------------------------

void STerrain::Acquire(float **heightmap, float **oldheightmap, unsigned char (**blendmap)[9],

        STile **tilemap,

        unsigned char **watermap)

{
  *heightmap = this->HeightMap;
  *oldheightmap = this->OldHeightMap;
  *blendmap = this->BlendMap;
  *tilemap = this->TileMap;
  *watermap = this->WaterMap;
}

//----- (0045CB80) --------------------------------------------------------

void STerrain::CalculateVisibleParcels(D3DXMATRIX *CameraMatrix, D3DXMATRIX *ProjectionMatrix)

{
  float _22; // xmm0_4
  float v5; // xmm4_4
  float v6; // xmm5_4
  float v7; // xmm6_4
  float v8; // xmm7_4
  float v9; // xmm5_4
  float v10; // xmm4_4
  float v11; // xmm0_4
  float v12; // xmm5_4
  int ZSize;
  STerrain *v14; // ecx
  float v15; // xmm0_4
  int v16;
  int v17;
  float v18; // xmm3_4
  float v19; // xmm1_4
  float v20; // xmm2_4
  float v21; // xmm5_4
  float v22; // xmm4_4
  int v23;
  STerrain *v24; // esi
  SParcelInfo *Parcels; // edx
  float LowPoint; // xmm0_4
  bool v27; // al
  int v28;
  D3DXPLANE bottom;
  D3DXPLANE right;
  D3DXPLANE left;
  D3DXPLANE top;
  float v33;
  float v34;
  float v35;
  int v36;
  float v37;
  float v38;
  STerrain *v39;
  int v40;
  float v41;
  float v42;
  D3DXMATRIX mx;
  D3DXVECTOR3 cam;
  D3DXVECTOR3 norm;
  v39 = this;
  D3DXMatrixTranspose(&mx, CameraMatrix);
  _22 = ProjectionMatrix->_22;
  memset(&cam, 0, sizeof(cam));
  norm.x = 0.0;
  norm.y = _22;
  norm.z = -1.0;
  D3DXVec3Normalize(&norm, &norm);
  D3DXPlaneFromPointNormal(&top, &cam, &norm);
  D3DXPlaneTransform(&top, &top, &mx);
  norm.y = -norm.y;
  D3DXPlaneFromPointNormal(&bottom, &cam, &norm);
  D3DXPlaneTransform(&bottom, &bottom, &mx);
  norm.x = ProjectionMatrix->_11;
  norm.y = 0.0;
  norm.z = -1.0;
  D3DXVec3Normalize(&norm, &norm);
  D3DXPlaneFromPointNormal(&right, &cam, &norm);
  D3DXPlaneTransform(&right, &right, &mx);
  norm.x = -norm.x;
  D3DXPlaneFromPointNormal(&left, &cam, &norm);
  D3DXPlaneTransform(&left, &left, &mx);
  v5 = (float)(top.a * (float)(-1.0f / top.b)) * 8.0f;
  v6 = (float)(top.c * (float)(-1.0f / top.b)) * 8.0f;
  v34 = v5;
  v33 = v6;
  v7 = top.d * (float)(-1.0 / top.b);
  if ( v5 > 0.0 )
    v7 = v7 + v5;
  if ( v6 > 0.0 )
    v7 = v7 + v6;
  v8 = (float)(left.a * (float)(-1.0f / left.b)) * 8.0f;
  v9 = (float)(left.c * (float)(-1.0f / left.b)) * 8.0f;
  v10 = left.d * (float)(-1.0 / left.b);
  v38 = v9;
  v42 = v10;
  if ( v8 > 0.0 )
  {
    v10 = v10 + v8;
    v42 = v10;
  }
  if ( v9 > 0.0 )
  {
    v10 = v10 + v9;
    v42 = v10;
  }
  v11 = (float)(right.a * (float)(-1.0f / right.b)) * 8.0f;
  v37 = (float)(right.c * (float)(-1.0f / right.b)) * 8.0f;
  v12 = right.d * (float)(-1.0 / right.b);
  v35 = v11;
  v41 = v12;
  if ( v11 > 0.0 )
  {
    v12 = v12 + v11;
    v41 = v12;
  }
  if ( v37 > 0.0 )
  {
    v12 = v12 + v37;
    v41 = v12;
  }
  ZSize = this->ZSize;
  v40 = 0;
  v36 = 0;
  if ( ZSize / 8 > 0 )
  {
    v14 = this;
    v15 = v38;
    v16 = this->XSize / 8;
    do
    {
      v17 = 0;
      v18 = v7;
      v19 = v10;
      v20 = v12;
      if ( v16 > 0 )
      {
        v21 = v35;
        v22 = v34;
        v23 = v40;
        do
        {
          v24 = v39;
          Parcels = v39->Parcels;
          LowPoint = Parcels[v23].LowPoint;
          v27 = v18 >= LowPoint && v19 >= LowPoint && v20 >= LowPoint;
          ++v40;
          Parcels[v23].Visible = v27;
          v18 = v18 + v22;
          v19 = v19 + v8;
          ++v17;
          v20 = v20 + v21;
          v16 = v24->XSize / 8;
          ++v23;
        }
        while ( v17 < v16 );
        v10 = v42;
        v12 = v41;
        v15 = v38;
        v14 = v39;
      }
      v10 = v10 + v15;
      v12 = v12 + v37;
      v7 = v7 + v33;
      v28 = v14->ZSize;
      v42 = v10;
      v41 = v12;
      ++v36;
    }
    while ( v36 < v28 / 8 );
  }
}

//----- (0045CEF0) --------------------------------------------------------

void STerrain::ComputeShadows()

{
  int v2;
  float v3; // xmm1_4
  SGepard *Gepard; // esi
  int XSize;
  float *p_x; // esi
  int v7;
  float *HeightMap; // edx
  int Stride;
  int v10;
  int v11;
  float *v12; // ecx
  float *v13; // eax
  float *v14; // edx
  float *v15; // esi
  float *v16; // eax
  float v17; // xmm0_4
  float v18; // xmm0_4
  float v19; // xmm1_4
  float v20; // xmm1_4
  float v21; // xmm0_4
  float v22; // xmm0_4
  float v23; // xmm1_4
  bool v24;
  int v25;
  int v26;
  float *v27; // eax
  int v28;
  float v29; // xmm0_4
  float v30; // xmm0_4
  int v31;
  float v32;
  int v33;
  float v34;
  int v35;
  int v36;
  float v37;
  char v38; // al
  float v39;
  int v40;
  int v41;
  float v42; // xmm0_4
  float v43; // xmm1_4
  float v44; // xmm2_4
  double v45; // st7
  float *v46; // ecx
  float v47; // xmm2_4
  float v48; // xmm1_4
  float v49;
  int v50;
  int v51;
  float v52;
  float v53;
  float v54;
  float v55;
  float v56; // xmm0_4
  float v57; // xmm5_4
  float ZSize; // xmm4_4
  double v59; // st7
  float *v60; // ecx
  float v61; // xmm2_4
  float v62; // xmm1_4
  float v63;
  int v64;
  float v65;
  float v66;
  float v67;
  float v68;
  float v69; // xmm4_4
  int v70;
  double v71;
  double v72;
  float v73;
  float v74;
  double v75;
  float *v76;
  float v77;
  float *v78;
  int v79;
  int v80;
  float v81;
  unsigned int v82;
  int v83;
  int v84;
  float *v85;
  float v86;
  float v87;
  float v88;
  float v89;
  int v90;
  char v91;
  v2 = 0;
  v3 = 0.0;
  v83 = 0;
  v81 = 0.0;
  Gepard = this->Gepard;
  XSize = this->XSize;
  p_x = &Gepard->SunDir.x;
  v76 = p_x;
  if ( XSize >= 0 )
  {
    do
    {
      v7 = 0;
      if ( XSize >= 0 )
      {
        if ( XSize + 1 >= 4 )
        {
          HeightMap = this->HeightMap;
          Stride = this->Stride;
          v10 = 16 * Stride;
          v78 = &HeightMap[v83];
          v85 = &HeightMap[2 * Stride + v83];
          v11 = Stride + v83;
          v12 = &HeightMap[2 * Stride + Stride + v83];
          v13 = &HeightMap[v11];
          v14 = v85;
          v15 = v13;
          v82 = (unsigned int)(this->XSize + 1) >> 2;
          v90 = 4 * v82;
          v16 = v78;
          do
          {
            v17 = *v16;
            v16 = (float *)((char *)v16 + v10);
            v18 = fmaxf(v17, v3);
            v19 = *v15;
            v15 = (float *)((char *)v15 + v10);
            v20 = fmaxf(v19, v18);
            v21 = *v14;
            v14 = (float *)((char *)v14 + v10);
            v22 = fmaxf(v21, v20);
            v23 = *v12;
            v12 = (float *)((char *)v12 + v10);
            v24 = v82-- == 1;
            v3 = fmaxf(v23, v22);
          }
          while ( !v24 );
          v2 = v83;
          v7 = v90;
          XSize = this->XSize;
          v81 = v3;
        }
        if ( v7 <= XSize )
        {
          v25 = this->Stride;
          v26 = 4 * v25;
          v27 = &this->HeightMap[v2 + v7 * v25];
          v28 = this->XSize - v7 + 1;
          do
          {
            v29 = *v27;
            v27 = (float *)((char *)v27 + v26);
            v30 = fmaxf(v29, v3);
            v3 = v30;
            --v28;
          }
          while ( v28 );
          XSize = this->XSize;
          v81 = v30;
        }
      }
      v83 = ++v2;
    }
    while ( v2 <= XSize );
    p_x = v76;
  }
  v31 = this->XSize;
  v84 = 0;
  if ( v31 >= 0 )
  {
    v32 = DOUBLE_0_25;
    v33 = this->XSize;
    v34 = DOUBLE_0_375;
    do
    {
      v35 = 0;
      v36 = v33;
      v79 = 0;
      if ( v33 >= 0 )
      {
        v37 = (float)((double)v84);

        do
        {
          v38 = 0;
          v39 = (float)((double)v35);

          v40 = 0;
          v91 = 0;
          v80 = 0;
          v72 = (double)v35;
          do
          {
            v41 = 0;
            v42 = (float)((double)v40 * v32 + v37 - v34);

            v73 = v42;
            do
            {
              v43 = v42;
              v88 = v42;
              v44 = (float)((double)v41 * v32 + v39 - v34);

              v86 = v44;
              if ( v42 < 0.0 || v42 >= (float)v31 || v44 < 0.0 || v44 >= (float)this->ZSize )
              {
                v56 = 0.0;
              }
              else
              {
                v75 = floor(v44);
                v45 = floor(v42);
                v46 = this->HeightMap;
                v47 = (float)(v75);

                v48 = (float)(v45);

                v49 = (float)(v42 - v48);
                v50 = (int)v48 + this->Stride * (int)v47;
                v51 = v50 + this->XSize;
                v52 = v46[v50 + 1];
                v53 = (float)(v86 - v47);
                v54 = v46[v50];
                v31 = this->XSize;
                v43 = v88;
                v44 = v86;
                v55 = v52 * (1.0f - v53) * v49 + v54 * (1.0f - v53) * (1.0f - v49) + v46[v51 + 1] * v53 * (1.0f - v49);
                *(float *)&v54 = v46[v51 + 2];
                v38 = v91;
                v56 = v55 + *(float *)&v54 * v53 * v49;
              }
              if ( v43 >= 0.0 )
              {
                v57 = (float)v31;
                v74 = (float)v31;
                while ( 1 )
                {
                  if ( v57 <= v43
                    || v44 < 0.0
                    || (ZSize = (float)this->ZSize, ZSize <= v44)
                    || (v56 = v56 - p_x[1],
                        v43 = v43 - *p_x,
                        v44 = v44 - p_x[2],
                        v89 = v43,
                        v77 = v56,
                        v87 = v44,
                        v56 > v81) )
                  {
LABEL_39:
                    v38 = v91;
                    goto LABEL_40;
                  }
                  if ( v43 < 0.0 || v43 >= v57 || v44 < 0.0 || v44 >= ZSize )
                  {
                    v69 = 0.0;
                  }
                  else
                  {
                    v71 = floor(v44);
                    v59 = floor(v43);
                    v60 = this->HeightMap;
                    v61 = (float)(v71);

                    v62 = (float)(v59);

                    v63 = (float)(v89 - v62);
                    v64 = (int)v62 + this->Stride * (int)v61;
                    v65 = (float)(v87 - v61);
                    v43 = v89;
                    v44 = v87;
                    v66 = (float)(v60[v64 + 1] * (1.0 - v65) * v63
                        + v60[v64] * (1.0 - v65) * (1.0 - v63)
                        + v60[v64 + 1 + this->XSize] * v65 * (1.0 - v63));
                    v67 = v60[v64 + 2 + this->XSize] * v65 * v63;
                    v57 = v74;
                    v68 = v66 + v67;
                    v56 = v77;
                    v69 = v68;
                  }
                  if ( v69 > v56 )
                    break;
                  if ( v43 < 0.0 )
                    goto LABEL_39;
                }
                v38 = ++v91;
LABEL_40:
                v31 = this->XSize;
              }
              v42 = v73;
              ++v41;
              v32 = DOUBLE_0_25;
              v34 = DOUBLE_0_375;
              v39 = (float)(v72);

            }
            while ( v41 < 4 );
            v37 = (float)((double)v84);

            v40 = v80 + 1;
            v80 = v40;
          }
          while ( v40 < 4 );
          v70 = v79 * this->Stride;
          v35 = ++v79;
          this->ShadowMap[v84 + v70] = 0x80 - 8 * v38;
          v31 = this->XSize;
        }
        while ( v79 <= v31 );
        v36 = this->XSize;
      }
      v33 = v36;
      ++v84;
    }
    while ( v84 <= v36 );
  }
}

//----- (0045D420) --------------------------------------------------------

void STerrain::Draw()

{
  STerrain *v1; // esi
  SGepard *Gepard; // ecx
  SGepard *v3; // edx
  int ViewType;
  bool v5; // cl
  int v6;
  int v7;
  int XParcels;
  int v9;
  int v10;
  int v11;
  unsigned int *v12; // ecx
  int FogMode;
  int v15;
  int v16;
  int v17;
  SParcelInfo *Parcels; // ecx
  float XSize; // xmm1_4
  IDirect3DDevice9 *lpD3DDev; // eax
  int SketchTexture;
  int v22;
  int v23;
  int v24;
  int v25;
  int v26;
  int v27;
  SParcelInfo *v28; // ecx
  int v29;
  int v30;
  SParcelInfo *v31; // ecx
  unsigned char *vismap;
  int v34;
  int v35;
  int v36;
  float sketchTransform[4];
  v1 = this;
  {
    static int drawLogCount = 0;
    if (drawLogCount < 3) {
      int vis = 0, mesh2Count = 0, meshCount = 0;
      for (int i = 0; i < this->NumParcels; i++) {
        if (this->Parcels[i].Visible) vis++;
        if (this->Parcels[i].Mesh2) mesh2Count++;
        if (this->Parcels[i].Mesh) meshCount++;
      }
      #ifdef HD_DEBUG_TERRAIN
      Logger.g->Log(0, "STerrain::Draw: ViewType=%d CompactMode=%d Flags[0]=%d VisibleParcels=%d/%d Mesh=%d Mesh2=%d XParcels=%d ZParcels=%d",
        this->ViewType, (int)this->CompactMode, (int)this->Gepard->Flags[0], vis, this->NumParcels, meshCount, mesh2Count, this->XParcels, this->ZParcels);
      #endif
      #ifdef HD_DEBUG_TERRAIN
      Logger.g->Log(0, "STerrain::Draw: terrainVS[0]=%p terrainPS[0]=%p FullBright=%d VisMap=%p ColorMap=%p",
        this->Gepard->terrainVertexShader[0].Ptr, this->Gepard->terrainPixelShader[0].Ptr,
        (int)this->Gepard->FullBright, this->VisMap, this->ColorMap);
      #endif
      if (vis > 0) {
        for (int i = 0; i < this->NumParcels && drawLogCount < 3; i++) {
          if (this->Parcels[i].Visible) {
            #ifdef HD_DEBUG_TERRAIN
            Logger.g->Log(0, "STerrain::Draw: Parcel[%d] Mesh=%p Mesh2=%p", i, this->Parcels[i].Mesh, this->Parcels[i].Mesh2);
            #endif
            break;
          }
        }
      }
      drawLogCount++;
    }
  }
  this->Gepard->SetDrawType(DT_NORMAL);
  v1->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 1u);
  if ( (v1->ViewType & 1) != 0 )
  {
    Gepard = v1->Gepard;
    if ( Gepard->Flags[0] )
    {
      Gepard->SetWorldViewProjVertexShaderConstantBuffer(&v1->IdentityMatrix);
    }
    else
    {
      Gepard->SetLightingType(LT_PRELIT);
      v1->lpD3DDev->SetTransform((_D3DTRANSFORMSTATETYPE)256, &v1->IdentityMatrix);
    }
    v3 = v1->Gepard;
    ViewType = v1->ViewType;
    v5 = v3->Flags[0];
    if ( (ViewType & 4) != 0 )
    {
      if ( v5 )
      {
        v1->lpD3DDev->SetVertexShader(v3->terrainVertexShader[0].Ptr);
        v1->lpD3DDev->SetPixelShader(v1->Gepard->terrainPixelShader[0].Ptr);
      }
      if ( v1->CompactMode )
      {
        v6 = 0;
        v36 = 0;
        if ( v1->ZParcels > 0 )
        {
          v7 = -2;
          v35 = -2;
          do
          {
            XParcels = v1->XParcels;
            v9 = 0;
            if ( XParcels > 0 )
            {
              v34 = 0;
              do
              {
                v10 = v9 + v6 * XParcels;
                v11 = XParcels;
                if ( v1->Parcels[v9 + v6 * XParcels].Visible )
                {
                  v12 = &v1->ColorMap[8 * v9 + 8 * v36 * v1->Stride];
                  // x64: was `v13 = (int)&v1->VisMap[...]; vismap =
                  // (unsigned char*)v13` — truncates VisMap[] heap pointer
                  // through int. Pass the typed pointer directly.
                  vismap = &v1->VisMap[v35 * v1->Stride - 1 + v34];
                  v1 = this;
                  if ( this->Gepard->FullBright )
                  {
                    this->Parcels[v10].Mesh2->UpdateFullBright();
                  }
                  else if ( this->VisMap )
                  {
                    if ( v9 >= 6
                      && v9 < XParcels - 6
                      && v35 >= 190
                      && v36 < this->ZParcels - 6
                      && (FogMode = this->FogMode) != 0 )
                    {
                      v15 = (FogMode != 1) + 2;
                    }
                    else
                    {
                      v15 = this->FogMode == 1;
                    }
                    this->Parcels[v10].Mesh2->Update(v12, vismap, this->Stride, v15);
                  }
                  {
                    static int drawCallLog = 0;
                    if (drawCallLog < 3) {
                      SMesh *m = (SMesh*)this->Parcels[v10].Mesh2;
                      #ifdef HD_DEBUG_TERRAIN
                      Logger.g->Log(0, "Parcel2Draw[%d]: VB=%p IB=%p NumVerts=%d NumIdx=%d NumMats=%d VertSize=%d FVF=0x%X",
                        v10, m->lpVertexBuffer, m->lpIndexBuffer ? m->lpIndexBuffer[0] : nullptr,
                        m->NumVertices, m->NumIndices ? m->NumIndices[0] : 0,
                        m->NumMaterials, m->VertexSize, m->VertexFormat);
                      #endif
                      drawCallLog++;
                    }
                  }
                  this->Parcels[v10].Mesh2->Draw(4u, 0, 0.0);
                  v11 = this->XParcels;
                }
                v34 += 16;
                ++v9;
                v6 = v36;
                XParcels = v11;
              }
              while ( v9 < v11 );
              v7 = v35;
            }
            ++v6;
            v7 += 32;
            v36 = v6;
            v35 = v7;
          }
          while ( v6 < v1->ZParcels );
        }
      }
      else
      {
        v16 = 0;
        if ( v1->NumParcels > 0 )
        {
          v17 = 0;
          do
          {
            Parcels = v1->Parcels;
            if ( Parcels[v17].Visible )
              Parcels[v17].Mesh->DrawLayered(v1->TileMap[v16].Level, v1->TileTextures);
            ++v16;
            ++v17;
          }
          while ( v16 < v1->NumParcels );
        }
      }
    }
    else
    {
      if ( v5 )
      {
        v1->lpD3DDev->SetVertexShader(v3->terrainVertexShader[1].Ptr);
        v1->lpD3DDev->SetPixelShader(v1->Gepard->terrainPixelShader[1].Ptr);
        XSize = (float)v1->XSize;
        lpD3DDev = v1->lpD3DDev;
        sketchTransform[2] = 0.0;
        sketchTransform[3] = 0.0;
        sketchTransform[0] = 1.0f / XSize;
        sketchTransform[1] = 1.0f / (float)v1->ZSize;
        lpD3DDev->SetVertexShaderConstantF(12u, sketchTransform, 1u);
        ViewType = v1->ViewType;
        v3 = v1->Gepard;
      }
      if ( (ViewType & 8) != 0 )
        SketchTexture = v1->SketchTexture;
      else
        SketchTexture = v3->_gray_texture;
      v3->SetTexture(0, SketchTexture, 1);
      v22 = 0;
      if ( v1->NumParcels > 0 )
      {
        v23 = 0;
        do
        {
          if ( v1->Parcels[v23].Visible )
 v1->Parcels[v23].Mesh->DrawSketch(v22 % v1->XParcels, v22 / v1->XParcels, v1->XParcels, v1->ZParcels);
          ++v22;
          ++v23;
        }
        while ( v22 < v1->NumParcels );
      }
    }
    if ( v1->Gepard->Flags[0] )
    {
      v1->lpD3DDev->SetVertexShader(0);
      v1->lpD3DDev->SetPixelShader(0);
    }
  }
  if ( v1->BlockMapMode )
  {
    v1->lpD3DDev->SetRenderState(D3DRS_SHADEMODE, 1u);
    v1->lpD3DDev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, 1u);
    v1->Gepard->SetDrawType(DT_ADD);
    v1->Gepard->SetTexture(0, -1, 1);
    // Disable fog around the blockmap mesh draw. The mesh inherits whatever
    // shader was bound by the lighting passes; with main map fog active that
    // shader's VS oFog (driven by lcb.FogParams at c20) blends the far half
    // of the blockmap into FOGCOLOR, drowning out the red/green block visual.
    // Shipped editor.exe shows the blockmap unfogged at any distance.
    v1->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 0);
    float vsNoFog[4] = { 0.0f, 1.0f, 0.0f, 0.0f };  // GetFog -> 1 -> no blend
    v1->lpD3DDev->SetVertexShaderConstantF(20, vsNoFog, 1);
    v24 = 0;
    if ( v1->NumParcels > 0 )
    {
      v25 = 0;
      do
      {
        if ( v1->Parcels[v25].Visible )
        {
          v1->UpdateBlockMap(v24);
          v1->Parcels[v25].BlockMapMesh->Draw(0, 0, 0.0);
        }
        ++v24;
        ++v25;
      }
      while ( v24 < v1->NumParcels );
    }
    v1->lpD3DDev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, 0);
    v1->lpD3DDev->SetRenderState(D3DRS_SHADEMODE, 2u);
    v1->Gepard->SetDrawType(DT_NORMAL);
    // Restore fog state (FFP + shader c20) for the wireframe pass below.
    v1->Gepard->EnableFog();
  }
  // Wireframe LINELIST pass — fixed-function, matches shipped binary.
  // Reset any bound vertex/pixel shader so fixed-function lighting path takes
  // effect (filled pass at line 943 skipped its Flags[0] reset for ViewType=1).
  v1->lpD3DDev->SetVertexShader(NULL);
  v1->lpD3DDev->SetPixelShader(NULL);
  v1->Gepard->SetTexture(0, -1, 1);
  v1->Gepard->SetLightingType(LT_AMBIENT);
  v1->Gepard->SetAmbientColor(0xFFFFFFu);
  v1->lpD3DDev->SetTransform((_D3DTRANSFORMSTATETYPE)256, &v1->WireframeLiftupMatrix);
  v1->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
  v1->lpD3DDev->SetRenderState(D3DRS_SRCBLEND, 5u);
  v1->lpD3DDev->SetRenderState(D3DRS_DESTBLEND, 6u);
  // Force material-sourced ambient/diffuse so WireframeMaterial.Ambient
  // (0.5, 1.0, 0.5) = green is used instead of the mesh's per-vertex color.
  // Corresponds to Ghidra's raw (0x93, 0) and (0x91, 0) states.
  v1->lpD3DDev->SetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL);
  v1->lpD3DDev->SetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_MATERIAL);
  if ( (v1->ViewType & 2) != 0 || v1->BlockMapMode )
  {
    v1->Gepard->SetMaterial(&v1->WireframeMaterial);
    v26 = 0;
    if ( v1->NumParcels > 0 )
    {
      v27 = 0;
      do
      {
        v28 = v1->Parcels;
        if ( v28[v27].Visible )
          v28[v27].Mesh->DrawWireframe(1);
        ++v26;
        ++v27;
      }
      while ( v26 < v1->NumParcels );
    }
  }
  if ( (v1->ViewType & 0x20) != 0 )
  {
    v1->Gepard->SetMaterial(&v1->Wireframe2Material);
    v29 = 0;
    if ( v1->NumParcels > 0 )
    {
      v30 = 0;
      do
      {
        v31 = v1->Parcels;
        if ( v31[v30].Visible )
          v31[v30].Mesh->DrawWireframe(2);
        ++v29;
        ++v30;
      }
      while ( v29 < v1->NumParcels );
    }
  }
  v1->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
}

//----- (0045D940) --------------------------------------------------------

void STerrain::DrawLighting(int a2)

{
  SGepard *Gepard; // ecx
  int v4;
  int v5;
  SParcelInfo *Parcels; // eax
  Gepard = this->Gepard;
  if ( Gepard->Flags[0] && Gepard->GetShadowQuality() > 1 && (this->ViewType & 1) != 0 )
  {
    this->lpD3DDev->SetRenderState((D3DRENDERSTATETYPE)34, 8421504);
    this->lpD3DDev->SetVertexShader(this->Gepard->terrainLightingVertexShader.Ptr);
    this->lpD3DDev->SetPixelShader(this->Gepard->terrainLightingPixelShader.Ptr);
    this->lpD3DDev->SetRenderState(D3DRS_SRCBLEND, 9u);
    this->lpD3DDev->SetRenderState(D3DRS_DESTBLEND, 3u);
    this->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
    this->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
    this->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
    this->Gepard->SetWorldViewProjVertexShaderConstantBuffer(&this->IdentityMatrix);
    v4 = 0;
    if ( this->NumParcels > 0 )
    {
      v5 = 0;
      do
      {
        Parcels = this->Parcels;
        if ( Parcels[v5].Visible )
          Parcels[v5].Mesh->DrawSimple();
        ++v4;
        ++v5;
      }
      while ( v4 < this->NumParcels );
    }
    this->lpD3DDev->SetVertexShader(0);
    this->lpD3DDev->SetPixelShader(0);
    this->Gepard->EnableFog();
  }
}

//----- (0045DA70) --------------------------------------------------------

void STerrain::DrawLimits()

{
  SGepard *Gepard; // edx
  IDirect3DDevice9 *lpD3DDev; // eax
  float decalColor[4] = { 1.0f, 0.0f, 0.0f, 0.5f };
  Gepard = this->Gepard;
#ifdef HD_HDBEEFUP_SETTINGS
  if ( Options && !Options->GetMapBorder() )
    return;
#endif
  if ( !Gepard->Flags[1] )
  {
    if ( this->LimitMesh )
    {
      this->lpD3DDev->SetVertexShader(Gepard->unlitDecalVertexShader.Ptr);
      this->lpD3DDev->SetPixelShader(this->Gepard->unlitDecalPixelShader.Ptr);
      this->Gepard->SetTexture(0, this->LimitTexture, 1);
      this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, 4u);
      this->lpD3DDev->SetSamplerState(0, D3DSAMP_BORDERCOLOR, 0);
      lpD3DDev = this->lpD3DDev;
      lpD3DDev->SetPixelShaderConstantF(0, decalColor, 1u);
      this->LimitMesh->Draw(0, 0, 0.0);
      this->lpD3DDev->SetSamplerState(0, D3DSAMP_ADDRESSV, 1u);
      this->lpD3DDev->SetVertexShader(0);
      this->lpD3DDev->SetPixelShader(0);
    }
  }
}

//----- (0045DB70) --------------------------------------------------------

void STerrain::DrawShadow2()

{
  int v2;
  int v3;
  SParcelInfo *Parcels; // eax
  this->lpD3DDev->SetVertexShader(this->Gepard->depthWriteVertexShaders[0].Ptr);
  this->lpD3DDev->SetPixelShader(this->Gepard->depthWritePixelShaders[0].Ptr);
  this->Gepard->SetWorldViewProjVertexShaderConstantBuffer(&this->IdentityMatrix);
  this->Gepard->SetTexture(0, -1, 1);
  this->lpD3DDev->SetTransform((_D3DTRANSFORMSTATETYPE)256, &this->IdentityMatrix);
  v2 = 0;
  if ( this->NumParcels > 0 )
  {
    v3 = 0;
    do
    {
      Parcels = this->Parcels;
      if ( Parcels[v3].Visible )
        Parcels[v3].Mesh->DrawSimple();
      ++v2;
      ++v3;
    }
    while ( v2 < this->NumParcels );
  }
  this->lpD3DDev->SetVertexShader(0);
  this->lpD3DDev->SetPixelShader(0);
}

//----- (0045DC20) --------------------------------------------------------

void STerrain::DrawShadow()

{
  int v2;
  int v3;
  this->Gepard->SetTexture(0, -1, 1);
  this->lpD3DDev->SetTransform((_D3DTRANSFORMSTATETYPE)256, &this->IdentityMatrix);
  v2 = 0;
  if ( this->NumParcels > 0 )
  {
    v3 = 0;
    do
    {
      this->Parcels[v3].Mesh->DrawSimple();
      ++v2;
      ++v3;
    }
    while ( v2 < this->NumParcels );
  }
}

//----- (0045DC70) --------------------------------------------------------

void STerrain::EnableGodMode(bool enable)

{
  this->GodMode = enable;
}

//----- (0045DC80) --------------------------------------------------------

bool STerrain::GetBlockMapMode()

{
  return this->BlockMapMode;
}

//----- (0045DC90) --------------------------------------------------------

bool STerrain::GetCompactMode()

{
  return this->CompactMode;
}

//----- (0045DCA0) --------------------------------------------------------

D3DXVECTOR3 *STerrain::GetEdgeNormal(D3DXVECTOR3 *result, float x, float z)

{
  float v5; // xmm2_4
  float v6; // xmm1_4
  D3DXVECTOR3 *Normal; // eax
  float v9; // xmm4_4
  float v10;
  float x_low; // xmm3
  float y_low; // xmm2
  float v13; // xmm4_4
  float v14; // xmm1_4
  D3DXVECTOR3 nv1;
  float xker;
  double v17;
  float zker;
  double X;
  double v20;
  D3DXVECTOR3 v22;
  D3DXVECTOR3 nv2;
  *(double *)&v22.y = x;
  *(double *)&nv2.y = x + 0.5;
  xker = (float)(floor(*(double *)&nv2.y));

  X = z;
  zker = (float)(floor(z + 0.5));

  v5 = fabsf(xker - x);
  v6 = fabsf(zker - z);
  if ( v5 >= 0.0001 || v6 >= 0.0001 )
  {
    if ( v6 < v5 )
    {
      v20 = floor(*(double *)&v22.y);
      nv1 = *this->GetNormal(&nv2, (int)(float)v20, (int)zker);
      *(double *)&nv2.y = ceil(*(double *)&v22.y);
      Normal = this->GetNormal(&v22, (int)(float)*(double *)&nv2.y, (int)zker);
      v9 = x;
      *(_QWORD *)&nv2.x = *(_QWORD *)&Normal->x;
      v10 = (float)(v20);

    }
    else
    {
      v17 = floor(X);
      nv1 = *this->GetNormal(&nv2, (int)xker, (int)(float)v17);
      *(double *)&nv2.y = ceil(X);
      Normal = this->GetNormal(&v22, (int)xker, (int)(float)*(double *)&nv2.y);
      v9 = z;
      *(_QWORD *)&nv2.x = *(_QWORD *)&Normal->x;
      v10 = (float)(v17);

    }
    x_low = nv2.x;
    y_low = nv2.y;
    nv2.z = Normal->z;
    v13 = v9 - (float)v10;
    x_low = (float)((float)(nv2.x - nv1.x) * v13) + nv1.x;
    y_low = (float)((float)(nv2.y - nv1.y) * v13) + nv1.y;
    v14 = (float)((float)(nv2.z - nv1.z) * v13) + nv1.z;
    result->x = x_low; result->y = y_low;
    result->z = v14;
    return result;
  }
  else
  {
    this->GetNormal(result, (int)*(double *)&nv2.y, (int)(z + 0.5));
    return result;
  }
}

//----- (0045DF20) --------------------------------------------------------

double STerrain::GetHeight(int x, int z)

{
  if ( x < 0 || x > this->XSize || z < 0 || z > this->ZSize )
    return 0.0;
  else
    return this->HeightMap[x + z * this->Stride];
}

//----- (0045DF60) --------------------------------------------------------

double STerrain::GetHeight(float x, float z)

{
  int XSize;
  double v5; // st7
  float *HeightMap; // ecx
  float v7; // xmm1_4
  float v8; // xmm0_4
  int v9;
  float v10;
  float v11;
  double v13;
  if ( x < 0.0 )
    return 0.0;
  XSize = this->XSize;
  if ( x >= (float)XSize || z < 0.0 || z >= (float)this->ZSize )
    return 0.0;
  v13 = floor(z);
  v5 = floor(x);
  HeightMap = this->HeightMap;
  v7 = (float)(v13);

  v8 = (float)(v5);

  v9 = (int)v8 + this->Stride * (int)v7;
  v10 = (float)(x - v8);
  v11 = (float)(z - v7);
  return (float)(HeightMap[v9 + 1] * (1.0 - v11) * v10
               + HeightMap[v9] * (1.0 - v11) * (1.0 - v10)
               + HeightMap[XSize + 1 + v9] * v11 * (1.0 - v10)
               + HeightMap[XSize + 2 + v9] * v11 * v10);
}

//----- (0045E0B0) --------------------------------------------------------

double STerrain::GetHeightTriangular(float x, float z)

{
  double v4; // st7
  int Stride;
  float v6; // xmm0_4
  float v7; // xmm1_4
  float v8;
  float v9;
  int v10;
  float *HeightMap; // eax
  int v12;
  double v15;
  if ( x < 0.0 || x >= (float)this->XSize || z < 0.0 || z >= (float)this->ZSize )
    return 0.0;
  v4 = floor(z);
  Stride = this->Stride;
  v15 = v4;
  v6 = (float)(floor(x));

  v7 = (float)(v15);

  v8 = (float)(z - v7);
  v9 = (float)(x - v6);
  v10 = (int)v6 + Stride * (int)v7;
  HeightMap = this->HeightMap;
  v12 = Stride + v10;
  if ( v8 + v9 >= 1.0 )
    return (float)((float)(HeightMap[v12] - HeightMap[v12 + 1]) * (1.0 - v9)
                 + HeightMap[v12 + 1]
                 + (float)(HeightMap[v10 + 1] - HeightMap[v12 + 1]) * (1.0 - v8));
  else
    return (float)((float)(HeightMap[v10 + 1] - HeightMap[v10]) * v9
                 + HeightMap[v10]
                 + (float)(HeightMap[v12] - HeightMap[v10]) * v8);
}

//----- (0045E230) --------------------------------------------------------

D3DXVECTOR3 *STerrain::GetNormal(D3DXVECTOR3 *result, int x, int z)

{
  int v5;
  float *HeightMap; // eax
  int v7;
  int v8;
  float v9; // xmm0_4
  float v10; // xmm0_4
  float *v11; // ecx
  float v12; // xmm0_4
  float v13; // xmm0_4
  v5 = z * this->Stride;
  result->y = 1.0;
  HeightMap = this->HeightMap;
  v7 = x + v5;
  v8 = v7;
  if ( x )
  {
    v10 = HeightMap[v8 - 1];
    if ( x == this->XSize )
      v9 = v10 - HeightMap[v7];
    else
      v9 = (float)(v10 - HeightMap[v8 + 1]) * 0.5f;
  }
  else
  {
    v9 = HeightMap[v8] - HeightMap[v8 + 1];
  }
  result->x = v9;
  v11 = this->HeightMap;
  if ( z )
  {
    v13 = v11[v7 - this->Stride];
    if ( z == this->ZSize )
      v12 = v13 - v11[v7];
    else
      v12 = (float)(v13 - v11[v7 + this->Stride]) * 0.5f;
  }
  else
  {
    v12 = v11[v7] - v11[v7 + this->Stride];
  }
  result->z = v12;
  D3DXVec3Normalize(result, result);
  return result;
}

//----- (0045E2F0) --------------------------------------------------------

double STerrain::GetOldHeight(int x, int z)

{
  if ( x < 0 || x >= this->XSize || z < 0 || z >= this->ZSize )
    return 0.0;
  else
    return this->OldHeightMap[x + z * this->Stride];
}

//----- (0045E330) --------------------------------------------------------

double STerrain::GetOldHeight(float x, float z)

{
  int XSize;
  double v5; // st7
  float *OldHeightMap; // ecx
  float v7; // xmm1_4
  float v8; // xmm0_4
  int v9;
  double v11;
  if ( x < 0.0 )
    return 0.0;
  XSize = this->XSize;
  if ( x >= (float)XSize || z < 0.0 || z >= (float)this->ZSize )
    return 0.0;
  v11 = floor(z);
  v5 = floor(x);
  OldHeightMap = this->OldHeightMap;
  v7 = (float)(v11);

  v8 = (float)(v5);

  v9 = (int)v8 + (int)v7 * (XSize + 1);
  return (float)((float)((float)((float)((float)(OldHeightMap[v9 + 1] * (float)(1.0 - (float)(z - v7))) * (float)(x - v8))
                               + (float)((float)((float)(1.0 - (float)(z - v7)) * OldHeightMap[v9])
                                       * (float)(1.0 - (float)(x - v8))))
                       + (float)((float)(OldHeightMap[XSize + 1 + v9] * (float)(z - v7)) * (float)(1.0 - (float)(x - v8))))
               + (float)((float)(OldHeightMap[XSize + 2 + v9] * (float)(z - v7)) * (float)(x - v8)));
}

//----- (0045E460) --------------------------------------------------------

void STerrain::InitLimits()

{
  int v2;
  int v3;
  SMesh *v4; // eax
  SMesh *v5; // eax
  SGepard *Gepard; // ecx
  int v7;
  SMesh *LimitMesh; // ecx
  int v9;
  int v10;
  int v11;
  int v12;
  unsigned int v13;
  int v14; // xmm5
  int v15; // xmm7
  int v16;
  float v19; // xmm4_4
  int v20;
  float v21; // xmm0_4
  float v22; // xmm3_4
  int v23;
  float v24; // xmm0_4
  int v25;
  int v26;
  int v27;
  unsigned int v28;
  int v29; // xmm5
  int v30; // xmm7
  int v31;
  float v34; // xmm4_4
  int v35;
  float v36; // xmm0_4
  float v37; // xmm3_4
  int v38;
  float v39; // xmm0_4
  SMesh *v40; // ecx
  int v41;
  int v42;
  int v43;
  int v44;
  unsigned short v45; // si
  int v46;
  short v47; // di
  unsigned short v48; // cx
  bool v49;
  int v50;
  int v51;
  int v52;
  unsigned short v53; // si
  unsigned int v54; // kr00_4
  int v55;
  short v56; // di
  unsigned short v57; // cx
  void *block;
  void *blocka;
  int v60;
  unsigned int v61;
  unsigned int v62;
  int v63;
  int v64;
  int v65;
  int v66;
  int v67;
  int v68;
  int v69;
  signed int v70;
  int v71;
  int v72;
  int v73;
  int v74;
  v2 = this->XSize - 48;
  v3 = this->ZSize - 48;
  v70 = v2;
  v68 = v3;
  v4 = (SMesh *)operator new(sizeof(SMesh));
  if ( v4 )
  {
    new (v4) SMesh(this->Gepard, 1u);
    v5 = v4;
  }
  else
    v5 = 0;
  Gepard = this->Gepard;
  this->LimitMesh = v5;
  v7 = Gepard->LoadTexture("fx/levellimits_a.tga", 1, 1);
  LimitMesh = this->LimitMesh;
  this->LimitTexture = v7;
  LimitMesh->CreateVertexBuffer(6 * (v3 + v2 - 94));
  v9 = 0;
  v10 = 0;
  v11 = v70;
  v72 = 0;
  do
  {
    v12 = 48;
    v64 = 48;
    if ( v11 >= 48 )
    {
      v13 = v3;
      if ( !v10 )
        v13 = 48;
      v14 = (v13 - 1);
      v15 = v13;
      v16 = v13 - 1 + 2;
      float fv14 = (float)v14;
      float fv15 = (float)v15;
      do
      {
        v19 = (float)v12;
        *(float *)((char *)this->LimitMesh->lpVertices + this->LimitMesh->OffsetXYZ + v9 * this->LimitMesh->VertexSize) = v19;
        *(float *)((char *)this->LimitMesh->lpVertices + this->LimitMesh->OffsetXYZ + v9 * this->LimitMesh->VertexSize + 4) = 0.0f;
        *(float *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetXYZ
                  + v9 * this->LimitMesh->VertexSize
                  + 8) = fv14;
        v20 = -v64;
        if ( v72 )
          v20 = v64;
        v21 = (float)v20 * 4.0f;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetTexture1
                 + v9 * this->LimitMesh->VertexSize) = v21;
        if ( v72 )
          v22 = FLOAT_4_5;
        else
          v22 = FLOAT_N3_5;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetTexture1
                 + v9 * this->LimitMesh->VertexSize
                 + 4) = v22;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetXYZ
                 + (v9 + 1) * this->LimitMesh->VertexSize) = v19;
        *(_DWORD *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetXYZ
                  + (v9 + 1) * this->LimitMesh->VertexSize
                  + 4) = 0;
        *(float *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetXYZ
                  + (v9 + 1) * this->LimitMesh->VertexSize
                  + 8) = fv15;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetTexture1
                 + (v9 + 1) * this->LimitMesh->VertexSize) = v21;
        v23 = v9 + 2;
        *(_DWORD *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetTexture1
                  + (v9 + 1) * this->LimitMesh->VertexSize
                  + 4) = 1056964608;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetXYZ
                 + (v9 + 2) * this->LimitMesh->VertexSize) = v19;
        *(_DWORD *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetXYZ
                  + (v9 + 2) * this->LimitMesh->VertexSize
                  + 4) = 0;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetXYZ
                 + (v9 + 2) * this->LimitMesh->VertexSize
                 + 8) = (float)v16;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetTexture1
                 + (v9 + 2) * this->LimitMesh->VertexSize) = v21;
        if ( v72 )
          v24 = FLOAT_N3_5;
        else
          v24 = FLOAT_4_5;
        v9 += 3;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetTexture1
                 + v23 * this->LimitMesh->VertexSize
                 + 4) = v24;
        v12 = v64 + 1;
        v64 = v12;
      }
      while ( v12 <= v70 );
      v10 = v72;
      v3 = v68;
      v11 = v70;
    }
    v72 = ++v10;
  }
  while ( v10 < 2 );
  v25 = v68;
  v26 = 0;
  v73 = 0;
  do
  {
    v27 = 48;
    v65 = 48;
    if ( v25 >= 48 )
    {
      v28 = v70;
      if ( !v26 )
        v28 = 48;
      v29 = (v28 - 1);
      v30 = v28;
      v31 = v28 - 1 + 2;
      float fv29 = (float)v29;
      float fv30 = (float)v30;
      do
      {
        v34 = (float)v27;
        *(float *)((char *)this->LimitMesh->lpVertices + this->LimitMesh->OffsetXYZ + v9 * this->LimitMesh->VertexSize) = fv29;
        *(_DWORD *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetXYZ
                  + v9 * this->LimitMesh->VertexSize
                  + 4) = 0;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetXYZ
                 + v9 * this->LimitMesh->VertexSize
                 + 8) = (float)v27;
        v35 = v65;
        if ( v73 )
          v35 = -v65;
        v36 = (float)v35 * 4.0f;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetTexture1
                 + v9 * this->LimitMesh->VertexSize) = v36;
        if ( v73 )
          v37 = FLOAT_N3_5;
        else
          v37 = FLOAT_4_5;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetTexture1
                 + v9 * this->LimitMesh->VertexSize
                 + 4) = v37;
        *(float *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetXYZ
                  + (v9 + 1) * this->LimitMesh->VertexSize) = fv30;
        *(_DWORD *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetXYZ
                  + (v9 + 1) * this->LimitMesh->VertexSize
                  + 4) = 0;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetXYZ
                 + (v9 + 1) * this->LimitMesh->VertexSize
                 + 8) = v34;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetTexture1
                 + (v9 + 1) * this->LimitMesh->VertexSize) = v36;
        v38 = v9 + 2;
        *(_DWORD *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetTexture1
                  + (v9 + 1) * this->LimitMesh->VertexSize
                  + 4) = 1056964608;
        *(float *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetXYZ
                  + (v9 + 2) * this->LimitMesh->VertexSize) = (float)v31;
        *(float *)((char *)this->LimitMesh->lpVertices
                  + this->LimitMesh->OffsetXYZ
                  + (v9 + 2) * this->LimitMesh->VertexSize + 4) = 0.0f;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetXYZ
                 + (v9 + 2) * this->LimitMesh->VertexSize
                 + 8) = v34;
        *(float *)((char *)this->LimitMesh->lpVertices
                 + this->LimitMesh->OffsetTexture1
                 + (v9 + 2) * this->LimitMesh->VertexSize) = v36;
        if ( v73 )
          v39 = FLOAT_4_5;
        else
          v39 = FLOAT_N3_5;
        v40 = this->LimitMesh;
        v9 += 3;
        v41 = v38 * v40->VertexSize;
        v27 = ++v65;
        *(float *)((char *)v40->lpVertices + v40->OffsetTexture1 + v41 + 4) = v39;
      }
      while ( v65 <= v68 );
      v26 = v73;
      v25 = v68;
    }
    v73 = ++v26;
  }
  while ( v26 < 2 );
  this->LimitMesh->CreateIndexBuffer(0, 24 * (v70 + v25 - 96));
  v42 = 0;
  v74 = 0;
  v43 = 3 * (v70 - 48);
  v71 = 0;
  v44 = 2;
  block = (void *)v43;
  v60 = 2;
  do
  {
    if ( v43 > 0 )
    {
      v45 = v42;
      v66 = v42;
      v46 = v74;
      v62 = (v43 - 1) / 3u + 1;
      do
      {
        v47 = v45 + 1;
        this->LimitMesh->lpIndices[0][v46] = v45 + 1;
        this->LimitMesh->lpIndices[0][v46 + 1] = v45;
        this->LimitMesh->lpIndices[0][v46 + 2] = v45 + 3;
        this->LimitMesh->lpIndices[0][v46 + 3] = v45 + 1;
        this->LimitMesh->lpIndices[0][v46 + 4] = v45 + 3;
        this->LimitMesh->lpIndices[0][v46 + 5] = v45 + 4;
        v48 = v66 + 4;
        this->LimitMesh->lpIndices[0][v74 + 6] = v66 + 2;
        this->LimitMesh->lpIndices[0][v74 + 7] = v45 + 1;
        this->LimitMesh->lpIndices[0][v74 + 8] = v66 + 4;
        this->LimitMesh->lpIndices[0][v74 + 9] = v66 + 2;
        v45 = v66 + 3;
        v66 += 3;
        this->LimitMesh->lpIndices[0][v74 + 10] = v48;
        this->LimitMesh->lpIndices[0][v74 + 11] = v47 + 4;
        v46 = v74 + 12;
        v49 = v62-- == 1;
        v74 += 12;
      }
      while ( !v49 );
      v42 = v71;
      v43 = (int)block;
      v44 = v60;
    }
    v42 += v43 + 3;
    --v44;
    v71 = v42;
    v60 = v44;
  }
  while ( v44 );
  v50 = 3 * (v68 - 48);
  v63 = 3 * v68 - 141;
  v51 = 2;
  v52 = v63;
  blocka = (void *)v50;
  v69 = 2;
  do
  {
    if ( v50 > 0 )
    {
      v53 = v42;
      v67 = v42;
      v54 = v50 - 1;
      v55 = v74;
      v61 = v54 / 3 + 1;
      do
      {
        v56 = v53 + 1;
        this->LimitMesh->lpIndices[0][v55] = v53 + 3;
        this->LimitMesh->lpIndices[0][v55 + 1] = v53;
        this->LimitMesh->lpIndices[0][v55 + 2] = v53 + 1;
        this->LimitMesh->lpIndices[0][v55 + 3] = v53 + 3;
        this->LimitMesh->lpIndices[0][v55 + 4] = v53 + 1;
        this->LimitMesh->lpIndices[0][v55 + 5] = v53 + 4;
        v57 = v67 + 2;
        this->LimitMesh->lpIndices[0][v74 + 6] = v67 + 4;
        this->LimitMesh->lpIndices[0][v74 + 7] = v53 + 1;
        this->LimitMesh->lpIndices[0][v74 + 8] = v67 + 2;
        this->LimitMesh->lpIndices[0][v74 + 9] = v67 + 4;
        v53 = v67 + 3;
        v67 += 3;
        this->LimitMesh->lpIndices[0][v74 + 10] = v57;
        this->LimitMesh->lpIndices[0][v74 + 11] = v56 + 4;
        v55 = v74 + 12;
        v49 = v61-- == 1;
        v74 += 12;
      }
      while ( !v49 );
      v42 = v71;
      v51 = v69;
      v50 = (int)blocka;
      v52 = v63;
    }
    v42 += v52;
    --v51;
    v71 = v42;
    v69 = v51;
  }
  while ( v51 );
}

//----- (0045EC00) --------------------------------------------------------

void STerrain::InitTileVariations(const char *tileset)

{
  char *buf; // eax
  int v4;
  char *v5; // eax
  size_t v6;
  char *v7; // eax
  char *v8; // ecx
  char _props_buf[sizeof(SProperties)];
  SProperties &props = *(SProperties *)_props_buf;
  int v10;
  new (&props) SProperties("layers.ini", 1, 1);
  v10 = 0;
  this->Variations[0] = props.GetInt(tileset, "Layer1", 0);
  this->Variations[1] = props.GetInt(tileset, "Layer2", 0);
  this->Variations[2] = props.GetInt(tileset, "Layer3", 0);
  this->Variations[3] = props.GetInt(tileset, "Layer4", 0);
  this->Variations[4] = props.GetInt(tileset, "Layer5", 0);
  this->Variations[5] = props.GetInt(tileset, "Layer6", 0);
  this->Variations[6] = props.GetInt(tileset, "Layer7", 0);
  this->Variations[7] = props.GetInt(tileset, "Layer8", 0);
  this->Variations[8] = props.GetInt(tileset, "Layer9", 0);
  this->Variations[9] = props.GetInt(tileset, "Layer10", 0);
  buf = this->TilePrefix.buf;
  if ( buf )
  {
    delete[] buf;
    this->TilePrefix.buf = 0;
  }
  if ( tileset )
  {
    v4 = strlen(tileset);
    this->TilePrefix.size = v4;
    v5 = new char[v4 + 1];
    v6 = this->TilePrefix.size + 1;
    this->TilePrefix.buf = v5;
    memcpy(v5, tileset, v6);
  }
  else
  {
    this->TilePrefix.size = 0;
    this->TilePrefix.buf = 0;
  }
  v7 = (char *)realloc(this->TilePrefix.buf, this->TilePrefix.size + 2);
  v8 = &v7[this->TilePrefix.size];
  this->TilePrefix.buf = v7;
  strcpy(v8, "\\");
  ++this->TilePrefix.size;
  props.~SProperties();
}

//----- (0045EDE0) --------------------------------------------------------

void STerrain::InitTileset2()

{
  // The IDA decomp built a per-layer scratch buffer holding `{ int count;
  // SDArray<SBitmap*> arrays[count]; }` and walked it via raw byte-offset
  // pointer arithmetic with hardcoded 0xC (== sizeof(SDArray) on x86).
  // SDArray<T> is 16 bytes on x64 (4 size + 4 maxsize + 8-byte ptr), so
  // every stride and field offset is wrong on x64 — the very first
  // *(v7-2) damage check fires because the `array` pointer slot of one
  // SDArray ended up overlapping the `size` slot of the next.
  //
  // Rewrite using a typed SDArray<SBitmap*> array. Behavior on x86 is
  // unchanged; x64 finally walks the right strides.
  for ( int layer = 0; layer < 10; ++layer )
  {
    int variations = this->Variations[layer];
    if ( variations <= 0 )
    {
      this->TileTextures[layer] = -1;
      continue;
    }
    SDArray<SBitmap *> *bitmapArrays = new SDArray<SBitmap *>[variations]();
    char buf[260];
    for ( int v = 0; v < variations; ++v )
    {
      const char *prefix = this->TilePrefix.buf ? this->TilePrefix.buf : fullpath;
      sprintf(buf, "%sL%02d_%d.dxt", prefix, layer + 1, v + 1);
      SDArray<SBitmap *> bitmaps;
      memset(&bitmaps, 0, sizeof(bitmaps));
      this->Gepard->LoadDXTToLevelBitmaps(buf, &bitmaps);
      // Damage check matches original semantics (size > 0 but null array).
      if ( bitmapArrays[v].size && !bitmapArrays[v].array )
        Logger.g->Panic("SDArray::Clear: array is damaged");
      int newSize = bitmaps.size;
      int oldMax = bitmapArrays[v].maxsize;
      bitmapArrays[v].size = newSize;
      if ( newSize > oldMax )
      {
        bitmapArrays[v].array = (SBitmap **)realloc(bitmapArrays[v].array, sizeof(SBitmap *) * newSize);
        bitmapArrays[v].maxsize = newSize;
        oldMax = newSize;
      }
      memset(bitmapArrays[v].array, 0, sizeof(SBitmap *) * oldMax);
      for ( int i = 0; i < newSize; ++i )
        bitmapArrays[v].array[i] = bitmaps.array[i];
      if ( bitmaps.array )
      {
        free(bitmaps.array);
        bitmaps.array = nullptr;
      }
    }
    SDArray<SBitmap *> atlasBitmaps;
    memset(&atlasBitmaps, 0, sizeof(atlasBitmaps));
    int numAtlasMips = bitmapArrays[0].size - 2;
    for ( int mip = 0; mip < numAtlasMips; ++mip )
    {
      SBitmap *first = bitmapArrays[0].array[mip];
      int width = first->Width;
      int height = first->Height;
      SBitmap *atlas = (SBitmap *)operator new(sizeof(SBitmap));
      new (atlas) SBitmap(2 * width, 2 * height, first->Format, 0);
      // 4 quadrants: variation index = q % variations.
      // Position is the standard 2x2 (TL, TR, BR, BL) for 2- and 4-variation
      // tilesets; for 3-variation, q=2 and q=3 swap so var 2 lands in BL and
      // var 0 (q=3 % 3) wraps to BR.
      for ( int q = 0; q < 4; ++q )
      {
        int srcVar = q % variations;
        int dstX, dstY;
        switch ( q )
        {
          case 0: dstX = 0;     dstY = 0;      break;
          case 1: dstX = width; dstY = 0;      break;
          case 2: dstY = height; dstX = (variations == 3) ? 0 : width; break;
          case 3: dstY = height; dstX = (variations != 3) ? 0 : width; break;
        }
        SBitmap *src = bitmapArrays[srcVar].array[mip];
        atlas->BitBlt(dstX, dstY, width, height, src, 0, 0);
      }
      // Append atlas to atlasBitmaps (inline SDArray::Add so we keep the
      // original 6/5 growth ratio).
      int idx = atlasBitmaps.size;
      if ( idx == atlasBitmaps.maxsize )
      {
        int newMax = (atlasBitmaps.maxsize >= 16) ? (6 * atlasBitmaps.maxsize / 5) : 16;
        atlasBitmaps.array = (SBitmap **)realloc(atlasBitmaps.array, sizeof(SBitmap *) * newMax);
        memset(&atlasBitmaps.array[atlasBitmaps.maxsize], 0, sizeof(SBitmap *) * (newMax - atlasBitmaps.maxsize));
        atlasBitmaps.maxsize = newMax;
      }
      atlasBitmaps.size = idx + 1;
      atlasBitmaps.array[idx] = atlas;
    }
    this->TileTextures[layer] = this->Gepard->CreateTextureFromLevelBitmaps(&atlasBitmaps, "TerrainLayer");
    for ( int i = 0; i < atlasBitmaps.size; ++i )
    {
      SBitmap *bm = atlasBitmaps.array[i];
      if ( bm )
      {
        bm->~SBitmap();
        ::operator delete(bm);
      }
    }
    for ( int v = 0; v < variations; ++v )
    {
      for ( int m = 0; m < bitmapArrays[v].size; ++m )
      {
        SBitmap *bm = bitmapArrays[v].array[m];
        if ( bm )
        {
          bm->~SBitmap();
          ::operator delete(bm);
        }
      }
      if ( bitmapArrays[v].array )
        free(bitmapArrays[v].array);
    }
    delete[] bitmapArrays;
    if ( atlasBitmaps.array )
    {
      free(atlasBitmaps.array);
      atlasBitmaps.array = nullptr;
    }
  }
}

//----- (0045F330) --------------------------------------------------------

void STerrain::LoadSketchTexture(const char *filename)

{
  int v3;
  v3 = this->Gepard->LoadTexture(filename, 1, 0);
  this->Gepard->ReleaseTexture(this->SketchTexture, 0);
  this->SketchTexture = v3;
}

//----- (0045F370) --------------------------------------------------------

void STerrain::RaiseHeight(int x, int z, float height)

{
  float *HeightMap; // edi
  int v5;
  if ( x >= 0 && x < this->XSize && z >= 0 && z < this->ZSize )
  {
    HeightMap = this->HeightMap;
    v5 = x + z * this->Stride;
    if ( height > HeightMap[v5] )
    {
      HeightMap[v5] = height;
      this->WaterMap[z * this->Stride + x] = 1;
    }
  }
}

//----- (0045F3C0) --------------------------------------------------------

void STerrain::ReleaseTileset()

{
  int v2;
  int *TileTextures; // esi
  v2 = 10;
  TileTextures = this->TileTextures;
  do
  {
    this->Gepard->ReleaseTexture(*TileTextures++, 0);
    --v2;
  }
  while ( v2 );
}

//----- (0045F3F0) --------------------------------------------------------

void STerrain::SetBlockMapAddress(unsigned char *blockmap, unsigned char *currentblockmap)

{
  this->BlockMap = blockmap;
  this->CurrentBlockMap = currentblockmap;
}

//----- (0045F410) --------------------------------------------------------

void STerrain::SetBlockMapMode(bool enable)

{
  STerrain *v2; // ebx
  bool v3; // al
  int v4;
  int v5;
  int XParcels;
  int v7;
  int v8;
  int v9;
  SMesh *v10; // eax
  SMesh *v11; // eax
  SMesh *v12; // esi
  int v13;
  int v14;
  int v15;
  unsigned short v16; // cx
  unsigned short v17; // dx
  int v18;
  int v19;
  float v20; // xmm4_4
  float v21; // xmm0_4
  float v22; // xmm1_4
  int v23;
  float v24; // xmm2_4
  float v25; // xmm1_4
  float v26; // xmm0_4
  float *HeightMap; // ecx
  float v28; // xmm1_4
  int v29;
  float v30;
  float v31;
  int v32;
  int v33;
  float v34; // xmm3_4
  float v35;
  float v36;
  float v37;
  float v38; // xmm0_4
  float v39; // xmm3_4
  float v40;
  int v41;
  int v42;
  SMesh *BlockMapMesh; // ecx
  double v44;
  int v46;
  int v47;
  int v48;
  int v49;
  float v50;
  int v51;
  int v52;
  int v53;
  int enablea;
  float enableb;
  v2 = this;
  v3 = enable && this->BlockMap && this->CurrentBlockMap;
  v4 = 0;
  this->BlockMapMode = v3;
  if ( v3 )
  {
    v52 = 0;
    if ( this->ZParcels > 0 )
    {
      v5 = 0;
      v51 = 0;
      do
      {
        XParcels = v2->XParcels;
        v7 = 0;
        v46 = 0;
        if ( XParcels > 0 )
        {
          v8 = 0;
          v53 = 0;
          do
          {
            v9 = v7 + v4 * XParcels;
            if ( !v2->Parcels[v9].BlockMapMesh )
            {
              v10 = (SMesh *)operator new(sizeof(SMesh));
              if ( v10 )
              {
                v11 = new (v10) SMesh(v2->Gepard, 0x40u);
                v12 = v11;
              }
              else
              {
                v12 = 0;
              }
              v2->Parcels[v9].BlockMapMesh = v12;
              v12->CreateIndexBuffer(0, 3456);
              v12->CreateVertexBuffer(0x271u);
              v13 = 0;
              v14 = 0;
              v49 = 0;
              do
              {
                v15 = v14;
                enablea = 24;
                do
                {
                  v16 = v15++;
                  v17 = v16 + 25;
                  v12->lpIndices[0][v13] = v16 + 25;
                  v12->lpIndices[0][v13 + 1] = v16++;
                  v12->lpIndices[0][v13 + 2] = v16;
                  v12->lpIndices[0][v13 + 3] = v17;
                  v12->lpIndices[0][v13 + 4] = v16;
                  v12->lpIndices[0][v13 + 5] = v17 + 1;
                  v13 += 6;
                  --enablea;
                }
                while ( enablea );
                v14 = v49 + 25;
                v49 = v14;
              }
              while ( v14 < 600 );
              v18 = 0;
              v19 = 0;
              v20 = FLOAT_0_33333334;
              v2 = this;
              v21 = (float)v53;
              v47 = 0;
              v22 = (float)v51;
              do
              {
                v23 = 0;
                v48 = 0;
                v24 = (float)((float)v19 * v20) + v22;
                enableb = v24;
                do
                {
                  v25 = (float)((float)v23 * v20) + v21;
                  *(float *)((char *)v12->lpVertices + v18 * v12->VertexSize + v12->OffsetXYZ) = v25;
                  v50 = v25;
                  *(float *)((char *)v12->lpVertices + v18 * v12->VertexSize + v12->OffsetXYZ + 8) = v24;
                  if ( v25 < 0.0 || v25 >= (float)this->XSize || v24 < 0.0 || v24 >= (float)this->ZSize )
                  {
                    v38 = 0.0;
                  }
                  else
                  {
                    v44 = floor(v24);
                    v26 = (float)(floor(v25));

                    HeightMap = this->HeightMap;
                    v28 = (float)(v44);

                    v29 = (int)v28;
                    v30 = (float)(enableb - v28);
                    v31 = (float)(v50 - v26);
                    v32 = (int)v26 + this->Stride * v29;
                    v33 = v32 + this->Stride;
                    if ( v30 + v31 >= 1.0 )
                    {
                      v39 = HeightMap[v33 + 1];
                      v35 = (float)(HeightMap[v32 + 1] - v39) * (1.0f - v30);
                      v40 = 1.0f - v31;
                      v20 = FLOAT_0_33333334;
                      v37 = (float)(HeightMap[v33] - v39) * v40;
                      v36 = v39;
                    }
                    else
                    {
                      v34 = HeightMap[v32];
                      v35 = (float)(HeightMap[v33] - v34) * v30;
                      v36 = v34;
                      v37 = (float)(HeightMap[v32 + 1] - v34) * v31;
                      v20 = FLOAT_0_33333334;
                    }
                    v38 = v35 + v37 + v36;
                    v24 = enableb;
                  }
                  *(float *)((char *)v12->lpVertices + v18 * v12->VertexSize + v12->OffsetXYZ + 4) = v38 + 0.0049999999f;
                  v21 = (float)v53;
                  v41 = v18 * v12->VertexSize;
                  ++v18;
                  *(_DWORD *)((char *)v12->lpVertices + v41 + v12->OffsetDiffuse) = 0;
                  v23 = v48 + 1;
                  v48 = v23;
                }
                while ( v23 < 25 );
                v22 = (float)v51;
                v19 = v47 + 1;
                v47 = v19;
              }
              while ( v19 < 25 );
              v12->UnlockIndexBuffer(0);
              v12->UnlockVertexBuffer();
              XParcels = this->XParcels;
              v7 = v46;
              v8 = v53;
            }
            v4 = v52;
            ++v7;
            v8 += 8;
            v46 = v7;
            v53 = v8;
          }
          while ( v7 < XParcels );
          v5 = v51;
        }
        ++v4;
        v5 += 8;
        v52 = v4;
        v51 = v5;
      }
      while ( v4 < v2->ZParcels );
    }
  }
  else if ( this->NumParcels > 0 )
  {
    v42 = 0;
    do
    {
      BlockMapMesh = v2->Parcels[v42].BlockMapMesh;
      if ( BlockMapMesh )
      {
        BlockMapMesh->Release();
        v2->Parcels[v42].BlockMapMesh = 0;
      }
      ++v4;
      ++v42;
    }
    while ( v4 < v2->NumParcels );
  }
}

//----- (0045F850) --------------------------------------------------------

void STerrain::SetCompactMode(bool enable)

{
  int v3;
  int XParcels;
  int i;
  int v6;
  SParcel2 *v8; // edx
  int v9;
  int v10;
  SParcel2 *Mesh2; // ecx
  SParcel2 *v12;
  int v13;
  int enablea;
  extern LoggerGlobal Logger;
  this->CompactMode = enable;
  if ( enable )
  {
    v3 = 0;
    for ( enablea = 0; v3 < this->ZParcels; enablea = v3 )
    {
      XParcels = this->XParcels;
      for ( i = 0; i < XParcels; ++i )
      {
        v6 = i + v3 * XParcels;
        v13 = i + v3 * this->Stride;
        if ( !this->Parcels[v6].Mesh2 )
        {
          v12 = (SParcel2 *)operator new(sizeof(SParcel2));
          if ( v12 )
          {
 new (v12) SParcel2(this->Gepard, 8 * i, 8 * enablea, &this->HeightMap[8 * v13], (unsigned char (*)[9])this->BlendMap[8 * v13],
              &this->ColorMap[8 * v13],
              &this->NormalMap[8 * v13],
              this->Stride,
              this->TileMap[v6].Level,
              this->TileTextures);
            v8 = v12;
          }
          else
          {
            v8 = 0;
          }
          this->Parcels[v6].Mesh2 = v8;
          XParcels = this->XParcels;
        }
        v3 = enablea;
      }
      ++v3;
    }
  }
  else
  {
    v9 = 0;
    if ( this->NumParcels > 0 )
    {
      v10 = 0;
      do
      {
        Mesh2 = this->Parcels[v10].Mesh2;
        if ( Mesh2 )
        {
          Mesh2->Release();
          this->Parcels[v10].Mesh2 = 0;
        }
        ++v9;
        ++v10;
      }
      while ( v9 < this->NumParcels );
    }
  }
}

//----- (0045F9D0) --------------------------------------------------------

void STerrain::SetViewType(int view_type)

{
  this->ViewType = view_type;
}

//----- (0045F9E0) --------------------------------------------------------

void STerrain::Update(int x0, int z0, int x1, int z1)

{
  int v6;
  int XSize;
  int v8;
  int v9;
  int ZSize;
  int v11;
  int v12;
  SMesh *LimitMesh; // ecx
  int v14;
  SMesh *v15; // eax
  int v16;
  float v17; // xmm1_4
  double v18; // st7
  float *HeightMap; // ecx
  float v20; // xmm1_4
  float v21; // xmm0_4
  int v22;
  float v23;
  float v24;
  float v25; // xmm0_4
  float v26;
  double v27;
  int v28;
  int x0a;
  int z0a;
  int x1a;
  // x64: x1b holds the lpVertices base pointer; was `(int)v15->lpVertices`
  // truncating an 8-byte vertex-buffer ptr. Use intptr_t so `(v16 + x1b +
  // const)` arithmetic preserves the full address.
  intptr_t x1b;
  int z1a;
  int z1b;
  if ( x0 <= 0 )
    v6 = 0;
  else
    v6 = (x0 - 1) / 8;
  XSize = this->XSize;
  v28 = v6;
  if ( x1 >= XSize - 1 )
    v8 = XSize / 8 - 1;
  else
    v8 = (x1 + 1) / 8;
  x1a = v8;
  if ( z0 <= 0 )
    v9 = 0;
  else
    v9 = (z0 - 1) / 8;
  ZSize = this->ZSize;
  x0a = v9;
  if ( z1 >= ZSize - 1 )
    v11 = ZSize / 8 - 1;
  else
    v11 = (z1 + 1) / 8;
  for ( z1a = v11; v9 <= v11; x0a = v9 )
  {
    if ( v28 <= v8 )
    {
      do
      {
        v12 = v6 + v9 * this->Stride;
        v9 = x0a;
 this->Parcels[v6 + x0a * this->XParcels].Mesh->Update(&this->HeightMap[8 * v12], (unsigned char (*)[9])this->BlendMap[8 * v12],
          &this->ColorMap[8 * v12],
          &this->NormalMap[8 * v12],
          this->Stride);
        v8 = x1a;
        ++v6;
      }
      while ( v6 <= x1a );
      v11 = z1a;
    }
    v6 = v28;
    ++v9;
  }
  if ( this->XSize > 96 && this->ZSize > 96 )
  {
    LimitMesh = this->LimitMesh;
    if ( !LimitMesh )
    {
      this->InitLimits();
      LimitMesh = this->LimitMesh;
    }
    LimitMesh->LockVertexBuffer();
    v14 = 0;
    z0a = this->LimitMesh->GetNumVertices();
    if ( z0a > 0 )
    {
      do
      {
        v15 = this->LimitMesh;
        v16 = v15->OffsetXYZ + v14 * v15->VertexSize;
        x1b = (intptr_t)v15->lpVertices;
        v17 = *(float *)(v16 + x1b);
        v26 = *(float *)(v16 + x1b + 8);
        *(float *)&z1b = v17;
        if ( v17 < 0.0 || v17 >= (float)this->XSize || v26 < 0.0 || v26 >= (float)this->ZSize )
        {
          v25 = 0.0;
        }
        else
        {
          v27 = floor(v26);
          v18 = floor(v17);
          HeightMap = this->HeightMap;
          v20 = (float)(v27);

          v21 = (float)(v18);

          v22 = (int)v21 + this->Stride * (int)v20;
          v23 = (float)(*(float *)&z1b - v21);
          v24 = (float)(v26 - v20);
          v25 = (float)(HeightMap[v22 + 1] * (1.0 - v24) * v23
              + HeightMap[v22] * (1.0 - v24) * (1.0 - v23)
              + HeightMap[v22 + 1 + this->XSize] * v24 * (1.0 - v23)
              + HeightMap[v22 + 2 + this->XSize] * v24 * v23);
        }
        ++v14;
        *(float *)(v16 + x1b + 4) = v25;
      }
      while ( v14 < z0a );
    }
    this->LimitMesh->UnlockVertexBuffer();
  }
}

//----- (0045FCB0) --------------------------------------------------------

void STerrain::UpdateBlockMap(int parcel)

{
  SMesh *BlockMapMesh; // esi
  STerrain *v3; // ecx
  int v4;
  int v5;
  int v6;
  int i;
  unsigned char v8; // dl
  int v9;
  int VertexSize;
  char *v11; // ecx
  int v12;
  _DWORD *v13; // eax
  bool v14;
  int v15;
  int v16;
  int v17;
  int OffsetDiffuse;
  int parcela;
  BlockMapMesh = this->Parcels[parcel].BlockMapMesh;
  BlockMapMesh->LockVertexBuffer();
  v3 = this;
  v4 = 0;
  v5 = parcel / this->XParcels;
  v16 = 8 * (parcel % this->XParcels);
  v6 = 0;
  v17 = v5;
  parcela = 0;
  do
  {
    for ( i = 0; i < 25; ++i )
    {
      if ( !v6 || i >= 24 )
      {
        *(_DWORD *)((char *)BlockMapMesh->lpVertices + BlockMapMesh->OffsetDiffuse + v4 * BlockMapMesh->VertexSize) = 0;
        goto LABEL_22;
      }
      v15 = 3 * (v16 + v3->XSize * (v6 + 24 * v5 - 1));
      v8 = this->BlockMap[v15 + i];
      if ( (v8 & 0xDC) != 0 )
      {
        *(_DWORD *)((char *)BlockMapMesh->lpVertices + BlockMapMesh->OffsetDiffuse + v4 * BlockMapMesh->VertexSize) = (((v4 & 1) == 0) + 3) << 21;
        v6 = parcela;
        goto LABEL_22;
      }
      if ( (v8 & 0x20) != 0 )
      {
        v9 = 8388736;
        if ( (v4 & 1) != 0 )
          v9 = 6291552;
        *(_DWORD *)((char *)BlockMapMesh->lpVertices + BlockMapMesh->OffsetDiffuse + v4 * BlockMapMesh->VertexSize) = v9;
        v6 = parcela;
      }
      else
      {
        VertexSize = BlockMapMesh->VertexSize;
        OffsetDiffuse = BlockMapMesh->OffsetDiffuse;
        if ( this->CurrentBlockMap[v15 + i] )
        {
          v11 = (char *)0xFF604000; // orange blocked
          v12 = 6308352;
        }
        else
        {
          if ( (v8 & 2) == 0 )
          {
            v13 = (_DWORD *)((char *)BlockMapMesh->lpVertices + v4 * VertexSize + OffsetDiffuse);
            v14 = (v8 & 1) == 0;
            v6 = parcela;
            if ( v14 )
              *v13 = 0;
            else
              *v13 = (v4 & 1 | 2) << 13;
            goto LABEL_22;
          }
          v11 = (char *)0x00404000; // dark green passable
          v12 = 6316032;
        }
        if ( (v4 & 1) != 0 )
          v11 = (char *)v12;
        *(_DWORD *)((char *)BlockMapMesh->lpVertices + v4 * VertexSize + OffsetDiffuse) = (_DWORD)v11;
        v6 = parcela;
      }
LABEL_22:
      v3 = this;
      v5 = v17;
      ++v4;
    }
    parcela = ++v6;
  }
  while ( v6 < 25 );
  BlockMapMesh->UnlockVertexBuffer();
}

//----- (0045FE40) --------------------------------------------------------

void STerrain::UpdateColorMap(int x0, int z0, int x1, int z1)

{
  int ZSize;
  int v7;
  int XSize;
  int v9;
  int v10;
  int v11;
  int v12;
  bool v13; // cc
  int v14;
  float v15; // xmm1_4
  int v16;
  int v17;
  int v18;
  int v19;
  float *HeightMap; // ecx
  int v21;
  float v22; // xmm0_4
  float v23; // xmm0_4
  float v24; // xmm0_4
  float v25; // xmm0_4
  float v26; // xmm2_4
  float v27; // xmm1_4
  float r; // xmm2_4
  float g; // xmm3_4
  float b; // xmm1_4
  unsigned int *v31; // edx
  int v32;
  int v33;
  int v34;
  int v35;
  int v36;
  int v37;
  int v38;
  int Stride;
  float v40; // xmm2_4
  float *v41; // ecx
  int v42;
  int v43;
  float v44; // xmm0_4
  float v45; // xmm2_4
  float v46; // xmm0_4
  float v47; // xmm1_4
  int v48;
  SGepard *Gepard;
  int v50;
  int v51;
  int v52;
  int v53;
  int v54;
  int v55;
  int v56;
  int v57;
  int v58;
  int v59;
  int v60;
  int v61;
  D3DXVECTOR3 nv;
  Gepard = this->Gepard;
  ZSize = this->ZSize;
  v7 = x0 - 1;
  if ( x0 <= 0 )
    v7 = x0;
  v52 = v7;
  XSize = this->XSize;
  v9 = x1 + 1;
  if ( x1 > XSize )
    v9 = x1;
  v58 = v9;
  v10 = z0 - 1;
  if ( z0 <= 0 )
    v10 = z0;
  v51 = v10;
  v11 = z1 + 1;
  if ( z1 > ZSize )
    v11 = z1;
  v12 = v51;
  v56 = v11;
  v13 = v51 < v11;
  v14 = v58;
  v60 = v51;
  if ( v13 )
  {
    v15 = 0.5f;
    v16 = v56;
    do
    {
      v17 = v12 * this->Stride;
      v18 = v52;
      v54 = v52;
      v19 = v52 + v17;
      if ( v52 < v14 )
      {
        v53 = v19;
        do
        {
          HeightMap = this->HeightMap;
          v21 = v19;
          nv.y = 1.0;
          if ( v18 )
          {
            v23 = HeightMap[v21 - 1];
            if ( v18 == this->XSize )
              v22 = v23 - HeightMap[v19];
            else
              v22 = (float)(v23 - HeightMap[v21 + 1]) * v15;
          }
          else
          {
            v22 = HeightMap[v19] - HeightMap[v19 + 1];
          }
          nv.x = v22;
          if ( v60 )
          {
            v25 = HeightMap[v19 - this->Stride];
            if ( v60 == this->ZSize )
              v24 = v25 - HeightMap[v19];
            else
              v24 = (float)(v25 - HeightMap[v19 + this->Stride]) * v15;
          }
          else
          {
            v24 = HeightMap[v19] - HeightMap[v19 + this->Stride];
          }
          nv.z = v24;
          D3DXVec3Normalize(&nv, &nv);
          this->NormalMap[v53] = nv;
          v26 = -((float)((float)(Gepard->SunDir.y * nv.y) + (float)(Gepard->SunDir.x * nv.x))
                         + (float)(Gepard->SunDir.z * nv.z));
          if ( v26 <= 0.0 )
          {
            r = Gepard->AmbientColorVal.r;
            g = Gepard->AmbientColorVal.g;
            b = Gepard->AmbientColorVal.b;
          }
          else
          {
            v27 = (float)((double)this->ShadowMap[v19] * 0.0078125f * v26);

            r = fminf(1.0, (float)(Gepard->SunColorVal.r * v27) + Gepard->AmbientColorVal.r);
            g = fminf(1.0, (float)(Gepard->SunColorVal.g * v27) + Gepard->AmbientColorVal.g);
            b = fminf(1.0, (float)(Gepard->SunColorVal.b * v27) + Gepard->AmbientColorVal.b);
          }
          if ( v53 < 0 || v19 >= (this->XSize + 1) * (this->ZSize + 1) )
            Logger.g->Panic("SPlane::UpdateColorMap: Invalid address");
          v31 = &this->ColorMap[v19];
          if ( this->Gepard->Flags[0] )
            v32 = 0xFFFFFF;
          else
            v32 = (int)(float)(b * 255.0) | (((int)(float)(g * 255.0) | ((int)(float)(r * 255.0) << 8)) << 8);
          ++v19;
          ++v53;
          v18 = v54 + 1;
          *v31 = v32;
          v14 = v58;
          v15 = 0.5f;
          v54 = v18;
        }
        while ( v18 < v58 );
        v16 = v56;
      }
      v12 = v60 + 1;
      v60 = v12;
    }
    while ( v12 < v16 );
    XSize = this->XSize;
    ZSize = this->ZSize;
  }
  v33 = v52 / 8;
  if ( v52 <= 0 )
    v33 = 0;
  v50 = v33;
  if ( v58 >= XSize )
  {
    v59 = XSize / 8 - 1;
    ZSize = this->ZSize;
  }
  else
  {
    v59 = v58 / 8;
  }
  v34 = v51 / 8;
  if ( v51 <= 0 )
    v34 = 0;
  v61 = v34;
  if ( v56 >= ZSize )
    v35 = ZSize / 8 - 1;
  else
    v35 = v56 / 8;
  v57 = v35;
  if ( v34 <= v35 )
  {
    v36 = v59;
    v37 = v57;
    do
    {
      v38 = v50;
      if ( v50 <= v36 )
      {
        do
        {
          Stride = this->Stride;
          v40 = FLOAT_100_0;
          v41 = this->HeightMap;
          v55 = 4 * Stride;
          v42 = 9;
          v43 = 32 * (v38 + v61 * Stride) + 28;
          do
          {
            v44 = fminf(*(float *)((char *)v41 + v43 - 28), v40);
            v45 = *(float *)((char *)v41 + v43 + 4);
            v46 = fminf(
                    *(float *)((char *)v41 + v43 - 4),
                    fminf(
                      *(float *)((char *)v41 + v43 - 8),
                      fminf(
                        *(float *)((char *)v41 + v43 - 12),
                        fminf(
                          *(float *)((char *)v41 + v43 - 16),
                          fminf(*(float *)((char *)v41 + v43 - 20), fminf(*(float *)((char *)v41 + v43 - 24), v44))))));
            v47 = *(float *)((char *)v41 + v43);
            v43 += v55;
            v40 = fminf(v45, fminf(v47, v46));
            --v42;
          }
          while ( v42 );
          v48 = v38 + v61 * this->XParcels;
          ++v38;
          this->Parcels[v48].LowPoint = v40;
          v36 = v59;
        }
        while ( v38 <= v59 );
        v34 = v61;
        v37 = v57;
      }
      v61 = ++v34;
    }
    while ( v34 <= v37 );
  }
}

//----- (00460290) --------------------------------------------------------

void STerrain::UpdateLimits()

{
  SMesh *LimitMesh; // ecx
  unsigned int NumVertices;
  signed int v4;
  SMesh *v5; // ecx
  char *lpVertices; // esi
  int v7;
  double Height; // st7
  signed int v9;
  signed int i;
  if ( this->XSize > 96 && this->ZSize > 96 )
  {
    LimitMesh = this->LimitMesh;
    if ( !LimitMesh )
    {
      this->InitLimits();
      LimitMesh = this->LimitMesh;
    }
    LimitMesh->LockVertexBuffer();
    NumVertices = this->LimitMesh->GetNumVertices();
    v4 = 0;
    v9 = NumVertices;
    for ( i = 0; v4 < v9; i = v4 )
    {
      v5 = this->LimitMesh;
      lpVertices = (char *)v5->lpVertices;
      v7 = v5->OffsetXYZ + v4 * v5->VertexSize;
      Height = this->GetHeight(*(float *)&lpVertices[v7], *(float *)&lpVertices[v7 + 8]);
      v4 = i + 1;
      *(float *)&lpVertices[v7 + 4] = (float)(Height);

    }
    this->LimitMesh->UnlockVertexBuffer();
  }
}

//----- (00460350) --------------------------------------------------------

void STerrain::UpdateVisMap(unsigned char *vismap, unsigned char *vismap2, int fogmode)

{
  this->VisMap = vismap;
  this->VisMap2 = vismap2;
  this->FogMode = fogmode;
}

