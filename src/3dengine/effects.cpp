// 3dengine/effects.cpp
// Visual effects — rain, lake, river, level-up lens
// Decompiled from: gameSplit/seffect.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <new>

#include "effects.h"
#include "iobject.h"
#include "gepard.h"
#include "glow.h"
#include "particles.h"
#include "particles2.h"
#include "particles3.h"
#include "particles4.h"
#include "logger.h"
#include "stream.h"
#include "properties.h"
#include "core_common.h"
#include "terrain.h"
#include "hdbeefup.h"

#define LT_AMBIENT 1
#define LT_PRELIT 2

// Classes: SEffect, SEryngo, SLevelUpLens, SRain, SLake, SRiver
// Function count: 50

//----- (00421D80) --------------------------------------------------------

SEffect::SEffect(SGepard *gepard)

{
  SProperties *v3; // eax
  this->EffectsLoaded.array = 0;
  this->EffectsLoaded.size = 0;
  this->EffectsLoaded.maxsize = 0;
  this->EffectsLoaded.nextempty = -1;
  this->EffectsLoaded.occupied = 0;
  this->EffectsPlaying.first = 0;
  this->EffectsPlaying.last = 0;
  this->EffectsPlaying.current = 0;
  this->EffectsPlaying.Closed = 0;
  this->EffectsPlaying.NumItems = 0;
  this->Gepard = gepard;
  this->lpD3DDev = gepard->lpD3DDev;
  v3 = new SProperties("effects.ini", 1, 1);
  this->EffectsINI = v3;
}

//----- (00421E60) --------------------------------------------------------

SEryngo::SEryngo(SEryngo *p, SIObject *object)

{
  float i; // xmm0_4
  this->Gepard = p->Gepard;
  this->Effect = p->Effect;
  this->Object = object;
  for ( i = (float)((float)((float)rand() / 32767.0) - 0.5) + this->Gepard->GlobalisSzelirany; i < 0.0; i = i + 6.2831855f )
    ;
  for ( ; i >= 6.2831855f; i = i - 6.2831855f )
    ;
  this->LokalisSzelirany = i;
  this->SzelIrany1 = i;
  this->SzelIrany1_b = i;
  this->SzelEro1 = 0.0;
  this->SzelMagassag1 = 0.80000001f;
  this->SzelTime1 = 0.0;
  this->SzelLokesMode1 = 1;
  this->SzelSpinDir1 = 0.0;
  this->Porzas = 0;
}

//----- (00421F10) --------------------------------------------------------

SEryngo::SEryngo(SGepard *gepard, SEffect *effect, char *classname)

{
  this->Gepard = gepard;
  this->Effect = effect;
}

//----- (00421F30) --------------------------------------------------------

SLevelUpLens::SLevelUpLens(SLevelUpLens *p, SIObject *object, char *meshname, int race)

{
  char *v6; // ecx
  char v7; // al
  int v8;
  v6 = meshname;
  *(_WORD *)&this->Prototype = 0;
  this->Effect = p->Effect;
  this->Gepard = p->Gepard;
  this->MainX = -1.0;
  this->Object = object;
  do
  {
    v7 = *v6++;
    v6[this->MeshName - meshname - 1] = v7;
  }
  while ( v7 );
  v8 = this->Object->GetMeshIndex(this->MeshName);
  this->MeshIdx = v8;
  if ( v8 == -1 )
#ifdef HDB_MISSING_ASSET_FALLBACK
  {
    // Likely the unit's model resolved to editor/missing.4d. Null Object;
    // Move's `if (Object)` guard skips rendering cleanly.
    Logger.g->Log(0, "SLevelUpLens: mesh '%s' missing in object -> skipping effect", this->MeshName);
    this->Object = 0;
  }
#else
    Logger.g->Panic("SLevelUpLens::RefreshIndices: cannot find mesh in object");
#endif
  this->Race = race;
  this->THandle1 = p->THandle1;
  this->THandle2 = p->THandle2;
  this->THandle3 = p->THandle3;
  this->Scale1 = p->Scale1;
  this->Scale2 = p->Scale2;
  this->Scale3 = p->Scale3;
  this->Rotate1 = 0.0;
  this->Rotate2 = 0.0;
  this->Rotate3 = 0.0;
  this->Alpha1 = 0.0;
  this->Alpha2 = 0.0;
}

//----- (00421FF0) --------------------------------------------------------

SLevelUpLens::SLevelUpLens(SGepard *gepard, SEffect *effect, char *classname)

{
  char *String; // ecx
  int v8;
  char *v9; // ecx
  int v12;
  char *v13; // ecx
  int v16;
  double Float; // st7
  SEffect *v18; // ecx
  double v19; // st7
  SEffect *v20; // ecx
  char tmpstr[260];
  char fn[260];
  this->Prototype = 1;
  this->Effect = effect;
  this->Gepard = gepard;
  String = effect->EffectsINI->GetString(classname, "Texture1", "error.tga");
  strcpy(tmpstr, String);
  sprintf(fn, "effects\\%s", tmpstr);
#ifdef HDB_MISSING_ASSET_FALLBACK
  v8 = this->Gepard->LoadTextureOrPlaceholder(fn, 0, 0);
#else
  v8 = this->Gepard->LoadTexture(fn, 0, 0);
#endif
  this->THandle1 = v8;
  if ( v8 == -1 )
    Logger.g->Panic("SLevelUpLens::SLevelUpLens: Cannot load texture1 \"%s\"", fn);
  v9 = this->Effect->EffectsINI->GetString(classname, "Texture2", "error.tga");
  strcpy(tmpstr, v9);
  sprintf(fn, "effects\\%s", tmpstr);
#ifdef HDB_MISSING_ASSET_FALLBACK
  v12 = this->Gepard->LoadTextureOrPlaceholder(fn, 0, 0);
#else
  v12 = this->Gepard->LoadTexture(fn, 0, 0);
#endif
  this->THandle2 = v12;
  if ( v12 == -1 )
    Logger.g->Panic("SLevelUpLens::SLevelUpLens: Cannot load texture2 \"%s\"", fn);
  v13 = this->Effect->EffectsINI->GetString(classname, "Texture3", "error.tga");
  strcpy(tmpstr, v13);
  sprintf(fn, "effects\\%s", tmpstr);
#ifdef HDB_MISSING_ASSET_FALLBACK
  v16 = this->Gepard->LoadTextureOrPlaceholder(fn, 0, 0);
#else
  v16 = this->Gepard->LoadTexture(fn, 0, 0);
#endif
  this->THandle3 = v16;
  if ( v16 == -1 )
    Logger.g->Panic("SLevelUpLens::SLevelUpLens: Cannot load texture3 \"%s\"", fn);
  Float = this->Effect->EffectsINI->GetFloat(classname, "Scale1", 1.0);
  v18 = this->Effect;
  this->Scale1 = (float)(Float);

  v19 = v18->EffectsINI->GetFloat(classname, "Scale2", 1.0);
  v20 = this->Effect;
  this->Scale2 = (float)(v19);

  this->Scale3 = v20->EffectsINI->GetFloat(classname, "Scale3", 1.0);
}

//----- (004221F0) --------------------------------------------------------

SRain::SRain(SGepard *gepard, SEffect *effect, char *classname)

{
  int v5;
  float *p_y; // esi
  int v7;
  int geparda;
  this->Gepard = gepard;
  this->Effect = effect;
  v5 = gepard->LoadTexture("effects\\esocsepp_a.tga", 0, 0);
  this->THandle = v5;
  if ( v5 == -1 )
    Logger.g->Panic("SRain::SRain: cannot load esocsepp_a.tga!");
  p_y = &this->EsoCseppek[0][0].y;
  geparda = 100;
  do
  {
    v7 = 14;
    do
    {
      *(p_y - 1) = (float)((float)rand() / 32767.0f) * 8.0f;
      *p_y = (float)((float)rand() / 32767.0f) * 12.0f;
      p_y[1] = (float)((float)rand() / 32767.0f) * 8.0f;
      p_y[2] = (float)((float)((float)rand() / 32767.0f) * 4.0f) + 17.0f;
      p_y += 4;
      --v7;
    }
    while ( v7 );
    --geparda;
  }
  while ( geparda );
}

//----- (004226B0) --------------------------------------------------------

SEffect::~SEffect()

{
  SEffectDesc2 *first; // eax
  SEffectDesc2 *current; // eax
  SEffectDesc2 *next; // edi
  SEffectDesc2 *prev; // eax
  int i;
  int size;
  SHeap<SEffectDesc>::__Tstruct *v8; // eax
  SProperties *EffectsINI; // edi
  SHeap<SEffectDesc>::__Tstruct *array; // ecx
  int v11;
  SHeap<SEffectDesc>::__Tstruct *v12; // edx
  SParticles *userdata; // eax
  int v16;
  SParticles2 *v19; // eax
  SParticles3 *v20; // eax
  int v21;
  SGlow *v23; // eax
  SParticles4 *v24; // eax
  SLevelUpLens *v25; // ecx
  SHeap<SEffectDesc>::__Tstruct *v27; // ecx
  SEffectDesc2 *block;
  void *blocka;
  unsigned int v32;
  int v33;
  first = this->EffectsPlaying.first;
  this->EffectsPlaying.current = first;
  while ( this->EffectsPlaying.current )
  {
    this->CleanUpPointersInCurrentPlayingEffect();
    current = this->EffectsPlaying.current;
    if ( current )
    {
      next = current->next;
      prev = current->prev;
      block = prev;
      if ( next )
        next->prev = prev;
      if ( prev )
        prev->next = next;
      operator delete(this->EffectsPlaying.current);
      if ( next )
      {
        this->EffectsPlaying.current = next;
      }
      else
      {
        this->EffectsPlaying.last = block;
        this->EffectsPlaying.current = 0;
      }
      if ( !block )
        this->EffectsPlaying.first = next;
      --this->EffectsPlaying.NumItems;
    }
  }
  for ( i = -1; ; this->EffectsLoaded.nextempty = i )
  {
    size = this->EffectsLoaded.size;
    v33 = ++i;
    if ( i >= size )
      break;
    v8 = &this->EffectsLoaded.array[i];
    while ( v8->use != 0x7FFFFFFF )
    {
      ++i;
      ++v8;
      v33 = i;
      if ( i >= size )
        goto LABEL_19;
    }
    if ( i < 0 )
      break;
    array = this->EffectsLoaded.array;
    v11 = 12 * i;
    v12 = &array[i];
    v32 = i;
    switch ( v12->data.type )
    {
      case 1:
        userdata = (SParticles *)v12->data.userdata;
        blocka = userdata;
        if ( userdata )
        {
          userdata->~SParticles();
          goto LABEL_64;
        }
        break;
      case 2:
      {
        // x64 fix: original cast userdata to `_DWORD *` and indexed
        // [1]..[6] expecting x86 SShaderLight layout (s2i@0/4B,
        // thandle@4, thandle0..4@8..27). On x64 s2i is 8 bytes so the
        // indices are off by one int — we'd release thandle..thandle3
        // (missing thandle4) and probe thandle0 instead of the
        // multitileset sentinel. Use typed access.
        SShaderLight *data = (SShaderLight *)v12->data.userdata;
        if ( data->thandle0 == -1 )
        {
          this->Gepard->ReleaseTexture(data->thandle, 0);
        }
        else
        {
          this->Gepard->ReleaseTexture(data->thandle0, 0);
          this->Gepard->ReleaseTexture(data->thandle1, 0);
          this->Gepard->ReleaseTexture(data->thandle2, 0);
          this->Gepard->ReleaseTexture(data->thandle3, 0);
          this->Gepard->ReleaseTexture(data->thandle4, 0);
        }
        operator delete(data);
        break;
      }
      case 3:
        // x64 fix: typed SBoom access. Original `_DWORD *userdata` indexing
        // assumed 4-byte pointers (`userdata[2]` == `thandles.array` at byte 8
        // on x86). On x64 SDArray<int>::array is 8 bytes, so the original
        // `v17[2]` read byte 16 (SBoom.framenr/frameinc) — uninitialized when
        // count==0, causing free() of CDCDCD garbage.
        {
          SBoom *boom = (SBoom *)this->EffectsLoaded.array[v32].data.userdata;
          if ( boom )
          {
            for ( v16 = 0; v16 < boom->thandles.size; ++v16 )
              this->Gepard->ReleaseTexture(boom->thandles.array[v16], 0);
            if ( boom->thandles.array )
              free(boom->thandles.array);
            ::operator delete(boom);
          }
        }
        break;
      case 4:
      case 0xC:
        this->Gepard->ReleaseTexture(*(_DWORD *)v12->data.userdata, 0);
        // x64 fix: typed array access. Original `(char *)&array->data.userdata + v11`
        // assumed sizeof(Element)==12 (x86); on x64 it's 24 (4 use + 4 pad + 4 type
        // + 4 pad + 8 userdata).
        operator delete(this->EffectsLoaded.array[v32].data.userdata);
        break;
      case 5:
      {
        // x64 fix: original `*(_DWORD*)userdata` truncated SRain::Gepard
        // pointer to 4 bytes and `userdata[2]` (=byte 8) read THandle on
        // x86 but Effect's lower half on x64 (Effect at byte 8 on x64).
        SRain *rain = (SRain *)v12->data.userdata;
        blocka = rain;
        if ( rain )
        {
          rain->Gepard->ReleaseTexture(rain->THandle, 0);
          goto LABEL_64;
        }
        break;
      }
      case 6:
        v19 = (SParticles2 *)v12->data.userdata;
        blocka = v19;
        if ( v19 )
        {
          v19->~SParticles2();
          goto LABEL_64;
        }
        break;
      case 7:
        v20 = (SParticles3 *)v12->data.userdata;
        blocka = v20;
        if ( v20 )
        {
          v20->~SParticles3();
          goto LABEL_64;
        }
        break;
      case 8:
        // x64 fix: typed STT access (same pattern as case 3 above).
        {
          STT *tt = (STT *)this->EffectsLoaded.array[v32].data.userdata;
          if ( tt )
          {
            for ( v21 = 0; v21 < tt->thandles.size; ++v21 )
              this->Gepard->ReleaseTexture(tt->thandles.array[v21], 0);
            if ( tt->thandles.array )
              free(tt->thandles.array);
            ::operator delete(tt);
          }
        }
        break;
      case 9:
      case 0xA:
      case 0x10:
        operator delete(v12->data.userdata);
        break;
      case 0xB:
        v23 = (SGlow *)v12->data.userdata;
        blocka = v23;
        if ( v23 )
        {
          v23->~SGlow();
          goto LABEL_64;
        }
        break;
      case 0xD:
        v24 = (SParticles4 *)v12->data.userdata;
        blocka = v24;
        if ( v24 )
        {
          v24->~SParticles4();
          goto LABEL_64;
        }
        break;
      case 0xE:
        v25 = (SLevelUpLens *)v12->data.userdata;
        if ( v25 )
          delete v25;
        break;
      case 0xF:
      {
        // x64 fix: `(_DWORD*)userdata + 7` was byte 28 = NagyajtoHang on
        // x86 but on x64 it's the upper half of the Ablakok pointer
        // (Effect/Gepard/Object/Ablakok at bytes 0..31). Use typed access.
        SWindowEffect *we = (SWindowEffect *)v12->data.userdata;
        Concert->ReleaseCachedSound(we->NagyajtoHang);
        operator delete(we);
        break;
      }
      case 0x11:
      {
        // x64 fix: same family as case 5 — `*(_DWORD*)userdata` truncated
        // SSnowfall::Gepard and `userdata[4]` (byte 16) read THandle on
        // x86 but Effect lower half on x64 (Effect at byte 8 on x64).
        SSnowfall *snow = (SSnowfall *)v12->data.userdata;
        blocka = snow;
        if ( snow )
        {
          snow->Gepard->ReleaseTexture(snow->THandle, 0);
LABEL_64:
          ::operator delete(blocka);
        }
        break;
      }
      default:
        break;
    }
    // x64 fix: typed array indexing. Original `(char *)&v27->use + v11` with
    // `v11 = 12 * i` is x86-only stride math (sizeof(Element)==12 on x86, 24 on x64).
    // This is the SHeap::Remove validation/free-list update; same correctness check,
    // typed access.
    if ( i >= this->EffectsLoaded.size
      || (v27 = this->EffectsLoaded.array, v27[v32].use != 0x7FFFFFFF) )
    {
      Logger.g->Panic("SHeap::Remove: invalid index (%d)", i);
    }
    v27[v32].use = this->EffectsLoaded.nextempty;
    --this->EffectsLoaded.occupied;
  }
LABEL_19:
  EffectsINI = this->EffectsINI;
  if ( this->EffectsINI )
  {
    delete EffectsINI;
    this->EffectsINI = 0;
  }
  this->EffectsPlaying.~SChain();
  if ( this->EffectsLoaded.array )
    free(this->EffectsLoaded.array);
}

