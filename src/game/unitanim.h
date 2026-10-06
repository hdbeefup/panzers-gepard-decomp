// src/game/unitanim.h
// pz::SUnitAnimation and pz::SPUnitAnimation (0x5c6300..0x5cfb00) and the
// running gear (0x5a9c70..0x5aa900). OWNER: agent A.
//
// The unit keeps its animation at SUnit +0x14. Order per 20 Hz tick (HD
// SGameLogic::Refresh 0x576d80): unit +0x16c StoreInterpolationState (the
// models' +0x3c copy the pose of the tick to the "previous" slot), +0x2c
// ServerRefresh (drivers move the unit), then +0x3c RefreshModel, which calls
// the animation's +0x08 UpdateModel: it moves and poses the models for the
// new tick. The renderer interpolates between the two poses with the scene
// +0x20 factor (SModel flags 1 = pose, 2 = node transforms and texture
// animations, 4 = skeletal time); there is no other interpolation hook.
//
// Classes lifted (menu): SVehicleAnimation (Sherman, M36, M7, Bedford, M2A1,
// jeep), SWalkerAnimation (squad members, hero, crews), SSquadAnimation (the
// squad unit itself), SBuildingAnimation, their SP* prototypes and the
// SPRunningGear / SRunningGear pair (tracks and wheels).
//
// Unit and prototype fields are read at their HD offsets (UnitField /
// PUnitField below). This is the contract with agent U: SUnit and the SP*Unit
// prototypes must keep these fields at the offsets listed here.

#ifndef PZ_GAME_UNITANIM_H
#define PZ_GAME_UNITANIM_H

#include "iunitanim.h"
#include "punit.h"

struct SProperties;

namespace pz {

struct SIDriver;
struct SIPixie;
struct SRunningGear;
struct SPRunningGear;

// ---------------------------------------------------------------------------
// HD containers (layout only).

template <class T>
struct SHdDArray {           // SDArray<T>: data, count, capacity (grow 16, then * 6 / 5)
    T*  Data;
    int Count;
    int Max;
};

struct SHdStr {              // SString: buffer, length
    char* buf;
    int   len;
};
const char* HdStr(const SHdStr& s);
void HdStrSet(SHdStr* s, const char* text, int len = -1);
void HdStrFree(SHdStr* s);

// ---------------------------------------------------------------------------
// SUnit fields used by the animations (contract with agent U). Offsets from
// the start of the object (the SIUnit vptr).

template <class T>
inline T& UnitField(const SIUnit* u, unsigned off)
{
    return *reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(const_cast<SIUnit*>(u)) + off);
}

