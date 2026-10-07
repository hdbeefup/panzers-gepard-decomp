// src/panzers/savemenu.h
// The in-game Save Game screen (HD PANZERS.exe):
//
//   SSaveMenu   vftable 0x805e84, 0x474 bytes (new in SGameView::OpenSaveMenu
//               0x6205f0, ctor 0x62ccb0 over SCenterMenu 0x64bab0, dtor
//               0x62d3f0 / deleting 0x62d7a0), Create 0x62fcb0; OnKeyDown
//               0x632530, OnMouseDown 0x632590, OnAction 0x632040; Save
//               0x632a50, Deselect 0x630f20.
//               Delete / Save / Back (0x64bed0 at x 3, 199, 0x18b), the
//               "Games" caption, the save list (SSaveLoadListBox, 0x12 rows)
//               with "Empty slot" first and the saves newest first, and an
//               edit box (SEditBox, a child of the list) that opens over the
//               title column of the selected row: "Empty slot" proposes the
//               campaign's name (0x5925d0) and saves to SaveGames/<time>.save,
//               a save proposes its title and is overwritten (no question).
//               Save sends 0x49531 (SGameView deletes the menu and resumes),
//               Back 0x49533 (the in-game menu again).
//   pz::SEditBox vftable 0x7f2b3c, 0x180 bytes (ctor 0x53a6c0, Create
//               0x53a750): Panzers' one-line ANSI edit box (char[0x100] at
//               +0x69; not SWINE's wide-char SEditBox in src/window).
//
// OWNER: agent M5-SV (docs/m5/sv.md).

#ifndef PANZERS_SAVEMENU_H
#define PANZERS_SAVEMENU_H

#include "loadgame_menu.h"

struct SGameView;

enum PzSaveMenuAction {
    PZA_SAVE_DONE = 0x49531,   // SSaveMenu saved (0x632a50)
    PZA_SAVE_BACK = 0x49533,   // SSaveMenu Back
};

namespace pz {

// HD SEditBox (0x180 bytes).
struct SEditBox : SDXWidget {
    int  Font;              // +0x58
    int  TextFrame;         // +0x5c
    int  CaretFrame;        // +0x60
    int  CaretTimer;        // +0x64
    bool CaretVisible;      // +0x68
    char Text[0x100];       // +0x69 (+0x168 stays 0)
    int  CaretPos;          // +0x16c
    bool Uppercase;         // +0x170
    int  LeadByte;          // +0x174 DBCS lead byte (0x64dd70 only)
    char DbcsBuf[6];        // +0x178
    bool Password;          // +0x17e

    SEditBox();                                                      // 0x53a6c0
    ~SEditBox() override;                                            // 0x53a720

    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x53aad0
    bool OnChar(int ch) override;                                    // +0x1c 0x53a8c0
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x53adc0
    void OnTimer(int id, unsigned int elapsed) override;             // +0x50 0x53adf0
    void Update() override;                                          // +0x78 0x53ae80

    void Create(int font, bool frame);                               // 0x53a750
    const char* GetText() const { return Text; }                     // 0x53a8b0
    void SetText(const char* text);                                  // 0x53ae10
};

} // namespace pz

// HD SSaveMenu (0x474 bytes).
struct SSaveMenu : SCenterMenu {
    SComplexButton       SaveButton;    // +0x58 "Save"
    SComplexButton       DeleteButton;  // +0xcc "Delete"
    SComplexButton       BackButton;    // +0x140 "Back"
    pz::SSaveLoadListBox List;          // +0x1b4
    pz::SEditBox         Edit;          // +0x2ec
    int                  _46c;          // +0x46c (-1)

    SSaveMenu();                                                     // 0x62ccb0
    ~SSaveMenu() override;                                           // 0x62d3f0 (deleting 0x62d7a0)
    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x632530
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x632590
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x632040
    void Create();                                                   // 0x62fcb0
    void Save();                                                     // 0x632a50
    void Deselect();                                                 // 0x630f20

private:
    void FillList();                                                 // part of 0x62fcb0 / 0x632040
};

// SGameView::OpenSaveMenu 0x6205f0: the menu into the view's slot +0x3e4c.
void PzOpenSaveMenu(SGameView* view);

#endif // PANZERS_SAVEMENU_H
