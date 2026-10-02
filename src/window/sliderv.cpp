// window/sliderv.cpp
// Vertical slider
// Decompiled from: gameSplit/sslider.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

extern unsigned char g_MenuRace;
#include "sliderv.h"

// Classes: SSliderV
// Function count: 14

//----- (00493790) --------------------------------------------------------

SSliderV::SSliderV()

{
  this->PressedUp = 0;
  this->ButtonFont = -1;
  this->SliderFont = -1;
  this->TickTimer = -1;
  this->FirstKlikkTimer = -1;
  this->NumberOfRows = 0;
  this->TopIndex = 0;
  this->TopIndexInListBox = 0;
  this->YPosOfLittleRedBigyo = 0;
  this->YSizeOfLittleRedBigyo = 0;
  this->DeltaYOfLRB = 0;
  this->RedBigyoDown = 0;
  this->MarginBottom = 6;
  this->MarginTop = 6;
  this->MarginRight = 6;
  this->MarginLeft = 6;
}

//----- (00493850) --------------------------------------------------------

SSliderV::~SSliderV()

{
  bool v2; // sf
  v2 = this->BackFrame < 0;
  if ( !v2 )
  {
    Board->ReleaseFont(this->SliderFont);
    Board->ReleaseFont(this->ButtonFont);
  }
  this->KillTimer(&this->FirstKlikkTimer);
  this->KillTimer(&this->TickTimer);
  // base destructor called automatically
}

//----- (00493990) --------------------------------------------------------

void SSliderV::Create(int a2, int listwidth, int listheight, int visiblerows)

{
  int v6;
  int v8;
  int v9;
  int v10;
  int v11;
  int v12;
  int v13;
  int v14;
  SCustomGlyph csuszka_glyphs[3];
  this->SetPosition(
    listwidth - this->MarginRight - 22,
    this->MarginTop,
    22,
    listheight - this->MarginBottom - this->MarginTop);
  SDXWidget::Create(a2);
  csuszka_glyphs[1].X = 1;
  csuszka_glyphs[0].Height = 0;
  this->VisibleRows = visiblerows;
  this->ButtonUpFrame = Board->CreateFrame(
                          FT_SPRITE,
                          this->BackFrame,
                          0,
                          0,
                          csuszka_glyphs[0].Height,
                          csuszka_glyphs[1].X);
  v6 = Board->CreateFrame(FT_SPRITE, this->BackFrame, 0, this->Height - 22, 0, 1);
  csuszka_glyphs[1].X = 1;
  csuszka_glyphs[0].Height = 0;
  this->ButtonDownFrame = v6;
  csuszka_glyphs[0].Width = 12;
  csuszka_glyphs[0].Y = 6;
  csuszka_glyphs[0].X = 22;
  if ( g_MenuRace )
    v8 = Board->LoadFixedFont("menu/pig_arrows_medium.png", 21, 22, 6, 12, NULL, X2);
  else
    v8 = Board->LoadFixedFont("menu/rabbit_arrows_medium.png", 21, 22, 6, 12, NULL, X2);
  this->ButtonFont = v8;
  v9 = this->Width / 2 - 2;
  this->XPosLine = v9;
  v10 = Board->CreateFrame(FT_BOX, this->BackFrame, v9, 21, 0, 0);
  this->SliderBox = v10;
  Board->SetBoxColor(v10, 0xFF848484);
  Board->ResizeFrame(this->SliderBox, 2, this->Height - 42);
  // Glyph definitions for csuszka.png (slider knob: top cap, middle, bottom cap)
  // Original data at 0x54CCC0, 0x54CCD0, 0x54CCE0
  csuszka_glyphs[0].X = 0;  csuszka_glyphs[0].Y = 0;  csuszka_glyphs[0].Width = 11; csuszka_glyphs[0].Height = 4;
  csuszka_glyphs[1].X = 0;  csuszka_glyphs[1].Y = 4;  csuszka_glyphs[1].Width = 11; csuszka_glyphs[1].Height = 4;
  csuszka_glyphs[2].X = 0;  csuszka_glyphs[2].Y = 8;  csuszka_glyphs[2].Width = 11; csuszka_glyphs[2].Height = 4;
  this->SliderFont = Board->LoadCustomFont("menu/csuszka.png", 3, csuszka_glyphs, X2);
  v11 = this->Width / 2 - 6;
  this->XPosRed = v11;
  v12 = Board->CreateFrame(FT_SPRITE, this->BackFrame, v11, 30, 0, 1);
  this->SliderUp = v12;
  Board->SetSpriteGlyph(v12, this->SliderFont, 0);
  v13 = Board->CreateFrame(FT_SPRITE, this->BackFrame, this->XPosRed, 49, 0, 1);
  this->SliderMiddle = v13;
  Board->SetSpriteGlyph(v13, this->SliderFont, 1);
  v14 = Board->CreateFrame(FT_SPRITE, this->BackFrame, this->XPosRed, 37, 0, 1);
  this->SliderDown = v14;
  Board->SetSpriteGlyph(v14, this->SliderFont, 2);
  this->Update();
}

