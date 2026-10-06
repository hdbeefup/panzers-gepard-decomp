// src/game/punit.cpp
// The unit prototypes SP*Unit (0x5a3810..0x5aa7b0): construction, the .unit
// header and resources, the gunner / driver / animation sub-prototypes.
// OWNER: agent U.

#include <stdlib.h>
#include <string.h>
#include "punit.h"
#include "projectile.h"
#include "flying.h"
#include "waster.h"
#include "m3common.h"
#include "unitprops.h"
#include "gunner.h"
#include "idriver.h"
#include "unitanim.h"
#include "singleunit.h"
#include "squadunit.h"
#include "buildingunit.h"
#include "worldapi.h"
#include "pz/igepardhd.h"
#include "pz/ipixie.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"

namespace pz {

void FreeSString(SString* s);

static void AssignSString(SString* dst, const char* src)
{
    FreeSString(dst);
    *dst = src ? src : "";
}

// HD SDArray::Add (0x550490 for the 0x0c effect slots): grows by 6/5 (at
// least 16) and returns the new index.
template <typename T>
static int ArrayAdd(SUnitArray<T>* a)
{
    if (a->Size == a->Max) {
        int nmax = a->Max < 0x10 ? 0x10 : (a->Max * 6) / 5;
        a->Array = (T*)realloc(a->Array, nmax * sizeof(T));
        memset((void*)&a->Array[a->Max], 0, (nmax - a->Max) * sizeof(T));
        a->Max = nmax;
    }
    return a->Size++;
}

// ---------------------------------------------------------------------------
// SPUnit

// PANZERS 0x5a4b80
SPUnit::SPUnit()
{
    memset((unsigned char*)this + sizeof(void*), 0, sizeof(SPUnit) - sizeof(void*));
    ModelProto = LowPolyProto = WreckProto = Proto5C = -1;   // param_1[0x14..0x17]
    MarketBuyFirst = MarketBuyLast = -1;                      // [8], [9]
    MarketPicture = HeroPicture = -1;                         // [0xc], [0xd]
    ClassType = -1;                                           // [0x10]
    Race = -1;                                                // [0x20]
    StorageType = -1;                                         // [0x36]
    _138 = -1;                                                // recompile: Unit.Animation index (M1 visuals)
}

// PANZERS 0x5a5180
SPUnit::~SPUnit()
{
    if (Loaded)
        SPUnit::ReleaseResources();
    for (int i = 0; i < PGunners.Size; ++i) {
        delete PGunners.Array[i];
        PGunners.Array[i] = nullptr;
    }
    for (int i = 0; i < PDrivers.Size; ++i) {
        delete PDrivers.Array[i];                             // vtbl +0 (1)
        PDrivers.Array[i] = nullptr;
    }
    if (PAnimation) {
        delete PAnimation;
        PAnimation = nullptr;
    }
    SUnitArray<SPUnitEffect>* effects[] = { &StaticEffects, &PilotLightEffects, &DieEffects,
                                            &DiedByFireEffects, &DestroyEffects, &NightEffects };
    for (SUnitArray<SPUnitEffect>* e : effects) {             // 0x54fd00
        for (int i = 0; i < e->Size; ++i)
            FreeSString(&e->Array[i].MeshName);
        free(e->Array);
        e->Array = nullptr;
        e->Size = e->Max = 0;
    }
    FreeSString(&BuiltInDriverUnitName);
    FreeSString(&IniName70);
    FreeSString(&IniName68);
    FreeSString(&Name);
    free(PDrivers.Array);
    PDrivers.Array = nullptr;
    PDrivers.Size = PDrivers.Max = 0;
    free(PGunners.Array);
    PGunners.Array = nullptr;
    PGunners.Size = PGunners.Max = 0;
}

// HD: two-letter prefix test of the unit name (0x5336b0 + _stricmp).
static bool NamePrefix(const SString& name, const char* prefix)
{
    return name.size != 0 && _strnicmp(name.buf, prefix, 3) == 0 && strlen(name.buf) >= 3;
}

// PANZERS 0x5a6650
void SPUnit::LoadHeader(SUPropStruct* unit)
{
    SUPropStruct* common = unit->GetStruct("Common");
    PAnimation = nullptr;
    UnitType = common->GetEnum("UnitType");
    MarketPicture = common->GetInt("MarketPicture");
    HeroPicture = common->GetInt("HeroPicture");
    Price = (int)common->GetFloat("Price");                   // 0x767700 (ftol)
    Price2 = Price;
    Sight = common->GetFloat("Sight") * 0.5f;                 // DAT_007f453c
    HP = common->GetFloat("HP");
    FrontArmor = common->GetFloat("FrontArmor");
    LeftSideArmor = common->GetFloat("LeftSideArmor");
    RightSideArmor = common->GetFloat("RightSideArmor");
    BackArmor = common->GetFloat("BackArmor");
    TopArmor = common->GetFloat("TopArmor");
    Thermostat = common->GetFloat("Thermostat");
    ArmourType = common->GetEnum("ArmourType");
    AttackGround = common->GetBool("AttackGround");
    SUPropStruct* slots = common->GetStruct("Slots");
    SlotGrenade = slots->GetBool("Grenade");
    SlotMolotov = slots->GetBool("Molotov coctail");
    SlotMineDetector = slots->GetBool("Mine detector");
    SlotGroundTankMine = slots->GetBool("Ground tank mine");
    SlotMagneticMine = slots->GetBool("Magnetic mine");
    SlotExplosives = slots->GetBool("Explosives");
    SlotSpade = slots->GetBool("Spade");
    SlotBinoculars = slots->GetBool("Binoculars");
    SlotBoat = slots->GetBool("Boat");
    SlotMine = slots->GetBool("Mine");
    MainGunner = common->GetInt("MainGunner");
    FireHeight = common->GetFloat("FireHeight") * 0.5f;
    UnitSize = common->GetFloat("UnitSize") * 0.5f;
    Selectable = common->GetBool("Selectable");
    Detector = common->GetBool("Detector");
    Invulnerable = common->GetBool("Invulnerable");
    Invisible = common->GetBool("Invisible");
    AlwaysVisible = common->GetBool("AlwaysVisible");
    OnlyWalkerFlag = common->GetBool("CanMoveOnOnlyWalkerFlag");
    BmEdgeFlag = common->GetBool("CanMoveOn_BM_EDGE_Flag");
    Repairer = common->GetBool("Repairer");
    Supporter = common->GetBool("Supporter");
    Cargo = (int)common->GetFloat("Cargo");
    MarketBuyFirst = common->GetInt("MarketBuyFirst");
    MarketBuyLast = common->GetInt("MarketBuyLast");
    MarketMulti = common->GetEnum("MarketMultiWhenBuyable");
    UnloadDirection = common->GetEnum("UnloadDirection");
    LoadGunners(unit);                                        // vtbl +0x14
    InitDrivers(unit);                                        // 0x5a6f10
    InitAnimation(unit);                                      // 0x5a6ce0
    if (NamePrefix(Name, "GB "))
        Race = 0;
    else if (NamePrefix(Name, "US "))
        Race = 1;
    else if (NamePrefix(Name, "GE "))
        Race = 2;
    else if (NamePrefix(Name, "PL "))                         // DAT_007fac18
        Race = 3;
    else if (NamePrefix(Name, "SU "))                         // 0x7fac1c..0x7fac28
        Race = 4;
    else if (NamePrefix(Name, "HU "))
        Race = 5;
    else if (NamePrefix(Name, "YU "))
        Race = 6;
    else if (NamePrefix(Name, "FR "))
        Race = 7;
    if (UnitType == 0x10)
        SupportPlace = true;
    if (Name.size != 0 && (_stricmp(Name.buf, "Support place") == 0 ||
                           _stricmp(Name.buf, "Support place desert") == 0))
        SupportPlace = true;
    _ec = 20.0f;                                              // 0x41a00000
    if (UnitType == 0x12)
        IsTrain = true;
}

// PANZERS 0x5a7390
void SPUnit::LoadGunners(SUPropStruct* unit)
{
    GunnerCount = (unsigned char)unit->GetArraySize("Gunners");
    for (int i = 0; i < GunnerCount; ++i) {
        int idx = ArrayAdd(&PGunners);
        PGunners.Array[idx] = new SPGunner();                 // new 0xcc, 0x582ba0
        PGunners.Array[idx]->Load(unit->GetArrayItem("Gunners", i));   // vtbl +0x04
    }
}

// PANZERS 0x5a6f10
// HD constructs the SP*Driver classes inline (P's classes): the recompile asks
// PzCreatePDriver. An unknown type is dropped from the array, as in HD.
void SPUnit::InitDrivers(SUPropStruct* unit)
{
    DriverCount = (unsigned char)unit->GetArraySize("Drivers");
    for (int i = 0; i < DriverCount; ++i) {
        SUPropStruct* driver = unit->GetArrayItem("Drivers", i);
        int type = driver->GetMultiIndex("DriverType");
        int idx = ArrayAdd(&PDrivers);
        SIPDriver* pd = PzCreatePDriver(type, driver, SStr(Name));   // new + vtbl +0x04 Load
        if (!pd && (type == 2 || type == 4 || type > 13)) {
            PDrivers.Size--;                                  // SDArray::Remove(idx)
            PDrivers.Array[PDrivers.Size] = nullptr;
            continue;
        }
        PDrivers.Array[idx] = pd;
    }
}

// PANZERS 0x5a6ce0
// The SP*Animation classes are A's: LoadPUnitAnimation creates the prototype
// of the Unit.Animation multi and loads it (+0x04) from the selected
// alternative. A reads the flat keys of the same file (SAnimProps).
void SPUnit::InitAnimation(SUPropStruct* unit)
{
    _138 = unit->GetMultiIndex("Animation");                  // recompile: M1 visuals (see header)
    PAnimation = LoadPUnitAnimation(this, SAnimProps::FromFlat(unit->Source, unit->Key.c_str()));
}

// One "Effect" array of LoadResources (0x5a8780): pixie +0x10 per entry.
static void LoadEffectArray(SUPropStruct* common, const char* name, SUnitArray<SPUnitEffect>* out)
{
    int n = common->GetArraySize(name);
    for (int i = 0; i < n; ++i) {
        SUPropStruct* e = common->GetArrayItem(name, i);
        int idx = ArrayAdd(out);
        out->Array[idx].Proto = g_Pixie ? g_Pixie->LoadEffectPrototype(e->GetString("Effect"), false, false, 0, 0) : -1;
        AssignSString(&out->Array[idx].MeshName, e->GetString("MeshName"));
    }
}

// PANZERS 0x5a8780
void SPUnit::LoadResources(SUPropStruct* unit)
{
    SUPropStruct* common = unit->GetStruct("Common");
    SIGepardHD* g = PzGepard();
    ModelProto = g->LoadModelPrototype(common->GetString("ModelName"), 0.005f, nullptr, 0);   // +0x20
    if (g->GetOption(0xe) == 0)                                                       // +0x14
        LowPolyProto = g->LoadModelPrototype(common->GetString("LowPolyModelName"), 0.005f, nullptr, 0);
    else
        LowPolyProto = -1;
    WreckProto = g->LoadModelPrototype(common->GetString("WreckModelName"), 0.005f, nullptr, 0);
    if (ModelProto < 0)
        Logger.g->Panic("SPUnit::LoadResources: PModelIdx < 0 , %s", common->GetString("ModelName"));
    LoadEffectArray(common, "Static_Effects", &StaticEffects);
    LoadEffectArray(common, "Pilot_Light_Effects", &PilotLightEffects);
    // Die_Effects: a mesh name ending in "_01" repeats the effect on
    // "<name>02", "<name>03", ... while the model prototype has that node
    // (Gepard +0x2c). The prototype node lookup is not in the recompile's
    // Gepard facade, so only the listed mesh is used here.
    LoadEffectArray(common, "Die_Effects", &DieEffects);
    LoadEffectArray(common, "Died_by_Fire_Effects", &DiedByFireEffects);
    LoadEffectArray(common, "Destroy_Effects", &DestroyEffects);
    LoadEffectArray(common, "Night_Effects", &NightEffects);
    for (int i = 0; i < PGunners.Size; ++i)
        if (PGunners.Array[i])
            PGunners.Array[i]->LoadResources(unit->GetArrayItem("Gunners", i));   // vtbl +0x08
    for (int i = 0; i < PDrivers.Size; ++i)
        if (PDrivers.Array[i])
            PDrivers.Array[i]->LoadSubProperties((SProperties*)unit->GetArrayItem("Drivers", i));   // vtbl +0x08: the "Drivers" item (effect arrays)
    if (PAnimation)                                                               // vtbl +0x08(this, sub)
        static_cast<SPUnitAnimation*>(PAnimation)->LoadResourcesProps(
            this, SAnimProps::FromFlat(unit->Source, unit->Key.c_str()).GetMultiSub("Animation"));
    Loaded = true;                                                                // +0xdd
}

// PANZERS 0x5a9950
void SPUnit::ReleaseResources()
{
    SUnitArray<SPUnitEffect>* effects[] = { &StaticEffects, &PilotLightEffects, &DieEffects,
                                            &DiedByFireEffects, &DestroyEffects, &PilotLightEffects,
                                            &NightEffects };
    for (SUnitArray<SPUnitEffect>* e : effects) {
        for (int i = 0; i < e->Size; ++i)
            if (g_Pixie && e->Array[i].Proto >= 0)
                g_Pixie->ReleaseEffectPrototype(e->Array[i].Proto);                 // pixie +0x20
        for (int i = 0; i < e->Size; ++i)                                           // 0x550980(0)
            FreeSString(&e->Array[i].MeshName);
        if (e->Size)
            memset(e->Array, 0, e->Max * sizeof(SPUnitEffect));
        e->Size = 0;
    }
    SIGepardHD* g = PzGepard();
    g->ReleaseModelPrototype(ModelProto);                                         // Gepard +0x24
    g->ReleaseModelPrototype(LowPolyProto);
    g->ReleaseModelPrototype(WreckProto);
    g->ReleaseModelPrototype(Proto5C);
    for (int i = 0; i < PGunners.Size; ++i)
        if (PGunners.Array[i])
            PGunners.Array[i]->ReleaseResources();                                // vtbl +0x0c
    for (int i = 0; i < PDrivers.Size; ++i)
        if (PDrivers.Array[i])
            PDrivers.Array[i]->Slot_0C();                                         // vtbl +0x0c
    if (PAnimation)
        PAnimation->Slot_0C();                                                    // vtbl +0x0c
    Loaded = false;
}

SIUnit* SPUnit::CreateUnit(int worldIndex)
{
    // _purecall in HD.
    Logger.g->Panic("SPUnit::CreateUnit: pure virtual call (%s, %d)", SStr(Name), worldIndex);
    return nullptr;
}

// ---------------------------------------------------------------------------
// SPSingleUnit

// PANZERS 0x5a4ae0
SPSingleUnit::SPSingleUnit()
{
    ChildUnits.Array = nullptr;
    ChildUnits.Size = ChildUnits.Max = 0;
    DemolishType = 0;
}

// PANZERS 0x5a57c0
SPSingleUnit::~SPSingleUnit()
{
    for (int i = 0; i < ChildUnits.Size; ++i) {               // 0x5a4e40
        FreeSString(&ChildUnits.Array[i].MeshName);
        FreeSString(&ChildUnits.Array[i].UnitName);
    }
    free(ChildUnits.Array);
    ChildUnits.Array = nullptr;
    ChildUnits.Size = ChildUnits.Max = 0;
}

// PANZERS 0x5a62d0
void SPSingleUnit::LoadHeader(SUPropStruct* unit)
{
    SPUnit::LoadHeader(unit);
    SUPropStruct* common = unit->GetStruct("Common");
    ClassType = common->GetMultiIndex("ClassType");
    SUPropStruct* ct;
    switch (ClassType) {
    case 0: {
        ct = common->GetMultiSubStruct("ClassType");
        BuiltInDriver = ct->GetBool("BuiltInDriver");
        AssignSString(&BuiltInDriverUnitName, ct->GetString("BuiltInDriverUnitName"));
        StorageCapacity = ct->GetInt("StorageCapacity");
        StorageType = ct->GetEnum("StorageType");
        OnlyCrew = ct->GetBool("OnlyCrew");
        int n = ct->GetArraySize("Child Units");
        for (int i = 0; i < n; ++i) {
            SUPropStruct* c = ct->GetArrayItem("Child Units", i);
            int idx = ArrayAdd(&ChildUnits);                  // 0x5a5990
            AssignSString(&ChildUnits.Array[idx].UnitName, c->GetString("UnitName"));
            AssignSString(&ChildUnits.Array[idx].MeshName, c->GetString("MeshName"));
        }
        DemolishType = ct->GetEnum("DemolishType");
        return;
    }
    case 4:
        return;
    case 0xb:
        ct = common->GetMultiSubStruct("ClassType");
        StorageCapacity = ct->GetInt("StorageCapacity");
        StorageType = ct->GetEnum("StorageType");
        DemolishType = ct->GetEnum("DemolishType");
        return;
    case 0xc:
        ct = common->GetMultiSubStruct("ClassType");
        BuiltInDriver = ct->GetBool("BuiltInDriver");
        AssignSString(&BuiltInDriverUnitName, ct->GetString("BuiltInDriverUnitName"));
        break;
    case 0xd:
        ct = common->GetMultiSubStruct("ClassType");
        break;
    default:
        Logger.g->Panic("SPSingleUnit::Init - bad ClassType");
        return;
    }
    StorageCapacity = ct->GetInt("StorageCapacity");
    StorageType = ct->GetEnum("StorageType");
}

// PANZERS 0x5a5cf0
SIUnit* SPSingleUnit::CreateUnit(int worldIndex)
{
    return new SSingleUnit(this, worldIndex);                 // new 0x3a8, 0x5aa7b0
}

// ---------------------------------------------------------------------------
// SPProjectileUnit (not created in the menu)

// PANZERS 0x5a4a20
SPProjectileUnit::SPProjectileUnit()
{
    memset((unsigned char*)this + 0x13c, 0, 0x178 - 0x13c);
    ClassType = 3;
}

// PANZERS 0x5a6280
void SPProjectileUnit::LoadHeader(SUPropStruct* unit)
{
    SPUnit::LoadHeader(unit);
    if (ClassType != unit->GetStruct("Common")->GetMultiIndex("ClassType"))
        Logger.g->Panic("SPProjectileUnit::Init - bad ClassType");
}

// PANZERS 0x5a7e70
// The incidence effects of the projectile (Unit.Common.ClassType sub-struct),
// then the SPUnit resources.
void SPProjectileUnit::LoadResources(SUPropStruct* unit)
{
    PZ_M3_TRACE("SPProjectileUnit::LoadResources (0x5a7e70)");
    SUPropStruct* ct = unit->GetStruct("Common")->GetMultiSubStruct("ClassType");
    LoadEffectArray(ct, "Ground_Incidence_Effects", &GroundIncidence);
    LoadEffectArray(ct, "Water_Incidence_Effects", &WaterIncidence);
    LoadEffectArray(ct, "Unit_Incidence_Effects", &UnitIncidence);
    LoadEffectArray(ct, "Building_Wood_Incidence_Effects", &WoodIncidence);
    LoadEffectArray(ct, "Building_Stone_Incidence_Effects", &StoneIncidence);
    SPUnit::LoadResources(unit);                              // 0x5a8780
}

// PANZERS 0x5a5c70
SIUnit* SPProjectileUnit::CreateUnit(int worldIndex)
{
    PZ_M3_TRACE("SPProjectileUnit::CreateUnit (0x5a5c70)");
    return new SProjectileUnit(this, worldIndex);             // new 0x36c, 0x5a3810
}

// ---------------------------------------------------------------------------
// SPPanzersSquadUnit

// PANZERS 0x5a49e0
SPPanzersSquadUnit::SPPanzersSquadUnit()
{
    MaxNumberOfUnits = 0;
    Formations = 0;
    ClassType = 5;
}

// PANZERS 0x5a5690
SPPanzersSquadUnit::~SPPanzersSquadUnit()
{
    FreeSString(&SquadMemberName);
}

// PANZERS 0x5a61d0
void SPPanzersSquadUnit::LoadHeader(SUPropStruct* unit)
{
    SPUnit::LoadHeader(unit);
    _ec = 12.5f;                                              // 0x41480000
    SUPropStruct* common = unit->GetStruct("Common");
    if (ClassType != common->GetMultiIndex("ClassType"))
        Logger.g->Panic("SPPanzersSquadUnit::Init - bad ClassType");
    SUPropStruct* ct = common->GetMultiSubStruct("ClassType");
    MaxNumberOfUnits = (unsigned char)ct->GetInt("MaxNumberOfUnits");
    AssignSString(&SquadMemberName, ct->GetString("SquadMemberName"));
    Formations = ct->GetInt("Formations");
    Price2 = Price2 / 2;
}

// PANZERS 0x5a5bf0
SIUnit* SPPanzersSquadUnit::CreateUnit(int worldIndex)
{
    return new SPanzersSquadUnit(this, worldIndex);           // new 0x3a4, 0x598ff0
}

// ---------------------------------------------------------------------------
// SPPanzersSquadMemberUnit

// PANZERS 0x5a49b0
SPPanzersSquadMemberUnit::SPPanzersSquadMemberUnit()
{
    ClassType = 6;
    ParachuteProto = -1;
}

// PANZERS 0x5a5600
SPPanzersSquadMemberUnit::~SPPanzersSquadMemberUnit()
{
    if (Loaded) {
        PzGepard()->ReleaseModelPrototype(ParachuteProto);    // Gepard +0x24
        ParachuteProto = -1;
    }
}

// PANZERS 0x5a7da0
void SPPanzersSquadMemberUnit::LoadResources(SUPropStruct* unit)
{
    SPUnit::LoadResources(unit);
    if (Name.size == 0)
        return;
    const char* model = nullptr;
    if (_stricmp(Name.buf, "Ge Parachute Member") == 0)
        model = "objects/21 special objects/parachute_german.4da";
    else if (_stricmp(Name.buf, "US Parachute Member") == 0)
        model = "objects/21 special objects/parachute_allied.4da";
    else if (_stricmp(Name.buf, "SU Parachute Member") == 0)
        model = "objects/21 special objects/parachute_russian.4da";
    if (model)
        ParachuteProto = PzGepard()->LoadModelPrototype(model, 0.005f, nullptr, 0);
}

// PANZERS 0x5a9770
void SPPanzersSquadMemberUnit::ReleaseResources()
{
    PzGepard()->ReleaseModelPrototype(ParachuteProto);
    ParachuteProto = -1;
}

// PANZERS 0x5a5b70
SIUnit* SPPanzersSquadMemberUnit::CreateUnit(int worldIndex)
{
    return new SPanzersSquadMemberUnit(this, worldIndex);     // new 0x354, 0x5977e0
}

// ---------------------------------------------------------------------------
// SPWasterUnit, SPFlyingUnit, SPTrainUnit (registry only in the menu)

// PANZERS 0x5a4db0
SPWasterUnit::SPWasterUnit()
{
    ClassType = 7;
    LifeTime = 0;
    Sensor = RemoteControl = false;
}

// PANZERS 0x5a6c30
void SPWasterUnit::LoadHeader(SUPropStruct* unit)
{
    SPUnit::LoadHeader(unit);
    SUPropStruct* common = unit->GetStruct("Common");
    if (ClassType != common->GetMultiIndex("ClassType"))
        Logger.g->Panic("Waster::Init - bad UnitType");
    SUPropStruct* ct = common->GetMultiSubStruct("ClassType");
    LifeTime = (int)(ct->GetFloat("LifeTime") * 20.0f + 0.5f);   // ROUND(LifeTime * 20)
    Sensor = ct->GetBool("Sensor");
    RemoteControl = ct->GetBool("RemoteControl");
}

// PANZERS 0x5a5df0
SIUnit* SPWasterUnit::CreateUnit(int worldIndex)
{
    PZ_M3_TRACE("SPWasterUnit::CreateUnit (0x5a5df0)");
    return new SWasterUnit(this, worldIndex);                 // new 0x364, 0x5d1cd0
}

// PANZERS 0x5a4980
SPFlyingUnit::SPFlyingUnit()
{
    ClassType = 8;
    PlaneType = 0;
}

// PANZERS 0x5a6160
void SPFlyingUnit::LoadHeader(SUPropStruct* unit)
{
    SPUnit::LoadHeader(unit);
    SUPropStruct* common = unit->GetStruct("Common");
    if (ClassType != common->GetMultiIndex("ClassType"))
        Logger.g->Panic("SPFlyingUnit::Init - bad UnitType");
    PlaneType = common->GetMultiSubStruct("ClassType")->GetEnum("PlaneType");
}

// PANZERS 0x5a5af0
SIUnit* SPFlyingUnit::CreateUnit(int worldIndex)
{
    PZ_M3_TRACE("SPFlyingUnit::CreateUnit (0x5a5af0)");
    return new SFlyingUnit(this, worldIndex);                 // new 0x388, 0x55c9d0
}

// PANZERS 0x5a4b30
SPTrainUnit::SPTrainUnit()
{
    memset(_13c, 0, sizeof(_13c));
}

// PANZERS 0x5a6580
void SPTrainUnit::LoadHeader(SUPropStruct* unit)
{
    SPUnit::LoadHeader(unit);
    _ec = 50.0f;                                              // 0x42480000
    SUPropStruct* common = unit->GetStruct("Common");
    Price = (int)common->GetFloat("Price");
    ClassType = common->GetMultiIndex("ClassType");
    if (ClassType == 4)
        return;
    if (ClassType != 10)
        Logger.g->Panic("SPTrainUnit::Init - bad UnitType");
    SUPropStruct* ct = common->GetMultiSubStruct("ClassType");
    BuiltInDriver = ct->GetBool("BuiltInDriver");
    AssignSString(&BuiltInDriverUnitName, ct->GetString("BuiltInDriverUnitName"));
    StorageCapacity = ct->GetInt("StorageCapacity");
    StorageType = ct->GetEnum("StorageType");
}

SIUnit* SPTrainUnit::CreateUnit(int worldIndex)
{
    STUB_LOG("SPTrainUnit::CreateUnit (0x5a5d70) STrainUnit");
    PZ_M2_TRACE("SPTrainUnit::CreateUnit (0x5a5d70)");
    (void)worldIndex;
    return nullptr;
}

// ---------------------------------------------------------------------------
// SPBuildingUnit

// PANZERS 0x5a4900
SPBuildingUnit::SPBuildingUnit()
{
    Products.Array = nullptr;
    Products.Size = Products.Max = 0;
    ClassType = 9;
    BuildingType = -1;
    BuildingDowngrading = false;
    BuildingMaterial = -1;
    CaptureRange = 0.0f;
    FlagProto = -1;
    FlagMultiProto = -1;
}

// PANZERS 0x5a5430
SPBuildingUnit::~SPBuildingUnit()
{
    if (Loaded) {
        if (BuildingType == 3) {
            PzGepard()->ReleaseModelPrototype(FlagProto);
            PzGepard()->ReleaseModelPrototype(FlagMultiProto);
        }
        SPUnit::ReleaseResources();
    }
    free(Products.Array);                                     // 0x5a4ed0 (strings: productive buildings only)
    Products.Array = nullptr;
    Products.Size = Products.Max = 0;
}

// PANZERS 0x5a6050
void SPBuildingUnit::LoadHeader(SUPropStruct* unit)
{
    SPUnit::LoadHeader(unit);
    SUPropStruct* common = unit->GetStruct("Common");
    if (ClassType != common->GetMultiIndex("ClassType"))
        Logger.g->Panic("SPBuildingUnit::Init - bad ClassType");
    SUPropStruct* ct = common->GetMultiSubStruct("ClassType");
    BuildingType = ct->GetEnum("BuildingType");
    BuildingDowngrading = ct->GetBool("BuildingDowngrading");
    BuildingMaterial = ct->GetEnum("BuildingMaterial");
    if (BuildingMaterial > 1)
        BuildingMaterial = 0;
    if (BuildingType == 0 || BuildingType == 1) {
        StorageCapacity = 2;
        StorageType = 0;
    } else if (BuildingType == 2) {
        StorageCapacity = 1;
        StorageType = 0;
    } else if (BuildingType == 5) {
        StorageCapacity = 10;
        StorageType = 0;
    } else {
        StorageType = -1;
        StorageCapacity = 0;
    }
    CaptureRange = ct->GetFloat("CaptureRange") * 0.5f;
}

// PANZERS 0x5a7500
// Capturable buildings (type 3) load the two flag models; productive ones
// (unit type 0x1a) read buildings/<name>.productive into Products. The menu
// house is type 0, so the productive part is not lifted.
void SPBuildingUnit::LoadResources(SUPropStruct* unit)
{
    if (BuildingType == 3) {
        FlagProto = PzGepard()->LoadModelPrototype("objects/21 Special Objects/flag.4DA", 0.005f, nullptr, 0);
        FlagMultiProto = PzGepard()->LoadModelPrototype("objects/21 Special Objects/flag-multi.4DA", 0.005f, nullptr, 0);
        if (UnitType == 0x1a) {
            STUB_LOG("SPBuildingUnit::LoadResources (0x5a7500) productive building");
            PZ_M2_TRACE("SPBuildingUnit::LoadResources (0x5a7500) productive");
        }
    }
    SPUnit::LoadResources(unit);
}

// PANZERS 0x5a9730
void SPBuildingUnit::ReleaseResources()
{
    if (BuildingType == 3) {
        PzGepard()->ReleaseModelPrototype(FlagProto);
        PzGepard()->ReleaseModelPrototype(FlagMultiProto);
    }
    SPUnit::ReleaseResources();
}

// PANZERS 0x5a5a70
SIUnit* SPBuildingUnit::CreateUnit(int worldIndex)
{
    return new SBuildingUnit(this, worldIndex);               // new 0x470, 0x545940
}

// ---------------------------------------------------------------------------

// The ClassType switch of SUnitRegistry::LoadUnitFiles 0x5d1050.
SPUnit* CreatePUnit(int classType)
{
    switch (classType) {
    case 0: case 0xb: case 0xc: case 0xd: case 4:
        return new SPSingleUnit();                            // new 0x14c, 0x5a4ae0
    case 3:
        return new SPProjectileUnit();                        // new 0x178, 0x5a4a20
    case 5:
        return new SPPanzersSquadUnit();                      // new 0x14c, 0x5a49e0
    case 6:
        return new SPPanzersSquadMemberUnit();                // new 0x140, 0x5a49b0
    case 7:
        return new SPWasterUnit();                            // new 0x144, 0x5a4db0
    case 8:
        return new SPFlyingUnit();                            // new 0x140, 0x5a4980
    case 9:
        return new SPBuildingUnit();                          // new 0x160, 0x5a4900
    case 10:
        return new SPTrainUnit();                             // new 0x14c, 0x5a4b30
    default:
        return nullptr;
    }
}

} // namespace pz
