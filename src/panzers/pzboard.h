// src/panzers/pzboard.h
// Panzers-side helpers over the (SWINE-shared) SIBoard, plus the menu skin
// globals that HD PANZERS.exe keeps next to its widget code.
//
// TEMP until P2-B merge: HD Panzers' 2D board interface (DAT_008f1c60) is not
// SWINE's SIBoard. Its slots are shifted (HD +0x08 CreateFrame = SWINE +0x00,
// HD +0x24 SetSprite = SWINE +0x20 SetSpriteGlyph, HD +0x34 SetText = SWINE
// +0x2c, ...) and HD has a texture API (+0x7c LoadTexture, +0x80
// ReleaseTexture) that SWINE expresses as a one-glyph font (LoadSingleFont).
// Every HD call site in src/panzers goes through these helpers or a plain
// SWINE Board-> call carrying the HD slot in a comment, so the mapping can be
// swapped in one place once P2-B lands Panzers' own board.

#ifndef PANZERS_PZBOARD_H
#define PANZERS_PZBOARD_H

// HD board +0x7c: load a TGA as a texture handle. SWINE: one-glyph font.
int  PzLoadTexture(const char* filename);
// HD board +0x80
void PzReleaseTexture(int handle);

// Menu skin handles, loaded by LoadMenuSkins (HD 0x544040).
extern int g_MenuControlsFont;   // HD 0x8da788 menu/controls_hq.tga, 68 glyphs
extern int g_MenuMediumTex;      // HD 0x8da78c menu/medium_menu_hq.tga
extern int g_MenuButtonsTex;     // HD 0x8da790 menu/buttons_menu_hq.tga
extern int g_MenuMessageTex;     // HD 0x8da794 menu/message_hq.tga
extern int g_MenuMessage2Tex;    // HD 0x8da798 menu/message_2_hq.tga

void LoadMenuSkins();            // HD 0x544040

// Panzers font handles 0..5 (SSuperWindow::Initialize 0x657910 loads
// menu/fonts/sans_serif_new/*.font into exactly these slots).
enum PzFont {
    PZF_SANS14 = 0,          // sans_serif_14_hq.font
    PZF_SANS14_BLACK = 1,    // sans_serif_14_black_hq.font
    PZF_SANS21 = 2,          // sans_serif_21_hq.font
    PZF_SANS21_SHADOW = 3,   // sans_serif_21_shadow_hq.font
    PZF_SANS28_SHADOW = 4,   // sans_serif_28_shadow_hq.font
    PZF_SANS18_SHADOW = 5,   // sans_serif_18_shadow_hq.font
};
// Board handle actually holding each Panzers font. HD gets 0..5 by load order
// and Panics otherwise; on the SWINE board other fonts may already occupy
// those slots, so the recompile keeps a map. TEMP until P2-B merge.
extern int g_PzFont[6];

// Localised menu strings: GetText(section, id) and PrepareGetText from
// core/gettext.h (P2-A, HD 0x660c50 / 0x660db0).
#include "gettext.h"

#endif // PANZERS_PZBOARD_H
