// src/panzers/tutorial.cpp
// The Tutorial button of the main menu (M4 agent T): SSuperWindow::OnAction
// 0x659250, case 0x4d4d3. The rest of the path is the Training Camp one:
// LoadNextCampaignView 0x658b10 (MenuToLoad 2: the game view, no market),
// SGameView::LoadMap, "Map loaded" (0x47561) -> mission start; the first
// trigger plays the cut-scene tutorial-01 (src/game/cutscene.cpp).

#include <windows.h>
#include "superwindow.h"
#include "mainmenu.h"
#include "campaign.h"
#include "logger.h"
#include "m3common.h"

void M3LoadNextCampaignView(SSuperWindow* sw);   // superwindow_m3.cpp 0x658b10

// PANZERS 0x659250 (case 0x4d4d3, 0x659a3e..)
bool PzTutorialAction(SSuperWindow* sw, int action)
{
    if (action != PZA_MAIN_TUTORIAL)
        return false;
    PZ_M3_TRACE("SSuperWindow::OnAction Tutorial (0x659250 / 0x4d4d3)");
    if (sw->MainMenu) {                                            // +0xe8: scalar deleting dtor (1)
        delete sw->MainMenu;
        sw->MainMenu = nullptr;
    }
    // 0x543970(0, -1): the window cursor back to the arrow (the recompile's
    // menu cursor is already the arrow here).
    delete pz::g_Campaign;
    pz::g_Campaign = new pz::SPanzersCampaign();                   // new 0xb8c, 0x590ec0
    pz::g_Campaign->InitTutorialMode("maps/tutorial.map", 1, 0);   // 0x594ab0
    Logger.g->Log(0, "PZM4: Tutorial");
    M3LoadNextCampaignView(sw);                                    // 0x658b10
    return true;
}
