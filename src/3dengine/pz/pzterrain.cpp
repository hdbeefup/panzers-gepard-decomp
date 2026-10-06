// src/3dengine/pz/pzterrain.cpp
// pz::STerrain: construction, buffers, layers, Update, visibility and the
// draw entry points SScene calls. OWNER: agent B. Roads are in road.cpp,
// decals in decal.cpp, parcels and the draw state in parcel.cpp.

#include <math.h>
#include <new>
#include <stdlib.h>
#include <string.h>
#include "pzterrain.h"
#include "parcel.h"
#include "pzscene.h"
#include "pzviewport.h"
#include "igepardhd.h"
#include "core_common.h"
#include "stream.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

static_assert(offsetof(STerrain, EffectDecals) == 0x04, "STerrain layout");
static_assert(offsetof(STerrain, DecalFormat) == 0x18, "STerrain layout");
static_assert(offsetof(STerrain, Roads) == 0x1c, "STerrain layout");
static_assert(offsetof(STerrain, Junctions) == 0x30, "STerrain layout");
static_assert(offsetof(STerrain, RoadFormat) == 0x44, "STerrain layout");
static_assert(offsetof(STerrain, Scene) == 0x48, "STerrain layout");
static_assert(offsetof(STerrain, Width) == 0x50, "STerrain layout");
static_assert(offsetof(STerrain, Stride) == 0x64, "STerrain layout");
static_assert(offsetof(STerrain, Heights) == 0x70, "STerrain layout");
static_assert(offsetof(STerrain, Normals) == 0x84, "STerrain layout");
static_assert(offsetof(STerrain, ParcelRenderer) == 0x90, "STerrain layout");
static_assert(offsetof(STerrain, ParcelsX) == 0xa0, "STerrain layout");
static_assert(offsetof(STerrain, TileMap) == 0xb0, "STerrain layout");
static_assert(offsetof(STerrain, LayerTextures) == 0xb4, "STerrain layout");
static_assert(offsetof(STerrain, LayerFlags) == 0x10a4, "STerrain layout");
static_assert(offsetof(STerrain, Flora) == 0x10e4, "STerrain layout");
static_assert(offsetof(STerrain, FloraJitter) == 0x1924, "STerrain layout");
static_assert(offsetof(STerrain, Dirty) == 0x11924, "STerrain layout");
static_assert(offsetof(STerrain, DrawFlags) == 0x11938, "STerrain layout");
static_assert(offsetof(STerrain, SketchState) == 0x1193c, "STerrain layout");
static_assert(offsetof(STerrain, SketchTexture) == 0x11c44, "STerrain layout");
static_assert(offsetof(STerrain, Compact) == 0x11c50, "STerrain layout");
static_assert(offsetof(STerrain, BlockMapShown) == 0x11c64, "STerrain layout");
static_assert(offsetof(STerrain, BlockMap) == 0x11c68, "STerrain layout");
static_assert(offsetof(STerrain, MapDecals) == 0x11c70, "STerrain layout");
static_assert(offsetof(STerrain, ParcelFormat) == 0x11c7c, "STerrain layout");
static_assert(sizeof(STerrainDrawState) <= 0x308, "draw state fits the HD block");

PFloraDraw g_FloraDraw = nullptr;

// SScene fields the terrain reads (HD offsets inside the HD-sized SScene).
template <class T> static T& SceneField(SScene* s, unsigned off)
{
    return *reinterpret_cast<T*>(reinterpret_cast<unsigned char*>(s) + off);
}

// HD 0x6f4c00 / 0x6f1f60 index with FLDCW 0x047f (round down).
static inline int FloorInt(float f)
{
    return (int)floorf(f);
}

