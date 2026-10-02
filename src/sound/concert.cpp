// sound/concert.cpp
// Audio engine
// Decompiled from: gameSplit/sconcert.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <corecrt_math.h>
#include <windows.h>
#include <math.h>
#include <new>

#include "concert.h"
#include <logger.h>
#include <decode.h>
#include "hdbeefup.h"

// Classes: SConcert, SWave
// Function count: 45

// HUD diagnostic hook (HD_HDBEEFUP_AUDIO_DEBUG). The DSound backend doesn't
// currently call it (the original engine works correctly on x86 so no
// diagnostic is needed there); defined here only so the linker doesn't fail
// when a TU references the extern in iconcert.h.
SwineAudioPlayHook g_AudioPlayHook = nullptr;

// Global data
int VolumeTable[11] = { -10000, -4500, -4000, -3500, -3000, -2500, -2000, -1500, -1000, -500, 0 };
int SoundGroupQuotas[2] = { 6, 4 };
int StreamThreadRunning = 0;
int StreamThreadShutdown = 0;
int StreamThreadDeviceChange = 0;
int PrecalculateCalled = 0;
short outsamples[576][2] = {};
short outsamples_0[576][2] = {};
// (word_5B573A was &outsamples_0[0][1], word_5B4E3A was &outsamples[0][1] — now use direct indexing)

// Vtable thunks for SMpegAudioCallBack dispatch.
//
// On x86 the trick is __fastcall + a `void* /*edx*/` placeholder so the
// thunk picks up `this` from ecx and the rest from the stack — that
// simulates __thiscall. On x64 there is no edx slot: the MS x64 ABI
// passes the first four args in RCX/RDX/R8/R9. Adding the dummy edx
// shifts every subsequent argument by one register and DataCallback
// then sees `right` as `left`, `num_samples` as `right`, etc., which
// is what produced the spurious `stereo != 2` panic.
//
// The second-base offset for SConcert (1 vtable pointer) also differs
// per arch — 4 on x86, 8 on x64 — so use sizeof(void*).

#if defined(_M_IX86)
static void __fastcall SConcert_DataCallback_Thunk(SMpegAudioCallBack *self, void* /*edx*/, short *a, short *b, int c, int d, int e) {
    SConcert *concert = reinterpret_cast<SConcert *>(reinterpret_cast<char *>(self) - sizeof(void*));
    concert->DataCallback(a, b, c, d, e);
}
#else
static void SConcert_DataCallback_Thunk(SMpegAudioCallBack *self, short *a, short *b, int c, int d, int e) {
    SConcert *concert = reinterpret_cast<SConcert *>(reinterpret_cast<char *>(self) - sizeof(void*));
    concert->DataCallback(a, b, c, d, e);
}
#endif

static SMpegAudioCallBack_vtbl SConcert_SMpegAudioCallBack_vtbl = {
    (void (__thiscall *)(SMpegAudioCallBack *, short *, short *, int, int, int))SConcert_DataCallback_Thunk
};

// SWave's first base is SMpegAudioCallBack, so no offset adjustment is
// needed; only the calling-convention difference matters.
#if defined(_M_IX86)
static void __fastcall SWave_DataCallback_Thunk(SMpegAudioCallBack *self, void* /*edx*/, short *a, short *b, int c, int d, int e) {
    SWave *wave = reinterpret_cast<SWave *>(self);
    wave->DataCallback(a, b, c, d, e);
}
#else
static void SWave_DataCallback_Thunk(SMpegAudioCallBack *self, short *a, short *b, int c, int d, int e) {
    SWave *wave = reinterpret_cast<SWave *>(self);
    wave->DataCallback(a, b, c, d, e);
}
#endif

static SMpegAudioCallBack_vtbl SWave_SMpegAudioCallBack_vtbl = {
    (void (__thiscall *)(SMpegAudioCallBack *, short *, short *, int, int, int))SWave_DataCallback_Thunk
};

//----- (0047F180) --------------------------------------------------------

// DirectSound device-enumeration callback. Only used inside SConcert's
// InitDirectSound; lived in game.cpp historically as an IDA artifact.
extern "C" BOOL CALLBACK DSEnumCallback(LPGUID /*lpGuid*/, LPCSTR /*desc*/,
                                        LPCSTR /*module*/, LPVOID /*ctx*/)
{
  return TRUE;
}

// SIConcert factory for the DirectSound backend. The miniaudio backend
// provides its own CreateConcert in concert_ma.cpp; CMake compiles exactly
// one of these TUs based on GEPARD_AUDIO_BACKEND.
SIConcert *__cdecl CreateConcert(HWND hwnd)
{
  SConcert *c = new SConcert(hwnd);
  if ( !c )
    return nullptr;
  return c;
}

//----- (0047EA50) --------------------------------------------------------

SConcert::SConcert(HWND hWnd)

{
  IDirectSound8 **p_DSound; // edi
  HRESULT v4;
  const char *v5; // eax
  char atmstr[200];
  int v7;
  // Initialize SMpegAudioCallBack vtable for DataCallback dispatch
  static_cast<SMpegAudioCallBack*>(this)->vftable = &SConcert_SMpegAudioCallBack_vtbl;
  this->RefCount = 1;
  this->DeviceDescription.buf = 0;
  this->DeviceDescription.size = 0;
  v7 = 0;
  this->DeviceModule.buf = 0;
  this->DeviceModule.size = 0;
  this->DSound = 0;
  p_DSound = &this->DSound;
  this->Listener = 0;
  this->SoundCache.array = 0;
  this->SoundCache.size = 0;
  this->SoundCache.maxsize = 0;
  this->SoundCache.nextempty = -1;
  this->SoundCache.occupied = 0;
  this->Sounds.array = 0;
  this->Sounds.size = 0;
  this->Sounds.maxsize = 0;
  this->Sounds.nextempty = -1;
  this->Sounds.occupied = 0;
  this->ListenerPosition.x = 0.0;
  this->ListenerPosition.y = 0.0;
  this->ListenerPosition.z = 0.0;
  this->StreamFile1.buf = 0;
  this->StreamFile1.size = 0;
  this->StreamFile2.buf = 0;
  this->StreamFile2.size = 0;
  v7 = 5;
  this->StreamBuffer = 0;
  InitializeCriticalSection(&this->StreamBufferCriticalSection);
  this->Volume[0] = 5;
  this->Volume[1] = 5;
  this->Volume[2] = 5;
  this->ReverseStereo = 1.0;
  this->MinDistanceMultiplier = 1.0;
  this->TransientSounds = 0;
  this->hWnd = hWnd;
  if ( g_HeadlessMode )
  {
    // Headless mode: skip DirectSound initialization entirely.
    // DSound stays null; methods that check DSound will no-op.
    MpegAudioPrecalculate();
    return;
  }
  if ( GetDeviceID(&DSDEVID_DefaultPlayback, &this->DefaultDeviceGuid) < 0 )
  {
    Logger.g->Warning("SConcert::Concert: No directx compatible soundcard found");
  }
  else
  {
    v4 = DirectSoundCreate8(&this->DefaultDeviceGuid, p_DSound, 0);
    if ( v4 )
    {
      v5 = DXGetErrorStringA(v4);
      sprintf(atmstr, "%s: %s", "SConcert::Concert: DirectSoundCreate failed", v5);
      Logger.g->Warning(atmstr);
    }
  }
  this->InitDirectSound();
  MpegAudioPrecalculate();
}

//----- (0047EC50) --------------------------------------------------------

SWave::SWave()

{
  this->vftable = &SWave_SMpegAudioCallBack_vtbl;
  this->Data = 0;
}

//----- (0047ECC0) --------------------------------------------------------

SConcert::~SConcert()

{
  int v2;
  int size;
  SHeap<SSoundProp>::Element *v4; // eax
  int v5;
  int v6;
  int v7;
  SHeap<SSoundCacheProp>::Element *v8; // eax
  SHeap<SSoundProp>::Element *array; // eax
  HRESULT v10;
  SHeap<SSoundCacheProp>::Element *v11; // ecx
  char *FileName; // eax
  IDirectSoundBuffer *Buffer; // ecx
  SHeap<SSoundCacheProp>::Element *v14; // ecx
  IDirectSoundBuffer *StreamBuffer; // ecx
  IDirectSound3DListener *Listener; // ecx
  IDirectSound8 *DSound; // ecx
  char *buf; // eax
  char *v19; // eax
  char *v20; // eax
  char *v21; // eax
  const char *v22; // eax
  int v23;
  int v24;
  char _Buffer[200];
  // this->SIConcert::__vftable = (SConcert_vtbl *)SConcert::`vftable'{for `SIConcert'};
  v2 = -1;
  // this->SMpegAudioCallBack::__vftable = (SMpegAudioCallBack_vtbl *)SConcert::`vftable'{for `SMpegAudioCallBack'};
  while ( 1 )
  {
    size = this->Sounds.size;
    if ( ++v2 >= size )
      break;
    v4 = &this->Sounds.array[v2];
    while ( v4->use != 0x7FFFFFFF )
    {
      ++v2;
      ++v4;
      if ( v2 >= size )
        goto LABEL_6;
    }
    if ( v2 < 0 )
      break;
    array = this->Sounds.array;
    if ( !array[v2].data.Looped )
    {
      v23 = 0;
      v10 = array[v2].data.Buffer->GetStatus((LPDWORD)&v23);
      if ( v10 )
      {
        v22 = DXGetErrorStringA(v10);
        sprintf(_Buffer, "%s: %s", "SConcert::Update: GetStatus failed", v22);
        Logger.g->Panic(_Buffer);
      }
      this->RemoveSound(v2);
      --this->TransientSounds;
    }
  }
LABEL_6:
  v5 = 0;
  v6 = -1;
LABEL_7:
  v24 = v5;
  while ( 1 )
  {
    v7 = this->SoundCache.size;
    if ( ++v6 >= v7 )
      break;
    v8 = &this->SoundCache.array[v6];
    while ( v8->use != 0x7FFFFFFF )
    {
      ++v6;
      ++v8;
      if ( v6 >= v7 )
        goto LABEL_12;
    }
    if ( v6 < 0 )
      break;
    v11 = this->SoundCache.array;
    if ( v11[v6].data.RefCount )
    {
      v5 = v24 + 1;
      goto LABEL_7;
    }
    FileName = v11[v6].data.FileName;
    if ( FileName )
    {
      delete[] FileName;
      this->SoundCache.array[v6].data.FileName = 0;
      v11 = this->SoundCache.array;
    }
    Buffer = v11[v6].data.Buffer;
    if ( Buffer )
    {
      Buffer->Release();
      this->SoundCache.array[v6].data.Buffer = 0;
    }
    if ( v6 >= this->SoundCache.size || (v14 = this->SoundCache.array, v14[v6].use != 0x7FFFFFFF) )
      Logger.g->Panic("SHeap::Remove: invalid index (%d)", v6);
    v14[v6].use = this->SoundCache.nextempty;
    --this->SoundCache.occupied;
    v5 = v24;
    this->SoundCache.nextempty = v6;
  }
LABEL_12:
  if ( v5 )
    Logger.g->Panic("SConcert::~SConcert: Sounds leaked on shutdown");
  while ( StreamThreadRunning )
  {
    StreamThreadShutdown = 1;
    Sleep(0);
  }
  StreamThreadShutdown = 0;
  StreamBuffer = this->StreamBuffer;
  if ( StreamBuffer )
  {
    StreamBuffer->Release();
    this->StreamBuffer = 0;
  }
  Listener = this->Listener;
  if ( Listener )
  {
    Listener->Release();
    this->Listener = 0;
  }
  DSound = this->DSound;
  if ( DSound )
  {
    DSound->Release();
    this->DSound = 0;
  }
  DeleteCriticalSection(&this->StreamBufferCriticalSection);
  buf = this->StreamFile2.buf;
  if ( buf )
  {
    delete[] buf;
    this->StreamFile2.buf = 0;
  }
  v19 = this->StreamFile1.buf;
  if ( v19 )
  {
    delete[] v19;
    this->StreamFile1.buf = 0;
  }
  if ( this->Sounds.array )
    free(this->Sounds.array);
  if ( this->SoundCache.array )
    free(this->SoundCache.array);
  v20 = this->DeviceModule.buf;
  if ( v20 )
  {
    delete[] v20;
    this->DeviceModule.buf = 0;
  }
  v21 = this->DeviceDescription.buf;
  if ( v21 )
  {
    delete[] v21;
    this->DeviceDescription.buf = 0;
  }
}

