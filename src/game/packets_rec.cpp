// src/game/packets_rec.cpp
// The .rec frame reader / writer and the record codec (packets_rec.h).
// OWNER: agent O. Depends on the core streams only, so the recdump tool
// (tools/recdump) links it without the game.

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "packets_rec.h"
#include "stream.h"

namespace pz {

// Field layout of every ProcessPacket 0x5739b8 case, in read order.
const char* const kPzPacketLayout[0x38] = {
    "",        // 0x00 end of the frame
    "bbiu",    // 0x01
    "bbffu",   // 0x02
    "bbfffu",  // 0x03
    "bbffu",   // 0x04
    "bbfffu",  // 0x05
    "bfu",     // 0x06 (the byte is read and ignored)
    "bbiu",    // 0x07
    "bu",      // 0x08
    "bbffu",   // 0x09
    "bbiu",    // 0x0a
    "biu",     // 0x0b
    "biu",     // 0x0c
    "biu",     // 0x0d
    "bbffu",   // 0x0e
    "bbiu",    // 0x0f
    "bbiu",    // 0x10
    "bbffu",   // 0x11
    "bbiu",    // 0x12
    "bbffu",   // 0x13
    nullptr,   // 0x14 no case: "Invalid command packet"
    "bbiu",    // 0x15
    "bbffu",   // 0x16
    "bbffu",   // 0x17
    "bu",      // 0x18
    "bu",      // 0x19
    "bbffu",   // 0x1a
    "ibu",     // 0x1b
    "ff",      // 0x1c
    "ff",      // 0x1d
    "ff",      // 0x1e
    "fiff",    // 0x1f (the builder writes the 2nd field as a float, the reader reads an int)
    "fiff",    // 0x20 (as 0x1f)
    "bffu",    // 0x21
    "bffu",    // 0x22 (HD reads B, x, z; its builder 0x5759c0 writes B, i32: an HD mismatch)
    "A",       // 0x23 army records (0x51f860)
    "",        // 0x24 no-op
    "B",       // 0x25 latency
    "Bu",      // 0x26
    "bffu",    // 0x27
    "biu",     // 0x28
    "bu",      // 0x29
    "bfu",     // 0x2a
    "bu",      // 0x2b
    "bu",      // 0x2c
    "bBu",     // 0x2d
    "bBu",     // 0x2e
    "bbiu",    // 0x2f
    nullptr,   // 0x30 builder 0x575d20 exists, no case: "Invalid command packet"
    "bBu",     // 0x31
    "w",       // 0x32 select unit
    "w",       // 0x33 deselect unit
    "biu",     // 0x34
    "bu",      // 0x35
    "iu",      // 0x36
    "iu",      // 0x37
};

const char* const kPzPacketName[0x38] = {
    "end", "target_unit", "move", "move_dir", "move_back", "move_back_dir", "cmd05", "cmd07",
    "cmd08", "cmd09", "cmd0c", "cmd0e", "cmd0d", "cmd0f", "cmd10", "cmd11",
    "cmd12", "cmd13", "cmd14", "cmd15", "-", "cmd16", "cmd17", "cmd18",
    "cmd19", "cmd1a", "cmd1b", "cmd1c_1d", "support_5674c0", "support_568300", "support_568740", "support_567760",
    "support_567d40", "cmd1f_pos", "cmd1f_22", "army", "nop", "latency", "cmd21", "cmd22",
    "cmd23", "cmd24", "cmd25", "cmd26", "cmd27", "cmd28", "cmd29", "cmd2a",
    "-", "cmd2b", "select", "deselect", "cmd2c", "cmd2d", "cmd2e", "cmd2f",
};

static unsigned char s_copyBuffer[0x4000];   // HD DAT_00929f30

// PANZERS 0x65d000
void CopyStreamBytes(SStream* from, SStream* to, int size)
{
    if (size < 0) {
        int n = from->ReadMax(s_copyBuffer, 0x4000);              // vtbl +0x08
        while (n != 0) {
            to->Write(s_copyBuffer, n);                           // vtbl +0x0c
            n = from->ReadMax(s_copyBuffer, 0x4000);
        }
        return;
    }
    while (size != 0) {
        int n = size > 0x4000 ? 0x4000 : size;
        from->Read(s_copyBuffer, n);                              // vtbl +0x04
        to->Write(s_copyBuffer, n);
        size -= n;
    }
}

// PANZERS 0x5737c0 (playback part, 0x5738e4..0x573918)
void ReadRecordedFrame(SStream* file, SStream* frame, int* running, unsigned camera[5])
{
    int size = file->ReadInt();                                   // 0x65d4d0
    *running = file->ReadInt();
    CopyStreamBytes(file, frame, size);                           // 0x65d000(frame, size)
    file->Read(camera, 0x14);                                     // vtbl +0x04 (local, not applied)
}

// PANZERS 0x5737c0 (recording part, 0x573839..0x573892)
void WriteRecordedFrame(SStream* file, SStream* frame, int running, const unsigned camera[5])
{
    int size = frame->Seek(0, 2);                                 // vtbl +0x10 (0, 2): the frame size
    file->WriteInt(size);                                         // 0x65dc40
    file->WriteInt(running);
    frame->Seek(0, 0);                                            // 0x65d790: position = 0
    CopyStreamBytes(frame, file, size);
    file->Write(camera, 0x14);                                    // vtbl +0x0c
}

// ---------------------------------------------------------------------------
// The dumper (recompile tool).

namespace {

struct SOpStat {
    int Count;
    int FirstFrame;
};

void AppendF(char* buf, size_t cap, const char* fmt, ...)
{
    size_t n = strlen(buf);
    if (n + 1 >= cap)
        return;
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf + n, cap - n, fmt, ap);
    va_end(ap);
}

// Decodes the records of one frame stream (positioned after the CRC) into
// `text` and re-encodes them into `out`. Returns false on a layout error.
bool DecodeRecords(SStreamBuffer* in, SStream* out, char* text, size_t cap, SOpStat* stats, int frame)
{
    for (;;) {
        unsigned char op = in->ReadByte();
        out->WriteByte(op);
        if (op == 0)
            return in->getSeek() == in->getSize();
        if (op >= 0x38 || !kPzPacketLayout[op]) {
            AppendF(text, cap, " [bad op %02x]", op);
            return false;
        }
        if (stats[op].Count++ == 0)
            stats[op].FirstFrame = frame;
        AppendF(text, cap, " %02x %s(", op, kPzPacketName[op]);
        for (const char* f = kPzPacketLayout[op]; *f; ++f) {
            if (f != kPzPacketLayout[op] && *f != 'u')
                AppendF(text, cap, ",");
            switch (*f) {
            case 'b': {
                unsigned char v = in->ReadByte();
                out->WriteByte(v);
                AppendF(text, cap, "%s", v ? (v == 1 ? "1" : "?") : "0");
                break;
            }
            case 'B': {
                unsigned char v = in->ReadByte();
                out->WriteByte(v);
                AppendF(text, cap, "%u", v);
                break;
            }
            case 'i': {
                int v = in->ReadInt();
                out->WriteInt(v);
                AppendF(text, cap, "%d", v);
                break;
            }
            case 'f': {
                float v = in->ReadFloat();
                out->WriteFloat(v);
                AppendF(text, cap, "%.4f", v);
                break;
            }
            case 'w': {
                unsigned short v = in->ReadWord();
                out->WriteWord(v);
                AppendF(text, cap, "unit %d", v);
                break;
            }
            case 'u': {
                AppendF(text, cap, ") units[");
                bool first = true;
                for (;;) {
                    unsigned short v = in->ReadWord();
                    out->WriteWord(v);
                    if (v == 0xffff)
                        break;
                    AppendF(text, cap, first ? "%d" : " %d", v);
                    first = false;
                }
                AppendF(text, cap, "]");
                break;
            }
            default:   // 'A': not decoded
                AppendF(text, cap, " [army records not decoded]");
                return false;
            }
        }
        if (!strchr(kPzPacketLayout[op], 'u'))
            AppendF(text, cap, ")");
    }
}

SStreamBuffer* LoadFile(const char* path)
{
    FILE* f = fopen(path, "rb");
    if (!f)
        return nullptr;
    SStreamBuffer* b = new SStreamBuffer();
    unsigned char buf[0x4000];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        b->Write(buf, (int)n);
    fclose(f);
    b->Seek(0, 0);
    return b;
}

} // namespace

