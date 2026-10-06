// src/game/logicextra.cpp
// SGameLogic per-tick helpers of the menu loop: the per-player visibility
// maps (fog of war) that CanSeeGroundUnit 0x562760 / IsInPlayerVision
// 0x562650 read, and the unit-created hook of SWorld::CreateUnit.
// OWNER: M2-I sub-agent LG. Lifted from the HD exe.
//
// Map layout: half-tile cells, VisW = TerrainW * 2 + 2 per row, VisH rows.
// Bits of a cell: 1 and 2 seen (shadow cast from the eye height), 8 seen at
// half range, 4 mine detection range, 0x10 hearing range. 0x565e10 clears
// the inner part of one player's map per tick (players in the order of HD
// table 0x7f6220) and re-adds every unit of that side (0x565530).

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "gamelogic.h"
#include "world.h"
#include "worldapi.h"
#include "unit.h"
#include "buildingunit.h"
#include "unitextern.h"
#include "drivermath.h"
#include "pzunitregistry.h"
#include "doodad.h"
#include "pz/iterrain.h"
#include "pz/imodel.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

// Shadow-cast tables of 0x564fb0 (HD globals), one entry per (n, k) with
// n = 1..0xbf and k = n..0 in that order (18527 entries):
//   HD 0x8f2080: k / n
//   HD 0x904200: sqrt(1 + (k / n)^2)
//   HD 0x916380: 1 / sqrt(k^2 + n^2)
enum { kVisTable = 0xbf * 0xc0 / 2 + 0xbf };
static float s_VisFrac[kVisTable];
static float s_VisSlopeLen[kVisTable];
static float s_VisInvDist[kVisTable];

// Scratch rows of 0x5662b0 (HD 0x928500 horizon heights, 0x928800 slopes).
static float s_RowH[0xc0];
static float s_RowS[0xc0];

static inline int PlayerType(int p) { return *(const int*)(g_World->Players[p] + 0x08); }   // World+0x178
static inline int PlayerTeam(int p) { return *(const int*)(g_World->Players[p] + 0x0c); }   // World+0x17c

// PANZERS 0x564c20
// The doodad grid: 8x8-tile cells; every doodad with a model is linked into
// the cells its logic-pose extent (+0x100) covers, floor(min / 8) ..
// ceil(max / 8) (fistp under 0x8de15c = 0x47f / 0x8de160 = 0x87f; outside
// the map 0 / size / 8), rows outer, each new link in front of the cell's
// list. ProjectileHitTest 0x562c20 and DamageArea 0x576490 read it.
void SGameLogic::BuildDoodadGrid()
{
    SWorld* w = g_World;
    int gw = w->TerrainW / 8, gh = w->TerrainH / 8;               // cdq / and 7 / sar 3
    DoodadGrid = (int*)malloc((size_t)gw * gh * 4);               // 0x766b87
    memset(DoodadGrid, 0xff, (size_t)gh * gw * 4);
    DoodadLinkCount = 0;                                          // 0x563330(0)
    if (DoodadLinks)
        memset(DoodadLinks, 0, (size_t)DoodadLinkMax * 8);
    for (int i = 0; i < w->Doodads.Size; ++i) {
        if (!w->Doodads.IsLive(i) || !w->Doodads.Array[i].Data.Model)
            continue;
        float x0, x1, z0, z1;
        w->Doodads.Array[i].Data.Model->GetLogicBoundsXZ(&x0, &x1, &z0, &z1);   // +0x100
        int c0 = 0.0f > x0 ? 0 : (int)floorf(x0 * 0.125f);       // 0x7f7f40
        int c1 = x1 > (float)w->TerrainW ? gw : (int)ceilf(x1 * 0.125f);
        int r0 = 0.0f > z0 ? 0 : (int)floorf(z0 * 0.125f);
        int r1 = z1 > (float)w->TerrainH ? gh : (int)ceilf(z1 * 0.125f);
        for (int r = r0; r < r1; ++r)
            for (int c = c0; c < c1; ++c) {
                int cell = gw * r + c;
                if (DoodadLinkCount == DoodadLinkMax) {
                    int m = DoodadLinkMax < 0x10 ? 0x10 : DoodadLinkMax * 6 / 5;
                    DoodadLinks = (SDoodadLink*)realloc(DoodadLinks, (size_t)m * 8);   // 0x78b864
                    memset(DoodadLinks + DoodadLinkMax, 0, (size_t)(m - DoodadLinkMax) * 8);
                    DoodadLinkMax = m;
                }
                int k = DoodadLinkCount++;
                DoodadLinks[k].Doodad = i;
                DoodadLinks[k].Next = DoodadGrid[cell];
                DoodadGrid[cell] = k;
            }
    }
}