enum EUnitAnimField : unsigned {
    kUnitPUnit         = 0x004,  // SIPUnit*
    kUnitModel         = 0x008,  // SIModel* main model (anim InitModel / UpdateModel)
    kUnitModel2        = 0x00c,  // SIModel* second model (follows the unit, dir + PI)
    kUnitMarker        = 0x010,  // SIModel* marker (shown with the unit)
    kUnitAnim          = 0x014,  // SIUnitAnimation*
    kUnitActiveDriver  = 0x028,  // int, -1 none
    kUnitDrivers       = 0x038,  // SIDriver** (SDArray data)
    kUnitDriverCount   = 0x03c,  // int
    kUnitGunnerSlot    = 0x044,  // int, walker "%s_gunner%d_fight%d%s"
    kUnitGunners       = 0x048,  // SGunner** (SDArray data)
    kUnitGunnerCount   = 0x04c,  // int
    kUnitFlag6D        = 0x06d,  // bool (vehicle 0x5c2a60, empty)
    kUnitVehicle       = 0x078,  // int, world index of the unit carrying this walker, -1
    kUnitYRel          = 0x088,  // float, height above ground (buildings)
    kUnitPos           = 0x08c,  // float[3] x, y, z (y written by the animations)
    kUnitPrevPos       = 0x098,  // float[3]
    kUnitDir           = 0x0b0,  // float
    kUnitPrevDir       = 0x0b4,  // float
    kUnitMoveSpeed     = 0x0c8,  // float, distance of this tick
    kUnitReverse       = 0x0cc,  // bool
    kUnitTurnSpeed     = 0x0d0,  // float
    kUnitSteer         = 0x0d4,  // float (running gear: steering wheels)
    kUnitGlobalState   = 0x0e0,  // int (walker state table index)
    kUnitNextState     = 0x0e4,  // int
    kUnitBehavior      = 0x0e8,  // int (+0x12c SetBehavior)
    kUnitStateChanging = 0x0f0,  // bool, counter +0xf4
    kUnitStartChange   = 0x0f1,  // bool
    kUnitChangeTicks   = 0x0f4,  // int
    kUnitMoveMode      = 0x0f8,  // int: 1 stand, 2 move, 4 spin right, 5 spin left, 6 changing
    kUnitPlayer        = 0x0fc,  // int
    kUnitSpotted110    = 0x110,  // bool
    kUnitHealth        = 0x114,  // float, 0..1
    kUnitDieBoom       = 0x151,  // bool
    kUnitDieFire       = 0x152,  // bool
    kUnitStartFight    = 0x154,  // bool
    kUnitStartDie      = 0x155,  // bool
    kUnitDying         = 0x156,  // bool
    kUnitDieTicks      = 0x15c,  // int
    kUnitForcedSeq     = 0x160,  // SString (+0x164 length)
    kUnitUnplaced      = 0x168,  // bool
    kUnitAlwaysShown   = 0x169,  // bool
    kUnitSubState      = 0x25c,  // SString (+0x260 length)
    kUnitBreakTicks    = 0x2f0,  // int
    kUnitSpotted       = 0x32c,  // bool, "enemy spotted" said
    kUnitSpottedFrame  = 0x330,  // int
    kUnitDead          = 0x150,  // bool (wreck model)
    kUnitEffects       = 0x320,  // int* pixie effects of the unit (count +0x324)
    kUnitEffectCount   = 0x324,
    kUnitBuildingY     = 0x358,  // float (SBuildingUnit: y of the units inside)
    kUnitBuildingModel = 0x448,  // SIModel* (SBuildingUnit, class 9)
    kUnitOccupants     = 0x16c,  // SBuildingUnit: SDArray of {unit index, ?} (count +0x170)
    kUnitOccupantCount = 0x170,
};

// SP*Unit prototype fields.
template <class T>
inline T& PUnitField(const SIPUnit* p, unsigned off)
{
    return *reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(const_cast<SIPUnit*>(p)) + off);
}

enum EPUnitAnimField : unsigned {
    kPUnitAnimation  = 0x04,  // SPUnitAnimation*
    kPUnitClassType  = 0x40,  // int (0 single, 5 squad, 6 member, 9 building, ...)
    kPUnitUnitType   = 0x44,  // int (0x13 parachutist)
    kPUnitGunCount   = 0x4c,  // unsigned char
    kPUnitModelProto = 0x50,  // int, Gepard model prototype
    kPUnitName       = 0x60,  // SString
    kPUnitCrewSlots  = 0xd4,  // int; vehicles look for crew nodes "man%d_*" up to 5 * this
};

// SGunner fields (unit +0x48).
enum EGunnerAnimField : unsigned {
    kGunnerYaw    = 0x30,     // float, turret angle
    kGunnerPitch  = 0x48,     // float, barrel angle
    kGunnerRecoil = 0x74,     // float
};

// ---------------------------------------------------------------------------
// World services used by the animations. unitanim_world.cpp binds them to
// g_World / g_GameLogic; the animview test tool binds its own.

struct SUnitAnimEnv {
    unsigned* (*Seed)();                                 // &World+0x7518 (the world random seed)
    float     (*TerrainHeight)(float x, float z);        // SWorld::GetTerrainHeight 0x5e7730
    int       (*LocalPlayer)();                          // World+0x16c
    bool      (*NoFogOfWar)();                           // World+0x4d0
    int       (*PlayerTeam)(int player);                 // World+0x17c + player * 0x48
    bool      (*HasGameLogic)();                         // DAT_008f2078 != 0
    bool      (*CanSeeGroundUnit)(int player, SIUnit* u);// SGameLogic 0x562760
    int       (*Frame)();                                // SGameLogic::GetFrame 0x56d1a0
    SIUnit*   (*GetUnit)(int index);                     // World+0x4d4 SHeapTRB
    int       (*GameLogicInt)(unsigned offset);          // an int field of SGameLogic (DAT_008f2078)
    SIPixie*  (*Pixie)();                                // DAT_00929f14
};
extern SUnitAnimEnv g_UnitAnimEnv;

