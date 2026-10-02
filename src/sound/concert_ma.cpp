// sound/concert_ma.cpp
// Portable audio backend — implements SIConcert via miniaudio.
//
// Selected by GEPARD_AUDIO_BACKEND=miniaudio. Replaces the DirectSound +
// in-tree mdec/ MP3 decoder path on x64 / non-Windows / WASM. The original
// SConcert (concert.cpp) stays as the x86 Windows pixel-parity oracle.
//
// Mapping summary (see GOALS.md Phase 2.5):
//   SIConcert::PrecacheSound        → ma_sound_init_from_file(MA_SOUND_FLAG_DECODE)
//   SIConcert::PlaySound{,3D}       → ma_sound_init_copy + ma_sound_start
//   SIConcert::CreateSound{,3D}     → same, but kept alive in the voice table
//   SIConcert::StartStreamingPlayback → 2× MA_SOUND_FLAG_STREAM, manual A↔B handoff
//   SIConcert::SetVolume(0|1|2,n)   → 3 ma_sound_groups (music / sfx / speech)
//   SIConcert::SetListener          → ma_engine_listener_set_position+direction+world_up
//   3D positional                   → ma_sound_set_position + min/max distance

#include <new>
#include <string>
#include <deque>
#include <cstring>
#include <cstdio>
#include <cmath>

#include <core_common.h>
#include <stream.h>
#include <logger.h>
#include "iconcert.h"
#include "../third_party/miniaudio/miniaudio.h"

// HUD diagnostic hook (HD_HDBEEFUP_AUDIO_DEBUG). Defined here, registered
// by SGameView when the toggle is enabled.
SwineAudioPlayHook g_AudioPlayHook = nullptr;

// ---------------------------------------------------------------------------
// SFileSystem ↔ miniaudio VFS bridge
// ---------------------------------------------------------------------------
// Game assets live inside .PAK archives, accessed transparently by the
// engine's SFileSystem search-path mechanism (SArchiveStream + SFileStream
// behind a virtual SStream interface). miniaudio's default VFS uses the OS
// directly and therefore can't find anything — every PrecacheSound /
// StartStreamingPlayback call would fail with "Resource does not exist".
//
// This VFS forwards every open/read/seek call to FileSystem.OpenRead(), so
// MP3 + WAV decoding sees the same byte stream the original DSound path saw.
namespace {

struct SwineVFS {
    ma_vfs_callbacks cb;   // MUST be first — miniaudio casts ma_vfs* to ma_vfs_callbacks*
};

static ma_result swine_vfs_open(ma_vfs * /*pVFS*/, const char *path,
                                ma_uint32 openMode, ma_vfs_file *outFile)
{
    if (openMode != MA_OPEN_MODE_READ) return MA_INVALID_OPERATION;
    if (!path || !*path) return MA_INVALID_ARGS;
    SStream *s = FileSystem.OpenRead(path, nullptr);
    if (!s) return MA_DOES_NOT_EXIST;
    *outFile = (ma_vfs_file)s;
    return MA_SUCCESS;
}

static ma_result swine_vfs_close(ma_vfs *, ma_vfs_file file)
{
    SStream *s = (SStream *)file;
    if (s) s->Release();
    return MA_SUCCESS;
}

static ma_result swine_vfs_read(ma_vfs *, ma_vfs_file file,
                                void *dst, size_t bytes, size_t *bytesRead)
{
    SStream *s = (SStream *)file;
    int got = s->ReadMax(dst, (int)bytes);
    if (got < 0) got = 0;
    if (bytesRead) *bytesRead = (size_t)got;
    if (got == 0) return MA_AT_END;
    return MA_SUCCESS;
}

static ma_result swine_vfs_seek(ma_vfs *, ma_vfs_file file,
                                ma_int64 offset, ma_seek_origin origin)
{
    SStream *s = (SStream *)file;
    int o = (origin == ma_seek_origin_current) ? 1
          : (origin == ma_seek_origin_end)     ? 2
          : 0;
    s->Seek((int)offset, o);
    return MA_SUCCESS;
}

static ma_result swine_vfs_tell(ma_vfs *, ma_vfs_file file, ma_int64 *cursor)
{
    SStream *s = (SStream *)file;
    *cursor = (ma_int64)s->Seek(0, 1);  // SEEK_CUR
    return MA_SUCCESS;
}

static ma_result swine_vfs_info(ma_vfs *, ma_vfs_file file, ma_file_info *info)
{
    SStream *s = (SStream *)file;
    int cur  = s->Seek(0, 1);  // save current position
    int size = s->Seek(0, 2);  // seek to end → byte length
    s->Seek(cur, 0);           // restore
    info->sizeInBytes = (ma_uint64)(size > 0 ? size : 0);
    return MA_SUCCESS;
}

static SwineVFS g_swineVFS;
static bool     g_swineVFS_initialized = false;

static ma_vfs *get_swine_vfs() {
    if (!g_swineVFS_initialized) {
        g_swineVFS.cb.onOpen  = swine_vfs_open;
        g_swineVFS.cb.onOpenW = nullptr;   // engine uses ANSI paths only
        g_swineVFS.cb.onClose = swine_vfs_close;
        g_swineVFS.cb.onRead  = swine_vfs_read;
        g_swineVFS.cb.onWrite = nullptr;   // read-only VFS
        g_swineVFS.cb.onSeek  = swine_vfs_seek;
        g_swineVFS.cb.onTell  = swine_vfs_tell;
        g_swineVFS.cb.onInfo  = swine_vfs_info;
        g_swineVFS_initialized = true;
    }
    return (ma_vfs *)&g_swineVFS;
}

} // namespace

