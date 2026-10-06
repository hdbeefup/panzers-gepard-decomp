// src/3dengine/pz/mesh.h
// HD meshes of a model: SMesh (static, FVF 0x112 / 0x2102), SAnimesh (vertex
// animation, one vertex block per frame) and SSkinnedMesh (FVF 0x4112, CPU
// skinning), their SMaterial (0x638 bytes) and the SRenderPass (0x308 bytes)
// render-state description a material fills and applies. OWNER: agent A.
//
// Vertex shader paths: HD only runs a model mesh through a vertex shader
// when the SGepard vertex shader ids 1 (skinning, +0x64c) or 0x11 (frame
// tweening, +0x68c) exist. Their creator (0x67e5a0, ids 1..0x40 from
// bytecode) has no caller in PANZERS.exe, so in HD both ids stay 0 and every
// model is drawn by the fixed-function / CPU paths lifted here. The VS slots
// (+0x0c..+0x18) are kept as logged no-ops.

#ifndef PZ_MESH_H
#define PZ_MESH_H

#include "pzcommon.h"

struct IDirect3DDevice9;
struct IDirect3DVertexBuffer9;
struct IDirect3DIndexBuffer9;

namespace pz {

struct SScene;

// Per-draw state SModel::Render sets in globals before a mesh draws
// (HD 0x93cec0..0x93ceec); read by SMaterial::Begin.
struct SMeshDrawGlobals {
    int   TexAnimType;     // 0x93cec0  0 none, 1 scroll, 2 rotate
    float TexAnim[3];      // 0x93cec4  u, v, angle
    bool  AlphaOverride;   // 0x93ced0
    float Alpha;           // 0x93ced4
    bool  ColorOverride;   // 0x93ced8  additive tint (TFACTOR, ADD)
    int   Color;           // 0x93cedc
    bool  Color2Override;  // 0x93cee0  modulating tint (TFACTOR, MODULATE)
    int   Color2;          // 0x93cee4
    bool  DeferAlpha;      // 0x93cee8  skip alpha-blended materials (drawn later)
    bool  DrawingDeferred; // 0x93cee9  only alpha-blended materials
    int   FogTint;         // 0x93ceec  fog mode 3 colour (0 = none)
};
extern SMeshDrawGlobals g_MeshDraw;

// HD render pass (0x308 bytes): reset 0x6883a0, ctor 0x687730, apply
// 0x687a50 (diffed against the last applied pass, HD globals 0x92f0a8..).
struct SRenderPass {
    int           VertexShader;       // +0x000
    bool          Lighting;           // +0x004
    unsigned char _005[3];
    float         Material[17];       // +0x008 D3DMATERIAL9
    bool          MaterialDirty;      // +0x04c
    unsigned char _04d[3];
    int           AmbientSource;      // +0x050 RS 147
    int           DiffuseSource;      // +0x054 RS 145
    int           SpecularSource;     // +0x058 RS 146
    int           EmissiveSource;     // +0x05c RS 148
    bool          SourceDirty;        // +0x060
    unsigned char _061[3];
    struct Transform {                // +0x064, 5 stages of 0x48
        int   TexCoordIndex;          // TSS 11
        int   TransformFlags;         // TSS 24
        float Matrix[16];             // D3DTS_TEXTURE0 + stage
    } Transforms[5];
    unsigned      TransformMask;      // +0x1cc
    int           ShadeMode;          // +0x1d0 RS 9
    int           CullMode;           // +0x1d4 RS 22
    struct Sampler {                  // +0x1d8, 5 stages of 0xc
        int Texture;                  // facade texture handle, -1 none
        int AddressU;                 // SAMP 1
        int AddressV;                 // SAMP 2
    } Samplers[5];
    unsigned      SamplerMask;        // +0x214
    int           PixelShader;        // +0x218
    int           TextureFactor;      // +0x21c RS 60
    struct Stage {                    // +0x220, 5 stages of 0x20
        int  ColorOp, ColorArg1, ColorArg2, ColorArg0;
        int  AlphaOp, AlphaArg1, AlphaArg2;
        bool ResultTemp;
        unsigned char _1d[3];
    } Stages[5];
    unsigned      StageMask;          // +0x2c0
    unsigned char ColorWrite;         // +0x2c4 RS 168
    bool          AlphaBlend;         // +0x2c5 RS 27
    unsigned char _2c6[2];
    int           SrcBlend;           // +0x2c8 RS 19
    int           DestBlend;          // +0x2cc RS 20
    bool          AlphaTest;          // +0x2d0 RS 15
    unsigned char AlphaRef;           // +0x2d1 RS 24
    unsigned char _2d2[2];
    int           AlphaFunc;          // +0x2d4 RS 25
    bool          ZEnable;            // +0x2d8 RS 7
    unsigned char _2d9[3];
    int           ZFunc;              // +0x2dc RS 23
    bool          ZWrite;             // +0x2e0 RS 14
    bool          Stencil;            // +0x2e1 RS 52
    unsigned char _2e2[2];
    int           StencilState[7];    // +0x2e4 RS 53..59
    int           FogMode;            // +0x300 0 scene fog, 1 black fog, 2 off, 3 tinted
    unsigned      FogColor;           // +0x304

