// src/world/pzunitregistry.h
// SUnitRegistry (HD 0x124 bytes, global HD 0x929a4c), the map's unit
// definitions (UNTD) and the typed variable loader. OWNER: agent U (M2;
// lifted by agent D in M1). Lifted from the HD exe.
//
// The registry entries hold the HD unit prototypes (pz::SPUnit family,
// src/game/punit.h), loaded from the .unit property trees
// (src/game/unitprops.h).

#ifndef PZ_UNITREGISTRY_H
#define PZ_UNITREGISTRY_H

#include <stddef.h>
#include "pz/pzcommon.h"
#include "string2.h"
#include "punit.h"

struct SStream;
struct SProperties;

namespace pz {

struct SUnitDef;

// HD unit class types (Unit.Common.ClassType; switch in LoadUnitFiles).
enum EUnitClass {
    UC_SINGLE = 0,           // Single Unit (new 0x14c, 0x5a4ae0)
    UC_3 = 3,                // new 0x178, 0x5a4a20
    UC_4 = 4,                // new 0x14c, 0x5a4ae0
    UC_SQUAD = 5,            // Panzers Squad Unit (new 0x14c, 0x5a49e0)
    UC_SQUAD_MEMBER = 6,     // Panzers Squad Member Unit (new 0x140, 0x5a49b0)
    UC_7 = 7,                // new 0x144, 0x5a4db0 ("Waster Tank Mine")
    UC_8 = 8,                // new 0x140, 0x5a4980
    UC_BUILDING = 9,         // Building Unit (new 0x160, 0x5a4900)
    UC_10 = 10,              // new 0x14c, 0x5a4b30
};

// Recompile-only legacy declaration: the M1 stand-in prototype. Nothing
// creates it any more (the registry holds SPUnit); it stays declared because
// agent L's stand-in code path (triggersunits.h, compiled but never taken
// once the heap holds SUnit) names its fields. No definitions exist.
struct SUnitType {
    int      ClassType;
    int      UnitType;
    SString  Name;
    int      AnimationType;
};

// HD registry entry (SDArray element 0x20).
struct SUnitRegistryEntry {
    SString    Name;              // +0x00 file name without extension ("US Sherman")
    SString    FileName;          // +0x08 file name with extension
    SString    Path;              // +0x10 dir + file name (sort key)
    SPUnit*    Type;              // +0x18 prototype (CreatePUnit by Unit.Common.ClassType)
    bool       Enabled;           // +0x1c
    bool       InGame;            // +0x1d LoadUnitFiles param 2 ("units/ingame/")
    unsigned char _1e[2];
};

struct SUnitRegistry {
    SUnitRegistryEntry* Entries;  // +0x00 SDArray {array, size, max}
    int  Count;                   // +0x04
    int  Max;                     // +0x08
    // unitvariables.ini [Game] constants (0x5cfe30), ranges already * 0.5.
    float DamageBulletToUnarmoured;    // +0x0c
    float DamageBulletToArmoured;      // +0x10
    float DamageBulletToBuilding;      // +0x14
    float DamageATToInfantry;          // +0x18
    float DamageATToBuilding;          // +0x1c
    float DamageHEToInfantry;          // +0x20
    float DamageHEToArmoured;          // +0x24
    float DamageHEToBuilding;          // +0x28
    float DamageFireToUnarmoured;      // +0x2c
    float DamageFireToArmoured;        // +0x30
    float DamageFireToBuilding;        // +0x34
    float ThermoOut;                   // +0x38
    float ThermoIn;                    // +0x3c
    float ThermoDecrease;              // +0x40
    int   XpLevel[4];                  // +0x44 XpLevel_1..4 (SUnit::GetRank 0x5b9e60)
    int   SquadHpLevel[5];             // +0x54
    int   HearingRange[5];             // +0x68
    float SquadDamageBonus[5];         // +0x7c
    float CrewDamageBonus[5];          // +0x90
    int   PriceGrenade;                // +0xa4
    int   PriceMolotov;                // +0xa8
    int   PriceBoat;                   // +0xac
    int   PriceTankMine;               // +0xb0
    int   PriceMineDetector;           // +0xb4
    int   PriceExplosives;             // +0xb8
    int   PriceMagneticMine;           // +0xbc
    int   PriceBinoculars;             // +0xc0
    int   GrenadeMaxRange[5];          // +0xc4
    int   CarriedMine[5];              // +0xd8
    int   MineDetectorRange[4];        // +0xec
    float ExplosivesDamageBonus[4];    // +0xfc
    int   BinocularsRange;             // +0x10c (SUnit::GetSightRange 0x5ba240)
    int   RainHearing;                 // +0x110
    int   AllUnitsMaxNumber;           // +0x114
    int   TankMaxNumber;               // +0x118
    int   ArtilleryMaxNumber;          // +0x11c
    int   SupportMaxNumber;            // +0x120

