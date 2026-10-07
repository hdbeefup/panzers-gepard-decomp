// panzers/cheats.cpp
// The single-player cheat codes. HD SGameView::OnKeyDown 0x622f50, Enter
// with the chat line open (chatline.cpp) and no SMulti (DAT_008f1a74 == 0)
// or SMulti +0x512c set: the line's text is compared with SString ==
// (0x52c410: case-insensitive _stricmp, an empty text equals nothing) in
// this order, the first match wins:
//
//   SheepInTheTrees, FreeWestMemphis3  "Cheat enabled (godmode)": toggles
//       World +0x4d0, the flag cut-scenes set (no fog of war: every unit
//       and building interior shown).
//   TheFunniestJokeInTheWorld  the Monty Python joke as a message; every
//       placed, unmounted unit the local player sees whose owner's player
//       record +0x04 is 0 dies (unit +0x124 EC_Die).
//   SpotTheBraincell [n]  (the first 16 letters) "Cheat enabled
//       (experience)": the selected units get n experience (1000 when no
//       number follows; unit +0x8c AddXP(-1, n, 1)).
//   MoneySong  "Cheat enabled (+1000 prestige)" (campaign 0x597500(-1000)).
//   TheSpanishInquisition, Inferno  "Cheat enabled (instant kill)":
//       SGameLogic +0x2d8 (combat.cpp TakeDamage 0x5c4080 reads it via
//       0x5b9e40).
//   BicycleRepairMan  "Cheat enabled (unlimited cargo)": SGameLogic +0x2d9
//       (0x548bf0: supply and repair cost no cargo).
//   SelfDefenceAgainstFreshFruit, Kilmister  "Cheat enabled
//       (invulnerability)": SGameLogic +0x2da (0x61f430; crew damage
//       0x5c3dd0, TakeDamage 0x5c4080, squad EC_Die 0x598280 skip the local
//       player's units).
//   MrHilter  "Cheat enabled (more outside support)": +100 to the local
//       player's five support counters (World +0x19c..+0x1ac).
//   DirtyHungarianPhrasebook  "Cheat enabled (win map)": mission result 1.
//
// Each one that takes effect sets the campaign's cheat state (0x597180(1),
// +0xb84: no medals / no high score in results.cpp). Instant kill, cargo
// and invulnerability do nothing while already on (the logic flag with the
// game not paused). Godmode reports "enabled" also when it switches off.

#include <stdlib.h>
#include <string.h>
#include "cheats.h"
#include "campaign.h"
#include "gamelogic.h"
#include "worldapi.h"
#include "world.h"
#include "unit.h"
#include "cutscene.h"
#include "gettext.h"
#include "logger.h"

namespace {

const char* Gv(const char* id) { return GetText("panzers/GameView.cpp", id); }   // 0x660c50

// SString == const char* 0x52c410: equal ignoring case; an empty string
// equals only an empty one.
bool StrEq(const char* a, int alen, const char* b)
{
    if (alen == 0)
        return !b || !*b;
    return b && *b && _stricmp(a, b) == 0;
}

// SString::Mid 0x5336b0 (start, count) compared with ==.
bool MidEq(const char* text, int len, int start, int count, const char* b)
{
    char buf[0x100];
    int n = 0;
    if (start < 0)
        start = 0;
    if (start < len && count != 0) {
        n = (count < 0 || len <= start + count) ? len - start : count;
        if (n > (int)sizeof(buf) - 1)
            n = (int)sizeof(buf) - 1;
        memcpy(buf, text + start, n);
    }
    buf[n] = 0;
    return StrEq(buf, n, b);
}

int MidInt(const char* text, int len, int start)                  // atoi(Mid(start, len))
{
    if (start >= len)
        return 0;
    return atoi(text + start);
}

int& PlayerInt(int p, int off) { return *(int*)&pz::g_World->Players[p][off]; }

void Enabled(const char* id)
{
    pz::PzMessageFading(pz::g_GameLogic, Gv(id), 1);              // SGameLogic 0x56a480(text, 1)
    PzCampaignSetCheated(pz::g_Campaign, 1);                      // 0x597180(1)
}

bool LogicFlag(int off)                                           // 0x5b9e40 / 0x548bf0 / 0x61f430
{
    return ((unsigned char*)pz::g_GameLogic)[off] != 0 && !pz::g_GameLogic->IsPaused();
}

} // namespace

// PANZERS 0x597180
void PzCampaignSetCheated(pz::SPanzersCampaign* c, int v)
{
    if (c && c->_b84 != 2)
        c->_b84 = v;
}

