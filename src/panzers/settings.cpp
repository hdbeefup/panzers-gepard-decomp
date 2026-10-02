// src/panzers/settings.cpp
// SSettings — options.ini loader and command-line parser (HD PANZERS.exe).
// See settings.h for the layout evidence.

#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include "settings.h"
#include "properties.h"
#include "stream.h"
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
    SetKeyboardBindings(OptionsIni->GetInt("Game options", "Keyboard Bindings", 0)); // 0x64f860
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
    // "RG Pass" is XOR-decoded by 0x64ddb0/0x64d6d0 (key: the volume serial of
    // C:\) into +0x1a4. RankedGaming is out of scope: the recompile keeps the
    // encoded text as read and Save writes it back unchanged (assumed equal
    // to HD's decode/encode round trip; only an empty pass was tested).
    AssignString(RGPass, OptionsIni->GetString("Network options", "RG Pass", ""));

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

// PANZERS 0x64f860
// Stores the key-binding scheme (+0xf0) and loads its hotkeys from
// keys<n>.ini (SProperties 0x65fe80 with exit-on-error) into +0x1ac..+0x214.
void SSettings::SetKeyboardBindings(int bindings)
{
    KeyboardBindings = bindings;
    char name[32];
    _snprintf(name, sizeof(name) - 1, "keys%d.ini", bindings);   // Format 0x51ee20
    name[sizeof(name) - 1] = 0;
    SProperties keys(name, true);
    static const char* const kNames[27] = {
        "Pause", "SpeedNormal", "SpeedDouble", "Paratroops", "Bomber",
        "FighterBomber", "HeavyArtillery", "ReconPlane", "Stop", "Attack",
        "Move", "MoveBackward", "Slot1", "Unload", "Slot2", "AttackGround",
        "Heal", "Support", "Repair", "Tow", "UnTow", "StateNormal",
        "StateKnee", "StateLay", "BehaviorFreeMove", "BehaviorHoldMove",
        "BehaviorPassive",
    };
    for (int i = 0; i < 27; ++i)
        Hotkeys[i] = keys.GetInt("Keyboard bindings", kNames[i], 0);   // 0x660500
}

// Save builds options.ini in one growing buffer. Each "key = value" line is
// padded with spaces to column 40 (0x28) and followed by its comment string,
// which carries the line break; lines without a comment get "\r\n".
namespace {
struct IniWriter {
    char* buf = nullptr;
    int   len = 0;
    int   cap = 0;
    void Append(const char* s, int n)
    {
        if (len + n + 1 > cap) {
            cap = (len + n + 1) * 2;
            buf = (char*)realloc(buf, cap);
        }
        memcpy(buf + len, s, n);
        len += n;
        buf[len] = 0;
    }
    void Append(const char* s) { Append(s, (int)strlen(s)); }
    // 0x52da80 (format), pad to 0x28, 0x52c4a0/0x52c580 (concat), append
    void Line(const char* comment, const char* fmt, ...)
    {
        char tmp[0x400];
        va_list ap;
        va_start(ap, fmt);
        int n = _vsnprintf(tmp, sizeof(tmp) - 1, fmt, ap);
        va_end(ap);
        if (n < 0)
            n = sizeof(tmp) - 1;
        tmp[n] = 0;
        Append(tmp, n);
        for (int pad = 0x28 - n; pad > 0; --pad)
            Append(" ", 1);
        Append(comment);
    }
    // Lines appended as formatted, without padding (Miles provider, network).
    void Raw(const char* fmt, const char* value)
    {
        char tmp[0x400];
        int n = _snprintf(tmp, sizeof(tmp) - 1, fmt, value);
        if (n < 0)
            n = sizeof(tmp) - 1;
        tmp[n] = 0;
        Append(tmp, n);
    }
};

// (speed - base) / 0.002 + 5.0 with _DAT_008043a0 = 0.002, _DAT_00806e18 =
// 0.025 (mouse), _DAT_007f59e8 = 0.02 (keyboard), DAT_007f5a40 = 5.0; HD
// stores the float through FISTP (round to nearest).
int ScrollSpeedToIni(float speed, double base)
{
    return (int)lrintf((float)(((double)speed - base) / 0.002 + 5.0));
}

const char* Str(const SString& s)
{
    return s.buf ? s.buf : "";
}
} // namespace

