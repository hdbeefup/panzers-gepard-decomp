// src/world/astar.cpp
// SAStar and SHeapList (HD 0x5a1300..0x5a3810) and the SWorld A* accessors
// 0x5e70e0 / 0x5e7960. OWNER: P. Lifted from the HD exe and checked against
// it in the unicorn emulator (see astar.h).

#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "astar.h"
#include "blockmap.h"
#include "world.h"
#include "worldapi.h"
#include "logger.h"

namespace pz {

// HD 0x65c970 (SLogger::Panic). The HD build aborts; the recompile logs and
// throws the message like the map loader does.
static void AStarPanic(const char* msg)
{
    if (Logger.g)
        Logger.g->Log(0, "PZ3D A*: fatal: %s", msg);
    throw msg;
}

// One search node (0x18 bytes): the SHeapList item payload, the local node
// of 0x5a3270/0x5a1d30, and (parent, f, x, z) in the closed SHeap.
struct SAStarNode {
    int   Parent;   // +0x00 closed-list index of the parent, -1 for the start
    float F;        // +0x04 G + H
    float G;        // +0x08 cost so far
    float H;        // +0x0c heuristic
    int   X;        // +0x10 cell
    int   Z;        // +0x14 cell
};

// SHeapList item (0x20 bytes). Item 0 is the head and item 1 the tail
// sentinel; Prev == -2 marks a free item.
struct SHeapListItem {
    int        Prev;   // +0x00
    int        Next;   // +0x04
    SAStarNode Data;   // +0x08
};

// HD SHeapList<SAStarNode> (0x20 bytes, at SAStar+0x38).
struct SHeapList {
    SHeapListItem* Items;   // +0x00
    int  Size;              // +0x04
    int  Max;               // +0x08
    int  Free;              // +0x0c free list head (linked by Next)
    int  _10;               // +0x10
    int  _14;               // +0x14
    int  Current;           // +0x18 cursor (1 = none)
    int  Count;             // +0x1c live items
};

// Closed list element (0x14 bytes, SHeap at SAStar+0x58).
struct SAStarClosed {
    int   Next;             // +0x00 kHeapLive when live
    int   Parent;           // +0x04
    float F;                // +0x08
    int   X;                // +0x0c
    int   Z;                // +0x10
};

struct SAStarClosedHeap {
    SAStarClosed* Array;    // +0x00
    int Size;               // +0x04
    int Max;                // +0x08
    int Free;               // +0x0c
    int Count;              // +0x10
};

// HD SAStar (0xa4 bytes, vftable 0x7fa..., 1 slot: the heuristic 0x5a1fd0).
struct SAStar {
    virtual float Heuristic(int x0, int z0, int x1, int z1);   // +0x00 HD 0x5a1fd0
    void*  Unit;              // +0x04 FindPath arg 1 (stored only)
    int    Size;              // +0x08 unit size in cells
    unsigned Mask;            // +0x0c block mask
    bool   Local;             // +0x10
    unsigned char _11[3];
    int    W;                 // +0x14 grid width (BlockW)
    int    H;                 // +0x18 grid height (BlockH)
    int*   Grid;              // +0x1c W*H: closed index, or open item + 0x40000000
    int    StartX;            // +0x20 start cell
    int    StartZ;            // +0x24
    int    GoalX;             // +0x28 goal cell
    int    GoalZ;             // +0x2c
    float  StartWX;           // +0x30 start (world)
    float  StartWZ;           // +0x34
    SHeapList Open;           // +0x38
    SAStarClosedHeap Closed;  // +0x58
    int    LastClosed;        // +0x6c index of the last closed node
    unsigned char _70[0x88 - 0x70];
    float  _88;               // +0x88 reset to -1.0
    int    Iterations;        // +0x8c
    int    MaxIterations;     // +0x90 GetGlobalAStar 40000, GetLocalAStar 10000
    int    Reopened;          // +0x94 closed nodes reopened by 0x5a1d30
    SHdArray<float[2]> Points;// +0x98 the result path (x, z)

