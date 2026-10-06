// src/game/manoeuvre.cpp
// SWayPointWithManoeuvres (HD 0x58cbb0..0x590e00). OWNER: P.
//
// Every arithmetic expression follows the HD disassembly operation by
// operation: SSE single-precision float ops where HD uses mulss/addss,
// double where HD widens (cvtps2pd ... cvtpd2ps), the HD float-pi-as-double
// constants, and the HD CRT helpers (drivermath.h). Do not "simplify" an
// expression: the evaluation order and the precision of each step are part
// of the menu's determinism (docs/M2_INTERFACES.md §7).

#include <math.h>
#include <string.h>
#include <emmintrin.h>
#include "manoeuvre.h"
#include "drivermath.h"
#include "target.h"

namespace pz {

namespace {

// HD constants (exact bits).
const double kHalfPi = 1.5707963705062866;   // 0x7f5a38 (float pi/2 as double)
const double kEps = 0.0001;                  // 0x7f1b50
const double kTanScale = 0.8;                // 0x7f8f10
const float  kTwoThirds = 0.6666666865348816f; // 0x7f8f08 (0x3f2aaaab)
const float  kRadiusScale = 1.7000000476837158f; // 0x7f8f0c (0x3fd9999a)
const float  kPiThird = 1.0471975803375244f;   // 0x3f860a92 (pushed by CreateManoeuvres)
const float  kPiHalfF = 1.5707963705062866f;   // 0x3fc90fdb

// The drivermath helpers return their double in ST0 (x86 cdecl). MSVC then
// tends to continue the expression on the x87 stack (fmul/fadd/fdivr at the
// current x87 precision: 53 bits by default, 24 bits under the game's CW
// 0x007F), which is not what HD does (SSE2). Moving every result into an
// SSE register first keeps the arithmetic on SSE: Sin/Cos/Tan/Atan2 below.
__forceinline double ToSse(double v)
{
    return _mm_cvtsd_f64(_mm_set_sd(v));
}
__forceinline double Sin(double x) { return ToSse(HdSin(x)); }
__forceinline double Cos(double x) { return ToSse(HdCos(x)); }
__forceinline double Tan(double x) { return ToSse(HdTan(x)); }
__forceinline double Atan2(double y, double x) { return ToSse(HdAtan2(y, x)); }
// For the HD call sites that store the result with fstp dword.
__forceinline float Atan2f(double y, double x)
{
    return _mm_cvtss_f32(_mm_set_ss(HdAtan2f(y, x)));
}

// The HD wrap idiom: x > pi -> x - 2pi; else -pi > x -> x + 2pi.
__forceinline double Wrap(double x)
{
    if (x > kHdPi)
        return x - kHdTwoPi;
    if (kHdMinusPi > x)
        return x + kHdTwoPi;
    return x;
}

// Sign as HD computes it: 0 > x -> -1; x > 0 -> 1; else 0 (NaN -> 0).
__forceinline double SignD(double x)
{
    if (0.0 > x)
        return -1.0;
    if (x > 0.0)
        return 1.0;
    return 0.0;
}
__forceinline float SignF(float x)
{
    if (0.0f > x)
        return -1.0f;
    if (x > 0.0f)
        return 1.0f;
    return 0.0f;
}

typedef SWayPointWithManoeuvres SWpm;

// PANZERS 0x58cbb0 (SDArray<SManoeuvre>::Add) + the name/type setup every
// manoeuvre creator starts with (FUN_0052c320(name) at +0x38).
int AddManoeuvre(SWpm* w, int type, const char* name)
{
    int i = w->Manoeuvres.Add();
    if (i < 0 || i >= w->Manoeuvres.Size) {
        DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SManoeuvre", i);
        return -1;
    }
    SManoeuvre* m = &w->Manoeuvres.Array[i];
    m->Name = name;
    m->Type = type;
    return i;
}

// PANZERS 0x58cc20 (SDArray<SWayPoint>::Add)
inline int AddPoint(SManoeuvre* m)
{
    return m->Points.Add();
}

inline SManoeuvre* Man(SWpm* w, int i)
{
    return &w->Manoeuvres.Array[i];
}

inline SWayPoint* Pt(SManoeuvre* m, int i)
{
    return &m->Points.Array[i];
}

// Frees one manoeuvre's members (HD: delete +0x38, free +0x08, zero +0x0c/+0x10).
void FreeManoeuvre(SManoeuvre* m)
{
    m->Name = nullptr;
    if (m->Points.Array) {
        ::free(m->Points.Array);
        m->Points.Array = nullptr;
    }
    m->Points.Size = 0;
    m->Points.Max = 0;
}

// PANZERS 0x58dc30 SDArray<SWayPoint>::Insert(i): zeroed new element.
int InsertPoint(SManoeuvre* m, int i)
{
    SHdDArray<SWayPoint>& a = m->Points;
    if (i < 0 || i > a.Size) {
        DrvPanic("SDArray<%s>::Insert: invalid index (%d)", "SWayPoint", i);
        return -1;
    }
    if (a.Size == a.Max) {
        int n = a.Max < 0x10 ? 0x10 : (a.Max * 6) / 5;
        a.Array = (SWayPoint*)::realloc(a.Array, (size_t)n * sizeof(SWayPoint));
        memset(a.Array + a.Max + 1, 0, (size_t)(n - a.Max) * sizeof(SWayPoint) - sizeof(SWayPoint));
        a.Max = n;
    }
    if (i < a.Size)
        memmove(a.Array + i + 1, a.Array + i, (size_t)(a.Size - i) * sizeof(SWayPoint));
    memset(&a.Array[i], 0, sizeof(SWayPoint));
    ++a.Size;
    return i;
}

// PANZERS 0x58df80 SDArray<SManoeuvre>::Remove(i)
void RemoveManoeuvre(SWpm* w, int i)
{
    SHdDArray<SManoeuvre>& a = w->Manoeuvres;
    if (i < 0 || i >= a.Size) {
        DrvPanic("SDArray<%s>::Remove: invalid index (%d) size = %d", "SManoeuvre", i, a.Size);
        return;
    }
    FreeManoeuvre(&a.Array[i]);
    --a.Size;
    if (a.Size - i != 0)
        memmove(&a.Array[i], &a.Array[i + 1], (size_t)(a.Size - i) * sizeof(SManoeuvre));
    memset(&a.Array[a.Size], 0, sizeof(SManoeuvre));
}

// PANZERS 0x58d3d0 (qsort comparator): a.Dist2 <= b.Dist2 (or unordered) -> 1.
int CompareManoeuvres(const SManoeuvre* a, const SManoeuvre* b)
{
    return (a->Dist2 > b->Dist2) ? -1 : 1;
}

// The HD CRT qsort (0x78ccd0, the VC CRT qsort.c with the "mid" pointer
// tracking and the max-selection shortsort 0x78cbf0 for <= 8 elements), so
// the element order for equal keys is HD's.
void SwapManoeuvres(SManoeuvre* a, SManoeuvre* b)
{
    if (a != b) {
        SManoeuvre t;
        memcpy(&t, a, sizeof(t));
        memcpy(a, b, sizeof(t));
        memcpy(b, &t, sizeof(t));
    }
}

void ShortSort(SManoeuvre* lo, SManoeuvre* hi)
{
    while (hi > lo) {
        SManoeuvre* max = lo;
        for (SManoeuvre* p = lo + 1; p <= hi; ++p)
            if (CompareManoeuvres(p, max) > 0)
                max = p;
        SwapManoeuvres(max, hi);
        --hi;
    }
}

void HdQsort(SManoeuvre* base, int num)
{
    SManoeuvre* lostk[30];
    SManoeuvre* histk[30];
    int stkptr = 0;
    if (num < 2)
        return;
    SManoeuvre* lo = base;
    SManoeuvre* hi = base + (num - 1);
    for (;;) {
        int size = (int)(hi - lo) + 1;
        if (size <= 8) {
            ShortSort(lo, hi);
        } else {
            SManoeuvre* mid = lo + size / 2;
            if (CompareManoeuvres(lo, mid) > 0)
                SwapManoeuvres(lo, mid);
            if (CompareManoeuvres(lo, hi) > 0)
                SwapManoeuvres(lo, hi);
            if (CompareManoeuvres(mid, hi) > 0)
                SwapManoeuvres(mid, hi);
            SManoeuvre* loguy = lo;
            SManoeuvre* higuy = hi;
            for (;;) {
                if (mid > loguy) {
                    do {
                        ++loguy;
                    } while (loguy < mid && CompareManoeuvres(loguy, mid) <= 0);
                }
                if (mid <= loguy) {
                    do {
                        ++loguy;
                    } while (loguy <= hi && CompareManoeuvres(loguy, mid) <= 0);
                }
                do {
                    --higuy;
                } while (higuy > mid && CompareManoeuvres(higuy, mid) > 0);
                if (higuy < loguy)
                    break;
                SwapManoeuvres(loguy, higuy);
                if (mid == higuy)
                    mid = loguy;
            }
            ++higuy;
            if (mid < higuy) {
                do {
                    --higuy;
                } while (higuy > mid && CompareManoeuvres(higuy, mid) == 0);
            }
            if (mid >= higuy) {
                do {
                    --higuy;
                } while (higuy > lo && CompareManoeuvres(higuy, mid) == 0);
            }
            if (higuy - lo >= hi - loguy) {
                if (lo < higuy) {
                    lostk[stkptr] = lo;
                    histk[stkptr] = higuy;
                    ++stkptr;
                }
                if (loguy < hi) {
                    lo = loguy;
                    continue;
                }
            } else {
                if (loguy < hi) {
                    lostk[stkptr] = loguy;
                    histk[stkptr] = hi;
                    ++stkptr;
                }
                if (lo < higuy) {
                    hi = higuy;
                    continue;
                }
            }
        }
        --stkptr;
        if (stkptr < 0)
            return;
        lo = lostk[stkptr];
        hi = histk[stkptr];
    }
}

// PANZERS 0x58ce90: the turning circle of radius +0x14 through the waypoint
// for the turn from dir a to dir b. p5 = the tangent (in) point, p6 = the
// turn point; p8/p9 receive the circle centre and the out tangent point.
void TurnCircle(SWpm* w, float a, float b, bool p4, SWayPoint* p5, SWayPoint* p6, bool p7,
                float* p8, float* p9)
{
    double d = Wrap((double)b - (double)a);
    int side = (int)SignD(d);                     // cvttsd2si
    if (p4)
        side = -side;
    float sb = (float)Sin((double)b);
    float cb = (float)Cos((double)b);
    float l3c, l38;
    if (side >= 0) {
        l3c = -cb;
        l38 = sb;
    } else {
        l38 = -sb;
        l3c = cb;
    }
    double hp = (double)side * kHalfPi;
    double ang = Wrap((double)a + hp);
    float fa = (float)ang;
    float cx = (float)(Sin((double)fa) * (double)w->Radius + (double)w->X);
    float cz = (float)(Cos((double)fa) * (double)w->Radius + (double)w->Z);
    p8[0] = cx;
    p8[1] = cz;
    float t0 = cx - w->X;
    float proj = cz - w->Z;
    float r2 = w->Radius * 2.0f;
    t0 = t0 * l3c;
    proj = proj * l38;
    r2 = r2 * r2;
    proj = proj + t0;
    float q = w->Radius - proj;
    float qq = q * q;
    r2 = r2 - qq;
    float hf = (float)(sqrt(fabs((double)r2)) * 0.5);
    float half = q * 0.5f;
    float hz = l38 * half;
    float hx = l3c * half;
    float px = hf * sb;
    float pz = hf * cb;
    px = px + cx;
    pz = pz + cz;
    px = px + hx;
    pz = pz + hz;
    p5->X = px;
    p5->Z = pz;
    if (!p4 && !p7) {
        p5->Type = kWpNear;
    } else {
        p5->Type = kWpPointDir;
        float dx = p5->X - cx;
        float dz = p5->Z - cz;
        double at = Atan2((double)dx, (double)dz);        // fstp qword, then cvtpd2ps
        double r = (double)(float)at - hp;
        p5->Dir = (float)Wrap(r);
    }
    float ox = sb * 2.0f;
    float oz = cb * 2.0f;
    float q2 = w->Radius - proj;
    ox = ox * hf;
    oz = oz * hf;
    ox = ox + cx;
    oz = oz + cz;
    float ax = l3c * q2;
    float az = l38 * q2;
    p9[0] = ox + ax;
    p9[1] = oz + az;
    float np = -proj;
    float bx = l3c * np;
    float bz = l38 * np;
    p6->Type = kWpPointDir;
    p6->X = ox + bx;
    p6->Z = oz + bz;
    if (p7)
        p6->Dir = (float)Wrap((double)b + kHdPi);
    else
        p6->Dir = b;
}

// PANZERS 0x590690 TurnInPlace_Out___Turn_To_Dir_Of_Next_Waypoint
void TurnInPlaceOut(SWpm* w)
{
    int mi = AddManoeuvre(w, 1, "TurnInPlace_Out___Turn_To_Dir_Of_Next_Waypoint");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    int i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPoint;
    Pt(m, i)->X = w->X;
    Pt(m, i)->Z = w->Z;
    i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpTurn;
    Pt(m, i)->Dir = w->OutDir;
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = w->X;
    m->EndZ = w->Z;
}

// PANZERS 0x590430 / 0x58dd20 TurnInPlace_In___Arrive_At_Waypoint_And_If_Necessary_Turn_To_Final_Direction
// (0x58dd20 differs only in the first point's type when it has to turn).
void TurnInPlaceIn(SWpm* w, int firstTurnType)
{
    int mi = AddManoeuvre(w, 3,
        "TurnInPlace_In___Arrive_At_Waypoint_And_If_Necessary_Turn_To_Final_Direction");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    if (w->InDir != w->OutDir) {
        int i = AddPoint(m);
        Pt(m, i)->Reverse = w->Reverse;
        Pt(m, i)->Type = firstTurnType;
        Pt(m, i)->X = w->X;
        Pt(m, i)->Z = w->Z;
        Pt(m, i)->Dir = w->OutDir;
        i = AddPoint(m);
        Pt(m, i)->Reverse = w->Reverse;
        Pt(m, i)->Type = kWpTurn;
        Pt(m, i)->Dir = w->OutDir;
    } else {
        int i = AddPoint(m);
        Pt(m, i)->Reverse = w->Reverse;
        Pt(m, i)->Type = kWpPointDir;
        Pt(m, i)->X = w->X;
        Pt(m, i)->Z = w->Z;
        Pt(m, i)->Dir = w->OutDir;
    }
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = w->X;
    m->EndZ = w->Z;
}

// PANZERS 0x590800 TurnInPlace_Out___Turn_To_X_Degree_Difference_To_Dir_Of_Next_Waypoint
void TurnInPlaceOutAngle(SWpm* w, float maxAngle)
{
    float f = (float)Wrap((double)w->DirToWp - (double)w->OutDir);
    float nx = w->NextX - w->X;
    float nz = w->NextZ - w->Z;
    nz = nz * nz;
    nx = nx * nx;
    float len = (float)sqrt((double)(nx + nz));
    if (!(len > 4.0f)) {
        float half = maxAngle * 0.5f;
        if (!((double)half >= fabs((double)f)))
            return;
    }
    int mi = AddManoeuvre(w, 2,
        "TurnInPlace_Out___Turn_To_X_Degree_Difference_To_Dir_Of_Next_Waypoint");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    int i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPoint;
    Pt(m, i)->X = w->X;
    Pt(m, i)->Z = w->Z;
    if (fabs((double)f) > (double)maxAngle) {
        i = AddPoint(m);
        Pt(m, i)->Reverse = w->Reverse;
        Pt(m, i)->Type = kWpTurn;
        int s = (int)SignF(f);                    // cvttss2si
        float turn = (float)s * maxAngle;
        Pt(m, i)->Dir = (float)Wrap((double)turn + (double)w->OutDir);
    } else {
        i = AddPoint(m);
        Pt(m, i)->Reverse = w->Reverse;
        Pt(m, i)->Type = kWpPoint;
        Pt(m, i)->X = w->X;
        Pt(m, i)->Z = w->Z;
    }
    float sn = (float)Sin((double)w->OutDir);
    float cs = (float)Cos((double)w->OutDir);
    i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPointDir;
    float ax = w->NextX - w->X;
    float az = w->NextZ - w->Z;
    ax = ax * ax;
    az = az * az;
    float k = (float)sqrt((double)(ax + az));
    k = k * kTwoThirds;
    float L = 5.0f;
    if (5.0f > k) {
        float bx = w->NextX - w->X;
        float bz = w->NextZ - w->Z;
        bx = bx * bx;
        bz = bz * bz;
        L = (float)sqrt((double)(bx + bz));
        L = L * kTwoThirds;
    }
    float cxv = sn * L;
    float czv = cs * L;
    cxv = cxv + w->X;
    czv = w->Z + czv;
    Pt(m, i)->X = cxv;
    Pt(m, i)->Z = czv;
    Pt(m, i)->Dir = w->OutDir;
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = Pt(m, i)->X;
    m->EndZ = Pt(m, i)->Z;
}

// PANZERS 0x58f100 TurnInAngle_In___Arrive_At_Waypoint
void TurnInAngleIn(SWpm* w)
{
    int mi = AddManoeuvre(w, 7, "TurnInAngle_In___Arrive_At_Waypoint");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    int i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPointDir;
    Pt(m, i)->X = w->X;
    Pt(m, i)->Z = w->Z;
    Pt(m, i)->Dir = w->OutDir;
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = w->X;
    m->EndZ = w->Z;
}

// PANZERS 0x58f200 TurnInAngle_In___Arrive_At_Waypoint_With_Turning_Radius
// PANZERS 0x58f4c0 ..._With_Y_Turn (yTurn: reversed circle side and the
// final point reverses to OutDir + pi)
void TurnInAngleInRadius(SWpm* w, bool yTurn)
{
    if (w->InDir == w->OutDir) {
        TurnInAngleIn(w);
        return;
    }
    int mi = yTurn
        ? AddManoeuvre(w, 9, "TurnInAngle_In___Arrive_At_Waypoint_With_Turning_Radius_With_Y_Turn")
        : AddManoeuvre(w, 8, "TurnInAngle_In___Arrive_At_Waypoint_With_Turning_Radius");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    float fa = (float)Wrap((double)w->OutDir + kHdPi);
    double b = Wrap((double)w->DirToWp + kHdPi);
    int i0 = AddPoint(m);
    Pt(m, i0)->Reverse = w->Reverse;
    int i1 = AddPoint(m);
    Pt(m, i1)->Reverse = w->Reverse;
    TurnCircle(w, fa, (float)b, yTurn, Pt(m, i1), Pt(m, i0), true, m->Circle0, m->Circle1);
    int i2 = AddPoint(m);
    if (!yTurn)
        Pt(m, i2)->Reverse = w->Reverse;
    Pt(m, i2)->Type = kWpPointDir;
    Pt(m, i2)->X = w->X;
    Pt(m, i2)->Z = w->Z;
    if (yTurn) {
        Pt(m, i2)->Dir = fa;
        Pt(m, i2)->Reverse = !w->Reverse;
    } else {
        Pt(m, i2)->Dir = w->OutDir;
    }
    m->StartX = Pt(m, i0)->X;
    m->StartZ = Pt(m, i0)->Z;
    m->EndX = w->X;
    m->EndZ = w->Z;
}

// PANZERS 0x58f780 TurnInAngle_Out___Departure_With_Turning_Radius
void TurnInAngleOutRadius(SWpm* w)
{
    int mi = AddManoeuvre(w, 5, "TurnInAngle_Out___Departure_With_Turning_Radius");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    int i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPoint;
    Pt(m, i)->X = w->X;
    Pt(m, i)->Z = w->Z;
    float a = w->DirToWp;
    float b = w->OutDir;
    int i1 = AddPoint(m);
    Pt(m, i1)->Reverse = w->Reverse;
    int i2 = AddPoint(m);
    Pt(m, i2)->Reverse = w->Reverse;
    TurnCircle(w, a, b, false, Pt(m, i1), Pt(m, i2), false, m->Circle0, m->Circle1);
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = Pt(m, i2)->X;
    m->EndZ = Pt(m, i2)->Z;
}

// PANZERS 0x58f9b0 TurnInAngle_Out___Departure_With_Y_Turn
void TurnInAngleOutYTurn(SWpm* w)
{
    int mi = AddManoeuvre(w, 6, "TurnInAngle_Out___Departure_With_Y_Turn");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    int i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPoint;
    Pt(m, i)->X = w->X;
    Pt(m, i)->Z = w->Z;
    float a = w->DirToWp;
    float b = w->OutDir;
    int i1 = AddPoint(m);
    Pt(m, i1)->Reverse = !w->Reverse;
    int i2 = AddPoint(m);
    Pt(m, i2)->Reverse = w->Reverse;
    int i3 = AddPoint(m);
    Pt(m, i3)->Reverse = w->Reverse;
    TurnCircle(w, a, b, true, Pt(m, i1), Pt(m, i3), false, m->Circle0, m->Circle1);
    Pt(m, i2)->Type = kWpPointDir;
    Pt(m, i2)->X = Pt(m, i1)->X;
    Pt(m, i2)->Z = Pt(m, i1)->Z;
    Pt(m, i2)->Dir = (float)Wrap((double)Pt(m, i1)->Dir + kHdPi);
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = Pt(m, i3)->X;
    m->EndZ = Pt(m, i3)->Z;
}

// PANZERS 0x58ec70 TurnInAngle_InAndOut___Through_Waypoint_With_YTurn
void ThroughWaypointYTurn(SWpm* w)
{
    double d = fabs((double)w->DirToWp - (double)w->OutDir);
    double d2 = d > kHdPi ? kHdTwoPi - d : d;
    if (d2 > kHalfPi)
        return;
    int mi = AddManoeuvre(w, 11, "TurnInAngle_InAndOut___Through_Waypoint_With_YTurn");
    if (mi < 0)
        return;
    float fa = (float)Wrap((double)w->DirToWp + kHdPi);
    float fb = (float)Wrap((double)w->OutDir + kHdPi);
    double x = fabs(Wrap((double)fb - (double)fa));
    double y = Wrap(kHdPi - x) * 0.5;
    double r08 = (double)w->Radius * kTanScale;
    float k = (float)(r08 / Tan((double)(float)y));
    float sa = (float)Sin((double)fa);
    float ca = (float)Cos((double)fa);
    float sb = (float)Sin((double)fb);
    float cb = (float)Cos((double)fb);
    SManoeuvre* m = Man(w, mi);
    int i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPoint;
    Pt(m, i)->X = w->X;
    Pt(m, i)->Z = w->Z;
    i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    float ux = sa * k;
    float uz = ca * k;
    Pt(m, i)->Type = kWpPointDir;
    Pt(m, i)->X = w->X - ux;
    Pt(m, i)->Z = w->Z - uz;
    Pt(m, i)->Dir = w->DirToWp;
    i = AddPoint(m);
    Pt(m, i)->Reverse = !w->Reverse;
    float vx = sb * k;
    float vz = cb * k;
    Pt(m, i)->Type = kWpPointDir;
    Pt(m, i)->X = vx + w->X;
    Pt(m, i)->Z = w->Z + vz;
    Pt(m, i)->Dir = fb;
    i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPointDir;
    Pt(m, i)->X = w->X;
    Pt(m, i)->Z = w->Z;
    Pt(m, i)->Dir = w->OutDir;
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = w->X;
    m->EndZ = w->Z;
}

// PANZERS 0x58e500 TurnInAngle_InAndOut___Through_Waypoint_With_Turning_Radius
// (not executed in the menu)
void ThroughWaypointRadius(SWpm* w, float radius)
{
    int mi = AddManoeuvre(w, 10, "TurnInAngle_InAndOut___Through_Waypoint_With_Turning_Radius");
    if (mi < 0)
        return;
    float fa = w->DirToWp;
    float fb = w->OutDir;
    double d = fabs(Wrap((double)fb - (double)fa));
    double e = Wrap(kHdPi - d) * 0.5;
    float k = (float)((double)radius / Tan((double)(float)e));
    float sa = (float)Sin((double)fa);
    float ca = (float)Cos((double)fa);
    float sb = (float)Sin((double)fb);
    float cb = (float)Cos((double)fb);
    SManoeuvre* m = Man(w, mi);
    int i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    float ux = sa * k;
    float uz = ca * k;
    Pt(m, i)->Type = kWpPointDir;
    Pt(m, i)->X = w->X - ux;
    Pt(m, i)->Z = w->Z - uz;
    Pt(m, i)->Dir = fa;
    i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    float vx = sb * k;
    float vz = cb * k;
    Pt(m, i)->Type = kWpPointDir;
    Pt(m, i)->X = vx + w->X;
    Pt(m, i)->Z = w->Z + vz;
    Pt(m, i)->Dir = fb;
    m->StartX = Pt(m, 0)->X;
    m->StartZ = Pt(m, 0)->Z;
    m->EndX = Pt(m, 1)->X;
    m->EndZ = Pt(m, 1)->Z;
}

// PANZERS 0x58e860 ..._With_Turning_Radius_With_Outer_Points (not executed
// in the menu). Scales the radius by 1.7 for its two turning circles.
void ThroughWaypointOuterPoints(SWpm* w)
{
    if (w->InDir == w->OutDir) {
        TurnInAngleIn(w);
        return;
    }
    w->Radius = w->Radius * kRadiusScale;
    int mi = AddManoeuvre(w, 12,
        "TurnInAngle_InAndOut___Through_Waypoint_With_Turning_Radius_With_Outer_Points");
    if (mi < 0)
        return;
    float fa = w->DirToWp;
    double h = Wrap((double)w->OutDir - (double)fa) * 0.5;
    float mid = (float)Wrap(h + (double)fa);
    float m2 = (float)Wrap((double)mid + kHdPi);
    double b = Wrap((double)fa + kHdPi);
    SManoeuvre* m = Man(w, mi);
    int i0 = AddPoint(m);
    Pt(m, i0)->Reverse = w->Reverse;
    int i1 = AddPoint(m);
    Pt(m, i1)->Reverse = w->Reverse;
    TurnCircle(w, m2, (float)b, false, Pt(m, i1), Pt(m, i0), true, m->Circle0, m->Circle1);
    int i2 = AddPoint(m);
    Pt(m, i2)->Reverse = w->Reverse;
    Pt(m, i2)->Type = kWpPointDir;
    Pt(m, i2)->X = w->X;
    Pt(m, i2)->Z = w->Z;
    Pt(m, i2)->Dir = mid;
    m->StartX = Pt(m, i0)->X;
    m->StartZ = Pt(m, i0)->Z;
    float ob = w->OutDir;
    int i3 = AddPoint(m);
    Pt(m, i3)->Reverse = w->Reverse;
    int i4 = AddPoint(m);
    Pt(m, i4)->Reverse = w->Reverse;
    TurnCircle(w, mid, ob, false, Pt(m, i3), Pt(m, i4), false, m->Circle0, m->Circle1);
    m->EndX = Pt(m, i4)->X;
    m->EndZ = Pt(m, i4)->Z;
    w->Radius = w->Radius / kRadiusScale;
}

// PANZERS 0x58fce0 TurnInAngle___Through_Next_Waypoint (not executed in the
// menu): intersects the incoming line, the line of the next waypoint and
// cuts the corner with the turning radius.
void ThroughNextWaypoint(SWpm* w)
{
    if (w->PointType == kLwpBeforeLast)
        return;
    double d2 = Wrap((double)w->DirToWp + kHdPi);
    double o = (double)w->OutDir;
    double n = (double)w->NextOutDir;
    double ad = fabs(o - d2);
    double prod = (o - n) * (n - d2);
    bool go = (prod < 0.0 || prod != prod) ? (ad > kHdPi) : (kHdPi > ad);
    if (!go)
        return;
    int mi = AddManoeuvre(w, 13, "TurnInAngle___Through_Next_Waypoint");
    if (mi < 0)
        return;
    float px = 0.0f;
    float pz = 0.0f;
    float fa = w->DirToWp;
    float c1 = (float)(-Cos((double)fa));
    float s1 = (float)Sin((double)fa);
    float t = w->Z * s1;
    float t2 = w->X * c1;
    float k1 = -(t + t2);
    float c2 = (float)(-Cos((double)w->OutDir));
    float s2 = (float)Sin((double)w->OutDir);
    t = w->Z * s2;
    t2 = w->X * c2;
    float k2 = -(t + t2);
    float on = w->NextOutDir;
    float c3 = (float)(-Cos((double)on));
    float s3 = (float)Sin((double)on);
    t = w->NextZ * s3;
    t2 = w->NextX * c3;
    float k3 = -(t + t2);
    float d1 = s3 * c1;
    float d0 = c3 * s1;
    float det = d1 - d0;
    if (det != 0.0f) {
        float inv = 1.0f / det;
        float a0 = k3 * s1;
        float a2 = k1 * s3;
        float a3 = k1 * c3;
        a2 = a2 - a0;
        float b0 = k3 * c1;
        b0 = b0 - a3;
        a2 = a2 * inv;
        px = -a2;
        b0 = b0 * inv;
        pz = -b0;
    }
    double dd = fabs(Wrap((double)on - (double)fa));
    double e = Wrap(kHdPi - dd) * 0.5;
    float r = (float)((double)w->Radius / Tan((double)(float)e));
    float sA = (float)Sin((double)fa);
    float cA = (float)Cos((double)fa);
    float sB = (float)Sin((double)on);
    float cB = (float)Cos((double)on);
    float sAr = sA * r;
    float cAr = cA * r;
    float q1x = px - sAr;
    float cBr = cB * r;
    float q1z = pz - cAr;
    float sBr = sB * r;
    float q2z = cBr + pz;
    float u = pz * s2;
    float q2x = sBr + px;
    float v = px * c2;
    float test1 = u + v;
    test1 = test1 + k2;
    float sign1 = SignF(test1);
    float w5 = q1z * s2;
    float w6 = q1x * c2;
    w5 = w5 + w6;
    w5 = w5 + k2;
    float sign2 = SignF(w5);
    SManoeuvre* m = Man(w, mi);
    if (sign1 == sign2) {
        int i = AddPoint(m);
        Pt(m, i)->Reverse = w->Reverse;
        Pt(m, i)->Type = kWpPointDir;
        Pt(m, i)->X = w->X;
        Pt(m, i)->Z = w->Z;
        Pt(m, i)->Dir = fa;
    }
    int i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPointDir;
    Pt(m, i)->X = q1x;
    Pt(m, i)->Z = q1z;
    Pt(m, i)->Dir = fa;
    i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPointDir;
    Pt(m, i)->X = q2x;
    Pt(m, i)->Z = q2z;
    Pt(m, i)->Dir = on;
    float sign1b = SignF(test1);
    float y0 = q2z * s2;
    float y5 = q2x * c2;
    y0 = y0 + y5;
    y0 = y0 + k2;
    float sign3 = SignF(y0);
    if (sign1b == sign3) {
        i = AddPoint(m);
        Pt(m, i)->Reverse = w->Reverse;
        Pt(m, i)->Type = kWpPointDir;
        Pt(m, i)->X = w->NextX;
        Pt(m, i)->Z = w->NextZ;
        Pt(m, i)->Dir = on;
    }
    if (m->Points.Size <= 0) {
        DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SWayPoint", 0);
        return;
    }
    m->StartX = Pt(m, 0)->X;
    m->StartZ = Pt(m, 0)->Z;
    int last = m->Points.Size - 1;
    m->EndX = Pt(m, last)->X;
    m->EndZ = Pt(m, last)->Z;
}

// PANZERS 0x58e0f0 Simple_In_Waypoint (not executed in the menu)
void SimpleIn(SWpm* w)
{
    int mi = AddManoeuvre(w, 0x11, "Simple_In_Waypoint");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    int i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpPoint;
    Pt(m, i)->X = w->X;
    Pt(m, i)->Z = w->Z;
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = w->X;
    m->EndZ = w->Z;
}

// PANZERS 0x58e1e0 Simple_Out_Waypoint (not executed in the menu)
void SimpleOut(SWpm* w)
{
    int mi = AddManoeuvre(w, 0x10, "Simple_Out_Waypoint");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    for (int k = 0; k < 2; ++k) {
        int i = AddPoint(m);
        Pt(m, i)->Reverse = w->Reverse;
        Pt(m, i)->Type = kWpPoint;
        Pt(m, i)->X = w->X;
        Pt(m, i)->Z = w->Z;
    }
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = w->X;
    m->EndZ = w->Z;
}

// PANZERS 0x58d3f0 FollowWP
void FollowWP(SWpm* w)
{
    int mi = AddManoeuvre(w, 0xf, "FollowWP");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    int i = AddPoint(m);
    Pt(m, i)->Reverse = w->Reverse;
    Pt(m, i)->Type = kWpFollowUnit;
    Pt(m, i)->X = w->X;
    Pt(m, i)->Z = w->Z;
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = w->X;
    m->EndZ = w->Z;
}

// PANZERS 0x58cc90: every flagged manoeuvre starts at the first flagged
// manoeuvre's first point (inserted when it differs).
void CommonStart(SWpm* w)
{
    SWayPoint first;
    memset(&first, 0, sizeof(first));
    int n = w->Manoeuvres.Size;
    bool found = false;
    for (int i = 0; i < n; ++i) {
        if (w->Flags & (1u << (i & 31))) {
            SManoeuvre* m = Man(w, i);
            if (m->Points.Size < 1) {
                DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SWayPoint", 0);
                return;
            }
            first = m->Points.Array[0];
            found = true;
            break;
        }
    }
    if (!found)
        return;                                   // nothing flagged: no writes
    for (int j = 0; j < w->Manoeuvres.Size; ++j) {
        if (!(w->Flags & (1u << (j & 31))))
            continue;
        SManoeuvre* m = Man(w, j);
        if (m->Points.Size < 1) {
            DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SWayPoint", 0);
            return;
        }
        SWayPoint* p = &m->Points.Array[0];
        if (p->X == first.X && p->Z == first.Z && p->Dir == first.Dir && p->Type == first.Type)
            continue;
        int k = InsertPoint(m, 0);
        if (k < 0 || k >= m->Points.Size)
            return;
        m->Points.Array[k] = first;
    }
}

} // namespace

// PANZERS 0x58d2f0
void Wpm_ClearManoeuvres(SWayPointWithManoeuvres* w, int n)
{
    SHdDArray<SManoeuvre>& a = w->Manoeuvres;
    if (a.Size != 0 && !a.Array) {
        DrvPanic("SDArray<%s>::Clear: array is damaged", "SManoeuvre");
        return;
    }
    for (int i = 0; i < a.Size; ++i)
        FreeManoeuvre(&a.Array[i]);
    a.Size = n;
    if (a.Max < n) {
        a.Max = n;
        a.Array = (SManoeuvre*)::realloc(a.Array, (size_t)n * sizeof(SManoeuvre));
    }
    if (a.Array)
        memset(a.Array, 0, (size_t)a.Max * sizeof(SManoeuvre));
}

// PANZERS 0x54fc60
void Wpm_Free(SWayPointWithManoeuvres* w)
{
    SHdDArray<SManoeuvre>& a = w->Manoeuvres;
    for (int i = 0; i < a.Size; ++i)
        FreeManoeuvre(&a.Array[i]);
    if (a.Array) {
        ::free(a.Array);
        a.Array = nullptr;
    }
    a.Max = 0;
    a.Size = 0;
}

// PANZERS 0x58dba0
int Wpm_PopBest(SWayPointWithManoeuvres* w)
{
    int best = -1;
    int bestIndex = 0;
    for (int i = 0; i < 32; ++i) {
        if (w->Flags & (1u << i)) {
            if (i >= w->Manoeuvres.Size) {
                DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SManoeuvre", i);
                return -1;
            }
            int idx = w->Manoeuvres.Array[i].Index;
            if (best < 0 || idx < bestIndex) {
                bestIndex = idx;
                best = i;
            }
        }
    }
    if (best > -1)
        w->Flags &= ~(1u << best);
    return best;
}

// PANZERS 0x58e050
void Wpm_FilterByStart(SWayPointWithManoeuvres* w, float x, float z)
{
    int n = w->Manoeuvres.Size;
    for (int i = 0; i < n; ++i) {
        float dz = z - w->Z;
        float dx = x - w->X;
        dz = dz * dz;
        dx = dx * dx;
        float d = dx + dz;
        if (w->Manoeuvres.Array[i].Dist2 > d)
            w->Flags &= ~(1u << (i & 31));
    }
    CommonStart(w);
}

// PANZERS 0x58daa0 StopWP
void Wpm_AddStop(SWayPointWithManoeuvres* w, bool unitReverse)
{
    int mi = AddManoeuvre(w, 0xe, "StopWP");
    if (mi < 0)
        return;
    SManoeuvre* m = Man(w, mi);
    int i = AddPoint(m);
    Pt(m, i)->Reverse = unitReverse;
    Pt(m, i)->Type = kWpPoint;
    Pt(m, i)->X = w->X;
    Pt(m, i)->Z = w->Z;
    m->StartX = w->X;
    m->StartZ = w->Z;
    m->EndX = w->X;
    m->EndZ = w->Z;
}

// PANZERS 0x58e350 SWayPointWithManoeuvres::SortManoeuvres(maxDist2)
static void SortManoeuvres(SWayPointWithManoeuvres* w, float maxDist2)
{
    for (int i = 0; i < w->Manoeuvres.Size; ++i) {
        SManoeuvre* m = Man(w, i);
        m->Index = i;
        float dx = m->StartX - w->X;
        float dz = m->StartZ - w->Z;
        dx = dx * dx;
        dz = dz * dz;
        m->Dist2 = dx + dz;
    }
    if (w->PointType != kLwpFirst) {
        int i = 0;
        while (i < w->Manoeuvres.Size) {
            if (Man(w, i)->Dist2 > maxDist2)
                RemoveManoeuvre(w, i);
            else
                ++i;
        }
    }
    if (w->PointType != kLwpLastStop && w->PointType != kLwpLastPass) {
        int i = 0;
        while (i < w->Manoeuvres.Size) {
            SManoeuvre* m = Man(w, i);
            float nx = w->NextX - w->X;
            float nz = w->NextZ - w->Z;
            float ez = m->EndZ - w->Z;
            float ex = m->EndX - w->X;
            nx = nx * nx;
            ex = ex * ex;
            ez = ez * ez;
            nz = nz * nz;
            float e = ex + ez;
            float nn = nx + nz;
            if (e > nn && m->Type != 0xd)
                RemoveManoeuvre(w, i);
            else
                ++i;
        }
    }
    HdQsort(w->Manoeuvres.Array, w->Manoeuvres.Size);
    int n = w->Manoeuvres.Size;
    if ((unsigned)n > 0x20) {
        DrvPanic("SWayPointWithManoeuvres::SortManoeuvres - sizeof ManoeuvresArrayFlag is too small!");
        return;
    }
    unsigned f = w->Flags;
    for (int i = 0; i < n; ++i)
        f |= 1u << i;
    if (n > 0)
        w->Flags = f;
}

// PANZERS 0x58d4e0 SWayPointWithManoeuvres::CreateManoeuvres
void Wpm_CreateManoeuvres(SWayPointWithManoeuvres* w, int driverType, STarget* target,
                          float radius, const SGhostFrame* frame)
{
    if (!target) {
        DrvPanic("SWayPointWithManoeuvres::CreateManoeuvres(): NewTarget is NULL");
        return;
    }
    Wpm_ClearManoeuvres(w, 0);
    w->Flags = 0;
    bool rev = target->Reverse;
    w->Radius = radius;
    w->Reverse = rev;
    float dx = w->X - frame->X;
    float dz = w->Z - frame->Z;
    w->DirToWp = Atan2f((double)dx, (double)dz);   // fstp dword
    if (kEps > fabs((double)dx) && kEps > fabs((double)dz)) {
        double d;
        if (rev)
            d = Wrap((double)frame->Dir + kHdPi);
        else
            d = (double)frame->Dir;
        w->DirToWp = (float)d;
    }
    double t = fabs((double)w->DirToWp - (double)w->OutDir);
    float turn = (float)(t > kHdPi ? kHdTwoPi - t : t);
    bool badType = false;
    if (w->PointType == kLwpFirst) {
        switch (driverType) {
        case 0:
            TurnInPlaceOutAngle(w, kPiThird);
            TurnInPlaceOut(w);
            break;
        case 1:
            if ((double)turn > kHalfPi) {
                TurnInAngleOutYTurn(w);
                TurnInAngleOutRadius(w);
            } else {
                TurnInAngleOutRadius(w);
                TurnInAngleOutYTurn(w);
            }
            ThroughWaypointYTurn(w);
            TurnInPlaceOut(w);
            break;
        case 3:
        case 10:
            TurnInPlaceOut(w);
            break;
        case 5:
            TurnInPlaceOutAngle(w, kPiHalfF);
            TurnInPlaceOut(w);
            break;
        case 13:
            SimpleOut(w);
            break;
        default:
            badType = true;
            break;
        }
        if (!badType) {
            // The ghost's own position and reverse flag go in front of every
            // first-waypoint manoeuvre.
            for (int i = 0; i < w->Manoeuvres.Size; ++i) {
                SManoeuvre* m = Man(w, i);
                if (InsertPoint(m, 0) != 0) {
                    DrvPanic("SWayPointWithManoeuvres::CreateManoeuvres - idx!=0");
                    return;
                }
                if (m->Points.Size < 2) {
                    DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SWayPoint", 1);
                    return;
                }
                SWayPoint* p = m->Points.Array;
                p[0] = p[1];
                if (m->Points.Size < 3) {
                    DrvPanic("SDArray<%s>::operator[]: invalid index (%d)", "SWayPoint", 2);
                    return;
                }
                p[1].Reverse = p[2].Reverse;
                p[0].Reverse = frame->Reverse;
            }
        }
    } else if (w->PointType == kLwpLastStop || w->PointType == kLwpLastPass) {
        if (target->IsUnitOrder() && w->PointType == kLwpLastStop) {
            FollowWP(w);
        } else {
            switch (driverType) {
            case 0:
            case 3:
            case 5:
                TurnInPlaceIn(w, kWpPointDir);
                break;
            case 1:
                if ((double)turn > kHalfPi) {
                    TurnInAngleInRadius(w, true);
                    TurnInAngleInRadius(w, false);
                } else {
                    TurnInAngleInRadius(w, false);
                    TurnInAngleInRadius(w, true);
                }
                ThroughWaypointYTurn(w);
                TurnInPlaceIn(w, kWpPointDir);
                break;
            case 10:
                TurnInPlaceIn(w, kWpPoint);
                break;
            case 13:
                SimpleIn(w);
                break;
            default:
                badType = true;
                break;
            }
        }
    } else {
        switch (driverType) {
        case 0:
            ThroughWaypointRadius(w, radius);
            ThroughWaypointOuterPoints(w);
            TurnInAngleInRadius(w, false);
            TurnInAngleOutRadius(w);
            ThroughNextWaypoint(w);
            TurnInPlaceOutAngle(w, kPiThird);
            TurnInPlaceOut(w);
            break;
        case 1:
            ThroughWaypointRadius(w, radius);
            ThroughNextWaypoint(w);
            TurnInAngleOutRadius(w);
            ThroughWaypointOuterPoints(w);
            ThroughWaypointYTurn(w);
            TurnInPlaceOut(w);
            break;
        case 3:
            TurnInAngleInRadius(w, false);
            TurnInPlaceOut(w);
            break;
        case 5:
            ThroughWaypointRadius(w, radius);
            ThroughWaypointOuterPoints(w);
            TurnInPlaceOutAngle(w, kPiHalfF);
            TurnInPlaceOut(w);
            break;
        case 10:
            TurnInPlaceOut(w);
            break;
        case 13:
            SimpleIn(w);
            break;
        default:
            badType = true;
            break;
        }
    }
    if (badType) {
        DrvPanic("SWayPointWithManoeuvres::CreateManoeuvres - Bad driver_type");
        return;
    }
    float ex = w->X - frame->X;
    float ez = w->Z - frame->Z;
    ex = ex * ex;
    ez = ez * ez;
    SortManoeuvres(w, ex + ez);
}

} // namespace pz
