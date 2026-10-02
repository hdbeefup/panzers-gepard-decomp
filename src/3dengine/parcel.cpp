// 3dengine/parcel.cpp
// Terrain parcels (subdivisions)
// Decompiled from: gameSplit/sterrain.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <new>
#include <xmmintrin.h>

#include "parcel.h"
#include "gepard.h"
#include "terrain.h"
#include "core_common.h"
#include "logger.h"

static const double DOUBLE_0_125 = 0.125;
static const double DOUBLE_0_0001 = 0.0001;

// BSS data arrays used by parcel rendering
static float LayerAlpha[10];
static float LayerVisible[10];
static float Block[81][10];
static unsigned char VertexRemap[81];
static unsigned char HiddenSmallTile[64];
static unsigned char FirstTriangle[128];
static unsigned int FoggedColorMap[81];

// Classes: SParcel, SParcel2
// Function count: 11

//----- (0044D6D0) --------------------------------------------------------

SParcel2::SParcel2( SGepard *gepard, int xbig, int zbig, float *heightmap, unsigned char (*blendmap)[9],

        unsigned int *colormap,

        D3DXVECTOR3 *normalmap,

        int stride,

        int bottom_level,

        int *textures)

{
  float *v12; // ecx
  unsigned char *v13; // eax
  int v14;
  int v15;
  unsigned char *v16; // edx
  int i;
  int v18;
  float v19; // xmm2_4
  float v20; // xmm3_4
  int v21;
  float v26;
  int v27;
  int v28;
  float v29;
  int v30;
  float v35; // xmm1_4
  bool v37;
  SGepard *j; // eax
  SShaderInfo *current; // edi
  int v40;
  int xpos;
  int v42;
  int v43;
  int v44;
  int v45;
  int v46;
  unsigned char *v47; // eax
  int v48;
  SShaderInfo *v49; // eax
  SShaderInfo *next; // eax
  int v51;
  int v52;
  unsigned char *v53; // ecx
  int v54;
  int v55;
  int v56;
  float v57;
  unsigned char v58; // al
  int v59;
  unsigned char v60; // al
  signed int v61;
  int v62;
  char *v63; // ecx
  char v64; // cl
  int v65;
  unsigned char v66; // al
  int v67;
  int v68;
  signed int v69;
  int v70;
  char *v71; // ecx
  int v72;
  unsigned char v73; // al
  int v74;
  signed int v75;
  int v76;
  char *v77; // ecx
  int v78;
  unsigned char v79; // al
  int v80;
  unsigned char v81; // al
  signed int v82;
  int v83;
  char *v84; // ecx
  int v85;
  unsigned char v86; // al
  int v87;
  int v88;
  signed int v89;
  int v90;
  char *v91; // ecx
  int v92;
  unsigned char v93; // al
  int v94;
  signed int v95;
  int v96;
  char *v97; // ecx
  int v98;
  int v99;
  int v100;
  int v101;
  SGepard *n; // eax
  int v103;
  int v104;
  int v105;
  SGepard *v106; // esi
  unsigned char *v107; // eax
  int v108;
  int v109;
  int v111;
  SGepard *v112; // eax
  int v113;
  signed int v114;
  unsigned char v115; // al
  int v116;
  signed int v117;
  int v118;
  char v119; // dl
  char v120; // al
  int v121;
  int v122;
  unsigned char v123; // dl
  signed int v124;
  int v125;
  signed int v126;
  char *v127; // eax
  SGepard *v128; // ecx
  int v129;
  char v130; // dl
  char v131; // al
  int v132;
  int v133;
  int v134;
  unsigned char v135; // cl
  signed int v136;
  int v137;
  signed int v138;
  char *v139; // eax
  int v140;
  char v141; // dl
  char v142; // al
  int v143;
  int v144;
  int v145;
  unsigned char v146; // cl
  signed int v147;
  int v148;
  signed int v149;
  int v150;
  char v151; // dl
  char v152; // al
  int v153;
  int v154;
  int v155;
  unsigned char v156; // cl
  signed int v157;
  int v158;
  signed int v159;
  char *v160; // eax
  int v161;
  char v162; // dl
  char v163; // al
  int v164;
  int v165;
  int v166;
  unsigned char v167; // cl
  signed int v168;
  int v169;
  signed int v170;
  int v171;
  char v172; // dl
  char v173; // al
  int v174;
  int v175;
  int v176;
  int v177;
  int v178;
  int v179;
  SShaderInfo *v180; // eax
  SShaderInfo *v181; // eax
  int v182;
  char *v183; // ecx
  int v184;
  float v185; // xmm0_4
  unsigned short v186; // dx
  unsigned short *IndexBuffer; // eax
  SMeshMaterial *MaterialBuffer; // eax
  void (__cdecl *v189)(void *); // eax
  int materials;
  int materials_4;
  char *materials_8;
  signed int vertices;
  char *vertices_8;
  int indices;
  int indices_4;
  char *indices_8;
  SGepard *v198;
  int v199;
  int v200;
  int v201;
  int v202;
  int v203;
  int v204;
  int v205;
  int v206;
  int v207;
  int v208;
  float *v209;
  int v210;
  int v211;
  int v212;
  int v213;
  int v214;
  int v215;
  int v216;
  int v217;
  int v218;
  int v219;
  int v220;
  int v221;
  int v222;
  unsigned char *v223;
  int v224;
  unsigned char *v225;
  int v226;
  int v227;
  int v228;
  int v229;
  int v230;
  int v231;
  int v232;
  int v233;
  int v234;
  int m;
  int v236;
  int v237;
  int v238;
  int v239;
  int v240;
  int v241;
  int v242;
  signed int v243;
  signed int v244;
  signed int v245;
  signed int v246;
  signed int v247;
  unsigned char *v248;
  char *v249;
  char *v250;
  char *v251;
  char v252;
  unsigned char v253;
  int v254;
  int k;
  signed int v256;
  signed int v257;
  char *v258;
  char *v259;
  char *v260;
  int v261;
  SGepard *geparda;
  int gepardb;
  SGepard *gepardc;
  SGepard *gepardd;
  SGepard *geparde;
  SGepard *gepardg;
  SGepard *gepardf;
  char gepard_3;
  unsigned char *blendmapa;
  unsigned char *blendmapb;
  unsigned char *blendmapc;
  char blendmap_3;
  int bottom_levela;
  int bottom_levelb;
  int bottom_levele;
  int bottom_levelf;
  int bottom_levelg;
  int bottom_levelh;
  SShaderInfo *si; // x64-aware typed SShaderInfo iterator (replaces int bottom_levelc / int v110)
  int bottom_leveld;
  char bottom_level_3;
  char bottom_level_3b;
  char bottom_level_3a;
  char bottom_level_3c;
  int *texturesa;
  extern LoggerGlobal Logger;
  new (static_cast<SMesh*>(this)) SMesh(gepard, 0x51u);
  v12 = Block[0];
  v13 = &(*blendmap)[-1];
  v14 = bottom_level;
  v225 = &(*blendmap)[-1];
  v15 = bottom_level - 1;
  do
  {
    v16 = v13;
    v209 = v12;
    blendmapa = v13;
    v222 = 9;
    do
    {
      for ( i = v15; i >= 0; --i )
      {
        LayerAlpha[i] = 0.0;
        LayerVisible[i] = 0.0;
      }
      v18 = v14 + 1;
      LayerAlpha[v14] = 1.0;
      LayerVisible[v14] = 1.0;
      if ( v14 + 1 < 10 )
      {
        geparda = (SGepard *)v14;
        do
        {
          v19 = (float)((double)v16[v18] * 0.00392156862745098f);

          v20 = v19;
          LayerVisible[v18] = v19;
          LayerAlpha[v18] = v19;
          if ( v19 > 0.9 )
          {
            v20 = 1.0f;
            v19 = 1.0f;
            LayerAlpha[v18] = 1.0f;
          }
          // Multiply LayerVisible[0..v14] by (1 - v20)
          // (replaced IDA SSE decompile that wrote out-of-bounds)
          if ( v14 >= 0 )
          {
            float factor = 1.0f - v20;
            for ( v21 = v14; v21 >= 0; --v21 )
              LayerVisible[v21] *= factor;
          }
          v16 = blendmapa;
          ++v14;
          ++v18;
          geparda = (SGepard *)v14;
        }
        while ( v14 < 9 );
      }
      v26 = (float)(DOUBLE_0_0001);

      v27 = 8;
      gepardb = 8;
      v28 = 0;
      do
      {
        v29 = LayerVisible[v28 + 9];
        if ( v26 <= v29 )
        {
          if ( v29 >= 0.1 )
            goto LABEL_30;
          // Divide LayerVisible[0..v27] by (1 - LayerAlpha[v28+9])
          // (replaced IDA SSE decompile that wrote out-of-bounds)
          if ( v27 >= 0 )
          {
            v35 = 1.0f - LayerAlpha[v28 + 9];
            for ( v30 = v27; v30 >= 0; --v30 )
              LayerVisible[v30] = LayerVisible[v30] / v35;
          }
          v26 = (float)(DOUBLE_0_0001);

        }
        LayerAlpha[v28 + 9] = 0.0;
LABEL_30:
        --v28;
        gepardb = v27 - 1;
      }
      while ( v27-- >= 0 );
      v14 = bottom_level;
      v16 = blendmapa + 9;
      // Copy 10 floats from LayerAlpha to Block row
      // (replaced IDA _OWORD/_QWORD copies that could be misaligned)
      memcpy(v209, LayerAlpha, 10 * sizeof(float));
      blendmapa += 9;
      v15 = bottom_level - 1;
      v12 = v209 + 10;
      v37 = v222-- == 1;
      v209 += 10;
    }
    while ( !v37 );
    v14 = bottom_level;
    v13 = &v225[9 * stride];
    v225 = v13;
    v15 = bottom_level - 1;
  }
  while ( v12 < &Block[81][0] );  // x64: end-of-Block compare; original IDA used &VertexRemap as sibling-static sentinel that's only adjacent to Block in x86 BSS layout
  memset(HiddenSmallTile, 0, sizeof(HiddenSmallTile));
  this->Gepard->ShaderInfos.current = this->Gepard->ShaderInfos.first;
  for ( j = this->Gepard; j->ShaderInfos.current; j = this->Gepard )
  {
    current = j->ShaderInfos.current;
    v40 = current->XVertices - 1;
    xpos = (int)current->xpos;
    v42 = (int)current->zpos - zbig;
    gepardc = (SGepard *)v42;
    v43 = current->ZVertices + v42 - 1;
    v44 = xpos - xbig;
    v45 = v44 + v40;
    bottom_levela = v45;
    if ( v44 < 8
      && v45 > 0
      && v42 < 8
      && v43 > 0
      && this->Gepard->IsTextureOpaque(this->Gepard->ShaderInfos.current->ShaderTHandle) )
    {
      v46 = bottom_levela;
      v47 = 0;
      if ( v44 >= 0 )
        v47 = (unsigned char *)v44;
      blendmapb = v47;
      if ( bottom_levela > 8 )
        v46 = 8;
      v48 = 0;
      bottom_levelb = v46;
      if ( (int)gepardc >= 0 )
        v48 = (int)gepardc;
      if ( v43 > 8 )
        v43 = 8;
      for ( gepardd = (SGepard *)v43; v48 < v43; ++v48 )
      {
        if ( (int)v47 < v46 )
        {
          memset(&HiddenSmallTile[8 * v48 + (_DWORD)v47], 1u, v46 - (_DWORD)v47);
          v47 = blendmapb;
          v46 = bottom_levelb;
          v43 = (int)gepardd;
        }
      }
    }
    v49 = this->Gepard->ShaderInfos.current;
    if ( this->Gepard->ShaderInfos.Closed )
    {
      if ( !v49 )
LABEL_330:
        Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
      next = v49->next;
      if ( next )
        this->Gepard->ShaderInfos.current = next;
      else
        this->Gepard->ShaderInfos.current = this->Gepard->ShaderInfos.first;
    }
    else if ( v49 )
    {
      this->Gepard->ShaderInfos.current = v49->next;
    }
    else
    {
      this->Gepard->ShaderInfos.current = 0;
    }
  }
  v51 = 0;
  vertices_8 = 0;
  vertices = 0;
  indices_8 = 0;
  indices = 0;
  indices_4 = 0;
  materials_8 = 0;
  materials = 0;
  materials_4 = 0;
  memset(FirstTriangle, 255, sizeof(FirstTriangle));
  for ( k = 0; k < 10; ++k )
  {
    for ( m = 0; m < 2; ++m )
    {
      memset(VertexRemap, 255, sizeof(VertexRemap));
      v52 = indices;
      v257 = vertices;
      v205 = indices;
      v252 = 0;
      v210 = 0;
      v223 = HiddenSmallTile;
      v248 = &FirstTriangle[1];
      do
      {
        v53 = v248;
        v54 = 0;
        v242 = 0;
        do
        {
          if ( !v223[v54] )
          {
            v55 = k;
            v56 = v54 + v210;
            v57 = (float)(DOUBLE_0_0001);

            v261 = v54 + v210;
            v203 = k + 10 * (v54 + v210);
            if ( Block[0][v203] >= 0.0001 || Block[v56 + 1][k] >= 0.0001 || Block[v56 + 9][k] >= 0.0001 )
            {
              v58 = *(v248 - 1);
              if ( v58 == 0xFF )
              {
                v58 = k;
                *(v248 - 1) = k;
              }
              v59 = indices_4;
              if ( (v58 == k) == (m == 0) )
              {
                v60 = VertexRemap[v56 + 9];
                if ( v60 == 0xFF )
                {
                  v61 = vertices;
                  if ( vertices == v51 )
                  {
                    if ( v51 >= 16 )
                      v62 = 6 * v51 / 5;
                    else
                      v62 = 16;
                    bottom_levele = v62;
                    vertices_8 = (char *)realloc(vertices_8, 5 * v62);
                    memset(&vertices_8[4 * v51 + v51], 0, 5 * (bottom_levele - v51));
                    v51 = bottom_levele;
                    v61 = vertices;
                  }
                  vertices = v61 + 1;
                  v63 = &vertices_8[4 * v61];
                  bottom_level_3 = v242;
                  v63[v61] = v242;
                  v63[v61 + 1] = v252 + 1;
                  v63[v61 + 3] = v242;
                  v63[v61 + 4] = v252 + 1;
                  v63[v61 + 2] = (int)((float)(Block[v261 + 9][k] * 255.0) + 0.5);
                  v64 = v257;
                  v60 = v61 - v257;
                  v59 = indices_4;
                  VertexRemap[v261 + 9] = v60;
                  gepard_3 = v252;
                  v56 = v261;
                }
                else
                {
                  bottom_level_3 = v242;
                  gepard_3 = v252;
                  v64 = v257;
                }
                blendmap_3 = v64;
                if ( indices == v59 )
                {
                  if ( v59 >= 16 )
                    v65 = 6 * v59 / 5;
                  else
                    v65 = 16;
                  v226 = v65;
                  indices_8 = (char *)realloc(indices_8, 2 * v65);
                  memset(&indices_8[2 * v59], 0, 2 * (v226 - v59));
                  v56 = v261;
                  indices_4 = v226;
                  v60 = VertexRemap[v261 + 9];
                }
                *(_WORD *)&indices_8[2 * indices] = v257 + v60;
                v66 = VertexRemap[v56];
                v67 = indices + 1;
                v68 = indices_4;
                v238 = indices + 1;
                if ( v66 == 0xFF )
                {
                  v69 = vertices;
                  if ( vertices == v51 )
                  {
                    if ( v51 >= 16 )
                      v70 = 6 * v51 / 5;
                    else
                      v70 = 16;
                    v227 = v70;
                    vertices_8 = (char *)realloc(vertices_8, 5 * v70);
                    memset(&vertices_8[4 * v51 + v51], 0, 5 * (v227 - v51));
                    v51 = v227;
                    v69 = vertices;
                  }
                  vertices = v69 + 1;
                  v71 = &vertices_8[4 * v69];
                  v71[v69] = bottom_level_3;
                  v71[v69 + 1] = v252;
                  v71[v69 + 3] = bottom_level_3;
                  v71[v69 + 4] = v252;
                  v56 = v261;
                  v71[v69 + 2] = (int)((float)(Block[0][v203] * 255.0) + 0.5);
                  v66 = v69 - v257;
                  v68 = indices_4;
                  v67 = indices + 1;
                  VertexRemap[v261] = v66;
                }
                if ( v67 == v68 )
                {
                  if ( v68 >= 16 )
                    v72 = 6 * v68 / 5;
                  else
                    v72 = 16;
                  v228 = v72;
                  indices_8 = (char *)realloc(indices_8, 2 * v72);
                  memset(&indices_8[2 * v68], 0, 2 * (v228 - v68));
                  v56 = v261;
                  v67 = indices + 1;
                  indices_4 = v228;
                  v66 = VertexRemap[v261];
                }
                v199 = v67 + 1;
                *(_WORD *)&indices_8[2 * v238] = v257 + v66;
                v73 = VertexRemap[v56 + 1];
                v74 = indices_4;
                if ( v73 == 0xFF )
                {
                  v75 = vertices;
                  if ( vertices == v51 )
                  {
                    if ( v51 >= 16 )
                      v76 = 6 * v51 / 5;
                    else
                      v76 = 16;
                    v229 = v76;
                    vertices_8 = (char *)realloc(vertices_8, 5 * v76);
                    memset(&vertices_8[4 * v51 + v51], 0, 5 * (v229 - v51));
                    v51 = v229;
                    v75 = vertices;
                  }
                  bottom_level_3b = bottom_level_3 + 1;
                  vertices = v75 + 1;
                  v77 = &vertices_8[4 * v75];
                  v77[v75] = bottom_level_3b;
                  v77[v75 + 1] = gepard_3;
                  v77[v75 + 3] = bottom_level_3b;
                  v77[v75 + 4] = gepard_3;
                  v56 = v261;
                  v77[v75 + 2] = (int)((float)(Block[v261 + 1][k] * 255.0) + 0.5);
                  v73 = v75 - blendmap_3;
                  v74 = indices_4;
                  VertexRemap[v261 + 1] = v73;
                }
                if ( v199 == v74 )
                {
                  if ( v74 >= 16 )
                    v78 = 6 * v74 / 5;
                  else
                    v78 = 16;
                  bottom_levelf = v78;
                  indices_8 = (char *)realloc(indices_8, 2 * v78);
                  memset(&indices_8[2 * v74], 0, 2 * (bottom_levelf - v74));
                  v56 = v261;
                  indices_4 = bottom_levelf;
                  v73 = VertexRemap[v261 + 1];
                }
                v57 = (float)(DOUBLE_0_0001);

                indices += 3;
                *(_WORD *)&indices_8[2 * v238 + 2] = v257 + v73;
              }
              v55 = k;
            }
            v200 = v55 + 10 * (v56 + 1);
            if ( Block[0][v200] < v57 && Block[v56 + 9][v55] < v57 && Block[v56 + 10][v55] < v57 )
              goto LABEL_154;
            v79 = *v248;
            if ( *v248 == 0xFF )
            {
              v79 = v55;
              *v248 = v55;
            }
            v80 = indices_4;
            if ( (v79 == k) == (m == 0) )
            {
              v81 = VertexRemap[v56 + 9];
              if ( v81 == 0xFF )
              {
                v82 = vertices;
                if ( vertices == v51 )
                {
                  if ( v51 >= 16 )
                    v83 = 6 * v51 / 5;
                  else
                    v83 = 16;
                  bottom_levelg = v83;
                  vertices_8 = (char *)realloc(vertices_8, 5 * v83);
                  memset(&vertices_8[4 * v51 + v51], 0, 5 * (bottom_levelg - v51));
                  v51 = bottom_levelg;
                  v82 = vertices;
                }
                vertices = v82 + 1;
                v84 = &vertices_8[4 * v82];
                bottom_level_3a = v242;
                v84[v82] = v242;
                v84[v82 + 1] = v252 + 1;
                v84[v82 + 3] = v242;
                v84[v82 + 4] = v252 + 1;
                v56 = v261;
                v84[v82 + 2] = (int)((float)(Block[v261 + 9][k] * 255.0) + 0.5);
                v81 = v82 - v257;
                v80 = indices_4;
                VertexRemap[v261 + 9] = v81;
              }
              else
              {
                bottom_level_3a = v242;
              }
              if ( indices == v80 )
              {
                if ( v80 >= 16 )
                  v85 = 6 * v80 / 5;
                else
                  v85 = 16;
                v230 = v85;
                indices_8 = (char *)realloc(indices_8, 2 * v85);
                memset(&indices_8[2 * v80], 0, 2 * (v230 - v80));
                v56 = v261;
                indices_4 = v230;
                v81 = VertexRemap[v261 + 9];
              }
              *(_WORD *)&indices_8[2 * indices] = v257 + v81;
              v86 = VertexRemap[v56 + 1];
              v87 = indices + 1;
              v88 = indices_4;
              v239 = indices + 1;
              if ( v86 == 0xFF )
              {
                v89 = vertices;
                if ( vertices == v51 )
                {
                  if ( v51 >= 16 )
                    v90 = 6 * v51 / 5;
                  else
                    v90 = 16;
                  v231 = v90;
                  vertices_8 = (char *)realloc(vertices_8, 5 * v90);
                  memset(&vertices_8[4 * v51 + v51], 0, 5 * (v231 - v51));
                  v51 = v231;
                  v89 = vertices;
                }
                vertices = v89 + 1;
                v91 = &vertices_8[4 * v89];
                v91[v89] = bottom_level_3a + 1;
                v91[v89 + 1] = v252;
                v91[v89 + 3] = bottom_level_3a + 1;
                v91[v89 + 4] = v252;
                v56 = v261;
                v91[v89 + 2] = (int)((float)(Block[0][v200] * 255.0) + 0.5);
                v86 = v89 - v257;
                v88 = indices_4;
                v87 = indices + 1;
                VertexRemap[v261 + 1] = v86;
              }
              if ( v87 == v88 )
              {
                if ( v88 >= 16 )
                  v92 = 6 * v88 / 5;
                else
                  v92 = 16;
                v232 = v92;
                indices_8 = (char *)realloc(indices_8, 2 * v92);
                memset(&indices_8[2 * v88], 0, 2 * (v232 - v88));
                v56 = v261;
                v87 = indices + 1;
                indices_4 = v232;
                v86 = VertexRemap[v261 + 1];
              }
              v201 = v87 + 1;
              *(_WORD *)&indices_8[2 * v239] = v257 + v86;
              v93 = VertexRemap[v56 + 10];
              v94 = indices_4;
              if ( v93 == 0xFF )
              {
                v95 = vertices;
                if ( vertices == v51 )
                {
                  if ( v51 >= 16 )
                    v96 = 6 * v51 / 5;
                  else
                    v96 = 16;
                  v233 = v96;
                  vertices_8 = (char *)realloc(vertices_8, 5 * v96);
                  memset(&vertices_8[4 * v51 + v51], 0, 5 * (v233 - v51));
                  v51 = v233;
                  v95 = vertices;
                }
                bottom_level_3c = bottom_level_3a + 1;
                vertices = v95 + 1;
                v97 = &vertices_8[4 * v95];
                v97[v95] = bottom_level_3c;
                v97[v95 + 1] = v252 + 1;
                v97[v95 + 3] = bottom_level_3c;
                v97[v95 + 4] = v252 + 1;
                v97[v95 + 2] = (int)((float)(Block[v261 + 10][k] * 255.0) + 0.5);
                v93 = v95 - v257;
                v94 = indices_4;
                VertexRemap[v261 + 10] = v93;
              }
              if ( v201 == v94 )
              {
                if ( v94 >= 16 )
                  v98 = 6 * v94 / 5;
                else
                  v98 = 16;
                bottom_levelh = v98;
                indices_8 = (char *)realloc(indices_8, 2 * v98);
                memset(&indices_8[2 * v94], 0, 2 * (bottom_levelh - v94));
                indices_4 = bottom_levelh;
                v93 = VertexRemap[v261 + 10];
              }
              v52 = indices + 3;
              indices += 3;
              *(_WORD *)&indices_8[2 * v239 + 2] = v257 + v93;
            }
            else
            {
LABEL_154:
              v52 = indices;
            }
            v54 = v242;
            v53 = v248;
          }
          ++v54;
          v53 += 2;
          v242 = v54;
          v248 = v53;
        }
        while ( v54 < 8 );
        ++v252;
        v223 += 8;
        v210 += 9;
      }
      while ( v252 < 8 );
      if ( v52 != v205 )
      {
        if ( materials == materials_4 )
        {
          if ( materials_4 >= 16 )
            v99 = 6 * materials_4 / 5;
          else
            v99 = 16;
          materials_8 = (char *)realloc(materials_8, 24 * v99);
          memset(&materials_8[24 * materials_4], 0, 24 * (v99 - materials_4));
          materials_4 = v99;
        }
        v100 = materials++;
        v101 = 3 * v100;
        *(_DWORD *)&materials_8[8 * v101] = this->Gepard->AddRefTexture(textures[k]);
        *(_DWORD *)&materials_8[8 * v101 + 4] = -1;
        *(_DWORD *)&materials_8[8 * v101 + 8] = (indices - v205) / 3;
        *(_DWORD *)&materials_8[8 * v101 + 20] = m;
        *(_DWORD *)&materials_8[8 * v101 + 12] = v257;
        *(_DWORD *)&materials_8[8 * v101 + 16] = vertices - v257;
      }
    }
  }
  v202 = -1;
  v234 = -1;
  this->Gepard->ShaderInfos.current = this->Gepard->ShaderInfos.first;
  // x64 fix: original IDA decomp aliased an SShaderInfo* through `int
  // bottom_levelc` (and a save copy `int v110`) and then read fields via
  // hardcoded x86 byte offsets `*(float*/_DWORD *)(.. + 128/132/140/144/
  // 148/152)`. On x64 the leading `next, prev` and embedded `SMesh *mesh`
  // grow from 4->8 bytes, shifting xpos/zpos/ShaderTHandle/XVertices/
  // ZVertices/Rotation by +8/+8/+12/+12/+12/+12. The +140 read (intended
  // `ShaderTHandle`) lands on `zpos` -> garbage texture handle ->
  // shadertextures don't paint; the +152 switch (intended `Rotation`)
  // lands on `ShaderTHandle` -> rotation arms 0..3 don't match -> UVs
  // wrong / terrain texturing looks uniform. Replace with typed access.
  for ( n = this->Gepard; n->ShaderInfos.current; n = this->Gepard )
  {
    si = n->ShaderInfos.current;
    v103 = (int)si->xpos - xbig;
    v236 = v103 + si->XVertices - 1;
    v104 = (int)si->zpos - zbig;
    v240 = v104 + si->ZVertices - 1;
    if ( v103 < 8 )
    {
      v105 = v103 + si->XVertices - 1;
      if ( v236 > 0 && v104 < 8 && v240 > 0 )
      {
        v106 = 0;
        if ( v103 >= 0 )
          v106 = (SGepard *)((int)si->xpos - xbig);
        v198 = v106;
        if ( v236 > 8 )
          v105 = 8;
        v237 = v105;
        v107 = 0;
        if ( v104 >= 0 )
          v107 = (unsigned char *)((int)si->zpos - zbig);
        blendmapc = v107;
        v108 = v104 + si->ZVertices - 1;
        if ( v240 > 8 )
          v108 = 8;
        v241 = v108;
        memset(VertexRemap, 255, sizeof(VertexRemap));
        v109 = indices;
        v256 = vertices;
        v204 = indices;
        if ( (int)blendmapc < v241 )
        {
          v111 = v237;
          do
          {
            v112 = v198;
            geparde = v198;
            if ( (int)v198 < v111 )
            {
              v113 = 9 * (_DWORD)blendmapc;
              v114 = vertices;
              texturesa = (int *)(9 * (_DWORD)blendmapc);
              do
              {
                v115 = VertexRemap[(_DWORD)v112 + v113 + 9];
                if ( v115 == 0xFF )
                {
                  if ( v114 == v51 )
                  {
                    if ( v51 >= 16 )
                      v116 = 6 * v51 / 5;
                    else
                      v116 = 16;
                    vertices_8 = (char *)realloc(vertices_8, 5 * v116);
                    memset(&vertices_8[4 * v51 + v51], 0, 5 * (v116 - v51));
                    v114 = vertices;
                    v51 = v116;
                    v113 = 9 * (_DWORD)blendmapc;
                  }
                  v117 = v114;
                  v249 = &vertices_8[4 * v114];
                  vertices = v114 + 1;
                  v249[v114] = (char)geparde;
                  v249[v114 + 1] = (_BYTE)blendmapc + 1;
                  switch ( si->Rotation )
                  {
                    case 0:
                      v249[v114 + 3] = ((_BYTE)geparde + xbig - (int)si->xpos)
                                     * (8
                                      / (si->XVertices - 1));
                      v118 = 8 / (si->ZVertices - 1);
                      v119 = (_BYTE)blendmapc + zbig - (int)si->zpos + 1;
                      goto LABEL_195;
                    case 1:
                      v249[v114 + 3] = ((_BYTE)blendmapc + zbig - (int)si->zpos + 1)
                                     * (8
                                      / (si->ZVertices - 1));
                      v120 = 8
                           - ((_BYTE)geparde + xbig - (int)si->xpos)
                           * (8
                            / (si->XVertices - 1));
                      goto LABEL_196;
                    case 2:
                      v249[v114 + 3] = 8
                                     - ((_BYTE)geparde + xbig - (int)si->xpos)
                                     * (8
                                      / (si->XVertices - 1));
                      v120 = ~((_BYTE)blendmapc + zbig - (int)si->zpos)
                           * (8
                            / (si->ZVertices - 1))
                           + 8;
                      goto LABEL_196;
                    case 3:
                      v249[v114 + 3] = ~((_BYTE)blendmapc + zbig - (int)si->zpos)
                                     * (8
                                      / (si->ZVertices - 1))
                                     + 8;
                      v118 = 8 / (si->XVertices - 1);
                      v119 = (_BYTE)geparde + xbig - (int)si->xpos;
LABEL_195:
                      v120 = v119 * v118;
LABEL_196:
                      v117 = v114;
                      v249[v114 + 4] = v120;
                      break;
                    default:
                      break;
                  }
                  v249[v117 + 2] = -1;
                  v253 = v117 - v256;
                  VertexRemap[(_DWORD)geparde + v113 + 9] = v117 - v256;
                  v109 = indices;
                  v115 = v253;
                }
                v121 = indices_4;
                if ( v109 == indices_4 )
                {
                  if ( indices_4 >= 16 )
                    v122 = 6 * indices_4 / 5;
                  else
                    v122 = 16;
                  v211 = v122;
                  indices_8 = (char *)realloc(indices_8, 2 * v122);
                  memset(&indices_8[2 * indices_4], 0, 2 * (v211 - indices_4));
                  v121 = v211;
                  v109 = indices;
                  indices_4 = v211;
                  v115 = VertexRemap[(unsigned int)geparde + (_DWORD)texturesa + 9];
                }
                v224 = v109 + 1;
                *(_WORD *)&indices_8[2 * v109] = v256 + v115;
                v123 = VertexRemap[(unsigned int)geparde + (_DWORD)texturesa];
                if ( v123 == 0xFF )
                {
                  v124 = vertices;
                  if ( vertices == v51 )
                  {
                    if ( v51 >= 16 )
                      v125 = 6 * v51 / 5;
                    else
                      v125 = 16;
                    v212 = v125;
                    vertices_8 = (char *)realloc(vertices_8, 5 * v125);
                    memset(&vertices_8[4 * v51 + v51], 0, 5 * (v212 - v51));
                    v51 = v212;
                    v124 = vertices;
                  }
                  v126 = v124;
                  v243 = v124;
                  vertices = v124 + 1;
                  v258 = &vertices_8[4 * v124];
                  v127 = v258;
                  v258[v243] = (char)geparde;
                  v258[v126 + 1] = (char)blendmapc;
                  v128 = geparde;
                  switch ( si->Rotation )
                  {
                    case 0:
                      v258[v243 + 3] = ((_BYTE)geparde + xbig - (int)si->xpos)
                                     * (8
                                      / (si->XVertices - 1));
                      v129 = 8 / (si->ZVertices - 1);
                      v130 = (_BYTE)blendmapc + zbig - (int)si->zpos;
                      goto LABEL_214;
                    case 1:
                      v258[v243 + 3] = ((_BYTE)blendmapc + zbig - (int)si->zpos)
                                     * (8
                                      / (si->ZVertices - 1));
                      v131 = 8
                           - ((_BYTE)geparde + xbig - (int)si->xpos)
                           * (8
                            / (si->XVertices - 1));
                      goto LABEL_215;
                    case 2:
                      v258[v243 + 3] = 8
                                     - ((_BYTE)geparde + xbig - (int)si->xpos)
                                     * (8
                                      / (si->XVertices - 1));
                      v131 = 8
                           - ((_BYTE)blendmapc + zbig - (int)si->zpos)
                           * (8
                            / (si->ZVertices - 1));
                      goto LABEL_215;
                    case 3:
                      v258[v243 + 3] = 8
                                     - ((_BYTE)blendmapc + zbig - (int)si->zpos)
                                     * (8
                                      / (si->ZVertices - 1));
                      v129 = 8 / (si->XVertices - 1);
                      v130 = (_BYTE)geparde + xbig - (int)si->xpos;
LABEL_214:
                      v131 = v130 * v129;
LABEL_215:
                      v126 = v243;
                      v258[v243 + 4] = v131;
                      v127 = v258;
                      v128 = geparde;
                      break;
                    default:
                      break;
                  }
                  v121 = indices_4;
                  v127[v126 + 2] = -1;
                  v123 = v126 - v256;
                  VertexRemap[(unsigned int)v128 + (_DWORD)texturesa] = v123;
                }
                v132 = v224;
                if ( v224 == v121 )
                {
                  if ( v121 >= 16 )
                    v133 = 6 * v121 / 5;
                  else
                    v133 = 16;
                  v213 = v133;
                  indices_8 = (char *)realloc(indices_8, 2 * v133);
                  memset(&indices_8[2 * v121], 0, 2 * (v213 - v121));
                  v121 = v213;
                  indices_4 = v213;
                  v123 = VertexRemap[(unsigned int)geparde + (_DWORD)texturesa];
                  v132 = v224;
                }
                v254 = v132 + 1;
                *(_WORD *)&indices_8[2 * v224] = v256 + v123;
                v134 = 9 * (_DWORD)blendmapc;
                v135 = VertexRemap[(unsigned int)geparde + (_DWORD)texturesa + 1];
                if ( v135 == 0xFF )
                {
                  v136 = vertices;
                  if ( vertices == v51 )
                  {
                    if ( v51 >= 16 )
                      v137 = 6 * v51 / 5;
                    else
                      v137 = 16;
                    v214 = v137;
                    vertices_8 = (char *)realloc(vertices_8, 5 * v137);
                    memset(&vertices_8[4 * v51 + v51], 0, 5 * (v214 - v51));
                    v51 = v214;
                    v136 = vertices;
                  }
                  v138 = v136;
                  v244 = v136;
                  vertices = v136 + 1;
                  v259 = &vertices_8[4 * v136];
                  v259[v136] = (_BYTE)geparde + 1;
                  v139 = v259;
                  v259[v138 + 1] = (char)blendmapc;
                  v134 = 9 * (_DWORD)blendmapc;
                  switch ( si->Rotation )
                  {
                    case 0:
                      v259[v244 + 3] = ((_BYTE)geparde + xbig - (int)si->xpos + 1)
                                     * (8
                                      / (si->XVertices - 1));
                      v140 = 8 / (si->ZVertices - 1);
                      v141 = (_BYTE)blendmapc + zbig - (int)si->zpos;
                      goto LABEL_233;
                    case 1:
                      v259[v244 + 3] = ((_BYTE)blendmapc + zbig - (int)si->zpos)
                                     * (8
                                      / (si->ZVertices - 1));
                      v142 = ~((_BYTE)geparde + xbig - (int)si->xpos)
                           * (8
                            / (si->XVertices - 1))
                           + 8;
                      goto LABEL_234;
                    case 2:
                      v259[v244 + 3] = ~((_BYTE)geparde + xbig - (int)si->xpos)
                                     * (8
                                      / (si->XVertices - 1))
                                     + 8;
                      v142 = 8
                           - ((_BYTE)blendmapc + zbig - (int)si->zpos)
                           * (8
                            / (si->ZVertices - 1));
                      goto LABEL_234;
                    case 3:
                      v259[v244 + 3] = 8
                                     - ((_BYTE)blendmapc + zbig - (int)si->zpos)
                                     * (8
                                      / (si->ZVertices - 1));
                      v140 = 8 / (si->XVertices - 1);
                      v141 = (_BYTE)geparde + xbig - (int)si->xpos + 1;
LABEL_233:
                      v142 = v141 * v140;
LABEL_234:
                      v138 = v244;
                      v259[v244 + 4] = v142;
                      v139 = v259;
                      v134 = 9 * (_DWORD)blendmapc;
                      break;
                    default:
                      break;
                  }
                  v121 = indices_4;
                  v139[v138 + 2] = -1;
                  v135 = v138 - v256;
                  VertexRemap[(_DWORD)geparde + v134 + 1] = v135;
                }
                v143 = v254;
                if ( v254 == v121 )
                {
                  if ( v121 >= 16 )
                    v144 = 6 * v121 / 5;
                  else
                    v144 = 16;
                  v215 = v144;
                  indices_8 = (char *)realloc(indices_8, 2 * v144);
                  memset(&indices_8[2 * v121], 0, 2 * (v215 - v121));
                  v134 = 9 * (_DWORD)blendmapc;
                  indices_4 = v215;
                  v135 = VertexRemap[(unsigned int)geparde + (_DWORD)texturesa + 1];
                  v143 = v254;
                }
                v206 = v143 + 1;
                *(_WORD *)&indices_8[2 * v254] = v256 + v135;
                v145 = indices_4;
                v146 = VertexRemap[(_DWORD)geparde + v134 + 9];
                if ( v146 == 0xFF )
                {
                  v147 = vertices;
                  if ( vertices == v51 )
                  {
                    if ( v51 >= 16 )
                      v148 = 6 * v51 / 5;
                    else
                      v148 = 16;
                    v216 = v148;
                    vertices_8 = (char *)realloc(vertices_8, 5 * v148);
                    memset(&vertices_8[4 * v51 + v51], 0, 5 * (v216 - v51));
                    v51 = v216;
                    v147 = vertices;
                    v134 = 9 * (_DWORD)blendmapc;
                  }
                  v149 = v147;
                  v245 = v147;
                  vertices = v147 + 1;
                  v250 = &vertices_8[4 * v147];
                  v250[v147] = (char)geparde;
                  v250[v147 + 1] = (_BYTE)blendmapc + 1;
                  switch ( si->Rotation )
                  {
                    case 0:
                      v250[v147 + 3] = ((_BYTE)geparde + xbig - (int)si->xpos)
                                     * (8
                                      / (si->XVertices - 1));
                      v150 = 8 / (si->ZVertices - 1);
                      v151 = (_BYTE)blendmapc + zbig - (int)si->zpos + 1;
                      goto LABEL_252;
                    case 1:
                      v250[v147 + 3] = ((_BYTE)blendmapc + zbig - (int)si->zpos + 1)
                                     * (8
                                      / (si->ZVertices - 1));
                      v152 = 8
                           - ((_BYTE)geparde + xbig - (int)si->xpos)
                           * (8
                            / (si->XVertices - 1));
                      goto LABEL_253;
                    case 2:
                      v250[v147 + 3] = 8
                                     - ((_BYTE)geparde + xbig - (int)si->xpos)
                                     * (8
                                      / (si->XVertices - 1));
                      v152 = ~((_BYTE)blendmapc + zbig - (int)si->zpos)
                           * (8
                            / (si->ZVertices - 1))
                           + 8;
                      goto LABEL_253;
                    case 3:
                      v250[v147 + 3] = ~((_BYTE)blendmapc + zbig - (int)si->zpos)
                                     * (8
                                      / (si->ZVertices - 1))
                                     + 8;
                      v150 = 8 / (si->XVertices - 1);
                      v151 = (_BYTE)geparde + xbig - (int)si->xpos;
LABEL_252:
                      v152 = v151 * v150;
LABEL_253:
                      v149 = v245;
                      v250[v245 + 4] = v152;
                      v134 = 9 * (_DWORD)blendmapc;
                      break;
                    default:
                      break;
                  }
                  v145 = indices_4;
                  v250[v149 + 2] = -1;
                  v146 = v149 - v256;
                  VertexRemap[(_DWORD)geparde + v134 + 9] = v146;
                }
                v153 = v206;
                if ( v206 == v145 )
                {
                  if ( v145 >= 16 )
                    v154 = 6 * v145 / 5;
                  else
                    v154 = 16;
                  v217 = v154;
                  indices_8 = (char *)realloc(indices_8, 2 * v154);
                  memset(&indices_8[2 * v145], 0, 2 * (v217 - v145));
                  v134 = 9 * (_DWORD)blendmapc;
                  indices_4 = v217;
                  v146 = VertexRemap[(unsigned int)geparde + (_DWORD)texturesa + 9];
                  v153 = v206;
                }
                v207 = v153 + 1;
                *(_WORD *)&indices_8[2 * v254 + 2] = v256 + v146;
                v155 = indices_4;
                v156 = VertexRemap[(_DWORD)geparde + v134 + 1];
                if ( v156 == 0xFF )
                {
                  v157 = vertices;
                  if ( vertices == v51 )
                  {
                    if ( v51 >= 16 )
                      v158 = 6 * v51 / 5;
                    else
                      v158 = 16;
                    v218 = v158;
                    vertices_8 = (char *)realloc(vertices_8, 5 * v158);
                    memset(&vertices_8[4 * v51 + v51], 0, 5 * (v218 - v51));
                    v51 = v218;
                    v157 = vertices;
                  }
                  v159 = v157;
                  v246 = v157;
                  vertices = v157 + 1;
                  v260 = &vertices_8[4 * v157];
                  v260[v157] = (_BYTE)geparde + 1;
                  v160 = v260;
                  v260[v159 + 1] = (char)blendmapc;
                  v134 = 9 * (_DWORD)blendmapc;
                  switch ( si->Rotation )
                  {
                    case 0:
                      v260[v246 + 3] = ((_BYTE)geparde + xbig - (int)si->xpos + 1)
                                     * (8
                                      / (si->XVertices - 1));
                      v161 = 8 / (si->ZVertices - 1);
                      v162 = (_BYTE)blendmapc + zbig - (int)si->zpos;
                      goto LABEL_271;
                    case 1:
                      v260[v246 + 3] = ((_BYTE)blendmapc + zbig - (int)si->zpos)
                                     * (8
                                      / (si->ZVertices - 1));
                      v163 = ~((_BYTE)geparde + xbig - (int)si->xpos)
                           * (8
                            / (si->XVertices - 1))
                           + 8;
                      goto LABEL_272;
                    case 2:
                      v260[v246 + 3] = ~((_BYTE)geparde + xbig - (int)si->xpos)
                                     * (8
                                      / (si->XVertices - 1))
                                     + 8;
                      v163 = 8
                           - ((_BYTE)blendmapc + zbig - (int)si->zpos)
                           * (8
                            / (si->ZVertices - 1));
                      goto LABEL_272;
                    case 3:
                      v260[v246 + 3] = 8
                                     - ((_BYTE)blendmapc + zbig - (int)si->zpos)
                                     * (8
                                      / (si->ZVertices - 1));
                      v161 = 8 / (si->XVertices - 1);
                      v162 = (_BYTE)geparde + xbig - (int)si->xpos + 1;
LABEL_271:
                      v163 = v162 * v161;
LABEL_272:
                      v159 = v246;
                      v260[v246 + 4] = v163;
                      v160 = v260;
                      v134 = 9 * (_DWORD)blendmapc;
                      break;
                    default:
                      break;
                  }
                  v155 = indices_4;
                  v160[v159 + 2] = -1;
                  v156 = v159 - v256;
                  VertexRemap[(_DWORD)geparde + v134 + 1] = v156;
                }
                v164 = v207;
                if ( v207 == v155 )
                {
                  if ( v155 >= 16 )
                    v165 = 6 * v155 / 5;
                  else
                    v165 = 16;
                  v219 = v165;
                  indices_8 = (char *)realloc(indices_8, 2 * v165);
                  memset(&indices_8[2 * v155], 0, 2 * (v219 - v155));
                  v134 = 9 * (_DWORD)blendmapc;
                  indices_4 = v219;
                  v156 = VertexRemap[(unsigned int)geparde + (_DWORD)texturesa + 1];
                  v164 = v207;
                }
                v208 = v164 + 1;
                *(_WORD *)&indices_8[2 * v254 + 4] = v256 + v156;
                v166 = indices_4;
                v167 = VertexRemap[(_DWORD)geparde + v134 + 10];
                if ( v167 == 0xFF )
                {
                  v168 = vertices;
                  if ( vertices == v51 )
                  {
                    if ( v51 >= 16 )
                      v169 = 6 * v51 / 5;
                    else
                      v169 = 16;
                    v220 = v169;
                    vertices_8 = (char *)realloc(vertices_8, 5 * v169);
                    memset(&vertices_8[4 * v51 + v51], 0, 5 * (v220 - v51));
                    v51 = v220;
                    v168 = vertices;
                    v134 = 9 * (_DWORD)blendmapc;
                  }
                  v170 = v168;
                  v247 = v168;
                  vertices = v168 + 1;
                  v251 = &vertices_8[4 * v168];
                  v251[v168] = (_BYTE)geparde + 1;
                  v251[v168 + 1] = (_BYTE)blendmapc + 1;
                  switch ( si->Rotation )
                  {
                    case 0:
                      v251[v168 + 3] = ((_BYTE)geparde + xbig - (int)si->xpos + 1)
                                     * (8
                                      / (si->XVertices - 1));
                      v171 = 8 / (si->ZVertices - 1);
                      v172 = (_BYTE)blendmapc + zbig - (int)si->zpos + 1;
                      goto LABEL_290;
                    case 1:
                      v251[v168 + 3] = ((_BYTE)blendmapc + zbig - (int)si->zpos + 1)
                                     * (8
                                      / (si->ZVertices - 1));
                      v173 = ~((_BYTE)geparde + xbig - (int)si->xpos)
                           * (8
                            / (si->XVertices - 1))
                           + 8;
                      goto LABEL_291;
                    case 2:
                      v251[v168 + 3] = ~((_BYTE)geparde + xbig - (int)si->xpos)
                                     * (8
                                      / (si->XVertices - 1))
                                     + 8;
                      v173 = ~((_BYTE)blendmapc + zbig - (int)si->zpos)
                           * (8
                            / (si->ZVertices - 1))
                           + 8;
                      goto LABEL_291;
                    case 3:
                      v251[v168 + 3] = ~((_BYTE)blendmapc + zbig - (int)si->zpos)
                                     * (8
                                      / (si->ZVertices - 1))
                                     + 8;
                      v171 = 8 / (si->XVertices - 1);
                      v172 = (_BYTE)geparde + xbig - (int)si->xpos + 1;
LABEL_290:
                      v173 = v172 * v171;
LABEL_291:
                      v170 = v247;
                      v251[v247 + 4] = v173;
                      v134 = 9 * (_DWORD)blendmapc;
                      break;
                    default:
                      break;
                  }
                  v166 = indices_4;
                  v251[v170 + 2] = -1;
                  v167 = v170 - v256;
                  VertexRemap[(_DWORD)geparde + v134 + 10] = v167;
                }
                v174 = v208;
                if ( v208 == v166 )
                {
                  if ( v166 >= 16 )
                    v175 = 6 * v166 / 5;
                  else
                    v175 = 16;
                  v221 = v175;
                  indices_8 = (char *)realloc(indices_8, 2 * v175);
                  memset(&indices_8[2 * v166], 0, 2 * (v221 - v166));
                  v113 = 9 * (_DWORD)blendmapc;
                  indices_4 = v221;
                  v167 = VertexRemap[(unsigned int)geparde + (_DWORD)texturesa + 10];
                  v174 = v208;
                }
                else
                {
                  v113 = 9 * (_DWORD)blendmapc;
                }
                indices = v174 + 1;
                *(_WORD *)&indices_8[2 * v254 + 6] = v256 + v167;
                v114 = vertices;
                v112 = (SGepard *)((_DWORD)geparde + 1);
                v109 = indices;
                geparde = v112;
              }
              while ( (int)v112 < v237 );
              v111 = v237;
            }
            ++blendmapc;
          }
          while ( (int)blendmapc < v241 );
        }
        v176 = si->ShaderTHandle;
        if ( v202 == v176 )
        {
          v177 = 3 * v234;
          *(_DWORD *)&materials_8[8 * v177 + 8] += (v109 - v204) / 3;
          *(_DWORD *)&materials_8[8 * v177 + 16] += vertices - v256;
        }
        else
        {
          if ( materials == materials_4 )
          {
            if ( materials_4 >= 16 )
              v178 = 6 * materials_4 / 5;
            else
              v178 = 16;
            gepardg = (SGepard *)v178;
            materials_8 = (char *)realloc(materials_8, 24 * v178);
            memset(&materials_8[24 * materials_4], 0, 24 * ((_DWORD)gepardg - materials_4));
            v176 = si->ShaderTHandle;
            materials_4 = (int)gepardg;
          }
          v234 = materials;
          v179 = 3 * materials++;
          *(_DWORD *)&materials_8[8 * v179] = this->Gepard->AddRefTexture(v176);
          *(_DWORD *)&materials_8[8 * v179 + 4] = -1;
          *(_DWORD *)&materials_8[8 * v179 + 8] = (indices - v204) / 3;
          *(_DWORD *)&materials_8[8 * v179 + 20] = 2;
          *(_DWORD *)&materials_8[8 * v179 + 12] = v256;
          *(_DWORD *)&materials_8[8 * v179 + 16] = vertices - v256;
          v202 = si->ShaderTHandle;
        }
      }
    }
    v180 = this->Gepard->ShaderInfos.current;
    if ( this->Gepard->ShaderInfos.Closed )
    {
      if ( !v180 )
        goto LABEL_330;
      v181 = v180->next;
      if ( v181 )
        this->Gepard->ShaderInfos.current = v181;
      else
        this->Gepard->ShaderInfos.current = this->Gepard->ShaderInfos.first;
    }
    else if ( v180 )
    {
      this->Gepard->ShaderInfos.current = v180->next;
    }
    else
    {
      this->Gepard->ShaderInfos.current = 0;
    }
  }
  this->ParcelVertices = (unsigned short *)operator new[](2 * vertices);
  this->CreateVertexBuffer(vertices);
  v182 = 0;
  bottom_leveld = 0;
  if ( vertices > 0 )
  {
    v183 = vertices_8 + 2;
    gepardf = (SGepard *)(vertices_8 + 2);
    do
    {
      v184 = (unsigned char)*(v183 - 2) + stride * (unsigned char)*(v183 - 1);
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + v182 * this->VertexSize) = (float)(xbig
                                                                                               + (unsigned char)*(v183 - 2));
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + bottom_leveld * this->VertexSize + 4) = heightmap[v184];
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + bottom_leveld * this->VertexSize + 8) = (float)(zbig + *((unsigned char *)gepardf - 1));
      *(_DWORD *)((char *)this->lpVertices + this->OffsetDiffuse + bottom_leveld * this->VertexSize) = colormap[v184] | (*(unsigned char *)gepardf << 24);
      *(D3DXVECTOR3 *)((char *)this->lpVertices + this->OffsetNormal + bottom_leveld * this->VertexSize) = normalmap[v184];
      v185 = (float)((double)*((unsigned char *)gepardf + 1) * 0.125f);

      *(float *)((char *)this->lpVertices + this->OffsetTexture1 + bottom_leveld * this->VertexSize) = v185;
      *(float *)((char *)this->lpVertices + this->OffsetTexture1 + bottom_leveld * this->VertexSize + 4) = (float)*((unsigned char *)gepardf + 2) * 0.125f;
      v183 = (char *)gepardf + 5;
      v186 = *((unsigned char *)gepardf - 2)
           + (*(unsigned char *)gepardf << 8)
           + 9 * *((unsigned char *)gepardf - 1);
      gepardf = (SGepard *)((char *)gepardf + 5);
      this->ParcelVertices[bottom_leveld++] = v186;
      v182 = bottom_leveld;
    }
    while ( bottom_leveld < vertices );
  }
  this->UnlockVertexBuffer();
  IndexBuffer = this->CreateIndexBuffer(0, indices);
  memcpy(IndexBuffer, indices_8, 2 * indices);
  this->UnlockIndexBuffer(0);
  MaterialBuffer = this->CreateMaterialBuffer(materials);
  memcpy(MaterialBuffer, materials_8, 24 * materials);
  v189 = free;
  if ( materials_8 )
  {
    free(materials_8);
    v189 = free;
  }
  if ( indices_8 )
    v189(indices_8);
  if ( vertices_8 )
    free(vertices_8);
}

