// src/panzers/gameview_loading.cpp
// SGameView loading backdrop: the full-screen picture behind the map load and
// the "Click to continue" screen. HD 0x61f460, called by LoadMap 0x6201c0 (0),
// the load-game LoadMap 0x61f840 (twice) and the in-game map change 0x624770
// (1). Mission start 0x6281a0 releases it.
//
// HD fields (gameview.h still names them after a guess):
//   +0x3894 (SoundHandle3894) board texture of the backdrop (board +0x7c/+0x80)
//   +0x3898 (SoundHandle3898) board sprite frame showing it (board +0x08/+0x0c)

#include <stdio.h>
#include <string.h>
#include "gameview.h"
#include "pzboard.h"
#include "campaign.h"
#include "gamelogic.h"
#include "stream.h"
#include "logger.h"
#include "iconcert.h"
#include "milesconcert.h"
#include "igepard.h"
#include "m3common.h"

// Board +0x0c / +0x80 on the two backdrop handles (the first half of the
// mission start 0x6281a0 after CreateSubViewports; also the start of 0x61f460
// for the texture).
void SGameView::ReleaseLoadingBackdrop()
{
    if (SoundHandle3898 >= 0) {
        Board->DestroyFrame(SoundHandle3898);                      // board +0x0c
        SoundHandle3898 = -1;
    }
    if (SoundHandle3894 >= 0) {
        PzReleaseTexture(SoundHandle3894);                         // board +0x80
        SoundHandle3894 = -1;
    }
}

// PANZERS 0x61f460
void SGameView::ShowLoadingBackdrop(bool plain)
{
    PZ_M3_TRACE("SGameView::ShowLoadingBackdrop (0x61f460)");
    // HD: if Logic (+0x3e44), 0x563860 (destroys the logic's 23 board frames
    // at +0x94) and 0x5638b0. Every recompile caller runs before the mission
    // SGameLogic exists, so Logic is null here.
    if (SoundHandle3894 >= 0) {
        PzReleaseTexture(SoundHandle3894);                         // board +0x80
        SoundHandle3894 = -1;
    }
    SetPanelMode(1);                                               // 0x625d80(1): panels off, full viewport
    pz::SPanzersCampaign* c = pz::g_Campaign;                      // DAT_00929a0c
    if (!c || c->IsTutorialMode() || c->IsScenarioMode() || c->IsMultiMode() || plain) {
        SoundHandle3894 = PzLoadTexture("menu/loading_hq.tga");    // board +0x7c
    } else {
        // Campaign mission: the briefing music and the mission's own picture.
        if (SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert)) {
            pc->ClearPlaylist();                                   // +0x6c
            char track[64];
            for (int i = 1; i < 8; ++i) {
                _snprintf(track, sizeof(track), "music/briefing_%02d.mp3", i);
                track[sizeof(track) - 1] = 0;
                pc->AddToPlaylist(track);                          // +0x70
            }
            pc->ShufflePlaylist();                                 // +0x74
            pc->StartPlaylist(true);                               // +0x78(1)
        }
        // Map name without directory (0x5625a0) and extension (0x597390(0)).
        const char* map = c->GetMapName();                         // 0x592040
        if (!map)
            map = "";
        const char* base = map;
        for (const char* p = map; *p; ++p)
            if (*p == '/' || *p == '\\')
                base = p + 1;
        char name[260];
        strncpy(name, base, sizeof(name) - 1);
        name[sizeof(name) - 1] = 0;
        if (char* dot = strrchr(name, '.'))
            *dot = 0;
        char file[300];
        _snprintf(file, sizeof(file), "menu/briefing/%s_hq.tga", name);
        file[sizeof(file) - 1] = 0;
        const char* pick = FileSystem.Stat(file, nullptr) == 0     // 0x65faf0
            ? file : "menu/briefing/ger-01_hq.tga";
        SoundHandle3894 = PzLoadTexture(pick);                     // board +0x7c
    }
    SoundHandle3898 = Board->CreateFrame(FT_SPRITE, GetFrame(), 0, 0, 0, 0);   // board +0x08(1, +0x48, 0, 0, 0, 0)
    Board->SetSpriteGlyph(SoundHandle3898, SoundHandle3894, 0);   // board +0x24
    Board->ShowFrame(SoundHandle3898, true);                       // (SWINE frames start hidden)
    Board->SetCursor(-1, 0, 0);                                    // board +0x9c(-1, 0, 0, 0)
    Gepard->RenderScene(0);                                        // Gepard +0x3c(0) viewport, +0x50(0, 0): one frame now
}
