// 3dengine/glow.h
// SGlow — glow post-processing effect
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef DENGINE3_GLOW_H
#define DENGINE3_GLOW_H

#include "darray.h"
#include "igepard.h"  // for SDrawType

struct SGepard;
struct SEffect;
struct SWorld;

struct SGlowData {
    float x;
    float y;
    float z;
    bool active;
    int whichtexture;
    int scale;
    int r;
    int g;
    int b;
    bool needtofadeout;
    float fadeing;
};

struct SGlow {
    SDArray<SGlowData> data;
    int ManageType;
    int Darabszam;
    float Scale;
    float Variations;
    float RecVariations;
    int THandle;
    SDrawType DrawType;
    char ClassName[120];
    bool FirstTime;
    bool StopNow;
    int sz;
    SEffect* Effect;
    SGepard* Gepard;
    SWorld* World;

    SGlow(char* classname, SEffect* effect, SGepard* gepard);
    ~SGlow();
    void AddGlow(float x, float y, float z, int r, int g, int b, int scale, float fadeing);
    void ClearGlow();
    char DrawParticles();
};

#endif // DENGINE3_GLOW_H
