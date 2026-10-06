// src/tools/terrainview/terrainview.cpp
// terrainview: draws the terrain of a Panzers map (default maps/menu.map)
// with pz::STerrain - 16-layer blend, roads, junctions and map decals - from
// the map's camera, in a window, through the HD scene-pass hook of the SWINE
// frame. Test tool of agent B (M1); it has its own minimal map reader (the
// real one is SWorld::LoadMap in src/world).
//
//   terrainview.exe <rundir> [--map maps/menu.map] [--seconds N] [--shot out.bmp]
//                   [--layers N] [--noroads] [--nodecals]
//
// <rundir> holds panzers.ini and the paks. Writes terrainview.log there.
// The world side it stands in for: TERR 0x5f2fc0, layer setup 0x608360,
// ROD2 0x5f0a30 + 0x601c10, RODJ 0x5f0b60 + 0x6029b0, DECS 0x5efcd0 +
// 0x5edfd0, weather 0x5fdc80 / 0x6088f0, CAM 0x5fd7c0 + ComputeCamera 0x5ddc30.

#include <windows.h>
#include <direct.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "dxwindow.h"
#include "gepard.h"
#include "board.h"
#include "logger.h"
#include "stream.h"
#include "properties.h"

#include "pzterrain.h"
#include "../../3dengine/pz/parcel.h"   // not SWINE 3dengine/parcel.h
#include "pzscene.h"
#include "pzviewport.h"
#include "igepardhd.h"

extern HINSTANCE hInstance;

#define TV_LOG(...) Logger.g->Log(-1, __VA_ARGS__)

namespace {

// ---- map reading ----

struct SReader {
    const unsigned char* P;
    const unsigned char* End;
    unsigned U() { unsigned v; memcpy(&v, P, 4); P += 4; return v; }
    int I() { int v; memcpy(&v, P, 4); P += 4; return v; }
    float F() { float v; memcpy(&v, P, 4); P += 4; return v; }
    unsigned char B() { return *P++; }
    std::string S() { unsigned short n; memcpy(&n, P, 2); P += 2; std::string s((const char*)P, n); P += n; return s; }
};

struct SChunk {
    const unsigned char* Data;
    unsigned Size;
};

// First chunk with this tag inside [p, end) (chunks are tag, u32 size, body).
static bool FindChunk(const unsigned char* p, const unsigned char* end, const char* tag, SChunk* out)
{
    while (p + 8 <= end) {
        unsigned size;
        memcpy(&size, p + 4, 4);
        if (!memcmp(p, tag, 4)) {
            out->Data = p + 8;
            out->Size = size;
            return true;
        }
        p += 8 + size;
    }
    return false;
}

static std::vector<unsigned char> ReadFile(const char* name)
{
    std::vector<unsigned char> d;
    SStream* s = FileSystem.OpenRead(name, "terrainview");
    if (!s)
        return d;
    unsigned char buf[65536];
    for (;;) {
        int n = s->ReadMax(buf, sizeof(buf));
        if (n <= 0)
            break;
        d.insert(d.end(), buf, buf + n);
    }
    s->Release();
    return d;
}

static std::string WithTga(const std::string& s)
{
    return strrchr(s.c_str(), '.') ? s : s + ".tga";
}

// HD 0x5ec6c0: hue (degrees), saturation and value (percent) to RGB.
static void HsvToRgb(int hue, int sat, int val, float* r, float* g, float* b)
{
    float fr = 1.0f, fg, fb;
    if (hue < 60) { fb = 0.0f; fg = hue / 60.0f; }
    else {
        fg = 1.0f;
        if (hue < 120) { fr = (120.0f - hue) / 60.0f; fb = 0.0f; }
        else if (hue < 180) { fr = 0.0f; fb = (hue - 120.0f) / 60.0f; }
        else {
            fb = 1.0f;
            if (hue < 240) { fr = 0.0f; fg = (240.0f - hue) / 60.0f; }
            else {
                fg = 0.0f;
                if (hue < 300) fr = (hue - 240.0f) / 60.0f;
                else fb = (360.0f - hue) / 60.0f;
            }
        }
    }
    float s = sat / 100.0f, v = val / 100.0f;
    *r = (s * fr + (1.0f - s)) * v;
    *g = (s * fg + (1.0f - s)) * v;
    *b = (s * fb + (1.0f - s)) * v;
}

// ---- state ----

struct SView {
    pz::SScene*    Scene = nullptr;
    pz::STerrain*  Terrain = nullptr;
    pz::SViewport* Viewport = nullptr;
    float Eye[3], Target[3];
    float Yaw = 0, Pitch = 0, Dist = 0;
    float Fov = 1.0471976f, Near = 1.0f, Far = 100.0f;
    float Ambient[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    float Sun[4] = { 0.5f, 0.5f, 0.5f, 1.0f };
    float SunDir[3] = { 0.0f, -1.0f, 0.0f };
    int Frames = 0;
    std::string Shot;
    bool ShotDone = false;
};
SView g_View;

static void SaveBackBuffer(IDirect3DDevice9* dev, const char* path)
{
    IDirect3DSurface9 *bb = nullptr, *sys = nullptr;
    if (FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb)))
        return;
    D3DSURFACE_DESC d;
    bb->GetDesc(&d);
    if (SUCCEEDED(dev->CreateOffscreenPlainSurface(d.Width, d.Height, d.Format, D3DPOOL_SYSTEMMEM, &sys, nullptr))
        && SUCCEEDED(dev->GetRenderTargetData(bb, sys))) {
        D3DLOCKED_RECT lr;
        if (SUCCEEDED(sys->LockRect(&lr, nullptr, D3DLOCK_READONLY))) {
            FILE* f = fopen(path, "wb");
            if (f) {
                int w = d.Width, h = d.Height, row = w * 3, pad = (4 - row % 4) % 4;
                BITMAPFILEHEADER fh = {};
                BITMAPINFOHEADER ih = {};
                fh.bfType = 0x4d42;
                fh.bfOffBits = sizeof(fh) + sizeof(ih);
                fh.bfSize = fh.bfOffBits + (row + pad) * h;
                ih.biSize = sizeof(ih);
                ih.biWidth = w;
                ih.biHeight = h;
                ih.biPlanes = 1;
                ih.biBitCount = 24;
                fwrite(&fh, sizeof(fh), 1, f);
                fwrite(&ih, sizeof(ih), 1, f);
                std::vector<unsigned char> line(row + pad, 0);
                for (int y = h - 1; y >= 0; --y) {
                    const unsigned char* src = (const unsigned char*)lr.pBits + y * lr.Pitch;
                    for (int x = 0; x < w; ++x) {
                        line[x * 3 + 0] = src[x * 4 + 0];
                        line[x * 3 + 1] = src[x * 4 + 1];
                        line[x * 3 + 2] = src[x * 4 + 2];
                    }
                    fwrite(line.data(), 1, line.size(), f);
                }
                fclose(f);
                TV_LOG("terrainview: wrote %s (%dx%d)", path, w, h);
            }
            sys->UnlockRect();
        }
    }
    if (sys) sys->Release();
    bb->Release();
}

