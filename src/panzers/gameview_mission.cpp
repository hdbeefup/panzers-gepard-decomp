// src/panzers/gameview_mission.cpp
// SGameView::LoadMap 0x6201c0 and SGameView::MissionStart 0x6281a0 (the
// mission flow of the game view). OWNER: agent F (docs/M3_INTERFACES.md).
// Lifted from the HD exe; the parts that belong to other agents' code are
// called through their functions (V: CreateSubViewports, SetPanelMode,
// Update; O: the packet streams' frames).

#include <windows.h>
#include <string.h>
#include "gameview.h"
#include "superwindow.h"
#include "settings.h"
#include "pzboard.h"
#include "m3common.h"
#include "campaign.h"
#include "stub_log.h"
#include "logger.h"
#include "timer.h"
#include "stream.h"
#include "gettext.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/iviewport.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "doodad.h"

// PANZERS 0x6201c0
// HD order: loading backdrop 0x61f460(0); the map name; MissionResult = 0;
// "Loading map: %s"; new SWorld; the loading icon; SWorld::LoadMap,
// Initialize and the three load extras; the mission properties of the map's
// MINA section; objectives, support calls, cursor, water map, bridges; the
// mission SGameLogic; the army prototypes; -script / -cutscene; then either
// mission start at once (no section, -packetplay, no market) or the "Click
// to continue" screen.
void PzGameViewEndCheck(SGameView* view);

void SGameView::LoadMap()
{
    PZ_M3_TRACE("SGameView::LoadMap (0x6201c0)");
    // HD 0x61f460(0): the "menu/loading_hq.tga" backdrop (+0x3894 / +0x3898)
    // over the view, and a frame through the viewport. Not lifted (visual).
    if (!pz::g_Campaign) {
        Logger.g->Warning("SGameView::LoadMap() Campaing == NULL");   // 0x65cac0
        pz::g_Campaign = new pz::SPanzersCampaign();             // new 0xb8c, 0x590ec0
        pz::g_Campaign->InitTutorialMode("maps/harc_teszt.map", 0, 0);   // HD only sets the name (0x52c320)
    }
    SString map;
    map = pz::g_Campaign->GetMapName();                           // 0x592040, 0x52c320
    pz::g_Campaign->SetMissionResult(0);                            // 0x597470(0)
    Logger.g->Log(0, "Loading map: %s", pz::SStr(map));
    if (!pz::g_M3.LoadMap) {
        Logger.g->Log(0, "PZM3: PZ_M3_LOADMAP=0, map not loaded");
        pz::FreeSString(&map);
        LoadingScreen = true;
        LoadingFrame = Board->CreateFrame(FT_TEXT, GetFrame(), 0x200, 0x2e0, 0, 1);
        Board->SetText(LoadingFrame, g_PzFont[PZF_SANS14], 2, GetText("panzers/GameView.cpp", "Click to continue"));
        Board->ShowFrame(LoadingFrame, true);
        return;
    }
    SStream* stream = FileSystem.OpenRead(pz::SStr(map), nullptr); // 0x65f420(name, 0)
    if (!stream)
        Logger.g->Panic("Can't load map: %s", pz::SStr(map));
    World = new pz::SWorld(0);                                    // new 0x7538, 0x5d2f90(0)
    World->ShowLoadingIcon(GetFrame());                           // 0x5edca0(+0x48)
    if (!World->LoadMap(stream, true, 0, 0))                      // 0x5f1990(stream, 1, 0, 0)
        Logger.g->Panic("Can't load map: %s", pz::SStr(map));
    World->Initialize();                                          // 0x5eec90
    World->LoadMapExtra_5e2d70();
    World->LoadMapExtra_5debb0();
    World->LoadMapExtra_607ad0();
    stream->Release();                                            // vtbl +0 (1)
    if (World->MinimapName.size != 0) {
        pz::g_Campaign->MissionSection = World->MinimapName;      // 0x52c2c0(World+0x751c)
        pz::g_Campaign->LoadMissionProps();                       // 0x593740
    }
    pz::g_Campaign->LoadObjectives();                             // 0x593ba0
    pz::g_Campaign->LoadSupportCounts();                          // 0x594140
    // HD 0x543970(0, -1): the window cursor (SWindow).
    World->UpdateWaterMap();                                      // 0x608600
    World->FixBridges();                                          // 0x5e65f0
    // new 0x318, 0x55e440(0, view +0x828, view +0x58): the minimap frame
    // and the SIGameViewCallback (logic +0x17c, +0x00).
    Logic = new pz::SGameLogic(0, LogicFrame828, (int)(size_t)static_cast<SIGameViewCallback*>(this));
    Logic->SetBoardArea(_4b8, 0x13c, 7, 7);                       // 0x57fac0
    Logic->PreloadArmyUnits();                                    // 0x56e8e0
    if (Settings._88)                                             // 0x929d80 -script
        Logger.g->Log(1, "STUB: SGameView::LoadMap -script %s (0x56f9e0) not run", pz::SStr(Settings._8c));
    if (Settings.CutsceneFile.size != 0)                          // 0x929d90 -cutscene
        Logger.g->Log(1, "STUB: SGameView::LoadMap -cutscene %s (0x56ea20) not run", pz::SStr(Settings.CutsceneFile));
    World->HideLoadingIcon();                                     // 0x5dc7d0
    pz::LogWorldStats("SGameView::LoadMap");
    pz::SPanzersCampaign* c = pz::g_Campaign;
    if (c->MissionSection.size == 0 || Settings.PacketPlay || (c->IsTutorialMode() && !c->HasMarket()) ||
        (c->IsScenarioMode() && !c->HasMarket()) || c->IsMultiMode()) {
        MissionStart();                                           // 0x6281a0
    } else {
        // HD: board +0x18(+0x388c, 1) shows the "Click to continue" frame
        // that Create made; the recompile makes it here.
        LoadingScreen = true;                                     // +0x3890
        LoadingFrame = Board->CreateFrame(FT_TEXT, GetFrame(), 0x200, 0x2e0, 0, 1);
        Board->SetText(LoadingFrame, g_PzFont[PZF_SANS14], 2, GetText("panzers/GameView.cpp", "Click to continue"));
        Board->ShowFrame(LoadingFrame, true);
        Cursor = 0;                                               // 0x543970(0, -1)
    }
    // HD 0x626290: the tool-tip texts of the hot keys (H, unit panel).
    *(int*)((unsigned char*)World + 0x73ac) = Settings.UnitVoice; // 0x64e2b0
    pz::FreeSString(&map);
}

