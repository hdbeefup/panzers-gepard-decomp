#ifndef DENGINE3_PARTICLES3_H
#define DENGINE3_PARTICLES3_H

#include "igepard.h"
#include "darray.h"

struct SEffect;
struct SGepard;
struct SWorld;
struct SIObject;

struct SParticles3Data {
    float x;
    float y;
    float z;
    float vectorx;
    float vectorz;
    float yspeed;
    float kisg;
    int tindex;
    float alpha;
    float fadeoutspeed;
    int whichtexture;
    float scale;
};

struct SParticles3 {
    SDArray<int> GroupPrototypes;
    SDArray<int> GroupPrototypeQs;
    SDArray<SParticles3Data> data;
    int DynamicVB;
    int MaxParticles;
    float MainX;
    float MainY;
    float MainZ;
    float X;
    float Y;
    float Z;
    int ManageType;
    int Darabszam;
    float HSpeed;
    float HSpeed_Rnd;
    float KisG;
    float VSpeed;
    float VSpeed_Rnd;
    int TalajKoszolas;
    float Scale;
    float BorningSpeed;
    float BorningCounter;
    float FadeOutSpeed;
    float FadeOutSpeed_Rnd;
    float RandomX;
    float RandomZ;
    unsigned int Color;
    float StartAlpha;
    float StartTimeMoment;
    float StopTimeMoment;
    float StopTimeDuration;
    float Variations;
    float RecVariations;
    SIObject *Object;
    char MeshName[200];
    int TorkolatMeshIdx;
    bool Stopping;
    float ScaleSpeed;
    int THandle;
    bool BorningState;
    float BorningStateCounter;
    int BorningEnabledTime;
    int BorningDisabledTime;
    int BorningEnabledTimeRND;
    int BorningDisabledTimeRND;
    float LokalisSzelirany;
    float SzelIrany1;
    float SzelIrany2;
    float SzelIranyVecX1;
    float SzelIranyVecZ1;
    float SzelIranyVecX2;
    float SzelIranyVecZ2;
    float SzelEro1;
    float SzelEro2;
    float SzelMagassag1;
    float SzelMagassag2;
    float SzelMagRatioConv1;
    float SzelMagRatioConv2;
    float SzelTime1;
    float SzelTime2;
    float SzelSpinDir1;
    float SzelSpinDir2;
    float SzelLokesEro1;
    float SzelLokesEro2;
    float SzelLokesSebesseg1;
    float SzelLokesSebesseg2;
    float SzelLokesCounter1;
    float SzelLokesCounter2;
    bool SzelLokesMode1;
    bool SzelLokesMode2;
    bool NoWind;
    SDrawType DrawType;
    bool Prototype;
    char ClassName[120];
    int sz;
    SEffect *Effect;
    SGepard *Gepard;
    SWorld *World;

    SParticles3(char *classname, SEffect *effect, SGepard *gepard, float _randomx, float _randomz);
    SParticles3(SParticles3 *p, float _x, float _y, float _z, float _scalespeed);
    SParticles3(SParticles3 *p, SIObject *object, char *meshname);
    ~SParticles3();
    void CloneParticles(SParticles3 *p);
    void GetSzelVector(float *x, float y, float *z);
    bool MoveParticles(int a2, int a3);
    int PrecacheObject(char *variable_name, SDrawType drawtype);
    void RefreshIndices();
    void SetPosition(float _x, float _y, float _z);
    void StopEffect();
};

#endif // DENGINE3_PARTICLES3_H
