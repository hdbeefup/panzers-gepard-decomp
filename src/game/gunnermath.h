// src/game/gunnermath.h
// The pure maths of the gunners and the projectiles (OWNER: M3-C sub-agent
// C1). Kept free of the unit classes so that a small test exe can compile
// it with hdmath.cpp and compare it, bit for bit, with the HD code run in
// the Unicorn harness (scratchpad sh/emu.py).

#ifndef PZ_GAME_GUNNERMATH_H
#define PZ_GAME_GUNNERMATH_H

namespace pz {

// One turning axis of a gunner (0x14 bytes): the turret at SGunner +0x30,
// the barrel elevation at +0x48.
struct SGunnerAxis {
    float Angle;            // +0x00 current angle
    int   Step;             // +0x04 0..20 (acceleration)
    int   Dir;              // +0x08 +1 accelerate, -1 brake, 0 idle
    bool  Flip;             // +0x0c the goal changed side while turning
    unsigned char _0d[3];
    float Goal;             // +0x10
};

// HD wraps once into (-pi, pi] on doubles with the float-precision pi
// (0x7f4560, 0x7f5aa0, 0x7f4570): if (a > pi) a -= 2pi; else if (-pi > a) a += 2pi.
double GunnerWrap(double a);

// SGunner::TurnTurret 0x583070 (elevation = false) and SGunner::TurnElevation
// 0x583570 (elevation = true) on one axis: clamps the target (turret: to the
// prototype's left / right limits when both lie inside +-3.1241393; barrel:
// to 0..1.5533431), accelerates over 20 steps of speed / 20 per tick and
// returns true once the axis reached its goal.
bool GunnerTurnAxis(SGunnerAxis* a, float target, float speed, float maxLeft, float maxRight,
                    bool elevation);

// The ballistic launch solver, inline twice in HD: SGunner::ServerRefresh
// 0x584d00 at 0x5860fd (angle = the gunner's ShotAngle + 0) and
// SProjectileDriver::Refresh 0x55a740 at 0x55a866 (angle = the current
// pitch of the velocity). (dx, dy, dz) = target - start. out = velocity per
// tick (gravity 0.0122625 per tick^2).
void GunnerBallisticVelocity(float dx, float dy, float dz, float angle, float* out);

// The straight launch of 0x584d00 at 0x585f0a (rockets, projectile driver
// prototype +0x44 set): yaw / pitch towards the target, times the prototype
// speed. dy already holds the 0.8 aim offset of unit targets.
void GunnerStraightVelocity(float dx, float dy, float dz, float speed, float* out);

// SProjectileDriver::Refresh 0x55a740 on the projectile fields: pos =
// unit +0x8c, vel = unit +0xbc, timer = driver +0xe8, straight = the driver
// prototype +0x44, target = the homing target's position (nullptr when the
// projectile's +0x344 is not a live unit).
void ProjectileDriverStep(float* pos, float* vel, int* timer, bool straight, const float* target);

// The aim test of 0x584d00 at 0x5857f5: the angle between the unit -> target
// direction and the gun (unit dir + parent turret + fire start arc + turret
// angle), as HdAngleDist returns it. parentTurret is used when hasParent.
double GunnerAimError(float ux, float uz, float tx, float tz, float unitDir, bool hasParent,
                      float parentTurret, float fireStartArc, float turretAngle);

} // namespace pz

#endif // PZ_GAME_GUNNERMATH_H
