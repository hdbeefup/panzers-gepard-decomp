// src/game/triggers_script.cpp
// SGameLogic::ExecuteScriptStatement 0x568bc0 (M4 agent T): one line of the
// script language of the .ingame cut-scenes ("move teher1, 52.06, 60.52").
// The command table is HD's (0x8db080, 0x28 per entry: id, name, argument
// types 1 float / 2 string / 3 int). The commands the tutorial cut-scenes use
// are lifted; the others log a STUB line once and do nothing.

#include <stdlib.h>
#include <string.h>
#include "cutscene.h"
#include "gamelogic.h"
#include "trigger.h"
#include "triggersunits.h"
#include "worldapi.h"
#include "world.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

using m2u::UV;

void ClearFound(SFoundUnits* g, int size);   // triggers.cpp 0x563580
SFoundUnit* AddFound(SFoundUnits* g);        // triggers.cpp 0x560d80 + 0x560790
void GroupStats(SFoundUnits* g);             // triggers.cpp 0x582770

namespace {

struct SScriptCommand {
    int         Id;
    const char* Name;
    int         Args[8];   // 1 float, 2 string, 3 int, 0 end
};

// HD 0x8db080 (dumped from the exe, same order: the first name that matches wins).
const SScriptCommand kCommands[] = {
    {0, "create", {2, 2, 1, 1, 1, 3}}, {0, "cr", {2, 2, 1, 1, 1}}, {1, "create2", {2, 2, 1, 1, 1, 3}},
    {2, "move", {2, 1, 1}}, {2, "mv", {2, 1, 1}}, {3, "move_backward", {2, 1, 1}}, {3, "mvb", {2, 1, 1}},
    {4, "follow", {2, 2}}, {4, "flw", {2, 2}}, {5, "attackground", {2, 1, 1}}, {5, "ag", {2, 1, 1}},
    {6, "attack", {2, 2}}, {6, "a", {2, 2}}, {7, "attackmove", {2, 1, 1}}, {7, "am", {2, 1, 1}},
    {8, "stop", {2}}, {9, "die", {2}}, {10, "boomdie", {2}}, {11, "remove", {2}}, {11, "re", {2}},
    {12, "stand", {2}}, {12, "std", {2}}, {13, "kneel", {2}}, {14, "lay", {2}},
    {15, "gunner_aim_ground", {2, 1, 1}}, {15, "gag", {2, 1, 1}}, {16, "gunner_aim_unit", {2, 2}},
    {16, "gau", {2, 2}}, {17, "gunner_lock", {2}}, {17, "gl", {2}}, {18, "gunner_change_active", {2, 3}},
    {18, "cag", {2, 3}}, {19, "board", {2, 2}}, {20, "unloadall", {2}}, {20, "un", {2}},
    {21, "gunner_spin_to_dir", {2, 1}}, {21, "gstd", {2, 1}}, {22, "fx", {2, 1, 1, 1}},
    {23, "change_behavior", {2, 3}}, {23, "cb", {2, 3}}, {24, "anim", {2, 2}}, {25, "tow", {2, 2}},
    {26, "untow", {2}}, {27, "heavy_bombardment", {1, 1, 3, 1, 3}}, {28, "tactical_bombardment", {1, 1, 3}},
    {29, "recon_plane", {1, 1, 3, 1, 3}}, {30, "parachute", {1, 1, 3, 1, 3}}, {31, "cannonade", {1, 1, 3}},
    {32, "create_model", {2, 1, 1, 1}}, {33, "move_to_dir", {2, 1, 1, 1}}, {34, "sethp", {2, 1}},
    {35, "createwithoutcrew", {2, 2, 1, 1, 1, 3}}, {36, "hideallunit", {0}}, {37, "showallunit", {0}},
    {38, "disableai", {2}}, {39, "enableai", {2}}, {40, "attack_move_on_path", {2, 2}},
    {41, "move_units_on_path", {2, 2}}, {42, "move_units_on_path_in_convoy", {2, 2}}, {43, "setweather", {2}},
    {44, "disable_ambient_sounds", {0}}, {45, "enable_ambient_sounds", {0}},
};

// The commands that do not act on the units named by their first argument
// (no found-unit group is built for them).
bool NoGroup(int id)
{
    return id == 0 || id == 1 || id == 0x16 || id == 0x1b || id == 0x1c || id == 0x20 || id == 0x1e ||
           id == 0x1d || id == 0x1f || id == 0x24 || id == 0x25 || id == 0x2b || id == 0x2d || id == 0x2c;
}

// HD 0x55cbd0 (SString ==, case-insensitive) or the '*' prefix test.
bool NameMatches(int u, const char* name, int len)
{
    const char* id = UV::ScriptId(u);
    if (len > 0 && name[len - 1] == '*')
        return strncmp(id, name, len - 1) == 0;
    return (int)strlen(id) == len && (len == 0 || _stricmp(id, name) == 0);
}

} // namespace

