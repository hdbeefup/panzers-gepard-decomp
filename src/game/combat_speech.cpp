// src/game/combat_speech.cpp
// The unit speech events: SWorld::UnitSpeech 0x5fff20 ("enemy spotted", "under
// attack", "out of ammo" ...) and the per-nation sample counts 0x5edd00.
// OWNER: agent M3-C (C0, the C integrator).
//
// World fields (raw offsets; world.h keeps them in the _648 blob):
//   +0x066c  unsigned SampleCount[8 races][27 groups][32 events]  (0x5edd00)
//   +0x726c  SHeap of queued speech (element 0x38, played by UpdateSpeech 0x607f50)
//   +0x7290..+0x729c  cleared by 0x5edd00
//   +0x72a0  double LastTime[32]  timer seconds of the last queued sample per event
//   +0x73a0  int LastEvent, +0x73a4 int Repeat, +0x73a8 int LastSpeaker
//   +0x73ac  int SpeechLevel (0 off, 1 important only, 2 all; SGameView::LoadMap
//            sets it from the options, 0x64e2b0; the menu world keeps 0)
//
// Determinism (docs/M3_INTERFACES.md §7): the sample index is drawn from the
// world LCG (0x555a00) when `anyPlayer` is set, else from CRT rand() (HD site
// 0x78c846, audio only). Both draws happen only when the event's delay has
// passed on the wall-clock timer (0x661800); the world-LCG draw is therefore
// timer dependent in HD itself for the events with a delay (4, 6, 7, 10, 11,
// 14..17, 19). Kept as in HD.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "unit.h"
#include "gamelogic.h"
#include "drivermath.h"
#include "m3common.h"
#include "logger.h"
#include "stream.h"
#include "worldapi.h"
#include "timer.h"
#include "iconcert.h"

extern SIConcert* Concert;

