// src/panzers/inputdialog_pz.h
// HD's Panzers input dialog (the market's "Save Army" box, M6-AR):
//   pz::SEditBoxWithText  HD SEditBoxWithText (ctor 0x53b140): a label and
//                         an SEditBox right of it (+0x60).
//   pz::SInputDialog      HD SInputDialog, vftable 0x7f2c3c, 0x418 bytes
//                         (ctor 0x53b3a0, dtor 0x53b480 / deleting 0x53b590,
//                         Create 0x53b5c0): the message-box frame with a
//                         title, one labelled edit line and OK / Cancel (or
//                         Yes / No); sends 0x49581..0x49584. Own vtable
//                         slots: +0x14 OnKeyDown 0x53b910, +0x44 OnAction
//                         0x53b880, +0x6c SetVisible 0x53ba60.
// The SWINE classes of the same names (src/window) draw SWINE skins, so
// these live in namespace pz (as hudwidgets.h does).
//
// OWNER: agent AR (M6, docs/m6/ar.md).

#ifndef PANZERS_INPUTDIALOG_PZ_H
#define PANZERS_INPUTDIALOG_PZ_H

#include "dxwidget.h"
#include "mainmenu.h"
#include "savemenu.h"

// SInputDialog results (sent to the target / the parents, 0x53b9c0).
enum PzInputDialogAction {
    PZA_INPUT_OK     = 0x49581,
    PZA_INPUT_YES    = 0x49582,
    PZA_INPUT_NO     = 0x49583,
    PZA_INPUT_CANCEL = 0x49584,
};

namespace pz {

struct SEditBoxWithText : SDXWidget {
    int      Font;            // +0x58 label font (g_PzFont index)
    int      TextFrame;       // +0x5c label text frame
    SEditBox Edit;            // +0x60

    SEditBoxWithText();                                              // 0x53b140
    void Create(int font, int editFont, const char* label);          // 0x53b200
    void SetLabel(const char* label);                                // 0x53b2d0
};

struct SInputDialog : SDXWidget {
    SComplexButton   OkButton;      // +0x058
    SComplexButton   YesButton;     // +0x0cc
    SComplexButton   NoButton;      // +0x140
    SComplexButton   CancelButton;  // +0x1b4
    int              Type;          // +0x228 0 OK, 1 OK / Cancel, 4 Yes / No
    bool             Modal;         // +0x22c
    SWidget*         Target;        // +0x230 receives the result first (0x53b9c0)
    int              TitleFrame;    // +0x234
    SEditBoxWithText Edit;          // +0x238

    SInputDialog();                                                  // 0x53b3a0
    ~SInputDialog() override;                                        // 0x53b480

    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x53b910
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x53b880
    void SetVisible(bool visible) override;                          // +0x6c 0x53ba60

    void Create(const char* title, const char* label, const char* text, int type, bool modal); // 0x53b5c0
    const char* GetText() const { return Edit.Edit.GetText(); }      // 0x53b260
    void SetText(const char* text) { Edit.Edit.SetText(text); }      // 0x53b360

private:
    void SendResult(int action);                                     // 0x53b9c0
};

} // namespace pz

#endif // PANZERS_INPUTDIALOG_PZ_H
