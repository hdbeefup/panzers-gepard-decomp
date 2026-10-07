// src/panzers/superwindow_m3.cpp
// The M3 part of SSuperWindow (HD OnAction 0x659250 campaign cases and
// LoadNextCampaignView 0x658b10), taken only with -m3 / PZ_M3=1. Without
// the switch SSuperWindow::OnAction keeps the pre-M3 behaviour (the Training
// Camp button is a logged stub). OWNER: agent F (docs/M3_INTERFACES.md).
//
// Training Camp round trip (HD action codes):
//   0x4d4d4 Training Camp   -> STrainingMenu into +0x118
//   0x544d1 Start           -> SPanzersCampaign (race, InitTutorialMode
//                              maps/training.map, 1500), delete the training
//                              and main menus, LoadNextCampaignView (2: game view)
//   0x544d2 Cancel          -> delete the training menu
//   0x47561 Map loaded      -> OnMapLoaded: 1 (market) -> LoadNextCampaignView;
//                              2 -> SetMenuGameView, view visible, MissionStart
//   0x4d542 market Start    -> ReleaseMultiView, SetMenuGameView, view visible, MissionStart
//   0x4d541 market Cancel   -> ReleaseMultiView, delete the view, LoadMainMenu
//   0x47562/3 GV_GAMEOVER   -> delete the view, release the scene, LetMapDone,
//                              LoadNextCampaignView (training: the main menu)

#include <windows.h>
#include "superwindow.h"
#include "mainmenu.h"
#include "trainingmenu.h"
#include "campaignmenu.h"
#include "briefing.h"
#include "results.h"
#include "milesconcert.h"
#include "market.h"
#include "gameview.h"
#include "campaign.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "iconcert.h"
#include "pz/iscene.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "settings.h"
#include <string.h>

// HD 0x929f18 / 0x929f1c / 0x929f20: the mission world, logic and scene
// while the market is up (LoadNextCampaignView case 1 -> ReleaseMultiView).
static pz::SWorld*     s_MovedWorld = nullptr;
static pz::SGameLogic* s_MovedLogic = nullptr;
static pz::SIScene*    s_MovedScene = nullptr;

// Part of SSuperWindow::ReleaseMultiView 0x65b8c0: move the mission world
// back after the market is deleted. No-op on the default path.
void M3RestoreMovedWorld()
{
    if (!s_MovedWorld && !s_MovedLogic && !s_MovedScene)
        return;
    pz::g_World = s_MovedWorld;
    pz::g_GameLogic = s_MovedLogic;
    pz::g_Scene = s_MovedScene;
    s_MovedWorld = nullptr;
    s_MovedLogic = nullptr;
    s_MovedScene = nullptr;
}

static SGameView* View(SSuperWindow* sw) { return static_cast<SGameView*>(sw->GameView); }

// HD 0x5439f0 (as in SMainMenu::Create): walk up the parents making each
// widget the focused child, so keys reach it.
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

// Concert +0x80(1): the music stream stops (HD LoadNextCampaignView cases
// 0 and 2, results Cancel).
static void StopMenuMusic()
{
    if (SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert))
        pc->StopStream(true);
}

static void DeleteWidget(SWidget*& w)
{
    if (w) {
        delete w;
        w = nullptr;
    }
}

// The main menu after a campaign screen: HD LoadMainMenu 0x6583e0 also
// deletes the campaign (DAT_00929a0c).
static void CampaignToMainMenu(SSuperWindow* sw)
{
    delete pz::g_Campaign;
    pz::g_Campaign = nullptr;
    sw->LoadMainMenu();                                            // 0x6583e0
}

static void DeleteGameView(SSuperWindow* sw)
{
    if (sw->GameView) {
        delete sw->GameView;
        sw->GameView = nullptr;
    }
}

static void ReleaseWindowScene()
{
    Logger.g->Log(0, "SSuperWindow::OnAction: releasing scene");
    if (pz::g_WindowScene) {                                       // SDXWindow +0xe0
        pz::g_WindowScene->Release();
        pz::g_WindowScene = nullptr;
    }
}

