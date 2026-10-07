// src/game/skirmish_game.cpp
// The game side of the single-player Skirmish (M5-SK, docs/m5/sk.md): the
// SMulti branches of the SGameLogic ctor 0x55e440 (players from the room's
// slots), PlaceAllUnits 0x571c70 (the armies, the Computer armies from the
// hard-coded tables, their "SKIRMISH_AI %d" groups and the attack order),
// Refresh 0x576d80 (who lost, victory) and SGameView::Update 0x628430 (the
// local player out while a team mate fights on). Offline the logic stays on
// the single-player frame path (no lockstep: SendFrameData 0x5212a0 and the
// per-player frame buffers are not needed with one host).
// OWNER: agent SK (M5).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "skirmish_state.h"
#include "gamelogic.h"
#include "world.h"
#include "worldapi.h"
#include "unit.h"
#include "punit.h"
#include "aigroup.h"
#include "pzunitregistry.h"
#include "logger.h"
#include "gettext.h"
#include "m3common.h"

PzSkirmish* g_Skirmish = nullptr;

namespace pz { void PzMessageFading(SGameLogic* gl, const char* text, int color); }   // tutorial_msg.cpp 0x56a480

using namespace pz;

static unsigned char* Player(SWorld* w, int p) { return w->Players[p]; }
static int& PType(SWorld* w, int p) { return *(int*)(Player(w, p) + 0x08); }   // World+0x178: 0 human, 1 computer, 2 none
static int& PTeam(SWorld* w, int p) { return *(int*)(Player(w, p) + 0x0c); }   // World+0x17c
static int& PRace(SWorld* w, int p) { return *(int*)(Player(w, p) + 0x04); }   // World+0x174
static int& PStart(SWorld* w, int p) { return *(int*)(Player(w, p) + 0x18); }  // World+0x188
static int& PState(SWorld* w, int p) { return *(int*)(Player(w, p) + 0x1c); }  // World+0x18c: 0 playing, 1 won, 2 lost

// World+0x7518: the world LCG (MSVC rand), 15 bits.
static int WorldRand15(SWorld* w)
{
    w->RandomSeed = w->RandomSeed * 0x343fd + 0x269ec3;
    return (int)((w->RandomSeed >> 16) & 0x7fff);
}

// ---------------------------------------------------------------------------
// The Computer armies (HD 0x7f6958..0x7f75b0: {count, unit name}, ending with
// a 0 count). Variant A / B by a 50/50 draw; by game age, prestige limit and
// nation (1 Allied, 0 German, 2 Russian).
struct SkEntry { int Count; const char* Name; };

