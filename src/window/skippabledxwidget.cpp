// window/skippabledxwidget.cpp
// Skippable DX widget (cinematics)
// Decompiled from: gameSplit/sdxwidget.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "skippabledxwidget.h"
#include "timer.h"

bool g_SkipButtonClicked = false;

// Classes: SSkippableDXWidget
// Function count: 10

//----- (004927F0) --------------------------------------------------------

void SSkippableDXWidget::Create(int a2, int skipAction, bool buttonMode)

{
  int p_x;
  int p_y;
  int p_width;
  int p_height;
  this->Parent->GetPosition(&p_x, &p_y, &p_width, &p_height);
  this->SetPosition(0, 0, p_width, p_height);
  SDXWidget::Create(a2);
  this->ButtonMode = buttonMode;
  this->SkipAction = skipAction;
  this->SkipVisibleTick = 0;
  this->StartTick = (unsigned int)Timer.GetTickValue();

  this->ElapsedMs = 0;
  this->Cursor = -1;
}

//----- (00492870) --------------------------------------------------------

void SSkippableDXWidget::InsertSkipButton()

{
  STextButton *p_SkipButton; // esi
  char *Text; // eax
  if ( this->ButtonMode )
  {
    p_SkipButton = &this->SkipButton;
    this->InsertChild(&this->SkipButton);
    p_SkipButton->SetPosition(this->Width - 50, this->Height - 50, 0, 0);
    p_SkipButton->SetGravity(10);
    Text = ::GetText("SWINE_SKIP");
    this->SkipButton.SetText(Text);
    this->SkipButton.Create((int)this, 9, 1, 0xF0F0F0u, 0xFFFFFFu, 0x666666u);
    p_SkipButton->SetVisible(0);
  }
}

//----- (004928F0) --------------------------------------------------------

void SSkippableDXWidget::KickButton()

{
  if ( this->ButtonMode )
  {
    this->SkipButton.SetVisible(1);
    this->SkipVisibleTick = (unsigned int)Timer.GetTickValue();

  }
}

//----- (00492960) --------------------------------------------------------

bool SSkippableDXWidget::OnKeyDown(int keycode, bool repeat)

{
  bool v3;
  if ( keycode != 13 && keycode != 27 )
    return 0;
  v3 = !this->ButtonMode;
  this->Cursor = 3;
  if ( v3 || this->SkipButton.Visible )
  {
    this->SendAction(this->SkipAction, 0);
    return 1;
  }
  else
  {
    this->KickButton();
    return 1;
  }
}

//----- (00492A70) --------------------------------------------------------

void SSkippableDXWidget::Update()

{
  int TickValue;
  bool v3;
  TickValue = (unsigned int)Timer.GetTickValue();

  v3 = !this->ButtonMode;
  this->ElapsedMs = TickValue - this->StartTick;
  if ( !v3 && this->SkipButton.Visible && TickValue > this->SkipVisibleTick + this->SkipVisibleInterval )
  {
    this->SkipButton.SetVisible(0);
    this->Cursor = -1;
  }
}

//----- (004929B0) --------------------------------------------------------

void SSkippableDXWidget::OnMouseDown(int button, int x, int y, int shift)

{
  bool v6;
  SDXWidget::OnMouseDown(button, x, y, shift);
  v6 = !this->ButtonMode;
  this->Cursor = 3;
  if ( v6 )
  {
    this->SendAction(this->SkipAction, 0);
  }
  else
  {
    this->SkipButton.SetVisible(1);
    this->SkipVisibleTick = (unsigned int)Timer.GetTickValue();

  }
}

//----- (00492A20) --------------------------------------------------------

void SSkippableDXWidget::OnMouseMove(int x, int y, int shift)

{
  bool v6;
  SDXWidget::OnMouseMove(x, y, shift);
  v6 = !this->ButtonMode;
  this->Cursor = 3;
  if ( !v6 )
  {
    this->SkipButton.SetVisible(1);
    this->SkipVisibleTick = (unsigned int)Timer.GetTickValue();

  }
}

//----- (00492720) --------------------------------------------------------

SSkippableDXWidget::SSkippableDXWidget()

{
  this->SkipVisibleInterval = 2000;
}

//----- (00492790) --------------------------------------------------------

SSkippableDXWidget::~SSkippableDXWidget()

{
  // base destructor called automatically
  // base destructor called automatically
}

//----- (00492920) --------------------------------------------------------

bool SSkippableDXWidget::OnAction(SWidget *sender, int action, int param)

{
  if ( !this->ButtonMode || action != 271682 || sender != &this->SkipButton )
    return 0;
  // Set global flag — SendAction from button click handler crashes in our build
  // (D3D9 CreateTexture pumps messages during STextButton::OnMouseUp → Update → RerenderText)
  g_SkipButtonClicked = true;
  return 1;
}
