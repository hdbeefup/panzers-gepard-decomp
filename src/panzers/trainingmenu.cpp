// src/panzers/trainingmenu.cpp
// STrainingMenu (trainingmenu.h): the Training Camp "Select nation" dialog.
// OWNER: agent F (docs/M3_INTERFACES.md). Lifted from the HD exe.

#include "trainingmenu.h"
#include "pzboard.h"
#include "m3common.h"
#include "stub_log.h"
#include "gettext.h"

// PANZERS 0x633b30
STrainingMenu::STrainingMenu()
{
    Nation = 1;                                          // param_1[0xa1] = 1
}

// PANZERS 0x634cf0
STrainingMenu::~STrainingMenu()
{
}

// PANZERS 0x63a370
// The message box skin (menu/message_hq.tga) centred on the screen, the
// header sprite (controls glyph 9) with "Select nation", three HD radio
// buttons at (0x1e, 0x4b + 0x23 i), the Russian one checked through
// OnAction, then Start / Cancel side by side at y 0xe6.
void STrainingMenu::Create()
{
    PZ_M3_TRACE("STrainingMenu::Create (0x63a370)");
    SetBackgroundSprite(g_MenuMessageTex, 0, false, false);       // 0x539ba0(DAT_008da794, 0)
    SDXWidget::Create(0);                                         // 0x539a10
    SetPosition((0x400 - Width) / 2, (0x300 - Height) / 2, Width, Height);   // vtbl +0x08
    Cursor = 0;                                                   // 0x543970(0, -1)
    int header = Board->CreateFrame(FT_SPRITE, BackFrame, (Width - 0x124) / 2, 0, 0, 0);   // board +0x08(1, ...)
    Board->SetSpriteGlyph(header, g_MenuControlsFont, 9);         // board +0x24
    int text = Board->CreateFrame(FT_TEXT, header, 0x92, 0xc, 0, 0);
    Board->SetText(text, g_PzFont[PZF_SANS21], 2, GetText("panzers/MainMenu.cpp", "Select nation"));   // board +0x34
    Board->SetTextColor(text, 0xffffff);                          // board +0x28
    for (int i = 0, y = 0x4b; y < 0xb4; ++i, y += 0x23) {
        InsertChild(&Nations[i]);                                 // vtbl +0x54
        Nations[i].SetPosition(0x1e, y, 0, 0);
        Nations[i].CreateHD(0, g_MenuControlsFont);               // 0x53eaf0
    }
    Nations[0].SetText(GetText("panzers/MainMenu.cpp", "German"));    // 0x53ecf0
    Nations[1].SetText(GetText("panzers/MainMenu.cpp", "Russian"));
    Nations[2].SetText(GetText("panzers/MainMenu.cpp", "Allied"));
    OnAction(&Nations[1], PZA_BUTTON_CLICK, 0);                   // vtbl +0x44(+0xc4, 0x42542, 0)
    InsertChild(&Start);
    Start.SetPosition(Width / 2 - 0xc4, 0xe6, 0, 0);
    Start.Create(1, GetText("panzers/MainMenu.cpp", "Start"));    // 0x5386c0(1, text)
    InsertChild(&Cancel);
    Cancel.SetPosition(Width / 2, 0xe6, 0, 0);
    Cancel.Create(1, GetText("panzers/MainMenu.cpp", "Cancel"));
    // HD 0x5439f0 (focus): SuperWindowM3Action does it after Create.
}

// PANZERS 0x63d410
bool STrainingMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    PZ_M3_TRACE("STrainingMenu::OnAction (0x63d410)");
    if (action != PZA_BUTTON_CLICK)                      // 0x42542
        return false;
    int idx = -1;
    for (int i = 0; i < 3; ++i)
        if (source == &Nations[i])
            idx = i;
    if (idx < 0) {
        if (source == &Start)
            SendAction(PZA_TRAINING_START, 0);           // 0x543930(0x544d1, 0)
        else if (source == &Cancel)
            SendAction(PZA_TRAINING_CANCEL, 0);          // 0x543930(0x544d2, 0)
        return true;
    }
    for (int i = 0; i < 3; ++i)                          // 0x53ecd0(i == idx) on each radio button
        Nations[i].SetCheck(i == idx);
    Nation = idx == 0 ? 0 : idx == 1 ? 2 : 1;            // German 0, Russian 2, Allied 1
    return true;
}
