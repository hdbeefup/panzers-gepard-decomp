// window/skippabledxwidget.h
// SSkippableDXWidget header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SSKIPPABLEDXWIDGET_H
#define WINDOW_SSKIPPABLEDXWIDGET_H

#include "dxwidget.h"
#include "textbutton.h"

struct SSkippableDXWidget : SDXWidget {
    int StartTick;
    int SkipVisibleTick;
    int ElapsedMs;
    int SkipVisibleInterval;
    STextButton SkipButton;
    int SkipAction;
    bool ButtonMode;

    SSkippableDXWidget();
    ~SSkippableDXWidget() override;

    bool OnKeyDown(int keycode, bool repeat = false) override;
    void Update() override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    void OnMouseMove(int x, int y, int shift) override;
    bool OnAction(SWidget *sender, int action, int param) override;

    void Create(int a2, int skipAction, bool buttonMode);
    void InsertSkipButton();
    void KickButton();
};

// Global flag: set by skip button click, polled by SSuperWindow::OnIdle
extern bool g_SkipButtonClicked;

#endif // WINDOW_SSKIPPABLEDXWIDGET_H
