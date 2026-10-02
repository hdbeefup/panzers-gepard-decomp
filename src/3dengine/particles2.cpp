// 3dengine/particles2.cpp
// Particle system 2
// Decompiled from: gameSplit/sparticles.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <math.h>

#include "particles2.h"
#include "gepard.h"
#include "terrain.h"
#include "effects.h"
#include "properties.h"
#include "logger.h"

// Classes: SParticles2
// Function count: 6

//----- (004555F0) --------------------------------------------------------

SParticles2::SParticles2(char *classname, SEffect *effect, SGepard *gepard)
{
  SDrawType Int;
  int v8;
  char *String;
  int v13, v14, v15, v16, v17, v18;
  double v19, v21;
  int v23, v25;
  double v27, v29;
  int Darabszam;
  float Float, v32, v33, v34, v35;
  char texturename[260];
  char fullname[260];

  this->GroupPrototypes.array = 0;
  this->GroupPrototypes.size = 0;
  this->GroupPrototypes.maxsize = 0;
  this->GroupPrototypeQs.array = 0;
  this->GroupPrototypeQs.size = 0;
  this->GroupPrototypeQs.maxsize = 0;
  this->data.array = 0;
  this->data.size = 0;
  this->data.maxsize = 0;
  strncpy(this->ClassName, classname, sizeof(this->ClassName) - 1);
  this->ClassName[sizeof(this->ClassName) - 1] = '\0';
  this->Effect = effect;
  this->Gepard = gepard;
  this->Prototype = 1;
  Int = (SDrawType)effect->EffectsINI->GetInt(this->ClassName, "DrawType", -1);
  this->DrawType = Int;
  if ( Int == (SDrawType)-1 )
    Logger.g->Panic(
      "SParticles2::Initializations: A DrawType nincs megadva, vagy nulla van megadva.");
  v8 = this->Effect->EffectsINI->GetInt(this->ClassName, "MultiTileset", 0);
  this->MultiTileset = v8 != 0;
  String = this->Effect->EffectsINI->GetString(this->ClassName, "Texture", "error");
  strcpy(texturename, String);
  v13 = strcmp(texturename, "error");
  if ( v13 )
    v13 = v13 < 0 ? -1 : 1;
  if ( !v13 )
    Logger.g->Panic(
      "SParticles2::SParticles2: no texture variable defined in class: \"%s\"",
      this->ClassName);
#ifdef HDB_MISSING_ASSET_FALLBACK
  #define HDB_LOADTEX LoadTextureOrPlaceholder
#else
  #define HDB_LOADTEX LoadTexture
#endif
  if ( this->MultiTileset )
  {
    sprintf(fullname, "effects/tileset_normal/%s", texturename);
    v14 = this->Gepard->HDB_LOADTEX(fullname, 1, 1);
    this->THandle0 = v14;
    if ( v14 == -1 )
      Logger.g->Panic("SParticles2::SParticles2: cannot load texture \"%s\"", fullname);
    sprintf(fullname, "effects/tileset_desert/%s", texturename);
    v15 = this->Gepard->HDB_LOADTEX(fullname, 1, 1);
    this->THandle1 = v15;
    if ( v15 == -1 )
      Logger.g->Panic("SParticles2::SParticles2: cannot load texture \"%s\"", fullname);
    sprintf(fullname, "effects/tileset_snow/%s", texturename);
    v16 = this->Gepard->HDB_LOADTEX(fullname, 1, 1);
    this->THandle2 = v16;
    if ( v16 == -1 )
      Logger.g->Panic("SParticles2::SParticles2: cannot load texture \"%s\"", fullname);
    sprintf(fullname, "effects/tileset_rock/%s", texturename);
    v17 = this->Gepard->HDB_LOADTEX(fullname, 1, 1);
    this->THandle3 = v17;
    if ( v17 == -1 )
      Logger.g->Panic("SParticles2::SParticles2: cannot load texture \"%s\"", fullname);
    sprintf(fullname, "effects/tileset_jungle/%s", texturename);
    v18 = this->Gepard->HDB_LOADTEX(fullname, 1, 1);
    this->THandle4 = v18;
  }
  else
  {
    sprintf(fullname, "effects/%s", texturename);
    v18 = this->Gepard->HDB_LOADTEX(fullname, 1, 1);
    this->THandle = v18;
  }
#undef HDB_LOADTEX
  if ( v18 == -1 )
    Logger.g->Panic("SParticles2::SParticles2: cannot load texture \"%s\"", fullname);
  Float = this->Effect->EffectsINI->GetFloat(this->ClassName, "HSpeed", -1.0f);
  this->HSpeed = Float;
  if ( Float == -1.0f )
    Logger.g->Panic("SParticles2::Initializations: A HSpeed nincs megadva, vagy nulla van megadva.");
  v32 = this->Effect->EffectsINI->GetFloat(this->ClassName, "HSpeed_Rnd", -1.0f);
  this->HSpeed_Rnd = v32;
  if ( v32 == -1.0f )
    Logger.g->Panic(
      "SParticles2::Initializations: A HSpeed_Rnd nincs megadva, vagy nulla van megadva.");
  v33 = this->Effect->EffectsINI->GetFloat(this->ClassName, "KisG", -1.0f);
  this->KisG = v33;
  if ( v33 == -1.0f )
    Logger.g->Panic("SParticles2::Initializations: A KisG nincs megadva, vagy nulla van megadva.");
  v34 = this->Effect->EffectsINI->GetFloat(this->ClassName, "VSpeed_Rnd", -1.0f);
  this->VSpeed_Rnd = v34;
  if ( v34 == -1.0f )
    Logger.g->Panic(
      "SParticles2::Initializations: A VSpeed_Rnd nincs megadva, vagy nulla van megadva.");
  v35 = this->Effect->EffectsINI->GetFloat(this->ClassName, "VSpeed", -1.0f);
  this->VSpeed = v35;
  if ( v35 == -1.0f )
    Logger.g->Panic("SParticles2::Initializations: A VSpeed nincs megadva, vagy nulla van megadva.");
  v19 = this->Effect->EffectsINI->GetFloat(this->ClassName, "FadeOutSpeed", 0.0f);
  this->FadeOutSpeed = (float)v19;
  v21 = this->Effect->EffectsINI->GetFloat(this->ClassName, "FadeOutSpeed_Rnd", 0.0f);
  this->FadeOutSpeed_Rnd = (float)v21;
  v23 = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "Talaj_Koszolas", 0.0f);
  this->TalajKoszolas = v23;
  v25 = this->Effect->EffectsINI->GetInt(this->ClassName, "Darabszam", 5);
  this->Darabszam = v25;
  v27 = this->Effect->EffectsINI->GetFloat(this->ClassName, "Scale", 0.5f);
  this->Scale = (float)v27;
  v29 = this->Effect->EffectsINI->GetFloat(this->ClassName, "Scale_Rnd", 0.5f);
  Darabszam = this->Darabszam;
  this->ScaleRnd = (float)v29;
  if ( Darabszam < 1024 )
    Darabszam = 1024;
  this->MaxParticles = Darabszam;
  this->DynamicVB = this->Gepard->CreateDynamicVB(0x1C4u, 12 * Darabszam);
}

