// src/panzers/trainingmenu.h
// STrainingMenu: the Training Camp "Select nation" dialog. HD object 0x288
// bytes, ctor 0x633b30, Create 0x63a370 ("Select nation", German, Russian,
// Allied, Start, Cancel), vftable 0x806f44: +0x00 dtor 0x634cf0 (1),
// +0x44 OnAction 0x63d410 (3) [m3: mkt]; all other slots SDXWidget's.
//
// HD layout: SDXWidget (0x58), SRadioButton[3] at +0x58 (stride 0x6c,
// 0x53e9f0), SComplexButton Start +0x19c and Cancel +0x210 (0x538640),
// Nation +0x284 (ctor 1).
// OnAction 0x63d410 (on 0x42542 clicks only): a radio button checks itself
// (0x53ecd0 on all three) and sets Nation: German 0, Russian 2, Allied 1
// (= SPanzersCampaign::Race); Start sends 0x544d1, Cancel 0x544d2.
// SSuperWindow::OnAction 0x4d4d4 creates it into +0x118 (TrainingCampMenu),
// Create, then 0x5435b0 / 0x544fe0 (modal).
//
// SHARED HEADER (owner P0; body: agent F, src/panzers/trainingmenu.cpp).

#ifndef PANZERS_TRAININGMENU_H
#define PANZERS_TRAININGMENU_H

#include "mainmenu.h"
#include "radiobutton.h"

enum PzTrainingAction {
    PZA_TRAINING_START  = 0x544d1,
    PZA_TRAINING_CANCEL = 0x544d2,
};

struct STrainingMenu : SDXWidget {
    SRadioButton   Nations[3];   // HD +0x58 (stride 0x6c, 0x53e9f0): German, Russian, Allied
    SComplexButton Start;        // HD +0x19c
    SComplexButton Cancel;       // HD +0x210
    int            Nation;       // HD +0x284 (ctor 1 = Allied)

    STrainingMenu();                                                 // 0x633b30
    ~STrainingMenu() override;                                       // 0x634cf0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x63d410
    void Create();                                                   // 0x63a370
};

#endif // PANZERS_TRAININGMENU_H
