// src/world/world.h
// pz::SWorld: the Panzers world (HD 0x7538 bytes, vftable 0x7ffef8, 5 slots).
// OWNER: agent D. Lifted from the HD exe only (the S.W.I.N.E. decomp's world/
// and game/ code is private and is never used here).
//
// Layout: only the fields the menu path (M1) reads or writes are named; they
// sit at their HD offsets (checked below). Everything else stays padding
// named after its offset, so the object keeps the HD size.

#ifndef PZ_WORLD_H
#define PZ_WORLD_H

#include <stddef.h>
#include "pz/pzcommon.h"
#include "string2.h"
#include "doodad.h"

struct SStream;
struct SProperties;

namespace pz {

struct SIViewport;
struct SITerrain;
struct SUnit;

// HD SHeap<T> (0x14 bytes): {array, size, max, free head, live count}.
// Each element is {int Next; T Data;}; Next == kHeapLive marks a live slot,
// otherwise it links the free list.
enum { kHeapLive = 0x7fffffff };

template <typename T>
struct SHeapElem {
    int Next;
    T Data;
};

template <typename T>
struct SHeap {
    SHeapElem<T>* Array;   // +0x00
    int Size;              // +0x04
    int Max;               // +0x08
    int Free;              // +0x0c
    int Count;             // +0x10
    bool IsLive(int i) const { return i >= 0 && i < Size && Array[i].Next == kHeapLive; }
};

// HD SHeapTRB (units, World+0x4d4): elements {int Next; SUnit* Unit}, plus
// the free-list tail and a reuse barrier (a freed slot is only reused after
// a number of frames).
struct SUnitHeap {
    struct Elem { int Next; SUnit* Unit; };
    Elem* Array;           // +0x00 (World+0x4d4)
    int Size;              // +0x04
    int Max;               // +0x08
    int Free;              // +0x0c free head
    int FreeTail;          // +0x10
    int Count;             // +0x14
    unsigned Frame;        // +0x18 reuse barrier: current frame
    unsigned ReuseDelay;   // +0x1c reuse barrier: frames a slot stays free
    bool IsLive(int i) const { return i >= 0 && i < Size && Array[i].Next == kHeapLive; }
};

// HD SDArray<T> (0x0c bytes): {array, size, max}.
template <typename T>
struct SHdArray {
    T* Array;
    int Size;
    int Max;
};

// HD terrain layer (TLAY 0x5efda0, element 0x14).
struct STerrainLayer {
    SString Name;          // +0x00
    SString Extra;         // +0x08 (attr & 2), "99 Regi" for attr & 8
    unsigned Attributes;   // +0x10
};

// HD SWeather (WTHR 0x5f0080 / SWeather::Load 0x5f1860, element 0x58):
// name plus up to 20 light parameters.
struct SWeather {
    SString Name;          // +0x00
    int Lite[20];          // +0x08
};

// pack(4): the double at +0x68 would make MSVC pad the vfptr to 8 bytes.
#pragma pack(push, 4)
struct SWorld {
    explicit SWorld(int p1);                                   // 0x5d2f90
    virtual ~SWorld();                                         // +0x00 HD 0x5d68b0 (scalar deleting dtor) -> 0x5d5510
    virtual void Slot_04();                                    // +0x04 HD 0x5ec2a0
    virtual void Slot_08();                                    // +0x08 HD 0x5fec80
    virtual void Slot_0C();                                    // +0x0c HD 0x5ec0c0
    virtual void Slot_10();                                    // +0x10 HD 0x5ebee0

    void ShowLoadingIcon(int parentFrame);                     // 0x5edca0 "menu/panzers_loading_icons_hq.tga"
    void HideLoadingIcon();                                    // 0x5dc7d0
    void SetLoadingStep(int step);                             // 0x5fedb0 (board +0x24, then a loading frame)
    bool LoadMap(SStream* stream, bool p2, int p3, int p4);    // 0x5f1990 (MAPF v201 chunk loop)
    void Initialize();                                         // 0x5eec90
    void ComputeCamera(SIViewport* vp);                        // 0x5ddc30

