// src/3dengine/pz/pzterrain.h
// pz::STerrain: HD STerrain (0x11c88 bytes, ctor 0x6efde0, dtor 0x6f06c0).
// Created only by SScene CreateTerrain (+0x64) with (scene, w, h, p); the
// TERR loader passes (168, 184, 4) for menu.map. 16 blend layers (SWINE has 9).
// OWNER: agent B.
//
// Files: pzterrain.cpp (ctor, buffers, layers, Update, visibility, draw
// entry points), parcel.cpp (SParcel family + draw state), road.cpp (roads,
// junctions, RenderRoads), decal.cpp (map decals and effect decals).
//
// ---- Entry points SScene (agent A) calls, non-virtually, with ECX = terrain ----
//   SScene::PrepareViewport 0x6bbc40 -> Cull(vp)              0x6f1180 RET 4
//   SScene::RenderScene     0x6b7760 -> MarkAllVisible()      0x6f8610 RET 0
//   SScene::RenderViewport  0x6acaf0 and RenderScene:
//        Render(Gepard 0x67bf40())                            0x6f2aa0 RET 4
//        if (Gepard option 2 != 0 && scene+0x10 >= 0)
//            RenderShadowPass()                               0x6f46e0 RET 0
//   SScene::RenderViewport  0x6acaf0, after the model batches:
//        RenderLate(Gepard 0x67bf40())                        0x6f33c0 RET 4
// 0x67bf40 returns the current viewport (Gepard viewport heap +0x578 at
// index +0x59c): 0x6f33c0 calls its +0x24 GetCamera. Pass the SViewport*
// being rendered. Render ignores it apart from handing it to RenderRoads.

#ifndef PZ_PZTERRAIN_H
#define PZ_PZTERRAIN_H

#include <stdlib.h>
#include <string.h>
#include "iterrain.h"

