// src/game/gamelogic_save.cpp
// The game state of the save game: SGameLogic::SaveGameState 0x57e110 and
// its logic-side chunks (running triggers, movement groups, animated
// models, echoes, counters, variables, the minimap objectives), and the
// reverse SGameLogic::LoadGameState 0x56eb50. The world chunks are in
// world_save.cpp. OWNER: agent S (M4, docs/M4_STATUS.md, docs/FORMATS.md).

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "gamelogic.h"
#include "world.h"
#include "world_save.h"
#include "worldapi.h"
#include "unitsave.h"
#include "trigger.h"
#include "stream.h"
#include "logger.h"
#include "m3common.h"
#include "doodad.h"
#include "aigroup.h"
#include "unit.h"
#include "campaign.h"
#include "iboard.h"
extern SIBoard* Board;   // HD DAT_008f1c60

namespace pz {

void RemoveObjectiveMarkers(SGameLogic* gl, int objective, int target);   // 0x579420 (triggers.cpp)

static inline int RI(const void* b, int off) { return *(const int*)((const unsigned char*)b + off); }
static inline float RF(const void* b, int off) { return *(const float*)((const unsigned char*)b + off); }
static inline unsigned char RB(const void* b, int off) { return *((const unsigned char*)b + off); }

// PANZERS 0x57d9c0 (RTRG, +0x268 SDArray<SRunningTrigger>; 0x57daa0 per
// found-unit list)
static void SaveRunningTriggers(SGameLogic* g, SStream* s)
{
    s->WriteInt(g->RunningTriggerCount);
    for (int i = 0; i < g->RunningTriggerCount; ++i) {
        const SRunningTrigger* r = &g->RunningTriggers[i];
        s->WriteInt(r->Trigger);
        s->WriteInt(r->Action);
        s->WriteInt(r->Wait);
        s->WriteInt(r->Unit);
        s->WriteInt(r->Unit2);
        s->WriteFloat(r->Found.X);
        s->WriteFloat(r->Found.Z);
        s->WriteFloat(r->Found.RadiusSq);
        s->WriteInt(r->Found.Kind);
        s->WriteInt(r->Found.Leader);
        // PANZERS 0x57daa0
        s->WriteInt(r->Found.Count);
        for (int k = 0; k < r->Found.Count; ++k) {
            const SFoundUnit* f = &r->Found.Units[k];
            s->WriteInt((unsigned char)f->Flag);
            s->WriteFloat(*(const float*)&f->_04);
            s->WriteInt(f->Unit);
        }
    }
}

// PANZERS 0x57db90 (MGRP, +0x2dc SHeap<SMovementGroup>, element 0x28)
static void SaveMovementGroups(SGameLogic* g, SStream* s)
{
    const unsigned char* h = (const unsigned char*)g + 0x2dc;
    const unsigned char* a = *(unsigned char* const*)h;
    int n = RI(h, 4);
    s->WriteInt(n);
    s->WriteInt(RI(h, 0xc));
    s->WriteInt(RI(h, 0x10));
    for (int i = 0; i < n; ++i) {
        const unsigned char* e = a + i * 0x28;
        s->WriteInt(RI(e, 0));
        if (RI(e, 0) != kHeapLive)
            continue;
        const unsigned char* m = *(unsigned char* const*)(e + 4);
        int c = RI(e, 8);
        s->WriteInt(c);
        for (int k = 0; k < c; ++k) {
            s->WriteInt(RI(m, k * 0x10));
            s->WriteFloat(RF(m, k * 0x10 + 4));
            s->WriteFloat(RF(m, k * 0x10 + 8));
            s->WriteFloat(RF(m, k * 0x10 + 0xc));
        }
        s->WriteFloat(RF(e, 0x10));
        s->WriteInt(RI(e, 0x14));
        s->WriteInt(RI(e, 0x18));
        s->WriteInt(RI(e, 0x1c));
        s->WriteFloat(RF(e, 0x20));
        s->WriteByte(RB(e, 0x24));
        s->WriteByte(RB(e, 0x25));
    }
}

// PANZERS 0x57d8c0 (AMOD, +0x2fc SDArray<SAnimatedModel>, 0x24 each)
static void SaveAnimatedModels(SGameLogic* g, SStream* s)
{
    s->WriteInt(g->AnimatedModelCount);
    for (int i = 0; i < g->AnimatedModelCount; ++i) {
        const unsigned char* e = (const unsigned char*)g->AnimatedModels + i * 0x24;
        SaveWorldString(s, e);
        s->WriteFloat(RF(e, 0x08));
        s->WriteFloat(RF(e, 0x0c));
        s->WriteFloat(RF(e, 0x10));
        s->WriteFloat(RF(e, 0x14));
        s->WriteFloat(RF(e, 0x1c));
    }
}

// PANZERS 0x57e4f0 (ECHO: +0x84 count, +0x88 board handles, written as the
// names the board gives them, board +0x38). The recompile's board does not
// keep echoes, so the count is 0.
static void SaveEchoes(SGameLogic* g, SStream* s)
{
    int n = RI(g, 0x84);
    s->WriteInt(n);
    for (int i = 0; i < n; ++i)
        s->WriteString("");
}

// PANZERS 0x57e110
void SGameLogic::SaveGameState(SStream* s)
{
    PZ_M3_TRACE("SGameLogic::SaveGameState (0x57e110)");
    SWorld* w = g_World;
    s->WriteChunkStart(0x33594c50);                               // PLY3
    SaveWorldPlayers(w, s);                                       // 0x5fb160
    s->WriteChunkEnd();
    s->WriteChunkStart(0x50474941);                               // AIGP
    SaveWorldAIGroups(w, s);                                      // 0x57db20
    s->WriteChunkEnd();
    s->WriteChunkStart(0x53494e55);                               // UNIS
    SaveWorldUnits(w, s);                                         // 0x5fb630
    s->WriteChunkEnd();
    s->WriteChunkStart(0x53464545);                               // EEFS
    SaveWorldEffects(w, s);                                       // 0x5fb030 -> 0x5f9450
    s->WriteChunkEnd();
    s->WriteChunkStart(0x204d4143);                               // "CAM "
    SaveWorldCamera(w, s);                                        // 0x5e6a70
    s->WriteChunkEnd();
    s->WriteChunkStart(0x53434f4c);                               // LOCS
    SaveWorldLocations(w, s);                                     // 0x5fb140 -> 0x5f9210
    s->WriteChunkEnd();
    if (RI(w, 0x7478) != 0) {
        s->WriteChunkStart(0x47495254);                           // TRIG
        SaveWorldTriggers(w, s);                                  // 0x5fb430 -> 0x5f8a80
        s->WriteChunkEnd();
    }
    s->WriteChunkStart(0x47525452);                               // RTRG
    SaveRunningTriggers(this, s);                                 // 0x57d9c0
    s->WriteChunkEnd();
    s->WriteChunkStart(0x52415654);                               // TVAR
    SaveWorldTriggerVariables(w, s);                              // 0x57dd60
    s->WriteChunkEnd();
    s->WriteChunkStart(0x4f484345);                               // ECHO
    SaveEchoes(this, s);                                          // 0x57e4f0
    s->WriteChunkEnd();
    s->WriteChunkStart(0x52544e43);                               // CNTR
    s->WriteInt(RB(this, 0x14c));
    s->WriteInt(RB(this, 0x14d));
    s->WriteInt(RI(this, 0x174));
    s->WriteInt(RI(this, 0x178));
    s->WriteChunkEnd();
    s->WriteChunkStart(0x53524156);                               // VARS
    SaveVariables(s, this, kLogicVarsDesc);                       // 0x8dba58
    s->WriteChunkEnd();
    s->WriteChunkStart(0x44454553);                               // SEED
    s->WriteInt((int)w->RandomSeed);                              // World+0x7518
    s->WriteChunkEnd();
    s->WriteChunkStart(0x4444444f);                               // ODDD
    SaveWorldDoodadAnims(w, s);                                   // 0x5fb020 -> 0x5f8cf0
    s->WriteChunkEnd();
    s->WriteChunkStart(0x33524957);                               // WIR3
    SaveWorldWires(w, s);                                         // 0x5fb7d0(s, 1)
    s->WriteChunkEnd();
    s->WriteChunkStart(0x5052474d);                               // MGRP
    SaveMovementGroups(this, s);                                  // 0x57db90
    s->WriteChunkEnd();
    s->WriteChunkStart(0x5052474d);                               // MGRP (HD writes it twice)
    SaveMovementGroups(this, s);
    s->WriteChunkEnd();
    s->WriteChunkStart(0x444f4d41);                               // AMOD
    SaveAnimatedModels(this, s);                                  // 0x57d8c0
    s->WriteChunkEnd();
    s->WriteChunkStart(0x52485457);                               // WTHR
    SaveWorldWeather(w, s);                                       // 0x5fb770
    s->WriteChunkEnd();
    s->WriteChunkStart(0x544a424f);                               // OBJT
    int n = RI(this, 0x198);                                      // minimap objectives SDArray +0x194 (0x14 each)
    s->WriteInt(n);
    const unsigned char* o = *(unsigned char* const*)((unsigned char*)this + 0x194);
    for (int i = 0; i < n; ++i) {
        s->WriteFloat(RF(o, i * 0x14));
        s->WriteFloat(RF(o, i * 0x14 + 4));
        s->WriteInt(RI(o, i * 0x14 + 0xc));
        s->WriteInt(RI(o, i * 0x14 + 0x10));
    }
    s->WriteChunkEnd();
}

// ---------------------------------------------------------------------------
// Load

static void ReadLogicStr(SStream* s, SString* out)
{
    FreeSString(out);
    int n = s->ReadWord() & 0xffff;
    if (n == 0)
        return;
    out->size = n;
    out->buf = new char[n + 1];
    s->Read(out->buf, n);
    out->buf[n] = 0;
}

static unsigned ReadArraySize(SStream* s)
{
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        throw "Invalid array size";
    return n;
}

// Grow an HD SDArray {array, size, max} to n elements (zeroed).
static void* ResizeRaw(void* arr, int elemSize, int n)
{
    int* a = (int*)arr;
    if (a[2] < n) {
        a[0] = (int)(size_t)realloc((void*)(size_t)a[0], (size_t)n * elemSize);
        a[2] = n;
    }
    if (a[0] && a[2] > 0)
        memset((void*)(size_t)a[0], 0, (size_t)a[2] * elemSize);
    a[1] = n;
    return (void*)(size_t)a[0];
}

// PANZERS 0x56e1d0 (RTRG; 0x56e750 per element, 0x56e6b0 for its found units)
static void LoadRunningTriggers(SGameLogic* g, SStream* s)
{
    unsigned n = ReadArraySize(s);
    for (int i = 0; i < g->RunningTriggerCount; ++i)
        free(g->RunningTriggers[i].Found.Units);
    ResizeRaw(&g->RunningTriggers, sizeof(SRunningTrigger), (int)n);   // 0x563410
    for (int i = 0; i < (int)n; ++i) {
        SRunningTrigger* r = &g->RunningTriggers[i];
        r->Trigger = s->ReadInt();
        r->Action = s->ReadInt();
        r->Wait = s->ReadInt();
        r->Unit = s->ReadInt();
        r->Unit2 = s->ReadInt();
        r->Found.X = s->ReadFloat();
        r->Found.Z = s->ReadFloat();
        r->Found.RadiusSq = s->ReadFloat();
        r->Found.Kind = s->ReadInt();
        r->Found.Leader = s->ReadInt();
        unsigned c = ReadArraySize(s);
        ResizeRaw(&r->Found.Units, sizeof(SFoundUnit), (int)c);
        for (int k = 0; k < (int)c; ++k) {
            SFoundUnit* f = &r->Found.Units[k];
            f->Flag = s->ReadInt() != 0;
            *(float*)&f->_04 = s->ReadFloat();
            f->Unit = s->ReadInt();
        }
    }
}

// PANZERS 0x56e380 (MGRP; 0x563770 clears, 0x56e620 per live group)
static void LoadMovementGroups(SGameLogic* g, SStream* s)
{
    unsigned char* h = (unsigned char*)g + 0x2dc;
    unsigned char* a = *(unsigned char**)h;
    for (int i = 0; i < RI(h, 4); ++i)
        if (RI(a, i * 0x28) == kHeapLive)
            free(*(void**)(a + i * 0x28 + 4));
    unsigned n = ReadArraySize(s);
    a = (unsigned char*)ResizeRaw(h, 0x28, (int)n);
    *(int*)(h + 0xc) = s->ReadInt();
    *(int*)(h + 0x10) = s->ReadInt();
    for (int i = 0; i < (int)n; ++i) {
        unsigned char* e = a + i * 0x28;
        *(int*)e = s->ReadInt();
        if (*(int*)e != kHeapLive)
            continue;
        unsigned c = ReadArraySize(s);
        unsigned char* m = (unsigned char*)ResizeRaw(e + 4, 0x10, (int)c);
        for (int k = 0; k < (int)c; ++k) {
            *(int*)(m + k * 0x10) = s->ReadInt();
            *(float*)(m + k * 0x10 + 4) = s->ReadFloat();
            *(float*)(m + k * 0x10 + 8) = s->ReadFloat();
            *(float*)(m + k * 0x10 + 0xc) = s->ReadFloat();
        }
        *(float*)(e + 0x10) = s->ReadFloat();
        *(int*)(e + 0x14) = s->ReadInt();
        *(int*)(e + 0x18) = s->ReadInt();
        *(int*)(e + 0x1c) = s->ReadInt();
        *(float*)(e + 0x20) = s->ReadFloat();
        *(e + 0x24) = s->ReadByte();
        *(e + 0x25) = s->ReadByte();
    }
}

// PANZERS 0x56e170 (AMOD; 0x56e500 per element). HD also creates each
// model again; the recompile restores the records (the models of +0x2fc are
// made by the next Tick_5822a0).
static void LoadAnimatedModels(SGameLogic* g, SStream* s)
{
    unsigned n = ReadArraySize(s);
    for (int i = 0; i < g->AnimatedModelCount; ++i)
        FreeSString(&g->AnimatedModels[i].Name);
    ResizeRaw(&g->AnimatedModels, 0x24, (int)n);                  // 0x563230
    for (int i = 0; i < (int)n; ++i) {
        unsigned char* e = (unsigned char*)g->AnimatedModels + i * 0x24;
        ReadLogicStr(s, (SString*)e);
        *(float*)(e + 0x08) = s->ReadFloat();
        *(float*)(e + 0x0c) = s->ReadFloat();
        *(float*)(e + 0x10) = s->ReadFloat();
        *(float*)(e + 0x14) = s->ReadFloat();
        *(float*)(e + 0x1c) = s->ReadFloat();
    }
    if (n)
        Logger.g->Log(1, "STUB: SGameLogic AMOD load (0x56e500): %u animated models restored without models", n);
}

// PANZERS 0x56f3e0 (ECHO): the board texts are made again; the recompile's
// board does not keep echoes, so the names are read and dropped.
static void LoadEchoes(SGameLogic* g, SStream* s)
{
    int n = s->ReadInt();
    *(int*)((unsigned char*)g + 0x84) = 0;
    for (int i = 0; i < n; ++i) {
        SString name;
        ReadLogicStr(s, &name);
        FreeSString(&name);
    }
}

// PANZERS 0x56eb50
// Removes every unit, then reads the chunks of 0x57e110 in any order; the
// camera only with `camera`. Afterwards every placed unit of type 0, 5, 11
// or 12 updates the active locations (0x582080), then 0x56da00.
void SGameLogic::LoadGameState(SStream* s, bool camera)
{
    PZ_M3_TRACE("SGameLogic::LoadGameState (0x56eb50)");
    SWorld* w = g_World;
    // HD: board frames +0x150 / +0x158 are released (board +0x0c), the
    // concert +0x64 (music); the bridges are unfixed (the load-game LoadMap
    // 0x61f840 fixes them again after the load).
    if (w->UseAltHeights && !getenv("PZ_M4_FIXFIRST"))           // World+0xf0 (PZ_M4_FIXFIRST: loadgame.cpp)
        w->UnfixBridges();                                        // 0x600f90
    for (int i = 0; i < w->Units.Size; ++i)
        if (w->Units.IsLive(i))
            w->RemoveUnit(i);                                     // 0x5f8060
    w->Units.Size = 0;
    w->Units.Free = -1;
    w->Units.Count = 0;
    RemoveObjectiveMarkers(this, -1, -1);                         // 0x579420(-1, -1)
    // HD: the loading frame of the renderer and the window's +0x44
    // (0x929a54): not mapped.
    while (!s->ReadChunkIsEnd()) {
        int tag = s->ReadChunkHeader();
        switch (tag) {
        case 0x50474941: AIGroupsLoad(w, s); break;                // AIGP 0x56e2c0
        case 0x44454553: w->RandomSeed = (unsigned)s->ReadInt(); break;   // SEED
        case 0x33594c50: w->LoadPlayers(s); break;                // PLY3 0x5f2ed0
        case 0x204d4143: {                                        // "CAM "
            float cam[5];
            s->Read(cam, 0x14);
            if (camera)
                w->LoadCamera(cam);                               // 0x5fd7c0
            break;
        }
        case 0x33524957:                                          // WIR3 0x5f3d60(s, 1)
            // The recompile does not create wires (mapload.cpp keeps the map's
            // chunk); the saved chunk is the map's.
            s->ReadChunkSkip();
            break;
        case 0x4444444f: LoadWorldDoodadAnims(w, s); break;       // ODDD 0x5f1fd0
        case 0x47525452: LoadRunningTriggers(this, s); break;     // RTRG 0x56e1d0
        case 0x444f4d41: LoadAnimatedModels(this, s); break;      // AMOD 0x56e170
        case 0x47495254:                                          // TRIG 0x5f33e0 -> 0x5f0140
            LoadTriggers(reinterpret_cast<STriggerArray<STrigger>*>(&w->Triggers), s);
            break;
        case 0x4f484345: LoadEchoes(this, s); break;              // ECHO 0x56f3e0
        case 0x53434f4c: w->LoadLocations(s); break;              // LOCS 0x5f2eb0 -> 0x5f0690
        case 0x52485457: LoadWorldWeather(w, s); break;           // WTHR 0x5f3cb0
        case 0x5052474d: LoadMovementGroups(this, s); break;      // MGRP 0x56e380
        case 0x52415654: w->LoadTriggerVariables(s); break;       // TVAR 0x56e440
        case 0x52544e43: {                                        // CNTR
            *((unsigned char*)this + 0x14c) = s->ReadInt() != 0;
            *((unsigned char*)this + 0x14d) = s->ReadInt() != 0;
            *(int*)((unsigned char*)this + 0x174) = s->ReadInt();
            *(int*)((unsigned char*)this + 0x178) = s->ReadInt();
            break;
        }
        case 0x53524156: LoadVariables(s, this, kLogicVarsDesc); break;   // VARS
        case 0x53464545: w->LoadEffects(s); break;                // EEFS 0x5f2a40 -> 0x5f08a0
        case 0x53494e55: LoadWorldUnits(w, s); break;             // UNIS 0x5f3820
        case 0x544a424f: {                                        // OBJT: 0x560eb0 per marker
            int n = s->ReadInt();
            for (int i = 0; i < n; ++i) {
                float x = s->ReadFloat();
                float z = s->ReadFloat();
                int a = s->ReadInt();
                int b = s->ReadInt();
                AddMinimapObjective(x, z, a, b);
            }
            break;
        }
        default:
            Logger.g->Warning("SGameLogic::LoadGameState: Unsupported chunk tag 0x%08X (%c%c%c%c)", tag,
                              (char)tag, (char)(tag >> 8), (char)(tag >> 16), (char)(tag >> 24));
            s->ReadChunkSkip();
            break;
        }
        s->ReadChunkValidate(0);
    }
    for (int i = 0; i < w->Units.Size; ++i) {
        if (!w->Units.IsLive(i))
            continue;
        SUnit* u = w->Units.Array[i].Unit;
        int type = *(int*)((unsigned char*)u->Proto + 0x40);
        if ((type == 0 || type == 5 || type == 0xc || type == 0xb) && !u->Unplaced)
            UpdateActiveLocationsAt(i, false);                    // 0x582080(unit, 0)
    }
    // HD 0x56da00: the board counters of the trigger variables (gamelogic.cpp
    // logs them at map load; not lifted).
}

// Recompile-only test hook (docs/M4_STATUS.md): PZ_M4_SAVE_AT=<frame> saves
// SaveGames/M4RT-<frame>.save at that frame's BeginFrame (after the CRC
// line), with a sidecar .rt that holds the -packetplay position, the CRC
// queue and FramesSent (not part of a save) for PZ_M4_RT loads.
void M4SaveTestHook(SGameLogic* g)
{
    static int s_At = getenv("PZ_M4_SAVE_AT") ? atoi(getenv("PZ_M4_SAVE_AT")) : -1;
    if (s_At < 0 || g->Frame != s_At || !g_Campaign)
        return;
    char file[200];
    _snprintf(file, sizeof(file) - 1, "SaveGames/M4RT-%d.save", s_At);
    file[sizeof(file) - 1] = 0;
    bool ok = g_Campaign->SaveGame(file, "M4 round trip");
    Logger.g->Log(0, "PZM4: saved %s at frame %d (%s)", file, g->Frame, ok ? "ok" : "failed");
    char side[220];
    _snprintf(side, sizeof(side) - 1, "%s.rt", file);
    side[sizeof(side) - 1] = 0;
    FILE* fp = fopen(side, "wb");
    if (!fp)
        return;
    int pos = g->PlaybackStream ? g->PlaybackStream->Seek(0, 1) : -1;
    fwrite(&pos, 4, 1, fp);
    int n = g->CrcCount;
    fwrite(&n, 4, 1, fp);
    for (int i = g->CrcBottom; i <= g->CrcTop; ++i) {
        int idx = i - g->CrcBase;
        if (g->CrcMax <= idx)
            idx -= g->CrcMax;
        fwrite(&g->CrcHistory[idx], 4, 1, fp);
    }
    fwrite(&g->FramesSent, 4, 1, fp);
    fclose(fp);
}

// PANZERS 0x560eb0
// +0x194 SDArray of 0x14 {x, z, marker, a, b} (0x560ae0 adds one). An
// objective point (x >= 0) gets a sprite on the minimap frame (+0x17c) with
// the glyph of +0x1a0 / +0x1a4 (LoadMap: the interface font, 0x13c); the
// minimap update (minimap.cpp) moves it. An objective area is given by its
// corners at -x: corner b of the magenta frame (board +0xb8, 0xff00ffff),
// in minimap pixels of the map bitmap (+0x18c: width, height).
void SGameLogic::AddMinimapObjective(float x, float z, int a, int b)
{
    int* arr = (int*)((unsigned char*)this + 0x194);
    int n = arr[1];
    if (arr[2] <= n) {
        int m = n < 16 ? 16 : n * 6 / 5;
        arr[0] = (int)(size_t)realloc((void*)(size_t)arr[0], (size_t)m * 0x14);
        arr[2] = m;
    }
    arr[1] = n + 1;
    unsigned char* e = (unsigned char*)(size_t)arr[0] + n * 0x14;
    *(float*)e = x;
    *(float*)(e + 4) = z;
    if (0.0 <= x) {
        int f = Board ? Board->CreateFrame(FT_SPRITE, MinimapFrame, 0, 0, 0, true) : -1;   // board +0x08(1, +0x17c, 0, 0, 0, 1)
        *(int*)(e + 8) = f;
        if (Board)
            Board->SetSpriteGlyph(f, BoardArea[0], BoardArea[1]);   // board +0x24(f, +0x1a0, +0x1a4)
    } else {
        *(int*)(e + 8) = -1;
        // HD reads the bitmap size without a test; the recompile's menu
        // world has no minimap bitmap.
        const int* size = *(const int* const*)((const unsigned char*)this + 0x18c);
        if (Board && size && g_World) {
            double u = (double)(((float)-x - 48.0f) / (float)(g_World->TerrainW - 0x60)) - 0.5;   // 0x7f5ac0, 0x7f7f90, 0x7ea760
            double v = ((double)((z - 48.0f) / (float)(g_World->TerrainH - 0x60)) - 0.5) * (double)size[1];
            Board->SetMinimapMarkCorner(b, (float)(u * (double)size[0]), (float)-v, 0xff00ffff);   // board +0xb8
        }
    }
    *(int*)(e + 0xc) = a;
    *(int*)(e + 0x10) = b;
}

} // namespace pz
