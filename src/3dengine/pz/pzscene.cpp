// src/3dengine/pz/pzscene.cpp
// pz::SScene: HD SScene (ctor 0x69faf0). OWNER: agent A.
// Stubs log their first call (STUB_LOG) and every call when -menu3d tracing
// is on (PZ_TRACE). Lifted bodies are marked "// PANZERS 0xADDR".

#include <d3d9.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "pzscene.h"
#include "pzmodel.h"
#include "pzviewport.h"
#include "pzgepard.h"
#include "pzterrain.h"
#include "parcel.h"
#include "pzpixie.h"
#include "mesh.h"
#include "shadowmath.h"
#include <vector>
#include "timer.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

static void Identity44(float* m)
{
    memset(m, 0, 64);
    m[0] = m[5] = m[10] = m[15] = 1.0f;   // 0x67cbc0
}

// The live scenes (HD: SGepard scene heap +0x4e4, filled by CreateScene).
static std::vector<SScene*> s_Scenes;

void ReleaseAllShadowBuffers()
{
    for (SScene* s : s_Scenes)
        s->ReleaseShadowBuffer();   // 0x6a2670
}

// PANZERS 0x69faf0
SScene::SScene(int param)
{
    PZ_TRACE("SScene::SScene (0x69faf0)");
    memset(static_cast<void*>(&RefCount), 0, sizeof(SScene) - offsetof(SScene, RefCount));
    RefCount = 1;
    Identity44(ShadowProjection);
    Identity44(ShadowMatrix);
    FrameCount = 1;
    Interpolation = 0.0;
    Lights.FreeHead = -1;
    Models.FreeHead = -1;
    FreeModels.FreeHead = -1;
    Rivers.FreeHead = -1;
    Param = param;
    Device = HD().Device;
    TimeMs = 0;
    LastTimeMs = 0;
    Seconds = 0.0f;
    // The sun: directional, grey 0.64, from (1, -1, 1)/sqrt(3).
    float n = (float)(1.0 / sqrt(3.0));
    SunLight = CreateDirectionalLight(0.64f, 0.64f, 0.64f, 1.0f, n, (float)(n * -1.0), n);
    SunAzimuth = -1000.0f;
    SunElevation = -1000.0f;
    SunMatrix[0] = SunMatrix[4] = SunMatrix[8] = 1.0f;
    RiverTexture = -1;
    ShadowViewport = -1;
    ShadowTexture = -1;
    FocusHeight = 0.0f;
    s_Scenes.push_back(this);
    Logger.g->Log(0, "Scene created");
}

// PANZERS 0x6a0440
SScene::~SScene()
{
    PZ_TRACE("SScene::~SScene (0x6a0440)");
    for (int i = Models.Next(-1); i >= 0; i = Models.Next(i)) {
        SModel* m = Models[i];
        m->Scene = nullptr;
        m->Release();
    }
    for (int i = FreeModels.Next(-1); i >= 0; i = FreeModels.Next(i)) {
        SModel* m = FreeModels[i];
        m->Scene = nullptr;
        m->Release();
    }
    // HD 0x6a0527: the effect manager (0x92f104) drops every effect that
    // still plays in this scene (pixie +0x44).
    if (SIPixie* pixie = GepardPixie())
        pixie->DestroySceneEffects(this);
    DestroyTerrain();
    Models.Free();
    FreeModels.Free();
    Lights.Free();
    Rivers.Free();
    ReleaseShadowBuffer();
    for (size_t i = 0; i < s_Scenes.size(); ++i)
        if (s_Scenes[i] == this) {
            s_Scenes.erase(s_Scenes.begin() + i);
            break;
        }
    free(CellLinks);
    free(Deferred);
    delete[] AtmosphereName;
    AtmosphereName = nullptr;
    Logger.g->Log(0, "Scene destroyed");
}

// PANZERS 0x6a1f70
void SScene::AddRef()
{
    PZ_TRACE("SScene::AddRef (0x6a1f70)");
    ++RefCount;
}

// PANZERS 0x6ac7f0
void SScene::Release()
{
    PZ_TRACE("SScene::Release (0x6ac7f0)");
    if (--RefCount == 0)
        delete this;   // ~SScene 0x6a0440 + operator delete(0x2b0)
}

// PANZERS 0x6baa80
void SScene::SetAtmosphere(const char* name)
{
    PZ_TRACE("SScene::SetAtmosphere (0x6baa80)");
    delete[] AtmosphereName;
    AtmosphereName = nullptr;
    AtmosphereLen = name ? (int)strlen(name) : 0;
    if (AtmosphereLen) {
        AtmosphereName = new char[AtmosphereLen + 1];
        memcpy(AtmosphereName, name, AtmosphereLen + 1);
    }
}

// PANZERS 0x6a1f80
void SScene::AdvanceTime(int milliseconds)
{
    PZ_TRACE("SScene::AdvanceTime (0x6a1f80)");
    TimeMs += milliseconds;
}

// PANZERS 0x6bab40
void SScene::SetInterpolation(double t)
{
    PZ_TRACE("SScene::SetInterpolation (0x6bab40)");
    Interpolation = t;
}

// PANZERS 0x6baed0
void SScene::SetFocusHeight(float height)
{
    PZ_TRACE("SScene::SetFocusHeight (0x6baed0)");
    FocusHeight = height;
}

// ---------------------------------------------------------------------------
// lights
// ---------------------------------------------------------------------------

// PANZERS 0x6a76f0
int SScene::CreateDirectionalLight(float r, float g, float b, float a, float dx, float dy, float dz)
{
    PZ_TRACE("SScene::CreateDirectionalLight (0x6a76f0)");
    int i = Lights.Add();   // 0x6a10c0
    SSceneLight& l = Lights[i];
    l.Type = 3;
    l.Color[0] = r; l.Color[1] = g; l.Color[2] = b; l.Color[3] = a;
    l.Dir[0] = dx; l.Dir[1] = dy; l.Dir[2] = dz;
    return i;
}

int SScene::CreateDirectionalLight(const float* rgba, const float* dir)
{
    return CreateDirectionalLight(rgba[0], rgba[1], rgba[2], 1.0f, dir[0], dir[1], dir[2]);
}

