// src/panzers/superwindow.cpp
// SSuperWindow — Panzers' application window (HD PANZERS.exe).
//
// Boot order (HD): ctor 0x656d50 -> Create 0x657540 -> Play 0x65b470 ->
//   intro (LoadBinkVideo 0x657ef0, per-frame BinkFrame 0x65ba00 from OnIdle
//   0x65ae50) -> Initialize 0x657910 -> LoadMainMenu 0x6583e0 -> SWindow::Run
//   0x544db0.
// Engine calls go to the SWINE-shared SDXWindow / SIGepard / SIBoard /
// SIConcert. Where the HD interface has no SWINE counterpart the call is
// marked "TEMP until P2-x merge" and either mapped (pzboard.h) or skipped
// with a log line.

#include <windows.h>
#include <math.h>
#include <string.h>
#include "superwindow.h"
#include "settings.h"
#include "mainmenu.h"
#include "credits.h"
#include "optionsmenu.h"
#include "pzboard.h"
#include "properties.h"
#include "stream.h"
#include "logger.h"
#include "timer.h"
#include "igepard.h"
#include "iconcert.h"
#include "gettext.h"
#include "options.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/iviewport.h"
#include "pz/ipixie.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "pzunitregistry.h"
#include "m3common.h"

#include "milesconcert.h"
#if PANZERS_HAVE_BINK
#include "bink.h"   // P2-C: src/panzers/bink.*
#include "mods.h"
#include "mod_widescreen.h"
static SBinkVideo s_Intro;   // handles mirrored into SSuperWindowData::Bink/BinkBuffer
#endif

static SIPanzersConcert* PzConcert()
{
    return dynamic_cast<SIPanzersConcert*>(Concert);
}

SSuperWindow* g_SuperWindow = nullptr;   // HD 0x929cf4
bool SSuperWindow::SkipIntro = false;

// Logged stubs for everything past a menu click (src/stubs/stub_panzers.cpp).
void PzStub_CreateHostFromCommandLine();           // 0x6576f0
void PzStub_ConnectToHostFromCommandLine(const char* host); // 0x657460
void PzStub_StartCampaignFromCommandLine();        // -market path in Play
void PzStub_LoadMapFromCommandLine(const char* map); // 0x5944d0 path in Play
bool PzStub_CheckCDKey(const char* key);           // SSettings::CHECKCDKEY 0x64d390
void PzStub_LoadMultiPreMenu();                    // 0x658a30
void PzStub_LoadChatRoomView();                    // 0x658050
void PzStub_LoadMenuWorld(SSuperWindow* sw);       // maps/menu.map world in 0x658690 (-menu3d off)
void PzStub_SuperWindowAction(int action);         // OnAction cases beyond the main menu
const char* PzStub_GetVersionString();             // SVersion::GetVersionString 0x65c070

// M3 (superwindow_m3.cpp): the Training Camp flow, only with -m3 / PZ_M3=1.
bool SuperWindowM3Action(SSuperWindow* sw, int action, int param);
void M3RestoreMovedWorld();
void M3OnMainMenu(SSuperWindow* sw);

static pz::SUnitRegistry* s_UnitRegistry = nullptr;   // HD 0x929a4c (new 0x124)

// ---------------------------------------------------------------------------

// PANZERS 0x656d50
SSuperWindow::SSuperWindow()
{
    // HD zeroes the menu pointers individually; all of SSuperWindowData that
    // the ctor does not set to -1 starts zero.
    memset(static_cast<SSuperWindowData*>(this), 0, sizeof(SSuperWindowData));
    MenuTopFrame = -1;      // param_1[0x65]
    MenuBottomFrame = -1;   // param_1[0x66]
    // DAT_008f1a74 (SMulti*) = 0: multiplayer is out of scope.

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(Settings.GetIniPath(), GetFileExInfoStandard, &fad))
        Logger.g->Panic("SSuperWindow::SSuperWindow: %s was not found", Settings.GetIniPath());

    PanzersIni = new SProperties(Settings.GetIniPath(), true);     // 0x65fe80(path, 1)
    Logger.g->SetLogLevel(PanzersIni->GetInt("Debug", "debug level", 0));   // 0x65ca80
#if PANZERS_MOD_WIDESCREEN
    ModWidescreenInit(PanzersIni);
