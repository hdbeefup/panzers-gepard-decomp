// window/messageboxwithoutbuttons.cpp
// Message box without buttons
// Decompiled from: gameSplit/swindow.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "messageboxwithoutbuttons.h"
#include "logger.h"

// Classes: SMessageBoxWithoutButtons
// Function count: 5

//----- (0048EA10) --------------------------------------------------------

void SMessageBoxWithoutButtons::Cancel()

{
  Logger.g->Panic(
    "SMessageBoxWithoutButtons::Cancel SMessageBoxWithoutButtons cannot run with DoModal");
}

//----- (0048EA30) --------------------------------------------------------

void SMessageBoxWithoutButtons::Create(BOOL inGame)

{
  int FontWidth;
  int FontHeight;
  this->bInGame = inGame;
  SDialog::Create();
  Board->GetTextExtent(6, 0, 0, &FontWidth, &FontHeight, 1.0f);
  this->InsertChild(&this->ListBox);
  this->ListBox.SetPosition(
    this->Width / 2 - 12,
    (this->Height - (5 * FontHeight + 30)) / 2,
    this->Width - 60,
    0);
  this->ListBox.Create(6, 5u, 0, 0, 0, 0, 2, 1);
}

//----- (0048EAD0) --------------------------------------------------------

void SMessageBoxWithoutButtons::SetText(const char *text)

{
  SListBox *p_ListBox; // esi
  p_ListBox = &this->ListBox;
  this->ListBox.ResetContent();
  p_ListBox->AddItem(text, 0xF0F0F0u, 0);
}

//----- (0048EB00) --------------------------------------------------------

void SMessageBoxWithoutButtons::SetVisible(bool vis)

{
  SDialog::SetVisible(vis);
  if ( !this->bInGame )
    Gepard->RenderScene(0);
}

//----- (004D89C0) --------------------------------------------------------

SMessageBoxWithoutButtons::~SMessageBoxWithoutButtons()

{
  // ListBox is a value member — C++ calls ~SListBox() automatically
  // base destructor called automatically
}

SMessageBoxWithoutButtons::SMessageBoxWithoutButtons()
{
  this->bInGame = false;
}

