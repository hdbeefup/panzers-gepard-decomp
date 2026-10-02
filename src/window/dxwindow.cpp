// window/dxwindow.cpp
// DirectX window
// Decompiled from: gameSplit/swindow.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>

#include "dxwindow.h"
#include "messagebox.h"
#include "logger.h"

// Classes: SDXWindow
// Function count: 20

// Forward declarations for global factory functions
int CreateGepard(HWND hwnd, bool fullscreen, bool vsync, int msaa, int width, int height, int a7, SIGepard **ppGepard);
SIConcert *CreateConcert(HWND hwnd);

//----- (00486DC0) --------------------------------------------------------

SDXWindow::SDXWindow()

{
  this->MonitorRects.array = 0;
  this->MonitorRects.size = 0;
  this->MonitorRects.maxsize = 0;
  Gepard = 0;
  this->DisplayMode = Windowed;
  this->VSync = 0;
  this->AntialiasingMode = Off;
  this->WindowedX = 40;
  this->WindowedY = 40;
  this->WindowedWidth = 800;
  this->WindowedHeight = 600;
  this->MBox = 0;
}

//----- (00486F10) --------------------------------------------------------

int CALLBACK SDXWindow::AddMonitorsCallBack(HMONITOR hMonitor, HDC hdcMonitor, RECT *lprcMonitor, SDArray<RECT> *rects)
{
  MONITORINFO mi;
  memset(&mi, 0, sizeof(mi));
  mi.cbSize = sizeof(mi);
  GetMonitorInfoA(hMonitor, &mi);
  int idx = rects->Add();
  rects->array[idx] = mi.rcMonitor;
  return 1;
}

//----- (00486FD0) --------------------------------------------------------

int SDXWindow::Create(HICON icon, HCURSOR cursor, const wchar_t *title)

{
  int FullScreenWidth;
  int v9;
  bool vsyncVal;
  int msaaLevel;
  int gepardResult;
  bool isfullscreen;
  int height;
  if ( this->DisplayMode != ExclusiveFullscreen )
  {
    if ( SWindow::Create(icon, cursor, title, 0xCF0000u, 0, 0) >= 0 )
    {
      Logger.g->Attach(this->hWnd);
      ValidateRect(this->hWnd, 0);
      height = this->Height;
      FullScreenWidth = this->Width;
      isfullscreen = 0;
      goto LABEL_7;
    }
    return -1;
  }
  if ( SWindow::Create(icon, cursor, title, 0, 0, 0) < 0 )
    return -1;
  Logger.g->Attach(this->hWnd);
  height = this->FullScreenHeight;
  FullScreenWidth = this->FullScreenWidth;
  isfullscreen = 1;
LABEL_7:
  v9 = FullScreenWidth;
  msaaLevel = this->GetMSAALevel();
  vsyncVal = this->VSync;
  gepardResult = CreateGepard(this->hWnd, isfullscreen, vsyncVal, msaaLevel, v9, height, 1, &::Gepard);
  Concert = CreateConcert(this->hWnd);
  return gepardResult;
}

//----- (004870D0) --------------------------------------------------------

int SDXWindow::GetFrame()

{
  return 0;
}

//----- (004870E0) --------------------------------------------------------

int SDXWindow::GetMSAALevel()

{
  int result;
  switch ( this->AntialiasingMode )
  {
    case MSAA2x:
      result = 2;
      break;
    case MSAA4x:
      result = 4;
      break;
    case MSAA8x:
      result = 8;
      break;
    default:
      result = 0;
      break;
  }
  return result;
}

//----- (00487120) --------------------------------------------------------

void SDXWindow::InitDesktopSize(int *monitorIdx)

{
  SDArray<RECT> *p_MonitorRects; // esi
  int *v4; // ecx
  int v5;
  int v6;
  RECT *v7; // ecx
  int v8;
  int v9;
  int left;
  int *top; // edi
  int v14;
  int v15;
  p_MonitorRects = &this->MonitorRects;
  EnumDisplayMonitors(0, 0, (MONITORENUMPROC)SDXWindow::AddMonitorsCallBack, (LPARAM)p_MonitorRects);
  v4 = &v15;
  v15 = p_MonitorRects->size - 1;
  if ( *monitorIdx < v15 )
    v4 = monitorIdx;
  v5 = *v4;
  v6 = 0;
  if ( v5 > 0 )
    v6 = v5;
  *monitorIdx = v6;
  v7 = &this->MonitorRects.array[v6];
  v8 = v7->right - v7->left;
  this->DesktopWidth = v8;
  v9 = v7->bottom - v7->top;
  this->DesktopHeight = v9;
  left = v7->left;
  this->DesktopX = v7->left;
  top = (int *)v7->top;
  this->DesktopY = (int)top;
  this->WindowedWidth = (int)(float)((float)((float)v8 * 4.0f) / 5.0f);
  v14 = (int)(float)((float)((float)v9 * 4.0f) / 5.0f);
  this->WindowedHeight = v14;
  this->WindowedX = (int)(float)((float)((float)(v8 - (int)(float)((float)((float)v8 * 4.0f) / 5.0f)) * 0.5f) + (float)left);
  this->WindowedY = (int)(float)((float)((float)(v9 - v14) * 0.5f) + (float)(int)top);
}

