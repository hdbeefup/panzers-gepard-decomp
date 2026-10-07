// src/world/mapload.cpp
// SWorld::LoadMap (HD 0x5f1990, MAPF v201) and its chunk readers: terrain,
// entities (doodads, decals, placed effects, unit definitions), weather,
// players and camera. OWNER: agent D. Lifted from the HD exe only.
//
// Chunks M1 does not use (rivers, locations, paths,
// triggers, trigger variables, wires, ambient sounds, lakes, AI groups, the
// minimap) are read whole and kept as raw copies for M2, with their counts
// logged. HD parses them into SWorld containers.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "aigroup.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "trigger.h"
#include "blockmaprefresh.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/iviewport.h"
#include "pz/ipixie.h"
#include "pz/iterrain.h"
#include "pz/imodel.h"
#include "pz/hdbitmap.h"
#include "stream.h"
#include "properties.h"
#include "logger.h"
#include "iconcert.h"
extern SIConcert* Concert;   // HD 0x8f1c5c
#include "stub_log.h"

namespace pz {

static const int kTagMAPF = 0x4650414d, kVer201 = 0x31303276, kVer100 = 0x30303176;

// HD throws a const char* (__CxxThrowException_8 with the string type info);
// the recompile throws the same message.
static void Throw(const char* msg)
{
    throw msg;
}

// PANZERS 0x56e7d0
// SString load used by the world: u16 length, then the bytes (no
// old-format length correction).
static void ReadWorldString(SStream* s, SString* out)
{
    int len = s->ReadWord() & 0xffff;
    FreeSString(out);
    out->size = len;
    out->buf = new char[len + 1];
    s->Read(out->buf, len);
    out->buf[len] = 0;
}

// PANZERS 0x5f0e30
// One ambient sound source: 'AMBI' chunk, version 'v100', the file name,
// x, y (above the ground), z, the minimum distance; the sound is cached
// (Concert +0x24 positional).
static void LoadAmbientSound(SWorld::SAmbientSound* a, SStream* s)
{
    if (s->ReadChunkHeader() != 0x49424d41)                       // 'AMBI'
        throw "Expected ambient chunk";
    if (s->ReadInt() != 0x30303176)                               // 'v100'
        throw "Unsupported ambient version";
    ReadWorldString(s, &a->Name);                                 // 0x56e7d0
    a->X = s->ReadFloat();
    a->Y = s->ReadFloat();
    a->Z = s->ReadFloat();
    a->MinDistance = s->ReadFloat();
    a->Cache = Concert ? Concert->PrecacheSound(a->Name.buf ? a->Name.buf : "", true) : -1;   // Concert +0x24(name, 1)
    s->ReadChunkValidate(0);                                      // 0x65d460
}

// PANZERS 0x5f02a0
// SHeap<SAmbientSound>::Load: the old sources go (0x5dd3e0), then size,
// free head, count and per slot its link (a live slot loads its record).
void SWorld::LoadAmbientSounds(SStream* s)
{
    ClearAmbientSounds();                                         // 0x5dd3e0
    SHeap<SAmbientSound>& h = AmbientSounds();
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        throw "Invalid array size";
    h.Size = (int)n;
    if (h.Max < (int)n) {
        h.Max = (int)n;
        h.Array = (SHeapElem<SAmbientSound>*)realloc(h.Array, n * sizeof(SHeapElem<SAmbientSound>));
    }
    if (h.Array && h.Max > 0)
        memset(h.Array, 0, (size_t)h.Max * sizeof(SHeapElem<SAmbientSound>));
    h.Free = s->ReadInt();
    h.Count = s->ReadInt();
    for (int i = 0; i < h.Size; ++i) {
        h.Array[i].Next = s->ReadInt();
        if (h.Array[i].Next == kHeapLive)
            LoadAmbientSound(&h.Array[i].Data, s);
    }
}

// PANZERS 0x5dd3e0
// SHeap<SAmbientSound>::Clear: every live source's loop stops (Concert
// +0x3c) and its name is freed.
void SWorld::ClearAmbientSounds()
{
    SHeap<SAmbientSound>& h = AmbientSounds();
    for (int i = 0; i < h.Size; ++i) {
        if (h.Array[i].Next != kHeapLive)
            continue;
        if (Concert)
            Concert->RemoveSound(h.Array[i].Data.Sound);
        FreeSString(&h.Array[i].Data.Name);
    }
    h.Size = 0;
    h.Free = -1;
    h.Count = 0;
}

static float ReadF(SStream* s)
{
    float f;
    s->Read(&f, 4);
    return f;
}

static void TagText(int tag, char out[5])
{
    out[0] = (char)tag;
    out[1] = (char)(tag >> 8);
    out[2] = (char)(tag >> 16);
    out[3] = (char)(tag >> 24);
    out[4] = 0;
}

// ---------------------------------------------------------------------------
// Raw chunk store (recompile only, see the file comment).

struct SRawChunk {
    int Tag;
    int Size;
    unsigned char* Data;
};
static SRawChunk s_RawChunks[32];
static int s_RawChunkCount;

static void KeepRawChunk(SStream* s, int tag, const char* what)
{
    int size = s->ReadChunkRemain();
    unsigned char* data = (unsigned char*)malloc(size > 0 ? size : 1);
    if (size > 0)
        s->Read(data, size);
    int count = size >= 4 ? *(int*)data : 0;
    if (s_RawChunkCount < (int)(sizeof(s_RawChunks) / sizeof(s_RawChunks[0]))) {
        s_RawChunks[s_RawChunkCount].Tag = tag;
        s_RawChunks[s_RawChunkCount].Size = size;
        s_RawChunks[s_RawChunkCount].Data = data;
        ++s_RawChunkCount;
    } else {
        free(data);
    }
    char t[5];
    TagText(tag, t);
    if (Logger.g)
        Logger.g->Log(0, "PZ3D world: %s %s kept raw (%d bytes, first dword %d)", t, what, size, count);
}

void FreeMapRawChunks()
{
    for (int i = 0; i < s_RawChunkCount; ++i)
        free(s_RawChunks[i].Data);
    s_RawChunkCount = 0;
}

// Recompile only (agent S, M4): the save game writes the map's wires (WIR3)
// back from the raw copy, as the world does not create them yet.
const unsigned char* GetMapRawChunk(int tag, int* size)
{
    for (int i = 0; i < s_RawChunkCount; ++i)
        if (s_RawChunks[i].Tag == tag) {
            *size = s_RawChunks[i].Size;
            return s_RawChunks[i].Data;
        }
    *size = 0;
    return nullptr;
}

// ---------------------------------------------------------------------------
// Terrain buffers (recompile fallback while agent B's STerrain is a stub).

static void* s_FallbackBuffers[8];

void FreeFallbackTerrainBuffers()
{
    for (int i = 0; i < 8; ++i) {
        free(s_FallbackBuffers[i]);
        s_FallbackBuffers[i] = nullptr;
    }
}

static void* FallbackBuffer(int slot, size_t bytes)
{
    free(s_FallbackBuffers[slot]);
    s_FallbackBuffers[slot] = calloc(1, bytes ? bytes : 1);
    return s_FallbackBuffers[slot];
}

// PANZERS 0x5f2fc0
void SWorld::LoadTerrain(SStream* s)
{
    PZ_TRACE("SWorld::LoadTerrain (0x5f2fc0)");
    if (s->ReadInt() != kVer100)
        Throw("Unsupported terrain version");
    while (!s->ReadChunkIsEnd()) {
        int tag = s->ReadChunkHeader();
        g_WorldStats.SubChunks++;
        switch (tag) {
        case 0x50414d48: {   // HMAP
            int w = s->ReadInt();
            int h = s->ReadInt();
            if ((TerrainW != w || TerrainH != h) && Terrain) {
                g_Scene->DestroyTerrain();                        // scene +0x68
                Terrain = nullptr;
            }
            TerrainW = w;
            BlockH = h * 4;
            TerrainH = h;
            BlockW = w * 4;
            BlockSize = h * 4 * w * 4;
            if (!Terrain) {
                Terrain = g_Scene->CreateTerrain(w, h, 4);        // scene +0x64
                STerrainBuffers b;
                memset(&b, 0, sizeof(b));
                if (Terrain)
                    Terrain->Acquire(&b);                         // terrain +0x00 (8 out pointers)
                Heights = b.Heights;
                AltHeights = (float*)b.Buffer74;
                Diffuse = b.Diffuse;
                Blend = b.Blend;
                TileMap = b.BufferB0;
                WaterHeights = (float*)b.Buffer8C;
                BlockMap = (unsigned*)b.Buffer11c68;
                BlockMap2 = b.Buffer11c6c;
                // Recompile: no buffers from the terrain (stub) -> world-owned
                // copies so the map still loads and heights can be queried.
                size_t verts = (size_t)(w + 1) * (h + 1);
                if (!Heights) Heights = (float*)FallbackBuffer(0, verts * 4);
                if (!Diffuse) Diffuse = (float*)FallbackBuffer(1, verts * 4);
                if (!Blend) Blend = (unsigned char*)FallbackBuffer(2, verts * 16);
                if (!TileMap) TileMap = FallbackBuffer(3, (size_t)(w / 8) * (h / 8) * 2);
                if (!WaterHeights) WaterHeights = (float*)FallbackBuffer(4, verts * 4);
                if (!BlockMap) BlockMap = (unsigned*)FallbackBuffer(5, (size_t)BlockSize * 4);
                ResetCamera();                                    // 0x5ecc20
            }
            s->Read(Heights, (TerrainH + 1) * (TerrainW + 1) * 4);
            break;
        }
        case 0x444e4c42: {   // BLND
            if (!Blend)
                Throw("Blendmap without Heightmap");
            int n = (TerrainH + 1) * (TerrainW + 1);
            unsigned char* tmp = (unsigned char*)malloc(n);
            int layer = 0;
            for (; layer < Layers.Size - 1; ++layer) {
                s->Read(tmp, n);
                for (int i = 0; i < n; ++i)
                    Blend[i * 16 + layer] = tmp[i];
            }
            for (; layer < 16; ++layer)
                for (int i = 0; i < n; ++i)
                    Blend[i * 16 + layer] = 0;
            free(tmp);
            break;
        }
        case 0x46464944:     // DIFF
            s->Read(Diffuse, (TerrainH + 1) * (TerrainW + 1) * 4);
            break;
        case 0x4b434c42:     // BLCK
            if (!BlockMap)
                Throw("Blockmap without Heightmap");
            for (int row = 0; row < BlockH; ++row)
                for (int col = 0; col < BlockW; ++col)
                    BlockMap[BlockW * row + col] = s->ReadWord() & 0xffff;
            break;
        case 0x50414d54:     // TMAP
            if (!TileMap)
                Throw("Tilemap without Heightmap");
            s->Read(TileMap, (TerrainH / 8) * (TerrainW / 8) * 2);
            break;
        case 0x59414c54:     // TLAY
            LoadTerrainLayers(s);
            break;
        default: {
            char t[5];
            TagText(tag, t);
            if (Logger.g)
                Logger.g->Warning("Unsupported terrain sub chunk tag 0x%08X (%s)", tag, t);
            s->ReadChunkSkip();
            break;
        }
        }
        s->ReadChunkValidate(0);
    }
    // HD 0x582d20: frees a temporary SDArray (layer texture names).
    if (Logger.g)
        Logger.g->Log(0, "PZ3D world: TERR %d x %d tiles, %d layers, terrain %s",
                      TerrainW, TerrainH, Layers.Size, Terrain ? "created" : "null (scene +0x64 stub)");
}

// PANZERS 0x5efda0
void SWorld::LoadTerrainLayers(SStream* s)
{
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        Throw("Invalid array size");
    // HD 0x5dccd0: resize the layer array.
    for (int i = 0; i < Layers.Size; ++i) {
        FreeSString(&Layers.Array[i].Name);
        FreeSString(&Layers.Array[i].Extra);
    }
    Layers.Size = (int)n;
    if (Layers.Max < (int)n) {
        Layers.Max = (int)n;
        Layers.Array = (STerrainLayer*)realloc(Layers.Array, n * sizeof(STerrainLayer));
    }
    memset(Layers.Array, 0, Layers.Max * sizeof(STerrainLayer));
    for (int i = 0; i < Layers.Size; ++i) {
        STerrainLayer* l = &Layers.Array[i];
        ReadWorldString(s, &l->Name);
        unsigned attr = (unsigned)s->ReadInt();
        l->Attributes = attr;
        if ((attr & 2) == 0) {
            if (attr & 8) {
                l->Attributes = (attr & 0xfffffff7) | 2;
                SetSString(&l->Extra, "99 Regi");
            }
        } else {
            ReadWorldString(s, &l->Extra);
        }
    }
}

// ---------------------------------------------------------------------------
// Doodads

// PANZERS 0x5dd530
void SWorld::ClearDoodads()
{
    for (int i = 0; i < Doodads.Size; ++i)
        if (Doodads.Array[i].Next == kHeapLive)
            Doodads.Array[i].Data.Release();
    Doodads.Size = 0;
    Doodads.Free = -1;
    Doodads.Count = 0;
}

// PANZERS 0x5f0500
void SWorld::LoadDoodads(SStream* s)
{
    ClearDoodads();
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        Throw("Invalid array size");
    Doodads.Size = (int)n;
    if (Doodads.Max < (int)n) {
        Doodads.Max = (int)n;
        Doodads.Array = (SHeapElem<SDoodad>*)realloc(Doodads.Array, n * 200);
    }
    memset(Doodads.Array, 0, Doodads.Max * 200);
    Doodads.Free = s->ReadInt();
    Doodads.Count = s->ReadInt();
    for (int i = 0; i < Doodads.Size; ++i) {
        Doodads.Array[i].Next = s->ReadInt();
        if (Doodads.Array[i].Next == kHeapLive)
            Doodads.Array[i].Data.Load(s);
    }
}

// PANZERS 0x5f73f0
void SWorld::RemoveDoodad(int index)
{
    if (!Doodads.IsLive(index))
        Logger.g->Panic("SHeap<SDoodad>::Remove: invalid index (%d)", index);
    Doodads.Array[index].Next = Doodads.Free;
    Doodads.Array[index].Data.Release();
    Doodads.Count--;
    Doodads.Free = index;
}

// PANZERS 0x5f1100
void SDoodad::Load(SStream* s)
{
    if (s->ReadChunkHeader() != 0x444f4f44)   // DOOD
        Throw("Expected doodad chunk");
    if (s->ReadInt() != kVer100)
        Throw("Unsupported doodad version");
    ReadWorldString(s, &Name);
    X = ReadF(s);
    YOffset = ReadF(s);
    Z = ReadF(s);
    Angle = ReadF(s);
    TiltX = ReadF(s);
    TiltZ = ReadF(s);
    _ac = -1;
    if (Name.size != 0)
        Initialize();
    s->ReadChunkSkip();
    s->ReadChunkValidate(0);
}

// PANZERS 0x5ee040
void SDoodad::Initialize()
{
    SWorld* w = g_World;
    SProperties* ini = w->ObjectsIni;
    // Two objects.ini sections: the file name ("nagybokor3.4D") and the
    // name of its directory ("02 Bushes"); the file section overrides.
    SString fileSec, dirPath, dirSec;
    PathFilePart(&Name, &fileSec);
    PathDirPart(&Name, &dirPath);
    PathFilePart(&dirPath, &dirSec);
    FreeSString(&dirPath);
    const char* fs = SStr(fileSec);
    const char* ds = SStr(dirSec);
    Obstruction = ini->GetInt(fs, "Obstruction", ini->GetInt(ds, "Obstruction", 0));
    Indestructible = ini->GetInt(fs, "Indestructible", ini->GetInt(ds, "Indestructible", 0));
    Demolishable = ini->GetInt(fs, "Demolishable", ini->GetInt(ds, "Demolishable", 0));
    Demolishable2 = ini->GetInt(fs, "Demolishable2", ini->GetInt(ds, "Demolishable2", 0));
    DemolishableFence = ini->GetInt(fs, "DemolishableFence", ini->GetInt(ds, "DemolishableFence", 0));
    DemolishableWreck = ini->GetInt(fs, "DemolishableWreck", ini->GetInt(ds, "DemolishableWreck", 0));
    Cover = ini->GetInt(fs, "Cover", ini->GetInt(ds, "Cover", 0));
    Scrub = ini->GetInt(fs, "Scrub", ini->GetInt(ds, "Scrub", 0));
    Tree = ini->GetInt(fs, "Tree", ini->GetInt(ds, "Tree", 0));
    DemolishAngleMax = ini->GetFloat(fs, "DemolishAngleMax", ini->GetFloat(ds, "DemolishAngleMax", 86.0f));
    // HD reads "DemolishAngleVelocity " (trailing space) from the directory section.
    DemolishAngleVelocity = ini->GetFloat(fs, "DemolishAngleVelocity", ini->GetFloat(ds, "DemolishAngleVelocity ", 8.0f));
    DemolishAngleAccel = ini->GetFloat(fs, "DemolishAngleAcceleration", ini->GetFloat(ds, "DemolishAngleAcceleration", 3.0f));
    const char* e1 = ini->GetString(fs, "DemolishEffectName1", ini->GetString(ds, "DemolishEffectName1", ""));
    DemolishEffect1 = -1;
    if (e1 && *e1 && g_Pixie) {
        DemolishEffect1 = g_Pixie->LoadEffectPrototype(e1, false, false, 0, 0);   // pixie +0x10
        g_WorldStats.EffectProtos++;
    }
    const char* e2 = ini->GetString(fs, "DemolishEffectName2", ini->GetString(ds, "DemolishEffectName2", ""));
    DemolishEffect2 = -1;
    if (e2 && *e2 && g_Pixie) {
        DemolishEffect2 = g_Pixie->LoadEffectPrototype(e2, false, false, 0, 0);
        g_WorldStats.EffectProtos++;
    }
    DemolishEffectShift = ini->GetFloat(fs, "DemolishEffectShift", ini->GetFloat(ds, "DemolishEffectShift", 0.0f));
    ScrubTrembling = ini->GetFloat(fs, "ScrubTrembling", ini->GetFloat(ds, "ScrubTrembling", 5.0f));
    ScrubTremblingAtten = ini->GetFloat(fs, "ScrubTremblingAttenuation", ini->GetFloat(ds, "ScrubTremblingAttenuation", 0.6f));
    Flags = Obstruction ? 4 : 0;
    if (Indestructible)
        Flags |= 8;
    if (Demolishable || DemolishableFence || DemolishableWreck)
        Flags |= 0x10;
    if (Demolishable2)
        Flags |= 0x1000;
    if (Scrub)
        Flags |= 0x20;
    if (Cover)
        Flags |= 0x2000;

    Model = g_Scene->CreateModelFromFile(SStr(Name), 0.005f, 0, 0);   // scene +0x50 (0x3ba3d70a)
    g_WorldStats.DoodadModels++;
    if (Model) {
        g_WorldStats.DoodadModelsOk++;
        if (Tree == 0) {
            Model->SetFlags(0x80);                                // model +0x94
        } else {
            Model->SetFlags(0xa0);
            int r = rand();                                       // HD 0x78c846
            Model->SetSway((float)((double)r * 3.0517578125e-05 * 100.0), 0.035f, 0.035f);   // model +0xe8
        }
        Model->SetNodeVisible(Model->FindNode("Block"), false);    // +0x40 / +0x60
        Model->SetNodeVisible(Model->FindNode("Platform"), false);
    } else if (Logger.g && g_WorldStats.DoodadModels - g_WorldStats.DoodadModelsOk <= 3) {
        // HD panics: "SDoodad::Initialize: Couldn't load object: %s". Logged
        // for the first 3 only while agent A's scene is a stub.
        Logger.g->Log(0, "SDoodad::Initialize: Couldn't load object: %s (HD panics; agent A scene pending)", SStr(Name));
    }
    // Ruin model: "<name without the last 3 chars>_rom.4d", if the file exists.
    SString ruin;
    ruin = Name;
    if (ruin.size >= 3) {
        ruin.buf[ruin.size - 3] = 0;
        ruin.size -= 3;
    }
    char* rb = new char[ruin.size + 8];
    memcpy(rb, ruin.buf, ruin.size);
    memcpy(rb + ruin.size, "_rom.4d", 8);
    FreeSString(&ruin);
    if (FileSystem.Stat(rb, nullptr) == 0) {                      // HD 0x65faf0
        g_WorldStats.Ruins++;
        RuinModel = g_Scene->CreateModelFromFile(rb, 0.005f, 0, 0);
        g_WorldStats.DoodadModels++;
        if (RuinModel) {
            g_WorldStats.DoodadModelsOk++;
            RuinModel->SetFlags(0);
            RuinModel->SetVisible(false, false);                  // model +0x30(0, 0): hidden until demolished
        } else if (Logger.g) {
            Logger.g->Log(0, "SWorld::Initialize: Couldn't load object: %s (HD panics; agent A scene pending)", rb);
        }
    }
    delete[] rb;
    BlockRect = nullptr;
    LightCount = 0;                                               // 0x546af0(0)
    UpdatePosition();                                             // 0x6012d0
    FreeSString(&fileSec);
    FreeSString(&dirSec);
}

// PANZERS 0x661b30
// SBlockBitmap dtor (frees the bits), then the caller's operator delete(0x1c).
// The bitmap comes from SModel::BuildNodeBlockBitmap (pz3d, new / new[]).
static void FreeBlockBitmap(SBlockBitmap* bm)
{
    delete[] bm->Bits;
    bm->Bits = nullptr;
    delete bm;
}

// PANZERS 0x6012d0
void SDoodad::UpdatePosition()
{
    float y = g_World->GetTerrainHeight(X, Z) + YOffset;
    if (Model) {
        Model->SetPosition(X, y, Z);                              // model +0x18
        Model->SetRotation(Angle, TiltX, TiltZ);                  // model +0x1c
        Model->SetHighlight(Highlight);                                 // model +0xc0
        // The old footprint's cells (3-cell margin) are rebuilt without it,
        // the new one (4 cells per unit from the "Block" node) is marked;
        // the block map rebuild 0x604620 ORs Flags into its set cells.
        if (BlockRect) {
            BlockMap_MarkDirty(g_World, BlockRect->X - 3, BlockRect->Z - 3,
                               BlockRect->W + 3 + BlockRect->X, BlockRect->H + 3 + BlockRect->Z,
                               0x303c);                           // 0x5ef380
            FreeBlockBitmap(BlockRect);                           // 0x661b30, delete(0x1c)
        }
        BlockRect = Model->BuildNodeBlockBitmap(4, "Block");      // model +0xa8
        if (BlockRect)
            BlockMap_MarkDirty(g_World, BlockRect->X - 3, BlockRect->Z - 3,
                               BlockRect->W + 3 + BlockRect->X, BlockRect->H + 3 + BlockRect->Z,
                               0x303c);                           // 0x5ef380
    }
    if (RuinModel) {
        RuinModel->SetPosition(X, y, Z);
        RuinModel->SetRotation(Angle, TiltX, TiltZ);
        RuinModel->SetHighlight(Highlight);                             // model +0xc0
    }
    if (LightCount > 0)
        STUB_LOG("SDoodad::UpdatePosition (0x6012d0) attached lights 0x6090e0");
}

// PANZERS 0x5d4e50
void SDoodad::Release()
{
    // HD: attached lights (0x5f82b0; none in menu.map), then pixie +0x20
    // for the two demolish effect prototypes.
    if (g_Pixie) {
        g_Pixie->ReleaseEffectPrototype(DemolishEffect1);
        g_Pixie->ReleaseEffectPrototype(DemolishEffect2);
    }
    DemolishEffect1 = DemolishEffect2 = -1;
    if (Model) {
        Model->Release();                                         // model +0x04
        Model = nullptr;
    }
    if (RuinModel) {
        RuinModel->Release();
        RuinModel = nullptr;
    }
    BlockMap_ApplyBitmap(g_World, BlockRect, false, Flags);       // 0x5f4910 (clear + mark dirty)
    if (BlockRect) {
        FreeBlockBitmap(BlockRect);                               // 0x661b30, delete(0x1c)
        BlockRect = nullptr;
    }
    free(Lights);
    Lights = nullptr;
    LightCount = LightMax = 0;
    FreeSString(&Name);
}

// ---------------------------------------------------------------------------
// Decals

// PANZERS 0x5dc890
void SWorld::ClearDecals(int newSize)
{
    for (int i = 0; i < Decals.Size; ++i)
        Decals.Array[i].Release();
    Decals.Size = newSize;
    if (Decals.Max < newSize) {
        Decals.Max = newSize;
        Decals.Array = (SDecal*)realloc(Decals.Array, newSize * sizeof(SDecal));
    }
    memset(Decals.Array, 0, Decals.Max * sizeof(SDecal));
}

// PANZERS 0x5efcd0
void SWorld::LoadDecals(SStream* s)
{
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        Throw("Invalid array size");
    ClearDecals((int)n);
    for (int i = 0; i < Decals.Size; ++i)
        Decals.Array[i].Load(s);
}

// PANZERS 0x5f7170
void SWorld::RemoveDecal(int index)
{
    if (index < 0 || index >= Decals.Size)
        Logger.g->Panic("SDArray<SDecal>::Remove: invalid index (%d) size = %d", index, Decals.Size);
    Decals.Array[index].Release();
    Decals.Size--;
    if (Decals.Size - index != 0)
        memmove(&Decals.Array[index], &Decals.Array[index + 1], (Decals.Size - index) * sizeof(SDecal));
    memset(&Decals.Array[Decals.Size], 0, sizeof(SDecal));
}

// PANZERS 0x5f1070
void SDecal::Load(SStream* s)
{
    if (s->ReadChunkHeader() != 0x41434544)   // DECA
        Throw("Expected decal chunk");
    if (s->ReadInt() != kVer100)
        Throw("Unsupported decal version");
    ReadWorldString(s, &Name);
    A = s->ReadInt();
    B = s->ReadInt();
    C = s->ReadInt();
    Create();
    s->ReadChunkValidate(0);
}

// HD 0x5e78b0: index of a decal in World+0x73c4.
static int DecalIndex(const SDecal* d)
{
    return (int)(d - g_World->Decals.Array);
}

// PANZERS 0x5edfd0
void SDecal::Create()
{
    int index = DecalIndex(this);                                 // 0x5e78b0
    Texture = PzGepard()->LoadTexture(SStr(Name), 1, true);       // Gepard +0x44(name, 1, 1)
    if (SITerrain* t = g_World->Terrain) {
        t->AddDecal(index, Texture, A, B, C);                     // terrain +0x48(index, tex, x, z, rotation)
        t->SetDecalType(index, _18);                              // terrain +0x54
    }
    g_WorldStats.Decals++;
}

// PANZERS 0x5d6800
void SDecal::Release()
{
    if (SITerrain* t = g_World ? g_World->Terrain : nullptr)
        t->RemoveDecal(DecalIndex(this));                         // terrain +0x4c
    if (Texture >= 0 && Name.buf)
        PzGepard()->ReleaseTexture(Texture);                      // Gepard +0x48
    Texture = 0;
    FreeSString(&Name);
}

// ---------------------------------------------------------------------------
// Water map

// PANZERS 0x608600
// The water map starts as a copy of the height map (no water anywhere); the
// lakes (+0x73b0, 0x38 each: +0x34 = 1, then 0x607ad0) and the rivers
// (+0x73e8, 0xa0 each, 0x6015d0 per river, the +0x98 & 0x10 ones last)
// raise it. Then the whole map's water bits (0x600) are marked dirty for the
// block-map rebuild. The menu map has neither lakes nor rivers (LAKS / RVR2
// are kept raw and hold 0 entries).
void SWorld::UpdateWaterMap()
{
    if (!WaterHeights || !Heights)
        Logger.g->Panic("SWorld::UpdateWaterMap(): WaterMap or HeightMap is NULL.");
    memcpy(WaterHeights, Heights, (size_t)(TerrainH + 1) * (TerrainW + 1) * 4);
    BlockMap_MarkDirty(this, 0, 0, BlockW, BlockH, 0x600);        // inline in HD
}

// ---------------------------------------------------------------------------
// Placed effects

// PANZERS 0x5dd880
void SWorld::ClearEffects()
{
    for (int i = 0; i < Effects.Size; ++i) {
        SHeapElem<SEffectSite>* e = &Effects.Array[i];
        if (e->Next != kHeapLive)
            continue;
        if (g_Pixie) {
            g_Pixie->StopEffect(e->Data.Effect);                  // pixie +0x34
            g_Pixie->ReleaseEffectPrototype(e->Data.Prototype);   // pixie +0x20
        }
        FreeSString(&e->Data.Name);
    }
    Effects.Size = 0;
    Effects.Free = -1;
    Effects.Count = 0;
}

// PANZERS 0x5f08a0
void SWorld::LoadEffects(SStream* s)
{
    ClearEffects();
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        Throw("Invalid array size");
    Effects.Size = (int)n;
    if (Effects.Max < (int)n) {
        Effects.Max = (int)n;
        Effects.Array = (SHeapElem<SEffectSite>*)realloc(Effects.Array, n * 0x2c);
    }
    memset(Effects.Array, 0, Effects.Max * 0x2c);
    Effects.Free = s->ReadInt();
    Effects.Count = s->ReadInt();
    for (int i = 0; i < Effects.Size; ++i) {
        Effects.Array[i].Next = s->ReadInt();
        if (Effects.Array[i].Next == kHeapLive)
            Effects.Array[i].Data.Load(s);
    }
}

// PANZERS 0x5f13e0
void SEffectSite::Load(SStream* s)
{
    if (s->ReadChunkHeader() != 0x45464645)   // EFFE
        Throw("Expected effect chunk");
    if (s->ReadInt() != kVer100)
        Throw("Unsupported effect version");
    ReadWorldString(s, &Name);
    X = ReadF(s);
    YOffset = ReadF(s);
    Z = ReadF(s);
    DirX = ReadF(s);
    DirZ = ReadF(s);
    Create();
    s->ReadChunkValidate(0);
}

// PANZERS 0x5ee9f0
void SEffectSite::Create()
{
    Prototype = -1;
    Effect = -1;
    if (g_Pixie) {
        // HD passes an empty SString by value as the last two dwords.
        Prototype = g_Pixie->LoadEffectPrototype(SStr(Name), false, false, 0, 0);   // pixie +0x10
        g_WorldStats.EffectProtos++;
    }
    double inv = 1.0 / sqrt((double)(DirX * DirX + 1.0f + DirZ * DirZ));
    float dir[3] = { (float)((double)DirX * inv), (float)inv, (float)((double)DirZ * inv) };
    float pos[3] = { X, g_World->GetTerrainHeight(X, Z) + YOffset, Z };
    if (g_Pixie) {
        Effect = g_Pixie->CreateEffect(g_Scene, Prototype, pos, dir);   // pixie +0x2c
        UpdatePosition();                                         // 0x6014e0
    }
    g_WorldStats.Effects++;
    if (Logger.g)
        Logger.g->Log(0, "PZ3D world: effect %s proto %d at %.2f %.2f %.2f dir %.3f %.3f %.3f",
                      SStr(Name), Prototype, pos[0], pos[1], pos[2], dir[0], dir[1], dir[2]);
}

// PANZERS 0x6014e0
// Re-places the effect on the terrain: pixie +0x4c position (terrain height
// + YOffset) and +0x50 the normalised direction (DirX, 1, DirZ).
void SEffectSite::UpdatePosition()
{
    if (!g_Pixie || Effect < 0)
        return;
    float pos[3] = { X, g_World->GetTerrainHeight(X, Z) + YOffset, Z };
    g_Pixie->SetEffectPosition(Effect, pos);                      // pixie +0x4c
    double inv = 1.0 / sqrt((double)(DirX * DirX + 1.0f + DirZ * DirZ));
    float dir[3] = { (float)((double)DirX * inv), (float)inv, (float)((double)DirZ * inv) };
    g_Pixie->SetEffectDirection(Effect, dir);                     // pixie +0x50
}

// ---------------------------------------------------------------------------
// Roads and junctions

// PANZERS 0x5d5330
void SMapRoad::Release()
{
    if (Texture >= 0)
        PzGepard()->ReleaseTexture(Texture);                      // Gepard +0x48
    if (Road >= 0 && g_World && g_World->Terrain)
        g_World->Terrain->DestroyRoad(Road);                      // terrain +0x84
    free(PathPoints);
    PathPoints = nullptr;
    PathPointCount = PathPointMax = 0;
    free(Points);
    Points = nullptr;
    PointCount = PointMax = 0;
    free(TerrainPoints);
    TerrainPoints = nullptr;
    TerrainPointCount = TerrainPointMax = 0;
    FreeSString(&Name);
}

// PANZERS 0x5dd9a0
void SWorld::ClearRoads()
{
    for (int i = 0; i < Roads.Size; ++i)
        if (Roads.Array[i].Next == kHeapLive)
            Roads.Array[i].Data.Release();
    Roads.Size = 0;
    Roads.Free = -1;
    Roads.Count = 0;
}

// PANZERS 0x5f0a30
void SWorld::LoadRoads(SStream* s)
{
    ClearRoads();
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        Throw("Invalid array size");
    Roads.Size = (int)n;
    if (Roads.Max < (int)n) {
        Roads.Max = (int)n;
        Roads.Array = (SHeapElem<SMapRoad>*)realloc(Roads.Array, n * 0x48);
    }
    memset(Roads.Array, 0, Roads.Max * 0x48);
    Roads.Free = s->ReadInt();
    Roads.Count = s->ReadInt();
    for (int i = 0; i < Roads.Size; ++i) {
        Roads.Array[i].Next = s->ReadInt();
        if (Roads.Array[i].Next == kHeapLive)
            Roads.Array[i].Data.Load(s);
    }
}

// PANZERS 0x601c10 (path-length part, agent TR)
// The control points' path length (+0x18) that the trains run on: each
// segment i .. i + 1 is sampled at t = k / (n + 1), k < n, n = round(chord /
// Step) (at least 1), on the Hermite curve of the points with the tangents
// scaled by the chord; the length is the sum of the distances between
// consecutive samples, and a point gets the length reached before its own
// segment. HD writes no length for the last point (its writer was not
// found; 0 would pin every train to the road's end): the recompile gives it
// the length plus the chord from the last sample. PathPoints (path finder)
// are still not built.
static void BuildRoadLengths(SMapRoad* r)
{
    int n = r->PointCount;
    float L = 0.0f;
    float prev[2] = { 0.0f, 0.0f };
    bool havePrev = false;
    for (int i = 0; i < n - 1; ++i) {
        SRoadControlPoint& a = r->Points[i];
        SRoadControlPoint& b = r->Points[i + 1];
        float ax = a.X, az = a.Z, bx = b.X, bz = b.Z;
        float len = (float)sqrt((double)((bz - az) * (bz - az) + (bx - ax) * (bx - ax)));
        int samples = (int)floor((double)(len / r->Step) + 0.5);  // ROUND (fistp)
        if (samples == 0)
            samples = 1;
        a.Length = L;
        float div = (float)(samples + 1);
        for (int k = 0; k < samples; ++k) {
            float t = (float)k / div;
            float t2 = t * t, t3 = t2 * t;
            float h10 = (t3 - t2 * 2.0f) + t;
            float h01 = t2 * 3.0f - t3 * 2.0f;
            float h00 = (t3 * 2.0f - t2 * 3.0f) + 1.0f;
            float x = b.DirX * len * (t3 - t2) + ax * h00 + bx * h01 + len * a.DirX * h10;
            float z = b.DirZ * len * (t3 - t2) + az * h00 + bz * h01 + len * a.DirZ * h10;
            if (havePrev)
                L = (float)sqrt((double)((prev[0] - x) * (prev[0] - x) + (prev[1] - z) * (prev[1] - z))) + L;
            prev[0] = x;
            prev[1] = z;
            havePrev = true;
        }
    }
    if (n > 1) {
        SRoadControlPoint& last = r->Points[n - 1];
        last.Length = L + (float)sqrt((double)((prev[0] - last.X) * (prev[0] - last.X) +
                                               (prev[1] - last.Z) * (prev[1] - last.Z)));
    }
}

void SMapRoad::Load(SStream* s)
{
    Texture = -1;
    ReadWorldString(s, &Name);                                    // 0x56e7d0
    Step = ReadF(s);
    TexLength = ReadF(s);
    Flags = (unsigned)s->ReadInt();
    // PANZERS 0x5effd0: the control points.
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        Throw("Invalid array size");
    PointCount = (int)n;
    if (PointMax < (int)n) {
        PointMax = (int)n;
        Points = (SRoadControlPoint*)realloc(Points, n * sizeof(SRoadControlPoint));
    }
    if (PointMax)
        memset(Points, 0, PointMax * sizeof(SRoadControlPoint));
    for (int i = 0; i < PointCount; ++i) {
        SRoadControlPoint* p = &Points[i];
        p->X = ReadF(s);
        p->Z = ReadF(s);
        p->DirX = ReadF(s);
        p->DirZ = ReadF(s);
        p->F14 = ReadF(s);
        p->W = s->ReadInt();                                      // raw dword (0x5f0043)
    }
    Road = -1;
    Texture = -1;
    BuildRoadLengths(this);                                       // 0x601c10 path lengths (TR)
    Build(Flags, true);                                           // 0x601c10(flags, 1)
}

// PANZERS 0x601c10 (terrain part)
// HD first samples a Hermite spline through the control points into
// PathPoints (step = Step, 0x601c10..0x602276) and fills each point's
// Length: path-finder data (M2), not built here. The terrain gets the
// control points themselves: x, z, tangent, Dirty = 1 and W; a point with
// W bit 0 at either end adds an extra point 1.5 tangents beyond it. Then
// terrain +0x78 CreateRoad(texture, points, Step / 2, Width / 2,
// TexLength / 2, flags), or +0x7c SetRoad for an existing road.
void SMapRoad::Build(unsigned flags, bool create)
{
    if (!create && (flags & 2) == 0)
        return;
    SITerrain* t = g_World->Terrain;
    if (Texture < 0) {
        Texture = PzGepard()->LoadTexture(SStr(Name), 1, true);   // Gepard +0x44(name, 1, 1)
        int w = 0, h = 0;
        PzGepard()->GetTextureSize(Texture, &w, &h);              // Gepard +0x50
        Width = (float)h * 2.0f * 0.015625f;                      // +0x3c
        TexLength = (float)w * 2.0f * 0.015625f;                  // +0x38
    }
    int n = PointCount;
    int cap = n + 2;
    if (TerrainPointMax < cap) {
        TerrainPointMax = cap;
        TerrainPoints = realloc(TerrainPoints, cap * sizeof(SRoadPoint));
    }
    memset(TerrainPoints, 0, TerrainPointMax * sizeof(SRoadPoint));
    SRoadPoint* out = (SRoadPoint*)TerrainPoints;
    int k = 0;
    if (n > 0 && (Points[0].W & 1)) {
        SRoadPoint& o = out[k++];                                 // 0x5ef090: insert at 0
        o.X = Points[0].X - Points[0].DirX * 1.5f;
        o.Z = Points[0].Z - Points[0].DirZ * 1.5f;
        o.DirX = Points[0].DirX;
        o.DirZ = Points[0].DirZ;
        memcpy(&o.W, &Points[0].W, 4);
        o.Dirty = 1;
    }
    for (int i = 0; i < n; ++i) {
        SRoadPoint& o = out[k++];
        o.X = Points[i].X;
        o.Z = Points[i].Z;
        o.DirX = Points[i].DirX;
        o.DirZ = Points[i].DirZ;
        memcpy(&o.W, &Points[i].W, 4);
        o.Dirty = 1;
    }
    if (n > 0 && (Points[n - 1].W & 1)) {
        SRoadPoint& o = out[k++];
        o.X = Points[n - 1].X + Points[n - 1].DirX * 1.5f;
        o.Z = Points[n - 1].Z + Points[n - 1].DirZ * 1.5f;
        o.DirX = Points[n - 1].DirX;
        o.DirZ = Points[n - 1].DirZ;
        memcpy(&o.W, &Points[n - 1].W, 4);
        o.Dirty = 1;
    }
    TerrainPointCount = k;
    if (!t)
        return;
    SRoadPointArray arr = { out, TerrainPointCount, TerrainPointMax };
    if (Road < 0)
        Road = t->CreateRoad(Texture, &arr, Step * 0.5f, Width * 0.5f, TexLength * 0.5f, flags);   // terrain +0x78
    else
        t->SetRoad(Road, Texture, &arr, Step * 0.5f, Width * 0.5f, TexLength * 0.5f, flags);       // terrain +0x7c
}

// PANZERS 0x5d5420
void SMapRoadJunction::Release()
{
    if (Texture >= 0)
        PzGepard()->ReleaseTexture(Texture);                      // Gepard +0x48
    if (Junction >= 0 && g_World && g_World->Terrain)
        g_World->Terrain->DestroyRoadJunction(Junction);          // terrain +0x94
    free(Connections);
    Connections = nullptr;
    ConnectionCount = ConnectionMax = 0;
    FreeSString(&Name);
}

// PANZERS 0x5dd9f0
void SWorld::ClearRoadJunctions()
{
    for (int i = 0; i < Junctions.Size; ++i)
        if (Junctions.Array[i].Next == kHeapLive)
            Junctions.Array[i].Data.Release();
    Junctions.Size = 0;
    Junctions.Free = -1;
    Junctions.Count = 0;
}

// PANZERS 0x5f0b60
void SWorld::LoadRoadJunctions(SStream* s)
{
    ClearRoadJunctions();
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        Throw("Invalid array size");
    Junctions.Size = (int)n;
    if (Junctions.Max < (int)n) {
        Junctions.Max = (int)n;
        Junctions.Array = (SHeapElem<SMapRoadJunction>*)realloc(Junctions.Array, n * 0x68);
    }
    memset(Junctions.Array, 0, Junctions.Max * 0x68);
    Junctions.Free = s->ReadInt();
    Junctions.Count = s->ReadInt();
    for (int i = 0; i < Junctions.Size; ++i) {
        Junctions.Array[i].Next = s->ReadInt();
        if (Junctions.Array[i].Next == kHeapLive)
            Junctions.Array[i].Data.Load(s);
    }
}

// PANZERS 0x5f1590
void SMapRoadJunction::Load(SStream* s)
{
    Texture = -1;
    ReadWorldString(s, &Name);
    HalfX = ReadF(s);
    HalfZ = ReadF(s);
    Flags = (unsigned)s->ReadInt();
    X = ReadF(s);
    Z = ReadF(s);
    DirX = ReadF(s);
    DirZ = ReadF(s);
    F3c = ReadF(s);
    I48 = s->ReadInt();
    if (Name.size != 0) {
        // PANZERS 0x5eec00: reset the handles and build. HD also resets 12
        // default connections first (path finder, M2).
        Junction = -1;
        Texture = -1;
        Build(Flags);                                             // 0x6029b0
    }
    // PANZERS 0x5eff50: the connections (road index, bool), 0x34 each.
    unsigned nc = (unsigned)s->ReadInt();
    if (nc > 0x1000000)
        Throw("Invalid array size");
    ConnectionCount = (int)nc;
    if (ConnectionMax < (int)nc) {
        ConnectionMax = (int)nc;
        Connections = realloc(Connections, nc * 0x34);
    }
    if (ConnectionMax)
        memset(Connections, 0, ConnectionMax * 0x34);
    for (int i = 0; i < ConnectionCount; ++i) {
        unsigned char* c = (unsigned char*)Connections + i * 0x34;
        *(int*)(c + 0x28) = s->ReadInt();
        c[0x30] = s->ReadByte() != 0;
    }
}

// PANZERS 0x6029b0 (terrain part)
// Only with flags & 2. HD also computes the 12 connection end points around
// the junction for the path finder (M2). The terrain gets the centre and the
// direction: terrain +0x8c CreateRoadJunction(texture, point,
// texture height / 128, texture width / 128, flags), or +0x90 to update.
void SMapRoadJunction::Build(unsigned flags)
{
    if ((flags & 2) == 0)
        return;
    if (Texture < 0) {
        Texture = PzGepard()->LoadTexture(SStr(Name), 1, true);   // Gepard +0x44(name, 1, 1)
        int w = 0, h = 0;
        PzGepard()->GetTextureSize(Texture, &w, &h);              // Gepard +0x50
        HalfX = (float)h * 0.015625f;                             // +0x58
        HalfZ = (float)w * 0.015625f;                             // +0x5c
    }
    PointX = X;
    PointZ = Z;
    PointDirX = DirX;
    PointDirZ = DirZ;
    PointValid = 1;
    SITerrain* t = g_World->Terrain;
    if (!t)
        return;
    SRoadJunctionPoint pt;
    memset(&pt, 0, sizeof(pt));
    pt.X = PointX;
    pt.Z = PointZ;
    pt.DirX = PointDirX;
    pt.DirZ = PointDirZ;
    pt.Valid = PointValid;
    if (Junction < 0)
        Junction = t->CreateRoadJunction(Texture, &pt, HalfX * 0.5f, HalfZ * 0.5f, flags);      // terrain +0x8c
    else
        t->SetRoadJunction(Junction, Texture, &pt, HalfX * 0.5f, HalfZ * 0.5f, flags);           // terrain +0x90
}

// ---------------------------------------------------------------------------
// Weather, players

// PANZERS 0x5f1860
static void LoadWeather(SWeather* w, SStream* s)
{
    ReadWorldString(s, &w->Name);
    int n = s->ReadInt();
    if (n >= 0x15)
        Logger.g->Panic("SWeather::Load: Invalid lite array size.");
    memset(w->Lite, 0, sizeof(w->Lite));
    for (int i = 0; i < n; ++i)
        w->Lite[i] = s->ReadInt();
}

// PANZERS 0x5f0080
void SWorld::LoadWeathers(SStream* s)
{
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        Throw("Invalid array size");
    ClearWeathers((int)n);
    for (int i = 0; i < Weathers.Size; ++i)
        LoadWeather(&Weathers.Array[i], s);
}

// PANZERS 0x5f2ed0
void SWorld::LoadPlayers(SStream* s)
{
    for (int i = 0; i < 12; ++i) {
        int* p = (int*)Players[i];
        p[0] = s->ReadInt();
        p[1] = s->ReadInt();
        p[2] = s->ReadInt();
        p[3] = s->ReadInt();
        p[11] = s->ReadInt();
        p[12] = s->ReadInt();
        p[13] = s->ReadInt();
        p[14] = s->ReadInt();
        p[15] = s->ReadInt();
    }
}

// ---------------------------------------------------------------------------
// Entities

// PANZERS 0x5f2a50
void SWorld::LoadEntities(SStream* s)
{
    PZ_TRACE("SWorld::LoadEntities (0x5f2a50)");
    if (s->ReadInt() != kVer100)
        Throw("Unsupported entities version");
    while (!s->ReadChunkIsEnd()) {
        int tag = s->ReadChunkHeader();
        g_WorldStats.SubChunks++;
        switch (tag) {
        case 0x53444f44:     // DODS
            SetLoadingStep(1);                                    // board +0x24(.., 1), vp +0x50(0, 0)
            LoadAborted = false;
            LoadDoodads(s);
            for (int i = 0; i < Doodads.Size; ++i)
                if (Doodads.Array[i].Next == kHeapLive && Doodads.Array[i].Data.Name.size == 0)
                    RemoveDoodad(i);                              // 0x5f73f0
            g_WorldStats.Doodads = Doodads.Count;
            // HD 0x5dcb20(0): clears the class-name remap table (+0x74c8).
            SetLoadingStep(2);
            break;
        case 0x53434544: {   // DECS
            LoadAborted = false;
            LoadDecals(s);
            int i = 0;
            while (i < Decals.Size) {
                if (Decals.Array[i].Name.size == 0)
                    RemoveDecal(i);
                else
                    ++i;
            }
            break;
        }
        case 0x50474941:     // AIGP (0x56e2c0, then 0x55ccc0/0x601060 per group)
            AIGroupsLoad(this, s);                                // aigroup.cpp (M3-C)
            break;
        case 0x53424d41:     // AMBS
            LoadAmbientSounds(s);                                 // 0x5f02a0
            break;
        case 0x53444e55:     // UNDS
            SetLoadingStep(3);                                    // 0x5fedb0(3)
            LoadAborted = false;
            LoadUnitDefinitions(s);
            break;
        case 0x53464545:     // EEFS
            LoadEffects(s);
            break;
        case 0x53494e55:     // UNIS
            LoadAborted = false;
            LoadUnits(s);
            break;
        case 0x534b414c:     // LAKS (0x5f05c0)
            KeepRawChunk(s, tag, "lakes");
            break;
        default: {
            char t[5];
            TagText(tag, t);
            if (Logger.g)
                Logger.g->Warning("Unsupported entities sub chunk tag 0x%08X (%s)", tag, t);
            s->ReadChunkSkip();
            break;
        }
        }
        s->ReadChunkValidate(0);
    }
    // HD: 0x601060 for every AI group (M2).
    SetLoadingStep(4);
}

// PANZERS 0x5f3820
// UNIS: saved-game units (SHeapTRB of UNIT chunks). Not in maps/menu.map;
// M1 keeps the chunk only.
void SWorld::LoadUnits(SStream* s)
{
    KeepRawChunk(s, 0x53494e55, "saved units (not created in M1)");
}

// ---------------------------------------------------------------------------
// The map

// PANZERS 0x5f1990
bool SWorld::LoadMap(SStream* stream, bool p2, int p3, int p4)
{
    PZ_TRACE("SWorld::LoadMap (0x5f1990)");
    SStream* s = stream;
    // HD: (SMulti sync), board +0x24(LoadIconFrame, LoadIconSet, 0), then a
    // loading frame through the viewport with no scene.
    SetLoadingStep(0);
    LoadParam3 = p3;
    LoadParam4 = p4;
    // HD 0x65d8f0(0): stream string format flag; the world reads its strings
    // with 0x56e7d0, which ignores it.
    if (Minimap) {                                                // 0x5f1a2c: 0x669cc0 + delete 0x20
        HdBitmapDelete((SHdBitmap*)Minimap);
        Minimap = nullptr;
    }
    SString skybox;
    s->ReadSignature();                                           // 0x65d6a0
    if (s->ReadChunkHeader() != kTagMAPF)
        Throw("Not a map file");
    if (s->ReadInt() != kVer201)
        Throw("Unsupported map file version");
    while (!s->ReadChunkIsEnd()) {
        int tag = s->ReadChunkHeader();
        g_WorldStats.Chunks++;
        switch (tag) {
        case 0x47495254:     // TRIG (0x5f0140; executed by SGameLogic in M2)
            LoadTriggers(reinterpret_cast<STriggerArray<STrigger>*>(&Triggers), s);   // trigger.cpp (L)
            break;
        case 0x33524957:     // WIR3 (0x5f3d60 SWorld::LoadWires)
            KeepRawChunk(s, tag, "wires");
            break;
        case 0x32525652:     // RVR2 (0x5f0960)
            KeepRawChunk(s, tag, "rivers");
            break;
        case 0x204d4143: {   // "CAM "
            float cam[5];
            s->Read(cam, 0x14);
            if (p2)
                LoadCamera(cam);                                  // 0x5fd7c0
            break;
        }
        case 0x32444f52:     // ROD2
            LoadRoads(s);                                         // 0x5f0a30
            break;
        case 0x32594c50:     // PLY2 (0x5f2f50)
            KeepRawChunk(s, tag, "players (v2)");
            break;
        case 0x4259534b: {   // KSYB
            ReadWorldString(s, &skybox);
            // PANZERS 0x5fec10 (inline): skybox.
            Skybox = skybox;
            g_Scene->ClearSkybox();                               // scene +0x100
            if (Skybox.size != 0)
                g_Scene->SetSkybox(SStr(Skybox), 500.0f);         // scene +0xfc (0x43fa0000)
            break;
        }
        case 0x33594c50:     // PLY3
            LoadPlayers(s);
            break;
        case 0x414e494d:     // MINA
            ReadWorldString(s, &MinimapName);
            break;
        case 0x4554494c: {   // LITE
            int n = s->ReadInt();
            if (n > 0x14)
                Throw("Too many light parameters");
            ClearWeathers(1);
            s->Read(Weathers.Array[0].Lite, n * 4);
            SetSString(&Weathers.Array[0].Name, "Default");
            CurrentWeather = 0;
            WeatherBlendTime = 0.0f;
            WeatherBlend = 0.0f;
            SetWeather(0, 0);
            break;
        }
        case 0x534d5441:     // ATMS
            ReadWorldString(s, &Atmosphere);
            g_Scene->SetAtmosphere(SStr(Atmosphere));             // scene +0x08
            break;
        case 0x52524554:     // TERR
            LoadTerrain(s);
            break;
        case 0x53434f4c:     // LOCS
            LoadLocations(s);                                     // 0x5f0690 (trigger.cpp, L)
            break;
        case 0x53544e45:     // ENTS
            LoadEntities(s);
            break;
        case 0x52485457: {   // WTHR
            LoadWeathers(s);
            int w = s->ReadInt();
            CurrentWeather = w;
            WeatherBlendTime = 0.0f;
            WeatherBlend = 0.0f;
            SetWeather(w, 0);
            if (Logger.g)
                Logger.g->Log(0, "PZ3D world: WTHR %d weather(s), current %d \"%s\"", Weathers.Size, w,
                              (w >= 0 && w < Weathers.Size) ? SStr(Weathers.Array[w].Name) : "");
            for (int i = 0; Logger.g && i < Weathers.Size; ++i)       // recompile log (M6-WX)
                if (Weathers.Array[i].Lite[17] || Weathers.Array[i].Lite[18])
                    Logger.g->Log(0, "PZ3D world: weather %d \"%s\" rain %d snow %d", i,
                                  SStr(Weathers.Array[i].Name), Weathers.Array[i].Lite[17], Weathers.Array[i].Lite[18]);
            break;
        }
        case 0x4a444f52:     // RODJ (0x5f0b60, then 0x5f7fa0 per junction)
            LoadAborted = false;
            LoadRoadJunctions(s);                                 // 0x5f0b60
            // HD then checks every live junction with 0x5f7fa0 (removes
            // the invalid ones through RemoveRoadJunction 0x5f7770).
            break;
        case 0x48544150:     // PATH
            LoadPaths(s);                                         // 0x5f07b0 (trigger.cpp, L)
            break;
        case 0x494e494d:     // MINI: World +0x74bc (new 0x20, 0x669ca0, 0x66ea40)
            Minimap = HdBitmapLoad(s);
            break;
        case 0x52415654:     // TVAR
            LoadTriggerVariables(s);                              // 0x56e440 (trigger.cpp, L)
            break;
        default: {
            char t[5];
            TagText(tag, t);
            if (Logger.g)
                Logger.g->Warning("Unsupported map chunk tag 0x%08X (%s)", tag, t);
            s->ReadChunkSkip();
            break;
        }
        }
        s->ReadChunkValidate(0);
    }
    s->ReadChunkValidate(0);                                      // MAPF
    // The whole block map is rebuilt from the loaded words, the doodad
    // footprints and the units: 0x5ef380(0, 0, BlockW, BlockH, 0x7f3f), then
    // the water map (0x608600), then 0x604620.
    BlockMap_MarkDirty(this, 0, 0, BlockW, BlockH, 0x7f3f);       // 0x5ef380
    UpdateWaterMap();                                             // 0x608600
    RefreshBlockMapDirtyRect();                                   // 0x604620
    LoadParam3 = 0;
    LoadParam4 = 0;
    PzGepard()->PurgeModelPrototypes();                           // Gepard +0x28
    FreeSString(&skybox);
    return true;
}

} // namespace pz
