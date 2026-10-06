// src/panzers/gameview.cpp
// SGameView skeleton (gameview.h). OWNERS: agent V (view, camera, Create,
// Update, draw), agent O (input handlers, the order dispatcher), agent F
// (LoadMap, MissionStart). docs/M3_INTERFACES.md.
//
// What the skeleton does (all behind -m3, see superwindow_m3.cpp):
//   LoadMap: maps/training.map into a new pz::SWorld with the M1/M2 loader
//     (PZ_M3_LOADMAP=0 skips it), a menu-style SGameLogic(0, -1, 0), then
//     the loading screen; a click sends 0x47561 "Map loaded".
//   MissionStart: the scene goes to the window, SetRunning(1).
//   Update: the SSuperWindow::OnIdle world tick (20 Hz Refresh, camera,
//     interpolation), which HD does in SGameView::Update 0x628430.
//   Esc opens the SInGameMenu stand-in; End Mission sends GV_GAMEOVER.

#include <windows.h>
#include <string.h>
#include "gameview.h"
#include "ingamemenu.h"
#include "superwindow.h"
#include "pzboard.h"
#include "m3common.h"
#include "campaign.h"
#include "stub_log.h"
#include "logger.h"
#include "timer.h"
#include "stream.h"
#include "iconcert.h"
#include "gettext.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/iviewport.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"

SGameView::SGameView()
{
    STUB_LOG("SGameView::SGameView (0x6181f0)");
    PZ_M3_TRACE("SGameView::SGameView (0x6181f0)");
    ToolTipFeatureEnabled = false;
    memset(static_cast<SGameViewData*>(this), 0, sizeof(SGameViewData));
    LoadingFrame = -1;
    SoundHandle3894 = -1;
    SoundHandle3898 = -1;
}

SGameView::~SGameView()
{
    STUB_LOG("SGameView::~SGameView (0x619430)");
    PZ_M3_TRACE("SGameView::~SGameView (0x619430)");
    if (InGameMenu) {
        delete InGameMenu;
        InGameMenu = nullptr;
    }
    if (LoadingFrame >= 0 && Board) {
        Board->DestroyFrame(LoadingFrame);
        LoadingFrame = -1;
    }
    // HD: the view owns the mission SGameLogic (+0x3e44) and SWorld (+0x3e40).
    if (Logic) {
        delete Logic;
        Logic = nullptr;
    }
    if (World) {
        delete World;                                              // releases g_Scene
        World = nullptr;
    }
}

void SGameView::Create()
{
    STUB_LOG("SGameView::Create (0x619c90)");
    PZ_M3_TRACE("SGameView::Create (0x619c90)");
    SDXWidget::Create(0);
}

void SGameView::LoadMap()
{
    STUB_LOG("SGameView::LoadMap (0x6201c0)");
    PZ_M3_TRACE("SGameView::LoadMap (0x6201c0)");
    const char* map = pz::g_Campaign ? pz::g_Campaign->GetMapName() : "maps/harc_teszt.map";
    Logger.g->Log(0, "Loading map: %s", map);
    if (pz::g_M3.LoadMap) {
        SStream* stream = FileSystem.OpenRead(map, nullptr);       // 0x65f420(name, 0)
        if (!stream)
            Logger.g->Panic("Can't load map: %s", map);
        World = new pz::SWorld(0);                                 // new 0x7538, 0x5d2f90(0)
        World->ShowLoadingIcon(GetFrame());                        // 0x5edca0(+0x48)
        if (!World->LoadMap(stream, true, 0, 0))                   // 0x5f1990(stream, 1, 0, 0)
            Logger.g->Panic("Can't load map: %s", map);
        World->Initialize();                                       // 0x5eec90
        World->LoadMapExtra_5e2d70();
        World->LoadMapExtra_5debb0();
        World->LoadMapExtra_607ad0();
        stream->Release();
        if (pz::g_Campaign)
            pz::g_Campaign->LoadObjectives();                      // 0x593ba0 (0x593740 / 0x594140 not declared)
        World->UpdateWaterMap();                                   // 0x608600
        World->FixBridges();                                       // 0x5e65f0
        // HD: new SGameLogic(0, +0x828, this +0x58 callback); the skeleton
        // uses the menu arguments until agent F lifts the mission ctor path.
        Logic = new pz::SGameLogic(0, -1, 0);                      // new 0x318, 0x55e440
        World->HideLoadingIcon();                                  // 0x5dc7d0
        pz::LogWorldStats("SGameView::LoadMap");
    } else {
        Logger.g->Log(0, "PZM3: PZ_M3_LOADMAP=0, map not loaded");
    }
    // HD: campaign +0xdc == 0, -packetplay, or no market -> MissionStart at
    // once; else the "Click to continue" loading screen (+0x3890).
    LoadingScreen = true;
    LoadingFrame = Board->CreateFrame(FT_TEXT, GetFrame(), 0x200, 0x2e0, 0, 1);
    Board->SetText(LoadingFrame, g_PzFont[PZF_SANS14], 2, GetText("panzers/GameView.cpp", "Click to continue"));
    Board->ShowFrame(LoadingFrame, true);
    Cursor = 0;
}

