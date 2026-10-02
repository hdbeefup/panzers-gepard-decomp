// 3dengine/particles3.cpp
// Particle system 3 (wind-affected billboard particles using dynamic vertex buffers)
// Decompiled from: gameSplit/sparticles.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <math.h>

#include "particles3.h"
#include "gepard.h"
#include "terrain.h"
#include "effects.h"
#include "properties.h"
#include "logger.h"

// SIObject — use full definition from iobject.h for correct vtable layout
#include "iobject.h"

// Transformed & Lit vertex for DynamicVB (FVF 0x1C4 = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX1)
struct TLVert3 {
    float x, y, z, rhw;
    unsigned int diffuse;
    unsigned int specular;
    float u, v;
};

// Classes: SParticles3
// Function count: 11

//----- (00456690) --------------------------------------------------------

SParticles3::SParticles3(char *classname, SEffect *effect, SGepard *gepard, float _randomx, float _randomz)
{
  float v59;
  this->GroupPrototypes.array = 0;
  this->GroupPrototypes.size = 0;
  this->GroupPrototypes.maxsize = 0;
  this->GroupPrototypeQs.array = 0;
  this->GroupPrototypeQs.size = 0;
  this->GroupPrototypeQs.maxsize = 0;
  this->data.array = 0;
  this->data.size = 0;
  this->data.maxsize = 0;
  this->MainX = -1.0f;
  strncpy(this->ClassName, classname, sizeof(this->ClassName) - 1);
  this->ClassName[sizeof(this->ClassName) - 1] = '\0';
  this->Effect = effect;
  this->Gepard = gepard;
  this->Prototype = 1;
  SDrawType Int = (SDrawType)effect->EffectsINI->GetInt(this->ClassName, "DrawType", -1);
  this->DrawType = Int;
  if ( Int == (SDrawType)-1 )
    Logger.g->Panic(
      "SParticles3::Initializations: A DrawType nincs megadva, vagy nulla van megadva.");
  char tmpstr[260];
  char *String = this->Effect->EffectsINI->GetString(this->ClassName, "Texture", "error");
  strcpy(tmpstr, String);
  if ( !strcmp(tmpstr, "error") )
    Logger.g->Panic(
      "SParticles3::SParticles3: cannot load texture in class: \"%s\"",
      this->ClassName);
  char fn[260];
  strcpy(fn, "effects\\");
  strcat(fn, tmpstr);
#ifdef HDB_MISSING_ASSET_FALLBACK
  int v17 = this->Gepard->LoadTextureOrPlaceholder(fn, 1, 1);
#else
  int v17 = this->Gepard->LoadTexture(fn, 1, 1);
#endif
  this->THandle = v17;
  if ( v17 == -1 )
    Logger.g->Panic("SParticles3::SParticles3: cannot load texture \"%s\"", fn);
  this->NoWind = this->Effect->EffectsINI->GetInt(this->ClassName, "NoWind", 0) != 0;
  this->BorningEnabledTime = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "BorningEnabledTime", 0.0f);
  this->BorningDisabledTime = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "BorningEnabledTime", 0.0f);
  this->BorningEnabledTimeRND = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "BorningEnabledTimeRND", 0.0f);
  this->BorningDisabledTimeRND = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "BorningDisabledTimeRND", 0.0f);
  this->ScaleSpeed = this->Effect->EffectsINI->GetFloat(this->ClassName, "ScaleSpeed", 0.0f);
  v59 = this->Effect->EffectsINI->GetFloat(this->ClassName, "Borning_Speed", 1.0f);
  this->BorningSpeed = v59;
  if ( _randomx == -1.0f )
  {
    this->RandomX = this->Effect->EffectsINI->GetFloat(this->ClassName, "RandomX", 0.0f);
    this->RandomZ = this->Effect->EffectsINI->GetFloat(this->ClassName, "RandomZ", 0.0f);
  }
  else
  {
    this->RandomX = _randomx;
    this->RandomZ = _randomz;
    this->BorningSpeed = (float)((float)(_randomx * _randomz) / 7.0f) * v59;
  }
  this->ManageType = this->Effect->EffectsINI->GetInt(this->ClassName, "ManageType", 0);
  this->HSpeed = this->Effect->EffectsINI->GetFloat(this->ClassName, "HSpeed", 0.0f);
  this->HSpeed_Rnd = this->Effect->EffectsINI->GetFloat(this->ClassName, "HSpeed_Rnd", 0.0f);
  this->KisG = this->Effect->EffectsINI->GetFloat(this->ClassName, "KisG", 0.0f);
  float vspeed = this->Effect->EffectsINI->GetFloat(this->ClassName, "VSpeed", -1.0f);
  this->VSpeed = vspeed;
  if ( vspeed == -1.0f )
    Logger.g->Panic("SParticles3::Initializations: A VSpeed nincs megadva, vagy nulla van megadva.");
  float vspeed_rnd = this->Effect->EffectsINI->GetFloat(this->ClassName, "VSpeed_Rnd", -1.0f);
  this->VSpeed_Rnd = vspeed_rnd;
  if ( vspeed_rnd == -1.0f )
    Logger.g->Panic(
      "SParticles3::Initializations: A VSpeed_Rnd nincs megadva, vagy nulla van megadva.");
  this->FadeOutSpeed = this->Effect->EffectsINI->GetFloat(this->ClassName, "FadeOutSpeed", 0.0f);
  this->FadeOutSpeed_Rnd = this->Effect->EffectsINI->GetFloat(this->ClassName, "FadeOutSpeed_Rnd", 0.0f);
  this->Color = this->Effect->EffectsINI->GetInt(this->ClassName, "Color", 0);
  this->StartAlpha = this->Effect->EffectsINI->GetFloat(this->ClassName, "StartAlpha", 1.0f);
  this->StopTimeMoment = this->Effect->EffectsINI->GetFloat(this->ClassName, "StopTimeMoment", 1.0f);
  this->TalajKoszolas = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "Talaj_Koszolas", 0.0f);
  this->Darabszam = this->Effect->EffectsINI->GetInt(this->ClassName, "Darabszam", 5);
  this->Scale = this->Effect->EffectsINI->GetFloat(this->ClassName, "Scale", 1.0f);
  float variations = this->Effect->EffectsINI->GetFloat(this->ClassName, "Variations", 1.0f);
  this->Variations = variations;
  this->RecVariations = 1.0f / this->Effect->EffectsINI->GetFloat(this->ClassName, "Variations", 1.0f);
  float maxParticlesF = this->StartAlpha * 1.5f / this->FadeOutSpeed * fmaxf(this->BorningSpeed, 6.0f);
  this->MaxParticles = (int)maxParticlesF;
  this->DynamicVB = this->Gepard->CreateDynamicVB(0x1C4u, 12 * (int)maxParticlesF);
}

