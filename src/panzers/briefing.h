// src/panzers/briefing.h
// SBriefingMenu: the campaign mission's diary page before the map loads
// (SSuperWindow::LoadNextCampaignView 0x658b10 case 0, MenuToLoad 0).
//
//   vftable 0x8072c4, 0x340 bytes (new 0x340; ctor 0x632f40, Create
//   0x634f20, deleting dtor 0x634940): +0x14 OnKeyDown 0x63d690 (any key:
//   0x424d1), +0x44 OnAction 0x63b670 (the picture button pressed: stop the
//   speech, 0x424d1), +0x50 0x63d950 (empty).
//   HD layout: SDXWidget 0x58, +0x58 speech sound (-1), +0x5c diary texture
//   (-1), +0x60 SButton (0x74) spanning the screen, +0xd4 / +0x1e8 two text
//   lists (0x53bdc0; never inserted: the "Briefing text" goes into +0xd4 but
//   is not shown), +0x33c (1).
//
// The page is menu/diary/<map>_hq.tga (menu/diary/ger-01_hq.tga when it is
// missing) with "Click to continue"; speech/diary/<map>.mp3 plays (0x63e050).
// 0x424d1 -> SSuperWindow: delete the menu, MenuToLoad 2, LoadNextCampaignView
// (the game view loads the map). 0x424d2 (no sender in HD) -> main menu.
// OWNER: agent K (M4 campaign shell).

#ifndef PANZERS_BRIEFING_H
#define PANZERS_BRIEFING_H

#include "hudwidgets.h"

enum PzBriefingAction {
    PZA_BRIEFING_DONE   = 0x424d1,
    PZA_BRIEFING_CANCEL = 0x424d2,
};

struct SBriefingMenu : SDXWidget {
    int         Speech;      // HD +0x58 (-1)
    int         Texture;     // HD +0x5c (-1)
    pz::SButton Picture;     // HD +0x60
    int         _33c;        // HD +0x33c

    SBriefingMenu();                                                 // 0x632f40
    ~SBriefingMenu() override;                                       // 0x634940
    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x63d690
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x63b670
    void Create();                                                   // 0x634f20
};

#endif // PANZERS_BRIEFING_H
