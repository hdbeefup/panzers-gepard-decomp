// src/3dengine/pz/road.cpp
// Roads and road junctions on the terrain. OWNER: agent B.
//
// HD draws a road as a subset of the terrain grid: every terrain cell with
// a corner closer than half the road width to the road's centre line gets
// re-drawn with the road texture (alpha blended, the texture's own alpha
// gives the soft edges), U along the road and V across (flag 0x10 swaps).
// The centre line is a Hermite spline through the control points, sampled
// every Step tiles. Meshes are built per parcel (heap at road+0x24) by
// UpdateRoad and drawn by RenderRoads. Junctions are a textured, rotated
// rectangle laid over the grid the same way.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "pzterrain.h"
#include "parcel.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

// ---- road heap and points ----

// PANZERS 0x6f1ec0
void STerrain::FreeRoadMeshes(SHdHeap<SRoadMesh>* heap)
{
    for (int i = 0; i < heap->Size; ++i) {
        SRoadMesh& m = heap->Data[i];
        if (m.Use != kHeapLive)
            continue;
        free(m.Indices);
        m.Indices = nullptr;
        m.NIndices = m.ICap = 0;
        free(m.Verts);
        m.Verts = nullptr;
        m.NVerts = m.VCap = 0;
    }
    heap->Size = 0;
    heap->FreeHead = -1;
    heap->Count = 0;
}

// PANZERS 0x6f64c0
void STerrain::RemoveRoad(int road)
{
    SRoad& r = Roads.Data[road];
    FreeRoadMeshes(&r.Meshes);
    free(r.Meshes.Data);
    r.Meshes.Data = nullptr;
    r.Meshes.Capacity = 0;
    free(r.Points);
    r.Points = nullptr;
    r.PointCount = r.PointCap = 0;
    HdHeapRemove(&Roads, road);
}

// PANZERS 0x6f6560
void STerrain::RemoveJunction(int junction)
{
    SRoadJunction& j = Junctions.Data[junction];
    FreeRoadMeshes(&j.Meshes);
    free(j.Meshes.Data);
    j.Meshes.Data = nullptr;
    j.Meshes.Capacity = 0;
    HdHeapRemove(&Junctions, junction);
}

// PANZERS 0x6f2650
int STerrain::CreateRoad(int texture, const SRoadPointArray* points, float step, float width,
                         float texLength, unsigned flags)
{
    PZ_TRACE("STerrain::CreateRoad (0x6f2650)");
    if (RoadFormat < 0)
        RoadFormat = kTerrainFVF;
    int road = HdHeapAlloc(&Roads);
    Roads.Data[road].Meshes.FreeHead = -1;
    SetRoad(road, texture, points, step, width, texLength, flags);
    return road;
}

// PANZERS 0x6f5840
void STerrain::SetRoad(int road, int texture, const SRoadPointArray* points, float step,
                       float width, float texLength, unsigned flags)
{
    PZ_TRACE("STerrain::SetRoad (0x6f5840)");
    if (!HdHeapLive(&Roads, road))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "STerrain::SRoad", road);
    SRoad& r = Roads.Data[road];
    r.Texture = texture;
    r.Flags = flags | 0x80;
    r.Step = step;
    r.Width = width;
    r.TexLength = texLength;
    int n = points ? points->Count : 0;
    if (r.PointCap < n) {
        r.Points = (SRoadCtrl*)realloc(r.Points, n * sizeof(SRoadCtrl));
        r.PointCap = n;
    }
    r.PointCount = n;
    if (r.PointCap)
        memset(r.Points, 0, r.PointCap * sizeof(SRoadCtrl));
    for (int i = 0; i < n; ++i) {
        const SRoadPoint& s = points->Data[i];
        SRoadCtrl& d = r.Points[i];
        d.X = s.X;
        d.Z = s.Z;
        d.DirX = s.DirX;
        d.DirZ = s.DirZ;
        d.Dirty = s.Dirty;
        d.W = s.W;
        d.FirstSample = -1;
    }
}

// PANZERS 0x6f2a30
void STerrain::DestroyRoad(int road)
{
    PZ_TRACE("STerrain::DestroyRoad (0x6f2a30)");
    if (HdHeapLive(&Roads, road))
        RemoveRoad(road);
}

