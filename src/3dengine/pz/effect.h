// src/3dengine/pz/effect.h
// HD effect classes behind pz::SPixie: prototypes (SPEffect family, loaded
// from .fx property files) and instances (SEffect family, played into a
// scene). OWNER: agent C.
//
//   SPEffectSet  one loaded .fx file (0x30, ctor 0x6dea70, vftable 0x883c5c):
//                a heap of SPEffect, one per "Effects.N.Data" entry.
//   SEffectSet   one playing .fx (0x38, ctor 0x6de8c0, vftable 0x883c64):
//                a heap of SEffect, one per prototype entry; SPixie keeps
//                these in its effect heap and hands out the heap index.
//   SPEffect     prototype base (0x14, vftable 0x879850, 6 slots).
//   SEffect      instance base (0x44, ctor 0x6de530, vftable 0x883c24, 13
//                slots; pz::SIEffect in ipixie.h has the same order).
//   SPParticles  EffectType 0 prototype (0x1a0, ctor 0x6e08b0, vftable 0x883e54)
//   SParticles   EffectType 0 instance (0x490, ctor 0x6e0980, vftable 0x883e70)
// EffectTypes 4 (decal), 5 (sound), 7 (lite), 8 (trail) and 10 (camera
// shake) are in effecttypes.cpp (agent M3-E). Flare, rain, snow,
// atmosphere, shock wave and sandstorm are still logged stubs in
// SPixie::InitEffectPrototype (the Training Camp path creates none).
//
// Field comments give the HD offsets. The objects are internal to agent C,
// so only SPixie (pzpixie.h) is held to the HD size.

#ifndef PZ_EFFECT_H
#define PZ_EFFECT_H

#include <string>
#include <vector>
#include <stdlib.h>
#include <string.h>
#include "pzcommon.h"
#include "ipixie.h"
#include "imodel.h"
#include "propertystruct.h"
#include "mesh.h"   // SRenderPass (the weather effects keep one)

namespace pz {

struct SIScene;
struct SIViewport;
struct SPixie;
struct SEffect;
struct SEffectSet;

// The effect property schema, built by the SPixie ctor (effectschema.cpp).
SPProperty* BuildEffectsSchema();   // HD 0x92f108 PProperty_Effects
SPProperty* BuildEfxSchema();       // HD 0x92f100 PProperty_EFX

// HD SHeap<T*> (0x14 bytes): slots of {0x7fffffff when used / next free
// index, T*}, growth 16 then *6/5. Index stability matters: SPixie hands
// heap indices out as handles.
template <class T>
struct SPtrHeap {
    struct SSlot { int Use; T* Ptr; };
    SSlot* Slots = nullptr;   // +0x00
    int Used = 0;             // +0x04 slots in use or free-listed
    int Max = 0;              // +0x08
    int FreeHead = -1;        // +0x0c
    int Count = 0;            // +0x10

    SPtrHeap() {}
    ~SPtrHeap() { free(Slots); }
    SPtrHeap(const SPtrHeap&) = delete;
    SPtrHeap& operator=(const SPtrHeap&) = delete;

    int Add()                 // 0x69d130 / 0x69cff0 / 0x69d090 / 0x6ded30 (one template)
    {
        ++Count;
        if (FreeHead >= 0) {
            int i = FreeHead;
            FreeHead = Slots[i].Use;
            Slots[i].Use = 0x7fffffff;
            Slots[i].Ptr = nullptr;
            return i;
        }
        if (Used == Max) {
            int n = Max < 0x10 ? 0x10 : Max * 6 / 5;
            Slots = (SSlot*)realloc(Slots, n * sizeof(SSlot));
            memset(Slots + Max, 0, (n - Max) * sizeof(SSlot));
            Max = n;
        }
        Slots[Used].Use = 0x7fffffff;
        Slots[Used].Ptr = nullptr;
        return Used++;
    }
    bool Valid(int i) const { return i >= 0 && i < Used && Slots[i].Use == 0x7fffffff; }
    T*& operator[](int i);    // panics "SHeap<%s>::operator[]: invalid index (%d)"
    void Remove(int i);       // 0x69e990 / 0x69e930 / 0x6df030
    int Size() const { return Used; }
};

// ---------------------------------------------------------------------------
// Prototypes
// ---------------------------------------------------------------------------

// HD SPEffect (0x14). vftable 0x879850.
struct SPEffect {
    virtual ~SPEffect();                                          // +0x00 0x69cf50
    virtual bool IsBulletIndicator();                             // +0x04 0x69dc10 returns false (SPParticles: "Bullet Indicator")
    virtual void ReloadResources();                               // +0x08 0x69ee20 nop (name guessed; SPixie +0x08 calls it for every prototype)
    // Returns true on failure (the caller then drops the prototype).
    virtual bool Init(SPropertyStruct* typeStruct, const char* name); // +0x0c 0x69d680 warns "Method not implemented"
    virtual void Slot_10();                                       // +0x10 0x69d660 panics (unused)
    virtual SEffect* CreateInstance(SIScene* scene, float param) = 0; // +0x14 (purecall in SPEffect)