// PANZERS 0x6efde0
STerrain::STerrain(SScene* scene, int width, int height, int p4)
{
    PZ_TRACE("STerrain::STerrain (0x6efde0)");
    memset(&EffectDecals, 0, sizeof(STerrain) - offsetof(STerrain, EffectDecals));
    EffectDecals.FreeHead = -1;
    Roads.FreeHead = -1;
    Junctions.FreeHead = -1;
    new (SketchState) STerrainDrawState();
    MapDecals = nullptr;
    MapDecalCount = MapDecalCap = 0;

    Scene = scene;
    Device = TerrainDevice();
    P = p4;
    Width = width;
    Height = height;
    WidthP = width * p4;
    Stride = width + 1;
    HeightP = height * p4;
    Verts = (height + 1) * (width + 1);
    Tiles = width * height;

    Heights = new float[Verts];
    memset(Heights, 0, Verts * sizeof(float));
    Heights2 = new float[Verts];
    memset(Heights2, 0, Verts * sizeof(float));
    Blend = (unsigned char*)malloc(Verts * 0x10);
    memset(Blend, 0, Verts * 0x10);
    Diffuse = new unsigned[Verts];
    memset(Diffuse, 0xff, Verts * sizeof(unsigned));
    Buffer80 = new unsigned[Verts];
    memset(Buffer80, 0, Verts * sizeof(unsigned));
    Normals = new float[Verts * 3];
    memset(Normals, 0, Verts * 3 * sizeof(float));
    Shade = new unsigned char[Verts];
    memset(Shade, 0x80, Verts);
    Buffer8C = new unsigned[Verts];
    memset(Buffer8C, 0, Verts * sizeof(unsigned));

    int bm = P * P * Tiles;
    BlockMap = new unsigned[bm];
    for (int i = 0; i < bm; ++i)
        BlockMap[i] = 0x2200;
    BlockAux = new unsigned char[bm];
    memset(BlockAux, 0, bm);

    if ((Width % 8) != 0 || (Height % 8) != 0)
        Logger.g->Panic("STerrain::STerrain: Plane should be multiple of parcels (%dx%d)", Width, Height);

    ParcelsX = Width / 8;
    ParcelsZ = Height / 8;
    ParcelCount = ParcelsX * ParcelsZ;
    Parcels = new SParcelInfo[ParcelCount];
    memset(Parcels, 0, ParcelCount * sizeof(SParcelInfo));
    for (int i = 0; i < ParcelCount; ++i)
        Parcels[i].Visible = true;
    TileMap = new unsigned char[ParcelCount * 2];
    memset(TileMap, 0, ParcelCount * 2);
    for (int i = 0; i < ParcelCount; ++i)
        TileMap[i * 2 + 1] = (unsigned char)((rand() * 0x3c) >> 15);

    DrawFlags = 5;
    reinterpret_cast<STerrainDrawState*>(SketchState)->Lighting = false;
    reinterpret_cast<STerrainDrawState*>(SketchState)->SetBlendMode(2);
    SketchTexture = -1;
    Overlay = 0;
    OverlayMode = 0;
    OverlayFrame = 0;
    Flag5C = false;
    Compact = false;
    BlockMapShown = false;

    for (int v = 0; v < 60; ++v)
        for (int l = 0; l < 17; ++l)
            LayerTextures[v][l] = -1;
    for (int l = 0; l < 16; ++l) {
        Flora[l].Count = 0;
        LayerFlags[l] = 0;
    }
    for (int i = 0; i < 0x1000; ++i) {
        FloraJitter[i].DX = (float)((double)(float)((double)rand() * 3.0517578125e-05) - 0.5);
        FloraJitter[i].DZ = (float)((double)(float)((double)rand() * 3.0517578125e-05) - 0.5);
        FloraJitter[i].Rotation = (float)((double)rand() * 3.0517578125e-05 * 6.2831854820251465);
        FloraJitter[i].Pick = (float)((double)rand() * 3.0517578125e-05);
    }

    DecalFormat = kTerrainFVF;
    ParcelFormat = kTerrainFVF;
    LineFormat = D3DFVF_XYZ | D3DFVF_DIFFUSE;
    Dirty = true;
    DirtyX0 = 0;
    DirtyZ0 = 0;
    DirtyX1 = Width + 1;
    DirtyZ1 = Height + 1;

    ParcelRenderer = new SParcel(this);
    WireGrid = new SWireframeParcel(this, false);
    WireOutline = new SWireframeParcel(this, true);
    BlockMapParcel = new SBlockMapParcel(this, P);
    RoadFormat = -1;
}

// PANZERS 0x6f06c0
STerrain::~STerrain()
{
    PZ_TRACE("STerrain::~STerrain (0x6f06c0)");
    // Effect decals hold texture references (0x6f27c0 releases them).
    for (int i = HdHeapNext(&EffectDecals, -1); i >= 0; i = HdHeapNext(&EffectDecals, i))
        TerrainReleaseTexture(EffectDecals.Data[i].Texture);
    for (int v = 0; v < 60; ++v)
        for (int l = 0; l < 17; ++l)
            TerrainReleaseTexture(LayerTextures[v][l]);
    for (int l = 0; l < 16; ++l)
        for (int i = 0; i < Flora[l].Count; ++i)
            PzGepard()->ReleaseModelPrototype(Flora[l].Protos[i]);
    delete[] Heights;
    delete[] Heights2;
    free(Blend);
    delete[] Diffuse;
    delete[] Buffer80;
    delete[] Normals;
    delete[] Shade;
    delete[] Buffer8C;
    delete[] BlockMap;
    delete[] BlockAux;
    for (int i = ParcelCount - 1; i >= 0; --i)
        delete Parcels[i].Compact;
    delete[] Parcels;
    delete[] TileMap;
    delete ParcelRenderer;
    delete WireGrid;
    delete WireOutline;
    delete BlockMapParcel;
    TerrainReleaseTexture(SketchTexture);
    for (int i = HdHeapNext(&Roads, -1); i >= 0; i = HdHeapNext(&Roads, i))
        RemoveRoad(i);
    for (int i = HdHeapNext(&Junctions, -1); i >= 0; i = HdHeapNext(&Junctions, i))
        RemoveJunction(i);
    free(Roads.Data);
    free(Junctions.Data);
    for (int i = 0; i < MapDecalCount; ++i)
        RemoveDecal(MapDecalCount - 1);
    free(MapDecals);
    free(EffectDecals.Data);
}

