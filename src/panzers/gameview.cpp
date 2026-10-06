// src/panzers/gameview.cpp
// SGameView skeleton (gameview.h). OWNERS: agent O (input handlers, the order
// dispatcher), agent F (LoadMap, MissionStart). The view itself (ctor,
// Create, Update, camera, panel modes, callbacks, OnAction) is agent V's
// gameview_view.cpp. docs/M3_INTERFACES.md.
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

// SGameView::LoadMap 0x6201c0 and MissionStart 0x6281a0: gameview_mission.cpp (agent F).

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

void SGameView::IssueOrder(int p1, int p2, int p3, int p4, int p5)
{
    STUB_LOG("SGameView::IssueOrder (0x61e740)");
    PZ_M3_TRACE("SGameView::IssueOrder (0x61e740)");
    (void)p1; (void)p2; (void)p3; (void)p4; (void)p5;
}
