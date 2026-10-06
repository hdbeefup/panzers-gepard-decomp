// src/world/blockmap.h
// The SWorld block maps (HD 0x5d8670..0x5da050, 0x5f4430, 0x5f4720,
// 0x6007d0, 0x600870, 0x5ddbc0). OWNER: P.
//
// Two grids at 4x terrain resolution (BlockW = TerrainW * 4):
//   World+0x74ec BlockMap  (u32 per cell): static blocking bits (BLCK chunk,
//                          plus units/buildings marking bits via 0x5f4720).
//   World+0x74f0 BlockMap2 (u8 per cell):  dynamic occupancy counts (units
//                          standing on a cell, 0x5f4430).
// A unit footprint of "size" cells is a disc: cell (i, j) of the size x size
// box is inside when (2i+1-size)^2 + (2j+1-size)^2 < size^2. World
// coordinates map to the box corner as trunc(x * 4 - (size - 1) * 0.5).
//
// These are SWorld methods (__thiscall) in HD; the recompile keeps them as
// free functions on the world so world.h only gains fields.

#ifndef PZ_WORLD_BLOCKMAP_H
#define PZ_WORLD_BLOCKMAP_H

namespace pz {

struct SWorld;

// 0x5da050 SWorld::CheckStaticBlockMapInternal(x, z, size, mask) (3,153 insns:
// unrolled per size 1..16 and 20, 24; other sizes log once and loop). Cell
// coordinates. True = blocked or out of the map.
bool BlockMap_CheckStaticInternal(SWorld* w, int x, int z, int size, unsigned mask);
// 0x5d9e00: world coordinates; size 0 -> false.
bool BlockMap_CheckStatic(SWorld* w, float x, float z, int size, unsigned mask);
// 0x5d9eb0: world coordinates; returns the first blocked cell. A footprint
// that leaves the map returns false. size 0 -> false.
bool BlockMap_CheckStaticCell(SWorld* w, float x, float z, int size, unsigned mask,
                              int* outX, int* outZ);
// 0x5d9c10: dynamic map, cell coordinates (false when the box leaves the map).
bool BlockMap_CheckDynamicInternal(SWorld* w, int x, int z, int size);
// 0x5d99f0: dynamic map, world coordinates.
bool BlockMap_CheckDynamic(SWorld* w, float x, float z, int size);
// 0x5d9a90: dynamic map, world coordinates, first occupied cell.
bool BlockMap_CheckDynamicCell(SWorld* w, float x, float z, int size, int* outX, int* outZ);
// 0x5f4430: dynamic map +1 (on) / -1 (off) over the footprint.
void BlockMap_MarkDynamic(SWorld* w, float x, float z, int size, bool on);
// 0x5f4720: static map |= mask (on) / &= ~mask (off) over the footprint.
void BlockMap_MarkStatic(SWorld* w, float x, float z, int size, bool on, unsigned mask);
// 0x600870: straight line between two cell-space points (floats), stepping
// along the major axis and testing two cells per step. True = free.
bool BlockMap_LineFreeCells(SWorld* w, float x0, float z0, float x1, float z1, int size,
                            unsigned mask);
// 0x6007d0: the same from world coordinates.
bool BlockMap_LineFree(SWorld* w, float x0, float z0, float x1, float z1, int size,
                       unsigned mask);
// 0x5ddbc0: clamps (x, y, z) into [0, TerrainW] x [0, TerrainH]; returns out.
float* World_ClampToMap(SWorld* w, float* out, float x, float y, float z);

} // namespace pz

#endif // PZ_WORLD_BLOCKMAP_H
