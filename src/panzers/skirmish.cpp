// src/panzers/skirmish.cpp
// The single-player Skirmish staging room (SSkirmishChatRoomMenu) and the
// offline SMulti subset it runs on (PzSkirmish); see skirmish.h and
// docs/m5/sk.md. Lifted from the HD exe. OWNER: agent SK (M5).

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <direct.h>
#include "skirmish.h"
#include "superwindow.h"
#include "pzboard.h"
#include "board.h"
#include "gettext.h"
#include "logger.h"
#include "stub_log.h"
#include "m3common.h"
#include "stream.h"
#include "darray.h"
#include "core_common.h"
#include "settings.h"
#include "hdbitmap.h"
#include "igepardhd.h"


static const char* Sk(const char* id) { return GetText("panzers/SkirmishChatRoom.cpp", id); }
static const char* Str(const SString& s) { return s.buf ? s.buf : ""; }
static void FreeSString(SString* s) { delete[] s->buf; s->buf = nullptr; s->size = 0; }

// SComplexButton::SetText (0x538a10).
static void SetButtonText(SComplexButton* b, const char* text)
{
    b->Text = text ? text : "";
    Board->SetText(b->TextFrame, g_PzFont[PZF_SANS21_SHADOW], 2, Str(b->Text));
    b->Update();
}

// Board +0x08(2, parent, x, y, 0, 1) + 0x34(frame, font, align, text).
static int TextFrame(int parent, int x, int y, int font, int align, const char* text, bool visible)
{
    int f = Board->CreateFrame(FT_TEXT, parent, x, y, 0, visible ? 1 : 0);
    Board->SetText(f, g_PzFont[font], align, text ? text : "");
    return f;
}

// ===========================================================================
// PzSkirmish: the SMulti data of the room
// ===========================================================================

// PANZERS 0x51dcf0 (the slot / settings part) + SMulti::Server 0x51e950
// The server socket (0x535cc0) is not opened offline. Server: this host is
// slot 0, connected, status 2 (+0x4c70 word 0x200), not ready, with the
// player name (0x64e070, Settings "Player Name").
PzSkirmish::PzSkirmish()
{
    memset(Slots, 0, sizeof(Slots));
    memset(Armies, 0, sizeof(Armies));
    GameType = 0;
    PrestigeLimit = 0;
    GameAge = 0;
    Locked = false;
    ArmiesChanged = false;
    OnePlayer = false;
    MapName[0] = 0;
    MapPath[0] = 0;
    MapCrc = 0;
    Started = false;
    Skirmish = false;
    for (int i = 0; i < 8; ++i)
        Slots[i].Team = i < 4 ? 1 : 2;
    MySlot = 0;                                                    // +0x4790 = +0x4794 = 0
    Slots[0].Connected = true;                                     // +0x4c5c
    const char* name = Settings.PlayerName.buf ? Settings.PlayerName.buf : "";
    strncpy(Slots[0].Name, name, 0x32);                            // 0x64e070
    Slots[0].Name[0x31] = 0;
    Slots[0].Status = 2;                                           // +0x4c70 = 0x200
    Slots[0].Ready = 0;
    Logger.g->Log(1, "SMulti::Server : ready");
}

PzSkirmish::~PzSkirmish()
{
    for (int i = 0; i < 8; ++i)
        pz::ArmyFree(&Armies[i]);                                  // 0x51de90
}

// PANZERS 0x51e2b0
// Toggle the local player's ready flag; reset = every slot not ready. The
// "slot info" message to the other hosts (0x521a70, 0x5211b0) has no
// receiver offline.
void PzSkirmish::SetReady(bool reset)
{
    PzSkirmishSlot& me = Slots[MySlot];
    me.Ready = me.Ready == 0;
    if (reset)
        for (int i = 0; i < 8; ++i)
            Slots[i].Ready = 0;
    me.Changed = 1;                                                // +0x4cb4
}

// PANZERS 0x51e270
void PzSkirmish::SetNotReady(bool market)
{
    Slots[MySlot].Ready = market ? 2 : 0;
}

// PANZERS 0x51e3a0
void PzSkirmish::SetNation(int slot, int nation)
{
    Slots[slot].Nation = nation;
}

// PANZERS 0x51e3c0 (the server branch: 0x51ed30, 0x5222d0)
// Only when the local player is not ready: move to the first free slot
// (not connected, Open) of the other team, searching from the slot after
// the current one; then the team numbers 1 (slots 0..3) and 2 (4..7).
void PzSkirmish::ChangeTeam(int team)
{
    if (Slots[MySlot].Ready != 0)
        return;
    int me = MySlot;
    int lo = team ? 4 : 0, hi = lo + 3;
    int s = (me >= lo && me <= hi) ? (me == hi ? lo : me + 1) : lo;
    int count = (me >= lo && me <= hi) ? 3 : 4;
    for (int i = 0; i < count; ++i) {
        if (!Slots[s].Connected && Slots[s].Status == 0) {         // 0x5222d0(me, s): swap the two slots
            PzSkirmishSlot t = Slots[s];
            Slots[s] = Slots[me];
            Slots[me] = t;
            pz::SArmyArray a = Armies[s];
            Armies[s] = Armies[me];
            Armies[me] = a;
            MySlot = s;
            break;
        }
        if (++s > hi)
            s = lo;
    }
    for (int i = 0; i < 8; ++i)
        Slots[i].Team = i < 4 ? 1 : 2;
}

int PzSkirmish::HostCount() const
{
    int n = 0;
    for (int i = 0; i < 8; ++i)
        n += Slots[i].Connected ? 1 : 0;
    return n;
}

// PANZERS 0x521d60
// The "start" message to every other host (0x5374f0) has no receiver offline.
void PzSkirmish::Start()
{
    if (HostCount() < 2)
        OnePlayer = true;                                          // +0x4a1c
    Started = true;                                                // +0x4774
    Logger.g->Log(1, "START GAME!");
}

// ===========================================================================
// SSkirmishChatRoomMenu
// ===========================================================================

