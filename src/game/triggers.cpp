// src/game/triggers.cpp
// SGameLogic trigger runtime: trigger variables, event dispatch, active
// locations, conditions and the running triggers (RunTriggers), plus the
// SWorld trigger-variable slots and the two SWorld per-tick rows of agent L.
// OWNER: agent L. Lifted from the HD exe.
//
// Data: World+0x7474 triggers, +0x7480 locations, +0x7494 paths, +0x74a8
// variables (src/world/trigger.cpp loads them). Runtime: SGameLogic+0x268
// running triggers, +0x2f0 active locations.

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "gamelogic.h"
#include "trigger.h"
#include "triggersunits.h"
#include "worldapi.h"
#include "world.h"
#include "pzunitregistry.h"
#include "blockmaprefresh.h"
#include "unit.h"
#include "gunner.h"
#include "buildingunit.h"
#include "unitextern.h"
#include "logger.h"
#include "stub_log.h"
#include "campaign.h"
#include "cutscene.h"
#include "pz/ipixie.h"
#include "pz/imodel.h"
#include "gettext.h"
#include "core_common.h"
#include "stream.h"

void PzPlayMusicTrack(const char* track);   // src/panzers/results.cpp: Concert +0x80(0), +0x6c, +0x70, +0x78(0)

namespace pz {

using m2u::UV;

void UnitSetPlayer(SUnit* u, int player);                         // 0x5c1630 (below)
void PzMessagesClearStatic(SGameLogic* gl);                       // tutorial_msg.cpp 0x5638b0
void PzMessageFading(SGameLogic* gl, const char* text, int color);// tutorial_msg.cpp 0x56a480
void PzTriggerTextBlock(SGameLogic* gl, const char* key, bool fading);   // tutorial_msg.cpp (0x40 / 0x41)
void PzSpeechQueue(const char* file);                             // tutorial_msg.cpp 0x600770
void PzSpeechTick();                                              // tutorial_msg.cpp 0x607f50 (trigger part)

// Trigger trace (recompile only): one line per started trigger and per
// executed action, with the tick, so the 90 s loop can be compared with the
// original timing. On with -m2 (PZ_M2_TRIGTRACE=0 turns it off).
static bool TrigTrace()
{
    static int on = -1;
    if (on < 0) {
        const char* e = getenv("PZ_M2_TRIGTRACE");
        on = e ? (e[0] != '0') : (g_M2.Enabled ? 1 : 0);
    }
    return on != 0 && Logger.g;
}

static STriggerArray<STrigger>* Triggers()
{
    return reinterpret_cast<STriggerArray<STrigger>*>(&g_World->Triggers);
}

static STrigger* TriggerAt(int index)
{
    STriggerArray<STrigger>* t = Triggers();
    if (index < 0 || index >= t->Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "STrigger", index);
    return &t->Array[index];
}

static SLocation* LocationAt(int index)
{
    SHeap<SLocation>& h = g_World->Locations;
    if (!h.IsLive(index))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SLocation", index);
    return &h.Array[index].Data;
}

// ---------------------------------------------------------------------------
// SWorld: trigger variables (HD SWorld vtbl +4 / +8)

// PANZERS 0x5ec2a0
int SWorld::GetTriggerVariableValue(int index, bool special)
{
    if (TriggerVariables.IsLive(index))
        return TriggerVariables.Array[index].Data.Value;
    const char* msg;
    if (!special) {
        msg = "SWorld::GetTriggerVariableValue(): Invalid variable index.";
    } else {
        // Support counters of the players (0x40000014.. in blocks of 4):
        // cannonade, recon plane, tactical bomber, heavy bomber, parachute.
        // Player p's block is at World+0x170 + p*0x48 + 0x10..0x20.
        unsigned u = (unsigned)index;
        if (u - 0x40000014u < 4) return *(int*)((unsigned char*)this - 0x404 + index * 0x48);
        if (u - 0x40000018u < 4) return *(int*)((unsigned char*)this - 0x520 + index * 0x48);
        if (u - 0x4000001cu < 4) return *(int*)((unsigned char*)this - 0x63c + index * 0x48);
        if (u - 0x40000020u < 4) return *(int*)((unsigned char*)this - 0x758 + index * 0x48);
        if (u - 0x40000024u < 4) return *(int*)((unsigned char*)this - 0x874 + index * 0x48);
        msg = "SWorld::GetTriggerVariableValue(): Invalid special variable index.";
    }
    Logger.g->Warning("%s", msg);                                 // 0x65cac0
    return 0;
}

// PANZERS 0x5fec80
void SWorld::SetTriggerVariableValue(int index, int value, bool special)
{
    if (TriggerVariables.IsLive(index)) {
        TriggerVariables.Array[index].Data.Value = value;
        return;
    }
    if (!special)
        Logger.g->Panic("SWorld::SetTriggerVariableValue(): Invalid variable index.");
    unsigned u = (unsigned)index;
    unsigned char* b = (unsigned char*)this;
    if (u - 0x40000014u < 4) { *(int*)(b - 0x404 + index * 0x48) = value; return; }
    if (u - 0x40000018u < 4) { *(int*)(b - 0x520 + index * 0x48) = value; return; }
    if (u - 0x4000001cu < 4) { *(int*)(b - 0x63c + index * 0x48) = value; return; }
    if (u - 0x40000020u < 4) { *(int*)(b - 0x758 + index * 0x48) = value; return; }
    if (u - 0x40000024u < 4) { *(int*)(b - 0x874 + index * 0x48) = value; return; }
    Logger.g->Panic("SWorld::SetTriggerVariableValue(): Invalid special variable index.");
}

// PANZERS 0x607f50 (menu path)
// Plays the queued speech (World+0x726c SHeap, element 0x38) through the
// Concert, by priority and time. The menu queues no speech, so with the
// queue empty only the guards run.
void SWorld::UpdateSpeech()
{
    PZ_M2_TRACE("SWorld::UpdateSpeech (0x607f50)");
    if (!g_GameLogic || g_GameLogic->IsPaused())
        return;
    PzSpeechTick();                                               // +0x7280 trigger speech (tutorial_msg.cpp, M4)
    const int* q = (const int*)((const unsigned char*)this + 0x726c);   // {array, size, max, free, count}
    if (q[1] > 0 && q[4] > 0) {
        static bool once;
        if (!once && Logger.g) {
            once = true;
            Logger.g->Warning("SWorld::UpdateSpeech: %d queued speech entries not played (0x607f50 not lifted)", q[4]);
        }
    }
}

// HD 0x604620; the body is P's BlockMap_RefreshDirtyRect (blockmaprefresh.cpp,
// which carries the marker).
// Rebuilds the block-map flags of the dirty rectangle (World+0x7504..0x7510,
// mask +0x7514) from the terrain layers, the effects and the units. It only
// runs when something marked the rectangle dirty (+0x7500).
void SWorld::RefreshBlockMapDirtyRect()
{
    PZ_M2_TRACE("SWorld::RefreshBlockMapDirtyRect (0x604620)");
    BlockMap_RefreshDirtyRect(this);
}

// ---------------------------------------------------------------------------
// Found-unit groups

// PANZERS 0x563580 (SDArray<SFoundUnit>::Clear(n))
void ClearFound(SFoundUnits* g, int size)
{
    if (g->Count != 0 && g->Units == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "SFoundUnit");
    g->Count = size;
    if (g->Max < size) {
        g->Max = size;
        g->Units = (SFoundUnit*)realloc(g->Units, size * sizeof(SFoundUnit));
    }
    if (g->Max > 0)
        memset(g->Units, 0, g->Max * sizeof(SFoundUnit));
}

// PANZERS 0x560d80 + 0x560790: append, returns the new element
SFoundUnit* AddFound(SFoundUnits* g)
{
    if (g->Count == g->Max) {
        int nmax = g->Max < 0x10 ? 0x10 : (g->Max * 6) / 5;
        g->Units = (SFoundUnit*)realloc(g->Units, nmax * sizeof(SFoundUnit));
        memset(g->Units + g->Max, 0, (nmax - g->Max) * sizeof(SFoundUnit));
        g->Max = nmax;
    }
    return &g->Units[g->Count++];
}

// PANZERS 0x5604d0 (copy)
static void CopyFound(SFoundUnits* dst, const SFoundUnits* src)
{
    dst->X = src->X;
    dst->Z = src->Z;
    dst->RadiusSq = src->RadiusSq;
    dst->Kind = src->Kind;
    dst->Leader = src->Leader;
    ClearFound(dst, src->Count);
    for (int i = 0; i < dst->Count; ++i)
        dst->Units[i] = src->Units[i];
}

static int FoundUnit(const SFoundUnits* g, int k)
{
    if (k < 0 || k >= g->Count)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SFoundUnit", k);
    int u = g->Units[k].Unit;
    if (!m2u::IsLive(u))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", u);
    return u;
}

// PANZERS 0x582770
// Centre, spread and leader of a found-unit group. The leader is the first
// unit of the best class: tanks with a crew member of kind 2 (4), other
// tanks (2), units with SPUnit +0x34 >= 0 (3), the rest (1, not unit types
// 0x10 / 0x17), else the first unit (0).
void GroupStats(SFoundUnits* g)
{
    float sx = 0.0f, sz = 0.0f;
    int prio = -1;
    g->Leader = -1;
    g->Kind = 0xff;
    for (int j = 0; j < g->Count; ++j) {
        int u = FoundUnit(g, j);
        sx = sx + UV::X(u);
        sz = sz + UV::Z(u);
        g->Units[j].Flag = false;
        if (prio < 0) {
            prio = 0;
            g->Leader = u;
        }
        if (UV::ClassType(u) == 0) {
            if (UV::PUnitByte(u, 0xc8) == 0 && UV::StoredCount(u) > 0) {
                for (int k = 0; k < UV::StoredCount(u); ++k) {
                    if (UV::StoredKind(u, k) == 2) {
                        prio = 4;
                        g->Leader = u;
                    }
                }
            }
            if (prio < 2) {
                prio = 2;
                g->Leader = u;
            }
        } else if (UV::PUnitInt(u, 0x34) < 0 || prio > 2) {
            if (prio < 1 && UV::UnitType(u) != 0x10 && UV::UnitType(u) != 0x17) {
                prio = 1;
                g->Leader = u;
            }
        } else {
            prio = 3;
            g->Leader = u;
        }
    }
    int n = g->Count;
    g->RadiusSq = 0.0f;
    double inv = 1.0 / (double)n;                                 // _DAT_007eed98 = 1.0
    sz = (float)inv * sz;
    sx = (float)inv * sx;
    g->Z = sz;
    g->X = sx;
    if (n > 1) {
        for (int j = 0; j < n; ++j) {
            int u = FoundUnit(g, j);
            float dx = sx - UV::X(u);
            float dz = sz - UV::Z(u);
            float d = dx * dx + dz * dz;
            if (d > g->RadiusSq)
                g->RadiusSq = d;
        }
        if ((float)((n * 0x10 - 0x10) * (n - 1)) < g->RadiusSq) {
            g->RadiusSq = 1048576.0f;
            return;
        }
        g->RadiusSq = g->RadiusSq * 4.0f;                         // DAT_007f4588
    }
}

// ---------------------------------------------------------------------------
// Condition helpers

// PANZERS 0x549ab0
static bool SameSide(int a, int b)
{
    int team = *(int*)(g_World->Players[a] + 0x0c);               // World+0x17c + a*0x48
    if (team != 0)
        return team == *(int*)(g_World->Players[b] + 0x0c);
    return a == b;
}

// PANZERS 0x5817f0
// Player selector of conditions: 0..11 that player, 12..23 the allies of
// player n-12, 24..35 the enemies of player n-24, 36 the triggering unit's
// player, 37 a "type 2" player slot, 38 any player, 39 the second unit's.
static bool PlayerMatches(int unitPlayer, int sel, const SRunningTrigger* rt)
{
    // HD 0x594cd0: in coop multiplayer (SMulti mode 3) player 7 counts as 0.
    if (sel < 0xc)
        return unitPlayer == sel;
    if (sel < 0x18) {
        int team = *(int*)(g_World->Players[sel - 0xc] + 0x0c);
        if (team != 0)
            return team == *(int*)(g_World->Players[unitPlayer] + 0x0c);
        return sel - 0xc == unitPlayer;
    }
    if (sel > 0x23) {
        int u;
        if (sel == 0x24) {
            u = rt->Unit;
            if (!m2u::IsLive(u))
                return false;
        } else if (sel == 0x27) {
            u = rt->Unit2;
            if (!m2u::IsLive(u))
                return false;
        } else if (sel == 0x25) {
            return *(int*)(g_World->Players[unitPlayer] + 0x08) == 2;    // World+0x178
        } else {
            return sel == 0x26;
        }
        return unitPlayer == UV::Player(u);
    }
    return !SameSide(sel - 0x18, unitPlayer);
}

// PANZERS 0x581ae0
// Location selector: 0 = anywhere, n = location n-1 (strictly inside).
static bool InLocation(int unit, int sel)
{
    if (sel == 0)
        return true;
    int loc = sel - 1;
    if (!g_World->Locations.IsLive(loc))
        return false;
    if (!m2u::IsLive(unit))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", unit);
    const SLocation* l = LocationAt(loc);
    float x = UV::X(unit), z = UV::Z(unit);
    return (float)l->X1 < x && x < (float)l->X2 && (float)l->Z1 < z && z < (float)l->Z2;
}

// PANZERS 0x581c50
// Unit selector: 0 any, 1 infantry-like unit types, 2 buildings, 3 the
// named type, 4 unit types 0xe..0x18, 5 unit types 8, 9, 0xb.
static bool TypeMatches(int unit, int sel, const char* name)
{
    if (!m2u::IsLive(unit))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", unit);
    int ct = UV::ClassType(unit);
    if (ct == 8 || ct == 0xd)
        return false;
    if (ct == 3 || (ct == 7 && UV::UnitType(unit) != 0x19))
        return false;
    int ut = UV::UnitType(unit);
    switch (sel) {
    case 0:
        return true;
    case 1:
        return ut == 0 || ut == 1 || ut == 2 || ut == 4 || ut == 3 || ut == 7 || ut == 6;
    case 2:
        return ct == 9;
    case 3:
        return _stricmp(UV::TypeName(unit), name) == 0;
    case 4:
        return ut >= 0xe && ut <= 0x18;
    case 5:
        return ut == 9 || ut == 8 || ut == 0xb;
    default:
        return false;
    }
}

// The common filter of the "find units" conditions (2, 0xc, 0xd).
static bool Findable(int u)
{
    return !UV::B110(u) && !UV::B150(u) && (UV::Container(u) < 0 || UV::B7c(u)) && !UV::Unplaced(u);
}

// Condition 0xc / 0xe name test: '*' at the end is a prefix wildcard.
static bool ScriptIdMatches(int u, const SString& pattern)
{
    if (pattern.size == 0)
        Logger.g->Panic("SString::operator[]: invalid index (%d)", -1);
    if (pattern.buf[pattern.size - 1] == '*')
        return strncmp(UV::ScriptId(u), SStr(pattern), pattern.size - 1) == 0;
    // HD 0x55cbd0: the unit's script id equals the pattern.
    return _stricmp(UV::ScriptId(u), SStr(pattern)) == 0;
}

// ---------------------------------------------------------------------------
// Events

// Builds the running-trigger candidate on the stack, as the HD dispatchers
// do (memset 0x34), and frees its found-unit array afterwards.
static void DispatchEvent(SGameLogic* gl, int type, int param, int unit)
{
    SRunningTrigger rt;
    memset(&rt, 0, sizeof(rt));
    STriggerArray<STrigger>* t = Triggers();
    for (int i = 0; i < t->Size; ++i) {
        if (t->Array[i].Event.Type != type)
            continue;
        if (type != 0 && t->Array[i].Event.Param != param)
            continue;
        rt.Trigger = i;
        rt.Unit = unit;
        rt.Unit2 = -1;
        gl->CheckConditions(&rt);
        t = Triggers();
    }
    free(rt.Found.Units);
}

// PANZERS 0x570cc0
void SGameLogic::DispatchEverySecond()
{
    PZ_M2_TRACE("SGameLogic::DispatchEverySecond (0x570cc0)");
    SHeap<STriggerVariable>& v = g_World->TriggerVariables;
    for (int i = 0; i < v.Size; ++i)
        if (v.Array[i].Next == kHeapLive)
            v.Array[i].Data.Value += v.Array[i].Data.Step;
    DispatchEvent(this, TE_EVERY_SECOND, -1, -1);
}

// PANZERS 0x571280
void SGameLogic::DispatchEnterLocation(int unit, int location)
{
    PZ_M2_TRACE("SGameLogic::DispatchEnterLocation (0x571280)");
    DispatchEvent(this, TE_ENTERS_LOCATION, location, unit);
}

// PANZERS 0x571470
void SGameLogic::DispatchLeaveLocation(int unit, int location)
{
    PZ_M2_TRACE("SGameLogic::DispatchLeaveLocation (0x571470)");
    DispatchEvent(this, TE_LEAVES_LOCATION, location, unit);
}

// PANZERS 0x571380
// Event 5 (a unit got in a vehicle, SUnit::StoreUnit): every trigger of that
// event type, whatever its parameter, with Unit = p1 and Unit2 = p2.
void SGameLogic::Dispatch_571380(int p1, int p2)
{
    PZ_M2_TRACE("SGameLogic::Dispatch_571380 (0x571380)");
    SRunningTrigger rt;
    memset(&rt, 0, sizeof(rt));                                   // memset 0x34
    STriggerArray<STrigger>* t = Triggers();
    for (int i = 0; i < t->Size; ++i) {
        if (t->Array[i].Event.Type != 5)
            continue;
        rt.Unit = p1;                                             // +0x0c
        rt.Unit2 = p2;                                            // +0x10
        rt.Trigger = i;
        CheckConditions(&rt);                                     // 0x580600
        t = Triggers();
    }
    free(rt.Found.Units);
}

// PANZERS 0x5640b0
// The locations named by enter/leave events of enabled triggers (flag 2).
void SGameLogic::CollectActiveLocations()
{
    // HD 0x546af0(0): clear the SDArray<int>.
    ActiveLocationCount = 0;
    if (ActiveLocationMax > 0)
        memset(ActiveLocations, 0, ActiveLocationMax * sizeof(int));
    STriggerArray<STrigger>* t = Triggers();
    for (int i = 0; i < t->Size; ++i) {
        const STrigger& tr = t->Array[i];
        if (tr.Event.Type != TE_ENTERS_LOCATION && tr.Event.Type != TE_LEAVES_LOCATION)
            continue;
        if (tr.Flags != 2)
            continue;
        int k = 0;
        while (k < ActiveLocationCount && ActiveLocations[k] != tr.Event.Param)
            ++k;
        if (k < ActiveLocationCount)
            continue;
        if (ActiveLocationCount == ActiveLocationMax) {
            int nmax = ActiveLocationMax < 0x10 ? 0x10 : (ActiveLocationMax * 6) / 5;
            ActiveLocations = (int*)realloc(ActiveLocations, nmax * sizeof(int));
            memset(ActiveLocations + ActiveLocationMax, 0, (nmax - ActiveLocationMax) * sizeof(int));
            ActiveLocationMax = nmax;
        }
        ActiveLocations[ActiveLocationCount++] = tr.Event.Param;
    }
    if (Logger.g)
        Logger.g->Log(1, "Found %d active locations", ActiveLocationCount);
    if (ActiveLocationCount > 0x20)
        Logger.g->Panic("SGameLogic::CollectActiveLocations: Too many active locations! (%d)", ActiveLocationCount);
}

// HD 0x582080 (entry with the unit pointer)
void SGameLogic::UpdateActiveLocations(SIUnit* unit, bool dispatch)
{
    // HD takes the unit; the recompile looks up its heap slot (+0x74).
    int index = -1;
    if (UV::kReal && unit)
        index = *(const int*)((const unsigned char*)(void*)unit + 0x74);
    if (index >= 0)
        UpdateActiveLocationsAt(index, dispatch);
}

// PANZERS 0x582080 (by heap index)
// Bit k of the unit's +0x258 is set while the unit is strictly inside
// active location k. With dispatch, a changed bit sends event 2 (entered)
// or 3 (left) for that location.
void SGameLogic::UpdateActiveLocationsAt(int unit, bool dispatch)
{
    PZ_M2_TRACE("SGameLogic::UpdateActiveLocations (0x582080)");
    unsigned bits = 0;
    float x = UV::X(unit), z = UV::Z(unit);
    for (int k = 0; k < ActiveLocationCount; ++k) {
        int loc = ActiveLocations[k];
        if (!g_World->Locations.IsLive(loc))
            Logger.g->Panic("SGameLogic::UpdateActiveLocations() Invalid active location");
        const SLocation* l = LocationAt(loc);
        if ((float)l->X1 < x && x < (float)l->X2 && (float)l->Z1 < z && z < (float)l->Z2)
            bits |= 1u << (k & 0x1f);
    }
    unsigned& cur = UV::ActiveLocations(unit);
    unsigned changed = cur ^ bits;
    if (dispatch && changed) {
        int wi = UV::WorldIndex(unit);
        for (int k = 0; k < ActiveLocationCount; ++k) {
            unsigned b = 1u << (k & 0x1f);
            if (!(changed & b))
                continue;
            if (bits & b)
                DispatchEnterLocation(wi, ActiveLocations[k]);    // 0x571280
            else
                DispatchLeaveLocation(wi, ActiveLocations[k]);    // 0x571470
        }
    }
    UV::ActiveLocations(unit) = bits;
}

// ---------------------------------------------------------------------------
// Conditions

// PANZERS 0x580600
void SGameLogic::CheckConditions(SRunningTrigger* rt)
{
    PZ_M2_TRACE("SGameLogic::CheckConditions (0x580600)");
    STrigger* t = TriggerAt(rt->Trigger);
    if (t->Flags != 2 || Flag288 || Flag2b8)
        return;
    for (int k = 0; k < t->Conditions.Size; ++k) {
        t = TriggerAt(rt->Trigger);
        const STriggerCondition& c = t->Conditions.Array[k];
        switch (c.Type) {
        case 0:
            break;
        default:                                                  // 1 and unknown types: never true
            return;
        case TC_FIND_UNITS_AT: {                                  // 2
            ClearFound(&rt->Found, 0);
            PZ_FOR_EACH_UNIT(i) {
                if (!Findable(i))
                    continue;
                if (PlayerMatches(UV::Player(i), c.Player, rt) && InLocation(i, c.Location) &&
                    TypeMatches(i, c.UnitType, SStr(c.UnitClass)))
                    AddFound(&rt->Found)->Unit = i;
            }
            GroupStats(&rt->Found);
            break;
        }
        case TC_COMPARE_VARIABLE: {                               // 3
            int v = c.P200[0];
            int value;
            if (g_World->TriggerVariables.IsLive(v)) {
                value = g_World->TriggerVariables.Array[v].Data.Value;   // 0x5609b0 +8
            } else if (v == 0x40000000) {
                value = rt->Found.Count;                          // number of found units
            } else if (v == 0x4000000c || v == 0x40000010) {
                // the common +0x250 / +0xe0 of the found units, -1 if mixed
                int common = -1;
                for (int j = 0; j < rt->Found.Count; ++j) {
                    int u = FoundUnit(&rt->Found, j);
                    int f = UV::RawInt(u, v == 0x4000000c ? 0x250 : 0xe0);
                    if (common != f)
                        common = common == -1 ? f : -2;
                }
                value = common >= 0 ? common : -1;
            } else if (v == 0x40000004) {
                // the mean health of the found units in percent (+0x170
                // GetHealth), rounded by fistp (round to nearest); 0 without
                // units. HD keeps it in DAT_0092e350.
                float sum = 0.0f;
                for (int j = 0; j < rt->Found.Count; ++j)
                    if (UV::kReal)
                        sum = UV::Iface(FoundUnit(&rt->Found, j))->GetHealth() + sum;
                value = rt->Found.Count == 0 ? 0 : (int)lrintf((sum * 100.0f) / (float)rt->Found.Count);
            } else if (v == 0x40000008) {
                // 0x580a5e: the mean ammunition of gunner 0 of the found
                // units with a gunner whose prototype has ammunition
                // (+0x2c GetPGunner()->Ammo +0x48 != 0): AmmoLeft (+0x20)
                // plus 1 / Ammo when a round is loaded (+0x5c), in percent
                // (0x7ee558 = 100.0, fistp); 0 when no unit counts.
                float sum = 0.0f;
                int n = 0;
                for (int j = 0; j < rt->Found.Count; ++j) {
                    SUnit* u = WorldUnit(FoundUnit(&rt->Found, j));
                    if (u->Gunners.Size == 0)                     // +0x4c
                        continue;
                    SGunner* g = u->Gunners.Array[0];             // 0x55cc40(0)
                    if (g->GetPGunner()->Ammo == 0)
                        continue;
                    sum = g->AmmoLeft + sum;
                    if (g->Loaded)
                        sum = 1.0f / (float)g->GetPGunner()->Ammo + sum;   // 0x7f1b58 = 1.0
                    ++n;
                }
                value = n == 0 ? 0 : (int)lrintf((sum * 100.0f) / (float)n);
            } else if (v >= 0x40000014 && v <= 0x40000027) {
                value = g_World->GetTriggerVariableValue(v, true);    // World vtbl +4 (v, 1)
            } else {
                break;                                            // unknown special variable: true
            }
            int want = c.P200[2];
            switch (c.P200[1]) {
            case 0: if (value != want) return; break;
            case 1: if (value <= want) return; break;
            case 2: if (want <= value) return; break;
            case 3: if (value == want) return; break;
            case 4: if (value < want) return; break;
            case 5: if (want < value) return; break;
            }
            break;
        }
        case 4:
        case 8: {
            int u = c.Type == 4 ? rt->Unit : rt->Unit2;
            if (!m2u::IsLive(u) || !PlayerMatches(UV::Player(u), c.Player, rt))
                return;
            break;
        }
        case TC_TRIGGERING_UNIT_TYPE:                             // 5
        case 9: {
            int u = c.Type == 5 ? rt->Unit : rt->Unit2;
            if (!m2u::IsLive(u) || !TypeMatches(u, c.UnitType, SStr(c.UnitClass)))
                return;
            break;
        }
        case TC_PUT_TRIGGERING_UNIT:                              // 6
        case 10: {
            ClearFound(&rt->Found, 0);
            rt->Found.Leader = -1;
            rt->Found.Kind = 0xff;
            int u = c.Type == 6 ? rt->Unit : rt->Unit2;
            if (!m2u::IsLive(u))
                return;
            AddFound(&rt->Found)->Unit = u;
            rt->Found.X = UV::X(u);
            rt->Found.Z = UV::Z(u);
            rt->Found.RadiusSq = 0.0f;
            rt->Found.Kind = UV::FlagsD8(u);
            break;
        }
        case 7:
        case 0xb: {
            int u = c.Type == 7 ? rt->Unit : rt->Unit2;
            if (!m2u::IsLive(u) || !InLocation(u, c.Location))
                return;
            break;
        }
        case 0xc:
        case 0xe: {
            ClearFound(&rt->Found, 0);
            rt->Found.Leader = -1;
            rt->Found.Kind = 0xff;
            PZ_FOR_EACH_UNIT(i) {
                bool ok = c.Type == 0xc ? Findable(i)
                                        : (UV::ClassType(i) == 9 && !UV::B150(i) &&
                                           (UV::Container(i) < 0 || UV::B7c(i)) && !UV::Unplaced(i));
                if (ok && ScriptIdMatches(i, c.Str40000)) {
                    SFoundUnit* f = AddFound(&rt->Found);
                    f->Flag = false;
                    f->Unit = i;
                }
            }
            GroupStats(&rt->Found);
            break;
        }
        case 0xd: {
            ClearFound(&rt->Found, 0);
            rt->Found.Leader = -1;
            rt->Found.Kind = 0xff;
            PZ_FOR_EACH_UNIT(i) {
                if (Findable(i) && UV::RawInt(i, 0x80) == c.P2000) {
                    SFoundUnit* f = AddFound(&rt->Found);
                    f->Flag = false;
                    f->Unit = i;
                }
            }
            // HD returns here without 0x582770 (falls to the next condition).
            break;
        }
        }
    }
    // Every condition holds: start the trigger.
    // PANZERS 0x560bc0 (SDArray<SRunningTrigger>::Add)
    if (RunningTriggerCount == RunningTriggerMax) {
        int nmax = RunningTriggerMax < 0x10 ? 0x10 : (RunningTriggerMax * 6) / 5;
        RunningTriggers = (SRunningTrigger*)realloc(RunningTriggers, nmax * sizeof(SRunningTrigger));
        memset(RunningTriggers + RunningTriggerMax, 0, (nmax - RunningTriggerMax) * sizeof(SRunningTrigger));
        RunningTriggerMax = nmax;
    }
    SRunningTrigger* r = &RunningTriggers[RunningTriggerCount++];
    r->Trigger = rt->Trigger;
    r->Action = rt->Action;
    r->Wait = rt->Wait;
    r->Unit = rt->Unit;
    r->Unit2 = rt->Unit2;
    CopyFound(&r->Found, &rt->Found);
    r->Action = 0;
    r->Wait = 0;
    TriggerAt(rt->Trigger)->Flags = 1;
    if (TrigTrace())
        Logger.g->Log(0, "PZM2 TRIG t%d frame %d start T%d '%s' unit %d found %d", (int)g_M2Tick, Frame,
                      rt->Trigger, SStr(TriggerAt(rt->Trigger)->Name), rt->Unit, r->Found.Count);
}

// PANZERS 0x579170 (SDArray<SRunningTrigger>::Remove)
void SGameLogic::RemoveRunningTrigger(int index)
{
    if (index < 0 || index >= RunningTriggerCount)
        Logger.g->Panic("SDArray<%s>::Remove: invalid index (%d) size = %d", "SRunningTrigger", index,
                        RunningTriggerCount);
    SRunningTrigger* r = &RunningTriggers[index];
    free(r->Found.Units);
    r->Found.Units = nullptr;
    r->Found.Count = 0;
    r->Found.Max = 0;
    --RunningTriggerCount;
    if (RunningTriggerCount - index != 0)
        memmove(&RunningTriggers[index], &RunningTriggers[index + 1],
                (RunningTriggerCount - index) * sizeof(SRunningTrigger));
    memset(&RunningTriggers[RunningTriggerCount], 0, sizeof(SRunningTrigger));
}

// ---------------------------------------------------------------------------
// Actions

// PANZERS 0x56abe0
// Player selector of actions: 12 = the triggering unit's player, 13 = the
// second unit's, else the player itself; -1 when the unit is gone.
static int ResolvePlayer(int sel, const SRunningTrigger* rt)
{
    if (sel == 0xc) {
        if (m2u::IsLive(rt->Unit))
            return UV::Player(rt->Unit);
        return -1;
    }
    if (sel != 0xd)
        return sel;
    if (m2u::IsLive(rt->Unit2))
        return UV::Player(rt->Unit2);
    return -1;
}

// RunTriggers cases 1 and 0x29: CREATE. HD builds an SUnitDef (0x5cfb10) on
// the stack, adds a crew ("XX Crew Squad", the first two letters of the
// class) as stored unit when B100000 is set, calls SWorld::CreateUnit
// 0x5e2da0, finds free space near the location centre (0x5e58d0) and
// places the unit (+0x50).
static void ActionCreateUnit(SGameLogic* gl, const STriggerAction* a, SRunningTrigger* rt, bool withCrew)
{
    SWorld* w = g_World;
    if (!w->Locations.IsLive(a->P20))                             // 0x573730
        return;
    int player = ResolvePlayer(a->Player, rt);
    if (player < 0)
        return;
    const SLocation* l = LocationAt(a->P20);                      // 0x5608c0
    float cx = (float)((double)(l->X2 + l->X1) * 0.5);            // cvtdq2pd, mulsd 0.5, cvtpd2ps
    float cz = (float)((double)(l->Z2 + l->Z1) * 0.5);
    if (!(cx >= 0.0f))
        return;
    SUnitDef def;                                                 // 0x5cfb10
    def.ClassName = a->Str10;                                     // 0x52c2c0
    def.Player = player;
    def.Pos[0] = cx;
    def.Pos[1] = cz;
    def.Dir = 0.0f;
    if (withCrew) {
        def.Slots[0] = a->P20000[0];                              // +0x40 / +0x44
        def.Slots[1] = a->P20000[1];
        def.Dir = (float)a->Num * 0.017453292f;                   // degrees (DAT_007f59a0)
        def.Behavior = a->P8000;                                  // +0x38
        def.GlobalState = a->P10000;                              // +0x3c
    }
    // +0x48 = 1: the unit is created unplaced and placed by +0x50 below. The
    // M1 stand-in has no +0x50, so it is placed by Init instead.
    def.Stored = UV::kReal ? true : false;
    if (withCrew && a->B100000) {
        // "XX Crew Squad": SString Mid(0, 2) (0x5336b0) + " Crew Squad" (0x7f7824)
        char crew[64];
        const char* cls = SStr(a->Str10);
        crew[0] = cls[0];
        crew[1] = cls[0] ? cls[1] : 0;
        crew[2] = 0;
        strncat(crew, " Crew Squad", sizeof(crew) - 3);
        SUnitDef cd;                                              // 0x5cfb10
        cd.ClassName = crew;
        cd.Player = player;
        cd.Pos[0] = 0.0f;
        cd.Pos[1] = 0.0f;
        cd.Behavior = a->P8000;
        cd.GlobalState = a->P10000;
        cd.Slots[0] = a->P20000[0];
        cd.Slots[1] = a->P20000[1];
        cd.Dir = 0.0f;
        cd.Stored = true;
        // PANZERS 0x560d10 + 0x560740 + 0x560290: def.StoredUnits.Add(cd).
        // The element takes over cd's strings (HD copies, then destroys cd).
        if (def.StoredCount == def.StoredMax) {
            int nmax = def.StoredMax < 0x10 ? 0x10 : (def.StoredMax * 6) / 5;
            def.StoredUnits = (SUnitDef*)realloc((void*)def.StoredUnits, nmax * sizeof(SUnitDef));
            memset((void*)(def.StoredUnits + def.StoredMax), 0, (nmax - def.StoredMax) * sizeof(SUnitDef));
            def.StoredMax = nmax;
        }
        memcpy((void*)&def.StoredUnits[def.StoredCount++], (const void*)&cd, sizeof(SUnitDef));
        memset((void*)&cd, 0, sizeof(SUnitDef));
    }
    if (def.StoredCount > 0)
        def.StoredUnits[0].Player = player;
    int unit = w->CreateUnit(&def);                               // 0x5e2da0
    if (TrigTrace())
        Logger.g->Log(0, "PZM2 TRIG t%d   CREATE '%s' player %d at loc %d (%.1f, %.1f) -> unit %d", (int)g_M2Tick,
                      SStr(a->Str10), player, a->P20, (double)cx, (double)cz, unit);
    if (unit < 0)
        return;
    if (UV::kReal) {
        // 0x5e58d0(out, cx, cz, def.Dir + pi (0x7f4584, addss), unit +0x58,
        // (word) unit +0xd8, 1); a spot more than 10 m away (squared distance
        // > 100.0f, 0x7ee558) is only a warning (0x65cac0).
        float out[2];
        w->FindEmptySpace(out, cx, cz, def.Dir + 3.1415927f, UV::RawInt(unit, 0x58), UV::FlagsD8(unit), true);
        float dx = out[0] - cx;
        float dz = out[1] - cz;
        if (dx * dx + dz * dz > 100.0f)
            Logger.g->Warning(withCrew ? "SGameLogic::RunTriggers/ACTION_CREATE_UNIT_2: World->FindEmptySpace tul "
                                         "messze talalt helyet"                                   // 0x7f7830
                                       : "SGameLogic::RunTriggers/ACTION_CREATE: World->FindEmptySpace tul messze "
                                         "talalt helyet");                                        // 0x7f7728
        UV::Iface(unit)->Place(out[0], out[1], def.Dir);          // +0x50
    }
    (void)gl;
}

// RunTriggers case 0x26: teleport the found units to the location centre.
static void ActionTeleport(const STriggerAction* a, SRunningTrigger* rt)
{
    if (rt->Found.Count == 0)
        return;
    const SLocation* l = LocationAt(a->P20);
    float cx = (float)(l->X2 + l->X1) * 0.5f;                     // cvtdq2ps, mulss 0.5f
    float cz = (float)(l->Z2 + l->Z1) * 0.5f;
    if (!(cx >= 0.0f))
        return;
    for (int k = 0; k < rt->Found.Count; ++k) {
        int u = FoundUnit(&rt->Found, k);
        if (UV::kReal)
            WorldUnit(u)->StopUnit();                             // 0x5c2d30
        UV::SetFlagsD8(u, UV::FlagsD8(u) & 0xfffe);
        float tx = cx, tz = cz;
        if (UV::UnitType(u) != 0xc && UV::kReal) {               // SPUnit +0x44
            // 0x5e5700(out, cx, cz, unit x, unit z, unit +0x58, (word) unit +0xd8, 1)
            float out[2];
            g_World->FindEmptySpaceNear(out, cx, cz, UV::X(u), UV::Z(u), UV::RawInt(u, 0x58), UV::FlagsD8(u), true);
            tx = out[0];
            tz = out[1];
        }
        if (TrigTrace())
            Logger.g->Log(0, "PZM2 TRIG t%d   TELEPORT unit %d '%s' -> loc %d (%.1f, %.1f)", (int)g_M2Tick, u,
                          UV::TypeName(u), a->P20, (double)tx, (double)tz);
        if (UV::kReal) {
            SIUnit* iu = UV::Iface(u);
            float dir = UV::Dir(u);
            int dirBits, f88;
            memcpy(&dirBits, &dir, 4);
            f88 = *(int*)((unsigned char*)(void*)iu + 0x88);
            iu->SetPosition(tx, tz, dirBits, f88);                // +0x24(x, z, +0xb0, +0x88)
        } else {
            SMenuUnit* mu = (SMenuUnit*)(void*)g_World->Units.Array[u].Unit;
            m2u::StandIn(u)->Order = 0;                           // as 0x5c2d30: the unit stops
            float ox = mu->Pos[0], oz = mu->Pos[2];
            mu->Pos[0] = tx;
            mu->Pos[2] = tz;
            mu->Pos[1] = g_World->GetTerrainHeight(tx, tz);
            for (int m = 0; m < mu->MemberCount; ++m) {           // M1 squad members move along
                int mi = mu->Members[m];
                if (!m2u::IsLive(mi))
                    continue;
                SMenuUnit* mm = (SMenuUnit*)(void*)g_World->Units.Array[mi].Unit;
                mm->Pos[0] += tx - ox;
                mm->Pos[2] += tz - oz;
                mm->Pos[1] = g_World->GetTerrainHeight(mm->Pos[0], mm->Pos[2]);
            }
        }
    }
}

// RunTriggers case 0x1f: remove the found units (buildings stay); the
// "PL PZLp11c" type leaves the anim/PZLp11C.4d puff behind.
static void ActionRemoveFound(SGameLogic* gl, SRunningTrigger* rt)
{
    if (rt->Found.Count == 0)
        return;
    for (int k = 0; k < rt->Found.Count; ++k) {
        int u = FoundUnit(&rt->Found, k);
        if (UV::ClassType(u) == 9)
            continue;
        if (_stricmp(UV::TypeName(u), "PL PZLp11c") == 0) {        // 0x52c410 (SString ==)
            float x = UV::X(u), z = UV::Z(u);
            gl->CreateAnimatedModel("anim/PZLp11C.4d", x, g_World->GetTerrainHeight(x, z), z,
                                    UV::Dir(u) + 3.1415927f);     // 0x5649e0
        }
        if (TrigTrace())
            Logger.g->Log(0, "PZM2 TRIG t%d   REMOVE unit %d '%s'", (int)g_M2Tick, u, UV::TypeName(u));
        if (!UV::kReal)
            gl->RemoveUnitFromMovementGroup(u);                   // HD: SUnit::Uninit 0x5b7e40 does it
        g_World->RemoveUnit(u);                                   // 0x5f8060
    }
}

// ---------------------------------------------------------------------------
// M4 (agent T): the actions of maps/tutorial.map.

// PANZERS 0x564510
// The found units follow a path: one movement group towards the first path
// point (0x56ff30), then 0x5bbb60(command, path, p4, queue) per unit.
void OrderAlongPath(SGameLogic* gl, SFoundUnits* g, int command, int path)
{
    SHeap<SPath>& paths = g_World->Paths;
    if (!paths.IsLive(path))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SPath", path);
    SPath& p = paths.Array[path].Data;
    if (p.Points.Size < 1)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SPathPoint", 0);
    gl->GroupOrder(false, 0, g, true, p.Points.Array[0].X, p.Points.Array[0].Z);   // 0x56ff30(0, p4, g, 1, x, z)
    for (int k = 0; k < g->Count; ++k)
        if (UV::kReal)
            WorldUnit(FoundUnit(g, k))->OrderParam(command, path, false, false);   // 0x5bbb60
}

// PANZERS 0x581930
// The unit stands strictly inside the location rectangle (false for a dead
// location).
static bool StrictlyInLocation(int unit, int location)
{
    if (!g_World->Locations.IsLive(location))
        return false;
    if (!m2u::IsLive(unit))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", unit);
    const SLocation* l = LocationAt(location);                    // 0x5608c0
    float x = UV::X(unit), z = UV::Z(unit);
    return (float)l->X1 < x && x < (float)l->X2 && (float)l->Z1 < z && z < (float)l->Z2;
}

// RunTriggers case 0x15: the found units enter (order 0x2a) the first live
// unit in the location that is placed, not stored (or class 10) and not
// dying, in heap order (0x56b3b0).
static void ActionEnterBuilding(SGameLogic* gl, const STriggerAction* a, SRunningTrigger* rt)
{
    if (rt->Found.Count == 0 || !g_World->Locations.IsLive(a->P20))   // 0x573730
        return;
    PZ_FOR_EACH_UNIT(u) {
        if (UV::B150(u))
            continue;
        if (!(UV::Container(u) < 0 || UV::ClassType(u) == 10))
            continue;
        if (UV::Unplaced(u))
            continue;
        if (StrictlyInLocation(u, a->P20)) {
            gl->OrderAtUnit(&rt->Found, 0x2a, u, false, false);  // 0x564720(g, 0x2a, u, 0, 0)
            break;
        }
    }
}

// PANZERS 0x5c1630 (SUnit::SetPlayer)
// The campaign statistics move the unit's category count from the old player
// to the new one (outside the cut-scenes and the multiplayer menu), the
// stored units and members follow, the team is the player's, and the
// selection bits are cleared.
void UnitSetPlayer(SUnit* u, int player)
{
    if (u->Player < 0)
        Logger.g->Panic("SUnit::SetPlayer - Player < 0");
    if (g_GameLogic && g_Campaign && g_Campaign->MenuToLoad != 1) {
        int cat = UnitStatsCategory(u);                           // 0x56d6d0
        if (u->Player != player && cat > 0 && !g_GameLogic->IsPaused()) {   // 0x56e150
            int* st = (int*)((unsigned char*)g_Campaign + 0x14c);
            st[u->Player * 0x36] -= 1;                            // +0x14c + player * 0xd8
            st[u->Player * 0x36 + cat] -= 1;
            st[player * 0x36] += 1;
            st[player * 0x36 + cat] += 1;
        }
    }
    u->Player = player;                                           // +0xfc
    for (int i = 0; i < u->Stored.Size; ++i) {                    // +0x16c
        int s = u->Stored.Array[i].Unit;
        if (!m2u::IsLive(s))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", s);
        UnitSetPlayer(WorldUnit(s), player);
    }
    for (int i = 0; i < u->Members.Size; ++i) {                   // +0x178
        int m = u->Members.Array[i].Unit;
        if (!m2u::IsLive(m))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", m);
        UnitSetPlayer(WorldUnit(m), player);
    }
    u->Team = *(int*)(g_World->Players[u->Player]);              // World +0x170 + player * 0x48
    u->_104 = 0;
    u->_108 = 0;
    u->_10c = 0;
}

// PANZERS 0x579420
// Removes the objective markers (+0x194 SDArray, 0x14 each: {x, z, board
// frame, objective +0x0c, target +0x10}) of one objective (-1: any) and one
// target (-1: any): a marker on the map (x >= 0) drops its board frame
// (board +0x0c), one off the map the minimap's marker (board +0xbc); then
// the record goes (0x579070, SDArray::Remove). The recompile makes no board
// frame for a marker (AddMinimapObjective keeps -1), so only the records go.
static void RemoveObjectiveMarkers(SGameLogic* gl, int objective, int target)
{
    int* arr = (int*)((unsigned char*)gl + 0x194);                // {data, size, max}
    int i = 0;
    while (i < arr[1]) {
        unsigned char* e = (unsigned char*)(size_t)arr[0] + i * 0x14;
        if ((objective != -1 && *(int*)(e + 0xc) != objective) ||
            (target != -1 && *(int*)(e + 0x10) != target)) {
            ++i;
            continue;
        }
        // HD: x >= 0 -> board +0x0c(frame); else board +0xbc (minimap marker).
        int n = --arr[1];                                         // 0x579070
        if (n - i > 0)
            memmove(e, e + 0x14, (size_t)(n - i) * 0x14);
        memset((unsigned char*)(size_t)arr[0] + n * 0x14, 0, 0x14);
        // HD keeps its index after a removal (the next record moved into it).
    }
}

// RunTriggers case 0x23: objective Num (1-based) completed.
static void ActionObjectiveCompleted(SGameLogic* gl, const STriggerAction* a)
{
    if (a->Num <= 0 || !g_Campaign || a->Num - 1 >= g_Campaign->ObjectiveCount)
        return;
    SCampaignObjective& o = g_Campaign->Objectives[a->Num - 1];   // 0x560580
    if (!o.Hero) {                                                // +0x07
        PzMessageFading(gl, GetText("world/GameLogic.cpp", "Objective completed: "), 2);   // 0x660c50, 0x56a480
        PzMessageFading(gl, SStr(o.Text), 0);
        PzMessageFading(gl, "", 0);
    }
    if (o.State != 2) {
        if (!o.Main)                                              // +0x05: side objectives give prestige
            g_Campaign->Prestige += o.Secret ? 0x28 : 0x3c;       // 0x592b10 (+0x38)
        // 0x57ba97: Concert +0x80(0), +0x6c, +0x70("music/Objective.mp3" 0x7f7810), +0x78(0)
        ::PzPlayMusicTrack("music/Objective.mp3");
    }
    o.State = 2;
    RemoveObjectiveMarkers(gl, a->Num - 1, -1);                   // 0x57baf8: 0x579420(objective n - 1, any target)
    int i = 0;
    for (; i < g_Campaign->ObjectiveCount; ++i) {
        const SCampaignObjective& m = g_Campaign->Objectives[i];
        if (m.Main && m.State != 2)
            break;
    }
    if (i < g_Campaign->ObjectiveCount)
        return;
    // Every main objective done: the human players win.
    for (int pl = 0; pl < 12; ++pl) {
        int* rec = (int*)g_World->Players[pl];                    // World +0x170 + pl * 0x48
        if (rec[3] == 1) {                                        // +0x17c
            rec[7] = 1;                                           // +0x18c
            g_Campaign->SetMissionResult(1);                      // 0x597470(1)
            rec[2] = 2;                                           // +0x178
        }
    }
}

// RunTriggers case 0x24: unit +0x114 = Num %, for units with health left,
// and for every member / seat of every found unit.
static void ActionSetHealth(SRunningTrigger* rt, int percent)
{
    if (rt->Found.Count == 0 || !UV::kReal)
        return;
    for (int k = 0; k < rt->Found.Count; ++k) {
        SUnit* u = WorldUnit(FoundUnit(&rt->Found, k));
        if (u->HP != 0.0f)                                        // ucomiss +0x114, 0
            u->HP = (float)percent * 0.01f;                       // mulss 0.01f (0x7f1b48)
        for (int m = 0; m < u->Members.Size; ++m) {
            int mi = u->Members.Array[m].Unit;
            if (!m2u::IsLive(mi))
                Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", mi);
            WorldUnit(mi)->HP = (float)percent * 0.01f;
        }
    }
}

// RunTriggers case 0x2c: move to the location centre facing Num degrees.
static void ActionMoveFacing(SGameLogic* gl, const STriggerAction* a, SRunningTrigger* rt)
{
    if (rt->Found.Count == 0)
        return;
    const SLocation* l = LocationAt(a->P20);
    float target[2];
    target[0] = (float)((double)(l->X2 + l->X1) * 0.5);
    target[1] = (float)((double)(l->Z2 + l->Z1) * 0.5);
    float dir = (float)a->Num * 0.017453292f;                     // 0x7f59a0
    GroupStats(&rt->Found);                                       // 0x582770
    if (!(target[0] >= 0.0f))
        return;
    float dx = target[0] - rt->Found.X;
    float dz = target[1] - rt->Found.Z;
    float d = dx * dx + dz * dz;
    if (d > rt->Found.RadiusSq)
        gl->MoveFoundUnitsToLocationDir(&rt->Found, 1, target, dir, false, false, false);   // 0x57f200
    else
        gl->MoveFoundUnitsNear(&rt->Found, 1, target, false, false, false);   // 0x57e8b0 (shared tail of case 0xe)
}

// RunTriggers case 0x2d: a one-shot effect at the location centre (guide
// arrows of the tutorial).
static void ActionLocationEffect(const STriggerAction* a)
{
    if (!g_Pixie || !g_Scene)
        return;
    int proto = g_Pixie->LoadEffectPrototype(SStr(a->Str4000), false, false, 0, 0);   // pixie +0x10(name, 0, 0)
    const SLocation* l = LocationAt(a->P20);
    float pos[3], dir[3] = {0.0f, 1.0f, 0.0f};
    pos[0] = (float)(l->X2 + l->X1) * 0.5f;                       // cvtdq2ps, mulss 0.5f (0x7f453c)
    pos[2] = (float)(l->Z2 + l->Z1) * 0.5f;
    pos[1] = g_World->GetTerrainHeight(pos[0], pos[2]);           // 0x5e7730
    g_Pixie->PlayEffect(g_Scene, proto, pos, dir, 0);             // pixie +0x24(scene, proto, &pos, &dir, 0)
    g_Pixie->ReleaseEffectPrototype(proto);                       // pixie +0x20
}

// PANZERS 0x579ab0 (case 0x38, the map branch 0x57d42a..0x57d5b8)
// The map cut-scene: the logic's state of the old map goes (messages,
// objective markers, animated models, doodad grid, vision maps, active
// locations, running triggers), the view loads "maps/<n>.map" in place
// (SIGameViewCallback +0x10, 0x624770), the logic's map state is built again
// as in the ctor 0x55e440 (vision maps 0x564fb0, the player table 0x565e10,
// active locations 0x5640b0 / 0x582080, doodad grid 0x564c20, every unit
// +0x34 and the buildings' 0x5497a0), and campaign +0x14 = 1: the new map's
// own 0x38 with the same name plays the cut-scene, PlaceMyUnits is off and
// the mission end keeps the army that 0x624770 backed up. Without a view
// (+0x00 = 0, the menu logic) nothing happens.
bool PzMapCutscene(SGameLogic* gl, const char* map)
{
    if (gl->Mode == 0)                                            // *(+0x00) == 0
        return false;
    Logger.g->Log(0, "PZM5: map cut-scene %s at frame %d", map, gl->Frame);
    PzMessagesClearFading(gl);                                    // 0x563860
    PzMessagesClearStatic(gl);                                    // 0x5638b0
    RemoveObjectiveMarkers(gl, -1, -1);                           // 0x579420(-1, -1)
    for (int i = 0; i < gl->AnimatedModelCount; ++i) {            // 0x563230(0) on +0x2fc
        if (gl->AnimatedModels[i].Model)
            gl->AnimatedModels[i].Model->Release();               // +0x18 vtbl +4
        FreeSString(&gl->AnimatedModels[i].Name);                 // +0x00
    }
    gl->AnimatedModelCount = 0;
    if (gl->AnimatedModels)
        memset(gl->AnimatedModels, 0, (size_t)gl->AnimatedModelMax * sizeof(SAnimatedModel));
    free(gl->DoodadGrid);                                         // +0x1b8 (0x76654a)
    gl->DoodadGrid = nullptr;
    gl->FreeVisMaps();                                            // 0x579a50
    gl->ActiveLocationCount = 0;                                  // 0x546af0(0) on +0x2f0
    if (gl->ActiveLocations)
        memset(gl->ActiveLocations, 0, (size_t)gl->ActiveLocationMax * sizeof(int));
    for (int i = 0; i < gl->RunningTriggerCount; ++i) {           // 0x563410(0) on +0x268
        free(gl->RunningTriggers[i].Found.Units);                 // +0x28
        gl->RunningTriggers[i].Found.Units = nullptr;
    }
    gl->RunningTriggerCount = 0;
    if (gl->RunningTriggers)
        memset(gl->RunningTriggers, 0, (size_t)gl->RunningTriggerMax * sizeof(SRunningTrigger));
    PzViewLoadMapInPlace(gl->Mode, map);                          // (+0x00) vtbl +0x10(map)
    gl->BuildVisMaps();                                           // 0x564fb0
    for (int i = 0; i < 12; ++i) {
        gl->PlayerTable[i] = -1;                                  // +0x234
        gl->Tick_565e10(i);                                       // 0x565e10
    }
    gl->CollectActiveLocations();                                 // 0x5640b0
    PZ_FOR_EACH_UNIT(i) {
        int ct = UV::ClassType(i);
        if ((ct == 0 || ct == 5 || ct == 0xc || ct == 0xb) && !UV::Unplaced(i))   // +0x168
            gl->UpdateActiveLocationsAt(i, false);                // 0x582080(unit, 0)
    }
    gl->BuildDoodadGrid();                                        // 0x564c20
    if (UV::kReal) {
        PZ_FOR_EACH_UNIT(i) {
            UV::Iface(i)->RefreshTargeting();                     // +0x34
            if (UV::ClassType(i) == 9)
                static_cast<SBuildingUnit*>(g_World->Units.Array[i].Unit)->InitBlockCells();   // 0x5497a0
        }
    }
    RemoveObjectiveMarkers(gl, -1, -1);                           // 0x579420(-1, -1)
    g_Campaign->_014 = 1;                                         // campaign +0x14
    return true;
}

// RunTriggers case 0x38: "Loading cutscene..." after frame 0, then the map
// cut-scene "maps/<n>.map" (when it exists and the campaign +0x14 is clear)
// or "cutscenes/<n>/<n>" (0x56ea20). HD returns from RunTriggers after this
// action (0x57d1b2): the other running triggers wait for the next tick.
static void ActionPlayCutscene(SGameLogic* gl, const STriggerAction* a, bool* mapLoaded)
{
    *mapLoaded = false;
    if (gl->Frame > 0) {
        PzMessageFading(gl, GetText("world/GameLogic.cpp", "Loading cutscene..."), 2);   // 0x56a480
        // viewport +0x3c / +0x50: one frame drawn with the message (not lifted).
    }
    const char* n = SStr(a->Str1000);
    char map[300];
    _snprintf(map, sizeof(map) - 1, "maps/%s.map", n);
    map[sizeof(map) - 1] = 0;
    bool mapCut = g_Campaign && g_Campaign->_014 == 0 && FileSystem.Stat(map, nullptr) == 0;   // 0x65faf0
    if (mapCut) {
        PzMapCutscene(gl, map);
        *mapLoaded = true;                                        // no action index to step (0x57d433 / 0x57d5b8)
        return;
    }
    char name[300];
    _snprintf(name, sizeof(name) - 1, "cutscenes/%s/%s", n, n);   // "cutscenes/" + n + "/" + n
    name[sizeof(name) - 1] = 0;
    PzCutscenePlay(gl, name);                                     // 0x56ea20
}

// RunTriggers case 0x42: World 0x600770 queues the speech file. The speech
// queue player (SWorld::UpdateSpeech 0x607f50) is not lifted.
static void ActionSpeech(const STriggerAction* a)
{
    PzSpeechQueue(SStr(a->Str1000));                              // World 0x600770
}

// PANZERS 0x579ab0
// Walks the running triggers. For each: drops found units that died or were
// unplaced (then 0x582770), waits (+0x08), and otherwise executes actions
// one after the other until a wait or the end; at the end the trigger's
// flag bit 0 is cleared (preserved triggers, action 0x0a, return to 2) and
// the running trigger is removed.
void SGameLogic::RunTriggers()
{
    PZ_M2_TRACE("SGameLogic::RunTriggers (0x579ab0)");
    if (Flag288 || Flag2b8)
        return;
    int i = 0;
    while (i < RunningTriggerCount) {
        SRunningTrigger* rt = &RunningTriggers[i];
        bool changed = false;
        int j = 0;
        while (j < rt->Found.Count) {
            int u = rt->Found.Units[j].Unit;
            if (m2u::IsLive(u) && !UV::B150(u) && !UV::Unplaced(u)) {
                ++j;
                continue;
            }
            --rt->Found.Count;
            if (rt->Found.Count - j != 0)
                memmove(&rt->Found.Units[j], &rt->Found.Units[j + 1], (rt->Found.Count - j) * sizeof(SFoundUnit));
            memset(&rt->Found.Units[rt->Found.Count], 0, sizeof(SFoundUnit));
            changed = true;
        }
        if (changed)
            GroupStats(&rt->Found);
        if (rt->Wait != 0) {
            --rt->Wait;
            ++i;
            continue;
        }
        STrigger* t = TriggerAt(rt->Trigger);
        if (rt->Action >= t->Actions.Size) {
            t->Flags &= ~1;
            if (TrigTrace())
                Logger.g->Log(0, "PZM2 TRIG t%d frame %d end T%d flag %d", (int)g_M2Tick, Frame, rt->Trigger, t->Flags);
            RemoveRunningTrigger(i);
            continue;
        }
        const STriggerAction* a = &t->Actions.Array[rt->Action];
        if (TrigTrace())
            Logger.g->Log(0, "PZM2 TRIG t%d frame %d T%d action %d type 0x%x", (int)g_M2Tick, Frame, rt->Trigger,
                          rt->Action, a->Type);
        switch (a->Type) {
        case 0:
            break;
        case 1:                                                   // create unit (no crew, dir 0)
            ActionCreateUnit(this, a, rt, false);
            break;
        case TA_MOVE_TO_LOCATION: {                               // 2
            if (rt->Found.Count == 0)
                break;
            const SLocation* l = LocationAt(a->P20);
            float target[2];
            target[0] = (float)((double)(l->X2 + l->X1) * 0.5);
            target[1] = (float)((double)(l->Z2 + l->Z1) * 0.5);
            GroupStats(&rt->Found);
            if (!(target[0] >= 0.0f))
                break;
            float dx = target[0] - rt->Found.X;
            float dz = target[1] - rt->Found.Z;
            float d = dx * dx + dz * dz;
            if (d > rt->Found.RadiusSq) {
                MoveFoundUnitsToLocation(&rt->Found, 1, target, false, false, false);   // 0x57efd0
            } else {
                // HD 0x57e8b0 (spread the group around the target; never
                // executed in the menu).
                STUB_LOG("SGameLogic::RunTriggers move to location, near branch (0x57e8b0)");
            }
            break;
        }
        case 0xe: {
            // M3 agent F. Not "end scenario": the found units attack-move
            // (command 9) to the location centre, as case 2 with command 1.
            // The Training Camp map sends its enemies to "start 1" with it.
            if (rt->Found.Count == 0)
                break;
            const SLocation* l = LocationAt(a->P20);              // 0x5608c0
            float target[2];
            target[0] = (float)((double)(l->X2 + l->X1) * 0.5);   // mulsd 0.5 (0x7ea760)
            target[1] = (float)((double)(l->Z2 + l->Z1) * 0.5);
            GroupStats(&rt->Found);                               // 0x582770
            if (!(target[0] >= 0.0f))
                break;
            float dx = target[0] - rt->Found.X;
            float dz = target[1] - rt->Found.Z;
            float d = dx * dx + dz * dz;
            if (TrigTrace())
                Logger.g->Log(0, "PZM2 TRIG t%d   ATTACK-MOVE %d units -> loc %d (%.1f, %.1f)", (int)g_M2Tick,
                              rt->Found.Count, a->P20, (double)target[0], (double)target[1]);
            if (rt->Found.RadiusSq <= d && d != rt->Found.RadiusSq) {
                MoveFoundUnitsToLocation(&rt->Found, 9, target, false, false, false);   // 0x57efd0(found, 9, &target, 0)
            } else {
                // HD 0x57e8b0(found, 9, &target, 0): agent O.
                STUB_LOG("SGameLogic::RunTriggers attack-move to location, near branch (0x57e8b0)");
            }
            break;
        }
        case TA_SET_VARIABLE:                                     // 3
            g_World->SetTriggerVariableValue(a->P400, a->Num, true);   // World vtbl +8(P400, Num, 1)
            if (TrigTrace())
                Logger.g->Log(0, "PZM2 TRIG t%d   SET var %d = %d", (int)g_M2Tick, a->P400, a->Num);
            break;
        case 4:
            g_World->SetTriggerVariableValue(a->P400, g_World->GetTriggerVariableValue(a->P400, true) + a->Num, true);
            break;
        case 5:
            g_World->SetTriggerVariableValue(a->P400, g_World->GetTriggerVariableValue(a->P400, true) - a->Num, true);
            break;
        case 6:
            if (g_World->TriggerVariables.IsLive(a->P400)) {
                STriggerVariable& v = g_World->TriggerVariables.Array[a->P400].Data;
                v.Value = v.Value == 0 ? 1 : 0;
            }
            break;
        case 7:
            if (g_World->TriggerVariables.IsLive(a->P400))
                g_World->TriggerVariables.Array[a->P400].Data.Step = 0;
            break;
        case TA_SET_VARIABLE_PER_SECOND:                          // 8
            if (g_World->TriggerVariables.IsLive(a->P400)) {
                g_World->TriggerVariables.Array[a->P400].Data.Step = a->Num;
                if (TrigTrace())
                    Logger.g->Log(0, "PZM2 TRIG t%d   STEP var %d = %d/s", (int)g_M2Tick, a->P400, a->Num);
            }
            break;
        case 9:
            if (g_World->TriggerVariables.IsLive(a->P400))
                g_World->TriggerVariables.Array[a->P400].Data.Step = -a->Num;
            break;
        case TA_PRESERVE_TRIGGER:                                 // 0x0a
            TriggerAt(rt->Trigger)->Flags |= 2;
            break;
        case TA_MOVE_ALONG_PATH_CONVOY: {                         // 0x1e
            if (rt->Found.Count == 0)
                break;
            SHeap<SPath>& paths = g_World->Paths;
            if (!paths.IsLive(a->P800))
                Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SPath", a->P800);
            SPath& p = paths.Array[a->P800].Data;                 // 0x560960
            if (p.Points.Size < 1)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SPathPoint", 0);
            int g = GroupOrder(true, 0, &rt->Found, true, p.Points.Array[0].X, p.Points.Array[0].Z);   // 0x56ff30
            ConvoyAlongPath(g, a->P800);                          // 0x57e600
            if (TrigTrace())
                Logger.g->Log(0, "PZM2 TRIG t%d   CONVOY %d units along path %d -> group %d", (int)g_M2Tick,
                              rt->Found.Count, a->P800, g);
            break;
        }
        case TA_REMOVE_FOUND_UNITS:                               // 0x1f
            ActionRemoveFound(this, rt);
            break;
        case TA_TELEPORT:                                         // 0x26
            ActionTeleport(a, rt);
            break;
        case TA_CREATE_UNIT:                                      // 0x29
            ActionCreateUnit(this, a, rt, true);
            break;
        case 0xd:                                                 // units along a path (order 6)
            if (rt->Found.Count != 0)
                OrderAlongPath(this, &rt->Found, 6, a->P800);     // 0x564510(g, 6, path, 0, 0)
            break;
        case 0x15:
            ActionEnterBuilding(this, a, rt);
            break;
        case 0x16:                                                // unload all (order 0x2b, -1)
            if (rt->Found.Count != 0)
                OrderValue(&rt->Found, 0x2b, -1, false, false);   // 0x564870(g, 0x2b, -1, 0, 0)
            break;
        case 0x17: {                                              // camera to the location centre
            if (!g_World->Locations.IsLive(a->P20))               // 0x573730
                break;
            const SLocation* l = LocationAt(a->P20);              // 0x5608c0
            float cx = (float)((double)(l->X2 + l->X1) * 0.5);    // mulsd 0.5 (0x7ea760)
            float cz = (float)((double)(l->Z2 + l->Z1) * 0.5);
            if (cx >= 0.0f)
                g_World->SetCameraTarget(cx, cz);                 // 0x5f4f60
            break;
        }
        case 0x1b: {                                              // give the found units to a player
            int player = ResolvePlayer(a->Player, rt);            // 0x56abe0
            if (player < 0)
                break;
            for (int k = 0; k < rt->Found.Count; ++k)
                if (UV::kReal)
                    UnitSetPlayer(WorldUnit(FoundUnit(&rt->Found, k)), player);   // 0x5c1630
            break;
        }
        case 0x23:                                                // objective completed
            ActionObjectiveCompleted(this, a);
            break;
        case 0x24:                                                // health of the found units, in percent
            ActionSetHealth(rt, a->Num);
            break;
        case 0x27:                                                // behaviour (order 0x21)
            if (rt->Found.Count != 0)
                OrderValue(&rt->Found, 0x21, a->P8000, false, false);   // 0x564870(g, 0x21, +0x40, 0, 0)
            break;
        case 0x28:                                                // global state (order 0x28)
            if (rt->Found.Count != 0)
                OrderValue(&rt->Found, 0x28, a->P10000, false, false);  // 0x564870(g, 0x28, +0x44, 0, 0)
            break;
        case 0x2a:                                                // stop (order 8)
            if (rt->Found.Count != 0)
                OrderPlain(&rt->Found, 8, false, false);          // 0x564440(g, 8, 0, 0)
            break;
        case 0x2b:                                                // unit +0x20 with the two slot values
            for (int k = 0; k < rt->Found.Count; ++k)
                if (UV::kReal)
                    UV::Iface(FoundUnit(&rt->Found, k))->Hook20((int)(size_t)&a->P20000[0]);   // +0x20(&action +0x48)
            break;
        case 0x2c:                                                // move to the location, facing Num degrees
            ActionMoveFacing(this, a, rt);
            break;
        case 0x2d:                                                // an effect at the location centre
            ActionLocationEffect(a);
            break;
        case 0x2f:                                                // wait Num ticks
            rt->Wait = a->Num;                                    // running trigger +0x08
            break;
        case 0x37:                                                // XP to the found units
            for (int k = 0; k < rt->Found.Count; ++k)
                if (UV::kReal)
                    UV::Iface(FoundUnit(&rt->Found, k))->AddXP(-1, (float)a->Num, 0);   // +0x8c(-1, Num, 0)
            break;
        case 0x38: {                                              // play a cut-scene
            bool mapLoaded;
            ActionPlayCutscene(this, a, &mapLoaded);
            if (!mapLoaded)
                RunningTriggers[i].Action++;                      // 0x57d772
            return;                                               // 0x57d1b2: out of RunTriggers
        }
        case 0x3a:                                                // weather
            g_World->SetWeather(a->P200000, (int)((float)a->Num / 20.0f));   // 0x5fdc80(+0x60, Num / 20.0f)
            break;
        case 0x40:                                                // echo: the static lines of a text block
            if (g_Campaign) {
                PzMessagesClearStatic(this);                      // 0x5638b0
                PzTriggerTextBlock(this, SStr(a->Str4000), false);   // campaign +0x120, 0x5815e0 per line
            }
            break;
        case 0x41:                                                // message: the fading lines of a text block
            PzTriggerTextBlock(this, SStr(a->Str4000), true);     // 0x56a480 per line
            break;
        case 0x42:                                                // speech
            ActionSpeech(a);                                      // 0x600770
            break;
        case 0x49:                                                // found units: +0x140 / +0x14c off
            for (int k = 0; k < rt->Found.Count; ++k)
                if (UV::kReal) {
                    SUnit* u = WorldUnit(FoundUnit(&rt->Found, k));
                    u->_140 = false;
                    u->_14c = false;
                }
            break;
        default: {
            // The other 66 actions (combat, AI, objectives, cameras, sounds,
            // scripts, waits, ...) are not used by maps/menu.map.
            static bool logged[0x4c];
            int ty = a->Type;
            if (ty >= 0 && ty < 0x4c && !logged[ty]) {
                logged[ty] = true;
                if (Logger.g)
                    Logger.g->Log(1, "STUB: SGameLogic::RunTriggers action 0x%x (0x579ab0) not implemented", ty);
            }
            break;
        }
        }
        // the actions may have grown the running-trigger array
        RunningTriggers[i].Action++;
    }
}

} // namespace pz
