// src/panzers/ingamemenu.h
// In-game menus: SInGameMenu (Esc), SHelpMenu, SInGameBriefingMenu
// (objectives), SSaveMenu, SLoadMenu. HD addresses.
//
//   SInGameMenu          vftable 0x805e04: +0x00 dtor 0x62d670 (1) [m3: end],
//                        +0x44 OnAction 0x631dd0 (3) [m3: end]
//                        8 SComplexButton at +0x58 (stride 0x74): Save Game, Load Game,
//                        Options, Help, Objectives, Restart Mission, End Mission, Resume.
//                        Clicks send PzInGameMenuAction to the parent (SGameView OnAction
//                        0x6216b0), which asks "Are you sure?" (SMessageBox) before
//                        End / Restart and then sends GV_GAMEOVER 0x47562 / 0x47565.
//                        In multiplayer (0x594d20) the first buttons are Options, Help,
//                        Objectives, End, Resume.
//   SSaveMenu            vftable 0x805e84: dtor 0x62d7a0, OnKeyDown 0x632530,
//                        OnMouseDown 0x632590, OnAction 0x632040
//   SLoadMenu            vftable 0x805f04: dtor 0x62d6f0, OnKeyDown 0x632500,
//                        OnAction 0x631f40 (main-menu Load Game: new 0x280 0x62cbc0,
//                        Create 0x62f950; LoadReplayNames 0x5959d0 is dead code)
//   SHelpMenu            vftable 0x805f84: dtor 0x62d5d0, OnAction 0x631d40
//   SInGameBriefingMenu  vftable 0x806004: dtor 0x62d640 [m3: play], OnAction 0x631d70 [m3: play]
//
// SHARED HEADER (owner P0; names and bodies: agent H, src/panzers/ingamemenu.cpp).

#ifndef PANZERS_INGAMEMENU_H
#define PANZERS_INGAMEMENU_H

#include "mainmenu.h"

enum PzInGameMenuAction {
    PZA_IGM_SAVE       = 0x494d1,
    PZA_IGM_LOAD       = 0x494d2,
    PZA_IGM_OPTIONS    = 0x494d3,
    PZA_IGM_HELP       = 0x494d4,
    PZA_IGM_OBJECTIVES = 0x494d6,
    PZA_IGM_RESTART    = 0x494d7,
    PZA_IGM_END        = 0x494d8,
    PZA_IGM_RESUME     = 0x494d9,
};

struct SInGameMenu : SRightMenu {
    SComplexButton Buttons[8];   // HD +0x58, stride 0x74

    SInGameMenu();
    ~SInGameMenu() override;                                         // 0x62d670
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x631dd0
    void Create();                                                   // (agent H: find the HD Create)
};

struct SHelpMenu : SRightMenu {
    SHelpMenu();
    ~SHelpMenu() override;                                           // 0x62d5d0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x631d40
};

struct SInGameBriefingMenu : SRightMenu {
    SInGameBriefingMenu();
    ~SInGameBriefingMenu() override;                                 // 0x62d640
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x631d70
};

struct SSaveMenu : SRightMenu {
    SSaveMenu();
    ~SSaveMenu() override;                                           // 0x62d7a0
    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x632530
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x632590
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x632040
};

#endif // PANZERS_INGAMEMENU_H