//----- (0047EFB0) --------------------------------------------------------

SWave::~SWave()

{
  unsigned char *Data; // eax
  Data = this->Data;
  if ( Data )
  {
    delete[] Data;
    this->Data = 0;
  }
}

//----- (0047F070) --------------------------------------------------------

void SConcert::AddRef()

{
  ++this->RefCount;
}

//----- (0047F080) --------------------------------------------------------

void SConcert::AddRefToCachedSound(int idx)

{
  if ( idx >= 0 )
    ++this->SoundCache.array[idx].data.RefCount;
}

//----- (0047F0A0) --------------------------------------------------------

bool SConcert::CheckStreamPlayback()

{
  return StreamThreadRunning;
}

//----- (0047F0B0) --------------------------------------------------------

int SConcert::CleanupSoundCache()

{
  int v1;
  int v3;
  int size;
  SHeap<SSoundCacheProp>::Element *i; // eax
  SHeap<SSoundCacheProp>::Element *array; // ecx
  char *FileName; // eax
  IDirectSoundBuffer *Buffer; // ecx
  SHeap<SSoundCacheProp>::Element *v10; // ecx
  int v11;
  v1 = 0;
  v3 = -1;
LABEL_2:
  v11 = v1;
  while ( 1 )
  {
    size = this->SoundCache.size;
    if ( ++v3 >= size )
      return v1;
    for ( i = &this->SoundCache.array[v3]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v3 >= size )
        return v1;
    }
    if ( v3 < 0 )
      return v1;
    array = this->SoundCache.array;
    if ( array[v3].data.RefCount )
    {
      v1 = v11 + 1;
      goto LABEL_2;
    }
    FileName = array[v3].data.FileName;
    if ( FileName )
    {
      delete[] FileName;
      this->SoundCache.array[v3].data.FileName = 0;
      array = this->SoundCache.array;
    }
    Buffer = array[v3].data.Buffer;
    if ( Buffer )
    {
      Buffer->Release();
      this->SoundCache.array[v3].data.Buffer = 0;
    }
    if ( v3 >= this->SoundCache.size || (v10 = this->SoundCache.array, v10[v3].use != 0x7FFFFFFF) )
      Logger.g->Panic("SHeap::Remove: invalid index (%d)", v3);
    v10[v3].use = this->SoundCache.nextempty;
    --this->SoundCache.occupied;
    v1 = v11;
    this->SoundCache.nextempty = v3;
  }
}

//----- (0047F390) --------------------------------------------------------

int SConcert::CreateSound(int cacheidx, int sound_group, float min_distance, float x, float y, float z)

{
  int v9;
  int v10;
  HRESULT v11;
  HRESULT v12;
  HRESULT v13;
  const char *v15; // eax
  const char *v16; // eax
  const char *v17; // eax
  IDirectSound3DBuffer *v18;
  IDirectSound3DBuffer *Buffer3d;
  int v21;
  char atmstr[200];
  v9 = this->SpawnSound(cacheidx, 1);
  v21 = v9;
  if ( v9 < 0 )
    return -1;
  v10 = v9;
  this->Sounds.array[v10].data.Looped = 1;
  this->Sounds.array[v10].data.SoundGroup = sound_group;
  this->Sounds.array[v10].data.Enabled = 0;
  this->Sounds.array[v10].data.MinDistance = min_distance;
  this->Sounds.array[v10].data.Position.x = x;
  this->Sounds.array[v10].data.Position.y = y;
  this->Sounds.array[v10].data.Position.z = z;
  Buffer3d = this->Sounds.array[v9].data.Buffer3d;
  v11 = Buffer3d->SetMinDistance(this->MinDistanceMultiplier * min_distance, DS3D_IMMEDIATE);
  if ( v11 )
  {
    v15 = DXGetErrorStringA(v11);
    sprintf(atmstr, "%s: %s", "SConcert::CreateSound: SetMinDistance failed", v15);
    Logger.g->Panic(atmstr);
  }
  v18 = this->Sounds.array[v10].data.Buffer3d;
  v12 = v18->SetPosition(this->ReverseStereo * x,
          y,
          z,
          0);
  if ( v12 )
  {
    v16 = DXGetErrorStringA(v12);
    sprintf(atmstr, "%s: %s", "SConcert::CreateSound: SetPosition failed", v16);
    Logger.g->Panic(atmstr);
  }
  rand();
  v13 = this->Sounds.array[v10].data.Buffer->SetCurrentPosition(0);
  if ( v13 )
  {
    v17 = DXGetErrorStringA(v13);
    sprintf(atmstr, "%s: %s", "SConcert::CreateSound: SetCurrentPosition failed", v17);
    Logger.g->Panic(atmstr);
  }
  this->OptimizeSoundGroup(this->Sounds.array[v10].data.SoundGroup);
  return v21;
}

//----- (0047F5B0) --------------------------------------------------------

int SConcert::CreateSound(int cacheidx, float volume, float panning)

{
  int v6;
  int v7;
  int v8;
  HRESULT v9;
  float v10; // xmm0_4
  HRESULT v11;
  const char *v13; // eax
  const char *v14; // eax
  IDirectSoundBuffer *v15;
  IDirectSoundBuffer *Buffer;
  char atmstr[200];
  v6 = this->SpawnSound(cacheidx, 0);
  if ( v6 < 0 )
    return -1;
  v7 = v6;
  this->Sounds.array[v7].data.Looped = 1;
  this->Sounds.array[v7].data.ChannelID = -1;
  this->Sounds.array[v7].data.Volume = (int)(volume * 100.0f);
  v8 = VolumeTable[this->Volume[1]] + 1000 + this->Sounds.array[v6].data.Volume;
  if ( v8 < 0 )
  {
    if ( v8 < -10000 )
      v8 = -10000;
  }
  else
  {
    v8 = 0;
  }
  Buffer = this->Sounds.array[v6].data.Buffer;
  v9 = Buffer->SetVolume(v8);
  if ( v9 )
  {
    v13 = DXGetErrorStringA(v9);
    sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetVolume failed", v13);
    Logger.g->Panic(atmstr);
  }
  v10 = 10000.0f;
  if ( (float)(panning * 100.0) < 10000.0 )
    v10 = ((panning * 100.0f) > -10000.0f ? (panning * 100.0f) : -10000.0f);
  this->Sounds.array[v6].data.Panning = (int)v10;
  v11 = this->Sounds.array[v6].data.Buffer->SetPan(this->Sounds.array[v6].data.Panning);
  if ( v11 )
  {
    v14 = DXGetErrorStringA(v11);
    sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetPan failed", v14);
    Logger.g->Panic(atmstr);
  }
  v15 = this->Sounds.array[v6].data.Buffer;
  v15->Play(0, 0, 1);
  return v6;
}

//----- (0047F740) --------------------------------------------------------

int SConcert::CreateSound3D(const char *filename, int sound_group, float min_distance, float x, float y, float z)

{
  int cacheidx = this->PrecacheSound(filename, true);
  return this->CreateSound(cacheidx, sound_group, min_distance, x, y, z);
}

//----- (0047F790) --------------------------------------------------------

int SConcert::CreateSound(const char *filename, float volume, float panning)

{
  int cacheidx = this->PrecacheSound(filename, false);
  return this->CreateSound(cacheidx, volume, panning);
}

//----- (0047F7D0) --------------------------------------------------------

void SConcert::CreateStreamBuffer()

{
  _RTL_CRITICAL_SECTION *p_StreamBufferCriticalSection; // edi
  IDirectSound8 *DSound; // ecx
  HRESULT v4;
  HRESULT v5;
  const char *v6; // eax
  const char *v7; // eax
  DSBUFFERDESC bd;
  WAVEFORMATEX fmt;
  char atmstr[200];
  p_StreamBufferCriticalSection = &this->StreamBufferCriticalSection;
  EnterCriticalSection(&this->StreamBufferCriticalSection);
  DSound = this->DSound;
  fmt.wFormatTag = 1; fmt.nChannels = 2;
  fmt.cbSize = 0;
  bd.dwReserved = 0;
  fmt.nSamplesPerSec = 48000;
  fmt.nAvgBytesPerSec = 192000;
  fmt.nBlockAlign = 4; fmt.wBitsPerSample = 16;
  bd.dwSize = sizeof(bd);
  bd.dwFlags = 360576;
  bd.dwBufferBytes = 0x40000;
  bd.lpwfxFormat = &fmt;
  bd.guid3DAlgorithm = GUID_NULL;
  v4 = DSound->CreateSoundBuffer(&bd, &this->StreamBuffer, 0);
  if ( v4 )
  {
    v6 = DXGetErrorStringA(v4);
    sprintf(atmstr, "%s: %s", "SConcert::StartStreamingPlayback: CreateSoundBuffer failed", v6);
    Logger.g->Panic(atmstr);
  }
  v5 = this->StreamBuffer->SetVolume(VolumeTable[this->Volume[0]]);
  if ( v5 )
  {
    v7 = DXGetErrorStringA(v5);
    sprintf(atmstr, "%s: %s", "SConcert::StartStreamingPlayback: SetVolume failed", v7);
    Logger.g->Panic(atmstr);
  }
  this->StreamStarted = 0;
  this->WriteCursor = 0;
  LeaveCriticalSection(p_StreamBufferCriticalSection);
}

//----- (0047F990) --------------------------------------------------------

void SConcert::DataCallback(short *left, short *right, int num_samples, int stereo, int freq)

