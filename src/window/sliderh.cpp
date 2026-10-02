// window/sliderh.cpp
// Horizontal slider
// Decompiled from: gameSplit/sslider.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

extern unsigned char g_MenuRace;

#include "sliderh.h"

// Classes: SSliderH
// Function count: 16

//----- (00492AD0) --------------------------------------------------------

SSliderH::SSliderH()

{
  *(_DWORD *)&this->PressedLeft = 0;
  this->ButtonFont = -1;
  this->SliderFont = -1;
  this->TickTimer = -1;
  this->FirstKlikkTimer = -1;
  this->XPosOfLittleRedBigyo = 0;
  this->DeltaXOfLRB = 0;
  this->SliderPos = 0;
  this->RedBigyoDown = 0;
  this->XDownPosOfLRB = 0;
  this->SliderRovatka = 0;
}

//----- (00492B60) --------------------------------------------------------

SSliderH::~SSliderH()

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
  if ( this->SliderRovatka )
  {
    operator delete[](this->SliderRovatka);
    this->SliderRovatka = 0;
  }
  // base destructor called automatically
}

//----- (00492CE0) --------------------------------------------------------

void SSliderH::Create(int posx, int posy, int width, int numberoffixpos, int sliderpos)

{
  int v7;
  int v9;
  int v10;
  int *v11; // eax
  int v12;
  int v13;
  int v14;
  float v15; // xmm1_4
  int v16;
  int savedregs;
  float widtha;
  this->SetPosition(posx, posy, width, 22);
  SDXWidget::Create((int)this);
  this->NumberOfFixPos = numberoffixpos;
  this->SliderPos = sliderpos;
  this->ButtonLeftFrame = Board->CreateFrame(FT_SPRITE, this->BackFrame, 2, 0, 0, 1);
  v7 = Board->CreateFrame(FT_SPRITE, this->BackFrame, this->Width - 22, 0, 0, 1);
  savedregs = 1;
  this->ButtonRightFrame = v7;
  if ( g_MenuRace )
    v9 = Board->LoadFixedFont("menu/pig_arrows_medium.png", 21, 22, 6, 12, 0, X2);
  else
    v9 = Board->LoadFixedFont("menu/rabbit_arrows_medium.png", 21, 22, 6, 12, 0, X2);
  this->ButtonFont = v9;
  v10 = Board->CreateFrame(FT_BOX, this->BackFrame, 18, 10, 0, 0);
  this->SliderBox = v10;
  Board->SetBoxColor(v10, 0xFF848484);
  Board->ResizeFrame(this->SliderBox, this->Width - 37, 2);
  v11 = (int *)operator new[](4 * (this->NumberOfFixPos + 1));
  v12 = this->Width;
  this->SliderRovatka = v11;
  v13 = v12 - 46;
  v14 = 0;
  if ( this->NumberOfFixPos + 1 > 0 )
  {
    v15 = (float)v13;
    widtha = (float)v13;
    do
    {
      this->SliderRovatka[v14] = Board->CreateFrame(
                                   FT_BOX,
                                   this->BackFrame,
                                   (int)(float)((float)((float)(v15 / (float)this->NumberOfFixPos) * (float)v14) + 22.0),
                                   9,
                                   0,
                                   0);
      Board->SetBoxColor(this->SliderRovatka[v14], 0xFF848484);
      Board->ResizeFrame(this->SliderRovatka[v14++], 2, 4);
      v15 = widtha;
    }
    while ( v14 < this->NumberOfFixPos + 1 );
  }
  this->SliderFont = Board->LoadSingleFont("menu/csuszka_gomb.png", X2);
  v16 = Board->CreateFrame(FT_SPRITE, this->BackFrame, 50, this->Height / 2 - 6, 0, 1);
  this->SliderFrame = v16;
  Board->SetSpriteGlyph(v16, this->SliderFont, 0);
  this->Update();
}

//----- (00492F00) --------------------------------------------------------

int SSliderH::GetNumberOfFixPos()

{
  return this->NumberOfFixPos;
}

//----- (00492F10) --------------------------------------------------------

int SSliderH::GetSliderPos()

{
  return this->SliderPos;
}

//----- (00492F20) --------------------------------------------------------

bool SSliderH::OnAction(SWidget *sender, int action, int param)

{
  bool v5; // sf
  int SliderPos;
  int NumberOfFixPos;
  if ( action == 341121 )
  {
    v5 = --this->SliderPos < 0;
  }
  else
  {
    if ( action != 341122 )
      return 0;
    v5 = ++this->SliderPos < 0;
  }
  SliderPos = this->SliderPos;
  if ( v5 )
  {
    this->SliderPos = 0;
    SliderPos = 0;
  }
  NumberOfFixPos = this->NumberOfFixPos;
  if ( SliderPos > NumberOfFixPos )
    this->SliderPos = NumberOfFixPos;
  this->Update();
  this->SendAction(341124, this->SliderPos);
  return 1;
}

