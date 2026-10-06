// src/3dengine/pz/iterrain.h
// SITerrain: the HD terrain interface (RTTI SITerrain vftable 0x886b5c,
// STerrain vftable 0x886bfc, 39 slots).
//
// SHARED HEADER (owner P0): same rules as iscene.h. Slot names and parameter
// lists filled in by agent B (the implementer, rule 6.2), order unchanged.
//
// Lifetime: not refcounted. SScene CreateTerrain (+0x64, 0x6a9dd0) does
// new STerrain(scene, w, h, p) (0x11c88 bytes, ctor 0x6efde0) and keeps it at
// SScene+0x1c8; DestroyTerrain (+0x68, 0x6aab20) runs ~STerrain 0x6f06c0 and
// deletes it. Parcels are 8x8 tiles (ctor divides w, h by 8); every parcel
// class (SParcel 0x8904a4, SParcel2 0x890558, SBlockMapParcel 0x89051c,
// SWireframeParcel 0x8904e0) uses the SIMesh slot layout (imesh.h).
//
// World call order (SWorld 0x5f2fc0 TERR, 0x6043a0 after LoadMap):
//   CreateTerrain(w, h, 4) -> Acquire -> HMAP/BLND/DIFF/BLCK/TMAP written into
//   the acquired buffers -> per layer i = 0..16: LoadLayerTexture(i, "tiles/<name>"),
//   for i > 0 LoadFloraLayer(i-1, ...) and SetLayerFlags(i-1, TLAY flags)
//   -> Invalidate(0, 0, w+1, h+1) -> UpdateDecals -> UpdateRoad / UpdateRoadJunction.

#ifndef PZ_ITERRAIN_H
#define PZ_ITERRAIN_H

#include "pzcommon.h"

