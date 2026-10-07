// src/world/world.cpp
// pz::SWorld: constructor, destructor, Initialize, loading icon, weather and
// camera. OWNER: agent D. Lifted from the HD exe only (SWINE world/ and game/
// are banned). Map loading is in mapload.cpp, units in unit.cpp.
//
// Where HD calls an interface slot that the owning agent has not named yet
// (Slot_XX), the call is left out and marked "slot pending".

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pz/hdbitmap.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/iviewport.h"
#include "pz/ipixie.h"
#include "pz/iterrain.h"
#include "pz/imodel.h"
#include "iboard.h"
#include "iconcert.h"
#include "properties.h"
#include "logger.h"
#include "timer.h"
#include "stub_log.h"

extern SIBoard* Board;   // window/widget.h (HD Board 0x8f1c60)
extern SIConcert* Concert;   // HD 0x8f1c5c

namespace pz {

SWorldLoadStats g_WorldStats;

// Recompile-only storage for the terrain buffers when the scene returns no
// terrain (agent B's STerrain still a stub): HD always reads straight into
// the buffers STerrain::Acquire hands out. See mapload.cpp.
void FreeFallbackTerrainBuffers();
// Raw copies of the map chunks M1 parses but does not use (mapload.cpp).
void FreeMapRawChunks();

// HD logs these lines at level 1 (hidden at the default level 0); with the
// -menu3d trace on they are shown, so the load/unload order can be checked.
static int HdLogLevel()
{
    return g_Menu3D.Trace ? 0 : 1;
}

static double WorldTimerSeconds()
{
    // HD 0x661800: timer seconds.
    return (double)Timer.GetTickValue() / 1000.0;
}

void LogWorldStats(const char* when)
{
    const SWorldLoadStats& s = g_WorldStats;
    if (!Logger.g)
        return;
    Logger.g->Log(0, "PZ3D world totals (%s): %d chunks + %d sub chunks; doodads %d (models %d requested, %d created, %d ruins); "
                  "decals %d; effects %d (%d effect prototypes); unit defs %d, units created %d (%d incl. stored/members), "
                  "unit models %d wanted / %d created; model prototypes %d requested / %d loaded",
                  when, s.Chunks, s.SubChunks, s.Doodads, s.DoodadModels, s.DoodadModelsOk, s.Ruins,
                  s.Decals, s.Effects, s.EffectProtos, s.UnitDefs, s.UnitsCreated, s.UnitsTotal,
                  s.UnitModels, s.UnitModelsOk, s.Prototypes, s.PrototypesOk);
}

int WorldLoadModelPrototype(const char* file)
{
    // Gepard +0x20 LoadModelPrototype(file, 0.005f, 0, 0) (0x3ba3d70a).
    int proto = PzGepard()->LoadModelPrototype(file, 0.005f, nullptr, 0);
    g_WorldStats.Prototypes++;
    if (proto >= 0)
        g_WorldStats.PrototypesOk++;
    if (g_Menu3D.Trace && Logger.g)
        Logger.g->Log(0, "PZ3D world: prototype %s -> %d", file, proto);
    return proto;
}

// PANZERS 0x5d2f90
SWorld::SWorld(int p1)
{
    PZ_TRACE("SWorld::SWorld (0x5d2f90)");
    memset(&g_WorldStats, 0, sizeof(g_WorldStats));   // recompile: M1 counters
    // HD zeroes or -1s each container header field by field. Everything the
    // ctor leaves alone is zero in a fresh 0x7538 allocation here.
    memset((unsigned char*)this + sizeof(void*), 0, sizeof(SWorld) - sizeof(void*));
    Doodads.Free = -1;                 // param_1[0x53]
    Units.Free = -1;                   // param_1[0x138]
    Units.FreeTail = -1;               // param_1[0x139]
    AIGroups.Free = -1;                // param_1[0x140]
    Effects.Free = -1;                 // param_1[0x1cf7]
    // Other heaps the menu path does not use (-1 free heads at +0x164,
    // +0x664, +0x7278, +0x73bc, +0x73f4, +0x7408, +0x741c, +0x7438, +0x744c,
    // +0x7460, +0x748c, +0x74a0, +0x74b4).
    static const int kFreeHeads[] = { 0x164, 0x664, 0x7278, 0x73bc, 0x73f4, 0x7408, 0x741c,
                                      0x7438, 0x744c, 0x7460, 0x748c, 0x74a0, 0x74b4 };
    for (int off : kFreeHeads)
        *(int*)((unsigned char*)this + off) = -1;

    if (g_World)
        delete g_World;                // vtbl +0 (1)
    g_World = this;
    CamNear = 1.0f;                    // param_1[0xb] = 0x3f800000
    CamFar = 100.0f;                   // param_1[0xc] = 0x42c80000
    Param = p1;

    // 12 player slots (0x48 bytes from +0x170).
    for (int i = 0; i < 12; ++i) {
        int* p = (int*)Players[i];
        p[0] = i;                      // +0x00 index
        p[1] = 0;
        p[2] = 2;
        p[3] = 0;
        Players[i][0x11] = 0;
        p[5] = 0;
        p[6] = i;
        p[11] = p[12] = p[13] = p[14] = p[15] = 0;
        // HD 0x661800 (timer), truncated by the CRT double -> int helper
        // 0x7669f1 (not rand: no CRT draw here; docs/M3_INTERFACES.md 7).
        p[16] = (int)WorldTimerSeconds();
        p[17] = 0;
    }
    *(int*)(Players[0] + 0x08) = 0;    // param_1[0x5e]
    *(int*)(Players[0] + 0x0c) = 1;    // param_1[0x5f]
    *(int*)(Players[0] + 0x50) = 1;    // param_1[0x70]
    *(int*)(Players[0] + 0x54) = 2;    // param_1[0x71]
    *(short*)((unsigned char*)this + 0x4d0) = 1;   // param_1[0x134]
    // HD 0x5d8fc0: one entry in the trigger variable heap at +0x74a8
    // ("Time"); the triggers are M2 and this heap is not kept.

    g_Scene = PzGepard()->CreateScene();                          // Gepard +0x0c ("Scene created")
    ClearWeathers(1);                                             // 0x5dd010(1)
    SetDefaultWeather(0);                                         // 0x5fdaf0(0)
    SetWeather(0, 0);                                             // 0x5fdc80(0, 0)
    SetSString(&Weathers.Array[0].Name, "Default");
    CurrentWeather = 0;
    WeatherBlendTime = 0.0f;
    WeatherBlend = 0.0f;
    g_Scene->SetAtmosphere(SStr(Atmosphere));                     // scene +0x08
    *(int*)((unsigned char*)this + 0x638) = -1;
    RainFx = -1;
    *(int*)((unsigned char*)this + 0x648) = -1;
    SnowFx = -1;
    // HD: Concert +0x24("sounds/ambient/eso/eso60stereo - eros.mp3", 0) ->
    // +0x63c, +0x640 = -1. Ambient sound is not part of M1 (SWINE concert).
    *(int*)((unsigned char*)this + 0x63c) = -1;
    *(int*)((unsigned char*)this + 0x640) = -1;
    // Board +0x74 "menu/selection_hq.tga" (0x38 glyphs), "menu/insignils_hq.tga",
    // "menu/multiplayer_insignils_hq.tga" and the four drag-box frames
    // (board +0x08): src/game/unitboard.cpp.
    BoardIconSet = Insignia = MultiInsignia = -1;
    for (int i = 0; i < 4; ++i)
        BoardFrames[i] = -1;
    WorldCreateBoardElements(this);
    SelectionTextures[0] = PzGepard()->LoadTexture("shaders/10 Misc-Egyeb/kijelolo keret_alul_a.tga", 1, true);
    SelectionTextures[1] = PzGepard()->LoadTexture("shaders/10 Misc-Egyeb/kijelolo keret_alul_csik2_a.tga", 1, true);
    SelectionTextures[2] = PzGepard()->LoadTexture("shaders/10 Misc-Egyeb/kijelolo keret_alul_csik1_a.tga", 1, true);
    SelectionTextures[3] = PzGepard()->LoadTexture("shaders/10 Misc-Egyeb/nyil.tga", 1, true);
    _13c = false;
    CamFollowPath = 0;
    CamFollowUnit = -1;
    CamLocked = 0;
    CamMode = 0;
    CamProjectionDirty = true;
    Minimap = nullptr;
    ObjectsIni = new SProperties("objects.ini", true);            // new 0x1c, 0x65fe80
    static const char* const kFlags[3] = {
        "units/flag/german-hero.4D", "units/flag/US-hero.4D", "units/flag/Russian-hero.4D" };
    for (int i = 0; i < 3; ++i) {
        FlagProto[i] = WorldLoadModelPrototype(kFlags[i]);        // Gepard +0x20(file, 0.005, 0, 0)
        if (FlagProto[i] < 0 && Logger.g)                         // HD panics here
            Logger.g->Log(0, "SWorld::SWorld: LoadModelPrototype failed: %s (HD panics; agent A loader pending)", kFlags[i]);
    }
    WireTearFx = -1;                                              // param_1[0x1d0a]
    LoadParam3 = LoadParam4 = 0;
    Obj74e4 = Obj74e8 = nullptr;
    RangeUnit = -1;
    // Gepard +0x44 (the weapon range overlay of the selected unit,
    // ShowUnitRange 0x5fee00). The facade's LoadTexture adds the ".tga".
    RangeTexture = PzGepard()->LoadTexture("shaders\\10 Misc-Egyeb\\range_a", 0, true);
    LoadIconSet = -1;
    LoadIconFrame = -1;
    LoadIconParent = 0;
    InitSpeechCounts();                // 0x5edd00 (combat_speech.cpp)
    if (Logger.g)
        Logger.g->Log(HdLogLevel(), "World created");
}

// PANZERS 0x5d5510
SWorld::~SWorld()
{
    PZ_TRACE("SWorld::~SWorld (0x5d68b0)");
    if (Logger.g)
        Logger.g->Log(HdLogLevel(), "SWorld::~SWorld");
    for (int i = 0; i < 3; ++i)
        if (FlagProto[i] >= 0)
            PzGepard()->ReleaseModelPrototype(FlagProto[i]);      // Gepard +0x24
    // HD 0x5ffe70 (selection), then every live doodad.
    for (int i = 0; i < Doodads.Size; ++i)
        if (Doodads.IsLive(i))
            RemoveDoodad(i);
    for (int i = 0; i < Units.Size; ++i)
        if (Units.IsLive(i))
            RemoveUnit(i);                                        // 0x5f8060
    for (int i = Decals.Size - 1; i >= 0; --i)
        RemoveDecal(i);                                           // 0x5f7170
    ClearEffects();                                               // HD removes each (0x5f75c0)
    ClearRoads();                                                 // 0x5dd9a0
    ClearRoadJunctions();                                         // 0x5dd9f0
    // Rivers, wires, locations and paths are kept as raw chunks in M1
    // (mapload.cpp).
    FreeMapRawChunks();
    if (Minimap) {                                                // 0x5d59f5: 0x669cc0 + delete 0x20
        HdBitmapDelete((SHdBitmap*)Minimap);
        Minimap = nullptr;
    }
    if (g_Pixie) {
        g_Pixie->ReleaseEffectPrototype(WireTearFx);              // pixie +0x20 (+0x7428)
        g_Pixie->ReleaseEffectPrototype(SnowFx);                  // +0x644 (no snow effect playing)
        g_Pixie->ReleaseEffectPrototype(RainFx);                  // +0x634
    }
    if (Terrain) {
        g_Scene->DestroyTerrain();                                // scene +0x68
        Terrain = nullptr;
    }
    FreeFallbackTerrainBuffers();
    for (int i = 0; i < 4; ++i)
        if (SelectionTextures[i] >= 0)
            PzGepard()->ReleaseTexture(SelectionTextures[i]);     // Gepard +0x48
    WorldReleaseBoardElements(this);                              // board +0x0c x4, +0x80 x3
    if (RangeTexture >= 0)
        PzGepard()->ReleaseTexture(RangeTexture);
    for (int i = 0; i < Layers.Size; ++i) {
        FreeSString(&Layers.Array[i].Name);
        FreeSString(&Layers.Array[i].Extra);
    }
    free(Layers.Array);
    for (int i = 0; i < Weathers.Size; ++i)
        FreeSString(&Weathers.Array[i].Name);
    free(Weathers.Array);
    free(Doodads.Array);
    free(Units.Array);
    free(Decals.Array);
    free(Effects.Array);
    free(Roads.Array);
    free(Junctions.Array);
    if (Logger.g)
        Logger.g->Log(HdLogLevel(), "SWorld::~SWorld: releasing scene");
    if (g_Scene) {
        g_Scene->Release();                                       // scene +0x04 ("Scene destroyed")
        g_Scene = nullptr;
    }
    // HD: Concert +0x0c(1).
    if (ObjectsIni) {
        delete ObjectsIni;                                        // 0x660080, delete 0x1c
        ObjectsIni = nullptr;
    }
    if (g_World == this)
        g_World = nullptr;
    if (Logger.g)
        Logger.g->Log(HdLogLevel(), "World destroyed");
    FreeSString(&Skybox);
    FreeSString(&Atmosphere);
    FreeSString(&MinimapName);
}

// HD 0x5ec2a0 / 0x5fec80 / 0x5ec0c0 / 0x5ebee0: not reached by the menu.
void SWorld::Slot_04()
{
    STUB_LOG("SWorld::Slot_04 (0x5ec2a0)");
    PZ_TRACE("SWorld::Slot_04 (0x5ec2a0)");
}

void SWorld::Slot_08()
{
    STUB_LOG("SWorld::Slot_08 (0x5fec80)");
    PZ_TRACE("SWorld::Slot_08 (0x5fec80)");
}

void SWorld::Slot_0C()
{
    STUB_LOG("SWorld::Slot_0C (0x5ec0c0)");
    PZ_TRACE("SWorld::Slot_0C (0x5ec0c0)");
}

void SWorld::Slot_10()
{
    STUB_LOG("SWorld::Slot_10 (0x5ebee0)");
    PZ_TRACE("SWorld::Slot_10 (0x5ebee0)");
}

// PANZERS 0x5edca0
void SWorld::ShowLoadingIcon(int parentFrame)
{
    PZ_TRACE("SWorld::ShowLoadingIcon (0x5edca0)");
    // HD board +0x70(name, 0xdd, 0x42, 1, 5, 0): 1x5 glyph sheet; the SWINE
    // board expresses it as a fixed font. board +0x08(1, parent, 0x191, 0x2be, 0, 1).
    LoadIconSet = Board->LoadFixedFont("menu/panzers_loading_icons_hq.tga", 0xdd, 0x42, 1, 5, nullptr, Default);
    LoadIconFrame = Board->CreateFrame(FT_SPRITE, parentFrame, 0x191, 0x2be, 0, true);
    LoadIconParent = parentFrame;
}

// PANZERS 0x5dc7d0
void SWorld::HideLoadingIcon()
{
    PZ_TRACE("SWorld::HideLoadingIcon (0x5dc7d0)");
    if (LoadIconFrame >= 0) {
        Board->DestroyFrame(LoadIconFrame);                       // board +0x0c
        LoadIconFrame = -1;
    }
    if (LoadIconSet >= 0) {
        Board->ReleaseFont(LoadIconSet);                          // board +0x80
        LoadIconSet = -1;
    }
}

// PANZERS 0x5fedb0
void SWorld::SetLoadingStep(int step)
{
    // HD: (SMulti sync), board +0x24(frame, set, step), then a loading frame.
    if (LoadIconFrame >= 0 && LoadIconSet >= 0)
        Board->SetSpriteGlyph(LoadIconFrame, LoadIconSet, step);
    if (SIViewport* vp = PzGepard()->GetViewport(0))
        vp->Render(nullptr, 0);
}

// PANZERS 0x5eec90
void SWorld::Initialize()
{
    PZ_TRACE("SWorld::Initialize (0x5eec90)");
    if (Logger.g)
        Logger.g->Log(HdLogLevel(), "SWorld::Initialize: %d doodads", Doodads.Count);
    // HD: new 0xa4 (0x5a1460(BlockW, BlockH)) at +0x74e4 and new 0xe0 at
    // +0x74e8: path-finding grids over the block map (M2, not created).
    if (g_Pixie) {
        RainFx = g_Pixie->LoadEffectPrototype("Effects/Atmosphere/Rain.fx", false, false, 0, 0);       // pixie +0x10
        SnowFx = g_Pixie->LoadEffectPrototype("Effects/Atmosphere/Snowfall.fx", false, false, 0, 0);
        WireTearFx = g_Pixie->LoadEffectPrototype("effects/extras/wiretear.fx", false, false, 0, 0);
        g_WorldStats.EffectProtos += 3;
    }
    RebuildTerrain();                                             // 0x6043a0
    // HD 0x5ecdf0 InitFlyingFox (983 insns): M2, not called.
}

// PANZERS 0x608360
// Hands the TLAY layers to the terrain: for each of the 17 layer slots the
// base texture "tiles/<name>" (terrain +0x04; the terrain tries
// "<base>_%d_a.tga", then "<base>_%d.tga"), and for i > 0 the flora directory
// "flora/<extra>/" (terrain +0x08, scale 0.005, only when attr & 2 and the
// name is set) and the layer flags (terrain +0x34). Slots past the TLAY count
// are cleared. invalidate: terrain +0x20 over the whole map per layer.
void SWorld::SetTerrainLayers(bool invalidate)
{
    if (!Terrain)
        return;
    for (int i = 0; i < 17; ++i) {
        if (i < Layers.Size) {
            STerrainLayer* l = &Layers.Array[i];
            char base[300];
            _snprintf(base, sizeof(base) - 1, "tiles/%s", SStr(l->Name));   // "tiles/" + name (0x52c4a0)
            base[sizeof(base) - 1] = 0;
            Terrain->LoadLayerTexture(i, base);                   // terrain +0x04
            if (i > 0) {
                if ((l->Attributes & 2) == 0 || l->Extra.size == 0) {
                    Terrain->LoadFloraLayer(i - 1, nullptr, 0.005f);   // terrain +0x08 (0x3ba3d70a)
                } else {
                    char dir[300];
                    _snprintf(dir, sizeof(dir) - 1, "flora/%s/", SStr(l->Extra));   // 0x52da80
                    dir[sizeof(dir) - 1] = 0;
                    Terrain->LoadFloraLayer(i - 1, dir, 0.005f);
                }
                Terrain->SetLayerFlags(i - 1, (int)l->Attributes);   // terrain +0x34
            }
            if (invalidate)
                Terrain->Invalidate(0, 0, TerrainW, TerrainH);    // terrain +0x20
        } else {
            // HD calls these for i = 0 too (layer -1) when TLAY is empty;
            // the recompile skips the negative layer.
            Terrain->LoadLayerTexture(i, nullptr);
            if (i > 0) {
                Terrain->LoadFloraLayer(i - 1, nullptr, 0.005f);
                Terrain->SetLayerFlags(i - 1, 0);
            }
        }
    }
}

// PANZERS 0x6043a0
// After the map is loaded (SWorld::Initialize): layers, a full terrain
// invalidate (terrain +0x20(0, 0, w + 1, h + 1)), the map decals
// (terrain +0x5c), every road (+0x88) and junction (+0x98). Rivers 0x608f40
// and DWires 0x605930 are not in menu.map; the block-map dirty rectangle
// (+0x7500..+0x7514) belongs to the M2 path finder.
void SWorld::RebuildTerrain()
{
    PZ_TRACE("SWorld::RebuildTerrain (0x6043a0)");
    if (!Terrain)
        return;
    SetTerrainLayers(false);                                      // 0x608360(0)
    Terrain->Invalidate(0, 0, TerrainW + 1, TerrainH + 1);        // terrain +0x20
    Terrain->UpdateDecals();                                      // terrain +0x5c
    for (int i = 0; i < Roads.Size; ++i)
        if (Roads.IsLive(i) && Roads.Array[i].Data.Road >= 0)
            Terrain->UpdateRoad(Roads.Array[i].Data.Road);        // terrain +0x88
    for (int i = 0; i < Junctions.Size; ++i)
        if (Junctions.IsLive(i) && Junctions.Array[i].Data.Junction >= 0)
            Terrain->UpdateRoadJunction(Junctions.Array[i].Data.Junction);   // terrain +0x98
}

// PANZERS 0x576d80 (M1 subset)
// The model part of SGameLogic::Refresh, once per 20 Hz logic tick: every
// unit (World+0x4d4; HD unit vtbl +0x2c -> the unit animation's UpdateModel)
// and every doodad model (World+0x140, model vtbl +0x3c).
void SWorld::RefreshModels()
{
    // As 0x576d80: every unit stores its pose (+0x16c) before the models are
    // posed for the new tick (+0x3c); no ServerRefresh without -m2.
    for (int i = 0; i < Units.Size; ++i)
        if (Units.IsLive(i) && Units.Array[i].Unit)
            Units.Array[i].Unit->StoreInterpolationState();
    for (int i = 0; i < Units.Size; ++i)
        if (Units.IsLive(i) && Units.Array[i].Unit)
            Units.Array[i].Unit->RefreshModel();
    for (int i = 0; i < Doodads.Size; ++i) {
        if (Doodads.Array[i].Next != kHeapLive)
            continue;
        SDoodad& d = Doodads.Array[i].Data;
        if (d.Model)
            d.Model->StoreInterpolationState();                   // model +0x3c
    }
}

// ---------------------------------------------------------------------------
// Weather

// PANZERS 0x5dd010
void SWorld::ClearWeathers(int newSize)
{
    for (int i = 0; i < Weathers.Size; ++i)
        FreeSString(&Weathers.Array[i].Name);
    Weathers.Size = newSize;
    if (Weathers.Max < newSize) {
        Weathers.Max = newSize;
        Weathers.Array = (SWeather*)realloc(Weathers.Array, newSize * sizeof(SWeather));
    }
    memset(Weathers.Array, 0, Weathers.Max * sizeof(SWeather));
}

// PANZERS 0x5fdaf0
void SWorld::SetDefaultWeather(int index)
{
    if (index < 0 || index >= Weathers.Size) {
        Logger.g->Panic("SWorld::SetDefaultWeather: Invalid index.");
    }
    int* lite = Weathers.Array[index].Lite;
    memset(lite, 0, sizeof(Weathers.Array[index].Lite));
    lite[2] = 0x2e;
    lite[5] = 0x40;
    lite[6] = 0x3c;
    lite[7] = 0x2d;
    lite[8] = 0xce;
    lite[9] = 0x32;
    lite[10] = 0x3c;
}

// PANZERS 0x5ec6c0
// Hue (degrees), saturation and value (percent) to RGB.
static void HsvToRgb(int h, int s, int v, float* r, float* g, float* b)
{
    float rr = 1.0f, gg, bb;
    if (h < 60) {
        bb = 0.0f;
        gg = (float)h / 60.0f;
    } else {
        gg = 1.0f;
        if (h < 120) {
            rr = (120.0f - (float)h) / 60.0f;
            bb = 0.0f;
        } else if (h < 180) {
            rr = 0.0f;
            bb = ((float)h - 120.0f) / 60.0f;
        } else {
            bb = 1.0f;
            if (h < 240) {
                rr = 0.0f;
                gg = (240.0f - (float)h) / 60.0f;
            } else {
                gg = 0.0f;
                if (h < 300)
                    rr = ((float)h - 240.0f) / 60.0f;
                else
                    bb = (360.0f - (float)h) / 60.0f;
            }
        }
    }
    float sat = (float)s / 100.0f;
    float val = (float)v / 100.0f;
    float inv = 1.0f - sat;
    *r = (sat * rr + inv) * val;
    *g = (sat * gg + inv) * val;
    *b = (sat * bb + inv) * val;
}

// Weather light block (HD SWorld +0x550..+0x630): the target values
// (+0x59c..+0x5e4), the blend start (+0x550..+0x598) and the applied values
// (+0x5e8..+0x630). 18 dwords each.
struct SWeatherLights {
    float Ambient[4];   // +0x00
    float Sun[4];       // +0x10
    float Fog[4];       // +0x20
    float SunAngle1;    // +0x30
    float SunAngle2;    // +0x34
    float FogStart;     // +0x38
    float FogEnd;       // +0x3c
    float FogDensity;   // +0x40
    float Rain;         // +0x44
    float Snow;         // +0x48
};
static_assert(sizeof(SWeatherLights) == 0x4c, "weather light block 0x4c");

// PANZERS 0x6088f0
// Applies the weather lights to the scene. (Rain/snow effects: the menu has
// neither, and pixie +0x2c is a pending slot.)
static void ApplyWeatherLights(SWorld* w)
{
    unsigned char* base = (unsigned char*)w;
    SWeatherLights* target = (SWeatherLights*)(base + 0x59c);
    SWeatherLights* out = (SWeatherLights*)(base + 0x5e8);
    if (w->WeatherBlendTime <= 0.0f) {
        *out = *target;
    } else {
        // Timed blend between weathers (+0x550 start): not used by the menu.
        *out = *target;
        w->WeatherBlendTime = 0.0f;
        w->WeatherBlend = 0.0f;
    }
    PzGepard()->SetOption(0x10, 1);                               // Gepard +0x10(0x10, 1)
    g_Scene->SetAmbientLight(out->Ambient);                       // scene +0x3c
    g_Scene->SetSunLight(out->Sun, out->SunAngle1, out->SunAngle2);   // scene +0x44
    g_Scene->SetFog(out->Fog[0], out->Fog[1], out->Fog[2], out->FogStart, out->FogEnd, out->FogDensity);   // scene +0x4c
}

// PANZERS 0x5fdc80
void SWorld::SetWeather(int index, int blend)
{
    if (index >= 0 && index < Weathers.Size) {
        CurrentWeather = index;
        WeatherBlend = 0.0f;
        if (blend == 0) {
            WeatherBlendTime = 0.0f;
        } else {
            WeatherBlendTime = (float)blend;
            memcpy((unsigned char*)this + 0x550, (unsigned char*)this + 0x5e8, 0x4c);
        }
        const int* lite = Weathers.Array[index].Lite;
        SWeatherLights* t = (SWeatherLights*)((unsigned char*)this + 0x59c);
        HsvToRgb(lite[0], lite[1], lite[2], &t->Ambient[0], &t->Ambient[1], &t->Ambient[2]);
        HsvToRgb(lite[3], lite[4], lite[5], &t->Sun[0], &t->Sun[1], &t->Sun[2]);
        t->SunAngle1 = (float)lite[6] * 0.017453292f;     // 0x7f59a0
        t->SunAngle2 = (float)lite[7] * -0.017453292f;    // 0x7f8418
        HsvToRgb(lite[8], lite[9], lite[10], &t->Fog[0], &t->Fog[1], &t->Fog[2]);
        t->FogStart = (float)lite[11];
        t->FogEnd = (float)lite[12];
        t->FogDensity = (float)lite[19] * 0.01f;          // 0x7f1b48
        t->Rain = (float)lite[17];
        t->Snow = (float)lite[18];
    }
    ApplyWeatherLights(this);
}

// ---------------------------------------------------------------------------
// Terrain queries

// PANZERS 0x5e7730
float SWorld::GetTerrainHeight(float x, float z)
{
    if (!(x >= 0.0f && x < (float)TerrainW && z >= 0.0f && z < (float)TerrainH))
        return 0.0f;
    const float* h = UseAltHeights ? AltHeights : Heights;
    if (!h)
        return 0.0f;
    int ix = (int)floorf(x);              // fistp with RC = down (0x47f)
    int iz = (int)floorf(z);
    float fx = x - (float)ix;
    float fz = z - (float)iz;
    int i = (TerrainW + 1) * iz + ix;
    int j = TerrainW + i;
    return h[i + 1] * (1.0f - fz) * fx + h[i] * (1.0f - fz) * (1.0f - fx)
         + h[j + 1] * fz * (1.0f - fx) + h[j + 2] * fz * fx;
}

// PANZERS 0x5ec490
float SWorld::GetWaterHeight(float x, float z)
{
    if (!(x >= 0.0f && x < (float)TerrainW && z >= 0.0f && z < (float)TerrainH))
        return 0.0f;
    const float* h = WaterHeights;
    if (!h)
        return 0.0f;
    int ix = (int)floorf(x);
    int iz = (int)floorf(z);
    float fx = x - (float)ix;
    float fz = z - (float)iz;
    int i = (TerrainW + 1) * iz + ix;
    int j = TerrainW + i;
    return h[i + 1] * (1.0f - fz) * fx + h[i] * (1.0f - fz) * (1.0f - fx)
         + h[j + 1] * fz * (1.0f - fx) + h[j + 2] * fz * fx;
}

// ---------------------------------------------------------------------------
// Camera

// PANZERS 0x5efb40
void SWorld::SetCameraLimits(bool p1)
{
    int w;
    if (!p1 || CamMode == 2) {
        CamPitchMin = -1.5707964f;          // 0xbfc90fdb
        CamPitchMax = 0.0f;
        if (CamMode >= 0) {
            if (CamMode < 2) {
                CamDistMin = 0.0f;
                CamDistMax = 1000.0f;       // 0x447a0000
            } else if (CamMode == 2) {
                CamDistMin = 0.3926991f;    // 0x3ec90fdb
                CamDistMax = 2.0943952f;    // 0x40060a92
            }
        }
        w = TerrainW;
    } else {
        w = TerrainW;
        CamPitchMin = -1.1344640f;          // 0xbf91361e (-65 deg)
        CamPitchMax = -0.5235988f;          // 0xbf060a92 (-30 deg)
        CamDistMin = 10.0f;
        CamDistMax = 30.0f;
        if (w > 0x60 && TerrainH > 0x60) {
            CamXMin = 48.0f;
            CamZMin = 48.0f;
            CamXMax = (float)(w - 0x30);
            CamZMax = (float)(TerrainH - 0x30);
            return;
        }
    }
    CamZMin = 0.0f;
    CamXMin = 0.0f;
    CamXMax = (float)w;
    CamZMax = (float)TerrainH;
}

// PANZERS 0x5f83b0
void SWorld::SetCameraAngles(float yaw, float pitch)
{
    CamYaw = yaw;
    float sy = (float)sin((double)yaw);     // 0x78d640
    float cy = (float)cos((double)yaw);     // 0x78d480
    CamYawSin = sy;
    CamYawCos = cy;
    float lo, hi;
    if (CamMode == 0 || CamMode == 1) {
        lo = CamPitchMin;
        hi = CamPitchMax;
    } else {
        lo = -1.57f;                        // 0x801b18
        hi = 1.57f;                         // 0x801ae4
    }
    if (pitch < lo)
        pitch = lo;
    else if (pitch > hi)
        pitch = hi;
    CamPitch = pitch;
    float cp = (float)cos((double)pitch);
    float sp = (float)sin((double)pitch);
    CamUp[1] = cp;
    CamForward[1] = sp;
    CamForward[0] = sy * cp;
    CamUp[0] = -(sy * sp);
    CamForward[2] = cy * cp;
    CamUp[2] = -(cy * sp);
    CamRight[0] = CamUp[2] * CamForward[1] - cp * CamForward[2];
    CamRight[1] = CamUp[0] * CamForward[2] - CamUp[2] * CamForward[0];
    CamRight[2] = CamUp[1] * CamForward[0] - CamUp[0] * CamForward[1];
}

static float ClampF(float v, float lo, float hi)
{
    if (v < lo || hi < v)
        return (v < lo) ? lo : hi;
    return v;
}

// PANZERS 0x5ecc20
void SWorld::ResetCamera()
{
    SetCameraLimits(true);
    if (CamMode == 0) {
        float x = (float)(TerrainW / 2);
        float z = (float)(TerrainH / 2);
        if (CamLocked == 0) {
            x = ClampF(x, CamXMin, CamXMax);
            z = ClampF(z, CamZMin, CamZMax);
            CamTarget[0] = x;
            CamTarget[2] = z;
            CamFollowPath = 0;
            CamFollowUnit = -1;
        }
        SetCameraAngles(0.0f, (CamPitchMax + CamPitchMin) * 0.5f);
        float d = (CamDistMin + CamDistMax) * 0.5f;
        float lo = CamDistMin;
        if (lo <= d) {
            lo = d;
            if (CamDistMax < d)
                lo = CamDistMax;
        }
        CamDist = lo;
    }
    if (CamMode == 2) {
        // HD: cinematic start above the map centre; not used by the menu.
        int x = TerrainW / 2;
        CamEye[0] = (float)x;
        CamEye[1] = GetTerrainHeight((float)x, (float)(TerrainH / 2)) + 20.0f;
        CamEye[2] = (float)(TerrainH / 2);
        SetCameraAngles(0.0f, (CamPitchMax + CamPitchMin) * 0.5f);
        float d = CamDistMin;
        if (d <= 1.0471976f) {
            d = 1.0471976f;
            if (CamDistMax < 1.0471976f)
                d = CamDistMax;
        }
        CamDist = d;
    }
    CamTime = 0.0;
}

// PANZERS 0x5fd7c0
void SWorld::LoadCamera(const float* cam)
{
    float x = cam[0];
    float z = cam[1];
    if (CamLocked == 0) {
        x = ClampF(x, CamXMin, CamXMax);
        z = ClampF(z, CamZMin, CamZMax);
        CamTarget[0] = x;
        CamTarget[2] = z;
        CamFollowPath = 0;
        CamFollowUnit = -1;
    }
    SetCameraAngles(cam[2], cam[3]);
    float d = cam[4];
    if (d >= CamDistMin && d <= CamDistMax)
        CamDist = d;
    else
        CamDist = (d < CamDistMin) ? CamDistMin : CamDistMax;
    if (Logger.g)
        Logger.g->Log(0, "PZ3D world: CAM target %.3f %.3f yaw %.3f pitch %.3f dist %.3f -> target %.3f %.3f pitch %.3f dist %.3f",
                      cam[0], cam[1], cam[2], cam[3], cam[4], CamTarget[0], CamTarget[2], CamPitch, CamDist);
}

// PANZERS 0x5ddc30
void SWorld::ComputeCamera(SIViewport* vp)
{
    PZ_TRACE("SWorld::ComputeCamera (0x5ddc30)");
    if (!vp)
        return;
    if (CamMode == 0) {
        if (CamFollowPath != 0) {
            // HD 0x5f57f0 / 0x5f4f60: follow a camera path (M2).
            CamFollowPath = 0;
        }
        if (CamFollowUnit >= 0) {
            if (!Units.IsLive(CamFollowUnit))
                CamFollowUnit = -1;
            // else HD 0x546490 / 0x5f4f60: centre on the unit (M2).
        }
        if (CamPitchMax == 0.0f) {
            CamTarget[1] = 0.0f;
        } else {
            float water = GetWaterHeight(CamTarget[0], CamTarget[2]);
            float ground = GetTerrainHeight(CamTarget[0], CamTarget[2]);
            CamTarget[1] = (water <= ground) ? GetTerrainHeight(CamTarget[0], CamTarget[2])
                                             : GetWaterHeight(CamTarget[0], CamTarget[2]);
            CamEye[0] = CamTarget[0];
            CamEye[1] = CamTarget[1];
            CamEye[2] = CamTarget[2];
            for (int i = 0; i < 4; ++i) {
                float d = CamDist;
                CamEye[0] -= (float)((double)(CamForward[0] * d) * 0.125);   // 0x7fe0b0
                CamEye[1] -= (float)((double)(CamForward[1] * d) * 0.125);
                CamEye[2] -= (float)((double)(CamForward[2] * d) * 0.125);
                float h = GetTerrainHeight(CamEye[0], CamEye[2]);
                if (CamTarget[1] < h)
                    CamTarget[1] = h;
            }
        }
        if (CamTime == 0.0) {
            CamSmoothTarget[0] = CamTarget[0];
            CamSmoothTarget[1] = CamTarget[1];
            CamSmoothTarget[2] = CamTarget[2];
            CamSmoothDist = CamDist;
        }
        double now = WorldTimerSeconds();
        double dt = now - CamTime;
        CamTime = now;
        if (now != 0.0) {
            double k = exp(dt * -10.0);                                 // 0x801b28
            CamSmoothTarget[0] = (float)((double)(CamSmoothTarget[0] - CamTarget[0]) * k + (double)CamTarget[0]);
            CamSmoothTarget[1] = (float)((double)(CamSmoothTarget[1] - CamTarget[1]) * k + (double)CamTarget[1]);
            CamSmoothTarget[2] = (float)((double)(CamSmoothTarget[2] - CamTarget[2]) * k + (double)CamTarget[2]);
            double k2 = exp(dt * -5.0);                                 // 0x801b20
            CamSmoothDist = (float)(k2 * (double)(CamSmoothDist - CamDist) + (double)CamDist);
        }
        float d = CamSmoothDist;
        CamEye[0] = CamSmoothTarget[0] - CamForward[0] * d;
        CamEye[1] = CamSmoothTarget[1] - CamForward[1] * d;
        CamEye[2] = CamSmoothTarget[2] - CamForward[2] * d;
        vp->SetCamera(CamEye[0], CamEye[1], CamEye[2], CamYaw, -CamPitch);   // vp +0x20
        // HD Concert +0x08(&eye, &forward, &up) (0x5de440): the 3D listener.
        // The SWINE concert slot takes the nine floats.
        if (Concert)
            Concert->SetListener(CamEye[0], CamEye[1], CamEye[2], CamForward[0], CamForward[1], CamForward[2],
                                 CamUp[0], CamUp[1], CamUp[2]);
        float focus = (CamPitchMax == 0.0f) ? GetTerrainHeight(CamTarget[0], CamTarget[2]) : CamTarget[1];
        g_Scene->SetFocusHeight(focus);                                 // scene +0x24
        if (CamProjectionDirty) {
            vp->SetProjection(1.0471976f, CamNear, CamFar);             // vp +0x28 (60 deg, 0x3f860a92)
            CamProjectionDirty = false;
            if (Logger.g)
                Logger.g->Log(0, "PZ3D world: camera eye %.3f %.3f %.3f yaw %.3f pitch %.3f fov 60 near %.2f far %.2f (target %.3f %.3f %.3f dist %.3f)",
                              CamEye[0], CamEye[1], CamEye[2], CamYaw, -CamPitch, CamNear, CamFar,
                              CamTarget[0], CamTarget[1], CamTarget[2], CamDist);
        }
        return;
    }
    // Modes 1 (free fall) and 2 (cinematic) are not used by the menu (M2).
    STUB_LOG("SWorld::ComputeCamera modes 1/2 (0x5ddc30)");
}

// ---------------------------------------------------------------------------
// Small helpers

void FreeSString(SString* s)
{
    if (s->buf) {
        delete[] s->buf;
        s->buf = nullptr;
    }
    s->size = 0;
}

void SetSString(SString* s, const char* text)
{
    *s = text;
}

// PANZERS 0x5625a0
void PathFilePart(const SString* path, SString* out)
{
    int last = -1;
    for (int i = 0; i < path->size - 1; ++i)
        if (path->buf[i] == '/' || path->buf[i] == '\\')
            last = i;
    FreeSString(out);
    if (last >= 0) {
        *out = path->buf + last + 1;
        return;
    }
    if (path->size == 0)
        return;
    *out = path->buf;
}

// PANZERS 0x5e4e00
void PathDirPart(const SString* path, SString* out)
{
    int last = -1;
    for (int i = 0; i < path->size - 1; ++i)
        if (path->buf[i] == '/' || path->buf[i] == '\\')
            last = i;
    FreeSString(out);
    if (last >= 0) {
        out->size = last;
        out->buf = new char[last + 1];
        memcpy(out->buf, path->buf, last);
        out->buf[last] = 0;
        return;
    }
    *out = ".";
}

} // namespace pz
