// src/game/buildingunit.cpp
// SBuildingUnit (0x545940..0x5500a0). OWNER: agent U. See buildingunit.h.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buildingunit.h"
#include "squadunit.h"
#include "unitanim.h"
#include "target.h"
#include "drivermath.h"
#include "unitextern.h"
#include "gunner.h"
#include "idriver.h"
#include "gamelogic.h"
#include "pz/iviewport.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/imodel.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {

static SUnit* BuildingUnitAt(int index)
{
    if (!g_World->Units.IsLive(index))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", index);
    return g_World->Units.Array[index].Unit;
}

// World+0x17c + player * 0x48: the player's team (0 = none).
static int PlayerTeam(int player)
{
    return *(int*)(g_World->Players[player] + 0x0c);
}

// Same side: same team, or the same player when unteamed (inline in HD).
static bool SameSide(int a, int b)
{
    int t = PlayerTeam(a);
    return t == 0 ? a == b : t == PlayerTeam(b);
}


// A product record string (SString {buf, len} at `off` of the 0x68-byte
// record, prototype +0x154).
static const char* ProductString(const SPBuildingProduct& p, int off)
{
    const char* s;
    memcpy(&s, p.Data + off, 4);
    return s ? s : "";
}

static int ProductInt(const SPBuildingProduct& p, int off)
{
    int v;
    memcpy(&v, p.Data + off, 4);
    return v;
}

// PANZERS 0x545940
// A productive building loads (GetPUnit(name, true)) every unit type its
// production slots name (prototype +0x154, 0x68 bytes each: the strings at
// +0x04, +0x0c, +0x14, +0x38, +0x40, +0x48 always, those at +0x1c, +0x24,
// +0x2c, +0x50, +0x58, +0x60 when their length is not 0).
SBuildingUnit::SBuildingUnit(SPBuildingUnit* proto, int worldIndex)
    : SUnit(proto, worldIndex)
{
    memset(&P, 0, sizeof(SBuildingUnit) - offsetof(SBuildingUnit, P));
    P = proto;
    WindowSet = -1;                                           // +0x3f8
    for (int i = 0; i < 12; ++i)
        _2cc[i] = true;                                       // +0x2cc[i] = 1, +0x43c[i] = 0
    FlagNode = -1;                                            // +0x44c
    for (int i = 0; i < proto->Products.Size; ++i) {
        const SPBuildingProduct& r = proto->Products.Array[i];
        static const int kAlways[6] = { 0x04, 0x0c, 0x14, 0x38, 0x40, 0x48 };
        static const int kOptional[6] = { 0x1c, 0x24, 0x2c, 0x50, 0x58, 0x60 };
        for (int k = 0; k < 6; ++k) {
            g_UnitRegistry->GetPUnit(ProductString(r, kAlways[k]), true);   // 0x5d0e70
            if (ProductInt(r, kOptional[k] + 4) > 0)
                g_UnitRegistry->GetPUnit(ProductString(r, kOptional[k]), true);
        }
    }
    OnStaticBlock = false;                                    // +0x454
    InsideY = 0.0f;                                           // +0x358
}

// PANZERS 0x5460c0
SBuildingUnit::~SBuildingUnit()
{
}

// SIModel +0xac (0x6d8090): the positions of the vertices of a node's mesh
// ({x, y, z} each) into an SDArray (the caller frees the array).
static int ModelNodePoints(SIModel* model, const char* node, float (**out)[3])
{
    SVec3Array a = { nullptr, 0, 0 };
    model->GetNodePoints(node, &a);
    *out = a.Array;
    return a.Size;
}

// HD 0x661b30 + operator delete(0x1c) (as SDoodad's in mapload.cpp).
static void FreeBuildingBitmap(SBlockBitmap* bm)
{
    delete[] bm->Bits;
    bm->Bits = nullptr;
    delete bm;
}

// PANZERS 0x600f90
// SWorld::UnfixBridges: drops the second height map layer again.
void SWorld::UnfixBridges()
{
    if (UseAltHeights) {
        UseAltHeights = false;
        return;
    }
    Logger.g->Panic("SWorld::UnfixBridges: HeightMapLayer2 is not enabled.");
}

