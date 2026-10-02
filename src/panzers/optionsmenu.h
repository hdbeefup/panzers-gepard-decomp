// src/panzers/optionsmenu.h
// The Options screens reached from the main menu (HD PANZERS.exe):
//   SMainOptionsMenu   vftable 0x807244, 0x234 bytes (new in 0x658500)
//     the right-hand "Options" menu: Game Options / Graphics / Audio / Back
//   SCenterMenu        vftable 0x808dd4, 0x58 bytes (ctor 0x64bab0)
//     base of the three pages, centred in the area left of the right menu
//   SGameOptionsMenu   vftable 0x806284, 0x6f4 bytes (new in 0x63afe0)
//   SGraphicsOptionsMenu vftable 0x806204, 0xa10 bytes (new in 0x63b060)
//   SAudioOptionsMenu  vftable 0x806184, 0x4b0 bytes (new in 0x63af60)
// The pages are shared with the in-game menu (menu.ini section
// "panzers/InGameMenu.cpp"). Create(instant) takes 1 from the main menu: no
// Ok/Back buttons, every change is written to options.ini at once, and the
// Graphics page shows Apply/Restore instead. With 0 (in-game, not lifted)
// the pages show Ok/Back (Ok/Cancel for Graphics).
//
// As elsewhere in src/panzers, the classes derive from the SWINE widget
// classes for behaviour; HD offsets are given per member.

#ifndef PANZERS_OPTIONSMENU_H
#define PANZERS_OPTIONSMENU_H

#include "mainmenu.h"
#include "pzwidgets.h"

struct SSuperWindow;

// Actions between the options screens and SSuperWindow (OnAction 0x659250).
enum PzOptionsAction {
    PZA_OPTIONS_BACK          = 0x4d4f4,   // SMainOptionsMenu Back -> main menu
    PZA_AUDIO_OK              = 0x4f411,
    PZA_AUDIO_BACK            = 0x4f412,
    PZA_AUDIO_MUSIC_VOLUME    = 0x4f413,   // param 0..10 -> concert SetVolume(0)
    PZA_AUDIO_EFFECTS_VOLUME  = 0x4f414,   // -> SetVolume(1)
    PZA_AUDIO_VOICE_VOLUME    = 0x4f415,   // -> SetVolume(2)
    PZA_AUDIO_REVERSE         = 0x4f416,   // -> concert +0x14
    PZA_GAME_OK               = 0x4f471,
    PZA_GAME_OK2              = 0x4f472,
    PZA_GAME_BACK             = 0x4f473,
    PZA_GRAPHICS_OK           = 0x4f561,
    PZA_GRAPHICS_CANCEL       = 0x4f562,
    PZA_GRAPHICS_APPLY        = 0x4f564,   // SSuperWindow applies the display settings
    PZA_GRAPHICS_BRIGHTNESS   = 0x4f565,   // param 0..10 -> Gepard +0x1c gamma
};

// HD SCenterMenu (0x58 bytes, no members past SDXWidget).
struct SCenterMenu : SDXWidget {
    SCenterMenu();                                                    // 0x64bab0
    ~SCenterMenu() override;

    void Create(const char* title, bool instant);                     // 0x64bc80
    void CreateOkCancel(SComplexButton* ok, const char* okText,
                        SComplexButton* cancel, const char* cancelText); // 0x64bf60
};

struct SGameOptionsMenu : SCenterMenu {
    bool           Instant;            // 0x58
    SComplexButton OkButton;           // 0x5c
    SComplexButton BackButton;         // 0xd0
    pz::SSliderH   MouseSpeed;         // 0x144
    pz::SSliderH   KeyboardSpeed;      // 0x1dc
    pz::SCheckBox  ToolTips;           // 0x274
    pz::SCheckBox  OwnIcon;            // 0x2e4
    pz::SCheckBox  AlliedIcon;         // 0x354
    pz::SCheckBox  EnemyIcon;          // 0x3c4
    pz::SDropList  Unused434;          // 0x434 (constructed, never created)
    pz::SDropList  KeyboardBindings;   // 0x4c0
    pz::SDropList  AutoSave;           // 0x54c
    pz::SDropList  FogOfWar;           // 0x5d8
    pz::SDropList  UnitVoice;          // 0x664

    SGameOptionsMenu();                                               // 0x62c7f0
    ~SGameOptionsMenu() override;                                     // 0x62cfb0
    bool OnAction(SWidget* source, int action, int param) override;   // +0x44 0x6313c0
    void Create(bool instant);                                        // 0x62dbf0
};