    std::string Name;      // +0x04 SString
    int  PriorityLayer;    // +0x0c "PriorityLayer" (0..2: SPixie draws layer 0, 1, 2)
    bool ForceUpdate;      // +0x10 "ForceUpdate"

protected:
    SPEffect();            // 0x6de5a0
};

// HD SPEffectSet (0x30). vftable 0x883c5c, one slot.
struct SPEffectSet {
    SPEffectSet();                                    // 0x6dea70
    ~SPEffectSet();                                   // 0x6dec10 (non-virtual)
    virtual bool IsBulletIndicator();                 // +0x00 0x6def00 (first prototype's)

    void ReloadResources();                           // 0x6df140
    void AddRef() { ++RefCount; }                     // 0x6dedd0
    void Release();                                   // 0x6df000
    SEffectSet* CreateInstance(SIScene* scene, int handle, float param, int model); // 0x6dede0

    SPtrHeap<SPEffect> Effects;   // +0x04
    std::string FileName;         // +0x18 SString
    int  Index;                   // +0x20 SPixie prototype heap index (0x6df3c0)
    int  RefCount;                // +0x24 (starts at 1)
    bool ForceUpdate;             // +0x28 any entry has ForceUpdate
    SPropertyArray* Properties;   // +0x2c the "Effects" instance tree
};

// ---------------------------------------------------------------------------
// Instances
// ---------------------------------------------------------------------------

// HD SEffect (0x44). The slot order is pz::SIEffect's (ipixie.h), with
// names and signatures known here.
struct SEffect {
    virtual ~SEffect();                                           // +0x00 0x6de620
    virtual void SetPosition(const float* pos);                   // +0x04 0x6de7a0
    virtual void SetDirection(const float* dir);                  // +0x08 0x6de6d0 (normalizes)
    virtual void SetAlphaScale(float scale);                      // +0x0c 0x6de6c0 nop (SParticles 0x6e9690) (name guessed)
    virtual void SetSizeScale(float scale);                       // +0x10 0x6de7c0 nop (SParticles 0x6e9920) (name guessed)
    virtual void Slot_14(int p);                                  // +0x14 0x6de7f0 nop
    virtual void Update(unsigned frame, const float* matrix);     // +0x18 0x6de810
    virtual bool Process();                                       // +0x1c 0x6de6a0 returns false (SParticles 0x6e84a0)
    virtual void Render(SIViewport* vp);                          // +0x20 0x6de6b0 nop (SParticles 0x6e95f0)
    virtual void SetEnabled(bool on);                             // +0x24 0x6de690 (+0x2c)
    virtual void SetActive(bool on);                              // +0x28 0x6de7e0 (+0x34)
    virtual void Stop();                                          // +0x2c 0x6de800 nop (SParticles 0x6e9940)
    virtual void SetSpeed(float p1, float p2);                    // +0x30 0x6de7d0 nop (SParticles keeps it; SSoundEffect overrides) (name guessed)

    void SetModel(int model) { Model = model; }                   // 0x6de790
    void SetOwner(SEffectSet* set, int index) { Set = set; SetIndex = index; } // 0x6de770

    float Pos[3];          // +0x04
    float PrevPos[3];      // +0x10
    float Dir[3];          // +0x1c (0, 1, 0)
    unsigned LastFrame;    // +0x28
    bool  Enabled;         // +0x2c (1)
    int   Model;           // +0x30 model to birth from (birth Style 1/2; HD SModel*)
    bool  Active;          // +0x34 (1): births on (SPixie clears it for hidden effects)
    SPEffect* Proto;       // +0x38
    SEffectSet* Set;       // +0x3c
    int   SetIndex;        // +0x40

protected:
    explicit SEffect(SPEffect* proto);                            // 0x6de530
};

// HD SEffectSet (0x38). Base SAttachable (0x6d4a60: +0x04 parent model,
// +0x08 node; imodel.h): SPixie +0x28 / +0x30 hang a set on a model node
// (SModel::AttachChild 0x6d5940), and the model then updates it with the
// node's world matrix. vftable 0x883c64, 10 slots; +0x00..+0x0c are the
// SIAttachable slots.
struct SEffectSet : SAttachable {
    SEffectSet(SIScene* scene, SPEffectSet* proto, int handle, float param, int model); // 0x6de8c0
    ~SEffectSet() override;                                       // +0x00 0x6ded00 -> 0x6deaf0
    // (frame, const float* node matrix or 0). Returns 0 when the set deleted
    // itself (no instance left).
    int  Update(int frame, int matrix) override;                  // +0x04 0x6df6d0
    void Attach_08() override { Stop(); }                         // +0x08 0x6def70 -> 0x6df620
    void Attach_0C(bool on) override { SetEnabled(on); }          // +0x0c 0x6def80 (+0x31; the parent's visibility)
    virtual void SetPosition(const float* pos);                   // +0x10 0x6df340
    virtual void SetDirection(const float* dir);                  // +0x14 0x6df240
    virtual void Slot_18(int p);                                  // +0x18 0x6df590
    virtual void Render(SIViewport* vp, int layer);               // +0x1c 0x6df090
    virtual void SetActive(bool on);                              // +0x20 0x6df510 (+0x32)
    virtual void SetSpeed(float p1, float p2);                    // +0x24 0x6df470 (SPixie +0x64)

