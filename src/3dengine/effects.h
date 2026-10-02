// 3dengine/effects.h
// SEffect — effects handler
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_EFFECTS_H
#define DENGINE3_EFFECTS_H

#include "chain.h"
#include "darray.h"
#include "igepard.h"  // for SDrawType, SIObject

#include <d3d9.h>

struct SGepard;
struct SProperties;
struct SGlow;
struct SShader2Info;
struct SParticles;
struct SParticles2;
struct SParticles3;
struct SParticles4;

// === Effect sub-types ===

struct SBoom {
    SDArray<int> thandles;
    float framenr;
    float frameinc;
    float x;
    float y;
    float z;
    int drawtype;
    float scale;
};

struct SGhost {
    int thandle;
    float x;
    float y;
    float z;
    float alpha;
    float eltunes_kezdete;
    float eltunes_sebesseg;
    float felfutas_sebesseg;
    float nagyitas_sebesseg;
    float aktualis_meret;
    float felszallas_sebesseg;
    float magassag;
    float kiteres_mertek;
    float kiteres_sebesseg;
    float orix;
    float oriz;
};

struct STT {
    SDArray<int> thandles;
};

struct SLokeshullam {
    int Prototype;
    SIObject *object;
    float scale;
    float scalespeed;
    float intensity;
    float disappearance;
};

struct SLighting {
    char lightingdata[200];
    int lightingFPS;
    float lightingpos;
};

struct SReflektor {
    int thandle;
    SIObject *object;
    int managetype;
    int meshidx;
};

struct SShaderLight {
    SShader2Info *s2i;
    int thandle;
    int thandle0;
    int thandle1;
    int thandle2;
    int thandle3;
    int thandle4;
    SDrawType drawtype;
    float eltunes_kezdete;
    float eltunes_sebesseg;
    float felfutas_sebesseg;
    bool multitileset;
    int forcebright;
};

struct SEsoCsepp {
    float x;
    float y;
    float z;
    float speed;
};

struct SAblak {
    int meshidx;
    int type;
    float param;
    float variable;
    bool active;
};

struct SEffectDesc {
    int type;
    void* userdata;
};

struct SEffectDesc2 {
    SEffectDesc2* next;
    SEffectDesc2* prev;
    int type;
    void* userdata;
    void* effectringofobject;
};

// === SEffect — main effects handler ===

struct SEffect {
    SProperties* EffectsINI;
    SHeap<SEffectDesc> EffectsLoaded;
    SChain<SEffectDesc2> EffectsPlaying;
    SGepard* Gepard;
    IDirect3DDevice9* lpD3DDev;
    int minh;
    int maxh;

    SEffect(SGepard *gepard);
    ~SEffect();

    void AddGlow(SGlow *glow, float x, float y, float z, int r, int g, int b, int scale, float fadeing);
    void ClearGlow(SGlow *glow);
    void CleanUpPointersInCurrentPlayingEffect();
    SBoom* Clone_Boom(SBoom *data, float _x, float _y, float _z);
    SGhost* Clone_Ghost(SGhost *data, float _x, float _y, float _z);
    SLighting* Clone_Lighting(SLighting *data, float _x, float _y, float _z);
    SLokeshullam* Clone_Lokeshullam(SLokeshullam *data, float _x, float _y, float _z);
    SReflektor* Clone_Reflektor(SReflektor *data, SIObject *object);
    SShaderLight* Clone_ShaderLight(SShaderLight *data, float _x, float _y, float _z);
    void DestroyPlayingEffects();
    void Init_Boom(SBoom *data, char *classname);
    void Init_Ghost(SGhost *data, const char *texturename);
    void Init_Lighting(SLighting *data, char *effectname);
    void Init_Lokeshullam(SLokeshullam *data, char *effectname);
    void Init_Reflektor(SReflektor *data, char *effectname);
    void Init_ShaderLight(SShaderLight *data, char *effectname);
    void Init_TT(STT *data, char *classname);
    int LoadEffect(char *effectname, const char *texturename, float _randomx, float _randomz);
    void MoveEffects(SEffectDesc2 *current, int next);
    bool Move_Boom(SBoom *data);
    bool Move_Ghost(SGhost *data);
    char Move_Lighting(SLighting *data);
    char Move_Lokeshullam(SLokeshullam *data);
    char Move_Reflektor(SReflektor *data);
    bool Move_ShaderLight(SShaderLight *data);
    void* PlayEffect(int handle, float _x, float _y, float _z, float _scalespeed);
    void PlayEffect(int handle, float _x1, float _y1, float _z1, float _x2, float _y2, float _z2, float strength, float fade_speed, float u_scale, float v_scale);
    void PlayEffect(int handle, SIObject *object, char *meshname, int race);
    void PlayEffect(int handle, SIObject *object, SDArray<SAblak> *ablakok);
    void PorzasParameterek(SEffectDesc2 *handle, float SzelTime1, bool SzelLokesMode1, float SzelLokesSebesseg1, float SzelLokesEro1, float SzelSpinDir1, float LokalisSzelirany);
    void Reflektor_RefreshIndices(SReflektor *data);
    void RefreshIndices(SParticles **handle);
    void StopEffect(SEffectDesc2 *handle);
    void TrackEffect(SEffectDesc2 *handle, float _x, float _y, float _z, float _borningspeed);
};