unsigned UnitAnimNextSeed();      // inline LCG of HD: seed = seed * 0x343fd + 0x269ec3
int UnitAnimRand(int n);          // SWorld 0x555a00: (seed >> 16 & 0x7fff) / 32768 * n

// ---------------------------------------------------------------------------
// Property reader. HD reads a typed property tree (SPropertyStruct, the
// "Animation" multi sub-struct of the .unit). The recompile may also read
// the flat .unit keys ("Unit.Animation.Vehicle.TrackWidth"); both go through
// this reader.

struct SAnimProps {
    ::SPropertyStruct* Tree;      // HD tree
    ::SProperties*     Flat;      // flat .unit file
    char               Prefix[192];
    static float (*ResolveSymbol)(const char* value);   // flat mode: non-numeric values (units.ini constants)

    static SAnimProps FromTree(::SPropertyStruct* tree);
    static SAnimProps FromFlat(::SProperties* file, const char* prefix);   // section "Unit"
    bool  Valid() const { return Tree || Flat; }
    float GetFloat(const char* name) const;                  // 0x665c00
    int   GetInt(const char* name) const;                    // 0x665e50
    int   GetEnum(const char* name) const;                   // 0x665950
    const char* GetString(const char* name) const;           // 0x666360
    int   GetMultiIndex(const char* name) const;             // 0x665fc0
    SAnimProps GetMultiSub(const char* name) const;          // 0x666190
    int   GetArraySize(const char* name) const;              // 0x6654a0
    SAnimProps GetArrayItem(const char* name, int i) const;  // 0x665370
};

// ---------------------------------------------------------------------------
// Prototypes (SPUnitAnimation family, HD vftable 0x7fd724).

struct SPUnitAnimation : SIPUnitAnimation {
    SPUnitAnimation();
    ~SPUnitAnimation() override;
    void Load(SIPUnit* punit, ::SPropertyStruct* props) override;
    void LoadResources(SIPUnit* punit, ::SPropertyStruct* props) override;
    void Slot_0C() override;
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override;

    // Recompile: the loaders on the property reader (the virtuals above wrap
    // the HD tree into one).
    virtual void LoadProps(SIPUnit* punit, const SAnimProps& props);
    virtual void LoadResourcesProps(SIPUnit* punit, const SAnimProps& props);

    int StateCount() const { return StateNames.Count; }
    const char* StateName(int i) const;
    int FindState(const char* name) const;                   // 0x5c7ed0 (0 if missing)

    SHdDArray<SHdStr> StateNames;   // +0x04 global state names ("normal", "kneel", ...)
};

struct SPVehicleAnimation : SPUnitAnimation {          // 0x3c, vftable 0x7fd73c
    SPVehicleAnimation();                                    // 0x5c6610
    ~SPVehicleAnimation() override;                          // 0x5c72c0
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override; // 0x5c7a30
    void LoadProps(SIPUnit* punit, const SAnimProps& props) override;          // 0x5c85c0
    void LoadResourcesProps(SIPUnit* punit, const SAnimProps& props) override; // 0x5ca690

    SPRunningGear* RunningGear;     // +0x10
    float SpringStrength;           // +0x14
    float SpringDecay;              // +0x18 1 - SpringDecay
    float SpringScale;              // +0x1c
    float RodSpringStrength;        // +0x20
    float RodSpringDecay;           // +0x24 1 - RodSpringDecay
    float RodSpringScale;           // +0x28
    float MaxSpringAngle;           // +0x2c radians
    float MaxSpringAngle2;          // +0x30 squared
    int   SmokeFx;                  // +0x34 Ground_Dark_Slow_Size2.fx (health < 0.6)
    int   HeavySmokeFx;             // +0x38 Ground_Dark_Slow_Size3.fx (health < 0.3)
};

struct SWalkerStateInfo {           // SPWalkerAnimation +0x10 element (0x24)
    int   IdleCount;                // +0x00 "%s_idle%d"
    int   RelaxCount;               // +0x04 "%s_idle_relax%d"
    int   FightCount;               // +0x08
    int   DieBoomCount;             // +0x0c
    int   DieFireCount;             // +0x10
    int   DieCount;                 // +0x14
    int   StressCount;              // +0x18
    float MoveSpeed;                // +0x1c
    float SpinSpeed;                // +0x20
};