//----- (00493BB0) --------------------------------------------------------

bool SSliderV::IsOverDownButton(int x, int y)

{
  int Height;
  bool result; // al
  result = 0;
  if ( x <= 0x15 )
  {
    Height = this->Height;
    if ( y >= Height - 22 && y < Height )
      return 1;
  }
  return result;
}

//----- (00493BE0) --------------------------------------------------------

bool SSliderV::IsOverRed(int x, int y)

{
  int XPosRed;
  int YPosOfLittleRedBigyo;
  bool result; // al
  XPosRed = this->XPosRed;
  result = 0;
  if ( x >= XPosRed && x < XPosRed + 10 )
  {
    YPosOfLittleRedBigyo = this->YPosOfLittleRedBigyo;
    if ( y >= YPosOfLittleRedBigyo - 4 && y < YPosOfLittleRedBigyo + this->YSizeOfLittleRedBigyo + 4 )
      return 1;
  }
  return result;
}

//----- (00493C30) --------------------------------------------------------

bool SSliderV::IsOverUpButton(int x, int y)

{
  return x <= 0x15 && y <= 0x15;
}

//----- (00493C50) --------------------------------------------------------

void SSliderV::OnMouseDown(int button, int x, int y, int shift)

{
  bool v6; // bl
  int Height;
  bool v8; // al
  int XPosRed;
  int YPosOfLittleRedBigyo;
  bool v11; // al
  int TopIndex;
  int v13;
  bool v14;
  if ( button == 1 )
  {
    v6 = (unsigned int)x <= 0x15 && (unsigned int)y <= 0x15;
    this->PressedUp = v6;
    v8 = 0;
    if ( (unsigned int)x <= 0x15 )
    {
      Height = this->Height;
      if ( y >= Height - 22 && y < Height )
        v8 = 1;
    }
    this->PressedDown = v8;
    v11 = 0;
    if ( !v6 && !v8 )
    {
      XPosRed = this->XPosRed;
      if ( x >= XPosRed && x < XPosRed + 10 )
      {
        YPosOfLittleRedBigyo = this->YPosOfLittleRedBigyo;
        if ( y >= YPosOfLittleRedBigyo - 4 && y < YPosOfLittleRedBigyo + this->YSizeOfLittleRedBigyo + 4 )
          v11 = 1;
      }
    }
    this->RedBigyoDown = v11;
    if ( v11 )
    {
      TopIndex = this->TopIndex;
      this->YDownPosOfLRB = y;
      this->DeltaYOfLRB = 0;
      this->TopIndexInListBox = TopIndex;
    }
    this->CaptureMouse();
    this->Update();
    if ( this->PressedUp || this->PressedDown )
    {
      Concert->PlaySound(
        "menu/button_down.wav",
        -12.0f,
        0.0f,
        -1);
      v13 = this->SetTimer(0xC8u);
      v14 = !this->PressedUp;
      this->FirstKlikkTimer = v13;
      if ( !v14 )
        this->SendAction(341345, 0);
      if ( this->PressedDown )
        this->SendAction(341346, 0);
    }
  }
}