// PANZERS 0x548f20
// The footprints ("Block" static block bit 4; a hangar's "Indoor" 0x8000; a
// productive building's "Indoor" 0x800) go on the static block map and
// their nodes are hidden; then the entrance / window / view points, the
// capture flag of a capturable building, the targeting refresh, the
// "Platform" rebuild when the bridges are already fixed, and +0x454 (a
// "Block" point stands on the static block map). The board elements
// (+0x350 / +0x354) are not created by the recompile.
void SBuildingUnit::Init(SUnitDef* def)
{
    SUnit::Init(def);                                         // 0x5ba8e0
    _110 = true;
    BlockNode = Model->BuildNodeBlockBitmap(4, "Block");      // +0xa8
    BlockMap_ApplyBitmap(g_World, BlockNode, true, 4);        // 0x5f4910
    Model->SetNodeVisible(Model->FindNode("Block"), false);   // +0x40 / +0x60
    if (P->BuildingType == 6) {
        IndoorNode = Model->BuildNodeBlockBitmap(4, "Indoor");
        BlockMap_ApplyBitmap(g_World, IndoorNode, true, 0x8000);
        Model->SetNodeVisible(Model->FindNode("Indoor"), false);
    }
    if (P->UnitType == 0x1a) {
        ProductiveIndoorNode = Model->BuildNodeBlockBitmap(4, "Indoor");
        BlockMap_ApplyBitmap(g_World, ProductiveIndoorNode, true, 0x800);
        Model->SetNodeVisible(Model->FindNode("Indoor"), false);
    }
    InitViewPoints();                                         // 0x548660
    if (P->BuildingType == 3) {
        FlagNode = Model->FindNode("flag");                   // +0x40
        STUB_LOG("SBuildingUnit 0x5471d0 (the capture flag model), called by SBuildingUnit::Init 0x548f20");
    }
    if (g_GameLogic)
        RefreshTargeting();                                   // +0x34
    if (g_World->UseAltHeights) {                             // World+0xf0
        if (Model->FindNode("Platform") != 0) {               // HD tests != 0 (not >= 0)
            if (BlockNode)
                BlockMap_MarkDirty(g_World, BlockNode->X, BlockNode->Z, BlockNode->W + BlockNode->X,
                                   BlockNode->H + BlockNode->Z, 0x2004);   // 0x5ef380
            g_World->UnfixBridges();                          // 0x600f90
            g_World->FixBridges();                            // 0x5e65f0
            g_GameLogic->BuildVisHeights();                   // 0x576a70
        }
    }
    OnStaticBlock = false;                                    // +0x454
    float (*pts)[3] = nullptr;
    int n = ModelNodePoints(Model, "Block", &pts);            // +0xac
    for (int i = 0; i < n; ++i) {
        int x, z;
        memcpy(&x, &pts[i][0], 4);
        memcpy(&z, &pts[i][2], 4);
        if (SUnit::TestBlockMap(x, z, 0, 1, 1)) {             // 0x5b7500 (direct call)
            OnStaticBlock = true;
            break;
        }
    }
    free(pts);
}

// PANZERS 0x547690
// The flag model goes, the footprints leave the static block map (and the
// bitmaps are freed), a "Platform" building rebuilds the bridges (or just
// the dirty block map), the wires are cut, then SUnit::Uninit.
void SBuildingUnit::Uninit()
{
    if (Model) {
        if (Model448) {
            if (FlagNode > -1)
                STUB_LOG("SIModel +0xe0 (0x6d7310, detach the flag), untyped in imodel.h (agent E); SBuildingUnit::Uninit 0x547690");
            if (Model448) {
                Model448->Release();                          // +0x04
                Model448 = nullptr;
            }
        }
        if (BlockNode)
            BlockMap_ApplyBitmap(g_World, BlockNode, false, 4);   // 0x5f4910
        if (IndoorNode && P->BuildingType == 6)
            BlockMap_ApplyBitmap(g_World, IndoorNode, false, 0x8000);
        if (ProductiveIndoorNode && P->UnitType == 0x1a)
            BlockMap_ApplyBitmap(g_World, ProductiveIndoorNode, false, 0x800);
        if (g_GameLogic && g_World->UseAltHeights) {
            int platform = Model->FindNode("Platform");
            if (BlockNode)
                BlockMap_MarkDirty(g_World, BlockNode->X, BlockNode->Z, BlockNode->W + BlockNode->X,
                                   BlockNode->H + BlockNode->Z, 0x2004);   // 0x5ef380
            SIModel* m = Model;
            Model = nullptr;                                  // the rebuild must not see this building
            if (platform < 0) {
                g_World->RefreshBlockMapDirtyRect();          // 0x604620
            } else {
                g_World->UnfixBridges();                      // 0x600f90
                g_World->FixBridges();                        // 0x5e65f0
            }
            g_GameLogic->BuildVisHeights();                   // 0x576a70
            Model = m;
        }
    }
    if (BlockNode) {
        FreeBuildingBitmap(BlockNode);                        // 0x661b30, delete(0x1c)
        BlockNode = nullptr;
    }
    if (IndoorNode) {
        FreeBuildingBitmap(IndoorNode);
        IndoorNode = nullptr;
    }
    if (ProductiveIndoorNode) {
        FreeBuildingBitmap(ProductiveIndoorNode);
        ProductiveIndoorNode = nullptr;
    }
    for (int i = 0; i < Wires.Size; ++i)
        STUB_LOG("SWorld 0x5f82b0 (remove a wire), called by SBuildingUnit::Uninit 0x547690");
    SUnit::Uninit();                                          // 0x5b7e40
}

// PANZERS 0x54de30
// A garrisoned bunker (type 2) hit by bullets passes a quarter of the
// damage to its last occupant (0x5c3dd0), then SUnit::TakeDamage.
void SBuildingUnit::TakeDamage(float damage, int weaponType, int attacker, float x, float y, float z, int hitMode)
{
    if (P->BuildingType == 2 && Members.Size > 0 && weaponType == 0)
        DamageMembers(true, damage * 0.25f, 0, attacker, x, y, z, hitMode);   // DAT_007f4538
    SUnit::TakeDamage(damage, weaponType, attacker, x, y, z, hitMode);         // 0x5c4080
}