//----- (0044FDE0) --------------------------------------------------------

SParcel::SParcel(SGepard *gepard, int xbig, int zbig)

{
  SParcel *v4; // ebx
  float v5;
  int v6;
  int v7;
  float v8; // xmm4_4
  float v9; // xmm5_4
  float v10; // xmm6_4
  float v11; // xmm7_4
  SGepard *v12; // eax
  float v13; // xmm1_4
  float v14; // xmm0_4
  char *v15; // edi
  char *v16; // edi
  char *v17; // edi
  char *v18; // edi
  char *v19; // eax
  char *v20; // eax
  char *v21; // eax
  char *v22; // eax
  char *v23; // eax
  char *v24; // eax
  char *v25; // eax
  char *v26; // eax
  int v27;
  int v28;
  unsigned short v29; // si
  int v30;
  short v31; // ax
  unsigned short v32; // dx
  unsigned short v33; // dx
  unsigned short v34; // dx
  unsigned short v35; // dx
  unsigned short v36; // dx
  unsigned short v37; // cx
  int v38;
  int v39;
  int v40;
  int v41;
  int v42;
  SGepard *geparda;
  int gepardb;
  short gepardc;
  int xbiga;
  int xbigb;
  int xbigc;
  v4 = this;
  new (static_cast<SMesh*>(this)) SMesh(gepard, 0x51u);
  // vtable init (compiler-generated)
  v4->CreateVertexBuffer(0x2D9u);
  v4->CreateIndexBuffer(0, 384);
  v4->CreateIndexBuffer(1, 256);
  v4->CreateIndexBuffer(2, 32);
  v5 = DOUBLE_0_125;
  v6 = xbig + 6;
  v39 = xbig + 5;
  v7 = xbig + 7;
  v40 = xbig + 4;
  v8 = (float)xbig;
  v38 = xbig + 8;
  v41 = 0;
  v9 = (float)(xbig + 1);
  v10 = (float)(xbig + 2);
  v11 = (float)(xbig + 3);
  do
  {
    xbiga = v41;
    v12 = (SGepard *)(v41 + 2);
    v42 = 0;
    for ( geparda = (SGepard *)(v41 + 2); ; v12 = geparda )
    {
      *(float *)((char *)v4->lpVertices + v4->OffsetXYZ + xbiga * v4->VertexSize) = v8;
      v13 = (float)(zbig + v42);
      v14 = (float)((double)v42 * v5);

      *(float *)((char *)this->lpVertices + this->OffsetXYZ + xbiga * this->VertexSize + 8) = v13;
      v15 = (char *)this->lpVertices + this->OffsetNormal + xbiga * this->VertexSize;
      *(_QWORD *)v15 = 0xBF80000000000000uLL;
      *((_DWORD *)v15 + 2) = 0;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetTexture1 + xbiga * this->VertexSize) = 0;
      *(float *)((char *)this->lpVertices + this->OffsetTexture1 + xbiga * this->VertexSize + 4) = v14;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetDiffuse + xbiga * this->VertexSize) = 65280;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + ((_DWORD)v12 - 1) * this->VertexSize) = v9;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + ((_DWORD)v12 - 1) * this->VertexSize + 8) = v13;
      v16 = (char *)this->lpVertices + this->OffsetNormal + ((_DWORD)v12 - 1) * this->VertexSize;
      *(_QWORD *)v16 = 0xBF80000000000000uLL;
      *((_DWORD *)v16 + 2) = 0;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetTexture1 + ((_DWORD)v12 - 1) * this->VertexSize) = 1040187392;
      *(float *)((char *)this->lpVertices + this->OffsetTexture1 + ((_DWORD)geparda - 1) * this->VertexSize + 4) = v14;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetDiffuse + ((_DWORD)geparda - 1) * this->VertexSize) = 65280;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (_DWORD)geparda * this->VertexSize) = v10;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (_DWORD)geparda * this->VertexSize + 8) = v13;
      v17 = (char *)this->lpVertices + this->OffsetNormal + (_DWORD)geparda * this->VertexSize;
      *(_QWORD *)v17 = 0xBF80000000000000uLL;
      *((_DWORD *)v17 + 2) = 0;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetTexture1 + (_DWORD)geparda * this->VertexSize) = 1048576000;
      *(float *)((char *)this->lpVertices + this->OffsetTexture1 + (_DWORD)geparda * this->VertexSize + 4) = v14;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetDiffuse + (_DWORD)geparda * this->VertexSize) = 65280;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (((_DWORD)geparda + 1)) * this->VertexSize) = v11;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (((_DWORD)geparda + 1)) * this->VertexSize + 8) = v13;
      v18 = (char *)this->lpVertices + this->OffsetNormal + (((_DWORD)geparda + 1)) * this->VertexSize;
      *(_QWORD *)v18 = 0xBF80000000000000uLL;
      *((_DWORD *)v18 + 2) = 0;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetTexture1 + (((_DWORD)geparda + 1)) * this->VertexSize) = 1052770304;
      *(float *)((char *)this->lpVertices
               + this->OffsetTexture1
               + (((_DWORD)geparda + 1)) * this->VertexSize
               + 4) = v14;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetDiffuse + (((_DWORD)geparda + 1)) * this->VertexSize) = 65280;
      v19 = (char *)((_DWORD)geparda + 2);
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (_DWORD)v19 * this->VertexSize) = (float)v40;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (_DWORD)v19 * this->VertexSize + 8) = v13;
      v20 = (char *)this->lpVertices + this->OffsetNormal + (((_DWORD)geparda + 2)) * this->VertexSize;
      *(_QWORD *)v20 = 0xBF80000000000000uLL;
      *((_DWORD *)v20 + 2) = 0;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetTexture1 + (((_DWORD)geparda + 2)) * this->VertexSize) = 1056964608;
      *(float *)((char *)this->lpVertices
               + this->OffsetTexture1
               + (((_DWORD)geparda + 2)) * this->VertexSize
               + 4) = v14;
      v21 = (char *)((_DWORD)geparda + 3);
      *(_DWORD *)((char *)this->lpVertices + this->OffsetDiffuse + (((_DWORD)geparda + 2)) * this->VertexSize) = 65280;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (_DWORD)v21 * this->VertexSize) = (float)v39;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (_DWORD)v21 * this->VertexSize + 8) = v13;
      v22 = (char *)this->lpVertices + this->OffsetNormal + (((_DWORD)geparda + 3)) * this->VertexSize;
      *(_QWORD *)v22 = 0xBF80000000000000uLL;
      *((_DWORD *)v22 + 2) = 0;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetTexture1 + (((_DWORD)geparda + 3)) * this->VertexSize) = 1059061760;
      *(float *)((char *)this->lpVertices
               + this->OffsetTexture1
               + (((_DWORD)geparda + 3)) * this->VertexSize
               + 4) = v14;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetDiffuse + (((_DWORD)geparda + 3)) * this->VertexSize) = 65280;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ
                                          + ((_DWORD)geparda + 4) * this->VertexSize) = (float)v6;
      *(float *)((char *)this->lpVertices
               + this->OffsetXYZ
               + ((_DWORD)geparda + 4) * this->VertexSize
               + 8) = v13;
      v23 = (char *)this->lpVertices + this->OffsetNormal + ((_DWORD)geparda + 4) * this->VertexSize;
      *(_QWORD *)v23 = 0xBF80000000000000uLL;
      *((_DWORD *)v23 + 2) = 0;
      *(_DWORD *)((char *)this->lpVertices
                + this->OffsetTexture1
                + ((_DWORD)geparda + 4) * this->VertexSize) = 1061158912;
      *(float *)((char *)this->lpVertices
               + this->OffsetTexture1
               + ((_DWORD)geparda + 4) * this->VertexSize
               + 4) = v14;
      *(_DWORD *)((char *)this->lpVertices
                + this->OffsetDiffuse
                + ((_DWORD)geparda + 4) * this->VertexSize) = 65280;
      *(float *)((char *)this->lpVertices
               + this->OffsetXYZ
               + (((_DWORD)geparda + 5)) * this->VertexSize) = (float)v7;
      *(float *)((char *)this->lpVertices
               + this->OffsetXYZ
               + (((_DWORD)geparda + 5)) * this->VertexSize
               + 8) = v13;
      v24 = (char *)this->lpVertices
          + this->OffsetNormal
          + (((_DWORD)geparda + 5)) * this->VertexSize;
      *(_QWORD *)v24 = 0xBF80000000000000uLL;
      *((_DWORD *)v24 + 2) = 0;
      *(_DWORD *)((char *)this->lpVertices
                + this->OffsetTexture1
                + (((_DWORD)geparda + 5)) * this->VertexSize) = 1063256064;
      *(float *)((char *)this->lpVertices
               + this->OffsetTexture1
               + (((_DWORD)geparda + 5)) * this->VertexSize
               + 4) = v14;
      *(_DWORD *)((char *)this->lpVertices
                + this->OffsetDiffuse
                + (((_DWORD)geparda + 5)) * this->VertexSize) = 65280;
      *(float *)((char *)this->lpVertices
               + this->OffsetXYZ
               + (((_DWORD)geparda + 6)) * this->VertexSize) = (float)v38;
      *(float *)((char *)this->lpVertices
               + this->OffsetXYZ
               + (((_DWORD)geparda + 6)) * this->VertexSize
               + 8) = v13;
      v25 = (char *)this->lpVertices
          + this->OffsetNormal
          + (((_DWORD)geparda + 6)) * this->VertexSize;
      *(_QWORD *)v25 = 0xBF80000000000000uLL;
      *((_DWORD *)v25 + 2) = 0;
      *(_DWORD *)((char *)this->lpVertices
                + this->OffsetTexture1
                + (((_DWORD)geparda + 6)) * this->VertexSize) = 1065353216;
      *(float *)((char *)this->lpVertices
               + this->OffsetTexture1
               + (((_DWORD)geparda + 6)) * this->VertexSize
               + 4) = v14;
      xbiga += 9;
      v26 = (char *)((_DWORD)geparda + 6);
      geparda = (SGepard *)((char *)geparda + 9);
      v5 = DOUBLE_0_125;
      ++v42;
      *(_DWORD *)((char *)this->lpVertices + this->OffsetDiffuse + (_DWORD)v26 * this->VertexSize) = 65280;
      v4 = this;
      if ( v42 >= 9 )
        break;
    }
    v41 += 81;
    v4 = this;
  }
  while ( v41 < 729 );
  v27 = 0;
  v28 = 13;
  xbigb = 0;
  gepardb = 13;
  do
  {
    v29 = v28 - 13 + 1;
    this->lpIndices[0][v27] = v28 - 13 + 9;
    this->lpIndices[0][v27 + 1] = v28 - 13;
    this->lpIndices[0][v27 + 2] = v29;
    this->lpIndices[0][v27 + 3] = v28 - 13 + 9;
    this->lpIndices[0][v27 + 4] = v29;
    this->lpIndices[0][v27 + 5] = v28 - 13 + 10;
    this->lpIndices[0][v27 + 6] = gepardb - 3;
    this->lpIndices[0][v27 + 7] = v29;
    this->lpIndices[0][xbigb + 8] = gepardb - 11;
    this->lpIndices[0][xbigb + 9] = gepardb - 3;
    this->lpIndices[0][xbigb + 10] = gepardb - 11;
    this->lpIndices[0][xbigb + 11] = v28 - 13 + 11;
    this->lpIndices[0][xbigb + 12] = gepardb - 11 + 9;
    this->lpIndices[0][xbigb + 13] = gepardb - 11;
    this->lpIndices[0][xbigb + 14] = gepardb - 11 + 1;
    this->lpIndices[0][xbigb + 15] = gepardb - 11 + 9;
    this->lpIndices[0][xbigb + 16] = gepardb - 11 + 1;
    this->lpIndices[0][xbigb + 17] = gepardb - 11 + 10;
    this->lpIndices[0][xbigb + 18] = gepardb - 10 + 9;
    this->lpIndices[0][xbigb + 19] = gepardb - 10;
    this->lpIndices[0][xbigb + 20] = gepardb - 10 + 1;
    this->lpIndices[0][xbigb + 21] = gepardb - 10 + 9;
    this->lpIndices[0][xbigb + 22] = gepardb - 10 + 1;
    this->lpIndices[0][xbigb + 23] = gepardb;
    this->lpIndices[0][xbigb + 24] = gepardb;
    this->lpIndices[0][xbigb + 25] = gepardb - 9;
    this->lpIndices[0][xbigb + 26] = gepardb - 8;
    this->lpIndices[0][xbigb + 27] = gepardb;
    this->lpIndices[0][xbigb + 28] = gepardb - 8;
    this->lpIndices[0][xbigb + 29] = gepardb - 9 + 10;
    this->lpIndices[0][xbigb + 30] = gepardb + 1;
    this->lpIndices[0][xbigb + 31] = gepardb - 8;
    this->lpIndices[0][xbigb + 32] = gepardb - 7;
    this->lpIndices[0][xbigb + 33] = gepardb + 1;
    this->lpIndices[0][xbigb + 34] = gepardb - 7;
    this->lpIndices[0][xbigb + 35] = gepardb - 8 + 10;
    this->lpIndices[0][xbigb + 36] = gepardb + 2;
    this->lpIndices[0][xbigb + 37] = gepardb - 7;
    this->lpIndices[0][xbigb + 38] = gepardb - 6;
    this->lpIndices[0][xbigb + 39] = gepardb + 2;
    this->lpIndices[0][xbigb + 40] = gepardb - 6;
    this->lpIndices[0][xbigb + 41] = gepardb - 7 + 10;
    this->lpIndices[0][xbigb + 42] = gepardb + 3;
    this->lpIndices[0][xbigb + 43] = gepardb - 6;
    this->lpIndices[0][xbigb + 44] = gepardb - 5;
    this->lpIndices[0][xbigb + 45] = gepardb + 3;
    this->lpIndices[0][xbigb + 46] = gepardb - 5;
    this->lpIndices[0][xbigb + 47] = gepardb - 6 + 10;
    v27 = xbigb + 48;
    v28 = gepardb + 9;
    xbigb += 48;
    gepardb = v28;
  }
  while ( v28 < 85 );
  v30 = 0;
  xbigc = 7;
  v31 = 0;
  gepardc = 0;
  do
  {
    this->lpIndices[1][v30] = 9 * v31;
    this->lpIndices[1][v30 + 1] = 9 * v31 + 1;
    this->lpIndices[1][v30 + 2] = 9 * v31;
    this->lpIndices[1][v30 + 3] = 9 * v31 + 9;
    this->lpIndices[1][v30 + 4] = 9 * v31 + 1;
    this->lpIndices[1][v30 + 5] = 9 * v31 + 2;
    this->lpIndices[1][v30 + 6] = 9 * v31 + 1;
    v32 = 9 * v31 + 2;
    this->lpIndices[1][v30 + 7] = 9 * v31 + 10;
    this->lpIndices[1][v30 + 8] = v32;
    this->lpIndices[1][v30 + 9] = 9 * v31 + 3;
    this->lpIndices[1][v30 + 10] = v32;
    v33 = 9 * v31 + 3;
    this->lpIndices[1][v30 + 11] = 9 * v31 + 11;
    this->lpIndices[1][v30 + 12] = v33;
    this->lpIndices[1][v30 + 13] = 9 * v31 + 4;
    this->lpIndices[1][v30 + 14] = v33;
    v34 = 9 * v31 + 4;
    this->lpIndices[1][v30 + 15] = 9 * v31 + 12;
    this->lpIndices[1][v30 + 16] = v34;
    this->lpIndices[1][v30 + 17] = 9 * v31 + 5;
    this->lpIndices[1][v30 + 18] = v34;
    v35 = 9 * v31 + 5;
    this->lpIndices[1][v30 + 19] = 9 * v31 + 13;
    this->lpIndices[1][v30 + 20] = v35;
    this->lpIndices[1][v30 + 21] = 9 * v31 + 6;
    this->lpIndices[1][v30 + 22] = v35;
    this->lpIndices[1][v30 + 23] = 9 * v31 + 14;
    this->lpIndices[1][v30 + 24] = xbigc - 1;
    this->lpIndices[1][v30 + 25] = xbigc;
    this->lpIndices[1][v30 + 26] = xbigc - 1;
    v36 = xbigc;
    this->lpIndices[1][v30 + 27] = xbigc - 1 + 9;
    v37 = xbigc + 1;
    this->lpIndices[1][v30 + 28] = xbigc;
    v30 += 32;
    xbigc += 9;
    this->lpIndices[1][v30 - 3] = v37;
    this->lpIndices[1][v30 - 2] = v36;
    this->lpIndices[1][v30 - 1] = v36 + 9;
    v31 = ++gepardc;
  }
  while ( xbigc < 79 );
  *this->lpIndices[2] = 0;
  this->lpIndices[2][1] = 1;
  this->lpIndices[2][2] = 1;
  this->lpIndices[2][3] = 2;
  this->lpIndices[2][4] = 2;
  this->lpIndices[2][5] = 3;
  this->lpIndices[2][6] = 3;
  this->lpIndices[2][7] = 4;
  this->lpIndices[2][8] = 4;
  this->lpIndices[2][9] = 5;
  this->lpIndices[2][10] = 5;
  this->lpIndices[2][11] = 6;
  this->lpIndices[2][12] = 6;
  this->lpIndices[2][13] = 7;
  this->lpIndices[2][14] = 7;
  this->lpIndices[2][15] = 8;
  this->lpIndices[2][16] = 0;
  this->lpIndices[2][17] = 9;
  this->lpIndices[2][18] = 9;
  this->lpIndices[2][19] = 18;
  this->lpIndices[2][20] = 18;
  this->lpIndices[2][21] = 27;
  this->lpIndices[2][22] = 27;
  this->lpIndices[2][23] = 36;
  this->lpIndices[2][24] = 36;
  this->lpIndices[2][25] = 45;
  this->lpIndices[2][26] = 45;
  this->lpIndices[2][27] = 54;
  this->lpIndices[2][28] = 54;
  this->lpIndices[2][29] = 63;
  this->lpIndices[2][30] = 63;
  this->lpIndices[2][31] = 72;
  this->UnlockVertexBuffer();
  this->UnlockIndexBuffer(0);
  this->UnlockIndexBuffer(1);
  this->UnlockIndexBuffer(2);
}

