// src/game/flying.h
// The air support classes: SFlyingUnit (planes, ClassType 8, vftable
// 0x7f5b74, 0x388 bytes), its driver SFlyingDriver (vftable 0x7f4ae8, 0xf4)
// with the prototype SPFlyingDriver (vftable 0x7fa8ec, 0x58), the plane
// animation SFlyingAnimation (vftable 0x7fda24, 0xb0) and the paratroopers'
// SPanzersParachuteDriver (vftable 0x7f4da8, 0xe8) / SPPanzersParachuteDriver
// (vftable 0x7fa97c, 0x44). OWNER: agent M3-C sub-agent C3.
//
// The support calls (combat_support.cpp) create the planes through
// SWorld::CreateUnit:
//   PlaneType (SPFlyingUnit +0x13c) 0 glider, 1 recon plane, 2 tactical
//   bomber (dives on the target, drops the bombs hung on the "suspension%d"
//   nodes), 3 heavy bomber / transport (drops bombs or paratroop squads near
//   the target, +0x348 / +0x34c).
// Bombs are projectile units (SProjectileUnit, C1): the plane writes their
// velocity (+0xbc, HD 0x5a4450), damage / radius / weapon type (+0x348,
// HD 0x5a4410) and owner (+0x354).

#ifndef PZ_GAME_FLYING_H
#define PZ_GAME_FLYING_H

#include "unit.h"

namespace pz {

// ---------------------------------------------------------------------------
// SFlyingUnit

struct SFlyingUnit : SUnit {
    SFlyingUnit(SPFlyingUnit* proto, int worldIndex);            // 0x55c9d0 (draws the world seed once)
    ~SFlyingUnit() override;                                     // 0x55cd10
    void GetClassDescriptor(void** obj, const SUnitClassDesc** desc) override;   // 0x55cec0 (M4 S, savedesc.cpp)
    void Init(SUnitDef* def) override;                           // 0x55cef0
    void InitNew(int player, const float* pos, float dir, int p4, float hp) override;   // 0x55cf20
    void RefreshDead() override;                                     // 0x55cfc0 the wreck falls / explodes
    void RefreshTargeting() override;                            // 0x55cf90
    void RefreshMisc() override;                                 // 0x55d0c0 (SFlyingUnit::ServerRefresh)
    void OnDriverReachedTarget() override;                       // 0x55cf80 RemoveMe
    void EC_Move(int xBits, int zBits, int p3, bool p4, int p5) override;   // 0x55ce00
    // +0x1c 0x55cec0 is the class descriptor getter (Slot_1C, untyped): not overridden.

    void SetupTransport();                                       // 0x55e370 (paratroopers: 2 squads, +7 altitude)

    SPFlyingUnit* P;                 // +0x340
    int      Timer;                  // +0x344 tactical bomber: ticks before the dive (30, invulnerable);
                                     //        others: ticks to the next drop (2..5)
    int      Bombs;                  // +0x348 bombs left (tactical 1, heavy 5)
    int      Squads;                 // +0x34c paratroop squads left (transport 2)
    int      _350;                   // +0x350 -1
    float    Target[3];              // +0x354 the support call's target (y 6.0)
    int      _360[3];                // +0x360
    float    DiveSpeed;              // +0x36c tactical bomber speed (0.4 .. )
    float    Velocity[3];            // +0x370 tactical bomber velocity of the tick (bombs inherit it)
    float    Pitch;                  // +0x37c tactical bomber dive angle in degrees (45 -> -55)
    float    RollStep;               // +0x380 +-0.02 (world seed), added to +0xd4 while climbing
    float    Altitude;               // +0x384 SPFlyingDriver +0x4c (+ the support call's extra)
};

PZ_HD_SIZE(SFlyingUnit, kHdSizeSFlyingUnit);

// SProjectileUnit setters (C1's class) the planes and the artillery use on
// the shells they create.
void FlyingSetProjectileVelocity(SUnit* p, float x, float y, float z);   // 0x5a4450 (+0xbc)
void FlyingSetProjectileDamage(SUnit* p, float damage, float radius, int weaponType);   // 0x5a4410 (+0x348)

// Parachute drop of a paratroop squad (parachute.cpp).
void ParachuteSquadDrop(SUnit* squad, float y);         // 0x5a1140 SPanzersSquadUnit (C5's class)

// The plane driver's ground tracking flag (SFlyingDriver +0xe8, 0x55af10).
void FlyingDriverSetGroundTracking(SIDriver* driver, bool on);
} // namespace pz

#endif // PZ_GAME_FLYING_H