// ---- slots (HD vtable order) ----

// PANZERS 0x6f0d00
void STerrain::Acquire(STerrainBuffers* out)
{
    PZ_TRACE("STerrain::Acquire (0x6f0d00)");
    out->Heights = Heights;
    out->Buffer74 = Heights2;
    out->Diffuse = reinterpret_cast<float*>(Diffuse);
    out->Blend = Blend;
    out->BufferB0 = TileMap;
    out->Buffer8C = Buffer8C;
    out->Buffer11c68 = BlockMap;
    out->Buffer11c6c = BlockAux;
}

// PANZERS 0x6f5510
void STerrain::LoadLayerTexture(int layer, const char* baseName)
{
    PZ_TRACE("STerrain::LoadLayerTexture (0x6f5510)");
    int found[6];
    int count = 0;
    if (baseName) {
        for (int i = 1; i <= 6; ++i) {
            char name[300];
            _snprintf(name, sizeof(name), "%s_%d_a.tga", baseName, i);
            name[sizeof(name) - 1] = 0;
            int t = TerrainLoadTexture(name, 1, false);
            if (t < 0) {
                _snprintf(name, sizeof(name), "%s_%d.tga", baseName, i);
                name[sizeof(name) - 1] = 0;
                t = TerrainLoadTexture(name, 1, false);
            }
            found[count] = t;
            if (t < 0)
                break;
            ++count;
        }
    }
    // The 60 parcel variants cycle through the 1..6 texture variants.
    for (int v = 0; v < 60; ++v) {
        TerrainReleaseTexture(LayerTextures[v][layer]);
        LayerTextures[v][layer] = count ? TerrainAddRefTexture(found[v % count]) : -1;
    }
    for (int i = 0; i < count; ++i)
        TerrainReleaseTexture(found[i]);
}

// HD 0x6f52b0 (the directory listing 0x65e720 is replaced by the SWINE
// file-system search; prototype loading goes through the facade)
void STerrain::LoadFloraLayer(int layer, const char* floraDir, float scale)
{
    PZ_TRACE("STerrain::LoadFloraLayer (0x6f52b0)");
    SFloraLayer& f = Flora[layer];
    for (int i = 0; i < f.Count; ++i) {
        PzGepard()->ReleaseModelPrototype(f.Protos[i]);
        f.Protos[i] = -1;
    }
    f.Count = 0;
    if (!floraDir || !*floraDir)
        return;
    // HD lists "<dir>*.4d" (up to 32 files) and loads each as a model
    // prototype with the given scale. The pak TOC listing is not ported:
    // try the names the flora folders use (grass01.4d ...).
    for (int i = 1; i <= 32 && f.Count < 32; ++i) {
        char name[300];
        _snprintf(name, sizeof(name), "%sgrass%02d.4d", floraDir, i);
        name[sizeof(name) - 1] = 0;
        if (FileSystem.Stat(name, nullptr) != 0)
            break;
        int proto = PzGepard()->LoadModelPrototype(name, scale, nullptr, 0);
        f.Protos[f.Count] = proto;
        if (proto >= 0)
            ++f.Count;
    }
}

// HD 0x6f5740 (reimplemented: loads the sketch bitmap as a texture; HD
// uploads a cropped copy and keeps the bitmap size)
void STerrain::LoadSketchTexture(const char* file)
{
    PZ_TRACE("STerrain::LoadSketchTexture (0x6f5740)");
    int t = file ? TerrainLoadTexture(file, 0, false) : -1;
    TerrainReleaseTexture(SketchTexture);
    SketchTexture = t;
    TerrainTextureSize(t, &SketchWidth, &SketchHeight);
}

// PANZERS 0x6f8b80
void STerrain::SetDrawFlags(int flags)
{
    PZ_TRACE("STerrain::SetDrawFlags (0x6f8b80)");
    DrawFlags = flags;
}

// PANZERS 0x6f50b0
int STerrain::GetDrawFlags()
{
    PZ_TRACE("STerrain::GetDrawFlags (0x6f50b0)");
    return DrawFlags;
}

// PANZERS 0x6f1f60
void STerrain::ComputeSunOcclusion()
{
    PZ_TRACE("STerrain::ComputeSunOcclusion (0x6f1f60)");
    const float sunX = SceneField<float>(Scene, 0x110);
    const float sunY = SceneField<float>(Scene, 0x114);
    const float sunZ = SceneField<float>(Scene, 0x118);
    float maxH = 0.0f;
    for (int i = 0; i < Verts; ++i)
        if (maxH < Heights[i])
            maxH = Heights[i];
    for (int x = 0; x <= Width; ++x) {
        for (int z = 0; z <= Width; ++z) {   // HD bounds this loop by +0x50 (Width) too
            int count = 0;
            for (int sx = 0; sx < 4; ++sx) {
                for (int sz = 0; sz < 4; ++sz) {
                    float px = (float)(((double)sx * 0.25 + (double)x) - 0.375);
                    float pz = (float)(((double)sz * 0.25 + (double)z) - 0.375);
                    float h = HeightAt(px, pz);
                    for (;;) {
                        if (px < 0.0f || (float)Width <= px || pz < 0.0f || (float)Height <= pz)
                            break;
                        h -= sunY;
                        px -= sunX;
                        pz -= sunZ;
                        if (maxH < h)
                            break;
                        if (HeightAt(px, pz) > h) {
                            ++count;
                            break;
                        }
                    }
                }
            }
            if (z <= Height)
                Shade[Stride * z + x] = (unsigned char)(count * -8 - 0x80);
        }
    }
}