static const SkEntry kA1500GB[] = { {4,"GB Matilda mkii"},{1,"GB Crusader"},{1,"US M2A1 support"},{2,"GB Rifle Squad"},{0,0} };                       // 0x7f6958
static const SkEntry kB1500GB[] = { {2,"GB Matilda mkii"},{1,"GB Crusader"},{1,"US M2A1 support"},{4,"GB Mortar Squad"},{3,"GB OldAT Squad"},{2,"GB MG Squad"},{2,"GB Medic Squad"},{0,0} };   // 0x7f69c0
static const SkEntry kA2000GB[] = { {4,"GB Matilda mkii"},{2,"GB Crusader"},{2,"US M2A1 support"},{2,"GB Mortar Squad"},{1,"GB Rifle Squad"},{1,"GB Medic Squad"},{0,0} };   // 0x7f6a3c
static const SkEntry kB2000GB[] = { {2,"GB Matilda mkii"},{1,"GB Crusader"},{1,"GB humberMkII"},{5,"GB Mortar Squad"},{3,"GB OldAT Squad"},{2,"GB Sniper Squad"},{2,"GB Medic Squad"},{2,"GB MG Squad"},{1,"US M2A1 support"},{0,0} };   // 0x7f6a78
static const SkEntry kA2500GB[] = { {6,"GB Matilda mkii"},{2,"GB Crusader"},{2,"US M2A1 support"},{1,"GB Mortar Squad"},{1,"GB MG Squad"},{1,"GB OldAT Squad"},{0,0} };   // 0x7f6ae8
static const SkEntry kB2500GB[] = { {3,"GB Matilda mkii"},{1,"GB Crusader"},{1,"GB humberMkII"},{1,"US M2A1 support"},{5,"GB Mortar Squad"},{2,"GB Sniper Squad"},{5,"GB OldAT Squad"},{4,"GB Medic Squad"},{3,"GB MG Squad"},{0,0} };   // 0x7f6b20
static const SkEntry kA2000US[] = { {5,"US Sherman"},{2,"Gb Bishop"},{1,"US M2A1 support"},{1,"US Mortar Squad"},{2,"US Rifle Squad"},{0,0} };   // 0x7f6b70
static const SkEntry kB2000US[] = { {3,"US Sherman"},{1,"US M2A1 support"},{6,"US Rocket Squad"},{6,"US Mortar Squad"},{2,"US Medic Squad"},{0,0} };   // 0x7f6bd8
static const SkEntry kA3000US[] = { {2,"Us M-26-Pershing"},{5,"US Sherman"},{2,"US M7-Priest"},{2,"US M2A1 support"},{2,"US Rocket Squad"},{1,"US Mortar Squad"},{0,0} };   // 0x7f6c28
static const SkEntry kB3000US[] = { {3,"US Sherman"},{1,"GB Firefly"},{2,"Us M-26-Pershing"},{6,"US Rocket Squad"},{6,"US Mortar Squad"},{2,"US MG Squad"},{4,"US Medic Squad"},{0,0} };   // 0x7f6c88
static const SkEntry kA4000US[] = { {3,"Us M-26-Pershing"},{5,"GB Firefly"},{4,"US M7-Priest"},{2,"US M2A1 support"},{3,"US Rocket Squad"},{1,"US Sniper Squad"},{0,0} };   // 0x7f6ce0
static const SkEntry kB4000US[] = { {3,"Us M-26-Pershing"},{1,"US M2A1 support"},{2,"GB Firefly"},{8,"US Rocket Squad"},{2,"US Sniper Squad"},{8,"US Mortar Squad"},{6,"US Flamethrower Squad"},{4,"US Medic Squad"},{0,0} };   // 0x7f6d28
static const SkEntry kA1500Ge[] = { {4,"Ge PZ IV D"},{2,"Ge Sdkfz 11 hansa-lloyd"},{1,"Ge Mortar Squad"},{1,"Ge MG Squad"},{2,"Ge Medic Squad"},{0,0} };   // 0x7f6d88
static const SkEntry kB1500Ge[] = { {2,"Ge PZ IV D"},{1,"Ge PZ II"},{1,"Ge Sdkfz223"},{5,"Ge Mortar Squad"},{3,"Ge AT Squad"},{1,"Ge MG Squad"},{1,"Ge Sniper Squad"},{2,"Ge Medic Squad"},{0,0} };   // 0x7f6e08
static const SkEntry kA2000Ge[] = { {5,"Ge PZ IV D"},{1,"Ge PZ I"},{1,"Ge Sdkfz223"},{2,"Ge Sdkfz 11 hansa-lloyd"},{1,"Ge Sniper Squad"},{2,"Ge Rifle Squad"},{0,0} };   // 0x7f6e84
static const SkEntry kB2000Ge[] = { {2,"Ge PZ IV D"},{2,"Ge PZ II"},{1,"Ge PZ I"},{1,"Ge Sdkfz223"},{1,"Ge Sdkfz 11 hansa-lloyd"},{4,"Ge Mortar Squad"},{4,"Ge AT Squad"},{3,"Ge Medic Squad"},{2,"Ge Sniper Squad"},{0,0} };   // 0x7f6ed8
static const SkEntry kA2500Ge[] = { {7,"Ge PZ IV D"},{2,"Ge Sdkfz 11 hansa-lloyd"},{1,"Ge Sdkfz223"},{2,"Ge Mortar Squad"},{1,"Ge Sniper Squad"},{2,"Ge Medic Squad"},{0,0} };   // 0x7f6f28
static const SkEntry kB2500Ge[] = { {4,"Ge PZ IV D"},{1,"Ge PZ II"},{1,"Ge Sdkfz 11 hansa-lloyd"},{5,"Ge Mortar Squad"},{2,"Ge AT Squad"},{3,"Ge MG Squad"},{3,"Ge Sniper Squad"},{2,"Ge Medic Squad"},{1,"Ge Sdkfz223"},{0,0} };   // 0x7f6f60
static const SkEntry kA2000GeL[] = { {2,"Ge Tiger"},{2,"Ge PZ IV F2"},{2,"Ge Wespe"},{1,"Ge Hetzer"},{1,"Ge Sdk 10 demag"},{1,"Ge MG Squad"},{0,0} };   // 0x7f6fb0
static const SkEntry kB2000GeL[] = { {2,"Ge PZ IV F2"},{1,"Ge Hetzer"},{2,"Ge Jagdpanther"},{7,"Ge Mortar Squad"},{3,"Ge MG Squad"},{4,"Ge Medic Squad"},{0,0} };   // 0x7f7028
static const SkEntry kA3000Ge[] = { {3,"Ge Tiger"},{1,"Ge Panzerwerfer"},{4,"Ge Panther A"},{2,"Ge Sdkfz 11 hansa-lloyd"},{3,"Ge Mortar Squad"},{1,"Ge MG Squad"},{0,0} };   // 0x7f7070
static const SkEntry kB3000Ge[] = { {2,"Ge Panther A"},{1,"Ge Hetzer"},{3,"Ge Sdk 10 demag"},{6,"Ge Rocket Squad"},{6,"Ge Mortar Squad"},{3,"Ge MG Squad"},{5,"Ge Flamethrower Squad"},{2,"Ge Medic Squad"},{0,0} };   // 0x7f70c8
static const SkEntry kA4000Ge[] = { {1,"Ge Sturmtiger"},{4,"Ge Tiger"},{2,"Ge Sdkfz 11 hansa-lloyd"},{6,"Ge PZ IV F2"},{2,"Ge MG Squad"},{1,"Ge Krupp Ammo"},{0,0} };   // 0x7f7138
static const SkEntry kB4000Ge[] = { {4,"Ge PzVI TigerII"},{2,"Ge Wespe"},{1,"Ge Sig 33i"},{2,"Ge Sdkfz 11 hansa-lloyd"},{5,"Ge Rocket Squad"},{7,"Ge Mortar Squad"},{5,"Ge MG Squad"},{2,"Ge Sniper Squad"},{4,"Ge Medic Squad"},{0,0} };   // 0x7f7190
static const SkEntry kA1500SU[] = { {5,"SU T-34 Model40"},{1,"Su Zis-20-halftr"},{2,"SU AT Squad"},{0,0} };   // 0x7f71fc
static const SkEntry kB1500SU[] = { {3,"SU T-34 Model40"},{1,"Su Zis-20-halftr"},{4,"SU AT Squad"},{2,"SU Medic Squad"},{3,"SU Mortar Squad"},{2,"SU MG Squad"},{0,0} };   // 0x7f724c
static const SkEntry kA2000SU[] = { {6,"SU T-34 Model40"},{2,"Su Zis-20-halftr"},{2,"SU AT Squad"},{1,"SU MG Squad"},{0,0} };   // 0x7f72b0
static const SkEntry kB2000SU[] = { {3,"SU T-34 Model40"},{1,"SU T-26"},{1,"SU Ba-64"},{1,"Su Zis-20-halftr"},{4,"SU Mortar Squad"},{3,"SU AT Squad"},{1,"SU Sniper Squad"},{2,"SU Medic Squad"},{2,"SU MG Squad"},{0,0} };   // 0x7f72d8
static const SkEntry kA2500SU[] = { {6,"SU T-34 Model40"},{2,"SU T-26"},{2,"Su Zis-20-halftr"},{2,"SU Mortar Squad"},{1,"SU AT Squad"},{1,"SU MG Squad"},{1,"SU Medic Squad"},{0,0} };   // 0x7f7350
static const SkEntry kB2500SU[] = { {3,"SU T-34 Model40"},{2,"SU T-26"},{1,"SU Ba-64"},{1,"Su Zis-20-halftr"},{5,"SU Mortar Squad"},{2,"SU Sniper Squad"},{3,"SU Medic Squad"},{3,"SU MG Squad"},{3,"SU AT Squad"},{0,0} };   // 0x7f7390
static const SkEntry kA2000SUL[] = { {4,"Su T-34_85"},{1,"Su Zis-20-halftr"},{2,"SU SU-122"},{1,"SU Rocket Squad"},{1,"SU Mortar Squad"},{0,0} };   // 0x7f73e0
static const SkEntry kB2000SUL[] = { {3,"Su T-34_85"},{5,"SU Mortar Squad"},{3,"SU Rocket Squad"},{1,"SU MG Squad"},{1,"SU Medic Squad"},{1,"SU KV-2"},{0,0} };   // 0x7f7438
static const SkEntry kA3000SU[] = { {4,"Su T-34_85"},{2,"SU KV-2"},{2,"Su Zis-20-halftr"},{1,"SU Katyusha-Zis-6"},{1,"SU Rocket Squad"},{1,"SU MG Squad"},{1,"SU Gaz AA ammo"},{0,0} };   // 0x7f7478
static const SkEntry kB3000SU[] = { {3,"Su T-34_85"},{1,"SU SU-85"},{7,"SU Rocket Squad"},{6,"SU Mortar Squad"},{2,"SU Sniper Squad"},{5,"SU Medic Squad"},{2,"SU MG Squad"},{2,"SU Flamethrower Squad"},{0,0} };   // 0x7f74e0
static const SkEntry kA4000SU[] = { {4,"Su T-34_85"},{2,"SU Is-2"},{2,"SU KV-2"},{1,"SU Isu-152"},{1,"SU SU-122"},{1,"SU Gaz AA ammo"},{2,"SU MG Squad"},{2,"Su Zis-20-halftr"},{0,0} };   // 0x7f7550
static const SkEntry kB4000SU[] = { {3,"Su T-34_85"},{1,"SU KV-2"},{1,"SU Is-2"},{1,"Su Zis-20-halftr"},{8,"SU Rocket Squad"},{5,"SU Mortar Squad"},{4,"SU Medic Squad"},{2,"SU Sniper Squad"},{5,"SU Flamethrower Squad"},{2,"SU MG Squad"},{0,0} };   // 0x7f75b0

