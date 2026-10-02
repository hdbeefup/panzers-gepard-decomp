// window/widget.cpp
// Base widget class
// Decompiled from: gameSplit/swidget.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "widget.h"
#include "window.h"
#include "scaler.h"
#include "logger.h"
#include "timer.h"

// Classes: SWidget
// Function count: 48

//----- (00482B00) --------------------------------------------------------

bool SWidget::CanAcceptEvents()

{
  return true;
}

//----- (00482B80) --------------------------------------------------------

bool SWidget::IsScaler()

{
  return 0;
}

//----- (00482B90) --------------------------------------------------------

bool SWidget::IsWindow()

{
  return 0;
}

//----- (00482BA0) --------------------------------------------------------

bool SWidget::OnAction(SWidget *source, int action, int param)

{
  return false;
}

//----- (00482BB0) --------------------------------------------------------

bool SWidget::OnChar(int code)

{
  return 0;
}

//----- (00482BC0) --------------------------------------------------------

bool SWidget::OnKeyDown(int keycode, bool repeat)

{
  return 0;
}

//----- (00482BD0) --------------------------------------------------------

bool SWidget::OnKeyUp(int keycode)

{
  return 0;
}

//----- (00482D70) --------------------------------------------------------

void SWidget::OnMove(int x, int y)

{
  this->X = x;
  this->Y = y;
}

//----- (00495E60) --------------------------------------------------------

SWidget::SWidget()

{
  this->Width = 0;
  this->Height = 0;
  this->Left = 0;
  this->Right = 0;
  this->Up = 0;
  this->Down = 0;
  this->X = 0;
  this->Y = 0;
  this->Parent = 0;
  this->Child = 0;
  this->Sibling = 0;
  this->Enabled = true;
  this->Visible = true;
  this->Cursor = -1;
  this->Focus = 0;
  this->FocusSibling = 0;
}

//----- (00495EE0) --------------------------------------------------------

SWidget::~SWidget()

{
  bool v2;
  SWidget *Parent; // ecx
  SWidget *v4; // eax
  SWidget *v5; // eax
  int v6;
  SHeap<TimerItem>::Element *v7; // eax
  v2 = this->Child == 0;
  if ( !v2 )
    Logger.g->Panic("SWidget::~SWidget: Children widgets should be removed first");
  Parent = this->Parent;
  if ( Parent )
    Parent->RemoveChild(this);
  v4 = SWidget::CaptureTarget;
  if ( this == SWidget::CaptureTarget )
    v4 = 0;
  SWidget::CaptureTarget = v4;
  v5 = SWidget::LastMouseTarget;
  if ( this == SWidget::LastMouseTarget )
    v5 = 0;
  SWidget::LastMouseTarget = v5;
LABEL_9:
  v6 = -1;
  while ( 1 )
  {
    if ( ++v6 >= TimerList.size )
    {
LABEL_14:
      v6 = -1;
    }
    else
    {
      v7 = &TimerList.array[v6];
      while ( v7->use != 0x7FFFFFFF )
      {
        ++v6;
        ++v7;
        if ( v6 >= TimerList.size )
          goto LABEL_14;
      }
    }
    if ( v6 < 0 )
      break;
    if ( this == TimerList.array[v6].data.Target && v6 < TimerList.size && TimerList.array[v6].use == 0x7FFFFFFF )
    {
      ::KillTimer(0, TimerList.array[v6].data.IDEvent);
      if ( v6 >= TimerList.size || TimerList.array[v6].use != 0x7FFFFFFF )
        Logger.g->Panic("SHeap::Remove: invalid index (%d)", v6);
      TimerList.array[v6].use = TimerList.nextempty;
      --TimerList.occupied;
      TimerList.nextempty = v6;
      goto LABEL_9;
    }
  }
}

//----- (00496150) --------------------------------------------------------

void SWidget::CaptureMouse()

{
  SWindow *i; // esi
  HWND v2;
  SWidget::CaptureTarget = this;
  for ( i = (SWindow *)this->Parent; i; i = (SWindow *)i->Parent )
  {
    if ( i->IsWindow() )
      break;
  }
  v2 = (HWND)*i;
  SetCapture(v2);
}

//----- (00496190) --------------------------------------------------------

void SWidget::ChildToParent(int *x, int *y)

{
  *x += this->X;
  *y += this->Y;
}

//----- (004961B0) --------------------------------------------------------

int SWidget::GetCurrentCursor()

