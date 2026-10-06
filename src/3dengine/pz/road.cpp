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

// HD 0x6f95b0 STerrain::UpdateRoad (reimplemented from the decompile, 14.6 KB
// of HD code: same Hermite sampling, step, per-segment bounds, half-width
// vertex test, cell inclusion, U/V rules and the integral texture-repeat
// scale; the per-parcel dirty-range bookkeeping is simplified to a full
// rebuild and the along-road coordinate inside a sample interval is a
// projection, where HD intersects the two sample planes)
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
    const float half = r.Width * 0.5f;
    for (int i = 0; i + 1 < r.PointCount; ++i) {
        SRoadCtrl& a = r.Points[i];
        const SRoadCtrl& b = r.Points[i + 1];
        float len = sqrtf((b.Z - a.Z) * (b.Z - a.Z) + (b.X - a.X) * (b.X - a.X));
        int n = (int)lrintf(len / r.Step);
        if (n == 0)
            n = 1;
        a.FirstSample = (int)s.size();
        a.Length = arc;
        float mnx = (float)(ParcelsX * 8), mnz = (float)(ParcelsZ * 8), mxx = 0, mxz = 0;
        for (int k = 0; k <= n; ++k) {
            float t = (float)k / (float)n, t2 = t * t, t3 = t2 * t;
            float h00 = t3 * 2.0f - t2 * 3.0f + 1.0f, h01 = t2 * 3.0f - t3 * 2.0f;
            float h10 = t3 - t2 * 2.0f + t, h11 = t3 - t2;
            SSample o;
            o.X = a.X * h00 + b.X * h01 + a.DirX * len * h10 + b.DirX * len * h11;
            o.Z = a.Z * h00 + b.Z * h01 + a.DirZ * len * h10 + b.DirZ * len * h11;
            float d00 = t2 * 6.0f - t * 6.0f, d01 = t * 6.0f - t2 * 6.0f;
            float d10 = t2 * 3.0f - t * 4.0f + 1.0f, d11 = t2 * 3.0f - t * 2.0f;
            o.DX = a.X * d00 + b.X * d01 + a.DirX * len * d10 + b.DirX * len * d11;
            o.DZ = a.Z * d00 + b.Z * d01 + a.DirZ * len * d10 + b.DirZ * len * d11;
            float dl = sqrtf(o.DX * o.DX + o.DZ * o.DZ);
            if (dl > 0.0f) {
                o.DX /= dl;
                o.DZ /= dl;
            }
            if (!s.empty()) {
                const SSample& p = s.back();
                arc += sqrtf((o.X - p.X) * (o.X - p.X) + (o.Z - p.Z) * (o.Z - p.Z));
            }
            o.Arc = arc;
            s.push_back(o);
            if (o.X < mnx) mnx = o.X;
            if (mxx < o.X) mxx = o.X;
            if (o.Z < mnz) mnz = o.Z;
            if (mxz < o.Z) mxz = o.Z;
        }
        a.X0 = (int)(mnx - (half + 1.0f)); if (a.X0 < 0) a.X0 = 0;
        a.X1 = (int)(half + 2.0f + mxx);   if (ParcelsX * 8 < a.X1) a.X1 = ParcelsX * 8;
        a.Z0 = (int)(mnz - (half + 1.0f)); if (a.Z0 < 0) a.Z0 = 0;
        a.Z1 = (int)(half + 2.0f + mxz);   if (ParcelsZ * 8 < a.Z1) a.Z1 = ParcelsZ * 8;
        if (a.X0 < bx0) bx0 = a.X0;
        if (bx1 < a.X1) bx1 = a.X1;
        if (a.Z0 < bz0) bz0 = a.Z0;
        if (bz1 < a.Z1) bz1 = a.Z1;
    }
    r.Points[r.PointCount - 1].FirstSample = (int)s.size() - 1;
    r.Points[r.PointCount - 1].Length = arc;
    // Whole texture repeats along the road.
    float reps = arc / r.TexLength;
    float scale = reps > 0.0f ? (float)lrintf(reps) / reps : 1.0f;
    if (scale == 0.0f)
        scale = 1.0f;

    // 2. Per parcel in the road bounds: flag grid vertices within half the
    // width of the centre line, give every vertex its U/V.
    int ns = (int)s.size();
    for (int pz = bz0 / 8; pz < (bz1 + 7) / 8 && pz < ParcelsZ; ++pz)
        for (int px = bx0 / 8; px < (bx1 + 7) / 8 && px < ParcelsX; ++px) {
            SParcelGrid g;
            memset(&g, 0, sizeof(g));
            int seg0 = r.PointCount, seg1 = -1;
            for (int i = 0; i + 1 < r.PointCount; ++i) {
                const SRoadCtrl& a = r.Points[i];
                if (px * 8 <= a.X1 && a.X0 < px * 8 + 8 && pz * 8 <= a.Z1 && a.Z0 < pz * 8 + 8) {
                    if (i < seg0) seg0 = i;
                    if (seg1 < i) seg1 = i;
                }
            }
            if (seg1 < 0)
                continue;
            for (int v = 0; v < 81; ++v) {
                float x = (float)(px * 8 + v % 9), z = (float)(pz * 8 + v / 9);
                // Nearest sample interval whose slab contains the vertex.
                float best = 1e30f, across = 0.0f, along = 0.0f;
                bool found = false;
                int k0 = r.Points[seg0].FirstSample, k1 = r.Points[seg1 + 1].FirstSample;
                for (int k = k0; k < k1 && k + 1 < ns; ++k) {
                    const SSample& a = s[k];
                    const SSample& b = s[k + 1];
                    if ((x - a.X) * a.DX + (z - a.Z) * a.DZ <= -0.0001f)
                        continue;
                    if ((x - b.X) * b.DX + (z - b.Z) * b.DZ > 0.0f)
                        continue;
                    float sx = b.X - a.X, sz = b.Z - a.Z;
                    float sl = sqrtf(sx * sx + sz * sz);
                    if (sl <= 0.0f)
                        continue;
                    float nx = sz / sl, nz = -sx / sl;   // left normal
                    float d = (x - a.X) * nx + (z - a.Z) * nz;
                    if (fabsf(d) < best) {
                        best = fabsf(d);
                        float f = ((x - a.X) * sx + (z - a.Z) * sz) / (sl * sl);
                        if (f < 0.0f) f = 0.0f;
                        if (f > 1.0f) f = 1.0f;
                        across = d;
                        along = (a.Arc + f * (b.Arc - a.Arc)) * scale;
                        found = true;
                    }
                }
                if (!found)
                    continue;
                if (best < half)
                    g.Flag[v] |= 2;
                float uAlong = along / r.TexLength;
                float vAcross = across / r.Width + 0.5f;
                float u, w;
                if ((r.Flags & 0x10) == 0) {
                    u = uAlong;
                    w = vAcross;
                } else {
                    u = vAcross;
                    w = uAlong;
                }
                if (r.Flags & 0x20)
                    u = -u;
                if (r.Flags & 0x40)
                    w = 1.0f - w;
                g.U[v] = u;
                g.V[v] = w;
            }
            BuildParcelMesh(this, &r.Meshes, px, pz, g, seg0, seg1);
        }
    // Meshes no build touched belong to parcels the road left.
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
