// window/inputdialog.cpp
// Input dialog
// Decompiled from: gameSplit/swindow.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "inputdialog.h"
#include "logger.h"

// Classes: SInputDialog
// Function count: 5

//----- (0048C650) --------------------------------------------------------

void SInputDialog::Cancel()

{
  int Type;
  Type = this->Type;
  if ( Type )
  {
    if ( Type == 4 )
    {
      this->ModalResult = 7;
    }
    else if ( Type == 1 )
    {
      this->ModalResult = 2;
    }
  }
  else
  {
    this->ModalResult = 1;
  }
}

//----- (0048C690) --------------------------------------------------------

void SInputDialog::Create(int a2, const char *text, const char *prompt, const char *edittext, int type)

{
  int v7;
  char *v8; // eax
  STextButton *p_CancelButton; // edi
  const char *v10; // eax
  char *v11; // eax
  char *v12; // eax
  SDialog::Create();
  v7 = Board->CreateFrame(FT_TEXT, this->BackFrame, this->Width / 2, 25, 0, 1);
  Board->SetText(v7, 6, 2, text);
  this->InsertChild(&this->EditBox);
  this->EditBox.SetPosition(30, 87, this->Width - 60, 20);
  this->EditBox.Create(a2, 0, 0, prompt);
  this->EditBox.SetText(edittext);
  this->Type = type;
  if ( type )
  {
    if ( type == 1 )
    {
      this->InsertChild(&this->OKButton);
      this->OKButton.SetPosition(this->Width / 2 - 50, this->Height - 47, 0, 0);
      v11 = GetText("SWINE_OK");
      this->OKButton.SetText(v11);
      this->OKButton.Create((int)this, 9, 2, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
      p_CancelButton = &this->CancelButton;
      this->InsertChild(&this->CancelButton);
      this->CancelButton.SetPosition(this->Width / 2 + 50, this->Height - 47, 0, 0);
      v10 = "SWINE_CANCEL";
    }
    else
    {
      if ( type != 4 )
        Logger.g->Panic("SMessageBox::Create: Unsupported messagebox type");
      this->InsertChild(&this->YesButton);
      this->YesButton.SetPosition(this->Width / 2 - 50, this->Height - 47, 0, 0);
      v8 = GetText("SWINE_YES");
      this->YesButton.SetText(v8);
      this->YesButton.Create((int)this, 9, 2, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
      p_CancelButton = &this->NoButton;
      this->InsertChild(&this->NoButton);
      this->NoButton.SetPosition(this->Width / 2 + 50, this->Height - 47, 0, 0);
      v10 = "SWINE_NO";
    }
  }
  else
  {
    p_CancelButton = &this->OKButton;
    this->InsertChild(&this->OKButton);
    this->OKButton.SetPosition(this->Width / 2, this->Height - 47, 0, 0);
    v10 = "SWINE_OK";
  }
  v12 = GetText(v10);
  p_CancelButton->SetText(v12);
  p_CancelButton->Create((int)this, 9, 2, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
  this->EditBox.SetFocusToInnerEditBox();
}

//----- (0048C8D0) --------------------------------------------------------

bool SInputDialog::OnAction(SWidget *sender, int action, int param)

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
  else
  {
    result = 1;
    if ( sender == &this->CancelButton )
      this->ModalResult = 2;
  }
  return result;
}

//----- (0048C950) --------------------------------------------------------

bool SInputDialog::OnKeyDown(int keycode, bool repeat)

{
  int Type;
  switch ( keycode )
  {
    case 13:
      Type = this->Type;
      if ( Type )
      {
        if ( Type == 4 )
        {
          this->ModalResult = 6;
          return 1;
        }
        if ( Type != 1 )
          return 1;
      }
      this->ModalResult = 1;
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

//----- (0048C540) --------------------------------------------------------

SInputDialog::SInputDialog()

{
  // EditBox member constructed automatically
}

SInputDialog::~SInputDialog()

{
}

