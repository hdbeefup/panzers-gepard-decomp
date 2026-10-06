// src/game/m2common.h
// Shared definitions for the M2 game logic (docs/M2_INTERFACES.md): HD object
// sizes, the -m2 switch and the per-tick trace used by every M2 stub.
//
// SHARED HEADER (owner P0). Changes are additive and go through P0 or the
// integrator.

#ifndef PZ_M2COMMON_H
#define PZ_M2COMMON_H

#include "pz/pzcommon.h"

namespace pz {

// HD object sizes (operator new in the factories / creators of PANZERS.exe).
enum M2HdSize : unsigned {
    // Units (SP*Unit +0x10 CreateUnit factories 0x5a5a70..0x5a5df0).
    kHdSizeSSingleUnit             = 0x3a8,   // 0x5a5cf0
    kHdSizeSBuildingUnit           = 0x470,   // 0x5a5a70
    kHdSizeSPanzersSquadUnit       = 0x3a4,   // 0x5a5bf0
    kHdSizeSPanzersSquadMemberUnit = 0x354,   // 0x5a5b70
    kHdSizeSFlyingUnit             = 0x388,   // 0x5a5af0
    kHdSizeSProjectileUnit         = 0x36c,   // 0x5a5c70
    kHdSizeSTrainUnit              = 0x3b4,   // 0x5a5d70
    kHdSizeSWasterUnit             = 0x364,   // 0x5a5df0
    // Unit prototypes (SUnitRegistry::LoadUnitFiles 0x5d1050).
    kHdSizeSPSingleUnit            = 0x14c,   // ClassType 0, 4
    kHdSizeSPProjectileUnit        = 0x178,   // ClassType 3
    kHdSizeSPPanzersSquadUnit      = 0x14c,   // ClassType 5
    kHdSizeSPPanzersSquadMemberUnit = 0x140,  // ClassType 6
    kHdSizeSPWasterUnit            = 0x144,   // ClassType 7
    kHdSizeSPFlyingUnit            = 0x140,   // ClassType 8
    kHdSizeSPBuildingUnit          = 0x160,   // ClassType 9
    kHdSizeSPTrainUnit             = 0x14c,   // ClassType 10
    // Drivers (SP*Driver +0x10 CreateDriver factories 0x551080..0x551460).
    kHdSizeSTurnInPlaceDriver      = 0xe8,    // 0x551400
    kHdSizeSTurnInAngleDriver      = 0xe8,    // 0x5513a0
    kHdSizeSWalkerDriver           = 0xe8,    // 0x551460
    kHdSizeSPanzersSquadDriver     = 0xe8,    // 0x5511a0
    kHdSizeSPanzersSquadMemberDriver = 0xf4,  // 0x5511f0
    kHdSizeSSquadDriver            = 0xe8,    // 0x551290
    kHdSizeSSquadMemberDriver      = 0xec,    // 0x5512e0
    kHdSizeSFlyingDriver           = 0xf4,    // 0x5510d0
    kHdSizeSProjectileDriver       = 0xec,    // 0x551230
    kHdSizeSChildUnitDriver        = 0xe8,    // 0x551080
    kHdSizeSPanzersParachuteDriver = 0xe8,    // 0x551150
    kHdSizeSTrainDriver            = 0xe8,    // 0x551340
    // Unit animations (SP*Animation +0x10 factories).
    kHdSizeSVehicleAnimation       = 0xc0,    // 0x5c7a30
    kHdSizeSWalkerAnimation        = 0x34,    // 0x5c7ab0
    kHdSizeSSquadAnimation         = 0x2c,    // 0x5c7930
    kHdSizeSBuildingAnimation      = 0x30,    // 0x5c7770
    kHdSizeSProjectileAnimation    = 0x2c,    // 0x5c78e0
    kHdSizeSWasterAnimation        = 0x2c,    // 0x5c7b20
    kHdSizeSFlyingAnimation        = 0xb0,    // 0x5c77c0
    // Game logic and triggers.
    kHdSizeSTarget                 = 0x38,    // SUnit EC_* (new 0x38, ctor 0x5b27c0)
    kHdSizeSTrigger                = 0x2c,    // World+0x7474 array (TRIG 0x5f0140)
    kHdSizeSTriggerEvent           = 0x08,    // 0x5b2400
    kHdSizeSTriggerCondition       = 0x38,    // 0x5b22b0
    kHdSizeSTriggerAction          = 0x64,    // 0x5b2010
    kHdSizeSRunningTrigger         = 0x34,    // SGameLogic+0x268 array
};

// Runtime switch (src/game/m2trace.cpp). Recompile-only test hook.
//   Enabled: SGameLogic::Refresh runs the M2 path (triggers, units, drivers)
//            instead of the M1 model-only tick. Default on (M2-I); "-nom2"
//            or PZ_M2=0 turns it off, "-m2" or PZ_M2=1 turns it on.
//   Trace:   per-tick call trace of the M2 functions. "-m2" turns it on;
//            PZ_M2_TRACE=0/1 overrides it.
//   Crc:     log the HD world CRC (SGameLogic 0x56aa10) every tick as
//            "PZM2 CRC <frame> <crc> <seed> <units>" (determinism test,
//            docs/M2_INTERFACES.md). PZ_M2_CRC=1.
struct SM2Switches {
    bool Enabled;
    bool Trace;
    bool Crc;
};
extern SM2Switches g_M2;

// Reads the environment and removes "-m2" from argv. Call once, before
// SSettings parses the command line.
void M2ParseCommandLine(int* argc, char** argv);

// Per-tick trace. Tick = one SGameLogic::Refresh (20 Hz). A call site is
// logged on trace ticks only (ticks 0..2, then every 200th = every 10 s) and
// at most 4 times per tick.
extern unsigned g_M2Tick;
void M2TraceCall(STraceSite* site, const char* what);
void M2NextTick();

} // namespace pz

#define PZ_M2_TRACE(WHAT)                                             \
    do {                                                              \
        if (pz::g_M2.Trace) {                                         \
            static pz::STraceSite s_pzM2Site = { 0xffffffffu, 0 };    \
            pz::M2TraceCall(&s_pzM2Site, WHAT);                       \
        }                                                             \
    } while (0)

#endif // PZ_M2COMMON_H
