// src/game/rail.h
// The track queries of the trains (agent TR). HD keeps no separate rail
// network: a train runs along a ROD2 road (World+0x73fc, SMapRoad), measured
// by the control points' path length (+0x18, filled by 0x601c10). Between
// two control points the position is the cubic Hermite curve of the points
// and their tangents scaled by the chord length.
//
// Also here: the train's block-map footprint helpers (World+0x74ec block
// bits, World+0x74f0 unit occupancy counters) that take a model "Block"
// bitmap instead of a square.

#ifndef PZ_GAME_RAIL_H
#define PZ_GAME_RAIL_H

namespace pz {

struct SBlockBitmap;

// PANZERS 0x5e7ae0: the path length of the point nearest to pos (x, y, z)
// on road `road` (within 8 units). False when the road is farther.
bool RailProjectOnRoad(const float* pos, int road, float* outDist);
// PANZERS 0x5e8750: the nearest road to pos (within 8 units) and the path
// length there; -1 when none.
int RailFindNearestRoad(const float* pos, float* outDist);
// PANZERS 0x5e98e0 SWorld::GetPositionOnRoad: x, z and the tangent (x, z)
// at path length *dist (clamped to the road; *dist is written back at the
// ends).
void RailGetPositionOnRoad(int road, float* dist, float* outPos2, float* outDir2);
// PANZERS 0x5f54f0 / 0x5f55d0: Hermite point / tangent.
float* RailHermite(float* out2, float p0x, float p0z, float m0x, float m0z,
                   float p1x, float p1z, float m1x, float m1z, float t);
float* RailHermiteTangent(float* out2, float p0x, float p0z, float m0x, float m0z,
                          float p1x, float p1z, float m1x, float m1z, float t);
// PANZERS 0x5e4e90: distance of pt (x, z) to the segment seg (ax, az, bx, bz).
float RailDistToSegment(const float* seg, const float* pt);

// PANZERS 0x5f45f0: World+0x74f0 counters +1 (on) / -1 under the bitmap.
void BlockMap_OccupyBitmap(const SBlockBitmap* bm, bool on);
// PANZERS 0x5d9cf0: true when a counter under the bitmap is non-zero.
bool BlockMap_TestOccupied(const SBlockBitmap* bm);
// PANZERS 0x5dc6b0: true when a block cell under the bitmap has a mask bit.
bool BlockMap_TestBitmap(const SBlockBitmap* bm, unsigned mask);
// HD 0x661b30 + delete 0x1c.
void BlockBitmap_Free(SBlockBitmap* bm);

} // namespace pz

#endif // PZ_GAME_RAIL_H
