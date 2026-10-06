// window/radiobutton.cpp
// Radio button widget
// Decompiled from: gameSplit/sslider.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "radiobutton.h"

// Classes: SRadioButton
// Function count: 23

//----- (00490310) --------------------------------------------------------

SRadioButton::SRadioButton()

{
  char *v2; // eax
  size_t v3;
  this->Text.buf = 0;
  this->Text.size = 0;
  this->Text.size = 12;
  v2 = new char[0xDu];
  v3 = this->Text.size + 1;
  this->Text.buf = v2;
  memcpy(v2, "SRadioButton", v3);
  *(_WORD *)&this->Active = 0;
  this->bChecked = 0;
  this->RadioFont = -1;
  this->RadioDisabledFont = -1;
  this->RadioHLFont = -1;
  this->HdControlsFont = -1;
}

//----- (004903D0) --------------------------------------------------------

SRadioButton::~SRadioButton()

{
  char *buf; // eax
  int RadioFont;
  RadioFont = this->RadioFont;
  Board->ReleaseFont(RadioFont);
  Board->ReleaseFont(this->RadioDisabledFont);
  Board->ReleaseFont(this->RadioHLFont);
  buf = this->Text.buf;
  if ( buf )
  {
    delete[] buf;
    this->Text.buf = 0;
  }
  // base destructor called automatically
}

//----- (00490530) --------------------------------------------------------

void SRadioButton::Create(int a2, int font, unsigned int normal_color, unsigned int active_color, unsigned int disabled_color)

{
  SDXWidget::Create(a2);
  this->Font = font;
  this->NormalColor = normal_color;
  this->ActiveColor = active_color;
  this->DisabledColor = disabled_color;
  this->TextFrame = Board->CreateFrame(FT_TEXT, this->BackFrame, 20, 0, 0, 0);
  this->RadioFrame = Board->CreateFrame(FT_SPRITE, this->BackFrame, 4, 6, 0, 0);
  this->RadioFont = Board->LoadFixedFont("menu/radio.png", 12, 12, 2, 2, 0, X2);
  this->RadioDisabledFont = Board->LoadFixedFont("menu/radio_disabled.png", 12, 12, 2, 2, 0, X2);
  this->RadioHLFont = Board->LoadFixedFont("menu/radio_hl.png", 12, 12, 2, 2, 0, X2);
  this->Update();
}

//----- (00490610) --------------------------------------------------------

bool SRadioButton::GetCheck()

{
  return this->bChecked;
}

//----- (00490620) --------------------------------------------------------

char *SRadioButton::GetText()

{
  char *buf; // ecx
  buf = this->Text.buf;
  if ( buf )
    return buf;
  return (char *)"";
}

//----- (00490770) --------------------------------------------------------

void SRadioButton::OnMouseDown(int button, int x, int y, int shift)

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

// PANZERS 0x53ebc0
// HD order: clear the hover and redraw first, then the SDXWidget handler.
void SRadioButton::OnMouseOut()

{
  this->Active = 0;
  this->Update();
  SDXWidget::OnMouseOut();
}

// PANZERS 0x53ebe0
// HD sends only 0x42543 (SWINE first sent its gamepad-focus action 0x4f501).
void SRadioButton::OnMouseOver()

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

//----- (00490840) --------------------------------------------------------

void SRadioButton::OnMouseUp(int button, int x, int y, int shift)

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
      this->bChecked = 1;
      this->Update();
      this->SendAction(271682, 0);
    }
  }
}

//----- (004908F0) --------------------------------------------------------

void SRadioButton::SetActive(bool active)

{
  if ( this->Active != active )
  {
    this->Active = active;
    this->Update();
  }
}

//----- (00490910) --------------------------------------------------------

void SRadioButton::SetCheck(bool checked)

{
  this->bChecked = checked;
  this->Update();
}

//----- (00490980) --------------------------------------------------------

void SRadioButton::SetText(const char *text)

{
  this->Text = text;
  this->Update();
}

//----- (004909B0) --------------------------------------------------------

void SRadioButton::SetVisible(bool visible)

{
  if ( this->Visible != visible )
  {
    this->Visible = visible;
    this->Update();
  }
}

