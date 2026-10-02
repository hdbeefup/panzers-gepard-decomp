// src/tools/rendertest/rendertest.cpp
// rendertest: bring up the window + D3D9 device the Panzers way (windowed),
// put one TGA on the 2D board fullscreen and optionally a line of text in a
// Panzers ".font", then run the SWindow::Run message pump until Esc / close.
//
//   rendertest.exe <image.tga> [--font <menu font .font>] [--text "..."]
//                  [--seconds N]
//
// All paths are loose files, relative to the working directory (no paks).
// Writes rendertest.log (engine log + test steps) to the working directory.
//
// Boot order mirrors the HD exe (docs/re/TRIAGE.md section 4):
//   SDXWindow ctor (0x539c70) -> SetPosition -> SDXWindow::Create (0x539d70:
//   SWindow::Create, CreateGepard, sound) -> post-create board setup like
//   0x657540 (root FT_SCALER frame sized to the window, 1024x768 virtual
//   resolution) -> SWindow::Run (0x544db0) with OnIdle (vtbl +0x90) drawing.

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dxwindow.h"
#include "gepard.h"
#include "board.h"
#include "logger.h"
#include "stream.h"

extern HINSTANCE hInstance;

static const int kVirtualW = 1024;   // Panzers menu virtual resolution
static const int kVirtualH = 768;    // (0x657540: Board slot +0x58 (scaler, 0x400, 0x300))

static SLogger *g_log = nullptr;

#define RT_LOG(...) Logger.g->Log(-1, __VA_ARGS__)

struct SRenderTestWindow : SDXWindow {
    DWORD StartTick = 0;
    DWORD Seconds = 0;          // 0 = run until Esc / close
    int Frames = 0;
    bool ExitByEsc = false;
    bool ExitByTimer = false;

    bool OnIdle() override
    {
        bool r = SDXWindow::OnIdle();
        if ( !::Gepard )
            return r;
        ++Frames;
        if ( Frames == 1 )
            RT_LOG("rendertest: first frame presented");
        if ( Seconds && GetTickCount() - StartTick >= Seconds * 1000 )
        {
            ExitByTimer = true;
            RT_LOG("rendertest: --seconds %u elapsed, closing", Seconds);
            this->OnClose();
        }
        return r;
    }

    LRESULT WindowProc(unsigned int message, unsigned int wParam, int lParam) override
    {
        if ( message == WM_KEYDOWN && wParam == VK_ESCAPE )
        {
            ExitByEsc = true;
            RT_LOG("rendertest: Esc pressed, closing");
            this->OnClose();
            return 0;
        }
        return SDXWindow::WindowProc(message, wParam, lParam);
    }
};

