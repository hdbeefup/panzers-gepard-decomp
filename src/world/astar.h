// src/world/astar.h
// SAStar and SHeapList (HD 0x5a1300..0x5a3810): the grid A* over the block
// map, and the SWorld accessors 0x5e70e0 / 0x5e7960. OWNER: P.
//
// HD keeps ONE SAStar (new 0xa4, ctor 0x5a1460(BlockW, BlockH)) at
// World+0x74e4, created by SWorld::Initialize. GetGlobalAStar sets its
// iteration limit (+0x90) to 40000, GetLocalAStar to 10000; both return the
// same object. The recompile creates it on the first Get (Initialize leaves
// Obj74e4 null in M1); the map is loaded by then.
//
// Path finding (0x5a3270): cells of the block map, 8 neighbours (table
// 0x8dc1d0), passability 0x5d86e0 = CheckStaticBlockMapInternal(size, mask),
// step cost 0x5d8670, heuristic slot 0 = 0x5a1fd0 (Manhattan * 1.5). Open
// list = SHeapList (0x20-byte items, linear min scan), closed list = SHeap
// (0x14-byte items) plus a W*H index grid (+0x1c; open items are stored as
// index + 0x40000000). The result path (MakePathPointsList 0x5a2020, line
// of sight 0x600870) is an SDArray of (x, z) world points at +0x98.

#ifndef PZ_WORLD_ASTAR_H
#define PZ_WORLD_ASTAR_H

namespace pz {

struct SWorld;
struct SAStar;

SAStar* World_GetGlobalAStar(SWorld* w);       // PANZERS 0x5e70e0
SAStar* World_GetLocalAStar(SWorld* w);        // PANZERS 0x5e7960

// PANZERS 0x5a3270 SAStar::FindPath(unit, &result, x0, z0, x1, z1, size,
// mask, local). result: 0 = no path (or start blocked, or iteration limit),
// 2 = path found (AStar_GetPoint). "unit" is only stored (+0x04).
void AStar_FindPath(SAStar* a, void* unit, int* result, float x0, float z0, float x1, float z1,
                    int size, unsigned mask, bool local);
int AStar_GetPointCount(const SAStar* a);      // +0x9c
const float* AStar_GetPoint(const SAStar* a, int i); // +0x98 [i] (x, z)
void AStar_Reset(SAStar* a);                   // PANZERS 0x5a19a0

} // namespace pz

#endif // PZ_WORLD_ASTAR_H
