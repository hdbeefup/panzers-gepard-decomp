// 3dengine/particles.cpp
// Particle system 1 (3D mesh-based particles with debris, explosions, smoke trails)
// Decompiled from: gameSplit/sparticles.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <math.h>
#include <stdlib.h>

#include "particles.h"
#include "gepard.h"
#include "terrain.h"
#include "effects.h"
#include "properties.h"
#include "logger.h"

// Concert global (sound system) — now defined in effects.h

// SIObject virtual methods used here
// SIObject — use full definition from iobject.h for correct vtable layout
#include "iobject.h"

// Classes: SParticles
// Function count: 9

//----- (00452B30) --------------------------------------------------------

SParticles::SParticles(char *classname, SEffect *effect, SGepard *gepard)
{
  char MeshTxt[32];
  char MeshTxt2[4];
  char tmpstr[260];
  char fn[260];

  this->GroupPrototypes.array = 0;
  this->GroupPrototypes.size = 0;
  this->GroupPrototypes.maxsize = 0;
  this->GroupPrototypeQs.array = 0;
  this->GroupPrototypeQs.size = 0;
  this->GroupPrototypeQs.maxsize = 0;
  this->GroupPrototypes0.array = 0;
  this->GroupPrototypes0.size = 0;
  this->GroupPrototypes0.maxsize = 0;
  this->GroupPrototypeQs0.array = 0;
  this->GroupPrototypeQs0.size = 0;
  this->GroupPrototypeQs0.maxsize = 0;
  this->GroupPrototypes1.array = 0;
  this->GroupPrototypes1.size = 0;
  this->GroupPrototypes1.maxsize = 0;
  this->GroupPrototypeQs1.array = 0;
  this->GroupPrototypeQs1.size = 0;
  this->GroupPrototypeQs1.maxsize = 0;
  this->GroupPrototypes2.array = 0;
  this->GroupPrototypes2.size = 0;
  this->GroupPrototypes2.maxsize = 0;
  this->GroupPrototypeQs2.array = 0;
  this->GroupPrototypeQs2.size = 0;
  this->GroupPrototypeQs2.maxsize = 0;
  this->GroupPrototypes3.array = 0;
  this->GroupPrototypes3.size = 0;
  this->GroupPrototypes3.maxsize = 0;
  this->GroupPrototypeQs3.array = 0;
  this->GroupPrototypeQs3.size = 0;
  this->GroupPrototypeQs3.maxsize = 0;
  this->GroupPrototypes4.array = 0;
  this->GroupPrototypes4.size = 0;
  this->GroupPrototypes4.maxsize = 0;
  this->GroupPrototypeQs4.array = 0;
  this->GroupPrototypeQs4.size = 0;
  this->GroupPrototypeQs4.maxsize = 0;
  this->data.array = 0;
  this->data.size = 0;
  this->data.maxsize = 0;
  strncpy(this->ClassName, classname, sizeof(this->ClassName) - 1);
  this->ClassName[sizeof(this->ClassName) - 1] = '\0';
  this->Effect = effect;
  this->Gepard = gepard;
  this->Prototype = 1;
  SDrawType Int = (SDrawType)effect->EffectsINI->GetInt(this->ClassName, "DrawType", -1);
  this->DrawType = Int;
  if ( Int == (SDrawType)-1 )
    Logger.g->Panic("SParticles::Initializations: A DrawType nincs megadva, vagy -1 van megadva.");
  int multiTileset = this->Effect->EffectsINI->GetInt(this->ClassName, "MultiTileset", 0);
  this->MultiTileset = multiTileset != 0;
  if ( multiTileset )
  {
    for ( int tileset = 0; tileset <= 4; tileset++ )
    {
      int size = 0;
      this->OsszesDarabszam = 0;
      while ( 1 )
      {
        switch ( tileset )
        {
          case 0: size = this->GroupPrototypes0.size; break;
          case 1: size = this->GroupPrototypes1.size; break;
          case 2: size = this->GroupPrototypes2.size; break;
          case 3: size = this->GroupPrototypes3.size; break;
          default: size = this->GroupPrototypes4.size; break;
        }
        int meshIdx = size;
        strcpy(MeshTxt, "Mesh");
        _itoa(size + 1, MeshTxt2, 10);
        strcat(MeshTxt, MeshTxt2);
        int cachedObj = this->PrecacheObject(MeshTxt, this->DrawType, tileset);
        if ( cachedObj == -1 )
          break;
        strcpy(MeshTxt, "Mesh");
        _itoa(size + 1, MeshTxt2, 10);
        strcat(MeshTxt, MeshTxt2);
        strcat(MeshTxt, "Q");
        int qty = this->Effect->EffectsINI->GetInt(this->ClassName, MeshTxt, 1);
        switch ( tileset )
        {
          case 0:
            this->GroupPrototypes0.Add();
            this->GroupPrototypes0.array[meshIdx] = cachedObj;
            this->GroupPrototypeQs0.Add();
            this->GroupPrototypeQs0.array[meshIdx] = qty;
            break;
          case 1:
            this->GroupPrototypes1.Add();
            this->GroupPrototypes1.array[meshIdx] = cachedObj;
            this->GroupPrototypeQs1.Add();
            this->GroupPrototypeQs1.array[meshIdx] = qty;
            break;
          case 2:
            this->GroupPrototypes2.Add();
            this->GroupPrototypes2.array[meshIdx] = cachedObj;
            this->GroupPrototypeQs2.Add();
            this->GroupPrototypeQs2.array[meshIdx] = qty;
            break;
          case 3:
            this->GroupPrototypes3.Add();
            this->GroupPrototypes3.array[meshIdx] = cachedObj;
            this->GroupPrototypeQs3.Add();
            this->GroupPrototypeQs3.array[meshIdx] = qty;
            break;
          case 4:
            this->GroupPrototypes4.Add();
            this->GroupPrototypes4.array[meshIdx] = cachedObj;
            this->GroupPrototypeQs4.Add();
            this->GroupPrototypeQs4.array[meshIdx] = qty;
            break;
        }
        this->OsszesDarabszam += qty;
      }
      if ( !size )
        Logger.g->Panic("SParticles::SParticles: 0 db 4d file lett megadva a %s-ben!", this->ClassName);
    }
  }
  else
  {
    this->OsszesDarabszam = 0;
    while ( 1 )
    {
      int meshIdx = this->GroupPrototypes.size;
      strcpy(MeshTxt, "Mesh");
      _itoa(meshIdx + 1, MeshTxt2, 10);
      strcat(MeshTxt, MeshTxt2);
      int cachedObj = this->PrecacheObject(MeshTxt, this->DrawType, -1);
      if ( cachedObj == -1 )
        break;
      strcpy(MeshTxt, "Mesh");
      _itoa(meshIdx + 1, MeshTxt2, 10);
      strcat(MeshTxt, MeshTxt2);
      strcat(MeshTxt, "Q");
      int qty = this->Effect->EffectsINI->GetInt(this->ClassName, MeshTxt, 1);
      this->GroupPrototypes.Add();
      this->GroupPrototypes.array[meshIdx] = cachedObj;
      this->GroupPrototypeQs.Add();
      this->GroupPrototypeQs.array[meshIdx] = qty;
      this->OsszesDarabszam += qty;
    }
    if ( !this->GroupPrototypes.size )
      Logger.g->Panic("SParticles::SParticles: 0 db 4d file lett megadva a %s-ben!", this->ClassName);
  }

  char *String = this->Effect->EffectsINI->GetString(this->ClassName, "Trail_Texture", "error");
  strcpy(tmpstr, String);
  if ( strcmp(tmpstr, "error") )
  {
    strcpy(fn, "effects\\");
    strcat(fn, tmpstr);
#ifdef HDB_MISSING_ASSET_FALLBACK
    int v60 = this->Gepard->LoadTextureOrPlaceholder(fn, 1, 1);
#else
    int v60 = this->Gepard->LoadTexture(fn, 1, 1);
#endif
    this->THandle = v60;
    if ( v60 == -1 )
      Logger.g->Panic("SParticles::SParticles: cannot load texture \"%s\"", fn);
  }
  else
  {
    this->THandle = -1;
  }
  float hspeed = this->Effect->EffectsINI->GetFloat(this->ClassName, "HSpeed", -1.0f);
  this->HSpeed = hspeed;
  if ( hspeed == -1.0f )
    Logger.g->Panic("SParticles::Initializations: A HSpeed nincs megadva, vagy -1 van megadva.");
  float hspeed_rnd = this->Effect->EffectsINI->GetFloat(this->ClassName, "HSpeed_Rnd", -1.0f);
  this->HSpeed_Rnd = hspeed_rnd;
  if ( hspeed_rnd == -1.0f )
    Logger.g->Panic("SParticles::Initializations: A HSpeed_Rnd nincs megadva, vagy -1 van megadva.");
  float kisg = this->Effect->EffectsINI->GetFloat(this->ClassName, "KisG", -1.0f);
  this->KisG = kisg;
  if ( kisg == -1.0f )
    Logger.g->Panic("SParticles::Initializations: A KisG nincs megadva, vagy -1 van megadva.");
  float vspeed_rnd = this->Effect->EffectsINI->GetFloat(this->ClassName, "VSpeed_Rnd", -1.0f);
  this->VSpeed_Rnd = vspeed_rnd;
  if ( vspeed_rnd == -1.0f )
    Logger.g->Panic("SParticles::Initializations: A VSpeed_Rnd nincs megadva, vagy -1 van megadva.");
  float vspeed = this->Effect->EffectsINI->GetFloat(this->ClassName, "VSpeed", -1.0f);
  this->VSpeed = vspeed;
  if ( vspeed == -1.0f )
    Logger.g->Panic("SParticles::Initializations: A VSpeed nincs megadva, vagy -1 van megadva.");
  float zrotspeed = this->Effect->EffectsINI->GetFloat(this->ClassName, "ZRotSpeed", -1.0f);
  this->ZRotSpeed = zrotspeed;
  if ( zrotspeed == -1.0f )
    Logger.g->Panic("SParticles::Initializations: A ZRotSpeed nincs megadva, vagy -1 van megadva.");
  float zrotspeed_rnd = this->Effect->EffectsINI->GetFloat(this->ClassName, "ZRotSpeed_Rnd", -1.0f);
  this->ZRotSpeed_Rnd = zrotspeed_rnd;
  if ( zrotspeed_rnd == -1.0f )
    Logger.g->Panic(
      "SParticles::Initializations: A ZRotSpeed_Rnd nincs megadva, vagy -1 van megadva.");
  this->Trail_Color = this->Effect->EffectsINI->GetInt(this->ClassName, "Trail_Color", 0);
  this->Trail_Strength = this->Effect->EffectsINI->GetFloat(this->ClassName, "Trail_Strength", 1.0f);
  this->Trail_FadeSpeed = this->Effect->EffectsINI->GetFloat(this->ClassName, "Trail_FadeSpeed", 1.0f);
  this->Trail_Scale = this->Effect->EffectsINI->GetFloat(this->ClassName, "Trail_Scale", 1.0f);
  this->FadeOutSpeed = this->Effect->EffectsINI->GetFloat(this->ClassName, "FadeOutSpeed", 0.0f);
  this->FadeOutSpeed_Rnd = this->Effect->EffectsINI->GetFloat(this->ClassName, "FadeOutSpeed_Rnd", 0.0f);
  this->TimeIntervall = this->Effect->EffectsINI->GetFloat(this->ClassName, "TimeIntervall", 0.0f);
  this->TalajKoszolas = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "Talaj_Koszolas", 0.0f);
  this->RandomX = this->Effect->EffectsINI->GetFloat(this->ClassName, "RandomX", 0.0f);
  this->RandomY = this->Effect->EffectsINI->GetFloat(this->ClassName, "RandomY", 0.0f);
  this->RandomZ = this->Effect->EffectsINI->GetFloat(this->ClassName, "RandomZ", 0.0f);
  this->BoomAfter = this->Effect->EffectsINI->GetInt(this->ClassName, "BoomAfter", 0);
}

