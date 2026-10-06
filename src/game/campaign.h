// src/game/campaign.h
// pz::SPanzersCampaign: the HD campaign / game-mode object (0xb8c bytes, no
// vtable; ctor 0x590ec0). HD keeps one in DAT_00929a0c (g_Campaign). The
// symbols name its functions both SCampaign:: and SPanzersCampaign:: (one
// class in the HD build; SCampaign is its base with no own vtable either).
//
// Who creates it (all in SSuperWindow::OnAction 0x659250 / Play 0x65b470):
//   0x544d1 Training Camp "Start": new, Race (+0x18) = STrainingMenu +0x284,
//           Difficulty (+0x1c) = 0, InitTutorialMode("maps/training.map",
//           race, 1500); LoadNextCampaignView; then World+0x174 = race.
//   0x4d4d3 Tutorial: InitTutorialMode("maps/tutorial.map", 1, 0).
//   0x53441 New Game: InitCampaignMode 0x592b20 (missions.ini).
//   0x494c1 Load Game: LoadGame 0x594f70 / LoadGameBefore.
//   0x494c2 Replay: StartReplay 0x597510 (dead in the shipped game: nothing
//           sends 0x494c2, see docs/M3_REPLAY.md).
//   Play -map / bare map path: InitScenarioMode 0x5944d0 (panics in the
//           original on training.map, docs/M3_REPLAY.md).
//
// The flow it drives (MenuToLoad +0xe0, read by SSuperWindow::
// LoadNextCampaignView 0x658b10): 0 SBriefingMenu, 1 SMarket, 2 SGameView,
// 3 SResultsMenu, 4 multiplayer menus, 6 nothing, else the main menu.
// Training: InitTutorialMode sets 2 (game view); "Map loaded" (0x47561 ->
// OnMapLoaded 0x594d50) sets 1 (market) because Prestige != 0; the market's
// Start Mission (0x4d542) -> SetMenuGameView 0x594e50 (2) -> mission start
// SGameView 0x6281a0; End Mission (0x47562) -> LetMapDone 0x594e00 ->
// LoadNextCampaignView -> main menu.
//
// SHARED HEADER (owner P0; names and bodies: agent F, docs/M3_INTERFACES.md).
// Fields are at HD offsets; "(guessed)" names rest on one reader/writer.

#ifndef PZ_CAMPAIGN_H
#define PZ_CAMPAIGN_H

#include <stddef.h>
#include "m3common.h"
#include "string2.h"

struct SStream;

