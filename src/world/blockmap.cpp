// src/world/blockmap.cpp
// The SWorld block maps (HD 0x5d8670..0x5da050, 0x5f4430, 0x5f4720,
// 0x6007d0, 0x600870, 0x5ddbc0). OWNER: P. Lifted from the HD exe and
// checked against it in the unicorn emulator (see blockmap.h).
//
// Float -> cell conversions: HD computes (float)((double)(v * 4.0f) -
// (double)(size - 1) * 0.5) and stores it with fld/fistp under the control
// word 0x0c7f (truncate), through the global 0x92e350. C's (int) cast of a
// float truncates the same way (cvttss2si; both give 0x80000000 when out of
// range). 0x600870 uses one floor conversion (control word 0x047f) for the
// first step index.

#include <math.h>
#include "blockmap.h"
#include "world.h"
#include "logger.h"

namespace pz {

static inline unsigned* StaticMap(SWorld* w) { return w->BlockMap; }
static inline unsigned char* DynamicMap(SWorld* w) { return (unsigned char*)w->BlockMap2; }

// HD: (float)((double)(v * 4.0f) - (double)(size - 1) * 0.5), then fistp (chop).
static inline int ToCell(float v, double half)
{
    float f = (float)((double)(v * 4.0f) - half);
    return (int)f;
}

static inline double HalfSize(int size)
{
    return (double)(size - 1) * 0.5;
}

// Sizes 0x5da050 has unrolled code for; others log the HD warning
// (FUN_0065cac0 = SLogger::Warning) and take the generic loop.
static bool IsUnrolledSize(int size)
{
    return (size >= 1 && size <= 16) || size == 20 || size == 24;
}

// PANZERS 0x5da050
// SWorld::CheckStaticBlockMapInternal. HD unrolls each size; the unrolled
// cell sets are the disc below (verified against every size in the
// emulator), so one loop reproduces all of them.
bool BlockMap_CheckStaticInternal(SWorld* w, int x, int z, int size, unsigned mask)
{
    if (x < 0)
        return true;
    if (z < 0)
        return true;
    int bw = w->BlockW;
    if (bw < x + size)
        return true;
    int zEnd = z + size;
    if (w->BlockH < zEnd)
        return true;
    if (!IsUnrolledSize(size)) {
        // HD warns on every call; the recompile warns once per size.
        static unsigned char s_warned[64];
        int slot = (size >= 0 && size < 64) ? size : 63;
        if (!s_warned[slot]) {
            s_warned[slot] = 1;
            if (Logger.g)
                Logger.g->Warning("SWorld::CheckStaticBlockMapInternal: Not optimized for unit_small_size = %d", size);
        }
    }
    const unsigned* map = StaticMap(w);
    int dz = 1 - size;
    for (int row = z; row < zEnd; ++row, dz += 2) {
        int dx = 1 - size;
        for (int col = x; col < x + size; ++col, dx += 2) {
            if (dx * dx + dz * dz < size * size && (map[bw * row + col] & mask) != 0)
                return true;
        }
    }
    return false;
}

// PANZERS 0x5d9e00
bool BlockMap_CheckStatic(SWorld* w, float x, float z, int size, unsigned mask)
{
    if (size == 0)
        return false;
    double half = HalfSize(size);
    int cx = ToCell(x, half);
    int cz = ToCell(z, half);
    return BlockMap_CheckStaticInternal(w, cx, cz, size, mask);
}

// PANZERS 0x5d9eb0
// First blocked cell of the footprint. A footprint that leaves the map
// returns false (HD only tests inside the map here).
bool BlockMap_CheckStaticCell(SWorld* w, float x, float z, int size, unsigned mask,
                              int* outX, int* outZ)
{
    if (size == 0)
        return false;
    double half = HalfSize(size);
    int cx = ToCell(x, half);
    int cz = ToCell(z, half);
    if (cx < 0 || cz < 0)
        return false;
    int bw = w->BlockW;
    int xEnd = cx + size;
    if (bw < xEnd)
        return false;
    int zEnd = cz + size;
    if (w->BlockH < zEnd || zEnd <= cz)
        return false;
    const unsigned* map = StaticMap(w);
    int dz = 1 - size;
    for (int row = cz; row < zEnd; ++row, dz += 2) {
        int dx = 1 - size;
        for (int col = cx; col < xEnd; ++col, dx += 2) {
            if (dx * dx + dz * dz < size * size &&
                (col < 0 || row < 0 || w->BlockW <= col || w->BlockH <= row ||
                 (map[bw * row + col] & mask) != 0)) {
                *outX = col;
                *outZ = row;
                return true;
            }
        }
    }
    return false;
}

// PANZERS 0x5d9c10
bool BlockMap_CheckDynamicInternal(SWorld* w, int x, int z, int size)
{
    if (x < 0 || z < 0)
        return false;
    int bw = w->BlockW;
    int xEnd = x + size;
    if (bw < xEnd)
        return false;
    int zEnd = z + size;
    if (w->BlockH < zEnd || zEnd <= z)
        return false;
    const unsigned char* map = DynamicMap(w);
    int dz = 1 - size;
    for (int row = z; row < zEnd; ++row, dz += 2) {
        int dx = 1 - size;
        for (int col = x; col < xEnd; ++col, dx += 2) {
            if (dx * dx + dz * dz < size * size && map[bw * row + col] != 0)
                return true;
        }
    }
    return false;
}

// PANZERS 0x5d99f0
bool BlockMap_CheckDynamic(SWorld* w, float x, float z, int size)
{
    double half = HalfSize(size);
    int cx = ToCell(x, half);
    int cz = ToCell(z, half);
    return BlockMap_CheckDynamicInternal(w, cx, cz, size);
}

// PANZERS 0x5d9a90
bool BlockMap_CheckDynamicCell(SWorld* w, float x, float z, int size, int* outX, int* outZ)
{
    double half = HalfSize(size);
    int cx = ToCell(x, half);
    int cz = ToCell(z, half);
    if (cx < 0 || cz < 0)
        return false;
    int bw = w->BlockW;
    int xEnd = cx + size;
    if (bw < xEnd)
        return false;
    int zEnd = cz + size;
    if (w->BlockH < zEnd || zEnd <= cz)
        return false;
    const unsigned char* map = DynamicMap(w);
    int dz = 1 - size;
    for (int row = cz; row < zEnd; ++row, dz += 2) {
        int dx = 1 - size;
        for (int col = cx; col < xEnd; ++col, dx += 2) {
            if (dx * dx + dz * dz < size * size && map[bw * row + col] != 0) {
                *outX = col;
                *outZ = row;
                return true;
            }
        }
    }
    return false;
}

// PANZERS 0x5f4430
void BlockMap_MarkDynamic(SWorld* w, float x, float z, int size, bool on)
{
    double half = HalfSize(size);
    int cx = ToCell(x, half);
    int cz = ToCell(z, half);
    if (cx < 0 || cz < 0)
        return;
    int xEnd = cx + size;
    if (w->BlockW < xEnd)
        return;
    int zEnd = cz + size;
    if (w->BlockH < zEnd)
        return;
    unsigned char* map = DynamicMap(w);
    int dz = 1 - size;
    for (int row = cz; row < zEnd; ++row, dz += 2) {
        int dx = 1 - size;
        for (int col = cx; col < xEnd; ++col, dx += 2) {
            if (dx * dx + dz * dz < size * size) {
                unsigned char* c = &map[w->BlockW * row + col];
                if (on)
                    *c = (unsigned char)(*c + 1);
                else
                    *c = (unsigned char)(*c - 1);
            }
        }
    }
}

// PANZERS 0x5f4720
void BlockMap_MarkStatic(SWorld* w, float x, float z, int size, bool on, unsigned mask)
{
    double half = HalfSize(size);
    int cx = ToCell(x, half);
    int cz = ToCell(z, half);
    if (cx < 0 || cz < 0)
        return;
    int xEnd = cx + size;
    if (w->BlockW < xEnd)
        return;
    int zEnd = cz + size;
    if (w->BlockH < zEnd)
        return;
    unsigned* map = StaticMap(w);
    int dz = 1 - size;
    for (int row = cz; row < zEnd; ++row, dz += 2) {
        int dx = 1 - size;
        for (int col = cx; col < xEnd; ++col, dx += 2) {
            if (dx * dx + dz * dz < size * size) {
                unsigned* c = &map[w->BlockW * row + col];
                if (on)
                    *c |= mask;
                else
                    *c &= ~mask;
            }
        }
    }
}

// PANZERS 0x600870
// Walks the major axis one cell at a time (first index floor(start) + 1)
// and tests the two cells on either side of the crossed grid line.
bool BlockMap_LineFreeCells(SWorld* w, float x0, float z0, float x1, float z1, int size,
                            unsigned mask)
{
    if ((double)fabsf(x1 - x0) > (double)fabsf(z1 - z0)) {
        // x major
        if (x0 > x1) {
            float t = x0; x0 = x1; x1 = t;
            t = z0; z0 = z1; z1 = t;
        }
        float slope = (z1 - z0) / (x1 - x0);
        int i = (int)floor((double)x0) + 1;          // fistp, control word 0x047f
        float fx = (float)i;
        float zz = (fx - x0) * slope + z0;
        while (x1 >= fx) {
            int cx = (int)fx;
            int cz = (int)zz;
            if (BlockMap_CheckStaticInternal(w, cx, cz, size, mask))
                return false;
            if (BlockMap_CheckStaticInternal(w, cx - 1, cz, size, mask))
                return false;
            zz = zz + slope;
            ++i;
            fx = (float)i;
        }
        return true;
    }
    // z major
    if (z0 > z1) {
        float t = x0; x0 = x1; x1 = t;
        t = z0; z0 = z1; z1 = t;
    }
    float slope = (x1 - x0) / (z1 - z0);
    int i = (int)floor((double)z0) + 1;              // fistp, control word 0x047f
    float fz = (float)i;
    float xx = (fz - z0) * slope + x0;
    while (z1 >= fz) {
        int cx = (int)xx;
        int cz = (int)fz;
        if (BlockMap_CheckStaticInternal(w, cx, cz, size, mask))
            return false;
        if (BlockMap_CheckStaticInternal(w, cx, cz - 1, size, mask))
            return false;
        xx = xx + slope;
        ++i;
        fz = (float)i;
    }
    return true;
}

// PANZERS 0x6007d0
bool BlockMap_LineFree(SWorld* w, float x0, float z0, float x1, float z1, int size,
                       unsigned mask)
{
    double half = HalfSize(size);
    float cx0 = (float)((double)(x0 * 4.0f) - half);
    float cz0 = (float)((double)(z0 * 4.0f) - half);
    float cx1 = (float)((double)(x1 * 4.0f) - half);
    float cz1 = (float)((double)(z1 * 4.0f) - half);
    return BlockMap_LineFreeCells(w, cx0, cz0, cx1, cz1, size, mask);
}

// PANZERS 0x5ddbc0
float* World_ClampToMap(SWorld* w, float* out, float x, float y, float z)
{
    out[0] = x;
    out[1] = y;
    out[2] = z;
    if (out[0] <= 0.0f && out[0] != 0.0f)
        out[0] = 0.0f;
    if (out[2] <= 0.0f && out[2] != 0.0f)
        out[2] = 0.0f;
    if ((float)w->TerrainW < out[0])
        out[0] = (float)w->TerrainW;
    if ((float)w->TerrainH < out[2])
        out[2] = (float)w->TerrainH;
    return out;
}

} // namespace pz
