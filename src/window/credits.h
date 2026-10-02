// window/credits.h
// SCredits header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SCREDITS_H
#define WINDOW_SCREDITS_H

#include "darray.h"
#include "skippabledxwidget.h"
#include "textbox.h"

struct SProperties;

struct SCredits : SSkippableDXWidget {
    SProperties *CreditsIni;
    SDXWidget ContentParent;
    SDArray<SDXWidget *> Widgets;
    SDArray<int> ImageFonts;
    int BottomGradientFont;
    int TopGradientFont;
    int BottomGradientBackground;
    int TopGradientBackground;
    int BottomFillBackground;
    int TopFillBackground;
    float ContentHeight;
    bool AutoScroll;
    SDArray<char *> SpeechPaths;
    int SpeechBeginDelay;
    int SpeechEndDelay;
    float MusicDuration;
    int SpeechInterval;
    int NextSpeechTime;
    int NextSpeechIndex;

    SCredits();
    ~SCredits() override;

    bool OnKeyDown(int keycode, bool repeat = false) override;
    bool OnKeyUp(int keycode) override;
    void OnSize(int w, int h) override;
    void Update() override;

    void Create(int a2);
};

#endif // WINDOW_SCREDITS_H