// PANZERS 0x54ca80
// The occupants shoot at the target: a position (type 2) through each
// member's +0xe4, a unit through +0xe8 with a world RNG draw per member for
// its gunner +0x60. Without a weapon or for other orders: +0xec.
void SBuildingUnit::SetCurrentTarget(STarget* target, int p2)
{
    if (CurrentTarget)
        CurrentTarget->Release();                             // 0x5bdef0
    if (target)
        target->AddRef();                                     // 0x5b5a30
    CurrentTarget = target;
    if ((target->Kind != 2 && target->Kind != 3) || MainGunner < 0) {
        StopGunners();                                        // +0xec
        return;
    }
    target->Mode = 1;
    if (CurrentTarget->Type == 2) {
        for (int i = 0; i < Members.Size; ++i) {
            SUnit* m = BuildingUnitAt(Members.Array[i].Unit);
            int xb, zb;
            memcpy(&xb, &CurrentTarget->Pos[0], 4);
            memcpy(&zb, &CurrentTarget->Pos[2], 4);
            m->EC_AttackPos(xb, zb, p2);                          // member +0xe4 (0x597fa0)
        }
        return;
    }
    if (CurrentTarget->Type != 0)
        Logger.g->Panic("SBuildingUnit::SetCurrentTarget: Unknown target type");
    for (int i = 0; i < Members.Size; ++i) {
        BuildingUnitAt(Members.Array[i].Unit)->EC_Attack(CurrentTarget->Unit, p2);   // +0xe8
        int r = WorldRand();
        if (i >= Members.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", i);
        SUnit* m = BuildingUnitAt(Members.Array[i].Unit);
        if (m->Gunners.Size < 1)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
        // DAT_007f4598 = -1/32768: the truncated product is always 0.
        m->Gunners.Array[0]->_60 = (i != 0) * 2 - (int)((double)r * -3.0517578125e-05);
    }
}

template <typename T>
static int BuildingArrayAdd(SUnitArray<T>* a)               // SDArray::Add (0x546830 / 0x5467c0 ...)
{
    if (a->Size == a->Max) {
        int m = a->Max < 0x10 ? 0x10 : a->Max * 6 / 5;
        a->Array = (T*)realloc(a->Array, m * sizeof(T));
        memset(a->Array + a->Max, 0, (m - a->Max) * sizeof(T));
        a->Max = m;
    }
    return a->Size++;
}

template <typename T>
static void BuildingArraySetSize(SUnitArray<T>* a, int n)   // 0x546ca0 / 0x546af0: size n, cleared
{
    a->Size = n;
    if (a->Max < n) {
        a->Max = n;
        a->Array = (T*)realloc(a->Array, n * sizeof(T));
    }
    if (a->Max)
        memset(a->Array, 0, a->Max * sizeof(T));
}

// The members of squad `s` move into list `dst` (owner = the squad) with
// their default gunner, the squad leaves the map as the building's stored
// unit; then each member stands at window `win[i]` (y = +0x358) in "stand".
static void MoveSquadInside(SBuildingUnit* b, SUnit* s, int mode, SUnitArray<SUnitMember>* dst,
                            SUnitArray<int>* slots, float (*win)[3])
{
    for (int i = 0; i < s->Members.Size; ++i) {
        int j = BuildingArrayAdd(dst);                        // 0x5467c0
        dst->Array[j].Unit = s->Members.Array[i].Unit;
        dst->Array[j].Owner = s->WorldIndex;
        BuildingUnitAt(dst->Array[j].Unit)->MainGunner = 0;   // +0x44
    }
    BuildingArraySetSize(&s->Members, 0);                     // 0x546ca0(0)
    s->Unplace();                                             // +0x4c
    s->Parent = b->WorldIndex;
    s->IsStored = true;
    s->StoreMode = mode;
    BuildingArraySetSize(slots, dst->Size);                   // 0x546af0
    for (int i = 0; i < dst->Size; ++i) {
        SUnit* m = BuildingUnitAt(dst->Array[i].Unit);
        m->Parent = b->WorldIndex;
        m->Pos[1] = b->InsideY;                               // +0x90 = +0x358
        float d;
        memcpy(&d, &win[i][2], 4);
        m->Place(win[i][0], win[i][1], d);                    // +0x50 (HD reads win[i] unchecked)
        if (i >= slots->Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "int", i);
        slots->Array[i] = i;
        int st = m->Anim ? static_cast<SUnitAnimation*>(m->Anim)->Proto->FindState("stand") : 0;   // 0x5c7ed0
        m->SetGlobalState(st, 0);                             // 0x5b7390
    }
    s->SetGlobalState(0, 0);
}

// PANZERS 0x54d380
// A squad enters (0x5b7040 decides): the players that can see it (or are
// on its side) lose sight of the building's state (+0x2cc). A bunker (type
// 5) just stores it (members unplaced); otherwise the first squad stands at
// the "p0N" windows (+0x178 / +0x3fc) and the building takes its main
// gunner, behaviour and player; a second one stands at the "paN" windows
// (+0x408 / +0x414) and the two fight each other (RefreshMisc 0x54b910).
bool SBuildingUnit::StoreUnit(int unit, int mode)
{
    if (BuildingUnitAt(unit)->Proto->ClassType != 5 || !CanStoreUnit(unit))   // 0x5b7040
        return false;
    SUnit* s = BuildingUnitAt(unit);
    if (s->Proto->HeroPicture > -1)
        mode = 2;
    for (int p = 0; p < 12; ++p) {
        bool same = SameSide(s->Player, p);
        int kind = *(int*)(g_World->Players[p] + 0x08);
        if (same || ((kind == 1 || kind == 0) && g_GameLogic && g_GameLogic->CanSeeGroundUnit(p, s)))   // 0x562760
            _2cc[p] = false;
    }
    if (P->BuildingType == 5) {
        int si = BuildingArrayAdd(&Stored);                   // 0x546830
        Stored.Array[si].Unit = unit;
        Stored.Array[si].Mode = mode;
        for (int i = 0; i < s->Members.Size; ++i) {
            int j = BuildingArrayAdd(&Members);
            Members.Array[j].Unit = s->Members.Array[i].Unit;
            Members.Array[j].Owner = s->WorldIndex;
        }
        if (s->Members.Size != 0 && !s->Members.Array)
            Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "struct SUnitMember");
        s->Members.Size = 0;
        if (s->Members.Max > 0)
            memset(s->Members.Array, 0, s->Members.Max * sizeof(SUnitMember));
        s->Unplace();                                         // +0x4c
        s->Parent = WorldIndex;
        s->IsStored = true;
        s->StoreMode = mode;
        for (int i = 0; i < Members.Size; ++i) {
            SUnit* m = BuildingUnitAt(Members.Array[i].Unit);
            m->Parent = WorldIndex;
            m->Unplace();                                     // +0x4c
        }
        s->TransferSelection(this);                           // 0x5c50c0
        _110 = false;
        Player = s->Player;
    } else if (Stored.Size == 1) {
        int si = BuildingArrayAdd(&Stored);
        Stored.Array[si].Unit = unit;
        Stored.Array[si].Mode = mode;
        MoveSquadInside(this, s, mode, &Members2, &WindowSlots2, WindowA);
        StopGunners();                                        // +0xec
        MainGunner = -1;
        s->TransferSelection(this);                           // 0x5c50c0
        Player = s->Player;
        g_World->UnitStored(WorldIndex, s->Player);           // 0x5ef760
    } else if (Stored.Size == 0) {
        int si = BuildingArrayAdd(&Stored);
        Stored.Array[si].Unit = unit;
        Stored.Array[si].Mode = mode;
        MoveSquadInside(this, s, mode, &Members, &WindowSlots, WindowB);
        MainGunner = s->MainGunner;
        s->TransferSelection(this);                           // 0x5c50c0
        Behavior = s->Behavior;                               // +0x250
        _110 = false;
        Player = s->Player;
        g_World->UnitStored(WorldIndex, s->Player);           // 0x5ef760
    } else {
        return false;
    }
    if (g_GameLogic)
        g_GameLogic->Dispatch_571380(WorldIndex, s->WorldIndex);   // 0x571380
    return true;
}