{
  SWidget *v1; // eax
  v1 = SWidget::LastMouseTarget;
  if ( !SWidget::LastMouseTarget )
    return -1;
  while ( v1->Cursor < 0 )
  {
    v1 = v1->Parent;
    if ( !v1 )
      return -1;
  }
  return v1->Cursor;
}

//----- (004961E0) --------------------------------------------------------

SWidget *SWidget::GetEventTarget(int x, int y, int *target_x, int *target_y)

{
  int v5;
  SWidget *Child; // esi
  SWidget *result; // eax
  int child_x;
  int child_y;
  v5 = y;
  Child = this->Child;
  if ( Child )
  {
    while ( 1 )
    {
      if ( Child->Visible && Child->Enabled )
      {
        child_x = x;
        child_y = v5;
        Child->ParentToChild(&child_x, &child_y);
        if ( child_x >= 0 && child_x < Child->Width && child_y >= 0 && child_y < Child->Height )
        {
          result = Child->GetEventTarget(child_x, child_y, target_x, target_y);
          if ( result )
            break;
        }
      }
      Child = Child->Sibling;
      if ( !Child )
        goto LABEL_10;
    }
  }
  else
  {
LABEL_10:
    if ( this->CanAcceptEvents() )
    {
      *target_x = x;
      *target_y = v5;
      if ( SWidget::LastMouseTarget != this )
      {
        if ( SWidget::LastMouseTarget )
          SWidget::LastMouseTarget->OnMouseOut();
        SWidget::LastMouseTarget = this;
        this->OnMouseOver();
      }
      return this;
    }
    else
    {
      return 0;
    }
  }
  return result;
}

//----- (004962A0) --------------------------------------------------------

SWidget *SWidget::GetFocusTargetFromKeyCode(int keycode)

{
  SWidget *result; // eax
  switch ( keycode )
  {
    case '%':
      result = this->Left;
      if ( !result )
        goto LABEL_18;
      while ( !result->Enabled )
      {
        if ( !result->Left )
          goto LABEL_18;
        result = result->Left;
      }
      break;
    case '&':
      result = this->Up;
      if ( !result )
        goto LABEL_18;
      while ( !result->Enabled )
      {
        if ( !result->Up )
          goto LABEL_18;
        result = result->Up;
      }
      break;
    case '\'':
      result = this->Right;
      if ( !result )
        goto LABEL_18;
      while ( !result->Enabled )
      {
        if ( !result->Right )
          goto LABEL_18;
        result = result->Right;
      }
      break;
    case '(':
      result = this->Down;
      if ( !result )
        goto LABEL_18;
      while ( !result->Enabled )
      {
        if ( !result->Down )
          goto LABEL_18;
        result = result->Down;
      }
      break;
    default:
LABEL_18:
      result = 0;
      break;
  }
  return result;
}

//----- (00496360) --------------------------------------------------------

SWidget *SWidget::GetFocusTargetFromParam(int param)

{
  SWidget *result; // eax
  switch ( param )
  {
    case 0:
      result = this->Up;
      if ( !result )
        goto LABEL_19;
      while ( !result->Enabled )
      {
        if ( !result->Up )
          goto LABEL_18;
        result = result->Up;
      }
      break;
    case 1:
      result = this->Down;
      if ( !result )
        goto LABEL_19;
      while ( !result->Enabled )
      {
        if ( !result->Down )
          goto LABEL_18;
        result = result->Down;
      }
      break;
    case 2:
      result = this->Left;
      if ( !result )
        goto LABEL_19;
      while ( !result->Enabled )
      {
        if ( !result->Left )
          goto LABEL_18;
        result = result->Left;
      }
      break;
    case 3:
      result = this->Right;
      if ( !result )
        goto LABEL_19;
      while ( !result->Enabled )
      {
        if ( !result->Right )
        {
LABEL_18:
          if ( !result->Enabled )
            goto LABEL_19;
          return result;
        }
        result = result->Right;
      }
      break;
    default:
LABEL_19:
      result = 0;
      break;
  }
  return result;
}

//----- (00496420) --------------------------------------------------------

int SWidget::GetFrame()

{
  return -1;
}

//----- (00496430) --------------------------------------------------------

SWidget *SWidget::GetKeyEventTarget()

{
  SWidget *result = this;
  SWidget *i; // eax
  for ( i = this->Focus; i; i = i->Focus )
    result = i;
  return result;
}

//----- (00496450) --------------------------------------------------------

