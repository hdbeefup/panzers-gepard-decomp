// window/widget.h
// SWidget — base widget class
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_WIDGET_H
#define WINDOW_WIDGET_H

#include <cstdint>
#include "string2.h"
#include "chain.h"
#include "iboard.h"

struct SIGepard;
struct SBoard;
struct SStream;
struct SLogger;
struct SWindow;

#include "iconcert.h"

// Global pointers used by all widgets
extern SIGepard *Gepard;
extern SIBoard *Board;
extern SIConcert *Concert;

struct SWidget {
    int X;
    int Y;
    int Width;
    int Height;
    SWidget *Parent;
    SWidget *Child;
    SWidget *Sibling;
    SWidget *Focus;
    SWidget *FocusSibling;
    SWidget *Left;
    SWidget *Right;
    SWidget *Up;
    SWidget *Down;
    bool Enabled;
    bool Visible;
    int Cursor;
    int Gravity;

    // Static members
    static SWidget *CaptureTarget;
    static SWidget *LastMouseTarget;
    // x64: SendAction's `int param` truncates pointers on 64-bit. Sites that
    // need to send a pointer payload alongside an action stash it here just
    // before SendAction; the receiver reads it instead of `param`.
    static intptr_t ActionPayload;

    // Constructor / destructor
    SWidget();
    virtual ~SWidget();

    // Virtual methods
    virtual void GetPosition(int *x, int *y, int *w, int *h);
    virtual void SetPosition(int x, int y, int w, int h);
    virtual void Resize(int w, int h);
    virtual bool OnKeyDown(int key, bool repeat = false);
    virtual bool OnKeyUp(int key);
    virtual bool OnChar(int ch);
    virtual void OnMouseDown(int button, int x, int y, int shift);
    virtual void OnMouseUp(int button, int x, int y, int shift);
    virtual void OnMouseMove(int x, int y, int shift);
    virtual void OnMouseWheel(int button, int x, int y, int delta);
    virtual void OnMouseOver();
    virtual void OnMouseOut();
    virtual void OnMove(int x, int y);
    virtual void OnSize(int w, int h);
    virtual bool OnAction(SWidget *source, int action, int param);
    virtual void OnUpdate();
    virtual void OnUserEvent(unsigned int event, unsigned int wparam, int lparam);
    virtual void OnTimer(int id, unsigned int elapsed);
    virtual void InsertChild(SWidget *child);
    virtual void RemoveChild(SWidget *child);
    virtual SWidget *GetEventTarget(int x, int y, int *local_x, int *local_y);
    virtual void ParentToChild(int *x, int *y);
    virtual void ChildToParent(int *x, int *y);
    virtual bool IsWindow();
    virtual bool IsScaler();
    virtual bool CanAcceptEvents();
    virtual void SetFocus();
    virtual int GetFrame();
    virtual void SetVisible(bool visible);
    virtual void SetEnable(bool enable);
    virtual void SetGravity(int gravity);
    virtual void Update();

    // Non-virtual methods
    SWidget *GetKeyEventTarget();
    void SetMouseTarget();
    void TranslateEventFromWindow(int *x, int *y);
    void CaptureMouse();
    void Create(int a2);
    void Destroy();
    SWidget *FindFocus();
    void GetAbsolutePosition(int *x, int *y, int *w, int *h);
    static int GetCurrentCursor();
    SWidget *GetSibling();
    void InsertBefore(SWidget *sibling);
    void InsertFirst(SWidget *child);
    SWidget *GetFocusTargetFromKeyCode(int keycode);
    SWidget *GetFocusTargetFromParam(int param);
    bool HasMouseCaptured();
    void KillTimer(int *id);
    void Move(int x, int y);
    void ReleaseFocus();
    void ReleaseMouse();
    void SendAction(int action, int param);
    void SetCursor(int cursor);
    void SetFocusOrder(SWidget *left, SWidget *right, SWidget *up, SWidget *down);
    void SetNavigation(SWidget *Left, SWidget *Right, SWidget *Up, SWidget *Down);
    void SetNextFocusSibling(SWidget *sibling);
    int SetTimer(unsigned int elapse);
    SWindow *GetWindowParent();
    SWidget *GetWindowOrScalerParent();
    void GetWindowOrParentScalerPosition(int *x, int *y, float *scaleFactor);

    // HD-only helpers (PANZERS addresses in widget.cpp)
    SWidget *GetWindow();                                  // 0x5435b0 this or the nearest window parent
    void GetWindowPosition(int *x, int *y);                // 0x5435d0
    bool IsFocused();                                      // 0x543670
    static void FormatKeyHint(SString *out, const char *text, unsigned int scanCode); // 0x543c00
};

// Timer data for global TimerList
struct TimerItem {
    SWidget *Target;
    UINT_PTR IDEvent;
};
extern SHeap<TimerItem> TimerList;
extern void CALLBACK TimerProc(HWND hwnd, UINT msg, UINT_PTR id, DWORD time);

// Global localization function
char *GetText(const char *key);

// SOptions defined in common/options.h
#include "options.h"
extern SOptions *Options;

#endif // WINDOW_WIDGET_H