// PANZERS 0x5c50c0
// SUnit: the selection (+0x104 bit 0) and +0x108 move to `to`.
void SUnit::TransferSelection(SUnit* to)
{
    if (_104 & 1) {
        to->_104 |= 1;
        _104 &= ~1u;
    }
    to->_108 |= _108;
    _108 = 0;
}

// PANZERS 0x548b20
void SBuildingUnit::GetClassDescriptor(void** obj, const SUnitClassDesc** desc)
{
    *obj = this;
    *desc = &kUnitClassDesc_8da7f8;
}

// PANZERS 0x549af0
bool SBuildingUnit::IsCapturable()
{
    return P->BuildingType == 3;
}

// PANZERS 0x548560
// A model node's {x, z, dir} (dir = atan2 of the node's axis x / z); its y
// becomes +0x358. Without the node: the building's own x, z and 0.
bool SBuildingUnit::GetNodePoint(float* out, const char* node)
{
    int n = Model->FindNode(node);                            // +0x40
    if (n > -1) {
        float pos[3] = { 0.0f, 0.0f, 0.0f };
        float axis[3] = { 0.0f, 0.0f, 0.0f };
        Model->GetNodePositionAxis(n, pos, axis);             // +0x50
        out[0] = pos[0];
        out[1] = pos[2];
        out[2] = DAtan2f((double)axis[0], (double)axis[2]);   // 0x78d07a, fstp float
        InsideY = pos[1];
        return true;
    }
    out[0] = Pos[0];
    out[1] = Pos[2];
    out[2] = 0.0f;
    return false;
}

