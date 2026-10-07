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
#include "unit.h"
#include "unitextern.h"
#include "pzunitregistry.h"
#include "pz/igepardhd.h"
#include "pz/ipixie.h"

namespace pz {

using m2u::UV;

void ClearFound(SFoundUnits* g, int size);   // triggers.cpp 0x563580
SFoundUnit* AddFound(SFoundUnits* g);        // triggers.cpp 0x560d80 + 0x560790
void GroupStats(SFoundUnits* g);             // triggers.cpp 0x582770
void OrderAlongPath(SGameLogic* gl, SFoundUnits* g, int command, int path);   // triggers.cpp 0x564510

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

// The first live unit whose script id equals the name (SString ==: same
// length, case-insensitive), or -1 (the loops of cases 4, 6, 0x10, 0x13, 0x19).
int UnitByScriptId(const char* name, int len)
{
    PZ_FOR_EACH_UNIT(i) {
        const char* id = UV::ScriptId(i);
        if ((int)strlen(id) == len && (len == 0 || _stricmp(id, name) == 0))
            return i;
    }
    return -1;
}

// The path whose name equals the string (0x5e9840 + strcmp), or -1.
int PathByName(const char* name)
{
    SHeap<SPath>& paths = g_World->Paths;                         // World +0x7494
    for (int i = 0; i < paths.Size; ++i)
        if (paths.IsLive(i) && strcmp(SStr(paths.Array[i].Data.Name), name) == 0)
            return i;
    return -1;
}

// The weather whose name equals the string (0x5e6520: same length,
// case-insensitive), or -1.
int WeatherByName(const char* name)
{
    SWorld* w = g_World;
    int len = (int)strlen(name);
    for (int i = 0; i < w->Weathers.Size; ++i) {
        const SString& n = w->Weathers.Array[i].Name;
        if (n.size == len && (len == 0 || _stricmp(n.buf, name) == 0))
            return i;
    }
    return -1;
}

// The SWorld::CreateUnit 0x5e3170 call of cases 0 / 1 / 0x23: the unit, its
// script id and the crew's ("<id>-crew", stored unit 0).
void CreateScripted(int player, const char* cls, const float* xzdir, int p5, bool crew, const char* passedId,
                    const char* id)
{
    float pos[3] = {xzdir[0], 0.0f, xzdir[1]};
    int u = g_World->CreateUnit(player, cls, pos, xzdir[2] * 0.017453292f, p5, 1.0f, -1, crew, passedId);   // 0x5e3170
    if (u < 0)
        return;
    SUnit* unit = WorldUnit(u);
    unit->ScriptID = id;                                          // 0x52c2c0 (+0x194)
    if (unit->Stored.Size != 0) {                                 // +0x170
        char crewId[160];
        _snprintf(crewId, sizeof(crewId) - 1, "%s-crew", id);     // 0x52c580(id, "-crew")
        crewId[sizeof(crewId) - 1] = 0;
        WorldUnit(unit->Stored.Array[0].Unit)->ScriptID = crewId; // 0x546450(0) -> +0x194 (0x52c320)
    }
}

// PANZERS 0x56d860 (hideallunit) / 0x580320 (showallunit): the infantry,
// vehicles and guns (classes 0, 5, 0xb) that are not inside another unit
// (or are on top of it, +0x7c).
void HideShowAll(bool show)
{
    PZ_FOR_EACH_UNIT(i) {
        int ct = UV::ClassType(i);
        if (ct != 0 && ct != 5 && ct != 0xb)
            continue;
        SUnit* u = WorldUnit(i);
        if (u->Parent >= 0 && !u->_7c)                            // +0x78, +0x7c
            continue;
        if (!show) {
            // PANZERS 0x5c5100
            u->_16a[0] = u->Unplaced;                             // +0x16a = +0x168
            if (!u->Unplaced)
                UV::Iface(i)->Unplace();                          // +0x4c
        } else if (!u->_16a[0]) {
            // PANZERS 0x5c5120
            UV::Iface(i)->Place(u->Pos[0], u->Pos[2], u->Dir);    // +0x50(+0x8c, +0x94, +0xb0)
        }
    }
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
        // "create" (a player 1..12) / "create2" load the unit class
        // (0x5d0e70(name, 1)), "create_model" loads the model once
        // (Gepard +0x20 / +0x24).
        if ((cmd->Id == 0 && in[5] - 1 >= 0 && in[5] - 1 < 0xc) || cmd->Id == 1) {
            if (g_UnitRegistry)
                g_UnitRegistry->GetPUnit(str[1], true);           // 0x5d0e70(str[1], 1)
        } else if (cmd->Id == 0x20) {
            int proto = PzGepard()->LoadModelPrototype(str[0], 0.005f, 0, 0);   // Gepard +0x20 (0x3ba3d70a)
            PzGepard()->ReleaseModelPrototype(proto);             // Gepard +0x24
        }
        free(group.Units);
        return;
    }
    float target[2] = {fl[1], fl[2]};
    switch (cmd->Id) {
    case 0:                                                       // create <id>, <class>, x, z, dir, player
        if (in[5] - 1 >= 0 && in[5] - 1 < 0xc) {
            float xzdir[3] = {fl[2], fl[3], fl[4]};
            CreateScripted(in[5] - 1, str[1], xzdir, 0, true, str[0], str[0]);
        }
        break;
    case 1: {                                                     // create2 <id>, <class>, x, z, dir, p5
        if (strLen[0] < 1)
            Logger.g->Panic("SString::operator[]: invalid index (%d)", 0);
        // HD: the player is (<id> starts with '#'), the script id passed is "".
        float xzdir[3] = {fl[2], fl[3], fl[4]};
        CreateScripted(str[0][0] == '#', str[1], xzdir, in[5], true, "", str[0]);
        break;
    }
    case 4: {                                                     // follow <id>, <id2>
        int t = UnitByScriptId(str[1], strLen[1]);
        if (t >= 0)
            gl->OrderAtUnit(&group, 7, t, false, false);          // 0x564720(g, 7, unit)
        break;
    }
    case 6: {                                                     // attack <id>, <id2>
        int t = UnitByScriptId(str[1], strLen[1]);
        if (t >= 0)
            gl->OrderAtUnit(&group, 0x11, t, false, false);
        break;
    }
    case 0xf:                                                     // gunner_aim_ground <id>, x, z
        gl->OrderAtPoint(&group, 0x22, target, false, false);     // 0x564660(g, 0x22, &t)
        break;
    case 0x10: {                                                  // gunner_aim_unit <id>, <id2>
        int t = UnitByScriptId(str[1], strLen[1]);
        if (t >= 0)
            gl->OrderAtUnit(&group, 0x23, t, false, false);
        break;
    }
    case 0x11:                                                    // gunner_lock
        gl->OrderPlain(&group, 0x24, false, false);
        break;
    case 0x12:                                                    // gunner_change_active <id>, n
        gl->OrderValue(&group, 0x29, in[1], false, false);
        break;
    case 0x13: {                                                  // board <id>, <carrier>
        int t = UnitByScriptId(str[1], strLen[1]);
        if (t < 0)
            break;
        SUnit* c = WorldUnit(t);
        if (!c->Wrecked && (c->Parent < 0 || UV::ClassType(t) == 10) && !c->Unplaced)   // +0x150, +0x78, +0x168
            gl->OrderAtUnit(&group, 0x2a, t, false, false);
        break;
    }
    case 0x15:                                                    // gunner_spin_to_dir <id>, deg
        gl->OrderFloat(&group, 0x25, fl[1] * 0.017453292f, false, false);   // 0x564910
        break;
    case 0x16: {                                                  // fx <file>, x, y, z
        if (!g_Pixie || !g_Scene)
            break;
        int proto = g_Pixie->LoadEffectPrototype(str[0], false, false, 0, 0);   // pixie +0x10(name, 0, 0, "")
        if (proto < 0)
            Logger.g->Panic("SGameLogic::ExecuteScriptStatement: Failed to load effect %s", str[0]);
        float dir[3] = {1.0f, 0.0f, 1.0f};
        float pos[3];
        pos[0] = fl[1];
        pos[1] = g_World->GetTerrainHeight(fl[1], fl[3]) + fl[2]; // 0x5e7730 + y
        pos[2] = fl[3];
        g_Pixie->PlayEffect(g_Scene, proto, pos, dir, 0);         // pixie +0x24 (the prototype is kept)
        break;
    }
    case 0x17:                                                    // change_behavior <id>, n
        gl->OrderValue(&group, 0x21, in[1], false, false);
        break;
    case 0x18:                                                    // anim <id>, <animation>
        for (int k = 0; k < group.Count; ++k)
            UV::Iface(group.Units[k].Unit)->SetSpecialAnimation(str[1]);   // +0x1b8(SString)
        break;
    case 0x19: {                                                  // tow <id>, <id2>
        int t = UnitByScriptId(str[1], strLen[1]);
        if (t >= 0)
            gl->OrderAtUnit(&group, 0x2c, t, false, false);
        break;
    }
    case 0x1a:                                                    // untow
        gl->OrderPlain(&group, 0x2d, false, false);
        break;
    case 0x1b:                                                    // heavy_bombardment x, z, player, alt, flag
        gl->SupportHeavyBomber(true, fl[0], fl[1], in[2] - 1, fl[3], in[4] != 0, false, 0.0f);   // 0x567760
        break;
    case 0x1c:                                                    // tactical_bombardment x, z, player
        gl->SupportTacBomber(true, fl[0], fl[1], in[2] - 1);      // 0x568740
        break;
    case 0x1d:                                                    // recon_plane x, z, player, alt, flag
        gl->SupportRecon(true, fl[0], fl[1], in[2] - 1, fl[3], in[4] != 0);   // 0x568300
        break;
    case 0x1e:                                                    // parachute x, z, player, alt, flag
        gl->SupportParatroopers(true, fl[0], fl[1], in[2] - 1, fl[3], in[4] != 0, false, 0.0f);   // 0x567d40
        break;
    case 0x1f:                                                    // cannonade x, z, player
        gl->SupportArtillery(true, fl[0], fl[1], in[2] - 1);      // 0x5674c0
        break;
    case 0x20:                                                    // create_model <file>, x, z, dir
        gl->CreateAnimatedModel(str[0], fl[1], g_World->GetTerrainHeight(fl[1], fl[2]), fl[2],
                                fl[3] * 0.017453292f);            // 0x5649e0
        break;
    case 0x21:                                                    // move_to_dir <id>, x, z, deg
        gl->MoveFoundUnitsToLocationDir(&group, 2, target, fl[3] * 0.017453292f, false, false, false);   // 0x57f200
        break;
    case 0x22:                                                    // sethp <id>, percent (0x57fa10)
        for (int k = 0; k < group.Count; ++k)
            UV::Iface(group.Units[k].Unit)->SetHealthPercent(fl[1]);   // +0x130
        break;
    case 0x23:                                                    // createwithoutcrew
        if (in[5] - 1 >= 0 && in[5] - 1 < 0xc) {
            float xzdir[3] = {fl[2], fl[3], fl[4]};
            CreateScripted(in[5] - 1, str[1], xzdir, 0, false, "", str[0]);
        }
        break;
    case 0x24:                                                    // hideallunit (0x56d860)
        HideShowAll(false);
        break;
    case 0x25:                                                    // showallunit (0x580320)
        HideShowAll(true);
        break;
    case 0x26:                                                    // disableai
    case 0x27:                                                    // enableai: both set +0x1d4 in HD
        for (int k = 0; k < group.Count; ++k)
            WorldUnit(group.Units[k].Unit)->_1d4 = true;
        break;
    case 0x28:                                                    // attack_move_on_path <id>, <path>
    case 0x29: {                                                  // move_units_on_path <id>, <path>
        int path = PathByName(str[1]);
        if (path < 0) {
            Logger.g->Warning("SGameLogic::ExecuteScriptStatement: path %s not found", str[1]);   // 0x65cac0
            break;
        }
        OrderAlongPath(gl, &group, cmd->Id == 0x28 ? 10 : 6, path);   // 0x564510
        break;
    }
    case 0x2a: {                                                  // move_units_on_path_in_convoy <id>, <path>
        int path = PathByName(str[1]);
        if (path < 0) {
            Logger.g->Warning("SGameLogic::ExecuteScriptStatement: path %s not found", str[1]);
            break;
        }
        SPath& pp = g_World->Paths.Array[path].Data;              // 0x560960
        if (pp.Points.Size < 1)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SPathPoint", 0);
        int g = gl->GroupOrder(true, 0, &group, true, pp.Points.Array[0].X, pp.Points.Array[0].Z);   // 0x56ff30
        gl->ConvoyAlongPath(g, path);                             // 0x57e600
        break;
    }
    case 0x2b: {                                                  // setweather <name>
        int wi = WeatherByName(str[0]);                           // 0x5e6520
        if (wi != -1)
            g_World->SetWeather(wi, 0);                           // 0x5fdc80(i, 0) and 0x6088f0
        break;
    }
    case 0x2c:                                                    // disable_ambient_sounds
        g_World->StopAmbientSounds();                             // 0x5f5140
        break;
    case 0x2d:                                                    // enable_ambient_sounds
        g_World->StartEffects();                                  // 0x5f5b50
        break;
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
