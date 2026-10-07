// src/panzers/hud.cpp
// HUD widgets (hud.h) and the HUD half of SGameView::Create 0x619c90.
// OWNER: agent H (docs/M3_INTERFACES.md).
//
// The HD SGameView embeds these widgets at fixed offsets (given per member
// of SGameHud below); the recompile's SGameView keeps its HD data in
// SGameViewData, so the HUD lives in a separate SGameHud the view points to
// (PzHud). HD board slots as in pzwidgets.cpp; the 0x18a-glyph interface
// font goes through PzLoadCustomFont / PzSetSpriteGlyph (hudwidgets.h).

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "hud.h"
#include "chatline.h"
#include "gameview.h"
#include "ingamemenu.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "gettext.h"
#include "pzboard.h"
#include "board.h"
#include "hudwidgets.h"
#include "campaign.h"
#include "packets.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "unit.h"

static const char* Gv(const char* id) { return GetText("panzers/GameView.cpp", id); }

#include "settings.h"
#include "timer.h"

// SSettings +0xcc OwnIcon / +0xd0 AlliedIcon / +0xd4 EnemyIcon (HD getters
// 0x64e060 / 0x64d900 / 0x64def0), for the unit board elements
// (src/game/unitboard.cpp).
int PzUnitIconSetting(int which)
{
    switch (which) {
    case 0: return Settings.OwnIcon;
    case 1: return Settings.AlliedIcon;
    default: return Settings.EnemyIcon;
    }
}

#include "hud_glyphs.inl"

// ===========================================================================
// SSpecInfoWidget
// ===========================================================================

// PANZERS 0x618a30
SSpecInfoWidget::SSpecInfoWidget() : Font(-1), SpriteFrame(-1) { ToolTipFeatureEnabled = false; }

// PANZERS 0x6194c0
SSpecInfoWidget::~SSpecInfoWidget() {}

// PANZERS 0x61e2d0
void SSpecInfoWidget::Create(int font)
{
    SDXWidget::Create(0);                                          // 0x539a10
    Font = font;
    SpriteFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 1);
}

// PANZERS 0x6260e0
void SSpecInfoWidget::SetGlyph(int glyph)
{
    PzSetSpriteGlyph(SpriteFrame, Font, glyph);                    // board +0x24
}

// ===========================================================================
// SUnitButton
// ===========================================================================

// PANZERS 0x618a60
SUnitButton::SUnitButton()
    : IconFont(-1), RedBox(-1), _7c(0), Icon1(-1), Icon2(-1), _88(0), _8c(0), Marker(-1), Unit(-1)
{
    for (int i = 0; i < 5; ++i)
        StateSprites[i] = Bars[i] = -1;
}

// PANZERS 0x6194f0
SUnitButton::~SUnitButton() {}

// PANZERS 0x6251d0
void SUnitButton::OnMouseOver()
{
    Over = true;
    Redraw();
    SendAction(0x42543, 0);
}

// PANZERS 0x626280 (a thunk to SButton's redraw 0x537d60)
void SUnitButton::Redraw()
{
    pz::SButton::Redraw();
}

// PANZERS 0x61e310
void SUnitButton::Create(int font, int glyph, int icon1, int icon2)
{
    IconFont = font;
    pz::SButton::Create(font, glyph, glyph, -1, -1);               // 0x537a80
    RedBox = Board->CreateFrame(FT_BOX, SpriteFrame, 0, 0, 0, 1);
    Board->SetBoxColor(RedBox, 0x60ff0000);                        // board +0x3c
    Board->ResizeFrame(RedBox, Width, Height);
    Board->ShowFrame(RedBox, false);
    for (int i = 0, x = 9; x < 0x31; ++i, x += 8) {
        StateSprites[i] = Board->CreateFrame(FT_SPRITE, SpriteFrame, 4, 4, 0, 0);
        Bars[i] = Board->CreateFrame(FT_BOX, SpriteFrame, x, 0x2f, 0, 1);
        Board->ResizeFrame(Bars[i], 5, 5);
        Board->ShowFrame(Bars[i], false);
    }
    Icon1 = -1;                                                    // +0x80
    if (icon1 >= 0) {
        Icon1 = Board->CreateFrame(FT_SPRITE, SpriteFrame, 0, 0, 0, 1);
        PzSetSpriteGlyph(Icon1, IconFont, icon1);
        Board->ShowFrame(Icon1, true);
    }
    Icon2 = -1;                                                    // +0x84
    if (icon2 >= 0) {
        Icon2 = Board->CreateFrame(FT_SPRITE, SpriteFrame, 0, 0, 0, 1);
        PzSetSpriteGlyph(Icon2, IconFont, icon2);
        Board->ShowFrame(Icon2, true);
    }
    Marker = Board->CreateFrame(FT_SPRITE, SpriteFrame, 3, 3, 0, 1);
    PzSetSpriteGlyph(Marker, IconFont, 0x149);
    Board->ShowFrame(Marker, false);
    _7c = 0;
    SetUnit(-1, false);                                            // 0x626c40(-1, 0)
}

static pz::SUnit* HeapUnit(int index)
{
    // SHeapTRB::operator[] (0x546490)
    pz::SWorld* w = pz::g_World;
    if (!w || !w->Units.IsLive(index))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", index);
    return w->Units.Array[index].Unit;
}

// HP state: 0 >= 0.6, 1 below 0.6, 2 below 0.3 (DAT_007fd6f0 / DAT_007f83d8).
static int HpLevel(float hp)
{
    if (hp >= 0.3f)
        return hp < 0.6f ? 1 : 0;
    return 2;
}

// Armour state glyph: >= 0.85, >= 0.7, below (DAT_007fb6a0 / DAT_007fb69c).
static int ArmorGlyph(float a, int good, int mid, int bad)
{
    if (a >= 0.7f)
        return a >= 0.85f ? good : mid;
    return bad;
}

static unsigned int BarColor(float hp)
{
    if (hp >= 0.3f)
        return hp >= 0.6f ? 0xff9eca3f : 0xffc4ae3a;
    return 0xffc34d3a;
}