// ---------------------------------------------------------------------------
// VolumeTable — millibels (DSound units). Index 0..10 = -10000..0 mB.
// Same table the DSound backend uses; duplicated here so the two TUs don't
// have to share state.
// ---------------------------------------------------------------------------
static const int VolumeTable[11] = {
    -10000, -4500, -4000, -3500, -3000, -2500, -2000, -1500, -1000, -500, 0
};

static inline float MillibelsToLinear(int mB) {
    if (mB <= -10000) return 0.0f;
    return powf(10.0f, mB / 2000.0f);
}

// ---------------------------------------------------------------------------
// Internal voice / cache tables
// ---------------------------------------------------------------------------
namespace {

struct CachedSound {
    bool         inUse = false;
    bool         sourceInited = false;
    bool         positional = false;
    uint32_t     refcount = 0;
    float        duration = 0.0f;
    std::string  filename;
    ma_sound     source{};
};

struct Voice {
    bool   inUse = false;
    bool   voiceInited = false;
    bool   transient = false;     // one-shot via PlaySound() — auto-freed in Update()
    bool   positional = false;
    int    cacheidx = -1;
    int    channelID = -1;
    float  minDistance = 1.0f;
    ma_sound voice{};
};

constexpr int TRANSIENT_VOICE_CAP = 16;  // matches concert.cpp:1717,1858

} // namespace

// ---------------------------------------------------------------------------
// SConcertMA
// ---------------------------------------------------------------------------
struct SConcertMA : public SIConcert {
    uint32_t        RefCount = 1;
    bool            engineInited = false;
    ma_engine       engine{};
    ma_sound_group  groupMusic{};
    ma_sound_group  groupSfx{};
    ma_sound_group  groupSpeech{};
    bool            groupsInited[3] = { false, false, false };

    int             Volume[3] = { 5, 5, 5 };   // index into VolumeTable
    float           reverseStereoSign = 1.0f;

    // std::deque (not std::vector) — miniaudio's node graph stores pointers
    // INTO the ma_sound structs, so element addresses MUST stay stable for
    // the lifetime of the sound. std::vector::emplace_back reallocates and
    // moves elements on growth; std::deque does not invalidate references.
    std::deque<CachedSound> cache;
    std::deque<Voice>       voices;
    int                      transientCount = 0;

    // Streaming
    SString  streamFile1;
    SString  streamFile2;
    ma_sound streamA{};
    ma_sound streamB{};
    bool     streamAInit = false;
    bool     streamBInit = false;
    int      streamCurrent = -1;   // 0 = A, 1 = B, -1 = none