//----- (00453750) --------------------------------------------------------

SParticles::SParticles(SParticles *p, float _x, float _y, float _z)
{
  this->GroupPrototypes.array = 0;
  this->GroupPrototypes.size = 0;
  this->GroupPrototypes.maxsize = 0;
  this->GroupPrototypeQs.array = 0;
  this->GroupPrototypeQs.size = 0;
  this->GroupPrototypeQs.maxsize = 0;
  this->GroupPrototypes0.array = 0;
  this->GroupPrototypes0.size = 0;
  this->GroupPrototypes0.maxsize = 0;
  this->GroupPrototypeQs0.array = 0;
  this->GroupPrototypeQs0.size = 0;
  this->GroupPrototypeQs0.maxsize = 0;
  this->GroupPrototypes1.array = 0;
  this->GroupPrototypes1.size = 0;
  this->GroupPrototypes1.maxsize = 0;
  this->GroupPrototypeQs1.array = 0;
  this->GroupPrototypeQs1.size = 0;
  this->GroupPrototypeQs1.maxsize = 0;
  this->GroupPrototypes2.array = 0;
  this->GroupPrototypes2.size = 0;
  this->GroupPrototypes2.maxsize = 0;
  this->GroupPrototypeQs2.array = 0;
  this->GroupPrototypeQs2.size = 0;
  this->GroupPrototypeQs2.maxsize = 0;
  this->GroupPrototypes3.array = 0;
  this->GroupPrototypes3.size = 0;
  this->GroupPrototypes3.maxsize = 0;
  this->GroupPrototypeQs3.array = 0;
  this->GroupPrototypeQs3.size = 0;
  this->GroupPrototypeQs3.maxsize = 0;
  this->GroupPrototypes4.array = 0;
  this->GroupPrototypes4.size = 0;
  this->GroupPrototypes4.maxsize = 0;
  this->GroupPrototypeQs4.array = 0;
  this->GroupPrototypeQs4.size = 0;
  this->GroupPrototypeQs4.maxsize = 0;
  this->data.array = 0;
  this->data.size = 0;
  this->data.maxsize = 0;
  this->MainX = _x;
  this->MainY = _y;
  this->MainZ = _z;
  this->Object = 0;
  this->Race = 0;
  this->CloneParticles(p);
}