// The cell head of 0x562c20 / 0x576490: (int)(x * 0.125) under CW 0xc7f.
int SGameLogic::DoodadGridHead(float x, float z) const
{
    int cx = (int)(x * 0.125f);
    int cz = (int)(z * 0.125f);
    return DoodadGrid[(g_World->TerrainW / 8) * cz + cx];
}

// PANZERS 0x564fb0
void SGameLogic::BuildVisMaps()
{
    SWorld* w = g_World;
    VisW = w->TerrainW * 2 + 2;
    VisH = w->TerrainH * 2 + 2;
    for (int p = 0; p < 12; ++p) {
        int type = PlayerType(p);
        if (type == 2 || type == 3) {
            VisMap[p] = nullptr;
            VisMapOwned[p] = nullptr;
            continue;
        }
        // Allies (same non-zero team, World+0x17c) share the first map.
        int team = PlayerTeam(p);
        int k = 0;
        if (team != 0) {
            for (; k < p; ++k)
                if (PlayerTeam(k) == team && VisMap[k] != nullptr)
                    break;
        }
        if (team != 0 && k < p) {
            VisMap[p] = VisMap[k];
            VisMapOwned[p] = nullptr;
            continue;
        }
        VisMap[p] = (unsigned char*)malloc(VisW * VisH);          // 0x766b87
        memset(VisMap[p], 0, VisW * VisH);
        VisMapOwned[p] = VisMap[p];
    }
    VisHeights = (float*)malloc(VisW * VisH * sizeof(float));
    BuildVisHeights();                                            // 0x576a70
    int t = 0;
    for (int n = 1; n < 0xc0; ++n) {
        float fn = (float)n;
        float nn = fn * fn;
        for (int k = n; k >= 0; --k) {
            float f = (float)k / fn;
            s_VisFrac[t] = f;
            s_VisSlopeLen[t] = (float)sqrt((double)(f * f + 1.0f));          // 0x78d090
            float fk = (float)k;
            float len = (float)sqrt((double)(fk * fk + nn));
            s_VisInvDist[t] = (float)(1.0 / (double)len);         // _DAT_007eed98 = 1.0
            ++t;
        }
    }
    VisOverlayMode = 1;
}

// The half-tile cells under a model's logic-pose extent (+0x100, 0x6d7150),
// widened by one cell, where a 1 m vertical segment above the ground at the
// cell corner crosses the model's collision (+0xd8, 0x6db550) get `add` on
// their eye height. The extent is converted with fistp under 0x8de160 (0x87f,
// up) for the minimum and 0x8de15c (0x47f, down) for the maximum.
static void RaiseVisHeights(SGameLogic* gl, SIModel* m, float add)
{
    float x0, x1, z0, z1;
    m->GetLogicBoundsXZ(&x0, &x1, &z0, &z1);
    int c0 = (int)ceilf(x0 * 2.0f) - 1;                       // 0x7f4558
    if (c0 < 0)
        c0 = 0;
    int c1 = (int)floorf(x1 * 2.0f) + 1;
    if (c1 > gl->VisW)
        c1 = gl->VisW;
    int r0 = (int)ceilf(z0 * 2.0f) - 1;
    if (r0 < 0)
        r0 = 0;
    int r1 = (int)floorf(z1 * 2.0f) + 1;
    if (r1 > gl->VisH)
        r1 = gl->VisH;
    for (int r = r0; r < r1; ++r) {
        float z = (float)r * 0.5f;                            // 0x7f453c
        for (int c = c0; c < c1; ++c) {
            float x = (float)c * 0.5f;
            float g = g_World->GetTerrainHeight(x, z);        // 0x5e7730
            float lo[3] = { x, g, z };
            float g2 = g_World->GetTerrainHeight(x, z);
            float hi[3] = { x, g2 + 1.0f, z };                // fld1 / faddp, fstp float
            if (m->HitTestSegment(hi, lo))                    // +0xd8
                gl->VisHeights[gl->VisW * r + c] += add;
        }
    }
}

