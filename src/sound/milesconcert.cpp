// sound/milesconcert.cpp
// SMilesConcert — Panzers HD audio engine on Miles Sound System 6.5c.
// Lifted from PANZERS.exe (HD, 2016); addresses are PANZERS.exe VAs.
//
// Built instead of sound/concert.cpp when GEPARD_AUDIO_BACKEND=miles (the
// default for Panzers). MP3 is decoded by Miles itself (mssmp3.asi from the
// redist directory "miles"): sound effects through AIL_decompress_ASI in
// PrecacheSound, music through AIL_open_stream. The SWINE mdec decoder and
// its MpegAudioPrecalculate tables are not used on this path.

#include "milesconcert.h"
#include <logger.h>
#include <stream.h>
#include <math.h>
#include <string.h>
#include <typeinfo>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>

// SIConcert's diagnostic hook (sound/iconcert.h). SWINE defines it in
// concert.cpp, which is not compiled on the Miles path.
SwineAudioPlayHook g_AudioPlayHook = nullptr;

SMilesConcert* g_MilesConcert = nullptr;   // PANZERS.exe 0x92e798

// Per-group voice quotas for OptimizeSoundGroup (PANZERS.exe 0x8de3cc = {6, 4},
// same values as SWINE's SoundGroupQuotas).
static const int MilesSoundGroupQuotas[2] = { 6, 4 };

static const char kEmpty[] = "";
static inline const char* SStr(const SString& s) { return s.buf ? s.buf : kEmpty; }

static void SStringFree(SString& s)
{
    if (s.buf) { delete[] s.buf; s.buf = nullptr; }
    s.size = 0;
}

// Volume in dB to linear gain, as in 0x685e00 / 0x6852b0:
// (float)sqrt(pow(10.0, vol/20.0f)).
static inline float DbToGain(float volume_db)
{
    return (float)sqrt(pow(10.0, (double)(volume_db / 20.0f)));
}

// rand() scaled into [lo, hi) the way PANZERS.exe does it:
// ((rand() * (range & 0xffff)) >> 15) + lo.
static inline int ScaledRand(int range)
{
    return (rand() * (range & 0xffff)) >> 15;
}

// ============================================================
// SMilesDArray (PANZERS.exe SDArray<T> layout)
// ============================================================

template<typename T>
int SMilesDArray<T>::Add()
{
    if (size == maxsize) {
        int newmax = (maxsize < 16) ? 16 : (maxsize * 6) / 5;
        array = (T*)realloc(array, newmax * sizeof(T));
        memset(&array[maxsize], 0, (newmax - maxsize) * sizeof(T));
        maxsize = newmax;
    }
    return size++;
}