// PANZERS 0x64fdd0
// Writes the whole of options.ini: SFileSystem::OpenWrite 0x65f7a0 with no
// panic string, stream vtbl +0x0c Write, then delete. Returns false when the
// file cannot be opened. Key names are HD's own: the writer emits "Unit
// acknowledgement" and "Keyboard bindings" while the loader reads "Unit
// Voice" and "Keyboard Bindings", so Unit acknowledgement does not survive a
// restart in HD either.
bool SSettings::Save()
{
    SStream* f = FileSystem.OpenWrite("options.ini", nullptr);
    if (!f)
        return false;
    IniWriter w;
    const char* const kOnOff = "; 0 = off, 1 = on\r\n";
    w.Append("[Game options]\r\n\r\n");
    w.Line("; 0-10\r\n", "Mouse scroll speed = %d", ScrollSpeedToIni(MouseScrollSpeed, 0.025));
    w.Line("; 0-10\r\n", "Keyboard scroll speed = %d", ScrollSpeedToIni(KeyboardScrollSpeed, 0.02));
    w.Line(kOnOff, "Tool tips = %d", ToolTips);
    w.Line(kOnOff, "OwnIcon = %d", OwnIcon);
    w.Line(kOnOff, "AlliedIcon = %d", AlliedIcon);
    w.Line(kOnOff, "EnemyIcon = %d", EnemyIcon);
    w.Line(kOnOff, "Show tips at startup = %d", ShowTipsAtStartup);
    w.Line("; 0 = no subtitles, 1 = only in dialogs, 2 = all subtitles\r\n", "Subtitles = %d", Subtitles);
    w.Line("; 0 = off, 1 = 5 minutes, 2 = 15 minutes, 3 = 30 minutes\r\n", "AutoSave = %d", AutoSave);
    w.Line("; 0 = off, 1 = normal, 2 = green\r\n", "Fog of war view = %d", FogOfWarView);
    w.Line("; 0 = off, 1 = no acknowledgement, 2 = all;\r\n", "Unit acknowledgement = %d", UnitVoice);
    w.Line("; 0 = classic, 1 = cdv\r\n", "Keyboard bindings = %d", KeyboardBindings);
    w.Line(kOnOff, "DynamicText = %d", DynamicText);
    w.Line("\r\n\r\n", "DynamicCharset = %s", Str(DynamicCharset));
    w.Append("[Graphics settings]\r\n\r\n");
    w.Line("; 0-10\r\n", "Brightness = %d", Brightness);
    w.Line(kOnOff, "FullScreen = %d", (int)FullScreen);
    w.Line("\r\n", "FullScreenWidth = %d", FullScreenWidth);
    w.Line("\r\n", "FullScreenHeight = %d", FullScreenHeight);
    w.Line("; 16 bit / 32 bit\r\n", "FullScreenBPP = %d", FullScreenBPP);
    w.Line("; \r\n", "FullScreenRefreshRate = %d", FullScreenRefreshRate);
    w.Line("; AntiAliasLevel\r\n", "FullScreenAALevel = %d", FullScreenAALevel);
    w.Line("; AntiAliasQualityLevel\r\n", "FullScreenAAQualityLevel = %d", FullScreenAAQualityLevel);
    w.Line(kOnOff, "FullScreenVSync = %d", (int)FullScreenVSync);
    w.Line("\r\n", "WindowWidth = %d", WindowWidth);
    w.Line("\r\n", "WindowHeight = %d", WindowHeight);
    w.Line("\r\n", "WindowX = %d", WindowX);
    w.Line("\r\n", "WindowY = %d", WindowY);
    w.Line("; AntiAliasLevel\r\n", "WindowAALevel = %d", WindowAALevel);
    w.Line("; AntiAliasQualityLevel\r\n", "WindowAAQualityLevel = %d", WindowAAQualityLevel);
    w.Line("; 0 = none, 1 = shadow, 2 = self shadow\r\n", "Shadows = %d", Shadows);
    w.Line("; 0 = low, 1 = high\r\n", "Effects detail = %d", EffectsDetail);
    w.Line("; 0 = low, 1 = high\r\n", "Texture detail = %d", TextureDetail);
    w.Line("; 0 = Bilinear, 1 = Trilinear, 2 = Anisotropic\r\n", "Texture filter = %d", TextureFilter);
    w.Line(kOnOff, "EnableTL = %d", (int)EnableTL);
    w.Line(kOnOff, "EnableHAL = %d", (int)EnableHAL);
    w.Line("\r\n", "ShadowBufferSize = %d", ShadowBufferSize);
    w.Line("; 0 = off, 1 = on\r\n\r\n", "Hardware Mouse Cursor = %d", (int)HardwareMouseCursor);
    w.Append("[Audio settings]\r\n\r\n");
    w.Line("; 0-10\r\n", "Music volume = %d", MusicVolume);
    w.Line("; 0-10\r\n", "Sound effect volume = %d", SoundEffectVolume);
    w.Line("; 0-10\r\n", "Voice volume = %d", VoiceVolume);
    w.Line(kOnOff, "Reverse channels = %d", ReverseChannels);
    w.Line(kOnOff, "Use Miles = %d", (int)UseMiles);
    w.Raw("Miles provider = %s\r\n\r\n", Str(MilesProvider));
    w.Append("[Network options]\r\n\r\n");
    w.Raw("Player name = %s\r\n", Str(PlayerName));
    w.Raw("Last host IP = %s\r\n", Str(LastHostIP));
    w.Raw("RG Login = %s\r\n", Str(RGLogin));
    // HD: 0x64ddb0 (volume-serial key) + 0x64d7d0 (XOR) encode +0x1a4 here.
    w.Raw("RG Pass = %s\r\n", Str(RGPass));
    if (w.len <= 0)
        Logger.g->Panic("SString::operator[]: invalid index (%d)", 0);
    f->Write(w.buf, w.len);
    f->Release();                                                   // HD vtbl +0 (delete)
    free(w.buf);
    return true;
}
