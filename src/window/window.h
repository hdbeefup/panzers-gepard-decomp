// window/window.h
// SWindow header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SWINDOW_H
#define WINDOW_SWINDOW_H

#include "chain.h"
#include "widget.h"

struct SStream;

struct UserEventProp {
    unsigned int Event;
    SWidget *Handler;
};

enum SDisplayMode : int {
    Windowed = 0,
    BorderlessWindowed = 1,
    ExclusiveFullscreen = 2,
};

enum SAntialiasingMode : int {
    Off = 0,
    MSAA2x = 1,
    MSAA4x = 2,
    MSAA8x = 3,
};

struct SWindow : SWidget {
    wchar_t ClassName[32];
    HWND hWnd;
    unsigned int Style;
    int Index;
    int ModalResult;
    SWidget *ModalTarget;
    SStream *EventStream;
    int EventDirection;
    SHeap<UserEventProp> UserEvents;
    int LastMouseX;
    int LastMouseY;
    int LastMouseButtons;
    bool MouseCursorRestricted;
    int Dpi;
    float Scaling;
    bool shouldClose;

    SWindow();
    ~SWindow() override;

    operator HWND();

    // SWidget overrides
    bool IsWindow() override;
    void SetPosition(int x, int y, int width, int height) override;
    SWidget *GetEventTarget(int x, int y, int *local_x, int *local_y) override;

    // SWindow virtual methods (new virtuals, not SWidget overrides)
    virtual void OnPaint(HDC hdc);
    virtual void OnDestroy();
    virtual void OnClose();
    virtual bool OnIdle();
    virtual int OnSetCursor();
    virtual void OnDPIChange(RECT r);
    virtual void OnActivateApp(bool active);
    virtual LRESULT WindowProc(unsigned int message, unsigned int wParam, int lParam);

    // Panzers HD SWindow vtable slots +0x7C / +0x80: two virtuals that SWINE
    // does not have (empty in SWindow 0x539f40 / 0x539f30, both `ret 4`).
    // SSuperWindow overrides them (0x659020 shows a status/notification text
    // box from a message record, 0x658fc0 removes it). Real names unknown.
    virtual void ShowStatusMessage(void *message);
    virtual void HideStatusMessage(void *unused);

    // Non-virtual methods
    void Run();
    void CloseEventStream();
    int Create(HICON icon, HCURSOR cursor, const wchar_t *title, unsigned int style, SWindow *parent, bool maximized);
    char EventFrame(int a2, int a3);
    void FlushMessages();
    bool InitDPI();
    bool IsMouseCursorRestricted();
    void PlaybackEvents(const char *filename);
    int ProcessMessages();
    void RecordEvents(char *filename);
    void RegisterUserEventHandler(unsigned int event, SWidget *handler);
    void RestrictMouseCursor(bool enable);
    bool RunModal(SWidget *modal_widget);
    void SetWindowStyle(unsigned int style);
    void UpdateMouse();
    void UpdateMouseCursorRestriction();

    // HD-only (PANZERS addresses in window.cpp)
    void SetModalWidget(SWidget *widget);                  // 0x544fe0 non-blocking: events go to the widget's subtree
    void UnsetModalWidget(SWidget *widget);                // 0x5450e0
    void GetLastMousePosition(int *x, int *y);             // 0x544c20
};

#endif // WINDOW_SWINDOW_H