// SSuperWindow::LoadNextCampaignView 0x658b10 (cases 1, 2 and the default;
// 0 briefing, 3 results and 4 multiplayer are logged stubs).
void M3LoadNextCampaignView(SSuperWindow* sw)
{
    PZ_M3_TRACE("SSuperWindow::LoadNextCampaignView (0x658b10)");
    if (!pz::g_Campaign)
        Logger.g->Panic("SSuperWindow::LoadNextCampaignView: Campaign unitialized");
    int menu = pz::g_Campaign->GetMenuToLoad();
    Logger.g->Log(0, "PZM3: LoadNextCampaignView MenuToLoad %d", menu);
    switch (menu) {
    case pz::PZ_MENU_MARKET: {
        sw->UnloadMenuBackground();                                // 0x65b940
        s_MovedWorld = pz::g_World;                                // 0x929f18
        s_MovedLogic = pz::g_GameLogic;                            // 0x929f1c
        s_MovedScene = pz::g_Scene;                                // 0x929f20
        pz::g_World = nullptr;
        pz::g_GameLogic = nullptr;
        pz::g_Scene = nullptr;
        if (sw->GameView)
            sw->GameView->SetVisible(false);                       // (recompile) HD shows it again at Start
        SMarket* m = new SMarket();                                // new 0x25b4, 0x63f2f0
        sw->MultiView = m;                                         // +0x110
        sw->InsertChild(m);                                        // vtbl +0x54
        m->SetPosition(0, 0, 0x400, 0x300);                        // vtbl +0x08
        m->Create();                                               // 0x6407d0
        FocusWidget(m);
        // HD: +0xd8 = 1; window scene = g_Scene (the market preview scene).
        if (pz::g_WindowScene)
            pz::g_WindowScene->Release();                          // scene +0x04
        if (pz::g_Scene)
            pz::g_Scene->AddRef();                                 // scene +0x00
        pz::g_WindowScene = pz::g_Scene;                           // SDXWindow +0xe0
        return;
    }
    case pz::PZ_MENU_GAMEVIEW: {
        sw->UnloadMenuBackground();                                // 0x65b940
        // HD Concert +0x80(1) (stops the menu music): not mapped yet.
        DeleteGameView(sw);
        SGameView* v = new SGameView();                            // new 0x3e98, 0x6181f0
        sw->GameView = v;                                          // +0xe4
        sw->InsertChild(v);
        v->SetPosition(0, 0, 0x400, 0x300);
        v->Create();                                               // 0x619c90
        v->LoadMap();                                              // 0x6201c0
        FocusWidget(v);
        return;
    }
    case pz::PZ_MENU_BRIEFING: {
        StopMenuMusic();                                           // Concert +0x80(1)
        sw->UnloadMenuBackground();                                // 0x65b940
        SBriefingMenu* b = new SBriefingMenu();                    // new 0x340, 0x632f40
        sw->Menu_10c = b;                                          // +0x10c
        sw->InsertChild(b);                                        // vtbl +0x54
        b->SetPosition(0, 0, 0x400, 0x300);                        // vtbl +0x08
        b->Create();                                               // 0x634f20
        FocusWidget(b);
        // HD: SDXWindow +0xd8 = 0.
        return;
    }
    case pz::PZ_MENU_RESULTS: {
        SResultsMenu* r = new SResultsMenu();                      // new 0x1228, 0x633540
        sw->Menu_114 = r;                                          // +0x114
        sw->InsertChild(r);
        r->SetPosition(0, 0, 0x400, 0x300);
        r->Create();                                               // 0x6360f0
        FocusWidget(r);
        // HD: SDXWindow +0xd8 = 0.
        return;
    }
    case pz::PZ_MENU_MULTI:
        STUB_LOG("SSuperWindow::LoadNextCampaignView multiplayer (0x658a30)");
        PZ_M3_TRACE("SSuperWindow::LoadNextCampaignView multiplayer (0x658a30)");
        return;
    case pz::PZ_MENU_NONE:
        return;
    default:
        // HD LoadMainMenu 0x6583e0 also deletes the campaign (DAT_00929a0c).
        delete pz::g_Campaign;
        pz::g_Campaign = nullptr;
        sw->LoadMainMenu();
        return;
    }
}