    void Stop();                                                  // 0x6df620
    void SetEnabled(bool on);                                     // 0x6def80
    void SetAlphaScale(float s);                                  // 0x6df1b0
    void SetSizeScale(float s);                                   // 0x6df3e0
    void SetModel(int model);                                     // 0x6df2c0
    void SetPersistent(bool on) { Persistent = on; }              // 0x6df3d0
    bool IsPersistent() const { return Persistent; }              // 0x6deef0
    void UnregisterEffect(int index);                             // 0x6df690
    void GetPosition(float* out);                                 // 0x6dee60 (first instance's)

    SIScene* Scene;          // +0x0c
    SPEffectSet* Proto;      // +0x10 (AddRef'd)
    SPtrHeap<SEffect> Effects; // +0x14
    int   Handle;            // +0x28 SPixie effect heap index
    unsigned LastFrame;      // +0x2c
    bool  Persistent;        // +0x30 created by SPixie +0x2c: survives running empty
    bool  Enabled;           // +0x31
    bool  Active;            // +0x32
    int   Model;             // +0x34
};

// ---------------------------------------------------------------------------
// Particles
// ---------------------------------------------------------------------------

// HD SParticleData, SDArray stride 0x90.
struct SParticleData {
    float Pos[3];          // +0x00
    float Dir[3];          // +0x0c trail direction
    float Vel[3];          // +0x18
    float Vel0[3];         // +0x24 birth velocity (scaled by AdditionalSpeed)
    float AddSpeed;        // +0x30 AdditionalSpeed track
    float Alpha;           // +0x34 Alpha track * alpha scale
    float Size;            // +0x38 half size: Size track * 0.5 + SizeRnd, * size scale
    float SizeRnd;         // +0x3c
    float Rotation;        // +0x40 degrees (Random rotate)
    float AlphaScale;      // +0x44
    float LifeTime;        // +0x48
    float InvLifeTime;     // +0x4c
    int   Frame;           // +0x50 texture frame
    float FrameTime;       // +0x54
    float FrameLength;     // +0x58
    float _5c;             // +0x5c
    float Age;             // +0x60
    float _64, _68;        // +0x64
    void* Model;           // +0x6c Draw "Object" (HD SModel*)
    float Spin[3];         // +0x70 radians (-1 = not set)
    float SpinSpeed[3];    // +0x7c
    bool  Collided;        // +0x88
    int   SubEffect;       // +0x8c Draw "Effect" (SPixie effect handle)
};
static_assert(sizeof(SParticleData) == 0x90, "SParticleData is the HD SDArray stride");

struct SPParticles : SPEffect {
    SPParticles();                                                    // 0x6e08b0
    ~SPParticles();                                                   // 0x6e1010 (0x6e1430 deleting)
    bool IsBulletIndicator() override;                                // +0x04 0x6e6880
    void ReloadResources() override;                                  // +0x08 0x6e9680 nop
    bool Init(SPropertyStruct* typeStruct, const char* name) override; // +0x0c 0x6e5eb0
    SEffect* CreateInstance(SIScene* scene, float lifeTime) override; // +0x14 0x6e3830

    bool LoadTextures(int weather, const char* suffix);              // 0x6e6890