// PANZERS 0x652d20
SSkirmishChatRoomMenu::SSkirmishChatRoomMenu()
{
    SelArmy = -1;                                                  // +0x1240
    Race = 0;                                                      // +0x123c
    ArmiesEnabled = true;                                          // +0x538
    CountdownTimer = -1;                                           // +0x524
    MeReady = false;                                               // +0x52c
    NoMapsBox = nullptr;                                           // +0x530
    DeleteBox = nullptr;                                           // +0x534
    Countdown = -1;                                                // +0x528
    MiniMapFrame = -1;                                             // +0x1f4
    MiniMapFont = CompassFont = -1;
    PrestigeText = GameTypeText = GameAgeText = -1;
    MapLabel = MapNameText = -1;
    for (int i = 0; i < 8; ++i)
        ReadyFrames[i] = -1;
    ShownMap[0] = 0;
}

// PANZERS 0x652ef0
SSkirmishChatRoomMenu::~SSkirmishChatRoomMenu()
{
    KillTimer(&CountdownTimer);                                    // 0x543690(+0x524)
    ReleaseMiniMap();
    if (DeleteBox) {
        delete DeleteBox;
        DeleteBox = nullptr;
    }
    if (NoMapsBox) {
        delete NoMapsBox;
        NoMapsBox = nullptr;
    }
}

// PANZERS 0x655a50
bool SSkirmishChatRoomMenu::OnKeyDown(int key, bool repeat)
{
    (void)key; (void)repeat;
    return true;
}

// PANZERS 0x656110
void SSkirmishChatRoomMenu::SetVisible(bool visible)
{
    SDXWidget::SetVisible(visible);                                // 0x539c40
}

// PANZERS 0x655a60
// The start countdown: the first tick after Start Game kills the timer and
// sends 0x534b1 (the counter starts at 4, so HD starts after one second).
void SSkirmishChatRoomMenu::OnTimer(int id, unsigned int elapsed)
{
    (void)elapsed;
    if (id != CountdownTimer)
        return;
    if (--Countdown >= 0) {
        KillTimer(&CountdownTimer);                                // 0x543690
        SendAction(PZA_SKIRMISH_START, 0);                         // 0x543930(0x534b1, 0)
    }
}

// PANZERS 0x653250
void SSkirmishChatRoomMenu::Create()
{
    PZ_M3_TRACE("SSkirmishChatRoomMenu::Create (0x653250)");
    SFullScreenMenu::Create(Sk("Skirmish"));                       // 0x64bd50
    // 0x64c010: the four bottom buttons, x 0 / 0x100 / 0x200 / 0x300, y 0x2d2, style 2.
    SComplexButton* bottom[4] = { &StartGame, &ImReady, &LockSettings, &Cancel };
    const char* bottomText[4] = { Sk("Start Game"), Sk("I'm Ready"), Sk("Lock Settings"), Sk("Cancel") };
    for (int i = 0; i < 4; ++i) {
        InsertChild(bottom[i]);                                    // vtbl +0x54
        bottom[i]->SetPosition(i * 0x100, 0x2d2, 0, 0);            // vtbl +0x08
        bottom[i]->Create(2, bottomText[i]);                       // 0x5386c0(2, text)
    }
    Logger.g->Log(1, "SSkirmishChatRoomMenu::Create() - Beginning of Create().");
    delete pz::g_Campaign;                                         // 0x591350 + delete 0xb8c
    pz::g_Campaign = new pz::SPanzersCampaign();                   // new 0xb8c, 0x590ec0
    pz::g_Campaign->InitMultiMode();                               // 0x593b70
    InsertChild(&Team1);
    Team1.SetPosition(0x22, 0x25, 0, 0);
    Team1.Create(0, Sk("Team 1"));                                 // 0x5386c0(0, text)
    InsertChild(&Team2);
    Team2.SetPosition(0x22, 0xf4, 0, 0);
    Team2.Create(0, Sk("Team 2"));
    SDXWidget* panels[2] = { &Panel1, &Panel2 };
    for (int p = 0; p < 2; ++p) {
        InsertChild(panels[p]);
        panels[p]->SetPosition(0x1c, p ? 0x115 : 0x46, 0x1ce, 0xac);
        panels[p]->SDXWidget::Create(0);                           // 0x539a10
        PzDrawFrameBox(panels[p]);                                 // 0x543d90
    }
    for (int p = 0; p < 2; ++p) {                                  // the column titles, font 2, centred
        int pf = panels[p]->GetFrame();                            // vtbl +0x68
        TextFrame(pf, 0x5d, 6, PZF_SANS21, 2, Sk("Name"), true);
        TextFrame(pf, 0x10c, 6, PZF_SANS21, 2, Sk("Nation"), true);
        TextFrame(pf, 0x196, 6, PZF_SANS21, 2, Sk("Status"), true);
    }
    for (int i = 0; i < 8; ++i) {
        SDXWidget* panel = i < 4 ? &Panel1 : &Panel2;
        int y = (i & 3) * 0x21;
        ReadyFrames[i] = TextFrame(panel->GetFrame(), 0x164, y + 0x22, PZF_SANS21, 0, Sk("Ready"), true);
        Board->SetTextColor(ReadyFrames[i], 0xff00);               // board +0x28
        Board->ShowFrame(ReadyFrames[i], false);                   // board +0x18(f, 0)
        panel->InsertChild(&Status[i]);
        Status[i].SetPosition(6, y + 0x20, 0, 0);
        Status[i].AddItem("", 0);                                  // 0x538dd0
        Status[i].SetCurSel(0);                                    // 0x5396f0
        Status[i].Create();                                        // 0x538f20
        panel->InsertChild(&Nations[i]);
        Nations[i].SetPosition(0xb5, y + 0x20, 0, 0);
        Nations[i].AddItem(Sk("German"), 0);
        Nations[i].AddItem(Sk("Allied"), 0);
        Nations[i].AddItem(Sk("Russian"), 0);
        Nations[i].SetCurSel(Race);                                // +0x123c
        Nations[i].Create();
    }
    TextFrame(BackFrame, 0x218, 0x2c, PZF_SANS21_SHADOW, 0, Sk("Armies"), true);
    InsertChild(&Armies);
    Armies.SetPosition(0x212, 0x46, 0x1c5, 0x60);
    Armies.Create(2, 4, true, true, 0, true);                      // 0x53c260(2, 4, 1, 1, 0, 1)
    InsertChild(&NewArmy);
    NewArmy.SetPosition(0x212, 0xb4, 0, 0);
    NewArmy.Create(0, Sk("New"));
    InsertChild(&EditArmy);
    EditArmy.SetPosition(0x2a9, 0xb4, 0, 0);
    EditArmy.Create(0, Sk("Edit"));
    InsertChild(&DeleteArmy);
    DeleteArmy.SetPosition(0x340, 0xb4, 0, 0);
    DeleteArmy.Create(0, Sk("Delete"));
    TextFrame(BackFrame, 0x218, 0x11e, PZF_SANS21_SHADOW, 0, Sk("Game Settings"), true);
    PrestigeText = TextFrame(BackFrame, 0x218, 0x14e, PZF_SANS21_SHADOW, 0, Sk("Prestige limit"), false);
    InsertChild(&Prestige);
    Prestige.SetPosition(0x29e, 0x149, 0, 0);
    FillPrestigeList(0);                                           // inline: 0x5396a0, 1500 / 2000 / 2500, sel 1
    Prestige.Create();
    int sel = Prestige.GetCurSel();
    g_Skirmish->PrestigeLimit = atoi(sel >= 0 && sel < Prestige.ItemCount ? Str(Prestige.Items[sel].Text) : "");   // +0x4a34
    GameTypeText = TextFrame(BackFrame, 0x218, 0x16f, PZF_SANS21_SHADOW, 0, Sk("Game type"), false);
    InsertChild(&GameType);
    GameType.SetPosition(0x29e, 0x16a, 0, 0);
    GameType.AddItem(Sk("Team Match"), 0);
    GameType.AddItem(Sk("Domination"), 0);
    GameType.AddItem(Sk("Assault"), 0);
    GameType.SetCurSel(0);
    GameType.Create();
    GameAgeText = TextFrame(BackFrame, 0x218, 400, PZF_SANS21_SHADOW, 0, Sk("Game age"), false);
    InsertChild(&GameAge);
    GameAge.SetPosition(0x29e, 0x18b, 0, 0);
    GameAge.AddItem(Sk("Early"), 0);
    GameAge.AddItem(Sk("Late"), 0);
    GameAge.SetCurSel(0);
    GameAge.Create();
    TextFrame(BackFrame, 0x218, 0x1c9, PZF_SANS21_SHADOW, 0, Sk("Select map"), true);
    InsertChild(&Maps);
    Maps.SetPosition(0x212, 0x1e2, 0xfa, 0);
    Maps.Create(0, 0xc, true, true, 0, true);                      // 0x53c260(0, 0xc, 1, 1, 0, 1)
    if (Maps.ItemCount > 0)
        Maps.SetCurSel(-1);                                        // +0x6e0 = -1, vtbl +0x78
    FillMapList();                                                 // 0x655ab0
    MapLabel = TextFrame(BackFrame, 0x218, 0x2a3, PZF_SANS21_SHADOW, 0, Sk("Map:"), false);
    int tw = 0, th = 0;
    const char* ml = Sk("Map:");
    Board->GetTextExtent(g_PzFont[PZF_SANS21_SHADOW], ml, (int)strlen(ml), &tw, &th, 1.0f);   // board +0x88(3, ...)
    MapNameText = TextFrame(BackFrame, tw + 0x21d, 0x2a3, PZF_SANS21_SHADOW, 0, "", false);
    Cursor = 0;                                                    // 0x543970(0, -1)
    if (Maps.ItemCount < 1) {
        NoMapsBox = new pz::SMessageBox();                         // new 0x3ec, 0x53e0d0
        InsertChild(NoMapsBox);
        NoMapsBox->Create("", "", 0, true);                        // 0x53e3b0("", "", 0, 1)
        NoMapsBox->SetTarget(this);                                // 0x53e8d0
        NoMapsBox->SetText(Sk("You have no map files. You cannot create a game, but you can join one."), 0xd0d0d0);
        NoMapsBox->SetVisible(true);                               // vtbl +0x6c(1)
        return;
    }
    Logger.g->Log(1, "SSkirmishChatRoomMenu::Create() - End of Create().");
    g_Skirmish->Slots[4].Status = 3;                               // SMulti +0x4de1 = 3: slot 4 is a Computer
    EnableControls(true, true);                                    // 0x656120(1, 1)
    g_Skirmish->Skirmish = true;                                   // +0x512c
}