// PANZERS 0x626c40
void SUnitButton::SetUnit(int unit, bool flag)
{
    Unit = unit;
    for (int i = 0; i < 5; ++i) {
        Board->ShowFrame(StateSprites[i], false);
        Board->ShowFrame(Bars[i], false);
    }
    Board->ShowFrame(Marker, false);
    Board->ShowFrame(Icon2, false);
    if (Unit < 0) {
        int glyph;
        if (Unit == -1) glyph = 0x186;
        else if (Unit == -2) glyph = 0x188;
        else if (Unit == -3) glyph = 0x189;
        else { Unit = -1; return; }
        Board->MoveFrame(StateSprites[0], 0, 0);
        PzSetSpriteGlyph(StateSprites[0], IconFont, glyph);
        Board->ShowFrame(StateSprites[0], true);
        Unit = -1;
        return;
    }
    pz::SUnit* u = HeapUnit(Unit);
    pz::SPUnit* p = u->Proto;
    if (p->ClassType != 5) {
        int lvl = HpLevel(u->HP);
        Board->MoveFrame(StateSprites[0], 0, 0);
        Board->ShowFrame(StateSprites[0], true);
        if (flag) {
            PzSetSpriteGlyph(StateSprites[0], IconFont, p->UnitType * 3 + 0x10f + lvl);
            return;
        }
        if (Icon1 >= 0)
            Board->ShowFrame(Icon1, false);
        if (p->ClassType == 9) {
            PzSetSpriteGlyph(StateSprites[0], IconFont, lvl + 0x136);
            return;
        }
        if (p->ArmourType != 2) {
            PzSetSpriteGlyph(StateSprites[0], IconFont, lvl + 0x118);
            return;
        }
        Board->ShowFrame(Icon2, true);
        Board->MoveFrame(StateSprites[0], 4, 4);
        PzSetSpriteGlyph(StateSprites[0], IconFont, lvl + 0xaa);
        if (p->FrontArmor > 0.0f) {
            Board->ShowFrame(StateSprites[1], true);
            PzSetSpriteGlyph(StateSprites[1], IconFont, ArmorGlyph(u->Armor[0], 0xae, 0xaf, 0xb0));
        }
        if (p->LeftSideArmor > 0.0f) {
            Board->ShowFrame(StateSprites[2], true);
            PzSetSpriteGlyph(StateSprites[2], IconFont, ArmorGlyph(u->Armor[1], 0xb6, 0xb7, 0xb8));
        }
        if (p->RightSideArmor > 0.0f) {
            Board->ShowFrame(StateSprites[3], true);
            PzSetSpriteGlyph(StateSprites[3], IconFont, ArmorGlyph(u->Armor[2], 0xba, 0xbb, 0xbc));
        }
        if (p->BackArmor > 0.0f) {
            Board->ShowFrame(StateSprites[4], true);
            PzSetSpriteGlyph(StateSprites[4], IconFont, ArmorGlyph(u->Armor[3], 0xb2, 0xb3, 0xb4));
        }
        return;
    }
    // A squad: the leader's stored / member lists give the marker and the
    // average health of the living members.
    Board->MoveFrame(StateSprites[0], 0, 0);
    pz::SUnit* leader = u;
    if (u->Members.Size == 0 && u->Parent >= 0)
        leader = HeapUnit(u->Parent);
    for (int i = 0; i < leader->Stored.Size; ++i)
        if (leader->Stored.Array[i].Unit == Unit && leader->Stored.Array[i].Mode == 1)
            Board->ShowFrame(Marker, true);
    float sum = -1.0f;                                             // DAT_007f5a98
    int count = 1;
    auto addMembers = [&](const pz::SUnitMember* m, int n) {
        for (int i = 0; i < n; ++i) {
            if (m[i].Owner != Unit)
                continue;
            float hp = HeapUnit(m[i].Unit)->HP;
            if (hp <= 0.0f)
                continue;
            if (sum == -1.0f) {
                sum = hp;
            } else {
                ++count;
                sum += hp;
            }
        }
    };
    addMembers(leader->Members.Array, leader->Members.Size);
    if (leader->Proto->ClassType == 9) {
        const pz::SUnitArray<pz::SUnitMember>* m2 = (const pz::SUnitArray<pz::SUnitMember>*)((const unsigned char*)leader + 0x408);
        addMembers(m2->Array, m2->Size);
    }
    int lvl = HpLevel(sum / (float)count);
    static const int kSquadGlyph[11] = { 0x17d, 0x16e, 0x177, 0x180, 0x171, 0x17a, 0x165, 0x16b, 0x162, 0x168, 0x174 };
    if (p->UnitType >= 0xe && p->UnitType <= 0x18)
        PzSetSpriteGlyph(StateSprites[0], IconFont, kSquadGlyph[p->UnitType - 0xe] + lvl);
    Board->ShowFrame(StateSprites[0], true);
    if (p->HeroPicture >= 0) {
        PzSetSpriteGlyph(StateSprites[0], IconFont, lvl + 0x183);
        return;
    }
    pz::SUnit* owner = u;
    if (u->Members.Size == 0 && u->Parent >= 0)
        owner = HeapUnit(u->Parent);
    int k = 0;
    auto addBars = [&](const pz::SUnitMember* m, int n) {
        for (int i = 0; i < n; ++i) {
            if (m[i].Owner != Unit)
                continue;
            float hp = HeapUnit(m[i].Unit)->HP;
            if (hp <= 0.0f)
                continue;
            Board->ShowFrame(Bars[k], true);
            Board->SetBoxColor(Bars[k], BarColor(hp));             // board +0x3c
            if (++k > 4)
                k = 4;
        }
    };
    addBars(owner->Members.Array, owner->Members.Size);
    if (owner->Proto->ClassType == 9) {
        const pz::SUnitArray<pz::SUnitMember>* m2 = (const pz::SUnitArray<pz::SUnitMember>*)((const unsigned char*)owner + 0x408);
        addBars(m2->Array, m2->Size);
    }
}

// ===========================================================================
// SHeroUnitButton
// ===========================================================================

// PANZERS 0x618a10
SHeroUnitButton::SHeroUnitButton()
    : PhotoFont(-1), HpBar(-1), RedBox(-1), _80(0), Photo(-1), _88(0.0f), _8c(0.0f), Hero(-1) {}

// PANZERS 0x619490
SHeroUnitButton::~SHeroUnitButton() {}

// PANZERS 0x6251b0
void SHeroUnitButton::OnMouseOver()
{
    Over = true;
    SendAction(0x42543, 0);
}

// PANZERS 0x61e1e0
void SHeroUnitButton::Create(int font, int glyph)
{
    PhotoFont = font;
    pz::SButton::Create(font, glyph, glyph, -1, -1);               // 0x537a80
    Photo = Board->CreateFrame(FT_SPRITE, SpriteFrame, 0, 0, 0, 1);
    RedBox = Board->CreateFrame(FT_BOX, SpriteFrame, 0, 0, 0, 1);
    Board->SetBoxColor(RedBox, 0x60ff0000);
    Board->ResizeFrame(RedBox, Width, Height);
    Board->ShowFrame(RedBox, false);
    HpBar = Board->CreateFrame(FT_BOX, SpriteFrame, 3, Height + 1, 0, 1);
    Board->SetBoxColor(HpBar, 0xff00c000);
    Board->ResizeFrame(HpBar, Width - 6, 5);
    Hero = -1;                                                     // +0x90
    _80 = 0;
}

// PANZERS 0x625b60
// The red box over the photo: hidden at 0, else red with the alpha byte of
// -(int)(level * -255) (the HD shift wraps for levels above 1).
void SHeroUnitButton::SetBlink(float level)
{
    if (level != 0.0f) {
        Board->SetBoxColor(RedBox, 0xff0000u - ((unsigned)(int)(level * -255.0f) << 24));   // board +0x3c, 0x8043b0
        Board->ShowFrame(RedBox, true);
        return;
    }
    Board->ShowFrame(RedBox, false);
}

// PANZERS 0x625c40
// The HP bar under the photo: green, yellow below 0.6, red below 0.3; the
// width (Width - 6) * hp.
void SHeroUnitButton::SetHp(float hp)
{
    unsigned color;
    if ((double)hp < 0.6)                                          // 0x7f4550
        color = (double)hp < 0.3 ? 0xffff0000u : 0xffffff00u;      // 0x7f4548
    else
        color = 0xff00c000u;
    Board->SetBoxColor(HpBar, color);                              // board +0x3c
    Board->ResizeFrame(HpBar, (int)((float)(Width - 6) * hp), 5);  // board +0x14
}

// Inline twice in 0x626690: while +0x80 counts down after an HP loss the red
// box pulses, (sin(t * 7) * 0.07 + 1) * 1.5 with t the timer seconds
// (0x661800, 0x7f5a48, 0x78d640, 0x7fb6a8, 0x7eed98, 0x7fe0c8); then 0.
static float HeroBlinkLevel(SHeroUnitButton* b)
{
    if (b->_80 < 1)
        return 0.0f;
    --b->_80;
    double t = (double)Timer.GetTickValue() / 1000.0 * 7.0;
    return (float)((sin(t) * 0.07 + 1.0) * 1.5);
}

