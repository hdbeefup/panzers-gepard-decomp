// src/tools/fxview/fxview.cpp
// fxview: plays the three effects of the main-menu map through pz::SPixie
// at the menu camera and saves screenshots. OWNER: agent C.
//
//   fxview.exe [--out <dir>] [--seconds N] [--fx <file.fx> ...]
//
// Run from a game directory (panzers.ini [Paths] Search names the paks).
// Without --fx it plays, one after the other, the EEFS entries of
// maps/menu.map at their map positions:
//   effects/smoke/Ground_Dark_Slow_Size3.fx  (92.260, 0.0, 90.740)
//   effects/fire/Fire_From_House_Size2.fx    (91.528, 0.7, 90.786)
//   effects/smoke/Ground_Dark_Fast_Size3.fx  (91.878, 0.0, 90.343)
// and then all three together. Each run simulates N seconds (default 10)
// at a fixed 1/60 s step through SPixie::UpdateFrame (+0x48) and draws
// with SPixie::Render 0x69ea50, as SScene::RenderViewport does; frames at
// 1 s and 5 s are saved as <out>/fx_<name>_1s.png and _5s.png.
//
// Test harness, not HD code: there is no terrain or model (ground height 0,
// a flat clear colour), the scene is a pz::SScene from the Gepard facade
// with its frame fields (+0xa0 frame, +0xac frame ms, +0xf8 light colour)
// written here, and the camera is the menu CAM chunk (target 97.645,
// 95.742, yaw 6.224, pitch -0.638, distance 19.08, 60 degree FOV, near 1,
// far 100) turned into a D3D look-at.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string>
#include <vector>
#include <d3d9.h>
#include <d3dx9.h>

#include "dxwindow.h"
#include "gepard.h"
#include "logger.h"
#include "stream.h"
#include "properties.h"
#include "igepardhd.h"
#include "iviewport.h"
#include "pzpixie.h"
#include "pzscene.h"

extern HINSTANCE hInstance;

static SLogger* g_log = nullptr;
#define FX_LOG(...) Logger.g->Log(-1, __VA_ARGS__)

struct SFxWindow : SDXWindow {
    bool Closed = false;
    bool OnIdle() override { return true; }
    LRESULT WindowProc(unsigned int message, unsigned int wParam, int lParam) override
    {
        if (message == WM_CLOSE || (message == WM_KEYDOWN && wParam == VK_ESCAPE)) {
            Closed = true;
            return 0;
        }
        return SDXWindow::WindowProc(message, wParam, lParam);
    }
};

static void OpenLog(const char* path)
{
    g_log = new SLogger((char*)"fxview", (char*)"fxview", false, 2);
    FILE* f = fopen(path, "wb");
    if (f)
        fclose(f);
    g_log->LogFileName = _strdup(path);
    g_log->LogToFile = true;
    g_log->Logging = true;
    g_log->LogLevel = 2;
    Logger.g = g_log;
}

struct SFxSpec {
    const char* File;
    const char* Tag;
    float Pos[3];
};

static const SFxSpec kMenuFx[] = {
    { "effects/smoke/Ground_Dark_Slow_Size3.fx", "smoke_slow", { 92.26034546f, 0.0f, 90.74023438f } },
    { "effects/fire/Fire_From_House_Size2.fx",   "fire",       { 91.52806091f, 0.7f, 90.78580475f } },
    { "effects/smoke/Ground_Dark_Fast_Size3.fx", "smoke_fast", { 91.87790680f, 0.0f, 90.34318542f } },
};

static void SetSceneFrame(pz::SIScene* s, unsigned frame, int ms)
{
    *(unsigned*)((char*)s + 0xa0) = frame;   // SScene::FrameCount
    *(int*)((char*)s + 0xac) = ms;           // frame milliseconds (0x6bbc40)
}

static void SetCamera(IDirect3DDevice9* dev, pz::SIViewport* vp, int w, int h)
{
    const float tx = 97.645f, tz = 95.742f, yaw = 6.224f, pitch = -0.638f, dist = 19.08f;
    float horiz = dist * cosf(pitch);
    D3DXVECTOR3 at(tx, 0.0f, tz);
    D3DXVECTOR3 eye(tx - sinf(yaw) * horiz, -sinf(pitch) * dist, tz - cosf(yaw) * horiz);
    D3DXVECTOR3 up(0.0f, 1.0f, 0.0f);
    D3DXMATRIX view, proj;
    D3DXMatrixLookAtLH(&view, &eye, &at, &up);
    D3DXMatrixPerspectiveFovLH(&proj, 60.0f * 3.14159265f / 180.0f, (float)w / (float)h, 1.0f, 100.0f);
    dev->SetTransform(D3DTS_VIEW, &view);
    dev->SetTransform(D3DTS_PROJECTION, &proj);
    vp->SetCamera(eye.x, eye.y, eye.z, yaw, pitch);
}

static void Save(IDirect3DDevice9* dev, const std::string& path)
{
    IDirect3DSurface9* bb = nullptr;
    if (SUCCEEDED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb))) {
        HRESULT hr = D3DXSaveSurfaceToFileA(path.c_str(), D3DXIFF_PNG, bb, nullptr, nullptr);
        FX_LOG("fxview: saved %s (hr=0x%08x)", path.c_str(), (unsigned)hr);
        bb->Release();
    }
}

