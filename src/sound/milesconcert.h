// sound/milesconcert.h
// SMilesConcert — the Panzers HD audio engine on the Miles Sound System 6.5c.
//
// Lifted from PANZERS.exe (HD, 2016). The class lives at 0x684300..0x6876ff,
// vftable 0x81ab4c (34 slots), object size 0x80, singleton pointer 0x92e798.
// It is created by the factory 0x684fb0, called from SDXWindow::Create
// 0x539d70 when options.ini says `Use Miles = 1`.
//
// The SWINE engine code in this repo talks to the sound system through
// SWINE's SIConcert (sound/iconcert.h). SMilesConcert implements that
// interface, and adds the Panzers-only methods through SIPanzersConcert
// below. Each SIConcert method maps onto the Panzers vtable slot named in its
// comment; methods with no Panzers counterpart are marked "SWINE-compat".

#ifndef SOUND_MILESCONCERT_H
#define SOUND_MILESCONCERT_H

#include <windows.h>
#include <mss.h>

#include <core_common.h>
#include <string2.h>
#include <darray.h>
#include <chain.h>
#include "iconcert.h"

// SDArray in Panzers HD has the layout { T* array; int size; int maxsize; }
// (see 0x684a00 / 0x5f7220 / 0x685730), which differs from SWINE's SDArray.
// This small copy keeps SMilesConcert's object layout equal to the original.
template<typename T>
struct SMilesDArray {
    T*  array;
    int size;
    int maxsize;

    int Add();                    // 0x684a00 / 0x5d8a60 pattern
    void Remove(int index);       // 0x5f7220 (SDArray<SString>::Remove)
    int Insert(int index);        // 0x685730 (SDArray<SString>::Insert)
    void Clear(int newsize);      // 0x684d30 / 0x563600
    void Free();                  // 0x6847d0 / 0x582d20 (destructor body)
    T& At(int index);             // bounds-checked operator[] (Panics)
};

// 3D provider list entry, 12 bytes (SDArray at SMilesConcert+0x0C).
struct SMilesProvider {
    SString   Name;   // +0x00
    HPROVIDER Id;     // +0x08
};

// Sound cache entry payload, 0x18 bytes (SHeap element 0x1C, +0x44).
struct SMilesCachedSound {
    SString FileName;   // +0x00  name as passed to PrecacheSound
    void*   Data;       // +0x08  file image (AIL_file_read / AIL_decompress_ASI)
    int     RefCount;   // +0x0C  live sounds spawned from this entry
    bool    Positional; // +0x10
    float   Size;       // +0x14  Data size in bytes, as float
};

// Playing sound payload, 0x30 bytes (SHeap element 0x34, +0x58).
struct SMilesSound {
    int       CacheIdx;     // +0x00
    H3DSAMPLE Sample3D;     // +0x04
    HSAMPLE   Sample;       // +0x08
    bool      Looped;       // +0x0C  persistent (CreateSound*); Update() never reclaims it
    float     MinDistance;  // +0x10
    float     Volume;       // +0x14  linear gain
    float     Position[3];  // +0x18
    int       SoundGroup;   // +0x24  OptimizeSoundGroup group, -1 = none
    bool      Active;       // +0x28
    bool      Paused;       // +0x29
    int       ChannelID;    // +0x2C  -1 = sound effect, else voice channel
};

// Panzers SConcert methods that SWINE's SIConcert does not have, in Panzers
// vtable order. Panzers code that is lifted later (menus, world) calls these.
struct SIPanzersConcert : public SIConcert {
    virtual HDIGDRIVER GetDigitalDriver() = 0;                                  // +0x18
    virtual void GetProviderNames(SDArray<SString>* names) = 0;                 // +0x1C
    virtual void SetProvider(const SString* name) = 0;                          // +0x20
    virtual int  CreateSoundEx(const char* name, bool sfx, float volume_db, float pan, bool loop) = 0;   // +0x2C
    virtual int  CreateSoundByIdEx(int cacheidx, bool sfx, float volume_db, float pan, bool loop) = 0;  // +0x30
    virtual int  CreateSound3DEx(const char* name, int group, float min_distance, const float* pos) = 0; // +0x34
    virtual int  CreateSound3DByIdEx(int cacheidx, int group, float min_distance, const float* pos) = 0; // +0x38
    virtual void SetSoundPositionV(int id, const float* pos) = 0;               // +0x40
    virtual void SetSoundVolume(int id, float volume, bool voice) = 0;          // +0x48
    virtual float PlaySound3DEx(const char* name, float min_distance, const float* pos) = 0;   // +0x58
    virtual float PlaySound3DByIdEx(int cacheidx, float min_distance, const float* pos) = 0;  // +0x5C
    virtual void PauseAllSounds() = 0;                                          // +0x60
    virtual void ResumeAllSounds() = 0;                                         // +0x64
    virtual void ClearPlaylist() = 0;                                           // +0x6C
    virtual void AddToPlaylist(const char* name) = 0;                           // +0x70
    virtual void ShufflePlaylist() = 0;                                         // +0x74
    virtual void StartPlaylist(bool loop) = 0;                                  // +0x78
    virtual void PlayNextTrack() = 0;                                           // +0x7C
    virtual void StopStream(bool fadeout) = 0;                                  // +0x80
    virtual bool IsStreamPlaying() = 0;                                         // +0x84
};

