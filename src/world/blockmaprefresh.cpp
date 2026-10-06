// src/world/blockmaprefresh.cpp
// Block-map dirty rectangle and rebuild (HD 0x5ef380, 0x5f4910, 0x604620).
// OWNER: P. Raw HD offsets are used for the world parts no other agent has
// named (rivers, lakes, the dirty rectangle).

#include <string.h>
#include "blockmaprefresh.h"
#include "world.h"
#include "worldapi.h"
#include "iunit.h"
#include "logger.h"

namespace pz {

#define W_I(w, off) (*(int*)((char*)(w) + (off)))
#define W_U(w, off) (*(unsigned*)((char*)(w) + (off)))
#define W_B(w, off) (*(unsigned char*)((char*)(w) + (off)))
#define W_P(w, off) (*(void**)((char*)(w) + (off)))

enum {
    kDirty = 0x7500, kX0 = 0x7504, kZ0 = 0x7508, kX1 = 0x750c, kZ1 = 0x7510, kMask = 0x7514,
};

static inline unsigned* Cell(SWorld* w, int x, int z)
{
    return w->BlockMap + (w->BlockW * z + x);
}

// PANZERS 0x5ef380
void BlockMap_MarkDirty(SWorld* w, int x0, int z0, int x1, int z1, unsigned mask)
{
    if (W_B(w, kDirty) == 0) {
        W_U(w, kMask) = mask;
        W_I(w, kX0) = x0;
        W_I(w, kZ0) = z0;
        W_I(w, kX1) = x1;
        W_B(w, kDirty) = 1;
        W_I(w, kZ1) = z1;
        return;
    }
    W_U(w, kMask) |= mask;
    if (x0 < W_I(w, kX0))
        W_I(w, kX0) = x0;
    if (z0 < W_I(w, kZ0))
        W_I(w, kZ0) = z0;
    if (W_I(w, kX1) < x1)
        W_I(w, kX1) = x1;
    if (W_I(w, kZ1) < z1)
        W_I(w, kZ1) = z1;
}

static inline bool BitAt(const SBlockBitmap* bm, int x, int z)
{
    int dx = x - bm->X;
    return ((bm->Bits[(z - bm->Z) * bm->Stride + (dx >> 3)] >> (dx & 7)) & 1) != 0;
}

// PANZERS 0x5f4910
void BlockMap_ApplyBitmap(SWorld* w, const SBlockBitmap* bm, bool on, unsigned mask)
{
    if (!bm)
        return;
    int x0 = bm->X > 0 ? bm->X : 0;
    int x1 = (bm->W + bm->X < w->BlockW) ? bm->W + bm->X : w->BlockW;
    int z0 = bm->Z > 0 ? bm->Z : 0;
    int z1 = (bm->H + bm->Z < w->BlockH) ? bm->H + bm->Z : w->BlockH;
    if (!on) {
        for (int z = z0; z < z1; ++z)
            for (int x = x0; x < x1; ++x)
                if (BitAt(bm, x, z))
                    *Cell(w, x, z) &= ~mask;
        BlockMap_MarkDirty(w, x0, z0, x1, z1, mask);
        return;
    }
    for (int z = z0; z < z1; ++z)
        for (int x = x0; x < x1; ++x)
            if (BitAt(bm, x, z))
                *Cell(w, x, z) |= mask;
}

static bool OutsideDirty(SWorld* w, const SBlockBitmap* bm)
{
    return bm->X > W_I(w, kX1) || bm->Z > W_I(w, kZ1) || bm->W + bm->X < W_I(w, kX0)
        || bm->H + bm->Z < W_I(w, kZ0);
}

// PANZERS 0x604620
void BlockMap_RefreshDirtyRect(SWorld* w)
{
    if (!w->BlockMap || W_B(w, kDirty) == 0)
        return;
    W_B(w, kDirty) = 0;
    if (W_I(w, kX0) < 0)
        W_I(w, kX0) = 0;
    if (W_I(w, kX1) > w->BlockW)
        W_I(w, kX1) = w->BlockW;
    if (W_I(w, kZ0) < 0)
        W_I(w, kZ0) = 0;
    if (W_I(w, kZ1) > w->BlockH)
        W_I(w, kZ1) = w->BlockH;

    // Reset the rebuilt bits (0x2200 are set, the others cleared).
    for (int x = W_I(w, kX0); x < W_I(w, kX1); ++x) {
        for (int z = W_I(w, kZ0); z < W_I(w, kZ1); ++z) {
            *Cell(w, x, z) &= ~(W_U(w, kMask) & 0xffffddffu);
            *Cell(w, x, z) |= W_U(w, kMask) & 0x2200u;
        }
    }

    // Bit 1 on a 192-cell border of big maps.
    if ((W_U(w, kMask) & 1) && w->BlockW > 0x180 && w->BlockH > 0x180) {
        for (int x = 0; x < w->BlockW; ++x)
            for (int z = 0; z < w->BlockH; ++z)
                if (x < 0xc0 || x >= w->BlockW - 0xc0 || z < 0xc0 || z >= w->BlockH - 0xc0)
                    *Cell(w, x, z) |= 1;
    }

    // Terrain layers: a cell takes a layer's bits when its bilinear blend
    // weight (16 bytes per vertex, layer i at byte i - 1) is above 0x2000.
    if ((W_U(w, kMask) & 0x10004502u) && w->Layers.Size > 1) {
        const unsigned char* blend = w->Blend;
        int vw = w->TerrainW + 1;
        for (int i = 1; i < w->Layers.Size; ++i) {
            unsigned attr = w->Layers.Array[i].Attributes;
            if (!(attr & 0x37))
                continue;
            for (int x = W_I(w, kX0); x < W_I(w, kX1); ++x) {
                int xq = x / 4;
                int wx = (x % 4) * 2 + 1;
                int ex = 8 - wx;
                for (int z = W_I(w, kZ0); z < W_I(w, kZ1); ++z) {
                    int zq = z / 4;
                    float wz = (float)((z % 4) * 2 + 1);
                    float ez = 8.0f - wz;
                    const unsigned char* r0 = blend + (vw * zq + xq) * 16 + (i - 1);
                    const unsigned char* r1 = blend + (vw * (zq + 1) + xq) * 16 + (i - 1);
                    float v = (float)(int)(r0[0] * ex) * ez;
                    v = v + (float)(int)(r0[16] * wx) * ez;
                    v = v + (float)(int)(r1[0] * ex) * wz;
                    v = v + (float)(int)(r1[16] * wx) * wz;
                    if ((int)v > 0x2000) {
                        unsigned m = W_U(w, kMask);
                        unsigned* c = Cell(w, x, z);
                        if (attr & 1)
                            *c |= m & 2;
                        if (attr & 2)
                            *c |= m & 0x100;
                        if (attr & 0x10)
                            *c |= m & 0x4000;
                        if (attr & 4)
                            *c |= (m >> 4) & 0x40;
                    }
                }
            }
        }
    }

    // Rivers (World+0x73b0, 0x38 each) and lakes (+0x73e8, 0xa0 each): the
    // scene slots +0xac / +0xc0 that return their cell bounds are not typed
    // in pz/iscene.h yet, so water above ground is not applied.
    if (W_U(w, kMask) & 0x600) {
        int live = 0;
        int* rivers = (int*)W_P(w, 0x73b0);
        for (int i = 0; rivers && i < W_I(w, 0x73b4); ++i)
            if (rivers[i * (0x38 / 4)] == kHeapLive)
                ++live;
        char* lakes = (char*)W_P(w, 0x73e8);
        for (int i = 0; lakes && i < W_I(w, 0x73ec); ++i)
            if (*(int*)(lakes + i * 0xa0) == kHeapLive && *(int*)(lakes + i * 0xa0 + 0x48) > 1)
                ++live;
        if (live) {
            static bool s_logged;
            if (!s_logged && Logger.g) {
                s_logged = true;
                Logger.g->Log(0, "PZM2: block map rebuild skips %d rivers/lakes (scene +0xac/+0xc0 not typed)", live);
            }
        }
        for (int z = W_I(w, kZ0); z < W_I(w, kZ1); ++z)
            for (int x = W_I(w, kX0); x < W_I(w, kX1); ++x)
                *Cell(w, x, z) &= ~0x40u;
    }

    if (W_U(w, kMask) & 0x303c) {
        if (W_U(w, kMask) & 0x2000)
            W_U(w, kMask) |= 0x40;
        // Doodads (World+0x140, 200 bytes each; +0x30 bitmap, +0x34 bits,
        // +0xb0 >= 0 skips) whose bitmap touches the rectangle.
        char* dods = (char*)w->Doodads.Array;
        for (int i = 0; i < w->Doodads.Size; ++i) {
            char* e = dods + i * 200;
            if (*(int*)e != kHeapLive)
                continue;
            SBlockBitmap* bm = *(SBlockBitmap**)(e + 0x30);
            if (!bm || *(int*)(e + 0xb0) >= 0 || OutsideDirty(w, bm))
                continue;
            unsigned* flags = (unsigned*)(e + 0x34);
            if (*flags & 0x2000)
                *flags |= 0x40;
            int x0 = bm->X > 0 ? bm->X : 0;
            int x1 = (bm->W + bm->X < w->BlockW) ? bm->W + bm->X : w->BlockW;
            int z0 = bm->Z > 0 ? bm->Z : 0;
            int z1 = (bm->H + bm->Z < w->BlockH) ? bm->H + bm->Z : w->BlockH;
            for (int z = z0; z < z1; ++z)
                for (int x = x0; x < x1; ++x)
                    if (BitAt(bm, x, z))
                        *Cell(g_World, x, z) |= *flags & W_U(w, kMask);
            *flags &= ~0x40u;
        }
        // Buildings (ClassType 9 with a model): footprints +0x344 (unit
        // +0x1a0 MarkBlockMap), +0x348 (0x8000) and +0x34c (0x800). A
        // footprint outside the rectangle skips the rest of the unit.
        int* units = (int*)W_P(w, 0x4d4);
        for (int i = 0; i < W_I(w, 0x4d8); ++i) {
            if (units[i * 2] != kHeapLive)
                continue;
            char* u = (char*)(size_t)units[i * 2 + 1];
            if (*(int*)(*(char**)(u + 4) + 0x40) != 9 || *(void**)(u + 8) == nullptr)
                continue;
            SBlockBitmap* f1 = *(SBlockBitmap**)(u + 0x344);
            if (f1) {
                if (OutsideDirty(w, f1))
                    continue;
                ((SIUnit*)u)->MarkBlockMap(true, 0, 0, 0, (short)(W_U(w, kMask) & 0x44));
            }
            SBlockBitmap* f2 = *(SBlockBitmap**)(u + 0x348);
            if (f2) {
                if (OutsideDirty(w, f2))
                    continue;
                BlockMap_ApplyBitmap(g_World, f2, true, W_U(w, kMask) & 0x8000);
            }
            SBlockBitmap* f3 = *(SBlockBitmap**)(u + 0x34c);
            if (f3 && !OutsideDirty(w, f3))
                BlockMap_ApplyBitmap(g_World, f3, true, W_U(w, kMask) & 0x800);
        }
    }

    // Bit 0x2000 is cleared next to cells of bit 0x40 (7x7 neighbourhood).
    if (W_U(w, kMask) & 0x2000) {
        int x0 = W_I(w, kX0) > 3 ? W_I(w, kX0) : 3;
        int x1 = W_I(w, kX1) < w->BlockW - 3 ? W_I(w, kX1) : w->BlockW - 3;
        int z0 = W_I(w, kZ0) > 3 ? W_I(w, kZ0) : 3;
        int z1 = W_I(w, kZ1) < w->BlockH - 3 ? W_I(w, kZ1) : w->BlockH - 3;
        for (int z = z0; z < z1; ++z) {
            for (int x = x0; x < x1; ++x) {
                int idx = w->BlockW * z + x;
                unsigned* bmap = w->BlockMap;
                if (bmap[idx] & 0x40)
                    continue;
                for (int dz = -3; dz <= 3; ++dz) {
                    bool hit = false;
                    for (int dx = -3; dx <= 3; ++dx) {
                        if ((dx != 0 || dz != 0) && (bmap[w->BlockW * dz + dx + idx] & 0x40)) {
                            bmap[idx] &= 0xffffdfffu;
                            hit = true;
                            break;
                        }
                    }
                    if (hit)
                        break;
                }
            }
        }
        for (int z = 0; z < w->BlockH; ++z)
            for (int x = 0; x < w->BlockW; ++x)
                *Cell(w, x, z) &= ~0x40u;
        W_U(w, kMask) &= ~0x40u;
    }
}

} // namespace pz
