// src/game/punit.h
// SIPUnit: the HD unit prototype interface (RTTI SPUnit vftable 0x7fa7c0,
// 6 slots; 9 classes) and the prototype classes. OWNER: agent U (the slot
// order follows the shared-header rules of docs/M2_INTERFACES.md).
//
// Prototypes come from the .unit property files (611 types, SUnitRegistry
// 0x5cfe30 / LoadUnitFiles 0x5d1050). Unit.Common.ClassType picks the class
// (operator new size, ctor):
//   0, 4, 0xb..0xd SPSingleUnit 0x14c 0x5a4ae0   3 SPProjectileUnit 0x178 0x5a4a20
//   5 SPPanzersSquadUnit 0x14c 0x5a49e0  6 SPPanzersSquadMemberUnit 0x140 0x5a49b0
//   7 SPWasterUnit 0x144 0x5a4db0        8 SPFlyingUnit 0x140 0x5a4980
//   9 SPBuildingUnit 0x160 0x5a4900     10 SPTrainUnit 0x14c 0x5a4b30
//
// Property system: the registry builds one SUPropStruct instance of the
// .unit schema per file (unitprops.h). LoadUnitFiles hands the root "Unit"
// struct to +0x04 LoadHeader (which also creates the gunner, driver and
// animation prototypes); GetPUnit(name, true) hands a fresh one to +0x08
// LoadResources (models, effects, and the sub-prototypes' resources).
// Units serialise differently (save games, M6): SUnit::Load 0x5bbd30 reads
// the sub-tags "vars", "targ" (STarget, table 0x8dd890) and "driv" through
// gLoadVariables 0x670440 (pz::LoadVariables in pzunitregistry.h). The menu
// creates units only from UNTD definitions and trigger CREATE actions.
//
// Slot comments: "+0xNN HD 0xADDR (N arg dwords)" from the HD vftable and the
// RET imm16 of the implementation (a double counts as 2; "?" = no RET found,
// e.g. a tail jump). "[menu: ...]" = the coverage trace of the original menu
// (docs/re/M2_COVERAGE.md) executed that implementation: "startup" = only while
// the menu loaded, otherwise the steady-loop rate bucket. The hit status is per
// implementation address, so a slot shared by several classes shows the same
// status everywhere. Only hit slots have names; "(name guessed)" names rest on
// the decompiled body or one caller. Slot_XX keep the order (never remove one).
// Override tables: "*" = that override was executed in the menu.

#ifndef PZ_PUNIT_H
#define PZ_PUNIT_H

#include "m2common.h"
#include "string2.h"

namespace pz {

struct SIUnit;
struct SUPropStruct;
struct SIPDriver;
struct SIPUnitAnimation;
struct SPGunner;

struct SIPUnit {
    virtual ~SIPUnit() {}                               // +0x00 HD 0x5a5900 (1 arg dwords) scalar deleting dtor
    virtual void LoadHeader(SUPropStruct* unit) = 0;             // +0x04 HD 0x5a6650 (1 arg dwords) [menu: startup] SPSingleUnit::Init (0x5a62d0); the "Unit" root struct
    virtual void LoadResources(SUPropStruct* unit) = 0;          // +0x08 HD 0x5a8780 (1 arg dwords) [menu: sporadic] SPUnit::LoadResources (models, "PModelIdx < 0")
    virtual void ReleaseResources() = 0;                         // +0x0c HD 0x5a9950 (0 arg dwords) (name guessed) releases every prototype handle LoadResources took, clears +0xdd
    virtual SIUnit* CreateUnit(int worldIndex) = 0;              // +0x10 _purecall (? arg dwords) [menu: periodic<1/s via SPSingleUnit 0x5a5cf0] factory: operator new + the unit ctor
    virtual void LoadGunners(SUPropStruct* unit) = 0;            // +0x14 HD 0x5a7390 (1 arg dwords) [menu: startup] reads "Gunners" (0x5a7390)
};

// Prototype overrides of SPUnit slots:
//   SPSingleUnit               vftable 0x7fa7dc, 6 slots: +0x00 5a57c0 +0x04 5a62d0* +0x10 5a5cf0*
//   SPBuildingUnit             vftable 0x7fa884, 6 slots: +0x00 5a5430 +0x04 5a6050* +0x08 5a7500*
//                                                         +0x0c 5a9730 +0x10 5a5a70*
//   SPPanzersSquadUnit         vftable 0x7fa84c, 6 slots: +0x00 5a5690 +0x04 5a61d0* +0x10 5a5bf0*
//   SPPanzersSquadMemberUnit   vftable 0x7fa868, 6 slots: +0x00 5a5600 +0x08 5a7da0* +0x0c 5a9770
//                                                         +0x10 5a5b70*
//   SPWasterUnit               vftable 0x7fa830, 6 slots: +0x00 5a5960 +0x04 5a6c30* +0x10 5a5df0
//   SPProjectileUnit           vftable 0x7fa814, 6 slots: +0x00 5a5710 +0x04 5a6280* +0x08 5a7e70*
//                                                         +0x0c 5a9790 +0x10 5a5c70
//   SPFlyingUnit               vftable 0x7fa7f8, 6 slots: +0x00 5a5540 +0x04 5a6160* +0x10 5a5af0
//   SPTrainUnit                vftable 0x7fa8a0, 6 slots: +0x00 5a58c0 +0x04 5a6580* +0x10 5a5d70

// HD SDArray<T> {array, size, max} (0x0c bytes).
template <typename T>
struct SUnitArray {
    T*  Array;
    int Size;
    int Max;
};

// One effect slot of the prototype (0x0c bytes): the pixie prototype and the
// mesh (node) name it attaches to.
struct SPUnitEffect {
    int     Proto;       // +0x00 pixie +0x10 LoadEffectPrototype
    SString MeshName;    // +0x04
};

// HD SPUnit base (vftable 0x7fa7c0), ctor 0x5a4b80. The base layout is
// 0x13c bytes (every subclass starts its own fields at +0x13c).
struct SPUnit : SIPUnit {
    SPUnit();                                                    // 0x5a4b80
    ~SPUnit() override;                                          // 0x5a5180
    void LoadHeader(SUPropStruct* unit) override;                // 0x5a6650
    void LoadResources(SUPropStruct* unit) override;             // 0x5a8780
    void ReleaseResources() override;                            // 0x5a9950
    SIUnit* CreateUnit(int worldIndex) override;                 // pure in HD
    void LoadGunners(SUPropStruct* unit) override;               // 0x5a7390
    void InitDrivers(SUPropStruct* unit);                        // 0x5a6f10
    void InitAnimation(SUPropStruct* unit);                      // 0x5a6ce0