namespace pz {

struct SScene;
struct SViewport;
struct SParcel;
struct SParcel2;
struct SWireframeParcel;
struct SBlockMapParcel;
struct SDecalMesh;
struct STerrainVertex;

// Parcel record, STerrain+0xac (0x24 bytes each, ctor 0x6efde0).
struct SParcelInfo {
    SParcel2*     Compact;     // +0x00 compact-mode parcel (SetCompactMode)
    bool          Visible;     // +0x04 set by Cull / MarkAllVisible
    unsigned char _05[3];
    float         MinY;        // +0x08 Update
    float         MaxY;        // +0x0c Update (+2)
    unsigned char Layers[16];  // +0x10 blend byte k non-zero somewhere in the parcel
    int           Stamp;       // +0x20 compact mode frame stamp
};
static_assert(sizeof(SParcelInfo) == 0x24, "SParcelInfo");

// Effect decal, SHeap at STerrain+0x04 (0x28 bytes, 0x6f24f0).
struct SEffectDecal {
    int           Use;         // +0x00 0x7fffffff live, else free-list link
    int           Texture;     // +0x04
    float         X, Z;        // +0x08
    float         Rotation;    // +0x10
    float         Scale;       // +0x14
    unsigned      Color;       // +0x18 RGB
    int           Blend;       // +0x1c 0 alpha blend, else SetBlendModeEx mode (1 additive: colour * alpha)
    unsigned char Flag;        // +0x20
    unsigned char Pass;        // +0x21 0 with the terrain, 1 after the models (Buffer74 heights)
    unsigned char _22[2];
    float         Alpha;       // +0x24
};
static_assert(sizeof(SEffectDecal) == 0x28, "SEffectDecal");

// Map decal, SDArray at STerrain+0x11c70 (0x28 bytes, 0x6f2420/0x6f5cf0).
struct SMapDecal {
    int         Texture;       // +0x00
    int         X, Z;          // +0x04 centre (tiles)
    int         X0, Z0;        // +0x0c
    int         X1, Z1;        // +0x14
    int         Rotation;      // +0x1c 0..7
    SDecalMesh* Mesh;          // +0x20 HD SMesh (FVF 0x112)
    int         Type;          // +0x24 0..3
};
static_assert(sizeof(SMapDecal) == 0x28, "SMapDecal");

// Road / junction mesh for one parcel (heap at road+0x24 / junction+0x44,
// 0x2c bytes).
struct SRoadMesh {
    int             Use;       // +0x00
    unsigned char   Visible;   // +0x04 "touched" marker of UpdateRoad
    unsigned char   _05[3];
    STerrainVertex* Verts;     // +0x08 FVF 0x152
    int             NVerts;    // +0x0c
    int             VCap;      // +0x10
    unsigned short* Indices;   // +0x14
    int             NIndices;  // +0x18
    int             ICap;      // +0x1c
    int             Parcel;    // +0x20 parcel index
    int             Seg0;      // +0x24 first / last control segment used
    int             Seg1;      // +0x28
};
static_assert(sizeof(SRoadMesh) == 0x2c, "SRoadMesh");

// HD SHeap<T> header {data, size, capacity, free head, count} (0x14 bytes).
template <class T> struct SHdHeap {
    T*  Data;
    int Size;
    int Capacity;
    int FreeHead;
    int Count;
};
static_assert(sizeof(SHdHeap<int>) == 0x14, "SHdHeap");

// Road control point inside the terrain (0x30 bytes, SetRoad 0x6f5840).
struct SRoadCtrl {
    float         X, Z;         // +0x00
    float         DirX, DirZ;   // +0x08
    unsigned char Dirty;        // +0x10
    unsigned char _11[3];
    float         W;            // +0x14
    int           FirstSample;  // +0x18 UpdateRoad: first sample of this segment
    float         Length;       // +0x1c UpdateRoad: arc length at this point
    int           X0, X1;       // +0x20 segment bounds (tiles)
    int           Z0, Z1;       // +0x28
};
static_assert(sizeof(SRoadCtrl) == 0x30, "SRoadCtrl");

// Road, SHeap at STerrain+0x1c (0x38 bytes).
struct SRoad {
    int                Use;          // +0x00
    int                Texture;      // +0x04
    SRoadCtrl*         Points;       // +0x08 SDArray<0x30>
    int                PointCount;   // +0x0c
    int                PointCap;     // +0x10
    float              Step;         // +0x14 sampling step (tiles)
    float              Width;        // +0x18
    float              TexLength;    // +0x1c
    unsigned           Flags;        // +0x20 0x80 dirty, 2 visible, 0x10 across/along swap, 0x20/0x40 flips
    SHdHeap<SRoadMesh> Meshes;       // +0x24
};
static_assert(sizeof(SRoad) == 0x38, "SRoad");

// Road junction, SHeap at STerrain+0x30 (0x58 bytes).
struct SRoadJunction {
    int                Use;          // +0x00
    int                Texture;      // +0x04
    float              X, Z;         // +0x08
    float              DirX, DirZ;   // +0x10
    unsigned char      Valid;        // +0x18
    unsigned char      _19[0x38 - 0x19];
    float              HalfX;        // +0x38
    float              HalfZ;        // +0x3c
    unsigned           Flags;        // +0x40 0x80 dirty, 2 visible, 0x20 flip
    SHdHeap<SRoadMesh> Meshes;       // +0x44
};
static_assert(sizeof(SRoadJunction) == 0x58, "SRoadJunction");

// Flora layer (STerrain+0x10e4, 0x84 bytes each).
struct SFloraLayer {
    int Count;          // +0x00
    int Protos[32];     // +0x04 Gepard model prototypes
};
static_assert(sizeof(SFloraLayer) == 0x84, "SFloraLayer");

// Random grass placement table (STerrain+0x1924, 4096 entries).
struct SFloraJitter {
    float DX, DZ;       // [-0.5, 0.5)
    float Rotation;     // [0, 2pi)
    float Pick;         // [0, 1) prototype choice
};

// Grass/flora draw hook. HD draws each flora instance with the prototype's
// mesh (SGepard+0x550 heap -> SPModel -> mesh vtbl +4); prototypes are agent
// A's. SScene can set this to draw one instance; with no hook the flora pass
// only counts instances.
typedef void (*PFloraDraw)(SScene* scene, int proto, const float world[16]);
extern PFloraDraw g_FloraDraw;

struct STerrain : SITerrain {
    STerrain(SScene* scene, int width, int height, int p4);   // 0x6efde0
    ~STerrain();                                              // 0x6f06c0

