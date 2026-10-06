// src/panzers/hud.h
// In-game HUD widgets owned by SGameView: the unit panel (SUnitButton,
// SHeroUnitButton), the command buttons (SCommandButton, also the five air
// support buttons), the SSpecInfoWidget icons (SGameView +0x2614 / +0x2678
// ...), the top-bar group icons (SGroupIcon) and the minimap (SMinimap).
// HD addresses; slots in HD vtable order (only the slots a class overrides
// are listed, the rest are SDXWidget's / HD SButton's).
//
// The button classes derive from the HD SButton (vtable +0x7c = redraw),
// lifted as pz::SButton in hudwidgets.h.
//
//   SSpecInfoWidget  vftable 0x803110 (0x60 bytes): +0x00 dtor 0x6194c0 (1)
//   SHeroUnitButton  vftable 0x803190 (0x94): +0x00 0x619490, +0x24 0x537b30,
//                    +0x28 0x537c40, +0x34 OnMouseOver 0x6251b0, +0x38 0x537bc0,
//                    +0x7c redraw 0x537d60
//   SUnitButton      vftable 0x803214 (0xc0): +0x00 0x6194f0, +0x24 0x537b30 [m3: mkt/play],
//                    +0x28 0x537c40, +0x34 OnMouseOver 0x6251d0, +0x38 0x537bc0,
//                    +0x7c redraw 0x626280 (= 0x537d60) [m3: mkt]
//                    unit state 0x626c40 (1019 insns) [m3: mkt/play]
//   SCommandButton   vftable 0x803298 (0x90): +0x00 0x619400, +0x24 0x537b30, +0x28 0x537c40,
//                    +0x34 OnMouseOver 0x625180 (tooltip), +0x38 0x537bc0,
//                    +0x6c SetVisible 0x627da0 [m3: all], +0x7c redraw 0x626150 [m3: mkt/play]
//   SGroupIcon       (0x78, ctor 0x6189e0): the nine top-bar group icons
//   SMinimap         vftable 0x808f70: +0x00 dtor 0x64c2c0 (1), +0x24 OnMouseDown 0x64c3b0 (4)
//                    [m3: play] (LMB: camera jump; RMB: move order -> packets.h),
//                    +0x28 OnMouseUp 0x64c510 (4), +0x2c OnMouseMove 0x64c4a0 (3),
//                    +0x38 OnMouseOut 0x64c500 (0), +0x6c SetVisible 0x64c530 (1)
//                    Create 0x64c2f0 (4), SetTerrain 0x64c390 (1)
//   Also: STextBox (vftable 0x7f3664) for tooltips and the message lines;
//   SMessageBox (0x7f312c, 0x3ec bytes, ctor 0x53e0d0): hudwidgets.h.
//
// PzHudCreate is the HUD half of SGameView::Create 0x619c90 (from the
// panzers_interface_hq.tga load to the end), agreed with the coordinator:
// agent V's Create calls it, PzHudDestroy runs in the view dtor, and
// PzHudUpdate is the HUD part of the per-frame SGameView::Update 0x628430
// that the recompile has so far (support buttons, clock, pause text, the
// empty unit panel); V's lift of 0x628430 replaces it.
//
// SHARED HEADER (owner P0; names and bodies: agent H, src/panzers/hud.cpp,
// minimap.cpp; docs/M3_INTERFACES.md).

#ifndef PANZERS_HUD_H
#define PANZERS_HUD_H

#include "dxwidget.h"
#include "hudwidgets.h"

struct SGameView;

// HD SSpecInfoWidget (0x60 bytes, ctor 0x618a30): a background sprite plus
// an overlay sprite in a second font (the info icons of the unit panel and
// the market).
struct SSpecInfoWidget : SDXWidget {
    int Font;                        // +0x58
    int SpriteFrame;                 // +0x5c (-1)

    SSpecInfoWidget();                                               // 0x618a30
    ~SSpecInfoWidget() override;                                     // 0x6194c0
    void Create(int font);                                           // 0x61e2d0
    void SetGlyph(int glyph);                                        // 0x6260e0
};

// HD SUnitButton (0xc0 bytes): a unit's state icon with five bars.
struct SUnitButton : pz::SButton {
    int  IconFont;                   // +0x74
    int  RedBox;                     // +0x78 box 0x60ff0000 over the button (hidden)
    int  _7c;                        // +0x7c
    int  Icon1;                      // +0x80 sprite (p4 glyph)
    int  Icon2;                      // +0x84 sprite (p5 glyph)
    int  _88;                        // +0x88
    int  _8c;                        // +0x8c
    int  StateSprites[5];            // +0x90 at (4, 4)
    int  Bars[5];                    // +0xa4 5x5 boxes at (9 + 8i, 0x2f)
    int  Marker;                     // +0xb8 glyph 0x149 at (3, 3)
    int  Unit;                       // +0xbc world unit index (-1 none, -2 / -3 special icons)

    SUnitButton();                                                   // 0x618a60
    ~SUnitButton() override;                                         // 0x6194f0
    void OnMouseOver() override;                                     // +0x34 0x6251d0
    void Redraw() override;                                          // +0x7c 0x626280