static void SetMatrices(IDirect3DDevice9* dev)
{
    SView& v = g_View;
    float f[3] = { v.Target[0] - v.Eye[0], v.Target[1] - v.Eye[1], v.Target[2] - v.Eye[2] };
    float fl = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    for (float& c : f) c /= fl;
    float r[3] = { f[2], 0.0f, -f[0] };   // up (0,1,0) x forward, left-handed
    float rl = sqrtf(r[0] * r[0] + r[2] * r[2]);
    r[0] /= rl; r[2] /= rl;
    float u[3] = { f[1] * r[2] - f[2] * r[1], f[2] * r[0] - f[0] * r[2], f[0] * r[1] - f[1] * r[0] };
    D3DMATRIX view = {};
    view._11 = r[0]; view._21 = r[1]; view._31 = r[2];
    view._12 = u[0]; view._22 = u[1]; view._32 = u[2];
    view._13 = f[0]; view._23 = f[1]; view._33 = f[2];
    view._41 = -(r[0] * v.Eye[0] + r[1] * v.Eye[1] + r[2] * v.Eye[2]);
    view._42 = -(u[0] * v.Eye[0] + u[1] * v.Eye[1] + u[2] * v.Eye[2]);
    view._43 = -(f[0] * v.Eye[0] + f[1] * v.Eye[1] + f[2] * v.Eye[2]);
    view._44 = 1.0f;
    dev->SetTransform(D3DTS_VIEW, &view);
    D3DVIEWPORT9 vp;
    dev->GetViewport(&vp);
    float aspect = vp.Height ? (float)vp.Width / (float)vp.Height : 4.0f / 3.0f;
    float ys = 1.0f / tanf(v.Fov * 0.5f), xs = ys / aspect;
    D3DMATRIX proj = {};
    proj._11 = xs;
    proj._22 = ys;
    proj._33 = v.Far / (v.Far - v.Near);
    proj._34 = 1.0f;
    proj._43 = -v.Near * v.Far / (v.Far - v.Near);
    dev->SetTransform(D3DTS_PROJECTION, &proj);
}

