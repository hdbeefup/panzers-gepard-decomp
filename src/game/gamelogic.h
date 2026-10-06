// src/game/gamelogic.h
// pz::SGameLogic: HD SGameLogic (0x318 bytes, no vtable; 0x55e440..0x582a00).
// In HD, SSuperWindow+0x1a0 holds it and DAT_008f2078 (pz::g_GameLogic)
// points to it. Moved here from src/world in M2-P0.
//
// SHARED HEADER (owner P0). Agent L implements the members (gamelogic.cpp,
// triggers.cpp, movementgroup.cpp) and names the fields it decodes, in place,
// at their HD offsets. Never grow the class: PZ_HD_SIZE keeps it at 0x318.
//
// Per-tick order of Refresh 0x576d80 (single-player path, executed in the
// original menu; see docs/re/M2_COVERAGE.md): FPU control word check,
// SWorld::UpdateSpeech 0x607f50, 0x578b00, 0x578a70, then per frame
// ProcessPacket 0x5737c0 / BeginFrame 0x571840 (world CRC 0x56aa10) /
// 0x579390, SWorld refresh 0x604620, 0x57dfe0, SWorld 0x6088f0 (weather),
// World+0x4ec = frame, DispatchEverySecond 0x570cc0 when frame % 20 == 0,
// RunTriggers 0x579ab0, 0x568af0, 0x565e10, per unit +0x16c / +0x2c(frame),
// scene +0xf0(2), SWorld::RefreshFlyingFox 0x5f6bf0, per unit +0x3c,
// 0x5822a0. Not executed in the menu: SMulti sync 0x5212a0, AI 0x5f5c70,
// 0x605930, 0x6090e0, the "%s has left/lost the game" branches.

#ifndef PZ_GAMELOGIC_H
#define PZ_GAMELOGIC_H

#include <stddef.h>
#include "pz/pzcommon.h"
#include "m2common.h"
#include "string2.h"

struct SStream;

namespace pz {

struct SIViewport;
struct SArmyArray;
struct STrigger;
struct SIUnit;
struct SIModel;
struct SRunningTrigger;
struct SFoundUnits;

// Movement group (SGameLogic+0x2dc SHeap element 0x28 = Next + 0x24). Agent L.
struct SMovementGroupMember {
    int   Unit;          // +0x00 world unit index
    float DX;            // +0x04 offset from the group centre at order time
    float DZ;            // +0x08
    float DistSq;        // +0x0c squared distance to the target (0x56ff30 p5)
};
struct SMovementGroupElem {
    int   Next;          // +0x00 kHeapLive = 0x7fffffff
    SMovementGroupMember* Members; // +0x04 SDArray (0x10 each)
    int   MemberCount;   // +0x08
    int   MemberMax;     // +0x0c
    float MoveSpeed;     // +0x10 GetMovementGroupMoveSpeed (slowest member)
    int   SquadsGlobalState; // +0x14 lowest squad +0xec, 0 if not all squads (0x5800d0)
    int   BossUnit;      // +0x18 world unit index (0x56adc0)
    int   BiggestUnit;   // +0x1c world unit index (0x56acf0)
    float FormationDir;  // +0x20 SetMovementGroupFormationDir 0x57ffc0 / GetMovementGroupFormationDir 0x56af90
    bool  B24;           // +0x24 SetMovementGroupFormationDir 0x57ff40
    bool  Convoy;        // +0x25 GetMovementGroupConvoy
    unsigned char _26[2];
};

// Animated one-shot model (SGameLogic+0x2fc, 0x24 bytes; 0x5649e0 / 0x5822a0).
struct SAnimatedModel {
    SString Name;        // +0x00 model file
    float   Pos[3];      // +0x08
    float   Dir;         // +0x14 model +0x1c SetRotation(Dir, 0, 0)
    SIModel* Model;      // +0x18
    float   Time;        // +0x1c += 0.05 per tick
    float   Duration;    // +0x20 the model's animation length
};

struct SGameLogic {
    SGameLogic(int p1, int p2, int p3);   // 0x55e440 (menu: 0, -1, 0); sets g_GameLogic
    ~SGameLogic();                        // 0x55fe00 (non-virtual; caller deletes 0x318)

