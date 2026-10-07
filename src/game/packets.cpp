// src/game/packets.cpp
// The lockstep packets (packets.h): the builders 0x5754d0..0x576430, the
// selected-unit list, SGameLogic::ProcessPacket 0x5737c0 (recording,
// playback, the CRC check and the record switch at 0x5739b8), the order
// handlers it calls and -packetrec / -packetplay.
// OWNER: agent O (docs/M3_INTERFACES.md §4).
//
// Effects of agent C (support calls, unit selection slots) and agent F
// (army placement, the world speech event) are called through the per-case
// stubs at the top of the file until those land.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "packets.h"
#include "packets_rec.h"
#include "gamelogic.h"
#include "trigger.h"
#include "unit.h"
#include "gunner.h"
#include "world.h"
#include "worldapi.h"
#include "blockmap.h"
#include "campaign.h"
#include "logger.h"
#include "stream.h"
#include "core_common.h"
#include "stub_log.h"
#include "pz/ipixie.h"

namespace pz {

void GroupStats(SFoundUnits* g);   // triggers.cpp (0x582770)

namespace {

SStream* Frame(SGameLogic* gl) { return (SStream*)gl->FrameObject; }   // +0x20

int FBits(float f)
{
    int i;
    memcpy(&i, &f, 4);
    return i;
}

float BitsF(int i)
{
    float f;
    memcpy(&f, &i, 4);
    return f;
}

// HD 0x546490 (SHeapTRB::operator[] of the unit heap): panics when not live.
SUnit* LiveUnit(int i)
{
    if (!g_World || !g_World->Units.IsLive(i))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", i);
    return WorldUnit(i);
}

// HD 0x560790 / inline SDArray<SFoundUnit>::operator[]: the unit of entry k.
int FoundAt(const SFoundUnits* g, int k)
{
    if (k < 0 || k >= g->Count)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SFoundUnit", k);
    int u = g->Units[k].Unit;
    if (!g_World->Units.IsLive(u))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", u);
    return u;
}

// PANZERS 0x563580 (SDArray<SFoundUnit>::Clear(n))
void ClearFoundUnits(SFoundUnits* g, int n)
{
    if (g->Count != 0 && g->Units == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "struct SFoundUnit");
    g->Count = n;
    if (g->Max < n) {
        g->Max = n;
        g->Units = (SFoundUnit*)realloc(g->Units, n * sizeof(SFoundUnit));
    }
    if (g->Max > 0)
        memset(g->Units, 0, g->Max * sizeof(SFoundUnit));
}

// PANZERS 0x560460 (SFoundUnits copy assignment)
void CopyFoundUnits(SFoundUnits* dst, const SFoundUnits* src)
{
    dst->X = src->X;
    dst->Z = src->Z;
    dst->RadiusSq = src->RadiusSq;
    dst->Kind = src->Kind;
    dst->Leader = src->Leader;
    ClearFoundUnits(dst, src->Count);
    for (int i = 0; i < dst->Count; ++i)
        dst->Units[i] = src->Units[i];
}

// PANZERS 0x560170 (SDArray<SFoundUnit>::Free)
void FreeFoundUnits(SFoundUnits* g)
{
    if (g->Units) {
        free(g->Units);
        g->Units = nullptr;
    }
    g->Max = 0;
    g->Count = 0;
}

// --- SUnit order-queue entry points that unit.h does not have yet (agent C
// owns SUnit; these follow SUnit::OrderAt 0x5bb980 exactly, with the order
// fields the HD variants set).

// PANZERS 0x5b75c0 (SDArray<SUnit::SOrder>::Clear(0))
void ClearOrders(SUnit* u)
{
    SUnitArray<SUnit::SOrder>* a = &u->Orders;
    if (a->Size != 0 && a->Array == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "struct SUnit::SOrder");
    a->Size = 0;
    if (a->Max > 0)
        memset((void*)a->Array, 0, a->Max * sizeof(SUnit::SOrder));
}

// PANZERS 0x5b59a0 (SDArray<SUnit::SOrder>::Add)
int AddOrder(SUnit* u)
{
    SUnitArray<SUnit::SOrder>* a = &u->Orders;
    if (a->Size == a->Max) {
        int nmax = a->Max < 0x10 ? 0x10 : (a->Max * 6) / 5;
        a->Array = (SUnit::SOrder*)realloc(a->Array, nmax * sizeof(SUnit::SOrder));
        memset((void*)&a->Array[a->Max], 0, (nmax - a->Max) * sizeof(SUnit::SOrder));
        a->Max = nmax;
    }
    return a->Size++;
}

void IssueOrder(SUnit* u, const SUnit::SOrder& o, bool add)
{
    if (!add) {
        ClearOrders(u);                                           // 0x5b75c0(0)
        u->ExecuteCommand(o);                                     // 0x5b95a0
        return;
    }
    int i = AddOrder(u);
    if (i < 0 || i >= u->Orders.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnit::SOrder", i);
    u->Orders.Array[i] = o;
}

// PANZERS 0x5bb7b0 (SUnit: an order without arguments)
void UnitOrderPlain(SUnit* u, int command, bool queue, bool add)
{
    SUnit::SOrder o;
    memset(&o, 0, sizeof(o));
    o.Command = command;
    o.Queue = queue;
    IssueOrder(u, o, add);
}

// PANZERS 0x5bbc40 (SUnit: an order with a float in +0x14)
void UnitOrderFloat(SUnit* u, int command, float value, bool queue, bool add)
{
    SUnit::SOrder o;
    memset(&o, 0, sizeof(o));
    o.Command = command;
    o.Param2 = FBits(value);
    o.Queue = queue;
    IssueOrder(u, o, add);
}

// PANZERS 0x5bba70 (SUnit: an order at a point with a direction in +0x14)
void UnitOrderAtDir(SUnit* u, int command, const float* xz, float dir, bool queue, bool add)
{
    SUnit::SOrder o;
    memset(&o, 0, sizeof(o));
    o.Command = command;
    o.X = xz[0];
    o.Z = xz[1];
    o.Param2 = FBits(dir);
    o.Queue = queue;
    IssueOrder(u, o, add);
}

// --- Order acknowledgements (SUnit 0x5bc9d0..0x5bcdf0): the voice of the
// unit through SWorld::UnitSpeech 0x5fff20. With its last argument 0 that
// event plays only for the local player's units and picks the sample with
// the CRT rand (0x78c846), so it never reaches the world CRC.
void WorldSpeech(int unit, int kind)
{
    g_World->UnitSpeech(unit, kind, false);                       // 0x5fff20(unit, kind, 0)
}

// PANZERS 0x5bcdf0
void SpeechMove(SUnit* u)
{
    WorldSpeech(u->WorldIndex, u->ActiveDriver >= 0 ? 3 : 4);
}

// PANZERS 0x5bcab0
void SpeechOrder(SUnit* u)
{
    WorldSpeech(u->WorldIndex, 2);
}

// PANZERS 0x5bca90
void SpeechGunners(SUnit* u)
{
    if (u->Gunners.Size > 0)
        WorldSpeech(u->WorldIndex, 2);
}

// PANZERS 0x5bc9d0
// Attack a unit: "Attack" (5) when the main gunner can hit it (0x583b60),
// else "CantAttack" (6); nothing without a gunner or a live target.
void SpeechAttackUnit(SUnit* u, int target)
{
    if (u->Gunners.Size <= 0 || !g_World->Units.IsLive(target))
        return;
    if (u->Gunners.Array[0]->CanTargetUnit(WorldUnit(target), true))   // +0x48[0], 0x583b60(target, 1)
        WorldSpeech(u->WorldIndex, 5);
    else
        WorldSpeech(u->WorldIndex, 6);
}

// PANZERS 0x5bcad0
// The voice for an order on a unit, by the selection's order on it
// (0x56d490): 1 / 2 move, 3 attack (0x5bc9d0), otherwise "Acknowledge".
void SpeechAttack(SUnit* u, int target)
{
    int k = SelectionActionOn(target);                            // 0x56d490
    if (k > 0) {
        if (k < 3) {
            SpeechMove(u);
            return;
        }
        if (k == 3) {
            SpeechAttackUnit(u, target);
            return;
        }
    }
    WorldSpeech(u->WorldIndex, 2);
}

// --- ProcessPacket cases whose effect belongs to another agent.

// The support calls 0x5674c0 / 0x568300 / 0x568740 / 0x567760 / 0x567d40
// (SGameLogic, combat_support.cpp, agent C) with the arguments ProcessPacket
// passes in HD: free = 0, extra altitude -1.0, p6 = 1.
void Support_5674c0(float x, float z, int player)
{
    g_GameLogic->SupportArtillery(false, x, z, player);            // 0x5674c0(0, x, z, player)
}
void Support_568300(float x, float z, int player)
{
    g_GameLogic->SupportRecon(false, x, z, player, -1.0f, true);   // 0x568300(0, x, z, player, -1.0, 1)
}
void Support_568740(float x, float z, int player)
{
    g_GameLogic->SupportTacBomber(false, x, z, player);            // 0x568740(0, x, z, player)
}
void Support_567760(float x, float z, int player, bool flag, float dir)
{
    g_GameLogic->SupportHeavyBomber(false, x, z, player, -1.0f, true, flag, dir);   // 0x567760(0, x, z, player, -1.0, 1, f, dir)
}
void Support_567d40(float x, float z, int player, bool flag, float dir)
{
    g_GameLogic->SupportParatroopers(false, x, z, player, -1.0f, true, flag, dir);  // 0x567d40(0, x, z, player, -1.0, 1, f, dir)
}

// Unit slots +0x148 / +0x14c take the player (HD pushes it).
void UnitSelectedBy(SUnit* u, int player)
{
    u->Slot_148(player);
}
void UnitDeselectedBy(SUnit* u, int player)
{
    u->Slot_14C(player);
}

// Op 0x23: army records (0x51f860) placed by SGameLogic::PlaceUnits 0x572ec0 (agent F).
void PlaceArmyFromPacket(SStream* frame, int player)
{
    STUB_LOG("SGameLogic::ProcessPacket op 0x23 army (0x51f860, PlaceUnits 0x572ec0, agent F)");
    (void)frame; (void)player;
    throw "SGameLogic::ProcessPacket: op 0x23 (army) not lifted";
}

} // namespace

// ---------------------------------------------------------------------------
// Builders. Each writes its record into the frame stream (+0x20), then calls
// CheckSendQSize. "units" = WriteSelectedUnits.

// PANZERS 0x576130
void WriteSelectedUnits(SGameLogic* gl)
{
    SStream* s = Frame(gl);
    SWorld* w = g_World;
    for (int i = 0; i < w->Units.Size; ++i) {
        if (!w->Units.IsLive(i))
            continue;
        if (WorldUnit(i)->_104 & 1)                               // TEST byte [unit+0x104], 1
            s->WriteWord((unsigned short)i);                      // 0x65dd00
    }
    s->WriteWord(0xffff);
}

// PANZERS 0x575a60
void Pkt_TargetUnit(SGameLogic* gl, int unit, bool b1, bool b2)
{
    PZ_M3_TRACE("SGameLogic packet 0x01 (0x575a60)");
    SStream* s = Frame(gl);
    s->WriteByte(PZ_PKT_TARGET_UNIT);
    s->WriteByte(b1);
    s->WriteByte(b2);
    s->WriteInt(unit);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x575ef0
void Pkt_Move(SGameLogic* gl, float x, float z, bool b1, bool b2)
{
    PZ_M3_TRACE("SGameLogic packet 0x02 Move (0x575ef0)");
    SStream* s = Frame(gl);
    s->WriteByte(PZ_PKT_MOVE);
    s->WriteByte(b1);
    s->WriteByte(b2);
    s->WriteFloat(x);                                             // 0x65dc20
    s->WriteFloat(z);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x575f50
void Pkt_MoveDir(SGameLogic* gl, float x, float z, float dir, bool b1, bool b2)
{
    PZ_M3_TRACE("SGameLogic packet 0x03 MoveDir (0x575f50)");
    SStream* s = Frame(gl);
    s->WriteByte(PZ_PKT_MOVE_DIR);
    s->WriteByte(b1);
    s->WriteByte(b2);
    s->WriteFloat(x);
    s->WriteFloat(z);
    s->WriteFloat(dir);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x575e10
void Pkt_MoveBack(SGameLogic* gl, float x, float z, bool b1, bool b2)
{
    Pkt_Pos(gl, PZ_PKT_MOVE_BACK, x, z, b1, b2);
}

// PANZERS 0x575e70
void Pkt_MoveBackDir(SGameLogic* gl, float x, float z, float dir, bool b1, bool b2)
{
    PZ_M3_TRACE("SGameLogic packet 0x05 MoveBackDir (0x575e70)");
    SStream* s = Frame(gl);
    s->WriteByte(PZ_PKT_MOVE_BACK_DIR);
    s->WriteByte(b1);
    s->WriteByte(b2);
    s->WriteFloat(x);
    s->WriteFloat(z);
    s->WriteFloat(dir);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x575b40 (op 0x07), 0x575d60 (0x0a), 0x575510 (0x0f), 0x575560 (0x10),
// 0x5756c0 (0x12), 0x575670 (0x15), 0x5757d0 (0x2f); 0x575a60 (0x01) as Pkt_TargetUnit.
void Pkt_Unit(SGameLogic* gl, PzPacketOp op, int value, bool b1, bool b2)
{
    PZ_M3_TRACE("SGameLogic packet (B1, B2, i32) (0x575b40 ...)");
    SStream* s = Frame(gl);
    s->WriteByte(op);
    s->WriteByte(b1);
    s->WriteByte(b2);
    s->WriteInt(value);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x575770 (op 0x09), 0x575610 (0x0e), 0x575710 (0x13), 0x575ae0 (0x16),
// 0x576300 (0x17), 0x575db0 (0x1a), 0x575e10 (0x04)
void Pkt_Pos(SGameLogic* gl, PzPacketOp op, float x, float z, bool b1, bool b2)
{
    PZ_M3_TRACE("SGameLogic packet (B1, B2, x, z) (0x575770 ...)");
    SStream* s = Frame(gl);
    s->WriteByte(op);
    s->WriteByte(b1);
    s->WriteByte(b2);
    s->WriteFloat(x);
    s->WriteFloat(z);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x576230 (op 0x08), 0x576200 (0x18), 0x575820 (0x19), 0x575910 (0x26),
// 0x575c30 (0x29), 0x575ab0 (0x2b), 0x575850 (0x2c)
void Pkt_Flag(SGameLogic* gl, PzPacketOp op, unsigned char b)
{
    PZ_M3_TRACE("SGameLogic packet (B) (0x576230 ...)");
    SStream* s = Frame(gl);
    s->WriteByte(op);
    s->WriteByte(b);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x575880 (op 0x1c), 0x576070 (0x1d), 0x5762b0 (0x1e): no units
void Pkt_Support(SGameLogic* gl, PzPacketOp op, float x, float z)
{
    PZ_M3_TRACE("SGameLogic packet support (x, z) (0x575880 ...)");
    SStream* s = Frame(gl);
    s->WriteByte(op);
    s->WriteFloat(x);
    s->WriteFloat(z);
    gl->CheckSendQSize();
}

// PANZERS 0x575cb0 (op 0x1f), 0x576000 (0x20): (x, z, u8 flag, f32 dir) written
// dir, (float)flag, x, z; no units. ProcessPacket reads the flag as an i32.
void Pkt_Support4(SGameLogic* gl, PzPacketOp op, float x, float z, unsigned char flag, float dir)
{
    PZ_M3_TRACE("SGameLogic packet support (x, z, flag, dir) (0x575cb0 / 0x576000)");
    SStream* s = Frame(gl);
    s->WriteByte(op);
    s->WriteFloat(dir);
    s->WriteFloat((float)flag);                                   // MOVZX, CVTDQ2PS
    s->WriteFloat(x);
    s->WriteFloat(z);
    gl->CheckSendQSize();
}

// PANZERS 0x576260 (op 0x0c), 0x5760c0 (0x0d), 0x5754d0 (0x0b), 0x5759c0 (0x22),
// 0x575bf0 (0x28), 0x575d20 (0x30), 0x576360 (0x34): B then i32
void Pkt_ValueB(SGameLogic* gl, PzPacketOp op, int value, bool b)
{
    PZ_M3_TRACE("SGameLogic packet (B, i32) (0x576260 ...)");
    SStream* s = Frame(gl);
    s->WriteByte(op);
    s->WriteByte(b);
    s->WriteInt(value);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x5763a0 (op 0x06), 0x575c60 (0x2a): B then f32
void Pkt_FloatB(SGameLogic* gl, PzPacketOp op, float value, bool b)
{
    PZ_M3_TRACE("SGameLogic packet (B, f32) (0x5763a0 / 0x575c60)");
    SStream* s = Frame(gl);
    s->WriteByte(op);
    s->WriteByte(b);
    s->WriteFloat(value);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x575a00 (op 0x21), 0x575b90 (0x27): B, x, z
void Pkt_PosB(SGameLogic* gl, PzPacketOp op, float x, float z, bool b)
{
    PZ_M3_TRACE("SGameLogic packet (B, x, z) (0x575a00 / 0x575b90)");
    SStream* s = Frame(gl);
    s->WriteByte(op);
    s->WriteByte(b);
    s->WriteFloat(x);
    s->WriteFloat(z);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x575940 (op 0x2d), 0x575980 (0x2e), 0x5763f0 (0x31): written b1 then b0
void Pkt_TwoFlags(SGameLogic* gl, PzPacketOp op, unsigned char b0, bool b1)
{
    PZ_M3_TRACE("SGameLogic packet (B, B) (0x575940 ...)");
    SStream* s = Frame(gl);
    s->WriteByte(op);
    s->WriteByte(b1);
    s->WriteByte(b0);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x5758d0
void Pkt_1B(SGameLogic* gl, int value, bool b)
{
    PZ_M3_TRACE("SGameLogic packet 0x1b (0x5758d0)");
    SStream* s = Frame(gl);
    s->WriteByte(PZ_PKT_1B);
    s->WriteInt(value);
    s->WriteByte(b);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x5761d0
void Pkt_Latency(SGameLogic* gl, unsigned char value)
{
    SStream* s = Frame(gl);
    s->WriteByte(PZ_PKT_LATENCY);
    s->WriteByte(value);
    gl->CheckSendQSize();
}

// PANZERS 0x575fd0
void Pkt_36(SGameLogic* gl, int value)
{
    SStream* s = Frame(gl);
    s->WriteByte(PZ_PKT_36);
    s->WriteInt(value);
    WriteSelectedUnits(gl);
    gl->CheckSendQSize();
}

// PANZERS 0x576100
void Pkt_32(SGameLogic* gl, unsigned short unit)
{
    PZ_M3_TRACE("SGameLogic packet 0x32 select (0x576100)");
    SStream* s = Frame(gl);
    s->WriteByte(PZ_PKT_32);
    s->WriteWord(unit);
    gl->CheckSendQSize();
}

// PANZERS 0x576430
void Pkt_33(SGameLogic* gl, unsigned short unit)
{
    PZ_M3_TRACE("SGameLogic packet 0x33 deselect (0x576430)");
    SStream* s = Frame(gl);
    s->WriteByte(PZ_PKT_33);
    s->WriteWord(unit);
    gl->CheckSendQSize();
}

// PANZERS 0x562fb0
// Only with SMulti (DAT_008f1a74): warns when the unsent part of the frame
// grows past 250 bytes ("SGameLogic::CheckSendQSize() tul nagy a meret %d").
void SGameLogic::CheckSendQSize()
{
    // The recompile has no SMulti: DAT_008f1a74 == 0, nothing to check.
}

// ---------------------------------------------------------------------------
// Reading a record's units

// PANZERS 0x56d540 (SFoundUnits from the u16 list of WriteSelectedUnits)
// Units that died or were stored since the order was sent are skipped.
static void ReadFoundUnits(SFoundUnits* g, SStream* s)
{
    g->X = 0.0f;
    g->Z = 0.0f;
    g->Units = nullptr;
    g->Count = 0;
    g->Max = 0;
    unsigned v = s->ReadWord();                                   // 0x65d770
    for (;;) {
        v &= 0xffff;
        if (v == 0xffff)
            break;
        SWorld* w = g_World;
        if ((int)v < w->Units.Size && w->Units.Array[v].Next == kHeapLive) {
            SUnit* u = WorldUnit((int)v);
            if (!u->Wrecked && !u->Unplaced) {                    // +0x150, +0x168
                // PANZERS 0x560d80 (SDArray<SFoundUnit>::Add)
                if (g->Count == g->Max) {
                    int nmax = g->Max < 0x10 ? 0x10 : (g->Max * 6) / 5;
                    g->Units = (SFoundUnit*)realloc(g->Units, nmax * sizeof(SFoundUnit));
                    memset(g->Units + g->Max, 0, (nmax - g->Max) * sizeof(SFoundUnit));
                    g->Max = nmax;
                }
                g->Units[g->Count++].Unit = (int)v;
            }
        }
        v = s->ReadWord();
    }
    GroupStats(g);                                                // 0x582770
}

// The idiom of every unit case: 0x56d540 into a temporary, copied into the
// frame's group (0x560460), the temporary freed (0x560170).
static void ReadGroup(SFoundUnits* group, SStream* s)
{
    SFoundUnits tmp;
    memset(&tmp, 0, sizeof(tmp));
    ReadFoundUnits(&tmp, s);
    CopyFoundUnits(group, &tmp);
    FreeFoundUnits(&tmp);
}

// ---------------------------------------------------------------------------
// Order handlers (SGameLogic)

// PANZERS 0x564440
void SGameLogic::OrderPlain(SFoundUnits* g, int command, bool p3, bool queue)
{
    GroupOrder(false, p3, g, false, 0.0f, 0.0f);                  // 0x56ff30(0, p3, g, 0, 0, 0)
    for (int j = 0; j < g->Count; ++j)
        UnitOrderPlain(LiveUnit(FoundAt(g, j)), command, p3, queue);   // 0x5bb7b0
}

// PANZERS 0x564660
void SGameLogic::OrderAtPoint(SFoundUnits* g, int command, const float* xz, bool p4, bool queue)
{
    GroupOrder(false, p4, g, true, xz[0], xz[1]);
    for (int j = 0; j < g->Count; ++j)
        LiveUnit(FoundAt(g, j))->OrderAt(command, xz, p4, queue); // 0x5bb980
}

// PANZERS 0x564720
// Command 0x16 keeps the movement groups; the others regroup towards the
// target unit (or without a target point when it is gone).
void SGameLogic::OrderAtUnit(SFoundUnits* g, int command, int unit, bool p4, bool queue)
{
    if (command != 0x16) {
        if (unit >= 0 && unit < g_World->Units.Size && g_World->Units.Array[unit].Next == kHeapLive) {
            SUnit* t = LiveUnit(unit);
            GroupOrder(false, p4, g, true, t->Pos[0], t->Pos[2]);    // +0x8c, +0x94
        } else {
            GroupOrder(false, p4, g, false, 0.0f, 0.0f);
        }
    }
    for (int j = 0; j < g->Count; ++j)
        LiveUnit(FoundAt(g, j))->OrderUnit(command, unit, p4, queue);   // 0x5bb8a0
}

// PANZERS 0x564870
void SGameLogic::OrderValue(SFoundUnits* g, int command, int value, bool p4, bool queue)
{
    for (int j = 0; j < g->Count; ++j)
        LiveUnit(FoundAt(g, j))->OrderParam(command, value, p4, queue);  // 0x5bbb60
}

// PANZERS 0x564910
void SGameLogic::OrderFloat(SFoundUnits* g, int command, float value, bool p4, bool queue)
{
    for (int j = 0; j < g->Count; ++j)
        UnitOrderFloat(LiveUnit(FoundAt(g, j)), command, value, p4, queue);   // 0x5bbc40
}

// PANZERS 0x5708f0
// Overwrites the formation offsets of the movement group members with the
// offsets of their found units (matched by unit, in found-unit order).
void SGameLogic::ModifyMovementGroupUnitsFormationPos(int group, SFoundUnits* g, const float* offsets)
{
    if (group < 0 || group >= MovementGroupSize || MovementGroups[group].Next != kHeapLive)
        Logger.g->Panic("SGameLogic::ModifyMovementGroupUnitsFormationPos: Invalid movement group %d", group);
    int k = 0;
    for (int j = 0; j < g->Count; ++j, offsets += 2) {
        SMovementGroupElem* mg = &MovementGroups[group];
        if (k >= mg->MemberCount)
            return;
        if (j < 0 || j >= g->Count)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SFoundUnit", j);
        if (g->Units[j].Unit == mg->Members[k].Unit) {
            mg->Members[k].DX = offsets[0];
            mg->Members[k].DZ = offsets[1];
            ++k;
        }
    }
}

// PANZERS 0x582a90 (qsort comparator: nearest to the group centre first)
static int __cdecl CompareFoundDist(const void* a, const void* b)
{
    float da = BitsF(((const SFoundUnit*)a)->_04), db = BitsF(((const SFoundUnit*)b)->_04);
    return (db <= da && da != db) * 2 - 1;
}

// PANZERS 0x57e8b0
// A move whose target lies inside the group: every unit gets a free spot
// next to the target, nearest units first, so they do not all stop on the
// same point. The spots are kept on the block map while the others search.
void SGameLogic::MoveFoundUnitsNear(SFoundUnits* g, int command, const float* target, bool p4, bool queue, bool marker)
{
    if (g->Count == 0)
        return;
    int n = g->Count;
    for (int j = 0; j < n; ++j) {
        SUnit* u = LiveUnit(FoundAt(g, j));
        u->SetOnBlockMap(false);                                  // +0x198(0)
        float dx = g->X - u->Pos[0];
        float dz = g->Z - u->Pos[2];
        g->Units[j]._04 = FBits(dx * dx + dz * dz);
    }
    qsort(g->Units, g->Count, sizeof(SFoundUnit), CompareFoundDist);
    float* spots = new float[n * 2]();                            // 0x766b87(n * 8)
    float* offsets = new float[n * 2]();
    for (int j = 0; j < g->Count; ++j) {
        SUnit* u = LiveUnit(FoundAt(g, j));
        float ref[2];
        ref[0] = target[0] + (u->Pos[0] - g->X);
        ref[1] = target[1] + (u->Pos[2] - g->Z);
        float p[2];
        g_World->FindEmptySpaceNear(p, target[0], target[1], ref[0], ref[1], u->UnitSizeBlocks, u->MoveFlags, true);   // 0x5e5700
        spots[j * 2] = p[0];
        spots[j * 2 + 1] = p[1];
        BlockMap_MarkDynamic(g_World, p[0], p[1], u->UnitSizeBlocks, true);    // 0x5f4430(.., 1)
    }
    for (int j = 0; j < g->Count; ++j) {
        SUnit* u = LiveUnit(FoundAt(g, j));
        BlockMap_MarkDynamic(g_World, spots[j * 2], spots[j * 2 + 1], u->UnitSizeBlocks, false);
    }
    for (int j = 0; j < g->Count; ++j)
        LiveUnit(FoundAt(g, j))->SetOnBlockMap(true);
    int mg = GroupOrder(false, p4, g, true, target[0], target[1]);    // 0x56ff30(0, p4, g, 1, x, z)
    if (mg >= 0) {
        if (mg >= MovementGroupSize || MovementGroups[mg].Next != kHeapLive)
            Logger.g->Panic("SGameLogic::SetMovementGroupFormationDir: Invalid movement group %d", mg);
        MovementGroups[mg].B24 = true;
        for (int j = 0; j < g->Count; ++j) {
            offsets[j * 2] = spots[j * 2] - target[0];
            offsets[j * 2 + 1] = spots[j * 2 + 1] - target[1];
        }
        ModifyMovementGroupUnitsFormationPos(mg, g, offsets);
    }
    for (int j = 0; j < g->Count; ++j) {
        SUnit* u = LiveUnit(FoundAt(g, j));
        const float* p = u->_254 < 0 ? &spots[j * 2] : target;    // +0x254 movement group
        u->OrderAt(command, p, p4, queue);                        // 0x5bb980
        if (marker && g_Pixie) {
            float pos[3] = { spots[j * 2], 0.0f, spots[j * 2 + 1] };
            float dir[3] = { 0.0f, 1.0f, 0.0f };
            g_Pixie->PlayEffect(g_Scene, TargetRingFx, pos, dir, 0);   // pixie +0x24
        }
    }
    delete[] spots;                                               // 0x76654a
    delete[] offsets;
}

// PANZERS 0x661bb0 (2D affine matrix product: rows (a b), (c d), (tx ty); out = a * b)
static void Mul23(const float* a, float* out, const float* b)
{
    float b2 = b[2], b0 = b[0], b3 = b[3], b1 = b[1];
    float a3 = a[3], a2 = a[2], a1 = a[1];
    out[0] = a[0] * b0 + a[1] * b2;
    out[1] = b1 * a[0] + b3 * a1;
    float a5 = a[5];
    out[3] = a2 * b1 + a3 * b3;
    float a4 = a[4];
    out[2] = a2 * b0 + a3 * b2;
    float b5 = b[5];
    out[4] = a4 * b0 + a5 * b2 + b[4];
    out[5] = a4 * b1 + a5 * b3 + b5;
}

// PANZERS 0x57f200
// A move with a facing: the group keeps its shape, turned about its centre
// by the angle between `dir` and the direction from the centre to the
// target, and moved onto the target (translate by -centre, rotate,
// translate by the target). Units that joined the movement group go to the
// target itself; each order carries the facing.
void SGameLogic::MoveFoundUnitsToLocationDir(SFoundUnits* g, int command, const float* target, float dir, bool p4,
                                             bool queue, bool marker)
{
    GroupOrder(false, p4, g, true, target[0], target[1]);         // 0x56ff30(0, p4, g, 1, x, z)
    float mt[6] = { 1.0f, 0.0f, 0.0f, 1.0f, target[0], target[1] };   // DAT_007f7fd0 + target
    double a = atan2((double)(target[0] - g->X), (double)(target[1] - g->Z));   // 0x78d07a(ST1 = dx, ST0 = dz)
    float r = dir - (float)a;
    float mr[6];
    mr[0] = (float)cos((double)r);                                // 0x78d480
    double sn = sin((double)r);                                   // 0x78d640
    mr[1] = (float)-sn;
    mr[2] = (float)sn;
    mr[3] = mr[0];
    mr[4] = 0.0f;
    mr[5] = 0.0f;
    float mg[6] = { 1.0f, 0.0f, 0.0f, 1.0f, -g->X, -g->Z };
    float m1[6], m[6];
    Mul23(mg, m1, mr);
    Mul23(m1, m, mt);
    for (int j = 0; j < g->Count; ++j) {
        SUnit* u = LiveUnit(FoundAt(g, j));
        float ux = u->Pos[0], uz = u->Pos[2];
        float p[2];
        p[0] = m[2] * uz + m[0] * ux + m[4];
        p[1] = m[3] * uz + m[1] * ux + m[5];
        const float* at = u->_254 < 0 ? p : target;
        UnitOrderAtDir(u, command, at, dir, p4, queue);           // 0x5bba70
        if (marker && g_Pixie) {
            float pos[3] = { p[0], 0.0f, p[1] };
            float up[3] = { 0.0f, 1.0f, 0.0f };
            g_Pixie->PlayEffect(g_Scene, TargetRingFx, pos, up, 0);   // pixie +0x24
        }
    }
}

// ---------------------------------------------------------------------------
// ProcessPacket

namespace {

// The record switch of 0x5737c0 (jump table at 0x5739b8). `group` is the
// frame's SFoundUnits (local_60), `b1` the B1 of the last record that read
// one (local_3c: op 0x34 passes it on without reading its own).
void ApplyRecords(SGameLogic* gl, int player, SStream* s, SFoundUnits* group)
{
    bool b1 = false;
    bool b2 = false;
    const bool local = player == g_World->LocalPlayer;           // marker: param_2 == World+0x16c
    unsigned char op = s->ReadByte();                             // 0x65d300
    while (op != 0) {
        switch (op) {
        case 0x01: {
            b1 = s->ReadByte() != 0;
            b2 = s->ReadByte() != 0;
            int unit = s->ReadInt();
            ReadGroup(group, s);
            if (group->Leader >= 0) {
                SpeechAttack(LiveUnit(group->Leader), unit);      // 0x5bcad0
                gl->OrderAtUnit(group, 0, unit, b1, b2);          // 0x564720(g, 0, unit, b1, b2)
            }
            break;
        }
        case 0x02: {
            b1 = s->ReadByte() != 0;
            b2 = s->ReadByte() != 0;
            float t[2];
            t[0] = s->ReadFloat();                                // 0x65d4b0
            t[1] = s->ReadFloat();
            ReadGroup(group, s);
            if (group->Leader >= 0) {
                SpeechMove(LiveUnit(group->Leader));              // 0x5bcdf0
                float dx = t[0] - group->X;
                float dz = t[1] - group->Z;
                float d = dz * dz + dx * dx;
                if (d <= group->RadiusSq)
                    gl->MoveFoundUnitsNear(group, 1, t, b1, b2, local);          // 0x57e8b0
                else
                    gl->MoveFoundUnitsToLocation(group, 1, t, b1, b2, local);    // 0x57efd0
            }
            break;
        }
        case 0x03:
        case 0x05: {
            b1 = s->ReadByte() != 0;
            b2 = s->ReadByte() != 0;
            float t[2];
            t[0] = s->ReadFloat();
            t[1] = s->ReadFloat();
            float dir = s->ReadFloat();
            ReadGroup(group, s);
            if (group->Leader >= 0) {
                SpeechMove(LiveUnit(group->Leader));
                if (op == 0x03)
                    gl->MoveFoundUnitsToLocationDir(group, 2, t, dir, b1, b2, local);   // 0x57f200
                else
                    gl->MoveFoundUnitsToLocationDir(group, 4, t, dir, false, b2, local);
            }
            break;
        }
        case 0x04: {
            b1 = s->ReadByte() != 0;
            b2 = s->ReadByte() != 0;
            float t[2];
            t[0] = s->ReadFloat();
            t[1] = s->ReadFloat();
            ReadGroup(group, s);
            if (group->Leader >= 0) {
                SpeechMove(LiveUnit(group->Leader));
                gl->MoveFoundUnitsToLocation(group, 3, t, false, b2, local);   // 0x57efd0(g, 3, t, 0, b2, local)
            }
            break;
        }
        case 0x06: {
            s->ReadByte();                                        // read, not used
            float v = s->ReadFloat();
            ReadGroup(group, s);
            if (group->Leader >= 0) {
                SpeechMove(LiveUnit(group->Leader));
                gl->OrderFloat(group, 5, v, false, false);        // 0x564910(g, 5, v, 0, 0)
            }
            break;
        }
        case 0x07: case 0x0a: case 0x10: case 0x12: case 0x2f: {
            // 0x564720 with the command of the op; no voice.
            b1 = s->ReadByte() != 0;
            b2 = s->ReadByte() != 0;
            int unit = s->ReadInt();
            ReadGroup(group, s);
            int command = op == 0x07 ? 7 : op == 0x0a ? 0xc : op == 0x10 ? 0x12 : op == 0x12 ? 0x14 : 0x2a;
            gl->OrderAtUnit(group, command, unit, b1, b2);
            break;
        }
        case 0x08: case 0x29: case 0x2b: case 0x2c: case 0x35: {
            b2 = s->ReadByte() != 0;
            ReadGroup(group, s);
            int command = op == 0x08 ? 8 : op == 0x29 ? 0x24 : op == 0x2b ? 0x26 : op == 0x2c ? 0x27 : 0x2d;
            gl->OrderPlain(group, command, false, b2);            // 0x564440(g, cmd, 0, b2)
            break;
        }
        case 0x09: case 0x0e: case 0x11: case 0x13: {
            b1 = s->ReadByte() != 0;
            b2 = s->ReadByte() != 0;
            float t[2];
            t[0] = s->ReadFloat();
            t[1] = s->ReadFloat();
            ReadGroup(group, s);
            int command = op == 0x09 ? 9 : op == 0x0e ? 0x10 : op == 0x11 ? 0x13 : 0x15;
            gl->OrderAtPoint(group, command, t, b1, b2);          // 0x564660
            if (op == 0x09 && group->Leader >= 0)
                SpeechMove(LiveUnit(group->Leader));              // 0x5bcdf0 after the order
            else if (op == 0x0e && group->Leader >= 0)
                SpeechGunners(LiveUnit(group->Leader));           // 0x5bca90
            break;
        }
        case 0x0b: case 0x0c: case 0x0d: {
            b2 = s->ReadByte() != 0;
            int unit = s->ReadInt();
            ReadGroup(group, s);
            int command = op == 0x0b ? 0xe : op == 0x0c ? 0xd : 0xf;
            gl->OrderAtUnit(group, command, unit, false, b2);     // 0x564720(g, cmd, unit, 0, b2)
            if (group->Leader >= 0)
                SpeechOrder(LiveUnit(group->Leader));             // LAB_00574165: 0x5bcab0
            break;
        }
        case 0x0f: {
            b1 = s->ReadByte() != 0;
            b2 = s->ReadByte() != 0;
            int unit = s->ReadInt();
            ReadGroup(group, s);
            gl->OrderAtUnit(group, 0x11, unit, b1, b2);
            if (group->Leader >= 0)
                SpeechAttackUnit(LiveUnit(group->Leader), unit);  // 0x5bc9d0(unit)
            break;
        }
        case 0x15: {
            b1 = s->ReadByte() != 0;
            b2 = s->ReadByte() != 0;
            int unit = s->ReadInt();
            ReadGroup(group, s);
            gl->OrderAtUnit(group, 0x16, unit, b1, b2);
            if (group->Leader >= 0)
                SpeechOrder(LiveUnit(group->Leader));
            break;
        }
        case 0x16: case 0x17: case 0x1a: {
            b1 = s->ReadByte() != 0;
            b2 = s->ReadByte() != 0;
            float t[2];
            t[0] = s->ReadFloat();
            t[1] = s->ReadFloat();
            ReadGroup(group, s);
            if (group->Leader >= 0) {
                SpeechOrder(LiveUnit(group->Leader));
                int command = op == 0x16 ? 0x17 : op == 0x17 ? 0x18 : 0x1b;
                gl->MoveFoundUnitsToLocation(group, command, t, false, b2, false);   // 0x57efd0(g, cmd, t, 0, b2, 0)
            }
            break;
        }
        case 0x18: case 0x19: {
            b2 = s->ReadByte() != 0;
            ReadGroup(group, s);
            gl->OrderPlain(group, op == 0x18 ? 0x19 : 0x1a, false, b2);
            if (group->Leader >= 0)
                SpeechOrder(LiveUnit(group->Leader));
            break;
        }
        case 0x1b: {
            int which = s->ReadInt();
            bool b = s->ReadByte() != 0;
            ReadGroup(group, s);
            if (group->Leader >= 0) {
                SpeechOrder(LiveUnit(group->Leader));
                gl->OrderValue(group, which == 0 ? 0x1c : 0x1d, b, false, false);   // 0x564870(g, cmd, b, 0, 0)
            }
            break;
        }
        case 0x1c: {
            float x = s->ReadFloat();
            float z = s->ReadFloat();
            Support_5674c0(x, z, player);                         // 0x5674c0(0, x, z, player)
            break;
        }
        case 0x1d: {
            float x = s->ReadFloat();
            float z = s->ReadFloat();
            Support_568300(x, z, player);                         // 0x568300(0, x, z, player, -1.0, 1)
            break;
        }
        case 0x1e: {
            float x = s->ReadFloat();
            float z = s->ReadFloat();
            Support_568740(x, z, player);                         // 0x568740(0, x, z, player)
            break;
        }
        case 0x1f:
        case 0x20: {
            float dir = s->ReadFloat();
            float flag = (float)s->ReadInt();                     // CVTDQ2PS of the raw i32
            float x = s->ReadFloat();
            float z = s->ReadFloat();
            bool f = !(flag == 0.0f);                             // UCOMISS [0x7f1038] (0.0), LAHF / TEST AH,0x44
            if (op == 0x1f)
                Support_567760(x, z, player, f, dir);             // 0x567760(0, x, z, player, -1.0, 1, f, dir)
            else
                Support_567d40(x, z, player, f, dir);             // 0x567d40(0, x, z, player, -1.0, 1, f, dir)
            break;
        }
        case 0x21: case 0x22: {
            b2 = s->ReadByte() != 0;
            float t[2];
            t[0] = s->ReadFloat();
            t[1] = s->ReadFloat();
            ReadGroup(group, s);
            if (group->Leader >= 0) {
                SpeechOrder(LiveUnit(group->Leader));
                gl->MoveFoundUnitsToLocation(group, 0x1f, t, false, b2, false);
            }
            break;
        }
        case 0x23:
            PlaceArmyFromPacket(s, player);                       // 0x51f860 + PlaceUnits 0x572ec0(player, army)
            break;
        case 0x24:
            break;
        case 0x25: {
            unsigned char latency = s->ReadByte();
            if (g_Campaign)
                ((unsigned char*)g_Campaign)[0xb80] = latency;    // campaign +0xb80
            // HD formats "Setting network latency to %dms" (latency * 200)
            // into a temporary string and drops it (0x51ee20 / 0x51e0f0).
            break;
        }
        case 0x26: {
            unsigned value = s->ReadByte();
            ReadGroup(group, s);
            gl->OrderValue(group, 0x21, (int)value, false, false);
            break;
        }
        case 0x27: {
            b2 = s->ReadByte() != 0;
            float t[2];
            t[0] = s->ReadFloat();
            t[1] = s->ReadFloat();
            ReadGroup(group, s);
            gl->OrderAtPoint(group, 0x22, t, false, b2);
            break;
        }
        case 0x28: {
            b2 = s->ReadByte() != 0;
            int unit = s->ReadInt();
            ReadGroup(group, s);
            gl->OrderAtUnit(group, 0x23, unit, false, b2);
            break;
        }
        case 0x2a: {
            b2 = s->ReadByte() != 0;
            float v = s->ReadFloat();
            ReadGroup(group, s);
            gl->OrderFloat(group, 0x25, v, false, b2);
            break;
        }
        case 0x2d: {
            b2 = s->ReadByte() != 0;
            unsigned value = s->ReadByte();
            ReadGroup(group, s);
            // Squads only (SPUnit +0x40 class 5): 0x5bbb60(0x28, value, 0, b2).
            for (int j = 0; j < group->Count; ++j) {
                SUnit* u = LiveUnit(FoundAt(group, j));
                if (*(int*)(((unsigned char*)u->Proto) + 0x40) == 5)
                    u->OrderParam(0x28, (int)value, false, b2);
            }
            break;
        }
        case 0x2e: case 0x31: {
            b2 = s->ReadByte() != 0;
            unsigned value = s->ReadByte();
            int v = (int)value;
            if (op == 0x31 && value == 0xff)
                v = -1;
            ReadGroup(group, s);
            gl->OrderValue(group, op == 0x2e ? 0x29 : 0x2b, v, false, b2);   // 0x564870(g, cmd, v, 0, b2)
            break;
        }
        case 0x32:
        case 0x33: {
            int unit = (short)s->ReadWord();                      // MOVSX
            SWorld* w = g_World;
            if (unit >= 0 && unit < w->Units.Size && w->Units.Array[unit].Next == kHeapLive) {   // 0x573790
                SUnit* u = LiveUnit(unit);
                // During playback the replayed player's selection is shown locally.
                if (gl->PlaybackStream && player == w->LocalPlayer)
                    u->_104 = op == 0x32 ? 1 : 0;
                if (op == 0x32) {
                    u->_108 |= 1u << (player & 0x1f);
                    UnitSelectedBy(u, player);                    // +0x148(player)
                } else {
                    u->_108 &= ~(1u << (player & 0x1f));
                    UnitDeselectedBy(u, player);                  // +0x14c(player)
                }
            }
            break;
        }
        case 0x34: {
            b2 = s->ReadByte() != 0;
            int unit = s->ReadInt();
            ReadGroup(group, s);
            gl->OrderAtUnit(group, 0x2c, unit, b1, b2);           // b1 of an earlier record (local_3c)
            break;
        }
        case 0x36: case 0x37: {
            int value = s->ReadInt();
            ReadGroup(group, s);
            gl->OrderValue(group, op == 0x36 ? 0x2e : 0x2f, value, false, false);
            break;
        }
        default:
            Logger.g->Panic("SGameLogic::ProcessPacket: Invalid command packet");
        }
        op = s->ReadByte();
    }
}

} // namespace

// PANZERS 0x5737c0
// One frame of one player (single player: the local frame stream, called by
// Refresh 0x576d80 after it ended the frame with a 0 byte). -packetrec
// appends the frame to the recording; -packetplay replaces it with the
// recorded one and restores Running (+0x04). Then the CRC at the start of
// the frame must equal the oldest CRC in CrcHistory, and the records run.
// The whole body is one try block: a read error (the end of the recording)
// closes the playback, sets the campaign's +0xe4 and replaces the frame by a
// bare CRC, which then passes the check (0x57391d, resumes at 0x5739ac).
void SGameLogic::ProcessPacket(int player, SStream** frame)
{
    PZ_M2_TRACE("SGameLogic::ProcessPacket (0x5737c0)");
    SFoundUnits group;                                            // local_60
    memset(&group, 0, sizeof(group));
    bool head = true;
    for (;;) {
        try {
            if (head) {
                if (RecordStream) {
                    unsigned camera[5];
                    g_World->GetCameraState(camera);              // 0x5e6a70
                    WriteRecordedFrame(RecordStream, *frame, Running, camera);
                }
                if (PlaybackStream) {
                    if (*frame) {
                        delete (SStreamBuffer*)*frame;            // vtbl +0 (1)
                        *frame = nullptr;
                    }
                    *frame = new SStreamBuffer();                 // new 0x30, 0x65cd20
                    unsigned camera[5];
                    ReadRecordedFrame(PlaybackStream, *frame, &Running, camera);   // camera read, not applied
                }
            }
            SStream* s = *frame;
            s->Seek(0, 0);                                        // 0x65d790
            unsigned crc = (unsigned)s->ReadInt();
            if (CrcTop < CrcBottom)                               // SDEQueue::operator[] (CMP ESI,ESI is never less)
                Logger.g->Panic("SDEQueue<%s>::operator[]: invalid index (%d)", "unsigned int", CrcBottom);
            int idx = CrcBottom - CrcBase;
            if (CrcMax <= idx)
                idx -= CrcMax;
            if (crc == CrcHistory[idx]) {
                ApplyRecords(this, player, s, &group);
            } else {
                Logger.g->Log(0, "Inconsistency in frame %d with player %d.", FramesSent, player);
                // HD: with SMulti and its +0x4f3c flag, 0x51f7f0(player).
            }
            break;
        } catch (const char* e) {
            // PANZERS 0x57391d
            head = false;
            if (PlaybackStream) {
                PlaybackStream->Release();                            // vtbl +0 (1)
                PlaybackStream = nullptr;
            }
            // HD: Timer 0x6616e0(0) (frame recording off; the recompile
            // timer has none), then formats "Packet playing finished with
            // error: %s" into a temporary string it drops.
            Logger.g->Log(0, "PZM3: Packet playing finished with error: %s", e);
            if (g_Campaign)
                g_Campaign->MissionResult = 1;                    // +0xe4 (F: 1 = victory, ends the mission)
            SStream* s = *frame;
            s->Seek(0, 0);
            if (CrcTop < CrcBottom)                               // 0x560820(CrcBottom): bottom <= i <= top
                Logger.g->Panic("SDEQueue<%s>::operator[]: invalid index (%d)", "unsigned int", CrcBottom);
            int idx = CrcBottom - CrcBase;
            if (CrcMax <= idx)
                idx -= CrcMax;
            s->WriteInt((int)CrcHistory[idx]);
            s->WriteByte(0);
        }
    }
    if (group.Units)
        free(group.Units);
}

// StartPacketRecording 0x5805c0 / StartPacketPlayback 0x580540: gamelogic_mission.cpp (agent F).

// PANZERS 0x56d2a0
// The relation of a unit to a player: 1 own, -1 enemy, 0 allied or neutral,
// 3 not to be ordered (an AI-less or passive player's unit, +0x110, or a
// building held by the player, +0x2cc). Not a unit: 0.
int GetUnitRelation(int unit, int player)
{
    SWorld* w = g_World;
    if (!w || !w->Units.IsLive(unit))
        return 0;
    SUnit* u = WorldUnit(unit);
    auto sameSide = [w](int a, int b) {                           // 0x549ab0
        int team = *(int*)(w->Players[a] + 0x0c);
        if (team != 0)
            return team == *(int*)(w->Players[b] + 0x0c);
        return a == b;
    };
    if (u->Proto->ClassType == 9) {
        if (u->_2cc[player])
            return 3;
        if (u->Player != player)
            return sameSide(player, u->Player) ? 0 : -1;
        return 1;
    }
    int kind = *(int*)(w->Players[u->Player] + 0x08);             // World+0x178 + p * 0x48
    if (kind == 3 || kind == 2 || u->_110)
        return 3;
    if (u->Player != player) {
        if (!sameSide(player, u->Player) && !u->_2cc[player])
            return -1;
        return 0;
    }
    return 1;
}

// PANZERS 0x56d280
int GetUnitRelationToLocal(int unit)
{
    return GetUnitRelation(unit, g_World ? g_World->LocalPlayer : 0);
}

// Kept for callers of the M3-P0 skeleton: the record switch of one frame.
void ApplyPacketRecords(SGameLogic* gl, int player, SStream* frame)
{
    SFoundUnits group;
    memset(&group, 0, sizeof(group));
    ApplyRecords(gl, player, frame, &group);
    if (group.Units)
        free(group.Units);
}

} // namespace pz
