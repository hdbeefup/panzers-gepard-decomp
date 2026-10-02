// window/tipdialog.cpp
// Tooltip dialog
// Decompiled from: gameSplit/swindow.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "tipdialog.h"
#include "properties.h"

// Classes: STipDialog
// Function count: 7

//----- (004957D0) --------------------------------------------------------

STipDialog::~STipDialog()

{
  SProperties *TipsIni; // edi
  TipsIni = this->TipsIni;
  if ( TipsIni )
  {
    TipsIni->~SProperties();
    ::operator delete(TipsIni);
    this->TipsIni = 0;
  }
  // ListBox is a value member — C++ calls ~SListBox() automatically
  // base destructor called automatically
}

//----- (004958D0) --------------------------------------------------------

void STipDialog::Cancel()

{
  this->ModalResult = 1;
}

//----- (004958E0) --------------------------------------------------------

void STipDialog::Create()

{
  int v5;
  char *Text; // eax
  STipDialog *v8; // esi
  SListBox *p_ListBox; // ebx
  STipDialog *v10; // ebx
  int Int;
  int v12;
  char *v14; // eax
  char *v15; // eax
  char *v16; // eax
  char *v17; // eax
  unsigned int v20;
  int h;
  int FontWidth[2];
  int FontHeight;
  int w;
  STipDialog *v25;
  v25 = this;
  SDialog::Create();
  this->TipsIni = new SProperties("tips.ini", 1, 1);
  v5 = Board->CreateFrame(FT_TEXT, this->BackFrame, this->Width / 2, 25, 0, 1);
  Text = ::GetText("SWINE_TIPS_SWINE_TIPS");
  Board->SetText(v5, 9, 2, Text);
  Board->GetTextExtent(0, 0, 0, FontWidth, &FontHeight, 1.0f);
  v8 = v25;
  p_ListBox = &this->ListBox;
  v25->InsertChild(p_ListBox);
  p_ListBox->SetPosition(v8->Width / 2 - 12, (v25->Height - 5 * FontHeight - 8) / 2, v8->Width - 60, 0);
  p_ListBox->Create(0, 5u, 0, 0, 0, 0, 2, 1);
  v10 = v25;
  Int = v25->TipsIni->GetInt("Tips", "number of tips", 0);
  v12 = rand();
  v10->ActiveTip = v10->LoadTip(v12 % Int + 1);
  v20 = strlen(GetText("SWINE_OK"));
  v14 = GetText("SWINE_OK");
  Board->GetTextExtent(9, v14, v20, &w, &h, 1.0f);
  v10->InsertChild(&v10->OKButton);
  v10->OKButton.SetPosition(v10->Width / 2, v10->Height - 47, 0, 0);
  v15 = GetText("SWINE_OK");
  v10->OKButton.SetText(v15);
  v10->OKButton.Create((int)v10, 9, 2, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
  v10->InsertChild(&v10->NextButton);
  v10->NextButton.SetPosition(v10->Width / 2 + w / 2 + 20, v10->Height - 47, 0, 0);
  v16 = GetText("SWINE_TIPS_NEXT");
  v10->NextButton.SetText(v16);
  v10->NextButton.Create((int)v10, 6, 0, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
  v10->InsertChild(&v10->PrevButton);
  v10->PrevButton.SetPosition(v10->Width / 2 - w / 2 - 20, v10->Height - 47, 0, 0);
  v17 = GetText("SWINE_TIPS_PREV");
  v10->PrevButton.SetText(v17);
  v10->PrevButton.Create((int)v10, 6, 1, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
}

//----- (00495BD0) --------------------------------------------------------

int STipDialog::LoadTip(int TipNumber)

{
  int Int;
  int v4;
  char *String; // eax
  int v9;
  char *v10; // esi
  char *v12; // edi
  char *i; // eax
  STipDialog *v20;
  void *Src;
  char *buf_str;
  int buf_size;
  v20 = this;
  Int = this->TipsIni->GetInt("Tips", "number of tips", 0);
  v4 = TipNumber;
  if ( TipNumber <= Int )
  {
    if ( TipNumber >= 1 )
      goto LABEL_6;
    v4 = this->TipsIni->GetInt("Tips", "number of tips", 0);
  }
  else
  {
    v4 = 1;
  }
  TipNumber = v4;
LABEL_6:
  char tipKey[260];
  sprintf(tipKey, "tip_%02d", v4);
  String = this->TipsIni->GetString("Tips", tipKey, "");
  Src = String;
  if ( String )
  {
    v9 = strlen(String);
    buf_size = v9;
    v10 = new char[v9 + 1];
    buf_str = v10;
    memcpy(v10, Src, v9 + 1);
  }
  else
  {
    v9 = 0;
    v10 = 0;
    buf_size = 0;
    buf_str = 0;
  }
  if ( v9 > 0 )
  {
    v12 = new char[v9 + 1];
    strcpy(v12, buf_str);
    for ( i = strchr(v12, '#'); i; i = strchr(v12, '#') )
      *i = '\n';
    v20->ListBox.ResetContent();
    v20->ListBox.AddItem(v12, 0xF0F0F0u, 0);
    delete[] v12;
  }
  if ( buf_str )
    delete[] buf_str;
  return TipNumber;
}

//----- (00495D80) --------------------------------------------------------

bool STipDialog::OnAction(SWidget *sender, int action, int param)

{
  if ( action != 271682 )
    return 1;
  if ( sender == &this->OKButton )
  {
    this->ModalResult = 1;
    return 1;
  }
  if ( sender == &this->NextButton )
  {
    ++this->ActiveTip;
LABEL_8:
    this->ActiveTip = this->LoadTip(this->ActiveTip);
    return 1;
  }
  if ( sender == &this->PrevButton )
  {
    --this->ActiveTip;
    goto LABEL_8;
  }
  return 1;
}

//----- (00495DF0) --------------------------------------------------------

bool STipDialog::OnKeyDown(int keycode, bool repeat)

{
  switch ( keycode )
  {
    case 13:
    case 27:
      this->ModalResult = 1;
      return 1;
    case 38:
    case 40:
      return 1;
    case 37:
      --this->ActiveTip;
LABEL_7:
      this->ActiveTip = this->LoadTip(this->ActiveTip);
      return 1;
    case 39:
      ++this->ActiveTip;
      goto LABEL_7;
  }
  return 0;
}

//----- (00495740) --------------------------------------------------------

STipDialog::STipDialog()

{
  this->TipsIni = 0;
}

