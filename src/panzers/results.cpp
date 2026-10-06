// src/panzers/results.cpp
// The end of a mission (results.h): the victory / defeat / error box of
// SGameView::Update 0x628430 and the results menu SStatisticMenu.
// OWNER: M3-P. Only UI: no world RNG draw, no CRC field is written here.

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "results.h"
#include "gameview.h"
#include "window.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "gettext.h"
#include "pzboard.h"
#include "properties.h"
#include "iconcert.h"
#include "milesconcert.h"
#include "igepard.h"
#include "campaign.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "doodad.h"

static const char* Gv(const char* id) { return GetText("panzers/GameView.cpp", id); }
static const char* Tx(const char* id) { return GetText("panzers/InGameMenu.cpp", id); }

// HD SGameView fields by HD offset (SGameViewData starts at HD +0x5c).
static SStatisticMenu*& StatisticMenu(SGameView* v)
{
    return *(SStatisticMenu**)&v->HdInt(0x3e6c);
}

// ---------------------------------------------------------------------------
// The end box / results menu of 0x628430
// ---------------------------------------------------------------------------

// The box of the three end cases (0x62bda1 / 0x62bfad / 0x62c166): new
// SMessageBox 0x3ec into +0x3e84, child of the view, Create("", "", OK,
// modal), target the view, title, text in the given colour, shown.
static void ShowEndBox(SGameView* v, const char* title, const char* text, unsigned color)
{
    pz::SMessageBox* box = new pz::SMessageBox();                  // new 0x3ec, 0x53e0d0
    v->EndBox = box;                                               // +0x3e84
    v->InsertChild(box);                                           // vtbl +0x54
    box->Create("", "", PZ_MB_OK, true);                           // 0x53e3b0("", "", 0, 1)
    box->SetTarget(v);                                             // 0x53e8d0
    box->SetTitle(Gv(title));                                      // 0x53e8e0
    box->SetText(Gv(text), color);                                 // 0x53e910
    box->SetVisible(true);                                         // vtbl +0x6c(1)
}

// The results menu (0x62bd57 / 0x62c0ad): logic stopped, new SStatisticMenu
// 700 into +0x3e6c, child of the view, Create.
static void ShowStatisticMenu(SGameView* v)
{
    v->Logic->SetRunning(0);                                       // 0x5802f0(0)
    SStatisticMenu* m = new SStatisticMenu();                      // new 0x2bc, 0x62cd50
    StatisticMenu(v) = m;
    v->InsertChild(m);                                             // vtbl +0x54
    m->Create();                                                   // 0x62ffa0
}

// +0x478 == 1 (box drag) -> SWorld 0x5ddb60, then 0.
static void ResetMouseMode(SGameView* v)
{
    if (v->MouseMode == 1)
        v->World->HideSelectionBox();                              // 0x5ddb60
    v->MouseMode = 0;
}

// Concert +0x80(0), +0x6c, +0x70(track), +0x78(0).
static void PlayEndMusic(const char* track)
{
    if (SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert)) {
        pc->StopStream(false);
        pc->ClearPlaylist();
        pc->AddToPlaylist(track);
        pc->StartPlaylist(false);
    }
}

static bool s_ForceStatistics = false;   // PZ_M3_FORCE_END 11 / 12 (test switch)

// HD 0x628430 (the UI of the single-player end check, 0x62bc65..0x62c233;
// the logic part, BackupCampaignUnits 0x561110 and SetRunning 0x5802f0(0),
// is PzGameViewEndCheck). Multiplayer (DAT_008f1a74) is not handled: the
// recompile has no network game.
void PzShowMissionEnd(SGameView* v, int result, bool out)
{
    PZ_M3_TRACE("SGameView::Update end of mission (0x628430)");
    pz::SPanzersCampaign* c = pz::g_Campaign;
    bool tutorial = c->IsTutorialMode() && !s_ForceStatistics;     // 0x594d40
    if (result == 1) {
        if (!tutorial) {
            // "Autosaving..." (SGameLogic 0x56a480 message, type 2), one frame
            // drawn (viewport +0x50(scene, 0)), SaveGameStartMission("End"),
            // "...done" / "...failed" and an empty line. The message log of
            // 0x56a480 is not in the recompile: logged instead.
            Logger.g->Log(0, "%s", Gv("Autosaving..."));
            int ok = c->SaveGameStartMission("End", Gv("End"));    // 0x596e30
            Logger.g->Log(0, "%s", Gv(ok ? "...done" : "...failed"));
        }
        if (tutorial)
            ShowEndBox(v, "Victory", "CONGRATULATIONS!\n\nYou are victorious!", 0xffffff00);
        else
            ShowStatisticMenu(v);
        ResetMouseMode(v);
        PlayEndMusic("music/Victory.mp3");
    } else if (!out) {
        if (result == 3) {
            ShowEndBox(v, "Error", "Inconsistency detected! Please send the `panzer.log' file to the developer team!",
                       0xd0d0d0);
            ResetMouseMode(v);
        }
    } else {
        if (tutorial) {
            ResetMouseMode(v);
            ShowEndBox(v, "Failure", "YOU LOST!", 0xffff0000);
            v->Logic->SetRunning(0);                               // 0x5802f0(0)
        } else {
            ShowStatisticMenu(v);
            ResetMouseMode(v);
        }
        PlayEndMusic("music/Defeat.mp3");
    }
    Logger.g->Log(0, "PZM3: mission result %d%s: %s", result, out ? " (local player out)" : "",
                  StatisticMenu(v) ? "results menu" : "end box");
}

