// src/3dengine/pz/pmodel.h
// SPModel: the HD .4d model prototype (0x54 bytes), its nodes, sequences and
// per-node animation channels, plus the SHeap container HD uses for
// prototypes, animations, models and lights. OWNER: agent A.
//
// Loader: SPModel::Load4DFile 0x6912b0 (SCEN v100 and v101),
//         SPModel::LoadSequences 0x693550 (SSQS: SSQE inline sequences and
//         SREF references to CANM .anim files), SortSequences 0x693f60.
// The prototypes live in the Gepard facade heap (HD SGepard +0x550) and are
// referenced by index (SIGepardHD::LoadModelPrototype).

#ifndef PZ_PMODEL_H
#define PZ_PMODEL_H

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include "pzcommon.h"
#include "string2.h"

struct SStream;

namespace pz {

struct SMesh;
struct SITrackPosition;
struct SITrackRotation;
struct STrackLink;
struct SKeyTrackFloat;
struct STrackBool;
struct STrackCameraChange;
struct SCollisionBase;

// HD SHeap<T>: {entries {use, value}, size, max, freeHead, count}. A used
// entry has use == 0x7fffffff; a free one links the next free index.
template <typename T>
struct SHeap {
    struct Entry { int Use; T Value; };
    Entry* Array = nullptr;
    int    Size = 0;
    int    Max = 0;
    int    FreeHead = -1;
    int    Count = 0;

