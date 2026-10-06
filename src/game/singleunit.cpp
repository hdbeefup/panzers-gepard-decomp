// src/game/singleunit.cpp
// SSingleUnit (0x5aa7b0..0x5b1d00). OWNER: agent U.

#include <string.h>
#include "singleunit.h"
#include "gunner.h"
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
        if (Anim && Anim->GetDriverNode() < 0)                // +0x20
            Logger.g->Warning("SSingleUnit::Init: no driver node (%s) (HD panics)", SStr(Proto->Name));
        *(SString*)&du->_25c = "_driver";
        du->SetGlobalState(du->Anim ? static_cast<SUnitAnimation*>(du->Anim)->Proto->FindState("vehicle") : 0, 0);
    }
    for (int i = 0; i < P->ChildUnits.Size; ++i) {
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

} // namespace pz
