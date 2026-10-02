// window/textbox.cpp
// Text box widget
// Decompiled from: gameSplit/slistbox.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>

#include "textbox.h"

// Classes: STextBox
// Function count: 8

//----- (004948F0) --------------------------------------------------------

STextBox::~STextBox()

{
  this->TopIndex = 0;
  this->Update();
  // base destructor called automatically
  if ( this->TextFrames.array )
  {
    free(this->TextFrames.array);
    this->TextFrames.array = 0;
  }
  // base destructor called automatically
}

//----- (00494A10) --------------------------------------------------------

void STextBox::Create(int font, unsigned char NumberOfTextLines, char bBackGround, int bScrollbars, SHorizontalAlign hAlign, SVerticalAlign vAlign)

{
  char v7; // bl
  int v9;
  v7 = bBackGround;
  this->horizontalAlign = hAlign;
  this->verticalAlign = vAlign;
  this->Scrollbars = bScrollbars;
  if ( v7 )
    this->SetBackgroundColor(0x4C50B4A8u);
  this->Font = font;
  Board->GetTextExtent(font, 0, 0, (int *)&hAlign, (int *)&bBackGround, 1.0f);
  SDXWidget::Create((int)this);
  bScrollbars = NumberOfTextLines;
  this->VisibleTextLines = NumberOfTextLines;
  if ( v7 )
  {
    v9 = Board->CreateFrame(FT_BOX, this->BackFrame, 6, 6, 0, 0);
    Board->SetBoxColor(v9, 860927144u);
    Board->ResizeFrame(v9, this->Width - 12, this->Height - 12);
  }
  if ( this->Scrollbars )
  {
    this->InsertChild(&this->Slider);
    this->Slider.Create((int)this, this->Width, this->Height, bScrollbars);
  }
  this->Update();
}

//----- (00494B10) --------------------------------------------------------

int STextBox::GetRowCount()

{
  return this->TextFrames.size;
}

//----- (00494B20) --------------------------------------------------------

int STextBox::GetTopIndex()

{
  return this->TopIndex;
}

//----- (00494B30) --------------------------------------------------------

void STextBox::ResetContent()

{
  this->TopIndex = 0;
  this->Update();
}

//----- (00494B50) --------------------------------------------------------

int STextBox::SetText(const char *text)