    // +0x000 vptr
    SIPUnitAnimation*       PAnimation;     // +0x004 created by 0x5a6ce0 from Unit.Animation
    SUnitArray<SPGunner*>   PGunners;       // +0x008 LoadGunners
    SUnitArray<SIPDriver*>  PDrivers;       // +0x014 InitDrivers
    int      MarketBuyFirst;                // +0x020
    int      MarketBuyLast;                 // +0x024
    int      MarketMulti;                   // +0x028
    int      UnloadDirection;               // +0x02c
    int      MarketPicture;                 // +0x030
    int      HeroPicture;                   // +0x034 >= 0: a hero (flag over the squad, RefreshModel 0x5c6130)
    int      Price;                         // +0x038
    int      Price2;                        // +0x03c (squads: Price / 2)
    int      ClassType;                     // +0x040 Unit.Common.ClassType
    int      UnitType;                      // +0x044 Unit.Common.UnitType
    int      Side;                          // +0x048 0 Ge/Hu, 1 US/GB/Fr, 2 SU/Yu, 4 Pl, 6 none (LoadUnitFiles)
    unsigned char GunnerCount;              // +0x04c
    unsigned char DriverCount;              // +0x04d
    unsigned char _4e[2];
    int      ModelProto;                    // +0x050 Gepard +0x20 (ModelName, 0.005)
    int      LowPolyProto;                  // +0x054 (LowPolyModelName unless GetOption(0xe))
    int      WreckProto;                    // +0x058 (WreckModelName)
    int      Proto5C;                       // +0x05c -1 (released by 0x5a9950)
    SString  Name;                          // +0x060 registry entry name
    SString  IniName68;                     // +0x068 units.ini name (Building: GetText "Building")
    SString  IniName70;                     // +0x070
    int      _78;                           // +0x078
    int      _7c;                           // +0x07c
    int      Race;                          // +0x080 name prefix: 0 GB, 1 US, 2 GE, 3..7 others
    float    Sight;                         // +0x084 Common.Sight * 0.5
    bool     Detector;                      // +0x088
    bool     Invisible;                     // +0x089
    bool     AlwaysVisible;                 // +0x08a
    bool     Invulnerable;                  // +0x08b
    bool     Selectable;                    // +0x08c
    bool     OnlyWalkerFlag;                // +0x08d CanMoveOnOnlyWalkerFlag
    bool     BmEdgeFlag;                    // +0x08e CanMoveOn_BM_EDGE_Flag
    unsigned char _8f;
    float    UnitSize;                      // +0x090 Common.UnitSize * 0.5
    int      ArmourType;                    // +0x094
    float    HP;                            // +0x098
    float    Thermostat;                    // +0x09c
    float    FrontArmor;                    // +0x0a0
    float    LeftSideArmor;                 // +0x0a4
    float    RightSideArmor;                // +0x0a8
    float    BackArmor;                     // +0x0ac
    float    TopArmor;                      // +0x0b0
    bool     AttackGround;                  // +0x0b4
    bool     SlotGrenade;                   // +0x0b5
    bool     SlotMolotov;                   // +0x0b6
    bool     SlotMagneticMine;              // +0x0b7
    bool     SlotExplosives;                // +0x0b8
    bool     SlotGroundTankMine;            // +0x0b9
    bool     SlotBoat;                      // +0x0ba
    bool     SlotSpade;                     // +0x0bb
    bool     SlotMineDetector;              // +0x0bc
    bool     SlotBinoculars;                // +0x0bd
    bool     SlotMine;                      // +0x0be
    unsigned char _bf;
    int      MainGunner;                    // +0x0c0
    float    FireHeight;                    // +0x0c4 * 0.5
    bool     BuiltInDriver;                 // +0x0c8 (Single/Train Unit)
    unsigned char _c9[3];
    SString  BuiltInDriverUnitName;         // +0x0cc
    int      StorageCapacity;               // +0x0d4
    int      StorageType;                   // +0x0d8
    bool     OnlyCrew;                      // +0x0dc
    bool     Loaded;                        // +0x0dd LoadResources done
    bool     SupportPlace;                  // +0x0de UnitType 0x10, or "Support place (desert)"
    bool     Repairer;                      // +0x0df
    bool     Supporter;                     // +0x0e0
    unsigned char _e1[3];
    int      Cargo;                         // +0x0e4
    bool     IsTrain;                       // +0x0e8 UnitType 0x12 (name guessed)
    unsigned char _e9[3];
    float    _ec;                           // +0x0ec 20.0 (squads 12.5, trains 50.0)
    SUnitArray<SPUnitEffect> StaticEffects;     // +0x0f0
    SUnitArray<SPUnitEffect> PilotLightEffects; // +0x0fc
    SUnitArray<SPUnitEffect> DieEffects;        // +0x108
    SUnitArray<SPUnitEffect> DiedByFireEffects; // +0x114
    SUnitArray<SPUnitEffect> DestroyEffects;    // +0x120
    SUnitArray<SPUnitEffect> NightEffects;      // +0x12c
    int      _138;                          // +0x138
};

// ClassType 0, 4, 0xb, 0xc, 0xd (vftable 0x7fa7dc). Child units: Single Unit
// "Child Units" {UnitName, MeshName} (0x10 bytes each).
struct SPChildUnit {
    SString UnitName;    // +0x00
    SString MeshName;    // +0x08
};

struct SPSingleUnit : SPUnit {
    SPSingleUnit();                                              // 0x5a4ae0
    ~SPSingleUnit() override;                                    // 0x5a57c0
    void LoadHeader(SUPropStruct* unit) override;                // 0x5a62d0
    SIUnit* CreateUnit(int worldIndex) override;                 // 0x5a5cf0

