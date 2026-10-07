// src/panzers/loadgame_menu.h
// The Load Game screen: SLoadMenu (vftable 0x805f04, 0x280 bytes, ctor
// 0x62cbc0 over SCenterMenu, Create 0x62f950) and its list box
// SSaveLoadListBox (vftable 0x7f3324, SGenericListBox<SSaveLoadListBoxItem>,
// ctor 0x53eef0). OWNER: agent S (M4).

#ifndef PANZERS_LOADGAME_MENU_H
#define PANZERS_LOADGAME_MENU_H

#include <windows.h>
#include "mainmenu.h"
#include "optionsmenu.h"
#include "pzwidgets.h"

namespace pz {

// HD SSaveLoadListBoxItem (0x34 bytes).
struct SSaveLoadListBoxItem {
    SString      Code;      // +0x00 mission code
    SString      Title;     // +0x08
    SString      Date;      // +0x10 "<date> <time>"
    SString      File;      // +0x18 file name in SaveGames/
    SYSTEMTIME   Time;      // +0x20 (sort key)
    unsigned int Color;     // +0x30
};

// HD SSaveLoadListBox: the SGenericListBox fields as pz::SListBox (0x58..
// 0x11f), the items, the font, and three text columns.
struct SSaveLoadListBox : SDXWidget {
    int  LineHeight;        // 0x58
    int  VisibleLines;      // 0x5c
    int  TopIndex;          // 0x60
    int  ColumnWidth;       // 0x64
    int  Columns;           // 0x68
    bool Selectable;        // 0x6c
    int  CurSel;            // 0x70
    int  Hover;             // 0x74
    float LastClickTime;    // 0x78
    bool HasScrollbar;      // 0x7c
    pz::SScrollbar Scrollbar; // 0x80
    SSaveLoadListBoxItem* Items; // 0x114
    int  ItemCount;         // 0x118
    int  ItemMax;           // 0x11c
    int  Font;              // 0x120
    int* CodeFrames;        // 0x124
    int* TitleFrames;       // 0x128
    int* DateFrames;        // 0x12c
    int* BoxFrames;         // 0x130
    int  TitleWidth;        // 0x134

    SSaveLoadListBox();                                              // 0x53eef0
    ~SSaveLoadListBox() override;                                    // 0x53f110

    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x53fc10
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x53fce0
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x53fe60
    void OnMouseWheel(int delta, int x, int y, int keys) override;   // +0x30 0x53fef0
    void OnMouseOut() override;                                      // +0x38 0x53fed0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x53fb60
    void Update() override;                                          // +0x78 0x540200

    void Create(int font, int lines, int codeW, int gap1, int titleRight, int gap2,
                int dateExtra, bool selectable);                     // 0x53f4c0
    int  AddItem(const char* code, const char* title, const char* date, const char* file,
                 const SYSTEMTIME& t, unsigned int color);           // 0x53f1b0
    void Sort(int from);                                             // 0x53ff80 newest first (0x53f390)
    void ResetContent();                                             // 0x53ffc0
    const char* GetFileName(int index) const;                        // 0x53f9c0
    void GetText(int index, SString* code, SString* title, SString* date) const;   // 0x53fa70
    void EnsureVisible(int index);                                   // 0x53f8f0
    int  SetCurSel(int index);
    int  SetTopIndex(int index);

private:
    void FreeItems();
};

} // namespace pz (SWINE has its own SSaveLoadListBox in window/)

// HD SLoadMenu (0x280 bytes).
struct SLoadMenu : SCenterMenu {
    SComplexButton  LoadButton;   // +0x58 "Load"
    SComplexButton  BackButton;   // +0xcc "Back" (in-game only)
    pz::SSaveLoadListBox List;    // +0x140
    int             Picture;      // +0x278 (-1)
    int             PictureFrame; // +0x27c
    bool            FromMainMenu = false;

    SLoadMenu();                                                     // 0x62cbc0
    ~SLoadMenu() override;                                           // 0x62d310 (deleting 0x62d6f0)
    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x632500
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x631f40
    void Create(bool fromMainMenu);                                  // 0x62f950
    void LoadSelected();                                             // 0x630f80
};

// The main menu's Load Game button (SMainMenu::OnAction 0x63b6f0): new
// SLoadMenu into the super window, Create(1).
SWidget* PzCreateLoadMenu(SWidget* parent);

#endif // PANZERS_LOADGAME_MENU_H