//----- (00493DA0) --------------------------------------------------------

void SSliderV::OnMouseMove(int x, int y, int shift)

{
  bool v4; // al
  int Height;
  bool v6; // al
  bool v7;
  v4 = x <= 0x15 && (unsigned int)y <= 0x15;
  this->ActiveUp = v4;
  v6 = 0;
  if ( x <= 0x15 )
  {
    Height = this->Height;
    if ( y >= Height - 22 && y < Height )
      v6 = 1;
  }
  v7 = !this->RedBigyoDown;
  this->ActiveDown = v6;
  if ( !v7 )
    this->DeltaYOfLRB = y - this->YDownPosOfLRB;
  this->Update();
}

//----- (00493E10) --------------------------------------------------------

void SSliderV::OnMouseOut()

{
  SDXWidget::OnMouseOut();
  this->ActiveUp = 0;
  this->ActiveDown = 0;
  this->Update();
}

//----- (00493E40) --------------------------------------------------------

void SSliderV::OnMouseUp(int button, int x, int y, int shift)

{
  bool v6; // al
  int Height;
  bool v8; // al
  if ( button == 1 )
  {
    if ( this->RedBigyoDown )
      this->TopIndex = this->TopIndexInListBox;
    this->RedBigyoDown = 0;
    this->DeltaYOfLRB = 0;
    this->ReleaseMouse();
    this->Update();
  }
  if ( (this->PressedUp || this->PressedDown) && button == 1 )
  {
    this->KillTimer(&this->TickTimer);
    v6 = x <= 0x15 && (unsigned int)y <= 0x15;
    this->ActiveUp = v6;
    v8 = 0;
    if ( x <= 0x15 )
    {
      Height = this->Height;
      if ( y >= Height - 22 && y < Height )
        v8 = 1;
    }
    this->ActiveDown = v8;
    this->PressedUp = 0;
    this->PressedDown = 0;
    this->Update();
  }
}

//----- (00493F00) --------------------------------------------------------

void SSliderV::OnTimer(int id, unsigned int time)

{
  if ( id == this->FirstKlikkTimer )
  {
    if ( this->PressedUp || this->PressedDown )
      this->TickTimer = this->SetTimer(0x46u);
    this->KillTimer(&this->FirstKlikkTimer);
  }
  else if ( id == this->TickTimer )
  {
    if ( this->PressedUp )
    {
      this->SendAction(341345, 0);
    }
    else if ( this->PressedDown )
    {
      this->SendAction(341346, 0);
    }
    else
    {
      this->KillTimer(&this->TickTimer);
    }
  }
}

//----- (00493FA0) --------------------------------------------------------

void SSliderV::SetMargin(int left, int right, int top, int bottom)

{
  this->MarginLeft = left;
  this->MarginRight = right;
  this->MarginTop = top;
  this->MarginBottom = bottom;
}

//----- (00493FD0) --------------------------------------------------------

void SSliderV::SetRows(int numrows, int topindex)

{
  bool v3;
  v3 = !this->RedBigyoDown;
  this->NumberOfRows = numrows;
  if ( v3 )
    this->TopIndex = topindex;
  else
    this->TopIndexInListBox = topindex;
  this->Update();
}

//----- (00494010) --------------------------------------------------------

void SSliderV::Update()

