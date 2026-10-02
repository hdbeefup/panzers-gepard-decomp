// window/scaler.cpp
// Scaler widget
// Decompiled from: gameSplit/sslider.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "scaler.h"
#include "logger.h"
#include "hdbeefup.h"

// Classes: SScaler
// Function count: 13

//----- (004923C0) --------------------------------------------------------

SScaler::SScaler()

{
  this->acceptEventsWhenHaveChildren = 0;
  this->BackFrame = -1;
  this->Gravity = 0;
  this->ScaleFactor = 1.0;
}

//----- (00492400) --------------------------------------------------------

SScaler::~SScaler()

{
  int BackFrame;
  BackFrame = this->BackFrame;
  if ( BackFrame >= 0 )
    Board->DestroyFrame(BackFrame);
  // base destructor called automatically
}

//----- (004924E0) --------------------------------------------------------

bool SScaler::CanAcceptEvents()

{
  return this->acceptEventsWhenHaveChildren && this->Child;
}

//----- (00492500) --------------------------------------------------------

void SScaler::ChildToParent(int *x, int *y)

{
  *x = (int)(float)((float)*x * this->ScaleFactor);
  *y = (int)(float)((float)*y * this->ScaleFactor);
  *x += this->X;
  *y += this->Y;
}

//----- (00492540) --------------------------------------------------------

void SScaler::Create()

{
  int v3;
  int v4;
  int Height;
  #ifdef HD_DEBUG_MENUS
  Logger.g->Log(0, "SScaler::Create: this=%p Parent=%p Board=%p", this, this->Parent, Board);
  #endif
  v3 = this->Parent->GetFrame();
  #ifdef HD_DEBUG_MENUS
  Logger.g->Log(0, "SScaler::Create: parentFrame=%d", v3);
  #endif
  v4 = Board->CreateFrame((SFrameType)7, v3, this->X, this->Y, this->Gravity, 1);
  #ifdef HD_DEBUG_MENUS
  Logger.g->Log(0, "SScaler::Create: newFrame=%d w=%d h=%d", v4, this->Width, this->Height);
  #endif
  Height = this->Height;
  this->BackFrame = v4;
  Board->ResizeFrame(v4, this->Width, Height);
  Board->ShowFrame(this->BackFrame, this->Visible);
  Board->SetScaleFactor(this->BackFrame, this->ScaleFactor);
  #ifdef HD_DEBUG_MENUS
  Logger.g->Log(0, "SScaler::Create: done");
  #endif
}

//----- (004925B0) --------------------------------------------------------

int SScaler::GetFrame()

{
  return this->BackFrame;
}

//----- (004925C0) --------------------------------------------------------

bool SScaler::IsScaler()

{
  return 1;
}

//----- (004925D0) --------------------------------------------------------

void SScaler::ParentToChild(int *x, int *y)

{
  *x -= this->X;
  *y -= this->Y;
  *x = (int)(float)((float)*x / this->ScaleFactor);
  *y = (int)(float)((float)*y / this->ScaleFactor);
}

//----- (00492610) --------------------------------------------------------

void SScaler::Resize(int width, int height)

{
  int BackFrame;
  SWidget::Resize(width, height);
  BackFrame = this->BackFrame;
  if ( BackFrame >= 0 )
    Board->ResizeFrame(BackFrame, this->Width, this->Height);
}

//----- (00492640) --------------------------------------------------------

void SScaler::SetGravity(int gravity)

{
  int BackFrame;
  SWidget::SetGravity(gravity);
  BackFrame = this->BackFrame;
  if ( BackFrame >= 0 )
    Board->GravitateFrame(BackFrame, this->Gravity);
}

//----- (00492670) --------------------------------------------------------

void SScaler::SetPosition(int x, int y, int width, int height)

{
  int BackFrame;
  SWidget::SetPosition(x, y, width, height);
  BackFrame = this->BackFrame;
  if ( BackFrame >= 0 )
  {
    Board->MoveFrame(BackFrame, this->X, this->Y);
    Board->ResizeFrame(this->BackFrame, this->Width, this->Height);
  }
}

//----- (004926C0) --------------------------------------------------------

void SScaler::SetScaleFactor(float scaleFactor)

{
  int BackFrame;
  BackFrame = this->BackFrame;
  this->ScaleFactor = scaleFactor;
  if ( BackFrame >= 0 )
    Board->SetScaleFactor(BackFrame, scaleFactor);
}

//----- (004926F0) --------------------------------------------------------

void SScaler::SetVisible(bool visible)

{
  SWidget::SetVisible(visible);
  if ( this->BackFrame >= 0 )
    Board->ShowFrame(this->BackFrame, this->Visible);
}

