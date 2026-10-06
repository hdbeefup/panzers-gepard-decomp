// src/world/mapload.cpp
// SWorld::LoadMap (HD 0x5f1990, MAPF v201) and its chunk readers: terrain,
// entities (doodads, decals, placed effects, unit definitions), weather,
// players and camera. OWNER: agent D. Lifted from the HD exe only.
//
// Chunks M1 does not use (roads, junctions, rivers, locations, paths,
// triggers, trigger variables, wires, ambient sounds, lakes, AI groups, the
// minimap) are read whole and kept as raw copies for M2, with their counts
// logged. HD parses them into SWorld containers.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/iviewport.h"
#include "pz/ipixie.h"
#include "pz/iterrain.h"
#include "pz/imodel.h"
#include "stream.h"
#include "properties.h"
#include "logger.h"
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
            RuinModel->SetSequence(0, false);                     // model +0x30(0, 0)
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

// PANZERS 0x6012d0
void SDoodad::UpdatePosition()
{
    float y = g_World->GetTerrainHeight(X, Z) + YOffset;
    if (Model) {
        Model->SetPosition(X, y, Z);                              // model +0x18
        Model->SetRotation(Angle, TiltX, TiltZ);                  // model +0x1c
        // HD: model +0xc0(_28) and the "Block" rectangle into the block map
        // (model +0xa8(4, "Block"), 0x5ef380): slots pending (agent A) / M2.
    }
    if (RuinModel) {
        RuinModel->SetPosition(X, y, Z);
        RuinModel->SetRotation(Angle, TiltX, TiltZ);
    }
    // HD: attached lights (0x6090e0) - none in the menu map.
}

// PANZERS 0x5d4e50
void SDoodad::Release()
{
    // HD: attached lights (0x5f82b0), pixie +0x20 for the two demolish
    // effects (slot pending, agent C).
    if (Model) {
        Model->Release();                                         // model +0x04
        Model = nullptr;
    }
    if (RuinModel) {
        RuinModel->Release();
        RuinModel = nullptr;
    }
    BlockRect = nullptr;
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

// PANZERS 0x5edfd0
void SDecal::Create()
{
    Texture = PzGepard()->LoadTexture(SStr(Name), 1, true);       // Gepard +0x44(name, 1, 1)
    // HD: terrain +0x48(index, Texture, A, B, C) and terrain +0x54(index,
    // _18) place the decal: slots pending (agent B, SITerrain Slot_48/Slot_54).
    g_WorldStats.Decals++;
}

// PANZERS 0x5d6800
void SDecal::Release()
{
    // HD: terrain +0x4c(index) (slot pending, agent B), then Gepard +0x48.
    if (Name.buf && Texture >= 0)
        PzGepard()->ReleaseTexture(Texture);
    Texture = 0;
    FreeSString(&Name);
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
        if (g_Pixie && e->Data.Effect >= 0)
            g_Pixie->StopEffect(e->Data.Effect);                  // pixie +0x34
        // HD: pixie +0x20(Prototype): slot pending (agent C).
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
    // HD: Effect = pixie +0x2c(g_Scene, Prototype, pos, dir) (4 arg dwords),
    // then 0x6014e0. Slot pending (agent C, SIPixie Slot_2C).
    (void)dir; (void)pos;
    g_WorldStats.Effects++;
    if (Logger.g)
        Logger.g->Log(0, "PZ3D world: effect %s proto %d at %.2f %.2f %.2f dir %.3f %.3f %.3f",
                      SStr(Name), Prototype, pos[0], pos[1], pos[2], dir[0], dir[1], dir[2]);
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
            KeepRawChunk(s, tag, "AI groups");
            break;
        case 0x53424d41:     // AMBS (0x5f02a0)
            KeepRawChunk(s, tag, "ambient sounds");
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
    Minimap = nullptr;
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
            KeepRawChunk(s, tag, "triggers");
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
        case 0x32444f52:     // ROD2 (0x5f0a30; roads go to the terrain)
            KeepRawChunk(s, tag, "roads");
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
        case 0x53434f4c:     // LOCS (0x5f0690)
            KeepRawChunk(s, tag, "locations");
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
            break;
        }
        case 0x4a444f52:     // RODJ (0x5f0b60, then 0x5f7fa0 per junction)
            LoadAborted = false;
            KeepRawChunk(s, tag, "road junctions");
            break;
        case 0x48544150:     // PATH (0x5f07b0)
            KeepRawChunk(s, tag, "paths");
            break;
        case 0x494e494d:     // MINI (0x669ca0 bitmap, 0x66ea40 load)
            KeepRawChunk(s, tag, "minimap bitmap");
            break;
        case 0x52415654:     // TVAR (0x56e440)
            KeepRawChunk(s, tag, "trigger variables");
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
    // HD: 0x5ef380(0, 0, BlockW, BlockH, 0x7f3f) block-map flags,
    // 0x608600 SWorld::UpdateWaterMap, 0x604620 world refresh (M2).
    LoadParam3 = 0;
    LoadParam4 = 0;
    PzGepard()->PurgeModelPrototypes();                           // Gepard +0x28
    FreeSString(&skybox);
    return true;
}

} // namespace pz