// PANZERS 0x626690
// Per frame from Update 0x628430 with a hero of the local player: the photo
// (hero font, prototype +0x34), the HP bar of the crew's first member (or
// of the hero itself on foot, unless +0x150), and the red box that pulses
// for 100 frames once the hero's vehicle (+0x8c) or its crew (+0x88) lost HP.
void SHeroUnitButton::SetHero(int unit)
{
    if (Hero == unit) {
        if (unit == -1) {
            Hero = unit;
            goto photo;
        }
        if (HeapUnit(Hero)->Parent >= 0) {
            if (_8c != -1.0f) {                                    // 0x7f5a98
                if (_8c <= HeapUnit(HeapUnit(Hero)->Parent)->HP) {
                    SetBlink(HeroBlinkLevel(this));
                    goto photo;
                }
                _80 = 100;
            }
            _8c = HeapUnit(HeapUnit(Hero)->Parent)->HP;
            goto photo;
        }
        if (HeapUnit(Hero)->Members.Size < 1) {
            if (!HeapUnit(Hero)->Wrecked)                          // +0x150
                SetHp(HeapUnit(Hero)->HP);
            goto photo;
        }
        _8c = -1.0f;
        if (_88 <= HeapUnit(HeapUnit(Hero)->Members.Array[0].Unit)->HP) {
            SetBlink(HeroBlinkLevel(this));
        } else {
            _80 = 100;
            _88 = HeapUnit(HeapUnit(Hero)->Members.Array[0].Unit)->HP;
        }
    } else {
        if (unit == -1) {
            Hero = unit;
            goto photo;
        }
        Hero = unit;
        if (HeapUnit(unit)->Parent < 0) {
            if (HeapUnit(unit)->Members.Size > 0) {
                _8c = -1.0f;
                _88 = HeapUnit(HeapUnit(Hero)->Members.Array[0].Unit)->HP;
            }
        } else {
            _8c = HeapUnit(HeapUnit(unit)->Parent)->HP;
            pz::SUnit* veh = HeapUnit(HeapUnit(Hero)->Parent);
            if (veh->Members.Size > 0)
                _88 = HeapUnit(veh->Members.Array[0].Unit)->HP;
        }
        Board->ShowFrame(RedBox, false);                           // board +0x18(+0x7c, 0)
        if (HeapUnit(Hero)->Members.Size < 1)
            goto photo;
    }
    SetHp(HeapUnit(HeapUnit(Hero)->Members.Array[0].Unit)->HP);
photo:
    if (Hero >= 0) {
        PzSetSpriteGlyph(Photo, PhotoFont, HeapUnit(Hero)->Proto->HeroPicture);   // board +0x24
        Board->ShowFrame(Photo, true);
    }
}

// ===========================================================================
// SCommandButton
// ===========================================================================

SCommandButton::SCommandButton()
    : IconNormal(-1), IconDown(-1), IconOver(-1), Icon(-1), _84(0), ExtraGlyph(-1), ExtraFrame(-1) {}

// PANZERS 0x619400
SCommandButton::~SCommandButton() {}

// PANZERS 0x625180
void SCommandButton::OnMouseOver()
{
    Over = true;
    Redraw();
    SendAction(0x42543, 0);
}

// PANZERS 0x627da0
void SCommandButton::SetVisible(bool visible)
{
    pz::SButton::SetVisible(visible);                              // 0x539c40
}

// PANZERS 0x619b90
void SCommandButton::Create(int font, int normal, int down, int over, int extra)
{
    Icon = -1;                                                     // +0x80
    IconNormal = -1;                                               // +0x74
    pz::SButton::Create(font, normal, down, over, -1);             // 0x537a80
    ExtraFrame = Board->CreateFrame(FT_SPRITE, SpriteFrame, 0x20, 0, 0, 1);
    if (extra > -2) {
        PzSetSpriteGlyph(ExtraFrame, Font, extra);
        ExtraGlyph = -1;                                           // +0x88
        Board->ShowFrame(ExtraFrame, false);
    }
    IconNormal = Board->CreateFrame(FT_SPRITE, SpriteFrame, 4, 0, 0, 0);
    IconDown = Board->CreateFrame(FT_SPRITE, SpriteFrame, 4, 0, 0, 0);
    IconOver = Board->CreateFrame(FT_SPRITE, SpriteFrame, 4, 0, 0, 0);
}

// The icon glyph triple (normal g, down g + 2, over g + 1), inline in
// 0x619c90 and the panel update.
void SCommandButton::SetIcon(int glyph)
{
    if (Icon == glyph)
        return;
    Icon = glyph;
    PzSetSpriteGlyph(IconNormal, Font, glyph);
    PzSetSpriteGlyph(IconDown, Font, glyph + 2);
    PzSetSpriteGlyph(IconOver, Font, glyph + 1);
    Board->ShowFrame(IconNormal, true);
    Board->ShowFrame(IconDown, false);
    Board->ShowFrame(IconOver, false);
}

// PANZERS 0x626150
void SCommandButton::Redraw()
{
    pz::SButton::Redraw();                                         // 0x537d60
    if (BackFrame < 0 || IconNormal < 0)
        return;
    if (!Checked && !Pressed) {
        if (Over && GlyphOver >= 0) {
            Board->ShowFrame(IconNormal, false);
            Board->ShowFrame(IconDown, false);
            Board->ShowFrame(IconOver, true);
            return;
        }
        Board->ShowFrame(IconNormal, true);
        Board->ShowFrame(IconDown, false);
    } else {
        Board->ShowFrame(IconNormal, false);
        Board->ShowFrame(IconDown, true);
    }
    Board->ShowFrame(IconOver, false);
}

// ===========================================================================
// SGroupIcon
// ===========================================================================

// PANZERS 0x6189e0
SGroupIcon::SGroupIcon() : Font(-1), Frame1(-1), Frame2(-1), Selected(false), _75(false)
{
    ToolTipFeatureEnabled = false;
    for (int i = 0; i < 4; ++i)
        Glyph[i] = -1;
}

// 0x619c90 (the group icon loop): two sprites, glyphs n-1 / n.
void SGroupIcon::Create(int font, int n)
{
    SDXWidget::Create(0);                                          // 0x539a10
    Font = font;
    Glyph[1] = n;                                                  // +0x60
    Glyph[0] = n - 1;                                              // +0x5c
    Glyph[2] = n - 1;                                              // +0x64
    Glyph[3] = n;                                                  // +0x68
    Frame1 = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 0);
    Frame2 = Board->CreateFrame(FT_SPRITE, BackFrame, 0, 0, 0, 1);
    if (BackFrame >= 0) {
        PzSetSpriteGlyph(Frame1, Font, Selected ? Glyph[1] : Glyph[0]);
        PzSetSpriteGlyph(Frame2, Font, Selected ? Glyph[3] : Glyph[2]);
        int w = 0, h = 0;
        Board->GetFrameSize(Frame1, &w, &h);
        Resize(w, h);                                              // vtbl +0x10
    }
}

// PANZERS 0x626200
void SGroupIcon::Redraw()
{
    if (BackFrame < 0)
        return;
    PzSetSpriteGlyph(Frame1, Font, Selected ? Glyph[1] : Glyph[0]);   // board +0x24
    PzSetSpriteGlyph(Frame2, Font, Selected ? Glyph[3] : Glyph[2]);
    int w = 0, h = 0;
    Board->GetFrameSize(Frame1, &w, &h);                           // board +0x20
    Resize(w, h);                                                  // vtbl +0x10
}

// ===========================================================================
// The HUD of SGameView
// ===========================================================================

