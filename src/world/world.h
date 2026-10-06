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
struct SPzSelectRect;   // selection.h
struct SITerrain;
struct SUnit;         // src/game/unit.h (agent U)
struct SUnitDef;
struct STrigger;

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
// the free-list tail and a reuse barrier. A freed slot stores the frame it
// was freed in (Unit field) and goes to the tail of the free list; Alloc
// (0x5d94b0) takes the head only while Frame <= freed frame + ReuseDelay
// (unsigned compare), otherwise it appends a new slot.
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

// HD map location (LOCS 0x5f0690, SHeap element 0x28 = Next + 0x24). The
// rectangle is in world units, stored as ints; the triggers test
// X1 < x < X2 and Z1 < z < Z2 (0x581ae0, 0x582080). Agent L.
struct SLocation {
    int     X1;            // +0x04 (element offsets)
    int     Z1;            // +0x08
    int     X2;            // +0x0c
    int     Z2;            // +0x10
    SString Name;          // +0x14
    int     SceneObject;   // +0x1c -1 (scene +0x70 removes it, 0x5dd650)
    int     Effect;        // +0x20 -1 (pixie +0x0c removes it)
    int     Color;         // +0x24 (read; editor colour?)
};

// HD map path (PATH 0x5f07b0, SHeap element 0x20). Agent L.
struct SPathPoint { float X, Z; };
struct SPath {
    SString Name;          // +0x04
    int     P0c;           // +0x0c
    bool    Closed;        // +0x10 (name guessed)
    unsigned char _11[3];
    SHdArray<SPathPoint> Points;   // +0x14 (0x5efd30)
};

// HD trigger variable (TVAR 0x56e440, SHeap element 0x14). Every second
// (0x570cc0) Value += Step; trigger actions 3/8 set Value/Step. Agent L.
struct STriggerVariable {
    SString Name;          // +0x04
    int     Value;         // +0x0c SWorld vtbl +4/+8 Get/SetTriggerVariableValue
    int     Step;          // +0x10 added once per second
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
    void LoadLocations(SStream* s);                            // 0x5f0690 LOCS (agent L, trigger.cpp)
    void LoadPaths(SStream* s);                                // 0x5f07b0 PATH (agent L, trigger.cpp)
    void LoadTriggerVariables(SStream* s);                     // 0x56e440 TVAR (agent L, trigger.cpp)

    // Trigger variables (agent L, triggers.cpp). HD vtbl +4 / +8 (Slot_04 /
    // Slot_08 above); the recompile calls these non-virtual bodies. Indices
    // 0x40000014..0x40000027 are the per-player support counters.
    int  GetTriggerVariableValue(int index, bool special);     // 0x5ec2a0
    void SetTriggerVariableValue(int index, int value, bool special);   // 0x5fec80
    void UpdateSpeech();                                       // 0x607f50 (agent L)
    void RefreshBlockMapDirtyRect();                           // 0x604620 (agent L; name guessed)
    void UpdateWaterMap();                                     // 0x608600 water map = height map (+ lakes, rivers)

    // Units (unit.cpp).
    int  CreateUnit(struct SUnitDef* def);                     // 0x5e2da0
    int  AllocUnitSlot();                                      // 0x5d94b0
    void RemoveUnit(int index);                                // 0x5f8060
    SUnit* GetUnit(int index);
    int  CreateUnit(int player, const char* className, const float* pos, float dir, int p5,
                    float hp, int parent, bool crew, const char* scriptId);   // 0x5e3170
    // HD returns the 2-float point through the hidden pointer `out`.
    float* FindEmptySpace(float* out, float x, float z, float dir, int size, unsigned mask,
                          bool units);                         // 0x5e58d0
    float* FindEmptySpaceNear(float* out, float x, float z, float refX, float refZ, int size,
                              unsigned mask, bool units);      // 0x5e5700
    void UnitStored(int unit, int player);                     // 0x5ef760 (StoreUnit: units targeting it stop)

