// src/world/aigroup.h
// pz::SAIGroup: one AI group of the map (AIGP chunk, World+0x4f4 SHeap,
// element 0x54 = Next + 0x50). OWNER: M3-C (layout by sub-agent C4).
//
// HD loads the heap with 0x56e2c0 (gLoadVariables 0x670440 per live element,
// descriptor table 0x8dde98), then 0x601060 per live group. Units join with
// SUnit::SetAIGroup 0x5c0c10 (UNTD "AIGroup" - 1). SGameLogic::Refresh
// 0x576d80 calls SAIGroup::Refresh 0x5f5c70 for every live group on every
// 20th frame (0x577958..0x5779b4, 0x577dc5).
//
// Tactic (+0x08): 0 defend (units walk back to StartPos; "DEFEND SUCCESS"
// once nobody fights), 3 support (repairers / suppliers / medics go to an
// allied group in range that needs them), 4 attack (a random enemy unit by
// the world LCG); other tactics only run the Status 2 / Status 1 parts.
// Status (+0x0c): 0 stand, 1 moving, 2 attacking.

#ifndef PZ_WORLD_AIGROUP_H
#define PZ_WORLD_AIGROUP_H

#include <stddef.h>
#include "string2.h"
#include "world.h"

struct SStream;

namespace pz {

struct SAIGroup {
    SString ID;                     // +0x00 "ID"
    int     Tactic;                 // +0x08 "Tactic"
    int     Status;                 // +0x0c "Status"
    int     Timer;                  // +0x10 "Timer" (Refresh calls, counts down to 0)
    int     AttackMoveTimer;        // +0x14 "AttackMoveTimer" (counts down; not read by Refresh)
    bool    HasCalledForHelp;       // +0x18 "HasCalledForHelp"
    unsigned char _19[3];
    float   StartPos[2];            // +0x1c "StartPos" x, z
    bool    AttackTarget;           // +0x24 "AttackTarget"
    unsigned char _25[3];
    float   TargetPos[2];           // +0x28 "TargetPos" x, z
    float   MaxHelpRange;           // +0x30 "MaxHelpRange" (a vec2 in the descriptor: its second float lands on +0x34)
    int     PathIdx;                // +0x34 "PathIdx" World PATH index (-1)
    SHdArray<int> Units;            // +0x38 world unit indices (SetAIGroup 0x5c0c10)
    SHdArray<int> DisabledAIGroups; // +0x44 "DisabledAIGroups" groups found unreachable

    void Refresh();                                     // 0x5f5c70 (0) (RefreshAI)
    void ComputeStartPos();                             // 0x601060 (0) mean position of the top-level units
    void Release();                                     // 0x5b3380 (0) frees the arrays and the ID
    void AddUnit(int unit);                             // 0x5d9560 (1)
    void RemoveUnit(int unit);                          // 0x5f7990 (1)
    void MoveTo(float x, float z);                      // 0x5f5000 (2) Status 1
    void AttackMoveTo(float x, float z);                // 0x5d95b0 (2) Status 2, AttackTarget
    void MoveAlongPath(int path);                       // 0x5d9750 (1)
    bool IsAlliedWith(int group);                       // 0x5ef440 (1)
    bool CanReach(float x, float z);                    // 0x5e63e0 (2) the biggest unit's 0x5b6e70
    void RefreshSupport();                              // the Tactic 3 / Status 0 part of 0x5f5c70
    int  Strength();                                    // 0x5e74e0 (0) armed units' hit points (squads 100 per member) + bonuses
    int  AntiTankStrength();                            // 0x5e7110 (0) the same for AT weapons (squads: AT or type 3)
};

#if defined(_M_IX86)
static_assert(sizeof(SAIGroup) == 0x50, "AIGP element 0x54 = Next + 0x50");
static_assert(offsetof(SAIGroup, StartPos) == 0x1c, "0x601060 +0x1c");
static_assert(offsetof(SAIGroup, MaxHelpRange) == 0x30, "0x601060 +0x30");
static_assert(offsetof(SAIGroup, PathIdx) == 0x34, "0x5f5c70 +0x34");
static_assert(offsetof(SAIGroup, Units) == 0x38, "0x601060 +0x38");
static_assert(offsetof(SAIGroup, DisabledAIGroups) == 0x44, "descriptor 0x8dde98 +0x44");
static_assert(sizeof(SHeapElem<SAIGroup>) == 0x54, "World+0x4f4 stride 0x54");
#endif

// World+0x4f4 viewed with its element type.
inline SHeap<SAIGroup>& AIGroupHeap(SWorld* w) { return *(SHeap<SAIGroup>*)&w->AIGroups; }

// 0x55ccc0 SHeap<SAIGroup>::operator[] (panics on a dead index).
SAIGroup* AIGroupAt(int index);

void AIGroupsClear(SWorld* w);                         // 0x5636a0 SHeap<SAIGroup>::Clear
void AIGroupsLoad(SWorld* w, ::SStream* s);            // 0x56e2c0 (AIGP), then 0x601060 per live group (0x5f2a50)
void AIGroupsRemove(SWorld* w, int index);             // 0x5be040 SHeap<SAIGroup>::Remove

// SGameLogic::Refresh 0x576d80 (0x577958..0x5779b4): every live group's
// Refresh; the caller tests Frame % 20 == 0.
void RefreshAIGroups();

} // namespace pz

#endif // PZ_WORLD_AIGROUP_H
