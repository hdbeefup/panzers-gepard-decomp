// window/textbutton.cpp
// Text button widget
// Decompiled from: gameSplit/sslider.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "textbutton.h"
#include "logger.h"

// Classes: STextButton
// Function count: 15

//----- (00494FB0) --------------------------------------------------------

STextButton::STextButton()

{
  char *v2; // eax
  size_t v3;
  this->Text.buf = 0;
  this->Text.size = 0;
  this->Text.size = 11;
  v2 = new char[0xCu];
  v3 = this->Text.size + 1;
  this->Text.buf = v2;
  memcpy(v2, "STextButton", v3);
  *(_WORD *)&this->Active = 0;
  this->Stuck = 0;
}

//----- (00495060) --------------------------------------------------------

STextButton::~STextButton()

{
  char *buf; // eax
  buf = this->Text.buf;
  if ( buf )
  {
    delete[] buf;
    this->Text.buf = 0;
  }
  // base destructor called automatically
}

//----- (004950E0) --------------------------------------------------------

void STextButton::Create(int a2, int font, int align, unsigned int normal_color, unsigned int active_color, unsigned int disabled_color)

{
  SDXWidget::Create(a2);
  this->Font = font;
  this->Align = align;
  this->NormalColor = normal_color;
  this->ActiveColor = active_color;
  this->DisabledColor = disabled_color;
  this->TextFrame = Board->CreateFrame(FT_TEXT, this->BackFrame, 0, 0, 0, 1);
  this->Update();
}

//----- (00495150) --------------------------------------------------------

char *STextButton::GetText()

{
  if ( this->Text.buf )
    return this->Text.buf;
  return (char *)"";
}

//----- (00495170) --------------------------------------------------------

bool STextButton::OnKeyDown(int keycode, bool repeat)

{
  SWidget *FocusTargetFromKeyCode; // edi
  if ( Options->GetKeyboardMode() )
  {
    if ( keycode == 13 )
    {
      if (Concert)
        Concert->PlaySound(
          "menu/button_down.wav",
          -12.0f,
          0,
          -1);
      this->SendAction(271682, 0);
      return 1;
    }
  }
  else
  {
    switch ( keycode )
    {
      case 38:
        this->SendAction(324866, 0);
        return 1;
      case 40:
        this->SendAction(324866, 1);
        return 1;
      case 37:
        this->SendAction(324866, 2);
        return 1;
      case 39:
      case 13:
        if ( keycode == 39 )
        {
          this->SendAction(324866, 3);
          return 1;
        }
        return 1;
    }
  }
  if ( keycode != 38 && keycode != 40 && keycode != 37 && keycode != 39 )
    return 0;
  FocusTargetFromKeyCode = this->GetFocusTargetFromKeyCode(keycode);
  if ( FocusTargetFromKeyCode )
  {
    this->Active = 0;
    this->Update();
    this->SendAction(271688, 0);
    FocusTargetFromKeyCode->SetFocus();
  }
  return 1;
}

//----- (004952B0) --------------------------------------------------------

void STextButton::OnMouseDown(int button, int x, int y, int shift)

{
  if ( button == 1 )
  {
    if (Concert)
      Concert->PlaySound(
        "menu/button_down.wav",
        -12.0f,
        0,
        -1);
    this->Pressed = 1;
    this->CaptureMouse();
    this->Update();
    this->SendAction(271681, 0);
  }
  SDXWidget::OnMouseDown(button, x, y, shift);
}

//----- (00495330) --------------------------------------------------------

void STextButton::OnMouseOut()

{
  SDXWidget::OnMouseOut();
  this->Active = 0;
  this->Update();
  this->SendAction(271685, 0);
}

//----- (00495360) --------------------------------------------------------

void STextButton::OnMouseOver()

{
  this->SendAction(324865, 0);
  if ( !this->Active && Concert )
    Concert->PlaySound(
      "menu/button_over.wav",
      -30.0f,
      0,
      -1);
  this->Active = 1;
  this->Update();
  this->SendAction(271683, 0);
}

//----- (004953D0) --------------------------------------------------------

void STextButton::OnMouseUp(int button, int x, int y, int shift)

{
  bool v6; // al
  if ( this->Pressed && button == 1 )
  {
    v6 = x >= 0 && x < this->Width && y >= 0 && y < this->Height;
    this->Active = v6;
    this->Pressed = 0;
    this->ReleaseMouse();
    this->Update();
    if ( this->Active )
      this->SendAction(271682, 0);
  }
}

//----- (00495440) --------------------------------------------------------

void STextButton::SetActive(bool active)

{
  if ( this->Active != active )
  {
    this->Active = active;
    this->Update();
  }
}

//----- (00495460) --------------------------------------------------------

void STextButton::SetFocus()

{
  if (Concert)
    Concert->PlaySound(
      "menu/button_over.wav",
      -30.0f,
      0,
      -1);
  this->Active = 1;
  this->Update();
  this->SendAction(271687, 0);
  SWidget::SetFocus();
}

//----- (004954C0) --------------------------------------------------------

void STextButton::SetStuck(bool stuck)

{
  if ( this->Stuck != stuck )
  {
    this->Stuck = stuck;
    this->Update();
  }
}

//----- (004954E0) --------------------------------------------------------

void STextButton::SetText(const char *text)

{
  this->Text = text;
  this->Update();
}

//----- (00495510) --------------------------------------------------------

void STextButton::Update()

{
  char *buf; // edx
  char *v5; // edx
  int v6;
  int Align;
  int width;
  int height;
  if ( this->BackFrame >= 0 )
  {
    if ( !this->Enabled )
    {
      Board->SetTextColor(this->TextFrame, this->DisabledColor);
      buf = this->Text.buf ? this->Text.buf : (char *)"";
LABEL_5:
      Board->SetText(this->TextFrame, this->Font, 0, buf);
      goto LABEL_19;
    }
    if ( this->Stuck || this->Pressed )
    {
      Board->SetTextColor(this->TextFrame, this->ActiveColor);
      v5 = this->Text.buf ? this->Text.buf : (char *)"";
      v6 = this->Font + 2;
    }
    else
    {
      if ( !this->Active )
      {
        Board->SetTextColor(this->TextFrame, this->NormalColor);
        buf = this->Text.buf ? this->Text.buf : (char *)"";
        goto LABEL_5;
      }
      Board->SetTextColor(this->TextFrame, this->ActiveColor);
      v5 = this->Text.buf ? this->Text.buf : (char *)"";
      v6 = this->Font + 1;
    }
    Board->SetText(this->TextFrame, v6, 0, v5);
LABEL_19:
    Board->GetFrameSize(this->TextFrame, &width, &height);
    Align = this->Align;
    if ( Align )
    {
      if ( Align == 2 )
      {
        this->SetPosition(this->X + this->Width / 2 - width / 2, this->Y, width, height);
      }
      else if ( Align == 1 )
      {
        this->SetPosition(this->X + this->Width - width, this->Y, width, height);
      }
    }
    else
    {
      this->Resize(width, height);
    }
  }
}

//----- (004956D0) --------------------------------------------------------

bool STextButton::isActive()

{
  return this->Active;
}

SWidget *STextButton::GetFocusTargetFromKeyCode(int keycode)
{
  return SWidget::GetFocusTargetFromKeyCode(keycode);
}

