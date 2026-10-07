// src/game/packets.h
// The lockstep packets: every player order (UI, AI support calls, the
// network) becomes a packet record in the per-frame SStreamBuffer at
// SGameLogic+0x20 (FrameObject). SGameLogic::ProcessPacket 0x5737c0 (the
// switch is at 0x5739b8) applies the records of a frame when that frame is
// executed; -packetrec / -packetplay record and replay exactly these frames
// (docs/M3_REPLAY.md).
//
// SHARED HEADER (owner P0; bodies: agent O in packets.cpp / packets_rec.cpp;
// callers: agent O (SGameView orders 0x61e740, selection 0x5fc050..), agent H
// (HUD buttons, minimap)). Names marked "(guessed)" rest on the handler only.
// The .rec file format and the per-op field table: packets_rec.h.
//
// Frame stream layout (SStream helpers, HD addresses):
//   WriteByte 0x65daf0 / ReadByte 0x65d300, WriteWord 0x65dd00 / ReadWord 0x65d770,
//   WriteInt 0x65dc40 / ReadInt 0x65d4d0, WriteFloat 0x65dc20 / ReadFloat 0x65d4b0.
//   int   crc        the world CRC of the frame (BeginFrame 0x571840 pushes it
//                    into SGameLogic CrcHistory); ProcessPacket compares it with
//                    CrcHistory[bottom] and logs "Inconsistency in frame %d with
//                    player %d." when it differs.
//   repeat: u8 op (0 ends the frame), then the record below.
// Record of a builder with "units": its arguments, then the selected units
//   (0x576130 WriteSelectedUnits: u16 unit-heap index of every live unit with
//   unit +0x104 bit 0 set, in heap order, then u16 0xffff). ProcessPacket
//   reads them back with 0x56d540 into an SFoundUnits group.
// The two leading u8 of most records are bools: B1 (MoveFoundUnitsToLocation
//   p4, meaning not pinned down) and B2 (its "queue" flag = Shift).
//
// Builders: SGameLogic methods (thiscall, this = SGameLogic, RET n = args),
//   0x575610..0x576430. Each writes its record into FrameObject, then calls
//   SGameLogic::CheckSendQSize 0x562fb0 (multiplayer size warning only).
//   The recompile declares them as free functions taking the logic object.
//
// Recording (-packetrec, SGameLogic +0x1b0 stream): file = replay header
// (SPanzersCampaign::WriteReplayHeader 0x596fc0: 'SAVE' 'v4pa' 3, map,
// mode, race, prestige, army, ... 12 player records) then per frame and
// player: int size, int SGameLogic+0x04 (game speed: 0 paused, 1, 2), the
// frame stream, 20 bytes from 0x5e6a70 (5 dwords of SWorld +0x38/+0x40/+0x44/
// +0x50/+0x54, (guessed) the camera). Playback (+0x1b4) replaces the local
// frame with the recorded one, restores +0x04 and the 20 bytes.

#ifndef PZ_PACKETS_H
#define PZ_PACKETS_H

#include "m3common.h"

struct SStream;