{
  int v8;
  short *v9; // edx
  unsigned int SpinCount;
  LPCRITICAL_SECTION p_StreamBuffer; // eax
  int v13;
  HRESULT v14;
  unsigned int v15;
  size_t v16;
  void *v17; // edi
  HRESULT v18;
  unsigned int v19;
  bool v20;
  unsigned int v21;
  HRESULT v22;
  HRESULT v23;
  const char *v24; // eax
  const char *v25; // eax
  char *v26; // eax
  _com_error *p_err = nullptr; // placeholder
  LPCRITICAL_SECTION lpCriticalSection;
  DWORD play_cursor;
  void *buf2;
  DWORD bufsize2;
  void *buf1;
  DWORD bufsize1;
  int v34;
  char atmstr[200];
  int v36;

  // IDA showed StreamFile2.size here due to 4-byte offset error in struct field ID.
  // Actually waits for StreamBuffer (DirectSound buffer) to be created.
  while ( !this->StreamBuffer )
    Sleep(0x64u);
  if ( stereo != 2 )
    Logger.g->Panic("xxx");
  lpCriticalSection = &this->StreamBufferCriticalSection;
  EnterCriticalSection(&this->StreamBufferCriticalSection);
  v8 = 0;
  if ( num_samples > 0 )
  {
    v9 = right;
    do
    {
      outsamples_0[v8][0] = left[v8];
      outsamples_0[v8][1] = right[v8];
      v8++;
    }
    while ( v8 < num_samples );

  }
  if ( this->StreamStarted )
  {
    if ( StreamThreadDeviceChange )
    {
LABEL_15:
      p_StreamBuffer = lpCriticalSection;
      goto LABEL_16;
    }
    v34 = 4 * num_samples;
    while ( 1 )
    {
      this->StreamBuffer->GetCurrentPosition(&play_cursor, 0);
      SpinCount = this->WriteCursor;
      if ( (SpinCount > play_cursor || SpinCount + v34 <= play_cursor) && SpinCount + v34 <= play_cursor + 0x40000 )
        break;
      Sleep(0);
      if ( StreamThreadDeviceChange )
        goto LABEL_15;
    }
    v13 = v34;
  }
  else
  {
    SpinCount = this->WriteCursor;
    v13 = 4 * num_samples;
  }
  v14 = this->StreamBuffer->Lock(SpinCount, v13, &buf1, &bufsize1, &buf2, &bufsize2, 0);
  if ( v14 )
  {
    v24 = DXGetErrorStringA(v14);
    sprintf(atmstr, "%s: %s", "SConcert::DataCallback: Lock failed", v24);
    Logger.g->Panic(atmstr);
  }
  v15 = bufsize1;
  v16 = bufsize2;
  if ( bufsize1 + bufsize2 != v13 )
    Logger.g->Panic("SConcert::DataCallback: Buffer size mismatch");
  v17 = buf1;
  if ( buf1 )
  {
    memcpy(buf1, outsamples_0, bufsize1);
    v15 = bufsize1;
    v17 = buf1;
    v16 = bufsize2;
  }
  if ( buf2 )
  {
    memcpy(buf2, (char *)outsamples_0 + v15, v16);
    v15 = bufsize1;
    v17 = buf1;
  }
  v18 = this->StreamBuffer->Unlock(
          v17,
          v15,
          buf2,
          bufsize2);
  if ( v18 )
  {
    v25 = DXGetErrorStringA(v18);
    sprintf(atmstr, "%s: %s", "SConcert::DataCallback: Unlock failed", v25);
    Logger.g->Panic(atmstr);
  }
  v19 = v13 + this->WriteCursor;
  v20 = this->StreamStarted == 0;
  this->WriteCursor = v19;
  v21 = v19;
  if ( v20 && v19 >= 0x20000 )
  {
    v22 = this->StreamBuffer->Play(
            0,
            0,
            1);
    v23 = v22;
    if ( v22 )
    {
    // _com_error init (manual)
      v36 = 0;
      v26 = ""/* ErrorMessage placeholder */;
      Logger.g->Panic("SConcert::DataCallback: Play failed, error: 0x%x - %s", v23, v26);
    }
    v21 = this->WriteCursor;
    this->StreamStarted = 1;
  }
  this->WriteCursor = v21 & 0x3FFFF;
  p_StreamBuffer = &this->StreamBufferCriticalSection;
LABEL_16:
  LeaveCriticalSection(p_StreamBuffer);
}

//----- (0047FCC0) --------------------------------------------------------

void SWave::DataCallback(short *left, short *right, int num_samples, int stereo, int freq)

{
  int v7;
  int v8;
  short *v9; // edx
  this->WaveFMT.wFormatTag = 1;
  this->WaveFMT.wBitsPerSample = 16;
  v7 = (unsigned short)((stereo == 2) + 1);
  this->WaveFMT.nSamplesPerSec = freq;
  this->WaveFMT.nChannels = v7;
  this->WaveFMT.nBlockAlign = 2 * v7;
  this->WaveFMT.nAvgBytesPerSec = 2 * freq * v7;
  if ( stereo == 2 )
  {
    v8 = 0;
    if ( num_samples > 0 )
    {
      v9 = right;
      do
      {
        outsamples[v8][0] = left[v8];
        outsamples[v8][1] = right[v8];
        v8++;
      }
      while ( v8 < num_samples );
    }
    this->DecodeBuffer->Write(outsamples, 4 * num_samples);
  }
  else
  {
    this->DecodeBuffer->Write(left, 2 * num_samples);
  }
}

//----- (0047FD80) --------------------------------------------------------

void SConcert::DumpChannels()

{
  int i;
  int size;
  SHeap<SSoundProp>::Element *j; // eax
  SHeap<SSoundProp>::Element *array; // ecx
  const char *v6; // eax
  Logger.g->Log(0, "SConcert::DumpChannels: Channels %d", this->Sounds.occupied);
  for ( i = -1;
        ;
 Logger.g->Log(
          0,
          "%d %s (%f,%f,%f) mdist=%f  %s",
          i,
          this->SoundCache.array[array[i].data.CacheIdx].data.FileName,
          array[i].data.Position.x,
          array[i].data.Position.y,
          array[i].data.Position.z,
          array[i].data.MinDistance,
          v6) )
  {
    size = this->Sounds.size;
    if ( ++i >= size )
      break;
    for ( j = &this->Sounds.array[i]; j->use != 0x7FFFFFFF; ++j )
    {
      if ( ++i >= size )
        return;
    }
    if ( i < 0 )
      break;
    array = this->Sounds.array;
    v6 = "Enabled";
    if ( !array[i].data.Enabled )
      v6 = "Disabled";
  }
}

//----- (0047FF50) --------------------------------------------------------

void SConcert::HandleDirectSoundDeviceChange()

{
  int v2;
  int size;
  SHeap<SSoundProp>::Element *v4; // eax
  int v5;
  int v6;
  SHeap<SSoundCacheProp>::Element *v7; // eax
  IDirectSoundBuffer *v8; // ecx
  IDirectSound3DListener *Listener; // ecx
  IDirectSound8 *DSound; // ecx
  IDirectSound8 **p_DSound; // esi
  HRESULT v12;
  const char *v13; // eax
  int v14;
  int v15;
  SHeap<SSoundCacheProp>::Element *v16; // eax
  int v17;
  SHeap<SSoundProp>::Element *array; // ecx
  IDirectSound3DBuffer *Buffer3d; // edx
  IDirectSoundBuffer *Buffer; // ecx
  IDirectSoundBuffer *v21; // ecx
  char *FileName; // eax
  SHeap<SSoundCacheProp>::Element *v23; // ecx
  IDirectSoundBuffer *StreamBuffer;
  char atmstr[200];
  v2 = -1;
  while ( 1 )
  {
    size = this->Sounds.size;
    if ( ++v2 >= size )
      break;
    v4 = &this->Sounds.array[v2];
    while ( v4->use != 0x7FFFFFFF )
    {
      ++v2;
      ++v4;
      if ( v2 >= size )
        goto LABEL_6;
    }
    if ( v2 < 0 )
      break;
    v17 = v2;
    this->Sounds.array[v2].data.Buffer->GetCurrentPosition((LPDWORD)&this->Sounds.array[v2].data.PlaybackPosition,
      0);
    array = this->Sounds.array;
    Buffer3d = array[v2].data.Buffer3d;
    if ( Buffer3d )
    {
      Buffer3d->Release();
      this->Sounds.array[v17].data.Buffer3d = 0;
      array = this->Sounds.array;
    }
    Buffer = array[v17].data.Buffer;
    if ( Buffer )
    {
      Buffer->Release();
      this->Sounds.array[v17].data.Buffer = 0;
    }
  }
LABEL_6:
  v5 = -1;
  while ( 1 )
  {
    v6 = this->SoundCache.size;
    if ( ++v5 >= v6 )
      break;
    v7 = &this->SoundCache.array[v5];
    while ( v7->use != 0x7FFFFFFF )
    {
      ++v5;
      ++v7;
      if ( v5 >= v6 )
        goto LABEL_11;
    }
    if ( v5 < 0 )
      break;
    v21 = this->SoundCache.array[v5].data.Buffer;
    if ( v21 )
    {
      v21->Release();
      this->SoundCache.array[v5].data.Buffer = 0;
    }
  }
LABEL_11:
  StreamBuffer = this->StreamBuffer;
  StreamThreadDeviceChange = 1;
  EnterCriticalSection(&this->StreamBufferCriticalSection);
  v8 = this->StreamBuffer;
  if ( v8 )
  {
    v8->Release();
    this->StreamBuffer = 0;
  }
  Listener = this->Listener;
  if ( Listener )
  {
    Listener->Release();
    this->Listener = 0;
  }
  DSound = this->DSound;
  p_DSound = &this->DSound;
  if ( DSound )
  {
    DSound->Release();
    *p_DSound = 0;
  }
  LeaveCriticalSection(&this->StreamBufferCriticalSection);
  StreamThreadDeviceChange = 0;
  v12 = DirectSoundCreate8(&this->DefaultDeviceGuid, &this->DSound, 0);
  if ( v12 )
  {
    v13 = DXGetErrorStringA(v12);
    sprintf(atmstr, "%s: %s", "SConcert::Concert: DirectSoundCreate failed", v13);
    Logger.g->Warning(atmstr);
  }
  this->InitDirectSound();
  if ( StreamBuffer )
    this->CreateStreamBuffer();
  if ( *p_DSound )
  {
    v14 = -1;
    while ( 1 )
    {
      v15 = this->SoundCache.size;
      if ( ++v14 >= v15 )
        break;
      v16 = &this->SoundCache.array[v14];
      while ( v16->use != 0x7FFFFFFF )
      {
        ++v14;
        ++v16;
        if ( v14 >= v15 )
          goto LABEL_27;
      }
      if ( v14 < 0 )
        break;
      if ( !this->LoadSoundInternal(v14) )
      {
 Logger.g->Log(
          0,
          "SConcert::ReclaimSoundCacheBuffers: Failed to reload sound: %s",
          this->SoundCache.array[v14].data.FileName);
        FileName = this->SoundCache.array[v14].data.FileName;
        if ( FileName )
        {
          delete[] FileName;
          this->SoundCache.array[v14].data.FileName = 0;
        }
        if ( v14 >= this->SoundCache.size || (v23 = this->SoundCache.array, v23[v14].use != 0x7FFFFFFF) )
          Logger.g->Panic("SHeap::Remove: invalid index (%d)", v14);
        v23[v14].use = this->SoundCache.nextempty;
        --this->SoundCache.occupied;
        this->SoundCache.nextempty = v14;
      }
    }
  }
LABEL_27:
  this->ReclaimPlayingSounds();
}

//----- (00480220) --------------------------------------------------------

void SConcert::InitDirectSound()

