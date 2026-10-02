// window/textbutton.h
// STextButton header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_STEXTBUTTON_H
#define WINDOW_STEXTBUTTON_H

#include "dxwidget.h"
#include "string2.h"

struct STextButton : SDXWidget {
    int Font;
    int TextFrame;
    int Align;
    SString Text;
    unsigned int NormalColor;
    unsigned int ActiveColor;
    unsigned int DisabledColor;
    bool Active;
    bool Pressed;
    bool Stuck;

    STextButton();
    ~STextButton() override;

    bool OnKeyDown(int keycode, bool repeat = false) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseOver() override;
    void OnMouseUp(int button, int x, int y, int shift) override;
    void SetFocus() override;
    void Update() override;

    void Create(int a2, int font, int align, unsigned int normal_color, unsigned int active_color, unsigned int disabled_color);
    void SetActive(bool active);
    void SetStuck(bool stuck);
    void SetText(const char *text);
    char *GetText();
    SWidget *GetFocusTargetFromKeyCode(int keycode);
    bool isActive();
};

#endif // WINDOW_STEXTBUTTON_H
