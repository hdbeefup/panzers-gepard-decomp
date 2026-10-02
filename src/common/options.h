// common/options.h
// SOptions — game options and settings
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef COMMON_OPTIONS_H
#define COMMON_OPTIONS_H

#include "core_common.h"
#include "string2.h"
#include "properties.h"
#include "stream.h"
#include "hdbeefup.h"
#include "keybinds.h"

extern int CurrentLanguage;

struct SOptions {
    SProperties* OptionsIni;        // 0x00
    float MouseScrollSpeed;         // 0x04
    float KeyboardScrollSpeed;      // 0x08
    int ShowTipsAtStartup;          // 0x0C
    int Subtitles;                  // 0x10
    int FogOfWarView;               // 0x14
    int UnitAcknowledgement;        // 0x18
    int OtherUnitVoice;             // 0x1C
    int Monitor;                    // 0x20
    int DisplayMode;                // 0x24
    int ScreenResolution;           // 0x28
    int ResolutionScale;            // 0x2C
    int VSync;                      // 0x30
    int MouseRestriction;           // 0x34
    int CursorMode;                 // 0x38
    int Antialiasing;               // 0x3C
    int AnisotropicFiltering;       // 0x40
    int Shadows;                    // 0x44
    int TexturesDetail;             // 0x48
    int MusicVolume;                // 0x4C
    int SoundEffectVolume;          // 0x50
    int VoiceVolume;                // 0x54
    int ReverseChannels;            // 0x58
    int PlaySoundInBackGround;      // 0x5C
    int StartSp;                    // 0x60
    int PeriodicSp;                 // 0x64
    unsigned char GameType;         // 0x68
    unsigned char CountMinute;      // 0x69
    bool equipment;                 // 0x6A
    bool limitedammo;               // 0x6B
    bool limitedfuel;               // 0x6C
    bool movingforce;               // 0x6D
    bool bomber;                    // 0x6E
    bool buyingingame;              // 0x6F
    bool randomstartposition;       // 0x70
    bool domination;                // 0x71
    // 2 bytes padding                 0x72-0x73
    SString PlayerName;             // 0x74
    SString LastHostIP;             // 0x7C
    bool keyboardmode;              // 0x84

#ifdef HD_HDBEEFUP_SETTINGS
    bool CameraRotation;
    bool EnableCheats;
    bool MapBorder;
    bool SkipIntro;
    bool FastMenu;
#endif

#ifdef HDB_MODLOADER_SYSTEM
    // Ordered list of active mods, slot 0 = highest priority.
    // Persisted in options.ini as "Active mods = A;B;C".
    SString* ActiveMods;
    int ActiveModCount;
    int ActiveModCapacity;
    // Folder code under langpack/ when a fan-made language is selected
    // (empty = use the base-game Language enum instead).
    SString LanguageLangPack;
#endif

#ifdef HD_KEYBINDS
    int ControlScheme;               // EControlScheme
    SKeyBindings CustomBindings;     // used when ControlScheme == CS_CUSTOM
    SKeyBindings ActiveBindings;     // resolved from ControlScheme on load/change
    void RefreshActiveBindings();
#endif

    SOptions();
    ~SOptions();

    // Getters
    int GetAnisotropicFiltering();
    int GetAntialiasing();
    unsigned char GetCountMinute();
    int GetCursorMode();
    int GetDisplayMode();
    int GetFogOfWarView();
    unsigned char GetGameType();
    bool GetKeyboardMode();
    float GetKeyboardScrollSpeed();
    char* GetLastHostIP();
    int GetMonitor();
    int GetMouseRestriction();
    float GetMouseScrollSpeed();
    int GetMusicVolume();
    int GetOtherUnitVoice();
    int GetPeriodicSp();
    int GetPlaySoundInBackground();
    char* GetPlayerName();
    float GetResolutionScale();
    int GetReverseChannels();
    int GetScreenResolution();
    int GetShadows();
    int GetShowTipsAtStartup();
    int GetSoundEffectVolume();
    int GetStartSp();
    int GetSubtitles();
    int GetTexturesDetail();
    int GetUnitAcknowledgement();
    int GetVSync();
    int GetVoiceVolume();

    // Setters
    void SetAnisotropicFiltering(int num);
    void SetAntialiasing(int num);
    void SetBomberEnabled(bool enabled);
    void SetBuyingingameEnabled(bool enabled);
    void SetCountMinute(unsigned char countMinute);
    void SetCursorMode(int num);
    void SetDisplayMode(int num);
    void SetDominationEnabled(bool enabled);
    void SetEquipmentEnabled(bool enabled);
    void SetFogOfWarView(int num);
    void SetGameType(unsigned char gameType);
    void SetKeyboardMode(bool on);
    void SetKeyboardScrollSpeed(int num);
    void SetLastHostIP(const char* str);
    void SetLimitedammoEnabled(bool enabled);
    void SetLimitedfuelEnabled(bool enabled);
    void SetMonitor(int num);
    void SetMouseRestriction(int num);
    void SetMouseScrollSpeed(int num);
    void SetMovingforceEnabled(bool enabled);
    void SetMusicVolume(int num);
    void SetOtherUnitVoice(int num);
    void SetPeriodicSp(int periodicSp);
    void SetPlaySoundInBackground(int num);
    void SetPlayerName(const char* str);
    void SetRandomStartPositionEnabled(bool enabled);
    void SetResolutionScale(float num);
    void SetReverseChannels(int num);
    void SetScreenResolution(int num);
    void SetShadows(int num);
    void SetShowTipsAtStartup(int num);
    void SetSoundEffectVolume(int num);
    void SetStartSp(int startSp);
    void SetSubtitles(int num);
    void SetTexturesDetail(int num);
    void SetUnitAcknowledgement(int num);
    void SetVSync(int num);
    void SetVoiceVolume(int num);

    // Query methods
    bool isBomberEnabled();
    bool isBuyingingameEnabled();
    bool isDominationEnabled();
    bool isEquipmentEnabled();
    bool isLimitedFuelEnabled();
    bool isLimitedammoEnabled();
    bool isMovingforceEnabled();
    bool isRandomStartPositionEnabled();

#ifdef HD_HDBEEFUP_SETTINGS
    bool GetCameraRotation();
    bool GetEnableCheats();
    bool GetMapBorder();
    void SetCameraRotation(bool enabled);
    void SetEnableCheats(bool enabled);
    void SetMapBorder(bool enabled);
#endif

#ifdef HDB_MODLOADER_SYSTEM
    int GetActiveModCount() const;
    const char* GetActiveMod(int index) const;          // index 0 = highest priority
    void SetActiveMods(const char* const* names, int count);
#endif

#ifdef HD_KEYBINDS
    int GetControlScheme();
    void SetControlScheme(int scheme);
    const SKeyBindings& GetActiveBindings() const { return ActiveBindings; }
#endif

    // I/O
    int WriteOptionsIni();
};

#endif // COMMON_OPTIONS_H