// PANZERS 0x656120
void SSkirmishChatRoomMenu::EnableControls(bool enable, bool buttons)
{
    for (int i = 0; i < 8; ++i) {
        Nations[i].SetVisible(enable);                             // vtbl +0x6c
        Status[i].SetVisible(enable);
    }
    Prestige.SetVisible(enable);
    GameType.SetVisible(enable);
    GameAge.SetVisible(enable);
    if (buttons) {
        StartGame.SetVisible(enable);
        LockSettings.SetVisible(enable);
    }
    ImReady.SetVisible(enable);
    NewArmy.SetVisible(enable);
    EditArmy.SetVisible(enable);
    DeleteArmy.SetVisible(enable);
}

// PANZERS 0x6560a0
void SSkirmishChatRoomMenu::FillPrestigeList(int age)
{
    Prestige.ResetContent();                                       // 0x5396a0
    if (age == 0) {
        Prestige.AddItem("1500", 0);
        Prestige.AddItem("2000", 0);
        Prestige.AddItem("2500", 0);
    } else {
        Prestige.AddItem("2000", 0);
        Prestige.AddItem("3000", 0);
        Prestige.AddItem("4000", 0);
    }
    Prestige.SetCurSel(1);
}

// PANZERS 0x653ee0
// A map of the list needs a MINI chunk (the preview).
bool SSkirmishChatRoomMenu::IsMap(const char* path)
{
    SStream* s = FileSystem.OpenRead(path, nullptr);               // 0x65f420(path, 0)
    if (!s) {
        Logger.g->Log(1, "SChatRoomMenu::LoadMiniMap -> Can't open %s map file.", path);
        return false;
    }
    bool mini = false;
    try {
        s->ReadSignature();                                        // 0x65d6a0
        if (s->ReadChunkHeader() != 0x4650414d)                    // MAPF
            throw "Not a map file";
        if (s->ReadInt() != 0x31303276)                            // v201
            throw "Unsupported map file version";
        while (!s->ReadChunkIsEnd()) {                             // 0x65d3b0
            if (s->ReadChunkHeader() == 0x494e494d)                // MINI
                mini = true;
            s->ReadChunkSkip();                                    // 0x65d430
            s->ReadChunkValidate(0);                               // 0x65d460
        }
        s->ReadChunkValidate(0);                                   // 0x65d460 (MAPF)
    } catch (const char* e) {
        Logger.g->Log(1, "SChatRoomMenu::LoadMiniMap. Map loading error: %s", e);
        mini = false;
    }
    s->Release();                                                  // vtbl +0(1)
    return mini;
}