//----- (00487240) --------------------------------------------------------

bool SDXWindow::IsFullScreen()

{
  return this->DisplayMode == BorderlessWindowed;
}

//----- (00487250) --------------------------------------------------------

bool SDXWindow::IsThereMessageBox()

{
  return this->MBox != 0;
}

//----- (00487270) --------------------------------------------------------

int SDXWindow::MessageBoxA(SWidget *windowOrScaler, const char *text, int type)

{
  SMessageBox *v9;
  int v11;
  SMessageBox *MBox;
  int v13;
  SMessageBox v14;
  this->RestrictMouseCursor(0);
  if ( type == 1971 )
  {
    windowOrScaler->InsertChild(&v14);
    v14.Create(text, 1971);
    Gepard->RenderScene(0);
    return 1;
  }
  if ( this->MBox )
  {
    if ( !type )
      return 1;
    if ( type == 4 )
      return 6;
    else
      return 4 * (type == 1972) + 1;
  }
  else
  {
    v9 = new SMessageBox();
    this->MBox = v9;
    windowOrScaler->InsertChild(v9);
    this->MBox->Create(text, type);
    v11 = this->MBox->DoModal();
    MBox = this->MBox;
    v13 = v11;
    if ( MBox )
    {
      delete MBox;
      this->MBox = 0;
    }
    return v13;
  }
}

//----- (00487450) --------------------------------------------------------

void SDXWindow::OnClose()

{
  if ( this->DisplayMode == ExclusiveFullscreen )
    Gepard->SetMode(0, 0, 0, this->WindowedWidth, this->WindowedHeight);
  SWindow::OnClose();
}

//----- (00487490) --------------------------------------------------------

void SDXWindow::OnDestroy()

{
  SMessageBox *MBox;
  MBox = this->MBox;
  if ( MBox )
  {
    delete MBox;
    this->MBox = 0;
  }
  if ( Concert )
  {
    Concert->Release();
    Concert = 0;
  }
  if ( Gepard )
  {
    Gepard->Release();
    Gepard = 0;
  }
  SWindow::OnDestroy();
}

//----- (004874F0) --------------------------------------------------------

bool SDXWindow::OnIdle()

{
  int CurrentCursor;
  int cursorX;
  int cursorY;
  if ( !Gepard )
    return 0;
  if ( !SWidget::LastMouseTarget )
    this->UpdateMouse();
  cursorY = this->CursorY;
  cursorX = this->CursorX;
  CurrentCursor = SWidget::GetCurrentCursor();
  Board->SetCursor(CurrentCursor, cursorX, cursorY);
  Gepard->RenderScene(0);
  return 1;
}

//----- (00487580) --------------------------------------------------------

void SDXWindow::OnPaint(HDC hdc)

{
  if ( !Gepard )
    SWindow::OnPaint(hdc);
}

//----- (004875A0) --------------------------------------------------------

int SDXWindow::OnSetCursor()

{
  if ( Board )
    Board->ApplyHardwareCursor();
  return 1;
}

//----- (004875C0) --------------------------------------------------------

void SDXWindow::OnSize(int width, int height)

{
  Logger.g->Log(0, "SDXWindow::OnSize(%d, %d)", width, height);
  if ( width && height && Gepard && this->DisplayMode != ExclusiveFullscreen )
    Gepard->Resize(width, height);
}

//----- (00487620) --------------------------------------------------------

void SDXWindow::SetCursorPos(int x, int y)

{
  bool v3;
  HWND hWnd;
  POINT p;
  v3 = this->DisplayMode == ExclusiveFullscreen;
  this->CursorX = x;
  this->CursorY = y;
  if ( v3 )
  {
    ::SetCursorPos(x, y);
  }
  else
  {
    p.x = x;
    hWnd = this->hWnd;
    p.y = y;
    ClientToScreen(hWnd, &p);
    ::SetCursorPos(p.x, p.y);
  }
}

