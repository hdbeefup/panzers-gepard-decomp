// src/panzers/scenariomenu.cpp
// Scenario mode (scenariomenu.h): SScenarioMenu, the super window's
// 0x53431 case and Play's "-map" branch. OWNER: agent M5-SC. Lifted from
// the HD exe.

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include "scenariomenu.h"
#include "campaignmenu.h"
#include "superwindow.h"
#include "mainmenu.h"
#include "campaign.h"
#include "pzboard.h"
#include "stream.h"
#include "gettext.h"
#include "logger.h"
#include "m3common.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "mods.h"

void M3LoadNextCampaignView(SSuperWindow* sw);   // superwindow_m3.cpp 0x658b10

static const char* Mm(const char* id) { return GetText("panzers/MainMenu.cpp", id); }

static void FreeStr(SString* s)
{
    delete[] s->buf;
    s->buf = nullptr;
    s->size = 0;
}

static void FreeFiles(SDArray<SString>* a)
{
    for (int i = 0; i < a->size; ++i)
        FreeStr(&a->array[i]);
    free(a->array);
    a->array = nullptr;
    a->size = a->maxsize = 0;
}

// HD 0x634d30: the qsort compare of the file list (SString, case-insensitive).
static int CompareFiles(const void* a, const void* b)
{
    const SString* x = (const SString*)a;
    const SString* y = (const SString*)b;
    return _stricmp(x->buf ? x->buf : "", y->buf ? y->buf : "");
}

// ---------------------------------------------------------------------------
// SScenarioMenu
// ---------------------------------------------------------------------------

// PANZERS 0x63b2d0 (the inline ctor: SCenterMenu 0x64bab0, SListBox 0x53bdc0
// at +0x58, SComplexButton 0x538640 at +0x18c, +0x200..+0x208 = 0)
SScenarioMenu::SScenarioMenu()
{
    Files.array = nullptr;
    Files.size = 0;
    Files.maxsize = 0;
}

// PANZERS 0x634bf0
SScenarioMenu::~SScenarioMenu()
{
    FreeFiles(&Files);                                            // 0x582d20(+0x200)
}

// PANZERS 0x639000
// "Scenario" centre menu: the Load button at the right-hand slot, the
// "Maps" caption and a 14-line list of every scenarios/*.map the file system
// finds (loose folders and paks), sorted case-insensitively. The list shows
// the name after "scenarios/" (a "maps/" entry would show after "maps/";
// anything else panics). The first entry is selected; with none, Load is off.
void SScenarioMenu::Create()
{
    PZ_M3_TRACE("SScenarioMenu::Create (0x639000)");
    SCenterMenu::Create(Mm("Scenario"), true);                    // 0x64bc80(text, 1)
    InsertChild(&Load);                                           // 0x64bfd0(+0x18c, "Load")
    Load.SetPosition(0x18b, 400, 0, 0);
    Load.Create(1, Mm("Load"));                                   // 0x5386c0(1, text)
    Cursor = 0;                                                   // 0x543970(0, -1)
    int caption = Board->CreateFrame(FT_TEXT, BackFrame, 0x32, 0x32, 0, 1);   // board +0x08(2, +0x48, 0x32, 0x32, 0, 1)
    Board->SetText(caption, g_PzFont[2], 0, Mm("Maps"));         // board +0x34(f, 2, 0, "Maps")
    InsertChild(&List);                                           // vtbl +0x54(+0x58)
    List.SetPosition(0x28, 0x46, 0x208, 0);                       // vtbl +0x08
    List.Create(2, 0xe, true, true, 0, true);                     // 0x53c260(2, 0xe, 1, 1, 0, 1)
    List.SetFocus();                                              // 0x5439f0

    SDArray<SString> found;                                       // 0x14-byte records in HD (name, size, time)
    memset(&found, 0, sizeof(found));
    FileSystem.FindFiles("scenarios/", "*.map", &found);          // 0x65e720("scenarios/*.map")
    for (int i = 0; i < found.size; ++i) {
        const char* name = found.array[i].buf ? found.array[i].buf : "";
        SString path;                                             // 0x5335c0("scenarios/", name)
        path.size = (int)strlen("scenarios/") + (int)strlen(name);
        path.buf = new char[path.size + 1];
        strcpy(path.buf, "scenarios/");
        strcat(path.buf, name);
        int idx = Files.Add();                                    // grow 16, then 6/5
        Files.array[idx] = path;
    }
    qsort(Files.array, Files.size, sizeof(SString), CompareFiles);   // 0x634d30
    for (int i = 0; i < Files.size; ++i) {
        const char* f = Files.array[i].buf ? Files.array[i].buf : "";
        if (strncmp(f, "maps/", 5) == 0)                          // 0x58c290(0, "maps/") == 0
            List.AddItem(f + 5, nullptr, 0xbfbfbf, 0);            // 0x5336b0(5, -5), 0x53c020(s, 0, 0xbfbfbf, 0)
        else if (strncmp(f, "scenarios/", 10) == 0)               // 0x58c290(0, "scenarios/") == 0
            List.AddItem(f + 10, nullptr, 0xbfbfbf, 0);           // 0x5336b0(10, -10)
        else
            Logger.g->Panic("SScenarioMenu::Create() - Invalid filename: %s", f);
    }
    if (List.ItemCount > 0) {                                     // +0x170
        List.CurSel = 0;                                          // +0xc8
        List.Update();                                            // vtbl +0x78
    }
    if (List.CurSel < 0)
        Load.SetEnable(false);                                    // vtbl +0x70(0)
    SetFocus();                                                   // 0x5439f0
    Cursor = 0;                                                   // 0x543970(0, -1)
    FreeFiles(&found);                                            // 0x591150
    Logger.g->Log(0, "PZM5: SScenarioMenu: %d scenarios", Files.size);
}