    // Map chunk readers (mapload.cpp).
    void LoadTerrain(SStream* s);                              // 0x5f2fc0 TERR
    void LoadTerrainLayers(SStream* s);                        // 0x5efda0 TLAY
    void LoadEntities(SStream* s);                             // 0x5f2a50 ENTS
    void LoadDoodads(SStream* s);                              // 0x5f0500 DODS
    void RemoveDoodad(int index);                              // 0x5f73f0
    void ClearDoodads();                                       // 0x5dd530
    void LoadDecals(SStream* s);                               // 0x5efcd0 DECS
    void RemoveDecal(int index);                               // 0x5f7170
    void ClearDecals(int newSize);                             // 0x5dc890
    void LoadEffects(SStream* s);                              // 0x5f08a0 EEFS
    void LoadRoads(SStream* s);                                // 0x5f0a30 ROD2
    void ClearRoads();                                         // 0x5dd9a0
    void LoadRoadJunctions(SStream* s);                        // 0x5f0b60 RODJ
    void ClearRoadJunctions();                                 // 0x5dd9f0
    void SetTerrainLayers(bool invalidate);                    // 0x608360
    void RebuildTerrain();                                     // 0x6043a0
    void RefreshModels();                                      // 0x576d80 subset (per tick)
    void ClearEffects();                                       // 0x5dd880
    void LoadUnitDefinitions(SStream* s);                      // 0x5f33f0 UNDS
    void LoadUnits(SStream* s);                                // 0x5f3820 UNIS (saved games; logs only)
    void LoadWeathers(SStream* s);                             // 0x5f0080 WTHR
    void ClearWeathers(int newSize);                           // 0x5dd010
    void SetDefaultWeather(int index);                         // 0x5fdaf0
    void SetWeather(int index, int blend);                     // 0x5fdc80
    void LoadPlayers(SStream* s);                              // 0x5f2ed0 PLY3
    void LoadCamera(const float* cam);                         // 0x5fd7c0 CAM

    // Units (unit.cpp).
    int  CreateUnit(struct SUnitDef* def);                     // 0x5e2da0
    int  AllocUnitSlot();                                      // 0x5d94b0
    void RemoveUnit(int index);                                // 0x5f8060
    SUnit* GetUnit(int index);

    // Camera and terrain queries.
    void ResetCamera();                                        // 0x5ecc20
    void SetCameraLimits(bool p1);                             // 0x5efb40
    void SetCameraAngles(float yaw, float pitch);              // 0x5f83b0
    float GetTerrainHeight(float x, float z);                  // 0x5e7730
    float GetWaterHeight(float x, float z);                    // 0x5ec490

