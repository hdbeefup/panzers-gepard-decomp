// src/tools/modelview/modelview.cpp
// modelview (agent M1-A): draws one .4d model through the HD 3D path only:
// the Gepard facade (LoadModelPrototype), SScene (CreateModel, lights, fog,
// RenderViewport), SModel (Update, Render, sequences), SPModel/SPAnim
// (.4d v100/v101, CANM) and SViewport (SetCamera, SetProjection, Render).
// Nothing from SWINE's 3D object code is used; the SWINE SGepard only owns
// the device, the texture table and the frame (Clear/Present).
//
//   modelview.exe <rundir> --model <path.4d> [--seq <sequence>] [--list]
//                 [--yaw rad] [--pitch rad] [--dist m] [--spin rad/s]
//                 [--seconds N] [--shot <file.png> --at <seconds>]...
//                 [--hide <node>]...
//
// <rundir> holds panzers.ini and the paks ([Paths] Search). Writes
// modelview.log there. "--shot a.png --at 2.5" saves the back buffer after
// 2.5 s of animation time (repeatable); the window closes after the last
// shot or after --seconds.

#include <windows.h>
#include <direct.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <d3d9.h>
#include <d3dx9.h>

#include "dxwindow.h"
#include "gepard.h"
#include "board.h"
#include "logger.h"
#include "stream.h"
#include "properties.h"

#include "igepardhd.h"
#include "pzgepard.h"
#include "pzscene.h"
#include "pzmodel.h"
#include "pzviewport.h"
#include "pmodel.h"

extern HINSTANCE hInstance;

#define MV_LOG(...) Logger.g->Log(-1, __VA_ARGS__)

namespace {

struct SShot {
    const char* File;
    float At;
    bool Done;
};

struct SViewState {
    pz::SScene*    Scene = nullptr;
    pz::SModel*    Model = nullptr;
    pz::SViewport* Viewport = nullptr;
    float Center[3] = { 0, 0, 0 };
    float Radius = 3.0f;
    float Yaw = 0.6f;
    float Pitch = -0.35f;
    float Dist = 0.0f;
    float Spin = 0.0f;
    double Time = 0.0;
    DWORD LastTick = 0;
    int Frames = 0;
    SShot Shots[16];
    int ShotCount = 0;
    unsigned Seconds = 0;
    DWORD StartTick = 0;
} g_View;

void OpenLog(const char* path)
{
    SLogger* log = new SLogger((char*)"modelview", (char*)"modelview", false, 2);
    FILE* f = fopen(path, "wb");
    if (f)
        fclose(f);
    log->LogFileName = _strdup(path);
    log->LogToFile = true;
    log->Logging = true;
    log->LogLevel = 2;
    Logger.g = log;
}

bool SaveBackBuffer(const char* file)
{
    IDirect3DDevice9* dev = pz::HD().Device;
    IDirect3DSurface9* bb = nullptr;
    if (!dev || FAILED(dev->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &bb)))
        return false;
    HRESULT hr = D3DXSaveSurfaceToFileA(file, D3DXIFF_PNG, bb, nullptr, nullptr);
    bb->Release();
    return SUCCEEDED(hr);
}

struct SModelViewWindow : SDXWindow {
    bool Done = false;

    bool OnIdle() override
    {
        if (!::Gepard || Done)
            return false;
        SViewState& v = g_View;
        DWORD now = GetTickCount();
        float dt = v.LastTick ? (now - v.LastTick) * 0.001f : 0.0f;
        if (dt > 0.1f)
            dt = 0.1f;
        v.LastTick = now;
        // Fixed 1/30 s steps so the shot times are deterministic.
        dt = 1.0f / 30.0f;
        v.Time += dt;
        v.Scene->AdvanceTime((int)(dt * 1000.0f));          // scene +0x1c
        v.Scene->SetInterpolation(0.0);                      // scene +0x20
        v.Model->AdvanceAnimation(dt);                       // model +0x70
        float yaw = v.Yaw + v.Spin * (float)v.Time;
        float dir[3] = { sinf(yaw) * cosf(v.Pitch), sinf(v.Pitch), cosf(yaw) * cosf(v.Pitch) };
        float eye[3];
        for (int i = 0; i < 3; ++i)
            eye[i] = v.Center[i] - dir[i] * v.Dist;
        // SWorld::ComputeCamera 0x5ddc30 passes the eye, the yaw and -pitch.
        v.Viewport->SetCamera(eye[0], eye[1], eye[2], yaw, -v.Pitch);
        v.Viewport->Render(v.Scene, 0);                      // viewport +0x50
        ++v.Frames;
        if (v.Frames == 1)
            MV_LOG("modelview: first frame");
        bool pending = false;
        for (int i = 0; i < v.ShotCount; ++i) {
            SShot& s = v.Shots[i];
            if (!s.Done && v.Time >= s.At) {
                s.Done = true;
                bool ok = SaveBackBuffer(s.File);
                MV_LOG("modelview: shot %s at %.2f s (seq %d t %.3f) -> %s", s.File, v.Time,
                       v.Model->Anim.Seq, v.Model->Anim.Time, ok ? "ok" : "FAILED");
            }
            pending |= !s.Done;
        }
        if ((v.ShotCount && !pending) ||
            (v.Seconds && now - v.StartTick >= v.Seconds * 1000)) {
            Done = true;
            OnClose();
        }
        return true;
    }