    // +0x14 texture name, +0x1c texture mode ("Texture Anim in": 0 more
    // files, 1 one file), +0x20 "ID for Debug".
    std::string Texture;
    int   TexMode;
    int   DebugID;
    int   WeatherTex[3];       // +0x24 Default / _snowy / _foggy
    std::vector<int> FrameTex; // +0x30 "More files" textures
    bool  MultiFile;           // +0x3c
    int   MaxParticles;        // +0x40
    SIScene* LastScene;        // +0x44 (0x6e3830 stores the scene)
    int   BlendType;           // +0x50 0 alpha, 1 add
    bool  ZBuffer;             // +0x54
    bool  ElevDependent;       // +0x55
    int   DrawType;            // +0x58 "Draw": 0 particles, 1 object, 2 effect, 3 trail
    int   MoveType;            // +0x5c "Move": 0 shot, 1 waste
    int   ParticleType;        // +0x60 0 normal, 1 billboard, 2 cloud, 3 trail, 5 (effect)
    float TrailLength;         // +0x64
    int   TrailOrientation;    // +0x68
    float BirthSpeed;          // +0x6c
    unsigned Color;            // +0x70
    int   SubEffectProto;      // +0x74 Draw "Effect": SPixie prototype
    int   LightingModel;       // +0x78
    unsigned LightColor1;      // +0x7c
    unsigned LightColor2;      // +0x80
    float OverLight1;          // +0x84
    float LightRange;          // +0x88
    bool  FixedLightSource;    // +0x8c
    float LifeTimeRnd;         // +0x90
    int   VariationType;       // +0x94 0 random frame, 1 animate
    float Gravity;             // +0x98
    float VSpeed, VSpeedRnd;   // +0x9c +0xa0
    float SizeRnd;             // +0xa4 Size_Rnd * 0.5
    float RandomXYZ[3];        // +0xa8
    float Radius;              // +0xb4
    float RadiusSqrt;          // +0xb8 pow(Radius, 0.5)
    float RadiusCbrt;          // +0xbc pow(Radius, 1/3)
    bool  Sphere;              // +0xc0
    float Duration;            // +0xc4
    int   BirthEnabledTime;    // +0xc8 ftol
    int   BirthDisabledTime;   // +0xcc
    int   BirthEnabledRnd;     // +0xd0
    int   BirthDisabledRnd;    // +0xd4
    bool  BirthInRain;         // +0xd8
    int   BalancedBirth;       // +0xdc
    bool  Turbulence;          // +0xe0
    bool  NoWind;              // +0xe1
    bool  ObjectSpin;          // +0xe2
    float SpinMin[3];          // +0xe4 X/Y/Z min (interleaved with Random in HD:
    float SpinRnd[3];          //       +0xe4 Xmin +0xe8 Xrnd +0xec Ymin ...)
    float InvTotalFrames;      // +0xfc
    float TotalFrames;         // +0x100
    float FirstFrame;          // +0x104
    float LastFrame;           // +0x108
    float LifeTime;            // +0x10c
    float GrowTime;            // +0x110
    float GrowSpeed;           // +0x114
    float HSpeed, HSpeedRnd;   // +0x118 +0x11c
    bool  Linked;              // +0x120
    bool  TrailDirectionUpdate;// +0x121
    bool  BulletIndicator;     // +0x122
    int   BirthStyle;          // +0x124 0 centralized, 1 along the edge, 2 from basement
    int   Quantity;            // +0x128
    float SamplingRate;        // +0x12c
    bool  FromTerrain;         // +0x130
    bool  FromWater;           // +0x131
    float Altitude;            // +0x134 Altitude * 0.5
    int   CollisionType;       // +0x138 0 none, 1 slide, 2 disappear, 3 jump
    float CollisionSticking;   // +0x13c
    bool  PivotDir;            // +0x140
    float AirKeep;             // +0x144 1 - AirResistance (>= 0)
    float AirResistance2;      // +0x148
    float Wait, WaitRnd;       // +0x14c +0x150
    bool  RandomRotate;        // +0x154
    int   ModelProto;          // +0x158 Draw "Object" (Gepard model prototype, -1)
    float InvLightDuration;    // +0x15c
    STrackFloat AlphaTrack;    // +0x160
    STrackFloat LightTrack;    // +0x170
    STrackFloat SizeTrack;     // +0x180
    STrackFloat SpeedTrack;    // +0x190 AdditionalSpeed
};

struct SParticles : SEffect {
    SParticles(SIScene* scene, SPParticles* proto, float lifeTime);   // 0x6e0980
    ~SParticles();                                                    // 0x6e1230 (0x6e1460 deleting)
    void SetDirection(const float* dir) override;                     // +0x08 0x6e96b0
    void SetAlphaScale(float s) override;                             // +0x0c 0x6e9690
    void SetSizeScale(float s) override;                              // +0x10 0x6e9920
    bool Process() override;                                          // +0x1c 0x6e84a0
    void Render(SIViewport* vp) override;                             // +0x20 0x6e95f0
    void Stop() override;                                             // +0x2c 0x6e9940

    SPParticles* P() const { return Prototype; }

    int  Prepare();                                                   // 0x6e9d70 (always 2)
    void UpdateWind(float dt);                                        // 0x6e3440
    bool MoveShot();                                                  // 0x6e6cb0
    bool MoveWaste();                                                 // 0x6e7a60
    void Birth();                                                     // 0x6e21a0
    void BirthCentralized();                                          // 0x6e1890
    void BirthAt(float x, float y, float z, float frac);              // 0x6e1610
    void InitParticle(int i, float frac, float x, float y, float z);  // 0x6e2250
    void InitParticleTracks(int i);                                   // 0x6e9950
    void InitParticleSpin(int i);                                     // 0x6e8150
    void InitParticleModel(int i);                                    // 0x6e5b80
    void UpdateParticleTracks(int i);                                 // 0x6e9b60
    void UpdateParticleSpins();                                       // 0x6e7df0
    void RemoveParticle(int i);                                       // 0x6e9540
    float GroundHeight(float x, float z);                             // 0x6e5dd0
    unsigned PackColor(int i, float r, float g, float b);             // 0x6e32d0
    int  WeatherIndex();                                              // 0x6e5c40
    void SetupMaterial();                                             // 0x6e1490
    void RenderNormal(SIViewport* vp);                                // 0x6e5230 case 0 (0x6e4740..)
    void RenderBillboard(SIViewport* vp);                             // 0x6e38c0
    void RenderCloud(SIViewport* vp);                                 // 0x6e3e90
    void RenderTrail(SIViewport* vp);                                 // 0x6e5290
    void RenderObjects(SIViewport* vp);                               // 0x6e5150
    void RenderSubEffect(SIViewport* vp);                             // 0x6e4660

