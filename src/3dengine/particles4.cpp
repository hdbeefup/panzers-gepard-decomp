// 3dengine/particles4.cpp
// Particle system 4 (rotating fire/smoke columns)
// Decompiled from: gameSplit/sparticles.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <math.h>
#include <emmintrin.h>

#include "particles4.h"
#include "gepard.h"
#include "terrain.h"
#include "effects.h"
#include "properties.h"
#include "logger.h"

// Concert global (sound system) — now defined in effects.h

// SIObject — use full definition from iobject.h for correct vtable layout
#include "iobject.h"

// Transformed & Lit vertex for DrawPrimitiveUP (FVF 0x144 = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1)
struct TLVert4 {
    float x, y, z, rhw;
    unsigned int diffuse;
    float u, v;
};

// Classes: SParticles4
// Function count: 10

//----- (00458450) --------------------------------------------------------

SParticles4::SParticles4(char *classname, SEffect *effect, SGepard *gepard)
{
  SDrawType Int;
  char *String;
  int v15;
  int Float;
  char hang[260];
  char tmpstr[260];
  char fn[260];

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
  this->MainX = -1.0f;
  strncpy(this->ClassName, classname, sizeof(this->ClassName) - 1);
  this->ClassName[sizeof(this->ClassName) - 1] = '\0';
  this->Effect = effect;
  this->Gepard = gepard;
  this->Prototype = 1;
  Int = (SDrawType)effect->EffectsINI->GetInt(this->ClassName, "DrawType", -1);
  this->DrawType = Int;
  if ( Int == (SDrawType)-1 )
    Logger.g->Panic(
      "SParticles4::Initializations: A DrawType nincs megadva, vagy nulla van megadva.");
  String = this->Effect->EffectsINI->GetString(this->ClassName, "Texture", "error");
  strcpy(tmpstr, String);
  if ( !strcmp(tmpstr, "error") )
    Logger.g->Panic(
      "SParticles4::SParticles4: cannot load texture in class: \"%s\"",
      this->ClassName);
  strcpy(fn, "effects\\");
  strcat(fn, tmpstr);
#ifdef HDB_MISSING_ASSET_FALLBACK
  v15 = this->Gepard->LoadTextureOrPlaceholder(fn, 1, 1);
#else
  v15 = this->Gepard->LoadTexture(fn, 1, 1);
#endif
  this->THandle = v15;
  if ( v15 == -1 )
    Logger.g->Panic("SParticles4::SParticles4: cannot load texture \"%s\"", fn);
  Float = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "BorningEnabledTime", 0.0f);
  this->BorningEnabledTime = Float;
  this->BorningDisabledTime = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "BorningEnabledTime", 0.0f);
  this->BorningEnabledTimeRND = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "BorningEnabledTimeRND", 0.0f);
  this->BorningDisabledTimeRND = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "BorningDisabledTimeRND", 0.0f);
  this->ScaleSpeed = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "ScaleSpeed", 0.0f);
  this->ScaleSpeed = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "ScaleSpeed", 0.0f);
  this->MainR = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "MainR", 0.40000001f);
  this->RandomR = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "RandomR", 0.40000001f);
  this->ManageType = this->Effect->EffectsINI->GetInt(this->ClassName, "ManageType", 0);
  this->HSpeed = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "HSpeed", 0.0f);
  this->HSpeed_Rnd = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "HSpeed_Rnd", 0.0f);
  this->KisG = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "KisG", 0.0f);
  float vspeed = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "VSpeed", -1.0f);
  this->VSpeed = vspeed;
  if ( vspeed == -1.0f )
    Logger.g->Panic("SParticles4::Initializations: A VSpeed nincs megadva, vagy nulla van megadva.");
  float vspeed_rnd = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "VSpeed_Rnd", -1.0f);
  this->VSpeed_Rnd = vspeed_rnd;
  if ( vspeed_rnd == -1.0f )
    Logger.g->Panic(
      "SParticles4::Initializations: A VSpeed_Rnd nincs megadva, vagy nulla van megadva.");
  this->FadeOutSpeed = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "FadeOutSpeed", 0.0f);
  this->FadeOutSpeed_Rnd = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "FadeOutSpeed_Rnd", 0.0f);
  this->ColorR = this->Effect->EffectsINI->GetInt(this->ClassName, "ColorR", 0);
  this->ColorG = this->Effect->EffectsINI->GetInt(this->ClassName, "ColorG", 0);
  this->ColorB = this->Effect->EffectsINI->GetInt(this->ClassName, "ColorB", 0);
  this->StartAlpha = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "StartAlpha", 1.0f);
  this->StopTimeMoment = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "StopTimeMoment", 1.0f);
  this->TalajKoszolas = (int)this->Effect->EffectsINI->GetFloat(this->ClassName, "Talaj_Koszolas", 0.0f);
  this->BorningSpeed = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "Borning_Speed", 1.0f);
  this->Darabszam = this->Effect->EffectsINI->GetInt(this->ClassName, "Darabszam", 5);
  this->Scale = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "Scale", 1.0f);
  float variations = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "Variations", 1.0f);
  this->Variations = variations;
  this->RecVariations = 1.0f / variations;
  this->LensEffectLaunch = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "LensEffectLauch", 0.5f);
  this->FadeInSpeed = (float)this->Effect->EffectsINI->GetFloat(this->ClassName, "FadeInSpeed", 2.0f);
  String = this->Effect->EffectsINI->GetString(this->ClassName, "Sound", "error");
  strcpy(hang, String);
  if ( strcmp(hang, "error") )
  {
    int sh = Concert->PrecacheSound(hang, 0);
    this->SHandle = sh;
    Concert->AddRefToCachedSound(sh);
  }
  else
  {
    this->SHandle = -1;
  }
}

