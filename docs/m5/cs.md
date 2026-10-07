# M5 agent CS: campaign cut-scenes

Branch `m5-cs` from master b298750. All addresses are HD `PANZERS.exe`.

## Which missions use which cut-scene

Trigger action 0x38 names a cut-scene `<n>`. HD plays (RunTriggers case 0x38, 0x56ea20):

1. `maps/<n>.map` when that file exists and campaign +0x14 is clear: the **map cut-scene**;
2. else `cutscenes/<n>/<n>.ingame` (SInGameAnimLogic, 0x56f530) when it exists;
3. else `cutscenes/<n>/<n>.4d` (0x56f0d0, a separate scene with the camera of the .4d file).

The names in the maps' trigger data (`scratchpad\m5cs\py\cutrefs.py` over the paks):

| Map | Cut-scenes (kind) |
|---|---|
| ger-01 | ger-01-1 (.ingame, .sub, .mp3), ger-01-2 (.ingame) |
| ger-02 | ger-02-0, ger-02-1, ger-02-2 (.ingame) |
| ger-03 | ger-03-1 (.ingame, .sub), ger-03-2, ger-03-3 (**.4d only**) |
| ger-04 | ger-04-1 (.ingame), ger-04-2 (.4d), **ger-04-3 (map: maps/ger-04-3.map)**, ger-06-3 (.4d) |
| ger-05 .. ger-07 | .ingame (ger-06-3: .4d) |
| ger-08 | ger-08-1..3 (.ingame), **ger-08-4 (map)** |
| ger-09 | ger-09-2, ger-09-3 (.4d) |
| ger-11 | ger-11-1 (.ingame), **ger-11-2 (map)** |
| su-*, us-* | mostly .ingame; su-05-1/-3, su-06-2, us-06-3 .4d |

The three map cut-scenes (ger-04-3, ger-08-4, ger-11-2) are the last part of their mission played
on another map: the new map's own trigger 0x38 with the same name then finds campaign +0x14 set
and plays the .ingame of that name.

## What was lifted

- **Subtitles**: `SSubtitler::ParseSubFile` 0x56fd70 (`{start}{end}text` lines, 1/25 s; Panic on
  a bad line; one board text frame at (0x200, 0x26e) under World +0x74e0) and the line of a time
  0x5826c0 (font 4, centred), called by 0x568af0 with `elapsed * 25 / 20`; removed by 0x565390.
  `src/panzers/cutscene_sub.cpp`.
- **The cut-scene mp3**: 0x56f530 resumes the sounds (Concert +0x64), starts `<n>.mp3` with
  Concert +0x2c(file, 1, 0, 0, 1) into SGameLogic +0x2d4 (looping) and 0x565390 removes it
  (+0x3c). tutorial-01 has no mp3 (the original logs "Can't load" too).
- 0x56f530 also calls scene +0x28 (0x6ac6e0, a renderer stub) and the view's ResetClock
  (callback +0x0c 0x624730).
- **The eye camera** of the campaign cut-scenes (camera type 2; the tutorial's is type 0):
  `SWorld::SetCameraMode` 0x5fd9f0 (0 -> 2: CamDist becomes the field of view, 60 deg in
  22.5 .. 120 deg; 2 -> 0: 24 in 12 .. 30; the free-fall state reset) in `cutscene.cpp`, and
  `SWorld::ComputeCamera` 0x5ddc30 mode 2 in `world.cpp` (eye set directly, focus height from a
  terrain ray along the view, the projection with CamDist as fov every frame). Mode 1 (free fall)
  is still a stub; no campaign data uses it.
- **ExecuteScriptStatement 0x568bc0**, every command of HD's table 0x8db080 except the ones
  M4 had: create / create2 / createwithoutcrew (SWorld::CreateUnit 0x5e3170 with the script id,
  the crew gets "<id>-crew"), follow (order 7), attack (0x11), gunner_aim_ground (0x22),
  gunner_aim_unit (0x23), gunner_lock (0x24), gunner_change_active (0x29), board (0x2a, with
  HD's +0x150 / +0x78 / +0x168 checks), gunner_spin_to_dir (0x25), fx (pixie +0x10 / +0x24),
  change_behavior (0x21), anim (unit +0x1b8), tow (0x2c) / untow (0x2d), heavy_bombardment /
  tactical_bombardment / recon_plane / parachute / cannonade (the support calls, free),
  create_model (0x5649e0), move_to_dir (0x57f200), sethp (0x57fa10: unit +0x130),
  hideallunit / showallunit (0x56d860 / 0x580320 with unit 0x5c5100 / 0x5c5120),
  disableai / enableai (both set unit +0x1d4 in HD, kept), attack_move_on_path /
  move_units_on_path (0x564510 with 10 / 6), move_units_on_path_in_convoy (0x56ff30 + 0x57e600),
  setweather (0x5e6520 + 0x5fdc80), disable / enable_ambient_sounds (0x5f5140: the two flags
  only; 0x5f5b50). The preprocess pass (second argument 1) loads the unit classes of create /
  create2 (0x5d0e70) and the create_model models (Gepard +0x20 / +0x24).
- **RunTriggers case 0x38** returns from RunTriggers after the action (0x57d1b2), as HD does
  (the recompile went on with the next running triggers in the same tick).
- **The map cut-scene** (case 0x38, 0x57d42a..0x57d5b8, `PzMapCutscene` in `triggers.cpp`):
  messages, objective markers (0x579420), animated models (0x563230), doodad grid, vision maps
  (0x579a50), active locations, running triggers (0x563410) go; the view loads the map in place
  (**SIGameViewCallback +0x10 0x624770**, `src/panzers/gameview_mapcut.cpp`: BackupCampaignUnits,
  loading backdrop, the old world deleted, new SWorld + LoadMap + the load extras, cursor, water,
  bridges, music stop, window scene, subports, clock, PlaceAllUnits, StartEffects, running,
  fog-of-war view); then the logic's map state is built again as in the ctor (0x564fb0,
  0x565e10 x 12, 0x5640b0, 0x582080, 0x564c20, unit +0x34, 0x5497a0) and campaign +0x14 = 1.
  HD keeps the old minimap image (it belongs to the logic ctor): so does the recompile.
  SGameLogic::Refresh re-reads the world pointer after RunTriggers (the old one is deleted).

## Not done

- **The .4d cut-scenes** (0x56f0d0, `SGameLogic::UpdateAnimation` 0x582380, 0x5652d0): their own
  scene with the .4d file's camera node; they need model +0x108 and +0xe4 (`SModel::Slot_E4`
  0x6d62e0, a stub) from the renderer. Still logged and skipped (ger-03-2, ger-03-3, ger-04-2,
  ger-06-3, ger-09-2/3, su-05-1/3, su-06-2, us-06-3). The UpdateAnimation STUB_LOG in
  gameview_view.cpp belongs to this path (SGameLogic +0x288) and stays.