namespace pz {

// Slot +0x00 (Acquire, 0x6f0d00): HD passes 8 out pointers as 8 arguments.
// The TERR loader 0x5f2fc0 stores them into SWorld; the HD STerrain offsets
// they read are given per member. The recompile passes one struct instead.
// Member names and types are P0's (agent D fills them); meanings by agent B.
struct STerrainBuffers {
    float*          Heights;   // STerrain+0x70 -> SWorld+0xe8   HMAP (w+1)*(h+1) floats
    void*           Buffer74;  // STerrain+0x74 -> SWorld+0xec   (w+1)*(h+1) floats, zeroed; second height field (the second effect-decal pass samples it)
    unsigned char*  Blend;     // STerrain+0x78 -> SWorld+0xf8   BLND, 16 bytes per vertex; byte k = alpha of layer k+1
    float*          Diffuse;   // STerrain+0x7c -> SWorld+0xf4   DIFF (w+1)*(h+1) ARGB vertex colours as u32 (ctor fills 0xffffffff)
    void*           BufferB0;  // STerrain+0xb0 -> SWorld+0xfc   TMAP, 2 bytes per parcel: base layer, texture variant 0..59
    void*           Buffer8C;  // STerrain+0x8c -> SWorld+0x100  (w+1)*(h+1) u32, zeroed (no terrain reader found)
    void*           Buffer11c68; // STerrain+0x11c68 -> SWorld+0x74ec BLCK, (w*p)*(h*p) u32 (ctor 0x2200)
    void*           Buffer11c6c; // STerrain+0x11c6c -> SWorld+0x74f0 (w*p)*(h*p) bytes, zeroed
};

// Road control point as the world hands it to CreateRoad/SetRoad (0x18 bytes;
// world 0x601c10 builds them from the ROD2 points: x, z, unit tangent, a
// "changed" byte and the per-point value at ROD2 point +0x20).
struct SRoadPoint {
    float         X, Z;        // +0x00
    float         DirX, DirZ;  // +0x08 unit tangent
    unsigned char Dirty;       // +0x10 world sets 1; UpdateRoad rebuilds the parcels near dirty points
    unsigned char _11[3];
    float         W;           // +0x14
};
// SDArray<SRoadPoint> {data, size, capacity}.
struct SRoadPointArray {
    SRoadPoint* Data;
    int         Count;
    int         Capacity;
};
// Junction placement (world 0x6029b0, junction +0x10..+0x20): centre and unit direction.
struct SRoadJunctionPoint {
    float         X, Z;
    float         DirX, DirZ;
    unsigned char Valid;       // world sets 1
};

struct SITerrain {
    virtual void Acquire(STerrainBuffers* out) = 0;         // +0x00 HD 0x6f0d00 (8 arg dwords) 8 out pointers (HD passes them as 8 args)
    virtual void LoadLayerTexture(int layer, const char* baseName) = 0; // +0x04 HD 0x6f5510 (2 arg dwords) "<base>_%d_a.tga" else "<base>_%d.tga", %d = 1..6; world 0x608360 passes "tiles/<TLAY name>" for layers 0..16
    virtual void LoadFloraLayer(int layer, const char* floraDir, float scale) = 0; // +0x08 HD 0x6f52b0 (3 arg dwords) up to 32 .4D in "flora/<name>/" -> Gepard LoadModelPrototype; layer = TLAY index - 1; world passes 0.005
    virtual void LoadSketchTexture(const char* file) = 0;   // +0x0c HD 0x6f5740 (1 arg dword) STerrain::LoadSketchTexture
    virtual void SetDrawFlags(int flags) = 0;               // +0x10 HD 0x6f8b80 (1 arg dword) STerrain+0x11938 (ctor 5: 1 draw, 4 layered, 8 no diffuse, 2/0x20 wireframes)
    virtual int GetDrawFlags() = 0;                         // +0x14 HD 0x6f50b0 (0 arg dwords)
    virtual void ComputeSunOcclusion() = 0;                 // +0x18 HD 0x6f1f60 (0 arg dwords) per-vertex shade byte (STerrain+0x88) from the scene sun direction +0x110 (name guessed)
    virtual void SetOverlay(int overlay, int mode) = 0;     // +0x1c HD 0x6ff510 (2 arg dwords) STerrain+0x11c54/+0x11c58, +0x11c60 = scene+0xa0 (compact-mode overlay; name guessed)
    virtual void Invalidate(int x0, int z0, int x1, int z1) = 0; // +0x20 HD 0x6f5230 (4 arg dwords) grows the dirty rect (vertices) that Update rebuilds
    virtual void Update() = 0;                              // +0x24 HD 0x6f8c90 (0 arg dwords) normals, parcel bounds and layer flags of the dirty rect
    virtual void SetFlag5C(bool on) = 0;                    // +0x28 HD 0x6f4b50 (1 arg dword) STerrain+0x11c5c (no reader on the menu path)
    virtual void SetCompactMode(bool on) = 0;               // +0x2c HD 0x6f8680 (1 arg dword) per-parcel SParcel2 objects (debug key toggle 0x5e2d70)
    virtual bool GetCompactMode() = 0;                      // +0x30 HD 0x6f4b70 (0 arg dwords)
    virtual void SetLayerFlags(int layer, int flags) = 0;   // +0x34 HD 0x6f8b60 (2 arg dwords) STerrain+0x10a4[layer]; TLAY flags, 0x80 hides the layer; layer = TLAY index - 1
    virtual void ShowBlockMap(bool on) = 0;                 // +0x38 HD 0x6f8640 (1 arg dword) blockmap overlay (SBlockMapParcel)
    virtual bool IsBlockMapShown() = 0;                     // +0x3c HD 0x6f4b60 (0 arg dwords)
    virtual void ClearBlockMarkers() = 0;                   // +0x40 HD 0x6f85f0 (0 arg dwords) SBlockMapParcel marker array
    virtual void AddBlockMarker(int x, int z, unsigned color) = 0; // +0x44 HD 0x6f10e0 (3 arg dwords) colour | 0x7f000000
    virtual void AddDecal(int index, int texture, int x, int z, int rotation) = 0; // +0x48 HD 0x6f2420 (5 arg dwords) map decal (DECS, world 0x5edfd0); rotation 0..7
    virtual void RemoveDecal(int index) = 0;                // +0x4c HD 0x6f2910 (1 arg dword)
    virtual bool IsInDecal(int index, float x, float z) = 0; // +0x50 HD 0x6f8bd0 (3 arg dwords)
    virtual void SetDecalType(int index, int type) = 0;     // +0x54 HD 0x6f88d0 (2 arg dwords) 0 normal, 1..3 editor highlight colours
    virtual void MoveDecal(int index, int x, int z, int rotation) = 0; // +0x58 HD 0x6f8840 (4 arg dwords)
    virtual void UpdateDecals() = 0;                        // +0x5c HD 0x6f9580 (0 arg dwords) rebuilds every map decal mesh
    virtual int CreateEffectDecal(int texture, float x, float z, float rotation, float scale, unsigned color, int blend, bool flag, bool pass) = 0; // +0x60 HD 0x6f24f0 (9 arg dwords) dynamic decal (alpha 1.0)
    virtual void DestroyEffectDecal(int decal) = 0;         // +0x64 HD 0x6f29b0 (1 arg dword)
    virtual void SetEffectDecalPosition(int decal, float x, float z) = 0; // +0x68 HD 0x6f8900 (3 arg dwords)
    virtual void SetEffectDecalRotation(int decal, float rotation) = 0; // +0x6c HD 0x6f8970 (2 arg dwords)
    virtual void SetEffectDecalTexture(int decal, int texture) = 0; // +0x70 HD 0x6f8ac0 (2 arg dwords)
    virtual void SetEffectDecalAlpha(int decal, float alpha) = 0; // +0x74 HD 0x6f8a50 (2 arg dwords)
    virtual int CreateRoad(int texture, const SRoadPointArray* points, float step, float width, float texLength, unsigned flags) = 0; // +0x78 HD 0x6f2650 (6 arg dwords) world 0x601c10: step = road+0x34 / 2, width = texH / 64, texLength = texW / 64
    virtual void SetRoad(int road, int texture, const SRoadPointArray* points, float step, float width, float texLength, unsigned flags) = 0; // +0x7c HD 0x6f5840 (7 arg dwords)
    virtual void UpdateRoadHeights(int road) = 0;           // +0x80 HD 0x6fcee0 (1 arg dword) re-samples heights and normals of the built road meshes
    virtual void DestroyRoad(int road) = 0;                 // +0x84 HD 0x6f2a30 (1 arg dword)
    virtual void UpdateRoad(int road) = 0;                  // +0x88 HD 0x6f95b0 (1 arg dword) STerrain::UpdateRoad
    virtual int CreateRoadJunction(int texture, const SRoadJunctionPoint* point, float halfX, float halfZ, unsigned flags) = 0; // +0x8c HD 0x6f2710 (5 arg dwords) world 0x6029b0: halfX = texH / 128, halfZ = texW / 128
    virtual void SetRoadJunction(int junction, int texture, const SRoadJunctionPoint* point, float halfX, float halfZ, unsigned flags) = 0; // +0x90 HD 0x6f5b80 (6 arg dwords)
    virtual void DestroyRoadJunction(int junction) = 0;     // +0x94 HD 0x6f2a70 (1 arg dword)
    virtual void UpdateRoadJunction(int junction) = 0;      // +0x98 HD 0x6fd380 (1 arg dword) "UpdateRoadJunction(): Invalid index"

protected:
    ~SITerrain() {}
};

} // namespace pz

#endif // PZ_ITERRAIN_H