// PANZERS 0x6fcee0
void STerrain::UpdateRoadHeights(int road)
{
    PZ_TRACE("STerrain::UpdateRoadHeights (0x6fcee0)");
    if (!HdHeapLive(&Roads, road))
        return;
    SHdHeap<SRoadMesh>& h = Roads.Data[road].Meshes;
    for (int i = HdHeapNext(&h, -1); i >= 0; i = HdHeapNext(&h, i)) {
        SRoadMesh& m = h.Data[i];
        for (int k = 0; k < m.NVerts; ++k) {
            STerrainVertex& v = m.Verts[k];
            v.Y = HeightAt(v.X, v.Z);
            float n[3];
            NormalAt(n, (int)v.X, (int)v.Z);
            v.NX = n[0]; v.NY = n[1]; v.NZ = n[2];
        }
    }
}

// ---- mesh building shared by roads and junctions ----

namespace {

struct SSample {
    float X, Z;      // position
    float DX, DZ;    // unit tangent
    float Arc;       // arc length from the road start
};

// One parcel's worth of grid: 9x9 vertex flags and per-vertex UV.
struct SParcelGrid {
    unsigned char Flag[81];   // 2 = on the road, 1 = corner of an included cell
    float         U[81], V[81];
};

SRoadMesh* MeshForParcel(SHdHeap<SRoadMesh>* heap, int parcel)
{
    for (int i = HdHeapNext(heap, -1); i >= 0; i = HdHeapNext(heap, i))
        if (heap->Data[i].Parcel == parcel)
            return &heap->Data[i];
    int i = HdHeapAlloc(heap);
    heap->Data[i].Parcel = parcel;
    return &heap->Data[i];
}

// Builds the mesh of parcel (px, pz) from the flagged grid (cells with a
// corner flagged 2), 0x51 vertices / 0x180 indices at most.
void BuildParcelMesh(STerrain* t, SHdHeap<SRoadMesh>* heap, int px, int pz, SParcelGrid& g,
                     int seg0, int seg1)
{
    int nidx = 0, nv = 0;
    unsigned char inc[81];
    memset(inc, 0, sizeof(inc));
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            int v = r * 9 + c;
            if ((g.Flag[v] | g.Flag[v + 1] | g.Flag[v + 9] | g.Flag[v + 10]) & 2) {
                nidx += 6;
                inc[v] = inc[v + 1] = inc[v + 9] = inc[v + 10] = 1;
            }
        }
    int parcel = t->ParcelsX * pz + px;
    if (!nidx) {
        // Remove a mesh left from an earlier build.
        for (int i = HdHeapNext(heap, -1); i >= 0; i = HdHeapNext(heap, i))
            if (heap->Data[i].Parcel == parcel) {
                free(heap->Data[i].Verts);
                free(heap->Data[i].Indices);
                HdHeapRemove(heap, i);
            }
        return;
    }
    short remap[81];
    for (int v = 0; v < 81; ++v)
        remap[v] = inc[v] ? (short)nv++ : (short)-1;
    SRoadMesh* m = MeshForParcel(heap, parcel);
    m->Visible = 1;
    m->Seg0 = seg0;
    m->Seg1 = seg1;
    if (m->VCap < nv) {
        m->Verts = (STerrainVertex*)realloc(m->Verts, nv * sizeof(STerrainVertex));
        m->VCap = nv;
    }
    if (m->ICap < nidx) {
        m->Indices = (unsigned short*)realloc(m->Indices, nidx * sizeof(unsigned short));
        m->ICap = nidx;
    }
    m->NVerts = nv;
    m->NIndices = nidx;
    for (int v = 0; v < 81; ++v) {
        if (remap[v] < 0)
            continue;
        int x = px * 8 + v % 9, z = pz * 8 + v / 9;
        STerrainVertex& o = m->Verts[remap[v]];
        o.X = (float)x;
        o.Z = (float)z;
        bool in = x >= 0 && x <= t->Width && z >= 0 && z <= t->Height;
        o.Y = in ? t->Heights[t->Stride * z + x] : 0.0f;
        float n[3];
        t->NormalAt(n, x, z);
        o.NX = n[0]; o.NY = n[1]; o.NZ = n[2];
        o.Color = in ? t->Diffuse[t->Stride * z + x] : 0;
        o.U = g.U[v];
        o.V = g.V[v];
    }
    int k = 0;
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c) {
            int v = r * 9 + c;
            if (!((g.Flag[v] | g.Flag[v + 1] | g.Flag[v + 9] | g.Flag[v + 10]) & 2))
                continue;
            m->Indices[k++] = remap[v];
            m->Indices[k++] = remap[v + 1];
            m->Indices[k++] = remap[v + 9];
            m->Indices[k++] = remap[v + 9];
            m->Indices[k++] = remap[v + 1];
            m->Indices[k++] = remap[v + 10];
        }
}

} // namespace

