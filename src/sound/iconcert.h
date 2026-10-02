// sound/iconcert.h
// IConcert interface
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef SOUND_ICONCERT_H
#define SOUND_ICONCERT_H

struct SIConcert {
    virtual void AddRef() = 0;
    virtual void Release() = 0;
    virtual void SetListener(float, float, float, float, float, float, float, float, float) = 0;
    virtual void Update(bool) = 0;
    virtual void SetVolume(int, int) = 0;
    virtual void SetReverseStereo(bool) = 0;
    virtual int PrecacheSound(const char *, bool) = 0;
    virtual void AddRefToCachedSound(int) = 0;
    virtual void ReleaseCachedSound(int) = 0;
    virtual int CleanupSoundCache() = 0;
    virtual int CreateSound(const char *, float, float) = 0;
    virtual int CreateSoundById(int, float, float) = 0;
    virtual int CreateSound3D(const char *, int, float, float, float, float) = 0;
    virtual int CreateSound3DById(int, int, float, float, float, float) = 0;
    virtual void RemoveSound(int) = 0;
    virtual void RemoveAllSounds() = 0;
    virtual void SetSoundPosition(int, float, float, float) = 0;
    virtual void SetSoundFrequency(int, unsigned int) = 0;
    virtual void SetSoundMinDistance(int, float) = 0;
    virtual float PlaySound(const char *, float, float, int) = 0;
    virtual float PlaySoundById(int, float, float, int) = 0;
    virtual float PlaySound3D(const char *, float, float, float, float) = 0;
    virtual float PlaySound3DById(int, float, float, float, float) = 0;
    virtual void StartStreamingPlayback(const char *, const char *) = 0;
    virtual void StopStreamingPlayback() = 0;
    virtual bool CheckStreamPlayback() = 0;
    virtual void NextTrack() = 0;
    virtual void DumpChannels() = 0;
};

// Factory function — creates SConcert, initializes DirectSound
SIConcert *CreateConcert(HWND hwnd);

// Diagnostic hook for HD_HDBEEFUP_AUDIO_DEBUG. The audio backend calls
// `g_AudioPlayHook(path, kind)` from PlaySound entry points; the game side
// (gameview.cpp) registers a forwarder that calls SGameWorld::Echo so the
// path appears in the rolling HUD message stack instead of the log file.
// `kind` is a short tag — "play", "dup", "fail" — so the overlay can show
// whether channel-id dedup engaged. Hook is null when the toggle is off.
typedef void (*SwineAudioPlayHook)(const char *path, const char *kind);
extern SwineAudioPlayHook g_AudioPlayHook;

#endif // SOUND_ICONCERT_H
