// src/game/projectile.cpp
// SProjectileUnit / SProjectileDriver / SProjectileAnimation, and the
// SGameLogic hit / area-damage rows the projectiles and gunners need.
// OWNER: M3-C sub-agent C1. See projectile.h.

#include <math.h>
#include <string.h>
#include "projectile.h"
#include "waster.h"
#include "gunnermath.h"
#include "gunner.h"
#include "idriver.h"
#include "unitprops.h"
#include "unitextern.h"
#include "world.h"
#include "worldapi.h"
#include "gamelogic.h"
#include "drivermath.h"
#include "pzunitregistry.h"
#include "pz/ipixie.h"
#include "pz/imodel.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {

PZ_HD_SIZE(SProjectileUnit, kHdSizeSProjectileUnit);
#if defined(_M_IX86)
static_assert(offsetof(SProjectileUnit, HomingTarget) == 0x344, "0x55a740 +0x344");
static_assert(offsetof(SProjectileUnit, Shooter) == 0x354, "0x584d00 +0x354");
static_assert(offsetof(SProjectileUnit, Burning) == 0x359, "0x584d00 +0x359");
static_assert(offsetof(SProjectileUnit, Range) == 0x368, "0x584d00 +0x368");
#endif

static bool UnitLive(int i)
{
    return g_World && g_World->Units.IsLive(i);
}

static SUnit* LiveUnit(int i)                                // SHeapTRB::operator[] 0x546490
{
    if (!UnitLive(i))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", i);
    return WorldUnit(i);
}

static int ClassOf(SUnit* u)
{
    return u->Proto->ClassType;
}

static bool NameIs(SPUnit* p, const char* s)                 // SString::operator== 0x52c410 (case-insensitive)
{
    return _stricmp(SStr(p->Name), s) == 0;
}

// ---------------------------------------------------------------------------
// SProjectileUnit

// PANZERS 0x5a3810
SProjectileUnit::SProjectileUnit(SPProjectileUnit* proto, int worldIndex)
    : SUnit(proto, worldIndex)                                // 0x5b2820
{
    Start[0] = Start[1] = Start[2] = 0.0f;
    P = proto;
    Damage = 0.0f;
    DamageRadius = 0.0f;
    Shooter = -1;
    Cannonade = false;
    Burning = false;
    WeaponType = 0;
    Range = 0.0f;
    HomingTarget = -1;
}

// PANZERS 0x5a38f0
SProjectileUnit::~SProjectileUnit()
{
}

// PANZERS 0x5a3950
void SProjectileUnit::InitNew(int player, const float* pos, float dir, int p4, float hp)
{
    SUnit::InitNew(player, pos, dir, p4, hp);                 // 0x5bace0
    if (g_GameLogic)
        RefreshTargeting();                                   // +0x34
}

// PANZERS 0x5a3920
void SProjectileUnit::EC_Die()
{
    Wrecked = true;                                           // +0x150
}

// PANZERS 0x5bcb50 (empty in HD)
static void UnitNop_5bcb50()
{
}

// PANZERS 0x5a39b0
// The dead projectile: the animation (0x5cc480) answers +0x155 with
// +0x15c = 1, the next tick removes the unit.
void SProjectileUnit::RefreshDead()
{
    unsigned char* b = (unsigned char*)this;
    if (_15c == 0) {
        b[0x155] = 1;
        return;
    }
    if (--_15c == 0) {
        UnitNop_5bcb50();
        g_World->RemoveUnit(WorldIndex);                      // 0x5f8060
    }
}

// PANZERS 0x5a3990
void SProjectileUnit::RefreshTargeting()
{
    FillNearUnits(50.0f, 0.0f);                               // 0x5b78a0 (0x42480000, 0)
}

// PANZERS 0x5a4410
void SProjectileUnit::SetDamage(float damage, float radius, int weaponType)
{
    Damage = damage;
    DamageRadius = radius;
    WeaponType = weaponType;
}