    SConcertMA();
    ~SConcertMA();

    // SIConcert interface
    void  AddRef() override;
    void  Release() override;
    void  SetListener(float, float, float, float, float, float, float, float, float) override;
    void  Update(bool) override;
    void  SetVolume(int, int) override;
    void  SetReverseStereo(bool) override;
    int   PrecacheSound(const char *, bool) override;
    void  AddRefToCachedSound(int) override;
    void  ReleaseCachedSound(int) override;
    int   CleanupSoundCache() override;
    int   CreateSound(const char *, float, float) override;
    int   CreateSoundById(int, float, float) override;
    int   CreateSound3D(const char *, int, float, float, float, float) override;
    int   CreateSound3DById(int, int, float, float, float, float) override;
    void  RemoveSound(int) override;
    void  RemoveAllSounds() override;
    void  SetSoundPosition(int, float, float, float) override;
    void  SetSoundFrequency(int, unsigned int) override;
    void  SetSoundMinDistance(int, float) override;
    float PlaySound(const char *, float, float, int) override;
    float PlaySoundById(int, float, float, int) override;
    float PlaySound3D(const char *, float, float, float, float) override;
    float PlaySound3DById(int, float, float, float, float) override;
    void  StartStreamingPlayback(const char *, const char *) override;
    void  StopStreamingPlayback() override;
    bool  CheckStreamPlayback() override;
    void  NextTrack() override;
    void  DumpChannels() override;

    // Internals
    int   findOrAllocCacheSlot(const char *filename, bool positional);
    int   allocVoiceSlot();
    void  freeVoiceSlot(int idx);
    bool  initSourceFromFile(CachedSound &cs);
    ma_sound_group *groupForChannel(int channelID, bool positional);
    void  applyGroupVolume(int groupIdx);
    void  startStreamSide(int side, const char *filename);
    void  uninitStreamSide(int side);
};

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------
SConcertMA::SConcertMA() {
    ma_engine_config cfg = ma_engine_config_init();
    cfg.listenerCount = 1;
    cfg.pResourceManagerVFS = get_swine_vfs();   // route through SFileSystem (.PAK-aware)
    if (ma_engine_init(&cfg, &engine) != MA_SUCCESS) {
        Logger.g->Log(0, "SConcertMA: ma_engine_init failed; audio disabled");
        return;
    }
    engineInited = true;

    if (ma_sound_group_init(&engine, MA_SOUND_FLAG_NO_SPATIALIZATION, NULL, &groupMusic) == MA_SUCCESS)
        groupsInited[0] = true;
    if (ma_sound_group_init(&engine, 0, NULL, &groupSfx) == MA_SUCCESS)
        groupsInited[1] = true;
    if (ma_sound_group_init(&engine, MA_SOUND_FLAG_NO_SPATIALIZATION, NULL, &groupSpeech) == MA_SUCCESS)
        groupsInited[2] = true;

    for (int i = 0; i < 3; ++i) applyGroupVolume(i);

    Logger.g->Log(0, "SConcertMA: miniaudio %s initialized (%uHz %uch)",
                  ma_version_string(),
                  ma_engine_get_sample_rate(&engine),
                  ma_engine_get_channels(&engine));
}

SConcertMA::~SConcertMA() {
    StopStreamingPlayback();
    RemoveAllSounds();
    CleanupSoundCache();
    if (groupsInited[2]) ma_sound_group_uninit(&groupSpeech);
    if (groupsInited[1]) ma_sound_group_uninit(&groupSfx);
    if (groupsInited[0]) ma_sound_group_uninit(&groupMusic);
    if (engineInited)    ma_engine_uninit(&engine);
}

void SConcertMA::AddRef()  { ++RefCount; }
void SConcertMA::Release() { if (--RefCount == 0) delete this; }