// [age][limit 0/1/2][variant][nation: 0 German, 1 Allied, 2 Russian]
static const SkEntry* const kTables[2][3][2][3] = {
    { { { kA1500Ge, kA1500GB, kA1500SU }, { kB1500Ge, kB1500GB, kB1500SU } },
      { { kA2000Ge, kA2000GB, kA2000SU }, { kB2000Ge, kB2000GB, kB2000SU } },
      { { kA2500Ge, kA2500GB, kA2500SU }, { kB2500Ge, kB2500GB, kB2500SU } } },
    { { { kA2000GeL, kA2000US, kA2000SUL }, { kB2000GeL, kB2000US, kB2000SUL } },
      { { kA3000Ge, kA3000US, kA3000SU }, { kB3000Ge, kB3000US, kB3000SU } },
      { { kA4000Ge, kA4000US, kA4000SU }, { kB4000Ge, kB4000US, kB4000SU } } },
};

static void SetName(SString* s, const char* text)
{
    *s = text ? text : "";
}

// The army part of 0x571c70's Computer branch: count blank records
// (0x5cfb10) per entry, named after the entry; a prototype that needs a
// crew (+0xdc) gets a stored "<first 3 chars>Crew Squad", a transport
// (class 0xb) a stored "<first 3 chars>Rifle Squad"; Behavior (+0x38) = 0.
void PzSkirmishBuildAIArmy(SArmyArray* out, int nation, int age, int limit, int variant)
{
    int l;
    if (age == 0)
        l = limit == 1500 ? 0 : limit == 2000 ? 1 : 2;            // 0x5dc / 2000 / else
    else
        l = limit == 2000 ? 0 : limit == 3000 ? 1 : 2;            // 2000 / 3000 / else
    int n = nation == 1 ? 1 : nation == 0 ? 0 : 2;
    const SkEntry* t = kTables[age ? 1 : 0][l][variant ? 1 : 0][n];
    SUnitDef blank;                                               // 0x5cfb10
    for (; t->Count != 0; ++t) {
        for (int k = 0; k < t->Count; ++k) {
            int idx = out->Size;
            UnitDefCopy(ArmyAdd(out), &blank);                    // grow 16 / 6/5, 0x560290(blank)
            SUnitDef* d = &out->Array[idx];
            SetName(&d->ClassName, t->Name);
            SPUnit* proto = g_UnitRegistry->GetPUnit(t->Name, false);   // 0x5d0e70(name, 0)
            const char* sub = nullptr;
            if (proto && proto->OnlyCrew)                          // +0xdc
                sub = "Crew Squad";                               // 0x7f763c
            else if (proto && proto->ClassType == 0xb)            // +0x40
                sub = "Rifle Squad";                              // 0x7f7648
            if (sub) {
                char name[64];
                _snprintf(name, sizeof(name) - 1, "%.3s%s", t->Name, sub);   // Left(name, 3) + sub
                name[sizeof(name) - 1] = 0;
                SArmyArray st = { d->StoredUnits, d->StoredCount, d->StoredMax };
                UnitDefCopy(ArmyAdd(&st), &blank);
                d->StoredUnits = st.Array;
                d->StoredCount = st.Size;
                d->StoredMax = st.Max;
                SetName(&d->StoredUnits[d->StoredCount - 1].ClassName, name);
            }
        }
    }
    for (int i = 0; i < out->Size; ++i)
        out->Array[i].Behavior = 0;                               // +0x38 = 0
}

