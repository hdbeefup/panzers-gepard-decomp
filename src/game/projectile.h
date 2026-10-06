// src/game/projectile.h
// Projectiles (OWNER: M3-C sub-agent C1): the projectile unit
// SProjectileUnit (vftable 0x7fa5b8, 0x36c bytes), its ballistic driver
// SProjectileDriver (vftable 0x7f4b98, 0xec bytes) with the prototype
// SPProjectileDriver (vftable 0x7fa91c, 0x4c bytes), and its animation
// SProjectileAnimation (vftable 0x7fd994, 0x2c bytes).
//
// SGunner::ServerRefresh 0x584d00 creates a projectile through
// SWorld::CreateUnit 0x5e3170 with the gunner's projectile prototype, sets
// its velocity (0x5a4450), shooter (+0x354) and damage (0x5a4410). Each tick
// the driver 0x55a740 moves it (gravity, homing on +0x344), RefreshMisc
// 0x5a39f0 tests the hits (units 0x562c20, ground / water) and damages the
// area (0x576490); EC_Die marks it wrecked and the dead-unit refresh
// 0x5a39b0 removes it two ticks later.

#ifndef PZ_GAME_PROJECTILE_H
#define PZ_GAME_PROJECTILE_H

#include "unit.h"

namespace pz {

struct SProjectileUnit : SUnit {
    SProjectileUnit(SPProjectileUnit* proto, int worldIndex);    // 0x5a3810
    ~SProjectileUnit() override;                                 // 0x5a38f0
    void InitNew(int player, const float* pos, float dir, int p4, float hp) override;   // 0x5a3950
    void RefreshDead() override;                                     // 0x5a39b0 dead refresh: removal countdown
    void RefreshTargeting() override;                            // 0x5a3990 FillNearUnits(50, 0)
    void RefreshMisc() override;                                 // 0x5a39f0 flight and impact
    void EC_Die() override;                                      // 0x5a3920 +0x150 = 1
    // HD +0x1c 0x5a3930 (*p1 = this, *p2 = the SUnit property table) is not
    // overridden: SIUnit::Slot_1C is untyped (agent C2's slot).

    void SetDamage(float damage, float radius, int weaponType);  // 0x5a4410
    void SetHomingTarget(int unit) { HomingTarget = unit; }      // 0x5a4440
    void SetVelocity(float x, float y, float z);                 // 0x5a4450 (unit +0xbc..+0xc4)
    bool MovedFromStart();                                       // 0x5a38b0 (on +0x35c: != pos)
    void GroundHit(float x, float y, float z);                   // 0x5a4470 ground incidence effects
    void WaterHit(float x, float y, float z);                    // 0x5a47f0 water incidence effects
    void UnitHit(float x, float y, float z, SUnit* hit);         // 0x5a4580 unit / building incidence effects

    float* Velocity() { return (float*)&_bc[0]; }                // unit +0xbc

    SPProjectileUnit* P;            // +0x340
    int      HomingTarget;          // +0x344 unit the driver steers to (-1)
    float    Damage;                // +0x348
    float    DamageRadius;          // +0x34c
    int      WeaponType;            // +0x350
    int      Shooter;               // +0x354 the firing unit's top carrier (-1)
    bool     Cannonade;             // +0x358 shell-fall sound pending ("Projectile cannonade")
    bool     Burning;               // +0x359 fire weapons: damage along the flight
    unsigned char _35a[2];
    float    Start[3];              // +0x35c launch position (fire weapons)
    float    Range;                 // +0x368 fire weapons: the gunner's MaxRange
};

// HD 0x5630c0 (SGameLogic, agent O's row): the first standing building
// (class 9, not wrecked, not the unit's own carrier) whose model the segment
// from -> to crosses; -1 if none. unit / exclude are skipped.
int BuildingBetween(int unit, int exclude, const float* from, const float* to);

// HD 0x5e9720 (SWorld, agent F's row): the bilinear ground height of the
// World +0xe8 vertex grid (0 outside the map). Projectiles test it.
float WorldGroundHeight(float x, float z);

} // namespace pz

#endif // PZ_GAME_PROJECTILE_H
