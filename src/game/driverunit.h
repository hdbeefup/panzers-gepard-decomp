// src/game/driverunit.h
// How the drivers reach the unit, the world and the game logic. OWNER: P.
//
// 1. Unit fields. HD driver code reads and writes SUnit fields directly. The
//    recompile keeps the HD offsets (agent U lays SUnit out at them, rule 3
//    of docs/M2_INTERFACES.md §6), so the drivers address them by offset
//    through the DU_* accessors below and never depend on U's field names.
//    Every offset used is listed in the table below.
// 2. Unit virtuals go through SIUnit (iunit.h).
// 3. Non-virtual SUnit / SGameLogic / SWorld functions owned by U or L go
//    through g_DriverEnv: function pointers with default bodies here, so P
//    builds and runs without L and U. L and U point them at their own
//    functions (same HD semantics) when they land.
//
// Unit fields the drivers use (HD offsets):
//   +0x04 SPUnit*          (+0x40 ClassType, +0x8e flag, +0xec radius)
//   +0x14 SIUnitAnimation* +0x28 active driver index  +0x30 override driver index
//   +0x38/+0x3c SDArray<SDriver*>  +0x44 weapon       +0x54 radius (float)
//   +0x58 small size (int, block map footprint)       +0x5c small size 2
//   +0x74 own heap index   +0x78 boss/owner index     +0x7c bool, +0x80 int
//   +0x8c/+0x90/+0x94 pos  +0xb0 dir   +0xc8 speed    +0xcc reverse
//   +0xcd "path changed"   +0xd0 spin speed           +0xd4 speed (vehicles)
//   +0xd8 u16 block flags (bit 0 = off the static map, 0x800 = on the map)
//   +0xe0 anim state       +0xe4 int   +0xf0/+0xf1 bool  +0xf8 move state
//   +0xfc player           +0x150 dead  +0x168 unplaced
//   +0x17c count of squad members                     +0x1a8/+0x1ac near units
//   +0x1cc/+0x1d0          +0x1d8 SDEQueue<SGhostFrame> GhostFrames (0x18)
//   +0x1f4/+0x1f8 STarget* +0x210..+0x24c running gear state (ghost +0x30)
//   +0x254 movement group  +0x2d8 towed unit index    +0x2e0 int
//   +0x300 (ClassType 10)  +0x308/+0x30c SDArray of points

#ifndef PZ_GAME_DRIVERUNIT_H
#define PZ_GAME_DRIVERUNIT_H

#include "drivertypes.h"

