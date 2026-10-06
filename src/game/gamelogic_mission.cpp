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

void SGameLogic::BackupCampaignUnits()
{
    STUB_LOG("SGameLogic::BackupCampaignUnits (0x561110)");
    PZ_M3_TRACE("SGameLogic::BackupCampaignUnits (0x561110)");
    // HD (mission end): marks the army records whose units survived (by
    // ScriptID, unit +0x194/+0x198), counts the live units per player into
    // the campaign records (+0x148, +0x210), and computes the score +0xb60
    // from the objectives. Training ends in the main menu and never reads it.
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