// PANZERS 0x5a4450
void SProjectileUnit::SetVelocity(float x, float y, float z)
{
    float* v = Velocity();
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

// PANZERS 0x5a38b0
bool SProjectileUnit::MovedFromStart()
{
    return !(Start[0] == Pos[0] && Start[1] == Pos[1] && Start[2] == Pos[2]);
}

static void PlayUp(const SUnitArray<SPUnitEffect>& fx, float x, float y, float z)
{
    for (int i = 0; i < fx.Size; ++i) {
        float pos[3] = { x + 0.0f, y + 1.0f, z + 0.0f };     // DAT_007f1b58
        float up[3] = { 0.0f, 1.0f, 0.0f };
        if (g_Pixie)
            g_Pixie->PlayEffect(g_Scene, fx.Array[i].Proto, pos, up, 0);   // pixie +0x24
    }
}

// PANZERS 0x5a4470
void SProjectileUnit::GroundHit(float x, float y, float z)
{
    if (_20c)
        PlayUp(P->GroundIncidence, x, y, z);
}

// PANZERS 0x5a47f0
void SProjectileUnit::WaterHit(float x, float y, float z)
{
    if (_20c)
        PlayUp(P->WaterIncidence, x, y, z);
}

// PANZERS 0x5a4580
// Buildings get a persistent wood (material 1) / stone effect tied to their
// model (pixie +0x2c, +0x58, +0x38); other units a one-shot unit effect.
void SProjectileUnit::UnitHit(float x, float y, float z, SUnit* hit)
{
    if (!_20c || !g_Pixie)
        return;
    float pos[3] = { x, y, z };
    float up[3] = { 0.0f, 1.0f, 0.0f };
    if (ClassOf(hit) == 9) {
        const SUnitArray<SPUnitEffect>& fx =
            ((SPBuildingUnit*)hit->Proto)->BuildingMaterial == 1 ? P->WoodIncidence : P->StoneIncidence;
        for (int i = 0; i < fx.Size; ++i) {
            int e = g_Pixie->CreateEffect(g_Scene, fx.Array[i].Proto, pos, up);   // +0x2c
            // HD: pixie +0x58 (effect, building model) attaches it to the
            // model; SIPixie::Slot_58 is not typed yet (agent E).
            g_Pixie->ReleaseEffect(e);                        // +0x38
        }
    } else {
        for (int i = 0; i < P->UnitIncidence.Size; ++i)
            g_Pixie->PlayEffect(g_Scene, P->UnitIncidence.Array[i].Proto, pos, up, 0);
    }
}

// SWasterUnit::Explode 0x5d2880 (waster.cpp, C3): the Molotov fire on the ground.
static void WasterIgniteGround(SUnit* waster)
{
    static_cast<SWasterUnit*>(waster)->Explode();
}

// PANZERS 0x5a39f0
// Per tick: the active driver moves the projectile (0x55a740); outside the
// map it is removed. Fire weapons damage along the way and burn out after
// their range; other projectiles hit a unit (0x562c20), the ground or the
// water, play the incidence effects and damage the area.
void SProjectileUnit::RefreshMisc()
{
    if (Shooter >= 0 && !UnitLive(Shooter))
        Shooter = -1;
    int ad = ActiveDriver;
    if (ad > -1) {
        if (ad >= Drivers.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SIDriver *", ad);
        Drivers.Array[ad]->Refresh();                         // +0x14
    }
    if (HomingTarget >= 0 && !UnitLive(HomingTarget))
        HomingTarget = -1;
    float x = Pos[0], z = Pos[2];
    float w = (float)g_World->TerrainW, h = (float)g_World->TerrainH;
    if (!(x > 0.0f) || !(z > 0.0f) || !(w > x) || !(h > z) || x >= w || z >= h) {
        g_World->RemoveUnit(WorldIndex);                      // 0x5f8060
        return;
    }
    if (Burning && MovedFromStart()) {
        int hit = g_GameLogic->ProjectileHitTest(Shooter, this);   // 0x562c20
        if (hit >= 0 && ClassOf(LiveUnit(hit)) != 6 && ClassOf(LiveUnit(hit)) != 5) {
            LiveUnit(hit)->TakeDamage(Damage, WeaponType, Shooter, Pos[0], Pos[1], Pos[2], 1);   // +0x94
            g_GameLogic->DamageArea(Damage * 0.5f, Shooter, hit, Pos[0], Pos[1], Pos[2], DamageRadius, 0, WeaponType);
            EC_Die();                                         // +0x124
        }
        float dx = Start[0] - Pos[0], dy = Start[1] - Pos[1], dz = Start[2] - Pos[2];
        float d2 = dx * dx + dy * dy + dz * dz;
        g_GameLogic->GetFrame();                              // 0x56d1a0 (result unused)
        if (d2 > 2.9f) {                                      // DAT_007fa7b4
            float gy = g_World->GetTerrainHeight(Pos[0], Pos[2]);   // 0x5e7730
            g_GameLogic->DamageArea(Damage * 0.5f, Shooter, -1, Pos[0], gy, Pos[2], 0.5f, 0, WeaponType);
        }
        if (!(d2 > Range * Range)) {
            if (!(g_World->GetTerrainHeight(Pos[0], Pos[2]) >= Pos[1])) {
                if (g_World->GetWaterHeight(Pos[0], Pos[2]) < Pos[1])   // 0x5ec490
                    return;
            }
        }
        g_GameLogic->DamageArea(Damage * 0.5f, Shooter, -1, Pos[0], Pos[1], Pos[2], DamageRadius, 0, WeaponType);
        EC_Die();
        return;
    }
    if (Cannonade && NameIs(P, "Projectile cannonade")) {
        if (g_World->GetTerrainHeight(Pos[0], Pos[2]) + 17.5f > Pos[1]) {   // DAT_007fa7b8 (x87 fadd, CW 0x007F)
            Cannonade = false;
            float up[3] = { 0.0f, 1.0f, 0.0f };
            if (g_Pixie)
                g_Pixie->PlayEffect(g_Scene, g_GameLogic->ShellFallFx, Pos, up, 0);   // +0x310
        }
    }
    int hit = g_GameLogic->ProjectileHitTest(Shooter, this);  // 0x562c20
    if (hit >= 0 && ClassOf(LiveUnit(hit)) != 6 && ClassOf(LiveUnit(hit)) != 5) {
        if (NameIs(P, "Projectile Molotov coctail")) {
            float fp[3] = { Pos[0], g_World->GetTerrainHeight(Pos[0], Pos[2]), Pos[2] };
            int fire = g_World->CreateUnit(Player, "Waster Molotov Fire", fp, 0.0f, 0, 1.0f, -1, true, "");   // 0x5e3170
            if (LiveUnit(hit)->Proto->ArmourType == 2) {
                WasterSetTarget(LiveUnit(fire), hit);         // 0x5d2870
                SUnit* f = LiveUnit(fire);
                f->Pos[0] = Pos[0];
                f->Pos[1] = Pos[1];
                f->Pos[2] = Pos[2];
            }
        }
        UnitHit(Pos[0], Pos[1], Pos[2], LiveUnit(hit));       // 0x5a4580
        LiveUnit(hit)->TakeDamage(Damage, WeaponType, Shooter, Pos[0], Pos[1], Pos[2], 1);
        if (!(DamageRadius == 0.0f))
            g_GameLogic->DamageArea(Damage, Shooter, hit, Pos[0], Pos[1], Pos[2], DamageRadius, 0, WeaponType);   // 0x576490
        EC_Die();
        return;
    }
    if (!(WorldGroundHeight(Pos[0], Pos[2]) >= Pos[1])) {     // 0x5e9720
        if (g_World->GetWaterHeight(Pos[0], Pos[2]) < Pos[1])
            return;
    }
    if (NameIs(P, "Projectile Molotov coctail")) {
        float fp[3] = { Pos[0], WorldGroundHeight(Pos[0], Pos[2]), Pos[2] };
        int fire = g_World->CreateUnit(Player, "Waster Molotov Fire", fp, 0.0f, 0, 1.0f, -1, true, "");
        WasterIgniteGround(LiveUnit(fire));                   // 0x5d2880
    }
    if (!(DamageRadius == 0.0f))
        g_GameLogic->DamageArea(Damage, Shooter, hit, Pos[0], Pos[1], Pos[2], DamageRadius, 0, WeaponType);
    float gy = WorldGroundHeight(Pos[0], Pos[2]);
    float wy = g_World->GetWaterHeight(Pos[0], Pos[2]) - 0.01f;   // x87 fsub (CW 0x007F), DAT_007f1b48
    if (wy > gy) {
        Pos[1] = g_World->GetWaterHeight(Pos[0], Pos[2]);
        WaterHit(Pos[0], Pos[1], Pos[2]);                     // 0x5a47f0
    } else {
        Pos[1] = WorldGroundHeight(Pos[0], Pos[2]);
        GroundHit(Pos[0], Pos[1], Pos[2]);                    // 0x5a4470
    }
    EC_Die();
}

// ---------------------------------------------------------------------------
// SGameLogic / SWorld rows of other agents that the combat path needs.
// M3-C: lifted here by C1 (agent O / F rows); the integrator moves them when
// their owners lift the surrounding code.

// PANZERS 0x5e9720
float WorldGroundHeight(float x, float z)
{
    SWorld* w = g_World;
    if (0.0f > x || x >= (float)w->TerrainW || 0.0f > z || z >= (float)w->TerrainH)
        return 0.0f;
    const float* h = w->Heights;
    int ix = (int)floorf(x);                                  // fistp, CW 0x047f (round down)
    int iz = (int)floorf(z);
    float fx = x - (float)ix;
    float fz = z - (float)iz;
    int i = (w->TerrainW + 1) * iz + ix;
    int j = w->TerrainW + i;
    return h[i + 1] * (1.0f - fz) * fx + h[i] * (1.0f - fz) * (1.0f - fx)
         + h[j + 1] * fz * (1.0f - fx) + h[j + 2] * fz * fx;
}

// PANZERS 0x5630c0
int BuildingBetween(int unit, int exclude, const float* from, const float* to)
{
    int carrier = -1;
    if (unit >= 0)
        carrier = LiveUnit(unit)->Parent;
    for (int i = 0; i < g_World->Units.Size; ++i) {
        if (!UnitLive(i))
            continue;
        if (i == unit || i == carrier || i == exclude)
            continue;
        SUnit* b = WorldUnit(i);
        if (ClassOf(b) != 9 || b->Wrecked)
            continue;
        if (b->Parent != -1 && b->Parent == unit)
            continue;
        // HD: building model +0xd8 (segment against the mesh); SIModel slot
        // +0xd8 is not typed yet (agent E): no building blocks the shot.
        (void)from;
        (void)to;
        STUB_LOG("BuildingBetween (0x5630c0) model +0xd8 segment test");
        PZ_M3_TRACE("BuildingBetween (0x5630c0)");
    }
    return -1;
}

// PANZERS 0x562c20
// The unit (or doodad) a projectile hits: doodads of its 8x8 grid cell whose
// mesh contains it (returns the projectile itself), then the units in its
// sight list (+0x1b4, filled every second by 0x5a3990) except the shooter,
// the shooter's carrier and itself, not projectiles / class 4 / wasters nor
// the shooter's passengers: buildings by mesh, others within 1.0 of the
// point 0.5 above them.
int SGameLogic::ProjectileHitTest(int shooter, SUnit* p)
{
    float px = p->Pos[0], pz = p->Pos[2];
    if (0.0f > px || px > (float)g_World->TerrainW || 0.0f > pz || pz > (float)g_World->TerrainH)
        return -1;
    // HD walks the doodad grid +0x1b8 / +0x1bc (built by 0x564c20, agent O)
    // and asks each doodad model +0xd0 whether it contains the projectile.
    STUB_LOG("SGameLogic::ProjectileHitTest (0x562c20) doodad grid 0x564c20 / model +0xd0");
    PZ_M3_TRACE("SGameLogic::ProjectileHitTest (0x562c20)");
    int carrier = shooter < 0 ? -1 : LiveUnit(shooter)->Parent;
    for (int i = 0; i < p->SightUnits.Size; ++i) {
        int idx = p->SightUnits.Array[i].Unit;                // 0x5463d0
        if (!p->IsTargetable(idx, false))                     // 0x5bb6b0
            continue;
        SUnit* u = LiveUnit(idx);
        if (idx == shooter || idx == carrier || idx == p->WorldIndex)
            continue;
        int ct = ClassOf(u);
        if (ct == 3 || ct == 4 || ct == 7)
            continue;
        if (u->Parent != -1 && u->Parent == shooter)
            continue;
        if (ct == 9) {
            STUB_LOG("SGameLogic::ProjectileHitTest (0x562c20) building model +0xd0");
            continue;
        }
        float dz = u->Pos[2] - p->Pos[2];
        double dy = ((double)u->Pos[1] + 0.5) - (double)p->Pos[1];   // 0x7ea760
        float dx = u->Pos[0] - p->Pos[0];
        float d = (float)((double)(dx * dx) + dy * dy + (double)(dz * dz));
        if (1.0f > d)
            return u->WorldIndex;
    }
    return -1;
}

// PANZERS 0x576490
// Explosion: doodads in the radius are crushed (the doodad grid, see
// ProjectileHitTest), then every live unit except `exclude`, projectiles,
// squads, class 4, planes and unplaced units, and soldiers inside
// buildings, takes damage * (1 - sqrt(d2 / r2)) within the radius (units on
// a class 0xb carrier through 0x5c3dd0).
void SGameLogic::DamageArea(float damage, int attacker, int exclude, float x, float y, float z, float radius,
                            int hitMode, int weaponType)
{
    float r2 = radius * radius;
    if (0.0f > x || x > (float)g_World->TerrainW || 0.0f > z || z > (float)g_World->TerrainH)
        return;
    STUB_LOG("SGameLogic::DamageArea (0x576490) doodad grid 0x564c20 (CrushDoodad 0x5e3c80)");
    PZ_M3_TRACE("SGameLogic::DamageArea (0x576490)");
    for (int i = 0; i < g_World->Units.Size; ++i) {
        if (!UnitLive(i))
            continue;
        if (i == exclude)
            continue;
        SUnit* u = WorldUnit(i);
        int ct = ClassOf(u);
        if (ct == 3 || ct == 5 || ct == 4 || ct == 8 || u->Unplaced)
            continue;
        if (ct == 6 && u->Parent >= 0 && ClassOf(LiveUnit(u->Parent)) == 9)
            continue;
        float dx = x - u->Pos[0], dy = y - u->Pos[1], dz = z - u->Pos[2];
        float d2 = dx * dx + dy * dy + dz * dz;
        if (!(r2 > d2))
            continue;
        float f = (float)sqrt((double)(d2 / r2));
        float dmg = (1.0f - f) * damage;
        if (u->Parent > -1 && ClassOf(LiveUnit(u->Parent)) == 0xb)
            u->DamageMembers(true, dmg, weaponType, attacker, x, y, z, hitMode);   // 0x5c3dd0 (push 1)
        else
            u->TakeDamage(dmg, weaponType, attacker, x, y, z, hitMode);       // +0x94
    }
}

} // namespace pz