#endif

    const char* home = PanzersIni->GetString("Paths", "Home", "");
    SString homeCopy;
    homeCopy = home ? home : "";
    const char* search = PanzersIni->GetString("Paths", "Search", nullptr);
    SString searchCopy;
    searchCopy = (search && *search) ? search : "";

    Logger.g->Log(0, "Search Path: %s", searchCopy.buf ? searchCopy.buf : "");
    Logger.g->Log(0, "Home Path: %s", homeCopy.buf ? homeCopy.buf : "");
    FileSystem.SetSearchPath(searchCopy.buf ? searchCopy.buf : "", false);   // 0x65df90
    FileSystem.SetHomePath(homeCopy.buf ? homeCopy.buf : "");                // 0x65fa30
    if (searchCopy.buf) { delete[] searchCopy.buf; searchCopy.buf = nullptr; }
    if (homeCopy.buf) { delete[] homeCopy.buf; homeCopy.buf = nullptr; }

    Settings.Initialize(__argc, __argv);                           // 0x64e330
    // TEMP until the SWINE widgets stop reading the SWINE SOptions
    // (Options->GetKeyboardMode() in textbutton/checkbox/...): HD has no such
    // object; a read-only SWINE SOptions keeps those widgets from crashing.
    if (!Options)
        Options = new SOptions();

    // [Locale] Locale -> 0x52c320 into the locale SString (default "en").
    // TEMP until P2-A merge: no locale global in core yet; logged only.
    Logger.g->Log(0, "Locale: %s", PanzersIni->GetString("Locale", "Locale", "en"));
    PrepareGetText("menu.ini");                                    // 0x660db0
    if (!FileSystem.GetHomePath())                                 // 0x65ed70
        Logger.g->Panic("SSuperWindow::SSuperWindow: Not found home path in %s", Settings.GetIniPath());

    ScreenWidth = GetSystemMetrics(SM_CXSCREEN);
    ScreenHeight = GetSystemMetrics(SM_CYSCREEN);

    // HD 0x53a440 = SDXWindow::SetPosition (vtbl +0x08). WindowX/Y default to
    // 0x80000000; SWindow::Create (HD 0x5447e0) centres the window on the
    // screen while a coordinate is still that sentinel.
    SetPosition(Settings.WindowX, Settings.WindowY, Settings.WindowWidth, Settings.WindowHeight);
    // HD 0x53a510(WindowAALevel, WindowAAQualityLevel): windowed AA.
    // TEMP until P2-B merge: SWINE SDXWindow keeps a single AntialiasingMode;
    // left Off.
    if (Settings.FullScreenWidth == 0 || Settings.FullScreenHeight == 0) {
        Settings.FullScreenWidth = ScreenWidth;                   // 0x64f6b0
        Settings.FullScreenHeight = ScreenHeight;                 // 0x64f670
        Settings.Save();                                           // 0x64fdd0
        Logger.g->Log(0, "No initial fullscreen resolution, using desktop's: %dx%d",
                      Settings.FullScreenWidth, Settings.FullScreenHeight);
    }
    // HD 0x53a380(fsW, fsH, bpp, vsync, refresh, aa, aaq, 0) stores the
    // fullscreen mode; FullScreen itself is applied at Create time
    // (0x539d70 tests SDXWindow +0x8c). TEMP until P2-B merge: SWINE's
    // SDXWindow keeps the mode in DisplayMode/FullScreenWidth/Height.
    DisplayMode = Settings.FullScreen ? ExclusiveFullscreen : Windowed;
    VSync = Settings.FullScreenVSync;
    FullScreenWidth = Settings.FullScreenWidth;
    FullScreenHeight = Settings.FullScreenHeight;
    // HD 0x53a370(EnableTL), 0x53a360(EnableHAL), +0xc6 = UseMiles,
    // 0x52c2c0(MilesProvider): SDXWindow fields with no SWINE counterpart
    // (TEMP until P2-B/P2-C merge).
    if (Settings.PlaybackEvents)
        PlaybackEvents(Settings.EventFile.buf ? Settings.EventFile.buf : "");   // 0x544d60
    else if (Settings.RecordEvents)
        RecordEvents(Settings.EventFile.buf ? Settings.EventFile.buf : (char*)"");  // 0x544e60

    NewsFrame = -1;        // param_1[0x60]
    NewsTextFrame = -1;    // param_1[0x61]
    RootFrame = -1;        // param_1[0x6f]
    SplashFrame = -1;      // param_1[0x6b]
    Initialized = false;   // +0x12d
}

// PANZERS 0x6572a0
SSuperWindow::~SSuperWindow()
{
    if (PanzersIni) {
        delete PanzersIni;                                         // 0x660080
        PanzersIni = nullptr;
    }
    // HD: benchmark MessageBox ("Completed frames: %d ...") when
    // DAT_00929d65 && DAT_00929d66 — only with -movierec style recording.
    // HD: delete SPanzersCampaign (DAT_00929a0c) — never created here.
}

// PANZERS 0x657540
int SSuperWindow::Create(int icon, int cursor, const char* title)
{
    unsigned short cw = 0;
    __asm fnstcw cw;
    Logger.g->Log(0, "FPU control word (before D3D): 0x%04X", cw);
    wchar_t wtitle[64];
    MultiByteToWideChar(CP_ACP, 0, title, -1, wtitle, 64);
    // HD 0x539d70 SDXWindow::Create: window + SGepard + SMilesConcert.
    int r = SDXWindow::Create((HICON)(UINT_PTR)icon, (HCURSOR)(UINT_PTR)cursor, wtitle);
    __asm fnstcw cw;
    Logger.g->Log(0, "FPU control word (after D3D): 0x%04X", cw);
    if (r < 0)
        Logger.g->Panic("Unable to create window...");
    if (r == 1 || r == 2)
        Logger.g->Panic("Unable to initialize DirectX 9\n\nPlease check if DirectX is installed properly.");
    if (r == 3)
        Logger.g->Panic("Unable to initialize the default graphics card.\n\nPlease check if you installed the latest official drivers for your graphics card.\n\nThe program requires support for 800x600 32bit mode.");
    if (r == 4)
        Logger.g->Panic("Your graphics card doesn't support hi-resolution textures.\n\nPlease check if you installed the latest official drivers for your graphics card.");
    if (!Gepard || !Board)
        Logger.g->Panic("SSuperWindow::Create: renderer not created (Gepard=%p Board=%p)", Gepard, Board);

    // HD Gepard +0x10(10, TextureDetail != 0) and Gepard +0x58([Paths] Cache):
    // render options with no SWINE counterpart. TEMP until P2-B merge.
    // HD: RootFrame = board CreateFrame(7 = scaler, 0, 0,0,0,0), resized to
    // the client area, virtual size 0x400x0x300 (board +0x58), shown.
    // The menus are laid out in 1024x768 units and stretched to the client
    // area separately in x and y (board +0x58, 0x6cb0c0).
    RootFrame = Board->CreateFrame(FT_SCALER, 0, 0, 0, 0, 1);     // board +0x08
    Board->ResizeFrame(RootFrame, Width, Height);                  // board +0x14
    Board->SetVirtualSize(RootFrame, 0x400, 0x300);                // board +0x58
    Board->ShowFrame(RootFrame, Visible);                          // board +0x18
#if PANZERS_MOD_WIDESCREEN
    ModWidescreenOnSize(this);
#endif
    // HD: if FullScreen, vtbl +0xa4(0, 0x65bad0()) (display-mode switch).
#if PANZERS_HAVE_BINK
    if (SIPanzersConcert* pc = PzConcert())
        SBinkVideo::SetSoundSystem(pc->GetDigitalDriver());   // BinkSetSoundSystem(BinkOpenMiles, Concert +0x18)
#endif
    return r;
}