// PANZERS 0x53eaf0
// HD Create: no fonts of its own; the radio glyphs come from the menu
// controls font (HD 0x8da788, menu/controls_hq.tga), the text frame at x 0x18
// and the glyph frame at x 4, cursor 0. HD has no colour or font arguments:
// the label is drawn in board font 3 (Update). Callers pass the controls
// font handle (the Panzers code keeps it in pz g_MenuControlsFont).
void SRadioButton::CreateHD(int a2, int controlsFont)

{
  SDXWidget::Create(a2);
  this->HdControlsFont = controlsFont;
  this->TextFrame = Board->CreateFrame(FT_TEXT, this->BackFrame, 0x18, 0, 0, 0);
  this->RadioFrame = Board->CreateFrame(FT_SPRITE, this->BackFrame, 4, 0, 0, 0);
  this->Cursor = 0;   // 0x543970(0, -1)
  this->Update();
}

// PANZERS 0x53ed30
// HD Update (CreateHD widgets): controls glyph 0x11 / 0x12 (unchecked /
// checked), 0x13 / 0x14 when disabled; text colour 0x666666 disabled,
// 0xffffff hovered or pressed, else 0xd0d0d0; font 3; width = text + 0x18.
void SRadioButton::UpdateHD()

{
  int width;
  int height;
  unsigned int color;
  if ( this->BackFrame < 0 )
    return;
  if ( !this->Enabled )
  {
    Board->SetSpriteGlyph(this->RadioFrame, this->HdControlsFont, this->bChecked ? 0x14 : 0x13);
    color = 0x666666;
  }
  else
  {
    Board->SetSpriteGlyph(this->RadioFrame, this->HdControlsFont, this->bChecked ? 0x12 : 0x11);
    color = ( this->Pressed || this->Active ) ? 0xFFFFFF : 0xD0D0D0;
  }
  Board->SetTextColor(this->TextFrame, color);
  Board->SetText(this->TextFrame, 3, 0, this->Text.buf ? this->Text.buf : "");
  Board->GetFrameSize(this->TextFrame, &width, &height);
  this->Resize(width + 0x18, height);
}

//----- (004909D0) --------------------------------------------------------

void SRadioButton::Update()

{
  if ( this->HdControlsFont >= 0 )
  {
    this->UpdateHD();
    return;
  }
  bool bChecked; // al
  bool v4;
  char *v6; // edx
  char *buf; // edx
  int width;
  int height;
  if ( this->BackFrame >= 0 )
  {
    bChecked = this->bChecked;
    if ( this->Enabled )
    {
      v4 = !bChecked;
      if ( v4 )
        Board->SetSpriteGlyph(this->RadioFrame, this->RadioFont, 1);
      else
        Board->SetSpriteGlyph(this->RadioFrame, this->RadioFont, 0);
      if ( this->Pressed || this->Active )
      {
        Board->SetSpriteGlyph(this->RadioFrame, this->RadioHLFont, !this->bChecked);
        Board->SetTextColor(this->TextFrame, this->ActiveColor);
        buf = (char *)"";
        if ( this->Text.buf )
          buf = this->Text.buf;
        Board->SetText(this->TextFrame, this->Font + 1, 0, buf);
        goto LABEL_20;
      }
      Board->SetSpriteGlyph(this->RadioFrame, this->RadioFont, !this->bChecked);
      Board->SetTextColor(this->TextFrame, this->NormalColor);
      v6 = (char *)"";
      if ( this->Text.buf )
        v6 = this->Text.buf;
    }
    else
    {
      v4 = !bChecked;
      if ( v4 )
        Board->SetSpriteGlyph(this->RadioFrame, this->RadioDisabledFont, 1);
      else
        Board->SetSpriteGlyph(this->RadioFrame, this->RadioDisabledFont, 0);
      Board->SetTextColor(this->TextFrame, this->DisabledColor);
      v6 = (char *)"";
      if ( this->Text.buf )
        v6 = this->Text.buf;
    }
    Board->SetText(this->TextFrame, this->Font, 0, v6);
LABEL_20:
    Board->GetFrameSize(this->TextFrame, &width, &height);
    this->Resize(width + 20, height);
  }
}
