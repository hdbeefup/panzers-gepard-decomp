// src/panzers/optionsmenu.cpp
// The Options screens (HD PANZERS.exe); see optionsmenu.h.

#include <windows.h>
#include <d3d9.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#include "optionsmenu.h"
#include "superwindow.h"
#include "settings.h"
#include "pzboard.h"
#include "gepard.h"
#include "iconcert.h"
#include "milesconcert.h"
#include "logger.h"
#include "window.h"

void PzStub_ApplyGraphicsOptions(int shadows, int shadowBuffer, int textureFilter,
                                 int textureDetail, bool hardwareCursor);   // src/stubs

static const char* const kInGame = "panzers/InGameMenu.cpp";

static const char* Tx(const char* id)
{
    return GetText(kInGame, id);
}

// HD: CreateFrame(2, BackFrame, x, y, 0, 1) + SetText(frame, font 3, 0, text, 0)
static void AddLabel(SDXWidget* w, int x, int y, const char* text)
{
    int f = Board->CreateFrame(FT_TEXT, w->BackFrame, x, y, 0, 1);
    Board->SetText(f, g_PzFont[PZF_SANS21_SHADOW], 0, text ? text : "");
}

// ---------------------------------------------------------------------------
// Display-mode lists of the primary viewport (HD SViewport, built by 0x689f80
// when the device is created). The SWINE renderer has no SViewport, so the
// lists are built the same way from SGepard's IDirect3D9 on first use.
// ---------------------------------------------------------------------------

namespace {
struct PzMode {
    int w, h;
    std::vector<int> refresh16;   // HD entry +0x08 (D3DFMT_R5G6B5)
    std::vector<int> refresh32;   // HD entry +0x14 (D3DFMT_X8R8G8B8)
};
struct PzAAMode { int level, quality; };
struct PzModeLists {
    bool built = false;
    std::vector<PzMode>   modes;     // HD viewport +0x204
    std::vector<PzAAMode> aa16;      // HD viewport +0x228 (R5G6B5, fullscreen)
    std::vector<PzAAMode> aa32;      // HD viewport +0x234 (X8R8G8B8, fullscreen)
};
PzModeLists s_Modes;
}

// PANZERS 0x689f80
static PzModeLists& PzGetModeLists()
{
    if (s_Modes.built)
        return s_Modes;
    s_Modes.built = true;
    SGepard* g = static_cast<SGepard*>(Gepard);
    if (!g || !g->lpD3D)
        return s_Modes;
    IDirect3D9* d3d = g->lpD3D;
    const D3DFORMAT fmts[2] = { D3DFMT_X8R8G8B8, D3DFMT_R5G6B5 };
    for (D3DFORMAT fmt : fmts) {
        UINT n = d3d->GetAdapterModeCount(g->Adapter, fmt);           // IDirect3D9 +0x18
        for (UINT i = 0; i < n; ++i) {
            D3DDISPLAYMODE m;
            d3d->EnumAdapterModes(g->Adapter, fmt, i, &m);            // IDirect3D9 +0x1c
            if (m.Format != fmt)
                continue;
            PzMode* e = nullptr;
            for (PzMode& x : s_Modes.modes)
                if (x.w == (int)m.Width && x.h == (int)m.Height) { e = &x; break; }
            if (!e) {
                s_Modes.modes.push_back(PzMode{ (int)m.Width, (int)m.Height, {}, {} });
                e = &s_Modes.modes.back();
            }
            (fmt == D3DFMT_X8R8G8B8 ? e->refresh32 : e->refresh16).push_back((int)m.RefreshRate);
        }
    }
    // Multisample levels 1..15 and their quality levels (IDirect3D9 +0x2c).
    // HD also keeps the windowed lists (+0x21c / +0x210); the menu reads only
    // the full-screen ones.
    for (int level = 1; level <= 0xf; ++level) {
        DWORD q = 0;
        if (SUCCEEDED(d3d->CheckDeviceMultiSampleType(g->Adapter, g->DeviceType, D3DFMT_X8R8G8B8,
                                                      FALSE, (D3DMULTISAMPLE_TYPE)level, &q)))
            for (DWORD i = 0; i < q; ++i)
                s_Modes.aa32.push_back(PzAAMode{ level, (int)i });
        q = 0;
        if (SUCCEEDED(d3d->CheckDeviceMultiSampleType(g->Adapter, g->DeviceType, D3DFMT_R5G6B5,
                                                      FALSE, (D3DMULTISAMPLE_TYPE)level, &q)))
            for (DWORD i = 0; i < q; ++i)
                s_Modes.aa16.push_back(PzAAMode{ level, (int)i });
    }
    return s_Modes;
}

// PANZERS 0x68b220 (viewport +0x68): bit depths available at w x h.
static std::vector<int> PzGetBitDepths(int w, int h)
{
    std::vector<int> r;
    for (const PzMode& m : PzGetModeLists().modes)
        if (m.w == w && m.h == h) {
            if (!m.refresh16.empty()) r.push_back(16);
            if (!m.refresh32.empty()) r.push_back(32);
        }
    return r;
}

// PANZERS 0x68b3a0 (viewport +0x6c)
static std::vector<int> PzGetRefreshRates(int w, int h, int bpp)
{
    for (const PzMode& m : PzGetModeLists().modes)
        if (m.w == w && m.h == h)
            return bpp == 32 ? m.refresh32 : m.refresh16;
    return {};
}

// PANZERS 0x68b350 (viewport +0x70)
static const std::vector<PzAAMode>& PzGetAAModes(int bpp)
{
    PzModeLists& l = PzGetModeLists();
    return bpp == 32 ? l.aa32 : l.aa16;
}

