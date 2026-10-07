// src/panzers/campaignmenu.h
// The New Game screens of the HD main menu (M4 campaign shell):
//
//   SSingleMenu      vftable 0x806fc4, 0x320 bytes (new 800 in SMainMenu::
//                    OnAction 0x63b6f0; ctor 0x633770 over SCenterMenu
//                    0x64bab0, Create 0x639790, OnAction 0x63c7e0, dtor
//                    0x6343e0 / deleting 0x634c90). "New Game": three campaign
//                    pictures (menu/campaign_select_hq.tga), Skirmish and
//                    Scenario. Child of the main menu's parent (the super
//                    window); SMainMenu +0x5c owns it.
//     HD layout: SCenterMenu 0x58, +0x58 dword (0), +0x5c SComplexButton
//     (constructed, not created), +0xd0 Skirmish, +0x144 Scenario,
//     +0x1b8 SSingleDiffMenu*, +0x1bc picture font (-1), +0x1c0 SButton[3]
//     (stride 0x74: German, Russian, Allied), +0x31c Race.
//
//   SSingleDiffMenu  vftable 0x807044, 0x3c8 bytes (new 0x3c8 in 0x63c7e0;
//                    ctor 0x6336d0, Create 0x639540, OnAction 0x63c580, dtor
//                    0x634330 / deleting 0x634c60). "Select difficulty": the
//                    Training Camp dialog's skin with Easy / Normal / Hard,
//                    a description box and Start / Cancel.
//     HD layout: SDXWidget 0x58, SRadioButton[3] +0x58 (stride 0x6c),
//     Start +0x19c, Cancel +0x210, STextBox +0x284, Difficulty +0x3c4
//     (ctor 1).
//
// Start sends 0x53441 to the super window (SSuperWindow::OnAction 0x659250:
// new campaign, Race = SSingleMenu +0x31c, Difficulty = SSingleDiffMenu
// +0x3c4, InitCampaignMode 0x592b20, LoadNextCampaignView); Cancel 0x53442
// deletes the difficulty dialog. OWNER: agent K (M4 campaign shell).

#ifndef PANZERS_CAMPAIGNMENU_H
#define PANZERS_CAMPAIGNMENU_H

#include "optionsmenu.h"
#include "hudwidgets.h"
#include "radiobutton.h"

enum PzCampaignMenuAction {
    PZA_NEWGAME_START     = 0x53441,   // SSingleDiffMenu Start (0x63c580)
    PZA_NEWGAME_CANCEL    = 0x53442,   // SSingleDiffMenu Cancel
    PZA_NEWGAME_SKIRMISH  = 0x534d1,   // SSingleMenu Skirmish (0x63c7e0) -> 0x658e70
};

struct SSingleDiffMenu : SDXWidget {
    SRadioButton   Levels[3];   // HD +0x58 Easy, Normal, Hard
    SComplexButton Start;       // HD +0x19c
    SComplexButton Cancel;      // HD +0x210
    pz::STextBox   Text;        // HD +0x284
    int            Difficulty;  // HD +0x3c4 (ctor 1)

    SSingleDiffMenu();                                               // 0x6336d0
    ~SSingleDiffMenu() override;                                     // 0x634330
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x63c580
    void Create();                                                   // 0x639540
};

struct SSingleMenu : SCenterMenu {
    int              _58;           // HD +0x58 (ctor 0)
    SComplexButton   Unused5c;      // HD +0x5c
    SComplexButton   Skirmish;      // HD +0xd0
    SComplexButton   Scenario;      // HD +0x144
    SSingleDiffMenu* DiffMenu;      // HD +0x1b8
    int              PictureFont;   // HD +0x1bc (ctor -1)
    pz::SButton      Pictures[3];   // HD +0x1c0 German, Russian, Allied
    int              Race;          // HD +0x31c

    SSingleMenu();                                                   // 0x633770
    ~SSingleMenu() override;                                         // 0x6343e0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x63c7e0
    void Create();                                                   // 0x639790
};

#endif // PANZERS_CAMPAIGNMENU_H
