// window/messagebox.cpp
// Message box dialog
// Decompiled from: gameSplit/swindow.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "messagebox.h"
#include "logger.h"

// Classes: SMessageBox
// Function count: 7

//----- (00486E60) --------------------------------------------------------

SMessageBox::~SMessageBox()

{
  // base destructor called automatically
  // base destructor called automatically
  // base destructor called automatically
  // base destructor called automatically
  // base destructor called automatically
  // base destructor called automatically
  // base destructor called automatically
  // base destructor called automatically
}

//----- (0048E320) --------------------------------------------------------

SMessageBox::SMessageBox()

{
}

//----- (0048E460) --------------------------------------------------------

void SMessageBox::Cancel()

{
  int Type;
  Type = this->Type;
  if ( Type )
  {
    if ( Type == 4 )
    {
      this->ModalResult = 7;
    }
    else if ( Type == 3 )
    {
      this->ModalResult = 2;
    }
  }
  else
  {
    this->ModalResult = 1;
  }
}

//----- (0048E4A0) --------------------------------------------------------

void SMessageBox::Create(const char *text, int type)

{
  SMessageBox *v4; // ebx
  char *v5; // eax
  char *v6; // eax
  STextButton *p_NoButton; // edi
  char *v8; // eax
  char *v9; // eax
  char *v10; // eax
  char *v11; // eax
  int FontWidth;
  int FontHeight;
  SMessageBox *v14;
  v14 = this;
  SDialog::Create();
  Board->GetTextExtent(6, 0, 0, &FontWidth, &FontHeight, 1.0f);
  this->InsertChild(&this->ListBox);
  this->ListBox.SetPosition(
    this->Width / 2 - 12,
    (v14->Height - (5 * FontHeight + 30)) / 2,
    this->Width - 60,
    0);
  this->ListBox.Create(6, 5u, 0, 0, 0, 0, 2, 1);
  this->ListBox.AddItem(text, 0xF0F0F0u, 0);
  v4 = v14;
  v14->Type = type;
  if ( type <= 4 )
  {
    switch ( type )
    {
      case 4:
        v4->InsertChild(&v4->YesButton);
        v4->YesButton.SetPosition(v4->Width / 2 - 50, v4->Height - 47, 0, 0);
        v9 = GetText("SWINE_YES");
        v4->YesButton.SetText(v9);
        v4->YesButton.Create((int)&v4->YesButton, 9, 2, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
        p_NoButton = &v4->NoButton;
        v4->InsertChild(&v4->NoButton);
        v4->NoButton.SetPosition(v4->Width / 2 + 50, v4->Height - 47, 0, 0);
        v8 = GetText("SWINE_NO");
        goto LABEL_6;
      case 0:
        p_NoButton = &v4->OKButton;
        v4->InsertChild(&v4->OKButton);
        v4->OKButton.SetPosition(v4->Width / 2, v4->Height - 47, 0, 0);
        v8 = GetText("SWINE_OK");
        goto LABEL_6;
      case 3:
        v4->InsertChild(&v4->YesButton);
        v4->YesButton.SetPosition(v4->Width / 2 - 70, v4->Height - 47, 0, 0);
        v5 = GetText("SWINE_YES");
        v4->YesButton.SetText(v5);
        v4->YesButton.Create((int)&v4->YesButton, 9, 2, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
        v4->InsertChild(&v4->NoButton);
        v4->NoButton.SetPosition(v4->Width / 2, v4->Height - 47, 0, 0);
        v6 = GetText("SWINE_NO");
        v4->NoButton.SetText(v6);
        v4->NoButton.Create((int)&v4->NoButton, 9, 2, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
        p_NoButton = &v4->CancelButton;
        v4->InsertChild(&v4->CancelButton);
        v4->CancelButton.SetPosition(v4->Width / 2 + 70, v4->Height - 47, 0, 0);
        v8 = GetText("SWINE_CANCEL");
LABEL_6:
        p_NoButton->SetText(v8);
        p_NoButton->Create((int)p_NoButton, 9, 2, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
        return;
    }
LABEL_13:
    Logger.g->Panic("SMessageBox::Create: Unsupported messagebox type");
  }
  if ( type != 1971 )
  {
    if ( type != 1972 )
      goto LABEL_13;
    v4->InsertChild(&v4->RetryButton);
    v4->RetryButton.SetPosition(v4->Width / 2 - 20, v4->Height - 47, 0, 0);
    v10 = GetText("SWINE_RETRY");
    v4->RetryButton.SetText(v10);
    v4->RetryButton.Create((int)&v4->RetryButton, 9, 1, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
    v4->InsertChild(&v4->IgnoreButton);
    v4->IgnoreButton.SetPosition(v4->Width / 2 + 20, v4->Height - 47, 0, 0);
    v11 = GetText("SWINE_IGNORE");
    v4->IgnoreButton.SetText(v11);
    v4->IgnoreButton.Create((int)&v4->IgnoreButton, 9, 0, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
  }
}

//----- (0048E830) --------------------------------------------------------

bool SMessageBox::OnAction(SWidget *sender, int action, int param)

{
  bool result; // al
  if ( action != 271682 )
    return 1;
  if ( sender == &this->OKButton )
  {
    this->ModalResult = 1;
    return 1;
  }
  if ( sender == &this->YesButton )
  {
    this->ModalResult = 6;
    return 1;
  }
  else if ( sender == &this->NoButton )
  {
    this->ModalResult = 7;
    return 1;
  }
  else if ( sender == &this->RetryButton )
  {
    this->ModalResult = 4;
    return 1;
  }
  else if ( sender == &this->IgnoreButton )
  {
    this->ModalResult = 5;
    return 1;
  }
  else
  {
    result = 1;
    if ( sender == &this->CancelButton )
      this->ModalResult = 2;
  }
  return result;
}

//----- (0048E8E0) --------------------------------------------------------

bool SMessageBox::OnChar(int code)

{
  int v3;
  int Type;
  switch ( code )
  {
    case 'n':
    case 'N':
      Type = this->Type;
      if ( Type == 3 || Type == 4 )
        this->ModalResult = 7;
      break;
    case 'y':
    case 'Y':
      v3 = this->Type;
      if ( v3 == 3 || v3 == 4 )
      {
        this->ModalResult = 6;
        return 1;
      }
      break;
    case 'r':
    case 'R':
      if ( this->Type == 1972 )
      {
        this->ModalResult = 4;
        return 1;
      }
      break;
    default:
      if ( (code == 105 || code == 73) && this->Type == 1972 )
      {
        this->ModalResult = 5;
        return 1;
      }
      break;
  }
  return 1;
}

//----- (0048E990) --------------------------------------------------------

bool SMessageBox::OnKeyDown(int keycode, bool repeat)

{
  int Type;
  switch ( keycode )
  {
    case 13:
      Type = this->Type;
      if ( !Type )
      {
        this->ModalResult = 1;
        return 1;
      }
      if ( Type != 4 )
      {
        if ( Type == 1972 )
        {
          this->ModalResult = 5;
          return 1;
        }
        if ( Type != 3 )
          return 1;
      }
      this->ModalResult = 6;
      return 1;
    case 27:
      this->Cancel();
      return 1;
    case 122:
      return 0;
    default:
      return 1;
  }
}

