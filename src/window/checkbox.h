// window/checkbox.h
// SCheckBox header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SCHECKBOX_H
#define WINDOW_SCHECKBOX_H

#include "dxwidget.h"
#include "string2.h"

struct SCheckBox : SDXWidget {
    int Font;
    int CheckFont;
    int UnCheckFont;
    int CheckFontGrey;
    int UnCheckFontGrey;
    int TextFrame;
    int CheckFrame;
    SString Text;
    unsigned int NormalColor;
    unsigned int ActiveColor;
    unsigned int DisabledColor;
    bool Active;
    bool Pressed;
    bool bChecked;

    SCheckBox();
    ~SCheckBox() override;

    bool OnKeyDown(int keycode, bool repeat = false) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseOver() override;
    void OnMouseUp(int button, int x, int y, int shift) override;
    void SetFocus() override;
    void Update() override;

    void Create(int a2, int font, unsigned int normal_color, unsigned int active_color, unsigned int disabled_color);
    bool GetCheck();
    char *GetText();
    void SetActive(bool active);
    void SetCheck(bool checked);
    void SetText(const char *text);
};

#endif // WINDOW_SCHECKBOX_H