namespace pz {

// Game modes (+0x10). Set once; a second Init* panics "GameMode can only be
// initialized once".
enum PzGameMode {
    PZ_GM_NONE      = 0,
    PZ_GM_SCENARIO  = 1,   // InitScenarioMode 0x5944d0 (IsScenarioMode 0x594d30)
    PZ_GM_2         = 2,   // (multiplayer / skirmish; OnMapLoaded tests 1, 2, 3)
    PZ_GM_CAMPAIGN  = 3,   // InitCampaignMode 0x592b20
    PZ_GM_TUTORIAL  = 5,   // InitTutorialMode 0x594ab0 (tutorial and Training Camp; IsTutorialMode 0x594d40)
};

// MenuToLoad (+0xe0), read by SSuperWindow::LoadNextCampaignView 0x658b10.
enum PzMenuToLoad {
    PZ_MENU_BRIEFING = 0,  // SBriefingMenu 0x340 (0x632f40 + 0x634f20)
    PZ_MENU_MARKET   = 1,  // SMarket 0x25b4 (0x63f2f0 + Create 0x6407d0); world moved aside
    PZ_MENU_GAMEVIEW = 2,  // SGameView 0x3e98 (0x6181f0 + Create 0x619c90 + LoadMap 0x6201c0)
    PZ_MENU_RESULTS  = 3,  // SResultsMenu 0x1228 (0x633540 + 0x6360f0)
    PZ_MENU_MULTI    = 4,  // 0x658230 / 0x658a30
    PZ_MENU_NONE     = 6,  // returns
};

struct SUnitDef;

// SDArray<SUnitDef> of the army ({array, size, max}; 0x80-byte UNTD records,
// pzunitregistry.h). Resize 0x51e520 frees the old records and zeroes the
// whole capacity; 0x560290 deep-copies one record.
struct SArmyArray {
    SUnitDef* Array;
    int       Size;
    int       Max;
};
void ArmyResize(SArmyArray* a, int size);                 // 0x51e520
void ArmyCopyFrom(SArmyArray* dst, const SArmyArray* src);// 0x591500 / 0x591e70 (resize + 0x560290 per record)
void ArmyFree(SArmyArray* a);                             // 0x51de90 (the array dtor)
SUnitDef* ArmyAdd(SArmyArray* a);                         // 0x560d10 (SDArray::Add, grows by 6/5)
void UnitDefCopy(SUnitDef* dst, const SUnitDef* src);     // 0x560290 (operator=)
void UnitDefSave(const SUnitDef* d, SStream* s);          // 0x5cfbf0 gSaveVariables(s, d, 0x8ddb48)

// Objective (+0x134 SDArray, 0x1c each, LoadObjectives 0x593ba0).
struct SCampaignObjective {
    int      State;          // +0x00 0 (saved as int)
    bool     Hidden;         // +0x04 "Objective %d Hidden" (saved as byte)
    bool     Main;           // +0x05 "Objective %d Main"
    bool     Secret;         // +0x06 "Objective %d Secret"
    bool     Hero;           // +0x07 "Objective %d Hero"
    SString  Text;           // +0x08 "Objective %d Text"
    float*   Targets;        // +0x10 SDArray of {x, z} ("Objective %d Target %d X/Z")
    int      TargetCount;    // +0x14
    int      TargetMax;      // +0x18
};

struct SPanzersCampaign {
    void*         MissionProps;     // +0x000 SProperties* of <dir>/<section>/mission.ini or missions.ini (0x593740)
    void*         LocalProps;       // +0x004 SProperties* of mission_local.ini or missions_local.ini (0x593740)
    int           _008;             // +0x008 SString buf (ctor 0; dtor frees)
    int           _00c;             // +0x00c
    int           GameMode;         // +0x010 PzGameMode
    unsigned char _014;             // +0x014 ctor 0; PlaceMyUnits 0x596650 returns false when set
    unsigned char _015[3];
    int           Race;             // +0x018 0 German, 1 Allied, 2 Russian (STrainingMenu +0x284)
    int           Difficulty;       // +0x01c ctor 1; Training sets 0
    SString       MapName;          // +0x020 InitTutorialMode; GetMapName 0x592040 returns it for modes 1, 2, 4, 5, 6
    int           StartPrestige;    // +0x028 Init*Mode param (Training 1500); OnMapLoaded: != 0 -> market
    SArmyArray    Army;             // +0x02c the army the campaign carries between missions (BackupCampaignUnits 0x561110)
    int           Prestige;         // +0x038 Init*Mode param
    SArmyArray    MissionArmy;      // +0x03c the army placed at mission start (PlaceAllUnits 0x571c70 via 0x591e70): Army plus the units bought; the replay header holds it
    unsigned char _048[0x0d8 - 0x048]; // +0x048 12 x 0xc records (ctor eh-vector 0x590ea0)
    SString       MissionSection;   // +0x0d8 missions.ini section = the map MINA name ("Training camp", LoadMap 0x6201c0); LoadMap starts the mission at once when its size (+0xdc) is 0
    int           MenuToLoad;       // +0x0e0 PzMenuToLoad (getter 0x596600)
    int           MissionResult;    // +0x0e4 0x5920b0 (setter 0x597470): 0 running, 1 victory, 3 defeat (SGameView::Update 0x628430 end check); mission start runs the logic when 0
    unsigned char _0e8[0x0f8 - 0x0e8];
    int           _0f8;             // +0x0f8 saved with the replay header and the save
    int           _0fc;             // +0x0fc saved
    int           _100;             // +0x100 ctor -1
    unsigned char _104[0x12c - 0x104]; // +0x11c byte (campaign next-mission flag)
    SString       ReplayName;       // +0x12c "Replays/<name>" (StartReplay 0x597510); mission start plays it instead of the -packetplay file when its size (+0x130) != 0
    SCampaignObjective* Objectives; // +0x134 SDArray (0x1c each, 0x593ba0)
    int           ObjectiveCount;   // +0x138
    int           ObjectiveMax;     // +0x13c
    unsigned char PlayerStats[12][0xd8]; // +0x140 per player (ctor 0x5910c0 / 0x594470): +0x08 type, +0x0c 4 x 12 unit counters, +0xcc, +0xd0, +0xd4 (all saved)
    int           Score;            // +0xb60 (guessed) BackupCampaignUnits 0x561110
    unsigned char _b64[0xb68 - 0xb64];
    int           _b68;             // +0xb68 0x5974e0
    unsigned char _b6c[0xb70 - 0xb6c];
    int           _b70;             // +0xb70 0x5974e0
    unsigned char _b74[0xb80 - 0xb74];
    unsigned char _b80;             // +0xb80 ctor 2
    unsigned char _b81[3];
    int           _b84;             // +0xb84 ctor 0; saved
    unsigned char _b88;             // +0xb88 ctor 1 (Play -market sets 1)
    unsigned char _b89[3];

    SPanzersCampaign();                                    // 0x590ec0
    ~SPanzersCampaign();                                   // 0x591350 (caller deletes 0xb8c)