namespace {

// A line a*x + b*z + c (HD keeps sample planes and chords this way).
struct SLine {
    float NX, NZ, C;
    float Eval(float x, float z) const { return NZ * z + NX * x + C; }
};

// The plane through (x0, z0) facing (dx, dz), normalised when its length > 0.
SLine PlaneAt(float x0, float z0, float dx, float dz)
{
    SLine l = { dx, dz, -(x0 * dx + z0 * dz) };
    float q = dz * dz + dx * dx;
    if (q > 0.0f) {
        float k = (float)(1.0 / sqrt((double)q));
        l.NX *= k; l.NZ *= k; l.C *= k;
    }
    return l;
}

// The line through (x0, z0) and (x1, z1), normal (z0 - z1, x1 - x0).
SLine ChordLine(float x0, float z0, float x1, float z1)
{
    SLine l;
    l.NX = z0 - z1;
    l.NZ = x1 - x0;
    l.C = -(x0 * l.NX + z0 * l.NZ);
    float q = l.NX * l.NX + l.NZ * l.NZ;
    if (q > 0.0f) {
        float k = (float)(1.0 / sqrt((double)q));
        l.NX *= k; l.NZ *= k; l.C *= k;
    }
    return l;
}

// Intersection of the lines a and b; false when they are parallel.
bool Cross(const SLine& a, const SLine& b, float& x, float& z)
{
    float det = a.NZ * b.NX - a.NX * b.NZ;
    if (det == 0.0f)
        return false;
    float inv = 1.0f / det;
    x = -((a.NZ * b.C - a.C * b.NZ) * inv);
    z = -((a.C * b.NX - a.NX * b.C) * inv);
    return true;
}

void RoadUV(unsigned flags, float along, float across, STerrainVertex& o, bool oneD)
{
    float u, w;
    if ((flags & 0x10) == 0) { u = along; w = across; }
    else { u = across; w = along; }
    if (flags & 0x20)
        u = u * -1.0f;
    if (flags & 0x40)
        w = oneD ? (float)(1.0 - (double)w) : 1.0f - w;
    o.U = u;
    o.V = w;
}

} // namespace

