// src/3dengine/pz/decal.cpp
// Terrain decals. OWNER: agent B.
//   Map decals (DECS chunk, world 0x5edfd0 -> AddDecal/SetDecalType): an
//   SDArray at STerrain+0x11c70; each owns a terrain-conforming mesh
//   (FVF 0x112) covering the texture at 64 texels per tile, drawn by
//   STerrain::Render after the roads.
//   Effect decals (SDecalEffect, agent C): an SHeap at STerrain+0x04,
//   rebuilt every frame by 0x6f6640 (pass 0 with the terrain, pass 1 late).

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "pzterrain.h"
#include "parcel.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

struct SDecalMesh {
    SDecalVertex*   Verts;
    int             NVerts;
    unsigned short* Indices;
    int             NIndices;
};

static void FreeDecalMesh(SDecalMesh* m)
{
    if (!m)
        return;
    delete[] m->Verts;
    delete[] m->Indices;
    delete m;
}

// PANZERS 0x6ce6a0
void DrawDecalMesh(IDirect3DDevice9* dev, const SDecalMesh* mesh)
{
    // SMesh::Draw: stream + indices, DrawIndexedPrimitive(TRIANGLELIST).
    if (!mesh)
        return;
    TerrainDraw(dev, kDecalFVF, mesh->Verts, mesh->NVerts, sizeof(SDecalVertex), mesh->Indices,
                mesh->NIndices);
}

// ---- map decals ----

// PANZERS 0x6f2420
void STerrain::AddDecal(int index, int texture, int x, int z, int rotation)
{
    PZ_TRACE("STerrain::AddDecal (0x6f2420)");
    Update();
    if (index < 0 || index > MapDecalCount)
        Logger.g->Panic("SDArray<%s>::Insert: invalid index (%d)", "STerrain::SDecal", index);
    if (MapDecalCount == MapDecalCap) {
        int cap = MapDecalCap < 0x10 ? 0x10 : (MapDecalCap * 6) / 5;
        MapDecals = (SMapDecal*)realloc(MapDecals, cap * sizeof(SMapDecal));
        MapDecalCap = cap;
    }
    if (index < MapDecalCount)
        memmove(MapDecals + index + 1, MapDecals + index, (MapDecalCount - index) * sizeof(SMapDecal));
    memset(&MapDecals[index], 0, sizeof(SMapDecal));
    ++MapDecalCount;
    SMapDecal& d = MapDecals[index];
    d.Texture = TerrainAddRefTexture(texture);
    d.X = x;
    d.Z = z;
    d.Rotation = rotation;
    d.Type = 0;
    d.Mesh = nullptr;
    UpdateDecal(index);
}

// PANZERS 0x6f2910
void STerrain::RemoveDecal(int index)
{
    PZ_TRACE("STerrain::RemoveDecal (0x6f2910)");
    if (index < 0 || index >= MapDecalCount)
        return;
    TerrainReleaseTexture(MapDecals[index].Texture);
    FreeDecalMesh(MapDecals[index].Mesh);
    --MapDecalCount;
    if (MapDecalCount - index)
        memmove(MapDecals + index, MapDecals + index + 1, (MapDecalCount - index) * sizeof(SMapDecal));
    memset(&MapDecals[MapDecalCount], 0, sizeof(SMapDecal));
}

// PANZERS 0x6f8bd0
bool STerrain::IsInDecal(int index, float x, float z)
{
    PZ_TRACE("STerrain::IsInDecal (0x6f8bd0)");
    if (index < 0 || index >= MapDecalCount)
        return false;
    const SMapDecal& d = MapDecals[index];
    return (float)d.X0 <= x && x <= (float)d.X1 && (float)d.Z0 <= z && z <= (float)d.Z1;
}

// PANZERS 0x6f88d0
void STerrain::SetDecalType(int index, int type)
{
    PZ_TRACE("STerrain::SetDecalType (0x6f88d0)");
    if (index > -1 && index < MapDecalCount)
        MapDecals[index].Type = type;
}

