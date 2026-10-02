// window/window.cpp
// Window class
// Decompiled from: gameSplit/swindow.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <eh.h>

#include "window.h"
#include "stream.h"
#include "logger.h"

// Globals
extern HINSTANCE hInstance;
extern SWindow *TheWindow;
extern SFileSystem FileSystem;

/* CONFORMANCE: g_SuppressMouseMessages removed — not in original binary.
   Was: int g_SuppressMouseMessages = 0; + mouse message suppression in StaticWindowProc.
   Re-enable if re-entrant dispatch crashes return. */
int g_SuppressMouseMessages = 0; // kept for linkage, always 0

// Static WNDPROC thunk — dispatches to TheWindow->WindowProc virtual
static LRESULT CALLBACK StaticWindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  if (!TheWindow)
    return DefWindowProcW(hWnd, message, wParam, lParam);
  TheWindow->hWnd = hWnd;
  return TheWindow->WindowProc(message, (unsigned int)wParam, (int)lParam);
}

// Classes: SWindow
// Function count: 28

//----- (00487260) --------------------------------------------------------

bool SWindow::IsWindow()

{
  return 1;
}

//----- (00496C50) --------------------------------------------------------

SWindow::SWindow()

{
  this->UserEvents.array = 0;
  this->UserEvents.size = 0;
  this->UserEvents.maxsize = 0;
  this->UserEvents.nextempty = -1;
  this->UserEvents.occupied = 0;
  this->hWnd = 0;
  this->Index = -1;
  this->ModalResult = 0;
  this->ModalTarget = 0;
  this->EventDirection = 0;
  this->MouseCursorRestricted = 0;
  this->shouldClose = 0;
  TheWindow = this;
}

//----- (00496CF0) --------------------------------------------------------

SWindow::~SWindow()

{
  if ( this->UserEvents.array )
    free(this->UserEvents.array);
  // base destructor called automatically
}

//----- (00496D30) --------------------------------------------------------

SWindow::operator HWND()

{
  return this->hWnd;
}

//----- (00496D80) --------------------------------------------------------

void SWindow::CloseEventStream()

{
  if ( this->EventDirection == 1 )
  {
    this->EventStream->WriteChunkEnd();
    this->EventStream->WriteChunkEnd();
    this->EventStream->Release();
  }
}

//----- (00496DB0) --------------------------------------------------------

int SWindow::Create(HICON icon, HCURSOR cursor, const wchar_t *title, unsigned int style, SWindow *parent, bool maximized)

{
  wchar_t *ClassName; // ebx
  int X;
  int Y;
  int Height;
  HWND hWndParent; // eax
  HWND Window; // ecx
  DWORD v15;
  WNDCLASSEXW wcex;
  LPCWSTR lpWindowName;
  HCURSOR CursorA;
  HICON IconA;
  RECT r;
  lpWindowName = title;
  ClassName = this->ClassName;
  IconA = 0;
  CursorA = 0;
  swprintf(this->ClassName, 0x20u, L"SR:%08x", (unsigned int)(uintptr_t)this);
  if ( icon )
    IconA = LoadIconA((ULONG_PTR)icon - 32512 > 0xFF ? hInstance : 0, (LPCSTR)icon);
  if ( cursor )
    CursorA = LoadCursorA((ULONG_PTR)cursor - 32512 > 0xFF ? hInstance : 0, (LPCSTR)cursor);
  this->Style = style;
  wcex.hInstance = hInstance;
  wcex.hIcon = IconA;
  wcex.hIconSm = IconA;
  wcex.cbSize = sizeof(wcex);
  wcex.style = 3;
  wcex.lpfnWndProc = StaticWindowProc;
  wcex.cbClsExtra = 0;
  wcex.cbWndExtra = 0;
  wcex.hCursor = CursorA;
  wcex.hbrBackground = 0;
  wcex.lpszMenuName = 0;
  wcex.lpszClassName = ClassName;
  if (!RegisterClassExW(&wcex))
    Logger.g->Log(0, "SWindow::Create: RegisterClassExW failed (err=%lu)", GetLastError());
  X = this->X;
  Y = this->Y;
  r.right = X + this->Width;
  Height = this->Height;
  r.left = X;
  v15 = this->Style;
  r.bottom = Y + Height;
  r.top = Y;
  AdjustWindowRect(&r, v15, 0);
  if ( parent )
    hWndParent = parent->hWnd;
  else
    hWndParent = 0;
  Window = CreateWindowExW(
             0,
             ClassName,
             lpWindowName,
             this->Style,
             r.left,
             r.top,
             r.right - r.left,
             r.bottom - r.top,
             hWndParent,
             0,
             hInstance,
             0);
  this->Parent = parent;
  this->hWnd = Window;
  if ( !Window )
    return -1;
  ShowWindow(Window, 2 * !maximized + 3);
  UpdateWindow(this->hWnd);
  return 0;
}

