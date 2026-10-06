// src/game/iunit.h
// SIUnit: the HD unit interface (RTTI SUnit vftable 0x7fcc24, 115 slots) and
// SIPanzersSquadUnit (SPanzersSquadUnit vftable 0x7f9e3c, 116 slots).
//
// HD units live in the SHeapTRB at World+0x4d4 (elements {int Next; SUnit*},
// Next == 0x7fffffff = live). SGameLogic::Refresh 0x576d80 calls, per tick and
// per live unit, +0x16c StoreInterpolationState, +0x2c ServerRefresh(frame),
// then +0x3c RefreshModel. UpdateUnitVisuals 0x5638f0 calls +0x40 per frame.
// Known SUnit fields (for U): +0x04 SPUnit*, +0x08/+0x0c/+0x10 SIModel*,
// +0x14 SUnitAnimation*, +0x28 active driver index, +0x38/+0x3c SDArray of
// SDriver*, +0x48/+0x4c SDArray of SGunner*, +0x54 unit size, +0x64 XP,
// +0x84 last ServerRefresh frame, +0x8c pos (x, y, z), +0x98 previous pos,
// +0xa4 pos before that, +0xb0 dir, +0xb4/+0xb8 previous dirs, +0xe0 state,
// +0xe8 behavior, +0xfc player, +0x108, +0x114 (HP?), +0x168 unplaced flag,
// +0x1dc, +0x1f4/+0x1f8 STarget* (refcounted, 0x38 bytes). The world CRC
// 0x56aa10 hashes +0xfc, +0x8c/+0x90/+0x94, +0xb0, +0x114, +0x108, +0x1dc.
//
// HD object sizes: see kHdSize* in m2common.h. The SUnit base size is not
// known (it is at most 0x354, SPanzersSquadMemberUnit).
//
// SHARED HEADER (owner P0; slot names belong to agent U, see
// docs/M2_INTERFACES.md). Same rules as the M1 interfaces.
// Slot comments: "+0xNN HD 0xADDR (N arg dwords)" from the HD vftable and the
// RET imm16 of the implementation (a double counts as 2; "?" = no RET found,
// e.g. a tail jump). "[menu: ...]" = the coverage trace of the original menu
// (docs/re/M2_COVERAGE.md) executed that implementation: "startup" = only while
// the menu loaded, otherwise the steady-loop rate bucket. The hit status is per
// implementation address, so a slot shared by several classes shows the same
// status everywhere. Only hit slots have names; "(name guessed)" names rest on
// the decompiled body or one caller. Slot_XX keep the order (never remove one).
// Override tables: "*" = that override was executed in the menu.

#ifndef PZ_IUNIT_H
#define PZ_IUNIT_H

#include "m2common.h"

