// window/splash.h
// SSplash header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SSPLASH_H
#define WINDOW_SSPLASH_H

#include "dxwidget.h"

enum SplashType : int {
    AssembleLogo = 0,
    SwineHD = 1
};

struct SSplash : SDXWidget {
    SplashType Type;
    int ImageFont;
    int FadeFrame;
    int Duration;
    int FadeDuration;
    int StartTick;
    int EndAction;
    int SkipAction;

    SSplash();
    ~SSplash() override;

    void OnSize(int w, int h) override;
    void Update() override;
    void OnMouseDown(int button, int x, int y, int shift) override;
    bool OnKeyDown(int keycode, bool repeat = false) override;

    void Create(SplashType type);
};

#endif // WINDOW_SSPLASH_H
