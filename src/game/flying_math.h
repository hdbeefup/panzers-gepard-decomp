// src/game/flying_math.h
// The pure arithmetic of the planes and the paratroopers, split out so that
// a test harness can run it against the HD functions under Unicorn
// (scratchpad m3c3/emu_fly.py). OWNER: agent M3-C sub-agent C3.
//
// Trig is a policy with static Sin / Cos (double -> double): the game passes
// the HD CRT routines (drivermath.h DSin / DCos), the test the same CRT
// routines its emulator hooks return. Height(x, z) is the terrain height
// (SWorld::GetTerrainHeight 0x5e7730).
//
// Precision: SSE2 scalar as HD; the terrain sums HD does on the x87 stack
// (fadd / fsubr dword, fstp dword) run under the in-game control word 0x007F
// (24-bit precision) and so round like float operations.

#ifndef PZ_GAME_FLYING_MATH_H
#define PZ_GAME_FLYING_MATH_H

#include <math.h>

namespace pz {

// SFlyingUnit::ServerRefresh 0x55d0c0, the tactical bomber's dive
// (0x55d3d3..0x55d5bf). Returns true when the plane leaves (RemoveMe).
struct SFlyingDive {
    float Pitch;          // unit +0x37c degrees
    float HP;             // unit +0x114
    int   Bombs;          // unit +0x348
    float DiveSpeed;      // unit +0x36c
    float Dir;            // unit +0xb0
    float Pos[3];         // unit +0x8c
    float Velocity[3];    // unit +0x370
    float Roll;           // unit +0xd4
    float RollStep;       // unit +0x380
};

template <class Trig, class HeightFn>
inline bool FlyingDiveStep(SFlyingDive& s, HeightFn height)
{
    if (0.0f > s.HP)
        s.Pitch = s.Pitch + 1.0f;                                 // 0x7f1b58
    else if (s.Bombs == 0 && s.Pitch > -55.0f)                    // 0x7f5e40
        s.Pitch = s.Pitch - 4.0f;                                 // 0x7f4588
    float a = s.Pitch * 0.017453292f;                             // 0x7f59a0
    double sn = Trig::Sin((double)a);
    float sp = (float)(sn * 2.0 * 0.012262499891221523 + (double)s.DiveSpeed);   // 0x7ea768, 0x7f59d8
    s.DiveSpeed = sp;
    if (0.4f > sp)                                                // 0x7f5e20
        s.DiveSpeed = 0.4f;
    double c = Trig::Cos((double)a);
    double sd = Trig::Sin((double)s.Dir);
    float vx = (float)(sd * (double)s.DiveSpeed * c);
    s.Velocity[0] = vx;
    float vy = (float)((double)(-s.DiveSpeed) * sn);
    s.Velocity[1] = vy;
    double cd = Trig::Cos((double)s.Dir);
    float x = s.Pos[0] + vx;
    s.Pos[0] = x;
    float vz = (float)(cd * (double)s.DiveSpeed * c);
    s.Velocity[2] = vz;
    s.Pos[1] = s.Pos[1] + vy;
    float z = s.Pos[2] + vz;
    s.Pos[2] = z;
    if (0.0f > s.Pitch) {
        float roll = s.Roll;
        if (0.4000000059604645 > fabs((double)roll))              // 0x7f5e28
            s.Roll = s.RollStep + roll;
    }
    return s.Bombs == 0 && s.Pos[1] > height(x, z) + 20.0f;       // fadd 0x7f35d8
}

// SFlyingDriver::MoveTowardNextWayPoint 0x5586a0 after the base step: the
// climb towards Altitude over the highest terrain 2..14 steps ahead.
// Returns the new ghost height.
template <class Trig, class HeightFn>
inline float FlyingClimbStep(float* climb, float x, float y, float z, float dir, float speed, float altitude,
                             HeightFn height)
{
    float step = speed * 5.0f;                                    // 0x7f5a6c
    float a = (float)((double)(6.2831855f - dir) + 1.5707963705062866);   // 0x7f458c, 0x7f5a38
    float cx = (float)(Trig::Cos((double)a) * (double)step);
    float sz = (float)(Trig::Sin((double)a) * (double)step);
    float top = -10000.0f;                                        // 0x7f5aa8
    for (int i = 2; i <= 0xe; ++i) {
        float fi = (float)i;
        float h = height(fi * cx + x, fi * sz + z) + altitude;    // fadd dword unit +0x384
        if (h > top)
            top = h;
    }
    float d = top - y;
    float ad = (float)fabs((double)d);
    if (0.1f >= ad) {                                             // 0x7f59a8
        *climb = 0.0f;
        return y;
    }
    float sign = 0.0f > d ? -1.0f : (d > 0.0f ? 1.0f : 0.0f);
    float v = *climb;
    float av = (float)fabs((double)v);
    if (av > 0.0005f) {                                           // 0x7f5990
        do {
            av = av - 0.0005f;
            if (0.0f > av)
                av = 0.0f;
            ad = ad - av;
        } while (av > 0.0005f);
    }
    float st = sign * 0.0005f;
    if (0.1f >= ad) {
        v = v - st;
        *climb = v;
        if (-0.005f > v)                                          // 0x7f5a94
            *climb = -0.005f;
    } else {
        v = st + v;
        *climb = v;
        if (v > 1.0f)
            *climb = 1.0f;
    }
    return *climb + y;
}

// SPanzersParachuteDriver::Refresh 0x55a360: one tick of a falling
// parachutist (pos += velocity, then velocity y = -0.1, not below ground).
template <class HeightFn>
inline void ParachuteFallStep(float* pos, float* vel, HeightFn height)
{
    pos[0] = vel[0] + pos[0];
    pos[1] = vel[1] + pos[1];
    pos[2] = vel[2] + pos[2];
    vel[1] = -0.1f;                                               // 0xbdcccccd
    float g = height(pos[0], pos[2]);
    if (g > pos[1])
        pos[1] = height(pos[0], pos[2]);
}

// ... and the walk off the canopy after landing (ticks 5..40).
template <class Trig>
inline void ParachuteWalkStep(float* pos, float dir)
{
    float a = dir - 1.5707964f;                                   // 0x7f5a00
    pos[0] = (float)((Trig::Sin((double)a) * 0.5) / 20.0 + (double)pos[0]);
    float a2 = dir - 1.5707964f;
    pos[2] = (float)((Trig::Cos((double)a2) * 0.5) / 20.0 + (double)pos[2]);
}

// SGameLogic 0x56cff0: the map border point (1 .. size - 1) on the line
// through (x, z) against dir. False = HD's "Can't compute start position".
template <class Trig>
inline bool FlyingBorderStart(float* out, float x, float z, float dir, int terrainW, int terrainH)
{
    out[0] = out[1] = out[2] = 0.0f;
    float s = -(float)Trig::Sin((double)dir);
    float c = -(float)Trig::Cos((double)dir);
    float w1 = (float)(terrainW - 1);
    float h1 = (float)(terrainH - 1);
    if (s > 0.0f) {
        out[0] = w1;
        out[2] = (w1 - x) * (c / s) + z;
        if (out[2] >= 1.0f && h1 >= out[2])
            return true;
    } else if (0.0f > s) {
        out[0] = 1.0f;
        out[2] = (c / s) * (1.0f - x) + z;
        if (out[2] >= 1.0f && h1 >= out[2])
            return true;
    }
    if (c > 0.0f) {
        out[2] = h1;
        out[0] = (h1 - z) * (s / c) + x;
        if (out[0] >= 1.0f && w1 >= out[0])
            return true;
    } else if (0.0f > c) {
        out[2] = 1.0f;
        out[0] = (s / c) * (1.0f - z) + x;
        if (out[0] >= 1.0f && w1 >= out[0])
            return true;
    }
    return false;
}

} // namespace pz

#endif // PZ_GAME_FLYING_MATH_H