    void Init();                                          // 0x687730
    void Reset();                                         // 0x6883a0
    void Apply();                                         // 0x687a50
    void SetTexture(unsigned stage, int texture, bool wrap);            // 0x688d80
    void SetTexCoordIndex(unsigned stage, int index);                   // 0x688c40
    void SetTexTransform(unsigned stage, int index, int flags, const float* m16); // 0x688c70
    void SetTexMatrix(unsigned stage, int flags, const float* m16);     // 0x688cd0
    void SetColorOp(unsigned stage, int op, int arg1, int arg2, int arg0); // 0x6888f0
    void SetAlphaOp(unsigned stage, int op, int arg1, int arg2);        // 0x688870
    void SetAlphaMode(int mode);                                        // 0x6887d0 (0 opaque, 1 test, 2 blend)
    void SetBlendMode(int mode, int texture);                           // 0x688980
    void SetFogMode(int mode, unsigned color);                          // 0x688aa0
};
static_assert(sizeof(SRenderPass) == 0x308, "HD render pass is 0x308 bytes");

// Forget the last applied pass (HD resets the 0x92f0a8 cache on device
// reset). Called at the start of every HD scene pass, because the SWINE
// board changes device state between our frames.
void InvalidateRenderPassCache();

// Scene fog for pass fog mode 0 (HD 0x688ac0 on the 0x92f0a8 cache).
void SetPassFog(float start, float end, unsigned color);
bool GetPassFog(float* start, float* end, float* start2, float* end2, float* inv);

// HD SMaterial (0x638 bytes, ctor 0x6ccd70 -> 0x6cbd50).
struct SMaterial {
    int         Textures[5];   // +0x000 DIFF, SPEC, SILL, BUMP, REFL
    int         PassCount;     // +0x014
    SRenderPass Passes[2];     // +0x018
    int         PrimCount;     // +0x628
    int         MinIndex;      // +0x62c
    int         NumVertices;   // +0x630
    int         PrimType;      // +0x634 4 = MATE (list), 5 = STRP (strip)

    void Init();                                        // 0x6cbd50
    void Release();                                     // 0x6cced0
    void SetTexture(unsigned slot, int texture);        // 0x6cccc0
    void Begin(SScene* scene, int type);                // 0x6cbe00
    void BeginPass(int pass) { Passes[pass].Apply(); }  // 0x6ccae0
    void SetTexAnim(int slot, int pass, unsigned stage);// 0x6ccb20
    void SetupReflectionPass(int pass);                 // 0x6cc9e0
};
static_assert(sizeof(SMaterial) == 0x638, "HD material stride 0x638");

// HD SMesh (0x58 bytes, ctor 0x6ccd00, dtor 0x6ccfd0). Not derived from SIMesh:
// agent B's parcels implement SIMesh's untyped placeholders, SMesh keeps the
// same slot order with the HD parameter lists.
struct SMesh {
    IDirect3DDevice9*       Device;        // +0x04
    IDirect3DVertexBuffer9* VertexBuffer;  // +0x08
    unsigned char*          Vertices;      // +0x0c locked data
    int                     VertexCount;   // +0x10
    unsigned                Fvf;           // +0x14 0 when a declaration is needed
    int                     Decl;          // +0x18 -1
    int                     Stride;        // +0x1c
    int                     OffPosition;   // +0x20
    int                     OffNormal;     // +0x24
    int                     OffColor0;     // +0x28
    int                     OffColor1;     // +0x2c
    int                     OffTex0;       // +0x30
    int                     OffTex1;       // +0x34
    int                     OffTex2;       // +0x38
    int                     OffTangents;   // +0x3c (0x2000 tangent frame)
    int                     OffBlend;      // +0x40 (0x4000 indices + weights)
    IDirect3DIndexBuffer9*  IndexBuffer;   // +0x44
    unsigned short*         Indices;       // +0x48 locked data
    int                     IndexCount;    // +0x4c
    SMaterial*              Materials;     // +0x50
    unsigned                MaterialCount; // +0x54

