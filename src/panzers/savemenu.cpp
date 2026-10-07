// src/panzers/savemenu.cpp
// The in-game Save Game screen (SSaveMenu) and Panzers' edit box
// (pz::SEditBox), lifted from the HD exe; see savemenu.h. The SGameView
// side (OpenSaveMenu 0x6205f0 and the actions 0x49531 / 0x49533) is in
// ingamemenu.cpp. OWNER: agent M5-SV.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "savemenu.h"
#include "pzboard.h"
#include "board.h"
#include "gettext.h"
#include "logger.h"
#include "stream.h"
#include "m3common.h"
#include "campaign.h"
#include "world_save.h"

static int EbFont(int hdFont)
{
    return (hdFont >= 0 && hdFont < 6) ? g_PzFont[hdFont] : hdFont;
}

static const char* Tx(const char* id) { return GetText("panzers/InGameMenu.cpp", id); }

// HD 0x5439f0: walk up the parents making each widget the focused child.
static void FocusWidget(SWidget* widget)
{
    for (SWidget* w = widget; w && w->Enabled && w->Visible && w->Parent; w = w->Parent) {
        if (w->Parent->Focus != w) {
            for (SWidget* c = w->Parent->Child; c; c = c->Sibling)
                if (c->FocusSibling == w)
                    c->FocusSibling = nullptr;
            w->FocusSibling = w->Parent->Focus;
            w->Parent->Focus = w;
        }
    }
}

// ===========================================================================
// pz::SEditBox
// ===========================================================================

