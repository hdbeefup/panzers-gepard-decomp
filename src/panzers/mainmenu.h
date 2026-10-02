// src/panzers/mainmenu.h
// Panzers main menu widgets (HD PANZERS.exe).
//
// Class names come from RTTI. As for SSuperWindow, the HD SWidget/SDXWidget
// layouts differ from the SWINE ones in src/window (see superwindow.h), so
// these classes derive from the SWINE SDXWidget for behaviour and keep their
// own HD fields in a separate size-checked block:
//   SComplexButton  vftable 0x7f2774, object 0x74 (eh_vector_constructor stride
//                   0x74 at 0x633030; delete size 0x74 in 0x538680)
//   SRightMenu      vftable 0x808e54, object 0x58 (delete size in 0x64bc50)
//   SMainMenu       vftable 0x806ec4, object 0x13dc (new 0x13dc at 0x6583e0)
//   SAchimMenu      vftable 0x807544, object 0xd4 (new 0xd4 in OnAction 0x4d4d8)
// The HD SDXWidget is 0x58 bytes (ctor 0x5398f0 writes up to +0x54), so the
// Panzers-own part of SComplexButton is 0x74 - 0x58 = 0x1c bytes.

#ifndef PANZERS_MAINMENU_H
#define PANZERS_MAINMENU_H

#include <stddef.h>
#include "dxwidget.h"
#include "string2.h"

// Action codes (SWidget::SendAction 0x543930 / OnAction vtbl +0x44).
enum PzAction {
    PZA_BUTTON_DOWN   = 0x42541,
    PZA_BUTTON_CLICK  = 0x42542,
    PZA_BUTTON_OVER   = 0x42543,
    PZA_BUTTON_RCLICK = 0x42544,
    PZA_BUTTON_OUT    = 0x42545,
    // SMainMenu -> SSuperWindow (OnAction 0x63b6f0 -> 0x659250)
    PZA_MAIN_NEWGAME_DONE = 0x4d4d1,
    PZA_MAIN_MULTIPLAYER  = 0x4d4d2,
    PZA_MAIN_TUTORIAL     = 0x4d4d3,
    PZA_MAIN_TRAINING     = 0x4d4d4,
    PZA_MAIN_OPTIONS      = 0x4d4d5,
    PZA_MAIN_CREDITS      = 0x4d4d6,
    PZA_MAIN_EXIT         = 0x4d4d8,
    PZA_MAIN_ALLIED2      = 0x4d4d9,
    // SAchimMenu -> SSuperWindow
    PZA_ACHIM_BACK        = 0x414d1,
    PZA_ACHIM_QUIT        = 0x414d2,
};

struct SComplexButtonData {
    int     GlyphBase;     // 0x58 controls glyph of the normal state (+1 over, +2 pressed)
    SString Text;          // 0x5c
    int     TextY;         // 0x64
    int     TextFrame;     // 0x68
    int     SpriteFrame;   // 0x6c
    bool    Over;          // 0x70
    bool    Pressed;       // 0x71
    bool    Stuck;         // 0x72
    unsigned char _73;
};
static_assert(sizeof(SComplexButtonData) == 0x74 - 0x58, "SComplexButton is 0x74 bytes in HD");

struct SComplexButton : SDXWidget, SComplexButtonData {
    SComplexButton();                                   // 0x538640
    ~SComplexButton() override;                         // 0x538680

    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x538820
    void OnMouseUp(int button, int x, int y, int shift) override;    // +0x28 0x538970
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x5388b0
    void OnMouseOver() override;                                     // +0x34 0x538920
    void OnMouseOut() override;                                      // +0x38 0x5388f0
    void Update() override;                                          // +0x78 0x538a60

    void Create(int style, const char* text);           // 0x5386c0
};

struct SRightMenu : SDXWidget {
    SRightMenu();                                        // 0x64bb30
    ~SRightMenu() override;                              // 0x64bc50

    void Create(const char* title, bool rightAligned);   // 0x64bdf0
    void CreateButtons(SComplexButton* buttons, const char* const* texts,
                       int count, unsigned char firstRow); // 0x64c210
};

struct SMainMenuData {
    SWidget* LoadGameMenu;   // 0x58 (button +0xd4, new 0x280 0x62cbc0)
    SWidget* NewGameMenu;    // 0x5c (button +0x60, new 0x320 0x633770)
};

struct SMainMenu : SRightMenu, SMainMenuData {
    // HD order: Buttons[8] at 0x60 (stride 0x74), then SInputDialog CD-key
    // dialog at 0x400 (0x418 bytes, 0x53b3a0) and three SMessageBox at 0x818,
    // 0xc04, 0xff0 (0x3ec bytes each, 0x53e0d0); 0xff0 + 0x3ec = 0x13dc. The
    // CD-key dialog and message boxes only serve SSettings::CHECKCDKEY, which
    // is a logged stub, so they are not instantiated here.
    SComplexButton Buttons[8];   // New Game, Load Game, Multiplayer, Tutorial,
                                 // Training Camp, Options, Credits, Exit

    SMainMenu();                                         // 0x633030
    ~SMainMenu() override;                               // 0x633cd0 (deleting 0x634a70)

    bool OnAction(SWidget* source, int action, int param) override; // +0x44 0x63b6f0

    void Create();                                       // 0x6352f0
};
static_assert(0x60 + 8 * 0x74 == 0x400 && 0xff0 + 0x3ec == 0x13dc, "SMainMenu HD layout (new 0x13dc)");

// Quit / "achievement" splash (menu/quit_1_hq.tga). The HD Exit button goes
// SMainMenu -> 0x4d4d8 -> SSuperWindow creates this and calls Create(true).
struct SAchimMenu : SDXWidget {
    bool Quit;           // HD +0xd0 (param_1[0x34] = param_2)
    int  ImageFrame;
    int  Image;

    SAchimMenu();                                        // 0x632ee0
    ~SAchimMenu() override;

    void OnMouseUp(int button, int x, int y, int shift) override;
    bool OnKeyDown(int key, bool repeat = false) override;

    void Create(bool quit);                              // 0x634d60
};

#endif // PANZERS_MAINMENU_H
