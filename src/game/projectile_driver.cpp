// src/game/projectile_driver.cpp
// SPProjectileDriver / SProjectileDriver (OWNER: M3-C sub-agent C1).

#include <math.h>
#include <string.h>
#include "projectile.h"
#include "projectile_driver.h"
#include "unitprops.h"
#include "gunnermath.h"
#include "unitprops.h"
#include "world.h"
#include "worldapi.h"
#include "gamelogic.h"
#include "drivermath.h"
#include "pz/imodel.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {

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

PZ_HD_SIZE(SProjectileDriver, kHdSizeSProjectileDriver);
PZ_HD_SIZE(SPProjectileDriver, 0x4c);
#if defined(_M_IX86)
static_assert(offsetof(SPProjectileDriver, RocketPropulsion) == 0x44, "0x556180 +0x44");
static_assert(offsetof(SProjectileDriver, Fuel) == 0xe8, "0x551230 +0xe8");
#endif

// ---------------------------------------------------------------------------
// SPProjectileDriver / SProjectileDriver

// PANZERS 0x556180
void SPProjectileDriver::Load(SProperties* propsp, const char* name, int nameLen)
{
    SUPropStruct* props = (SUPropStruct*)propsp;
    Type = 6;
    if (props->GetMultiIndex("DriverType") != Type)
        Logger.g->Panic("SPProjectileDriver::Init - bad DriverType");
    SUPropStruct* s = props->GetMultiSubStruct("DriverType");
    MaxSpeed = (((s->GetFloat("MoveSpeed") / 3.6f) * 100.0f) / 20.0f) * 0.005f;   // 0x7f5a68 0x7ee558 0x7f35d8 0x7f5994
    RocketPropulsion = s->GetBool("RocketPropulsion");
    MaxFuel = s->GetFloat("MaxFuel");
    SPDriver::Load(propsp, name, nameLen);                    // the name (0x52c320 on +0x18)
}

// PANZERS 0x551230
SIDriver* SPProjectileDriver::CreateDriver(SIUnit* unit)
{
    return new SProjectileDriver(this, unit);                 // new 0xec
}

SProjectileDriver::SProjectileDriver(SPProjectileDriver* pd, SIUnit* unit)
    : SDriver(pd, unit)                                       // 0x54f4b0
{
    PDriver2 = pd;                                            // +0xe4
    Fuel = (int)pd->MaxFuel;
}

// PANZERS 0x550330
SProjectileDriver::~SProjectileDriver()
{
}

// PANZERS 0x55a740
// Moves the projectile by its velocity; after the rocket fuel (prototype
// +0x44) it steers ballistically onto its homing target (+0x344) and falls
// (gunnermath.cpp ProjectileDriverStep, emulator-verified).
void SProjectileDriver::Refresh()
{
    SProjectileUnit* p = (SProjectileUnit*)Unit;
    const float* target = nullptr;
    // HD drops a dead homing target (+0x344 = -1) after the move, before
    // steering; the step below reads it only after the move too.
    float* vel = p->Velocity();
    if (vel[0] * vel[0] + vel[1] * vel[1] + vel[2] * vel[2] == 0.0f)
        return;
    bool straight = ((SPProjectileDriver*)PDriver2)->RocketPropulsion;
    if (!(straight && Fuel - (Fuel > 0 ? 1 : 0) != 0)) {
        if (p->HomingTarget >= 0 && !UnitLive(p->HomingTarget))
            p->HomingTarget = -1;
        if (p->HomingTarget >= 0)
            target = LiveUnit(p->HomingTarget)->Pos;
    }
    ProjectileDriverStep(p->Pos, vel, &Fuel, straight, target);
}

} // namespace pz
