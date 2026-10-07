// src/panzers/superwindow_sk.cpp
// The skirmish part of SSuperWindow (HD OnAction 0x659250 cases 0x534d1,
// 0x534b1..0x534b4, LoadSkirmishChatRoomView 0x658e70), taken with -m3
// (called first by SuperWindowM3Action). OWNER: agent SK (M5, docs/m5/sk.md).

#include <windows.h>
#include "mods.h"
#include "superwindow.h"
#include "mainmenu.h"
#include "skirmish.h"
#include "campaign.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "pz/iscene.h"
#include "worldapi.h"
#include "results.h"
#include "pzboard.h"
#include "board.h"
#include "gettext.h"
#include "market.h"

void M3LoadNextCampaignView(SSuperWindow* sw);                    // superwindow_m3.cpp (0x658b10)

// HD 0x5439f0: the widget and its parents become the focused children.
static void FocusWidget(SWidget* widget)
{
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

static void DeleteRoom(SSuperWindow* sw)
{
    if (sw->Menu_108) {                                            // +0x108 vtbl +0(1)
        delete sw->Menu_108;
        sw->Menu_108 = nullptr;
    }
}

static void DeleteSkirmish()
{
    if (g_Skirmish) {                                              // SMulti vtbl +0x30(1)
        delete g_Skirmish;
        g_Skirmish = nullptr;
    }
}

// PANZERS 0x658e70
// SMulti::Server 0x51e950 opens no socket offline (PzSkirmish).
static void LoadSkirmishChatRoomView(SSuperWindow* sw)
{
    PZ_M3_TRACE("SSuperWindow::LoadSkirmishChatRoomView (0x658e70)");
    if (!g_Skirmish)
        g_Skirmish = new PzSkirmish();                             // new 0x5130, 0x51dcf0, Server 0x51e950("", 0, 0)
    SSkirmishChatRoomMenu* room = new SSkirmishChatRoomMenu();     // new 0x1244, 0x652d20
    sw->Menu_108 = room;                                           // +0x108
    sw->InsertChild(room);                                         // vtbl +0x54
    room->SetPosition(0, 0, 0x400, 0x300);                         // vtbl +0x08
    room->Create();                                                // 0x653250
    FocusWidget(room);
    // HD: SDXWindow +0xd8 = 1.
    Logger.g->Log(0, "SSuperWindow::LoadSkirmishChatRoomView: releasing scene");
    if (pz::g_WindowScene) {                                       // SDXWindow +0xe0
        pz::g_WindowScene->Release();
        pz::g_WindowScene = nullptr;
    }
}


// SResultsMenu::Create 0x6360f0, multi mode and not coop (0x637474 skips the
// objectives; 0x638fa7 skips the officer and medal sheet): the unit table
// (headers at y 0xf8, 8 rows from y 0x111, the player names of the
// PlayerStats records) and only Continue at (0x300, 0x2d2).
static int ResultsText(int parent, int x, int y, int align, const char* text)
{
    int f = Board->CreateFrame(FT_TEXT, parent, x, y, 0, 1);      // board +0x08(2, P, x, y, 0, 1)
    Board->SetText(f, g_PzFont[1], align, text ? text : "");      // board +0x34(f, 1, align, s, 0)
    return f;
}

void PzSkirmishResults(SResultsMenu* r)
{
    int P = r->GetFrame();
    static const int kHx[6] = { 0xff, 0x172, 0x1e5, 0x267, 0x2e4, 0x35c };
    static const char* const kHead[6] = { "Total units", "Destroyed units", "Lost units", "Captured units ", "XP", "Accomplishment" };
    for (int i = 0; i < 6; ++i)
        ResultsText(P, kHx[i], 0xf8, 2, GetText("panzers/MainMenu.cpp", kHead[i]));
    static const int kGx[7] = { 0xa3, 0xff, 0x172, 0x1e5, 0x267, 0x2e4, 0x35c };
    for (int row = 0; row < 8; ++row)
        for (int k = 0; k < 7; ++k)
            r->Grid[row][k] = ResultsText(P, kGx[k], 0x111 + 0x19 * row, 2, "");
    r->ShowPage(0);                                                // 0x63e690(0)
    r->InsertChild(&r->Continue);                                  // +0x71c
    r->Continue.SetPosition(0x300, 0x2d2, 0, 0);
    r->Continue.Create(2, GetText("panzers/MainMenu.cpp", "Continue"));
}

// BackupCampaignUnits 0x561110, the multi part (not coop): the results name
// of every player: "COMPUTER" for a Computer slot, else the slot's name.
// The recompile fills them when the game view ends, before PzSkirmish goes.
static void SetPlayerStatNames()
{
    if (!g_Skirmish || !pz::g_Campaign)
        return;
    for (int p = 0; p < 8; ++p) {
        SString* name = (SString*)pz::g_Campaign->PlayerStats[p];  // campaign +0x140 + p * 0xd8
        const PzSkirmishSlot& s = g_Skirmish->Slots[p];
        const char* text = s.Status == 3 ? GetText("world/GameLogic.cpp", "COMPUTER") : s.Name;
        *name = text ? text : "";
    }
}
bool PzSkirmishAction(SSuperWindow* sw, int action, int param)
{
    (void)param;
    switch (action) {
    case 0x534d1: {                                                // SSingleMenu Skirmish
        PZ_M3_TRACE("SSuperWindow::OnAction Skirmish (0x659250 / 0x534d1)");
        if (sw->MainMenu) {                                        // +0xe8
            delete sw->MainMenu;
            sw->MainMenu = nullptr;
        }
        DeleteSkirmish();
        delete pz::g_Campaign;                                     // 0x591350 + delete 0xb8c
        pz::g_Campaign = new pz::SPanzersCampaign();               // new 0xb8c, 0x590ec0
        pz::g_Campaign->InitMultiMode();                           // 0x593b70
        LoadSkirmishChatRoomView(sw);                              // 0x658e70
        return true;
    }
    case PZA_SKIRMISH_START:                                       // 0x534b1 (the room's countdown)
        PZ_M3_TRACE("SSuperWindow::OnAction Skirmish start (0x659250 / 0x534b1)");
        DeleteRoom(sw);
        pz::g_Campaign->LetChatroomDone();                         // 0x594dc0
        Logger.g->Log(0, "PZM5: skirmish %s, game type %d, prestige %d, age %d",
                      g_Skirmish ? g_Skirmish->MapPath : "", g_Skirmish ? g_Skirmish->GameType : -1,
                      g_Skirmish ? g_Skirmish->PrestigeLimit : 0, g_Skirmish ? g_Skirmish->GameAge : 0);
        M3LoadNextCampaignView(sw);                                // 0x658b10 (2: the game view, LoadMap)
        return true;
    case PZA_SKIRMISH_CANCEL:                                      // 0x534b2
        sw->ReleaseMultiView();                                    // 0x65b8c0
        DeleteRoom(sw);
        delete pz::g_Campaign;                                     // (LoadMainMenu 0x6583e0 deletes the campaign)
        pz::g_Campaign = nullptr;
        sw->LoadMainMenu();                                        // 0x6583e0
        DeleteSkirmish();
        return true;
    case PZA_SKIRMISH_NEW: {                                       // 0x534b3 New army
        // The room hides; the campaign's army and its names are cleared
        // (0x591d50); the race of the local slot; LoadNextCampaignView:
        // MenuToLoad is still 1 (InitMultiMode), so the market (case 1).
        SSkirmishChatRoomMenu* room = static_cast<SSkirmishChatRoomMenu*>(sw->Menu_108);
        room->SetVisible(false);                                   // +0x108 vtbl +0x6c(0)
        pz::g_Campaign->ClearArmy();                               // 0x591d50
        pz::g_Campaign->SetRace(room->Race);                       // 0x5974b0(+0x123c)
        M3LoadNextCampaignView(sw);                                // 0x658b10
        return true;
    }
    case PZA_SKIRMISH_EDIT: {                                      // 0x534b4 Edit army
        // As New, but the campaign keeps the army LoadCurrentArmy bought.
        SSkirmishChatRoomMenu* room = static_cast<SSkirmishChatRoomMenu*>(sw->Menu_108);
        room->SetVisible(false);
        pz::g_Campaign->SetRace(room->Race);                       // 0x5974b0(+0x123c)
        M3LoadNextCampaignView(sw);                                // 0x658b10
        return true;
    }
    case PZA_MARKET_START:                                         // 0x4d542 market saved the army
    case PZA_MARKET_CANCEL: {                                      // 0x4d541 market left without saving
        // HD (0x659250): ReleaseMultiView; with the multiplayer room
        // (+0x104, not lifted) ...; with the skirmish room (+0x108): +0x538
        // = 1 (the army list on), room visible, and after a save the army
        // list again (LoadArmyNames 0x654030). Without a room: single player.
        SSkirmishChatRoomMenu* room = static_cast<SSkirmishChatRoomMenu*>(sw->Menu_108);
        if (!room)
            return false;
        sw->ReleaseMultiView();                                    // 0x65b8c0
        room->ArmiesEnabled = true;                                // +0x538 = 1
        // HD bug kept: New / Edit left the local slot "in the market" (ready
        // 2, 0x51e270(1)); unlike the multiplayer room's branch, this one
        // does not reset it (0x51e270(0)), so the first I'm Ready click only
        // toggles 2 -> 0 (0x51e2b0) and a second one is needed.
#if PANZERS_MOD_BUGFIXES
        if (g_Skirmish)
            g_Skirmish->SetNotReady(false);                        // MOD_BUGFIXES: 0x51e270(0) as the multiplayer room
#endif
        room->SetVisible(true);                                    // vtbl +0x6c(1)
        if (action == PZA_MARKET_START)
            room->LoadArmyNames();                                 // 0x654030
        return true;
    }
    case 0x47562:                                                  // GV_GAMEOVER / GV_ERROR: HD deletes
    case 0x47563:                                                  // SMulti before LetMapDone; the rest is SuperWindowM3Action's
        SetPlayerStatNames();
        DeleteSkirmish();
        return false;
    case 0x524d1:                                                  // results Continue (PZA_RESULTS_CONTINUE)
        if (pz::g_Campaign && pz::g_Campaign->GameMode == 4 && sw->Menu_114) {
            delete sw->Menu_114;                                   // +0x114
            sw->Menu_114 = nullptr;
            pz::g_Campaign->LetResultsDone();                      // 0x594e70: mode 4, no SMulti -> 4
            if (pz::g_Campaign->MenuToLoad == pz::PZ_MENU_MULTI) {
                // HD LoadNextCampaignView case 4: the multiplayer pre-menu
                // 0x658a30 (or the RankedGaming lobby 0x658230).
                STUB_LOG("SSuperWindow::LoadNextCampaignView 4 after a skirmish: SMultiPreMenu (0x658a30) not lifted, main menu");
                delete pz::g_Campaign;
                pz::g_Campaign = nullptr;
                sw->LoadMainMenu();                                // 0x6583e0
                return true;
            }
            M3LoadNextCampaignView(sw);
            return true;
        }
        return false;
    default:
        return false;
    }
}