// PZ_M5_SK_ARMY (recompile-only test hook, src/panzers/skirmish.cpp): the
// local player gets variant A of the Computer army of its nation.
void PzSkirmishTestArmy(SArmyArray* out, int nation)
{
    if (!g_Skirmish)
        return;
    PzSkirmishBuildAIArmy(out, nation, g_Skirmish->GameAge, g_Skirmish->PrestigeLimit, 0);
}

// ---------------------------------------------------------------------------

// SGameLogic ctor 0x55e440 (0x55f02x..0x55f293), the SMulti part (game type
// != 3): the local player is the local slot; a connected slot is a human
// (type 0), a Computer slot type 1, the others none (type 2, team 0); team
// 1 for slots 0..3, 2 for 4..7, the race from the slot's nation. These
// override the map's PLY3 records.
void PzSkirmishSetupPlayers(SWorld* w)
{
    PzSkirmish* m = g_Skirmish;
    if (!m || !w)
        return;
    w->LocalPlayer = m->MySlot;                                   // World+0x16c = SMulti +0x4790
    for (int s = 0; s < 8; ++s) {
        const PzSkirmishSlot& slot = m->Slots[s];
        int team = s < 4 ? 1 : 2;
        unsigned char* p = Player(w, s);
        if (slot.Connected) {
            PType(w, s) = 0;
            PTeam(w, s) = team;
            PRace(w, s) = slot.Nation;
            p[0x10] = 1;
        } else if (slot.Status == 3) {
            PType(w, s) = 1;
            PTeam(w, s) = team;
            PRace(w, s) = slot.Nation;
            p[0x10] = 0;
        } else {
            PType(w, s) = 2;
            PTeam(w, s) = 0;
            p[0x10] = 0;
        }
    }
    Logger.g->Log(0, "PZM5: skirmish players set up, local player %d", w->LocalPlayer);
}

