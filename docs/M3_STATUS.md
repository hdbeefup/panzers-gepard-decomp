# M3 status

## THE WHOLE tc1 REPLAY MATCHES THE ORIGINAL: FRAMES 0..3900 (M3-I2, 2026-10-06)

Our build replays the original's recording `m3ref\tc1.rec` from the recorded start state:

```
PZM3I2.exe -nointro -m3 -packetplay Replays\tc1.rec      (PZ_M2_CRC=1, PZ_M3_REPLAY_PAUSED=1)
```

(copy `m3ref\tc1.rec` to `<run>\Replays\tc1.rec`; full recipe in docs/M3_REPLAY.md "Our build").
CRC, seed and unit count equal the original on **every frame 0..3900**: the whole recording, with the
paratrooper call (frame 1391, op 0x20), the artillery call (frame 2719, op 0x1c) and the combat to
the end (358 -> 344 live units). M3-I reached 654, M3-I2 started there. 0 "Inconsistency" lines.

| Frame | Original | Ours |
|---|---|---|
| mission start frame 0 | `737376f6 141334d6 358` | `737376f6 141334d6 358` |
| frame 655 (M3-I's first difference) | `ff2a6364 a464bdd9 360` | `ff2a6364 a464bdd9 360` |
| frame 2198 (end of the old reference) | `436ff68a d77f4ffe 355` | `436ff68a d77f4ffe 355` |
| frame 3900 | `735a3f42 88b7d7c8 344` | `735a3f42 88b7d7c8 344` |

References (scratch `m3ref\`): `tc1_crc_bf_full.txt` (BeginFrame CRCs 0..3900, from the deep capture
below; equal to the recording's polled `tc1_crc_ref.txt` except the packet frames 402 / 449 / 462 /
642 / 1391 / 2719, where the poller saw the frame after ProcessPacket) and the older
`tc1_crc_bf_mission.txt` (0..2198). Compare with `m3i2\tools\mdiff.py <log> <ref>`. Our replay keeps
reading through the 61 paused records after 2198 (`PZ_M3_REPLAY_PAUSED=1`); the original needs a
Space press per paused record (the deep capture did that).

### The original's deep capture (one run, M3-I2)

`m3ref\tc1_deep.bin` / `.crc.txt` / `.rng.txt` (README line in m3ref; tool `m3ref\tools\ucapdbg2.py`,
loader `capdump2.py`): the original replaying tc1 (recipe B) under the debugger, **every mission frame
0..3900**: SWorld, SGameLogic, every unit with model / anim, drivers with their local and global
waypoints, manoeuvres and targets, gunners, the unit arrays (+0x16c .. +0x308), ghost queues, targets,
movement groups, running triggers, AI groups, trigger variables; the whole static and dynamic block
maps every frame 600..1100 and every 5th frame otherwise; doodads, doodad grid and vis maps every
25th frame. The RNG log works now (26,378 world-seed writes with eip, registers mapped to units /
drivers / gunners, and the return addresses): ucapdbg.py's handler wrote a stale DR7 back.

Our side: `PZ_M3_DEEPCAP=<file>,<f0>,<f1>` (src/game/m3deepcap.cpp, recompile only) dumps the same
blobs; `m3i2\tools\dc.py` loads both, `ucmp2.py` (per unit, `--raw`, `--key d|g|gh|dl|dg|ua|t`),
`wcmp.py` (SWorld / SGameLogic dwords), `bmsum.py` (block-map differences by bit), `ushow.py`,
`udw.py`. HD-only tools: `hdis.py` (capstone disassembly of the HD exe), `hdword.py` (jump tables).

### Fixes, in the order the frames moved (HD addresses)

| Match to | Root cause | Fix |
|---|---|---|
| 1 | The 88mm flak (unit 84) on the bridge deck: FixBridges 0x5e65f0 skipped the height patches | SIModel +0xb0 `GetHeightPatch` 0x6d6700, applied as HD |
| 140 | An occupied tower (unit 16) saw 35 m (the prototype sight) instead of its squad's; player 9's AA truck (340) saw and attacked the riflemen at frame 0. Also CanAttack 0x583af0 had the bullet / armour test inverted (the AA trucks aimed at the tank) | `SBuildingUnit::GetSightRange` 0x548b40 / `GetMinRange` 0x5482e0; CanAttack fixed |
| 140 | (same commit) buildings never blocked vision or the line of fire | SIModel +0xd0 0x6db7c0, +0xd8 0x6db550, +0x100 0x6d7150, the BSP / convex-poly / AABB / sphere tests (0x6d4130, 0x6d3c00, 0x6d3e40, 0x6d41d0, 0x6d40e0, 0x6d3c90), eye heights 0x546f70 / 0x57f6a0, BuildingBetween 0x5630c0 |
| 265 | Trigger T0's attack-move (command 9) hit the "untyped EC_ slot" stub | ExecuteCommand 0x5b95a0 dispatches every typed slot |
| 504 | A medic squad never healed (stub) | `SUnit::ServerRefreshMedic` 0x5bfa50 |
| 654 | The riflemen's empty squad stayed in the world | `SPanzersSquadUnit::RefreshDead` 0x59dfe0 |
| (654) | Building vision cells (+0x458) empty: node points stub, ctor skipped 0x5497a0; doodad grid missing | SIModel +0xac 0x6d8090, ctor loop as 0x55e440, doodad grid 0x564c20 with ProjectileHitTest 0x562c20 / DamageArea 0x576490 (CrushDoodad) |
| 1390 (M3-I2) | The tank's frame-654 route: the static block map differed from frame 0 on 5,870 cells. The map-load rebuild 0x604620 re-marks buildings through unit +0x1a0 with mask & 0x44; `SBuildingUnit::MarkBlockMap` was empty, so 2,925 building cells lost bit 4 and the 0x40 pass never cleared road bit 0x2000 around buildings (2,945 cells); the A* (road cells are cheap) took another way | `SBuildingUnit::MarkBlockMap` 0x549b20 ("Block" footprint, mask p5 \| 4); the block map now equals HD's at frame 0 |
| 2003 (M3-I2) | The paratrooper transport (op 0x20, 0x567d40) started at (0, 0): World player +0x24 / +0x28 (air start) were never set | the SGameLogic ctor's inline part 0x55f2c2..0x55f3c6: the map-edge point towards the centre of the "start <n>" location, (1, 1) without it |
| 3900 (M3-I2) | A paratrooper's grenade (member 412, gunner 2) left from the world origin: `SWalkerAnimation::InitModel` 0x5ca110 replaces gun 1's **muzzle** list with "R Arm03" (+0x24 / +0x2c), the recompile replaced the recoil list | Guns[1].M / MCount (HD's own quirk, every gun's recoil nodes stored into Guns[0].S at 0x5ca43d, is not kept: walkers never read S) |

### Lifted on the way (not reached by tc1; read against the disassembly, not run)

- `SBuildingUnit::OnMemberDied` 0x549b70 (an occupant died: member and window lists, the second
  squad moves up, the building re-takes gunner / player / behaviour or turns neutral, +0x420 list).
- iunit +0xa8 typed `int ActionOn(int target)`: 0x5ba3e0 / 0x5ace90 / 0x548c10 / 0x59c0d0 are the
  vtable slots now (M3-P's cursor bodies moved into the classes).
- The untyped ExecuteCommand slots: +0xb0 `EC_MoveReverse` 0x5b8d20 (squads 0x59aea0), +0xbc
  `EC_TurnTo` 0x5b9370, +0xdc `EC_AttackMoveUnit` 0x5b8c90, +0xd8 `EC_Tow` 0x5b8fa0, +0x128
  `EC_Destroy` 0x5b88d0 (squads 0x59a730), +0x168 0x5b9360 (empty), `SetItemAutoUse` 0x5b88f0
  (commands 3, 4, 5, 0xb, 0x1c, 0x1d, 0x27, 0x2c, 0x2f; names guessed).
- The MINI chunk: World +0x74bc is the map's SBitmap (0x669ca0 / 0x66ea40 / InitPixelFormat
  0x66e990, freed 0x669cc0 at map load and in ~SWorld); the minimap no longer re-reads the map file.
- World +0x73ac (speech level, 0x64e2b0) was already set by gameview_mission.cpp; nothing to do.

### Left

- `SWorld 0x5d68e0` (an AI group under attack: support calls, help from other groups, "Last man
  standing", one world draw; 7.5 KB) is still a stub. tc1 never calls it (no AI-group unit is
  attacked); it will matter in the campaign missions.
- World fields that differ from HD and are not in the CRC: +0x726c..+0x7450 (speech queue,
  +0x73a0 last event: HD's speech timing is wall-clock), +0x74e8 (new 0xe0 at Initialize, not
  lifted), player +0x40 (a millisecond clock written by Refresh 0x57727d), new units' ScriptID
  string (+0x194: HD null, ours an empty SString).

## What it took (F)

- `SPanzersCampaign::StartReplay` 0x597510 (dead action 0x494c2, wired as a recompile-only test
  switch: `-m3` + `-packetplay Replays\<name>`), `ReadReplayHeader` 0x595780, `WriteReplayHeader`
  0x596fc0, `SGameLogic::StartPacketPlayback` 0x580540 / `StartPacketRecording` 0x5805c0.
  HD's StartReplay does not skip the header's version dword and misreads every v3 file; the
  recompile skips it (`PZ_M3_REPLAY_HD=1` restores HD's read).
- `SGameLogic::PlaceAllUnits` 0x571c70 / `PlaceUnits` 0x572ec0 ("start <n>" location, 4 m grid,
  facing the map centre, FindEmptySpace, camera).
- `SGameView::LoadMap` 0x6201c0 / `MissionStart` 0x6281a0 in HD order (gameview_mission.cpp),
  with the mission SGameLogic ctor arguments and the single Refresh that makes mission frame 0.
- The 15 missing map-load RNG draws: squads stored in buildings (bunkers, towers) at map load.
  `SBuildingUnit::StoreUnit` 0x54d380 + the window points of `SBuildingUnit::Init` (0x548660 /
  0x548560) + the building part of `CanStore` 0x5b7040: each member stands at its window in the
  "stand" state, one world draw each (buildingunit_store.cpp; agent C's area, see the report).

## Frame 1 (history)

Frame 1 differed because of unit 84's bridge height (above). The other suspects listed here before
(SBuildingUnit::RefreshMisc 0x54b910, SSingleUnit::RefreshMisc 0x5af890, FindTarget 0x5b4720) were
not involved; they match the original's capture through frame 654.

## The flow

- Training Camp dialog in the HD layout: German (302,330), Russian (302,365), Allied (302,400),
  Start (413,495), Cancel (609,495), as the m3ref coordinates. Russian is checked at open (HD).
- Two Training Camp round trips without a crash; the default boot menu CRC: 1180 frames, 0
  mismatches.
- Mission end (M3-P): `PzGameViewEndCheck` calls `PzShowMissionEnd` (results.cpp, the UI tail of
  0x628430): Training Camp / tutorial get HD's OK box ("Victory" / "Failure", result 3 = the
  "Inconsistency" error box) with the end music; other modes the results menu SStatisticMenu
  (0x62cd50, Create 0x62ffa0). OK sends 0x47562 through 0x6216b0. Test switch `PZ_M3_FORCE_END`
  (1 victory, 2 defeat, 3 error, 11/12 results menu; off by default).

## Mission polish (M3-P, 2026-10-06)

- **Shadows=2 white terrain** (missions, HQ preview): the recompile's render-pass cache reset
  only assumed device states; HD 0x688230 writes them with the cache. After the terrain's own
  draw state (lighting and blending off) the lit multiply pass 0x6f46e0 ran unlit and unblended
  (white). The menu escaped because its road / decal draws turned them back on. Fixed in
  `InvalidateRenderPassCache` (mesh.cpp); menu unchanged, menu CRC 0 mismatches.
- **Compact terrain / roads**: SParcel2 ctor 0x704fd0, dtor 0x708830, DrawCompact 0x7089e0,
  vertex colours 0x70b240, 0x6f9330; STerrain::Render's compact branch as 0x6f2aa0.
- **Minimap**: board frame type 6 render (0x6c7150 case 6), board +0x4c/+0x54/+0xa4..+0xc0,
  SGepard::UpdateTexture 0x681540; the minimap parts of SGameLogic 0x55e440 / 0x5638f0 / 0x565f1d
  run from `PzHudUpdate` (minimap.cpp). The image is the map's MINI chunk, World +0x74bc (decoded
  by the world loader since M3-I2).
  `view +0x828` (SGameLogic minimap frame) is now the minimap frame.
- **Cursor**: HoverCursor 0x621540, the mode-4 target cursor colour, the cursor colour on the
  software cursor when options.ini asks for the hardware cursor (hudcursor.cpp).
- **HQ squad damage**: HD 0x64741a shows the squad member's first weapon (SquadMemberName).
- **MOD_WIDESCREEN**: picks / box select follow the centred game view; HQ preview placed.

For M3-I / C (src/game, not edited): `SGameLogic::PingAttackedUnit` 0x570f30 can call
`PzMinimapPing(unit)` (hud.h); the STUB_LOGs "UpdateUnitVisuals minimap dots" and
"Tick_565e10 minimap fog bitmap" are done by the HUD path (drop or call `PzMinimapUpdate`);
SGameLogic ctor could call `PzMinimapCreate`; iunit +0xa8 is `int ActionOn(int target)`
(typed by M3-I2; hudcursor.cpp dispatches through the slot).
- Autosave `SaveGames/TRNG-Start.save`: the campaign part equals the original's byte for byte;
  the game state is not written (docs/FORMATS.md).

## Shutdown panic (M3-I)

"SWidget::~SWidget: Children widgets should be removed first" at exit: reproduced only by closing
the window while the Training Camp dialog is open. HD's SSuperWindow::OnDestroy 0x65ab50 deletes
+0x104..+0x11c except +0x118 (the dialog) and +0x100 (credits), so the original has the same bug
(decided from the code, as for the credits). Fixed only under `PANZERS_MOD_BUGFIXES` (exit code 0
there); the faithful build keeps it. Training Camp mission -> End Mission -> menu -> close, and
replay -> End Mission -> Exit, exit cleanly (code 0) in the faithful build.

## Not verified

- Other recordings, nations and maps (tc1 is German, Training Camp, one army).