    bool Valid(int i) const { return i >= 0 && i < Size && Array[i].Use == 0x7fffffff; }
    T& operator[](int i) { return Array[i].Value; }
    const T& operator[](int i) const { return Array[i].Value; }
    // 0x6a0de0 / 0x677b20 / 0x677bc0: take the free list head or append.
    int Add()
    {
        ++Count;
        int i = FreeHead;
        if (i >= 0) {
            FreeHead = Array[i].Use;
            Array[i].Use = 0x7fffffff;
            memset(&Array[i].Value, 0, sizeof(T));
            return i;
        }
        if (Size == Max) {
            int n = Max < 0x10 ? 0x10 : (Max * 6) / 5;
            Array = (Entry*)realloc(Array, (size_t)n * sizeof(Entry));
            memset(Array + Max, 0, (size_t)(n - Max) * sizeof(Entry));
            Max = n;
        }
        Array[Size].Use = 0x7fffffff;
        return Size++;
    }
    // SHeap::Remove
    void Remove(int i)
    {
        if (!Valid(i))
            return;
        Array[i].Use = FreeHead;
        FreeHead = i;
        --Count;
    }
    // The "next used index" walk HD writes inline everywhere.
    int Next(int i) const
    {
        for (++i; i < Size; ++i)
            if (Array[i].Use == 0x7fffffff)
                return i;
        return -1;
    }
    void Free() { free(Array); Array = nullptr; Size = Max = Count = 0; FreeHead = -1; }
};

// Per-node animation channel of a sequence (HD 100 bytes, ctor 0x68e2e0).
struct SPChannel {
    float            Matrix[12];   // +0x00 3x4 (from the node, then the CANM NODE)
    float            Pos[3];       // +0x30
    float            Quat[4];      // +0x3c
    SITrackPosition* PosTrack;     // +0x4c CPOS / CXYZ / CPSP
    SITrackRotation* RotTrack;     // +0x50 CROT / CEUL / CEUP
    STrackLink*      Link;         // +0x54 CLNK
    SKeyTrackFloat*     Fov;          // +0x58 CFOV / LINT
    STrackBool*      Visibility;   // +0x5c VISI
    int              Inherit;      // +0x60 INHE (0x38 = inherit only the position)
};
static_assert(sizeof(SPChannel) == 0x64, "HD channel stride 100");

// Sequence (HD 0x38 bytes, ctor 0x68e320, dtor 0x68e7f0).
struct SPSequence {
    SString             Name;          // +0x00
    int                 Unknown08;     // +0x08 SEQ2/SSQE dword; SREF copies SPAnim+8
    float               Speed;         // +0x0c (distance per second, slot +0x74)
    int                 Next;          // +0x10 next sequence: -2 loop, -1 stop, else index
    int                 Type;          // +0x14
    float               BlendTime;     // +0x18 (SEQ2: first frame time)
    float               Length;        // +0x1c (SEQ2: last frame time)
    SPChannel*          Channels;      // +0x20 one per node (skeletal sequences)
    int                 FirstFrame;    // +0x24 (SEQ2)
    int                 FrameCount;    // +0x28 (SEQ2)
    float*              FrameTimes;    // +0x2c (SEQ2)
    STrackCameraChange* CameraChanges; // +0x30 CCHG
    int                 Anim;          // +0x34 SPAnim index (SREF) or -1
};
static_assert(sizeof(SPSequence) == 0x38, "HD sequence stride 0x38");

// Camera node component (HD 0x24 bytes).
struct SPCamera {
    float Data[3];      // +0x00 (+4, +8 scaled)
    bool  HasCmfg;      // +0x0c
    float Cmfg[5];      // +0x10 (+0x1c, +0x20 scaled)
};
static_assert(sizeof(SPCamera) == 0x24, "HD operator new(0x24)");

// Light node component (HD 0x2c bytes): type 1 point, 2 spot, 3 directional.
struct SPLight {
    int   Type;         // +0x00
    float Color[3];     // +0x04
    float Intensity;    // +0x10 (animated by a channel's LINT track)
    float Range;        // +0x14 (* scale)
    float Falloff;      // +0x18
    float Atten1;       // +0x1c (/ scale)
    float Atten2;       // +0x20 (/ scale^2)
    float Theta;        // +0x24 (spot)
    float Phi;          // +0x28 (spot)
};
static_assert(sizeof(SPLight) == 0x2c, "HD operator new(0x2c)");

// Bone of a skinned mesh node (HD 0x34 bytes): node index and the inverse
// bind 3x4 matrix.
struct SPBone {
    int   Node;
    float Matrix[12];
};
static_assert(sizeof(SPBone) == 0x34, "HD bone stride 0x34");

// Model node (HD 0xc4 bytes, SDArray grown by 0x68ead0).
struct SPModelNode {
    SString         Name;        // +0x00
    int             Parent;      // +0x08
    int             Link;        // +0x0c v101 (rotation parent); -1 in v100
    int             Link2;       // +0x10 v101; -1 in v100
    float           Transform[12]; // +0x14 3x4 relative to the parent
    SMesh*          Mesh;        // +0x44 MESH / ANIM / SKIN
    SCollisionBase* Collision;   // +0x48 POLY / BSP_
    SPCamera*       Camera;      // +0x4c CAM_
    SPLight*        Light;       // +0x50 LITE
    float           BBox[24];    // +0x54 BBOX (0x60 bytes)
    int             Effect;      // +0xb4 ':' nodes: pixie effect prototype, else -1
    bool            EffectOnce;  // +0xb8 '::' prefix
    int             BoneCount;   // +0xbc BONS
    SPBone*         Bones;       // +0xc0
};
static_assert(sizeof(SPModelNode) == 0xc4, "HD node stride 0xc4");

struct SPModel {
    SPModelNode* Nodes;          // +0x00
    int          NodeCount;      // +0x04
    int          NodeMax;        // +0x08
    int*         PolyNodes;      // +0x0c nodes with a collision object
    int          PolyNodeCount;  // +0x10
    int          PolyNodeMax;    // +0x14
    int          Param;          // +0x18 (LoadModelPrototype p4)
    SPSequence*  Sequences;      // +0x1c
    int          SequenceCount;  // +0x20
    bool         FrameSequences; // +0x24 SEQS/FRM2 (vertex animation) vs SSQS (skeletal)
    unsigned char _25[3];
    SString      FileName;       // +0x28
    SString      SequenceFile;   // +0x30
    float        Scale;          // +0x38
    bool         FlyZ;           // +0x3c
    unsigned char _3d[3];
    float        Ambient[4];     // +0x40 AMBI (rgb, +0x4c unused)
    int          RefCount;       // +0x50