    SAStar(int w, int h);
};

#if defined(_M_IX86)
static_assert(sizeof(SAStar) == 0xa4, "HD new 0xa4");
static_assert(offsetof(SAStar, Grid) == 0x1c, "SAStar +0x1c");
static_assert(offsetof(SAStar, Open) == 0x38, "SAStar +0x38");
static_assert(offsetof(SAStar, Closed) == 0x58, "SAStar +0x58");
static_assert(offsetof(SAStar, LastClosed) == 0x6c, "SAStar +0x6c");
static_assert(offsetof(SAStar, MaxIterations) == 0x90, "SAStar +0x90");
static_assert(offsetof(SAStar, Points) == 0x98, "SAStar +0x98");
static_assert(sizeof(SHeapListItem) == 0x20 && sizeof(SAStarClosed) == 0x14, "HD items");
#endif

// HD SDArray/SHeap growth: 16, then * 6 / 5.
static int Grow(int max)
{
    return max < 0x10 ? 0x10 : (max * 6) / 5;
}

// PANZERS 0x5a1fd0
// Manhattan distance in cells * 5.
float SAStar::Heuristic(int x0, int z0, int x1, int z1)
{
    int dx = x1 - x0;
    int dz = z1 - z0;
    if (dx < 0) dx = -dx;
    if (dz < 0) dz = -dz;
    return (float)(dx + dz) * 5.0f;
}

// PANZERS 0x5a19a0
void AStar_Reset(SAStar* a)
{
    a->Iterations = 0;
    a->Open.Size = 2;
    a->Open.Free = -1;
    a->Open.Count = 0;
    a->Open.Items[0].Next = 1;
    a->Open.Items[1].Prev = 0;
    a->Open.Current = 1;
    a->Closed.Size = 0;
    a->Closed.Free = -1;
    a->Closed.Count = 0;
    a->LastClosed = -1;
    // 0x550a30 SDArray::Clear(0)
    if (a->Points.Size != 0 && a->Points.Array == nullptr)
        AStarPanic("SDArray<%s>::Clear: array is damaged");
    a->Points.Size = 0;
    if (a->Points.Max < 0) {
        a->Points.Max = 0;
        a->Points.Array = (float(*)[2])realloc(a->Points.Array, 0);
    }
    if (a->Points.Array)
        memset(a->Points.Array, 0, a->Points.Max * 8);
    a->_88 = -1.0f;
    a->Reopened = 0;
}

// PANZERS 0x5a1460
SAStar::SAStar(int w, int h)
{
    StartWX = 0.0f;
    StartWZ = 0.0f;
    Open.Items = (SHeapListItem*)operator new(0x140);
    Open.Size = 2;
    Open.Max = 10;
    Open.Free = -1;
    Open.Count = 0;
    Open.Items[0].Prev = -1;
    Open.Items[0].Next = 1;
    Open.Items[1].Prev = 0;
    Open.Items[1].Next = -1;
    Open.Current = 1;
    Closed.Array = nullptr;
    Closed.Size = 0;
    Closed.Max = 0;
    Closed.Free = -1;
    Closed.Count = 0;
    Points.Array = nullptr;
    Points.Size = 0;
    Points.Max = 0;
    W = w;
    H = h;
    MaxIterations = 0;
    Grid = (int*)operator new((size_t)w * (size_t)h * 4);
    // Fields HD leaves uninitialised; the search never reads them before
    // writing them.
    Unit = nullptr;
    Size = 0;
    Mask = 0;
    Local = false;
    StartX = StartZ = GoalX = GoalZ = 0;
    Open._10 = Open._14 = 0;
    memset(_70, 0, sizeof(_70));
    AStar_Reset(this);
}

// PANZERS 0x5a1740
// SHeapList::operator->: the current item's payload.
static SAStarNode* ListCurrent(SHeapList* l)
{
    if (l->Current == 1)
        AStarPanic("SHeapList::operator->: no current item");
    return &l->Items[l->Current].Data;
}

// PANZERS 0x5a1910
// Allocates an item (free list first), zeroes its payload, returns its index.
static int ListAlloc(SHeapList* l)
{
    int i = l->Free;
    if (i >= 0) {
        l->Free = l->Items[i].Next;
        memset(&l->Items[i].Data, 0, sizeof(SAStarNode));
        return i;
    }
    if (l->Size == l->Max) {
        int n = Grow(l->Max);
        l->Items = (SHeapListItem*)realloc(l->Items, (size_t)n * 0x20);
        l->Max = n;
    }
    memset(&l->Items[l->Size].Data, 0, sizeof(SAStarNode));
    return l->Size++;
}

// Links item n in front of the first item (the head's Next) and makes it
// current (the start of 0x5a1820 and its inline copy in 0x5a3270).
static int ListInsertFront(SHeapList* l)
{
    l->Current = l->Items[0].Next;
    int n = ListAlloc(l);
    int cur = l->Current;
    int prev = l->Items[cur].Prev;
    l->Items[n].Next = cur;
    l->Items[n].Prev = prev;
    l->Items[cur].Prev = n;
    l->Items[prev].Next = n;
    ++l->Count;
    l->Current = n;
    if (n == 1)
        AStarPanic("SHeapList::GetDataPointer: no current item");
    return n;
}

// PANZERS 0x5a1820
// Inserts a node into the open list and records it in the grid.
static void OpenInsert(SAStar* a, const SAStarNode* node)
{
    int n = ListInsertFront(&a->Open);
    a->Open.Items[n].Data = *node;
    SAStarNode* d = ListCurrent(&a->Open);
    a->Grid[d->Z * a->W + d->X] = a->Open.Current + 0x40000000;
}

// PANZERS 0x5a1a80
// SHeapList::Delete of the current item; the cursor moves to the next one.
static void ListDeleteCurrent(SHeapList* l)
{
    int cur = l->Current;
    if (cur == 1)
        AStarPanic("SHeapList::Delete: no current item");
    SHeapListItem* it = l->Items;
    int next = it[cur].Next;
    it[it[cur].Next].Prev = it[cur].Prev;
    it[it[cur].Prev].Next = it[cur].Next;
    it[cur].Next = l->Free;
    it[cur].Prev = -2;
    --l->Count;
    l->Current = next;
    l->Free = cur;
}

// PANZERS 0x5a16f0
static SAStarClosed* ClosedAt(SAStarClosedHeap* h, int i)
{
    if (i < 0 || i >= h->Size || h->Array[i].Next != kHeapLive)
        AStarPanic("SHeap<%s>::operator[]: invalid index (%d)");
    return &h->Array[i];
}

// PANZERS 0x5a1770
static int ClosedAlloc(SAStarClosedHeap* h)
{
    ++h->Count;
    int i = h->Free;
    if (i >= 0) {
        h->Free = h->Array[i].Next;
        h->Array[i].Next = kHeapLive;
        h->Array[i].Parent = 0;
        h->Array[i].F = 0.0f;
        h->Array[i].X = 0;
        h->Array[i].Z = 0;
        return i;
    }
    if (h->Size == h->Max) {
        int n = Grow(h->Max);
        h->Array = (SAStarClosed*)realloc(h->Array, (size_t)n * 0x14);
        memset(h->Array + h->Max, 0, (size_t)(n - h->Max) * 0x14);
        h->Max = n;
    }
    h->Array[h->Size].Next = kHeapLive;
    return h->Size++;
}

// PANZERS 0x5a2f50
static void ClosedRemove(SAStarClosedHeap* h, int i)
{
    if (i < 0 || i >= h->Size || h->Array[i].Next != kHeapLive)
        AStarPanic("SHeap<%s>::Remove: invalid index (%d)");
    h->Array[i].Next = h->Free;
    --h->Count;
    h->Free = i;
}

// PANZERS 0x5d86e0
// Passability of a node's cell: CheckStaticBlockMapInternal(size, mask).
static bool NodeBlocked(const SAStarNode* n, int size, unsigned mask)
{
    return BlockMap_CheckStaticInternal(g_World, n->X, n->Z, size, mask);
}

// PANZERS 0x5d8670
// Step cost: 1.0 for the global search; the local search makes cells with
// block bit 0x2000 expensive (10.0) and the others cheap (0.1).
static float NodeCost(const SAStarNode* n, int size, bool local)
{
    if (!local)
        return 1.0f;
    if (BlockMap_CheckStaticInternal(g_World, n->X, n->Z, size, 0x2000))
        return 10.0f;
    return 0.1f;
}

// Neighbour table 0x8dc1d0: {dx, dz, bit set when blocked, skip if any of
// these bits is set}. Diagonals are skipped when one of their two straight
// neighbours was blocked.
struct SAStarDir { int Dx, Dz, Bit, Skip; };
static const SAStarDir kAStarDirs[8] = {
    { -1,  0, 1, 0 }, { 0, -1, 2, 0 }, { 1, 0, 4, 0 }, { 0, 1, 8, 0 },
    { -1, -1, 0, 3 }, { 1, -1, 0, 6 }, { 1, 1, 0, 12 }, { -1, 1, 0, 9 },
};

// PANZERS 0x5a1d30
// Expands the node just closed (a->LastClosed).
static void Expand(SAStar* a, const SAStarNode* parent)
{
    unsigned blocked = 0;
    for (int d = 0; d < 8; ++d) {
        const SAStarDir& dir = kAStarDirs[d];
        SAStarNode node;
        node.X = dir.Dx + parent->X;
        node.Z = parent->Z + dir.Dz;
        if ((unsigned)dir.Skip & blocked)
            continue;
        if (NodeBlocked(&node, a->Size, a->Mask)) {
            blocked |= (unsigned)dir.Bit;
            continue;
        }
        float cost = NodeCost(&node, a->Size, a->Local);
        if (parent->X != node.X && parent->Z != node.Z)
            cost = cost * 1.41421354f;
        node.G = parent->G + cost;
        node.H = a->Heuristic(node.X, node.Z, a->GoalX, a->GoalZ);
        node.Parent = a->LastClosed;
        node.F = node.H + node.G;
        int idx = a->Grid[a->W * node.Z + node.X];
        if (idx > 0x40000000) {
            int it = idx - 0x40000000;
            if (it >= 2 && it < a->Open.Size && a->Open.Items[it].Prev != -2) {
                a->Open.Current = it;
                SHeapListItem* item = &a->Open.Items[it];
                if (node.X == item->Data.X && node.Z == item->Data.Z) {
                    SAStarNode* cur = ListCurrent(&a->Open);
                    if (cur->F > node.F) {
                        cur->Parent = node.Parent;
                        cur->G = node.G;
                        cur->H = node.H;
                        cur->F = node.F;
                    }
                    continue;
                }
            }
        } else if (idx >= 0 && idx < a->Closed.Size && a->Closed.Array[idx].Next == kHeapLive) {
            SAStarClosed* c = ClosedAt(&a->Closed, idx);
            if (node.X == c->X && node.Z == c->Z) {
                if (node.F + 1.0f >= c->F)
                    continue;
                ++a->Reopened;
                ClosedRemove(&a->Closed, idx);
            }
        }
        OpenInsert(a, &node);
    }
}

// MakePathPointsList for local searches (0x5a23a0, below). It did not run in
// the menu: the drivers always search with local = false.
static void MakePathPointsListLocal(SAStar* a);

// PANZERS 0x5a2020
// String pulling: from the current point, the furthest node back from the
// goal that has a free line of sight becomes the next path point.
static void MakePathPointsList(SAStar* a)
{
    if (a->Local) {
        MakePathPointsListLocal(a);
        return;
    }
    double half = (double)(a->Size - 1) * 0.5;
    float px = (float)((double)(a->StartWX * 4.0f) - half);
    float pz = (float)((double)(a->StartWZ * 4.0f) - half);
    for (;;) {
        int k = a->LastClosed;
        for (;;) {
            if (k < 0)
                AStarPanic("SAStar::MakePathPointsList: Can't step.");
            SAStarClosed* c = ClosedAt(&a->Closed, k);
            float tx = (float)((double)c->X + 0.5);
            float tz = (float)((double)c->Z + 0.5);
            if (BlockMap_LineFreeCells(g_World, px, pz, tx, tz, a->Size, a->Mask))
                break;
            k = c->Parent;
        }
        // SDArray::Add
        if (a->Points.Size == a->Points.Max) {
            int n = Grow(a->Points.Max);
            a->Points.Array = (float(*)[2])realloc(a->Points.Array, (size_t)n * 8);
            memset(a->Points.Array + a->Points.Max, 0, (size_t)(n - a->Points.Max) * 8);
            a->Points.Max = n;
        }
        int p = a->Points.Size++;
        SAStarClosed* c = ClosedAt(&a->Closed, k);
        a->Points.Array[p][0] = (float)(((double)c->X + (double)a->Size * 0.5) * 0.25);
        a->Points.Array[p][1] = (float)(((double)c->Z + (double)a->Size * 0.5) * 0.25);
        if (k == a->LastClosed) {
            a->Open.Size = 2;
            a->Open.Free = -1;
            a->Open.Count = 0;
            a->Open.Items[0].Next = 1;
            a->Open.Items[1].Prev = 0;
            a->Open.Current = 1;
            a->Closed.Size = 0;
            a->Closed.Free = -1;
            a->Closed.Count = 0;
            return;
        }
        px = (float)((double)c->X + 0.5);
        pz = (float)((double)c->Z + 0.5);
    }
}

// PANZERS 0x557180
// SDArray<(x, z)>::Insert(index) with a zeroed element; returns the index.
static int PointsInsert(SAStar* a, int idx)
{
    if (idx < 0 || idx > a->Points.Size)
        AStarPanic("SDArray<%s>::Insert: invalid index (%d)");
    if (a->Points.Size == a->Points.Max) {
        int n = Grow(a->Points.Max);
        a->Points.Array = (float(*)[2])realloc(a->Points.Array, (size_t)n * 8);
        memset(a->Points.Array + a->Points.Max + 1, 0, (size_t)(n - a->Points.Max) * 8 - 8);
        a->Points.Max = n;
    }
    if (idx < a->Points.Size)
        memmove(a->Points.Array + idx + 1, a->Points.Array + idx, (size_t)(a->Points.Size - idx) * 8);
    a->Points.Array[idx][0] = 0.0f;
    a->Points.Array[idx][1] = 0.0f;
    ++a->Points.Size;
    return idx;
}

// PANZERS 0x5d86c0
// CheckStaticBlockMapInternal at a closed node's cell.
static bool ClosedBlocked(const SAStarClosed* c, int size, unsigned mask)
{
    return BlockMap_CheckStaticInternal(g_World, c->X, c->Z, size, mask);
}

static void PointFromNode(SAStar* a, int p, const SAStarClosed* c)
{
    a->Points.Array[p][0] = (float)(((double)a->Size * 0.5 + (double)c->X) * 0.25);
    a->Points.Array[p][1] = (float)(((double)a->Size * 0.5 + (double)c->Z) * 0.25);
}

// PANZERS 0x5a23a0
// Local variant: walks back from the goal and inserts points at the front.
// While the nodes are free of block bit 0x2000 the line of sight also has to
// avoid 0x2000 cells. Quirk kept from HD: after a new point the walk resumes
// at the parent of the node that ended the previous sight line.
static void MakePathPointsListLocal(SAStar* a)
{
    int k = a->LastClosed;
    SAStarClosed* g = ClosedAt(&a->Closed, k);
    float px = (float)((double)g->X + 0.5);
    float pz = (float)((double)g->Z + 0.5);
    bool flag = !ClosedBlocked(g, a->Size, 0x2000);
    int p = PointsInsert(a, 0);
    PointFromNode(a, p, ClosedAt(&a->Closed, k));
    for (;;) {
        int last = k;
        k = ClosedAt(&a->Closed, k)->Parent;
        if (k < 0)
            break;
        if (flag && ClosedBlocked(ClosedAt(&a->Closed, k), a->Size, 0x2000))
            flag = false;
        int anchor;
        for (;;) {
            unsigned mask = flag ? (a->Mask | 0x2000) : a->Mask;
            SAStarClosed* c = ClosedAt(&a->Closed, k);
            float tx = (float)((double)c->X + 0.5);
            float tz = (float)((double)c->Z + 0.5);
            if (!BlockMap_LineFreeCells(g_World, px, pz, tx, tz, a->Size, mask)) {
                anchor = last;
                break;
            }
            last = k;
            k = c->Parent;
            if (!flag && !ClosedBlocked(ClosedAt(&a->Closed, last), a->Size, 0x2000)) {
                flag = true;
                anchor = last;
                break;
            }
            if (k < 0) {
                anchor = -1;
                break;
            }
        }
        if (k < 0)
            break;
        p = PointsInsert(a, 0);
        SAStarClosed* c = ClosedAt(&a->Closed, anchor);
        PointFromNode(a, p, c);
        px = (float)((double)c->X + 0.5);
        pz = (float)((double)c->Z + 0.5);
    }
    a->Open.Size = 2;
    a->Open.Free = -1;
    a->Open.Count = 0;
    a->Open.Items[0].Next = 1;
    a->Open.Items[1].Prev = 0;
    a->Open.Current = 1;
    a->Closed.Size = 0;
    a->Closed.Free = -1;
    a->Closed.Count = 0;
}

// PANZERS 0x5a3270
void AStar_FindPath(SAStar* a, void* unit, int* result, float x0, float z0, float x1, float z1,
                    int size, unsigned mask, bool local)
{
    AStar_Reset(a);
    a->Mask = mask;
    a->Size = size;
    a->Local = local;
    a->Unit = unit;
    if (local)
        a->MaxIterations = 30000;
    double half = (double)(size - 1) * 0.5;
    a->StartX = (int)(float)((double)(x0 * 4.0f) - half);
    a->StartZ = (int)(float)((double)(z0 * 4.0f) - half);
    a->StartWX = x0;
    a->StartWZ = z0;
    a->GoalX = (int)(float)((double)(x1 * 4.0f) - half);
    a->GoalZ = (int)(float)((double)(z1 * 4.0f) - half);

    SAStarNode node;
    node.X = a->StartX;
    node.Z = a->StartZ;
    if (NodeBlocked(&node, a->Size, a->Mask)) {
        *result = 0;
        return;
    }
    // The start node goes into the open list without a grid entry.
    int n = ListInsertFront(&a->Open);
    (void)n;
    SAStarNode* s = ListCurrent(&a->Open);
    s->Parent = -1;
    s->X = a->StartX;
    s->Z = a->StartZ;
    s->G = 0.0f;
    s->H = a->Heuristic(a->StartX, a->StartZ, a->GoalX, a->GoalZ);
    s->F = s->H + s->G;

    while (a->Open.Count > 0) {
        ++a->Iterations;
        float best = 3.40282347e+38f;
        SAStarNode* bestNode = nullptr;
        a->Open.Current = a->Open.Items[0].Next;
        while (a->Open.Current != 1) {
            SHeapListItem* it = &a->Open.Items[a->Open.Current];
            if (best > it->Data.F) {
                best = it->Data.F;
                bestNode = &it->Data;
            }
            a->Open.Current = it->Next;
        }
        node = *bestNode;
        SHeapListItem* item = (SHeapListItem*)((char*)bestNode - 8);
        if (item < a->Open.Items + 2 || item >= a->Open.Items + a->Open.Size || item->Prev == -2)
            AStarPanic("SHeapList::Delete: the specified item is not a valid list item");
        a->Open.Current = (int)(item - a->Open.Items);
        ListDeleteCurrent(&a->Open);

        int c = ClosedAlloc(&a->Closed);
        a->LastClosed = c;
        SAStarClosed* cl = ClosedAt(&a->Closed, c);
        cl->Parent = node.Parent;
        cl->X = node.X;
        cl->Z = node.Z;
        cl->F = node.F;
        a->Grid[a->W * node.Z + node.X] = a->LastClosed;
        if (a->Iterations > a->MaxIterations)
            break;
        if (node.X == a->GoalX && node.Z == a->GoalZ) {
            *result = 2;
            MakePathPointsList(a);
            return;
        }
        Expand(a, &node);
    }
    *result = 0;
}

int AStar_GetPointCount(const SAStar* a)
{
    return a->Points.Size;
}

const float* AStar_GetPoint(const SAStar* a, int i)
{
    if (i < 0 || i >= a->Points.Size)
        AStarPanic("SDArray<%s>::operator[]: invalid index (%d)");
    return a->Points.Array[i];
}

static SAStar* WorldAStar(SWorld* w)
{
    if (!w->Obj74e4)
        w->Obj74e4 = new SAStar(w->BlockW, w->BlockH);   // HD: SWorld::Initialize 0x5eec90
    return (SAStar*)w->Obj74e4;
}

// PANZERS 0x5e70e0
SAStar* World_GetGlobalAStar(SWorld* w)
{
    SAStar* a = WorldAStar(w);
    if (!a)
        AStarPanic("SWorld::GetGlobalAStar - !GlobalAStar");
    a->MaxIterations = 40000;
    return a;
}

// PANZERS 0x5e7960
SAStar* World_GetLocalAStar(SWorld* w)
{
    SAStar* a = WorldAStar(w);
    if (!a)
        AStarPanic("SWorld::GetLocalAStar - !GlobalAStar");
    a->MaxIterations = 10000;
    return a;
}

} // namespace pz
