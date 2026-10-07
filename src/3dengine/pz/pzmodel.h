// src/3dengine/pz/pzmodel.h
// pz::SModel: HD SModel (0x150 bytes, ctor 0x6d4ae0), created by SScene
// CreateModel*. Two vptrs: SIModel at +0x00, SIAttachable at +0x04 (MSVC
// puts them in declaration order of the bases, as HD does).
// OWNER: agent A.

#ifndef PZ_PZMODEL_H
#define PZ_PZMODEL_H

#include "imodel.h"
#include "pmodel.h"

namespace pz {

struct SScene;
struct SViewport;

// Per-node instance state (HD 0xd4 bytes, ctor 0x6d4e40, set up by
// SModel::Initialize 0x6d82b0).
struct SModelNode {
    float World[12];      // +0x00 3x4 world matrix (current pose)
    float PrevWorld[12];  // +0x30 (pose of the previous tick, "p4" pass)
    float Pos[3];         // +0x60 user local transform (Slot_48 / Slot_4C)
    float Scale;          // +0x6c
    float Quat[4];        // +0x70
    float PrevPos[3];     // +0x80 copied by StoreInterpolationState
    float PrevScale;      // +0x8c
    float PrevQuat[4];    // +0x90
    bool  Visible;        // +0xa0 own flag (SetNodeVisible)
    bool  EffVisible;     // +0xa1 own && parent's
    unsigned char _a2[2];
    int   TexAnimType;    // +0xa4 0 none, 1 scroll, 2 rotate
    float TexAnim[6];     // +0xa8 (u, prevU, v, prevV, angle, prevAngle)
    int   Light;          // +0xc0 scene light index or -1
    int   Effect;         // +0xc4 pixie effect instance or -1
    SIAttachable** Attached; // +0xc8
    int   AttachedCount;  // +0xcc
    int   AttachedMax;    // +0xd0
};
static_assert(sizeof(SModelNode) == 0xd4, "HD node instance stride 0xd4");

// Animation playback state (HD +0xfc..+0x10c, advanced by 0x6dae20).
struct SAnimState {
    int   Seq;        // +0xfc
    float Time;       // +0x100
    int   PrevSeq;    // +0x104 sequence blended out, -1 none
    float PrevTime;   // +0x108
    float Blend;      // +0x10c 0..1 weight of Seq
};

struct SModel : SIModel, SAttachable {
    SModel(SScene* scene, SPModel* proto, SPModel* proto2, bool flag, int index);   // 0x6d4ae0
    ~SModel() override;                                                           // via SIAttachable +0x00

    void AddRef() override;
    void Release() override;
    void Slot_08() override;
    void Slot_0C() override;
    void GetPosition(float* xyz) override;
    void GetRenderPosition(float* xyz) override;
    void SetPosition(float x, float y, float z) override;
    void SetRotation(float angle, float tiltX, float tiltZ) override;
    void SetRotationYawPitchRoll(float pitch, float yaw, float roll) override;
    void RotateAxis(float x, float y, float z, float angle) override;
    void SetScale(float scale) override;
    float GetScale() override;
    void SetVisible(bool show, bool fade) override;
    bool GetVisible() override;
    void SetNodeFade(bool show, int node, int node2) override;
    void StoreInterpolationState() override;
    int FindNode(const char* name) override;
    void Slot_44() override;
    void SetNodeTilt(int node, float x, float y, float z, float yaw, float tiltX, float tiltZ) override;
    void SetNodeRotation(int node, float x, float y, float z, float a, float b) override;
    void GetNodePositionAxis(int node, float* pos, float* axisY) override;
    void GetNodePosition(int node, float* pos) override;
    void GetNodeMatrix(float* m34, int node) override;
    void Slot_5C() override;
    void SetNodeVisible(int node, bool visible) override;
    void SetNodeTexRotation(int node, float u, float v, float angle) override;
    void SetNodeTexScroll(int node, float u, float v) override;
    void PlaySequence(const char* name, bool blend) override;
    void AdvanceAnimation(float seconds) override;
    void AdvanceAnimationByDistance(float distance) override;
    int GetSequenceType() override;
    void Slot_7C() override;
    float GetSequenceLengthAt(int seq) override;
    float GetSequenceLength(const char* name) override;
    float GetSequenceBlendTime(const char* name) override;
    void Slot_8C() override;
    void Slot_90() override;
    void SetFlags(unsigned flags) override;
    void Slot_98() override;
    void Slot_9C() override;
    void Slot_A0() override;
    SBlockBitmap* BuildNodeBlockBitmapAt(int cellsPerUnit, const char* node, const float* pos,
                                         float angle, float tiltX, float tiltZ) override;   // 0x6d61c0
    SBlockBitmap* BuildNodeBlockBitmap(int cellsPerUnit, const char* node) override;
    void GetNodePoints(const char* node, SVec3Array* out) override;   // +0xac 0x6d8090
    SHeightPatch* GetHeightPatch() override;                     // +0xb0 0x6d6700
    void Slot_B4() override;
    void Slot_B8() override;
    void Slot_BC() override;
    void SetHighlight(int mode) override;
    void Slot_C4() override;
    void Slot_C8() override;
    void Slot_CC() override;
    bool HitTestPoint(const float* p) override;                  // +0xd0 0x6db7c0
    void Slot_D4() override;
    bool HitTestSegment(const float* a, const float* b) override; // +0xd8 0x6db550
    void AttachTo(SIModel* parent, int node) override;
    void Slot_E0() override;
    void Slot_E4() override;
    void SetSway(float phase, float p2, float p3) override;
    void SetColor(bool on, unsigned color) override;
    void SetColor2(bool on, unsigned color) override;
    void Slot_F4() override;
    void Slot_F8() override;
    void GetWorldBounds(float* minX, float* maxX, float* minY, float* maxY, float* minZ, float* maxZ) override;
    void GetLogicBoundsXZ(float* minX, float* maxX, float* minZ, float* maxZ) override;   // +0x100 0x6d7150
    void Slot_104() override;

