// window/colorbutton.cpp
// Color button widget
// Decompiled from: gameSplit/swidget.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "colorbutton.h"

extern unsigned int PlayerColors[13];

// Classes: SColorButton
// Function count: 10

//----- (00483770) --------------------------------------------------------

SColorButton::SColorButton()

{
  this->Active = 0;
  this->ColorButtonBackGroundFont = -1;
}

//----- (004837A0) --------------------------------------------------------

SColorButton::~SColorButton()

{
  int ColorButtonBackGroundFont;
  ColorButtonBackGroundFont = this->ColorButtonBackGroundFont;
  Board->ReleaseFont(ColorButtonBackGroundFont);
  // base destructor called automatically
}

//----- (00483880) --------------------------------------------------------

void SColorButton::Create(int a2)

{
  int v3;
  int v4;
  short v5; // ax
  int Height;
  SDXWidget::Create(a2);
  this->ColorButtonBackGroundFont = Board->LoadFixedFont("menu/colorbutton_background.png", 22, 24, 2, 2, 0, X2);
  v3 = Board->CreateFrame(FT_SPRITE, this->BackFrame, -3, -4, 0, 1);
  this->ColorButtonBackGroundFrame = v3;
  Board->SetSpriteGlyph(v3, this->ColorButtonBackGroundFont, 0);
  v4 = Board->CreateFrame(FT_BOX, this->BackFrame, 0, 0, 0, 1);
  Height = this->Height;
  this->ColorFrame = v4;
  Board->ResizeFrame(v4, this->Width, Height);
  v5 = LOBYTE(this->ColorIndex) % 0xDu;
  this->ColorIndex = v5;
  Board->SetBoxColor(this->ColorFrame, PlayerColors[v5]);
  this->Update();
  this->Update();
}

//----- (00483960) --------------------------------------------------------

unsigned char SColorButton::GetColorIndex()

{
  return (unsigned char)this->ColorIndex;
}

//----- (00483970) --------------------------------------------------------

void SColorButton::OnMouseDown(int button, int x, int y, int shift)

{
  if ( button == 1 || button == 3 )
  {
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
}

//----- (004839E0) --------------------------------------------------------

void SColorButton::OnMouseOut()

{
  this->Active = 0;
  this->Update();
  SDXWidget::OnMouseOut();
}

//----- (00483A00) --------------------------------------------------------

void SColorButton::OnMouseOver()

{
  Concert->PlaySound(
    "menu/button_over.wav",
    -30.0f,
    0,
    -1);
  this->Active = 1;
  this->Update();
  this->SendAction(271683, 0);
}

//----- (00483A50) --------------------------------------------------------

void SColorButton::OnMouseUp(int button, int x, int y, int shift)

{
  bool v6; // al
  bool Active; // al
  if ( this->Pressed && (button == 1 || button == 3) )
  {
    v6 = x >= 0 && x < this->Width && y >= 0 && y < this->Height;
    this->Active = v6;
    this->Pressed = 0;
    this->ReleaseMouse();
    this->Update();
    Active = this->Active;
    if ( Active )
    {
      if ( button == 1 )
      {
        this->SendAction(271682, 0);
        Active = this->Active;
      }
      if ( Active && button == 3 )
        this->SendAction(271686, 0);
    }
  }
}

//----- (00483AF0) --------------------------------------------------------

void SColorButton::SetColor(unsigned char NewColorIndex)

{
  short v3; // ax
  v3 = NewColorIndex % 0xDu;
  this->ColorIndex = v3;
  Board->SetBoxColor(this->ColorFrame, PlayerColors[v3]);
  this->Update();
}

//----- (00483B40) --------------------------------------------------------

void SColorButton::Update()

{
  if ( this->BackFrame >= 0 )
  {
    if ( this->Pressed )
      Board->MoveFrame(this->ColorFrame, 1, 1);
    else
      Board->MoveFrame(this->ColorFrame, 0, 0);
    Board->SetSpriteGlyph(
      this->ColorButtonBackGroundFrame,
      this->ColorButtonBackGroundFont,
      0);
  }
}

