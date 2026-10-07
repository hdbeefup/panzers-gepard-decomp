// src/panzers/campaignmenu.cpp
// The New Game screens (campaignmenu.h): SSingleMenu (campaign pictures)
// and SSingleDiffMenu (difficulty). OWNER: agent K (M4 campaign shell).
// Lifted from the HD exe.

#include "campaignmenu.h"
#include "pzboard.h"
#include "window.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "gettext.h"

static const char* Mm(const char* id) { return GetText("panzers/MainMenu.cpp", id); }

// ---------------------------------------------------------------------------
// SSingleDiffMenu
// ---------------------------------------------------------------------------

// PANZERS 0x6336d0
SSingleDiffMenu::SSingleDiffMenu()
{
    Difficulty = 1;                                               // param_1[0xf1] = 1
}

// PANZERS 0x634330
SSingleDiffMenu::~SSingleDiffMenu()
{
}

// PANZERS 0x639540
// The Training Camp dialog's skin (menu/message_hq.tga, centred), the header
// sprite with "Select difficulty", Easy / Normal / Hard at (0x1e, 0x4b +
// 0x23 i), the description box at (0xb4, 0x46) 300 wide, Normal checked
// through OnAction, Start / Cancel at y 0xe6.
void SSingleDiffMenu::Create()
{
    PZ_M3_TRACE("SSingleDiffMenu::Create (0x639540)");
    SetBackgroundSprite(g_MenuMessageTex, 0, false, false);       // 0x539ba0(DAT_008da794, 0)
    SDXWidget::Create(0);                                         // 0x539a10
    SetPosition((0x400 - Width) / 2, (0x300 - Height) / 2, Width, Height);   // vtbl +0x08
    Cursor = 0;                                                   // 0x543970(0, -1)
    int header = Board->CreateFrame(FT_SPRITE, BackFrame, (Width - 0x124) / 2, 0, 0, 0);   // board +0x08(1, ...)
    Board->SetSpriteGlyph(header, g_MenuControlsFont, 9);         // board +0x24(f, DAT_008da788, 9)
    int text = Board->CreateFrame(FT_TEXT, header, 0x92, 0xc, 0, 0);
    Board->SetText(text, g_PzFont[PZF_SANS21], 2, Mm("Select difficulty"));   // board +0x34
    Board->SetTextColor(text, 0xffffff);                          // board +0x28
    for (int i = 0, y = 0x4b; y < 0xb4; ++i, y += 0x23) {
        InsertChild(&Levels[i]);                                  // vtbl +0x54
        Levels[i].SetPosition(0x1e, y, 0, 0);
        Levels[i].CreateHD(0, g_MenuControlsFont);                // 0x53eaf0
    }
    Levels[0].SetText(Mm("Easy"));                                // 0x53ecf0 (0x807784)
    Levels[1].SetText(Mm("Normal"));
    Levels[2].SetText(Mm("Hard"));                                // 0x80778c
    InsertChild(&Text);                                           // vtbl +0x54(+0x284)
    Text.SetPosition(0xb4, 0x46, 300, 0);
    Text.Create(3, 5, false, false, 0, false);                    // 0x542380(3, 5, 0, 0, 0, 0)
    OnAction(&Levels[1], PZA_BUTTON_CLICK, 0);                    // vtbl +0x44(+0xc4, 0x42542, 0)
    InsertChild(&Start);
    Start.SetPosition(Width / 2 - 0xc4, 0xe6, 0, 0);
    Start.Create(1, Mm("Start"));                                 // 0x5386c0(1, text)
    InsertChild(&Cancel);
    Cancel.SetPosition(Width / 2, 0xe6, 0, 0);
    Cancel.Create(1, Mm("Cancel"));
    SetFocus();                                                   // 0x5439f0
}

// PANZERS 0x63c580
bool SSingleDiffMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action != PZA_BUTTON_CLICK)                               // 0x42542
        return false;
    int idx = -1;
    for (int i = 0; i < 3; ++i)
        if (source == &Levels[i])
            idx = i;
    if (idx < 0) {
        if (source == &Start)                                     // +0x19c
            SendAction(PZA_NEWGAME_START, 0);                     // 0x543930(0x53441, 0)
        else if (source == &Cancel)                               // +0x210
            SendAction(PZA_NEWGAME_CANCEL, 0);                    // 0x543930(0x53442, 0)
        return true;
    }
    for (int i = 0; i < 3; ++i)                                   // 0x53ecd0(i == idx)
        Levels[i].SetCheck(i == idx);
    Text.Clear();                                                 // +0x2f4 = -1, 0x53e300(0), +0x2e4 = 0, vtbl +0x78
    static const char* const kText[3] = {
        "After a battle you will receive free trained reinformcements to make up for your lost units.",
        "After a battle you will receive free greenhorn reinformcements to make up for your lost units.",
        "There are no replacements for the units lost in a battle.",
    };
    Text.AddLine(Mm(kText[idx]), 0xd0d0d0);                       // 0x541b60
    Difficulty = idx;                                             // +0x3c4
    return true;
}

