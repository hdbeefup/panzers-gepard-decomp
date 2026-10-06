// src/game/packets.cpp
// Packet builders and the ProcessPacket record switch (packets.h).
// OWNER: agent L (docs/M3_INTERFACES.md). Skeleton stubs.

#include "packets.h"
#include "stub_log.h"
#include "stream.h"

namespace pz {

void Pkt_TargetUnit(SGameLogic* gl, int unit, bool b1, bool b2)
{
    STUB_LOG("SGameLogic packet 0x01 TargetUnit (0x575a60)");
    PZ_M3_TRACE("SGameLogic packet 0x01 TargetUnit (0x575a60)");
    (void)gl; (void)unit; (void)b1; (void)b2;
}

void Pkt_Move(SGameLogic* gl, float x, float z, bool b1, bool b2)
{
    STUB_LOG("SGameLogic packet 0x02 Move (0x575ef0)");
    PZ_M3_TRACE("SGameLogic packet 0x02 Move (0x575ef0)");
    (void)gl; (void)x; (void)z; (void)b1; (void)b2;
}

void Pkt_MoveDir(SGameLogic* gl, float x, float z, float dir, bool b1, bool b2)
{
    STUB_LOG("SGameLogic packet 0x03 MoveDir (0x575f50)");
    PZ_M3_TRACE("SGameLogic packet 0x03 MoveDir (0x575f50)");
    (void)gl; (void)x; (void)z; (void)dir; (void)b1; (void)b2;
}

void Pkt_MoveBack(SGameLogic* gl, float x, float z, bool b1, bool b2)
{
    STUB_LOG("SGameLogic packet 0x04 MoveBack (0x575e10)");
    PZ_M3_TRACE("SGameLogic packet 0x04 MoveBack (0x575e10)");
    (void)gl; (void)x; (void)z; (void)b1; (void)b2;
}

void Pkt_MoveBackDir(SGameLogic* gl, float x, float z, float dir, bool b1, bool b2)
{
    STUB_LOG("SGameLogic packet 0x05 MoveBackDir (0x575e70)");
    PZ_M3_TRACE("SGameLogic packet 0x05 MoveBackDir (0x575e70)");
    (void)gl; (void)x; (void)z; (void)dir; (void)b1; (void)b2;
}

void Pkt_Unit(SGameLogic* gl, PzPacketOp op, int value, bool b1, bool b2)
{
    STUB_LOG("SGameLogic packet (i32, B1, B2) builders (0x575b40 ...)");
    PZ_M3_TRACE("SGameLogic packet (i32, B1, B2) builders (0x575b40 ...)");
    (void)gl; (void)op; (void)value; (void)b1; (void)b2;
}

void Pkt_Pos(SGameLogic* gl, PzPacketOp op, float x, float z, bool b1, bool b2)
{
    STUB_LOG("SGameLogic packet (x, z, B1, B2) builders (0x575770 ...)");
    PZ_M3_TRACE("SGameLogic packet (x, z, B1, B2) builders (0x575770 ...)");
    (void)gl; (void)op; (void)x; (void)z; (void)b1; (void)b2;
}

void Pkt_Flag(SGameLogic* gl, PzPacketOp op, bool b)
{
    STUB_LOG("SGameLogic packet (B) builders (0x576230 ...)");
    PZ_M3_TRACE("SGameLogic packet (B) builders (0x576230 ...)");
    (void)gl; (void)op; (void)b;
}

void Pkt_Support(SGameLogic* gl, PzPacketOp op, float x, float z)
{
    STUB_LOG("SGameLogic packet support (x, z) builders (0x575880 ...)");
    PZ_M3_TRACE("SGameLogic packet support (x, z) builders (0x575880 ...)");
    (void)gl; (void)op; (void)x; (void)z;
}

void Pkt_Support4(SGameLogic* gl, PzPacketOp op, float x, float z, float x2, float z2)
{
    STUB_LOG("SGameLogic packet support (4 floats) builders (0x575cb0 / 0x576000)");
    PZ_M3_TRACE("SGameLogic packet support (4 floats) builders (0x575cb0 / 0x576000)");
    (void)gl; (void)op; (void)x; (void)z; (void)x2; (void)z2;
}

void Pkt_ValueB(SGameLogic* gl, PzPacketOp op, int value, bool b)
{
    STUB_LOG("SGameLogic packet (B, i32) builders (0x576260 ...)");
    PZ_M3_TRACE("SGameLogic packet (B, i32) builders (0x576260 ...)");
    (void)gl; (void)op; (void)value; (void)b;
}

void Pkt_FloatB(SGameLogic* gl, PzPacketOp op, float value, bool b)
{
    STUB_LOG("SGameLogic packet (B, f32) builders (0x5763a0 / 0x575c60)");
    PZ_M3_TRACE("SGameLogic packet (B, f32) builders (0x5763a0 / 0x575c60)");
    (void)gl; (void)op; (void)value; (void)b;
}

void Pkt_PosB(SGameLogic* gl, PzPacketOp op, float x, float z, bool b)
{
    STUB_LOG("SGameLogic packet (B, x, z) builders (0x575a00 / 0x575b90)");
    PZ_M3_TRACE("SGameLogic packet (B, x, z) builders (0x575a00 / 0x575b90)");
    (void)gl; (void)op; (void)x; (void)z; (void)b;
}

void Pkt_TwoFlags(SGameLogic* gl, PzPacketOp op, bool b0, bool b1)
{
    STUB_LOG("SGameLogic packet (B, B) builders (0x575940 ...)");
    PZ_M3_TRACE("SGameLogic packet (B, B) builders (0x575940 ...)");
    (void)gl; (void)op; (void)b0; (void)b1;
}

void Pkt_1B(SGameLogic* gl, int value, bool b)
{
    STUB_LOG("SGameLogic packet 0x1b (0x5758d0)");
    PZ_M3_TRACE("SGameLogic packet 0x1b (0x5758d0)");
    (void)gl; (void)value; (void)b;
}

void Pkt_Latency(SGameLogic* gl, unsigned char value)
{
    STUB_LOG("SGameLogic packet 0x25 latency (0x5761d0)");
    PZ_M3_TRACE("SGameLogic packet 0x25 latency (0x5761d0)");
    (void)gl; (void)value;
}

void Pkt_36(SGameLogic* gl, int value)
{
    STUB_LOG("SGameLogic packet 0x36 (0x575fd0)");
    PZ_M3_TRACE("SGameLogic packet 0x36 (0x575fd0)");
    (void)gl; (void)value;
}

void Pkt_32(SGameLogic* gl, unsigned short unit)
{
    STUB_LOG("SGameLogic packet 0x32 (0x576100)");
    PZ_M3_TRACE("SGameLogic packet 0x32 (0x576100)");
    (void)gl; (void)unit;
}

void WriteSelectedUnits(SGameLogic* gl)
{
    STUB_LOG("SGameLogic::WriteSelectedUnits (0x576130)");
    PZ_M3_TRACE("SGameLogic::WriteSelectedUnits (0x576130)");
    (void)gl;
}

void ApplyPacketRecords(SGameLogic* gl, int player, SStream* frame)
{
    STUB_LOG("SGameLogic::ProcessPacket record switch (0x5739b8)");
    PZ_M3_TRACE("SGameLogic::ProcessPacket record switch (0x5739b8)");
    (void)gl; (void)player; (void)frame;
}

} // namespace pz