    bool  Stopped;             // +0x44
    float LightColor1[4];      // +0x4c * OverLight1
    float LightColor2[4];      // +0x5c
    float LightIntensity;      // +0x6c
    float LightPos[3];         // +0x70
    float LightRange2;         // +0x7c
    float BirthAngle;          // +0x80
    bool  RefreshBirthPoints;  // +0x88
    bool  LightPosSet;         // +0x89
    float FrameLength;         // +0x8c
    float FrameLengthRnd;      // +0x90
    int   CurAlpha, CurSize, CurLight, CurSpeed; // +0x94..+0xa0 track cursors
    std::vector<SParticleData> Particles; // +0xa8
    std::vector<float> BirthPoints;       // +0xb4 (x, y, z) triples
    float DurationLeft;        // +0xc0
    float BirthAcc;            // +0xc4
    bool  BirthOn;             // +0xc8
    float BirthToggle;         // +0xcc
    float ColorF[4];           // +0xd0 Color / 255, times the scene light unless additive
    unsigned char ColorB[3];   // +0xe0
    float Dt;                  // +0xe4 seconds this frame (scene ms * 0.001, <= 0.125)
    float Time;                // +0xe8
    float Air2Acc;             // +0xec
    float Right[3];            // +0xf0 emitter basis (PivotDir)
    float Forward[3];          // +0xfc
    float AirFactor;           // +0x108
    float AlphaScale;          // +0x10c
    float SizeScale;           // +0x110
    float LifeTimeBase;        // +0x114
    float WaitLeft;            // +0x118
    int   TrailHandle;         // +0x11c
    float WindBase;            // +0x120
    float WindAngle[2];        // +0x124
    float WindLow[2];          // +0x12c sin/cos * strength (low layer)
    float WindHigh[2];         // +0x134 (high layer)
    float WindStrength[2];     // +0x13c
    float WindLowHeight;       // +0x144 1.1
    float WindHighHeight;      // +0x148 2.2
    float WindLowScale;        // +0x14c 1/1.1
    float WindBlendScale;      // +0x150 1/1.1
    float WindTimer[2];        // +0x154
    float WindTurn[2];         // +0x15c
    float WindStrengthTo[2];   // +0x164
    float WindSpeed[2];        // +0x16c
    float WindPhase[2];        // +0x174
    bool  WindCalm[2];         // +0x17c
    SIScene* Scene;            // +0x180
    SPParticles* Prototype;    // +0x184
    bool  RenderFlag;          // +0x18c
    // +0x188..0x488: HD SMaterial (render state block); see effectrender.h.
};

// Scene fields the effects read. HD reads them straight from SScene (agent
// A's class); these accessors name the offsets.
unsigned SceneFrameCount(SIScene* scene);       // +0xa0 (RenderViewport increments it)
int   SceneFrameMs(SIScene* scene);             // +0xac (PrepareViewport 0x6bbc40: +0xa4 - +0xa8)
const float* SceneLightColor(SIScene* scene);   // +0xf8 ambient + sun, clamped to 1 (0x6ba970)
const char* SceneAtmosphere(SIScene* scene);    // +0x170 SString (Default / Snowy / Foggy)
void* SceneTerrain(SIScene* scene);             // +0x1c8

// Hook for the terrain height under a point (HD STerrain 0x6f4c00, water
// 0x6f50c0; agent B). Null = height 0.
extern float (*g_EffectGroundHeight)(float x, float z);
extern float (*g_EffectWaterHeight)(float x, float z);

// HD rand() (0x78c846) scaled like the HD code: rand() * (1/32768).
float EffectRand();

// HD SLogger 0x65ca40 (the logger at 0x929f28): keeps the text as the last
// error (logger +0x10, _strdup) and shows nothing. The effect loaders report
// a missing mesh / texture / sub-effect with it and return failure. The
// recompile writes the text to the log; it must not become the SWINE modal
// Warning box, which stops a nested prototype load until it is clicked.
void EffectSetLastError(const char* fmt, ...);

// ---------------------------------------------------------------------------
// Other effect types (effecttypes.cpp). HD sizes in the comments; the
// objects are internal, so only the field order follows HD.
// ---------------------------------------------------------------------------

// EffectType 4 "04_DECALEFFECT": a terrain effect decal (STerrain +0x60..+0x74).
struct SPDecalEffect : SPEffect {                                     // 0x68, ctor 0x6ea0f0, vftable 0x884098
    SPDecalEffect();
    ~SPDecalEffect();                                                 // 0x6ea1d0 (0x6ea320 deleting)
    bool Init(SPropertyStruct* ts, const char* name) override;        // +0x0c 0x6ea3c0
    SEffect* CreateInstance(SIScene* scene, float param) override;    // +0x14 0x6ea350

    int   NumTextures;       // +0x14
    int   Textures[8];       // +0x18 Gepard textures ("effects/media/<name>"), -1
    int   BlendType;         // +0x38
    bool  ForceBright;       // +0x3c (the decal's flag and pass)
    bool  RealTime;          // +0x3d timer seconds instead of scene time (runs in pause)
    float Duration;          // +0x40
    STrackFloat AlphaTrack;  // +0x44
    STrackFloat ScaleTrack;  // +0x54
    bool  Unstoppable;       // +0x64
    bool  RandomRotate;      // +0x65
};

struct SDecalEffect : SEffect {                                       // 0x6c, ctor 0x6e9f70, vftable 0x8840b4
    SDecalEffect(SIScene* scene, SPDecalEffect* proto);
    ~SDecalEffect();                                                  // +0x00 0x6ea2a0
    bool Process() override;                                          // +0x1c 0x6ea650
    void Render(SIViewport* vp) override;                             // +0x20 0x6ea740
    void Stop() override;                                             // +0x2c 0x6ea810