namespace pz {

struct SIViewport;
struct SIModel;
struct SUnitDef;
struct STarget;
struct SIDriver;
struct SIUnitAnimation;

struct SIUnit {
    virtual ~SIUnit() {}                                // +0x00 HD 0x5b3730 (1 arg dwords) [menu: periodic<1/s via SSingleUnit 0x5aaa00] scalar deleting dtor
    virtual void Uninit() = 0;                                   // +0x04 HD 0x5b7e40 (0 arg dwords) [menu: periodic<1/s] (name guessed) once per removed unit (159 calls = SWorld::RemoveUnit count); stops the unit effects (pixie +0x34)
    virtual void Init(SUnitDef* def) = 0;                        // +0x08 HD 0x5ba8e0 (1 arg dwords) [menu: periodic<1/s] SSingleUnit::Init (0x5ad150); placed (UNDS) and CREATEd units
    virtual void InitNew(int player, const float* pos, float dir, int p4, float hp) = 0; // +0x0c HD 0x5bace0 (5 arg dwords) [menu: periodic<1/s] SSingleUnit::Init (0x5ad9f0), the second overload; squad members and crews (SWorld::CreateUnit 0x5e3170)
    virtual void Slot_10() = 0;                                  // +0x10 HD 0x5bc2d0 (2 arg dwords) symbol: SUnit::MakeDescription
    virtual void Slot_14() = 0;                                  // +0x14 HD 0x5bb1c0 (0 arg dwords)
    virtual void Slot_18() = 0;                                  // +0x18 HD 0x5baf30 (0 arg dwords)
    virtual void Slot_1C() = 0;                                  // +0x1c HD 0x5ba220 (2 arg dwords)
    virtual void Hook20(int p1) = 0;                             // +0x20 HD 0x54cd40 (1 arg dwords) [menu: periodic<1/s] (name guessed) empty in every menu class; 30 calls
    virtual void SetPosition(float x, float z, int dirBits, int yrelBits) = 0; // +0x24 HD 0x5c1980 (4 arg dwords) [menu: periodic<1/s] teleport: RunTriggers case 0x26 calls unit +0x24 (block map off, pos +0x8c/+0x94, dir +0xb0, +0x88, +0x3c, +0x16c, block map on)
    virtual void Slot_28() = 0;                                  // +0x28 HD 0x5be2b0 (0 arg dwords)
    virtual void ServerRefresh(int frame) = 0;                   // +0x2c HD 0x5bee90 (1 arg dwords) [menu: >=20/s] SUnit::ServerRefresh; per tick from SGameLogic::Refresh (returns at once if +0x84 == frame)
    virtual void Slot_30() = 0;                                  // +0x30 HD 0x5bd900 (0 arg dwords)
    virtual void RefreshTargeting() = 0;                         // +0x34 HD 0x5bd600 (0 arg dwords) [menu: >=1/s via SSingleUnit 0x5aef40] (name guessed) ~1/s per unit; weapon range (+0x17c) / sight (+0x184) checks
    virtual void RefreshMisc() = 0;                              // +0x38 HD 0x5bdee0 (0 arg dwords) [menu: >=20/s via SSingleUnit 0x5af890] SSingleUnit::RefreshMisc (0x5af890)
    virtual void RefreshModel() = 0;                             // +0x3c HD 0x5c6130 (0 arg dwords) [menu: >=20/s] per tick from Refresh: animation +0x08 UpdateModel, stored units
    virtual void UpdateVisuals(SIViewport* vp) = 0;              // +0x40 HD 0x5b76c0 (1 arg dwords) [menu: >=20/s via SSingleUnit 0x5aaaa0] (name guessed) per frame from UpdateUnitVisuals 0x5638f0: viewport +0x3c projection, CanSeeGroundUnit
    virtual void Slot_44() = 0;                                  // +0x44 HD 0x5bce20 (0 arg dwords)
    virtual void Slot_48() = 0;                                  // +0x48 HD 0x5bcb60 (0 arg dwords) symbol: SUnit::OnDriverReachedTarget
    virtual void Unplace() = 0;                                  // +0x4c HD 0x5ba850 (0 arg dwords) [menu: periodic<1/s] (name guessed) +0x168 (short) = 1; +0x50 clears it
    virtual void Place(float x, float z, float dir) = 0;         // +0x50 HD 0x5c5160 (3 arg dwords) [menu: periodic<1/s] (name guessed) +0x168 = 0, pos +0x8c/+0x94, dir +0xb0, then +0x3c and +0x16c; CREATE
    virtual void Slot_54() = 0;                                  // +0x54 HD 0x546ab0 (0 arg dwords)
    virtual void Slot_58() = 0;                                  // +0x58 HD 0x5468d0 (1 arg dwords)
    virtual bool StoreUnit(int unit, int mode) = 0;              // +0x5c HD 0x5c30d0 (2 arg dwords) [menu: periodic<1/s] SUnit::StoreUnit (crew into the jeep); false when the unit does not fit
    virtual void Slot_60() = 0;                                  // +0x60 HD 0x5468c0 (1 arg dwords)
    virtual void Slot_64() = 0;                                  // +0x64 HD 0x5c51d0 (1 arg dwords)
    virtual void Slot_68() = 0;                                  // +0x68 HD 0x5c6000 (0 arg dwords)
    virtual void Slot_6C() = 0;                                  // +0x6c HD 0x5c2390 (1 arg dwords)
    virtual void Remove(bool p1) = 0;                            // +0x70 HD 0x5c2df0 (1 arg dwords) [menu: periodic<1/s] (name guessed) once per removed unit
    virtual void Slot_74() = 0;                                  // +0x74 HD 0x5b5c10 (9 arg dwords) symbol: SUnit::GhostFrames_AddTop
    virtual void Slot_78() = 0;                                  // +0x78 HD 0x55ce70 (1 arg dwords)
    virtual void Slot_7C() = 0;                                  // +0x7c HD 0x55ce60 (0 arg dwords)
    virtual bool HasWoundedMember() = 0;                         // +0x80 HD 0x54a240 (0 arg dwords) [menu: >=1/s via SPanzersSquadUnit 0x59d670] (name guessed) squads: any member with +0x114 below the threshold
    virtual void Slot_84() = 0;                                  // +0x84 HD 0x5c0fa0 (1 arg dwords)
    virtual int GetRank() = 0;                                   // +0x88 HD 0x5b9e60 (0 arg dwords) [menu: >=20/s] (name guessed) XP (+0x64) against the SUnitRegistry thresholds +0x44..
    virtual void Slot_8C() = 0;                                  // +0x8c HD 0x55cee0 (3 arg dwords)
    virtual void Slot_90() = 0;                                  // +0x90 HD 0x5b6040 (2 arg dwords)
    virtual void Slot_94() = 0;                                  // +0x94 HD 0x5c4080 (7 arg dwords) symbol: SUnit::TakeDamage
    virtual void Slot_98() = 0;                                  // +0x98 HD 0x5c5050 (1 arg dwords)
    virtual void Slot_9C() = 0;                                  // +0x9c HD 0x5c1d40 (1 arg dwords)
    virtual void SetCurrentTarget(STarget* target, int p2) = 0;  // +0xa0 HD 0x5c0d10 (2 arg dwords) [menu: periodic<1/s] SPanzersSquadUnit::SetCurrentTarget (0x59f580); takes a reference into +0x1f4
    virtual void Slot_A4() = 0;                                  // +0xa4 HD 0x5b8ab0 (2 arg dwords) symbol: SSingleUnit::EC_Default
    virtual void Slot_A8() = 0;                                  // +0xa8 HD 0x5ba3e0 (1 arg dwords)
    virtual void EC_Move(int xBits, int zBits, int p3, bool p4, int p5) = 0; // +0xac HD 0x5b8ea0 (5 arg dwords) [menu: periodic<1/s] (name guessed) new STarget (0x38) type 2 at (x, z) (type 3 + p5 when p4) into +0x1f8, then +0xa0(target, p3)
    virtual void Slot_B0() = 0;                                  // +0xb0 HD 0x5b8d20 (5 arg dwords)
    virtual void EC_MoveAlongPath(int path, int p2, int p3) = 0; // +0xb4 HD 0x5b8e20 (3 arg dwords) [menu: periodic<1/s] (name guessed) new STarget type 2 (+0x30 path, +0x34), STarget::ConsumePath; convoy
    virtual void EC_Follow(int unit, int p2) = 0;                // +0xb8 HD 0x5b8ba0 (2 arg dwords) [menu: periodic<1/s] SUnit::EC_Follow
    virtual void Slot_BC() = 0;                                  // +0xbc HD 0x5b9370 (1 arg dwords)
    virtual void Stop() = 0;                                     // +0xc0 HD 0x5b91b0 (0 arg dwords) [menu: periodic<1/s] (name guessed) drops +0x1f4, active driver 0x55bf50(1), STarget at own position
    virtual void ClearTargets() = 0;                             // +0xc4 HD 0x5b90b0 (? arg dwords) [menu: periodic<1/s] (name guessed) releases the STargets at +0x1f4/+0x1f8
    virtual void Slot_C8() = 0;                                  // +0xc8 HD 0x5b9170 (? arg dwords)
    virtual void Slot_CC() = 0;                                  // +0xcc HD 0x5b8660 (3 arg dwords) symbol: SUnit::EC_AttackMove
    virtual void Slot_D0() = 0;                                  // +0xd0 HD 0x5b8740 (3 arg dwords)
    virtual void Slot_D4() = 0;                                  // +0xd4 HD 0x5b82b0 (2 arg dwords) symbol: SUnit::EC_AssaultBuilding
    virtual void Slot_D8() = 0;                                  // +0xd8 HD 0x5b8fa0 (1 arg dwords)
    virtual void Slot_DC() = 0;                                  // +0xdc HD 0x5b8c90 (1 arg dwords)
    virtual void Slot_E0() = 0;                                  // +0xe0 HD 0x547b90 (2 arg dwords)
    virtual void Slot_E4() = 0;                                  // +0xe4 HD 0x5b8570 (3 arg dwords)
    virtual void Slot_E8() = 0;                                  // +0xe8 HD 0x5b8440 (2 arg dwords) symbol: SUnit::EC_Attack
    virtual void StopGunners() = 0;                              // +0xec HD 0x5b9110 (0 arg dwords) [menu: periodic<1/s] (name guessed) every gunner (+0x48, count +0x4c) slot +0x28
    virtual void Slot_F0() = 0;                                  // +0xf0 HD 0x5478c0 (2 arg dwords)
    virtual void Slot_F4() = 0;                                  // +0xf4 HD 0x5478e0 (2 arg dwords)
    virtual void Slot_F8() = 0;                                  // +0xf8 HD 0x5478d0 (2 arg dwords)
    virtual void Slot_FC() = 0;                                  // +0xfc HD 0x547b30 (3 arg dwords)
    virtual void Slot_100() = 0;                                 // +0x100 HD 0x548180 (3 arg dwords)
    virtual void Slot_104() = 0;                                 // +0x104 HD 0x547e80 (0 arg dwords)
    virtual void Slot_108() = 0;                                 // +0x108 HD 0x5478f0 (0 arg dwords)
    virtual void Slot_10C() = 0;                                 // +0x10c HD 0x547ba0 (3 arg dwords)
    virtual void Slot_110() = 0;                                 // +0x110 HD 0x547b80 (2 arg dwords)
    virtual void Slot_114() = 0;                                 // +0x114 HD 0x547b40 (2 arg dwords)
    virtual void Slot_118() = 0;                                 // +0x118 HD 0x547b50 (1 arg dwords)
    virtual void Slot_11C() = 0;                                 // +0x11c HD 0x547b60 (0 arg dwords)
    virtual void Slot_120() = 0;                                 // +0x120 HD 0x547b70 (1 arg dwords)
    virtual void Slot_124() = 0;                                 // +0x124 HD 0x5b8b10 (0 arg dwords) symbol: SUnit::EC_Die
    virtual void Slot_128() = 0;                                 // +0x128 HD 0x5b88d0 (0 arg dwords)
    virtual void SetBehavior(int behavior) = 0;                  // +0x12c HD 0x5b8960 (1 arg dwords) [menu: periodic<1/s] (name guessed) +0xe8 = behavior
    virtual void Slot_130() = 0;                                 // +0x130 HD 0x54cd50 (1 arg dwords)
    virtual void Slot_134() = 0;                                 // +0x134 HD 0x55cdf0 (1 arg dwords)
    virtual void Slot_138() = 0;                                 // +0x138 HD 0x547900 (1 arg dwords)
    virtual void Slot_13C() = 0;                                 // +0x13c HD 0x5b8920 (1 arg dwords)
    virtual void Slot_140() = 0;                                 // +0x140 HD 0x5b87f0 (2 arg dwords)
    virtual void Slot_144() = 0;                                 // +0x144 HD 0x5b93f0 (1 arg dwords)
    virtual void Slot_148() = 0;                                 // +0x148 HD 0x547e70 (1 arg dwords)
    virtual void Slot_14C() = 0;                                 // +0x14c HD 0x5481a0 (1 arg dwords)
    virtual void Slot_150() = 0;                                 // +0x150 HD 0x548190 (1 arg dwords)
    virtual void Slot_154() = 0;                                 // +0x154 HD 0x5481b0 (0 arg dwords)
    virtual void Slot_158() = 0;                                 // +0x158 HD 0x55ce50 (1 arg dwords)
    virtual void Slot_15C() = 0;                                 // +0x15c HD 0x547e60 (1 arg dwords)
    virtual void Slot_160() = 0;                                 // +0x160 HD 0x5478b0 (1 arg dwords)
    virtual void Slot_164() = 0;                                 // +0x164 HD 0x55ce40 (1 arg dwords)
    virtual void Slot_168() = 0;                                 // +0x168 HD 0x5b9360 (1 arg dwords)
    virtual void StoreInterpolationState() = 0;                  // +0x16c HD 0x5b5ad0 (0 arg dwords) [menu: >=20/s] pos +0x8c -> +0x98 -> +0xa4, dir +0xb0 -> +0xb4 -> +0xb8; models +0x08/+0x0c/+0x10 +0x3c; per tick
    virtual void Slot_170() = 0;                                 // +0x170 HD 0x5b9e30 (0 arg dwords)
    virtual void Slot_174() = 0;                                 // +0x174 HD 0x5b9ec0 (0 arg dwords)
    virtual void Slot_178() = 0;                                 // +0x178 HD 0x5b9d90 (0 arg dwords)
    virtual float GetMaxRange(int weapon) = 0;                   // +0x17c HD 0x5b9f60 (1 arg dwords) [menu: >=1/s via SSingleUnit 0x5acbd0] (name guessed) gunner +0x2c ... +0x38
    virtual float GetMinRange(int weapon) = 0;                   // +0x180 HD 0x5b9f90 (1 arg dwords) [menu: >=1/s via SSingleUnit 0x5acd30] (name guessed) gunner +0x2c ... +0x34
    virtual float GetSightRange() = 0;                           // +0x184 HD 0x5ba240 (0 arg dwords) [menu: >=20/s] (name guessed) PUnit +0x84, registry +0x10c for type 6
    virtual float GetExtra188() = 0;                             // +0x188 HD 0x548230 (0 arg dwords) [menu: >=1/s] (name guessed) 0.0 except squads (0x59bb80)
    virtual void Slot_18C() = 0;                                 // +0x18c HD 0x548380 (0 arg dwords)
    virtual void AI_Heartbeat() = 0;                             // +0x190 HD 0x5b37d0 (0 arg dwords) [menu: >=1/s] SPanzersSquadMemberUnit::AI_Heartbeat (0x5979d0)
    virtual void Slot_194() = 0;                                 // +0x194 HD 0x55e330 (1 arg dwords)
    virtual void SetOnBlockMap(bool on) = 0;                     // +0x198 HD 0x5bc660 (1 arg dwords) [menu: >=20/s] (name guessed) +0x2ea = on, 0x5f4430(x, z, size, on)
    virtual void Slot_19C() = 0;                                 // +0x19c HD 0x5b74c0 (0 arg dwords)
    virtual void MarkBlockMap(bool on, int p2, int p3, int p4, short p5) = 0; // +0x1a0 HD 0x5bc6d0 (5 arg dwords) [menu: >=1/s] (name guessed) 0x5f4720
    virtual int TestBlockMapPath(int p1, int p2, int p3, int p4, short p5, int p6, int p7) = 0; // +0x1a4 HD 0x5b7520 (7 arg dwords) [menu: >=20/s] (name guessed) 0x5d9eb0
    virtual int TestBlockMap(int p1, int p2, int p3, int p4, short p5) = 0; // +0x1a8 HD 0x5b7500 (5 arg dwords) [menu: >=20/s] (name guessed) 0x5d9e00
    virtual void RestoreBehavior() = 0;                          // +0x1ac HD 0x546ac0 (0 arg dwords) [menu: sporadic via SPanzersSquadUnit 0x599430] (name guessed) squads: +0x12c(+0xec) when +0xec != +0xe0
    virtual float GetMoveSpeed(int p1) = 0;                      // +0x1b0 HD 0x5b9ed0 (1 arg dwords) [menu: >=20/s] (name guessed) driver +0x48 calls it with -1
    virtual void Slot_1B4() = 0;                                 // +0x1b4 HD 0x55cf70 (1 arg dwords)
    virtual void Slot_1B8() = 0;                                 // +0x1b8 HD 0x5c1da0 (2 arg dwords)
    virtual void Slot_1BC() = 0;                                 // +0x1bc HD 0x55cf60 (0 arg dwords)
    virtual void ServerRefreshMedic(float dt) = 0;               // +0x1c0 HD 0x5bfa50 (1 arg dwords) [menu: >=20/s] SUnit::ServerRefreshMedic
    virtual void SetUnitSize() = 0;                              // +0x1c4 HD 0x5c21d0 (0 arg dwords) [menu: periodic<1/s via SPanzersSquadUnit 0x5a0f30] SPanzersSquadUnit::SetUnitSize (0x5a0f30)
    virtual void GetCenterPosition(float* out) = 0;              // +0x1c8 HD 0x5b9d40 (1 arg dwords) [menu: >=1000/s via SPanzersSquadUnit 0x59b4a0] (name guessed) squads: mean member position; others +0x8c
};

// SPanzersSquadUnit appends one slot.
struct SIPanzersSquadUnit : SIUnit {
    virtual void RefreshSquadFormation() = 0;                    // +0x1cc HD 0x59dda0 (0 arg dwords) [menu: >=1/s] (name guessed) SPanzersSquadUnit only (0x59dda0), ~9/s
};

// Subclass overrides of SUnit slots (offset, HD address):
//   SSingleUnit                vftable 0x7fb2a4, 115 slots: +0x00 5aaa00* +0x04 5abde0*
//                                                           +0x08 5ad150* +0x0c 5ad9f0*
//                                                           +0x14 5ae560 +0x18 5ae370 +0x1c 5ace70
//                                                           +0x30 5af2c0 +0x34 5aef40*
//                                                           +0x38 5af890* +0x40 5aaaa0*
//                                                           +0x48 5aed30 +0x4c 5ad050 +0x50 5b0950*
//                                                           +0x54 5aaa80 +0x58 5aaa30 +0x80 5aec70
//                                                           +0x84 5b08b0 +0x88 5acb30 +0x8c 5ad070
//                                                           +0xa4 5ac1a0 +0xa8 5ace90 +0xac 5ac4b0*
//                                                           +0x114 5ac290 +0x118 5ac360
//                                                           +0x11c 5ac420 +0x120 5ac460
//                                                           +0x134 5ac180 +0x13c 5ac0d0
//                                                           +0x150 5ac7d0 +0x154 5aca70
//                                                           +0x15c 5ac650 +0x160 5abf50
//                                                           +0x178 5aca80 +0x17c 5acbd0*
//                                                           +0x180 5acd30* +0x194 5b0420
//                                                           +0x198 5ae950* +0x1b4 5ae970
//   SBuildingUnit              vftable 0x7f4184, 115 slots: +0x00 5464d0 +0x04 547690 +0x08 548f20*
//                                                           +0x0c 549200 +0x14 549500 +0x1c 548b20
//                                                           +0x24 54cf40 +0x28 54c8b0 +0x30 54afc0
//                                                           +0x34 54a250* +0x38 54b910*
//                                                           +0x40 546d20* +0x5c 54d380 +0x64 54ded0
//                                                           +0x68 54e480 +0x78 548200 +0x7c 5481f0
//                                                           +0x8c 548d30 +0x94 54de30 +0xa0 54ca80
//                                                           +0xa4 547a90 +0xa8 548c10 +0xec 547e90
//                                                           +0x134 547a70 +0x13c 547910
//                                                           +0x158 547f60 +0x164 547bb0
//                                                           +0x16c 5468a0* +0x17c 548240*
//                                                           +0x180 5482e0 +0x184 548b40
//                                                           +0x190 546500* +0x194 54c960
//                                                           +0x198 549b10* +0x1a0 549b20*
//                                                           +0x1a4 546ae0 +0x1a8 546ad0*
//                                                           +0x1b4 549b70 +0x1bc 549af0
//                                                           +0x1c8 5481c0
//   SPanzersSquadUnit          vftable 0x7f9e3c, 116 slots: +0x00 599230* +0x04 599c70*
//                                                           +0x08 59c470* +0x0c 59ca40 +0x14 59cf80
//                                                           +0x1c 59bfd0 +0x20 59fab0*
//                                                           +0x24 59fe30* +0x30 59dfe0
//                                                           +0x34 59db00* +0x38 59e0d0*
//                                                           +0x40 599620* +0x48 59d720
//                                                           +0x4c 59c1e0* +0x50 5a1220 +0x60 599260
//                                                           +0x80 59d670* +0x8c 59c2a0 +0x9c 5a0a10
//                                                           +0xa0 59f580* +0xa4 59a960 +0xa8 59c0d0
//                                                           +0xac 59af20* +0xb0 59aea0 +0xb4 59aef0
//                                                           +0xb8 59ab90 +0xcc 59a3f0 +0xd0 59a460
//                                                           +0xd4 599d00 +0xe0 59acd0 +0xe8 599d60
//                                                           +0xec 59b120* +0xf0 599f80 +0xf4 59a280
//                                                           +0xf8 59a0f0 +0xfc 59aa90 +0x100 59b1d0
//                                                           +0x104 59b110 +0x108 59a500
//                                                           +0x110 59abf0 +0x124 59aa20
//                                                           +0x128 59a730 +0x12c 59a910*
//                                                           +0x130 59fd00 +0x138 59a7a0
//                                                           +0x13c 59a860 +0x140 59a4a0
//                                                           +0x148 59af90 +0x170 59ba90
//                                                           +0x178 59b650 +0x17c 59bd10*
//                                                           +0x180 59be20* +0x188 59bb80*
//                                                           +0x18c 59bf30 +0x194 59ef00
//                                                           +0x198 59d460* +0x19c 599510
//                                                           +0x1a0 59d4b0* +0x1a4 599580*
//                                                           +0x1a8 599550* +0x1ac 599430*
//                                                           +0x1b0 59bc10* +0x1b4 59d4e0
//                                                           +0x1b8 5a0de0 +0x1c0 59ede0*
//                                                           +0x1c4 5a0f30* +0x1c8 59b4a0*
//                                                           +0x1cc 59dda0
//   SPanzersSquadMemberUnit    vftable 0x7f9a40, 115 slots: +0x00 597940* +0x08 598910
//                                                           +0x0c 598940* +0x14 598980 +0x1c 5988f0
//                                                           +0x30 598a20 +0x34 5989d0*
//                                                           +0x38 598ab0* +0x40 597a10*
//                                                           +0x88 598770* +0xac 598570*
//                                                           +0xe4 597fa0 +0xe8 597e80 +0xec 5986e0*
//                                                           +0x104 598670 +0x114 598350
//                                                           +0x118 598420 +0x11c 5984e0
//                                                           +0x120 598520 +0x124 598280
//                                                           +0x12c 5980b0* +0x134 598250
//                                                           +0x138 598070 +0x170 598760
//                                                           +0x174 5987c0 +0x190 5979d0
//                                                           +0x198 5989b0* +0x19c 5979f0
//                                                           +0x1a0 5989c0 +0x1a8 597a00
//                                                           +0x1b0 5988d0
//   SFlyingUnit                vftable 0x7f5b74, 115 slots: +0x00 55cd10 +0x08 55cef0 +0x0c 55cf20
//                                                           +0x1c 55cec0 +0x30 55cfc0 +0x34 55cf90
//                                                           +0x38 55d0c0 +0x48 55cf80 +0xac 55ce00
//   SProjectileUnit            vftable 0x7fa5b8, 115 slots: +0x00 5a38f0 +0x0c 5a3950 +0x1c 5a3930
//                                                           +0x30 5a39b0 +0x34 5a3990 +0x38 5a39f0
//                                                           +0x124 5a3920
//   STrainUnit                 vftable 0x7fb780, 115 slots: +0x00 5b0ed0 +0x04 5b15e0 +0x08 5b1650
//                                                           +0x0c 5b1680 +0x14 5b16d0 +0x18 5ae370
//                                                           +0x1c 5b1630 +0x24 5b1980 +0x30 5af2c0
//                                                           +0x34 5aef40* +0x38 5af890*
//                                                           +0x40 5aaaa0* +0x48 5aed30 +0x4c 5ad050
//                                                           +0x50 5b0950* +0x54 5aaa80 +0x58 5aaa30
//                                                           +0x74 5b0f00 +0x80 5aec70 +0x84 5b08b0
//                                                           +0x88 5acb30 +0x8c 5ad070 +0xa4 5ac1a0
//                                                           +0xa8 5ace90 +0xac 5ac4b0* +0xb8 5b1620
//                                                           +0x114 5ac290 +0x118 5ac360
//                                                           +0x11c 5ac420 +0x120 5ac460
//                                                           +0x134 5ac180 +0x13c 5ac0d0
//                                                           +0x150 5ac7d0 +0x154 5aca70
//                                                           +0x15c 5ac650 +0x160 5abf50
//                                                           +0x178 5aca80 +0x17c 5acbd0*
//                                                           +0x180 5acd30* +0x194 5b0420
//                                                           +0x198 5b16f0 +0x19c 5b1400
//                                                           +0x1a0 5b17c0 +0x1a8 5b1420
//                                                           +0x1b4 5ae970 +0x1c4 5b1b70
//   SWasterUnit                vftable 0x7ffc74, 115 slots: +0x00 5d1db0 +0x04 5d2070 +0x08 5d22b0
//                                                           +0x0c 5d2330 +0x14 5d23d0 +0x1c 5d2290
//                                                           +0x30 5d2460 +0x34 5d2440 +0x38 5d24a0
//                                                           +0x40 5d1e30 +0xe4 5d21a0 +0xe8 5d2090
//                                                           +0x124 5d2280

} // namespace pz

#endif // PZ_IUNIT_H
