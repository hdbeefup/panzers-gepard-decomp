// src/world/doodad.h
// Static map entities of the Panzers world: doodads (DODS), terrain decals
// (DECS) and placed effects (EEFS). OWNER: agent D. Lifted from the HD exe.

#ifndef PZ_DOODAD_H
#define PZ_DOODAD_H

#include <stddef.h>
#include "string2.h"

struct SStream;

namespace pz {

struct SIModel;
struct SBlockBitmap;   // blockmaprefresh.h

// HD SDoodad: the payload of a World+0x140 heap element (element stride 200,
// so 0xc4 bytes after the heap link).
struct SDoodad {
    SString  Name;                  // +0x00 .4d file
    float    X;                     // +0x08
    float    YOffset;               // +0x0c above the terrain
    float    Z;                     // +0x10
    float    Angle;                 // +0x14 model +0x1c SetRotation(angle, tiltX, tiltZ)
    float    TiltX;                 // +0x18
    float    TiltZ;                 // +0x1c
    SIModel* Model;                 // +0x20 scene +0x50 CreateModelFromFile
    SIModel* RuinModel;             // +0x24 "<name>_rom.4d" if it exists
    int      Highlight;             // +0x28 model +0xc0 SetHighlight argument (0)
    SBlockBitmap* BlockRect;        // +0x2c model +0xa8 BuildNodeBlockBitmap(4, "Block"), owned
    unsigned Flags;                 // +0x30 4 obstruction, 8 indestructible, 0x10 demolishable, ...
    unsigned char _34[0x5c - 0x34];
    int      Obstruction;           // +0x5c objects.ini
    int      Indestructible;        // +0x60
    int      Demolishable;          // +0x64
    int      Demolishable2;         // +0x68
    int      DemolishableFence;     // +0x6c
    int      DemolishableWreck;     // +0x70
    int      Cover;                 // +0x74
    int      Scrub;                 // +0x78
    int      Tree;                  // +0x7c
    float    DemolishAngleMax;      // +0x80
    float    DemolishAngleVelocity; // +0x84
    float    DemolishAngleAccel;    // +0x88
    int      DemolishEffect1;       // +0x8c pixie prototype or -1
    int      DemolishEffect2;       // +0x90
    float    DemolishEffectShift;   // +0x94
    float    ScrubTrembling;        // +0x98
    float    ScrubTremblingAtten;   // +0x9c
    int*     Lights;                // +0xa0 SDArray<int> {array, size, max}
    int      LightCount;            // +0xa4
    int      LightMax;              // +0xa8
    int      _ac;                   // +0xac -1 after load
    unsigned char _b0[0xc4 - 0xb0];

    void Load(SStream* s);          // 0x5f1100 DOOD v100
    void Initialize();              // 0x5ee040 SDoodad::Initialize
    void UpdatePosition();          // 0x6012d0
    void Release();                 // 0x5d4e50
};

// HD terrain decal (World+0x73c4 SDArray, element 0x1c).
struct SDecal {
    SString  Name;                  // +0x00 texture
    int      A;                     // +0x08 (DECA ints, passed to terrain +0x48)
    int      B;                     // +0x0c
    int      C;                     // +0x10
    int      Texture;               // +0x14 Gepard +0x44 LoadTexture(name, 1, 1)
    int      _18;                   // +0x18 terrain +0x54 argument

    void Load(SStream* s);          // 0x5f1070 DECA v100
    void Create();                  // 0x5edfd0
    void Release();                 // 0x5d6800 (without the delete)
};

// HD placed effect (World+0x73d0 heap, element 0x2c).
struct SEffectSite {
    SString  Name;                  // +0x00 .fx
    float    X;                     // +0x08
    float    YOffset;               // +0x0c
    float    Z;                     // +0x10
    float    DirX;                  // +0x14
    float    DirZ;                  // +0x18
    int      Prototype;             // +0x1c pixie +0x10 LoadEffectPrototype
    int      Effect;                // +0x20 pixie +0x2c (scene, proto, pos, dir)
    int      _24;                   // +0x24

