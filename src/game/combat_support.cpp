// src/game/combat_support.cpp
// The support calls of SGameLogic (artillery, recon plane, tactical bomber,
// heavy bomber, paratroopers: 0x5674c0 / 0x568300 / 0x568740 / 0x567760 /
// 0x567d40), the border start point 0x56cff0 and the trigger events 9 / 10
// (0x570ac0 / 0x570bc0). OWNER: agent M3-C sub-agent C3.
//
// Callers: SGameLogic::ProcessPacket 0x5737c0 (agent O; packets 0x1c..0x20,
// free = false) and the AI / triggers (free = true). Every call creates its
// units through SWorld::CreateUnit 0x5e3170 with the player's nation name.
// Only the artillery draws the world seed (two draws per shell).

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "gamelogic.h"
#include "flying.h"
#include "flying_math.h"
#include "projectile.h"
#include "drivermath.h"
#include "trigger.h"
#include "world.h"
#include "worldapi.h"
#include "logger.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {

static inline int& PlayerI(int player, unsigned off) { return *(int*)(g_World->Players[player] + off); }
static inline float& PlayerF(int player, unsigned off) { return *(float*)(g_World->Players[player] + off); }
struct SHdTrig {                                                  // flying_math.h policy: the HD CRT
    static double Sin(double x) { return DSin(x); }               // 0x78d640
    static double Cos(double x) { return DCos(x); }               // 0x78d480
};
static inline int Bits(float f) { int i; memcpy(&i, &f, 4); return i; }

static const char* NationName(int player, const char* ge, const char* su, const char* us)
{
    int nation = PlayerI(player, 0x04);                           // World+0x174
    return nation == 0 ? ge : (nation == 2 ? su : us);
}

// The player's support counter (World+0x19c + 4 * kind + player * 0x48):
// false = no call left.
static bool ChargeSupport(bool free, int player, unsigned off)
{
    int& n = PlayerI(player, off);
    if (n == 0)
        return free;
    if (!free)
        --n;
    return true;
}

// LocalPlayer's team rule (World+0x17c): true when `player` is on the local
// player's side.
static bool IsLocalSide(int player)
{
    int lp = g_World->LocalPlayer;
    int team = PlayerI(lp, 0x0c);
    if (team == 0)
        return lp == player;
    return team == PlayerI(player, 0x0c);
}

// The new plane's position history (HD copies +0x8c to +0x98 and +0xa4 after
// raising it by the extra altitude).
static void RaisePlane(SFlyingUnit* u, float extraAltitude)
{
    if (extraAltitude != -1.0f)                                   // 0x7f5a98
        u->Altitude = extraAltitude;                              // +0x384
    u->Pos[1] = u->Altitude + u->Pos[1];
    memcpy(u->PrevPos, u->Pos, sizeof(u->Pos));
    memcpy(u->PrevPos2, u->Pos, sizeof(u->Pos));
}

