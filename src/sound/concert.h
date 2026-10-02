// sound/concert.h
// SConcert, SWave — Audio engine
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef SOUND_CONCERT_H
#define SOUND_CONCERT_H

#include <windows.h>
#include <dsound.h>
#include <comdef.h>
#include <math.h>

#include <core_common.h>
#include <string2.h>
#include <chain.h>
#include <stream.h>
#include "iconcert.h"

// MP3 decoder types (includes SMpegAudioCallBack definition)
#include <decode.h>

// Sound property — per-playing-sound data (56 bytes, element = 60)
// Field order from PDB (common_types.h SSoundProp)
struct SSoundProp {
    int CacheIdx;
    IDirectSoundBuffer *Buffer;
    IDirectSound3DBuffer *Buffer3d;
    bool Looped;
    float MinDistance;
    int Volume;
    int Panning;
    unsigned int PlaybackPosition;
    struct { float x, y, z; } Position;
    int SoundGroup;
    bool Enabled;
    bool HasBeenDucked;
    int ChannelID;
};

// Sound cache property — per-cached-sound data (24 bytes, element = 28)
// Field order from PDB (common_types.h SSoundCacheProp)
struct SSoundCacheProp {
    char *FileName;
    IDirectSoundBuffer *Buffer;
    int RefCount;
    bool Positional;
    float Duration;
    unsigned int Size;
};

// WAV file header
struct SWaveHeader {
    char riff[4];
    int riffsize;
    char wavefmt[8];
    int wsize;
    short wFormatTag;
    short nChannels;
    int nSamplesPerSec;
    int nAvgBytesPerSec;
    short nBlockAlign;
    short wBitsPerSample;
};

// WAV data header
struct SWaveDataHeader {
    char data[4];
    int dsize;
};

// SWave — WAV/MP3 file loader
struct SWave : public SMpegAudioCallBack {
    WAVEFORMATEX WaveFMT;
    unsigned int Size;
    unsigned char *Data;
    SStreamBuffer *DecodeBuffer;

    SWave();
    ~SWave();
    void DataCallback(short *left, short *right, int num_samples, int stereo, int freq);
    char LoadMP3(const char *filename, const char *panicstr);
    char LoadWAV(char *filename, char *panicstr);
};

// SConcert — Main audio engine, implements SIConcert
// Field order must match original binary layout (from IDA common_types.h:38296)
struct SConcert : public SIConcert, public SMpegAudioCallBack {
    unsigned int RefCount;
    HWND hWnd;
    GUID DefaultDeviceGuid;
    SString DeviceDescription;
    SString DeviceModule;
    bool TerratecHack;
    IDirectSound8 *DSound;
    IDirectSound3DListener *Listener;
    SHeap<SSoundCacheProp> SoundCache;
    SHeap<SSoundProp> Sounds;
    struct { float x, y, z; } ListenerPosition;
    SString StreamFile1;
    SString StreamFile2;
    IDirectSoundBuffer *StreamBuffer;
    CRITICAL_SECTION StreamBufferCriticalSection;
    unsigned int WriteCursor;
    bool StreamStarted;
    int Volume[3];
    float MinDistanceMultiplier;
    float ReverseStereo;
    int TransientSounds;

    // Constructor / Destructor
    SConcert(HWND hWnd);
    ~SConcert();

    // SIConcert interface implementation
    void AddRef() override;
    void Release() override;
    void SetListener(float, float, float, float, float, float, float, float, float) override;
    void Update(bool) override;
    void SetVolume(int, int) override;
    void SetReverseStereo(bool) override;
    int PrecacheSound(const char *, bool) override;
    void AddRefToCachedSound(int) override;
    void ReleaseCachedSound(int) override;
    int CleanupSoundCache() override;
    int CreateSound(const char *, float, float) override;
    int CreateSoundById(int, float, float) override;
    int CreateSound3D(const char *, int, float, float, float, float) override;
    int CreateSound3DById(int, int, float, float, float, float) override;
    void RemoveSound(int) override;
    void RemoveAllSounds() override;
    void SetSoundPosition(int, float, float, float) override;
    void SetSoundFrequency(int, unsigned int) override;
    void SetSoundMinDistance(int, float) override;
    float PlaySound(const char *, float, float, int) override;
    float PlaySoundById(int, float, float, int) override;
    float PlaySound3D(const char *, float, float, float, float) override;
    float PlaySound3DById(int, float, float, float, float) override;
    void StartStreamingPlayback(const char *, const char *) override;
    void StopStreamingPlayback() override;
    bool CheckStreamPlayback() override;
    void NextTrack() override;
    void DumpChannels() override;

    // Internal methods
    void CreateStreamBuffer();
    void DataCallback(short *left, short *right, int num_samples, int stereo, int freq);
    void HandleDirectSoundDeviceChange();
    void InitDirectSound();
    char LoadSoundInternal(int i);
    void OptimizeSoundGroup(int sound_group);
    double PlaySound(int cacheidx, float volume, float panning, int channelid);
    double PlaySound(int cacheidx, float min_distance, float x, float y, float z);
    void ReclaimPlayingSounds();
    void ReclaimSoundCacheBuffers();
    void ReleaseSoundCacheBuffers();
    void ReleasePlayingSounds();
    int SpawnSound(int cacheidx, bool positional);
    int CreateSound(int cacheidx, int sound_group, float min_distance, float x, float y, float z);
    int CreateSound(int cacheidx, float volume, float panning);
};

#if defined(_M_IX86)
static_assert(sizeof(SConcert) == 0xBC, "SConcert size must match original binary (0xBC bytes)");
#endif

// Global data
extern int VolumeTable[];
extern int SoundGroupQuotas[];
extern int StreamThreadRunning;
extern int StreamThreadShutdown;
extern int StreamThreadDeviceChange;
extern short outsamples_0[][2];
extern short word_5B573A[];
extern short outsamples[][2];
extern short word_5B4E3A[];

// Thread procedure
extern "C" DWORD WINAPI StreamThreadProc(LPVOID param);

// DirectSound enumeration callback
extern "C" BOOL CALLBACK DSEnumCallback(LPGUID lpGuid, LPCSTR lpcstrDescription, LPCSTR lpcstrModule, LPVOID lpContext);

// DXGetErrorStringA (from DX SDK dxerr)
extern "C" const char* DXGetErrorStringA(HRESULT hr);

// DirectSound device ID helper
extern "C" HRESULT WINAPI GetDeviceID(LPCGUID pGuidSrc, LPGUID pGuidDest);

#endif // SOUND_CONCERT_H
