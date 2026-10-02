// window/introduction.cpp
// Game introduction/tutorial
// Decompiled from: gameSplit/swindow.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "introduction.h"
#include "properties.h"

// Classes: SIntroduction
// Function count: 5

//----- (00489AC0) --------------------------------------------------------

SIntroduction::~SIntroduction()

{
  SProperties *IntroTextsIni; // edi
  IntroTextsIni = this->IntroTextsIni;
  if ( IntroTextsIni )
  {
    IntroTextsIni->~SProperties();
    ::operator delete(IntroTextsIni);
    this->IntroTextsIni = 0;
  }
  Board->SetPixelRounding(1);
  Board->ReleaseFont(this->BottomGradientFont);
  Board->ReleaseFont(this->TopGradientFont);
  Concert->Update(1);
  // TextListBox is a value member — C++ calls ~SListBox() automatically
  // base destructor called automatically
}

//----- (00489C50) --------------------------------------------------------

void SIntroduction::Create(int a2, int race)

{
  char fullpath[260] = {0};
  int v5;
  const char *v7; // eax
  char *String; // eax
  int Count;
  int v10;
  int v11;
  int v12;
  int v13;
  int v14; // kr08_4
  const char *v16; // eax
  double v17; // st7
  int v19;
  int v20;
  int w;
  int v22;
  int rowHeight;
  SSkippableDXWidget::Create(a2, 300257, 1);
  Board->SetPixelRounding(0);
  this->InsertChild(&this->TextListBox);
  this->TextListBox.SetPosition(this->Width / 2, this->Height / 2 + 300, 800, 600);
  this->TextListBox.Create(13, 0xFFu, 0, 0, 0, 0, 2, 0);
  v5 = race;
  v7 = "Rabbit";
  if ( race )
    v7 = "Pig";
  String = this->IntroTextsIni->GetString(v7, "Text", fullpath);
  this->TextListBox.AddItem(String, 0xF0F0F0u, 0);
  Board->GetTextExtent(13, 0, 0, &w, &rowHeight, 1.0f);
  Count = this->TextListBox.GetCount();
  this->TextHeight = rowHeight * Count;
  this->BottomGradientFont = Board->LoadSingleFont("menu/gradient_bottom.png", Default);
  v10 = Board->CreateFrame(FT_SPRITE, this->BackFrame, 0, 0, 0, 1);
  this->BottomGradientBackground = v10;
  Board->SetSpriteGlyph(v10, this->BottomGradientFont, 0);
  this->TopGradientFont = Board->LoadSingleFont("menu/gradient_top.png", Default);
  v11 = Board->CreateFrame(FT_SPRITE, this->BackFrame, 0, 0, 0, 1);
  this->TopGradientBackground = v11;
  Board->SetSpriteGlyph(v11, this->TopGradientFont, 0);
  v12 = Board->CreateFrame(FT_BOX, this->BackFrame, 0, 0, 0, 1);
  this->BottomFillBackground = v12;
  Board->SetBoxColor(v12, 0xFF000000u);
  v13 = Board->CreateFrame(FT_BOX, this->BackFrame, 0, 0, 0, 1);
  this->TopFillBackground = v13;
  Board->SetBoxColor(v13, 0xFF000000u);
  this->InsertSkipButton();
  this->Parent->GetPosition(&v19, &v20, &v22, &race);
  this->SetPosition(0, 0, v22, race);
  v14 = this->Height - 600;
  Board->ResizeFrame(this->TopFillBackground, this->Width, v14 / 2 + 1);
  Board->MoveFrame(this->TopGradientBackground, 0, v14 / 2);
  Board->ResizeFrame(this->TopGradientBackground, this->Width, 150);
  Board->MoveFrame(this->BottomGradientBackground, 0, v14 / 2 + 450);
  Board->ResizeFrame(this->BottomGradientBackground, this->Width, 150);
  Board->MoveFrame(this->BottomFillBackground, 0, v14 / 2 + 599);
  Board->ResizeFrame(this->BottomFillBackground, this->Width, v14 / 2 + 1);
  v16 = "videos/r_ismerteto.mp3";
  if ( v5 )
    v16 = "videos/p_ismerteto.mp3";
  v17 = Concert->PlaySound(v16, -5.0f, 0, -1);
  this->AudioDuration = (float)(v17 * 1000.0);
  this->SetFocus();
}

//----- (00489F70) --------------------------------------------------------

void SIntroduction::OnSize(int w, int h)

{
  int v2;
  int p_x;
  int p_y;
  int p_width;
  int p_height;
  this->Parent->GetPosition(&p_x, &p_y, &p_width, &p_height);
  this->SetPosition(0, 0, p_width, p_height);
  v2 = (this->Height - 600) / 2;
  Board->ResizeFrame(this->TopFillBackground, this->Width, v2 + 1);
  Board->MoveFrame(this->TopGradientBackground, 0, v2);
  Board->ResizeFrame(this->TopGradientBackground, this->Width, 150);
  Board->MoveFrame(this->BottomGradientBackground, 0, v2 + 450);
  Board->ResizeFrame(this->BottomGradientBackground, this->Width, 150);
  Board->MoveFrame(this->BottomFillBackground, 0, v2 + 599);
  Board->ResizeFrame(this->BottomFillBackground, this->Width, v2 + 1);
}

//----- (0048A070) --------------------------------------------------------

void SIntroduction::Update()

{
  SSkippableDXWidget::Update();
  this->TextListBox.SetPosition(
    this->Width / 2,
    (int)(float)((float)(this->Height / 2 + 300)
               - (float)((float)((float)this->ElapsedMs / this->AudioDuration) * (float)(this->TextHeight + 600))),
    800,
    600);
  if ((float)this->ElapsedMs > this->AudioDuration)
    this->SendAction(300258, 0);
}

//----- (00489A10) --------------------------------------------------------

SIntroduction::SIntroduction()

{
  this->IntroTextsIni = new SProperties("introductions.ini", 1, 1);
}

