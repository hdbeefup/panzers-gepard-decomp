// src/game/campaign_save.cpp
// The campaign part of the save game: SPanzersCampaign::SaveGame 0x5966a0
// (moved here from campaign.cpp) and LoadGame 0x594f70, and the save-file
// header read of the Load Game screen (0x5955d0). The game state is
// SGameLogic::SaveGameState / LoadGameState (gamelogic_save.cpp).
// OWNER: agent S (M4, docs/M4_STATUS.md, docs/FORMATS.md).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "campaign.h"
#include "stub_log.h"
#include "logger.h"
#include "stream.h"
#include "pzunitregistry.h"
#include "unitsave.h"
#include "world.h"
#include "worldapi.h"
#include "gamelogic.h"
#include "m3common.h"
#include "world_save.h"
#include "gettext.h"

namespace pz {

static void SetSaveStr(SString* s, const char* text)
{
    // 0x52c320
    FreeSString(s);
    int n = (int)strlen(text);
    s->size = n;
    s->buf = new char[n + 1];
    memcpy(s->buf, text, n + 1);
}

static void ReadSaveStr(SStream* s, SString* out)
{
    // 0x65d6f0 / 0x56e7d0: u16 length, bytes
    FreeSString(out);
    int n = s->ReadWord() & 0xffff;
    out->size = n;
    out->buf = new char[n + 1];
    s->Read(out->buf, n);
    out->buf[n] = 0;
}

// PANZERS 0x51f860 (SDArray<SUnitDef>::Load; campaign.cpp has the same as a static)
static void ArmyLoadSave(SArmyArray* a, SStream* s)
{
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        throw "Invalid array size";
    ArmyResize(a, (int)n);
    for (int i = 0; i < a->Size; ++i) {
        memset((void*)&a->Array[i], 0, sizeof(SUnitDef));
        a->Array[i].Load(s);                                      // 0x5cfbd0
    }
}

static void CheckSaveFileHeader(SStream* s)
{
    s->ReadSignature();                                           // 0x65d6a0
    if (s->ReadChunkHeader() != 0x45564153)                       // 'SAVE'
        throw "Not a map file";
    if (s->ReadInt() != 0x61703476)                               // 'v4pa'
        throw "Unsupported map file version";
}

// PANZERS 0x5966a0
// SaveGames/<file>: 'SAVE' chunk {'v4pa', 1, map, mission code, title, mode,
// race, start prestige, Army, MissionArmy, prestige, section, +0xe4,
// difficulty, +0xf8, +0xfc, +0xb84, objectives, 12 player records, +0xb60,
// then the game state (SGameLogic 0x57e110)}. docs/FORMATS.md.
bool SPanzersCampaign::SaveGame(const char* file, const char* title)
{
    PZ_M3_TRACE("SPanzersCampaign::SaveGame (0x5966a0)");
    if (GameMode == 4)
        return false;
    SString dir;
    FileSystem.FileNameProcess(&dir, "SaveGames");                // 0x65f020
    CreateDirectoryA(SStr(dir), nullptr);                         // 0x65fde0 (MakeDir)
    FreeSString(&dir);
    SStream* s = FileSystem.OpenWrite(file, nullptr);             // 0x65f7a0
    if (!s)
        return false;
    s->WriteSignature();                                          // 0x65dc60
    s->WriteChunkStart(0x45564153);                               // 'SAVE'
    s->WriteInt(0x61703476);                                      // 'v4pa'
    s->WriteInt(1);
    s->WriteString(GetMapName());
    s->WriteString(GetMissionCode());
    s->WriteString(title ? title : "");
    s->WriteInt(GameMode);
    s->WriteInt(Race);
    s->WriteInt(StartPrestige);
    s->WriteInt(Army.Size);
    for (int i = 0; i < Army.Size; ++i)
        UnitDefSave(&Army.Array[i], s);
    s->WriteInt(MissionArmy.Size);
    for (int i = 0; i < MissionArmy.Size; ++i)
        UnitDefSave(&MissionArmy.Array[i], s);
    s->WriteInt(Prestige);
    s->WriteString(SStr(MissionSection));
    s->WriteInt(MissionResult);                                     // +0xe4
    s->WriteInt(Difficulty);
    s->WriteInt(_0f8);
    s->WriteInt(_0fc);
    s->WriteInt(_b84);
    s->WriteInt(ObjectiveCount);
    for (int i = 0; i < ObjectiveCount; ++i) {
        s->WriteByte((unsigned char)Objectives[i].Hidden);        // +4 (byte)
        s->WriteInt(Objectives[i].State);                         // +0
    }
    for (int pl = 0; pl < 12; ++pl) {
        const int* r = (const int*)PlayerStats[pl];
        // 12 x {+0x0c, +0x3c, +0x6c, +0x9c} (index i), then +0xcc, +0xd4.
        for (int i = 0; i < 12; ++i) {
            s->WriteInt(r[3 + i]);
            s->WriteInt(r[0xf + i]);
            s->WriteInt(r[0x1b + i]);
            s->WriteInt(r[0x27 + i]);
        }
        s->WriteInt(r[0x33]);
        s->WriteInt(r[0x35]);
    }
    s->WriteInt(Score);                                           // +0xb60
    if (g_GameLogic)
        g_GameLogic->SaveGameState(s);                            // 0x57e110
    s->WriteChunkEnd();                                           // 0x65db10
    s->Release();
    return true;
}


// PANZERS 0x5955d0
// The map name of a save (the Load Game screen and SGameView's load-game
// LoadMap 0x61f840): signature, 'SAVE', 'v4pa', version 1, the map.
bool ReadSaveGameMapName(const char* file, SString* map)
{
    SStream* s = FileSystem.OpenRead(file, nullptr);              // 0x65f420
    if (!s)
        return false;
    bool ok = false;
    try {
        CheckSaveFileHeader(s);
        if (s->ReadInt() == 1) {
            ReadSaveStr(s, map);
            ok = true;
        }
    } catch (...) {
        ok = false;
    }
    s->Release();
    return ok;
}

// The save's title (the Load Game list): header, map, mission code, title.
bool ReadSaveGameTitle(const char* file, SString* title)
{
    SStream* s = FileSystem.OpenRead(file, nullptr);
    if (!s)
        return false;
    bool ok = false;
    try {
        CheckSaveFileHeader(s);
        if (s->ReadInt() == 1) {
            SString skip;
            ReadSaveStr(s, &skip);
            ReadSaveStr(s, &skip);
            FreeSString(&skip);
            ReadSaveStr(s, title);
            ok = true;
        }
    } catch (...) {
        ok = false;
    }
    s->Release();
    return ok;
}

// PANZERS 0x595fa0 (SPanzersCampaign::LoadSavedGameNames; the recompile
// has it as a free function)
// Every SaveGames/*.save whose header reads ('SAVE', 'v4pa' / 'v2ps' /
// 'v1ps'): the mission code, the title, "<date> <time>" of the file (local
// time, the user's short date and time formats) and the file name, and
// the SYSTEMTIME for sorting. A file that fails to read is logged and
// skipped.
void LoadSavedGameNames(SHdArray<SLoadSaveName>* out)
{
    Logger.g->Log(1, "SLoadMenu::LoadSavedGameNames");
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("SaveGames\\*.save", &fd);          // 0x65e720("SaveGames/*.save")
    if (h == INVALID_HANDLE_VALUE)
        return;
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            continue;
        char path[MAX_PATH + 16];
        _snprintf(path, sizeof(path) - 1, "SaveGames/%s", fd.cFileName);
        path[sizeof(path) - 1] = 0;
        SStream* s = FileSystem.OpenRead(path, nullptr);          // 0x65f420
        if (!s)
            continue;
        SString code, title, skip;
        try {
            s->ReadSignature();                                   // 0x65d6a0
            if (s->ReadChunkHeader() != 0x45564153)               // 'SAVE'
                throw "Not a map file";
            int v = s->ReadInt();
            if (v != 0x61703476 && v != 0x73703276 && v != 0x73703176)   // 'v4pa', 'v2ps', 'v1ps'
                throw "Unsupported map file version";
            s->ReadInt();                                         // the save type
            ReadSaveStr(s, &skip);                                // the map
            ReadSaveStr(s, &code);                                // 0x5910e0
            ReadSaveStr(s, &title);
            FILETIME lt;
            SYSTEMTIME st;
            FileTimeToLocalFileTime(&fd.ftLastWriteTime, &lt);
            FileTimeToSystemTime(&lt, &st);
            char date[260], time[260];
            GetDateFormatA(0, DATE_SHORTDATE, &st, nullptr, date, sizeof(date));
            GetTimeFormatA(0, TIME_NOSECONDS | TIME_FORCE24HOURFORMAT | TIME_NOTIMEMARKER, &st, nullptr, time, sizeof(time));
            if (out->Size == out->Max) {                          // 0x591550 SDArray::Add
                int m = out->Max < 0x10 ? 0x10 : out->Max * 6 / 5;
                out->Array = (SLoadSaveName*)realloc(out->Array, m * sizeof(SLoadSaveName));
                memset(out->Array + out->Max, 0, (m - out->Max) * sizeof(SLoadSaveName));
                out->Max = m;
            }
            SLoadSaveName* n = &out->Array[out->Size++];
            SetSaveStr(&n->Code, SStr(code));                     // +0x00
            SetSaveStr(&n->Title, SStr(title));                   // +0x08
            char when[530];
            _snprintf(when, sizeof(when) - 1, "%s %s", date, time);   // date + " " (0x7f33a0) + time
            when[sizeof(when) - 1] = 0;
            SetSaveStr(&n->Date, when);                           // +0x10
            SetSaveStr(&n->File, fd.cFileName);                   // +0x18
            n->Time = st;                                         // +0x20
        } catch (const char* e) {
            Logger.g->Warning("SLoadMenu::LoadSavedGameNames: Error loading saved game. %s", e);
        }
        FreeSString(&code);
        FreeSString(&title);
        FreeSString(&skip);
        s->Release();
    } while (FindNextFileA(h, &fd));
    FindClose(h);                                                 // 0x591150 frees the file list
}