{
  int v2;
  const char *v3; // esi
  char *v4; // eax
  signed int v5;
  signed int v6;
  const char *v7; // esi
  const char *v8; // edi
  const char *v9; // eax
  signed int v10;
  int v11;
  bool v12;
  char *v13; // edi
  int v14;
  int v15;
  int v16;
  char *v17; // ecx
  char v18; // al
  char *v19; // ecx
  char v20; // al
  int v22;
  int v23;
  int v24;
  int v25;
  int *v26; // eax
  int maxsize;
  int bufheight;
  int FontWidth;
  int bufwidth;
  char *Destination;
  char *Source;
  signed int v35;
  int FontHeight;
  const char *v37;
  signed int v38;
  signed int v39;
  const char *v40;
  int v41;
  int v42;
  int v43;
  STextBox *v44;
  char rowText[260];
  v42 = 0;
  v2 = 0;
  v44 = this;
  v3 = text;
  if ( this->Scrollbars )
    v2 = 18;
  v40 = text;
  v43 = this->Width - v2;
  Board->GetTextExtent(this->Font, 0, 0, &FontWidth, &FontHeight, 1.0f);
  v38 = strlen(text);
  v4 = new char[v38 + 1];
  v5 = 0;
  Destination = v4;
  v35 = 0;
  v6 = 0;
  v39 = 0;
  while ( 2 )
  {
    v41 = 0;
    Source = (char *)&v3[v5];
    do
    {
      v7 = &v3[v6];
      v8 = strchr(v7, 32);
      v9 = strchr(v7, 10);
      v37 = v9;
      if ( !v9 )
        goto LABEL_11;
      if ( !v8 )
      {
        v8 = v9;
        goto LABEL_12;
      }
      if ( v9 < v8 )
        v8 = v9;
      else
LABEL_11:
        v37 = 0;
LABEL_12:
      v10 = v38;
      v11 = v8 - v40;
      v12 = v8 == 0;
      v13 = Destination;
      if ( !v12 )
        v10 = v11;
      v14 = v10 - v35;
      strncpy(Destination, Source, v10 - v35);
      v13[v14] = 0;
      v6 = v10 + 1;
      Board->GetTextExtent(v44->Font, v13, strlen(v13), &bufwidth, &bufheight, 1.0f);
      v15 = bufwidth;
      v16 = v43;
      if ( bufwidth < v43 )
      {
        v17 = v13;
        do
        {
          v18 = *v17++;
          v17[rowText - v13 - 1] = v18;
        }
        while ( v18 );
        ++v41;
LABEL_21:
        v16 = v43;
        v39 = v6;
        goto LABEL_22;
      }
      if ( !v41 )
      {
        v19 = v13;
        do
        {
          v20 = *v19++;
          v19[rowText - v13 - 1] = v20;
        }
        while ( v20 );
        goto LABEL_21;
      }
LABEL_22:
      if ( v37 )
        break;
      if ( v15 >= v16 )
        break;
      v3 = v40;
    }
    while ( v6 < v38 );
    v22 = Board->CreateFrame(FT_TEXT, v44->BackFrame, 0, v42 * FontHeight, 0, 1);
    Board->ResizeFrame(v22, v43, FontHeight);
    Board->SetText(v22, v44->Font, v44->horizontalAlign, rowText);
    v23 = v44->TextFrames.size;
    v24 = v44->TextFrames.maxsize;
    if ( v23 == v24 )
    {
      if ( v24 >= 16 )
        v25 = 6 * v24 / 5;
      else
        v25 = 16;
      v26 = (int *)realloc(v44->TextFrames.array, 4 * v25);
      maxsize = v44->TextFrames.maxsize;
      v44->TextFrames.array = v26;
      memset(&v26[maxsize], 0, 4 * (v25 - maxsize));
      v44->TextFrames.maxsize = v25;
      v23 = v44->TextFrames.size;
    }
    ++v42;
    v44->TextFrames.size = v23 + 1;
    v44->TextFrames.array[v23] = v22;
    v5 = v39;
    v6 = v39;
    v35 = v39;
    if ( v39 < v38 )
    {
      v3 = v40;
      continue;
    }
    break;
  }
  operator delete[](v13);
  v44->Update();
  return v42;
}

//----- (00494EB0) --------------------------------------------------------

void STextBox::Update()

{
  STextBox *v1; // esi
  int size;
  int v3;
  int v4;
  int v5;
  SVerticalAlign verticalAlign;
  int v7;
  int v8;
  int FontWidth;
  STextBox *v10;
  int v11;
  int FontHeight;
  v1 = this;
  v10 = this;
  if ( this->BackFrame >= 0 )
  {
    Board->GetTextExtent(
      this->Font,
      0,
      0,
      &FontWidth,
      &FontHeight,
      1.0f);
    size = v1->TextFrames.size;
    v3 = 0;
    v4 = FontHeight;
    v5 = v1->Height - FontHeight * size;
    v11 = v5;
    if ( size > 0 )
    {
      while ( 1 )
      {
        verticalAlign = v1->verticalAlign;
        if ( verticalAlign == Top )
          break;
        v7 = verticalAlign - 1;
        if ( !v7 )
        {
          v1 = v10;
          Board->MoveFrame(v10->TextFrames.array[v3], 0, v5 / 2 + FontHeight * v3);
          goto LABEL_10;
        }
        if ( v7 == 1 )
        {
          v8 = v11 + v4 * v3;
          goto LABEL_9;
        }
LABEL_11:
        size = v1->TextFrames.size;
        if ( ++v3 >= size )
          goto LABEL_12;
      }
      v8 = v4 * v3;
LABEL_9:
      Board->MoveFrame(v1->TextFrames.array[v3], 0, v8);
LABEL_10:
      v4 = FontHeight;
      v5 = v11;
      goto LABEL_11;
    }
LABEL_12:
    if ( v1->Scrollbars )
      v1->Slider.SetRows(size, v1->TopIndex);
  }
}

//----- (00494860) --------------------------------------------------------

STextBox::STextBox()

{
  this->TextFrames.array = 0;
  this->TextFrames.size = 0;
  this->TextFrames.maxsize = 0;
  this->TopIndex = 0;
  this->MaxInnerTextLines = 2000;
}

