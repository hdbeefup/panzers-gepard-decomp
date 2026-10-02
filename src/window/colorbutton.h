// window/colorbutton.h
// SColorButton header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SCOLORBUTTON_H
#define WINDOW_SCOLORBUTTON_H

#include "dxwidget.h"

struct SColorButton : SDXWidget {
    int ColorFrame;
    int ColorButtonBackGroundFont;
    int ColorButtonBackGroundFrame;
    bool Active;
    bool Pressed;
    __int16 ColorIndex;

    SColorButton();
    ~SColorButton() override;

    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseOver() override;
    void OnMouseUp(int button, int x, int y, int shift) override;
    void Update() override;

    void Create(int a2);
    unsigned char GetColorIndex();
    void SetColor(unsigned char NewColorIndex);
};

#endif // WINDOW_SCOLORBUTTON_H
