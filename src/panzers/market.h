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
// Yes: the army goes to the campaign (0x5971b0) and 0x4d542 is sent
// (SSuperWindow: ReleaseMultiView, SetMenuGameView 0x594e50, GameView
// SetVisible(1), MissionStart 0x6281a0). 0x4d541 cancels (the multiplayer
// "Leave Market" box; single player has no cancel).
//
// HD vtables (own slots only):
//   SMarket               0x808638: +0x00 0x6400b0 [m3: start], +0x44 OnAction 0x647d00 (3)
//                         [m3: mkt/start], +0x78 Update 0x64b380 [m3: mkt/start]
//   SFullScreenMenu       0x808ed4 (base, 0x5c bytes): ctor 0x64bad0, dtor 0x64bb50, Create 0x64bd50
//   SMarketListBox        0x808434: +0x00 0x640110, +0x14 OnKeyDown 0x648e30, +0x24 OnMouseDown
//                         0x648f00 [m3: mkt], +0x2c OnMouseMove 0x6490a0, +0x30 OnMouseWheel
//                         0x649130, +0x38 OnMouseOut 0x649110, +0x44 OnAction 0x647c50,
//                         +0x78 Update 0x64b4e0 (base SGenericListBox<SMarketListBoxItem> 0x8083b4)
//   SMarketSlotButton     0x8084b4: +0x00 0x640140, +0x24 OnMouseDown 0x649080
//   SMarketCategoryButton 0x808534 (SButton, 32): +0x00 0x6400e0, +0x7c redraw 0x64b220 [m3: mkt]
//   SMarketView           0x8085b8: +0x00 0x6401a0, +0x78 Update 0x64b890 [m3: mkt/start] (3D preview)
//   SMarketVehicleMenu    0x8086b8: +0x00 0x640170, +0x44 OnAction 0x648c20
//
// SHARED HEADER (owner P0; names and bodies: agent H, src/panzers/market.cpp).

#ifndef PANZERS_MARKET_H
#define PANZERS_MARKET_H

#include "mainmenu.h"
#include "hudwidgets.h"
#include "pzunitregistry.h"
#include "hud.h"

namespace pz { struct SWorld; struct SGameLogic; }

enum PzMarketAction {
    PZA_MARKET_CANCEL = 0x4d541,
    PZA_MARKET_START  = 0x4d542,
    PZA_MARKET_SLOT   = 0x53421,   // SMarketSlotButton LMB (0x649080)
};

// HD SFullScreenMenu (0x5c bytes): the full-screen menu frame.
struct SFullScreenMenu : SDXWidget {
    int Texture;                     // +0x58 menu/fullscreen_menu_hq.tga

    SFullScreenMenu();                                               // 0x64bad0
    ~SFullScreenMenu() override;                                     // 0x64bb50
    void Create(const char* title);                                  // 0x64bd50
};

// HD SMarketListBoxItem (0x24 bytes).
struct SMarketListBoxItem {
    SString      Name;       // +0x00 shown name (units.ini)
    SString      FileName;   // +0x08 registry unit name
    unsigned int Color;      // +0x10 price colour (green / red)
    int          Font;       // +0x14 icon font
    int          Glyph;      // +0x18 icon glyph (MarketPicture)
    int          Price;      // +0x1c
    int          Data;       // +0x20 army index (army list)
};

// HD SMarketListBox (SGenericListBox<SMarketListBoxItem>).
struct SMarketListBox : SDXWidget {
    int  LineHeight;        // 0x58
    int  VisibleLines;      // 0x5c
    int  TopIndex;          // 0x60
    int  ColumnWidth;       // 0x64
    int  Columns;           // 0x68
    bool Selectable;        // 0x6c
    int  CurSel;            // 0x70
    int  Hover;             // 0x74
    float LastClickTime;    // 0x78
    bool HasScrollbar;      // 0x7c
    pz::SScrollbar Scrollbar; // 0x80
    SMarketListBoxItem* Items; // 0x114 SDArray {array, size, max}
    int  ItemCount;         // 0x118
    int  ItemMax;           // 0x11c
    int  Font;              // 0x120
    int* IconFrames[4];     // 0x124 per column: sprite (icon)
    int* NameFrames[4];     // 0x134 per column: fixed text (name)
    int* PriceFrames[4];    // 0x144 per column: text (price)
    int* BoxFrames[4];      // 0x154 per column: box (cell)
    int  HighlightFrame;    // 0x164 controls glyph 0x42
    int  Align;             // 0x168
    unsigned int SelColor;  // 0x16c (0x80808080)

