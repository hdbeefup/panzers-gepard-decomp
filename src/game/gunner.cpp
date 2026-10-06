// src/game/gunner.cpp
// SPGunner / SGunner (0x582a00..0x587d00). OWNER: agent U. See gunner.h.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gunner.h"
#include "unit.h"
#include "unitprops.h"
#include "unitextern.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/ipixie.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"

namespace pz {

void FreeSString(SString* s);

static const double kPi = 3.1415927410125732;        // DAT_007f4560
static const double kTwoPi = 6.2831854820251465;     // DAT_007f4570
static const float kDeg = 0.01745329238474369f;      // DAT_007f59a0

// HD angle wrap into (-pi, pi] on doubles (inline everywhere).
static double WrapAngle(double a)
{
    if (a <= kPi) {
        if (a < -kPi)
            a += kTwoPi;
    } else {
        a -= kTwoPi;
    }
    return a;
}

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
        // HD: pixie +0x10 LoadEffectPrototype. Not loaded (-1): the
        // recompile's SPixie crashes on nested prototype loads (see
        // LoadEffectArray in punit.cpp); these effects play only on firing.
        fx->Proto = -1;
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

// PANZERS 0x583070
// Turns the turret (+0x30) towards `angle` (clamped to the prototype's
// left/right limits) with an acceleration of TurretStep 0..20 ticks; true
// when it arrived.
bool SGunner::TurnTurret(float angle)
{
    if (TurretGoal != angle || TurretFlip) {
        float lo = Proto->MaxLeftAngle, hi = Proto->MaxRightAngle;
        float a = angle;
        if (-3.14159f < lo && hi < 3.14159f) {               // _DAT_007f841c / DAT_007f8400
            if (angle > hi)
                a = hi;
            else if (angle < lo)
                a = lo;
        }
        angle = a;
        bool same;
        if (TurretStep == 0) {
            same = true;
        } else {
            double d0 = WrapAngle((double)TurretGoal - (double)TurretAngle);
            double s0 = d0 < 0.0 ? -1.0 : (d0 > 0.0 ? 1.0 : 0.0);
            double d1 = WrapAngle((double)angle - (double)TurretAngle);
            double s1 = d1 < 0.0 ? -1.0 : (d1 > 0.0 ? 1.0 : 0.0);
            same = s0 == s1;
            if (!same) {
                TurretFlip = true;
                TurretDir = -1;
            }
        }
        if (same) {
            TurretFlip = false;
            TurretGoal = angle;
            TurretDir = 1;
        }
    }
    float goal = TurretGoal;
    float stepA = (float)(((double)TurretStep / 20.0) * (double)Proto->TurretSpeed);
    double diff = (double)goal - (double)TurretAngle;
    double ad = fabs(diff);
    double dist = ad <= kPi ? ad : kTwoPi - ad;
    double unit = (double)Proto->TurretSpeed / 20.0;
    if (dist <= (double)stepA + unit) {
        TurretDir = 0;
        TurretStep = 0;
        TurretAngle = goal;
        return true;
    }
    if (TurretStep != 0 && dist < (double)(((float)unit + stepA) * 0.5f * (float)TurretStep - stepA * 0.5f))
        TurretDir = -1;
    TurretStep += TurretDir;
    if (TurretStep < 0)
        TurretStep = 0;
    if (TurretStep > 20)
        TurretStep = 20;
    stepA = (float)(((double)TurretStep / 20.0) * (double)Proto->TurretSpeed);
    if (stepA <= 1e-5f) {                                     // DAT_007f1038
        if (goal == angle) {
            TurretDir = 0;
            TurretStep = 0;
            TurretAngle = goal;
            return true;
        }
        return false;
    }
    diff = WrapAngle(diff);
    double sgn = diff < 0.0 ? -1.0 : (diff > 0.0 ? 1.0 : 0.0);
    TurretAngle = (float)WrapAngle((double)stepA * sgn + (double)TurretAngle);
    return false;
}

// PANZERS 0x583550
void SGunner::Slot_08(float p1)
{
    TurnElevation(p1);
}

// PANZERS 0x583570
// The barrel elevation (+0x48) towards `angle` (0..DAT_007f83e8), same
// acceleration scheme as TurnTurret.
bool SGunner::TurnElevation(float angle)
{
    if (ElevGoal != angle || ElevFlip) {
        const float maxElev = 1.5707964f;                    // DAT_007f83e8 (not verified)
        if (angle > maxElev)
            angle = maxElev;
        else if (angle < 0.0f)
            angle = 0.0f;
        bool same;
        if (ElevStep == 0) {
            same = true;
        } else {
            double d0 = WrapAngle((double)ElevGoal - (double)Elevation);
            double s0 = d0 < 0.0 ? -1.0 : (d0 > 0.0 ? 1.0 : 0.0);
            double d1 = WrapAngle((double)angle - (double)Elevation);
            double s1 = d1 < 0.0 ? -1.0 : (d1 > 0.0 ? 1.0 : 0.0);
            same = s0 == s1;
            if (!same) {
                ElevFlip = true;
                ElevDir = -1;
            }
        }
        if (same) {
            ElevFlip = false;
            ElevGoal = angle;
            ElevDir = 1;
        }
    }
    float goal = ElevGoal;
    float stepA = (float)(((double)ElevStep / 20.0) * (double)Proto->TurretSpeed);
    double diff = (double)goal - (double)Elevation;
    double ad = fabs(diff);
    double dist = ad <= kPi ? ad : kTwoPi - ad;
    double unit = (double)Proto->TurretSpeed / 20.0;
    if (dist <= (double)stepA + unit) {
        ElevDir = 0;
        ElevStep = 0;
        Elevation = goal;
        return true;
    }
    if (ElevStep != 0 && dist < (double)(((float)unit + stepA) * 0.5f * (float)ElevStep - stepA * 0.5f))
        ElevDir = -1;
    ElevStep += ElevDir;
    if (ElevStep < 0)
        ElevStep = 0;
    if (ElevStep > 20)
        ElevStep = 20;
    stepA = (float)(((double)ElevStep / 20.0) * (double)Proto->TurretSpeed);
    if (stepA <= 1e-5f) {
        if (goal == angle) {
            ElevDir = 0;
            ElevStep = 0;
            Elevation = goal;
            return true;
        }
        return false;
    }
    diff = WrapAngle(diff);
    double sgn = diff < 0.0 ? -1.0 : (diff > 0.0 ? 1.0 : 0.0);
    Elevation = (float)WrapAngle((double)stepA * sgn + (double)Elevation);
    return false;
}

// PANZERS 0x582f70
// A world angle into the turret frame (unit dir, the parent gunner's angle,
// the fire arc start), then +0x04.
void SGunner::Slot_10(float p1)
{
    float base = Unit->Dir;
    int parent = Proto->ParentGunner;
    if (parent >= 0) {
        if (parent >= Unit->Gunners.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", parent);
        base = (float)WrapAngle((double)Unit->Gunners.Array[parent]->TurretAngle + (double)base);
    }
    double a = WrapAngle((double)Proto->FireStartArc + (double)base);
    TurnTurret((float)WrapAngle((double)p1 - a));
}

// PANZERS 0x584d00 (prologue)
// Lifted: the early returns, the pending-target hand-over to the unit, the
// rest-angle turret turn, the kick decay and the reload counter, up to the
// return of a gunner without a target. Targeting, aiming and firing (the
// remaining ~2000 instructions) are not lifted: the menu never has a target.
unsigned SGunner::ServerRefresh()
{
    if (Active == 0 && Proto->GunnerType != 1)
        return 0;
    if (Target && !PzTargetRefresh(Target, Unit->WorldIndex))   // STarget::Refresh 0x5bd210
        Stop();                                               // +0x28
    SUnit* u = Unit;
    int ct = u->Proto->ClassType;
    if ((ct == 5 || ct == 6) && u->MainGunner != Index)
        return 0;
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
            STUB_LOG("SGunner::ServerRefresh (0x584d00) aim at target");
            PZ_M2_TRACE("SGunner::ServerRefresh (0x584d00) aim at target");
        }
    }
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
            return 0;
        }
        if (0.0f < AmmoLeft) {
            ConsumeAmmo();                                    // 0x583e30
            Loaded = true;
            ReloadLeft = Proto->ReloadTime;
            // HD then shortens the reload of squad members and crews by
            // their rank (unit +0x88); no menu unit carries XP.
        }
    }
    if (!Target || Idle) {
        _6c = 0;
        _68 = 0;
        return 0;
    }
    STUB_LOG("SGunner::ServerRefresh (0x584d00) fire at target");
    PZ_M2_TRACE("SGunner::ServerRefresh (0x584d00) fire at target");
    return 1;
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
    RestAngle = (float)WrapAngle((double)angle - (double)Unit->Dir);
    AimAngle = (float)WrapAngle((double)angle - (double)Unit->Dir);
}