// PANZERS 0x63c450
// Load (or a double click on the list): a new campaign holding the selected
// file as its map name (+0x20), then 0x53431 to the super window.
bool SScenarioMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action == PZA_BUTTON_CLICK) {                             // 0x42542
        if (source != &Load)                                      // +0x18c
            return false;
    } else if (action != PZA_LISTBOX_DBLCLICK) {                  // 0x4c423
        return false;
    }
    delete pz::g_Campaign;                                        // 0x591350 + delete 0xb8c
    pz::g_Campaign = new pz::SPanzersCampaign();                  // new 0xb8c, 0x590ec0
    int sel = List.CurSel;                                        // +0xc8
    if (sel < 0 || sel >= Files.size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SString", sel);
    pz::g_Campaign->MapName = Files.array[sel];                   // 0x52c2c0 on campaign +0x20
    SendAction(PZA_SCENARIO_START, sel);                          // 0x543930(0x53431, +0xc8)
    return true;
}

// PANZERS 0x63b2d0
// The New Game menu's Scenario button: the old scenario menu (if any) goes,
// a new one becomes a child of this menu's parent (over this menu).
void SSingleMenu::LoadScenarioMenu()
{
    PZ_M3_TRACE("SSingleMenu::LoadScenarioMenu (0x63b2d0)");
    if (ScenarioMenu) {                                           // +0x58 vtbl +0 (1)
        delete ScenarioMenu;
        ScenarioMenu = nullptr;
    }
    SScenarioMenu* m = new SScenarioMenu();                       // new 0x20c
    ScenarioMenu = m;
    Parent->InsertChild(m);                                       // (+0x24)->vtbl +0x54
    m->Create();                                                  // 0x639000
}

// ---------------------------------------------------------------------------
// The super window side
// ---------------------------------------------------------------------------

// HD (both callers): campaign +0x18 = World->Players[World->LocalPlayer]
// +0x04 (World +0x174 + LocalPlayer * 0x48), after the map is loaded.
static void RaceFromLocalPlayer()
{
    if (!pz::g_World || !pz::g_Campaign)
        return;
    const unsigned char* w = (const unsigned char*)pz::g_World;
    int local = *(const int*)(w + 0x16c);
    pz::g_Campaign->Race = *(const int*)(w + 0x174 + local * 0x48);
    Logger.g->Log(0, "PZM5: scenario %s, local player %d, race %d",
                  pz::g_Campaign->MapName.buf ? pz::g_Campaign->MapName.buf : "", local, pz::g_Campaign->Race);
}

// PANZERS 0x659250 (case 0x53431, 0x659e9d..0x659f89)
// The scenario menu's Load: the map name is copied out of the menu's
// campaign, the main menu (and with it the New Game and scenario menus)
// goes, a new campaign in scenario mode, LoadNextCampaignView (2: game
// view), the race of the map's local player.
bool PzScenarioAction(SSuperWindow* sw, int action, int param)
{
    (void)param;
    if (action != PZA_SCENARIO_START)
        return false;
    PZ_M3_TRACE("SSuperWindow::OnAction Scenario (0x659250 / 0x53431)");
    SString map;
    map = pz::g_Campaign->MapName;                                // 0x52bf00 (copy of campaign +0x20)
    if (sw->MainMenu) {                                           // +0xe8 vtbl +0 (1)
        delete sw->MainMenu;
        sw->MainMenu = nullptr;
    }
    sw->Cursor = 0;                                               // 0x543970(0, -1)
    delete pz::g_Campaign;                                        // 0x591350 + delete 0xb8c
    pz::g_Campaign = new pz::SPanzersCampaign();                  // new 0xb8c, 0x590ec0
    pz::g_Campaign->InitScenarioMode(map.buf ? map.buf : "", "", 0, 0);   // 0x5944d0
    M3LoadNextCampaignView(sw);                                   // 0x658b10
    RaceFromLocalPlayer();
    FreeStr(&map);                                                // 0x51e0f0
    return true;
}

