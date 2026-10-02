// src/panzers/mainmenu.cpp
// Panzers main menu: SComplexButton, SRightMenu, SMainMenu, SAchimMenu.
// Lifted from HD PANZERS.exe; see mainmenu.h for the layout notes and
// pzboard.h for the HD-board -> SWINE-board mapping (TEMP until P2-B).

#include <windows.h>
#include <stdio.h>
#include "mainmenu.h"
#include "pzboard.h"
#include "logger.h"
#include "stream.h"
#include "iconcert.h"
#include "stub_log.h"
#include "window.h"

// Logged stubs for the submenus the main menu opens (src/stubs/stub_panzers.cpp).
void PzStub_NewGameMenu(SWidget* parent);
void PzStub_LoadGameMenu(SWidget* parent);
bool PzStub_CheckCDKey(const char* key);

// ---------------------------------------------------------------------------
// SComplexButton
// ---------------------------------------------------------------------------

// PANZERS 0x538640
SComplexButton::SComplexButton()
{
    // Panzers widgets have no tooltips; the SWINE SDXWidget would otherwise
    // load SWINE's menu/*_tooltip.png in Create.
    ToolTipFeatureEnabled = false;
    GlyphBase = 0;
    TextY = 0;
    TextFrame = -1;
    SpriteFrame = -1;     // param_1[0x1b] = -1
    Over = false;         // param_1[0x1c] = 0 (0x70..0x73)
    Pressed = false;
    Stuck = false;
    _73 = 0;
}

// PANZERS 0x538680
SComplexButton::~SComplexButton()
{
    if (Text.buf) {
        delete[] Text.buf;
        Text.buf = nullptr;
    }
    // SDXWidget::~SDXWidget (0x539930) runs next and destroys BackFrame.
}

// PANZERS 0x5386c0
void SComplexButton::Create(int style, const char* text)
{
    if (style == 0) {
        GlyphBase = 6;
        TextY = 4;
    } else {
        TextY = 0xb;
        GlyphBase = (style == 1) ? 3 : 0;
    }
    SDXWidget::Create(0);                                          // 0x539a10
    // HD board +0x08 CreateFrame(1 = sprite, BackFrame, 0, 0, 0, 0)
    SpriteFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 0);
    // HD board +0x24 SetSprite(frame, controls, GlyphBase)
    Board->SetSpriteGlyph(SpriteFrame, g_MenuControlsFont, GlyphBase);
    int w = 0, h = 0;
    Board->GetFrameSize(SpriteFrame, &w, &h);                      // HD +0x20
    Resize(w, h);                                                  // vtbl +0x10
    // HD: CreateFrame(2 = text, SpriteFrame, Width/2, TextY, 0, 1)
    TextFrame = Board->CreateFrame(FT_TEXT, SpriteFrame, Width / 2, TextY, 0, 1);
    Text = text ? text : "";                                       // 0x52c320
    // HD board +0x34 SetText(frame, font 3, 2, text, 0)
    Board->SetText(TextFrame, g_PzFont[PZF_SANS21_SHADOW], 2, Text.buf ? Text.buf : "");
    Update();                                                      // vtbl +0x78, twice in HD
    Update();
}

// PANZERS 0x538820
void SComplexButton::OnMouseDown(int button, int x, int y, int shift)
{
    SDXWidget::OnMouseDown(button, x, y, shift);                   // 0x539b10
    if (button == 1) {
        if (Concert)
            Concert->PlaySound("menu/button_down.wav", -12.0f, 0, -1);   // 0xc1400000
        Pressed = true;
        CaptureMouse();                                            // 0x543300
        Update();
        SendAction(PZA_BUTTON_DOWN, 0);
        return;
    }
    if (button == 3)
        SendAction(PZA_BUTTON_RCLICK, 0);
}

// PANZERS 0x538970
void SComplexButton::OnMouseUp(int button, int x, int y, int shift)
{
    (void)shift;
    if (Pressed && button == 1) {
        Over = x >= 0 && x < Width && y >= 0 && y < Height;
        Pressed = false;
        ReleaseMouse();                                            // 0x5437c0
        Update();
        if (Over)
            SendAction(PZA_BUTTON_CLICK, 0);
    }
}