{
  char *buf; // eax
  char *v3; // eax
  size_t v4;
  char *v5; // eax
  char *v6; // eax
  size_t v7;
  IDirectSound8 *DSound; // eax
  char *v9; // ecx
  const char *v10; // eax
  char v11; // dl
  bool v12; // al
  HRESULT v13;
  IDirectSound8 *v14; // eax
  HRESULT v15;
  void **p_Listener; // esi
  HRESULT v17;
  const char *v18; // eax
  const char *v19; // eax
  const char *v20; // eax
  IDirectSoundBuffer *dsb;
  DSCAPS dsc;
  DSBUFFERDESC bd;
  char atmstr[200];
  if ( this->DSound )
  {
    buf = this->DeviceDescription.buf;
    if ( buf )
    {
      delete[] buf;
      this->DeviceDescription.buf = 0;
    }
    this->DeviceDescription.size = 7;
    v3 = new char[8u];
    v4 = this->DeviceDescription.size + 1;
    this->DeviceDescription.buf = v3;
    memcpy(v3, "unknown", v4);
    v5 = this->DeviceModule.buf;
    if ( v5 )
    {
      delete[] v5;
      this->DeviceModule.buf = 0;
    }
    this->DeviceModule.size = 7;
    v6 = new char[8u];
    v7 = this->DeviceModule.size + 1;
    this->DeviceModule.buf = v6;
    memcpy(v6, "unknown", v7);
    DirectSoundEnumerateA((LPDSENUMCALLBACKA)DSEnumCallback, this);
    DSound = this->DSound;
    dsc.dwSize = 96;
    DSound->GetCaps(&dsc);
    Logger.g->Log(0, "DSOUND Description: %s", this->DeviceDescription.buf);
    Logger.g->Log(0, "DSOUND Module: %s", this->DeviceModule.buf);
    Logger.g->Log(0, "  Maximum hardware mixing buffers %d", dsc.dwMaxHwMixingAllBuffers);
    Logger.g->Log(0, "  Maximum hardware mixing static buffers %d", dsc.dwMaxHwMixingStaticBuffers);
 Logger.g->Log(
      0,
      "  Maximum hardware mixing streaming buffers %d",
      dsc.dwMaxHwMixingStreamingBuffers);
    Logger.g->Log(0, "  Maximum hardware 3D buffers %d", dsc.dwMaxHw3DAllBuffers);
    Logger.g->Log(0, "  Maximum hardware 3D static buffers %d", dsc.dwMaxHw3DStaticBuffers);
    Logger.g->Log(0, "  Maximum hardware 3D streaming buffers %d", dsc.dwMaxHw3DStreamingBuffers);
    Logger.g->Log(0, "  Total hardware memory %d", dsc.dwTotalHwMemBytes);
    v9 = this->DeviceModule.buf;
    if ( v9 )
    {
      v10 = "SND801.VXD";
      while ( *v9 == *v10 )
      {
        if ( !*v9 )
          goto LABEL_12;
        v11 = v9[1];
        if ( v11 != v10[1] )
          break;
        v9 += 2;
        v10 += 2;
        if ( !v11 )
        {
LABEL_12:
          v12 = 1;
          goto LABEL_15;
        }
      }
      v12 = 0;
    }
    else
    {
      v12 = 0;
    }
LABEL_15:
    this->TerratecHack = v12;
    if ( v12 )
      Logger.g->Log(0, "Activating workaround for Terratec cards!");
    v13 = this->DSound->SetCooperativeLevel(this->hWnd, 3u);
    if ( v13 )
    {
      v18 = DXGetErrorStringA(v13);
      sprintf(atmstr, "%s: %s", "SConcert::Concert: SetCooperativeLevel failed", v18);
      Logger.g->Panic(atmstr);
    }
    v14 = this->DSound;
    memset(&bd, 0, sizeof(bd));
    bd.dwSize = sizeof(bd);
    bd.dwFlags = 17;
    v15 = v14->CreateSoundBuffer(&bd, &dsb, 0);
    if ( v15 )
    {
      v19 = DXGetErrorStringA(v15);
      sprintf(atmstr, "%s: %s", "SConcert::Concert: CreateSoundBuffer failed", v19);
      Logger.g->Panic(atmstr);
    }
    p_Listener = (void **)&this->Listener;
    v17 = dsb->QueryInterface(IID_IDirectSound3DListener, p_Listener);
    if ( v17 )
    {
      v20 = DXGetErrorStringA(v17);
      sprintf(atmstr, "%s: %s", "SConcert::Concert: QueryInterface failed", v20);
      Logger.g->Panic(atmstr);
    }
    ((IDirectSound3DListener*)*p_Listener)->SetRolloffFactor(12.0f, DS3D_IMMEDIATE);
  }
}

//----- (00480560) --------------------------------------------------------

char SWave::LoadMP3(const char *filename, const char *panicstr)

{
#if defined(_M_X64)
  // TODO(x64): the decoder in mdec/{huffman,layer3}.cpp relies on x86 int-stride
  // pointer arithmetic through SMpegAudioDecoder (Callback field is 4 bytes on
  // x86, 8 on x64) — many `&Frame.version + 40*ch + 18*gr`-style accesses would
  // need to be rewritten to use symbolic si.ch[ch].gr[gr] access. For bring-up,
  // skip MP3 loading on x64 so the game boots; sounds that come as MP3 will be
  // silent. WAV files still work.
  (void)filename; (void)panicstr;
  static bool warned = false;
  if (!warned) {
    Logger.g->Log(0, "SWave::LoadMP3: MP3 decoder disabled on x64 (see huffman.cpp/layer3.cpp TODO)");
    warned = true;
  }
  return 0;
#else
  SStream *v4; // eax
  SStream *v5; // esi
  SMpegAudioDecoder *v6; // eax
  SStreamBuffer *v7; // eax
  SMpegAudioDecoder *v8; // eax
  SMpegAudioDecoder *v9; // eax
  SMpegAudioDecoder *v10; // esi
  SStreamBuffer *DecodeBuffer; // ecx
  int v12;
  unsigned char *v13; // eax
  SStreamBuffer *v14; // ecx
  SStreamBuffer *v15; // esi
  int v17;
  SMpegAudioDecoder *decoder;
  int *v19;
  int v20;
  v19 = &v17;
  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SWave::LoadMP3: opening '%s'", filename);
  #endif
  v4 = FileSystem.OpenRead(filename, panicstr);
  v5 = v4;
  if ( !v4 )
    return 0;
  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SWave::LoadMP3: stream=%p, allocating SStreamBuffer", v5);
  #endif
  v6 = (SMpegAudioDecoder *)operator new(sizeof(SStreamBuffer));
  decoder = v6;
  v20 = 0;
  if ( v6 )
    v7 = new (v6) SStreamBuffer();
  else
    v7 = 0;
  v20 = -1;
  this->DecodeBuffer = v7;
  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SWave::LoadMP3: DecodeBuffer=%p, allocating decoder", v7);
  #endif
  v8 = (SMpegAudioDecoder *)operator new(sizeof(SMpegAudioDecoder));
  decoder = v8;
  v20 = 1;
  if ( v8 )
  {
    #ifdef HD_DEBUG_AUDIO
    Logger.g->Log(0, "SWave::LoadMP3: constructing decoder at %p, this=%p vftable=%p", v8, this, this->vftable);
    #endif
    v9 = new (v8) SMpegAudioDecoder(v5, (SMpegAudioCallBack*)this);
    v10 = v9;
  }
  else
  {
    v10 = 0;
  }
  decoder = v10;
  v20 = 2;
  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SWave::LoadMP3: starting decode loop, decoder=%p", v10);
  #endif
  while ( v10->DecodeFrame() )
    ;
  if ( v10 )
  {
    v10->~SMpegAudioDecoder();
    ::operator delete(v10);
  }
  DecodeBuffer = this->DecodeBuffer;
  v20 = -1;
  v12 = DecodeBuffer->Seek(0, 2) - 4514 * this->WaveFMT.nChannels;
  this->Size = v12;
  if ( v12 <= 0 )
    Logger.g->Panic("SWave::LoadMP3: Sample too short");
  v13 = new unsigned char[v12];
  v14 = this->DecodeBuffer;
  this->Data = v13;
  v14->Seek(2210 * this->WaveFMT.nChannels, 0);
  this->DecodeBuffer->Read(this->Data, this->Size);
  v15 = this->DecodeBuffer;
  if ( v15 )
  {
    this->DecodeBuffer->~SStreamBuffer();
    ::operator delete(v15);
    this->DecodeBuffer = 0;
  }
  return 1;
#endif // _M_X64
}

//----- (00480730) --------------------------------------------------------

char SConcert::LoadSoundInternal(int i)

{
  SHeap<SSoundCacheProp> *p_SoundCache; // esi
  SHeap<SSoundCacheProp>::Element *array; // esi
  int v5;
  IDirectSound8 *DSound; // ecx
  int v7;
  SHeap<SSoundCacheProp>::Element *v8; // esi
  char *v9; // eax
  SConcert *v10; // esi
  HRESULT v11;
  HRESULT v12;
  SSoundCacheProp *v13; // eax
  const char *v14; // eax
  const char *v15; // eax
  _com_error *p_err = nullptr; // placeholder
  void *buf;
  DWORD bufsize;
  int index;
  SConcert *v20;
  DSBUFFERDESC bd;
  SWave wave;
  char f_buf[260];
  char atmstr[200];
  int v25;
  v20 = this;
  p_SoundCache = &this->SoundCache;
  index = i;
  sprintf(f_buf, "sounds/%s", this->SoundCache.array[i].data.FileName);
  // wave.__vftable = (SWave_vtbl *)SWave::`vftable';
  wave.Data = 0;
  v25 = 0;
  if ( strstr(f_buf, ".mp3") )
  {
    if ( !wave.LoadMP3(f_buf, 0) )
    {
LABEL_3:
      // ~SWave() destructor handles wave.Data cleanup automatically
      return 0;
    }
  }
  else
  {
    if ( !strstr(f_buf, ".wav") )
    {
      v13 = &p_SoundCache->array[index].data;
      Logger.g->Panic("SConcert::PrecacheSound: unknown file type: %s", v13->FileName);
    }
    if ( !wave.LoadWAV(f_buf, 0) )
      goto LABEL_3;
  }
  p_SoundCache->array[i].data.Duration = (float)wave.Size / (float)wave.WaveFMT.nAvgBytesPerSec;
  p_SoundCache->array[i].data.Size = wave.Size;
  array = p_SoundCache->array;
  bd.dwSize = sizeof(bd);
  if ( v20->TerratecHack )
  {
    v5 = 194;
    if ( array[i].data.Positional )
      v5 = 131122;
  }
  else
  {
    v5 = 393264;
    if ( !array[i].data.Positional )
      v5 = 262336;
  }
  DSound = v20->DSound;
  bd.dwFlags = v5 | 0x8000;
  bd.dwBufferBytes = wave.Size;
  bd.lpwfxFormat = &wave.WaveFMT;
  bd.dwReserved = 0;
  bd.guid3DAlgorithm = GUID_NULL;
  v7 = DSound->CreateSoundBuffer(&bd, &array[i].data.Buffer, 0);
  index = v7;
  if ( v7 < 0 )
  {
    // err.__vftable = (_com_error_vtbl *)_com_error::`vftable';
    /* err.m_hresult = v7; */
    /* err.m_perrinfo = 0; */
    /* err.m_pszMsg = 0; */
    v25 = 1;
    v8 = v20->SoundCache.array;
    v9 = ""/* ErrorMessage placeholder */;
 Logger.g->Log(
      0,
      "SConcert::CreateSoundFromWave: CreateSoundBuffer failed: %s, error: 0x%x - %s",
      v8[i].data.FileName,
      index,
      v9);
    v25 = 0;
    // err.__vftable = (_com_error_vtbl *)_com_error::`vftable';
    /* cleanup err */
    /* cleanup err msg */
    goto LABEL_3;
  }
  v10 = v20;
  v11 = v20->SoundCache.array[i].data.Buffer->Lock(0,
          wave.Size,
          &buf,
          &bufsize,
          0,
          0,
          2u);
  if ( v11 )
  {
    v14 = DXGetErrorStringA(v11);
    sprintf(atmstr, "%s: %s", "SConcert::CreateSoundFromWave: Lock failed", v14);
    Logger.g->Panic(atmstr);
  }
  if ( bufsize != wave.Size )
    Logger.g->Panic("SConcert::CreateSoundFromWave: Buffer size mismatch");
  memcpy(buf, wave.Data, wave.Size);
  v12 = v10->SoundCache.array[i].data.Buffer->Unlock(buf, bufsize, 0, 0);
  if ( v12 )
  {
    v15 = DXGetErrorStringA(v12);
    sprintf(atmstr, "%s: %s", "SConcert::CreateSoundFromWave: Unlock failed", v15);
    Logger.g->Panic(atmstr);
  }
  // ~SWave() destructor handles wave.Data cleanup automatically
  return 1;
}

//----- (00480B10) --------------------------------------------------------

char SWave::LoadWAV(char *filename, char *panicstr)

