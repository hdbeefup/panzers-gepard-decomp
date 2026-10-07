// src/game/waster.cpp
// SWasterUnit (0x5d1cd0..0x5d2ee0, 0x5870f0, SPWasterUnit::CreateUnit
// 0x5a5df0 in punit.cpp) and SWasterAnimation (0x5c7570, 0x5c7b20,
// 0x5ca600, 0x5cb3c0, 0x5cf790). OWNER: agent M3-C sub-agent C3.
//
// No world RNG draw here. The board elements (a timer bar per waster,
// DAT_008f1c60 +0x08 / +0x10 / +0x14 / +0x18 / +0x3c) are not created, as
// for the other units (SSingleUnit +0x344): the HD board interface is not
// mapped yet (src/panzers/pzboard.h).

#include <math.h>
#include <string.h>
#include "waster.h"
#include "squadunit.h"
#include "drivermath.h"
#include "gamelogic.h"
#include "gunner.h"
#include "target.h"
#include "unitanim.h"
#include "unitanim_model.h"
#include "world.h"
#include "worldapi.h"
#include "pz/imodel.h"
#include "pz/ipixie.h"
#include "logger.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {

#if defined(_M_IX86)
static_assert(offsetof(SWasterUnit, P) == 0x340, "+0x340");
static_assert(offsetof(SWasterUnit, LifeTime) == 0x34c, "+0x34c");
static_assert(offsetof(SWasterUnit, OwnerRank) == 0x360, "+0x360");
#endif

// HD 0x549ab0 (inline): the same side (team 0 = alone).
static bool SameSide(int a, int b)
{
    int team = *(int*)(g_World->Players[a] + 0x0c);               // World+0x17c
    if (team != 0)
        return team == *(int*)(g_World->Players[b] + 0x0c);
    return a == b;
}

// The class record of +0x1c (HD 0x8dddf0).
static const SUnitClassDesc kUnitClassDesc_8dddf0 = { "SUnit", &kUnitClassDesc_8dc540, 0x8dddf0 };

static bool IsMolotov(SPUnit* p)                                  // 0x52c410(proto +0x60, "Waster Molotov Fire")
{
    return p->Name.size != 0 && _stricmp(p->Name.buf, "Waster Molotov Fire") == 0;
}