static bool Pump(SFxWindow* wnd)
{
    MSG msg;
    while (PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return !wnd->Closed;
}

static int ParticleCount(pz::SPixie* px)
{
    int n = 0;
    for (int i = 0; i < px->Effects.Size(); ++i) {
        if (!px->Effects.Valid(i) || !px->Effects[i])
            continue;
        pz::SEffectSet* set = px->Effects[i];
        for (int k = 0; k < set->Effects.Size(); ++k)
            if (set->Effects.Valid(k))
                if (pz::SParticles* ps = dynamic_cast<pz::SParticles*>(set->Effects[k]))   // M3: other effect types too
                    n += (int)ps->Particles.size();
    }
    return n;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    hInstance = hInst;
    char cwd[MAX_PATH];
    GetCurrentDirectoryA(sizeof(cwd) - 20, cwd);
    std::string logpath = std::string(cwd) + "\\fxview.log";
    OpenLog(logpath.c_str());

    std::string out = cwd;
    float seconds = 10.0f;
    std::vector<SFxSpec> specs;
    std::vector<std::string> extra;
    for (int i = 1; i < __argc; ++i) {
        const char* a = __argv[i];
        if (!strcmp(a, "--out") && i + 1 < __argc) out = __argv[++i];
        else if (!strcmp(a, "--seconds") && i + 1 < __argc) seconds = (float)atof(__argv[++i]);
        else if (!strcmp(a, "--fx") && i + 1 < __argc) extra.push_back(__argv[++i]);
    }

    SProperties ini("panzers.ini", false);
    const char* search = ini.GetString("Paths", "Search", "");
    FileSystem.SetSearchPath(search ? search : "", false);
    FileSystem.SetHomePath(cwd);
    FX_LOG("fxview: search path \"%s\"", search ? search : "");

    SFxWindow* wnd = new SFxWindow();
    const int W = 1024, H = 768;
    wnd->SetPosition(40, 40, W, H);
    if (wnd->Create((HICON)0, (HCURSOR)IDC_ARROW, L"Panzers fxview") != 0 || !::Gepard) {
        FX_LOG("fxview: device creation failed");
        return 3;
    }
    IDirect3DDevice9* dev = static_cast<SGepard*>(::Gepard)->lpD3DDev;
    RECT rc;
    GetClientRect(wnd->hWnd, &rc);

    pz::SIGepardHD* g = pz::PzGepard();
    pz::SPixie* px = static_cast<pz::SPixie*>(g->GetPixie());
    pz::SIScene* scene = g->CreateScene();
    float* light = (float*)((char*)scene + 0xf8);   // ambient + sun (0x6ba970)
    light[0] = light[1] = light[2] = light[3] = 1.0f;
    pz::SIViewport* vp = g->GetViewport(0);

    struct SRun { std::string Tag; std::vector<SFxSpec> Fx; };
    std::vector<SRun> runs;
    if (extra.empty()) {
        for (const SFxSpec& s : kMenuFx)
            runs.push_back({ s.Tag, { s } });
        runs.push_back({ "all", { kMenuFx[0], kMenuFx[1], kMenuFx[2] } });
    } else {
        for (const std::string& f : extra) {
            SFxSpec s = { f.c_str(), "custom", { 97.645f, 0.0f, 95.742f } };
            std::string tag = f.substr(f.find_last_of("/\\") + 1);
            runs.push_back({ tag.substr(0, tag.find('.')), { s } });
        }
    }

    unsigned frame = 1;
    const int stepMs = 16;
    const float up[3] = { 0.0f, 1.0f, 0.0f };
    for (SRun& run : runs) {
        std::vector<int> protos, handles;
        for (const SFxSpec& s : run.Fx) {
            int p = px->LoadEffectPrototype(s.File, false, false, 0, 0);
            int h = px->CreateEffect(scene, p, s.Pos, up);   // as the EEFS loader 0x5ee9f0
            FX_LOG("fxview: %s -> prototype %d, effect %d", s.File, p, h);
            protos.push_back(p);
            handles.push_back(h);
        }
        int frames = (int)(seconds * 1000.0f / stepMs);
        for (int f = 1; f <= frames; ++f) {
            if (!Pump(wnd))
                break;
            SetSceneFrame(scene, ++frame, stepMs);
            dev->Clear(0, nullptr, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, D3DCOLOR_XRGB(0x86, 0x7a, 0x52), 1.0f, 0);
            dev->BeginScene();
            SetCamera(dev, vp, rc.right - rc.left, rc.bottom - rc.top);
            px->UpdateFrame((int)frame);   // pixie +0x48, as SScene::RenderViewport
            px->Render(scene, vp);         // 0x69ea50
            dev->EndScene();
            dev->Present(nullptr, nullptr, nullptr, nullptr);
            int ms = f * stepMs;
            if (ms == 992 || ms == 4992 || f == frames)
                FX_LOG("fxview: %s t=%.2fs particles=%d", run.Tag.c_str(), ms / 1000.0f, ParticleCount(px));
            if (ms == 992)
                Save(dev, out + "\\fx_" + run.Tag + "_1s.png");
            else if (ms == 4992)
                Save(dev, out + "\\fx_" + run.Tag + "_5s.png");
        }
        for (int h : handles)
            px->DestroyEffect(h);
        for (int p : protos)
            px->ReleaseEffectPrototype(p);
        if (wnd->Closed)
            break;
    }
    FX_LOG("fxview: done");
    scene->Release();
    px->Release();
    delete wnd;
    return 0;
}
