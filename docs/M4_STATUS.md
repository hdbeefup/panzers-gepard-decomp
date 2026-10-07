# M4 status

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
  The objective bonus prestige of the statistics menu (60 / 40 per objective) is shown but not added
  to the campaign (no HD writer found yet).
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