    LRESULT WindowProc(unsigned int message, unsigned int wParam, int lParam) override
    {
        if (message == WM_KEYDOWN && wParam == VK_ESCAPE) {
            OnClose();
            return 0;
        }
        return SDXWindow::WindowProc(message, wParam, lParam);
    }
};

} // namespace

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    hInstance = hInst;
    const char* rundir = nullptr;
    const char* model = nullptr;
    const char* seq = nullptr;
    const char* hide[16];
    int hideCount = 0;
    bool list = false;
    SViewState& v = g_View;
    for (int i = 1; i < __argc; ++i) {
        const char* a = __argv[i];
        if (!strcmp(a, "--model") && i + 1 < __argc) model = __argv[++i];
        else if (!strcmp(a, "--seq") && i + 1 < __argc) seq = __argv[++i];
        else if (!strcmp(a, "--yaw") && i + 1 < __argc) v.Yaw = (float)atof(__argv[++i]);
        else if (!strcmp(a, "--pitch") && i + 1 < __argc) v.Pitch = (float)atof(__argv[++i]);
        else if (!strcmp(a, "--dist") && i + 1 < __argc) v.Dist = (float)atof(__argv[++i]);
        else if (!strcmp(a, "--spin") && i + 1 < __argc) v.Spin = (float)atof(__argv[++i]);
        else if (!strcmp(a, "--seconds") && i + 1 < __argc) v.Seconds = (unsigned)atoi(__argv[++i]);
        else if (!strcmp(a, "--hide") && i + 1 < __argc && hideCount < 16) hide[hideCount++] = __argv[++i];
        else if (!strcmp(a, "--list")) list = true;
        else if (!strcmp(a, "--shot") && i + 1 < __argc && v.ShotCount < 16) {
            v.Shots[v.ShotCount].File = __argv[++i];
            v.Shots[v.ShotCount].At = 1.0f;
            v.Shots[v.ShotCount].Done = false;
            ++v.ShotCount;
        } else if (!strcmp(a, "--at") && i + 1 < __argc && v.ShotCount) {
            v.Shots[v.ShotCount - 1].At = (float)atof(__argv[++i]);
        } else if (!rundir) rundir = a;
    }
    if (rundir && _chdir(rundir) != 0)
        return 2;
    char logpath[MAX_PATH];
    GetCurrentDirectoryA(sizeof(logpath) - 20, logpath);
    strcat(logpath, "\\modelview.log");
    OpenLog(logpath);
    if (!model) {
        MV_LOG("modelview: usage: modelview <rundir> --model <file.4d> [--seq name] ...");
        return 2;
    }

    SProperties* ini = new SProperties("panzers.ini", true);
    const char* search = ini->GetString("Paths", "Search", nullptr);
    if (search)
        FileSystem.SetSearchPath(search, false);
    FileSystem.SetHomePath(ini->GetString("Paths", "home", ""));
    delete ini;

    SModelViewWindow* wnd = new SModelViewWindow();
    wnd->SetPosition(40, 40, 1024, 768);
    int rc = wnd->Create((HICON)0, (HCURSOR)IDC_ARROW, L"Panzers modelview");
    if (rc != 0 || !::Gepard) {
        MV_LOG("modelview: device creation failed (%d)", rc);
        return 3;
    }

    pz::SIGepardHD* g = pz::PzGepard();
    g->SetOption(0x10, 1);   // MODULATE2X materials (the world weather update 0x6088f0 sets it)
    pz::SIScene* scene = g->CreateScene();
    v.Scene = static_cast<pz::SScene*>(scene);
    // Light and fog in the range of the menu weather ("Default").
    const float ambient[4] = { 0.45f, 0.45f, 0.45f, 1.0f };
    const float sun[4] = { 0.85f, 0.83f, 0.78f, 1.0f };
    scene->SetAmbientLight(ambient);                  // scene +0x3c
    scene->SetSunLight(sun, 2.4f, -0.75f);            // scene +0x44 (azimuth, elevation)
    scene->SetFog(0.62f, 0.66f, 0.70f, 60.0f, 200.0f, 1.0f);   // scene +0x4c

    int proto = g->LoadModelPrototype(model, 0.005f, nullptr, 0);   // Gepard +0x20 (world: scale 0.005)
    if (proto < 0) {
        MV_LOG("modelview: %s did not load", model);
        return 4;
    }
    v.Model = static_cast<pz::SModel*>(scene->CreateModel(proto, -1, true));   // scene +0x54, free heap
    pz::SPModel* p = v.Model->Proto;
    MV_LOG("modelview: %s: %d nodes, %d sequences (%s), FLYZ %d", model, p->NodeCount, p->SequenceCount,
           p->FrameSequences ? "frames" : "skeletal", (int)p->FlyZ);
    for (int i = 0; i < p->NodeCount; ++i) {
        pz::SPModelNode& n = p->Nodes[i];
        MV_LOG("  node %2d '%s' parent %d mesh %s bones %d", i, n.Name.buf ? n.Name.buf : "", n.Parent,
               n.Mesh ? "yes" : "no", n.BoneCount);
    }
    if (list)
        for (int i = 0; i < p->SequenceCount; ++i)
            MV_LOG("  seq %3d '%s' len %.3f blend %.3f next %d type %d", i,
                   p->Sequences[i].Name.buf ? p->Sequences[i].Name.buf : "", p->Sequences[i].Length,
                   p->Sequences[i].BlendTime, p->Sequences[i].Next, p->Sequences[i].Type);
    // As SDoodad::Initialize 0x5ee040: hide the "Block" / "Platform" helpers.
    const char* helpers[] = { "Block", "Platform" };
    for (const char* h : helpers) {
        int n = v.Model->FindNode(h);   // model +0x40
        if (n >= 0)
            v.Model->SetNodeVisible(n, false);   // model +0x60
    }
    for (int i = 0; i < hideCount; ++i) {
        int n = v.Model->FindNode(hide[i]);
        if (n >= 0)
            v.Model->SetNodeVisible(n, false);
    }
    v.Model->SetPosition(0.0f, 0.0f, 0.0f);
    v.Model->SetRotation(0.0f, 0.0f, 0.0f);
    v.Model->SetFlags(0);
    if (seq) {
        v.Model->PlaySequence(seq, false);   // model +0x6c
        MV_LOG("modelview: sequence '%s' -> %d (len %.3f)", seq, v.Model->Anim.Seq,
               p->SequenceCount ? p->Sequences[v.Model->Anim.Seq].Length : 0.0f);
    }
    // Frame the model: world bounds of the mesh nodes.
    v.Model->Update(v.Scene->FrameCount, 0);
    float mn[3], mx[3];
    v.Model->GetWorldBounds(&mn[0], &mx[0], &mn[1], &mx[1], &mn[2], &mx[2]);
    float r = 0.0f;
    for (int i = 0; i < 3; ++i) {
        v.Center[i] = (mn[i] + mx[i]) * 0.5f;
        r = fmaxf(r, mx[i] - mn[i]);
    }
    v.Radius = r > 0.1f ? r : 1.0f;
    if (v.Dist <= 0.0f)
        v.Dist = v.Radius * 1.6f + 1.0f;
    MV_LOG("modelview: bounds (%.2f %.2f %.2f)-(%.2f %.2f %.2f) dist %.2f", mn[0], mn[1], mn[2], mx[0], mx[1],
           mx[2], v.Dist);
    v.Viewport = static_cast<pz::SViewport*>(g->GetViewport(0));
    v.Viewport->SetProjection(60.0f * 3.14159265f / 180.0f, 0.1f, 200.0f);   // viewport +0x28

    v.StartTick = GetTickCount();
    wnd->Run();
    MV_LOG("modelview: %d frames, %.2f s", v.Frames, v.Time);
    v.Model->Release();
    scene->Release();
    g->PurgeModelPrototypes();
    pz::PzGepardShutdown();
    delete wnd;
    return v.Frames > 0 ? 0 : 5;
}