// PANZERS 0x6a8ee0
int SScene::CreatePointLight(float r, float g, float b, float a, float x, float y, float z, float range, float atten2)
{
    PZ_TRACE("SScene::CreatePointLight (0x6a8ee0)");
    int i = Lights.Add();
    SSceneLight& l = Lights[i];
    l.Type = 1;
    l.Color[0] = r; l.Color[1] = g; l.Color[2] = b; l.Color[3] = a;
    l.Pos[0] = x; l.Pos[1] = y; l.Pos[2] = z;
    l.Range = range;
    l.Atten[0] = 0.0f;
    l.Atten[1] = 0.0f;
    l.Atten[2] = atten2;
    return i;
}

// PANZERS 0x6baca0
void SScene::SetLightPosition(int light, float x, float y, float z)
{
    PZ_TRACE("SScene::SetLightPosition (0x6baca0)");
    if (!Lights.Valid(light) || (Lights[light].Type != 1 && Lights[light].Type != 2)) {
        Logger.g->Log(0, "SScene::SetLightPosition: Invalid light idx (%d)", light);
        return;
    }
    Lights[light].Pos[0] = x;
    Lights[light].Pos[1] = y;
    Lights[light].Pos[2] = z;
}

// PANZERS 0x6babe0
void SScene::SetLightDirection(int light, const float* dir)
{
    if (!Lights.Valid(light) || (Lights[light].Type != 3 && Lights[light].Type != 2)) {
        Logger.g->Log(0, "SScene::SetLightDirection: Invalid light idx (%d)", light);
        return;
    }
    memcpy(Lights[light].Dir, dir, 12);
}

// PANZERS 0x6bab60
void SScene::SetLightColor(int light, const float* rgba)
{
    if (!Lights.Valid(light)) {
        Logger.g->Log(0, "SScene::SetLightColor: Invalid light idx (%d)", light);
        return;
    }
    memcpy(Lights[light].Color, rgba, 12);
    Lights[light].Color[3] = 1.0f;
}

// PANZERS 0x6aa890
void SScene::DestroyLight(int light)
{
    PZ_TRACE("SScene::DestroyLight (0x6aa890)");
    if (!Lights.Valid(light)) {
        Logger.g->Log(0, "SScene::DestroyLight: Invalid light idx (%d)", light);
        return;
    }
    Lights.Remove(light);   // 0x6ac8d0
}

static void UpdateLightColor(SScene* s)   // tail of 0x6ba970 / 0x6baef0
{
    for (int i = 0; i < 4; ++i) {
        s->LightColor[i] = s->SunColor[i] + s->Ambient[i];
        if (1.0f < s->LightColor[i])
            s->LightColor[i] = 1.0f;
    }
}

// PANZERS 0x6ba970
// With option 0x10 (MODULATE2X materials) the colours are halved (0x87c820).
void SScene::SetAmbientLight(const float* rgba)
{
    PZ_TRACE("SScene::SetAmbientLight (0x6ba970)");
    bool half = GepardOption(0x10) != 0;
    for (int i = 0; i < 4; ++i)
        Ambient[i] = half ? rgba[i] * 0.5f : rgba[i];
    UpdateLightColor(this);
    if (g_Menu3D.Trace)
        Logger.g->Log(0, "  SetAmbientLight %.3f %.3f %.3f %.3f", rgba[0], rgba[1], rgba[2], rgba[3]);
}

// PANZERS 0x6abc60
const float* SScene::GetAmbientLight()
{
    return Ambient;
}

// PANZERS 0x6baef0
// Sun colour and angles: direction (cos(e) sin(a), sin(e), cos(a) cos(e));
// also the sun's 3x4 view basis at +0x12c for the shadow buffer.
void SScene::SetSunLight(const float* color, float azimuth, float elevation)
{
    PZ_TRACE("SScene::SetSunLight (0x6baef0)");
    SunAzimuth = azimuth;
    SunElevation = elevation;
    bool half = GepardOption(0x10) != 0;
    for (int i = 0; i < 4; ++i)
        SunColor[i] = half ? color[i] * 0.5f : color[i];
    if (!Lights.Valid(SunLight)) {
        Logger.g->Log(0, "SHeap<%s>::operator[]: invalid index (%d)", "SScene::SLight", SunLight);
        return;
    }
    memcpy(Lights[SunLight].Color, SunColor, 16);
    double sa = sin((double)azimuth);
    double cb = cos((double)elevation);
    SunDir[0] = (float)(cb * sa);
    double sb = sin((double)elevation);
    SunDir[1] = (float)sb;
    double ca = cos((double)azimuth);
    SunDir[2] = (float)(ca * cb);
    // +0x12c..+0x158 as HD writes them (sun view basis for the shadow buffer).
    memset(SunMatrix, 0, sizeof(SunMatrix));
    SunMatrix[0] = (float)ca;
    SunMatrix[1] = (float)sa;
    SunMatrix[2] = (float)(cb * sa);
    SunMatrix[3] = 0.0f;
    SunMatrix[4] = (float)-(cb / sb);
    SunMatrix[5] = (float)sb;
    SunMatrix[6] = (float)-sa;
    SunMatrix[7] = (float)ca;
    SunMatrix[8] = (float)(ca * cb);
    memcpy(Lights[SunLight].Dir, SunDir, 12);
    UpdateLightColor(this);
    if (g_Menu3D.Trace)
        Logger.g->Log(0, "  SetSunLight %.3f %.3f %.3f az %.3f el %.3f dir %.3f %.3f %.3f", color[0], color[1], color[2], azimuth, elevation, SunDir[0], SunDir[1], SunDir[2]);
}

// PANZERS 0x6ac1e0
void SScene::GetSunLight(float* color, float* p2, float* p3)
{
    memcpy(color, SunColor, 16);
    *p2 = SunAzimuth;
    *p3 = SunElevation;
}

// PANZERS 0x6baa90
void SScene::SetFog(float r, float g, float b, float start, float end, float alpha)
{
    PZ_TRACE("SScene::SetFog (0x6baa90)");
    FogRGB[0] = r;
    FogRGB[1] = g;
    FogRGB[2] = b;
    FogEnd = end;
    unsigned rgb = (((unsigned)(int)(r * 255.0f) << 8 | (unsigned)(int)(g * 255.0f)) << 8) |
                   (unsigned)(int)(b * 255.0f);
    FogColor = rgb;
    FogStart = start;
    FogColorAlpha = (unsigned)(int)(alpha * 255.0f) << 24 | rgb;
    FogInvRange = 1.0f / (end - start);
    if (g_Menu3D.Trace)
        Logger.g->Log(0, "  SetFog rgb %.3f %.3f %.3f start %.2f end %.2f alpha %.2f", r, g, b, start, end, alpha);
}

