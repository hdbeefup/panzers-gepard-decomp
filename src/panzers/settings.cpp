// src/panzers/settings.cpp
// SSettings — options.ini loader and command-line parser (HD PANZERS.exe).
// See settings.h for the layout evidence.

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "settings.h"
#include "properties.h"
#include "logger.h"
#include "stub_log.h"

SSettings Settings;   // HD global 0x929cf8

static void AssignString(SString& s, const char* v)
{
    // FUN_0052c320 (SString::operator=(const char*)) on the HD side.
    s = v ? v : "";
}

// PANZERS 0x64f6c0
void SSettings::SetIniPath(const char* path)
{
    AssignString(IniPath, path);
}

// PANZERS 0x64f780
void SSettings::SetLogDir(const char* path)
{
    AssignString(LogDir, path);
}

static bool SwitchIs(const char* arg, const char* sw)
{
    // FUN_0052c410 is SString::operator==(const char*) — a case-insensitive
    // compare in the HD build (the -ini/-log tests right before it use
    // _stricmp, and the switch table mixes "+host" with "-server").
    return _stricmp(arg, sw) == 0;
}

// PANZERS 0x64e330
//
// SSettings::Initialize: reset the command-line state, read options.ini into
// the fields below, then parse the command line. The HD function reads
// __argc/__argv itself (FUN_0079e14d/FUN_0079e153 are the CRT accessors);
// this lift takes them as parameters so the recompile can pre-filter
// recompile-only switches (see gamemain.cpp, "-nointro").
void SSettings::Initialize(int argc, char** argv)
{
    RecordFrames = 0;            // +0x20 / +0x10 / +0x14 / +0x18 / +0x1c resets
    _20 = 0;
    RecordWidth = 0x280;
    RecordHeight = 0x1e0;
    Skip = 0;
    AssignString(_24, nullptr);
    AssignString(MapFile, nullptr);
    _88 = false;
    AssignString(_8c, nullptr);
    AssignString(CutsceneFile, nullptr);
    SkipFrames = 0;
    AssignString(EventFile, nullptr);
    RecordEvents = false;
    PlaybackEvents = false;
    AssignString(_40, nullptr);
    _48 = 0;
    _4c = false;
    _4d = false;
    AssignString(_50, nullptr);
    _58 = false;
    HostFromCommandLine = false;
    AssignString(ConnectHost, nullptr);
    StartMultiFromCommandLine = false;
    _a9 = false;
    UseMiles = true;

    OptionsIni = new SProperties("options.ini", false, false);

    // [Game options]
    int v = OptionsIni->GetInt("Game options", "Mouse scroll speed", 5);
    MouseScrollSpeed = (float)((double)(v - 5) * 0.002 + 0.025);    // _DAT_008043a0 / _DAT_00806e18
    v = OptionsIni->GetInt("Game options", "Keyboard scroll speed", 5);
    KeyboardScrollSpeed = (float)((double)(v - 5) * 0.002 + 0.02);  // _DAT_008043a0 / _DAT_007f59e8
    ToolTips = OptionsIni->GetInt("Game options", "Tool tips", 1);
    OwnIcon = OptionsIni->GetInt("Game options", "OwnIcon", 1);
    AlliedIcon = OptionsIni->GetInt("Game options", "AlliedIcon", 1);
    EnemyIcon = OptionsIni->GetInt("Game options", "EnemyIcon", 1);
    ShowTipsAtStartup = OptionsIni->GetInt("Game options", "Show tips at startup", 1);
    Subtitles = OptionsIni->GetInt("Game options", "Subtitles", 2);
    AutoSave = OptionsIni->GetInt("Game options", "AutoSave", 3);
    FogOfWarView = OptionsIni->GetInt("Game options", "Fog of war view", 1);
    UnitVoice = OptionsIni->GetInt("Game options", "Unit Voice", 2);
    KeyboardBindings = OptionsIni->GetInt("Game options", "Keyboard Bindings", 0); // via 0x64f860
    DynamicText = OptionsIni->GetInt("Game options", "DynamicText", 0);
    AssignString(DynamicCharset, OptionsIni->GetString("Game options", "DynamicCharset", ""));

    // [Graphics settings]
    Brightness = OptionsIni->GetInt("Graphics settings", "Brightness", 5);
    FullScreen = OptionsIni->GetInt("Graphics settings", "FullScreen", 0) != 0;
    FullScreenWidth = OptionsIni->GetInt("Graphics settings", "FullScreenWidth", 0);
    FullScreenHeight = OptionsIni->GetInt("Graphics settings", "FullScreenHeight", 0);
    FullScreenBPP = OptionsIni->GetInt("Graphics settings", "FullScreenBPP", 32);
    FullScreenRefreshRate = OptionsIni->GetInt("Graphics settings", "FullScreenRefreshRate", 0);
    FullScreenAALevel = OptionsIni->GetInt("Graphics settings", "FullScreenAALevel", 0);
    FullScreenAAQualityLevel = OptionsIni->GetInt("Graphics settings", "FullScreenAAQualityLevel", 0);
    FullScreenVSync = OptionsIni->GetInt("Graphics settings", "FullScreenVSync", 0) != 0;
    WindowWidth = OptionsIni->GetInt("Graphics settings", "WindowWidth", 0x400);
    WindowHeight = OptionsIni->GetInt("Graphics settings", "WindowHeight", 0x300);
    WindowX = OptionsIni->GetInt("Graphics settings", "WindowX", (int)0x80000000);
    WindowY = OptionsIni->GetInt("Graphics settings", "WindowY", (int)0x80000000);
    WindowAALevel = OptionsIni->GetInt("Graphics settings", "WindowAALevel", 0);
    WindowAAQualityLevel = OptionsIni->GetInt("Graphics settings", "WindowAAQualityLevel", 0);
    Shadows = OptionsIni->GetInt("Graphics settings", "Shadows", 1);
    EffectsDetail = OptionsIni->GetInt("Graphics settings", "Effects detail", 1);
    TextureDetail = OptionsIni->GetInt("Graphics settings", "Texture detail", 1);
    TextureFilter = OptionsIni->GetInt("Graphics settings", "Texture filter", 2);
    EnableTL = OptionsIni->GetInt("Graphics settings", "EnableTL", 1) != 0;
    EnableHAL = OptionsIni->GetInt("Graphics settings", "EnableHAL", 1) != 0;
    ShadowBufferSize = OptionsIni->GetInt("Graphics settings", "ShadowBufferSize", 0x800);
    HardwareMouseCursor = OptionsIni->GetInt("Graphics settings", "Hardware Mouse Cursor", 0) != 0;

    // [Audio settings]
    MusicVolume = OptionsIni->GetInt("Audio settings", "Music volume", 5);
    SoundEffectVolume = OptionsIni->GetInt("Audio settings", "Sound effect volume", 5);
    VoiceVolume = OptionsIni->GetInt("Audio settings", "Voice volume", 5);
    ReverseChannels = OptionsIni->GetInt("Audio settings", "Reverse channels", 0);
    UseMiles = OptionsIni->GetInt("Audio settings", "Use Miles", 1) != 0;
    AssignString(MilesProvider, OptionsIni->GetString("Audio settings", "Miles provider", ""));

    // [Network options]
    char userName[0x32];
    DWORD userNameLen = sizeof(userName);
    if (!GetUserNameA(userName, &userNameLen)) {
        Logger.g->Log(1, "Settings:Initalize: ERROR: GetUserName");
        strcpy(userName, "Player");
    }
    AssignString(PlayerName, OptionsIni->GetString("Network options", "Player Name", userName));
    AssignString(LastHostIP, OptionsIni->GetString("Network options", "Last host IP", ""));
    AssignString(RGLogin, OptionsIni->GetString("Network options", "RG Login", ""));
    // "RG Pass" is decoded by 0x64ddb0/0x64d6d0 into the RankedGaming client
    // state — RankedGaming is out of scope (logged stub, not read here).

    // Command line (HD: __argc/__argv, index 1..argc-1).
    for (int i = 1; i < argc; ++i) {
        const char* a = argv[i];
        if (!a)
            continue;
        Logger.g->Log(0, "SSettings::Initalize() %s", a);
        if (!*a) {
            // empty argument: falls through to the switch chain in HD
        }
        if (!_stricmp(a, "-ini") || !_stricmp(a, "-log")) {
            ++i;   // handled by WinMain 0x64c920
        } else if (!_stricmp(a, "-recwidth")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a numerical value", a);
            RecordWidth = atoi(argv[i]);
            if (RecordWidth < 1) Logger.g->Panic("Recording width need to be a positive number");
        } else if (!_stricmp(a, "-recheight")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a numerical value", a);
            RecordHeight = atoi(argv[i]);
            if (RecordHeight < 1) Logger.g->Panic("Recording height need to be a positive number");
        } else if (!_stricmp(a, "-packetrec")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            AssignString(PacketFile, argv[i]);
            PacketRec = true;
        } else if (!_stricmp(a, "-packetplay")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            AssignString(PacketFile, argv[i]);
            PacketRec = false; PacketPlay = true;   // word write 0x100 at +0x3c
        } else if (SwitchIs(a, "-skip")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a numerical value", a);
            Skip = atoi(argv[i]);
        } else if (SwitchIs(a, "-skipframes")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a numerical value", a);
            SkipFrames = atoi(argv[i]);
        } else if (SwitchIs(a, "-movierec")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            RecordFrames = 1;
            ForceDisplayMode = true;
            AssignString(_24, argv[i]);
        } else if (SwitchIs(a, "-csrec")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            AssignString(_40, argv[i]);
            _4c = true;
        } else if (SwitchIs(a, "-csplay")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            AssignString(_40, argv[i]);
            _4d = true;
        } else if (SwitchIs(a, "-csplay2")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            AssignString(_50, argv[i]);
            _58 = true;
        } else if (SwitchIs(a, "-fullscreen")) {
            FullScreen = true;
        } else if (SwitchIs(a, "-windowed")) {
            FullScreen = false;
        } else if (SwitchIs(a, "-fwidth")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a numerical value", a);
            FullScreenWidth = atoi(argv[i]);
        } else if (SwitchIs(a, "-fheight")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a numerical value", a);
            FullScreenHeight = atoi(argv[i]);
        } else if (SwitchIs(a, "-width")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a numerical value", a);
            WindowWidth = atoi(argv[i]);
        } else if (SwitchIs(a, "-height")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a numerical value", a);
            WindowHeight = atoi(argv[i]);
        } else if (SwitchIs(a, "-bpp")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a numerical value", a);
            FullScreenBPP = atoi(argv[i]);
        } else if (SwitchIs(a, "-eventrec")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            AssignString(EventFile, argv[i]);
            RecordEvents = true;
        } else if (SwitchIs(a, "-eventplay")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            AssignString(EventFile, argv[i]);
            PlaybackEvents = true;
        } else if (SwitchIs(a, "-script")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            AssignString(_8c, argv[i]);
            _88 = true;
        } else if (SwitchIs(a, "-cutscene")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            AssignString(CutsceneFile, argv[i]);
        } else if (SwitchIs(a, "-map")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a path specified", a);
            AssignString(MapFile, argv[i]);
        } else if (SwitchIs(a, "-miles")) {
            UseMiles = true;
        } else if (SwitchIs(a, "-server") || SwitchIs(a, "+host")) {
            HostFromCommandLine = true;
        } else if (SwitchIs(a, "-name")) {
            if (++i >= argc) Logger.g->Panic("The switch '%s' needs a name specified", a);
            AssignString(PlayerName, argv[i]);
            Save();
        } else if (SwitchIs(a, "-client") || SwitchIs(a, "+connect")) {
            ++i;
            AssignString(ConnectHost, i < argc ? argv[i] : "");
        } else if (SwitchIs(a, "-market")) {
            _a9 = true;
        } else {
            // A bare argument is a map file to load directly (full path into
            // +0x2c); a second one is fatal.
            if (MapFile.size != 0)
                Logger.g->Panic("Unknown command line parameter %s", a);
            char full[MAX_PATH];
            _fullpath(full, a, MAX_PATH);
            AssignString(MapFile, full);
        }
    }
}

// PANZERS 0x64fdd0 is the options.ini writer. Writing options.ini is not
// needed to reach the main menu; it is a logged stub (src/stubs) until the
// options submenus are lifted. Declared here, defined in
// src/stubs/stub_panzers.cpp.
