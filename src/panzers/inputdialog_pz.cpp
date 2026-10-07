// src/panzers/inputdialog_pz.cpp
// HD's Panzers SInputDialog / SEditBoxWithText (inputdialog_pz.h), lifted
// for the market's "Save Army" box (army making, M6-AR).
// OWNER: agent AR (M6, docs/m6/ar.md).

#include <windows.h>
#include "inputdialog_pz.h"
#include "pzboard.h"
#include "board.h"
#include "gettext.h"
#include "logger.h"
#include "window.h"

namespace pz {

static int IdFont(int hdFont)
{
    return (hdFont >= 0 && hdFont < 6) ? g_PzFont[hdFont] : hdFont;
}

// PANZERS 0x53b140
SEditBoxWithText::SEditBoxWithText()
{
    Font = 0;
    TextFrame = -1;
}

// PANZERS 0x53b200
void SEditBoxWithText::Create(int font, int editFont, const char* label)
{
    SDXWidget::Create(0);                                          // 0x539a10
    Font = font;                                                   // +0x58
    TextFrame = Board->CreateFrame(FT_TEXT, BackFrame, 0, 0, 0, 0); // board +0x08(2, +0x48, 0, 0, 0, 0)
    InsertChild(&Edit);                                            // vtbl +0x54(+0x60)
    SetLabel(label);                                               // 0x53b2d0
    Edit.Create(editFont, true);                                   // 0x53a750(editFont, 1)
}

// PANZERS 0x53b2d0
// The label, then the edit box from 2 pixels right of it to the right edge
// (6 pixels up with the 21-point fonts, else 1).
void SEditBoxWithText::SetLabel(const char* label)
{
    Board->SetText(TextFrame, IdFont(Font), 0, label ? label : ""); // board +0x34(f, font, 0, s, 0)
    int w = 0, h = 0;
    Board->GetFrameSize(TextFrame, &w, &h);                        // board +0x20
    w += 2;
    int y = (Font == 2 || Font == 3 || Font == 8 || Font == 9) ? -6 : -1;
    Edit.SetPosition(w, y, Width - w, Height);                     // vtbl +0x08
}

// PANZERS 0x53b3a0
SInputDialog::SInputDialog()
{
    Type = 0;
    Modal = false;                                                 // +0x22c = 0
    Target = nullptr;                                              // +0x230 = 0
    TitleFrame = -1;                                               // +0x234 = -1
}

// PANZERS 0x53b480
SInputDialog::~SInputDialog()
{
    if (Modal && Visible) {                                        // +0x22c, +0x39
        if (SWindow* w = GetWindowParent())                        // 0x5435b0
            w->UnsetModalWidget(this);                             // 0x5450e0
    }
}

// PANZERS 0x53b5c0
void SInputDialog::Create(const char* title, const char* label, const char* text, int type, bool modal)
{
    Type = type;                                                   // +0x228
    Modal = modal;                                                 // +0x22c
    SetBackgroundSprite(g_MenuMessageTex, 0, false, false);        // 0x539ba0(DAT_008da794, 0)
    SDXWidget::Create(0);                                          // 0x539a10
    SetPosition((0x400 - Width) / 2, (0x300 - Height) / 2, Width, Height); // vtbl +0x08
    Cursor = 0;                                                    // 0x543970(0, -1)
    int header = Board->CreateFrame(FT_SPRITE, BackFrame, (Width - 0x124) / 2, 0, 0, 0);
    Board->SetSpriteGlyph(header, g_MenuControlsFont, 9);          // board +0x24(f, DAT_008da788, 9)
    TitleFrame = Board->CreateFrame(FT_TEXT, header, 0x92, 0xc, 0, 0);
    Board->SetText(TitleFrame, g_PzFont[PZF_SANS21], 2, title ? title : "");
    Board->SetTextColor(TitleFrame, 0xffffff);
    InsertChild(&Edit);                                            // vtbl +0x54(+0x238)
    Edit.SetPosition(Width / 2 - 200, Height / 2 - 0x14, 400, 0x14);
    Edit.Create(3, 2, label);                                      // 0x53b200(3, 2, label)
    SetText(text);                                                 // 0x53b360
    const char* last;
    if (type == 0) {
        InsertChild(&OkButton);
        OkButton.SetPosition((Width - 0xc4) / 2, 0xe6, 0, 0);
        last = "OK";
        OkButton.Create(1, ::GetText("window/InputDialog.cpp", last));
    } else if (type == 1) {
        InsertChild(&OkButton);
        OkButton.SetPosition(Width / 2 - 0xc4, 0xe6, 0, 0);
        OkButton.Create(1, ::GetText("window/InputDialog.cpp", "OK"));   // DAT_007f2cb8
        InsertChild(&CancelButton);
        CancelButton.SetPosition(Width / 2, 0xe6, 0, 0);
        CancelButton.Create(1, ::GetText("window/InputDialog.cpp", "Cancel"));
    } else if (type == 4) {
        InsertChild(&YesButton);
        YesButton.SetPosition(Width / 2 - 0xc4, 0xe6, 0, 0);
        YesButton.Create(1, ::GetText("window/InputDialog.cpp", "Yes")); // DAT_007f2cd4
        InsertChild(&NoButton);
        NoButton.SetPosition(Width / 2, 0xe6, 0, 0);
        NoButton.Create(1, ::GetText("window/InputDialog.cpp", "No"));
    } else {
        Logger.g->Panic("SInputDialog::Create: Unsupported messagebox type");
    }
    SDXWidget::SetVisible(false);                                  // 0x539c40(0)
    Edit.Edit.SetFocus();                                          // 0x53b2c0 (0x5439f0 on the edit box)
}

// PANZERS 0x53b9c0
void SInputDialog::SendResult(int action)
{
    SWidget* w = Target ? Target : this;                           // +0x230
    while (w && !w->OnAction(this, action, 0))                     // vtbl +0x44
        w = w->Parent;                                             // +0x24
}

// PANZERS 0x53b880
// The buttons send their result; the dialog stays up (the receiver hides it).
bool SInputDialog::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action != 0x42542)
        return false;
    if (source == &OkButton) SendResult(PZA_INPUT_OK);
    else if (source == &YesButton) SendResult(PZA_INPUT_YES);
    else if (source == &NoButton) SendResult(PZA_INPUT_NO);
    else if (source == &CancelButton) SendResult(PZA_INPUT_CANCEL);
    return true;
}

