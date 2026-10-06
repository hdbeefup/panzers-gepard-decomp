// src/3dengine/pz/effecttypes.cpp
// The .fx effect types besides particles that the Training Camp mission
// plays (HD 0x6e9f70..0x6ee8e0): decals (4), sounds (5), lights (7), smoke
// trails (8) and camera shakes (10). OWNER: agent M3-E.
//
// Every instance runs through SEffect::Update (0x6de810) -> Process once per
// rendered frame and Render once per layer pass, like particles. Time is the
// scene frame time (scene +0xac ms * 0.001) unless noted. None of this code
// reads or writes logic state: it only feeds the terrain, the scene lights,
// the scene smoke trails, the viewport eye offset and the Miles concert.

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include "effect.h"
#include "pzpixie.h"
#include "pzscene.h"
#include "pzterrain.h"
#include "pzviewport.h"
#include "igepardhd.h"
#include "iterrain.h"
#include "timer.h"
#include "logger.h"
#include "stub_log.h"
#if PZ_AUDIO_MILES
#include "milesconcert.h"
#endif

namespace pz {

// Scene frame time in seconds as HD computes it: (float)(unsigned ms) * 0.001f
// (DAT_0087c78c; the int -> double conversion corrects for the sign bit).
static float FrameSeconds(SIScene* scene)
{
    double ms = (double)(unsigned)SceneFrameMs(scene);
    return (float)ms * 0.001f;
}

static SITerrain* EffectTerrain(SIScene* scene)
{
    return static_cast<SITerrain*>(static_cast<STerrain*>(SceneTerrain(scene)));
}

// HD reads the STimer 0x92e354 through 0x661800 (seconds).
static float TimerSeconds()
{
    return (float)((double)Timer.GetTickValue() / 1000.0);
}

// ===========================================================================
// 4: decal
// ===========================================================================

// PANZERS 0x6ea0f0
SPDecalEffect::SPDecalEffect()
    : NumTextures(0), BlendType(0), ForceBright(false), RealTime(false), Duration(0.0f),
      Unstoppable(false), RandomRotate(false)
{
    for (int i = 0; i < 8; ++i)
        Textures[i] = -1;
}

// PANZERS 0x6ea1d0
SPDecalEffect::~SPDecalEffect()
{
    for (int i = 0; i < 8; ++i)
        if (Textures[i] >= 0)
            PzGepard()->ReleaseTexture(Textures[i]);   // Gepard +0x48
}

// PANZERS 0x6ea3c0
// Returns true on failure. HD reports a missing name or texture with the
// SLogger last error (0x65ca40). Note: HD writes up to the "Textures" array
// size into the 8-entry table (the schema caps the array at 8).
bool SPDecalEffect::Init(SPropertyStruct* ts, const char* name)
{
    BlendType = ts->GetEnum(0, "BlendType");
    NumTextures = ts->GetArraySize("Textures");
    for (int i = 0; i < NumTextures; ++i) {
        SProperty* item = ts->GetArrayItem("Textures", i);
        const char* tex = static_cast<SPropertyString*>(item)->Value.c_str();
        if (!tex || !*tex) {
            EffectSetLastError("SPDecalEffect::Init(): No texture specified for effect \"%s\".", name);
            return true;
        }
        std::string path = std::string("effects/media/") + tex;
        int t = PzGepard()->LoadTexture(path.c_str(), 1, true);   // Gepard +0x44 (name, 1, 1)
        Textures[i] = t;
        if (t < 0) {
            EffectSetLastError("SPDecalEffect::Init(): Can't load texture \"effects/media/%s\" for effect \"%s\".",
                               tex, name);
            return true;
        }
    }
    ForceBright = ts->GetBool(2, "ForceBright");
    Duration = ts->GetFloat(3, "Duration");
    RealTime = ts->GetBool(4, "RealTime");
    ts->GetTrackFloat("Alpha", &AlphaTrack);
    ts->GetTrackFloat("Scale", &ScaleTrack);
    Unstoppable = ts->GetBool(7, "Unstoppable");
    RandomRotate = ts->GetBool(8, "RandomRotate");
    return false;
}

// PANZERS 0x6ea350
SEffect* SPDecalEffect::CreateInstance(SIScene* scene, float)
{
    return new SDecalEffect(scene, this);   // new 0x6c
}

// PANZERS 0x6e9f70
// The decal is created at the effect's position, which is still (0, 0, 0)
// here; Render moves it every frame. CRT rand (0x78c846) draws the rotation
// (when RandomRotate) and the texture.
SDecalEffect::SDecalEffect(SIScene* scene, SPDecalEffect* proto)
    : SEffect(proto), Scene(scene), P(proto), CurAlpha(0), CurScale(0), Decal(-1),
      Time(0.0f), LastSeconds(0.0f), Stopped(false)
{
    Alpha = P->AlphaTrack.Evaluate(0.0f, &CurAlpha);
    Scale = P->ScaleTrack.Evaluate(0.0f, &CurScale);
    float rot = 0.0f;
    if (P->RandomRotate)
        rot = (float)((double)rand() * (1.0 / 32768.0) * 6.2831854820251465);   // 0x7f4540, 0x7f4570
    SITerrain* t = EffectTerrain(Scene);
    int tex = P->Textures[rand() % P->NumTextures];
    if (!t)
        return;   // recompile: a scene without terrain (fxview); HD assumes one
    Decal = t->CreateEffectDecal(tex, Pos[0], Pos[2], rot, Scale, 0xffffff, P->BlendType,
                                 P->ForceBright, P->ForceBright);   // terrain +0x60
    t->SetEffectDecalAlpha(Decal, Alpha);                           // terrain +0x74
    if (P->RealTime)
        LastSeconds = TimerSeconds();
}

// PANZERS 0x6ea2a0
SDecalEffect::~SDecalEffect()
{
    if (Decal >= 0 && EffectTerrain(Scene))
        EffectTerrain(Scene)->DestroyEffectDecal(Decal);   // terrain +0x64
}

// PANZERS 0x6ea650
bool SDecalEffect::Process()
{
    PZ_TRACE("SDecalEffect::Process (0x6ea650)");
    float dt = FrameSeconds(Scene);
    if (P->RealTime) {
        float now = TimerSeconds();
        dt = now - LastSeconds;
        LastSeconds = now;
    }
    if (dt != 0.0f) {
        Time = Time + dt;
        Alpha = P->AlphaTrack.Evaluate(Time / P->Duration, &CurAlpha);
        Scale = P->ScaleTrack.Evaluate(Time / P->Duration, &CurScale);
        if (P->Duration != 0.0f && P->Duration <= Time)
            return false;
    }
    return !Stopped;
}

// PANZERS 0x6ea740
void SDecalEffect::Render(SIViewport*)
{
    PZ_TRACE("SDecalEffect::Render (0x6ea740)");
    STerrain* st = static_cast<STerrain*>(SceneTerrain(Scene));
    if (!st)
        return;   // recompile: no terrain (fxview)
    SITerrain* t = st;
    t->SetEffectDecalPosition(Decal, Pos[0], Pos[2]);               // terrain +0x68
    if (!P->RandomRotate)
        t->SetEffectDecalRotation(Decal, (float)atan2((double)Dir[0], (double)Dir[2]));   // +0x6c, _CIatan2 0x78d07a
    t->SetEffectDecalAlpha(Decal, Alpha);                           // +0x74
    st->SetEffectDecalScale(Decal, Scale);                          // 0x6f89e0
}

// PANZERS 0x6ea810
void SDecalEffect::Stop()
{
    if (!P->Unstoppable)
        Stopped = true;
}

// ===========================================================================
// 5: sound
// ===========================================================================

#if PZ_AUDIO_MILES
static SMilesConcert* EffectConcert() { return g_MilesConcert; }   // HD 0x92e798
#endif

// PANZERS 0x6ec900
SPSoundEffect::SPSoundEffect()
    : Looping(false), Positional(false), SoundGroup(0), DistanceMin(0.0f), Duration(0.0f),
      Frequency(1.0f), NumSounds(0)
{
    Sounds[0] = -1;
    Sounds[1] = Sounds[2] = -1;
}

// PANZERS 0x6ec9e0
// The cached sounds stay in the concert's cache (HD releases nothing here).
SPSoundEffect::~SPSoundEffect() {}

// PANZERS 0x6ecc40
bool SPSoundEffect::Init(SPropertyStruct* ts, const char*)
{
    SPropertyStruct* birth = ts->GetStruct(0, "Birth");
    SoundGroup = birth->GetEnum(0, "SoundGroup");
    Looping = birth->GetBool(1, "Looping");
    SoundFile[0] = birth->GetString(2, "SoundFile");
    SoundFile[1] = birth->GetString(3, "SoundFile2");
    SoundFile[2] = birth->GetString(4, "SoundFile3");
    Positional = birth->GetBool(5, "Positional");
    DistanceMin = birth->GetFloat(6, "Distance_Min");
    birth->GetTrackFloat("Volume", &VolumeTrack);
    Duration = birth->GetFloat(8, "Duration");
    float rnd = (float)birth->GetInt(9, "RNDFrequency");
    Frequency = rnd;
    if (rnd == 0.0f) {
        Frequency = 1.0f;
    } else {
        // CRT rand 0x78c846, at the HD site (render side, not in the CRC).
        bool up = ((rand() * 100) >> 15) % 2 == 0;
        int r = (rand() * ((int)Frequency & 0xffff)) >> 15;
        Frequency = up ? (float)r / 100.0f + 1.0f : 1.0f - (float)r / 100.0f;
    }
    NumSounds = 0;
#if PZ_AUDIO_MILES
    SMilesConcert* c = EffectConcert();
    for (int i = 0; i < 3; ++i) {
        if (SoundFile[i].empty())
            continue;
        Sounds[NumSounds] = c ? c->PrecacheSound(SoundFile[i].c_str(), Positional) : -1;   // concert +0x24
        ++NumSounds;
    }
#endif
    return false;
}

// PANZERS 0x6ecb90
SEffect* SPSoundEffect::CreateInstance(SIScene* scene, float)
{
    return new SSoundEffect(scene, this);   // new 0x68
}

SSoundEffect::SSoundEffect(SIScene* scene, SPSoundEffect* proto)
    : SEffect(proto), CurVolume(0), Scene(scene), P(proto), Stopped(false), Time(0.0f),
      VolumeScale(1.0f), FrequencyScale(1.0f), VolumeDirty(true), FrequencyDirty(true), Sound(-1)
{
}

// PANZERS 0x6ecb10
SSoundEffect::~SSoundEffect()
{
#if PZ_AUDIO_MILES
    if (Sound >= 0 && EffectConcert())
        EffectConcert()->RemoveSound(Sound);   // concert +0x3c
#endif
}

// PANZERS 0x6ece40
// One-shot sounds play once (2D or 3D, a random file of the three) in the
// first Process and the instance dies. Looping sounds are created once and
// then follow the effect (position, SetSpeed volume and frequency) until the
// effect is disabled or stopped.
bool SSoundEffect::Process()
{
    PZ_TRACE("SSoundEffect::Process (0x6ece40)");
    if (P->NumSounds == 0)
        return false;
    Time = FrameSeconds(Scene) + Time;
    float vol = P->VolumeTrack.Evaluate(Time / P->Duration, &CurVolume) * VolumeScale;
#if PZ_AUDIO_MILES
    SMilesConcert* c = EffectConcert();
    if (!c)
        return !Stopped;
    if (!Enabled) {
        if (P->Looping && Sound >= 0) {
            c->RemoveSound(Sound);                                       // +0x3c
            Sound = -1;
        }
        return !Stopped;
    }
    if (!P->Looping) {
        if (P->Positional) {
            int i = rand() % P->NumSounds;                               // CRT rand 0x78c846
            c->PlaySound3DByIdEx(P->Sounds[i], P->DistanceMin, Pos);     // +0x5c
            return false;
        }
        float v0 = P->VolumeTrack.Evaluate(0.0f, &CurVolume);
        int i = rand() % P->NumSounds;
        c->PlaySoundById(P->Sounds[i], v0, 0.0f, -1);                    // +0x54
        return false;
    }
    if (Sound < 0) {
        if (!P->Positional)
            Sound = c->CreateSoundByIdEx(P->Sounds[0], vol != 0.0f, 0.0f, 0.0f, true);   // +0x30
        else
            Sound = c->CreateSound3DByIdEx(P->Sounds[0], P->SoundGroup, P->DistanceMin, Pos); // +0x38
    }
    if (Sound >= 0) {
        if (!P->Positional) {
            c->SetSoundVolume(Sound, vol, false);                        // +0x48
            return !Stopped;
        }
        c->SetSoundPositionV(Sound, Pos);                                // +0x40
        if (FrequencyDirty) {
            c->SetSoundFrequency(Sound, (unsigned)(int)(FrequencyScale * 30000.0f + 14100.0f)); // +0x44, 0x884460 / 0x88445c
            FrequencyDirty = false;
        }
        if (VolumeDirty) {
            c->SetSoundVolume(Sound, vol, false);                        // +0x48
            VolumeDirty = false;
        }
    }
#else
    (void)vol;
    STUB_LOG("SSoundEffect::Process (0x6ece40): no Miles concert in this build");
#endif
    return !Stopped;
}

// PANZERS 0x6ed0a0
void SSoundEffect::Render(SIViewport*) {}

// PANZERS 0x6ed140
void SSoundEffect::Stop()
{
#if PZ_AUDIO_MILES
    if (Sound >= 0 && EffectConcert()) {
        EffectConcert()->RemoveSound(Sound);
        Sound = -1;
    }
#endif
    Stopped = true;
}

// PANZERS 0x6ed0b0
// SDriver::SetEffectsSpeed (0x55c900): p1 -> frequency 0..1, p2 -> volume.
void SSoundEffect::SetSpeed(float p1, float p2)
{
    float v = p2 * 0.5f + 0.5f;
    if (v < 1.0f) {
        if (v <= 0.0f)
            v = 0.0f;
    } else {
        v = 1.0f;
    }
    if (VolumeScale != v) {
        VolumeScale = v;
        VolumeDirty = true;
    }
    float f = 1.0f;
    if (p1 < 1.0f) {
        f = p1;
        if (p1 <= 0.0f)
            f = 0.0f;
    }
    if (FrequencyScale != f) {
        FrequencyScale = f;
        FrequencyDirty = true;
    }
}

// ===========================================================================
// 7: lite
// ===========================================================================

// PANZERS 0x6ed790
SPLiteEffect::SPLiteEffect()
    : Duration(0.0f), Range(0.0f), Attenuation(0.0f), Unstoppable(false), Altitude(0.0f)
{
    Color[0] = Color[1] = Color[2] = Color[3] = 0.0f;
}

// PANZERS 0x6ed900
SPLiteEffect::~SPLiteEffect() {}

// PANZERS 0x6ed9e0
bool SPLiteEffect::Init(SPropertyStruct* ts, const char*)
{
    unsigned c = ts->GetColor(0, "Color");
    Color[3] = 0.0f;
    Color[0] = (float)(double)(c >> 16 & 0xff) / 255.0f;
    Color[1] = (float)(double)(c >> 8 & 0xff) / 255.0f;
    Color[2] = (float)(double)(c & 0xff) / 255.0f;
    Duration = ts->GetFloat(1, "Duration");
    Range = ts->GetFloat(2, "Range");
    Attenuation = ts->GetFloat(3, "Attenuation");
    ts->GetTrackFloat("Brightness", &BrightnessTrack);
    Unstoppable = ts->GetBool(5, "Unstoppable");
    Altitude = ts->GetFloat("Altitude") * 0.5f;
    return false;
}

// PANZERS 0x6ed970
SEffect* SPLiteEffect::CreateInstance(SIScene* scene, float)
{
    return new SLiteEffect(scene, this);   // new 0x60
}

// PANZERS 0x6ed700
SLiteEffect::SLiteEffect(SIScene* scene, SPLiteEffect* proto)
    : SEffect(proto), Scene(scene), P(proto), CurBrightness(0), Light(-1), Time(0.0f), Stopped(false)
{
    Brightness = P->BrightnessTrack.Evaluate(0.0f, &CurBrightness);
}

// PANZERS 0x6ed890
SLiteEffect::~SLiteEffect()
{
    if (Light >= 0)
        Scene->DestroyLight(Light);   // scene +0x38
}

// PANZERS 0x6edaf0
bool SLiteEffect::Process()
{
    PZ_TRACE("SLiteEffect::Process (0x6edaf0)");
    if (!Active)
        return false;
    float dt = FrameSeconds(Scene);
    if (dt != 0.0f) {
        Time = Time + dt;
        Brightness = P->BrightnessTrack.Evaluate(Time / P->Duration, &CurBrightness);
        if (P->BrightnessTrack.Loop && P->Duration <= Time)
            Time = Time - P->Duration;
        if (P->Duration <= Time)
            return false;
    }
    return !Stopped;
}

// PANZERS 0x6edba0
// The light is created on the first render pass and then follows the
// effect; with Altitude it sits that high above the terrain (0x6f4c00).
void SLiteEffect::Render(SIViewport*)
{
    PZ_TRACE("SLiteEffect::Render (0x6edba0)");
    float c[4] = { P->Color[0] * Brightness, P->Color[1] * Brightness,
                   P->Color[2] * Brightness, P->Color[3] * Brightness };
    if (Light == -1) {
        Light = Scene->CreatePointLight(c[0], c[1], c[2], c[3], Pos[0], Pos[1], Pos[2],
                                        P->Range, P->Attenuation);   // scene +0x30
        return;
    }
    float x = Pos[0], y = Pos[1], z = Pos[2];
    if (P->Altitude != 0.0f) {
        STerrain* t = static_cast<STerrain*>(SceneTerrain(Scene));
        y = (t ? t->HeightAt(x, z) : 0.0f) + P->Altitude;   // recompile: fxview has no terrain
    }
    Scene->SetLightPosition(Light, x, y, z);                        // scene +0x34
    static_cast<SScene*>(Scene)->SetLightColor(Light, c);           // 0x6bab60
}

// PANZERS 0x6edcc0
void SLiteEffect::Stop()
{
    if (!P->Unstoppable)
        Stopped = true;
}

// ===========================================================================
// 8: smoke trail
// ===========================================================================

// PANZERS 0x6edcd0
SPTrailEffect::SPTrailEffect()
    : Color(0), Strength(0.0f), FadeSpeed(0.0f), UScale(0.0f), VScale(0.0f), DrawType(0),
      Duration(0.0f), Texture(-1), LastScene(nullptr)
{
}

// PANZERS 0x6edd70
SPTrailEffect::~SPTrailEffect()
{
    PzGepard()->ReleaseTexture(Texture);   // Gepard +0x48
}

// PANZERS 0x6edff0
// "Texture" is a full path (default effects/media/smoketrail_a.tga).
bool SPTrailEffect::Init(SPropertyStruct* ts, const char*)
{
    Color = ts->GetColor("Color");
    Texture = PzGepard()->LoadTexture(ts->GetString("Texture"), 1, true);   // Gepard +0x44 (name, 1, 1)
    Strength = ts->GetFloat("Strength");
    FadeSpeed = ts->GetFloat("FadeSpeed");
    UScale = ts->GetFloat("U_Scale");
    VScale = ts->GetFloat("V_Scale");
    DrawType = ts->GetEnum("DrawType");
    ts->GetTrackFloat("Alpha", &AlphaTrack);
    ts->GetTrackFloat("Size", &SizeTrack);
    Duration = ts->GetFloat("Duration");
    return false;
}

// PANZERS 0x6edf40
SEffect* SPTrailEffect::CreateInstance(SIScene* scene, float)
{
    LastScene = scene;
    return new STrailEffect(scene, this);   // new 0x68
}

STrailEffect::STrailEffect(SIScene* scene, SPTrailEffect* proto)
    : SEffect(proto), FrameSeconds(0.0f), Scene(scene), P(proto), Trail(-1), Stopped(false),
      AlphaScale(1.0f), SizeScale(0.0f)
{
}

// PANZERS 0x6edec0
STrailEffect::~STrailEffect()
{
    if (Trail >= 0)
        Scene->CloseSmokeTrail(Trail);   // scene +0x88
}

// PANZERS 0x6ee0d0
// HD then calls 0x6ac6d0 (an empty SScene member) with the trail and dt.
bool STrailEffect::Process()
{
    PZ_TRACE("STrailEffect::Process (0x6ee0d0)");
    FrameSeconds = (float)((double)(unsigned)SceneFrameMs(Scene) * 0.001);   // 0x7f59b8
    return !Stopped;
}

// PANZERS 0x6ee120
void STrailEffect::Render(SIViewport*)
{
    PZ_TRACE("STrailEffect::Render (0x6ee120)");
    if (Trail == -1 && Enabled)
        Trail = Scene->CreateSmokeTrail(P->Texture, Pos[0], Pos[1], Pos[2], P->Color, P->Strength,
                                        P->FadeSpeed, P->UScale, P->VScale, P->DrawType == 1,
                                        &P->AlphaTrack, &P->SizeTrack, P->Duration,
                                        AlphaScale, SizeScale);           // scene +0x7c
    if (Trail != -1 && Enabled)
        Scene->TrackSmokeTrail(Trail, Pos[0], Pos[1], Pos[2], AlphaScale, SizeScale);   // +0x80
    if (Trail != -1 && !Enabled) {
        Scene->CloseSmokeTrail(Trail);                                    // +0x88
        Trail = -1;
    }
}

// ===========================================================================
// 10: camera shake
// ===========================================================================

// PANZERS 0x6ee390
SPCameraShake::SPCameraShake() : Positional(false), Strength(0.0f), Duration(0.0f) {}

// PANZERS 0x6ee460
SPCameraShake::~SPCameraShake() {}

// PANZERS 0x6ee5e0
bool SPCameraShake::Init(SPropertyStruct* ts, const char*)
{
    SPropertyStruct* birth = ts->GetStruct(0, "Birth");
    Positional = birth->GetBool(0, "Positional");
    Strength = birth->GetFloat(1, "Strength");
    birth->GetTrackFloat("Amplitude", &AmplitudeTrack);
    Duration = birth->GetFloat(3, "Duration");
    return false;
}

// PANZERS 0x6ee570
SEffect* SPCameraShake::CreateInstance(SIScene* scene, float)
{
    return new SCameraShake(scene, this);   // new 0x70
}

// PANZERS 0x6ee260
// Six CRT rand phases (0x78c846, render side).
SCameraShake::SCameraShake(SIScene* scene, SPCameraShake* proto)
    : SEffect(proto), CurAmplitude(0), Scene(scene), P(proto), Stopped(false), Time(0.0f)
{
    for (int i = 0; i < 6; ++i)
        Phase[i] = (float)((double)rand() * (1.0 / 32768.0) * 6.2831854820251465);
}

// PANZERS 0x6ee650
bool SCameraShake::Process()
{
    PZ_TRACE("SCameraShake::Process (0x6ee650)");
    Time = FrameSeconds(Scene) + Time;
    return !Stopped && Time < P->Duration;
}

// PANZERS 0x6ee6a0
// Each axis is the sum of two sines (0x78d480, the SSE2 CRT sine) of fixed
// frequencies (0x8845a0..0x8845c0) and random phases, times the amplitude
// track * Strength; positional shakes fall off with 30 / distance^2 from the
// camera eye.
void SCameraShake::Render(SIViewport* vp)
{
    PZ_TRACE("SCameraShake::Render (0x6ee6a0)");
    float a = P->AmplitudeTrack.Evaluate(Time / P->Duration, &CurAmplitude) * P->Strength;
    double t = (double)Time;
    float o[3];
    o[0] = (float)(sin(t * 33.33333407839141 + (double)Phase[0]) + sin(t * 57.13333461036287 + (double)Phase[3]));
    o[1] = (float)(sin(t * 19.033333758761493 + (double)Phase[4]) + sin(t * 45.56666768516106 + (double)Phase[1]));
    o[2] = (float)(sin(t * 37.10000082924964 + (double)Phase[5]) + sin(t * 27.066667271653827 + (double)Phase[2]));
    if (P->Positional) {
        float cx = 0.0f, cy = 0.0f, cz = 0.0f, yaw, pitch;
        vp->GetCamera(&cx, &cy, &cz, &yaw, &pitch);                      // vp +0x24
        float dx = Pos[0] - cx, dy = Pos[1] - cy, dz = Pos[2] - cz;
        float f = (a * 30.0f) / (dx * dx + dy * dy + dz * dz);           // 0x7fb6b8
        o[0] = f * o[0];
        o[1] = f * o[1];
        o[2] = f * o[2];
    } else {
        o[0] = o[0] * a;
        o[2] = o[2] * a;
        o[1] = o[1] * a;
    }
    ViewportAddCameraOffset(vp, o);
}

// PANZERS 0x6ee4d0
// HD walks from a sub-viewport to its root (+0x1f4 parent index into the
// +0x1f8 array) first; the recompile's facade viewport has no subports.
void ViewportAddCameraOffset(SIViewport* vp, const float* offset)
{
    SViewport* v = static_cast<SViewport*>(vp);
    v->CamOffset[0] = v->CamOffset[0] + offset[0];
    v->CamOffset[1] = offset[1] + v->CamOffset[1];
    v->CamOffset[2] = offset[2] + v->CamOffset[2];
}

} // namespace pz