// PANZERS 0x680540
// SGepard brightness (vtbl +0x1c): a gamma ramp pow(i/255, 1/gamma) * 65535,
// truncated (FLDCW 0x7f0c), the same on all three channels, set through
// IDirect3DDevice9::SetGammaRamp (+0x54). D3D9 applies it only in full
// screen, as in HD.
static void PzSetBrightness(float gamma)
{
    SGepard* g = static_cast<SGepard*>(Gepard);
    if (!g || !g->lpD3DDev)
        return;
    D3DGAMMARAMP ramp;
    float inv = (float)(1.0 / (double)gamma);                      // _DAT_007eed98 = 1.0
    for (int i = 0; i < 0x100; ++i) {
        // _DAT_0081a340 = 1/255, _DAT_0081a348 = 65535.0
        double v = pow((double)((float)i * 0.003921568859368563f), (double)inv) * 65535.0;
        WORD w = (WORD)(int)(float)v;
        ramp.red[i] = ramp.green[i] = ramp.blue[i] = w;
    }
    g->lpD3DDev->SetGammaRamp(0, 0, &ramp);
}

// ---------------------------------------------------------------------------
// SCenterMenu
// ---------------------------------------------------------------------------

// PANZERS 0x64bab0
SCenterMenu::SCenterMenu()
{
    ToolTipFeatureEnabled = false;
}

SCenterMenu::~SCenterMenu()
{
}

// PANZERS 0x64bc80
void SCenterMenu::Create(const char* title, bool instant)
{
    SetBackgroundSprite(g_MenuMediumTex, 0, false, false);           // 0x539ba0
    SDXWidget::Create(0);
    int area, y;
    if (!instant) {
        y = 0x5f;
        area = 0x400;
    } else {
        y = 0x96;
        area = 0x2a0;          // the part of the screen left of SRightMenu
    }
    SetPosition((area - Width) / 2, y, Width, Height);               // vtbl +0x08
    int header = Board->CreateFrame(FT_SPRITE, BackFrame, (Width - 0x124) / 2, 0, 0, 0);
    Board->SetSpriteGlyph(header, g_MenuControlsFont, 9);
    int text = Board->CreateFrame(FT_TEXT, header, 0x92, 0xc, 0, 0);
    Board->SetText(text, g_PzFont[PZF_SANS21], 2, title ? title : "");
    Board->SetTextColor(text, 0xffffff);
}

// PANZERS 0x64bf60
void SCenterMenu::CreateOkCancel(SComplexButton* ok, const char* okText,
                                 SComplexButton* cancel, const char* cancelText)
{
    InsertChild(ok);
    ok->SetPosition(199, 400, 0, 0);
    ok->Create(1, okText);
    InsertChild(cancel);
    cancel->SetPosition(0x18b, 400, 0, 0);
    cancel->Create(1, cancelText);
}

// ---------------------------------------------------------------------------
// SGameOptionsMenu
// ---------------------------------------------------------------------------

// PANZERS 0x62c7f0
SGameOptionsMenu::SGameOptionsMenu()
{
    Instant = false;
}

// PANZERS 0x62cfb0
SGameOptionsMenu::~SGameOptionsMenu()
{
}

// _DAT_00806e18 = 0.025, _DAT_007f59e8 = 0.02, _DAT_008043a0 = 0.002,
// DAT_007f5a40 = 5.0, DAT_007ea760 = 0.5 (truncating conversion).
static int SpeedToSlider(float speed, double base)
{
    return (int)(((double)speed - base) / 0.002 + 5.0 + 0.5);
}

// PANZERS 0x64fc20 / 0x64fbc0
static float SliderToSpeed(int v, double base)
{
    return (float)((double)(v - 5) * 0.002 + base);
}

// PANZERS 0x62dbf0
void SGameOptionsMenu::Create(bool instant)
{
    Instant = instant;
    SCenterMenu::Create(Tx("Game Options"), instant);
    if (!Instant)
        CreateOkCancel(&OkButton, Tx("Ok"), &BackButton, Tx("Back"));

    AddLabel(this, 0x50, 0x3c, Tx("Mouse scroll speed"));
    InsertChild(&MouseSpeed);
    MouseSpeed.Create(0x127, 0x3c, 0xb4, 10, SpeedToSlider(Settings.MouseScrollSpeed, 0.025));
    AddLabel(this, 0x50, 0x5a, Tx("Keyboard Scroll speed"));
    InsertChild(&KeyboardSpeed);
    KeyboardSpeed.Create(0x127, 0x5a, 0xb4, 10, SpeedToSlider(Settings.KeyboardScrollSpeed, 0.02));

    struct { pz::SCheckBox* box; int y; const char* id; int value; } checks[4] = {
        { &ToolTips,   0x7c, "Tooltips",                               Settings.ToolTips },
        { &OwnIcon,    0x9e, "Flag and level stars for my own units",  Settings.OwnIcon },
        { &AlliedIcon, 0xc0, "Flag and level stars for allied units",  Settings.AlliedIcon },
        { &EnemyIcon,  0xe2, "Flag and level stars for enemy units",   Settings.EnemyIcon },
    };
    for (auto& c : checks) {
        InsertChild(c.box);
        c.box->SetPosition(0x50, c.y, 0, 0);
        c.box->SetText(Tx(c.id));
        c.box->Create();
        c.box->SetCheck(c.value != 0);
    }

    AddLabel(this, 0x50, 0x10c, Tx("Keyboard bindings"));
    InsertChild(&KeyboardBindings);
    KeyboardBindings.SetPosition(0x127, 0x107, 0xaa, 0x14);
    KeyboardBindings.AddItem(Tx("Classic"), 0);
    KeyboardBindings.AddItem(Tx("CDV"), 0);
    KeyboardBindings.SetCurSel(Settings.KeyboardBindings);
    KeyboardBindings.Create();

    AddLabel(this, 0x50, 0x12a, Tx("Autosave"));
    InsertChild(&AutoSave);
    AutoSave.SetPosition(0x127, 0x125, 0xaa, 0x14);
    AutoSave.AddItem(Tx("Disabled"), 0);
    AutoSave.AddItem(Tx("5 minutes"), 0);
    AutoSave.AddItem(Tx("15 minutes"), 0);
    AutoSave.AddItem(Tx("30 minutes"), 0);
    AutoSave.SetCurSel(Settings.AutoSave);
    AutoSave.Create();

    AddLabel(this, 0x50, 0x148, Tx("Fog of war view"));
    InsertChild(&FogOfWar);
    FogOfWar.SetPosition(0x127, 0x143, 0xaa, 0x14);
    FogOfWar.AddItem(Tx("Disabled"), 0);
    FogOfWar.AddItem(Tx("Normal"), 0);
    FogOfWar.AddItem(Tx("Green"), 0);
    FogOfWar.SetCurSel(Settings.FogOfWarView);
    FogOfWar.Create();

    AddLabel(this, 0x50, 0x166, Tx("Unit acknowledgement"));
    InsertChild(&UnitVoice);
    UnitVoice.SetPosition(0x127, 0x161, 0xaa, 0x14);
    UnitVoice.AddItem(Tx("No"), 0);
    UnitVoice.AddItem(Tx("No acknowledgement"), 0);
    UnitVoice.AddItem(Tx("All"), 0);
    UnitVoice.SetCurSel(Settings.UnitVoice);
    UnitVoice.Create();
}