// ---------------------------------------------------------------------------
// Volume routing
// ---------------------------------------------------------------------------
void SConcertMA::applyGroupVolume(int groupIdx) {
    if (!groupsInited[groupIdx]) return;
    float v = MillibelsToLinear(VolumeTable[Volume[groupIdx]]);
    ma_sound_group *g = (groupIdx == 0) ? &groupMusic
                       : (groupIdx == 1) ? &groupSfx
                                         : &groupSpeech;
    ma_sound_group_set_volume(g, v);
}

void SConcertMA::SetVolume(int volume_type, int value) {
    if (volume_type < 0 || volume_type > 2) return;
    if (value < 0) value = 0;
    if (value > 10) value = 10;
    Volume[volume_type] = value;
    applyGroupVolume(volume_type);
}

void SConcertMA::SetReverseStereo(bool reverse) {
    reverseStereoSign = reverse ? -1.0f : 1.0f;
    // Pan reversal is applied at voice-spawn time; existing voices are left
    // alone (the original DSound impl behaves the same — only new sounds
    // pick up the new sign).
}

// ---------------------------------------------------------------------------
// Listener
// ---------------------------------------------------------------------------
void SConcertMA::SetListener(float x, float y, float z,
                             float fx, float fy, float fz,
                             float ux, float uy, float uz) {
    if (!engineInited) return;
    ma_engine_listener_set_position (&engine, 0, x,  y,  z);
    ma_engine_listener_set_direction(&engine, 0, fx, fy, fz);
    ma_engine_listener_set_world_up (&engine, 0, ux, uy, uz);
}

// ---------------------------------------------------------------------------
// Cache management
// ---------------------------------------------------------------------------
bool SConcertMA::initSourceFromFile(CachedSound &cs) {
    // Path is relative to the game data root and resolved by our VFS, which
    // delegates to FileSystem.OpenRead() and therefore searches .PAK archives
    // exactly like the original DSound LoadSoundInternal did. No MakeFullPath
    // — that only works for files that physically exist on disk.
    char relpath[512];
    snprintf(relpath, sizeof(relpath), "sounds/%s", cs.filename.c_str());

    // MA_SOUND_FLAG_DECODE = decode at load; matches DSound's static-buffer
    // semantics (no streaming for short SFX). Spatialization is enabled at
    // the voice level per call, not the source.
    ma_uint32 flags = MA_SOUND_FLAG_DECODE;
    ma_result r = ma_sound_init_from_file(&engine, relpath, flags, NULL, NULL, &cs.source);
    if (r != MA_SUCCESS) {
        Logger.g->Log(0, "SConcertMA: ma_sound_init_from_file('%s') failed: %s",
                      relpath, ma_result_description(r));
        return false;
    }
    cs.sourceInited = true;
    float length = 0.0f;
    ma_result lr = ma_sound_get_length_in_seconds(&cs.source, &length);
    if (lr != MA_SUCCESS || length <= 0.0f) {
        // Length unknown — happens with some streamed/VBR MP3s where the
        // backing data source can't report duration. Fall back to a small
        // positive value so callers (notably TestTriggers' WaitTime2 math at
        // gameworld.cpp:16117 — `if duration <= 0 goto LABEL_835`) don't
        // treat the call as a failure and re-fire the trigger every frame.
        length = 0.5f;
        Logger.g->Log(0, "SConcertMA: '%s' length unknown (%s), defaulting to %.2fs",
                      relpath, ma_result_description(lr), length);
    }
    cs.duration = length;
    return true;
}

int SConcertMA::findOrAllocCacheSlot(const char *filename, bool positional) {
    if (!engineInited || !filename || !*filename) return -1;
    // Existing entry? same filename + same positional flag → reuse.
    for (size_t i = 0; i < cache.size(); ++i) {
        if (cache[i].inUse && cache[i].filename == filename &&
            cache[i].positional == positional) {
            return (int)i;
        }
    }
    // Free slot or grow.
    int idx = -1;
    for (size_t i = 0; i < cache.size(); ++i) {
        if (!cache[i].inUse) { idx = (int)i; break; }
    }
    if (idx < 0) {
        cache.emplace_back();
        idx = (int)cache.size() - 1;
    }
    CachedSound &cs = cache[idx];
    cs = CachedSound{};
    cs.inUse = true;
    cs.filename = filename;
    cs.positional = positional;
    cs.refcount = 0;

    if (!initSourceFromFile(cs)) {
        cs.inUse = false;
        return -1;
    }
    return idx;
}

