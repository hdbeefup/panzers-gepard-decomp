// window/demoexit.cpp
// SDemoExit — click-to-advance quit cycle (2001 demo build style)
// Non-original: gated behind HD_DEMO_EXIT_SCREENS

#include "demoexit.h"

#ifdef HD_DEMO_EXIT_SCREENS

#include "iboard.h"

extern SIBoard *Board;

SDemoExit::SDemoExit()
{
    for (int i = 0; i < 4; ++i)
        ImageFonts[i] = -1;
    CurrentIndex = 0;
    DoneAction = -1;
    ToolTipFeatureEnabled = 0;
}

SDemoExit::~SDemoExit()
{
    for (int i = 0; i < 4; ++i) {
        if (ImageFonts[i] >= 0)
            Board->ReleaseFont(ImageFonts[i]);
    }
}

void SDemoExit::Create(int doneAction)
{
    DoneAction = doneAction;
    CurrentIndex = 0;

    SWidget *parent = this->Parent;
    int px, py, pw, ph;
    parent->GetPosition(&px, &py, &pw, &ph);
    SetPosition((pw - 800) / 2, (ph - 600) / 2, 800, 600);
    SetGravity(5);

    ImageFonts[0] = Board->LoadSingleFont("menu/exit1.png", FullHD_Shift);
    ImageFonts[1] = Board->LoadSingleFont("menu/exit2.png", FullHD_Shift);
    ImageFonts[2] = Board->LoadSingleFont("menu/exit3.png", FullHD_Shift);
    ImageFonts[3] = Board->LoadSingleFont("menu/exit4.png", FullHD_Shift);

    SetBackgroundSprite(ImageFonts[0], 0, 1, 0);
    Board->SetCursor(-1, 0, 0);
    SDXWidget::Create(0);
    SetFocus();
}

void SDemoExit::Advance()
{
    CurrentIndex++;
    if (CurrentIndex >= 4) {
        if (DoneAction >= 0)
            SendAction(DoneAction, 0);
        return;
    }
    Board->SetSpriteGlyph(this->BackFrame, ImageFonts[CurrentIndex], 0);
}

void SDemoExit::OnMouseDown(int button, int x, int y, int shift)
{
    SDXWidget::OnMouseDown(button, x, y, shift);
    Advance();
}

bool SDemoExit::OnKeyDown(int keycode, bool repeat)
{
    if (keycode != 13 && keycode != 27)
        return false;
    Advance();
    return true;
}

#endif // HD_DEMO_EXIT_SCREENS