// PANZERS 0x53b910
// Enter: hidden, OK (Yes); Esc: hidden, OK / No / Cancel by the type.
bool SInputDialog::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    if (key == VK_RETURN) {
        if (Type == 0 || Type == 1) {
            SetVisible(false);
            SendResult(PZA_INPUT_OK);
        } else if (Type == 4) {
            SetVisible(false);
            SendResult(PZA_INPUT_YES);
        }
    } else if (key == VK_ESCAPE) {
        if (Type == 0) {
            SetVisible(false);
            SendResult(PZA_INPUT_OK);
        } else if (Type == 4) {
            SetVisible(false);
            SendResult(PZA_INPUT_NO);
        } else if (Type == 1) {
            SetVisible(false);
            SendResult(PZA_INPUT_CANCEL);
        }
    }
    return true;
}

// PANZERS 0x53ba60
void SInputDialog::SetVisible(bool visible)
{
    if (visible == Visible)
        return;
    SDXWidget::SetVisible(visible);                                // 0x539c40
    if (visible)
        Edit.Edit.SetFocus();                                      // 0x53b2c0
    if (!Modal)
        return;
    SWindow* w = GetWindowParent();                                // 0x5435b0
    if (Visible) {
        SetFocus();                                                // 0x5439f0
        if (w)
            w->SetModalWidget(this);                               // 0x544fe0
    } else if (w) {
        w->UnsetModalWidget(this);                                 // 0x5450e0
    }
}

} // namespace pz