int SConcertMA::PrecacheSound(const char *filename, bool positional) {
    int idx = findOrAllocCacheSlot(filename, positional);
    if (idx >= 0) ++cache[idx].refcount;
    return idx;
}

void SConcertMA::AddRefToCachedSound(int idx) {
    if (idx < 0 || idx >= (int)cache.size() || !cache[idx].inUse) return;
    ++cache[idx].refcount;
}

void SConcertMA::ReleaseCachedSound(int idx) {
    if (idx < 0 || idx >= (int)cache.size() || !cache[idx].inUse) return;
    if (cache[idx].refcount > 0) --cache[idx].refcount;
}

int SConcertMA::CleanupSoundCache() {
    int freed = 0;
    for (auto &cs : cache) {
        if (cs.inUse && cs.refcount == 0) {
            if (cs.sourceInited) ma_sound_uninit(&cs.source);
            cs = CachedSound{};
            ++freed;
        }
    }
    return freed;
}

// ---------------------------------------------------------------------------
// Voice allocation + playback
// ---------------------------------------------------------------------------
ma_sound_group *SConcertMA::groupForChannel(int channelID, bool positional) {
    if (positional) return groupsInited[1] ? &groupSfx : NULL;
    // Non-positional 2D: channelID == -1 → SFX/UI bucket; otherwise speech voice.
    if (channelID != -1 && groupsInited[2]) return &groupSpeech;
    return groupsInited[1] ? &groupSfx : NULL;
}

int SConcertMA::allocVoiceSlot() {
    for (size_t i = 0; i < voices.size(); ++i)
        if (!voices[i].inUse) return (int)i;
    voices.emplace_back();
    return (int)voices.size() - 1;
}

void SConcertMA::freeVoiceSlot(int idx) {
    if (idx < 0 || idx >= (int)voices.size()) return;
    Voice &v = voices[idx];
    if (v.voiceInited) ma_sound_uninit(&v.voice);
    if (v.transient && transientCount > 0) --transientCount;
    v = Voice{};
}

// Internal: build a fresh voice from a cached source.
// Returns voice index, or -1 on failure.
static int spawn_voice(SConcertMA *self, int cacheidx, bool positional, bool looped,
                       int channelID, float minDistance, bool transient) {
    if (cacheidx < 0 || cacheidx >= (int)self->cache.size()) return -1;
    CachedSound &cs = self->cache[cacheidx];
    if (!cs.inUse || !cs.sourceInited) return -1;

    if (transient && self->transientCount >= TRANSIENT_VOICE_CAP) return -1;

    int vidx = self->allocVoiceSlot();
    Voice &v = self->voices[vidx];
    v = Voice{};
    v.inUse = true;
    v.cacheidx = cacheidx;
    v.transient = transient;
    v.positional = positional;
    v.channelID = channelID;
    v.minDistance = minDistance;

    ma_sound_group *grp = self->groupForChannel(channelID, positional);
    ma_uint32 flags = positional ? 0u : MA_SOUND_FLAG_NO_SPATIALIZATION;
    ma_result r = ma_sound_init_copy(&self->engine, &cs.source, flags, grp, &v.voice);
    if (r != MA_SUCCESS) {
        v = Voice{};
        return -1;
    }
    v.voiceInited = true;

    ma_sound_set_looping(&v.voice, looped ? MA_TRUE : MA_FALSE);
    if (positional) {
        ma_sound_set_min_distance(&v.voice, minDistance);
    }

    if (transient) ++self->transientCount;
    return vidx;
}

// ---------------------------------------------------------------------------
// CreateSound — looped/long-lived voices the game tracks by handle
// ---------------------------------------------------------------------------
int SConcertMA::CreateSound(const char *filename, float volume, float panning) {
    int cidx = PrecacheSound(filename, false);
    return CreateSoundById(cidx, volume, panning);
}