// PANZERS 0x65b830
void SSuperWindow::CloseBinkVideo()
{
    // HD first checks NetWkstaGetInfo's platform major version (>9) and, in
    // fullscreen, re-applies the display mode (vtbl +0xa4(1, 0)).
#if PANZERS_HAVE_BINK
    s_Intro.Close();
#endif
    Bink = nullptr;
    BinkBuffer = nullptr;
}

// PANZERS 0x657ef0
bool SSuperWindow::LoadBinkVideo(const char* filename)
{
#if PANZERS_HAVE_BINK
    SIPanzersConcert* pc = PzConcert();
    if (s_Intro.Open(filename, hWnd, Width, Height, Settings.FullScreen,
                     pc ? pc->GetDigitalDriver() : nullptr)) {
        Bink = s_Intro.Bink;
        BinkBuffer = s_Intro.Buffer;
        return true;
    }
    Logger.g->Log(0, "SSuperWindow::LoadBinkVideo: BinkOpen(%s) failed.", filename);
#else
    Logger.g->Log(0, "SSuperWindow::LoadBinkVideo: %s skipped (built without PANZERS_HAVE_BINK)", filename);
#endif
    // HD: BinkOpen failure -> Initialize + LoadMainMenu directly.
    Initialize();
    LoadMainMenu();
    return false;
}

// PANZERS 0x65ba00
bool SSuperWindow::BinkFrame()
{
#if PANZERS_HAVE_BINK
    if (s_Intro.Step())            // false when the last frame was shown
        return true;
    Bink = nullptr;                // Step closed it (0x65b830)
    BinkBuffer = nullptr;
    Initialize();
    LoadMainMenu();
    return false;
#else
    return true;
#endif
}

// PANZERS 0x65b470
void SSuperWindow::Play()
{
    // HD: _DAT_00929d65 = 0 (benchmark flags), DAT_00929d68 = -4 (frame count)
    if (Settings.RecordFrames)
        Logger.g->Log(0, "SSuperWindow::Play: frame recording requested (not lifted)");   // 0x6616e0
    if (!Settings.HostFromCommandLine) {
        if (Settings.ConnectHost.size != 0)
            PzStub_ConnectToHostFromCommandLine(Settings.ConnectHost.buf);
    } else {
        PzStub_CreateHostFromCommandLine();
    }

    if (Settings._a9) {
        // HD: Initialize, new SPanzersCampaign (0xb8c), new SMulti (0x5130),
        // new multi view (0x25b4) — the "-market" campaign start.
        Initialize();
        PzStub_StartCampaignFromCommandLine();
        LoadMainMenu();
    } else if (Settings.MapFile.size == 0) {
        if (!Settings.StartMultiFromCommandLine) {
            if (SkipIntro || BinkShouldSkipIntro(GetCommandLineA(), "intro.bik")) {
                // Recompile-only "-nointro": the same branch HD takes when
                // BinkOpen(intro.bik) fails (0x657ef0).
                Logger.g->Log(0, "SSuperWindow::Play: -nointro, skipping intro.bik");
                Initialize();
                LoadMainMenu();
            } else {
                SString name;
                FileSystem.FileNameProcess(&name, "intro.bik");    // 0x65f020
                LoadBinkVideo(name.buf ? name.buf : "intro.bik");
                if (name.buf) delete[] name.buf;
            }
        } else {
            if (!PzStub_CheckCDKey(nullptr)) {                     // SSettings::CHECKCDKEY
                OnClose();                                         // vtbl +0x8c
                ::MessageBoxA(hWnd, "No / Invalid Keycode!", "Invalid CD Key", MB_ICONERROR);
                return;
            }
            Initialize();
            if (!Settings.HostFromCommandLine)
                PzStub_LoadMultiPreMenu();                         // 0x658a30
            else
                PzStub_LoadChatRoomView();                         // 0x658050
        }
    } else {
        Initialize();
        PzStub_LoadMapFromCommandLine(Settings.MapFile.buf);       // 0x5944d0 + 0x6585e0
        LoadMainMenu();
    }
    Run();                                                         // SWindow::Run 0x544db0
}

