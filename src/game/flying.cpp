// src/game/flying.cpp
// SFlyingUnit (0x55c9d0..0x55e3e0), SFlyingDriver / SPFlyingDriver
// (0x550150, 0x5510d0, 0x551ef0, 0x552410, 0x554fb0, 0x555d30, 0x5586a0)
// and SFlyingAnimation (0x5c63c0, 0x5c6af0, 0x5c8d00, 0x5cb120, 0x5cb3e0,
// 0x5cb750). OWNER: agent M3-C sub-agent C3. See flying.h.
//
// Determinism: the plane's ctor draws the world seed once (RollStep), the
// level drop timer once per drop, the paratroop squad's direction once
// (0x555a00) and each parachutist four times (parachute.cpp). The x87 sums
// of HD (terrain height + constant, fadd dword at the game's 24-bit
// precision, fstp dword) are float additions here.

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "flying.h"
#include "aigroup.h"
#include "flyinganim.h"
#include "flying_math.h"
#include "projectile.h"
#include "drivermath.h"
#include "idriver.h"
#include "gamelogic.h"
#include "unitprops.h"
#include "gunner.h"
#include "target.h"
#include "unitanim_model.h"
#include "campaign.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/imodel.h"
#include "pz/iscene.h"
#include "logger.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {

PZ_HD_SIZE(SFlyingAnimation, kHdSizeSFlyingAnimation);
#if defined(_M_IX86)
static_assert(offsetof(SFlyingUnit, P) == 0x340, "+0x340");
static_assert(offsetof(SFlyingUnit, Target) == 0x354, "+0x354");
static_assert(offsetof(SFlyingUnit, DiveSpeed) == 0x36c, "+0x36c");
static_assert(offsetof(SFlyingUnit, Altitude) == 0x384, "+0x384");
static_assert(offsetof(SFlyingAnimation, PropAngle) == 0x48, "+0x48");
static_assert(offsetof(SFlyingAnimation, Spring) == 0x58, "+0x58");
static_assert(offsetof(SFlyingAnimation, AntennaReset) == 0xa0, "+0xa0");
static_assert(offsetof(SFlyingAnimation, Bombs) == 0xa4, "+0xa4");
#endif

// ---------------------------------------------------------------------------
// Helpers

static inline float& FF(void* p, unsigned off) { return *(float*)((char*)p + off); }
static inline int& FI(void* p, unsigned off) { return *(int*)((char*)p + off); }
static inline unsigned char& FB(void* p, unsigned off) { return *(unsigned char*)((char*)p + off); }

// World+0x170 + player * 0x48 (offsets of the player record).
static inline int& PlayerI(int player, unsigned off) { return *(int*)(g_World->Players[player] + off); }

static inline float Height(float x, float z) { return g_World->GetTerrainHeight(x, z); }   // 0x5e7730

struct SHdTrig {                                                  // flying_math.h policy: the HD CRT
    static double Sin(double x) { return DSin(x); }               // 0x78d640
    static double Cos(double x) { return DCos(x); }               // 0x78d480
};

static const char* NationName(int player, const char* ge, const char* su, const char* us)
{
    int nation = PlayerI(player, 0x04);                           // World+0x174
    if (nation == 0)
        return ge;
    if (nation == 2)
        return su;
    return us;
}

// SProjectileUnit setters (C1's class, projectile.h) on the shells.
void FlyingSetProjectileVelocity(SUnit* p, float x, float y, float z)
{
    static_cast<SProjectileUnit*>(p)->SetVelocity(x, y, z);       // 0x5a4450
}
void FlyingSetProjectileDamage(SUnit* p, float damage, float radius, int weaponType)
{
    static_cast<SProjectileUnit*>(p)->SetDamage(damage, radius, weaponType);   // 0x5a4410
}