// PANZERS 0x680620 (SGepard::SetLights, scene light heap)
void SScene::SetLights()
{
    IDirect3DDevice9* dev = HD().Device;
    int n = 0;
    for (int i = Lights.Next(-1); i >= 0; i = Lights.Next(i)) {
        const SSceneLight& l = Lights[i];
        D3DLIGHT9 d;
        memset(&d, 0, sizeof(d));
        memcpy(&d.Diffuse, l.Color, 16);
        memcpy(&d.Specular, l.Color, 16);
        if (l.Type == 3) {
            memcpy(&d.Direction, l.Dir, 12);
        } else if (l.Type == 1) {
            memcpy(&d.Position, l.Pos, 12);
            d.Range = l.Range;
            d.Attenuation0 = l.Atten[0];
            d.Attenuation1 = l.Atten[1];
            d.Attenuation2 = l.Atten[2];
        } else if (l.Type == 2) {
            memcpy(&d.Position, l.Pos, 12);
            memcpy(&d.Direction, l.Dir, 12);
            d.Range = l.Range;
            d.Attenuation0 = l.Atten[0];
            d.Attenuation1 = l.Atten[1];
            d.Attenuation2 = l.Atten[2];
            d.Falloff = 1.0f;
            d.Theta = l.Theta;
            d.Phi = l.Phi;
        } else {
            Logger.g->Panic("SGepard::SetLights: Unsupported light type");
        }
        d.Type = (D3DLIGHTTYPE)l.Type;
        dev->SetLight(n, &d);
        dev->LightEnable(n, TRUE);
        if (7 < ++n)
            break;
    }
    HD().LightCount = n;
    for (; n < 8; ++n)
        dev->LightEnable(n, FALSE);
}

// ---------------------------------------------------------------------------
// models
// ---------------------------------------------------------------------------

// PANZERS 0x6a89d0
// flag 0: main heap +0x178 (sorted into terrain cells, updated through the
// cells); otherwise the free heap +0x18c (updated and drawn every frame).
SIModel* SScene::CreateModel(int proto, int proto2, bool flag)
{
    PZ_TRACE("SScene::CreateModel (0x6a89d0)");
    SPModel* p = GepardModelPrototype(proto);
    if (!p) {
        Logger.g->Panic("SScene::CreateModel: Invalid prototype index");
        return nullptr;
    }
    SPModel* p2 = GepardModelPrototype(proto2);
    if (!flag) {
        int i = Models.Add();
        SModel* m = new SModel(this, p, p2, false, i);
        Models[i] = m;
        ModelsSorted = false;
        return m;
    }
    int i = FreeModels.Add();
    SModel* m = new SModel(this, p, p2, true, i);
    FreeModels[i] = m;
    return m;
}

// PANZERS 0x6a8bc0
SIModel* SScene::CreateModelFromPrototype(int proto, int flag)
{
    PZ_TRACE("SScene::CreateModelFromPrototype (0x6a8bc0)");
    return CreateModel(proto, -1, (char)flag != 0);
}

// PANZERS 0x6a8d50
SIModel* SScene::CreateModelFromFile(const char* file, float scale, int p3, int p4)
{
    PZ_TRACE("SScene::CreateModelFromFile (0x6a8d50)");
    SIGepardHD* g = PzGepard();
    int proto = g->LoadModelPrototype(file, scale, nullptr, p4);   // Gepard +0x20(file, scale, p4, 0)
    if (proto < 0)
        return nullptr;
    SIModel* m = CreateModelFromPrototype(proto, p3);
    g->ReleaseModelPrototype(proto);   // Gepard +0x24
    return m;
}

void SScene::RemoveModel(SModel* m)
{
    if (m->Flag) {
        if (FreeModels.Valid(m->Index) && FreeModels[m->Index] == m)
            FreeModels.Remove(m->Index);
    } else if (Models.Valid(m->Index) && Models[m->Index] == m) {
        Models.Remove(m->Index);
        ModelsSorted = false;
    }
    for (int i = 0; i < DeferredCount; ++i)
        if (Deferred[i] == m)
            Deferred[i] = nullptr;
}

// PANZERS 0x6d5820 (SDArray<SModel*>::Add on scene +0x2a0)
void SScene::AddDeferredModel(SModel* m)
{
    if (DeferredCount == DeferredMax) {
        int n = DeferredMax < 0x10 ? 0x10 : (DeferredMax * 6) / 5;
        Deferred = (SModel**)realloc(Deferred, n * sizeof(SModel*));
        memset(Deferred + DeferredMax, 0, (n - DeferredMax) * sizeof(SModel*));
        DeferredMax = n;
    }
    Deferred[DeferredCount++] = m;
}

// PANZERS 0x6ac210
// Links every visible main-heap model into the terrain cells (8x8 tiles)
// its world AABB covers; CellTails keeps the highest model top per cell.
void SScene::SortModelsIntoCells()
{
    int cells = CellsX * CellsZ;
    memset(CellHeads, 0xff, cells * 4);
    memset(CellTails, 0xfe, cells * 4);
    CellLinkCount = 0;   // 0x6a22c0(0)
    for (int i = Models.Next(-1); i >= 0; i = Models.Next(i)) {
        SModel* m = Models[i];
        if (!m->IsVisible())
            continue;
        m->Update(FrameCount, 0);   // SIAttachable +0x04
        float minX, maxX, minY, maxY, minZ, maxZ;
        m->GetWorldBounds(&minX, &maxX, &minY, &maxY, &minZ, &maxZ);
        // FISTP under FLDCW 0x047f (floor) for the minimum, 0x087f (ceil) for the maximum.
        int x0 = 0.0f <= minX ? (int)floorf(minX * 0.125f) : 0;
        int x1 = maxX <= (float)TerrainW ? (int)ceilf(maxX * 0.125f) : CellsX;
        int z0 = 0.0f <= minZ ? (int)floorf(minZ * 0.125f) : 0;
        int z1 = maxZ <= (float)TerrainH ? (int)ceilf(maxZ * 0.125f) : CellsZ;
        for (int z = z0; z < z1; ++z) {
            for (int x = x0; x < x1; ++x) {
                int c = CellsX * z + x;
                if (CellLinkCount == CellLinkMax) {
                    int n = CellLinkMax < 0x10 ? 0x10 : (CellLinkMax * 6) / 5;
                    CellLinks = (SCellLink*)realloc(CellLinks, n * sizeof(SCellLink));
                    memset(CellLinks + CellLinkMax, 0, (n - CellLinkMax) * sizeof(SCellLink));
                    CellLinkMax = n;
                }
                int k = CellLinkCount++;
                CellLinks[k].Model = i;
                CellLinks[k].Next = CellHeads[c];
                CellHeads[c] = k;
                float* top = (float*)&CellTails[c];
                if (*top < maxY)
                    *top = maxY;
            }
        }
    }
    ModelsSorted = true;
}