// PANZERS 0x65b470 (the map branch, 0x65b778..0x65b814) with 0x6585e0
// HD Play: Initialize (the caller), then without SMulti (DAT_008f1a74; the
// recompile never makes one here) a new campaign in scenario mode (map, "",
// 0, 0) and 0x6585e0: UnloadMenuBackground 0x65b940, new SGameView 0x3e98
// (0x6181f0) into +0xe4, vtbl +0x54, size 0, 0, 0x400, 0x300, Create
// 0x619c90, +0xd8 = 1, LoadMap 0x6201c0 -- LoadNextCampaignView case 2 does
// the same, so it is reused. Then the race of the map's local player.
//
// The game-view actions ("Map loaded", GV_GAMEOVER, results) live in the
// -m3 action path (superwindow_m3.cpp), so a map start turns it on
// (recompile only; the -m3 switch is the recompile's own gate).
void PzStartMapFromCommandLine(SSuperWindow* sw, const char* map)
{
    PZ_M3_TRACE("SSuperWindow::Play map from the command line (0x65b470)");
    if (!pz::g_M3.Enabled) {
        Logger.g->Log(0, "PZM5: -map %s: the game-view actions need the -m3 path; turning it on", map);
        pz::g_M3.Enabled = true;
    }
    // HD bug: Initialize loaded the menu world (0x658690(1): maps/menu.map,
    // its SWorld +0x19c and SGameLogic +0x1a0) but made no menu frames, so
    // 0x6585e0's UnloadMenuBackground (0x65b940, which tests the top frame
    // +0x194) keeps both. The mission's SWorld / SGameLogic ctors then delete
    // them as the previous g_World / g_GameLogic (0x5d2f90, 0x55e440) and
    // OnIdle 0x65ae50 keeps refreshing the freed menu logic: a use after
    // free (our build: access violation in SGameLogic::Refresh on the first
    // frame; HD: "SHeap<struct SFrame>::operator[]: invalid index",
    // docs/M3_REPLAY.md). The faithful build keeps it; the fix (mod switch,
    // or the recompile-only test hook PZ_M5_MAPFIX=1) unloads the menu world
    // first, as UnloadMenuBackground does on every other path.
#if PANZERS_MOD_BUGFIXES
    const bool fix = true;
#else
    const bool fix = getenv("PZ_M5_MAPFIX") != nullptr;
#endif
    if (fix && (sw->MenuGameLogic || sw->MenuWorld)) {
        Logger.g->Log(0, "PZM5: -map: unloading the menu world first (HD keeps it)");
        delete sw->MenuGameLogic;                                 // 0x55fe00 + delete 0x318
        sw->MenuGameLogic = nullptr;
        delete sw->MenuWorld;                                     // vtbl +0 (0x5d68b0)
        sw->MenuWorld = nullptr;
    }
    delete pz::g_Campaign;                                        // (recompile) none exists here
    pz::g_Campaign = new pz::SPanzersCampaign();                  // new 0xb8c, 0x590ec0
    pz::g_Campaign->InitScenarioMode(map, "", 0, 0);              // 0x5944d0
    M3LoadNextCampaignView(sw);                                   // 0x6585e0(map)
    RaceFromLocalPlayer();
    if (fix) {
        // (recompile) The window queued a WM_SIZE while the map loaded
        // (no message loop yet). Taken after the first mission frame, our
        // renderer's device Reset fails (D3DERR_INVALIDCALL: a default-pool
        // resource of the mission scene is not released), and every later
        // texture / vertex buffer creation panics. The menu paths handle it
        // between the load and the first frame; do the same here.
        MSG msg;
        int n = 0;
        while (n < 256 && PeekMessageA(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                PostQuitMessage((int)msg.wParam);
                break;
            }
            TranslateMessage(&msg);
            DispatchMessageA(&msg);
            ++n;
        }
        Logger.g->Log(0, "PZM5: -map: %d pending window messages handled before the first frame", n);
    }
}