namespace pz {

// PANZERS 0x53a6c0
SEditBox::SEditBox()
{
    Font = 0;
    TextFrame = -1;                                               // +0x5c
    CaretFrame = -1;                                              // +0x60
    CaretTimer = 0;
    CaretVisible = false;
    memset(Text, 0, sizeof(Text));                                // +0x69, 0xff (+0x168)
    CaretPos = 0;                                                 // +0x16c
    Uppercase = false;                                            // +0x170
    LeadByte = 0;                                                 // +0x174
    memset(DbcsBuf, 0, sizeof(DbcsBuf));
    Password = false;                                             // +0x17e
}

// PANZERS 0x53a720 (SDXWidget dtor 0x539930)
SEditBox::~SEditBox()
{
}

// PANZERS 0x53a750
// The widget is one line of the font high (plus 0xc with the 9-slice frame
// 0x543d90); a fixed-text frame holds the text, a sprite of the font's
// glyph '|' (0x7c) the caret, which blinks every 500 ms.
void SEditBox::Create(int font, bool frame)
{
    Font = font;                                                  // +0x58
    int w = 0, h = 0;
    Board->GetTextExtent(EbFont(font), nullptr, 0, &w, &h, 1.0f); // board +0x88(font, 0, 0, ...)
    Resize(Width, frame ? h + 0xc : h);                           // vtbl +0x10
    SDXWidget::Create(0);                                         // 0x539a10
    if (!frame) {
        TextFrame = Board->CreateFrame(FT_FIXTEXT, BackFrame, 0, 0, 0, 1);   // board +0x08(3, ...)
        Board->ResizeFrame(TextFrame, Width, h);                  // board +0x14
    } else {
        PzDrawFrameBox(this);                                     // 0x543d90
        TextFrame = Board->CreateFrame(FT_FIXTEXT, BackFrame, 8, 6, 0, 1);
        Board->ResizeFrame(TextFrame, Width - 0xc, h);
    }
    CaretFrame = Board->CreateFrame(FT_SPRITE, TextFrame, 0, 0, 0, 1);   // board +0x08(1, ...)
    Board->SetSpriteGlyph(CaretFrame, EbFont(Font), 0x7c);        // board +0x24(caret, font, '|')
    CaretVisible = true;                                          // +0x68
    CaretTimer = SetTimer(500);                                   // 0x543b10
    FocusWidget(this);                                            // 0x5439f0
    Update();                                                     // vtbl +0x78
}

// PANZERS 0x53ae10 (the DBCS reset 0x64dd70 is not mapped: Western text)
void SEditBox::SetText(const char* text)
{
    strncpy(Text, text ? text : "", 0xff);
    Text[0xff] = 0;                                               // +0x168
    CaretPos = (int)strlen(Text);                                 // +0x16c
    Update();
}

// PANZERS 0x53a8c0
// A printable character is inserted at the caret while the text is
// narrower than the box minus 0x12 and shorter than 0xff. The DBCS path
// (0x64dd70, lead bytes kept in +0x174 / +0x178) is not mapped.
// Recompile: the window gets UTF-16 WM_CHAR codes; HD's ANSI window got
// code-page bytes, so a code above 0x7f is converted to the ANSI code page.
bool SEditBox::OnChar(int code)
{
    unsigned int c = (unsigned int)code;
    if (c > 0x7f) {
        wchar_t wc = (wchar_t)code;
        char mb[4];
        BOOL lossy = FALSE;
        int n = WideCharToMultiByte(CP_ACP, 0, &wc, 1, mb, sizeof(mb), nullptr, &lossy);
        if (n != 1 || lossy)
            return true;
        c = (unsigned char)mb[0];
    }
    if (c <= 0x1f)
        return true;
    int len = (int)strlen(Text);
    int w = 0, h = 0;
    Board->GetTextExtent(EbFont(Font), Text, len, &w, &h, 1.0f);  // board +0x88
    if (w >= Width - 0x12)
        return true;
    if (Uppercase && c - 0x61 < 0x1a)                             // +0x170
        c -= 0x20;
    if (c > 0x1f && len < 0xff) {
        memmove(Text + CaretPos + 1, Text + CaretPos, (len - CaretPos) + 1);
        Text[CaretPos] = (char)c;
        ++CaretPos;
        Update();
    }
    return true;
}

// PANZERS 0x53aad0 (the DBCS steps under DAT_00929dec are not mapped)
// Backspace, End, Home, Left, Right and Delete edit; Tab, Enter and Esc go
// to the parent; every other key is taken.
bool SEditBox::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    switch (key) {
    case VK_BACK: {
        if (CaretPos < 1)
            return true;
        --CaretPos;
        memmove(Text + CaretPos, Text + CaretPos + 1, strlen(Text + CaretPos + 1) + 1);
        Update();
        return true;
    }
    case VK_TAB:
    case VK_RETURN:
    case VK_ESCAPE:
        return false;
    case VK_END:
        CaretPos = (int)strlen(Text);
        Update();
        return true;
    case VK_HOME:
        CaretPos = 0;
        Update();
        return true;
    case VK_LEFT:
        if (CaretPos > 0) {
            --CaretPos;
            Update();
        }
        return true;
    case VK_RIGHT:
        if (CaretPos < (int)strlen(Text)) {
            ++CaretPos;
            Update();
        }
        return true;
    case VK_DELETE:
        if (Text[CaretPos] == 0)
            return true;
        memmove(Text + CaretPos, Text + CaretPos + 1, strlen(Text + CaretPos + 1) + 1);
        Update();
        return true;
    default:
        return true;
    }
}

// PANZERS 0x53adc0
void SEditBox::OnMouseDown(int button, int x, int y, int shift)
{
    (void)shift;
    if (button == 1 && x >= 0 && x < Width && y >= 0 && y < Height)
        FocusWidget(this);                                        // 0x5439f0
}

// PANZERS 0x53adf0
void SEditBox::OnTimer(int id, unsigned int elapsed)
{
    (void)id; (void)elapsed;
    CaretVisible = !CaretVisible;
    Update();
}

// PANZERS 0x53ae80
// The text ('*' per character with +0x17e), the caret after CaretPos
// characters (back to 0 when that is past the box) and shown only while the
// box has the focus.
void SEditBox::Update()
{
    if (BackFrame < 0)
        return;
    char shown[0x100];
    int len = (int)strlen(Text);
    if (Password) {
        memset(shown, '*', len);
        shown[len] = 0;
    } else {
        memcpy(shown, Text, len + 1);
    }
    Board->SetText(TextFrame, EbFont(Font), 0, shown);            // board +0x34(frame, font, 0, text, 0)
    int w = 0, h = 0;
    Board->GetTextExtent(EbFont(Font), shown, CaretPos, &w, &h, 1.0f);   // board +0x88
    if (Width - 8 < w) {
        CaretPos = 0;
        w = 1;
    }
    Board->MoveFrame(CaretFrame, w - 1, 0);                       // board +0x10
    Board->ShowFrame(CaretFrame, IsFocused() ? CaretVisible : false);    // board +0x18, 0x543670
}

} // namespace pz