//----- (004509D0) --------------------------------------------------------

SParcel2::~SParcel2()

{
  unsigned short *ParcelVertices; // eax
  ParcelVertices = this->ParcelVertices;
  if ( ParcelVertices )
  {
    delete[] ParcelVertices;
    this->ParcelVertices = 0;
  }
  // ~SMesh called automatically
}

//----- (00450A00) --------------------------------------------------------

SParcel::~SParcel()

{
  // ~SMesh called automatically
}

//----- (00450A90) --------------------------------------------------------

void SParcel::DrawLayered(int bottom_level, int *textures)

{
  int v4;
  int v5;
  int *v6; // eax
  int v7;
  int *bottom_levela;
  this->lpD3DDev->SetStreamSource(0, this->lpVertexBuffer, 0, this->VertexSize);
  this->lpD3DDev->SetFVF(this->VertexFormat);
  this->lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, 3u);
  this->lpD3DDev->SetRenderState(D3DRS_SRCBLEND, 5u);
  this->lpD3DDev->SetRenderState(D3DRS_DESTBLEND, 6u);
  this->lpD3DDev->SetRenderState(D3DRS_ALPHAREF, 0);
  this->lpD3DDev->SetRenderState(D3DRS_ALPHAFUNC, 6u);
  this->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
  this->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
  this->lpD3DDev->SetIndices(this->lpIndexBuffer[0]);
  v4 = bottom_level;
  v7 = textures[bottom_level];
  bottom_levela = &textures[bottom_level];
  this->Gepard->SetTexture(0, v7, 0);
  this->lpD3DDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 81u, 0, 128u);
  this->Gepard->PolyCount += 128;
  if ( v4 < 9 )
  {
    v5 = 81 * v4;
    // x64: was `bottom_levelb = (int)(bottom_levela+1); ... v6 =
    // (int*)bottom_levelb; ... bottom_levelb = (int)v6` register-spill
    // round-trip; truncates an 8-byte int* to int across SetTexture. v6 is
    // never reassigned inside the if-block, so the restore is redundant.
    v6 = bottom_levela + 1;
    do
    {
      if ( this->BlendVisible[v4] )
      {
        this->Gepard->SetTexture(0, *v6, 0);
        this->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 1u);
        this->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 1u);
        this->lpD3DDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, v5, 0, 81u, 0, 128u);
        this->Gepard->PolyCount += 128;
      }
      ++v6;
      v5 += 81;
      ++v4;
    }
    while ( v5 < 729 );
  }
  this->lpD3DDev->SetRenderState(D3DRS_ALPHABLENDENABLE, 0);
  this->lpD3DDev->SetRenderState(D3DRS_ALPHATESTENABLE, 0);
  this->lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, 2u);
}