// PANZERS 0x655ab0
// The map list of the game type: multimaps/(*.map (Team Match),
// multimaps/Factory/*.map (Domination), multimaps/Assault/*.map; the name
// without the extension, the path as the second text; duplicates skipped.
// The first map is selected; SMulti +0x4a40 / +0x4b44 / +0x4c4c follow it.
void SSkirmishChatRoomMenu::FillMapList()
{
    Maps.CurSel = -1;                                              // +0x6e0 = -1
    Maps.ResetContent();                                           // 0x53c0c0(0)
    Maps.TopIndex = 0;                                             // +0x6d0 = 0
    Maps.Update();                                                 // vtbl +0x78
    const char* dir = nullptr;
    const char* filter = nullptr;
    switch (g_Skirmish->GameType) {
    case 0: dir = "multimaps/"; filter = "(*.map"; break;
    case 1: dir = "multimaps/Factory/"; filter = "*.map"; break;
    case 2: dir = "multimaps/Assault/"; filter = "*.map"; break;
    default: break;
    }
    SDArray<SString> files;
    memset(&files, 0, sizeof(files));
    if (dir)
        FileSystem.FindFiles(dir, filter, &files);                 // 0x65e720
    Logger.g->Log(0, "PZM5: map list %s%s: %d files", dir ? dir : "", filter ? filter : "", files.size);
    for (int i = 0; i < files.size; ++i) {
        const char* file = Str(files.array[i]);
        char path[0x104];
        _snprintf(path, sizeof(path) - 1, "%s%s", dir, file);      // 0x5335c0(dir, file)
        path[sizeof(path) - 1] = 0;
        char name[0x104];
        strncpy(name, file, sizeof(name) - 1);
        name[sizeof(name) - 1] = 0;
        int dot = -1;
        for (int k = 0; name[k]; ++k) {
            if (name[k] == '/' || name[k] == '\\')
                dot = -1;
            else if (name[k] == '.')
                dot = k;
        }
        if (dot >= 0)
            name[dot] = 0;                                         // 0x5651f0(0, dot)
        if (!IsMap(path)) {
            Logger.g->Log(0, "PZM5: %s has no MINI chunk", path);
            continue;
        }
        bool dup = false;
        for (int k = 0; k < Maps.ItemCount; ++k)                   // 0x53c490 (the second text)
            if (_stricmp(Str(Maps.Items[k].Text2), path) == 0)
                dup = true;
        if (!dup)
            Maps.AddItem(name, path, 0xd0d0d0, 0);                 // 0x53c020
    }
    for (int i = 0; i < files.size; ++i)
        FreeSString(&files.array[i]);
    free(files.array);
    Maps.Update();                                                 // 0x53ca40(0)
    if (Maps.ItemCount > 0) {
        Maps.CurSel = 0;                                           // +0x6e0 = 0
        Maps.Update();
    }
    if (Maps.CurSel < 0) {
        g_Skirmish->MapName[0] = 0;
        g_Skirmish->MapPath[0] = 0;
    } else {
        strncpy(g_Skirmish->MapName, Maps.GetItemText(Maps.CurSel), sizeof(g_Skirmish->MapName) - 1);   // 0x53c580
        strncpy(g_Skirmish->MapPath, Str(Maps.Items[Maps.CurSel].Text2), sizeof(g_Skirmish->MapPath) - 1);   // 0x53c490
    }
    g_Skirmish->MapCrc = 0;                                        // HD 0x65ec50(path, 0): the file CRC the clients check (no clients offline)
}

// PANZERS 0x654b30
// The map preview at (0x2ff, 0x1ba): the map's MINI bitmap (0x669ca0 /
// 0x66ea40) pasted centred into menu/minimap_hq.tga (board +0x84 the
// texture, Gepard +0x4c), menu/minimap_compass_hq.tga over it. HD releases
// both textures at once (board +0x80; the frames keep them); the recompile
// keeps the two board fonts until the next preview or the room's end.
void SSkirmishChatRoomMenu::LoadMiniMap(const char* path)
{
    if (strcmp(ShownMap, path) == 0)                               // (recompile) Update calls this every frame
        return;
    strncpy(ShownMap, path, sizeof(ShownMap) - 1);
    ReleaseMiniMap();                                              // board +0x0c(+0x1f4), +0x1f4 = -1
    SStream* s = FileSystem.OpenRead(path, nullptr);               // 0x65f420(path, 0)
    if (!s) {
        Logger.g->Log(1, "SChatRoomMenu::LoadMiniMap -> Can't open %s map file.", path);
        return;
    }
    pz::SHdBitmap* bmp = nullptr;
    try {
        s->ReadSignature();                                        // 0x65d6a0
        if (s->ReadChunkHeader() != 0x4650414d)                    // MAPF
            throw "Not a map file";
        if (s->ReadInt() != 0x31303276)                            // v201
            throw "Unsupported map file version";
        while (!s->ReadChunkIsEnd()) {                             // 0x65d3b0
            if (s->ReadChunkHeader() == 0x494e494d) {              // MINI
                if (bmp)
                    pz::HdBitmapDelete(bmp);
                bmp = pz::HdBitmapLoad(s);                         // new 0x20, 0x669ca0, 0x66ea40
            } else {
                s->ReadChunkSkip();                                // 0x65d430
            }
            s->ReadChunkValidate(0);                               // 0x65d460
        }
        s->ReadChunkValidate(0);
    } catch (const char* e) {
        Logger.g->Log(1, "SChatRoomMenu::LoadMiniMap. Map loading error: %s", e);
    }
    s->Release();                                                  // vtbl +0(1)
    if (!bmp)
        return;
    MiniMapFont = Board->LoadSingleFont("menu/minimap_hq.tga", Default);   // board +0x7c
    int tex = Board->GetFontTexture(MiniMapFont);                  // board +0x84
    pz::PzGepard()->UpdateTexture(tex, 0x80 - bmp->Width / 2, 0x80 - bmp->Height / 2, bmp);   // Gepard +0x4c
    MiniMapFrame = Board->CreateFrame(FT_SPRITE, BackFrame, 0x2ff, 0x1ba, 0, 0);   // board +0x08(1, +0x48, ...)
    Board->SetSpriteGlyph(MiniMapFrame, MiniMapFont, 0);           // board +0x24
    pz::HdBitmapDelete(bmp);                                       // 0x669cc0 + delete 0x20
    CompassFont = Board->LoadSingleFont("menu/minimap_compass_hq.tga", Default);
    int compass = Board->CreateFrame(FT_SPRITE, MiniMapFrame, 0, 0, 0, 0);
    Board->SetSpriteGlyph(compass, CompassFont, 0);
}