struct SPWalkerAnimation : SPUnitAnimation {           // 0x28, vftable 0x7fd754
    SPWalkerAnimation();                                     // 0x5c6690
    ~SPWalkerAnimation() override;                           // 0x5c7310
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override; // 0x5c7ab0
    void LoadProps(SIPUnit* punit, const SAnimProps& props) override;          // 0x5c86e0
    void LoadResourcesProps(SIPUnit* punit, const SAnimProps& props) override; // 0x5ca980
    int CountSequences(const char* name, int modelProto, int state);           // 0x5c8130

    SHdDArray<SWalkerStateInfo> States;   // +0x10
    int ShadowTexture;                    // +0x1c (-1)
    int LeftOarProto;                     // +0x20 units/walker/oar_left.4d
    int RightOarProto;                    // +0x24
};

struct SPSquadAnimation : SPUnitAnimation {            // 0x10, vftable 0x7fd7b4
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override; // 0x5c7930
    void LoadProps(SIPUnit* punit, const SAnimProps& props) override;          // 0x5c85b0 (empty)
};

struct SPBuildingAnimation : SPUnitAnimation {         // 0x10, vftable 0x7fd7cc
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override; // 0x5c7770
    void LoadProps(SIPUnit* punit, const SAnimProps& props) override;          // 0x5c8470 (empty)
    void LoadResourcesProps(SIPUnit* punit, const SAnimProps& props) override; // 0x5ca620
};

// Prototypes of the animation types that no menu unit uses. They are loaded
// at startup with every .unit file; their animations (STrainAnimation,
// SBoatAnimation, SFlyingAnimation, SProjectileAnimation, SWasterAnimation)
// are not lifted: CreateAnimation logs and returns nullptr.
struct SPProjectileAnimation : SPUnitAnimation {       // 0x10, vftable 0x7fd76c
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override; // 0x5c78e0 (not lifted)
    void LoadProps(SIPUnit* punit, const SAnimProps& props) override;          // 0x5c85a0 (empty)
};

struct SPWasterAnimation : SPUnitAnimation {           // 0x10, vftable 0x7fd784
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override; // 0x5c7b20 (not lifted)
    void LoadProps(SIPUnit* punit, const SAnimProps& props) override;          // 0x5c8be0 (empty)
};

struct SPFlyingAnimation : SPUnitAnimation {           // 0x34, vftable 0x7fd79c
    SPFlyingAnimation();                                     // 0x5c6540
    ~SPFlyingAnimation() override;                           // 0x5c71e0
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override; // 0x5c77c0 (not lifted)
    void LoadProps(SIPUnit* punit, const SAnimProps& props) override;          // 0x5c8480

    SPRunningGear* RunningGear;     // +0x10 (same block as SPVehicleAnimation +0x10..+0x30)
    float SpringStrength;           // +0x14
    float SpringDecay;              // +0x18
    float SpringScale;              // +0x1c
    float RodSpringStrength;        // +0x20
    float RodSpringDecay;           // +0x24
    float RodSpringScale;           // +0x28
    float MaxSpringAngle;           // +0x2c
    float MaxSpringAngle2;          // +0x30
};

struct SPTrainAnimation : SPVehicleAnimation {         // 0x3c, vftable 0x7fa9ac
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override; // 0x5c7990 (not lifted)
};

struct SPBoatAnimation : SPVehicleAnimation {          // 0x3c, vftable 0x7fa9c4
    SIUnitAnimation* CreateAnimation(SIUnit* unit) override; // 0x5c76d0 (not lifted)
};

// SPUnit 0x5a6ce0: the "Animation" multi of the unit (1 vehicle, 2 walker,
// 3 projectile, 4 squad, 5 building, 6 waster, 7 flying, 8 train, 9 boat)
// creates the prototype and loads it. U stores the result at SPUnit +0x04.
SPUnitAnimation* CreatePUnitAnimation(int animationType);
SPUnitAnimation* LoadPUnitAnimation(SIPUnit* punit, const SAnimProps& unitCommon);

// ---------------------------------------------------------------------------
// Animations (SUnitAnimation family, HD vftable 0x7fd7e4, base 0x24 bytes).

struct SAnimGun {                   // vehicle +0x2c / walker +0x28 element (0x18)
    int  H;                         // +0x00 node "gun%dh"
    int  V;                         // +0x04 node "gun%dv"
    int* S;                         // +0x08 nodes "gun%ds%d" (recoil)
    int* M;                         // +0x0c nodes "gun%dm%d" (muzzle)
    int  SCount;                    // +0x10
    int  MCount;                    // +0x14
};

