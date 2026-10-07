# M4 status

## Small fixes (agent FX, branch `m4-fixes`, 2026-10-07)

- **The Tutorial's minimap was black**: `maps/tutorial.map` has no MINI chunk (training.map has
  one, 124 x 116 format 2). For such maps the SGameLogic ctor 0x55e440 renders the image: scene
  +0x0c **0x6b0000** makes a lockable A8R8G8B8 offscreen viewport (0x678c70(w, h, 0x15, 1)) of
  2 * round(86 * w / |(w, h)|) x 2 * round(86 * h / |(w, h)|) (playable tiles; 82 x 150 for the
  tutorial), an orthographic projection (SViewport +0x30 **0x68ce50**, 2 / (W - 0x60), near 1, far
  600) from (W / 2, 30, H / 2) looking down, renders the scene with option 0x11 off and the fog start
  +1000 (**SScene::RenderScene 0x6b7760**), reads it back (SSurfaceBitmap **0x6c0e50**, 0x669be0
  format 2, 0x6c1030) with alpha 0xff (0x66f0b0); then 0x55e440 paints every quarter-tile cell
  without static block bit 0x2000 (0x5da050) white. The recompile's viewport select / unselect
  (0x6803e0 / 0x6814b0) now nest, as HD's viewport stack does: RenderScene selects the shadow
  buffer inside the minimap viewport. The image (`PZ_MINIMAP_DUMP=<file>` writes it raw) shows the
  road, the fields and the woods as in the original's `o_g1.png`.
- **The window did not close after a `-packetplay` replay ended**: the end of the recording sets
  the mission result (0x57391d), and in a tutorial-mode mission (Training Camp, Tutorial) the view
  shows the modal "Victory" end box (0x628430, SMessageBox modal: SetModalWidget 0x544fe0). WM_CLOSE
  then went to the SWINE-shared `SWindow::OnClose` (SWINE 0x497200), which only sets `shouldClose`
  while a modal widget is up; the Panzers main loop (0x544db0) never reads it, so the game ran on.
  HD's `SWindow::OnClose` **0x544c40** is a plain `DestroyWindow(hWnd)` (its only DestroyWindow
  call); the recompile now does the same. Not a replay problem: closing the window under any modal
  box behaved the same.
- **Objective completed (action 0x23)**: the objective music is played (0x57ba97: Concert +0x80(0),
  +0x6c, +0x70("music/Objective.mp3"), +0x78(0), the sequence of the end music), and the markers are
  removed with HD's argument order: 0x57baf8 calls **0x579420**(objective n - 1, target -1); the
  recompile passed (-1, n - 1), i.e. removed the markers whose *target* index was n - 1. 0x579420
  is lifted on the +0x194 records (match objective +0x0c and target +0x10, -1 = any; 0x579070
  SDArray::Remove); the board frames HD drops there (board +0x0c / +0xbc) are not made by the
  recompile. The markers are added by action 0x21 (0x57b746: "new objective" message, objective
  +0x04 cleared, 0x560eb0 per target of the objective), which is **not lifted** (no 0x21 / 0x22 in
  the tutorial).
- **Tutorial objectives, checked against the trigger data** (TRIG of maps/tutorial.map, 126
  triggers): action 0x23 occurs once, in T122 ("26. Cél teljesítve": after variable 1 = 25 and
  variable 2 = 1) for objective 1, the only objective of `[Tutorial]` (`Objective 1 Main = 1`). A
  main objective: no prestige, the music, state 2, no markers to remove (no 0x21), and every main
  objective done -> victory. The tutorial's steps are trigger variable 1 (actions 0x03) and the
  echo / speech texts, not campaign objectives. Step 23 (T102) waits on **0x40000008 > 97** (the
  refilled LeFH 18): it could never pass with the stub (value 0); see below.
