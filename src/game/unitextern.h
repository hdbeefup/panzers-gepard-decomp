// src/game/unitextern.h
// What the unit code (agent U) needs from the other M2 agents, as one list of
// extern "C" hooks with HD addresses. OWNER of this file: agent U; OWNER of
// each implementation: the agent named on it. Each hook has a weak default in
// unitextern.cpp (MSVC /alternatename) that keeps the menu running until the
// owner defines the real function; defining it anywhere in the link replaces
// the default (no edit of this file needed).
//
// Also the HD layout of STarget as the unit uses it (P owns the class and
// its methods; the unit only allocates, reference-counts and fills targets,
// like SUnit::EC_Move 0x5b8ea0 does).

#ifndef PZ_UNITEXTERN_H
#define PZ_UNITEXTERN_H

#include "m2common.h"

namespace pz {

struct SIUnit;
struct SIUnitAnimation;
struct STarget;

// HD STarget (0x38 bytes, no vtable, ctor 0x5b27c0(kind)). Offsets used by
// the unit code; the accessors stay valid when P's target.h lands.
namespace tgt {
enum {
    kRefCount = 0x00,   // released with the sized delete (0x76654a(p, 0x38)) at 0
    kType     = 0x04,   // 0 follow unit, 2 move to position, 3 position + p7
    kUnit     = 0x08,   // world index (-1)
    kUnit2    = 0x0c,   // (-1)
    kPos      = 0x10,   // x, y (terrain height), z
    kP1C      = 0x1c,
    kP20      = 0x20,   // SetCurrentTarget sets 1 for kinds 2/3 with a gunner
    kKind     = 0x24,   // ctor argument
    kP28      = 0x28,
    kFlag2C   = 0x2c,   // byte (Stop: unit +0xcc)
    kPath     = 0x30,   // PATH index (-1); STarget::ConsumePath 0x5b7c50
    kPathPt   = 0x34,   // current path point (-1)
};
inline int&   I(STarget* t, int off) { return *(int*)((unsigned char*)t + off); }
inline float& F(STarget* t, int off) { return *(float*)((unsigned char*)t + off); }
inline unsigned char& B(STarget* t, int off) { return *((unsigned char*)t + off); }
} // namespace tgt

} // namespace pz

extern "C" {

// ---- P (drivers, path finding, block map, STarget) ----------------------
// new STarget (operator new 0x38 + ctor 0x5b27c0(kind)); RefCount 0.
pz::STarget* PzTargetNew(int kind);
// Drops one reference; deletes at 0 (HD inline: --RefCount, 0x76654a(p, 0x38)).
void PzTargetRelease(pz::STarget* t);
// STarget::ConsumePath 0x5b7c50: next point of PATH +0x30 into +0x10..+0x18.
bool PzTargetConsumePath(pz::STarget* t);
// STarget::Refresh 0x5bd210 (ServerRefresh: false = the target is gone/done).
bool PzTargetRefresh(pz::STarget* t, int unitIndex);
// SWorld 0x5f4430: put / take a unit footprint (x, z, size in blocks) on the
// dynamic block map (unit +0x198 SetOnBlockMap).
void PzBlockMapSetUnit(float x, float z, int size, bool on);
// SWorld 0x5f4720 (unit +0x1a0 MarkBlockMap).
void PzBlockMapMark(int p1, int p2, int size, bool on, short flags);
// SWorld 0x5d9eb0 (unit +0x1a4 TestBlockMapPath) / 0x5d9e00 (+0x1a8 TestBlockMap).
int PzBlockMapTestPath(int p1, int p2, int p4, short flags, int p6, int p7);
int PzBlockMapTest(int p1, int p2, int size, short flags);
// SWorld 0x5d99f0 (unit +0x19c, FindEmptySpace): true when units stand on
// the footprint (x, z, size) (name guessed).
bool PzBlockMapTestUnits(float x, float z, int size);
// SDriver 0x55bf50 (Stop: reset the driver, drop its targets).
void PzDriverReset(void* driver, bool dropTargets);

// ---- L (logic) ------------------------------------------------------------
// SGameLogic 0x565530 (SWorld::CreateUnit 0x5e3170 for top-level units).
void PzGameLogicUnitCreated(int unitIndex);

} // extern "C"

#endif // PZ_UNITEXTERN_H
