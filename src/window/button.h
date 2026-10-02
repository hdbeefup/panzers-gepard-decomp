// window/button.h
// SButton header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SBUTTON_H
#define WINDOW_SBUTTON_H

#include "dxwidget.h"

struct SButton : SDXWidget {
    int Font;
    int NormalGlyph;
    int ActiveGlyph;
    int PressedGlyph;
    int SpecialGlyph;
    int SpriteFrame;
    bool Active;
    bool Pressed;
    bool Stuck;
    bool Special;

    SButton();
    ~SButton() override = default;

    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseOver() override;
    void OnMouseUp(int button, int x, int y, int shift) override;

    void Create(int a2, int font, int normal, int pressed, int active, int special);
    void SetActive(bool active);
    void SetGlyphs(int font, int normal, int pressed, int active, int special);
    void SetSpecial(bool special);
    void SetState();
    void SetStuck(bool stuck);
};

#endif // WINDOW_SBUTTON_H
