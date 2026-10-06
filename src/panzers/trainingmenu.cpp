// src/panzers/trainingmenu.cpp
// STrainingMenu (trainingmenu.h). OWNER: agent F (docs/M3_INTERFACES.md).
// Skeleton stand-in: a centred SRightMenu with five SComplexButtons. The HD
// dialog (radio buttons, the "Select nation" frame, Start / Cancel side by
// side) is Create 0x63a370, still to lift.

#include "trainingmenu.h"
#include "m3common.h"
#include "stub_log.h"
#include "gettext.h"

STrainingMenu::STrainingMenu()
{
    Nation = 1;                                          // param_1[0xa1] = 1
}

STrainingMenu::~STrainingMenu()
{
}

static void PlaceButton(SRightMenu* menu, SComplexButton* b, int row, bool last, const char* text)
{
    menu->InsertChild(b);
    b->SetPosition((menu->Width - 0x100) / 2, last ? 0x189 : row * 0x31 + 0x32, 0, 0);
    b->Create(2, text);
}

void STrainingMenu::Create()
{
    STUB_LOG("STrainingMenu::Create (0x63a370)");
    PZ_M3_TRACE("STrainingMenu::Create (0x63a370)");
    SRightMenu::Create(GetText("panzers/MainMenu.cpp", "Select nation"), false);
    PlaceButton(this, &Nations[0], 0, false, GetText("panzers/MainMenu.cpp", "German"));
    PlaceButton(this, &Nations[1], 1, false, GetText("panzers/MainMenu.cpp", "Russian"));
    PlaceButton(this, &Nations[2], 2, false, GetText("panzers/MainMenu.cpp", "Allied"));
    PlaceButton(this, &Start, 4, false, GetText("panzers/MainMenu.cpp", "Start"));
    PlaceButton(this, &Cancel, 5, true, GetText("panzers/MainMenu.cpp", "Cancel"));
    // Nation 1 (Allied) is the ctor default; HD checks the matching radio button.
    for (int i = 0; i < 3; ++i) {
        static const int kNation[3] = { 0, 2, 1 };
        Nations[i].Stuck = kNation[i] == Nation;
        Nations[i].Update();
    }
    Cursor = 0;
}

// PANZERS 0x63d410
bool STrainingMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    PZ_M3_TRACE("STrainingMenu::OnAction (0x63d410)");
    if (action != PZA_BUTTON_CLICK)                      // 0x42542
        return false;
    int idx = -1;
    for (int i = 0; i < 3; ++i)
        if (source == &Nations[i])
            idx = i;
    if (idx < 0) {
        if (source == &Start)
            SendAction(PZA_TRAINING_START, 0);           // 0x543930(0x544d1, 0)
        else if (source == &Cancel)
            SendAction(PZA_TRAINING_CANCEL, 0);          // 0x543930(0x544d2, 0)
        return true;
    }
    for (int i = 0; i < 3; ++i) {                        // 0x53ecd0(i == idx) on each radio button
        Nations[i].Stuck = i == idx;
        Nations[i].Update();
    }
    Nation = idx == 0 ? 0 : idx == 1 ? 2 : 1;            // German 0, Russian 2, Allied 1
    return true;
}
