// window/messageboxwithoutbuttons.h
// SMessageBoxWithoutButtons header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_MESSAGEBOXWITHOUTBUTTONS_H
#define WINDOW_MESSAGEBOXWITHOUTBUTTONS_H

#include "dialog.h"
#include "listbox.h"

struct SMessageBoxWithoutButtons : SDialog {
    SListBox ListBox;
    bool bInGame;

    SMessageBoxWithoutButtons();
    ~SMessageBoxWithoutButtons() override;

    void SetVisible(bool vis) override;

    void Cancel();
    void Create(BOOL inGame);
    void SetText(const char *text);
};

#endif // WINDOW_MESSAGEBOXWITHOUTBUTTONS_H
