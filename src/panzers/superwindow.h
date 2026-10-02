// src/panzers/superwindow.h
// SSuperWindow — Panzers' application window (HD PANZERS.exe).
//
// LAYOUT DECISION (P2-D):
// The HD vtables do NOT match the SWINE window classes this repo imported:
//   HD SWidget 31 slots (vftable 0x7f3770), SWINE SWidget 33
//   HD SWindow 39 slots (0x7f3e14),         SWINE SWindow 41
//   HD SDXWindow 46 slots (0x7f2964),       SWINE SDXWindow 45
// and the HD field order differs (HD SWidget keeps the four navigation
// pointers at +0x04..+0x10 and X/Y/W/H at +0x14..+0x20; SWINE has X at +0x04;
// HD SWindow has hWnd at +0x50, SWINE at +0x84 after a wchar_t ClassName[32]).
// SDXWindow/SWindow/SWidget belong to P2-B, so SSuperWindow derives from the
// SWINE SDXWindow for BEHAVIOUR ONLY, and Panzers' own fields (the HD range
// 0xe4..0x1c0 of the 0x1c0-byte object, operator new(0x1c0) at GameMain
// 0x64c870) are kept as a separate, size-checked block SSuperWindowData whose
// member offsets are the HD offsets minus 0xe4 (HD SDXWindow ends at 0xe4: its
// ctor 0x539c70 writes up to +0xe0).

#ifndef PANZERS_SUPERWINDOW_H
#define PANZERS_SUPERWINDOW_H

#include <stddef.h>
#include "dxwindow.h"

struct SProperties;
struct SMainMenu;
struct SAchimMenu;
struct SMainCreditMenu;

// HD SSuperWindow fields 0xe4..0x1c0. Names follow SWINE's SSuperWindow
// where the use matches; unknown ones keep the HD offset.
struct SSuperWindowData {
    SWidget*     GameView;          // 0xe4 (OnAction GV_*; per-frame Update)
    SMainMenu*   MainMenu;          // 0xe8 (LoadMainMenu 0x6583e0, new 0x13dc)
    SWidget*     Menu_ec;           // 0xec (action 0x534d2, new 0xbac 0x633820)
    SWidget*     Menu_f0;           // 0xf0 (per-frame Update first)
    SWidget*     Menu_f4;           // 0xf4
    SWidget*     Menu_f8;           // 0xf8
    SWidget*     Menu_fc;           // 0xfc
    SMainCreditMenu* CreditMenu;    // 0x100 (LoadMainCreditMenu 0x658300, new 0xd4 0x632fd0)
    SWidget*     BriefingMenu;      // 0x104 (actions 0x434d1..0x434d5)
    SWidget*     Menu_108;          // 0x108 (actions 0x534b1..0x534b4)
    SWidget*     Menu_10c;          // 0x10c (actions 0x424d1/0x424d2)
    SWidget*     MultiView;         // 0x110 (Play multiplayer path, new 0x25b4 0x63f2f0)
    SWidget*     Menu_114;          // 0x114
    SWidget*     TrainingCampMenu;  // 0x118 (action 0x4d4d4, new 0x288 0x633b30)
    SAchimMenu*  AchimMenu;         // 0x11c (action 0x4d4d8 quit screen, new 0xd4 0x632ee0)
    SProperties* PanzersIni;        // 0x120 (ctor: new SProperties(panzers.ini))
    int          ScreenWidth;      // 0x124 GetSystemMetrics(SM_CXSCREEN)
    int          ScreenHeight;     // 0x128 GetSystemMetrics(SM_CYSCREEN)
    bool         _12c;              // 0x12c
    bool         Initialized;       // 0x12d (set at the end of Initialize 0x657910)
    unsigned char _12e[0x02];
    unsigned char _130[0x48];       // 0x130..0x177 not touched on the boot path
    int          NewsCounter;       // 0x178
    int          _17c;              // 0x17c
    int          NewsFrame;         // 0x180 (news ticker box, 0x659020)
    int          NewsTextFrame;     // 0x184
    int          _188;              // 0x188
    int          _18c;              // 0x18c
    SWidget*     NewsWidget;        // 0x190
    int          MenuTopFrame;      // 0x194 menu/main_menu_top_hq.tga
    int          MenuBottomFrame;   // 0x198 menu/main_menu_bottom_hq.tga
    void*        MenuWorld;         // 0x19c maps/menu.map world (new 0x7538)
    void*        MenuCamera;        // 0x1a0 (new 0x318)
    float        MenuTime;          // 0x1a4
    float        MenuNextTick;      // 0x1a8
    int          SplashFrame;       // 0x1ac menu/splash_hq.tga while loading
    int          _1b0;              // 0x1b0
    void*        Bink;              // 0x1b4 HBINK of intro.bik
    void*        BinkBuffer;        // 0x1b8 HBINKBUFFER
    int          RootFrame;         // 0x1bc scaler frame, 1024x768 virtual
};
static_assert(sizeof(void*) == 4, "x86 only");
static_assert(sizeof(SSuperWindowData) == 0x1c0 - 0xe4, "SSuperWindow: HD object is 0x1c0 bytes (new 0x1c0 at 0x64c870)");
static_assert(offsetof(SSuperWindowData, PanzersIni) == 0x120 - 0xe4, "SSuperWindow layout");
static_assert(offsetof(SSuperWindowData, Initialized) == 0x12d - 0xe4, "SSuperWindow layout");
static_assert(offsetof(SSuperWindowData, NewsFrame) == 0x180 - 0xe4, "SSuperWindow layout");
static_assert(offsetof(SSuperWindowData, MenuTopFrame) == 0x194 - 0xe4, "SSuperWindow layout");
static_assert(offsetof(SSuperWindowData, Bink) == 0x1b4 - 0xe4, "SSuperWindow layout");
static_assert(offsetof(SSuperWindowData, RootFrame) == 0x1bc - 0xe4, "SSuperWindow layout");