struct SAnimNodeSlot {              // base +0x18 element (8): crew node, kind
    int Node;
    int Kind;                       // 1 driver, 2 sitgun, ... 0xc rightboat
};

struct SUnitAnimation : SIUnitAnimation {
    SUnitAnimation(SPUnitAnimation* proto, SIUnit* unit);    // 0x5c67d0 (draws the world seed twice)
    ~SUnitAnimation() override;                              // 0x5c74c0
    void InitModel(SIModel* model) override;
    void UpdateModel() override;
    void Slot_0C() override;
    void Slot_10() override;
    void Slot_14() override;
    float GetStateMoveSpeed(int state) override;             // 0x5c7e90
    float GetStateTurnSpeed(int state) override;             // 0x5c7f50
    int GetDriverNode() override;                            // 0x5c7c60
    void Slot_24() override;
    void Slot_28() override;
    void Slot_2C() override;
    void Slot_30() override;
    void Slot_34() override;
    void Slot_38() override;
    int GetShadowTexture() override;                         // 0x5c8250
    SIPUnitAnimation* GetPrototype() override;               // 0x5c8240

    SIModel* Model() const { return UnitField<SIModel*>(Unit, kUnitModel); }
    void GetStandText(SHdStr* out, const int* state);        // 0x5c7f90 GetGlobalStateStandText_From_Number
    void GetChangingText(SHdStr* out);                       // 0x5c7d90 GetGlobalStateChangingText
    const char* MoveText();                                  // 0x5c8050 "%s_move%s"
    const char* StandText();                                 // 0x5c8270 "%s_stand%s"
    int StateChangeTicks();                                  // 0x5c7c80
    void PlayGlobalStand(bool blend);                        // 0x5cae80 (SUnit 0x5b7390 calls it)

    SIUnit*          Unit;          // +0x04
    SPUnitAnimation* Proto;         // +0x08
    int              Type;          // +0x0c 1 vehicle, 2 walker, 6 squad, 7 building
    float            SpeedFactor;   // +0x10 0.78..1.22, from two seed draws
    int              Timer;         // +0x14 ticks to the next idle animation
    SHdDArray<SAnimNodeSlot> Slots; // +0x18
};

struct SVehicleAnimation : SUnitAnimation {             // 0xc0, vftable 0x7fd82c
    SVehicleAnimation(SPVehicleAnimation* proto, SIUnit* unit);   // 0x5c68e0
    ~SVehicleAnimation() override;                           // 0x5c6e30 / 0x5c7510
    void InitModel(SIModel* model) override;                 // 0x5c93a0
    void UpdateModel() override;                             // 0x5cd020
    int GetDriverNode() override;                            // 0x5c7c70

    SPVehicleAnimation* VProto;     // +0x24
    SRunningGear* Gear;             // +0x28
    SAnimGun*     Guns;             // +0x2c (punit +0x4c entries)
    int           _30;              // +0x30
    int           BodyNode;         // +0x34 "body"
    int           AntennaNode;      // +0x38 "antenna"
    int           SmokeEffect;      // +0x3c pixie effect, -1
    int           HeavySmokeEffect; // +0x40
    int           _44;              // +0x44
    double        Spring[2];        // +0x48 body sway (x, z)
    double        SpringVel[2];     // +0x58
    double        Rod[2];           // +0x68 antenna sway
    double        RodVel[2];        // +0x78
    float         AntennaPrev[2];   // +0x88 antenna node x, z of the last tick
    bool          AntennaReset;     // +0x90
    unsigned char _91[3];
    int           DriverNode;       // +0x94 "built0_driver"
    int           HookNode;         // +0x98 "hook"
    int           HoleFNode;        // +0x9c "hole_f"
    int           HoleRNode;        // +0xa0 "hole_r"
    float         HookPos[2];       // +0xa4 -x, -z of the node
    float         HoleFPos[2];      // +0xac
    float         HoleRPos[2];      // +0xb4
    int           _bc;              // +0xbc
};