static void StartMission(SSuperWindow* sw)
{
    // HD LAB_0065a1c0: 0x594e50, GameView +0x6c(1), 0x6281a0.
    pz::g_Campaign->SetMenuGameView();
    if (SGameView* v = View(sw)) {
        v->SetVisible(true);
        FocusWidget(v);
        v->MissionStart();
    }
}

// HD action 0x494c2 (the cut Load Replay screen): nothing in the shipped
// exe sends it (docs/M3_REPLAY.md).
enum { PZA_LOAD_REPLAY = 0x494c2 };
bool SuperWindowM3Action(SSuperWindow* sw, int action, int param);

// SSuperWindow::OnAction 0x659250, case 0x494c2 (0x659d49..0x659e98): a
// new game view, a new campaign that reads the replay header
// (StartReplay 0x597510), -packetplay on, LoadMap (which starts the mission
// at once because of -packetplay).
static void LoadReplay(SSuperWindow* sw, const char* name)
{
    PZ_M3_TRACE("SSuperWindow::OnAction Load Replay (0x659250 / 0x494c2)");
    if (!sw->GameView) {
        if (sw->MainMenu) {                                        // +0xe8 vtbl +0 (1)
            delete sw->MainMenu;
            sw->MainMenu = nullptr;
        }
        sw->UnloadMenuBackground();                                // 0x65b940
    }
    DeleteGameView(sw);
    SGameView* v = new SGameView();                                // new 0x3e98, 0x6181f0
    sw->GameView = v;                                              // +0xe4
    sw->InsertChild(v);                                            // vtbl +0x54
    v->SetPosition(0, 0, 0x400, 0x300);                            // vtbl +0x08
    v->Create();                                                   // 0x619c90
    // HD: SDXWindow +0xd8 = 1.
    delete pz::g_Campaign;                                         // 0x591350 + delete 0xb8c
    pz::g_Campaign = new pz::SPanzersCampaign();                   // new 0xb8c, 0x590ec0
    try {
        pz::g_Campaign->StartReplay(name, (int)strlen(name));      // 0x597510 (SString by value)
    } catch (const char* e) {
        Logger.g->Panic("SPanzersCampaign::StartReplay: %s", e);   // (recompile) HD has no handler here
    }
    // HD Concert +0x80(1): not mapped yet.
    Settings.PacketRec = false;                                    // word 0x929d34 = 0x100
    Settings.PacketPlay = true;
    v->LoadMap();                                                  // 0x6201c0
    FocusWidget(v);
}

// Recompile-only test switch (docs/M3_REPLAY.md "Our build"): with -m3 and
// "-packetplay Replays\<name>", the first main menu sends HD's dead action
// 0x494c2 with <name>, so the recording's own header (nation, army) starts
// the mission. Called by SSuperWindow::LoadMainMenu.
void M3OnMainMenu(SSuperWindow* sw)
{
    static bool s_Done = false;
    if (s_Done || !pz::g_M3.Enabled || !Settings.PacketPlay || Settings.PacketFile.size == 0 || getenv("PZ_M3_NAIVE_PLAY"))
        return;
    s_Done = true;
    const char* f = Settings.PacketFile.buf;
    if (_strnicmp(f, "Replays/", 8) == 0 || _strnicmp(f, "Replays\\", 8) == 0)
        f += 8;
    Logger.g->Log(0, "PZM3: -packetplay %s: Load Replay (0x494c2) of Replays/%s", Settings.PacketFile.buf, f);
    SuperWindowM3Action(sw, PZA_LOAD_REPLAY, (int)(size_t)f);
}