    explicit SPModel(int param);                                   // 0x68e250
    ~SPModel();                                                    // 0x68e4c0
    bool Load4DFile(const char* dir, const char* file, float scale); // 0x6912b0
    void LoadSequences(SStream* is, const char* dir);              // 0x693550
    void SortSequences();                                          // 0x693f60
    int  AddNode();                                                // 0x68ead0
    SPModelNode* Node(int i) { return &Nodes[i]; }                 // 0x677800
    int  FindSequence(const char* name) const;                    // 0x6d7f70 (on the model)
    void AddRef() { ++RefCount; }                                  // 0x68eb50
};
static_assert(sizeof(SPModel) == 0x54, "HD operator new(0x54)");
static_assert(offsetof(SPModel, Sequences) == 0x1c, "SPModel layout");
static_assert(offsetof(SPModel, FileName) == 0x28, "SPModel layout");
static_assert(offsetof(SPModel, Scale) == 0x38, "SPModel layout");
static_assert(offsetof(SPModel, RefCount) == 0x50, "SPModel layout");

void FreeSequence(SPSequence* s, int nodeCount);   // 0x68e7f0 + channel dtors 0x68e760

// ---- collision objects of POLY / BSP_ nodes ----
// The world reads these for block maps; the 3D path only loads them.
// The AABB (+0x04 sub-object, vftable 0x8834d4: minX, maxX, minY, maxY, minZ,
// maxZ) and the sphere (+0x20, vftable 0x8834bc: centre, radius squared) are
// kept as plain floats; their tests are the static helpers in pmodel.cpp.
struct SCollisionBase {
    virtual ~SCollisionBase() {}
    virtual void Load(SStream* is) = 0;   // +0x04
    virtual bool TestPoint(const float* p) = 0;                       // +0x08 (node space)
    virtual bool TestLineSection(const float* a, const float* b) = 0; // +0x10 (node space)
};

// SCollisionBSPTree (0x38 bytes, ctor 0x6d3490, Load 0x6d37b0).
struct SCollisionBSPTree : SCollisionBase {
    void*  AabbVtbl;       // +0x04 SCollisionAABB sub-object
    float  Aabb[6];        // +0x08
    void*  SphereVtbl;     // +0x20 SCollisionSphere sub-object
    float  Sphere[4];      // +0x24
    void*  Nodes;          // +0x34 0x14 * count
    SCollisionBSPTree();
    ~SCollisionBSPTree() override;
    void Load(SStream* is) override;
    bool TestPoint(const float* p) override;                          // 0x6d4130
    bool TestLineSection(const float* a, const float* b) override;    // 0x6d3c00
    int  FirstHit(const float* a, const float* b, int node, int hit); // 0x6d3e40
};

// SCollisionConvexPoly (0x4c bytes, ctor 0x6d3500, Load 0x6d3950).
struct SCollisionConvexPoly : SCollisionBase {
    void*  AabbVtbl;
    float  Aabb[6];
    void*  SphereVtbl;
    float  Sphere[4];
    float* Verts;          // +0x34
    int    VertCount;      // +0x38
    unsigned short* Indices; // +0x3c
    int    IndexCount;     // +0x40
    void*  Faces;          // +0x44 0x14 * count
    int    FaceCount;      // +0x48
    SCollisionConvexPoly();
    ~SCollisionConvexPoly() override;
    void Load(SStream* is) override;
    bool TestPoint(const float* p) override;                          // 0x6d41d0
    bool TestLineSection(const float* a, const float* b) override;    // 0x6d3c70 (HD panics)
};

} // namespace pz

#endif // PZ_PMODEL_H