// The HD scene pass stand-in: what SScene::RenderViewport does for the
// terrain (lights, camera, Cull, Render, RenderLate).
static void TerrainViewPass(IDirect3DDevice9* dev)
{
    SView& v = g_View;
    if (!v.Terrain)
        return;
    IDirect3DStateBlock9* saved = nullptr;
    if (FAILED(dev->CreateStateBlock(D3DSBT_ALL, &saved)))
        saved = nullptr;
    dev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0xff404850, 1.0f, 0);
    SetMatrices(dev);
    D3DLIGHT9 sun = {};
    sun.Type = D3DLIGHT_DIRECTIONAL;
    sun.Diffuse.r = v.Sun[0]; sun.Diffuse.g = v.Sun[1]; sun.Diffuse.b = v.Sun[2]; sun.Diffuse.a = 1.0f;
    sun.Direction.x = v.SunDir[0]; sun.Direction.y = v.SunDir[1]; sun.Direction.z = v.SunDir[2];
    dev->SetLight(0, &sun);
    dev->LightEnable(0, TRUE);
    for (int i = 1; i < 8; ++i)
        dev->LightEnable(i, FALSE);
    dev->SetRenderState(D3DRS_AMBIENT, D3DCOLOR_COLORVALUE(v.Ambient[0], v.Ambient[1], v.Ambient[2], 1.0f));
    dev->SetRenderState(D3DRS_NORMALIZENORMALS, TRUE);

    v.Terrain->Cull(v.Viewport);                       // 0x6f1180 (PrepareViewport)
    v.Terrain->Render(v.Viewport);                     // 0x6f2aa0
    if (pz::PzGepard()->GetOption(2) != 0)
        v.Terrain->RenderShadowPass();                 // 0x6f46e0
    v.Terrain->RenderLate(v.Viewport);                 // 0x6f33c0

    ++v.Frames;
    if (!v.Shot.empty() && !v.ShotDone && v.Frames >= 5) {
        SaveBackBuffer(dev, v.Shot.c_str());
        v.ShotDone = true;
    }
    if (saved) {
        saved->Apply();
        saved->Release();
    }
}

struct STerrainViewWindow : SDXWindow {
    DWORD StartTick = 0;
    DWORD Seconds = 0;
    bool OnIdle() override
    {
        bool r = SDXWindow::OnIdle();
        if (Seconds && GetTickCount() - StartTick >= Seconds * 1000)
            this->OnClose();
        return r;
    }
    LRESULT WindowProc(unsigned int message, unsigned int wParam, int lParam) override
    {
        if (message == WM_KEYDOWN && wParam == VK_ESCAPE) {
            this->OnClose();
            return 0;
        }
        return SDXWindow::WindowProc(message, wParam, lParam);
    }
};

static void OpenLog(const char* path)
{
    SLogger* log = new SLogger((char*)"terrainview", (char*)"terrainview", false, 2);
    FILE* f = fopen(path, "wb");
    if (f)
        fclose(f);
    log->LogFileName = _strdup(path);
    log->LogToFile = true;
    log->Logging = true;
    log->LogLevel = 2;
    Logger.g = log;
}