// PANZERS 0x568bc0
void PzExecuteScriptStatement(SGameLogic* gl, const char* text, bool preprocess)
{
    const char* p = text;
    while (*p && *p != ' ' && *p != '\t')
        ++p;
    char word[64];
    int wl = (int)(p - text) < 63 ? (int)(p - text) : 63;
    memcpy(word, text, wl);
    word[wl] = 0;
    const SScriptCommand* cmd = nullptr;
    for (const SScriptCommand& c : kCommands) {                   // strcmp loop over 0x8db084
        if (strcmp(word, c.Name) == 0) {
            cmd = &c;
            break;
        }
    }
    if (!cmd)
        Logger.g->Panic("SGameLogic::ExecuteScriptStatement: Unknown command: %s", text);
    // Arguments: strings at local_74 (8 SStrings), floats at local_34, ints
    // at local_94, each by argument index.
    char  str[8][128];
    int   strLen[8];
    float fl[8];
    int   in[8];
    memset(strLen, 0, sizeof(strLen));
    memset(fl, 0, sizeof(fl));
    memset(in, 0, sizeof(in));
    for (int k = 0; k < 8 && cmd->Args[k] != 0; ++k) {
        while (*p == ' ' || *p == '\t')
            ++p;
        const char* b = p;
        while (*p && *p != ',')
            ++p;
        int n = (int)(p - b);
        if (n == 0)
            Logger.g->Panic("SGameLogic::ExecuteScriptStatement: Not enough parameters: %s", text);
        char tok[128];
        if (n > 127)
            n = 127;
        memcpy(tok, b, n);
        tok[n] = 0;
        while (n > 0 && tok[n - 1] == ' ')                        // 0x5651f0: trailing spaces off
            tok[--n] = 0;
        if (cmd->Args[k] == 1)
            fl[k] = (float)atof(tok);
        else if (cmd->Args[k] == 2) {
            strcpy(str[k], tok);
            strLen[k] = n;
        } else if (cmd->Args[k] == 3)
            in[k] = atoi(tok);
        if (*p == ',')
            ++p;
    }
    SFoundUnits group;
    memset(&group, 0, sizeof(group));
    group.Leader = -1;
    if (!NoGroup(cmd->Id)) {
        if (strLen[0] < 1)
            Logger.g->Panic("SString::operator[]: invalid index (%d)", strLen[0] - 1);
        PZ_FOR_EACH_UNIT(i) {                                     // 0x56b3b0: every live unit
            if (NameMatches(i, str[0], strLen[0]))
                AddFound(&group)->Unit = i;
        }
        GroupStats(&group);                                       // 0x582770
    }
    if (preprocess) {
        // HD: "create" / "create2" check the unit class (0x5d0e70),
        // "create_model" preloads the model (Gepard +0x20 / +0x24).
        if (cmd->Id == 0 || cmd->Id == 1 || cmd->Id == 0x20)
            STUB_LOG("SGameLogic::ExecuteScriptStatement preprocess create / create_model (0x5d0e70)");
        free(group.Units);
        return;
    }
    float target[2] = {fl[1], fl[2]};
    switch (cmd->Id) {
    case 2:                                                       // move
        gl->MoveFoundUnitsToLocation(&group, 1, target, false, false, false);   // 0x57efd0(g, 1, &t, 0, 0, 0)
        break;
    case 3:                                                       // move_backward
        gl->MoveFoundUnitsToLocation(&group, 3, target, false, false, false);
        break;
    case 5:                                                       // attackground
        gl->MoveFoundUnitsToLocation(&group, 0x10, target, false, false, false);
        break;
    case 7:                                                       // attackmove
        gl->MoveFoundUnitsToLocation(&group, 9, target, false, false, false);
        break;
    case 8:                                                       // stop
        gl->OrderPlain(&group, 8, false, false);                  // 0x564440(g, 8, 0, 0)
        break;
    case 9:                                                       // die
        gl->OrderPlain(&group, 0x26, false, false);
        break;
    case 10:                                                      // boomdie
        gl->OrderPlain(&group, 0x27, false, false);
        break;
    case 0xb:                                                     // remove: every live unit of that script id
        PZ_FOR_EACH_UNIT(i) {
            const char* id = UV::ScriptId(i);
            if ((int)strlen(id) == strLen[0] && (strLen[0] == 0 || _stricmp(id, str[0]) == 0))
                g_World->RemoveUnit(i);                           // 0x5f8060
        }
        break;
    case 0xc:                                                     // stand
        gl->OrderValue(&group, 0x28, 0, false, false);            // 0x564870(g, 0x28, 0, 0, 0)
        break;
    case 0xd:                                                     // kneel
        gl->OrderValue(&group, 0x28, 1, false, false);
        break;
    case 0xe:                                                     // lay
        gl->OrderValue(&group, 0x28, 2, false, false);
        break;
    case 0x14:                                                    // unloadall
        gl->OrderValue(&group, 0x2b, -1, false, false);           // 0x564870(g, 0x2b, -1, 0, 0)
        break;
    default: {
        static bool logged[0x2e];
        if (cmd->Id >= 0 && cmd->Id < 0x2e && !logged[cmd->Id]) {
            logged[cmd->Id] = true;
            Logger.g->Log(1, "STUB: SGameLogic::ExecuteScriptStatement '%s' (0x568bc0) not implemented", cmd->Name);
        }
        break;
    }
    }
    free(group.Units);                                            // the group's SDArray dtor
}

} // namespace pz