//----- (00456CD0) --------------------------------------------------------

SParticles3::SParticles3(SParticles3 *p, float _x, float _y, float _z, float _scalespeed)
{
  this->GroupPrototypes.array = 0;
  this->GroupPrototypes.size = 0;
  this->GroupPrototypes.maxsize = 0;
  this->GroupPrototypeQs.array = 0;
  this->GroupPrototypeQs.size = 0;
  this->GroupPrototypeQs.maxsize = 0;
  this->data.array = 0;
  this->data.size = 0;
  this->data.maxsize = 0;
  this->Object = 0;
  this->MeshName[0] = 0;
  this->MainX = _x;
  this->MainY = _y;
  this->TorkolatMeshIdx = -1;
  this->MainZ = _z;
  float ScaleSpeed = p->ScaleSpeed;
  if ( ScaleSpeed == 0.0f )
    ScaleSpeed = _scalespeed;
  this->ScaleSpeed = ScaleSpeed;
  this->CloneParticles(p);
}

//----- (00456DC0) --------------------------------------------------------

SParticles3::SParticles3(SParticles3 *p, SIObject *object, char *meshname)
{
  this->GroupPrototypes.array = 0;
  this->GroupPrototypes.size = 0;
  this->GroupPrototypes.maxsize = 0;
  this->GroupPrototypeQs.array = 0;
  this->GroupPrototypeQs.size = 0;
  this->GroupPrototypeQs.maxsize = 0;
  this->data.array = 0;
  this->data.size = 0;
  this->data.maxsize = 0;
  this->MainX = -1.0f;
  this->ScaleSpeed = p->ScaleSpeed;
  this->Object = object;
  strncpy(this->MeshName, meshname, sizeof(this->MeshName) - 1);
  this->MeshName[sizeof(this->MeshName) - 1] = '\0';
  if ( this->MeshName[0] )
  {
    int v8 = this->Object->GetMeshIndex(this->MeshName);
    this->TorkolatMeshIdx = v8;
    if ( v8 == -1 )
      Logger.g->Panic("SParticles3::RefreshIndices: mesh \"%s\" cannot be load!", this->MeshName);
  }
  else
  {
    this->TorkolatMeshIdx = -1;
  }
  this->CloneParticles(p);
}