void SGameView::MissionStart()
{
    STUB_LOG("SGameView::MissionStart (0x6281a0)");
    PZ_M3_TRACE("SGameView::MissionStart (0x6281a0)");
    // HD order: Concert +0x80(1); the window scene = g_Scene; CreateSubViewports
    // 0x61e500; release +0x3898 / +0x3894; clocks +0x45c/+0x460; PlaceAllUnits
    // 0x571c70; SWorld 0x5f5b50; SetRunning(1) unless campaign +0xe4;
    // 0x57f970; -packetrec / -packetplay; InitCameraSpline 0x609760;
    // SetPanelMode(0); SaveGameStartMission("Start") unless multiplayer;
    // -skipframes ticks; one Refresh.
    if (pz::g_WindowScene)
        pz::g_WindowScene->Release();
    if (pz::g_Scene)
        pz::g_Scene->AddRef();
    pz::g_WindowScene = pz::g_Scene;
    CreateSubViewports();
    if (World)
        World->StartEffects();
    if (Logic && pz::g_Campaign && pz::g_Campaign->GetStartPaused() == 0)
        Logic->SetRunning(1);                                      // 0x5802f0(1)
    if (World)
        World->InitCameraSpline("");
    SetPanelMode(0);
    if (pz::g_Campaign)
        pz::g_Campaign->SaveGameStartMission("Start", "Start");
    NextTick = (double)Timer.GetTickValue() / 1000.0;
}

void SGameView::Update()
{
    PZ_M3_TRACE("SGameView::Update (0x628430)");
    if (!World || !Logic || !pz::g_World || LoadingScreen)
        return;
    // Skeleton: the SSuperWindow::OnIdle menu-world tick (HD 0x628430 has
    // its own ms clock at +0x45c/+0x460 and adds 50 ms per logic frame).
    double now = (double)Timer.GetTickValue() / 1000.0;
    while (NextTick < now) {
        Logic->Refresh();                                          // 0x576d80
        NextTick += 0.05;
    }
    pz::SIViewport* vp = pz::PzGepard()->GetViewport(0);
    World->ComputeCamera(vp);
    double interpolation = (NextTick - now) * 20.0;
    if (pz::g_Scene)
        pz::g_Scene->SetInterpolation(interpolation);
    Logic->UpdateUnitVisuals(vp, interpolation);
}

void SGameView::OpenInGameMenu()
{
    if (InGameMenu)
        return;
    InGameMenu = new SInGameMenu();
    InsertChild(InGameMenu);
    InGameMenu->Create();
}

bool SGameView::OnKeyDown(int key, bool repeat)
{
    STUB_LOG("SGameView::OnKeyDown (0x622f50)");
    PZ_M3_TRACE("SGameView::OnKeyDown (0x622f50)");
    (void)repeat;
    if (LoadingScreen) {
        // HD 0x622fec: a key on the loading screen also sends "Map loaded".
        LoadingScreen = false;
        if (LoadingFrame >= 0) { Board->DestroyFrame(LoadingFrame); LoadingFrame = -1; }
        SendAction(PZA_GV_MAP_LOADED, 0);
        return true;
    }
    if (key == VK_ESCAPE)
        OpenInGameMenu();
    return true;
}

bool SGameView::OnKeyUp(int key)
{
    STUB_LOG("SGameView::OnKeyUp (0x6246d0)");
    PZ_M3_TRACE("SGameView::OnKeyUp (0x6246d0)");
    (void)key;
    return true;
}

void SGameView::OnMouseDown(int button, int x, int y, int shift)
{
    STUB_LOG("SGameView::OnMouseDown (0x624a70)");
    PZ_M3_TRACE("SGameView::OnMouseDown (0x624a70)");
    (void)button; (void)x; (void)y; (void)shift;
    if (LoadingScreen) {
        // HD 0x624ac6: +0x3890 set -> SendAction(0x47561).
        LoadingScreen = false;
        if (LoadingFrame >= 0) { Board->DestroyFrame(LoadingFrame); LoadingFrame = -1; }
        SendAction(PZA_GV_MAP_LOADED, 0);
    }
}