// flying.cpp's AIGroupAlloc 0x55cd40 / AIGroupInit 0x5ecbe0 are file-static
// there; the same code over World+0x4f4.
static int AIGroupAlloc(SWorld* w)                                // 0x55cd40
{
    SHeap<unsigned char[0x50]>& h = w->AIGroups;
    h.Count++;
    int i = h.Free;
    if (i >= 0) {
        h.Free = h.Array[i].Next;
        h.Array[i].Next = kHeapLive;
        memset(&h.Array[i].Data, 0, 0x50);
        return i;
    }
    if (h.Size == h.Max) {
        int n = h.Max < 0x10 ? 0x10 : (h.Max * 6) / 5;
        h.Array = (SHeapElem<unsigned char[0x50]>*)realloc(h.Array, (size_t)n * 0x54);
        memset((char*)h.Array + (size_t)h.Max * 0x54, 0, (size_t)(n - h.Max) * 0x54);
        h.Max = n;
    }
    h.Array[h.Size].Next = kHeapLive;
    return h.Size++;
}

static SAIGroup* AIGroupAt(SWorld* w, int g)
{
    return reinterpret_cast<SAIGroup*>(&w->AIGroups.Array[g].Data);
}

static void AIGroupInit(SAIGroup* g)                              // 0x5ecbe0
{
    g->Timer = 0;                                                 // +0x10
    g->AttackMoveTimer = 0;                                       // +0x14
    g->PathIdx = -1;                                              // +0x34
    g->AttackTarget = false;                                      // +0x24
    g->MaxHelpRange = 600.0f;                                     // +0x30 (0x44160000)
    g->StartPos[0] = 0.0f;                                        // +0x1c
    g->StartPos[1] = 0.0f;                                        // +0x20
    g->ComputeStartPos();                                         // 0x601060
}