{
  SStream *v3; // eax
  SStream *v4; // esi
  unsigned int wsize;
  SWave *v7; // eax
  SWave *v8; // edi
  unsigned char *v9; // eax
  unsigned int dsize;
  int v12;
  const char *filenamea;
  const char *panicstra;
  SStream *is;
  const char *v16;
  const char *pExceptionObject;
  SWave *v18;
  SWaveHeader wh;
  SWaveDataHeader wdh;
  int *v21;
  int v22;
  v21 = &v12;
  v18 = this;
  filenamea = filename;
  panicstra = panicstr;
  v3 = FileSystem.OpenRead(filename, panicstr);
  v4 = v3;
  is = v3;
  if ( !v3 )
    return 0;
  v22 = 0;
  v4->Read(&wh, 36);
  // Format gating: only PCM (wFormatTag == 1) is supported by this DSound path.
  // Originally `throw "Not a WAV file"` — replaced with a graceful return so
  // modded assets in unsupported encodings (ADPCM, float, extensible) cause
  // a silent miss instead of an unhandled C-string exception. The miniaudio
  // backend (concert_ma.cpp) handles ADPCM/float natively; this softening
  // just prevents the x86 oracle from regressing on the same inputs.
  if ( strncmp(wh.riff, "RIFF", 4u)
    || strncmp(wh.wavefmt, "WAVEfmt ", 8u)
    || (wsize = wh.wsize, wh.wsize < 0x10)
    || wh.wFormatTag != 1 )
  {
    Logger.g->Log(0, "SWave::LoadWAV: '%s' rejected (riff=%.4s fmt=%.8s wsize=%u tag=%d) — DSound path only handles PCM",
                  filename, wh.riff, wh.wavefmt, (unsigned)wh.wsize, (int)wh.wFormatTag);
    v4->Release();
    return 0;
  }
  v7 = v18;
  memcpy(&v18->WaveFMT, &wh.wFormatTag, 16);
  v7->WaveFMT.cbSize = 0;
  v4->Seek(wsize - 16, 1);
  v4->Read(&wdh, 8);
  if ( strncmp(wdh.data, "data", 4u) )
  {
    Logger.g->Log(0, "SWave::LoadWAV: '%s' rejected (data chunk header missing: %.4s)",
                  filename, wdh.data);
    v4->Release();
    return 0;
  }
  v8 = v18;
  dsize = wdh.dsize;
  v18->Size = wdh.dsize;
  v9 = new unsigned char[dsize];
  v8->Data = v9;
  v4->Read(v9, v8->Size);
  v4->Release();
  return 1;
}

//----- (00480CB0) --------------------------------------------------------

void SConcert::NextTrack()

{
  int size;
  unsigned int v3;
  void *v4; // edi
  int v5;
  unsigned int v6;
  char *v7; // eax
  SConcert *v8; // esi
  char *v9; // ebx
  const char *v10; // edx
  const char *v11; // ecx
  size_t v12;
  if ( this->StreamFile2.size )
  {
    size = this->StreamFile1.size;
    if ( size )
    {
      v3 = size + 1;
      v4 = operator new[](v3);
      memcpy(v4, this->StreamFile1.buf, v3);
    }
    else
    {
      v4 = 0;
    }
    v5 = this->StreamFile2.size;
    if ( v5 )
    {
      v6 = v5 + 1;
      v7 = new char[v6];
      v12 = v6;
      v8 = this;
      v9 = v7;
      memcpy(v7, this->StreamFile2.buf, v12);
    }
    else
    {
      v8 = this;
      v9 = 0;
    }
    v10 = "";
    v11 = "";
    if ( v4 )
      v11 = (const char *)v4;
    if ( v9 )
      v10 = v9;
    v8->StartStreamingPlayback(v10, v11);
    if ( v9 )
      delete[] v9;
    if ( v4 )
      delete[] v4;
  }
}

//----- (00480DB0) --------------------------------------------------------

void SConcert::OptimizeSoundGroup(int sound_group)

{
  int v2;
  SConcert *v3; // eax
  int v4;
  int size;
  SHeap<SSoundProp>::Element *v6; // eax
  int v7;
  int v8;
  int v9;
  int *p_idx; // edi
  int v11;
  float v12; // xmm0_4
  int v13;
  float volume; // xmm1_4
  float v15; // xmm2_4
  bool v16; // cc
  int v17;
  float v18; // xmm1_4
  float v19; // xmm3_4
  int v20;
  float v21; // xmm1_4
  float v22; // xmm4_4
  int v23;
  float v24; // xmm1_4
  float v25; // xmm2_4
  float v26; // xmm1_4
  float v27; // xmm2_4
  int v28;
  int v29;
  float v30;
  int i;
  int idx;
  SHeap<SSoundProp>::Element *v33; // eax
  bool Enabled; // cl
  int v35;
  SConcert *v36; // ecx
  SHeap<SSoundProp>::Element *array; // eax
  IDirectSoundBuffer *v39;
  IDirectSoundBuffer *Buffer;
  int v43;
  int v44;
  int v45;
  int v46;
  struct elem_t { int idx; float volume; };
  elem_t *v47;
  int v48;
  elem_t elems[128];
  v2 = 0;
  v45 = SoundGroupQuotas[sound_group];
  v3 = this;
  v4 = -1;
  v43 = 0;
  size = this->Sounds.size;
  v46 = size;
  while ( ++v4 < size )
  {
    v6 = &v3->Sounds.array[v4];
    while ( v6->use != 0x7FFFFFFF )
    {
      ++v4;
      ++v6;
      if ( v4 >= size )
        goto LABEL_8;
    }
    if ( v4 < 0 )
      break;
    v36 = this;
    array = this->Sounds.array;
    if ( array[v4].data.Looped && array[v4].data.Buffer3d && array[v4].data.SoundGroup == sound_group )
    {
      elems[v43].idx = v4;
      {
        float dx = this->ListenerPosition.x - array[v4].data.Position.x;
        float dy = this->ListenerPosition.y - array[v4].data.Position.y;
        float dz = this->ListenerPosition.z - array[v4].data.Position.z;
        float dist = sqrtf(dx * dx + dy * dy + dz * dz);
        elems[v43].volume = this->Sounds.array[v4].data.MinDistance / dist;
      }
      v36 = this;
      size = v46;
      v2 = ++v43;
    }
    else
    {
      v2 = v43;
    }
    v3 = v36;
  }
LABEL_8:
  v7 = v45;
  v8 = 0;
  if ( v2 > v45 )
  {
    v9 = 0;
    v44 = 0;
    if ( v45 > 0 )
    {
      p_idx = (int *)elems;
      v11 = v2;
      v47 = elems;
      v48 = v2;
      do
      {
        v12 = -1.0f;
        v13 = v9;
        if ( v9 < v2 )
        {
          if ( v11 >= 4 )
          {
            do
            {
              volume = elems[v13].volume;
              v15 = v12;
              if ( volume > v12 )
                v12 = elems[v13].volume;
              v16 = volume <= v15;
              v17 = v13;
              v18 = elems[v13 + 1].volume;
              v19 = v12;
              if ( v16 )
                v17 = v8;
              if ( v18 > v12 )
                v12 = elems[v13 + 1].volume;
              v16 = v18 <= v19;
              v20 = v13 + 1;
              v21 = elems[v13 + 2].volume;
              v22 = v12;
              if ( v16 )
                v20 = v17;
              if ( v21 > v12 )
                v12 = elems[v13 + 2].volume;
              v16 = v21 <= v22;
              v23 = v13 + 2;
              v24 = elems[v13 + 3].volume;
              v25 = v12;
              if ( v16 )
                v23 = v20;
              if ( v24 > v12 )
                v12 = elems[v13 + 3].volume;
              v8 = v13 + 3;
              if ( v24 <= v25 )
                v8 = v23;
              v13 += 4;
            }
            while ( v13 < v2 - 3 );
            p_idx = &v47->idx;
          }
          for ( ; v13 < v2; v8 = v28 )
          {
            v26 = elems[v13].volume;
            v27 = v12;
            if ( v26 > v12 )
              v12 = elems[v13].volume;
            v28 = v13;
            if ( v26 <= v27 )
              v28 = v8;
            ++v13;
          }
        }
        v29 = *p_idx;
        v30 = *((float *)p_idx + 1);
        *p_idx = elems[v8].idx;
        p_idx[1] = LODWORD(elems[v8].volume);
        p_idx += 2;
        elems[v8].volume = v30;
        v9 = v44 + 1;
        v11 = v48 - 1;
        elems[v8].idx = v29;
        v44 = v9;
        --v48;
        v47 = (elem_t *)p_idx;
      }
      while ( v9 < v45 );
      v7 = v45;
    }
  }
  for ( i = v2 - 1; i >= 0; --i )
  {
    idx = elems[i].idx;
    v33 = this->Sounds.array;
    Enabled = v33[elems[i].idx].data.Enabled;
    if ( i >= v7 )
    {
      if ( Enabled )
      {
        Buffer = v33[elems[i].idx].data.Buffer;
        Buffer->Stop();
        v7 = v45;
        this->Sounds.array[idx].data.Enabled = 0;
      }
    }
    else if ( !Enabled )
    {
      v39 = v33[elems[i].idx].data.Buffer;
      v35 = v39->Play(0, 0, 1u);
      v7 = v45;
      if ( v35 >= 0 )
        this->Sounds.array[idx].data.Enabled = 1;
    }
  }
}

//----- (004810B0) --------------------------------------------------------

double SConcert::PlaySound(int cacheidx, float volume, float panning, int channelid)

{
  int v5;
  int v6;
  int size;
  int v9;
  SHeap<SSoundProp>::Element *v10; // eax
  SHeap<SSoundCacheProp>::Element *v11; // ecx
  int v12;
  int v13;
  SHeap<SSoundProp>::Element *v14; // eax
  int v15;
  unsigned int v16;
  SHeap<SSoundProp>::Element *v17; // ebx
  int v18;
  SHeap<SSoundProp>::Element *array; // ecx
  SHeap<SSoundProp>::Element *v21; // eax
  int v22;
  HRESULT v23;
  HRESULT v24;
  float v25; // xmm0_4
  HRESULT v26;
  SHeap<SSoundProp>::Element *v27; // eax
  const char *v28; // eax
  const char *v29; // eax
  const char *v30; // eax
  IDirectSoundBuffer *v31;
  IDirectSoundBuffer *Buffer;
  char atmstr[200];
  v5 = channelid;
  v6 = cacheidx;
  if ( this->TransientSounds >= 16 )
  {
    this->Update(0);
    if ( this->TransientSounds >= 16 )
      return 0.0;
  }
  if ( channelid >= 0 )
  {
    size = this->Sounds.size;
    v9 = -1;
    while ( ++v9 < size )
    {
      v10 = &this->Sounds.array[v9];
      while ( v10->use != 0x7FFFFFFF )
      {
        ++v9;
        ++v10;
        if ( v9 >= size )
          goto LABEL_9;
      }
      if ( v9 < 0 )
        break;
      array = this->Sounds.array;
      if ( !array[v9].data.Looped && !array[v9].data.Buffer3d && array[v9].data.ChannelID == channelid )
        return 0.0;
    }
LABEL_9:
    v6 = cacheidx;
  }
  if ( channelid != -1 && v6 >= 0 && v6 < this->SoundCache.size )
  {
    v11 = this->SoundCache.array;
    if ( v11[v6].use == 0x7FFFFFFF && v11[v6].data.Positional )
    {
      v12 = -1;
      while ( 1 )
      {
        v13 = this->Sounds.size;
        if ( ++v12 >= v13 )
          break;
        v14 = &this->Sounds.array[v12];
        while ( v14->use != 0x7FFFFFFF )
        {
          ++v12;
          ++v14;
          if ( v12 >= v13 )
            goto LABEL_20;
        }
        if ( v12 < 0 )
          break;
        v21 = this->Sounds.array;
        if ( !v21[v12].data.Looped
          && !v21[v12].data.Buffer3d
          && v21[v12].data.ChannelID >= 0
          && !v21[v12].data.HasBeenDucked )
        {
          v21[v12].data.Volume -= 1800;
          this->Sounds.array[v12].data.HasBeenDucked = 1;
          v22 = VolumeTable[this->Volume[2]] + 1000 + this->Sounds.array[v12].data.Volume;
          if ( v22 < 0 )
          {
            if ( v22 < -10000 )
              v22 = -10000;
          }
          else
          {
            v22 = 0;
          }
          Buffer = this->Sounds.array[v12].data.Buffer;
          v23 = Buffer->SetVolume(v22);
          if ( v23 )
          {
            v28 = DXGetErrorStringA(v23);
            sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetVolume failed", v28);
            Logger.g->Panic(atmstr);
          }
        }
      }
LABEL_20:
      v5 = channelid;
    }
  }
  v15 = this->SpawnSound(cacheidx, 0);
  if ( v15 < 0 )
    return 0.0;
  v16 = v15;
  this->Sounds.array[v16].data.ChannelID = v5;
  this->Sounds.array[v16].data.Looped = 0;
  this->Sounds.array[v16].data.Volume = (int)(volume * 100.0f);
  v17 = this->Sounds.array;
  v18 = VolumeTable[this->Volume[(channelid != -1) + 1]] + 1000 + v17[v15].data.Volume;
  if ( v18 < 0 )
  {
    if ( v18 < -10000 )
      v18 = -10000;
  }
  else
  {
    v18 = 0;
  }
  v24 = v17[v15].data.Buffer->SetVolume(v18);
  if ( v24 )
  {
    v29 = DXGetErrorStringA(v24);
    sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetVolume failed", v29);
    Logger.g->Panic(atmstr);
  }
  v25 = 10000.0f;
  if ( (float)(panning * 100.0) < 10000.0 )
    v25 = ((panning * 100.0f) > -10000.0f ? (panning * 100.0f) : -10000.0f);
  this->Sounds.array[v16].data.Panning = (int)v25;
  v26 = this->Sounds.array[v16].data.Buffer->SetPan(this->Sounds.array[v16].data.Panning);
  if ( v26 )
  {
    v30 = DXGetErrorStringA(v26);
    sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetPan failed", v30);
    Logger.g->Panic(atmstr);
  }
  v31 = this->Sounds.array[v16].data.Buffer;
  v31->Play(0, 0, 0);
  v27 = this->Sounds.array;
  ++this->TransientSounds;
  return this->SoundCache.array[v27[v16].data.CacheIdx].data.Duration;
}