static SGunner* Gunner0(SUnit* u)
{
    if (u->Gunners.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
    return u->Gunners.Array[0];
}

static const char* FirstProjectile(SGunner* g)
{
    SPGunner* pg = g->GetPGunner();                               // +0x2c
    if (pg->Projectiles.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SString", 0);
    const char* s = pg->Projectiles.Array[0].buf;
    return s ? s : "";
}

// The bomb's damage from gunner 0 of the plane (HD order: weapon type,
// radius, damage).
static void SetBombDamage(SUnit* plane, SUnit* bomb)
{
    int wt = Gunner0(plane)->GetWeaponType();                     // 0x584240
    float radius = Gunner0(plane)->GetPGunner()->DamageRadius;    // +0x5c
    float damage = Gunner0(plane)->GetPGunner()->Damage;          // +0x58
    FlyingSetProjectileDamage(bomb, damage, radius, wt);          // 0x5a4410
}

// World+0x4f4 SHeap<SAIGroup> (element 0x54: Next + 0x50 data).
struct SAIGroupElem { int Next; unsigned char Data[0x50]; };
static SHeap<unsigned char[0x50]>& AIGroups() { return g_World->AIGroups; }
static unsigned char* AIGroupRawAt(int i)                            // 0x55ccc0
{
    SHeap<unsigned char[0x50]>& h = AIGroups();
    if (i < 0 || i >= h.Size || h.Array[i].Next != kHeapLive)
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "struct SAIGroup", i);
    return (unsigned char*)&h.Array[i].Data;
}
static int AIGroupNext(int i)                                     // 0x55ce90
{
    SHeap<unsigned char[0x50]>& h = AIGroups();
    for (++i; i < h.Size; ++i)
        if (h.Array[i].Next == kHeapLive)
            return i;
    return -1;
}
static int AIGroupAlloc()                                         // 0x55cd40
{
    SHeap<unsigned char[0x50]>& h = AIGroups();
    h.Count++;
    int i = h.Free;
    if (i >= 0) {
        h.Free = h.Array[i].Next;
        h.Array[i].Next = kHeapLive;
        memset(&h.Array[i].Data, 0, 0x50);
        return i;
    }
    if (h.Size == h.Max) {
        int n = h.Max < 0x10 ? 0x10 : (h.Max * 6) / 5;
        h.Array = (SHeapElem<unsigned char[0x50]>*)realloc(h.Array, (size_t)n * 0x54);
        memset((char*)h.Array + (size_t)h.Max * 0x54, 0, (size_t)(n - h.Max) * 0x54);
        h.Max = n;
    }
    h.Array[h.Size].Next = kHeapLive;
    return h.Size++;
}
// SAIGroup::ComputeStartPos 0x601060 (aigroup.cpp).
static void AIGroup_601060(unsigned char* grp)
{
    reinterpret_cast<SAIGroup*>(grp)->ComputeStartPos();
}
static void AIGroupInit(unsigned char* grp)                       // 0x5ecbe0
{
    FI(grp, 0x10) = 0;
    FI(grp, 0x14) = 0;
    FI(grp, 0x34) = -1;
    FB(grp, 0x24) = 0;
    FF(grp, 0x30) = 600.0f;                                       // 0x44160000
    FI(grp, 0x1c) = 0;
    FI(grp, 0x20) = 0;
    AIGroup_601060(grp);
}
static bool SStringEqualI(const SString& a, const char* b)        // 0x55cbd0
{
    int lb = (int)strlen(b);
    if (a.size != lb)
        return false;
    if (a.size != 0 && _stricmp(a.buf, b) != 0)
        return false;
    return true;
}

// SUnit death effects (unitbase.cpp, C2).
void FlyingUnitDeathEffects(SUnit* u)                             // SUnit 0x5c26e0
{
    u->PlayDeathEffects();
}
void FlyingUnitWreckEffects(SUnit* u)                             // SUnit 0x5c2590
{
    u->PlayDiedByFireEffects();
}

// ---------------------------------------------------------------------------
// SFlyingUnit

// PANZERS 0x55c9d0
SFlyingUnit::SFlyingUnit(SPFlyingUnit* proto, int worldIndex) : SUnit(proto, worldIndex)
{
    PZ_M3_TRACE("SFlyingUnit::SFlyingUnit (0x55c9d0)");
    Target[0] = Target[1] = Target[2] = 0.0f;
    _360[0] = _360[1] = _360[2] = 0;
    Velocity[0] = Velocity[1] = Velocity[2] = 0.0f;
    P = proto;
    _350 = -1;
    Timer = 0;
    Bombs = 0;
    Squads = 0;
    DiveSpeed = 0.4f;                                             // 0x3ecccccd
    Pitch = 45.0f;                                                // 0x42340000
    int r = WorldRand();
    RollStep = (int)((double)r * 3.0517578125e-05 * 2.0) != 0 ? 0.02f : -0.02f;   // 0x7f4530 / 0x7f5e3c
    if (g_UnitRegistry)
        g_UnitRegistry->GetPUnit(NationName(Player, "Ge Parachute Squad", "SU Parachute Squad",
                                            "US Parachute Squad"), true);   // 0x5d0e70(name, 1)
    if (Drivers.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SDriver *", 0);
    Altitude = *(float*)((char*)Drivers.Array[0]->GetPDriver() + 0x4c);   // SPFlyingDriver +0x4c Altitude
    if (P->PlaneType == 2) {
        Invulnerable = true;                                      // +0x111
        Timer = 0x1e;
    }
}

// PANZERS 0x55cd10
SFlyingUnit::~SFlyingUnit() {}

// PANZERS 0x55cef0
void SFlyingUnit::Init(SUnitDef* def)
{
    SUnit::Init(def);                                             // 0x5ba8e0
    if (g_GameLogic)
        RefreshTargeting();                                       // +0x34
}

// PANZERS 0x55cf20
void SFlyingUnit::InitNew(int player, const float* pos, float dir, int p4, float hp)
{
    SUnit::InitNew(player, pos, dir, p4, hp);                     // 0x5bace0
    if (g_GameLogic)
        RefreshTargeting();
}

// PANZERS 0x55cf90
void SFlyingUnit::RefreshTargeting()
{
    if (!IsWaitingAfterStuck())                                   // 0x5bb400
        FillNearUnits(20.0f, 0.0f);                               // 0x5b78a0
}

// PANZERS 0x55cf80
void SFlyingUnit::OnDriverReachedTarget()
{
    RemoveMe = true;                                              // +0x334
}

// PANZERS 0x55ce00
void SFlyingUnit::EC_Move(int xBits, int zBits, int p3, bool p4, int p5)
{
    if (ActiveDriver >= 0)
        SUnit::EC_Move(xBits, zBits, p3, p4, p5);                 // 0x5b8ea0
}

// PANZERS 0x55e370
void SFlyingUnit::SetupTransport()
{
    Squads = 2;
    Altitude = Altitude + 7.0f;                                   // 0x7f5e38
    if (g_UnitRegistry)
        g_UnitRegistry->GetPUnit(NationName(Player, "Ge Parachute Member", "SU Parachute Member",
                                            "US Parachute Member"), true);
}

// PANZERS 0x55cfc0
// The wreck: falls on (RefreshMisc) until it touches the ground, then
// explodes on the second last tick and is removed.
void SFlyingUnit::RefreshDead()
{
    PZ_M3_TRACE("SFlyingUnit::RefreshDead (0x55cfc0)");
    if (_15c == 0) {
        FB(this, 0x155) = 1;                                      // start the death animation
        FlyingUnitDeathEffects(this);                             // 0x5c26e0
    }
    if (0.0f >= HP) {
        float h1 = Height(Pos[0], Pos[2]) + 1.0f;                 // fadd 0x7f1b58
        if (Pos[1] > h1) {
            RefreshMisc();                                        // +0x38
            return;
        }
    }
    if (_15c == 1 && g_GameLogic)
        g_GameLogic->DamageArea(100.0f, -1, -1, Pos[0], Pos[1], Pos[2], 10.0f, 0, 2);
    if (--_15c == 0) {
        // 0x5bcb50 (empty)
        g_World->RemoveUnit(WorldIndex);                          // 0x5f8060
    } else if (_15c == 1) {
        FlyingUnitWreckEffects(this);                             // 0x5c2590
    }
}

// PANZERS 0x55d0c0 SFlyingUnit::ServerRefresh (vtable +0x38)
void SFlyingUnit::RefreshMisc()
{
    PZ_M3_TRACE("SFlyingUnit::ServerRefresh (0x55d0c0)");
    if (MainGunner >= 0 && P->PlaneType == 2) {
        if (Timer > 0) {
            --Timer;
            return;
        }
        Invulnerable = false;
    }

    // A shot-down plane near the ground: what it hits explodes with it.
    bool dead = false;
    float h5 = Height(Pos[0], Pos[2]) + 5.0f;                     // fadd 0x7f5a6c
    if (h5 > Pos[1] && 0.0f >= HP && g_GameLogic) {
        int hit = g_GameLogic->ProjectileHitTest(-1, this);
        if (hit >= 0) {
            if (hit != WorldIndex && WorldUnit(hit)->Proto->ArmourType == 0) {
                SUnit* h = WorldUnit(hit);
                if (h->ActiveDriver > 0)
                    h->EC_Die();                                  // +0x124
            } else {
                WorldUnit(hit)->TakeDamage(500.0f, 2, -1, Pos[0], Pos[1], Pos[2], 1);   // +0x94
                g_GameLogic->DamageArea(500.0f, -1, hit, Pos[0], Pos[1], Pos[2], 4.0f, 0, 2);
                EC_Die();                                         // +0x124
            }
            dead = true;
        }
    }
    if (!dead) {
        float h = Height(Pos[0], Pos[2]);
        if (h >= Pos[1] && 0.0f >= HP) {
            if (g_GameLogic)
                g_GameLogic->DamageArea(100.0f, -1, -1, Pos[0], Pos[1], Pos[2], 10.0f, 0, 2);
            Pos[1] = Height(Pos[0], Pos[2]);
            EC_Die();
        }
    }

    if (P->PlaneType == 2) {
        // Tactical bomber: dives along its direction (Pitch 45 -> -55 degrees
        // in 4 degree steps once the bombs are gone; +1 per tick when shot).
        SFlyingDive d = { Pitch, HP, Bombs, DiveSpeed, Dir, { Pos[0], Pos[1], Pos[2] },
                          { Velocity[0], Velocity[1], Velocity[2] }, FF(this, 0xd4), RollStep };
        bool leave = FlyingDiveStep<SHdTrig>(d, Height);
        Pitch = d.Pitch;
        DiveSpeed = d.DiveSpeed;
        memcpy(Pos, d.Pos, sizeof(Pos));
        memcpy(Velocity, d.Velocity, sizeof(Velocity));
        FF(this, 0xd4) = d.Roll;
        if (leave)
            RemoveMe = true;
    } else {
        float y0 = Pos[1];
        if (ActiveDriver >= 0) {
            if (ActiveDriver >= Drivers.Size)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SDriver *", ActiveDriver);
            Drivers.Array[ActiveDriver]->Refresh();               // +0x14
        }
        if (HP == 0.0f)
            Pos[1] = y0 - 0.1f;                                   // 0x7f59a8
    }

    if (MainGunner >= 0 && P->PlaneType == 2) {
        // The dive: below 10 m every bomb is released towards the target.
        float t = Pos[1] - Height(Pos[0], Pos[2]);                // fsubr, fstp dword
        if (!(10.0f > t) || Bombs <= 0)
            return;
        SFlyingAnimation* an = (SFlyingAnimation*)Anim;
        int n = an->BombCount;                                    // anim +0xa8
        if (n == 0) {
            // HD 0x65cac0 (a Warning box); logged here.
            Logger.g->Log(1, "FlyingUnit: SuspensionsNum problema");
            SGunner* g = GetGunner(0);                            // 0x55cc40
            const char* name = FirstProjectile(g);                // 0x55cc80(0)
            int b = g_World->CreateUnit(Player, name, Pos, Dir, 0, 1.0f, -1, true, "");   // 0x5e3170
            SUnit* bomb = WorldUnit(b);
            FlyingSetProjectileVelocity(bomb, Velocity[0], Velocity[1], Velocity[2]);
            SIDriver* bd = WorldUnit(b)->GetDriver(0);            // 0x55cc00
            if (FB(bd->GetPDriver(), 0x44) != 0)
                FlyingSetProjectileVelocity(WorldUnit(b), Velocity[0], Velocity[1], Velocity[2]);
            static_cast<SProjectileUnit*>(WorldUnit(b))->Shooter = WorldIndex;   // +0x354
            SetBombDamage(this, WorldUnit(b));
            if (Bombs > 0)
                --Bombs;
            return;
        }
        for (int i = 0; i < n; ++i) {
            float p[3] = { 0.0f, 0.0f, 0.0f };
            Model->GetNodePosition(an->Bombs[i].Node, p);         // +0x54
            const char* name = FirstProjectile(Gunner0(this));
            int b = g_World->CreateUnit(Player, name, p, Dir, 0, 1.0f, -1, true, "");
            FlyingSetProjectileVelocity(WorldUnit(b), Velocity[0], Velocity[1], Velocity[2]);
            SUnit* bomb = WorldUnit(b);
            if (bomb->Drivers.Size < 1)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SDriver *", 0);
            if (FB(bomb->Drivers.Array[0]->GetPDriver(), 0x44) != 0) {
                // Guided bombs fly at the target with twice the plane's speed.
                if (!CurrentTarget)
                    Logger.g->Panic("SFlyingUnit::ServerRefresh: No driver target");
                float a = Pitch * 0.017453292f;
                float hd = DAtan2f((double)(CurrentTarget->Pos[0] - p[0]),
                                   (double)(CurrentTarget->Pos[2] - p[2]));   // 0x78d07a, fstp dword
                double ca = DCos((double)a);
                float vx = (float)(DSin((double)hd) * (double)DiveSpeed * ca * 2.0);
                float vy = Velocity[1] * 2.0f;                    // 0x7f4558
                float vz = (float)(DCos((double)hd) * (double)DiveSpeed * ca * 2.0);
                FlyingSetProjectileVelocity(WorldUnit(b), vx, vy, vz);
            }
            static_cast<SProjectileUnit*>(WorldUnit(b))->Shooter = WorldIndex;   // +0x354
            SetBombDamage(this, WorldUnit(b));
            if (Bombs > 0)
                --Bombs;
            SIModel*& m = an->Bombs[i].Model;
            if (m) {
                m->Slot_E0();                                     // +0xe0 detach
                if (m) {
                    m->Release();                                 // +0x04
                    m = nullptr;
                }
            }
        }
        return;
    }

    // Level flight: bombs or paratroopers once near the target.
    float dx = Target[0] - Pos[0];
    float dz = Target[2] - Pos[2];
    double d = sqrt((double)(dx * dx + dz * dz));                 // 0x78d090
    if (!((24.0 > d && Bombs > 0) || (5.0 > d && Squads > 0)))
        return;
    if (Timer-- != 0)
        return;
    int r = WorldRand();
    Timer = 2 - (int)((double)r * -3.0517578125e-05 * 4.0);       // 0x7f4598, 0x7ea770
    if (Bombs > 0) {
        --Bombs;
        const char* name = FirstProjectile(Gunner0(this));
        int b = g_World->CreateUnit(Player, name, Pos, Dir, 0, 1.0f, -1, true, "");
        float vx = (float)(DSin((double)Dir) * (double)Speed);    // +0xc8
        float vz = (float)(DCos((double)Dir) * (double)Speed);
        FlyingSetProjectileVelocity(WorldUnit(b), vx, 9.999999747378752e-05f, vz);   // 0x7f5e1c
        static_cast<SProjectileUnit*>(WorldUnit(b))->Shooter = WorldIndex;   // +0x354
        SetBombDamage(this, WorldUnit(b));
        return;
    }
    if (Squads <= 0)
        return;
    --Squads;
    if (g_Campaign && g_Campaign->GameMode == 4 && PlayerI(Player, 0x1c) == 2)   // 0x594d20, World+0x18c
        return;
    const char* sqName = NationName(Player, "Ge Parachute Squad", "SU Parachute Squad", "US Parachute Squad");
    int sq = g_World->CreateUnit(Player, sqName, Pos, Dir, 0, 1.0f, -1, true, "");
    float sx = Pos[0], sz = Pos[2];
    SUnit* squad = WorldUnit(sq);
    float out[2];
    g_World->FindEmptySpace(out, sx, sz, Dir + 3.1415927f, squad->UnitSizeBlocks, squad->MoveFlags, true);   // 0x5e58d0
    float h = Height(out[0], out[1]);
    int rd = HdRandInt(3);                                        // 0x555a00
    float dir = (float)rd + Dir;
    int dirBits, hBits;
    memcpy(&dirBits, &dir, 4);
    memcpy(&hBits, &h, 4);
    WorldUnit(sq)->SetPosition(out[0], out[1], dirBits, hBits);   // +0x24
    int hookArg[2] = { 1, 2 };
    WorldUnit(sq)->_140 = true;
    WorldUnit(sq)->_14c = true;
    WorldUnit(sq)->Hook20((int)(intptr_t)hookArg);                // +0x20 (squads 0x59fab0)
    ParachuteSquadDrop(WorldUnit(sq), Pos[1]);                    // 0x5a1140

    if (PlayerI(Player, 0x08) == 1) {                             // World+0x178: an AI player
        char gname[64];
        _snprintf(gname, sizeof(gname) - 1, "Paratroopers %d", WorldIndex);   // 0x7f5dd0
        gname[sizeof(gname) - 1] = 0;
        for (int g = AIGroupNext(-1); g >= 0; g = AIGroupNext(g)) {
            if (SStringEqualI(*(SString*)AIGroupRawAt(g), gname)) {
                WorldUnit(sq)->SetAIGroup(g);                     // 0x5c0c10
                return;
            }
        }
        int g = AIGroupAlloc();
        AIGroupInit(AIGroupRawAt(g));                                // 0x5ecbe0
        *(SString*)AIGroupRawAt(g) = gname;                          // 0x52c320
        WorldUnit(sq)->SetAIGroup(g);
        AIGroup_601060(AIGroupRawAt(g));
        if (g_Campaign && g_Campaign->GameMode == 4) {
            FI(AIGroupRawAt(g), 8) = 4;
            FI(AIGroupRawAt(g), 0xc) = 2;
        } else {
            FI(AIGroupRawAt(g), 8) = 2;
        }
    }
}

// ---------------------------------------------------------------------------
// SPFlyingAnimation::CreateAnimation / SFlyingAnimation

// PANZERS 0x5c77c0 (operator new 0xb0)
SIUnitAnimation* SPFlyingAnimation::CreateAnimation(SIUnit* unit)
{
    PZ_M3_TRACE("SPFlyingAnimation::CreateAnimation (0x5c77c0)");
    return new SFlyingAnimation(this, unit);
}

// PANZERS 0x5c63c0
SFlyingAnimation::SFlyingAnimation(SPFlyingAnimation* proto, SIUnit* unit) : SUnitAnimation(proto, unit)
{
    Type = 5;
    FProto = proto;
    Unit = unit;
    Gear = nullptr;
    BodyNode = AntennaNode = -1;
    PropNode[0] = PropNode[1] = PropNode[2] = -1;
    PropAngle[0] = PropAngle[1] = PropAngle[2] = 0.0f;
    _54 = 0;
    if (proto->RunningGear)
        Gear = new SRunningGear(proto->RunningGear);              // new 0x38, 0x5a9cc0
    AntennaReset = true;
    _a1[0] = _a1[1] = _a1[2] = 0;
    Spring[0] = Spring[1] = 0.0;
    SpringVel[0] = SpringVel[1] = 0.0;
    Guns = nullptr;
    Muzzle = 0;
    Rod[0] = Rod[1] = 0.0;
    RodVel[0] = RodVel[1] = 0.0;
    AntennaPrev[0] = AntennaPrev[1] = 0.0f;
    Bombs = nullptr;
    BombCount = 0;
    _ac = 0;
}

// PANZERS 0x5c6af0 (0x5c7150 deleting)
SFlyingAnimation::~SFlyingAnimation()
{
    if (Guns) {
        int n = PUnitField<unsigned char>(UnitField<SIPUnit*>(Unit, kUnitPUnit), kPUnitGunCount);
        for (int i = 0; i < n; ++i) {
            free(Guns[i].S);
            Guns[i].S = nullptr;
            free(Guns[i].M);
            Guns[i].M = nullptr;
        }
        free(Guns);
        Guns = nullptr;
    }
    for (int i = 0; i < BombCount; ++i) {
        if (Bombs[i].Model) {
            Bombs[i].Model->Slot_E0();                            // +0xe0 detach
            if (Bombs[i].Model) {
                Bombs[i].Model->Release();                        // +0x04
                Bombs[i].Model = nullptr;
            }
        }
    }
    free(Bombs);
    Bombs = nullptr;
    delete Gear;
    Gear = nullptr;
}

SAnimGun* UnitAnimLoadGuns(SAnimGun*& guns, SIUnit* unit, SIModel* model, bool warnNoMuzzle);   // unitanim.cpp

// PANZERS 0x5c8d00
void SFlyingAnimation::InitModel(SIModel* model)
{
    PZ_M3_TRACE("SFlyingAnimation::InitModel (0x5c8d00)");
    Model()->SetFlags(3);                                         // +0x94
    if (Gear)
        Gear->InitModel(model);                                   // 0x5aa180
    BodyNode = model->FindNode("body");                           // +0x40
    PropNode[0] = model->FindNode("prop0");
    PropNode[1] = model->FindNode("prop1");
    PropNode[2] = model->FindNode("prop2");
    if (PUnitField<unsigned char>(UnitField<SIPUnit*>(Unit, kUnitPUnit), kPUnitGunCount) != 0)
        UnitAnimLoadGuns(Guns, Unit, model, false);
    AntennaNode = model->FindNode("antenna");

    // The bombs hang on "suspension%d": one model of gunner 0's first
    // projectile type per node.
    int n = 0;
    for (;; ++n) {
        char name[32];
        _snprintf(name, sizeof(name) - 1, "suspension%d", n);
        name[sizeof(name) - 1] = 0;
        if (model->FindNode(name) < 0)
            break;
    }
    Bombs = (SBombSlot*)malloc((size_t)(n > 0 ? n : 1) * sizeof(SBombSlot));
    for (int i = 0; i < n; ++i) {
        char name[32];
        _snprintf(name, sizeof(name) - 1, "suspension%d", i);
        name[sizeof(name) - 1] = 0;
        Bombs[i].Node = model->FindNode(name);
        Bombs[i].Model = nullptr;
        SUnit* u = (SUnit*)Unit;
        const char* pname = FirstProjectile(Gunner0(u));
        SPUnit* pp = g_UnitRegistry ? g_UnitRegistry->GetPUnit(pname, true) : nullptr;   // 0x5d0e70
        SIModel* bm = (g_Scene && pp) ? g_Scene->CreateModelFromPrototype(pp->ModelProto, 1) : nullptr;   // scene +0x58
        Bombs[i].Model = bm;
        if (bm) {
            bm->SetPosition(0.0f, 0.0f, 0.0f);                    // +0x18
            bm->AttachTo(model, Bombs[i].Node);                   // +0xdc
        }
    }
    BombCount = n;
}

// PANZERS 0x5cb120
float* SFlyingAnimation::GetFirePosition(float* out, int gunner, float dirOffset, float kick)
{
    double a = (double)UnitField<float>(Unit, kUnitDir) + (double)dirOffset;
    if (a > 3.1415927410125732)                                   // DAT_007f4560
        a -= 6.2831854820251465;                                  // DAT_007f4570
    else if (-3.1415927410125732 > a)                             // DAT_007f5aa0
        a += 6.2831854820251465;
    out[0] = out[1] = out[2] = 0.0f;
    ++Muzzle;
    if (Muzzle > Guns[gunner].MCount - 1)
        Muzzle = 0;
    Model()->GetNodePosition(Guns[gunner].M[Muzzle], out);        // +0x54
    float af = (float)a;
    SpringVel[0] = SpringVel[0] - DSin((double)af) * (double)kick;
    SpringVel[1] = SpringVel[1] - DCos((double)af) * (double)kick;
    return out;
}

// PANZERS 0x5cb3e0
void SFlyingAnimation::AddBodyKick(float x, float y, float z)
{
    (void)y;
    SpringVel[0] = SpringVel[0] - (double)x;
    SpringVel[1] = SpringVel[1] - (double)z;
}

// PANZERS 0x5cb750
void SFlyingAnimation::UpdateModel()
{
    PZ_M3_TRACE("SFlyingAnimation::UpdateModel (0x5cb750)");
    SIUnit* u = Unit;
    SIModel* m = Model();
    if (!m)
        return;
    SUnitAnimEnv& e = g_UnitAnimEnv;
    if (!UnitField<bool>(u, kUnitUnplaced) &&
        (e.NoFogOfWar() || !e.HasGameLogic() || e.CanSeeGroundUnit(e.LocalPlayer(), u))) {
        m->SetVisible(true, true);                                // +0x30(1, 1)
        int lp = e.LocalPlayer();
        int team = e.PlayerTeam(lp);
        bool same = team == 0 ? lp == UnitField<int>(u, kUnitPlayer)
                              : team == e.PlayerTeam(UnitField<int>(u, kUnitPlayer));
        bool done = same;
        if (!done && !UnitField<bool>(u, kUnitSpotted)) {
            if (!e.HasGameLogic()) {
                done = true;
            } else if (UnitField<int>(u, kUnitSpottedFrame) < e.Frame() - 100) {
                // "Enemy plane": the nearest unit (class 0 or 5) of the
                // local player says it.
                UnitField<bool>(u, kUnitSpotted) = true;
                int best = -1;
                int bestD = 10000;
                const float* up = &UnitField<float>(u, kUnitPos);
                SUnitHeap& H = g_World->Units;
                for (int i = 0; i < H.Size; ++i) {
                    if (H.Array[i].Next != kHeapLive)
                        continue;
                    SUnit* o = H.Array[i].Unit;
                    int ct = o->Proto->ClassType;
                    if ((ct != 0 && ct != 5) || o->Player != g_World->LocalPlayer)
                        continue;
                    float dz = up[2] - o->Pos[2];
                    float dz2 = dz * dz;
                    float dx = up[0] - o->Pos[0];
                    int d = (int)(dx * dx + dz2);                 // cvttss2si
                    if (d < bestD) {
                        bestD = d;
                        best = i;
                    }
                }
                if (best >= 0)
                    g_World->UnitSpeech(WorldUnit(best)->WorldIndex, 0x13, false);   // 0x5fff20
                done = true;
            }
        }
        if (!done && e.HasGameLogic())
            UnitField<int>(u, kUnitSpottedFrame) = e.Frame();     // 0x56d1a0
    } else {
        m->SetVisible(false, true);
    }

    float* pos = &UnitField<float>(u, kUnitPos);
    m->SetPosition(pos[0], pos[1], pos[2]);                       // +0x18
    m->SetRotation(UnitField<float>(u, kUnitDir) + 3.1415927f, 0.0f, 0.0f);   // +0x1c
    SFlyingUnit* fu = (SFlyingUnit*)u;
    float pitch = fu->P->PlaneType == 2 ? fu->Pitch * 0.017453292f : 0.0f;
    float roll = UnitField<float>(u, kUnitSteer);
    if (UnitField<bool>(u, kUnitReverse))
        roll = -roll;
    AnimModelSetNodeTilt(m, BodyNode, 0.0f, 0.0f, 0.0f, 0.0f, roll, pitch);   // +0x48
    for (int k = 0; k < 3; ++k) {
        if (PropNode[k] > -1) {
            double a = (double)PropAngle[k] + 0.5235987901687622;    // 0x7fe0b8
            if (a > 3.1415927410125732)
                a -= 6.2831854820251465;
            else if (-3.1415927410125732 > a)
                a += 6.2831854820251465;
            PropAngle[k] = (float)a;
            AnimModelSetNodeTilt(m, PropNode[k], 0.0f, 0.0f, 0.0f, PropAngle[k], 0.0f, 0.0f);
        }
    }
    if (Gear) {
        float steer = UnitField<float>(u, kUnitSteer);
        if (UnitField<bool>(u, kUnitReverse))
            steer = -steer;
        Gear->Update(m, pos[0], pos[2], (double)UnitField<float>(u, kUnitDir), steer * 0.5f);   // 0x5aa2c0
    }

    // Body sway (the slopes are flat for a plane: sin(atan(0)) = 0).
    float t0 = (float)DSin(HdAtan(0.0));                          // 0x78d190, 0x78d640
    const SPFlyingAnimation* p = FProto;
    const float* prev = &UnitField<float>(u, kUnitPrevPos);
    Spring[0] = (double)(prev[0] - pos[0]) * 1.5 + SpringVel[0] * 0.05 + Spring[0];
    double s0 = Spring[0];
    Spring[1] = (double)(prev[2] - pos[2]) * 1.5 + SpringVel[1] * 0.05 + Spring[1];
    double s1 = Spring[1];
    double slopeX = (double)t0 * 0.125;
    double slopeZ = (double)t0 * 0.125;
    SpringVel[0] = (slopeX - (double)p->SpringStrength * s0) + SpringVel[0];
    SpringVel[1] = (slopeZ - (double)p->SpringStrength * s1) + SpringVel[1];
    double len2 = s1 * s1 + s0 * s0;
    if ((double)p->MaxSpringAngle2 + 0.01 < len2) {
        double k = (double)p->MaxSpringAngle / sqrt(len2);        // 0x78d090
        Spring[0] = s0 * k;
        Spring[1] = s1 * k;
        SpringVel[0] = (double)(prev[0] - pos[0]) * -1.5 * 20.0;
        SpringVel[1] = (double)(prev[2] - pos[2]) * -1.5 * 20.0;
    }
    Spring[0] = (double)p->SpringDecay * Spring[0];
    Spring[1] = (double)p->SpringDecay * Spring[1];

    if (AntennaNode < 0) {
        AntennaReset = true;
        Rod[0] = Rod[1] = 0.0;
        RodVel[0] = RodVel[1] = 0.0;
    } else {
        float ap[3] = { 0.0f, 0.0f, 0.0f };
        AnimModelGetNodePosition(m, AntennaNode, ap);             // +0x54
        if (!AntennaReset) {
            double rx = (double)(AntennaPrev[0] - ap[0]) * 0.9 + RodVel[0] * 0.05 + Rod[0];
            Rod[0] = rx;
            double rz = (double)(AntennaPrev[1] - ap[2]) * 0.9 + RodVel[1] * 0.05 + Rod[1];
            Rod[1] = rz;
            RodVel[0] = (slopeX - (double)p->RodSpringStrength * rx) + RodVel[0];
            RodVel[1] = (slopeZ - (double)p->RodSpringStrength * rz) + RodVel[1];
            Rod[0] = (double)p->RodSpringDecay * rx;
            Rod[1] = (double)p->RodSpringDecay * rz;
        }
        AntennaPrev[0] = ap[0];
        AntennaPrev[1] = ap[2];
        AntennaReset = false;
    }

    int guns = PUnitField<unsigned char>(UnitField<SIPUnit*>(u, kUnitPUnit), kPUnitGunCount);
    if (guns == 0) {
        double cd = DCos((double)UnitField<float>(u, kUnitDir));
        double sd = DSin((double)UnitField<float>(u, kUnitDir));
        AnimModelSetNodeTilt(m, AntennaNode, 0.0f, 0.0f, 0.0f, 0.0f,
                             (float)((cd * Rod[0] - sd * Rod[1]) * (double)p->RodSpringScale),
                             (float)((cd * Rod[1] + sd * Rod[0]) * (double)p->RodSpringScale));
    } else {
        int gunners = UnitField<int>(u, kUnitGunnerCount);
        unsigned char** g = UnitField<unsigned char**>(u, kUnitGunners);
        for (int i = 0; i < guns; ++i) {
            if (i < 0 || i >= gunners)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", i);
            double cd = DCos((double)UnitField<float>(u, kUnitDir));
            double sd = DSin((double)UnitField<float>(u, kUnitDir));
            float yaw = *reinterpret_cast<const float*>(g[i] + kGunnerYaw);
            float bx = (float)((cd * Spring[0] - sd * Spring[1]) * (double)p->SpringScale);
            float bz = (float)((cd * Spring[1] + sd * Spring[0]) * (double)p->SpringScale);
            AnimModelSetNodeTilt(m, Guns[i].H, 0.0f, 0.0f, 0.0f, -yaw, bx, bz);
            for (int k = 0; k < Guns[i].SCount; ++k) {
                if (i >= gunners)
                    Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", i);
                float recoil = *reinterpret_cast<const float*>(g[i] + kGunnerRecoil);
                AnimModelSetNodeTilt(m, Guns[i].S[k], 0.0f, recoil, 0.0f, 0.0f, 0.0f, 0.0f);
            }
            if (gunners < 1)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
            double a = (double)*reinterpret_cast<const float*>(g[0] + kGunnerYaw);
            double ca = DCos(a);
            double sa = DSin(a);
            float tz = (float)((ca * Rod[1] + sa * Rod[0]) * (double)p->RodSpringScale);
            double ca2 = DCos(a);
            double sa2 = DSin(a);
            float tx = (float)((ca2 * Rod[0] - sa2 * Rod[1]) * (double)p->RodSpringScale);
            AnimModelSetNodeTilt(m, AntennaNode, 0.0f, 0.0f, 0.0f, 0.0f, tx, tz);
        }
    }

    if (UnitField<bool>(u, kUnitStartDie)) {
        UnitField<int>(u, kUnitDieTicks) = 3;
        UnitField<bool>(u, kUnitStartDie) = false;
        FlyingUnitDeathEffects((SUnit*)u);                        // 0x5c26e0
    }
}

} // namespace pz
