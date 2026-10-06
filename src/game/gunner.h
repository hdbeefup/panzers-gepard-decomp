// src/game/gunner.h
// SGunner (vftable 0x7f8180, 13 slots, 0x78 bytes) and its prototype SPGunner
// (vftable 0x7f8168, 5 slots, 0xcc bytes): one weapon of a unit
// (0x582a00..0x587d00). OWNER: agent U.
//
// The unit keeps its gunners at +0x48 (SDArray<SGunner*>, created by the SUnit
// ctor 0x5b2820 through SPGunner +0x10). SGunner::ServerRefresh 0x584d00 is
// lifted up to the point where a unit without a target returns (everything
// the menu executes); the targeting, turret and firing part is a logged stub.

#ifndef PZ_GAME_GUNNER_H
#define PZ_GAME_GUNNER_H

#include "punit.h"

namespace pz {

struct SUnit;
struct SGunner;
struct SUPropStruct;
struct SIUnitAnimation;
struct STarget;

// HD SPGunner (0xcc bytes), ctor 0x582ba0, Load 0x5842e0.
struct SPGunner {
    SPGunner();                                                  // 0x582ba0
    virtual ~SPGunner();                                         // +0x00 HD 0x582ef0
    virtual void Load(SUPropStruct* gunner);                     // +0x04 HD 0x5842e0
    virtual void LoadResources(SUPropStruct* gunner);            // +0x08 HD 0x584a30
    virtual void ReleaseResources();                             // +0x0c HD 0x587c00
    virtual SGunner* CreateGunner(SUnit* unit, SIUnitAnimation* anim, int index);   // +0x10 HD 0x583df0

    // +0x00 vptr
    bool     HandToHand;        // +0x04 "HandToHand / Ranged" index 0
    unsigned char _05[3];
    int      _08;               // +0x08
    int      ProjectileCount;   // +0x0c
    SUnitArray<SString> Projectiles; // +0x10 projectile unit names (8 bytes each)
    int      GunnerType;        // +0x1c 0 ground, 1 air defense
    float    GunnerHeight;      // +0x20 * 0.5
    int      WeaponType;        // +0x24 0 bullet, 1 AT, 2 HE, 3 fire
    float    FireStartArc;      // +0x28 radians
    float    FireArc;           // +0x2c radians
    int      ParentGunner;      // +0x30
    float    MinRange;          // +0x34 * 0.5
    float    MaxRange;          // +0x38 * 0.5
    float    TurretSpeed;       // +0x3c radians per tick
    float    MaxLeftAngle;      // +0x40 radians (negative)
    float    MaxRightAngle;     // +0x44 radians
    int      Ammo;              // +0x48
    bool     DirectDamage;      // +0x4c
    unsigned char _4d[3];
    int      ReloadTime;        // +0x50 ticks
    int      ReshotTime;        // +0x54 ticks
    float    Damage;            // +0x58
    float    DamageRadius;      // +0x5c
    float    KickSize;          // +0x60
    int      KickTime;          // +0x64 ticks
    int      ShotDelay;         // +0x68 ticks
    int      Fixed;             // +0x6c "Fixed / Rotatable" index 0
    float    ShotAngle;         // +0x70 radians
    float    ShotSpread;        // +0x74
    bool     AutoShot;          // +0x78
    unsigned char _79[3];
    int      BurstShot;         // +0x7c
    float    BodyKick;          // +0x80
    SUnitArray<SPUnitEffect> ShotEffects;            // +0x84
    SUnitArray<SPUnitEffect> GroundIncidenceEffects; // +0x90
    SUnitArray<SPUnitEffect> WaterIncidenceEffects;  // +0x9c
    SUnitArray<SPUnitEffect> UnitIncidenceEffects;   // +0xa8
    SUnitArray<SPUnitEffect> WoodIncidenceEffects;   // +0xb4
    SUnitArray<SPUnitEffect> StoneIncidenceEffects;  // +0xc0
};

// HD SGunner (0x78 bytes), ctor 0x582ad0.
struct SGunner {
    SGunner(SPGunner* proto, SUnit* unit, SIUnitAnimation* anim, int index);   // 0x582ad0
    virtual ~SGunner();                                          // +0x00 HD 0x582e80
    virtual bool TurnTurret(float angle);                        // +0x04 HD 0x583070 (name guessed) turret angle +0x30 towards +0x40
    virtual void Slot_08(float p1);                              // +0x08 HD 0x583550 -> +0x0c
    virtual bool TurnElevation(float angle);                     // +0x0c HD 0x583570 (name guessed) +0x48 towards +0x58
    virtual void Slot_10(float p1);                              // +0x10 HD 0x582f70 (world angle -> +0x04)
    virtual unsigned ServerRefresh();                            // +0x14 HD 0x584d00
    virtual void SetTarget(STarget* t);                          // +0x18 HD 0x583950
    virtual void SetTargetKeepTurret(STarget* t);                // +0x1c HD 0x583460 (name guessed)
    virtual void AimAt(float angle);                             // +0x20 HD 0x5834a0 (name guessed)
    virtual void ResetToParent();                                // +0x24 HD 0x584cd0 (name guessed)
    virtual void Stop();                                         // +0x28 HD 0x587bd0 (unit +0xec StopGunners)
    virtual SPGunner* GetPGunner();                              // +0x2c HD 0x584210
    virtual void Slot_30(void** p1, void** p2);                  // +0x30 HD 0x584220

    void ConsumeAmmo();                                          // 0x583e30

    // +0x00 vptr
    int        Index;           // +0x04 gunner index in the unit
    SPGunner*  Proto;           // +0x08
    SUnit*     Unit;            // +0x0c
    SIUnitAnimation* Anim;      // +0x10
    STarget*   Target;          // +0x14 refcounted
    STarget*   PendingTarget;   // +0x18
    int        Active;          // +0x1c 1 (0 for armoured units, SSingleUnit::Init)
    float      AmmoLeft;        // +0x20 1.0
    int        BurstLeft;       // +0x24 prototype +0x7c
    bool       Idle;            // +0x28
    unsigned char _29[3];
    float      RestAngle;       // +0x2c
    float      TurretAngle;     // +0x30
    int        TurretStep;      // +0x34 0..20
    int        TurretDir;       // +0x38
    bool       TurretFlip;      // +0x3c
    unsigned char _3d[3];
    float      TurretGoal;      // +0x40
    float      AimAngle;        // +0x44
    float      Elevation;       // +0x48
    int        ElevStep;        // +0x4c
    int        ElevDir;         // +0x50
    bool       ElevFlip;        // +0x54
    unsigned char _55[3];
    float      ElevGoal;        // +0x58
    bool       Loaded;          // +0x5c (name guessed)
    unsigned char _5d[3];
    int        _60;             // +0x60
    int        ReloadLeft;      // +0x64 ticks
    int        _68;             // +0x68
    int        _6c;             // +0x6c
    int        KickLeft;        // +0x70 ticks
    unsigned   Kick;            // +0x74 float bits (sign flipped)
};

PZ_HD_SIZE(SPGunner, 0xcc);
PZ_HD_SIZE(SGunner, 0x78);

} // namespace pz

#endif // PZ_GAME_GUNNER_H