    void SetRunning(int running);         // 0x5802f0 (+0x04 = running; Concert +0x64 resume)
    int  Refresh();                       // 0x576d80, 20 Hz fixed step (M1 path, or RefreshM2 with -m2)
    void UpdateUnitVisuals(SIViewport* vp, double interpolation);   // 0x5638f0 (per frame)

    // --- M2 (agent L). Steady-loop functions of the original menu.
    void RefreshM2();                     // the 0x576d80 single-player path (recompile split)
    void Tick_578b00();                   // per tick, before the frame loop
    void Tick_578a70();                   // per tick: message lines (+0x5c/+0x78/+0x7c)
    void ProcessPacket(int player, SStream** frame);   // 0x5737c0 per frame and player (packets.cpp, agent O)
    void BeginFrame();                    // 0x571840 new frame stream, world CRC into CrcHistory
    unsigned ComputeWorldCRC();           // 0x56aa10 rotl-xor over the live units (see iunit.h) ^ World+0x7518
    void DumpUnitsForCrc();               // recompile only: PZ_M2_UNITDUMP=<n> per-tick unit trace
    void Tick_579390();                   // 0x579390 CrcHistory.RemoveBottom
    void Tick_57dfe0();                   // per tick
    void Tick_568af0();                   // per tick (scripted sequence, +0x2b8 only)
    void Tick_565e10(int player);         // per tick, player from table 0x7f6220[frame % 12]
    void Tick_5822a0();                   // 0x5822a0 animated models (+0x2fc): advance, drop when done
    void CreateAnimatedModel(const char* file, float x, float y, float z, float dir);   // 0x5649e0
    int  GetFrame();                      // 0x56d1a0 (+0x08)
    bool IsPaused();                      // 0x56e150 (+0x288 || +0x2b8) (name guessed)
    bool CanSeeGroundUnit(int player, SIUnit* unit);   // 0x562760
    bool IsInPlayerVision(int player, SIUnit* unit);   // 0x562650 own unit, or VisMap bit 4 at its cell

