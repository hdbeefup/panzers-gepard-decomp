// src/tools/modelview/animview.cpp
// animview (agent M2-A): drives one unit's animation class (src/game
// unitanim*: SVehicleAnimation + SRunningGear, SWalkerAnimation, ...) with a
// synthetic movement at the HD 20 Hz logic step, and renders it with the HD
// 3D path at 60 frames per second, interpolating between the ticks like
// SSuperWindow::OnIdle (scene +0x20 = (next tick - now) * 20).
//
//   animview.exe <rundir> --unit <units/xxx.unit> [--speed m/s] [--turn deg/s]
//                [--program drive|walk|idle] [--yaw rad] [--pitch rad]
//                [--dist m] [--fixedcam] [--seconds N] [--log-ticks]
//                [--shot <file.png> --at <seconds>]...
//
// Program "drive" (default): 0-4 s straight at --speed, 4-8 s an arc
// (--turn while moving), 8-10 s turn in place, then stand. "walk": the same
// path at the walker speed. "idle": stand only (idle sequences).
//
// The unit and its prototype are zeroed byte blocks with the fields the
// animations read at their HD offsets (src/game/unitanim.h), as agent U's
// SUnit will provide them. No game world: flat ground at y = 0.

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
#include "logger.h"
#include "stream.h"
#include "properties.h"

#include "igepardhd.h"
#include "pzgepard.h"
#include "pzscene.h"
#include "pzmodel.h"
#include "pzviewport.h"
#include "pmodel.h"
#include "unitanim.h"

extern HINSTANCE hInstance;

#define AV_LOG(...) Logger.g->Log(-1, __VA_ARGS__)

namespace {

using namespace pz;

// --- world services of the test (flat ground, local seed) ------------------
unsigned g_Seed = 0x1234;
int g_Tick = 0;
unsigned* EnvSeed() { return &g_Seed; }
float EnvHeight(float, float) { return 0.0f; }
int EnvLocalPlayer() { return 0; }
bool EnvNoFog() { return true; }
int EnvTeam(int) { return 0; }
bool EnvHasGL() { return false; }
bool EnvCanSee(int, SIUnit*) { return true; }
int EnvFrame() { return g_Tick; }
SIUnit* EnvGetUnit(int) { return nullptr; }
int EnvGLInt(unsigned) { return 0; }
SIPixie* EnvPixie() { return nullptr; }
bool EnvBuildingOccupied(SIUnit*, int) { return false; }

} // namespace

namespace pz {
SUnitAnimEnv g_UnitAnimEnv = { EnvSeed, EnvHeight, EnvLocalPlayer, EnvNoFog, EnvTeam, EnvHasGL,
                               EnvCanSee, EnvFrame, EnvGetUnit, EnvGLInt, EnvPixie, EnvBuildingOccupied,
                               nullptr };
}