// HD SGameView members (offsets; "[n]" = the dword index the decompile uses).
struct SGameHud {
    int InterfaceFont = -1;               // +0x4b8 [0x12e] menu/panzers_interface_hq.tga
    int MultiUnitFont = -1;               // +0x4bc [0x12f] menu/interface_units_multiplayer_hq.tga
    int HeroFont = -1;                    // +0x4c0 [0x130] menu/hero_photo_hq.tga
    int UnitFonts[4] = { -1, -1, -1, -1 }; // +0x4c4..+0x4d0 headquarters_units_german / allied / russian / misc
    SDXWidget TopBar;                     // +0x4d4 [0x135]
    SDXWidget Panel;                      // +0x52c [0x14b]
    int ClockFrame = -1;                  // +0x5cc [0x173]
    SMinimap Minimap;                     // +0x7cc [0x1f3]
    SGroupIcon Groups[9];                 // +0x830 [0x20c]
    pz::SButton PanelToggle;              // +0xc68 [0x31a]
    pz::SButton MenuButton;               // +0xcdc [0x337]
    pz::SButton ObjectivesButton;         // +0xd50 [0x354]
    pz::SButton SupportArtillery;         // +0xdc4 [0x371]
    pz::SButton SupportRecon;             // +0xe38 [0x38e]
    pz::SButton SupportFighter;           // +0xeac [0x3ab]
    pz::SButton SupportBomber;            // +0xf20 [0x3c8]
    pz::SButton SupportParatroops;        // +0xf94 [0x3e5]
    pz::SButton PauseButton;              // +0x1008 [0x402]
    pz::SButton PlayButton;               // +0x107c [0x41f]
    pz::SButton FastButton;               // +0x10f0 [0x43c]
    pz::SButton StateFree;                // +0x1164 [0x459]
    pz::SButton StateStand;               // +0x11d8 [0x476]
    pz::SButton StatePassive;             // +0x124c [0x493]
    pz::SButton StanceRun;                // +0x12c0 [0x4b0]
    pz::SButton StanceCrouch;             // +0x1334 [0x4cd]
    pz::SButton StanceCrawl;              // +0x13a8 [0x4ea]
    pz::SButton MarkerButton;             // +0x141c [0x507]
    pz::SButton TerrainButton;            // +0x1490 [0x524]
    pz::SButton AllyColorsButton;         // +0x1504 [0x541]
    int SupportCount[5] = { -1, -1, -1, -1, -1 }; // +0x1578 [0x55e] artillery, recon, fighter, bomber, paratroops
    SUnitButton Selection[16];            // +0x158c [0x563]
    SHeroUnitButton Heroes[5];            // +0x218c [0x863]
    int UnitHeader = -1;                  // +0x2478 [0x91e]
    int UnitName = -1;                    // +0x247c [0x91f]
    int Stars[4] = { -1, -1, -1, -1 };    // +0x2480 [0x920]
    int _2490 = -1;                       // +0x2490 [0x924]
    SSpecInfoWidget Crew[4];              // +0x2494 [0x925]
    SSpecInfoWidget Info[5];              // +0x2614 [0x985] hp, ammo, cargo, xp, thermostat
    int InfoText[5] = { -1, -1, -1, -1, -1 }; // [0x99d] [0x9ce] [0x9cf] [0x9e8] [0xa01]
    int ArmorText[4] = { -1, -1, -1, -1 };// +0x2808 [0xa02]
    SUnitButton UnitButton;               // +0x2818 [0xa06]
    SUnitButton Units2[2];                // +0x28d8 [0xa36]
    int Vehicle[4] = { -1, -1, -1, -1 };  // +0x2a60 [0xa98] sprites, [0xa9a] texts
    int PanelFrame = -1;                  // +0x2a70 [0xa9c] glyph 0xa1 (shown by SetPanelMode)
    SCommandButton Stop;                  // +0x2a74 [0xa9d]
    SCommandButton Move;                  // +0x2b04 [0xac1]
    SCommandButton Reverse;               // +0x2b94 [0xae5]
    SCommandButton Attack;                // +0x2c24 [0xb09]
    SCommandButton Suppress;              // +0x2cb4 [0xb2d]
    SCommandButton Heal;                  // +0x2d44 [0xb51]
    SCommandButton Attach;                // +0x2dd4 [0xb75]
    SCommandButton Detach;                // +0x2e64 [0xb99]
    SCommandButton Repair;                // +0x2ef4 [0xbbd]
    SCommandButton Resupply;              // +0x2f84 [0xbe1]
    SCommandButton ResupplyAll;           // +0x3014 [0xc05]
    SCommandButton Leave;                 // +0x30a4 [0xc29]
    SCommandButton Equip1;                // +0x3134 [0xc4d]
    SCommandButton Equip2;                // +0x31c4 [0xc71]
    SCommandButton Empty[6];              // +0x3254 [0xc95] (the six empty command slots)
    pz::SButton MpUnits[6];               // +0x35b4 [0xd6d]
    int MpUnitText[6] = { -1, -1, -1, -1, -1, -1 }; // +0x386c [0xe1b]
    SGameView* View = nullptr;
};

static SGameHud* s_Hud = nullptr;

SGameHud* PzHud(SGameView* view)
{
    return (s_Hud && s_Hud->View == view) ? s_Hud : nullptr;
}

static void AddButton(SWidget* parent, pz::SButton* b, int x, int y, int font, int normal, int down, int over, int checked)
{
    parent->InsertChild(b);                                        // vtbl +0x54
    b->SetPosition(x, y, 0, 0);                                    // vtbl +0x08
    b->Create(font, normal, down, over, checked);                  // 0x537a80
}

static void AddCommand(SWidget* parent, SCommandButton* b, int x, int y, int font, int base, int extra, int icon)
{
    parent->InsertChild(b);
    b->SetPosition(x, y, 0, 0);
    b->Create(font, base, base + 2, base + 1, extra);              // 0x619b90
    if (icon >= 0)
        b->SetIcon(icon);
}

static void AddInfo(SWidget* panel, SSpecInfoWidget* w, int x, int y, int ifFont, int glyph)
{
    panel->InsertChild(w);
    w->SetPosition(x, y, 0, 0);
    w->SetBackgroundSprite(ifFont, glyph, false, false);           // 0x539ba0
    w->Create(ifFont);                                             // 0x61e2d0
    w->SetVisible(false);
}