// PANZERS 0x6313c0
bool SGameOptionsMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action == PZA_BUTTON_CLICK) {
        if (source == &OkButton) {
            Settings.MouseScrollSpeed = SliderToSpeed(MouseSpeed.GetValue(), 0.025);       // 0x64fc20
            Settings.KeyboardScrollSpeed = SliderToSpeed(KeyboardSpeed.GetValue(), 0.02);  // 0x64fbc0
            Settings.ToolTips = ToolTips.GetCheck();                                       // 0x64fd30
            Settings.OwnIcon = OwnIcon.GetCheck();                                         // 0x64fc70
            Settings.AlliedIcon = AlliedIcon.GetCheck();                                   // 0x64f450
            Settings.EnemyIcon = EnemyIcon.GetCheck();                                     // 0x64f600
            Settings.SetKeyboardBindings(KeyboardBindings.GetCurSel());                    // 0x64f860
            Settings.AutoSave = AutoSave.GetCurSel();                                      // 0x64f460
            Settings.FogOfWarView = FogOfWar.GetCurSel();                                  // 0x64f610
            Settings.UnitVoice = UnitVoice.GetCurSel();                                    // 0x64fd40
            Settings.Save();                                                               // 0x64fdd0
            SendAction(PZA_GAME_OK2, 0);
            SendAction(PZA_GAME_OK, 0);
            return true;
        }
        if (source == &BackButton) {
            SendAction(PZA_GAME_BACK, 0);
            return true;
        }
        if ((source == &ToolTips || source == &OwnIcon || source == &AlliedIcon || source == &EnemyIcon)
            && Instant) {
            Settings.ToolTips = ToolTips.GetCheck();
            Settings.OwnIcon = OwnIcon.GetCheck();
            Settings.AlliedIcon = AlliedIcon.GetCheck();
            Settings.EnemyIcon = EnemyIcon.GetCheck();
            Settings.Save();
            return false;
        }
    } else if ((action == PZA_DROPLIST_SELECT || action == PZA_LISTBOX_SELECT
                || action == PZA_SLIDER_CHANGED) && Instant) {
        Settings.MouseScrollSpeed = SliderToSpeed(MouseSpeed.GetValue(), 0.025);
        Settings.KeyboardScrollSpeed = SliderToSpeed(KeyboardSpeed.GetValue(), 0.02);
        Settings.SetKeyboardBindings(KeyboardBindings.GetCurSel());
        Settings.AutoSave = AutoSave.GetCurSel();
        Settings.FogOfWarView = FogOfWar.GetCurSel();
        Settings.UnitVoice = UnitVoice.GetCurSel();
        Settings.Save();
    }
    return false;
}

// ---------------------------------------------------------------------------
// SGraphicsOptionsMenu
// ---------------------------------------------------------------------------

// PANZERS 0x62c900
SGraphicsOptionsMenu::SGraphicsOptionsMenu()
{
    Instant = false;
    SelWidth = SelHeight = SelBpp = SelRefresh = SelAA = SelAAQuality = 0;
    SavedBrightness = SavedWidth = SavedHeight = SavedBpp = SavedRefresh = 0;
    SavedAA = SavedAAQuality = 0;
    SavedVSync = false;
    SavedShadows = SavedShadowBuffer = SavedEffects = SavedTextureDetail = SavedTextureFilter = 0;
}

// PANZERS 0x62d090
SGraphicsOptionsMenu::~SGraphicsOptionsMenu()
{
}