void PzForcedMissionEnd(int* result, bool* out)
{
    static int forced = -1;
    static DWORD start = 0;
    if (forced < 0) {
        const char* e = getenv("PZ_M3_FORCE_END");
        forced = e ? atoi(e) : 0;
        start = GetTickCount();
    }
    if (forced == 0 || GetTickCount() - start < 10000)
        return;
    if (forced > 10) {
        s_ForceStatistics = true;
        forced -= 10;
    }
    if (forced == 2) {
        *out = true;
    } else {
        *result = forced;
        pz::g_Campaign->SetMissionResult(forced);                  // as the trigger would (test only)
    }
}

void PzDeleteStatisticMenu(SGameView* v)
{
    SStatisticMenu*& m = StatisticMenu(v);
    if (m) {
        delete m;
        m = nullptr;
    }
}

// ---------------------------------------------------------------------------
// SStatisticMenu
// ---------------------------------------------------------------------------

// PANZERS 0x62cd50
SStatisticMenu::SStatisticMenu() {}

// PANZERS 0x62d4a0
SStatisticMenu::~SStatisticMenu()
{
    if (SWindow* w = GetWindowParent())                            // 0x5435b0
        w->UnsetModalWidget(this);                                 // 0x5450e0
}

// PANZERS 0x632570
bool SStatisticMenu::OnKeyDown(int key, bool repeat)
{
    (void)repeat;
    if (key == VK_RETURN)
        SendAction(PZA_STAT_OK, 0);                                // 0x543930(0x4953541, 0)
    return true;
}

// PANZERS 0x6324d0
bool SStatisticMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action == PZA_BUTTON_CLICK && source == &OkButton) {
        SendAction(PZA_STAT_OK, 0);
        return true;
    }
    return false;
}

// HD SPanzersCampaign +0x134 objective records (0x1c each, 0x593ba0):
// +0 state (2 completed), +5 main, +6 secret, +7 (excluded from the counts),
// +8 SString text.
struct PzStatObjective {
    int           State;
    unsigned char _4, Main, Secret, Excluded;
    SString       Text;
    unsigned char _10[0x1c - 0x10];
};
static_assert(sizeof(PzStatObjective) == 0x1c, "objective record stride (0x593ba0)");

static void AddFormat(pz::STextBox& t, unsigned color, const char* fmt, ...)
{
    char buf[0x400];                                               // 0x51ee20: 0x400 bytes
    va_list ap;
    va_start(ap, fmt);
    int n = _vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (n < 1 || n > 0x3ff)
        buf[0] = 0;
    buf[sizeof(buf) - 1] = 0;
    t.AddLine(buf, color);                                         // 0x541b60
}

// 0x5928b0: the prestige of the next mission ([<section>] "Next Mission",
// its "SP"), 500 without a next mission.
static int NextMissionPrestige(pz::SPanzersCampaign* c)
{
    SProperties* p = (SProperties*)c->MissionProps;
    if (!p)
        return 500;
    const char* next = p->GetString(pz::SStr(c->MissionSection), "Next Mission", nullptr);
    if (!next || !*next)
        return 500;
    char name[260];
    strncpy(name, next, sizeof(name) - 1);
    name[sizeof(name) - 1] = 0;
    return p->GetInt(name, "SP", 0);
}

