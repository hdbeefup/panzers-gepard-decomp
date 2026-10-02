// window/introduction.h
// SIntroduction header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SINTRODUCTION_H
#define WINDOW_SINTRODUCTION_H

#include "listbox.h"
#include "skippabledxwidget.h"

struct SProperties;

struct SIntroduction : SSkippableDXWidget {
    SProperties *IntroTextsIni;
    SListBox TextListBox;
    int TextHeight;
    int BottomGradientFont;
    int TopGradientFont;
    int BottomGradientBackground;
    int TopGradientBackground;
    int BottomFillBackground;
    int TopFillBackground;
    float AudioDuration;

    SIntroduction();
    ~SIntroduction() override;

    void OnSize(int w, int h) override;
    void Update() override;

    void Create(int a2, int race);
};

#endif // WINDOW_SINTRODUCTION_H