// SGameLogic::PlaceAllUnits 0x571c70, the SMulti branch (game type != 3).
// Returns false without a skirmish (the single-player path runs).
bool PzSkirmishPlaceAllUnits(SGameLogic* logic)
{
    PzSkirmish* m = g_Skirmish;
    SWorld* w = g_World;
    if (!m || !w)
        return false;
    PZ_M3_TRACE("SGameLogic::PlaceAllUnits skirmish (0x571c70)");
    for (int s = 0; s < 8; ++s) {
        const PzSkirmishSlot& slot = m->Slots[s];
        if (slot.Connected) {
            if (m->Armies[s].Size == 0)
                Logger.g->Panic("SGameLogic::PlaceAllUnits() player: %d, units: %d", s, m->Armies[s].Size);   // 0x572cbe
            logic->PlaceUnits(s, &m->Armies[s]);                  // 0x572ec0(s, SMulti +0x4820 + s * 0x34)
            continue;
        }
        if (slot.Status != 3)
            continue;
        int variant = (int)((double)WorldRand15(w) * (1.0 / 32768.0) * 2.0);   // 0x7f4540, 0x7ea768
        SArmyArray army = { nullptr, 0, 0 };
        PzSkirmishBuildAIArmy(&army, slot.Nation, m->GameAge, m->PrestigeLimit, variant);
        logic->PlaceUnits(s, &army);                              // 0x572ec0(s, &army)
        int g = AIGroupAlloc(w);                                  // 0x55cd40
        SAIGroup* grp = AIGroupAt(w, g);
        AIGroupInit(grp);                                         // 0x5ecbe0
        char id[32];
        sprintf(id, "SKIRMISH_AI %d", s);                         // 0x51ee20
        grp->ID = id;
        for (int i = 0; i < w->Units.Size; ++i) {
            if (!w->Units.IsLive(i))
                continue;
            SUnit* u = w->Units.Array[i].Unit;
            if (u->Player == s && u->Parent == -1)                // +0xfc, +0x78
                u->SetAIGroup(g);                                 // 0x5c0c10
        }
        grp = AIGroupAt(w, g);
        grp->ComputeStartPos();                                   // 0x601060
        grp->Tactic = 4;                                          // +0x08
        // A random enemy (a human or Computer of the other team) and its "start %d".
        int enemies[8];
        int n = 0;
        for (int j = 0; j < 8; ++j) {
            if (j == s || (PType(w, j) != 0 && PType(w, j) != 1))
                continue;
            if (PTeam(w, s) != 0 && PTeam(w, j) == PTeam(w, s))
                continue;
            enemies[n++] = j;
        }
        if (n > 0) {
            int k = (int)((double)WorldRand15(w) * (1.0 / 32768.0) * (double)n);
            char name[32];
            sprintf(name, "start %d", PStart(w, enemies[k]) + 1);
            int loc = -1;
            for (int i = 0; i < w->Locations.Size && loc < 0; ++i)
                if (w->Locations.IsLive(i) && _stricmp(w->Locations.Array[i].Data.Name.buf ? w->Locations.Array[i].Data.Name.buf : "", name) == 0)
                    loc = i;
            if (loc >= 0) {
                // HD: GroupStats 0x582770 over the player's top-level units first (its result is not used).
                const SLocation& a = w->Locations.Array[loc].Data;
                float x = (float)(a.X1 + a.X2) * 0.5f;            // 0x7f453c
                float z = (float)(a.Z1 + a.Z2) * 0.5f;
                AIGroupAt(w, g)->AttackMoveTo(x, z);               // 0x5d95b0
                Logger.g->Log(0, "PZM5: %s (%d units) attacks player %d at %s", id, AIGroupAt(w, g)->Units.Size, enemies[k], name);
            }
        }
        ArmyFree(&army);
    }
    return true;
}