// PANZERS 0x62e280
void SGraphicsOptionsMenu::Create(bool instant)
{
    Instant = instant;
    SCenterMenu::Create(Tx("Graphics"), instant);
    if (!Instant)
        CreateOkCancel(&OkButton, Tx("Ok"), &CancelButton, Tx("Cancel"));
    else
        CreateOkCancel(&ApplyButton, Tx("Apply"), &RestoreButton, Tx("Restore"));

    AddLabel(this, 0x50, 0x3c, Tx("Brightness"));
    InsertChild(&Brightness);
    Brightness.Create(0xfa, 0x3c, 0xaa, 10, Settings.Brightness);

    AddLabel(this, 0x50, 0x5a, Tx("Screen Resolution"));
    InsertChild(&Resolution);
    Resolution.SetPosition(0xfa, 0x55, 0xaa, 0x14);
    for (const PzMode& m : PzGetModeLists().modes) {                  // viewport +0x64 (0x68b470)
        char buf[32];
        _snprintf(buf, sizeof(buf) - 1, "%dx%d", m.w, m.h);
        buf[sizeof(buf) - 1] = 0;
        int i = Resolution.AddItem(buf, (unsigned)(m.w * 0x10000 + m.h));
        if (m.w == Settings.FullScreenWidth && m.h == Settings.FullScreenHeight)
            Resolution.SetCurSel(i);
    }
    Resolution.Create();

    AddLabel(this, 0x50, 0x78, Tx("Color Depth"));
    InsertChild(&ColorDepth);
    ColorDepth.SetPosition(0xfa, 0x73, 0xaa, 0x14);
    ColorDepth.Create();

    AddLabel(this, 0x50, 0x96, Tx("Refresh Rate"));
    InsertChild(&RefreshRate);
    RefreshRate.SetPosition(0xfa, 0x91, 0xaa, 0x14);
    RefreshRate.Create();

    AddLabel(this, 0x50, 0xb4, Tx("Anti-aliasing"));
    InsertChild(&AntiAliasing);
    AntiAliasing.SetPosition(0xfa, 0xaf, 0xaa, 0x14);
    AntiAliasing.Create();

    AddLabel(this, 0x50, 0xd2, Tx("VSync"));
    InsertChild(&VSync);
    VSync.SetPosition(0xfa, 0xcd, 0xaa, 0x14);
    VSync.AddItem(Tx("Off"), 0);
    VSync.AddItem(Tx("On"), 0);
    VSync.SetCurSel(Settings.FullScreenVSync);
    VSync.Create();

    AddLabel(this, 0x50, 0xf0, Tx("Shadows"));
    InsertChild(&Shadows);
    Shadows.SetPosition(0xfa, 0xeb, 0xaa, 0x14);
    Shadows.AddItem(Tx("None"), 0);
    Shadows.AddItem(Tx("Normal Shadow"), 0);
    // HD: Gepard GetCap(0) (0x67a7d0, SGepard +0x540) > 1 offers self
    // shadows. The cap is the shadow technique picked by PS version in
    // SGepard::Initialize; the SWINE renderer has no such cap and a D3D9
    // device here always has PS 2.0, so the HD result (2) is assumed.
    const int shadowCap = 2;
    if (shadowCap > 1)
        Shadows.AddItem(Tx("Self Shadow"), 0);
    Shadows.SetCurSel(Settings.Shadows);
    Shadows.Create();

    AddLabel(this, 0x50, 0x10e, Tx("Shadow quality"));
    InsertChild(&ShadowQuality);
    ShadowQuality.SetPosition(0xfa, 0x109, 0xaa, 0x14);
    ShadowQuality.AddItem(Tx("Normal"), 0);
    ShadowQuality.AddItem(Tx("High"), 0);
    ShadowQuality.SetCurSel(Settings.ShadowBufferSize == 0x800);
    ShadowQuality.Create();

    AddLabel(this, 0x50, 300, Tx("Texture detail"));
    InsertChild(&TextureDetail);
    TextureDetail.SetPosition(0xfa, 0x127, 0xaa, 0x14);
    TextureDetail.AddItem(Tx("Low"), 0);
    TextureDetail.AddItem(Tx("High"), 0);
    TextureDetail.SetCurSel(Settings.TextureDetail);
    TextureDetail.Create();

    AddLabel(this, 0x50, 0x14a, Tx("Texture filtering"));
    InsertChild(&TextureFilter);
    TextureFilter.SetPosition(0xfa, 0x145, 0xaa, 0x14);
    TextureFilter.AddItem(Tx("Bilinear"), 0);
    TextureFilter.AddItem(Tx("Trilinear"), 0);
    TextureFilter.AddItem(Tx("Anisotropic"), 0);
    TextureFilter.SetCurSel(Settings.TextureFilter);
    TextureFilter.Create();

    InsertChild(&HardwareCursor);
    HardwareCursor.SetPosition(0x50, 0x168, 0x154, 0x14);
    HardwareCursor.SetText(Tx("Hardware Mouse Cursor"));
    HardwareCursor.SetCheck(Settings.HardwareMouseCursor);
    HardwareCursor.Create();

    SelWidth = Settings.FullScreenWidth;
    SelHeight = Settings.FullScreenHeight;
    SelBpp = Settings.FullScreenBPP;
    SelRefresh = Settings.FullScreenRefreshRate;
    SelAA = Settings.FullScreenAALevel;
    SelAAQuality = Settings.FullScreenAAQualityLevel;
    SavedBrightness = Settings.Brightness;
    SavedWidth = Settings.FullScreenWidth;
    SavedHeight = Settings.FullScreenHeight;
    SavedBpp = Settings.FullScreenBPP;
    SavedRefresh = Settings.FullScreenRefreshRate;
    SavedAA = Settings.FullScreenAALevel;
    SavedAAQuality = Settings.FullScreenAAQualityLevel;
    SavedVSync = Settings.FullScreenVSync;
    SavedShadows = Settings.Shadows;
    SavedShadowBuffer = Settings.ShadowBufferSize;
    SavedEffects = Settings.EffectsDetail;
    SavedTextureDetail = Settings.TextureDetail;
    SavedTextureFilter = Settings.TextureFilter;
    RefreshModeLists();
}