//----- (00422B50) --------------------------------------------------------

SLevelUpLens::~SLevelUpLens()

{
  if ( this->Prototype )
  {
    this->Gepard->ReleaseTexture(this->THandle1, 0);
    this->Gepard->ReleaseTexture(this->THandle2, 0);
    this->Gepard->ReleaseTexture(this->THandle3, 0);
  }
}

//----- (00422BC0) --------------------------------------------------------

SRain::~SRain()

{
  this->Gepard->ReleaseTexture(this->THandle, 0);
}

// scalar deleting destructor — compiler-generated, replaced with delete calls at call sites
#if 0
SLevelUpLens *SLevelUpLens_scalar_deleting_destructor(SLevelUpLens *self, char a2)
{
  self->~SLevelUpLens();
  if ( (a2 & 1) != 0 )
    delete self;
  return self;
}
#endif

//----- (00422CF0) --------------------------------------------------------

void SEffect::AddGlow(SGlow *glow, float x, float y, float z, int r, int g, int b, int scale, float fadeing)

{
  glow->AddGlow(x, y, z, r, g, b, scale, fadeing);
}

//----- (00422D40) --------------------------------------------------------

void SEffect::CleanUpPointersInCurrentPlayingEffect()

{
  SEffectDesc2 *current; // esi
  int v3;
  SParticles *userdata; // esi
  SParticles2 *v6; // esi
  int v7;
  SParticles3 *v8; // esi
  int v9;
  SParticles4 *v10; // esi
  int v11;
  SLevelUpLens *v12; // ecx
  current = this->EffectsPlaying.current;
  switch ( current->type )
  {
    case 1:
      if ( current->effectringofobject )
      {
        // x64: original `*((_DWORD*)userdata + 72)` is byte-288 offset of
        // SParticles.Object on x86. SParticles has 13 leading SDArrays, each
        // grew 12→16 on x64 (+52 total), so Object lands at byte 360. Reading
        // 4 bytes at offset 288 returns garbage. Same fix family as case 7/0xD.
        SIObject *obj = ((SParticles *)current->userdata)->Object;
        if ( obj )
        {
          obj->ClearEffect(current->effectringofobject);
          current = this->EffectsPlaying.current;
        }
        v3 = (int)(intptr_t)obj;
      }
      userdata = (SParticles *)current->userdata;
      if ( userdata )
      {
        userdata->~SParticles();
        ::operator delete(userdata);
      }
      break;
    case 2:
      this->Gepard->DestroyShader2(*(SShader2Info **)current->userdata);
      operator delete(this->EffectsPlaying.current->userdata);
      break;
    case 3:
    {
      // x64 fix: original `(void**)userdata` indexed at `v5[2]` lands at
      // byte 16 on x64 (= framenr+frameinc floats) instead of byte 8
      // (=thandles.array on x86). free() got two floats interpreted as
      // a pointer (e.g. 0x41c800004190ccce = [18.1f, 25.0f]) and faulted
      // inside ucrtbased. Use typed SBoom — mirrors PlayEffect/MoveEffects
      // case 3 typed access.
      SBoom *boom = (SBoom *)current->userdata;
      if ( boom )
      {
        if ( boom->thandles.array )
        {
          free(boom->thandles.array);
          boom->thandles.array = nullptr;
        }
        ::operator delete(boom);
      }
      break;
    }
    case 4:
    case 0xA:
    case 0xC:
      goto LABEL_41;
    case 6:
      v6 = (SParticles2 *)current->userdata;
      if ( v6 )
      {
        v6->~SParticles2();
        ::operator delete(v6);
      }
      break;
    case 7:
      #ifdef HD_DEBUG_EFFECTS
      Logger.g->Log(0, "(diag) CleanUp: destroying type 7 effect at %p", current);
      #endif
      if ( current->effectringofobject )
      {
        // x64 fix: original `*((_DWORD *)userdata + 39)` is the byte-156
        // offset of SParticles3.Object on x86. On x64 the embedded SDArrays
        // grow (12->16 bytes each, +12 total) so Object lands at offset 168;
        // reading 4 bytes at offset 156 returns junk (a float field) and
        // the dereference crashed with `0xFFFFFFFFCDCDCDCD`. Typed access.
        SIObject *obj = ((SParticles3 *)current->userdata)->Object;
        if ( obj )
        {
          obj->ClearEffect(current->effectringofobject);
          current = this->EffectsPlaying.current;
        }
        v7 = (int)(intptr_t)obj;
      }
      v8 = (SParticles3 *)current->userdata;
      if ( v8 )
      {
        v8->~SParticles3();
        ::operator delete(v8);
      }
      break;
    case 9:
      ((SLokeshullam *)current->userdata)->object->Release();
      operator delete(this->EffectsPlaying.current->userdata);
      break;
    case 0xD:
      if ( current->effectringofobject )
      {
        // x64 fix: same _DWORD truncation as case 7 — SParticles4.Object
        // is at byte offset 160 on x86, ~172 on x64.
        SIObject *obj = ((SParticles4 *)current->userdata)->Object;
        if ( obj )
        {
          obj->ClearEffect(current->effectringofobject);
          current = this->EffectsPlaying.current;
        }
        v9 = (int)(intptr_t)obj;
      }
      v10 = (SParticles4 *)current->userdata;
      if ( v10 )
      {
        v10->~SParticles4();
        ::operator delete(v10);
      }
      break;
    case 0xE:
      if ( current->effectringofobject )
      {
        // x64 fix: same pattern — SLevelUpLens.Object at byte offset 68 on
        // x86 (`+ 17` ints), shifts on x64 because the leading SEffect*/SGepard*
        // pointers grow.
        SIObject *obj = ((SLevelUpLens *)current->userdata)->Object;
        if ( obj )
        {
          obj->ClearEffect(current->effectringofobject);
          current = this->EffectsPlaying.current;
        }
        v11 = (int)(intptr_t)obj;
      }
      v12 = (SLevelUpLens *)current->userdata;
      if ( v12 )
        delete v12;
      break;
    case 0xF:
    {
      // x64 fix: original `(_DWORD*)userdata + 3` was byte 12 (=Ablakok ptr
      // on x86); on x64 byte 12 is the upper half of Object. Same family
      // as the case 3 SBoom cleanup. Plus `(_DWORD*)userdata + 2` was the
      // Object ptr on x86 (byte 8) but on x64 byte 8 is Gepard's lower
      // half. SWindowEffect (effects.h:251) starts with four pointers that
      // grew 4->8 each on x64. Preserve the ablakok ownership-transfer
      // semantics (free both the SDArray and its inner `array`) — the
      // caller of PlayEffect-3 hands ownership to the effect.
      SWindowEffect *w = (SWindowEffect *)current->userdata;
      SDArray<SAblak> *ablakok = w ? w->Ablakok : nullptr;
      if ( ablakok )
      {
        if ( ablakok->array )
        {
          free(ablakok->array);
          ablakok->array = nullptr;
        }
        ::operator delete(ablakok);
        if ( w )
          w->Ablakok = nullptr;
      }
      SEffectDesc2 *playing = this->EffectsPlaying.current;
      if ( playing->effectringofobject )
      {
        SIObject *obj = ((SWindowEffect *)playing->userdata)->Object;
        if ( obj )
        {
          obj->ClearEffect(playing->effectringofobject);
          playing = this->EffectsPlaying.current;
        }
      }
      operator delete(playing->userdata);
      break;
    }
    case 0x10:
      if ( current->effectringofobject )
      {
        // x64 fix: original `(_DWORD*)userdata + 2` was byte 8 = Object on
        // x86; on x64 byte 8 is Gepard's lower half (SEryngo starts with
        // SEffect*, SGepard*, SIObject*). Plus the read truncated the
        // pointer to 4 bytes. Mirror cases 1/7/0xD/0xE typed access.
        SIObject *obj = ((SEryngo *)current->userdata)->Object;
        if ( obj )
        {
          obj->ClearEffect(current->effectringofobject);
          current = this->EffectsPlaying.current;
        }
      }
LABEL_41:
      operator delete(current->userdata);
      break;
    default:
      return;
  }
}

//----- (00423000) --------------------------------------------------------

void SEffect::ClearGlow(SGlow *glow)

{
  glow->ClearGlow();
}

//----- (00423010) --------------------------------------------------------

SBoom *SEffect::Clone_Boom(SBoom *data, float _x, float _y, float _z)

{
  SBoom *v5; // eax
  SBoom *v6; // esi
  SBoom *v7; // edx
  int i;
  int maxsize;
  int v10;
  char *v11; // eax
  int v12;
  SBoom *result; // eax
  v5 = (SBoom *)operator new(sizeof(SBoom));
  v6 = v5;
  if ( v5 )
  {
    v5->thandles.array = 0;
    v5->thandles.size = 0;
    v5->thandles.maxsize = 0;
  }
  else
  {
    v6 = 0;
  }
  v7 = data;
  for ( i = 0; i < v7->thandles.size; ++i )
  {
    maxsize = v6->thandles.maxsize;
    if ( v6->thandles.size == maxsize )
    {
      if ( maxsize >= 16 )
        v10 = 6 * maxsize / 5;
      else
        v10 = 16;
      v11 = (char *)realloc(v6->thandles.array, 4 * v10);
      v12 = v6->thandles.maxsize;
      v6->thandles.array = (int *)v11;
      memset(&v11[4 * v12], 0, 4 * (v10 - v12));
      v7 = data;
      v6->thandles.maxsize = v10;
    }
    ++v6->thandles.size;
    v6->thandles.array[i] = v7->thandles.array[i];
  }
  result = v6;
  v6->x = _x;
  v6->y = _y;
  v6->frameinc = 25.0;
  v6->framenr = 0.0;
  v6->z = _z;
  v6->scale = v7->scale;
  return result;
}

//----- (00423100) --------------------------------------------------------

SGhost *SEffect::Clone_Ghost(SGhost *data, float _x, float _y, float _z)

{
  SGhost *result; // eax
  result = (SGhost *)operator new(sizeof(SGhost));
  result->thandle = data->thandle;
  result->x = _x;
  result->y = _y;
  *(_QWORD *)&result->z = LODWORD(_z);
  result->eltunes_kezdete = 0.5;
  result->eltunes_sebesseg = 0.25;
  result->felfutas_sebesseg = 2.0;
  result->nagyitas_sebesseg = 0.5;
  result->aktualis_meret = 0.2f;
  result->felszallas_sebesseg = 0.5;
  result->magassag = 0.0;
  result->kiteres_mertek = 0.0;
  result->kiteres_sebesseg = 4.0;
  result->orix = _x;
  result->oriz = _z;
  return result;
}

//----- (00423190) --------------------------------------------------------

SLighting *SEffect::Clone_Lighting(SLighting *data, float _x, float _y, float _z)

{
  SLighting *result; // eax
  result = (SLighting *)operator new(sizeof(SLighting));
  qmemcpy(result, data, sizeof(SLighting));
  return result;
}

//----- (004231C0) --------------------------------------------------------

SLokeshullam *SEffect::Clone_Lokeshullam(SLokeshullam *data, float _x, float _y, float _z)

{
  SLokeshullam *v6; // ebx
  SIObject *v7; // eax
  v6 = (SLokeshullam *)operator new(sizeof(SLokeshullam));
  *v6 = *data;
  v7 = this->Gepard->CreateObjectByIndex(data->Prototype, 0);
  v6->object = v7;
  v7->SetPosition(_x, _y, _z);
  return v6;
}

//----- (00423230) --------------------------------------------------------

SReflektor *SEffect::Clone_Reflektor(SReflektor *data, SIObject *object)

{
  SReflektor *v3; // esi
  int v4;
  v3 = (SReflektor *)operator new(sizeof(SReflektor));
  *v3 = *data;
  v3->object = object;
  v3->managetype = data->managetype;
  v4 = object->GetMeshIndex("test");
  v3->meshidx = v4;
  if ( v4 == -1 )
#ifdef HDB_MISSING_ASSET_FALLBACK
  {
    Logger.g->Log(0, "SEffect::Clone_Reflektor: no \"test\" mesh in object -> skipping effect");
    v3->object = 0;
  }
#else
    Logger.g->Panic("SEffect::Reflektor_RefreshIndices: the object has no \"test\" mesh!");
#endif
  return v3;
}

//----- (00423280) --------------------------------------------------------

SShaderLight *SEffect::Clone_ShaderLight(SShaderLight *data, float _x, float _y, float _z)

{
  SShaderLight *v6; // eax
  SShaderLight *v7; // esi
  int thandle0;
  SDrawType drawtype;
  SShaderLight *result; // eax
  // Machinima deep state: skip ground-decal effect creation. Keeps tire-track
  // / wheel-mark / light-projection Shader2Infos out of recorded shots; the
  // unit moves normally, just no decal baked into the scene.
  if ( g_HideWorldOverlays )
    return 0;
  v6 = (SShaderLight *)operator new(sizeof(SShaderLight));
  v7 = v6;
  if ( data->multitileset )
  {
    switch ( this->Gepard->Tileset )
    {
      case 0:
        thandle0 = data->thandle0;
        v6->thandle = thandle0;
        break;
      case 1:
        thandle0 = data->thandle1;
        v6->thandle = thandle0;
        break;
      case 2:
        thandle0 = data->thandle2;
        v6->thandle = thandle0;
        break;
      case 3:
        thandle0 = data->thandle3;
        v6->thandle = thandle0;
        break;
      case 4:
        v6->thandle = data->thandle4;
        goto LABEL_8;
      default:
LABEL_8:
        thandle0 = v6->thandle;
        break;
    }
  }
  else
  {
    thandle0 = data->thandle;
    v6->thandle = thandle0;
  }
  drawtype = data->drawtype;
  v6->drawtype = drawtype;
  v6->eltunes_kezdete = data->eltunes_kezdete;
  v6->eltunes_sebesseg = data->eltunes_sebesseg;
  v6->felfutas_sebesseg = data->felfutas_sebesseg;
  v7->s2i = this->Gepard->CreateShader2Info(_x, _z, thandle0, 1, drawtype, 1, 1, 0);
  if ( v7->s2i )
  {
    v7->s2i->Erosseg = 0.0f;
    result = v7;
    v7->s2i->ForceBright = data->forcebright != 0;
  }
  else
  {
    ::operator delete(v7);
    return 0;
  }
  return result;
}