// PANZERS 0x546f70
// SBuildingUnit (called by 0x576a70 for every building): cells under a
// building that is not of type 3 rise by 2.0.
static void AddBuildingHeights(SGameLogic* gl, SUnit* u)
{
    const unsigned char* pb = *(const unsigned char* const*)((const unsigned char*)u + 0x340);   // SPBuildingUnit
    if (*(const int*)(pb + 0x13c) == 3 || u->Model == nullptr)
        return;
    RaiseVisHeights(gl, u->Model, 2.0f);                      // 0x7f4558
}

// PANZERS 0x57f6a0
// An indestructible, non-scrub doodad without a "Platform" node: cells
// under it rise by 0.75.
static void AddDoodadHeights(SGameLogic* gl, int i)
{
    SWorld* w = g_World;
    if (i < 0 || i >= w->Doodads.Size || w->Doodads.Array[i].Next != kHeapLive)
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "struct SDoodad", i);
    RaiseVisHeights(gl, w->Doodads.Array[i].Data.Model, 0.75f);   // 0x7f2fcc
}

// PANZERS 0x576a70
void SGameLogic::BuildVisHeights()
{
    SWorld* w = g_World;
    for (int row = 0; row < VisH; ++row) {
        float z = (float)((double)row * 0.5 - 0.25);              // 0x7ea760, 0x7f5a10
        if (0.0f > z)
            z = 0.0f;
        else if (z > (float)w->TerrainH)
            z = (float)w->TerrainH;
        for (int col = 0; col < VisW; ++col) {
            float x = (float)((double)col * 0.5 - 0.25);
            if (0.0f > x)
                x = 0.0f;
            else if (x > (float)w->TerrainW)
                x = (float)w->TerrainW;
            // A 47 m border on big maps (more than 0x60 tiles) is a wall.
            bool border = false;
            if (w->TerrainW > 0x60 && (47.0f > x || x >= (float)(w->TerrainW - 0x30)))   // 0x7f7f8c
                border = true;
            else if (w->TerrainH > 0x60 && (47.0f > z || !(z < (float)(w->TerrainH - 0x30))))
                border = true;
            if (border)
                VisHeights[VisW * row + col] = 100.0f;            // 0x42c80000
            else
                VisHeights[VisW * row + col] = w->GetTerrainHeight(x, z);   // 0x5e7730
        }
    }
    for (int i = 0; i < w->Doodads.Size; ++i) {
        if (w->Doodads.Array[i].Next != kHeapLive)
            continue;
        const SDoodad& d = w->Doodads.Array[i].Data;
        if (d.Scrub != 0 || d.Indestructible == 0)
            continue;
        if (d.Model->FindNode("Platform") < 0)                   // +0x40
            AddDoodadHeights(this, i);                            // 0x57f6a0
    }
    for (int i = 0; i < w->Units.Size; ++i) {
        if (w->Units.Array[i].Next != kHeapLive)
            continue;
        SUnit* u = w->Units.Array[i].Unit;
        if (u->Proto->ClassType == 9)
            AddBuildingHeights(this, u);                          // 0x546f70
    }
}