void SSkirmishChatRoomMenu::ReleaseMiniMap()
{
    if (MiniMapFrame >= 0)
        Board->DestroyFrame(MiniMapFrame);                         // board +0x0c(+0x1f4)
    MiniMapFrame = -1;
    if (MiniMapFont >= 0)
        Board->ReleaseFont(MiniMapFont);                           // board +0x80
    if (CompassFont >= 0)
        Board->ReleaseFont(CompassFont);
    MiniMapFont = CompassFont = -1;
}
// An army file (SMarket::SaveArmy 0x64a180 writes it): the signature, an
// "ARMY" (v5) or "AREC" (v2, coop) chunk with the army name, the CRC32 of
// the body (0x65d090) and the body xor-coded backwards (byte i ^ (i >> 3) ^
// i * 0x23 ^ 0x55); the body is the race (int), the SP (float) and the
// records (0x51f860). The inline reader of 0x654030 / 0x6547b0: HD throws
// a const char* and logs "...: Error loading army: %s".
// race / sp: the values the file must hold (sp < 0: not checked, coop).
static bool ReadArmyFile(const char* path, int race, int sp, SString* name, pz::SArmyArray* army, float* spOut,
                         const char* who)
{
    SStream* f = FileSystem.OpenRead(path, nullptr);               // 0x65f420(path, who)
    if (!f) {
        Logger.g->Log(1, "%s: Error loading army: %s", who, path);
        return false;
    }
    bool ok = true;
    try {
        f->ReadSignature();                                        // 0x65d6a0
        int tag = f->ReadChunkHeader();                            // 0x65d320
        if (tag != 0x594d5241 && tag != 0x43455241)                // ARMY, AREC
            throw "Not an army file";
        int ver = f->ReadInt();                                    // 0x65d4d0
        if (ver != 5 && ver != 2)
            throw "Bad army version";
        name->Load(f);                                             // 0x65d6f0
        SStreamBuffer body;                                        // 0x65cd20
        float crc = (float)f->ReadInt();                           // HD compares the CRCs as floats
        int n = f->ReadChunkRemain();                              // 0x65d3f0
        while (n-- != 0) {
            unsigned char b = f->ReadByte();                       // 0x65d300
            body.WriteByte((unsigned char)((n >> 3) ^ (n * 0x23) ^ b ^ 0x55));   // 0x65daf0
        }
        if (crc != (float)body.GenerateCRC())                      // 0x65d090
            throw "Corrupted army file";
        body.Seek(0, 0);                                           // 0x65d8a0(0, 0)
        if (body.ReadInt() != race)                                // 0x65d4d0
            throw "Hacker Laci, Race!";
        float s = body.ReadFloat();                                // 0x65d4b0
        if (spOut)
            *spOut = s;
        if (sp >= 0 && s != (float)sp)
            throw "Hacker Laci, SP!";
        if (army) {
            unsigned count = (unsigned)body.ReadInt();             // 0x51f860 (SDArray<SUnitDef>::Load)
            if (count > 0x1000000)
                throw "Invalid array size";
            pz::ArmyResize(army, (int)count);
            for (int i = 0; i < army->Size; ++i) {
                memset((void*)&army->Array[i], 0, sizeof(pz::SUnitDef));
                army->Array[i].Load(&body);                        // 0x5cfbd0
            }
            f->ReadChunkValidate(0);                               // 0x65d460
        }
    } catch (const char* e) {
        Logger.g->Log(1, "%s: Error loading army: %s", who, e);
        ok = false;
    }
    f->Release();                                                  // vtbl +0(1)
    return ok;
}

// PZ_M5_SK_SAVEARMY=1 (recompile-only test hook, inert when unset): the
// PZ_M5_SK_ARMY army is also written as an army file the way SMarket::
// SaveArmy 0x64a180 writes one in multi mode (not coop):
// armies/<G|A|R><e|l><SP>-<time>.army, so the file reader above can be
// tried. SaveArmy itself (the market's army making) is not lifted.
static void WriteTestArmyFile(const pz::SArmyArray* army, int race, int age, int sp, const char* name)
{
    SString dir;
    FileSystem.FileNameProcess(&dir, "armies");                    // 0x65f020, __mkdir
    _mkdir(Str(dir));
    FreeSString(&dir);
    char path[0x104];
    _snprintf(path, sizeof(path) - 1, "armies/%c%c%d-%d.army", race == 0 ? 'G' : race == 1 ? 'A' : 'R',
              age == 0 ? 'e' : 'l', sp, (int)time(nullptr));
    path[sizeof(path) - 1] = 0;
    SStreamBuffer body;                                            // 0x65cd20
    body.WriteInt(race);                                           // 0x65dc40
    body.WriteFloat((float)sp);                                    // 0x65dc20
    body.WriteInt(army->Size);                                     // 0x64a140 (SDArray<SUnitDef>::Save)
    for (int i = 0; i < army->Size; ++i)
        pz::UnitDefSave(&army->Array[i], &body);
    SStream* f = FileSystem.OpenWrite(path, nullptr);              // 0x65f7a0
    if (!f)
        return;
    f->WriteSignature();                                           // 0x65dc60
    f->WriteChunkStart(0x594d5241);                                // ARMY
    f->WriteInt(5);
    f->WriteString(name);                                          // 0x65dca0
    f->WriteInt(body.GenerateCRC());                               // 0x65d090
    int n = body.Seek(0, 2);
    body.Seek(0, 0);
    while (n-- != 0) {
        unsigned char b = body.ReadByte();
        f->WriteByte((unsigned char)((n >> 3) ^ (n * 0x23) ^ b ^ 0x55));
    }
    f->WriteChunkEnd();                                            // 0x65db10
    f->Release();
    Logger.g->Log(0, "PZM5: test army written to %s", path);
}

