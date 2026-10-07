// src/game/rail.cpp
// Track queries of the trains on the ROD2 roads, and the block-map footprint
// helpers of a model "Block" bitmap (agent TR). See rail.h.

#include <math.h>
#include "rail.h"
#include "world.h"
#include "worldapi.h"
#include "doodad.h"
#include "blockmaprefresh.h"
#include "logger.h"

namespace pz {

static const float kRailSnap = 8.0f;                              // 0x7fa408

static SMapRoad& RoadAt(int road)                                 // SHeap<SRoad>::operator[] 0x5d6650
{
    if (!g_World->Roads.IsLive(road))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "struct SRoad", road);
    return g_World->Roads.Array[road].Data;
}

static SRoadControlPoint& PointAt(SMapRoad& r, int i)             // SDArray<SRoadControlPoint>::operator[] 0x5d64a0
{
    if (i < 0 || i >= r.PointCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SRoadControlPoint", i);
    return r.Points[i];
}

static float Dist3(const SRoadControlPoint& p, const float* pos)
{
    double d = (double)((p.X - pos[0]) * (p.X - pos[0]) + (p.Y - pos[1]) * (p.Y - pos[1]) +
                        (p.Z - pos[2]) * (p.Z - pos[2]));
    return (float)sqrt(d);
}

// pos lies ahead of the control point along its tangent (> 0) or behind (< 0).
static float Side(const SRoadControlPoint& p, const float* pos)
{
    return pos[2] * p.DirZ + pos[0] * p.DirX + -(p.Z * p.DirZ + p.X * p.DirX);
}

// The candidates of one road: index 0 = before the first point, i + 1 = the
// segment i .. i + 1, PointCount = past the last point.
static void ScanRoad(SMapRoad& r, const float* pos, float* best, int* bestIdx, bool* hit)
{
    int n = r.PointCount;
    if (n == 1) {
        float d = Dist3(r.Points[0], pos);
        if (d < *best) {
            *best = d;
            *bestIdx = 0;
            *hit = true;
        }
        return;
    }
    if (Side(PointAt(r, 0), pos) < 0.0f) {
        float d = Dist3(PointAt(r, 0), pos);
        if (d < *best) {
            *best = d;
            *bestIdx = 0;
            *hit = true;
        }
    }
    for (int i = 0; i < n - 1; ++i) {
        SRoadControlPoint& a = PointAt(r, i);
        SRoadControlPoint& b = PointAt(r, i + 1);
        if (0.0f < Side(a, pos) && Side(b, pos) < 0.0f) {
            float seg[4] = { a.X, a.Z, b.X, b.Z };
            float pt[2] = { pos[0], pos[2] };
            float d = RailDistToSegment(seg, pt);                 // 0x5e4e90
            if (d < *best) {
                *best = d;
                *bestIdx = i + 1;
                *hit = true;
            }
        }
    }
    SRoadControlPoint& last = PointAt(r, n - 1);
    if (0.0f < Side(last, pos)) {
        float d = Dist3(last, pos);
        if (d < *best) {
            *best = d;
            *bestIdx = n;
            *hit = true;
        }
    }
}

// The path length at candidate idx: the foot of the perpendicular from pos
// on the chord, as the ratio |A - foot| / (|A - pos| + |B - pos|) clamped to
// -1 .. 1, between the two points' lengths. False on a degenerate chord.
static bool CandidateDist(SMapRoad& r, int idx, const float* pos, float* outDist)
{
    if (idx == 0) {
        *outDist = 0.0f;
        return true;
    }
    if (idx == r.PointCount) {
        *outDist = PointAt(r, idx - 1).Length;
        return true;
    }
    SRoadControlPoint& A = PointAt(r, idx - 1);
    SRoadControlPoint& B = PointAt(r, idx);
    float ax = A.X, az = A.Z;
    float nz = az - B.Z;                                          // line normal (az - bz, bx - ax)
    float dxa = ax - B.X;
    float nx = B.X - ax;
    float c1 = -(az * nx + ax * nz);
    float c2 = -(dxa * pos[0] + nz * pos[2]);
    float det = nz * nz - dxa * nx;
    if (det == 0.0f)
        return false;
    float inv = 1.0f / det;
    float fx = ax - (-((nz * c1 - c2 * nx) * inv));
    float fz = az - (-((c2 * nz - dxa * c1) * inv));
    float bx = B.X, bz = B.Z;
    float d1 = (float)sqrt((double)(fx * fx + fz * fz));
    float d2 = (float)sqrt((double)((ax - pos[0]) * (ax - pos[0]) + (az - pos[2]) * (az - pos[2])));
    float d3 = (float)sqrt((double)((bx - pos[0]) * (bx - pos[0]) + (bz - pos[2]) * (bz - pos[2])));
    float t = d1 / (d2 + d3);
    if (t <= -1.0f)                                               // 0x7f5a98, 0x7f1b58
        t = -1.0f;
    else if (1.0f <= t)
        t = 1.0f;
    *outDist = A.Length + (B.Length - A.Length) * t;
    return true;
}

// PANZERS 0x5e7ae0
bool RailProjectOnRoad(const float* pos, int road, float* outDist)
{
    SMapRoad& r = RoadAt(road);
    if (r.PointCount <= 0)
        return false;
    float best = kRailSnap;
    int idx = -1;
    bool hit = false;
    ScanRoad(r, pos, &best, &idx, &hit);
    if (!(best < kRailSnap) || idx < 0)
        return false;
    return CandidateDist(r, idx, pos, outDist);
}

// PANZERS 0x5e8750
int RailFindNearestRoad(const float* pos, float* outDist)
{
    float best = kRailSnap;
    int bestRoad = -1, bestIdx = -1;
    SHeap<SMapRoad>& h = g_World->Roads;
    for (int i = 0; i < h.Size; ++i) {
        if (h.Array[i].Next != kHeapLive)
            continue;
        SMapRoad& r = h.Array[i].Data;
        if (r.PointCount < 1)
            continue;
        bool hit = false;
        ScanRoad(r, pos, &best, &bestIdx, &hit);
        if (hit)
            bestRoad = i;
    }
    if (bestRoad < 0)
        return bestRoad;
    CandidateDist(RoadAt(bestRoad), bestIdx, pos, outDist);       // a degenerate chord leaves *outDist
    return bestRoad;
}

// PANZERS 0x5e98e0
void RailGetPositionOnRoad(int road, float* dist, float* outPos2, float* outDir2)
{
    if (!g_World->Roads.IsLive(road))
        Logger.g->Panic("SWorld::GetPositionOnRoad(): Invalid index specified.");
    SMapRoad& r = g_World->Roads.Array[road].Data;
    if (r.PointCount < 2) {
        Logger.g->Warning("SWorld::GetPositionOnRoad(): Road is not tesselated.");
        return;
    }
    if (!(0.0f < *dist)) {
        SRoadControlPoint& p = PointAt(r, 0);
        outPos2[0] = p.X;
        outPos2[1] = p.Z;
        outDir2[0] = p.DirX;
        outDir2[1] = p.DirZ;
        *dist = 0.0f;
        return;
    }
    SRoadControlPoint& last = PointAt(r, r.PointCount - 1);
    if (last.Length <= *dist) {
        outPos2[0] = last.X;
        outPos2[1] = last.Z;
        outDir2[0] = last.DirX;
        outDir2[1] = last.DirZ;
        *dist = last.Length;
        return;
    }
    for (int i = 0; i < r.PointCount - 1; ++i) {
        SRoadControlPoint& a = PointAt(r, i);
        if (!(a.Length <= *dist))
            continue;
        SRoadControlPoint& b = PointAt(r, i + 1);
        if (!(*dist <= b.Length))
            continue;
        float dx = a.X - b.X, dz = a.Z - b.Z;
        float len = (float)sqrt((double)(dx * dx + dz * dz));
        float t = (*dist - a.Length) / (b.Length - a.Length);
        float o[2];
        RailHermite(o, a.X, a.Z, a.DirX * len, a.DirZ * len, b.X, b.Z, b.DirX * len, b.DirZ * len, t);
        outPos2[0] = o[0];
        outPos2[1] = o[1];
        RailHermiteTangent(o, a.X, a.Z, a.DirX * len, a.DirZ * len, b.X, b.Z, b.DirX * len, b.DirZ * len, t);
        outDir2[0] = o[0];
        outDir2[1] = o[1];
        return;
    }
}

// PANZERS 0x5f54f0
float* RailHermite(float* out, float p0x, float p0z, float m0x, float m0z,
                   float p1x, float p1z, float m1x, float m1z, float t)
{
    float t2 = t * t;
    float t3 = t2 * t;
    float h01 = t2 * 3.0f - t3 * 2.0f;
    float h10 = (t3 - t2 * 2.0f) + t;
    float h00 = (t3 * 2.0f - t2 * 3.0f) + 1.0f;
    out[0] = p0x * h00 + p1x * h01 + m0x * h10 + m1x * (t3 - t2);
    out[1] = p0z * h00 + p1z * h01 + m0z * h10 + m1z * (t3 - t2);
    return out;
}

// PANZERS 0x5f55d0
float* RailHermiteTangent(float* out, float p0x, float p0z, float m0x, float m0z,
                          float p1x, float p1z, float m1x, float m1z, float t)
{
    float a = t * t * 3.0f;
    float b = t * t * 6.0f;
    float h11 = a - t * 2.0f;
    float h01 = t * 6.0f - b;
    float h00 = b - t * 6.0f;
    float h10 = (a - t * 4.0f) + 1.0f;
    out[0] = p0x * h00 + p1x * h01 + m0x * h10 + m1x * h11;
    out[1] = p0z * h00 + p1z * h01 + m0z * h10 + m1z * h11;
    return out;
}

// PANZERS 0x5e4e90
float RailDistToSegment(const float* s, const float* p)
{
    float ax = s[0], az = s[1];
    float nx = s[2] - ax, nz = s[3] - az;
    float c = -(az * nz + ax * nx);
    float l = nz * nz + nx * nx;
    if (0.0f < l) {
        float inv = (float)(1.0 / sqrt((double)l));
        nz *= inv;
        nx *= inv;
        c *= inv;
    }
    float px = p[0], pz = p[1];
    if (px * nx + pz * nz + c < 0.0f)
        return (float)sqrt((double)((ax - px) * (ax - px) + (az - pz) * (az - pz)));
    nz = az - s[3];
    nx = ax - s[2];
    c = -(s[3] * nz + s[2] * nx);
    l = nz * nz + nx * nx;
    if (0.0f < l) {
        float inv = (float)(1.0 / sqrt((double)l));
        nz *= inv;
        nx *= inv;
        c *= inv;
    }
    if (px * nx + pz * nz + c < 0.0f)
        return (float)sqrt((double)((s[2] - px) * (s[2] - px) + (s[3] - pz) * (s[3] - pz)));
    float qx = az - s[3];
    float qz = s[2] - ax;
    l = qz * qz + qx * qx;
    c = -(az * qz + ax * qx);
    if (0.0f < l) {
        float inv = (float)(1.0 / sqrt((double)l));
        c *= inv;
        qx *= inv;
        qz *= inv;
    }
    return (float)fabs((double)(px * qx + pz * qz + c));
}

// ---------------------------------------------------------------------------
// Block map

static bool BitAt(const SBlockBitmap* bm, int x, int z)
{
    int cx = x - bm->X;
    return ((bm->Bits[(z - bm->Z) * bm->Stride + (cx >> 3)] >> (cx & 7)) & 1) != 0;
}

// PANZERS 0x5f45f0
void BlockMap_OccupyBitmap(const SBlockBitmap* bm, bool on)
{
    if (!bm)
        return;
    SWorld* w = g_World;
    int x0 = bm->X > 0 ? bm->X : 0;
    int x1 = bm->W + bm->X < w->BlockW ? bm->W + bm->X : w->BlockW;
    int z0 = bm->Z > 0 ? bm->Z : 0;
    int z1 = bm->H + bm->Z < w->BlockH ? bm->H + bm->Z : w->BlockH;
    unsigned char* occ = (unsigned char*)w->BlockMap2;
    for (int z = z0; z < z1; ++z)
        for (int x = x0; x < x1; ++x)
            if (BitAt(bm, x, z)) {
                if (on)
                    ++occ[w->BlockW * z + x];
                else
                    --occ[w->BlockW * z + x];
            }
}

// PANZERS 0x5d9cf0
bool BlockMap_TestOccupied(const SBlockBitmap* bm)
{
    if (!bm)
        return false;
    SWorld* w = g_World;
    int x0 = bm->X > 0 ? bm->X : 0;
    int x1 = bm->W + bm->X < w->BlockW ? bm->W + bm->X : w->BlockW;
    int z0 = bm->Z > 0 ? bm->Z : 0;
    int z1 = bm->H + bm->Z < w->BlockH ? bm->H + bm->Z : w->BlockH;
    const unsigned char* occ = (const unsigned char*)w->BlockMap2;
    for (int z = z0; z < z1; ++z)
        for (int x = x0; x < x1; ++x)
            if (BitAt(bm, x, z) && occ[w->BlockW * z + x] != 0)
                return true;
    return false;
}

// PANZERS 0x5dc6b0
bool BlockMap_TestBitmap(const SBlockBitmap* bm, unsigned mask)
{
    if (!bm)
        return false;
    SWorld* w = g_World;
    int x0 = bm->X > 0 ? bm->X : 0;
    int x1 = bm->W + bm->X < w->BlockW ? bm->W + bm->X : w->BlockW;
    int z0 = bm->Z > 0 ? bm->Z : 0;
    int z1 = bm->H + bm->Z < w->BlockH ? bm->H + bm->Z : w->BlockH;
    for (int z = z0; z < z1; ++z)
        for (int x = x0; x < x1; ++x)
            if (BitAt(bm, x, z) && (w->BlockMap[w->BlockW * z + x] & mask) != 0)
                return true;
    return false;
}

// HD 0x661b30 + operator delete(0x1c) (as SDoodad's in mapload.cpp).
void BlockBitmap_Free(SBlockBitmap* bm)
{
    if (!bm)
        return;
    delete[] bm->Bits;
    bm->Bits = nullptr;
    delete bm;
}

} // namespace pz