// PANZERS 0x6bbcb0
// Updates (SModel::Update with the frame stamp) the main-heap models in the
// visible terrain cells and, when "all", the free-heap models.
void SScene::UpdateModels(SViewport* vp, bool all)
{
    (void)vp;   // HD builds the (unused here) view basis from the viewport
    if (Terrain) {
        if (!ModelsSorted)
            SortModelsIntoCells();
        int cells = CellsX * CellsZ;
        for (int c = 0; c < cells; ++c) {
            if (!Terrain->Parcels[c].Visible)
                continue;
            for (int k = CellHeads[c]; k >= 0; k = CellLinks[k].Next) {
                int i = CellLinks[k].Model;
                if (!Models.Valid(i))
                    continue;
                SModel* m = Models[i];
                if (m->IsVisible() && m->Frame != FrameCount) {
                    float p[3];
                    m->GetPosition(p);   // +0x10
                    m->Update(FrameCount, 0);
                }
            }
        }
    }
    if (!all)
        return;
    for (int i = FreeModels.Next(-1); i >= 0; i = FreeModels.Next(i)) {
        SModel* m = FreeModels[i];
        if (m->AttachParent || !m->IsVisible())
            continue;
        if (m->Flags & 8)
            continue;
        // With terrain and without flag 0x10, HD skips models the pixie
        // fog-of-war test 0x69db40 hides (agent C/D); not applied here.
        m->Update(FrameCount, 0);
    }
}

// PANZERS 0x6b02a0
void SScene::DrawModels(SViewport* vp, int p2)
{
    (void)p2;
    int drawn = 0, cellsVisible = 0;
    if (Terrain) {
        int cells = CellsX * CellsZ;
        for (int c = 0; c < cells; ++c) {
            if (!Terrain->Parcels[c].Visible)
                continue;
            ++cellsVisible;
            for (int k = CellHeads[c]; k >= 0; k = CellLinks[k].Next) {
                int i = CellLinks[k].Model;
                if (Models.Valid(i) && Models[i]->Frame == FrameCount) {
                    Models[i]->Render(vp);
                    ++drawn;
                }
            }
        }
    }
    if (g_Menu3D.Trace && (FrameCount % 300) == 2)
        Logger.g->Log(0, "  DrawModels: %d main models, %d free, %d cell links, %d/%d cells visible, %d drawn",
                      Models.Count, FreeModels.Count, CellLinkCount, cellsVisible, CellsX * CellsZ, drawn);
    for (int i = FreeModels.Next(-1); i >= 0; i = FreeModels.Next(i))
        if (FreeModels[i]->Frame == FrameCount)
            FreeModels[i]->Render(vp);
}

// ---------------------------------------------------------------------------
// terrain
// ---------------------------------------------------------------------------

// PANZERS 0x6a9dd0
SITerrain* SScene::CreateTerrain(int width, int height, int p3)
{
    PZ_TRACE("SScene::CreateTerrain (0x6a9dd0)");
    TerrainW = width;
    TerrainH = height;
    CellsX = width / 8;
    CellsZ = height / 8;
    Terrain = new STerrain(this, width, height, p3);
    CellHeads = (int*)malloc((size_t)CellsX * CellsZ * 4);
    CellTails = (int*)malloc((size_t)CellsX * CellsZ * 4);
    ModelsSorted = false;
    return Terrain;
}

// PANZERS 0x6aab20
void SScene::DestroyTerrain()
{
    PZ_TRACE("SScene::DestroyTerrain (0x6aab20)");
    free(CellHeads);
    CellHeads = nullptr;
    free(CellTails);
    CellTails = nullptr;
    // HD: pixie +0x44(scene) drops the effect decals on the terrain (agent C).
    if (Terrain) {
        delete Terrain;
        Terrain = nullptr;
    }
}

// ---------------------------------------------------------------------------
// frame
// ---------------------------------------------------------------------------

// PANZERS 0x6bbc40
void SScene::PrepareViewport(SViewport* vp)
{
    PZ_TRACE("SScene::PrepareViewport (0x6bbc40)");
    FrameMs = TimeMs - LastTimeMs;
    LastTimeMs = TimeMs;
    Seconds = (float)((double)Timer.GetTickValue() / 1000.0);   // 0x661800
    if (Terrain)
        Terrain->Cull(vp);
    UpdateModels(vp, true);
    if (Terrain)
        GenerateShadowBuffer(vp);   // 0x6aac20
}

// PANZERS 0x6a24c0
void SScene::UpdateViewport(SViewport* vp)
{
    PZ_TRACE("SScene::UpdateViewport (0x6a24c0)");
    vp->Clear(FogColor, 1.0f, 0);   // SViewport::Clear 0x689f10
}

// Device states HD sets once (SGepard 0x67cc30 / 0x688230) and the HD scene
// relies on; the SWINE frame changes several of them before our pass.
static void ApplyHdDeviceDefaults(IDirect3DDevice9* dev)
{
    for (unsigned i = 0; i < HD().SamplerStages; ++i) {
        dev->SetSamplerState(i, D3DSAMP_MAXANISOTROPY, 1);
        dev->SetSamplerState(i, D3DSAMP_MAXMIPLEVEL, 0);
    }
    dev->SetRenderState(D3DRS_DITHERENABLE, TRUE);
    dev->SetRenderState(D3DRS_LOCALVIEWER, FALSE);
    dev->SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID);
    dev->SetRenderState(D3DRS_NORMALIZENORMALS, FALSE);   // D3D default; HD never sets it
    dev->SetRenderState(D3DRS_SPECULARENABLE, FALSE);     // D3D default
    dev->SetRenderState(D3DRS_COLORVERTEX, TRUE);         // D3D default
    dev->SetPixelShader(nullptr);
    dev->SetVertexShader(nullptr);
    for (int i = 0; i < 8; ++i)
        dev->SetTexture(i, nullptr);
}