void SWidget::GetPosition(int *x, int *y, int *width, int *height)

{
  *x = this->X;
  *y = this->Y;
  *width = this->Width;
  *height = this->Height;
}

//----- (00496480) --------------------------------------------------------

void SWidget::GetWindowOrParentScalerPosition(int *x, int *y, float *scaleFactor)

{
  SWidget *Parent; // esi
  Parent = this->Parent;
  *x = this->X;
  *y = this->Y;
  if ( !Parent )
    goto LABEL_5;
  while ( !Parent->IsWindow() && !Parent->IsScaler() )
  {
    Parent->ChildToParent(x, y);
    Parent = Parent->Parent;
    if ( !Parent )
      goto LABEL_5;
  }
  if ( Parent->IsScaler() )
    *scaleFactor = ((SScaler *)Parent)->ScaleFactor;
  else
LABEL_5:
    *scaleFactor = 1.0f;
}

//----- (00496500) --------------------------------------------------------

SWidget *SWidget::GetWindowOrScalerParent()

{
  SWidget *i; // esi
  for ( i = this->Parent; i; i = i->Parent )
  {
    if ( i->IsWindow() )
      break;
    if ( i->IsScaler() )
      break;
  }
  return i;
}

//----- (00496530) --------------------------------------------------------

SWindow *SWidget::GetWindowParent()

{
  SWidget *i; // esi
  for ( i = this->Parent; i; i = i->Parent )
  {
    if ( i->IsWindow() )
      break;
  }
  return (SWindow *)i;
}

//----- (00496550) --------------------------------------------------------

bool SWidget::HasMouseCaptured()

{
  return SWidget::CaptureTarget == this;
}

//----- (00496560) --------------------------------------------------------

void SWidget::InsertChild(SWidget *child)

{
  if ( !child || child->Parent )
    Logger.g->Panic("SWidget::InsertChild: Invalid parameter");
  child->Parent = this;
  child->Sibling = this->Child;
  this->Child = child;
}

//----- (004965A0) --------------------------------------------------------

void SWidget::KillTimer(int *id)

{
  int v2;
  int v3;
  v2 = *id;
  if ( *id >= 0 && v2 < TimerList.size && TimerList.array[v2].use == 0x7FFFFFFF )
  {
    ::KillTimer(0, TimerList.array[v2].data.IDEvent);
    v3 = *id;
    if ( *id < 0 || v3 >= TimerList.size || TimerList.array[v3].use != 0x7FFFFFFF )
      Logger.g->Panic("SHeap::Remove: invalid index (%d)", *id);
    TimerList.array[v3].use = TimerList.nextempty;
    --TimerList.occupied;
    TimerList.nextempty = v3;
    *id = -1;
  }
}

//----- (00496630) --------------------------------------------------------

void SWidget::OnMouseWheel(int button, int x, int y, int delta)

{
  SWidget *Parent; // esi
  Parent = this->Parent;
  if ( Parent )
    Parent->OnMouseWheel(button, x + this->X, y + this->Y, delta);
}

//----- (00496660) --------------------------------------------------------

void SWidget::ParentToChild(int *x, int *y)

{
  *x -= this->X;
  *y -= this->Y;
}

//----- (00496680) --------------------------------------------------------

void SWidget::ReleaseFocus()

{
  SWidget *Parent; // eax
  if ( this->Enabled && this->Visible )
  {
    Parent = this->Parent;
    if ( Parent )
    {
      if ( Parent->Focus == this )
        Parent->Focus = 0;
    }
  }
}

//----- (004966A0) --------------------------------------------------------

void SWidget::ReleaseMouse()

{
  if ( SWidget::CaptureTarget == this )
  {
    SWidget::CaptureTarget = 0;
    ReleaseCapture();
  }
}

//----- (004966C0) --------------------------------------------------------

void SWidget::RemoveChild(SWidget *child)

{
  SWidget *v3; // edx
  SWidget *v4; // ecx
  SWidget *i; // eax
  v3 = this->Child;
  if ( !v3 || !child || child->Parent != this )
    goto LABEL_16;
  do
  {
    if ( v3->FocusSibling == child )
      v3->FocusSibling = child->FocusSibling;
    v3 = v3->Sibling;
  }
  while ( v3 );
  if ( this->Focus == child )
    this->Focus = child->FocusSibling;
  v4 = this->Child;
  if ( v4 == child )
  {
    this->Child = child->Sibling;
    child->Parent = 0;
    child->Sibling = 0;
    return;
  }
  for ( i = v4->Sibling; i; i = i->Sibling )
  {
    if ( i == child )
      break;
    v4 = i;
  }
  if ( !v4->Sibling )
LABEL_16:
    Logger.g->Panic("SWidget::RemoveChild: Invalid parameter");
  v4->Sibling = child->Sibling;
  child->Parent = 0;
  child->Sibling = 0;
}