static SGunner* Gunner0(SUnit* u)
{
    if (u->Gunners.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
    return u->Gunners.Array[0];
}


// PANZERS 0x5a5df0 is SPWasterUnit::CreateUnit (punit.cpp).

// PANZERS 0x5d1cd0
SWasterUnit::SWasterUnit(SPWasterUnit* proto, int worldIndex) : SUnit(proto, worldIndex)
{
    PZ_M3_TRACE("SWasterUnit::SWasterUnit (0x5d1cd0)");
    P = proto;
    AttachedUnit = -1;
    LifeTime = proto->LifeTime;                                   // +0x13c
    Activated = 0;
    FireEffect = -1;
    OwnerSquad = -1;
    OwnerRank = 0;
    BoardBar = -1;
    BoardFrame = -1;
}

// PANZERS 0x5d1db0
SWasterUnit::~SWasterUnit()
{
    if (FireEffect != -1 && g_Pixie)
        g_Pixie->StopEffect(FireEffect);                          // pixie +0x34
}

// PANZERS 0x5d2070
void SWasterUnit::Uninit()
{
    ReleaseBoardElements();                                       // board +0x0c(+0x358)
    SUnit::Uninit();                                              // 0x5b7e40
}

// PANZERS 0x5d22b0
void SWasterUnit::Init(SUnitDef* def)
{
    SUnit::Init(def);                                             // 0x5ba8e0
    CreateBoardElements();                                        // board +0x08(4, ...) twice (unitboard.cpp)
    if (g_GameLogic)
        RefreshTargeting();                                       // +0x34
}

// PANZERS 0x5d2330
void SWasterUnit::InitNew(int player, const float* pos, float dir, int p4, float hp)
{
    SUnit::InitNew(player, pos, dir, p4, hp);                     // 0x5bace0
    if (g_GameLogic)
        RefreshTargeting();
}

// PANZERS 0x5d23d0
void SWasterUnit::Slot_14()
{
    SUnit::Slot_14();                                             // 0x5bb1c0
    CreateBoardElements();                                        // the board elements again (HD does not release the old pair)
}

// PANZERS 0x5d2290
void SWasterUnit::GetClassDescriptor(void** obj, const SUnitClassDesc** desc)
{
    *obj = this;
    *desc = &kUnitClassDesc_8dddf0;
}

// PANZERS 0x5d2460
void SWasterUnit::RefreshDead()
{
    PZ_M3_TRACE("SWasterUnit::RefreshDead (0x5d2460)");
    if (_15c == 0) {
        *((unsigned char*)this + 0x155) = 1;                      // start the death animation
        Unplace();                                                // +0x4c (tail call)
        return;
    }
    if (--_15c == 0) {
        // 0x5bcb50 (empty)
        g_World->RemoveUnit(WorldIndex);                          // 0x5f8060
    }
}

// PANZERS 0x5d2440
void SWasterUnit::RefreshTargeting()
{
    FillNearUnits(10.0f, 0.0f);                                   // 0x5b78a0
}

// PANZERS 0x5d2280
void SWasterUnit::EC_Die()
{
    Wrecked = true;                                               // +0x150
}

// SWasterUnit::UpdateVisuals 0x5d1e30: unitboard.cpp (M5-VX).

// PANZERS 0x5d21a0
void SWasterUnit::EC_AttackPos(int xBits, int zBits, int p3)
{
    (void)p3;
    if (MainGunner > -1) {
        STarget* t = STarget::Create(2);                          // new 0x38, 0x5b27c0(2)
        float xz[2];
        memcpy(&xz[0], &xBits, 4);
        memcpy(&xz[1], &zBits, 4);
        t->SetGroundPos(xz);                                      // 0x5c1860
        t->Mode = 1;                                              // +0x20
        if (MainGunner >= Gunners.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", MainGunner);
        Gunners.Array[MainGunner]->SetTarget(t);                  // +0x18
    }
    Activated = 1;
}

// PANZERS 0x5d2090
void SWasterUnit::EC_Attack(int unit, int p2)
{
    (void)p2;
    if (!g_World->Units.IsLive(unit) || unit == WorldIndex)
        return;
    if (WorldUnit(unit)->Proto->ClassType == 3)                   // not on projectiles
        return;
    if (MainGunner > -1) {
        STarget* t = STarget::Create(2);
        t->Type = kTargetUnit;                                    // 0x5c21b0
        t->Unit = unit;
        t->Mode = 1;
        GetGunner(MainGunner)->SetTarget(t);                      // 0x55cc40, +0x18
    }
    Activated = 1;
}

// PANZERS 0x5d24a0 (vtable +0x38)
void SWasterUnit::RefreshMisc()
{
    PZ_M3_TRACE("SWasterUnit::RefreshMisc (0x5d24a0)");
    if (AttachedUnit >= 0 && !g_World->Units.IsLive(AttachedUnit))
        AttachedUnit = -1;
    if (LifeTime != 0) {
        --LifeTime;
        return;
    }
    if (Activated == 0 && !P->Sensor && !P->RemoteControl)
        Explode();                                                // 0x5d2880
    if (Activated == 0 && P->RemoteControl)
        return;
    if (P->Sensor && Activated == 0) {
        // A mine: every fifth tick (phase by world index) it looks for an
        // enemy on it; anti-tank (weapon 1) for armoured units, HE (2) for
        // the soft ones.
        int f = (g_GameLogic ? g_GameLogic->GetFrame() : 0) + WorldIndex;   // 0x56d1a0
        if (f % 5 == 0)
            return;
        int hit = g_GameLogic ? g_GameLogic->ProjectileHitTest(WorldIndex, this) : -1;
        if (hit == -1)
            return;
        if (SameSide(Player, WorldUnit(hit)->Player))             // 0x549ab0
            return;
        bool fire = false;
        if (WorldUnit(hit)->Proto->ArmourType != 0 && Gunner0(this)->GetWeaponType() == 1) {   // 0x584240
            fire = true;
        } else {
            if (Player == WorldUnit(hit)->Player) {
                if (WorldUnit(hit)->Proto->ArmourType == 0)
                    return;
            }
            if (Gunner0(this)->GetWeaponType() != 2)
                return;
            fire = true;
        }
        if (fire) {
            AttachedUnit = hit;
            Explode();
        }
    }
    if (AttachedUnit != -1) {
        if (Gunner0(this)->AmmoLeft > 0.0f) {                     // +0x20
            SUnit* a = WorldUnit(AttachedUnit);
            Pos[0] = a->Pos[0];
            Pos[1] = a->Pos[1];
            Pos[2] = a->Pos[2];
            if (IsMolotov(Proto)) {
                Pos[1] = WorldUnit(AttachedUnit)->Proto->FireHeight + Pos[1];   // proto +0xc4
                if (g_Pixie)
                    g_Pixie->SetEffectPosition(FireEffect, Pos);  // pixie +0x4c
                SGunner* g = Gunner0(this);
                if (g->Target == nullptr) {
                    EC_Attack(AttachedUnit, 0);                   // +0xe8
                } else {
                    STarget* t = Gunner0(this)->Target;           // +0x14
                    t->Pos[0] = Pos[0];
                    t->Pos[1] = Pos[1];
                    t->Pos[2] = Pos[2];
                }
            }
        }
    }
    Gunner0(this)->ServerRefresh();                               // +0x14
    if (0.0f >= Gunner0(this)->AmmoLeft)
        EC_Die();                                                 // +0x124
}

// PANZERS 0x5d2880
// Goes off: on a building, the incidence effects of its material (stone /
// wood) of gunner 0; on a unit, gunner 0's shot at it (molotov: a fire
// effect hung on the unit); on the ground, the shot straight up at the
// terrain height.
void SWasterUnit::Explode()
{
    PZ_M3_TRACE("SWasterUnit::Explode (0x5d2880)");
    static const float kUp[3] = { 0.0f, 1.0f, 0.0f };
    if (AttachedUnit <= -1) {
        int xb, zb;
        memcpy(&xb, &Pos[0], 4);
        memcpy(&zb, &Pos[2], 4);
        EC_AttackPos(xb, zb, 0);                                  // +0xe4
        Pos[1] = g_World->GetTerrainHeight(Pos[0], Pos[2]);       // 0x5e7730
        float dir[3] = { 0.0f, 1.0f, 0.0f };
        Gunner0(this)->ShotEffects(Pos, dir, true);              // 0x587250
    } else if (WorldUnit(AttachedUnit)->Proto->ClassType == 9) {
        SUnit* b = WorldUnit(AttachedUnit);
        bool stone = *(int*)((char*)b->Proto + 0x144) == 1;       // SPBuildingUnit +0x144 BuildingMaterial
        for (int i = 0;; ++i) {
            SPGunner* pg = Gunner0(this)->GetPGunner();           // +0x2c
            SUnitArray<SPUnitEffect>& fx = stone ? pg->StoneIncidenceEffects : pg->WoodIncidenceEffects;
            if (i >= fx.Size) {
                AttachedUnit = -1;
                break;
            }
            float dir[3] = { 0.0f, 1.0f, 0.0f };
            if (IsMolotov(Proto)) {
                if (g_Pixie && g_Scene) {
                    int e = g_Pixie->CreateEffect(g_Scene, fx.Array[i].Proto, Pos, dir);   // pixie +0x2c
                    // HD: pixie +0x58(e, building model): attaches the fire to
                    // the building (SIPixie Slot_58, not typed).
                    STUB_LOG("SWasterUnit::Explode (0x5d2880) pixie +0x58 (fire on the building)");
                    g_Pixie->ReleaseEffect(e);                    // pixie +0x38
                }
            } else if (fx.Size > 0) {
                if (g_Pixie && g_Scene)
                    g_Pixie->PlayEffect(g_Scene, fx.Array[i].Proto, Pos, dir, 0);   // pixie +0x24
            }
        }
    } else if (!IsMolotov(Proto)) {
        if (AttachedUnit != -1 && Gunner0(this)->AmmoLeft > 0.0f) {
            SUnit* a = WorldUnit(AttachedUnit);
            Pos[0] = a->Pos[0];
            Pos[1] = a->Pos[1];
            Pos[2] = a->Pos[2];
        }
        SUnit* a = WorldUnit(AttachedUnit);
        float d[3];
        d[0] = a->Pos[0] - Pos[0];
        d[1] = a->Pos[1] - Pos[1];
        d[2] = a->Pos[2] - Pos[2];
        Gunner0(this)->ShotEffects(Pos, d, true);                // 0x587250
    } else {
        SPGunner* pg = Gunner0(this)->GetPGunner();
        if (pg->ShotEffects.Size > 0) {
            float dir[3] = { 0.0f, 1.0f, 0.0f };
            if (g_Pixie && g_Scene)
                FireEffect = g_Pixie->CreateEffect(g_Scene, pg->ShotEffects.Array[0].Proto, Pos, dir);   // pixie +0x2c
            // HD: pixie +0x58(FireEffect, unit model): the fire follows the unit.
            STUB_LOG("SWasterUnit::Explode (0x5d2880) pixie +0x58 (molotov fire on the unit)");
        }
    }
    (void)kUp;
    if (AttachedUnit != -1)
        EC_Attack(AttachedUnit, 0);                               // +0xe8
    Activated = 1;
}

// PANZERS 0x5870f0
void WasterSetOwner(SUnit* waster, int squad)
{
    SWasterUnit* w = static_cast<SWasterUnit*>(waster);
    w->OwnerSquad = squad;
    w->OwnerRank = WorldUnit(squad)->GetRank();                   // +0x88
}

// ---------------------------------------------------------------------------
// SWasterAnimation (0x2c, vftable 0x7fd9dc)

struct SWasterAnimation : SUnitAnimation {
    SWasterAnimation(SPWasterAnimation* proto, SIUnit* unit) : SUnitAnimation(proto, unit)
    {
        Unit = unit;
        WProto = proto;
        Type = 4;
        Spin = 0.0f;
    }
    ~SWasterAnimation() override {}                              // 0x5c7570
    void InitModel(SIModel* model) override;                     // 0x5ca600
    void UpdateModel() override;                                 // 0x5cf790
    float* GetFirePosition(float* out, int gunner, float dirOffset, float kick) override;   // 0x5cb3c0

    SPWasterAnimation* WProto;       // +0x24
    float              Spin;         // +0x28 grenade / catapult stone tumble
};
PZ_HD_SIZE(SWasterAnimation, kHdSizeSWasterAnimation);

// PANZERS 0x5c7b20
SIUnitAnimation* SPWasterAnimation::CreateAnimation(SIUnit* unit)
{
    PZ_M3_TRACE("SPWasterAnimation::CreateAnimation (0x5c7b20)");
    return new SWasterAnimation(this, unit);
}

// PANZERS 0x5ca600
void SWasterAnimation::InitModel(SIModel* model)
{
    (void)model;
    Spin = 0.0f;
    Model()->SetFlags(0x11);                                      // +0x94
}

// PANZERS 0x5cb3c0
float* SWasterAnimation::GetFirePosition(float* out, int gunner, float dirOffset, float kick)
{
    (void)gunner; (void)dirOffset; (void)kick;
    out[0] = out[1] = out[2] = 0.0f;
    return out;
}

// PANZERS 0x5cf790
void SWasterAnimation::UpdateModel()
{
    SIUnit* u = Unit;
    SIModel* m = Model();
    if (!m)
        return;
    SUnitAnimEnv& e = g_UnitAnimEnv;
    bool show = !UnitField<bool>(u, kUnitUnplaced) &&
                (e.NoFogOfWar() || !e.HasGameLogic() || e.CanSeeGroundUnit(e.LocalPlayer(), u));
    m->SetVisible(show, true);                                    // +0x30
    const SHdStr& name = PUnitField<SHdStr>(UnitField<SIPUnit*>(u, kUnitPUnit), kPUnitName);
    bool grenade = name.len != 0 && _stricmp(name.buf, "Waster german grenade") == 0;
    bool catapult = !grenade && name.len != 0 && _stricmp(name.buf, "Waster catapult") == 0;
    float* pos = &UnitField<float>(u, kUnitPos);
    if (grenade || catapult) {
        // The thrown object tumbles along its flight direction.
        float vx = UnitField<float>(u, 0xbc);
        float vz = UnitField<float>(u, 0xc4);
        float v2 = vx * vx + vz * vz;
        m->SetPosition(pos[0], pos[1], pos[2]);                   // +0x18
        if ((double)v2 > 0.0001) {                                // 0x7f1b50
            float a = DAtan2f((double)vx, (double)vz);            // 0x78d07a, fstp dword
            m->SetRotation(a, 0.0f, 0.0f);                        // +0x1c
            AnimModelSetNodeTilt(m, 0, 0.0f, 0.0f, 0.0f, Spin, 0.0f, 0.0f);   // +0x48 node 0
        } else {
            m->SetRotation(0.0f, 0.0f, 0.0f);
        }
        Spin = grenade ? Spin + 0.4f : Spin - 0.4f;               // 0x7f5e20
    } else {
        m->SetPosition(pos[0], pos[1], pos[2]);
    }
    if (UnitField<bool>(u, kUnitStartDie)) {
        UnitField<int>(u, kUnitDieTicks) = 1;
        UnitField<bool>(u, kUnitStartDie) = false;
    }
}

} // namespace pz