// PANZERS 0x6acaf0
void SScene::RenderViewport(SViewport* vp)
{
    PZ_TRACE("SScene::RenderViewport (0x6acaf0)");
    IDirect3DDevice9* dev = HD().Device;
    if (!dev)
        return;
    // Recompile: the render-pass cache reset HD runs at device init
    // (0x688230), every frame, since SWINE draws in between.
    ApplyHdDeviceDefaults(dev);
    InvalidateRenderPassCache();
    vp->ApplyTransforms();
    SetLights();                                      // 0x680620
    GepardSetAmbient(Ambient);                        // 0x680510
    SetPassFog(FogStart, FogEnd, FogColor);           // 0x688ac0
    g_HdFog.Enabled = FogStart < FogEnd;              // agent B's copy of the fog
    g_HdFog.Color = FogColor;
    g_HdFog.Start = FogStart;
    g_HdFog.End = FogEnd;
    if (Terrain) {
        Terrain->Render(vp);                          // 0x6f2aa0
        if (GepardOption(2) != 0 && ShadowViewport >= 0)
            Terrain->RenderShadowPass();              // 0x6f46e0
    }
    if (Terrain && !Terrain->GetCompactMode()) {
        STUB_LOG("SScene::DrawSea (0x6b04d0)");
    }
    STUB_LOG("SScene::DrawSkybox (0x6b7920)");      // KSYB empty in menu.map
    DeferredCount = 0;                                // 0x6a20f0(0) on +0x2a0
    DrawModels(vp, 0);                                // 0x6b02a0
    if (Terrain)
        STUB_LOG("SScene::DrawTerrainDecals (0x6ad0e0)");
    STUB_LOG("SScene::DrawTrails (0x6acf20)");
    if (Terrain)
        Terrain->RenderLate(vp);                      // 0x6f33c0
    STUB_LOG("SScene::DrawLakes (0x6ad740)");       // LAKS: 0 in menu.map
    DrawRivers(vp);                                   // 0x6b0920
    STUB_LOG("SScene::DrawWires (0x6b8b10)");
    STUB_LOG("SScene::DrawDecals2 (0x6b7b30)");
    SPixie* pixie = static_cast<SPixie*>(static_cast<SPzGepard*>(PzGepard())->Pixie);
    if (pixie) {
        pixie->UpdateFrame(FrameCount);              // pixie +0x48
        pixie->Render(this, vp);                      // 0x69ea50
    }
    g_MeshDraw.DrawingDeferred = true;                // 0x93cee9
    for (int i = 0; i < DeferredCount; ++i)
        if (Deferred[i])
            Deferred[i]->Render(vp);
    g_MeshDraw.DrawingDeferred = false;
    // HD: GetOption(2) > 2 && GetOption(4): debug quad of the shadow buffer.
    ++FrameCount;
}

// PANZERS 0x6a2670
// Drops the shadow buffer: the models' second terrain shadow decal (0x6da0c0),
// the offscreen viewport (Gepard +0x40) and its texture.
void SScene::ReleaseShadowBuffer()
{
    for (int i = FreeModels.Next(-1); i >= 0; i = FreeModels.Next(i)) {
        SModel* m = FreeModels[i];
        if (m->ShadowDecal2 >= 0)
            m->ShadowDecal2 = -1;   // terrain +0x64 (decals are not placed by the menu)
    }
    if (ShadowViewport >= 0)
        PzGepard()->DestroyViewport(ShadowViewport);
    ShadowTexture = -1;
    ShadowViewport = -1;
}

