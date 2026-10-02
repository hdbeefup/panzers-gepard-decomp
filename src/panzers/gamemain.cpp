// src/panzers/gamemain.cpp
// GameMain (HD 0x64c870) and its SEH wrapper (0x64c800).

#include <windows.h>
#include "superwindow.h"
#include "settings.h"
#include "logger.h"
#include "timer.h"

// Engine singletons (Board, Gepard, Concert, Options, Timer, TheWindow,
// hInstance) stay in src/stubs/stub_globals.cpp for now: the console tools
// (paktest, rendertest, soundtest) link the engine without this game layer.
// HD addresses: renderer DAT_008f1c58, board DAT_008f1c60, concert
// DAT_008f1c5c, logger 0x929f28, timer 0x92e354.

// PANZERS 0x64c870
void GameMain()
{
    g_SuperWindow = new SSuperWindow();                 // new 0x1c0, ctor 0x656d50
    g_SuperWindow->Create(0x67, 0x7f00, "PANZERS");     // 0x657540 (icon 0x67, IDC_ARROW)
    g_SuperWindow->Play();                              // 0x65b470
    if (g_SuperWindow) {
        delete g_SuperWindow;                           // vtbl +0x00 (deleting dtor 0x6573e0)
        g_SuperWindow = nullptr;
    }
}

// PANZERS 0x64c800
// HD wraps GameMain in an SEH frame (handler 0x76c960, the CRT's
// _except_handler4) with no filter of its own. The recompile's unhandled-
// exception filter (src/main/crashhandler.cpp) does the crash reporting.
void RunGame()
{
    GameMain();
}
