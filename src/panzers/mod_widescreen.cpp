// src/panzers/mod_widescreen.cpp
// MOD_WIDESCREEN: widescreen layout for the 2D menus (see src/core/mods.h).
// Not lifted code; built only with -DPANZERS_MOD_WIDESCREEN=ON.
//
// HD stretches the 1024x768 UI to any window (root scaler with a 1024x768
// virtual size, board +0x58). With the mod, a window wider than 4:3 gives the
// root scaler a virtual size of (768 * w / h) x 768 instead, so the UI keeps
// its aspect at the window height. In that wider virtual space:
//   - top-level widgets laid out against the right edge of the 1024 design
//     (the main menu and other SRightMenu screens, X = 0x3f8 - width) move by
//     the extra width, so they stay at the right edge; every other top-level
//     widget moves by half of it (centred);
//   - the top and bottom bars (SSuperWindow::LoadMenuBackground) are centred,
//     which keeps the logo in the middle, and are extended to the window edges
//     with tiles cut from their own textures.
// Widgets are moved with SetPosition, so hit-testing (virtual coordinates
// from ModWidescreenEventPoint) matches what is drawn.

#include <windows.h>
#include <map>
#include "mod_widescreen.h"

#if PANZERS_MOD_WIDESCREEN

#include "superwindow.h"
#include "properties.h"
#include "logger.h"
#include "igepard.h"

static const int kDesignW = 0x400;    // HD's virtual size
static const int kDesignH = 0x300;
static const int kBottomY = 0x29a;    // bottom bar y (LoadMenuBackground 0x658690)
// Tile width: two rivet spacings (115 px) of both bar textures, so the rivet
// rows continue across the tiles and into the bar.
static const int kTile = 230;
static const int kMaxTiles = 16;      // per side; enough for 32:9 and beyond

static int VirtualWidth(const SSuperWindow* w)
{
    if (!g_ModWidescreen || w->Width <= 0 || w->Height <= 0 || w->Width * 3 <= w->Height * 4)
        return kDesignW;
    return (w->Width * kDesignH + w->Height / 2) / w->Height;
}

void ModWidescreenInit(SProperties* ini)
{
    g_ModWidescreen = !ini || ini->GetInt("Mods", "Widescreen", 1) != 0;
    Logger.g->Log(0, "MOD_WIDESCREEN: %s (panzers.ini [Mods] Widescreen)", g_ModWidescreen ? "on" : "off");
}

bool ModWidescreenEventPoint(SSuperWindow* w, int x, int y, int* vx, int* vy)
{
    int vw = VirtualWidth(w);
    if (vw == kDesignW)
        return false;
    // Same rounding as HD's GetEventTarget 0x657850, with the wider virtual width.
    *vx = ((w->Width >> 1) + x * vw) / w->Width;
    *vy = ((w->Height >> 1) + y * kDesignH) / w->Height;
    return true;
}

// ---- top-level widgets ----

struct SAnchored {
    int DesignX;    // X in the 1024-wide design
    int AppliedX;   // X this mod last set
};
static std::map<SWidget*, SAnchored> s_Anchored;

static void AnchorWidgets(SSuperWindow* w, int vw)
{
    const int centre = (vw - kDesignW) / 2;
    const int right = vw - kDesignW;
    std::map<SWidget*, SAnchored> seen;
    for (SWidget* c = w->Child; c; c = c->Sibling) {
        SAnchored a;
        std::map<SWidget*, SAnchored>::iterator it = s_Anchored.find(c);
        if (it != s_Anchored.end() && it->second.AppliedX == c->X)
            a = it->second;
        else
            a.DesignX = c->X;   // new widget, or it moved itself: its X is a design X
        bool rightAligned = a.DesignX > kDesignW / 2 && a.DesignX + c->Width >= kDesignW - 16;
        int x = a.DesignX + (rightAligned ? right : centre);
        if (c->X != x)
            c->SetPosition(x, c->Y, c->Width, c->Height);
        a.AppliedX = c->X;
        seen[c] = a;
    }
    s_Anchored.swap(seen);
}