struct SMilesConcert : public SIPanzersConcert {
    // Object layout from the constructor 0x684300 (vptr at +0x00).
    unsigned int                       RefCount;          // +0x04
    HDIGDRIVER                         DigitalDriver;     // +0x08
    SMilesDArray<SMilesProvider>       Providers;         // +0x0C
    HPROVIDER                          Provider;          // +0x18
    HPROVIDER                          FallbackProvider;  // +0x1C "Miles Fast 2D Positional Audio"
    H3DPOBJECT                         Listener;          // +0x20
    HSTREAM                            Stream;            // +0x24
    float                              ListenerPosition[3]; // +0x28
    float                              MusicVolume;       // +0x34
    float                              Volume[2];         // +0x38 sfx, +0x3C voice
    float                              ReverseStereo;     // +0x40 1.0 or -1.0
    SHeap<SMilesCachedSound>           SoundCache;        // +0x44
    SHeap<SMilesSound>                 Sounds;            // +0x58
    int                                TransientSounds;   // +0x6C
    SMilesDArray<SString>              Playlist;          // +0x70
    bool                               LoopPlaylist;      // +0x7C

    SMilesConcert(HWND hwnd, const char* provider, int unused);
    ~SMilesConcert();

    // SIConcert (SWINE interface)
    void AddRef() override;                                                     // +0x00
    void Release() override;                                                    // +0x04
    void SetListener(float x, float y, float z, float fx, float fy, float fz,
                     float ux, float uy, float uz) override;                    // +0x08
    void Update(bool force) override;                                           // +0x0C
    void SetVolume(int group, int volume) override;                             // +0x10
    void SetReverseStereo(bool reverse) override;                               // +0x14
    int  PrecacheSound(const char* name, bool positional) override;             // +0x24
    void AddRefToCachedSound(int cacheidx) override;                            // SWINE-compat
    void ReleaseCachedSound(int cacheidx) override;                             // SWINE-compat
    int  CleanupSoundCache() override;                                          // +0x28
    int  CreateSound(const char* name, float volume_db, float pan) override;    // -> +0x2C
    int  CreateSoundById(int cacheidx, float volume_db, float pan) override;    // -> +0x30
    int  CreateSound3D(const char* name, int group, float min_distance, float x, float y, float z) override;   // -> +0x34
    int  CreateSound3DById(int cacheidx, int group, float min_distance, float x, float y, float z) override;  // -> +0x38
    void RemoveSound(int id) override;                                          // +0x3C
    void RemoveAllSounds() override;                                            // +0x68
    void SetSoundPosition(int id, float x, float y, float z) override;          // -> +0x40
    void SetSoundFrequency(int id, unsigned int rate) override;                 // +0x44
    void SetSoundMinDistance(int id, float min_distance) override;              // +0x4C
    float PlaySound(const char* name, float volume_db, float pan, int channel) override;   // +0x50
    float PlaySoundById(int cacheidx, float volume_db, float pan, int channel) override;   // +0x54
    float PlaySound3D(const char* name, float min_distance, float x, float y, float z) override;   // -> +0x58
    float PlaySound3DById(int cacheidx, float min_distance, float x, float y, float z) override;  // -> +0x5C
    void StartStreamingPlayback(const char* file1, const char* file2) override; // SWINE-compat -> playlist
    void StopStreamingPlayback() override;                                      // -> +0x80
    bool CheckStreamPlayback() override;                                        // -> +0x84
    void NextTrack() override;                                                  // SWINE-compat -> +0x7C
    void DumpChannels() override;                                               // SWINE-compat

    // SIPanzersConcert
    HDIGDRIVER GetDigitalDriver() override;
    void GetProviderNames(SDArray<SString>* names) override;
    void SetProvider(const SString* name) override;
    int  CreateSoundEx(const char* name, bool sfx, float volume_db, float pan, bool loop) override;
    int  CreateSoundByIdEx(int cacheidx, bool sfx, float volume_db, float pan, bool loop) override;
    int  CreateSound3DEx(const char* name, int group, float min_distance, const float* pos) override;
    int  CreateSound3DByIdEx(int cacheidx, int group, float min_distance, const float* pos) override;
    void SetSoundPositionV(int id, const float* pos) override;
    void SetSoundVolume(int id, float volume, bool voice) override;
    float PlaySound3DEx(const char* name, float min_distance, const float* pos) override;
    float PlaySound3DByIdEx(int cacheidx, float min_distance, const float* pos) override;
    void PauseAllSounds() override;
    void ResumeAllSounds() override;
    void ClearPlaylist() override;
    void AddToPlaylist(const char* name) override;
    void ShufflePlaylist() override;
    void StartPlaylist(bool loop) override;
    void PlayNextTrack() override;
    void StopStream(bool fadeout) override;
    bool IsStreamPlaying() override;

    // Internal (non-virtual in PANZERS.exe)
    void SetListenerV(const float* pos, const float* front, const float* up);   // 0x686890 (+0x08)
    int  SpawnSound(int cacheidx, bool positional);                             // 0x687300
    void OptimizeSoundGroup(int group);                                         // 0x685810
    SMilesSound&       SoundAt(int id);                                         // 0x6849b0
    SMilesCachedSound& CacheAt(int cacheidx);                                   // 0x684950
};

#if defined(_M_IX86)
static_assert(sizeof(SMilesConcert) == 0x80, "SMilesConcert must match PANZERS.exe (operator new(0x80) in 0x684fb0)");
static_assert(sizeof(SHeap<SMilesCachedSound>::Element) == 0x1C, "cache element 0x1C");
static_assert(sizeof(SHeap<SMilesSound>::Element) == 0x34, "sound element 0x34");
#endif

// The single live SMilesConcert (PANZERS.exe 0x92e798), or null.
extern SMilesConcert* g_MilesConcert;

// Factory 0x684fb0: new SMilesConcert + FPU control word logging/reset.
SIPanzersConcert* CreateMilesConcert(HWND hwnd, const char* provider, int unused);

#endif // SOUND_MILESCONCERT_H