//----- (00496770) --------------------------------------------------------

void SWidget::Resize(int width, int height)

{
  SWidget *Child; // esi
  int Gravity;
  int v6;
  int v7;
  Child = this->Child;
  if ( Child )
  {
    while ( 1 )
    {
      Gravity = Child->Gravity;
      if ( (Gravity & 1) != 0 )
      {
        v6 = width / 2 - this->Width / 2;
      }
      else
      {
        if ( (Gravity & 2) == 0 )
          goto LABEL_7;
        v6 = width - this->Width;
      }
      Child->X += v6;
LABEL_7:
      if ( (Gravity & 4) != 0 )
      {
        Child->Y += height / 2 - this->Height / 2;
      }
      else if ( (Gravity & 8) != 0 )
      {
        v7 = height;
        Child->Y += height - this->Height;
        goto LABEL_10;
      }
      v7 = height;
LABEL_10:
      Child = Child->Sibling;
      if ( !Child )
      {
        this->Width = width;
        this->Height = v7;
        return;
      }
    }
  }
  this->Width = width;
  this->Height = height;
}

//----- (00496810) --------------------------------------------------------

void SWidget::SendAction(int action, int param)

{
  SWidget *i; // esi
  for ( i = this; i; i = i->Parent )
  {
    if ( i->OnAction(this, action, param) )
      break;
  }
}

//----- (00496850) --------------------------------------------------------

void SWidget::SetEnable(bool enable)

{
  SWidget *Parent; // edx
  this->Enabled = enable;
  if ( !enable )
  {
    if ( SWidget::CaptureTarget != this || (SWidget::CaptureTarget = 0, !this->Enabled) )
    {
      Parent = this->Parent;
      if ( Parent->Focus == this )
        Parent->Focus = this->FocusSibling;
    }
  }
  this->Update();
}

//----- (00496890) --------------------------------------------------------

void SWidget::SetFocus()

{
  SWidget *Parent; // edx
  SWidget *Child; // eax
  if ( this->Enabled && this->Visible )
  {
    Parent = this->Parent;
    if ( Parent )
    {
      if ( Parent->Focus != this )
      {
        Child = Parent->Child;
        if ( Child )
        {
          do
          {
            if ( Child->FocusSibling == this )
              Child->FocusSibling = 0;
            Child = Child->Sibling;
          }
          while ( Child );
          Parent = this->Parent;
        }
        this->FocusSibling = Parent->Focus;
        Parent->Focus = this;
        Parent = this->Parent;
      }
      Parent->SetFocus();
    }
  }
}

//----- (004968E0) --------------------------------------------------------

void SWidget::SetGravity(int gravity)

{
  this->Gravity = gravity;
}

//----- (004968F0) --------------------------------------------------------

void SWidget::SetMouseTarget()

{
  if ( SWidget::LastMouseTarget != this )
  {
    if ( SWidget::LastMouseTarget )
      SWidget::LastMouseTarget->OnMouseOut();
    SWidget::LastMouseTarget = this;
    this->OnMouseOver();
  }
}

//----- (00496920) --------------------------------------------------------

void SWidget::SetNavigation(SWidget *Left, SWidget *Right, SWidget *Up, SWidget *Down)

{
  this->Left = Left;
  this->Right = Right;
  this->Up = Up;
  this->Down = Down;
}

//----- (00496940) --------------------------------------------------------

void SWidget::SetPosition(int x, int y, int width, int height)

{
  SWidget *Child; // esi
  int Gravity;
  int v8;
  int v9;
  Child = this->Child;
  this->X = x;
  this->Y = y;
  if ( Child )
  {
    while ( 1 )
    {
      Gravity = Child->Gravity;
      if ( (Gravity & 1) != 0 )
      {
        v8 = width / 2 - this->Width / 2;
      }
      else
      {
        if ( (Gravity & 2) == 0 )
          goto LABEL_7;
        v8 = width - this->Width;
      }
      Child->X += v8;
LABEL_7:
      if ( (Gravity & 4) != 0 )
      {
        Child->Y += height / 2 - this->Height / 2;
      }
      else if ( (Gravity & 8) != 0 )
      {
        v9 = height;
        Child->Y += height - this->Height;
        goto LABEL_10;
      }
      v9 = height;
LABEL_10:
      Child = Child->Sibling;
      if ( !Child )
      {
        this->Width = width;
        this->Height = v9;
        return;
      }
    }
  }
  this->Width = width;
  this->Height = height;
}

