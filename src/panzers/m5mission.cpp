// src/panzers/m5mission.cpp
// Recompile-only test hooks for the campaign mission sweep (docs/m5/ms.md).
// HD has none of this; every hook is inert when its variable is unset.
//
//   PZ_M5_MISSION=<missions.ini section>   ("German 5", "Allied 3", ...)
//       InitCampaignMode 0x592b20 starts at that section instead of the
//       nation's first mission; the race follows the section's first word
//       (German 0, Allied 1, Russian 2).
//   PZ_M5_AUTO=1   (needs -m3 and PZ_M5_MISSION)
//       The first main menu starts New Game (as 0x53441 does) with
//       difficulty PZ_M5_DIFF (default 1, Normal), and the screens between
//       the menu and the mission are passed by sending their actions: the
//       briefing (0x424d1), the loading screen (0x47561) and the market
//       (0x4d542).
//   PZ_M5_FRAMES=<n>
//       Once the mission's logic reaches frame n, "PZM5: frame n" is logged
//       and the window gets WM_CLOSE.
//   PZ_M5_WATCHDOG=<s>
//       A thread writes the main thread's stack to hang.txt when OnIdle has
//       not run for <s> seconds (hangs in the sweep).
//   PZ_M5_NOGHOST=1
//       DisableProcessWindowsGhosting at the first main menu (see
//       M5OnMainMenu): keeps busy-machine test runs clear of the WM_SIZE that
//       makes the in-mission device Reset fail.

#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "superwindow.h"
#include "mainmenu.h"
#include "briefing.h"
#include "market.h"
#include "gameview.h"
#include "campaign.h"
#include "m3common.h"
#include "logger.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"

void M3LoadNextCampaignView(SSuperWindow* sw);

namespace pz {

const char* PzM5MissionSection()
{
    const char* e = getenv("PZ_M5_MISSION");
    return (e && *e) ? e : nullptr;
}

int PzM5RaceOfSection(const char* s, int fallback)
{
    if (!_strnicmp(s, "German", 6))
        return 0;
    if (!_strnicmp(s, "Allied", 6))
        return 1;
    if (!_strnicmp(s, "Russian", 7))
        return 2;
    return fallback;
}

} // namespace pz

using pz::PzM5MissionSection;
using pz::PzM5RaceOfSection;

// ---------------------------------------------------------------------------
// PZ_M5_WATCHDOG=<seconds>: a thread that writes the main thread's stack to
// hang.txt when SSuperWindow::OnIdle has not run for that
// long (test runs: the sweep finds hangs with it).

static volatile DWORD s_LastIdle = 0;
static HANDLE s_MainThread = nullptr;