//----- (00453900) --------------------------------------------------------

SParticles::SParticles(SParticles *p, SIObject *object, char *meshname, int race)
{
  this->GroupPrototypes.array = 0;
  this->GroupPrototypes.size = 0;
  this->GroupPrototypes.maxsize = 0;
  this->GroupPrototypeQs.array = 0;
  this->GroupPrototypeQs.size = 0;
  this->GroupPrototypeQs.maxsize = 0;
  this->GroupPrototypes0.array = 0;
  this->GroupPrototypes0.size = 0;
  this->GroupPrototypes0.maxsize = 0;
  this->GroupPrototypeQs0.array = 0;
  this->GroupPrototypeQs0.size = 0;
  this->GroupPrototypeQs0.maxsize = 0;
  this->GroupPrototypes1.array = 0;
  this->GroupPrototypes1.size = 0;
  this->GroupPrototypes1.maxsize = 0;
  this->GroupPrototypeQs1.array = 0;
  this->GroupPrototypeQs1.size = 0;
  this->GroupPrototypeQs1.maxsize = 0;
  this->GroupPrototypes2.array = 0;
  this->GroupPrototypes2.size = 0;
  this->GroupPrototypes2.maxsize = 0;
  this->GroupPrototypeQs2.array = 0;
  this->GroupPrototypeQs2.size = 0;
  this->GroupPrototypeQs2.maxsize = 0;
  this->GroupPrototypes3.array = 0;
  this->GroupPrototypes3.size = 0;
  this->GroupPrototypes3.maxsize = 0;
  this->GroupPrototypeQs3.array = 0;
  this->GroupPrototypeQs3.size = 0;
  this->GroupPrototypeQs3.maxsize = 0;
  this->GroupPrototypes4.array = 0;
  this->GroupPrototypes4.size = 0;
  this->GroupPrototypes4.maxsize = 0;
  this->GroupPrototypeQs4.array = 0;
  this->GroupPrototypeQs4.size = 0;
  this->GroupPrototypeQs4.maxsize = 0;
  this->data.array = 0;
  this->data.size = 0;
  this->data.maxsize = 0;
  strncpy(this->MeshName, meshname, sizeof(this->MeshName) - 1);
  this->MeshName[sizeof(this->MeshName) - 1] = '\0';
  this->Object = object;
  this->Race = race;
  int v9 = object->GetMeshIndex((char *)"test");
  this->MeshIdx = v9;
  if ( v9 == -1 )
    Logger.g->Panic("SParticles::RefreshIndices: \"test\"mesh not found");
  this->CloneParticles(p);
}