// PANZERS 0x596b30 (SPanzersCampaign::SaveGameBefore; a free function in
// the recompile so that campaign.h stays untouched: K's stub
// LetResultsDone 0x594e70 calls it)
// Not in multiplayer (mode 4). SaveGames/<mission code>-Before.save:
// 'SAVE' {'v2ps', 2, map, mission code, "Before - <name>", mode, race,
// start prestige, the Army (+0x2c) records, section, +0xe4, difficulty,
// +0xf8, +0xfc, score +0xb60, +0xb84}. No game state: LoadGameBefore
// 0x595330 starts the mission from it.
bool CampaignSaveGameBefore(SPanzersCampaign* c)
{
    PZ_M3_TRACE("SPanzersCampaign::SaveGameBefore (0x596b30)");
    if (c->GameMode == 4)
        return false;
    SString dir;
    FileSystem.FileNameProcess(&dir, "SaveGames");                // 0x65f020
    CreateDirectoryA(SStr(dir), nullptr);                         // 0x65fde0
    FreeSString(&dir);
    const char* code = c->GetMissionCode();                       // 0x660570(section, "Mission code")
    char file[400];
    _snprintf(file, sizeof(file) - 1, "SaveGames/%s-Before.save", code);   // 0x51ee20(0x7f9318)
    file[sizeof(file) - 1] = 0;
    SStream* s = FileSystem.OpenWrite(file, nullptr);             // 0x65f7a0
    if (!s)
        return false;
    s->WriteSignature();                                          // 0x65dc60
    s->WriteChunkStart(0x45564153);                               // 'SAVE'
    s->WriteInt(0x73703276);                                      // 'v2ps'
    s->WriteInt(2);
    s->WriteString(c->GetMapName());                              // 0x592040
    s->WriteString(code);
    char title[600];
    _snprintf(title, sizeof(title) - 1, "%s - %s",
              GetText("world/PanzersCampaign.cpp", "Before"), c->GetMapName());   // 0x660c50, " - ", 0x5925d0
    title[sizeof(title) - 1] = 0;
    s->WriteString(title);
    s->WriteInt(c->GameMode);                                     // +0x10
    s->WriteInt(c->Race);                                         // +0x18
    s->WriteInt(c->StartPrestige);                                // +0x28
    s->WriteInt(c->Army.Size);                                    // +0x30
    for (int i = 0; i < c->Army.Size; ++i)
        UnitDefSave(&c->Army.Array[i], s);                        // 0x5cfbf0
    s->WriteString(SStr(c->MissionSection));                      // +0xd8
    s->WriteInt(c->MissionResult);                                // +0xe4
    s->WriteInt(c->Difficulty);                                   // +0x1c
    s->WriteInt(c->_0f8);
    s->WriteInt(c->_0fc);
    s->WriteInt(c->Score);                                        // +0xb60
    s->WriteInt(c->_b84);
    s->WriteChunkEnd();
    s->Release();
    return true;
}

