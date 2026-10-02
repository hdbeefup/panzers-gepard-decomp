// window/editboxwithtext.cpp
// Edit box with label
// Decompiled from: gameSplit/sdxwidget.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "editboxwithtext.h"

// Classes: SEditBoxWithText
// Function count: 8

//----- (004885F0) --------------------------------------------------------

void SEditBoxWithText::Create(int a2, int promptfont, int editfont, const char *textprompt)

{
  int v6;
  int height;
  int width;
  SDXWidget::Create(a2);
  this->Font = promptfont;
  v6 = Board->CreateFrame(FT_TEXT, this->BackFrame, 0, 0, 0, 0);
  this->Prompt = v6;
  Board->SetText(v6, this->Font, 0, textprompt);
  Board->GetFrameSize(this->Prompt, &width, &height);
  width += 10;
  this->InsertChild(&this->EditBox);
  int editW = (this->Width > width) ? this->Width - width : 0;
  int editH = this->Height;
  if ( this->Font == 6 )
    this->EditBox.SetPosition(width, 1, editW, editH);
  else
    this->EditBox.SetPosition(width, -1, editW, editH);
  this->EditBox.Create(editfont, 1, 1);
}

//----- (004886D0) --------------------------------------------------------

void SEditBoxWithText::SetFocusToInnerEditBox()

{
  this->EditBox.SetFocus();
}

//----- (00488530) --------------------------------------------------------

SEditBoxWithText::SEditBoxWithText()

{
}

//----- (00488590) --------------------------------------------------------

SEditBoxWithText::~SEditBoxWithText()

{
  // base destructor called automatically
  // base destructor called automatically
}

//----- (004886B0) --------------------------------------------------------

char *SEditBoxWithText::GetText()

{
  return this->EditBox.GetText();
}

//----- (004886C0) --------------------------------------------------------

void SEditBoxWithText::SetBackgroundColor(unsigned int color)

{
  this->EditBox.SetBackgroundColor(color);
}

//----- (004886E0) --------------------------------------------------------

void SEditBoxWithText::SetText(const char *text)

{
  this->EditBox.SetText(text);
}

//----- (004886F0) --------------------------------------------------------

void SEditBoxWithText::SetTooltipText(const char *tooltiptext)

{
  SDXWidget::SetTooltipText(tooltiptext);
  this->EditBox.SetTooltipText(tooltiptext);
}

