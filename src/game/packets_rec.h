// src/game/packets_rec.h
// The -packetrec / -packetplay file format (docs/M3_REPLAY.md) and a
// world-independent record codec. OWNER: agent O.
//
// File (HD, all little-endian):
//   u8  version 3            StartPacketRecording 0x5805c0 / StartPacketPlayback 0x580540
//   replay header            SPanzersCampaign::WriteReplayHeader 0x596fc0: the Stormregion
//                            signature (0x1b1a7253, 0x0a870a0d), then the chunk 'SAVE'
//                            {u32 size, 'v4pa', 3, map, mode, race, prestige, army, ...}
//   per logic frame and player (SGameLogic::ProcessPacket 0x5737c0):
//     i32 size               of the frame stream below
//     i32 SGameLogic+0x04    Running (game speed; 0 = paused)
//     size bytes             the frame stream: i32 CRC, records (u8 op, fields), u8 0
//     20 bytes               SWorld::GetCameraState 0x5e6a70 (playback reads and ignores them)
//
// The record fields are listed in kPzPacketLayout (one character per field,
// in stream order). ProcessPacket (packets.cpp) reads them case by case as
// HD does; the table is the same knowledge for tools (the .rec dumper).

#ifndef PZ_PACKETS_REC_H
#define PZ_PACKETS_REC_H

struct SStream;

namespace pz {

// Field codes: 'b' u8 bool, 'B' u8 value, 'i' i32, 'f' f32, 'w' u16 unit,
// 'u' the selected-unit list (u16 heap indices, u16 0xffff), 'A' the army
// records of op 0x23 (not decoded: the dumper stops at it).
// nullptr = no ProcessPacket case (op 0x14, 0x30, > 0x37).
extern const char* const kPzPacketLayout[0x38];
// Short names for the dump ("move", "select", ...).
extern const char* const kPzPacketName[0x38];

// HD 0x65d000 (SStream::CopyTo): copies `size` bytes from `from` to `to`
// through a 0x4000-byte buffer; size < 0 copies until ReadMax returns 0.
void CopyStreamBytes(SStream* from, SStream* to, int size);

// One recorded frame of a .rec file (the per-frame block above).
// Reader: ProcessPacket's playback branch (0x5737c0 at 0x5738e4); throws the
// SStream "Read failed" string at the end of the file, as HD does.
void ReadRecordedFrame(SStream* file, SStream* frame, int* running, unsigned camera[5]);
// Writer: ProcessPacket's recording branch (0x573839).
void WriteRecordedFrame(SStream* file, SStream* frame, int running, const unsigned camera[5]);

// Recompile tool (no HD counterpart): parses a .rec file with the readers
// above and the record table, writes one text line per frame with records
// (and a summary per op) to `textOut`, and re-encodes every field into
// `recOut` (the header chunk is copied as bytes). Returns 0 when every
// frame decoded, 1 otherwise. Either output may be null.
int PzRecDump(const char* recIn, const char* textOut, const char* recOut);

} // namespace pz

#endif // PZ_PACKETS_REC_H