    SUnitArray<SPChildUnit> ChildUnits;     // +0x13c
    int      DemolishType;                  // +0x148
};

struct SPProjectileUnit : SPUnit {
    SPProjectileUnit();                                          // 0x5a4a20
    void LoadHeader(SUPropStruct* unit) override;                // 0x5a6280
    void LoadResources(SUPropStruct* unit) override;             // 0x5a7e70
    SIUnit* CreateUnit(int worldIndex) override;                 // 0x5a5c70

    unsigned char _13c[0x178 - 0x13c];      // incidence effect arrays (0x5a7e70)
};

struct SPPanzersSquadUnit : SPUnit {
    SPPanzersSquadUnit();                                        // 0x5a49e0
    ~SPPanzersSquadUnit() override;                              // 0x5a5690
    void LoadHeader(SUPropStruct* unit) override;                // 0x5a61d0
    SIUnit* CreateUnit(int worldIndex) override;                 // 0x5a5bf0

    unsigned char MaxNumberOfUnits;         // +0x13c
    unsigned char _13d[3];
    SString  SquadMemberName;               // +0x140
    int      Formations;                    // +0x148
};

struct SPPanzersSquadMemberUnit : SPUnit {
    SPPanzersSquadMemberUnit();                                  // 0x5a49b0
    ~SPPanzersSquadMemberUnit() override;                        // 0x5a5600
    void LoadResources(SUPropStruct* unit) override;             // 0x5a7da0
    void ReleaseResources() override;                            // 0x5a9770
    SIUnit* CreateUnit(int worldIndex) override;                 // 0x5a5b70