namespace pz {

static const char* const kSpeechRace[8] = {                     // 0x8ddfd0
    "Gb", "Us", "Ge", "Pl", "Hu", "Su", "Fr", "Yu" };
static const char* const kSpeechGroup[27] = {                   // 0x8ddff0
    "Medic", "Sniper", "Infantry1", "Infantry2", "Infantry3", "Infantry4",
    "Infantry1Veh", "Infantry2Veh", "Infantry3Veh", "Infantry4Veh",
    "Infantry1VehDist3", "Infantry2VehDist3", "Infantry3VehDist3", "Infantry4VehDist3",
    "Infantry1VehDist2", "Infantry2VehDist2", "Infantry3VehDist2", "Infantry4VehDist2",
    "Infantry1VehDist1", "Infantry2VehDist1", "Infantry3VehDist1", "Infantry4VehDist1",
    "Hero", "HeroVeh", "HeroVehDist3", "HeroVehDist2", "HeroVehDist1" };
static const char* const kSpeechEvent[32] = {                   // 0x8de060
    "Selection", "MultiSelection", "Acknowledge", "Movement", "CantMove", "Attack",
    "CantAttack", "LowHealth", "Death", "Flamethrower", "LowAmmo", "OutOfAmmo",
    "EnemyUnknownUnit", "EnemyDeath", "EnemySpotted", "UnderAttack", "EnemyInvisible",
    "MineFound", "Mined", "EnemyAirAttack", "Grenade", "Surrender", "ReqReconPlane",
    "ReqTacBomber", "ReqHeavyBomber", "ReqParachutes", "ReqArtillery", "AckReconPlane",
    "AckTacBomber", "AckHeavyBomber", "AckParachutes", "AckArtillery" };
static const double kSpeechDelay[32] = {                        // 0x8018f8 (seconds)
    0, 0, 0, 0, 5, 0, 5, 5, 0, 0, 5, 5, 0, 0, 7, 5, 5, 5, 0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

namespace {
struct SSpeechState {
    unsigned char* w;
    unsigned* Count(int race, int group, int ev) { return (unsigned*)(w + 0x66c) + ((race * 0x1b + group) * 0x20 + ev); }
    double* LastTime(int ev) { return (double*)(w + 0x72a0) + ev; }
    int& At(int off) { return *(int*)(w + off); }
};
}

// The unit speech queue, HD World+0x726c SHeap (element 0x38: {Next, pad,
// SString name, unit, race, group, event, sample, priority, bool anyPlayer,
// double time}). Kept here; InitSpeechCounts (the SWorld ctor) empties it.
struct SSpeechEntry {
    int    Next;          // +0x00 kSpeechLive when used, else the next free
    int    _04;
    char*  Name;          // +0x08 SString
    int    Unit;          // +0x10
    int    Race;          // +0x14
    int    Group;         // +0x18
    int    Event;         // +0x1c
    int    Sample;        // +0x20
    int    Priority;      // +0x24 0 dropped unless played this frame, 1, 2
    bool   AnyPlayer;     // +0x28
    double Time;          // +0x30 timer seconds when queued
};
struct SSpeechHeap {
    SSpeechEntry* Array;  // +0x726c
    int Size;             // +0x7270
    int Max;              // +0x7274
    int Free;             // +0x7278
    int Count;            // +0x727c
};
static SSpeechHeap g_UnitSpeech = { nullptr, 0, 0, -1, 0 };
static const int kSpeechLive = 0x7fffffff;

static bool SpeechLive(int i)
{
    return i >= 0 && i < g_UnitSpeech.Size && g_UnitSpeech.Array[i].Next == kSpeechLive;
}

// HD 0x5d8ed0 (SHeap::Alloc): the free head first, else a new slot (the
// array grows to 16, then by 6/5).
static int SpeechAlloc()
{
    SSpeechHeap& q = g_UnitSpeech;
    ++q.Count;
    if (q.Free >= 0) {
        int i = q.Free;
        q.Free = q.Array[i].Next;
        q.Array[i].Next = kSpeechLive;
        memset((char*)&q.Array[i] + 8, 0, sizeof(SSpeechEntry) - 8);
        return i;
    }
    if (q.Size == q.Max) {
        int m = q.Max < 0x10 ? 0x10 : q.Max * 6 / 5;
        q.Array = (SSpeechEntry*)realloc(q.Array, m * sizeof(SSpeechEntry));
        memset(q.Array + q.Max, 0, (m - q.Max) * sizeof(SSpeechEntry));
        q.Max = m;
    }
    q.Array[q.Size].Next = kSpeechLive;
    return q.Size++;
}

// HD 0x5f7540 (SHeap::Remove): the name freed, the slot to the free head.
static void SpeechRemove(int i)
{
    SSpeechHeap& q = g_UnitSpeech;
    if (!SpeechLive(i))
        return;
    q.Array[i].Next = q.Free;
    free(q.Array[i].Name);
    q.Array[i].Name = nullptr;
    --q.Count;
    q.Free = i;
}

static void SpeechClear()
{
    SSpeechHeap& q = g_UnitSpeech;
    for (int i = 0; i < q.Size; ++i)
        if (q.Array[i].Next == kSpeechLive)
            free(q.Array[i].Name);
    free(q.Array);
    q.Array = nullptr;
    q.Size = q.Max = q.Count = 0;
    q.Free = -1;
}

// SWorld::UpdateSpeech 0x607f50, the unit speech part before the trigger
// speech (triggers.cpp calls it with the Concert set): entries of units that
// are gone leave the queue; when the last sample has ended (+0x7290, wall
// clock) the entry of the highest priority plays (ties: the higher group,
// then the older one); a "MultiSelection" resets the repeat count.
void PzUnitSpeechPlay(SWorld* w)
{
    SSpeechHeap& q = g_UnitSpeech;
    for (int i = 0; i < q.Size; ++i)
        if (q.Array[i].Next == kSpeechLive && q.Array[i].Unit >= 0 && !w->Units.IsLive(q.Array[i].Unit))
            SpeechRemove(i);                                      // 0x5f7540
    SSpeechState s = { (unsigned char*)w };
    float now = (float)((double)Timer.GetTickValue() / 1000.0);   // 0x661800
    double& until = *(double*)(s.w + 0x7290);
    if (!(until <= (double)now))
        return;
    int best = -1, bestPriority = -1, bestGroup = -1;
    float bestTime = now;
    for (int i = 0; i < q.Size; ++i) {
        const SSpeechEntry& e = q.Array[i];
        if (e.Next != kSpeechLive)
            continue;
        if (bestPriority < e.Priority ||
            (e.Priority == bestPriority && (bestGroup < e.Group || e.Time < (double)bestTime))) {
            bestPriority = e.Priority;
            bestGroup = e.Group;
            bestTime = (float)e.Time;
            best = i;
        }
    }
    if (best < 0)
        return;
    if (q.Array[best].Event == 1)
        s.At(0x73a4) = 0;
    const char* name = q.Array[best].Name ? q.Array[best].Name : "";
    float len = Concert->PlaySound(name, 0.0f, 0.0f, -2);         // concert +0x50
    static int s_log = -1;                                        // recompile-only: PZ_M6_SPEECH_LOG=1 logs each sample
    if (s_log < 0)
        s_log = getenv("PZ_M6_SPEECH_LOG") != nullptr;
    if (s_log && Logger.g)
        Logger.g->Log(0, "PZM6 speech %s %.2f s (priority %d, queue %d)", name, (double)len,
                      q.Array[best].Priority, q.Count);
    until = (double)len + (double)now;
    SpeechRemove(best);
}

// The end of 0x607f50: the entries of priority 0 not played this frame leave.
void PzUnitSpeechDropUnplayed()
{
    SSpeechHeap& q = g_UnitSpeech;
    for (int i = 0; i < q.Size; ++i)
        if (q.Array[i].Next == kSpeechLive && q.Array[i].Priority == 0)
            SpeechRemove(i);
}

// PANZERS 0x5edd00
// Counts speech/<race>/<group>/<event>_NN.mp3 (NN = 01, 02 ...) for every
// race, group and event, and resets the speech state. Called at the end of
// the SWorld ctor 0x5d2f90.
void SWorld::InitSpeechCounts()
{
    SSpeechState s = { (unsigned char*)this };
    char name[260];
    for (int r = 0; r < 8; ++r)
        for (int g = 0; g < 27; ++g)
            for (int e = 0; e < 32; ++e) {
                unsigned* c = s.Count(r, g, e);
                *c = 0;
                for (int n = 1;; ++n) {
                    _snprintf(name, sizeof(name) - 1, "speech/%s/%s/%s_%02d.mp3", kSpeechRace[r], kSpeechGroup[g],
                              kSpeechEvent[e], n);                // 0x52da80
                    name[sizeof(name) - 1] = 0;
                    if (FileSystem.Stat(name, nullptr) != 0)      // 0x65faf0
                        break;
                    ++*c;
                }
            }
    SpeechClear();                                                // the +0x726c queue starts empty
    memset(s.LastTime(0), 0, 32 * sizeof(double));               // +0x72a0, 0x40 dwords
    s.At(0x73a0) = -1;
    s.At(0x73a8) = -1;
    s.At(0x73a4) = 0;
    s.At(0x73ac) = 0;
    s.At(0x7290) = 0;
    s.At(0x7294) = 0;
    s.At(0x7298) = 0;
    s.At(0x729c) = 0;
}

// PANZERS 0x5fff20
// Picks the speech sample group from the speaking unit (a vehicle speaks with
// its hero / last stored crew member), its nation, health and whether it is a
// hero, then a random sample of that event, and queues it.
void SWorld::UnitSpeech(int unit, int event, bool anyPlayer)
{
    PZ_M3_TRACE("SWorld::UnitSpeech (0x5fff20)");
    if (!g_GameLogic || g_GameLogic->IsPaused())                  // 0x56e150
        return;
    SSpeechState s = { (unsigned char*)this };
    if (s.At(0x73ac) < 1)
        return;
    if (!anyPlayer && WorldUnit(unit)->Player != LocalPlayer)     // +0xfc vs World+0x16c
        return;

    bool vehicle = false;                                         // local_11 ([ebp-0xd])
    bool armoured = false;                                        // local_12 ([ebp-0xe])
    bool hero = false;
    int speaker = unit;
    SUnit* u = WorldUnit(unit);
    if (!u->Proto->BuiltInDriver && u->Stored.Size > 0) {         // proto +0xc8, +0x170
        for (int i = 0; i < u->Stored.Size; ++i) {
            speaker = u->Stored.Array[i].Unit;
            if (u->Stored.Array[i].Mode == 2) {                   // a hero inside
                hero = true;
                break;
            }
        }
        if (WorldUnit(unit)->Proto->UnitType != 0xd)
            vehicle = true;
    }
    u = WorldUnit(unit);
    if (u->Proto->HeroPicture >= 0)                               // proto +0x34
        hero = true;
    switch (u->Proto->UnitType) {
    case 0: case 1: case 2: case 4: case 3: case 7: case 6: case 10: case 0x1b: case 0x1c: case 0x1d:
        armoured = true;
        vehicle = true;
        break;
    default:
        break;
    }
    int health = 1;                                               // [ebp-0x24] then esi
    if (0.6f > u->HP)                                             // DAT_007fd6f0 (comiss, unordered = no)
        health = 2;
    if (0.3f > u->HP)                                             // DAT_007f83d8
        health = 3;

    SUnit* sp = WorldUnit(speaker);
    int race = sp->Proto->Race;                                   // +0x80
    int side = sp->RandomSide;                                    // +0xdc
    if (race < 0)
        return;

    int group = 2;                                                // ebx
    int heroGroup = 0;                                            // eax at 0x60052b
    bool useHero = false;
    switch (sp->Proto->UnitType) {
    case 0xe:
        if (!vehicle)
            group = hero ? 0x16 : (side == 0) * 2 + 3;
        else if (!armoured)
            group = hero ? 0x17 : (side == 0) * 2 + 7;
        else if (health == 1)
            group = hero ? 0x1a : (side == 0) * 2 + 0x13;
        else if (health == 2)
            group = hero ? 0x19 : (side == 0) * 2 + 0xf;
        else
            group = hero ? 0x18 : (side == 0) * 2 + 0xb;
        break;
    case 0xf:
    case 0x12:
        if (!vehicle) { group = 5; heroGroup = 0x16; useHero = true; }
        else if (!armoured) { group = 9; heroGroup = 0x17; useHero = true; }
        else if (health == 1) { group = 0x15; heroGroup = 0x1a; useHero = true; }
        else if (health == 2) group = hero * 8 + 0x11;
        else { group = 0xd; heroGroup = 0x18; useHero = true; }
        break;
    case 0x10:
        group = 0;                                                // Medic
        break;
    case 0x11:
    case 0x16:
        if (!vehicle) { group = 4; heroGroup = 0x16; }
        else if (!armoured) { group = 8; heroGroup = 0x17; }
        else if (health == 1) { group = 0x14; heroGroup = 0x1a; }
        else if (health != 2) { group = 0xc; heroGroup = 0x18; }
        else { group = 0x10; heroGroup = 0x19; }
        useHero = true;
        break;
    case 0x15:
    case 0x18:
        if (!vehicle) { group = 3; heroGroup = 0x16; }
        else if (!armoured) { group = 7; heroGroup = 0x17; }
        else if (health == 1) { group = 0x13; heroGroup = 0x1a; }
        else if (health != 2) { group = 0xb; heroGroup = 0x18; }
        else { group = 0xf; heroGroup = 0x19; }
        useHero = true;
        break;
    case 0x17:
        group = 1;                                                // Sniper
        break;
    default:
        if (!vehicle) { heroGroup = 0x16; useHero = true; }       // group stays 2 (Infantry1)
        else if (!armoured) { group = 6; heroGroup = 0x17; useHero = true; }
        else if (health == 1) group = hero * 8 + 0x12;
        else if (health != 2) { group = 10; heroGroup = 0x18; useHero = true; }
        else { group = 0xe; heroGroup = 0x19; useHero = true; }
        break;
    }
    if (useHero && hero)                                          // 0x60052b cmovne
        group = heroGroup;

    // A sixth selection of the same unit in a row becomes "MultiSelection".
    if (event == 0 && s.At(0x73a0) == 0 && s.At(0x73a8) == speaker) {
        if (++s.At(0x73a4) >= 5)
            event = 1;
    } else {
        s.At(0x73a0) = event;
        s.At(0x73a8) = speaker;
        s.At(0x73a4) = 0;
    }
    int priority = 0;
    switch (event) {
    case 4: case 7: case 10: case 0xb: case 0xc: case 0xe: case 0x11: case 0x12:
        priority = 1;
        break;
    case 6: case 9: case 0xf: case 0x10: case 0x13:
        priority = 2;
        break;
    default:
        if (s.At(0x73ac) == 1)                                    // "important speech only"
            return;
        break;
    }
    double now = (double)Timer.GetTickValue() / 1000.0;           // 0x661800 on the timer 0x92e354
    unsigned count = *s.Count(race, group, event);
    if ((int)count > 0 && kSpeechDelay[event] + *s.LastTime(event) < now) {
        int sample;
        if (anyPlayer)
            sample = HdRandInt((int)count) + 1;                   // 0x555a00 (world LCG)
        else
            sample = ((rand() * (int)(count & 0xffff)) >> 15) + 1;   // CRT rand 0x78c846: audio only
        // 0x5f43b0 builds "speech/%s/%s/%s_%02d.mp3" (it fails only out of
        // range), then the entry goes into the World+0x726c queue that
        // UpdateSpeech 0x607f50 plays.
        if ((unsigned)race < 8 && (unsigned)group < 0x1b && (unsigned)event < 0x20) {
            char name[260];
            _snprintf(name, sizeof(name) - 1, "speech/%s/%s/%s_%02d.mp3", kSpeechRace[race], kSpeechGroup[group],
                      kSpeechEvent[event], sample);               // 0x52da80
            name[sizeof(name) - 1] = 0;
            int i = SpeechAlloc();                                // 0x5d8ed0
            SSpeechEntry& e = g_UnitSpeech.Array[i];              // 0x5d6590
            e.Unit = unit;
            e.Name = _strdup(name);                               // 0x52c2c0
            e.AnyPlayer = anyPlayer;
            e.Race = race;
            e.Group = group;
            e.Event = event;
            e.Sample = sample;
            e.Priority = priority;
            e.Time = now;
            *s.LastTime(event) = now;
        }
    }
}

} // namespace pz