// PANZERS 0x5388b0
void SComplexButton::OnMouseMove(int x, int y, int shift)
{
    (void)shift;
    Over = x >= 0 && x < Width && y >= 0 && y < Height;
    Update();
}

// PANZERS 0x538920
void SComplexButton::OnMouseOver()
{
    if (Concert)
        Concert->PlaySound("menu/button_over.wav", -30.0f, 0, -1);      // 0xc1f00000
    Over = true;
    Update();
    SendAction(PZA_BUTTON_OVER, 0);
}

// PANZERS 0x5388f0
void SComplexButton::OnMouseOut()
{
    Over = false;
    Update();
    SDXWidget::OnMouseOut();                                       // 0x539b30
    SendAction(PZA_BUTTON_OUT, 0);
}

// PANZERS 0x538a60
void SComplexButton::Update()
{
    if (BackFrame < 0)
        return;
    int glyph;
    if (!Enabled) {
        Board->SetTextColor(TextFrame, 0x666666);                  // HD +0x28
        Board->MoveFrame(TextFrame, Width / 2, TextY);             // HD +0x10
        Board->SetSpriteGlyph(SpriteFrame, g_MenuControlsFont, GlyphBase);
        return;
    }
    if ((Pressed && Over) || Stuck) {
        Board->SetTextColor(TextFrame, 0xffffff);
        Board->MoveFrame(TextFrame, Width / 2 + 1, TextY + 1);
        glyph = GlyphBase + 2;
    } else if (!Over && !Pressed) {
        Board->SetTextColor(TextFrame, 0xd0d0d0);
        Board->MoveFrame(TextFrame, Width / 2, TextY);
        Board->SetSpriteGlyph(SpriteFrame, g_MenuControlsFont, GlyphBase);
        return;
    } else {
        Board->SetTextColor(TextFrame, 0xffffff);
        Board->MoveFrame(TextFrame, Width / 2, TextY);
        glyph = GlyphBase + 1;
    }
    Board->SetSpriteGlyph(SpriteFrame, g_MenuControlsFont, glyph);
}

// ---------------------------------------------------------------------------
// SRightMenu
// ---------------------------------------------------------------------------

// PANZERS 0x64bb30
SRightMenu::SRightMenu()
{
    ToolTipFeatureEnabled = false;
}

// PANZERS 0x64bc50
SRightMenu::~SRightMenu()
{
}

// PANZERS 0x64bdf0
void SRightMenu::Create(const char* title, bool rightAligned)
{
    SetBackgroundSprite(g_MenuButtonsTex, 0, false, false);        // 0x539ba0
    SDXWidget::Create(0);                                          // 0x539a10
    if (rightAligned)
        SetPosition(0x3f8 - Width, 0x96, Width, Height);           // vtbl +0x08
    else
        SetPosition((0x400 - Width) / 2, 0x5f, Width, Height);
    int header = Board->CreateFrame(FT_SPRITE, BackFrame, (Width - 0x124) / 2, 0, 0, 0);
    Board->SetSpriteGlyph(header, g_MenuControlsFont, 9);
    int text = Board->CreateFrame(FT_TEXT, header, 0x92, 0xc, 0, 0);
    Board->SetText(text, g_PzFont[PZF_SANS21], 2, title ? title : "");      // HD +0x34 (f, 2, 2, t, 0)
    Board->SetTextColor(text, 0xffffff);                           // HD +0x28
}

// PANZERS 0x64c210
void SRightMenu::CreateButtons(SComplexButton* buttons, const char* const* texts,
                               int count, unsigned char firstRow)
{
    for (int i = 0; i < count; ++i) {
        SComplexButton* b = &buttons[i];
        InsertChild(b);                                            // vtbl +0x54
        int y = (i < count - 1) ? ((int)firstRow + i) * 0x31 + 0x32 : 0x189;
        b->SetPosition((Width - 0x100) / 2, y, 0, 0);              // vtbl +0x08
        b->Create(2, texts[i]);
    }
}