    // --- M2-I sub-agent LG: add SGameLogic declarations here only.
    void BuildDoodadGrid();               // 0x564c20 (ctor): +0x1b8 / +0x1bc
    int  DoodadGridHead(float x, float z) const;   // the list head of the cell under (x, z) (fistp CW 0xc7f)
    void BuildVisMaps();                  // 0x564fb0 (ctor): +0x1c8..+0x264 and the shadow-cast tables
    void BuildVisHeights();               // 0x576a70 (from 0x564fb0): +0x1d0 eye heights
    void AddUnitVision(int player, SIUnit* unit);   // 0x565530 marks the unit's sight/hearing in VisMap[player]
    void CastVisOctant(int player, float eye, int cell, int radius, int outer, int inner, unsigned char bits);   // 0x5662b0
    void FillVisOctant(int player, int cell, int radius, int outer, int inner, unsigned char bits);   // 0x567180
    void FreeVisMaps();                   // recompile: the maps owned by this object (dtor)
    // --- end LG
    // --- M3 agent O (packets.cpp): recording / playback and the order handlers.
    void CheckSendQSize();                         // 0x562fb0 (multiplayer send-queue warning only)
    void OrderPlain(SFoundUnits* g, int command, bool p3, bool queue);              // 0x564440 SUnit 0x5bb7b0 per unit
    void OrderAtPoint(SFoundUnits* g, int command, const float* xz, bool p4, bool queue);   // 0x564660 SUnit 0x5bb980 per unit
    void OrderAtUnit(SFoundUnits* g, int command, int unit, bool p4, bool queue);   // 0x564720 SUnit 0x5bb8a0 per unit
    void OrderValue(SFoundUnits* g, int command, int value, bool p4, bool queue);   // 0x564870 SUnit 0x5bbb60 per unit
    void OrderFloat(SFoundUnits* g, int command, float value, bool p4, bool queue); // 0x564910 SUnit 0x5bbc40 per unit
    void MoveFoundUnitsNear(SFoundUnits* g, int command, const float* target, bool p4, bool queue, bool marker);   // 0x57e8b0
    void MoveFoundUnitsToLocationDir(SFoundUnits* g, int command, const float* target, float dir, bool p4, bool queue, bool marker);   // 0x57f200
    void ModifyMovementGroupUnitsFormationPos(int group, SFoundUnits* g, const float* offsets);   // 0x5708f0
    // --- end O
    // --- M2-I sub-agent UB / SQ / BW: SGameLogic functions the units need (one line each, tagged).
    // --- end units
    // --- M3 C (combat + AI): SGameLogic functions the combat code needs. One sub-block per
    //     C sub-agent; add declarations only inside your own.
    // --- C1 (agent O's rows lifted by C1 for the combat path, projectile.cpp)
    void DamageArea(float damage, int attacker, int exclude, float x, float y, float z, float radius,
                    int hitMode, int weaponType);   // 0x576490 (9) doodads crushed in the radius, TakeDamage(damage * (1 - sqrt(d2 / r2))) on the units
    int  ProjectileHitTest(int shooter, struct SUnit* projectile);   // 0x562c20 (2) doodad mesh or unit hit by the projectile (-1)
    // --- end C1
    // --- C2 (combat.cpp)
    static int UnitStatCategory(struct SUnit* u);               // 0x56d6d0 campaign statistics category (0 = none)
    bool IsKillCheat();                                   // 0x5b9e40 +0x2d8 and not paused
    void DispatchAttacked(int unit, int attacker);        // 0x570e40 trigger event 4
    void DispatchUnitDies(int unit);                      // 0x571090 loss statistics + trigger event 1
    void DispatchLeaves(int carrier, int unit);           // 0x571570 trigger event 6 (a unit left a vehicle / building)
    void DispatchStopsTowing(int tower, int towed);       // 0x571750 trigger event 8
    void PingAttackedUnit(int unit);                      // 0x570f30 minimap blink (board +0xb4 not mapped)
    bool IsSeenByPlayer(int player, struct SUnit* u);     // 0x562b10 own unit, or VisMap bit 0x20 at its cell
    void AreaDamage(float damage, int attacker, int p3, float x, float y, float z, float radius, int p8,
                    int p9);                              // 0x576490 (9) explosion: units and doodads in radius (stub, owner C1?)
    // --- end C2
    // --- C3 (the support calls 0x5674c0 / 0x568300 / 0x568740 / 0x567760 / 0x567d40: combat_support.cpp)
    // free = true: no charge (AI / triggers); false: decrements the player's
    // counter World+0x19c.. (+0x2c.. of the player record) and does nothing
    // when it is 0. (x, z) the target; player the caller. The planes enter at
    // the player's entry point (World+0x194 / +0x198) or, with fromDir && p6,
    // on the map border along dir (0x56cff0).
    void SupportArtillery(bool free, float x, float z, int player);   // 0x5674c0 (4) 16 "Projectile cannonade" shells (counter +0x2c)
    void SupportRecon(bool free, float x, float z, int player, float extraAltitude, bool groundTracking);   // 0x568300 (6) recon plane (counter +0x30); extraAltitude -1.0 = none
    void SupportTacBomber(bool free, float x, float z, int player);   // 0x568740 (4) tactical bomber diving from 21 m before the target (counter +0x34)
    void SupportHeavyBomber(bool free, float x, float z, int player, float extraAltitude, bool p6, bool fromDir, float dir);   // 0x567760 (8) SGameLogic::EC_HeavyBomber, 5 bombs (counter +0x38); trigger event 9
    void SupportParatroopers(bool free, float x, float z, int player, float extraAltitude, bool p6, bool fromDir, float dir);  // 0x567d40 (8) transport plane, 2 squads (counter +0x3c); trigger event 10
    float* ComputeBorderStart(float* out, float x, float y, float z, float dir);   // 0x56cff0 (5) the map border point (1 .. size-1) on the line through (x, z) against dir
    void DispatchBomberSent(int unit, int location);       // 0x570ac0 (2) trigger event 9 (TE_BOMBER_SENT)
    void DispatchParatroopersSent(int unit, int location); // 0x570bc0 (2) trigger event 10 (TE_PARATROOPERS_SENT)
    void SupportTestHook();                                   // recompile only: PZ_M3C_TESTSUPPORT=1..5 calls one support call (free) at frame PZ_M3C_TESTFRAME (400) for the local player
    // --- end C3
    // --- C4
    // --- end C4
    // --- end M3 C
    // --- M3 agent F: mission start / end and the save (gamelogic_mission.cpp).
    void PlaceAllUnits();                 // 0x571c70 mission start: the campaign's mission army (0x591e70) for the local player
    void PlaceUnits(int player, struct SArmyArray* army);   // 0x572ec0 at location "start <n>" in a 4 m grid, then the camera
    void BackupCampaignUnits();           // 0x561110 mission end: surviving campaign units back into the army, stats, score
    void SetVisOverlayMode(int mode);     // 0x57f970 +0x264 and the terrain overlay of the local player
    void SetBoardArea(int frame, int w, int h1, int h2);    // 0x57fac0 +0x1a0..+0x1ac (LoadMap: view +0x4b8, 0x13c, 7, 7) (name guessed)
    void PreloadArmyUnits();              // 0x56e8e0 GetPUnit(name, 1) for every mission-army record
    void StartPacketRecording(const char* file);   // 0x5805c0 +0x1b0 = file, byte 3, the replay header
    void StartPacketPlayback(const char* file);    // 0x580540 +0x1b4 = file, version byte 3, ReadReplayHeader
    void SaveGameState(SStream* s);       // 0x57e110 the game part of a save (PLY3 AIGP UNIS EEFS CAM LOCS TRIG RTRG TVAR ECHO CNTR VARS SEED ODDD WIR3 MGRP AMOD WTHR OBJT)
    void CastVisCone(int player, float eye, int cell, int radius, float dir, float width);   // 0x5664f0 occupied-building window sight (logicextra.cpp)
    void CastVisOctantHalf(int player, float eye, int cell, int radius, int outer, int inner, float cx, float cy);   // 0x567280 0x5662b0 limited to ring*cy + step*cx > 0, bits 0xb
    // --- end M3 F