// PANZERS 0x6ff510
void STerrain::SetOverlay(int overlay, int mode)
{
    PZ_TRACE("STerrain::SetOverlay (0x6ff510)");
    Overlay = overlay;
    OverlayMode = mode;
    OverlayFrame = SceneField<int>(Scene, 0xa0);
}

// PANZERS 0x6f5230
void STerrain::Invalidate(int x0, int z0, int x1, int z1)
{
    PZ_TRACE("STerrain::Invalidate (0x6f5230)");
    if (!Dirty) {
        DirtyX0 = x0;
        DirtyZ0 = z0;
        DirtyX1 = x1;
        Dirty = true;
        DirtyZ1 = z1;
        return;
    }
    if (x0 < DirtyX0) DirtyX0 = x0;
    if (z0 < DirtyZ0) DirtyZ0 = z0;
    if (DirtyX1 < x1) DirtyX1 = x1;
    if (DirtyZ1 < z1) DirtyZ1 = z1;
}

// PANZERS 0x6f8c90
void STerrain::Update()
{
    PZ_TRACE("STerrain::Update (0x6f8c90)");
    if (!Dirty)
        return;
    Dirty = false;
    float x0 = (float)DirtyX0, z0 = (float)DirtyZ0, x1 = (float)DirtyX1, z1 = (float)DirtyZ1;
    if (0.0f < x0) x0 -= 1.0f;
    if (x1 <= (float)Width) x1 += 1.0f;
    if (0.0f < z0) z0 -= 1.0f;
    if (z1 <= (float)Height) z1 += 1.0f;

    // Normals: central differences, one-sided on the border, y = 1.
    for (int z = (int)z0; (float)z < z1; ++z) {
        int i = (int)((float)(Stride * z) + x0);
        for (int x = (int)x0; (float)x < x1; ++x, ++i) {
            const float* h = Heights;
            float dx, dz;
            if (x == 0)
                dx = h[i] - h[i + 1];
            else if (x == Width)
                dx = h[i - 1] - h[i];
            else
                dx = (h[i - 1] - h[i + 1]) * 0.5f;
            if (z == 0)
                dz = h[i] - h[i + Stride];
            else if (z == Height)
                dz = h[i - Stride] - h[i];
            else
                dz = (h[i - Stride] - h[i + Stride]) * 0.5f;
            double inv = 1.0 / sqrt((double)(dx * dx + 1.0f + dz * dz));
            Normals[i * 3 + 0] = (float)((double)dx * inv);
            Normals[i * 3 + 1] = (float)inv;
            Normals[i * 3 + 2] = (float)((double)dz * inv);
        }
    }

    // Parcels touched: height bounds and the layers present.
    float px0 = 0.0f < x0 ? x0 * 0.125f : 0.0f;
    float px1 = (float)Width <= x1 ? (float)(Width / 8 - 1) : x1 * 0.125f;
    float pz0 = 0.0f < z0 ? z0 * 0.125f : 0.0f;
    float pz1 = (float)Height <= z1 ? (float)(Height / 8 - 1) : z1 * 0.125f;
    for (int pz = (int)pz0; (float)pz <= pz1; ++pz) {
        for (int px = (int)px0; (float)px <= px1; ++px) {
            int v0 = (Stride * pz + px) * 8;
            float mn = 100.0f, mx = -100.0f;
            for (int r = 0; r < 9; ++r)
                for (int c = 0; c < 9; ++c) {
                    float h = Heights[v0 + r * Stride + c];
                    if (h < mn) mn = h;
                    if (mx < h) mx = h;
                }
            SParcelInfo& p = Parcels[ParcelsX * pz + px];
            p.MinY = mn;
            p.MaxY = mx + 2.0f;
            for (int k = 0; k < 16; ++k) {
                bool any = false;
                if ((LayerFlags[k] & 0x80) == 0)
                    for (int r = 0; r < 9 && !any; ++r)
                        for (int c = 0; c < 9; ++c)
                            if (Blend[(v0 + r * Stride + c) * 0x10 + k]) {
                                any = true;
                                break;
                            }
                p.Layers[k] = any;
            }
            if (p.Compact) {
                int i = ParcelsX * pz + px;
                delete p.Compact;
                p.Compact = new SParcel2(px * 8, pz * 8, Heights + v0, Blend + v0 * 0x10,
                                         Normals + v0 * 3, Diffuse + v0, Stride, TileMap[i * 2],
                                         LayerTextures[TileMap[i * 2 + 1]], LayerFlags,
                                         &MapDecals, i, this);
            }
        }
    }
}