//----- (00453B10) --------------------------------------------------------

SParticles::~SParticles()
{
  if ( this->Prototype )
  {
    this->Gepard->ReleaseTexture(this->THandle, 0);
  }
  else
  {
    for ( int i = 0; i < this->Darabszam; i++ )
    {
      this->data.array[i].group->Release();
    }
  }
  if ( this->data.array )
  {
    free(this->data.array);
    this->data.array = 0;
  }
  if ( this->GroupPrototypeQs4.array )
  {
    free(this->GroupPrototypeQs4.array);
    this->GroupPrototypeQs4.array = 0;
  }
  if ( this->GroupPrototypes4.array )
  {
    free(this->GroupPrototypes4.array);
    this->GroupPrototypes4.array = 0;
  }
  if ( this->GroupPrototypeQs3.array )
  {
    free(this->GroupPrototypeQs3.array);
    this->GroupPrototypeQs3.array = 0;
  }
  if ( this->GroupPrototypes3.array )
  {
    free(this->GroupPrototypes3.array);
    this->GroupPrototypes3.array = 0;
  }
  if ( this->GroupPrototypeQs2.array )
  {
    free(this->GroupPrototypeQs2.array);
    this->GroupPrototypeQs2.array = 0;
  }
  if ( this->GroupPrototypes2.array )
  {
    free(this->GroupPrototypes2.array);
    this->GroupPrototypes2.array = 0;
  }
  if ( this->GroupPrototypeQs1.array )
  {
    free(this->GroupPrototypeQs1.array);
    this->GroupPrototypeQs1.array = 0;
  }
  if ( this->GroupPrototypes1.array )
  {
    free(this->GroupPrototypes1.array);
    this->GroupPrototypes1.array = 0;
  }
  if ( this->GroupPrototypeQs0.array )
  {
    free(this->GroupPrototypeQs0.array);
    this->GroupPrototypeQs0.array = 0;
  }
  if ( this->GroupPrototypes0.array )
  {
    free(this->GroupPrototypes0.array);
    this->GroupPrototypes0.array = 0;
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

//----- (00453D90) --------------------------------------------------------

void SParticles::CloneParticles(SParticles *p)
{
  this->THandle = p->THandle;
  this->Prototypes = p;
  this->LastedTime = 0;
  this->RandomX = p->RandomX;
  this->RandomY = p->RandomY;
  this->RandomZ = p->RandomZ;
  this->BoomAfter = p->BoomAfter;
  this->sz = 0;
  strncpy(this->ClassName, p->ClassName, sizeof(this->ClassName) - 1);
  this->ClassName[sizeof(this->ClassName) - 1] = '\0';
  this->Effect = p->Effect;
  SGepard *Gepard = p->Gepard;
  this->Gepard = Gepard;
  this->World = p->World;
  this->THandle = p->THandle;
  this->FadeOutSpeed = p->FadeOutSpeed;
  this->FadeOutSpeed_Rnd = p->FadeOutSpeed_Rnd;
  float TimeIntervall = p->TimeIntervall;
  this->TimeIntervall = TimeIntervall;
  int OsszesDarabszam = p->OsszesDarabszam;
  this->OsszesDarabszam = OsszesDarabszam;
  this->Darabszam = 0;
  this->TimePeriod = (int)(float)(TimeIntervall / (float)OsszesDarabszam);
  this->MultiTileset = p->MultiTileset;
  if ( p->MultiTileset )
  {
    SDArray<int> *srcQs;
    switch ( Gepard->Tileset )
    {
      case 0: srcQs = &p->GroupPrototypeQs0; break;
      case 1: srcQs = &p->GroupPrototypeQs1; break;
      case 2: srcQs = &p->GroupPrototypeQs2; break;
      case 3: srcQs = &p->GroupPrototypeQs3; break;
      case 4: srcQs = &p->GroupPrototypeQs4; break;
      default: srcQs = 0; break;
    }
    if ( srcQs )
    {
      for ( int i = 0; i < srcQs->size; i++ )
      {
        this->GroupPrototypeQs.Add();
        this->GroupPrototypeQs.array[i] = srcQs->array[i];
      }
    }
  }
  else
  {
    for ( int i = 0; i < p->GroupPrototypeQs.size; i++ )
    {
      this->GroupPrototypeQs.Add();
      this->GroupPrototypeQs.array[i] = p->GroupPrototypeQs.array[i];
    }
  }
  this->HSpeed = p->HSpeed;
  this->HSpeed_Rnd = p->HSpeed_Rnd;
  this->KisG = p->KisG;
  this->VSpeed = p->VSpeed;
  this->VSpeed_Rnd = p->VSpeed_Rnd;
  this->ZRotSpeed = p->ZRotSpeed;
  this->ZRotSpeed_Rnd = p->ZRotSpeed_Rnd;
  this->TalajKoszolas = p->TalajKoszolas;
  this->Trail_Color = p->Trail_Color;
  this->Trail_Strength = p->Trail_Strength;
  this->Trail_FadeSpeed = p->Trail_FadeSpeed;
  this->Trail_Scale = p->Trail_Scale;
  this->DrawType = p->DrawType;
  this->Prototype = 0;
  this->ThisMeshRepeating = 0;
  if ( TimeIntervall == 0.0f )
  {
    int totalCount = this->OsszesDarabszam;
    for ( int ci = 0; ci < totalCount; ci++ )
    {
      // Grow data array
      int dataSize = this->data.size;
      if ( dataSize == this->data.maxsize )
      {
        int newmax = (this->data.maxsize >= 16) ? (6 * this->data.maxsize / 5) : 16;
        SParticlesData *newArray = (SParticlesData *)realloc(this->data.array, newmax * sizeof(SParticlesData));
        int oldmax = this->data.maxsize;
        this->data.array = newArray;
        memset(&newArray[oldmax], 0, (newmax - oldmax) * sizeof(SParticlesData));
        dataSize = this->data.size;
        this->data.maxsize = newmax;
      }
      this->data.size = dataSize + 1;
      if ( dataSize != ci )
        Logger.g->Panic("SParticles::Initializations: data.Add()!=ci");

      SParticlesData *pd = &this->data.array[ci];
      pd->yspeed = this->VSpeed + (float)((float)((float)rand() / 32767.0f) * this->VSpeed_Rnd);
      pd->kisg = this->KisG;
      float hspeed = this->HSpeed + (float)((float)((float)rand() / 32767.0f) * this->HSpeed_Rnd);
      float angleDeg = (float)((float)rand() / 32767.0f) * 360.0f;
      float dirDeg = (float)(360.0f - angleDeg) + 90.0f;
      while ( dirDeg < 0.0f ) dirDeg += 360.0f;
      while ( dirDeg >= 360.0f ) dirDeg -= 360.0f;
      float dirRad = dirDeg * 0.017453292f;
      pd->vectorx = cosf(dirRad) * hspeed;
      pd->vectorz = sinf(dirRad) * hspeed;
      pd->xrot = 0.0f;
      pd->yrot = angleDeg + 90.0f * 0.017453292f;
      pd->zrot = 0.0f;
      pd->zrotspeed = this->ZRotSpeed + (float)((float)((float)rand() / 32767.0f) * this->ZRotSpeed_Rnd);
      pd->x = (float)(this->MainX - (float)(this->RandomX * 0.5f)) + (float)((float)((float)rand() / 32767.0f) * this->RandomX);
      pd->y = this->MainY + (float)((float)((float)rand() / 32767.0f) * this->RandomY);
      pd->z = (float)(this->MainZ - (float)(this->RandomZ * 0.5f)) + (float)((float)((float)rand() / 32767.0f) * this->RandomZ);

      // Create 3D mesh object for this particle
      int *protoArray;
      if ( this->MultiTileset )
      {
        switch ( this->Gepard->Tileset )
        {
          case 0: protoArray = p->GroupPrototypes0.array; break;
          case 1: protoArray = p->GroupPrototypes1.array; break;
          case 2: protoArray = p->GroupPrototypes2.array; break;
          case 3: protoArray = p->GroupPrototypes3.array; break;
          case 4: protoArray = p->GroupPrototypes4.array; break;
          default: protoArray = p->GroupPrototypes.array; break;
        }
      }
      else
      {
        protoArray = p->GroupPrototypes.array;
      }
      pd->group = this->Gepard->CreateObjectByIndex(protoArray[this->ThisMeshRepeating], 1);
      pd->group->SetVisClass(0);
      pd->group->SetPosition(pd->x, pd->y, pd->z);
      pd->group->SetRotation(pd->xrot, pd->yrot, pd->zrot);

      int *qsArray = this->GroupPrototypeQs.array;
      if ( --qsArray[this->ThisMeshRepeating] <= 0 )
        ++this->ThisMeshRepeating;

      if ( this->THandle != -1 )
      {
        pd->smoketrailindex = this->Gepard->CreateSmokeTrail(
          this->THandle, pd->x, pd->y, pd->z,
          this->Trail_Color, this->Trail_Strength, this->Trail_FadeSpeed,
          this->Trail_Scale, 0.25f, this->DrawType);
      }
      pd->alpha = 1.0f;
      pd->fadeoutspeed = this->FadeOutSpeed + (float)((float)((float)rand() / 32767.0f) * this->FadeOutSpeed_Rnd);
      pd->active = 1;
      totalCount = this->OsszesDarabszam;
    }
    this->Darabszam = totalCount;
  }
}

//----- (004546A0) --------------------------------------------------------

bool SParticles::MoveParticles()
{
  SIObject *Object;
  SGepard *Gepard;
  SVector pos;
  SVector head;

  Object = this->Object;
  if ( Object )
  {
    memset(&pos, 0, sizeof(pos));
    memset(&head, 0, sizeof(head));
    Object->GetMeshProperties(this->MeshIdx, &pos.x, &pos.y, &pos.z, &head.x, &head.y, &head.z);
    this->MainX = pos.x;
    this->MainY = pos.y;
    this->MainZ = pos.z;
  }
  Gepard = this->Gepard;
  float TimeIntervall = this->TimeIntervall;
  unsigned int ElapsedTime = Gepard->ElapsedTime;
  float dt = (float)((double)ElapsedTime * 0.001);
  if ( TimeIntervall > 0.0f )
  {
    int LastedTime = this->LastedTime + ElapsedTime;
    this->LastedTime = LastedTime;
    float newInterval = TimeIntervall - (float)ElapsedTime;
    this->TimeIntervall = newInterval;
    if ( newInterval <= 0.0f && this->BoomAfter )
    {
      STerrain *Terrain;
      // Play explosion effects at ground level
      Terrain = Gepard->Terrain;
      float groundY = (float)Terrain->GetHeight(this->MainX, this->MainZ) + 0.5f;
      Gepard->PlayEffectPos(Gepard->_kis_robbanas_darabok, this->MainX, groundY, this->MainZ, 0.0f);

      Terrain = Gepard->Terrain;
      groundY = (float)Terrain->GetHeight(this->MainX, this->MainZ) + 0.5f;
      Gepard->PlayEffectPos(Gepard->_shader_light, this->MainX, groundY, this->MainZ, 0.0f);

      Terrain = Gepard->Terrain;
      groundY = (float)Terrain->GetHeight(this->MainX, this->MainZ) + 1.0f;
      Gepard->PlayEffectPos(Gepard->_nagy_robbanas_darabok, this->MainX, groundY, this->MainZ, 0.0f);

      int rnd = rand();
      Terrain = Gepard->Terrain;
      groundY = (float)Terrain->GetHeight(this->MainX, this->MainZ) + 1.4f;
      if ( (int)(float)((float)((float)rnd * 0.000030517578f) + (float)((float)rnd * 0.000030517578f)) )
        Gepard->PlayEffectPos(Gepard->_robbanas_anim, this->MainX, groundY, this->MainZ, 0.0f);
      else
        Gepard->PlayEffectPos(Gepard->_robbanas_anim2, this->MainX, groundY, this->MainZ, 0.0f);

      Gepard->EgysegAlattiTalajKoszolas(this->MainX, this->MainZ, this->Race);
      LastedTime = this->LastedTime;
    }
    int TimePeriod = this->TimePeriod;
    int spawnCount = LastedTime / TimePeriod;
    if ( spawnCount > 0 )
    {
      this->LastedTime = LastedTime - TimePeriod;
      for ( int s = 0; s < spawnCount; s++ )
      {
        if ( this->Darabszam != this->OsszesDarabszam )
        {
          this->data.Add();
          if ( this->data.size - 1 != this->Darabszam )
            Logger.g->Panic("SParticles::Initializations: data.Add()!=Darabszam");

          SParticlesData *pd = &this->data.array[this->Darabszam];
          pd->yspeed = this->VSpeed + (float)((float)((float)rand() / 32767.0f) * this->VSpeed_Rnd);
          pd->kisg = this->KisG;
          float hspeed = this->HSpeed + (float)((float)((float)rand() / 32767.0f) * this->HSpeed_Rnd);
          float angleDeg = (float)((float)rand() / 32767.0f) * 360.0f;
          float dirDeg = (float)(360.0f - angleDeg) + 90.0f;
          while ( dirDeg < 0.0f ) dirDeg += 360.0f;
          while ( dirDeg >= 360.0f ) dirDeg -= 360.0f;
          float dirRad = dirDeg * 0.017453292f;
          pd->vectorx = cosf(dirRad) * hspeed;
          pd->vectorz = sinf(dirRad) * hspeed;
          pd->xrot = 0.0f;
          pd->yrot = angleDeg + 90.0f * 0.017453292f;
          pd->zrot = 0.0f;
          pd->zrotspeed = this->ZRotSpeed + (float)((float)((float)rand() / 32767.0f) * this->ZRotSpeed_Rnd);
          pd->x = (float)(this->MainX - (float)(this->RandomX * 0.5f)) + (float)((float)((float)rand() / 32767.0f) * this->RandomX);
          pd->y = this->MainY + (float)((float)((float)rand() / 32767.0f) * this->RandomY);
          pd->z = (float)(this->MainZ - (float)(this->RandomZ * 0.5f)) + (float)((float)((float)rand() / 32767.0f) * this->RandomZ);

          pd->group = this->Gepard->CreateObjectByIndex(
            this->Prototypes->GroupPrototypes.array[this->ThisMeshRepeating], 1);
          pd->group->SetVisClass(0);
          pd->group->SetPosition(pd->x, pd->y, pd->z);
          pd->group->SetRotation(pd->xrot, pd->yrot, pd->zrot);

          int *qsArray = this->GroupPrototypeQs.array;
          if ( --qsArray[this->ThisMeshRepeating] <= 0 )
            ++this->ThisMeshRepeating;

          if ( this->THandle != -1 )
          {
            pd->smoketrailindex = this->Gepard->CreateSmokeTrail(
              this->THandle, pd->x, pd->y, pd->z,
              this->Trail_Color, this->Trail_Strength, this->Trail_FadeSpeed,
              this->Trail_Scale, 0.25f, this->DrawType);
          }
          pd->alpha = 1.0f;
          pd->fadeoutspeed = this->FadeOutSpeed + (float)((float)((float)rand() / 32767.0f) * this->FadeOutSpeed_Rnd);
          pd->active = 1;

          // Small explosion at every 6th particle
          if ( this->BoomAfter && !(this->Darabszam % 6) )
          {
            int rnd = rand();
            if ( (int)(float)((float)((float)rnd * 0.000030517578f) + (float)((float)rnd * 0.000030517578f)) )
              Gepard->PlayEffectPos(Gepard->_tuzijatek_robbanas_anim, pd->x, pd->y + 0.2f, pd->z, 0.0f);
            else
              Gepard->PlayEffectPos(Gepard->_tuzijatek_robbanas_anim2, pd->x, pd->y + 0.2f, pd->z, 0.0f);
          }
          ++this->Darabszam;
        }
      }
    }
  }

  // Update existing particles
  bool hasActive = false;
  for ( int i = 0; i < this->Darabszam; i++ )
  {
    SParticlesData *pd = &this->data.array[i];
    if ( !pd->active )
      continue;

    hasActive = true;
    pd->yspeed = pd->yspeed - (float)(pd->kisg * dt);
    pd->x = (float)(pd->vectorx * dt) + pd->x;
    pd->z = (float)(pd->vectorz * dt) + pd->z;
    pd->y = (float)(pd->yspeed * dt) + pd->y;
    pd->zrot = (float)(pd->zrotspeed * dt) + pd->zrot;
    pd->alpha = pd->alpha - (float)(pd->fadeoutspeed * dt);
    if ( pd->alpha < 0.0f )
      pd->alpha = 0.0f;

    // Update 3D object transform
    pd->group->SetPosition(pd->x, pd->y, pd->z);
    pd->group->SetRotation(pd->xrot, pd->yrot, pd->zrot);
    if ( this->DrawType == DT_ADD )
      pd->group->SetAmbient(pd->alpha);

    // Track smoke trail
    if ( this->THandle != -1 )
      this->Gepard->TrackSmokeTrail(pd->smoketrailindex, pd->x, pd->y, pd->z, pd->alpha);

    // Check if particle hit ground or faded out
    STerrain *Terrain = this->Gepard->Terrain;
    float groundY = (float)Terrain->GetHeight(pd->x, pd->z);
    if ( groundY > pd->y + 0.6f || pd->alpha == 0.0f )
    {
      pd->active = 0;
      if ( this->THandle != -1 )
        this->Gepard->CloseSmokeTrail(pd->smoketrailindex);

      // Ground impact effects
      if ( this->TalajKoszolas )
      {
        switch ( (int)(float)((float)((float)rand() * 0.000030517578f) * 4.0f) )
        {
          case 0:
            Gepard->PlayEffectPos(Gepard->_becsapodo_darab_nyom1, pd->x, pd->y, pd->z, 0.0f);
            break;
          case 1:
            Gepard->PlayEffectPos(Gepard->_becsapodo_darab_nyom2, pd->x, pd->y, pd->z, 0.0f);
            break;
          case 2:
            Gepard->PlayEffectPos(Gepard->_becsapodo_darab_nyom3, pd->x, pd->y, pd->z, 0.0f);
            break;
          case 3:
            Gepard->PlayEffectPos(Gepard->_becsapodo_darab_nyom4, pd->x, pd->y, pd->z, 0.0f);
            break;
        }

        Terrain = this->Gepard->Terrain;
        float smokeY = (float)Terrain->GetHeight(pd->x, pd->z) - 0.15f;
        Gepard->PlayEffectPos(Gepard->_a_foldbe_csapodo_darabok_fustje, pd->x, smokeY, pd->z, 0.0f);
      }
    }
  }
  if ( hasActive )
    return 1;
  return this->TimeIntervall > 0.0f;
}

//----- (00455410) --------------------------------------------------------

int SParticles::PrecacheObject(char *variable_name, SDrawType drawtype, int Tileset)
{
  char *String;
  char fullpathandname[260];

  String = this->Effect->EffectsINI->GetString(this->ClassName, variable_name, 0);
  if ( !String )
    return -1;
  switch ( Tileset )
  {
    case -1:
      strcpy(fullpathandname, "effects\\");
      break;
    case 0:
      strcpy(fullpathandname, "effects/tileset_normal/");
      break;
    case 1:
      strcpy(fullpathandname, "effects/tileset_desert/");
      break;
    case 2:
      strcpy(fullpathandname, "effects\\tileset_snow\\");
      break;
    case 3:
      strcpy(fullpathandname, "effects\\tileset_rock\\");
      break;
    case 4:
      strcpy(fullpathandname, "effects/tileset_jungle/");
      break;
  }
  strcat(fullpathandname, String);
  // Scale must match original binary (0x3b1374bc ≈ 0.00225) — normals are
  // multiplied by this in Load4DFile; zero produces zero-length normals which
  // break lighting and make meshes invisible.
  union { unsigned int i; float f; } _s; _s.i = 0x3b1374bc;
  return this->Gepard->PrecacheObject(fullpathandname, _s.f, drawtype);
}

//----- (00455580) --------------------------------------------------------

void SParticles::RefreshIndices()
{
  int v2 = this->Object->GetMeshIndex((char *)"test");
  this->MeshIdx = v2;
  if ( v2 == -1 )
    Logger.g->Panic("SParticles::RefreshIndices: \"test\"mesh not found");
}

//----- (004555C0) --------------------------------------------------------

void SParticles::SetPosition(float _x, float _y, float _z)
{
  this->MainX = _x;
  this->MainY = _y;
  this->MainZ = _z;
}
