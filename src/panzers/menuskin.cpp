// src/panzers/menuskin.cpp
// Menu skin textures and the controls glyph atlas (HD PANZERS.exe 0x544040).

#include <windows.h>
#include "pzboard.h"
#include "widget.h"
#include "logger.h"

int g_PzFont[6] = { -1, -1, -1, -1, -1, -1 };
int g_MenuControlsFont = -1;
int g_MenuMediumTex = -1;
int g_MenuButtonsTex = -1;
int g_MenuMessageTex = -1;
int g_MenuMessage2Tex = -1;

int PzLoadTexture(const char* filename)
{
    // HD board +0x7c LoadTexture. TEMP until P2-B merge: SWINE one-glyph font.
    return Board->LoadSingleFont(filename, Default);
}

void PzReleaseTexture(int handle)
{
    // HD board +0x80 ReleaseTexture. TEMP until P2-B merge.
    if (handle >= 0)
        Board->ReleaseFont(handle);
}

// Glyph rectangles of menu/controls_hq.tga (x, y, w, h). The HD function
// builds this 0x44-entry table on the stack from 16-byte .rdata blocks at
// 0x7f39d0..0x7f3e00 (MOVAPS/MOVUPS pairs) and passes it to board +0x74.
// Extracted from the bytes of PANZERS.exe, not from the decompiler.
// Index use seen so far: 0..2 big button (normal/over/pressed, SComplexButton
// style 2), 3..5 medium button (style 1), 6..8 small button (style 0),
// 9 menu header plate (SRightMenu::Create 0x64bdf0).
static SCustomGlyph s_ControlsGlyphs[0x44] = {
    {   0,   0, 256, 46 }, {   0,  47, 256, 46 }, {   0,  94, 256, 46 },
    { 275,   0, 196, 46 }, { 275,  47, 196, 46 }, { 275,  94, 196, 46 },
    {   1, 140, 151, 32 }, {   1, 173, 151, 32 }, {   1, 206, 151, 32 },
    { 154, 140, 292, 45 }, {   1, 243, 175, 34 }, {   1, 278, 175, 34 },
    {   1, 313, 175, 34 }, {   1, 348, 175, 34 }, {   1, 383, 175, 24 },
    {   1, 408, 175, 24 }, {   1, 433, 175, 13 }, { 155, 185,  18, 22 },
    { 173, 185,  18, 22 }, { 191, 185,  18, 22 }, { 209, 185,  18, 22 },
    { 227, 185,  18, 22 }, { 245, 185,  18, 22 }, { 263, 185,  18, 22 },
    { 281, 185,  18, 22 }, { 360, 185, 147, 16 }, { 334, 185,  17, 17 },
    { 154, 214,  22, 22 }, { 200, 214,  22, 22 }, { 246, 214,  22, 22 },
    { 177, 214,  22, 22 }, { 223, 214,  22, 22 }, { 269, 214,  22, 22 },
    { 292, 214,  22, 22 }, { 338, 214,  22, 22 }, { 384, 214,  22, 22 },
    { 430, 214,  22, 22 }, { 315, 214,  22, 22 }, { 361, 214,  22, 22 },
    { 407, 214,  22, 22 }, { 453, 214,  22, 22 }, { 476, 214,  18,  1 },
    { 476, 216,  18,  1 }, { 476, 218,  18,  1 }, { 477, 220,  16,  1 },
    { 477, 222,  16,  1 }, { 477, 224,  16,  1 }, { 302, 185,  14, 14 },
    { 316, 185,   1, 14 }, { 319, 185,  14, 14 }, { 302, 199,  14,  1 },
    { 316, 199,   1,  1 }, { 319, 199,  14,  1 }, { 302, 201,  14, 14 },
    { 316, 201,   1, 14 }, { 319, 201,  14, 14 }, { 186, 239,  53, 51 },
    { 239, 239,  53, 51 }, { 292, 239,  53, 51 }, { 345, 239,  53, 51 },
    { 398, 239,  53, 51 }, { 186, 294,  53, 51 }, { 239, 294,  53, 51 },
    { 292, 294,  53, 51 }, { 345, 294,  53, 51 }, { 398, 294,  53, 51 },
    { 184, 349, 128, 67 }, { 452, 141,  15, 15 },
};

// PANZERS 0x544040
void LoadMenuSkins()
{
    // HD: board +0x74 (name, 0x44, table) — 3 stack args; SWINE's
    // LoadCustomFont takes an extra HDMode (Default = 1:1 pixels).
    g_MenuControlsFont = Board->LoadCustomFont("menu/controls_hq.tga", 0x44, s_ControlsGlyphs, Default);
    g_MenuMediumTex = PzLoadTexture("menu/medium_menu_hq.tga");
    g_MenuButtonsTex = PzLoadTexture("menu/buttons_menu_hq.tga");
    g_MenuMessageTex = PzLoadTexture("menu/message_hq.tga");
    g_MenuMessage2Tex = PzLoadTexture("menu/message_2_hq.tga");
}
