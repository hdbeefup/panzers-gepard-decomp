// src/game/gunnermath.cpp
// Gunner and projectile maths (OWNER: M3-C sub-agent C1). See gunnermath.h.
//
// Every double expression goes through SSE2 (the D* wrappers move the x87
// return values of the CRT routines into an SSE register): the game runs
// with the x87 control word 0x007F, under which MSVC's x87 code would round
// to 24 bits where HD computed in SSE2 (see drivermath.cpp).

#include <math.h>
#include <emmintrin.h>
#include "gunnermath.h"
#include "drivermath.h"

namespace pz {

static const double kPi = 3.1415927410125732;          // 0x7f4560
static const double kMinusPi = -3.1415927410125732;    // 0x7f5aa0
static const double kTwoPi = 6.2831854820251465;       // 0x7f4570
static const double kGravityD = 0.012262499891221523;  // 0x7f59d8 (double)0.0122625f
static const float kGravity = 0.0122625f;              // 0x7f599c
static const float kHalfGravity = 0.00613125f;         // 0x7f5998

static __forceinline double DTan(double x) { return SseD(HdTan(x)); }
static __forceinline double DAtan(double x) { return SseD(HdAtan(x)); }
static __forceinline double DSqrt(double x) { return _mm_cvtsd_f64(_mm_sqrt_sd(_mm_setzero_pd(), _mm_set_sd(x))); }   // 0x78d090

double GunnerWrap(double a)
{
    __m128d s = _mm_set_sd(a);
    if (_mm_comigt_sd(s, _mm_set_sd(kPi)))
        return _mm_cvtsd_f64(_mm_sub_sd(s, _mm_set_sd(kTwoPi)));
    if (_mm_comigt_sd(_mm_set_sd(kMinusPi), s))
        s = _mm_add_sd(s, _mm_set_sd(kTwoPi));
    return _mm_cvtsd_f64(s);
}

// 0x7ea780 -1, 0x7eed98 1: the sign HD computes inline (0 for 0 and NaN).
static double Sign(double d)
{
    if (0.0 > d)
        return -1.0;
    if (d > 0.0)
        return 1.0;
    return 0.0;
}

// PANZERS 0x583070 / 0x583570 (shared body, see gunnermath.h)
bool GunnerTurnAxis(SGunnerAxis* a, float target, float speed, float maxLeft, float maxRight,
                    bool elevation)
{
    float t = target;
    if (!(a->Goal == target) || a->Flip) {
        if (elevation) {
            if (t > 1.5533431f)                                   // 0x7f83e8
                t = 1.5533431f;
            else if (0.0f > t)
                t = 0.0f;
        } else if (maxLeft > -3.1241393f && 3.1241393f > maxRight) {   // 0x7f841c, 0x7f8400
            if (t > maxRight)
                t = maxRight;
            else if (maxLeft > t)
                t = maxLeft;
        }
        bool same = a->Step == 0;
        if (a->Step != 0) {
            double s0 = Sign(GunnerWrap((double)a->Goal - (double)a->Angle));
            double s1 = Sign(GunnerWrap((double)t - (double)a->Angle));
            if (s0 == s1)
                same = true;
            double s2 = Sign(GunnerWrap((double)a->Goal - (double)a->Angle));
            double s3 = Sign(GunnerWrap((double)t - (double)a->Angle));
            if (!(s2 == s3)) {
                a->Flip = true;
                a->Dir = -1;
            }
        }
        if (same) {
            a->Flip = false;
            a->Goal = t;
            a->Dir = 1;
        }
    }
    float goal = a->Goal;
    float stepA = (float)(((double)a->Step / 20.0) * (double)speed);   // 0x7f5a58
    double diff = (double)goal - (double)a->Angle;
    double ad = fabs(diff);
    double dist = ad > kPi ? kTwoPi - ad : ad;
    double unit = (double)speed / 20.0;
    if (!(dist > (double)stepA + unit)) {
        a->Dir = 0;
        a->Step = 0;
        a->Angle = goal;
        return true;
    }
    if (a->Step != 0) {
        float thr = (((float)unit + stepA) * 0.5f) * (float)a->Step - stepA * 0.5f;   // 0x7f453c
        double dist2 = ad > kPi ? kTwoPi - ad : ad;
        if ((double)thr > dist2)
            a->Dir = -1;
    }
    a->Step += a->Dir;
    if (a->Step < 0)
        a->Step = 0;
    if (a->Step > 20)
        a->Step = 20;
    float stepB = (float)(((double)a->Step / 20.0) * (double)speed);
    if (stepB > 0.0f) {                                           // 0x7f1038
        double r = (double)stepB * Sign(GunnerWrap(diff)) + (double)a->Angle;
        a->Angle = (float)GunnerWrap(r);
        return false;
    }
    if (goal == t) {
        a->Dir = 0;
        a->Step = 0;
        a->Angle = goal;
        return true;
    }
    return false;
}

void GunnerBallisticVelocity(float dx, float dy, float dz, float angle, float* out)
{
    double dist = DSqrt((double)(dx * dx + dz * dz));
    float h = (float)(DTan((double)angle) * dist);               // 0x78d810
    float k = (h - dy) * 2.0f;                                   // 0x7f4558
    double c = DCos((double)angle) / dist;                       // 0x78d480
    double v = c;
    if ((double)k > kGravityD / (c * c))
        v = DSqrt((double)(kGravity / k));
    float vf = (float)v;
    out[0] = vf * dx;
    out[1] = vf * h;
    out[2] = vf * dz;
}

void GunnerStraightVelocity(float dx, float dy, float dz, float speed, float* out)
{
    float yaw = DAtan2f((double)dx, (double)dz);                 // 0x78d07a, fstp dword
    double dist = DSqrt((double)(dz * dz + dx * dx));
    float pitch = (float)DAtan((double)dy / dist);               // 0x78d190
    double cp = DCos((double)pitch);
    out[0] = (float)(DSin((double)yaw) * cp * (double)speed);
    out[1] = (float)(DSin((double)pitch) * (double)speed);
    out[2] = (float)(DCos((double)yaw) * cp * (double)speed);
}

// PANZERS 0x55a740 (the maths; projectile.cpp holds the driver wrapper)
void ProjectileDriverStep(float* pos, float* vel, int* timer, bool straight, const float* target)
{
    if (vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2] == 0.0f)   // 0x7f1038
        return;
    pos[0] = vel[0] + pos[0];
    pos[1] = vel[1] + pos[1];
    pos[2] = vel[2] + pos[2];
    if (*timer > 0)
        --*timer;
    if (straight && *timer != 0)
        return;
    if (target) {
        float tx = target[0], ty = target[1], tz = target[2];
        float px = pos[0], py = pos[1], pz = pos[2];
        double h = DSqrt((double)(vel[0] * vel[0] + vel[2] * vel[2]));
        float pitch = (float)DAtan((double)vel[1] / h);
        float dz = tz - pz;
        float dx = tx - px;
        GunnerBallisticVelocity(dx, ty - py, dz, pitch, vel);
    }
    pos[1] = pos[1] - kHalfGravity;
    vel[1] = vel[1] - kGravity;
}

double GunnerAimError(float ux, float uz, float tx, float tz, float unitDir, bool hasParent,
                      float parentTurret, float fireStartArc, float turretAngle)
{
    float ang = DAtan2f((double)(tx - ux), (double)(tz - uz));
    float base = unitDir;
    if (hasParent)
        base = (float)DWrapAdd((double)base, (double)parentTurret);   // 0x550600, fstp dword
    double t = DWrapAdd((double)base, (double)fireStartArc);
    double t2 = DWrapAdd((double)turretAngle, t);
    return DAngleDist((double)ang, t2);                          // 0x551bc0
}

} // namespace pz