// PANZERS 0x619c90 (the HUD half: from the panzers_interface_hq.tga load on)
void PzHudCreate(SGameView* v)
{
    PZ_M3_TRACE("SGameView::Create HUD (0x619c90)");
    PzHudDestroy(v);
    SGameHud* h = new SGameHud();
    s_Hud = h;
    h->View = v;
    int IF = h->InterfaceFont = PzLoadCustomFont("menu/panzers_interface_hq.tga", 0x18a, kHudInterfaceGlyphs);
    static SCustomGlyph unitGlyphs[0x50];
    for (int i = 0; i < 0x50; ++i) {
        unitGlyphs[i].X = (i % 8) << 7;
        unitGlyphs[i].Y = (i / 8) * 0x33;
        unitGlyphs[i].Width = 0x80;
        unitGlyphs[i].Height = 0x33;
    }
    h->UnitFonts[0] = Board->LoadCustomFont("menu/headquarters_units_german_hq.tga", 0x50, unitGlyphs, Default);
    h->UnitFonts[1] = Board->LoadCustomFont("menu/headquarters_units_allied_hq.tga", 0x50, unitGlyphs, Default);
    h->UnitFonts[2] = Board->LoadCustomFont("menu/headquarters_units_russian_hq.tga", 0x50, unitGlyphs, Default);
    h->UnitFonts[3] = Board->LoadCustomFont("menu/headquarters_units_misc_hq.tga", 0x50, unitGlyphs, Default);

    // Top bar (0, 0) and bottom panel (0, 0x24a): interface glyphs 0 / 1.
    v->InsertChild(&h->TopBar);
    h->TopBar.SetPosition(0, 0, 0, 0);
    h->TopBar.SetBackgroundSprite(IF, 0, false, false);
    h->TopBar.SDXWidget::Create(0);
    v->InsertChild(&h->Panel);
    h->Panel.SetPosition(0, 0x24a, 0, 0);
    h->Panel.SetBackgroundSprite(IF, 1, false, false);
    h->Panel.SDXWidget::Create(0);

    h->MultiUnitFont = Board->LoadCustomFont("menu/interface_units_multiplayer_hq.tga", 0x42,
                                             const_cast<SCustomGlyph*>(kMultiplayerUnitGlyphs), Default);
    // Six multiplayer unit buttons on the panel.
    for (int i = 0; i < 6; ++i) {
        int y = (i / 3) * 0x33, x = (i % 3) * 0x80;
        h->Panel.InsertChild(&h->MpUnits[i]);
        h->MpUnits[i].SetPosition(x + 0x110, y + 0x46, 0x80, 0x33);
        h->MpUnits[i].Create(h->UnitFonts[0], 0, 0, -1, -1);
        h->MpUnits[i].SetVisible(false);
        h->MpUnitText[i] = Board->CreateFrame(FT_TEXT, h->Panel.GetFrame(), x + 0x18d, y + 0x69, 0, 1);
    }
    // The top-left panel toggle (hidden; SetPanelMode).
    h->TopBar.InsertChild(&h->PanelToggle);
    h->PanelToggle.SetPosition(0, 0, 0, 0);
    h->PanelToggle.Create(IF, 0x14, 0x14, -1, -1);
    h->PanelToggle.Cursor = 0;
    h->PanelToggle.SetVisible(false);
    // Nine group icons on the top bar.
    for (int i = 0, n = 3, x = 0x41; x < 0x17c; ++i, n += 2, x += 0x23) {
        SGroupIcon* g = &h->Groups[i];
        v->InsertChild(g);
        g->SetPosition(x, 5, 0, 0);
        g->Create(IF, n);
        g->Cursor = 0;
        g->SetVisible(false);
    }
    // Menu / Objectives, the clock, Pause / Play / Fast.
    AddButton(&h->TopBar, &h->MenuButton, 0x193, 0, IF, 0x109, 0x10b, 0x10a, -1);
    h->MenuButton.Cursor = 0;
    AddButton(&h->TopBar, &h->ObjectivesButton, 0x204, 0, IF, 0x10c, 0x10e, 0x10d, -1);
    h->ObjectivesButton.Cursor = 0;
    h->ClockFrame = Board->CreateFrame(FT_TEXT, h->TopBar.GetFrame(), 0x370, 2, 2, 0);
    AddButton(&h->TopBar, &h->PauseButton, 0x38a, 0, IF, 0x74, 0x76, 0x75, -1);
    AddButton(&h->TopBar, &h->PlayButton, 0x3af, 0, IF, 0x77, 0x79, 0x78, -1);
    AddButton(&h->TopBar, &h->FastButton, 0x3d4, 0, IF, 0x7a, 0x7c, 0x7b, -1);
    // The chat line (+0x5d8 text, +0x5dc SEditBox, +0x75c "Send message to
    // allies only" SCheckBox): chatline.cpp.
    PzChatLineCreate(v);

    // Minimap (view child at (0xb, 0x24c)).
    v->InsertChild(&h->Minimap);
    h->Minimap.SetPosition(0xb, 0x24c, 0, 0);
    h->Minimap.Create(IF, 0, 0, 0xf0);                             // 0x64c2f0
    h->Minimap.Cursor = 0xb;
    // HD keeps the SMinimap at view +0x7cc, so its frame (+0x5c) is view
    // +0x828: the minimap frame LoadMap passes to the SGameLogic ctor.
    v->LogicFrame828 = h->Minimap.Frame;
    // Minimap buttons on the panel.
    AddButton(&h->Panel, &h->MarkerButton, 0xcb, 0x2c, IF, 0x1a, 0x1b, 0x1c, -1);
    h->MarkerButton.SetVisible(false);
    AddButton(&h->Panel, &h->TerrainButton, 0xcc, 0x51, IF, 0x1e, 0x1f, 0x20, -1);
    h->TerrainButton.SetChecked(true);
    AddButton(&h->Panel, &h->AllyColorsButton, 0xc4, 0x77, IF, 0x22, 0x23, 0x24, -1);
    h->AllyColorsButton.SetVisible(false);
    // Air support (shown by the update when the player has them).
    AddButton(&h->Panel, &h->SupportParatroops, 0x10d, -10, IF, 0x29, 0x2a, 0x29, -1);
    h->SupportParatroops.SetVisible(false);
    AddButton(&h->Panel, &h->SupportBomber, 0x141, -10, IF, 0x2c, 0x2d, 0x2c, -1);
    h->SupportBomber.SetVisible(false);
    AddButton(&h->Panel, &h->SupportFighter, 0x175, -10, IF, 0x2f, 0x30, 0x2f, -1);
    h->SupportFighter.SetVisible(false);
    AddButton(&h->Panel, &h->SupportArtillery, 0x1a9, -10, IF, 0x32, 0x33, 0x32, -1);
    h->SupportArtillery.SetVisible(false);
    AddButton(&h->Panel, &h->SupportRecon, 0x1dd, -10, IF, 0x35, 0x36, 0x35, -1);
    h->SupportRecon.SetVisible(false);
    h->SupportCount[4] = Board->CreateFrame(FT_TEXT, h->SupportParatroops.GetFrame(), 8, 0x20, 0, 1);
    h->SupportCount[3] = Board->CreateFrame(FT_TEXT, h->SupportBomber.GetFrame(), 8, 0x20, 0, 1);
    h->SupportCount[2] = Board->CreateFrame(FT_TEXT, h->SupportFighter.GetFrame(), 8, 0x20, 0, 1);
    h->SupportCount[0] = Board->CreateFrame(FT_TEXT, h->SupportArtillery.GetFrame(), 8, 0x20, 0, 1);
    h->SupportCount[1] = Board->CreateFrame(FT_TEXT, h->SupportRecon.GetFrame(), 8, 0x20, 0, 1);

    // The unit info of the panel.
    int pf = h->Panel.GetFrame();
    h->UnitHeader = Board->CreateFrame(FT_SPRITE, pf, 0xff, 0x33, 0, 1);
    PzSetSpriteGlyph(h->UnitHeader, IF, 0xf1);
    h->UnitName = Board->CreateFrame(FT_TEXT, pf, 0x118, 0x35, 0, 1);
    for (int i = 0, x = 0x26b, cx = 0x103; x < 0x2a7; ++i, x += 0xf, cx += 0x21) {
        h->Stars[i] = Board->CreateFrame(FT_SPRITE, pf, x, 0x34, 0, 1);
        AddInfo(&h->Panel, &h->Crew[i], cx, 0x86, IF, 0xf4);       // hint "Crew"
    }
    h->_2490 = Board->CreateFrame(FT_SPRITE, pf, 0x106, 0x4d, 0, 1);
    static const struct { int y, glyph, tx, ty; } kInfo[5] = {
        { 0x49, 0xfa, 0x1bd, 0x4c },   // HP
        { 0x61, 0xfb, 0x1bd, 100 },    // Ammo
        { 0x61, 0xfc, 0x1bd, 0x61 },   // Cargo
        { 0x79, 0xfd, 0x1bd, 0x7c },   // XP
        { 0x91, 0xfe, 0x1bd, 0x94 },   // Thermostat
    };
    for (int i = 0; i < 5; ++i) {
        AddInfo(&h->Panel, &h->Info[i], 0x191, kInfo[i].y, IF, kInfo[i].glyph);
        h->InfoText[i] = Board->CreateFrame(FT_TEXT, pf, kInfo[i].tx, kInfo[i].ty, 0, 0);
    }
    h->ArmorText[0] = Board->CreateFrame(FT_TEXT, pf, 0x25a, 0x4f, 0, 0);
    h->ArmorText[1] = Board->CreateFrame(FT_TEXT, pf, 0x227, 0x74, 0, 0);
    h->ArmorText[2] = Board->CreateFrame(FT_TEXT, pf, 0x28e, 0x74, 0, 0);
    h->ArmorText[3] = Board->CreateFrame(FT_TEXT, pf, 0x25a, 0x99, 0, 0);
    h->Panel.InsertChild(&h->UnitButton);
    h->UnitButton.SetPosition(0x23f, 0x5f, 0, 0);
    h->UnitButton.Create(IF, 0xda, 0x187, 0xdb);                   // 0x61e310
    h->UnitButton.Cursor = 0;
    h->UnitButton.SetVisible(false);
    h->Vehicle[2] = Board->CreateFrame(FT_TEXT, pf, 0x2b6, 0x3c, 0, 1);
    h->Vehicle[3] = Board->CreateFrame(FT_TEXT, pf, 0x2b6, 0x71, 0, 1);
    h->Vehicle[0] = Board->CreateFrame(FT_SPRITE, pf, 0x2b6, 0x3c, 0, 1);
    h->Vehicle[1] = Board->CreateFrame(FT_SPRITE, pf, 0x2b6, 0x71, 0, 1);
    for (int i = 0, y = 0x36; i < 2; ++i, y += 0x38) {
        h->Panel.InsertChild(&h->Units2[i]);
        h->Units2[i].SetPosition(0x2b6, y, 0, 0);
        h->Units2[i].Create(IF, 0xda, 0x187, 0xdb);
        h->Units2[i].Cursor = 0;
        h->Units2[i].SetVisible(false);
    }
    for (int i = 0; i < 16; ++i) {
        int col = i < 8 ? i : i - 2;
        h->Panel.InsertChild(&h->Selection[i]);
        h->Selection[i].SetPosition((col + 5) * 0x38, i > 7 ? 0x6e : 0x36, 0, 0);
        h->Selection[i].Create(IF, 0xda, 0x187, 0xdb);
        h->Selection[i].Cursor = 0;
        h->Selection[i].SetVisible(false);
    }
    // Hero photos (view children on the right edge).
    h->HeroFont = Board->LoadCustomFont("menu/hero_photo_hq.tga", 0x15, const_cast<SCustomGlyph*>(kHeroPhotoGlyphs), Default);
    for (int i = 0, y = 0x1e3; y > -0x2a; ++i, y -= 0x69) {
        v->InsertChild(&h->Heroes[i]);
        h->Heroes[i].SetPosition(0x3a7, y, 0x4a, 0x60);
        h->Heroes[i].Create(h->HeroFont, 0);                       // 0x61e1e0
        h->Heroes[i].Cursor = 0;
        h->Heroes[i].SetVisible(false);
    }
    // The six empty command slots, then the command buttons.
    static const int kEmptyX[6] = { 0x353, 0x385, 0x3b7, 0x353, 0x385, 0x3b7 };
    static const int kEmptyY[6] = { 0x1c, 0x1c, 0x1c, 0x4e, 0x4e, 0x4e };
    for (int i = 0; i < 6; ++i) {
        AddCommand(&h->Panel, &h->Empty[i], kEmptyX[i], kEmptyY[i], IF, 0xde + 3 * i, -1, -1);
        h->Empty[i].SetVisible(false);
    }
    AddCommand(&h->Panel, &h->Stop, 0x353, 0x1c, IF, 0xde, -1, 0x7d);
    AddCommand(&h->Panel, &h->Attack, 0x385, 0x1c, IF, 0xe1, -1, 0x81);
    AddCommand(&h->Panel, &h->Move, 0x3b7, 0x1c, IF, 0xe4, -1, 0x85);
    AddCommand(&h->Panel, &h->Reverse, 0x353, 0x4e, IF, 0xe7, -1, 0x89);
    AddCommand(&h->Panel, &h->Equip1, 0x353, 0x4e, IF, 0xe7, 0x154, 0x89);
    h->Equip1.SetVisible(false);
    AddCommand(&h->Panel, &h->Leave, 0x385, 0x4e, IF, 0xea, -1, 0x156);
    h->Leave.SetVisible(false);
    AddCommand(&h->Panel, &h->Equip2, 0x385, 0x4e, IF, 0xea, 0x154, 0x89);
    h->Equip2.SetVisible(false);
    AddCommand(&h->Panel, &h->Suppress, 0x3b7, 0x4e, IF, 0xed, -1, 0x59);
    AddCommand(&h->Panel, &h->Heal, 0x3b7, 0x4e, IF, 0xed, -1, 0x68);
    h->Heal.Highlight = true;                                      // [0xb73] = 1
    Board->ShowFrame(h->Heal.ExtraFrame, true);                    // [0xb74]
    AddCommand(&h->Panel, &h->Repair, 0x3b7, 0x4e, IF, 0xed, -1, 0x15c);
    h->Repair.Highlight = true;
    Board->ShowFrame(h->Repair.ExtraFrame, true);
    AddCommand(&h->Panel, &h->Resupply, 0x3b7, 0x4e, IF, 0xed, -1, 0x62);
    h->Resupply.Highlight = true;
    Board->ShowFrame(h->Resupply.ExtraFrame, true);
    AddCommand(&h->Panel, &h->ResupplyAll, 0x3b7, 0x4e, IF, 0xed, -1, 0x15c);
    h->ResupplyAll.Highlight = true;
    Board->ShowFrame(h->ResupplyAll.ExtraFrame, true);
    h->ResupplyAll.SetVisible(false);
    AddCommand(&h->Panel, &h->Attach, 0x3b7, 0x4e, IF, 0xed, -1, 0x5c);
    AddCommand(&h->Panel, &h->Detach, 0x3b7, 0x4e, IF, 0xed, -1, 0x5f);
    // Behaviour states and stances.
    AddButton(&h->Panel, &h->StateFree, 0x348, 0x82, IF, 0x90, 0x8f, 0x8e, -1);
    AddButton(&h->Panel, &h->StateStand, 0x380, 0x82, IF, 0x94, 0x93, 0x92, -1);
    AddButton(&h->Panel, &h->StatePassive, 0x3b8, 0x82, IF, 0x98, 0x97, 0x96, -1);
    h->PanelFrame = Board->CreateFrame(FT_SPRITE, v->GetFrame(), 0x321, 0x24b, 0, 1);
    PzSetSpriteGlyph(h->PanelFrame, IF, 0xa1);
    Board->ShowFrame(h->PanelFrame, false);
    AddButton(&h->Panel, &h->StanceRun, 0x315, 0x1c, IF, 0x9c, 0x9b, 0x9a, -1);
    AddButton(&h->Panel, &h->StanceCrouch, 0x315, 0x4a, IF, 0x108, 0x107, 0x106, -1);
    AddButton(&h->Panel, &h->StanceCrawl, 0x315, 0x78, IF, 0xa0, 0x9f, 0x9e, -1);
    // The "Click to continue" text (+0x388c) and the bug-report dialog
    // (+0x38a4 ...) that follow in HD stay with agent V / F (loading screen).
}