- **Campaign objective bonus prestige**: already added, by action 0x23. HD's only writer of
  campaign +0x38 through AddPrestige **0x592b10** is RunTriggers at 0x57ba92 (`push 0x28` secret /
  `push 0x3c` optional, main objectives none), which T's 0x23 does (+0x38 += 0x28 / 0x3c). The
  statistics screen recomputes the same numbers for display only; LetResultsDone carries Prestige
  into the next mission's StartPrestige. Nothing else to add.
- **Condition variable 0x40000008** (CheckConditions 0x580600, 0x580a5e): the mean ammunition of
  gunner 0 of the found units that have a gunner whose prototype has ammunition (GetPGunner +0x48
  != 0): AmmoLeft (+0x20) plus 1 / Ammo when a round is loaded (+0x5c), * 100 (0x7ee558), rounded
  by fistp; 0 when no unit counts. Lifted (was a STUB_LOG).

Regressions (final build): menu CRC 0 of 1,180 frames differ; tc1 replay frames 0..3900 equal
(WM_CLOSE -> exit in 0.2 s); tut1 replay frames 0..2011 equal (exit in 0.6 s). Census: 2377 + 1317
+ 183 -> 2382 lifted + 1316 SWINE-shared (SWindow::OnClose now HD's) + 179 stubs; 79 lifted bodies
with a STUB_LOG.

## Campaign shell (agent K, branch `m4-campaign`, 2026-10-07)

With `-m3`, **New Game** goes through the original's campaign screens into campaign mission 1
and, after the mission, through the results screen to the next mission of `missions.ini`.
Without `-m3` nothing changed (New Game is still the logged stub; menu CRC 0 mismatches).

| Screen / step | HD | Where |
|---|---|---|
| New Game submenu (`SSingleMenu`, 0x320): campaign pictures `menu/campaign_select_hq.tga`, Skirmish, Scenario | ctor 0x633770, Create 0x639790, OnAction 0x63c7e0, dtor 0x6343e0; SMainMenu::OnAction 0x63b6f0 | `src/panzers/campaignmenu.*` |
| Difficulty dialog (`SSingleDiffMenu`, 0x3c8): Easy / Normal / Hard, description, Start / Cancel | ctor 0x6336d0, Create 0x639540, OnAction 0x63c580 | same |
| Start 0x53441 / Cancel 0x53442: campaign, Race (+0x31c), Difficulty (+0x3c4), `InitCampaignMode` | OnAction 0x659250 | `superwindow_m3.cpp` |
| `InitCampaignMode` ("German 1" / "Allied 1" / "Russian 1", missions.ini + missions_local.ini, map, "SP") | 0x592b20 | `src/game/campaign.cpp` |
| `PrepareMission` (name guessed): props, support counts, player counters, Prestige = StartPrestige, MissionArmy = Army + "Unit %d" / "stored" / "slot 1/2", objectives; MenuToLoad 8 (Intro Anim), 0 (Diary, default 1) or 2 | 0x592d80 | same |
| Briefing (`SBriefingMenu`, 0x340): `menu/diary/<map>_hq.tga`, "Click to continue", `speech/diary/<map>.mp3`; any key / press sends 0x424d1 | 0x632f40, Create 0x634f20, 0x63e050, OnKeyDown 0x63d690, OnAction 0x63b670, dtor 0x634940 | `src/panzers/briefing.*` |
| LoadNextCampaignView cases 0 (briefing) and 3 (results) | 0x658b10 | `superwindow_m3.cpp` |
| Mission: loading screen, HQ when `Market` = 1 (Russian 2 tested), mission start | (M3) | |
| End: SStatisticMenu (M3-P) -> GV_GAMEOVER -> LetMapDone -> results | | |
| Results (`SResultsMenu`, 0x1228): debriefing page, officer and medal sheet per nation, name / rank / time / prestige / score, objectives (HD's swapped optional / secret rows kept), unit table with 12 category pages (0x63e690), Upload Result (disabled), Restart, Continue (victory only), Cancel, medals along the nation's chain, "New medal:" | ctor 0x633540, Create 0x6360f0, OnAction 0x63c230, dtor 0x6341d0 | `src/panzers/results.cpp` |
| `SGameLogic::BackupCampaignUnits` (single-player part): survivors marked in the mission army by ScriptID (XP + 100); Easy keeps, Normal resets, Hard drops the dead (a surviving crew becomes a record, a built-in driver keeps its vehicle); live units, losses %, time (frames / 20), score (+5000 victory, +8000 optional, +6000 secret). Now also called by End Mission (ingamemenu.cpp, was a STUB_LOG) | 0x561110 | `src/game/gamelogic_mission.cpp` |
| Continue 0x524d1 -> `LetResultsDone` ("Next Mission", MissionArmy -> Army (carried), StartPrestige = next "SP" + Prestige left, SaveGameBefore, PrepareMission); Restart 0x524d2 / 0x47565 -> 0x594f60; Cancel 0x524d3 -> main menu | 0x594e70, 0x594f60 | campaign.cpp, superwindow_m3.cpp |
| `GetBriefingText` 0x591e30, `GetNextMissionSP` 0x5928b0 | | campaign.cpp |

Fixes found on the way:
- `LetResultsDone` copies the mission army into the carried army (0x591500 with ECX = +0x2c);
  PrepareMission copies it back and adds the mission's own units.
- `LoadObjectives` 0x593ba0 reads the text from `missions_local.ini` (campaign +0x04), as HD does; the
  medal names too (SStatisticMenu 0x62ffa0 and SResultsMenu). Its resize-to-0 now clears the reused
  records: the second LoadObjectives of a campaign (PrepareMission, then LoadMap) wrote through a
  freed Targets array (heap corruption on the next map load).

Verified (`scratchpad\m4k\shots\k*.png`, our build only): New Game -> German -> Normal -> briefing ->
ger-01 loads and plays -> End Mission -> results (defeat) -> Cancel -> menu -> Restart reloads ger-01
-> second round trip with the Allied campaign (Allied 1 = British, James Barnes) -> menu -> WM_CLOSE
exit; `PZ_M3_FORCE_END=11`: German 1 victory -> results -> Continue -> ger-02 (panics: "SWorld::
CreateUnit: Train Carriage4 platform cannot be created (class 10)", trains not lifted); Russian 1 ->
victory (new medal) -> results (score 5000, time, the unit table row) -> Continue -> Russian 2
briefing -> HQ (prestige 200, the 7 carried units + BA 12) -> mission. Training Camp round trip
(buy, mission, End Mission, menu) still fine.
Menu CRC (75 s): 0 of 1,180 frames differ. tc1 replay: frames 0..3900 equal.

Census: 2238 -> 2263 lifted, 1317 SWINE-shared, 189 -> 186 stubs (shell 22 -> 23: SaveGameBefore;
m2 58 -> 57; m3 31 -> 28).

Nothing was SWINE-adapted: SWINE's `world/campaign.cpp` is a different design (campaign map,
briefing parts, map status), and the Panzers HD functions above were lifted from the HD exe directly.

Left:
- `BackupCampaignUnits`: the multiplayer part (SMulti names, the `armies/*.army` save) is left out.
  (The objective bonus prestige is added by trigger action 0x23 through 0x592b10: see "Small
  fixes".)
- `SPanzersCampaign::SaveGameBefore` (agent S): logged stub in `src/stubs/stub_panzers.cpp`.
- Scenario (`LoadScenarioMenu` 0x63b2d0) and Skirmish (0x534d1) buttons: logged stubs.
- Results: Upload Result / the save menu (SSaveMenu 0x633820) are not lifted; the multiplayer and
  scenario variants of 0x6360f0 are left out.
- MenuToLoad 8 ("Intro Anim", commented out in every mission) goes to the main menu as in HD.
- Cut-scenes between missions: no HD call in this flow (they are trigger-driven, agent T).
- Trains (class 10) in ger-02.



## Save / load (agent S)

The game state is saved and loaded in the original's format (docs/FORMATS.md "Save games").

### What saves and loads

| Part | HD | Recompile |
|---|---|---|
| Campaign part of a save | SaveGame 0x5966a0, LoadGame 0x594f70 | `src/game/campaign_save.cpp` |
| Game state, 19 chunks | SGameLogic 0x57e110 / 0x56eb50 | `src/game/gamelogic_save.cpp`, `src/game/world_save.cpp` |
| Units, gunners, drivers, targets | SUnit::Save 0x5be320 / Load 0x5bbd30, UNIS 0x5fb630 / 0x5f3820 | `src/game/unitsave.cpp` |
| Descriptor lists (32, from HD .data) | 0x8dc540 ... | `src/game/savedesc.cpp` |
| After-load slots | SUnit 0x5bb1c0 / 0x5baf30, SBuildingUnit 0x549500, squads 0x59cf80 / 0x598980 | `unitsave.cpp`, `unit_afterload.cpp` |
| Load Game action, load-game LoadMap | 0x659250 case 0x494c1, SGameView 0x61f840 | `src/panzers/loadgame.cpp` |
| Quick save / quick load | SGameView::OnKeyDown 0x622f50 F6 / F9 | `loadgame.cpp` |
| Save list | LoadSavedGameNames 0x595fa0 | `campaign_save.cpp` |
| Load Game screen | SLoadMenu 0x62cbc0 / 0x62f950 / 0x631f40, SSaveLoadListBox 0x53eef0..0x540200 | `src/panzers/loadgame_menu.cpp` |
| "Before" save (campaign) | SaveGameBefore 0x596b30 | `campaign_save.cpp` (`pz::CampaignSaveGameBefore`, for K's LetResultsDone stub) |

The mission-start autosave, F6 (`SaveGames/quick.save`), F9 and the main menu's Load Game screen
(the saves with code, title and date, newest first; Load or a double click loads) work. SSaveMenu
(the in-game save screen with a name) is not lifted: the in-game menu's Save Game saves as F6.

### Byte compatibility

Our Training Camp mission-start autosave (tc1.rec start: German, Panzer III F + Riflemen) against the
original's `TRNG-Start.save` (scratch `m3f\ref_TRNG-Start.save`): same size (2,572,085 bytes), 184
bytes differ, all of them the `VoiceVar` (+0xdc) of 184 units. That value comes from the CRT `rand()`,
which both games seed with the time at start-up, so it also differs between two runs of the original.
Fixed on the way: SUnit::SetGlobalState 0x5b7390 cleared +0xf8 (`MoveState`), which HD keeps.

The original's own save loads without a warning (358 units, seed 141334d6, every chunk read).

### Round trip

Test hooks (recompile only): `PZ_M4_SAVE_AT=<frame>` saves `SaveGames/M4RT-<frame>.save` between two
logic ticks, with a sidecar `.rt` (the `-packetplay` position, the CRC queue, FramesSent);
`PZ_M4_LOADGAME=<file>` loads it through the Load Game action from the first main menu; with
`-packetplay Replays\tc1.rec PZ_M3_NAIVE_PLAY=1 PZ_M4_RT=1` the replay goes on after the load.

Saving tc1 at frame 1000 and loading it:

- `PZ_M4_FIXFIRST=1`: the world CRC of frame 1000 after the load is the original's
  (`7460c00e f14045bc 357`). The run then goes on to frame 2275 without errors, but frame 1001 differs
  (seed d16309ba instead of f9642426).
- `PZ_M4_VISION=1` as well (every player's vision map built right after the load): the Panzer III
  F's targeting and the LastHeardFrame differences below go away; what is left are squad and crew
  members that move slightly.
- HD order (default): frame 1000 differs already (`752283d7`, same seed and unit count). HD fixes the
  bridges after the units' after-load slots, so units on platforms (the 88 mm flak and its crew) get
  their height from the unfixed terrain until their next refresh; it also shifts LastPos / PreLastPos
  in LinkAfterLoad. `PZ_M4_FIXFIRST=1` fixes the bridges first and keeps the loaded interpolation state.

What still differs one tick after the load, by saving frame 1001 in both runs and comparing the
files: squad and crew members that stand still in the saved run move slightly, the Panzer III F's
gunner does not acquire unit 150, and a squad's LastHeardFrame. These come from state the save
does not hold, in HD too: the per-player vision maps (rebuilt one player per tick), the member
formation state and the animation state.

### Regressions (final build)

- Menu CRC: 0 mismatches over the 1,180 frames of `docs/re/m2_crc_original.txt` (75 s, `-nointro`).
- Training Camp replay: CRC, seed and units equal on every frame 0..3900, 0 Inconsistency lines.

### Left

- SSaveMenu 0x632040 (the in-game save screen), LoadGameBefore 0x595330 (type 2 saves).
- Wires: the recompile keeps the map's WIR3 raw; the third heap (+0x7454) is written empty and WIR3
  is skipped on load.
- Partly lifted in a load: ODDD (the heap only, doodad animations are not restarted), AMOD (records
  only, no models), ECHO (board texts are not kept), OBJT (records only, 0x560eb0's board marker),
  the board elements of the after-load slots, the capture flag 0x5471d0.



2026-10-07, branch `m4-tutorial` from master fe92a6a. All addresses are HD `PANZERS.exe`.

## The Tutorial replay matches the original: frames 0..2011

The original recorded one tutorial session (scratch `m4ref\`, README there): Tutorial button, the
cut-scene tutorial-01 played to its end, objective 1 (camera), the rifle squad selected and sent to the
guard post (objective 2 completed, the Russian truck created), camera scrolling, End Mission.

```
copy m4ref\tut1.rec <run>\Replays\tut1.rec
set PZ_M2_CRC=1
set PZ_M3_REPLAY_PAUSED=1
PZM4T.exe -nointro -m3 -packetplay Replays\tut1.rec
mdiff.py <log> m4ref\tut1_crc_mission.txt          (m3i2\tools, compares by frame number)
```

**CRC, seed and unit count equal the original on every frame 0..2011** (the whole recording): the
in-game cut-scene (frames 0..221: the truck drives in, unloads the squad, the squad kneels and moves,
the truck is removed), the selection packet (778), the move (799), trigger T4 (943: objective 2 done,
`SU Gaz AA truck` created at location 13) and the triggers after it.

| Frame | Original | Ours |
|---|---|---|
| mission start 0 | `265af43f 8d78afc6 103` | `265af43f 8d78afc6 103` |
| 1 (seed reset by the cut-scene) | `c0931c67 00269ec3 103` | `c0931c67 00269ec3 103` |
| 2011 | `f35220c6 a6fdfa4b 102` | `f35220c6 a6fdfa4b 102` |

Before M4 the replay differed from frame 1 (no cut-scene: the seed was not reset to 0 and the truck
never moved).

## The path

- **Tutorial button** (`src/panzers/tutorial.cpp`): SSuperWindow::OnAction 0x659250 case 0x4d4d3
  (delete the main menu, new SPanzersCampaign 0x590ec0, `InitTutorialMode("maps/tutorial.map", 1, 0)`
  0x594ab0, LoadNextCampaignView 0x658b10). MenuToLoad 2: the game view, no market; "Map loaded" ->
  mission start, as Training Camp. Hook: one line in SuperWindowM3Action (superwindow_m3.cpp).
- **Cut-scene** (`src/game/cutscene.cpp`): trigger T0 action 0x38 -> 0x56ea20 -> the .ingame kind
  (cutscenes/tutorial-01/tutorial-01.ingame exists, so 0x56f530):
  - `SInGameAnimLogic::LoadProject` 0x589d90: ING0 / LAFN (length 220) / ISL0 lines (0x5883f0, sorted
    by frame), the type-0 lines become the logic's statement list (0x58be80), **world seed = 0**
    (0x58a566);
  - 0x56f530: camera state saved, everything deselected, game speed 1, vis overlay 0, World +0x4d0,
    camera limits off, every squad / vehicle / gun / building stopped (0x5bb7b0 order 8) and its
    targets cleared (+0xc4), unit +0x1d4 cleared, statements preprocessed, SGameLogic +0x2b8 set;
  - 0x568af0 per logic tick (from Refresh): the statements of this frame through
    `SGameLogic::ExecuteScriptStatement` 0x568bc0 (`src/game/triggers_script.cpp`: HD's command table
    0x8db080, argument parsing, the units named by script id, move / move_backward / attackground /
    attackmove / stop / die / boomdie / remove / stand / kneel / lay / unloadall), the end after the
    length (frame 221);
  - 0x565390: the end (camera restored, limits on, +0x4d0 off, vis overlay back).
  - **The camera** (between ticks, from UpdateUnitVisuals 0x5638f0 -> 0x58adc0): the CMS3 / CMS4
    tracks (20 cameras; key 0x28: frame, target, yaw, pitch, distance, tension / continuity /
    bias), the Kochanek-Bartels splines 0x58b760 / 0x672dd0 / 0x673c00, the evaluators 0x6749e0 /
    0x676200 (with HD's ease remapping), the camera of each frame 0x58ae50, the apply step 0x589970
    (camera mode 0x5fd9f0, near / far, 0x5fd880 target or eye, 0x5ef360 no smoothing), and the
    letterbox: SGameView::Update switches to panel mode 2 while `IsPaused && 0x589860` (one hook in
    gameview_view.cpp). Screenshots of ours and the original's (m4ref `o_cut2.png`) frame the
    truck the same way.
  - **The colour fade**: `change_camera_colour` lines become a colour per frame (0x58b170: from
    the line's frame for ceil(seconds * 20) frames). They are interpolated between frames
    (0x589970) and drawn as a board box over the view (0x58c5d0, tutorial_msg.cpp). Our shot about
    1 s into the cut-scene equals the original's `o_cut1.png`: the same dark fade-in and the same
    framing.
  - **Not shown**: the subtitles (.sub, none for tutorial-01) and the cut-scene mp3 (missing in the
    original too: "Can't load sounds/cutscenes/tutorial-01/tutorial-01.mp3"). One STUB_LOG.
  - Esc: HD has no skip call for the .ingame kind (0x565390 is called only by 0x568af0 and the logic
    teardown 0x55fe20); Esc during it was not tried in the recording. The .4d kind (0x56f0d0) is
    logged and skipped.
- **Messages** (`src/panzers/tutorial_msg.cpp`): `SPanzersCampaign::InitTriggerTexts` 0x594540
  (maps/tutorial.txt, `#key` blocks, colour / alignment tags; HD keeps them at campaign +0x120, the
  recompile in tutorial_msg.cpp, loaded on first use), the static echo lines 0x5815e0 / 0x5638b0, the
  fading lines `SGameLogic::StaticMessage` 0x56a480 / 0x563860 and their ageing (part of 0x578b00:
  a line goes after 400 ticks; HD also fades its alpha).

## Trigger actions added (RunTriggers 0x579ab0, `src/game/triggers.cpp`)

| Action | What | HD |
|---|---|---|
| 0x0d | found units along a path (order 6) | 0x564510 |
| 0x15 | enter the first building / unit in a location (order 0x2a) | 0x581930, 0x564720 |
| 0x16 | unload all (order 0x2b, -1) | 0x564870 |
| 0x17 | camera to a location | 0x5f4f60 |
| 0x1b | give the found units to a player | `SUnit::SetPlayer` 0x5c1630 |
| 0x23 | objective completed (messages, prestige, state 2, victory when every main objective is done) | case body + 0x579420 (markers: records only; music lifted by agent FX) |
| 0x24 | health of the found units and their members, percent | case body |
| 0x27 / 0x28 | behaviour (order 0x21) / global state (order 0x28) | 0x564870 |
| 0x2a | stop (order 8) | 0x564440 |
| 0x2b | unit +0x20 with the slot pair | case body |
| 0x2c | move facing a direction | 0x57f200 / 0x57e8b0 |
| 0x2d | one-shot effect at a location (guide arrows) | pixie +0x10 / +0x24 / +0x20 |
| 0x2f | wait Num ticks | running trigger +0x08 |
| 0x37 | XP to the found units | unit +0x8c |
| 0x38 | play a cut-scene ("Loading cutscene..." after frame 0; map cut-scenes: **STUB_LOG**) | case body, 0x56ea20 |
| 0x3a | weather | 0x5fdc80 |
| 0x40 / 0x41 | echo / message text blocks | 0x5815e0 / 0x56a480 |
| 0x42 | speech | 0x600770; played by the trigger part of SWorld::UpdateSpeech 0x607f50 (wall clock) |
| 0x49 | found units +0x140 / +0x14c off | case body |

Condition 3, special variable 0x40000004 (mean health in percent of the found units) lifted
(0x580600; T6 of the tutorial); 0x40000008 (ammunition) is lifted too (agent FX). Every condition and event
type the tutorial uses (conditions 2, 3, 5, 6, 7, 9; events 0..6, 8, 9) is present.

**Objectives**: action 0x23 sets the objective state that the objectives screen
(SInGameBriefingMenu, agent H) reads, so "Completed" shows there. **The tutorial's objectives screen
is empty, though.** missions.ini `[Tutorial]` has only `Objective 1 Main = 1`. The text is in
missions_local.ini (`Objective 1 Text = Follow the on-screen instructions...`), and HD 0x593ba0
reads "Objective %d Text" from the **local** properties (campaign +0x04, `mov ecx, [esi+4]` at
0x593c29). The recompile's `SPanzersCampaign::LoadObjectives` (campaign.cpp, agent K) reads it from
MissionProps (+0x00) and stops at the first objective. The fix is one line there: read the text
from `LocalProps`.

## Manual check

`PZM4T.exe -nointro -m3`: Tutorial (843,369) -> "Click to continue" -> the cut-scene (letterbox,
the camera follows the truck) -> objective 1 text and the guide arrow -> click the squad
(529,326), right-click the guard post (705,155) -> "Destroy the Russian truck!" (T4 created the
truck, T5 started). The screens match the original's `o_g1` / `o_g2`.

## Regression checks (this branch)

- Menu CRC (`PZ_M2_CRC=1`, 75 s, `-nointro`) against docs/re/m2_crc_original.txt: 1180 frames, 0
  mismatches.
- Training Camp replay tc1 (`-nointro -m3 -packetplay Replays\tc1.rec`, `PZ_M2_CRC=1
  PZ_M3_REPLAY_PAUSED=1`) against m3ref\tc1_crc_bf_full.txt: frames 0..3900 all equal (3,901 frames).
  When WM_CLOSE arrives at the end of this replay, the process has to be killed after 8 s. master
  fe92a6a behaves the same, so this is not new. (Fixed by agent FX: SWindow::OnClose 0x544c40.)
- Census: 2275 lifted + 1317 SWINE-shared + 193 stubs; 81 lifted bodies with a STUB_LOG (master:
  2238 + 1317 + 189; 79).

## Left

- The cut-scene subtitles and sound (0x56fd70 / 0x5826c0); the .4d cut-scenes (0x56f0d0) and the
  map cut-scenes of action 0x38. The colour box is sized to cover any window, not to the
  viewport (viewport +0x10 is untyped).
- (The black tutorial minimap, the objective music, 0x579420 and condition variable 0x40000008
  are done: see "Small fixes" above.)
- campaign.cpp LoadObjectives text from LocalProps (agent K, above).
- Action 0x21 / 0x22 (the objective markers: 0x560eb0 board frames and minimap markers) and the
  board alpha fade of 0x578b00.
- The tutorial was replayed to objective 3 (frame 2011); the later objectives (house, AT rifles, the
  captured gun, the towed howitzer, the forest, the bomber) are not verified.