// PANZERS 0x657910 (exported ?Initialize@SSuperWindow@@AAEXXZ)
void SSuperWindow::Initialize()
{
    // Gepard render options (Gepard +0x10 SetOption): 8/9 texture filter
    // (0: 0/0, 1: 1/0, else 1/1), 3 shadow buffer size, 2 the shadow
    // technique (Shadows = 2 "Self Shadow": GetCap(0), the one the card
    // supports; 1 "Normal Shadow": the compatible technique 1; 0 off), and
    // 0, 5, 6 on.
    pz::SIGepardHD* g = pz::PzGepard();
    if (Settings.TextureFilter == 0) {
        g->SetOption(8, 0);
        g->SetOption(9, 0);
    } else if (Settings.TextureFilter == 1) {
        g->SetOption(8, 1);
        g->SetOption(9, 0);
    } else {
        g->SetOption(8, 1);
        g->SetOption(9, 1);
    }
    g->SetOption(3, Settings.ShadowBufferSize);
    g->SetOption(2, Settings.Shadows == 2 ? g->GetCap(0) : Settings.Shadows);   // 0x64e240
    g->SetOption(0, 1);
    g->SetOption(5, 1);
    g->SetOption(6, 1);
    // HD then: board +0x9c(-1,0,0,0) and +0xc8(HW cursor); Concert +0x10
    // volumes.
    Board->SetCursor(-1, 0, 0);                                    // board +0x9c(-1, 0, 0, 0): no cursor yet
    // HD board +0xc8 SetHardwareMouseCursor(Settings +0x154 "Hardware Mouse
    // Cursor"). The D3D hardware cursor is not ported: the board always
    // draws the software cursor, as HD does with the default setting 0.
    if (Settings.HardwareMouseCursor)
        Logger.g->Log(0, "SSuperWindow::Initialize: Hardware Mouse Cursor = 1 ignored, using the software cursor");
    if (Concert) {
        Concert->SetVolume(0, Settings.MusicVolume);               // Concert +0x10
        Concert->SetVolume(1, Settings.SoundEffectVolume);
        Concert->SetVolume(2, Settings.VoiceVolume);
    }
    if (SIPanzersConcert* pc = PzConcert()) {
        pc->ClearPlaylist();                                       // +0x6c
        pc->AddToPlaylist("music/Menu.mp3");                       // +0x70
        pc->StartPlaylist(true);                                   // +0x78(1)
    }

    // Fonts 0..5. HD: board +0x6c LoadFont(".font") must return 0..5 in order,
    // else Panic("SSuperWindow::Create: Bad font handle."). The SWINE-shared
    // board may hold other fonts already, so the handles are recorded in
    // g_PzFont instead of being required to be 0..5.
    static const struct { const char* file; int size; } kFonts[6] = {
        { "menu/fonts/sans_serif_new/sans_serif_14_hq.font", 14 },
        { "menu/fonts/sans_serif_new/sans_serif_14_black_hq.font", 14 },
        { "menu/fonts/sans_serif_new/sans_serif_21_hq.font", 21 },
        { "menu/fonts/sans_serif_new/sans_serif_21_shadow_hq.font", 21 },
        { "menu/fonts/sans_serif_new/sans_serif_28_shadow_hq.font", 28 },
        { "menu/fonts/sans_serif_new/sans_serif_18_shadow_hq.font", 18 },
    };
    for (int i = 0; i < 6; ++i) {
        g_PzFont[i] = Board->LoadFontFileFont(kFonts[i].file);    // P2-B 0x6c61a0
        if (g_PzFont[i] < 0)
            Logger.g->Panic("SSuperWindow::Create: Bad font handle.");
    }
    // HD: if DynamicText, fonts 6..11 = "Microsoft Sans Serif" system fonts
    // (board +0x60). Not on the default path (DynamicText = 0).

    int splash = PzLoadTexture("menu/splash_hq.tga");              // board +0x7c
    SplashFrame = Board->CreateFrame(FT_SPRITE, RootFrame, 0, 0, 0, 0);
    Board->SetSpriteGlyph(SplashFrame, splash, 0);
    Board->ShowFrame(SplashFrame, true);
    // HD: scene +0x50(0, 0) on SDXWindow +0xdc (draw one frame) — render now
    // so the splash is visible while the data loads.
    Gepard->RenderScene(0);
    PzReleaseTexture(splash);                                      // board +0x80

    // Menu cursor: 21 glyphs of 40x40 in menu/cursor2_hq.tga. HD builds the
    // hotspot table on the stack from the 16-byte .rdata blocks at
    // 0x80ae00..0x80ae6f (MOVUPS pairs); values read from the PANZERS.exe
    // bytes. Glyph 0 (hotspot 2,2) is the arrow every menu screen selects
    // with SetCursor(0) (0x543970).
    static const POINT kCursorHotspots[0x15] = {
        {  2,  2 }, {  2,  2 }, {  2,  2 }, {  2,  2 }, {  2,  2 },   // 0x80ae10 x4, 0x80ae18 x4
        {  2,  2 }, {  2,  2 }, {  2,  2 }, {  2,  2 },
        { 15, 16 }, { 15, 16 },                                         // 0x80ae40
        { 16, 16 }, {  0, 16 },                                         // 0x80ae30
        {  0,  0 }, { 16,  0 },                                         // 0x80ae00
        { 31,  0 }, { 31, 16 },                                         // 0x80ae50
        { 31, 31 }, { 16, 31 },                                         // 0x80ae60
        {  0, 31 }, { 15, 15 },                                         // 0x80ae20
    };
    Board->LoadCursorSetFile("menu/cursor2_hq.tga", 0x28, 0x15, kCursorHotspots);   // board +0x94 (0x6c59e0)
    LoadMenuSkins();                         // 0x544040
    // HD: DAT_00929f14 = Gepard +0x5c() (SPixie, AddRef'd; released in
    // OnDestroy). Only on the 3D menu path (-menu3d / PZ_MENU_WORLD), so the
    // default boot path does not create the pz facade.
    Logger.g->Log(0, "SSuperWindow::Initialize: 3D menu world %s, trace %s",
                  pz::g_Menu3D.World ? "on" : "off", pz::g_Menu3D.Trace ? "on" : "off");
    if (pz::g_Menu3D.World)
        pz::g_Pixie = pz::PzGepard()->GetPixie();                  // Gepard +0x5c
    // HD: new 0x124, SUnitRegistry ctor 0x5cfe30 (LoadUnitFiles 0x5d1050 on
    // units/, buildings/, units/ingame/). Only the 3D menu world reads unit
    // data, so the default path (world off) does not scan the .unit files.
    if (pz::g_Menu3D.World)
        s_UnitRegistry = new pz::SUnitRegistry();
    if (!Settings.StartMultiFromCommandLine)
        LoadMenuBackground(true);            // 0x658690(1)
    if (SplashFrame >= 0) {
        Board->DestroyFrame(SplashFrame);    // board +0x0c
        SplashFrame = -1;
    }
    Initialized = true;                      // +0x12d
}

