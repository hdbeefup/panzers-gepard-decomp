// src/panzers/loadgame.cpp
// Loading a saved game: SSuperWindow::OnAction case 0x494c1 (0x659250) and
// SGameView's load-game LoadMap 0x61f840, plus the recompile's test hooks
// (PZ_M4_LOADGAME, PZ_M4_SAVE_AT; docs/M4_STATUS.md "Save / load").
// OWNER: agent S (M4).

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "superwindow.h"
#include "mainmenu.h"
#include "gameview.h"
#include "campaign.h"
#include "settings.h"
#include "pzboard.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "timer.h"
#include "stream.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "doodad.h"
#include "world_save.h"
#include "loadgame.h"
#include "ingamemenu.h"

void M3LoadNextCampaignView(SSuperWindow* sw);   // superwindow_m3.cpp 0x658b10

static void LgFocusWidget(SWidget* widget)
{
    // HD 0x5439f0 (as superwindow_m3.cpp)
    for (SWidget* w = widget; w && w->Enabled && w->Visible && w->Parent; w = w->Parent) {
        if (w->Parent->Focus != w) {
            for (SWidget* c = w->Parent->Child; c; c = c->Sibling)
                if (c->FocusSibling == w)
                    c->FocusSibling = nullptr;
            w->FocusSibling = w->Parent->Focus;
            w->Parent->Focus = w;
        }
    }
}

// PANZERS 0x5955d0
// The save type of a file (1 = a game saved in a mission, 2 = between
// missions, 0 = not a save) and its map name.
// The path of a save: the Load Game action passes the name in SaveGames/
// ("quick.save"); a "SaveGames/..." path is taken as it is.
static void SavePath(char* out, size_t cap, const char* file)
{
    if (_strnicmp(file, "SaveGames/", 10) == 0 || _strnicmp(file, "SaveGames\\", 10) == 0)
        _snprintf(out, cap - 1, "%s", file);
    else
        _snprintf(out, cap - 1, "SaveGames/%s", file);           // 0x52c580("SaveGames/" + name)
    out[cap - 1] = 0;
}

int PzSaveGameType(const char* file, SString* map)
{
    return pz::ReadSaveGameType(file, map);                       // campaign_save.cpp
}

