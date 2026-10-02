// window/editbox.h
// SEditBox header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SEDITBOX_H
#define WINDOW_SEDITBOX_H

#include "dxwidget.h"

struct SEditBox : SDXWidget {
    int Font;
    int TextFrame;
    int CaretFrame;
    int CaretTimer;
    bool CaretVisible;
    char Utf8Text[766];
    wchar_t Text[256];
    int CaretPos;
    bool CtrlPressed;

    SEditBox();
    ~SEditBox() override;

    bool OnChar(int ch) override;
    bool OnKeyDown(int keycode, bool repeat = false) override;
    bool OnKeyUp(int keycode) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnTimer(int id, unsigned int time) override;
    void Update() override;

    void Create(int font, bool alpha, int background);
    char *GetText();
    void SetText(const char *text);

    // HD v1.7 backport — Win32 clipboard support (Ctrl+C / Ctrl+V).
    void GetClipboardText();
    void SetClipboardText();
};

#endif // WINDOW_SEDITBOX_H