// PANZERS 0x6aac20
// Renders the shadow buffer before the frame (from PrepareViewport):
//  - the offscreen viewport, made on first use with the Gepard option 3
//    size: technique 4 G16R16 (the height in red), 3 X8R8G8B8, 2 a D24X8
//    depth texture beside X8R8G8B8 / R5G6B5, 1 R5G6B5; linear filtering
//    except for technique 1;
//  - the light projection (shadowmath.cpp) as the PROJECTION with an
//    identity VIEW, cleared to white;
//  - the casters drawn black with the lights off: the terrain (depth, and
//    the height for techniques 3/4), then the models of the visible cells
//    and the free-heap models;
//  - scene +0x58 becomes camera space -> shadow texture for the receivers.
// Recompile: the SWINE frame tests the device only after this, so a lost
// device skips the pass, and a D3DSBT_ALL state block keeps the shadow
// pass's device state from the SWINE frame.
void SScene::GenerateShadowBuffer(SViewport* vp)
{
    PZ_TRACE("SScene::GenerateShadowBuffer (0x6aac20)");
    int tech = GepardOption(2);
    if (tech == 0)
        return;
    IDirect3DDevice9* dev = HD().Device;
    if (!dev || dev->TestCooperativeLevel() != D3D_OK)
        return;
    if (ShadowViewport < 0) {
        int size = GepardOption(3);
        unsigned fmt, flags = 3;
        if (tech == 2) {
            fmt = (HD().VendorId == 0x10de && HD().DeviceId < 0x250) ? D3DFMT_X8R8G8B8 : D3DFMT_R5G6B5;
            flags = 7;
        } else if (tech == 3) {
            fmt = D3DFMT_X8R8G8B8;
        } else if (tech == 4) {
            fmt = D3DFMT_G16R16;
        } else {
            fmt = D3DFMT_R5G6B5;
        }
        ShadowViewport = GepardCreateRenderTarget(size, size, fmt, flags);   // 0x678c70
        if (ShadowViewport < 0)
            return;
        if (tech == 2 || tech == 3 || tech == 4)
            GepardSetTextureFilter(GepardRenderTargetTexture(ShadowViewport, false), 1);   // 0x680bf0
    }
    IDirect3DStateBlock9* saved = nullptr;
    if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &saved)))
        saved = nullptr;
    GepardSelectRenderTarget(ShadowViewport);   // 0x6803e0

    // The matrices.
    static std::vector<SShadowCell> s_Cells;   // reused every frame
    int cells = CellsX * CellsZ;
    s_Cells.resize(cells);
    for (int c = 0; c < cells; ++c) {
        SShadowCell& sc = s_Cells[c];
        sc.Visible = Terrain->Parcels[c].Visible;
        sc.MinY = Terrain->Parcels[c].MinY;
        sc.MaxY = Terrain->Parcels[c].MaxY;
        memcpy(&sc.ModelTop, &CellTails[c], 4);   // a float in the int array
    }
    SShadowFrame f;
    memset(&f, 0, sizeof(f));
    memcpy(f.Cam.View, vp->View, sizeof(f.Cam.View));
    memcpy(f.Cam.Proj, vp->Proj, sizeof(f.Cam.Proj));
    f.Cam.Near = vp->Camera.NearZ;
    f.Cam.Far = vp->Camera.FarZ;
    float camX, camZ, pitch;
    vp->GetCamera(&camX, &f.CamY, &camZ, &f.Yaw, &pitch);   // vp +0x24
    memcpy(f.SunDir, SunDir, sizeof(f.SunDir));
    f.SunAzimuth = SunAzimuth;
    f.SunElevation = SunElevation;
    f.Cells = s_Cells.data();
    f.CellsX = CellsX;
    f.CellsZ = CellsZ;
    f.Technique = tech;
    f.BufferSize = GepardOption(3);
    f.DepthTextureFlag = HD().DepthTextureFlag;
    ShadowFrameMatrices(f);
    FocusHeight = f.FocusHeight;
    ShadowCull = f.Cull;
    float identity[16];
    Identity44(identity);
    dev->SetTransform(D3DTS_VIEW, (const D3DMATRIX*)identity);
    dev->SetTransform(D3DTS_PROJECTION, (const D3DMATRIX*)f.Projection);
    memcpy(ShadowProjection, f.Projection, 64);
    if (tech == 2) {
        float bias = 4.0e-05f, slope = 1.1f;   // 0x3827c5ac, 0x3f8ccccd
        dev->SetRenderState(D3DRS_DEPTHBIAS, *(DWORD*)&bias);
        dev->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, *(DWORD*)&slope);
    }
    if (tech == 3 || tech == 4)
        memcpy(ShadowMatrix, f.PassMatrix, 64);
    GepardClearRenderTarget(0xffffff, 1.0f, 0);   // 0x689f10
    if (FAILED(dev->BeginScene())) {
        Logger.g->Log(0, "%s", "SScene::GenerateShadowBuffer\\IDirect3DDevice::BeginScene failed");
        GepardUnselectRenderTarget();
        if (saved) {
            saved->Apply();
            saved->Release();
        }
        return;
    }
    // Recompile: the device state the HD passes expect (the SWINE frame
    // ran in between), as RenderViewport does.
    ApplyHdDeviceDefaults(dev);
    InvalidateRenderPassCache();
    GepardEnableLights(false);                  // 0x67a720(0)
    const float black[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    GepardSetAmbient(black);                    // 0x680510
    Terrain->RenderShadowCasters();             // 0x6f4530
    for (int c = 0; c < cells; ++c) {
        if (!Terrain->Parcels[c].Visible)
            continue;
        for (int k = CellHeads[c]; k >= 0; k = CellLinks[k].Next) {
            int i = CellLinks[k].Model;
            if (!Models.Valid(i))
                continue;
            SModel* m = Models[i];
            if (m->Frame == FrameCount) {
                float p[3];
                m->GetPosition(p);              // +0x10 (unused)
                m->RenderShadow(vp);            // 0x6d9570
            }
        }
    }
    for (int i = FreeModels.Next(-1); i >= 0; i = FreeModels.Next(i)) {
        SModel* m = FreeModels[i];
        if (m->Frame != FrameCount)
            continue;
        if (m->ShadowDecal < 0 || GepardOption(5) != 0)
            m->RenderShadow(vp);
    }
    // HD also draws the wires (0x6b8b10(vp, 1)); DrawWires is not lifted.
    if (FAILED(dev->EndScene()))
        Logger.g->Log(0, "%s", "SScene::GenerateShadowBuffer\\IDirect3DDevice::EndScene failed");
    GepardUnselectRenderTarget();               // 0x6814b0
    memcpy(ShadowMatrix, f.TexMatrix, 64);
    if (tech == 1 && GepardOption(4) != 0) {
        if (ShadowTexture == -1)
            ShadowTexture = HD().DebugShadowTexture;   // SGepard +0x548
    } else {
        ShadowTexture = GepardRenderTargetTexture(ShadowViewport, tech == 2);
    }
    GepardEnableLights(true);
    GepardSetAmbient(Ambient);
    if (tech == 2) {
        dev->SetRenderState(D3DRS_DEPTHBIAS, 0);
        dev->SetRenderState(D3DRS_SLOPESCALEDEPTHBIAS, 0);
    }
    if (saved) {
        saved->Apply();
        saved->Release();
    }
    InvalidateRenderPassCache();
}

// 0x6b0920: water courses of scene +0x220 (Slot_B0, rivers and waterfalls;
// 6170 instructions). menu.map has no RVR2, so the heap stays empty.
void SScene::DrawRivers(SViewport* vp)
{
    (void)vp;
    if (Rivers.Count == 0 || RiverTexture < 0)
        return;
    STUB_LOG("SScene::DrawRivers (0x6b0920)");
}

// PANZERS 0x6b7760
// The full-frame variant (minimap / screenshot targets). Not on the menu path.
void SScene::RenderScene()
{
    STUB_LOG("SScene::RenderScene (0x6b7760)");
    PZ_TRACE("SScene::RenderScene (0x6b7760)");
}

// ---- generated slot stubs (HD vtable order) ----

// HD SScene vtbl +0x0c -> 0x6b0000 (3 arg dwords)
int SScene::Slot_0C_CreateMinimapTarget(int p1, int w2, int h2)
{
    STUB_LOG("SScene::Slot_0C_CreateMinimapTarget (0x6b0000)");
    PZ_TRACE("SScene::Slot_0C_CreateMinimapTarget (0x6b0000)");
    (void)p1; (void)w2; (void)h2;
    return 0;
}

// HD SScene vtbl +0x10 -> 0x6ac7d0 (2 arg dwords)
void SScene::Slot_10()
{
    STUB_LOG("SScene::Slot_10 (0x6ac7d0)");
    PZ_TRACE("SScene::Slot_10 (0x6ac7d0)");
}

// HD SScene vtbl +0x14 -> 0x6ac5b0 (4 arg dwords)
void SScene::Slot_14()
{
    STUB_LOG("SScene::Slot_14 (0x6ac5b0)");
    PZ_TRACE("SScene::Slot_14 (0x6ac5b0)");
}

