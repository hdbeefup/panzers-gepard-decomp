// src/game/singleunit.cpp
// SSingleUnit (0x5aa7b0..0x5b1d00). OWNER: agent U.

#include <math.h>
#include <string.h>
#include "singleunit.h"
#include "drivermath.h"
#include "gunner.h"
#include "idriver.h"
#include "pz/imodel.h"
#include "unitanim.h"
#include "unitextern.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"

namespace pz {

// PANZERS 0x5aa7b0
SSingleUnit::SSingleUnit(SPSingleUnit* proto, int worldIndex)
    : SUnit(proto, worldIndex)
{
    P = proto;
    for (int i = 0; i < 0x19; ++i)
        Board[i] = -1;                                        // param_1[0xd1..0xe9]
    _110 = !proto->BuiltInDriver;                             // +0x110 = proto +0xc8 == 0
}

// PANZERS 0x5aaa00
SSingleUnit::~SSingleUnit()
{
}

// The part of SSingleUnit::Init 0x5ad150 / 0x5ad9f0 after the base Init:
// the built-in driver unit (+0x208), the child units (+0x1fc), then the
// armoured-vehicle rule (no active driver, gunners off until a crew gets
// in). The board elements HD creates in between (selection, health and rank
// icons) are not created by the recompile; the model attach of the driver
// and children (model +0xdc at the animation's driver node / the child mesh
// node) needs a model slot not in imodel.h.
void SSingleUnit::InitCrewAndChildren(int player, float dir, bool fromDef, float cargo)
{
    if (P->BuiltInDriver && P->BuiltInDriverUnitName.size != 0) {
        float zero[3] = { 0.0f, 0.0f, 0.0f };
        int d = g_World->CreateUnit(player, SStr(P->BuiltInDriverUnitName), zero, dir, 0, 1.0f, -1, true, "");
        BuiltInDriverUnit = d;
        SUnit* du = WorldUnit(d);
        du->SetParent(WorldIndex);                            // 0x5c1600
        if (Anim->GetDriverNode() < 0)                        // +0x20
            Logger.g->Panic("SSingleUnit::Init: no driver node (%s)", SStr(Proto->Name));
        du->Model->AttachTo(Model, Anim->GetDriverNode());    // driver model +0xdc at the driver node
        *(SString*)&du->_25c = "_driver";
        du->SetGlobalState(du->Anim ? static_cast<SUnitAnimation*>(du->Anim)->Proto->FindState("vehicle") : 0, 0);
    }
    for (int i = 0; i < P->ChildUnits.Size; ++i) {
        int node = Model->FindNode(SStr(P->ChildUnits.Array[i].MeshName));   // model +0x40
        if (node < 0) {
            Logger.g->Warning("SSingleUnit::Init(): Invalid mesh ('%s') specified for unit '%s'.",
                              SStr(P->ChildUnits.Array[i].MeshName), SStr(Proto->Name));
            continue;
        }
        float zero[3] = { 0.0f, 0.0f, 0.0f };
        int c = g_World->CreateUnit(player, SStr(P->ChildUnits.Array[i].UnitName), zero, dir, 0, 1.0f, -1, true, "");
        if (ChildUnits.Size == ChildUnits.Max) {
            int nmax = ChildUnits.Max < 0x10 ? 0x10 : (ChildUnits.Max * 6) / 5;
            ChildUnits.Array = (int*)realloc(ChildUnits.Array, nmax * sizeof(int));
            memset(&ChildUnits.Array[ChildUnits.Max], 0, (nmax - ChildUnits.Max) * sizeof(int));
            ChildUnits.Max = nmax;
        }
        ChildUnits.Array[ChildUnits.Size++] = c;
        WorldUnit(c)->SetParent(WorldIndex);
        WorldUnit(c)->Model->AttachTo(Model, node);           // child model +0xdc at its mesh node
    }
    if (P->ArmourType != 0) {
        if (!P->BuiltInDriver) {
            SetActiveDriver(-1);                              // 0x5c0cb0(-1)
            _110 = true;
        }
        for (int i = 0; i < Gunners.Size; ++i)
            Gunners.Array[i]->Active = 0;
    }
    if (fromDef) {
        Cargo = cargo;                                        // +0x2ec = UNTD Cargo
        if (P->ArmourType == 0)
            Logger.g->Warning("SSingleUnit::Init: %s has no armour type", SStr(Proto->Name));
    }
    if (g_GameLogic && P->ClassType != 10)
        RefreshTargeting();                                   // +0x34
}

// PANZERS 0x5ad150
void SSingleUnit::Init(SUnitDef* def)
{
    SUnit::Init(def);                                         // 0x5ba8e0
    InitCrewAndChildren(def->Player, def->Dir, true, def->Cargo);
}

// PANZERS 0x5ad9f0
void SSingleUnit::InitNew(int player, const float* pos, float dir, int p4, float hp)
{
    SUnit::InitNew(player, pos, dir, p4, hp);                 // 0x5bace0
    InitCrewAndChildren(player, dir, false, 0.0f);
}

// PANZERS 0x5b0950
void SSingleUnit::Place(float x, float z, float dir)
{
    if (!Unplaced)
        return;
    SUnit::Place(x, z, dir);                                  // 0x5c5160
    SetOnBlockMap(true);                                      // +0x198
}

// PANZERS 0x5ae950
void SSingleUnit::SetOnBlockMap(bool on)
{
    if (P->ClassType != 4)
        SUnit::SetOnBlockMap(on);                             // 0x5bc660
}

// PANZERS 0x5b02e0
// A unit carried at a node of this unit's model (seated crew, child units,
// the built-in driver) takes the node's logic-pose position and faces along
// the node's second axis; its speed is the distance moved since the last
// tick. Wrecked units run their +0x30 refresh instead.
void SSingleUnit::PlaceAttachedUnit(SUnit* u, int node)
{
    if (u->Wrecked) {                                         // +0x150
        u->Slot_30();
        return;
    }
    if (node < 0) {
        u->Pos[0] = Pos[0];
        u->Pos[1] = Pos[1];
        u->Pos[2] = Pos[2];
        u->Dir = Dir;
    } else {
        float axis[3] = { 0.0f, 0.0f, 0.0f };
        Model->GetNodePositionAxis(node, u->Pos, axis);       // model +0x50
        u->Dir = HdAtan2f((double)axis[0], (double)axis[2]);  // 0x78d07a (x, z), fstp dword
    }
    float dx = u->Pos[0] - u->PrevPos[0];
    float dz = u->Pos[2] - u->PrevPos[2];
    u->Speed = (float)sqrt((double)(dx * dx + dz * dz));      // 0x78d090
    u->_f8 = _f8;
    if (Speed == 0.0f && *(float*)&_d0 != 0.0f)               // +0xc8 / +0xd0
        u->_f8 = 2;
    u->_154 = (unsigned short)((u->_154 & 0xff00) | (_154 & 0xff));   // byte +0x154
}

// PANZERS 0x5af890
void SSingleUnit::RefreshMisc()
{
    float dir0 = Dir;
    if (ActiveDriver >= 0) {
        if (ActiveDriver >= Drivers.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SDriver *", ActiveDriver);
        Drivers.Array[ActiveDriver]->Refresh();               // driver +0x14
    }
    if (Unplaced)
        return;
    if (Speed > 0.0f && (P->ArmourType == 2 || P->ArmourType == 1) && P->ClassType != 0xb)
        UpdateSeenByPlayers();                                // 0x5bc5c0
    for (int i = 0; i < Gunners.Size; ++i)
        Gunners.Array[i]->ServerRefresh();                    // gunner +0x14
    if (P->ClassType == 0xb && Members.Size > 2) {
        // HD: the gun crew (seats 2..) follows the gun: a moved gun stamps
        // +0x264 with the frame; 10 ticks later the crew either stops
        // (+0xec) or takes new ground positions from the gun's crew nodes
        // (model +0x54, unit +0xac EC_Move), then runs their +0x2c refresh.
        (void)dir0;
        STUB_LOG("SSingleUnit::RefreshMisc (0x5af890) gun crew (class 0xb)");
    }
    if (_118 > 0.0f) {
        float ground = g_World->GetTerrainHeight(Pos[0], Pos[2]);           // 0x5e7730
        float water = g_World->GetWaterHeight(Pos[0], Pos[2]) - 0.01f;      // 0x5ec490 - DAT_007f1b48
        if (water <= ground) {
            _118 -= g_UnitRegistry->ThermoDecrease;
        } else {
            _118 -= g_UnitRegistry->ThermoDecrease * 4.0f;    // DAT_007f4588
            // HD: pixie +0x24(scene, logic +0x1c tank dust, pos, (0, 1, 0), 0): steam.
            STUB_LOG("SSingleUnit::RefreshMisc (0x5af890) water steam effect");
        }
    }
    for (int i = 0; i < Members.Size; ++i) {
        if (!Members.Array[i].Attached)
            continue;
        PlaceAttachedUnit(WorldUnit(Members.Array[i].Unit), Members.Array[i].Node);
    }
    for (int i = 0; i < ChildUnits.Size; ++i) {
        SUnit* c = WorldUnit(ChildUnits.Array[i]);
        if (i >= P->ChildUnits.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SPChildUnit", i);
        int node = Model->FindNode(SStr(P->ChildUnits.Array[i].MeshName));   // model +0x40
        PlaceAttachedUnit(c, node);
    }
    if (BuiltInDriverUnit >= 0)
        PlaceAttachedUnit(WorldUnit(BuiltInDriverUnit), Anim->GetDriverNode());   // anim +0x20
    RefreshRepairTarget(10.0f);                               // 0x5c01e0
    RefreshSupplyTarget(10.0f);                               // 0x5bf280
}

} // namespace pz
