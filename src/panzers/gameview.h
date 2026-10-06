// src/panzers/gameview.h
// SGameView: the in-game view (map, camera, input, HUD owner). HD object
// 0x3e98 bytes, ctor 0x6181f0 (second ctor 0x618aa0), Create 0x619c90,
// LoadMap 0x6201c0, mission start 0x6281a0, per-frame Update 0x628430.
// SSuperWindow +0xe4 holds it (LoadNextCampaignView 0x658b10 case 2).
//
// As for the other Panzers widgets (superwindow.h, mainmenu.h), the HD
// SWidget/SDXWidget layout differs from the SWINE classes in src/window, so
// SGameView derives from the SWINE SDXWidget for behaviour and keeps the HD
// fields in SGameViewData at their HD offsets minus 0x5c. Do not assert the
// whole object size (CLAUDE.md "Layouts"); SGameViewData asserts its own.
//
// HD vtables:
//   SGameView vftable 0x80331c (31 SWidget slots), overrides:
//     +0x00 dtor            0x619430 (1)  [m3: end]
//     +0x14 OnKeyDown       0x622f50 (2)  [m3: play/end] 28 cases: scroll, groups, pause, Esc menu, cheats
//     +0x18 OnKeyUp         0x6246d0 (2)  [m3: play/end]
//     +0x24 OnMouseDown     0x624a70 (4)  [m3: mkt/play] loading screen click -> 0x47561 "Map loaded"
//     +0x28 OnMouseUp       0x6251f0 (4)  [m3: play] box select 0x5fc5b0
//     +0x2c OnMouseMove     0x6250e0 (3)  [m3: all phases]
//     +0x30 OnMouseWheel    0x625510 (4)
//     +0x34 OnMouseOver     0x6251a0 (0)
//     +0x38 OnMouseOut      0x625170 (0)
//     +0x44 OnAction        0x6216b0 (3)  [m3: mkt/play/end] in-game menu actions, "Are you sure?" boxes
//     +0x78 Update          0x628430 (0)  [m3: all phases] per frame: logic ticks, camera, HUD
//     all other slots are SDXWidget's.
//   SIGameViewCallback vftable 0x80339c at HD +0x58 (5 slots). SGameLogic
//   gets this pointer as its 3rd ctor argument (LoadMap 0x6201c0:
//   new SGameLogic(0, view +0x828, view +0x58)):
//     +0x00 0x622b30 (1) ShowMessageBox(text) (name guessed: new SMessageBox 0x3ec at +0x3e30)
//     +0x04 0x622c50 (2)
//     +0x08 0x625560 (1)
//     +0x0c 0x624730 (0) ResetClock (name guessed: +0x45c/+0x460 = now, ms)
//     +0x10 0x624770 (1)
//
// SGameView functions 0x6181f0..0x6291ff (address, insns, RET bytes, m3
// coverage phases, repo status; "-" = not run in the Training Camp trace):
//   0x6181f0 400  ctor                     mkt      | 0x618aa0 511 ctor 2            end
//   0x6195b0 334                           play     | 0x619b40..0x619b90             mkt
//   0x619c90 4162 Create                   mkt      | 0x61e1e0..0x61e310             mkt
//   0x61e500 132  CreateSubViewports       start    | 0x61e680 50 DestroySubViewports end
//   0x61e740 880  order dispatcher (5 args) play     | 0x61f460 209 (1 arg)           mkt
//   0x61f840 426  load-game variant of LoadMap (OnAction 0x494c1)
//   0x6201c0 255  LoadMap                  mkt      | 0x6206f0 214 (2)               play
//   0x620bc0 626  mouse move / camera drag start/play/end | 0x621540 119 (5)      start/play
//   0x625d80 322  SetPanelMode (1)         mkt/start/play | 0x626290 228            mkt
//   0x626c40 1019 unit panel (SUnitButton) mkt/play  | 0x6281a0 160 MissionStart   start
//   0x628430 4139 Update                   all phases
//   (99 functions in the range; the full list with coverage is
//   docs/re/m3_coverage.tsv, column m3agent = V / O / F.)
//
// SHARED HEADER (owner P0). Names and bodies: agent V (view, camera, Update,
// Create, draw), agent O (input: OnMouseDown/Up/Move, OnKeyDown, the order
// dispatcher 0x61e740), agent F (LoadMap, MissionStart, the "Map loaded" /
// GV_GAMEOVER flow). See docs/M3_INTERFACES.md.