//----- (00458B00) --------------------------------------------------------

SParticles4::SParticles4(SParticles4 *p, float _x, float _y, float _z, float _scalespeed)
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
  this->MainX = -1.0f;
  this->MeshName[0] = 0;
  // Note: original dereferences NULL Object here — likely a decompiler artifact
  // v7 = this->Object->GetMeshIndex(this->MeshName);
  this->MeshIdx = 0;
  this->MainX = _x;
  this->MainY = _y;
  this->MainZ = _z;
  float ScaleSpeed = p->ScaleSpeed;
  if ( ScaleSpeed == 0.0f )
    ScaleSpeed = _scalespeed;
  this->ScaleSpeed = ScaleSpeed;
  this->CloneParticles(p);
}

//----- (00458C00) --------------------------------------------------------

SParticles4::SParticles4(SParticles4 *p, SIObject *object, char *meshname, int race)
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
  this->Race = race;
  this->MainX = -1.0f;
  this->ScaleSpeed = p->ScaleSpeed;
  this->Object = object;
  this->Object->AddRef();  // prevent use-after-free if unit dies while effect is active
  strncpy(this->MeshName, meshname, sizeof(this->MeshName) - 1);
  this->MeshName[sizeof(this->MeshName) - 1] = '\0';
  this->MeshIdx = this->Object->GetMeshIndex(this->MeshName);
  this->CloneParticles(p);
}

//----- (00458D10) --------------------------------------------------------