    SIScene* Scene;          // +0x44
    SPDecalEffect* P;        // +0x48
    int   CurAlpha;          // +0x4c
    int   CurScale;          // +0x50
    int   Decal;             // +0x54 terrain effect decal
    float Alpha;             // +0x58
    float Scale;             // +0x5c
    float Time;              // +0x60
    float LastSeconds;       // +0x64 (RealTime)
    bool  Stopped;           // +0x68
};

// EffectType 5 "05_SOUNDEFFECT": Miles sounds through SMilesConcert (HD 0x92e798).
struct SPSoundEffect : SPEffect {                                     // 0x60, ctor 0x6ec900, vftable 0x88440c
    SPSoundEffect();
    ~SPSoundEffect();                                                 // 0x6ec9e0 (0x6ecae0 deleting)
    bool Init(SPropertyStruct* ts, const char* name) override;        // +0x0c 0x6ecc40
    SEffect* CreateInstance(SIScene* scene, float param) override;    // +0x14 0x6ecb90

    std::string SoundFile[3];  // +0x14 SString x3
    bool  Looping;             // +0x2c
    bool  Positional;          // +0x2d
    int   SoundGroup;          // +0x30 0 ambient, 1 engine
    float DistanceMin;         // +0x34
    STrackFloat VolumeTrack;   // +0x38
    float Duration;            // +0x48
    float Frequency;           // +0x4c 1 +- RNDFrequency % (not read by the instance)
    int   NumSounds;           // +0x50
    int   Sounds[3];           // +0x54 concert cache indices (+0x24 PrecacheSound)
};

struct SSoundEffect : SEffect {                                       // 0x68, vftable 0x884428
    SSoundEffect(SIScene* scene, SPSoundEffect* proto);               // (inline in 0x6ecb90)
    ~SSoundEffect();                                                  // +0x00 0x6ecb10
    bool Process() override;                                          // +0x1c 0x6ece40
    void Render(SIViewport* vp) override;                             // +0x20 0x6ed0a0 nop
    void Stop() override;                                             // +0x2c 0x6ed140
    void SetSpeed(float p1, float p2) override;                       // +0x30 0x6ed0b0

    int   CurVolume;         // +0x44
    SIScene* Scene;          // +0x48
    SPSoundEffect* P;        // +0x4c
    bool  Stopped;           // +0x50
    float Time;              // +0x54
    float VolumeScale;       // +0x58 SetSpeed p2 * 0.5 + 0.5
    float FrequencyScale;    // +0x5c SetSpeed p1
    bool  VolumeDirty;       // +0x60
    bool  FrequencyDirty;    // +0x61
    int   Sound;             // +0x64 concert sound id (looping sounds), -1
};

// EffectType 7 "07_LITE": a scene point light.
struct SPLiteEffect : SPEffect {                                      // 0x48, ctor 0x6ed790, vftable 0x8844a0
    SPLiteEffect();
    ~SPLiteEffect();                                                  // 0x6ed900
    bool Init(SPropertyStruct* ts, const char* name) override;        // +0x0c 0x6ed9e0
    SEffect* CreateInstance(SIScene* scene, float param) override;    // +0x14 0x6ed970

    float Color[4];          // +0x14 RGB / 255, alpha 0
    float Duration;          // +0x24
    float Range;             // +0x28
    float Attenuation;       // +0x2c
    STrackFloat BrightnessTrack; // +0x30 (loop flag +0x3c)
    bool  Unstoppable;       // +0x40
    float Altitude;          // +0x44 Altitude * 0.5 (0: the effect height)
};

struct SLiteEffect : SEffect {                                        // 0x60, ctor 0x6ed700, vftable 0x8844bc
    SLiteEffect(SIScene* scene, SPLiteEffect* proto);
    ~SLiteEffect();                                                   // +0x00 0x6ed890
    bool Process() override;                                          // +0x1c 0x6edaf0
    void Render(SIViewport* vp) override;                             // +0x20 0x6edba0
    void Stop() override;                                             // +0x2c 0x6edcc0

    SIScene* Scene;          // +0x44
    SPLiteEffect* P;         // +0x48
    int   CurBrightness;     // +0x4c
    int   Light;             // +0x50 scene light, -1
    float Brightness;        // +0x54
    float Time;              // +0x58
    bool  Stopped;           // +0x5c
};

// EffectType 8 "08_TRAIL": a scene smoke trail (SIScene +0x7c..+0x88).
struct SPTrailEffect : SPEffect {                                     // 0x58, ctor 0x6edcd0, vftable 0x8844f4
    SPTrailEffect();
    ~SPTrailEffect();                                                 // 0x6edd70 (0x6ede90 deleting)
    bool Init(SPropertyStruct* ts, const char* name) override;        // +0x0c 0x6edff0
    SEffect* CreateInstance(SIScene* scene, float param) override;    // +0x14 0x6edf40

    unsigned Color;          // +0x14
    float Strength;          // +0x18
    float FadeSpeed;         // +0x1c
    float UScale;            // +0x20
    float VScale;            // +0x24
    int   DrawType;          // +0x28 0 normal, 1 add
    STrackFloat AlphaTrack;  // +0x2c
    STrackFloat SizeTrack;   // +0x3c
    float Duration;          // +0x4c
    int   Texture;           // +0x50
    SIScene* LastScene;      // +0x54
};

struct STrailEffect : SEffect {                                       // 0x68, vftable 0x884510
    STrailEffect(SIScene* scene, SPTrailEffect* proto);               // (inline in 0x6edf40)
    ~STrailEffect();                                                  // +0x00 0x6edec0
    void SetAlphaScale(float s) override { AlphaScale = s; }          // +0x0c 0x6ee210
    void SetSizeScale(float s) override { SizeScale = s; }            // +0x10 0x6ee230
    bool Process() override;                                          // +0x1c 0x6ee0d0
    void Render(SIViewport* vp) override;                             // +0x20 0x6ee120
    void Stop() override { Stopped = true; }                          // +0x2c 0x6ee250

