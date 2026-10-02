// src/panzers/credits.cpp
// SMainCreditMenu — the Credits screen (HD PANZERS.exe). See credits.h.

#include <windows.h>
#include <stdio.h>
#include "credits.h"
#include "mainmenu.h"
#include "pzboard.h"
#include "logger.h"
#include "milesconcert.h"

// PANZERS 0x632fd0
SMainCreditMenu::SMainCreditMenu()
{
    ToolTipFeatureEnabled = false;   // Panzers widgets have no tooltips (see SComplexButton)
    Picture = 0;
    Timer = -1;                      // param_1[0x34] = -1
}

// PANZERS 0x6349f0
SMainCreditMenu::~SMainCreditMenu()
{
    KillTimer(&Timer);               // 0x543690(this + 0xd0)
    // HD then destroys the embedded SButton (+0x5c) and the SDXWidget base.
}

// PANZERS 0x63b560 (exported ?NextPicture@SMainCreditMenu@@AAEXXZ)
void SMainCreditMenu::NextPicture()
{
    ++Picture;
    char name[64];
    _snprintf(name, sizeof(name) - 1, "menu/Credits_%d_hq.tga", Picture);   // Format 0x51ee20
    name[sizeof(name) - 1] = 0;
    int tex = PzLoadTexture(name);                                 // board +0x7c
    SetBackgroundSprite(tex, 0, false, false);                     // 0x539ba0 (+0x4c/+0x50)
    // HD adds a new sprite on top each time and keeps the old ones; they go
    // with the widget's back frame.
    int f = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 1);  // board +0x08(1, +0x48, 0,0,0, 1)
    Board->SetSpriteGlyph(f, tex, 0);                              // board +0x24
    Board->ShowFrame(f, true);   // SWINE CreateFrame leaves new frames hidden
    PzReleaseTexture(tex);                                         // board +0x80
}

// Shared tail of 0x63d6b0 / 0x63d930 / 0x63d960: after the 4th picture the
// screen asks SSuperWindow to go back to the main menu.
static void CreditsAdvance(SMainCreditMenu* m)
{
    if (m->Picture > 3)
        m->SendAction(PZA_CREDITS_DONE, 0);                        // 0x543930(0x43521, 0)
    else
        m->NextPicture();
}

// PANZERS 0x63d6b0 (vtbl +0x14). HD: any key advances (Esc too), and only
// a key after the 4th picture returns to the main menu.
bool SMainCreditMenu::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    // Recompile-only (user request): Esc leaves the credits at once. HD
    // treats Esc like any other key.
    if (key == VK_ESCAPE) {
        SendAction(PZA_CREDITS_DONE, 0);
        return true;
    }
    CreditsAdvance(this);
    return true;
}

// PANZERS 0x63d930 (vtbl +0x24)
void SMainCreditMenu::OnMouseDown(int button, int x, int y, int shift)
{
    (void)button; (void)x; (void)y; (void)shift;
    CreditsAdvance(this);
}

// PANZERS 0x63b6c0 (vtbl +0x44): the hidden full-screen button's click.
bool SMainCreditMenu::OnAction(SWidget* source, int action, int param)
{
    (void)source; (void)param;
    if (action == PZA_BUTTON_CLICK) {                              // 0x42542
        SendAction(PZA_CREDITS_DONE, 0);
        return true;
    }
    return false;
}

// PANZERS 0x63d960 (vtbl +0x50): the 60 s timer from Create.
void SMainCreditMenu::OnTimer(int id, unsigned int elapsed)
{
    (void)elapsed;
    if (id == Timer)
        CreditsAdvance(this);
}

// PANZERS 0x635190
void SMainCreditMenu::Create()
{
    SDXWidget::Create(0);                                          // 0x539a10
    if (SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert)) {
        pc->ClearPlaylist();                                       // Concert +0x6c
        pc->AddToPlaylist("music/credits.mp3");                    // +0x70
        pc->StartPlaylist(true);                                   // +0x78(1)
    }
    Picture = 1;                                                   // +0x58
    int tex = PzLoadTexture("menu/Credits_1_hq.tga");              // Format("menu/Credits_%d_hq.tga", 1)
    int f = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 1);
    Board->SetSpriteGlyph(f, tex, 0);
    Board->ShowFrame(f, true);   // SWINE CreateFrame leaves new frames hidden (cf. SAchimMenu::Create)
    // HD: embedded SButton +0x5c: InsertChild, SetPosition(0,0,0x400,0x300),
    // 0x537a80(tex, 0,0,-1,-1), Resize(0x400,0x300), SetVisible(0). Not
    // instantiated (credits.h).
    PzReleaseTexture(tex);
    Timer = SetTimer(60000);                                       // 0x543b10(60000) -> +0xd0
    SetFocus();                                                    // 0x5439f0: keys reach OnKeyDown
}