    // Mode set-up (SCampaign::*).
    void InitTutorialMode(const char* map, int race, int prestige);         // 0x594ab0 (3 dwords)
    void InitScenarioMode(const char* map, const char* section, int race, int prestige); // 0x5944d0 (4)
    void InitCampaignMode(const char* missionsIni, const char* mission, int race, int difficulty); // 0x592b20 (name guessed; us-02 uses 0x592d20)

    // Flow.
    int  GetMenuToLoad();                                  // 0x596600
    void OnMapLoaded();                                    // 0x594d50 (name guessed) "Map loaded": market or game view
    void SetMenuGameView();                                // 0x594e50 (name guessed) MenuToLoad = 2, +0xe4 = 0
    void LetMapDone();                                     // 0x594e00 needs MenuToLoad 2; modes 1-4 -> 3 (results), 5 -> 5 (main menu)
    void LetResultsDone();                                 // 0x594e70
    const char* GetMapName();                              // 0x592040
    bool IsTutorialMode();                                 // 0x594d40 GameMode == 5
    bool IsScenarioMode();                                 // 0x594d30 GameMode == 1
    bool IsMultiMode();                                    // 0x594d20 GameMode == 4
    bool HasMarket();                                      // 0x596610 [Market] key, true outside modes 1/2/3
    bool PlaceMyUnits();                                   // 0x596650 [PlaceMyUnits] key, true outside modes 1/2/3 (false when +0x14)
    int  GetMissionResult();                                 // 0x5920b0
    void SetMissionResult(int v);                            // 0x597470
    void LoadMissionProps();                               // 0x593740 maps|scenarios|multimaps/<section>/mission.ini, mission_local.ini
    void LoadObjectives();                                 // 0x593ba0 "Objective %d Text" ...
    void LoadSupportCounts();                              // 0x594140 World players +0x19c..+0x1ac (Cannonade, ReconPlane, ...)
    void GetMissionArmy(SArmyArray* out);                  // 0x591e70 out = copy of MissionArmy (+0x3c)
    const char* GetMissionCode();
    // Market hand-over (SMarket::OnAction 0x647d00, Start Mission -> Yes).
    void SetArmy(SArmyArray* army, bool refund);           // 0x5971b0 refund: sell back MissionArmy, then AddMissionArmyUnit per record; else plain copy
    bool AddMissionArmyUnit(const SUnitDef* def);          // 0x5916e0 buy one record (price check, unique ScriptIDs via 0x592640, deep copy, Prestige -= price)                          // (recompile) [<section>] "Mission code" (0x660570, inline in 0x5966a0 / 0x596e30)

    // Save path (the mission start writes SaveGames/TRNG-Start.save).
    bool SaveGame(const char* file, const char* title);    // 0x5966a0 (2)
    int  SaveGameStartMission(const char* name, const char* title); // 0x596e30 (2, cdecl-like: this in ECX unused)
    bool LoadGame(const char* file);                       // 0x594f70 (1)

    // Replay (docs/M3_REPLAY.md).
    void WriteReplayHeader(SStream* s);                    // 0x596fc0 'SAVE' 'v4pa' 3, map, mode, race, prestige, army, ...
    void ReadReplayHeader(SStream* s);                     // 0x595780 (SGameLogic::StartPacketPlayback)
    void StartReplay(const char* name, int len);           // 0x597510 (OnAction 0x494c2; not reachable in the shipped game)
};
PZ_HD_SIZE(SPanzersCampaign, kHdSizeSPanzersCampaign);
static_assert(offsetof(SPanzersCampaign, GameMode) == 0x10, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, MapName) == 0x20, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, StartPrestige) == 0x28, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, MissionArmy) == 0x3c, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, Army) == 0x2c, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, Objectives) == 0x134, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, PlayerStats) == 0x140, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, Score) == 0xb60, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, _b84) == 0xb84, "SPanzersCampaign layout");
static_assert(sizeof(SCampaignObjective) == 0x1c, "0x593ba0 stride 0x1c");
static_assert(offsetof(SPanzersCampaign, MissionSection) == 0xd8, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, Prestige) == 0x38, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, MenuToLoad) == 0xe0, "SPanzersCampaign layout");
static_assert(offsetof(SPanzersCampaign, ReplayName) == 0x12c, "SPanzersCampaign layout");

extern SPanzersCampaign* g_Campaign;   // HD 0x929a0c

// The tail of SWorld::CreateUnit 0x5e2da0: the campaign counts every new
// unit of a category (0x56d6d0) per player (PlayerStats +0x0c, saved).
int  UnitStatsCategory(const struct SUnit* u);            // 0x56d6d0
void CampaignUnitCreated(int unit, int player);           // 0x5e2da0 (inline)

} // namespace pz

#endif // PZ_CAMPAIGN_H