// ===========================================================================
// SSaveMenu
// ===========================================================================

// PANZERS 0x62ccb0
SSaveMenu::SSaveMenu()
{
    _46c = -1;                                                    // +0x46c
}

// PANZERS 0x62d3f0
SSaveMenu::~SSaveMenu()
{
}

// The list part of Create 0x62fcb0 (also in OnAction's Delete): "Empty
// slot" first, then LoadSavedGameNames 0x595fa0 sorted newest first from
// row 1 (0x53ff80(1)); 0x62cde0 frees the names.
void SSaveMenu::FillList()
{
    SYSTEMTIME zero;
    memset(&zero, 0, sizeof(zero));
    List.AddItem("", Tx("Empty slot"), "", "", zero, 0xd0d0d0);   // 0x53f1b0
    pz::SHdArray<pz::SLoadSaveName> names = { nullptr, 0, 0 };
    pz::LoadSavedGameNames(&names);                               // 0x595fa0
    for (int i = 0; i < names.Size; ++i) {
        const pz::SLoadSaveName& n = names.Array[i];
        List.AddItem(pz::SStr(n.Code), pz::SStr(n.Title), pz::SStr(n.Date), pz::SStr(n.File), n.Time, 0xd0d0d0);
    }
    List.Sort(1);                                                 // 0x53ff80(1)
    for (int i = 0; i < names.Size; ++i) {
        pz::FreeSString(&names.Array[i].Code);
        pz::FreeSString(&names.Array[i].Title);
        pz::FreeSString(&names.Array[i].Date);
        pz::FreeSString(&names.Array[i].File);
    }
    free(names.Array);
}

// PANZERS 0x62fcb0
void SSaveMenu::Create()
{
    PZ_M3_TRACE("SSaveMenu::Create (0x62fcb0)");
    SCenterMenu::Create(Tx("Save Game"), false);                  // 0x64bc80
    // 0x64bed0: three buttons at the bottom.
    InsertChild(&DeleteButton);
    DeleteButton.SetPosition(3, 400, 0, 0);
    DeleteButton.Create(1, Tx("Delete"));
    InsertChild(&SaveButton);
    SaveButton.SetPosition(199, 400, 0, 0);
    SaveButton.Create(1, Tx("Save"));
    InsertChild(&BackButton);
    BackButton.SetPosition(0x18b, 400, 0, 0);
    BackButton.Create(1, Tx("Back"));
    SaveButton.SetEnable(false);                                  // vtbl +0x70(0)
    // HD 0x543bf0: the Delete hint "Delete the selected savegame file"
    // (tool tips are not mapped).
    DeleteButton.SetEnable(false);
    int caption = Board->CreateFrame(FT_TEXT, BackFrame, 0x32, 0x32, 0, 1);   // board +0x08(2, ...)
    Board->SetText(caption, EbFont(2), 0, Tx("Games"));           // board +0x34
    InsertChild(&List);                                           // vtbl +0x54
    List.SetPosition(0x28, 0x46, 0x208, 0);                       // vtbl +0x08
    List.Create(0, 0x12, 0x1e, 8, 0x172, 8, 0x5f, true);          // 0x53f4c0
    FillList();
    // The edit box over the title column (moved to the row on a click).
    List.InsertChild(&Edit);                                      // vtbl +0x54
    Edit.SetPosition(0, 0, List.TitleWidth, 0x12);                // +0x2e8 = list +0x134
    Edit.SetBackgroundColor(0xff000000);                          // 0x539b90
    Edit.Create(0, false);                                        // 0x53a750(0, 0)
    Edit.SetVisible(false);                                       // vtbl +0x6c(0)
    FocusWidget(this);                                            // 0x5439f0
}

