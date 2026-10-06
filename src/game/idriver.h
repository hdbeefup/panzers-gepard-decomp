// src/game/idriver.h
// SIDriver: the HD movement driver interface (RTTI SDriver vftable 0x7f49e0,
// 21 slots; 13 classes) and SIPDriver, the driver prototype (SPDriver
// vftable 0x7f4998, 5 slots; 13 classes, +0x10 is the driver factory).
//
// A unit owns its drivers (SUnit +0x38 SDArray, active index +0x28;
// SUnit::SetActiveDriver). Driver fields: +0x04 SPDriver*, +0x08 SUnit*
// owner, +0xc0/+0xc4 STarget*, +0xd0 turn-speed factor. Ghost frames:
// "the ghost runs ahead of the unit" (SDEQueue GhostFrames in the unit),
// Ghost_FirstStep / Ghost_NextStep, SWayPointWithManoeuvres 0x58d4e0..
//
// Menu classes (dtor hits): STurnInPlaceDriver (Sherman, M36, M7: 15 removed),
// STurnInAngleDriver (jeep, Bedford, M2A1: 16), SPanzersSquadDriver and
// SPanzersSquadMemberDriver (squads). The hero walks with SWalkerDriver
// code (0x550490..0x5594a0 ran) but is never deleted.
//
// HD sizes: STurnInPlaceDriver 0xe8 (factory 0x551400); others 0xe8/0xec/0xf4.
//
// SHARED HEADER (owner P0; slot names belong to agent P).
// Slot comments: "+0xNN HD 0xADDR (N arg dwords)" from the HD vftable and the
// RET imm16 of the implementation (a double counts as 2; "?" = no RET found,
// e.g. a tail jump). "[menu: ...]" = the coverage trace of the original menu
// (docs/re/M2_COVERAGE.md) executed that implementation: "startup" = only while
// the menu loaded, otherwise the steady-loop rate bucket. The hit status is per
// implementation address, so a slot shared by several classes shows the same
// status everywhere. Only hit slots have names; "(name guessed)" names rest on
// the decompiled body or one caller. Slot_XX keep the order (never remove one).
// Override tables: "*" = that override was executed in the menu.

#ifndef PZ_IDRIVER_H
#define PZ_IDRIVER_H

#include "m2common.h"

