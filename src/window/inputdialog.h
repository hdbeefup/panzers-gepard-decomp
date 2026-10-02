// window/inputdialog.h
// SInputDialog header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SINPUTDIALOG_H
#define WINDOW_SINPUTDIALOG_H

#include "dialog.h"
#include "editboxwithtext.h"
#include "textbutton.h"

struct SInputDialog : SDialog {
    STextButton OKButton;
    STextButton YesButton;
    STextButton NoButton;
    STextButton CancelButton;
    int Type;
    SEditBoxWithText EditBox;

    SInputDialog();
    ~SInputDialog() override;

    void Cancel() override;
    bool OnAction(SWidget *sender, int action, int param) override;
    bool OnKeyDown(int keycode, bool repeat = false) override;

    void Create(int a2, const char *text, const char *prompt, const char *edittext, int type);
};

#endif // WINDOW_SINPUTDIALOG_H
