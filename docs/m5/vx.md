# M5-VX: visible renderer gaps in missions

Branch `m5-vx` (agent VX). Census: start `2432 lifted + 1316 SWINE-shared + 174 stubs`
(76 lifted with a STUB_LOG); end `2436 + 1316 + 169` (75). SWINE-shared count unchanged.

## How it was compared

Our build was run on the Training Camp replay (`-nointro -m3 -packetplay Replays\tc1.rec`) and
the Tutorial replay (`... Replays\tut1.rec`) with `PZ_M2_CRC=1 PZ_M3_REPLAY_PAUSED=1`, and shots
were taken by PrintWindow when the log reached a given logic frame (`scratchpad\m5vx\shot.ps1`).
The HUD clock gives the frame of the original's shots (frame = seconds x 20):
`m3ref\shots\pb_g1.png` (original replaying tc1, 0:28), `rec_g2.png` (recording, 0:31),
`m4ref\o_g1.png` (tutorial 0:21), `o_sel.png` (0:39), `o_g2.png` (0:48).

## Differences found, with the HD functions behind them

| # | Difference (original vs ours) | HD functions | Status |
|---|---|---|---|
| 1 | No insignia (iron cross) over squads, no health bars under the members of a selected squad, no bars / rank stars / group number / crew and weapon icons / heat column over vehicles, no fog markers | SSingleUnit::UpdateVisuals 0x5aaaa0, Uninit 0x5abde0, board part of Init 0x5ad150 / 0x5ad9f0; SPanzersSquadUnit 0x599620 / 0x599c70 / Init 0x59c470; member 0x597a10 (ctor 0x5977e0, dtor 0x597940); SBuildingUnit 0x546d20 (Init 0x548f20); SWasterUnit 0x5d1e30 (0x5d22b0, 0x5d2070, 0x5d23d0); the board part of SWorld 0x5d2f90 (selection_hq / insignils icon sets, drag-box frames) | **lifted** (src/game/unitboard.cpp) |
| 2 | No selection / armour-damage decals under a selected vehicle (kijelolo keret textures) | 0x5aaaa0 tail (terrain +0x60 / +0x64) | **lifted** |
| 3 | No red weapon-range ring of the selected unit (rec_g3, rec_g4) | SWorld::ShowUnitRange 0x5fee00 was lifted, but the range_a texture was never loaded | **fixed** (world ctor loads it) |
| 4 | No grass on the mission maps (o_g1: grass tufts right and left of the road) | STerrain::LoadFloraLayer 0x6f52b0 lists `<dir>*.4d`; ours guessed `grass%02d.4d`, which only the menu's flora folder has (others: scrogs01.4d, sas.4d) | **lifted** |
| 5 | Grass, once drawn, was much darker than HD's | flora part of 0x6f33c0: sun light at 0.4, ambient + sun * (max(0,-sun.y) - 0.127324), 0x6ba8f0; alpha fade 50..60 m | **lifted** |
| 6 | Destroyed vehicles kept their intact model | SScene::ReplaceModel 0x6ba810, SModel::SetPrototype 0x6d9c20 | **lifted** |
| 7 | No heat glow / damage tint on vehicles | model +0xec (typed SetColor 0x6da310), +0xf0 from 0x5aaaa0 | **lifted** |
| 8 | Tree shadows at the left / bottom-left of the tc1 view that the original does not show (every tc1 shot) | STerrain::Cull 0x6f1180 takes the half extents from the projection (m00, m11); ours used the horizontal Camera.Fov as the vertical one, so the parcel frustum was 4/3 too wide and 16/9 too tall: trees under the HUD were updated, drawn and put into the shadow buffer | **fixed** (pzterrain.cpp; tc1 0:28 now equals pb_g1) |
| 9 | Grass placement differs in detail | the jitter table of STerrain (CRT rand() at terrain creation: HD's CRT rand state differs, see M2 "CRT rand()") | open (as in the menu) |
| 10 | Tutorial 0:21: the green objective marker at the guard post is not yet shown (it is at 0:39) | probably the replay: the paused records replay in a moment, the original sat in the Esc menu; effect time is wall-clock | not a renderer gap (unverified) |
| 11 | The unit panel at the bottom stays empty when a replay selects units | HUD selection panel (the original's shots with the panel are from the recording run, not a replay) | not comparable |

Checked and equal: track marks of the tank (pb_g1 / ours 0:28), the flame-thrower effect, the medic
red cross, the menu wreck smoke, soldier shadows, the squad member bars (o_sel / o_g2 positions
match pixel for pixel).

## Shots (scratch, not committed)

`scratchpad\m5vx\shots\`:

- before: `base_0560.png`, `base_0620.png` (tc1 0:28 / 0:31, build of master), `t1_0420.png`
  (tutorial 0:21: board elements already in, no grass), `t3_0420.png` (grass found, dark).
- after: `v1_0560.png`, `v1_0620.png` (tc1: insignia over the squads as in pb_g1 / rec_g2),
  `t1_0790.png`, `t1_0960.png` (tutorial: member health bars as in o_sel / o_g2),
  `t4_0420.png` (tutorial 0:21 with HD-lit grass, compare o_g1), `menu4_0400.png` (menu),
  `v4_0650.png` (tc1 0:32: the replay selects the Panzer III: health bar, selection decal and
  heat column; no original shot of this exists), `v4_0470.png` (0:23: bars of the targeted
  enemy flame-thrower squad).
- frustum fix: `v5_0560.png` (tc1 0:28, no stray tree shadows; compare `pb_g1.png` and
  `base_0560.png`), `t5_0420.png`, `menu5_0400.png`.
- crops: `scratchpad\m5vx\cmp_soldiers.png`, `cmp_wreck.png`.

## Notes on the lifts

- Nothing in unitboard.cpp writes a logic field or draws a random number; regress.ps1 stays exact.
  The board frames are created in the unit Init / ctor as in HD (CreateFrame does not touch the
  world). Text frames (unit / squad names, board +0x34) are created but left empty: HD shows them
  nowhere in these bodies.
- HD bug kept: 0x5aaaa0 destroys the five decals every frame without resetting the slots, so a
  deselected vehicle keeps destroying stale decal ids. `PANZERS_MOD_BUGFIXES` resets them.
- HD's Slot_14 of the waster creates its two frames again without releasing the old pair (kept).
- DAT_008f1a74 (SMulti) does not exist in the recompile; the multiplayer insignia set is loaded
  but only the single-player branch runs.
- The heat glow reads the wall-clock timer (0x661800) and HdSin 0x78d640, as HD.

## Files outside my area (minimal edits)

- src/game/singleunit.*, squadunit.*, squadrefresh.cpp, buildingunit.*, waster.*, unitai.cpp:
  the moved UpdateVisuals / Uninit bodies (now in unitboard.cpp) and one CreateBoardElements call
  each; squad ctor sets its board slots to -1 (0 would be the board root).
- src/world/world.cpp / world.h: calls WorldCreateBoardElements / WorldReleaseBoardElements,
  loads range_a.
- src/main/panzers_main.cpp: `PZ_M5_NOGHOST=1` (recompile-only test hook, inert when unset):
  `DisableProcessWindowsGhosting`. On this machine, with three other game windows running, the
  Training Camp map load takes over 5 s; Windows ghosts the window and resizes it when it
  answers again, the OnSize device Reset fails (0x8876086C) and the replay dies with a
  CreateVertexBuffer panic at frame ~270 (also on master). With the hook the runs are clean.
  The regression runs below were made with it set.

## Regression

`regress.ps1` (with PZ_M5_NOGHOST=1): menu 1181 / 0 mismatches, tc1 3901 / 0 differing,
tutorial 2012 / 0 differing, all three `exit: closed`, no panic (after each of the three code commits).

## For the coordinator (references from the original that would help)

- A Training Camp shot with the Panzer III selected and damaged (armour decals, heat column,
  group number) to check item 1/2 for vehicles; the replays select no vehicle in view.