struct SGraphicsOptionsMenu : SCenterMenu {
    bool           Instant;            // 0x58
    SComplexButton OkButton;           // 0x5c
    SComplexButton CancelButton;       // 0xd0
    SComplexButton ApplyButton;        // 0x144
    SComplexButton RestoreButton;      // 0x1b8
    pz::SDropList  Resolution;         // 0x22c
    pz::SDropList  ColorDepth;         // 0x2b8
    pz::SDropList  RefreshRate;        // 0x344
    pz::SDropList  AntiAliasing;       // 0x3d0
    pz::SDropList  VSync;              // 0x45c
    pz::SDropList  Shadows;            // 0x4e8
    pz::SDropList  ShadowQuality;      // 0x574
    pz::SDropList  EffectsDetail;      // 0x600 (never created; see OnAction)
    pz::SDropList  TextureDetail;      // 0x68c
    pz::SDropList  Unused718;          // 0x718
    pz::SDropList  TextureFilter;      // 0x7a4
    pz::SCheckBox  HardwareCursor;     // 0x830
    pz::SDropList  Unused8a0;          // 0x8a0
    pz::SSliderH   Brightness;         // 0x92c
    int SelWidth, SelHeight, SelBpp, SelRefresh, SelAA, SelAAQuality;  // 0x9c4..0x9d8
    // 0x9dc..0xa0c: the settings as last applied (Restore / Cancel)
    int  SavedBrightness;              // 0x9dc
    int  SavedWidth, SavedHeight, SavedBpp, SavedRefresh;             // 0x9e0..0x9ec
    int  SavedAA, SavedAAQuality;      // 0x9f0, 0x9f4
    bool SavedVSync;                   // 0x9f8
    int  SavedShadows, SavedShadowBuffer, SavedEffects;               // 0x9fc..0xa04
    int  SavedTextureDetail, SavedTextureFilter;                      // 0xa08, 0xa0c

    SGraphicsOptionsMenu();                                           // 0x62c900
    ~SGraphicsOptionsMenu() override;                                 // 0x62d090
    bool OnAction(SWidget* source, int action, int param) override;   // +0x44 0x6316a0
    void Create(bool instant);                                        // 0x62e280
    void RefreshModeLists();                                          // 0x6325b0
};

struct SAudioOptionsMenu : SCenterMenu {
    bool           Instant;            // 0x58
    SComplexButton OkButton;           // 0x5c
    SComplexButton BackButton;         // 0xd0
    pz::SSliderH   MusicVolume;        // 0x144
    pz::SSliderH   EffectsVolume;      // 0x1dc
    pz::SSliderH   VoiceVolume;        // 0x274
    pz::SCheckBox  ReverseChannels;    // 0x30c
    pz::SListBox   Providers;          // 0x37c

    SAudioOptionsMenu();                                              // 0x62c740
    ~SAudioOptionsMenu() override;                                    // 0x62cf00
    bool OnAction(SWidget* source, int action, int param) override;   // +0x44 0x631010
    void Create(bool instant);                                        // 0x62d800
};

struct SMainOptionsMenu : SRightMenu {
    SAudioOptionsMenu*    AudioMenu;      // 0x58
    SGraphicsOptionsMenu* GraphicsMenu;   // 0x5c
    SGameOptionsMenu*     GameMenu;       // 0x60
    SComplexButton        Buttons[4];     // 0x64 Game Options, Graphics, Audio, Back

    SMainOptionsMenu();                                               // 0x6330e0
    ~SMainOptionsMenu() override;                                     // 0x633d80 (deleting 0x634aa0)
    bool OnAction(SWidget* source, int action, int param) override;   // +0x44 0x63ba50
    bool OnKeyDown(int key, bool repeat = false) override;            // recompile-only Esc

    void Create();                                                    // 0x635400
    void OpenGameOptions();                                           // 0x63afe0
    void OpenGraphicsOptions();                                       // 0x63b060
    void OpenAudioOptions();                                          // 0x63af60
    void ClosePages();
};
static_assert(0x64 + 4 * 0x74 == 0x234, "SMainOptionsMenu HD layout (new 0x234)");

// SSuperWindow::LoadMainOptionsMenu (HD 0x658500): kept here as a free
// function so superwindow.h stays as P2-D left it.
void LoadMainOptionsMenu(SSuperWindow* sw);
// SSuperWindow::OnAction (0x659250) cases for the actions above. Returns
// false for actions that are not options actions.
bool SuperWindowOptionsAction(SSuperWindow* sw, int action, int param);

#endif // PANZERS_OPTIONSMENU_H