    int      ParachuteProto;                // +0x13c parachute model (Ge/US/SU Parachute Member), -1
};

struct SPWasterUnit : SPUnit {
    SPWasterUnit();                                              // 0x5a4db0
    void LoadHeader(SUPropStruct* unit) override;                // 0x5a6c30
    SIUnit* CreateUnit(int worldIndex) override;                 // 0x5a5df0

    int      LifeTime;                      // +0x13c ticks
    bool     Sensor;                        // +0x140
    bool     RemoteControl;                 // +0x141
    unsigned char _142[2];
};

struct SPFlyingUnit : SPUnit {
    SPFlyingUnit();                                              // 0x5a4980
    void LoadHeader(SUPropStruct* unit) override;                // 0x5a6160
    SIUnit* CreateUnit(int worldIndex) override;                 // 0x5a5af0

    int      PlaneType;                     // +0x13c
};

// One production slot of a productive building (0x68 bytes, 0x5a7500).
struct SPBuildingProduct {
    unsigned char Data[0x68];
};

struct SPBuildingUnit : SPUnit {
    SPBuildingUnit();                                            // 0x5a4900
    ~SPBuildingUnit() override;                                  // 0x5a5430
    void LoadHeader(SUPropStruct* unit) override;                // 0x5a6050
    void LoadResources(SUPropStruct* unit) override;             // 0x5a7500
    void ReleaseResources() override;                            // 0x5a9730
    SIUnit* CreateUnit(int worldIndex) override;                 // 0x5a5a70

    int      BuildingType;                  // +0x13c 0 house .. 6 hangar (3 capturable)
    bool     BuildingDowngrading;           // +0x140
    unsigned char _141[3];
    int      BuildingMaterial;              // +0x144 (> 1 reads as 0)
    float    CaptureRange;                  // +0x148 * 0.5
    int      FlagProto;                     // +0x14c capturable: flag.4DA
    int      FlagMultiProto;                // +0x150 capturable: flag-multi.4DA
    SUnitArray<SPBuildingProduct> Products; // +0x154 (0x5a5a00)
};

struct SPTrainUnit : SPUnit {
    SPTrainUnit();                                               // 0x5a4b30
    void LoadHeader(SUPropStruct* unit) override;                // 0x5a6580
    SIUnit* CreateUnit(int worldIndex) override;                 // 0x5a5d70

    int      _13c[4];                       // +0x13c
};

PZ_HD_SIZE(SPSingleUnit, kHdSizeSPSingleUnit);
PZ_HD_SIZE(SPProjectileUnit, kHdSizeSPProjectileUnit);
PZ_HD_SIZE(SPPanzersSquadUnit, kHdSizeSPPanzersSquadUnit);
PZ_HD_SIZE(SPPanzersSquadMemberUnit, kHdSizeSPPanzersSquadMemberUnit);
PZ_HD_SIZE(SPWasterUnit, kHdSizeSPWasterUnit);
PZ_HD_SIZE(SPFlyingUnit, kHdSizeSPFlyingUnit);
PZ_HD_SIZE(SPBuildingUnit, kHdSizeSPBuildingUnit);
PZ_HD_SIZE(SPTrainUnit, kHdSizeSPTrainUnit);

// Creates the prototype of a .unit ClassType (the switch of LoadUnitFiles
// 0x5d1050); nullptr for an unknown class type.
SPUnit* CreatePUnit(int classType);

} // namespace pz

// Prototype factories of the other agents. HD SPUnit::InitDrivers 0x5a6f10
// and the "Animation" switch 0x5a6ce0 construct those classes inline
// (operator new + ctor + vtbl +0x04 Load); the recompile asks the owners:
//   P defines PzCreatePDriver (driver.cpp): new SP*Driver by
//     Unit.Drivers.N.Driver.DriverType (0 TurnInPlace, 1 TurnInAngle,
//     3 Flying, 5 Walker, 6 Projectile, 7 Squad, 8 SquadMember, 9 ChildUnit,
//     10 PanzersSquad, 11 PanzersSquadMember, 12 Parachute, 13 Train; 2 and 4
//     log "SPUnit::InitDrivers - Unknown drivertype" and return nullptr), then
//     its Load(driverStruct, unitName). driverStruct is "N.Driver".
// Animation prototypes come from agent A's LoadPUnitAnimation (unitanim.h).
// Until P defines it, a weak default (/alternatename, unitextern.cpp)
// returns nullptr: the unit then has no drivers.
extern "C" pz::SIPDriver* PzCreatePDriver(int driverType, pz::SUPropStruct* driverStruct, const char* unitName);

#endif // PZ_PUNIT_H