// PANZERS 0x6f8840
void STerrain::MoveDecal(int index, int x, int z, int rotation)
{
    PZ_TRACE("STerrain::MoveDecal (0x6f8840)");
    if (index > -1 && index < MapDecalCount) {
        MapDecals[index].X = x;
        MapDecals[index].Z = z;
        MapDecals[index].Rotation = rotation;
        UpdateDecal(index);
    }
}

// PANZERS 0x6f9580
void STerrain::UpdateDecals()
{
    PZ_TRACE("STerrain::UpdateDecals (0x6f9580)");
    for (int i = 0; i < MapDecalCount; ++i)
        UpdateDecal(i);
}

// PANZERS 0x6f5cf0
void STerrain::UpdateDecal(int index)
{
    if (index < 0 || index >= MapDecalCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "STerrain::SDecal", index);
    SMapDecal& d = MapDecals[index];
    FreeDecalMesh(d.Mesh);
    d.Mesh = nullptr;

    // Size in tiles: 64 texels per tile; odd rotations swap the axes.
    int tw = 0, th = 0;
    if ((d.Rotation & 1) == 0)
        TerrainTextureSize(d.Texture, &tw, &th);
    else
        TerrainTextureSize(d.Texture, &th, &tw);
    int w = tw / 64, h = th / 64;
    d.X0 = d.X - w / 2;
    d.X1 = d.X0 + w;
    d.Z0 = d.Z - h / 2;
    d.Z1 = d.Z0 + h;

    // u = u0 + duX*i + duZ*j, v = v0 + dvX*i + dvZ*j (i, j from X0, Z0).
    float u0 = 0, v0 = 0, duX = 0, duZ = 0, dvX = 0, dvZ = 0;
    float iw = (float)(1.0 / (double)w), ih = (float)(1.0 / (double)h);
    switch (d.Rotation) {
    case 0: v0 = 1; u0 = 0; duX = iw; dvZ = -ih; break;
    case 1: u0 = 0; v0 = 0; dvX = iw; duZ = ih; break;
    case 2: v0 = 0; u0 = 1; duX = -iw; dvZ = ih; break;
    case 3: u0 = 1; v0 = 1; dvX = -iw; duZ = -ih; break;
    case 4: u0 = 1; v0 = 1; duX = -iw; dvZ = -ih; break;
    case 5: v0 = 0; u0 = 1; dvX = iw; duZ = -ih; break;
    case 6: u0 = 0; v0 = 0; duX = iw; dvZ = ih; break;
    case 7: v0 = 1; u0 = 0; dvX = -iw; duZ = ih; break;
    default: break;
    }

    SDecalMesh* m = new SDecalMesh;
    m->NVerts = (w + 1) * (h + 1);
    m->Verts = new SDecalVertex[m->NVerts > 0 ? m->NVerts : 1];
    int n = 0;
    for (int z = d.Z0, j = 0; z <= d.Z1; ++z, ++j) {
        for (int x = d.X0, i = 0; x <= d.X1; ++x, ++i, ++n) {
            SDecalVertex& v = m->Verts[n];
            v.X = (float)x;
            v.Z = (float)z;
            bool in = x >= 0 && x <= Width && z >= 0 && z <= Height;
            v.Y = in ? Heights[Stride * z + x] : 0.0f;
            if (in) {
                const float* nn = Normals + (Stride * z + x) * 3;
                v.NX = nn[0]; v.NY = nn[1]; v.NZ = nn[2];
            } else {
                v.NX = 0; v.NY = 1; v.NZ = 0;
            }
            v.U = u0 + duX * i + duZ * j;
            v.V = v0 + dvX * i + dvZ * j;
        }
    }
    m->NIndices = w * h * 6;
    m->Indices = new unsigned short[m->NIndices > 0 ? m->NIndices : 1];
    int k = 0;
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ++i) {
            unsigned short a = (unsigned short)((w + 1) * j + i);
            unsigned short b = (unsigned short)(w + 1 + a);
            m->Indices[k++] = a;
            m->Indices[k++] = (unsigned short)(a + 1);
            m->Indices[k++] = b;
            m->Indices[k++] = b;
            m->Indices[k++] = (unsigned short)(a + 1);
            m->Indices[k++] = (unsigned short)(b + 1);
        }
    d.Mesh = m;
}