static DWORD WINAPI M5Watchdog(void* arg)
{
    DWORD limit = (DWORD)(size_t)arg * 1000;
    bool reported = false;
    for (;;) {
        Sleep(1000);
        DWORD last = s_LastIdle;
        if (!last || GetTickCount() - last < limit) {
            reported = false;
            continue;
        }
        if (reported)
            continue;
        reported = true;
        if (SuspendThread(s_MainThread) == (DWORD)-1)
            continue;
        CONTEXT ctx;
        memset(&ctx, 0, sizeof(ctx));
        ctx.ContextFlags = CONTEXT_FULL;
        if (GetThreadContext(s_MainThread, &ctx)) {
            HANDLE proc = GetCurrentProcess();
            HMODULE base = GetModuleHandleA(nullptr);
            SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
            BOOL syms = SymInitialize(proc, nullptr, TRUE);
            STACKFRAME64 sf;
            memset(&sf, 0, sizeof(sf));
            sf.AddrPC.Offset = ctx.Eip;    sf.AddrPC.Mode = AddrModeFlat;
            sf.AddrFrame.Offset = ctx.Ebp; sf.AddrFrame.Mode = AddrModeFlat;
            sf.AddrStack.Offset = ctx.Esp; sf.AddrStack.Mode = AddrModeFlat;
            FILE* f = fopen("hang.txt", "w");
            if (!f) {
                ResumeThread(s_MainThread);
                continue;
            }
            fprintf(f, "PZM5 HANG: no idle for %lu s, main thread stack:\n", (unsigned long)(limit / 1000));
            for (int i = 0; i < 40; ++i) {
                if (!StackWalk64(IMAGE_FILE_MACHINE_I386, proc, s_MainThread, &sf, &ctx, nullptr,
                                 SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
                    break;
                DWORD64 pc = sf.AddrPC.Offset;
                if (!pc)
                    break;
                char buf[sizeof(SYMBOL_INFO) + 256];
                SYMBOL_INFO* sym = (SYMBOL_INFO*)buf;
                sym->SizeOfStruct = sizeof(SYMBOL_INFO);
                sym->MaxNameLen = 255;
                DWORD64 disp = 0;
                const char* name = "?";
                if (syms && SymFromAddr(proc, pc, &disp, sym))
                    name = sym->Name;
                fprintf(f, "  #%02d RVA 0x%08lX %s+0x%lX\n", i,
                              (unsigned long)(pc - (DWORD64)(UINT_PTR)base), name, (unsigned long)disp);
            }
            if (syms)
                SymCleanup(proc);
            fclose(f);
        }
        ResumeThread(s_MainThread);
    }
    return 0;
}

static void M5StartWatchdog()
{
    const char* w = getenv("PZ_M5_WATCHDOG");
    if (!w || atoi(w) <= 0 || s_MainThread)
        return;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &s_MainThread, 0, FALSE,
                    DUPLICATE_SAME_ACCESS);
    s_LastIdle = GetTickCount();
    CreateThread(nullptr, 0, M5Watchdog, (void*)(size_t)atoi(w), 0, nullptr);
}

void M5OnMainMenu(SSuperWindow* sw)
{
    static bool s_Ghost = false;
    if (!s_Ghost) {
        s_Ghost = true;
        M5StartWatchdog();                                         // PZ_M5_WATCHDOG
        // PZ_M5_NOGHOST=1: no window ghosting. A map load that keeps the
        // message loop away for > 5 s on a busy machine lets Windows ghost
        // the window; when the ghost goes, the window gets a WM_SIZE, and the
        // device Reset that follows fails in a mission (D3DERR_INVALIDCALL).
        // Test runs only; the bug itself is in the renderer (docs/m5/ms.md).
        if (getenv("PZ_M5_NOGHOST")) {
            DisableProcessWindowsGhosting();
            Logger.g->Log(0, "PZM5: window ghosting disabled");
        }
    }
    static bool s_Done = false;
    const char* sec = PzM5MissionSection();
    if (s_Done || !sec || !getenv("PZ_M5_AUTO") || !pz::g_M3.Enabled)
        return;
    s_Done = true;
    const char* d = getenv("PZ_M5_DIFF");
    delete pz::g_Campaign;
    pz::g_Campaign = new pz::SPanzersCampaign();                   // as 0x53441
    pz::g_Campaign->Race = PzM5RaceOfSection(sec, 0);
    pz::g_Campaign->Difficulty = d ? atoi(d) : 1;
    Logger.g->Log(0, "PZM5: New Game %s, race %d, difficulty %d", sec, pz::g_Campaign->Race,
                  pz::g_Campaign->Difficulty);
    pz::g_Campaign->InitCampaignMode();                            // 0x592b20
    // The main menu is deleted on the next idle call (we are inside
    // LoadMainMenu here); LoadNextCampaignView follows it.
}

void M5OnIdle(SSuperWindow* sw)
{
    s_LastIdle = GetTickCount();
    static int s_Stage = 0;                                        // 0 start, 1 running, 2 closed
    static DWORD s_Last = 0;
    static int s_Frames = -2;
    if (s_Frames == -2) {
        const char* f = getenv("PZ_M5_FRAMES");
        s_Frames = f ? atoi(f) : -1;
    }
    bool aut = getenv("PZ_M5_AUTO") && PzM5MissionSection() && pz::g_M3.Enabled;
    if (aut && pz::g_Campaign && GetTickCount() - s_Last > 1500) {
        if (s_Stage == 0 && sw->MainMenu) {
            s_Last = GetTickCount();
            delete sw->MainMenu;
            sw->MainMenu = nullptr;
            s_Stage = 1;
            M3LoadNextCampaignView(sw);                            // 0x658b10
            return;
        }
        if (sw->Menu_10c) {                                        // briefing
            s_Last = GetTickCount();
            Logger.g->Log(0, "PZM5: briefing -> 0x424d1");
            sw->OnAction(nullptr, PZA_BRIEFING_DONE, 0);
            return;
        }
        SGameView* v = static_cast<SGameView*>(sw->GameView);
        static DWORD s_MarketSince = 0;                            // PZ_M5_MARKETWAIT=<s>: stay in the market
        const char* mw = getenv("PZ_M5_MARKETWAIT");
        if (sw->MultiView && mw && !s_MarketSince)
            s_MarketSince = GetTickCount();
        if (sw->MultiView && mw && GetTickCount() - s_MarketSince < (DWORD)atoi(mw) * 1000)
            return;
        if (sw->MultiView) {                                       // market
            s_Last = GetTickCount();
            Logger.g->Log(0, "PZM5: market -> 0x4d542");
            sw->OnAction(nullptr, PZA_MARKET_START, 0);
            return;
        }
        if (v && v->LoadingScreen) {
            s_Last = GetTickCount();
            Logger.g->Log(0, "PZM5: loading screen -> 0x47561");
            v->OnKeyDown(VK_SPACE, false);                         // as a key on "Click to continue"
            return;
        }
    }
    if (s_Frames > 0 && s_Stage != 2) {
        SGameView* v = static_cast<SGameView*>(sw->GameView);
        if (v && v->Logic && v->Logic->Frame >= s_Frames) {
            s_Stage = 2;
            Logger.g->Log(0, "PZM5: frame %d reached, %d units", v->Logic->Frame,
                          pz::g_World ? pz::g_World->Units.Count : -1);
            PostMessageA(sw->hWnd, WM_CLOSE, 0, 0);
        }
    }
}
