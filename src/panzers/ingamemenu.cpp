// src/panzers/ingamemenu.cpp
// In-game menus (ingamemenu.h) and the in-game-menu part of SGameView.
// OWNER: agent H (docs/M3_INTERFACES.md).

#include "loadgame.h"
#include <windows.h>
#include <string.h>
#include "ingamemenu.h"
#include "cheats.h"
#include "gameview.h"
#include "hud.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "gettext.h"
#include "pzboard.h"
#include "stream.h"
#include "campaign.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "results.h"
#include "settings.h"

static const char* Tx(const char* id) { return GetText("panzers/InGameMenu.cpp", id); }

// ---------------------------------------------------------------------------
// SInGameMenu
// ---------------------------------------------------------------------------

// PANZERS 0x62cb50
SInGameMenu::SInGameMenu() {}

// PANZERS 0x62d670
SInGameMenu::~SInGameMenu() {}

// PANZERS 0x62f690
// The button hints (0x543bf0: "Save the game", ... "Continue game") feed
// the game view's hint box; the recompile's SComplexButton has no hint
// field yet, so they are not stored.
void SInGameMenu::Create()
{
    PZ_M3_TRACE("SInGameMenu::Create (0x62f690)");
    SRightMenu::Create(Tx("Menu"), false);                         // 0x64bdf0
    Cursor = 0;                                                    // 0x543970(0, -1)
    if (!pz::g_Campaign || !pz::g_Campaign->IsMultiMode()) {       // 0x594d20
        const char* texts[8] = {
            Tx("Save Game"), Tx("Load Game"), Tx("Options"), Tx("Help"),
            Tx("Objectives"), Tx("Restart Mission"), Tx("End Mission"), Tx("Resume"),
        };
        CreateButtons(Buttons, texts, 8, 0);                       // 0x64c210
    } else {
        const char* texts[5] = {
            Tx("Options"), Tx("Help"), Tx("Objectives"), Tx("End Mission"), Tx("Resume"),
        };
        CreateButtons(Buttons, texts, 5, 0);
    }
}

// PANZERS 0x631dd0 (single-player button order; the multiplayer order is not lifted)
bool SInGameMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    PZ_M3_TRACE("SInGameMenu::OnAction (0x631dd0)");
    if (action != PZA_BUTTON_CLICK)
        return false;
    static const int kActions[8] = {
        PZA_IGM_SAVE, PZA_IGM_LOAD, PZA_IGM_OPTIONS, PZA_IGM_HELP,
        PZA_IGM_OBJECTIVES, PZA_IGM_RESTART, PZA_IGM_END, PZA_IGM_RESUME,
    };
    for (int i = 0; i < 8; ++i) {
        if (source == &Buttons[i]) {
            SendAction(kActions[i], 0);                            // 0x543930
            return true;
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// SOptionsMenu (in-game)
// ---------------------------------------------------------------------------

// PANZERS 0x62cc40
SOptionsMenu::SOptionsMenu() {}

SOptionsMenu::~SOptionsMenu() {}

// PANZERS 0x62fc10
void SOptionsMenu::Create()
{
    PZ_M3_TRACE("SOptionsMenu::Create (0x62fc10)");
    SRightMenu::Create(GetText("panzers/InGameMenu.cpp", "Options"), false);   // 0x64bdf0
    const char* texts[4] = { Tx("Game Options"), Tx("Graphics"), Tx("Audio"), Tx("Back") };
    CreateButtons(Buttons, texts, 4, 0);                           // 0x64c210
}

// PANZERS 0x631fb0
bool SOptionsMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action != PZA_BUTTON_CLICK)
        return false;
    static const int kActions[4] = { PZA_IGO_GAME, PZA_IGO_GRAPHICS, PZA_IGO_AUDIO, PZA_IGO_BACK };
    for (int i = 0; i < 4; ++i) {
        if (source == &Buttons[i]) {
            SendAction(kActions[i], 0);                            // 0x543930
            return true;
        }
    }
    return false;
}