// PANZERS 0x6f4b50
void STerrain::SetFlag5C(bool on)
{
    PZ_TRACE("STerrain::SetFlag5C (0x6f4b50)");
    Flag5C = on;
}

// PANZERS 0x6f8680
void STerrain::SetCompactMode(bool on)
{
    PZ_TRACE("STerrain::SetCompactMode (0x6f8680)");
    if (!on) {
        for (int i = ParcelCount - 1; i >= 0; --i) {
            delete Parcels[i].Compact;
            Parcels[i].Compact = nullptr;
        }
    } else {
        Update();
        // HD 0x6f9330 (overlay prepare) is not ported.
        for (int pz = 0; pz < ParcelsZ; ++pz)
            for (int px = 0; px < ParcelsX; ++px) {
                int i = ParcelsX * pz + px;
                int v0 = (Stride * pz + px) * 8;
                if (!Parcels[i].Compact)
                    Parcels[i].Compact = new SParcel2(px * 8, pz * 8, Heights + v0,
                                                      Blend + v0 * 0x10, Normals + v0 * 3,
                                                      Diffuse + v0, Stride, TileMap[i * 2],
                                                      LayerTextures[TileMap[i * 2 + 1]],
                                                      LayerFlags, &MapDecals, i, this);
            }
    }
    Compact = on;
}

// PANZERS 0x6f4b70
bool STerrain::GetCompactMode()
{
    PZ_TRACE("STerrain::GetCompactMode (0x6f4b70)");
    return Compact;
}

// PANZERS 0x6f8b60
void STerrain::SetLayerFlags(int layer, int flags)
{
    PZ_TRACE("STerrain::SetLayerFlags (0x6f8b60)");
    LayerFlags[layer] = flags;
}

// PANZERS 0x6f8640
void STerrain::ShowBlockMap(bool on)
{
    PZ_TRACE("STerrain::ShowBlockMap (0x6f8640)");
    BlockMapShown = on && BlockMap && BlockAux;
}

// PANZERS 0x6f4b60
bool STerrain::IsBlockMapShown()
{
    PZ_TRACE("STerrain::IsBlockMapShown (0x6f4b60)");
    return BlockMapShown;
}

// PANZERS 0x6f85f0
void STerrain::ClearBlockMarkers()
{
    PZ_TRACE("STerrain::ClearBlockMarkers (0x6f85f0)");
    BlockMapParcel->MarkerCount = 0;
}

// PANZERS 0x6f10e0
void STerrain::AddBlockMarker(int x, int z, unsigned color)
{
    PZ_TRACE("STerrain::AddBlockMarker (0x6f10e0)");
    SBlockMapParcel* b = BlockMapParcel;
    if (b->MarkerCount == b->MarkerCap) {
        int cap = b->MarkerCap < 0x10 ? 0x10 : (b->MarkerCap * 6) / 5;
        b->Markers = (SBlockMapParcel::SMarker*)realloc(b->Markers, cap * sizeof(SBlockMapParcel::SMarker));
        b->MarkerCap = cap;
    }
    SBlockMapParcel::SMarker& m = b->Markers[b->MarkerCount++];
    m.X = x;
    m.Z = z;
    m.Color = (color & 0xffffff) | 0x7f000000;
}

// ---- internals ----

// PANZERS 0x6f4c00
float STerrain::HeightAt(float x, float z) const
{
    if (0.0f <= x && x < (float)Width && 0.0f <= z && z < (float)Height) {
        int ix = FloorInt(x), iz = FloorInt(z);
        float fx = x - (float)ix, fz = z - (float)iz;
        int i = Stride * iz + ix;
        int j = Width + i;   // the next row is i + Width + 1
        const float* h = Heights;
        return h[i + 1] * (1.0f - fz) * fx + h[i] * (1.0f - fz) * (1.0f - fx)
             + h[j + 1] * fz * (1.0f - fx) + h[j + 2] * fz * fx;
    }
    return 0.0f;
}

// PANZERS 0x6f4f60
void STerrain::NormalAt(float out[3], int x, int z) const
{
    if (x > -1 && x < Width && z > -1 && z < Height) {
        int i = Stride * z + x;
        const float* h = Heights;
        float dx = x == 0 ? h[i] - h[i + 1] : (h[i - 1] - h[i + 1]) * 0.5f;
        float dz = z == 0 ? h[i] - h[i + Stride] : (h[i - Stride] - h[i + Stride]) * 0.5f;
        double inv = 1.0 / sqrt((double)(dx * dx + 1.0f + dz * dz));
        out[0] = (float)((double)dx * inv);
        out[1] = (float)inv;
        out[2] = (float)((double)dz * inv);
        return;
    }
    out[0] = out[1] = out[2] = 0.0f;
}