    void Acquire(STerrainBuffers* out) override;
    void LoadLayerTexture(int layer, const char* baseName) override;
    void LoadFloraLayer(int layer, const char* floraDir, float scale) override;
    void LoadSketchTexture(const char* file) override;
    void SetDrawFlags(int flags) override;
    int GetDrawFlags() override;
    void ComputeSunOcclusion() override;
    void SetOverlay(int overlay, int mode) override;
    void Invalidate(int x0, int z0, int x1, int z1) override;
    void Update() override;
    void SetFlag5C(bool on) override;
    void SetCompactMode(bool on) override;
    bool GetCompactMode() override;
    void SetLayerFlags(int layer, int flags) override;
    void ShowBlockMap(bool on) override;
    bool IsBlockMapShown() override;
    void ClearBlockMarkers() override;
    void AddBlockMarker(int x, int z, unsigned color) override;
    void AddDecal(int index, int texture, int x, int z, int rotation) override;
    void RemoveDecal(int index) override;
    bool IsInDecal(int index, float x, float z) override;
    void SetDecalType(int index, int type) override;
    void MoveDecal(int index, int x, int z, int rotation) override;
    void UpdateDecals() override;
    int CreateEffectDecal(int texture, float x, float z, float rotation, float scale, unsigned color, int blend, bool flag, bool pass) override;
    void DestroyEffectDecal(int decal) override;
    void SetEffectDecalPosition(int decal, float x, float z) override;
    void SetEffectDecalRotation(int decal, float rotation) override;
    void SetEffectDecalTexture(int decal, int texture) override;
    void SetEffectDecalAlpha(int decal, float alpha) override;
    int CreateRoad(int texture, const SRoadPointArray* points, float step, float width, float texLength, unsigned flags) override;
    void SetRoad(int road, int texture, const SRoadPointArray* points, float step, float width, float texLength, unsigned flags) override;
    void UpdateRoadHeights(int road) override;
    void DestroyRoad(int road) override;
    void UpdateRoad(int road) override;
    int CreateRoadJunction(int texture, const SRoadJunctionPoint* point, float halfX, float halfZ, unsigned flags) override;
    void SetRoadJunction(int junction, int texture, const SRoadJunctionPoint* point, float halfX, float halfZ, unsigned flags) override;
    void DestroyRoadJunction(int junction) override;
    void UpdateRoadJunction(int junction) override;

    // ---- Non-virtual entry points for SScene (agent A); see the header comment ----
    void Cull(SViewport* vp);              // 0x6f1180 parcel frustum test (PrepareViewport)
    void MarkAllVisible();                 // 0x6f8610 every parcel visible (RenderScene)
    void Render(SViewport* vp);            // 0x6f2aa0 parcels, roads, map decals, effect decals pass 0
    void RenderShadowPass();               // 0x6f46e0 shadow-buffer and fog passes (Gepard option 2 on)
    void RenderLate(SViewport* vp);        // 0x6f33c0 effect decals pass 1, flora

    // ---- Internals ----
    float HeightAt(float x, float z) const;                 // 0x6f4c00 bilinear, 0 outside
    void  NormalAt(float out[3], int x, int z) const;       // 0x6f4f60
    void  RenderRoads(SViewport* vp, bool shadowPass);      // 0x6f7810 STerrain::RenderRoads
    void  RenderEffectDecals(bool shadowPass, int pass);    // 0x6f6640
    void  RenderFlora(SViewport* vp);                               // 0x6f33c0 flora part
    void  UpdateDecal(int index);                           // 0x6f5cf0
    void  FreeRoadMeshes(SHdHeap<SRoadMesh>* heap);         // 0x6f1ec0
    void  RemoveRoad(int road);                             // 0x6f64c0
    void  RemoveJunction(int junction);                     // 0x6f6560
    bool  ParcelVisibleAt(float x, float z) const;          // 0x69db40 (SPixie copy; the parcel under x,z is visible)