// PANZERS 0x584cd0
void SGunner::ResetToParent()
{
    if (Target) {
        PzTargetRelease(Target);
        Target = nullptr;
    }
    Idle = true;
    RestAngle = TurretAngle;
    AimAngle = TurretAngle;
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

// PANZERS 0x583e30
// Ammunition of non-bullet weapons: AmmoLeft -= 1 / Ammo, with the "low"
// and "out of ammo" world events (0x5fff20) at the thresholds. Squads and
// bullet weapons (the menu infantry) return at once.
void SGunner::ConsumeAmmo()
{
    if (Unit->Proto->ClassType == 5)
        return;
    if (Proto->WeaponType == 0)                               // 0x584240 (squad member's first gunner)
        return;
    if (Unit->Proto->ClassType == 6 && Index == 0)
        return;
    float before = AmmoLeft;
    float after = before - 1.0f / (float)Proto->Ammo;         // DAT_007f1b58
    AmmoLeft = after;
    const float low = 0.2f;                                   // DAT_007f83d8 (not verified)
    if ((after < low && low <= before) || after <= 0.0f) {
        STUB_LOG("SGunner::ConsumeAmmo (0x583e30) world event 0x5fff20");
    }
    if (AmmoLeft < 0.0f)
        AmmoLeft = 0.0f;
}

} // namespace pz