// PANZERS 0x69db40
bool STerrain::ParcelVisibleAt(float x, float z) const
{
    if (0.0f <= x && x < (float)Width && 0.0f <= z && z < (float)Height) {
        int px = (int)floor((double)x * 0.125);
        int pz = (int)floor((double)z * 0.125);
        return Parcels[ParcelsX * pz + px].Visible;
    }
    return false;
}

// ---- entry points for SScene ----

// PANZERS 0x6f8610
void STerrain::MarkAllVisible()
{
    PZ_TRACE("STerrain::MarkAllVisible (0x6f8610)");
    for (int i = 0; i < ParcelCount; ++i)
        Parcels[i].Visible = true;
}

// HD 0x6f1180 (reimplemented: HD builds six planes from the viewport
// matrices 0x68b740/0x68bd80 and sweeps the parcel rows per plane with the
// parcel min/max heights and the scene's per-parcel object heights
// (scene+0x1a8). Here: the same six planes from the viewport camera and an
// AABB test per parcel, MinY..MaxY.)
void STerrain::Cull(SViewport* vp)
{
    PZ_TRACE("STerrain::Cull (0x6f1180)");
    MarkAllVisible();
    if (!vp || !vp->ProjectionSet)
        return;
    const SCameraParams& c = vp->Camera;
    float a = -c.Pitch;
    float f[3] = { sinf(c.Yaw) * cosf(a), sinf(a), cosf(c.Yaw) * cosf(a) };
    float r[3] = { cosf(c.Yaw), 0.0f, -sinf(c.Yaw) };
    float u[3] = { f[1] * r[2] - f[2] * r[1], f[2] * r[0] - f[0] * r[2], f[0] * r[1] - f[1] * r[0] };
    float aspect = 4.0f / 3.0f;
    IDirect3DDevice9* dev = TerrainDevice();
    D3DVIEWPORT9 view;
    if (dev && SUCCEEDED(dev->GetViewport(&view)) && view.Height)
        aspect = (float)view.Width / (float)view.Height;
    float ty = tanf(c.Fov * 0.5f), tx = ty * aspect;
    struct Plane { float N[3]; float D; } planes[6];
    for (int i = 0; i < 3; ++i) {
        planes[0].N[i] = f[i] * tx + r[i];
        planes[1].N[i] = f[i] * tx - r[i];
        planes[2].N[i] = f[i] * ty + u[i];
        planes[3].N[i] = f[i] * ty - u[i];
        planes[4].N[i] = f[i];
        planes[5].N[i] = -f[i];
    }
    const float eye[3] = { c.X, c.Y, c.Z };
    for (int p = 0; p < 6; ++p)
        planes[p].D = -(planes[p].N[0] * eye[0] + planes[p].N[1] * eye[1] + planes[p].N[2] * eye[2]);
    planes[4].D -= c.NearZ;
    planes[5].D += c.FarZ;
    for (int pz = 0; pz < ParcelsZ; ++pz)
        for (int px = 0; px < ParcelsX; ++px) {
            SParcelInfo& info = Parcels[ParcelsX * pz + px];
            float mn[3] = { (float)(px * 8), info.MinY, (float)(pz * 8) };
            float mx[3] = { (float)(px * 8 + 8), info.MaxY, (float)(pz * 8 + 8) };
            bool in = true;
            for (int p = 0; p < 6 && in; ++p) {
                const Plane& pl = planes[p];
                float d = pl.D;
                for (int i = 0; i < 3; ++i)
                    d += pl.N[i] * (pl.N[i] > 0.0f ? mx[i] : mn[i]);
                if (d < 0.0f)
                    in = false;
            }
            info.Visible = in;
        }
}