// World part of SSuperWindow::LoadMenuBackground 0x658690 (taken when the 3D
// menu world is on). OWNER: agent D.
void SSuperWindow::LoadMenuWorld()
{
    const char* map = "maps/menu.map";                             // 0x51de30
    SStream* stream = FileSystem.OpenRead(map, nullptr);           // 0x65f420(name, 0)
    if (!stream)
        Logger.g->Panic("Can't load map: %s", map);
    MenuWorld = new pz::SWorld(0);                                 // new 0x7538, 0x5d2f90(0)
    MenuWorld->ShowLoadingIcon(RootFrame);                         // 0x5edca0(+0x1bc)
    if (!MenuWorld->LoadMap(stream, true, 0, 0))                   // 0x5f1990(stream, 1, 0, 0)
        Logger.g->Panic("Can't load map: %s", map);
    MenuWorld->Initialize();                                       // 0x5eec90
    stream->Release();                                             // HD stream vtbl +0 (delete)
    MenuGameLogic = new pz::SGameLogic(0, -1, 0);                  // new 0x318, 0x55e440(0, -1, 0)
    MenuGameLogic->SetRunning(1);                                  // 0x5802f0(1)
    MenuWorld->HideLoadingIcon();                                  // 0x5dc7d0
    pz::LogWorldStats("LoadMenuBackground");                       // recompile: M1 verification totals
}

// PANZERS 0x658690
void SSuperWindow::LoadMenuBackground(bool keepScene)
{
    if (MenuTopFrame >= 0)
        return;
    // HD: if no music is playing, restart music/menu.mp3.
    if (SIPanzersConcert* pc = PzConcert()) {
        if (!pc->IsStreamPlaying()) {                              // +0x84
            pc->ClearPlaylist();
            pc->AddToPlaylist("music/menu.mp3");
            pc->ShufflePlaylist();                                 // +0x74
            pc->StartPlaylist(true);
        }
    }
    if (!MenuWorld) {
        if (pz::g_Menu3D.World)
            LoadMenuWorld();          // maps/menu.map: SWorld (0x7538), SGameLogic (0x318)
        else
            PzStub_LoadMenuWorld(this);
    }
    if (keepScene)
        return;
    // HD: SDXWindow+0xe0 scene: Release the old one, AddRef g_Scene into it;
    // +0xd8 = 1 (continuous render: the SWINE SDXWindow::OnIdle always
    // renders). g_Scene is null unless the menu world is loaded.
    if (pz::g_WindowScene)
        pz::g_WindowScene->Release();                              // scene +0x04
    if (pz::g_Scene)
        pz::g_Scene->AddRef();                                     // scene +0x00
    pz::g_WindowScene = pz::g_Scene;
    // MenuTime/MenuNextTick = timer seconds (0x661800).
    MenuTime = MenuNextTick = (float)((double)Timer.GetTickValue() / 1000.0);
    Cursor = 0;                                                  // 0x543970 (SWidget +0x3c; SWINE SetCursor has no body)(0, -1)
    int tex = PzLoadTexture("menu/main_menu_top_hq.tga");
    MenuTopFrame = Board->CreateFrame(FT_SPRITE, RootFrame, 0, 0, 0, 0);
    Board->SetSpriteGlyph(MenuTopFrame, tex, 0);
    Board->ShowFrame(MenuTopFrame, true);
    PzReleaseTexture(tex);
    tex = PzLoadTexture("menu/main_menu_bottom_hq.tga");
    MenuBottomFrame = Board->CreateFrame(FT_SPRITE, RootFrame, 0, 0x29a, 0, 0);
    Board->SetSpriteGlyph(MenuBottomFrame, tex, 0);
    Board->ShowFrame(MenuBottomFrame, true);
    PzReleaseTexture(tex);
    // Version text, right-aligned at (0x3f6, 5) on the top bar.
    int text = Board->CreateFrame(FT_TEXT, MenuTopFrame, 0x3f6, 5, 0, 1);
    char buf[64];
    _snprintf(buf, sizeof(buf) - 1, "Version: %s", PzStub_GetVersionString());
    buf[sizeof(buf) - 1] = 0;
    Board->SetText(text, g_PzFont[PZF_SANS14], 1, buf);            // HD +0x34(f, 0, 1, s, 0)
    if (SplashFrame >= 0) {
        Board->DestroyFrame(SplashFrame);
        SplashFrame = -1;
    }
}

