// src/game/unitboard.cpp
// The units' board elements (M5-VX): the 2D icons and bars HD draws over the
// units through the board (DAT_008f1c60), and the selection / armour decals
// under a selected vehicle (terrain +0x60 / +0x64).
//
//   SSingleUnit       +0x344..+0x3a4: name text, nation insignia, the
//                     "attacked" / "seen" fog markers, control group number,
//                     rank stars, weapon / crew icons, health bar, heat bar,
//                     low ammo and no-crew icons; five terrain decals.
//   SPanzersSquadUnit +0x360..+0x380: insignia, "attacked" marker, group
//                     number, rank stars, name text.
//   squad member      +0x344 / +0x348: health bar (shown while the squad is
//                     selected).
//   SBuildingUnit     +0x350 / +0x354: health bar (selected).
//   SWasterUnit       +0x354 / +0x358: timer bar.
//
// Nothing here reads or writes logic state or draws a random number: the
// bodies only read unit fields and set board frames / terrain decals.
// Board slots (HD -> SIBoard): +0x08 CreateFrame, +0x0c DestroyFrame, +0x10
// MoveFrame, +0x14 ResizeFrame, +0x18 ShowFrame, +0x24 SetSpriteGlyph, +0x34
// SetText, +0x3c SetBoxColor.

#include <math.h>
#include "unit.h"
#include "singleunit.h"
#include "squadunit.h"
#include "buildingunit.h"
#include "waster.h"
#include "punit.h"
#include "gunner.h"
#include "gamelogic.h"
#include "unitanim.h"
#include "world.h"
#include "worldapi.h"
#include "pz/imodel.h"
#include "pz/iviewport.h"
#include "pz/iterrain.h"
#include "iboard.h"
#include "logger.h"
#include "stub_log.h"
#include "mods.h"
#include "timer.h"
#include "pz/hdmath.h"
// pz/pzgepard.h clashes with the game heaps; the one helper:

extern SIBoard* Board;   // src/window/widget.h (HD DAT_008f1c60)

// SSettings +0xcc / +0xd0 / +0xd4 (OwnIcon, AlliedIcon, EnemyIcon): HD reads
// them through 0x64e060 / 0x64d900 / 0x64def0. Defined in src/panzers/hud.cpp.
int PzUnitIconSetting(int which);