    void Create(int font, int glyph, int icon1, int icon2);          // 0x61e310
    void SetUnit(int unit, bool flag);                               // 0x626c40
};

// HD SHeroUnitButton (0x94 bytes): a hero photo with a health bar.
struct SHeroUnitButton : pz::SButton {
    int  PhotoFont;                  // +0x74
    int  HpBar;                      // +0x78 box 0xff00c000
    int  RedBox;                     // +0x7c box 0x60ff0000
    int  _80;                        // +0x80
    int  Photo;                      // +0x84 sprite
    int  _88;
    int  _8c;
    int  Hero;                       // +0x90 (-1)

    SHeroUnitButton();                                               // 0x618a10
    ~SHeroUnitButton() override;                                     // 0x619490
    void OnMouseOver() override;                                     // +0x34 0x6251b0

    void Create(int font, int glyph);                                // 0x61e1e0
};

// HD SCommandButton (0x90 bytes): a button with a three-state icon.
struct SCommandButton : pz::SButton {
    int  IconNormal;                 // +0x74 sprite at (4, 0)
    int  IconDown;                   // +0x78
    int  IconOver;                   // +0x7c
    int  Icon;                       // +0x80 current icon glyph (-1)
    int  _84;
    int  ExtraGlyph;                 // +0x88
    int  ExtraFrame;                 // +0x8c sprite at (0x20, 0)

    SCommandButton();                                                // 0x6189?? (ctor = SButton 0x537a20)
    ~SCommandButton() override;                                      // 0x619400
    void OnMouseOver() override;                                     // +0x34 0x625180
    void SetVisible(bool visible) override;                          // +0x6c 0x627da0
    void Redraw() override;                                          // +0x7c 0x626150

    void Create(int font, int normal, int down, int over, int extra); // 0x619b90
    void SetIcon(int glyph);                                         // (inline in 0x619c90 / Update)
};

// HD SGroupIcon (0x78 bytes): a top-bar group icon (Ctrl+n groups).
struct SGroupIcon : SDXWidget {
    int  Font;                       // +0x58
    int  Glyph[4];                   // +0x5c..+0x68 (n-1, n, n-1, n)
    int  Frame1;                     // +0x6c
    int  Frame2;                     // +0x70
    bool Selected;                   // +0x74
    bool _75;

    SGroupIcon();                                                    // 0x6189e0
    void Create(int font, int n);                                    // (inline in 0x619c90)
};

struct SMinimap : SDXWidget {
    int  _58;                        // +0x58 Create p3
    int  Frame;                      // +0x5c board minimap frame (type 6), child of the parent's frame
    bool Dragging;                   // +0x60

    SMinimap();
    ~SMinimap() override;                                            // 0x64c2c0
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x64c3b0
    void OnMouseUp(int button, int x, int y, int shift) override;    // +0x28 0x64c510
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x64c4a0
    void OnMouseOut() override;                                      // +0x38 0x64c500
    void SetVisible(bool visible) override;                          // +0x6c 0x64c530
    void Create(int p1, int p2, int p3, int p4);                     // 0x64c2f0 (4)
    void SetTerrain(bool on);                                        // 0x64c390 board +0x54
};

// The minimap's calls into the game view (HD SGameView members, minimap
// part; minimap.cpp). Positions are 0..1 on the minimap.
bool PzViewMinimapClick(SGameView* view, float u, float v);          // 0x6206f0
void PzViewMinimapDrag(SGameView* view, float u, float v);           // 0x620af0
void PzViewMinimapOrder(SGameView* view, float u, float v);          // 0x620b40

// The minimap image and overlay (minimap.cpp; the minimap parts of the HD
// SGameLogic ctor 0x55e440, UpdateUnitVisuals 0x5638f0, the vision tick
// 0x565e10 and PingAttackedUnit 0x570f30).
namespace pz { struct SGameLogic; }
void PzMinimapCreate(pz::SGameLogic* logic);                         // 0x55e440 (minimap part)
void PzMinimapUpdate(SGameView* view);                               // 0x5638f0 / 0x565f1d (minimap parts)
void PzMinimapPing(int unit);                                        // 0x570f30 (board +0xb4)
void PzMinimapRelease();

// The game view's cursor over the map (hudcursor.cpp).
void PzHoverCursor(SGameView* view, int x, int y);                   // 0x621540
void PzTargetCursor(SGameView* view, int unit);                      // 0x620bc0 case 4
void PzViewSetCursor(SGameView* view, int cursor, unsigned color);   // 0x543970
void PzCursorColorUpdate(SGameView* view);                           // board +0x9c colour
void PzCursorColorReset();

// The HUD of SGameView (HD widgets at the view offsets given in hud.cpp).
struct SGameHud;
void PzHudCreate(SGameView* view);                                   // 0x619c90 (HUD half)
void PzHudDestroy(SGameView* view);
void PzHudUpdate(SGameView* view);                                   // part of 0x628430
void PzHudSetPanelMode(SGameView* view, int mode);                   // widget part of 0x625d80
SGameHud* PzHud(SGameView* view);
// The top-bar Menu / Objectives buttons: true when the action was handled
// (called from PzGameViewMenuAction).
bool PzHudAction(SGameView* view, SWidget* source, int action, int param);

#endif // PANZERS_HUD_H
