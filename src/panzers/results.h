// src/panzers/results.h
// The end of a mission: the victory / defeat / error box of SGameView::Update
// 0x628430 (single-player tail 0x62bc2f..0x62c233) and the results menu
// SStatisticMenu. HD addresses.
//
//   SStatisticMenu  vftable 0x806084, 700 (0x2bc) bytes, ctor 0x62cd50 over
//                   SCenterMenu 0x64bab0: +0x00 dtor 0x62d7d0 (0x62d4a0),
//                   +0x14 OnKeyDown 0x632570, +0x44 OnAction 0x6324d0;
//                   Create 0x62ffa0. +0x58 SComplexButton Ok, +0xcc STextBox,
//                   +0x20c medal picture, +0x264 losses stamp (SDXWidget).
//                   Ok / Enter send PZA_STAT_OK to the game view, whose
//                   OnAction 0x6216b0 sends GV_GAMEOVER 0x47562.
//
// Which one HD shows (0x628430): in tutorial mode (Training Camp and the
// Tutorial, campaign GameMode 5, 0x594d40) the box; in the other single-player
// modes the results menu (after the "Autosaving..." save "End").
// OWNER: M3-P.

#ifndef PANZERS_RESULTS_H
#define PANZERS_RESULTS_H

#include "optionsmenu.h"
#include "hudwidgets.h"

struct SGameView;

enum PzStatisticMenuAction {
    PZA_STAT_OK = 0x4953541,    // SStatisticMenu Ok / Enter (0x6324d0, 0x632570)
};

struct SStatisticMenu : SCenterMenu {
    SComplexButton OkButton;    // +0x58
    pz::STextBox   Text;        // +0xcc
    SDXWidget      Medal;       // +0x20c
    SDXWidget      Stamp;       // +0x264

    SStatisticMenu();                                                // 0x62cd50
    ~SStatisticMenu() override;                                      // 0x62d4a0 (deleting 0x62d7d0)
    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x632570
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x6324d0
    void Create();                                                   // 0x62ffa0
};

// SResultsMenu: the campaign debriefing after a mission (SSuperWindow::
// LoadNextCampaignView 0x658b10 case 3, MenuToLoad 3 from LetMapDone).
//   vftable 0x8074c4, 0x1228 bytes (new 0x1228; ctor 0x633540, Create
//   0x6360f0, dtor 0x6341d0 / deleting 0x634bc0), +0x44 OnAction 0x63c230.
//   HD layout: SDXWidget 0x58, +0x58 SButton[12] (stride 0x74, the pages:
//   0x42541 on one checks it and shows its page, 0x63e690), +0x6a8 Save
//   (SSaveMenu 0xbac 0x633820(1) into +0x10e4), +0x71c Continue (0x524d1),
//   +0x790 Restart (0x524d2), +0x804 Cancel (0x524d3), the text widgets
//   after them, +0x10e4 the save menu.
//   OWNER: agent K (M4 campaign shell).
enum PzResultsAction {
    PZA_RESULTS_CONTINUE = 0x524d1,   // -> LetResultsDone 0x594e70, LoadNextCampaignView
    PZA_RESULTS_RESTART  = 0x524d2,   // (also 0x47565) -> 0x594f60, LoadNextCampaignView
    PZA_RESULTS_MAINMENU = 0x524d3,   // -> LoadMainMenu
};

struct SResultsMenu : SDXWidget {
    pz::SButton    Pages[12];   // HD +0x58
    SComplexButton Save;        // HD +0x6a8
    SComplexButton Continue;    // HD +0x71c
    SComplexButton Restart;     // HD +0x790
    SComplexButton Cancel;      // HD +0x804
    int            Grid[8][7];  // HD +0x5c8 the unit table's text frames (row stride 0x1c)
    SDXWidget      Medals[20];  // HD +0x9ac (stride 0x58)
    SDXWidget      NewMedal;    // HD +0x108c
    SWidget*       SaveMenu;    // HD +0x10e4
    pz::STextBox   Upload;      // HD +0x10e8 "this score is not uploadable"
    int            Background;  // (recompile) menu/debriefing_menu_hq.tga custom font

    void ShowPage(int page);                                         // 0x63e690

    SResultsMenu();                                                  // 0x633540
    ~SResultsMenu() override;                                        // 0x6341d0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x63c230
    void Create();                                                   // 0x6360f0
};

// The UI part of the 0x628430 end check, after the logic part (backup of the
// army, logic stopped) in PzGameViewEndCheck: result 1 = victory, 3 = the
// "Inconsistency" error, out = the local player is out (defeat).
void PzShowMissionEnd(SGameView* view, int result, bool out);

// Recompile test switch PZ_M3_FORCE_END=1 (victory) | 2 (local player out) |
// 3 (error): 10 s after the first end check, the end check sees that result;
// 11 / 12 also take the non-tutorial path (the results menu).
// Unset (the default) it changes nothing.
void PzForcedMissionEnd(int* result, bool* out);

// Deletes the results menu (+0x3e6c) with the view's other menus.
void PzDeleteStatisticMenu(SGameView* view);

#endif // PANZERS_RESULTS_H