// ---- effect decals ----

// PANZERS 0x6f24f0
int STerrain::CreateEffectDecal(int texture, float x, float z, float rotation, float scale,
                                unsigned color, int blend, bool flag, bool pass)
{
    PZ_TRACE("STerrain::CreateEffectDecal (0x6f24f0)");
    int i = HdHeapAlloc(&EffectDecals);
    SEffectDecal& d = EffectDecals.Data[i];
    d.Texture = TerrainAddRefTexture(texture);
    d.X = x;
    d.Z = z;
    d.Rotation = rotation;
    d.Scale = scale;
    d.Color = color;
    d.Blend = blend;
    d.Flag = flag;
    d.Pass = pass;
    d.Alpha = 1.0f;
    return i;
}

// PANZERS 0x6f29b0
void STerrain::DestroyEffectDecal(int decal)
{
    PZ_TRACE("STerrain::DestroyEffectDecal (0x6f29b0)");
    if (!HdHeapLive(&EffectDecals, decal))
        return;
    TerrainReleaseTexture(EffectDecals.Data[decal].Texture);
    HdHeapRemove(&EffectDecals, decal);
}

// PANZERS 0x6f8900
void STerrain::SetEffectDecalPosition(int decal, float x, float z)
{
    PZ_TRACE("STerrain::SetEffectDecalPosition (0x6f8900)");
    if (HdHeapLive(&EffectDecals, decal)) {
        EffectDecals.Data[decal].X = x;
        EffectDecals.Data[decal].Z = z;
    }
}

// PANZERS 0x6f8970
void STerrain::SetEffectDecalRotation(int decal, float rotation)
{
    PZ_TRACE("STerrain::SetEffectDecalRotation (0x6f8970)");
    if (HdHeapLive(&EffectDecals, decal))
        EffectDecals.Data[decal].Rotation = rotation;
}

// PANZERS 0x6f8ac0
void STerrain::SetEffectDecalTexture(int decal, int texture)
{
    PZ_TRACE("STerrain::SetEffectDecalTexture (0x6f8ac0)");
    if (HdHeapLive(&EffectDecals, decal)) {
        TerrainReleaseTexture(EffectDecals.Data[decal].Texture);
        EffectDecals.Data[decal].Texture = TerrainAddRefTexture(texture);
    }
}

// PANZERS 0x6f8a50
void STerrain::SetEffectDecalAlpha(int decal, float alpha)
{
    PZ_TRACE("STerrain::SetEffectDecalAlpha (0x6f8a50)");
    if (HdHeapLive(&EffectDecals, decal))
        EffectDecals.Data[decal].Alpha = alpha;
}

