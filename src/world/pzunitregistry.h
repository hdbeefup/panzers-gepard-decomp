// src/world/pzunitregistry.h
// SUnitRegistry (HD 0x124 bytes, global HD 0x929a4c), its unit types and the
// map's unit definitions (UNTD). OWNER: agent D. Lifted from the HD exe.
//
// M1 scope: the registry (file scan, entries, sorting, side by name prefix,
// GetPUnit) is lifted. The HD unit type classes (SPUnit family, 0x140..0x178
// bytes, ctors 0x5a4900..0x5a4db0, Load through the property tree of agent C's
// SPropertyStruct) are NOT lifted yet: SUnitType below is a reduced stand-in
// that keeps the HD fields the menu needs at their HD offsets and reads the
// model names straight from the .unit SProperties file.

#ifndef PZ_UNITREGISTRY_H
#define PZ_UNITREGISTRY_H

#include <stddef.h>
#include "pz/pzcommon.h"
#include "string2.h"

struct SStream;
struct SProperties;

namespace pz {

struct SUnit;
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

// M1 stand-in for the HD SPUnit family. HD offsets are kept for the fields the
// registry touches (+0x40 class type, +0x48 side, +0x60 name, +0x68/+0x70
// units.ini names, +0xdd loaded). Not HD-sized.
struct SUnitType {
    SUnitType();
    virtual ~SUnitType();                      // HD vtbl +0x00 (scalar deleting dtor)
    virtual void LoadHeader(SProperties* p);    // HD vtbl +0x04 (property tree "Unit")
    virtual void Load(SProperties* p);          // HD vtbl +0x08 (full load: models, weapons, ...)
    virtual SUnit* CreateUnit(int worldIndex);  // HD vtbl +0x10

    unsigned char _04[0x40 - 0x04];
    int      ClassType;           // +0x40 Unit.Common.ClassType
    int      UnitType;            // +0x44 Unit.Common.UnitType
    int      Side;                // +0x48 0 Ge/Hu, 1 US/GB/Fr, 2 SU/Yu, 4 Pl, 6 none
    unsigned char _4c[0x60 - 0x4c];
    SString  Name;                // +0x60
    SString  IniName68;           // +0x68 units.ini section (Building: GetText "Building")
    SString  IniName70;           // +0x70
    unsigned char _78[0xdd - 0x78];
    bool     Loaded;              // +0xdd set by Load
    unsigned char _de[2];

    // M1 data (recompile only, read from the .unit file in Load).
    SString  ModelName;           // Unit.Common.ModelName
    SString  WreckModelName;      // Unit.Common.WreckModelName
    SString  SquadMemberName;     // ...Panzers Squad Unit.SquadMemberName
    int      SquadMaxUnits;       // ...Panzers Squad Unit.MaxNumberOfUnits
    int      AnimationType;       // Unit.Animation (2 walker, 4 squad, 5 building, ...)
    int      ModelProto;          // Gepard +0x20 (ModelName)
    int      WreckProto;          // Gepard +0x20 (WreckModelName)
};

// HD registry entry (SDArray element 0x20).
struct SUnitRegistryEntry {
    SString    Name;              // +0x00 file name without extension ("US Sherman")
    SString    FileName;          // +0x08 file name with extension
    SString    Path;              // +0x10 dir + file name (sort key)
    SUnitType* Type;              // +0x18
    bool       Enabled;           // +0x1c
    bool       InGame;            // +0x1d LoadUnitFiles param 2 ("units/ingame/")
    unsigned char _1e[2];
};

struct SUnitRegistry {
    SUnitRegistryEntry* Entries;  // +0x00 SDArray {array, size, max}
    int  Count;                   // +0x04
    int  Max;                     // +0x08
    unsigned char _0c[0x124 - 0x0c];   // unitvariables.ini [Game] constants (damage, XP, prices; M2)

    SUnitRegistry();                                        // 0x5cfe30
    ~SUnitRegistry();                                       // 0x5d0c10
    void LoadUnitFiles(const char* dir, bool inGame);       // 0x5d1050 (exported)
    SUnitType* GetPUnit(const char* name, bool load);       // 0x5d0e70
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

// M1 stand-in for HD SUnit (115-slot vtables; SSingleUnit, SPanzersSquadUnit,
// SPanzersSquadMemberUnit, SBuildingUnit). Only places the unit's model in the
// scene; no driver, no AI, no Refresh.
struct SUnit {
    explicit SUnit(SUnitType* type, int worldIndex);
    virtual ~SUnit();
    void Initialize(SUnitDef* def);         // HD vtbl +0x08 (model placement only)

    SUnitType* Type;
    int        WorldIndex;
    int        Player;
    float      Pos[3];
    float      Dir;
    bool       Stored;
    struct SIModel* Model;
    int        Members[16];                 // squad members (world unit indices)
    int        MemberCount;
};

} // namespace pz

#endif // PZ_UNITREGISTRY_H