    // SIAttachable (+0x04 vtable 0x883910)
    int Update(int frame, int attachMatrix) override;
    void Attach_08() override;
    void Attach_0C(bool visible) override;

    // Non-virtual HD members.
    void Initialize(SPModel* proto, SPModel* proto2);            // 0x6d82b0
    void SetPrototype(SPModel* proto, SPModel* proto2);          // 0x6d9c20 (SScene::ReplaceModel)
    void Render(SViewport* vp);                                  // 0x6d8830
    void RenderShadow(SViewport* vp);                            // 0x6d9570
    void ComputeNodes(int frame, const float* attach, bool prev); // 0x6dc7b0
    void UpdateFade();                                           // 0x6dc4f0
    void AttachChild(int node, SIAttachable* child);             // 0x6d5940
    void DetachChild(int node, SIAttachable* child);             // 0x6d7340
    SAnimState Advance(const SAnimState& s, float dt) const;     // 0x6dae20
    bool IsVisible() const { return Visible; }                   // +0x34 0x6d86d0

    // --- HD layout (offsets asserted below) ---
    // +0x08 AttachParent, +0x0c AttachNode: the SAttachable base at +0x04.
    int            RefCount;          // +0x10 (ctor sets 1)
    SScene*        Scene;             // +0x14
    void*          Device;            // +0x18 SGepard +0x478
    SPModel*       Proto;             // +0x1c
    SPModel*       Proto2;            // +0x20 low-poly prototype (beyond 40 units)
    SModelNode*    Nodes;             // +0x24
    float          World[12];         // +0x28
    float          PrevWorld[12];     // +0x58
    float          Pos[3];            // +0x88
    float          Scl;               // +0x94
    float          Quat[4];           // +0x98
    float          PrevPos[3];        // +0xa8
    float          PrevScl;           // +0xb4
    float          PrevQuat[4];       // +0xb8
    bool           Dirty;             // +0xc8
    bool           PrevDirty;         // +0xc9
    unsigned char  _ca[2];
    int            Param;             // +0xcc proto +0x18
    int            MainHeap;          // +0xd0 created with flag 0 (doodads; sorted into terrain cells)
    bool           Visible;           // +0xd4 (+0x30 SetVisible)
    unsigned char  _d5[3];
    int            Index;             // +0xd8 scene heap index
    unsigned       Flags;             // +0xdc 1 interpolate pose, 2 interpolate nodes, 4 animate, 0x20 sway, 0x40 deferred alpha, 0x80 fog of war
    int            Frame;             // +0xe0 scene frame of the last Update
    int            ExtraFrame;        // +0xe4 (Slot_58)
    bool           Drawn;             // +0xe8 Render: once per frame
    bool           DrawnDeferred;     // +0xe9
    bool           FlagBit8;          // +0xea shadow drawn this frame (Update: Flags & 0x100 = casts none)
    unsigned char  _eb;
    int            ShadowDecal;       // +0xec terrain decal (Slot_CC texture), -1
    int            ShadowDecal2;      // +0xf0 -1
    bool           Flag;              // +0xf4 CreateModel flag
    unsigned char  _f5[3];
    int            Highlight;         // +0xf8 (Slot_C0): 1..3 fog tint, bit 2 half alpha
    SAnimState     Anim;              // +0xfc
    float          AnimDelta;         // +0x110 accumulated by AdvanceAnimation (flag 4)
    float          SwayPhase;         // +0x114
    float          SwayX;             // +0x118
    float          SwayZ;             // +0x11c
    bool           ColorOverride;     // +0x120 (SetColor)
    unsigned char  _121[3];
    int            Color;             // +0x124
    bool           Color2Override;    // +0x128 (Slot_F0)
    unsigned char  _129[3];
    int            Color2;            // +0x12c
    int            FadeState;         // +0x130 1 in, -1 out, 0 done
    float          FadeAlpha;         // +0x134
    float          FadeStart;         // +0x138
    int            NodeFadeState;     // +0x13c
    float          NodeFadeAlpha;     // +0x140
    float          NodeFadeStart;     // +0x144
    int            NodeFadeNode;      // +0x148
    int            NodeFadeNode2;     // +0x14c
};
PZ_HD_SIZE(SModel, kHdSizeSModel);
static_assert(offsetof(SModel, RefCount) == 0x10, "SModel layout");
static_assert(offsetof(SModel, Scene) == 0x14, "SModel layout");
static_assert(offsetof(SModel, Nodes) == 0x24, "SModel layout");
static_assert(offsetof(SModel, Pos) == 0x88, "SModel layout");
static_assert(offsetof(SModel, Dirty) == 0xc8, "SModel layout");
static_assert(offsetof(SModel, Flags) == 0xdc, "SModel layout");
static_assert(offsetof(SModel, Anim) == 0xfc, "SModel layout");
static_assert(offsetof(SModel, Color2) == 0x12c, "SModel layout");
static_assert(offsetof(SModel, NodeFadeNode2) == 0x14c, "SModel layout");

// 3x4 (row-vector) matrix helpers, HD 0x7c5530 / 0x7c59c0 / 0x6d7a20.
void Mat34Mul(float* out, const float* a, const float* b);           // out = a * b
void Mat34Inverse(float* out, const float* m);
void PoseToMatrix(float* out, const float* pos, float scale, const float* quat);
void Mat34To44(float* out, const float* m);                          // 0x676f60

} // namespace pz

#endif // PZ_PZMODEL_H