// PANZERS 0x5662b0
// One octant of the shadow cast from `cell` (eye height `eye`): `outer`
// steps to the next ring, `inner` along it. A cell is seen when it is above
// the horizon of the cells before it (less than 0.5 below counts too).
void SGameLogic::CastVisOctant(int player, float eye, int cell, int radius, int outer, int inner, unsigned char bits)
{
    memset(&s_RowS[1], 0, 0xbf * sizeof(float));
    memset(&s_RowH[1], 0, 0xbf * sizeof(float));
    s_RowH[0] = VisHeights[cell];
    s_RowS[0] = -10.0f;                                           // 0xc1200000
    int diag = (int)(float)((double)radius * 0.7071067811865);    // 0x7f7f48, fistp 0xc7f
    int t = 0;
    for (int d = 1; d < radius; ++d) {
        int len = d;
        if (d > diag) {
            len = (int)(float)sqrt((double)(float)(radius * radius - d * d));   // 0x78d090
            t += d - len;
        }
        cell += (len + 1) * inner + outer;
        for (int k = len; k >= 0; --k) {
            cell -= inner;
            if (k == 0) {
                s_RowH[0] = s_RowS[0] + s_RowH[0];
            } else {
                float f = s_VisFrac[t];
                float s0 = s_RowS[k];
                float s = (s_RowS[k - 1] - s0) * f + s0;
                s_RowS[k] = s;
                float h0 = s_RowH[k];
                s_RowH[k] = (s_RowH[k - 1] - h0) * f + h0 + s_VisSlopeLen[t] * s;
            }
            float h = VisHeights[cell];
            if (h >= s_RowH[k]) {
                s_RowH[k] = h;
                s_RowS[k] = (h - eye) * s_VisInvDist[t];
                VisMap[player][cell] |= bits;
            } else if ((double)h + 0.5 >= (double)s_RowH[k]) {   // 0x7ea760
                VisMap[player][cell] |= bits;
            }
            ++t;
        }
    }
}

// Scratch rows of 0x567280 (HD 0x928b00 / 0x928e00).
static float s_HalfH[0xc0];
static float s_HalfS[0xc0];

// PANZERS 0x567280
// CastVisOctant with bits 0xb, marking only the cells on the positive side
// of the line through the eye: ring * cy + step * cx > 0.
void SGameLogic::CastVisOctantHalf(int player, float eye, int cell, int radius, int outer, int inner, float cx, float cy)
{
    memset(&s_HalfS[1], 0, 0xbf * sizeof(float));
    memset(&s_HalfH[1], 0, 0xbf * sizeof(float));
    s_HalfH[0] = VisHeights[cell];
    s_HalfS[0] = -10.0f;
    int diag = (int)(float)((double)radius * 0.7071067811865);    // 0x7f7f48
    int t = 0;
    for (int d = 1; d < radius; ++d) {
        int len = d;
        if (d > diag) {
            len = (int)sqrt((double)(radius * radius - d * d));   // cvttsd2si of the double
            t += d - len;
        }
        cell += (len + 1) * inner + outer;
        for (int k = len; k >= 0; --k) {
            cell -= inner;
            if (k == 0) {
                s_HalfH[0] = s_HalfS[0] + s_HalfH[0];
            } else {
                float f = s_VisFrac[t];
                float s0 = s_HalfS[k];
                float s = (s_HalfS[k - 1] - s0) * f + s0;
                s_HalfS[k] = s;
                float h0 = s_HalfH[k];
                s_HalfH[k] = (s_HalfH[k - 1] - h0) * f + h0 + s_VisSlopeLen[t] * s;
            }
            float h = VisHeights[cell];
            bool mark;
            if (h < s_HalfH[k]) {
                mark = (double)s_HalfH[k] <= (double)h + 0.5;     // 0x7ea760
            } else {
                s_HalfH[k] = h;
                s_HalfS[k] = (h - eye) * s_VisInvDist[t];
                mark = true;
            }
            if (mark && 0.0f < (float)d * cy + (float)k * cx)
                VisMap[player][cell] |= 0xb;
            ++t;
        }
    }
}

static inline double WrapPiD(double a)
{
    if (a > 3.1415927410125732)                                   // 0x7f4560
        return a - 6.2831854820251465;                            // 0x7f4570
    if (a < -3.1415927410125732)                                  // 0x7f5aa0
        return a + 6.2831854820251465;
    return a;
}

