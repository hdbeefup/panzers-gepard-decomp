// src/game/gunner.cpp
// SPGunner / SGunner (0x582a00..0x587d00). OWNER: M3-C sub-agent C1 (agent U in M2). See gunner.h.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gunner.h"
#include "waster.h"
#include "unit.h"
#include "unitprops.h"
#include "unitextern.h"
#include "world.h"
#include "worldapi.h"
#include "drivermath.h"
#include "pzunitregistry.h"
#include "pz/ipixie.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"
#include "target.h"
#include "projectile.h"
#include "projectile_driver.h"
#include "pz/imodel.h"
#include "squadunit.h"
#include "buildingunit.h"
#include "gamelogic.h"
#include "campaign.h"
#include "iunitanim.h"
#include "idriver.h"
#include "m3common.h"

namespace pz {

void FreeSString(SString* s);

static const double kPi = 3.1415927410125732;        // DAT_007f4560
static const double kTwoPi = 6.2831854820251465;     // DAT_007f4570
static const float kDeg = 0.01745329238474369f;      // DAT_007f59a0

static void FreeEffects(SUnitArray<SPUnitEffect>* e)            // 0x54fd00
{
    for (int i = 0; i < e->Size; ++i)
        FreeSString(&e->Array[i].MeshName);
    free(e->Array);
    e->Array = nullptr;
    e->Size = e->Max = 0;
}

// ---------------------------------------------------------------------------
// SPGunner

// PANZERS 0x582ba0
SPGunner::SPGunner()
{
    memset((unsigned char*)this + sizeof(void*), 0, sizeof(SPGunner) - sizeof(void*));
    GunnerType = -1;                                          // param_1[7]
    DirectDamage = true;                                      // (char)param_1[0x13] = 1
}

// PANZERS 0x582ef0
SPGunner::~SPGunner()
{
    FreeEffects(&ShotEffects);
    FreeEffects(&GroundIncidenceEffects);
    FreeEffects(&WaterIncidenceEffects);
    FreeEffects(&UnitIncidenceEffects);
    FreeEffects(&WoodIncidenceEffects);
    FreeEffects(&StoneIncidenceEffects);
    for (int i = 0; i < Projectiles.Size; ++i)                // 0x582d20
        FreeSString(&Projectiles.Array[i]);
    free(Projectiles.Array);
    Projectiles.Array = nullptr;
    Projectiles.Size = Projectiles.Max = 0;
}

// PANZERS 0x5842e0
void SPGunner::Load(SUPropStruct* g)
{
    GunnerType = g->GetEnum("GunnerType");
    GunnerHeight = g->GetFloat("GunnerHeight") * 0.5f;
    WeaponType = g->GetEnum("WeaponType");
    FireStartArc = g->GetFloat("WeaponFireStartArcAngle") * kDeg;
    FireArc = g->GetFloat("WeaponFireArcAngle") * kDeg;
    ParentGunner = g->GetInt("ParentGunner");
    int htr = g->GetMultiIndex("HandToHand / Ranged");
    HandToHand = htr == 0;
    SUPropStruct* s = g->GetMultiSubStruct("HandToHand / Ranged");
    if (htr == 0) {
        float max = s->GetFloat("MaxRange");
        MinRange = 0.0f;
        Ammo = 0;
        ReloadTime = 0;
        ProjectileCount = 0;
        MaxRange = max * 0.5f;
        KickSize = 0.0f;
        KickTime = 0;
        ShotAngle = 0.0f;
        ShotSpread = 0.0f;
    } else {
        MinRange = s->GetFloat("MinRange") * 0.5f;
        MaxRange = s->GetFloat("MaxRange") * 0.5f;
        Ammo = (int)s->GetFloat("Ammo");                     // 0x767700
        DirectDamage = s->GetBool("DirectDamage");
        ProjectileCount = s->GetArraySize("Projectiles");
        for (int i = 0; i < ProjectileCount; ++i) {
            SUPropStruct* p = s->GetArrayItem("Projectiles", i);
            if (Projectiles.Size == Projectiles.Max) {
                int nmax = Projectiles.Max < 0x10 ? 0x10 : (Projectiles.Max * 6) / 5;
                Projectiles.Array = (SString*)realloc(Projectiles.Array, nmax * sizeof(SString));
                memset((void*)&Projectiles.Array[Projectiles.Max], 0, (nmax - Projectiles.Max) * sizeof(SString));
                Projectiles.Max = nmax;
            }
            SString* dst = &Projectiles.Array[Projectiles.Size++];
            *dst = p->GetString("Name");
        }
        KickSize = s->GetFloat("KickSize");
        KickTime = (int)(s->GetFloat("KickTime") * 20.0f);    // DAT_007f35d8
        ShotAngle = s->GetFloat("ShotAngle") * kDeg;
        ShotSpread = s->GetFloat("ShotSpread");
    }
    int fr = g->GetMultiIndex("Fixed / Rotatable");
    Fixed = fr == 0;
    if (fr != 0) {
        SUPropStruct* r = g->GetMultiSubStruct("Fixed / Rotatable");
        TurretSpeed = (float)((((double)r->GetFloat("TurretSpeed") / 180.0) * kPi) / 20.0);
        MaxLeftAngle = r->GetFloat("MaxLeftAngle") * -kDeg;   // _DAT_007f8418
        MaxRightAngle = r->GetFloat("MaxRightAngle") * kDeg;
    } else {
        TurretSpeed = MaxLeftAngle = MaxRightAngle = 0.0f;
    }
    Damage = g->GetFloat("Damage");
    DamageRadius = g->GetFloat("DamageRadius");
    if (Damage == 0.0f && DamageRadius == 0.0f) {
        int dr = g->GetMultiIndex("Direct / Radius Damage");
        SUPropStruct* d = g->GetMultiSubStruct("Direct / Radius Damage");
        Damage = d->GetFloat("Damage");
        DamageRadius = dr == 0 ? 0.0f : d->GetFloat("DamageRadius");
    }
    ReloadTime = (int)floorf(g->GetFloat("ReloadTime") * 20.0f + 0.5f);   // ROUND
    ReshotTime = (int)floorf(g->GetFloat("ReshotTime") * 20.0f + 0.5f);
    ShotDelay = (int)floorf(g->GetFloat("ShotDelay") * 20.0f + 0.5f);
    AutoShot = g->GetBool("AutoShot");
    BurstShot = g->GetInt("BurstShot");
    BodyKick = g->GetFloat("BodyKick");
    double minArc = ((double)TurretSpeed / 20.0) * 2.0;       // DAT_007ea768
    if ((double)FireArc < minArc)
        FireArc = (float)minArc;
}

static void LoadEffects(SUPropStruct* g, const char* name, SUnitArray<SPUnitEffect>* out)
{
    int n = g->GetArraySize(name);
    for (int i = 0; i < n; ++i) {
        SUPropStruct* e = g->GetArrayItem(name, i);
        if (out->Size == out->Max) {
            int nmax = out->Max < 0x10 ? 0x10 : (out->Max * 6) / 5;
            out->Array = (SPUnitEffect*)realloc(out->Array, nmax * sizeof(SPUnitEffect));
            memset((void*)&out->Array[out->Max], 0, (nmax - out->Max) * sizeof(SPUnitEffect));
            out->Max = nmax;
        }
        SPUnitEffect* fx = &out->Array[out->Size++];
        fx->Proto = g_Pixie ? g_Pixie->LoadEffectPrototype(e->GetString("Effect"), false, false, 0, 0) : -1;   // pixie +0x10
        fx->MeshName = e->GetString("MeshName");
    }
}

// PANZERS 0x584a30
// The projectile prototypes are loaded through the registry (GetPUnit(name,
// true)), then the effect arrays.
void SPGunner::LoadResources(SUPropStruct* g)
{
    for (int i = 0; i < Projectiles.Size; ++i)
        if (g_UnitRegistry)
            g_UnitRegistry->GetPUnit(SStr(Projectiles.Array[i]), true);
    LoadEffects(g, "Shot_Effects", &ShotEffects);
    LoadEffects(g, "Ground_Incidence_Effects", &GroundIncidenceEffects);
    LoadEffects(g, "Water_Incidence_Effects", &WaterIncidenceEffects);
    LoadEffects(g, "Unit_Incidence_Effects", &UnitIncidenceEffects);
    LoadEffects(g, "Building_Wood_Incidence_Effects", &WoodIncidenceEffects);
    LoadEffects(g, "Building_Stone_Incidence_Effects", &StoneIncidenceEffects);
}

// PANZERS 0x587c00
void SPGunner::ReleaseResources()
{
    SUnitArray<SPUnitEffect>* all[] = { &ShotEffects, &GroundIncidenceEffects, &WaterIncidenceEffects,
                                        &UnitIncidenceEffects, &StoneIncidenceEffects, &WoodIncidenceEffects };
    for (SUnitArray<SPUnitEffect>* e : all) {
        for (int i = 0; i < e->Size; ++i)
            if (g_Pixie && e->Array[i].Proto >= 0)
                g_Pixie->ReleaseEffectPrototype(e->Array[i].Proto);
        for (int i = 0; i < e->Size; ++i)                     // 0x550980(0)
            FreeSString(&e->Array[i].MeshName);
        e->Size = 0;
    }
}

// PANZERS 0x583df0
SGunner* SPGunner::CreateGunner(SUnit* unit, SIUnitAnimation* anim, int index)
{
    return new SGunner(this, unit, anim, index);              // new 0x78, 0x582ad0
}

// ---------------------------------------------------------------------------
// SGunner

static bool UnitLive(int i)
{
    return g_World && g_World->Units.IsLive(i);
}

// HD 0x549ab0 (the copy of squadrefresh.cpp): players a and b are allies
// (same alliance World+0x17c + p * 0x48 +0xc, or the same player without one).
static bool WorldIsAlly(int a, int b)
{
    int ta = *(int*)(g_World->Players[a] + 0xc);
    if (ta != 0)
        return ta == *(int*)(g_World->Players[b] + 0xc);
    return a == b;
}

static int ClassOf(SUnit* u)
{
    return u->Proto->ClassType;
}

static SGunner* UnitGunner(SUnit* u, int i)                  // SDArray<SGunner*>::operator[] 0x55cc40
{
    if (i < 0 || i >= u->Gunners.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", i);
    return u->Gunners.Array[i];
}

// HD 0x546330: SBuildingUnit +0x3e8 SDArray of 0x48-byte window records
// {x, z, dir, ...} (buildingunit.h declares the array with 0xc elements).
static const float* BuildingWindow(SUnit* b, int i)
{
    unsigned char* arr = *(unsigned char**)((unsigned char*)b + 0x3e8);
    int n = *(int*)((unsigned char*)b + 0x3ec);
    if (i < 0 || i >= n)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SBuildingUnit::SWindow", i);
    return (const float*)(arr + i * 0x48);
}

// HD 0x56d1d0 (SGameLogic, agent O's row; a local copy for the gunner): the
// unit at `index` when it is live and, with a viewer, seen by the viewer's
// player (0x562760); otherwise null.
static SUnit* VisibleUnit(SUnit* viewer, int index)
{
    if (!UnitLive(index))
        return nullptr;
    if (viewer && !g_GameLogic->CanSeeGroundUnit(viewer->Player, WorldUnit(index)))
        return nullptr;
    return WorldUnit(index);
}

// PANZERS 0x5ba860
// SUnit: switches the unit's static effects off (pixie +0x60) for 30 ticks
// (+0x70; SUnit::ServerRefresh switches them on again, 0x5c2200).
void SUnit::DisableStaticEffects()
{
    for (int i = 0; i < StaticEffects.Size; ++i)
        if (g_Pixie)
            g_Pixie->SetEffectEnabled(StaticEffects.Array[i], false);
    EffectTimer = 30;
}

// PANZERS 0x5b8260
// SUnit: stops the unit and locks its driver (a magnetic mine was placed on
// it: SGunner::ServerRefresh, waster units).
void SUnit::LockDriver()
{
    Stop();                                                   // +0xc0
    if (CurrentTarget)
        RefreshDriver = ActiveDriver < 0 ? PrevDriver : ActiveDriver;
    PrevDriver = ActiveDriver;
    ActiveDriver = -1;
    if (g_GameLogic)
        g_GameLogic->RemoveUnitFromMovementGroup(WorldIndex); // 0x579510
    DriverLocked = true;
}

// PANZERS 0x582ad0
SGunner::SGunner(SPGunner* proto, SUnit* unit, SIUnitAnimation* anim, int index)
{
    memset((unsigned char*)this + sizeof(void*), 0, sizeof(SGunner) - sizeof(void*));
    Unit = unit;
    Anim = anim;
    Proto = proto;
    BurstLeft = proto->BurstShot;
    Index = index;
    Active = 1;
    AmmoLeft = 1.0f;
}

// PANZERS 0x582e80
SGunner::~SGunner()
{
    if (Target) {
        PzTargetRelease(Target);                              // 0x5bdef0
        Target = nullptr;
    }
}

// PANZERS 0x582f70
// A world angle into the turret frame (unit dir, the parent gunner's turret,
// the fire arc start), then +0x04.
void SGunner::AimWorld(float worldAngle)
{
    float base = Unit->Dir;
    int parent = Proto->ParentGunner;
    if (parent > -1)
        base = (float)GunnerWrap((double)UnitGunner(Unit, parent)->Turret.Angle + (double)base);
    double a = GunnerWrap((double)Proto->FireStartArc + (double)base);
    TurnTurret((float)GunnerWrap((double)worldAngle - a));
}

// PANZERS 0x583070
// Turns the turret (+0x30) towards `angle` (clamped to the prototype's
// left / right limits) with an acceleration of 0..20 steps; true when it
// arrived (gunnermath.cpp, emulator-verified).
bool SGunner::TurnTurret(float angle)
{
    return GunnerTurnAxis(&Turret, angle, Proto->TurretSpeed, Proto->MaxLeftAngle, Proto->MaxRightAngle, false);
}

// PANZERS 0x583550
void SGunner::AimElevation(float angle)
{
    TurnElevation(angle);                                     // +0x0c
}

// PANZERS 0x583570
// The barrel elevation (+0x48) towards `angle` (0..1.5533431, DAT_007f83e8),
// same acceleration scheme and speed as the turret.
bool SGunner::TurnElevation(float angle)
{
    return GunnerTurnAxis(&Elev, angle, Proto->TurretSpeed, 0.0f, 0.0f, true);
}

// PANZERS 0x583af0
// Whether the weapon can hurt the target: other weapons anything; bullets
// (weapon type 0) not armour type 2 (except class 0xb without +0x110), and
// buildings only when their prototype +0x13c is 2; planes (class 8) only
// for air-defence gunners. (M3-I: the recompile had the weapon test
// inverted; HD 0x583af0 skips the armour tests when 0x584240 != 0.)
bool SGunner::CanAttack(SUnit* target)
{
    if (GetWeaponType() != 0)                                 // 0x584240
        goto air;
    {
        SPUnit* p = target->Proto;
        if (p->ArmourType == 2) {                             // +0x94
            if (p->ClassType != 0xb || target->_110)
                return false;
        }
        if (p->ClassType == 9 && *(int*)((unsigned char*)target->Proto + 0x13c) != 2)   // SPBuildingUnit +0x13c BuildingType
            return false;
    }
air:
    if (target->Proto->ClassType == 8 && Proto->GunnerType != 1)
        return false;
    return true;
}

// PANZERS 0x583b60
// CanAttack, then: a gunner inside a building (parent class 9) checks every
// window of the building (+0x3e8, 0x48 bytes each) for one in range
// (+0x17c / +0x180 of the building) whose direction (+8) is within 1.1781
// of the target; any other gunner needs the target inside its fire arc
// unless ignoreArc.
bool SGunner::CanTargetUnit(SUnit* target, bool ignoreArc)
{
    if (!CanAttack(target))
        return false;
    int parent = Unit->Parent;
    if (parent > -1) {
        if (!UnitLive(parent))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", parent);
        SUnit* b = WorldUnit(parent);
        if (b->Proto->ClassType == 9) {
            int n = *(int*)((unsigned char*)b + 0x3ec);
            if (n == 0)
                goto arc;
            float tx = target->Pos[0], tz = target->Pos[2];
            for (int i = 0; i < n; ++i) {
                const float* w = BuildingWindow(b, i);
                float dz = tz - w[1];
                float dx = tx - w[0];
                float dist = (float)sqrt((double)(dz * dz + dx * dx));   // 0x78d090
                float maxR = b->GetMaxRange(Index);           // +0x17c
                if (maxR > dist) {
                    float minR = b->GetMinRange(Index);       // +0x180
                    if (dist > minR) {
                        double ang = (double)(float)DAtan2((double)dx, (double)dz);   // 0x78d07a (fstp qword)
                        double d = fabs(ang - (double)BuildingWindow(b, i)[2]);
                        if (d > kPi)
                            d = kTwoPi - d;
                        if (1.1780972480773926 >= d)          // 0x7f83f8
                            return true;
                    }
                }
            }
            return false;
        }
    }
arc:
    if (ignoreArc)
        return true;
    return IsInArc(target->Pos[0], target->Pos[1], target->Pos[2]);
}

// PANZERS 0x583e30
// Ammunition of non-bullet weapons: AmmoLeft -= 1 / Ammo, with the "low
// ammo" (0xa, crossing 0.3) and "out of ammo" (0xb) speech events. Squads,
// bullet weapons and a squad member's first gunner do not count.
void SGunner::ConsumeAmmo()
{
    if (ClassOf(Unit) == 5)
        return;
    if (GetWeaponType() == 0)                                 // 0x584240
        return;
    if (ClassOf(Unit) == 6 && Index == 0)
        return;
    float before = AmmoLeft;
    float after = before - 1.0f / (float)Proto->Ammo;         // DAT_007f1b58
    AmmoLeft = after;
    if (0.3f > after && before > 0.3f)                        // DAT_007f83d8
        g_World->UnitSpeech(Unit->WorldIndex, 0xa, false);    // 0x5fff20
    else if (!(0.0f < after))
        g_World->UnitSpeech(Unit->WorldIndex, 0xb, false);
    if (0.0f > AmmoLeft)
        AmmoLeft = 0.0f;
}

// PANZERS 0x583ed0
// The prototype damage (+0x58) times the rank bonus of the unit registry:
// squad members (not engineers, 0xe, nor crews of class 0 / 0xb vehicles)
// +0x7c.., vehicles of class 0 (and 0xb with OnlyCrew) +0x90.., wasters
// with an owner (+0x360) +0xfc.. by rank 1..4.
float SGunner::GetDamage()
{
    float dmg = Proto->Damage;
    SUnit* u = Unit;
    int ct = ClassOf(u);
    const SUnitRegistry* r = g_UnitRegistry;
    if (ct == 0 || ct == 0xb) {
        if (ct != 6)
            goto vehicle;
    } else if (ct != 6) {
        if (ct != 7)
            Logger.g->Panic("SGunner.ServerRefresh - Bad Unittype");
        dmg = GetPGunner()->Damage;                           // +0x2c
        if (*(int*)((unsigned char*)u + 0x360) != 0) {        // SWasterUnit +0x360 (owner rank)
            int rank = u->GetRank();                          // +0x88
            if (rank >= 1 && rank <= 4)
                return r->ExplosivesDamageBonus[rank - 1] * dmg;   // +0xfc..+0x108
        }
        return dmg;
    }
    if (u->Proto->UnitType != 0xe && u->Parent > -1 && ClassOf(WorldUnit(u->Parent)) != 0 &&
        ClassOf(WorldUnit(u->Parent)) != 0xb) {
        int rank = u->GetRank();
        if ((unsigned)rank <= 4)
            return r->SquadDamageBonus[rank] * dmg;           // +0x7c..+0x8c
        return dmg;
    }
vehicle:
    {
        SPUnit* p = Unit->Proto;
        if (p->ClassType == 0 || (p->ClassType == 0xb && p->OnlyCrew)) {   // +0xdc
            int rank = Unit->GetRank();
            if ((unsigned)rank <= 4)
                return r->CrewDamageBonus[rank] * dmg;        // +0x90..+0xa0
        }
    }
    return dmg;
}

// PANZERS 0x5847e0
// Projectile weapons (and the unit type 0x15) always have a line of fire;
// direct-damage weapons need no building between the shooter (or the
// building it sits in) and the target, both 0.5 above their position
// (0x5630c0).
bool SGunner::HasLineOfFire(int unit)
{
    if (!Proto->DirectDamage && Unit->Proto->UnitType != 0x15)
        return true;
    SUnit* u = Unit;
    int parent = u->Parent;
    float from[3], to[3];
    SUnit* src = u;
    if (parent > -1) {
        if (!UnitLive(parent))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", parent);
        if (ClassOf(WorldUnit(parent)) == 9)
            src = WorldUnit(parent);
    }
    if (!UnitLive(unit))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", unit);
    SUnit* t = WorldUnit(unit);
    to[0] = t->Pos[0] + 0.0f;
    to[1] = t->Pos[1] + 0.5f;                                 // DAT_007f453c
    to[2] = t->Pos[2] + 0.0f;
    from[0] = src->Pos[0] + 0.0f;
    from[1] = src->Pos[1] + 0.5f;
    from[2] = src->Pos[2] + 0.0f;
    return BuildingBetween(u->WorldIndex, unit, from, to) < 0;   // 0x5630c0
}

// PANZERS 0x583950
void SGunner::SetTarget(STarget* t)
{
    if (Target)
        PzTargetRelease(Target);
    if (t)
        tgt::I(t, tgt::kRefCount)++;                          // 0x5b5a30
    Target = t;
    Idle = false;
    RestAngle = Proto->FireStartArc;
    AimAngle = 0.0f;
}

// PANZERS 0x583460
void SGunner::SetTargetKeepTurret(STarget* t)
{
    if (Target)
        PzTargetRelease(Target);
    if (t)
        tgt::I(t, tgt::kRefCount)++;
    Target = t;
    Idle = true;
    RestAngle = Proto->FireStartArc;
    AimAngle = 0.0f;
}

// PANZERS 0x5834a0
void SGunner::AimAt(float angle)
{
    if (Target) {
        PzTargetRelease(Target);
        Target = nullptr;
    }
    Idle = true;
    RestAngle = (float)GunnerWrap((double)angle - (double)Unit->Dir);
    AimAngle = (float)GunnerWrap((double)angle - (double)Unit->Dir);
}

// PANZERS 0x584cd0
void SGunner::ResetToParent()
{
    if (Target) {
        PzTargetRelease(Target);
        Target = nullptr;
    }
    Idle = true;
    RestAngle = Turret.Angle;
    AimAngle = Turret.Angle;
}

// PANZERS 0x587bd0
void SGunner::Stop()
{
    if (Target) {
        PzTargetRelease(Target);
        Target = nullptr;
    }
    Idle = true;
    RestAngle = 0.0f;
    AimAngle = 0.0f;
}

// PANZERS 0x584210
SPGunner* SGunner::GetPGunner()
{
    return Proto;
}

// PANZERS 0x584220
void SGunner::Slot_30(void** p1, void** p2)
{
    *p1 = this;
    *p2 = nullptr;                                            // &PTR_s_Enabled_008dbaf8 (a property table)
}

// PANZERS 0x584240
// The weapon type (SPGunner +0x24); a squad answers with its first member's
// first gunner.
int SGunner::GetWeaponType()
{
    SGunner* g = this;
    while (ClassOf(g->Unit) == 5 && g->Unit->Members.Size > 0) {
        SUnit* m = WorldUnit(g->Unit->Members.Array[0].Unit);
        g = UnitGunner(m, 0);
    }
    return g->Proto->WeaponType;
}

// PANZERS 0x583990
// Whether the direction from the unit to (x, z) lies in the gunner's firing
// arc (start arc + parent turret + unit direction, +-half the arc width).
bool SGunner::IsInArc(float x, float y, float z)
{
    (void)y;
    SUnit* u = Unit;
    float dx = x - u->Pos[0];
    float dz = z - u->Pos[2];
    float angle = DAtan2f((double)dx, (double)dz);            // 0x78d07a, fstp dword
    float base = u->Dir;
    int parent = Proto->ParentGunner;
    if (parent > -1)
        base = (float)GunnerWrap((double)UnitGunner(u, parent)->Turret.Angle + (double)base);
    double a = GunnerWrap((double)Proto->FireStartArc + (double)base);
    float d = (float)GunnerWrap((double)angle - a);
    float half = Proto->FireArc * 0.5f;                       // DAT_007f453c
    if (d > Proto->MaxRightAngle + half)
        return false;
    return Proto->MaxLeftAngle - half <= d;
}

// The projectile prototype of this gunner's first projectile (0x5d0e70).
static SPProjectileUnit* FirstProjectile(SPGunner* p)
{
    if (p->Projectiles.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SString", 0);
    return (SPProjectileUnit*)g_UnitRegistry->GetPUnit(SStr(p->Projectiles.Array[0]), true);
}

static void PlayEffects(const SUnitArray<SPUnitEffect>& fx, const float* pos)
{
    for (int i = 0; i < fx.Size; ++i) {
        float up[3] = { 0.0f, 1.0f, 0.0f };
        if (g_Pixie)
            g_Pixie->PlayEffect(g_Scene, fx.Array[i].Proto, pos, up, 0);   // pixie +0x24
    }
}

// PANZERS 0x587150
// Ground incidence effects of a direct hit (the first projectile's
// prototype +0x13c), pointing up.
void SGunner::GroundIncidence(const float* pos)
{
    if (Proto->ProjectileCount > 0)
        PlayEffects(FirstProjectile(Proto)->GroundIncidence, pos);
}

// PANZERS 0x587ad0
void SGunner::WaterIncidence(const float* pos)
{
    if (Proto->ProjectileCount > 0)
        PlayEffects(FirstProjectile(Proto)->WaterIncidence, pos);
}

// PANZERS 0x587860
// Unit incidence effects of a direct hit: buildings use the wood (material
// 1) or stone arrays, other units the unit array; played along `dir`.
void SGunner::UnitIncidence(const float* pos, const float* dir, SUnit* hit)
{
    if (Proto->ProjectileCount <= 0)
        return;
    SPProjectileUnit* p = FirstProjectile(Proto);
    const SUnitArray<SPUnitEffect>* fx = &p->UnitIncidence;
    if (ClassOf(hit) == 9)
        fx = ((SPBuildingUnit*)hit->Proto)->BuildingMaterial == 1 ? &p->WoodIncidence : &p->StoneIncidence;
    for (int i = 0; i < fx->Size; ++i)
        if (g_Pixie)
            g_Pixie->PlayEffect(g_Scene, fx->Array[i].Proto, pos, dir, 0);
}

static int FloatBits(float f)
{
    int i;
    memcpy(&i, &f, 4);
    return i;
}

// PANZERS 0x587250
// The shot effects of the gunner prototype (+0x84). The unit's static
// effects pause first (0x5ba860). Effect 0 plays only with `sound` (the
// squad shot cooldown); later ones only while the model is visible. Fire
// weapons spray along the mesh node towards the target (lifetime
// distance / 6.5); bullet indicators every BurstShot shots (distance / 52);
// others at the muzzle or hung on their mesh node.
void SGunner::ShotEffects(const float* pos, const float* dir, bool sound)
{
    SUnit* u = Unit;
    u->DisableStaticEffects();                                // 0x5ba860
    if (!u->_20c)
        return;
    for (int i = sound ? 0 : 1; i < Proto->ShotEffects.Size; ++i) {
        if (i > 0 && !u->Model->GetVisible())                 // model +0x34
            return;
        const SPUnitEffect& fx = Proto->ShotEffects.Array[i];
        const char* mesh = SStr(fx.MeshName);
        if (GetWeaponType() == 3) {
            float dx = u->Pos[0] - Target->Pos[0], dy = u->Pos[1] - Target->Pos[1], dz = u->Pos[2] - Target->Pos[2];
            float life = (float)sqrt((double)(dx * dx + dy * dy + dz * dz)) / 6.5f;   // DAT_007f8410
            float npos[3] = { 0, 0, 0 }, axis[3] = { 0, 0, 0 };
            float tp[3] = { Target->Pos[0], Target->Pos[1], Target->Pos[2] };
            int node = u->Model->FindNode(mesh);              // model +0x40
            u->Model->GetNodePositionAxis(node, npos, axis);  // model +0x50
            float d[3] = { tp[0] - npos[0], tp[1] - npos[1], tp[2] - npos[2] };
            if (g_Pixie)
                g_Pixie->PlayEffect(g_Scene, fx.Proto, npos, d, FloatBits(life));
        } else if (g_Pixie && g_Pixie->IsBulletIndicator(fx.Proto)) {   // pixie +0x5c
            if (BurstLeft >= Proto->BurstShot) {
                BurstLeft = 0;
                float dx = u->Pos[0] - Target->Pos[0], dy = u->Pos[1] - Target->Pos[1], dz = u->Pos[2] - Target->Pos[2];
                float life = (float)sqrt((double)(dx * dx + dy * dy + dz * dz)) / 52.0f;   // DAT_007f8414
                if (fx.MeshName.size == 0) {
                    g_Pixie->PlayEffect(g_Scene, fx.Proto, pos, dir, FloatBits(life));
                } else {
                    float npos[3] = { 0, 0, 0 }, axis[3] = { 0, 0, 0 };
                    float tp[3] = { Target->Pos[0], Target->Pos[1], Target->Pos[2] };
                    int node = u->Model->FindNode(mesh);
                    u->Model->GetNodePositionAxis(node, npos, axis);
                    float d[3] = { tp[0] - npos[0], tp[1] - npos[1], tp[2] - npos[2] };
                    g_Pixie->PlayEffect(g_Scene, fx.Proto, npos, d, FloatBits(life));
                }
            }
            ++BurstLeft;
        } else if (g_Pixie) {
            if (fx.MeshName.size == 0)
                g_Pixie->PlayEffect(g_Scene, fx.Proto, pos, dir, 0);
            else
                g_Pixie->PlayEffectOnNode(g_Scene, fx.Proto, u->Model, u->Model->FindNode(mesh), 0);   // pixie +0x28
        }
    }
}

// SWasterUnit 0x5870f0 WasterSetOwner: waster.cpp (C3).

// PANZERS 0x5d2870
// SWasterUnit: the unit the waster (magnetic mine, fire) is attached to, +0x344.
void WasterSetTarget(SUnit* waster, int unit)
{
    static_cast<SWasterUnit*>(waster)->SetAttachedUnit(unit);
}

// Recompile-only test (PZ_C1_FIRETEST=1, never in the default path): a
// gunner without a target takes the nearest visible enemy within its range,
// so the fire path and the projectiles run before the AI targeting
// (SUnit::FindTarget 0x5b4720) is lifted.
static void C1FireTest(SGunner* g)
{
    static int on = -1;
    if (on < 0) {
        const char* e = getenv("PZ_C1_FIRETEST");
        on = e ? atoi(e) : 0;   // 1 enemies, 2 any unit (all training units are player 0), 3 vehicles at vehicles, 4 projectile weapons
    }
    if (!on || g->Target || !g_GameLogic || !g_World || g_World->Units.Size < 200)   // the mission world only
        return;
    SUnit* u = g->Unit;
    if (u->Proto->ClassType == 5 || u->MainGunner != g->Index || u->Wrecked || u->Unplaced)
        return;
    if (on == 3 && u->Proto->ClassType != 0)                 // 3: vehicles against vehicles only
        return;
    if (on == 4 && (g->Proto->DirectDamage || g->Proto->ProjectileCount <= 0))   // 4: projectile weapons only
        return;
    float best = g->Proto->MaxRange * g->Proto->MaxRange;
    int bi = -1;
    for (int i = 0; i < g_World->Units.Size; ++i) {
        if (!UnitLive(i) || i == u->WorldIndex)
            continue;
        SUnit* t = WorldUnit(i);
        int ct = t->Proto->ClassType;
        if (ct == 3 || ct == 7 || ct == 9 || t->Wrecked || t->Unplaced || t->Parent >= 0 ||
            u->Parent == i || (on == 3 && ct != 0) || (on == 1 && (t->Player == u->Player || WorldIsAlly(u->Player, t->Player))))
            continue;
        float dx = t->Pos[0] - u->Pos[0], dz = t->Pos[2] - u->Pos[2];
        float d2 = dx * dx + dz * dz;
        if (d2 < best && d2 > g->Proto->MinRange * g->Proto->MinRange && (on >= 2 || g_GameLogic->CanSeeGroundUnit(u->Player, t))) {
            best = d2;
            bi = i;
        }
    }
    static int logged = 0;
    if (bi < 0) {
        if (logged < 40 && g_GameLogic->GetFrame() % 200 == 0) {
            ++logged;
            float nd = 1e30f;
            int ni = -1;
            for (int i = 0; i < g_World->Units.Size; ++i) {
                if (!UnitLive(i) || i == u->WorldIndex)
                    continue;
                SUnit* t = WorldUnit(i);
                if (t->Player == u->Player || WorldIsAlly(u->Player, t->Player) || t->Unplaced)
                    continue;
                float dx = t->Pos[0] - u->Pos[0], dz = t->Pos[2] - u->Pos[2];
                if (dx * dx + dz * dz < nd) {
                    nd = dx * dx + dz * dz;
                    ni = i;
                }
            }
            Logger.g->Log(0, "PZC1 firetest: unit %d player %d range %.1f nearest enemy %d dist %.1f", u->WorldIndex,
                          u->Player, g->Proto->MaxRange, ni, sqrtf(nd));
        }
        return;
    }
    STarget* t = STarget::Create(0);
    t->Type = kTargetUnit;
    t->Unit = bi;
    t->Pos[0] = WorldUnit(bi)->Pos[0];
    t->Pos[1] = WorldUnit(bi)->Pos[1];
    t->Pos[2] = WorldUnit(bi)->Pos[2];
    g->SetTarget(t);
    g->Active = 1;
    Logger.g->Log(0, "PZC1 firetest: unit %d (gunner %d '%s' direct %d proj %d weapon %d) targets %d", u->WorldIndex, g->Index,
                  SStr(u->Proto->Name), g->Proto->DirectDamage ? 1 : 0, g->Proto->ProjectileCount, g->Proto->WeaponType, bi);
}

// PANZERS 0x584d00
// Per tick: the target refresh, the turret aim, the kick decay, reloading
// (with the rank bonus of experienced crews), and then, with a target, the
// range / line-of-fire / fire-arc checks and the fire stage (shot delay,
// burst, speech, shot effects, then a projectile unit or direct damage).
void SGunner::ServerRefresh()
{
    C1FireTest(this);                                         // recompile-only, PZ_C1_FIRETEST=1
    if (Active == 0 && Proto->GunnerType != 1)
        return;
    if (Target && !PzTargetRefresh(Target, Unit->WorldIndex))   // STarget::Refresh 0x5bd210
        Stop();                                               // +0x28
    SUnit* u = Unit;
    int ct = ClassOf(u);
    if ((ct == 5 || ct == 6) && u->MainGunner != Index)
        return;
    if (PendingTarget && _68 == 0) {
        u->SetCurrentTarget(PendingTarget, 0);                // unit +0xa0
        if (PendingTarget) {
            PzTargetRelease(PendingTarget);
            PendingTarget = nullptr;
        }
    }
    if (Proto->Fixed == 0) {
        if (!Target) {
            TurnTurret(RestAngle);                            // +0x04(+0x2c)
            TurnElevation(AimAngle);                          // +0x0c(+0x44)
        } else {
            u = Unit;
            AimWorld(DAtan2f((double)(Target->Pos[0] - u->Pos[0]), (double)(Target->Pos[2] - u->Pos[2])));   // +0x00
            if (Proto->GunnerType == 1) {
                // Air defence: the barrel follows the target's elevation
                // above the gun (gunner height +0x20).
                float dx = Target->Pos[0] - u->Pos[0];
                float dz = Target->Pos[2] - u->Pos[2];
                float dist = (float)sqrt((double)(dx * dx + dz * dz));
                AimElevation(DAtan2f((double)(Target->Pos[1] - u->Pos[1] - Proto->GunnerHeight), (double)dist));   // +0x08
            }
        }
    }
    const float zero = 0.0f;                                  // the 0.0 local [ebp-0x60]
    if (KickLeft == 0) {
        Kick = 0;
    } else {
        int k = KickLeft--;
        float v = ((float)k * Proto->KickSize) / (float)Proto->KickTime;
        unsigned bits;
        memcpy(&bits, &v, 4);
        Kick = bits ^ 0x80000000u;                            // DAT_007f5ac0 (negate)
    }
    if (_6c == 0 && !Loaded) {
        if (ReloadLeft != 0) {
            ReloadLeft--;
            _68 = 0;
            return;
        }
        if (zero < AmmoLeft || GetWeaponType() == 0) {
            ConsumeAmmo();                                    // 0x583e30
            Loaded = true;
            ReloadLeft = Proto->ReloadTime;
            // Experienced squad members (rank 2: 5 %, 3..4: 10 % a rank)
            // and crews of unit type 0xe (rank 1: 5 %, 2..4: 10 % a rank)
            // reload faster.
            SUnit* ru = Unit;
            int mode = 0;                                     // 0 none, 1 five percent, 2 ten percent a rank
            if (ClassOf(ru) == 6 && ru->Parent > -1 &&
                ClassOf(WorldUnit(ru->Parent)) == 5 && WorldUnit(ru->Parent)->Proto->UnitType != 0xe) {
                int rank = ru->GetRank();                     // +0x88
                if (rank == 2)
                    mode = 1;
                else if ((unsigned)(rank - 3) <= 1)
                    mode = 2;
            } else if (ClassOf(ru) == 0 && ru->Proto->UnitType == 0xe) {
                int rank = ru->GetRank();
                if (rank == 1)
                    mode = 1;
                else if ((unsigned)(rank - 2) <= 2)
                    mode = 2;
            }
            if (mode == 1) {
                ReloadLeft = (int)((float)ReloadLeft - (float)Proto->ReloadTime * 0.05f);   // DAT_007f4534
            } else if (mode == 2) {
                float left = (float)ReloadLeft;
                float r = (float)ru->GetRank();
                ReloadLeft = (int)(left - r * ((float)Proto->ReloadTime * 0.1f));          // DAT_007f59a8
            }
        } else {
            // Out of ammunition: a squad member hands the target to the
            // next gunner (grenade, special weapon) if it can take it.
            if (AmmoLeft == -1.0f)                            // DAT_007f5a98
                return;
            if (!Target)
                return;
            SUnit* su = Unit;
            if (ClassOf(su) == 6)
                return;
            int next = su->MainGunner + 1;
            if ((int)su->Proto->GunnerCount <= next)
                return;
            if (su->MainGunner > 1)
                return;
            su->MainGunner = next;
            if (Target->Type == 0) {
                if (!UnitLive(Target->Unit))
                    Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", Target->Unit);
                SUnit* tu = WorldUnit(Target->Unit);
                if (UnitGunner(su, su->MainGunner)->CanTargetUnit(tu, true)) {
                    su->SetCurrentTarget(Target, 0);          // unit +0xa0
                    Target = nullptr;                         // HD drops the pointer without a release
                }
            }
            goto release;
        }
    }
    if (!Target || Idle) {
        _6c = 0;
        _68 = 0;
        return;
    }
    {
        float tx = Target->Pos[0], ty = Target->Pos[1], tz = Target->Pos[2];
        float aimX = tx, aimZ = tz;                           // [ebp-0x1c] / [ebp-0x24] (shot spread)
        if (_68 == 0 && ClassOf(Unit) != 7) {
            // ---- range, visibility and line-of-fire checks
            float maxR2 = (float)(((double)Proto->MaxRange + 0.1) * ((double)Proto->MaxRange + 0.1));   // 0x7f83e0
            float minR2 = (float)(((double)Proto->MinRange - 0.1) * ((double)Proto->MinRange - 0.1));
            SUnit* uu = Unit;
            int uct = ClassOf(uu);
            if (uct == 5) {
                float r = uu->GetLowestMaxRange();            // +0x178
                float r2 = uu->GetLowestMaxRange();
                minR2 = zero;
                maxR2 = r2 * r;
            }
            if (ClassOf(uu) == 6 && uu->Parent > -1) {
                SUnit* p = WorldUnit(uu->Parent);
                if (p->CurrentTarget && p->CurrentTarget->Type == 0 && p->CurrentTarget->Mode != 1)
                    return;
            }
            if (ClassOf(Unit) == 6 && (Index == 1 || Index == 2)) {
                int rank = Unit->GetRank();
                if ((unsigned)rank <= 4) {
                    int g = g_UnitRegistry->GrenadeMaxRange[rank];   // +0xc4..+0xd4
                    maxR2 = (float)(g * g);
                }
            }
            SUnit* su = Unit;
            float ox = su->Pos[0], oz = su->Pos[2];
            if (su->Parent > -1) {
                SUnit* p = WorldUnit(su->Parent);
                if (ClassOf(p) == 9 && *(int*)((unsigned char*)p + 0x3f8) >= 0) {   // SBuildingUnit +0x3f8 WindowSet
                    int ws = *(int*)((unsigned char*)p + 0x3f8);
                    const float* w = BuildingWindow(p, ws);
                    ox = w[0];
                    oz = w[1];
                    float ang = DAtan2f((double)(tx - ox), (double)(tz - oz));
                    if (DAngleDist((double)ang, (double)BuildingWindow(p, ws)[2]) > 1.1780972480773926)   // 0x551bc0, 0x7f83f8
                        return;
                } else {
                    ox = p->Pos[0];
                    oz = p->Pos[2];
                }
            }
            bool visible = VisibleUnit(Unit, Target->Unit) != nullptr;   // 0x56d1d0
            if (Target->Type == 0) {
                if (VisibleUnit(nullptr, Target->Unit) && ClassOf(WorldUnit(Target->Unit)) == 6)
                    goto aim;
            }
            {
                float dz = Target->Pos[2] - oz, dx = Target->Pos[0] - ox;
                float d2 = dz * dz + dx * dx;
                int type = Target->Type;
                if (d2 > maxR2 || ((type == 0 || type == 1) && !visible)) {
                    // out of range or not seen
                    if (ClassOf(Unit) == 6)
                        return;
                    if (Unit->CurrentTarget == Target) {
                        Target->Mode = 0;
                        return;
                    }
                    goto release;
                }
                bool tooClose = minR2 > d2;
                if (!tooClose && (type == 0 || type == 1) && !visible) {
                    if (!UnitLive(Target->Unit))
                        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", Target->Unit);
                    tooClose = ClassOf(WorldUnit(Target->Unit)) != 8;
                }
                if (tooClose) {
                    if (ClassOf(Unit) == 6)
                        return;
                    if (Unit->CurrentTarget == Target) {
                        Target->Mode = 2;
                        return;
                    }
                    goto release;
                }
            }
            if (Target->Type == 0 && !HasLineOfFire(Target->Unit)) {   // 0x5847e0
                if (ClassOf(Unit) == 6)
                    return;
                if (Unit->CurrentTarget == Target) {
                    Target->Mode = 0;
                    return;
                }
                goto release;
            }
            if (ClassOf(Unit) != 6)
                Target->Mode = 1;
            if (Unit->MainGunner == Index && Target->Type == 0) {
                if (!UnitLive(Target->Unit))
                    Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", Target->Unit);
                if (!CanAttack(WorldUnit(Target->Unit))) {   // 0x583af0
                    SUnit* au = Unit;
                    int act = ClassOf(au);
                    if (act == 6) {
                        Stop();                               // +0x28
                        return;
                    }
                    if (act == 5)
                        return;
                    if (au->CurrentTarget == au->PrimaryTarget) {
                        au->ClearTargets();                   // +0xc4
                        return;
                    }
                    if (au->MainGunner != Index)
                        return;
                    if (au->CurrentTarget) {
                        PzTargetRelease(au->CurrentTarget);
                        au->CurrentTarget = nullptr;
                    }
                    au->AI_Heartbeat();                       // +0x190
                    return;
                }
            }
            {
                int type = Target->Type;
                SUnit* au = Unit;
                if ((type == 0 || type == 1) && au->MainGunner == Index && ClassOf(au) == 0) {
                    float dz = Target->Pos[2] - oz, dx = Target->Pos[0] - ox;
                    float d2 = dz * dz + dx * dx;
                    float r = au->GetLowestMaxRange();        // +0x178
                    float r2 = au->GetLowestMaxRange() * r;
                    if (d2 > r2 && au->CurrentTarget == Target)
                        Target->Mode = 0;
                }
                int ut = Unit->Proto->UnitType;
                if ((ut == 0x15 && Unit->GlobalState == 2) || (ut == 0x12 && Unit->GlobalState == 0)) {
                    ((SPanzersSquadUnit*)Unit)->SetSquadBehavior(1);   // 0x599450 (on the unit itself)
                    return;
                }
            }
aim:
            {
                // ---- the turret must point at the target within the fire arc
                SUnit* au = Unit;
                int parent = Proto->ParentGunner;
                double err = GunnerAimError(au->Pos[0], au->Pos[2], tx, tz, au->Dir, parent > -1,
                                            parent > -1 ? UnitGunner(au, parent)->Turret.Angle : 0.0f,
                                            Proto->FireStartArc, Turret.Angle);
                if (err > (double)Proto->FireArc + 0.0001) {  // 0x7f1b50
                    if (IsInArc(tx, ty, tz))                  // 0x583990
                        return;
                    if (ClassOf(Unit) == 6)
                        return;
                    if (Unit->ActiveDriver == -1)
                        goto release;
                    if (Unit->CurrentTarget == Target)
                        return;
                    goto release;
                }
            }
        }

        // ---- the fire stage
        if (_60 > 0) {
            _60--;
            return;
        }
        if (_68 != 0) {
            _68--;
        } else if (_6c == 0) {
            _68 = Proto->ShotDelay;
            Unit->_154 = (Unit->_154 & 0xff00) | 1;           // byte +0x154
        }
        if (_68 > 0)
            return;
        if (ClassOf(Unit) == 5)
            return;
        if (Target->Type == 0) {
            if (!UnitLive(Target->Unit))
                Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", Target->Unit);
            int pl = WorldUnit(Target->Unit)->Player;
            for (int i = 0; i < 12; ++i) {
                if (WorldIsAlly(i, pl)) {                     // 0x549ab0
                    if (Unit->Parent >= 0)
                        WorldUnit(Unit->Parent)->_2cc[i] = false;
                    Unit->_2cc[i] = false;
                }
            }
        }
        if (Unit->Parent > -1)
            WorldUnit(Unit->Parent)->UpdateSeenByPlayers();   // 0x5bc5c0
        else
            Unit->UpdateSeenByPlayers();
        if (g_Campaign)
            ++*(int*)((unsigned char*)g_Campaign + 0x214 + Unit->Player * 0xd8);   // shots fired statistic
        if (Proto->BurstShot > 0) {
            if (_6c == 0) {
                _6c = (Proto->BurstShot - 1) * Proto->ReshotTime;
            } else {
                _6c--;
                if (_6c % Proto->ReshotTime > 0)
                    return;
                if (!(zero < AmmoLeft) && GetWeaponType() != 0) {
                    _6c = 0;
                    return;
                }
                ConsumeAmmo();
            }
        }
        if (Proto->HandToHand) {
            int type = Target->Type;
            if (type == 1)
                WorldUnit(Target->Unit2)->OnAttackedBy(Unit->WorldIndex);   // +0x194
            if (type == 0 || type == 1)
                WorldUnit(Target->Unit)->OnAttackedBy(Unit->WorldIndex);
        }
        float muzzle[3] = { Unit->Pos[0], Unit->Pos[1], Unit->Pos[2] };
        if (Anim) {
            float tmp[3];
            float* m = Anim->GetFirePosition(tmp, Index, Turret.Angle, Proto->BodyKick);   // anim +0x10
            muzzle[0] = m[0];
            muzzle[1] = m[1];
            muzzle[2] = m[2];
        }
        bool sound = true;
        if (Unit->Parent >= 0 && ClassOf(WorldUnit(Unit->Parent)) == 5) {
            SUnit* sq = WorldUnit(Unit->Parent);
            sound = g_GameLogic->GetFrame() - sq->_1f0 >= 12;   // 0x56d1a0
            if (sound)
                WorldUnit(Unit->Parent)->_1f0 = g_GameLogic->GetFrame();
        }
        if (ClassOf(Unit) != 7) {
            float dir[3] = { tx - Unit->Pos[0], ty - Unit->Pos[1], tz - Unit->Pos[2] };
            ShotEffects(muzzle, dir, sound);                  // 0x587250
        }
        Loaded = false;
        KickLeft = Proto->KickTime;
        if (!Proto->DirectDamage && Proto->ProjectileCount > 0) {
            FireProjectile(muzzle, tx, ty, tz, aimX, aimZ);
            goto tail;
        }
        FireDirect(tx, ty, tz);
    }
tail:
    {
        SUnit* tu = Unit;
        if (ClassOf(tu) != 6)
            return;
        if (tu->MainGunner < 0)
            return;
        if (tu->MainGunner == tu->Proto->MainGunner)
            return;
    }
release:
    if (Target) {
        PzTargetRelease(Target);                              // 0x5bdef0
        Target = nullptr;
    }
}

// 0x584d00 from 0x585cf8: speech, shot spread (two world-LCG draws), the
// projectile unit (SWorld::CreateUnit 0x5e3170 with the first projectile
// prototype at the muzzle), its velocity (straight for rocket propulsion,
// else ballistic), the shooter, damage and the fire weapon fields.
void SGunner::FireProjectile(const float* muzzle, float tx, float ty, float tz, float aimX, float aimZ)
{
    SUnit* u = Unit;
    if (ClassOf(u) == 6) {
        if (u->MainGunner == 1)
            g_World->UnitSpeech(u->WorldIndex, 0x14, false);  // 0x5fff20
        if (Unit->MainGunner == 2)
            g_World->UnitSpeech(Unit->WorldIndex, 0x14, false);
    }
    if (!(Proto->ShotSpread == 0.0f)) {
        float dist = (float)(sqrt(DRandDouble(1.0)) * (double)Proto->ShotSpread);   // 0x559740
        float a = (float)DRandDouble(6.2831854820251465);
        aimX = (float)(DSin((double)a) * (double)dist + (double)tx);
        aimZ = (float)(DCos((double)a) * (double)dist + (double)tz);
    }
    float dir = DAtan2f((double)(aimX - Unit->Pos[0]), (double)(aimZ - Unit->Pos[2]));
    if (Proto->Projectiles.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SString", 0);
    const char* name = SStr(Proto->Projectiles.Array[0]);
    float pos[3] = { muzzle[0], muzzle[1], muzzle[2] };
    int proj = g_World->CreateUnit(Unit->Player, name, pos, dir, 0, 1.0f, -1, true, "");   // 0x5e3170
    SUnit* pu = WorldUnit(proj);
    pu->_20c = Unit->_20c;
    SIDriver* drv = pu->GetDriver(0);                         // 0x55cc00
    SPProjectileDriver* pd = (SPProjectileDriver*)drv->GetPDriver();
    float vel[3];
    if (pd->RocketPropulsion) {                               // driver prototype +0x44
        float dy = ty;
        if (Target->Type == 0) {
            if (ClassOf(WorldUnit(Target->Unit)) != 5)
                dy = ty + 0.8f;                               // DAT_007f83dc
        }
        float speed = ((SPProjectileDriver*)WorldUnit(proj)->GetDriver(0)->GetPDriver())->MaxSpeed;   // +0x08
        GunnerStraightVelocity(aimX - pos[0], dy - pos[1], aimZ - pos[2], speed, vel);
    } else {
        GunnerBallisticVelocity(aimX - pos[0], ty - pos[1], aimZ - pos[2], Proto->ShotAngle + 0.0f, vel);
    }
    if (!_finite((double)vel[0]) || !_finite((double)vel[1]) || !_finite((double)vel[2]))   // 0x793d6c
        Logger.g->Panic("SGunner::ServerRefresh: Projectile speed is not finite!");
    int owner = Unit->WorldIndex;
    while (WorldUnit(owner)->Parent >= 0)
        owner = WorldUnit(owner)->Parent;
    SProjectileUnit* p = (SProjectileUnit*)WorldUnit(proj);
    p->SetVelocity(vel[0], vel[1], vel[2]);                   // 0x5a4450
    p->Shooter = owner;                                       // +0x354
    int weapon = GetWeaponType();
    p->SetDamage(GetDamage(), Proto->DamageRadius, weapon);   // 0x583ed0, 0x5a4410
    if (GetWeaponType() == 3 && Unit->MainGunner == 0) {
        p->Burning = true;                                    // +0x359
        p->Start[0] = p->Pos[0];
        p->Start[1] = p->Pos[1];
        p->Start[2] = p->Pos[2];
        p->Range = Proto->MaxRange;                           // +0x368
    }
}

// 0x584d00 from 0x586402: direct damage (hand to hand, bullets, and
// direct-damage weapons) with the incidence effects.
void SGunner::FireDirect(float tx, float ty, float tz)
{
    if (Target->Type == 0) {
        if (!UnitLive(Target->Unit))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", Target->Unit);
        if (WorldUnit(Target->Unit)->Wrecked)
            return;
        float dmg = GetDamage();                              // 0x583ed0
        int victim = ClassOf(WorldUnit(Target->Unit)) == 5 ? ((SPanzersSquadUnit*)WorldUnit(Target->Unit))->GetARandomMemberIdx()   // 0x59b3d0
                                                          : Target->Unit;
        SUnit* u = Unit;
        if (u->MainGunner == 3 && ClassOf(u) == 6 && ClassOf(WorldUnit(Target->Unit)) != 5) {
            // A soldier places a magnetic mine on the vehicle.
            float pos[3];
            pos[0] = u->Pos[0];
            pos[1] = g_World->GetTerrainHeight(u->Pos[0], u->Pos[2]);   // 0x5e7730
            pos[2] = u->Pos[2];
            int mine = g_World->CreateUnit(u->Player, "Waster Magnetic Mine", pos, 0.0f, 0, 1.0f, -1, true, "");
            WasterSetTarget(WorldUnit(mine), Target->Unit);   // 0x5d2870
            WasterSetOwner(WorldUnit(mine), Unit->WorldIndex); // 0x5870f0
            if (Target) {
                PzTargetRelease(Target);
                Target = nullptr;
            }
            return;
        }
        SUnit* vu = nullptr;
        if (ClassOf(u) == 7) {
            if (u->Proto->UnitType == 0x19 && WorldUnit(victim)->Proto->ArmourType == 2) {
                SUnit* v = WorldUnit(victim);
                if (v->PrimaryTarget) {
                    PzTargetRelease(v->PrimaryTarget);
                    WorldUnit(victim)->PrimaryTarget = nullptr;
                }
                WorldUnit(victim)->LockDriver();              // 0x5b8260
                g_World->UnitSpeech(victim, 0x12, false);
            }
            vu = WorldUnit(victim);
            SUnit* au = Unit;
            vu->TakeDamage(dmg, GetWeaponType(), au->WorldIndex, au->Pos[0], au->Pos[1], au->Pos[2], 2);   // +0x94
            au = Unit;
            g_GameLogic->DamageArea(GetDamage(), au->WorldIndex, victim, au->Pos[0], au->Pos[1], au->Pos[2],
                                    Proto->DamageRadius, 2, GetWeaponType());   // 0x576490
        } else if (GetWeaponType() == 2) {
            if (ClassOf(WorldUnit(Target->Unit)) == 5) {
                g_GameLogic->DamageArea(dmg, Unit->WorldIndex, -1, Target->Pos[0], Target->Pos[1], Target->Pos[2],
                                        Proto->DamageRadius, 0, 2);
            } else {
                vu = WorldUnit(victim);
                SUnit* au = Unit;
                vu->TakeDamage(dmg, GetWeaponType(), au->WorldIndex, au->Pos[0], au->Pos[1], au->Pos[2], 0);
                SUnit* v = WorldUnit(victim);
                g_GameLogic->DamageArea(dmg, Unit->WorldIndex, victim, v->Pos[0], v->Pos[1], v->Pos[2],
                                        Proto->DamageRadius, 0, GetWeaponType());
            }
        } else if (GetWeaponType() == 3 || GetWeaponType() == 1) {
            vu = WorldUnit(victim);
            SUnit* au = Unit;
            vu->TakeDamage(dmg, GetWeaponType(), au->WorldIndex, au->Pos[0], au->Pos[1], au->Pos[2], 0);
            if (Target)
                g_GameLogic->DamageArea(GetDamage(), Unit->WorldIndex, Target->Unit, Target->Pos[0], Target->Pos[1],
                                        Target->Pos[2], Proto->DamageRadius, 0, GetWeaponType());
        } else {
            vu = WorldUnit(victim);
            SUnit* au = Unit;
            vu->TakeDamage(dmg, GetWeaponType(), au->WorldIndex, au->Pos[0], au->Pos[1], au->Pos[2], 0);
        }
        // ---- incidence effects (visual)
        SUnit* v = WorldUnit(victim);
        SUnit* au = Unit;
        float dx = v->Pos[0] - au->Pos[0];
        float dz = v->Pos[2] - au->Pos[2];
        double inv = 1.0 / sqrt((double)(dx * dx + 0.0f + dz * dz));
        float nx = (float)((double)dx * inv);
        float ny = (float)(inv * 0.0);
        float nz = (float)((double)dz * inv);
        float yaw = DAtan2f((double)nx, (double)nz);
        double f;
        if (ClassOf(WorldUnit(victim)) == 0xb) {
            f = 0.4;                                          // 0x7f5a18
        } else if (0.7853981852531433 > DAngleDist((double)WorldUnit(victim)->Dir, (double)yaw)) {   // 0x7f7f50
            f = 1.1;                                          // 0x7f83f0
        } else if (DAngleDist((double)WorldUnit(victim)->Dir, (double)yaw) > 2.35619455575943) {     // 0x7f7f58
            f = 1.1;
        } else {
            f = 0.7;                                          // 0x7f5a28
        }
        float a = (float)((double)nx * f), b = (float)((double)ny * f), c = (float)((double)nz * f);
        float size = WorldUnit(victim)->UnitSize;             // +0x54
        a = a * size * 0.5f;
        b = b * size * 0.5f;
        c = c * size * 0.5f;
        if (ClassOf(Unit) != 7 && ClassOf(WorldUnit(victim)) != 6) {
            SUnit* vv = WorldUnit(victim);
            if (ClassOf(vv) == 9) {
                // Step back out of the building model along the shot.
                SUnit* su = Unit;
                float ex = vv->Pos[0] - su->Pos[0], ey = vv->Pos[1] - su->Pos[1], ez = vv->Pos[2] - su->Pos[2];
                double einv = 1.0 / sqrt((double)(ey * ey + ex * ex + ez * ez));
                float sx = (float)((double)ex * einv) * 0.1f;   // DAT_007f59a8
                float sy = (float)((double)ey * einv) * 0.1f;
                float sz = (float)((double)ez * einv) * 0.1f;
                float p[3] = { vv->Pos[0], vv->Pos[1], vv->Pos[2] };
                p[1] = p[1] + 1.0f;
                int steps = 0;
                for (;;) {
                    p[0] = p[0] - sx;
                    p[1] = p[1] - sy;
                    p[2] = p[2] - sz;
                    if (!WorldUnit(victim)->Model->HitTestPoint(p))   // building model (+0x08) +0xd0
                        break;
                    if (++steps >= 100)
                        break;
                }
                if (steps < 100) {
                    float up[3] = { 0.0f, 1.0f, 0.0f };
                    UnitIncidence(p, up, WorldUnit(victim));  // 0x587860
                }
            } else {
                float p[3] = { vv->Pos[0] - a, (vv->Pos[1] - b) + 0.5f, vv->Pos[2] - c };
                float d[3] = { -a, -b, -c };
                UnitIncidence(p, d, vv);
            }
            return;
        }
        if (ClassOf(Unit) == 7)
            return;
    } else {
        int w = GetWeaponType();
        if (w == 3 || w == 1)
            g_GameLogic->DamageArea(GetDamage(), Unit->WorldIndex, Target->Unit, Target->Pos[0], Target->Pos[1], Target->Pos[2],
                                    Proto->DamageRadius, 2, GetWeaponType());
        else
            g_GameLogic->DamageArea(GetDamage(), Unit->WorldIndex, -1, Target->Pos[0], Target->Pos[1], Target->Pos[2],
                                    Proto->DamageRadius, 0, GetWeaponType());
        if (ClassOf(Unit) == 7)
            return;
    }
    // ---- ground or water incidence at the target point
    float g = g_World->GetTerrainHeight(tx, tz);              // 0x5e7730
    float w = g_World->GetWaterHeight(tx, tz) - 0.01f;        // 0x5ec490, x87 fsub (CW 0x007F)
    float p[3] = { tx, 0.0f, tz };
    (void)ty;
    if (w > g) {
        p[1] = g_World->GetWaterHeight(tx, tz);
        WaterIncidence(p);                                    // 0x587ad0
    } else {
        p[1] = g_World->GetTerrainHeight(tx, tz);
        GroundIncidence(p);                                   // 0x587150
    }
}

} // namespace pz