//----- (00496F60) --------------------------------------------------------

char SWindow::EventFrame(int a2, int a3)

{
  int EventDirection;
  bool IsEnd; // al
  SStream *EventStream; // ecx
  int Int;
  int v9;
  int v10;
  SStream *v12; // ecx
  int v13;
  int v14;
  EventDirection = this->EventDirection;
  if ( EventDirection == 1 )
  {
    this->EventStream->WriteChunkEnd();
    this->EventStream->WriteChunkStart(1296126534);
    return 0;
  }
  if ( EventDirection == 2 )
  {
    IsEnd = this->EventStream->ReadChunkIsEnd();
    EventStream = this->EventStream;
    if ( IsEnd )
    {
      EventStream->ReadChunkValidate((int)this);
      this->EventStream->Release();
      return 1;
    }
    if ( EventStream->ReadChunkHeader() != 1296126534 )
LABEL_13:
      Logger.g->Panic("SWindow::EventFrame: Invalid event frame", v13, v14);
    if ( !this->EventStream->ReadChunkIsEnd() )
    {
      v14 = a2;
      v13 = a3;
      while ( this->EventStream->ReadChunkHeader() == 1668248176 )
      {
        Int = this->EventStream->ReadInt();
        v9 = this->EventStream->ReadInt();
        v10 = this->EventStream->ReadInt();
        this->EventDirection = 0;
        this->WindowProc(Int, v9, v10);
        v12 = this->EventStream;
        this->EventDirection = 2;
        v12->ReadChunkValidate((int)this);
        if ( this->EventStream->ReadChunkIsEnd() )
          goto LABEL_11;
      }
      goto LABEL_13;
    }
LABEL_11:
    this->EventStream->ReadChunkValidate((int)this);
  }
  return 0;
}

//----- (00497080) --------------------------------------------------------

void SWindow::FlushMessages()

{
  MSG msg;
  while ( this->hWnd )
  {
    if ( !PeekMessageA(&msg, 0, 0, 0, 0) )
      break;
    if ( msg.message == 18 )
      break;
    PeekMessageA(&msg, 0, 0, 0, 1u);
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }
}

//----- (004970F0) --------------------------------------------------------

SWidget *SWindow::GetEventTarget(int x, int y, int *target_x, int *target_y)

{
  int v5;
  int v6;
  SWidget *ModalTarget; // ecx
  v5 = x;
  v6 = y;
  this->LastMouseX = x;
  this->LastMouseY = v6;
  if ( SWidget::CaptureTarget )
  {
    SWidget::CaptureTarget->TranslateEventFromWindow(&x, &y);
    SWidget::CaptureTarget->SetMouseTarget();
    *target_x = x;
    *target_y = y;
    return SWidget::CaptureTarget;
  }
  else
  {
    ModalTarget = this->ModalTarget;
    if ( ModalTarget )
    {
      ModalTarget->TranslateEventFromWindow(&x, &y);
      return this->ModalTarget->SWidget::GetEventTarget(x, y, target_x, target_y);
    }
    else
    {
      return SWidget::GetEventTarget(v5, v6, target_x, target_y);
    }
  }
}

//----- (00497190) --------------------------------------------------------

bool SWindow::InitDPI()

{
  HMODULE ModuleHandleW;
  FARPROC GetDpiForWindow;
  FARPROC v4;
  bool result; // al
  this->Dpi = 96;
  ModuleHandleW = GetModuleHandleW(L"user32.dll");
  GetDpiForWindow = GetProcAddress(ModuleHandleW, "GetDpiForWindow");
  v4 = GetDpiForWindow;
  if ( GetDpiForWindow )
    this->Dpi = ((int (__stdcall *)(HWND))GetDpiForWindow)(this->hWnd);
  result = v4 != 0;
  this->Scaling = (float)this->Dpi / 96.0f;
  return result;
}

