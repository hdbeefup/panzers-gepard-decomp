// window/dxwindow.h
// SDXWindow header
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef WINDOW_SDXWINDOW_H
#define WINDOW_SDXWINDOW_H

#include "darray.h"
#include "window.h"

struct SMessageBox;

struct SDXWindow : SWindow {
    SDisplayMode DisplayMode;
    bool VSync;
    SAntialiasingMode AntialiasingMode;
    int Monitor;
    int FullScreenWidth;
    int FullScreenHeight;
    int WindowedX;
    int WindowedY;
    int WindowedWidth;
    int WindowedHeight;
    int DesktopX;
    int DesktopY;
    int DesktopWidth;
    int DesktopHeight;
    SDArray<RECT> MonitorRects;
    int CursorX;
    int CursorY;
    SMessageBox *MBox;

    SDXWindow();
    ~SDXWindow() override;

    // SWidget overrides
    int GetFrame() override;
    void OnSize(int width, int height) override;
    void SetPosition(int x, int y, int width, int height) override;

    // SWindow overrides
    void OnClose() override;
    void OnDestroy() override;
    bool OnIdle() override;
    void OnPaint(HDC hdc) override;
    int OnSetCursor() override;
    LRESULT WindowProc(unsigned int message, unsigned int wParam, int lParam) override;

    // SDXWindow new virtual methods
    virtual bool IsFullScreen();
    virtual void SetDisplayMode(SDisplayMode displayMode, bool vsync, SAntialiasingMode antialiasingMode, int monitor, int fullscreenWidth, int fullscreenHeight);
    virtual void InitDesktopSize(int *monitorIdx);
    virtual int GetMSAALevel();

    // Non-virtual methods
    static int CALLBACK AddMonitorsCallBack(HMONITOR hMonitor, HDC hdcMonitor, RECT *lprcMonitor, SDArray<RECT> *rects);
    int Create(HICON icon, HCURSOR cursor, const wchar_t *title);
    bool IsThereMessageBox();
    int MessageBoxA(SWidget *windowOrScaler, const char *text, int type);
    void SetCursorPos(int x, int y);
};

#endif // WINDOW_SDXWINDOW_H
