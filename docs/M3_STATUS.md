# M3 status

## THE tc1 REPLAY MATCHES THE ORIGINAL THROUGH FRAME 654 (M3-I, 2026-10-06)

Our build replays the original's recording `m3ref\tc1.rec` from the recorded start state:

```
PZM3I.exe -nointro -m3 -packetplay Replays\tc1.rec      (PZ_M2_CRC=1, PZ_M3_REPLAY_PAUSED=1)
```

(copy `m3ref\tc1.rec` to `<run>\Replays\tc1.rec`; full recipe in docs/M3_REPLAY.md "Our build").
CRC, seed and unit count equal the original on **every frame 0..654** (it was frame 0 only before
M3-I); the first difference is frame 655, the tick after the recorded minimap move of the tank.

| Frame | Original | Ours |
|---|---|---|
| mission start frame 0 | `737376f6 141334d6 358` | `737376f6 141334d6 358` |
| frame 1 | `17efc580 141334d6 358` | `17efc580 141334d6 358` |
| frame 654 | `d96af00d 82919470 360` | `d96af00d 82919470 360` |
| frame 655 | `ff2a6364 a464bdd9 360` | `f1ea6364 a464bdd9 360` (seed equal) |

The reference to diff against is `m3ref\tc1_crc_bf_mission.txt` (computed at BeginFrame 0x571840,
like our `PZM2 CRC` line). The polled `tc1_crc_ref.txt` differs from it on frames 402 / 449 / 462 /
642 only (selection packets: the poller saw them after ProcessPacket). Tools (scratch `m3i\tools`):
`mdiff.py <log>` (first differing frame), `ucap_cmp.py <log> f0 f1 [--all]` (per-unit fields
against the original's capture), `rawcmp.py <log> <unit> u|drv|gunK` (raw dwords against the
capture; our side `PZ_M2_UNITRAW` / `PZ_M2_DRVDUMP` / the new `PZ_M2_GUNDUMP`).

### The original's per-unit capture (one run, M3-I)

`m3ref\tc1_units_f0-40.txt` / `tc1_units_raw.bin` (README line in m3ref): the original replaying
tc1 (recipe B) under a small debugger (`m3ref\tools\ucapdbg.py`: execute breakpoint on
SGameLogic::BeginFrame 0x571840, so no frame is missed). Frames 0..40: every unit 0x480 bytes, its
model, model2, animation, drivers and gunners, and the SWorld; frames 41..200 and every 10th to
2190: unit raw 0x480. Uninitialised fields read 0xbaadf00d there (the debugger's heap fill).

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

### Frame 655: the next bug

The recorded minimap right click (frame 654, `02` move of the tank 346 to (93.09, 86.00)) gives
the tank a different route: HD's ghost queue (+0x1dc) holds 0x61 frames at frame 660, ours 0x58
(+0x1e8 top 0x2f4 / 0x2eb); positions stay equal to frame 680, then HD drives at heading -2.252,
ours at -2.655. The global path waypoint count (+0x30c) is the same, so the A* result differs in
its points: most likely the block map along the way (the A* itself is emulator-verified), e.g.
dead soldiers or crushed doodads near (130, 146). Not resolved; see "Requests" in the M3-I report.

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
  run from `PzHudUpdate` (minimap.cpp). The image is the map's MINI chunk, re-read from the map
  file because the world loader keeps it raw (World +0x74bc stays null).
  `view +0x828` (SGameLogic minimap frame) is now the minimap frame.
- **Cursor**: HoverCursor 0x621540, the mode-4 target cursor colour, the cursor colour on the
  software cursor when options.ini asks for the hardware cursor (hudcursor.cpp).
- **HQ squad damage**: HD 0x64741a shows the squad member's first weapon (SquadMemberName).
- **MOD_WIDESCREEN**: picks / box select follow the centred game view; HQ preview placed.

For M3-I / C (src/game, not edited): `SGameLogic::PingAttackedUnit` 0x570f30 can call
`PzMinimapPing(unit)` (hud.h); the STUB_LOGs "UpdateUnitVisuals minimap dots" and
"Tick_565e10 minimap fog bitmap" are done by the HUD path (drop or call `PzMinimapUpdate`);
SGameLogic ctor could call `PzMinimapCreate`; iunit +0xa8 is `int ActionOn(int target)`
(hudcursor.cpp reads 0x5ba3e0 / 0x5ace90 / 0x548c10 / 0x59c0d0 for the cursor).
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

- Frames after 654. Other recordings and nations.