//----- (00481440) --------------------------------------------------------

double SConcert::PlaySound(int cacheidx, float min_distance, float x, float y, float z)

{
  int v8;
  unsigned int v9;
  HRESULT v10;
  HRESULT v11;
  IDirectSoundBuffer *Buffer; // eax
  SHeap<SSoundProp>::Element *array; // eax
  const char *v15; // eax
  const char *v16; // eax
  IDirectSound3DBuffer *v17;
  IDirectSound3DBuffer *Buffer3d;
  char atmstr[200];
  if ( this->TransientSounds >= 16 )
  {
    this->Update(0);
    if ( this->TransientSounds >= 16 )
      return 0.0;
  }
  v8 = this->SpawnSound(cacheidx, 1);
  if ( v8 < 0 )
    return 0.0;
  v9 = v8;
  this->Sounds.array[v9].data.Looped = 0;
  this->Sounds.array[v9].data.MinDistance = min_distance;
  this->Sounds.array[v9].data.Position.x = x;
  this->Sounds.array[v9].data.Position.y = y;
  this->Sounds.array[v9].data.Position.z = z;
  Buffer3d = this->Sounds.array[v8].data.Buffer3d;
  v10 = Buffer3d->SetMinDistance(this->MinDistanceMultiplier * min_distance, DS3D_IMMEDIATE);
  if ( v10 )
  {
    v15 = DXGetErrorStringA(v10);
    sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetMinDistance failed", v15);
    Logger.g->Panic(atmstr);
  }
  v17 = this->Sounds.array[v9].data.Buffer3d;
  v11 = v17->SetPosition(this->ReverseStereo * x,
          y,
          z,
          0);
  if ( v11 )
  {
    v16 = DXGetErrorStringA(v11);
    sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetPosition failed", v16);
    Logger.g->Panic(atmstr);
  }
  Buffer = this->Sounds.array[v9].data.Buffer;
  Buffer->Play(0, 0, 0);
  array = this->Sounds.array;
  ++this->TransientSounds;
  return this->SoundCache.array[array[v9].data.CacheIdx].data.Duration;
}

//----- (00481610) --------------------------------------------------------

float SConcert::PlaySound(const char *filename, float volume, float panning, int channelid)

{
  int cacheidx = this->PrecacheSound(filename, false);
  return this->PlaySoundById(cacheidx, volume, panning, channelid);
}

//----- (00481650) --------------------------------------------------------

float SConcert::PlaySound3D(const char *filename, float min_distance, float x, float y, float z)

{
  int cacheidx = this->PrecacheSound(filename, true);
  return this->PlaySound3DById(cacheidx, min_distance, x, y, z);
}

// ById variants — take cache index instead of filename

float SConcert::PlaySoundById(int cacheidx, float volume, float panning, int channelid)
{
  return (float)this->PlaySound(cacheidx, volume, panning, channelid);
}

float SConcert::PlaySound3DById(int cacheidx, float min_distance, float x, float y, float z)
{
  return (float)this->PlaySound(cacheidx, min_distance, x, y, z);
}

int SConcert::CreateSoundById(int cacheidx, float volume, float panning)
{
  return this->CreateSound(cacheidx, volume, panning);
}

int SConcert::CreateSound3DById(int cacheidx, int sound_group, float min_distance, float x, float y, float z)
{
  return this->CreateSound(cacheidx, sound_group, min_distance, x, y, z);
}

//----- (004816A0) --------------------------------------------------------

int SConcert::PrecacheSound(const char *filename, bool positional)

{
  SHeap<SSoundCacheProp> *p_SoundCache; // ebx
  int result;
  int size;
  SHeap<SSoundCacheProp>::Element *v7; // ecx
  int nextempty;
  SHeap<SSoundCacheProp>::Element *array; // ecx
  int v10;
  SHeap<SSoundCacheProp>::Element *v11; // eax
  int v12;
  int v13;
  int maxsize;
  int v15;
  SHeap<SSoundCacheProp>::Element *v16; // eax
  int v17;
  int v18;
  int v19;
  char *v20; // eax
  SHeap<SSoundCacheProp> *v22;
  int v23;
  if ( !this->DSound || !filename || !strlen(filename) )
    return -1;
  p_SoundCache = &this->SoundCache;
  result = -1;
  v22 = p_SoundCache;
  size = p_SoundCache->size;
  while ( ++result < size )
  {
    v7 = &p_SoundCache->array[result];
    while ( v7->use != 0x7FFFFFFF )
    {
      ++result;
      ++v7;
      if ( result >= size )
        goto LABEL_9;
    }
    if ( result < 0 )
      break;
    v12 = strcmp(filename, p_SoundCache->array[result].data.FileName);
    if ( v12 )
      v12 = v12 < 0 ? -1 : 1;
    p_SoundCache = v22;
    if ( !v12 )
      return result;
  }
LABEL_9:
  ++p_SoundCache->occupied;
  nextempty = p_SoundCache->nextempty;
  if ( nextempty < 0 )
  {
    v13 = p_SoundCache->size;
    maxsize = p_SoundCache->maxsize;
    if ( v13 == maxsize )
    {
      if ( maxsize >= 16 )
        v15 = 6 * maxsize / 5;
      else
        v15 = 16;
      v23 = v15;
      // x64: literal `28` is x86 sizeof(SHeap<SSoundCacheProp>::Element); on x64
      // the two pointers + bool padding push it to 40. Same family as group.cpp:1884.
      v16 = (SHeap<SSoundCacheProp>::Element *)realloc(p_SoundCache->array, sizeof(SHeap<SSoundCacheProp>::Element) * v15);
      v17 = p_SoundCache->maxsize;
      p_SoundCache->array = v16;
      memset(&v16[v17], 0, sizeof(SHeap<SSoundCacheProp>::Element) * (v23 - v17));
      v13 = p_SoundCache->size;
      p_SoundCache->maxsize = v23;
    }
    p_SoundCache->array[v13].use = 0x7FFFFFFF;
    nextempty = p_SoundCache->size;
    p_SoundCache->size = nextempty + 1;
  }
  else
  {
    array = p_SoundCache->array;
    v10 = nextempty;
    p_SoundCache->nextempty = p_SoundCache->array[nextempty].use;
    array[v10].use = 0x7FFFFFFF;
    v11 = p_SoundCache->array;
    memset(&v11[v10].data, 0, 16);
    memset(&v11[v10].data.Duration, 0, 8);
  }
  v18 = (int)&p_SoundCache->array[nextempty];
  p_SoundCache->array[nextempty].data.FileName = _strdup(filename);
  v19 = nextempty;
  p_SoundCache->array[v19].data.RefCount = 0;
  p_SoundCache->array[v19].data.Positional = positional;
  if ( this->LoadSoundInternal(nextempty) )
    return nextempty;
  v20 = p_SoundCache->array[v19].data.FileName;
  if ( v20 )
  {
    delete[] v20;
    p_SoundCache->array[v19].data.FileName = 0;
  }
  p_SoundCache->Remove(nextempty);
  return -1;
}

//----- (004818C0) --------------------------------------------------------

void SConcert::ReclaimPlayingSounds()

{
  SConcert *v1; // esi
  int v2;
  int size;
  SHeap<SSoundProp>::Element *i; // eax
  SHeap<SSoundProp>::Element *array; // edx
  int v6;
  HRESULT v7;
  SHeap<SSoundProp>::Element *v8; // ecx
  HRESULT v9;
  HRESULT v10;
  SHeap<SSoundProp>::Element *v11; // eax
  HRESULT v12;
  HRESULT v13;
  SHeap<SSoundProp>::Element *v14; // eax
  int *Buffer; // ecx
  int v16;
  int v17;
  HRESULT v18;
  HRESULT v19;
  const char *v20; // eax
  const char *v21; // eax
  const char *v22; // eax
  const char *v23; // eax
  const char *v24; // eax
  const char *v25; // eax
  const char *v26; // eax
  IDirectSoundBuffer *v27;
  int CacheIdx;
  char atmstr[200];
  v1 = this;
  v2 = -1;
  while ( 1 )
  {
    size = v1->Sounds.size;
    if ( ++v2 >= size )
      break;
    for ( i = &v1->Sounds.array[v2]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v2 >= size )
        return;
    }
    if ( v2 < 0 )
      break;
    array = v1->Sounds.array;
    v6 = v2;
    CacheIdx = array[v2].data.CacheIdx;
    v7 = v1->DSound->DuplicateSoundBuffer(this->SoundCache.array[CacheIdx].data.Buffer,
           &array[v2].data.Buffer);
    if ( v7 )
    {
      v20 = DXGetErrorStringA(v7);
      sprintf(atmstr, "%s: %s", "SConcert::SpawnSound: DuplicateSoundBuffer failed", v20);
      Logger.g->Panic(atmstr);
    }
    v8 = this->Sounds.array;
    v1 = this;
    if ( this->SoundCache.array[CacheIdx].data.Positional )
    {
      v9 = v8[v6].data.Buffer->QueryInterface(
             IID_IDirectSound3DBuffer,
             (void **)&v8[v6].data.Buffer3d);
      if ( v9 )
      {
        v24 = DXGetErrorStringA(v9);
        sprintf(atmstr, "%s: %s", "SConcert::SpawnSound: QueryInterface failed", v24);
        Logger.g->Panic(atmstr);
      }
      v10 = this->Sounds.array[v6].data.Buffer3d->SetMinDistance(this->Sounds.array[v6].data.MinDistance * this->MinDistanceMultiplier, DS3D_IMMEDIATE);
      if ( v10 )
      {
        v23 = DXGetErrorStringA(v10);
        sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetMinDistance failed", v23);
        Logger.g->Panic(atmstr);
      }
      v11 = this->Sounds.array;
      v12 = v11[v6].data.Buffer3d->SetPosition(v11[v6].data.Position.x * this->ReverseStereo,
              *(float *)&v11[v6].data.Position.y,
              *(float *)&v11[v6].data.Position.z,
              0);
      if ( v12 )
      {
        v22 = DXGetErrorStringA(v12);
        sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetPosition failed", v22);
        Logger.g->Panic(atmstr);
      }
      v13 = this->Sounds.array[v6].data.Buffer->SetCurrentPosition(this->Sounds.array[v6].data.PlaybackPosition);
      if ( v13 )
        goto LABEL_28;
      v14 = this->Sounds.array;
      if ( v14[v6].data.Looped )
      {
        v14[v6].data.Enabled = 0;
      }
      else
      {
        Buffer = (int *)v14[v6].data.Buffer;
        v16 = *Buffer;
        if ( this->TerratecHack )
          (*(void (__stdcall **)(int *, unsigned int, unsigned int, unsigned int))(v16 + 48))(Buffer, 0, 0, 0);
        else
          (*(void (__stdcall **)(int *, unsigned int, unsigned int, int))(v16 + 48))(Buffer, 0, 0, 16);
      }
    }
    else
    {
      v8[v6].data.Buffer3d = 0;
      v17 = VolumeTable[this->Volume[(this->Sounds.array[v6].data.ChannelID != -1) + 1]]
          + 1000
          + this->Sounds.array[v6].data.Volume;
      if ( v17 < 0 )
      {
        if ( v17 < -10000 )
          v17 = -10000;
      }
      else
      {
        v17 = 0;
      }
      v27 = this->Sounds.array[v6].data.Buffer;
      v18 = v27->SetVolume(v17);
      if ( v18 )
      {
        v26 = DXGetErrorStringA(v18);
        sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetVolume failed", v26);
        Logger.g->Panic(atmstr);
      }
      v19 = this->Sounds.array[v6].data.Buffer->SetPan(this->Sounds.array[v6].data.Panning);
      if ( v19 )
      {
        v25 = DXGetErrorStringA(v19);
        sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetPan failed", v25);
        Logger.g->Panic(atmstr);
      }
      v13 = this->Sounds.array[v6].data.Buffer->SetCurrentPosition(this->Sounds.array[v6].data.PlaybackPosition);
      if ( v13 )
      {
LABEL_28:
        v21 = DXGetErrorStringA(v13);
        sprintf(atmstr, "%s: %s", "SConcert::CreateSound: SetCurrentPosition failed", v21);
        Logger.g->Panic(atmstr);
      }
      this->Sounds.array[v6].data.Buffer->Play(0,
        0,
        this->Sounds.array[v6].data.Looped);
      v1 = this;
    }
  }
}

