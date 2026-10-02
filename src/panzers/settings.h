// src/panzers/settings.h
// SSettings — Panzers' global settings object (options.ini + command line).
//
// HD PANZERS.exe keeps ONE instance as a global at 0x929cf8 (the
// SSettings::CHECKCDKEY export takes it as `this`). Field offsets below are
// the HD offsets, taken from the options.ini loader 0x64e330 (each
// `MOV [EBX+off],EAX` after a SProperties::GetInt) and from the readers in
// WinMain 0x64c920, SSuperWindow::SSuperWindow 0x656d50, SSuperWindow::Play
// 0x65b470 and SSuperWindow::Initialize 0x657910. Fields whose meaning is not
// yet known keep an offset-based name.
//
// Not the SWINE SOptions (common/options.h): that class reads SWINE keys
// ("Game settings", "Language settings") from %LOCALAPPDATA%; Panzers reads
// "Game options" / "Graphics settings" / "Audio settings" / "Network options"
// from options.ini next to the exe.

#ifndef PANZERS_SETTINGS_H
#define PANZERS_SETTINGS_H

#include <stddef.h>
#include "string2.h"

struct SProperties;

struct SSettings {
    unsigned char _00[0x08];
    bool          ForceDisplayMode;      // 0x08 set with 0x10 by a display-mode switch
    unsigned char _09[0x07];
    int           RecordFrames;          // 0x10 (non-zero: frame recording / benchmark timer)
    int           RecordWidth;           // 0x14 -recwidth  (default 640)
    int           RecordHeight;          // 0x18 -recheight (default 480)
    int           Skip;                  // 0x1c -skip
    int           _20;                   // 0x20
    SString       _24;                   // 0x24
    SString       MapFile;               // 0x2c -map or bare argument (Play 0x65b470: non-empty => load map directly)
    SString       PacketFile;            // 0x34 -packetrec / -packetplay file
    bool          PacketRec;             // 0x3c -packetrec
    bool          PacketPlay;            // 0x3d -packetplay
    unsigned char _3e[0x02];
    SString       _40;                   // 0x40
    int           _48;                   // 0x48
    bool          _4c;                   // 0x4c
    bool          _4d;                   // 0x4d
    unsigned char _4e[0x02];
    SString       _50;                   // 0x50
    bool          _58;                   // 0x58
    unsigned char _59[0x03];
    SString       EventFile;             // 0x5c event record/playback file
    bool          RecordEvents;          // 0x64 (ctor 0x656d50 -> SWindow::RecordEvents 0x544e60)
    bool          PlaybackEvents;        // 0x65 (ctor 0x656d50 -> SWindow::PlaybackEvents 0x544d60)
    unsigned char _66[0x02];
    int           SkipFrames;            // 0x68 -skipframes
    unsigned char _6c[0x1c];
    bool          _88;                   // 0x88
    unsigned char _89[0x03];
    SString       _8c;                   // 0x8c
    SString       CutsceneFile;          // 0x94 -cutscene
    bool          HostFromCommandLine;   // 0x9c (Play: -host path, 0x6576f0)
    unsigned char _9d[0x03];
    SString       ConnectHost;           // 0xa0 (Play: -connect path, 0x657460)
    bool          StartMultiFromCommandLine; // 0xa8
    bool          _a9;                   // 0xa9 (Play: "start campaign" path)
    unsigned char _aa[0x02];
    SProperties*  OptionsIni;            // 0xac
    SString       IniPath;               // 0xb0 "panzers.ini" (set 0x64f6c0, get 0x64df90)
    SString       LogDir;                // 0xb8 "log"         (set 0x64f780, get 0x64dfb0)
    // [Game options]
    float         MouseScrollSpeed;      // 0xc0
    float         KeyboardScrollSpeed;   // 0xc4
    int           ToolTips;              // 0xc8
    int           OwnIcon;               // 0xcc
    int           AlliedIcon;            // 0xd0
    int           EnemyIcon;             // 0xd4
    int           ShowTipsAtStartup;     // 0xd8
    int           Subtitles;             // 0xdc
    int           AutoSave;              // 0xe0
    int           FogOfWarView;          // 0xe4
    int           UnitVoice;             // 0xe8
    int           _ec;                   // 0xec (the loader reads "Unit Voice" into 0xe8; nothing writes 0xec)
    int           KeyboardBindings;      // 0xf0 (SetKeyboardBindings 0x64f860; getter 0x64dfe0)
    int           DynamicText;           // 0xf4
    SString       DynamicCharset;        // 0xf8
    // [Graphics settings]
    int           Brightness;            // 0x100
    bool          FullScreen;            // 0x104
    unsigned char _105[0x03];
    int           FullScreenWidth;       // 0x108
    int           FullScreenHeight;      // 0x10c
    int           FullScreenBPP;         // 0x110
    int           FullScreenRefreshRate; // 0x114
    int           FullScreenAALevel;     // 0x118
    int           FullScreenAAQualityLevel; // 0x11c
    bool          FullScreenVSync;       // 0x120
    unsigned char _121[0x03];
    int           WindowWidth;           // 0x124
    int           WindowHeight;          // 0x128
    int           WindowX;               // 0x12c
    int           WindowY;               // 0x130
    int           WindowAALevel;         // 0x134
    int           WindowAAQualityLevel;  // 0x138
    int           Shadows;               // 0x13c
    int           EffectsDetail;         // 0x140
    int           TextureDetail;         // 0x144
    int           TextureFilter;         // 0x148
    bool          EnableTL;              // 0x14c
    bool          EnableHAL;             // 0x14d
    unsigned char _14e[0x02];
    int           ShadowBufferSize;      // 0x150
    bool          HardwareMouseCursor;   // 0x154
    unsigned char _155[0x03];
    // [Audio settings]
    int           MusicVolume;           // 0x158
    int           SoundEffectVolume;     // 0x15c
    int           VoiceVolume;           // 0x160
    int           ReverseChannels;       // 0x164
    bool          UseMiles;              // 0x168
    unsigned char _169[0x03];
    SString       MilesProvider;         // 0x16c
    // [Network options]
    SString       PlayerName;            // 0x174
    SString       LastHostIP;            // 0x17c
    unsigned char _184[0x18];
    SString       RGLogin;               // 0x19c
    SString       RGPass;                // 0x1a4 RG Pass (decoded by 0x64ddb0/0x64d6d0; RankedGaming, not lifted)
    int           Hotkeys[27];           // 0x1ac..0x214 keys%d.ini [Keyboard bindings] (0x64f860)

