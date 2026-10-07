// src/3dengine/pz/weatherfx.cpp
// The weather effect types: rain (EffectType 2) and snowfall (3), HD
// 0x6ea820..0x6ec590, and the lens flares (EffectType 1, 0x6df7a0..0x6e08a0).
// OWNER: agent M6-WX.
//
// SWorld 0x6088f0 starts "Effects/Atmosphere/Rain.fx" / "Snowfall.fx" as
// persistent effects at (0, 0, 0) facing up and sets the weather's Rain /
// Snow value as the intensity (instance slot +0x14): the number of the 400
// particles of a tile that fall. Each effect keeps 4 x 4 tiles of an 8 x 8
// area; every visible terrain parcel near the camera draws the tile of
// (parcel x & 3, parcel z & 3) on top of the ground. The particle heights are
// above the terrain, not absolute.
//
// Render side only: CRT rand (0x78c846) as HD, and the pixie's rain / snow
// intensity fields (+0x58 / +0x5c) that the particle births read.

#include <math.h>
#include <d3d9.h>
#include <string.h>
#include <stdlib.h>
#include <string>
#include "effect.h"
#include "effectrender.h"
#include "pzpixie.h"
#include "pzterrain.h"
#include "pzgepard.h"
#include "iviewport.h"
#include "hdmath.h"
#include "logger.h"
#include "weathersound.h"
#if PZ_AUDIO_MILES
#include "milesconcert.h"
#endif