//----- (00450C40) --------------------------------------------------------

void SParcel::DrawSimple()

{
  this->lpD3DDev->SetStreamSource(0, this->lpVertexBuffer, 0, this->VertexSize);
  this->lpD3DDev->SetFVF(this->VertexFormat);
  this->lpD3DDev->SetIndices(this->lpIndexBuffer[0]);
  this->lpD3DDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 81u, 0, 128u);
}

//----- (00450CA0) --------------------------------------------------------

void SParcel::DrawSketch(int x, int z, int width, int height)

{
  IDirect3DDevice9 *lpD3DDev; // eax
  IDirect3DDevice9 *v7;
  D3DXMATRIX mat;
  lpD3DDev = this->lpD3DDev;
  memset(&mat.m[2][3], 0, 16);
  memset(&mat.m[1][2], 0, 16);
  memset(&mat.m[0][1], 0, 16);
  mat._44 = 1.0;
  mat._33 = 1.0;
  mat._22 = 1.0;
  mat._11 = 1.0;
  lpD3DDev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 2u);
  this->lpD3DDev->SetStreamSource(0, this->lpVertexBuffer, 0, this->VertexSize);
  this->lpD3DDev->SetFVF(this->VertexFormat);
  v7 = this->lpD3DDev;
  mat._11 = 1.0f / (float)width;
  mat._22 = 1.0f / (float)height;
  mat._31 = (float)x * mat._11;
  mat._32 = (float)z * mat._22;
  v7->SetTransform(D3DTS_TEXTURE0, &mat);
  this->lpD3DDev->SetIndices(this->lpIndexBuffer[0]);
  this->lpD3DDev->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, 81u, 0, 128u);
  this->lpD3DDev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, 0);
}

