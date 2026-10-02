// window/tipdialog.h
// STipDialog header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_STIPDIALOG_H
#define WINDOW_STIPDIALOG_H

#include "dialog.h"
#include "listbox.h"
#include "textbutton.h"

struct SProperties;

struct STipDialog : SDialog {
    STextButton OKButton;
    STextButton NextButton;
    STextButton PrevButton;
    SListBox ListBox;
    SProperties *TipsIni;
    int ActiveTip;

    STipDialog();
    ~STipDialog() override;

    void Cancel() override;
    bool OnAction(SWidget *sender, int action, int param) override;
    bool OnKeyDown(int keycode, bool repeat = false) override;

    void Create();
    int LoadTip(int TipNumber);
};

#endif // WINDOW_STIPDIALOG_H
