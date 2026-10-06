// src/3dengine/pz/tracks.cpp
// HD animation tracks. OWNER: agent A. See tracks.h.
//
// Notes that hold for every evaluator below (checked in the disassembly):
// - The time folding uses FRNDINT under FLDCW 0x047f (round down = floor);
//   the packed key index uses FISTP under FLDCW 0x0c7f (truncate). Both
//   control words are HD's (0x8de15c / 0x8de164); the decompiler hides them.
// - The libm calls are the SSE2 __libm_sse2_cos (0x78d480) and
//   __libm_sse2_sin (0x78d640), on doubles.

#include <math.h>
#include <string.h>
#include "tracks.h"
#include "stream.h"
#include "logger.h"

namespace pz {

static inline float RoundNearest(float v) { return floorf(v); }   // FRNDINT, RC = down
static inline int   RoundToInt(float v)   { return (int)v; }       // FISTP, RC = truncate

// The "fold time into key range" switch every key track starts with.
// Returns false (and *which = 0 first / 1 last) when the ORT says "hold".
static bool FoldTime(float* t, float t0, float t1, int ortBefore, int ortAfter, int* which,
                     bool strict = false)
{
    float f, range;
    if (strict ? (t0 < *t) : (t0 <= *t)) {
        if (*t < t1)
            return true;
        switch (ortAfter) {
        case 0:
            *which = 1;
            return false;
        case 1:
        case 2:
            f = (*t - t0) / (t1 - t0);
            *t = *t - RoundNearest(f) * (t1 - t0);
            return true;
        case 3:
            range = (t1 - t0) * 2.0f;
            f = (*t - t0) / range;
            *t = *t - RoundNearest(f) * range;
            if (t1 <= *t)
                *t = t1 * 2.0f - *t;
            return true;
        default:
            return true;
        }
    }
    switch (ortBefore) {
    case 0:
        *which = 0;
        return false;
    case 1:
    case 2:
        f = (*t - t0) / (t1 - t0);
        *t = *t - RoundNearest(f) * (t1 - t0);
        return true;
    case 3:
        range = (t1 - t0) * 2.0f;
        f = (*t - t0) / range;
        *t = *t - RoundNearest(f) * range;
        if (t1 <= *t)
            *t = t1 * 2.0f - *t;
        return true;
    default:
        return true;
    }
}

// PANZERS 0x676560
// Quaternion slerp with HD's atan approximation (x / (1 + 0.280872 x^2)).
float* QuatSlerp(float* out, const float* a, const float* b, float t)
{
    double c = (double)(b[1] * a[1] + b[0] * a[0] + b[2] * a[2] + b[3] * a[3]);
    float sign = 1.0f;
    if (c < 0.0) {
        c = -c;
        sign = -1.0f;
    }
    double wa, wb;
    if (c <= 0.999999) {
        double s = sqrt(1.0 - c * c);
        double inv = 1.0 / s;
        double angle;
        if (c <= s) {
            double x = inv * c;
            angle = 1.5707963705062866 - x / (x * 0.280872 * x + 1.0);
        } else {
            double x = s / c;
            angle = x / (x * 0.280872 * x + 1.0);
        }
        double tb = (double)t * angle;
        double ta = angle - tb;
        wa = sin(ta) * inv;
        wb = sin(tb) * inv;
    } else {
        wa = 1.0 - (double)t;
        wb = (double)t;
    }
    wb = (double)sign * wb;
    out[0] = (float)((double)b[0] * wb + (double)a[0] * wa);
    out[1] = (float)((double)b[1] * wb + (double)a[1] * wa);
    out[2] = (float)((double)b[2] * wb + (double)a[2] * wa);
    out[3] = (float)((double)b[3] * wb + (double)a[3] * wa);
    return out;
}

// PANZERS 0x672c90
float* QuatFromEuler(float* out, float x, float y, float z)
{
    y = y * 0.5f;
    double cy = cos((double)y);
    z = z * 0.5f;
    double cz = cos((double)z);
    x = x * 0.5f;
    double sx = sin((double)x);
    double sy = sin((double)y);
    double sz = sin((double)z);
    double cx = cos((double)x);
    out[3] = (float)(sz * sy * sx + cx * cz * cy);
    out[0] = (float)(sx * cz * cy - cx * sz * sy);
    out[1] = (float)(sy * cz * cx + sz * cy * sx);
    out[2] = (float)(sz * cy * cx - sy * cz * sx);
    return out;
}

// ---- SKeyTrackFloat ----

// PANZERS 0x671100
void SKeyTrackFloat::Load(SStream* is)
{
    Count = is->ReadInt();
    OrtBefore = is->ReadByte();
    OrtAfter = is->ReadByte();
    Keys = (float*)operator new((size_t)Count * 0x10);
    is->Read(Keys, Count << 4);
}

SKeyTrackFloat::~SKeyTrackFloat()
{
    operator delete(Keys);
}

// PANZERS 0x6746d0
float SKeyTrackFloat::Evaluate(float t) const
{
    int n = Count;
    const float* k = Keys;
    if (n < 2)
        return k[1];
    int which;
    if (!FoldTime(&t, k[0], k[n * 4 - 4], OrtBefore, OrtAfter, &which))
        return which ? k[n * 4 - 3] : k[1];
    int i = 0;
    if (0 < n - 2) {
        const float* p = Keys;
        do {
            p += 4;
            if (t < *p)
                break;
            ++i;
        } while (i < n - 2);
    }
    float a = k[i * 4];
    if (t < a)
        t = a;
    float b = k[i * 4 + 4];
    if (b <= t)
        t = b;
    float s = (t - a) / (b - a);
    float s2 = s * s;
    float s3 = s2 * s;
    return ((s3 * 2.0f - s2 * 3.0f) + 1.0f) * k[i * 4 + 1]
         + (s2 * 3.0f - s3 * 2.0f) * k[i * 4 + 5]
         + ((s3 - s2 * 2.0f) + s) * k[i * 4 + 3]
         + (s3 - s2) * k[i * 4 + 6];
}

// ---- STrackBool ----

// PANZERS 0x671090
void STrackBool::Load(SStream* is)
{
    Count = is->ReadInt();
    OrtBefore = is->ReadByte();
    OrtAfter = is->ReadByte();
    Keys = (float*)operator new((size_t)Count * 8);
    is->Read(Keys, Count << 3);
}

STrackBool::~STrackBool()
{
    operator delete(Keys);
}

// PANZERS 0x674520
bool STrackBool::Evaluate(float t) const
{
    int n = Count;
    const float* k = Keys;
    if (n < 2)
        return k[1] != 0.0f;
    int which;
    if (!FoldTime(&t, k[0], k[n * 2 - 2], OrtBefore, OrtAfter, &which))
        return which ? k[n * 2 - 1] != 0.0f : k[1] != 0.0f;
    int i = 0;
    if (0 < n - 2) {
        const float* p = Keys;
        do {
            p += 2;
            if (t < *p)
                break;
            ++i;
        } while (i < n - 2);
    }
    // HD tests the key's dword, not the float (int compare with 0).
    int v;
    memcpy(&v, &k[i * 2 + 1], 4);
    return v != 0;
}

// ---- STrackLink ----

// PANZERS 0x6711c0
void STrackLink::Load(SStream* is)
{
    Count = is->ReadInt();
    Keys = (unsigned char*)operator new((size_t)Count * 0x54);
    memset(Keys, 0, (size_t)Count * 0x54);   // HD runs the key ctor 0x671000
    for (int i = 0; i < Count; ++i) {
        unsigned char* key = Keys + i * 0x54;
        *(float*)(key + 0x4c) = is->ReadFloat();
        *(int*)(key + 0x50) = is->ReadInt();
        is->Read(key + 0x30, 0xc);
        is->Read(key + 0x3c, 0x10);
        is->Read(key, 0x30);
    }
}

STrackLink::~STrackLink()
{
    operator delete(Keys);
}

// PANZERS 0x674960
// Picks the key in effect at t: node index (+0x50) and the channel block
// (matrix, position, quaternion = 0x4c bytes).
void STrackLink::Evaluate(float t, int* node, float* channel) const
{
    int i = 0;
    if (1 < Count) {
        int j = 1;
        const float* p = (const float*)(Keys + 0xa0);
        do {
            if (t < *p)
                break;
            ++j;
            ++i;
            p += 0x15;
        } while (j < Count);
    }
    const unsigned char* key = Keys + i * 0x54;
    *node = *(const int*)(key + 0x50);
    memcpy(channel, key, 0x4c);
}

// ---- STrackCameraChange ----

// PANZERS 0x671170
void STrackCameraChange::Load(SStream* is)
{
    Count = is->ReadInt();
    Keys = (float*)operator new((size_t)Count * 8);
    is->Read(Keys, Count << 3);
}

STrackCameraChange::~STrackCameraChange()
{
    operator delete(Keys);
}

// ---- STrackPositionBezier (CPOS) ----

static void ReadVec3(SStream* is, float* v)   // 0x65d740
{
    is->Read(v, 12);
}

// PANZERS 0x6712a0
STrackPositionBezier::STrackPositionBezier(SStream* is)
{
    Count = is->ReadInt();
    OrtBefore = is->ReadByte();
    OrtAfter = is->ReadByte();
    Keys = (float*)operator new((size_t)Count * 0x28);
    memset(Keys, 0, (size_t)Count * 0x28);
    for (int i = 0; i < Count; ++i) {
        float* k = Keys + i * 10;
        k[0] = is->ReadFloat();
        ReadVec3(is, k + 1);
        ReadVec3(is, k + 4);
        ReadVec3(is, k + 7);
    }
}

// PANZERS 0x672690
STrackPositionBezier::~STrackPositionBezier()
{
    operator delete(Keys);
    Keys = nullptr;
}

// PANZERS 0x674e50
float* STrackPositionBezier::Evaluate(float* out, float t)
{
    const float* k = Keys;
    int n = Count;
    if (n < 2) {
        out[0] = k[1]; out[1] = k[2]; out[2] = k[3];
        return out;
    }
    int which;
    if (!FoldTime(&t, k[0], k[n * 10 - 10], OrtBefore, OrtAfter, &which)) {
        const float* v = which ? k + n * 10 - 9 : k + 1;
        out[0] = v[0]; out[1] = v[1]; out[2] = v[2];
        return out;
    }
    int i = 0;
    if (0 < n - 2) {
        const float* p = Keys;
        do {
            p += 10;
            if (t < *p)
                break;
            ++i;
        } while (i < n - 2);
    }
    const float* a = k + i * 10;
    const float* b = a + 10;
    if (t < a[0])
        t = a[0];
    if (b[0] <= t)
        t = b[0];
    float s = (t - a[0]) / (b[0] - a[0]);
    float s2 = s * s;
    float s3 = s2 * s;
    float h4 = s3 - s2;
    float h3 = (s3 - s2 * 2.0f) + s;
    float h2 = s2 * 3.0f - s3 * 2.0f;
    float h1 = (s3 * 2.0f - s2 * 3.0f) + 1.0f;
    // value a (+4), value b (+0x2c), out-tangent a (+0x1c), in-tangent b (+0x38)
    out[1] = a[2] * h1 + b[2] * h2 + a[8] * h3 + b[5] * h4;
    out[0] = h1 * a[1] + b[1] * h2 + a[7] * h3 + b[4] * h4;
    out[2] = a[3] * h1 + b[3] * h2 + a[9] * h3 + b[6] * h4;
    return out;
}

// ---- STrackPositionIndependent (CXYZ) ----

// PANZERS 0x671420
STrackPositionIndependent::STrackPositionIndependent(SStream* is)
    : X(nullptr), Y(nullptr), Z(nullptr)
{
    while (!is->ReadChunkIsEnd()) {
        int id = is->ReadChunkHeader();
        SKeyTrackFloat* tr = new SKeyTrackFloat();
        tr->Load(is);
        if (id == 0x58425a43)        // CZBX
            X = tr;
        else if (id == 0x59425a43)   // CZBY
            Y = tr;
        else if (id == 0x5a425a43)   // CZBZ
            Z = tr;
        else {
            delete tr;
            throw "STrackPositionIndependent::STrackPositionIndependent: Invalid sub-controller";
        }
        is->ReadChunkValidate(0);
    }
}

// PANZERS 0x6726e0
STrackPositionIndependent::~STrackPositionIndependent()
{
    delete X;
    delete Y;
    delete Z;
}

// PANZERS 0x675170
float* STrackPositionIndependent::Evaluate(float* out, float t)
{
    float z = Z ? Z->Evaluate(t) : 0.0f;
    float y = Y ? Y->Evaluate(t) : 0.0f;
    float x = X ? X->Evaluate(t) : 0.0f;
    out[0] = x;
    out[1] = y;
    out[2] = z;
    return out;
}

// ---- STrackPositionPacked (CPSP) ----

// PANZERS 0x6715b0
STrackPositionPacked::STrackPositionPacked(SStream* is)
{
    Count = is->ReadInt();
    Start = is->ReadFloat();
    End = is->ReadFloat();
    Rate = (float)(Count - 1) / (End - Start);
    OrtBefore = is->ReadByte();
    OrtAfter = is->ReadByte();
    Keys = (float*)operator new((size_t)Count * 12);
    memset(Keys, 0, (size_t)Count * 12);
    is->Read(Keys, Count * 12);
}

// PANZERS 0x672710
STrackPositionPacked::~STrackPositionPacked()
{
    operator delete(Keys);
    Keys = nullptr;
}

// PANZERS 0x675220
float* STrackPositionPacked::Evaluate(float* out, float t)
{
    int n = Count;
    if (n < 2) {
        out[0] = Keys[0]; out[1] = Keys[1]; out[2] = Keys[2];
        return out;
    }
    int which;
    if (!FoldTime(&t, Start, End, OrtBefore, OrtAfter, &which)) {
        const float* v = which ? Keys + n * 3 - 3 : Keys;
        out[0] = v[0]; out[1] = v[1]; out[2] = v[2];
        return out;
    }
    int i = RoundToInt(Rate * (t - Start));   // FISTP, truncate
    if (i < n - 1) {
        if (i < 0)   // recompile guard: HD would read before the key array
            i = 0;
        float f = Rate * (t - Start) - (float)i;
        const float* a = Keys + i * 3;
        out[0] = (a[3] - a[0]) * f + a[0];
        out[1] = (a[4] - a[1]) * f + a[1];
        out[2] = (a[5] - a[2]) * f + a[2];
        return out;
    }
    const float* v = Keys + n * 3 - 3;
    out[0] = v[0]; out[1] = v[1]; out[2] = v[2];
    return out;
}

// ---- STrackRotationTCB (CROT) ----

// PANZERS 0x672830
// Incoming/outgoing tangent quaternions of a TCB key from its neighbours.
static void TcbTangents(float* key, const float* prev, const float* next, float dtPrev, float dtNext)
{
    double h = (double)prev[8] * 0.5;
    float px = (float)((double)prev[5] * h);
    float py = (float)((double)prev[6] * h);
    float pz = (float)((double)prev[7] * h);
    h = (double)next[8] * 0.5;
    float nx = (float)((double)next[5] * h);
    float ny = (float)((double)next[6] * h);
    float nz = (float)((double)next[7] * h);
    float cont = key[10];
    float ein = (float)((double)(dtPrev / (dtPrev + dtNext)) * 2.0);
    float eout = (float)((double)(dtNext / (dtPrev + dtNext)) * 2.0);
    float ac = fabsf(cont);
    ein = (ac + ein) - ac * ein;
    eout = (ac + eout) - ac * eout;
    double tens = 1.0 - (double)key[9];
    double halfT = tens * 0.5;
    double a = (1.0 - (double)cont) * halfT;
    double biasP = (double)key[11] + 1.0;
    float ka = (float)(1.0 - a * biasP * (double)ein);
    double biasM = 1.0 - (double)key[11];
    float kb = (float)(tens * -0.5 * ((double)cont + 1.0) * biasM * (double)ein);
    float kc = (float)(((double)cont + 1.0) * halfT * biasP * (double)eout);
    float kd = (float)(a * biasM * (double)eout - 1.0);

    float x = (float)((double)(px * ka + nx * kb) * 0.5);
    float y = (float)((double)(py * ka + ny * kb) * 0.5);
    float z = (float)((double)(pz * ka + nz * kb) * 0.5);
    float len = (float)sqrt((double)(y * y + x * x + z * z));
    double s = 1.0;
    if (0.0f < len)
        s = sin((double)len) / (double)len;
    key[14] = x * (float)s;
    key[15] = y * (float)s;
    key[16] = z * (float)s;
    key[17] = (float)cos((double)len);

    x = (float)((double)(px * kc + nx * kd) * 0.5);
    y = (float)((double)(py * kc + ny * kd) * 0.5);
    z = (float)((double)(pz * kc + nz * kd) * 0.5);
    len = (float)sqrt((double)(y * y + x * x + z * z));
    s = 1.0;
    if (0.0f < len)
        s = sin((double)len) / (double)len;
    key[18] = (float)s * x;
    key[19] = (float)s * y;
    key[20] = (float)s * z;
    key[21] = (float)cos((double)len);
}

// The end-key tangent when the track does not loop (0x671980 inline).
static void TcbEndTangents(float* inQ, float* outQ, const float* key, const float* axisKey)
{
    double h = (double)axisKey[8] * 0.5;
    float tc = key[10] * key[11];
    float ax = (float)((double)axisKey[5] * h);
    float ay = (float)((double)axisKey[6] * h);
    float az = (float)((double)axisKey[7] * h);
    double tens = 1.0 - (double)key[9];
    float kin = (float)(1.0 - (1.0 - (double)tc) * tens);
    float kout = (float)(((double)tc + 1.0) * tens - 1.0);
    float x = (float)((double)(ax * kout) * 0.5);
    float y = (float)((double)(ay * kout) * 0.5);
    float z = (float)((double)(az * kout) * 0.5);
    float len = (float)sqrt((double)(y * y + x * x + z * z));
    double s = 1.0;
    if (0.0f < len)
        s = sin((double)len) / (double)len;
    inQ[0] = x * (float)s;
    inQ[1] = y * (float)s;
    inQ[2] = z * (float)s;
    inQ[3] = (float)cos((double)len);
    x = (float)((double)(ax * kin) * 0.5);
    y = (float)((double)(ay * kin) * 0.5);
    z = (float)((double)(az * kin) * 0.5);
    len = (float)sqrt((double)(y * y + x * x + z * z));
    s = 1.0;
    if (0.0f < len)
        s = sin((double)len) / (double)len;
    outQ[0] = x * (float)s;
    outQ[1] = y * (float)s;
    outQ[2] = z * (float)s;
    outQ[3] = (float)cos((double)len);
}

// PANZERS 0x671980
// Keys (22 floats): t, absolute quaternion [1..4], axis [5..7], angle [8]
// (stored negated), tension [9], continuity [10], bias [11], ease in/out
// [12, 13], tangent-in quaternion [14..17], tangent-out [18..21].
STrackRotationTCB::STrackRotationTCB(SStream* is)
{
    Count = is->ReadInt();
    OrtBefore = is->ReadByte();
    OrtAfter = is->ReadByte();
    Keys = (float*)operator new((size_t)Count * 0x58);
    memset(Keys, 0, (size_t)Count * 0x58);
    for (int i = 0; i < Count; ++i) {
        float* k = Keys + i * 22;
        k[4] = 1.0f; k[17] = 1.0f; k[21] = 1.0f;
    }
    float acc[4] = { 0.0f, 0.0f, 0.0f, 1.0f };   // 0x7f7fc0 identity
    for (int i = 0; i < Count; ++i) {
        float* k = Keys + i * 22;
        k[0] = is->ReadFloat();
        ReadVec3(is, k + 5);
        float angle = is->ReadFloat();
        k[8] = -angle;
        double h = (double)k[8] * 0.5;
        float s = (float)sin(h);
        float ax = k[5] * s, ay = k[6] * s, az = k[7] * s;
        float c = (float)cos(h);
        // acc = acc * (axis, angle), as the HD inline product
        float x = (acc[0] * c + ax * acc[3] + ay * acc[2]) - az * acc[1];
        float y = (acc[1] * c + ay * acc[3] + az * acc[0]) - ax * acc[2];
        float z = (acc[2] * c + az * acc[3] + ax * acc[1]) - ay * acc[0];
        float w = ((acc[3] * c - ax * acc[0]) - ay * acc[1]) - az * acc[2];
        k[1] = x; k[2] = y; k[3] = z; k[4] = w;
        k[9] = is->ReadFloat();
        k[10] = is->ReadFloat();
        k[11] = is->ReadFloat();
        k[12] = is->ReadFloat();
        k[13] = is->ReadFloat();
        acc[0] = x; acc[1] = y; acc[2] = z; acc[3] = w;
    }
    if (1 < Count) {
        for (int i = 1; i < Count - 1; ++i) {
            float* k = Keys + i * 22;
            TcbTangents(k, k, k + 22, k[0] - k[-22], k[22] - k[0]);
        }
        float* first = Keys;
        float* last = Keys + (Count - 1) * 22;
        if (OrtBefore == 2)
            TcbTangents(first, last, first + 22, last[0] - Keys[(Count - 2) * 22], first[22] - first[0]);
        else
            TcbEndTangents(first + 14, first + 18, first, first + 22);
        if (OrtAfter == 2)
            TcbTangents(last, last, first + 22, last[0] - Keys[(Count - 2) * 22], first[22] - first[0]);
        else
            TcbEndTangents(last + 14, last + 18, last, last);
    }
}

// PANZERS 0x6727e0
STrackRotationTCB::~STrackRotationTCB()
{
    operator delete(Keys);
    Keys = nullptr;
}

// PANZERS 0x675d40
float* STrackRotationTCB::Evaluate(float* out, float t)
{
    const float* k = Keys;
    int n = Count;
    if (n < 2) {
        out[0] = k[1]; out[1] = k[2]; out[2] = k[3]; out[3] = k[4];
        return out;
    }
    // HD compares "t0 < t" (not <=) for the before/after split here.
    int which;
    if (!FoldTime(&t, k[0], k[n * 22 - 22], OrtBefore, OrtAfter, &which, true)) {
        const float* q = which ? k + n * 22 - 21 : k + 1;
        out[0] = q[0]; out[1] = q[1]; out[2] = q[2]; out[3] = q[3];
        return out;
    }
    int i = 0;
    if (0 < n - 2) {
        const float* p = Keys;
        do {
            p += 22;
            if (t < *p)
                break;
            ++i;
        } while (i < n - 2);
    }
    const float* a = k + i * 22;
    const float* b = a + 22;
    if (t < a[0])
        t = a[0];
    if (b[0] <= t)
        t = b[0];
    float f = (t - a[0]) / (b[0] - a[0]);
    double h = (double)(b[8] * f) * 0.5;
    float s = (float)sin(h);
    float ax = b[5] * s, ay = b[6] * s, az = b[7] * s;
    float c = (float)cos(h);
    float qx = a[1], qy = a[2], qz = a[3], qw = a[4];
    float rx = (qx * c + qw * ax + qz * ay) - qy * az;
    float ry = (qy * c + qw * ay + qx * az) - qz * ax;
    float rz = (qz * c + qw * az + qy * ax) - qx * ay;
    float rw = ((qw * c - qx * ax) - qy * ay) - qz * az;
    float s1[4], s2[4];
    QuatSlerp(s1, a + 18, b + 14, f);
    static const float kIdentity[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    QuatSlerp(s2, kIdentity, s1, (float)((1.0 - (double)f) * 2.0 * (double)f));
    out[0] = (rx * s2[3] + rw * s2[0] + rz * s2[1]) - ry * s2[2];
    out[1] = (ry * s2[3] + rw * s2[1] + rx * s2[2]) - rz * s2[0];
    out[2] = (rz * s2[3] + rw * s2[2] + ry * s2[0]) - rx * s2[1];
    out[3] = ((rw * s2[3] - rx * s2[0]) - ry * s2[1]) - rz * s2[2];
    return out;
}

// ---- STrackRotationEuler (CEUL) ----

// PANZERS 0x6716d0
STrackRotationEuler::STrackRotationEuler(SStream* is)
    : X(nullptr), Y(nullptr), Z(nullptr)
{
    while (!is->ReadChunkIsEnd()) {
        int id = is->ReadChunkHeader();
        SKeyTrackFloat* tr = new SKeyTrackFloat();
        tr->Load(is);
        if (id == 0x58425a43)
            X = tr;
        else if (id == 0x59425a43)
            Y = tr;
        else if (id == 0x5a425a43)
            Z = tr;
        else {
            delete tr;
            throw "STrackRotationEuler::STrackRotationEuler: Invalid sub-controller";
        }
        is->ReadChunkValidate(0);
    }
}

// PANZERS 0x672760
STrackRotationEuler::~STrackRotationEuler()
{
    delete X;
    delete Y;
    delete Z;
}

// PANZERS 0x675470
// Product of the three axis quaternions, transcribed with HD's "* 0.0" terms.
float* STrackRotationEuler::Evaluate(float* out, float t)
{
    float x = X ? X->Evaluate(t) : 0.0f;
    float y = Y ? Y->Evaluate(t) : 0.0f;
    float z = Z ? Z->Evaluate(t) : 0.0f;
    double hx = (double)x * 0.5;
    float sx = (float)sin(hx);
    float sx0 = sx * 0.0f;
    float cx = (float)cos(hx);
    double hy = (double)y * 0.5;
    float sy = (float)sin(hy);
    float sy0 = sy * 0.0f;
    float cy = (float)cos(hy);
    double hz = (double)z * 0.5;
    float sz = (float)sin(hz);
    float sz0 = sz * 0.0f;
    float cz = (float)cos(hz);
    float t14 = sz0 * sy0;
    float t15 = (sy0 * cz + sz0 * cy + t14) - sz * sy;
    float t16 = (sy * cz + sz0 * cy + sz * sy0) - t14;
    float t13 = (sz * cy + sy0 * cz + sz0 * sy) - t14;
    float t8 = ((cz * cy - t14) - sz0 * sy) - sz * sy0;
    out[0] = (t8 * sx + t15 * cx + t16 * sx0) - t13 * sx0;
    out[1] = (t16 * cx + t8 * sx0 + t13 * sx) - t15 * sx0;
    out[2] = (t13 * cx + t8 * sx0 + t15 * sx0) - t16 * sx;
    out[3] = ((t8 * cx - t15 * sx) - t16 * sx0) - t13 * sx0;
    return out;
}

// ---- STrackRotationEulerPacked (CEUP) ----

// PANZERS 0x671860
STrackRotationEulerPacked::STrackRotationEulerPacked(SStream* is)
{
    Count = is->ReadInt();
    Start = is->ReadFloat();
    End = is->ReadFloat();
    Rate = (float)(Count - 1) / (End - Start);
    OrtBefore = is->ReadByte();
    OrtAfter = is->ReadByte();
    Keys = (float*)operator new((size_t)Count * 12);
    memset(Keys, 0, (size_t)Count * 12);
    is->Read(Keys, Count * 12);
}

// PANZERS 0x672790
STrackRotationEulerPacked::~STrackRotationEulerPacked()
{
    operator delete(Keys);
    Keys = nullptr;
}

// PANZERS 0x675710
float* STrackRotationEulerPacked::Evaluate(float* out, float t)
{
    int n = Count;
    if (n < 2)
        return QuatFromEuler(out, Keys[0], Keys[1], Keys[2]);
    int which;
    if (!FoldTime(&t, Start, End, OrtBefore, OrtAfter, &which)) {
        const float* v = which ? Keys + n * 3 - 3 : Keys;
        return QuatFromEuler(out, v[0], v[1], v[2]);
    }
    int i = RoundToInt((t - Start) * Rate);   // FISTP, truncate
    if (i < n - 1) {
        if (i < 0)   // recompile guard (see the packed position track)
            i = 0;
        const float* a = Keys + i * 3;
        float f = (t - Start) * Rate - (float)i;
        return QuatFromEuler(out, (a[3] - a[0]) * f + a[0], (a[4] - a[1]) * f + a[1],
                             (a[5] - a[2]) * f + a[2]);
    }
    const float* v = Keys + n * 3 - 3;
    return QuatFromEuler(out, v[0], v[1], v[2]);
}

} // namespace pz