//----- (00455BF0) --------------------------------------------------------

SParticles2::SParticles2(SParticles2 *p, float _x, float _y, float _z)
{
  SGepard *Gepard;
  bool MultiTileset;
  int THandle0;
  int v14;
  int Darabszam;
  SDrawType DrawType;
  int maxsize;
  int v19;
  SParticles2Data *v20;
  int v21;
  int size;
  float v23;
  float v27;
  float _xc, _xd, _xb, _xe, _xf;
  float angle_rad;

  this->GroupPrototypes.array = 0;
  this->GroupPrototypes.size = 0;
  this->GroupPrototypes.maxsize = 0;
  this->GroupPrototypeQs.array = 0;
  this->GroupPrototypeQs.size = 0;
  this->GroupPrototypeQs.maxsize = 0;
  this->data.array = 0;
  this->data.size = 0;
  this->data.maxsize = 0;
  this->sz = 0;
  strncpy(this->ClassName, p->ClassName, sizeof(this->ClassName) - 1);
  this->ClassName[sizeof(this->ClassName) - 1] = '\0';
  this->Effect = p->Effect;
  Gepard = p->Gepard;
  this->Gepard = Gepard;
  this->World = p->World;
  this->Color = (int)(float)(Gepard->EnvironmentColorVal.b * 255.0f) | (((int)(float)(Gepard->EnvironmentColorVal.g
                                                                                   * 255.0f) | ((int)(float)(Gepard->EnvironmentColorVal.r * 255.0f) << 8)) << 8);
  MultiTileset = p->MultiTileset;
  this->MultiTileset = MultiTileset;
  if ( MultiTileset )
  {
    switch ( Gepard->Tileset )
    {
      case 0:
        THandle0 = p->THandle0;
        goto LABEL_11;
      case 1:
        THandle0 = p->THandle1;
        goto LABEL_11;
      case 2:
        THandle0 = p->THandle2;
        goto LABEL_11;
      case 3:
        THandle0 = p->THandle3;
        goto LABEL_11;
      case 4:
        THandle0 = p->THandle4;
        goto LABEL_11;
      default:
        break;
    }
  }
  else
  {
    THandle0 = p->THandle;
LABEL_11:
    this->THandle = THandle0;
  }
  v14 = 0;
  this->FadeOutSpeed = p->FadeOutSpeed;
  this->FadeOutSpeed_Rnd = p->FadeOutSpeed_Rnd;
  Darabszam = p->Darabszam;
  this->Darabszam = Darabszam;
  this->Scale = p->Scale;
  this->ScaleRnd = p->ScaleRnd;
  this->HSpeed = p->HSpeed;
  this->HSpeed_Rnd = p->HSpeed_Rnd;
  this->KisG = p->KisG;
  this->VSpeed = p->VSpeed;
  this->VSpeed_Rnd = p->VSpeed_Rnd;
  this->TalajKoszolas = p->TalajKoszolas;
  DrawType = p->DrawType;
  this->MainX = _x;
  this->MainY = _y;
  this->DrawType = DrawType;
  this->Prototype = 0;
  this->MainZ = _z;
  if ( Darabszam > 0 )
  {
    for (int ci = 0; ci < Darabszam; ci++)
    {
      maxsize = this->data.maxsize;
      if ( this->data.size == maxsize )
      {
        if ( maxsize >= 16 )
          v19 = 6 * maxsize / 5;
        else
          v19 = 16;
        v20 = (SParticles2Data *)realloc(this->data.array, 48 * v19);
        v21 = this->data.maxsize;
        this->data.array = v20;
        memset(&v20[v21], 0, 48 * (v19 - v21));
        this->data.maxsize = v19;
      }
      size = this->data.size;
      this->data.size = size + 1;
      if ( size != ci )
        Logger.g->Panic("SParticles2::Initializations: data.Add()!=ci");
      _xc = this->VSpeed_Rnd;
      this->data.array[ci].yspeed = this->VSpeed
                                  + (float)((float)rand() / 32767.0f) * _xc;
      this->data.array[ci].kisg = this->KisG;
      _xd = this->HSpeed_Rnd;
      _xb = (float)((float)rand() / 32767.0f) * _xd + this->HSpeed;
      v23 = (float)(360.0f - (float)((float)rand() / 32767.0f) * 360.0f) + 90.0f;
      while ( v23 < 0.0f ) v23 += 360.0f;
      while ( v23 >= 360.0f ) v23 -= 360.0f;
      angle_rad = v23 * 0.017453292f;
      this->data.array[ci].vectorx = cosf(angle_rad) * _xb;
      this->data.array[ci].vectorz = sinf(angle_rad) * _xb;
      this->data.array[ci].x = this->MainX;
      this->data.array[ci].y = this->MainY;
      this->data.array[ci].z = this->MainZ;
      _xe = this->ScaleRnd;
      this->data.array[ci].scale = (float)((float)rand() / 32767.0f) * _xe + this->Scale;
      this->data.array[ci].alpha = 1.0f;
      _xf = this->FadeOutSpeed_Rnd;
      v27 = (float)((float)rand() / 32767.0f) * _xf;
      this->data.array[ci].fadeoutspeed = v27 + this->FadeOutSpeed;
      this->data.array[ci].active = 1;
    }
  }
  this->DynamicVB = p->DynamicVB;
}