// PANZERS 0x61f840
// The load-game variant of LoadMap. When the save is on the map that is
// loaded, the world and the logic stay (LoadGameState removes the units);
// otherwise a new world loads the map as LoadMap 0x6201c0 does and a new
// SGameLogic is made. Either way a new campaign reads the save (LoadGame
// 0x594f70), then the mission runs as after MissionStart.
void SGameView::LoadGame(const char* file)
{
    PZ_M3_TRACE("SGameView::LoadGame (0x61f840)");
    PzGameViewCloseDialogs(this);                                 // +0x3e48..+0x3e6c (ingamemenu.cpp)
    // HD: the open dialogs +0x3e48..+0x3e6c are deleted, the panel widgets
    // (+0xc68, 9 x +0x830, +0x5dc, +0x75c) hidden, board frame +0x5d8 hidden.
    if (Board && _5d8 >= 0)
        Board->ShowFrame(_5d8, false);                            // board +0x18(+0x5d8, 0)
    SString oldMap;
    if (pz::g_Campaign) {
        oldMap = pz::g_Campaign->GetMapName();                    // 0x592040
        delete pz::g_Campaign;                                    // 0x591350 + delete 0xb8c
        pz::g_Campaign = nullptr;
    }
    SString map;
    PzSaveGameType(file, &map);                                   // 0x5955d0
    bool sameMap = World && Logic && oldMap.size == map.size && (map.size == 0 || _stricmp(pz::SStr(oldMap), pz::SStr(map)) == 0);
    if (sameMap) {
        ShowLoadingBackdrop(true);                                // 0x61f460(1)
        pz::g_Campaign = new pz::SPanzersCampaign();              // new 0xb8c, 0x590ec0
        pz::g_Campaign->LoadGame(file);                           // 0x594f70
    } else {
        ShowLoadingBackdrop(false);                               // 0x61f460(0)
        if (Logic) {
            delete Logic;                                         // 0x55fe00 + delete 0x318
            Logic = nullptr;
        }
        if (World) {
            delete World;                                         // vtbl +0 (1)
            World = nullptr;
        }
        // HD: renderer +0x3c(0) +0x50(0, 0): the loading viewport.
        pz::g_Campaign = new pz::SPanzersCampaign();              // new 0xb8c, 0x590ec0
        World = new pz::SWorld(0);                                // new 0x7538, 0x5d2f90(0)
        World->ShowLoadingIcon(GetFrame());                       // 0x5edca0(+0x48)
        SStream* stream = FileSystem.OpenRead(pz::SStr(map), nullptr);   // 0x65f420(map, 0)
        if (!stream || !World->LoadMap(stream, true, 0, 0))       // 0x5f1990(stream, 1, 0, 0)
            Logger.g->Panic("Can't load map: %s", pz::SStr(map));
        World->Initialize();                                      // 0x5eec90
        World->LoadMapExtra_5e2d70();
        World->LoadMapExtra_5debb0();
        World->LoadMapExtra_607ad0();
        stream->Release();
        unsigned now = NowMs();                                   // ftol(0x661800() * 1000.0)
        ClockNextTick = now;                                      // +0x460
        ClockStart = now;                                         // +0x45c
        World->UpdateWaterMap();                                  // 0x608600
        Logic = new pz::SGameLogic(0, LogicFrame828, (int)(size_t)static_cast<SIGameViewCallback*>(this));   // new 0x318, 0x55e440
        Logic->SetBoardArea(_4b8, 0x13c, 7, 7);                   // 0x57fac0
        // Recompile-only PZ_M4_FIXFIRST=1: the bridges are fixed before the
        // units' after-load slots, so the units on platforms get their
        // height as in the saved game (HD fixes them only after the load,
        // which leaves those heights stale until the next tick).
        if (getenv("PZ_M4_FIXFIRST"))
            World->FixBridges();                                  // 0x5e65f0
        pz::g_Campaign->LoadGame(file);                           // 0x594f70
        World->StartEffects();                                    // 0x5f5b50
    }
    // LAB_0061fcf6
    Logic->_308 = Logic->Frame;                                   // 0x57fab0
    if (!World->UseAltHeights)
        World->FixBridges();                                      // 0x5e65f0
    Logic->BuildVisHeights();                                     // 0x576a70
    Logic->SetVisOverlayMode(Settings.FogOfWarView);              // 0x57f970(0x64df00())
    World->_4d0[0] = 0;                                           // World+0x4d0
    // HD 0x609760(0x929d38): the -csplay cut-scene file (or "").
    if (pz::g_Campaign->GetMissionResult() == 0)                  // 0x5920b0
        Logic->SetRunning(1);                                     // 0x5802f0(1)
    World->HideLoadingIcon();                                     // 0x5dc7d0
    ReleaseLoadingBackdrop();                                     // board +0x0c(+0x3898), Concert +0x80(+0x3894)
    // HD 0x626290: the tool-tip texts of the hot keys.
    *(int*)((unsigned char*)World + 0x73ac) = Settings.UnitVoice; // 0x64e2b0
    SetPanelMode(0);                                              // 0x625d80(0)
    pz::FreeSString(&map);
    pz::FreeSString(&oldMap);
    PzM4AfterLoad(this, file);
}