// PANZERS 0x622f50 (the cheat part of the Enter case, 0x623859..0x623fcb)
void PzCheatRun(const char* text)
{
    using namespace pz;
    if (!g_World || !g_GameLogic || !g_Campaign || !text)
        return;
    int len = (int)strlen(text);
    SWorld* w = g_World;
    SUnitHeap& units = w->Units;                                  // World +0x4d4
    if (StrEq(text, len, "SheepInTheTrees") || StrEq(text, len, "FreeWestMemphis3")) {
        Enabled("Cheat enabled (godmode)");
        w->_4d0[0] = w->_4d0[0] == 0;                             // World +0x4d0
        if (Logger.g)
            Logger.g->Log(0, "PZM5 CHEAT godmode World+0x4d0=%d", w->_4d0[0]);
        return;
    }
    if (StrEq(text, len, "TheFunniestJokeInTheWorld")) {
        PzMessageFading(g_GameLogic, "Wenn ist das Nunstuck git und Slotermeyer?\n"
                                     "Ja! Beiherhund das oder die Flipperwald gersput!", 1);
        PzCampaignSetCheated(g_Campaign, 1);
        int killed = 0;
        for (int i = 0; i < units.Size; ++i) {                    // 0x56b3b0 NextLive
            if (!units.IsLive(i))
                continue;
            SUnit* u = units.Array[i].Unit;                       // 0x546490
            if (PlayerInt(u->Player, 0x04) != 0)                  // World +0x174 + owner * 0x48
                continue;
            if (g_GameLogic->CanSeeGroundUnit(w->LocalPlayer, u) && u->Unplaced == 0 && u->Parent < 0) {
                u->EC_Die();                                      // vtbl +0x124
                ++killed;
            }
        }
        if (Logger.g)
            Logger.g->Log(0, "PZM5 CHEAT joke killed %d", killed);
        return;
    }
    if (MidEq(text, len, 0, 0x10, "SpotTheBraincell")) {
        Enabled("Cheat enabled (experience)");
        int xp = 1000;
        // HD also reads "smarten n" and "beautiful mind n"; they cannot
        // match after the SpotTheBraincell test above (kept as in HD).
        if (MidEq(text, len, 0, 7, "smarten") && len > 8)
            xp = MidInt(text, len, 8);
        if (MidEq(text, len, 0, 0xe, "beautiful mind") && len > 0xf)
            xp = MidInt(text, len, 0xf);
        if (MidEq(text, len, 0, 0x10, "SpotTheBraincell") && len > 0x11)
            xp = MidInt(text, len, 0x11);
        int n = 0;
        for (int i = 0; i < units.Size; ++i) {
            if (!units.IsLive(i))
                continue;
            SUnit* u = units.Array[i].Unit;
            if (u->_104 & 1) {                                    // selected
                u->AddXP(-1, (float)xp, 1);                       // vtbl +0x8c
                ++n;
            }
        }
        if (Logger.g)
            Logger.g->Log(0, "PZM5 CHEAT experience %d to %d units", xp, n);
        return;
    }
    if (StrEq(text, len, "MoneySong")) {
        Enabled("Cheat enabled (+1000 prestige)");
        g_Campaign->Prestige -= -1000;                            // 0x597500(-1000): +0x38 -= p
        if (Logger.g)
            Logger.g->Log(0, "PZM5 CHEAT prestige %d", g_Campaign->Prestige);
        return;
    }
    if (StrEq(text, len, "TheSpanishInquisition") || StrEq(text, len, "Inferno")) {
        if (!LogicFlag(0x2d8)) {                                  // 0x5b9e40
            Enabled("Cheat enabled (instant kill)");
            ((unsigned char*)g_GameLogic)[0x2d8] = 1;
            if (Logger.g)
                Logger.g->Log(0, "PZM5 CHEAT instant kill");
        }
        return;
    }
    if (StrEq(text, len, "BicycleRepairMan")) {
        if (!LogicFlag(0x2d9)) {                                  // 0x548bf0
            Enabled("Cheat enabled (unlimited cargo)");
            ((unsigned char*)g_GameLogic)[0x2d9] = 1;
            if (Logger.g)
                Logger.g->Log(0, "PZM5 CHEAT unlimited cargo");
        }
        return;
    }
    if (StrEq(text, len, "SelfDefenceAgainstFreshFruit") || StrEq(text, len, "Kilmister")) {
        if (!LogicFlag(0x2da)) {                                  // 0x61f430
            Enabled("Cheat enabled (invulnerability)");
            ((unsigned char*)g_GameLogic)[0x2da] = 1;
            if (Logger.g)
                Logger.g->Log(0, "PZM5 CHEAT invulnerability");
        }
        return;
    }
    if (StrEq(text, len, "MrHilter")) {
        int p = w->LocalPlayer;                                   // World +0x16c
        PlayerInt(p, 0x30) += 100;                                // World +0x1a0
        PlayerInt(p, 0x2c) += 100;                                // World +0x19c
        PlayerInt(p, 0x38) += 100;                                // World +0x1a8
        PlayerInt(p, 0x3c) += 100;                                // World +0x1ac
        PlayerInt(p, 0x34) += 100;                                // World +0x1a4
        Enabled("Cheat enabled (more outside support)");
        if (Logger.g)
            Logger.g->Log(0, "PZM5 CHEAT support %d %d %d %d %d", PlayerInt(p, 0x2c), PlayerInt(p, 0x30),
                          PlayerInt(p, 0x34), PlayerInt(p, 0x38), PlayerInt(p, 0x3c));
        return;
    }
    if (StrEq(text, len, "DirtyHungarianPhrasebook")) {
        // HD then tests Mid(0, 4) == "jump" with a number at 5 ("German %d" /
        // "Allied %d" / "Russian %d" into the campaign's +0xd8): it cannot
        // match the whole text "DirtyHungarianPhrasebook" (dead in HD).
        Enabled("Cheat enabled (win map)");
        g_Campaign->SetMissionResult(1);                          // 0x597470(1)
        if (Logger.g)
            Logger.g->Log(0, "PZM5 CHEAT win map");
        return;
    }
}
