// window/checkbox.cpp
// Checkbox widget
// Decompiled from: gameSplit/swidget.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "checkbox.h"

// Classes: SCheckBox
// Function count: 15

//----- (00482F10) --------------------------------------------------------

SCheckBox::SCheckBox()

{
  char *v2; // eax
  size_t v3;
  this->Text.buf = 0;
  this->Text.size = 0;
  this->Text.size = 9;
  v2 = new char[0xAu];
  v3 = this->Text.size + 1;
  this->Text.buf = v2;
  memcpy(v2, "SCheckBox", v3);
  *(_WORD *)&this->Active = 0;
  this->bChecked = 0;
}

//----- (00482FE0) --------------------------------------------------------

SCheckBox::~SCheckBox()

{
  char *buf; // eax
  int CheckFont;
  CheckFont = this->CheckFont;
  Board->ReleaseFont(CheckFont);
  Board->ReleaseFont(this->UnCheckFont);
  Board->ReleaseFont(this->CheckFontGrey);
  Board->ReleaseFont(this->UnCheckFontGrey);
  buf = this->Text.buf;
  if ( buf )
  {
    delete[] buf;
    this->Text.buf = 0;
  }
  // base destructor called automatically
}

//----- (00483160) --------------------------------------------------------

void SCheckBox::Create(int a2, int font, unsigned int normal_color, unsigned int active_color, unsigned int disabled_color)

{
  int v7;
  int BackFrame;
  int v9;
  int v10;
  SDXWidget::Create(a2);
  this->NormalColor = normal_color;
  this->ActiveColor = active_color;
  this->Font = font;
  this->DisabledColor = disabled_color;
  v7 = Board->CreateFrame((SFrameType)2, this->BackFrame, 20, 0, 0, 0);
  if ( font )
  {
    v10 = -9;
    v9 = -4;
  }
  else
  {
    v10 = -12;
    v9 = -3;
  }
  BackFrame = this->BackFrame;
  this->TextFrame = v7;
  this->CheckFrame = Board->CreateFrame(FT_SPRITE, BackFrame, v9, v10, 0, 1);
  this->CheckFont = Board->LoadSingleFont("menu/check2_kicsi.png", X2);
  this->UnCheckFont = Board->LoadSingleFont("menu/check_kicsi.png", X2);
  this->CheckFontGrey = Board->LoadSingleFont("menu/check2_kicsi_szurke.png", X2);
  this->UnCheckFontGrey = Board->LoadSingleFont("menu/check_kicsi_szurke.png", X2);
  this->Update();
}

//----- (00483250) --------------------------------------------------------

bool SCheckBox::GetCheck()

{
  return this->bChecked;
}

//----- (00483260) --------------------------------------------------------

char *SCheckBox::GetText()

{
  if ( this->Text.buf )
    return this->Text.buf;
  return (char *)"";
}

//----- (00483280) --------------------------------------------------------

bool SCheckBox::OnKeyDown(int keycode, bool repeat)

{
  SWidget *Up; // ecx
  if ( !Options->GetKeyboardMode() )
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
      case 39:
      case 13:
        return 1;
    }
  }
  switch ( keycode )
  {
    case 38:
      Up = this->Up;
      if ( Up )
      {
        if ( this->Active )
        {
          this->Active = 0;
          this->Update();
          this->Up->SetFocus();
          return 1;
        }
        goto LABEL_17;
      }
      return 1;
    case 40:
      Up = this->Down;
      if ( Up )
      {
        if ( this->Active )
        {
          this->Active = 0;
          this->Update();
          Up = this->Down;
        }
LABEL_17:
        Up->SetFocus();
      }
      return 1;
    case 13:
    case 32:
      Concert->PlaySound(
        "menu/radiobutton.wav",
        -12.0f,
        0,
        -1);
      this->bChecked = !this->bChecked;
      this->Update();
      break;
  }
  return 0;
}

//----- (004833B0) --------------------------------------------------------

void SCheckBox::OnMouseDown(int button, int x, int y, int shift)

{
  if ( button == 1 )
  {
    this->Pressed = 1;
    this->CaptureMouse();
    this->Update();
    this->SendAction(271681, 0);
  }
  SDXWidget::OnMouseDown(button, x, y, shift);
}

//----- (00483400) --------------------------------------------------------

void SCheckBox::OnMouseOut()

{
  this->Active = 0;
  this->Update();
  SDXWidget::OnMouseOut();
}

//----- (00483420) --------------------------------------------------------

void SCheckBox::OnMouseOver()

{
  this->SendAction(324865, 0);
  Concert->PlaySound(
    "menu/button_over.wav",
    -30.0f,
    0,
    -1);
  this->Active = 1;
  this->Update();
}

//----- (00483470) --------------------------------------------------------

void SCheckBox::OnMouseUp(int button, int x, int y, int shift)

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
    {
      Concert->PlaySound(
        "menu/radiobutton.wav",
        -12.0f,
        0,
        -1);
      this->bChecked = !this->bChecked;
      this->Update();
      this->SendAction(271682, 0);
    }
  }
}

//----- (00483530) --------------------------------------------------------

void SCheckBox::SetActive(bool active)

{
  if ( this->Active != active )
  {
    this->Active = active;
    this->Update();
  }
}

//----- (00483550) --------------------------------------------------------

void SCheckBox::SetCheck(bool checked)

{
  this->bChecked = checked;
  this->Update();
}

//----- (00483570) --------------------------------------------------------

void SCheckBox::SetFocus()

{
  Concert->PlaySound(
    "menu/button_over.wav",
    -30.0f,
    0,
    -1);
  if ( !this->Active )
  {
    this->Active = 1;
    this->Update();
  }
  SWidget::SetFocus();
}

//----- (004835C0) --------------------------------------------------------

void SCheckBox::SetText(const char *text)

{
  this->Text = text;
  this->Update();
}

//----- (004835F0) --------------------------------------------------------

void SCheckBox::Update()

{
  char *v3; // edx
  char *v5; // edx
  char *buf; // edx
  int width;
  int height;
  if ( this->BackFrame >= 0 )
  {
    if ( this->Enabled )
    {
      if ( this->bChecked )
        Board->SetSpriteGlyph(this->CheckFrame, this->CheckFont, 0);
      else
        Board->SetSpriteGlyph(this->CheckFrame, this->UnCheckFont, 0);
      if ( this->Pressed || this->Active )
      {
        Board->SetTextColor(this->TextFrame, this->ActiveColor);
        buf = this->Text.buf ? this->Text.buf : (char *)"";
        Board->SetText(this->TextFrame, this->Font + 1, 0, buf);
      }
      else
      {
        Board->SetTextColor(this->TextFrame, this->NormalColor);
        v5 = this->Text.buf ? this->Text.buf : (char *)"";
        Board->SetText(this->TextFrame, this->Font, 0, v5);
      }
    }
    else
    {
      Board->SetTextColor(this->TextFrame, this->DisabledColor);
      v3 = this->Text.buf ? this->Text.buf : (char *)"";
      Board->SetText(this->TextFrame, this->Font, 0, v3);
      if ( this->bChecked )
        Board->SetSpriteGlyph(this->CheckFrame, this->CheckFontGrey, 0);
      else
        Board->SetSpriteGlyph(this->CheckFrame, this->UnCheckFontGrey, 0);
    }
    Board->GetFrameSize(this->TextFrame, &width, &height);
    this->Resize(width + 20, height);
  }
}