// PANZERS 0x6325b0
// Refills Color Depth / Refresh Rate / Anti-aliasing for the selected
// resolution (SelWidth x SelHeight) and selects SelBpp / SelRefresh / SelAA.
void SGraphicsOptionsMenu::RefreshModeLists()
{
    ColorDepth.ResetContent();
    for (int bpp : PzGetBitDepths(SelWidth, SelHeight)) {
        char buf[64];
        _snprintf(buf, sizeof(buf) - 1, Tx("%d bits"), bpp);
        buf[sizeof(buf) - 1] = 0;
        int i = ColorDepth.AddItem(buf, (unsigned)bpp);
        if (bpp == SelBpp)
            ColorDepth.SetCurSel(i);
    }

    RefreshRate.ResetContent();
    RefreshRate.AddItem(Tx("Default"), 0);
    RefreshRate.SetCurSel(0);
    for (int hz : PzGetRefreshRates(SelWidth, SelHeight, SelBpp)) {
        char buf[64];
        _snprintf(buf, sizeof(buf) - 1, Tx("%d Hertz"), hz);
        buf[sizeof(buf) - 1] = 0;
        int i = RefreshRate.AddItem(buf, (unsigned)hz);
        if (hz == SelRefresh)
            RefreshRate.SetCurSel(i);
    }

    AntiAliasing.ResetContent();
    // HD reads the bit depth through GetItemData(GetCurSel()), which panics
    // ("SDropList::GetItemData: Invalid index") when nothing is selected.
    int bpp = (int)ColorDepth.GetItemData(ColorDepth.GetCurSel());
    AntiAliasing.AddItem(Tx("Disabled"), 0);
    AntiAliasing.SetCurSel(0);
    for (const PzAAMode& m : PzGetAAModes(bpp)) {
        char buf[64];
        _snprintf(buf, sizeof(buf) - 1, "%dX (%d)", m.level, m.quality);
        buf[sizeof(buf) - 1] = 0;
        int i = AntiAliasing.AddItem(buf, (unsigned)(m.level * 0x10000 + m.quality));
        if (m.level == SelAA && m.quality == SelAAQuality)
            AntiAliasing.SetCurSel(i);
    }
}

// PANZERS 0x6316a0
bool SGraphicsOptionsMenu::OnAction(SWidget* source, int action, int param)
{
    if (action == PZA_BUTTON_CLICK) {
        if (source == &OkButton || source == &ApplyButton) {
            Settings.Brightness = Brightness.GetValue();                                   // 0x64f470
            unsigned res = Resolution.GetItemData(Resolution.GetCurSel());
            Settings.FullScreenWidth = (int)(res >> 16);                                   // 0x64f6b0
            Settings.FullScreenHeight = (int)(unsigned short)res;                          // 0x64f670
            Settings.FullScreenBPP = (int)ColorDepth.GetItemData(ColorDepth.GetCurSel());  // 0x64f660
            Settings.FullScreenRefreshRate = (int)RefreshRate.GetItemData(RefreshRate.GetCurSel()); // 0x64f680
            unsigned aa = AntiAliasing.GetItemData(AntiAliasing.GetCurSel());
            Settings.FullScreenAALevel = (int)(aa >> 16);                                  // 0x64f640
            Settings.FullScreenAAQualityLevel = (int)(unsigned short)aa;                   // 0x64f650
            Settings.FullScreenVSync = VSync.GetCurSel() != 0;                             // 0x64f690
            Settings.Shadows = Shadows.GetCurSel();                                        // 0x64fcd0
            Settings.ShadowBufferSize = ShadowQuality.GetCurSel() != 0 ? 0x800 : 0x400;    // 0x64fcc0
            // HD reads Effects detail from the drop list at +0x600, which
            // Create never builds, so Apply stores -1 (kept as in HD).
            Settings.EffectsDetail = EffectsDetail.GetCurSel();                            // 0x64f5b0
            Settings.TextureDetail = TextureDetail.GetCurSel();                            // 0x64fd10
            Settings.TextureFilter = TextureFilter.GetCurSel();                            // 0x64fd20
            Settings.HardwareMouseCursor = HardwareCursor.GetCheck();                      // 0x64f840
            Settings.Save();                                                               // 0x64fdd0
            SendAction(PZA_GRAPHICS_APPLY, 0);
            if (source != &OkButton) {
                SavedBrightness = Settings.Brightness;
                SavedWidth = Settings.FullScreenWidth;
                SavedHeight = Settings.FullScreenHeight;
                SavedBpp = Settings.FullScreenBPP;
                SavedRefresh = Settings.FullScreenRefreshRate;
                SavedAA = Settings.FullScreenAALevel;
                SavedAAQuality = Settings.FullScreenAAQualityLevel;
                SavedVSync = Settings.FullScreenVSync;
                SavedShadows = Settings.Shadows;
                SavedShadowBuffer = Settings.ShadowBufferSize;
                SavedEffects = Settings.EffectsDetail;
                SavedTextureDetail = Settings.TextureDetail;
                SavedTextureFilter = Settings.TextureFilter;
                return true;
            }
            SendAction(PZA_GRAPHICS_OK, 0);
            return true;
        }
        if (source == &CancelButton) {
            SendAction(PZA_GRAPHICS_BRIGHTNESS, SavedBrightness);
            SendAction(PZA_GRAPHICS_CANCEL, 0);
            return true;
        }
        if (source == &RestoreButton) {
            Settings.Brightness = SavedBrightness;
            Settings.FullScreenWidth = SavedWidth;
            Settings.FullScreenHeight = SavedHeight;
            Settings.FullScreenBPP = SavedBpp;
            Settings.FullScreenRefreshRate = SavedRefresh;
            Settings.FullScreenAALevel = SavedAA;
            Settings.FullScreenAAQualityLevel = SavedAAQuality;
            Settings.FullScreenVSync = SavedVSync;
            Settings.Shadows = SavedShadows;
            Settings.ShadowBufferSize = SavedShadowBuffer;
            Settings.EffectsDetail = SavedEffects;
            Settings.TextureDetail = SavedTextureDetail;
            Settings.TextureFilter = SavedTextureFilter;
            Brightness.SetValue(Settings.Brightness);
            for (int i = 0; i < Resolution.GetCount(); ++i)
                if (Resolution.GetItemData(i) == (unsigned)(SavedWidth * 0x10000 + SavedHeight)) {
                    Resolution.SetCurSel(i);
                    break;
                }
            SelWidth = Settings.FullScreenWidth;
            SelHeight = Settings.FullScreenHeight;
            SelBpp = Settings.FullScreenBPP;
            SelRefresh = Settings.FullScreenRefreshRate;
            SelAA = Settings.FullScreenAALevel;
            SelAAQuality = Settings.FullScreenAAQualityLevel;
            RefreshModeLists();
            VSync.SetCurSel(Settings.FullScreenVSync);
            Shadows.SetCurSel(Settings.Shadows);
            ShadowQuality.SetCurSel(Settings.ShadowBufferSize == 0x800);
            EffectsDetail.SetCurSel(Settings.EffectsDetail);
            TextureDetail.SetCurSel(Settings.TextureDetail);
            TextureFilter.SetCurSel(Settings.TextureFilter);
            HardwareCursor.SetCheck(Settings.HardwareMouseCursor);
            SendAction(PZA_GRAPHICS_BRIGHTNESS, SavedBrightness);
            SendAction(PZA_GRAPHICS_APPLY, 0);
            return true;
        }
    } else if (action == PZA_SLIDER_CHANGED) {
        if (source == &Brightness) {
            SendAction(PZA_GRAPHICS_BRIGHTNESS, param);
            return false;
        }
    } else if (action == PZA_DROPLIST_SELECT) {
        unsigned res = Resolution.GetItemData(Resolution.GetCurSel());
        SelWidth = (int)(res >> 16);
        SelHeight = (int)(res & 0xffff);
        SelBpp = (int)ColorDepth.GetItemData(ColorDepth.GetCurSel());
        SelRefresh = (int)RefreshRate.GetItemData(RefreshRate.GetCurSel());
        unsigned aa = AntiAliasing.GetItemData(AntiAliasing.GetCurSel());
        SelAA = (int)(aa >> 16);
        SelAAQuality = (int)(aa & 0xffff);
        RefreshModeLists();
    }
    return false;
}