    SMarketListBox();                                                // 0x63f240
    ~SMarketListBox() override;                                      // 0x640110

    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x648e30
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x648f00
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x6490a0
    void OnMouseWheel(int delta, int x, int y, int keys) override;   // +0x30 0x649130
    void OnMouseOut() override;                                      // +0x38 0x649110
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x647c50
    void Update() override;                                          // +0x78 0x64b4e0

    void Create(int font, int lines, int columns, bool background, bool scrollbars); // 0x644130
    int  AddItem(const char* name, const char* file, int font, int glyph,
                 unsigned int color, int price);                     // 0x6402f0
    void ResetContent(int size);                                     // 0x640620
    int  SetCurSel(int index);                                       // 0x64ae80
    int  SetTopIndex(int index);                                     // 0x64b310
    void EnsureVisible(int index);                                   // 0x644b80
    int  GetPrice(int index) const;                                  // 0x644d40
    const char* GetFileName(int index) const;                        // 0x644c70

private:
    void GenericCreate(int lineHeight, int lines, int columnWidth, int columns,
                       bool selectable, bool background, bool scrollbars); // 0x640720
    void FreeItems();
};

// HD SMarketCategoryButton (SButton + count text).
struct SMarketCategoryButton : pz::SButton {
    int  CountFrame;         // +0x74
    bool Full;               // +0x78 red count

    SMarketCategoryButton();                                         // 0x63f6f0
    void Redraw() override;                                          // +0x7c 0x64b220
};

// HD SMarketSlotButton (0x68 bytes): an equipment slot of the unit.
struct SMarketSlotButton : SDXWidget {
    int Font;                // +0x58 headquarters font
    int SpriteFrame;         // +0x5c icon
    int OverlayFrame;        // +0x60
    int PriceFrame;          // +0x64

    SMarketSlotButton();                                             // 0x63f750
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x649080
};

// HD SMarketView: the 3D preview, its own SWorld on the empty map
// "vasarlomenu" (0x5f5280) drawn in the primary viewport's subport 1.
struct SMarketView : SDXWidget {
    pz::SWorld*     World = nullptr;  // +0x58 new 0x7538 (0x5d2f90(0))
    pz::SGameLogic* Logic = nullptr;  // +0x5c (deleted by the dtor; never created in single player)
    unsigned        TickLast = 0;     // +0x60 ms of the last Update
    unsigned        TickNext = 0;     // +0x64 ms of the next 50 ms world tick

    SMarketView();
    ~SMarketView() override;                                         // 0x6401a0
    void Update() override;                                          // +0x78 0x64b890
    void Create();                                                   // 0x6449a0
};

// HD army record (0x8c bytes): the unit definition plus the vehicle of a
// crew squad and the price.
struct SMarketArmyItem {
    pz::SUnitDef Def;        // +0x00 (ClassName: the unit, or "XX Crew Squad")
    SString      Vehicle;    // +0x80 the vehicle a crew squad drives
    int          Price;      // +0x88
};

