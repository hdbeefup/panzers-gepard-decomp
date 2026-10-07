// src/panzers/ingamemenu.h
// In-game menus: SInGameMenu (Esc), SHelpMenu, SInGameBriefingMenu
// (objectives), SSaveMenu, SLoadMenu. HD addresses.
//
//   SInGameMenu          vftable 0x805e04, 0x3f8 bytes (ctor 0x62cb50 over SRightMenu
//                        0x64bb30): +0x00 dtor 0x62d670 (1) [m3: end],
//                        +0x44 OnAction 0x631dd0 (3) [m3: end]; Create 0x62f690.
//                        8 SComplexButton at +0x58 (stride 0x74): Save Game, Load Game,
//                        Options, Help, Objectives, Restart Mission, End Mission, Resume.
//                        Clicks send PzInGameMenuAction to the parent (SGameView OnAction
//                        0x6216b0), which asks "Are you sure?" (SMessageBox) before
//                        End / Restart and then sends GV_GAMEOVER 0x47562 / 0x47565.
//                        In multiplayer (0x594d20) the buttons are Options, Help,
//                        Objectives, End, Resume.
//   SSaveMenu            vftable 0x805e84, 0x474 bytes over SCenterMenu (savemenu.h,
//                        savemenu.cpp; SGameView::OpenSaveMenu 0x6205f0 here)
//   SLoadMenu            vftable 0x805f04: dtor 0x62d6f0, OnKeyDown 0x632500,
//                        OnAction 0x631f40 (main-menu Load Game: new 0x280 0x62cbc0,
//                        Create 0x62f950; LoadReplayNames 0x5959d0 is dead code)
//   SHelpMenu            vftable 0x805f84, 0x20c bytes (ctor 0x62ca60 over SCenterMenu):
//                        dtor 0x62d5d0, OnAction 0x631d40; Create 0x62ec90 (help.txt)
//   SInGameBriefingMenu  vftable 0x806004, 0x284 bytes (ctor 0x62cad0 over SCenterMenu):
//                        dtor 0x62d640 [m3: play], OnAction 0x631d70 [m3: play];
//                        Create 0x62eda0 (the objectives screen)
//
// SGameView side (HD SGameView members; their bodies live here, agent H, as
// agreed with the coordinator): PzOpenInGameMenu 0x620080, PzOpenHelpMenu
// 0x61ff30, PzOpenObjectivesMenu 0x61ffb0, and PzGameViewMenuAction = the
// in-game-menu cases of SGameView::OnAction 0x6216b0 (V calls it first).
// The menus sit in the HD SGameView slots +0x3e48 (in-game menu; the
// recompile keeps it in SGameView::InGameMenu), +0x3e54 help, +0x3e58
// objectives, +0x3e8c / +0x3e90 the Restart / End "Are you sure?" boxes;
// +0x3888 (byte) remembers whether the game was paused when a menu opened.
//
// SHARED HEADER (owner P0; names and bodies: agent H, src/panzers/ingamemenu.cpp).

#ifndef PANZERS_INGAMEMENU_H
#define PANZERS_INGAMEMENU_H

#include "mainmenu.h"
#include "optionsmenu.h"
#include "hudwidgets.h"
#include "savemenu.h"
#include "loadgame_menu.h"

struct SGameView;

enum PzInGameMenuAction {
    PZA_IGM_SAVE       = 0x494d1,
    PZA_IGM_LOAD       = 0x494d2,
    PZA_IGM_OPTIONS    = 0x494d3,
    PZA_IGM_HELP       = 0x494d4,
    PZA_IGM_OBJECTIVES = 0x494d6,
    PZA_IGM_RESTART    = 0x494d7,
    PZA_IGM_END        = 0x494d8,
    PZA_IGM_RESUME     = 0x494d9,
    PZA_HELP_BACK      = 0x49481,      // SHelpMenu Back (0x631d40)
    PZA_OBJ_BACK       = 0x4947421,    // SInGameBriefingMenu Back; param = opened from the briefing
    PZA_OBJ_BRIEFING   = 0x4947422,    // SInGameBriefingMenu Briefing
};

struct SInGameMenu : SRightMenu {
    SComplexButton Buttons[8];   // HD +0x58, stride 0x74

    SInGameMenu();                                                   // 0x62cb50
    ~SInGameMenu() override;                                         // 0x62d670
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x631dd0
    void Create();                                                   // 0x62f690
};

struct SHelpMenu : SCenterMenu {
    SComplexButton BackButton;   // +0x58
    pz::STextBox   Text;         // +0xcc

    SHelpMenu();                                                     // 0x62ca60
    ~SHelpMenu() override;                                           // 0x62d5d0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x631d40
    void Create();                                                   // 0x62ec90
};

struct SInGameBriefingMenu : SCenterMenu {
    SComplexButton BriefingButton;  // +0x58
    SComplexButton BackButton;      // +0xcc
    pz::STextBox   Text;            // +0x140
    bool           FromBriefing;    // +0x280 (0x61ffb0 param)

    SInGameBriefingMenu();                                           // 0x62cad0
    ~SInGameBriefingMenu() override;                                 // 0x62d240 (deleting 0x62d640)
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x631d70
    void Create();                                                   // 0x62eda0
    void AddObjective(int index);                                    // 0x632bd0
};

// SSaveMenu (over SCenterMenu): savemenu.h.

// SGameView helpers (HD SGameView members, see above).
void PzOpenInGameMenu(SGameView* view);                              // 0x620080
void PzOpenHelpMenu(SGameView* view);                                // 0x61ff30
void PzOpenObjectivesMenu(SGameView* view, bool fromBriefing);       // 0x61ffb0
// The in-game-menu cases of SGameView::OnAction 0x6216b0. Returns true when
// the action was handled (the caller then returns true).
bool PzGameViewMenuAction(SGameView* view, SWidget* source, int action, int param);
// Deletes the menus and boxes above (the view's dtor must call it before
// SWidget::~SWidget, which panics while children are left).
void PzGameViewDeleteMenus(SGameView* view);
// The Esc key of SGameView::OnKeyDown 0x622f50 (the dialog part): closes the
// in-game menu (and runs again), else the help / objectives / save menus
// and opens the in-game menu.
void PzGameViewEscape(SGameView* view);
// SGameView::OpenLoadMenu 0x620140: SLoadMenu (Load + Back) into +0x3e50.
void PzOpenLoadMenu(SGameView* view);
// The dialog part of SGameView's load-game LoadMap 0x61f840.
void PzGameViewCloseDialogs(SGameView* view);

#endif // PANZERS_INGAMEMENU_H