void PzHudDestroy(SGameView* v)
{
    PzChatLineDestroy(v);
    SGameHud* h = PzHud(v);
    if (!h)
        return;
    int fonts[] = { h->MultiUnitFont, h->HeroFont, h->UnitFonts[0], h->UnitFonts[1], h->UnitFonts[2], h->UnitFonts[3] };
    int ifFont = h->InterfaceFont;
    delete h;                                                      // the members unlink themselves from the view
    s_Hud = nullptr;
    PzMinimapRelease();
    PzCursorColorReset();
    for (int f : fonts)
        if (f >= 0)
            Board->ReleaseFont(f);
    PzReleaseCustomFont(ifFont);
}

// PANZERS 0x625d80 (the widget part; the sub-viewports are agent V's)
void PzHudSetPanelMode(SGameView* v, int mode)
{
    SGameHud* h = PzHud(v);
    if (!h)
        return;
    if (mode != 0) {
        h->PanelToggle.SetVisible(false);
        for (int i = 0; i < 9; ++i)
            h->Groups[i].SetVisible(false);
        for (int i = 0; i < 5; ++i)
            h->Heroes[i].SetVisible(false);
    }
    bool show = mode == 0;
    h->TopBar.SetVisible(show);
    h->Panel.SetVisible(show);
    h->Minimap.SetVisible(show);
    Board->ShowFrame(h->PanelFrame, show);
}