// PANZERS 0x5664f0
// A sight cone of `width` around `dir`: the 8 octants (from -pi in steps of
// pi/4) entirely inside the cone are cast with 0x5662b0, the ones holding a
// cone edge with 0x567280 limited by that edge.
void SGameLogic::CastVisCone(int player, float eye, int cell, int radius, float dir, float width)
{
    const float kPiF = 3.1415927f, kTwoPiF = 6.2831855f;           // 0x7f4584, 0x7f458c
    float a = (float)fmod((double)dir, 6.2831854820251465);       // 0x793cba
    if (a > kPiF)
        a -= kTwoPiF;
    else if (!(a > -kPiF))                                        // 0x7f7fb8
        a += kTwoPiF;
    float half = width * 0.5f;                                    // 0x7f453c
    float lo = (float)WrapPiD((double)a - (double)half);
    float hi = (float)WrapPiD((double)half + (double)a);
    const int W = VisW;
    // {start, end, outer, inner}; the edge vectors (cx, cy) of the lo / hi
    // edge in each octant, as HD computes them from sin / cos (0x78d640 /
    // 0x78d480).
    struct SOct { double S, E; int Outer, Inner; };
    const double q = 0.7853981852531433, h2 = 1.5707963705062866, t3 = 2.35619455575943, pi = 3.1415927410125732;
    const SOct oct[8] = {
        { -pi, -t3, -W, -1 }, { -t3, -h2, -1, -W }, { -h2, -q, -1, W }, { -q, 0.0, W, -1 },
        { 0.0, q, W, 1 },     { q, h2, 1, W },      { h2, t3, 1, -W },  { t3, pi, -W, 1 },
    };
    for (int o = 0; o < 8; ++o) {
        const SOct& s = oct[o];
        if (!(WrapPiD((double)lo - s.E) < 0.0))
            continue;
        float sl = (float)sin((double)lo), cl = (float)cos((double)lo);
        float sh = (float)sin((double)hi), ch = (float)cos((double)hi);
        float cx, cy;                                             // 0x567280 p8, p9
        bool edge = true;
        if (WrapPiD((double)lo - s.S) > 0.0) {
            switch (o) {                                          // the lo edge
            case 0: cx = -cl; cy = sl; break;
            case 1: cx = sl;  cy = -cl; break;
            case 2: cx = -sl; cy = -cl; break;
            case 3: cx = -cl; cy = -sl; break;
            case 4: cx = cl;  cy = -sl; break;
            case 5: cx = -sl; cy = cl; break;
            case 6: cx = sl;  cy = cl; break;
            default: cx = cl; cy = sl; break;
            }
        } else {
            if (!(WrapPiD((double)hi - s.S) > 0.0))
                continue;
            if (WrapPiD((double)hi - s.E) < 0.0) {
                switch (o) {                                      // the hi edge
                case 0: cx = ch;  cy = -sh; break;
                case 1: cx = -sh; cy = ch; break;
                case 2: cx = sh;  cy = ch; break;
                case 3: cx = ch;  cy = sh; break;
                case 4: cx = -ch; cy = sh; break;
                case 5: cx = sh;  cy = -ch; break;
                case 6: cx = -sh; cy = -ch; break;
                default: cx = -ch; cy = -sh; break;
                }
            } else {
                edge = false;
                cx = cy = 0.0f;
            }
        }
        if (edge)
            CastVisOctantHalf(player, eye, cell, radius, s.Outer, s.Inner, cx, cy);
        else
            CastVisOctant(player, eye, cell, radius, s.Outer, s.Inner, 0xb);
    }
}

// PANZERS 0x567180
// The same octant walk without the horizon: every cell in range.
void SGameLogic::FillVisOctant(int player, int cell, int radius, int outer, int inner, unsigned char bits)
{
    int diag = (int)(float)((double)radius * 0.7071067811865);
    for (int d = 1; d < radius; ++d) {
        int len = d;
        if (d > diag)
            len = (int)(float)sqrt((double)(float)(radius * radius - d * d));
        cell += (len + 1) * inner + outer;
        for (int k = len; k >= 0; --k) {
            cell -= inner;
            VisMap[player][cell] |= bits;
        }
    }
}

// HD SUnit vtbl +0x18c (0x548380 returns 0.0; SPanzersSquadUnit 0x59bf30:
// mine detector range by rank). iunit.h types the slot as void (agent U), so
// the two bodies are read here.
// PANZERS 0x59bf30
static float UnitMineRange(SUnit* u)
{
    if (u->Proto->ClassType != 5)
        return 0.0f;                                              // 0x548380 fldz
    switch (u->GetRank()) {                                       // +0x88
    case 0: return (float)g_UnitRegistry->MineDetectorRange[0];   // registry +0xec
    case 1: return (float)g_UnitRegistry->MineDetectorRange[1];
    case 2: return (float)g_UnitRegistry->MineDetectorRange[2];
    case 3: return (float)g_UnitRegistry->MineDetectorRange[3];
    default: return SseF(u->GetSightRange());                     // +0x184
    }
}