    // PANZERS 0x64e330 (loader) — see settings.cpp
    void Initialize(int argc, char** argv);
    // PANZERS 0x64fdd0 (writer) — see settings.cpp
    bool Save();
    // PANZERS 0x64f860: KeyboardBindings + keys%d.ini hotkeys
    void SetKeyboardBindings(int bindings);

    const char* GetIniPath() const { return IniPath.buf ? IniPath.buf : ""; }  // 0x64df90
    const char* GetLogDir() const  { return LogDir.buf ? LogDir.buf : ""; }    // 0x64dfb0
    void SetIniPath(const char* path);                                          // 0x64f6c0
    void SetLogDir(const char* path);                                           // 0x64f780
};

// HD offsets (the SSettings global is at 0x929cf8; e.g. FullScreen is read
// as byte [0x929dfc] = +0x104, FullScreenWidth as [0x929e00] = +0x108).
static_assert(offsetof(SSettings, OptionsIni) == 0xac, "SSettings layout");
static_assert(offsetof(SSettings, IniPath) == 0xb0, "SSettings layout");
static_assert(offsetof(SSettings, LogDir) == 0xb8, "SSettings layout");
static_assert(offsetof(SSettings, MouseScrollSpeed) == 0xc0, "SSettings layout");
static_assert(offsetof(SSettings, DynamicCharset) == 0xf8, "SSettings layout");
static_assert(offsetof(SSettings, FullScreen) == 0x104, "SSettings layout");
static_assert(offsetof(SSettings, FullScreenVSync) == 0x120, "SSettings layout");
static_assert(offsetof(SSettings, WindowX) == 0x12c, "SSettings layout");
static_assert(offsetof(SSettings, EnableHAL) == 0x14d, "SSettings layout");
static_assert(offsetof(SSettings, UseMiles) == 0x168, "SSettings layout");
static_assert(offsetof(SSettings, MilesProvider) == 0x16c, "SSettings layout");
static_assert(offsetof(SSettings, RGLogin) == 0x19c, "SSettings layout");
static_assert(offsetof(SSettings, KeyboardBindings) == 0xf0, "SSettings layout (0x64f860 writes +0xf0)");
static_assert(offsetof(SSettings, Hotkeys) == 0x1ac && sizeof(SSettings) == 0x218, "SSettings layout (0x64f860 writes up to +0x214)");
static_assert(offsetof(SSettings, StartMultiFromCommandLine) == 0xa8, "SSettings layout");

extern SSettings Settings;   // HD global 0x929cf8

#endif // PANZERS_SETTINGS_H