// PANZERS 0x6f2aa0
void STerrain::Render(SViewport* vp)
{
    PZ_TRACE("STerrain::Render (0x6f2aa0)");
    IDirect3DDevice9* dev = TerrainDevice();
    if (!dev)
        return;
    Update();
    TerrainSetWorldIdentity(dev);
    const bool shadow = TerrainOption(2) != 0;
    STerrainDrawState st;

    if (!Compact || (DrawFlags & 1) == 0) {
        if (DrawFlags & 1) {
            if ((DrawFlags & 4) == 0) {
                // Sketch: one texture over the whole map.
                st.Reset();
                if (DrawFlags & 8)
                    st.SetTexture(0, SketchTexture, false);
                if (shadow)
                    st.SetFog(2, 0);
                else if (TerrainOption(0x10) != 0)
                    st.SetColorOp(0, D3DTOP_MODULATE2X, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
                st.Apply(dev);
                for (int pz = 0; pz < ParcelsZ; ++pz)
                    for (int px = 0; px < ParcelsX; ++px) {
                        if (!Parcels[ParcelsX * pz + px].Visible)
                            continue;
                        int v0 = (Stride * pz + px) * 8;
                        bool noDiffuse = (DrawFlags & 8) != 0;
                        ParcelRenderer->DrawSketch(px * 8, pz * 8, Heights + v0,
                                                   noDiffuse ? nullptr : Diffuse + v0,
                                                   noDiffuse ? 0xffffffffu : 0u, Normals + v0 * 3,
                                                   Stride, 2.0f / (float)SketchWidth,
                                                   2.0f / (float)SketchHeight, true);
                    }
            } else {
                // The 16-layer blend, parcel by parcel.
                for (int pz = 0; pz < ParcelsZ; ++pz)
                    for (int px = 0; px < ParcelsX; ++px) {
                        int i = ParcelsX * pz + px;
                        if (!Parcels[i].Visible)
                            continue;
                        int v0 = (Stride * pz + px) * 8;
                        ParcelRenderer->DrawLayered(px * 8, pz * 8, Heights + v0, Diffuse + v0,
                                                    Blend + v0 * 0x10, Normals + v0 * 3, Stride,
                                                    TileMap[i * 2], LayerTextures[TileMap[i * 2 + 1]],
                                                    Parcels[i].Layers, shadow);
                    }
            }
        }
    } else {
        // Compact mode (debug).
        for (int i = 0; i < ParcelCount; ++i) {
            if (!Parcels[i].Visible || !Parcels[i].Compact)
                continue;
            Parcels[i].Stamp = OverlayFrame;
            Parcels[i].Compact->DrawCompact(shadow);
        }
        // 0x688780: blend and alpha test off, z write on.
        dev->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
        dev->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
        dev->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
    }

    if (!Compact) {
        RenderRoads(vp, shadow);
        // Map decals (DECS): one SMesh each, blend from the texture alpha.
        for (int i = 0; i < MapDecalCount; ++i) {
            SMapDecal& d = MapDecals[i];
            st.Reset();
            if (!shadow && TerrainOption(0x10) != 0)
                st.SetColorOp(0, D3DTOP_MODULATE2X, D3DTA_TEXTURE, D3DTA_CURRENT, D3DTA_CURRENT);
            st.SetTexture(0, d.Texture, false);
            st.SetBlendModeEx(0, d.Texture);
            if (d.Type == 3)
                st.SetFog(3, 0x40ffffc0);
            else if (d.Type == 2)
                st.SetFog(3, 0x20ffffff);
            else if (d.Type == 1)
                st.SetFog(3, 0x40ffff00);
            else
                st.SetFog(shadow ? 2 : 0, 0);
            st.Apply(dev);
            DrawDecalMesh(dev, d.Mesh);
        }
    }

    RenderEffectDecals(shadow, 0);

    if (BlockMapShown) {
        st.Reset();
        st.SetBlendModeEx(1, -1);
        st.SetBlendMode(2);
        st.Lighting = false;
        st.Apply(dev);
        for (int pz = 0; pz < ParcelsZ; ++pz)
            for (int px = 0; px < ParcelsX; ++px) {
                if (!Parcels[ParcelsX * pz + px].Visible)
                    continue;
                int b = (WidthP * pz + px) * P * 8;
                BlockMapParcel->DrawParcel(px * 8, pz * 8, Heights + (Stride * pz + px) * 8, Stride,
                                           BlockMap + b, BlockAux + b, WidthP);
            }
    }

    if (DrawFlags & 0x22) {
        // Editor wireframes (grid 0x8080ff80 / outline 0xff80ff80), z bias.
        st.Reset();
        st.Lighting = false;
        st.Apply(dev);
        float bias = 0.000244140625f;   // 0x39800000
        dev->SetRenderState(D3DRS_DEPTHBIAS, *reinterpret_cast<DWORD*>(&bias));
        for (int pz = 0; pz < ParcelsZ; ++pz)
            for (int px = 0; px < ParcelsX; ++px) {
                if (!Parcels[ParcelsX * pz + px].Visible)
                    continue;
                const float* h = Heights + (Stride * pz + px) * 8;
                if (DrawFlags & 2)
                    WireGrid->DrawWireframe(px * 8, pz * 8, h, Stride, 0x8080ff80);
                if (DrawFlags & 0x20)
                    WireOutline->DrawWireframe(px * 8, pz * 8, h, Stride, 0xff80ff80);
            }
        dev->SetRenderState(D3DRS_DEPTHBIAS, 0);
    }
}

// HD 0x6f46e0 (the shadow-buffer projection part is reimplemented without
// the per-technique texture transforms; the fog pass is lifted)
void STerrain::RenderShadowPass()
{
    PZ_TRACE("STerrain::RenderShadowPass (0x6f46e0)");
    IDirect3DDevice9* dev = TerrainDevice();
    if (!dev)
        return;
    Update();
    TerrainSetWorldIdentity(dev);
    STerrainDrawState st;
    // Pass 1: multiply the scene shadow texture (scene+0x14) over the
    // terrain, texture coordinates from the world position through the
    // scene shadow matrix (scene+0x58). Technique 1 (option 2) blacks out
    // the ambient while it draws. Same geometry, z equal.
    int shadowTex = SceneField<int>(Scene, 0x14);
    if (shadowTex >= 0) {
        st.SetTexture(0, shadowTex, false);
        st.SetBlendModeEx(TerrainOption(0x10) ? 4 : 3, -1);
        st.SetFog(1, 0);
        st.ZFunc = D3DCMP_EQUAL;
        st.Apply(dev);
        dev->SetTransform(D3DTS_TEXTURE0, reinterpret_cast<const D3DMATRIX*>(
                              reinterpret_cast<unsigned char*>(Scene) + 0x58));
        dev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, D3DTSS_TCI_CAMERASPACEPOSITION);
        dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_COUNT4 | D3DTTFF_PROJECTED);
        DWORD ambient = 0;
        int tech = TerrainOption(2);
        if (tech == 1) {
            dev->GetRenderState(D3DRS_AMBIENT, &ambient);
            dev->SetRenderState(D3DRS_AMBIENT, 0);
        }
        for (int pz = 0; pz < ParcelsZ; ++pz)
            for (int px = 0; px < ParcelsX; ++px) {
                if (!Parcels[ParcelsX * pz + px].Visible)
                    continue;
                int v0 = (Stride * pz + px) * 8;
                ParcelRenderer->DrawSketch(px * 8, pz * 8, Heights + v0, nullptr, 0xffffffff,
                                           Normals + v0 * 3, Stride, 2.0f / (float)Width,
                                           2.0f / (float)Height, true);
            }
        if (tech == 1)
            dev->SetRenderState(D3DRS_AMBIENT, ambient);
        dev->SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
        dev->SetTextureStageState(0, D3DTSS_TEXTURETRANSFORMFLAGS, D3DTTFF_DISABLE);
    }
    // Pass 2: fog. The terrain itself was drawn without fog (fog mode 2 when
    // option 2 is on); black terrain drawn additively with fog adds the fog.
    float fogStart = SceneField<float>(Scene, 0x164), fogEnd = SceneField<float>(Scene, 0x168);
    if (fogStart < fogEnd && SceneField<unsigned>(Scene, 0x15c) != 0) {
        st.Reset();
        st.ZFunc = D3DCMP_EQUAL;
        st.SetBlendModeEx(1, -1);
        st.SetFog(0, 0);
        st.Apply(dev);
        for (int pz = 0; pz < ParcelsZ; ++pz)
            for (int px = 0; px < ParcelsX; ++px) {
                if (!Parcels[ParcelsX * pz + px].Visible)
                    continue;
                int v0 = (Stride * pz + px) * 8;
                ParcelRenderer->DrawSketch(px * 8, pz * 8, Heights + v0, nullptr, 0xff000000,
                                           Normals + v0 * 3, Stride, 2.0f / (float)Width,
                                           2.0f / (float)Height, true);
            }
    }
}