namespace pz {

struct SGameLogic;
struct SUnit;

// Opcodes. "units" = the selected units follow the arguments.
// cmd = the unit command code passed to the handler (SUnit order queue).
enum PzPacketOp : unsigned char {
    PZ_PKT_END              = 0x00,
    PZ_PKT_TARGET_UNIT      = 0x01, // 0x575a60 (B1,B2,i32 unit) units; 0x5bcad0 + 0x564720 cmd 0 (attack unit, guessed)
    PZ_PKT_MOVE             = 0x02, // 0x575ef0 (B1,B2,f32 x,f32 z) units; 0x57e8b0 (near) / 0x57efd0 MoveFoundUnitsToLocation cmd 1
    PZ_PKT_MOVE_DIR         = 0x03, // 0x575f50 (B1,B2,x,z,f32 dir) units; 0x57f200 cmd 2
    PZ_PKT_MOVE_BACK        = 0x04, // 0x575e10 (B1,B2,x,z) units; 0x57efd0 cmd 3 (guessed: move backward)
    PZ_PKT_MOVE_BACK_DIR    = 0x05, // 0x575e70 (B1,B2,x,z,dir) units; 0x57f200 cmd 4
    PZ_PKT_06               = 0x06, // 0x5763a0 (B,f32) units; 0x564910 cmd 5
    PZ_PKT_07               = 0x07, // 0x575b40 (B1,B2,i32) units; 0x564720 cmd 7
    PZ_PKT_08               = 0x08, // 0x576230 (B) units; 0x564440 cmd 8 (guessed: stop)
    PZ_PKT_09               = 0x09, // 0x575770 (B1,B2,x,z) units; 0x564660 cmd 9 (guessed: attack ground)
    PZ_PKT_0A               = 0x0a, // 0x575d60 (B1,B2,i32) units; 0x564720 cmd 0xc
    PZ_PKT_0B               = 0x0b, // 0x5754d0 (B,i32) units; 0x564720 cmd 0xe
    PZ_PKT_0C               = 0x0c, // 0x576260 (B,i32) units; cmd 0xd
    PZ_PKT_0D               = 0x0d, // 0x5760c0 (B,i32) units; cmd 0xf
    PZ_PKT_0E               = 0x0e, // 0x575610 (B1,B2,x,z) units; 0x564660 cmd 0x10, 0x5bca90
    PZ_PKT_0F               = 0x0f, // 0x575510 (B1,B2,i32) units; 0x564720 cmd 0x11, 0x5bc9d0
    PZ_PKT_10               = 0x10, // 0x575560 (B1,B2,i32) units; 0x564720 cmd 0x12
    PZ_PKT_11               = 0x11, // (no builder) 0x564660 cmd 0x13
    PZ_PKT_12               = 0x12, // 0x5756c0 (B1,B2,i32) units; 0x564720 cmd 0x14
    PZ_PKT_13               = 0x13, // 0x575710 (B1,B2,x,z) units; 0x564660 cmd 0x15
    // 0x14: no case (default: ignored)
    PZ_PKT_15               = 0x15, // 0x575670 (B1,B2,i32) units; 0x564720 cmd 0x16
    PZ_PKT_16               = 0x16, // 0x575ae0 (B1,B2,x,z) units; 0x5bcab0 + 0x57efd0 cmd 0x17
    PZ_PKT_17               = 0x17, // 0x576300 (B1,B2,x,z) units; 0x57efd0 cmd 0x18
    PZ_PKT_18               = 0x18, // 0x576200 (B) units; 0x564440 cmd 0x19
    PZ_PKT_19               = 0x19, // 0x575820 (B) units; 0x564440 cmd 0x1a
    PZ_PKT_1A               = 0x1a, // 0x575db0 (B1,B2,x,z) units; 0x57efd0 cmd 0x1b
    PZ_PKT_1B               = 0x1b, // 0x5758d0 (i32, B) units; cmd 0x1c / 0x1d
    PZ_PKT_SUPPORT_1C       = 0x1c, // 0x575880 (x,z); 0x5674c0(0,x,z,player)  air/artillery support (guessed)
    PZ_PKT_SUPPORT_1D       = 0x1d, // 0x576070 (x,z); 0x568300(0,x,z,player,-1.0,1)
    PZ_PKT_SUPPORT_1E       = 0x1e, // 0x5762b0 (x,z); 0x568740(0,x,z,player)
    PZ_PKT_SUPPORT_1F       = 0x1f, // 0x575cb0 (4 floats); 0x567760(0,x,z,player,-1.0,1,...)
    PZ_PKT_SUPPORT_20       = 0x20, // 0x576000 (4 floats); 0x567d40(...)
    PZ_PKT_21               = 0x21, // 0x575a00 (B,x,z) units; 0x57efd0 cmd 0x1f
    PZ_PKT_22               = 0x22, // 0x5759c0 writes (B,i32) units, ProcessPacket reads (B,x,z) units (HD mismatch); 0x57efd0 cmd 0x1f
    PZ_PKT_ARMY             = 0x23, // (no builder) army records (0x51f860) + PlaceUnits 0x572ec0(player): reinforcements (guessed)
    PZ_PKT_24               = 0x24, // (no builder) no-op
    PZ_PKT_LATENCY          = 0x25, // 0x5761d0 (B) "Setting network latency to %dms"
    PZ_PKT_26               = 0x26, // 0x575910 (B) units; cmd 0x21
    PZ_PKT_27               = 0x27, // 0x575b90 (B,x,z) units; 0x564660 cmd 0x22
    PZ_PKT_28               = 0x28, // 0x575bf0 (B,i32) units; 0x564720 cmd 0x23
    PZ_PKT_29               = 0x29, // 0x575c30 (B) units; 0x564440 cmd 0x24
    PZ_PKT_2A               = 0x2a, // 0x575c60 (B,f32) units; 0x564910 cmd 0x25
    PZ_PKT_2B               = 0x2b, // 0x575ab0 (B) units; 0x564440 cmd 0x26
    PZ_PKT_2C               = 0x2c, // 0x575850 (B) units; 0x564440 cmd 0x27
    PZ_PKT_2D               = 0x2d, // 0x575940 (B,B) units; 0x560790 + 0x5bbb60 cmd 0x28
    PZ_PKT_2E               = 0x2e, // 0x575980 (B,B) units; cmd 0x29
    PZ_PKT_2F               = 0x2f, // 0x5757d0 (B1,B2,i32) units; 0x564720 cmd 0x2a
    PZ_PKT_30               = 0x30, // 0x575d20 (B,i32) units; NO ProcessPacket case (ignored)
    PZ_PKT_31               = 0x31, // 0x5763f0 (B,B) units; cmd 0x2b
    PZ_PKT_32               = 0x32, // 0x576100 (u16 unit) select: unit +0x108 |= 1 << player, +0x148(player)
    PZ_PKT_33               = 0x33, // 0x576430 (u16 unit) deselect: +0x108 &= ~bit, +0x14c(player)
    PZ_PKT_34               = 0x34, // 0x576360 (B,i32) units; 0x564720 cmd 0x2c
    PZ_PKT_35               = 0x35, // 0x576460 (B) units; 0x564440 cmd 0x2d
    PZ_PKT_36               = 0x36, // 0x575fd0 (i32) units; 0x564870 cmd 0x2e
    PZ_PKT_37               = 0x37, // (no builder) i32; 0x564870 cmd 0x2f
};
// 53 ProcessPacket cases: 0x01..0x13, 0x15..0x2f, 0x31..0x37; any other op panics
// "SGameLogic::ProcessPacket: Invalid command packet" (also 0x30, which has a builder).

// Builders (packets.cpp, agent O). Argument order = HD stack order.
void Pkt_TargetUnit(SGameLogic* gl, int unit, bool b1, bool b2);            // 0x575a60 op 0x01 (RET 0xc)
void Pkt_Move(SGameLogic* gl, float x, float z, bool b1, bool b2);          // 0x575ef0 op 0x02 (RET 0x10)
void Pkt_MoveDir(SGameLogic* gl, float x, float z, float dir, bool b1, bool b2); // 0x575f50 op 0x03 (RET 0x14)
void Pkt_MoveBack(SGameLogic* gl, float x, float z, bool b1, bool b2);      // 0x575e10 op 0x04 (RET 0x10)
void Pkt_MoveBackDir(SGameLogic* gl, float x, float z, float dir, bool b1, bool b2); // 0x575e70 op 0x05 (RET 0x14)
void Pkt_Unit(SGameLogic* gl, PzPacketOp op, int value, bool b1, bool b2);  // 0x575b40 (0x07), 0x575d60 (0x0a), 0x575510 (0x0f), 0x575560 (0x10), 0x5756c0 (0x12), 0x575670 (0x15), 0x5757d0 (0x2f): (i32, B1, B2) RET 0xc
void Pkt_Pos(SGameLogic* gl, PzPacketOp op, float x, float z, bool b1, bool b2); // 0x575770 (0x09), 0x575610 (0x0e), 0x575710 (0x13), 0x575ae0 (0x16), 0x576300 (0x17), 0x575db0 (0x1a), 0x575e10 (0x04) RET 0x10
void Pkt_Flag(SGameLogic* gl, PzPacketOp op, unsigned char b);                     // 0x576230 (0x08), 0x576200 (0x18), 0x575820 (0x19), 0x575910 (0x26), 0x575c30 (0x29), 0x575ab0 (0x2b), 0x575850 (0x2c), 0x576460 (0x35) RET 4
void Pkt_Support(SGameLogic* gl, PzPacketOp op, float x, float z);          // 0x575880 (0x1c), 0x576070 (0x1d), 0x5762b0 (0x1e) RET 8, no units
void Pkt_Support4(SGameLogic* gl, PzPacketOp op, float x, float z, unsigned char flag, float dir); // 0x575cb0 (0x1f), 0x576000 (0x20) RET 0x10, no units: written dir, (float)flag, x, z
void Pkt_ValueB(SGameLogic* gl, PzPacketOp op, int value, bool b);         // 0x5754d0 (0x0b), 0x576260 (0x0c), 0x5760c0 (0x0d), 0x5759c0 (0x22), 0x575bf0 (0x28), 0x575d20 (0x30), 0x576360 (0x34): writes B then i32, RET 8
void Pkt_FloatB(SGameLogic* gl, PzPacketOp op, float value, bool b);        // 0x5763a0 (0x06), 0x575c60 (0x2a): B then f32, RET 8
void Pkt_PosB(SGameLogic* gl, PzPacketOp op, float x, float z, bool b);     // 0x575a00 (0x21), 0x575b90 (0x27): B, x, z, RET 0xc
void Pkt_TwoFlags(SGameLogic* gl, PzPacketOp op, unsigned char b0, bool b1); // 0x575940 (0x2d), 0x575980 (0x2e), 0x5763f0 (0x31): writes b1 (queue) then b0 (value), RET 8
void Pkt_1B(SGameLogic* gl, int value, bool b);                             // 0x5758d0 op 0x1b: i32 then B, RET 8
void Pkt_Latency(SGameLogic* gl, unsigned char value);                     // 0x5761d0 op 0x25, RET 4, no units
void Pkt_36(SGameLogic* gl, int value);                                     // 0x575fd0 op 0x36, RET 4
void Pkt_32(SGameLogic* gl, unsigned short unit);                           // 0x576100 op 0x32 select: u16, RET 4, no units
void Pkt_33(SGameLogic* gl, unsigned short unit);                           // 0x576430 op 0x33 deselect: u16, RET 4, no units
// Relation of a unit to a player (the order input and the cursor use it).
int GetUnitRelation(int unit, int player);                                  // 0x56d2a0: 1 own, -1 enemy, 0 allied / neutral, 3 not to be ordered
int GetUnitRelationToLocal(int unit);                                       // 0x56d280 (World+0x16c)
int UnitActionOn(SUnit* u, int target);                                    // 0x5ba280 (panzers/hudcursor.cpp): vtbl +0xa8, 10 -> 0x5ba2b0
int SelectionActionOn(int target);                                         // 0x56d490 (panzers/hudcursor.cpp): the selection's order on target, -1 none, 0 mixed
void WriteSelectedUnits(SGameLogic* gl);                                    // 0x576130

// Applies one player's frame records (the switch of SGameLogic::ProcessPacket
// 0x5737c0 at 0x5739b8; ProcessPacket itself is in packets.cpp too).
void ApplyPacketRecords(SGameLogic* gl, int player, SStream* frame);

} // namespace pz

#endif // PZ_PACKETS_H
