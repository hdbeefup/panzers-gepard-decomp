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