// PANZERS 0x548660
// +0x358 = the ground under the building (then the node heights), the
// "entrance", the windows "pa0".."pa4" (+0x370) and "p00".."p04" (+0x3ac),
// the views "view1".. (+0x3e8) with their points "pN0".."pN4" and +0x3f4 =
// the farthest view; the list ends at the first missing view (that last
// entry is dropped), then the block cells (0x5497a0).
void SBuildingUnit::InitViewPoints()
{
    InsideY = g_World->GetTerrainHeight(Pos[0], Pos[2]);      // 0x5e7730
    GetNodePoint(Entrance, "entrance");
    char name[32];
    for (int i = 0; i < 5; ++i) {
        sprintf(name, "pa%d", i);                             // 0x7f4380
        GetNodePoint(WindowA[i], name);
        sprintf(name, "p0%d", i);                             // 0x7f4388
        GetNodePoint(WindowB[i], name);
    }
    RangeBonus = 0.0f;                                        // +0x3f4
    if (WindowSets.Size != 0 && !WindowSets.Array)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "SBuildingView");
    WindowSets.Size = 0;
    if (WindowSets.Max > 0)
        memset(WindowSets.Array, 0, WindowSets.Max * sizeof(SBuildingView));
    for (;;) {
        if (WindowSets.Size == WindowSets.Max) {              // SDArray::Add growth
            int m = WindowSets.Max < 0x10 ? 0x10 : WindowSets.Max * 6 / 5;
            WindowSets.Array = (SBuildingView*)realloc(WindowSets.Array, m * sizeof(SBuildingView));
            memset(WindowSets.Array + WindowSets.Max, 0, (m - WindowSets.Max) * sizeof(SBuildingView));
            WindowSets.Max = m;
        }
        int idx = WindowSets.Size;
        int n = ++WindowSets.Size;
        sprintf(name, "view%d", n);
        SBuildingView& v = WindowSets.Array[idx];
        if (!GetNodePoint(v.Pos, name)) {
            // SDArray::Remove(idx)
            WindowSets.Size--;
            int rest = WindowSets.Size - idx;
            if (rest != 0)
                memmove(&WindowSets.Array[idx], &WindowSets.Array[idx + 1], rest * sizeof(SBuildingView));
            memset(&WindowSets.Array[WindowSets.Size], 0, sizeof(SBuildingView));
            if (g_GameLogic)
                InitBlockCells();                             // 0x5497a0
            return;
        }
        float dx = WindowSets.Array[idx].Pos[0] - Pos[0];
        float dz = WindowSets.Array[idx].Pos[1] - Pos[2];
        float d = (float)sqrt((double)(dx * dx + dz * dz));   // 0x78d090
        if (d > RangeBonus) {
            float ex = WindowSets.Array[idx].Pos[0] - Pos[0];
            float ez = WindowSets.Array[idx].Pos[1] - Pos[2];
            RangeBonus = (float)sqrt((double)(ex * ex + ez * ez));
        }
        for (int k = 0; k < 5; ++k) {
            sprintf(name, "p%d%d", n, k);                     // 0x7f4398
            GetNodePoint(WindowSets.Array[idx].Points[k], name);
        }
    }
}

// PANZERS 0x5497a0
// +0x458: the vis-map cells (SGameLogic +0x1c8 wide, 2 per metre,
// truncated) of the "Block" node points, or the building's own cell.
void SBuildingUnit::InitBlockCells()
{
    float (*pts)[3] = nullptr;
    int n = ModelNodePoints(Model, "Block", &pts);            // +0xac
    BlockCells.Size = 0;                                      // 0x546b60(0)
    for (int i = 0; i < n; ++i) {
        int cx = (int)(pts[i][0] * 2.0f);                     // DAT_007f4558, fistp RC=chop (0xc7f)
        int cz = (int)(pts[i][2] * 2.0f);
        if (BlockCells.Size == BlockCells.Max) {
            int m = BlockCells.Max < 0x10 ? 0x10 : BlockCells.Max * 6 / 5;
            BlockCells.Array = (int*)realloc(BlockCells.Array, m * sizeof(int));
            memset(BlockCells.Array + BlockCells.Max, 0, (m - BlockCells.Max) * sizeof(int));
            BlockCells.Max = m;
        }
        BlockCells.Array[BlockCells.Size++] = g_GameLogic->VisW * cz + cx;
    }
    if (BlockCells.Size == 0) {
        int cx = (int)(Pos[0] * 2.0f);
        int cz = (int)(Pos[2] * 2.0f);
        if (BlockCells.Size == BlockCells.Max) {
            int m = BlockCells.Max < 0x10 ? 0x10 : BlockCells.Max * 6 / 5;
            BlockCells.Array = (int*)realloc(BlockCells.Array, m * sizeof(int));
            memset(BlockCells.Array + BlockCells.Max, 0, (m - BlockCells.Max) * sizeof(int));
            BlockCells.Max = m;
        }
        BlockCells.Array[BlockCells.Size++] = g_GameLogic->VisW * cz + cx;
    }
    free(pts);
}

// PANZERS 0x5468a0
void SBuildingUnit::StoreInterpolationState()
{
    SUnit::StoreInterpolationState();                         // 0x5b5ad0
    if (Model448)
        Model448->StoreInterpolationState();                  // +0x3c
}

// PANZERS 0x549b10
void SBuildingUnit::SetOnBlockMap(bool on)
{
    (void)on;
}

// PANZERS 0x549b20
void SBuildingUnit::MarkBlockMap(bool on, int p2, int p3, int p4, short p5)
{
    (void)on; (void)p2; (void)p3; (void)p4; (void)p5;
}

// PANZERS 0x546ad0
int SBuildingUnit::TestBlockMap(int p1, int p2, int p3, int p4, short p5)
{
    (void)p1; (void)p2; (void)p3; (void)p4; (void)p5;
    return 0;
}