//----- (00423390) --------------------------------------------------------

void SEffect::DestroyPlayingEffects()

{
  SEffectDesc2 *first; // eax
  SEffectDesc2 *current; // ebx
  SEffectDesc2 *next; // edi
  SEffectDesc2 *prev; // ebx
  SEffectDesc2 *v6; // eax
  first = this->EffectsPlaying.first;
  this->EffectsPlaying.current = first;
  while ( this->EffectsPlaying.current )
  {
    this->CleanUpPointersInCurrentPlayingEffect();
    current = this->EffectsPlaying.current;
    if ( current )
    {
      next = current->next;
      prev = current->prev;
      if ( next )
        next->prev = prev;
      if ( prev )
        prev->next = next;
      operator delete(this->EffectsPlaying.current);
      if ( next )
      {
        v6 = next;
      }
      else
      {
        this->EffectsPlaying.last = prev;
        v6 = 0;
      }
      this->EffectsPlaying.current = v6;
      if ( !prev )
        this->EffectsPlaying.first = next;
      --this->EffectsPlaying.NumItems;
    }
  }
}

//----- (00423400) --------------------------------------------------------

void SLevelUpLens::DrawLensLayer(int a2, int a3, float sec, int thandle, float _scale, float *rotate, float rotatespeed, float alpha)

{
  float i; // xmm0_4
  float v11; // xmm0_4
  float v12; // xmm5_4
  float v13; // xmm5_4
  unsigned int fog;
  float zbuf;
  float ypos;
  float xpos;
  float scale;
  TLVertEffect vert[4];
 this->Gepard->TransformScaledPointAdd( this->MainX, this->MainY, this->MainZ, _scale, &xpos, &ypos, &scale, &zbuf, &fog);
  this->Gepard->lpD3DDev->SetRenderState(D3DRS_ZENABLE, 0);
  this->Gepard->lpD3DDev->SetRenderState((D3DRENDERSTATETYPE)a2, a3);
  this->Gepard->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 0);
  this->Gepard->lpD3DDev->SetFVF(324u);
  this->Gepard->SetDrawType(DT_ADD);
  this->Gepard->SetTexture(0, thandle, 1);
  for ( i = (float)(sec * rotatespeed) + *rotate; i < 0.0; i = i + 6.2831855f )
    ;
  for ( ; i >= 6.2831855f; i = i - 6.2831855f )
    ;
  *rotate = i;
  v11 = cosf(i);
  v12 = sinf(i);
  vert[0].u = 0.0;
  vert[1].u = 0.0;
  vert[0].v = 0.0;
  vert[2].v = 0.0;
  v13 = v12 * scale;
  float v18 = v11 * scale;
  vert[2].u = 1.0;
  vert[3].u = 1.0;
  vert[1].v = 1.0;
  vert[3].v = 1.0;
  vert[0].rhw = 1.0;
  vert[1].rhw = 1.0;
  vert[2].rhw = 1.0;
  vert[3].rhw = 1.0;
  vert[0].x = (float)(xpos - v18) + v13;
  vert[1].x = (float)(xpos - v18) - v13;
  vert[1].y = (float)(ypos - v13) + v18;
  vert[0].y = (float)(ypos - v13) - v18;
  vert[3].x = (float)(v18 + xpos) - v13;
  vert[2].x = (float)(v18 + xpos) + v13;
  vert[2].y = (float)(v13 + ypos) - v18;
  vert[0].z = zbuf;
  vert[1].z = zbuf;
  vert[2].z = zbuf;
  vert[3].z = zbuf;
  vert[3].y = (float)(v13 + ypos) + v18;
  vert[0].diffuse = 65793 * (unsigned int)(float)((float)((double)(unsigned char)fog + 0.0) * alpha);
  vert[1].diffuse = vert[0].diffuse;
  vert[2].diffuse = vert[0].diffuse;
  vert[3].diffuse = vert[0].diffuse;
  this->Gepard->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, sizeof(TLVertEffect));
  this->Gepard->lpD3DDev->SetRenderState(D3DRS_ZENABLE, 1u);
  this->Gepard->EnableFog();
}

//----- (004236C0) --------------------------------------------------------

void SEffect::Init_Boom(SBoom *data, char *classname)

{
  char *String; // eax
  int v7;
  int v16;
  int size;
  int v18;
  int maxsize;
  int v20;
  unsigned int v21;
  int *v22; // eax
  int v23;
  char szamstr[260];
  char tmpstr[260];
  char fn[260];
  String = this->EffectsINI->GetString(classname, "Texture", "error");
  strcpy(tmpstr, String);
  v7 = strcmp(tmpstr, "error");
  if ( v7 )
    v7 = v7 < 0 ? -1 : 1;
  if ( !v7 )
 Logger.g->Panic(
      "SEffect::Init_Boom: Texture variable not specified for the class: %s",
      classname);
  data->scale = this->EffectsINI->GetFloat(classname, "Scale", 1.0);
  while ( 1 )
  {
    strcpy(fn, "effects\\");
    strcat(fn, tmpstr);
    _itoa(data->thandles.size, szamstr, 10);
    strcat(fn, szamstr);
    strcat(fn, ".tga");
    v16 = this->Gepard->LoadTexture(fn, 0, 0);
    size = data->thandles.size;
    v18 = v16;
    if ( v16 == -1 )
      break;
    maxsize = data->thandles.maxsize;
    if ( size == maxsize )
    {
      if ( maxsize >= 16 )
      {
        v21 = (int)((unsigned long long)(10307921514LL * maxsize) >> 32) >> 1;
        v20 = v21 + (v21 >> 31);
      }
      else
      {
        v20 = 16;
      }
      v22 = (int *)realloc(data->thandles.array, 4 * v20);
      v23 = data->thandles.maxsize;
      data->thandles.array = v22;
      memset(&v22[v23], 0, 4 * (v20 - v23));
      size = data->thandles.size;
      data->thandles.maxsize = v20;
    }
    data->thandles.size = size + 1;
    data->thandles.array[size] = v18;
  }
  if ( !size )
    Logger.g->Panic("SEffect::Init_Boom: Cannot load %s", fn);
  this->minh = *data->thandles.array;
  this->maxh = data->thandles.array[data->thandles.size - 1];
}

//----- (004238F0) --------------------------------------------------------

void SEffect::Init_Ghost(SGhost *data, const char *texturename)

{
  int v3;
  v3 = this->Gepard->LoadTexture(texturename, 0, 0);
  data->thandle = v3;
  if ( v3 == -1 )
    Logger.g->Panic("SEffect::Init_Ghost: cannot load %s", texturename);
}

//----- (00423930) --------------------------------------------------------

void SEffect::Init_Lighting(SLighting *data, char *effectname)

{
  char *String; // eax
  String = this->EffectsINI->GetString(effectname, "LightingData", "0");
  strcpy((char *)data, String);
  data->lightingFPS = this->EffectsINI->GetInt(effectname, "LightingFPS", 20);
  data->lightingpos = 0.0;
}

//----- (00423990) --------------------------------------------------------

void SEffect::Init_Lokeshullam(SLokeshullam *data, char *effectname)

{
  char *String; // eax
  int v7;
  char str[200];
  char ide[200];
  String = this->EffectsINI->GetString(effectname, "Lokott4D", "error in init_lokotthulla");
  strcpy(str, String);
  sprintf(ide, "effects\\%s", str);
  { union { int i; float f; } _s; _s.i = 991130812; v7 = this->Gepard->PrecacheObject(ide, _s.f, (SDrawType)1); }
  data->Prototype = v7;
  if ( v7 == -1 )
    Logger.g->Panic("SEffect::Init_Lokeshullam: cannot load 4d file: %s", ide);
  data->intensity = this->EffectsINI->GetFloat(effectname, "startintensity", 1.0);
  data->disappearance = this->EffectsINI->GetFloat(effectname, "disappearance", 2.0);
  data->scale = this->EffectsINI->GetFloat(effectname, "startscale", 0.2f);
  data->scalespeed = this->EffectsINI->GetFloat(effectname, "scalespeed", 6.0);
}

//----- (00423AB0) --------------------------------------------------------

void SEffect::Init_Reflektor(SReflektor *data, char *effectname)

{
  char *String; // eax
  int v7;
  char str[200];
  char fn[200];
  String = this->EffectsINI->GetString(effectname, "Texture", "error in init_reflektor");
  strcpy(str, String);
  sprintf(fn, "effects\\%s", str);
  v7 = this->Gepard->LoadTexture(fn, 1, 0);
  data->thandle = v7;
  if ( v7 == -1 )
    Logger.g->Panic("SEffect::Init_Reflektor: couldn't load %s", fn);
  data->managetype = this->EffectsINI->GetInt(effectname, "ManageType", 0);
}

//----- (00423B80) --------------------------------------------------------

void SEffect::Init_ShaderLight(SShaderLight *data, char *effectname)

{
  char *String; // eax
  int v7;
  char *v8; // eax
  int v9;
  int v10;
  int v11;
  int v12;
  int v13;
  int v14;
  float Float;
  char texturename[260];
  char fullname[260];
  String = this->EffectsINI->GetString(effectname, "Texture", "error");
  strcpy(texturename, String);
  v7 = strcmp(texturename, "error");
  if ( v7 )
    v7 = v7 < 0 ? -1 : 1;
  if ( !v7 )
    Logger.g->Panic("SEffect::Init_ShaderLight: cannot load texture in class: \"%s\"", effectname);
  v8 = this->EffectsINI->GetString(effectname, "MultiTileset", 0);
  data->multitileset = v8 != 0;
  if ( v8 )
  {
    sprintf(fullname, "effects/tileset_normal/%s", texturename);
    v9 = this->Gepard->LoadTexture(fullname, 1, 1);
    data->thandle0 = v9;
    if ( v9 == -1 )
      Logger.g->Panic("SEffect::Init_ShaderLight: cannot load texture \"%s\"", fullname);
    sprintf(fullname, "effects/tileset_desert/%s", texturename);
    v10 = this->Gepard->LoadTexture(fullname, 1, 1);
    data->thandle1 = v10;
    if ( v10 == -1 )
      Logger.g->Panic("SEffect::Init_ShaderLight: cannot load texture \"%s\"", fullname);
    sprintf(fullname, "effects/tileset_snow/%s", texturename);
    v11 = this->Gepard->LoadTexture(fullname, 1, 1);
    data->thandle2 = v11;
    if ( v11 == -1 )
      Logger.g->Panic("SEffect::Init_ShaderLight: cannot load texture \"%s\"", fullname);
    sprintf(fullname, "effects/tileset_rock/%s", texturename);
    v12 = this->Gepard->LoadTexture(fullname, 1, 1);
    data->thandle3 = v12;
    if ( v12 == -1 )
      Logger.g->Panic("SEffect::Init_ShaderLight: cannot load texture \"%s\"", fullname);
    sprintf(fullname, "effects/tileset_jungle/%s", texturename);
    v13 = this->Gepard->LoadTexture(fullname, 1, 1);
    data->thandle4 = v13;
    if ( v13 == -1 )
      Logger.g->Panic("SEffect::Init_ShaderLight: cannot load texture \"%s\"", fullname);
  }
  else
  {
    sprintf(fullname, "effects/%s", texturename);
    v14 = this->Gepard->LoadTexture(fullname, 1, 1);
    data->thandle = v14;
    if ( v14 == -1 )
      Logger.g->Panic("SEffect::Init_ShaderLight: cannot load texture \"%s\"", fullname);
    data->thandle0 = -1;
  }
  data->drawtype = (SDrawType)this->EffectsINI->GetInt(effectname, "DrawType", 0);
  Float = this->EffectsINI->GetFloat(effectname, "Eltunes_Kezdete", 0.0);
  data->eltunes_kezdete = Float;
  if ( Float == 0.0 )
    data->eltunes_kezdete = 0.000001f;
  data->eltunes_sebesseg = this->EffectsINI->GetFloat(effectname, "Eltunes_Sebesseg", 1.0);
  data->felfutas_sebesseg = this->EffectsINI->GetFloat(effectname, "Felfutas_Sebesseg", 1000.0);
  data->forcebright = this->EffectsINI->GetInt(effectname, "ForceBright", 0);
}

//----- (00423EA0) --------------------------------------------------------

void SEffect::Init_TT(STT *data, char *classname)

{
  char *String; // eax
  int v6;
  int v15;
  int size;
  int v17;
  int maxsize;
  int v19;
  unsigned int v20;
  int *v21; // eax
  int v22;
  char szamstr[260];
  char tmpstr[260];
  char fn[260];
  String = this->EffectsINI->GetString(classname, "Texture", "error");
  strcpy(tmpstr, String);
  v6 = strcmp(tmpstr, "error");
  if ( v6 )
    v6 = v6 < 0 ? -1 : 1;
  if ( !v6 )
 Logger.g->Panic(
      "SEffect::Init_Boom: Texture variable not specified for the class: %s",
      classname);
  while ( 1 )
  {
    strcpy(fn, "effects\\torkolat_tuzek\\");
    strcat(fn, tmpstr);
    _itoa(data->thandles.size, szamstr, 10);
    strcat(fn, szamstr);
    strcat(fn, ".tga");
    v15 = this->Gepard->LoadTexture(fn, 0, 0);
    size = data->thandles.size;
    v17 = v15;
    if ( v15 == -1 )
      break;
    maxsize = data->thandles.maxsize;
    if ( size == maxsize )
    {
      if ( maxsize >= 16 )
      {
        v20 = (int)((unsigned long long)(10307921514LL * maxsize) >> 32) >> 1;
        v19 = v20 + (v20 >> 31);
      }
      else
      {
        v19 = 16;
      }
      v21 = (int *)realloc(data->thandles.array, 4 * v19);
      v22 = data->thandles.maxsize;
      data->thandles.array = v21;
      memset(&v21[v22], 0, 4 * (v19 - v22));
      size = data->thandles.size;
      data->thandles.maxsize = v19;
    }
    data->thandles.size = size + 1;
    data->thandles.array[size] = v17;
  }
  if ( !size )
    Logger.g->Panic("SEffect::Init_TT: Cannot load texture %s in class %s", fn, classname);
}

//----- (004240B0) --------------------------------------------------------

int SEffect::LoadEffect(char *effectname, const char *texturename, float _randomx, float _randomz)