// HD SScene vtbl +0x18 -> 0x6ac5e0 (3 arg dwords)
void SScene::Slot_18()
{
    STUB_LOG("SScene::Slot_18 (0x6ac5e0)");
    PZ_TRACE("SScene::Slot_18 (0x6ac5e0)");
}

// HD SScene vtbl +0x28 -> 0x6ac6e0 (0 arg dwords)
void SScene::Slot_28()
{
    STUB_LOG("SScene::Slot_28 (0x6ac6e0)");
    PZ_TRACE("SScene::Slot_28 (0x6ac6e0)");
}

// HD SScene vtbl +0x5c -> 0x6ba8a0 (4 arg dwords)
void SScene::Slot_5C()
{
    STUB_LOG("SScene::Slot_5C (0x6ba8a0)");
    PZ_TRACE("SScene::Slot_5C (0x6ba8a0)");
}

// HD SScene vtbl +0x60 -> 0x6ba810 (2 arg dwords)
void SScene::ReplaceModel(SIModel* model, int proto)
{
    STUB_LOG("SScene::ReplaceModel (0x6ba810)");
    PZ_TRACE("SScene::ReplaceModel (0x6ba810)");
    (void)model; (void)proto;
}

// HD SScene vtbl +0x6c -> 0x6a9000 (5 arg dwords)
void SScene::Slot_6C()
{
    STUB_LOG("SScene::Slot_6C (0x6a9000)");
    PZ_TRACE("SScene::Slot_6C (0x6a9000)");
}

// HD SScene vtbl +0x70 -> 0x6aa8e0 (1 arg dword)
void SScene::Slot_70()
{
    STUB_LOG("SScene::Slot_70 (0x6aa8e0)");
    PZ_TRACE("SScene::Slot_70 (0x6aa8e0)");
}

// HD SScene vtbl +0x74 -> 0x6bad60 (2 arg dwords)
void SScene::Slot_74()
{
    STUB_LOG("SScene::Slot_74 (0x6bad60)");
    PZ_TRACE("SScene::Slot_74 (0x6bad60)");
}

// HD SScene vtbl +0x78 -> 0x6badd0 (5 arg dwords)
void SScene::Slot_78()
{
    STUB_LOG("SScene::Slot_78 (0x6badd0)");
    PZ_TRACE("SScene::Slot_78 (0x6badd0)");
}

// HD SScene vtbl +0x7c -> 0x6a9850 (15 arg dwords)
void SScene::Slot_7C()
{
    STUB_LOG("SScene::Slot_7C (0x6a9850)");
    PZ_TRACE("SScene::Slot_7C (0x6a9850)");
}

// HD SScene vtbl +0x80 -> 0x6bb680 (6 arg dwords)
void SScene::Slot_80()
{
    STUB_LOG("SScene::Slot_80 (0x6bb680)");
    PZ_TRACE("SScene::Slot_80 (0x6bb680)");
}

// HD SScene vtbl +0x84 -> 0x6aaad0 (0 arg dwords)
void SScene::Slot_84()
{
    STUB_LOG("SScene::Slot_84 (0x6aaad0)");
    PZ_TRACE("SScene::Slot_84 (0x6aaad0)");
}

// HD SScene vtbl +0x88 -> 0x6a2780 (1 arg dword)
void SScene::Slot_88()
{
    STUB_LOG("SScene::Slot_88 (0x6a2780)");
    PZ_TRACE("SScene::Slot_88 (0x6a2780)");
}

// HD SScene vtbl +0x8c -> 0x6a7790 (6 arg dwords)
void SScene::Slot_8C()
{
    STUB_LOG("SScene::Slot_8C (0x6a7790)");
    PZ_TRACE("SScene::Slot_8C (0x6a7790)");
}

// HD SScene vtbl +0x90 -> 0x6bb320 (4 arg dwords)
void SScene::Slot_90()
{
    STUB_LOG("SScene::Slot_90 (0x6bb320)");
    PZ_TRACE("SScene::Slot_90 (0x6bb320)");
}

// HD SScene vtbl +0x94 -> 0x6a2710 (1 arg dword)
void SScene::Slot_94()
{
    STUB_LOG("SScene::Slot_94 (0x6a2710)");
    PZ_TRACE("SScene::Slot_94 (0x6a2710)");
}

// HD SScene vtbl +0x98 -> 0x6aa320 (1 arg dword)
void SScene::Slot_98()
{
    STUB_LOG("SScene::Slot_98 (0x6aa320)");
    PZ_TRACE("SScene::Slot_98 (0x6aa320)");
}

// HD SScene vtbl +0x9c -> 0x6a7940 (8 arg dwords)
int SScene::CreateLake(const char* p1, const char* p2, float p3, float p4, short* p5, int p6, short* p7, int p8)
{
    STUB_LOG("SScene::CreateLake (0x6a7940)");
    PZ_TRACE("SScene::CreateLake (0x6a7940)");
    (void)p1; (void)p2; (void)p3; (void)p4; (void)p5; (void)p6; (void)p7; (void)p8;
    return -1;
}

// HD SScene vtbl +0xa0 -> 0x6aa3f0 (1 arg dword)
void SScene::DestroyLake(int lake)
{
    STUB_LOG("SScene::DestroyLake (0x6aa3f0)");
    PZ_TRACE("SScene::DestroyLake (0x6aa3f0)");
    (void)lake;
}

// HD SScene vtbl +0xa4 -> 0x6a1f90 (2 arg dwords)
void SScene::Slot_A4()
{
    STUB_LOG("SScene::Slot_A4 (0x6a1f90)");
    PZ_TRACE("SScene::Slot_A4 (0x6a1f90)");
}

// HD SScene vtbl +0xa8 -> 0x6aabe0 (0 arg dwords)
void SScene::Slot_A8()
{
    STUB_LOG("SScene::Slot_A8 (0x6aabe0)");
    PZ_TRACE("SScene::Slot_A8 (0x6aabe0)");
}

// HD SScene vtbl +0xac -> 0x6abcb0 (5 arg dwords)
void SScene::Slot_AC()
{
    STUB_LOG("SScene::Slot_AC (0x6abcb0)");
    PZ_TRACE("SScene::Slot_AC (0x6abcb0)");
}

// HD SScene vtbl +0xb0 -> 0x6a90e0 (16 arg dwords)
void SScene::Slot_B0()
{
    STUB_LOG("SScene::Slot_B0 (0x6a90e0)");
    PZ_TRACE("SScene::Slot_B0 (0x6a90e0)");
}