// The 8 octants in HD call order: (outer, inner).
#define PZ_VIS_OCTANTS(CALL)                                                    \
    CALL(1, VisW); CALL(1, -VisW); CALL(-VisW, 1); CALL(-VisW, -1);             \
    CALL(-1, -VisW); CALL(-1, VisW); CALL(VisW, -1); CALL(VisW, 1)

// PANZERS 0x565530
void SGameLogic::AddUnitVision(int player, SIUnit* unit)
{
    if (VisMap[player] == nullptr)
        return;
    SUnit* u = static_cast<SUnit*>(unit);
    SWorld* w = g_World;
    float x = u->Pos[0];
    float z = u->Pos[2];
    if (48.0f > x || 48.0f > z)                                   // 0x7f7f90
        return;
    if (x > (float)(w->TerrainW - 0x30) || z > (float)(w->TerrainH - 0x30))
        return;
    int cx = (int)(x * 2.0f);                                     // fistp 0xc7f (truncation)
    int cz = (int)(u->Pos[2] * 2.0f);
    int sight = (int)(SseF(u->GetSightRange()) * 2.0f);          // +0x184, fmul 2.0, fstp, fistp
    if (sight == 0)
        return;
    if (sight > 0x5f)
        sight = 0x5f;
    if (u->Proto->ClassType == 9) {
        SBuildingUnit* b = static_cast<SBuildingUnit*>(u);
        if (b->WindowSets.Size != 0) {                            // +0x3ec
            // Per view set (+0x3e8, 0x48 each; 0x546330): a 3pi/4 cone
            // along the view direction from max(cell height, +0x358) + 0.75
            // (M3 agent F).
            const float* views = (const float*)(const void*)b->WindowSets.Array;
            for (int i = 0; i < b->WindowSets.Size; ++i) {
                const float* v = views + i * (0x48 / 4);
                int vx = (int)(v[0] * 2.0f);                      // fistp 0xc7f
                int vz = (int)(v[1] * 2.0f);
                int c = VisW * vz + vx;
                VisMap[player][c] |= 0xb;
                float e = VisHeights[c];
                if (e <= b->InsideY)
                    e = b->InsideY;
                CastVisCone(player, e + 0.75f, c, sight, v[2], 2.3561945f);   // 0x4016cbe4
            }
            return;
        }
        int cell = VisW * cz + cx;
        float eye = VisHeights[cell];
        float inside = b->InsideY;                                // +0x358
        if (!(eye > inside))
            eye = inside;
        eye = eye + 0.75f;                                        // 0x7f2fcc
        VisMap[player][cell] |= 0xb;
#define PZ_CAST_B(o, i) CastVisOctant(player, eye, cell, sight, (o), (i), 0xb)
        PZ_VIS_OCTANTS(PZ_CAST_B);
#undef PZ_CAST_B
        return;
    }
    int cell = VisW * cz + cx;
    float eye = VisHeights[cell] + 0.75f;
    if (u->Proto->Detector || u->_138 == 8 || u->_144 == 8) {    // SPUnit +0x88, 0x5ba820(8)
        int mines = (int)(UnitMineRange(u) * 2.0f);               // +0x18c, cvttss2si
        if (mines > 0) {
            if (mines > 0x5f)
                mines = 0x5f;
            VisMap[player][cell] |= 4;
#define PZ_FILL_4(o, i) FillVisOctant(player, cell, mines, (o), (i), 4)
            PZ_VIS_OCTANTS(PZ_FILL_4);
#undef PZ_FILL_4
        }
    }
    VisMap[player][cell] |= 3;
#define PZ_CAST_3(o, i) CastVisOctant(player, eye, cell, sight, (o), (i), 3)
    PZ_VIS_OCTANTS(PZ_CAST_3);
#undef PZ_CAST_3
    VisMap[player][cell] |= 8;
    int half = sight >> 1;
#define PZ_CAST_8(o, i) CastVisOctant(player, eye, cell, half, (o), (i), 8)
    PZ_VIS_OCTANTS(PZ_CAST_8);
#undef PZ_CAST_8
    // Hearing (+0x188), shortened by rain (World+0x62c) against the registry
    // RainHearing (+0x110).
    int hear = (int)(SseF(u->GetExtra188()) * 2.0f);
    if (hear <= 0)
        return;
    float rainHearing = (float)g_UnitRegistry->RainHearing;
    float rain = *(const float*)((const unsigned char*)w + 0x62c);
    if (!(rainHearing > rain))
        return;
    float h2 = SseF(u->GetExtra188()) * 2.0f;
    float k = 1.0f - rain / rainHearing;                          // 0x7f1b58
    hear = (int)(h2 * k);
    if (hear <= 0)
        return;
    if (hear > 0x5f)
        hear = 0x5f;
    VisMap[player][cell] |= 0x10;
#define PZ_FILL_10(o, i) FillVisOctant(player, cell, hear, (o), (i), 0x10)
    PZ_VIS_OCTANTS(PZ_FILL_10);
#undef PZ_FILL_10
}