{
  int nextempty;
  SHeap<SEffectDesc>::__Tstruct *array; // ecx
  unsigned int v8;
  int size;
  int maxsize;
  int v11;
  SHeap<SEffectDesc>::__Tstruct *v12; // eax
  int v13;
  int v14;
  SParticles *v15; // eax
  int *v19; // esi
  int v20;
  char *v22; // eax
  SGepard *Gepard; // ecx
  int v24;
  float *v25; // esi
  int v26;
  SHeap<SEffectDesc>::__Tstruct *v27; // eax
  char *v28; // ecx
  SParticles2 *v29; // eax
  SParticles3 *v31; // eax
  _DWORD *v33; // eax
  void *v34; // ecx
  char *String; // eax
  char *v36; // edx
  char v37; // cl
  char *v39; // eax
  int v40;
  char v41; // cl
  SGlow *v42; // eax
  char *v44; // eax
  int v47;
  SParticles4 *v48; // eax
  SLevelUpLens *v50; // eax
  SEffect **v52; // eax
  int v53;
  SEffect **v54; // eax
  SSnowfall *v55; // eax
  void *v57;
  int Int;
  int v59;
  int v60;
  int *v61;
  int v62;
  int v63;
  SEffect **v64;
  char *classname;
  char v66[200];
  char v68[200];
  int v70;
  Int = this->EffectsINI->GetInt(effectname, "EffectType", -1);
  if ( Int == -1 )
 Logger.g->Panic(
      "Fatal error: no EffectType defined for \"%s\" or this section does not exists",
      effectname);
  // SHeap inline-Add() reimplementation in the original IDA decomp used a
  // hardcoded 12-byte stride for SHeap<SEffectDesc>::Element. SEffectDesc has a
  // void* (userdata), so on x64 sizeof(Element) is 24, not 12. Use the real
  // SHeap::Add() and subscript access for the slot to keep this arch-correct.
  v62 = this->EffectsLoaded.Add();
  nextempty = v62;
  this->EffectsLoaded.array[v62].data.type = Int;
  switch ( Int )
  {
    case 1:
      v15 = new SParticles(effectname, this, this->Gepard);
      v70 = 0;
      this->EffectsLoaded.array[v62].data.userdata = v15;
      return nextempty;
    case 2:
      this->EffectsLoaded.array[v62].data.userdata = operator new(sizeof(SShaderLight));
      this->Init_ShaderLight((SShaderLight *)this->EffectsLoaded.array[v62].data.userdata,
        effectname);
      return nextempty;
    case 3:
      {
        SBoom *newBoom = (SBoom *)operator new(sizeof(SBoom));
        if (newBoom) {
          newBoom->thandles.array = nullptr;
          newBoom->thandles.size = 0;
          newBoom->thandles.maxsize = 0;
        }
        this->EffectsLoaded.array[v62].data.userdata = newBoom;
        this->Init_Boom(newBoom, effectname);
      }
      return nextempty;
    case 4:
      this->EffectsLoaded.array[v62].data.userdata = operator new(sizeof(SGhost));
      v19 = (int *)this->EffectsLoaded.array[v62].data.userdata;
      v20 = this->Gepard->LoadTexture(texturename, 0, 0);
      *v19 = v20;
      if ( v20 == -1 )
        Logger.g->Panic("SEffect::Init_Ghost: cannot load %s", texturename);
      return v62;
    case 5:
      // 0x578Cu (22412) is sizeof(SRain) on x86; on x64 the two leading
      // pointer fields widen to 8 bytes and SRain is 22424. Allocating the
      // x86 size and then writing all 100*14 EsoCseppek entries stomps 16
      // bytes of heap canary on x64 — that's the real cause of the
      // downstream "SDArray::Clear: array is damaged" panic.
      v22 = (char *)operator new(sizeof(SRain));
      classname = v22;
      v70 = 1;
      if ( v22 )
      {
        Gepard = this->Gepard;
        // SRain layout: SGepard*, SEffect*, int THandle, then EsoCseppek[].
        // On x64 the two leading pointers are 8 bytes each, so we cannot use
        // _DWORD writes — use typed access via the real struct.
        SRain *rain = (SRain *)v22;
        rain->Gepard = Gepard;
        rain->Effect = this;
        v24 = Gepard->LoadTexture("effects\\esocsepp_a.tga", 0, 0);
        rain->THandle = v24;
        if ( v24 == -1 )
          Logger.g->Panic("SRain::SRain: cannot load esocsepp_a.tga!");
        // Initialize EsoCseppek[100][14] — original walked it as float* with
        // pre-decrement so the first store landed at offset 12 (fourth float
        // of the entry before the cursor). Reproduce by starting v25 at the
        // first SEsoCsepp's second float.
        v25 = (float *)&rain->EsoCseppek[0][0] + 1;
        v59 = 100;
        do
        {
          v26 = 14;
          do
          {
            *(v25 - 1) = (float)((float)rand() / 32767.0f) * 8.0f;
            *v25 = (float)((float)rand() / 32767.0f) * 12.0f;
            v25[1] = (float)((float)rand() / 32767.0f) * 8.0f;
            v25[2] = (float)((float)((float)rand() / 32767.0f) * 4.0f) + 17.0f;
            v25 += 4;
            --v26;
          }
          while ( v26 );
          --v59;
        }
        while ( v59 );
      }
      else
      {
        classname = 0;
      }
      v27 = this->EffectsLoaded.array;
      v28 = classname;
      break;
    case 6:
      v29 = (SParticles2 *)operator new(sizeof(SParticles2));
      v70 = 2;
      if ( !v29 )
        goto LABEL_34;
      new (v29) SParticles2(effectname, this, this->Gepard);
      v28 = (char *)v29;
      v27 = this->EffectsLoaded.array;
      break;
    case 7:
      v31 = (SParticles3 *)operator new(sizeof(SParticles3));
      v70 = 3;
      if ( !v31 )
        goto LABEL_34;
      new (v31) SParticles3(effectname, this, this->Gepard, _randomx, _randomz);
      v28 = (char *)v31;
      v27 = this->EffectsLoaded.array;
      break;
    case 8:
      {
        STT *newTT = (STT *)operator new(sizeof(STT));
        if (newTT) {
          newTT->thandles.array = nullptr;
          newTT->thandles.size = 0;
          newTT->thandles.maxsize = 0;
        }
        this->EffectsLoaded.array[v62].data.userdata = newTT;
        this->Init_TT(newTT, effectname);
      }
      return nextempty;
    case 9:
      {
        // x64 fix: original allocated `0x18u` = 24B (x86 sizeof(SLokeshullam))
        // and wrote fields via byte offsets +8/+12/+16/+20 — those landed in
        // wrong fields on x64 because SIObject* grew 4->8 (Prototype@0,
        // pad@4, object@8, scale@16, scalespeed@20, intensity@24,
        // disappearance@28; x64 sizeof = 32). Call the typed Init_Lokeshullam
        // helper (effects.cpp:1140) instead of inlining the byte-offset writes.
        SLokeshullam *lokeshull = (SLokeshullam *)operator new(sizeof(SLokeshullam));
        this->EffectsLoaded.array[v62].data.userdata = lokeshull;
        this->Init_Lokeshullam(lokeshull, effectname);
      }
      return nextempty;
    case 10:
      {
        SLighting *newLight = (SLighting *)operator new(sizeof(SLighting));
        this->EffectsLoaded.array[v62].data.userdata = newLight;
        v39 = this->EffectsINI->GetString(effectname, "LightingData", "0");
        strcpy((char *)newLight, v39);
        // NOTE: original used byte offsets (v60 + 200, v60 + 204) to reach
        // LightingFPS / lightingpos. Symbolic would be better but requires
        // knowing SLighting layout — keeping byte math with correct base for now.
        *(_DWORD *)((char *)newLight + 200) = this->EffectsINI->GetInt(effectname, "LightingFPS", 20);
        *(_DWORD *)((char *)newLight + 204) = 0;
      }
      return nextempty;
    case 11:
      v42 = (SGlow *)operator new(sizeof(SGlow));
      v70 = 4;
      if ( !v42 )
        goto LABEL_34;
      new (v42) SGlow(effectname, this, this->Gepard);
      v28 = (char *)v42;
      v27 = this->EffectsLoaded.array;
      break;
    case 12:
    {
      // x64: original IDA decomp inline-init wrote `*v61 = thandle` (offset 0,
      // ok) and `v61[2] = ManageType`. v61 is `int *`, so `[2]` = byte offset
      // 8 = `managetype` field on x86 (after 4B thandle + 4B SIObject*), but
      // on x64 = lower half of `object` pointer (after 4B thandle + 4B pad).
      // Result on x64: managetype stays 0/garbage, Move_Reflektor's
      // `if (data->managetype == 1)` gate fails, headlight cone never draws
      // (no reflektor.dxt visible at night). Defer to typed Init_Reflektor.
      SReflektor *r = (SReflektor *)operator new(sizeof(SReflektor));
      r->object = nullptr;
      r->meshidx = -1;
      this->EffectsLoaded.array[v62].data.userdata = r;
      this->Init_Reflektor(r, effectname);
      return nextempty;
    }
    case 13:
      v48 = (SParticles4 *)operator new(sizeof(SParticles4));
      v70 = 5;
      if ( !v48 )
        goto LABEL_34;
      new (v48) SParticles4(effectname, this, this->Gepard);
      v28 = (char *)v48;
      v27 = this->EffectsLoaded.array;
      break;
    case 14:
      v50 = (SLevelUpLens *)operator new(sizeof(SLevelUpLens));
      v70 = 6;
      if ( !v50 )
        goto LABEL_34;
      new (v50) SLevelUpLens(this->Gepard, this, effectname);
      v28 = (char *)v50;
      v27 = this->EffectsLoaded.array;
      break;
    case 15:
      v52 = (SEffect **)operator new(sizeof(SEffect *) * 8);  // x86: 0x20, x64: 0x40
      v64 = v52;
      v70 = 7;
      if ( v52 )
      {
        v52[1] = (SEffect *)this->Gepard;
        *v52 = this;
        v53 = Concert->PrecacheSound("effects/nagydisznoajto.wav", 1);
        v64[7] = (SEffect *)(intptr_t)v53;
        Concert->AddRefToCachedSound(v53);
        v27 = this->EffectsLoaded.array;
        v28 = (char *)v64;
      }
      else
      {
        v27 = this->EffectsLoaded.array;
        v28 = 0;
      }
      break;
    case 16:
      v54 = (SEffect **)operator new(sizeof(SEffect *) * 17);  // x86: 0x44, x64: 0x88
      if ( v54 )
      {
        v54[1] = (SEffect *)this->Gepard;
        *v54 = this;
        this->EffectsLoaded.array[v62].data.userdata = v54;
      }
      else
      {
        this->EffectsLoaded.array[v62].data.userdata = nullptr;
      }
      return nextempty;
    case 17:
      v55 = (SSnowfall *)operator new(sizeof(SSnowfall));
      v70 = 8;
      if ( v55 )
      {
        new (v55) SSnowfall(this->Gepard, this, effectname);
        v28 = (char *)v55;
        v27 = this->EffectsLoaded.array;
      }
      else
      {
LABEL_34:
        v27 = this->EffectsLoaded.array;
        v28 = 0;
      }
      break;
    default:
 Logger.g->Panic(
        "SEffect::LoadEffect: The effecttype (%d) is not valid for effect: %s",
        Int,
        effectname);
  }
  v27[v62].data.userdata = v28;
  return nextempty;
}

//----- (004249E0) --------------------------------------------------------

void SEffect::MoveEffects(SEffectDesc2 *current, int next)

