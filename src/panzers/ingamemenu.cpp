// src/panzers/ingamemenu.cpp
// In-game menus (ingamemenu.h). OWNER: agent H (docs/M3_INTERFACES.md).
// Skeleton: SInGameMenu is a working stand-in (an SRightMenu with the eight
// HD buttons that sends the HD action codes); the others are stubs.

#include "ingamemenu.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "gettext.h"

SInGameMenu::SInGameMenu() {}
SInGameMenu::~SInGameMenu() {}

void SInGameMenu::Create()
{
    STUB_LOG("SInGameMenu::Create (skeleton stand-in)");
    PZ_M3_TRACE("SInGameMenu::Create (skeleton stand-in)");
    SRightMenu::Create(GetText("panzers/GameView.cpp", "Menu"), false);
    const char* texts[8] = {
        GetText("panzers/GameView.cpp", "Save Game"),
        GetText("panzers/GameView.cpp", "Load Game"),
        GetText("panzers/GameView.cpp", "Options"),
        GetText("panzers/GameView.cpp", "Help"),
        GetText("panzers/GameView.cpp", "Objectives"),
        GetText("panzers/GameView.cpp", "Restart Mission"),
        GetText("panzers/GameView.cpp", "End Mission"),
        GetText("panzers/GameView.cpp", "Resume"),
    };
    CreateButtons(Buttons, texts, 8, 0);
    Cursor = 0;
}

// PANZERS 0x631dd0 (single-player button order; the multiplayer order is not lifted)
bool SInGameMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    PZ_M3_TRACE("SInGameMenu::OnAction (0x631dd0)");
    if (action != PZA_BUTTON_CLICK)
        return false;
    static const int kActions[8] = {
        PZA_IGM_SAVE, PZA_IGM_LOAD, PZA_IGM_OPTIONS, PZA_IGM_HELP,
        PZA_IGM_OBJECTIVES, PZA_IGM_RESTART, PZA_IGM_END, PZA_IGM_RESUME,
    };
    for (int i = 0; i < 8; ++i) {
        if (source == &Buttons[i]) {
            SendAction(kActions[i], 0);                  // 0x543930
            return true;
        }
    }
    return true;
}

SHelpMenu::SHelpMenu() {}
SHelpMenu::~SHelpMenu() {}

bool SHelpMenu::OnAction(SWidget* source, int action, int param)
{
    STUB_LOG("SHelpMenu::OnAction (0x631d40)");
    PZ_M3_TRACE("SHelpMenu::OnAction (0x631d40)");
    (void)source; (void)action; (void)param;
    return false;
}

SInGameBriefingMenu::SInGameBriefingMenu() {}
SInGameBriefingMenu::~SInGameBriefingMenu() {}

bool SInGameBriefingMenu::OnAction(SWidget* source, int action, int param)
{
    STUB_LOG("SInGameBriefingMenu::OnAction (0x631d70)");
    PZ_M3_TRACE("SInGameBriefingMenu::OnAction (0x631d70)");
    (void)source; (void)action; (void)param;
    return false;
}

SSaveMenu::SSaveMenu() {}
SSaveMenu::~SSaveMenu() {}

bool SSaveMenu::OnKeyDown(int key, bool repeat)
{
    STUB_LOG("SSaveMenu::OnKeyDown (0x632530)");
    PZ_M3_TRACE("SSaveMenu::OnKeyDown (0x632530)");
    (void)key; (void)repeat;
    return false;
}

void SSaveMenu::OnMouseDown(int button, int x, int y, int shift)
{
    STUB_LOG("SSaveMenu::OnMouseDown (0x632590)");
    PZ_M3_TRACE("SSaveMenu::OnMouseDown (0x632590)");
    SRightMenu::OnMouseDown(button, x, y, shift);
}

bool SSaveMenu::OnAction(SWidget* source, int action, int param)
{
    STUB_LOG("SSaveMenu::OnAction (0x632040)");
    PZ_M3_TRACE("SSaveMenu::OnAction (0x632040)");
    (void)source; (void)action; (void)param;
    return false;
}