//----- (00492FA0) --------------------------------------------------------

bool SSliderH::OnKeyDown(int keycode, bool repeat)

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
    case '&':
      Up = this->Up;
      if ( Up )
      {
        if ( this->ActiveLeft || this->ActiveRight )
        {
          *(_WORD *)&this->ActiveLeft = 0;
          this->Update();
          this->Up->SetFocus();
          return 1;
        }
        goto LABEL_19;
      }
      return 1;
    case '(':
      Up = this->Down;
      if ( Up )
      {
        if ( this->ActiveLeft || this->ActiveRight )
        {
          *(_WORD *)&this->ActiveLeft = 0;
          this->Update();
          Up = this->Down;
        }
LABEL_19:
        Up->SetFocus();
      }
      return 1;
    case '%':
      this->SendAction(341121, 0);
      return 1;
    case '\'':
      this->SendAction(341122, 0);
      return 1;
    default:
      return 0;
  }
}

//----- (004930D0) --------------------------------------------------------

void SSliderH::OnMouseDown(int button, int x, int y, int shift)

{
  bool v6; // cl
  int Width;
  bool v8; // al
  int NumberOfFixPos;
  int v10;
  int v11;
  int XPosOfLittleRedBigyo;
  bool v13; // al
  int v14;
  bool v15;
  if ( button == 1 )
  {
    v6 = (unsigned int)x <= 0x13 && y >= 0 && y < this->Height;
    Width = this->Width;
    this->PressedLeft = v6;
    v8 = x >= Width - 20 && x < Width && y >= 0 && y < this->Height;
    this->PressedRight = v8;
    if ( v6 )
      goto LABEL_26;
    if ( !v8 )
    {
      NumberOfFixPos = this->NumberOfFixPos;
      v10 = (int)((float)((float)(x - 24) / (float)((float)(Width - 46) / (float)NumberOfFixPos)) + 0.5);
      this->SliderPos = v10;
      if ( v10 < 0 )
      {
        this->SliderPos = 0;
        v10 = 0;
      }
      if ( v10 > NumberOfFixPos )
        this->SliderPos = NumberOfFixPos;
      this->Update();
      this->SendAction(341124, this->SliderPos);
      if ( this->PressedLeft )
        goto LABEL_26;
    }
    if ( this->PressedRight
      || (v11 = this->Height / 2, y < v11 - 5)
      || y >= v11 + 5
      || (XPosOfLittleRedBigyo = this->XPosOfLittleRedBigyo, x < XPosOfLittleRedBigyo)
      || x >= XPosOfLittleRedBigyo + 12 )
    {
LABEL_26:
      v13 = 0;
    }
    else
    {
      v13 = 1;
    }
    this->RedBigyoDown = v13;
    if ( v13 )
    {
      this->XDownPosOfLRB = x;
      this->DeltaXOfLRB = 0;
    }
    this->CaptureMouse();
    this->Update();
    if ( this->PressedLeft || this->PressedRight )
    {
      Concert->PlaySound(
        "menu/button_down.wav",
        -12.0f,
        0,
        -1);
      v14 = this->SetTimer(0xC8u);
      v15 = !this->PressedLeft;
      this->FirstKlikkTimer = v14;
      if ( !v15 )
        this->SendAction(341121, 0);
      if ( this->PressedRight )
        this->SendAction(341122, 0);
    }
  }
}

//----- (004932A0) --------------------------------------------------------

void SSliderH::OnMouseMove(int x, int y, int shift)

{
  bool v4; // al
  int Width;
  bool v6; // al
  bool v7;
  v4 = (unsigned int)x <= 0x13 && y >= 0 && y < this->Height;
  Width = this->Width;
  this->ActiveLeft = v4;
  v6 = x >= Width - 20 && x < Width && y >= 0 && y < this->Height;
  v7 = !this->RedBigyoDown;
  this->ActiveRight = v6;
  if ( !v7 )
    this->DeltaXOfLRB = x - this->XDownPosOfLRB;
  this->Update();
}

//----- (00493310) --------------------------------------------------------

void SSliderH::OnMouseOut()

{
  SDXWidget::OnMouseOut();
  if ( this->ActiveLeft || this->ActiveRight )
  {
    *(_WORD *)&this->ActiveLeft = 0;
    this->Update();
  }
}

//----- (00493340) --------------------------------------------------------

void SSliderH::OnMouseOver()

{
  this->SendAction(324865, 0);
}

//----- (00493350) --------------------------------------------------------

void SSliderH::OnMouseUp(int button, int x, int y, int shift)