#ifndef PANZERS_GAMEVIEW_H
#define PANZERS_GAMEVIEW_H

#include <stddef.h>
#include "dxwidget.h"

namespace pz { struct SWorld; struct SGameLogic; struct SIViewport; }

// Actions the game view sends to SSuperWindow::OnAction 0x659250.
enum PzGameViewAction {
    PZA_GV_MAP_LOADED  = 0x47561,   // loading screen clicked ("SSuperWindow::OnAction: Map loaded")
    PZA_GV_GAMEOVER    = 0x47562,   // End Mission confirmed / trigger "end scenario" ("GV_GAMEOVER ...")
    PZA_GV_ERROR       = 0x47563,   // ("GV_ERROR : What is?")
    PZA_GV_EXIT        = 0x47564,   // same as main-menu Exit
    PZA_GV_RESTART     = 0x47565,   // Restart Mission (OnAction: as 0x524d2)
};

// The callback interface SGameLogic calls on the view (HD vftable 0x80339c).
struct SIGameViewCallback {
    virtual void ShowMessageBox(const char* text) = 0;   // +0x00 0x622b30 (1) (name guessed)
    virtual void Slot_04(int p1, int p2) = 0;            // +0x04 0x622c50 (2)
    virtual void Slot_08(int p1) = 0;                    // +0x08 0x625560 (1)
    virtual void ResetClock() = 0;                       // +0x0c 0x624730 (0) (name guessed)
    virtual void Slot_10(int p1) = 0;                    // +0x10 0x624770 (1)
};

