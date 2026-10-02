// src/panzers/credits.h
// SMainCreditMenu — the Credits screen (HD PANZERS.exe).
//
// RTTI .?AVSMainCreditMenu@@ (0x8ec20c), vftable 0x807344, object 0xd4
// (new 0xd4 in SSuperWindow::LoadMainCreditMenu 0x658300). Like the other
// Panzers widgets (mainmenu.h) it derives from the SWINE SDXWidget for
// behaviour; the HD fields are named below with their HD offsets.
//
// HD flow: main menu Credits button -> SMainMenu::OnAction 0x63b6f0 sends
// 0x4d4d6 -> SSuperWindow::OnAction 0x659250 deletes the main menu and calls
// LoadMainCreditMenu 0x658300 -> Create 0x635190 shows menu/Credits_1_hq.tga
// and plays music/credits.mp3. A key, a click or the 60 s timer shows the
// next picture (NextPicture 0x63b560, Credits_2..4); after the 4th the menu
// sends 0x43521 and SSuperWindow deletes it, restarts music/Menu.mp3 and
// reloads the main menu (LoadMainMenu 0x6583e0).
//
// HD reads no credits.ini: the pictures are numbered files, so the
// SProperties section order does not matter here.

#ifndef PANZERS_CREDITS_H
#define PANZERS_CREDITS_H

#include "dxwidget.h"

// SMainCreditMenu -> SSuperWindow (OnAction 0x659250)
enum { PZA_CREDITS_DONE = 0x43521 };

struct SMainCreditMenu : SDXWidget {
    int Picture;    // HD +0x58: number of the picture shown (1..4)
    // HD +0x5c..+0xd0: an embedded 0x74-byte SButton (ctor 0x537a20) spanning
    // 0x400x0x300 with the picture as its glyph, created hidden (vtbl
    // +0x6c(0)); its click 0x42542 would reach OnAction 0x63b6c0. Hidden
    // widgets get no input, so it is not instantiated (as in SAchimMenu).
    int Timer;      // HD +0xd0: SWidget::SetTimer(60000) index, -1 if none

    SMainCreditMenu();                                                // 0x632fd0
    ~SMainCreditMenu() override;                                      // 0x6349f0

    bool OnKeyDown(int key, bool repeat = false) override;            // +0x14 0x63d6b0
    void OnMouseDown(int button, int x, int y, int shift) override;   // +0x24 0x63d930
    bool OnAction(SWidget* source, int action, int param) override;   // +0x44 0x63b6c0
    void OnTimer(int id, unsigned int elapsed) override;              // +0x50 0x63d960

    void Create();                                                    // 0x635190
    void NextPicture();                                               // 0x63b560 (exported)
};

#endif // PANZERS_CREDITS_H