// PANZERS 0x6f95b0
// STerrain::UpdateRoad. Samples the Hermite centre line every Step tiles
// (both ends of every control segment, so a join repeats its sample), then
// per parcel in the road bounds: every grid vertex picks the control
// segment whose two end planes hold it and whose chord is nearest (closer
// than Width); a vertex closer than Width / 2 to the chord of one of that
// segment's sample intervals (between the interval's end planes) is on the
// road (flag 2); cells with such a corner are drawn. U/V of every drawn
// vertex: the vertex's line parallel to the nearest holding interval's
// chord meets the interval's two end planes, U is the position between
// them along the chord, V the chord distance / Width + 0.5; before the
// first / after the last sample the distance to the end plane continues U.
// HD skips parcels whose control segments are not Dirty and keeps their
// mesh; this rebuilds every parcel the road covers (the same meshes).
void STerrain::UpdateRoad(int road)
{
    PZ_TRACE("STerrain::UpdateRoad (0x6f95b0)");
    if (!HdHeapLive(&Roads, road))
        Logger.g->Panic("STerrain::UpdateRoad(): Invalid index specified.");
    SRoad& r = Roads.Data[road];
    if (r.PointCount < 2) {
        FreeRoadMeshes(&r.Meshes);
        r.Flags &= ~0x80u;
        return;
    }
    for (int i = HdHeapNext(&r.Meshes, -1); i >= 0; i = HdHeapNext(&r.Meshes, i))
        r.Meshes.Data[i].Visible = 0;

    // 1. Sample the Hermite centre line (segment length scales the tangents).
    std::vector<SSample> s;
    int bx0 = ParcelsX * 8, bz0 = ParcelsZ * 8, bx1 = 0, bz1 = 0;
    float arc = 0.0f;
    for (int i = 0; i + 1 < r.PointCount; ++i) {
        SRoadCtrl& a = r.Points[i];
        const SRoadCtrl& b = r.Points[i + 1];
        int mnx = ParcelsX * 8, mnz = ParcelsZ * 8, mxx = 0, mxz = 0;
        float len = (float)sqrt((double)((b.Z - a.Z) * (b.Z - a.Z) + (b.X - a.X) * (b.X - a.X)));
        int n = (int)lrintf(len / r.Step);
        if (n == 0)
            n = 1;
        for (int k = 0; k <= n; ++k) {
            float t = (float)k / (float)n, t2 = t * t, t3 = t2 * t;
            if (k == 0) {
                a.FirstSample = (int)s.size();
                a.Length = arc;
            }
            float h00 = (t3 * 2.0f - t2 * 3.0f) + 1.0f, h01 = t2 * 3.0f - t3 * 2.0f;
            float h10 = (t3 - t2 * 2.0f) + t, h11 = t3 - t2;
            SSample o;
            o.X = a.X * h00 + b.X * h01 + a.DirX * len * h10 + b.DirX * len * h11;
            o.Z = a.Z * h00 + b.Z * h01 + a.DirZ * len * h10 + b.DirZ * len * h11;
            float d00 = t2 * 6.0f - t * 6.0f, d01 = t * 6.0f - t2 * 6.0f;
            float d10 = (t2 * 3.0f - t * 4.0f) + 1.0f, d11 = t2 * 3.0f - t * 2.0f;
            o.DX = a.X * d00 + b.X * d01 + a.DirX * len * d10 + b.DirX * len * d11;
            o.DZ = a.Z * d00 + b.Z * d01 + a.DirZ * len * d10 + b.DirZ * len * d11;
            double inv = 1.0 / sqrt((double)(o.DX * o.DX + o.DZ * o.DZ));
            o.DX = (float)((double)o.DX * inv);
            o.DZ = (float)((double)o.DZ * inv);
            if (!s.empty()) {
                const SSample& p = s.back();
                arc += (float)sqrt((double)((o.X - p.X) * (o.X - p.X) + (o.Z - p.Z) * (o.Z - p.Z)));
            }
            o.Arc = arc;
            s.push_back(o);
            if (o.X < (float)mnx) mnx = (int)o.X;
            if ((float)mxx < o.X) mxx = (int)o.X;
            if (o.Z < (float)mnz) mnz = (int)o.Z;
            if ((float)mxz < o.Z) mxz = (int)o.Z;
        }
        a.X0 = (int)((float)mnx - (r.Width * 0.5f + 1.0f)); if (a.X0 < 0) a.X0 = 0;
        a.X1 = (int)(r.Width * 0.5f + 2.0f + (float)mxx);  if (ParcelsX * 8 < a.X1) a.X1 = ParcelsX * 8;
        a.Z0 = (int)((float)mnz - (r.Width * 0.5f + 1.0f)); if (a.Z0 < 0) a.Z0 = 0;
        a.Z1 = (int)(r.Width * 0.5f + 2.0f + (float)mxz);  if (ParcelsZ * 8 < a.Z1) a.Z1 = ParcelsZ * 8;
        if (a.X0 < bx0) bx0 = a.X0;
        if (bx1 < a.X1) bx1 = a.X1;
        if (a.Z0 < bz0) bz0 = a.Z0;
        if (bz1 < a.Z1) bz1 = a.Z1;
    }
    const int ns = (int)s.size();
    r.Points[r.PointCount - 1].FirstSample = ns - 1;
    r.Points[r.PointCount - 1].Length = arc;

    // Whole texture repeats along the road (HD rounds the length in widths).
    const float total = r.Points[r.PointCount - 1].Length;
    const float scale = (float)(int)lrintf(total / r.Width) / (total / r.Width);

    for (int px = bx0 / 8; px < (bx1 + 7) / 8; ++px)
        for (int pz = bz0 / 8; pz < (bz1 + 7) / 8; ++pz) {
            unsigned char flag[81];
            int remap[81], seg[81];
            memset(flag, 0, sizeof(flag));
            memset(remap, 0, sizeof(remap));
            memset(seg, -1, sizeof(seg));
            int segMin = r.PointCount + 1, segMax = -1;
            int nv = 0;
            // 2a. Nearest control segment of every vertex, then the on-road flag.
            for (int i = 0; i < 9; ++i) {
                const int x = px * 8 + i;
                const float fx = (float)x;
                for (int j = 0; j < 9; ++j) {
                    const int z = pz * 8 + j;
                    const float fz = (float)z;
                    const int v = i + j * 9;
                    if (!(bx0 <= x && x < bx1 && bz0 <= z && z < bz1))
                        continue;
                    float best = r.Width;
                    for (int c = 0; c + 1 < r.PointCount; ++c) {
                        const SRoadCtrl& a = r.Points[c];
                        const SRoadCtrl& b = r.Points[c + 1];
                        if (a.X1 < x || x + 8 <= a.X0 || a.Z1 < z || z + 8 <= a.Z0)
                            continue;
                        float sa = a.DirZ * fz + a.DirX * fx + -(a.Z * a.DirZ + a.X * a.DirX);
                        float sb = b.DirZ * fz + b.DirX * fx + -(b.Z * b.DirZ + b.X * b.DirX);
                        if (sa <= 0.0f || 0.0f < sb)
                            continue;
                        float d = fabsf(ChordLine(a.X, a.Z, b.X, b.Z).Eval(fx, fz));
                        if (best <= d)
                            continue;
                        seg[v] = c;
                        best = d;
                    }
                    const int c = seg[v];
                    if (c < 0)
                        continue;
                    if (c < segMin) segMin = c;
                    if (segMax < c) segMax = c;
                    for (int k = r.Points[c].FirstSample; k < r.Points[c + 1].FirstSample; ++k) {
                        const SSample& a = s[k];
                        const SSample& b = s[k + 1];
                        if (a.DZ * fz + a.DX * fx + -(a.Z * a.DZ + a.X * a.DX) <= 0.0f ||
                            0.0f < b.DZ * fz + b.DX * fx + -(b.X * b.DX + b.Z * b.DZ))
                            continue;
                        if ((double)fabsf(ChordLine(a.X, a.Z, b.X, b.Z).Eval(fx, fz)) <
                            (double)(r.Width * 0.5f)) {
                            flag[v] |= 2;
                            ++nv;
                            break;
                        }
                    }
                }
            }
            // 2b. Cells with an on-road corner; their other corners join.
            int nidx = 0;
            for (int i = 0; i < 8; ++i)
                for (int j = 0; j < 8; ++j) {
                    const int x = px * 8 + i, z = pz * 8 + j, v = i + j * 9;
                    if (!(bx0 <= x && x < bx1 - 1 && bz0 <= z && z < bz1 - 1))
                        continue;
                    if (!((flag[v] | flag[v + 1] | flag[v + 9] | flag[v + 10]) & 2))
                        continue;
                    nidx += 6;
                    const int corner[4] = { v, v + 1, v + 9, v + 10 };
                    for (int q = 0; q < 4; ++q)
                        if (flag[corner[q]] == 0) {
                            ++nv;
                            flag[corner[q]] = 1;
                        }
                }
            if (nv <= 0 || nidx <= 0)
                continue;
            if (nv > 0x51)
                Logger.g->Panic("STerrain::UpdateRoad(): too many vertices...");
            if (nidx > 0x180)
                Logger.g->Panic("STerrain::UpdateRoad(): too many indices...");
            SRoadMesh* m = MeshForParcel(&r.Meshes, ParcelsX * pz + px);
            m->Visible = 1;
            m->Seg0 = segMin;
            m->Seg1 = segMax;
            if (m->VCap < nv) {
                m->VCap = nv;
                m->Verts = (STerrainVertex*)realloc(m->Verts, nv * sizeof(STerrainVertex));
            }
            m->NVerts = nv;
            memset(m->Verts, 0, m->VCap * sizeof(STerrainVertex));
            if (m->ICap < nidx) {
                m->ICap = nidx;
                m->Indices = (unsigned short*)realloc(m->Indices, nidx * sizeof(unsigned short));
            }
            m->NIndices = nidx;
            memset(m->Indices, 0, m->ICap * sizeof(unsigned short));

            // 2c. Vertices (x outer, z inner) with their U/V.
            int out = 0;
            for (int i = 0; i < 9; ++i) {
                const int x = px * 8 + i;
                for (int j = 0; j < 9; ++j) {
                    const int z = pz * 8 + j;
                    const int v = i + j * 9;
                    if (!(bx0 <= x && x < bx1 && bz0 <= z && z < bz1) || flag[v] == 0)
                        continue;
                    remap[v] = out;
                    STerrainVertex& o = m->Verts[out++];
                    const float fx = (float)x, fz = (float)z;
                    const bool in = x >= 0 && x <= Width && z >= 0 && z <= Height;
                    o.X = fx;
                    o.Y = in ? Heights[Stride * z + x] : 0.0f;
                    o.Z = fz;
                    float nrm[3];
                    NormalAt(nrm, x, z);
                    o.NX = nrm[0]; o.NY = nrm[1]; o.NZ = nrm[2];
                    o.Color = in ? Diffuse[Stride * z + x] : 0;
                    o.U = 0.0f;
                    o.V = 0.0f;

                    float best = 3.4028235e+38f;
                    float along0 = 0.0f;
                    int k0 = 0, k1 = ns - 1;
                    if (seg[v] >= 0) {
                        along0 = r.Points[seg[v]].Length * scale;
                        k0 = r.Points[seg[v]].FirstSample;
                        k1 = r.Points[seg[v] + 1].FirstSample;
                    }
                    for (int k = k0; k < k1; ++k) {
                        const SSample& a = s[k];
                        const SSample& b = s[k + 1];
                        float clen = (float)sqrt((double)((a.X - b.X) * (a.X - b.X) +
                                                          (a.Z - b.Z) * (a.Z - b.Z)));
                        SLine pa = PlaneAt(a.X, a.Z, a.DX, a.DZ);
                        SLine pb = PlaneAt(b.X, b.Z, b.DX, b.DZ);
                        SLine ch = ChordLine(a.X, a.Z, b.X, b.Z);
                        float d = ch.Eval(fx, fz);
                        if (0.0f < pa.Eval(fx, fz) && pb.Eval(fx, fz) <= 0.0f &&
                            (double)fabsf(d) < (double)best) {
                            best = fabsf(d);
                            // The vertex's line parallel to the chord.
                            SLine par = { ch.NX, ch.NZ, -(ch.NZ * fz + ch.NX * fx) };
                            float p0x, p0z, p1x, p1z;
                            if (Cross(par, pa, p0x, p0z) && Cross(par, pb, p1x, p1z)) {
                                float f0 = (float)sqrt((double)((p0x - fx) * (p0x - fx) +
                                                                (p0z - fz) * (p0z - fz)));
                                float f1 = (float)sqrt((double)((p0x - p1x) * (p0x - p1x) +
                                                                (p0z - p1z) * (p0z - p1z)));
                                RoadUV(r.Flags, ((f0 / f1) * scale * clen + along0) / r.TexLength,
                                       d / r.Width + 0.5f, o, false);
                            } else {
                                Logger.g->Log(1, "STerrain::UpdateRoad(): Unable to generate texture coords.");
                            }
                        }
                        along0 = clen * scale + along0;
                    }
                    // Before the first sample: the distance to its plane.
                    {
                        SLine pa = PlaneAt(s[0].X, s[0].Z, s[0].DX, s[0].DZ);
                        float d = ChordLine(s[0].X, s[0].Z, s[1].X, s[1].Z).Eval(fx, fz);
                        if ((double)fabsf(d) < (double)best) {
                            float dist = (float)sqrt((double)((s[0].X - fx) * (s[0].X - fx) +
                                                              (s[0].Z - fz) * (s[0].Z - fz)));
                            float e = pa.Eval(fx, fz);
                            if (dist < best && e <= 0.0f) {
                                best = dist;
                                RoadUV(r.Flags, e / r.TexLength, d / r.Width + 0.5f, o, false);
                            }
                        }
                    }
                    // Past the last sample: the end length plus the distance to its plane.
                    {
                        const SSample& a = s[ns - 2];
                        const SSample& b = s[ns - 1];
                        SLine pb = PlaneAt(b.X, b.Z, b.DX, b.DZ);
                        float d = ChordLine(a.X, a.Z, b.X, b.Z).Eval(fx, fz);
                        if ((double)fabsf(d) < (double)best) {
                            float dist = (float)sqrt((double)((b.X - fx) * (b.X - fx) +
                                                              (b.Z - fz) * (b.Z - fz)));
                            float e = pb.Eval(fx, fz);
                            if (dist < best && 0.0f < e)
                                RoadUV(r.Flags, (along0 + e) / r.TexLength, d / r.Width + 0.5f, o,
                                       true);
                        }
                    }
                }
            }
            // 2d. Two triangles per drawn cell (x outer, z inner).
            int k = 0;
            for (int i = 0; i < 8; ++i)
                for (int j = 0; j < 8; ++j) {
                    const int x = px * 8 + i, z = pz * 8 + j, v = i + j * 9;
                    if (!(bx0 <= x && x < bx1 - 1 && bz0 <= z && z < bz1 - 1))
                        continue;
                    if (!((flag[v] | flag[v + 1] | flag[v + 9] | flag[v + 10]) & 2))
                        continue;
                    m->Indices[k++] = (unsigned short)remap[v];
                    m->Indices[k++] = (unsigned short)remap[v + 1];
                    m->Indices[k++] = (unsigned short)remap[v + 9];
                    m->Indices[k++] = (unsigned short)remap[v + 9];
                    m->Indices[k++] = (unsigned short)remap[v + 1];
                    m->Indices[k++] = (unsigned short)remap[v + 10];
                }
        }
    // Meshes no build touched belong to parcels the road left (HD 0x6f65d0).
    for (int i = HdHeapNext(&r.Meshes, -1); i >= 0; i = HdHeapNext(&r.Meshes, i))
        if (!r.Meshes.Data[i].Visible) {
            free(r.Meshes.Data[i].Verts);
            free(r.Meshes.Data[i].Indices);
            HdHeapRemove(&r.Meshes, i);
        }
    for (int i = 0; i < r.PointCount; ++i)
        r.Points[i].Dirty = 0;
    r.Flags &= ~0x80u;
}

