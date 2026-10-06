// src/3dengine/pz/parcel.h
// Terrain parcels (HD SParcel / SParcel2 / SBlockMapParcel / SWireframeParcel,
// all on the SIMesh slot layout) and the fixed-function draw state the terrain
// passes use. OWNER: agent B.
//
// HD draws the terrain with fixed function only (no vertex or pixel shader
// on the PS 2.0 path): one FVF 0x152 (XYZ | NORMAL | DIFFUSE | TEX1, 36
// bytes) through the Gepard dynamic VB and DrawIndexedPrimitive. The
// recompile draws the same vertices with DrawIndexedPrimitiveUP, so nothing
// lives in D3DPOOL_DEFAULT (no device-reset hook yet, docs/MENU3D_INTERFACES.md §7).

#ifndef PZ_PARCEL_H
#define PZ_PARCEL_H

#include <d3d9.h>
#include "imesh.h"

namespace pz {

struct STerrain;

// FVF 0x152 vertex (HD SGepard dynamic VB format created by 0x6787a0(0x152)).
struct STerrainVertex {
    float    X, Y, Z;
    float    NX, NY, NZ;
    unsigned Color;
    float    U, V;
};
static_assert(sizeof(STerrainVertex) == 0x24, "FVF 0x152 is 36 bytes");
enum { kTerrainFVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_DIFFUSE | D3DFVF_TEX1 };   // 0x152

// FVF 0x112 vertex (map decal SMesh, 0x6f5cf0).
struct SDecalVertex {
    float X, Y, Z;
    float NX, NY, NZ;
    float U, V;
};
static_assert(sizeof(SDecalVertex) == 0x20, "FVF 0x112 is 32 bytes");
enum { kDecalFVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1 };   // 0x112

// HD global fog (SGepard DAT_0092f0e0..0x92f0ec): set from the scene fog by
// SScene (agent A); fog modes 0/1 of STerrainDrawState read it.
struct SHdFog {
    bool     Enabled;   // 0x92f0ec
    unsigned Color;     // 0x92f0e0
    float    Start;     // 0x92f0e4
    float    End;       // 0x92f0e8
};
extern SHdFog g_HdFog;

// The per-draw render state block HD keeps on the caller's stack
// (0x308 bytes): reset by 0x687730 (+ defaults 0x6883a0), edited by the
// small setters below, sent to the device by 0x687a50. Only the members the
// terrain passes use are kept; the rest of HD's block (vertex/pixel shader
// index, texture transforms, stencil) stays at the HD defaults.
struct STerrainDrawState {
    struct Stage {
        DWORD ColorOp, ColorArg1, ColorArg2, ColorArg0;   // +0x220 + 0x20*i
        DWORD AlphaOp, AlphaArg1, AlphaArg2;              // +0x230 + 0x20*i
    };
    struct Tex {
        int   Handle;          // +0x1d8 + 0xc*i (SWINE texture index, -1 none)
        DWORD AddrU, AddrV;    // D3DTADDRESS_WRAP 1 / CLAMP 3
    };

    bool         Lighting;          // +0x04 RS LIGHTING (default on)
    D3DMATERIAL9 Material;          // +0x08 diffuse and ambient 1.0
    DWORD        AmbientSource;     // +0x50 RS 147 (1 = COLOR1)
    DWORD        DiffuseSource;     // +0x54 RS 145 (1 = COLOR1)
    DWORD        SpecularSource;    // +0x58 RS 146 (2 = COLOR2)
    DWORD        EmissiveSource;    // +0x5c RS 148 (0 = MATERIAL)
    DWORD        ShadeMode;         // +0x1d0 (GOURAUD)
    DWORD        CullMode;          // +0x1d4 (CW)
    Tex          Textures[2];
    DWORD        TextureFactor;     // +0x21c
    Stage        Stages[2];         // stage 0 MODULATE tex*current / SELECTARG1 tex; stage 1 DISABLE
    BYTE         ColorWrite;        // +0x2c4 (0xf)
    bool         Blend;             // +0x2c5
    DWORD        SrcBlend;          // +0x2c8
    DWORD        DestBlend;         // +0x2cc
    bool         AlphaTest;         // +0x2d0
    BYTE         AlphaRef;          // +0x2d1
    DWORD        AlphaFunc;         // +0x2d4
    bool         ZEnable;           // +0x2d8
    DWORD        ZFunc;             // +0x2dc
    bool         ZWrite;            // +0x2e0
    int          FogMode;           // +0x300: 0 global fog, 1 global fog in black, 2 off, 3 FogColor
    unsigned     FogColor;          // +0x304 (mode 3; alpha byte = density)