// HD fields of SGameView from +0x5c (after the callback vptr at +0x58).
// Offsets in the comments are HD object offsets.
struct SGameViewData {
    unsigned      KeyDownTime[256];  // +0x05c ms time a key went down (OnKeyDown 0x622f50 sets it, OnKeyUp 0x6246d0
                                     //        clears it); Update 0x628430 calls KeyScroll 0x62c470 for each held key.
                                     //        +0x420 is KeyDownTime[0xf1], not a field: the "+0x420" written by
                                     //        ShowMessageBox 0x622b30 is relative to its callback `this` (+0x58), so
                                     //        it is MouseMode +0x478 (== 1 -> HideSelectionBox 0x5ddb60, then 0).
    unsigned      ClockStart;        // +0x45c ms clock of the last Update (0x628430; mission start 0x6281a0 and
                                     //        ResetClock 0x624730 set it to now)
    unsigned      ClockNextTick;     // +0x460 next 50 ms logic tick (Update 0x628430 adds 0x32)
    unsigned      NetPingTime;       // +0x464 multiplayer: last 0x52ed70 call (every 2.2 s)
    pz::SIViewport* Viewport;        // +0x468 ctor: Gepard GetViewport(0); CreateSubViewports: subport 0
    int           Subport[3];        // +0x46c 3D view, top bar strip, bottom panel strip (-1 = none)
    int           MouseMode;         // +0x478 OnMouseDown: 1 box drag, 2/3 rotate drag, 4 command target,
                                     //        5/6 placement, 7 scroll drag (2/3/7 warp the cursor)
    int           PressX;            // +0x47c
    int           PressY;            // +0x480
    float         PressWorld[3];     // +0x484 terrain point under the press (modes 5/6)
    unsigned char _490[0x49c - 0x490];
    bool          Dragging;          // +0x49c
    unsigned char _49d[0x4a0 - 0x49d];
    float         ClickTime[2];      // +0x4a0 / +0x4a4 left / right press time (s, Timer 0x661800) for double clicks
                                     //        (0.75 s, 4 px; OnMouseDown 0x624a70 stores the float seconds)
    int           DoubleClickPick;   // +0x4a8
    int           MouseX;            // +0x4ac OnMouseMove 0x6250e0 (ctor 50); Update passes it to 0x620bc0
    int           MouseY;            // +0x4b0
    bool          MouseInside;       // +0x4b4 OnMouseOver 0x6251a0 / OnMouseOut 0x625170
    unsigned char _4b5[0x4b8 - 0x4b5];
    int           _4b8;              // +0x4b8 Create: board icon set "menu/panzers_interface_hq.tga" (H); LoadMap: 0x57fac0(+0x4b8, 0x13c, 7, 7)
    int           _4bc[2];           // +0x4bc ctor -1
    int           _4c4[4];           // +0x4c4 ctor -1; Update: unit panel icon sets by class
    unsigned char _4d4[0x584 - 0x4d4];
    int           MessageFrame[2];   // +0x584 Create: text frames (1010, 80) and (1010, 182)
    int           MessageText[8];    // +0x58c Create: 8 lines of text frames at x 1000
    int           MessageIcon[8];    // +0x5ac Create: 8 lines of sprite frames at x 1005
    int           ClockText;         // +0x5cc ctor -1; Update: mission time "%d:%02d"
    int           TimerText;         // +0x5d0 Create (Width / 2, 53); Update: countdown
    int           PauseText;         // +0x5d4 Create (Width - 8, 61); Update: "PAUSE"
    int           _5d8;              // +0x5d8 ctor -1
    unsigned char _5dc[0x828 - 0x5dc];
    int           LogicFrame828;     // +0x828 SGameLogic ctor p2 (a board frame)
    unsigned char _82c[0xc68 - 0x82c];
    void*         PanelWidget;       // +0xc68 SetPanelMode 0x625d80 hides it (vtbl +0x6c)
    unsigned char _c6c[0x2614 - 0xc6c];
    unsigned char SpecInfo[2][0x64]; // +0x2614 / +0x2678 SSpecInfoWidget (vftable 0x803110) (size guessed)
    unsigned char _26dc[0x3884 - 0x26dc];
    int           CommandMode;       // +0x3884
    unsigned char _3888[0x388c - 0x3888];
    int           LoadingFrame;      // +0x388c loading-screen text frame ("Click to continue")
    bool          LoadingScreen;     // +0x3890 set by LoadMap; OnMouseDown then sends 0x47561
    unsigned char _3891[0x3894 - 0x3891];
    int           SoundHandle3894;   // +0x3894 released at mission start (Concert +0x80)
    int           SoundHandle3898;   // +0x3898 released at mission start (Concert +0x0c)
    int           Modal;             // +0x389c
    unsigned char _38a0[0x3e30 - 0x38a0];
    SWidget*      MessageBox;        // +0x3e30 SMessageBox (0x3ec) of ShowMessageBox
    unsigned char _3e34[0x3e40 - 0x3e34];
    pz::SWorld*   World;             // +0x3e40 new 0x7538 in LoadMap
    pz::SGameLogic* Logic;           // +0x3e44 new 0x318 in LoadMap
    unsigned char _3e48[0x3e70 - 0x3e48]; // +0x3e48..+0x3e68 open dialogs (OnKeyDown Esc closes them)
    int           RotateUnit;        // +0x3e70 the unit of MouseMode 6
    void*         Widgets;           // +0x3e74 widget list
    int           WidgetCount;       // +0x3e78
    unsigned char _3e7c[0x3e80 - 0x3e7c];
    int           ViewState;         // +0x3e80 panel mode (SetPanelMode 0x625d80): 0 game, 1 full screen (loading), 2 cut-scene
    SWidget*      EndBox;            // +0x3e84 victory / defeat box (Update tail)
    SWidget*      ModalBox;          // +0x3e88 Update 0x628430 does nothing while it is set
    SWidget*      Box3e8c;           // +0x3e8c
    SWidget*      Box3e90;           // +0x3e90
    SWidget*      NetBox;            // +0x3e94 multiplayer "player left" box
};
static_assert(sizeof(SGameViewData) == 0x3e98 - 0x5c, "SGameView: HD object is 0x3e98 bytes (new 0x3e98 in 0x658b10)");
static_assert(offsetof(SGameViewData, ClockStart) == 0x45c - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, MouseMode) == 0x478 - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, MouseX) == 0x4ac - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, MessageIcon) == 0x5ac - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, LogicFrame828) == 0x828 - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, LoadingScreen) == 0x3890 - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, World) == 0x3e40 - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, ViewState) == 0x3e80 - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, Viewport) == 0x468 - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, PressWorld) == 0x484 - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, Dragging) == 0x49c - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, ClickTime) == 0x4a0 - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, DoubleClickPick) == 0x4a8 - 0x5c, "SGameView layout");
static_assert(offsetof(SGameViewData, MouseInside) == 0x4b4 - 0x5c, "SGameView layout");
static_assert(sizeof(((SGameViewData*)0)->ClickTime[0]) == 4, "SGameView: ClickTime is a float");
static_assert(offsetof(SGameViewData, RotateUnit) == 0x3e70 - 0x5c, "SGameView layout");