int SConcertMA::CreateSoundById(int cacheidx, float volume, float panning) {
    // CreateSound voices are LOOPED and start playing immediately. Original
    // SConcert::CreateSound (concert.cpp:505) sets Looped=1 and the buffer
    // gets started looping inside OptimizeSoundGroup (concert.cpp:1702 with
    // DSBPLAY_LOOPING). Engine sounds, ambient sounds, and any other long-
    // lived audio relies on this — without start+loop they're silent.
    int v = spawn_voice(this, cacheidx, /*positional*/false, /*looped*/true,
                        /*channelID*/-1, /*minDistance*/1.0f, /*transient*/false);
    if (v < 0) return -1;
    Voice &voice = voices[v];
    // Match DirectSound: VolumeTable[Volume[group]] + 1000 + (volume * 100).
    // The VolumeTable[Volume[group]] term is supplied by the SFX/Speech group
    // (applyGroupVolume); group * per-sound multiplies linearly = adds in mB.
    ma_sound_set_volume(&voice.voice, MillibelsToLinear(1000 + (int)(volume * 100.0f)));
    ma_sound_set_pan   (&voice.voice, panning * reverseStereoSign);
    ma_sound_start(&voice.voice);
    return v;
}

int SConcertMA::CreateSound3D(const char *filename, int /*sound_group*/,
                              float min_distance, float x, float y, float z) {
    int cidx = PrecacheSound(filename, true);
    return CreateSound3DById(cidx, 0, min_distance, x, y, z);
}

int SConcertMA::CreateSound3DById(int cacheidx, int /*sound_group*/,
                                  float min_distance, float x, float y, float z) {
    int v = spawn_voice(this, cacheidx, /*positional*/true, /*looped*/true,
                        /*channelID*/-1, min_distance, /*transient*/false);
    if (v < 0) return -1;
    Voice &voice = voices[v];
    ma_sound_set_position(&voice.voice, x, y, z);
    ma_sound_start(&voice.voice);
    return v;
}

void SConcertMA::RemoveSound(int idx) {
    if (idx < 0 || idx >= (int)voices.size()) return;
    if (!voices[idx].inUse) return;
    Voice &v = voices[idx];
    if (v.voiceInited) ma_sound_stop(&v.voice);
    freeVoiceSlot(idx);
}

void SConcertMA::RemoveAllSounds() {
    for (int i = 0; i < (int)voices.size(); ++i) {
        if (voices[i].inUse) RemoveSound(i);
    }
}

void SConcertMA::SetSoundPosition(int idx, float x, float y, float z) {
    if (idx < 0 || idx >= (int)voices.size() || !voices[idx].inUse) return;
    if (voices[idx].voiceInited)
        ma_sound_set_position(&voices[idx].voice, x, y, z);
}

void SConcertMA::SetSoundFrequency(int idx, unsigned int frequency) {
    if (idx < 0 || idx >= (int)voices.size() || !voices[idx].inUse) return;
    if (!voices[idx].voiceInited) return;
    // Engine passes raw Hz; convert to a pitch ratio against the source rate.
    ma_uint32 srcRate = 0;
    ma_sound_get_data_format(&voices[idx].voice, NULL, NULL, &srcRate, NULL, 0);
    if (srcRate == 0) return;
    ma_sound_set_pitch(&voices[idx].voice, (float)frequency / (float)srcRate);
}

void SConcertMA::SetSoundMinDistance(int idx, float min_distance) {
    if (idx < 0 || idx >= (int)voices.size() || !voices[idx].inUse) return;
    voices[idx].minDistance = min_distance;
    if (voices[idx].voiceInited && voices[idx].positional)
        ma_sound_set_min_distance(&voices[idx].voice, min_distance);
}