// PANZERS 0x565e10
// Rebuilds the map of `player` (only the player that owns it): clears the
// inside, adds the vision of every unit of that side that is not inside
// another one (or is class 10) and not +0x110, hands the local player's map
// to the terrain, and clears bit 0 on a 3-cell frame at 0x5e.
void SGameLogic::Tick_565e10(int player)
{
    PZ_M2_TRACE("SGameLogic::Tick_565e10 (0x565e10)");
    if (VisMapOwned[player] == nullptr)
        return;
    if (VisMap[player] == VisMapOwned[player])
        PlayerTable[player] = Frame;                              // +0x234[player]
    SWorld* w = g_World;
    for (int row = 0x5e; row < w->TerrainH * 2 - 0x5e; ++row)
        memset(VisMap[player] + 0x5e + VisW * row, 0, VisW - 0xbe);
    for (int i = 0; i < w->Units.Size; ++i) {
        if (w->Units.Array[i].Next != kHeapLive)
            continue;
        SUnit* u = w->Units.Array[i].Unit;
        int up = u->Player;
        bool side;
        if (up == player) {
            side = true;
        } else {
            int team = PlayerTeam(up);
            side = team == 0 ? up == player : team == PlayerTeam(player);   // 0x549ab0
        }
        if (!side)
            continue;
        if ((u->Parent < 0 || u->Proto->ClassType == 10) && !u->_110)
            AddUnitVision(player, u);                             // 0x565530
    }
    if (VisMap[player] == VisMap[w->LocalPlayer]) {
        if (w->Terrain)
            w->Terrain->SetOverlay((int)(size_t)VisMap[player], VisOverlayMode);   // terrain +0x1c
        // The minimap fog bitmap (0x565f1d) is the HUD's (PzMinimapUpdate).
    }
    for (int i = 0; i < 3; ++i) {
        for (int c = 0x5e; c < w->TerrainW * 2 - 0x5e; ++c) {
            VisMap[player][(i + 0x5e) * VisW + c] &= 0xfe;
            VisMap[player][(VisH - 0x63 + i) * VisW + c] &= 0xfe;
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int r = 0x5e; r < w->TerrainH * 2 - 0x5e; ++r) {
            VisMap[player][VisW * r + i + 0x5e] &= 0xfe;
            VisMap[player][VisW * (r + 1) + i - 0x63] &= 0xfe;
        }
    }
}

// Recompile: the maps this object allocated (HD frees them in 0x55fe00).
void SGameLogic::FreeVisMaps()
{
    for (int p = 0; p < 12; ++p) {
        free(VisMapOwned[p]);
        VisMapOwned[p] = nullptr;
        VisMap[p] = nullptr;
    }
    free(VisHeights);
    VisHeights = nullptr;
}

} // namespace pz

// The hook of SWorld::CreateUnit 0x5e3170 (unitextern.h): for a new unit
// that is top-level (or class 10) and not +0x110, HD calls
// g_GameLogic->0x565530(player, unit) with CreateUnit's player argument,
// which InitNew stored in the unit's +0xfc.
extern "C" void PzGameLogicUnitCreated(int unitIndex)
{
    using namespace pz;
    if (!g_GameLogic)
        return;
    SUnit* u = WorldUnit(unitIndex);                              // 0x546490
    g_GameLogic->AddUnitVision(u->Player, u);                     // 0x565530
}