// === SEryngo ===

struct SEryngo {
    SEffect *Effect;
    SGepard *Gepard;
    SIObject *Object;
    float LokalisSzelirany;
    float SzelIrany1;
    float SzelIrany1_b;
    float SzelIranyVecX1;
    float SzelIranyVecZ1;
    float SzelEro1;
    float SzelMagassag1;
    float SzelTime1;
    float SzelSpinDir1;
    float SzelLokesEro1;
    float SzelLokesSebesseg1;
    float SzelLokesCounter1;
    bool SzelLokesMode1;
    void *Porzas;

    SEryngo(SEryngo *p, SIObject *object);
    SEryngo(SGepard *gepard, SEffect *effect, char *classname);
    char MoveEryngo();
};

// === SLevelUpLens ===

struct SLevelUpLens {
    SEffect *Effect;
    SGepard *Gepard;
    float MainX;
    float MainY;
    float MainZ;
    int THandle1;
    int THandle2;
    int THandle3;
    float Scale1;
    float Scale2;
    float Scale3;
    float Rotate1;
    float Rotate2;
    float Rotate3;
    float Alpha1;
    float Alpha2;
    bool Prototype;
    bool Stopping;
    SIObject *Object;
    int MeshIdx;
    char MeshName[200];
    int Race;

    SLevelUpLens(SLevelUpLens *p, SIObject *object, char *meshname, int race);
    SLevelUpLens(SGepard *gepard, SEffect *effect, char *classname);
    ~SLevelUpLens();
    void DrawLensLayer(int a2, int a3, float sec, int thandle, float _scale, float *rotate, float rotatespeed, float alpha);
    bool MoveLevelUpLens(int a2, int a3);
    void RefreshIndices();
    void StopEffect();
};

// === SRain ===

struct SRain {
    SGepard *Gepard;
    SEffect *Effect;
    int THandle;
    SEsoCsepp EsoCseppek[100][14];

    SRain(SGepard *gepard, SEffect *effect, char *classname);
    ~SRain();
    char MoveRain();
};

// === SWindowEffect (forward decl, defined elsewhere) ===

struct SWindowEffect {
    SEffect *Effect;
    SGepard *Gepard;
    SIObject *Object;
    SDArray<SAblak> *Ablakok;
    float NextRandom;
    int AblakCounter;
    int AblakHang;
    int NagyajtoHang;

    SWindowEffect(SWindowEffect *p, SIObject *object, SDArray<SAblak> *ablakok);
    SWindowEffect(SGepard *gepard, SEffect *effect, char *classname);
    char MoveWindows();
};

// === SSnowfall ===

struct SHopehely {
    float x;
    float y;
    float z;
    float speed;
    int alak;
    float sinvaluex;
    float sinvaluez;
    float sinspeedx;
    float sinspeedz;
    float sindevx;
    float sindevz;
};

struct SSnowfall {
    SGepard *Gepard;
    SEffect *Effect;
    float Variations;
    float RecVariations;
    int THandle;
    SHopehely Hopelyhek[100][160];

    SSnowfall(SGepard *gepard, SEffect *effect, char *classname);
    ~SSnowfall();
    char MoveSnow();
};

// Vertex types used in local scopes
struct TLVertEffect {
    float x, y, z, rhw;
    union { unsigned int diffuse; unsigned int color; };
    float u, v;
};

struct TLVertRain {
    float x, y, z;
    float u, v;
};

// Concert (SIConcert) — full definition in sound/iconcert.h
#include "iconcert.h"
extern SIConcert *Concert;

#endif // DENGINE3_EFFECTS_H
