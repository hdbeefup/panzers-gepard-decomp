// src/3dengine/pz/imodel.h
// SIModel: the HD model interface (RTTI SIModel vftable 0x8836dc, SModel
// vftable 0x883800, 66 slots) and SIAttachable, the second base of
// SModel (vftable 0x883910 at SModel+0x04, 4 slots).
//
// SHARED HEADER (owner P0): same rules as iscene.h.
//
// HD layout of SModel (0x150 bytes, ctor 0x6d4ae0(scene, proto, proto2,
// flag, index)): +0x00 SIModel vptr, +0x04 SIAttachable vptr, +0x10 RefCount
// (ctor sets 1), +0x14 SScene*, +0x1c prototype, +0x24 node array (0xd4
// stride), +0x88 position, +0x94 scale, +0x98 rotation quaternion,
// +0xa8..+0xc4 previous-tick pose, +0xc8 dirty word, +0xdc flags.
// Release at 0 calls SIAttachable's scalar deleting dtor with 1.
//
// .4d / .anim prototypes are not interfaces: SGepard owns them in a heap at
// +0x550 (SPModel, 0x54 bytes) and hands out int indices
// (SIGepardHD::LoadModelPrototype). Their loaders are non-virtual:
//   SPModel::Load4DFile    0x6912b0  (file, ?, float scale) MESH/SCEN v100+v101
//   SPModel::LoadSequences 0x693550  (file, ?)
//   SPAnim::LoadAnimFile   0x692d10  (file) CANM v100
// Meshes of a model implement SIMesh (imesh.h).

#ifndef PZ_IMODEL_H
#define PZ_IMODEL_H

#include "pzcommon.h"

