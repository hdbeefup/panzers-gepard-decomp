// src/game/parachute.cpp
// The paratroopers: SPanzersParachuteDriver (0x5502a0, 0x551150, 0x557d00,
// 0x55a360), SPPanzersParachuteDriver::Load (0x555ee0), and the drop of a
// paratroop squad (SPanzersSquadUnit 0x5a1140: C5's class, lifted here as a
// free function because only the transport plane calls it; the member part
// 0x598cb0 is C5's SPanzersSquadMemberUnit::ParachuteDrop). OWNER: agent M3-C sub-agent C3.
//
// A parachutist is a squad member (ClassType 6) whose driver 1 is the
// parachute driver; SPPanzersSquadMemberUnit loads the parachute_*.4da model
// (+0x350). The drop draws the world seed four times per member.

#include <math.h>
#include <string.h>
#include "flying.h"
#include "unitanim.h"
#include "squadunit.h"
#include "drivermath.h"
#include "world.h"
#include "worldapi.h"
#include "pz/imodel.h"
#include "logger.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {


// ---------------------------------------------------------------------------
// The landing (SPanzersParachuteDriver::Refresh 0x55a360, flyingdriver.cpp):
// the member's walker animation returns to its stand sequence.
void ParachuteMemberLanded(SUnit* m)
{
    SWalkerAnimation* an = static_cast<SWalkerAnimation*>(m->Anim);
    an->PlayGlobalStand(true);                                    // 0x5cae80
    an->ResetRelax();                                             // 0x5cb0a0
}

// ---------------------------------------------------------------------------
// The drop

// SPanzersSquadUnit +0x138 EC_ChangeActiveDriver 0x59a7a0 (C5's class; the
// squad override is not lifted there yet, the member one 0x598070 is).
static void SquadChangeActiveDriver(SUnit* squad, int d)
{
    if (d < squad->Drivers.Size)
        squad->SetActiveDriver(d);                                // 0x5c0cb0
    for (int i = 0; i < squad->Members.Size; ++i)
        WorldUnit(squad->Members.Array[i].Unit)->EC_ChangeActiveDriver(d);   // +0x138
}

// PANZERS 0x5a1140 (SPanzersSquadUnit)
void ParachuteSquadDrop(SUnit* squad, float y)
{
    PZ_M3_TRACE("SPanzersSquadUnit 0x5a1140 (parachute drop)");
    squad->Invulnerable = true;                                   // +0x111
    SquadChangeActiveDriver(squad, 1);                            // +0x138(1)
    SUnit::EnvSlot9C(squad, 0);                                   // +0x9c(0) (0x5a0a10)
    squad->Invulnerable = true;
    squad->SetBehavior(4);                                        // +0x12c
    for (int i = 0; i < squad->Members.Size; ++i)
        static_cast<SPanzersSquadMemberUnit*>(WorldUnit(squad->Members.Array[i].Unit))->ParachuteDrop(y);   // 0x598cb0 (C5)
}

} // namespace pz
