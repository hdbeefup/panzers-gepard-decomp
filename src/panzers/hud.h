// src/panzers/hud.h
// In-game HUD widgets owned by SGameView: the unit panel (SUnitButton,
// SHeroUnitButton), the command buttons (SCommandButton, also the five air
// support buttons), the two SSpecInfoWidget at SGameView +0x2614 / +0x2678,
// and the minimap (SMinimap). HD addresses; slots in HD vtable order (only
// the slots a class overrides are listed, the rest are SDXWidget's / HD
// SButton's).
//
// The button classes derive from the HD SButton (vtable +0x7c = redraw);
// SButton is SWINE-shared (src/window) with a different slot layout, so the
// recompile derives them from SDXWidget for behaviour, like mainmenu.h.
//
//   SSpecInfoWidget  vftable 0x803110: +0x00 dtor 0x6194c0 (1)
//   SHeroUnitButton  vftable 0x803190 (32 slots): +0x00 0x619490, +0x24 0x537b30,
//                    +0x28 0x537c40, +0x34 OnMouseOver 0x6251b0, +0x38 0x537bc0,
//                    +0x7c redraw 0x537d60
//   SUnitButton      vftable 0x803214 (32): +0x00 0x6194f0, +0x24 0x537b30 [m3: mkt/play],
//                    +0x28 0x537c40, +0x34 OnMouseOver 0x6251d0, +0x38 0x537bc0,
//                    +0x7c redraw 0x626280 [m3: mkt]
//                    unit panel builder SGameView 0x626c40 (1019 insns) [m3: mkt/play]
//   SCommandButton   vftable 0x803298 (32): +0x00 0x619400, +0x24 0x537b30, +0x28 0x537c40,
//                    +0x34 OnMouseOver 0x625180 (tooltip), +0x38 0x537bc0,
//                    +0x6c SetVisible 0x627da0 [m3: all], +0x7c redraw 0x626150 [m3: mkt/play]
//   SMinimap         vftable 0x808f70: +0x00 dtor 0x64c2c0 (1), +0x24 OnMouseDown 0x64c3b0 (4)
//                    [m3: play] (LMB: camera jump; RMB: move order -> packets.h),
//                    +0x28 OnMouseUp 0x64c510 (4), +0x2c OnMouseMove 0x64c4a0 (3),
//                    +0x38 OnMouseOut 0x64c500 (0), +0x6c SetVisible 0x64c530 (1)
//                    other: 0x64c2f0 (4 args) [m3: mkt] (Create, name guessed),
//                    0x64c580, 0x64c780, 0x64c800, 0x64c870, 0x64c920 (4 args, 502 insns: draw?)
//   Also: STextBox (SWINE-shared candidate, vftable 0x7f3664) for tooltips and
//   the message lines; SMessageBox (0x7f312c, 0x3ec bytes, ctor 0x53e0d0).
//
// SHARED HEADER (owner P0; names and bodies: agent H, src/panzers/hud.cpp,
// minimap.cpp; docs/M3_INTERFACES.md).

#ifndef PANZERS_HUD_H
#define PANZERS_HUD_H

#include "dxwidget.h"

struct SSpecInfoWidget : SDXWidget {
    SSpecInfoWidget();
    ~SSpecInfoWidget() override;                                     // 0x6194c0
};

struct SUnitButton : SDXWidget {
    SUnitButton();
    ~SUnitButton() override;                                         // 0x6194f0
    void OnMouseOver() override;                                     // +0x34 0x6251d0
    virtual void Redraw();                                           // +0x7c 0x626280
};

struct SHeroUnitButton : SUnitButton {
    SHeroUnitButton();
    ~SHeroUnitButton() override;                                     // 0x619490
    void OnMouseOver() override;                                     // +0x34 0x6251b0
    void Redraw() override;                                          // +0x7c 0x537d60
};

struct SCommandButton : SDXWidget {
    SCommandButton();
    ~SCommandButton() override;                                      // 0x619400
    void OnMouseOver() override;                                     // +0x34 0x625180
    void SetVisible(bool visible) override;                          // +0x6c 0x627da0
    virtual void Redraw();                                           // +0x7c 0x626150
};

struct SMinimap : SDXWidget {
    SMinimap();
    ~SMinimap() override;                                            // 0x64c2c0
    void OnMouseDown(int button, int x, int y, int shift) override;  // +0x24 0x64c3b0
    void OnMouseUp(int button, int x, int y, int shift) override;    // +0x28 0x64c510
    void OnMouseMove(int x, int y, int shift) override;              // +0x2c 0x64c4a0
    void OnMouseOut() override;                                      // +0x38 0x64c500
    void SetVisible(bool visible) override;                          // +0x6c 0x64c530
    void Create(int p1, int p2, int p3, int p4);                     // 0x64c2f0 (4) (name guessed)
};

#endif // PANZERS_HUD_H