// PANZERS 0x65b940
void SSuperWindow::UnloadMenuBackground()
{
    if (MenuTopFrame < 0)
        return;
    Board->DestroyFrame(MenuTopFrame);
    Board->DestroyFrame(MenuBottomFrame);
    MenuTopFrame = -1;
    MenuBottomFrame = -1;
    if (pz::g_WindowScene) {                                       // SDXWindow+0xe0
        pz::g_WindowScene->Release();                              // scene +0x04
        pz::g_WindowScene = nullptr;
    }
    if (MenuGameLogic) {
        delete MenuGameLogic;                                      // 0x55fe00 + delete 0x318
        MenuGameLogic = nullptr;
    }
    if (MenuWorld) {
        delete MenuWorld;                                          // vtbl +0 (0x5d68b0)
        MenuWorld = nullptr;
    }
    // HD: +0xd8 = 0 (continuous render off).
    Cursor = -1;                                                 // 0x543970 (SWidget +0x3c; SWINE SetCursor has no body)(-1, -1)
}

// PANZERS 0x6583e0
void SSuperWindow::LoadMainMenu()
{
    // HD: delete SPanzersCampaign (DAT_00929a0c), the GameSpy object
    // (DAT_008f1c3c) and SMulti (DAT_008f1a74) — none exist on this path.
    LoadMenuBackground(false);                                     // 0x658690(0)
    MainMenu = new SMainMenu();                                    // new 0x13dc, 0x633030
    InsertChild(MainMenu);                                         // vtbl +0x54
    MainMenu->SetPosition(0, 0, 0x400, 0x300);                     // vtbl +0x08
    MainMenu->Create();                                            // 0x6352f0
    Logger.g->Log(0, "SSuperWindow::LoadMainMenu: releasing scene");
    if (pz::g_M3.Enabled)
        M3OnMainMenu(this);                                        // recompile-only -m3 -packetplay replay start
}

// PANZERS 0x658300
void SSuperWindow::LoadMainCreditMenu()
{
    // HD: delete SPanzersCampaign (DAT_00929a0c) � never created here.
    LoadMenuBackground(false);                                     // 0x658690(0)
    CreditMenu = new SMainCreditMenu();                            // new 0xd4, 0x632fd0
    InsertChild(CreditMenu);                                       // vtbl +0x54
    CreditMenu->SetPosition(0, 0, 0x400, 0x300);                   // vtbl +0x08
    CreditMenu->Create();                                          // 0x635190
    Logger.g->Log(0, "SSuperWindow::LoadMainCreditMenu: releasing scene");
}

// PANZERS 0x65b8c0
void SSuperWindow::ReleaseMultiView()
{
    if (MultiView) {
        delete MultiView;
        MultiView = nullptr;
    }
    // HD then moves the world, logic and scene the market set aside back
    // (0x929f18 / 0x929f1c / 0x929f20; superwindow_m3.cpp). No-op unless the
    // M3 market was opened.
    M3RestoreMovedWorld();
}

// PANZERS 0x658fc0
void SSuperWindow::ReleaseNews()
{
    if (NewsFrame != -1) {
        Board->DestroyFrame(NewsTextFrame);
        NewsTextFrame = -1;
        Board->DestroyFrame(NewsFrame);
        NewsFrame = -1;
    }
    if (NewsWidget) {
        delete NewsWidget;
        NewsWidget = nullptr;
    }
}

// PANZERS 0x65b390
bool SSuperWindow::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    if (Bink) {
        CloseBinkVideo();
        Initialize();
        LoadMainMenu();
        return true;
    }
    if (key == VK_F11) {
        // HD: vtbl +0xa4(0, 0) toggles the display mode. TEMP until P2-B.
        Logger.g->Log(0, "SSuperWindow::OnKeyDown: F11 display-mode toggle not lifted");
    }
    return true;
}

// PANZERS 0x65b3e0
void SSuperWindow::OnMouseDown(int button, int x, int y, int shift)
{
    (void)button; (void)x; (void)y; (void)shift;
    if (Bink) {
        CloseBinkVideo();
        Initialize();
        LoadMainMenu();
    }
}

// PANZERS 0x65b410
void SSuperWindow::OnSize(int w, int h)
{
    SDXWindow::OnSize(w, h);                                       // 0x53a1b0
    // HD 0x53a1b0 ends with viewport +0x04 Resize (0x68c4e0): device reset
    // to the new client size and the projection's y scale recomputed for the
    // new aspect. The SWINE SDXWindow::OnSize does the device reset
    // (Gepard->Resize); the pz viewport does the rest.
    if (w && h && pz::g_Menu3D.World && DisplayMode != ExclusiveFullscreen)
        pz::PzGepard()->GetViewport(0)->Resize(w, h);              // viewport +0x04
    // The root scaler follows the client size; its virtual size stays
    // 1024x768, so the menus stretch with the window.
    if (Board && RootFrame >= 0)
        Board->ResizeFrame(RootFrame, Width, Height);              // board +0x14
#if PANZERS_MOD_WIDESCREEN
    ModWidescreenOnSize(this);
#endif
    // HD: MultiView -> 0x64b9e0; else GameView -> 0x625d80 (in-game HUD).
}