SParticles4::~SParticles4()
{
  if ( this->Object )
  {
    this->Object->Release();
    this->Object = 0;
  }
  if ( this->Prototype )
  {
    this->Gepard->ReleaseTexture(this->THandle, 0);
    Concert->ReleaseCachedSound(this->SHandle);
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

//----- (00458DC0) --------------------------------------------------------

void SParticles4::CloneParticles(SParticles4 *p)
{
  int BorningEnabledTime;
  bool v8;
  int SHandle;

  this->LensStarted = 0;
  this->sz = 0;
  strncpy(this->ClassName, p->ClassName, sizeof(this->ClassName) - 1);
  this->ClassName[sizeof(this->ClassName) - 1] = '\0';
  this->Effect = p->Effect;
  this->Gepard = p->Gepard;
  this->World = p->World;
  this->THandle = p->THandle;
  this->FadeOutSpeed = p->FadeOutSpeed;
  this->FadeOutSpeed_Rnd = p->FadeOutSpeed_Rnd;
  this->ColorR = p->ColorR;
  this->ColorG = p->ColorG;
  this->ColorB = p->ColorB;
  this->StartAlpha = p->StartAlpha;
  this->StopTimeMoment = p->StopTimeMoment;
  this->Variations = p->Variations;
  this->RecVariations = p->RecVariations;
  BorningEnabledTime = p->BorningEnabledTime;
  this->BorningEnabledTime = BorningEnabledTime;
  this->BorningDisabledTime = p->BorningDisabledTime;
  this->BorningEnabledTimeRND = p->BorningEnabledTimeRND;
  this->BorningDisabledTimeRND = p->BorningDisabledTimeRND;
  this->Stopping = 0;
  this->Darabszam = p->Darabszam;
  this->Scale = p->Scale;
  this->MainR = p->MainR;
  this->RandomR = p->RandomR;
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
  if ( BorningEnabledTime )
  {
    int v7 = rand();
    v8 = (int)(float)((float)((float)v7 * 0.000030517578f) + (float)((float)v7 * 0.000030517578f)) != 0;
  }
  else
  {
    v8 = 1;
  }
  this->BorningState = v8;
  this->BorningStateCounter = 0.0f;
  this->LensEffectLaunch = p->LensEffectLaunch;
  this->FadeInSpeed = p->FadeInSpeed;
  SHandle = p->SHandle;
  if ( SHandle != -1 )
    Concert->PlaySoundById(SHandle, 0.0f, 0.0f, -1); // IDA: PlaySound(handle, 0, 0, -1)
}

//----- (00458FA0) --------------------------------------------------------

bool SParticles4::MoveParticles(int a2)
{
  SIObject *Object;
  SGepard *Gepard;
  double Height;
  float MainX;
  SGepard *v8;
  STerrain *Terrain;
  unsigned char *VisMap;
  STerrain *v11;
  float v14;
  bool v15;
  float BorningStateCounter;
  float v22;
  float StopTimeMoment;
  float BorningCounter;
  int size, v26, maxsize, v29, v33;
  SParticles4Data *v30;
  int v31, v32;
  SGepard *v38;
  char v39;
  int v41;
  SParticles4Data *array;
  SVector head;
  unsigned int fog;
  float zbuf;
  float ypos;
  float scale;
  SVector pos;
  float xpos;
  float v81;
  char v82;
  bool v83;
  TLVert4 vert[4];

  Object = this->Object;
  if ( Object )
  {
    memset(&pos, 0, sizeof(pos));
    memset(&head, 0, sizeof(head));
    Object->GetMeshProperties(this->MeshIdx, &pos.x, &pos.y, &pos.z, &head.x, &head.y, &head.z);
    Gepard = this->Gepard;
    Height = Gepard->Terrain->GetHeight(pos.x, pos.z);
    MainX = pos.x;
    v81 = (float)Height + 0.1f;
    this->MainY = v81;
    this->MainX = MainX;
    this->MainZ = pos.z;
  }
  else
  {
    MainX = this->MainX;
    if ( MainX == -1.0f )
      return 0;
  }
  v8 = this->Gepard;
  if ( this->ManageType == 3 )
  {
    Terrain = v8->Terrain;
    VisMap = Terrain->VisMap;
    if ( VisMap
      && !Terrain->GodMode
      && !VisMap[(int)(float)(MainX + MainX) - 2 * (Terrain->XSize + 1) * (int)(float)(this->MainZ * -2.0f)] )
    {
      return 0;
    }
    this->ManageType = 0;
  }
  v11 = v8->Terrain;
  v14 = (float)((double)v8->ElapsedTime * 0.001);
  v81 = v14;
  unsigned char *visMap = v11->VisMap;
  v15 = !visMap
     || v11->GodMode
     || visMap[(int)(float)(MainX + MainX) - 2 * (v11->XSize + 1) * (int)(float)(this->MainZ * -2.0f)];
  v83 = v15;
  if ( this->BorningEnabledTime != 0 )
  {
    BorningStateCounter = this->BorningStateCounter;
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
      int v20 = rand();
      BorningStateCounter = (float)baseTime + (float)((float)((float)v20 / 32767.0f) * timeRnd);
    }
    v14 = v81;
    this->BorningStateCounter = BorningStateCounter - v81;
  }
  v22 = 1.0f;
  StopTimeMoment = this->StopTimeMoment;
  if ( StopTimeMoment > 0.0f && !this->Stopping && this->BorningState )
  {
    BorningCounter = (float)(this->BorningSpeed * v14) + this->BorningCounter;
    this->BorningCounter = BorningCounter;
    if ( BorningCounter > 1.0f )
    {
      do
      {
        BorningCounter = BorningCounter - v22;
        this->BorningCounter = BorningCounter;
        if ( v15 || this->ManageType != 2 )
        {
          size = this->data.size;
          v26 = 0;
          if ( size > 0 )
          {
            for (v26 = 0; v26 < size; v26++)
            {
              if ( !this->data.array[v26].active )
                break;
            }
          }
          if ( v26 == size )
          {
            maxsize = this->data.maxsize;
            if ( size == maxsize )
            {
              if ( maxsize >= 16 )
                v29 = 6 * maxsize / 5;
              else
                v29 = 16;
              v30 = (SParticles4Data *)realloc(this->data.array, 72 * v29);
              v31 = this->data.maxsize;
              v32 = v29 - v31;
              this->data.array = v30;
              memset(&v30[v31], 0, 72 * v32);
              size = this->data.size;
              this->data.maxsize = v29;
            }
            this->data.size = size + 1;
            if ( size != v26 )
              Logger.g->Panic("SParticles4::Play: what, ahhhhhhhhhh... :(");
          }
          v33 = v26;
          float rndVSpeed = this->VSpeed + (float)((float)((float)rand() / 32767.0f) * this->VSpeed_Rnd);
          this->data.array[v33].yspeed = rndVSpeed;
          this->data.array[v33].kisg = this->KisG;
          this->data.array[v33].randomr = (float)((float)rand() / 32767.0f) * this->RandomR;
          float hspeed = (float)(this->HSpeed + (float)((float)((float)rand() / 32767.0f) * this->HSpeed_Rnd)) - (float)(this->HSpeed_Rnd * 0.5f);
          float angle = (float)(360.0f - (float)((float)((float)rand() / 32767.0f) * 360.0f)) + 90.0f;
          while ( angle < 0.0f ) angle += 360.0f;
          while ( angle >= 360.0f ) angle -= 360.0f;
          float angle_rad = angle * 0.017453292f;
          this->data.array[v33].vectorx = cosf(angle_rad) * hspeed;
          this->data.array[v33].vectorz = sinf(angle_rad) * hspeed;
          this->data.array[v33].mainx = this->MainX;
          this->data.array[v33].y = this->MainY;
          this->data.array[v33].mainz = this->MainZ;
          this->data.array[v33].whichtexture = (int)(float)((float)((float)rand() * 0.000030517578f)
                                                          * (float)(int)this->Variations);
          this->data.array[v33].alpha = 0.0f;
          this->data.array[v33].fadein = 1;
          this->data.array[v33].fadeoutspeed = this->FadeOutSpeed + (float)((float)((float)rand() / 32767.0f) * this->FadeOutSpeed_Rnd);
          v22 = 1.0f;
          this->data.array[v33].rotcounter = (float)((float)rand() / 32767.0f) * 6.283f;
          this->data.array[v33].scale = this->Scale;
          this->data.array[v33].active = 1;
          BorningCounter = this->BorningCounter;
          v15 = v83;
        }
      }
      while ( BorningCounter > v22 );
      StopTimeMoment = this->StopTimeMoment;
    }
  }
  v38 = this->Gepard;
  v39 = 0;
  v82 = 0;
  this->StopTimeMoment = StopTimeMoment - v81;
  v38->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 0);
  this->Gepard->lpD3DDev->SetFVF(324u);
  this->Gepard->SetDrawType(DT_ADD);
  this->Gepard->SetTexture(0, this->THandle, 1);
  if ( this->data.size > 0 )
  {
    v41 = 0;
    do
    {
      array = this->data.array;
      if ( array[v41].active )
      {
        float dt = v81;
        v82 = 1;
        array[v41].yspeed = array[v41].yspeed - array[v41].kisg * dt;
        float rotAngle = this->data.array[v41].rotcounter;
        float cosRot = cosf(rotAngle);
        this->data.array[v41].x = (float)(cosRot * (float)(this->data.array[v41].randomr + this->MainR)) + this->MainX;
        float sinRot = sinf(rotAngle);
        this->data.array[v41].z = (float)(sinRot * (float)(this->data.array[v41].randomr + this->MainR)) + this->MainZ;
        this->data.array[v41].y = (float)(this->data.array[v41].yspeed * dt) + this->data.array[v41].y;
        this->data.array[v41].rotcounter = (float)(dt * 3.0f) + this->data.array[v41].rotcounter;
        this->data.array[v41].scale = (float)(this->ScaleSpeed * dt) + this->data.array[v41].scale;
        SParticles4Data *pd = &this->data.array[v41];
        if ( pd->fadein )
        {
          pd->alpha = (float)(this->FadeInSpeed * dt) + pd->alpha;
          float StartAlpha = this->StartAlpha;
          if ( this->data.array[v41].alpha >= StartAlpha )
          {
            this->data.array[v41].alpha = StartAlpha;
            this->data.array[v41].fadein = 0;
          }
        }
        else
        {
          pd->alpha = pd->alpha - (float)(pd->fadeoutspeed * dt);
          if ( this->data.array[v41].alpha < 0.0f )
          {
            this->data.array[v41].alpha = 0.0f;
            this->data.array[v41].active = 0;
          }
        }
        this->Gepard->TransformScaledPointAdd(
          this->data.array[v41].x, this->data.array[v41].y, this->data.array[v41].z,
          this->data.array[v41].scale, &xpos, &ypos, &scale, &zbuf, &fog);
        SParticles4Data *v54 = &this->data.array[v41];
        vert[0].x = xpos - scale;
        vert[1].x = xpos - scale;
        vert[2].x = xpos + scale;
        vert[3].x = xpos + scale;
        vert[0].y = ypos - scale;
        vert[2].y = ypos - scale;
        float RecVariations = this->RecVariations;
        vert[1].y = ypos + scale;
        vert[3].y = ypos + scale;
        int whichtexture = v54->whichtexture;
        vert[0].v = 0.0f;
        vert[2].v = 0.0f;
        vert[1].v = 1.0f;
        vert[3].v = 1.0f;
        vert[0].rhw = 1.0f;
        vert[1].rhw = 1.0f;
        vert[2].rhw = 1.0f;
        vert[3].rhw = 1.0f;
        vert[0].u = (float)whichtexture * RecVariations;
        float uNext = (float)(whichtexture + 1) * RecVariations;
        vert[0].z = zbuf;
        vert[1].z = zbuf;
        vert[2].z = zbuf;
        vert[3].z = zbuf;
        vert[1].u = vert[0].u;
        unsigned int ColorR = this->ColorR;
        vert[2].u = uNext;
        vert[3].u = uNext;
        float alphaMul = (float)((float)((double)(unsigned char)fog + 0.0) * v54->alpha) * 0.00392156862745098f;
        int ri = (int)(float)((float)ColorR * alphaMul);
        float ColorG = (float)this->ColorG;
        vert[0].diffuse = (((int)(float)(ColorG * alphaMul) + (ri << 8)) << 8) + (int)(float)((float)this->ColorB * alphaMul);
        vert[1].diffuse = vert[0].diffuse;
        vert[2].diffuse = vert[0].diffuse;
        vert[3].diffuse = vert[0].diffuse;
        if ( v83 || this->ManageType != 1 )
          this->Gepard->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2u, vert, 28u);
      }
      ++v41;
    }
    while ( v41 < this->data.size );
    v39 = v82;
  }
  this->Gepard->EnableFog();
  if ( (this->StopTimeMoment <= this->LensEffectLaunch || this->Stopping) && !this->LensStarted )
  {
    SIObject *v63 = this->Object;
    if ( v63 && !this->Stopping )
    {
      this->Effect->PlayEffect(this->Gepard->_levelup_lens, v63, this->MeshName, this->Race);
      this->LensStarted = 1;
    }
  }
  return v39 || (this->StopTimeMoment > 0.0f && !this->Stopping);
}

//----- (00459A90) --------------------------------------------------------

int SParticles4::PrecacheObject(char *variable_name, SDrawType drawtype)
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

//----- (00459B60) --------------------------------------------------------

void SParticles4::RefreshIndices()
{
  this->MeshIdx = this->Object->GetMeshIndex(this->MeshName);
}

//----- (00459B80) --------------------------------------------------------

void SParticles4::SetPosition(float _x, float _y, float _z)
{
  this->MainX = _x;
  this->MainY = _y;
  this->MainZ = _z;
}

//----- (00459BB0) --------------------------------------------------------

void SParticles4::StopEffect()
{
  this->Stopping = 1;
}