// ---------------------------------------------------------------------------
// SAudioOptionsMenu
// ---------------------------------------------------------------------------

// PANZERS 0x62c740
SAudioOptionsMenu::SAudioOptionsMenu()
{
    Instant = false;
}

// PANZERS 0x62cf00
SAudioOptionsMenu::~SAudioOptionsMenu()
{
}

static SIPanzersConcert* PzConcert()
{
    return dynamic_cast<SIPanzersConcert*>(Concert);
}

// PANZERS 0x62d800
void SAudioOptionsMenu::Create(bool instant)
{
    Instant = instant;
    SCenterMenu::Create(Tx("Audio"), instant);
    if (!Instant)
        CreateOkCancel(&OkButton, Tx("Ok"), &BackButton, Tx("Back"));

    AddLabel(this, 0x50, 0x5a, Tx("Music volume"));
    InsertChild(&MusicVolume);
    MusicVolume.Create(0x127, 0x5a, 0xb4, 10, Settings.MusicVolume);
    AddLabel(this, 0x50, 0x78, Tx("Sound effect volume"));
    InsertChild(&EffectsVolume);
    EffectsVolume.Create(0x127, 0x78, 0xb4, 10, Settings.SoundEffectVolume);
    AddLabel(this, 0x50, 0x96, Tx("Voice volume"));
    InsertChild(&VoiceVolume);
    VoiceVolume.Create(0x127, 0x96, 0xb4, 10, Settings.VoiceVolume);

    InsertChild(&ReverseChannels);
    ReverseChannels.SetPosition(0x50, 0xc0, 0, 0);
    ReverseChannels.SetText(Tx("Reverse stereo channels"));
    ReverseChannels.Create();
    ReverseChannels.SetCheck(Settings.ReverseChannels != 0);

    AddLabel(this, 0x50, 0xe6, Tx("Miles 3D sound provider"));
    InsertChild(&Providers);
    Providers.SetPosition(0x46, 0xfc, 0x1c7, 0);
    Providers.SetHint(Tx("Select the 3D sound provider."));
    Providers.Create(2, 5, true, true, 0, true);
    SDArray<SString> names = {};
    if (SIPanzersConcert* pc = PzConcert())
        pc->GetProviderNames(&names);                                // concert +0x1c
    for (int i = 0; i < names.size; ++i) {
        const SString& n = names.array[i];
        int idx = Providers.AddItem(n.buf ? n.buf : "", nullptr, 0xd0d0d0, 0);
        if (n.size == Settings.MilesProvider.size
            && (n.size == 0 || _stricmp(n.buf, Settings.MilesProvider.buf) == 0)) {
            if (idx < Providers.GetCount() && idx > -2) {
                Providers.CurSel = idx;
                Providers.Update();                                  // vtbl +0x78
            }
            Providers.EnsureVisible(idx);                            // 0x53c3c0
        }
    }
    // 0x582d20: free the names
    for (int i = 0; i < names.size; ++i)
        if (names.array[i].buf) {
            delete[] names.array[i].buf;
            names.array[i].buf = nullptr;
        }
    free(names.array);
}

