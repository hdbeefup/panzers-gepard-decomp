// src/game/combat_math.h
// The pure arithmetic of SUnit::TakeDamage 0x5c4080 (combat.cpp), split out
// so that a test harness can run it against the HD function under Unicorn
// (scratchpad m3c2/emu_td.py). OWNER: agent M3-C sub-agent C2.
//
// Precision: HD does this in SSE2 scalar single precision except the two
// HP updates (x87 fdivr / fsubr after the ST0 return of +0x174), which run
// under the in-game control word 0x007F (24-bit precision) and so round
// like float operations; float arithmetic here is bit-identical.

#ifndef PZ_GAME_COMBAT_MATH_H
#define PZ_GAME_COMBAT_MATH_H

#include <math.h>
#include "drivermath.h"

namespace pz {

// The unitvariables.ini damage factors (SUnitRegistry +0x0c..+0x38).
struct SDamageFactors {
    float BulletToUnarmoured;   // +0x0c
    float BulletToArmoured;     // +0x10
    float BulletToBuilding;     // +0x14
    float ATToInfantry;         // +0x18
    float ATToBuilding;         // +0x1c
    float HEToInfantry;         // +0x20
    float HEToArmoured;         // +0x24
    float HEToBuilding;         // +0x28
    float FireToUnarmoured;     // +0x2c
    float FireToArmoured;       // +0x30
    float FireToBuilding;       // +0x34
    float ThermoOut;            // +0x38
};

// HD 0x5c4305..0x5c44c3: the weapon / armour type table. weaponType: 0
// bullet, 1 AT, 2 HE, 3 fire; armourType (prototype +0x94): 0 infantry,
// 1 unarmoured, 2 armoured, 3 building. Fire against armour heats the unit
// (+0x118) instead; above the prototype's thermostat (+0x9c) HP drops to 0.
inline float DamageByWeapon(const SDamageFactors& f, float d, int weaponType, int armourType,
                            int classType, int globalState, float thermostat, float* heat,
                            float* hp)
{
    switch (weaponType) {
    case 0:
        if (armourType == 1) {
            if (classType != 0xb)
                d = f.BulletToUnarmoured * d;
        } else if (armourType == 2) {
            if (classType != 0xb)
                d = f.BulletToArmoured;                 // HD assigns (no multiply)
        } else if (armourType == 3) {
            d = f.BulletToBuilding;                     // HD assigns (no multiply)
        }
        break;
    case 1:
        if (armourType == 0)
            d = f.ATToInfantry * d;
        else if (armourType == 3)
            d = f.ATToBuilding * d;
        break;
    case 2:
        if (armourType == 0)
            d = f.HEToInfantry * d;
        else if (armourType == 2)
            d = f.HEToArmoured * d;
        else if (armourType == 3)
            d = f.HEToBuilding * d;
        return d;                                       // 0x5c44c3: no infantry tail
    case 3:
        if (armourType == 1) {
            d = f.FireToUnarmoured * d;
        } else if (armourType == 2) {
            if (classType != 0xb) {
                float h = f.FireToArmoured * d + *heat;
                *heat = h;
                if (h > thermostat)
                    *hp = 0.0f;
                d = 0.0f;
            }
        } else if (armourType == 3) {
            d = f.FireToBuilding * d;
        }
        return d;
    default:
        break;
    }
    // 0x5c44a6: infantry lying down (global state 2) take 0.6.
    if (armourType == 0 && globalState == 2)
        d = d * 0.6f;                                   // 0x7fd6f0
    return d;
}

// HD 0x5c44c8..0x5c46a1: the hit direction, normalised (0 for hitMode 2),
// and the armour side. Returns the side: -1 none (hitMode 2), 0 front, 1
// left, 2 right, 3 back (unit +0x11c.. / prototype +0xa0..), 4 top (hitMode 1).
inline int DamageSide(const float* pos, float dir, float x, float y, float z, int hitMode,
                      float* norm)
{
    norm[0] = norm[1] = norm[2] = 0.0f;
    if (hitMode != 2) {
        float dx = x - pos[0];
        float dy = y - pos[1];
        float dz = z - pos[2];
        float l2 = dx * dx + dy * dy;
        l2 = l2 + dz * dz;
        double inv = 1.0 / sqrt((double)l2);            // 0x78d090, 0x7eed98
        norm[0] = (float)((double)dx * inv);
        norm[1] = (float)((double)dy * inv);
        norm[2] = (float)((double)dz * inv);
    }
    if (hitMode == 0) {
        float dx = x - pos[0];
        float dz = z - pos[2];
        float a = (float)DAtan2((double)dx, (double)dz);                // 0x78d07a, fstp qword
        float diff = (float)DWrapSub((double)dir, (double)a);          // 0x55c1e0
        double ad = fabs((double)diff);
        if ((double)0.785398185f > ad)                  // 0x7f7f50
            return 0;
        if (ad > (double)2.35619450f)                   // 0x7fd700
            return 3;
        if (0.0f > diff)
            return 2;
        return 1;
    }
    if (hitMode == 1)
        return 4;
    return -1;
}

// HD 0x5c4731..0x5c487d: armour absorbs up to armour * protoArmour
// (DamageAbsorb); the rest, truncated to int, costs HP (the caller divides it
// by +0x174 GetHitPoints); the armour wears by the absorbed part / 10 /
// protoArmour (DamageWearArmour).
inline float DamageAbsorb(float armour, float protoArmour)
{
    return armour * protoArmour;
}

inline void DamageWearArmour(float d, int part, float* armour, float protoArmour)
{
    float a = *armour - ((d - (float)part) / 10.0f) / protoArmour;   // 0x7f5a7c
    *armour = a;
    if (0.0f > a)
        *armour = 0.0f;
}

} // namespace pz

#endif // PZ_GAME_COMBAT_MATH_H