// PANZERS 0x654030
// The army files of the local player's nation, age and prestige limit:
// armies/<G|A|R><e|l><SP>*.army (FindFirstFileA / FindNextFileA); an item
// per readable file (ReadArmyFile: race and SP must match): the army name,
// the file. The army chosen before (the campaign's GetArmyName 0x591e00:
// the army loaded last, or the one the market just saved) becomes the
// selection; then LoadCurrentArmy.
// PZ_M5_SK_ARMY=1 (recompile-only test hook, inert when unset): one more
// item "PZ_M5_SK_ARMY" that gives the local player the Computer army of
// its nation (src/game/skirmish_game.cpp). Not needed since M6-AR (New /
// Edit make army files in the market); kept for scripted tests.
void SSkirmishChatRoomMenu::LoadArmyNames()
{
    if (!Visible || !ArmiesEnabled)                                // +0x39, +0x538
        return;
    Logger.g->Log(1, "SSkirmishChatRoomMenu::LoadArmyNames");
    Armies.CurSel = -1;                                            // +0x5ac = -1
    Armies.ResetContent();                                         // 0x53c0c0(0)
    Armies.TopIndex = 0;                                           // +0x59c = 0
    Armies.Update();
    int sp = pz::g_Campaign ? pz::g_Campaign->StartPrestige : 0;   // 0x592110 (MissionSP)
    char pattern[0x104];
    char race = Race == 0 ? 'G' : Race == 2 ? 'R' : 'A';           // 0x802568 / 0x80256c / 0x802570
    _snprintf(pattern, sizeof(pattern) - 1, "%c%c%d*.army", race, g_Skirmish->GameAge == 0 ? 'e' : 'l', sp);
    pattern[sizeof(pattern) - 1] = 0;
    SDArray<SString> files;
    memset(&files, 0, sizeof(files));
    FileSystem.FindFiles("armies/", pattern, &files);              // FindFirstFileA("armies/...")
    for (int i = 0; i < files.size; ++i) {
        char path[0x104];
        _snprintf(path, sizeof(path) - 1, "armies/%s", Str(files.array[i]));
        path[sizeof(path) - 1] = 0;
        FreeSString(&files.array[i]);
        SString name;
        if (!ReadArmyFile(path, Race, sp, &name, nullptr, nullptr, "SSkirmishChatRoomMenu::LoadArmyNames")) {
            FreeSString(&name);
            continue;
        }
        Armies.AddItem(Str(name), path, 0xd0d0d0, 0);              // 0x53c020(name, file, 0xd0d0d0, 0)
        if (_stricmp(Str(name), pz::g_Campaign->GetArmyName()) == 0)   // GetArmyName 0x591e00
            SelArmy = Armies.ItemCount - 1;                        // +0x1240 = +0x654 - 1
        FreeSString(&name);
    }
    free(files.array);
    const char* hook = getenv("PZ_M5_SK_ARMY");
    if (hook && *hook && *hook != '0')
        Armies.AddItem("PZ_M5_SK_ARMY", "PZ_M5_SK_ARMY", 0xd0d0d0, 0);
    if (SelArmy < Armies.ItemCount && SelArmy >= 0) {
        Armies.SetCurSel(SelArmy);                                 // 0x53caf0
        Armies.EnsureVisible(SelArmy);                             // 0x53c3c0
    } else {
        SelArmy = -1;
        Armies.SetCurSel(-1);
    }
    LoadCurrentArmy();                                             // 0x6547b0
}

void PzSkirmishTestArmy(pz::SArmyArray* out, int nation);         // src/game/skirmish_game.cpp (PZ_M5_SK_ARMY)