static void OpenLog(const char *path)
{
    // SLogger(logging=false) skips the AppData log dir; then point it at a
    // file in the working directory. Note: Panic() shows a MessageBox.
    g_log = new SLogger((char *)"rendertest", (char *)"rendertest", false, 2);
    FILE *f = fopen(path, "wb");
    if ( f )
        fclose(f);
    g_log->LogFileName = _strdup(path);
    g_log->LogToFile = true;
    g_log->Logging = true;
    g_log->LogLevel = 2;
    Logger.g = g_log;
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    hInstance = hInst;
    char logpath[MAX_PATH];
    GetCurrentDirectoryA(sizeof(logpath) - 20, logpath);
    strcat(logpath, "\\rendertest.log");
    OpenLog(logpath);

    const char *image = nullptr, *font = nullptr;
    const char *text = "Codename: Panzers - Phase One  |  rendertest";
    unsigned seconds = 0;
    for ( int i = 1; i < __argc; ++i )
    {
        const char *a = __argv[i];
        if ( !strcmp(a, "--font") && i + 1 < __argc ) font = __argv[++i];
        else if ( !strcmp(a, "--text") && i + 1 < __argc ) text = __argv[++i];
        else if ( !strcmp(a, "--seconds") && i + 1 < __argc ) seconds = (unsigned)atoi(__argv[++i]);
        else if ( !image ) image = a;
    }
    RT_LOG("rendertest: image=%s font=%s seconds=%u", image ? image : "(none)",
           font ? font : "(none)", seconds);
    RT_LOG("rendertest: x86 sizeof SWidget=0x%X SWindow=0x%X SDXWindow=0x%X SGepard=0x%X SBoard=0x%X SFontProp=0x%X SFrame=0x%X",
           (unsigned)sizeof(SWidget), (unsigned)sizeof(SWindow), (unsigned)sizeof(SDXWindow),
           (unsigned)sizeof(SGepard), (unsigned)sizeof(SBoard), (unsigned)sizeof(SFontProp),
           (unsigned)sizeof(SFrame));
    if ( !image )
    {
        RT_LOG("rendertest: usage: rendertest <image.tga> [--font f.font] [--text s] [--seconds N]");
        return 2;
    }

    SRenderTestWindow *wnd = new SRenderTestWindow();
    wnd->Seconds = seconds;
    // Windowed, client area = the Panzers menu resolution.
    wnd->SetPosition(40, 40, kVirtualW, kVirtualH);
    int rc = wnd->Create((HICON)0, (HCURSOR)IDC_ARROW, L"Panzers rendertest");
    RT_LOG("rendertest: SDXWindow::Create -> %d (hWnd=%p Gepard=%p Board=%p)", rc,
           (void *)wnd->hWnd, (void *)::Gepard, (void *)::Board);
    if ( rc != 0 || !::Gepard || !::Board )
    {
        RT_LOG("rendertest: device creation failed");
        return 3;
    }

    // Post-create board setup as in 0x657540: a root scaler frame sized to
    // the window, scaled from the 1024x768 virtual resolution.
    RECT rcl;
    GetClientRect(wnd->hWnd, &rcl);
    int cw = rcl.right - rcl.left, ch = rcl.bottom - rcl.top;
    int scaler = ::Board->CreateFrame(FT_SCALER, 0, 0, 0, 0, false);
    ::Board->ResizeFrame(scaler, cw, ch);
    ::Board->SetScaleFactor(scaler, (float)cw / (float)kVirtualW);
    ::Board->ShowFrame(scaler, true);
    RT_LOG("rendertest: client %dx%d, scaler frame %d", cw, ch, scaler);

    int imgFont = ::Board->LoadSingleFont(image, Default);
    int sprite = ::Board->CreateFrame(FT_SPRITE, scaler, 0, 0, 0, false);
    ::Board->SetSpriteGlyph(sprite, imgFont, 0);
    ::Board->ShowFrame(sprite, true);
    int iw = 0, ih = 0;
    ::Board->GetFrameSize(sprite, &iw, &ih);
    RT_LOG("rendertest: image font %d, sprite frame %d (%dx%d)", imgFont, sprite, iw, ih);

    if ( font )
    {
        int txtFont = ::Board->LoadFontFileFont(font);
        int tf = ::Board->CreateFrame(FT_TEXT, scaler, 32, 32, 0, true);
        ::Board->SetTextColor(tf, 0xFFFFFFFF);
        ::Board->SetText(tf, txtFont, 0, text);
        ::Board->ShowFrame(tf, true);
        int tw = 0, th = 0;
        ::Board->GetTextExtent(txtFont, text, (int)strlen(text), &tw, &th, 1.0f);
        RT_LOG("rendertest: text font %d (%s), frame %d, extent %dx%d", txtFont, font, tf, tw, th);
    }

    wnd->StartTick = GetTickCount();
    RT_LOG("rendertest: entering SWindow::Run");
    wnd->Run();
    RT_LOG("rendertest: SWindow::Run returned after %d frames (esc=%d timer=%d)",
           wnd->Frames, (int)wnd->ExitByEsc, (int)wnd->ExitByTimer);
    bool ok = wnd->Frames > 0;
    delete wnd;
    RT_LOG("rendertest: exit %d", ok ? 0 : 4);
    return ok ? 0 : 4;
}