struct SSuperWindow : SDXWindow, SSuperWindowData {
    SSuperWindow();                                   // 0x656d50
    ~SSuperWindow() override;                         // 0x6572a0 (deleting 0x6573e0)

    // HD vtable 0x80a174 overrides
    bool OnKeyDown(int key, bool repeat = false) override;          // +0x14 0x65b390
    void OnMouseDown(int button, int x, int y, int shift) override; // +0x24 0x65b3e0
    void OnSize(int w, int h) override;                             // +0x40 0x65b410
    bool OnAction(SWidget* source, int action, int param) override; // +0x44 0x659250
    SWidget* GetEventTarget(int x, int y, int* lx, int* ly) override; // +0x5c 0x657850
    int GetFrame() override;                                        // +0x68 0x6578f0
    void OnDestroy() override;                                      // +0x88 0x65ab50
    bool OnIdle() override;                                         // +0x90 0x65ae50

    int  Create(int icon, int cursor, const char* title);  // 0x657540
    void Play();                                           // 0x65b470
    void Initialize();                                     // 0x657910 (exported)
    void LoadMainMenu();                                   // 0x6583e0
    void LoadMainCreditMenu();                             // 0x658300
    void LoadMenuBackground(bool keepScene);               // 0x658690
    void UnloadMenuBackground();                           // 0x65b940
    void CloseBinkVideo();                                 // 0x65b830
    void ReleaseMultiView();                               // 0x65b8c0
    void ReleaseNews();                                    // 0x658fc0
    bool LoadBinkVideo(const char* filename);              // 0x657ef0
    bool BinkFrame();                                      // 0x65ba00

    // Recompile-only: set by gamemain.cpp from "-nointro" (not an HD switch;
    // HD would treat it as a map path, see SSettings::Initialize).
    static bool SkipIntro;
};

extern SSuperWindow* g_SuperWindow;   // HD 0x929cf4

#endif // PANZERS_SUPERWINDOW_H