//----- (00481C80) --------------------------------------------------------

void SConcert::ReclaimSoundCacheBuffers()

{
  int v2;
  int size;
  SHeap<SSoundCacheProp>::Element *i; // eax
  char *FileName; // eax
  SHeap<SSoundCacheProp>::Element *array; // ecx
  if ( this->DSound )
  {
    v2 = -1;
    while ( 1 )
    {
      size = this->SoundCache.size;
      if ( ++v2 >= size )
        break;
      for ( i = &this->SoundCache.array[v2]; i->use != 0x7FFFFFFF; ++i )
      {
        if ( ++v2 >= size )
          return;
      }
      if ( v2 < 0 )
        break;
      if ( !this->LoadSoundInternal(v2) )
      {
 Logger.g->Log(
          0,
          "SConcert::ReclaimSoundCacheBuffers: Failed to reload sound: %s",
          this->SoundCache.array[v2].data.FileName);
        FileName = this->SoundCache.array[v2].data.FileName;
        if ( FileName )
        {
          delete[] FileName;
          this->SoundCache.array[v2].data.FileName = 0;
        }
        if ( v2 >= this->SoundCache.size || (array = this->SoundCache.array, array[v2].use != 0x7FFFFFFF) )
          Logger.g->Panic("SHeap::Remove: invalid index (%d)", v2);
        array[v2].use = this->SoundCache.nextempty;
        --this->SoundCache.occupied;
        this->SoundCache.nextempty = v2;
      }
    }
  }
}

//----- (00481D50) --------------------------------------------------------

void SConcert::Release()

{
  if ( this->RefCount-- == 1 )
  {
    this->~SConcert();
    operator delete(this);
  }
}

//----- (00481D70) --------------------------------------------------------

void SConcert::ReleaseCachedSound(int idx)

{
  if ( idx >= 0 )
    --this->SoundCache.array[idx].data.RefCount;
}

//----- (00481D90) --------------------------------------------------------

void SConcert::ReleasePlayingSounds()

{
  int v2;
  int size;
  SHeap<SSoundProp>::Element *i; // eax
  int v5;
  SHeap<SSoundProp>::Element *array; // ecx
  IDirectSound3DBuffer *Buffer3d; // edx
  IDirectSoundBuffer *Buffer; // ecx
  v2 = -1;
  while ( 1 )
  {
    size = this->Sounds.size;
    if ( ++v2 >= size )
      break;
    for ( i = &this->Sounds.array[v2]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v2 >= size )
        return;
    }
    if ( v2 < 0 )
      break;
    v5 = v2;
    this->Sounds.array[v2].data.Buffer->GetCurrentPosition((LPDWORD)&this->Sounds.array[v2].data.PlaybackPosition,
      0);
    array = this->Sounds.array;
    Buffer3d = array[v2].data.Buffer3d;
    if ( Buffer3d )
    {
      Buffer3d->Release();
      this->Sounds.array[v5].data.Buffer3d = 0;
      array = this->Sounds.array;
    }
    Buffer = array[v5].data.Buffer;
    if ( Buffer )
    {
      Buffer->Release();
      this->Sounds.array[v5].data.Buffer = 0;
    }
  }
}

//----- (00481E30) --------------------------------------------------------

void SConcert::ReleaseSoundCacheBuffers()

{
  int v2;
  int size;
  SHeap<SSoundCacheProp>::Element *i; // eax
  IDirectSoundBuffer *Buffer; // ecx
  v2 = -1;
  while ( 1 )
  {
    size = this->SoundCache.size;
    if ( ++v2 >= size )
      break;
    for ( i = &this->SoundCache.array[v2]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v2 >= size )
        return;
    }
    if ( v2 < 0 )
      break;
    Buffer = this->SoundCache.array[v2].data.Buffer;
    if ( Buffer )
    {
      Buffer->Release();
      this->SoundCache.array[v2].data.Buffer = 0;
    }
  }
}

//----- (00481EE0) --------------------------------------------------------

void SConcert::RemoveSound(int idx)

{
  SHeap<SSoundProp>::Element *array; // eax
  int CacheIdx;
  SHeap<SSoundProp>::Element *v5; // ecx
  IDirectSound3DBuffer *Buffer3d; // edx
  IDirectSoundBuffer *Buffer; // ecx
  SHeap<SSoundProp>::Element *v8; // ecx
  if ( idx >= 0 && idx < this->Sounds.size )
  {
    array = this->Sounds.array;
    if ( array[idx].use == 0x7FFFFFFF )
    {
      CacheIdx = array[idx].data.CacheIdx;
      --this->SoundCache.array[CacheIdx].data.RefCount;
      v5 = this->Sounds.array;
      Buffer3d = v5[idx].data.Buffer3d;
      if ( Buffer3d )
      {
        Buffer3d->Release();
        this->Sounds.array[idx].data.Buffer3d = 0;
        v5 = this->Sounds.array;
      }
      Buffer = v5[idx].data.Buffer;
      if ( Buffer )
      {
        Buffer->Release();
        this->Sounds.array[idx].data.Buffer = 0;
      }
      if ( idx >= this->Sounds.size || (v8 = this->Sounds.array, v8[idx].use != 0x7FFFFFFF) )
        Logger.g->Panic("SHeap::Remove: invalid index (%d)", idx);
      v8[idx].use = this->Sounds.nextempty;
      --this->Sounds.occupied;
      this->Sounds.nextempty = idx;
    }
  }
}

// Iterate the SHeap free-list and tear down every live SSoundProp. Encapsulates
// the loop that used to be open-coded in superwindow.cpp (cast SIConcert*→SConcert*
// + walk Sounds.array). Both backends provide it via SIConcert.
void SConcert::RemoveAllSounds()
{
  for (int si = 0; si < this->Sounds.size; ++si) {
    if (this->Sounds.array[si].use == 0x7FFFFFFF)
      this->RemoveSound(si);
  }
}

//----- (00481F90) --------------------------------------------------------

void SConcert::SetListener(float x, float y, float z, float head_x, float head_y, float head_z, float up_x, float up_y, float up_z)

{
  IDirectSound3DListener *Listener; // ecx
  Listener = this->Listener;
  if ( Listener )
  {
    this->ListenerPosition.z = z;
    this->ListenerPosition.x = x;
    this->ListenerPosition.y = y;
    Listener->SetPosition(this->ReverseStereo * x,
      y,
      z,
      1);
    this->Listener->SetOrientation(
      this->ReverseStereo * head_x,
      head_y,
      head_z,
      this->ReverseStereo * up_x,
      up_y,
      up_z,
      DS3D_DEFERRED);
    this->Listener->CommitDeferredSettings();
    this->OptimizeSoundGroup(0);
    this->OptimizeSoundGroup(1);
  }
}

//----- (00482070) --------------------------------------------------------

void SConcert::SetReverseStereo(bool reverse)

{
  int v3;
  int size;
  SHeap<SSoundProp>::Element *i; // eax
  SHeap<SSoundProp>::Element *array; // edx
  v3 = -1;
  this->ReverseStereo = (float)(2 * !reverse - 1);
  while ( 1 )
  {
    size = this->Sounds.size;
    if ( ++v3 >= size )
      break;
    for ( i = &this->Sounds.array[v3]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v3 >= size )
        return;
    }
    if ( v3 < 0 )
      break;
    array = this->Sounds.array;
    if ( array[v3].data.Buffer3d )
      this->SetSoundPosition(v3,
        array[v3].data.Position.x,
        array[v3].data.Position.y,
        array[v3].data.Position.z);
  }
}

//----- (00482110) --------------------------------------------------------

void SConcert::SetSoundFrequency(int idx, unsigned int frequency)

{
  SHeap<SSoundProp>::Element *array; // ecx
  if ( idx >= 0 && idx < this->Sounds.size )
  {
    array = this->Sounds.array;
    if ( array[idx].use == 0x7FFFFFFF )
      array[idx].data.Buffer->SetFrequency(frequency);
  }
}

//----- (00482150) --------------------------------------------------------

void SConcert::SetSoundMinDistance(int idx, float min_distance)

{
  SHeap<SSoundProp>::Element *array; // eax
  IDirectSound3DBuffer *Buffer3d;
  if ( idx >= 0 && idx < this->Sounds.size )
  {
    array = this->Sounds.array;
    if ( array[idx].use == 0x7FFFFFFF )
    {
      array[idx].data.MinDistance = min_distance;
      Buffer3d = this->Sounds.array[idx].data.Buffer3d;
      Buffer3d->SetMinDistance(this->MinDistanceMultiplier * min_distance, DS3D_IMMEDIATE);
      this->OptimizeSoundGroup(this->Sounds.array[idx].data.SoundGroup);
    }
  }
}

//----- (004821C0) --------------------------------------------------------

void SConcert::SetSoundPosition(int idx, float x, float y, float z)

{
  SHeap<SSoundProp>::Element *array; // eax
  IDirectSound3DBuffer *Buffer3d;
  if ( idx >= 0 && idx < this->Sounds.size )
  {
    array = this->Sounds.array;
    if ( array[idx].use == 0x7FFFFFFF )
    {
      array[idx].data.Position.x = x;
      this->Sounds.array[idx].data.Position.y = y;
      this->Sounds.array[idx].data.Position.z = z;
      Buffer3d = this->Sounds.array[idx].data.Buffer3d;
      Buffer3d->SetPosition(this->ReverseStereo * x,
        y,
        z,
        0);
      this->OptimizeSoundGroup(this->Sounds.array[idx].data.SoundGroup);
    }
  }
}

//----- (00482250) --------------------------------------------------------

void SConcert::SetVolume(int volume_type, int value)

