// window/textbox.h
// STextBox header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_STEXTBOX_H
#define WINDOW_STEXTBOX_H

#include "darray.h"
#include "dxwidget.h"
#include "sliderv.h"

enum SHorizontalAlign : int {
    ALIGN_LEFT = 0,
    ALIGN_RIGHT = 1,
    ALIGN_CENTER = 2,
};

enum SVerticalAlign : int {
    Top = 0,
    Middle = 1,
    Bottom = 2,
};

struct STextBox : SDXWidget {
    int Font;
    int TopIndex;
    int VisibleTextLines;
    int MaxInnerTextLines;
    SDArray<int> TextFrames;
    bool Scrollbars;
    SSliderV Slider;
    SHorizontalAlign horizontalAlign;
    SVerticalAlign verticalAlign;
    bool alignverticalcenter;

    STextBox();
    ~STextBox() override;

    void Update() override;

    void Create(int font, unsigned char NumberOfTextLines, char bBackGround, int bScrollbars, SHorizontalAlign hAlign, SVerticalAlign vAlign);
    int GetRowCount();
    int GetTopIndex();
    void ResetContent();
    int SetText(const char *text);
};

#endif // WINDOW_STEXTBOX_H