// PANZERS 0x657850
SWidget* SSuperWindow::GetEventTarget(int x, int y, int* lx, int* ly)
{
    LastMouseX = x;                                                // +0x80 / +0x84
    LastMouseY = y;
    if (Width == 0 || Height == 0)
        return this;
#if PANZERS_MOD_WIDESCREEN
    {
        int mx, my;
        if (ModWidescreenEventPoint(this, x, y, &mx, &my))
            return SDXWindow::GetEventTarget(mx, my, lx, ly);
    }
#endif
    // HD maps the client area onto the 1024x768 virtual screen, separately
    // in x and y, with rounding: x' = (W/2 + x*1024) / W, y' = (H/2 + y*768) / H.
    int vx = ((Width >> 1) + x * 0x400) / Width;
    int vy = ((Height >> 1) + y * 0x300) / Height;
    // HD: with a captured widget (SWidget +0x5c) the point goes to that
    // widget's GetEventTarget relative to it (0x5435d0); SWINE's
    // SWindow::GetEventTarget handles the capture itself.
    return SDXWindow::GetEventTarget(vx, vy, lx, ly);
}

// PANZERS 0x6578f0
int SSuperWindow::GetFrame()
{
    return RootFrame;
}

// PANZERS 0x659250 (main-menu subset; the rest is logged by a stub)
bool SSuperWindow::OnAction(SWidget* source, int action, int param)
{
    (void)source; (void)param;
    // Options screens: 0x4d4d5 -> LoadMainOptionsMenu 0x658500, 0x4d4f4 back
    // to the main menu, 0x4f413..0x4f416 / 0x4f564 / 0x4f565 live settings
    // (optionsmenu.cpp).
    if (SuperWindowOptionsAction(this, action, param))
        return true;
    // M3 Training Camp path (default off): campaign, game view, market,
    // mission start and GV_GAMEOVER cases of 0x659250.
    if (pz::g_M3.Enabled && SuperWindowM3Action(this, action, param))
        return true;
    switch (action) {
    case PZA_MAIN_MULTIPLAYER:                     // 0x4d4d2 -> 0x658a30
    case PZA_MAIN_TUTORIAL:                        // 0x4d4d3 -> maps/tutorial.map
    case PZA_MAIN_TRAINING:                        // 0x4d4d4 -> training camp menu
    case PZA_MAIN_ALLIED2:                         // 0x4d4d9 -> maps/us-02.map
    case PZA_MAIN_NEWGAME_DONE:                    // 0x4d4d1 -> SPanzersCampaign
        // HD deletes the main menu first for every one of these; the
        // recompile keeps it (the target screens are stubs) so the user can
        // still pick Exit.
        PzStub_SuperWindowAction(action);
        return true;
    case PZA_MAIN_CREDITS:                         // 0x4d4d6
        if (MainMenu) {
            delete MainMenu;
            MainMenu = nullptr;
        }
        LoadMainCreditMenu();                      // 0x658300
        return true;
    case PZA_CREDITS_DONE:                         // 0x43521 from SMainCreditMenu
        if (CreditMenu) {
            delete CreditMenu;
            CreditMenu = nullptr;
        }
        if (SIPanzersConcert* pc = PzConcert()) {
            pc->ClearPlaylist();                   // Concert +0x6c
            pc->AddToPlaylist("music/Menu.mp3");   // +0x70
            pc->StartPlaylist(true);               // +0x78(1)
        }
        LoadMainMenu();                            // 0x6583e0
        return true;
    case PZA_MAIN_EXIT:                            // 0x4d4d8 (also 0x47564)
    case 0x47564:
        if (MainMenu) {
            delete MainMenu;
            MainMenu = nullptr;
        }
        UnloadMenuBackground();                    // 0x65b940
        AchimMenu = new SAchimMenu();              // new 0xd4, 0x632ee0
        InsertChild(AchimMenu);
        AchimMenu->SetPosition(0, 0, 0x400, 0x300);
        AchimMenu->Create(true);                   // 0x634d60(1)
        return true;
    case PZA_ACHIM_BACK:                           // 0x414d1
        if (AchimMenu) {
            delete AchimMenu;
            AchimMenu = nullptr;
        }
        LoadMainMenu();
        return true;
    case PZA_ACHIM_QUIT:                           // 0x414d2
        if (AchimMenu) {
            delete AchimMenu;
            AchimMenu = nullptr;
        }
        OnClose();                                 // vtbl +0x8c
        return true;
    case PZA_BUTTON_DOWN:                          // button notifications that
    case PZA_BUTTON_CLICK:                         // bubbled up unhandled; HD
    case PZA_BUTTON_OVER:                          // also just returns 1
    case PZA_BUTTON_RCLICK:
    case PZA_BUTTON_OUT:
        return true;
    default:
        if (action >= 0x40000)
            PzStub_SuperWindowAction(action);
        return true;
    }
}

