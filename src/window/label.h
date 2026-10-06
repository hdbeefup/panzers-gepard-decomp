// window/label.h
// SLabel header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SLABEL_H
#define WINDOW_SLABEL_H

#include "dxwidget.h"
#include "string2.h"

struct SLabel : SDXWidget {
    int Font;
    int TextFrame;
    SString Text;
    unsigned int Color;
    int Align;            // HD +0x6c: 0 left, 1 right (x = Width), passed to SetText

    SLabel();
    ~SLabel() override;

    void Resize(int width, int height) override;
    void SetPosition(int x, int y, int width, int height) override;
    void Update() override;

    void Create(int a2, int font);
    char *GetText();
    void SetText(const char *text);
    void SetTextColor(unsigned int color);
    void SetTextF(const char *format, ...);
    void SetTextV(const char *format, char *args);
};

#endif // WINDOW_SLABEL_H