// PANZERS 0x631010
bool SAudioOptionsMenu::OnAction(SWidget* source, int action, int param)
{
    if (action == PZA_BUTTON_CLICK) {
        if (source == &OkButton) {
            Settings.MusicVolume = MusicVolume.GetValue();                                 // 0x64fc60
            Settings.SoundEffectVolume = EffectsVolume.GetValue();                         // 0x64fcf0
            Settings.VoiceVolume = VoiceVolume.GetValue();                                 // 0x64fd50
            Settings.ReverseChannels = ReverseChannels.GetCheck();                         // 0x64fcb0
            Settings.MilesProvider = Providers.GetItemText(Providers.GetCurSel());         // 0x64fc10
            Settings.Save();
            SendAction(PZA_AUDIO_MUSIC_VOLUME, Settings.MusicVolume);
            SendAction(PZA_AUDIO_EFFECTS_VOLUME, Settings.SoundEffectVolume);
            SendAction(PZA_AUDIO_VOICE_VOLUME, Settings.VoiceVolume);
            SendAction(PZA_AUDIO_REVERSE, Settings.ReverseChannels);
            SendAction(PZA_AUDIO_OK, 0);
            return true;
        }
        if (source == &BackButton) {
            SendAction(PZA_AUDIO_MUSIC_VOLUME, Settings.MusicVolume);
            SendAction(PZA_AUDIO_EFFECTS_VOLUME, Settings.SoundEffectVolume);
            SendAction(PZA_AUDIO_VOICE_VOLUME, Settings.VoiceVolume);
            SendAction(PZA_AUDIO_REVERSE, Settings.ReverseChannels);
            SendAction(PZA_AUDIO_BACK, 0);
            return true;
        }
        if (source != &ReverseChannels)
            return false;
        SendAction(PZA_AUDIO_REVERSE, ReverseChannels.GetCheck());
        if (Instant) {
            Settings.ReverseChannels = ReverseChannels.GetCheck();
            Settings.Save();
        }
        return true;
    }
    if (action == PZA_SLIDER_CHANGED) {
        int code = 0;
        if (source == &MusicVolume)
            code = PZA_AUDIO_MUSIC_VOLUME;
        else if (source == &EffectsVolume)
            code = PZA_AUDIO_EFFECTS_VOLUME;
        else if (source == &VoiceVolume)
            code = PZA_AUDIO_VOICE_VOLUME;
        if (code)
            SendAction(code, param);
        if (!Instant)
            return true;
        Settings.MusicVolume = MusicVolume.GetValue();
        Settings.SoundEffectVolume = EffectsVolume.GetValue();
        Settings.VoiceVolume = VoiceVolume.GetValue();
        Settings.Save();
        return true;
    }
    if (action == PZA_LISTBOX_SELECT) {
        if (source != &Providers)
            return false;
        if (!Instant)
            return true;
        // HD stores the name only; the provider itself changes on restart.
        Settings.MilesProvider = Providers.GetItemText(Providers.GetCurSel());
        Settings.Save();
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// SMainOptionsMenu
// ---------------------------------------------------------------------------

// PANZERS 0x6330e0
SMainOptionsMenu::SMainOptionsMenu()
{
    GameMenu = nullptr;      // param_1[0x18]
    AudioMenu = nullptr;     // param_1[0x16]
    GraphicsMenu = nullptr;  // param_1[0x17]
}

void SMainOptionsMenu::ClosePages()
{
    if (GameMenu) { delete GameMenu; GameMenu = nullptr; }
    if (AudioMenu) { delete AudioMenu; AudioMenu = nullptr; }
    if (GraphicsMenu) { delete GraphicsMenu; GraphicsMenu = nullptr; }
}

// PANZERS 0x633d80
SMainOptionsMenu::~SMainOptionsMenu()
{
    ClosePages();
}

// PANZERS 0x635400
void SMainOptionsMenu::Create()
{
    SRightMenu::Create(GetText("panzers/MainMenu.cpp", "Options"), true);
    const char* texts[4];
    texts[0] = GetText("panzers/MainMenu.cpp", "Game Options");
    texts[1] = GetText("panzers/MainMenu.cpp", "Graphics");
    texts[2] = GetText("panzers/MainMenu.cpp", "Audio");
    texts[3] = GetText("panzers/MainMenu.cpp", "Back");
    CreateButtons(Buttons, texts, 4, 0);
    // 0x5439f0: make this the focused child up the parent chain.
    for (SWidget* w = this; w->Enabled && w->Visible && w->Parent; w = w->Parent) {
        if (w->Parent->Focus != w) {
            for (SWidget* c = w->Parent->Child; c; c = c->Sibling)
                if (c->FocusSibling == w)
                    c->FocusSibling = nullptr;
            w->FocusSibling = w->Parent->Focus;
            w->Parent->Focus = w;
        }
    }
    // 0x5435b0 + 0x545110: re-send the last mouse position.
    if (SWindow* wnd = GetWindowParent())
        wnd->UpdateMouse();
}

// PANZERS 0x63afe0
void SMainOptionsMenu::OpenGameOptions()
{
    GameMenu = new SGameOptionsMenu();                                // new 0x6f4
    Parent->InsertChild(GameMenu);                                    // vtbl +0x54
    GameMenu->Create(true);                                           // 0x62dbf0(1)
}

// PANZERS 0x63b060
void SMainOptionsMenu::OpenGraphicsOptions()
{
    GraphicsMenu = new SGraphicsOptionsMenu();                        // new 0xa10
    Parent->InsertChild(GraphicsMenu);
    GraphicsMenu->Create(true);                                       // 0x62e280(1)
}

// PANZERS 0x63af60
void SMainOptionsMenu::OpenAudioOptions()
{
    AudioMenu = new SAudioOptionsMenu();                              // new 0x4b0
    Parent->InsertChild(AudioMenu);
    AudioMenu->Create(true);                                          // 0x62d800(1)
}

// PANZERS 0x63ba50
// The pages are children of SSuperWindow, not of this menu, so their
// 0x4f4xx/0x4f56x actions reach SSuperWindow::OnAction directly; the cases
// here only fire for actions that pass through this widget.
bool SMainOptionsMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action == PZA_GAME_OK || action == PZA_GAME_BACK || action == PZA_AUDIO_OK)
        return true;
    if (action == PZA_GRAPHICS_OK) {
        if (GraphicsMenu) { delete GraphicsMenu; GraphicsMenu = nullptr; }
        return true;
    }
    if (action == PZA_AUDIO_BACK || action == PZA_GRAPHICS_CANCEL)
        return true;
    if (action != PZA_BUTTON_CLICK)
        return false;
    if (source == &Buttons[0]) {
        ClosePages();
        OpenGameOptions();
    } else if (source == &Buttons[1]) {
        ClosePages();
        OpenGraphicsOptions();
    } else if (source == &Buttons[2]) {
        ClosePages();
        OpenAudioOptions();
    } else if (source == &Buttons[3]) {
        SendAction(PZA_OPTIONS_BACK, 0);
    }
    return true;
}