static void SetPlaneGroundTracking(SUnit* u, bool on)
{
    if (u->Drivers.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SDriver *", 0);
    FlyingDriverSetGroundTracking(u->Drivers.Array[0], on);       // 0x55af10
}

// PANZERS 0x5674c0
// 16 shells fall around (x, z): +-5 m (world seed), 20 m up in 6 m steps.
void SGameLogic::SupportArtillery(bool free, float x, float z, int player)
{
    PZ_M3_TRACE("SGameLogic::SupportArtillery (0x5674c0)");
    if (!ChargeSupport(free, player, 0x2c))
        return;
    float dir = (float)DAtan2((double)(x - 1.0f), (double)(z - 1.0f));   // 0x78d07a, fstp qword
    for (int i = 0; i < 0x10; ++i) {
        int r1 = WorldRand();
        int r2 = WorldRand();
        int ox = (int)((double)r1 * 3.0517578125e-05 * 10.0);    // 0x7f4540, 0x7f4578
        int oz = (int)((double)r2 * 3.0517578125e-05 * 10.0);
        float pos[3];
        pos[0] = (float)(ox - 5) + x;
        pos[1] = (float)(i * 6 + 0x14);
        pos[2] = (float)(oz - 5) + z;
        int u = g_World->CreateUnit(player, "Projectile cannonade", pos, dir, 0, 1.0f, -1, true, "");   // 0x5e3170
        FlyingSetProjectileDamage(WorldUnit(u), 90.0f, 5.0f, 2);  // 0x5a4410
        FlyingSetProjectileVelocity(WorldUnit(u), 0.0f, 9.999999747378752e-05f, 0.0f);   // 0x5a4450
        if (i == 0) {
            static_cast<SProjectileUnit*>(WorldUnit(u))->Cannonade = true;   // +0x358
            if (g_World->LocalPlayer == player)
                g_World->UnitSpeech(u, 0x1a, false);              // 0x5fff20
        }
    }
}

// PANZERS 0x568300
void SGameLogic::SupportRecon(bool free, float x, float z, int player, float extraAltitude, bool groundTracking)
{
    PZ_M3_TRACE("SGameLogic::SupportRecon (0x568300)");
    if (!ChargeSupport(free, player, 0x30))
        return;
    float pos[3];
    pos[0] = PlayerF(player, 0x24);                               // World+0x194
    pos[2] = PlayerF(player, 0x28);                               // World+0x198
    pos[1] = g_World->GetTerrainHeight(pos[0], pos[2]);
    double dir = DAtan2((double)(x - pos[0]), (double)(z - pos[2]));
    const char* name = NationName(player, "Ge Recon plane", "SU Recon plane", "US Recon plane");
    int u = g_World->CreateUnit(player, name, pos, (float)dir, 0, 1.0f, -1, true, "");
    SFlyingUnit* p = static_cast<SFlyingUnit*>(WorldUnit(u));
    p->EC_Move(Bits(x), Bits(z), 0, false, 0);                    // +0xac
    p->Target[0] = x;                                             // +0x354
    p->Target[2] = z;                                             // +0x35c
    RaisePlane(p, extraAltitude);
    SetPlaneGroundTracking(p, groundTracking);
    if (g_World->LocalPlayer == player)
        g_World->UnitSpeech(u, 0x16, false);
}

// PANZERS 0x568740
void SGameLogic::SupportTacBomber(bool free, float x, float z, int player)
{
    PZ_M3_TRACE("SGameLogic::SupportTacBomber (0x568740)");
    if (!ChargeSupport(free, player, 0x34))
        return;
    float y = g_World->GetTerrainHeight(x, z) + 22.0f;            // fadd 0x7f35dc
    float ex = PlayerF(player, 0x24);
    float ez = PlayerF(player, 0x28);
    (void)g_World->GetTerrainHeight(ex, ez);                      // HD computes and drops it
    double dir = DAtan2((double)(x - ex), (double)(z - ez));
    float df = (float)dir;
    float pos[3];
    pos[0] = x - (float)DSin((double)df) * 21.0f;                 // 0x7f7f88
    pos[1] = y;
    pos[2] = z - (float)DCos((double)df) * 21.0f;
    const char* name = NationName(player, "Ge TacBomber", "SU TacBomber", "US TacBomber");
    int u = g_World->CreateUnit(player, name, pos, (float)dir, 0, 1.0f, -1, true, "");
    SFlyingUnit* p = static_cast<SFlyingUnit*>(WorldUnit(u));
    p->Bombs = 1;                                                 // +0x348
    p->EC_Move(Bits(x), Bits(z), 0, false, 0);
    g_World->UnitSpeech(u, IsLocalSide(player) ? 0x17 : 0x13, false);
}

// The plane's start for the heavy bomber and the transport.
static void PlaneStart(SGameLogic* gl, float* start, float x, float z, int player, bool p6, bool fromDir, float dir)
{
    if (fromDir && p6) {
        float s[3];
        gl->ComputeBorderStart(s, x, 6.0f, z, dir);               // 0x56cff0
        start[0] = s[0];
        start[2] = s[2];
    } else {
        start[0] = PlayerF(player, 0x24);
        start[2] = PlayerF(player, 0x28);
    }
    start[1] = g_World->GetTerrainHeight(start[0], start[2]);
}

// The trigger events of the locations that contain (x, z).
static void DispatchLocations(SGameLogic* gl, int unit, float x, float z, bool bomber)
{
    SHeap<SLocation>& L = g_World->Locations;
    for (int i = 0; i < L.Size; ++i) {
        if (L.Array[i].Next != kHeapLive)
            continue;
        const SLocation& l = L.Array[i].Data;
        if (x < (float)l.X1 || !((float)l.X2 > x) || z < (float)l.Z1 || !((float)l.Z2 > z))
            continue;
        if (bomber)
            gl->DispatchBomberSent(unit, i);                      // 0x570ac0
        else
            gl->DispatchParatroopersSent(unit, i);                // 0x570bc0
    }
}

// PANZERS 0x567760 SGameLogic::EC_HeavyBomber
void SGameLogic::SupportHeavyBomber(bool free, float x, float z, int player, float extraAltitude, bool p6,
                                    bool fromDir, float dir)
{
    PZ_M3_TRACE("SGameLogic::SupportHeavyBomber (0x567760)");
    if (!ChargeSupport(free, player, 0x38))
        return;
    float start[3];
    PlaneStart(this, start, x, z, player, p6, fromDir, dir);
    double d = DAtan2((double)(x - start[0]), (double)(z - start[2]));
    const char* name = NationName(player, "Ge Heavy Bomber", "SU Heavy Bomber", "US Heavy Bomber");
    int u = g_World->CreateUnit(player, name, start, (float)d, 0, 1.0f, -1, true, "");
    SFlyingUnit* p = static_cast<SFlyingUnit*>(WorldUnit(u));
    p->EC_Move(Bits(x), Bits(z), 0, true, Bits((float)d));       // +0xac: arrive heading d
    p->Target[0] = x;
    p->Target[1] = 6.0f;
    p->Target[2] = z;
    p->Bombs = 5;
    RaisePlane(p, extraAltitude);
    SetPlaneGroundTracking(p, p6);
    DispatchLocations(this, u, x, z, true);
    if (g_World->LocalPlayer == player)
        g_World->UnitSpeech(u, 0x18, false);
}

// PANZERS 0x567d40
void SGameLogic::SupportParatroopers(bool free, float x, float z, int player, float extraAltitude, bool p6,
                                     bool fromDir, float dir)
{
    PZ_M3_TRACE("SGameLogic::SupportParatroopers (0x567d40)");
    if (!ChargeSupport(free, player, 0x3c))
        return;
    float start[3];
    PlaneStart(this, start, x, z, player, p6, fromDir, dir);
    double d = DAtan2((double)(x - start[0]), (double)(z - start[2]));
    // HD: the Soviet transport is the "SU Heavy Bomber".
    const char* name = NationName(player, "Ge Transport Plane", "SU Heavy Bomber", "US Transport Plane");
    int u = g_World->CreateUnit(player, name, start, (float)d, 0, 1.0f, -1, true, "");
    SFlyingUnit* p = static_cast<SFlyingUnit*>(WorldUnit(u));
    p->SetupTransport();                                          // 0x55e370
    p->EC_Move(Bits(x), Bits(z), 0, false, 0);
    p->Target[0] = x;
    p->Target[1] = 6.0f;
    p->Target[2] = z;
    RaisePlane(p, extraAltitude);
    SetPlaneGroundTracking(p, p6);
    DispatchLocations(this, u, x, z, false);
    if (g_World->LocalPlayer == player)
        g_World->UnitSpeech(u, 0x19, false);
}

// PANZERS 0x56cff0
float* SGameLogic::ComputeBorderStart(float* out, float x, float y, float z, float dir)
{
    (void)y;
    if (!FlyingBorderStart<SHdTrig>(out, x, z, dir, g_World->TerrainW, g_World->TerrainH))
        Logger.g->Panic("SGameLogic::EC_HeavyBomber: Can't compute start position");
    return out;
}

// Event 9 / 10 dispatch: every trigger of that event whose parameter is the
// location, with Unit = the plane.
static void DispatchSupportEvent(SGameLogic* gl, int type, int unit, int location)
{
    SRunningTrigger rt;
    memset(&rt, 0, sizeof(rt));                                   // memset 0x34
    STriggerArray<STrigger>* t = reinterpret_cast<STriggerArray<STrigger>*>(&g_World->Triggers);
    for (int i = 0; i < t->Size; ++i) {
        if (t->Array[i].Event.Type != type)
            continue;
        if (t->Array[i].Event.Param != location)
            continue;
        rt.Trigger = i;
        rt.Unit = unit;                                           // +0x0c
        rt.Unit2 = -1;                                            // +0x10
        gl->CheckConditions(&rt);                                 // 0x580600
        t = reinterpret_cast<STriggerArray<STrigger>*>(&g_World->Triggers);
    }
    free(rt.Found.Units);
}

// PANZERS 0x570ac0
void SGameLogic::DispatchBomberSent(int unit, int location)
{
    PZ_M3_TRACE("SGameLogic::DispatchBomberSent (0x570ac0)");
    DispatchSupportEvent(this, TE_BOMBER_SENT, unit, location);
}

// PANZERS 0x570bc0
void SGameLogic::DispatchParatroopersSent(int unit, int location)
{
    PZ_M3_TRACE("SGameLogic::DispatchParatroopersSent (0x570bc0)");
    DispatchSupportEvent(this, TE_PARATROOPERS_SENT, unit, location);
}

// ---------------------------------------------------------------------------
// Recompile test hook (not in HD): PZ_M3C_TESTSUPPORT=n (1 artillery, 2 recon,
// 3 tactical bomber, 4 heavy bomber, 5 paratroopers) makes one free support
// call at logic frame PZ_M3C_TESTFRAME (default 400) for the local player at
// the camera target. Off by default (no effect on the CRC).
void SGameLogic::SupportTestHook()
{
    static int s_kind = -1, s_frame = 400;
    if (s_kind < 0) {
        const char* e = getenv("PZ_M3C_TESTSUPPORT");
        s_kind = e ? atoi(e) : 0;
        const char* f = getenv("PZ_M3C_TESTFRAME");
        if (f)
            s_frame = atoi(f);
    }
    if (s_kind <= 0 || GetFrame() != s_frame || !g_World)
        return;
    int lp = g_World->LocalPlayer;
    float x = g_World->CamTarget[0];                              // the point the camera looks at
    float z = g_World->CamTarget[2];
    Logger.g->Log(0, "PZM3C TESTSUPPORT %d at frame %d: player %d target %g %g entry %g %g", s_kind, s_frame, lp,
                  (double)x, (double)z, (double)PlayerF(lp, 0x24), (double)PlayerF(lp, 0x28));
    switch (s_kind) {
    case 1: SupportArtillery(true, x, z, lp); break;
    case 2: SupportRecon(true, x, z, lp, -1.0f, true); break;
    case 3: SupportTacBomber(true, x, z, lp); break;
    case 4: SupportHeavyBomber(true, x, z, lp, -1.0f, true, false, 0.0f); break;
    case 5: SupportParatroopers(true, x, z, lp, -1.0f, true, false, 0.0f); break;
    default: break;
    }
}

} // namespace pz
