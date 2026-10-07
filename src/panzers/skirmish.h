// src/panzers/skirmish.h
// The single-player Skirmish (M5-SK, docs/m5/sk.md): the staging room
// SSkirmishChatRoomMenu and the offline subset of HD's SMulti it runs on.
//
// HD runs the skirmish room on a real SMulti server (DAT_008f1a74, 0x5130
// bytes, ctor 0x51dcf0, Server 0x51e950) with the local player in slot 0
// and Computer players in other slots; nothing is ever sent because no other
// host is connected. The recompile does not lift SNetworkUDP / SMulti: it
// keeps the SMulti data the room and the game read in PzSkirmish (HD SMulti
// offsets in the comments) and turns the SMulti calls of the room into local
// operations on it.
//
//   SSkirmishChatRoomMenu  vftable 0x809f50, 0x1244 bytes (new in
//                          SSuperWindow::LoadSkirmishChatRoomView 0x658e70;
//                          ctor 0x652d20 over SFullScreenMenu 0x64bad0,
//                          Create 0x653250, dtor 0x652ef0 / deleting
//                          0x653170). Own vtable slots: +0x14 OnKeyDown
//                          0x655a50 (returns 1), +0x44 OnAction 0x654df0
//                          (the symbols call it "PlayerNameDropList"), +0x50
//                          OnTimer 0x655a60, +0x6c SetVisible 0x656110, +0x78
//                          Update 0x656210.
//
// OWNER: agent SK (M5).

#ifndef PANZERS_SKIRMISH_H
#define PANZERS_SKIRMISH_H

#include "market.h"
#include "pzwidgets.h"
#include "hudwidgets.h"
#include "campaign.h"
#include "skirmish_state.h"   // PzSkirmish, g_Skirmish (src/game)

// Actions the room sends to the super window (SSuperWindow::OnAction 0x659250).
enum PzSkirmishAction {
    PZA_SKIRMISH_START  = 0x534b1,   // countdown over (0x655a60) -> LetChatroomDone, game view
    PZA_SKIRMISH_CANCEL = 0x534b2,   // Cancel -> main menu, SMulti deleted
    PZA_SKIRMISH_EDIT   = 0x534b3,   // Edit army -> the market (army making)
    PZA_SKIRMISH_NEW    = 0x534b4,   // New army -> the market (army making)
};

struct SSkirmishChatRoomMenu : SFullScreenMenu {
    SComplexButton Team1;              // +0x05c "Team 1" / "Attack"
    SComplexButton Team2;              // +0x0d0 "Team 2" / "Defense"
    SDXWidget      Panel1;             // +0x144 slots 0..3
    SDXWidget      Panel2;             // +0x19c slots 4..7
    int            MiniMapFrame;       // +0x1f4 (-1)
    SComplexButton StartGame;          // +0x1f8
    SComplexButton ImReady;            // +0x26c
    SComplexButton Cancel;             // +0x2e0
    SComplexButton LockSettings;       // +0x354
    SComplexButton NewArmy;            // +0x3c8 "New"
    SComplexButton EditArmy;           // +0x43c "Edit"
    SComplexButton DeleteArmy;         // +0x4b0 "Delete"
    int            CountdownTimer;     // +0x524 (-1)
    int            Countdown;          // +0x528 (-1)
    bool           MeReady;            // +0x52c
    pz::SMessageBox* NoMapsBox;        // +0x530 "You have no map files..."
    pz::SMessageBox* DeleteBox;        // +0x534 "Are you sure, do you want to delete this army?"
    bool           ArmiesEnabled;      // +0x538 (1)
    pz::SListBox   Armies;             // +0x53c
    pz::SListBox   Maps;               // +0x670
    int            ReadyFrames[8];     // +0x7a4
    pz::SDropList  Nations[8];         // +0x7c4
    pz::SDropList  Status[8];          // +0xc24
    int            PrestigeText;       // +0x1084
    int            GameTypeText;       // +0x1088
    int            GameAgeText;        // +0x108c
    pz::SDropList  Prestige;           // +0x1090
    pz::SDropList  GameType;           // +0x111c
    pz::SDropList  GameAge;            // +0x11a8
    int            MapLabel;           // +0x1234 "Map:"
    int            MapNameText;        // +0x1238
    unsigned char  Race;               // +0x123c the local player's nation
    int            SelArmy;            // +0x1240 (-1)
    char           ShownMap[0x104];    // (recompile) the map whose preview is up
    int            MiniMapFont;        // (recompile) menu/minimap_hq.tga (HD releases it at once)
    int            CompassFont;        // (recompile) menu/minimap_compass_hq.tga
    char           ArmyName[0x80];     // (recompile) the campaign's army name (HD SetArmyName 0x597150 / GetArmyName 0x591e00)

    SSkirmishChatRoomMenu();                                         // 0x652d20
    ~SSkirmishChatRoomMenu() override;                               // 0x652ef0
    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x655a50
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x654df0
    void OnTimer(int id, unsigned int elapsed) override;             // +0x50 0x655a60
    void SetVisible(bool visible) override;                          // +0x6c 0x656110
    void Update() override;                                          // +0x78 0x656210

    void Create();                                                   // 0x653250
    bool IsMap(const char* path);                                    // 0x653ee0
    void LoadArmyNames();                                            // 0x654030
    void LoadCurrentArmy();                                          // 0x6547b0
    void LoadMiniMap(const char* path);                              // 0x654b30
    void ReleaseMiniMap();                                           // (the board +0x0c / +0x80 parts of 0x654b30)
    void FillMapList();                                              // 0x655ab0
    void FillPrestigeList(int age);                                  // 0x6560a0
    void EnableControls(bool enable, bool buttons);                  // 0x656120
};

struct SSuperWindow;
// SSuperWindow::OnAction 0x659250, the skirmish cases (0x534d1, 0x534b1..
// 0x534b4) and the end of a skirmish game; superwindow_sk.cpp.
bool PzSkirmishAction(SSuperWindow* sw, int action, int param);

#endif // PANZERS_SKIRMISH_H