    // Camera and terrain queries.
    void ResetCamera();                                        // 0x5ecc20
    void SetCameraLimits(bool p1);                             // 0x5efb40
    void SetCameraAngles(float yaw, float pitch);              // 0x5f83b0
    float GetTerrainHeight(float x, float z);                  // 0x5e7730
    float GetWaterHeight(float x, float z);                    // 0x5ec490
    // --- M2-I sub-agent BW (SWorld unit rows): add declarations here only.
    // World+0x158 SHeap of doodad animations (element 0x20 = Next + 0x1c):
    // a doodad a unit drove over falls (Kind 0x10) or trembles (Kind 0x20).
    // Filled by CrushDoodad 0x5e3c80, animated by RefreshDoodadAnims (inline in
    // SGameLogic::Refresh 0x576d80).
    struct SDoodadAnim {
        int   Doodad;        // +0x00 World+0x140 index
        int   Kind;          // +0x04 0x10 falls, 0x20 scrub trembles
        float Amplitude;     // +0x08 tilt in degrees: ScrubTrembling, or 0 rising to DemolishAngleMax
        float Velocity;      // +0x0c DemolishAngleVelocity, then += sin(tilt) * Velocity2 per tick
        float Velocity2;     // +0x10 DemolishAngleVelocity
        float Dir;           // +0x14 fall / tremble direction (radians)
        bool  Done;          // +0x18
        unsigned char _19[3];
    };
    SHeap<SDoodadAnim>& DoodadAnims() { return *(SHeap<SDoodadAnim>*)((unsigned char*)this + 0x158); }
    int  AllocDoodadAnim();                                    // 0x5d8b90 SHeap<0x1c>::Alloc on +0x158
    void UnitMoved(int unit, float wantedSpeed);               // 0x5e4870 (drivers, after each move)
    void CrushDoodad(int doodad, float x, float y, float z);   // 0x5e3c80 (unit position by value)
    void RefreshFlyingFox();                                   // 0x5f6bf0
    void RefreshDoodadAnims();                                 // 0x577a10 inline in 0x576d80
    void RefreshWires();                                       // 0x577a47 inline in 0x576d80 (0x605930 per +0x7454)
    // --- end BW
    // --- M2-I sub-agent LG (SWorld rows the logic needs): add declarations here only.
    // --- end LG
    // --- M3 (docs/M3_INTERFACES.md). Placeholders with the HD dword counts
    // (RET n); the owner fixes the parameter types when lifting.
    // Selection and picking: agent O, src/world/selection.cpp (selection.h).
    // A unit is selected when unit +0x104 bit 0 is set (the packet builders
    // send exactly those units, 0x576130); every change of that bit also
    // sends packet 0x32 / 0x33 (select / deselect). `mode` (names guessed):
    // bits 0-1 keep the old selection bit (& mode & 3), bits 2-3 are or-ed
    // into the hit units, bit 4 toggles them; 0x10 = select only the hit,
    // 0x11 / 5 = add (Shift), 0 = clear all. A ray is 6 floats (origin,
    // direction) as viewport +0x34 (0x689c20) returns it.
    int  PickUnitAt(float* ray, unsigned mode);                // 0x5fc050 (2) click: own / allied unit under the ray; returns it (-1)
    void SelectUnitsInBox(const struct SPzSelectRect* box, unsigned mode);   // 0x5fc5b0 (2) drag box (HD: frustum of viewport +0x38)
    void ApplySelectionToAll(unsigned mode);                   // 0x5fc860 (1) (name guessed) mode 0 deselects everything
    void SelectUnit(int unit, unsigned mode);                  // 0x5fcb10 (2) (name guessed)
    void SelectSameType(const struct SPzSelectRect* box, int unit, unsigned mode);   // 0x5fcd10 (3) (name guessed) double click: same prototype in view
    void DrawSelectionBox(int x0, int y0, int x1, int y1);     // 0x5fd630 (4) the 4 board frames +0x114..+0x120
    void HideSelectionBox();                                   // 0x5ddb60 (0)
    void SelectGroup(int group);                               // 0x5fd2f0 (1) (name guessed) key n: units with +0x10c == n; World+0xa0 = n
    void AssignGroup(int group);                               // 0x5e3660 (1) (name guessed) Ctrl+n
    void SelectByClass(unsigned mode, bool vehicles, bool classB, bool squads);   // 0x5fd030 (4) (name guessed) Ctrl+A / S / T
    bool IsUnitSelected(int unit);                             // 0x5ef660 (1)
    int  CountSelectedUnits();                                 // 0x5e0d70 (0)
    int  PickAnyUnitAt(float* ray);                            // 0x5ebac0 (1) (name guessed) any visible unit under the ray (order target)
    void RayTerrain(const float* ray, float* x, float* y, float* z);   // 0x5ea910 (4) (name guessed) first terrain / water hit; x = z = -1 when none
    void ShowUnitRange(int unit);                              // 0x5fee00 (1) SWorld::ShowUnitRange (visual; agent V, worldcamera.cpp)
    void GetCameraState(unsigned* out5);                       // 0x5e6a70 (1) CamTarget x/z, yaw, +0x50, CamDist (replay records it)
    // AI and mission start / load extras: agent L, src/world/ai.cpp.
    // RefreshAI 0x5f5c70 is SAIGroup::Refresh (aigroup.h, M3-C); RefreshAIGroups() runs them all.
    void StartEffects();                                       // 0x5f5b50 (0) (name guessed) mission start: map effects on (+0x73e4, +0x64c)
    void InitCameraSpline(const char* file);                   // 0x609760 (1) SGameWorld::InitCameraSpline (-csplay file, else ""); HD `this` is the SGameWorld global 0x929a60, not the world: worldcamera.cpp forwards (agent V)
    // Camera moves of the game view (agent V, worldcamera.cpp).
    void MoveCamera(float forward, float right);               // 0x5f4dc0 (2) scroll in view space, speed CamDist * 0.075, clamped to CamXMin..CamZMax
    void RotateCamera(float yaw, float pitch);                 // 0x5f8380 (2) SetCameraAngles(CamYaw + yaw, CamPitch + pitch)
    void ZoomCamera(float delta);                              // 0x609390 (1) CamDist += delta, clamped to CamDistMin..Max
    void SetCameraTarget(float x, float z);                    // 0x5f4f60 (2) minimap jump / centre (unless CamLocked)
    void LoadMapExtra_5e2d70();                                // 0x5e2d70 (0) SGameView::LoadMap after Initialize
    void LoadMapExtra_5debb0();                                // 0x5debb0 (0)
    void LoadMapExtra_607ad0();                                // 0x607ad0 (0)
    void FixBridges();                                         // 0x5e65f0 (0) SWorld::FixBridges
    // --- end M3
    // --- M3 C (combat + AI, docs/M3_INTERFACES.md row C). Sub-blocks per C sub-agent:
    //     add declarations only inside your own sub-block.
    // C0 (integrator): unit speech / event announcer, src/game/combat_speech.cpp.
    void UnitSpeech(int unit, int event, bool anyPlayer);      // 0x5fff20 (3) (name guessed) the unit's sound event; draws the world LCG on one branch (M3_INTERFACES §7)
    void InitSpeechCounts();                                   // 0x5edd00 (0) per-nation speech sample counts (+0x66c), speech state reset; end of the ctor
    // C2 (damage / death / XP): src/game/combat.cpp.
    void IncreaseUnitXP(int unit, int victim, float amount);   // 0x5ec840 (3) SWorld::IncreaseUnitXP: unit +0x8c(victim, amount, 0), campaign XP
    // --- C1 (gunner / projectile)
    // --- end C1
    // --- C2 (damage / death / waster)
    void RecordKill(int attacker, int victim);                 // 0x5eca50 (2) campaign kill statistics (combat.cpp)
    // --- end C2
    // --- C3 (air support / parachute / support calls)
    // --- end C3
    // --- C4 (AI / squads / buildings)
    // --- end C4
    // --- C5 (squads / buildings)
    void UnfixBridges();                                       // 0x600f90 (0) +0xf0 = 0 (panics when not set) (buildingunit.cpp)
    void RemoveDoodadIfLive(int index);                        // 0x5f7c00 (1) RemoveDoodad 0x5f73f0 when the slot is live (squadrefresh.cpp)
    void AIGroupUnitAttacked(int unit, int attacker);          // 0x5d68e0 (2) (name guessed) the AI group of `unit` reacts (C4 owns the body; stub in squadunit.cpp)
    // --- end C5
    // --- end M3 C

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
    unsigned char _158[0x16c - 0x158];
    int           LocalPlayer;           // +0x16c the player at this machine (0x5737c0, 0x5638f0)
    unsigned char Players[12][0x48];     // +0x170 PLY3 (12 player slots)
    unsigned char _4d0[4];
    SUnitHeap     Units;                 // +0x4d4
    SHeap<unsigned char[0x50]> AIGroups; // +0x4f4 AIGP (element 0x54)
    unsigned char _508[0x510 - 0x508];
    int           FlagProto[3];          // +0x510 german / US / russian hero flag
    int           RangeUnit;             // +0x51c -1; the unit ShowUnitRange 0x5fee00 draws
    int           RangeTexture;          // +0x520 "shaders\\10 Misc-Egyeb\\range_a"
    int           MinRangeCount;         // +0x524 ShowUnitRange: terrain effect decals of the min range circle
    int           MaxRangeCount;         // +0x528 ... of the max range circle (or the building window arcs)
    int*          MinRangeDecals;        // +0x52c new[MinRangeCount]
    int*          MaxRangeDecals;        // +0x530 new[MaxRangeCount]
    unsigned char _534[0x538 - 0x534];
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
    unsigned char _742c[0x7474 - 0x742c];
    SHdArray<STrigger> Triggers;         // +0x7474 TRIG (0x5f0140, element 0x2c; src/world/trigger.h)
    SHeap<SLocation> Locations;          // +0x7480 LOCS (0x5f0690, element 0x28)
    SHeap<SPath>     Paths;              // +0x7494 PATH (0x5f07b0, element 0x20)
    SHeap<STriggerVariable> TriggerVariables; // +0x74a8 TVAR (0x56e440, element 0x14)
    void*         Minimap;               // +0x74bc MINI bitmap (SHdBitmap, pz/hdbitmap.h; 0x66ea40)
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
    unsigned char _7500[0x7518 - 0x7500];
    unsigned      RandomSeed;            // +0x7518 hashed into the world CRC 0x56aa10 every tick
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
static_assert(offsetof(SWorld, Triggers) == 0x7474, "TRIG 0x5f0140 +0x7474");
static_assert(offsetof(SWorld, RandomSeed) == 0x7518, "0x56aa10 +0x7518");
static_assert(offsetof(SWorld, Locations) == 0x7480, "LoadMap LOCS +0x7480");
static_assert(offsetof(SWorld, Paths) == 0x7494, "LoadMap PATH +0x7494");
static_assert(offsetof(SWorld, TriggerVariables) == 0x74a8, "LoadMap TVAR +0x74a8");
static_assert(sizeof(SHeapElem<SLocation>) == 0x28, "LOCS stride 0x28");
static_assert(sizeof(SHeapElem<SPath>) == 0x20, "PATH stride 0x20");
static_assert(sizeof(SHeapElem<STriggerVariable>) == 0x14, "TVAR stride 0x14");
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