//----- (00450E10) --------------------------------------------------------

void SParcel::DrawWireframe(int index)

{
  unsigned int v3;
  this->lpD3DDev->SetStreamSource(0, this->lpVertexBuffer, 0, this->VertexSize);
  this->lpD3DDev->SetFVF(this->VertexFormat);
  this->lpD3DDev->SetIndices(this->lpIndexBuffer[index]);
  v3 = 128;
  if ( index == 2 )
    v3 = 16;
  this->lpD3DDev->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, 81u, 0, v3);
}

//----- (00450E80) --------------------------------------------------------

void SParcel2::Update(unsigned int *colormap, unsigned char *vismap, int stride, int mode)

{
  SParcel2 *v5; // ebx
  int v6;
  unsigned int *v7; // esi
  int v8;
  unsigned int *v9; // eax
  unsigned int *v10; // edi
  unsigned int *v11; // ebx
  unsigned int v12;
  unsigned char *v13; // ecx
  unsigned int *v14; // ecx
  unsigned int *v15; // ecx
  unsigned int *v16; // ecx
  unsigned int *v17; // ecx
  unsigned int v18;
  unsigned int v19;
  unsigned int *v20; // esi
  int v21;
  unsigned int *v22; // edi
  unsigned int v23;
  unsigned int v24;
  int v25;
  int v26;
  unsigned int v27;
  int v28;
  unsigned int v29;
  int v30;
  unsigned int v31;
  unsigned int v32;
  int v33;
  unsigned int v34;
  int v35;
  unsigned int v36;
  unsigned int v37;
  unsigned int v38;
  unsigned int v39;
  int v40;
  unsigned int v41;
  unsigned int v42;
  int v43;
  unsigned int v44;
  int v45;
  unsigned int v46;
  unsigned int v47;
  int v48;
  unsigned int v49;
  int v50;
  unsigned int v51;
  unsigned int v52;
  int v53;
  unsigned int v54;
  int v55;
  unsigned int v56;
  unsigned int v57;
  int v58;
  unsigned int v59;
  int v60;
  unsigned char *v61; // ecx
  unsigned char *v62; // edi
  unsigned char *v63; // edx
  int v64;
  int v65;
  int v66;
  int v67;
  int v68;
  int v69;
  int v70;
  int v71;
  int v72;
  int v73;
  int v74;
  int v75;
  int v76;
  int v77;
  int v78;
  int v79;
  int v80;
  unsigned char *v81; // ecx
  unsigned int *v82; // ebx
  unsigned char *v83; // edi
  unsigned char *v84; // edx
  unsigned int v85;
  unsigned int v86;
  unsigned int v87;
  int v88;
  unsigned int v89;
  unsigned int v90;
  int v91;
  int v92;
  unsigned int v93;
  int v94;
  int v95;
  unsigned int v96;
  unsigned int v97;
  unsigned int v98;
  int v99;
  int v100;
  unsigned int v101;
  unsigned int v102;
  bool v103; // cc
  unsigned int i;
  int v105;
  int v106;
  int v107;
  int v108;
  _DWORD *v110;
  unsigned int *v111;
  _DWORD *v112;
  _DWORD *v113;
  _DWORD *v114;
  unsigned int *v115;
  _DWORD *v116;
  _DWORD *v117;
  _DWORD *v118;
  unsigned int *v119;
  _DWORD *v120;
  unsigned int *v121;
  unsigned char *v122;
  unsigned int *v123;
  _DWORD *v124;
  unsigned char *v125;
  int v126;
  int v127;
  int v128;
  int v129;
  int v130;
  unsigned char *v131;
  unsigned char *v132;
  unsigned char *v133;
  unsigned char *v134;
  unsigned char *v135;
  unsigned char *v136;
  unsigned char *v137;
  unsigned char *v138;
  int v139;
  int v140;
  int v141;
  int v142;
  int v143;
  int v144;
  unsigned char *v145;
  unsigned char *v146;
  unsigned char *v147;
  unsigned char *v148;
  unsigned char *v149;
  unsigned char *v150;
  unsigned char *v151;
  unsigned char *v152;
  unsigned char *v153;
  unsigned char *v154;
  unsigned char *v155;
  unsigned char *v156;
  unsigned char *v157;
  unsigned char *v158;
  unsigned char *v159;
  unsigned char *v160;
  unsigned char *v161;
  unsigned char *v162;
  unsigned char *v163;
  unsigned char *v164;
  unsigned char *v165;
  unsigned char *v166;
  unsigned char *v167;
  unsigned char *v168;
  unsigned char *v169;
  unsigned char *v170;
  unsigned char *v171;
  unsigned char *v172;
  unsigned char *v173;
  unsigned char *v174;
  unsigned char *v175;
  unsigned char *v176;
  unsigned char *v177;
  unsigned char *v178;
  unsigned char *v179;
  unsigned char *v180;
  unsigned char *v181;
  unsigned char *v182;
  unsigned char *v183;
  unsigned char *v184;
  unsigned char *v185;
  unsigned char *v186;
  unsigned char *v187;
  unsigned char *v188;
  unsigned char *v189;
  unsigned char *v190;
  unsigned char *v191;
  unsigned char *v192;
  unsigned char *v193;
  unsigned char *v194;
  unsigned char *v195;
  unsigned char *v196;
  unsigned char *v197;
  unsigned char *v198;
  unsigned char *v199;
  unsigned char *v200;
  unsigned char *v201;
  unsigned char *v202;
  unsigned char *v203;
  unsigned char *v204;
  unsigned char *v205;
  unsigned char *v206;
  unsigned char *v207;
  unsigned char *v208;
  unsigned char *v209;
  unsigned char *v210;
  unsigned char *v211;
  unsigned char *v212;
  unsigned char *v213;
  unsigned char *v214;
  unsigned char *v215;
  unsigned char *v216;
  unsigned char *v217;
  unsigned char *v218;
  unsigned char *v219;
  unsigned char *v220;
  unsigned char *v221;
  unsigned char *v222;
  unsigned char *v223;
  unsigned char *v224;
  unsigned char *v225;
  unsigned char *v226;
  int v227;
  int v228;
  int v229;
  int v230;
  int v231;
  int v232;
  int v233;
  int v234;
  int v235;
  int v236;
  int v237;
  int v238;
  int v239;
  int v240;
  int v241;
  int v242;
  int v243;
  int v244;
  int v245;
  int v246;
  int v247;
  int v248;
  int v249;
  int v250;
  int v251;
  int v252;
  int v253;
  int v254;
  int v255;
  int v256;
  int v257;
  unsigned int *v258;
  unsigned char *v259;
  unsigned char *v260;
  unsigned int *v261;
  unsigned char *v262;
  unsigned char *v263;
  unsigned char *v264;
  int v265;
  int v266;
  int v267;
  int v268;
  int v269;
  int v270;
  int v271;
  int v272;
  int v273;
  unsigned int *v274;
  unsigned int *v275;
  unsigned char *v276;
  unsigned char *v277;
  int v278;
  int v279;
  int v280;
  int v281;
  int v282;
  int v283;
  int v284;
  int v285;
  int v286;
  int v287;
  int v288;
  int v289;
  int v290;
  int v291;
  unsigned int *colormapa;
  unsigned int *colormapb;
  unsigned int *colormapc;
  unsigned char *colormapd;
  unsigned char *vismapa;
  unsigned char *vismapb;
  unsigned int *stridea;
  unsigned int *strideb;
  unsigned int *stridec;
  unsigned int *modea;
  unsigned int *modeb;
  unsigned int *modec;
  unsigned int *moded;
  v5 = this;
  v6 = 2 * stride;
  if ( mode )
  {
    switch ( mode )
    {
      case 1:
        v20 = colormap;
        v21 = 4 * stride;
        v22 = &FoggedColorMap[1];
        v261 = colormap + 8;
        v258 = colormap + 7;
        v275 = colormap + 6;
        modeb = colormap + 5;
        strideb = colormap + 4;
        colormapb = colormap + 3;
        vismapb = (unsigned char *)(v20 + 2);
        do
        {
          v23 = v20[1];
          *(v22 - 1) = ((80 * (unsigned int)(unsigned char)*v20) >> 7) | (80 * ((*v20 >> 7) & 0x1FFFE00)) & 0xFFFF0000 | (160 * (unsigned char)BYTE1(*v20)) & 0xFFFFFF00;
          v24 = (80 * ((v23 >> 7) & 0x1FFFE00)) & 0xFFFF00FF | (160 * BYTE1(v23));
          v25 = (unsigned char)v23;
          v26 = *(_DWORD *)vismapb;
          v27 = ((unsigned int)(80 * v25) >> 7) | v24 & 0xFFFFFF00;
          v28 = (*(_DWORD *)vismapb >> 7) & 0x1FFFE00;
          *v22 = v27;
          v29 = (160 * *((unsigned char *)v20 + 9)) | (80 * v28) & 0xFFFF00FF;
          v30 = (unsigned char)v26;
          v31 = *colormapb;
          v32 = ((unsigned int)(80 * v30) >> 7) | v29 & 0xFFFFFF00;
          v33 = (*colormapb >> 7) & 0x1FFFE00;
          v22[1] = v32;
          v34 = (160 * *((unsigned char *)v20 + 13)) | (80 * v33) & 0xFFFF00FF;
          v35 = (unsigned char)v31;
          v36 = *strideb;
          v37 = ((unsigned int)(80 * v35) >> 7) | v34 & 0xFFFFFF00;
          v38 = *strideb;
          v22[2] = v37;
          v39 = (160 * *((unsigned char *)v20 + 17)) | (80 * ((v38 >> 7) & 0x1FFFE00)) & 0xFFFF00FF;
          v40 = (unsigned char)v36;
          v41 = *modeb;
          v42 = ((unsigned int)(80 * v40) >> 7) | v39 & 0xFFFFFF00;
          v43 = (*modeb >> 7) & 0x1FFFE00;
          v22[3] = v42;
          v44 = (160 * *((unsigned char *)v20 + 21)) | (80 * v43) & 0xFFFF00FF;
          v45 = (unsigned char)v41;
          v46 = *v275;
          v47 = ((unsigned int)(80 * v45) >> 7) | v44 & 0xFFFFFF00;
          v48 = (*v275 >> 7) & 0x1FFFE00;
          v22[4] = v47;
          v49 = (160 * *((unsigned char *)v20 + 25)) | (80 * v48) & 0xFFFF00FF;
          v50 = (unsigned char)v46;
          v51 = *v258;
          v52 = ((unsigned int)(80 * v50) >> 7) | v49 & 0xFFFFFF00;
          v53 = (*v258 >> 7) & 0x1FFFE00;
          v22[5] = v52;
          v54 = (160 * *((unsigned char *)v20 + 29)) | (80 * v53) & 0xFFFF00FF;
          v55 = (unsigned char)v51;
          v56 = *v261;
          v57 = ((unsigned int)(80 * v55) >> 7) | v54 & 0xFFFFFF00;
          v58 = (*v261 >> 7) & 0x1FFFE00;
          v22[6] = v57;
          v59 = (80 * v58) & 0xFFFF00FF;
          v60 = *((unsigned char *)v20 + 33);
          v20 = (unsigned int *)((char *)v20 + v21);
          vismapb += v21;
          colormapb = (unsigned int *)((char *)colormapb + v21);
          strideb = (unsigned int *)((char *)strideb + v21);
          modeb = (unsigned int *)((char *)modeb + v21);
          v275 = (unsigned int *)((char *)v275 + v21);
          v258 = (unsigned int *)((char *)v258 + v21);
          v261 = (unsigned int *)((char *)v261 + v21);
          v22[7] = ((80 * (unsigned int)(unsigned char)v56) >> 7) | ((160 * v60) | v59) & 0xFFFFFF00;
          v22 += 9;
        }
        while ( (int)v22 < (int)&FoggedColorMap[81] );
        v5 = this;
        break;
      case 2:
        v61 = vismap;
        modec = colormap;
        v122 = &vismap[v6];
        v155 = &vismap[4 * stride];
        v262 = vismap + 15;
        v157 = vismap + 18;
        v159 = &vismap[v6 + 15];
        v161 = &vismap[v6 + 18];
        v163 = &vismap[4 * stride + 16];
        v165 = &vismap[4 * stride + 18];
        v167 = vismap + 16;
        v169 = vismap + 17;
        v171 = &vismap[v6 + 16];
        v173 = &vismap[v6 + 17];
        v175 = vismap + 13;
        v177 = &vismap[v6 + 13];
        v179 = &vismap[4 * stride + 14];
        v181 = vismap + 14;
        v183 = &vismap[v6 + 14];
        v185 = vismap + 11;
        v187 = &vismap[v6 + 11];
        v189 = &vismap[4 * stride + 12];
        v191 = vismap + 12;
        v193 = &vismap[v6 + 12];
        v195 = vismap + 9;
        v197 = &vismap[v6 + 9];
        v199 = &vismap[4 * stride + 10];
        v201 = vismap + 10;
        v203 = &vismap[v6 + 10];
        v205 = vismap + 7;
        v207 = &vismap[v6 + 7];
        v209 = &vismap[4 * stride + 8];
        v211 = vismap + 8;
        v213 = &vismap[v6 + 8];
        v215 = vismap + 5;
        v217 = &vismap[v6 + 5];
        v219 = &vismap[4 * stride + 6];
        v221 = vismap + 6;
        v62 = vismap + 2;
        colormapc = &FoggedColorMap[1];
        v223 = &vismap[v6 + 6];
        v225 = vismap + 3;
        v131 = &vismap[v6 + 3];
        v133 = vismap + 4;
        v135 = &vismap[v6 + 4];
        v259 = vismap + 2;
        v137 = &vismap[4 * stride + 4];
        v145 = vismap - 1;
        v147 = &vismap[v6 + 2];
        v149 = &vismap[v6 - 1];
        v63 = &vismap[v6 + 1];
        v276 = v63;
        v151 = v155 + 2;
        v153 = vismap + 1;
        v124 = modec + 8;
        v264 = &vismap[-v6];
        v110 = modec + 7;
        v112 = modec + 6;
        v114 = modec + 5;
        v116 = modec + 4;
        v118 = modec + 3;
        v120 = modec + 2;
        do
        {
          v267 = *v63;
          v64 = *v62;
          v250 = *v153;
          v139 = *v151;
          v126 = *v147;
          v65 = 2
              * (v139 + v126 + *v145 + *v155 + v264[1] + 40 + 4 * (v267 + v250 + *v122 + *v61) + v64 + *v149 + *v264);
          *(colormapc - 1) = ((v65 * (unsigned int)(unsigned char)*modec) >> 7) | (v65 * ((*modec >> 7) & 0x1FFFE00)) & 0xFFFF0000 | (2 * v65 * (unsigned char)BYTE1(*modec)) & 0xFFFFFF00;
          v285 = *v131;
          v234 = *v225;
          v278 = *v137;
          v242 = *v135;
          v66 = *v133;
          v67 = v66 + v267 + v250 + v139 + v264[3] + 4 * (v285 + v234 + v64 + v126) + 40 + v264[2];
          *colormapc = ((2 * (v278 + v242 + v67) * (unsigned int)(unsigned char)modec[1]) >> 7) | (2 * (v278 + v242 + v67) * ((modec[1] >> 7) & 0x1FFFE00)) & 0xFFFF0000 | (2 * 2 * (v278 + v242 + v67) * (unsigned char)BYTE1(modec[1])) & 0xFFFFFF00;
          v127 = *v217;
          v140 = *v215;
          v268 = *v219;
          v251 = *v223;
          v68 = *v221;
          v69 = 2 * (v285 + v234 + v278 + v268 + v251 + v68 + v264[5] + 4 * (v242 + v66 + v127 + v140) + 40 + v264[4]);
          colormapc[1] = ((v69 * (unsigned int)(unsigned char)*v120) >> 7) | (2 * v69
                                                                                  * *((unsigned char *)modec + 9)) & 0xFFFFFF00 | (v69 * ((*v120 >> 7) & 0x1FFFE00)) & 0xFFFF0000;
          v227 = *v207;
          v243 = *v205;
          v286 = *v209;
          v235 = *v213;
          v70 = *v211;
          v71 = 2 * (v127 + v140 + v268 + v286 + v235 + v70 + v264[7] + 4 * (v251 + v68 + v227 + v243) + 40 + v264[6]);
          colormapc[2] = ((v71 * (unsigned int)(unsigned char)*v118) >> 7) | (v71 * ((*v118 >> 7) & 0x1FFFE00)) & 0xFFFF0000 | (2 * v71 * *((unsigned char *)modec + 13)) & 0xFFFFFF00;
          v128 = *v197;
          v141 = *v195;
          v279 = *v199;
          v252 = *v203;
          v72 = *v201;
          v73 = 2 * (v227 + v243 + v286 + v279 + v252 + v72 + v264[9] + 4 * (v235 + v70 + v128 + v141) + 40 + v264[8]);
          colormapc[3] = ((v73 * (unsigned int)(unsigned char)*v116) >> 7) | (v73 * ((*v116 >> 7) & 0x1FFFE00)) & 0xFFFF0000 | (2 * v73 * *((unsigned char *)modec + 17)) & 0xFFFFFF00;
          v244 = *v187;
          v236 = *v185;
          v287 = *v189;
          v228 = *v193;
          v74 = *v191;
          v75 = 2 * (v128 + v141 + v279 + v287 + v228 + v74 + v264[11] + 4 * (v252 + v72 + v244 + v236) + 40 + v264[10]);
          colormapc[4] = ((v75 * (unsigned int)(unsigned char)*v114) >> 7) | (v75 * ((*v114 >> 7) & 0x1FFFE00)) & 0xFFFF0000 | (2 * v75 * *((unsigned char *)modec + 21)) & 0xFFFFFF00;
          v269 = *v177;
          v129 = *v175;
          v280 = *v179;
          v142 = *v183;
          v253 = *v181;
          v76 = 2
              * (v244 + v236 + v287 + v280 + v142 + v253 + v264[13] + 4 * (v228 + v74 + v269 + v129) + 40 + v264[12]);
          v77 = *v171;
          v78 = *v167;
          colormapc[5] = ((v76 * (unsigned int)(unsigned char)*v112) >> 7) | (v76 * ((*v112 >> 7) & 0x1FFFE00)) & 0xFFFF0000 | (2 * v76 * *((unsigned char *)modec + 25)) & 0xFFFFFF00;
          v245 = *v159;
          v229 = *v163;
          v237 = *v262;
          v79 = v78 + v264[15] + 4 * (v142 + v253 + v237 + v245) + 40 + v264[14];
          colormapc[6] = ((2 * (v269 + v129 + v280 + v229 + v77 + v79) * (unsigned int)(unsigned char)*v110) >> 7) | (2 * (v269 + v129 + v280 + v229 + v77 + v79) * ((*v110 >> 7) & 0x1FFFE00)) & 0xFFFF0000 | (2 * 2 * (v269 + v129 + v280 + v229 + v77 + v79) * *((unsigned char *)modec + 29)) & 0xFFFFFF00;
          v80 = 2
              * (v245 + v237 + *v157 + v264[16] + v264[17] + 40 + 4 * (v77 + v78 + *v173 + *v169) + v229 + *v161 + *v165);
          colormapc[7] = ((v80 * (unsigned int)(unsigned char)*v124) >> 7) | (v80 * ((*v124 >> 7) & 0x1FFFE00)) & 0xFFFF0000 | (2 * v80 * *((unsigned char *)modec + 33)) & 0xFFFFFF00;
          v61 = &vismap[4 * stride];
          v276 += 4 * stride;
          v153 += 4 * stride;
          v151 += 4 * stride;
          v155 += 4 * stride;
          v149 += 4 * stride;
          v147 += 4 * stride;
          v145 += 4 * stride;
          v259 += 4 * stride;
          v137 += 4 * stride;
          v135 += 4 * stride;
          v133 += 4 * stride;
          v131 += 4 * stride;
          v225 += 4 * stride;
          v223 += 4 * stride;
          v221 += 4 * stride;
          v219 += 4 * stride;
          v217 += 4 * stride;
          v215 += 4 * stride;
          v213 += 4 * stride;
          v211 += 4 * stride;
          v209 += 4 * stride;
          v207 += 4 * stride;
          v205 += 4 * stride;
          v203 += 4 * stride;
          v201 += 4 * stride;
          v199 += 4 * stride;
          v197 += 4 * stride;
          v195 += 4 * stride;
          v193 += 4 * stride;
          v191 += 4 * stride;
          v189 += 4 * stride;
          v187 += 4 * stride;
          v185 += 4 * stride;
          v183 += 4 * stride;
          v181 += 4 * stride;
          v179 += 4 * stride;
          v177 += 4 * stride;
          v175 += 4 * stride;
          v173 += 4 * stride;
          v171 += 4 * stride;
          v169 += 4 * stride;
          v167 += 4 * stride;
          v165 += 4 * stride;
          v163 += 4 * stride;
          v161 += 4 * stride;
          v159 += 4 * stride;
          v157 += 4 * stride;
          v262 += 4 * stride;
          v264 += 4 * stride;
          v122 += 4 * stride;
          vismap = v61;
          colormapc += 9;
          v120 += stride;
          v118 += stride;
          v116 += stride;
          v114 += stride;
          v112 += stride;
          v110 += stride;
          v124 += stride;
          modec += stride;
          v62 = v259;
          v63 = v276;
        }
        while ( (int)colormapc < (int)&FoggedColorMap[81] );
        v5 = this;
        break;
      case 3:
        v81 = vismap;
        v82 = colormap;
        v107 = 4 * stride;
        moded = colormap;
        v125 = &vismap[v6];
        v156 = &vismap[4 * stride];
        v108 = 4 * stride - 18;
        v150 = vismap + 15;
        v148 = vismap + 18;
        v146 = &vismap[v6 + 15];
        v138 = &vismap[v6 + 18];
        v136 = &vismap[4 * stride + 16];
        v134 = &vismap[4 * stride + 18];
        v132 = vismap + 16;
        v226 = vismap + 17;
        v224 = &vismap[v6 + 16];
        v222 = &vismap[v6 + 17];
        v220 = vismap + 13;
        v218 = &vismap[v6 + 13];
        v216 = &vismap[4 * stride + 14];
        v214 = vismap + 14;
        v212 = &vismap[v6 + 14];
        v210 = vismap + 11;
        v208 = &vismap[v6 + 11];
        v206 = &vismap[4 * stride + 12];
        v204 = vismap + 12;
        v202 = &vismap[v6 + 12];
        v200 = vismap + 9;
        v198 = &vismap[v6 + 9];
        v196 = &vismap[4 * stride + 10];
        v194 = vismap + 10;
        v192 = &vismap[v6 + 10];
        v190 = vismap + 7;
        v188 = &vismap[v6 + 7];
        v186 = &vismap[4 * stride + 8];
        v184 = vismap + 8;
        v182 = &vismap[v6 + 8];
        v180 = vismap + 5;
        v178 = &vismap[v6 + 5];
        stridec = &FoggedColorMap[1];
        v176 = &vismap[2 * v6 + 6];
        v83 = &vismap[v6 + 2];
        v152 = v83;
        v174 = vismap + 6;
        v172 = &vismap[v6 + 6];
        v170 = vismap + 3;
        v168 = &vismap[v6 + 3];
        v166 = vismap + 4;
        v164 = &vismap[v6 + 4];
        v162 = &vismap[2 * v6 + 4];
        v160 = vismap + 2;
        v158 = vismap - 1;
        v263 = &vismap[v6 - 1];
        v277 = &vismap[v6 + 1];
        v260 = &vismap[2 * v6 + 2];
        v123 = colormap + 8;
        v84 = vismap + 1;
        v154 = vismap + 1;
        colormapd = &vismap[-v6];
        v121 = moded + 7;
        v119 = moded + 6;
        v117 = moded + 5;
        v115 = moded + 4;
        v113 = moded + 3;
        v111 = moded + 2;
        do
        {
          v246 = *v84;
          v281 = *v277;
          v85 = *v82;
          v238 = *v83;
          v288 = *v260;
          v230 = *v160;
          v86 = (unsigned char)v85 | (BYTE1(v85) << 8) | (((v85 >> 7) & 0x1FFFE00)
                                                          * (128
                                                           - ((3
                                                             * (v288
                                                              + v238
                                                              + *v158
                                                              + *v156
                                                              + colormapd[1]
                                                              + 4 * (v281 + v246 + *v125 + *v81)
                                                              + v230
                                                              + *colormapd
                                                              + *v263)) >> 1))) & 0xFFFF0000;
          v87 = moded[1];
          *(stridec - 1) = v86;
          v130 = *v168;
          v143 = *v170;
          v270 = *v162;
          v254 = *v164;
          v88 = *v166;
          v89 = (unsigned char)v87 | (BYTE1(v87) << 8) | (((v87 >> 7) & 0x1FFFE00)
                                                          * (128
                                                           - ((3
                                                             * (v281
                                                              + v246
                                                              + v288
                                                              + v270
                                                              + v254
                                                              + v88
                                                              + colormapd[3]
                                                              + 4 * (v238 + v230 + v130 + v143)
                                                              + colormapd[2])) >> 1))) & 0xFFFF0000;
          v90 = *v111;
          *stridec = v89;
          v239 = *v178;
          v231 = *v180;
          v282 = *v176;
          v289 = *v172;
          v91 = *v174;
          stridec[1] = (unsigned char)v90 | (*((unsigned char *)moded + 9) << 8) | (((v90 >> 7) & 0x1FFFE00)
                                                                                      * (128
                                                                                       - ((3
                                                                                         * (v130
                                                                                          + v143
                                                                                          + v270
                                                                                          + v282
                                                                                          + v289
                                                                                          + v91
                                                                                          + colormapd[5]
                                                                                          + 4
                                                                                          * (v254 + v88 + v239 + v231)
                                                                                          + colormapd[4])) >> 1))) & 0xFFFF0000;
          v255 = *v188;
          v247 = *v190;
          v265 = *v186;
          v271 = *v182;
          v92 = *v184;
          v93 = *v115;
          stridec[2] = (unsigned char)*v113 | (*((unsigned char *)moded + 13) << 8) | (((*v113 >> 7) & 0x1FFFE00)
                                                                                         * (128
                                                                                          - ((3
                                                                                            * (v239
                                                                                             + v231
                                                                                             + v282
                                                                                             + v265
                                                                                             + v271
                                                                                             + v92
                                                                                             + colormapd[7]
                                                                                             + 4
                                                                                             * (v289 + v91 + v255 + v247)
                                                                                             + colormapd[6])) >> 1))) & 0xFFFF0000;
          v240 = *v198;
          v232 = *v200;
          v283 = *v196;
          v290 = *v192;
          v94 = *v194;
          stridec[3] = (unsigned char)v93 | (*((unsigned char *)moded + 17) << 8) | (((v93 >> 7) & 0x1FFFE00)
                                                                                       * (128
                                                                                        - ((3
                                                                                          * (v255
                                                                                           + v247
                                                                                           + v265
                                                                                           + v283
                                                                                           + v290
                                                                                           + v94
                                                                                           + colormapd[9]
                                                                                           + 4
                                                                                           * (v271 + v92 + v240 + v232)
                                                                                           + colormapd[8])) >> 1))) & 0xFFFF0000;
          v256 = *v208;
          v248 = *v210;
          v266 = *v206;
          v272 = *v202;
          v95 = *v204;
          v96 = *v119;
          stridec[4] = (unsigned char)*v117 | (*((unsigned char *)moded + 21) << 8) | (((*v117 >> 7) & 0x1FFFE00)
                                                                                         * (128
                                                                                          - ((3
                                                                                            * (v240
                                                                                             + v232
                                                                                             + v283
                                                                                             + v266
                                                                                             + v272
                                                                                             + v95
                                                                                             + colormapd[11]
                                                                                             + 4
                                                                                             * (v290 + v94 + v256 + v248)
                                                                                             + colormapd[10])) >> 1))) & 0xFFFF0000;
          v284 = *v218;
          v144 = *v220;
          v241 = *v216;
          v291 = *v212;
          v233 = *v214;
          v97 = (unsigned char)v96 | (*((unsigned char *)moded + 25) << 8) | (((v96 >> 7) & 0x1FFFE00)
                                                                                * (128
                                                                                 - ((3
                                                                                   * (v291
                                                                                    + v233
                                                                                    + v256
                                                                                    + v248
                                                                                    + v266
                                                                                    + v241
                                                                                    + colormapd[13]
                                                                                    + 4 * (v272 + v95 + v284 + v144)
                                                                                    + colormapd[12])) >> 1))) & 0xFFFF0000;
          v98 = *v121;
          stridec[5] = v97;
          v257 = *v146;
          v249 = *v150;
          v273 = *v136;
          v99 = *v224;
          v100 = *v132;
          v101 = (unsigned char)v98 | (*((unsigned char *)moded + 29) << 8) | (((v98 >> 7) & 0x1FFFE00)
                                                                                 * (128
                                                                                  - ((3
                                                                                    * (v273
                                                                                     + v99
                                                                                     + v100
                                                                                     + v284
                                                                                     + v144
                                                                                     + v241
                                                                                     + colormapd[15]
                                                                                     + 4 * (v291 + v233 + v257 + v249)
                                                                                     + colormapd[14])) >> 1))) & 0xFFFF0000;
          v102 = *v123;
          stridec[6] = v101;
          v277 += v108 + 18;
          stridec[7] = (unsigned char)v102 | (*((unsigned char *)moded + 33) << 8) | (((v102 >> 7) & 0x1FFFE00)
                                                                                        * (128
                                                                                         - ((3
                                                                                           * (v257
                                                                                            + v249
                                                                                            + *v148
                                                                                            + colormapd[16]
                                                                                            + colormapd[17]
                                                                                            + 4
                                                                                            * (v99 + v100 + *v222 + *v226)
                                                                                            + v273
                                                                                            + *v138
                                                                                            + *v134)) >> 1))) & 0xFFFF0000;
          v152 += v108 + 18;
          v154 += v108 + 18;
          v260 += v108 + 18;
          v81 = &vismap[v108 + 18];
          v263 += v108 + 18;
          v158 += v108 + 18;
          v160 += v108 + 18;
          v162 += v108 + 18;
          v164 += v108 + 18;
          v166 += v108 + 18;
          v168 += v108 + 18;
          v170 += v108 + 18;
          v172 += v108 + 18;
          v174 += v108 + 18;
          v176 += v108 + 18;
          v178 += v108 + 18;
          v180 += v108 + 18;
          v182 += v108 + 18;
          v184 += v108 + 18;
          v186 += v108 + 18;
          v188 += v108 + 18;
          v190 += v108 + 18;
          v192 += v108 + 18;
          v194 += v108 + 18;
          v196 += v108 + 18;
          v198 += v108 + 18;
          v200 += v108 + 18;
          v202 += v108 + 18;
          v204 += v108 + 18;
          v206 += v108 + 18;
          v208 += v108 + 18;
          v210 += v108 + 18;
          v212 += v108 + 18;
          v214 += v108 + 18;
          v216 += v108 + 18;
          v218 += v108 + 18;
          v220 += v108 + 18;
          v222 += v108 + 18;
          v224 += v108 + 18;
          v226 += v108 + 18;
          v132 += v108 + 18;
          v134 += v108 + 18;
          v136 += v108 + 18;
          v138 += v108 + 18;
          v146 += v108 + 18;
          v148 += v108 + 18;
          v150 += v108 + 18;
          v84 = v154;
          v156 += v108 + 18;
          v125 += v108 + 18;
          v82 = &moded[v107 / 4u];
          v111 = (unsigned int *)((char *)v111 + v107);
          v113 = (_DWORD *)((char *)v113 + v107);
          v115 = (unsigned int *)((char *)v115 + v107);
          v117 = (_DWORD *)((char *)v117 + v107);
          v119 = (unsigned int *)((char *)v119 + v107);
          v121 = (unsigned int *)((char *)v121 + v107);
          v123 = (unsigned int *)((char *)v123 + v107);
          v103 = (int)(stridec + 9) < (int)&FoggedColorMap[81];
          stridec += 9;
          v83 = v152;
          vismap = v81;
          colormapd += v108 + 18;
          moded = (unsigned int *)((char *)moded + v107);
        }
        while ( v103 );
        v5 = this;
        break;
      default:
        { for (unsigned int _i = 0; _i < 81; _i++) FoggedColorMap[_i] = 0xFFFFFF; }
        break;
    }
  }
  else
  {
    v7 = colormap;
    v8 = 4 * stride;
    v9 = &FoggedColorMap[1];
    v274 = colormap + 6;
    v10 = colormap + 8;
    modea = colormap + 5;
    v11 = colormap + 7;
    stridea = colormap + 4;
    colormapa = colormap + 3;
    vismapa = (unsigned char *)(v7 + 2);
    do
    {
      *(v9 - 1) = *v7;
      v12 = v7[1];
      v7 = (unsigned int *)((char *)v7 + v8);
      *v9 = v12;
      v13 = vismapa;
      vismapa += v8;
      v9[1] = *(_DWORD *)v13;
      v14 = colormapa;
      colormapa = (unsigned int *)((char *)colormapa + v8);
      v9[2] = *v14;
      v15 = stridea;
      stridea = (unsigned int *)((char *)stridea + v8);
      v9[3] = *v15;
      v16 = modea;
      modea = (unsigned int *)((char *)modea + v8);
      v9[4] = *v16;
      v17 = v274;
      v274 = (unsigned int *)((char *)v274 + v8);
      v9[5] = *v17;
      v18 = *v11;
      v11 = (unsigned int *)((char *)v11 + v8);
      v9[6] = v18;
      v19 = *v10;
      v10 = (unsigned int *)((char *)v10 + v8);
      v9[7] = v19;
      v9 += 9;
    }
    while ( (int)v9 < (int)&FoggedColorMap[81] );
    v5 = this;
  }
  v5->LockVertexBuffer();
  for ( i = 0;
        i < v5->NumVertices;
        *(_DWORD *)((char *)v5->lpVertices + v5->OffsetDiffuse + v105) = FoggedColorMap[(unsigned char)v106]
                                                                       + ((v106 & 0xFFFFFF00) << 16) )
  {
    v105 = i * v5->VertexSize;
    v106 = v5->ParcelVertices[i++];
  }
  v5->UnlockVertexBuffer();
}