    // Triggers (triggers.cpp).
    void DispatchEverySecond();           // 0x570cc0 event 0 (and Value += Step of every variable)
    void DispatchEnterLocation(int unit, int location);   // 0x571280 event 2
    void DispatchLeaveLocation(int unit, int location);   // 0x571470 event 3
    void Dispatch_571380(int p1, int p2); // 0x571380 (an event dispatcher; crew enters the jeep?)
    void CollectActiveLocations();        // 0x5640b0 locations used by events 2/3 (+0x2f0)
    void UpdateActiveLocations(SIUnit* unit, bool dispatch);   // 0x582080 (called by SUnit::ServerRefresh 0x5bee90)
    void UpdateActiveLocationsAt(int unit, bool dispatch);     // 0x582080 by heap index (recompile)
    void CheckConditions(SRunningTrigger* rt);            // 0x580600 (15 condition types); starts the trigger
    void RemoveRunningTrigger(int index); // 0x579170
    void RunTriggers();                   // 0x579ab0 (76 action cases; menu: 1, 2, 3, 8, 0xa, 0x1e, 0x1f, 0x26, 0x29)

    // Orders and movement groups (movementgroup.cpp).
    int  GroupOrder(bool convoy, int p3, SFoundUnits* group, bool p5, float x, float z);   // 0x56ff30
    void MoveFoundUnitsToLocation(SFoundUnits* group, int command, const float* target, bool p4, bool queue, bool marker);   // 0x57efd0
    void ConvoyAlongPath(int group, int path);            // 0x57e600
    void RemoveUnitFromMovementGroup(int unit);           // 0x579510
    void SendConvoyMovementGroupFollowers(int group);     // 0x57e6f0
    int  GetMovementGroupConvoy(int group);               // 0x56af10
    float GetMovementGroupMoveSpeed(SIUnit* unit);        // 0x56b010
    int  GetMovementGroupBossUnit(int group);             // 0x56adc0 (unit index; HD returns the pointer)
    int  GetMovementGroupBiggestUnit(int group);          // 0x56acf0 (unit index; HD returns the pointer)
    int  GetMovementGroupSquadsGlobalState(int unit);     // 0x56b090 (HD takes the unit pointer)
    void GetMovementGroupUnitFormationPos(float* out, SIUnit* unit);   // 0x56b240
    void SetMovementGroupBiggestUnit(int group);          // 0x57faf0
    void SetMovementGroupBossUnit(int group, int p2, float x, float z);   // 0x57fcb0
    void SetMovementGroupFormationDir(int group, int p2); // 0x57ff40
    void SetMovementGroupFormationDir2(int group, float dir);// 0x57ffc0 (same symbol in HD)
    void RefreshMovementGroup(int p3, int group);         // 0x5800d0
    void UpdateMovementGroupSlowestMoveSpeed(int group);  // 0x5824b0