void SGameView::OnMouseUp(int button, int x, int y, int shift)
{
    STUB_LOG("SGameView::OnMouseUp (0x6251f0)");
    PZ_M3_TRACE("SGameView::OnMouseUp (0x6251f0)");
    (void)button; (void)x; (void)y; (void)shift;
}

void SGameView::OnMouseMove(int x, int y, int shift)
{
    STUB_LOG("SGameView::OnMouseMove (0x6250e0)");
    PZ_M3_TRACE("SGameView::OnMouseMove (0x6250e0)");
    (void)x; (void)y; (void)shift;
}

void SGameView::OnMouseWheel(int button, int x, int y, int delta)
{
    STUB_LOG("SGameView::OnMouseWheel (0x625510)");
    PZ_M3_TRACE("SGameView::OnMouseWheel (0x625510)");
    (void)button; (void)x; (void)y; (void)delta;
}

bool SGameView::OnAction(SWidget* source, int action, int param)
{
    STUB_LOG("SGameView::OnAction (0x6216b0)");
    PZ_M3_TRACE("SGameView::OnAction (0x6216b0)");
    (void)source; (void)param;
    switch (action) {
    case PZA_IGM_RESUME:
        if (InGameMenu) {
            delete InGameMenu;                     // ~SWidget unlinks it (as SSuperWindow does with its menus)
            InGameMenu = nullptr;
        }
        return true;
    case PZA_IGM_END:
        // HD: "End Mission" / "Are you sure?" (SMessageBox), Yes -> 0x47562
        // (0x622b02). The stand-in confirms at once.
        Logger.g->Log(0, "PZM3: End Mission (confirm box not lifted) -> GV_GAMEOVER");
        SendAction(PZA_GV_GAMEOVER, 0);
        return true;
    case PZA_BUTTON_DOWN: case PZA_BUTTON_CLICK: case PZA_BUTTON_OVER:
    case PZA_BUTTON_RCLICK: case PZA_BUTTON_OUT:
        return true;
    default:
        // Not handled here: SendAction (which starts at the sender itself)
        // passes it on to SSuperWindow (0x47561, 0x47562, ...).
        return false;
    }
}

void SGameView::ShowMessageBox(const char* text)
{
    STUB_LOG("SGameView::ShowMessageBox (0x622b30)");
    PZ_M3_TRACE("SGameView::ShowMessageBox (0x622b30)");
    Logger.g->Log(0, "PZM3: message box: %s", text ? text : "");
}

void SGameView::Slot_04(int p1, int p2)
{
    STUB_LOG("SGameView callback +0x04 (0x622c50)");
    PZ_M3_TRACE("SGameView callback +0x04 (0x622c50)");
    (void)p1; (void)p2;
}

void SGameView::Slot_08(int p1)
{
    STUB_LOG("SGameView callback +0x08 (0x625560)");
    PZ_M3_TRACE("SGameView callback +0x08 (0x625560)");
    (void)p1;
}

void SGameView::ResetClock()
{
    STUB_LOG("SGameView::ResetClock (0x624730)");
    PZ_M3_TRACE("SGameView::ResetClock (0x624730)");
}

void SGameView::Slot_10(int p1)
{
    STUB_LOG("SGameView callback +0x10 (0x624770)");
    PZ_M3_TRACE("SGameView callback +0x10 (0x624770)");
    (void)p1;
}

void SGameView::CreateSubViewports()
{
    STUB_LOG("SGameView::CreateSubViewports (0x61e500)");
    PZ_M3_TRACE("SGameView::CreateSubViewports (0x61e500)");
}

void SGameView::DestroySubViewports()
{
    STUB_LOG("SGameView::DestroySubViewports (0x61e680)");
    PZ_M3_TRACE("SGameView::DestroySubViewports (0x61e680)");
}

void SGameView::SetPanelMode(int mode)
{
    STUB_LOG("SGameView::SetPanelMode (0x625d80)");
    PZ_M3_TRACE("SGameView::SetPanelMode (0x625d80)");
    (void)mode;
}

void SGameView::IssueOrder(int p1, int p2, int p3, int p4, int p5)
{
    STUB_LOG("SGameView::IssueOrder (0x61e740)");
    PZ_M3_TRACE("SGameView::IssueOrder (0x61e740)");
    (void)p1; (void)p2; (void)p3; (void)p4; (void)p5;
}