// The medal sheets (0x806eb0, 0x806e20..0x806ea0): glyph 0 the frame, 1.. the
// medals; board +0x74 LoadCustomFont.
static const SCustomGlyph kMedalGlyphs[10] = {
    { 0, 206, 171, 180 },
    { 0, 0, 130, 180 }, { 130, 0, 130, 180 }, { 260, 0, 130, 180 }, { 390, 0, 130, 180 },
    { 520, 0, 130, 180 }, { 650, 0, 130, 180 }, { 780, 0, 130, 180 }, { 910, 0, 130, 180 },
    { 1040, 0, 130, 180 },
};

// PANZERS 0x62ffa0
// "VICTORY" / "DEFEAT", Ok, then the lines: unit statistics (destroyed,
// lost: campaign player record +0x3c / +0x6c), the objectives (main /
// optional / secret done / total and their prestige: next mission's "SP",
// 60 and 40 each), the summary, the failed objective (World player +0x20);
// on a victory the medal of this mission (the nation's mission chain from
// "German 1" / "Russian 1" / "Allied 1" counts the medals before it) and the
// losses stamp (player record +0xd0 percent). Modal.
void SStatisticMenu::Create()
{
    PZ_M3_TRACE("SStatisticMenu::Create (0x62ffa0)");
    pz::SPanzersCampaign* c = pz::g_Campaign;
    pz::SWorld* w = pz::g_World;
    bool victory = c->MissionResult == 1;                          // +0xe4
    SCenterMenu::Create(Tx(victory ? "VICTORY" : "DEFEAT"), false); // 0x64bc80
    InsertChild(&OkButton);                                        // 0x64bfd0
    OkButton.SetPosition(0x18b, 400, 0, 0);
    OkButton.Create(1, Tx("Ok"));
    InsertChild(&Text);                                            // vtbl +0x54
    Text.SetPosition(0x14, 0x37, 0x226, 0);
    Text.Create(5, 0x11, true, false, 0, false);                   // 0x542380(5, 0x11, 1, 0, 0, 0)

    int local = w ? w->LocalPlayer : 0;                            // World +0x16c
    const unsigned char* stats = c->PlayerStats[local];            // campaign +0x140 + local * 0xd8
    Text.AddLine(Tx("Unit statistics"), 0xffff3f);
    AddFormat(Text, 0xffffff, Tx("- Destroyed enemy units: %d"), *(const int*)(stats + 0x3c));
    AddFormat(Text, 0xffffff, Tx("- Lost units: %d"), *(const int*)(stats + 0x6c));
    Text.AddLine("", 0xd0d0d0);
    Text.AddLine(Tx("Mission Objectives"), 0xffff3f);

    int mainDone = 0, mainAll = 0, optDone = 0, optAll = 0, secDone = 0, secAll = 0;
    const PzStatObjective* obj = (const PzStatObjective*)c->Objectives;
    for (int i = 0; i < c->ObjectiveCount; ++i) {
        const PzStatObjective& o = obj[i];
        if (o.Main && !o.Excluded) {
            ++mainAll;
            if (o.State == 2) ++mainDone;
        } else if (o.Secret && !o.Excluded) {
            ++secAll;
            if (o.State == 2) ++secDone;
        } else if (!o.Excluded) {
            ++optAll;
            if (o.State == 2) ++optDone;
        }
    }
    int mainSP = victory ? NextMissionPrestige(c) : 0;             // 0x5928b0
    AddFormat(Text, 0xffffff, Tx("- Main: %d/%d >>> Prestige: %d"), mainDone, mainAll, mainSP);
    AddFormat(Text, 0xffffff, Tx("- Optional: %d/%d >>> Prestige: %d"), optDone, optAll, victory ? optDone * 0x3c : 0);
    AddFormat(Text, 0xffffff, Tx("- Secret: %d/%d >>> Prestige: %d"), secDone, secAll, victory ? secDone * 0x28 : 0);
    int sum = victory ? NextMissionPrestige(c) + secDone * 0x28 + optDone * 0x3c : 0;
    AddFormat(Text, 0x3fff3f, Tx("- Summary Prestige: %d"), sum);
    Text.AddLine("", 0xd0d0d0);

    int failed = w ? *(const int*)(w->Players[local] + 0x20) : 0;  // World +0x190 + local * 0x48
    if (failed != 0) {
        Text.AddLine("", 0xd0d0d0);
        Text.AddLine(Tx("Objective failed:"), 0xff3f3f);
        int k = failed - 1;
        if (k < 0 || k >= c->ObjectiveCount) {
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SCampaign::SObjective", k);
            return;
        }
        // DAT_00929dec picks "-" over "- " (not mapped: "- ").
        char line[0x400];
        _snprintf(line, sizeof(line) - 1, "- %s", pz::SStr(obj[k].Text));
        line[sizeof(line) - 1] = 0;
        Text.AddLine(line, 0xffffff);
        Text.AddLine("", 0xd0d0d0);
    }

    if (victory) {
        // The medal sheet of the nation (campaign +0x18) and the first
        // mission of its chain; the Allied chain is the English one when this
        // mission's number is 1 or 7.
        bool english = false;
        SProperties* p = (SProperties*)c->MissionProps;
        const char* cur = pz::SStr(c->MissionSection);
        auto missionNumber = [&](const char* m) { return p ? p->GetInt(m, "Mission number", 0) : 0; };
        if (c->Race == 1 && (missionNumber(cur) == 1 || missionNumber(cur) == 7))   // 0x5920f0
            english = true;
        int sheet;
        char mission[260];
        if (c->Race == 0) {
            sheet = PzLoadCustomFont("menu/medals_ger_hq.tga", 9, kMedalGlyphs);
            strcpy(mission, "German 1");
        } else if (c->Race == 2) {
            sheet = PzLoadCustomFont("menu/medals_rus_hq.tga", 9, kMedalGlyphs);
            strcpy(mission, "Russian 1");
        } else {
            sheet = english ? PzLoadCustomFont("menu/medals_eng_hq.tga", 3, kMedalGlyphs)
                            : PzLoadCustomFont("menu/medals_usa_hq.tga", 8, kMedalGlyphs);
            strcpy(mission, "Allied 1");
        }
        // Medals earned before this mission along the "Next Mission" chain.
        int medals = 0;
        for (;;) {
            if (_stricmp(mission, cur) == 0)
                break;
            auto hasMedal = [&](const char* m) {
                const char* s = p ? p->GetString(m, "Medal", "") : "";
                return s && *s;
            };
            if (c->Race == 1) {
                int n = missionNumber(mission);
                bool firstOfChain = n == 1 || n == 7;
                if (english) {
                    if (firstOfChain && hasMedal(mission))
                        ++medals;
                } else if (!firstOfChain) {
                    if (hasMedal(mission))
                        ++medals;
                } else {
                    ++medals;                                      // HD 0x630a0b: counts 1 and 7 as earned
                }
            } else if (hasMedal(mission)) {
                ++medals;
            }
            const char* next = p ? p->GetString(mission, "Next Mission", nullptr) : nullptr;
            if (!next || !*next)
                break;
            strncpy(mission, next, sizeof(mission) - 1);
            mission[sizeof(mission) - 1] = 0;
        }
        const char* medal = p ? p->GetString(cur, "Medal", "") : "";
        if (medal && *medal) {
            char medalName[260];
            strncpy(medalName, medal, sizeof(medalName) - 1);
            medalName[sizeof(medalName) - 1] = 0;
            Text.AddLine("", 0xd0d0d0);
            Text.AddLine(Tx("You earned a new medal!"), 0x3fffff);
            Text.AddLine("", 0xd0d0d0);
            InsertChild(&Medal);                                   // vtbl +0x54
            Medal.SetPosition(Width - 0xeb, 0x46, 0, 0);
            Medal.SetTooltipText(medalName);                       // 0x543bf0
            Medal.SetBackgroundSprite(sheet, medals + 1, false, false);   // 0x539ba0
            Medal.Create(0);                                       // 0x539a10
        }
        PzReleaseCustomFont(sheet);                                // board +0x80

        int losses = *(const int*)(stats + 0xd0);                  // campaign player record +0xd0
        int stamp;
        const char* stampText;
        if (losses < 1) {
            stamp = PzLoadTexture("menu/stamp_great_hq.tga");      // board +0x7c
            stampText = Tx("Without losses");
        } else if (losses < 0x34) {
            stamp = PzLoadTexture("menu/stamp_ok_hq.tga");
            stampText = Tx("Your losses less than 50%");
        } else {
            stamp = PzLoadTexture("menu/stamp_bad_hq.tga");
            stampText = Tx("Your losses more than 50%");
        }
        InsertChild(&Stamp);                                       // vtbl +0x54
        Stamp.SetPosition(Width - 0xeb, Height - 0xdc, 0, 0);
        Stamp.SetTooltipText(stampText ? stampText : "");          // 0x543bf0
        Stamp.SetBackgroundSprite(stamp, 0, false, false);         // 0x539ba0
        Stamp.Create(0);                                           // 0x539a10
        PzReleaseTexture(stamp);                                   // board +0x80
    }
    SetFocus();                                                    // 0x5439f0
    if (SWindow* win = GetWindowParent())                          // 0x5435b0
        win->SetModalWidget(this);                                 // 0x544fe0
}