// Loads the map into a fresh STerrain the way SWorld does.
static bool LoadMap(const char* mapName, int maxLayers, bool roads, bool decals)
{
    SView& v = g_View;
    std::vector<unsigned char> d = ReadFile(mapName);
    if (d.size() < 16) {
        TV_LOG("terrainview: cannot read %s", mapName);
        return false;
    }
    const unsigned char* p = d.data();
    const unsigned char* end = p + d.size();
    if (!memcmp(p, "Sr\x1a\x1b", 4))
        p += 8;
    SChunk mapf;
    if (!FindChunk(p, end, "MAPF", &mapf))
        return false;
    const unsigned char* mp = mapf.Data + 4;   // version
    const unsigned char* me = mapf.Data + mapf.Size;

    SChunk terr, ch;
    if (!FindChunk(mp, me, "TERR", &terr))
        return false;
    const unsigned char* tp = terr.Data + 4;
    const unsigned char* te = terr.Data + terr.Size;
    if (!FindChunk(tp, te, "HMAP", &ch))
        return false;
    SReader r = { ch.Data, ch.Data + ch.Size };
    int w = r.I(), h = r.I();
    v.Scene = new pz::SScene(0);
    v.Terrain = new pz::STerrain(v.Scene, w, h, 4);            // scene +0x64 CreateTerrain(w, h, 4)
    pz::STerrain* t = v.Terrain;
    pz::STerrainBuffers b;
    t->Acquire(&b);
    memcpy(b.Heights, r.P, (w + 1) * (h + 1) * 4);
    TV_LOG("terrainview: HMAP %dx%d", w, h);

    std::vector<std::string> layerNames;
    std::vector<unsigned> layerFlags;
    std::vector<std::string> layerFlora;
    if (FindChunk(tp, te, "TLAY", &ch)) {
        SReader q = { ch.Data, ch.Data + ch.Size };
        unsigned n = q.U();
        for (unsigned i = 0; i < n; ++i) {
            layerNames.push_back(q.S());
            unsigned fl = q.U();
            std::string flora;
            if (fl & 2)
                flora = q.S();
            else if (fl & 8) {
                fl = (fl & ~8u) | 2;
                flora = "99 Regi";
            }
            layerFlags.push_back(fl);
            layerFlora.push_back(flora);
        }
    }
    int nl = (int)layerNames.size();
    if (FindChunk(tp, te, "BLND", &ch)) {
        // (layers - 1) planes of one byte per vertex, into byte k of the 16.
        int verts = (w + 1) * (h + 1);
        for (int k = 0; k < nl - 1 && k < 16; ++k)
            for (int i = 0; i < verts; ++i)
                b.Blend[i * 0x10 + k] = ch.Data[k * verts + i];
    }
    if (FindChunk(tp, te, "DIFF", &ch))
        memcpy(b.Diffuse, ch.Data, (w + 1) * (h + 1) * 4);
    if (FindChunk(tp, te, "TMAP", &ch))
        memcpy(b.BufferB0, ch.Data, (w / 8) * (h / 8) * 2);

    // SWorld 0x608360: layer i = 0..16 texture; flora and flags go to i - 1.
    for (int i = 0; i < 17; ++i) {
        if (i < nl && i < maxLayers) {
            t->LoadLayerTexture(i, ("tiles/" + layerNames[i]).c_str());
            if (i > 0) {
                std::string fl = layerFlora[i].empty() ? "" : "flora/" + layerFlora[i] + "/";
                t->LoadFloraLayer(i - 1, fl.empty() ? nullptr : fl.c_str(), 0.005f);
                t->SetLayerFlags(i - 1, layerFlags[i]);
            }
        } else {
            t->LoadLayerTexture(i, nullptr);
            if (i > 0) {
                t->LoadFloraLayer(i - 1, nullptr, 0.005f);
                t->SetLayerFlags(i - 1, 0);
            }
        }
        TV_LOG("terrainview: layer %d %s -> tex %d", i, i < nl ? layerNames[i].c_str() : "-",
               t->LayerTextures[0][i]);
    }
    t->Invalidate(0, 0, w + 1, h + 1);

    // Weather 0 -> ambient and sun (option 0x10 halves both, HD 0x6ba970 / 0x6baef0).
    if (FindChunk(mp, me, "WTHR", &ch)) {
        SReader q = { ch.Data, ch.Data + ch.Size };
        if (q.U() > 0) {
            q.S();
            int n = q.I(), vals[20] = {};
            for (int i = 0; i < n && i < 20; ++i)
                vals[i] = q.I();
            HsvToRgb(vals[0], vals[1], vals[2], &v.Ambient[0], &v.Ambient[1], &v.Ambient[2]);
            HsvToRgb(vals[3], vals[4], vals[5], &v.Sun[0], &v.Sun[1], &v.Sun[2]);
            float a1 = vals[6] * 0.01745329238f, a2 = vals[7] * -0.01745329238f;
            for (int i = 0; i < 3; ++i) {
                v.Ambient[i] *= 0.5f;
                v.Sun[i] *= 0.5f;
            }
            v.SunDir[0] = sinf(a1) * cosf(a2);
            v.SunDir[1] = sinf(a2);
            v.SunDir[2] = cosf(a1) * cosf(a2);
            // Scene sun direction for STerrain::ComputeSunOcclusion (scene+0x110).
            float* sd = reinterpret_cast<float*>(reinterpret_cast<unsigned char*>(v.Scene) + 0x110);
            sd[0] = v.SunDir[0]; sd[1] = v.SunDir[1]; sd[2] = v.SunDir[2];
            TV_LOG("terrainview: weather ambient %.2f %.2f %.2f sun %.2f %.2f %.2f dir %.2f %.2f %.2f",
                   v.Ambient[0], v.Ambient[1], v.Ambient[2], v.Sun[0], v.Sun[1], v.Sun[2],
                   v.SunDir[0], v.SunDir[1], v.SunDir[2]);
        }
    }

    // ROD2 -> CreateRoad (world 0x601c10: step = file float 1 / 2,
    // width = texture height / 64, texture length = texture width / 64).
    if (roads && FindChunk(mp, me, "ROD2", &ch)) {
        SReader q = { ch.Data, ch.Data + ch.Size };
        unsigned n = q.U();
        q.U(); q.U();
        for (unsigned i = 0; i < n; ++i) {
            if (q.U() != 0x7fffffff)
                continue;
            std::string tex = q.S();
            float f1 = q.F();
            q.F();
            unsigned flags = q.U();
            unsigned np = q.U();
            std::vector<pz::SRoadPoint> pts(np);
            for (unsigned k = 0; k < np; ++k) {
                pz::SRoadPoint& o = pts[k];
                memset(&o, 0, sizeof(o));
                o.X = q.F();
                o.Z = q.F();
                o.DirX = q.F();
                o.DirZ = q.F();
                q.F();
                o.W = (float)q.U();
                o.Dirty = 1;
            }
            int th = pz::TerrainLoadTexture(WithTga(tex).c_str(), 1, true), tw2, th2;
            pz::TerrainTextureSize(th, &tw2, &th2);
            pz::SRoadPointArray arr = { pts.data(), (int)np, (int)np };
            int road = t->CreateRoad(th, &arr, f1 * 0.5f, (float)th2 / 64.0f, (float)tw2 / 64.0f, flags);
            TV_LOG("terrainview: road %d '%s' tex %d (%dx%d) flags 0x%x points %u", road, tex.c_str(),
                   th, tw2, th2, flags, np);
        }
    }
    // RODJ -> CreateRoadJunction (world 0x6029b0: half sizes = texH / 128, texW / 128).
    if (roads && FindChunk(mp, me, "RODJ", &ch)) {
        SReader q = { ch.Data, ch.Data + ch.Size };
        unsigned n = q.U();
        q.U(); q.U();
        for (unsigned i = 0; i < n; ++i) {
            if (q.U() != 0x7fffffff)
                continue;
            std::string tex = q.S();
            q.F(); q.F();
            unsigned flags = q.U();
            pz::SRoadJunctionPoint jp = {};
            jp.X = q.F();
            jp.Z = q.F();
            jp.DirX = q.F();
            jp.DirZ = q.F();
            jp.Valid = 1;
            q.F();
            q.U();
            unsigned nc = q.U();
            for (unsigned k = 0; k < nc; ++k) { q.I(); q.B(); }
            int tx = pz::TerrainLoadTexture(WithTga(tex).c_str(), 1, true), tw2, th2;
            pz::TerrainTextureSize(tx, &tw2, &th2);
            int j = t->CreateRoadJunction(tx, &jp, (float)th2 / 128.0f, (float)tw2 / 128.0f, flags);
            TV_LOG("terrainview: junction %d '%s' tex %d (%dx%d) at %.1f %.1f flags 0x%x", j, tex.c_str(),
                   tx, tw2, th2, jp.X, jp.Z, flags);
        }
    }
    // ENTS/DECS -> AddDecal + SetDecalType (world 0x5f1070 / 0x5edfd0).
    SChunk ents;
    if (decals && FindChunk(mp, me, "ENTS", &ents)
        && FindChunk(ents.Data + 4, ents.Data + ents.Size, "DECS", &ch)) {
        SReader q = { ch.Data, ch.Data + ch.Size };
        unsigned n = q.U();
        for (unsigned i = 0; i < n; ++i) {
            q.P += 4;   // "DECA"
            unsigned size = q.U();
            const unsigned char* next = q.P + size;
            q.U();      // v100
            std::string tex = q.S();
            int x = q.I(), z = q.I(), rot = q.I();
            int tx = pz::TerrainLoadTexture(tex.c_str(), 1, true);
            t->AddDecal((int)i, tx, x, z, rot);
            t->SetDecalType((int)i, 0);
            pz::TerrainReleaseTexture(tx);
            q.P = next;
        }
        TV_LOG("terrainview: %u map decals", n);
    }
    t->UpdateDecals();

    // CAM: target x/z, yaw, pitch, distance (0x5fd7c0); eye = target - dir * dist (0x5ddc30 mode 0).
    v.Yaw = 6.224015f; v.Pitch = -0.637599f; v.Dist = 19.08f;
    v.Target[0] = 97.645f; v.Target[2] = 95.742f;
    if (FindChunk(mp, me, "CAM ", &ch)) {
        SReader q = { ch.Data, ch.Data + ch.Size };
        v.Target[0] = q.F(); v.Target[2] = q.F(); v.Yaw = q.F(); v.Pitch = q.F(); v.Dist = q.F();
    }
    t->Update();
    v.Target[1] = t->HeightAt(v.Target[0], v.Target[2]);
    float dir[3] = { sinf(v.Yaw) * cosf(v.Pitch), sinf(v.Pitch), cosf(v.Yaw) * cosf(v.Pitch) };
    for (int i = 0; i < 3; ++i)
        v.Eye[i] = v.Target[i] - dir[i] * v.Dist;
    TV_LOG("terrainview: camera target %.2f %.2f %.2f eye %.2f %.2f %.2f", v.Target[0], v.Target[1],
           v.Target[2], v.Eye[0], v.Eye[1], v.Eye[2]);
    v.Viewport = static_cast<pz::SViewport*>(pz::PzGepard()->GetViewport(0));
    v.Viewport->SetCamera(v.Eye[0], v.Eye[1], v.Eye[2], v.Yaw, -v.Pitch);
    v.Viewport->SetProjection(v.Fov, v.Near, v.Far);
    return true;
}

} // namespace

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    hInstance = hInst;
    const char* rundir = nullptr;
    const char* map = "maps/menu.map";
    unsigned seconds = 0;
    int maxLayers = 17;
    bool roads = true, decals = true;
    for (int i = 1; i < __argc; ++i) {
        const char* a = __argv[i];
        if (!strcmp(a, "--map") && i + 1 < __argc) map = __argv[++i];
        else if (!strcmp(a, "--seconds") && i + 1 < __argc) seconds = (unsigned)atoi(__argv[++i]);
        else if (!strcmp(a, "--shot") && i + 1 < __argc) g_View.Shot = __argv[++i];
        else if (!strcmp(a, "--layers") && i + 1 < __argc) maxLayers = atoi(__argv[++i]);
        else if (!strcmp(a, "--noroads")) roads = false;
        else if (!strcmp(a, "--nodecals")) decals = false;
        else if (!rundir) rundir = a;
    }
    if (rundir && _chdir(rundir) != 0)
        return 2;
    char logpath[MAX_PATH];
    GetCurrentDirectoryA(sizeof(logpath) - 20, logpath);
    strcat(logpath, "\\terrainview.log");
    OpenLog(logpath);

    SProperties* ini = new SProperties("panzers.ini", true);
    const char* search = ini->GetString("Paths", "Search", nullptr);
    if (search)
        FileSystem.SetSearchPath(search, false);
    FileSystem.SetHomePath(ini->GetString("Paths", "home", ""));
    delete ini;

    STerrainViewWindow* wnd = new STerrainViewWindow();
    wnd->Seconds = seconds;
    wnd->SetPosition(40, 40, 1024, 768);
    int rc = wnd->Create((HICON)0, (HCURSOR)IDC_ARROW, L"Panzers terrainview");
    if (rc != 0 || !::Gepard) {
        TV_LOG("terrainview: device creation failed (%d)", rc);
        return 3;
    }
    pz::PzGepard()->SetOption(0x10, 1);   // set by the world weather update 0x6088f0
    if (!LoadMap(map, maxLayers, roads, decals))
        return 4;
    SGepard::PanzersScenePass = &TerrainViewPass;
    wnd->StartTick = GetTickCount();
    wnd->Run();
    TV_LOG("terrainview: %d frames", g_View.Frames);
    SGepard::PanzersScenePass = nullptr;
    delete g_View.Terrain;
    delete wnd;
    return g_View.Frames > 0 ? 0 : 5;
}
