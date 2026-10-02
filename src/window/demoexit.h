// window/demoexit.h
// SDemoExit — click-to-advance quit cycle (2001 demo build style)
// Non-original: gated behind HD_DEMO_EXIT_SCREENS

#ifndef WINDOW_DEMOEXIT_H
#define WINDOW_DEMOEXIT_H

#include "hdbeefup.h"

#ifdef HD_DEMO_EXIT_SCREENS

#include "dxwidget.h"

struct SDemoExit : SDXWidget {
    int ImageFonts[4];
    int CurrentIndex;
    int DoneAction;

    SDemoExit();
    ~SDemoExit() override;

    void OnMouseDown(int button, int x, int y, int shift) override;
    bool OnKeyDown(int keycode, bool repeat = false) override;

    void Create(int doneAction);

private:
    void Advance();
};

#endif // HD_DEMO_EXIT_SCREENS
#endif // WINDOW_DEMOEXIT_H