{
  SEffectDesc2 *first; // ecx
  float v5;
  float *userdata; // ecx
  float v7; // xmm2_4
  float v8; // xmm0_4
  bool v9; // cc
  SGepard *Gepard; // ecx
  float v11; // xmm0_4
  IDirect3DDevice9 *lpD3DDev; // eax
  SGepard *v13; // edx
  float v14; // xmm0_4
  float v16; // xmm1_4
  float v17; // xmm0_4
  float v18; // xmm0_4
  SEffectDesc2 *v19; // eax
  SEffectDesc2 *v20; // eax
  float *v21; // esi
  SGepard *v22; // ecx
  // v23 removed (SGepard_vtbl)
  float v24; // xmm1_4
  int v25;
  char v27; // al
  char v30;
  float v31;
  float s_x[2];
  float s_y;
  float s_zbuf;
  unsigned int s_fog;
  double v36;
  float v37[29];
  v30 = 0;
  first = this->EffectsPlaying.first;
  this->EffectsPlaying.current = first;
  if ( first )
  {
    v5 = 0.001f;
    v36 = 0.001;
    while ( 1 )
    {
      switch ( first->type )
      {
        case 1:
          if ( ((SParticles *)first->userdata)->MoveParticles() )
            goto LABEL_49;
          goto LABEL_47;
        case 2:
        {
          // x64 fix: original cast userdata as `float *` then `*(_DWORD *)userdata`
          // truncated the SShaderLight::s2i pointer to 4 bytes on x64, and the
          // `+36` byte offset into that truncated value was used as a memory
          // address — undefined memory access. Use typed SShaderLight pointer;
          // matches Move_ShaderLight (effects.cpp:2613) which is already typed.
          // s2i back-ref may have been NULLed by F10/F12 brute Shader2Info
          // sweep (gameview.cpp NeutralizeShader2BackRefs). Skip the Erosseg
          // animation and route to cleanup so the SEffectDesc2 unlinks; the
          // case-2 cleanup is NULL-safe (DestroyShader2 early-returns).
          SShaderLight *data = (SShaderLight *)first->userdata;
          if ( !data->s2i )
            goto LABEL_47;
          v9 = !this->Move_ShaderLight(data);
          goto LABEL_12;
        }
        case 3:
        {
          // x64 fix: original used `next = (int)first->userdata` (truncating
          // x64 pointer) and `(next + N)` byte reads at x86 SBoom field
          // offsets. `SDArray<int> thandles` widened 12->16 bytes on x64
          // (the `array` pointer grew 4->8), shifting framenr/frameinc/x/y/z
          // /scale by +4 each. Use typed SBoom access — mirrors the typed
          // play-instance build in PlayEffect case 3.
          SBoom *boom = (SBoom *)first->userdata;
          Gepard = this->Gepard;
          v11 = (float)((double)Gepard->ElapsedTime * v5);

          Gepard->TransformScaledPointAdd( boom->x,
            boom->y,
            boom->z,
            boom->scale,
            s_x,
            &s_y,
            &s_x[1],
            &s_zbuf,
            &s_fog);
          this->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 0);
          this->lpD3DDev->SetFVF(324u);
          this->Gepard->SetDrawType(DT_ADD);
          this->Gepard->SetTexture(0, boom->thandles.array[(int)boom->framenr], 1);
          boom->framenr = boom->frameinc * v11 + boom->framenr;
          this->Gepard->SetAmbientColor(0xFFFFFFu);
          v37[5] = 0.0;
          v37[12] = 0.0;
          v37[6] = 0.0;
          v37[20] = 0.0;
          v37[0] = s_x[0] - s_x[1];
          v37[7] = s_x[0] - s_x[1];
          v37[14] = s_x[0] + s_x[1];
          v37[21] = s_x[0] + s_x[1];
          v37[19] = 1.0;
          v37[26] = 1.0;
          v37[1] = s_y - s_x[1];
          v37[15] = s_y - s_x[1];
          v37[8] = s_y + s_x[1];
          v37[22] = s_y + s_x[1];
          LODWORD(v37[4]) = s_fog;
          LODWORD(v37[11]) = s_fog;
          LODWORD(v37[18]) = s_fog;
          LODWORD(v37[25]) = s_fog;
          lpD3DDev = this->lpD3DDev;
          v37[13] = 1.0;
          v37[27] = 1.0;
          v37[2] = s_zbuf;
          v37[3] = 1.0;
          v37[9] = s_zbuf;
          v37[10] = 1.0;
          v37[16] = s_zbuf;
          v37[17] = 1.0;
          v37[23] = s_zbuf;
          v37[24] = 1.0;
          lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v37, 28);
          this->Gepard->EnableFog();
          v9 = (float)boom->thandles.size <= boom->framenr;
        }
LABEL_12:
          if ( v9 )
            goto LABEL_47;
          goto LABEL_49;
        case 4:
          if ( !this->Move_Ghost((SGhost *)first->userdata) )
            goto LABEL_47;
          goto LABEL_49;
        case 5:
          if ( !((SRain *)first->userdata)->MoveRain() )
            goto LABEL_47;
          goto LABEL_49;
        case 6:
          if ( !((SParticles2 *)first->userdata)->MoveParticles(next) )
            goto LABEL_47;
          goto LABEL_49;
        case 7:
          if ( !((SParticles3 *)first->userdata)->MoveParticles((int)this, next) )
            goto LABEL_47;
          goto LABEL_49;
        case 9:
        {
          v13 = this->Gepard;
          SLokeshullam *lok = (SLokeshullam *)first->userdata;
          v14 = (float)((double)v13->ElapsedTime * v5);

          if ( lok->intensity <= 0.0 )
            goto LABEL_35;
          lok->object->SetRotation2(0, v13->CameraHRot, 0);
          v16 = lok->intensity - (float)(lok->disappearance * v14);
          v17 = (float)(lok->scalespeed * v14) + lok->scale;
          lok->intensity = v16;
          lok->scale = v17;
          lok->object->SetAmbient(v16);
          v18 = sqrtf(lok->scale);
          lok->object->SetScale(v18 * 0.0022499999f);
          goto LABEL_25;
        }
        case 0xA:
          v21 = (float *)first->userdata;
          v22 = this->Gepard;
          v31 = (float)((double)v22->ElapsedTime * v5);

          if ( *((_BYTE *)v21 + (int)v21[51]) == 49 )
            this->Gepard->SetFullBright(1);
          else
            this->Gepard->SetFullBright(0);
          v24 = (float)((float)*((int *)v21 + 50) * v31) + v21[51];
          v21[51] = v24;
          v25 = strlen((const char *)v21);
          if ( (float)(unsigned int)v25 > v24 )
            goto LABEL_25;
          this->Gepard->SetFullBright(0);
LABEL_35:
          this->CleanUpPointersInCurrentPlayingEffect();
          goto LABEL_51;
        case 0xB:
          if ( !((SGlow *)first->userdata)->DrawParticles() )
            goto LABEL_47;
          goto LABEL_49;
        case 0xC:
          if ( !this->Move_Reflektor((SReflektor *)first->userdata) )
            goto LABEL_47;
          goto LABEL_49;
        case 0xD:
          if ( !((SParticles4 *)first->userdata)->MoveParticles((int)this) )
            goto LABEL_47;
          goto LABEL_49;
        case 0xE:
          if ( !((SLevelUpLens *)first->userdata)->MoveLevelUpLens((int)current, next) )
            goto LABEL_47;
          goto LABEL_49;
        case 0xF:
          if ( !((SWindowEffect *)first->userdata)->MoveWindows() )
            goto LABEL_47;
          goto LABEL_49;
        case 0x10:
          if ( !((SEryngo *)first->userdata)->MoveEryngo() )
            goto LABEL_47;
          goto LABEL_49;
        case 0x11:
          if ( ((SSnowfall *)first->userdata)->MoveSnow() )
            goto LABEL_49;
LABEL_47:
          this->CleanUpPointersInCurrentPlayingEffect();
          v27 = 1;
          v30 = 1;
          goto LABEL_50;
        default:
LABEL_49:
          v27 = v30;
LABEL_50:
          if ( v27 )
          {
LABEL_51:
            current = this->EffectsPlaying.current;
            if ( current )
            {
              // NOTE: Ghidra audit suggested removing this, but it's needed for stale pointer detection.
              current->type = 0;
              // x64 fix: original used `next = (int)current->next` and
              // `*(_DWORD *)(next + 4) = (_DWORD)current` — both truncate
              // SEffectDesc2 pointers to 32 bits, AND the +4 offset was the
              // x86 `prev` slot (offset 4); on x64 prev moved to offset 8
              // (next/prev pointers grew 4->8). Result: the prev-update
              // clobbered the upper 4 bytes of next->next, producing the
              // `0x1351ba2000000000`-style high-shifted pointer that crashed
              // when re-read as `first->type`. Mirror the typed cleanup that
              // PlayEffect's case-2 fail path already uses (effects.cpp:2768).
              SEffectDesc2 *next_node = current->next;
              SEffectDesc2 *prev_node = current->prev;
              if ( next_node )
                next_node->prev = prev_node;
              if ( prev_node )
                prev_node->next = next_node;
              operator delete(this->EffectsPlaying.current);
              this->EffectsPlaying.current = next_node;
              if ( !next_node )
                this->EffectsPlaying.last = prev_node;
              if ( !prev_node )
                this->EffectsPlaying.first = next_node;
              --this->EffectsPlaying.NumItems;
            }
            v30 = 0;
          }
          else
          {
LABEL_25:
            v19 = this->EffectsPlaying.current;
            if ( this->EffectsPlaying.Closed )
            {
              if ( !v19 )
                Logger.g->Panic("StepToNext: In a closed chain <current> is NULL");
              v20 = v19->next;
              if ( !v20 )
                v20 = this->EffectsPlaying.first;
              this->EffectsPlaying.current = v20;
            }
            else if ( v19 )
            {
              this->EffectsPlaying.current = v19->next;
            }
            else
            {
              this->EffectsPlaying.current = 0;
            }
          }
          first = this->EffectsPlaying.current;
          if ( !first )
            return;
          v5 = (float)(v36);

          break;
      }
    }
  }
}

//----- (00425110) --------------------------------------------------------

char SEryngo::MoveEryngo()

{
  float v4; // xmm0_4
  bool SzelLokesMode1; // cl
  float i; // xmm0_4
  int v7;
  float v8; // xmm1_4
  float v9;
  float v10; // xmm1_4
  float v11; // xmm5_4
  float v12; // xmm5_4
  float SzelIrany1_b; // xmm3_4
  float v14; // xmm4_4
  float v15; // xmm2_4
  float v16; // xmm1_4
  float v17; // xmm0_4
  float v18; // xmm3_4
  float v19; // xmm1_4
  float v20;
  SIObject *Object; // ecx
  STerrain *Terrain; // ecx
  double Height; // st7
  float v24; // xmm1_4
  SIObject *v25; // edi
  float v27; // xmm1_4
  float v28; // xmm0_4
  float x;
  float v41;
  float z;
  float v43;
  float v44;
  float ujy;
  float v46;
  if ( !this->Object )
    return 0;
  v4 = (float)((double)this->Gepard->ElapsedTime * 0.001f);

  v46 = v4;
  if ( this->SzelTime1 <= 0.0 )
  {
    SzelLokesMode1 = this->SzelLokesMode1;
    this->SzelTime1 = 1.0;
    this->SzelLokesMode1 = !SzelLokesMode1;
    if ( SzelLokesMode1 )
    {
      v7 = rand();
      this->SzelLokesEro1 = 0.0;
      this->SzelSpinDir1 = 0.0;
      this->SzelLokesSebesseg1 = (float)((float)((float)v7 / 32767.0f) * 0.5f) + 0.25f;
    }
    else
    {
      v41 = (float)((float)((float)rand() / 32767.0f) * 0.5f) + 0.25f;
      this->SzelLokesSebesseg1 = v41;
      this->SzelLokesEro1 = (float)((float)(1.25 - this->SzelLokesSebesseg1) * 1.5)
                          + (float)((float)((float)((float)rand() / 32767.0) * (float)(1.25 - v41)) * 1.5);
      for ( i = (float)((float)((float)rand() / 32767.0f) * 0.77999997f) - 0.38999999f; i < 0.0f; i = i + 6.2831855f )
        ;
      for ( ; i >= 6.2831855f; i = i - 6.2831855f )
        ;
      this->SzelSpinDir1 = i;
    }
  }
  v8 = this->SzelTime1 - (float)(this->SzelLokesSebesseg1 * v46);
  this->SzelTime1 = v8;
  this->SzelLokesCounter1 = (float)(1.0 - v8) * 3.1415927f;
  v9 = sinf(this->SzelLokesCounter1);
  v10 = this->SzelSpinDir1 + this->LokalisSzelirany;
  v11 = v9;
  v12 = (float)(v11 * this->SzelLokesEro1) + 0.0f;
  v43 = v12;
  for ( this->SzelEro1 = v12; v10 < 0.0; v10 = v10 + 6.2831855f )
    ;
  for ( ; v10 >= 6.2831855f; v10 = v10 - 6.2831855f )
    ;
  SzelIrany1_b = this->SzelIrany1_b;
  v14 = v10 - SzelIrany1_b;
  this->SzelIrany1 = v10;
  ujy = SzelIrany1_b;
  if ( (float)(v10 - SzelIrany1_b) <= 0.0 )
    v14 = 6.2831855f - (float)(SzelIrany1_b - v10);
  if ( v14 >= 3.1415927f )
    v15 = 6.2831855f - v14;
  else
    v15 = v14;
  v16 = v12 * v46;
  v17 = fabsf(v15);
  if ( v17 > (float)(v12 * v46) )
  {
    if ( v14 >= 3.1415927f )
      v18 = SzelIrany1_b - v16;
    else
      v18 = SzelIrany1_b + v16;
    this->SzelIrany1_b = v18;
    ujy = v18;
  }
  v19 = sinf(ujy);
  this->SzelIranyVecX1 = (float)(v19 * v43) * v46;
  v20 = cosf(ujy);
  Object = this->Object;
  this->SzelIranyVecZ1 = (float)(v20 * v43) * v46;
  { float _y; Object->GetPosition(&x, &_y, &z); }
  Terrain = this->Gepard->Terrain;
  v43 = this->SzelIranyVecX1 + x;
  Height = Terrain->GetHeight(x, z);
  v24 = this->SzelIranyVecZ1 + z;
  ujy = (float)(Height + 0.2f);

  v41 = v24;
  if ( v43 <= 0.0 || v24 <= 0.0 || (float)this->Gepard->XSize <= v43 || (float)this->Gepard->ZSize <= v24 )
    return 1;
  this->Object->SetPosition(v43, ujy, v24);
  v25 = this->Object;
  v27 = sinf(this->SzelIrany1_b);
  v28 = cosf(this->SzelIrany1_b);
  v25->SetRelativeRotation(v28, 0, -v27, v46 * this->SzelEro1 * 3.0f);
  if ( !this->SzelLokesMode1 )
  {
    if ( this->Porzas )
    {
      this->Gepard->StopEffect(this->Porzas);
      this->Porzas = 0;
    }
    return 1;
  }
  if ( this->Porzas )
  {
    v44 = ujy - 0.2f;
  }
  else
  {
    v44 = ujy - 0.2f;
    this->Porzas = this->Gepard->PlayEffectPos(
      this->Gepard->_eryngo_porzas, v43, v44, v41,
      this->SzelLokesEro1 * 0.079999998f);
  }
  this->Gepard->TrackEffect(this->Porzas, v43, v44, v41, this->SzelEro1 * 6.0f);
  return 1;
}

//----- (004255E0) --------------------------------------------------------

bool SLevelUpLens::MoveLevelUpLens(int a2, int a3)

{
  float v3; // xmm3_4
  SIObject *Object; // ecx
  float Alpha2; // xmm4_4
  float v7; // xmm5_4
  float v8; // xmm0_4
  float v9; // xmm0_4
  float v10; // xmm1_4
  float alpha; // xmm0_4
  float v12; // xmm0_4
  float v13; // xmm0_4
  SVector head;
  SVector pos;
  float sec;
  v3 = -1.0f;
  Object = this->Object;
  if ( Object )
  {
    memset(&pos, 0, sizeof(pos));
    memset(&head, 0, sizeof(head));
    Object->GetMeshProperties(this->MeshIdx, &pos.x, &pos.y, &pos.z, &head.x, &head.y, &head.z);
    v3 = -1.0f;
    *(SVector *)&this->MainX = pos;
  }
  else if ( this->MainX == -1.0 )
  {
    return 0;
  }
  Alpha2 = this->Alpha2;
  v7 = (float)((double)this->Gepard->ElapsedTime * 0.001f);

  sec = v7;
  if ( Alpha2 == v3 || Alpha2 > 1.5707999 )
    v8 = (float)(v7 * 1.5707999) + (float)(v7 * 1.5707999);
  else
    v8 = (float)(v7 * 1.5707999f) * 0.5f;
  v9 = v8 + this->Alpha1;
  this->Alpha1 = v9;
  if ( v9 > 3.1415927f )
  {
    v9 = 3.1415927f;
    this->Alpha1 = 3.1415927f;
  }
  v10 = Alpha2;
  if ( v9 > 1.1781 && Alpha2 != v3 )
  {
    v10 = (float)((float)(v7 + v7) * 1.5707999) + Alpha2;
    this->Alpha2 = v10;
    if ( v10 > 3.1415927f )
    {
      this->Alpha2 = -1.0;
      v10 = v3;
    }
  }
  if ( v9 != 3.1415927f )
  {
    alpha = sinf(v9);
    this->DrawLensLayer(a2, a3, sec, this->THandle1, this->Scale1, &this->Rotate1, -0.30000001f, alpha);
    v12 = sinf(this->Alpha1);
    this->DrawLensLayer(a2, a3, sec, this->THandle2, this->Scale2, &this->Rotate2, 1.0f, v12);
    v10 = this->Alpha2;
    v3 = -1.0f;
  }
  if ( v10 != v3 )
  {
    v13 = sinf(v9);
    this->DrawLensLayer(a2, a3, sec, this->THandle3, this->Scale3, &this->Rotate3, 2.0f, v13);
  }
  return this->Alpha1 < 3.1415927f && !this->Stopping;
}

//----- (00425850) --------------------------------------------------------

char SRain::MoveRain()

