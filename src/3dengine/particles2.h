#ifndef DENGINE3_PARTICLES2_H
#define DENGINE3_PARTICLES2_H

#include "igepard.h"
#include "darray.h"

struct SEffect;
struct SGepard;
struct SWorld;

struct SParticles2Data {
    float x;
    float y;
    float z;
    float vectorx;
    float vectorz;
    float yspeed;
    float kisg;
    int tindex;
    bool active;
    float alpha;
    float fadeoutspeed;
    float scale;
};

struct SParticles2 {
    SDArray<int> GroupPrototypes;
    SDArray<int> GroupPrototypeQs;
    SDArray<SParticles2Data> data;
    int DynamicVB;
    int MaxParticles;
    float MainX;
    float MainY;
    float MainZ;
    float X;
    float Y;
    float Z;
    int Darabszam;
    float HSpeed;
    float HSpeed_Rnd;
    float KisG;
    float VSpeed;
    float VSpeed_Rnd;
    int TalajKoszolas;
    float Scale;
    float ScaleRnd;
    unsigned int Color;
    float FadeOutSpeed;
    float FadeOutSpeed_Rnd;
    int THandle;
    int THandle0;
    int THandle1;
    int THandle2;
    int THandle3;
    int THandle4;
    bool MultiTileset;
    SDrawType DrawType;
    bool Prototype;
    char ClassName[120];
    int sz;
    SEffect *Effect;
    SGepard *Gepard;
    SWorld *World;

    SParticles2(char *classname, SEffect *effect, SGepard *gepard);
    SParticles2(SParticles2 *p, float _x, float _y, float _z);
    ~SParticles2();
    char MoveParticles(int a2);
    int PrecacheObject(char *variable_name, SDrawType drawtype);
    void SetPosition(float _x, float _y, float _z);
};

#endif // DENGINE3_PARTICLES2_H