// PANZERS 0x5f5970
// SWorld: has[g] = 1 (g = 1..9) when a live unit is in group g (+0x10c);
// has[0] = the group (0 = no group) holding every selected unit (+0x104
// bit 0) and no unselected one, if exactly one group has selected units;
// else 0.
static void WorldGroupState(pz::SWorld* w, int has[10])
{
    int unsel[10], sel[10];
    for (int i = 0; i < 10; ++i)
        has[i] = unsel[i] = sel[i] = 0;
    for (int i = 0; i < w->Units.Size; ++i) {
        if (!w->Units.IsLive(i))
            continue;
        pz::SUnit* u = w->Units.Array[i].Unit;
        int g = u->_10c;
        if (0 < g && g < 10)
            has[g] = 1;
        // HD indexes its counters with any g < 10; groups are 0..9.
        if (g < 10 && g >= 0) {
            if ((u->_104 & 1) == 0)
                ++unsel[g];
            else
                ++sel[g];
        }
    }
    int found = -1;
    for (int g = 0; g < 10; ++g) {
        if (sel[g] == 0)
            continue;
        if (unsel[g] != 0 || found != -1)
            return;
        found = g;
    }
    if (found != -1)
        has[0] = found;
}

// HD 0x56b3e0 (the selection summary of Update) starts with the heroes: the
// first five live units (heap order) of the local player whose prototype
// has a hero picture (+0x34 >= 0). The rest of 0x56b3e0 (the selection's
// command flags) is not lifted.
static void HudHeroList(pz::SWorld* w, int heroes[5])
{
    for (int i = 0; i < 5; ++i)
        heroes[i] = -1;
    int n = 0;
    for (int i = 0; i < w->Units.Size; ++i) {
        if (!w->Units.IsLive(i))
            continue;
        pz::SUnit* u = w->Units.Array[i].Unit;
        if (u->Proto->HeroPicture >= 0 && w->LocalPlayer == u->Player && n < 5)
            heroes[n++] = u->WorldIndex;                           // +0x74
    }
}

// The HUD part of SGameView::Update 0x628430 that the recompile has so far.
// HD: air support buttons from the local player's record (World +0x16c,
// stride 0x48: +0x19c artillery, +0x1a0 recon, +0x1a4 fighter, +0x1a8
// bomber, +0x1ac paratroops), all hidden when campaign +0x14 is set; the
// clock (logic frames / 20, blinking while paused, 0x56d1b0); with no
// selection the unit panel is empty.
void PzHudUpdate(SGameView* v)
{
    SGameHud* h = PzHud(v);
    if (!h || !pz::g_World)
        return;
    PzMinimapUpdate(v);                                            // the minimap part of 0x5638f0
    PzCursorColorUpdate(v);
    unsigned char* w = (unsigned char*)pz::g_World;
    int player = *(int*)(w + 0x16c);
    const int* support = (const int*)(w + 0x19c + player * 0x48);
    pz::SButton* buttons[5] = { &h->SupportArtillery, &h->SupportRecon, &h->SupportFighter,
                                &h->SupportBomber, &h->SupportParatroops };
    bool noSupport = pz::g_Campaign && ((unsigned char*)pz::g_Campaign)[0x14] != 0;
    char buf[32];
    for (int i = 0; i < 5; ++i) {
        bool on = support[i] != 0 && !noSupport;
        if (buttons[i]->Visible != on)
            buttons[i]->SetVisible(on);
        sprintf(buf, "%d", support[i]);
        Board->SetText(h->SupportCount[i], g_PzFont[PZF_SANS14], 0, buf);
    }
    // The groups (0x628ed8): the top-left button shows glyph 0x15 while one
    // whole group is selected (0x14 otherwise); in the game panel mode the
    // icons of the groups that have units, the selected one highlighted.
    int groups[10];
    WorldGroupState(pz::g_World, groups);                          // 0x5f5970(World +0x3e40)
    int tg = groups[0] == 0 ? 0x14 : 0x15;
    h->PanelToggle.SetGlyphs(h->InterfaceFont, tg, tg, -1, -1);    // +0xc68, 0x537d10
    if (v->ViewState == 0) {                                       // +0x3e80
        h->PanelToggle.SetVisible(true);                           // vtbl +0x6c
        for (int i = 0; i < 9; ++i) {
            h->Groups[i].SetVisible(groups[i + 1] != 0);
            bool sel = i == groups[0] - 1;
            if (h->Groups[i].Selected != sel) {                    // +0x74
                h->Groups[i].Selected = sel;
                h->Groups[i].Redraw();                             // 0x626200
            }
        }
    }
    // The hero photos (0x629497): up to five heroes of the local player.
    int heroes[5];
    HudHeroList(pz::g_World, heroes);                              // 0x56b3e0, the hero part
    if (v->ViewState == 0) {
        for (int i = 0; i < 5; ++i) {
            if (heroes[i] != -1)
                h->Heroes[i].SetHero(heroes[i]);                   // 0x626690
            h->Heroes[i].SetVisible(heroes[i] != -1);              // vtbl +0x6c
        }
    }
    // Clock.
    pz::SGameLogic* gl = pz::g_GameLogic;
    if (gl) {
        int sec = gl->Frame / 0x14;
        unsigned blink = (unsigned)v->ClockStart;                  // [0x117] (+0x45c) / 500 & 1
        if (gl->Running == 0 && ((blink / 500) & 1) == 0) {
            Board->SetText(h->ClockFrame, g_PzFont[PZF_SANS14], 1, "");
        } else {
            int min = sec / 0x3c;
            if (min < 0x3c)
                sprintf(buf, "%d:%02d", min, sec % 0x3c);
            else
                sprintf(buf, "%d:%02d:%02d", min / 0x3c, min % 0x3c, sec % 0x3c);
            Board->SetText(h->ClockFrame, g_PzFont[PZF_SANS14], 1, buf);
        }
    }
    // PAUSE / 2x (+0x5d4, V's frame): "2x" at double speed, "PAUSE"
    // blinking once a second while paused (ROUND(seconds) & 1).
    if (gl && v->PauseText >= 0) {
        const char* t = "";
        if (gl->Running == 2)
            t = "2x";                                              // 0x803ef4
        else if (gl->Running == 0 && (((unsigned)floor((double)v->NowMs() / 1000.0 + 0.5)) & 1))
            t = Gv("PAUSE");
        Board->SetText(v->PauseText, g_PzFont[PZF_SANS14], 1, t);
    }
    // The command highlight [0xa9c] is hidden while no command is active
    // (0x628430: local_114 < 0); no selection here, so always.
    Board->ShowFrame(h->PanelFrame, false);
    // The empty panel (no unit selected; the selection display is part of
    // V's 0x628430 lift).
    static bool cleared = false;
    if (!cleared || h->Stop.Visible) {
        cleared = true;
        for (int i = 0; i < 4; ++i) {
            Board->ShowFrame(h->Vehicle[i], false);
            Board->ShowFrame(h->Stars[i], false);
            h->Crew[i].SetVisible(false);
        }
        Board->ShowFrame(h->_2490, false);
        Board->ShowFrame(h->UnitHeader, false);
        Board->ShowFrame(h->UnitName, false);
        h->UnitButton.SetVisible(false);
        for (int i = 0; i < 2; ++i)
            h->Units2[i].SetVisible(false);
        for (int i = 0; i < 6; ++i) {
            h->Empty[i].SetVisible(false);
            h->MpUnits[i].SetVisible(false);
            Board->ShowFrame(h->MpUnitText[i], false);
        }
        SCommandButton* cmds[] = { &h->Stop, &h->Move, &h->Reverse, &h->Attack, &h->Suppress, &h->Heal,
                                   &h->Attach, &h->Detach, &h->Repair, &h->Resupply, &h->ResupplyAll,
                                   &h->Leave, &h->Equip1, &h->Equip2 };
        for (SCommandButton* c : cmds)
            c->SetVisible(false);
        for (int i = 0; i < 5; ++i) {
            h->Info[i].SetVisible(false);
            Board->ShowFrame(h->InfoText[i], false);
        }
        for (int i = 0; i < 4; ++i)
            Board->ShowFrame(h->ArmorText[i], false);
    }
}