// PANZERS 0x6f33c0
void STerrain::RenderLate(SViewport* vp)
{
    PZ_TRACE("STerrain::RenderLate (0x6f33c0)");
    TerrainSetWorldIdentity(TerrainDevice());
    RenderEffectDecals(true, 1);
    RenderFlora(vp);
}

// HD 0x6f33c0 flora part (reimplemented order: HD walks parcels and vertices
// back to front by the camera yaw octant; here row by row. Placement is the
// HD one: vertex with blend byte 0xff, jitter table by (z & 63, x & 63),
// within 60 tiles of the camera.)
void STerrain::RenderFlora(SViewport* vp)
{
    bool any = false;
    for (int k = 0; k < 16; ++k)
        any |= Flora[k].Count != 0;
    if (!any || !g_FloraDraw)
        return;
    if (!vp)
        return;
    float cx, cy, cz, yaw, pitch;
    vp->GetCamera(&cx, &cy, &cz, &yaw, &pitch);   // viewport +0x24
    for (int pz = 0; pz < ParcelsZ; ++pz)
        for (int px = 0; px < ParcelsX; ++px) {
            if (!Parcels[ParcelsX * pz + px].Visible)
                continue;
            for (int r = 0; r < 8; ++r)
                for (int c = 0; c < 8; ++c) {
                    int x = px * 8 + c, z = pz * 8 + r;
                    for (int k = 0; k < 16; ++k) {
                        if (!Flora[k].Count)
                            continue;
                        if (Blend[(Stride * z + x) * 0x10 + k] != 0xff)
                            continue;
                        float h = Heights[Stride * z + x];
                        float d2 = (z - cz) * (z - cz) + (x - cx) * (x - cx) + (h - cy) * (h - cy);
                        if (d2 > 3600.0f)
                            continue;
                        const SFloraJitter& j = FloraJitter[(z & 0x3f) * 0x40 + (x & 0x3f)];
                        int proto = Flora[k].Protos[(int)((float)Flora[k].Count * j.Pick)];
                        float wx = x + j.DX, wz = z + j.DZ;
                        float s = sinf(j.Rotation), co = cosf(j.Rotation);
                        float m[16] = { co, 0, -s, 0,   0, 1, 0, 0,   s, 0, co, 0,
                                        wx, HeightAt(wx, wz), wz, 1 };
                        g_FloraDraw(Scene, proto, m);
                    }
                }
        }
}

} // namespace pz