    // --- HD layout (offsets asserted in pzterrain.cpp) ---
    SHdHeap<SEffectDecal>  EffectDecals;   // +0x04
    int                    DecalFormat;    // +0x18 Gepard VB format 0x152
    SHdHeap<SRoad>         Roads;          // +0x1c
    SHdHeap<SRoadJunction> Junctions;      // +0x30
    int                    RoadFormat;     // +0x44 Gepard VB format 0x152 (-1 until the first road)
    SScene*                Scene;          // +0x48
    void*                  Device;         // +0x4c HD Gepard+0x478 (IDirect3DDevice8*); the SWINE device here
    int                    Width;          // +0x50 tiles
    int                    Height;         // +0x54
    int                    WidthP;         // +0x58 Width * P
    int                    HeightP;        // +0x5c
    int                    P;              // +0x60 blockmap sub-resolution (4)
    int                    Stride;         // +0x64 Width + 1
    int                    Tiles;          // +0x68 Width * Height
    int                    Verts;          // +0x6c (Width+1) * (Height+1)
    float*                 Heights;        // +0x70
    float*                 Heights2;       // +0x74
    unsigned char*         Blend;          // +0x78 16 bytes per vertex
    unsigned*              Diffuse;        // +0x7c
    unsigned*              Buffer80;       // +0x80
    float*                 Normals;        // +0x84 3 floats per vertex
    unsigned char*         Shade;          // +0x88 (0x80)
    unsigned*              Buffer8C;       // +0x8c
    SParcel*               ParcelRenderer; // +0x90
    SWireframeParcel*      WireGrid;       // +0x94
    SWireframeParcel*      WireOutline;    // +0x98
    SBlockMapParcel*       BlockMapParcel; // +0x9c
    int                    ParcelsX;       // +0xa0
    int                    ParcelsZ;       // +0xa4
    int                    ParcelCount;    // +0xa8
    SParcelInfo*           Parcels;        // +0xac
    unsigned char*         TileMap;        // +0xb0 2 bytes per parcel
    int                    LayerTextures[60][17];   // +0xb4 [variant][layer]
    int                    LayerFlags[16];          // +0x10a4
    SFloraLayer            Flora[16];               // +0x10e4
    SFloraJitter           FloraJitter[0x1000];     // +0x1924
    bool                   Dirty;          // +0x11924
    unsigned char          _11925[3];
    int                    DirtyX0, DirtyZ0, DirtyX1, DirtyZ1;   // +0x11928
    int                    DrawFlags;      // +0x11938
    unsigned char          SketchState[0x308];   // +0x1193c HD draw-state block (ctor: lighting off, blend mode 2)
    int                    SketchTexture;  // +0x11c44
    int                    SketchWidth;    // +0x11c48
    int                    SketchHeight;   // +0x11c4c
    bool                   Compact;        // +0x11c50
    unsigned char          _11c51[3];
    int                    Overlay;        // +0x11c54
    int                    OverlayMode;    // +0x11c58
    bool                   Flag5C;         // +0x11c5c
    unsigned char          _11c5d[3];
    int                    OverlayFrame;   // +0x11c60
    bool                   BlockMapShown;  // +0x11c64
    unsigned char          _11c65[3];
    unsigned*              BlockMap;       // +0x11c68
    unsigned char*         BlockAux;       // +0x11c6c
    SMapDecal*             MapDecals;      // +0x11c70 SDArray
    int                    MapDecalCount;  // +0x11c74
    int                    MapDecalCap;    // +0x11c78
    int                    ParcelFormat;   // +0x11c7c Gepard VB format 0x152
    int                    LineFormat;     // +0x11c80 Gepard VB format 0x42
    int                    _11c84;         // +0x11c84
};
PZ_HD_SIZE(STerrain, kHdSizeSTerrain);

// ---- HD SHeap<T> operations (alloc 0x6f0dd0 / 0x6f0e90 / 0x6f0f80 / 0x6f1030,
// remove 0x6f6460 / 0x6f64c0 / 0x6f6560). Live entries have Use == 0x7fffffff;
// free entries chain through Use. Growth: 16, then *6/5.
enum { kHeapLive = 0x7fffffff };

template <class T> inline int HdHeapAlloc(SHdHeap<T>* h)
{
    ++h->Count;
    int idx = h->FreeHead;
    if (idx >= 0) {
        h->FreeHead = h->Data[idx].Use;
        memset(&h->Data[idx], 0, sizeof(T));
        h->Data[idx].Use = kHeapLive;
        return idx;
    }
    if (h->Size == h->Capacity) {
        int cap = h->Capacity < 0x10 ? 0x10 : (h->Capacity * 6) / 5;
        h->Data = (T*)realloc(h->Data, cap * sizeof(T));
        memset(h->Data + h->Capacity, 0, (cap - h->Capacity) * sizeof(T));
        h->Capacity = cap;
    }
    h->Data[h->Size].Use = kHeapLive;
    return h->Size++;
}

template <class T> inline bool HdHeapLive(const SHdHeap<T>* h, int idx)
{
    return idx >= 0 && idx < h->Size && h->Data[idx].Use == kHeapLive;
}

template <class T> inline void HdHeapRemove(SHdHeap<T>* h, int idx)
{
    h->Data[idx].Use = h->FreeHead;
    --h->Count;
    h->FreeHead = idx;
}

// Next live index after idx (the HD "for (i = -1; next(i) >= 0;)" walk).
template <class T> inline int HdHeapNext(const SHdHeap<T>* h, int idx)
{
    for (++idx; idx < h->Size; ++idx)
        if (h->Data[idx].Use == kHeapLive)
            return idx;
    return -1;
}

} // namespace pz

#endif // PZ_PZTERRAIN_H
