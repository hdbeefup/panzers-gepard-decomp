// src/game/world_save.h
// The SWorld chunks of the save game (world_save.cpp) and the load-game
// entry points (gamelogic_save.cpp, campaign_save.cpp). OWNER: agent S (M4).

#ifndef PZ_WORLD_SAVE_H
#define PZ_WORLD_SAVE_H

#include <windows.h>
#include "string2.h"
#include "world.h"

namespace pz {

struct SWorld;

void SaveWorldString(SStream* s, const void* sstring);
void SaveWorldPlayers(SWorld* w, SStream* s);           // 0x5fb160 PLY3
void SaveWorldAIGroups(SWorld* w, SStream* s);          // 0x57db20 AIGP
void SaveWorldUnits(SWorld* w, SStream* s);             // 0x5fb630 UNIS
void SaveWorldEffects(SWorld* w, SStream* s);           // 0x5f9450 EEFS
void SaveWorldCamera(SWorld* w, SStream* s);            // 0x5e6a70 "CAM "
void SaveWorldLocations(SWorld* w, SStream* s);         // 0x5f9210 LOCS
void SaveWorldTriggers(SWorld* w, SStream* s);          // 0x5f8a80 TRIG
void SaveWorldTriggerVariables(SWorld* w, SStream* s);  // 0x57dd60 TVAR
void SaveWorldDoodadAnims(SWorld* w, SStream* s);       // 0x5f8cf0 ODDD
void SaveWorldWires(SWorld* w, SStream* s);             // 0x5fb7d0 WIR3
void SaveWorldWeather(SWorld* w, SStream* s);           // 0x5fb770 WTHR

void LoadWorldUnits(SWorld* w, SStream* s);             // 0x5f3820 UNIS
void LoadWorldDoodadAnims(SWorld* w, SStream* s);       // 0x5f1fd0 ODDD
void LoadWorldWeather(SWorld* w, SStream* s);           // 0x5f3cb0 WTHR

// campaign_save.cpp: the save-file header (0x5955d0) for the Load Game list
// and SGameView's load-game LoadMap.
bool ReadSaveGameMapName(const char* file, SString* map);
bool ReadSaveGameTitle(const char* file, SString* title);

// HD SLoadSaveName (0x30 bytes), an entry of the Load Game list.
struct SLoadSaveName {
    SString    Code;     // +0x00 mission code ("TRNG")
    SString    Title;    // +0x08 "Start - maps/training.map"
    SString    Date;     // +0x10 "<short date> <hh:mm>" of the file
    SString    File;     // +0x18 file name in SaveGames/
    SYSTEMTIME Time;     // +0x20
};
// PANZERS 0x595fa0 SPanzersCampaign::LoadSavedGameNames (campaign_save.cpp)
void LoadSavedGameNames(SHdArray<SLoadSaveName>* out);
// PANZERS 0x596b30 SPanzersCampaign::SaveGameBefore (campaign_save.cpp)
struct SPanzersCampaign;
bool CampaignSaveGameBefore(SPanzersCampaign* c);
// PANZERS 0x595330 SPanzersCampaign::LoadGameBefore (campaign_save.cpp): a
// type-2 save (SaveGames/<file>) into a new campaign, then PrepareMission.
void CampaignLoadGameBefore(SPanzersCampaign* c, const char* file);
// PANZERS 0x5925d0 SCampaign::GetName: the map (modes 1, 2, 4, 5), the
// mission's localised "Name" (mode 3, default "Unnamed").
const char* CampaignGetName(SPanzersCampaign* c);
// PANZERS 0x5955d0: the save type of SaveGames/<file> (1 in a mission,
// 2 "Before", 0 when the file does not open or its header does not read)
// and its map.
int ReadSaveGameType(const char* file, SString* map);

} // namespace pz

#endif // PZ_WORLD_SAVE_H