int PzRecDump(const char* recIn, const char* textOut, const char* recOut)
{
    SStreamBuffer* in = LoadFile(recIn);
    if (!in) {
        fprintf(stderr, "recdump: cannot read %s\n", recIn);
        return 1;
    }
    FILE* txt = textOut ? fopen(textOut, "w") : nullptr;
    SStreamBuffer out;
    int result = 0;
    SOpStat stats[0x38];
    memset(stats, 0, sizeof(stats));
    int frames = 0, paused = 0, logicFrame = 0, withRecords = 0;
    try {
        // StartPacketPlayback 0x580540: the version byte, then ReadReplayHeader
        // 0x595780. The header is copied as bytes: signature (8) + chunk 'SAVE'.
        unsigned char version = in->ReadByte();
        out.WriteByte(version);
        unsigned head[3];
        in->Read(head, 4);
        if (head[0] == 0x1b1a7253) {                              // the Stormregion signature
            in->Read(&head[1], 8);                                // 0x0a870a0d, 'SAVE'
            unsigned chunkSize = in->ReadInt();
            out.Write(head, 12);
            out.WriteInt((int)chunkSize);
            CopyStreamBytes(in, &out, (int)chunkSize);
            if (txt)
                fprintf(txt, "# %s: version %u, header chunk %08x size %u\n", recIn, version, head[2], chunkSize);
        } else {
            // A recompile recording made while the replay header (agent F) was a stub.
            in->Seek(1, 0);
            if (txt)
                fprintf(txt, "# %s: version %u, no replay header\n", recIn, version);
        }
        while (in->getSeek() < in->getSize()) {
            SStreamBuffer frame;
            int running = 0;
            unsigned camera[5];
            ReadRecordedFrame(in, &frame, &running, camera);
            frame.Seek(0, 0);
            // Re-encode: the same block, field by field.
            SStreamBuffer reframe;
            unsigned crc = (unsigned)frame.ReadInt();
            reframe.WriteInt((int)crc);
            char line[4096];
            line[0] = 0;
            bool ok = DecodeRecords(&frame, &reframe, line, sizeof(line), stats, logicFrame);
            if (!ok)
                result = 1;
            WriteRecordedFrame(&out, &reframe, running, camera);
            // "frame" = SGameLogic +0x08 when ProcessPacket applies the records,
            // i.e. the first "PZM2 CRC <frame>" line (BeginFrame, right after)
            // that sees them. The record carries the CRC of the frame before
            // (record 0: the map-load CRC).
            if (txt && (line[0] || frames == 0 || !ok))
                fprintf(txt, "rec %d frame %d speed %d crc %08x%s cam %.2f %.2f%s\n", frames, logicFrame, running, crc,
                        line, *(float*)&camera[0], *(float*)&camera[1], ok ? "" : " DECODE-ERROR");
            if (line[0])
                ++withRecords;
            if (running == 0)
                ++paused;
            else
                ++logicFrame;
            ++frames;
        }
    } catch (const char* e) {
        fprintf(stderr, "recdump: %s at offset %d\n", e, in->getSeek());
        result = 1;
    }
    if (txt) {
        fprintf(txt, "# %d recorded frames (%d paused), last logic frame %d, %d with records\n", frames, paused,
                logicFrame, withRecords);
        for (int op = 1; op < 0x38; ++op)
            if (stats[op].Count)
                fprintf(txt, "# op %02x %-16s %4d records, first at frame %d\n", op, kPzPacketName[op],
                        stats[op].Count, stats[op].FirstFrame);
        fclose(txt);
    }
    if (recOut) {
        FILE* f = fopen(recOut, "wb");
        if (f) {
            int size = out.getSize();
            out.Seek(0, 0);
            unsigned char buf[0x4000];
            while (size > 0) {
                int n = size > 0x4000 ? 0x4000 : size;
                out.Read(buf, n);
                fwrite(buf, 1, n, f);
                size -= n;
            }
            fclose(f);
        } else {
            result = 1;
        }
    }
    delete in;
    return result;
}

} // namespace pz