// SGameLogic::Refresh 0x576d80, after RunTriggers: every 20th frame after
// frame 100, a human or Computer without a live unit of class 0, 0xb or 5
// has lost ("Computer %d has lost the game" / "%s has lost the game"); a
// human out takes his team out (skirmish); when only the local player's
// team is left, the mission result is 1 (victory). Assault: the defenders
// (players 4..7) lose when no unit named "Assault" of theirs is left.
void PzSkirmishCheckPlayers(SGameLogic* logic, int frame)
{
    PzSkirmish* m = g_Skirmish;
    SWorld* w = g_World;
    if (!m || !w || m->GameType == 3 || frame % 20 != 0 || frame <= 100)
        return;
    if (frame % 600 == 0) {                                       // (recompile) a log line per 30 s: the AI groups and the players
        for (int g = 0; g < w->AIGroups.Size; ++g) {
            if (w->AIGroups.Array[g].Next != kHeapLive)
                continue;
            SAIGroup* grp = AIGroupAt(w, g);
            if (!grp->ID.buf || strncmp(grp->ID.buf, "SKIRMISH_AI", 11) != 0)
                continue;
            float x = 0, z = 0;
            int n = 0;
            for (int i = 0; i < grp->Units.Size; ++i) {
                int u = grp->Units.Array[i];
                if (!w->Units.IsLive(u))
                    continue;
                x += w->Units.Array[u].Unit->Pos[0];
                z += w->Units.Array[u].Unit->Pos[2];
                ++n;
            }
            Logger.g->Log(0, "PZM5: frame %d %s tactic %d status %d, %d live units at (%.0f, %.0f)", frame, grp->ID.buf,
                          grp->Tactic, grp->Status, n, n ? x / n : 0.0f, n ? z / n : 0.0f);
        }
    }
    for (int p = 0; p < 12; ++p) {
        if (PState(w, p) == 2 || (PType(w, p) != 0 && PType(w, p) != 1))
            continue;
        bool alive = false;
        for (int i = 0; i < w->Units.Size && !alive; ++i) {
            if (!w->Units.IsLive(i))
                continue;
            SUnit* u = w->Units.Array[i].Unit;
            if (u->Player != p || u->_110)                        // +0xfc, +0x110
                continue;
            int ct = u->Proto ? u->Proto->ClassType : -1;          // +0x04 -> +0x40
            if (ct == 0 || ct == 0xb || ct == 5)
                alive = true;
        }
        if (alive)
            continue;
        PState(w, p) = 2;
        char text[160];
        if (PType(w, p) == 1)
            _snprintf(text, sizeof(text) - 1, GetText("world/GameLogic.cpp", "Computer %d has lost the game"), p + 1);
        else
            _snprintf(text, sizeof(text) - 1, GetText("world/GameLogic.cpp", "%s has lost the game"), p < 8 ? m->Slots[p].Name : "");
        text[sizeof(text) - 1] = 0;
        PzMessageFading(logic, text, 4);                          // 0x56a480(text, 4)
        Logger.g->Log(0, "PZM5: %s", text);
        if (m->Skirmish && PType(w, p) == 0) {                    // +0x512c: a human out takes his team out
            for (int q = 0; q < 12; ++q)
                if (PTeam(w, p) ? PTeam(w, q) == PTeam(w, p) : q == p)
                    PState(w, q) = 2;
        }
    }
    int me = w->LocalPlayer;
    bool won = true;
    for (int p = 0; p < 12; ++p) {
        if (PState(w, p) == 2 || (PType(w, p) != 0 && PType(w, p) != 1))
            continue;
        if (PTeam(w, me) ? PTeam(w, p) != PTeam(w, me) : p != me)
            won = false;
    }
    if (won && g_Campaign)
        g_Campaign->MissionResult = 1;                            // 0x57779c: Campaign +0xe4 = 1
    if (m->GameType == 2) {                                       // Assault (0x5777f4)
        bool assault = false;
        for (int i = 0; i < w->Units.Size && !assault; ++i) {
            if (!w->Units.IsLive(i))
                continue;
            SUnit* u = w->Units.Array[i].Unit;
            if (u->Player >= 4 && u->ScriptID.buf && _stricmp(u->ScriptID.buf, "Assault") == 0)   // +0x194
                assault = true;
        }
        if (!assault) {
            for (int p = 4; p < 8; ++p)
                if (PType(w, p) == 0 || PType(w, p) == 1)
                    PState(w, p) = 2;
            *(int*)((unsigned char*)w + 0x45c) = 2;               // World+0x45c = 2
        }
    }
}

// SGameView::Update 0x628430 with SMulti (not coop): the local player is out
// but a player of his team (type 0 / 1, not lost) still fights: the game
// goes on (the player watches).
bool PzSkirmishTeammateAlive(SWorld* w)
{
    if (!g_Skirmish || !w)
        return false;
    int me = w->LocalPlayer;
    int team = PTeam(w, me);
    if (team == 0)
        return false;
    for (int p = 0; p < 12; ++p)
        if (p != me && PTeam(w, p) == team && PState(w, p) != 2 && (PType(w, p) == 0 || PType(w, p) == 1))
            return true;
    return false;
}