// ---------------------------------------------------------------------------
// PlaySound — fire-and-forget transient one-shots; return duration in seconds
// ---------------------------------------------------------------------------
float SConcertMA::PlaySound(const char *filename, float volume, float panning, int channelid) {
    int cidx = PrecacheSound(filename, false);
    float dur = PlaySoundById(cidx, volume, panning, channelid);
    if (g_AudioPlayHook && filename) {
        // Embed the returned duration in the HUD line so trigger-loop bugs
        // (where PlaySound returns 0 and TestTriggers' WaitTime2 set is
        // skipped via `if duration <= 0 goto LABEL_835`) are obvious without
        // having to dig into the log.
        char hudline[128];
        snprintf(hudline, sizeof(hudline), "play %.2fs ch=%d", dur, channelid);
        g_AudioPlayHook(filename, cidx >= 0 ? hudline : "fail");
    }
    return dur;
}

float SConcertMA::PlaySoundById(int cacheidx, float volume, float panning, int channelid) {
    if (cacheidx < 0 || cacheidx >= (int)cache.size() || !cache[cacheidx].inUse)
        return 0.0f;
    // ChannelID dedup — original SConcert::PlaySound (concert.cpp:1750-1768)
    // refuses to spawn a second non-looped, non-3D voice on the same channel.
    // This is what stops speech from looping every frame: the game polls
    // "play random commander speech" continually, and the dedup keeps only
    // one instance alive at a time. channelid == -1 means "no channel"
    // (UI / fire-and-forget) and is never deduped.
    if (channelid >= 0) {
        for (auto &existing : voices) {
            if (existing.inUse && existing.transient && !existing.positional &&
                existing.channelID == channelid) {
                if (g_AudioPlayHook) {
                    g_AudioPlayHook(cache[cacheidx].filename.c_str(), "dup");
                }
                return 0.0f;
            }
        }
    }
    int v = spawn_voice(this, cacheidx, /*positional*/false, /*looped*/false,
                        channelid, /*minDistance*/1.0f, /*transient*/true);
    if (v < 0) return 0.0f;
    Voice &voice = voices[v];
    // Match DirectSound: VolumeTable[Volume[group]] + 1000 + (volume * 100).
    // Without the +1000 boost and the *100 scale, credits commentary plays
    // at the same level as music instead of ~10 dB hotter (concert.cpp:1840).
    ma_sound_set_volume(&voice.voice, MillibelsToLinear(1000 + (int)(volume * 100.0f)));
    ma_sound_set_pan   (&voice.voice, panning * reverseStereoSign);
    voice.channelID = channelid;
    ma_sound_start(&voice.voice);
    return cache[cacheidx].duration;
}

float SConcertMA::PlaySound3D(const char *filename, float min_distance, float x, float y, float z) {
    int cidx = PrecacheSound(filename, true);
    if (g_AudioPlayHook && filename) {
        g_AudioPlayHook(filename, cidx >= 0 ? "play3d" : "fail3d");
    }
    return PlaySound3DById(cidx, min_distance, x, y, z);
}

float SConcertMA::PlaySound3DById(int cacheidx, float min_distance, float x, float y, float z) {
    if (cacheidx < 0 || cacheidx >= (int)cache.size() || !cache[cacheidx].inUse)
        return 0.0f;
    int v = spawn_voice(this, cacheidx, /*positional*/true, /*looped*/false,
                        /*channelID*/-1, min_distance, /*transient*/true);
    if (v < 0) return 0.0f;
    Voice &voice = voices[v];
    ma_sound_set_position(&voice.voice, x, y, z);
    ma_sound_start(&voice.voice);
    return cache[cacheidx].duration;
}

// ---------------------------------------------------------------------------
// Streaming music — A/B ping-pong, MP3 or any format miniaudio decodes
// ---------------------------------------------------------------------------
void SConcertMA::startStreamSide(int side, const char *filename) {
    if (!engineInited || !filename || !*filename) return;
    ma_sound *snd = (side == 0) ? &streamA : &streamB;
    bool *inited = (side == 0) ? &streamAInit : &streamBInit;
    if (*inited) { ma_sound_uninit(snd); *inited = false; }

    // Streaming sources also go through the VFS — music tracks live inside
    // PAK archives just like SFX. The caller passes paths like "music/foo.mp3"
    // already (no "sounds/" prefix), so we forward verbatim.
    ma_uint32 flags = MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION;
    ma_sound_group *grp = groupsInited[0] ? &groupMusic : NULL;
    if (ma_sound_init_from_file(&engine, filename, flags, grp, NULL, snd) != MA_SUCCESS) {
        Logger.g->Log(0, "SConcertMA: stream init failed: %s", filename);
        return;
    }
    *inited = true;
    ma_sound_set_looping(snd, MA_FALSE);   // Update() handles A→B handoff
    ma_sound_start(snd);
    streamCurrent = side;
}