// ---------------------------------------------------------------------------
// SMainMenu
// ---------------------------------------------------------------------------

// PANZERS 0x633030
SMainMenu::SMainMenu()
{
    LoadGameMenu = nullptr;    // param_1[0x16]
    NewGameMenu = nullptr;     // param_1[0x17]
}

// PANZERS 0x633cd0
SMainMenu::~SMainMenu()
{
    if (LoadGameMenu) {
        delete LoadGameMenu;
        LoadGameMenu = nullptr;
    }
    if (NewGameMenu) {
        delete NewGameMenu;
        NewGameMenu = nullptr;
    }
    // HD then destroys the 3 SMessageBox, the SInputDialog and Buttons[8]
    // (eh_vector_destructor_iterator stride 0x74); here Buttons[] are members.
}

// PANZERS 0x6352f0
void SMainMenu::Create()
{
    SRightMenu::Create(GetText("panzers/MainMenu.cpp", "Main Menu"), true);
    const char* texts[8];
    texts[0] = GetText("panzers/MainMenu.cpp", "New Game");
    texts[1] = GetText("panzers/MainMenu.cpp", "Load Game");
    texts[2] = GetText("panzers/MainMenu.cpp", "Multiplayer");
    texts[3] = GetText("panzers/MainMenu.cpp", "Tutorial");
    texts[4] = GetText("panzers/MainMenu.cpp", "Training Camp");
    texts[5] = GetText("panzers/MainMenu.cpp", "Options");
    texts[6] = GetText("panzers/MainMenu.cpp", "Credits");
    texts[7] = GetText("panzers/MainMenu.cpp", "Exit");
    CreateButtons(Buttons, texts, 8, 0);
    // 0x5439f0: walk up the parents making this the focused child.
    for (SWidget* w = this; w->Enabled && w->Visible && w->Parent; w = w->Parent) {
        if (w->Parent->Focus != w) {
            for (SWidget* c = w->Parent->Child; c; c = c->Sibling)
                if (c->FocusSibling == w)
                    c->FocusSibling = nullptr;
            w->FocusSibling = w->Parent->Focus;
            w->Parent->Focus = w;
        }
    }
    Cursor = 0;                                                  // 0x543970 (SWidget +0x3c; SWINE SetCursor has no body)(0, -1)
    // 0x5435b0 + 0x545110: find the owning window and re-send the last mouse
    // position so the button under the cursor highlights immediately.
    SWindow* wnd = GetWindowParent();
    if (wnd)
        wnd->UpdateMouse();
}

// PANZERS 0x63b6f0
bool SMainMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action == 0x494c3) {
        // A submenu (New Game / Load Game) closed itself.
        if (LoadGameMenu) { delete LoadGameMenu; LoadGameMenu = nullptr; }
        if (NewGameMenu) { delete NewGameMenu; NewGameMenu = nullptr; }
        return true;
    }
    if (action != PZA_BUTTON_CLICK) {
        // HD: CD-key dialog (+0x400) Ok/Cancel (0x49581/0x49582) and the
        // CD-key error box (+0x818) / "exit" box (+0xff0). The CD-key dialog
        // is not instantiated (SSettings::CHECKCDKEY is a stub), so only the
        // exit box path remains reachable in HD terms; keep its effect.
        return false;
    }
    if (source == &Buttons[0]) {                     // +0x60 New Game
        if (LoadGameMenu) { delete LoadGameMenu; LoadGameMenu = nullptr; }
        if (NewGameMenu) { delete NewGameMenu; NewGameMenu = nullptr; }
        PzStub_NewGameMenu(Parent);                  // new 0x320 0x633770 + 0x639790
    } else if (source == &Buttons[1]) {              // +0xd4 Load Game
        if (LoadGameMenu) { delete LoadGameMenu; LoadGameMenu = nullptr; }
        if (NewGameMenu) { delete NewGameMenu; NewGameMenu = nullptr; }
        PzStub_LoadGameMenu(Parent);                 // new 0x280 0x62cbc0 + 0x62f950(1)
    } else if (source == &Buttons[2]) {              // +0x148 Multiplayer
        SendAction(PZA_MAIN_MULTIPLAYER, 0);
    } else if (source == &Buttons[3]) {              // +0x1bc Tutorial
        SendAction(PZA_MAIN_TUTORIAL, 0);
    } else if (source == &Buttons[4]) {              // +0x230 Training Camp
        SendAction(PZA_MAIN_TRAINING, 0);
    } else if (source == &Buttons[5]) {              // +0x2a4 Options
        SendAction(PZA_MAIN_OPTIONS, 0);
    } else if (source == &Buttons[6]) {              // +0x318 Credits
        SendAction(PZA_MAIN_CREDITS, 0);
    } else if (source == &Buttons[7]) {              // +0x38c Exit
        SendAction(PZA_MAIN_EXIT, 0);
    }
    return true;
}