namespace pz {

struct SIUnit;
struct STarget;
struct SProperties;
struct SIPDriver;

// HD ghost frame (SGhostFrame, 0x74): layout in drivertypes.h (agent P).
//
struct SGhostFrame;
struct SUnitClassDesc;     // unit.h (M3-C5)

struct SIDriver {
    virtual ~SIDriver() {}                              // +0x00 HD 0x550120 (1 arg dwords) [menu: periodic<1/s via STurnInPlaceDriver 0x550420] scalar deleting dtor
    virtual SIPDriver* GetPDriver() = 0;                         // +0x04 HD 0x5531d0 (0 arg dwords) [menu: >=1000/s] returns +0x04 (the SPDriver prototype)
    virtual void SetTarget(STarget* target) = 0;                 // +0x08 HD 0x557bf0 (1 arg dwords) [menu: periodic<1/s] (name guessed) if target->Refresh(unit +0x74): restore +0xc4, assign +0xc0, reset the ghost
    virtual void SetTargetStopped(STarget* target) = 0;          // +0x0c HD 0x55c020 (1 arg dwords) (name guessed) Stop, SetTarget, then a stopped ghost keeping the global path
    virtual void Init() = 0;                                     // +0x10 HD 0x555a50 (0 arg dwords) [menu: periodic<1/s] (name guessed) sizes the arrays from SPDriver +0x24
    virtual void Refresh() = 0;                                  // +0x14 HD 0x559ab0 (0 arg dwords) [menu: >=20/s] per tick per moving unit (SChildUnitDriver 0x5598b0 and SProjectileDriver 0x55a740 override)
    virtual bool MoveTowardNextWayPoint(SGhostFrame* frame) = 0; // +0x18 HD 0x5582a0 (1 arg dwords) [menu: >=20/s] SDriver::MoveTowardNextWayPoint
    virtual float TurnToDir(float dir) = 0;                      // +0x1c HD 0x55c3f0 (1 arg dwords) (name guessed) returns the angle still to turn
    virtual float TurnToPoint(float x, float y, float z) = 0;    // +0x20 HD 0x55c530 (3 arg dwords) (name guessed) target +0x10..+0x18
    virtual bool GhostStepStraight(SGhostFrame* frame, float x, float z) = 0; // +0x24 HD 0x557b30 (3 arg dwords) [menu: >=20/s] (name guessed) advances a ghost frame along its direction (sin/cos 0x78d640/0x78d480), height 0x5e7730
    virtual bool GhostStepTowards(SGhostFrame* frame, float x, float z) = 0; // +0x28 HD 0x559360 (3 arg dwords) [menu: >=20/s] (name guessed) 
    virtual bool GhostTurnTowards(SGhostFrame* frame, float x, float z) = 0; // +0x2c HD 0x55c230 (3 arg dwords) [menu: >=20/s] (name guessed) 
    virtual bool FindGlobalPath(int size) = 0;                   // +0x30 HD 0x551cb0 (1 arg dwords) [menu: periodic<1/s] SDriver::FindGlobalPath (size -1 = unit +0x5c)
    virtual bool FindLocalPath(SGhostFrame* frame) = 0;          // +0x34 HD 0x552050 (1 arg dwords) [menu: periodic<1/s] (name guessed) pairs with +0x30 (STrainDriver/SFlyingDriver override both)
    virtual bool RefreshGhost() = 0;                             // +0x38 HD 0x555450 (0 arg dwords) [menu: >=20/s] (name guessed) per tick; true = the bottom frame is blocked
    virtual bool PredictGhost(SGhostFrame* frame, int* outUnit, float* outPos, float* outDir) = 0; // +0x3c HD 0x554870 (4 arg dwords) [menu: >=20/s] (name guessed) per tick; true = blocked
    virtual void Ghost_FirstStep() = 0;                          // +0x40 HD 0x553700 (0 arg dwords) [menu: periodic<1/s] SDriver::Ghost_FirstStep
    virtual void Ghost_NextStep() = 0;                           // +0x44 HD 0x553c00 (0 arg dwords) [menu: >=20/s] SDriver::Ghost_NextStep
    virtual float GetMaxSpeed() = 0;                             // +0x48 HD 0x553260 (0 arg dwords) [menu: >=20/s] (name guessed) min(unit +0x1b0(-1), GetMovementGroupMoveSpeed 0x56b010)
    virtual float GetTurnSpeed() = 0;                            // +0x4c HD 0x5533d0 (0 arg dwords) [menu: >=20/s] (name guessed) animation +0x1c(unit +0xe0) * +0xd0
    virtual void GetClassDescriptor(void** obj, const SUnitClassDesc** desc) = 0; // +0x50 HD 0x5531e0 (2 arg dwords) (M3-C5) the save / property class record, as SIUnit +0x1c
};

struct SIPDriver {
    virtual ~SIPDriver() {}                             // +0x00 HD 0x550180 (1 arg dwords) scalar deleting dtor
    virtual void Load(SProperties* props, const char* name, int nameLen) = 0; // +0x04 HD 0x555cc0 (3 arg dwords) [menu: startup via SPTurnInPlaceDriver 0x556800] (name guessed) startup (.unit "Driver" tree)
    virtual void LoadSubProperties(SProperties* props) = 0;      // +0x08 HD 0x557520 (1 arg dwords) [menu: sporadic] (name guessed) iterates a property array (0x6654a0)
    virtual void Slot_0C() = 0;                                  // +0x0c HD 0x55c820 (0 arg dwords)
    virtual SIDriver* CreateDriver(SIUnit* unit) = 0;            // +0x10 _purecall (? arg dwords) [menu: periodic<1/s via SPTurnInPlaceDriver 0x551400] factory: new 0xe8 STurnInPlaceDriver (0x551400) etc.
};

// Driver overrides of SDriver slots:
//   STurnInPlaceDriver         vftable 0x7f4bf0, 21 slots: +0x00 550420* +0x18 559340*
//   STurnInAngleDriver         vftable 0x7f4c48, 21 slots: +0x00 5503f0* +0x18 559330*
//   SWalkerDriver              vftable 0x7f4b40, 21 slots: +0x00 550450 +0x18 559350
//   SPanzersSquadDriver        vftable 0x7f4cf8, 21 slots: +0x00 5502d0* +0x18 5588e0* +0x1c 55c4d0
//                                                          +0x20 55c6a0 +0x48 5532e0* +0x4c 553420*
//   SPanzersSquadMemberDriver  vftable 0x7f4d50, 21 slots: +0x00 550300* +0x08 557da0*
//                                                          +0x14 55a640* +0x18 558a60*
//                                                          +0x48 553360* +0x50 553220
//   SSquadDriver               vftable 0x7f4a38, 21 slots: +0x00 550360 +0x18 559220
//   SSquadMemberDriver         vftable 0x7f4a90, 21 slots: +0x00 550390 +0x08 557e40
//   SFlyingDriver              vftable 0x7f4ae8, 21 slots: +0x00 550150 +0x18 5586a0 +0x30 551ef0
//                                                          +0x34 552410 +0x3c 554fb0 +0x50 553200
//   SProjectileDriver          vftable 0x7f4b98, 21 slots: +0x00 550330 +0x14 55a740 +0x50 553240
//   SChildUnitDriver           vftable 0x7f4ca0, 21 slots: +0x00 5500f0 +0x14 5598b0
//   SPanzersParachuteDriver    vftable 0x7f4da8, 21 slots: +0x00 5502a0 +0x08 557d00 +0x14 55a360
//   STrainDriver               vftable 0x7f4e00, 21 slots: +0x00 5503c0 +0x08 557ec0 +0x18 559230
//                                                          +0x30 551fa0 +0x34 552b50
// Prototype overrides of SPDriver slots:
//   SPTurnInPlaceDriver        vftable 0x7f49b0, 5 slots: +0x00 550240 +0x04 556800* +0x10 551400*
//   SPTurnInAngleDriver        vftable 0x7f49c8, 5 slots: +0x00 5501e0 +0x04 556680* +0x10 5513a0*
//   SPWalkerDriver             vftable 0x7fa904, 5 slots: +0x00 5a5930 +0x04 556970 +0x10 551460
//   SPPanzersSquadDriver       vftable 0x7fa94c, 5 slots: +0x00 5a55a0 +0x04 555fc0* +0x10 5511a0*
//   SPPanzersSquadMemberDriver vftable 0x7fa964, 5 slots: +0x00 5a55d0 +0x04 5560a0* +0x10 5511f0*
//   SPSquadDriver              vftable 0x7fa8bc, 5 slots: +0x00 5a5800 +0x04 5562c0 +0x10 551290
//   SPSquadMemberDriver        vftable 0x7fa8d4, 5 slots: +0x00 5a5830 +0x04 556410 +0x10 5512e0
//   SPFlyingDriver             vftable 0x7fa8ec, 5 slots: +0x00 5a5510 +0x04 555d30* +0x10 5510d0
//   SPProjectileDriver         vftable 0x7fa91c, 5 slots: +0x00 5a56e0 +0x04 556180* +0x10 551230
//   SPChildUnitDriver          vftable 0x7fa934, 5 slots: +0x00 5a54e0 +0x04 555ba0 +0x10 551080
//   SPPanzersParachuteDriver   vftable 0x7fa97c, 5 slots: +0x00 5a5570 +0x04 555ee0* +0x10 551150
//   SPTrainDriver              vftable 0x7fa994, 5 slots: +0x00 5a5890 +0x04 556560* +0x10 551340

} // namespace pz

#endif // PZ_IDRIVER_H
