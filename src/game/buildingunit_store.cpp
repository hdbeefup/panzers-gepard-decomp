// src/game/buildingunit_store.cpp
// SBuildingUnit: the window points (Init 0x548660 / 0x548560) and a squad
// getting in (StoreUnit 0x54d380). Lifted by agent F for the M3 replay
// oracle: the Training Camp map puts squads in bunkers and towers at map
// load, and each member's "stand" (0x5cae80) draws the world RNG
// (docs/M3_STATUS.md). The rest of SBuildingUnit stays with its owner
// (buildingunit.cpp).

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buildingunit.h"
#include "unitanim.h"
#include "gamelogic.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/imodel.h"
#include "pz/hdmath.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

// HD layout of the window data (SBuildingUnit +0x364..+0x3f4).
struct SBuildingPoint { float X, Z, Dir; };
struct SBuildingView {                       // +0x3e8 element (0x48 bytes)
    SBuildingPoint View;                     // "view%d"
    SBuildingPoint Points[5];                // "p%d%d"
};
static_assert(sizeof(SBuildingView) == 0x48, "0x548660 stride 0x48");

static SBuildingPoint* Pt(SBuildingUnit* b, int off) { return (SBuildingPoint*)((unsigned char*)b + off); }
struct SViewArray { SBuildingView* Array; int Size; int Max; };
static SViewArray* Views(SBuildingUnit* b) { return (SViewArray*)((unsigned char*)b + 0x3e8); }

static SUnit* UnitAt(int i)
{
    if (!g_World->Units.IsLive(i))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", i);
    return g_World->Units.Array[i].Unit;
}

// PANZERS 0x548560
// The node's position (x, z), its facing atan2(axis y.x, axis y.z), and the
// node's height into +0x358; the building's position and 0 when the model
// has no such node.
bool SBuildingUnit::ReadNodePoint(float* dst, const char* node)
{
    int n = Model->FindNode(node);                                // model +0x40
    if (n < 0) {
        dst[0] = Pos[0];
        dst[1] = Pos[2];
        dst[2] = 0.0f;
        return false;
    }
    float pos[3] = { 0.0f, 0.0f, 0.0f };
    float axis[3] = { 0.0f, 0.0f, 0.0f };
    Model->GetNodePositionAxis(n, pos, axis);                     // model +0x50
    dst[0] = pos[0];
    dst[1] = pos[2];
    dst[2] = HdAtan2f((double)axis[0], (double)axis[2]);          // 0x78d07a(ST1 = axis x, ST0 = axis z)
    InsideY = pos[1];                                             // +0x358
    return true;
}

// PANZERS 0x548660
// +0x358 = the terrain height, then the node points: "entrance" (+0x364),
// "pa%d" (+0x370) and "p0%d" (+0x3ac) for 0..4 (each found node also sets
// +0x358 to its height), and the view sets "view%d" with "p%d%d" (+0x3e8);
// +0x3f4 = the farthest view from the building.
void SBuildingUnit::FindWindowPoints()
{
    InsideY = g_World->GetTerrainHeight(Pos[0], Pos[2]);          // 0x5e7730
    ReadNodePoint(&Pt(this, 0x364)->X, "entrance");
    for (int i = 0; i < 5; ++i) {
        char name[16];
        sprintf(name, "pa%d", i);                                 // 0x7f4380
        ReadNodePoint(&Pt(this, 0x370 + i * 0xc)->X, name);
        sprintf(name, "p0%d", i);                                 // 0x7f4388
        ReadNodePoint(&Pt(this, 0x3ac + i * 0xc)->X, name);
    }
    RangeBonus = 0.0f;                                            // +0x3f4
    SViewArray* v = Views(this);
    v->Size = 0;
    if (v->Max > 0 && v->Array)
        memset(v->Array, 0, v->Max * sizeof(SBuildingView));
    for (;;) {
        // PANZERS 0x548660 inline SDArray::Add
        if (v->Size == v->Max) {
            int nmax = v->Max < 0x10 ? 0x10 : (v->Max * 6) / 5;
            v->Array = (SBuildingView*)realloc(v->Array, nmax * sizeof(SBuildingView));
            memset(v->Array + v->Max, 0, (nmax - v->Max) * sizeof(SBuildingView));
            v->Max = nmax;
        }
        int i = v->Size++;
        char name[16];
        sprintf(name, "view%d", v->Size);
        if (!ReadNodePoint(&v->Array[i].View.X, name)) {
            --v->Size;                                            // SDArray::Remove(i)
            memset(&v->Array[v->Size], 0, sizeof(SBuildingView));
            if (g_GameLogic)
                Logger.g->Log(1, "STUB: SBuildingUnit 0x5497a0 (view sets with a game logic) not lifted");
            return;
        }
        float dx = v->Array[i].View.X - Pos[0];
        float dz = v->Array[i].View.Z - Pos[2];
        float d = (float)sqrt((double)(dx * dx + dz * dz));
        if (RangeBonus <= d && d != RangeBonus)
            RangeBonus = d;
        for (int k = 0; k < 5; ++k) {
            sprintf(name, "p%d%d", v->Size, k);
            ReadNodePoint(&v->Array[i].Points[k].X, name);
        }
    }
}