    float FrameSeconds;      // +0x44
    SIScene* Scene;          // +0x48
    SPTrailEffect* P;        // +0x4c
    int   Trail;             // +0x54 scene smoke trail, -1
    bool  Stopped;           // +0x5c
    float AlphaScale;        // +0x60 (1)
    float SizeScale;         // +0x64 (0)
};

// EffectType 10 "10_CAMERASHAKE": adds an offset to the viewport eye (+0xc0).
struct SPCameraShake : SPEffect {                                     // 0x30, ctor 0x6ee390, vftable 0x884548
    SPCameraShake();
    ~SPCameraShake();                                                 // 0x6ee460
    bool Init(SPropertyStruct* ts, const char* name) override;        // +0x0c 0x6ee5e0
    SEffect* CreateInstance(SIScene* scene, float param) override;    // +0x14 0x6ee570

    bool  Positional;        // +0x14 strength * 30 / distance^2
    float Strength;          // +0x18
    STrackFloat AmplitudeTrack; // +0x1c
    float Duration;          // +0x2c
};

struct SCameraShake : SEffect {                                       // 0x70, ctor 0x6ee260, vftable 0x884564
    SCameraShake(SIScene* scene, SPCameraShake* proto);
    ~SCameraShake() {}                                                // +0x00 0x6ee430
    bool Process() override;                                          // +0x1c 0x6ee650
    void Render(SIViewport* vp) override;                             // +0x20 0x6ee6a0
    void Stop() override { Stopped = true; }                          // +0x2c 0x6ee8d0

    int   CurAmplitude;      // +0x44
    SIScene* Scene;          // +0x48
    SPCameraShake* P;        // +0x4c
    bool  Stopped;           // +0x50
    float Time;              // +0x54
    float Phase[6];          // +0x58 rand() * 2 pi each
};

// ---------------------------------------------------------------------------
// Weather effect types (weatherfx.cpp, agent M6-WX). The world's weather
// (SWorld 0x6088f0) plays "Effects/Atmosphere/Rain.fx" / "Snowfall.fx" as
// persistent effects and sets their intensity through SPixie +0x54
// (instance +0x14). Both keep 4 x 4 tiles of 400 particles over an 8 x 8
// area and draw a tile on every visible terrain parcel near the camera.
// ---------------------------------------------------------------------------

// EffectType 2 "02_RAIN".
struct SPRain : SPEffect {                                            // 0x48, ctor 0x6ea820, vftable 0x88417c
    SPRain();
    ~SPRain();                                                        // 0x6eaa80 (0x6eab70 deleting)
    bool Init(SPropertyStruct* ts, const char* name) override;        // +0x0c 0x6eac50
    SEffect* CreateInstance(SIScene* scene, float param) override;    // +0x14 0x6eabd0

    STrackFloat NumTrack;    // +0x14 "RainDropNum" (drops per tile over Duration)
    int   Texture;           // +0x24 "effects/media/<Texture>", -1
    int   Variations;        // +0x28
    unsigned Color;          // +0x2c (not drawn)
    int   VB;                // +0x30 dynamic VB, FVF 0x102 (XYZ|TEX1), -1
    float Duration;          // +0x34
    float InvVariations;     // +0x38 1 / Variations
    float Alpha;             // +0x3c (not drawn)
    float VertSpeed;         // +0x40
    float VertSpeedRnd;      // +0x44
};

struct SRain : SEffect {                                              // 0x42c, ctor 0x6ea860, vftable 0x884198
    SRain(SIScene* scene, SPRain* proto);
    ~SRain();                                                         // +0x00 0x6eab20 (0x6eaba0 deleting)
    void Slot_14(int p) override;                                     // +0x14 0x6eb5b0 intensity (float bits)
    bool Process() override;                                          // +0x1c 0x6eaeb0
    void Render(SIViewport* vp) override;                             // +0x20 0x6eb170
    void Stop() override { Stopped = true; }                          // +0x2c 0x6eb5d0

    float* Drops[16];        // +0x44 per tile 400 x {x, y, z, fall speed} (0x1900 bytes)
    unsigned TileFrame[16];  // +0x84 last processed frame
    int   TileCount[16];     // +0xc4 live drops
    SIScene* Scene;          // +0x104
    SPRain* P;               // +0x108
    bool  Stopped;           // +0x10c
    int   Cursor;            // +0x110
    bool  UseTrack;          // +0x114 (1) the intensity follows NumTrack until +0x14
    float Time;              // +0x118
    float Intensity;         // +0x11c drops per tile
    int   Alive;             // +0x120
    SRenderPass Pass;        // +0x124 (0x308)
};

// EffectType 3 "03_SNOWFALL".
struct SPSnowfall : SPEffect {                                        // 0x48, ctor 0x6eb5e0, vftable 0x8842ac
    SPSnowfall();
    ~SPSnowfall();                                                    // 0x6eb940 (0x6eba40 deleting)
    bool Init(SPropertyStruct* ts, const char* name) override;        // +0x0c 0x6ebb20
    SEffect* CreateInstance(SIScene* scene, float param) override;    // +0x14 0x6ebaa0

