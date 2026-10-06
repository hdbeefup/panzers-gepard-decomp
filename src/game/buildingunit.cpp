// src/game/buildingunit.cpp
// SBuildingUnit (0x545940..0x5500a0). OWNER: agent U. See buildingunit.h.

#include <string.h>
#include "buildingunit.h"
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


// PANZERS 0x545940
// HD also loads (GetPUnit(name, true)) the unit types every production slot
// of a productive building names (prototype +0x154, 0x68 bytes each).
SBuildingUnit::SBuildingUnit(SPBuildingUnit* proto, int worldIndex)
    : SUnit(proto, worldIndex)
{
    memset(&P, 0, sizeof(SBuildingUnit) - offsetof(SBuildingUnit, P));
    P = proto;
    for (int i = 0; i < 12; ++i)
        _2cc[i] = true;                                       // +0x2cc[i] = 1, +0x43c[i] = 0
    if (proto->Products.Size > 0)
        STUB_LOG("SBuildingUnit::SBuildingUnit (0x545940) productive building");
}

// PANZERS 0x5460c0
SBuildingUnit::~SBuildingUnit()
{
}

// PANZERS 0x548f20
// Lifted: base Init, the hidden "Block" / "Indoor" nodes. Not lifted: the
// node footprints on the static block map (model +0xa8, world 0x5f4910),
// the capture flag (type 3, 0x5471d0), the "Platform" rebuild and the
// "Block" point test (+0x454); the board elements.
void SBuildingUnit::Init(SUnitDef* def)
{
    SUnit::Init(def);                                         // 0x5ba8e0
    _110 = true;
    if (Model) {
        Model->SetNodeVisible(Model->FindNode("Block"), false);   // +0x40 / +0x60
        if (P->BuildingType == 6 || P->UnitType == 0x1a)
            Model->SetNodeVisible(Model->FindNode("Indoor"), false);
    }
    if (P->BuildingType == 3)
        STUB_LOG("SBuildingUnit::Init (0x548f20) capture flag");
    if (g_GameLogic)
        RefreshTargeting();                                   // +0x34
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
    // HD: drops a kind-3 current target that is not +0x20 (+0xc4), then,
    // without a primary target, picks one in range (0x5b4720 with +0x180 /
    // +0x17c and RangeBonus) and orders it (STarget kind 3, +0xa0) or stops
    // the gunners (+0xec).
    STUB_LOG("SBuildingUnit::AI_Heartbeat (0x546500) occupied building target search");
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
        WindowSet = -1;                                           // +0x3f8
        if (Members.Size != 0 || Members2.Size != 0) {
            // HD: the occupants go to the window points (+0x370 / +0x3ac,
            // the +0x3e8 set nearest the current target), with world random
            // draws for gunner +0x60 and their targets refreshed (0x5bd210).
            STUB_LOG("SBuildingUnit::RefreshMisc (0x54b910) occupants at the windows");
        }
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
        STUB_LOG("SBuildingUnit::UpdateVisuals (0x546d20) health bar");
        return;
    }
    // HD: board +0x18(+0x350, 0) and +0x18(+0x354, 0) hide the bar. The
    // recompile creates no board elements (Board350 / Board354 stay 0).
}

} // namespace pz