//----- (004560A0) --------------------------------------------------------

SParticles2::~SParticles2()
{
  if ( this->Prototype )
  {
    if ( this->MultiTileset )
    {
      this->Gepard->ReleaseTexture(this->THandle0, 0);
      this->Gepard->ReleaseTexture(this->THandle1, 0);
      this->Gepard->ReleaseTexture(this->THandle2, 0);
      this->Gepard->ReleaseTexture(this->THandle3, 0);
      this->Gepard->ReleaseTexture(this->THandle4, 0);
    }
    else
    {
      this->Gepard->ReleaseTexture(this->THandle, 0);
    }
    this->Gepard->RemoveDynamicVB(this->DynamicVB);
  }
  if ( this->data.array )
  {
    free(this->data.array);
    this->data.array = 0;
  }
  if ( this->GroupPrototypeQs.array )
  {
    free(this->GroupPrototypeQs.array);
    this->GroupPrototypeQs.array = 0;
  }
  if ( this->GroupPrototypes.array )
  {
    free(this->GroupPrototypes.array);
    this->GroupPrototypes.array = 0;
  }
}

//----- (004561B0) --------------------------------------------------------

char SParticles2::MoveParticles(int a2)
{
  int v3;
  char v4;
  int Darabszam;
  float v6;
  bool v7;
  SParticles2Data *array;
  float yspeed;
  float v10;
  SParticles2Data *v11;
  STerrain *Terrain;
  double Height;
  unsigned char *v14;
  int v16;
  float xpos, ypos, scale;
  float zbuf_f, rhw_f;
  unsigned int fog;
  float v36, v37;
  char v38;

  v3 = 0;
  v4 = 0;
  v38 = 0;
  Darabszam = this->Darabszam;
  v6 = (float)this->Gepard->ElapsedTime * 0.001f;
  xpos = v6;
  v7 = Darabszam == 0;
  if ( Darabszam > 0 )
  {
    a2 = 0;
    do
    {
      array = this->data.array;
      if ( array[a2].active )
      {
        v4 = 1;
        yspeed = array[a2].yspeed;
        v10 = array[a2].kisg * v6;
        v38 = 1;
        array[a2].yspeed = yspeed - v10;
        this->data.array[a2].x = (float)(this->data.array[a2].vectorx * v6) + this->data.array[a2].x;
        this->data.array[a2].z = (float)(this->data.array[a2].vectorz * v6) + this->data.array[a2].z;
        this->data.array[a2].y = (float)(this->data.array[a2].yspeed * v6) + this->data.array[a2].y;
        this->data.array[a2].alpha = this->data.array[a2].alpha - (float)(this->data.array[a2].fadeoutspeed * v6);
        v11 = this->data.array;
        if ( v11[a2].alpha <= 0.05f
          || (Terrain = this->Gepard->Terrain,
              v37 = v11[a2].y + 0.60000002f,
              Height = Terrain->GetHeight(v11[a2].x, v11[a2].z),
              v4 = v38,
              v36 = (float)Height,
              v36 > v37) )
        {
          this->data.array[a2].alpha = 0.0f;
          this->data.array[a2].active = 0;
        }
      }
      else
      {
        v4 = v38;
      }
      Darabszam = this->Darabszam;
      ++v3;
      v6 = xpos;
      ++a2;
    }
    while ( v3 < Darabszam );
    v7 = Darabszam == 0;
  }
  if ( v7 )
    return v4;
  v14 = this->Gepard->LockDynamicVB(v3, (int)this, a2 * 48, this->DynamicVB, 6 * Darabszam);
  int vertCount = 0;
  v16 = 0;
  if ( this->Darabszam > 0 )
  {
    unsigned char *pVert = v14;
    do
    {
      SParticles2Data *pd = &this->data.array[v16];
      if ( pd->active )
      {
        this->Gepard->TransformScaledPointBlend(
          pd->x, pd->y, pd->z, pd->scale,
          &xpos, &ypos, &scale, &zbuf_f, &rhw_f, &fog);
        this->Gepard->SetDrawType(DT_NORMAL);
        this->Gepard->SetTexture(0, this->THandle, 1);

        // Write 6 transformed vertices (2 triangles for a quad billboard)
        struct TLVertex {
          float x, y, z, rhw;
          unsigned int color;
          unsigned int specular;
          float u, v;
        };
        TLVertex *verts = (TLVertex *)pVert;
        float left = xpos - scale;
        float right = xpos + scale;
        float top = ypos - scale;
        float bottom = ypos + scale;
        unsigned int zbuf = *(unsigned int*)&zbuf_f;
        unsigned int rhw = *(unsigned int*)&rhw_f;

        // Vertex 0: top-left
        verts[0].x = left;  verts[0].y = top;
        verts[0].z = zbuf_f; verts[0].rhw = rhw_f;
        verts[0].color = this->Color; verts[0].specular = fog;
        verts[0].u = 0.0f; verts[0].v = 0.0f;

        // Vertex 1: bottom-left
        verts[1].x = left;  verts[1].y = bottom;
        verts[1].z = zbuf_f; verts[1].rhw = rhw_f;
        verts[1].color = this->Color; verts[1].specular = fog;
        verts[1].u = 0.0f; verts[1].v = 1.0f;

        // Vertex 2: top-right
        verts[2].x = right; verts[2].y = top;
        verts[2].z = zbuf_f; verts[2].rhw = rhw_f;
        verts[2].color = this->Color; verts[2].specular = fog;
        verts[2].u = 1.0f; verts[2].v = 0.0f;

        // Vertex 3: top-right (same as 2)
        verts[3].x = right; verts[3].y = top;
        verts[3].z = zbuf_f; verts[3].rhw = rhw_f;
        verts[3].color = this->Color; verts[3].specular = fog;
        verts[3].u = 1.0f; verts[3].v = 0.0f;

        // Vertex 4: bottom-left (same as 1)
        verts[4].x = left;  verts[4].y = bottom;
        verts[4].z = zbuf_f; verts[4].rhw = rhw_f;
        verts[4].color = this->Color; verts[4].specular = fog;
        verts[4].u = 0.0f; verts[4].v = 1.0f;

        // Vertex 5: bottom-right
        verts[5].x = right; verts[5].y = bottom;
        verts[5].z = zbuf_f; verts[5].rhw = rhw_f;
        verts[5].color = this->Color; verts[5].specular = fog;
        verts[5].u = 1.0f; verts[5].v = 1.0f;

        pVert += 6 * sizeof(TLVertex);
        ++vertCount;
      }
      ++v16;
    }
    while ( v16 < this->Darabszam );
  }
  this->Gepard->UnlockDynamicVB(this->DynamicVB);
  this->Gepard->DrawDynamicVB(this->DynamicVB, 2 * vertCount);
  return v38;
}

//----- (00456590) --------------------------------------------------------

int SParticles2::PrecacheObject(char *variable_name, SDrawType drawtype)
{
  char *String;
  char tempfilename[260];
  char fullpathandname[260];

  String = this->Effect->EffectsINI->GetString(this->ClassName, variable_name, "error");
  strcpy(tempfilename, String);
  strcpy(fullpathandname, "effects\\");
  strcat(fullpathandname, tempfilename);
  return this->Gepard->PrecacheObject(fullpathandname, 0.0f, drawtype);
}

//----- (00456660) --------------------------------------------------------

void SParticles2::SetPosition(float _x, float _y, float _z)
{
  this->MainX = _x;
  this->MainY = _y;
  this->MainZ = _z;
}