// PANZERS 0x659250 (case 0x494c1, a type-1 save)
// No game view yet: the main menu (+0xe8) and the other menu (+0x114) are
// deleted, the menu background unloaded, and a new game view made with its
// sub-viewports. Then the load-game LoadMap, and the window scene becomes
// the world's.
bool PzLoadGameAction(SSuperWindow* sw, const char* file)
{
    PZ_M3_TRACE("SSuperWindow::OnAction Load Game (0x659250 / 0x494c1)");
    int type = PzSaveGameType(file, nullptr);                     // 0x5955d0
    // HD Concert +0x80(1): not mapped.
    if (!sw->GameView) {
        if (sw->MainMenu) {
            delete sw->MainMenu;                                  // +0xe8
            sw->MainMenu = nullptr;
        }
        sw->UnloadMenuBackground();                               // 0x65b940
    }
    if (type == 2) {
        // A "Before" save (SaveGameBefore 0x596b30): the campaign starts the
        // mission from its beginning with the carried army. HD: +0xd8 = 0.
        if (sw->GameView) {
            delete sw->GameView;                                  // +0xe4
            sw->GameView = nullptr;
        }
        if (pz::g_WindowScene) {                                  // SDXWindow +0xe0 vtbl +4
            pz::g_WindowScene->Release();
            pz::g_WindowScene = nullptr;
        }
        delete pz::g_Campaign;                                    // 0x591350 + delete 0xb8c
        pz::g_Campaign = new pz::SPanzersCampaign();              // new 0xb8c, 0x590ec0
        pz::CampaignLoadGameBefore(pz::g_Campaign, file);         // 0x595330
        M3LoadNextCampaignView(sw);                               // 0x658b10 (focuses the new view)
        return true;
    }
    if (type != 1) {
        // HD panics here; the recompile warns and stays in the menu.
        Logger.g->Warning("SSuperWindow::OnAction - unknow CAMPAIGN_SAVEGAMETYPE (%s)", file);
        return false;
    }
    if (!sw->GameView) {
        SGameView* v = new SGameView();                           // new 0x3e98, 0x6181f0
        sw->GameView = v;                                         // +0xe4
        sw->InsertChild(v);                                       // vtbl +0x54
        v->SetPosition(0, 0, 0x400, 0x300);                       // vtbl +0x08
        v->Create();                                              // 0x619c90
        v->CreateSubViewports();                                  // 0x61e500
        // HD: SDXWindow +0xd8 = 1.
    }
    SGameView* v = static_cast<SGameView*>(sw->GameView);
    v->LoadGame(file);                                            // 0x61f840
    if (pz::g_WindowScene)
        pz::g_WindowScene->Release();                             // window +0xe0 vtbl +4
    if (pz::g_Scene)
        pz::g_Scene->AddRef();
    pz::g_WindowScene = pz::g_Scene;
    LgFocusWidget(v);                                             // 0x51e0f0
    return true;
}

// ---------------------------------------------------------------------------
// Quick save / quick load (SGameView::OnKeyDown 0x622f50, F6 / F9)

// PANZERS 0x622f50 (case VK_F6)
// Single player only (HD also needs view +0x9c == 0): "Quicksaving..." on
// the logic's message line (0x56a480, type 2), the frame is presented, then
// SaveGames/quick.save with the title "Quick - <name>" (SCampaign::GetName
// 0x5925d0), then "...done" or "...failed".
bool PzQuickSave(SGameView* v)
{
    PZ_M3_TRACE("SGameView::OnKeyDown F6 quicksave (0x622f50)");
    if (!pz::g_Campaign || !v->Logic)
        return false;
    char title[600];
    _snprintf(title, sizeof(title) - 1, "Quick - %s", pz::CampaignGetName(pz::g_Campaign));   // 0x660c50("Quick - "), 0x5925d0
    title[sizeof(title) - 1] = 0;
    Logger.g->Log(0, "Quicksaving...");                           // 0x56a480 (message line not mapped)
    bool ok = pz::g_Campaign->SaveGame("SaveGames/quick.save", title);   // 0x5966a0
    Logger.g->Log(0, "%s", ok ? "...done" : "...failed");
    return ok;
}

