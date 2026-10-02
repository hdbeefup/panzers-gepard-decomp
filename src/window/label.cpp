// window/label.cpp
// Text label widget
// Decompiled from: gameSplit/swidget.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "label.h"

// Classes: SLabel
// Function count: 11

//----- (0048C9B0) --------------------------------------------------------

SLabel::SLabel()

{
  this->Text.buf = new char[1];
  this->Text.buf[0] = '\0';
  this->Text.size = 0;
  this->Color = 0xFFFFFF;
}

//----- (0048CAA0) --------------------------------------------------------

void SLabel::Create(int a2, int font)

{
  SDXWidget::Create(a2);
  this->Font = font;
  this->TextFrame = Board->CreateFrame(FT_TEXT, this->BackFrame, 0, 0, 0, 0);
  this->Update();
}

//----- (0048CAE0) --------------------------------------------------------

char *SLabel::GetText()

{
  if ( this->Text.buf )
    return this->Text.buf;
  return (char *)"";
}

//----- (0048CB00) --------------------------------------------------------

void SLabel::Resize(int width, int height)

{
  SDXWidget::Resize(width, height);
  this->Update();
}

//----- (0048CB20) --------------------------------------------------------

void SLabel::SetPosition(int x, int y, int width, int height)

{
  SDXWidget::SetPosition(x, y, width, height);
  this->Update();
}

//----- (0048CB50) --------------------------------------------------------

void SLabel::SetText(const char *text)

{
  this->Text = text;
  this->Update();
}

//----- (0048CB80) --------------------------------------------------------

void SLabel::SetTextColor(unsigned int color)

{
  bool v2; // sf
  v2 = this->BackFrame < 0;
  this->Color = color;
  if ( !v2 )
    Board->SetTextColor(this->TextFrame, color);
}

//----- (0048CBB0) --------------------------------------------------------

void SLabel::SetTextF(const char *format, ...)

{
  char *buf;
  int v4;
  char *v5;
  size_t v6;
  char Buffer[512];
  va_list ArgList;
  va_start(ArgList, format);
  vsprintf(Buffer, format, ArgList);
  buf = this->Text.buf;
  if ( buf )
  {
    delete[] buf;
    this->Text.buf = 0;
  }
  v4 = strlen(Buffer);
  this->Text.size = v4;
  v5 = new char[v4 + 1];
  v6 = this->Text.size + 1;
  this->Text.buf = v5;
  memcpy(v5, Buffer, v6);
  this->Update();
}

//----- (0048CC70) --------------------------------------------------------

void SLabel::SetTextV(const char *format, char *args)

{
  char *v5;
  int v6;
  char *v7;
  size_t v8;
  char buf[512];
  vsprintf(buf, format, args);
  v5 = this->Text.buf;
  if ( v5 )
  {
    delete[] v5;
    this->Text.buf = 0;
  }
  v6 = strlen(buf);
  this->Text.size = v6;
  v7 = new char[v6 + 1];
  v8 = this->Text.size + 1;
  this->Text.buf = v7;
  memcpy(v7, buf, v8);
  this->Update();
}

//----- (0048CD30) --------------------------------------------------------

void SLabel::Update()

{
  if ( this->BackFrame >= 0 )
  {
    Board->ResizeFrame(this->TextFrame, this->Width, this->Height);
    const char *buf = this->Text.buf ? this->Text.buf : "";
    Board->SetText(this->TextFrame, this->Font, 0, buf);
  }
}

//----- (004DB230) --------------------------------------------------------

SLabel::~SLabel()

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

