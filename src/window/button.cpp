// window/button.cpp
// Button widget
// Decompiled from: gameSplit/swidget.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "button.h"

// Classes: SButton
// Function count: 11

//----- (00482AA0) --------------------------------------------------------

SButton::SButton()

{
  this->SpriteFrame = -1;
  // IDA: *(_DWORD *)&this->Active = 0; — zeros all 4 packed bools
  this->Active = 0;
  this->Pressed = 0;
  this->Stuck = 0;
  this->Special = 0;
}

//----- (00482B10) --------------------------------------------------------

void SButton::Create(int a2, int font, int normal, int pressed, int active, int special)

{
  SDXWidget::Create(a2);
  this->Font = font;
  this->NormalGlyph = normal;
  this->ActiveGlyph = active;
  this->PressedGlyph = pressed;
  this->SpecialGlyph = special;
  this->SpriteFrame = Board->CreateFrame(FT_SPRITE, this->BackFrame, 0, 0, 0, 0);
  this->SetState();
}

//----- (00482BE0) --------------------------------------------------------

void SButton::OnMouseDown(int button, int x, int y, int shift)

{
  SDXWidget::OnMouseDown(button, x, y, shift);
  if ( button == 1 )
  {
    Concert->PlaySound(
      "menu/button_down.wav",
      -12.0f,
      0,
      -1);
    this->Pressed = 1;
    this->CaptureMouse();
    this->SetState();
    this->SendAction(271681, 0);
  }
  else if ( button == 3 )
  {
    this->SendAction(271684, 0);
  }
}

//----- (00482C70) --------------------------------------------------------

void SButton::OnMouseOut()

{
  this->Active = 0;
  this->SetState();
  SDXWidget::OnMouseOut();
  this->SendAction(271685, 0);
}

//----- (00482CA0) --------------------------------------------------------

void SButton::OnMouseOver()

{
  this->SendAction(324865, 0);
  Concert->PlaySound(
    "menu/button_over.wav",
    -30.0f,
    0,
    -1);
  this->Active = 1;
  this->SetState();
  this->SendAction(271683, 0);
}

//----- (00482D00) --------------------------------------------------------

void SButton::OnMouseUp(int button, int x, int y, int shift)

{
  bool v6; // al
  if ( this->Pressed && button == 1 )
  {
    v6 = x >= 0 && x < this->Width && y >= 0 && y < this->Height;
    this->Active = v6;
    this->Pressed = 0;
    this->ReleaseMouse();
    this->SetState();
    if ( this->Active )
      this->SendAction(271682, 0);
  }
}

//----- (00482DC0) --------------------------------------------------------

void SButton::SetActive(bool active)

{
  if ( this->Active != active )
  {
    this->Active = active;
    this->SetState();
  }
}

//----- (00482DE0) --------------------------------------------------------

void SButton::SetGlyphs(int font, int normal, int pressed, int active, int special)

{
  this->Font = font;
  this->NormalGlyph = normal;
  this->ActiveGlyph = active;
  this->PressedGlyph = pressed;
  this->SpecialGlyph = special;
  this->SetState();
}

//----- (00482E20) --------------------------------------------------------

void SButton::SetSpecial(bool special)

{
  if ( this->Special != special )
  {
    this->Special = special;
    this->SetState();
  }
}

//----- (00482E40) --------------------------------------------------------

void SButton::SetState()

{
  int ActiveGlyph;
  int width;
  int height;
  if ( this->BackFrame >= 0 )
  {
    if ( this->Stuck || this->Pressed )
    {
      Board->SetSpriteGlyph(this->SpriteFrame, this->Font, this->PressedGlyph);
    }
    else if ( this->Active && (ActiveGlyph = this->ActiveGlyph, ActiveGlyph >= 0)
           || this->Special && (ActiveGlyph = this->SpecialGlyph, ActiveGlyph >= 0) )
    {
      Board->SetSpriteGlyph(this->SpriteFrame, this->Font, ActiveGlyph);
    }
    else
    {
      Board->SetSpriteGlyph(this->SpriteFrame, this->Font, this->NormalGlyph);
    }
    Board->GetFrameSize(this->SpriteFrame, &width, &height);
    this->Resize(width, height);
  }
}

//----- (00482EE0) --------------------------------------------------------

void SButton::SetStuck(bool stuck)

{
  if ( this->Stuck != stuck )
  {
    this->Stuck = stuck;
    this->SetState();
  }
}