    SMesh();
    // The 14 HD slots in SIMesh order (imesh.h), with HD's parameter lists.
    virtual ~SMesh();                                                // +0x00 0x6ccfd0
    virtual void Draw(SScene* scene);                                // +0x04 0x6cd890
    virtual void DrawShadow(SScene* scene);                          // +0x08 0x6cdb70
    // Shadow-buffer variants of +0x1c..+0x28 (SModel::RenderShadow 0x6d9570).
    virtual void DrawShadowFramesBlend(SScene*, int, int, float, int, float) {}   // +0x0c empty
    virtual void DrawShadowFramesLerp(SScene*, int, int, float) {}   // +0x10 empty
    virtual void DrawShadowFrame(SScene*, int) {}                    // +0x14 empty
    virtual void DrawShadowSkinned(SScene*, int, const float*) {}    // +0x18 empty
    virtual void DrawFramesBlend(SScene*, int, int, float, int, float) {}   // +0x1c empty
    virtual void DrawFramesLerp(SScene*, int, int, float) {}         // +0x20 empty
    virtual void DrawFrame(SScene*, int) {}                          // +0x24 empty
    virtual void DrawSkinned(SScene*, int, const float*) {}          // +0x28 empty
    virtual void CreateVertexBuffer(unsigned fvf, int count);        // +0x2c 0x6cd1f0
    virtual void Lock();                                             // +0x30 0x6ce9c0
    virtual void Unlock();                                           // +0x34 0x6cea50

    void SetVertexFormat(unsigned fvf);           // 0x6cd290
    unsigned short* CreateIndexBuffer(int count); // 0x6cd070
    void UnlockIndexBuffer();                     // 0x6cea30
    SMaterial* CreateMaterials(unsigned count);   // 0x6cd0f0
    float* VertexNormal(int i) { return (float*)(Vertices + Stride * i + OffNormal); } // 0x691200
    void DrawDynamic(SScene* scene);              // 0x6cda70
    void DrawShadowDynamic(SScene* scene);        // 0x6ce170
    void DrawShadowMaterials(SScene* scene, bool dynamic);   // 0x6cdb70 / 0x6ce170 material loop
};
static_assert(sizeof(SMesh) == 0x58, "HD operator new(0x58)");

// HD SAnimesh (0x70 bytes, ctor 0x6cea70).
struct SAnimesh : SMesh {
    IDirect3DVertexBuffer9** FrameBuffers;   // +0x58 (VS path only)
    int                      FrameBufferCount; // +0x5c
    int                      FrameBufferMax;   // +0x60
    unsigned char**          Frames;         // +0x64 system-memory frames
    int                      FrameCount;     // +0x68
    int                      FrameMax;       // +0x6c

    SAnimesh();
    ~SAnimesh() override;
    void Draw(SScene*) override {}
    void DrawShadow(SScene*) override {}
    void DrawShadowFramesBlend(SScene* scene, int a0, int a1, float t, int b, float w) override;   // 0x6d0400
    void DrawShadowFramesLerp(SScene* scene, int a0, int a1, float t) override;   // 0x6cfe00
    void DrawShadowFrame(SScene* scene, int frame) override;                      // 0x6cf9c0
    bool FillFrame(int frame);                                                    // 0x6cef00 fill
    bool FillFramesLerp(int a0, int a1, float t);                                 // 0x6cf140 fill
    bool FillFramesBlend(int a0, int a1, float t, int b, float w);               // 0x6cf500 fill
    void DrawFramesBlend(SScene* scene, int a0, int a1, float t, int b, float w) override;
    void DrawFramesLerp(SScene* scene, int a0, int a1, float t) override;
    void DrawFrame(SScene* scene, int frame) override;
    void CreateVertexBuffer(unsigned fvf, int count) override;
    void Lock() override;
    void Unlock() override;
};
static_assert(sizeof(SAnimesh) == 0x70, "HD operator new(0x70)");

// HD SSkinnedMesh (0x60 bytes, ctor 0x6d0c30).
struct SSkinnedMesh : SMesh {
    IDirect3DVertexBuffer9* SkinBuffer;   // +0x58 (VS path only)
    unsigned char*          SkinVertices; // +0x5c system memory (CPU skinning)

    SSkinnedMesh();
    ~SSkinnedMesh() override;
    void Draw(SScene*) override {}
    void DrawShadow(SScene*) override {}
    void DrawShadowSkinned(SScene* scene, int bones, const float* matrices) override;   // 0x6d0e30
    void FillSkinned(const float* matrices);                                       // 0x6d2200 fill
    void DrawSkinned(SScene* scene, int bones, const float* matrices) override;
    void CreateVertexBuffer(unsigned fvf, int count) override;
    void Lock() override;
    void Unlock() override;
};
static_assert(sizeof(SSkinnedMesh) == 0x60, "HD operator new(0x60)");

} // namespace pz

#endif // PZ_MESH_H