    // Fields (HD offsets). Decoded so far:
    int           Mode;              // +0x000 ctor p4 (non-zero: multiplayer/replay branches in Refresh)
    int           Running;           // +0x004 SetRunning; logic frames per Refresh call
    int           Frame;             // +0x008 logic tick (GetFrame 0x56d1a0)
    int           _00c;              // +0x00c ctor 0
    int           _010;              // +0x010 ctor 0
    int           _014;              // +0x014 ctor -0x12f
    int           TargetRingFx;      // +0x018 "effects/target_ring.fx" (pixie +0x10)
    int           TankDustFx;        // +0x01c "effects/smoke/tank_goz.fx"
    void*         FrameObject;       // +0x020 per-tick SStreamBuffer (new 0x30) of BeginFrame
    bool          InFrameSync;       // +0x024 MP frame loop flag
    unsigned char _025[3];
    int           FramesSent;        // +0x028 +1 per BeginFrame in Refresh
    unsigned*     CrcHistory;        // +0x02c SDEQueue<unsigned> of world CRCs (0x5610a0 push, 0x579390 pop)
    int           CrcCount;          // +0x030
    int           CrcMax;            // +0x034
    int           CrcBase;           // +0x038
    int           CrcTop;            // +0x03c (ctor -1)
    int           CrcBottom;         // +0x040
    unsigned char _044[0x05c - 0x044];
    int           MessagePlayer;     // +0x05c ctor p2 (menu -1)
    unsigned char _060[0x078 - 0x060];
    int           MessageCount;      // +0x078 (0x578a70)
    int           MessageTimer;      // +0x07c ctor 400
    unsigned char _080[0x17c - 0x080];
    int           MinimapFrame;      // +0x17c ctor p3 (menu -1: no minimap)
    unsigned char _180[0x1a0 - 0x180];
    int           BoardArea[4];      // +0x1a0 0x57fac0 (LoadMap: view +0x4b8, 0x13c, 7, 7)
    SStream*      RecordStream;      // +0x1b0 -packetrec file (0x5805c0)
    SStream*      PlaybackStream;    // +0x1b4 -packetplay file (0x580540); ProcessPacket reads the frames from it
    // The doodad grid (0x564c20): per 8x8-tile cell the head of a list of
    // {doodad, next} links in +0x1bc (-1 ends).
    int*          DoodadGrid;        // +0x1b8 (TerrainW / 8) * (TerrainH / 8) heads
    struct SDoodadLink { int Doodad; int Next; };
    SDoodadLink*  DoodadLinks;       // +0x1bc SDArray
    int           DoodadLinkCount;   // +0x1c0
    int           DoodadLinkMax;     // +0x1c4
    int           VisW;              // +0x1c8 TerrainW * 2 + 2 (half-tile cells per row, 0x564fb0)
    int           VisH;              // +0x1cc TerrainH * 2 + 2
    float*        VisHeights;        // +0x1d0 VisW * VisH eye heights (0x576a70)
    unsigned char* VisMap[12];       // +0x1d4 per player (allies share one); bits 1|2 sight, 4 mines, 8 half sight, 0x10 hearing
    unsigned char* VisMapOwned[12];  // +0x204 the maps a player allocated (0 when shared / none)
    int           PlayerTable[12];   // +0x234 ctor -1; 0x565e10 stores the frame of the last rebuild
    int           VisOverlayMode;    // +0x264 1 (0x564fb0); terrain +0x1c SetOverlay(map, mode)
    SRunningTrigger* RunningTriggers;// +0x268 SDArray<SRunningTrigger> (0x34 each)
    int           RunningTriggerCount; // +0x26c
    int           RunningTriggerMax; // +0x270
    unsigned char _274[0x288 - 0x274];
    bool          Flag288;           // +0x288 (IsPaused)
    unsigned char _289[0x2b8 - 0x289];
    bool          Flag2b8;           // +0x2b8 (IsPaused; scripted sequence running)
    unsigned char _2b9[0x2dc - 0x2b9];
    SMovementGroupElem* MovementGroups; // +0x2dc SHeap<SMovementGroup> (element 0x28)
    int           MovementGroupSize; // +0x2e0
    int           MovementGroupMax;  // +0x2e4
    int           MovementGroupFree; // +0x2e8 ctor -1
    int           MovementGroupCount;// +0x2ec
    int*          ActiveLocations;   // +0x2f0 SDArray<int> (0x5640b0, at most 32)
    int           ActiveLocationCount; // +0x2f4
    int           ActiveLocationMax; // +0x2f8
    SAnimatedModel* AnimatedModels;  // +0x2fc SDArray<SAnimatedModel> (0x24 each, 0x5649e0 / 0x5822a0)
    int           AnimatedModelCount;// +0x300
    int           AnimatedModelMax;  // +0x304
    int           _308;              // +0x308 ctor -1
    unsigned char _30c[0x310 - 0x30c];
    int           ShellFallFx;       // +0x310 "effects/sound/s_shell_fall.fx"
    unsigned char _314[0x318 - 0x314];
};
PZ_HD_SIZE(SGameLogic, kHdSizeSGameLogic);
#if defined(_M_IX86)
static_assert(offsetof(SGameLogic, Frame) == 0x008, "Refresh param_1[2]");
static_assert(offsetof(SGameLogic, FrameObject) == 0x020, "0x571840 +0x20");
static_assert(offsetof(SGameLogic, CrcHistory) == 0x02c, "0x571840 +0x2c");
static_assert(offsetof(SGameLogic, RunningTriggers) == 0x268, "RunTriggers +0x268");
static_assert(offsetof(SGameLogic, Flag2b8) == 0x2b8, "0x56e150 +0x2b8");
static_assert(offsetof(SGameLogic, CrcBottom) == 0x040, "0x5610a0 param_1[5]");
static_assert(offsetof(SGameLogic, MessageTimer) == 0x07c, "0x578a70 +0x7c");
static_assert(offsetof(SGameLogic, PlayerTable) == 0x234, "ctor param_1 + 0x8d");
static_assert(offsetof(SGameLogic, VisW) == 0x1c8, "0x564fb0 +0x1c8");
static_assert(offsetof(SGameLogic, DoodadGrid) == 0x1b8, "0x564c20 +0x1b8");
static_assert(offsetof(SGameLogic, PlaybackStream) == 0x1b4, "0x580540 +0x1b4");
static_assert(offsetof(SGameLogic, VisMap) == 0x1d4, "0x565530 +0x1d4");
static_assert(offsetof(SGameLogic, VisMapOwned) == 0x204, "0x565e10 +0x204");
static_assert(offsetof(SGameLogic, VisOverlayMode) == 0x264, "0x565e10 +0x264");
static_assert(offsetof(SGameLogic, MovementGroups) == 0x2dc, "0x579510 +0x2dc");
static_assert(offsetof(SGameLogic, ActiveLocations) == 0x2f0, "0x582080 +0x2f0");
static_assert(offsetof(SGameLogic, AnimatedModels) == 0x2fc, "0x5822a0 +0x2fc");
static_assert(offsetof(SGameLogic, ShellFallFx) == 0x310, "ctor param_1[0xc4]");
static_assert(sizeof(SMovementGroupElem) == 0x28, "SHeap<SMovementGroup> stride 0x28");
static_assert(sizeof(SAnimatedModel) == 0x24, "0x5822a0 stride 0x24");
#endif

} // namespace pz

#endif // PZ_GAMELOGIC_H
