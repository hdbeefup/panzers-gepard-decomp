// src/game/gamelogic_mission.cpp
// SGameLogic at mission start and end (gamelogic.h block "M3 agent F"):
// placing the campaign army, the -packetrec / -packetplay streams, the
// map-load helpers of SGameView::LoadMap, the game part of the save.
// OWNER: agent F (docs/M3_INTERFACES.md). Lifted from the HD exe.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gamelogic.h"
#include "campaign.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "punit.h"
#include "unit.h"
#include "triggersunits.h"
#include "stream.h"
#include "logger.h"
#include "stub_log.h"
#include "pz/iterrain.h"
#include "pz/hdmath.h"
#include "doodad.h"

namespace pz {

using m2u::UV;

// PANZERS 0x5f4f60 (SWorld: camera target, clamped to the camera area
// unless the camera is locked)
static void WorldSetCameraTarget(SWorld* w, float x, float z)
{
    if (w->CamLocked != 0)
        return;
    if (x < w->CamXMin || w->CamXMax < x)
        x = x < w->CamXMin ? w->CamXMin : w->CamXMax;
    if (z < w->CamZMin || w->CamZMax < z)
        z = z < w->CamZMin ? w->CamZMin : w->CamZMax;
    w->CamTarget[0] = x;
    w->CamTarget[2] = z;
    w->CamFollowPath = 0;                                         // +0xa0
    w->CamFollowUnit = -1;                                        // +0xa4
}

// PANZERS 0x6093d0 (SWorld: camera distance, clamped)
static void WorldSetCameraDist(SWorld* w, float d)
{
    float lo = w->CamDistMin;
    if (lo <= d) {
        lo = w->CamDistMax;
        if (d <= lo) {
            w->CamDist = d;
            return;
        }
    }
    w->CamDist = lo;
}

// PANZERS 0x571c70 (single-player path; the multiplayer armies and the
// skirmish starting units of DAT_008f1a74 are not lifted)
void SGameLogic::PlaceAllUnits()
{
    PZ_M3_TRACE("SGameLogic::PlaceAllUnits (0x571c70)");
    SArmyArray army = { nullptr, 0, 0 };
    if (g_Campaign)
        g_Campaign->GetMissionArmy(&army);                        // 0x591e70
    PlaceUnits(g_World->LocalPlayer, &army);                      // 0x572ec0(World+0x16c, &army)
    ArmyFree(&army);                                              // 0x51de90
}

// PANZERS 0x572ec0
// The army goes to the location "start <n>" (n = World player +0x18 + 1)
// in a grid of 4 m cells, (X2 - X1) / 4 per row, facing the map centre;
// each unit at the free spot nearest its cell (0x5e58d0). The local
// player's camera then looks at the location.
void SGameLogic::PlaceUnits(int player, SArmyArray* army)
{
    PZ_M3_TRACE("SGameLogic::PlaceUnits (0x572ec0)");
    SWorld* w = g_World;
    if (army->Size == 0)
        return;
    char name[32];
    sprintf(name, "start %d", *(int*)(w->Players[player] + 0x18) + 1);   // World+0x188 + player*0x48
    // HD: multiplayer mode 3, player 7 builds the name of player 0 too (unused).
    int loc = -1;
    for (int i = 0; i < w->Locations.Size; ++i) {
        if (!w->Locations.IsLive(i))
            continue;
        if (_stricmp(SStr(w->Locations.Array[i].Data.Name), name) == 0) {
            loc = i;
            break;
        }
    }
    if (loc < 0)
        return;
    if (!g_Campaign || !g_Campaign->PlaceMyUnits())               // 0x596650
        return;
    const SLocation* l = &w->Locations.Array[loc].Data;
    float cx = (float)((double)(l->X2 + l->X1) * 0.5);            // cvtdq2pd, mulsd 0.5 (0x7ea760), cvtpd2ps
    float cz = (float)((double)(l->Z2 + l->Z1) * 0.5);
    double dy = (double)w->TerrainW * 0.5 - (double)cx;
    double dx = (double)w->TerrainH * 0.5 - (double)cz;
    float dir = HdAtan2f(dy, dx);                                 // 0x78d07a(ST1 = dy, ST0 = dx)
    Logger.g->Log(1, "SGameLogic::PlaceUnits player: %d, units: %d", player, army->Size);
    int perRow = (l->X2 - l->X1) / 4;                             // cdq, and 3, sar 2
    for (int i = 0; i < army->Size; ++i) {
        int gx = (i % perRow) * 4 + 1 + l->X1;
        int gz = (i / perRow) * 4 + 1 + l->Z1;
        float fx = (float)gx;
        float fz = (float)gz;
        if (!(0.0f <= fx))
            continue;
        SUnitDef* d = &army->Array[i];
        d->Player = player;                                       // +0x08
        d->Pos[0] = fx;
        d->Pos[1] = fz;
        d->Dir = dir;                                             // +0x1c
        d->Stored = true;                                         // +0x48: placed by +0x50 below
        if (d->StoredCount > 0)
            d->StoredUnits[0].Player = player;
        int u = w->CreateUnit(d);                                 // 0x5e2da0
        if (!w->Units.IsLive(u))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", u);
        float out[2];
        w->FindEmptySpace(out, fx, fz, dir + 3.1415927f, UV::RawInt(u, 0x58), UV::FlagsD8(u), true);   // 0x5e58d0
        UV::Iface(u)->Place(out[0], out[1], dir);                 // +0x50
    }
    if (player == w->LocalPlayer) {
        WorldSetCameraTarget(w, cx, cz);                          // 0x5f4f60
        w->SetCameraAngles(dir, (w->CamPitchMax + w->CamPitchMin) * 0.5f);   // 0x5f83b0
        WorldSetCameraDist(w, (w->CamDistMax + w->CamDistMin) * 0.5f);      // 0x6093d0
    }
}

// The army record's XP dword (+0x0c) holds a float in HD (unit +0x64 + 100).
static float DefXP(const SUnitDef* d) { float f; memcpy(&f, &d->XP, 4); return f; }
static void SetDefXP(SUnitDef* d, float f) { memcpy(&d->XP, &f, 4); }

// The record (its crew first, then itself) whose ScriptID is the unit's
// gets the unit's XP + 100 (0x561110 inner loop).
static void MarkSurvivor(SArmyArray* army, const SUnit* u)
{
    for (int i = 0; i < army->Size; ++i) {
        SUnitDef* d = &army->Array[i];
        if (d->StoredCount > 0) {
            SUnitDef* c = &d->StoredUnits[0];
            if (c->ScriptID.size == u->ScriptID.size &&
                (c->ScriptID.size == 0 || _stricmp(c->ScriptID.buf, u->ScriptID.buf) == 0)) {
                SetDefXP(c, u->XP + 100.0f);                      // DAT_007ee558
                return;
            }
        }
        if (d->ScriptID.size == u->ScriptID.size &&
            (d->ScriptID.size == 0 || _stricmp(d->ScriptID.buf, u->ScriptID.buf) == 0)) {
            SetDefXP(d, u->XP + 100.0f);
            return;
        }
    }
}

// PANZERS 0x561110
// Mission end (single-player part; the multiplayer army save and SMulti
// names are not in the recompile). When the player's units were placed
// (PlaceMyUnits 0x596650) the mission army's records learn which units
// survived, by difficulty (campaign +0x1c): Easy keeps the dead units' XP,
// Normal resets it to 0 (greenhorn replacements), Hard drops the dead ones
// (a surviving crew becomes its own record; a dead crew of a vehicle with
// a built-in driver only loses the crew). Then per player the live units
// that count (0x56d6d0, not +0x110), the losses percent (+0xd0), the
// mission time (frames / 20, 0x5974e0) and the score (+0xb60: XP and kills
// of the local player minus the time, +5000 on victory, +8000 per optional
// and +6000 per secret objective done).
void SGameLogic::BackupCampaignUnits()
{
    PZ_M3_TRACE("SGameLogic::BackupCampaignUnits (0x561110)");
    SWorld* w = g_World;
    SPanzersCampaign* c = g_Campaign;
    if (!w || !c)
        return;
    int live[12] = {};
    SArmyArray* army = &c->MissionArmy;                           // +0x3c
    if (c->PlaceMyUnits()) {                                      // 0x596650
        if (c->Difficulty == 1 || c->Difficulty == 2) {
            float reset = c->Difficulty == 1 ? 0.0f : -1.0f;      // 0 / 0xbf800000
            for (int i = 0; i < army->Size; ++i) {
                SetDefXP(&army->Array[i], reset);
                if (army->Array[i].StoredCount > 0)
                    SetDefXP(&army->Array[i].StoredUnits[0], reset);
            }
        }
        for (int i = 0; i < w->Units.Size; ++i)
            if (w->Units.IsLive(i))
                MarkSurvivor(army, w->Units.Array[i].Unit);
        if (c->Difficulty == 2) {
            for (int i = 0; i < army->Size; ++i) {
                SUnitDef* d = &army->Array[i];
                bool remove = false;
                if (d->StoredCount > 0 && DefXP(&d->StoredUnits[0]) == -1.0f) {
                    // the crew died: a built-in driver keeps the vehicle
                    SPUnit* p = g_UnitRegistry->GetPUnit(SStr(d->ClassName), false);   // 0x5d0e70(name, 0)
                    if (!p->BuiltInDriver) {                      // +0xc8
                        remove = true;
                    } else {
                        SArmyArray st = { d->StoredUnits, d->StoredCount, d->StoredMax };
                        ArmyRemove(&st, 0);                       // 0x579220(0)
                        d->StoredUnits = st.Array;
                        d->StoredCount = st.Size;
                        d->StoredMax = st.Max;
                    }
                } else if (DefXP(d) == -1.0f) {
                    if (d->StoredCount > 0 && DefXP(&d->StoredUnits[0]) != -1.0f) {
                        SUnitDef crew;                            // the crew walks on
                        UnitDefCopy(&crew, &d->StoredUnits[0]);
                        UnitDefCopy(ArmyAdd(army), &crew);        // 0x560290
                        d = &army->Array[i];
                    }
                    remove = true;
                }
                if (remove) {
                    ArmyRemove(army, i);                          // 0x579220(i)
                    --i;
                }
            }
        }
    }
    for (int i = 0; i < w->Units.Size; ++i) {
        if (!w->Units.IsLive(i))
            continue;
        SUnit* u = w->Units.Array[i].Unit;
        if (!u->_110 && UnitStatsCategory(u) > 0 && u->Player >= 0 && u->Player < 12)   // 0x56d6d0
            ++live[u->Player];
    }
    for (int p = 0; p < 12; ++p) {
        unsigned char* rec = c->PlayerStats[p];                   // +0x140 + p * 0xd8
        *(int*)(rec + 0x08) = *(const int*)(w->Players[p] + 0x08);   // World +0x178 + p * 0x48
        SString* name = (SString*)rec;                            // single player: no name
        FreeSString(name);
        int total = *(const int*)(rec + 0x0c);
        *(int*)(rec + 0xd0) = total < 1 ? -1 : 100 - live[p] * 100 / total;
    }
    c->_b68 = Frame / 20;                                         // 0x5974e0(frames / 20, 0)
    c->_b70 = 0;
    const unsigned char* me = c->PlayerStats[w->LocalPlayer];
    c->Score = *(const int*)(me + 0xcc) + (*(const int*)(me + 0x3c) * 10 - c->_b68) * 10;
    if (c->Score < 0)
        c->Score = 0;
    if (c->GetMissionResult() == 1)                               // 0x5920b0
        c->Score += 5000;
    for (int i = 0; i < c->ObjectiveCount; ++i) {
        const SCampaignObjective& o = c->Objectives[i];
        if (o.Hero || o.State != 2)                               // +0x07 excludes
            continue;
        if (!o.Main)
            c->Score += o.Secret ? 6000 : 8000;
    }
    Logger.g->Log(0, "PZM4: BackupCampaignUnits: army %d records, live %d, time %d s, score %d",
                  army->Size, live[w->LocalPlayer], c->_b68, c->Score);
}

// PANZERS 0x57f970
void SGameLogic::SetVisOverlayMode(int mode)
{
    VisOverlayMode = mode;
    if (g_World && g_World->Terrain)
        g_World->Terrain->SetOverlay((int)(size_t)VisMap[g_World->LocalPlayer], mode);   // terrain +0x1c
    for (int i = 0; i < 12; ++i)
        if (VisMap[i])
            PlayerTable[i] = Frame;                               // rebuild the vision maps from this frame
}

// PANZERS 0x57fac0
void SGameLogic::SetBoardArea(int frame, int w, int h1, int h2)
{
    BoardArea[0] = frame;
    BoardArea[1] = w;
    BoardArea[2] = h1;
    BoardArea[3] = h2;
}

// PANZERS 0x56e8e0 (single-player path)
void SGameLogic::PreloadArmyUnits()
{
    PZ_M3_TRACE("SGameLogic::PreloadArmyUnits (0x56e8e0)");
    SArmyArray army = { nullptr, 0, 0 };
    if (g_Campaign)
        g_Campaign->GetMissionArmy(&army);                        // 0x591e70
    for (int i = 0; i < army.Size; ++i)
        g_UnitRegistry->GetPUnit(SStr(army.Array[i].ClassName), true);   // 0x5d0e70(name, 1)
    ArmyFree(&army);
}

// PANZERS 0x5805c0
void SGameLogic::StartPacketRecording(const char* file)
{
    PZ_M3_TRACE("SGameLogic::StartPacketRecording (0x5805c0)");
    RecordStream = FileSystem.OpenWrite(file, "SGameLogic::StartPacketRecording");   // 0x65f7a0
    RecordStream->WriteByte(3);                                   // 0x65daf0
    g_Campaign->WriteReplayHeader(RecordStream);                  // 0x596fc0
}

// PANZERS 0x580540
void SGameLogic::StartPacketPlayback(const char* file)
{
    PZ_M3_TRACE("SGameLogic::StartPacketPlayback (0x580540)");
    PlaybackStream = FileSystem.OpenRead(file, "SGameLogic::StartPacketPlayback");   // 0x65f420
    unsigned char ver = PlaybackStream->ReadByte();               // 0x65d300
    if (ver == 2) {
        Logger.g->Warning("SGameLogic::StartPacketPlayback: Old Rec version (EXA)");   // 0x65cac0
        return;
    }
    if (ver != 3)
        Logger.g->Panic("SGameLogic::StartPacketPlayback: Unsupported file version");
    g_Campaign->ReadReplayHeader(PlaybackStream);                 // 0x595780
}

void SGameLogic::SaveGameState(SStream* s)
{
    STUB_LOG("SGameLogic::SaveGameState (0x57e110)");
    PZ_M3_TRACE("SGameLogic::SaveGameState (0x57e110)");
    // HD writes 19 chunks (docs/FORMATS.md "Save game"): PLY3 0x5fb160,
    // AIGP 0x57db20, UNIS 0x5fb630 (every unit: 'UNIT' {'v100', class,
    // SUnit::Save 0x5be320}), EEFS 0x5fb030, "CAM " (0x5e6a70, 20 bytes),
    // LOCS 0x5fb140, TRIG 0x5fb430 (when World+0x7478), RTRG 0x57d9c0, TVAR
    // 0x57dd60, ECHO 0x57e4f0, CNTR {+0x14c, +0x14d, +0x174, +0x178}, VARS
    // (gSaveVariables, 0x8dba58), SEED {World+0x7518}, ODDD 0x5fb020, WIR3
    // 0x5fb7d0, MGRP 0x57db90 (twice), AMOD 0x57d8c0, WTHR 0x5fb770, OBJT.
    // The unit / gunner / driver descriptor tables are not lifted, so the
    // recompile writes only the campaign part of the file.
    (void)s;
}

} // namespace pz