// PANZERS 0x594f70
// Called by SGameView's load-game LoadMap 0x61f840 after the map is loaded
// (or kept, when the save is on the current map): the campaign part, then
// SGameLogic::LoadGameState 0x56eb50(stream, 1).
bool SPanzersCampaign::LoadGame(const char* file)
{
    PZ_M3_TRACE("SPanzersCampaign::LoadGame (0x594f70)");
    MenuToLoad = 2;                                               // +0xe0
    char path[400];
    // HD: SString "SaveGames/" + file (0x52c580); the callers pass the
    // full "SaveGames/<name>" path, which the recompile accepts as well.
    if (_strnicmp(file, "SaveGames/", 10) == 0 || _strnicmp(file, "SaveGames\\", 10) == 0)
        _snprintf(path, sizeof(path) - 1, "%s", file);
    else
        _snprintf(path, sizeof(path) - 1, "SaveGames/%s", file);
    path[sizeof(path) - 1] = 0;
    SStream* s = FileSystem.OpenRead(path, nullptr);              // 0x65f420(name, 0)
    if (!s)
        return false;
    CheckSaveFileHeader(s);
    if (s->ReadInt() != 1)
        throw "Not a normal save game file";
    SString str;
    ReadSaveStr(s, &str);
    SetSaveStr(&MapName, SStr(str));                              // 0x52c320 on +0x20
    ReadSaveStr(s, &str);                                         // mission code
    ReadSaveStr(s, &str);                                         // title
    FreeSString(&str);
    GameMode = s->ReadInt();
    Race = s->ReadInt();
    StartPrestige = s->ReadInt();
    ArmyLoadSave(&Army, s);                                       // 0x51f860 on +0x2c
    ArmyLoadSave(&MissionArmy, s);                                // 0x51f860 on +0x3c
    Prestige = s->ReadInt();
    ReadSaveStr(s, &MissionSection);                              // 0x56e7d0 on +0xd8
    MissionResult = s->ReadInt();                                 // +0xe4
    Difficulty = s->ReadInt();
    _0f8 = s->ReadInt();
    _0fc = s->ReadInt();
    _b84 = s->ReadInt();
    LoadMissionProps();                                           // 0x593740
    LoadObjectives();                                             // 0x593ba0
    int n = s->ReadInt();
    if (ObjectiveCount < n)
        Logger.g->Panic("SPanzersCampaign::LoadGame()");
    for (int i = 0; i < n; ++i) {
        Objectives[i].Hidden = s->ReadByte() != 0;                // +4 (0x560580(i))
        Objectives[i].State = s->ReadInt();                       // +0
    }
    for (int pl = 0; pl < 12; ++pl) {
        int* r = (int*)PlayerStats[pl];
        for (int i = 0; i < 12; ++i) {
            r[3 + i] = s->ReadInt();
            r[0xf + i] = s->ReadInt();
            r[0x1b + i] = s->ReadInt();
            r[0x27 + i] = s->ReadInt();
        }
        r[0x33] = s->ReadInt();
        r[0x35] = s->ReadInt();
    }
    Score = s->ReadInt();                                         // +0xb60
    if (g_GameLogic)
        g_GameLogic->LoadGameState(s, true);                      // 0x56eb50(stream, 1)
    s->ReadChunkValidate(0);                                      // 'SAVE'
    s->Release();
    return true;
}

} // namespace pz