template<typename T>
T& SMilesDArray<T>::At(int index)
{
    if (index < 0 || index >= size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", typeid(T).name(), index);
    return array[index];
}

template<>
void SMilesDArray<SString>::Remove(int index)
{
    if (index < 0 || index >= size)
        Logger.g->Panic("SDArray<%s>::Remove: invalid index (%d) size = %d", "SString", index, size);
    SStringFree(array[index]);
    --size;
    if (size - index != 0)
        memmove(&array[index], &array[index + 1], (size - index) * sizeof(SString));
    memset(&array[size], 0, sizeof(SString));
}

template<>
int SMilesDArray<SString>::Insert(int index)
{
    if (index < 0 || index > size)
        Logger.g->Panic("SDArray<%s>::Insert: invalid index (%d)", "SString", index);
    if (size == maxsize) {
        int newmax = (maxsize < 16) ? 16 : (maxsize * 6) / 5;
        array = (SString*)realloc(array, newmax * sizeof(SString));
        memset(&array[maxsize + 1], 0, (newmax - maxsize) * sizeof(SString) - sizeof(SString));
        maxsize = newmax;
    }
    if (index < size)
        memmove(&array[index + 1], &array[index], (size - index) * sizeof(SString));
    memset(&array[index], 0, sizeof(SString));
    ++size;
    return index;
}

template<typename T>
void SMilesDArray<T>::Clear(int newsize);

template<>
void SMilesDArray<SString>::Clear(int newsize)
{
    if (size != 0 && array == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "SString");
    for (int i = 0; i < size; i++)
        SStringFree(array[i]);
    size = newsize;
    if (maxsize < newsize) {
        maxsize = newsize;
        array = (SString*)realloc(array, newsize * sizeof(SString));
    }
    memset(array, 0, maxsize * sizeof(SString));
}

template<>
void SMilesDArray<SMilesProvider>::Clear(int newsize)
{
    if (size != 0 && array == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "SMilesProvider");
    for (int i = 0; i < size; i++)
        SStringFree(array[i].Name);
    size = newsize;
    if (maxsize < newsize) {
        maxsize = newsize;
        array = (SMilesProvider*)realloc(array, newsize * sizeof(SMilesProvider));
    }
    memset(array, 0, maxsize * sizeof(SMilesProvider));
}

template<>
void SMilesDArray<SString>::Free()
{
    for (int i = 0; i < size; i++)
        SStringFree(array[i]);
    if (array) { free(array); array = nullptr; }
    maxsize = 0;
    size = 0;
}

template<>
void SMilesDArray<SMilesProvider>::Free()
{
    for (int i = 0; i < size; i++)
        SStringFree(array[i].Name);
    if (array) { free(array); array = nullptr; }
    maxsize = 0;
    size = 0;
}

// ============================================================
// Miles file callbacks — route Miles file I/O through the engine
// file system, so music/sounds are found inside the .pak archives.
// ============================================================

// PANZERS 0x685540
static U32 AILCALLBACK MilesFileOpen(char const* filename, U32* file_handle)
{
    SStream* s = FileSystem.OpenRead(filename, nullptr);
    *file_handle = (U32)s;
    return (U32)s;
}

// PANZERS 0x685520
static void AILCALLBACK MilesFileClose(U32 file_handle)
{
    // Panzers tail-calls the stream's scalar deleting destructor (vtbl+0).
    // The SWINE SStream equivalent is Release().
    if (file_handle)
        ((SStream*)file_handle)->Release();
}

// PANZERS 0x685590
static S32 AILCALLBACK MilesFileSeek(U32 file_handle, S32 offset, U32 type)
{
    // Panzers SStream vtbl+0x10 is Seek(offset, origin); its result is
    // returned to Miles as the new position.
    if (file_handle)
        return ((SStream*)file_handle)->Seek(offset, (int)type);
    return 0;
}

// PANZERS 0x685560
static U32 AILCALLBACK MilesFileRead(U32 file_handle, void* buffer, U32 bytes)
{
    // Panzers SStream vtbl+0x4 is Read(buf, size), which throws on a short
    // read; the callback then reports `bytes` as read.
    if (file_handle) {
        ((SStream*)file_handle)->Read(buffer, (int)bytes);
        return bytes;
    }
    return 0;
}

// PANZERS 0x6875a0 — registered by PlayNextTrack when the playlist is not
// empty; plays the next playlist entry when the current stream ends.
static void AILCALLBACK MilesStreamCallback(HSTREAM)
{
    Logger.g->Log(0, "MILES StreamCallback()");
    g_MilesConcert->PlayNextTrack();
}

// ============================================================
// Construction / destruction
// ============================================================

// PANZERS 0x684300
SMilesConcert::SMilesConcert(HWND hwnd, const char* provider, int unused)
{
    (void)hwnd; (void)unused;   // passed by the factory, not read by 0x684300
    RefCount = 1;
    DigitalDriver = nullptr;
    Providers.array = nullptr; Providers.size = 0; Providers.maxsize = 0;
    Provider = 0;
    FallbackProvider = 0;
    Listener = nullptr;
    Stream = nullptr;
    ListenerPosition[0] = ListenerPosition[1] = ListenerPosition[2] = 0.0f;
    SoundCache.array = nullptr; SoundCache.size = 0; SoundCache.maxsize = 0;
    SoundCache.nextempty = -1; SoundCache.occupied = 0;
    Sounds.array = nullptr; Sounds.size = 0; Sounds.maxsize = 0;
    Sounds.nextempty = -1; Sounds.occupied = 0;
    Playlist.array = nullptr; Playlist.size = 0; Playlist.maxsize = 0;

    if (g_MilesConcert)
        Logger.g->Panic("SMilesConcert::SMilesConcert(): Concert already exists");
    g_MilesConcert = this;

    char version[32];
    HMODULE mss = LoadLibraryA("MSS32.DLL");
    if ((UINT_PTR)mss <= 0x20) {
        version[0] = 0;
    } else {
        LoadStringA(mss, 1, version, sizeof(version));
        FreeLibrary(mss);
    }
    Logger.g->Log(0, "SMilesConcert::SMilesConcert(): Miles Sound System version %s", version);

    // FileNameProcess("miles") 0x65f020: a relative name is resolved against
    // the home path, an absolute one is copied.
    AIL_set_redist_directory(FileSystem.MakeFullPath("miles"));
    // (0x684300 also calls 0x42a7c0 here, an atexit registration for a
    // function-local static; it has no effect on the sound system.)
    AIL_startup();

    DigitalDriver = AIL_open_digital_driver(44100, 16, 2, 0);
    if (!DigitalDriver)
        Logger.g->Panic("SMilesConcert::SMilesConcert(): Digital driver initialization failed: %s.", AIL_last_error());

    Logger.g->Log(1, "  Mixer Channels = %d", AIL_get_preference(DIG_MIXER_CHANNELS));
    Logger.g->Log(1, "  Output Buffer Size = %d", AIL_get_preference(DIG_OUTPUT_BUFFER_SIZE));
    Logger.g->Log(1, "SMilesConcert::SMilesConcert(): Enumerating 3D sound providers:");

    Providers.Clear(0);
    HPROENUM next = HPROENUM_FIRST;
    HPROVIDER id;
    char* name;
    while (AIL_enumerate_3D_providers(&next, &id, &name)) {
        int idx = Providers.Add();
        SMilesProvider& p = Providers.At(idx);
        p.Name = name;
        p.Id = id;
        Logger.g->Log(1, "  0x%02X: %s", id, name);
        if (strcmp(name, provider) == 0)
            Provider = id;
        if (strcmp(name, "Miles Fast 2D Positional Audio") == 0)
            FallbackProvider = id;
    }

    if (AIL_open_3D_provider(Provider) != 0) {
        Logger.g->Log(0, "SMilesConcert::SMilesConcert(): 3D sound provider (%s) initialization failed: %s.",
                      provider, AIL_last_error());
        Provider = FallbackProvider;
        if (AIL_open_3D_provider(FallbackProvider) != 0)
            Logger.g->Panic("SMilesConcert::SMilesConcert(): 3D sound provider (Miles Fast 2D Positional Audio) initialization failed: %s.",
                            AIL_last_error());
    }

    AIL_set_file_callbacks(MilesFileOpen, MilesFileClose, MilesFileSeek, MilesFileRead);

    Listener = AIL_open_3D_listener(Provider);
    if (!Listener)
        Logger.g->Panic("SMilesConcert::SMilesConcert(): Listener initialization failed.");

    MusicVolume = 1.0f;
    if (Stream)
        AIL_set_stream_volume_pan(Stream, 1.0f, 0.5f);
    SMilesConcert::SetVolume(1, 10);
    SMilesConcert::SetVolume(2, 10);
    ReverseStereo = 1.0f;
    TransientSounds = 0;
    Logger.g->Log(1, "SMilesConcert::SMilesConcert() finished succesfully.");
}

// PANZERS 0x6848b0
SMilesConcert::~SMilesConcert()
{
    if (Listener)
        AIL_close_3D_listener(Listener);
    AIL_close_3D_provider(Provider);
    AIL_shutdown();
    if (RefCount != 0)
        Logger.g->Panic("SMilesConcert::~SMilesConcert(): Deleted instead of release.");
    g_MilesConcert = nullptr;
    Playlist.Free();
    if (Sounds.array)
        free(Sounds.array);
    // ~SHeap<SMilesCachedSound> 0x684840: frees used entries' names, then the array.
    for (int i = 0; i < SoundCache.size; i++) {
        if (SoundCache.array[i].use == 0x7FFFFFFF)
            SStringFree(SoundCache.array[i].data.FileName);
    }
    if (SoundCache.array)
        free(SoundCache.array);
    Providers.Free();
}

// PANZERS 0x684fb0
SIPanzersConcert* CreateMilesConcert(HWND hwnd, const char* provider, int unused)
{
    SMilesConcert* c = new SMilesConcert(hwnd, provider, unused);
    unsigned short cw;
    __asm fnstcw cw;
    Logger.g->Log(0, "FPU control word (after new SMilesConcert): 0x%04X", cw);
    // fldcw 0x8de158 (= 0x007F: all exceptions masked, 24-bit precision,
    // round to nearest) — undoes whatever the Miles startup left behind.
    static const unsigned short kPanzersFpuCW = 0x007F;
    __asm fldcw kPanzersFpuCW;
    __asm fnstcw cw;
    Logger.g->Log(0, "FPU control word (after fldcw): 0x%04X", cw);
    return c;
}

// SWINE factory name, called by window/dxwindow.cpp. Panzers passes the
// options.ini "Miles provider" string; until the options layer is lifted
// this uses the shipped default.
SIConcert* __cdecl CreateConcert(HWND hwnd)
{
    return CreateMilesConcert(hwnd, "Miles Fast 2D Positional Audio", 0);
}

// ============================================================
// Reference counting
// ============================================================

// PANZERS 0x684c00
void SMilesConcert::AddRef()
{
    ++RefCount;
}

// PANZERS 0x686570
void SMilesConcert::Release()
{
    if (--RefCount == 0)
        delete this;
}

// ============================================================
// Heap accessors
// ============================================================

// PANZERS 0x6849b0
SMilesSound& SMilesConcert::SoundAt(int id)
{
    if (id < 0 || id >= Sounds.size || Sounds.array[id].use != 0x7FFFFFFF)
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SSoundProp", id);
    return Sounds.array[id].data;
}

// PANZERS 0x684950
SMilesCachedSound& SMilesConcert::CacheAt(int cacheidx)
{
    if (cacheidx < 0 || cacheidx >= SoundCache.size || SoundCache.array[cacheidx].use != 0x7FFFFFFF)
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SSoundCacheProp", cacheidx);
    return SoundCache.array[cacheidx].data;
}

// ============================================================
// Listener / global settings
// ============================================================

// PANZERS 0x686890
void SMilesConcert::SetListenerV(const float* pos, const float* front, const float* up)
{
    if (!Listener)
        return;
    ListenerPosition[0] = pos[0];
    ListenerPosition[1] = pos[1];
    ListenerPosition[2] = pos[2];
    AIL_set_3D_position(Listener, ReverseStereo * pos[0], pos[1], pos[2]);
    AIL_set_3D_orientation(Listener, front[0] * ReverseStereo, front[1], front[2],
                           up[0] * ReverseStereo, up[1], up[2]);
    OptimizeSoundGroup(0);
    OptimizeSoundGroup(1);
}

void SMilesConcert::SetListener(float x, float y, float z, float fx, float fy, float fz,
                                float ux, float uy, float uz)
{
    const float pos[3] = { x, y, z }, front[3] = { fx, fy, fz }, up[3] = { ux, uy, uz };
    SetListenerV(pos, front, up);
}

// PANZERS 0x686e80
void SMilesConcert::SetVolume(int group, int volume)
{
    if (group == 0) {
        MusicVolume = (float)volume * 0.1f;
        if (Stream)
            AIL_set_stream_volume_pan(Stream, MusicVolume, 0.5f);
        return;
    }
    if (group == 1) {
        Volume[0] = (float)volume * 0.1f;
        for (int i = Sounds.GetNext(-1); i >= 0; i = Sounds.GetNext(i)) {
            SMilesSound& s = SoundAt(i);
            if (s.Sample3D && (!s.Looped || s.Active))
                AIL_set_3D_sample_volume(s.Sample3D, Volume[0]);
            if (s.Sample && s.ChannelID == -1) {
                F32 vol, pan;
                AIL_sample_volume_pan(s.Sample, &vol, &pan);
                AIL_set_sample_volume_pan(s.Sample, s.Volume * Volume[0], pan);
            }
        }
    } else if (group == 2) {
        Volume[1] = (float)volume * 0.1f;
        for (int i = Sounds.GetNext(-1); i >= 0; i = Sounds.GetNext(i)) {
            SMilesSound& s = SoundAt(i);
            if (s.Sample && s.ChannelID != -1) {
                F32 vol, pan;
                AIL_sample_volume_pan(s.Sample, &vol, &pan);
                AIL_set_sample_volume_pan(s.Sample, s.Volume * Volume[1], pan);
            }
        }
    }
}

// PANZERS 0x686a80
void SMilesConcert::SetReverseStereo(bool reverse)
{
    ReverseStereo = reverse ? -1.0f : 1.0f;
    for (int i = Sounds.GetNext(-1); i >= 0; i = Sounds.GetNext(i))
        SetSoundPositionV(i, SoundAt(i).Position);
}

// PANZERS 0x6855b0
HDIGDRIVER SMilesConcert::GetDigitalDriver()
{
    return DigitalDriver;
}

// PANZERS 0x6855c0
void SMilesConcert::GetProviderNames(SDArray<SString>* names)
{
    for (int i = 0; i < names->size; i++)
        SStringFree(names->array[i]);
    names->size = 0;
    for (int i = 0; i < Providers.size; i++) {
        int idx = names->Add();
        names->array[idx].buf = nullptr;
        names->array[idx].size = 0;
        names->array[idx] = Providers.At(i).Name;
    }
}

// PANZERS 0x686960
void SMilesConcert::SetProvider(const SString* name)
{
    if (Listener)
        AIL_close_3D_listener(Listener);
    AIL_close_3D_provider(Provider);
    for (int i = 0; i < Providers.size; i++) {
        SMilesProvider& p = Providers.At(i);
        if (p.Name.size == name->size && (p.Name.size == 0 || _stricmp(p.Name.buf, name->buf) == 0)) {
            Provider = p.Id;
            break;
        }
    }
    if (AIL_open_3D_provider(Provider) != 0) {
        // The original pushes the SString by value here (buf, size), so the
        // second %s receives the length; the name and the error are logged instead.
        Logger.g->Log(0, "SMilesConcert::SMilesConcert(): 3D sound provider (%s) initialization failed: %s.",
                      SStr(*name), AIL_last_error());
        Provider = FallbackProvider;
        if (AIL_open_3D_provider(FallbackProvider) != 0)
            Logger.g->Panic("SMilesConcert::SMilesConcert(): 3D sound provider (Miles Fast 2D Positional Audio) initialization failed: %s.",
                            AIL_last_error());
    }
    Listener = AIL_open_3D_listener(Provider);
    if (!Listener)
        Logger.g->Panic("SMilesConcert::SMilesConcert(): Listener initialization failed.");
}

// ============================================================
// Sound cache
// ============================================================

// PANZERS 0x686220
int SMilesConcert::PrecacheSound(const char* name, bool positional)
{
    if (!DigitalDriver || !name || !*name)
        return -1;

    for (int i = SoundCache.GetNext(-1); i >= 0; i = SoundCache.GetNext(i)) {
        SMilesCachedSound& c = SoundCache.array[i].data;
        if (strcmp(name, SStr(c.FileName)) == 0 && CacheAt(i).Positional == positional)
            return i;
    }

    SString path;
    struct _stat st;
    if (FileSystem.Stat(name, &st) == 0)
        path = name;
    else
        SString::Format(&path, "sounds/%s", name);

    int result = -1;
    void* data = nullptr;
    U32 size = 0;
    if (strstr(SStr(path), ".mp3")) {
        void* mp3 = AIL_file_read(SStr(path), nullptr);
        if (!mp3) {
            Logger.g->Log(0, "SMilesConcert::PrecacheSound(): Can't load \"%s\".", SStr(path));
            goto done;
        }
        U32 mp3size = (U32)AIL_file_size(SStr(path));
        if (!AIL_decompress_ASI(mp3, mp3size, ".mp3", &data, &size, nullptr)) {
            Logger.g->Log(0, "SMilesConcert::PrecacheSound(): Can't load \"%s\".", SStr(path));
            AIL_mem_free_lock(mp3);
            goto done;
        }
        AIL_mem_free_lock(mp3);
    } else if (strstr(SStr(path), ".wav")) {
        data = AIL_file_read(SStr(path), nullptr);
        if (!data) {
            Logger.g->Log(0, "SMilesConcert::PrecacheSound(): Load failed: \"%s\".", SStr(path));
            goto done;
        }
        size = (U32)AIL_file_size(SStr(path));
    } else {
        // The format string has no %s in the original either.
        Logger.g->Warning("SMilesConcert::PrecacheSound(): Unknown file type: \"\".", SStr(path));
        goto done;
    }

    {
        result = SoundCache.Add();
        SMilesCachedSound& c = CacheAt(result);
        c.FileName = name;
        c.RefCount = 0;
        c.Positional = positional;
        c.Data = data;
        c.Size = (float)(unsigned int)size;
    }
done:
    SStringFree(path);
    return result;
}

// SWINE-compat: SWINE pins cache entries explicitly. Panzers counts live
// sounds in RefCount (SpawnSound / RemoveSound) and has no such call.
void SMilesConcert::AddRefToCachedSound(int cacheidx)
{
    if (cacheidx >= 0)
        ++CacheAt(cacheidx).RefCount;
}

// SWINE-compat, see AddRefToCachedSound.
void SMilesConcert::ReleaseCachedSound(int cacheidx)
{
    if (cacheidx >= 0)
        --CacheAt(cacheidx).RefCount;
}

// PANZERS 0x684c80
int SMilesConcert::CleanupSoundCache()
{
    int kept = 0;
    for (int i = SoundCache.GetNext(-1); i >= 0; i = SoundCache.GetNext(i)) {
        SMilesCachedSound& c = CacheAt(i);
        if (c.RefCount == 0) {
            AIL_mem_free_lock(c.Data);
            // SHeap<SSoundCacheProp>::Remove 0x686590 also frees the name.
            SStringFree(c.FileName);
            SoundCache.Remove(i);
        } else {
            ++kept;
        }
    }
    return kept;
}

// ============================================================
// Sound instances
// ============================================================

// PANZERS 0x687300
int SMilesConcert::SpawnSound(int cacheidx, bool positional)
{
    if (!Provider || cacheidx < 0 || cacheidx >= SoundCache.size ||
        SoundCache.array[cacheidx].use != 0x7FFFFFFF)
        return -1;
    if (CacheAt(cacheidx).Positional != positional)
        return -1;

    if (!positional) {
        HSAMPLE s = AIL_allocate_sample_handle(DigitalDriver);
        if (!s)
            return -1;
        int id = Sounds.Add();
        ++CacheAt(cacheidx).RefCount;
        SoundAt(id).CacheIdx = cacheidx;
        SoundAt(id).Sample = s;
        SoundAt(id).Sample3D = nullptr;
        AIL_init_sample(s);
        if (AIL_set_sample_file(s, CacheAt(cacheidx).Data, 0))
            return id;
        Logger.g->Log(0, "SMilesConcert::SpawnSound(): Can't load '%s': %s", SStr(CacheAt(cacheidx).FileName), AIL_last_error());
        Sounds.Remove(id);   // as in the original: the handle and the RefCount are not released
        return -1;
    }

    H3DSAMPLE s = AIL_allocate_3D_sample_handle(Provider);
    if (!s)
        return -1;
    int id = Sounds.Add();
    ++CacheAt(cacheidx).RefCount;
    SoundAt(id).CacheIdx = cacheidx;
    SoundAt(id).Sample = nullptr;
    SoundAt(id).Sample3D = s;
    if (!AIL_set_3D_sample_file(s, CacheAt(cacheidx).Data)) {
        Logger.g->Log(0, "SMilesConcert::SpawnSound(): Can't load '%s': %s", SStr(CacheAt(cacheidx).FileName), AIL_last_error());
        Sounds.Remove(id);
        return -1;
    }
    return id;
}

// PANZERS 0x6852b0
int SMilesConcert::CreateSoundByIdEx(int cacheidx, bool sfx, float volume_db, float pan, bool loop)
{
    int id = SpawnSound(cacheidx, false);
    if (id < 0)
        return id;
    SMilesSound& s = SoundAt(id);
    s.ChannelID = -1;
    s.SoundGroup = -1;
    s.Active = false;
    s.Paused = false;
    s.Looped = loop;
    s.Volume = DbToGain(volume_db);
    AIL_set_sample_volume_pan(s.Sample, Volume[sfx ? 0 : 1] * s.Volume, pan * 0.5f + 0.5f);
    AIL_set_sample_loop_count(s.Sample, !s.Looped);
    AIL_start_sample(s.Sample);
    return id;
}

// PANZERS 0x6854e0
int SMilesConcert::CreateSoundEx(const char* name, bool sfx, float volume_db, float pan, bool loop)
{
    return CreateSoundByIdEx(PrecacheSound(name, false), sfx, volume_db, pan, loop);
}

// PANZERS 0x685070
int SMilesConcert::CreateSound3DByIdEx(int cacheidx, int group, float min_distance, const float* pos)
{
    int id = SpawnSound(cacheidx, true);
    if (id < 0)
        return id;
    SMilesSound& s = SoundAt(id);
    s.SoundGroup = group;
    s.Active = false;
    s.Paused = false;
    s.Looped = true;
    s.Position[0] = pos[0];
    s.Position[1] = pos[1];
    s.Position[2] = pos[2];
    s.MinDistance = min_distance;
    SetSoundPositionV(id, pos);
    SetSoundMinDistance(id, min_distance);
    AIL_set_3D_sample_volume(SoundAt(id).Sample3D, Volume[0]);
    if (SoundAt(id).Looped) {
        U32 len = AIL_3D_sample_length(SoundAt(id).Sample3D);
        AIL_set_3D_sample_loop_block(SoundAt(id).Sample3D, 8000, (S32)len - 8000);
    }
    AIL_set_3D_sample_loop_count(SoundAt(id).Sample3D, !SoundAt(id).Looped);
    if (SoundAt(id).Looped) {
        U32 len = AIL_3D_sample_length(SoundAt(id).Sample3D);
        U32 offset = (U32)ScaledRand((int)len) & 0xFFFFFFF0u;
        AIL_set_3D_sample_offset(SoundAt(id).Sample3D, offset);
    }
    // Not started here: OptimizeSoundGroup (from SetListener) starts the
    // nearest sounds of each group.
    return id;
}

// PANZERS 0x6854b0
int SMilesConcert::CreateSound3DEx(const char* name, int group, float min_distance, const float* pos)
{
    return CreateSound3DByIdEx(PrecacheSound(name, true), group, min_distance, pos);
}

// SWINE CreateSound(name, volume, panning) -> Panzers +0x2C as a looped sound effect.
int SMilesConcert::CreateSound(const char* name, float volume_db, float pan)
{
    return CreateSoundEx(name, true, volume_db, pan, true);
}

int SMilesConcert::CreateSoundById(int cacheidx, float volume_db, float pan)
{
    return CreateSoundByIdEx(cacheidx, true, volume_db, pan, true);
}

int SMilesConcert::CreateSound3D(const char* name, int group, float min_distance, float x, float y, float z)
{
    const float pos[3] = { x, y, z };
    return CreateSound3DEx(name, group, min_distance, pos);
}

int SMilesConcert::CreateSound3DById(int cacheidx, int group, float min_distance, float x, float y, float z)
{
    const float pos[3] = { x, y, z };
    return CreateSound3DByIdEx(cacheidx, group, min_distance, pos);
}

// PANZERS 0x686670
void SMilesConcert::RemoveSound(int id)
{
    if (id < 0 || id >= Sounds.size || Sounds.array[id].use != 0x7FFFFFFF)
        return;
    SMilesSound& s = SoundAt(id);
    --CacheAt(s.CacheIdx).RefCount;
    if (s.Sample3D)
        AIL_release_3D_sample_handle(s.Sample3D);
    if (s.Sample)
        AIL_release_sample_handle(s.Sample);
    Sounds.Remove(id);
}

// PANZERS 0x686740
void SMilesConcert::RemoveAllSounds()
{
    for (int i = Sounds.GetNext(-1); i >= 0; i = Sounds.GetNext(i))
        RemoveSound(i);
}

// PANZERS 0x686c80
void SMilesConcert::SetSoundPositionV(int id, const float* pos)
{
    if (id < 0 || id >= Sounds.size || Sounds.array[id].use != 0x7FFFFFFF)
        return;
    SMilesSound& s = SoundAt(id);
    s.Position[0] = pos[0];
    s.Position[1] = pos[1];
    s.Position[2] = pos[2];
    if (s.Sample3D)
        AIL_set_3D_position(s.Sample3D, ReverseStereo * pos[0], pos[1], pos[2]);
}

void SMilesConcert::SetSoundPosition(int id, float x, float y, float z)
{
    const float pos[3] = { x, y, z };
    SetSoundPositionV(id, pos);
}

// PANZERS 0x686b20
void SMilesConcert::SetSoundFrequency(int id, unsigned int rate)
{
    if (id < 0 || id >= Sounds.size || Sounds.array[id].use != 0x7FFFFFFF)
        return;
    SMilesSound& s = SoundAt(id);
    if (s.Sample3D)
        AIL_set_3D_sample_playback_rate(s.Sample3D, (S32)rate);
    if (s.Sample)
        AIL_set_sample_playback_rate(s.Sample, (S32)rate);
}

// PANZERS 0x686bd0
void SMilesConcert::SetSoundMinDistance(int id, float min_distance)
{
    if (id < 0 || id >= Sounds.size || Sounds.array[id].use != 0x7FFFFFFF)
        return;
    SMilesSound& s = SoundAt(id);
    s.MinDistance = min_distance;
    if (s.Sample3D)
        AIL_set_3D_sample_distances(s.Sample3D, min_distance, min_distance * 100.0f);
}

// PANZERS 0x686d60
void SMilesConcert::SetSoundVolume(int id, float volume, bool voice)
{
    if (id < 0 || id >= Sounds.size || Sounds.array[id].use != 0x7FFFFFFF)
        return;
    SMilesSound& s = SoundAt(id);
    s.Volume = volume;
    if (s.Sample3D)
        AIL_set_3D_sample_volume(s.Sample3D, Volume[voice ? 1 : 0] * volume);
    if (s.Sample)
        AIL_set_sample_volume_pan(s.Sample, Volume[voice ? 1 : 0] * volume, 0.5f);
}

// PANZERS 0x685e00
float SMilesConcert::PlaySoundById(int cacheidx, float volume_db, float pan, int channel)
{
    if (TransientSounds >= 16) {
        Update(false);
        if (TransientSounds >= 16)
            return 0.0f;
    }
    int id = SpawnSound(cacheidx, false);
    if (id < 0)
        return 0.0f;

    if (channel >= 0) {
        // One sound per voice channel. The scan also sees the sound spawned
        // just above (ChannelID 0 from the heap memset), so channel 0 never
        // plays; as in the original. Update() reclaims the unstarted sample.
        for (int i = Sounds.GetNext(-1); i >= 0; i = Sounds.GetNext(i)) {
            if (Sounds.array[i].data.Looped)
                continue;
            if (SoundAt(i).Sample == nullptr)
                continue;
            if (SoundAt(i).ChannelID == channel)
                return 0.0f;
        }
    }

    SMilesSound& s = SoundAt(id);
    s.ChannelID = channel;
    s.SoundGroup = -1;
    s.Active = true;
    s.Paused = false;
    s.Looped = false;
    s.Volume = DbToGain(volume_db);
    AIL_set_sample_volume_pan(s.Sample, Volume[channel != -1 ? 1 : 0] * s.Volume, pan * 0.5f + 0.5f);
    AIL_start_sample(s.Sample);
    ++TransientSounds;

    S32 rate = AIL_sample_playback_rate(SoundAt(id).Sample);
    S32 gran = AIL_sample_granularity(SoundAt(id).Sample);
    unsigned int bytes_per_sec = (unsigned int)(rate * gran);
    return CacheAt(SoundAt(id).CacheIdx).Size / (float)(double)bytes_per_sec;
}

// PANZERS 0x6861e0
float SMilesConcert::PlaySound(const char* name, float volume_db, float pan, int channel)
{
    if (g_AudioPlayHook)
        g_AudioPlayHook(name, "play");
    return PlaySoundById(PrecacheSound(name, false), volume_db, pan, channel);
}

// PANZERS 0x685cd0
float SMilesConcert::PlaySound3DByIdEx(int cacheidx, float min_distance, const float* pos)
{
    if (TransientSounds > 15) {
        Update(false);
        if (TransientSounds > 15)
            return 0.0f;
    }
    int id = SpawnSound(cacheidx, true);
    if (id < 0)
        return 0.0f;
    SMilesSound& s = SoundAt(id);
    s.ChannelID = -1;
    s.SoundGroup = -1;
    s.Active = true;
    s.Paused = false;
    s.Looped = false;
    s.Position[0] = pos[0];
    s.Position[1] = pos[1];
    s.Position[2] = pos[2];
    s.MinDistance = min_distance;
    SetSoundPositionV(id, pos);
    SetSoundMinDistance(id, min_distance);
    AIL_set_3D_sample_volume(SoundAt(id).Sample3D, Volume[0]);
    AIL_start_3D_sample(SoundAt(id).Sample3D);
    ++TransientSounds;
    return 1.0f;
}

// PANZERS 0x6861b0
float SMilesConcert::PlaySound3DEx(const char* name, float min_distance, const float* pos)
{
    return PlaySound3DByIdEx(PrecacheSound(name, true), min_distance, pos);
}

float SMilesConcert::PlaySound3D(const char* name, float min_distance, float x, float y, float z)
{
    const float pos[3] = { x, y, z };
    return PlaySound3DEx(name, min_distance, pos);
}

float SMilesConcert::PlaySound3DById(int cacheidx, float min_distance, float x, float y, float z)
{
    const float pos[3] = { x, y, z };
    return PlaySound3DByIdEx(cacheidx, min_distance, pos);
}

// PANZERS 0x6875d0
void SMilesConcert::Update(bool force)
{
    for (int i = Sounds.GetNext(-1); i >= 0; i = Sounds.GetNext(i)) {
        SMilesSound& s = Sounds.array[i].data;
        if (s.Looped)
            continue;
        if (!force) {
            bool finished = false;
            if (!s.Paused && s.Sample3D && AIL_3D_sample_status(SoundAt(i).Sample3D) != SMP_PLAYING)
                finished = true;
            if (!finished) {
                if (s.Paused || !s.Sample)
                    continue;
                if (AIL_sample_status(SoundAt(i).Sample) == SMP_PLAYING)
                    continue;
            }
        }
        RemoveSound(i);
        --TransientSounds;
    }
}

// PANZERS 0x685810
void SMilesConcert::OptimizeSoundGroup(int group)
{
    struct Candidate { int idx; float priority; };
    Candidate list[128];   // 0x41c-byte frame in the original; no overflow guard there either
    int quota = MilesSoundGroupQuotas[group];
    int n = 0;

    for (int i = Sounds.GetNext(-1); i >= 0; i = Sounds.GetNext(i)) {
        SMilesSound& s = Sounds.array[i].data;
        if (!s.Looped || !s.Sample3D || s.SoundGroup != group)
            continue;
        list[n].idx = i;
        float dx = ListenerPosition[0] - s.Position[0];
        float dy = ListenerPosition[1] - s.Position[1];
        float dz = ListenerPosition[2] - s.Position[2];
        double dist = sqrt((double)(dx * dx + dy * dy + dz * dz));
        list[n].priority = s.MinDistance / (float)dist;
        ++n;
    }

    if (quota < n) {
        // Partial selection sort: move the `quota` highest priorities to the
        // front. `best` is not reset between passes, as in the original.
        int best = 0;
        for (int k = 0; k < quota; k++) {
            float bestp = -1.0f;
            for (int j = k; j < n; j++) {
                if (bestp < list[j].priority) {
                    best = j;
                    bestp = list[j].priority;
                }
            }
            Candidate tmp = list[k];
            list[k] = list[best];
            list[best] = tmp;
        }
    }

    for (int i = n - 1; i >= 0; i--) {
        SMilesSound& s = SoundAt(list[i].idx);
        if (i >= quota) {
            if (s.Active) {
                AIL_stop_3D_sample(s.Sample3D);
                s.Active = false;
            }
        } else if (!s.Active) {
            s.Active = true;
            if (!s.Paused)
                AIL_start_3D_sample(s.Sample3D);
        }
    }
}

// PANZERS 0x685bd0
void SMilesConcert::PauseAllSounds()
{
    for (int i = Sounds.GetNext(-1); i >= 0; i = Sounds.GetNext(i)) {
        SMilesSound& s = SoundAt(i);
        s.Paused = true;
        if (!s.Active)
            continue;
        if (s.Sample)
            AIL_stop_sample(s.Sample);
        if (s.Sample3D)
            AIL_stop_3D_sample(s.Sample3D);
    }
}

// PANZERS 0x686780
void SMilesConcert::ResumeAllSounds()
{
    for (int i = Sounds.GetNext(-1); i >= 0; i = Sounds.GetNext(i)) {
        SMilesSound& s = SoundAt(i);
        if (s.Active && s.Paused) {
            if (s.Sample)
                AIL_resume_sample(s.Sample);
            if (s.Sample3D)
                AIL_resume_3D_sample(s.Sample3D);
        }
        s.Paused = false;
    }
}

// SWINE-compat: SWINE logs every channel; Panzers has no DumpChannels.
void SMilesConcert::DumpChannels()
{
    Logger.g->Log(0, "SMilesConcert::DumpChannels: Channels %d", Sounds.occupied);
}

// ============================================================
// Music playlist / stream
// ============================================================

// PANZERS 0x684de0
void SMilesConcert::ClearPlaylist()
{
    Playlist.Clear(0);
}

// PANZERS 0x684c10
void SMilesConcert::AddToPlaylist(const char* name)
{
    int idx = Playlist.Add();
    Playlist.At(idx) = name;
}

// PANZERS 0x687150
void SMilesConcert::ShufflePlaylist()
{
    int n = Playlist.size;
    for (int i = 0; i < n; i++) {
        int j = ScaledRand(n - i) + i;
        SString tmp;
        tmp = Playlist.At(j);
        Playlist.Remove(j);
        Playlist.Insert(i);
        Playlist.At(i) = tmp;
        SStringFree(tmp);
    }
}

// PANZERS 0x6874f0
void SMilesConcert::StartPlaylist(bool loop)
{
    if (!DigitalDriver)
        Logger.g->Panic("SMilesConcert::StartStreamingPlayback(): Miles_DigitalDriver is NULL.");
    if (Playlist.size <= 0)
        Logger.g->Panic("SMilesConcert::StartStreamingPlayback(): Playlist is empty.");
    if (Stream)
        AIL_close_stream(Stream);
    LoopPlaylist = loop;
    PlayNextTrack();
}

// PANZERS 0x684df0
void SMilesConcert::PlayNextTrack()
{
    SString name;
    name = Playlist.At(0);
    Playlist.Remove(0);
    if (LoopPlaylist) {
        // Rotate: the track goes back to the end of the playlist.
        int idx = Playlist.Add();
        Playlist.At(idx) = name;
    }
    Stream = AIL_open_stream(DigitalDriver, SStr(name), 0);
    Logger.g->Log(0, "SMilesConcert::StartStreamingPlayback(): Playing '%s'...", SStr(name));
    if (!Stream) {
        Logger.g->Log(0, "SMilesConcert::StartStreamingPlayback(): Stream initialization failed.");
    } else {
        AIL_set_stream_loop_count(Stream, 1);
        AIL_set_stream_volume_pan(Stream, MusicVolume, 0.5f);
        AIL_start_stream(Stream);
        if (Playlist.size != 0)
            AIL_register_stream_callback(Stream, MilesStreamCallback);
    }
    SStringFree(name);
}

// PANZERS 0x687550
void SMilesConcert::StopStream(bool fadeout)
{
    if (DigitalDriver && Stream) {
        if (fadeout)
            Logger.g->Log(0, "SMilesConcert::StopStreamingPlayback(): Fadeout not supported.");
        AIL_close_stream(Stream);
        Stream = nullptr;
    }
}

// PANZERS 0x684c70
bool SMilesConcert::IsStreamPlaying()
{
    return Stream != nullptr;
}

// SWINE-compat: SWINE streams file1 then loops file2. Mapped onto the
// Panzers playlist (looping).
void SMilesConcert::StartStreamingPlayback(const char* file1, const char* file2)
{
    ClearPlaylist();
    AddToPlaylist(file1);
    if (file2 && *file2)
        AddToPlaylist(file2);
    StartPlaylist(true);
}

void SMilesConcert::StopStreamingPlayback()
{
    StopStream(false);
}

bool SMilesConcert::CheckStreamPlayback()
{
    return IsStreamPlaying();
}

// SWINE-compat: SWINE's NextTrack is the stream-end hook of its decoder.
void SMilesConcert::NextTrack()
{
    if (Playlist.size > 0)
        PlayNextTrack();
}