namespace {

using namespace pz;

struct SShot {
    const char* File;
    float At;
    bool Done;
};

struct SAnimView {
    SScene* Scene = nullptr;
    SViewport* Viewport = nullptr;
    SIModel* Model = nullptr;
    SIUnitAnimation* Anim = nullptr;
    SPUnitAnimation* Proto = nullptr;
    unsigned char Unit[0x500];
    unsigned char PUnit[0x200];
    unsigned char Gunners[4][0x100];
    unsigned char* GunnerPtrs[4];
    int Program = 0;           // 0 drive, 1 walk, 2 idle
    float Speed = 4.0f;        // m/s
    float Turn = 30.0f;        // deg/s
    float Yaw = 2.2f, Pitch = -0.35f, Dist = 9.0f;
    bool FixedCam = false;
    bool LogTicks = false;
    double Time = 0.0;         // render clock
    double NextTick = 0.0;     // HD MenuNextTick
    int Frames = 0;
    unsigned Seconds = 0;
    SShot Shots[24];
    int ShotCount = 0;
} g_V;

SIUnit* U() { return reinterpret_cast<SIUnit*>(g_V.Unit); }
template <class T> T& F(unsigned off) { return UnitField<T>(U(), off); }

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

// One HD logic tick: SUnit +0x16c (store), the synthetic driver (the
// ServerRefresh part), then +0x3c RefreshModel -> animation +0x08.
void LogicTick()
{
    SAnimView& v = g_V;
    // +0x16c StoreInterpolationState (0x5b5ad0)
    float* pos = &F<float>(kUnitPos);
    float* prev = &F<float>(kUnitPrevPos);
    float* prev2 = &F<float>(0xa4);
    memcpy(prev2, prev, 12);
    memcpy(prev, pos, 12);
    F<float>(0xb8) = F<float>(kUnitPrevDir);
    F<float>(kUnitPrevDir) = F<float>(kUnitDir);
    v.Model->StoreInterpolationState();                       // model +0x3c

    // Driver: speeds of this tick.
    float t = g_Tick * 0.05f;
    float move = 0.0f, turn = 0.0f;
    float spt = v.Speed * 0.05f;
    float tpt = v.Turn * 0.05f * 3.14159265f / 180.0f;
    if (v.Program != 2) {
        if (t < 4.0f) {
            move = spt;
        } else if (t < 8.0f) {
            move = spt;
            turn = tpt;
        } else if (t < 10.0f) {
            turn = -tpt * 1.5f;
        }
    }
    float& dir = F<float>(kUnitDir);
    dir += turn;
    pos[0] += sinf(dir) * move;                               // forward = (sin dir, cos dir)
    pos[2] += cosf(dir) * move;
    F<float>(kUnitMoveSpeed) = move;
    F<float>(kUnitTurnSpeed) = turn;
    F<float>(kUnitSteer) = turn != 0.0f && move != 0.0f ? (turn > 0 ? 0.5f : -0.5f) : 0.0f;
    int mode = 0.0f < move ? 2 : (1e-05f < turn ? 4 : (turn < -1e-05f ? 5 : 1));   // SUnit 0x5c10f0
    F<int>(kUnitMoveMode) = mode;
    // A slow turret sweep for the vehicles with gunners.
    for (int i = 0; i < 4; ++i)
        *reinterpret_cast<float*>(v.Gunners[i] + kGunnerYaw) = 0.4f * sinf(t * 0.7f);

    v.Anim->UpdateModel();                                    // via unit +0x3c
    if (v.LogTicks && (g_Tick % 10) == 0) {
        SModel* m = static_cast<SModel*>(v.Model);
        AV_LOG("tick %4d pos %.2f %.2f dir %.3f mode %d seq %d t %.3f", g_Tick, pos[0], pos[2], dir, mode,
               m->Anim.Seq, m->Anim.Time);
    }
    ++g_Tick;
}

struct SAnimViewWindow : SDXWindow {
    bool Done = false;