// PANZERS 0x630f20
// Hide the edit box, focus the parent, no selection, Save / Delete off.
void SSaveMenu::Deselect()
{
    Edit.SetVisible(false);                                       // vtbl +0x6c(0)
    FocusWidget(Parent);                                          // [+0x24] 0x5439f0
    SaveButton.SetEnable(false);
    DeleteButton.SetEnable(false);
    List.SetCurSel(-1);                                           // +0x224 = -1, vtbl +0x78
}

// PANZERS 0x632a50
// Row 0 ("Empty slot"): SaveGames/<time>.save; else the selected file is
// written again. The title is the edit box's text. Then 0x49531 to the view.
void SSaveMenu::Save()
{
    PZ_M3_TRACE("SSaveMenu::Save (0x632a50)");
    char file[400];
    if (List.CurSel == 0) {
        _snprintf(file, sizeof(file) - 1, "SaveGames/%d.save", (int)_time64(nullptr));   // 0x51ee20(0x80644c)
    } else {
        _snprintf(file, sizeof(file) - 1, "SaveGames/%s", List.GetFileName(List.CurSel));   // 0x53f9c0, 0x5335c0
    }
    file[sizeof(file) - 1] = 0;
    if (pz::g_Campaign)
        pz::g_Campaign->SaveGame(file, Edit.GetText());           // 0x5966a0
    Logger.g->Log(0, "SSaveMenu: saved %s \"%s\"", file, Edit.GetText());
    SendAction(PZA_SAVE_DONE, 0);                                 // 0x543930(0x49531, 0)
}

// PANZERS 0x632530
bool SSaveMenu::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    if (key == VK_ESCAPE) {
        if (Edit.Visible) {                                       // +0x325
            Deselect();
            return true;
        }
    } else if (key == VK_RETURN && SaveButton.Enabled) {          // +0x90
        Save();
        return true;
    }
    return false;
}

// PANZERS 0x632590
void SSaveMenu::OnMouseDown(int button, int x, int y, int shift)
{
    (void)x; (void)y; (void)shift;
    if (button == 1)
        Deselect();
}

// PANZERS 0x632040
bool SSaveMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    PZ_M3_TRACE("SSaveMenu::OnAction (0x632040)");
    if (action == PZA_LISTBOX_SCROLLED) {
        Deselect();
        return true;
    }
    if (action == PZA_BUTTON_CLICK) {
        if (source == &SaveButton) {
            Save();
            return true;
        }
        if (source == &DeleteButton) {
            char path[400];
            _snprintf(path, sizeof(path) - 1, "SaveGames/%s", List.GetFileName(List.CurSel));   // 0x53f9c0, 0x52c580
            path[sizeof(path) - 1] = 0;
            DeleteFileA(path);
            Logger.g->Log(0, "SSaveMenu: deleted %s", path);
            List.ResetContent();                                  // 0x53ffc0
            FillList();
            Deselect();                                           // 0x630f20
            return true;
        }
        if (source == &BackButton) {
            SendAction(PZA_SAVE_BACK, 0);                         // 0x543930(0x49533, 0)
            return true;
        }
        return false;
    }
    if (action != PZA_LISTBOX_SELECT)
        return false;
    // A row was picked: the edit box opens over its title.
    Edit.SetVisible(false);                                       // vtbl +0x6c(0)
    int sel = List.CurSel;                                        // +0x224
    if (sel == 0) {
        Edit.SetText(pz::g_Campaign ? pz::CampaignGetName(pz::g_Campaign) : "");   // 0x5925d0
    } else {
        SString code, title, date;
        List.GetText(sel, &code, &title, &date);                  // 0x53fa70
        Edit.SetText(pz::SStr(title));
        pz::FreeSString(&code);
        pz::FreeSString(&title);
        pz::FreeSString(&date);
    }
    Edit.SetPosition(0x31, (sel - List.Columns * List.TopIndex) * 0xe + 6, Edit.Width, Edit.Height);   // vtbl +0x0c (0x543770: move)
    Edit.SetVisible(true);
    FocusWidget(&Edit);                                           // 0x5439f0
    SaveButton.SetEnable(true);                                   // vtbl +0x70(1)
    DeleteButton.SetEnable(sel >= 1);
    return true;
}
