// src/world/blockmaprefresh.h
// The block-map dirty rectangle (World+0x7500..+0x7514) and its rebuild
// (HD 0x5ef380, 0x5f4910, 0x604620). OWNER: P.
//
// Loaders and editors mark a cell rectangle dirty with a mask of the block
// bits to rebuild; SGameLogic's tick (0x576d80 step 3) calls the rebuild,
// which recomputes those bits in the rectangle from the terrain layers (blend
// weights), rivers and lakes (water above ground), doodad block bitmaps and
// building footprints. Fields: +0x7500 bool dirty, +0x7504 x0, +0x7508 z0,
// +0x750c x1, +0x7510 z1 (exclusive), +0x7514 mask.

#ifndef PZ_WORLD_BLOCKMAPREFRESH_H
#define PZ_WORLD_BLOCKMAPREFRESH_H

namespace pz {

struct SWorld;

// HD footprint bitmap (building +0x344/+0x348/+0x34c, doodad +0x30): cell
// rectangle plus one bit per cell.
struct SBlockBitmap {
    int X;                     // +0x00
    int Z;                     // +0x04
    int W;                     // +0x08
    int H;                     // +0x0c
    int Stride;                // +0x10 bytes per row
    int _14;
    unsigned char* Bits;       // +0x18
};

// PANZERS 0x5ef380: unions the rectangle into the dirty one, ORs the mask.
void BlockMap_MarkDirty(SWorld* w, int x0, int z0, int x1, int z1, unsigned mask);
// PANZERS 0x5f4910: sets (on) or clears the mask on the bitmap's cells
// (clearing also marks them dirty).
void BlockMap_ApplyBitmap(SWorld* w, const SBlockBitmap* bm, bool on, unsigned mask);
// PANZERS 0x604620: rebuilds the dirty rectangle (no-op when clean).
void BlockMap_RefreshDirtyRect(SWorld* w);

} // namespace pz

#endif // PZ_WORLD_BLOCKMAPREFRESH_H