// The HUD-button cases of SGameView::OnAction 0x6216b0 (button down
// 0x42541 for the toggles and states, click 0x42542 for the others).
bool PzHudAction(SGameView* v, SWidget* s, int action, int param)
{
    (void)param;
    SGameHud* h = PzHud(v);
    if (!h || !s)
        return false;
    pz::SGameLogic* gl = pz::g_GameLogic;
    bool shift = false;                                            // [0x27] (+0x9c) key state: agent O's input code
    if (action == 0x42541) {
        if (s == &h->StateFree || s == &h->StateStand || s == &h->StatePassive) {
            int st = s == &h->StateFree ? 0 : s == &h->StateStand ? 1 : 2;
            if (gl)
                pz::Pkt_Flag(gl, pz::PZ_PKT_26, st != 0);          // 0x575910(st): HD writes the byte st (0..2)
            static_cast<pz::SButton*>(s)->SetChecked(true);
            return true;
        }
        if (s == &h->StanceRun || s == &h->StanceCrouch || s == &h->StanceCrawl) {
            int st = s == &h->StanceRun ? 0 : s == &h->StanceCrouch ? 1 : 2;
            if (gl)
                pz::Pkt_TwoFlags(gl, pz::PZ_PKT_2D, st != 0, false);   // 0x575940(st, 0): HD writes the byte st
            static_cast<pz::SButton*>(s)->SetChecked(true);
            return true;
        }
        if (s == &h->MarkerButton) {
            h->MarkerButton.SetChecked(!h->MarkerButton.Checked);
            return true;
        }
        if (s == &h->TerrainButton) {
            h->TerrainButton.SetChecked(!h->TerrainButton.Checked);
            h->Minimap.SetTerrain(h->TerrainButton.Checked);               // 0x64c390(byte +0x1502)
            return true;
        }
        if (s == &h->AllyColorsButton) {
            h->AllyColorsButton.SetChecked(!h->AllyColorsButton.Checked);
            return true;
        }
        return false;
    }
    if (action != 0x42542)
        return false;
    if (s == &h->MenuButton) {
        if (!v->InGameMenu)                                        // and no other menu (+0x3e48 .. +0x3e60)
            PzOpenInGameMenu(v);                                   // 0x620080
        return true;
    }
    if (s == &h->ObjectivesButton) {
        if (!v->InGameMenu)
            PzOpenObjectivesMenu(v, true);                         // 0x61ffb0(1)
        return true;
    }
    if (s == &h->PauseButton) {
        if (gl)
            gl->SetRunning(0);                                     // 0x5802f0(0)
        return true;
    }
    if (s == &h->PlayButton) {
        if (gl && (!pz::g_Campaign || pz::g_Campaign->GetMissionResult() == 0))
            gl->SetRunning(1);                                     // LAB_00621843
        return true;
    }
    if (s == &h->FastButton) {
        if (gl && (!pz::g_Campaign || pz::g_Campaign->GetMissionResult() == 0))
            gl->SetRunning(2);
        return true;
    }
    // Air support and the area commands: enter a command mode (+0x478 = 4,
    // +0x3884 = mode) that the next click on the map completes (agent O).
    struct { pz::SButton* b; int mode; } modes[] = {
        { &h->SupportArtillery, 0x16 }, { &h->SupportRecon, 0x17 }, { &h->SupportFighter, 0x18 },
        { &h->SupportBomber, 0x19 }, { &h->SupportParatroops, 0x1a },
        { &h->Reverse, 2 }, { &h->Attack, 4 }, { &h->Heal, 0xf }, { &h->Resupply, 0x11 },
        { &h->Repair, 0x12 }, { &h->ResupplyAll, 0x10 }, { &h->Attach, 0x13 }, { &h->Suppress, 0x15 },
    };
    for (auto& m : modes) {
        if (s == m.b) {
            v->MouseMode = 0;
            v->ReleaseMouse();                                     // 0x5437c0
            v->MouseMode = 4;
            v->CommandMode = m.mode;                               // +0x3884
            if (m.b != &h->SupportArtillery && m.b != &h->SupportRecon && m.b != &h->SupportFighter &&
                m.b != &h->SupportBomber && m.b != &h->SupportParatroops)
                m.b->SetChecked(true);
            return true;
        }
    }
    if (s == &h->Stop) {
        h->Stop.SetChecked(true);
        if (gl)
            pz::Pkt_Flag(gl, pz::PZ_PKT_08, shift);                // 0x576230
        return true;
    }
    if (s == &h->Detach) {
        h->Detach.SetChecked(true);
        STUB_LOG("SGameView 0x6216b0: Detach 0x576460 (no builder in packets.h)");
        return true;
    }
    if (s == &h->Leave) {
        h->Leave.SetChecked(true);
        STUB_LOG("SGameView 0x6216b0: Leave 0x5763f0(-1) (packets.h Pkt_TwoFlags 0x31 takes bools)");
        return true;
    }
    for (int i = 0; i < 6; ++i) {
        if (s == &h->Empty[i]) {
            if (gl)
                pz::Pkt_36(gl, i);                                 // 0x575fd0(i)
            return true;
        }
        if (s == &h->MpUnits[i]) {
            if (gl)
                pz::Pkt_36(gl, -1 - i);
            return true;
        }
    }
    if (s == &h->Move || s == &h->Equip1 || s == &h->Equip2) {
        static_cast<pz::SButton*>(s)->SetChecked(true);
        STUB_LOG("SGameView 0x6280f0 (move / equipment mode, agent V/O)");
        return true;
    }
    return false;
}