// HD SScene vtbl +0xb4 -> 0x6aa910 (1 arg dword)
void SScene::Slot_B4()
{
    STUB_LOG("SScene::Slot_B4 (0x6aa910)");
    PZ_TRACE("SScene::Slot_B4 (0x6aa910)");
}

// HD SScene vtbl +0xb8 -> 0x6ac0b0 (1 arg dword)
void SScene::Slot_B8()
{
    STUB_LOG("SScene::Slot_B8 (0x6ac0b0)");
    PZ_TRACE("SScene::Slot_B8 (0x6ac0b0)");
}

// HD SScene vtbl +0xbc -> 0x6ac150 (2 arg dwords)
void SScene::Slot_BC()
{
    STUB_LOG("SScene::Slot_BC (0x6ac150)");
    PZ_TRACE("SScene::Slot_BC (0x6ac150)");
}

// HD SScene vtbl +0xc0 -> 0x6abfc0 (5 arg dwords)
void SScene::Slot_C0()
{
    STUB_LOG("SScene::Slot_C0 (0x6abfc0)");
    PZ_TRACE("SScene::Slot_C0 (0x6abfc0)");
}

// HD SScene vtbl +0xc4 -> 0x6a9ee0 (3 arg dwords)
void SScene::Slot_C4()
{
    STUB_LOG("SScene::Slot_C4 (0x6a9ee0)");
    PZ_TRACE("SScene::Slot_C4 (0x6a9ee0)");
}

// HD SScene vtbl +0xc8 -> 0x6aa050 (4 arg dwords)
void SScene::Slot_C8()
{
    STUB_LOG("SScene::Slot_C8 (0x6aa050)");
    PZ_TRACE("SScene::Slot_C8 (0x6aa050)");
}

// HD SScene vtbl +0xcc -> 0x6c0b40 (2 arg dwords)
void SScene::UpdateWire(int wire, int* p2)
{
    STUB_LOG("SScene::UpdateWire (0x6c0b40)");
    PZ_TRACE("SScene::UpdateWire (0x6c0b40)");
    (void)wire; (void)p2;
}

// HD SScene vtbl +0xd0 -> 0x6aaba0 (1 arg dword)
void SScene::Slot_D0()
{
    STUB_LOG("SScene::Slot_D0 (0x6aaba0)");
    PZ_TRACE("SScene::Slot_D0 (0x6aaba0)");
}

// HD SScene vtbl +0xd4 -> 0x6aa280 (0 arg dwords)
void SScene::Slot_D4()
{
    STUB_LOG("SScene::Slot_D4 (0x6aa280)");
    PZ_TRACE("SScene::Slot_D4 (0x6aa280)");
}

// HD SScene vtbl +0xd8 -> 0x6a1d80 (5 arg dwords)
void SScene::Slot_D8()
{
    STUB_LOG("SScene::Slot_D8 (0x6a1d80)");
    PZ_TRACE("SScene::Slot_D8 (0x6a1d80)");
}

// HD SScene vtbl +0xdc -> 0x6a1cd0 (8 arg dwords)
void SScene::Slot_DC()
{
    STUB_LOG("SScene::Slot_DC (0x6a1cd0)");
    PZ_TRACE("SScene::Slot_DC (0x6a1cd0)");
}

// HD SScene vtbl +0xe0 -> 0x6a14f0 (6 arg dwords)
void SScene::Slot_E0()
{
    STUB_LOG("SScene::Slot_E0 (0x6a14f0)");
    PZ_TRACE("SScene::Slot_E0 (0x6a14f0)");
}

// HD SScene vtbl +0xe4 -> 0x6a1b00 (5 arg dwords)
void SScene::Slot_E4()
{
    STUB_LOG("SScene::Slot_E4 (0x6a1b00)");
    PZ_TRACE("SScene::Slot_E4 (0x6a1b00)");
}

// HD SScene vtbl +0xe8 -> 0x6a1790 (6 arg dwords)
void SScene::Slot_E8()
{
    STUB_LOG("SScene::Slot_E8 (0x6a1790)");
    PZ_TRACE("SScene::Slot_E8 (0x6a1790)");
}

// HD SScene vtbl +0xec -> 0x6a24e0 (0 arg dwords)
void SScene::Slot_EC()
{
    STUB_LOG("SScene::Slot_EC (0x6a24e0)");
    PZ_TRACE("SScene::Slot_EC (0x6a24e0)");
}

// HD SScene vtbl +0xf0 -> 0x6a24f0 (1 arg dword)
void SScene::Slot_F0()
{
    STUB_LOG("SScene::Slot_F0 (0x6a24f0)");
    PZ_TRACE("SScene::Slot_F0 (0x6a24f0)");
}

// HD SScene vtbl +0xf4 -> 0x6aabf0 (1 arg dword)
void SScene::Slot_F4()
{
    STUB_LOG("SScene::Slot_F4 (0x6aabf0)");
    PZ_TRACE("SScene::Slot_F4 (0x6aabf0)");
}

// HD SScene vtbl +0xf8 -> 0x6abca0 (0 arg dwords)
void SScene::Slot_F8()
{
    STUB_LOG("SScene::Slot_F8 (0x6abca0)");
    PZ_TRACE("SScene::Slot_F8 (0x6abca0)");
}

// HD SScene vtbl +0xfc -> 0x6a96c0 (2 arg dwords)
void SScene::SetSkybox(const char* file, float radius)
{
    STUB_LOG("SScene::SetSkybox (0x6a96c0)");
    PZ_TRACE("SScene::SetSkybox (0x6a96c0)");
    (void)file; (void)radius;
}

// HD SScene vtbl +0x100 -> 0x6aa9a0 (0 arg dwords)
void SScene::ClearSkybox()
{
    STUB_LOG("SScene::ClearSkybox (0x6aa9a0)");
    PZ_TRACE("SScene::ClearSkybox (0x6aa9a0)");
}

// HD SScene vtbl +0x104 -> 0x6ba950 (0 arg dwords)
// PANZERS 0x6ba950
// Drops the shadow buffer (Graphics Apply, after a new buffer size); the
// next frame creates it again. The texture handle +0x14 is left as is.
void SScene::Slot_104_RecreateShadowBuffer()
{
    PZ_TRACE("SScene::Slot_104_RecreateShadowBuffer (0x6ba950)");
    PzGepard()->DestroyViewport(ShadowViewport);   // Gepard +0x40
    ShadowViewport = -1;
}

} // namespace pz