{
  SGepard *Gepard; // edi
  SGepard *v3; // esi
  int v4;
  float v5; // xmm0_4
  SRain *v6; // ecx
  int v7;
  float *p_y; // esi
  SGepard *v9; // eax
  SGepard *v10; // ecx
  float v11; // xmm0_4
  float x; // xmm0_4
  int v13;
  float z;
  float za;
  float v17;
  int v20;
  int v21;
  int v22;
  int v23;
  float v24;
  D3DXMATRIX tmpm2;
  D3DXMATRIX tmpm1;
  D3DXMATRIX tmpm3;
  D3DXMATRIX worldmatrix;
  TLVertRain vert[4];
  v24 = (float)((double)this->Gepard->ElapsedTime * 0.001f);

  this->Gepard->lpD3DDev->SetRenderState(D3DRS_CULLMODE, 1u);
  Gepard = this->Gepard;
  v22 = 0;
  if ( this->Gepard->XSize / 8 > 0 )
  {
    v3 = this->Gepard;
    v20 = 0;
    do
    {
      v4 = 0;
      Gepard = v3;
      v23 = 0;
      if ( v3->ZSize / 8 > 0 )
      {
        v21 = 0;
        do
        {
          Gepard = v3;
          if ( v3->Terrain->Parcels[v22 + v4 * (v3->XSize / 8)].Visible )
          {
            float _dx = v3->CameraXPos - (float)(v20 + 4);
            float _dz = v3->CameraZPos - (float)(v21 + 4);
            v5 = sqrtf(_dx * _dx + _dz * _dz);
            if ( v5 < 40.0 )
            {
              v6 = this;
              v7 = 14;
              p_y = &this->EsoCseppek[10 * v23][140 * (v22 / -10 - 10 * (v23 / 10)) + 14 * v22].y;
              while ( 1 )
              {
                *p_y = *p_y - (float)(p_y[2] * v24);
                v6->Gepard->SetLightingType(LT_AMBIENT);
                this->Gepard->SetDrawType(DT_NORMAL);
                this->Gepard->SetTexture(0, this->THandle, 1);
                this->Gepard->lpD3DDev->SetFVF(258u);
                this->Gepard->SetAmbientGray(255);
                vert[0].x = -0.5;
                vert[0].y = -1.0;
                vert[0].z = 0.0;
                vert[1].x = -0.5;
                vert[1].y = 1.0;
                vert[1].z = 0.0;
                vert[2].x = 0.5;
                vert[2].y = -1.0;
                vert[2].z = 0.0;
                vert[3].x = 0.5;
                vert[3].y = 1.0;
                vert[3].z = 0.0;
                vert[0].u = 0.0;
                vert[0].v = 1.0;
                vert[1].u = 0.0;
                vert[1].v = 0.0;
                vert[2].u = 1.0;
                vert[2].v = 1.0;
                vert[3].u = 1.0;
                vert[3].v = 0.0;
                memset(&tmpm1.m[2][3], 0, 16);
                memset(&tmpm1.m[1][2], 0, 16);
                memset(&tmpm1.m[0][1], 0, 16);
                tmpm1._44 = 1.0;
                tmpm1._33 = 1.0;
                tmpm1._22 = 1.0;
                tmpm1._11 = 1.0;
                D3DXMatrixScaling(&tmpm1, 0.035f, 1.0f, 0.1f);
                memset(&tmpm2.m[2][3], 0, 16);
                memset(&tmpm2.m[1][2], 0, 16);
                memset(&tmpm2.m[0][1], 0, 16);
                v9 = this->Gepard;
                tmpm2._44 = 1.0;
                tmpm2._33 = 1.0;
                tmpm2._22 = 1.0;
                tmpm2._11 = 1.0;
                D3DXMatrixRotationY(&tmpm2, v9->CameraHRot);
                v10 = this->Gepard;
                memset(&tmpm3.m[2][3], 0, 16);
                v11 = (float)v21 + p_y[1];
                memset(&tmpm3.m[1][2], 0, 16);
                v17 = v11;
                z = v11;
                memset(&tmpm3.m[0][1], 0, 16);
                tmpm3._44 = 1.0;
                x = (float)v20 + *(p_y - 1);
                tmpm3._33 = 1.0;
                tmpm3._22 = 1.0;
                tmpm3._11 = 1.0;
                za = (float)(v10->Terrain->GetHeight(x, z) + *p_y);

                D3DXMatrixTranslation(&tmpm3, (float)v20 + *(p_y - 1), za, v17);
                D3DXMatrixMultiply(&worldmatrix, &tmpm1, &tmpm2);
                D3DXMatrixMultiply(&worldmatrix, &worldmatrix, &tmpm3);
                this->Gepard->lpD3DDev->SetTransform((_D3DTRANSFORMSTATETYPE)256, &worldmatrix);
                this->Gepard->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2u, vert, 20u);
                if ( (float)(*p_y + 1.0) < 0.0 )
                {
                  v13 = rand();
                  *p_y = 12.0;
                  *(p_y - 1) = (float)((float)v13 / 32767.0f) * 8.0f;
                  p_y[1] = (float)((float)rand() / 32767.0f) * 8.0f;
                }
                p_y += 4;
                if ( !--v7 )
                  break;
                v6 = this;
              }
              Gepard = this->Gepard;
            }
          }
          v3 = Gepard;
          v21 += 8;
          v4 = v23 + 1;
          v23 = v4;
        }
        while ( v4 < Gepard->ZSize / 8 );
      }
      v3 = Gepard;
      v20 += 8;
      ++v22;
    }
    while ( v22 < Gepard->XSize / 8 );
  }
  Gepard->lpD3DDev->SetRenderState(D3DRS_CULLMODE, 2u);
  return 1;
}

//----- (00426910) --------------------------------------------------------

bool SEffect::Move_Boom(SBoom *data)

{
  SGepard *Gepard; // ecx
  float v5; // xmm0_4
  IDirect3DDevice9 *lpD3DDev; // eax
  unsigned int fog;
  float zbuf;
  float ypos;
  float scale;
  float xpos;
  float v13;
  TLVertEffect vert[4];
  Gepard = this->Gepard;
  v5 = (float)((double)Gepard->ElapsedTime * 0.001f);

  v13 = v5;
  Gepard->TransformScaledPointAdd(data->x, data->y, data->z, data->scale, &xpos, &ypos, &scale, &zbuf, &fog);
  this->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 0);
  this->lpD3DDev->SetFVF(324u);
  this->Gepard->SetDrawType(DT_ADD);
  this->Gepard->SetTexture(0, data->thandles.array[(int)data->framenr], 1);
  data->framenr = (float)(data->frameinc * v13) + data->framenr;
  this->Gepard->SetAmbientColor(0xFFFFFFu);
  vert[0].u = 0.0;
  vert[1].u = 0.0;
  vert[0].v = 0.0;
  vert[0].x = xpos - scale;
  vert[1].x = xpos - scale;
  vert[2].x = xpos + scale;
  vert[3].x = xpos + scale;
  vert[0].y = ypos - scale;
  vert[2].y = ypos - scale;
  vert[1].y = ypos + scale;
  vert[3].y = ypos + scale;
  vert[0].color = fog;
  vert[1].color = fog;
  vert[2].color = fog;
  vert[3].color = fog;
  lpD3DDev = this->lpD3DDev;
  vert[2].v = 0.0;
  vert[2].u = 1.0;
  vert[3].u = 1.0;
  vert[1].v = 1.0;
  vert[3].v = 1.0;
  vert[0].z = zbuf;
  vert[0].rhw = 1.0;
  vert[1].z = zbuf;
  vert[1].rhw = 1.0;
  vert[2].z = zbuf;
  vert[2].rhw = 1.0;
  vert[3].z = zbuf;
  vert[3].rhw = 1.0;
  lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, 28);
  this->Gepard->EnableFog();
  return (float)data->thandles.size > data->framenr;
}

//----- (00426B20) --------------------------------------------------------

bool SEffect::Move_Ghost(SGhost *data)

{
  float v3; // xmm1_4
  float v4;
  float eltunes_kezdete; // xmm1_4
  float v6; // xmm0_4
  float v7; // xmm1_4
  float alpha; // xmm0_4
  float v9; // xmm1_4
  float aktualis_meret; // xmm0_4
  SGepard *Gepard; // eax
  float z; // xmm0_4
  float x;
  float y;
  float v18;
  D3DXMATRIX worldmatrix;
  D3DXMATRIX tmpm3;
  D3DXMATRIX tmpm2;
  D3DXMATRIX tmpm1;
  TLVertRain vert[4];
  v3 = (float)((double)this->Gepard->ElapsedTime * 0.001f);

  v18 = (float)v3;
  data->y = (float)(data->felszallas_sebesseg * v3) + data->y;
  data->aktualis_meret = (float)(data->nagyitas_sebesseg * v3) + data->aktualis_meret;
  data->magassag = (float)(data->kiteres_sebesseg * v3) + data->magassag;
  v4 = cosf(data->magassag);
  eltunes_kezdete = data->eltunes_kezdete;
  *(float *)&v4 = v4;
  data->x = (float)(*(float *)&v4 * data->kiteres_mertek) + data->orix;
  if ( eltunes_kezdete > 0.0 )
  {
    v6 = (float)(data->felfutas_sebesseg * v18) + data->alpha;
    data->alpha = v6;
    if ( v6 > 1.0 )
      data->alpha = 1.0;
  }
  v7 = eltunes_kezdete - v18;
  data->eltunes_kezdete = v7;
  if ( v7 <= 0.0 )
  {
    alpha = data->alpha;
    v9 = data->eltunes_sebesseg * v18;
    data->eltunes_kezdete = 0.0;
    data->alpha = alpha - v9;
  }
  this->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 0);
  this->lpD3DDev->SetFVF(258u);
  this->Gepard->SetLightingType(LT_AMBIENT);
  this->Gepard->SetDrawType(DT_ADD);
  this->Gepard->SetTexture(0, data->thandle, 1);
  this->Gepard->SetAmbientGray((int)(float)(data->alpha * 255.0));
  this->lpD3DDev->SetRenderState(D3DRS_CULLMODE, 1u);
  aktualis_meret = data->aktualis_meret;
  vert[0].x = -0.5;
  vert[0].y = -1.0;
  vert[0].z = 0.0;
  vert[1].x = -0.5;
  vert[1].y = 1.0;
  vert[1].z = 0.0;
  vert[2].x = 0.5;
  vert[2].y = -1.0;
  vert[2].z = 0.0;
  vert[3].x = 0.5;
  vert[3].y = 1.0;
  vert[3].z = 0.0;
  vert[0].u = 0.0;
  vert[0].v = 1.0;
  vert[1].u = 0.0;
  vert[1].v = 0.0;
  vert[2].u = 1.0;
  vert[2].v = 1.0;
  vert[3].u = 1.0;
  vert[3].v = 0.0;
  memset(&tmpm1.m[2][3], 0, 16);
  memset(&tmpm1.m[1][2], 0, 16);
  memset(&tmpm1.m[0][1], 0, 16);
  tmpm1._44 = 1.0;
  tmpm1._33 = 1.0;
  tmpm1._22 = 1.0;
  tmpm1._11 = 1.0;
  D3DXMatrixScaling(&tmpm1, aktualis_meret, aktualis_meret, aktualis_meret);
  Gepard = this->Gepard;
  memset(&tmpm2.m[2][3], 0, 16);
  memset(&tmpm2.m[1][2], 0, 16);
  memset(&tmpm2.m[0][1], 0, 16);
  tmpm2._44 = 1.0;
  tmpm2._33 = 1.0;
  tmpm2._22 = 1.0;
  tmpm2._11 = 1.0;
  D3DXMatrixRotationY(&tmpm2, Gepard->CameraHRot);
  D3DXMatrixMultiply(&worldmatrix, &tmpm2, &tmpm1);
  z = data->z;
  memset(&tmpm3.m[2][3], 0, 16);
  y = data->y;
  x = data->x;
  memset(&tmpm3.m[1][2], 0, 16);
  memset(&tmpm3.m[0][1], 0, 16);
  tmpm3._44 = 1.0;
  tmpm3._33 = 1.0;
  tmpm3._22 = 1.0;
  tmpm3._11 = 1.0;
  D3DXMatrixTranslation(&tmpm3, x, y, z);
  D3DXMatrixMultiply(&worldmatrix, &worldmatrix, &tmpm3);
  this->lpD3DDev->SetTransform((_D3DTRANSFORMSTATETYPE)256, &worldmatrix);
  this->lpD3DDev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2u, vert, 20u);
  this->lpD3DDev->SetRenderState(D3DRS_CULLMODE, 2u);
  this->Gepard->EnableFog();
  return data->alpha > 0.0;
}

//----- (00426FF0) --------------------------------------------------------

char SEffect::Move_Lighting(SLighting *data)

{
  SGepard *Gepard; // ecx
  float v5; // xmm1_4
  float v7;
  Gepard = this->Gepard;
  v7 = (float)((double)Gepard->ElapsedTime * 0.001);
  if ( data->lightingdata[(int)data->lightingpos] == 49 )
    Gepard->SetFullBright(1);
  else
    Gepard->SetFullBright(0);
  v5 = (float)((float)data->lightingFPS * v7) + data->lightingpos;
  data->lightingpos = v5;
  if ( (float)strlen(data->lightingdata) > v5 )
    return 1;
  this->Gepard->SetFullBright(0);
  return 0;
}

//----- (004270C0) --------------------------------------------------------

char SEffect::Move_Lokeshullam(SLokeshullam *data)

{
  SGepard *Gepard; // edx
  float v3; // xmm0_4
  SIObject *object; // ecx
  float v5; // xmm1_4
  float v6; // xmm0_4
  SIObject *v7; // esi
  float v9; // xmm0_4
  Gepard = this->Gepard;
  v3 = (float)((double)Gepard->ElapsedTime * 0.001);
  if ( data->intensity <= 0.0 )
    return 0;
  data->object->SetRotation2(0, Gepard->CameraHRot, 0);
  object = data->object;
  v5 = data->intensity - (float)(data->disappearance * v3);
  v6 = (float)(data->scalespeed * v3) + data->scale;
  data->intensity = v5;
  data->scale = v6;
  object->SetAmbient(v5);
  v7 = data->object;
  v9 = sqrtf(data->scale);
  v7->SetScale(v9 * 0.0022499999f);
  return 1;
}

//----- (004271C0) --------------------------------------------------------

char SEffect::Move_Reflektor(SReflektor *data)

{
  IDirect3DDevice9 *lpD3DDev; // eax
  STerrain *Terrain; // ecx
  unsigned char *VisMap; // edx
  IDirect3DDevice9 *v7; // eax
  float v8; // xmm5_4
  float v9; // xmm0_4
  float fvy;
  float y;
  float z;
  float x;
  float fvz;
  float fvx;
  D3DXVECTOR2 vec2;
  D3DXMATRIX worldmatrix;
  struct { float x, y, z; unsigned int diffuse; float u, v; } vert[4];
  if ( !data->object )
    return 0;
  this->Gepard->lpD3DDev->SetFVF(322u);
  this->Gepard->SetLightingType(LT_PRELIT);
  this->Gepard->SetDrawType(DT_ADD);
  this->Gepard->lpD3DDev->SetRenderState(D3DRS_FOGENABLE, 0);
  this->Gepard->SetTexture(0, data->thandle, 1);
#ifdef HDB_LIGHT_OCCLUSION
  this->Gepard->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 0);
#else
  this->Gepard->lpD3DDev->SetRenderState(D3DRS_ZENABLE, 0);
