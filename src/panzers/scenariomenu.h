// src/panzers/scenariomenu.h
// Scenario mode (agent M5-SC):
//
//   SScenarioMenu    vftable 0x8073c4, 0x20c bytes (new 0x20c in
//                    SSingleMenu::LoadScenarioMenu 0x63b2d0; ctor inline over
//                    SCenterMenu 0x64bab0, Create 0x639000, OnAction
//                    0x63c450, deleting dtor 0x634bf0). "Scenario": the
//                    "Maps" list of scenarios/*.map and one "Load" button.
//                    Child of the New Game menu's parent (the super window),
//                    drawn over the New Game menu; SSingleMenu +0x58 owns it.
//     HD layout: SCenterMenu 0x58, SListBox +0x58 (0x134), SComplexButton
//     Load +0x18c, SDArray<SString> Files +0x200 ("scenarios/<name>.map").
//
// Load (or a double click) makes a new campaign whose MapName (+0x20) is
// the selected file and sends 0x53431 to the super window (SSuperWindow::
// OnAction 0x659250): main menu deleted, a new campaign in scenario mode
// (InitScenarioMode 0x5944d0 (map, "", 0, 0)), LoadNextCampaignView (game
// view), the campaign's race = the map's local player. Play 0x65b470 takes
// the same way for "-map <path>" (0x6585e0 = LoadNextCampaignView case 2).

#ifndef PANZERS_SCENARIOMENU_H
#define PANZERS_SCENARIOMENU_H

#include "optionsmenu.h"
#include "pzwidgets.h"
#include "darray.h"
#include "string2.h"

struct SSuperWindow;

enum PzScenarioAction {
    PZA_SCENARIO_START = 0x53431,   // SScenarioMenu Load / double click, param = list index
};

struct SScenarioMenu : SCenterMenu {
    pz::SListBox     List;          // HD +0x58 (ctor 0x53bdc0)
    SComplexButton   Load;          // HD +0x18c (ctor 0x538640)
    SDArray<SString> Files;         // HD +0x200 {array, size, max} (recompile member order differs)

    SScenarioMenu();                                                 // inline in 0x63b2d0
    ~SScenarioMenu() override;                                       // 0x634bf0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x63c450
    void Create();                                                   // 0x639000
};

// SSuperWindow::OnAction 0x659250 case 0x53431 (called from the -m3 action
// dispatcher, superwindow_m3.cpp). Returns true when handled.
bool PzScenarioAction(SSuperWindow* sw, int action, int param);

// SSuperWindow::Play 0x65b470, the "-map" / bare map path branch.
void PzStartMapFromCommandLine(SSuperWindow* sw, const char* map);

#endif // PANZERS_SCENARIOMENU_H