//----- (004971F0) --------------------------------------------------------

bool SWindow::IsMouseCursorRestricted()

{
  return this->MouseCursorRestricted;
}

//----- (00497200) --------------------------------------------------------

void SWindow::OnClose()

{
  if ( this->ModalTarget )
    this->shouldClose = 1;
  else
    DestroyWindow(this->hWnd);
}

//----- (00497220) --------------------------------------------------------

void SWindow::OnDPIChange(RECT r)

{
  HMODULE ModuleHandleW;
  FARPROC GetDpiForWindow;
  FARPROC v5;
  this->Dpi = 96;
  ModuleHandleW = GetModuleHandleW(L"user32.dll");
  GetDpiForWindow = GetProcAddress(ModuleHandleW, "GetDpiForWindow");
  v5 = GetDpiForWindow;
  if ( GetDpiForWindow )
    this->Dpi = ((int (__stdcall *)(HWND))GetDpiForWindow)(this->hWnd);
  this->Scaling = (float)this->Dpi / 96.0f;
  if ( v5 )
    SetWindowPos(this->hWnd, 0, r.left, r.top, r.right - r.left, r.bottom - r.top, 0);
}

//----- (004972B0) --------------------------------------------------------

void SWindow::OnDestroy()

{
  UnregisterClassW(this->ClassName, hInstance);
  this->hWnd = 0;
  this->Index = -1;
  TheWindow = 0;
}

//----- (004972F0) --------------------------------------------------------

bool SWindow::OnIdle()

{
  return 0;
}

//----- (00497300) --------------------------------------------------------

void SWindow::OnPaint(HDC hdc)

{
  HBRUSH SolidBrush;
  RECT rt;
  GetClientRect(this->hWnd, &rt);
  SolidBrush = CreateSolidBrush(0);
  FillRect(hdc, &rt, SolidBrush);
  DeleteObject(SolidBrush);
}

//----- (00497360) --------------------------------------------------------

int SWindow::OnSetCursor()

{
  return 0;
}

//----- (00497370) --------------------------------------------------------

void SWindow::PlaybackEvents(const char *filename)

{
  SStream *v3; // eax
  v3 = FileSystem.OpenRead(filename, "SWindow::RecordEvents");
  this->EventStream = v3;
  this->EventDirection = 2;
  v3->ReadSignature();
  if ( this->EventStream->ReadChunkHeader() != 1414420037 )
    Logger.g->Panic("SWindow::PlaybackEvents: %s: not an event file", filename);
}

//----- (004973D0) --------------------------------------------------------

static void DebugMark(const char* msg)
{
  static char debugPath[MAX_PATH] = {0};
  if (!debugPath[0]) {
    GetTempPathA(MAX_PATH, debugPath);
    strcat(debugPath, "swine_debug.txt");
  }
  FILE* f = fopen(debugPath, "a");
  if (f) { fprintf(f, "%s\n", msg); fclose(f); }
}

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* ep)
{
  char crashPath[MAX_PATH];
  GetTempPathA(MAX_PATH, crashPath);
  strcat(crashPath, "swine_crash.txt");
  FILE* f = fopen(crashPath, "a");
  if (f) {
    fprintf(f, "EXCEPTION 0x%08X at 0x%p\n",
      (unsigned)ep->ExceptionRecord->ExceptionCode,
      ep->ExceptionRecord->ExceptionAddress);
#if defined(_M_X64)
    fprintf(f, "RAX=%016llX RBX=%016llX RCX=%016llX RDX=%016llX\n",
      (unsigned long long)ep->ContextRecord->Rax, (unsigned long long)ep->ContextRecord->Rbx,
      (unsigned long long)ep->ContextRecord->Rcx, (unsigned long long)ep->ContextRecord->Rdx);
    fprintf(f, "RSI=%016llX RDI=%016llX RSP=%016llX RBP=%016llX RIP=%016llX\n",
      (unsigned long long)ep->ContextRecord->Rsi, (unsigned long long)ep->ContextRecord->Rdi,
      (unsigned long long)ep->ContextRecord->Rsp, (unsigned long long)ep->ContextRecord->Rbp,
      (unsigned long long)ep->ContextRecord->Rip);
#elif defined(_M_IX86)
    fprintf(f, "EAX=%08X EBX=%08X ECX=%08X EDX=%08X\n",
      (unsigned)ep->ContextRecord->Eax, (unsigned)ep->ContextRecord->Ebx,
      (unsigned)ep->ContextRecord->Ecx, (unsigned)ep->ContextRecord->Edx);
    fprintf(f, "ESI=%08X EDI=%08X ESP=%08X EBP=%08X EIP=%08X\n",
      (unsigned)ep->ContextRecord->Esi, (unsigned)ep->ContextRecord->Edi,
      (unsigned)ep->ContextRecord->Esp, (unsigned)ep->ContextRecord->Ebp,
      (unsigned)ep->ContextRecord->Eip);
#else
    fprintf(f, "(register dump: unsupported arch)\n");
#endif
    fclose(f);
  }
  return EXCEPTION_CONTINUE_SEARCH;
}