// PANZERS 0x6547b0
// The selected army file into the campaign: SetArmyName (clears the army),
// SetRace, read the file (its army name must be the list's), SetArmy
// 0x5971b0 (buy every record against the prestige limit), SetArmyName
// 0x597150 / SetArmyFileName 0x597120 (Edit army and SaveArmy use them).
void SSkirmishChatRoomMenu::LoadCurrentArmy()
{
    Logger.g->Log(1, "SSkirmishChatRoomMenu::LoadCurrentArmy");
    pz::g_Campaign->ClearArmy();                                   // 0x591d50
    pz::g_Campaign->SetRace(Race);                                 // 0x5974b0
    if (Armies.CurSel < 0)                                         // +0x5ac
        return;
    const char* file = Str(Armies.Items[Armies.CurSel].Text2);     // 0x53c490
    const char* text = Armies.GetItemText(Armies.CurSel);          // 0x53c580
    pz::SArmyArray army;
    memset(&army, 0, sizeof(army));
    if (strcmp(file, "PZ_M5_SK_ARMY") == 0) {                     // the PZ_M5_SK_ARMY item
        PzSkirmishTestArmy(&army, Race);
        const char* save = getenv("PZ_M5_SK_SAVEARMY");
        if (save && *save && *save != '0')
            WriteTestArmyFile(&army, Race, g_Skirmish->GameAge, pz::g_Campaign->StartPrestige, "PZ_M5_SK test army");
    } else {
        SString name;
        bool coop = g_Skirmish->GameType == 3;
        float sp = 0.0f;
        bool ok = ReadArmyFile(file, Race, coop ? -1 : pz::g_Campaign->StartPrestige, &name, &army, &sp,
                               "SSkirmishChatRoomMenu::LoadCurrentArmy");
        if (ok && _stricmp(text ? text : "", Str(name)) != 0) {
            // "A fajl nev alapjan megnyitott armyfajlban nem az a nevu csapat van aminek kellene.."
            Logger.g->Log(1, "SSkirmishChatRoomMenu::LoadCurrentArmy: Error loading army: %s", "the file holds another army");
            ok = false;
        }
        if (ok && coop)
            pz::g_Campaign->SetMissionSP((int)sp);                 // 0x597480
        FreeSString(&name);
        if (!ok) {
            pz::ArmyFree(&army);
            return;
        }
    }
    pz::g_Campaign->SetArmy(&army, g_Skirmish->GameType != 3);     // 0x5971b0(&army, !coop)
    pz::ArmyFree(&army);
    pz::g_Campaign->SetArmyName(text ? text : "");                 // 0x597150 (the file's army name = the item's)
    pz::g_Campaign->SetArmyFileName(file);                         // 0x597120
}
// PANZERS 0x656210
void SSkirmishChatRoomMenu::Update()
{
    PzSkirmish* m = g_Skirmish;
    if (pz::g_Campaign && m)
        pz::g_Campaign->SetMapName(m->MapPath);                    // 0x52c320(SMulti +0x4b44) on campaign +0x20
    if (NoMapsBox || !m || DeleteBox)
        return;
    if (m->Started && Countdown == -1) {                           // +0x4774
        CountdownTimer = SetTimer(1000);                           // 0x543b10(1000)
        Countdown = 4;
        return;
    }
    bool allReady = true;
    for (int i = 0; i < 8; ++i) {
        PzSkirmishSlot& s = m->Slots[i];
        Status[i].SetEnable(i != m->MySlot);                       // 0x539740 (server, not coop)
        if (i == m->MySlot)
            Race = (unsigned char)s.Nation;                        // +0x123c = slot nation
        Nations[i].SetEnable(i == m->MySlot || s.Status == 3);     // 0x539740
        if (s.Ready == 1) {
            Board->ShowFrame(ReadyFrames[i], true);                // board +0x18(f, 1)
            if (s.Status != 3) {
                if (i == m->MySlot) {
                    MeReady = true;                                // +0x52c
                    Nations[i].SetEnable(false);
                }
                if (m->Armies[i].Size == 0)                        // SMulti +0x4824 + slot * 0x34
                    allReady = false;
                // HD: and no army upload in progress (+0x47fc[8] != -1): none offline.
            }
        } else {
            if (i == m->MySlot)
                MeReady = false;
            if (s.Status == 2)
                allReady = false;
            Board->ShowFrame(ReadyFrames[i], false);
        }
        switch (s.Status) {
        case 0:
        case 1:
            if (Status[i].GetCount() != 3) {
                Status[i].ResetContent();                          // 0x5396a0
                Status[i].AddItem(Sk("Open"), 0);
                Status[i].AddItem(Sk("Closed"), 0);
                Status[i].AddItem(Sk("Computer"), 0);
            }
            Status[i].SetCurSel(s.Status);
            Nations[i].SetCurSel(-1);
            break;
        case 2:
            if (Status[i].GetCount() != 5) {
                Status[i].ResetContent();
                Status[i].AddItem(s.Name, 0);                      // SMulti +0x4c73
                Status[i].AddItem(Sk("Open"), 0);
                Status[i].AddItem(Sk("Closed"), 0);
                Status[i].AddItem(Sk("Kick Player"), 0);
                Status[i].AddItem(Sk("Computer"), 0);
            }
            Status[i].SetCurSel(0);
            Nations[i].SetCurSel(s.Nation);
            break;
        case 3:
            if (Status[i].GetCount() != 3) {
                Status[i].ResetContent();
                Status[i].AddItem(Sk("Open"), 0);
                Status[i].AddItem(Sk("Closed"), 0);
                Status[i].AddItem(Sk("Computer"), 0);
            }
            Status[i].SetCurSel(2);
            Nations[i].SetCurSel(s.Nation);
            break;
        default:
            break;
        }
    }
    Board->ShowFrame(PrestigeText, true);                          // board +0x18(+0x1084, 1)
    Board->ShowFrame(GameTypeText, true);
    Board->ShowFrame(GameAgeText, true);
    Prestige.SetVisible(true);                                     // vtbl +0x6c(1)
    GameType.SetVisible(true);
    GameAge.SetVisible(true);
    if (m->GameType == 2) {
        SetButtonText(&Team1, Sk("Attack"));                       // 0x538a10
        SetButtonText(&Team2, Sk("Defense"));
    } else {
        SetButtonText(&Team1, Sk("Team 1"));
        SetButtonText(&Team2, Sk("Team 2"));
    }
    // Server (+0x4f3c): the settings can be changed until they are locked.
    Prestige.SetEnable(true);                                      // 0x539740(1)
    GameType.SetEnable(true);
    GameAge.SetEnable(true);
    Maps.SetEnable(!m->Locked);                                    // vtbl +0x70
    if (m->Locked) {
        Prestige.SetEnable(false);
        GameType.SetEnable(false);
        GameAge.SetEnable(false);
    }
    Board->SetText(MapNameText, g_PzFont[PZF_SANS21_SHADOW], 0, m->MapName);   // board +0x34(+0x1238, 3, 0, +0x4a40)
    LoadMiniMap(m->MapPath);                                       // 0x654b30(+0x4b44)
    int psel = Prestige.GetCurSel();
    pz::g_Campaign->SetMissionSP(atoi(psel >= 0 && psel < Prestige.ItemCount ? Str(Prestige.Items[psel].Text) : ""));   // 0x597480
    Armies.SetEnable(m->Locked && !MeReady);                       // vtbl +0x70
    NewArmy.SetEnable(m->Locked && !MeReady);
    SelArmy = Armies.CurSel;                                       // +0x1240 = +0x5ac
    if (!m->Locked) {
        Armies.CurSel = -1;
        Armies.ResetContent();
        Armies.TopIndex = 0;
        Armies.Update();
        Armies.AddItem(Sk("Lock game settings so players can choose army."), "", 0xd0d0d0, 0);
    } else if (m->ArmiesChanged) {                                 // +0x4a1d
        m->ArmiesChanged = false;
        LoadArmyNames();                                           // 0x654030
    }
    ImReady.SetEnable(m->Started ? false : SelArmy >= 0);          // +0x26c
    EditArmy.SetEnable(m->Locked && !MeReady && SelArmy >= 0);
    DeleteArmy.SetEnable(m->Locked && !MeReady && SelArmy >= 0);
    bool team1 = false, team2 = false;
    if (!m->Started) {
        for (int i = 0; i < 8; ++i)
            if (m->Slots[i].Connected || m->Slots[i].Status == 3)
                (i < 4 ? team1 : team2) = true;
    }
    StartGame.SetEnable(team1 && team2 && allReady);               // +0x1f8
    SetButtonText(&ImReady, MeReady ? Sk("I'm Not Ready") : Sk("I'm Ready"));
    pz::g_Campaign->SetMapName(m->MapPath);
}