    SUnitRegistry();                                        // 0x5cfe30
    ~SUnitRegistry();                                       // 0x5d0c10
    void LoadUnitFiles(const char* dir, bool inGame);       // 0x5d1050 (exported)
    SPUnit* GetPUnit(const char* name, bool load);          // 0x5d0e70
    void RemoveEntry(int index);                            // 0x5d1c40
};
PZ_HD_SIZE(SUnitRegistry, 0x124);

extern SUnitRegistry* g_UnitRegistry;   // HD 0x929a4c

// HD SUnitDef: one UNTD property list (0x80 bytes, ctor 0x5cfb10, loaded by
// gLoadVariables 0x670440 with the descriptor table at HD 0x8ddb48).
struct SUnitDef {
    SString  ClassName;           // +0x00
    int      Player;              // +0x08
    int      XP;                  // +0x0c
    float    Pos[2];              // +0x10 x, z
    float    Yrel;                // +0x18
    float    Dir;                 // +0x1c
    float    HP;                  // +0x20
    float    Ammo;                // +0x24
    float    FrontArmor;          // +0x28
    float    LeftSideArmor;       // +0x2c
    float    RightSideArmor;      // +0x30
    float    BackArmor;           // +0x34
    int      Behavior;            // +0x38
    int      GlobalState;         // +0x3c GlobalState_ActiveState
    int      Slots[2];            // +0x40
    bool     Stored;              // +0x48 set by CreateUnit for stored units
    unsigned char _49[3];
    SUnitDef* StoredUnits;        // +0x4c SDArray<SUnitDef> {array, size, max}
    int      StoredCount;         // +0x50
    int      StoredMax;           // +0x54
    SString  ScriptID;            // +0x58
    int      AIGroup;             // +0x60
    float    Cargo;               // +0x64
    bool     FirstKill;           // +0x68
    bool     FirstBlood;          // +0x69
    bool     FirstShot;           // +0x6a
    bool     FirstVehicleLost;    // +0x6b
    bool     FirstArmouredVehicleKill; // +0x6c
    unsigned char _6d[3];
    SUnitDef* TowedUnits;         // +0x70
    int      TowedCount;          // +0x74
    int      TowedMax;            // +0x78
    int      StoredSpecial;       // +0x7c

    SUnitDef();                   // 0x5cfb10
    ~SUnitDef();
    void Load(SStream* s);        // 0x5cfbd0 -> gLoadVariables(s, this, 0x8ddb48)
};
#if defined(_M_IX86)
static_assert(sizeof(SUnitDef) == 0x80, "UNTD stride 0x80");
static_assert(offsetof(SUnitDef, StoredUnits) == 0x4c, "0x5e2da0 puVar2[0x13]");
static_assert(offsetof(SUnitDef, TowedCount) == 0x74, "0x5e2da0 puVar2[0x1d]");
static_assert(offsetof(SUnitDef, ScriptID) == 0x58, "descriptor ScriptID +0x58");
static_assert(sizeof(SUnitRegistryEntry) == 0x20, "registry stride 0x20");
#endif

// HD gLoadVariables 0x670440: generic typed variable list (type, name,
// value; type 0 ends). desc is HD's 6-dword descriptor table.
struct SVarDesc {
    const char* Name;
    int Type;           // 1 bool, 2 int, 3 float, 4 vec3, 5 SString, 6 member class,
                        // 7 int array, 8 vec2, 9 dequeue, 10 typed array, 11 typed dequeue, 12 word
    int Offset;
    const SVarDesc* Members;   // type 6 / element type 6: nested descriptor
    int ElemSize;
    int ElemType;
};
void LoadVariables(SStream* s, void* obj, const SVarDesc* desc);   // 0x670440
void LoadSingleVariable(SStream* s, void* dst, int type, const SVarDesc* members);   // 0x670300
void SkipSingleVariable(SStream* s, int type);                     // 0x670e10

// Recompile-only legacy declaration of the M1 stand-in unit (see SUnitType
// above): the heap holds pz::SUnit (src/game/unit.h). No definitions exist.
struct SMenuUnit {
    SUnitType* Type;
    int        Player;
    float      Pos[3];
    float      Dir;
    bool       Stored;
    struct SIModel* Model;
    int        Members[16];
    int        MemberCount;
};

} // namespace pz

// The unit classes (the heap holds pz::SUnit*).
#include "unit.h"

#endif // PZ_UNITREGISTRY_H