    bool OnIdle() override
    {
        if (!::Gepard || Done)
            return false;
        SAnimView& v = g_V;
        const double dt = 1.0 / 60.0;                         // fixed render step
        v.Time += dt;
        // SSuperWindow::OnIdle 0x65ae50: ticks while the next tick is due.
        while (v.NextTick < v.Time) {
            LogicTick();
            v.NextTick += 0.05f;
        }
        v.Scene->AdvanceTime((int)(dt * 1000.0));             // scene +0x1c
        double interp = (v.NextTick - v.Time) * 20.0;          // 0..1, 1 = previous tick
        v.Scene->SetInterpolation(interp);                    // scene +0x20
        float c[3];
        v.Model->GetRenderPosition(c);                        // interpolated
        if (v.FixedCam) {
            c[0] = 6.0f;
            c[1] = 0.0f;
            c[2] = 10.0f;
        }
        c[1] += 1.0f;
        float dirv[3] = { sinf(v.Yaw) * cosf(v.Pitch), sinf(v.Pitch), cosf(v.Yaw) * cosf(v.Pitch) };
        float eye[3];
        for (int i = 0; i < 3; ++i)
            eye[i] = c[i] - dirv[i] * v.Dist;
        v.Viewport->SetCamera(eye[0], eye[1], eye[2], v.Yaw, -v.Pitch);
        v.Viewport->Render(v.Scene, 0);
        ++v.Frames;
        bool pending = false;
        for (int i = 0; i < v.ShotCount; ++i) {
            SShot& s = v.Shots[i];
            if (!s.Done && v.Time >= s.At) {
                s.Done = true;
                bool ok = SaveBackBuffer(s.File);
                SModel* m = static_cast<SModel*>(v.Model);
                AV_LOG("animview: shot %s at %.3f s (tick %d, interp %.2f, seq %d t %.3f) -> %s", s.File, v.Time,
                       g_Tick, interp, m->Anim.Seq, m->Anim.Time, ok ? "ok" : "FAILED");
            }
            pending |= !s.Done;
        }
        if ((v.ShotCount && !pending) || (v.Seconds && v.Time >= v.Seconds)) {
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

void OpenLog(const char* path)
{
    SLogger* log = new SLogger((char*)"animview", (char*)"animview", false, 2);
    FILE* f = fopen(path, "wb");
    if (f)
        fclose(f);
    log->LogFileName = _strdup(path);
    log->LogToFile = true;
    log->Logging = true;
    log->LogLevel = 2;
    Logger.g = log;
}

} // namespace

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    hInstance = hInst;
    SAnimView& v = g_V;
    const char* rundir = nullptr;
    const char* unitFile = nullptr;
    for (int i = 1; i < __argc; ++i) {
        const char* a = __argv[i];
        if (!strcmp(a, "--unit") && i + 1 < __argc) unitFile = __argv[++i];
        else if (!strcmp(a, "--speed") && i + 1 < __argc) v.Speed = (float)atof(__argv[++i]);
        else if (!strcmp(a, "--turn") && i + 1 < __argc) v.Turn = (float)atof(__argv[++i]);
        else if (!strcmp(a, "--yaw") && i + 1 < __argc) v.Yaw = (float)atof(__argv[++i]);
        else if (!strcmp(a, "--pitch") && i + 1 < __argc) v.Pitch = (float)atof(__argv[++i]);
        else if (!strcmp(a, "--dist") && i + 1 < __argc) v.Dist = (float)atof(__argv[++i]);
        else if (!strcmp(a, "--seconds") && i + 1 < __argc) v.Seconds = (unsigned)atoi(__argv[++i]);
        else if (!strcmp(a, "--fixedcam")) v.FixedCam = true;
        else if (!strcmp(a, "--log-ticks")) v.LogTicks = true;
        else if (!strcmp(a, "--program") && i + 1 < __argc) {
            const char* p = __argv[++i];
            v.Program = !strcmp(p, "walk") ? 1 : !strcmp(p, "idle") ? 2 : 0;
        } else if (!strcmp(a, "--shot") && i + 1 < __argc && v.ShotCount < 24) {
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
    strcat(logpath, "\\animview.log");
    OpenLog(logpath);
    if (!unitFile) {
        AV_LOG("animview: usage: animview <rundir> --unit <units/xxx.unit> ...");
        return 2;
    }

    ::SProperties* ini = new ::SProperties("panzers.ini", true);
    const char* search = ini->GetString("Paths", "Search", nullptr);
    if (search)
        FileSystem.SetSearchPath(search, false);
    FileSystem.SetHomePath(ini->GetString("Paths", "home", ""));
    delete ini;

    SAnimViewWindow* wnd = new SAnimViewWindow();
    wnd->SetPosition(40, 40, 1024, 768);
    int rc = wnd->Create((HICON)0, (HCURSOR)IDC_ARROW, L"Panzers animview");
    if (rc != 0 || !::Gepard) {
        AV_LOG("animview: device creation failed (%d)", rc);
        return 3;
    }
    SIGepardHD* g = PzGepard();
    g->SetOption(0x10, 1);
    SIScene* scene = g->CreateScene();
    v.Scene = static_cast<SScene*>(scene);
    const float ambient[4] = { 0.45f, 0.45f, 0.45f, 1.0f };
    const float sun[4] = { 0.85f, 0.83f, 0.78f, 1.0f };
    scene->SetAmbientLight(ambient);
    scene->SetSunLight(sun, 2.4f, -0.75f);
    scene->SetFog(0.62f, 0.66f, 0.70f, 60.0f, 200.0f, 1.0f);

    // The .unit (flat keys) -> stand-in SP*Unit fields and the animation prototype.
    ::SProperties* uf = new ::SProperties(unitFile, false);
    const char* modelName = uf->GetString("Unit", "Unit.Common.ModelName", "");
    int proto = g->LoadModelPrototype(modelName, 0.005f, nullptr, 0);   // Gepard +0x20
    if (proto < 0) {
        AV_LOG("animview: model %s did not load", modelName);
        return 4;
    }
    memset(v.PUnit, 0, sizeof(v.PUnit));
    memset(v.Unit, 0, sizeof(v.Unit));
    SIPUnit* pu = reinterpret_cast<SIPUnit*>(v.PUnit);
    PUnitField<int>(pu, kPUnitClassType) = uf->GetInt("Unit", "Unit.Common.ClassType", 0);
    PUnitField<int>(pu, kPUnitUnitType) = uf->GetInt("Unit", "Unit.Common.UnitType", 0);
    int guns = uf->GetInt("Unit", "Unit.Gunners", 0);
    if (guns > 4)
        guns = 4;
    PUnitField<unsigned char>(pu, kPUnitGunCount) = (unsigned char)guns;
    PUnitField<int>(pu, kPUnitModelProto) = proto;
    HdStrSet(&PUnitField<SHdStr>(pu, kPUnitName), unitFile);
    SAnimProps root = SAnimProps::FromFlat(uf, "Unit");
    v.Proto = LoadPUnitAnimation(pu, root);                   // SPUnit 0x5a6ce0
    if (!v.Proto) {
        AV_LOG("animview: animation type %d not supported", root.GetMultiIndex("Animation"));
        return 5;
    }
    PUnitField<SPUnitAnimation*>(pu, kPUnitAnimation) = v.Proto;
    v.Proto->LoadResourcesProps(pu, root.GetMultiSub("Animation"));   // SPUnit LoadResources -> +0x08

    // The unit (SUnit::Init / InitModel 0x5b7d70 order).
    F<SIPUnit*>(kUnitPUnit) = pu;
    F<int>(kUnitActiveDriver) = -1;
    F<int>(kUnitVehicle) = -1;
    F<float>(kUnitHealth) = 1.0f;
    F<bool>(kUnitAlwaysShown) = true;
    F<float>(kUnitDir) = 0.0f;
    F<float>(kUnitPrevDir) = 0.0f;
    for (int i = 0; i < 4; ++i) {
        memset(v.Gunners[i], 0, sizeof(v.Gunners[i]));
        v.GunnerPtrs[i] = v.Gunners[i];
    }
    F<unsigned char**>(kUnitGunners) = v.GunnerPtrs;
    F<int>(kUnitGunnerCount) = guns;
    v.Anim = v.Proto->CreateAnimation(U());                   // +0x10 (draws the seed twice)
    F<SIUnitAnimation*>(kUnitAnim) = v.Anim;
    v.Model = scene->CreateModelFromPrototype(proto, 1);      // scene +0x58, free heap
    F<SIModel*>(kUnitModel) = v.Model;
    v.Model->SetFlags(3);                                     // SUnit::InitModel
    v.Anim->InitModel(v.Model);                               // anim +0x04
    v.Model->SetPosition(0.0f, 0.0f, 0.0f);
    v.Model->StoreInterpolationState();
    SModel* m = static_cast<SModel*>(v.Model);
    AV_LOG("animview: %s model %s proto %d anim type %d states %d flags 0x%x seq %d", unitFile, modelName, proto,
           root.GetMultiIndex("Animation"), v.Proto->StateCount(), m->Flags, m->Anim.Seq);
    for (int i = 0; i < v.Proto->StateCount(); ++i)
        AV_LOG("  state %d '%s'", i, v.Proto->StateName(i));
    if (SVehicleAnimation* va = dynamic_cast<SVehicleAnimation*>(v.Anim)) {
        if (va->Gear) {
            SRunningGear* gr = va->Gear;
            AV_LOG("  running gear: width %.3f caterpillar %d belts %d/%d wheel types %d", gr->Proto->TrackWidth,
                   (int)gr->Proto->Caterpillar, gr->LeftBelt, gr->RightBelt, gr->Proto->WheelTypeCount);
            for (int i = 0; i < gr->Proto->WheelTypeCount; ++i)
                AV_LOG("    wheel %d type %d 1/r %.4f uv %.3f %.3f nodes %d/%d", i, gr->Proto->WheelTypes[i].Type,
                       gr->Proto->WheelTypes[i].InvRadius, gr->Proto->WheelTypes[i].U, gr->Proto->WheelTypes[i].V,
                       gr->Wheels[i].LeftNode, gr->Wheels[i].RightNode);
        }
        AV_LOG("  body %d antenna %d driver %d", va->BodyNode, va->AntennaNode, va->DriverNode);
    }
    if (v.Program == 1 && v.Speed == 4.0f)
        v.Speed = 1.4f;

    v.Viewport = static_cast<SViewport*>(g->GetViewport(0));
    v.Viewport->SetProjection(60.0f * 3.14159265f / 180.0f, 0.1f, 300.0f);
    wnd->Run();
    AV_LOG("animview: %d frames, %d ticks, %.2f s", v.Frames, g_Tick, v.Time);
    delete v.Anim;
    v.Model->Release();
    delete v.Proto;
    scene->Release();
    g->PurgeModelPrototypes();
    PzGepardShutdown();
    delete wnd;
    return v.Frames > 0 ? 0 : 6;
}