// ---- bar extensions ----

struct SBarFill {
    const char* File;
    int SrcLeft;      // tile source x for the left side
    int SrcRight;     // tile source x for the right side
    int Owner;        // the bar frame the tiles belong to (-1: none)
    int Font;         // two-glyph custom font: 0 = left tile, 1 = right tile
    int Frames[kMaxTiles * 2];
    int Count;
};
// Source x: for seamless rivets the left tile must start at a multiple of
// 115 and the right tile at 1024 - a multiple of 115 (= 104 mod 115); both
// avoid the logos (Stormregion 30..240 and nordic games 845..990 on the
// bottom bar, the PANZERS plate 300..720 on the top bar).
static SBarFill s_Top = { "menu/main_menu_top_hq.tga", 0, 794, -1, -1, {}, 0 };
static SBarFill s_Bottom = { "menu/main_menu_bottom_hq.tga", 345, 564, -1, -1, {}, 0 };

static void DestroyTiles(SBarFill& b)
{
    for (int i = 0; i < b.Count; ++i)
        Board->DestroyFrame(b.Frames[i]);
    b.Count = 0;
}

static void UpdateBar(SBarFill& b, int barFrame, int root, int centre, int y)
{
    if (barFrame != b.Owner) {
        // The bar was destroyed (UnloadMenuBackground) or recreated; its old
        // tiles are still children of the root scaler.
        DestroyTiles(b);
        b.Owner = barFrame;
    }
    if (barFrame < 0)
        return;
    Board->MoveFrame(barFrame, centre, y);
    int need = (centre + kTile - 1) / kTile;
    if (need > kMaxTiles)
        need = kMaxTiles;
    if (need > 0 && b.Font < 0) {
        int tw = 0, th = 0;
        Board->GetFrameSize(barFrame, &tw, &th);
        SCustomGlyph g[2] = { { b.SrcLeft, 0, kTile, th }, { b.SrcRight, 0, kTile, th } };
        b.Font = Board->LoadCustomFont(b.File, 2, g, Default);
    }
    while (b.Count < need * 2) {
        int f = Board->CreateFrame(FT_SPRITE, root, 0, y, 0, 0);
        Board->SetSpriteGlyph(f, b.Font, b.Count & 1);
        b.Frames[b.Count++] = f;
    }
    for (int i = 0; i < b.Count; ++i) {
        int k = i >> 1;
        bool show = k < need;
        if (show) {
            int x = (i & 1) ? centre + kDesignW + kTile * k : centre - kTile * (k + 1);
            Board->MoveFrame(b.Frames[i], x, y);
        }
        Board->ShowFrame(b.Frames[i], show);
    }
}

void ModWidescreenFrame(SSuperWindow* w)
{
    if (!Board || w->RootFrame < 0)
        return;
    int vw = VirtualWidth(w);
    AnchorWidgets(w, vw);
    int centre = (vw - kDesignW) / 2;
    UpdateBar(s_Top, w->MenuTopFrame, w->RootFrame, centre, 0);
    UpdateBar(s_Bottom, w->MenuBottomFrame, w->RootFrame, centre, kBottomY);
}

void ModWidescreenOnSize(SSuperWindow* w)
{
    if (!Board || w->RootFrame < 0)
        return;
    Board->SetVirtualSize(w->RootFrame, VirtualWidth(w), kDesignH);
    ModWidescreenFrame(w);
}

void ModWidescreenShutdown()
{
    if (!Board)
        return;
    DestroyTiles(s_Top);
    DestroyTiles(s_Bottom);
    s_Top.Owner = s_Bottom.Owner = -1;
    if (s_Top.Font >= 0)
        Board->ReleaseFont(s_Top.Font);
    if (s_Bottom.Font >= 0)
        Board->ReleaseFont(s_Bottom.Font);
    s_Top.Font = s_Bottom.Font = -1;
    s_Anchored.clear();
}

#endif // PANZERS_MOD_WIDESCREEN