    STrackFloat NumTrack;    // +0x14 "SnowFlakeNum"
    int   Texture;           // +0x24
    int   Variations;        // +0x28
    unsigned Color;          // +0x2c (not drawn)
    int   VB;                // +0x30 dynamic VB, FVF 0x1c4 (XYZRHW|DIFFUSE|SPECULAR|TEX1), -1
    float Duration;          // +0x34
    float InvVariations;     // +0x38
    float Alpha;             // +0x3c (not drawn)
    float VertSpeed;         // +0x40
    float VertSpeedRnd;      // +0x44
};

// One flake (0x2c).
struct SSnowFlake {
    float Pos[3];            // +0x00 x, y (above the terrain), z in the tile
    float Fall;              // +0x0c
    float Swing[2];          // +0x10 x / z sway amplitude 0.2 .. 0.4
    float Phase[2];          // +0x18
    float PhaseSpeed[2];     // +0x20 2 .. 4 rad/s
    int   Variation;         // +0x28 0..3 (texture strip column)
};

struct SSnowfall : SEffect {                                          // 0x7ac, ctor 0x6eb620, vftable 0x8842c8
    SSnowfall(SIScene* scene, SPSnowfall* proto);
    ~SSnowfall();                                                     // +0x00 0x6eb9e0 (0x6eba70 deleting)
    void Slot_14(int p) override;                                     // +0x14 0x6ec570
    bool Process() override;                                          // +0x1c 0x6ebd90
    void Render(SIViewport* vp) override;                             // +0x20 0x6ec0c0
    void Stop() override { Stopped = true; }                          // +0x2c 0x6ec590

    // HD keeps [4][32] tables (+0x44 flakes, +0x244 frames) and uses
    // columns 0..3 only; the counts (+0x444) are [16].
    SSnowFlake* Flakes[16];
    unsigned TileFrame[16];
    int   TileCount[16];
    SIScene* Scene;          // +0x484
    SPSnowfall* P;           // +0x488
    bool  Stopped;           // +0x48c
    int   Cursor;            // +0x490
    bool  UseTrack;          // +0x494
    float Time;              // +0x498
    float Intensity;         // +0x49c
    int   Alive;             // +0x4a0
    SRenderPass Pass;        // +0x4a4
};

// EffectType 1 "01_FLARE": a lens glow drawn by SPixie::Render after the
// effects (weatherfx.cpp, agent M6-WX). Every instance is also in the pixie
// flare heap (+0x3c) and fades with an occlusion query.
struct SPFlare : SPEffect {                                           // 0x38, ctor 0x6df880, vftable 0x883ccc
    SPFlare();
    ~SPFlare();                                                       // 0x6dfa00 (deleting)
    bool Init(SPropertyStruct* ts, const char* name) override;        // +0x0c 0x6dfc60
    SEffect* CreateInstance(SIScene* scene, float param) override;    // +0x14 0x6dfb30

    int   Texture;           // +0x14 -1
    int   Variations;        // +0x18
    unsigned Color;          // +0x1c
    float InvVariations;     // +0x20
    float Alpha;             // +0x24
    float AlphaExp;          // +0x28
    float Scale;             // +0x2c
    float ScaleExp;          // +0x30
    bool  Directional;       // +0x34
};

struct SFlare : SEffect {                                             // 0x36c, ctor 0x6df7a0, vftable 0x883ce8
    SFlare(SPFlare* proto, int flareIndex);
    ~SFlare();                                                        // +0x00 0x6df8b0 (0x6df9d0 deleting)
    bool Process() override;                                          // +0x1c 0x6dfea0
    void Stop() override;                                             // +0x2c 0x6e08a0 deletes the flare
    // SFlare's own slots, called by SPixie::Render 0x69ea50.
    float* BeginBatch(SIViewport* vp, int texture);                   // +0x34 0x6e00c0
    void Draw(SIViewport* vp, const float* camera, float* vb);        // +0x38 0x6e0150
    void EndBatch(SIViewport* vp, int texture);                       // +0x3c 0x6e0100
    void IssueOcclusionQuery(SIViewport* vp);                         // +0x40 0x6dfeb0
    void ReleaseQuery();                                              // +0x44 0x6dfc40
    void CreateQuery();                                               // +0x48 0x6dfc10

    int   FlareIndex;        // +0x44 pixie flare heap index
    int   Variation;         // +0x48
    SPFlare* P;              // +0x50
    bool  Pending;           // +0x35 (HD keeps it in the SEffect padding): Active this frame, not drawn yet
    SRenderPass Pass;        // +0x54
    int   ExpectedPixels;    // +0x35c (2 * screen size)^2, 1 when off screen (not read)
    int   QueryState;        // +0x360 0 idle, 2 issue next, 1 issued
    float Visibility;        // +0x364 visible pixels / 45, at most 1 (1)
    void* Query;             // +0x368 IDirect3DQuery9 (D3DQUERYTYPE_OCCLUSION), null when unsupported
};

// HD 0x6ee4d0: add a camera shake offset to the root viewport's eye offset
// (+0xc0; SViewport::SetCamera adds it and clears it).
void ViewportAddCameraOffset(SIViewport* vp, const float* offset);

} // namespace pz

#endif // PZ_EFFECT_H
