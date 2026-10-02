#ifndef DENGINE3_PARTICLES4_H
#define DENGINE3_PARTICLES4_H

#include "igepard.h"
#include "darray.h"

struct SEffect;
struct SGepard;
struct SWorld;
struct SIObject;
struct SIConcert;

struct SParticles4Data {
    float x;
    float y;
    float z;
    float mainx;
    float mainz;
    float vectorx;
    float vectorz;
    float yspeed;
    float rotcounter;
    float kisg;
    int tindex;
    bool active;
    float alpha;
    float fadeoutspeed;
    int whichtexture;
    float scale;
    float randomr;
    bool fadein;
};

struct SParticles4 {
    SDArray<int> GroupPrototypes;
    SDArray<int> GroupPrototypeQs;
    SDArray<SParticles4Data> data;
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
    float FadeInSpeed;
    float BorningSpeed;
    float BorningCounter;
    float FadeOutSpeed;
    float FadeOutSpeed_Rnd;
    float RandomR;
    float MainR;
    unsigned int ColorR;
    unsigned int ColorG;
    unsigned int ColorB;
    float StartAlpha;
    float StartTimeMoment;
    float StopTimeMoment;
    float StopTimeDuration;
    float Variations;
    float RecVariations;
    SIObject *Object;
    int MeshIdx;
    char MeshName[200];
    bool Stopping;
    float ScaleSpeed;
    int THandle;
    int SHandle;
    bool BorningState;
    float BorningStateCounter;
    int BorningEnabledTime;
    int BorningDisabledTime;
    int BorningEnabledTimeRND;
    int BorningDisabledTimeRND;
    SDrawType DrawType;
    bool Prototype;
    char ClassName[120];
    bool LensStarted;
    int sz;
    int Race;
    float LensEffectLaunch;
    SEffect *Effect;
    SGepard *Gepard;
    SWorld *World;

    SParticles4(char *classname, SEffect *effect, SGepard *gepard);
    SParticles4(SParticles4 *p, float _x, float _y, float _z, float _scalespeed);
    SParticles4(SParticles4 *p, SIObject *object, char *meshname, int race);
    ~SParticles4();
    void CloneParticles(SParticles4 *p);
    bool MoveParticles(int a2);
    int PrecacheObject(char *variable_name, SDrawType drawtype);
    void RefreshIndices();
    void SetPosition(float _x, float _y, float _z);
    void StopEffect();
};

#endif // DENGINE3_PARTICLES4_H