{
  int NumberOfRows;
  int VisibleRows;
  int v6;
  int v7;
  bool v8;
  int v9;
  int YSizeOfLittleRedBigyo;
  unsigned int v11;
  int v13;
  double v14;
  int v16;
  if ( this->BackFrame < 0 || !this->Enabled )
    return;
  NumberOfRows = this->NumberOfRows;
  VisibleRows = this->VisibleRows;
  if ( NumberOfRows <= VisibleRows )
  {
    Board->ShowFrame(this->ButtonUpFrame, 0);
    Board->ShowFrame(this->ButtonDownFrame, 0);
    Board->ShowFrame(this->SliderBox, 0);
    Board->ShowFrame(this->SliderUp, 0);
    Board->ShowFrame(this->SliderMiddle, 0);
    Board->ShowFrame(this->SliderDown, 0);
    return;
  }
  v6 = this->Height - 48;
  v7 = (int)((double)v6 / ((double)NumberOfRows / (double)VisibleRows));
  this->YSizeOfLittleRedBigyo = v7;
  if ( v7 <= 1 )
  {
    this->YSizeOfLittleRedBigyo = 1;
    v7 = 1;
  }
  Board->ResizeFrame(this->SliderMiddle, 11, v7);
  v8 = !this->RedBigyoDown;
  v9 = (int)((double)(v6 * this->TopIndex) / (double)this->NumberOfRows + 24.0);
  this->YPosOfLittleRedBigyo = v9;
  if ( !v8 )
  {
    v9 += this->DeltaYOfLRB;
    this->YPosOfLittleRedBigyo = v9;
  }
  if ( v9 < 24 )
  {
    this->YPosOfLittleRedBigyo = 24;
    v9 = 24;
  }
  YSizeOfLittleRedBigyo = this->YSizeOfLittleRedBigyo;
  if ( YSizeOfLittleRedBigyo + v9 > v6 + 24 )
    this->YPosOfLittleRedBigyo = v6 - YSizeOfLittleRedBigyo + 24;
  Board->MoveFrame(this->SliderMiddle, this->XPosRed, this->YPosOfLittleRedBigyo);
  Board->MoveFrame(this->SliderUp, this->XPosRed, this->YPosOfLittleRedBigyo - 4);
  Board->MoveFrame(this->SliderDown, this->XPosRed, this->YSizeOfLittleRedBigyo + this->YPosOfLittleRedBigyo);
  Board->ShowFrame(this->ButtonUpFrame, 1);
  Board->ShowFrame(this->ButtonDownFrame, 1);
  Board->ShowFrame(this->SliderBox, 1);
  Board->ShowFrame(this->SliderUp, 1);
  Board->ShowFrame(this->SliderMiddle, 1);
  Board->ShowFrame(this->SliderDown, 1);
  Board->SetSpriteGlyph(this->ButtonUpFrame, this->ButtonFont, 9);
  Board->SetSpriteGlyph(this->ButtonDownFrame, this->ButtonFont, 6);
  if ( this->PressedUp )
  {
    Board->SetSpriteGlyph(this->ButtonUpFrame, this->ButtonFont, 11);
    goto LABEL_21;
  }
  if ( this->PressedDown )
  {
    v16 = 8;
LABEL_20:
    Board->SetSpriteGlyph(this->ButtonDownFrame, this->ButtonFont, v16);
    goto LABEL_21;
  }
  if ( this->ActiveUp )
  {
    Board->SetSpriteGlyph(this->ButtonUpFrame, this->ButtonFont, 10);
    goto LABEL_21;
  }
  if ( this->ActiveDown )
  {
    v16 = 7;
    goto LABEL_20;
  }
LABEL_21:
  if ( this->RedBigyoDown )
  {
    v11 = this->NumberOfRows;
    v13 = v11 - this->VisibleRows;
    v14 = (double)(this->YPosOfLittleRedBigyo - 24) / (double)v6 * (double)(int)v11 + 0.5;
    if ( (int)v14 <= v13 )
      v13 = (int)v14;
    if ( v13 != this->TopIndexInListBox )
      this->SendAction(341347, v13);
  }
}