// PANZERS 0x65ab50
void SSuperWindow::OnDestroy()
{
#if PANZERS_MOD_WIDESCREEN
    ModWidescreenShutdown();
#endif
    CloseBinkVideo();                              // 0x65b830
    ReleaseNews();
    // HD: 0x5447b0 (SWindow capture reset), 0x65b8c0
    ReleaseMultiView();
    if (GameView) { delete GameView; GameView = nullptr; }
    if (MainMenu) { delete MainMenu; MainMenu = nullptr; }
    if (Menu_ec) { delete Menu_ec; Menu_ec = nullptr; }
    if (Menu_f0) { delete Menu_f0; Menu_f0 = nullptr; }
    if (Menu_fc) { delete Menu_fc; Menu_fc = nullptr; }
    if (BriefingMenu) { delete BriefingMenu; BriefingMenu = nullptr; }
    if (Menu_108) { delete Menu_108; Menu_108 = nullptr; }
    if (Menu_f8) { delete Menu_f8; Menu_f8 = nullptr; }
    if (Menu_f4) { delete Menu_f4; Menu_f4 = nullptr; }
    if (Menu_10c) { delete Menu_10c; Menu_10c = nullptr; }
    if (Menu_114) { delete Menu_114; Menu_114 = nullptr; }
    if (AchimMenu) { delete AchimMenu; AchimMenu = nullptr; }
    // HD deletes neither CreditMenu (+0x100) nor TrainingCampMenu (+0x118)
    // here. Closing the window on the Credits screen therefore leaves the
    // credits widget linked under SSuperWindow, and ~SWidget (0x5430e0, via
    // ~SSuperWindow 0x6572a0 -> 0x5445a0) panics "Children widgets should be
    // removed first": an original-game bug, kept in the faithful build.
    // The same holds for the Training Camp dialog (-m3, superwindow_m3.cpp):
    // closing the window while it is open panics the same way (seen in the
    // recompile; HD 0x65ab50 deletes +0x104..+0x11c except +0x118, so HD
    // has the bug too; decided statically, M3-I).
#if PANZERS_MOD_BUGFIXES
    if (CreditMenu) { delete CreditMenu; CreditMenu = nullptr; }  // as PZA_CREDITS_DONE (0x43521) does
    if (TrainingCampMenu) { delete TrainingCampMenu; TrainingCampMenu = nullptr; }   // as Start / Cancel do
#endif
    UnloadMenuBackground();                        // 0x65b940
    Board->UnloadCursorSet();                      // board +0x98 (0x6cbd00)
    // HD: ReleaseFont 0..11 (board +0x80).
    for (int i = 0; i < 6; ++i)
        if (g_PzFont[i] >= 0) { Board->ReleaseFont(g_PzFont[i]); g_PzFont[i] = -1; }
    if (s_UnitRegistry) {
        delete s_UnitRegistry;                     // 0x5d0c10 + delete 0x124
        s_UnitRegistry = nullptr;
    }
    if (Board && RootFrame >= 0) {
        Board->DestroyFrame(RootFrame);            // board +0x0c
        RootFrame = -1;
    }
    if (pz::g_Pixie) {                             // DAT_00929f14 vtbl +0x04
        pz::g_Pixie->Release();
        pz::g_Pixie = nullptr;
    }
    pz::PzGepardShutdown();                        // recompile: the facade over the SWINE Gepard
    SDXWindow::OnDestroy();                        // 0x53a040
}

// PANZERS 0x65ae50
bool SSuperWindow::OnIdle()
{
    if (Bink) {
        BinkFrame();                               // 0x65ba00
        return true;
    }
    if (Menu_f0)
        Menu_f0->Update();                         // vtbl +0x78
    // HD 0x65ae50: +0x110 (the market), +0x104, +0x108, else +0xe4.
    SWidget* w = MultiView ? MultiView : BriefingMenu ? BriefingMenu : Menu_108 ? Menu_108 : GameView;
    if (w)
        w->Update();
    // HD: (Menu_f8 && Menu_f8->Visible) || Menu_f4 -> Update
    if (Menu_f8 && Menu_f8->Visible)
        Menu_f8->Update();
    else if (Menu_f4)
        Menu_f4->Update();
    if (MenuWorld) {
        // HD 0x65aef8..0x65b03a (SSE single-precision maths).
        float now = (float)((double)Timer.GetTickValue() / 1000.0);  // 0x661800
        if (MenuNextTick < now) {
            do {
                MenuGameLogic->Refresh();                          // 0x576d80
                MenuNextTick += 0.05f;                             // _DAT_007f4534: 20 Hz
            } while (MenuNextTick < now);
        }
        int elapsedMs = (int)((now - MenuTime) * 1000.0f);         // 0x7f1b94 = 1000.0, ftol
        pz::g_Scene->AdvanceTime(elapsedMs);                       // scene +0x1c
        // HD board +0xa0(elapsedMs): board animation clock. The SWINE board
        // keeps its own clock; not mapped.
        MenuTime = now;
        pz::SIViewport* vp = pz::PzGepard()->GetViewport(0);       // Gepard +0x3c(0)
        pz::g_World->ComputeCamera(vp);                            // 0x5ddc30
        double interpolation = (double)((MenuNextTick - now) * 20.0f);   // 0x7f35d8 = 20.0
        pz::g_Scene->SetInterpolation(interpolation);              // scene +0x20
        vp = pz::PzGepard()->GetViewport(0);
        pz::g_GameLogic->UpdateUnitVisuals(vp, interpolation);     // 0x5638f0 on DAT_008f2078
        if (Concert)
            Concert->Update(false);                                // Concert +0x0c(0)
    }
    // HD: -movierec frame capture (Settings.RecordFrames) — not lifted.
#if PANZERS_MOD_WIDESCREEN
    ModWidescreenFrame(this);
#endif
    if (EventFrame(0, 0))                          // SWindow::EventFrame 0x544a50
        OnClose();                                 // vtbl +0x8c
    // HD: news ticker fade (+0x184/+0x178, board +0x20/+0x10) — news.ini
    // fetch is a stub, NewsTextFrame stays -1.
    if (pz::g_WindowScene) {
        // HD SDXWindow::OnIdle 0x53a0d0 with SDXWindow+0xe0 set: board cursor
        // (+0x9c), then primary viewport +0x50 Render(scene, 0). Same as the
        // SWINE SDXWindow::OnIdle below, but through the HD viewport so the
        // scene is drawn before the board.
        if (!SWidget::LastMouseTarget)
            UpdateMouse();
        Board->SetCursor(SWidget::GetCurrentCursor(), CursorX, CursorY);
        pz::PzGepard()->GetViewport(0)->Render(pz::g_WindowScene, 0);
        return true;
    }
    return SDXWindow::OnIdle();                    // 0x53a0d0
}