// PANZERS 0x654df0
bool SSkirmishChatRoomMenu::OnAction(SWidget* source, int action, int param)
{
    PzSkirmish* m = g_Skirmish;
    if (action == 0x42546)
        return false;
    if (action == PZA_BUTTON_CLICK) {                              // 0x42542
        if (source == &Team1)                                      // +0x5c
            m->ChangeTeam(0);                                      // 0x51e3c0(0)
        if (source == &Team2)                                      // +0xd0
            m->ChangeTeam(1);
        if (source == &LockSettings) {                             // +0x354
            if (!m->Locked) {
                for (int i = 0; i < 8; ++i)                        // the Computers are ready
                    if (m->Slots[i].Status == 3)
                        m->Slots[i].Ready = 1;
            }
            // else: 0x51e1b0 cancels the army uploads (none offline).
            m->Locked = !m->Locked;                                // +0x4a30
            if (!m->Locked) {
                SetButtonText(&LockSettings, Sk("Lock Settings"));
                m->SetReady(true);                                 // 0x51e2b0(1)
            } else {
                SetButtonText(&LockSettings, Sk("Unlock Settings"));
            }
            m->ArmiesChanged = true;                               // 0x5215e0 SendGameSettings: +0x4a1d = 1
        }
        if (source == &StartGame) {                                // +0x1f8
            StartGame.SetEnable(false);
            ImReady.SetEnable(false);
            LockSettings.SetEnable(false);
            m->Start();                                            // 0x521d60
        }
        if (source == &ImReady) {                                  // +0x26c
            pz::g_Campaign->GetMissionArmy(&m->Armies[m->MySlot]); // 0x591e70(SMulti army of the slot)
            m->SetReady(false);                                    // 0x51e2b0(0)
        }
        if (source == &Cancel) {                                   // +0x2e0
            SendAction(PZA_SKIRMISH_CANCEL, 0);                    // 0x543930(0x534b2, 0)
            return true;
        }
        if (source == &NewArmy) {                                  // +0x3c8
            ArmiesEnabled = false;                                 // +0x538 = 0
            m->SetNotReady(true);                                  // 0x51e270(1)
            SendAction(PZA_SKIRMISH_NEW, param);                   // 0x543930(0x534b3, param)
            return true;
        }
        if (source == &EditArmy) {                                 // +0x43c
            ArmiesEnabled = false;
            m->SetNotReady(true);
            SendAction(PZA_SKIRMISH_EDIT, 0);                      // 0x543930(0x534b4, 0)
            return true;
        }
        if (source == &DeleteArmy) {                               // +0x4b0
            DeleteBox = new pz::SMessageBox();                     // new 0x3ec, 0x53e0d0
            InsertChild(DeleteBox);                                // vtbl +0x54
            DeleteBox->Create("", "", 4, false);                   // 0x53e3b0("", "", 4): Yes / No
            DeleteBox->SetTarget(this);
            DeleteBox->SetText(Sk("Are you sure, do you want to delete this army?"), 0xd0d0d0);
            DeleteBox->SetVisible(true);
        }
        return true;
    }
    if (action == PZA_DROPLIST_SELECT) {                           // 0x444c1
        for (int i = 0; i < 8; ++i) {
            if (source == &Nations[i]) {
                m->SetNation(i, param);                            // 0x51e3a0(i, sel)
                m->ArmiesChanged = true;                           // +0x4a1d
            }
            if (source != &Status[i])
                continue;
            Status[i].SetCurSel(param);                            // 0x5396f0
            PzSkirmishSlot& s = m->Slots[i];
            switch (s.Status) {
            case 0:
            case 1:
            case 3:
                if (param == 0) { s.Status = 0; s.Ready = 0; }
                else if (param == 1) { s.Status = 1; s.Ready = 0; }
                else if (param == 2) { s.Status = 3; s.Ready = 1; }
                break;
            case 2:                                                // a human: 0x51f7f0 kicks it first
                if (param == 1 || param == 3) { s.Status = 0; s.Ready = 0; }
                else if (param == 2) { s.Status = 1; s.Ready = 0; }
                else if (param == 4) { s.Status = 3; s.Ready = 1; }
                break;
            default:
                break;
            }
            // 0x521a70: the slot info to the other hosts (none offline).
            break;
        }
        if (source != &Prestige && source != &GameType && source != &GameAge)
            return true;
        if (m->GameAge != GameAge.GetCurSel())
            FillPrestigeList(GameAge.GetCurSel());                 // 0x6560a0
        int psel = Prestige.GetCurSel();
        m->PrestigeLimit = atoi(psel >= 0 && psel < Prestige.ItemCount ? Str(Prestige.Items[psel].Text) : "");   // +0x4a34
        m->GameType = GameType.GetCurSel();                        // +0x4a38
        m->GameAge = GameAge.GetCurSel();                          // +0x4a3c
        if (source == &GameType)
            FillMapList();                                         // 0x655ab0
        m->ArmiesChanged = true;                                   // 0x5215e0 SendGameSettings (+0x4a1d = 1; nothing to send offline)
        return true;
    }
    if (action == PZA_LISTBOX_SELECT) {                            // 0x4c421
        if (source == &Maps) {                                     // +0x670, server
            if (m->GameType != 3) {
                const char* t = Maps.GetItemText(Maps.CurSel);     // 0x53c580
                strncpy(m->MapName, t ? t : "", sizeof(m->MapName) - 1);   // +0x4a40
            }
            const char* p = Maps.CurSel >= 0 && Maps.CurSel < Maps.ItemCount ? Str(Maps.Items[Maps.CurSel].Text2) : "";
            strncpy(m->MapPath, p, sizeof(m->MapPath) - 1);        // +0x4b44 (0x53c490)
            m->MapCrc = 0;                                         // 0x65ec50
            return true;
        }
        if (source == &Armies) {                                   // +0x53c
            LoadCurrentArmy();                                     // 0x6547b0
            // HD 0x60f9f0: SChatRoomMenu's "send my army" (no receiver offline).
        }
        return true;
    }
    if (NoMapsBox && source == NoMapsBox) {                        // +0x530
        delete NoMapsBox;
        NoMapsBox = nullptr;
        SendAction(PZA_SKIRMISH_CANCEL, 0);
        return true;
    }
    if (DeleteBox && source == DeleteBox) {                        // +0x534
        delete DeleteBox;
        DeleteBox = nullptr;
        if (action == 0x4d582 || action == 0x4d581) {              // Yes / OK
            const char* file = Armies.CurSel >= 0 && Armies.CurSel < Armies.ItemCount ? Str(Armies.Items[Armies.CurSel].Text2) : "";
            if (*file)
                DeleteFileA(file);                                 // GetArmyFileName 0x591dd0
            LoadArmyNames();                                       // 0x654030
        }
        return true;
    }
    return false;
}