// Returns true when the action was handled by the M3 path.
bool PzTutorialAction(SSuperWindow* sw, int action);   // tutorial.cpp (M4)
bool PzSkirmishAction(SSuperWindow* sw, int action, int param);   // superwindow_sk.cpp (M5-SK)

bool SuperWindowM3Action(SSuperWindow* sw, int action, int param)
{
    if (PzSkirmishAction(sw, action, param))                       // 0x534d1, 0x534b1..0x534b4 Skirmish (M5-SK)
        return true;
    if (PzTutorialAction(sw, action))                              // 0x4d4d3 Tutorial
        return true;
    switch (action) {
    case PZA_LOAD_REPLAY:                                          // 0x494c2
        LoadReplay(sw, (const char*)(size_t)param);
        return true;
    case PZA_MAIN_TRAINING: {                                      // 0x4d4d4
        PZ_M3_TRACE("SSuperWindow::OnAction Training Camp (0x659250 / 0x4d4d4)");
        STrainingMenu* t = new STrainingMenu();                    // new 0x288, 0x633b30
        sw->TrainingCampMenu = t;                                  // +0x118
        sw->InsertChild(t);
        t->Create();                                               // 0x63a370
        FocusWidget(t);
        // HD then 0x5435b0(t) / 0x544fe0(t): the dialog becomes modal.
        return true;
    }
    case PZA_TRAINING_START: {                                     // 0x544d1
        PZ_M3_TRACE("SSuperWindow::OnAction Training Start (0x544d1)");
        STrainingMenu* t = static_cast<STrainingMenu*>(sw->TrainingCampMenu);
        int nation = t ? t->Nation : 1;
        delete pz::g_Campaign;
        pz::g_Campaign = new pz::SPanzersCampaign();               // new 0xb8c, 0x590ec0
        pz::g_Campaign->Race = nation;                             // +0x18
        pz::g_Campaign->Difficulty = 0;                            // +0x1c
        pz::g_Campaign->InitTutorialMode("maps/training.map", nation, 0x5dc);   // 0x594ab0
        Logger.g->Log(0, "PZM3: Training Camp, nation %d", nation);
        if (sw->TrainingCampMenu) {                                // 0x5450e0 (end modal), delete
            delete sw->TrainingCampMenu;
            sw->TrainingCampMenu = nullptr;
        }
        if (sw->MainMenu) {                                        // +0xe8
            delete sw->MainMenu;
            sw->MainMenu = nullptr;
        }
        M3LoadNextCampaignView(sw);                                // 0x658b10
        if (pz::g_World)
            *(int*)((unsigned char*)pz::g_World + 0x174) = nation; // World +0x174 = race (player record of the local player)
        return true;
    }
    case PZA_NEWGAME_START: {                                      // 0x53441 (SSingleDiffMenu Start)
        PZ_M3_TRACE("SSuperWindow::OnAction New Game Start (0x659250 / 0x53441)");
        SMainMenu* mm = sw->MainMenu;
        SSingleMenu* single = mm ? static_cast<SSingleMenu*>(mm->NewGameMenu) : nullptr;   // main menu +0x5c
        if (!single || !single->DiffMenu)
            return true;
        delete pz::g_Campaign;                                     // (recompile) HD overwrites the pointer
        pz::g_Campaign = new pz::SPanzersCampaign();               // new 0xb8c, 0x590ec0
        pz::g_Campaign->Race = single->Race;                       // +0x18 = SSingleMenu +0x31c
        pz::g_Campaign->Difficulty = single->DiffMenu->Difficulty; // +0x1c = SSingleDiffMenu +0x3c4
        Logger.g->Log(0, "PZM4: New Game, race %d, difficulty %d", single->Race, single->DiffMenu->Difficulty);
        pz::g_Campaign->InitCampaignMode();                        // 0x592b20
        if (SWindow* w = single->DiffMenu->GetWindowParent())      // 0x5450e0
            w->UnsetModalWidget(single->DiffMenu);
        if (sw->MainMenu) {                                        // +0xe8 (deletes the New Game menus)
            delete sw->MainMenu;
            sw->MainMenu = nullptr;
        }
        M3LoadNextCampaignView(sw);                                // 0x658b10
        return true;
    }
    case PZA_NEWGAME_CANCEL: {                                     // 0x53442 (SSingleDiffMenu Cancel)
        SMainMenu* mm = sw->MainMenu;
        SSingleMenu* single = mm ? static_cast<SSingleMenu*>(mm->NewGameMenu) : nullptr;
        if (!single || !single->DiffMenu)
            return true;
        if (SWindow* w = single->DiffMenu->GetWindowParent())      // 0x5450e0
            w->UnsetModalWidget(single->DiffMenu);
        delete single->DiffMenu;                                   // +0x1b8
        single->DiffMenu = nullptr;
        return true;
    }
    case PZA_BRIEFING_DONE:                                        // 0x424d1
        DeleteWidget(sw->Menu_10c);                                // +0x10c
        pz::g_Campaign->MenuToLoad = pz::PZ_MENU_GAMEVIEW;         // +0xe0 = 2
        M3LoadNextCampaignView(sw);
        return true;
    case PZA_BRIEFING_CANCEL:                                      // 0x424d2
        DeleteWidget(sw->Menu_10c);
        CampaignToMainMenu(sw);
        return true;
    case PZA_RESULTS_CONTINUE:                                     // 0x524d1
        DeleteWidget(sw->Menu_114);                                // +0x114
        pz::g_Campaign->LetResultsDone();                          // 0x594e70
        M3LoadNextCampaignView(sw);
        return true;
    case PZA_RESULTS_RESTART:                                      // 0x524d2
    case 0x47565:
        DeleteGameView(sw);
        ReleaseWindowScene();
        DeleteWidget(sw->Menu_114);
        pz::g_Campaign->RestartMission();                          // 0x594f60
        M3LoadNextCampaignView(sw);
        return true;
    case PZA_RESULTS_MAINMENU:                                     // 0x524d3
        DeleteWidget(sw->Menu_114);
        StopMenuMusic();                                           // Concert +0x80(1)
        CampaignToMainMenu(sw);
        return true;
    case PZA_TRAINING_CANCEL:                                      // 0x544d2
        if (sw->TrainingCampMenu) {
            delete sw->TrainingCampMenu;
            sw->TrainingCampMenu = nullptr;
        }
        return true;
    case PZA_GV_MAP_LOADED:                                        // 0x47561
        Logger.g->Log(0, "SSuperWindow::OnAction: Map loaded");
        pz::g_Campaign->OnMapLoaded();                             // 0x594d50
        if (pz::g_Campaign->MenuToLoad != pz::PZ_MENU_GAMEVIEW)
            M3LoadNextCampaignView(sw);
        else
            StartMission(sw);
        return true;
    case PZA_MARKET_START:                                         // 0x4d542
        sw->ReleaseMultiView();                                    // 0x65b8c0 (moves the world back)
        StartMission(sw);
        return true;
    case PZA_MARKET_CANCEL:                                        // 0x4d541
        sw->ReleaseMultiView();
        DeleteGameView(sw);
        delete pz::g_Campaign;
        pz::g_Campaign = nullptr;
        sw->LoadMainMenu();                                        // 0x6583e0
        return true;
    case PZA_GV_GAMEOVER:                                          // 0x47562
    case PZA_GV_ERROR:                                             // 0x47563
        Logger.g->Log(0, "GV_ ... SSuperWindow::OnAction()");
        DeleteGameView(sw);
        ReleaseWindowScene();
        pz::g_Campaign->LetMapDone();                              // 0x594e00
        M3LoadNextCampaignView(sw);
        Logger.g->Log(action == PZA_GV_ERROR ? 1 : 0, action == PZA_GV_ERROR
                      ? "SSuperWindow::OnAction() GV_ERROR : What is?" : "GV_GAMEOVER ... SSuperWindow::OnAction()");
        return true;
    default:
        return false;
    }
}
