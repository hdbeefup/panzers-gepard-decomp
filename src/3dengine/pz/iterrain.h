// src/3dengine/pz/iterrain.h
// SITerrain: the HD terrain interface (RTTI SITerrain vftable 0x886b5c,
// STerrain vftable 0x886bfc, 39 slots).
//
// SHARED HEADER (owner P0): same rules as iscene.h.
//
// Lifetime: not refcounted. SScene CreateTerrain (+0x64, 0x6a9dd0) does
// new STerrain(scene, w, h, p) (0x11c88 bytes, ctor 0x6efde0) and keeps it at
// SScene+0x1c8; DestroyTerrain (+0x68, 0x6aab20) runs ~STerrain 0x6f06c0 and
// deletes it. Parcels are 8x8 tiles (ctor divides w, h by 8); every parcel
// class (SParcel 0x8904a4, SParcel2 0x890558, SBlockMapParcel 0x89051c,
// SWireframeParcel 0x8904e0) uses the SIMesh slot layout (imesh.h).

#ifndef PZ_ITERRAIN_H
#define PZ_ITERRAIN_H

#include "pzcommon.h"

namespace pz {

// Slot +0x00 (Acquire, 0x6f0d00): HD passes 8 out pointers as 8 arguments.
// The TERR loader 0x5f2fc0 stores them into SWorld; the HD STerrain offsets
// they read are given per member. The recompile passes one struct instead.
struct STerrainBuffers {
    float*          Heights;   // STerrain+0x70 -> SWorld+0xe8   HMAP (w+1)*(h+1) floats
    void*           Buffer74;  // STerrain+0x74 -> SWorld+0xec
    unsigned char*  Blend;     // STerrain+0x78 -> SWorld+0xf8   BLND, 16 layers, stride 0x10
    float*          Diffuse;   // STerrain+0x7c -> SWorld+0xf4   DIFF (w+1)*(h+1) floats
    void*           BufferB0;  // STerrain+0xb0 -> SWorld+0xfc
    void*           Buffer8C;  // STerrain+0x8c -> SWorld+0x100
    void*           Buffer11c68; // STerrain+0x11c68 -> SWorld+0x74ec
    void*           Buffer11c6c; // STerrain+0x11c6c -> SWorld+0x74f0
};

struct SITerrain {
    virtual void Acquire(STerrainBuffers* out) = 0;         // +0x00 HD 0x6f0d00 (8 arg dwords) 8 out pointers (HD passes them as 8 args)
    virtual void Slot_04_LoadLayerTexture(int layer, int p2) = 0; // +0x04 HD 0x6f5510 (2 arg dwords) "_%d_a.tga" / "_%d.tga" (name guessed)
    virtual void Slot_08() = 0;                             // +0x08 HD 0x6f52b0 (3 arg dwords)
    virtual void LoadSketchTexture(const char* file) = 0;   // +0x0c HD 0x6f5740 (1 arg dword) STerrain::LoadSketchTexture
    virtual void Slot_10() = 0;                             // +0x10 HD 0x6f8b80 (1 arg dword)
    virtual void Slot_14() = 0;                             // +0x14 HD 0x6f50b0 (0 arg dwords)
    virtual void Slot_18() = 0;                             // +0x18 HD 0x6f1f60 (0 arg dwords)
    virtual void Slot_1C() = 0;                             // +0x1c HD 0x6ff510 (2 arg dwords)
    virtual void Slot_20() = 0;                             // +0x20 HD 0x6f5230 (4 arg dwords)
    virtual void Slot_24() = 0;                             // +0x24 HD 0x6f8c90 (0 arg dwords)
    virtual void Slot_28() = 0;                             // +0x28 HD 0x6f4b50 (1 arg dword)
    virtual void Slot_2C() = 0;                             // +0x2c HD 0x6f8680 (1 arg dword)
    virtual void Slot_30() = 0;                             // +0x30 HD 0x6f4b70 (0 arg dwords)
    virtual void Slot_34() = 0;                             // +0x34 HD 0x6f8b60 (2 arg dwords)
    virtual void Slot_38() = 0;                             // +0x38 HD 0x6f8640 (1 arg dword)
    virtual void Slot_3C() = 0;                             // +0x3c HD 0x6f4b60 (0 arg dwords)
    virtual void Slot_40() = 0;                             // +0x40 HD 0x6f85f0 (0 arg dwords)
    virtual void Slot_44() = 0;                             // +0x44 HD 0x6f10e0 (3 arg dwords)
    virtual void Slot_48() = 0;                             // +0x48 HD 0x6f2420 (5 arg dwords)
    virtual void Slot_4C() = 0;                             // +0x4c HD 0x6f2910 (1 arg dword)
    virtual void Slot_50() = 0;                             // +0x50 HD 0x6f8bd0 (3 arg dwords)
    virtual void Slot_54() = 0;                             // +0x54 HD 0x6f88d0 (2 arg dwords)
    virtual void Slot_58() = 0;                             // +0x58 HD 0x6f8840 (4 arg dwords)
    virtual void Slot_5C() = 0;                             // +0x5c HD 0x6f9580 (0 arg dwords)
    virtual void Slot_60() = 0;                             // +0x60 HD 0x6f24f0 (9 arg dwords)
    virtual void Slot_64() = 0;                             // +0x64 HD 0x6f29b0 (1 arg dword)
    virtual void Slot_68() = 0;                             // +0x68 HD 0x6f8900 (3 arg dwords)
    virtual void Slot_6C() = 0;                             // +0x6c HD 0x6f8970 (2 arg dwords)
    virtual void Slot_70() = 0;                             // +0x70 HD 0x6f8ac0 (2 arg dwords)
    virtual void Slot_74() = 0;                             // +0x74 HD 0x6f8a50 (2 arg dwords)
    virtual void Slot_78() = 0;                             // +0x78 HD 0x6f2650 (6 arg dwords)
    virtual void Slot_7C() = 0;                             // +0x7c HD 0x6f5840 (7 arg dwords)
    virtual void Slot_80() = 0;                             // +0x80 HD 0x6fcee0 (1 arg dword)
    virtual void Slot_84() = 0;                             // +0x84 HD 0x6f2a30 (1 arg dword)
    virtual void UpdateRoad(int road) = 0;                  // +0x88 HD 0x6f95b0 (1 arg dword) STerrain::UpdateRoad
    virtual void Slot_8C() = 0;                             // +0x8c HD 0x6f2710 (5 arg dwords)
    virtual void Slot_90() = 0;                             // +0x90 HD 0x6f5b80 (6 arg dwords)
    virtual void Slot_94() = 0;                             // +0x94 HD 0x6f2a70 (1 arg dword)
    virtual void UpdateRoadJunction(int junction) = 0;      // +0x98 HD 0x6fd380 (1 arg dword) "UpdateRoadJunction(): Invalid index"

protected:
    ~SITerrain() {}
};

} // namespace pz

#endif // PZ_ITERRAIN_H