struct SWalkerAnimation : SUnitAnimation {              // 0x34, vftable 0x7fd94c
    SWalkerAnimation(SPWalkerAnimation* proto, SIUnit* unit);     // 0x5c7ab0
    ~SWalkerAnimation() override;                            // 0x5c6f50 / 0x5c7540
    void InitModel(SIModel* model) override;                 // 0x5ca110
    void UpdateModel() override;                             // 0x5ce2a0
    float GetStateMoveSpeed(int state) override;             // 0x5c7ea0
    float GetStateTurnSpeed(int state) override;             // 0x5c7f60
    int GetShadowTexture() override;                         // 0x5c8260
    void AdvanceByDistance();                                // 0x5cafc0
    void ResetRelax();                                       // 0x5cb0a0
    float TimeFactor() const { return Oar ? 1.0f : SpeedFactor; }

    SPWalkerAnimation* WProto;      // +0x24
    SAnimGun*     Guns;             // +0x28
    int           RelaxTimer;       // +0x2c
    SIModel*      Oar;              // +0x30 boat oar model, attached to "R Arm03" / "L Arm03"
};

struct SSquadAnimation : SUnitAnimation {               // 0x2c, vftable 0x7fda6c
    SSquadAnimation(SPSquadAnimation* proto, SIUnit* unit);       // 0x5c7930
    ~SSquadAnimation() override;                             // 0x5c73f0
    void InitModel(SIModel* model) override;                 // 0x5c9340 (empty)
    void UpdateModel() override;                             // 0x5cc8c0

    SPSquadAnimation* SProto;       // +0x24
    SIModel*      Extra;            // +0x28 (released in the dtor)
};

struct SBuildingAnimation : SUnitAnimation {            // 0x30, vftable 0x7fdab4
    SBuildingAnimation(SPBuildingAnimation* proto, SIUnit* unit); // 0x5c7770
    ~SBuildingAnimation() override;                          // 0x5c7100
    void InitModel(SIModel* model) override;                 // 0x5c8c20
    void UpdateModel() override;                             // 0x5cb650

    SPBuildingAnimation* BProto;    // +0x24
    int           RoofNode;         // +0x28 "teto"
    int           InteriorNode;     // +0x2c "belso"
};

// ---------------------------------------------------------------------------
// Running gear (tracks and wheels).

struct SWheelType {                 // SPRunningGear +0x20 element (0x10)
    int   Type;                     // 0 texture wheel (rotates the UVs), 1 node about z, 2 node + steering
    float InvRadius;                // 1 / Radius
    float U;                        // texture rotation centre
    float V;                        // 1 - V
};

struct SPRunningGear {                                 // 0x24, vftable SPRunningGear
    SPRunningGear();                                         // 0x5a9c70
    virtual ~SPRunningGear();
    void Load(const SAnimProps& props);                      // 0x5aa030

    int         WheelTypeCount;     // +0x04
    float       TrackWidth;         // +0x08 half distance between the tracks (* 0.005)
    bool        Caterpillar;        // +0x0c
    unsigned char _0d[3];
    SHdStr      TrailTexture;       // +0x10
    float       TrailWidth;         // +0x18 (* 0.005)
    float       TrailTextureMovement; // +0x1c belt texture scroll per unit of track distance
    SWheelType* WheelTypes;         // +0x20
};

struct SWheelState {                // SRunningGear +0x34 element (0x20)
    int    LeftNode;                // +0x00 "wheel%dl"
    int    _04;
    double LeftAngle;               // +0x08
    int    RightNode;               // +0x10 "wheel%dr"
    int    _14;
    double RightAngle;              // +0x18
};

struct SRunningGear {                                  // 0x38, vftable SRunningGear
    explicit SRunningGear(SPRunningGear* proto);             // 0x5a9cc0 SRunningGear::Init
    virtual ~SRunningGear();                                 // 0x5a9f90
    void InitModel(SIModel* model);                          // 0x5aa180
    void Update(SIModel* model, float x, float z, double dir, float steer);   // 0x5aa2c0

    int    LeftTrail;               // +0x04 scene trail, -1
    int    RightTrail;              // +0x08
    int    LeftBelt;                // +0x0c node "cpbelt0l"
    float  LeftPrev[2];             // +0x10 left track x, z of the last tick
    float  LeftScroll;              // +0x18
    int    RightBelt;               // +0x1c node "cpbelt0r"
    float  RightPrev[2];            // +0x20
    float  RightScroll;             // +0x28
    bool   HasPrev;                 // +0x2c
    unsigned char _2d[3];
    SPRunningGear* Proto;           // +0x30
    SWheelState*   Wheels;          // +0x34 (Proto->WheelTypeCount)
};

} // namespace pz

#endif // PZ_GAME_UNITANIM_H
