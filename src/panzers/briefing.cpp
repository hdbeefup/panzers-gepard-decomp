// src/panzers/briefing.cpp
// SBriefingMenu (briefing.h): the diary page of a campaign mission.
// OWNER: agent K (M4 campaign shell). Lifted from the HD exe.

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "briefing.h"
#include "pzboard.h"
#include "window.h"
#include "campaign.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "gettext.h"
#include "core_common.h"
#include "milesconcert.h"

// The map's base name: GetMapName 0x592040, the file part (0x5625a0), cut
// at the last '.' after the last separator (0x5651f0(0, dot)).
static void MapBaseName(char* out, size_t size)
{
    const char* map = pz::g_Campaign ? pz::g_Campaign->GetMapName() : "";
    const char* base = map;
    for (const char* p = map; *p; ++p)
        if (*p == '/' || *p == '\\')
            base = p + 1;
    strncpy(out, base, size - 1);
    out[size - 1] = 0;
    int dot = -1;
    for (int i = 0; out[i]; ++i) {
        if (out[i] == '/' || out[i] == '\\')
            dot = -1;
        else if (out[i] == '.')
            dot = i;
    }
    if (dot >= 0)
        out[dot] = 0;
}

// PANZERS 0x632f40
SBriefingMenu::SBriefingMenu()
{
    Speech = -1;                                                  // param_1[0x16]
    Texture = -1;                                                 // param_1[0x17]
    _33c = 1;                                                     // param_1[0xcf]
}

// PANZERS 0x634940
SBriefingMenu::~SBriefingMenu()
{
    if (Texture >= 0)
        PzReleaseTexture(Texture);                                // board +0x80
    if (Speech >= 0) {
        if (SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert))
            pc->RemoveSound(Speech);                              // Concert +0x3c
    }
}

// PANZERS 0x63d690
bool SBriefingMenu::OnKeyDown(int key, bool repeat)
{
    (void)key; (void)repeat;
    SendAction(PZA_BRIEFING_DONE, 0);                             // 0x543930(0x424d1, 0)
    return true;
}

// PANZERS 0x63b670
// The picture pressed (0x42541): stop the speech, 0x424d1. Clicks are eaten.
bool SBriefingMenu::OnAction(SWidget* source, int action, int param)
{
    (void)source; (void)param;
    if (action == PZA_BUTTON_CLICK)
        return true;
    if (action != PZA_BUTTON_DOWN)
        return false;
    if (Speech >= 0) {
        if (SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert))
            pc->RemoveSound(Speech);                              // Concert +0x3c
        Speech = -1;
    }
    SendAction(PZA_BRIEFING_DONE, 0);                             // 0x543930(0x424d1, 0)
    return true;
}

// PANZERS 0x63e050 (the briefing text and the diary speech)
static int StartBriefingSpeech()
{
    const char* text = pz::g_Campaign->GetBriefingText();          // 0x591e30
    if (text && *text) {
        // HD: a copy with '#' -> '\n' into the text list +0xd4 (0x53c020(s,
        // 0, 0xd0d0d0, 0)), which is never inserted (briefing.h).
        Logger.g->Log(0, "PZM4: briefing text (%d chars, not shown in HD)", (int)strlen(text));
    }
    char base[260];
    MapBaseName(base, sizeof(base));
    char speech[300];
    _snprintf(speech, sizeof(speech) - 1, "speech/diary/%s.mp3", base);   // 0x52da80
    speech[sizeof(speech) - 1] = 0;
    SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert);
    return pc ? pc->CreateSoundEx(speech, false, 0.0f, 0.0f, false) : -1;   // Concert +0x2c(s, 0, 0, 0, 0)
}

// PANZERS 0x634f20
void SBriefingMenu::Create()
{
    PZ_M3_TRACE("SBriefingMenu::Create (0x634f20)");
    char base[260];
    MapBaseName(base, sizeof(base));
    char name[300];
    _snprintf(name, sizeof(name) - 1, "menu/diary/%s_hq.tga", base);   // 0x52da80
    name[sizeof(name) - 1] = 0;
    const char* pick = FileSystem.Stat(name, nullptr) == 0 ? name : "menu/diary/ger-01_hq.tga";   // 0x65faf0
    Texture = PzLoadTexture(pick);                                // board +0x7c -> +0x5c
    Logger.g->Log(0, "PZM4: briefing %s", pick);
    SetBackgroundSprite(Texture, 0, false, false);                // 0x539ba0(tex, 0)
    SDXWidget::Create(0);                                         // 0x539a10
    InsertChild(&Picture);                                        // vtbl +0x54(+0x60)
    Picture.SetPosition(0, 0, 0x400, 0x300);                      // vtbl +0x08
    Picture.Create(Texture, 0, 0, -1, -1);                        // 0x537a80(tex, 0, 0, -1, -1)
    Picture.Resize(0x400, 0x300);                                 // vtbl +0x10
    int text = Board->CreateFrame(FT_TEXT, GetFrame(), 0x200, 0x2e2, 0, 1);   // board +0x08(2, +0x48, 0x200, 0x2e2, 0, 1)
    Board->SetText(text, g_PzFont[PZF_SANS21_SHADOW], 2, GetText("panzers/MainMenu.cpp", "Click to continue"));   // +0x34(f, 3, 2, s)
    _33c = 1;
    Speech = StartBriefingSpeech();                               // 0x63e050 -> +0x58
    Cursor = 0;                                                   // 0x543970(0, -1)
    SetFocus();                                                   // 0x5439f0
    if (SWindow* w = GetWindowParent())                           // 0x5435b0 + 0x545110
        w->UpdateMouse();
}
