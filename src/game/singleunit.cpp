// src/game/singleunit.cpp
// SSingleUnit (0x5aa7b0..0x5b1d00). OWNER: agent U.

#include <math.h>
#include <string.h>
#include "singleunit.h"
#include "drivermath.h"
#include "gunner.h"
#include "idriver.h"
#include "pz/imodel.h"
#include "pz/ipixie.h"
#include "pz/iscene.h"
#include "unitanim.h"
#include "unitextern.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"
#include "m3common.h"
#include "gamelogic.h"
#include "target.h"
#include "packets.h"

namespace pz {

static SUnitMember& MemberAt(SUnit* u, int i)                     // SDArray<SUnitMember>::operator[] 0x5991e0
{
    if (i < 0 || i >= u->Members.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SUnitMember", i);
    return u->Members.Array[i];
}

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
// in). The board elements (selection, health and rank icons) are created in
// between (CreateBoardElements, unitboard.cpp).
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
    CreateBoardElements();                                    // 0x5ad5e0..0x5ad91e
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
        u->RefreshDead();
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
        // The gun crew (rows 2..) follows the gun: a moved / turned gun
        // stamps +0x264; for 10 ticks after that the crew walks to the gun's
        // crew nodes (model +0x54, +0xc0 Stop, +0xac EC_Move facing the gun's
        // direction) and drops its gunner target; later it fights: behaviour
        // 2 (hold fire) stops their gunners, otherwise the gun's target
        // search (FindTarget 0x5b4720 with the first crew member's gunner and
        // range) hands each crew gunner and driver a unit target (kind 2).
        // Then every crew member runs its ServerRefresh (+0x2c).
        if (Pos[0] != PrevPos[0] || Pos[1] != PrevPos[1] || Pos[2] != PrevPos[2] || Dir != dir0)
            _264 = g_GameLogic->GetFrame();                       // 0x56d1a0
        if (g_GameLogic->GetFrame() <= _264 + 10) {
            for (int i = 2; i < Members.Size; ++i) {
                if (Members.Array[i].Attached)
                    Logger.g->Panic("SSingleUnit::RefreshMisc: attached gun crew member");   // 0x7fb5c8
                float np[3] = { 0.0f, 0.0f, 0.0f };
                Model->GetNodePosition(Members.Array[i].Node, np);   // model +0x54
                SUnit* m = WorldUnit(Members.Array[i].Unit);
                m->Stop();                                        // +0xc0
                int xb, zb, db;
                memcpy(&xb, &np[0], 4);
                memcpy(&zb, &np[2], 4);
                memcpy(&db, &Dir, 4);
                m->EC_Move(xb, zb, 0, true, db);                  // +0xac
                if (m->Gunners.Size <= 0)
                    Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
                SGunner* g = m->Gunners.Array[0];
                if (g->Target) {
                    g->Target->Release();                         // 0x5bdef0
                    g->Target = nullptr;
                }
            }
        } else if (Behavior == 2) {
            for (int i = 2; i < Members.Size; ++i)
                WorldUnit(Members.Array[i].Unit)->StopGunners();  // +0xec
        } else {
            SUnit* m0 = WorldUnit(Members.Array[0].Unit);
            float range = m0->GetMaxRange(0);                     // +0x17c(0), fstp dword
            if (m0->Gunners.Size <= 0)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
            SGunner* g0 = WorldUnit(Members.Array[0].Unit)->Gunners.Array[0];
            int wt = WorldUnit(Members.Array[0].Unit)->Gunners.Array[0]->GetWeaponType();   // 0x584240
            int t = FindTarget(wt, g0, 0.0f, range, true);        // 0x5b4720
            if (t >= 0) {
                for (int i = 2; i < Members.Size; ++i) {
                    SUnit* m = WorldUnit(Members.Array[i].Unit);
                    if (m->Gunners.Size <= 0)
                        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
                    STarget* cur = m->Gunners.Array[0]->Target;
                    if (cur && WorldUnit(MemberAt(this, i).Unit)->Gunners.Array[0]->Target->Unit == t)
                        continue;
                    STarget* nt = PzTargetNew(2);                 // new 0x38, 0x5b27c0(2)
                    tgt::I(nt, tgt::kType) = 0;                   // 0x5c21b0
                    tgt::I(nt, tgt::kUnit) = t;
                    m->Gunners.Array[0]->SetTarget(nt);           // gunner +0x18
                    if (m->Drivers.Size <= 0)
                        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SDriver *", 0);
                    m->Drivers.Array[0]->SetTarget(nt);           // driver +0x08
                }
            }
        }
        for (int i = 2; i < Members.Size; ++i)
            WorldUnit(Members.Array[i].Unit)->ServerRefresh(LastRefreshFrame);   // +0x2c(+0x84)
    }
    if (_118 > 0.0f) {
        float ground = g_World->GetTerrainHeight(Pos[0], Pos[2]);           // 0x5e7730
        float water = g_World->GetWaterHeight(Pos[0], Pos[2]) - 0.01f;      // 0x5ec490 - DAT_007f1b48
        if (water <= ground) {
            _118 -= g_UnitRegistry->ThermoDecrease;
        } else {
            _118 -= g_UnitRegistry->ThermoDecrease * 4.0f;    // DAT_007f4588
            float up[3] = { 0.0f, 1.0f, 0.0f };
            g_Pixie->PlayEffect(g_Scene, g_GameLogic->TankDustFx, Pos, up, 0);   // pixie +0x24: steam
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

// ---------------------------------------------------------------------------
// M3-C2: SSingleUnit combat / death overrides

// SGunner 0x583b60 (agent C1; declared in squadcombat.cpp until C1 lifts it).
bool GunnerCanAttackUnit(SGunner* g, SUnit* target, bool p3);

static int SuPlayerKind(int player)                               // World+0x178 + player * 0x48
{
    return *(int*)(g_World->Players[player] + 0x08);
}

static bool SuSameSide(int a, int b)                              // 0x549ab0
{
    int team = *(int*)(g_World->Players[a] + 0x0c);
    if (team != 0)
        return team == *(int*)(g_World->Players[b] + 0x0c);
    return a == b;
}

// An x87 "fadd qword" under the in-game control word 0x007F (24-bit
// precision): the sum rounded once to 24 bits.
static float X87AddPC24f(double a, double b)
{
    unsigned short cw;
    unsigned short ncw = 0x007f;
    float r;
    __asm {
        fnstcw cw
        fldcw ncw
        fld a
        fadd b
        fstp r
        fldcw cw
    }
    return r;
}

// PANZERS 0x5aca80
// The lowest max range: gunner MainGunner's, or against a squad target the
// shortest of all gunners.
float SSingleUnit::GetLowestMaxRange()
{
    float r = GetMaxRange(MainGunner);                            // +0x17c(+0x44)
    STarget* t = CurrentTarget;
    if (t && tgt::I(t, tgt::kType) == 0) {
        int u = tgt::I(t, tgt::kUnit);
        if (g_World->Units.IsLive(u)) {
            SUnit* tu = WorldUnit(u);
            if (Gunners.Size > 1 && tu->Proto->ClassType == 5) {
                for (int i = 0; i < Gunners.Size; ++i)
                    if (GetMaxRange(i) < r)
                        r = GetMaxRange(i);
            }
        }
    }
    return r;
}

// PANZERS 0x5b0420
// Shot at by `attacker`: an idle armed unit (no targets, behaviour 0) that
// can hit the enemy attacks it (out of range: +0xe8 EC_Attack; in range:
// +0xcc EC_AttackMove to it); idle AI units nearby that can hit it join in
// (+0xcc); an AI unit of an AI group tells the group (0x5d68e0).
void SSingleUnit::OnAttackedBy(int attacker)
{
    PZ_M3_TRACE("SSingleUnit::OnAttackedBy (0x5b0420)");
    if (MainGunner > -1 && !PrimaryTarget && !CurrentTarget) {
        if (attacker <= -1)
            return;
        SUnit* a = WorldUnit(attacker);
        if (a->Proto->ClassType != 8 && Behavior == 0 && WorldUnit(attacker)->Player != Player) {
            a = WorldUnit(attacker);
            if (Gunners.Size > 0 && GunnerCanAttackUnit(GetGunner(0), a, true)) {   // 0x55cc40, 0x583b60
                float dz = a->Pos[2] - Pos[2];
                float dx = a->Pos[0] - Pos[0];
                float r1 = X87AddPC24f((double)GetMaxRange(0), 0.1);   // 0x7f83e0
                float d2 = dx * dx + dz * dz;
                float r2 = X87AddPC24f((double)GetMaxRange(0), 0.1);
                float r = r2 * r1;                                // x87 fmul, PC24
                if ((double)d2 > (double)r)
                    EC_Attack(attacker, 0);                       // +0xe8
                else
                    EC_AttackMove(a->Pos[0], a->Pos[2], 0);       // +0xcc
            }
        }
    }
    if (attacker > -1 && AIGroup == -1) {
        SUnit* a = WorldUnit(attacker);
        if (a->Proto->ClassType != 8) {
            SUnit* au = WorldUnit(attacker);
            for (int i = 0; i < SightUnits.Size; ++i) {
                if (!IsTargetable(SightUnits.Array[i].Unit, false))   // 0x5bb6b0
                    continue;
                SUnit* o = WorldUnit(SightUnits.Array[i].Unit);
                if (!SuSameSide(o->Player, Player))
                    continue;
                if (SuPlayerKind(o->Player) != 1 || o->PrimaryTarget || o->CurrentTarget)
                    continue;
                if (o->Behavior != 0 && !o->IsAIDefault())        // 0x5bb470
                    continue;
                if (o->AIGroup != -1 || o->Gunners.Size <= 0)
                    continue;
                if (!GunnerCanAttackUnit(o->GetGunner(0), au, true) || o->IsHiddenInBlockMap())   // 0x5bb5c0
                    continue;
                // HD adds dx unsquared (kept).
                float d = (o->Pos[0] - Pos[0]) + (o->Pos[2] - Pos[2]) * (o->Pos[2] - Pos[2]);
                if (d > 100.0f)                                   // 0x7ee558
                    continue;
                o->EC_AttackMove(au->Pos[0], au->Pos[2], 0);      // +0xcc
            }
        }
    }
    if (AIGroup > -1 && attacker > -1) {
        SUnit* a = WorldUnit(attacker);
        if (a->Proto->ClassType != 8 && SuPlayerKind(Player) == 1 &&
            !SuSameSide(Player, WorldUnit(attacker)->Player))
            g_World->AIGroupUnitAttacked(WorldIndex, attacker);   // 0x5d68e0
    }
}

// PANZERS 0x5aed30
// The driver reached the current target: kind 9 get in (target +0x5c
// StoreUnit; an AI carrier takes over our primary target), kind 0xb unload
// (+0x64), kind 0xc tow (target +0x58, then +0x6c); then the SUnit part.
void SSingleUnit::OnDriverReachedTarget()
{
    STarget* ct = CurrentTarget;
    if (!ct)
        Logger.g->Panic("SSingleUnit::OnDriverReachedTarget - No Currenttarget, unit:%s, idx:%d",
                        SStr(P->Name), WorldIndex);
    int kind = tgt::I(ct, tgt::kKind);
    if (kind == 9) {
        int c = tgt::I(ct, tgt::kUnit);
        if (WorldUnit(c)->StoreUnit(WorldIndex, 0)) {             // +0x5c
            if (PrimaryTarget && CurrentTarget != PrimaryTarget && SuPlayerKind(Player) == 1) {
                SUnit* car = WorldUnit(tgt::I(CurrentTarget, tgt::kUnit));
                SetTarget(&car->PrimaryTarget, PrimaryTarget);    // 0x5bdef0 / 0x5b5a30
                WorldUnit(tgt::I(CurrentTarget, tgt::kUnit))->SetCurrentTarget(PrimaryTarget, 0);   // +0xa0
            }
            if (PrimaryTarget) {
                SetTarget(&PrimaryTarget, nullptr);
                SUnit::OnDriverReachedTarget();                   // 0x5bcb60
                return;
            }
        }
    } else if (kind == 0xb) {
        UnloadUnit(tgt::I(ct, tgt::kP28));                        // +0x64
        if (ActiveDriver < 0) {
            if (PrimaryTarget)
                SetTarget(&PrimaryTarget, nullptr);
            return;
        }
    } else if (kind == 0xc) {
        // HD: if (target unit +0x58(this)) this +0x6c(target unit): tow it.
        STUB_LOG("SSingleUnit::OnDriverReachedTarget (0x5aed30) tow: unit +0x58 (C5) / +0x6c (0x5c2390)");
    }
    SUnit::OnDriverReachedTarget();                               // 0x5bcb60
}

// PANZERS 0x5af2c0
// The per-tick refresh of a wreck (+0x30). +0x15c counts down after the
// death: at 99 the wreck model, at 4 the burning effects, at 0 the unit
// leaves the world. With the counter at 0 (the first wreck tick): the
// built-in driver and the child units go, the crew takes 10..14 damage
// (world LCG) and an explosion (0x576490, 10 at radius 5) hits around,
// the targets, passengers and the driver seat are released and squads that
// lost their vehicle get XP.
void SSingleUnit::RefreshDead()
{
    _104 = 0;
    _108 = 0;
    ScriptID = "";                                                // +0x194 freed (0x76654a)
    if (_15c != 0) {
        if (--_15c == 0) {
            Frozen = true;                                        // +0x153
            if (_2e4 > -1 && g_World->Units.IsLive(_2e4))
                WorldUnit(_2e4)->_304 = false;
            g_World->RemoveUnit(WorldIndex);                      // 0x5f8060
            return;
        }
        if (_15c == 99)
            SetWreckModel();                                      // +0x28
        if (_15c == 4)
            PlayDiedByFireEffects();                              // 0x5c2590
        return;
    }
    *((unsigned char*)this + 0x155) = 1;                          // +0x155
    if (P->BuiltInDriver && P->BuiltInDriverUnitName.size != 0) {
        SUnit* d = WorldUnit(BuiltInDriverUnit);
        d->Model->Slot_E0();                                      // model +0xe0
        d->Unplace();                                             // +0x4c
        g_World->RemoveUnit(BuiltInDriverUnit);
        BuiltInDriverUnit = -1;
    }
    for (int i = 0; i < ChildUnits.Size; ++i) {
        SUnit* c = WorldUnit(ChildUnits.Array[i]);
        c->Model->Slot_E0();
        *((unsigned char*)c + 0x155) = 1;
    }
    if (_2e4 == -1) {
        for (int k = 0; k < Members.Size; ++k) {
            SUnit* m = WorldUnit(Members.Array[k].Unit);
            int r = WorldRand();                                  // inline LCG
            int dmg = 10 - (int)((double)r * -3.0517578125e-05 * 5.0);   // 0x7f4598, 0x7f5a40
            m->TakeDamage((float)dmg, 0, -1, 0.0f, 0.0f, 0.0f, 1);   // +0x94
        }
        g_GameLogic->AreaDamage(10.0f, -1, -1, Pos[0], Pos[1], Pos[2], 5.0f, 0, 2);   // 0x576490
    }
    if (CurrentTarget)
        ClearTargets();                                           // +0xc4
    UnloadAll();                                                  // +0x68
    Remove(false);                                                // +0x70
    if (_7c)
        WorldUnit(Parent)->Remove(false);
    LeaveDriverSeat();                                            // 0x5c1d50
    if (_2e4 != -1)
        return;
    for (int i = 0; i < Stored.Size; ++i) {
        SUnit* s = WorldUnit(Stored.Array[i].Unit);
        if (s->Proto->ClassType != 5 || s->Proto->UnitType != 0xe)
            continue;
        // The crew's XP for the lost vehicle: 100 the first time (+0x6b), then 50.
        if (!s->FirstVehicleLost) {
            s->AddXP(-1, 100.0f, 0);                              // +0x8c (0x42c80000)
            s->FirstVehicleLost = true;
        } else {
            s->AddXP(-1, 50.0f, 0);                               // 0x42480000
        }
    }
}

// PANZERS 0x5ae970
// A crew member died (+0x1b4, from the member's EC_Die): it leaves its seat
// (the attached model comes off, it is placed at the seat node with the
// vehicle's direction, sub-state "", the "kneel" state) and its seat row
// goes (0x5be140). When the last one is gone the vehicle drops its order
// (without a driver), unloads (+0x68), the crew's squad dies (+0x124) and
// whoever targeted the vehicle lets go (0x5ef760).
void SSingleUnit::OnMemberDied(int unit)
{
    PZ_M3_TRACE("SSingleUnit::OnMemberDied (0x5ae970)");
    for (int i = 0; i < Members.Size; ++i) {
        if (Members.Array[i].Unit != unit)
            continue;
        SUnit* m = WorldUnit(Members.Array[i].Unit);
        m->Parent = -1;                                           // +0x78
        if (Members.Array[i].Attached)                            // row +0x14
            m->Model->Slot_E0();                                  // model +0xe0 (detach)
        float pos[3] = { 0.0f, 0.0f, 0.0f };
        Model->GetNodePosition(Members.Array[i].Node, pos);       // model +0x54, row +0x10
        m->Place(pos[0], pos[2], Dir);                            // +0x50
        *(SString*)&m->_25c = "";                                 // +0x25c sub-state
        m->_f0 = false;
        int st = static_cast<SUnitAnimation*>(m->Anim)->Proto->FindState("kneel");   // 0x5c7ed0
        m->SetBehavior(st);                                       // +0x12c
        int owner = Members.Array[i].Owner;                       // row +0x04
        RemoveStoredMember(i);                                    // 0x5be140
        if (Members.Size != 0)
            return;
        if (ActiveDriver < 0 && PrimaryTarget) {
            PzTargetRelease(PrimaryTarget);                       // 0x5bdef0
            PrimaryTarget = nullptr;
        }
        UnloadAll();                                              // +0x68
        WorldUnit(owner)->EC_Die();                               // +0x124
        g_World->UnitStored(WorldIndex, -1);                      // 0x5ef760
        return;
    }
}

// PANZERS 0x5ad070
// XP goes to every stored unit (the crew squad); a vehicle of an observer
// (kind 4) player first moves to the local player.
void SSingleUnit::AddXP(int victim, float xp, int p3)
{
    if (*(const int*)(g_World->Players[Player] + 8) == 4)        // World+0x178 + player * 0x48
        Player = g_World->LocalPlayer;                            // World+0x16c
    for (int i = 0; i < Stored.Size; ++i)
        WorldUnit(Stored.Array[i].Unit)->AddXP(victim, xp, p3);   // +0x8c
}

// PANZERS 0x5ace90 (SSingleUnit / STrainUnit +0xa8)
// Unhook (6) the towed unit; a building 10; an enemy 3; tow (5) when this
// can tow (+0x54) and the target can be towed by it (+0x58); enter (4);
// repair (7) / supply (8) with cargo left; else 2.
int SSingleUnit::ActionOn(int target)
{
    if (!IsTargetable(target, true) || target == WorldIndex)
        return 0;
    const unsigned char* p = *(const unsigned char* const*)((const unsigned char*)this + 0x340);
    if (target == Towed && *(const int*)(p + 0x40) != 10)         // +0x2d8, P +0x40
        return 6;
    if (!g_World->Units.IsLive(target))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", target);
    SUnit* t = g_World->Units.Array[target].Unit;
    if (t->Proto->ClassType == 9)
        return 10;
    if (GetUnitRelation(target, Player) == -1)                    // 0x56d2a0
        return 3;
    if (Slot_54() && t->Slot_58((int)(size_t)this))               // +0x54, target +0x58(this)
        return 5;
    if (t->CanStoreUnit(WorldIndex))                              // 0x5b7040
        return 4;
    if (p[0xdf] && t->NeedsRepair() && 0.0f < Cargo)              // 0x5bc840, +0x2ec
        return 7;
    if (p[0xe0] && t->NeedsSupply(1.0f) && 0.0f < Cargo)          // 0x5bc700
        return 8;
    return 2;
}

// PANZERS 0x5aaa80
// Can this unit tow: it has a "hook" node (animation +0x24) and tows
// nothing yet (+0x2d8 < 0). Used by the action cursor (0x5ace90).
bool SSingleUnit::Slot_54()
{
    return Anim->GetHookNode() > -1 && Towed < 0;                 // +0x24, +0x2d8
}

// PANZERS 0x5aaa30
// Can this unit be towed by `p1` (the tower's SUnit*, as HD passes it):
// not when +0x7c is set or it is owned (not +0x110) by another player, and
// only with a tow hole (animation +0x28 "hole_f" or +0x2c "hole_r").
bool SSingleUnit::Slot_58(int p1)
{
    SUnit* tower = reinterpret_cast<SUnit*>(static_cast<intptr_t>(p1));
    if (_7c || (!_110 && Player != tower->Player))
        return false;
    if (Anim->GetHoleFNode() < 0 && Anim->GetHoleRNode() < 0)  // +0x28, +0x2c
        return false;
    return true;
}

} // namespace pz
