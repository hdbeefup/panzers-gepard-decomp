// src/game/skirmish_state.h
// The offline subset of HD's SMulti (DAT_008f1a74) that the single-player
// Skirmish runs on: the room (src/panzers/skirmish.cpp) fills it, the game
// (skirmish_game.cpp) reads it. HD SMulti offsets in the comments; see
// src/panzers/skirmish.h and docs/m5/sk.md. OWNER: agent SK (M5).

#ifndef PZ_SKIRMISH_STATE_H
#define PZ_SKIRMISH_STATE_H

#include "campaign.h"

namespace pz { struct SGameLogic; struct SWorld; }

// One SMulti slot (HD +0x4c5c + slot * 0x5c).
struct PzSkirmishSlot {
    bool          Connected;     // +0x00 (+0x4c5c) a host sits in the slot
    unsigned char Status;        // +0x15 (+0x4c71) 0 Open, 1 Closed, 2 human, 3 Computer
    unsigned char Ready;         // +0x16 (+0x4c72) 0 / 1 (2: in the market)
    char          Name[0x32];    // +0x17 (+0x4c73) the player name (strncpy 0x32)
    int           Nation;        // +0x4c (+0x4ca8) 0 German, 1 Allied, 2 Russian (drop list order)
    int           Team;          // +0x54 (+0x4cb0) 1 / 2 (0x51ed30)
    int           Changed;       // +0x58 (+0x4cb4) set by 0x51e2b0
};

// The offline SMulti subset.
struct PzSkirmish {
    PzSkirmishSlot Slots[8];
    pz::SArmyArray Armies[8];    // +0x4820 + slot * 0x34: the army each host uploaded (the local player's from LoadCurrentArmy)
    int  GameType;               // +0x4a38 (19000) 0 Team Match, 1 Domination, 2 Assault (3 coop: not in the room)
    int  PrestigeLimit;          // +0x4a34 atoi of the Prestige limit drop list
    int  GameAge;                // +0x4a3c 0 Early, 1 Late
    bool Locked;                 // +0x4a30 Lock Settings
    bool ArmiesChanged;          // +0x4a1d a nation changed: reload the army list
    bool OnePlayer;              // +0x4a1c set by 0x521d60 when fewer than 2 hosts are connected
    char MapName[0x104];         // +0x4a40 the map list's text
    char MapPath[0x104];         // +0x4b44 the map list's file name
    unsigned MapCrc;             // +0x4c4c 0x65ec50(path, 0)
    bool Started;                // +0x4774 Start Game (0x521d60)
    bool Skirmish;               // +0x512c set by SSkirmishChatRoomMenu::Create
    int  MySlot;                 // +0x4790 (Server 0x51e950: 0)

    PzSkirmish();                                    // 0x51dcf0 + Server 0x51e950 (the parts kept)
    ~PzSkirmish();
    void SetReady(bool reset);                       // 0x51e2b0
    void SetNotReady(bool market);                   // 0x51e270
    void SetNation(int slot, int nation);            // 0x51e3a0
    void ChangeTeam(int team);                       // 0x51e3c0 (server: 0x51ed30)
    void Start();                                    // 0x521d60
    int  HostCount() const;                          // the connected count of 0x521d60
};

// HD DAT_008f1a74 for the skirmish: non-null from the room to the end of
// the game. The single-player paths keep testing their own "no SMulti".
extern PzSkirmish* g_Skirmish;

// The game side (skirmish_game.cpp).
void PzSkirmishSetupPlayers(pz::SWorld* w);                      // SGameLogic ctor 0x55e440, the SMulti part
bool PzSkirmishPlaceAllUnits(pz::SGameLogic* logic);             // SGameLogic::PlaceAllUnits 0x571c70, the SMulti part
void PzSkirmishCheckPlayers(pz::SGameLogic* logic, int frame);   // SGameLogic::Refresh 0x576d80, the lost / victory check
bool PzSkirmishTeammateAlive(pz::SWorld* w);                     // SGameView::Update 0x628430, the SMulti end check
// The Computer army of a nation (0x571c70's tables).
void PzSkirmishBuildAIArmy(pz::SArmyArray* out, int nation, int age, int limit, int variant);

#endif // PZ_SKIRMISH_STATE_H