// ---------------------------------------------------------------------------
// SAchimMenu (quit screens menu/quit_%d_hq.tga)
// ---------------------------------------------------------------------------

// PANZERS 0x632ee0
SAchimMenu::SAchimMenu()
{
    ToolTipFeatureEnabled = false;
    Quit = false;
    ImageFrame = -1;
    Image = 0;
}

SAchimMenu::~SAchimMenu()
{
}

static bool PzFileExists(const char* name)
{
    // HD: FUN_0065e720 (SFileSystem::FindFiles) and a count > 0 test.
    return FileSystem.Stat(name, nullptr) == 0;
}

// Shared body of HD 0x63d510 (OnKeyDown, vtbl +0x14) and 0x63d790
// (OnMouseDown, vtbl +0x24): advance to the next quit image, or quit.
static void AchimAdvance(SAchimMenu* m)
{
    if (!m->Quit) {
        m->SendAction(PZA_ACHIM_BACK, 0);
        return;
    }
    ++m->Image;
    char name[64];
    sprintf(name, "menu/quit_%d_hq.tga", m->Image);
    if (!PzFileExists(name)) {
        m->SendAction(PZA_ACHIM_QUIT, 0);
        return;
    }
    int tex = PzLoadTexture(name);
    m->SetBackgroundSprite(tex, 0, false, false);                  // 0x539ba0
    int f = Board->CreateFrame(FT_SPRITE, m->BackFrame, 0, 0, 0, 1);
    Board->SetSpriteGlyph(f, tex, 0);
    PzReleaseTexture(tex);
}

// PANZERS 0x63d510
bool SAchimMenu::OnKeyDown(int key, bool repeat)
{
    (void)key; (void)repeat;
    AchimAdvance(this);
    return true;
}

// PANZERS 0x63d790 is the HD OnMouseDown; the recompile advances on button
// release so the press that opened the window does not also dismiss it.
void SAchimMenu::OnMouseUp(int button, int x, int y, int shift)
{
    (void)button; (void)x; (void)y; (void)shift;
    AchimAdvance(this);
}

// PANZERS 0x634d60
void SAchimMenu::Create(bool quit)
{
    Quit = quit;
    int tex;
    if (!quit) {
        tex = PzLoadTexture("menu/splash_hq.tga");
    } else {
        Image = 1;
        if (!PzFileExists("menu/quit_1_hq.tga")) {
            SendAction(PZA_ACHIM_QUIT, 0);
            return;
        }
        tex = PzLoadTexture("menu/quit_1_hq.tga");
    }
    SDXWidget::Create(0);
    int f = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 0);
    Board->SetSpriteGlyph(f, tex, 0);
    Board->ShowFrame(f, true);
    // HD: an SButton child (+0x5c) spanning 0x400x0x300 with the same
    // texture (0x537a80) and a text frame at (0x200, 0x2e2); the button's
    // click (0x42542) reaches OnAction 0x63b640 -> 0x414d1. Not needed for the
    // quit flow, which is driven by the key/mouse handlers above.
    if (tex >= 0)
        PzReleaseTexture(tex);
    Cursor = 0;                                                    // 0x543970(0, -1)
}
