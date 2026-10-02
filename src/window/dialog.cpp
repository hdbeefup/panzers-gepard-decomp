// window/dialog.cpp
// Dialog window
// Decompiled from: gameSplit/swidget.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "dialog.h"
#include "window.h"

extern unsigned char g_MenuRace;

// Classes: SDialog
// Function count: 3

//----- (00484C60) --------------------------------------------------------

SDialog::SDialog()

{
}

//----- (00484CB0) --------------------------------------------------------

void SDialog::Create()

{
  int v3;
  int v4;
  int x;
  int y;
  int width;
  int height;
  if ( g_MenuRace )
    v3 = Board->LoadSingleFont("menu/pig_message.png", (HDMode)2);
  else
    v3 = Board->LoadSingleFont("menu/rabbit_message.png", (HDMode)2);
  v4 = v3;
  this->Parent->GetPosition(&x, &y, &width, &height);
  this->SetPosition((width - 384) / 2, (height - 192) / 2, 384, 192);
  this->SetGravity(5);
  this->SetBackgroundSprite(v4, 0, 1, 0);
  SDXWidget::Create(v4);
  Board->ReleaseFont(v4);
  this->Cursor = 3;
  this->ModalResult = 0;
}

//----- (00484D70) --------------------------------------------------------

void SDialog::Cancel()
{
  this->ModalResult = 2;
}

//----- (00484D70) --------------------------------------------------------

int SDialog::DoModal()

{
  int result;
  SWindow *WindowParent; // eax
  this->SetFocus();
  for ( result = this->ModalResult; !result; result = this->ModalResult )
  {
    WindowParent = this->GetWindowParent();
    if ( WindowParent->RunModal(this) )
      this->Cancel();
  }
  return result;
}

