// src/game/campaign.cpp
// pz::SPanzersCampaign (campaign.h). OWNER: agent L (docs/M3_INTERFACES.md).
// M3-P0 lifted the small flow functions the Training Camp path needs; the
// rest are logged skeleton stubs.

#include <string.h>
#include "campaign.h"
#include "stub_log.h"
#include "logger.h"
#include "stream.h"

namespace pz {

SPanzersCampaign* g_Campaign = nullptr;   // HD 0x929a0c

SPanzersCampaign::SPanzersCampaign()
{
    STUB_LOG("SPanzersCampaign::SPanzersCampaign (0x590ec0)");
    PZ_M3_TRACE("SPanzersCampaign::SPanzersCampaign (0x590ec0)");
    // Skeleton: the fields the ctor sets. Not done: the eh-vector ctors of
    // the 12 records at +0x48 (0x590ea0) and +0x140 (0x5910c0, 0xd8 each)
    // and FUN_00594470.
    memset(this, 0, sizeof(*this));
    Difficulty = 1;                                  // param_1[7] = 1
    ((int*)this)[0x40] = -1;                         // +0x100
    ((unsigned char*)this)[0xb80] = 2;               // param_1[0x2e0] (byte)
    ((unsigned char*)this)[0xb88] = 1;               // param_1[0x2e2] (byte)
}

SPanzersCampaign::~SPanzersCampaign()
{
    STUB_LOG("SPanzersCampaign::~SPanzersCampaign (0x591350)");
    PZ_M3_TRACE("SPanzersCampaign::~SPanzersCampaign (0x591350)");
    delete[] MapName.buf;
    delete[] MissionSection.buf;
    delete[] ReplayName.buf;
}

// PANZERS 0x594ab0
void SPanzersCampaign::InitTutorialMode(const char* map, int race, int prestige)
{
    PZ_M3_TRACE("SCampaign::InitTutorialMode (0x594ab0)");
    if (GameMode != PZ_GM_NONE)
        Logger.g->Panic("SCampaign::InitTutorialMode: GameMode can only be initialized once.");
    GameMode = PZ_GM_TUTORIAL;
    Race = race;
    MapName = map;                                   // 0x52c320 on +0x20
    StartPrestige = prestige;
    Prestige = prestige;
    MenuToLoad = PZ_MENU_GAMEVIEW;
    // 0x591500(+0x2c): StartArmy (+0x3c) = Army (+0x2c), a deep copy of the
    // 0x80-byte army records (0x560290). A new campaign has no army.
    if (ArmyCount != 0)
        Logger.g->Log(0, "STUB: SCampaign::InitTutorialMode army copy (0x591500) of %d records not done", ArmyCount);
    StartPaused = 0;
}

void SPanzersCampaign::InitScenarioMode(const char* map, const char* section, int race, int prestige)
{
    STUB_LOG("SCampaign::InitScenarioMode (0x5944d0)");
    PZ_M3_TRACE("SCampaign::InitScenarioMode (0x5944d0)");
    (void)map; (void)section; (void)race; (void)prestige;
}

void SPanzersCampaign::InitCampaignMode(const char* missionsIni, const char* mission, int race, int difficulty)
{
    STUB_LOG("SCampaign::InitCampaignMode (0x592b20)");
    PZ_M3_TRACE("SCampaign::InitCampaignMode (0x592b20)");
    (void)missionsIni; (void)mission; (void)race; (void)difficulty;
}

// PANZERS 0x596600
int SPanzersCampaign::GetMenuToLoad()
{
    return MenuToLoad;
}

// PANZERS 0x594d50
void SPanzersCampaign::OnMapLoaded()
{
    PZ_M3_TRACE("SCampaign::OnMapLoaded (0x594d50)");
    if (GameMode == PZ_GM_CAMPAIGN || GameMode == PZ_GM_SCENARIO || GameMode == PZ_GM_2) {
        // HD: if missions.ini [<section>] Market = 0 (0x660500 default 1):
        // MenuToLoad = 2, +0xe4 = 0, return. Needs the mission properties
        // (agent L); the skeleton falls through to the prestige rule.
        STUB_LOG("SCampaign::OnMapLoaded missions.ini Market key (0x594d50)");
    }
    StartPaused = 0;
    MenuToLoad = 2 - (StartPrestige != 0);           // market when there is prestige to spend
}

// PANZERS 0x594e50
void SPanzersCampaign::SetMenuGameView()
{
    MenuToLoad = PZ_MENU_GAMEVIEW;
    StartPaused = 0;
}

// PANZERS 0x594e00
void SPanzersCampaign::LetMapDone()
{
    PZ_M3_TRACE("SCampaign::LetMapDone (0x594e00)");
    if (MenuToLoad != PZ_MENU_GAMEVIEW)
        Logger.g->Panic("SCampaign::LetMapDone: Menu order problem.");
    switch (GameMode) {
    case 1: case 2: case 3: case 4:
        MenuToLoad = PZ_MENU_RESULTS;
        break;
    case 5:
        MenuToLoad = 5;                              // -> LoadNextCampaignView default: main menu
        break;
    default:
        break;
    }
}

void SPanzersCampaign::LetResultsDone()
{
    STUB_LOG("SCampaign::LetResultsDone (0x594e70)");
    PZ_M3_TRACE("SCampaign::LetResultsDone (0x594e70)");
}

// PANZERS 0x592040 (modes 1, 2, 4, 5, 6; mode 3 reads missions.ini)
const char* SPanzersCampaign::GetMapName()
{
    switch (GameMode) {
    case 1: case 2: case 4: case 5: case 6:
        return MapName.buf ? MapName.buf : "";
    case 3:
        STUB_LOG("SCampaign::GetMapName campaign map from missions.ini (0x592040)");
        return "";
    default:
        Logger.g->Panic("SCampaign::GetMapName: unknown game mode");
        return "";
    }
}

// PANZERS 0x594d40
bool SPanzersCampaign::IsTutorialMode() { return GameMode == PZ_GM_TUTORIAL; }

// PANZERS 0x594d30
bool SPanzersCampaign::IsScenarioMode() { return GameMode == PZ_GM_SCENARIO; }

bool SPanzersCampaign::IsMultiMode()
{
    STUB_LOG("SCampaign::IsMultiMode (0x594d20)");
    PZ_M3_TRACE("SCampaign::IsMultiMode (0x594d20)");
    return false;
}

bool SPanzersCampaign::HasMarket()
{
    STUB_LOG("SCampaign::HasMarket (0x596610)");
    PZ_M3_TRACE("SCampaign::HasMarket (0x596610)");
    // HD: true outside modes 1, 2, 3; else missions.ini [section] Market (default 1).
    return true;
}

// PANZERS 0x5920b0
int SPanzersCampaign::GetStartPaused() { return StartPaused; }

void SPanzersCampaign::LoadObjectives()
{
    STUB_LOG("SCampaign::LoadObjectives (0x593ba0)");
    PZ_M3_TRACE("SCampaign::LoadObjectives (0x593ba0)");
}

bool SPanzersCampaign::SaveGame(const char* file, const char* title)
{
    STUB_LOG("SPanzersCampaign::SaveGame (0x5966a0)");
    PZ_M3_TRACE("SPanzersCampaign::SaveGame (0x5966a0)");
    (void)file; (void)title;
    return true;
}

int SPanzersCampaign::SaveGameStartMission(const char* name, const char* title)
{
    STUB_LOG("SPanzersCampaign::SaveGameStartMission (0x596e30)");
    PZ_M3_TRACE("SPanzersCampaign::SaveGameStartMission (0x596e30)");
    // HD: SaveGame("SaveGames/<Mission code>-<name>.save", title); panics
    // "SPanzersCampaign::SaveGameStartMission() - failed" when it fails.
    // Training writes SaveGames/TRNG-Start.save. The world CRC does not
    // depend on it, so the skeleton writes nothing.
    (void)name; (void)title;
    return 1;
}

bool SPanzersCampaign::LoadGame(const char* file)
{
    STUB_LOG("SPanzersCampaign::LoadGame (0x594f70)");
    PZ_M3_TRACE("SPanzersCampaign::LoadGame (0x594f70)");
    (void)file;
    return false;
}

void SPanzersCampaign::WriteReplayHeader(SStream* s)
{
    STUB_LOG("SPanzersCampaign::WriteReplayHeader (0x596fc0)");
    PZ_M3_TRACE("SPanzersCampaign::WriteReplayHeader (0x596fc0)");
    (void)s;
}

void SPanzersCampaign::ReadReplayHeader(SStream* s)
{
    STUB_LOG("SPanzersCampaign::ReadReplayHeader (0x595780)");
    PZ_M3_TRACE("SPanzersCampaign::ReadReplayHeader (0x595780)");
    (void)s;
}

void SPanzersCampaign::StartReplay(const char* name, int len)
{
    STUB_LOG("SPanzersCampaign::StartReplay (0x597510)");
    PZ_M3_TRACE("SPanzersCampaign::StartReplay (0x597510)");
    (void)name; (void)len;
}

} // namespace pz
