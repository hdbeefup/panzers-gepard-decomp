// src/game/target.h
// pz::STarget (HD 0x38 bytes, no vtable), the refcounted order target that
// units and drivers share (SUnit +0x1f4/+0x1f8, SDriver +0xc0/+0xc4/+0xc8).
// OWNER: P. Units create them for their orders (EC_Move: new 0x38 + ctor
// 0x5b27c0); use STarget::Create and AddRef/Release.

#ifndef PZ_GAME_TARGET_H
#define PZ_GAME_TARGET_H

#include "m2common.h"

namespace pz {

// STarget::Type (+0x04).
enum {
    kTargetUnit = 0,         // follow / reach unit +0x08
    kTargetUnitByUnit = 1,   // unit +0x0c, seen from unit +0x08
    kTargetPath = 2,
    kTargetPosDir = 3,       // position + final direction (+0x1c)
    kTargetTurn = 4,         // turn to +0x1c
};

struct STarget {
    int   RefCount;          // +0x00 (0x5b5a30 ++, 0x5bdef0 --, delete at 0)
    int   Type;              // +0x04
    int   Unit;              // +0x08 unit heap index (-1)
    int   Unit2;             // +0x0c unit heap index (-1)
    float Pos[3];            // +0x10 x, y, z
    float Dir;               // +0x1c
    int   Mode;              // +0x20 (1 = stay, read by SDriver::Refresh)
    int   Kind;              // +0x24 ctor argument (order kind)
    int   Field28;           // +0x28
    bool  Reverse;           // +0x2c
    unsigned char _2d[3];
    int   Path;              // +0x30 World PATH heap index (-1)
    int   PathPoint;         // +0x34 current point of that path (-1)

    // new 0x38 + PANZERS 0x5b27c0 (RefCount 0: the creator AddRefs).
    static STarget* Create(int kind);
    void AddRef() { ++RefCount; }                       // PANZERS 0x5b5a30
    void Release();                                     // PANZERS 0x5bdef0
    bool ConsumePath();                                 // PANZERS 0x5b7c50
    bool Refresh(int viewerUnit);                       // PANZERS 0x5bd210
    bool IsUnitOrder() const;                           // PANZERS 0x5bb440
    void SetGroundPos(const float* xz);                 // PANZERS 0x5c1860 (Type 2, y = terrain)
    bool GetPathNodeDir(float* out) const;              // PANZERS 0x5ba0b0
    bool IsKind14() const { return Kind == 0xe; }       // PANZERS 0x5bb6a0
    bool HasPathAhead() const;                          // PANZERS 0x5bb630
};

PZ_HD_SIZE(STarget, kHdSizeSTarget);

// HD "smart pointer" assignment used all over the drivers:
//   if (dst) dst->Release(); if (src) src->AddRef(); dst = src;
// (dst == src is a no-op here; HD would Release first and could free it.)
inline void TargetAssign(STarget*& dst, STarget* src)
{
    if (dst == src)
        return;
    if (dst)
        dst->Release();
    if (src)
        src->AddRef();
    dst = src;
}
inline void TargetClear(STarget*& t)
{
    if (t) {
        t->Release();
        t = nullptr;
    }
}

} // namespace pz

#endif // PZ_GAME_TARGET_H