// ---------------------------------------------------------------------------
// SHelpMenu
// ---------------------------------------------------------------------------

// PANZERS 0x62ca60
SHelpMenu::SHelpMenu() {}

// PANZERS 0x62d5d0
SHelpMenu::~SHelpMenu() {}

// PANZERS 0x62ec90
void SHelpMenu::Create()
{
    PZ_M3_TRACE("SHelpMenu::Create (0x62ec90)");
    SCenterMenu::Create(Tx("Help"), false);                        // 0x64bc80
    // 0x64bfd0: one button at the right-hand Ok/Cancel slot.
    InsertChild(&BackButton);
    BackButton.SetPosition(0x18b, 400, 0, 0);
    BackButton.Create(1, Tx("Back"));
    InsertChild(&Text);
    Text.SetPosition(0x28, 0x32, 0x208, 0);
    Text.Create(0, 0x16, true, true, 0, false);                    // 0x542380
    char* buf = nullptr;
    unsigned int len = 0;
    FileSystem.ReadFile("help.txt", &buf, &len, "SHelpMenu::Create"); // 0x65f8e0
    if (buf) {
        buf[len] = 0;
        Text.AddLine(buf, 0xd0d0d0);                               // 0x541b60
        delete[] buf;                                              // HD free()
    } else {
        Text.AddLine("", 0xd0d0d0);
    }
    if (Text.LineCount > 0) {
        Text.TopIndex = 0;                                         // [0x4b] = 0 / Columns
        Text.Update();
    }
}