// PANZERS 0x622f50 (case VK_F9)
// When SaveGames/quick.save is a save (0x594b20), the Load Game action
// 0x494c1 with "quick.save" (0x929bf0) goes to the super window; else
// "Quickload failed.".
bool PzQuickLoad(SGameView* v)
{
    PZ_M3_TRACE("SGameView::OnKeyDown F9 quickload (0x622f50)");
    unsigned t0 = SGameView::NowMs();
    if (PzSaveGameType("quick.save", nullptr) != 1) {             // 0x594b20
        Logger.g->Log(0, "Quickload failed.");
        return false;
    }
    static char s_File[64];                                       // DAT_00929bf0
    strcpy(s_File, "quick.save");
    v->SendAction(0x494c1, (int)(size_t)s_File);                  // 0x543930
    Logger.g->Log(0, "Game loaded... (%f sec)", (double)(SGameView::NowMs() - t0) / 1000.0);
    return true;
}


// ---------------------------------------------------------------------------
// Recompile-only test hooks (docs/M4_STATUS.md).
//
// PZ_M4_LOADGAME=<file>: the first main menu loads SaveGames/<file> through
// the Load Game action (as the Load Game screen does).
// PZ_M4_SAVE_AT=<frame>: at that logic frame (BeginFrame, after the CRC
// line) the game is saved to SaveGames/M4RT-<frame>.save, with a sidecar
// (.rt) that holds what a replay needs to go on after a load: the position
// in the -packetplay file and the logic's CRC queue (neither is part of a
// save). With PZ_M4_RT=1, loading such a save under -packetplay reopens the
// replay there.

void M4OnMainMenu(SSuperWindow* sw)
{
    static bool s_Done = false;
    const char* f = getenv("PZ_M4_LOADGAME");
    if (s_Done || !f || !*f)
        return;
    s_Done = true;
    Logger.g->Log(0, "PZM4: Load Game (0x494c1) %s", f);
    static char s_File[260];
    _snprintf(s_File, sizeof(s_File) - 1, "%s", f);
    PzLoadGameAction(sw, s_File);
}

void PzM4AfterLoad(SGameView* v, const char* file)
{
    pz::SGameLogic* g = v->Logic;
    Logger.g->Log(0, "PZM4: loaded %s: frame %d, %d units, seed %08x", file, g ? g->Frame : -1,
                  pz::g_World ? pz::g_World->Units.Count : 0, pz::g_World ? pz::g_World->RandomSeed : 0u);
    if (getenv("PZ_M4_VISION") && g && pz::g_World) {
        // Recompile-only test switch: build every player's vision map now
        // (HD rebuilds one player per tick, 0x565e10 by table 0x7f6220).
        for (int pl = 0; pl < 12; ++pl)
            if (*(int*)(pz::g_World->Players[pl] + 0x08) != 2)
                g->Tick_565e10(pl);
    }
    if (!getenv("PZ_M4_RT") || !g || !Settings.PacketPlay || Settings.PacketFile.size == 0)
        return;
    char path[400];
    SavePath(path, sizeof(path), file);
    char side[420];
    _snprintf(side, sizeof(side) - 1, "%s.rt", path);
    side[sizeof(side) - 1] = 0;
    FILE* fp = fopen(side, "rb");
    if (!fp)
        return;
    int pos = -1, n = 0;
    fread(&pos, 4, 1, fp);
    fread(&n, 4, 1, fp);
    free(g->CrcHistory);
    g->CrcHistory = (unsigned*)malloc(sizeof(unsigned) * (n > 0x10 ? n : 0x10));
    g->CrcMax = n > 0x10 ? n : 0x10;
    g->CrcCount = n;
    g->CrcBase = 0;
    g->CrcBottom = 0;
    g->CrcTop = n - 1;
    for (int i = 0; i < n; ++i)
        fread(&g->CrcHistory[i], 4, 1, fp);
    fread(&g->FramesSent, 4, 1, fp);
    fclose(fp);
    if (pos >= 0) {
        g->PlaybackStream = FileSystem.OpenRead(pz::SStr(Settings.PacketFile), "PZM4 replay");
        if (g->PlaybackStream)
            g->PlaybackStream->Seek(pos, 0);
    }
    Logger.g->Log(0, "PZM4: replay %s continues at byte %d, CRC queue %d", pz::SStr(Settings.PacketFile), pos, n);
}
