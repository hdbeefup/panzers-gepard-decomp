// src/panzers/market.h
// SMarket: the Headquarters screen (buy units with prestige before the
// mission). HD object 0x25b4 bytes, ctor 0x63f2f0, Create 0x6407d0 (3154
// insns), dtor 0x63fa10 (deleting 0x6400b0), LoadUnitInfo 0x6450c0 (2908),
// SaveArmy 0x64a180. SSuperWindow +0x110 holds it (the repo's "MultiView";
// HD Play -market also puts an SMarket there).
//
// Flow: LoadNextCampaignView 0x658b10 case 1 moves the mission world aside
// (World 0x929a50 -> 0x929f18, SGameLogic 0x8f2078 -> 0x929f1c, Scene
// 0x929a54 -> 0x929f20, then nulls them) so the market's 3D preview has its
// own scene; SSuperWindow::ReleaseMultiView 0x65b8c0 deletes the market and
// moves them back. OnAction 0x647d00: "Start Mission" -> "Are you sure?" ->
// sends 0x4d542 (SSuperWindow: ReleaseMultiView, SetMenuGameView 0x594e50,
// GameView SetVisible(1), MissionStart 0x6281a0); cancel sends 0x4d541
// (ReleaseMultiView, delete the game view, LoadMainMenu).
//
// HD vtables (own slots only):
//   SMarket               0x808638: +0x00 0x6400b0 [m3: start], +0x44 OnAction 0x647d00 (3)
//                         [m3: mkt/start], +0x78 Update 0x64b380 [m3: mkt/start]
//   SMarketListBox        0x808434: +0x00 0x640110, +0x14 0x648e30, +0x24 0x648f00 [m3: mkt],
//                         +0x2c 0x6490a0, +0x30 0x649130, +0x38 0x649110, +0x44 0x647c50,
//                         +0x78 0x64b4e0 (base SGenericListBox<SMarketListBoxItem> 0x8083b4)
//   SMarketSlotButton     0x8084b4: +0x00 0x640140, +0x24 0x649080
//   SMarketCategoryButton 0x808534 (SButton, 32): +0x00 0x6400e0, +0x7c redraw 0x64b220 [m3: mkt]
//   SMarketView           0x8085b8: +0x00 0x6401a0, +0x78 Update 0x64b890 [m3: mkt/start] (3D preview)
//   SMarketVehicleMenu    0x8086b8: +0x00 0x640170, +0x44 OnAction 0x648c20
//
// SHARED HEADER (owner P0; body: agent H, src/panzers/market.cpp).

#ifndef PANZERS_MARKET_H
#define PANZERS_MARKET_H

#include "mainmenu.h"

enum PzMarketAction {
    PZA_MARKET_CANCEL = 0x4d541,
    PZA_MARKET_START  = 0x4d542,
};

struct SMarket : SRightMenu {
    // Skeleton stand-in: "Start Mission" (and "Cancel") only. The HD screen
    // (army / warehouse list boxes, the 3D view, prestige) is Create 0x6407d0.
    SComplexButton StartMission;
    SComplexButton CancelButton;

    SMarket();                                                       // 0x63f2f0
    ~SMarket() override;                                             // 0x63fa10
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x647d00
    void Update() override;                                          // +0x78 0x64b380
    void Create();                                                   // 0x6407d0
    void LoadUnitInfo();                                             // 0x6450c0
    void SaveArmy(int p1);                                           // 0x64a180 (1)
};

#endif // PANZERS_MARKET_H