// ---------------------------------------------------------------------------
// SSingleMenu
// ---------------------------------------------------------------------------

// PANZERS 0x633770
SSingleMenu::SSingleMenu()
{
    ScenarioMenu = nullptr;                                       // param_1[0x16]
    DiffMenu = nullptr;                                           // param_1[0x6e]
    PictureFont = -1;                                             // param_1[0x6f]
    Race = 0;
}

// PANZERS 0x6343e0
SSingleMenu::~SSingleMenu()
{
    if (ScenarioMenu) {                                           // +0x58 vtbl +0 (1)
        delete ScenarioMenu;
        ScenarioMenu = nullptr;
    }
    if (DiffMenu) {
        delete DiffMenu;
        DiffMenu = nullptr;
    }
    if (PictureFont >= 0)
        PzReleaseCustomFont(PictureFont);                         // board +0x80(+0x1bc)
}

// PANZERS 0x639790
// "New Game" centred left of the main menu (SCenterMenu instant), Skirmish /
// Scenario, then the three campaign pictures of menu/campaign_select_hq.tga
// (board +0x70: 0xa1 x 0x143 cells, 3 per row, 9 glyphs: row 0 normal, row 1
// over, row 2 pressed) at (0x1b + 0xbc i, 0x3a) with their tool tips.
void SSingleMenu::Create()
{
    PZ_M3_TRACE("SSingleMenu::Create (0x639790)");
    SCenterMenu::Create(Mm("New Game"), true);                    // 0x64bc80(text, 1)
    CreateOkCancel(&Skirmish, Mm("Skirmish"), &Scenario, Mm("Scenario"));   // 0x64bf60(+0xd0, .., +0x144, ..)
    SCustomGlyph glyphs[9];
    for (int i = 0; i < 9; ++i)
        glyphs[i] = { (i % 3) * 0xa1, (i / 3) * 0x143, 0xa1, 0x143 };
    PictureFont = PzLoadCustomFont("menu/campaign_select_hq.tga", 9, glyphs);   // board +0x70(name, 0xa1, 0x143, 3, 9, 0)
    for (int i = 0, x = 0x1b; x < 0x24f; ++i, x += 0xbc) {
        InsertChild(&Pictures[i]);                                // vtbl +0x54
        Pictures[i].SetPosition(x, 0x3a, 0, 0);                   // vtbl +0x08
        Pictures[i].Create(PictureFont, i, i + 6, i + 3, -1);     // 0x537a80(font, i, i + 6, i + 3, -1)
    }
    Pictures[0].SetEnable(true);                                  // vtbl +0x70(1)
    Pictures[0].SetTooltipText(Mm("German campaign"));            // 0x543bf0
    Pictures[1].SetTooltipText(Mm("Russian campaign"));
    Pictures[2].SetTooltipText(Mm("Allied campaign"));
    SetFocus();                                                   // 0x5439f0
}

// PANZERS 0x63c7e0
// A picture: Race (German 0, Russian 2, Allied 1) and the difficulty dialog
// (child of this menu's parent, modal). Scenario: LoadScenarioMenu
// 0x63b2d0; Skirmish: 0x534d1 to the super window.
bool SSingleMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action != PZA_BUTTON_CLICK)                               // 0x42542
        return false;
    for (int i = 0; i < 3; ++i) {
        if (source != &Pictures[i])
            continue;
        Race = i == 0 ? 0 : 3 - i;                                // +0x31c
        DiffMenu = new SSingleDiffMenu();                         // new 0x3c8, 0x6336d0
        Parent->InsertChild(DiffMenu);                            // (+0x24)->vtbl +0x54
        DiffMenu->Create();                                       // 0x639540
        if (SWindow* w = DiffMenu->GetWindowParent())             // 0x5435b0
            w->SetModalWidget(DiffMenu);                          // 0x544fe0
    }
    if (source == &Scenario) {                                    // +0x144
        LoadScenarioMenu();                                       // 0x63b2d0
        return true;
    }
    if (source == &Skirmish) {                                    // +0xd0
        SendAction(PZA_NEWGAME_SKIRMISH, 0);                      // 0x543930(0x534d1, 0) -> SSuperWindow 0x658e70 (M5-SK)
        return true;
    }
    return true;
}
