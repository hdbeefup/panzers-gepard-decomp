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
    PzOpenInGameMenu(this);                                        // H: 0x620080 (pauses, ingamemenu.cpp)
}