//----- (00487680) --------------------------------------------------------

void SDXWindow::SetDisplayMode(SDisplayMode displayMode, bool vsync, SAntialiasingMode antialiasingMode, int monitor, int fullscreenWidth, int fullscreenHeight)

{
  SDisplayMode v8;
  int v9;
  int msaaLevel;
  LONG v20;
  if ( displayMode == this->DisplayMode
    && vsync == this->VSync
    && antialiasingMode == this->AntialiasingMode
    && monitor == this->Monitor
    && (fullscreenWidth <= 0
     || fullscreenHeight <= 0
     || fullscreenWidth == this->FullScreenWidth && fullscreenHeight == this->FullScreenHeight) )
  {
    return;
  }
  this->DisplayMode = displayMode;
  this->AntialiasingMode = antialiasingMode;
  this->VSync = vsync;
  this->Monitor = monitor;
  if ( fullscreenWidth > 0 && fullscreenHeight > 0 )
  {
    this->FullScreenWidth = fullscreenWidth;
    this->FullScreenHeight = fullscreenHeight;
  }
  this->InitDesktopSize(&this->Monitor);
  v8 = this->DisplayMode;
  if ( v8 == Windowed )
  {
    this->SetWindowStyle(0xCF0000u);
    ShowWindow(this->hWnd, 5);
    if ( Gepard )
    {
      msaaLevel = this->GetMSAALevel();
      Gepard->SetMode(0, this->VSync, msaaLevel, this->WindowedWidth, this->WindowedHeight);
    }
    this->SetPosition(this->WindowedX, this->WindowedY, this->WindowedWidth, this->WindowedHeight);
    v20 = 0;
    goto LABEL_24;
  }
  v9 = v8 - 1;
  if ( !v9 )
  {
    this->SetWindowStyle(0);
    ShowWindow(this->hWnd, 5);
    if ( Gepard )
    {
      msaaLevel = this->GetMSAALevel();
      Gepard->SetMode(0, this->VSync, msaaLevel, this->DesktopWidth, this->DesktopHeight);
    }
    this->SetPosition(this->DesktopX, this->DesktopY, this->DesktopWidth, this->DesktopHeight);
    v20 = 8;
LABEL_24:
    SetWindowLongA(this->hWnd, -20, v20);
    return;
  }
  if ( v9 == 1 )
  {
    this->SetWindowStyle(0);
    ShowWindow(this->hWnd, 5);
    if ( Gepard )
    {
      msaaLevel = this->GetMSAALevel();
      Gepard->SetMode(1, this->VSync, msaaLevel, this->FullScreenWidth, this->FullScreenHeight);
    }
    SWidget::Resize(this->FullScreenWidth, this->FullScreenHeight);
    this->OnSize(this->FullScreenWidth, this->FullScreenHeight);
    ValidateRect(this->hWnd, 0);
    this->FlushMessages();
  }
}

//----- (004878D0) --------------------------------------------------------

void SDXWindow::SetPosition(int x, int y, int width, int height)

{
  SWindow::SetPosition(x, y, width, height);
  if ( this->DisplayMode == Windowed )
  {
    this->WindowedX = this->X;
    this->WindowedY = this->Y;
    this->WindowedWidth = this->Width;
    this->WindowedHeight = this->Height;
  }
  if ( Gepard )
    Gepard->Resize(this->Width, this->Height);
  ValidateRect(this->hWnd, 0);
}

//----- (00487940) --------------------------------------------------------

LRESULT SDXWindow::WindowProc(unsigned int message, unsigned int wParam, int lParam)

{
  if ( message == 512 )
  {
    this->CursorX = (short)lParam;
    this->CursorY = (short)HIWORD(lParam);
    return SWindow::WindowProc(message, wParam, lParam);
  }
  if ( message != 5 || this->DisplayMode != ExclusiveFullscreen )
    return SWindow::WindowProc(message, wParam, lParam);
  return DefWindowProcW(this->hWnd, 5u, wParam, lParam);
}

//----- (004F6500) --------------------------------------------------------

SDXWindow::~SDXWindow()

{
  if ( this->MonitorRects.array )
  {
    free((void *)this->MonitorRects.array);
    this->MonitorRects.array = 0;
  }
  // base destructor called automatically
}

