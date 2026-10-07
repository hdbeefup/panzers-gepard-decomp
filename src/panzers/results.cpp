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

// The same Concert sequence for the game's other one-shot tracks (RunTriggers
// action 0x23 "music/Objective.mp3", 0x57ba97); called from src/game.
void PzPlayMusicTrack(const char* track)
{
    PlayEndMusic(track);
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
    if (getenv("PZ_M3_FORCE_END_ONCE"))                            // TR: end the first mission only (reach mission 2)
        forced = 0;
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
                SProperties* lp = (SProperties*)c->LocalProps;    // "Medal": campaign +0x04 (0x630911)
                const char* s = lp ? lp->GetString(m, "Medal", "") : "";
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
        SProperties* lp = (SProperties*)c->LocalProps;
        const char* medal = lp ? lp->GetString(cur, "Medal", "") : "";
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

// ---------------------------------------------------------------------------
// SResultsMenu (the campaign debriefing; OWNER: agent K, M4)
// ---------------------------------------------------------------------------

static const char* Mm(const char* id) { return GetText("panzers/MainMenu.cpp", id); }

// PANZERS 0x633540
SResultsMenu::SResultsMenu()
{
    Background = -1;
    SaveMenu = nullptr;                                            // param_1[0x439]
    for (auto& row : Grid)
        for (int& f : row)
            f = -1;
}

// PANZERS 0x6341d0
SResultsMenu::~SResultsMenu()
{
    if (SaveMenu) {
        delete SaveMenu;
        SaveMenu = nullptr;
    }
    if (SIPanzersConcert* pc = dynamic_cast<SIPanzersConcert*>(Concert))
        pc->StopStream(true);                                      // Concert +0x80(1)
    if (Background >= 0)
        PzReleaseCustomFont(Background);
}

// PANZERS 0x63c230
bool SResultsMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action == PZA_BUTTON_DOWN) {                               // 0x42541: a page button
        for (int i = 0; i < 12; ++i) {
            if (source != &Pages[i])
                continue;
            for (int k = 0; k < 12; ++k)
                Pages[k].SetChecked(false);                        // 0x537df0(0)
            Pages[i].SetChecked(true);                             // 0x537df0(1)
            ShowPage(i);                                           // 0x63e690(i)
        }
        return true;
    }
    if (action == PZA_BUTTON_CLICK) {                              // 0x42542
        if (source == &Save)                                       // +0x6a8: SSaveMenu 0xbac 0x633820(1), 0x6398f0
            STUB_LOG("SResultsMenu Save -> SSaveMenu (0x633820 / 0x6398f0)");
        if (source == &Restart)                                    // +0x790
            SendAction(PZA_RESULTS_RESTART, 0);
        if (source == &Continue)                                   // +0x71c
            SendAction(PZA_RESULTS_CONTINUE, 0);
        if (source == &Cancel)                                     // +0x804
            SendAction(PZA_RESULTS_MAINMENU, 0);
        return true;
    }
    if (action == 0x54501 || action == 0x54502) {                  // the save menu closed
        if (SaveMenu) {
            delete SaveMenu;
            SaveMenu = nullptr;
        }
        return true;
    }
    return false;
}

// The unit table of one page (0x63e690): per player with a name (campaign
// +0x140 + p * 0xd8, SString), the name and the page's counters: total
// (+0x0c), destroyed (+0x3c), lost (+0x6c), captured (+0x9c) + page * 4,
// XP (+0xcc), and the accomplishment from the losses (+0xd0).
void SResultsMenu::ShowPage(int page)
{
    pz::SPanzersCampaign* c = pz::g_Campaign;
    for (int p = 0; p < 8; ++p) {
        const unsigned char* rec = c->PlayerStats[p];             // +0x140 + p * 0xd8
        const SString* name = (const SString*)rec;
        if (name->size == 0)
            continue;
        int* f = Grid[p];
        char buf[64];
        Board->SetText(f[0], g_PzFont[1], 2, name->buf ? name->buf : "");   // "%s"
        static const int kCol[4] = { 0x0c, 0x3c, 0x6c, 0x9c };
        for (int k = 0; k < 4; ++k) {
            _snprintf(buf, sizeof(buf) - 1, "%d", *(const int*)(rec + kCol[k] + page * 4));
            buf[sizeof(buf) - 1] = 0;
            Board->SetText(f[1 + k], g_PzFont[1], 2, buf);
        }
        _snprintf(buf, sizeof(buf) - 1, "%d", *(const int*)(rec + 0xcc));
        buf[sizeof(buf) - 1] = 0;
        Board->SetText(f[5], g_PzFont[1], 2, buf);
        int losses = *(const int*)(rec + 0xd0);
        const char* acc = losses < 2 ? "Excellent" : losses < 0xc ? "Very good" : losses < 0x1b ? "Good"
                        : losses < 0x34 ? "Avarage" : "Poor";
        Board->SetText(f[6], g_PzFont[1], 2, Mm(acc));
    }
}