namespace pz {

struct SIUnit;
struct SWorld;
struct STarget;

// Raw HD field access on a unit (or any HD-layout object).
#define DU_F(p, off)  (*(float*)((char*)(p) + (off)))
#define DU_I(p, off)  (*(int*)((char*)(p) + (off)))
#define DU_U(p, off)  (*(unsigned*)((char*)(p) + (off)))
#define DU_B(p, off)  (*(unsigned char*)((char*)(p) + (off)))
#define DU_S(p, off)  (*(unsigned short*)((char*)(p) + (off)))
#define DU_P(p, off)  (*(void**)((char*)(p) + (off)))

inline int DU_ClassType(const void* unit)
{
    return DU_I(DU_P(unit, 0x04), 0x40);
}

// Unit heap (World+0x4d4, SHeapTRB elements {int Next; SUnit*}).
bool    HdUnitLive(int index);              // index valid and live
SIUnit* HdUnit(int index);                  // HD 0x546490 (panics when not live)
SIUnit* HdUnitOrNull(int index);

// World PATH heap (World+0x7494, elements 0x20: +0x10 bool loop,
// +0x14 SVec2* points, +0x18 count). PATH is agent L's loader.
struct SHdPath {
    int    Next;          // +0x00 0x7fffffff = live
    unsigned char _04[0x0c];
    bool   Loop;          // +0x10
    unsigned char _11[3];
    SVec2* Points;        // +0x14
    int    Count;         // +0x18
    int    Max;           // +0x1c
};
SHdPath* HdPath(int index);                 // panics when not live

// The unit ghost-frame queue at +0x1d8 (HD SDEQueue<SGhostFrame>).
struct SGhostQueue {
    SGhostFrame* Array;   // +0x1d8
    int Count;            // +0x1dc
    int Max;              // +0x1e0
    int Base;             // +0x1e4
    int Top;              // +0x1e8
    int Bottom;           // +0x1ec
};
inline SGhostQueue* DU_Ghosts(void* unit) { return (SGhostQueue*)((char*)unit + 0x1d8); }
SGhostFrame* GhostAt(void* unit, int index);  // HD 0x5500a0 (panics out of bottom..top)
int  GhostAddTop(void* unit);                 // HD 0x5b5a40 (grow 0x5ba790), zeroed frame
void GhostClear(void* unit);                  // HD 0x5b7640
void GhostRemoveBottom(void* unit);           // HD 0x5be0b0
void GhostRemoveTop(void* unit);              // HD 0x5be240

// The unit-side ghost operations HD keeps in SUnit (0x5ba430..0x5ba790).
// They walk the towed-unit chain (+0x2d8). U may take them over.
void UnitGhostPush(SIUnit* unit, const SGhostFrame* frame);   // HD 0x5ba430
void UnitGhostClearAll(SIUnit* unit);                         // HD 0x5ba5a0
void UnitGhostRemoveBottomAll(SIUnit* unit);                  // HD 0x5ba610
void UnitGhostRemoveTopAll(SIUnit* unit);                     // HD 0x5ba680
void UnitGhostSetBottomAll(SIUnit* unit, int bottom);         // HD 0x5ba6f0

// Cross-agent functions (defaults in driverunit.cpp; see the header comment).
struct SDriverEnv {
    // L (SGameLogic).
    int     (*GetFrame)();                                   // 0x56d1a0 SGameLogic +0x08
    SIUnit* (*GetMovementGroupBoss)(int group);              // 0x56adc0 (0 = none)
    SIUnit* (*GetMovementGroupSecond)(int group);            // 0x56acf0 (0 = none)
    bool    (*GetMovementGroupConvoy)(int group);            // 0x56af10
    float   (*GetMovementGroupMoveSpeed)(SIUnit* unit);      // 0x56b010
    void    (*GetMovementGroupUnitFormationPos)(float* out, SIUnit* unit); // 0x56b240
    float   (*GetMovementGroupFormationDir)(int group);      // 0x56af90
    void    (*SetMovementGroupFormationDir)(int group, float dir); // 0x57ffc0
    float   (*GetMovementGroupDistance)(SIUnit* unit);       // 0x56b110
    bool    (*CanSeeGroundUnit)(int player, SIUnit* unit);   // 0x562760
    // U (SUnit / SWorld rows of U).
    float*  (*FindEmptySpace)(float* out, float x, float z, float refX, float refZ,
                              int size, int mask, bool flag); // 0x5e5700
    float*  (*FindEmptySpaceDir)(float* out, float x, float z, float dir, int size,
                                 int mask, bool flag);        // 0x5e58d0
    void    (*WorldUnitMoved)(int unitIndex, float wantedSpeed); // 0x5e4870
    float*  (*UnitGetEntrance)(SIUnit* unit, float* out);    // unit vtable +0x78 (buildings)
    void    (*UnitOnDriverReachedTarget)(SIUnit* unit);      // unit vtable +0x48
    void    (*UnitSlot9C)(SIUnit* unit, int p1);             // unit vtable +0x9c
    // unit vtable +0x74 SUnit::GhostFrames_AddTop (9 dwords): from the pulling
    // unit's (x, y, z, dir, +0x300) computes the towed unit's pos/dir/+0x300.
    void    (*UnitTowedFollow)(SIUnit* unit, float x, float y, float z, float dir, float f300,
                               SIUnit* towed, float* outPos, float* outDir, float* out300);
    void    (*UnitOnDriverStucked)(SIUnit* unit);            // 0x5bcd20 SUnit::OnDriverStucked
    bool    (*UnitIsPosInRange)(SIUnit* unit, float x, float y, float z); // 0x5b6fd0 (not run in the menu)
    void*   (*UnitGetAimer)(SIUnit* unit);                   // 0x5b9ce0 (not run; +0x28 = its dir)
    float   (*SquadUnitMoveSpeed)(SIUnit* squad);            // 0x59b990
    void    (*SquadMemberRelativePos)(SIUnit* squad, float* out, int member); // 0x59bff0
    void    (*SquadMembersStep)(SIUnit* squad, float dir);   // 0x5a0450
    void    (*SquadMembersStep2)(SIUnit* squad, float dir);  // 0x5a0060
    // World terrain (M1).
    float   (*TerrainHeight)(float x, float z);              // 0x5e7730
    float   (*WaterHeight)(float x, float z);                // 0x5ec490
};
extern SDriverEnv g_DriverEnv;

// Small SUnit functions on the movement path, lifted here (raw HD fields +
// SIUnit virtuals). U may call them or replace them.
struct SDriver;
SDriver* DrvUnit_ActiveDriver(SIUnit* unit);              // HD 0x5b9bf0
bool DrvUnit_IsMoving(SIUnit* unit);                      // HD 0x5bb500
bool DrvUnit_IsOffStaticMap(SIUnit* unit);                // HD 0x5bb5c0
void DrvUnit_SetMoveState(SIUnit* unit);                  // HD 0x5c10f0
void DrvUnit_ClearNear(SIUnit* unit);                     // HD 0x5b76a0
void DrvUnit_MarkNear(SIUnit* unit, bool on, int except, float x, float z); // HD 0x5b9a20
void DrvUnit_CheckStaticMap(SIUnit* unit);                // HD 0x5c1220
void DrvUnit_CheckOnMap(SIUnit* unit);                    // HD 0x5c1460
void DrvUnit_CopyPosToTowed(SIUnit* unit);                // HD 0x5c2040
void DrvUnit_MoveTowedChain(SIUnit* unit);                // HD 0x5c1f60

// SIUnit block-map slots with float arguments (iunit.h types them as ints).
bool DrvUnit_TestBlockMap(SIUnit* unit, float x, float z, float dir, int size, short mask); // +0x1a8
void DrvUnit_MarkBlockMap(SIUnit* unit, bool on, float x, float z, float dir, short mask);  // +0x1a0

} // namespace pz

#endif // PZ_GAME_DRIVERUNIT_H