#endif
  lpD3DDev = this->lpD3DDev;
  memset(&worldmatrix.m[2][3], 0, 16);
  memset(&worldmatrix.m[1][2], 0, 16);
  memset(&worldmatrix.m[0][1], 0, 16);
  worldmatrix._44 = 1.0;
  worldmatrix._33 = 1.0;
  worldmatrix._22 = 1.0;
  worldmatrix._11 = 1.0;
  lpD3DDev->SetTransform((_D3DTRANSFORMSTATETYPE)256, &worldmatrix);
  data->object->GetMeshInterpolatedProperties(data->meshidx, &x, &y, &z, &fvx, &fvy, &fvz);
  if ( data->managetype == 1 )
  {
    Terrain = this->Gepard->Terrain;
    VisMap = Terrain->VisMap;
    if ( !VisMap
      || Terrain->GodMode
      || VisMap[(int)(float)(x * 2.0) - 2 * (int)(float)(z * -2.0) * (Terrain->XSize + 1)] )
    {
      vec2.x = fvx;
      vec2.y = fvz;
      D3DXVec2Normalize(&vec2, &vec2);
      v7 = this->lpD3DDev;
      fvx = vec2.x;
      fvz = vec2.y;
      v8 = (float)(vec2.y * 4.5999999) + z;
      vert[0].y = y;
      vert[0].x = (float)((float)(vec2.x * 0.60000002) + x) + (-vec2.y);
      vert[1].x = (float)((float)(vec2.x * 0.60000002) + x) + vec2.y;
      vert[1].y = y;
      vert[2].y = y;
      vert[3].y = y;
      vert[0].z = (float)((float)(vec2.y * 0.60000002) + z) + vec2.x;
      v9 = (float)(vec2.x * 4.5999999) + x;
      vert[1].z = (float)((float)(vec2.y * 0.60000002) + z) + (-vec2.x);
      vert[0].u = 0.0;
      vert[0].v = 1.0;
      vert[1].u = 1.0;
      vert[3].x = v9 + vec2.y;
      vert[2].x = v9 + (-vec2.y);
      vert[1].v = 1.0;
      vert[2].u = 0.0;
      vert[2].v = 0.0;
      vert[3].u = 1.0;
      vert[2].z = v8 + vec2.x;
      vert[3].z = v8 + (-vec2.x);
      vert[3].v = 0.0;
      vert[0].diffuse = 3487012;
      vert[1].diffuse = 3487012;
      vert[2].diffuse = 3487012;
      vert[3].diffuse = 3487012;
      v7->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, vert, 24);
    }
  }
#ifdef HDB_LIGHT_OCCLUSION
  this->Gepard->lpD3DDev->SetRenderState(D3DRS_ZWRITEENABLE, 1u);
#else
  this->Gepard->lpD3DDev->SetRenderState(D3DRS_ZENABLE, 1u);
#endif
  this->Gepard->EnableFog();
  return 1;
}

//----- (00427560) --------------------------------------------------------

bool SEffect::Move_ShaderLight(SShaderLight *data)

{
  float v2; // xmm2_4
  float v3; // xmm0_4
  v2 = (float)((double)this->Gepard->ElapsedTime * 0.001f);

  if ( data->eltunes_kezdete > 0.0 )
  {
    data->s2i->Erosseg = (float)(data->felfutas_sebesseg * v2) + data->s2i->Erosseg;
    if ( data->s2i->Erosseg > 1.0 )
      data->s2i->Erosseg = 1.0;
  }
  v3 = data->eltunes_kezdete - v2;
  data->eltunes_kezdete = v3;
  if ( v3 <= 0.0 )
  {
    data->s2i->Erosseg = data->s2i->Erosseg - (float)(data->eltunes_sebesseg * v2);
    data->eltunes_kezdete = 0.0;
  }
  return data->s2i->Erosseg > 0.0;
}

//----- (00427610) --------------------------------------------------------

void *SEffect::PlayEffect(int handle, float _x, float _y, float _z, float _scalespeed)

{
  SEffectDesc2 *v7; // edi
  SEffectDesc2 *last; // eax
  SParticles *v9; // edx
  void *result; // eax
  SEffectDesc2 *next; // eax
  SEffectDesc2 *prev; // ecx
  SEffectDesc2 *first; // eax
  _DWORD *userdata; // esi
  char *v30; // eax
  SParticles2 *v31; // edx
  void *v32; // eax
  SParticles3 *v33; // edx
  void *v34; // eax
  _QWORD *v35; // esi
  _QWORD *v36; // edi
  SIObject *v37; // eax
  void *v38; // esi
  void *v39; // eax
  int type;
  SEffectDesc2 *v43;
  int handlea;
  SEffectDesc2 *_xa;
  SEffectDesc2 *_za;
  handlea = handle;
  type = this->EffectsLoaded.array[handlea].data.type;
  v7 = (SEffectDesc2 *)operator new(sizeof(SEffectDesc2));
  v43 = v7;
  memset(&v7->next, 0, 16);
  v7->effectringofobject = 0;
  last = this->EffectsPlaying.last;
  if ( last )
  {
    last->next = v7;
    v7->prev = this->EffectsPlaying.last;
    this->EffectsPlaying.last = v7;
    v7->next = 0;
  }
  else
  {
    this->EffectsPlaying.last = v7;
    this->EffectsPlaying.first = v7;
  }
  ++this->EffectsPlaying.NumItems;
  v7->type = type;
  v7->effectringofobject = 0;
  #ifdef HD_DEBUG_EFFECTS
  Logger.g->Log(0, "(diag) PlayEffect: node=%p type=%d handle=%d", v7, type, handlea);
  #endif
  switch ( type )
  {
    case 1:
      v9 = (SParticles *)operator new(sizeof(SParticles));
      if ( !v9 )
        goto LABEL_7;
      new (v9) SParticles((SParticles *)this->EffectsLoaded.array[handlea].data.userdata, _x, _y, _z);
      v7->userdata = v9;
      result = 0;
      break;
    case 2:
      // x64 fix: was `_DWORD *_ya = (_DWORD *)userdata` with _ya[N] byte
      // arithmetic assuming x86 SShaderLight layout. SShaderLight starts
      // with `SShader2Info *s2i` which grew 4->8 bytes on x64, so _ya[1]
      // read the upper half of s2i (= 0xCDCDCDCD when the prototype's s2i
      // was never set), passing a garbage texture handle to
      // CreateShader2Info and crashing in CreateShader2 at
      // array[_thandle].data.Width. The post-create writes at byte +36
      // and +52 into the resulting SShader2Info were also wrong on x64
      // (SShader2Info starts with two SShader2Info pointers + an SMesh*).
      // Mirror Clone_ShaderLight (effects.cpp:852) — typed field copies,
      // sizeof(SShaderLight) allocation, named s2i->Erosseg/ForceBright.
      {
        SShaderLight *data = (SShaderLight *)this->EffectsLoaded.array[handlea].data.userdata;
        SShaderLight *out  = (SShaderLight *)operator new(sizeof(SShaderLight));
        int thandle;
        if ( data->multitileset )
        {
          switch ( this->Gepard->Tileset )
          {
            case 0: out->thandle = data->thandle0; break;
            case 1: out->thandle = data->thandle1; break;
            case 2: out->thandle = data->thandle2; break;
            case 3: out->thandle = data->thandle3; break;
            case 4: out->thandle = data->thandle4; break;
            default: break;
          }
          thandle = out->thandle;
        }
        else
        {
          out->thandle = data->thandle;
          thandle = out->thandle;
        }
        out->drawtype          = data->drawtype;
        out->eltunes_kezdete   = data->eltunes_kezdete;
        out->eltunes_sebesseg  = data->eltunes_sebesseg;
        out->felfutas_sebesseg = data->felfutas_sebesseg;
        // Machinima deep state: skip Shader2Info creation here too (mirrors the
        // gate at the top of Clone_ShaderLight). Downstream null check handles it.
        out->s2i = g_HideWorldOverlays
                     ? nullptr
                     : this->Gepard->CreateShader2Info(_x, _z, thandle, 1, out->drawtype, 1, 1, 0);
        SShaderLight *playing;
        if ( out->s2i )
        {
          out->s2i->Erosseg = 0.0f;
          out->s2i->ForceBright = (data->forcebright != 0);
          playing = out;
        }
        else
        {
          ::operator delete(out);
          playing = nullptr;
        }
        v7->userdata = playing;
        if ( playing )
          goto LABEL_54;
      }
 Logger.g->Log(
        0,
        "(info) SEffect::PlayEffect: Clone_ShaderLight creation was failed, deleting this playing effect...");
      next = v7->next;
      prev = v7->prev;
      _xa = v7->next;
      _za = prev;
      if ( v7->next )
        next->prev = prev;
      if ( prev )
        prev->next = next;
      ::operator delete(v7);
      first = _xa;
      if ( !_xa )
        this->EffectsPlaying.last = _za;
      if ( _za )
      {
        first = this->EffectsPlaying.first;
        --this->EffectsPlaying.NumItems;
      }
      else
      {
        --this->EffectsPlaying.NumItems;
        this->EffectsPlaying.first = _xa;
      }
      this->EffectsPlaying.current = first;
      result = 0;
      break;
    case 3:
    {
      // x64 fix: original used `(float *)operator new(0x28u)` (= x86
      // sizeof(SBoom) = 40B; x64 sizeof = 44B because SDArray<int>::array
      // grew 4->8 bytes), then `(_DWORD*)v22 + N` byte arithmetic targeting
      // x86 field offsets, plus the realloc input read via `(void**)v22 + 2`
      // = byte 16 on x64 (correct on x86 = 8). Result on x64: realloc
      // received uninitialized stack/heap memory (e.g. `0xFFFFFFFFFFFFFFFF`)
      // and faulted inside ucrtbased. Use typed SBoom + SDArray access.
      // SBoom is the prototype/play struct shared by both sides; the loader
      // (LoadEffect case 3 -> Init_Boom, effects.cpp:1380) is already typed.
      SBoom *src = (SBoom *)this->EffectsLoaded.array[handlea].data.userdata;
      SBoom *dst = (SBoom *)operator new(sizeof(SBoom));
      if ( dst )
      {
        dst->thandles.size = 0;
        dst->thandles.maxsize = 0;
        dst->thandles.array = nullptr;
        for ( int i = 0; i < src->thandles.size; ++i )
        {
          if ( dst->thandles.size == dst->thandles.maxsize )
          {
            int newmax = (dst->thandles.maxsize >= 16)
                           ? (6 * dst->thandles.maxsize / 5)
                           : 16;
            int oldmax = dst->thandles.maxsize;
            dst->thandles.array = (int *)realloc(dst->thandles.array, sizeof(int) * newmax);
            memset(&dst->thandles.array[oldmax], 0, sizeof(int) * (newmax - oldmax));
            dst->thandles.maxsize = newmax;
          }
          dst->thandles.array[dst->thandles.size++] = src->thandles.array[i];
        }
        dst->framenr  = 0.0f;
        dst->frameinc = 25.0f;
        dst->x        = _x;
        dst->y        = _y;
        dst->z        = _z;
        dst->scale    = src->scale;
        // drawtype intentionally left uninitialized — matches original
        // behaviour (`*((_DWORD*)v22 + 8)` was never written).
      }
      v7->userdata = dst;
      return 0;
    }
    case 4:
      userdata = (_DWORD *)this->EffectsLoaded.array[handlea].data.userdata;
      v30 = (char *)operator new(0x40u);
      *(_DWORD *)v30 = *userdata;
      *((float *)v30 + 1) = _x;
      *((float *)v30 + 2) = _y;
      *(_QWORD *)(v30 + 12) = LODWORD(_z);
      *((_DWORD *)v30 + 5) = 1056964608;
      *((_DWORD *)v30 + 6) = 1048576000;
      *((_DWORD *)v30 + 7) = 0x40000000;
      *((_DWORD *)v30 + 8) = 1056964608;
      *((_DWORD *)v30 + 9) = 1045220557;
      *((_DWORD *)v30 + 10) = 1056964608;
      *((_DWORD *)v30 + 11) = 0;
      *((_DWORD *)v30 + 12) = 0;
      *((_DWORD *)v30 + 13) = 1082130432;
      *((float *)v30 + 14) = _x;
      *((float *)v30 + 15) = _z;
      v7->userdata = v30;
      return 0;
    case 5:
    case 17:
      v7->userdata = this->EffectsLoaded.array[handlea].data.userdata;
LABEL_54:
      result = 0;
      break;
    case 6:
      v31 = (SParticles2 *)operator new(sizeof(SParticles2));
      if ( v31 )
      {
        v32 = new (v31) SParticles2((SParticles2 *)this->EffectsLoaded.array[handlea].data.userdata, _x, _y, _z);
        v7->userdata = v32;
        result = 0;
      }
      else
      {
LABEL_7:
        result = 0;
        v7->userdata = 0;
      }
      break;
    case 7:
      v33 = (SParticles3 *)operator new(sizeof(SParticles3));
      if ( v33 )
        v34 = new (v33) SParticles3((SParticles3 *)this->EffectsLoaded.array[handlea].data.userdata, _x, _y, _z, _scalespeed);
      else
        v34 = 0;
      v7->userdata = v34;
      result = v7;
      break;
    case 9:
    {
      // x64 fix: original used `operator new(0x18u)` (= x86 sizeof(SLokeshullam)
      // = 24B; x64 sizeof = 32B because SIObject* grew + alignment), then
      // `memcpy(v36, v35, 16)` copied only the first 16 bytes (incomplete
      // on x64), `v36[2] = v35[2]` was a _QWORD copy, and
      // `*((_DWORD*)v36 + 1) = (_DWORD)v37` truncated the SIObject* return
      // from CreateObjectByIndex. Use Clone_Lokeshullam (effects.cpp:831)
      // which does all of this typed.
      SLokeshullam *proto = (SLokeshullam *)this->EffectsLoaded.array[handlea].data.userdata;
      SLokeshullam *lok = this->Clone_Lokeshullam(proto, _x, _y, _z);
      result = 0;
      v43->userdata = lok;
      break;
    }
    case 10:
      v38 = this->EffectsLoaded.array[handlea].data.userdata;
      v39 = operator new(0xD0u);
      qmemcpy(v39, v38, 0xD0u);
      v7->userdata = v39;
      result = 0;
      break;
    case 11:
      result = this->EffectsLoaded.array[handlea].data.userdata;
      v7->userdata = result;
      break;
    default:
      Logger.g->Panic("SEffect::PlayEffect(3): Unknown effect type! (%d)", type);
  }
  return result;
}

//----- (00427CC0) --------------------------------------------------------

void SEffect::PlayEffect(int handle, float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, float strength, float fade_speed, float u_scale, float v_scale)

{
  // x64 fix: original cast userdata to `int *` and then read
  // `userdata[2]` as the `thandles.array` pointer (x86 byte 8 of STT).
  // On x64 `thandles.array` is at byte 8 too BUT it's an 8-byte pointer;
  // reading it as `int` truncated to 4 bytes, then using as a memory
  // address + offset corrupted the smoke-trail texture handle lookup.
  // Use typed STT access.
  if ( this->EffectsLoaded.array[handle].data.type != 8 )
    Logger.g->Panic("SEffect::PlayEffect(1): Unknow effect type: %d",
                    this->EffectsLoaded.array[handle].data.type);
  STT *data = (STT *)this->EffectsLoaded.array[handle].data.userdata;
  int size = data->thandles.size;
  int idx = (int)((float)((float)rand() * 0.000030517578f) * (float)size);
  int thandle = data->thandles.array[idx];
  int v17 = this->Gepard->CreateSmokeTrail(thandle, _x1, _y1, _z1, 0xFFFFFF,
                                           strength, fade_speed, u_scale, v_scale, (SDrawType)1);
  this->Gepard->TrackSmokeTrail(v17, _x2, _y2, _z2, 1.0f);
  this->Gepard->CloseSmokeTrail(v17);
}

//----- (00427DD0) --------------------------------------------------------

void SEffect::PlayEffect(int handle, SIObject *object, char *meshname, int race)

