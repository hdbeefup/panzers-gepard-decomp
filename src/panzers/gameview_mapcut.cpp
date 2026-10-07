// src/panzers/gameview_mapcut.cpp
// The map cut-scene of trigger action 0x38 (M5 agent CS): SIGameViewCallback
// +0x10 (HD 0x624770) loads another map in place of the mission's map, with
// the same SGameLogic, and the mission goes on there. The campaign's
// ger-04, ger-08 and ger-11 switch to maps/ger-04-3.map, ger-08-4.map and
// ger-11-2.map this way; those maps play their cut-scene and end the mission.
// Also the SGameLogic -> view callbacks the game code uses (cutscene.cpp,
// triggers.cpp): SGameLogic +0x00 is the view's SIGameViewCallback.

#include <windows.h>
#include <string.h>
#include "gameview.h"
#include "settings.h"
#include "pzboard.h"
#include "m3common.h"
#include "campaign.h"
#include "logger.h"
#include "stream.h"
#include "iconcert.h"
#include "milesconcert.h"
#include "pz/iscene.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "cutscene.h"

extern SIConcert* Concert;

// PANZERS 0x624770
// The old world goes (its surviving campaign units first: BackupCampaignUnits
// 0x561110), the new map is loaded as LoadMap 0x6201c0 does, and the mission
// start of 0x6281a0 follows (army placed, effects, running, fog-of-war view).
// The trigger code (RunTriggers case 0x38) tore the logic's map state down
// before and rebuilds it after this call.
void SGameView::Slot_10(int p1)
{
    PZ_M3_TRACE("SGameView::LoadMapInPlace (0x624770)");
    const char* map = (const char*)(intptr_t)p1;
    if (Logic && pz::g_Campaign)                                  // +0x3dec (view +0x3e44), DAT_00929a0c
        Logic->BackupCampaignUnits();                             // 0x561110
    ShowLoadingBackdrop(true);                                    // 0x61f460(1)
    pz::g_GameLogic = nullptr;                                    // DAT_008f2078 = 0
    if (World) {
        delete World;                                             // vtbl +0 (1); releases its scene
        World = nullptr;
    }
    SString name;
    name = map ? map : "";                                        // 0x766b87 + 0x76b3a0 copy
    Logger.g->Log(0, "Loading map: %s", pz::SStr(name));
    SStream* stream = FileSystem.OpenRead(pz::SStr(name), nullptr);   // 0x65f420(name, 0)
    if (!stream)
        Logger.g->Panic("Can't load map: %s", pz::SStr(name));
    World = new pz::SWorld(0);                                    // new 0x7538, 0x5d2f90(0)
    World->ShowLoadingIcon(GetFrame());                           // 0x5edca0(+0x48)
    // HD: Gepard +0x3c(0) viewport +0x50(0, 0): one loading frame (as
    // LoadGame, the recompile draws it from the loading icon).
    if (!World->LoadMap(stream, true, 0, 0))                      // 0x5f1990(stream, 1, 0, 0)
        Logger.g->Panic("Can't load map: %s", pz::SStr(name));
    World->Initialize();                                          // 0x5eec90
    World->LoadMapExtra_5e2d70();
    World->LoadMapExtra_5debb0();
    World->LoadMapExtra_607ad0();
    stream->Release();                                            // vtbl +0 (1)
    // HD 0x543970(0, -1): the window cursor (SWindow).
    World->UpdateWaterMap();                                      // 0x608600
    World->FixBridges();                                          // 0x5e65f0
    pz::g_GameLogic = Logic;                                      // DAT_008f2078 = +0x3dec
    World->HideLoadingIcon();                                     // 0x5dc7d0
    if (SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert))
        pc->StopStream(true);                                     // Concert +0x80(1): the briefing music of 0x61f460
    if (pz::g_WindowScene)
        pz::g_WindowScene->Release();                             // window +0xe0 vtbl +4
    if (pz::g_Scene)
        pz::g_Scene->AddRef();                                    // vtbl +0
    pz::g_WindowScene = pz::g_Scene;
    CreateSubViewports();                                         // 0x61e500
    ReleaseLoadingBackdrop();                                     // 0x619b40
    unsigned now = NowMs();                                       // ftol(0x661800() * 1000.0)
    ClockNextTick = now;                                          // +0x460
    ClockStart = now;                                             // +0x45c
    Logic->PlaceAllUnits();                                       // 0x571c70
    World->StartEffects();                                        // 0x5f5b50
    if (pz::g_Campaign->GetMissionResult() == 0)                  // 0x5920b0
        Logic->SetRunning(1);                                     // 0x5802f0(1)
    Logic->SetVisOverlayMode(Settings.FogOfWarView);              // 0x57f970(0x64df00())
    World->_4d0[0] = 0;                                           // World+0x4d0
    // HD 0x626290: the tool-tip texts of the hot keys (as LoadMap: not here).
    *(int*)((unsigned char*)World + 0x73ac) = Settings.UnitVoice; // 0x64e2b0
    pz::FreeSString(&name);
    pz::LogWorldStats("SGameView::LoadMapInPlace");
}

namespace pz {

// SGameLogic +0x00 vtbl +0x10: the map cut-scene (RunTriggers case 0x38).
void PzViewLoadMapInPlace(int callback, const char* map)
{
    if (callback)
        reinterpret_cast<SIGameViewCallback*>((intptr_t)callback)->Slot_10((int)(intptr_t)map);
}

// SGameLogic +0x00 vtbl +0x0c: the view clock restarts (0x56f530).
void PzViewResetClock(int callback)
{
    if (callback)
        reinterpret_cast<SIGameViewCallback*>((intptr_t)callback)->ResetClock();
}

} // namespace pz