// Recompile-only: HD has no key handler on the Options screens (Esc does
// nothing there; checked on the original). Esc acts as the Back button.
bool SMainOptionsMenu::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    if (key == VK_ESCAPE) {
        SendAction(PZA_OPTIONS_BACK, 0);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// SSuperWindow side
// ---------------------------------------------------------------------------

// PANZERS 0x658500
void LoadMainOptionsMenu(SSuperWindow* sw)
{
    Logger.g->Log(0, "SSuperWindow::LoadMainOptionsMenu START");
    sw->LoadMenuBackground(false);                                    // 0x658690(0)
    SMainOptionsMenu* m = new SMainOptionsMenu();                     // new 0x234, 0x6330e0
    sw->Menu_fc = m;                                                  // +0xfc
    sw->InsertChild(m);                                               // vtbl +0x54
    m->SetPosition(0, 0, 0x400, 0x300);                               // vtbl +0x08
    m->Create();                                                      // 0x635400
    Logger.g->Log(0, "SSuperWindow::SMainOptionsMenuMenu: releasing scene");
    Logger.g->Log(0, "SSuperWindow::LoadMainOptionsMenu END");
}

// The options cases of SSuperWindow::OnAction (PANZERS 0x659250).
bool SuperWindowOptionsAction(SSuperWindow* sw, int action, int param)
{
    switch (action) {
    case PZA_MAIN_OPTIONS:                                // 0x4d4d5
        if (sw->MainMenu) {
            delete sw->MainMenu;
            sw->MainMenu = nullptr;
        }
        LoadMainOptionsMenu(sw);                          // 0x658500
        return true;
    case PZA_OPTIONS_BACK:                                // 0x4d4f4
        if (sw->Menu_fc) {
            delete sw->Menu_fc;
            sw->Menu_fc = nullptr;
        }
        sw->LoadMainMenu();                               // 0x6583e0
        return true;
    case PZA_AUDIO_MUSIC_VOLUME:                          // concert +0x10(0, v)
        if (Concert) Concert->SetVolume(0, param);
        return true;
    case PZA_AUDIO_EFFECTS_VOLUME:                        // concert +0x10(1, v)
        if (Concert) Concert->SetVolume(1, param);
        return true;
    case PZA_AUDIO_VOICE_VOLUME:                          // concert +0x10(2, v)
        if (Concert) Concert->SetVolume(2, param);
        return true;
    case PZA_AUDIO_REVERSE:                               // concert +0x14(v != 0)
        if (Concert) Concert->SetReverseStereo(param != 0);
        return true;
    case PZA_GRAPHICS_BRIGHTNESS:                         // Gepard +0x1c((v - 5) * 0.1 + 1.0)
        PzSetBrightness((float)((double)(param - 5) * 0.1 + 1.0));
        return true;
    case PZA_GRAPHICS_APPLY: {
        // HD: if the window's stored full-screen mode (SDXWindow +0x90..+0xa8)
        // differs from the settings, vtbl +0xa0 (0x53a380) stores the new
        // one and, only while full screen, resets the device to it. The
        // recompile runs windowed: store it in the SWINE SDXWindow fields
        // (as SSuperWindow's ctor does) so it is used the next time full
        // screen is entered; the device reset itself is not lifted.
        if (sw->FullScreenWidth != Settings.FullScreenWidth
            || sw->FullScreenHeight != Settings.FullScreenHeight
            || sw->VSync != Settings.FullScreenVSync) {
            sw->FullScreenWidth = Settings.FullScreenWidth;
            sw->FullScreenHeight = Settings.FullScreenHeight;
            sw->VSync = Settings.FullScreenVSync;
            Logger.g->Log(0, "SSuperWindow::OnAction: full-screen mode %dx%d stored%s",
                          Settings.FullScreenWidth, Settings.FullScreenHeight,
                          sw->DisplayMode == Windowed ? "" : " (device reset not lifted; applies on restart)");
        }
        // HD: Gepard SetOption 3 (shadow buffer), 2 (shadows), 8/9 (texture
        // filter), 10 (texture detail), scene +0x104, board +0xc8 (hardware
        // cursor). No SWINE renderer counterpart: logged stub.
        PzStub_ApplyGraphicsOptions(Settings.Shadows, Settings.ShadowBufferSize, Settings.TextureFilter,
                                    Settings.TextureDetail, Settings.HardwareMouseCursor);
        return true;
    }
    // HD has no case for these and returns 1 (they only matter in-game).
    case PZA_AUDIO_OK: case PZA_AUDIO_BACK:
    case PZA_GAME_OK: case PZA_GAME_OK2: case PZA_GAME_BACK:
    case PZA_GRAPHICS_OK: case PZA_GRAPHICS_CANCEL:
    // Widget notifications the pages leave unhandled (HD: default return 1).
    case PZA_DROPLIST_SELECT: case PZA_LISTBOX_SELECT:
    case PZA_LISTBOX_SCROLLED: case PZA_LISTBOX_DBLCLICK:
    case PZA_SLIDER_CHANGED: case PZA_SLIDER_LEFT: case PZA_SLIDER_RIGHT:
    case PZA_SCROLL_UP: case PZA_SCROLL_DOWN: case PZA_SCROLL_TO:
        return true;
    default:
        return false;
    }
}