// HD 0x549ab0
static bool SameSideB(int a, int b)
{
    int team = *(int*)(g_World->Players[a] + 0x0c);
    return team != 0 ? team == *(int*)(g_World->Players[b] + 0x0c) : a == b;
}

static int MembersAdd(SUnitArray<SUnitMember>* a)                // 0x5467c0
{
    if (a->Size == a->Max) {
        int nmax = a->Max < 0x10 ? 0x10 : (a->Max * 6) / 5;
        a->Array = (SUnitMember*)realloc(a->Array, nmax * sizeof(SUnitMember));
        memset(a->Array + a->Max, 0, (nmax - a->Max) * sizeof(SUnitMember));
        a->Max = nmax;
    }
    return a->Size++;
}

static int StoredAdd(SUnitArray<SUnitStored>* a)                  // 0x546830
{
    if (a->Size == a->Max) {
        int nmax = a->Max < 0x10 ? 0x10 : (a->Max * 6) / 5;
        a->Array = (SUnitStored*)realloc(a->Array, nmax * sizeof(SUnitStored));
        memset(a->Array + a->Max, 0, (nmax - a->Max) * sizeof(SUnitStored));
        a->Max = nmax;
    }
    return a->Size++;
}

static int AnimStateIndex(SIUnitAnimation* anim, const char* name)   // 0x5c7ed0
{
    if (!anim)
        return 0;
    SPUnitAnimation* p = static_cast<SUnitAnimation*>(anim)->Proto;
    return p ? p->FindState(name) : 0;
}

static void MembersClear(SUnitArray<SUnitMember>* a)              // 0x546ca0(0)
{
    if (a->Size != 0 && !a->Array)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "SStoredMemberProperties");
    a->Size = 0;
    if (a->Array)
        memset(a->Array, 0, a->Max * sizeof(SUnitMember));
}

static void IntsResize(SUnitArray<int>* a, int n)                 // 0x546af0
{
    if (a->Size != 0 && !a->Array)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "int");
    a->Size = n;
    if (a->Max < n) {
        a->Max = n;
        a->Array = (int*)realloc(a->Array, n * 4);
    }
    if (a->Array)
        memset(a->Array, 0, a->Max * 4);
}

// PANZERS 0x5c50c0 (SUnit: hand the selection to `to`)
static void MoveSelection(SUnit* from, SUnit* to)
{
    if (from->_104 & 1) {
        to->_104 |= 1;
        from->_104 &= ~1u;
    }
    to->_108 |= from->_108;
    from->_108 = 0;
}