namespace pz {

struct SPModel;   // HD 0x54 bytes, owned by the Gepard facade (agent A)
struct SPAnim;    // HD CANM animation prototype (agent A)
struct SBlockBitmap; // HD 0x1c-byte cell bitmap (0x661a50), world/blockmaprefresh.h

struct SIModel {
    virtual void AddRef() = 0;                              // +0x00 HD 0x6d5900 (0 arg dwords) RefCount +0x10
    virtual void Release() = 0;                             // +0x04 HD 0x6d8790 (0 arg dwords) deletes through the SIAttachable base at +0x04
    virtual void Slot_08() = 0;                             // +0x08 HD 0x6d7e90 (0 arg dwords)
    virtual void Slot_0C() = 0;                             // +0x0c HD 0x6d7ea0 (0 arg dwords)
    virtual void GetPosition(float* xyz) = 0;               // +0x10 HD 0x6d7eb0 (1 arg dword) +0x88
    virtual void GetRenderPosition(float* xyz) = 0;         // +0x14 HD 0x6d7950 (1 arg dword) interpolated with scene +0xb8
    virtual void SetPosition(float x, float y, float z) = 0; // +0x18 HD 0x6da8d0 (3 arg dwords) +0x88
    virtual void SetRotation(float angle, float tiltX, float tiltZ) = 0; // +0x1c HD 0x6daab0 (3 arg dwords) quaternion at +0x98 (param names guessed)
    virtual void SetRotationYawPitchRoll(float pitch, float yaw, float roll) = 0; // +0x20 HD 0x6daa60 (3 arg dwords) D3DXQuaternionRotationYawPitchRoll(yaw=p2, pitch=p1, roll=p3)
    virtual void RotateAxis(float x, float y, float z, float angle) = 0; // +0x24 HD 0x6da9e0 (4 arg dwords)
    virtual void SetScale(float scale) = 0;                 // +0x28 HD 0x6dad20 (1 arg dword) +0x94
    virtual float GetScale() = 0;                           // +0x2c HD 0x6d7ee0 (0 arg dwords)
    virtual void SetVisible(bool show, bool fade) = 0;      // +0x30 HD 0x6dafd0 (2 arg dwords) +0xd4 visible; fade = 1 s alpha fade in/out. Walkers: SWalkerAnimation::UpdateModel 0x5ce2a0 calls (1, 0) every tick; ruins (0, 0)
    virtual void Slot_34() = 0;                             // +0x34 HD 0x6d86d0 (0 arg dwords)
    virtual void SetNodeFade(bool show, int node, int node2) = 0; // +0x38 HD 0x6db2a0 (3 arg dwords) fade a node (roof) out / in over 1 s, node2 swaps in
    virtual void StoreInterpolationState() = 0;             // +0x3c HD 0x6da0f0 (0 arg dwords) copies pose to the previous-tick slot; SGameLogic::Refresh per doodad
    virtual int FindNode(const char* name) = 0;             // +0x40 HD 0x6d7b90 (1 arg dword) "Block", "Platform" in SDoodad::Initialize
    virtual void Slot_44() = 0;                             // +0x44 HD 0x6d7c20 (1 arg dword)
    virtual void SetNodeTilt(int node, float x, float y, float z, float yaw, float tiltX, float tiltZ) = 0; // +0x48 HD 0x6da4d0 (7 arg dwords) node user transform: position, yaw, tilt towards (tiltX, tiltZ)
    virtual void SetNodeRotation(int node, float x, float y, float z, float a, float b) = 0; // +0x4c HD 0x6da330 (6 arg dwords) node user transform: position, a about z then b about x (wheels)
    virtual void GetNodePositionAxis(int node, float* pos, float* axisY) = 0; // +0x50 HD 0x6d7d70 (3 arg dwords) translation and second row of the +0x58 matrix (seat facing, SSingleUnit 0x5b02e0)
    virtual void GetNodePosition(int node, float* pos) = 0; // +0x54 HD 0x6d7d30 (2 arg dwords) translation of the +0x58 matrix
    virtual void GetNodeMatrix(float* m34, int node) = 0;   // +0x58 HD 0x6d7c60 (2 arg dwords) node logic-pose matrix (node +0x30), recomputed by 0x6dc7b0(.., 0, 1) when +0xc9 is set; identity for a bad node
    virtual void Slot_5C() = 0;                             // +0x5c HD 0x6d7dd0 (3 arg dwords)
    virtual void SetNodeVisible(int node, bool visible) = 0; // +0x60 HD 0x6da8a0 (2 arg dwords) hides "Block"/"Platform" (decompiler shows 1 arg; RET 8)
    virtual void SetNodeTexRotation(int node, float u, float v, float angle) = 0; // +0x64 HD 0x6da810 (4 arg dwords) texture animation 2 (rotate the UVs)
    virtual void SetNodeTexScroll(int node, float u, float v) = 0; // +0x68 HD 0x6da7a0 (3 arg dwords) texture animation 1 (scroll the UVs; track belts)
    virtual void PlaySequence(const char* name, bool blend) = 0; // +0x6c HD 0x6da200 (2 arg dwords) starts the named sequence (0x6d7f70 lookup); blend keeps the old one for its BlendTime
    virtual void AdvanceAnimation(float seconds) = 0;       // +0x70 HD 0x6da960 (1 arg dword) advances the sequence (accumulated when flag 4 is set)
    virtual void AdvanceAnimationByDistance(float distance) = 0; // +0x74 HD 0x6da920 (1 arg dword) seconds = distance / (sequence Speed * prototype scale)
    virtual int GetSequenceType() = 0;                      // +0x78 HD 0x6d78c0 (0 arg dwords) Type of the playing sequence (1 stand, 2 move, 4/5 spin)
    virtual void Slot_7C() = 0;                             // +0x7c HD 0x6d7850 (1 arg dword)
    virtual float GetSequenceLengthAt(int seq) = 0;         // +0x80 HD 0x6d7f00 (1 arg dword) Length of sequence seq, 0 if out of range
    virtual float GetSequenceLength(const char* name) = 0;  // +0x84 HD 0x6d7f30 (1 arg dword) Length of a named sequence, 0 if the model has none
    virtual float GetSequenceBlendTime(const char* name) = 0; // +0x88 HD 0x6d8010 (1 arg dword) BlendTime (+0x18) of a named sequence, 0 if none
    virtual void Slot_8C() = 0;                             // +0x8c HD 0x6d7e30 (0 arg dwords)
    virtual void Slot_90() = 0;                             // +0x90 HD 0x6d8050 (1 arg dword)
    virtual void SetFlags(unsigned flags) = 0;              // +0x94 HD 0x6da2e0 (1 arg dword) 0x80 / 0xa0 / 0 in SDoodad::Initialize
    virtual void Slot_98() = 0;                             // +0x98 HD 0x6d7910 (0 arg dwords)
    virtual void Slot_9C() = 0;                             // +0x9c HD 0x6dad50 (2 arg dwords)
    virtual void Slot_A0() = 0;                             // +0xa0 HD 0x6d6ce0 (8 arg dwords)
    virtual void Slot_A4() = 0;                             // +0xa4 HD 0x6d61c0 (6 arg dwords)
    virtual SBlockBitmap* BuildNodeBlockBitmap(int cellsPerUnit, const char* node) = 0; // +0xa8 HD 0x6d5ca0 (2 arg dwords) new cell bitmap of the node mesh's triangles projected on XZ (logic pose); null when the node is missing ("Block" in SDoodad::UpdatePosition)
    virtual void Slot_AC() = 0;                             // +0xac HD 0x6d8090 (2 arg dwords)
    virtual void Slot_B0() = 0;                             // +0xb0 HD 0x6d6700 (0 arg dwords)
    virtual void Slot_B4() = 0;                             // +0xb4 HD 0x6d59e0 (1 arg dword)
    virtual void Slot_B8() = 0;                             // +0xb8 HD 0x6d5ae0 (1 arg dword)
    virtual void Slot_BC() = 0;                             // +0xbc HD 0x6dae10 (1 arg dword)
    virtual void SetHighlight(int mode) = 0;                // +0xc0 HD 0x6dad40 (1 arg dword) +0xf8 (SDoodad +0x28)
    virtual void Slot_C4() = 0;                             // +0xc4 HD 0x6d7ef0 (0 arg dwords)
    virtual void Slot_C8() = 0;                             // +0xc8 HD 0x6d7920 (2 arg dwords)
    virtual void Slot_CC() = 0;                             // +0xcc HD 0x6dad80 (1 arg dword)
    virtual void Slot_D0() = 0;                             // +0xd0 HD 0x6db7c0 (1 arg dword)
    virtual void Slot_D4() = 0;                             // +0xd4 HD 0x6db960 (1 arg dword)
    virtual void Slot_D8() = 0;                             // +0xd8 HD 0x6db550 (2 arg dwords)
    virtual void AttachTo(SIModel* parent, int node) = 0;   // +0xdc HD 0x6d5910 (2 arg dwords) parent->AttachChild(node, this) 0x6d5940
    virtual void Slot_E0() = 0;                             // +0xe0 HD 0x6d7310 (0 arg dwords)
    virtual void Slot_E4() = 0;                             // +0xe4 HD 0x6d62e0 (2 arg dwords)
    virtual void SetSway(float phase, float p2, float p3) = 0; // +0xe8 HD 0x6dade0 (3 arg dwords) swaying doodads (0.035, 0.035) (name guessed)
    virtual void Slot_EC() = 0;                             // +0xec HD 0x6da310 (2 arg dwords)
    virtual void SetColor2(bool on, unsigned color) = 0;    // +0xf0 HD 0x6da2c0 (2 arg dwords) second colour override (fog-of-war tint of buildings)
    virtual void Slot_F4() = 0;                             // +0xf4 HD 0x6d7900 (0 arg dwords)
    virtual void Slot_F8() = 0;                             // +0xf8 HD 0x6d85d0 (0 arg dwords)
    virtual void GetWorldBounds(float* minX, float* maxX, float* minY, float* maxY, float* minZ, float* maxZ) = 0; // +0xfc HD 0x6d6f40 (6 arg dwords) world AABB of the mesh-node BBOX corners (tick pose)
    virtual void Slot_100() = 0;                            // +0x100 HD 0x6d7150 (4 arg dwords)
    virtual void Slot_104() = 0;                            // +0x104 HD 0x6d7400 (2 arg dwords)

protected:
    ~SIModel() {}
};

// Second base of SModel (+0x04). Slot +0x00 is the scalar deleting dtor.
struct SIAttachable {
    virtual ~SIAttachable() {}                              // +0x00 HD 0x6d562d (SModel) / 0x6d5710 (SAttachable)
    virtual int Update(int p1, int p2) = 0;                 // +0x04 HD 0x6dba80 (2 arg dwords) SModel::Update 0x6dba80
    virtual void Attach_08() = 0;                           // +0x08 HD 0x6d86e0 (0 arg dwords) empty in SModel
    virtual void Attach_0C() = 0;                           // +0x0c HD 0x6d86f0 (1 arg dword) empty in SModel (RET 4)
};

} // namespace pz

#endif // PZ_IMODEL_H