    // +0x000 vptr
    unsigned char _004[4];
    int           Param;                // +0x008 ctor argument
    float         CamXMin;               // +0x00c
    float         CamXMax;               // +0x010
    float         CamZMin;               // +0x014
    float         CamZMax;               // +0x018
    float         CamPitchMin;           // +0x01c
    float         CamPitchMax;           // +0x020 (0 = no terrain clamp)
    float         CamDistMin;            // +0x024
    float         CamDistMax;            // +0x028
    float         CamNear;               // +0x02c ctor 1.0
    float         CamFar;                // +0x030 ctor 100.0
    bool          CamProjectionDirty;    // +0x034
    unsigned char _035[3];
    float         CamTarget[3];          // +0x038 x, focus height, z
    float         CamYaw;                // +0x044
    float         CamYawSin;             // +0x048
    float         CamYawCos;             // +0x04c
    float         CamPitch;              // +0x050
    float         CamDist;               // +0x054
    float         CamSmoothTarget[3];    // +0x058
    float         CamSmoothDist;         // +0x064
    double        CamTime;               // +0x068 seconds of the last ComputeCamera
    float         CamEye[3];             // +0x070
    float         CamForward[3];         // +0x07c
    float         CamRight[3];           // +0x088
    float         CamUp[3];              // +0x094
    int           CamFollowPath;         // +0x0a0
    int           CamFollowUnit;         // +0x0a4 (-1 = none)
    int           CamLocked;             // +0x0a8
    int           CamMode;               // +0x0ac 0 normal, 1 free fall, 2 cinematic
    unsigned char _0b0[0x0dc - 0x0b0];   // mode 1 state
    int           TerrainW;              // +0x0dc tiles
    int           TerrainH;              // +0x0e0
    SITerrain*    Terrain;               // +0x0e4 scene +0x64 CreateTerrain
    float*        Heights;               // +0x0e8 STerrain+0x70 (HMAP)
    float*        AltHeights;            // +0x0ec STerrain+0x74
    bool          UseAltHeights;         // +0x0f0
    unsigned char _0f1[3];
    float*        Diffuse;               // +0x0f4 STerrain+0x7c (DIFF)
    unsigned char* Blend;                // +0x0f8 STerrain+0x78 (BLND, 16 per vertex)
    void*         TileMap;               // +0x0fc STerrain+0xb0 (TMAP)
    float*        WaterHeights;          // +0x100 STerrain+0x8c
    SHdArray<STerrainLayer> Layers;      // +0x104 TLAY
    int           BoardIconSet;          // +0x110 board +0x74 "menu/selection_hq.tga" (not loaded in M1)
    int           BoardFrames[4];        // +0x114
    int           Insignia;              // +0x124
    int           MultiInsignia;         // +0x128
    int           SelectionTextures[4];  // +0x12c Gepard +0x44
    bool          _13c;                  // +0x13c
    unsigned char _13d[3];
    SHeap<SDoodad> Doodads;              // +0x140 (element 200 bytes)
    SProperties*  ObjectsIni;            // +0x154 "objects.ini"
    unsigned char _158[0x170 - 0x158];
    unsigned char Players[12][0x48];     // +0x170 PLY3 (12 player slots)
    unsigned char _4d0[4];
    SUnitHeap     Units;                 // +0x4d4
    SHeap<unsigned char[0x50]> AIGroups; // +0x4f4 AIGP (element 0x54)
    unsigned char _508[0x510 - 0x508];
    int           FlagProto[3];          // +0x510 german / US / russian hero flag
    int           _51c;                  // +0x51c -1
    int           RangeTexture;          // +0x520 "shaders\\10 Misc-Egyeb\\range_a"
    unsigned char _524[0x538 - 0x524];
    SHdArray<SWeather> Weathers;         // +0x538
    int           CurrentWeather;        // +0x544
    float         WeatherBlend;          // +0x548
    float         WeatherBlendTime;      // +0x54c
    unsigned char _550[0x634 - 0x550];   // weather lights (0x5fdc80)
    int           RainFx;                // +0x634 "Effects/Atmosphere/Rain.fx"
    unsigned char _638[0x644 - 0x638];
    int           SnowFx;                // +0x644 "Effects/Atmosphere/Snowfall.fx"
    unsigned char _648[0x73c4 - 0x648];
    SHdArray<SDecal> Decals;             // +0x73c4 (element 0x1c)
    SHeap<SEffectSite> Effects;          // +0x73d0 (element 0x2c)
    unsigned char _73e4[0x73fc - 0x73e4];
    SHeap<SMapRoad> Roads;               // +0x73fc ROD2 (element 0x48)
    SHeap<SMapRoadJunction> Junctions;   // +0x7410 RODJ (element 0x68)
    unsigned char _7424[4];
    int           WireTearFx;            // +0x7428 "effects/extras/wiretear.fx"
    unsigned char _742c[0x74bc - 0x742c];
    void*         Minimap;               // +0x74bc MINI bitmap (not decoded in M1)
    int           LoadParam3;            // +0x74c0
    int           LoadParam4;            // +0x74c4
    unsigned char _74c8[0x74d4 - 0x74c8]; // class-name remap table (0x5f4a70)
    bool          LoadAborted;           // +0x74d4
    unsigned char _74d5[3];
    int           LoadIconSet;           // +0x74d8 board +0x70
    int           LoadIconFrame;         // +0x74dc board +0x08
    int           LoadIconParent;        // +0x74e0
    void*         Obj74e4;               // +0x74e4 Initialize: new 0xa4 (0x5a1460, not lifted)
    void*         Obj74e8;               // +0x74e8 Initialize: new 0xe0 (not lifted)
    unsigned*     BlockMap;              // +0x74ec STerrain+0x11c68 (BLCK, 4x resolution)
    void*         BlockMap2;             // +0x74f0 STerrain+0x11c6c
    int           BlockSize;             // +0x74f4 BlockW * BlockH
    int           BlockW;                // +0x74f8 TerrainW * 4
    int           BlockH;                // +0x74fc TerrainH * 4
    unsigned char _7500[0x751c - 0x7500];
    SString       MinimapName;           // +0x751c MINA
    SString       Atmosphere;            // +0x7524 ATMS
    SString       Skybox;                // +0x752c KSYB (0x5fec10)
    unsigned char _7534[0x7538 - 0x7534];
};
#pragma pack(pop)
PZ_HD_SIZE(SWorld, kHdSizeSWorld);

#if defined(_M_IX86)
static_assert(offsetof(SWorld, Param) == 0x008, "HD param_1[2]");
static_assert(offsetof(SWorld, CamNear) == 0x02c, "HD param_1[0xb]");
static_assert(offsetof(SWorld, CamProjectionDirty) == 0x034, "HD (char)param_1[0xd]");
static_assert(offsetof(SWorld, CamTarget) == 0x038, "ComputeCamera +0x38");
static_assert(offsetof(SWorld, CamDist) == 0x054, "ComputeCamera +0x54");
static_assert(offsetof(SWorld, CamTime) == 0x068, "ComputeCamera +0x68");
static_assert(offsetof(SWorld, CamEye) == 0x070, "ComputeCamera +0x70");
static_assert(offsetof(SWorld, CamForward) == 0x07c, "0x5f83b0 +0x7c");
static_assert(offsetof(SWorld, CamUp) == 0x094, "0x5f83b0 +0x94");
static_assert(offsetof(SWorld, CamMode) == 0x0ac, "ComputeCamera +0xac");
static_assert(offsetof(SWorld, TerrainW) == 0x0dc, "TERR +0xdc");
static_assert(offsetof(SWorld, Terrain) == 0x0e4, "TERR +0xe4");
static_assert(offsetof(SWorld, Heights) == 0x0e8, "TERR Acquire arg 1");
static_assert(offsetof(SWorld, Diffuse) == 0x0f4, "TERR Acquire arg 3");
static_assert(offsetof(SWorld, Blend) == 0x0f8, "TERR Acquire arg 4");
static_assert(offsetof(SWorld, WaterHeights) == 0x100, "TERR Acquire arg 6");
static_assert(offsetof(SWorld, Layers) == 0x104, "TERR BLND +0x108");
static_assert(offsetof(SWorld, Doodads) == 0x140, "ENTS DODS +0x140");
static_assert(offsetof(SWorld, ObjectsIni) == 0x154, "HD param_1[0x55]");
static_assert(offsetof(SWorld, Players) == 0x170, "HD memset(param_1 + 0x5c, 0, 0x360)");
static_assert(offsetof(SWorld, Units) == 0x4d4, "UNDS +0x4d4");
static_assert(offsetof(SWorld, AIGroups) == 0x4f4, "ENTS AIGP +0x4f4");
static_assert(offsetof(SWorld, FlagProto) == 0x510, "HD param_1[0x144]");
static_assert(offsetof(SWorld, RangeTexture) == 0x520, "HD param_1[0x148]");
static_assert(offsetof(SWorld, Weathers) == 0x538, "0x5fdaf0 +0x538");
static_assert(offsetof(SWorld, CurrentWeather) == 0x544, "LoadMap WTHR +0x544");
static_assert(offsetof(SWorld, RainFx) == 0x634, "Initialize +0x634");
static_assert(offsetof(SWorld, SnowFx) == 0x644, "Initialize +0x644");
static_assert(offsetof(SWorld, Decals) == 0x73c4, "ENTS DECS +0x73c4");
static_assert(offsetof(SWorld, Effects) == 0x73d0, "dtor param_1[0x1cf4]");
static_assert(offsetof(SWorld, Roads) == 0x73fc, "0x6043a0 +0x73fc");
static_assert(offsetof(SWorld, Junctions) == 0x7410, "0x6043a0 +0x7410");
static_assert(offsetof(SWorld, WireTearFx) == 0x7428, "Initialize +0x7428");
static_assert(offsetof(SWorld, Minimap) == 0x74bc, "LoadMap MINI +0x74bc");
static_assert(offsetof(SWorld, LoadAborted) == 0x74d4, "LoadMap +0x74d4");
static_assert(offsetof(SWorld, LoadIconSet) == 0x74d8, "0x5edca0 +0x74d8");
static_assert(offsetof(SWorld, BlockMap) == 0x74ec, "TERR Acquire arg 7");
static_assert(offsetof(SWorld, BlockW) == 0x74f8, "TERR +0x74f8");
static_assert(offsetof(SWorld, MinimapName) == 0x751c, "dtor param_1[0x1d47]");
static_assert(offsetof(SWorld, Atmosphere) == 0x7524, "LoadMap ATMS +0x7524");
#endif

// Totals for the M1 verification log (recompile only; HD keeps no counters).
struct SWorldLoadStats {
    int Chunks;            // top-level map chunks read
    int SubChunks;         // TERR/ENTS sub chunks read
    int Doodads;           // live DODS entries
    int DoodadModels;      // scene +0x50 CreateModelFromFile calls (doodads + ruins)
    int DoodadModelsOk;    // ... that returned a model
    int Ruins;             // "_rom.4d" ruin models found
    int Decals;
    int Effects;
    int EffectProtos;      // pixie +0x10 LoadEffectPrototype calls
    int UnitDefs;          // UNTD entries in UNDS
    int UnitsCreated;      // top-level units (CreateUnit >= 0)
    int UnitsTotal;        // incl. stored units and squad members
    int UnitModels;        // unit model instances requested
    int UnitModelsOk;
    int Prototypes;        // Gepard +0x20 LoadModelPrototype calls
    int PrototypesOk;
};
extern SWorldLoadStats g_WorldStats;
void LogWorldStats(const char* when);

// Prototype load through Gepard +0x20 with the M1 counters. HD panics on
// failure in some callers; while agent A's loader is a stub it returns -1.
int WorldLoadModelPrototype(const char* file);

} // namespace pz

#endif // PZ_WORLD_H