// PANZERS 0x54d380
// Only squads (class 5) that CanStore 0x5b7040 accepts. The players that
// see the squad lose the building's "unknown" flag (+0x2cc). Type 5
// buildings keep the members unplaced; otherwise the first squad's members
// stand at the "p0%d" points (+0x3ac, Members +0x178, slots +0x3fc) and a
// second (enemy) squad's at "pa%d" (+0x370, Members2 +0x408, slots +0x414),
// each in its "stand" state (0x5b7390 -> 0x5cae80, one world RNG draw).
bool SBuildingUnit::StoreUnit(int unit, int mode)
{
    SUnit* sq = UnitAt(unit);
    if (sq->Proto->ClassType != 5 || !SUnit::CanStoreUnit(unit))  // 0x5b7040
        return false;
    if (sq->Proto->HeroPicture >= 0)                              // prototype +0x34
        mode = 2;
    for (int i = 0; i < 12; ++i) {
        int type = *(int*)(g_World->Players[i] + 0x08);           // World+0x178
        if (SameSideB(sq->Player, i) ||
            ((type == 1 || type == 0) && g_GameLogic && g_GameLogic->CanSeeGroundUnit(i, sq)))   // 0x562760
            _2cc[i] = false;
    }
    if (P->BuildingType == 5) {
        int si = StoredAdd(&Stored);                             // 0x546830
        Stored.Array[si].Unit = unit;
        Stored.Array[si].Mode = mode;
        for (int k = 0; k < sq->Members.Size; ++k) {
            int mi = MembersAdd(&Members);
            Members.Array[mi].Unit = sq->Members.Array[k].Unit;
            Members.Array[mi].Owner = sq->WorldIndex;
        }
        MembersClear(&sq->Members);
        sq->Unplace();                                            // +0x4c
        sq->Parent = WorldIndex;
        sq->IsStored = true;
        sq->StoreMode = mode;
        for (int k = 0; k < Members.Size; ++k) {
            SUnit* m = UnitAt(Members.Array[k].Unit);
            m->Parent = WorldIndex;
            m->Unplace();
        }
        MoveSelection(sq, this);                                  // 0x5c50c0
        _110 = false;
        Player = sq->Player;
    } else {
        bool second = Stored.Size != 0;
        if (second && Stored.Size != 1)
            return false;
        int si = StoredAdd(&Stored);
        Stored.Array[si].Unit = unit;
        Stored.Array[si].Mode = mode;
        SUnitArray<SUnitMember>* mem = second ? &Members2 : &Members;
        SUnitArray<int>* slots = second ? &WindowSlots2 : &WindowSlots;
        int pts = second ? 0x370 : 0x3ac;
        for (int k = 0; k < sq->Members.Size; ++k) {
            int mi = MembersAdd(mem);
            mem->Array[mi].Unit = sq->Members.Array[k].Unit;
            mem->Array[mi].Owner = sq->WorldIndex;
            UnitAt(mem->Array[mi].Unit)->MainGunner = 0;          // member +0x44 = 0
        }
        MembersClear(&sq->Members);
        sq->Unplace();
        sq->Parent = WorldIndex;
        sq->IsStored = true;
        sq->StoreMode = mode;
        IntsResize(slots, mem->Size);
        for (int k = 0; k < mem->Size; ++k) {
            SUnit* m = UnitAt(mem->Array[k].Unit);
            m->Parent = WorldIndex;                               // +0x78
            m->Pos[1] = InsideY;                                  // +0x90 = +0x358
            const SBuildingPoint* p = Pt(this, pts + k * 0xc);
            m->Place(p->X, p->Z, p->Dir);                         // +0x50
            if (k >= slots->Size)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "int", k);
            slots->Array[k] = k;
            m->SetGlobalState(AnimStateIndex(m->Anim, "stand"), 0);   // 0x5c7ed0 on +0x14, 0x5b7390
        }
        sq->SetGlobalState(0, 0);                                 // 0x5b7390(0, 0)
        if (second) {
            StopGunners();                                        // vtbl +0xec (0x547e90)
            MainGunner = -1;                                      // +0x44
            MoveSelection(sq, this);
        } else {
            MainGunner = sq->MainGunner;                          // +0x44
            MoveSelection(sq, this);
            Behavior = sq->Behavior;                              // +0x250
            _110 = false;
        }
        Player = sq->Player;
        g_World->UnitStored(WorldIndex, sq->Player);              // 0x5ef760
    }
    if (g_GameLogic)
        g_GameLogic->Dispatch_571380(WorldIndex, sq->WorldIndex); // 0x571380
    return true;
}

} // namespace pz
