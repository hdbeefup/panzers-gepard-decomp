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
    int      _28;                   // +0x28 model +0xc0 argument (0)
    void*    BlockRect;             // +0x2c model +0xa8(4, "Block") result (blockmap; M2)
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
