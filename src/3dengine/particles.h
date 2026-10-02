#ifndef DENGINE3_PARTICLES_H
#define DENGINE3_PARTICLES_H

#include "igepard.h"
#include "darray.h"

struct SEffect;
struct SGepard;
struct SWorld;
struct SIObject;

struct SParticlesData {
    float x;
    float y;
    float z;
    float xrot;
    float yrot;
    float zrot;
    float zrotspeed;
    float vectorx;
    float vectorz;
    float yspeed;
    float kisg;
    SIObject *group;
    int smoketrailindex;
    bool active;
    float alpha;
    float fadeoutspeed;
};

struct SParticles {
    SDArray<int> GroupPrototypes;
    SDArray<int> GroupPrototypeQs;
    SDArray<int> GroupPrototypes0;
    SDArray<int> GroupPrototypeQs0;
    SDArray<int> GroupPrototypes1;
    SDArray<int> GroupPrototypeQs1;
    SDArray<int> GroupPrototypes2;
    SDArray<int> GroupPrototypeQs2;
    SDArray<int> GroupPrototypes3;
    SDArray<int> GroupPrototypeQs3;
    SDArray<int> GroupPrototypes4;
    SDArray<int> GroupPrototypeQs4;
    SDArray<SParticlesData> data;
    float MainX;
    float MainY;
    float MainZ;
    float X;
    float Y;
    float Z;
    int Darabszam;
    int OsszesDarabszam;
    float HSpeed;
    float HSpeed_Rnd;
    float KisG;
    float ZRotSpeed;
    float ZRotSpeed_Rnd;
    float VSpeed;
    float VSpeed_Rnd;
    int Trail_Color;
    float Trail_Strength;
    float Trail_FadeSpeed;
    float Trail_Scale;
    int TalajKoszolas;
    int BoomAfter;
    bool MultiTileset;
    float FadeOutSpeed;
    float FadeOutSpeed_Rnd;
    float TimeIntervall;
    int ThisMeshRepeating;
    SParticles *Prototypes;
    int LastedTime;
    int TimePeriod;
    float RandomX;
    float RandomY;
    float RandomZ;
    int Race;
    SIObject *Object;
    int MeshIdx;
    int THandle;
    SDrawType DrawType;
    bool Prototype;
    char ClassName[120];
    char MeshName[200];
    int sz;
    SEffect *Effect;
    SGepard *Gepard;
    SWorld *World;

    SParticles(char *classname, SEffect *effect, SGepard *gepard);
    SParticles(SParticles *p, float _x, float _y, float _z);
    SParticles(SParticles *p, SIObject *object, char *meshname, int race);
    ~SParticles();
    void CloneParticles(SParticles *p);
    bool MoveParticles();
    int PrecacheObject(char *variable_name, SDrawType drawtype, int Tileset);
    void RefreshIndices();
    void SetPosition(float _x, float _y, float _z);
};

#endif // DENGINE3_PARTICLES_H
