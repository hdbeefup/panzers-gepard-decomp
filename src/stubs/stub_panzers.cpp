// src/stubs/stub_panzers.cpp
// Logged stand-ins for HD PANZERS.exe code that the app shell / main menu
// (src/panzers, P2-D) reaches but that is not lifted yet: everything past a
// menu click, the command-line multiplayer paths, the unit registry and the
// 3D menu background. Each comment names the HD address it stands in for.
// None of these bodies come from the original; each is a minimal stand-in.

#include <windows.h>
#include <stdio.h>
#include "stub_log.h"
#include "logger.h"
#include "settings.h"

struct SWidget;
struct SSuperWindow;

static void StubDetail(const char* fmt, const char* a)
{
    if (Logger.g)
        Logger.g->Log(0, fmt, a ? a : "");
}

// HD 0x64fdd0: SSettings writer (options.ini). Not needed to reach the menu.
void SSettings::Save()
{
    STUB_LOG("SSettings::Save (0x64fdd0)");
}

// HD SSettings::CHECKCDKEY 0x64d390 (exported). The HD build only checks the
// key on the command-line multiplayer path; the stub accepts nothing so that
// path stops, as with an invalid key.
bool PzStub_CheckCDKey(const char* key)
{
    STUB_LOG("SSettings::CHECKCDKEY (0x64d390)");
    (void)key;
    return false;
}

// HD 0x6576f0 SSuperWindow::CreateHostFromCommandLine (-server / +host).
void PzStub_CreateHostFromCommandLine()
{
    STUB_LOG("SSuperWindow::CreateHostFromCommandLine (0x6576f0)");
}

// HD 0x657460 SSuperWindow::ConnectToHostFromCommandLine (-client / +connect).
void PzStub_ConnectToHostFromCommandLine(const char* host)
{
    STUB_LOG("SSuperWindow::ConnectToHostFromCommandLine (0x657460)");
    StubDetail("  connect host: %s", host);
}

// HD Play 0x65b470 "-market" branch: SPanzersCampaign 0x590ec0 (0xb8c),
// SMulti 0x51dcf0 (0x5130), multi view 0x63f2f0 (0x25b4).
void PzStub_StartCampaignFromCommandLine()
{
    STUB_LOG("SSuperWindow::Play -market campaign start (0x590ec0/0x51dcf0/0x63f2f0)");
}

// HD Play 0x65b470 map branch: SPanzersCampaign::LoadMap 0x5944d0 + 0x6585e0.
void PzStub_LoadMapFromCommandLine(const char* map)
{
    STUB_LOG("SSuperWindow::Play load map from command line (0x5944d0)");
    StubDetail("  map: %s", map);
}

// HD 0x658a30 SSuperWindow::LoadMultiPreMenu.
void PzStub_LoadMultiPreMenu()
{
    STUB_LOG("SSuperWindow::LoadMultiPreMenu (0x658a30)");
}

// HD 0x658050 SSuperWindow::LoadChatRoomView (GameSpy chat).
void PzStub_LoadChatRoomView()
{
    STUB_LOG("SSuperWindow::LoadChatRoomView (0x658050)");
}

// HD 0x5cfe30 (new 0x124): unit registry ctor, which calls
// SUnitRegistry::LoadUnitFiles 0x5d1050 (exported). The main menu does not
// read unit data; returns a dummy non-null handle.
void* PzStub_CreateUnitRegistry()
{
    STUB_LOG("SUnitRegistry ctor + LoadUnitFiles (0x5cfe30 / 0x5d1050)");
    static int dummy;
    return &dummy;
}

// HD 0x5d0c10: unit registry dtor.
void PzStub_DestroyUnitRegistry(void* reg)
{
    STUB_LOG("SUnitRegistry dtor (0x5d0c10)");
    (void)reg;
}

// HD 0x658690 world part: maps/menu.map -> SWorld 0x5d2f90 (0x7538),
// map load 0x5f1990, camera 0x55e440 (0x318). The 3D tank scene behind the
// main menu. Needs the world/game layer.
void PzStub_LoadMenuWorld(SSuperWindow* sw)
{
    STUB_LOG("SSuperWindow::LoadMenuBackground maps/menu.map world (0x658690 -> 0x5d2f90/0x5f1990)");
    (void)sw;
}

// HD 0x659250: SSuperWindow::OnAction cases the shell does not handle yet
// (campaign 0x4d4d1, multiplayer 0x4d4d2, tutorial 0x4d4d3, training camp
// 0x4d4d4, options 0x4d4d5, us-02 0x4d4d9, game view, ...).
void PzStub_SuperWindowAction(int action)
{
    STUB_LOG("SSuperWindow::OnAction unhandled action (0x659250)");
    if (Logger.g)
        Logger.g->Log(0, "STUB: SSuperWindow::OnAction action 0x%X ignored", action);
}

// HD New Game button: SMainMenu::OnAction 0x63b6f0 -> new 0x320 (0x633770)
// campaign selection, Create 0x639790.
void PzStub_NewGameMenu(SWidget* parent)
{
    STUB_LOG("SMainMenu New Game submenu (0x633770 / 0x639790)");
    (void)parent;
}

// HD Load Game button: new 0x280 (0x62cbc0), Create 0x62f950(1).
void PzStub_LoadGameMenu(SWidget* parent)
{
    STUB_LOG("SMainMenu Load Game submenu (0x62cbc0 / 0x62f950)");
    (void)parent;
}

// HD SVersion::GetVersionString 0x65c070 (exported) over 0x65bbf0. The HD
// install shows "Version: 1.25" in the menu (original_menu.png reference run).
const char* PzStub_GetVersionString()
{
    STUB_LOG("SVersion::GetVersionString (0x65c070)");
    return "1.25";
}

// HD SSuperWindow::Initialize 0x657910: Gepard +0x10 render options (8/9
// texture filter, 3 shadow buffer size, 2 shadows, 0/5/6 = 1). No
// SWINE-renderer match. (Board +0x9c/+0x94 are lifted in superwindow.cpp.)
void PzStub_GepardRenderStates()
{
    STUB_LOG("SSuperWindow::Initialize Gepard render options (Gepard +0x10)");
}