// ---- junctions ----

// PANZERS 0x6f2710
int STerrain::CreateRoadJunction(int texture, const SRoadJunctionPoint* point, float halfX,
                                 float halfZ, unsigned flags)
{
    PZ_TRACE("STerrain::CreateRoadJunction (0x6f2710)");
    if (RoadFormat < 0)
        RoadFormat = kTerrainFVF;
    int j = HdHeapAlloc(&Junctions);
    Junctions.Data[j].Meshes.FreeHead = -1;
    SetRoadJunction(j, texture, point, halfX, halfZ, flags);
    return j;
}

// PANZERS 0x6f5b80
void STerrain::SetRoadJunction(int junction, int texture, const SRoadJunctionPoint* point,
                               float halfX, float halfZ, unsigned flags)
{
    PZ_TRACE("STerrain::SetRoadJunction (0x6f5b80)");
    if (!HdHeapLive(&Junctions, junction))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "STerrain::SRoadJunction", junction);
    SRoadJunction& j = Junctions.Data[junction];
    j.Texture = texture;
    j.Flags = flags | 0x80;
    j.HalfX = halfX;
    j.HalfZ = halfZ;
    j.X = point->X;
    j.Z = point->Z;
    j.DirX = point->DirX;
    j.DirZ = point->DirZ;
    j.Valid = point->Valid;
}