static void __cdecl PureCallHandler()
{
  DebugMark("PURECALL HANDLER - pure virtual function call!");
  abort();
}

static void __cdecl InvalidParamHandler(const wchar_t* expr, const wchar_t* func,
  const wchar_t* file, unsigned int line, uintptr_t reserved)
{
  char buf[256];
  sprintf(buf, "INVALID PARAMETER HANDLER func=%ls file=%ls line=%u",
    func ? func : L"(null)", file ? file : L"(null)", line);
  DebugMark(buf);
}

static void __cdecl TerminateHandler()
{
  DebugMark("TERMINATE HANDLER - std::terminate called!");
  abort();
}

static void __cdecl AtExitHandler()
{
  DebugMark("ATEXIT - process exiting normally through CRT");
}

static struct CrashHandlerInstaller {
  CrashHandlerInstaller() {
    SetUnhandledExceptionFilter(CrashHandler);
    _set_purecall_handler(PureCallHandler);
    _set_invalid_parameter_handler(InvalidParamHandler);
    set_terminate(TerminateHandler);
    atexit(AtExitHandler);
  }
} g_crashInstaller;

int SWindow::ProcessMessages()

{
  MSG msg;
  if ( !this->hWnd )
    return this->ModalResult;
  do
  {
    if ( !this->OnIdle() )
    {
      if ( !GetMessageA(&msg, 0, 0, 0) )
        return this->ModalResult;
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
    while ( this->hWnd )
    {
      if ( !PeekMessageA(&msg, 0, 0, 0, 1u) )
        break;
      if ( msg.message == 18 )
        return this->ModalResult;
      TranslateMessage(&msg);
      DispatchMessageA(&msg);
    }
    if ( this->shouldClose )
      this->OnClose();
  }
  while ( this->hWnd );
  return this->ModalResult;
}

//----- (004974A0) --------------------------------------------------------

void SWindow::RecordEvents(char *filename)

{
  SStream *v3; // eax
  v3 = FileSystem.OpenWrite(filename, "SWindow::RecordEvents");
  this->EventStream = v3;
  this->EventDirection = 1;
  v3->WriteSignature();
  this->EventStream->WriteChunkStart(1414420037);
  this->EventStream->WriteChunkStart(1296126534);
}

//----- (00497500) --------------------------------------------------------

void SWindow::RegisterUserEventHandler(unsigned int event, SWidget *handler)

{
  int v4;
  int size;
  SHeap<UserEventProp>::__Tstruct *v7; // ecx
  int nextempty;
  SHeap<UserEventProp>::__Tstruct *array; // ecx
  unsigned int v10;
  SHeap<UserEventProp>::__Tstruct *v11; // edx
  int v12;
  int maxsize;
  int v14;
  SHeap<UserEventProp>::__Tstruct *v15; // eax
  int v16;
  int v17;
  int handlera;
  v4 = -1;
  while ( 1 )
  {
    size = this->UserEvents.size;
    if ( ++v4 >= size )
      break;
    v7 = &this->UserEvents.array[v4];
    while ( v7->use != 0x7FFFFFFF )
    {
      ++v4;
      ++v7;
      if ( v4 >= size )
        goto LABEL_6;
    }
    if ( v4 < 0 )
      break;
    v11 = &this->UserEvents.array[v4];
    if ( event == v11->data.Event )
    {
      if ( handler )
      {
        v11->data.Handler = handler;
      }
      else
      {
        if ( v4 >= size || v11->use != 0x7FFFFFFF )
          Logger.g->Panic("SHeap::Remove: invalid index (%d)", v4);
        v11->use = this->UserEvents.nextempty;
        --this->UserEvents.occupied;
        this->UserEvents.nextempty = v4;
      }
    }
  }
LABEL_6:
  if ( handler )
  {
    ++this->UserEvents.occupied;
    nextempty = this->UserEvents.nextempty;
    if ( nextempty < 0 )
    {
      v12 = this->UserEvents.size;
      maxsize = this->UserEvents.maxsize;
      if ( v12 == maxsize )
      {
        if ( maxsize >= 16 )
          v14 = 6 * maxsize / 5;
        else
          v14 = 16;
        handlera = v14;
        // x64: literal `12` is x86 sizeof(SHeap<UserEventProp>::__Tstruct). Same family as group.cpp:1884.
        v15 = (SHeap<UserEventProp>::__Tstruct *)realloc(this->UserEvents.array, sizeof(SHeap<UserEventProp>::__Tstruct) * v14);
        v16 = this->UserEvents.maxsize;
        this->UserEvents.array = v15;
        memset(&v15[v16], 0, sizeof(SHeap<UserEventProp>::__Tstruct) * (handlera - v16));
        v12 = this->UserEvents.size;
        this->UserEvents.maxsize = handlera;
      }
      this->UserEvents.array[v12].use = 0x7FFFFFFF;
      nextempty = this->UserEvents.size;
      this->UserEvents.size = nextempty + 1;
    }
    else
    {
      array = this->UserEvents.array;
      v10 = nextempty;
      this->UserEvents.nextempty = array[nextempty].use;
      array[v10].use = 0x7FFFFFFF;
      memset(&array[v10].data, 0, sizeof(array[v10].data));
    }
    v17 = nextempty;
    this->UserEvents.array[v17].data.Event = event;
    this->UserEvents.array[v17].data.Handler = handler;
  }
}

//----- (004976B0) --------------------------------------------------------

void SWindow::RestrictMouseCursor(bool enable)

{
  RECT Rect;
  if ( this->MouseCursorRestricted != enable )
  {
    this->MouseCursorRestricted = enable;
    if ( enable )
    {
      GetClientRect(this->hWnd, &Rect);
      ClientToScreen(this->hWnd, (LPPOINT)&Rect);
      ClientToScreen(this->hWnd, (LPPOINT)&Rect.right);
      ClipCursor(&Rect);
    }
    else
    {
      ClipCursor(0);
    }
  }
}

//----- (00497730) --------------------------------------------------------

bool SWindow::RunModal(SWidget *modal_widget)

{
  MSG msg;
  this->ModalTarget = modal_widget;
  if ( !this->OnIdle() )
  {
    if ( !GetMessageA(&msg, 0, 0, 0) )
      return this->shouldClose;
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }
  while ( this->hWnd )
  {
    if ( !PeekMessageA(&msg, 0, 0, 0, 1u) )
      break;
    if ( msg.message == 18 )
      return this->shouldClose;
    TranslateMessage(&msg);
    DispatchMessageA(&msg);
  }
  this->ModalTarget = 0;
  return this->shouldClose;
}

//----- (004977E0) --------------------------------------------------------

void SWindow::SetPosition(int x, int y, int width, int height)

{
  int v6;
  int v7;
  int v8;
  DWORD Style;
  RECT r;
  SWidget::SetPosition(x, y, width, height);
  if ( this->hWnd )
  {
    v6 = this->X;
    v7 = this->Y;
    r.right = v6 + this->Width;
    v8 = this->Height;
    r.left = v6;
    Style = this->Style;
    r.bottom = v7 + v8;
    r.top = v7;
    AdjustWindowRect(&r, Style, 0);
    MoveWindow(this->hWnd, r.left, r.top, r.right - r.left, r.bottom - r.top, 1);
  }
}

//----- (00497870) --------------------------------------------------------

void SWindow::SetWindowStyle(unsigned int style)

{
  HWND hWnd;
  hWnd = this->hWnd;
  this->Style = style;
  SetWindowLongA(hWnd, -16, style);
}

//----- (00497890) --------------------------------------------------------

void SWindow::UpdateMouse()

{
  int target_x;
  int target_y;
  this->GetEventTarget(this->LastMouseX, this->LastMouseY, &target_x, &target_y);
}

//----- (004978C0) --------------------------------------------------------

void SWindow::UpdateMouseCursorRestriction()

{
  RECT windowRect;
  if ( this->MouseCursorRestricted )
  {
    GetClientRect(this->hWnd, &windowRect);
    ClientToScreen(this->hWnd, (LPPOINT)&windowRect);
    ClientToScreen(this->hWnd, (LPPOINT)&windowRect.right);
    ClipCursor(&windowRect);
  }
  else
  {
    ClipCursor(0);
  }
}

//----- (00497970) --------------------------------------------------------

LRESULT SWindow::WindowProc(unsigned int message, unsigned int wParam, int lParam)

{
  int v5;
  int EventDirection;
  int result;
  HDC v9;
  SWidget *i; // esi
  SWidget *j; // esi
  SWidget *k; // esi
  SWidget *v13; // eax
  SWidget *v14; // eax
  SWidget *v15; // eax
  SWidget *v16; // eax
  SWidget *v17; // eax
  SWidget *v18; // eax
  SWidget *v19; // eax
  SWidget *v20; // esi
  int v21;
  SHeap<UserEventProp>::__Tstruct *v22; // ecx
  SHeap<UserEventProp>::__Tstruct *array; // edx
  WPARAM v24;
  int v25;
  int v26;
  int v27;
  int v28;
  int v29;
  int v30;
  int v31;
  LPARAM v32;
  int target_x;
  int target_y;
  PAINTSTRUCT ps;
  v5 = lParam;
  if ( message == 513
    || message == 519
    || message == 516
    || message == 514
    || message == 520
    || message == 517
    || message == 512
    || message == 522
    || message == 256
    || message == 257 )
  {
    EventDirection = this->EventDirection;
    if ( EventDirection == 1 )
    {
      this->EventStream->WriteChunkStart(1668248176);
      this->EventStream->WriteInt(message);
      this->EventStream->WriteInt(wParam);
      this->EventStream->WriteInt(lParam);
      this->EventStream->WriteChunkEnd();
      EventDirection = this->EventDirection;
      v5 = lParam;
    }
    if ( EventDirection == 2 )
      return 0;
  }
  if ( message <= 0x100 )
  {
    if ( message != 256 )
    {
      switch ( message )
      {
        case 1u:
          this->InitDPI();
          return 0;
        case 2u:
          if ( this->MouseCursorRestricted )
          {
            this->MouseCursorRestricted = 0;
            ClipCursor(0);
          }
          this->OnDestroy();
          return 0;
        case 3u:
          this->X = (short)v5;
          this->Y = (short)HIWORD(lParam);
          this->OnMove((short)v5, (short)HIWORD(lParam));
          goto $LN45_6;
        case 5u:
          this->Resize((short)v5, (short)HIWORD(v5));
          this->OnSize(this->Width, this->Height);
          this->UpdateMouseCursorRestriction();
          return 0;
        case 6u:
$LN45_6:
          this->UpdateMouseCursorRestriction();
          return 0;
        case 0xFu:
          v9 = BeginPaint(this->hWnd, &ps);
          this->OnPaint(v9);
          EndPaint(this->hWnd, &ps);
          return 0;
        case 0x10u:
          this->OnClose();
          return 0;
        case 0x1Cu:
          this->OnActivateApp(wParam != 0);
          goto LABEL_34;
        case 0x20u:
          if ( (WORD)v5 != 1 )
            goto $LN42_10;
          if ( this->OnSetCursor() )
            return 1;
          v5 = lParam;
$LN42_10:
          v32 = v5;
          break;
        case 0x24u:
          *(DWORD *)(v5 + 24) = 800;
          result = 0;
          *(DWORD *)(v5 + 28) = 600;
          return result;
        case 0x46u:
          return 1;
        case 0x81u:
          goto $LN42_10;
        default:
          goto LABEL_65;
      }
LABEL_35:
      v24 = wParam;
      return DefWindowProcW(this->hWnd, message, v24, v32);
    }
$LN23_12:
    if ( wParam == 115 && (v5 & 0x20000000) != 0 )
    {
      v32 = v5;
      v24 = 115;
      return DefWindowProcW(this->hWnd, message, v24, v32);
    }
    for ( i = this->GetKeyEventTarget(); i; i = i->Parent )
    {
      // HD v1.7 backport — pass Win32 lParam bit 30 (previous-key-state)
      // so widgets can ignore auto-repeat for one-shot game hotkeys.
      if ( i->OnKeyDown(wParam, (v5 & 0x40000000) != 0) )
        break;
    }
    return 0;
  }
  if ( message <= 0x200 )
  {
    if ( message != 512 )
    {
      switch ( message )
      {
        case 0x101u:
        case 0x105u:
          for ( j = this->GetKeyEventTarget(); j; j = j->Parent )
          {
            if ( j->OnKeyUp(wParam) )
              break;
          }
          return 0;
        case 0x102u:
          for ( k = this->GetKeyEventTarget(); k; k = k->Parent )
          {
            if ( k->OnChar(wParam) )
              break;
          }
          return 0;
        case 0x104u:
          goto $LN23_12;
        default:
          goto LABEL_65;
      }
    }
    {
      v13 = this->GetEventTarget((short)v5, (short)HIWORD(v5), &target_x, &target_y);
      v25 = target_y;
      this->LastMouseButtons = wParam;
      v13->OnMouseMove(target_x, v25, wParam);
      return 0;
    }
  }
  switch ( message )
  {
    case 0x201u:
      v14 = this->GetEventTarget((short)v5, (short)HIWORD(v5), &target_x, &target_y);
      v26 = target_y;
      this->LastMouseButtons = wParam;
      v14->OnMouseDown(1, target_x, v26, wParam);
      return 0;
    case 0x202u:
      v17 = this->GetEventTarget((short)v5, (short)HIWORD(v5), &target_x, &target_y);
      v29 = target_y;
      this->LastMouseButtons = wParam;
      v17->OnMouseUp(1, target_x, v29, wParam);
      return 0;
    case 0x204u:
      v16 = this->GetEventTarget((short)v5, (short)HIWORD(v5), &target_x, &target_y);
      v28 = target_y;
      this->LastMouseButtons = wParam;
      v16->OnMouseDown(3, target_x, v28, wParam);
      return 0;
    case 0x205u:
      v19 = this->GetEventTarget((short)v5, (short)HIWORD(v5), &target_x, &target_y);
      v31 = target_y;
      this->LastMouseButtons = wParam;
      v19->OnMouseUp(3, target_x, v31, wParam);
      return 0;
    case 0x207u:
      v15 = this->GetEventTarget((short)v5, (short)HIWORD(v5), &target_x, &target_y);
      v27 = target_y;
      this->LastMouseButtons = wParam;
      v15->OnMouseDown(2, target_x, v27, wParam);
      return 0;
    case 0x208u:
      v18 = this->GetEventTarget((short)v5, (short)HIWORD(v5), &target_x, &target_y);
      v30 = target_y;
      this->LastMouseButtons = wParam;
      v18->OnMouseUp(2, target_x, v30, wParam);
      return 0;
    case 0x20Au:
      v20 = this->GetEventTarget((short)v5 - this->X, (short)HIWORD(v5) - this->Y, &target_x, &target_y);
      v20->OnMouseWheel((short)HIWORD(wParam) / 120, target_x, target_y, (unsigned short)wParam);
      return 0;
    case 0x2E0u:
      {
        RECT *pRect = (RECT *)v5;
        RECT dpiRect;
        dpiRect.left = pRect->left;
        dpiRect.top = pRect->top;
        dpiRect.right = pRect->right;
        dpiRect.bottom = pRect->bottom;
        this->OnDPIChange(dpiRect);
      }
      return 0;
    default:
LABEL_65:
      v21 = -1;
      break;
  }
  do
  {
    if ( ++v21 >= this->UserEvents.size )
      goto LABEL_34;
    v22 = &this->UserEvents.array[v21];
    while ( v22->use != 0x7FFFFFFF )
    {
      ++v21;
      ++v22;
      if ( v21 >= this->UserEvents.size )
        goto LABEL_34;
    }
    if ( v21 < 0 )
    {
LABEL_34:
      v32 = lParam;
      goto LABEL_35;
    }
    array = this->UserEvents.array;
  }
  while ( array[v21].data.Event != message );
  array[v21].data.Handler->OnUserEvent(message, wParam, lParam);
  return 0;
}

// ============================================================
// Missing virtual method stub
// ============================================================

void SWindow::OnActivateApp(bool active) {} // Base class virtual — intentionally empty
