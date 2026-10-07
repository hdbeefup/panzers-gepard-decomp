# M5 SC: Scenario mode

Agent M5-SC, branch `m5-sc`. Scenario mode works with `-m3`: the Scenario menu
(New Game > Scenario), the mission, the results screen's scenario variant, and back to the main menu.
The command-line map start (`-map <file>`) also works, but only with the fix described below.

## How to use it

Scenario menu (needs `-m3`, like every campaign screen):

    panzers.exe -nointro -m3
    Main Menu > New Game > Scenario > pick a map > Load (or double-click it)

Command-line map start (any map, the fastest way into a mission; `-m3` is switched on by itself):

    set PZ_M5_MAPFIX=1
    panzers.exe -nointro -map maps/ger-01.map
    panzers.exe -nointro -map scenarios/ah-01.map
    panzers.exe -nointro D:\full\path\to\some.map      (a bare map path: HD stores _fullpath of it)

On a build with `-DPANZERS_MOD_BUGFIXES=ON`, `PZ_M5_MAPFIX` is not needed. Without either of them
the faithful build crashes after the first frame (HD bug, see below).

What happens: scenario mode with the map, section `""`, race 0 and prestige 0. With an empty section
`LoadMap` starts the mission at once: there is no briefing, no market and no "Click to continue". The
campaign's race becomes the map's local player race (World +0x174 + LocalPlayer * 0x48). A map with
an opening cut-scene runs it first (ger-01: about 80 s, 1550 frames). End Mission (Esc > End Mission
> Yes) or the mission's own end leads to the results screen. Continue there goes to the main menu.
`-map maps/training.map` loads and plays with the fix.

## What is lifted (HD addresses)

| HD | What | Where |
|---|---|---|
| 0x5944d0 | `SCampaign::InitScenarioMode(map, section, race, prestige)`: GameMode 1, Race, MapName, StartPrestige, MissionResult 0, MissionSection, PrepareMission 0x592d80, MenuToLoad 2; it panics on a second Init | `src/game/campaign.cpp` |
| 0x63b2d0 | `SSingleMenu::LoadScenarioMenu`: deletes the old +0x58 and makes a new SScenarioMenu (0x20c) as a child of the menu's parent, then Create | `src/panzers/scenariomenu.cpp` |
| 0x639000 | `SScenarioMenu::Create`: the "Scenario" centre menu, a Load button at the right-hand slot (0x64bfd0), the "Maps" caption, a 14-line list (0x53c260(2, 0xe, 1, 1, 0, 1)) of `scenarios/*.map` (0x65e720), qsort by stricmp (0x634d30). Each entry shows the name after `scenarios/` (or `maps/`); any other prefix panics with "Invalid filename". The first entry is selected; with no entries Load is off | same |
| 0x63c450 | `SScenarioMenu::OnAction`: Load click (0x42542 from +0x18c) or a list double-click (0x4c423) deletes the campaign and makes a new one, sets campaign +0x20 = the selected file, sends 0x53431 (param = index) | same |
| 0x634bf0 | SScenarioMenu dtor (frees the file list) | same |
| 0x6343e0 | SSingleMenu dtor now deletes +0x58 (the scenario menu) | `src/panzers/campaignmenu.cpp` |
| 0x659250 case 0x53431 | copies the campaign's map name, deletes the main menu, Cursor 0 (0x543970), new campaign, InitScenarioMode(map, "", 0, 0), LoadNextCampaignView, Race = the local player's race | `PzScenarioAction`, scenariomenu.cpp (hooked into `SuperWindowM3Action`) |
| 0x65b470 map branch + 0x6585e0 | Initialize. Without SMulti: new campaign, InitScenarioMode(map, "", 0, 0), then 0x6585e0 (UnloadMenuBackground, new SGameView, add, 0,0,0x400,0x300, Create, +0xd8 = 1, LoadMap; this is LoadNextCampaignView case 2, which is reused), then Race as above. With SMulti: 0x658050. The old recompile also loaded the main menu here; HD does not | `PzStartMapFromCommandLine`, scenariomenu.cpp; `SSuperWindow::Play` |
| 0x6360f0 (scenario parts) | Results, scenario / mode 2 / multiplayer variant: the title "CONGRATULATION <player name>" (SSettings +0x174 via 0x64e070), no "Name:" line, the player record gets the player name (0x636c15), only **Continue** at the Cancel slot (x 0x300, 0x637d84), no medals (0x637dc9 -> 0x638fa7) | `src/panzers/results.cpp` |

How HD finds scenarios: `0x65e720("scenarios/*.map")` searches every search-path directory and pak plus
the loose folder, and each name gets the `scenarios/` prefix. The install's `scenarios\` folder holds
`ah-01.map`, `ah-02.map` and `ah-03 bonus.map`, so the list shows those three. The recompile uses
`SFileSystem::FindFiles("scenarios/", "*.map")`, which drops duplicate names. HD's 0x65e720 may list a
file twice if two search-path entries hold it (not checked).