static int TextFrame(int parent, int x, int y, int align, const char* text)
{
    int f = Board->CreateFrame(FT_TEXT, parent, x, y, 0, 1);      // board +0x08(2, P, x, y, 0, 1)
    Board->SetText(f, g_PzFont[1], align, text ? text : "");      // board +0x34(f, 1, align, s, 0)
    return f;
}

// PANZERS 0x6360f0
// The debriefing page (menu/debriefing_menu_hq.tga, 50 glyphs): the officer
// portrait of the nation's medal sheet, the title, name / rank / time /
// prestige / score, the objectives, the unit table with its 12 category
// pages, the buttons and the medals earned along the nation's mission chain.
// Multiplayer / scenario variants: the recompile has no multiplayer.
void SResultsMenu::Create()
{
    PZ_M3_TRACE("SResultsMenu::Create (0x6360f0)");
    pz::SPanzersCampaign* c = pz::g_Campaign;
    SProperties* props = (SProperties*)c->MissionProps;
    const char* cur = pz::SStr(c->MissionSection);
    SDXWidget::Create(0);                                          // 0x539a10
    int P = GetFrame();                                            // +0x48

    SCustomGlyph g[50];                                            // 0x8083a0.., the rest zero
    memset(g, 0, sizeof(g));
    g[0] = { 0, 0, 1024, 768 };
    g[1] = { 128, 490, 153, 35 };
    g[2] = { 1, 806, 153, 35 };
    g[3] = { 1, 770, 153, 35 };
    for (int i = 0; i < 11; ++i) {
        g[4 + 3 * i] = { 283 + 56 * i, 490, 53, 35 };
        g[5 + 3 * i] = { 155 + 54 * i, 806, 53, 35 };
        g[6 + 3 * i] = { 155 + 54 * i, 770, 53, 35 };
    }
    Background = PzLoadCustomFont("menu/debriefing_menu_hq.tga", 0x32, g);   // board +0x74
    int bg = Board->CreateFrame(FT_SPRITE, P, 0, 0, 0, 0);
    PzSetSpriteGlyph(bg, Background, 0);                          // board +0x24
    for (int i = 0; i < 12; ++i) {
        InsertChild(&Pages[i]);
        if (i == 0) {
            Pages[0].SetPosition(0x80, 0x1ea, 0, 0);
            Pages[0].Create(Background, 1, 3, 2, -1);              // 0x537a80
            Pages[0].SetChecked(true);                             // 0x537df0(1)
        } else {
            int k = i - 1;
            Pages[i].SetPosition(0x11b + 0x38 * k, 0x1ea, 0, 0);
            Pages[i].Create(Background, 4 + 3 * k, 6 + 3 * k, 5 + 3 * k, 6 + 3 * k);
        }
    }

    // The nation: medal sheet, officer.
    bool victory = c->GetMissionResult() == 1;                     // 0x5920b0
    auto missionNumber = [&](const char* m) { return props ? props->GetInt(m, "Mission number", 0) : 0; };
    bool british = c->Race == 1 && (missionNumber(cur) == 1 || missionNumber(cur) == 7);   // 0x5920f0
    int sheet;
    const char* officer;
    const char* fullName;
    const char* first;
    if (c->Race == 0) {
        sheet = PzLoadCustomFont("menu/medals_ger_hq.tga", 9, kMedalGlyphs);
        officer = "HANS"; fullName = "Hans von Gr\xfc" "bel"; first = "German 1";
    } else if (c->Race == 2) {
        sheet = PzLoadCustomFont("menu/medals_rus_hq.tga", 9, kMedalGlyphs);
        officer = "SASHA"; fullName = "Alexander Vladimiriov"; first = "Russian 1";
    } else if (british) {
        sheet = PzLoadCustomFont("menu/medals_eng_hq.tga", 3, kMedalGlyphs);
        officer = "JAMES"; fullName = "James Barnes"; first = "Allied 1";
    } else {
        sheet = PzLoadCustomFont("menu/medals_usa_hq.tga", 8, kMedalGlyphs);
        officer = "WILLSON"; fullName = "Jeffrey S. Wilson"; first = "Allied 1";
    }
    const char* name = Mm(officer);
    int portrait = Board->CreateFrame(FT_SPRITE, P, 0x53, 3, 0, 1);
    PzSetSpriteGlyph(portrait, sheet, 0);

    char line[0x400];
    if (victory)
        _snprintf(line, sizeof(line) - 1, "%s %s", Mm("CONGRATULATION"), name);
    else
        _snprintf(line, sizeof(line) - 1, "%s", Mm("DEFEAT!"));
    line[sizeof(line) - 1] = 0;
    TextFrame(P, 0x244, 0x32, 2, line);
    _snprintf(line, sizeof(line) - 1, "%s%s", Mm("Name: "), Mm(fullName));   // campaign mode only
    line[sizeof(line) - 1] = 0;
    TextFrame(P, 0x10e, 0x55, 0, line);
    {                                                              // the name goes into the player record (+0x140 SString)
        SString* rec = (SString*)c->PlayerStats[0];
        pz::FreeSString(rec);
        const char* n = Mm(fullName);
        rec->size = (int)strlen(n);
        rec->buf = new char[rec->size + 1];
        memcpy(rec->buf, n, rec->size + 1);
    }
    _snprintf(line, sizeof(line) - 1, "%s%s", Mm("Rank: "),
              props ? props->GetString(cur, "Rank", Mm("Commander")) : Mm("Commander"));
    line[sizeof(line) - 1] = 0;
    TextFrame(P, 0x10e, 0x6e, 0, line);
    int t = c->_b68;                                               // 0x591f90 (seconds)
    char tbuf[64];
    if (t / 60 >= 60)
        _snprintf(tbuf, sizeof(tbuf) - 1, "%d:%02d:%02d", t / 3600, (t / 60) % 60, t % 60);
    else
        _snprintf(tbuf, sizeof(tbuf) - 1, "%d:%02d", t / 60, t % 60);
    tbuf[sizeof(tbuf) - 1] = 0;
    _snprintf(line, sizeof(line) - 1, "%s%s", Mm("Time: "), tbuf);
    line[sizeof(line) - 1] = 0;
    TextFrame(P, 0x10e, 0x87, 0, line);
    if (victory) {
        _snprintf(line, sizeof(line) - 1, "%s%d", Mm("Prestige: "), c->GetNextMissionSP() + c->Prestige);   // 0x5928b0 + +0x38
        line[sizeof(line) - 1] = 0;
        TextFrame(P, 0x10e, 0xa0, 0, line);
    }
    int score = c->Score;                                          // 0x592a40 (+0xb60)
    _snprintf(line, sizeof(line) - 1, "%s%d", Mm("Score: "), score);
    line[sizeof(line) - 1] = 0;
    TextFrame(P, 0x10e, 0xb9, 0, line);
    InsertChild(&Upload);                                          // +0x10e8
    Upload.SetPosition(0x109, 0xcd, 0x190, 0);
    Upload.Create(1, 5, false, false, 0, false);                   // 0x542380(1, 5, 0, 0, 0, 0)
    if (score == 0)
        Upload.AddLine(Mm("this score is not uploadable"), 0xd0d0d0);
    if (c->_b84 != 0) {
        _snprintf(line, sizeof(line) - 1, "%s%s", Mm("this score is not uploadable"),
                  Mm(" (conditions: v1.10 or higher started campaign game spoil without cheatcodes)"));
        line[sizeof(line) - 1] = 0;
        Upload.AddLine(line, 0xd0d0d0);
    }

    // Objectives (counts as SStatisticMenu; HD swaps the optional and secret
    // rows, kept).
    int mainDone = 0, mainAll = 0, optDone = 0, optAll = 0, secDone = 0, secAll = 0;
    const PzStatObjective* obj = (const PzStatObjective*)c->Objectives;
    for (int i = 0; i < c->ObjectiveCount; ++i) {
        const PzStatObjective& o = obj[i];
        if (o.Excluded)
            continue;
        if (o.Main) { ++mainAll; if (o.State == 2) ++mainDone; }
        else if (o.Secret) { ++optAll; if (o.State == 2) ++optDone; }
        else { ++secAll; if (o.State == 2) ++secDone; }
    }
    TextFrame(P, 0x1e0, 0x55, 0, Mm("Objectives:"));
    const char* labels[3] = { "- Main:", "- Optional:", "- Secret:" };
    int done[3] = { mainDone, secDone, optDone }, all[3] = { mainAll, secAll, optAll };
    for (int r = 0; r < 3; ++r) {
        TextFrame(P, 0x1ea, 0x64 + 0xf * r, 0, Mm(labels[r]));
        _snprintf(line, sizeof(line) - 1, "%d/%d", done[r], all[r]);
        line[sizeof(line) - 1] = 0;
        TextFrame(P, 0x230, 0x64 + 0xf * r, 0, line);
    }

    // The unit table.
    static const int kHx[6] = { 0xff, 0x172, 0x1e5, 0x267, 0x2e4, 0x35c };
    static const char* const kHead[6] = { "Total units", "Destroyed units", "Lost units", "Captured units ", "XP", "Accomplishment" };
    for (int i = 0; i < 6; ++i)
        TextFrame(P, kHx[i], 0xf8, 2, Mm(kHead[i]));
    static const int kGx[7] = { 0xa3, 0xff, 0x172, 0x1e5, 0x267, 0x2e4, 0x35c };
    for (int r = 0; r < 8; ++r)
        for (int k = 0; k < 7; ++k)
            Grid[r][k] = TextFrame(P, kGx[k], 0x111 + 0x19 * r, 2, "");
    ShowPage(0);                                                   // 0x63e690(0)

    // The buttons (y 0x2d2).
    InsertChild(&Save);
    Save.SetPosition(0, 0x2d2, 0, 0);
    Save.Create(2, Mm("Upload Result"));
    if (score < 1 || !victory || c->_b84 != 0)
        Save.SetEnable(false);                                     // vtbl +0x70(0)
    InsertChild(&Restart);
    Restart.SetPosition(0x100, 0x2d2, 0, 0);
    Restart.Create(2, Mm("Restart"));
    if (victory) {
        InsertChild(&Continue);
        Continue.SetPosition(0x200, 0x2d2, 0, 0);
        Continue.Create(2, Mm("Continue"));
    }
    InsertChild(&Cancel);
    Cancel.SetPosition(0x300, 0x2d2, 0, 0);
    Cancel.Create(2, Mm("Cancel"));

    // The medals of the missions before this one along the chain.
    struct Pos { int x, y; };
    static const Pos kUsa[7] = { {33,545},{126,545},{231,545},{330,545},{430,545},{534,545},{636,545} };
    static const Pos kEng[2] = { {33,545},{163,545} };
    static const Pos kRus[8] = { {33,545},{138,545},{251,545},{371,545},{477,545},{588,545},{700,545},{822,545} };
    static const Pos kGer[8] = { {33,545},{140,545},{251,545},{373,545},{490,545},{604,545},{720,545},{822,545} };
    const Pos* pos = c->Race == 0 ? kGer : c->Race == 2 ? kRus : british ? kEng : kUsa;
    int maxMedals = c->Race == 0 || c->Race == 2 ? 8 : british ? 2 : 7;
    char m[260];
    strncpy(m, first, sizeof(m) - 1);
    m[sizeof(m) - 1] = 0;
    int k = 0;
    SProperties* local = (SProperties*)c->LocalProps;            // "Medal": campaign +0x04
    auto medalOf = [&](const char* mission) { return local ? local->GetString(mission, "Medal", "") : ""; };
    for (int guard = 0; guard < 64 && k < maxMedals && _stricmp(m, cur) != 0; ++guard) {
        int n = missionNumber(m);
        bool chainStart = n == 1 || n == 7;
        bool take = c->Race == 1 ? (british ? (chainStart && victory) : !chainStart) : true;
        if (c->Race == 1 && !british && chainStart) {
            ++k;                                                   // skipped, the slot advances
        } else if (take) {
            const char* medal = medalOf(m);
            if (medal && *medal) {
                SDXWidget& w = Medals[k];
                InsertChild(&w);
                w.SetPosition(pos[k].x, pos[k].y, 1, 1);
                w.SetTooltipText(medal);                           // 0x543bf0
                w.SetBackgroundSprite(sheet, k + 1, false, false); // 0x539ba0
                w.Create(0);                                       // 0x539a10
            }
            ++k;
        }
        const char* next = props ? props->GetString(m, "Next Mission", nullptr) : nullptr;
        if (!next || !*next)
            break;
        strncpy(m, next, sizeof(m) - 1);
        m[sizeof(m) - 1] = 0;
    }
    const char* medal = medalOf(cur);
    if (victory && medal && *medal) {
        if (!british)
            TextFrame(P, 0x29e, 0x55, 0, Mm("New medal:"));
        InsertChild(&NewMedal);                                    // +0x108c
        NewMedal.SetPosition(0x30a, 0x47, 0, 0);
        NewMedal.SetTooltipText(medal);
        NewMedal.SetBackgroundSprite(sheet, k + 1, false, false);
        NewMedal.Create(0);
    }
    PzReleaseCustomFont(sheet);                                    // board +0x80
    Cursor = 0;                                                    // 0x543970(0, -1)
    SetFocus();
    Logger.g->Log(0, "PZM4: results [%s] %s, prestige %d, score %d, medals %d",
                  cur, victory ? "victory" : "defeat", c->Prestige, score, k);
}
