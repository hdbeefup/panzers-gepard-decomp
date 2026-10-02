// window/sliderv.h
// SSliderV header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SSLIDERV_H
#define WINDOW_SSLIDERV_H

#include "dxwidget.h"

struct SSliderV : SDXWidget {
    int SliderFont;
    int ButtonFont;
    int SliderUp;
    int SliderMiddle;
    int SliderDown;
    int SliderBox;
    int ButtonUpFrame;
    int ButtonDownFrame;
    int FirstKlikkTimer;
    int TickTimer;
    int MarginLeft;
    int MarginRight;
    int MarginTop;
    int MarginBottom;
    bool PressedUp;
    bool PressedDown;
    bool ActiveUp;
    bool ActiveDown;
    bool RedBigyoDown;
    int VisibleRows;
    int TopIndex;
    int TopIndexInListBox;
    int NumberOfRows;
    int YSizeOfLittleRedBigyo;
    int YPosOfLittleRedBigyo;
    int XPosRed;
    int XPosLine;
    int DeltaYOfLRB;
    int YDownPosOfLRB;

    SSliderV();
    ~SSliderV() override;

    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseMove(int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseUp(int button, int x, int y, int shift) override;
    void OnTimer(int id, unsigned int time) override;
    void Update() override;

    void Create(int a2, int listwidth, int listheight, int visiblerows);
    bool IsOverDownButton(int x, int y);
    bool IsOverRed(int x, int y);
    bool IsOverUpButton(int x, int y);
    void SetMargin(int left, int right, int top, int bottom);
    void SetRows(int numrows, int topindex);
};

#endif // WINDOW_SSLIDERV_H
