// window/splash.cpp
// Splash screen
// Decompiled from: gameSplit/swindow.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "splash.h"
#include "logger.h"
#include "timer.h"

// Classes: SSplash
// Function count: 7

//----- (004943A0) --------------------------------------------------------

SSplash::~SSplash()

{
  int ImageFont;
  ImageFont = this->ImageFont;
  Board->ReleaseFont(ImageFont);
  Board->DestroyFrame(this->FadeFrame);
  // base destructor called automatically
}

//----- (004944A0) --------------------------------------------------------

void SSplash::Create(SplashType type)

{
  SWidget *Parent; // ecx
  int v5;
  int v6;
  int v7;
  int v8;
  bool v9;
  bool v10; // sf
  int TickValue;
  SWidget *v12; // ecx
  int v13;
  int v14;
  int v15;
  int p_x;
  int p_y;
  int p_width;
  int p_height;
  int v20;
  int duration;
  Parent = this->Parent;
  this->Type = type;
  Parent->GetPosition(&p_x, &p_y, &p_width, &p_height);
  this->SetPosition((p_width - 800) / 2, (p_height - 600) / 2, 800, 600);
  this->SetGravity(5);
  if ( type )
  {
    if ( type != SwineHD )
      Logger.g->Panic("Invalid splash type");
    v5 = -1;
    v20 = 0;
    v6 = 341251;
    duration = 4500;
    v7 = Board->LoadSingleFont("menu/splash.png", FullHD_Shift);
  }
  else
  {
    v5 = 341250;
    v20 = 1000;
    duration = 3500;
    v6 = 341249;
    v7 = Board->LoadSingleFont("menu/assemble_logo.png", FullHD_Shift);
  }
  this->ImageFont = v7;
  this->Duration = duration;
  this->FadeDuration = v20;
  this->EndAction = v6;
  this->SkipAction = v5;
  this->SetBackgroundSprite(this->ImageFont, 0, 1, 0);
  Gepard->SetFog(0, 0, 0, 0, 0);
  Board->SetCursor(-1, 0, 0);
  SDXWidget::Create(v6);
  v8 = Board->CreateFrame(FT_BOX, this->BackFrame, 0, 0, 5, 1);
  v9 = this->FadeDuration == 0;
  v10 = this->FadeDuration < 0;
  this->FadeFrame = v8;
  Board->ShowFrame(v8, !v10 && !v9);
  Board->SetBoxColor(this->FadeFrame, 0xFF000000);
  TickValue = (unsigned int)Timer.GetTickValue();

  v12 = this->Parent;
  this->StartTick = TickValue;
  int frameW, frameH;
  v12->GetPosition(&v14, &v15, &frameW, &frameH);
  v13 = frameW - 800;
  Board->ResizeFrame(this->FadeFrame, frameW, frameH);
  Board->MoveFrame(this->FadeFrame, v13 / -2, 0);
  this->SetFocus();
}

//----- (00494720) --------------------------------------------------------

void SSplash::OnSize(int w, int h)

{
  int v2;
  int p_x;
  int p_y;
  int p_height;
  int p_width;
  this->Parent->GetPosition(&p_x, &p_y, &p_width, &p_height);
  v2 = p_width - 800;
  Board->ResizeFrame(this->FadeFrame, p_width, p_height);
  Board->MoveFrame(this->FadeFrame, v2 / -2, 0);
}

//----- (00494790) --------------------------------------------------------

void SSplash::Update()

{
  int v2;
  int FadeDuration;
  int v4;
  int v5;
  v2 = (unsigned int)(Timer.GetTickValue() - this->StartTick);

  if ( v2 < this->Duration )
  {
    Board->SetBoxColor(this->FadeFrame, 0x80FF0000);
    FadeDuration = this->FadeDuration;
    v4 = 0;
    if ( v2 >= FadeDuration )
    {
      v5 = this->Duration - FadeDuration;
      if ( v2 > v5 )
        v4 = (int)(float)((float)((float)(v2 - v5) / (float)FadeDuration) * 255.0);
    }
    else
    {
      v4 = (int)(float)((float)(1.0 - (float)((float)v2 / (float)FadeDuration)) * 255.0);
    }
    Board->SetBoxColor(this->FadeFrame, v4 << 24);
  }
  else
  {
    this->SendAction(this->EndAction, 0);
  }
}

//----- (00494360) --------------------------------------------------------

SSplash::SSplash()

{
  this->FadeFrame = -1;
  this->ImageFont = -1;
  this->ToolTipFeatureEnabled = 0;
}

//----- (004946F0) --------------------------------------------------------

void SSplash::OnMouseDown(int button, int x, int y, int shift)

{
  int SkipAction;
  SDXWidget::OnMouseDown(button, x, y, shift);
  SkipAction = this->SkipAction;
  if ( SkipAction >= 0 )
    this->SendAction(SkipAction, 0);
}

//----- (004946C0) --------------------------------------------------------

bool SSplash::OnKeyDown(int keycode, bool repeat)

{
  if ( keycode != 13 && keycode != 27 )
    return 0;
  this->SendAction(this->SkipAction, 0);
  return 1;
}