//----- (00452640) --------------------------------------------------------

void SParcel::Update( float *heightmap, unsigned char (*blendmap)[9],

        unsigned int *colormap,

        D3DXVECTOR3 *normalmap,

        int stride)

{
  int v6;
  D3DXVECTOR3 *v7; // esi
  float *v8; // ebx
  int v9;
  SParcel *v10; // edi
  int v11;
  float *v12; // eax
  unsigned char *v13; // ebx
  // x64 fix: v14 and v17 hold byte addresses computed from
  // `(char*)lpVertices + offsetN`. The original IDA decomp typed them as
  // `int`, casting the 8-byte lpVertices pointer down to 4 bytes; on x64
  // every normal-vector write at lines 3118-3120 / 3128-3130 went to a
  // phantom low-memory address. Use char* to keep all 64 bits.
  char *v14;
  char *lpVertices; // ecx
  int v16;
  char *v17;
  char *v18; // ecx
  int v19;
  char *v20; // ecx
  char *v21; // ecx
  char *v22; // ecx
  char *v23; // ecx
  char *v24; // ecx
  char *v25; // ecx
  bool v26; // al
  bool v27;
  D3DXVECTOR3 *v28;
  float *v29;
  int v30;
  int v31;
  int v32;
  unsigned int *v33;
  int v34;
  int v35;
  int v36;
  int v38;
  int v39;
  float *v40;
  // heightmapa is the byte distance between heightmap and colormap, used
  // below as `(char*)&normalmapa->x + heightmapa` to convert a colormap-
  // relative pointer to a heightmap-relative one. The IDA decomp stored
  // it in a `float*` then truncated through `(_DWORD)heightmapa` at the
  // use site — correct on x86 (32-bit pointers) but loses the top half of
  // the difference on x64. Use ptrdiff_t so the full signed distance
  // survives.
  ptrdiff_t heightmapa;
  unsigned int *colormapa;
  D3DXVECTOR3 *normalmapa;
  bool stride_3;
  char stride_3a;
  char stride_3b;
  char stride_3c;
  char stride_3d;
  char stride_3e;
  char stride_3f;
  char stride_3g;
  char stride_3h;
  this->LockVertexBuffer();
  v6 = 0;
  v7 = normalmap + 2;
  v35 = 0;
  v28 = normalmap + 2;
  v30 = 9 * stride;
  v31 = 4 * stride;
  v32 = 3 * stride;
  v8 = heightmap + 3;
  v29 = heightmap + 3;
  heightmapa = (char *)heightmap - (char *)colormap;
  v9 = 0;
  v33 = colormap + 1;
  v10 = this;
  v34 = 0;
  do
  {
    v11 = v9;
    stride_3 = 0;
    v38 = v9;
    v39 = v9 + 2;
    v12 = v8;
    normalmapa = (D3DXVECTOR3 *)v33;
    v40 = v8;
    colormapa = (unsigned int *)v7;
    v13 = &(*blendmap)[v6 + 18];
    v36 = 9;
    while ( 1 )
    {
      *(float *)((char *)v10->lpVertices + v10->OffsetXYZ + v11 * v10->VertexSize + 4) = *(v12 - 3);
      v14 = (char *)v10->lpVertices + v38 * v10->VertexSize + v10->OffsetNormal;
      *(_QWORD *)v14 = *(_QWORD *)&v7[-2].x;
      *(float *)(v14 + 8) = v7[-2].z;
      *(_DWORD *)((char *)v10->lpVertices + v38 * v10->VertexSize + v10->OffsetDiffuse) = LODWORD(normalmapa[-1].z) | (*(v13 - 18) << 24);
      if ( stride_3 || (stride_3a = 0, *(v13 - 18)) )
        stride_3a = 1;
      lpVertices = (char *)v10->lpVertices;
      v16 = v10->OffsetXYZ + (v39 - 1) * v10->VertexSize;
      v10 = this;
      *(float *)&lpVertices[v16 + 4] = *(float *)((char *)&normalmapa->x + heightmapa);
      v17 = (char *)v10->lpVertices + (v39 - 1) * v10->VertexSize + v10->OffsetNormal;
      *(_QWORD *)v17 = *(_QWORD *)(colormapa - 3);
      *(_DWORD *)(v17 + 8) = *(colormapa - 1);
      *(_DWORD *)((char *)this->lpVertices + (v39 - 1) * this->VertexSize + this->OffsetDiffuse) = LODWORD(normalmapa->x) | (*(v13 - 9) << 24);
      if ( stride_3a || (stride_3b = 0, *(v13 - 9)) )
        stride_3b = 1;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + v39 * this->VertexSize + 4) = *(v40 - 1);
      v18 = (char *)this->lpVertices + v39 * this->VertexSize + this->OffsetNormal;
      *(_QWORD *)v18 = *(_QWORD *)colormapa;
      *((_DWORD *)v18 + 2) = colormapa[2];
      *(_DWORD *)((char *)this->lpVertices + v39 * this->VertexSize + this->OffsetDiffuse) = LODWORD(normalmapa->y) | (*v13 << 24);
      if ( stride_3b || (stride_3c = 0, *v13) )
        stride_3c = 1;
      v19 = v39 + 1;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + v19 * this->VertexSize + 4) = *v40;
      v20 = (char *)this->lpVertices + (v39 + 1) * this->VertexSize + this->OffsetNormal;
      *(_QWORD *)v20 = *(_QWORD *)(colormapa + 3);
      *((_DWORD *)v20 + 2) = colormapa[5];
      *(_DWORD *)((char *)this->lpVertices + v19 * this->VertexSize + this->OffsetDiffuse) = LODWORD(normalmapa->z) | (v13[9] << 24);
      if ( stride_3c || (stride_3d = 0, v13[9]) )
        stride_3d = 1;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (v39 + 2) * this->VertexSize + 4) = v40[1];
      v21 = (char *)this->lpVertices + (v39 + 2) * this->VertexSize + this->OffsetNormal;
      *(_QWORD *)v21 = *((_QWORD *)colormapa + 3);
      *((_DWORD *)v21 + 2) = colormapa[8];
      *(_DWORD *)((char *)this->lpVertices + (v39 + 2) * this->VertexSize + this->OffsetDiffuse) = LODWORD(normalmapa[1].x) | (v13[18] << 24);
      if ( stride_3d || (stride_3e = 0, v13[18]) )
        stride_3e = 1;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (v39 + 3) * this->VertexSize + 4) = v40[2];
      v22 = (char *)this->lpVertices + (v39 + 3) * this->VertexSize + this->OffsetNormal;
      *(_QWORD *)v22 = *(_QWORD *)(colormapa + 9);
      *((_DWORD *)v22 + 2) = colormapa[11];
      *(_DWORD *)((char *)this->lpVertices + (v39 + 3) * this->VertexSize + this->OffsetDiffuse) = LODWORD(normalmapa[1].y) | (v13[27] << 24);
      if ( stride_3e || (stride_3f = 0, v13[27]) )
        stride_3f = 1;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (v39 + 4) * this->VertexSize + 4) = v40[3];
      v23 = (char *)this->lpVertices + (v39 + 4) * this->VertexSize + this->OffsetNormal;
      *(_QWORD *)v23 = *((_QWORD *)colormapa + 6);
      *((_DWORD *)v23 + 2) = colormapa[14];
      *(_DWORD *)((char *)this->lpVertices + (v39 + 4) * this->VertexSize + this->OffsetDiffuse) = LODWORD(normalmapa[1].z) | (v13[36] << 24);
      if ( stride_3f || (stride_3g = 0, v13[36]) )
        stride_3g = 1;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (v39 + 5) * this->VertexSize + 4) = v40[4];
      v24 = (char *)this->lpVertices + (v39 + 5) * this->VertexSize + this->OffsetNormal;
      *(_QWORD *)v24 = *(_QWORD *)(colormapa + 15);
      *((_DWORD *)v24 + 2) = colormapa[17];
      *(_DWORD *)((char *)this->lpVertices + (v39 + 5) * this->VertexSize + this->OffsetDiffuse) = LODWORD(normalmapa[2].x) | (v13[45] << 24);
      if ( stride_3g || (stride_3h = 0, v13[45]) )
        stride_3h = 1;
      *(float *)((char *)this->lpVertices + this->OffsetXYZ + (v39 + 6) * this->VertexSize + 4) = v40[5];
      v25 = (char *)this->lpVertices + (v39 + 6) * this->VertexSize + this->OffsetNormal;
      *(_QWORD *)v25 = *((_QWORD *)colormapa + 9);
      *((_DWORD *)v25 + 2) = colormapa[20];
      *(_DWORD *)((char *)this->lpVertices + (v39 + 6) * this->VertexSize + this->OffsetDiffuse) = LODWORD(normalmapa[2].y) | (v13[54] << 24);
      v26 = stride_3h || v13[54];
      v11 = v38 + 9;
      v7 = (D3DXVECTOR3 *)&colormapa[v32];
      v39 += 9;
      normalmapa = (D3DXVECTOR3 *)((char *)normalmapa + v31);
      v40 = (float *)((char *)v40 + v31);
      v13 += v30;
      v27 = v36-- == 1;
      stride_3 = v26;
      v38 += 9;
      colormapa = (unsigned int *)((char *)colormapa + v32 * 4);
      if ( v27 )
        break;
      v12 = v40;
    }
    v8 = v29;
    v7 = v28;
    this->BlendVisible[v35] = v26;
    v6 = v35 + 1;
    v9 = v34 + 81;
    ++v35;
    v34 = v9;
  }
  while ( v9 < 729 );
  this->UnlockVertexBuffer();
}

//----- (00452AE0) --------------------------------------------------------

void SParcel2::UpdateFullBright()

{
  unsigned int i;
  int v3;
  int v4;
  this->LockVertexBuffer();
  for ( i = 0;
        i < this->NumVertices;
        *(_DWORD *)((char *)this->lpVertices + this->OffsetDiffuse + v3) = ((v4 & 0xFFFFFF00) << 16) + 0xFFFFFF )
  {
    v3 = i * this->VertexSize;
    v4 = this->ParcelVertices[i++];
  }
  this->UnlockVertexBuffer();
}