- SInGameAnimLogic::CacheLoad 0x588820 / QuickLoad 0x5889ea are the editor's memory caches of
  the animation project (SAVE / inga chunks), not a save of the mission around a cut-scene: no
  in-game caller needs them. The camera accessors GetCameraType / SetCameraType /
  Get/SetCameraNearPlane (0x5893d0, 0x58c3c0, 0x589370, 0x58c360, ...) are editor accessors; the
  game reads the same fields in 0x589970 (`ApplyCamera`).
- The letterbox mode by difficulty (0x64e240, campaign 0x591fb0: Gepard +0x10(2)) in 0x56f530 /
  0x565390; the recompile's letterbox is SGameView::Update's panel mode 2.
- disable_ambient_sounds: the ambient sound sources' sounds (+0x658) and the +0x73e8 sources.

## Verified (our build, `scratchpad\m5cs\shots\`, logs in `scratchpad\m5cs\logs\`)

- **German 1** (New Game -> German -> Normal -> briefing -> mission): ger-01-1 plays at frame 0
  for 1550 frames: the eye cameras (airfield, the column, Hans in the SdKfz 223, `g1c_02`,
  `g1c_08`), the subtitles "1st September, 1939" / "A few hours later..." / "Lt. Hans von
  Gröbel - Tank Commandant" at their times (`g1_01`, `g1_08`, `g1_11`), the mp3, the scripted
  units created / boarded / removed; then the mission view with the HUD (`g1c_12`). Exit clean.
- **The map cut-scene** (German 1 with `PZ_M5_CS_FORCE=1700:ger-04-3`): BackupCampaignUnits, the
  backdrop, `maps/ger-04-3.map` loaded in place, the new map's trigger plays ger-04-3.ingame at
  frame 1720 (the column over the pontoon bridge, the trucks towing the LeFH 18, `m1_02`), it ends
  at frame 2021 and the mission goes on on the new map (`m1_10`). Exit clean.
- **German 4** directly (`PZ_M5_CS_SECTION="German 4"`, a Panzer I bought in the HQ): ger-04-1
  plays (1115 frames). Deleting the ger-04 world (the map cut-scene's 0x624770, and also a plain
  exit of the mission) ends in a pure virtual call in `SUnit::Uninit` (unitbase.cpp, Model
  ->SetVisible) of "ge sig33i artillery" (player 4) from `SWorld::~SWorld`: a world-teardown
  problem of the units (towing: SUnit::Remove stores the tower's crew into the gun while
  g_GameLogic is 0), not of the cut-scene code; ger-01's world deletes cleanly.

## Test hooks (recompile only, inert when unset)

- `PZ_M5_CS_SECTION=<mission section>`: InitCampaignMode starts at that section ("German 4").
- `PZ_M5_CS_FORCE=<frame>:<name>`: at that logic frame of a mission, what action 0x38 `<name>`
  does (once per process).

## Regression (scratchpad\coord\regress.ps1, exe of commit 02246b5)

- Menu CRC: 1181 frames, 0 mismatches (5 of 5 runs).
- Tutorial replay: frames 0..2011 all equal (3 runs passed; 2 runs stopped by the D3D failure below,
  equal up to their last frame). The tutorial's cut-scene tutorial-01 is in frames 0..221.
- tc1 replay: frames 0..270 equal, then every run (5 of 5) stopped at frame 270: a second
  `SDXWindow::OnSize(1024, 768)` right after mission start, `ResetDevice: Reset failed
  (hr=0x8876086C)` in a loop, then a Panic in SMesh::CreateVertexBuffer. The same happens in the
  other M5 agents' tc1 runs at the same times (m5sk 16:49 / 17:06 / 17:29 / 17:51, m5sv 16:29 /
  17:17, m5sc 16:48 / 17:11 / 17:25, m5vx 15:53..16:06), while their earlier runs passed: an
  environment problem of the shared desktop (D3D device reset), not of this branch. tc1 has no
  cut-scene and no trigger action 0x38; it needs a rerun on a quiet desktop.
