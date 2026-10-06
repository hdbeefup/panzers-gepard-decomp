// src/3dengine/pz/imesh.h
// SIMesh: the slot layout shared by every HD mesh class, 14 slots:
//   SMesh 0x8830b4, SAnimesh 0x883344, SSkinnedMesh 0x8833f0 (agent A, the
//   meshes inside an SModel) and SParcel 0x8904a4, SParcel2 0x890558,
//   SBlockMapParcel 0x89051c, SWireframeParcel 0x8904e0 (agent B, terrain
//   parcels; they inherit every slot but +0x00 from SMesh).
// No RTTI interface class exists for it in HD (SMesh is the root); the world
// never calls these, so this header only fixes the order for agents A and B.
//
// SHARED HEADER (owner P0): same rules as iscene.h. Addresses are SMesh's;
// the other classes' overrides are listed where they differ.

#ifndef PZ_IMESH_H
#define PZ_IMESH_H

#include "pzcommon.h"

namespace pz {

struct SIMesh {
    virtual ~SIMesh() {}                   // +0x00 HD 0x6ccfd0 scalar deleting dtor
    virtual void Draw(int p1) = 0;         // +0x04 HD 0x6cd890 (1) SMesh::Draw; SAnimesh 0x6ceef0 / SSkinnedMesh 0x6d0e10 empty
    virtual void DrawShadow(int p1) = 0;   // +0x08 HD 0x6cdb70 (1) SMesh::DrawShadow
    virtual void Slot_0C() = 0;            // +0x0c HD 0x6ce160 (6) empty in SMesh; SAnimesh 0x6d0400 draw variant
    virtual void Slot_10() = 0;            // +0x10 HD 0x6ce150 (4) empty in SMesh; SAnimesh 0x6cfe00
    virtual void Slot_14() = 0;            // +0x14 HD 0x6ce140 (2) empty in SMesh; SAnimesh 0x6cf9c0
    virtual void Slot_18() = 0;            // +0x18 HD 0x6ce680 (3) empty in SMesh; SSkinnedMesh 0x6d0e30 (1189 insns)
    virtual void Slot_1C() = 0;            // +0x1c HD 0x6cda60 (6) empty in SMesh; SAnimesh 0x6cf500
    virtual void Slot_20() = 0;            // +0x20 HD 0x6cda50 (4) empty in SMesh; SAnimesh 0x6cf140
    virtual void Slot_24() = 0;            // +0x24 HD 0x6cda40 (2) empty in SMesh; SAnimesh 0x6cef00
    virtual void Slot_28() = 0;            // +0x28 HD 0x6ce690 (3) empty in SMesh; SSkinnedMesh 0x6d2200 (1070 insns)
    virtual void CreateVertexBuffer(int p1, int p2) = 0; // +0x2c HD 0x6cd1f0 (2) SMesh/SAnimesh/SSkinnedMesh::CreateVertexBuffer
    virtual void Slot_30() = 0;            // +0x30 HD 0x6ce9c0 (0)
    virtual void Slot_34() = 0;            // +0x34 HD 0x6cea50 (0)
};

} // namespace pz

#endif // PZ_IMESH_H