void SConcertMA::uninitStreamSide(int side) {
    ma_sound *snd = (side == 0) ? &streamA : &streamB;
    bool *inited = (side == 0) ? &streamAInit : &streamBInit;
    if (*inited) {
        ma_sound_stop(snd);
        ma_sound_uninit(snd);
        *inited = false;
    }
}

void SConcertMA::StartStreamingPlayback(const char *filename1, const char *filename2) {
    StopStreamingPlayback();
    streamFile1 = filename1 ? filename1 : "";
    streamFile2 = filename2 ? filename2 : "";
    if (filename1 && *filename1) startStreamSide(0, filename1);
}

void SConcertMA::StopStreamingPlayback() {
    uninitStreamSide(0);
    uninitStreamSide(1);
    streamCurrent = -1;
}

bool SConcertMA::CheckStreamPlayback() {
    if (streamCurrent == 0 && streamAInit) return ma_sound_is_playing(&streamA) != 0;
    if (streamCurrent == 1 && streamBInit) return ma_sound_is_playing(&streamB) != 0;
    return false;
}

void SConcertMA::NextTrack() {
    // Force-advance to the other side's file; matches the original "skip
    // current track" semantic.
    const char *next = (streamCurrent == 0)
        ? (streamFile2.buf ? streamFile2.buf : (const char *)"")
        : (streamFile1.buf ? streamFile1.buf : (const char *)"");
    int nextSide = (streamCurrent == 0) ? 1 : 0;
    uninitStreamSide(streamCurrent);
    if (next && *next) startStreamSide(nextSide, next);
}

// ---------------------------------------------------------------------------
// Update — reap finished transients and pump streaming A↔B handoff
// ---------------------------------------------------------------------------
void SConcertMA::Update(bool shutdown) {
    // Reap finished one-shot voices. Under shutdown=true (e.g. ~SVideoView's
    // call when Skip is pressed on the intro) every transient is killed
    // regardless of play state — matches original SConcert::Update at
    // concert.cpp:2878-2893. Without this, intro audio leaks into the menu.
    for (int i = 0; i < (int)voices.size(); ++i) {
        Voice &v = voices[i];
        if (!v.inUse || !v.transient || !v.voiceInited) continue;
        if (shutdown || ma_sound_at_end(&v.voice)) freeVoiceSlot(i);
    }

    // Streaming A↔B ping-pong.
    if (streamCurrent == 0 && streamAInit && ma_sound_at_end(&streamA)) {
        const char *f2 = streamFile2.buf ? streamFile2.buf : (const char *)"";
        if (*f2) startStreamSide(1, f2);
        else { ma_sound_seek_to_pcm_frame(&streamA, 0); ma_sound_start(&streamA); }
    } else if (streamCurrent == 1 && streamBInit && ma_sound_at_end(&streamB)) {
        const char *f1 = streamFile1.buf ? streamFile1.buf : (const char *)"";
        if (*f1) startStreamSide(0, f1);
        else { ma_sound_seek_to_pcm_frame(&streamB, 0); ma_sound_start(&streamB); }
    }
}

void SConcertMA::DumpChannels() {
    Logger.g->Log(0, "SConcertMA channels: cache=%zu voices=%zu transient=%d stream=%d",
                  cache.size(), voices.size(), transientCount, streamCurrent);
    int activeVoices = 0;
    for (auto &v : voices) if (v.inUse) ++activeVoices;
    Logger.g->Log(0, "  active voices=%d", activeVoices);
}

// ---------------------------------------------------------------------------
// Factory — replaces the DSound CreateConcert when this backend is selected
// ---------------------------------------------------------------------------
SIConcert *CreateConcert(HWND /*hwnd*/) {
    return new SConcertMA();
}
