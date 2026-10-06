// src/game/gamelogic.h
// pz::SGameLogic: HD SGameLogic (0x318 bytes, no vtable; 0x55e440..0x582a00).
// In HD, SSuperWindow+0x1a0 holds it and DAT_008f2078 (pz::g_GameLogic)
// points to it. Moved here from src/world in M2-P0.
//
// SHARED HEADER (owner P0). Agent L implements the members (gamelogic.cpp,
// triggers.cpp, movementgroup.cpp) and names the fields it decodes, in place,
// at their HD offsets. Never grow the class: PZ_HD_SIZE keeps it at 0x318.
//
// Per-tick order of Refresh 0x576d80 (single-player path, executed in the
// original menu; see docs/re/M2_COVERAGE.md): FPU control word check,
// SWorld::UpdateSpeech 0x607f50, 0x578b00, 0x578a70, then per frame
// ProcessPacket 0x5737c0 / BeginFrame 0x571840 (world CRC 0x56aa10) /
// 0x579390, SWorld refresh 0x604620, 0x57dfe0, SWorld 0x6088f0 (weather),
// World+0x4ec = frame, DispatchEverySecond 0x570cc0 when frame % 20 == 0,
// RunTriggers 0x579ab0, 0x568af0, 0x565e10, per unit +0x16c / +0x2c(frame),
// scene +0xf0(2), SWorld::RefreshFlyingFox 0x5f6bf0, per unit +0x3c,
// 0x5822a0. Not executed in the menu: SMulti sync 0x5212a0, AI 0x5f5c70,
// 0x605930, 0x6090e0, the "%s has left/lost the game" branches.

#ifndef PZ_GAMELOGIC_H
#define PZ_GAMELOGIC_H

#include <stddef.h>
#include "pz/pzcommon.h"
#include "m2common.h"

namespace pz {

struct SIViewport;
struct STrigger;
struct SIUnit;

struct SGameLogic {
    SGameLogic(int p1, int p2, int p3);   // 0x55e440 (menu: 0, -1, 0); sets g_GameLogic
    ~SGameLogic();                        // 0x55fe00 (non-virtual; caller deletes 0x318)

    void SetRunning(int running);         // 0x5802f0 (+0x04 = running; Concert +0x64 resume)
    int  Refresh();                       // 0x576d80, 20 Hz fixed step (M1 path, or RefreshM2 with -m2)
    void UpdateUnitVisuals(SIViewport* vp, double interpolation);   // 0x5638f0 (per frame)

    // --- M2 (agent L). Steady-loop functions of the original menu.
    void RefreshM2();                     // the 0x576d80 single-player path (recompile split)
    void Tick_578b00();                   // per tick, before the frame loop
    void Tick_578a70();                   // per tick
    void ProcessPacket(int frame, int p2);// 0x5737c0 per frame (SMulti absent: local frame)
    void BeginFrame();                    // 0x571840 "Frame %d started with server CRC" (log level 1, MP only)
    unsigned ComputeWorldCRC();           // 0x56aa10 rotl-xor over the live units (see iunit.h) ^ World+0x7518
    void Tick_579390();                   // per frame
    void Tick_57dfe0();                   // per tick
    void Tick_568af0();                   // per tick
    void Tick_565e10(int p1);             // per tick, argument from table 0x7f6220[frame % 12]
    void Tick_5822a0();                   // per tick, after the unit loop
    int  GetFrame();                      // 0x56d1a0 (+0x08)
    bool IsPaused();                      // 0x56e150 (+0x288 || +0x2b8) (name guessed)
    bool CanSeeGroundUnit(int player, SIUnit* unit);   // 0x562760

    // Triggers (triggers.cpp).
    void DispatchEverySecond();           // 0x570cc0 event 0
    void DispatchEnterLocation(int unit, int location);   // 0x571280 event 2
    void DispatchLeaveLocation(int unit, int location);   // 0x571470 event 3
    void Dispatch_571380(int p1, int p2); // 0x571380 (an event dispatcher; crew enters the jeep?)
    void UpdateActiveLocations(int p1, int p2);           // 0x582080
    bool CheckConditions(STrigger* trigger);              // 0x580600 (15 condition types)
    void StartRunningTrigger(int p1);     // 0x579510 (name guessed)
    void RunTriggers();                   // 0x579ab0 (76 action cases; menu: 1, 2, 3, 8, 0xa, 0x1e, 0x1f, 0x26, 0x29)

    // Orders and movement groups (movementgroup.cpp).
    void GroupOrder(int p1, int p2, int p3, int p4, int p5, int p6);          // 0x56ff30
    void MoveFoundUnitsToLocation(int p1, int p2, int p3, int p4, int p5, int p6);   // 0x57efd0
    void ConvoyAlongPath(int p1, int p2); // 0x57e600
    void SendConvoyMovementGroupFollowers(int group);     // 0x57e6f0
    int  GetMovementGroupConvoy(int group);               // 0x56af10
    float GetMovementGroupMoveSpeed(SIUnit* unit);        // 0x56b010
    int  GetMovementGroupSquadsGlobalState(int group);    // 0x56b090
    void GetMovementGroupUnitFormationPos(int unit, float* out);   // 0x56b240
    void SetMovementGroupBiggestUnit(int group);          // 0x57faf0
    void SetMovementGroupBossUnit(int group, int p2, int p3, int p4);   // 0x57fcb0
    void SetMovementGroupFormationDir(int group, int p2); // 0x57ff40
    void SetMovementGroupFormationDir2(int group, int p2);// 0x57ffc0 (same symbol in HD)
    void RefreshMovementGroup(int group, int p2);         // 0x5800d0
    void UpdateMovementGroupSlowestMoveSpeed(int group);  // 0x5824b0

    // Fields (HD offsets). Decoded so far:
    int           Mode;              // +0x000 (non-zero: multiplayer/replay branches in Refresh)
    int           Running;           // +0x004 SetRunning
    int           Frame;             // +0x008 logic tick (GetFrame 0x56d1a0)
    unsigned char _00c[0x020 - 0x00c];
    void*         FrameObject;       // +0x020 per-tick object (new 0x30) of BeginFrame
    unsigned char _024[0x02c - 0x024];
    unsigned*     CrcHistory;        // +0x02c SDEQueue of world CRCs (+0x34 size, +0x38, +0x3c/+0x40 bounds)
    unsigned char _030[0x268 - 0x030];
    void*         RunningTriggers;   // +0x268 SDArray<SRunningTrigger> (0x34 each)
    unsigned char _26c[0x288 - 0x26c];
    bool          Flag288;           // +0x288 (IsPaused)
    unsigned char _289[0x2b8 - 0x289];
    bool          Flag2b8;           // +0x2b8 (IsPaused)
    unsigned char _2b9[0x318 - 0x2b9];
};
PZ_HD_SIZE(SGameLogic, kHdSizeSGameLogic);
#if defined(_M_IX86)
static_assert(offsetof(SGameLogic, Frame) == 0x008, "Refresh param_1[2]");
static_assert(offsetof(SGameLogic, FrameObject) == 0x020, "0x571840 +0x20");
static_assert(offsetof(SGameLogic, CrcHistory) == 0x02c, "0x571840 +0x2c");
static_assert(offsetof(SGameLogic, RunningTriggers) == 0x268, "RunTriggers +0x268");
static_assert(offsetof(SGameLogic, Flag2b8) == 0x2b8, "0x56e150 +0x2b8");
#endif

} // namespace pz

#endif // PZ_GAMELOGIC_H