struct SGameView : SDXWidget, SIGameViewCallback, SGameViewData {
    SGameView();                                                     // 0x6181f0
    ~SGameView() override;                                           // vtbl +0x00 0x619430 (scalar deleting dtor)

    // SWidget slots (HD vftable 0x80331c).
    bool OnKeyDown(int key, bool repeat = false) override;           // +0x14 0x622f50
    bool OnKeyUp(int key) override;                                  // +0x18 0x6246d0
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x624a70
    void OnMouseUp(int button, int x, int y, int shift) override;    // +0x28 0x6251f0
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x6250e0
    void OnMouseWheel(int button, int x, int y, int delta) override; // +0x30 0x625510
    void OnMouseOver() override;                                     // +0x34 0x6251a0 MouseInside = 1
    void OnMouseOut() override;                                      // +0x38 0x625170 MouseInside = 0
    bool OnAction(SWidget* source, int action, int param) override;  // +0x44 0x6216b0
    void Update() override;                                          // +0x78 0x628430

    // SIGameViewCallback (HD vftable 0x80339c).
    void ShowMessageBox(const char* text) override;                  // 0x622b30
    void Slot_04(int p1, int p2) override;                           // 0x622c50
    void Slot_08(int p1) override;                                   // 0x625560 SetWindowScene(SIScene*) (name guessed)
    void ResetClock() override;                                      // 0x624730
    void Slot_10(int p1) override;                                   // 0x624770

    void Create();                                                   // 0x619c90
    void LoadMap();                                                  // 0x6201c0 (agent F)
    void MissionStart();                                             // 0x6281a0 (agent F)
    void CreateSubViewports();                                       // 0x61e500
    void DestroySubViewports();                                      // 0x61e680
    void SetPanelMode(int mode);                                     // 0x625d80
    void ShowLoadingBackdrop(bool plain);                            // 0x61f460 (gameview_loading.cpp)
    void ReleaseLoadingBackdrop();                                   // board +0x0c / +0x80 on +0x3898 / +0x3894 (0x6281a0)
    int  GetPanelMode();                                             // 0x61f450 (+0x3e80; SSuperWindow 0x65b410)
    void IssueOrder(int x, int y, int p3, int force, int command);   // 0x61e740 (agent O; name guessed) the order at a screen point -> packets.h builders
    void OnHotkey(int key);                                          // 0x6195b0 (agent O; name guessed) Settings.Hotkeys: pause, speed, command keys
    void OpenInGameMenu();                                           // (recompile) Esc in OnKeyDown 0x622f50 -> SInGameMenu
    // Camera input (agent V, gameview_view.cpp). OnKeyDown (O) stores
    // KeyDownTime[key] = NowMs() when it is 0; OnKeyUp 0x6246d0 (O) calls
    // KeyScroll(NowMs(), key) once more when World is set, then clears it.
    // OnMouseMove 0x6250e0 (O) stores MouseX / MouseY when World is set.
    void KeyScroll(unsigned now, int key);                           // 0x62c470 (2)
    void MouseCamera(int x, int y, int ms);                          // 0x620bc0 (3) mouse modes + edge scroll, per frame
    void HoverCursor(int x, int y, int ms);                          // 0x621540 (5) (not lifted) cursor over unit / terrain
    static unsigned NowMs();                                         // ftol(0x661800() * 1000.0)
    int& HdInt(int offset);                                          // recompile: an unnamed HD field

    // Recompile state (not HD).
    int  SubportRect[3][4] = {};     // CreateSubViewports rects until the viewport subports exist (agent E)
    bool FramesCreated = false;      // Create made the message / timer / pause frames

    // Recompile skeleton state (not HD): the in-game menu widget and the
    // 20 Hz clock of the skeleton tick.
    struct SInGameMenu* InGameMenu = nullptr;
    double NextTick = 0;
};

#endif // PANZERS_GAMEVIEW_H