    STerrainDrawState() { Reset(); }
    void Reset();                                               // 0x687730 / 0x6883a0
    void SetTexture(unsigned stage, int texture, bool wrap);    // 0x688d80
    void SetTexture(unsigned stage, int texture, bool wrapU, bool wrapV);   // 0x688d20
    void SetColorOp(unsigned stage, DWORD op, DWORD arg1, DWORD arg2, DWORD arg0); // 0x6888f0
    void SetAlphaOp(unsigned stage, DWORD op, DWORD arg1, DWORD arg2);   // 0x688870
    void SetFog(int mode, unsigned color);                      // 0x688aa0
    void SetBlendMode(int mode);                                // 0x6887d0: 0 opaque, 1 alpha test, 2 alpha blend
    void SetBlendModeEx(int mode, int texture);                 // 0x688980: 1 add, 2/3/4 multiply, else by texture alpha
    void Apply(IDirect3DDevice9* dev) const;                    // 0x687a50
};

// Device and texture access shared by the terrain files.
IDirect3DDevice9* TerrainDevice();
int  TerrainTextureAlpha(int texture);                    // 0x67c9e0 (HD texture +0x20: 0 opaque, 1 test, 2 blend)
bool TerrainTextureSize(int texture, int* w, int* h);     // Gepard +0x50
int  TerrainAddRefTexture(int texture);                   // 0x677f20
void TerrainReleaseTexture(int texture);                  // Gepard +0x48
int  TerrainLoadTexture(const char* file, int mipmap, bool alpha);   // Gepard +0x44
int  TerrainOption(unsigned option);                      // Gepard +0x14 GetOption
void TerrainSetWorldIdentity(IDirect3DDevice9* dev);      // 0x680fe0
// Set while an HD render pass (SRenderPass with a pixel shader) draws
// terrain geometry: TerrainDraw then keeps the bound shaders.
extern bool g_TerrainDrawKeepShaders;
void TerrainDraw(IDirect3DDevice9* dev, DWORD fvf, const void* verts, int nverts, int stride,
                 const unsigned short* idx, int nidx);

// Map decal mesh (decal.cpp; HD SMesh FVF 0x112 built by 0x6f5cf0).
struct SDecalMesh;
void DrawDecalMesh(IDirect3DDevice9* dev, const SDecalMesh* mesh);   // 0x6ce6a0 SMesh::Draw

// Shared parcel geometry: 9x9 vertices, 8x8 cells, two triangles per cell
// (v, v+1, v+9) and (v+9, v+1, v+10): the SParcel ctor 0x708180 index list.
extern unsigned short g_ParcelIndices[0x180];

// SParcel (vftable 0x8904a4, 0x5c bytes in HD; ctor 0x708180): the one
// renderer object STerrain keeps at +0x90. DrawLayered and DrawSketch fill
// 81 vertices per call and draw them with the shared index list.
struct SParcel : SIMesh {
    explicit SParcel(STerrain* terrain);   // 0x708180
    ~SParcel() override;

    void Draw(int p1) override;
    void DrawShadow(int p1) override;
    void Slot_0C() override;
    void Slot_10() override;
    void Slot_14() override;
    void Slot_18() override;
    void Slot_1C() override;
    void Slot_20() override;
    void Slot_24() override;
    void Slot_28() override;
    void CreateVertexBuffer(int p1, int p2) override;
    void Slot_30() override;
    void Slot_34() override;

    // 0x708ee0: the 16-layer blend. Base layer opaque, then one alpha pass
    // per present layer above it; vertex alpha = BLND byte.
    void DrawLayered(int x0, int z0, const float* heights, const unsigned* diffuse,
                     const unsigned char* blend, const float* normals, int stride,
                     int baseLayer, const int* textures, const unsigned char* layers,
                     bool shadowPass);
    // 0x70a930: one texture over the whole map (sketch / shadow / fog passes).
    void DrawSketch(int x0, int z0, const float* heights, const unsigned* diffuse,
                    unsigned color, const float* normals, int stride, float uScale,
                    float vScale, bool fixedFunction);