// PANZERS 0x6281a0
void SGameView::MissionStart()
{
    PZ_M3_TRACE("SGameView::MissionStart (0x6281a0)");
    // HD Concert +0x80(1) (music): not mapped yet.
    if (pz::g_WindowScene)
        pz::g_WindowScene->Release();                             // window +0xe0 vtbl +4
    if (pz::g_Scene)
        pz::g_Scene->AddRef();                                    // vtbl +0
    pz::g_WindowScene = pz::g_Scene;
    CreateSubViewports();                                         // 0x61e500
    if (SoundHandle3898 >= 0) {                                   // board +0x0c / +0x80: the 0x61f460 backdrop
        Board->DestroyFrame(SoundHandle3898);
        SoundHandle3898 = -1;
    }
    SoundHandle3894 = -1;
    unsigned now = NowMs();                                       // ftol(0x661800() * 1000.0)
    ClockNextTick = now;                                          // +0x460
    ClockStart = now;                                             // +0x45c
    if (!Logic || !World) {

        return;
    }
    Logic->PlaceAllUnits();                                       // 0x571c70
    World->StartEffects();                                        // 0x5f5b50
    if (pz::g_Campaign->GetMissionResult() == 0)
        Logic->SetRunning(1);                                     // 0x5802f0(1)
    Logic->SetVisOverlayMode(Settings.FogOfWarView);              // 0x57f970(0x64df00())
    World->_4d0[0] = 0;                                           // World+0x4d0
    if (Settings.PacketRec)                                       // 0x929d34
        Logic->StartPacketRecording(pz::SStr(Settings.PacketFile));   // 0x5805c0
    if (Settings.PacketPlay) {                                    // 0x929d35
        if (pz::g_Campaign->ReplayName.size == 0) {
            Logic->StartPacketPlayback(pz::SStr(Settings.PacketFile));   // 0x580540
        } else {
            SString name;
            name = pz::g_Campaign->ReplayName;                    // 0x52c2c0
            Logic->StartPacketPlayback(pz::SStr(name));
            pz::g_Campaign->ReplayName = "";                      // 0x52c320("")
            pz::FreeSString(&name);
        }
    }
    World->InitCameraSpline(pz::SStr(Settings._40));              // 0x609760 (-csplay file)
    SetPanelMode(0);                                              // 0x625d80
    // HD: unless multiplayer (DAT_008f1a74).
    pz::g_Campaign->SaveGameStartMission("Start", GetText("panzers/GameView.cpp", "Start"));   // 0x596e30
    Logger.g->Log(0, "PZM3: mission start");
    if (Settings.SkipFrames > 0) {                                // 0x929d60
        // HD: viewport(0) +0x50(0, 0xff), then the frames without drawing.
        for (int i = 0; i < Settings.SkipFrames; ++i)
            Logic->Refresh();                                     // 0x576d80
        Logic->SetRunning(0);
    } else {
        Logic->Refresh();                                         // 0x576d80: frame 0 of the mission
    }
}

// PANZERS 0x628430 (the tail from 0x62bb83, single player; called by V's
// SGameView::Update)
// The "exit" switch 0x929d90 (send 0x47564 once the logic runs), then,
// while no end box (+0x3e84), results menu (+0x3e6c) or network box
// (+0x3e94) is up, the campaign's mission result 0x5920b0: victory (1), or
// the local player out (World player +0x1c == 2), keeps the surviving army
// (BackupCampaignUnits 0x561110) and stops the logic; defeat (3) stops it
// too. The boxes (SMessageBox 0x53e0d0, "Mission accomplished" / "Mission
// failed", OK -> 0x47562), the results menu (0x62cd50) and the music are
// agent H's screens and are not made here: the recompile sends 0x47562.
void PzGameViewEndCheck(SGameView* view)
{
    if (Settings.CutsceneFile.size != 0) {                        // 0x929d90
        if (!view->Logic->IsPaused())                             // 0x56e150
            view->SendAction(PZA_GV_EXIT, 0);                     // 0x543930(0x47564)
        return;
    }
    if (view->EndBox || view->HdInt(0x3e6c) || view->NetBox)
        return;
    pz::SPanzersCampaign* c = pz::g_Campaign;
    int result = c->GetMissionResult();                           // 0x5920b0
    pz::SWorld* w = view->World;
    bool out = *(int*)(w->Players[w->LocalPlayer] + 0x1c) == 2;   // World+0x18c + local * 0x48
    if (result != 1 && result != 3 && !out)
        return;
    if ((result == 1 || out) && !c->_014)
        view->Logic->BackupCampaignUnits();                       // 0x561110
    view->Logic->SetRunning(0);                                   // 0x5802f0(0)
    Logger.g->Log(0, "PZM3: mission result %d%s: end box / results menu not lifted (H), ending the mission",
                  result, out ? " (local player out)" : "");
    view->SendAction(PZA_GV_GAMEOVER, 0);
}
