// src/game/campaign_multi.cpp
// SPanzersCampaign in multi mode (GameMode 4): what the skirmish room
// (src/panzers/skirmish.cpp) and SSuperWindow's skirmish cases call. Lifted
// from the HD exe. OWNER: agent SK (M5, docs/m5/sk.md).

#include "campaign.h"
#include "logger.h"
#include "m3common.h"
#include <string.h>

namespace pz {

// PANZERS 0x593b70
void SPanzersCampaign::InitMultiMode()
{
    if (GameMode != PZ_GM_NONE)
        Logger.g->Panic("SCampaign::InitMultiMode: GameMode can only be initialized once.");
    GameMode = 4;
    MenuToLoad = 1;                                               // +0xe0 = 1
    LoadMissionProps();                                           // 0x593740
}

// PANZERS 0x594dc0
void SPanzersCampaign::LetChatroomDone()
{
    if (GameMode != 4 || MenuToLoad != 1)
        Logger.g->Panic("SCampaign::LetChatroomDone: Menu order problem.");
    MenuToLoad = PZ_MENU_GAMEVIEW;                                // +0xe0 = 2
    MissionResult = 0;                                            // +0xe4 = 0
}

// PANZERS 0x591d50
// SetArmyName("") and SetArmyFileName("") (inline), no mission army.
void SPanzersCampaign::ClearArmy()
{
    if (GameMode != 4)
        Logger.g->Panic("SCampaign::SetArmyName: ArmyName can only be set in Multi mode.");
    ArmyName = "";                                                // 0x52c320 on +0xe8
    ArmyFileName = "";                                            // 0x52c320 on +0xf0
    ArmyResize(&MissionArmy, 0);                                  // 0x51e520(0)
    Prestige = StartPrestige;                                     // +0x38 = +0x28
}

// PANZERS 0x591e00
const char* SPanzersCampaign::GetArmyName() const
{
    if (GameMode != 4)
        Logger.g->Panic("SCampaign::GetArmyName: GameMode is not Multi.");
    return ArmyName.buf ? ArmyName.buf : "";
}

// PANZERS 0x591dd0
const char* SPanzersCampaign::GetArmyFileName() const
{
    if (GameMode != 4)
        Logger.g->Panic("SCampaign::GetArmyFileName: GameMode is not Multi.");
    return ArmyFileName.buf ? ArmyFileName.buf : "";
}

// PANZERS 0x597150
void SPanzersCampaign::SetArmyName(const char* name)
{
    if (GameMode != 4)
        Logger.g->Panic("SCampaign::SetArmyName: ArmyName can only be set in Multi mode.");
    ArmyName = name ? name : "";                                  // 0x52c320
}

// PANZERS 0x597120
void SPanzersCampaign::SetArmyFileName(const char* file)
{
    if (GameMode != 4)
        Logger.g->Panic("SCampaign::SetArmyFileName: ArmyFileName can only be set in Multi mode.");
    ArmyFileName = file ? file : "";                              // 0x52c320
}

// PANZERS 0x5974b0
void SPanzersCampaign::SetRace(int race)
{
    if (GameMode != 4)
        Logger.g->Panic("SCampaign::SetRace: Race can only be set in Multi mode.");
    Race = race;                                                  // +0x18
}

// PANZERS 0x597480
void SPanzersCampaign::SetMissionSP(int sp)
{
    if (GameMode != 4)
        Logger.g->Panic("SPanzersCampaign::SetMissionSP: MissionSP can only be set in Multi mode.");
    StartPrestige = sp;                                           // +0x28
}

// The inline SString assignment (0x52c320) of SMulti +0x4b44 to +0x20 in
// SSkirmishChatRoomMenu::Update 0x656210.
void SPanzersCampaign::SetMapName(const char* path)
{
    MapName = path ? path : "";
}

} // namespace pz