Mission end in scenario mode: `LetMapDone` (modes 1-4) leads to MenuToLoad 3 (results). On the results
screen, Continue (0x524d1) calls `LetResultsDone`, which leads to MenuToLoad 5 (main menu, campaign deleted).
`PrepareMission` with an empty section finds no `<dir>/_mission.ini` and reads `missions.ini`. That gives
no mission units, no objectives and no "Market" key. `LoadMap` starts the mission at once because the
section is empty.

## HD bug kept: the command-line map start

`Initialize` calls `LoadMenuBackground(1)` (0x658690). That loads the menu world (SWorld +0x19c,
SGameLogic +0x1a0) but makes no menu frames. 0x6585e0's `UnloadMenuBackground` (0x65b940) only acts
when the top frame +0x194 exists, so it keeps the menu world. The mission's SWorld and SGameLogic ctors
then delete it as the previous `g_World` / `g_GameLogic` (0x5d2f90 / 0x55e440). After that, `OnIdle`
(0x65ae50) keeps refreshing the freed menu logic, which is a use after free. HD panics with
"SHeap<struct SFrame>::operator[]: invalid index" (docs/M3_REPLAY.md, training.map). Our faithful
build gets an access violation in `SGameLogic::Refresh` on the first frame. The fix runs under
`PANZERS_MOD_BUGFIXES`, or under the test hook `PZ_M5_MAPFIX=1`, which does nothing when unset. It
unloads the menu world before the campaign is made, the way every other path does.

The same switch also handles the window messages that queued up while the map loaded (no message loop
yet) before the first frame. Without this, the queued `WM_SIZE` reaches `SDXWindow::OnSize` after the
mission has drawn a frame. The SWINE device Reset then fails with D3DERR_INVALIDCALL and keeps failing
(a default-pool resource of the mission scene is not released before Reset), and the next texture or
vertex buffer creation panics. The menu paths do not hit this because their `WM_SIZE` comes between the
load and the first frame. **This is a renderer issue (not mine):** a WM_SIZE / device reset during a
mission fails in every mode. The owner of `gepard.cpp` / `pzgepard.cpp` PreDeviceReset should look at it.

## Verified (our build, `scratchpad\m5sc\shots`)

- `scenario.png`: New Game > Scenario shows the list of ah-01 / ah-02 / ah-03 bonus, the "Maps"
  caption and Load, drawn over the New Game menu.
- ah-01 from the menu (`sc_play2.png`, Allied, race 1 from the map) played for 60 s. Then Esc > End
  Mission > Yes led to the results screen (`sc_end2.png`: DEFEAT!, player name "swine", only
  Continue). Continue led to the main menu (`sc_back.png`). Opening Scenario again worked, and
  WM_CLOSE exited with code 0.
- `-map scenarios/ah-01.map` with `PZ_M5_MAPFIX=1`: mission, results, Continue, main menu, exit 0.
  `-map scenarios/ah-02.map`, `maps/ger-01.map` (cut-scene) and `maps/training.map` load and run.
- `-map` without the fix: crash on the first frame (the HD bug above).

## Regression (regress.ps1)

I ran it five times, four on this branch and two on master's `coord\run\rebuild.exe` from the same
run folder. Each check passes on this branch in at least one run:

- menu: 0 of 1181 frames differ (all runs).
- tc1: 3901/3901 equal (run 3).
- Tutorial: 2012/2012 equal (run 4).

The failed runs stop at frame 260 or 270, and every frame up to that point is equal. Each one is the
renderer problem described above: a late `WM_SIZE` (`SDXWindow::OnSize`) arrives mid-mission, the
device Reset fails, and the next CreateTexture / CreateVertexBuffer panics. Master's exe fails the same
way (tc1 failed in both master runs), so this depends on the machine and how busy it is, not on this
branch. The scenario code is not on the replay or tutorial paths (no PZM5 log lines there).

## Census

Before 2432 lifted + 1316 SWINE-shared + 174 stubs. After: 2440 lifted + 1316 SWINE-shared + 171
stubs (InitScenarioMode, PzStub_LoadMapFromCommandLine and the LoadScenarioMenu STUB_LOG are gone).

## Left / not done

- `SSuperWindow::Play` with SMulti (0x658050) is still the chat-room stub (no SMulti in the recompile).
- The results screen's multiplayer parts (SMulti names) are not lifted. Mode 2 and multiplayer use
  the same Continue-only branch.
- HD's 0x6585e0 sets SDXWindow +0xd8 = 1 (continuous render). The reused LoadNextCampaignView case 2
  does not model it, the same as before.

## For the coordinator: references to record from the original

Run the HD exe from a scratch copy:
1. `PANZERS.exe` > New Game > Scenario: a screenshot of the scenario menu (list, caption, Load position).
   Check whether a name appears twice.
2. Load ah-01 > Esc > End Mission > Yes: a screenshot of the results screen (title, buttons, whether
   the medals are hidden).
3. `PANZERS.exe -map scenarios/ah-01.map`: does it panic like training.map does ("SHeap<struct
   SFrame>::operator[]: invalid index")? Record the log's last lines.