{
  int NumberOfFixPos;
  int v7;
  bool v8; // al
  int Width;
  bool v10; // al
  if ( button == 1 )
  {
    this->DeltaXOfLRB = 0;
    this->ReleaseMouse();
  }
  if ( this->PressedLeft || this->PressedRight )
    goto LABEL_13;
  if ( button == 1 && this->RedBigyoDown )
  {
    NumberOfFixPos = this->NumberOfFixPos;
    this->RedBigyoDown = 0;
    v7 = (int)((float)((float)(x - 24) / (float)((float)(this->Width - 46) / (float)NumberOfFixPos)) + 0.5);
    this->SliderPos = v7;
    if ( v7 < 0 )
    {
      this->SliderPos = 0;
      v7 = 0;
    }
    if ( v7 > NumberOfFixPos )
      this->SliderPos = NumberOfFixPos;
    this->Update();
    this->SendAction(341124, this->SliderPos);
    if ( this->PressedLeft )
      goto LABEL_13;
  }
  if ( this->PressedRight )
  {
LABEL_13:
    if ( button == 1 )
    {
      this->KillTimer(&this->TickTimer);
      v8 = (unsigned int)x <= 0x13 && y >= 0 && y < this->Height;
      Width = this->Width;
      this->ActiveLeft = v8;
      v10 = x >= Width - 20 && x < Width && y >= 0 && y < this->Height;
      this->ActiveRight = v10;
      *(_WORD *)&this->PressedLeft = 0;
      this->Update();
    }
  }
}

//----- (004934A0) --------------------------------------------------------

void SSliderH::OnTimer(int id, unsigned int time)

{
  if ( id == this->FirstKlikkTimer )
  {
    if ( this->PressedLeft || this->PressedRight )
      this->TickTimer = this->SetTimer(0x46u);
    this->KillTimer(&this->FirstKlikkTimer);
  }
  else if ( id == this->TickTimer )
  {
    if ( this->PressedLeft )
    {
      this->SendAction(341121, 0);
    }
    else if ( this->PressedRight )
    {
      this->SendAction(341122, 0);
    }
    else
    {
      this->KillTimer(&this->TickTimer);
    }
  }
}

//----- (00493540) --------------------------------------------------------

void SSliderH::SetActive(bool active)

{
  bool ActiveLeft; // al
  ActiveLeft = this->ActiveLeft;
  if ( active )
  {
    if ( !ActiveLeft || !this->ActiveRight )
    {
LABEL_7:
      this->ActiveRight = active;
      this->ActiveLeft = active;
      this->Update();
    }
  }
  else if ( ActiveLeft || this->ActiveRight )
  {
    goto LABEL_7;
  }
}

//----- (00493590) --------------------------------------------------------

void SSliderH::SetFocus()

{
  Concert->PlaySound(
    "menu/button_over.wav",
    -30.0f,
    0,
    -1);
  if ( !this->ActiveLeft || !this->ActiveRight )
  {
    *(_WORD *)&this->ActiveLeft = 257;
    this->Update();
  }
  SWidget::SetFocus();
}

//----- (004935F0) --------------------------------------------------------

void SSliderH::Update()

{
  int v3;
  bool v4;
  int v5;
  int v6;
  int v7;
  if ( this->BackFrame >= 0 && this->Enabled )
  {
    v3 = this->Width - 46;
    v4 = !this->RedBigyoDown;
    v5 = (int)(float)((float)((float)((float)v3 / (float)this->NumberOfFixPos) * (float)this->SliderPos) + 20.0);
    this->XPosOfLittleRedBigyo = v5;
    if ( !v4 )
    {
      v5 += this->DeltaXOfLRB;
      this->XPosOfLittleRedBigyo = v5;
    }
    if ( v5 < 20 )
    {
      this->XPosOfLittleRedBigyo = 20;
      v5 = 20;
    }
    v6 = v3 + 20;
    if ( v5 > v6 )
    {
      this->XPosOfLittleRedBigyo = v6;
      v5 = v6;
    }
    Board->MoveFrame(this->SliderFrame, v5 - 2, this->Height / 2 - 6);
    Board->ShowFrame(this->ButtonLeftFrame, 1);
    Board->ShowFrame(this->ButtonRightFrame, 1);
    Board->ShowFrame(this->SliderBox, 1);
    Board->ShowFrame(this->SliderFrame, 1);
    Board->SetSpriteGlyph(this->ButtonLeftFrame, this->ButtonFont, 0);
    Board->SetSpriteGlyph(this->ButtonRightFrame, this->ButtonFont, 3);
    if ( this->PressedLeft )
    {
      v7 = 2;
    }
    else
    {
      if ( this->PressedRight )
      {
        Board->SetSpriteGlyph(this->ButtonRightFrame, this->ButtonFont, 5);
        goto LABEL_16;
      }
      if ( !this->ActiveLeft )
      {
LABEL_16:
        if ( this->ActiveRight )
          Board->SetSpriteGlyph(this->ButtonRightFrame, this->ButtonFont, 4);
        return;
      }
      v7 = 1;
    }
    Board->SetSpriteGlyph(this->ButtonLeftFrame, this->ButtonFont, v7);
    goto LABEL_16;
  }
}