    STerrain* Terrain;    // HD +0x58
};

// SWireframeParcel (vftable 0x8904e0, ctor 0x708400): line lists over a
// parcel (grid or outline) for the editor flags 2 / 0x20.
struct SWireframeParcel : SIMesh {
    SWireframeParcel(STerrain* terrain, bool outline);   // 0x708400
    ~SWireframeParcel() override;

    void Draw(int p1) override;
    void DrawShadow(int p1) override;
    void Slot_0C() override;
    void Slot_10() override;
    void Slot_14() override;
    void Slot_18() override;
    void Slot_1C() override;
    void Slot_20() override;
    void Slot_24() override;
    void Slot_28() override;
    void CreateVertexBuffer(int p1, int p2) override;
    void Slot_30() override;
    void Slot_34() override;

    void DrawWireframe(int x0, int z0, const float* heights, int stride, unsigned color);   // 0x70b050

    STerrain*      Terrain;    // HD +0x58
    bool           Outline;
    unsigned short Indices[0x100];
    int            IndexCount;
};

// SBlockMapParcel (vftable 0x89051c, 0x74 bytes, ctor 0x704e70): the
// blockmap overlay (STerrain+0x9c), (8p+1)^2 vertices per parcel.
struct SBlockMapParcel : SIMesh {
    SBlockMapParcel(STerrain* terrain, int p);   // 0x704e70
    ~SBlockMapParcel() override;

    void Draw(int p1) override;
    void DrawShadow(int p1) override;
    void Slot_0C() override;
    void Slot_10() override;
    void Slot_14() override;
    void Slot_18() override;
    void Slot_1C() override;
    void Slot_20() override;
    void Slot_24() override;
    void Slot_28() override;
    void CreateVertexBuffer(int p1, int p2) override;
    void Slot_30() override;
    void Slot_34() override;

    void DrawParcel(int x0, int z0, const float* heights, int stride,
                    const unsigned* blockmap, const unsigned char* aux, int bmStride);   // 0x709d20

    struct SMarker { int X, Z; unsigned Color; };
    STerrain*       Terrain;     // HD +0x58
    int             P;           // HD +0x5c
    int             NVerts;      // HD +0x60 (8p+1)^2
    int             NTris;       // HD +0x64 p*p*0x80
    SMarker*        Markers;     // HD +0x68 SDArray<0xc>
    int             MarkerCount; // HD +0x6c
    int             MarkerCap;   // HD +0x70
    unsigned short* Indices;
};

// SParcel2 (vftable 0x890558, 0x1c4 bytes, ctor 0x704fd0): a parcel with its
// own prebuilt vertices, made per parcel only in compact mode
// (SetCompactMode, a debug toggle). Kept minimal: it stores what the ctor
// gets and draws the same passes as SParcel::DrawLayered.
struct SParcel2 : SIMesh {
    SParcel2(int x0, int z0, const float* heights, const unsigned char* blend,
             const float* normals, const unsigned* diffuse, int stride, int baseLayer,
             const int* textures, const int* layerFlags, void* overlays, int parcel,
             STerrain* terrain);   // 0x704fd0
    ~SParcel2() override;

    void Draw(int p1) override;
    void DrawShadow(int p1) override;
    void Slot_0C() override;
    void Slot_10() override;
    void Slot_14() override;
    void Slot_18() override;
    void Slot_1C() override;
    void Slot_20() override;
    void Slot_24() override;
    void Slot_28() override;
    void CreateVertexBuffer(int p1, int p2) override;
    void Slot_30() override;
    void Slot_34() override;

    void DrawCompact(bool shadowPass);   // 0x7089e0

    int                  X0, Z0, Stride, BaseLayer, Parcel;
    const float*         Heights;
    const unsigned char* BlendMap;
    const float*         Normals;
    const unsigned*      Diffuse;
    const int*           Textures;
    STerrain*            Terrain;
};

} // namespace pz

#endif // PZ_PARCEL_H
