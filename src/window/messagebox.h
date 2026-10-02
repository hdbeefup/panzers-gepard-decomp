// window/messagebox.h
// SMessageBox header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SMESSAGEBOX_H
#define WINDOW_SMESSAGEBOX_H

#include "dialog.h"
#include "listbox.h"
#include "textbutton.h"

struct SMessageBox : SDialog {
    STextButton OKButton;
    STextButton YesButton;
    STextButton NoButton;
    STextButton RetryButton;
    STextButton IgnoreButton;
    STextButton CancelButton;
    SListBox ListBox;
    int Type;

    SMessageBox();
    ~SMessageBox() override;

    void Cancel() override;
    bool OnAction(SWidget *sender, int action, int param) override;
    bool OnChar(int ch) override;
    bool OnKeyDown(int keycode, bool repeat = false) override;

    void Create(const char *text, int type);
};

#endif // WINDOW_SMESSAGEBOX_H