    void Load(SStream* s);          // 0x5f13e0 EFFE v100
    void Create();                  // 0x5ee9f0
    void UpdatePosition();          // 0x6014e0
};

// ROD2 control point (0x5effd0, element 0x24).
struct SRoadControlPoint {
    float    X;                     // +0x00
    float    Y;                     // +0x04 (not in the file)
    float    Z;                     // +0x08
    float    DirX;                  // +0x0c tangent
    float    DirZ;                  // +0x10
    float    F14;                   // +0x14
    float    Length;                // +0x18 path length at this point (0x601c10)
    int      _1c;
    int      W;                     // +0x20 raw dword; bit 0 extends the road end by 1.5 x tangent
};

// HD road (World+0x73fc heap, element 0x48; ROD2 0x5f0a30).
struct SMapRoad {
    SString  Name;                  // +0x00 texture (no extension: Gepard +0x44 with alpha adds .tga)
    int      Road;                  // +0x08 terrain +0x78 handle, -1
    int      Texture;               // +0x0c Gepard +0x44(name, 1, 1), -1
    void*    TerrainPoints;         // +0x10 SDArray<SRoadPoint> for the terrain
    int      TerrainPointCount;     // +0x14
    int      TerrainPointMax;       // +0x18
    SRoadControlPoint* Points;      // +0x1c SDArray<SRoadControlPoint>
    int      PointCount;            // +0x20
    int      PointMax;              // +0x24
    void*    PathPoints;            // +0x28 SDArray (0x14) Hermite samples for the path finder (M2)
    int      PathPointCount;        // +0x2c
    int      PathPointMax;          // +0x30
    float    Step;                  // +0x34 file; terrain step = Step / 2
    float    TexLength;             // +0x38 file, then texture width * 2 / 64
    float    Width;                 // +0x3c texture height * 2 / 64
    unsigned Flags;                 // +0x40

    void Load(SStream* s);          // 0x5f0a30 element body
    void Build(unsigned flags, bool create);   // 0x601c10 (terrain part)
    void Release();                 // 0x5d5330
};

// HD road junction (World+0x7410 heap, element 0x68; RODJ 0x5f0b60 / 0x5f1590).
struct SMapRoadJunction {
    SString  Name;                  // +0x00 texture
    int      Junction;              // +0x08 terrain +0x8c handle, -1
    int      Texture;               // +0x0c
    float    PointX, PointZ;        // +0x10 SRoadJunctionPoint handed to the terrain
    float    PointDirX, PointDirZ;  // +0x18
    unsigned char PointValid;       // +0x20
    unsigned char _21[3];
    int      _24;
    float    X;                     // +0x28
    float    Y;                     // +0x2c
    float    Z;                     // +0x30
    float    DirX;                  // +0x34
    float    DirZ;                  // +0x38
    float    F3c;                   // +0x3c
    int      _40, _44;
    int      I48;                   // +0x48
    void*    Connections;           // +0x4c SDArray (0x34): road index +0x28, bool +0x30 (path finder, M2)
    int      ConnectionCount;       // +0x50
    int      ConnectionMax;         // +0x54
    float    HalfX;                 // +0x58 file, then texture height / 64
    float    HalfZ;                 // +0x5c file, then texture width / 64
    unsigned Flags;                 // +0x60

    void Load(SStream* s);          // 0x5f1590
    void Build(unsigned flags);     // 0x6029b0 (terrain part)
    void Release();                 // 0x5d5420
};

#if defined(_M_IX86)
static_assert(sizeof(SDoodad) == 0xc4, "DODS element stride 200");
static_assert(offsetof(SDoodad, Model) == 0x20, "0x5ee040 param_1[8]");
static_assert(offsetof(SDoodad, Flags) == 0x30, "0x5ee040 param_1[0xc]");
static_assert(offsetof(SDoodad, Obstruction) == 0x5c, "0x5ee040 param_1[0x17]");
static_assert(offsetof(SDoodad, DemolishEffect1) == 0x8c, "0x5ee040 param_1[0x23]");
static_assert(offsetof(SDoodad, ScrubTremblingAtten) == 0x9c, "0x5ee040 param_1[0x27]");
static_assert(offsetof(SDoodad, LightCount) == 0xa4, "0x5d4e50 param_1[0x29]");
static_assert(offsetof(SDoodad, _ac) == 0xac, "0x5f1100 +0xac");
static_assert(sizeof(SDecal) == 0x1c, "DECS element 0x1c");
static_assert(sizeof(SEffectSite) == 0x28, "EEFS element 0x2c");
static_assert(sizeof(SRoadControlPoint) == 0x24, "ROD2 point 0x24");
static_assert(sizeof(SMapRoad) == 0x44, "ROD2 element 0x48");
static_assert(offsetof(SMapRoad, Step) == 0x34, "0x601c10 +0x34");
static_assert(offsetof(SMapRoad, Flags) == 0x40, "0x5f0a30 elem +0x44");
static_assert(sizeof(SMapRoadJunction) == 0x64, "RODJ element 0x68");
static_assert(offsetof(SMapRoadJunction, X) == 0x28, "0x5f1590 +0x28");
static_assert(offsetof(SMapRoadJunction, Connections) == 0x4c, "0x5eec00 +0x4c");
static_assert(offsetof(SMapRoadJunction, Flags) == 0x60, "0x5f1590 +0x60");
#endif

// Helpers shared by the map readers: HD 0x5625a0 (file part of a path) and
// 0x5e4e00 (directory part, "." when there is none).
void PathFilePart(const SString* path, SString* out);
void PathDirPart(const SString* path, SString* out);
void FreeSString(SString* s);
void SetSString(SString* s, const char* text);
inline const char* SStr(const SString& s) { return s.buf ? s.buf : ""; }

} // namespace pz

#endif // PZ_DOODAD_H