// PANZERS 0x6f2a70
void STerrain::DestroyRoadJunction(int junction)
{
    PZ_TRACE("STerrain::DestroyRoadJunction (0x6f2a70)");
    if (HdHeapLive(&Junctions, junction))
        RemoveJunction(junction);
}

// HD 0x6fd380 UpdateRoadJunction (reimplemented: bounds = centre +-
// 1.5 * max half size as in HD, then the junction rectangle in its own
// frame (V along the direction, U across, both clamped to 0..1); cells with
// a corner inside the rectangle are drawn. Orientation of the texture in
// that frame not verified against HD.)
void STerrain::UpdateRoadJunction(int junction)
{
    PZ_TRACE("STerrain::UpdateRoadJunction (0x6fd380)");
    if (!HdHeapLive(&Junctions, junction)) {
        Logger.g->Log(0, "UpdateRoadJunction(): Invalid index (%d).", junction);
        return;
    }
    SRoadJunction& j = Junctions.Data[junction];
    float ext = (j.HalfX < j.HalfZ ? j.HalfZ : j.HalfX) * 3.0f * 0.5f;
    int x0 = (int)(j.X - ext), x1 = (int)(ext + j.X);
    int z0 = (int)(j.Z - ext), z1 = (int)(ext + j.Z);
    if (x0 < 0) x0 = 0;
    if (z0 < 0) z0 = 0;
    if (Width + 1 <= x1) x1 = Width + 1;
    if (Height + 1 <= z1) z1 = Height + 1;
    FreeRoadMeshes(&j.Meshes);
    float dl = sqrtf(j.DirX * j.DirX + j.DirZ * j.DirZ);
    float dx = dl > 0 ? j.DirX / dl : 0.0f, dz = dl > 0 ? j.DirZ / dl : 1.0f;
    float rx = dz, rz = -dx;   // right of the direction
    for (int pz = z0 / 8; pz < (z1 + 6) / 8 && pz < ParcelsZ; ++pz)
        for (int px = x0 / 8; px < (x1 + 6) / 8 && px < ParcelsX; ++px) {
            SParcelGrid g;
            memset(&g, 0, sizeof(g));
            for (int v = 0; v < 81; ++v) {
                float x = (float)(px * 8 + v % 9) - j.X, z = (float)(pz * 8 + v / 9) - j.Z;
                float u = (x * rx + z * rz) / (2.0f * j.HalfZ) + 0.5f;
                float w = (x * dx + z * dz) / (2.0f * j.HalfX) + 0.5f;
                if ((j.Flags & 0x20) != 0)
                    u = 1.0f - u;
                if (u >= 0.0f && u <= 1.0f && w >= 0.0f && w <= 1.0f)
                    g.Flag[v] = 2;
                g.U[v] = u;
                g.V[v] = w;
            }
            BuildParcelMesh(this, &j.Meshes, px, pz, g, 0, 0);
        }
    j.Flags &= ~0x80u;
}