// HD 0x6f6640 (reimplemented texture-coordinate transform: HD composes
// translate(-x,-z), rotate, scale(1/sx, -1/sz), translate(0.5, 0.5) with its
// 2D matrix helper 0x661bb0; the composition order was not verified)
void STerrain::RenderEffectDecals(bool shadowPass, int pass)
{
    IDirect3DDevice9* dev = TerrainDevice();
    if (!dev)
        return;
    for (int i = HdHeapNext(&EffectDecals, -1); i >= 0; i = HdHeapNext(&EffectDecals, i)) {
        const SEffectDecal& d = EffectDecals.Data[i];
        if (d.Pass != pass)
            continue;
        int tw, th;
        TerrainTextureSize(d.Texture, &tw, &th);
        float sx = (float)((double)((float)tw * d.Scale) * 0.015625);
        float sz = (float)((double)((float)th * d.Scale) * 0.015625);
        float ex, ez;
        if (d.Rotation == 0.0f) {
            ex = sx * 0.5f;
            ez = sz * 0.5f;
        } else {
            float s = fabsf(sinf(d.Rotation)), c = fabsf(cosf(d.Rotation));
            ex = s * sz * 0.5f + c * sx * 0.5f;
            ez = c * sz * 0.5f + s * sx * 0.5f;
        }
        int x0 = (int)lrintf(d.X - ex), x1 = (int)lrintf(d.X + ex);
        int z0 = (int)lrintf(d.Z - ez), z1 = (int)lrintf(d.Z + ez);
        if (!ParcelVisibleAt((float)x0, (float)z0) && !ParcelVisibleAt((float)x0, (float)z1)
            && !ParcelVisibleAt((float)x1, (float)z0) && !ParcelVisibleAt((float)x1, (float)z1))
            continue;
        if (d.Alpha == 0.0f)
            continue;
        unsigned color;
        if (d.Blend == 0) {
            color = ((unsigned)(int)(d.Alpha * 255.0f) << 24) | (d.Color & 0xffffff);
        } else {
            unsigned r = (unsigned)lrintf((float)((d.Color >> 16) & 0xff) * d.Alpha);
            unsigned g = (unsigned)lrintf((float)((d.Color >> 8) & 0xff) * d.Alpha);
            unsigned b = (unsigned)lrintf((float)(d.Color & 0xff) * d.Alpha);
            color = (r << 16) | (g << 8) | b;
        }
        STerrainDrawState st;
        st.SetAlphaOp(0, D3DTOP_MODULATE, D3DTA_TEXTURE, D3DTA_CURRENT);
        st.SetTexture(0, d.Texture, false);
        if (d.Blend == 0)
            st.SetBlendMode(2);
        else
            st.SetBlendModeEx(d.Blend, -1);
        st.SetFog(shadowPass ? 2 : (d.Blend == 1 ? 1 : 0), 0);
        st.Apply(dev);

        float cr = cosf(-d.Rotation), sr = sinf(-d.Rotation);
        int w = x1 - x0, h = z1 - z0;
        if (w <= 0 || h <= 0)
            continue;
        const float* hs = pass ? Heights2 : Heights;
        STerrainVertex* v = new STerrainVertex[(w + 1) * (h + 1)];
        int n = 0;
        for (int z = z0; z <= z1; ++z)
            for (int x = x0; x <= x1; ++x, ++n) {
                bool in = x >= 0 && x <= Width && z >= 0 && z <= Height;
                STerrainVertex& o = v[n];
                o.X = (float)x;
                o.Y = in ? hs[Stride * z + x] : 0.0f;
                o.Z = (float)z;
                if (in) {
                    const float* nn = Normals + (Stride * z + x) * 3;
                    o.NX = nn[0]; o.NY = nn[1]; o.NZ = nn[2];
                } else {
                    o.NX = 0; o.NY = 1; o.NZ = 0;
                }
                o.Color = color;
                float dx = (float)x - d.X, dz = (float)z - d.Z;
                o.U = (dx * cr - dz * sr) / sx + 0.5f;
                o.V = -(dx * sr + dz * cr) / sz + 0.5f;
            }
        unsigned short* idx = new unsigned short[w * h * 6];
        int k = 0;
        for (int j = 0; j < h; ++j)
            for (int i2 = 0; i2 < w; ++i2) {
                unsigned short a = (unsigned short)((w + 1) * j + i2);
                unsigned short b = (unsigned short)(w + 1 + a);
                idx[k++] = a;
                idx[k++] = (unsigned short)(a + 1);
                idx[k++] = b;
                idx[k++] = b;
                idx[k++] = (unsigned short)(a + 1);
                idx[k++] = (unsigned short)(w + 2 + a);
            }
        TerrainDraw(dev, kTerrainFVF, v, n, sizeof(STerrainVertex), idx, k);
        delete[] v;
        delete[] idx;
    }
}

} // namespace pz