struct SMarket : SFullScreenMenu {
    int  Mode;                          // +0x05c 0 single player, 1 multiplayer deck
    SComplexButton BuyButton;           // +0x0e4 Buy / Sell
    SComplexButton StartButton;         // +0x158 Start Mission
    SComplexButton ChangeVehicle;       // +0x240
    SMarketView    View;                // +0x2b4
    SDXWidget      CostBar;             // +0x31c
    int  Race;                          // +0x37c
    int  CostFrame;                     // +0x374
    int  PrestigeFrame;                 // +0x378
    int  Cost;                          // +0x380
    SString SelUnit;                    // +0x384
    SString SelVehicle;                 // +0x38c
    int  HqFont;                        // +0x394 menu/headquarters_hq.tga
    int  InterfaceFont;                 // +0x398 menu/panzers_interface_hq.tga
    int  UnitFont[3];                   // +0x39c german, +0x3a0 allied, +0x3a4 russian
    SDXWidget InfoPanel;                // +0x3ac
    SUnitButton UnitState;              // +0x404 the preview unit's state (0x61e310 / 0x626c40)
    int  ArmorFrames[4];                // +0x4c4..+0x4d0
    int  HeaderSprite;                  // +0x4d4
    int  HeaderFrame;                   // +0x4d8
    int  StarFrames[4];                 // +0x4dc
    SSpecInfoWidget HpIcon;                   // +0x4ec
    int  HpFrame;                       // +0x54c
    SSpecInfoWidget AmmoIcon;                 // +0x550
    SSpecInfoWidget CargoIcon;                // +0x5b0
    int  AmmoFrame;                     // +0x610
    int  CargoFrame;                    // +0x614
    SSpecInfoWidget XpIcon;                   // +0x618
    int  XpFrame;                       // +0x678
    SSpecInfoWidget ThermoIcon;               // +0x67c
    int  ThermoFrame;                   // +0x6dc
    SSpecInfoWidget InfoRows[4];              // +0x6e0 stride 0x60: speed, then the weapons
    int  InfoRowFrames[4];              // +0x860
    SDXWidget SlotPanel;                // +0x870
    SMarketSlotButton Slots[8];         // +0x8c8 stride 0x68
    int  Slot1;                         // +0xcd8 equipment chosen
    int  Slot2;                         // +0xcdc
    int  SlotsAllowed;                  // +0xce0
    SMarketArmyItem* Army;              // +0xcf4 SDArray {array, size, max}
    int  ArmyCount;                     // +0xcf8
    int  ArmyMax;                       // +0xcfc
    int  ArmyCategory;                  // +0xd18 shown army tab
    int  CategoryCount[5];              // +0xd1c all, infantry, tanks, support, artillery
    SMarketListBox ArmyList;            // +0xd30
    SMarketCategoryButton ArmyTabs[5];  // +0xea0
    int  WarehouseCategory;             // +0x110c
    SMarketListBox Warehouse;           // +0x1110
    SMarketCategoryButton WarehouseTabs[4]; // +0x1280
    int  Prestige;                      // +0x1474
    int  StartPrestige;                 // +0x1478
    int  SelArmy;                       // +0x147c
    pz::SMessageBox StartBox;           // +0x1c84 "Start Mission / Are you sure?"
    pz::STextBox Description;           // +0x2474
    int  MaxAll;                        // +0x2464 SUnitRegistry +0x114
    int  MaxTanks;                      // +0x2468 +0x118
    int  MaxArtillery;                  // +0x246c +0x11c
    int  MaxSupport;                    // +0x2470 +0x120
    int  Subport[2] = { -1, -1 };       // +0x245c whole screen (board), +0x2460 the 3D preview (scene)

    SMarket();                                                       // 0x63f2f0
    ~SMarket() override;                                             // 0x63fa10
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x647d00
    void Update() override;                                          // +0x78 0x64b380
    void Create();                                                   // 0x6407d0
    void LoadUnitInfo();                                             // 0x6450c0
    void SaveArmy(int p1);                                           // 0x64a180 (1)

    void FillArmyList();                                             // 0x6492d0
    void FillWarehouse();                                            // 0x6499a0
    void Buy();                                                      // 0x6403b0
    void Sell();                                                     // 0x64ad10
    void SelectWarehouse(int index);                                 // 0x64ab50
    void SelectArmy(int index);                                      // 0x64a950
    void ColorPrices();                                              // 0x64b940
    void SetArmyCategory(int category);                              // 0x64b0a0
    static int UnitCategory(const char* unit);                       // 0x644f00
    int  IconFont(const char* unit) const;                           // 0x644d80
    static bool Available(const pz::SPUnit* proto);                  // 0x644fc0

private:
    int  AddArmyItem();                                              // 0x640210
    void RemoveArmyItem(int index);                                  // 0x64a080
};

#endif // PANZERS_MARKET_H