{
  SEffectDesc2 *v6; // edi
  SEffectDesc2 *last; // eax
  SParticles *v8; // ecx
  void *v10; // eax
  SParticles3 *v11; // ecx
  _DWORD *userdata; // esi
  int v13;
  SParticles4 *v14; // ecx
  float *v22; // esi
  float *v23; // eax
  float i; // xmm0_4
  void *block;
  int handlea;
  char *meshnamea;
  if ( object )
  {
    handlea = handle;
    block = (void *)this->EffectsLoaded.array[handlea].data.type;
    v6 = (SEffectDesc2 *)operator new(sizeof(SEffectDesc2));
    memset(&v6->next, 0, 16);
    v6->effectringofobject = 0;
    last = this->EffectsPlaying.last;
    if ( last )
    {
      last->next = v6;
      v6->prev = this->EffectsPlaying.last;
      this->EffectsPlaying.last = v6;
      v6->next = 0;
    }
    else
    {
      this->EffectsPlaying.last = v6;
      this->EffectsPlaying.first = v6;
    }
    ++this->EffectsPlaying.NumItems;
    v6->effectringofobject = object->SetEffect(v6);
    v6->type = (int)block;
    switch ( (unsigned int)block )
    {
      case 1u:
        v8 = (SParticles *)operator new(sizeof(SParticles));
        if ( !v8 )
          goto LABEL_8;
        new (v8) SParticles((SParticles *)this->EffectsLoaded.array[handlea].data.userdata, object, meshname, race);
        v6->userdata = v8;
        return;
      case 7u:
        v11 = (SParticles3 *)operator new(sizeof(SParticles3));
        if ( !v11 )
          goto LABEL_8;
        new (v11) SParticles3((SParticles3 *)this->EffectsLoaded.array[handlea].data.userdata, object, meshname);
        v6->userdata = v11;
        return;
      case 0xCu:
      {
        // x64 fix: original used `operator new(0x10u)` (= x86 sizeof(SReflektor)
        // = 16B; x64 sizeof = 24B because SIObject* widened 4->8 with
        // alignment), then `*((_DWORD*)meshnamea + 1) = (_DWORD)object`
        // truncated the SIObject* and `(_DWORD*) + 2/3` byte arithmetic
        // landed in the wrong fields on x64 (byte 8 is x86 managetype but
        // x64 lower-half of object pointer). Use Clone_Reflektor
        // (effects.cpp:846) — already typed, includes the same missing-mesh
        // fallback.
        SReflektor *proto = (SReflektor *)this->EffectsLoaded.array[handlea].data.userdata;
        SReflektor *ref = this->Clone_Reflektor(proto, object);
        v6->userdata = ref;
        return;
      }
      case 0xDu:
        v14 = (SParticles4 *)operator new(sizeof(SParticles4));
        if ( v14 )
          v10 = new (v14) SParticles4((SParticles4 *)this->EffectsLoaded.array[handlea].data.userdata, object, meshname, race);
        else
LABEL_8:
          v10 = 0;
        v6->userdata = v10;
        return;
      case 0xEu:
      {
        // x64 fix: original open-coded SLevelUpLens construction via
        // `_DWORD *v17 = userdata` byte arithmetic with `operator new(0x118u)`
        // (= x86 sizeof(SLevelUpLens) = 280B; on x64 the struct is ~296B
        // because Effect/Gepard/Object pointers grew 4->8 each), plus
        // `*((_DWORD*)v16+17) = (_DWORD)object` truncated SIObject* to 32
        // bits, plus a vtable lookup on the truncated pointer. Use placement
        // new with the existing typed copy ctor (effects.cpp:88) — which
        // already does the field copies and the missing-mesh fallback.
        SLevelUpLens *proto = (SLevelUpLens *)this->EffectsLoaded.array[handlea].data.userdata;
        SLevelUpLens *lens  = (SLevelUpLens *)operator new(sizeof(SLevelUpLens));
        if ( lens )
          new (lens) SLevelUpLens(proto, object, meshname, race);
        v6->userdata = lens;
        break;
      }
      case 0x10u:
      {
        // x64 fix: original used `(float *)operator new(0x44u)` (= x86
        // sizeof(SEryngo) = 68B; x64 sizeof = 88B because the three leading
        // pointers Effect/Gepard/Object grew 4->8 each plus a trailing
        // Porzas pointer + alignment), then `*((_DWORD*)v22+2) = (_DWORD)object`
        // truncated SIObject* and `*((_BYTE*)v22 + 60)` byte-offset writes
        // landed in the wrong fields on x64. Plus
        // `*(float *)(*((_DWORD*)v22+1) + 1504)` truncated the Gepard ptr
        // to 4 bytes before adding 1504 — undefined access. Use the existing
        // typed copy ctor SEryngo(SEryngo *p, SIObject *object)
        // (effects.cpp:55) which does all the field copies correctly.
        SEryngo *proto = (SEryngo *)this->EffectsLoaded.array[handlea].data.userdata;
        SEryngo *eryngo = (SEryngo *)operator new(sizeof(SEryngo));
        if ( eryngo )
          new (eryngo) SEryngo(proto, object);
        v6->userdata = eryngo;
        break;
      }
      default:
        Logger.g->Panic("SEffect::PlayEffect(): Unknow effect type: %d", block);
    }
  }
  else
  {
    Logger.g->Warning("SEffect::PlayEffect(): object is NULL.");
  }
}

//----- (00428210) --------------------------------------------------------

void SEffect::PlayEffect(int handle, SIObject *object, SDArray<SAblak> *ablakok)

{
  // x64 fix: original open-coded SWindowEffect creation via `_DWORD*` byte
  // arithmetic over a `(float *)operator new(0x20u)` buffer (= x86
  // sizeof(SWindowEffect) = 32B; on x64 the struct is 48B because Effect /
  // Gepard / Object / Ablakok pointers grew 4->8 each), plus
  // `(DWORD)object` and `(DWORD)ablakok` truncations, plus a vtable lookup
  // through `**(_DWORD**)(v9+8) + 8` whose +8 byte offset points at slot 1
  // (Release) on x64 instead of slot 2 (GetPosition) on x86. Use typed
  // SWindowEffect access; the dummy GetPosition call is preserved for
  // fidelity (its outputs were already discarded). MoveWindows
  // (gepard.cpp:395) only reads Object/Ablakok/NextRandom/AblakCounter
  // from the playing instance — Hang fields are read from the prototype
  // for sound playback only.
  SHeap<SEffectDesc>::__Tstruct *array;
  SEffectDesc2 *v6;
  SEffectDesc2 *last;
  int handlea;
  array = this->EffectsLoaded.array;
  handlea = array[handle].data.type;
  v6 = (SEffectDesc2 *)operator new(sizeof(SEffectDesc2));
  memset(&v6->next, 0, 16);
  v6->effectringofobject = 0;
  last = this->EffectsPlaying.last;
  if ( last )
  {
    last->next = v6;
    v6->prev = this->EffectsPlaying.last;
    this->EffectsPlaying.last = v6;
    v6->next = 0;
  }
  else
  {
    this->EffectsPlaying.last = v6;
    this->EffectsPlaying.first = v6;
  }
  ++this->EffectsPlaying.NumItems;
  v6->effectringofobject = object->SetEffect(v6);
  v6->type = handlea;
  if ( handlea != 15 )
    Logger.g->Panic("SEffect::PlayEffect(4): Unknow effect type: %d", handlea);
  SWindowEffect *proto = (SWindowEffect *)array[handle].data.userdata;
  SWindowEffect *w = (SWindowEffect *)operator new(sizeof(SWindowEffect));
  if ( w )
  {
    w->Effect = proto->Effect;
    w->Gepard = proto->Gepard;
    w->Object = object;
    w->Ablakok = ablakok;
    w->NextRandom = (float)((float)rand() / 32767.0f) + 0.5f;
    w->AblakCounter = 0;

    SAblak *first = &ablakok->array[0];
    first->active = true;
    if ( first->type == 1 )
    {
      float dummy_x, dummy_y, dummy_z;
      object->GetPosition(&dummy_x, &dummy_y, &dummy_z);
      // NOTE: IDA shows PlaySound3DById with positional args, but reconstructed args
      // were garbage (pointer casts). Keep as PlaySoundById until args are properly decoded.
      if ( proto->NagyajtoHang != -1 )
        Concert->PlaySoundById(proto->NagyajtoHang, 0.0f, 0.0f, 0);
    }
    else
    {
      ablakok->array[w->AblakCounter + 1].active = true;
    }
  }
  v6->userdata = w;
}

//----- (004283D0) --------------------------------------------------------

void SEffect::PorzasParameterek(SEffectDesc2 *handle, float SzelTime1, bool SzelLokesMode1, float SzelLokesSebesseg1, float SzelLokesEro1, float SzelSpinDir1, float LokalisSzelirany)

{
  // x64: was `_DWORD *handle` with handle[2]=type, handle[3]=userdata, plus
  // raw byte offsets into userdata. On x64 SEffectDesc2 pointers grew (4→8)
  // so handle[2]/[3] read the wrong fields, and SParticles3's leading SDArrays
  // grew (12→16 each) so the byte offsets shifted. Typed access — same
  // pattern as StopEffect.
  if ( handle->type != 7 )
    Logger.g->Panic("SEffect::PorzasParameterek: Ez NEM porzas effekt: %d !", handle->type);
  SParticles3 *p = (SParticles3 *)handle->userdata;
  p->SzelTime1 = SzelTime1;
  p->SzelLokesMode1 = SzelLokesMode1;
  p->SzelLokesSebesseg1 = SzelLokesSebesseg1;
  p->SzelLokesEro1 = SzelLokesEro1;
  p->SzelSpinDir1 = SzelSpinDir1;
  p->LokalisSzelirany = LokalisSzelirany;
}

//----- (00428450) --------------------------------------------------------

void SEffect::Reflektor_RefreshIndices(SReflektor *data)

{
  int v2;
  v2 = data->object->GetMeshIndex("test");
  data->meshidx = v2;
  if ( v2 == -1 )
#ifdef HDB_MISSING_ASSET_FALLBACK
  {
    Logger.g->Log(0, "SEffect::Reflektor_RefreshIndices: \"test\" mesh missing -> skipping effect");
    data->object = 0;
  }
#else
    Logger.g->Panic("SEffect::Reflektor_RefreshIndices: the object has no \"test\" mesh!");
#endif
}

//----- (00428490) --------------------------------------------------------

void SEffect::RefreshIndices(SParticles **handle)

{
  switch ( (unsigned int)handle[2] )
  {
    case 0u:
      // Stale handle: underlying EffectsPlaying entry was freed
      // (see cleanup at effects.cpp ~1936 which zeroes type before delete).
      return;
    case 1u:
      ((SParticles *)handle[3])->RefreshIndices();
      break;
    case 7u:
      ((SParticles3 *)handle[3])->RefreshIndices();
      break;
    case 0xCu:
      this->Reflektor_RefreshIndices((SReflektor *)handle[3]);
      break;
    case 0xDu:
      ((SParticles4 *)handle[3])->RefreshIndices();
      break;
    case 0xEu:
      ((SLevelUpLens *)handle[3])->RefreshIndices();
      break;
    case 0xFu:
      handle[3]->GroupPrototypes.array = 0;
      break;
    default:
#ifdef HDB_MISSING_ASSET_FALLBACK
      Logger.g->Log(0, "SEffect::RefreshIndices: skipping unknown effect type %d", handle[2]);
      break;
#else
      Logger.g->Panic("SEffect::RefreshIndices: Unknow effect type: %d", handle[2]);
#endif
  }
}

//----- (00428550) --------------------------------------------------------

void SLevelUpLens::RefreshIndices()

{
  int v2;
  v2 = this->Object->GetMeshIndex(this->MeshName);
  this->MeshIdx = v2;
  if ( v2 == -1 )
#ifdef HDB_MISSING_ASSET_FALLBACK
  {
    Logger.g->Log(0, "SLevelUpLens::RefreshIndices: mesh '%s' missing -> skipping effect", this->MeshName);
    this->Object = 0;
  }
#else
    Logger.g->Panic("SLevelUpLens::RefreshIndices: cannot find mesh in object");
#endif
}

//----- (00428580) --------------------------------------------------------

// x64 fix: original signature took `_DWORD *handle` and used handle[2]/handle[3]
// for SEffectDesc2.type/.userdata. Those _DWORD indexes assume 32-bit
// next/prev pointers in SEffectDesc2 (effects.h:117-123) — on x64 the
// pointers are 8 bytes, so the indexes read the wrong fields and the
// validation block silently bailed on every call. SGroup::~SGroup relies
// on this to null SParticles3.Object back-pointers before being freed; on
// x64 that never happened, leaving SEffectDesc2.userdata->Object dangling
// and crashing later in CleanUpPointersInCurrentPlayingEffect.
//
// x64: PorzasParameterek, TrackEffect, and CleanUp case 1 (SParticles.Object
// at byte 288→360) now use typed SEffectDesc2 / SParticles* / SParticles3*
// access. RefreshIndices (~3284) is not bugged — its `SParticles **handle`
// signature has 8-byte stride which compensates for x64's field-offset shift.
void SEffect::StopEffect(SEffectDesc2 *handle)

{
  if ( !handle ) return;
  int type = handle->type;
  void *userdata = handle->userdata;
  if (type != 1 && type != 7 && type != 0xB && type != 0xC &&
      type != 0xD && type != 0xE && type != 0xF && type != 0x10) {
    Logger.g->Warning("SEffect::StopEffect: invalid type %d at handle=%p, skipping (likely stale handle)", type, handle);
    return;
  }
  switch ( type )
  {
    case 1:
      ((SParticles *)userdata)->Object = nullptr;
      break;
    case 7:
      ((SParticles3 *)userdata)->Object = nullptr;
      ((SParticles3 *)userdata)->Stopping = true;
      break;
    case 0xB:
      ((SGlow *)userdata)->StopNow = true;
      break;
    case 0xC:
      ((SReflektor *)userdata)->object = nullptr;
      break;
    case 0xD:
    {
      SParticles4 *p = (SParticles4 *)userdata;
      SIObject *obj = p->Object;
      p->Object = nullptr;
      p->Stopping = true;
      if (obj) obj->Release();  // balance AddRef in SParticles4 constructor
      break;
    }
    case 0xE:
      ((SLevelUpLens *)userdata)->Object = nullptr;
      ((SLevelUpLens *)userdata)->Stopping = true;
      break;
    case 0xF:
      ((SWindowEffect *)userdata)->Object = nullptr;
      break;
    case 0x10:
      ((SEryngo *)userdata)->Object = nullptr;
      break;
    default:
      break;
  }
}

//----- (00428670) --------------------------------------------------------

void SLevelUpLens::StopEffect()

{
  this->Stopping = 1;
}

//----- (00428680) --------------------------------------------------------

void SEffect::TrackEffect(SEffectDesc2 *handle, float _x, float _y, float _z, float _borningspeed)

{
  // x64: see PorzasParameterek above for the same fix rationale.
  if ( !handle ) return;
  if ( handle->type != 7 )
  {
    Logger.g->Warning("SEffect::TrackEffect: unexpected effect type: %d (expected 7) at handle=%p, skipping", handle->type, handle);
    return;
  }
  SParticles3 *p = (SParticles3 *)handle->userdata;
  p->MainX = _x;
  p->MainY = _y;
  p->MainZ = _z;
  if ( _borningspeed != -1.0 )
    p->BorningSpeed = _borningspeed;
}