// PANZERS 0x5468e0
// Called by SBuildingAnimation::UpdateModel 0x5cb650 for the local player:
// true when the building shows its interior (no fog of war and occupied,
// one of its first two occupants is on the player's side, or a hangar the
// player's side holds).
bool SBuildingUnit::IsOccupiedByTeam(int player)
{
    if (g_World->_4d0[0] && Stored.Size != 0)                   // World+0x4d0
        return true;
    int n = Stored.Size;
    if (n > 0) {
        SUnit* a = BuildingUnitAt(Stored.Array[0].Unit);
        if (SameSide(a->Player, player))
            return true;
        n = Stored.Size;
    }
    if (n > 1) {
        if (Stored.Size < 2)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SStoredUnitProperties", 1);
        SUnit* b = BuildingUnitAt(Stored.Array[1].Unit);
        if (SameSide(b->Player, player))
            return true;
    }
    if (P->BuildingType == 6) {
        for (int i = 0; i < 12; ++i)
            if (_43c[i] && SameSide(player, i))
                return true;
    }
    return false;
}

// PANZERS 0x548240
// Capturable buildings: the capture range; occupied ones: the first
// member's first gunner's MaxRange; else 0.
float SBuildingUnit::GetMaxRange(int weapon)
{
    (void)weapon;
    if (P->BuildingType == 3)
        return P->CaptureRange;
    if (Stored.Size != 0 && Members.Size != 0) {
        if (Members.Size < 1)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SStoredMemberProperties", 0);
        SUnit* m = BuildingUnitAt(Members.Array[0].Unit);         // 0x546490
        if (m->Gunners.Size < 1)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SGunner", 0);
        return m->Gunners.Array[0]->GetPGunner()->MaxRange;       // +0x2c, +0x38
    }
    return 0.0f;
}