//----- (004969F0) --------------------------------------------------------

int SWidget::SetTimer(UINT elapse)

{
  int nextempty;
  SWidget *v3; // esi
  int v4;
  int size;
  int v6;
  SHeap<TimerItem>::Element *array; // esi
  int result;
  ++TimerList.occupied;
  nextempty = TimerList.nextempty;
  v3 = this;
  if ( TimerList.nextempty < 0 )
  {
    size = TimerList.size;
    if ( TimerList.size == TimerList.maxsize )
    {
      if ( TimerList.maxsize >= 16 )
        v6 = 6 * TimerList.maxsize / 5;
      else
        v6 = 16;
      TimerList.array = (SHeap<TimerItem>::Element *)realloc(TimerList.array, sizeof(SHeap<TimerItem>::Element) * v6);
      memset(&TimerList.array[TimerList.maxsize], 0, sizeof(SHeap<TimerItem>::Element) * (v6 - TimerList.maxsize));
      size = TimerList.size;
      TimerList.maxsize = v6;
      v3 = this;
    }
    TimerList.array[size].use = 0x7FFFFFFF;
    nextempty = TimerList.size++;
  }
  else
  {
    v4 = TimerList.nextempty;
    TimerList.nextempty = TimerList.array[TimerList.nextempty].use;
    TimerList.array[v4].use = 0x7FFFFFFF;
    memset(&TimerList.array[v4].data, 0, sizeof(TimerItem));
  }
  TimerList.array[nextempty].data.Target = v3;
  array = TimerList.array;
  array[nextempty].data.IDEvent = ::SetTimer(0, 0, elapse, TimerProc);
  if ( TimerList.array[nextempty].data.IDEvent )
    return nextempty;
  if ( nextempty < 0 || nextempty >= TimerList.size || TimerList.array[nextempty].use != 0x7FFFFFFF )
    Logger.g->Panic("SHeap::Remove: invalid index (%d)", nextempty);
  TimerList.array[nextempty].use = TimerList.nextempty;
  --TimerList.occupied;
  result = -1;
  TimerList.nextempty = nextempty;
  return result;
}

//----- (00496B60) --------------------------------------------------------

void SWidget::SetVisible(bool visible)

{
  SWidget *Parent; // edx
  this->Visible = visible;
  if ( !visible )
  {
    if ( SWidget::CaptureTarget != this || (SWidget::CaptureTarget = 0, !this->Visible) )
    {
      Parent = this->Parent;
      if ( Parent->Focus == this )
        Parent->Focus = this->FocusSibling;
    }
  }
}

//----- (00496C10) --------------------------------------------------------

void SWidget::TranslateEventFromWindow(int *x, int *y)

{
  if ( !this->Parent->IsWindow() )
    this->Parent->TranslateEventFromWindow(x, y);
  this->ParentToChild(x, y);
}

// ============================================================
// Missing virtual method stubs for SWidget base class
// ============================================================

void SWidget::OnMouseDown(int button, int x, int y, int shift) {} // Base class virtual — intentionally empty
void SWidget::OnMouseUp(int button, int x, int y, int shift) {} // Base class virtual — intentionally empty
void SWidget::OnMouseMove(int x, int y, int shift) {} // Base class virtual — intentionally empty
void SWidget::OnMouseOut() {} // Base class virtual — intentionally empty
void SWidget::OnMouseOver() {} // Base class virtual — intentionally empty
void SWidget::OnSize(int w, int h) {} // Base class virtual — intentionally empty
void SWidget::OnTimer(int id, unsigned int elapsed) {} // Base class virtual — intentionally empty
void SWidget::OnUpdate() {} // Base class virtual — intentionally empty
void SWidget::OnUserEvent(unsigned int event, unsigned int wparam, int lparam) {} // Base class virtual — intentionally empty
void SWidget::Update() {} // Base class virtual — intentionally empty
void SWidget::Create(int a2) {} // Base class virtual — intentionally empty

// SResultsMenu::OnAction removed — belongs in a separate menu file