// ---- drawing ----

static void DrawRoadMeshes(STerrain* t, IDirect3DDevice9* dev, SHdHeap<SRoadMesh>* heap)
{
    for (int i = HdHeapNext(heap, -1); i >= 0; i = HdHeapNext(heap, i)) {
        const SRoadMesh& m = heap->Data[i];
        if (m.Parcel < 0 || m.Parcel >= t->ParcelCount || !t->Parcels[m.Parcel].Visible)
            continue;
        if (!m.NVerts)
            continue;
        TerrainDraw(dev, kTerrainFVF, m.Verts, m.NVerts, sizeof(STerrainVertex), m.Indices,
                    m.NIndices);
    }
}

// PANZERS 0x6f7810
void STerrain::RenderRoads(SViewport* vp, bool shadowPass)
{
    PZ_TRACE("STerrain::RenderRoads (0x6f7810)");
    (void)vp;
    IDirect3DDevice9* dev = TerrainDevice();
    if (!dev)
        return;
    STerrainDrawState st;
    // Junctions first (clamped texture), then the roads.
    for (int i = HdHeapNext(&Junctions, -1); i >= 0; i = HdHeapNext(&Junctions, i)) {
        if (Junctions.Data[i].Flags & 0x80)
            UpdateRoadJunction(i);
        SRoadJunction& j = Junctions.Data[i];
        if ((j.Flags & 2) == 0)
            continue;
        st.Reset();
        if (!shadowPass && TerrainOption(0x10) != 0)
            st.SetColorOp(0, D3DTOP_MODULATE2X, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
        st.SetTexture(0, j.Texture, false, false);
        st.SetBlendMode(2);
        if (TerrainOption(2) != 0)
            st.Lighting = false;   // 0x6f797d / 0x6f804d: lit by the shadow pass (0x6f46e0)
        st.Apply(dev);
        TerrainSetWorldIdentity(dev);
        DrawRoadMeshes(this, dev, &j.Meshes);
    }
    for (int i = HdHeapNext(&Roads, -1); i >= 0; i = HdHeapNext(&Roads, i)) {
        if (Roads.Data[i].Flags & 0x80)
            UpdateRoad(i);
        SRoad& r = Roads.Data[i];
        if ((r.Flags & 2) == 0 || r.PointCount < 2)
            continue;
        st.Reset();
        if (!shadowPass && TerrainOption(0x10) != 0)
            st.SetColorOp(0, D3DTOP_MODULATE2X, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
        bool swap = ((r.Flags >> 4) & 1) != 0;
        st.SetTexture(0, r.Texture, !swap, swap);
        st.SetBlendMode(2);
        if (TerrainOption(2) != 0)
            st.Lighting = false;   // 0x6f797d / 0x6f804d: lit by the shadow pass (0x6f46e0)
        st.Apply(dev);
        TerrainSetWorldIdentity(dev);
        DrawRoadMeshes(this, dev, &r.Meshes);
    }
}

} // namespace pz
