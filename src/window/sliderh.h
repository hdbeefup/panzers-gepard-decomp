// window/sliderh.h
// SSliderH header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SSLIDERH_H
#define WINDOW_SSLIDERH_H

#include "dxwidget.h"

struct SSliderH : SDXWidget {
    int SliderFont;
    int ButtonFont;
    int SliderFrame;
    int SliderBox;
    int ButtonLeftFrame;
    int ButtonRightFrame;
    int FirstKlikkTimer;
    int TickTimer;
    int NumberOfFixPos;
    int SliderPos;
    int *SliderRovatka;
    bool PressedLeft;
    bool PressedRight;
    bool ActiveLeft;
    bool ActiveRight;
    bool RedBigyoDown;
    int XPosOfLittleRedBigyo;
    int DeltaXOfLRB;
    int XDownPosOfLRB;

    SSliderH();
    ~SSliderH() override;

    bool OnAction(SWidget *sender, int action, int param) override;
    bool OnKeyDown(int keycode, bool repeat = false) override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseMove(int x, int y, int shift) override;
    void OnMouseOut() override;
    void OnMouseOver() override;
    void OnMouseUp(int button, int x, int y, int shift) override;
    void OnTimer(int id, unsigned int time) override;
    void SetFocus() override;
    void Update() override;

    void Create(int posx, int posy, int width, int numberoffixpos, int sliderpos);
    int GetNumberOfFixPos();
    int GetSliderPos();
    void SetActive(bool active);
};

#endif // WINDOW_SSLIDERH_H