//----- (00456EF0) --------------------------------------------------------

SParticles3::~SParticles3()
{
  if ( this->Prototype )
  {
    this->Gepard->ReleaseTexture(this->THandle, 0);
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

//----- (00456FA0) --------------------------------------------------------

void SParticles3::CloneParticles(SParticles3 *p)
{
  SGepard *Gepard;
  this->sz = 0;
  strncpy(this->ClassName, p->ClassName, sizeof(this->ClassName) - 1);
  this->ClassName[sizeof(this->ClassName) - 1] = '\0';
  this->Effect = p->Effect;
  Gepard = p->Gepard;
  this->Gepard = Gepard;
  this->World = p->World;
  this->THandle = p->THandle;
  this->FadeOutSpeed = p->FadeOutSpeed;
  this->FadeOutSpeed_Rnd = p->FadeOutSpeed_Rnd;
  // Apply environment color modulation to particle color components
  unsigned int srcColor = p->Color;
  int b = (int)(float)((float)((float)((float)(srcColor & 0xFF) * 0.00390625f)
                               * Gepard->EnvironmentColorVal.b)
                       * 255.0f);
  int g = (int)(float)((float)((float)((float)((srcColor >> 8) & 0xFF) * 0.00390625f)
                               * Gepard->EnvironmentColorVal.g)
                       * 255.0f);
  int r = (int)(float)((float)((float)((float)((srcColor >> 16) & 0xFF) * 0.00390625f)
                               * Gepard->EnvironmentColorVal.r)
                       * 255.0f);
  this->Color = b | ((g | (r << 8)) << 8);
  this->StartAlpha = p->StartAlpha;
  this->StopTimeMoment = p->StopTimeMoment;
  this->Variations = p->Variations;
  this->RecVariations = p->RecVariations;
  int BorningEnabledTime = p->BorningEnabledTime;
  this->BorningEnabledTime = BorningEnabledTime;
  this->BorningDisabledTime = p->BorningDisabledTime;
  this->BorningEnabledTimeRND = p->BorningEnabledTimeRND;
  this->BorningDisabledTimeRND = p->BorningDisabledTimeRND;
  this->Stopping = 0;
  this->Darabszam = p->Darabszam;
  this->Scale = p->Scale;
  this->RandomX = p->RandomX;
  this->RandomZ = p->RandomZ;
  this->BorningSpeed = p->BorningSpeed;
  this->BorningCounter = 1.0f;
  this->HSpeed = p->HSpeed;
  this->HSpeed_Rnd = p->HSpeed_Rnd;
  this->KisG = p->KisG;
  this->VSpeed = p->VSpeed;
  this->VSpeed_Rnd = p->VSpeed_Rnd;
  this->TalajKoszolas = p->TalajKoszolas;
  this->DrawType = p->DrawType;
  this->ManageType = p->ManageType;
  this->Prototype = 0;
  bool v10;
  if ( BorningEnabledTime )
  {
    int v9 = rand();
    v10 = (int)(float)((float)((float)v9 * 0.000030517578f) + (float)((float)v9 * 0.000030517578f)) != 0;
  }
  else
  {
    v10 = 1;
  }
  this->BorningState = v10;
  this->BorningStateCounter = 0.0f;
  float i;
  for ( i = (float)((float)((float)rand() / 32767.0f) - 0.5f) + this->Gepard->GlobalisSzelirany; i < 0.0f; i = i + 6.2831855f )
    ;
  for ( ; i >= 6.2831855f; i = i - 6.2831855f )
    ;
  this->LokalisSzelirany = i;
  this->SzelIrany1 = i;
  this->SzelIrany2 = i;
  this->SzelEro1 = 0.30000001f;
  this->SzelEro2 = 0.69999999f;
  this->SzelMagassag1 = 0.80000001f;
  this->SzelMagassag2 = 1.6f;
  this->SzelMagRatioConv1 = 1.25f;
  this->SzelMagRatioConv2 = 1.25f;
  this->SzelTime1 = 0.0f;
  this->SzelTime2 = 0.0f;
  this->SzelLokesMode1 = true;
  this->SzelLokesMode2 = true;
  this->NoWind = p->NoWind;
  this->DynamicVB = p->DynamicVB;
  this->MaxParticles = p->MaxParticles;
}

//----- (00457290) --------------------------------------------------------

void SParticles3::GetSzelVector(float *x, float y, float *z)
{
  float SzelMagassag1 = this->SzelMagassag1;
  if ( SzelMagassag1 < y )
  {
    float v6 = (float)(y - SzelMagassag1) * this->SzelMagRatioConv2;
    *x = (float)(this->SzelIranyVecX2 * v6) + (float)((float)(1.0f - v6) * this->SzelIranyVecX1);
    *z = (float)(this->SzelIranyVecZ2 * v6) + (float)((float)(1.0f - v6) * this->SzelIranyVecZ1);
  }
  else
  {
    float v5 = this->SzelMagRatioConv1 * y;
    *x = v5 * this->SzelIranyVecX1;
    *z = v5 * this->SzelIranyVecZ1;
  }
}

//----- (004573B0) --------------------------------------------------------

bool SParticles3::MoveParticles(int a2, int a3)
{
  SIObject *Object;
  SGepard *Gepard;
  STerrain *Terrain;
  SVector pos;
  SVector head;

  Object = this->Object;
  if ( Object )
  {
    memset(&pos, 0, sizeof(pos));
    memset(&head, 0, sizeof(head));
    Object->GetMeshProperties(this->TorkolatMeshIdx, &pos.x, &pos.y, &pos.z, &head.x, &head.y, &head.z);
    this->MainX = pos.x;
    this->MainY = pos.y + 0.1f;
    this->MainZ = pos.z;
  }
  else if ( this->MainX == -1.0f )
  {
    return 0;
  }

  if ( this->ManageType == 3 )
  {
    Terrain = this->Gepard->Terrain;
    unsigned char *vm = Terrain->VisMap;
    if ( vm
      && !Terrain->GodMode
      && !vm[(int)(float)(this->MainX + this->MainX) - 2 * (Terrain->XSize + 1) * (int)(float)(this->MainZ * -2.0f)] )
    {
      return 0;
    }
    this->ManageType = 0;
  }

  float MainX = this->MainX;
  float MainZ = this->MainZ;
  float x = MainX - 4.0f;
  Gepard = this->Gepard;
  float zCheck = MainZ - 4.0f;
  Terrain = Gepard->Terrain;

  if ( x < 0.0f
    || (float)Terrain->XSize <= x
    || zCheck < 0.0f
    || (float)Terrain->ZSize <= zCheck
    || !Terrain->Parcels[-(int)(x * -0.125f) - Terrain->XParcels * (int)(zCheck * -0.125f)].Visible )
  {
    float xPlus = MainX + 4.0f;
    if ( xPlus < 0.0f
      || (float)Terrain->XSize <= xPlus
      || zCheck < 0.0f
      || (float)Terrain->ZSize <= zCheck
      || !Terrain->Parcels[-(int)(xPlus * -0.125f) - Terrain->XParcels * (int)(zCheck * -0.125f)].Visible )
    {
      float zPlus = MainZ + 4.0f;
      if ( !Terrain->IsParcelVisible(x, zPlus) && !Terrain->IsParcelVisible(xPlus, zPlus) )
        return 1;
      MainZ = this->MainZ;
    }
    MainX = this->MainX;
  }

  unsigned char *VisMap = Terrain->VisMap;
  float dt = (float)((double)Gepard->ElapsedTime * 0.001);
  bool visible;
  if ( !VisMap
    || Terrain->GodMode
    || VisMap[(int)(float)(MainX + MainX) - 2 * (Terrain->XSize + 1) * (int)(float)(MainZ * -2.0f)] )
  {
    visible = true;
  }
  else
  {
    visible = false;
  }

  if ( this->BorningEnabledTime )
  {
    float BorningStateCounter = this->BorningStateCounter;
    if ( BorningStateCounter <= 0.0f )
    {
      bool BorningState = this->BorningState;
      this->BorningState = !BorningState;
      float timeRnd;
      int baseTime;
      if ( BorningState )
      {
        timeRnd = (float)this->BorningDisabledTimeRND;
        baseTime = this->BorningDisabledTime;
      }
      else
      {
        timeRnd = (float)this->BorningEnabledTimeRND;
        baseTime = this->BorningEnabledTime;
      }
      int v21 = rand();
      BorningStateCounter = (float)baseTime + (float)((float)((float)v21 / 32767.0f) * timeRnd);
    }
    this->BorningStateCounter = BorningStateCounter - dt;
  }

  float StopTimeMoment = this->StopTimeMoment;
  if ( StopTimeMoment > 0.0f && !this->Stopping && this->BorningState )
  {
    float BorningCounter = (float)(this->BorningSpeed * dt) + this->BorningCounter;
    this->BorningCounter = BorningCounter;
    if ( BorningCounter > 1.0f )
    {
      do
      {
        this->BorningCounter = BorningCounter - 1.0f;
        if ( visible || this->ManageType != 2 )
        {
          int size = this->data.size;
          if ( size < this->MaxParticles )
          {
            int maxsize = this->data.maxsize;
            if ( size == maxsize )
            {
              int newsize;
              if ( maxsize >= 16 )
                newsize = 6 * maxsize / 5;
              else
                newsize = 16;
              SParticles3Data *v29 = (SParticles3Data *)realloc(this->data.array, 48 * newsize);
              int oldmax = this->data.maxsize;
              this->data.array = v29;
              memset(&v29[oldmax], 0, 48 * (newsize - oldmax));
              size = this->data.size;
              this->data.maxsize = newsize;
            }
            this->data.size = size + 1;
            int idx = size;
            this->data.array[idx].yspeed = this->VSpeed + (float)((float)((float)rand() / 32767.0f) * this->VSpeed_Rnd);
            this->data.array[idx].kisg = this->KisG;
            float hspeed = (float)(this->HSpeed + (float)((float)((float)rand() / 32767.0f) * this->HSpeed_Rnd))
                         - (float)(this->HSpeed_Rnd * 0.5f);
            float angle = (float)(360.0f - (float)((float)((float)rand() / 32767.0f) * 360.0f)) + 90.0f;
            while ( angle < 0.0f ) angle += 360.0f;
            while ( angle >= 360.0f ) angle -= 360.0f;
            float angle_rad = angle * 0.017453292f;
            this->data.array[idx].vectorx = cosf(angle_rad) * hspeed;
            float angle2 = angle; // second angle for sin (same value, different normalization path in original)
            while ( angle2 < 0.0f ) angle2 += 360.0f;
            while ( angle2 >= 360.0f ) angle2 -= 360.0f;
            float angle_rad2 = angle2 * 0.017453292f;
            this->data.array[idx].vectorz = sinf(angle_rad2) * hspeed;
            this->data.array[idx].x = (float)((float)((float)((float)rand() / 32767.0f) * this->RandomX) + this->MainX)
                                    - (float)(this->RandomX * 0.5f);
            this->data.array[idx].y = this->MainY;
            this->data.array[idx].z = (float)(this->MainZ + (float)((float)((float)rand() / 32767.0f) * this->RandomZ))
                                    - (float)(this->RandomZ * 0.5f);
            this->data.array[idx].whichtexture = (int)(float)((float)((float)rand() * 0.000030517578f)
                                                            * (float)(int)this->Variations);
            this->data.array[idx].alpha = this->StartAlpha;
            this->data.array[idx].fadeoutspeed = this->FadeOutSpeed + (float)((float)((float)rand() / 32767.0f) * this->FadeOutSpeed_Rnd);
            this->data.array[idx].scale = this->Scale;
          }
        }
        BorningCounter = this->BorningCounter;
      }
      while ( BorningCounter > 1.0f );
      StopTimeMoment = this->StopTimeMoment;
    }
  }

  bool hasParticles = false;
  this->StopTimeMoment = StopTimeMoment - dt;
  this->Gepard->SetLightingType(1); // LT_PRELIT
  this->Gepard->SetDrawType(DT_NORMAL);
  this->Gepard->SetTexture(0, this->THandle, 1);
  this->Gepard->lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);

  // Wind gust processing for channel 1
  if ( this->SzelTime1 <= 0.0f )
  {
    bool SzelLokesMode1 = this->SzelLokesMode1;
    this->SzelTime1 = 1.0f;
    this->SzelLokesMode1 = !SzelLokesMode1;
    if ( SzelLokesMode1 )
    {
      int v39 = rand();
      this->SzelLokesEro1 = 0.0f;
      this->SzelSpinDir1 = 0.0f;
      this->SzelLokesSebesseg1 = (float)((float)((float)v39 / 32767.0f) * 0.2f) + 0.16f;
    }
    else
    {
      this->SzelLokesSebesseg1 = (float)((float)((float)rand() / 32767.0f) * 0.2f) + 0.16f;
      this->SzelLokesEro1 = (float)((float)rand() / 32767.0f) * 1.2f;
      this->SzelSpinDir1 = (float)((float)((float)rand() / 32767.0f) * 3.0f) - 1.5f;
    }
  }

  // Wind gust processing for channel 2
  if ( this->SzelTime2 <= 0.0f )
  {
    bool SzelLokesMode2 = this->SzelLokesMode2;
    this->SzelTime2 = 1.0f;
    this->SzelLokesMode2 = !SzelLokesMode2;
    if ( SzelLokesMode2 )
    {
      int v41 = rand();
      this->SzelLokesEro2 = 0.0f;
      this->SzelSpinDir2 = 0.0f;
      this->SzelLokesSebesseg2 = (float)((float)((float)v41 / 32767.0f) * 0.2f) + 0.16f;
    }
    else
    {
      this->SzelLokesSebesseg2 = (float)((float)((float)rand() / 32767.0f) * 0.2f) + 0.16f;
      this->SzelLokesEro2 = (float)((float)rand() / 32767.0f) * 1.2f;
      this->SzelSpinDir2 = (float)((float)((float)rand() / 32767.0f) * 3.0f) - 1.5f;
    }
  }

  // Compute wind direction vectors
  float t1 = this->SzelTime1 - (float)(this->SzelLokesSebesseg1 * dt);
  this->SzelTime1 = t1;
  this->SzelLokesCounter1 = (float)(1.0f - t1) * 3.1415f;
  float sinCounter1 = sinf(this->SzelLokesCounter1);
  float ero1 = (float)(this->SzelLokesEro1 * sinCounter1) + 0.30000001f;
  this->SzelEro1 = ero1;
  float dir1 = (float)(this->SzelSpinDir1 * sinCounter1) + this->LokalisSzelirany;
  this->SzelIrany1 = dir1;

  float t2 = this->SzelTime2 - (float)(this->SzelLokesSebesseg2 * dt);
  this->SzelTime2 = t2;
  this->SzelLokesCounter2 = (float)(1.0f - t2) * 3.1415f;
  float sinCounter2 = sinf(this->SzelLokesCounter2);
  float ero2 = (float)(this->SzelLokesEro2 * sinCounter2) + 0.69999999f;
  this->SzelEro2 = ero2;
  float dir2 = (float)(this->SzelSpinDir2 * sinCounter2) + this->LokalisSzelirany;
  this->SzelIrany2 = dir2;

  this->SzelIranyVecX1 = sinf(dir1) * ero1;
  this->SzelIranyVecZ1 = cosf(dir1) * ero1;
  this->SzelIranyVecX2 = sinf(dir2) * ero2;
  this->SzelIranyVecZ2 = cosf(dir2) * ero2;

  // Update and cull particles
  int size = this->data.size;
  int i = 0;
  if ( size > 0 )
  {
    hasParticles = true;
    do
    {
      float windX = 0.0f;
      float windZ = 0.0f;
      SParticles3Data *pd = &this->data.array[i];
      pd->yspeed = pd->yspeed - (float)(pd->kisg * dt);

      if ( !this->NoWind )
      {
        float height = pd->y - this->MainY;
        if ( this->SzelMagassag1 < height )
        {
          float blend = (float)(height - this->SzelMagassag1) * this->SzelMagRatioConv2;
          windX = (float)((float)(1.0f - blend) * this->SzelIranyVecX1) + (float)(blend * this->SzelIranyVecX2);
          windZ = (float)((float)(1.0f - blend) * this->SzelIranyVecZ1) + (float)(blend * this->SzelIranyVecZ2);
        }
        else
        {
          float ratio = this->SzelMagRatioConv1 * height;
          windX = ratio * this->SzelIranyVecX1;
          windZ = ratio * this->SzelIranyVecZ1;
        }
      }

      pd->x = (float)((float)(pd->vectorx + windX) * dt) + pd->x;
      pd->z = (float)((float)(pd->vectorz + windZ) * dt) + pd->z;
      pd->y = (float)(pd->yspeed * dt) + pd->y;
      pd->scale = (float)(this->ScaleSpeed * dt) + pd->scale;
      pd->alpha = pd->alpha - (float)(pd->fadeoutspeed * dt);

      if ( pd->alpha > 0.0f )
      {
        ++i;
      }
      else
      {
        // Remove dead particle by shifting remaining elements down
        int newSize = this->data.size - 1;
        this->data.size = newSize;
        for ( int j = i; j < newSize; j++ )
          this->data.array[j] = this->data.array[j + 1];
        memset(&this->data.array[newSize], 0, sizeof(SParticles3Data));
      }
      size = this->data.size;
    }
    while ( i < size );
  }

  // Render surviving particles
  if ( (visible || this->ManageType != 1) && size )
  {
    TLVert3 *vb = (TLVert3 *)this->Gepard->LockDynamicVB(i, (int)this, 0, this->DynamicVB, 6 * size);
    if ( !vb )
      return 1; // keep effect alive, skip rendering this frame
    int vertIdx = 0;
    if ( this->data.size > 0 )
    {
      for ( int p = 0; p < this->data.size; p++ )
      {
        float xpos, ypos, scale, zbuf, rhw;
        unsigned int fog;
        this->Gepard->TransformScaledPointBlend(
          this->data.array[p].x, this->data.array[p].y, this->data.array[p].z,
          this->data.array[p].scale, &xpos, &ypos, &scale, &zbuf, &rhw, &fog);

        int whichtexture = this->data.array[p].whichtexture;
        float RecVariations = this->RecVariations;
        float uLeft = (float)whichtexture * RecVariations;
        float uRight = (float)(whichtexture + 1) * RecVariations;
        unsigned int color = this->Color + ((int)(this->data.array[p].alpha * 255.0f) << 24);

        // Vertex 0: top-left
        vb[0].x = xpos - scale;
        vb[0].y = ypos - scale;
        vb[0].z = zbuf;
        vb[0].rhw = rhw;
        vb[0].diffuse = color;
        vb[0].specular = fog;
        vb[0].u = uLeft;
        vb[0].v = 0.0f;

        // Vertex 1: bottom-left
        vb[1].x = xpos - scale;
        vb[1].y = ypos + scale;
        vb[1].z = zbuf;
        vb[1].rhw = rhw;
        vb[1].diffuse = color;
        vb[1].specular = fog;
        vb[1].u = uLeft;
        vb[1].v = 1.0f;

        // Vertex 2: top-right
        vb[2].x = xpos + scale;
        vb[2].y = ypos - scale;
        vb[2].z = zbuf;
        vb[2].rhw = rhw;
        vb[2].diffuse = color;
        vb[2].specular = fog;
        vb[2].u = uRight;
        vb[2].v = 0.0f;

        // Vertex 3 = Vertex 2 (second triangle shares top-right)
        vb[3] = vb[2];

        // Vertex 4 = Vertex 1 (second triangle shares bottom-left)
        vb[4] = vb[1];

        // Vertex 5: bottom-right
        vb[5].x = xpos + scale;
        vb[5].y = ypos + scale;
        vb[5].z = zbuf;
        vb[5].rhw = rhw;
        vb[5].diffuse = color;
        vb[5].specular = fog;
        vb[5].u = uRight;
        vb[5].v = 1.0f;

        vb += 6;
        vertIdx++;
      }
    }
    this->Gepard->UnlockDynamicVB(this->DynamicVB);
    this->Gepard->DrawDynamicVB(this->DynamicVB, 2 * vertIdx);
  }

  this->Gepard->lpD3DDev->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
  return hasParticles || (this->StopTimeMoment > 0.0f && !this->Stopping);
}

//----- (004582F0) --------------------------------------------------------

int SParticles3::PrecacheObject(char *variable_name, SDrawType drawtype)
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

//----- (004583C0) --------------------------------------------------------

void SParticles3::RefreshIndices()
{
  char *MeshName = this->MeshName;
  if ( this->MeshName[0] )
  {
    int v3 = this->Object->GetMeshIndex(MeshName);
    this->TorkolatMeshIdx = v3;
    if ( v3 == -1 )
      Logger.g->Panic("SParticles3::RefreshIndices: mesh \"%s\" cannot be load!", MeshName);
  }
  else
  {
    this->TorkolatMeshIdx = -1;
  }
}

//----- (00458410) --------------------------------------------------------

void SParticles3::SetPosition(float _x, float _y, float _z)
{
  this->MainX = _x;
  this->MainY = _y;
  this->MainZ = _z;
}

//----- (00458440) --------------------------------------------------------

void SParticles3::StopEffect()
{
  this->Stopping = 1;
}