// PANZERS 0x5482e0
// Occupied buildings: the first member's first gunner's MinRange; else 0.
float SBuildingUnit::GetMinRange(int weapon)
{
    (void)weapon;
    if (Stored.Size == 0 || Members.Size == 0)
        return 0.0f;
    if (Members.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SStoredMemberProperties", 0);
    SUnit* m = BuildingUnitAt(Members.Array[0].Unit);             // heap check inline
    if (m->Gunners.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SGunner", 0);
    return m->Gunners.Array[0]->GetPGunner()->MinRange;           // +0x2c, +0x34
}

// PANZERS 0x548b40
// A building with one stored unit sees with that unit's sight (tail call to
// its +0x184); otherwise only a capturable (type 3) building that is not
// +0x110 has its own prototype sight (+0x84), the rest 0. The recompile used
// SUnit 0x5ba240 (the prototype sight for every building), so occupied
// towers saw 35 m instead of the squad's range.
float SBuildingUnit::GetSightRange()
{
    if (Stored.Size != 1) {
        if (P->BuildingType == 3 && !_110)
            return Proto->Sight;                                  // SPUnit +0x84
        return 0.0f;
    }
    int u = Stored.Array[0].Unit;                                 // +0x16c[0]
    if (!g_World->Units.IsLive(u))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", u);
    return g_World->Units.Array[u].Unit->GetSightRange();         // +0x184
}

// PANZERS 0x54a250
void SBuildingUnit::RefreshTargeting()
{
    if (IsWaitingAfterStuck())                                    // 0x5bb400
        return;
    if (g_GameLogic && g_GameLogic->IsPaused() && !_1d4)         // 0x56e150
        return;
    int bt = P->BuildingType;
    if (bt == 3) {
        FillNearUnits(P->CaptureRange, 0.0f);                 // 0x5b78a0
        // HD then: "multi radar" 0x54ac00, unit type 0x1a 0x54a660,
        // "Support place" / "Support place desert" 0x54acd0, else 0x54cd70.
        STUB_LOG("SBuildingUnit::RefreshTargeting (0x54a250) capturable building");
    } else if (bt == 6) {
        STUB_LOG("SBuildingUnit::RefreshTargeting (0x54a250) hangar 0x54a380");
    } else {
        float range = GetMaxRange(0) + RangeBonus;                // +0x17c, fadd +0x3f4
        FillNearUnits(range, 0.0f);                 // 0x5b78a0
    }
    AI_Heartbeat();                                               // +0x190 (tail jump)
}

// PANZERS 0x546500
// Only an occupied building (one stored unit) of a non-passive player
// looks for targets; the menu house is empty.
void SBuildingUnit::AI_Heartbeat()
{
    if (IsWaitingAfterStuck())                                    // 0x5bb400
        return;
    if (Unplaced || Wrecked)                                      // +0x168, +0x150
        return;
    if (g_GameLogic && g_GameLogic->IsPaused() && !_1d4)         // 0x56e150
        return;
    if (*(int*)(g_World->Players[Player] + 0x08) == 2)           // World+0x178 + player * 0x48
        return;
    if (Stored.Size != 1 || Behavior == 2)
        return;
    // A kind-3 target that does not hold (+0x20 != 1) is dropped; without
    // a primary target the first occupant's main weapon looks for one
    // within [min - bonus, max + bonus] (0x5b4720) and the building orders
    // it (kind 3, held), or stops its gunners when there is none.
    if (CurrentTarget && CurrentTarget->Kind == 3 && CurrentTarget->Mode != 1)
        ClearTargets();                                           // +0xc4
    if (PrimaryTarget)
        return;
    for (int w = 0; w < 1; ++w) {
        float minR = 0.0f;
        if (0.0f <= GetMinRange(w) - RangeBonus)                  // +0x180
            minR = GetMinRange(w) - RangeBonus;
        float maxR = GetMaxRange(w) + RangeBonus;                 // +0x17c
        if (Stored.Size < 1)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SStoredUnitProperties", 0);
        SUnit* occ = BuildingUnitAt(Stored.Array[0].Unit);
        if (occ->Gunners.Size < 1)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
        int t = FindTarget(0, occ->Gunners.Array[0], minR, maxR, true);   // 0x5b4720
        if (t < 0) {
            if (CurrentTarget)
                StopGunners();                                    // +0xec
        } else if (!CurrentTarget || CurrentTarget->Type != 0 || CurrentTarget->Unit != t) {
            STarget* nt = STarget::Create(3);                     // new 0x38, 0x5b27c0(3)
            nt->Type = kTargetUnit;                               // 0x5c21b0
            nt->Unit = t;
            nt->Mode = 1;
            SetCurrentTarget(nt, 0);                              // +0xa0
        }
    }
}

// PANZERS 0x54b910
void SBuildingUnit::RefreshMisc()
{
    if (ActiveDriver >= 0) {
        if (ActiveDriver >= Drivers.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SIDriver *", ActiveDriver);
        Drivers.Array[ActiveDriver]->Refresh();                   // +0x14
    }
    if (Unplaced)                                                 // +0x168
        return;
    if (P->BuildingType != 5) {
        RefreshOccupants();                                       // HD inline (0x54b955..0x54c291)
        for (int i = 0; i < Gunners.Size; ++i)
            Gunners.Array[i]->ServerRefresh();                    // +0x14
        if (_118 > 0.0f)
            _118 -= 0.05f;                                        // 0x7f4534
        for (int i = 0; i < Members.Size; ++i)
            BuildingUnitAt(Members.Array[i].Unit)->ServerRefresh(LastRefreshFrame);   // +0x2c
        for (int i = 0; i < Members2.Size; ++i)
            BuildingUnitAt(Members2.Array[i].Unit)->ServerRefresh(LastRefreshFrame);
        for (int i = 0; i < Inside.Size; ) {
            int u = Inside.Array[i];
            if (g_World->Units.IsLive(u)) {
                BuildingUnitAt(u)->ServerRefresh(LastRefreshFrame);
                ++i;
            } else {
                Inside.Size--;                                    // SDArray<int>::Remove
                if (Inside.Size - i != 0)
                    memmove(&Inside.Array[i], &Inside.Array[i + 1], (Inside.Size - i) * sizeof(int));
                Inside.Array[Inside.Size] = 0;
            }
        }
    }
    RefreshRepairTarget(P->CaptureRange);                         // 0x5c01e0
    RefreshSupplyTarget(P->CaptureRange);                         // 0x5bf280
    ServerRefreshMedic(P->CaptureRange);                          // +0x1c0
    if (P->BuildingType == 2) {
        for (int p = 0; p < 12; ++p) {
            if (*(int*)((unsigned char*)g_GameLogic + 0x234 + p * 4) == g_GameLogic->Frame &&
                g_GameLogic->CanSeeGroundUnit(p, this))           // 0x562760
                _2cc[p] = _110;
        }
    }
}

static int Bits(float f)
{
    int b;
    memcpy(&b, &f, 4);
    return b;
}

static double FoldAngle(double d)                             // |a - b| folded to [0, pi]
{
    d = fabs(d);                                              // andps 0x7ea7a0
    return d > kHdPi ? kHdTwoPi - d : d;                      // 0x7f4560 / 0x7f4570
}

// One occupant of a building held by two squads: without a target it picks
// a random enemy of the other list (world RNG) and attacks it; its gunner
// gets the +0x60 draw. Returns false when it still has no target (HD skips
// the move then); else `dir` faces the gunner's (refreshed) target.
static bool OccupantDuel(int m, int i, const SUnitArray<SUnitMember>& enemies, float px, float pz, float* dir)
{
    SUnit* mu = BuildingUnitAt(m);
    if (mu->Gunners.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
    if (!mu->Gunners.Array[0]->Target) {
        int r = WorldRand();
        int j = (int)((double)r * 3.0517578125e-05 * (double)enemies.Size);   // DAT_007f4540
        if (j < 0 || j >= enemies.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", j);
        mu->EC_Attack(enemies.Array[j].Unit, 0);              // +0xe8
        SUnit* m2 = BuildingUnitAt(m);
        if (m2->Gunners.Size < 1)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
        if (!m2->Gunners.Array[0]->Target)
            return false;
        int r2 = WorldRand();
        // DAT_007f4598 = -1/32768: the truncated product is always 0.
        BuildingUnitAt(m)->Gunners.Array[0]->_60 = (i != 0) * 2 - (int)((double)r2 * -3.0517578125e-05);
    }
    BuildingUnitAt(m)->Gunners.Array[0]->Target->Refresh(m);  // 0x5bd210
    STarget* t = BuildingUnitAt(m)->Gunners.Array[0]->Target;
    *dir = DAtan2f((double)(t->Pos[0] - px), (double)(t->Pos[2] - pz));   // 0x78d07a, fstp float
    return true;
}

// HD inline in RefreshMisc 0x54b910: the occupants take their windows. With
// a position target the view set facing it (+0x3f8) gives the first squad's
// spots and they face the target; two squads inside fight each other.
void SBuildingUnit::RefreshOccupants()
{
    bool both = Members.Size != 0 && Members2.Size != 0;
    bool aiming = Members.Size != 0 && CurrentTarget &&
                  (CurrentTarget->Kind == 2 || CurrentTarget->Kind == 3);
    WindowSet = -1;                                           // +0x3f8
    if (!both && aiming && WindowSets.Size != 0) {
        float a = DAtan2f((double)(CurrentTarget->Pos[0] - Pos[0]), (double)(CurrentTarget->Pos[2] - Pos[2]));
        float best = 6.2831855f;                              // DAT_007f458c
        for (int s = 0; s < WindowSets.Size; ++s) {
            double d = FoldAngle((double)a - (double)WindowSets.Array[s].Pos[2]);   // 0x546330
            if ((double)best > d) {
                double d2 = FoldAngle((double)a - (double)WindowSets.Array[s].Pos[2]);
                WindowSet = s;
                best = (float)d2;
            }
        }
    }
    for (int c = 0; c < Members.Size; ++c) {
        if (c >= WindowSlots.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "int", c);
        int slot = WindowSlots.Array[c];
        float px, pz, dir;
        if (aiming) {
            if (WindowSet < 0) {
                px = WindowB[slot][0];
                pz = WindowB[slot][1];
            } else {
                if (WindowSet >= WindowSets.Size)
                    Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SBuildingView", WindowSet);
                px = WindowSets.Array[WindowSet].Points[slot][0];   // HD flat index set * 6 + slot + 1
                pz = WindowSets.Array[WindowSet].Points[slot][1];
            }
            dir = DAtan2f((double)(CurrentTarget->Pos[0] - px), (double)(CurrentTarget->Pos[2] - pz));
        } else {
            px = WindowB[slot][0];
            pz = WindowB[slot][1];
            dir = WindowB[slot][2];
        }
        int m = Members.Array[c].Unit;
        if (both && !OccupantDuel(m, c, Members2, px, pz, &dir))
            continue;
        BuildingUnitAt(m)->EC_Move(Bits(px), Bits(pz), 0, true, Bits(dir));   // +0xac
    }
    for (int c = 0; c < Members2.Size; ++c) {
        if (c >= WindowSlots2.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "int", c);
        int slot = WindowSlots2.Array[c];
        float px = WindowA[slot][0];
        float pz = WindowA[slot][1];
        float dir = WindowA[slot][2];
        int m = Members2.Array[c].Unit;
        if (both && !OccupantDuel(m, c, Members, px, pz, &dir))
            continue;
        BuildingUnitAt(m)->EC_Move(Bits(px), Bits(pz), 0, true, Bits(dir));   // +0xac
    }
}

// PANZERS 0x546d20
// The health bar over the building (board elements +0x350 / +0x354) when
// it is selected and in front of the camera; otherwise both are hidden.
void SBuildingUnit::UpdateVisuals(SIViewport* vp)
{
    if (!Model)
        return;
    float pos[3] = { 0.0f, 0.0f, 0.0f };
    Model->GetRenderPosition(pos);                                // +0x14
    float sx, sy, size, sz;
    int fog;
    vp->ProjectToScreen(pos, 1.0f, &sx, &sy, &size, &sz, &fog);   // +0x3c
    if ((!g_GameLogic || !g_GameLogic->IsPaused()) && _104 != 0 && size >= 0.0f) {
        // HD: board +0x18(+0x350 / +0x354, 1) and their position / size
        // and HP colour. The recompile creates no board elements (as the
        // squads' 0x599620), so there is nothing to show.
        return;
    }
    // HD: board +0x18(+0x350, 0) and +0x18(+0x354, 0) hide the bar. The
    // recompile creates no board elements (Board350 / Board354 stay 0).
}

// HD 0x549b70: an occupant died: RemoveStoredMember 0x5be140, its window
// slot goes, an empty building unloads (0x5c51d0). Not lifted yet.
void SBuildingUnit::OnMemberDied(int unit)
{
    (void)unit;
    STUB_LOG("SBuildingUnit::OnMemberDied (0x549b70) occupant died");
    PZ_M3_TRACE("SBuildingUnit::OnMemberDied (0x549b70)");
}

// PANZERS 0x548d30
// XP of a building: without second occupants (+0x408) every stored unit gets
// it; otherwise the squad whose member row (+0x178, then +0x408) holds
// `victim` gets it (rows without a squad drop it).
void SBuildingUnit::AddXP(int victim, float xp, int p3)
{
    if (*(const int*)(g_World->Players[Player] + 8) == 4)
        Player = g_World->LocalPlayer;
    if (Members2.Size < 1) {
        for (int i = 0; i < Stored.Size; ++i)
            BuildingUnitAt(Stored.Array[i].Unit)->AddXP(victim, xp, p3);   // +0x8c
        return;
    }
    for (int i = 0; i < Members.Size; ++i) {
        if (Members.Array[i].Unit != victim)
            continue;
        int owner = Members.Array[i].Owner;
        if (owner != -1) {
            BuildingUnitAt(owner)->AddXP(victim, xp, p3);
            return;
        }
        break;
    }
    for (int i = 0; i < Members2.Size; ++i) {
        if (Members2.Array[i].Unit != victim)
            continue;
        int owner = Members2.Array[i].Owner;
        if (owner == -1)
            return;
        BuildingUnitAt(owner)->AddXP(victim, xp, p3);
        return;
    }
}

} // namespace pz
