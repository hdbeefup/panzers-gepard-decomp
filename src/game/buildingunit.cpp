// src/game/buildingunit.cpp
// SBuildingUnit (0x545940..0x5500a0). OWNER: agent U. See buildingunit.h.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "buildingunit.h"
#include "campaign.h"
#include "pz/iscene.h"
#include "packets.h"
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
// (+0x350 / +0x354): unitboard.cpp.
void SBuildingUnit::Init(SUnitDef* def)
{
    SUnit::Init(def);                                         // 0x5ba8e0
    CreateBoardElements();                                    // board +0x08(4, ...) twice
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
        UpdateCaptureFlag();                                  // 0x5471d0
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
                Model448->Slot_E0();                          // +0xe0 0x6d7310 (detach the flag)
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
// The "Block" footprint of the model's current pose, with the caller's mask
// plus bit 4 (the block-map rebuild 0x604620 passes mask & 0x44, so a
// rebuild keeps bit 4 and sets the 0x40 that clears road bit 0x2000 around
// the building). The position / direction arguments are not used.
void SBuildingUnit::MarkBlockMap(bool on, int p2, int p3, int p4, short p5)
{
    (void)p2; (void)p3; (void)p4;
    SBlockBitmap* bm = Model->BuildNodeBlockBitmap(4, "Block");   // +0xa8
    BlockMap_ApplyBitmap(g_World, bm, on, (unsigned)(unsigned short)p5 | 4u);   // 0x5f4910
    if (bm)
        FreeBuildingBitmap(bm);                                   // 0x661b30, delete(0x1c)
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
        if (Proto->Name.size == 0 || _stricmp(SStr(Proto->Name), "multi radar") != 0) {   // +0x64, +0x60
            if (P->UnitType == 0x1a)                          // +0x44
                STUB_LOG("SBuildingUnit::RefreshTargeting (0x54a250) productive building 0x54a660");
            else if (_stricmp(SStr(Proto->Name), "Support place") == 0 ||   // 0x52c410
                     _stricmp(SStr(Proto->Name), "Support place desert") == 0)
                RefreshSupportPlace();                        // 0x54acd0
            else
                RefreshCapture();                             // 0x54cd70
        } else {
            RefreshRadar();                                   // 0x54ac00
        }
    } else if (bt == 6) {
        RefreshHangar();                                      // 0x54a380
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

// SBuildingUnit::UpdateVisuals 0x546d20: unitboard.cpp (M5-VX).

// SDArray<T>::Remove (inline in HD): memmove the tail down, clear the freed
// last element.
template <typename T>
static void BuildingArrayRemove(SUnitArray<T>* a, int i, const char* type)
{
    if (i >= a->Size)
        Logger.g->Panic("SDArray<%s>::Remove: invalid index (%d) size = %d", type, i, a->Size);
    --a->Size;
    if (a->Size - i != 0)
        memmove(a->Array + i, a->Array + i + 1, (a->Size - i) * sizeof(T));   // 0x76bb40
    memset(&a->Array[a->Size], 0, sizeof(T));
}

// The building takes main gunner / player (and behaviour) from its first
// stored squad again (0x549d62..0x549e48 / 0x54a03f..0x54a0e1).
static SUnit* FirstStoredSquad(SBuildingUnit* b)
{
    if (b->Stored.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitStored", 0);
    return BuildingUnitAt(b->Stored.Array[0].Unit);
}

// PANZERS 0x549b70
// An occupant died. A member of the first squad (+0x178 / windows +0x3fc):
// it leaves the list (0x5be140) with its window slot; when none is left the
// squad gets out (0x5c51d0) and dies (+0x124), the second squad (+0x408 /
// +0x414) becomes the first, the guns stop (+0xec) and the building takes
// main gunner, player and behaviour from the remaining stored squad, or
// becomes neutral (+0x110, 0x5ef760) when none is left. A member of the
// second squad: as above without the move; when it is empty every stored
// unit gets out (+0x68). Either way the dead unit goes on the +0x420 list;
// a unit in neither list is ignored.
void SBuildingUnit::OnMemberDied(int unit)
{
    PZ_M3_TRACE("SBuildingUnit::OnMemberDied (0x549b70)");
    for (int i = 0; i < Members.Size; ++i) {
        if (Members.Array[i].Unit != unit)
            continue;
        int squad = Members.Array[i].Owner;
        RemoveStoredMember(i);                                    // 0x5be140
        BuildingArrayRemove(&WindowSlots, i, "int");
        if (Members.Size == 0) {
            SUnit::UnloadUnit(squad);                             // 0x5c51d0 (direct call)
            BuildingUnitAt(squad)->EC_Die();                      // +0x124
            BuildingArraySetSize(&Members, Members2.Size);        // 0x546ca0
            for (int k = 0; k < Members.Size; ++k)
                Members.Array[k] = Members2.Array[k];
            BuildingArraySetSize(&WindowSlots, WindowSlots2.Size); // 0x546af0
            for (int k = 0; k < WindowSlots.Size; ++k)
                WindowSlots.Array[k] = WindowSlots2.Array[k];
            BuildingArraySetSize(&WindowSlots2, 0);               // 0x546af0(0)
            BuildingArraySetSize(&Members2, 0);                   // 0x546ca0(0)
            StopGunners();                                        // +0xec
            if (Stored.Size == 0) {
                _110 = true;
                g_World->UnitStored(WorldIndex, -1);              // 0x5ef760
            } else {
                MainGunner = FirstStoredSquad(this)->MainGunner;  // +0x44
                Player = FirstStoredSquad(this)->Player;          // +0xfc
                Behavior = FirstStoredSquad(this)->Behavior;      // +0x250
            }
        }
        int k = BuildingArrayAdd(&Inside);                        // +0x420
        Inside.Array[k] = unit;
        return;
    }
    for (int i = 0; i < Members2.Size; ++i) {
        if (Members2.Array[i].Unit != unit)
            continue;
        int squad = Members2.Array[i].Owner;
        BuildingArrayRemove(&Members2, i, "struct SUnitMember");
        BuildingArrayRemove(&WindowSlots2, i, "int");
        if (Members2.Size == 0) {
            UnloadAll();                                          // +0x68
            BuildingUnitAt(squad)->EC_Die();                      // +0x124
            StopGunners();                                        // +0xec
            MainGunner = FirstStoredSquad(this)->MainGunner;
            Player = FirstStoredSquad(this)->Player;
        }
        int k = BuildingArrayAdd(&Inside);
        Inside.Array[k] = unit;
        return;
    }
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

// PANZERS 0x548c10 (SBuildingUnit +0xa8)
// Attack (3) an enemy; heal (9) a squad with wounded members, repair (7) /
// supply (8) with cargo left; else nothing.
int SBuildingUnit::ActionOn(int target)
{
    if (!IsTargetable(target, true) || target == WorldIndex)
        return 0;
    if (GetUnitRelation(target, Player) == -1)
        return 3;
    const unsigned char* p = (const unsigned char*)P;
    if (!g_World->Units.IsLive(target))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", target);
    SUnit* t = g_World->Units.Array[target].Unit;
    if (p[0xde] && t->HasWoundedMember())                         // target +0x80
        return 9;
    if (p[0xdf] && t->NeedsRepair() && 0.0f < Cargo)
        return 7;
    if (p[0xe0] && t->NeedsSupply(1.0f) && 0.0f < Cargo)
        return 8;
    return 0;
}

// ---------------------------------------------------------------------------
// Capturable buildings and hangars (M5-MS)

void UnitSetPlayer(SUnit* u, int player);                         // 0x5c1630 (triggers.cpp)

static float FBitsF(unsigned b)
{
    float f;
    memcpy(&f, &b, 4);
    return f;
}

// PANZERS 0x5471d0
// SBuildingUnit::CreateCaptureFlag: an owned building (not +0x110) gets the
// flag model (prototype +0x14c, multiplayer +0x150), attached to the "flag"
// node or 2 above the building, the flag's "Block" node hidden; the flag
// texture is scrolled to the owner's race (multiplayer: the player colour).
// A neutral building drops the model.
void SBuildingUnit::UpdateCaptureFlag()
{
    if (_110) {
        if (Model448) {
            if (FlagNode >= 0)
                Model448->Slot_E0();                          // +0xe0 (detach)
            if (Model448) {
                Model448->Release();                          // +0x04
                Model448 = nullptr;
            }
        }
        return;
    }
    bool multi = g_Campaign && g_Campaign->IsMultiMode();     // 0x594d20 (&& !0x594cd0 coop: no SMulti here)
    int proto = multi ? P->FlagMultiProto : P->FlagProto;     // +0x150 / +0x14c
    if (proto < 0)
        return;
    if (!Model448) {
        Model448 = g_Scene->CreateModelFromPrototype(proto, 1);   // scene +0x58
        Model448->SetFlags(5);                                // +0x94
        if (FlagNode < 0)
            Model448->SetPosition(Pos[0], Pos[1] + 2.0f, Pos[2]);   // +0x18, 0x7f4558
        else
            Model448->AttachTo(Model, FlagNode);              // +0xdc
        Model448->SetVisible(true, false);                    // +0x30(1, 0)
        Model448->SetNodeVisible(Model->FindNode("Block"), false);   // flag +0x60(building model +0x40("Block"), 0)
        Model448->StoreInterpolationState();                  // +0x3c
    }
    const unsigned char* rec = g_World->Players[Player];
    if (multi) {
        static const unsigned kU[5] = { 0, 0x3e3e76c9, 0x3ebe76c9, 0x3f0e978d, 0x3f3e353f };
        static const unsigned kV[3] = { 0, 0x3eab020c, 0x3f2ac083 };
        int c = *(const int*)(rec + 0x00);                    // World+0x170: the player colour
        if (c < 0 || c > 14)
            Logger.g->Panic("SBuildingUnit::CreateCaptureFlag() - invalid Player Color");
        Model448->SetNodeTexScroll(0, FBitsF(kU[c % 5]), FBitsF(kV[c / 5]));   // +0x68
        return;
    }
    switch (*(const int*)(rec + 0x04)) {                      // World+0x174: the race
    case 0: Model448->SetNodeTexScroll(0, 0.0f, 0.0f); return;
    case 1: Model448->SetNodeTexScroll(0, 0.25f, 0.0f); return;
    case 2: Model448->SetNodeTexScroll(0, 0.5f, 0.0f); return;
    case 3: Model448->SetNodeTexScroll(0, 0.25f, 0.5f); return;
    case 4: Model448->SetNodeTexScroll(0, 0.75f, 0.0f); return;
    case 5: Model448->SetNodeTexScroll(0, 0.0f, 0.5f); return;
    default:
        Logger.g->Panic("SBuildingUnit::CreateCaptureFlag() - invalid Player Race");
    }
}

// PANZERS 0x54cd70
// The units in capture range (+0x1b4; not aircraft / projectiles; buildings
// only of building type 0 or 2; not +0x110; of a player still in the game)
// decide the owner: while one of the owner's own units is there it stays
// (a neutral building becomes owned); else the building goes to the last
// allied player seen, else to the last enemy player seen. A change clears
// +0x110 and +0x2cc..+0x2d7 and renews the flag.
void SBuildingUnit::RefreshCapture()
{
    int own = -1, allied = -1, enemy = -1;
    if (SightUnits.Size <= 0)
        return;
    for (int i = 0; i < SightUnits.Size; ++i) {
        if (!IsTargetable(SightUnits.Array[i].Unit, false))   // 0x5bb6b0(unit, 0)
            continue;
        SUnit* u = BuildingUnitAt(SightUnits.Array[i].Unit);
        int ct = u->Proto->ClassType;                         // SPUnit +0x40
        if (ct == 7 || ct == 8)
            continue;
        if (ct == 9) {
            int bt = static_cast<SBuildingUnit*>(u)->P->BuildingType;   // +0x340 +0x13c
            if (bt != 0 && bt != 2)
                continue;
        }
        if (u->_110)
            continue;
        int pl = u->Player;                                   // +0xfc
        int st = *(int*)(g_World->Players[pl] + 0x08);        // World+0x178
        if (st == 2 || st == 3)
            continue;
        if (!SameSide(Player, pl)) {                          // 0x549ab0
            enemy = pl;
            continue;
        }
        if (Player == pl)
            own = pl;
        else
            allied = pl;
    }
    if (own == -1) {
        int to = allied >= 0 ? allied : enemy;
        if (to < 0)
            return;
        UnitSetPlayer(this, to);                              // 0x5c1630
    } else if (!_110) {
        return;
    }
    _110 = false;
    memset(_2cc, 0, sizeof(_2cc));                            // +0x2cc 8 bytes, +0x2d4 4 bytes
    UpdateCaptureFlag();                                      // 0x5471d0
}

// PANZERS 0x54ac00
// The radar of the multiplayer maps: captured as any building; a change of
// side adds one to four counters of the new owner (player record
// +0x30..+0x3c). (SMulti 0x8f1a74 is null here, so its +0x4c55 test passes.)
void SBuildingUnit::RefreshRadar()
{
    int old = Player;
    RefreshCapture();                                         // 0x54cd70
    if (_110 || Player == old)
        return;
    int t = PlayerTeam(Player);
    bool same = t == 0 ? Player == old : t == PlayerTeam(old);
    if (same)
        return;
    for (int k = 0; k < 4; ++k)
        *(int*)(g_World->Players[Player] + 0x30 + k * 4) += 1;   // World+0x1a0..+0x1ac
}

// PANZERS 0x548bf0
bool UnitSuppliesForFree(SUnit* u)
{
    return ((unsigned char*)u)[0x2d9] != 0 && !(g_GameLogic && g_GameLogic->IsPaused());   // 0x56e150
}

// PANZERS 0x54a4a0
// As the supply part of 0x5bd610: an idle support place with cargo orders a
// heal (target kind 8, 0x5c1030) for the first allied unit in range whose
// +0x80 says it has wounded members.
void SBuildingUnit::HealNearUnits()
{
    if (!_2eb || !(0.0f < Cargo))
        return;
    STarget* ct = CurrentTarget;                              // +0x1f4
    if (ct && (ct->Kind == 6 || ct->Kind == 7 || ct->Kind == 8 || ct->Kind == 1))
        return;
    STarget* pt = PrimaryTarget;                              // +0x1f8
    bool primFollow = pt && pt->Kind == 0 && pt->Type == 0;
    bool curFollow = ct && ct->Kind == 0 && ct->Type == 0;
    bool primKind4 = pt && pt->Kind == 4;
    bool none = !pt && !ct;
    if (!none && !primKind4 && !(primFollow && curFollow))
        return;
    for (int i = 0; i < SightUnits.Size; ++i) {
        if (!IsTargetable(SightUnits.Array[i].Unit, false))   // 0x5bb6b0
            continue;
        SUnit* u = BuildingUnitAt(SightUnits.Array[i].Unit);  // 0x5463d0 / 0x546490
        int uc = u->Proto->ClassType;
        if (uc == 7 || uc == 8 || uc == 9)
            continue;
        if (!SameSide(Player, u->Player) || !u->HasWoundedMember())   // 0x549ab0, +0x80
            continue;
        STarget* t = STarget::Create(8);                      // 0x5c1030 (new 0x38, kind 8)
        t->Type = kTargetUnit;
        t->Unit = SightUnits.Array[i].Unit;
        SetCurrentTarget(t, 0);                               // +0xa0
        return;
    }
}

// PANZERS 0x54acd0
// The support place: a unit target that left the capture range is dropped;
// without a target the place is captured as any building; an owned place
// repairs and supplies (0x5bd610(1.0)), heals (0x54a4a0) and, while idle,
// refills the cargo (+0x2ec) of each allied repairer / supporter in range
// by 1 / its prototype cargo (+0xe4) per tick, paid from the place's own
// cargo (1 / its +0xe4) unless 0x548bf0 holds for the local player's place.
void SBuildingUnit::RefreshSupportPlace()
{
    STarget* t = CurrentTarget;                               // +0x1f4
    if (t && t->Type == 0) {
        SUnit* u = BuildingUnitAt(t->Unit);
        float dx = Pos[0] - u->Pos[0];
        float dz = Pos[2] - u->Pos[2];
        float r = P->CaptureRange;                            // +0x148
        if (r * r <= dx * dx + dz * dz)
            ClearTargets();                                   // +0xc4
    }
    if (!CurrentTarget)
        RefreshCapture();                                     // 0x54cd70
    if (_110)
        return;
    AutoRepairSupply(1.0f);                                   // 0x5bd610
    HealNearUnits();                                          // 0x54a4a0
    if (CurrentTarget || !_2eb)
        return;
    for (int i = 0; i < SightUnits.Size; ++i) {
        if (!IsTargetable(SightUnits.Array[i].Unit, false))   // 0x5bb6b0
            continue;
        SUnit* u = BuildingUnitAt(SightUnits.Array[i].Unit);
        int uc = u->Proto->ClassType;
        if (uc == 7 || uc == 8 || uc == 9)
            continue;
        if (!u->Proto->Supporter && !u->Proto->Repairer)      // +0xe0, +0xdf
            continue;
        if (!SameSide(Player, u->Player))                     // 0x549ab0
            continue;
        if (!(u->Cargo < 1.0f) || !(0.0f < Cargo))            // +0x2ec, 0x7f1b58, 0x7f1038
            continue;
        if (!UnitSuppliesForFree(this) || *(int*)((unsigned char*)g_World + 0x16c) != Player)   // 0x548bf0, World+0x16c
            Cargo = Cargo - 1.0f / (float)P->Cargo;           // +0x340 +0xe4
        if (Cargo < 0.0f)
            Cargo = 0.0f;
        u->Cargo = 1.0f / (float)u->Proto->Cargo + u->Cargo;  // +0x4 +0xe4
        if (1.0f < u->Cargo)
            u->Cargo = 1.0f;
    }
}

// PANZERS 0x54a380
// A hangar marks (+0x43c) every player with a unit of class 0, 5, 10, 11 or
// 12 that is placed on its indoor block bit 0x8000.
void SBuildingUnit::RefreshHangar()
{
    memset(_43c, 0, sizeof(_43c));
    for (int i = 0; i < g_World->Units.Size; ++i) {
        if (!g_World->Units.IsLive(i))
            continue;
        SUnit* u = g_World->Units.Array[i].Unit;
        if (i == WorldIndex)                                  // +0x74
            continue;
        int ct = u->Proto->ClassType;
        if (ct != 0 && ct != 0xb && ct != 5 && ct != 0xc && ct != 10)
            continue;
        if (u->Unplaced)                                      // +0x168
            continue;
        int x, z, d;
        memcpy(&x, &u->Pos[0], 4);
        memcpy(&z, &u->Pos[2], 4);
        memcpy(&d, &u->Dir, 4);
        if (u->TestBlockMap(x, z, d, u->UnitSizeBlocks, (short)0x8000))   // +0x1a8(+0x8c, +0x94, +0xb0, +0x58, 0x8000)
            _43c[u->Player] = true;
    }
}

} // namespace pz