{
  IDirectSoundBuffer *StreamBuffer; // edx
  HRESULT v5;
  float v6; // xmm0_4
  int v7;
  int size;
  SHeap<SSoundProp>::Element *i; // eax
  SHeap<SSoundProp>::Element *array; // ebx
  IDirectSound3DBuffer *Buffer3d; // ecx
  HRESULT v12;
  int v13;
  HRESULT v14;
  int v15;
  int v16;
  SHeap<SSoundProp>::Element *j; // eax
  SHeap<SSoundProp>::Element *v18; // ebx
  int v19;
  const char *v20; // eax
  const char *v21; // eax
  const char *v22; // eax
  IDirectSoundBuffer *Buffer;
  IDirectSoundBuffer *v24;
  char atmstr[200];
  this->Volume[volume_type] = value;
  if ( volume_type )
  {
    if ( volume_type == 1 )
    {
      /* logf call removed */;
      v6 = (float)((double)expf(0.0f));

      v7 = -1;
      this->MinDistanceMultiplier = v6;
      while ( 1 )
      {
        size = this->Sounds.size;
        if ( ++v7 >= size )
          break;
        for ( i = &this->Sounds.array[v7]; i->use != 0x7FFFFFFF; ++i )
        {
          if ( ++v7 >= size )
            return;
        }
        if ( v7 < 0 )
          break;
        array = this->Sounds.array;
        Buffer3d = array[v7].data.Buffer3d;
        if ( Buffer3d )
        {
          v12 = Buffer3d->SetMinDistance(array[v7].data.MinDistance * this->MinDistanceMultiplier, DS3D_IMMEDIATE);
          if ( v12 )
          {
            v22 = DXGetErrorStringA(v12);
            sprintf(atmstr, "%s: %s", "SConcert::CreateSound: SetMinDistance failed", v22);
            Logger.g->Panic(atmstr);
          }
        }
        else if ( array[v7].data.ChannelID == -1 )
        {
          v13 = VolumeTable[this->Volume[1]] + 1000 + array[v7].data.Volume;
          if ( v13 < 0 )
          {
            if ( v13 < -10000 )
              v13 = -10000;
          }
          else
          {
            v13 = 0;
          }
          Buffer = this->Sounds.array[v7].data.Buffer;
          v14 = Buffer->SetVolume(v13);
          if ( v14 )
            goto LABEL_38;
        }
      }
    }
    else if ( volume_type == 2 )
    {
      v15 = -1;
      while ( 1 )
      {
        v16 = this->Sounds.size;
        if ( ++v15 >= v16 )
          break;
        for ( j = &this->Sounds.array[v15]; j->use != 0x7FFFFFFF; ++j )
        {
          if ( ++v15 >= v16 )
            return;
        }
        if ( v15 < 0 )
          break;
        v18 = this->Sounds.array;
        if ( !v18[v15].data.Buffer3d && v18[v15].data.ChannelID != -1 )
        {
          v19 = VolumeTable[this->Volume[2]] + 1000 + v18[v15].data.Volume;
          if ( v19 < 0 )
          {
            if ( v19 < -10000 )
              v19 = -10000;
          }
          else
          {
            v19 = 0;
          }
          v24 = this->Sounds.array[v15].data.Buffer;
          v14 = v24->SetVolume(v19);
          if ( v14 )
          {
LABEL_38:
            v20 = DXGetErrorStringA(v14);
            sprintf(atmstr, "%s: %s", "SConcert::PlaySound: SetVolume failed", v20);
            Logger.g->Panic(atmstr);
          }
        }
      }
    }
  }
  else
  {
    StreamBuffer = this->StreamBuffer;
    if ( StreamBuffer )
    {
      v5 = StreamBuffer->SetVolume(VolumeTable[this->Volume[0]]);
      if ( v5 )
      {
        v21 = DXGetErrorStringA(v5);
        sprintf(atmstr, "%s: %s", "SConcert::SetVolume: VOLUME_MUSIC/SetVolume failed", v21);
        Logger.g->Panic(atmstr);
      }
    }
  }
}

//----- (00482520) --------------------------------------------------------

int SConcert::SpawnSound(int cacheidx, bool positional)

{
  SHeap<SSoundCacheProp>::Element *array; // ecx
  int nextempty;
  SHeap<SSoundProp>::Element *v6; // ecx
  int size;
  int maxsize;
  int v9;
  SHeap<SSoundProp>::Element *v10; // eax
  int v11;
  HRESULT v12;
  SHeap<SSoundProp>::Element *v13; // ecx
  HRESULT v14;
  const char *v16; // eax
  const char *v17; // eax
  int v18;
  char atmstr[200];
  if ( cacheidx < 0 )
    return -1;
  if ( cacheidx >= this->SoundCache.size )
    return -1;
  array = this->SoundCache.array;
  if ( array[cacheidx].use != 0x7FFFFFFF || positional != array[cacheidx].data.Positional )
    return -1;
  ++this->Sounds.occupied;
  nextempty = this->Sounds.nextempty;
  if ( nextempty < 0 )
  {
    size = this->Sounds.size;
    maxsize = this->Sounds.maxsize;
    if ( size == maxsize )
    {
      if ( maxsize >= 16 )
        v9 = 6 * maxsize / 5;
      else
        v9 = 16;
      v18 = v9;
      // x64: literal `60` is x86 sizeof(SHeap<SSoundProp>::Element). Same family as group.cpp:1884.
      v10 = (SHeap<SSoundProp>::Element *)realloc(this->Sounds.array, sizeof(SHeap<SSoundProp>::Element) * v9);
      v11 = this->Sounds.maxsize;
      this->Sounds.array = v10;
      memset(&v10[v11], 0, sizeof(SHeap<SSoundProp>::Element) * (v18 - v11));
      size = this->Sounds.size;
      this->Sounds.maxsize = v18;
    }
    this->Sounds.array[size].use = 0x7FFFFFFF;
    nextempty = this->Sounds.size;
    this->Sounds.size = nextempty + 1;
  }
  else
  {
    v6 = this->Sounds.array;
    this->Sounds.nextempty = v6[nextempty].use;
    v6[nextempty].use = 0x7FFFFFFF;
    memset(&this->Sounds.array[nextempty].data, 0, sizeof(this->Sounds.array[nextempty].data));
  }
  ++this->SoundCache.array[cacheidx].data.RefCount;
  this->Sounds.array[nextempty].data.CacheIdx = cacheidx;
  v12 = this->DSound->DuplicateSoundBuffer(
           this->SoundCache.array[cacheidx].data.Buffer,
          &this->Sounds.array[nextempty].data.Buffer);
  if ( v12 )
  {
    v16 = DXGetErrorStringA(v12);
    sprintf(atmstr, "%s: %s", "SConcert::SpawnSound: DuplicateSoundBuffer failed", v16);
    Logger.g->Panic(atmstr);
  }
  v13 = &this->Sounds.array[nextempty];
  if ( this->SoundCache.array[cacheidx].data.Positional )
  {
    v14 = v13->data.Buffer->QueryInterface(IID_IDirectSound3DBuffer, (void **)&v13->data.Buffer3d);
    if ( v14 )
    {
      v17 = DXGetErrorStringA(v14);
      sprintf(atmstr, "%s: %s", "SConcert::SpawnSound: QueryInterface failed", v17);
      Logger.g->Panic(atmstr);
    }
  }
  else
  {
    v13->data.Buffer3d = 0;
  }
  return nextempty;
}

//----- (00482760) --------------------------------------------------------

void SConcert::StartStreamingPlayback(const char *filename1, const char *filename2)

{
  SStream *v4; // eax
  SStream *v5; // edi
  SMpegAudioDecoder *v6; // eax
  SMpegAudioDecoder *v7;
  HANDLE Thread;
  DWORD thread_id[4];
  #ifdef HD_DEBUG_AUDIO
  Logger.g->Log(0, "SConcert::StartStreamingPlayback: file1='%s' file2='%s' DSound=%p", filename1, filename2, this->DSound);
  #endif
  if ( this->DSound )
  {
    v4 = FileSystem.OpenRead(filename1, 0);
    v5 = v4;
    #ifdef HD_DEBUG_AUDIO
    Logger.g->Log(0, "SConcert::StartStreamingPlayback: OpenRead('%s') returned %p", filename1, v4);
    #endif
    if ( v4 )
    {
      this->StreamFile1 = filename1;
      this->StreamFile2 = filename2;
      this->CreateStreamBuffer();
      v6 = (SMpegAudioDecoder *)operator new(sizeof(SMpegAudioDecoder));
      thread_id[3] = 0;
      if ( v6 )
        v7 = new (v6) SMpegAudioDecoder(v5, static_cast<SMpegAudioCallBack*>(this));
      else
        v7 = 0;
      StreamThreadRunning = 1;
      StreamThreadShutdown = 0;
      Thread = CreateThread(0, 0, (LPTHREAD_START_ROUTINE)StreamThreadProc, v7, 0, (LPDWORD)thread_id);
      CloseHandle(Thread);
    }
  }
}

//----- (00482830) --------------------------------------------------------

void SConcert::StopStreamingPlayback()

{
  IDirectSoundBuffer *StreamBuffer; // ecx
  while ( StreamThreadRunning )
  {
    StreamThreadShutdown = 1;
    Sleep(0);
  }
  StreamThreadShutdown = 0;
  StreamBuffer = this->StreamBuffer;
  if ( StreamBuffer )
  {
    StreamBuffer->Release();
    this->StreamBuffer = 0;
  }
}

//----- (00482930) --------------------------------------------------------

void SConcert::Update(bool shutdown)

{
  bool v2; // bl
  unsigned int v4;
  _GUID *p_DefaultDeviceGuid; // ecx
  _GUID *p_newDefaultDeviceGuid; // edx
  bool v7;
  int v8;
  int size;
  SHeap<SSoundProp>::Element *i; // eax
  SHeap<SSoundProp>::Element *array; // eax
  HRESULT v12;
  const char *v13; // eax
  DWORD status;
  _GUID newDefaultDeviceGuid;
  char atmstr[200];
  v2 = shutdown;
  if ( this->DSound && !shutdown && GetDeviceID(&DSDEVID_DefaultPlayback, &newDefaultDeviceGuid) >= 0 )
  {
    v4 = 12;
    p_DefaultDeviceGuid = &this->DefaultDeviceGuid;
    p_newDefaultDeviceGuid = &newDefaultDeviceGuid;
    while ( p_DefaultDeviceGuid->Data1 == p_newDefaultDeviceGuid->Data1 )
    {
      p_DefaultDeviceGuid = (_GUID *)((char *)p_DefaultDeviceGuid + 4);
      p_newDefaultDeviceGuid = (_GUID *)((char *)p_newDefaultDeviceGuid + 4);
      v7 = v4 < 4;
      v4 -= 4;
      if ( v7 )
        goto LABEL_9;
    }
    this->DefaultDeviceGuid = newDefaultDeviceGuid;
    this->HandleDirectSoundDeviceChange();
LABEL_9:
    v2 = 0;
  }
  v8 = -1;
  while ( 1 )
  {
    size = this->Sounds.size;
    if ( ++v8 >= size )
      break;
    for ( i = &this->Sounds.array[v8]; i->use != 0x7FFFFFFF; ++i )
    {
      if ( ++v8 >= size )
        return;
    }
    if ( v8 < 0 )
      break;
    array = this->Sounds.array;
    if ( !array[v8].data.Looped )
    {
      status = 0;
      v12 = array[v8].data.Buffer->GetStatus(&status);
      if ( v12 )
      {
        v13 = DXGetErrorStringA(v12);
        sprintf(atmstr, "%s: %s", "SConcert::Update: GetStatus failed", v13);
        Logger.g->Panic(atmstr);
      }
      if ( v2 || (status & 1) == 0 )
      {
        this->RemoveSound(v8);
        --this->TransientSounds;
      }
    }
  }
}