namespace pz {

unsigned GepardArgb(const float* rgba);   // 0x5aa900 clamped float4 -> ARGB (pz/pzgepard.h)

namespace {

// DAT_008f1a74 (SMulti) is never created by the recompile: single player.
bool PzIsMultiplayer() { return false; }

// HD 0x549ab0 (inline): the same side (team 0 = alone).
bool BoardSameSide(int a, int b)
{
    int ta = *(const int*)&g_World->Players[a][0xc];              // World +0x17c + a * 0x48
    if (ta != 0)
        return ta == *(const int*)&g_World->Players[b][0xc];
    return a == b;
}

int PlayerInt(int p, int off) { return *(const int*)&g_World->Players[p][off]; }

bool AnyPlayer() { return g_World->_4d0[0] != 0; }               // World +0x4d0

bool Paused() { return g_GameLogic && g_GameLogic->IsPaused(); }  // 0x56e150

void BShow(int f, bool on) { if (Board && f >= 0) Board->ShowFrame(f, on); }
void BMove(int f, int x, int y) { if (Board && f >= 0) Board->MoveFrame(f, x, y); }
void BSize(int f, int w, int h) { if (Board && f >= 0) Board->ResizeFrame(f, w, h); }
void BGlyph(int f, int font, int g) { if (Board && f >= 0) Board->SetSpriteGlyph(f, font, g); }
void BColor(int f, unsigned c) { if (Board && f >= 0) Board->SetBoxColor(f, c); }
void BDestroy(int f) { if (Board && f >= 0) Board->DestroyFrame(f); }
int BCreate(SFrameType t, int parent = 0) { return Board ? Board->CreateFrame(t, parent, 0, 0, 0, false) : -1; }

// The insignia glyph of a player's nation (World +0x174 + p * 0x48; the
// multiplayer set +0x128 adds the colour row +0x170 * 6).
void SetInsignia(int frame, int player)
{
    if (!PzIsMultiplayer())
        BGlyph(frame, g_World->Insignia, PlayerInt(player, 4));
    else
        BGlyph(frame, g_World->MultiInsignia, PlayerInt(player, 4) + PlayerInt(player, 0) * 6);
}

// The icon settings test of 0x5aaaa0 / 0x599620: true = show the insignia
// and the rank stars.
bool IconAllowed(int player)
{
    int local = g_World->LocalPlayer;
    if (g_World->_13c || (PzUnitIconSetting(0) != 0 && local == player))
        return true;
    if (PzUnitIconSetting(1) != 0) {
        if (BoardSameSide(player, local) && local != player)
            return true;
    }
    if (PzUnitIconSetting(2) != 0 && !BoardSameSide(player, local))
        return true;
    return false;
}

// The HP bar colour: green from 0.6, yellow from 0.3, else red.
unsigned HpColor(float hp)
{
    if ((double)hp < 0.6) {                                       // DAT_007f4550
        if ((double)hp < 0.3)                                     // DAT_007f4548
            return 0xffff0000;
        return 0xffffff00;
    }
    return 0xff00c000;
}

void ProjectUnit(SUnit* u, SIViewport* vp, const float* pos, float* sx, float* sy, float* size)
{
    (void)u;
    float z = 0.0f;
    int fog = 0;
    vp->ProjectToScreen(pos, 1.0f, sx, sy, size, &z, &fog);       // viewport +0x3c
}

} // namespace

// ---------------------------------------------------------------------------
// SWorld (the board part of the ctor 0x5d2f90)

static const SCustomGlyph kWorldSelectionGlyphs[0x38] = {
    { 0x0, 0x0, 0x20, 0x20 },    { 0x20, 0x0, 0x20, 0x20 },   { 0x41, 0x1b, 0x1e, 0x5 },   { 0x62, 0x1c, 0x1c, 0x3 },
    { 0x82, 0x1c, 0x1c, 0x3 },   { 0xa2, 0x1c, 0x1c, 0x3 },   { 0xc1, 0x1, 0x1e, 0x0 },    { 0xc1, 0x1e, 0x1e, 0x0 },
    { 0xc1, 0x1, 0x0, 0x1e },    { 0xde, 0x1, 0x0, 0x1e },    { 0x3f, 0x0, 0x7, 0x9 },     { 0x46, 0x0, 0x7, 0x9 },
    { 0x4d, 0x0, 0x7, 0x9 },     { 0x54, 0x0, 0x7, 0x9 },     { 0x5b, 0x0, 0x7, 0x9 },     { 0x63, 0x0, 0x7, 0x9 },
    { 0x6a, 0x0, 0x7, 0x9 },     { 0x71, 0x0, 0x7, 0x9 },     { 0x78, 0x0, 0x7, 0x9 },     { 0x40, 0x10, 0x2, 0x2 },
    { 0x40, 0x12, 0x2, 0x2 },    { 0x42, 0x10, 0x2, 0x2 },    { 0x42, 0x12, 0x2, 0x2 },    { 0x44, 0x10, 0x2, 0x2 },
    { 0x44, 0x12, 0x2, 0x2 },    { 0x46, 0x10, 0x2, 0x2 },    { 0x46, 0x12, 0x2, 0x2 },    { 0x7e, 0x0, 0x7, 0x9 },
    { 0x85, 0x0, 0x7, 0x9 },     { 0x1, 0x0, 0x14, 0x14 },    { 0x16, 0x0, 0x14, 0x14 },   { 0x2b, 0x0, 0x14, 0x14 },
    { 0x1, 0x15, 0x14, 0x14 },   { 0x16, 0x15, 0x14, 0x14 },  { 0x2b, 0x15, 0x14, 0x14 },  { 0x40, 0x1, 0xc, 0x17 },
    { 0x4d, 0x1, 0xe, 0xe },     { 0x40, 0x18, 0x14, 0xf },   { 0x5b, 0x1, 0x13, 0x5f },   { 0x71, 0x1, 0x9, 0xd },
    { 0x7b, 0x1, 0x9, 0xd },     { 0x85, 0x1, 0x9, 0xd },     { 0x8f, 0x1, 0x9, 0xd },     { 0x99, 0x1, 0x9, 0xd },
    { 0xa3, 0x1, 0x9, 0xd },     { 0xad, 0x1, 0x9, 0xd },     { 0xb7, 0x1, 0x9, 0xd },     { 0xb7, 0xc, 0x9, 0xd },
    { 0x9d, 0x21, 0x20, 0x20 },  { 0xbe, 0x21, 0x20, 0x20 },  { 0xdf, 0x21, 0x20, 0x20 },  { 0x1, 0x15, 0x14, 0x14 },
    { 0x16, 0x15, 0x14, 0x14 },  { 0x1, 0x2a, 0x14, 0x14 },   { 0x16, 0x2a, 0x14, 0x14 },  { 0x9d, 0x42, 0x20, 0x20 },
};   // .rdata 0x801b30.. (copied to the stack by 0x5d2f90)

static const SCustomGlyph kWorldInsigniaGlyphs[6] = {
    { 0x0, 0x1, 0x11, 0x11 },  { 0x11, 0x1, 0x11, 0x11 }, { 0x22, 0x1, 0x11, 0x11 },
    { 0x34, 0x1, 0x11, 0x11 }, { 0x45, 0x1, 0x11, 0x11 }, { 0x56, 0x1, 0x11, 0x11 },
};   // .rdata 0x801d70..0x801dcf

// The board part of SWorld::SWorld 0x5d2f90 (0x5d3580..0x5d36a0): the
// selection icon set (board +0x74, 0x38 glyphs), the insignia sets and the
// four drag-box frames. The selection textures (Gepard +0x44) stay in the
// ctor (world.cpp).
void WorldCreateBoardElements(SWorld* w)
{
    if (!Board)
        return;
    w->BoardIconSet = Board->LoadCustomFont("menu/selection_hq.tga", 0x38,
                                            const_cast<SCustomGlyph*>(kWorldSelectionGlyphs), Default);
    static SCustomGlyph multi[0x60];
    for (int i = 0; i < 0x60; ++i) {
        multi[i].X = (i % 6) * 0x11;
        multi[i].Y = (i / 6) * 0x13 + 1;
        multi[i].Width = 0x11;
        multi[i].Height = 0x11;
    }
    w->Insignia = Board->LoadCustomFont("menu/insignils_hq.tga", 6,
                                        const_cast<SCustomGlyph*>(kWorldInsigniaGlyphs), Default);
    w->MultiInsignia = Board->LoadCustomFont("menu/multiplayer_insignils_hq.tga", 0x60, multi, Default);
    for (int i = 0; i < 4; ++i)
        w->BoardFrames[i] = BCreate(FT_SPRITE);                  // board +0x08(1, 0, 0, 0, 0, 0)
}

// The board part of SWorld::~SWorld 0x5d5510: the four drag-box frames and
// the icon sets.
void WorldReleaseBoardElements(SWorld* w)
{
    for (int i = 0; i < 4; ++i) {
        BDestroy(w->BoardFrames[i]);
        w->BoardFrames[i] = -1;
    }
    if (Board) {
        if (w->BoardIconSet >= 0) Board->ReleaseFont(w->BoardIconSet);
        if (w->Insignia >= 0) Board->ReleaseFont(w->Insignia);
        if (w->MultiInsignia >= 0) Board->ReleaseFont(w->MultiInsignia);
    }
    w->BoardIconSet = w->Insignia = w->MultiInsignia = -1;
}

// ---------------------------------------------------------------------------
// SSingleUnit

// The board part of SSingleUnit::Init 0x5ad150 / 0x5ad9f0 (0x5ad5e0..0x5ad91e):
// the 25 board elements, all hidden.
void SSingleUnit::CreateBoardElements()
{
    if (!::Board)
        return;
    int font = g_World->BoardIconSet;                            // World +0x110
    Board[0] = BCreate(FT_TEXT);                                 // +0x344 name (board +0x34 SetText(f, 0, 1, name, 0))
    BShow(Board[0], false);
    Board[1] = BCreate(FT_SPRITE);                               // +0x348 insignia
    if (!PzIsMultiplayer())
        BGlyph(Board[1], g_World->Insignia, 0);
    else
        BGlyph(Board[1], g_World->MultiInsignia, 0);
    BShow(Board[1], false);
    Board[2] = BCreate(FT_SPRITE);                               // +0x34c "attacked" marker
    int g = 0x31;
    if (Anim && static_cast<SUnitAnimation*>(Anim)->Type == 1) { // anim +0x0c: vehicle
        SPVehicleAnimation* pa = static_cast<SPVehicleAnimation*>(Anim->GetPrototype());   // +0x40
        if (pa->RunningGear->Caterpillar)                        // +0x10 -> +0x0c
            g = 0x32;
    }
    BGlyph(Board[2], font, g);
    BShow(Board[2], false);
    Board[3] = BCreate(FT_SPRITE);                               // +0x350 "seen" marker
    BGlyph(Board[3], font, 0x37);
    BShow(Board[3], false);
    Board[21] = BCreate(FT_BOX);                                 // +0x398 health bar frame
    BShow(Board[21], false);
    Board[20] = BCreate(FT_BOX, Board[21]);                      // +0x394 health bar
    BShow(Board[20], false);
    Board[4] = BCreate(FT_TEXT);                                 // +0x354
    BShow(Board[4], false);
    Board[5] = BCreate(FT_SPRITE);                               // +0x358 group number
    BShow(Board[5], false);
    Board[22] = BCreate(FT_SPRITE);                              // +0x39c low ammo
    BShow(Board[22], false);
    BGlyph(Board[22], font, 0x23);
    Board[23] = BCreate(FT_SPRITE);                              // +0x3a0 no crew
    BGlyph(Board[23], font, 0x25);
    BShow(Board[23], false);
    for (int i = 0; i < 4; ++i) {
        Board[10 + i] = BCreate(FT_SPRITE);                      // +0x36c weapon / crew icons
        BGlyph(Board[10 + i], font, i + 0x1c);
        BShow(Board[10 + i], false);
        Board[6 + i] = BCreate(FT_SPRITE);                       // +0x35c rank stars
        BGlyph(Board[6 + i], font, 0x24);
        BShow(Board[6 + i], false);
    }
    Board[24] = BCreate(FT_SPRITE);                              // +0x3a4 thermometer
    BGlyph(Board[24], font, 0x26);
    BShow(Board[24], false);
    Board[19] = BCreate(FT_BOX, Board[24]);                      // +0x390 heat column
    BShow(Board[19], false);
}

// PANZERS 0x5abde0
// Releases the board elements and the five terrain decals, then SUnit::Uninit.
void SSingleUnit::Uninit()
{
    static const int kOrder[] = { 0, 1, 2, 3, 21, 19, 4, 5, 22, 23 };   // +0x344 .. +0x3a0 as HD
    for (int k : kOrder) {
        BDestroy(Board[k]);
        Board[k] = -1;
    }
    for (int i = 0; i < 4; ++i) {
        BDestroy(Board[10 + i]);
        BDestroy(Board[6 + i]);
        Board[10 + i] = Board[6 + i] = -1;
    }
    BDestroy(Board[24]);
    Board[24] = -1;
    Board[20] = -1;                                              // child of +0x398 (destroyed with it)
    if (g_World && g_World->Terrain)
        for (int i = 14; i <= 18; ++i)
            g_World->Terrain->DestroyEffectDecal(Board[i]);      // terrain +0x64
    SUnit::Uninit();                                             // 0x5b7e40
}

// PANZERS 0x5aaaa0
// Per frame: the model's heat / damage glow (model +0xec / +0xf0), the board
// elements over the unit, placed from the screen projection of the model, and
// the selection / armour decals under a selected unit.
void SSingleUnit::UpdateVisuals(SIViewport* vp)
{
    if (!Model)
        return;
    float pos[3] = { 0.0f, 0.0f, 0.0f };
    Model->GetRenderPosition(pos);                               // model +0x14
    float sx = 0.0f, sy = 0.0f, size = 0.0f;
    ProjectUnit(this, vp, pos, &sx, &sy, &size);
    bool onScreen = sx != 0.0f || sy != 0.0f;
    float w = UnitSize * size * 0.02f;                           // local_30
    int bx = (int)((sx - 1.0f) - w * 20.0f);                     // local_10
    int by = (int)(size * 50.0f * 0.02f + sy + UnitSize * 3.0f); // local_14
    // The heat glow: red, pulsing at 7 rad/s by 7%, up to 0.3 at full heat.
    if (_118 <= 0.0f) {
        Model->SetColor(false, 0);                               // model +0xec
    } else {
        double t = (double)Timer.GetTickValue() / 1000.0 * 7.0;  // 0x661800, DAT_007f5a48
        float c[4];
        c[0] = (float)((HdSin(t) * 0.07 + 1.0) * (double)((_118 / P->Thermostat) * 0.3f));   // 0x78d640
        c[1] = 0.0f;
        c[2] = 0.0f;
        c[3] = 0.0f;
        Model->SetColor(true, GepardArgb(c));                    // 0x5aa900
    }
    // The damage tint: grey HP + 0.5 below half health.
    if (0.5f < HP) {                                             // DAT_007f453c
        Model->SetColor2(false, 0);                              // model +0xf0
    } else {
        float v = HP + 0.5f;
        float c[4] = { v, v, v, 0.0f };
        Model->SetColor2(true, GepardArgb(c));
    }
    if (!::Board)
        return;

    int local = g_World->LocalPlayer;                            // World +0x16c
    bool gl = g_GameLogic != nullptr;                            // DAT_008f2078
    // The fog markers: a unit the local player cannot see that was attacked
    // (+0x26c) or seen (+0x29c) in the last 40 ticks.
    bool show350 = false;
    if ((onScreen && gl && Paused()) || InVehicleAnim || Unplaced || AnyPlayer() || !gl
        || g_GameLogic->CanSeeGroundUnit(local, this)) {
        BShow(Board[2], false);
    } else if (IsRecent26c(local)) {
        BMove(Board[2], (int)(UnitSize * size * 0.02f * 0.0f + (sx - 8.0f)),
              (int)(((sy - 1.0f) - size * 100.0f * 0.02f) + UnitSize * 4.0f));
        BShow(Board[2], true);
    } else {
        BShow(Board[2], false);
        if (WasSeenRecently(local)) {
            BMove(Board[3], (int)(UnitSize * size * 0.02f * 0.0f + (sx - 8.0f)),
                  (int)(((sy - 1.0f) - size * 100.0f * 0.02f) + UnitSize * 4.0f));
            show350 = true;
        }
    }
    BShow(Board[3], show350);

    // Insignia, rank stars and the no-crew icon.
    bool showCrew = false;
    int kind = PlayerInt(Player, 8);                             // World +0x178 + p * 0x48
    if (onScreen && (!gl || !Paused()) && !Unplaced && !_110 && P->ClassType != 4
        && (kind == 0 || kind == 1)
        && (AnyPlayer() || !gl || g_GameLogic->CanSeeGroundUnit(local, this))) {
        BMove(Board[1], bx, by - 0x1f);
        SetInsignia(Board[1], Player);
        for (int i = 0; i < 4; ++i)
            BShow(Board[6 + i], false);
        if (IconAllowed(Player)) {
            BShow(Board[1], true);
            int x = bx + 0x1e;
            for (int i = 0; i < GetRank(); ++i, x -= 10) {       // +0x88
                BShow(Board[6 + i], true);
                BMove(Board[6 + i], x, by - 0xf);
            }
        } else {
            BShow(Board[1], false);
        }
        if (Drivers.Size >= 1 && ActiveDriver == -1 && !P->BuiltInDriver && !_110) {
            BMove(Board[23], bx + 0x32, by - 0x12);
            showCrew = true;
        }
    } else {
        BShow(Board[1], false);
        for (int i = 0; i < 4; ++i)
            BShow(Board[6 + i], false);
    }
    BShow(Board[23], showCrew);

    // The low-ammo icon (blinks every 16 ticks while the ammo is above 0).
    bool showAmmo = false;
    if (onScreen) {
        int team = PlayerInt(local, 0xc);                        // World +0x17c
        bool mine = team == 0 ? local == Player : team == PlayerInt(Player, 0xc);
        if (mine && !(gl && Paused()) && !Unplaced && !_110 && P->ClassType != 5 && Gunners.Size >= 1) {
            SGunner* g0 = Gunners.Array[0];
            if (g0->GetWeaponType() != 0 && g0->AmmoLeft < 0.3f
                && !(0.0f < g0->AmmoLeft && ((gl ? g_GameLogic->Frame : 0) & 0x10) == 0)) {
                BMove(Board[22], bx - 0xf, by - 0xf);
                showAmmo = true;
            }
        }
    }
    BShow(Board[22], showAmmo);

    // A selected unit: health bar, group number, weapon / crew icons, heat bar.
    if (!onScreen || (gl && Paused()) || _104 == 0 || size < 0.0f) {
        BShow(Board[0], false);
        BShow(Board[4], false);
        BShow(Board[5], false);
        for (int i = 0; i < 4; ++i)
            BShow(Board[10 + i], false);
        BShow(Board[24], false);
        BShow(Board[20], false);
        BShow(Board[21], false);
        BShow(Board[19], false);
        if (g_World->Terrain)
            for (int i = 14; i <= 18; ++i) {
                g_World->Terrain->DestroyEffectDecal(Board[i]);  // terrain +0x64
                Board[i] = -1;
            }
        return;
    }
    BShow(Board[20], true);
    BShow(Board[21], true);
    BMove(Board[20], 1, 1);
    BMove(Board[21], bx, by);
    BColor(Board[20], HpColor(HP));
    BColor(Board[21], 0xff000000);
    BSize(Board[20], (int)(HP * 100.0f * UnitSize * size * 0.008f), 4);
    BSize(Board[21], (int)(UnitSize * 100.0f * size * 0.008f + 2.0f), 6);
    if (_10c < 1 || (_104 & 1) == 0) {
        BShow(Board[5], false);
    } else {
        BShow(Board[5], true);
        BGlyph(Board[5], g_World->BoardIconSet, _10c + 0x26);
        BMove(Board[5], (int)(sx - w * 26.0f), (int)(size * 49.0f * 0.02f + sy + UnitSize * 3.0f));
    }
    // Weapon / crew icons: the driver (0x20 without a driver) and up to three
    // gunners (by weapon type; the "inactive" glyph shows a missing crew).
    int n = 0;
    bool any = false;
    int font = g_World->BoardIconSet;
    if (Drivers.Size > 0) {
        int d = ActiveDriver;
        BGlyph(Board[10], font, d != -1 ? 0x1d : 0x20);
        any = d == -1;
        BMove(Board[10], bx + 0x32, by - 0x14);
        n = 1;
    }
    int x = n * 0x16 + bx + 0x32;
    for (int i = 0; i < (Gunners.Size < 3 ? Gunners.Size : 3); ++i, ++n, x += 0x16) {
        SGunner* g = Gunners.Array[i];
        int on = g->Active;
        int glyph = -1;
        switch (g->GetWeaponType()) {                            // 0x584240
        case 0: glyph = on ? 0x1f : 0x22; break;
        case 1: glyph = on ? 0x1e : 0x21; break;
        case 2: glyph = on ? 0x33 : 0x35; break;
        case 3: glyph = on ? 0x34 : 0x36; break;
        default: break;
        }
        if (glyph >= 0) {
            BGlyph(Board[10 + n], font, glyph);
            if (!on)
                any = true;
        }
        BMove(Board[10 + n], x, by - 0x14);
    }
    if (any && !_110) {
        for (int i = 0; i < n; ++i)
            BShow(Board[10 + i], true);
    } else {
        for (int i = 0; i < 4; ++i)
            BShow(Board[10 + i], false);
    }
    // The heat column beside the health bar.
    if (_118 <= 0.0f || HP <= 0.0f) {
        BShow(Board[19], false);
        BShow(Board[24], false);
    } else {
        BShow(Board[19], true);
        float wh = size * UnitSize * 0.02f;
        int hh = (int)((_118 * 70.0f) / P->Thermostat);
        BMove(Board[19], 8, 0x52 - hh);
        BColor(Board[19], 0xffd72634);
        BSize(Board[19], 5, hh);
        BShow(Board[24], true);
        BMove(Board[24], (int)(wh * 30.0f + (sx - 8.0f)),
              (int)(((sy - 1.0f) - size * 80.0f * 0.02f) + UnitSize * 3.0f));
    }
    // The selection and armour decals, re-created every frame. HD destroys
    // the old ones without resetting the slots (they are destroyed again on
    // the next frames until re-created).
    SITerrain* t = g_World->Terrain;
    if (!t)
        return;
    for (int i = 18; i >= 14; --i)
        if (Board[i] != -1)
            t->DestroyEffectDecal(Board[i]);                     // terrain +0x64
#if PANZERS_MOD_BUGFIXES
    for (int i = 14; i <= 18; ++i)
        Board[i] = -1;
#endif
    if ((_104 & 1) == 0)
        return;
    float scale = UnitSize * 0.855f * 0.5f;                      // DAT_007fb6a4, DAT_007f453c
    const double hp2 = 1.5707963705062866;                       // DAT_007f5a38
    Board[14] = t->CreateEffectDecal(g_World->SelectionTextures[0], pos[0], pos[2],
                                     (float)((double)Dir + hp2), scale, 0xffffff, 0, true, true);   // +0x60
    unsigned c;
    // Front (+0x11c): red below 0.7, yellow below 0.85.
    if (Armor[0] < 0.7f && P->FrontArmor > 0.0f) {
        c = 0xff0000;
        Board[15] = t->CreateEffectDecal(g_World->SelectionTextures[1], pos[0], pos[2],
                                         (float)((double)Dir + hp2), scale, c, 0, true, true);
    } else if (Armor[0] < 0.85f && P->FrontArmor > 0.0f) {
        c = 0xffff00;
        Board[15] = t->CreateEffectDecal(g_World->SelectionTextures[1], pos[0], pos[2],
                                         (float)((double)Dir + hp2), scale, c, 0, true, true);
    }
    // Left (+0x120).
    if (Armor[1] < 0.7f && P->LeftSideArmor > 0.0f) {
        Board[16] = t->CreateEffectDecal(g_World->SelectionTextures[2], pos[0], pos[2],
                                         (float)((double)Dir - hp2), scale, 0xff0000, 0, true, true);
    } else if (Armor[1] < 0.85f && P->LeftSideArmor > 0.0f) {
        Board[16] = t->CreateEffectDecal(g_World->SelectionTextures[2], pos[0], pos[2],
                                         (float)((double)Dir - hp2), scale, 0xffff00, 0, true, true);
    }
    // Right (+0x124).
    if (Armor[2] < 0.7f && P->RightSideArmor > 0.0f) {
        Board[17] = t->CreateEffectDecal(g_World->SelectionTextures[2], pos[0], pos[2],
                                         (float)((double)Dir + hp2), scale, 0xff0000, 0, true, true);
    } else if (Armor[2] < 0.85f && P->RightSideArmor > 0.0f) {
        Board[17] = t->CreateEffectDecal(g_World->SelectionTextures[2], pos[0], pos[2],
                                         (float)((double)Dir + hp2), scale, 0xffff00, 0, true, true);
    }
    // Back (+0x128).
    if (Armor[3] < 0.7f && P->BackArmor > 0.0f) {
        Board[18] = t->CreateEffectDecal(g_World->SelectionTextures[1], pos[0], pos[2],
                                         (float)((double)Dir - hp2), scale, 0xff0000, 0, true, true);
    } else if (Armor[3] < 0.85f && P->BackArmor > 0.0f) {
        Board[18] = t->CreateEffectDecal(g_World->SelectionTextures[1], pos[0], pos[2],
                                         (float)((double)Dir - hp2), scale, 0xffff00, 0, true, true);
    }
}

// ---------------------------------------------------------------------------
// SPanzersSquadUnit

// The board part of SPanzersSquadUnit::Init 0x59c470 (0x59c7f8..0x59c96a).
void SPanzersSquadUnit::CreateBoardElements()
{
    if (!::Board)
        return;
    int font = g_World->BoardIconSet;
    _380 = BCreate(FT_TEXT);                                     // +0x380 name (board +0x34)
    BShow(_380, false);
    Board[0] = BCreate(FT_SPRITE);                               // +0x360 insignia
    if (!PzIsMultiplayer())
        BGlyph(Board[0], g_World->Insignia, 0);
    else
        BGlyph(Board[0], g_World->MultiInsignia, 0);
    BShow(Board[0], false);
    Board[1] = BCreate(FT_SPRITE);                               // +0x364 "attacked" marker
    BGlyph(Board[1], font, 0x30);
    BShow(Board[1], false);
    Board[2] = BCreate(FT_TEXT);                                 // +0x368
    Board[3] = BCreate(FT_SPRITE);                               // +0x36c group number
    for (int i = 0; i < 4; ++i) {
        Board[4 + i] = BCreate(FT_SPRITE);                       // +0x370 rank stars
        BShow(Board[4 + i], false);
        BGlyph(Board[4 + i], font, 0x24);
    }
}

// PANZERS 0x599c70
void SPanzersSquadUnit::Uninit()
{
    BDestroy(_380);
    _380 = -1;
    for (int i = 0; i < 8; ++i) {                                // +0x360..+0x37c
        BDestroy(Board[i]);
        Board[i] = -1;
    }
    SUnit::Uninit();
}

// PANZERS 0x599620
// Per frame: the squad's insignia and rank stars over its centre (+0x1c8),
// the fog marker, and the group number while selected.
void SPanzersSquadUnit::UpdateVisuals(SIViewport* vp)
{
    if (!Model || !::Board)
        return;
    float c[3];
    GetCenterPosition(c);                                        // +0x1c8
    float sx = 0.0f, sy = 0.0f, size = 0.0f;
    ProjectUnit(this, vp, c, &sx, &sy, &size);
    int local = g_World->LocalPlayer;
    bool gl = g_GameLogic != nullptr;
    bool onScreen = !(sx == 0.0f && sy == 0.0f);
    bool show364 = false;
    if (!(onScreen && gl && Paused())) {
        if (!(Wrecked || InVehicleAnim || Unplaced || AnyPlayer() || !gl
              || g_GameLogic->CanSeeGroundUnit(local, this))) {
            if (IsRecent26c(local)) {
                BMove(Board[1], (int)(UnitSize * size * 0.02f * 0.0f + (sx - 8.0f)),
                      (int)(((sy - 1.0f) - size * 100.0f * 0.02f) + UnitSize * 4.0f));
                show364 = true;
            }
        }
    }
    BShow(Board[1], show364);
    int kind = PlayerInt(Player, 8);
    if (!onScreen || (gl && Paused()) || Unplaced || _110 || (kind != 0 && kind != 1)
        || (!AnyPlayer() && gl && !g_GameLogic->CanSeeGroundUnit(local, this))) {
        BShow(Board[0], false);
        for (int i = 0; i < 4; ++i)
            BShow(Board[4 + i], false);
    } else {
        int h = 0x50;
        switch (Members.Size) {                                  // +0x17c
        case 1: h = 0x28; break;
        case 2: h = 0x32; break;
        case 3: h = 0x3c; break;
        default: break;
        }
        BMove(Board[0], (int)(sx - 8.0f), (int)(((sy - 1.0f) - (float)h * size * 0.02f) + UnitSize * 3.0f));
        SetInsignia(Board[0], Player);
        for (int i = 0; i < 4; ++i)
            BShow(Board[4 + i], false);
        if (IconAllowed(Player)) {
            BShow(Board[0], true);
            int x = (int)(UnitSize * size * 0.02f * 5.0f + sx);
            int y = (int)(size * 22.0f * 0.02f + sy + UnitSize * 3.0f);   // _DAT_007f35dc
            for (int i = 0; i < GetRank(); ++i, x -= 10) {       // +0x88
                BShow(Board[4 + i], true);
                BMove(Board[4 + i], x, y);
            }
        } else {
            BShow(Board[0], false);
        }
    }
    if (onScreen && !(gl && Paused()) && _104 != 0 && 0.0f <= size) {
        float w = UnitSize * size * 0.02f;
        if (_10c > 0 && (_104 & 1) != 0) {
            BShow(Board[3], true);
            BGlyph(Board[3], g_World->BoardIconSet, _10c + 0x26);
            BMove(Board[3], (int)(sx - w * 26.0f), (int)(size * 49.0f * 0.02f + sy + UnitSize * 3.0f));
            return;
        }
        BShow(Board[3], false);
        return;
    }
    BShow(Board[2], false);
    BShow(Board[3], false);
    BShow(_380, false);
}

// ---------------------------------------------------------------------------
// SPanzersSquadMemberUnit

// The board part of the ctor 0x5977e0: the health bar frame (+0x348) and the
// bar (+0x344, its child).
void SPanzersSquadMemberUnit::CreateBoardElements()
{
    Board348 = BCreate(FT_BOX);
    Board344 = BCreate(FT_BOX, Board348);
}

// The board part of the dtor 0x597940 (the bar goes with its frame).
void SPanzersSquadMemberUnit::ReleaseBoardElements()
{
    BDestroy(Board348);
    Board348 = Board344 = -1;
}

// PANZERS 0x597a10
// Per frame: the health bar under the member while its squad (or the
// building / vehicle it sits in) is selected.
void SPanzersSquadMemberUnit::UpdateVisuals(SIViewport* vp)
{
    if (!::Board)
        return;
    if (g_GameLogic && Paused()) {
        BShow(Board344, false);
        BShow(Board348, false);
        return;
    }
    if (Parent >= 0) {
        int ct = WorldUnit(Parent)->Proto->ClassType;
        if (ct != 5 && ct != 9 && WorldUnit(Parent)->Proto->ClassType != 0xb) {
            BShow(Board344, false);
            BShow(Board348, false);
            return;
        }
    }
    if (!Model)
        return;
    float pos[3] = { 0.0f, 0.0f, 0.0f };
    Model->GetRenderPosition(pos);
    float sx = 0.0f, sy = 0.0f, size = 0.0f;
    ProjectUnit(this, vp, pos, &sx, &sy, &size);
    bool show = true;
    if ((sx == 0.0f && sy == 0.0f)
        || (Unplaced && (Parent < 0 || WorldUnit(Parent)->Proto->ClassType != 0xb || !InVehicleAnim))
        || Wrecked || Parent < 0 || WorldUnit(Parent)->_104 == 0) {
        show = false;
    } else {
        SUnit* par = WorldUnit(Parent);
        if (par->Proto->ClassType == 9
            && (static_cast<SBuildingUnit*>(par)->P->BuildingType != 2
                || (g_GameLogic && !g_GameLogic->CanSeeGroundUnit(g_World->LocalPlayer, this)))) {
            int local = g_World->LocalPlayer;
            if (!BoardSameSide(Player, local)) {
                if (!(Stored.Size > 1 && BoardSameSide(WorldUnit(Stored.Array[1].Unit)->Player, local)))
                    show = false;
            }
        }
    }
    if (show && 0.0f <= size) {
        BShow(Board344, true);
        BShow(Board348, true);
        BMove(Board344, 1, 1);
        BMove(Board348, (int)((sx - 1.0f) - UnitSize * 20.0f * size * 0.02f),
              (int)(size * 10.0f * 0.02f + (sy - 1.0f) + UnitSize * 3.0f));
        BColor(Board344, HpColor(HP));
        BColor(Board348, 0xff000000);
        BSize(Board344, (int)(HP * 100.0f * UnitSize * size * 0.01f), 3);
        BSize(Board348, (int)(UnitSize * 100.0f * size * 0.01f + 2.0f), 5);
        return;
    }
    BShow(Board344, false);
    BShow(Board348, false);
}

// ---------------------------------------------------------------------------
// SBuildingUnit

// The board part of SBuildingUnit::Init 0x548f20 (after SUnit::Init): the
// health bar frame (+0x354) and the bar (+0x350, its child).
void SBuildingUnit::CreateBoardElements()
{
    Board354 = BCreate(FT_BOX);
    Board350 = BCreate(FT_BOX, Board354);
}

// PANZERS 0x546d20
// The health bar over the building while it is selected.
void SBuildingUnit::UpdateVisuals(SIViewport* vp)
{
    if (!Model)
        return;
    float pos[3] = { 0.0f, 0.0f, 0.0f };
    Model->GetRenderPosition(pos);                                // +0x14
    float sx = 0.0f, sy = 0.0f, size = 0.0f;
    ProjectUnit(this, vp, pos, &sx, &sy, &size);
    if (!(g_GameLogic && Paused()) && _104 != 0 && 0.0f <= size) {
        BShow(Board350, true);
        BShow(Board354, true);
        BMove(Board350, 1, 1);
        BMove(Board354, (int)((sx - 1.0f) - UnitSize * 20.0f * size * 0.02f),
              (int)(size * 50.0f * 0.02f + (sy - 1.0f) + UnitSize * 3.0f));
        BColor(Board350, HpColor(HP));
        BColor(Board354, 0xff000000);
        BSize(Board350, (int)(HP * 100.0f * UnitSize * size * 0.01f), 5);
        BSize(Board354, (int)(UnitSize * 100.0f * size * 0.01f + 2.0f), 7);
        return;
    }
    BShow(Board350, false);
    BShow(Board354, false);
}

// ---------------------------------------------------------------------------
// SWasterUnit

// The board part of SWasterUnit::Init 0x5d22b0 / Slot_14 0x5d23d0: the bar
// frame (+0x358) and the bar (+0x354), hidden. (HD creates them again in
// Slot_14 without releasing the first pair.)
void SWasterUnit::CreateBoardElements()
{
    BoardFrame = BCreate(FT_BOX);
    BShow(BoardFrame, false);
    BoardBar = BCreate(FT_BOX, BoardFrame);
    BShow(BoardBar, false);
}

void SWasterUnit::ReleaseBoardElements()
{
    BDestroy(BoardFrame);
    BoardFrame = BoardBar = -1;
}

// PANZERS 0x5d1e30
// The timer bar (HP = time left) under a selected waster.
void SWasterUnit::UpdateVisuals(SIViewport* vp)
{
    if (!Model)
        return;
    float pos[3] = { 0.0f, 0.0f, 0.0f };
    Model->GetRenderPosition(pos);
    float sx = 0.0f, sy = 0.0f, size = 0.0f;
    ProjectUnit(this, vp, pos, &sx, &sy, &size);
    if (!(g_GameLogic && Paused()) && _104 != 0 && 0.0f <= size) {
        BShow(BoardBar, true);
        BShow(BoardFrame, true);
        float f = size * 0.02f;
        BMove(BoardBar, 1, 1);
        BMove(BoardFrame, (int)((sx - 1.0f) - f * 20.0f), (int)(size * 50.0f * 0.02f + (sy - 1.0f) + 3.0f));
        BColor(BoardBar, HpColor(HP));
        BColor(BoardFrame, 0xff000000);
        BSize(BoardBar, (int)(HP * 100.0f * size * 0.01f), 4);
        BSize(BoardFrame, (int)(size * 100.0f * 0.01f + 2.0f), 6);
        return;
    }
    BShow(BoardBar, false);
    BShow(BoardFrame, false);
}

} // namespace pz