namespace pz {

namespace {

const int kTileParticles = 400;           // 0x1900 / 0x10, 0x44c0 / 0x2c
const float kTileSize = 8.0f;             // 0x884290 (double) and the parcel size
const float kRainDistance2 = 4800.0f;     // 0x8842a4
const float kSnowDistance2 = 6400.0f;     // 0x8843c4

// Scene frame seconds as HD: (float)(unsigned ms) * 0.001f, at most 0.125.
float WeatherFrameSeconds(SIScene* scene)
{
    float dt = (float)(double)(unsigned)SceneFrameMs(scene) * 0.001f;   // 0x87c78c
    if (dt > 0.125f)                                                    // 0x7f7f40
        dt = 0.125f;
    return dt;
}

// rand() * (1 / 32768) as a double (0x7f4540).
double Rand01()
{
    return (double)rand() * 3.0517578125e-05;
}

float Ground(float x, float z)   // STerrain 0x6f4c00
{
    return g_EffectGroundHeight ? g_EffectGroundHeight(x, z) : 0.0f;
}

// Shared prototype loader: 0x6eac50 and 0x6ebb20 differ in the FVF, the
// track name and the messages.
bool InitWeatherProto(SPropertyStruct* ts, const char* name, unsigned fvf, const char* track,
                      const char* cls, int* vb, float* duration, STrackFloat* numTrack,
                      float* vertSpeed, float* vertSpeedRnd, int* texture, int* variations,
                      float* invVariations, unsigned* color, float* alpha)
{
    if (*vb < 0) {
        *vb = EffectCreateDynamicVB(fvf);                                 // 0x6787a0
        if (*vb < 0)
            Logger.g->Panic("%s::Init(): Unable to create dynamic vertex-buffer.", cls);
    }
    SPropertyStruct* birth = ts->GetStruct(0, "Birth");
    *duration = birth->GetFloat(0, "Duration");
    birth->GetTrackFloat(track, numTrack);
    *vertSpeed = birth->GetFloat(2, "VertSpeed");
    *vertSpeedRnd = birth->GetFloat(3, "VertSpeed_Rnd");
    const char* tex = birth->GetString(4, "Texture");
    if (!tex || !*tex) {
        EffectSetLastError("%s::Init(): No texture specified for effect \"%s\".", cls, name);
        return true;
    }
    std::string path = std::string("effects/media/") + tex;
    *texture = PzGepard()->LoadTexture(path.c_str(), 1, true);           // Gepard +0x44 (name, 1, 1)
    if (*texture < 0) {
        EffectSetLastError("%s::Init(): Can't load texture \"effects/media/%s\" for effect \"%s\".", cls, tex,
                           name);
        return true;
    }
    *variations = birth->GetInt(5, "Variations");
    *invVariations = 1.0f / (float)*variations;
    *color = birth->GetColor(6, "Color");
    *alpha = birth->GetFloat(7, "Alpha");
    return false;
}

// The instance material (ctor: 0x687730, +0x1d4 = 1, 0x688d80(0, tex, 0),
// 0x688980(0, tex), 0x6887d0(2)).
void InitWeatherPass(SRenderPass& pass, int texture)
{
    pass.Init();
    pass.CullMode = 1;                      // D3DCULL_NONE
    pass.SetTexture(0, texture, false);
    pass.SetBlendMode(0, texture);
    pass.SetAlphaMode(2);
}

STerrain* WeatherTerrain(SIScene* scene)
{
    return static_cast<STerrain*>(SceneTerrain(scene));
}

} // namespace

// ===========================================================================
// 2: rain
// ===========================================================================

// PANZERS 0x6ea820
SPRain::SPRain()
    : Texture(-1), Variations(0), Color(0), VB(-1), Duration(0.0f), InvVariations(0.0f), Alpha(0.0f),
      VertSpeed(0.0f), VertSpeedRnd(0.0f)
{
}

// PANZERS 0x6eaa80
SPRain::~SPRain()
{
    // 0x67a130(VB): the recompile's dynamic VBs are CPU arrays.
    if (Texture >= 0)
        PzGepard()->ReleaseTexture(Texture);                             // Gepard +0x48
}

// PANZERS 0x6eac50
bool SPRain::Init(SPropertyStruct* ts, const char* name)
{
    return InitWeatherProto(ts, name, 0x102, "RainDropNum", "SPRain", &VB, &Duration, &NumTrack, &VertSpeed,
                            &VertSpeedRnd, &Texture, &Variations, &InvVariations, &Color, &Alpha);
}

// PANZERS 0x6eabd0
SEffect* SPRain::CreateInstance(SIScene* scene, float)
{
    return new SRain(scene, this);   // new 0x42c
}

// PANZERS 0x6ea860
// Every drop starts below the ground (y = -1 - 0..32), so nothing falls
// until the intensity lets drops respawn.
SRain::SRain(SIScene* scene, SPRain* proto)
    : SEffect(proto), Scene(scene), P(proto), Stopped(false), Cursor(0), UseTrack(true), Time(0.0f),
      Intensity(0.0f), Alive(0)
{
    InitWeatherPass(Pass, P->Texture);
    for (int t = 0; t < 16; ++t) {
        TileCount[t] = 0;
        TileFrame[t] = 0;
        Drops[t] = (float*)malloc(kTileParticles * 0x10);
        for (int k = 0; k < kTileParticles; ++k) {
            float* d = Drops[t] + k * 4;
            d[0] = (float)(Rand01() * 8.0);                              // 0x884290
            d[1] = -1.0f - (float)(Rand01() * 32.0);                     // 0x7f5a98, 0x884298
            d[2] = (float)(Rand01() * 8.0);
            float rnd = P->VertSpeedRnd;
            d[3] = P->VertSpeed + (float)(Rand01() * (double)rnd);
        }
    }
}

// PANZERS 0x6eab20
SRain::~SRain()
{
    for (int t = 0; t < 16; ++t)
        free(Drops[t]);
}

// PANZERS 0x6eb5b0
void SRain::Slot_14(int p)
{
    UseTrack = false;
    memcpy(&Intensity, &p, 4);   // SPixie +0x54 passes the float's bits
}

// PANZERS 0x6eaeb0
// Wind = VertSpeed * 0.3 * the effect direction (0 for the weather's
// upright effect). A drop under -1 respawns at a random spot of its tile,
// 32 units higher, while its index is below the intensity; a stopped rain
// ends when no drop is left.
bool SRain::Process()
{
    float dt = WeatherFrameSeconds(Scene);
    Time += dt;
    if (dt > 0.0f) {                                                     // 0x7f1038
        Alive = 0;
        if (UseTrack)
            Intensity = P->NumTrack.Evaluate(Time / P->Duration, &Cursor);   // 0x6abda0
        if (SPixie* px = SPixie::Instance())
            px->RainIntensity = Intensity;                               // pixie +0x58
        for (int t = 0; t < 16; ++t) {
            if (TileFrame[t] == LastFrame)
                continue;
            TileFrame[t] = LastFrame;
            TileCount[t] = 0;
            for (int k = 0; k < kTileParticles; ++k) {
                float* d = Drops[t] + k * 4;
                d[0] = P->VertSpeed * 0.3f * dt * Dir[0] + d[0];         // 0x7f83d8
                d[2] = P->VertSpeed * 0.3f * dt * Dir[2] + d[2];
                d[1] = d[1] - d[3] * dt;
                if (d[1] < -1.0f) {
                    if (!Stopped && (float)k < Intensity) {
                        ++Alive;
                        ++TileCount[t];
                        d[0] = (float)(Rand01() * 8.0);
                        d[2] = (float)(Rand01() * 8.0);
                        while (d[1] < -1.0f)
                            d[1] += 32.0f;                               // 0x8842a0
                    }
                } else {
                    ++Alive;
                    ++TileCount[t];
                }
            }
        }
        if (Stopped && Alive == 0)
            return false;
    }
    return true;
}

// PANZERS 0x6eb170
// One textured quad per drop (two triangles, FVF 0x102): 2 units tall from
// its height above the ground, slanted by the wind (0.3 * direction) and
// 0.04 wide across the camera's yaw.
void SRain::Render(SIViewport* vp)
{
    float cx, cy, cz, yaw, pitch;
    vp->GetCamera(&cx, &cy, &cz, &yaw, &pitch);                          // vp +0x24
    double a = (double)(-yaw);
    float sideX = (float)(HdCos(a) * 0.019999999552965164);              // 0x78d480, 0x884288
    float sideZ = (float)(HdSin(a) * 0.019999999552965164);              // 0x78d640
    float windX = Dir[0] * 0.3f;
    float windZ = Dir[2] * 0.3f;
    Pass.Lighting = false;
    Pass.Apply();                                                        // 0x687a50
    STerrain* ter = WeatherTerrain(Scene);
    if (!ter)
        return;   // recompile: a scene without terrain (fxview); HD assumes one
    for (int i = 0; i < ter->ParcelsX; ++i) {
        float px = (float)(i * 8);
        for (int j = 0; j < ter->ParcelsZ; ++j) {
            float pz = (float)(j * 8);
            if (!ter->Parcels[ter->ParcelsX * j + i].Visible)
                continue;
            float dx = cx - (px + 4.0f), dz = cz - (pz + 4.0f);         // 0x7f4588
            if (!(dz * dz + dx * dx < kRainDistance2))
                continue;
            int tile = (j & 3) + (i & 3) * 4;
            int n = TileCount[tile];
            if (n <= 0)
                continue;
            float* v = EffectLockDynamicVB(P->VB, n * 6);               // 0x67f150
            int quads = 0;
            for (int k = 0; k < kTileParticles && quads < n; ++k) {
                const float* d = Drops[tile] + k * 4;
                if (!(-1.0f <= d[1]))
                    continue;
                float x = d[0] + px;
                float z = d[2] + pz;
                float h = Ground(x, z) + d[1];
                float top = h + 1.0f, bot = h - 1.0f;
                float tx = x - windX, tz = z - windZ;                    // top end
                float bx = x + windX, bz = z + windZ;                    // bottom end
                float q[30] = {
                    tx - sideX, top, tz - sideZ, 0.0f, 0.0f,
                    bx - sideX, bot, bz - sideZ, 0.0f, 1.0f,
                    bx + sideX, bot, bz + sideZ, 1.0f, 1.0f,
                    tx - sideX, top, tz - sideZ, 0.0f, 0.0f,
                    bx + sideX, bot, bz + sideZ, 1.0f, 1.0f,
                    tx + sideX, top, tz + sideZ, 1.0f, 0.0f,
                };
                memcpy(v, q, sizeof(q));
                v += 30;
                ++quads;
            }
            EffectUnlockDynamicVB();                                     // 0x681440
            EffectDrawDynamicVB(4, quads * 2);                           // 0x67a490(D3DPT_TRIANGLELIST, n * 2)
            EffectAdvanceDynamicVB(n * 6);                               // 0x677fc0
        }
    }
}

// ===========================================================================
// 3: snowfall
// ===========================================================================

// PANZERS 0x6eb5e0
SPSnowfall::SPSnowfall()
    : Texture(-1), Variations(0), Color(0), VB(-1), Duration(0.0f), InvVariations(0.0f), Alpha(0.0f),
      VertSpeed(0.0f), VertSpeedRnd(0.0f)
{
}

// PANZERS 0x6eb940
SPSnowfall::~SPSnowfall()
{
    if (Texture >= 0)
        PzGepard()->ReleaseTexture(Texture);
}

// PANZERS 0x6ebb20
bool SPSnowfall::Init(SPropertyStruct* ts, const char* name)
{
    return InitWeatherProto(ts, name, 0x1c4, "SnowFlakeNum", "SPSnowfall", &VB, &Duration, &NumTrack,
                            &VertSpeed, &VertSpeedRnd, &Texture, &Variations, &InvVariations, &Color, &Alpha);
}

// PANZERS 0x6ebaa0
SEffect* SPSnowfall::CreateInstance(SIScene* scene, float)
{
    return new SSnowfall(scene, this);   // new 0x7ac
}

// PANZERS 0x6eb620
// HD leaves the track cursor (+0x490) and the time (+0x498) as allocated;
// the recompile starts both at 0.
SSnowfall::SSnowfall(SIScene* scene, SPSnowfall* proto)
    : SEffect(proto), Scene(scene), P(proto), Stopped(false), Cursor(0), UseTrack(true), Time(0.0f),
      Intensity(0.0f), Alive(0)
{
    InitWeatherPass(Pass, P->Texture);
    for (int t = 0; t < 16; ++t) {
        TileFrame[t] = 0;
        TileCount[t] = 0;
        Flakes[t] = (SSnowFlake*)malloc(kTileParticles * sizeof(SSnowFlake));
        for (int k = 0; k < kTileParticles; ++k) {
            SSnowFlake& f = Flakes[t][k];
            f.Pos[0] = (float)(Rand01() * 8.0);
            f.Pos[1] = -1.0f - (float)(Rand01() * 32.0);
            f.Pos[2] = (float)(Rand01() * 8.0);
            float rnd = P->VertSpeedRnd;
            f.Fall = P->VertSpeed + (float)(Rand01() * (double)rnd);
            f.Variation = (rand() << 2) >> 15;
            f.Swing[0] = (float)(Rand01() * 0.20000000298023224) + 0.2f;   // 0x87c7b8, 0x7f1b4c
            f.Swing[1] = (float)(Rand01() * 0.20000000298023224) + 0.2f;
            f.Phase[0] = 1.0f;
            f.Phase[1] = 1.0f;
            f.PhaseSpeed[0] = (float)(Rand01() * 2.0 + 2.0);             // 0x7ea768, 0x7f4558
            f.PhaseSpeed[1] = (float)(Rand01() * 2.0 + 2.0);
        }
    }
}

// PANZERS 0x6eb9e0
SSnowfall::~SSnowfall()
{
    for (int t = 0; t < 16; ++t)
        free(Flakes[t]);
}

// PANZERS 0x6ec570
void SSnowfall::Slot_14(int p)
{
    UseTrack = false;
    memcpy(&Intensity, &p, 4);
}

// PANZERS 0x6ebd90
// As the rain, with wind VertSpeed * 1.3 * direction, a sway phase per
// flake, and the respawn under 0 (a flake respawned into -1..0 respawns
// again on the next frame).
bool SSnowfall::Process()
{
    float dt = WeatherFrameSeconds(Scene);
    Time += dt;
    if (0.0f < dt) {
        Alive = 0;
        if (UseTrack)
            Intensity = P->NumTrack.Evaluate(Time / P->Duration, &Cursor);
        if (SPixie* px = SPixie::Instance())
            px->_5c = Intensity;                                         // pixie +0x5c
        for (int t = 0; t < 16; ++t) {
            if (TileFrame[t] == LastFrame)
                continue;
            TileFrame[t] = LastFrame;
            TileCount[t] = 0;
            for (int k = 0; k < kTileParticles; ++k) {
                SSnowFlake& f = Flakes[t][k];
                f.Pos[0] = P->VertSpeed * 1.3f * dt * Dir[0] + f.Pos[0];   // 0x87c7a0
                f.Pos[2] = P->VertSpeed * 1.3f * dt * Dir[2] + f.Pos[2];
                f.Pos[1] = f.Pos[1] - f.Fall * dt;
                f.Phase[0] = f.PhaseSpeed[0] * dt + f.Phase[0];
                f.Phase[1] = f.PhaseSpeed[1] * dt + f.Phase[1];
                if (f.Pos[1] < 0.0f) {
                    if (!Stopped && (float)k < Intensity) {
                        ++Alive;
                        ++TileCount[t];
                        f.Pos[0] = (float)(Rand01() * 8.0);
                        f.Pos[2] = (float)(Rand01() * 8.0);
                        while (f.Pos[1] < -1.0f)
                            f.Pos[1] += 32.0f;
                    }
                } else {
                    ++Alive;
                    ++TileCount[t];
                }
            }
        }
        if (Stopped && Alive == 0)
            return false;
    }
    return true;
}

// PANZERS 0x6ec0c0
// Screen-space sprites (FVF 0x1c4): every flake of the tile (HD draws the
// ones under the ground too; the depth test hides them) projected with size
// 0.07 (vp +0x3c), a quad of the projected size, white, the fog in the
// specular alpha, the texture column of its variation (1 / Variations wide).
void SSnowfall::Render(SIViewport* vp)
{
    float cx, cy, cz, yaw, pitch;
    vp->GetCamera(&cx, &cy, &cz, &yaw, &pitch);                          // vp +0x24
    Pass.Lighting = false;
    Pass.Apply();                                                        // 0x687a50
    STerrain* ter = WeatherTerrain(Scene);
    if (!ter)
        return;
    for (int i = 0; i < ter->ParcelsX; ++i) {
        float px = (float)(i * 8);
        for (int j = 0; j < ter->ParcelsZ; ++j) {
            float pz = (float)(j * 8);
            if (!ter->Parcels[ter->ParcelsX * j + i].Visible)
                continue;
            float dx = cx - (px + 4.0f), dz = cz - (pz + 4.0f);
            if (!(dz * dz + dx * dx < kSnowDistance2))
                continue;
            int tile = (i & 3) * 4 + (j & 3);                            // HD (i & 3) * 0x20 + (j & 3)
            float* v = EffectLockDynamicVB(P->VB, kTileParticles * 6);   // 0x67f150(VB, 0x960)
            int nv = 0;
            for (int k = 0; k < kTileParticles; ++k) {
                const SSnowFlake& f = Flakes[tile][k];
                float x = px + f.Pos[0];
                float z = f.Pos[2] + pz;
                float h = Ground(x, z) + f.Pos[1];
                float pos[3];
                pos[0] = (float)(HdSin((double)f.Phase[0]) * (double)f.Swing[0] + (double)x);
                pos[1] = h;
                pos[2] = (float)(HdSin((double)f.Phase[1]) * (double)f.Swing[1] + (double)z);
                float sx, sy, ss, sz;
                int fog;
                vp->ProjectToScreen(pos, 0.07f, &sx, &sy, &ss, &sz, &fog);  // vp +0x3c (0x3d8f5c29)
                if (!(0.0f < ss))
                    continue;
                float u0 = (float)f.Variation * P->InvVariations;
                float u1 = (float)(f.Variation + 1) * P->InvVariations;
                float l = sx - ss, r = sx + ss, t = sy - ss, b = sy + ss;
                const float corner[6][4] = {
                    { l, t, u0, 0.0f }, { l, b, u0, 1.0f }, { r, b, u1, 1.0f },
                    { l, t, u0, 0.0f }, { r, b, u1, 1.0f }, { r, t, u1, 0.0f },
                };
                for (int c = 0; c < 6; ++c) {
                    v[0] = corner[c][0];
                    v[1] = corner[c][1];
                    v[2] = sz;
                    v[3] = 1.0f;
                    unsigned white = 0xffffffffu;
                    memcpy(&v[4], &white, 4);
                    memcpy(&v[5], &fog, 4);
                    v[6] = corner[c][2];
                    v[7] = corner[c][3];
                    v += 8;
                }
                nv += 6;
            }
            EffectUnlockDynamicVB();                                     // 0x681440
            EffectDrawDynamicVB(4, nv / 3);                              // 0x67a490(D3DPT_TRIANGLELIST, n / 3)
            EffectAdvanceDynamicVB(nv);                                  // 0x677fc0
        }
    }
}

// ===========================================================================
// 1: flare
// ===========================================================================

// PANZERS 0x6df880
SPFlare::SPFlare()
    : Texture(-1), Variations(0), Color(0), InvVariations(0.0f), Alpha(0.0f), AlphaExp(0.0f), Scale(0.0f),
      ScaleExp(0.0f), Directional(false)
{
}

// PANZERS 0x6dfa00
SPFlare::~SPFlare()
{
    if (Texture >= 0)
        PzGepard()->ReleaseTexture(Texture);                             // Gepard +0x48
}

// PANZERS 0x6dfc60
bool SPFlare::Init(SPropertyStruct* ts, const char* name)
{
    SPropertyStruct* draw = ts->GetStruct(0, "Draw");
    const char* tex = draw->GetString(0, "Texture");
    if (!tex || !*tex) {
        EffectSetLastError("SPFlare::Init(): No texture specified for effect \"%s\".", name);
        return true;
    }
    std::string path = std::string("effects/media/") + tex;
    Texture = PzGepard()->LoadTexture(path.c_str(), 1, true);           // Gepard +0x44 (name, 1, 1)
    if (Texture < 0) {
        EffectSetLastError("SPFlare::Init(): Can't load texture \"effects/media/%s\" for effect \"%s\".", tex, name);
        return true;
    }
    Variations = draw->GetInt(1, "Variations");
    InvVariations = 1.0f / (float)Variations;
    Color = draw->GetColor(2, "Color");
    Alpha = draw->GetFloat(3, "Alpha");
    AlphaExp = draw->GetFloat(4, "Alpha_Exp");
    Scale = draw->GetFloat(5, "Scale");
    ScaleExp = draw->GetFloat(6, "Scale_Exp");
    Directional = draw->GetBool(7, "Directional");
    SPixie* px = SPixie::Instance();
    if (!px || px->FlareVB == 0)
        Logger.g->Panic("SPFlare::Init(): Pixie->FlareDynamicVB is NULL.");
    return false;
}

// PANZERS 0x6dfb30
// The instance also goes into the pixie flare heap (0x6dfa90 Add).
SEffect* SPFlare::CreateInstance(SIScene*, float)
{
    SPixie* px = SPixie::Instance();
    int i = px->Flares.Add();
    SFlare* f = new SFlare(this, i);                                     // new 0x36c
    px->Flares[i] = f;
    return f;
}

// PANZERS 0x6df7a0
// Additive, clamped texture, a random column of the texture strip, and an
// occlusion query (device +0x1d8 CreateQuery(D3DQUERYTYPE_OCCLUSION)).
SFlare::SFlare(SPFlare* proto, int flareIndex)
    : SEffect(proto), FlareIndex(flareIndex), Variation(0), P(proto), Pending(false), ExpectedPixels(0),
      QueryState(0), Visibility(1.0f), Query(nullptr)
{
    Pass.Init();                                                         // 0x687730
    Pass.CullMode = 1;
    Pass.SetTexture(0, P->Texture, false);                               // 0x688d80(0, tex, 0)
    Pass.SetBlendMode(1, -1);                                            // 0x688980(1, -1)
    Variation = (rand() * (int)(unsigned short)P->Variations) >> 15;
    CreateQuery();
}

// PANZERS 0x6df8b0
SFlare::~SFlare()
{
    ReleaseQuery();
    if (SPixie* px = SPixie::Instance())
        px->Flares.Remove(FlareIndex);
}

// PANZERS 0x6dfea0
bool SFlare::Process()
{
    Pending = Active;
    return true;
}

// PANZERS 0x6e08a0
// HD deletes the flare here (the set's slot goes with it, ~SEffect).
void SFlare::Stop()
{
    delete this;
}

// PANZERS 0x6dfc10
void SFlare::CreateQuery()
{
    IDirect3DDevice9* dev = HD().Device;
    IDirect3DQuery9* q = nullptr;
    if (!dev || FAILED(dev->CreateQuery(D3DQUERYTYPE_OCCLUSION, &q)))
        q = nullptr;
    Query = q;
}

// PANZERS 0x6dfc40
void SFlare::ReleaseQuery()
{
    if (Query)
        static_cast<IDirect3DQuery9*>(Query)->Release();
    Query = nullptr;   // recompile: HD keeps the pointer
}

// PANZERS 0x6e00c0
float* SFlare::BeginBatch(SIViewport*, int)
{
    Pass.Lighting = false;
    Pass.Apply();                                                        // 0x687a50
    SPixie* px = SPixie::Instance();
    px->_54 = 0;                                                         // vertices in the batch
    return EffectLockDynamicVB(px->FlareVB, 0x600);                      // 0x67f150
}

// PANZERS 0x6e0100
void SFlare::EndBatch(SIViewport*, int)
{
    SPixie* px = SPixie::Instance();
    EffectUnlockDynamicVB();                                             // 0x681440
    EffectDrawDynamicVB(4, px->_54 / 3);                                 // 0x67a490(D3DPT_TRIANGLELIST)
    EffectAdvanceDynamicVB(px->_54);                                     // 0x677fc0
}

// PANZERS 0x6e0150
// The glow faces the camera; a directional flare is weighted by the cosine
// between its direction and the way to the camera (behind it: not drawn).
// t = visibility * that weight; alpha = t^Alpha_Exp * Alpha, screen size
// = t^Scale_Exp * Scale projected (vp +0x3c); colour = Color * alpha (no
// alpha channel: additive). The query result of the last frame (pixels / 45)
// is the visibility.
void SFlare::Draw(SIViewport* vp, const float* camera, float* vb)
{
    if (!Pending)
        return;
    float dx = camera[0] - Pos[0];
    float dy = camera[1] - Pos[1];
    float dz = camera[2] - Pos[2];
    double inv = 1.0 / sqrt((double)(dy * dy + dx * dx + dz * dz));    // 0x78d090, 0x7eed98
    float nx = (float)((double)dx * inv);
    float ny = (float)((double)dy * inv);
    float nz = (float)((double)dz * inv);
    float dot = 1.0f;
    if (P->Directional)
        dot = Dir[1] * ny + Dir[0] * nx + Dir[2] * nz;
    if (QueryState == 1) {
        DWORD pixels = 0;
        HRESULT hr = static_cast<IDirect3DQuery9*>(Query)->GetData(&pixels, 4, D3DGETDATA_FLUSH);
        if (hr != S_FALSE)
            QueryState = 0;
        if (QueryState == 0) {
            Visibility = (float)(double)(unsigned)pixels / 45.0f;        // 0x883df8
            if (Visibility > 1.0f)
                Visibility = 1.0f;
        }
    }
    if (!(dot >= 0.0f))
        return;
    if (QueryState == 0)
        QueryState = 2;
    float l = (float)log((double)(Visibility * dot));                    // 0x7a2970
    float alpha = (float)exp((double)(P->AlphaExp * l));                 // 0x794960
    float size = (float)exp((double)(P->ScaleExp * l)) * P->Scale;
    float sx, sy, ss, sz;
    int fog;
    vp->ProjectToScreen(Pos, size, &sx, &sy, &ss, &sz, &fog);            // vp +0x3c
    if (!(ss > 0.0f)) {
        ExpectedPixels = 1;
        return;
    }
    int w = (int)(ss * 2.0f);                                            // _ftol2 0x766810
    ExpectedPixels = w * w;
    float a = P->Alpha * alpha;
    float u0 = (float)Variation * P->InvVariations;
    float u1 = (float)(Variation + 1) * P->InvVariations;
    unsigned c = P->Color;
    int r = (int)((float)((c >> 16) & 0xff) * a);                        // fistp, control word 0xc7f (chop)
    int g = (int)((float)((c >> 8) & 0xff) * a);
    int b = (int)((float)(c & 0xff) * a);
    unsigned color = (unsigned)((r * 0x100 + g) * 0x100 + b);
    SPixie* px = SPixie::Instance();
    if (px->_54 + 6 > 0x600)
        return;   // recompile: HD writes past the 0x600 locked vertices
    float l0 = sx - ss, r0 = sx + ss, t0 = sy - ss, b0 = sy + ss;
    const float corner[6][4] = {
        { l0, t0, u0, 0.0f }, { l0, b0, u0, 1.0f }, { r0, b0, u1, 1.0f },
        { l0, t0, u0, 0.0f }, { r0, b0, u1, 1.0f }, { r0, t0, u1, 0.0f },
    };
    for (int k = 0; k < 6; ++k) {
        float* v = vb + px->_54 * 8;
        v[0] = corner[k][0];
        v[1] = corner[k][1];
        v[2] = 0.0f;
        v[3] = 1.0f;
        memcpy(&v[4], &color, 4);
        memcpy(&v[5], &fog, 4);
        v[6] = corner[k][2];
        v[7] = corner[k][3];
        ++px->_54;
    }
}

// PANZERS 0x6dfeb0
// After the effects: a 10 x 10 pixel quad just in front of the projected
// flare (depth - 0.0001, colour writes off) inside the occlusion query.
// HD fills only three of the four strip vertices of its static buffer
// (0x93da78); the fourth stays zero, as here.
void SFlare::IssueOcclusionQuery(SIViewport* vp)
{
    if (!Query)
        return;
    IDirect3DDevice9* dev = HD().Device;
    if (!dev)
        return;
    static float s_Quad[4][8];                                           // 0x93da78 (FVF 0x1c4)
    dev->SetFVF(0x1c4);                                                  // device +0x164
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0);                      // device +0xe4(0xa8, 0)
    if (QueryState == 2) {
        float sx, sy, ss, sz;
        int fog;
        vp->ProjectToScreen(Pos, P->Scale, &sx, &sy, &ss, &sz, &fog);
        const unsigned white = 0xffffffffu;
        float* v = s_Quad[0];
        v[0] = sx - 5.0f; v[1] = sy - 5.0f; v[2] = sz - 0.0001f; v[3] = 1.0f;   // 0x7f5a6c, 0x7f5e1c
        memcpy(&v[4], &white, 4); v[5] = 0.0f; v[6] = 0.0f; v[7] = 0.0f;
        v = s_Quad[1];
        v[0] = sx - 5.0f; v[1] = sy + 5.0f; v[2] = s_Quad[0][2]; v[3] = 1.0f;
        memcpy(&v[4], &white, 4); v[5] = 0.0f; v[6] = 0.0f; v[7] = 1.0f;
        v = s_Quad[2];
        v[0] = sx + 5.0f; v[1] = s_Quad[1][1]; v[2] = s_Quad[0][2]; v[3] = 1.0f;
        memcpy(&v[4], &white, 4); v[5] = 0.0f; v[6] = 1.0f; v[7] = 1.0f;
        IDirect3DQuery9* q = static_cast<IDirect3DQuery9*>(Query);
        q->Issue(D3DISSUE_BEGIN);
        dev->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, s_Quad, 0x20);      // device +0x14c
        q->Issue(D3DISSUE_END);
        QueryState = 1;
    }
    dev->SetRenderState(D3DRS_COLORWRITEENABLE, 0xf);
}

// ===========================================================================
// The world's rain sound (weathersound.h)
// ===========================================================================

int WeatherPrecacheSound(const char* name)
{
#if PZ_AUDIO_MILES
    if (g_MilesConcert)
        return g_MilesConcert->PrecacheSound(name, false);              // concert +0x24
#endif
    (void)name;
    return -1;
}

int WeatherCreateLoopSound(int cache)
{
#if PZ_AUDIO_MILES
    if (g_MilesConcert && cache >= 0)
        return g_MilesConcert->CreateSoundByIdEx(cache, true, 0.0f, 0.0f, true);   // concert +0x30
#endif
    (void)cache;
    return -1;
}

void WeatherSetSoundVolume(int sound, float volume)
{
#if PZ_AUDIO_MILES
    if (g_MilesConcert && sound >= 0)
        g_MilesConcert->SetSoundVolume(sound, volume, false);           // concert +0x48
#endif
    (void)sound;
    (void)volume;
}

void WeatherRemoveSound(int sound)
{
#if PZ_AUDIO_MILES
    if (g_MilesConcert && sound >= 0)
        g_MilesConcert->RemoveSound(sound);                             // concert +0x3c
#endif
    (void)sound;
}

} // namespace pz