// PANZERS 0x631d40
bool SHelpMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    if (action == PZA_BUTTON_CLICK && source == &BackButton) {
        SendAction(PZA_HELP_BACK, 0);
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// SInGameBriefingMenu (the objectives screen)
// ---------------------------------------------------------------------------

// HD SPanzersCampaign +0x134: SDArray of 0x1c-byte objective records (agent
// F's campaign.h keeps them in padding): +0 state (1 failed, 2 completed),
// +4 hidden, +5 main, +6 secret, +7 victory condition, +8 SString text.
struct PzObjective {
    int           State;      // +0x00
    unsigned char Hidden;     // +0x04
    unsigned char Main;       // +0x05
    unsigned char Secret;     // +0x06
    unsigned char Victory;    // +0x07
    SString       Text;       // +0x08
    unsigned char _10[0x1c - 0x10];
};
static_assert(sizeof(PzObjective) == 0x1c, "objective record stride (0x593ba0)");

static PzObjective* Objectives(int* count)
{
    unsigned char* c = (unsigned char*)pz::g_Campaign;
    if (!c) { *count = 0; return nullptr; }
    *count = *(int*)(c + 0x138);
    return *(PzObjective**)(c + 0x134);
}

// PANZERS 0x62cad0
SInGameBriefingMenu::SInGameBriefingMenu() : FromBriefing(false) {}

// PANZERS 0x62d240
SInGameBriefingMenu::~SInGameBriefingMenu() {}

// PANZERS 0x632bd0
void SInGameBriefingMenu::AddObjective(int index)
{
    int count;
    PzObjective* o = Objectives(&count);
    if (index < 0 || index >= count)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SObjective", index);
    if (o[index].Hidden)
        return;
    const char* text = o[index].Text.buf ? o[index].Text.buf : "";
    char line[1024];
    unsigned int color;
    if (o[index].State == 1) {
        _snprintf(line, sizeof line, "%s%s", Tx("- Failed: "), text);
        color = 0xff3f3f;
    } else if (o[index].State == 2 && !o[index].Victory) {
        _snprintf(line, sizeof line, "%s%s", Tx("- Completed: "), text);
        color = 0x3fff3f;
    } else {
        // DAT_00929dec selects "-" over "- " (0x8068c0 / 0x8068c4).
        _snprintf(line, sizeof line, "%s%s", "- ", text);
        color = 0xffffff;
    }
    line[sizeof line - 1] = 0;
    Text.AddLine(line, color);
}

// PANZERS 0x62eda0 (single-player branch; the multiplayer game-mode texts
// are not lifted)
void SInGameBriefingMenu::Create()
{
    PZ_M3_TRACE("SInGameBriefingMenu::Create (0x62eda0)");
    SCenterMenu::Create(Tx("Objectives"), false);                  // 0x64bc80
    CreateOkCancel(&BriefingButton, Tx("Briefing"), &BackButton, Tx("Back")); // 0x64bf60
    pz::SPanzersCampaign* c = pz::g_Campaign;
    bool multiNotCoop = c && c->IsMultiMode();                     // 0x594d20 (&& 0x594cd0 coop: no SMulti here)
    if (!c || c->IsTutorialMode() || c->IsScenarioMode() || c->GameMode == pz::PZ_GM_2 || multiNotCoop)
        BriefingButton.SetEnable(false);                           // vtbl +0x70
    InsertChild(&Text);                                            // vtbl +0x54
    Text.SetPosition(0x14, 0x37, 0x226, 0);
    Text.Create(5, 0x11, true, false, 0, false);                   // 0x542380

    int count;
    PzObjective* o = Objectives(&count);
    int n = 0;
    for (int i = 0; i < count; ++i)
        if (!o[i].Hidden && o[i].Main && !o[i].Victory)
            ++n;
    if (n > 0) {
        Text.AddLine(Tx("Main Objectives"), 0xffff3f);
        for (int i = 0; i < count; ++i)
            if (o[i].Main && !o[i].Victory)
                AddObjective(i);
        Text.AddLine("", 0xd0d0d0);
    }
    n = 0;
    for (int i = 0; i < count; ++i)
        if (!o[i].Hidden && !o[i].Main && !o[i].Secret && !o[i].Victory)
            ++n;
    if (n > 0) {
        Text.AddLine(Tx("Optional Objectives"), 0xffff3f);
        for (int i = 0; i < count; ++i)
            if (!o[i].Main && !o[i].Secret && !o[i].Victory)
                AddObjective(i);
        Text.AddLine("", 0xd0d0d0);
    }
    int secret = 0, shown = 0;
    for (int i = 0; i < count; ++i) {
        if (o[i].Secret) {
            ++secret;
            if (!o[i].Hidden)
                ++shown;
        }
    }
    if (secret > 0) {
        char line[256];
        _snprintf(line, sizeof line, "%s (%d/%d)", Tx("Secret Objectives"), shown, secret);
        line[sizeof line - 1] = 0;
        Text.AddLine(line, 0xffff3f);
        for (int i = 0; i < count; ++i)
            if (o[i].Secret && !o[i].Victory)
                AddObjective(i);
        Text.AddLine("", 0xd0d0d0);
    }
    n = 0;
    for (int i = 0; i < count; ++i)
        if (!o[i].Hidden && o[i].Victory)
            ++n;
    if (n > 0) {
        Text.AddLine(Tx("Victory Conditions"), 0xffff3f);
        for (int i = 0; i < count; ++i)
            if (o[i].Victory)
                AddObjective(i);
    }
}

// PANZERS 0x631d70
bool SInGameBriefingMenu::OnAction(SWidget* source, int action, int param)
{
    (void)param;
    PZ_M3_TRACE("SInGameBriefingMenu::OnAction (0x631d70)");
    if (action == PZA_BUTTON_CLICK) {
        if (source == &BackButton) {
            SendAction(PZA_OBJ_BACK, FromBriefing);
            return true;
        }
        if (source == &BriefingButton) {
            SendAction(PZA_OBJ_BRIEFING, FromBriefing);
            return true;
        }
    }
    return false;
}

// SSaveMenu: savemenu.cpp.

// ---------------------------------------------------------------------------
// SGameView: the in-game menus
// ---------------------------------------------------------------------------

// HD SGameView fields by HD offset (SGameViewData starts at HD +0x5c).
template <typename T>
static T& ViewField(SGameView* v, int hdOffset)
{
    return *(T*)((unsigned char*)static_cast<SGameViewData*>(v) + (hdOffset - 0x5c));
}
static SHelpMenu*&            HelpMenu(SGameView* v)       { return ViewField<SHelpMenu*>(v, 0x3e54); }
static SInGameBriefingMenu*&  ObjectivesMenu(SGameView* v) { return ViewField<SInGameBriefingMenu*>(v, 0x3e58); }
static SSaveMenu*&            SaveMenu(SGameView* v)       { return ViewField<SSaveMenu*>(v, 0x3e4c); }
static SLoadMenu*&            LoadMenu(SGameView* v)       { return ViewField<SLoadMenu*>(v, 0x3e50); }
static SOptionsMenu*&         OptionsMenu(SGameView* v)    { return ViewField<SOptionsMenu*>(v, 0x3e5c); }
static SAudioOptionsMenu*&    AudioPage(SGameView* v)      { return ViewField<SAudioOptionsMenu*>(v, 0x3e60); }
static SGraphicsOptionsMenu*& GraphicsPage(SGameView* v)   { return ViewField<SGraphicsOptionsMenu*>(v, 0x3e64); }
static SGameOptionsMenu*&     GameOptionsPage(SGameView* v){ return ViewField<SGameOptionsMenu*>(v, 0x3e68); }
static pz::SMessageBox*&      RestartBox(SGameView* v)     { return ViewField<pz::SMessageBox*>(v, 0x3e8c); }
static pz::SMessageBox*&      EndBox(SGameView* v)         { return ViewField<pz::SMessageBox*>(v, 0x3e90); }
static unsigned char&         WasPaused(SGameView* v)      { return ViewField<unsigned char>(v, 0x3888); }

template <typename T>
static void DeleteWidget(T*& w)
{
    if (w) {
        delete w;
        w = nullptr;
    }
}

static void DeleteInGameMenu(SGameView* v)
{
    DeleteWidget(v->InGameMenu);                                   // HD +0x3e48
}

// The pause part of 0x620080 / 0x61ffb0: remember whether the game was
// running and stop it (single player only: DAT_008f1a74 == 0).
static void PauseForMenu(SGameView* v)
{
    if (pz::g_GameLogic) {
        WasPaused(v) = pz::g_GameLogic->Running == 0;              // 0x56d190
        pz::g_GameLogic->SetRunning(0);                            // 0x5802f0
    }
}

// LAB_00621843: run again unless the mission has ended (campaign +0xe4, 0x5920b0).
static void ResumeAfterMenu()
{
    if (pz::g_GameLogic && (!pz::g_Campaign || pz::g_Campaign->GetMissionResult() == 0))
        pz::g_GameLogic->SetRunning(1);                            // 0x5802f0(1)
}

// PANZERS 0x620080
void PzOpenInGameMenu(SGameView* v)
{
    PZ_M3_TRACE("SGameView::OpenInGameMenu (0x620080)");
    PauseForMenu(v);
    SInGameMenu* m = new SInGameMenu();                            // new 0x3f8, 0x62cb50
    v->InGameMenu = m;                                             // +0x3e48
    v->InsertChild(m);                                             // vtbl +0x54
    m->Create();                                                   // 0x62f690
}

// PANZERS 0x6205f0
void PzOpenSaveMenu(SGameView* v)
{
    PZ_M3_TRACE("SGameView::OpenSaveMenu (0x6205f0)");
    SSaveMenu* m = new SSaveMenu();                                // new 0x474, 0x62ccb0
    SaveMenu(v) = m;                                               // +0x3e4c
    v->InsertChild(m);                                             // vtbl +0x54
    m->Create();                                                   // 0x62fcb0
}

// PANZERS 0x620140
void PzOpenLoadMenu(SGameView* v)
{
    PZ_M3_TRACE("SGameView::OpenLoadMenu (0x620140)");
    SLoadMenu* m = new SLoadMenu();                                // new 0x280, 0x62cbc0
    LoadMenu(v) = m;                                               // +0x3e50
    v->InsertChild(m);                                             // vtbl +0x54
    m->Create(false);                                              // 0x62f950(0): Load + Back
}

// PANZERS 0x620570
void PzOpenOptionsMenu(SGameView* v)
{
    PZ_M3_TRACE("SGameView::OpenOptionsMenu (0x620570)");
    SOptionsMenu* m = new SOptionsMenu();                          // new 0x228, 0x62cc40
    OptionsMenu(v) = m;                                            // +0x3e5c
    v->InsertChild(m);                                             // vtbl +0x54
    m->Create();                                                   // 0x62fc10
}

// PANZERS 0x61f7c0
static void PzOpenAudioPage(SGameView* v)
{
    SAudioOptionsMenu* m = new SAudioOptionsMenu();                // new 0x4b0, 0x62c740
    AudioPage(v) = m;                                              // +0x3e60
    v->InsertChild(m);
    m->Create(false);                                              // 0x62d800(0): Ok / Back
}

// PANZERS 0x61feb0
static void PzOpenGraphicsPage(SGameView* v)
{
    SGraphicsOptionsMenu* m = new SGraphicsOptionsMenu();          // new 0xa10, 0x62c900
    GraphicsPage(v) = m;                                           // +0x3e64
    v->InsertChild(m);
    m->Create(false);                                              // 0x62e280(0)
}

// PANZERS 0x61fe30
static void PzOpenGameOptionsPage(SGameView* v)
{
    SGameOptionsMenu* m = new SGameOptionsMenu();                  // new 0x6f4, 0x62c7f0
    GameOptionsPage(v) = m;                                        // +0x3e68
    v->InsertChild(m);
    m->Create(false);                                              // 0x62dbf0(0)
}

// A page closed with its changes kept (Game Ok, or Esc on a page): HD
// re-registers the key hint texts (0x626290, not lifted: no hint registry
// yet), takes the unit voice level into World+0x73ac (0x64e2b0) and opens
// the Options menu again (0x620570).
static void PzOptionsPageDone(SGameView* v)
{
    if (v->World)
        *(int*)((unsigned char*)v->World + 0x73ac) = Settings.UnitVoice;   // 0x64e2b0
    PzOpenOptionsMenu(v);
}

// PANZERS 0x627db0
// The mission's briefing picture (menu/briefing/<map>_hq.tga, ger-01 when
// it is missing) as a full-screen button over the hidden panels; the game
// stops until it is clicked (0x619520).
void PzShowBriefing(SGameView* v)
{
    PZ_M3_TRACE("SGameView::ShowBriefing (0x627db0)");
    v->SetPanelMode(1);                                            // 0x625d80(1)
    if (v->SoundHandle3894 >= 0) {
        PzReleaseTexture(v->SoundHandle3894);                      // board +0x80
        v->SoundHandle3894 = -1;
    }
    const char* map = pz::g_Campaign ? pz::g_Campaign->GetMapName() : nullptr;   // 0x592040
    if (!map)
        map = "";
    const char* base = map;                                        // 0x5625a0: no directory
    for (const char* p = map; *p; ++p)
        if (*p == '/' || *p == '\\')
            base = p + 1;
    char name[260];
    strncpy(name, base, sizeof(name) - 1);
    name[sizeof(name) - 1] = 0;
    if (char* dot = strrchr(name, '.'))                            // 0x5651f0: no extension
        *dot = 0;
    char file[300];
    _snprintf(file, sizeof(file) - 1, "menu/briefing/%s_hq.tga", name);   // 0x52da80
    file[sizeof(file) - 1] = 0;
    const char* pick = FileSystem.Stat(file, nullptr) == 0 ? file : "menu/briefing/ger-01_hq.tga";   // 0x65faf0
    v->SoundHandle3894 = PzLoadTexture(pick);                      // board +0x7c
    pz::SButton* b = new pz::SButton();                            // new 0x74, 0x537a20
    v->Modal = b;                                                  // +0x389c
    v->InsertChild(b);                                             // vtbl +0x54
    b->SetPosition(0, 0, 0x400, 0x300);                            // vtbl +0x08
    b->Create(v->SoundHandle3894, 0, 0, -1, -1);                   // 0x537a80
    b->SetFocus();                                                 // 0x5439f0
    PzViewSetCursor(v, -1, 0xffffffffu);                           // 0x543970(-1, -1)
    if (v->Logic)
        v->Logic->SetRunning(0);                                   // +0x3e44, 0x5802f0(0)
}

// PANZERS 0x619520
// Closes the briefing picture: the panels come back (0x625d80(0)), then the
// objectives menu it was opened from, or the game runs again.
void PzCloseModal(SGameView* v)
{
    if (v->Modal) {
        delete v->Modal;
        v->Modal = nullptr;
    }
    v->SetPanelMode(0);                                            // 0x625d80(0)
    if (ObjectivesMenu(v)) {
        ObjectivesMenu(v)->SetVisible(true);                       // +0x3e58 vtbl +0x6c(1)
        return;
    }
    if (v->Logic && (!pz::g_Campaign || pz::g_Campaign->GetMissionResult() == 0))   // 0x5920b0
        v->Logic->SetRunning(1);
}

// The start of SGameView's load-game LoadMap 0x61f840: the open dialogs
// go (the load menu, the in-game menu, help, objectives, save, the
// statistics; the options pages +0x3e60..+0x3e68 and +0x3e5c are not lifted).
void PzGameViewCloseDialogs(SGameView* v)
{
    DeleteWidget(LoadMenu(v));                                     // +0x3e50
    DeleteInGameMenu(v);                                           // +0x3e48
    DeleteWidget(HelpMenu(v));                                     // +0x3e54
    DeleteWidget(ObjectivesMenu(v));                               // +0x3e58
    DeleteWidget(SaveMenu(v));                                     // +0x3e4c
    DeleteWidget(OptionsMenu(v));                                  // +0x3e5c
    DeleteWidget(AudioPage(v));                                    // +0x3e60
    DeleteWidget(GraphicsPage(v));                                 // +0x3e64
    DeleteWidget(GameOptionsPage(v));                              // +0x3e68
    PzDeleteStatisticMenu(v);                                      // +0x3e6c
}

// PANZERS 0x622f50 (case VK_ESCAPE, the dialog part; the chat line, the
// +0x38dd overlay and the options pages +0x3e60..+0x3e68 are not lifted)
void PzGameViewEscape(SGameView* v)
{
    if (v->InGameMenu) {                                           // +0x3e48
        DeleteInGameMenu(v);
        if (!WasPaused(v))                                         // LAB_0062456f
            ResumeAfterMenu();
        return;
    }
    if (AudioPage(v) || GraphicsPage(v) || GameOptionsPage(v)) {   // an options page: back to Options
        DeleteWidget(AudioPage(v));                                // +0x3e60
        DeleteWidget(GraphicsPage(v));                             // +0x3e64
        DeleteWidget(GameOptionsPage(v));                          // +0x3e68
        PzOptionsPageDone(v);
        return;
    }
    bool open = !(ObjectivesMenu(v) && ObjectivesMenu(v)->FromBriefing);   // +0x280
    DeleteWidget(OptionsMenu(v));                                  // +0x3e5c
    DeleteWidget(HelpMenu(v));                                     // +0x3e54
    DeleteWidget(ObjectivesMenu(v));                               // +0x3e58
    DeleteWidget(SaveMenu(v));                                     // +0x3e4c
    DeleteWidget(LoadMenu(v));                                     // +0x3e50
    if (open) {
        PzOpenInGameMenu(v);                                       // 0x620080
        return;
    }
    if (!WasPaused(v))
        ResumeAfterMenu();
}

// PANZERS 0x61ff30
void PzOpenHelpMenu(SGameView* v)
{
    PZ_M3_TRACE("SGameView::OpenHelpMenu (0x61ff30)");
    SHelpMenu* m = new SHelpMenu();                                // new 0x20c, 0x62ca60
    HelpMenu(v) = m;                                               // +0x3e54
    v->InsertChild(m);
    m->Create();                                                   // 0x62ec90
}

// PANZERS 0x61ffb0
void PzOpenObjectivesMenu(SGameView* v, bool fromBriefing)
{
    PZ_M3_TRACE("SGameView::OpenObjectivesMenu (0x61ffb0)");
    PauseForMenu(v);
    SInGameBriefingMenu* m = new SInGameBriefingMenu();            // new 0x284, 0x62cad0
    ObjectivesMenu(v) = m;                                         // +0x3e58
    v->InsertChild(m);
    m->Create();                                                   // 0x62eda0
    m->FromBriefing = fromBriefing;                                // +0x280
}

// PANZERS 0x619580: leave a pending mouse mode (1 = box select: 0x5ddb60).
static void ResetMouseMode(SGameView* v)
{
    if (v->MouseMode == 1)
        v->World->HideSelectionBox();                              // +0x3e40, 0x5ddb60
    v->MouseMode = 0;                                              // +0x478
}

// The "End Mission" / "Restart Mission" boxes of 0x6216b0.
static pz::SMessageBox* AskSure(SGameView* v, const char* title)
{
    pz::SMessageBox* b = new pz::SMessageBox();                    // new 0x3ec, 0x53e0d0
    v->InsertChild(b);                                             // vtbl +0x54
    b->Create(GetText("panzers/GameView.cpp", title), "", PZ_MB_YESNO, true); // 0x53e3b0(t, "", 4, 1)
    b->SetText(GetText("panzers/GameView.cpp", "Are you sure?"), 0xd0d0d0); // 0x53e910
    b->SetTarget(v);                                               // 0x53e8d0
    b->SetVisible(true);                                           // vtbl +0x6c
    return b;
}

void PzGameViewDeleteMenus(SGameView* v)
{
    DeleteInGameMenu(v);
    DeleteWidget(HelpMenu(v));
    DeleteWidget(ObjectivesMenu(v));
    DeleteWidget(SaveMenu(v));
    DeleteWidget(LoadMenu(v));
    DeleteWidget(OptionsMenu(v));
    DeleteWidget(AudioPage(v));
    DeleteWidget(GraphicsPage(v));
    DeleteWidget(GameOptionsPage(v));
    DeleteWidget(RestartBox(v));
    DeleteWidget(EndBox(v));
    PzDeleteStatisticMenu(v);                                      // +0x3e6c (results.cpp)
}

// PANZERS 0x6216b0 (the in-game-menu cases; the rest of OnAction is agent V's / O's)
bool PzGameViewMenuAction(SGameView* v, SWidget* source, int action, int param)
{
    if (action == PZA_BUTTON_DOWN && v->Modal) {                   // 0x42541 with +0x389c: the picture clicked
        PzCloseModal(v);                                           // 0x619520
        return true;
    }
    if (PzHudAction(v, source, action, param))                     // the HUD buttons (hud.cpp)
        return true;
    if (source && source == RestartBox(v)) {
        DeleteWidget(RestartBox(v));
        if (action == PZA_MSGBOX_YES) {
            v->SendAction(0x47565, 0);                             // GV_RESTART
            PzCampaignSetCheated(pz::g_Campaign, 0);               // 0x597180(0) (cheats.cpp)
        }
        return true;
    }
    if (source && source == EndBox(v)) {
        DeleteWidget(EndBox(v));
        if (action == PZA_MSGBOX_YES) {
            DeleteInGameMenu(v);
            if (v->Logic && pz::g_Campaign)
                v->Logic->BackupCampaignUnits();                   // 0x561110
            v->SendAction(0x47562, 0);                             // GV_GAMEOVER (LAB_00622b00)
        }
        return true;
    }
    switch (action) {
    case PZA_IGM_SAVE:
        DeleteInGameMenu(v);
        PzOpenSaveMenu(v);                                         // 0x6205f0
        return true;
    case PZA_SAVE_DONE:                                            // 0x49531 (saved)
        DeleteWidget(SaveMenu(v));
        if (!WasPaused(v))
            ResumeAfterMenu();                                     // LAB_00621836
        return true;
    case PZA_SAVE_BACK:                                            // 0x49533
        DeleteWidget(SaveMenu(v));
        PzOpenInGameMenu(v);                                       // 0x620080
        return true;
    case PZA_IGM_LOAD:
        DeleteInGameMenu(v);
        PzOpenLoadMenu(v);                                         // 0x620140
        return true;
    case 0x494c3:                                                  // SLoadMenu Back
        DeleteWidget(LoadMenu(v));
        PzOpenInGameMenu(v);                                       // 0x620080
        return true;
    case PZA_IGM_OPTIONS:
        DeleteInGameMenu(v);
        PzOpenOptionsMenu(v);                                      // 0x620570
        return true;
    case PZA_IGO_AUDIO:
        DeleteWidget(OptionsMenu(v));                              // +0x3e5c
        PzOpenAudioPage(v);                                        // 0x61f7c0
        return true;
    case PZA_IGO_GRAPHICS:
        DeleteWidget(OptionsMenu(v));
        PzOpenGraphicsPage(v);                                     // 0x61feb0
        return true;
    case PZA_IGO_GAME:
        DeleteWidget(OptionsMenu(v));
        PzOpenGameOptionsPage(v);                                  // 0x61fe30
        return true;
    case PZA_IGO_BACK:
        DeleteWidget(OptionsMenu(v));
        PzOpenInGameMenu(v);                                       // 0x620080
        return true;
    case PZA_AUDIO_OK:                                             // 0x4f411
    case PZA_AUDIO_BACK:                                           // 0x4f412
        DeleteWidget(AudioPage(v));                                // +0x3e60
        PzOpenOptionsMenu(v);
        return true;
    case PZA_GRAPHICS_OK:                                          // 0x4f561
    case PZA_GRAPHICS_CANCEL:                                      // 0x4f562
        DeleteWidget(GraphicsPage(v));                             // +0x3e64
        PzOpenOptionsMenu(v);
        return true;
    case PZA_GAME_OK2:                                             // 0x4f472: the fog view at once
        if (pz::g_GameLogic)
            pz::g_GameLogic->SetVisOverlayMode(Settings.FogOfWarView);   // 0x57f970(0x64df00())
        return true;
    case PZA_GAME_OK:                                              // 0x4f471
        DeleteWidget(GameOptionsPage(v));                          // +0x3e68
        PzOptionsPageDone(v);
        return true;
    case PZA_GAME_BACK:                                            // 0x4f473
        DeleteWidget(GameOptionsPage(v));
        PzOpenOptionsMenu(v);
        return true;
    case PZA_IGM_HELP:
        DeleteInGameMenu(v);
        PzOpenHelpMenu(v);                                         // 0x61ff30
        return true;
    case PZA_IGM_OBJECTIVES:
        DeleteInGameMenu(v);
        PzOpenObjectivesMenu(v, false);                            // 0x61ffb0(0)
        return true;
    case PZA_IGM_RESTART:
        DeleteInGameMenu(v);
        ResetMouseMode(v);                                         // 0x619580
        RestartBox(v) = AskSure(v, "Restart Mission");
        return true;
    case PZA_IGM_END:
        DeleteInGameMenu(v);
        ResetMouseMode(v);
        EndBox(v) = AskSure(v, "End Mission");
        return true;
    case PZA_IGM_RESUME:
        if (!WasPaused(v))
            ResumeAfterMenu();
        DeleteInGameMenu(v);
        return true;
    case PZA_HELP_BACK:
        DeleteWidget(HelpMenu(v));
        PzOpenInGameMenu(v);                                       // 0x620080
        return true;
    case PZA_OBJ_BACK:
        DeleteWidget(ObjectivesMenu(v));
        if (param == 0)
            PzOpenInGameMenu(v);
        else if (!WasPaused(v))
            ResumeAfterMenu();
        return true;
    case PZA_OBJ_BRIEFING:
        if (ObjectivesMenu(v))
            ObjectivesMenu(v)->SetVisible(false);                  // vtbl +0x6c(0)
        PzShowBriefing(v);                                         // 0x627db0
        return true;
    default:
        return false;
    }
}
