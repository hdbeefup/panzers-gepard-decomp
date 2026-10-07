# M5-MS: campaign mission sweep

Agent MS, branch `m5-ms` (from master b298750), 2026-10-07. All addresses are HD `PANZERS.exe`.

Goal: every campaign mission of `missions.ini` (German 1-12, Allied 1, 2, 4-10, Russian 1-9; 30
missions) loads and runs with the AI.

## Test hooks (recompile only, inert when unset)

`src/panzers/m5mission.cpp`, one line each in `InitCampaignMode` (campaign.cpp) and in
`SSuperWindow::LoadMainMenu` / `OnIdle` (superwindow.cpp).

| Variable | Effect |
|---|---|
| `PZ_M5_MISSION=<section>` | `InitCampaignMode` 0x592b20 starts at that `missions.ini` section ("German 5", "Allied 4", ...) instead of "<nation> 1"; the race follows the section's first word (German 0, Allied 1, Russian 2). Works from the New Game screens too. |
| `PZ_M5_AUTO=1` (with `-m3` and `PZ_M5_MISSION`) | The first main menu starts New Game as 0x53441 does (difficulty `PZ_M5_DIFF`, default 1 = Normal); the briefing (0x424d1), the "Click to continue" loading screen (a key, 0x47561) and the market (Start, 0x4d542: nothing bought, the carried army is empty) are passed by their actions, 1.5 s apart. |
| `PZ_M5_FRAMES=<n>` | At logic frame n: `PZM5: frame n reached, <units> units` in the log and WM_CLOSE to the window. |
| `PZ_M5_NOGHOST=1` | `DisableProcessWindowsGhosting` at the first main menu (busy-machine test runs; the device-reset fix below makes it optional). |
| `PZ_M5_MARKETWAIT=<s>` | With PZ_M5_AUTO: stay s seconds in the market before Start (screenshots). |
| `PZ_M5_WATCHDOG=<s>` | A thread writes the main thread's stack to `hang.txt` when `OnIdle` has not run for s seconds. |

Example: `set PZ_M5_MISSION=Allied 2 & set PZ_M5_AUTO=1 & set PZ_M5_FRAMES=1500 & PZM5MS.exe -nointro -m3`.

## Sweep script

`scratchpad\m5ms\sweep.ps1 <exe> [-Missions ...] [-Frames 1500] [-Tag name] [-MaxSec 300]`: per mission
it starts the exe with the hooks above, waits for the exit (frame 1500 = 75 s of game time at 20 Hz;
the AI and the mission triggers run, the player's units get no orders), kills it on a panic, a
crash.txt or the time limit, and writes `<tag>.tsv` (mission, status, last frame, seconds, first
problem, stub count) plus `<tag>\<mission>.log.txt / .stubs.txt / .crash.txt`. A run whose window
lost the device (`Reset failed` in the log) is marked `(reset)` and retried once; "Warning" message
boxes (SLogger::Warning, HD 0x65cac0 shows them too) are closed with OK and counted (`(warn n)`);
`PZ_M5_WATCHDOG=45` turns a blocked main thread into status HANG with its stack; a mission that ends
(the results / end box, "PZM3: mission result") is status END<result>. It sets PZ_M5_NOGHOST=1.
`shot.ps1` (screenshots of a mission at given seconds) and `sizetest.ps1` (WM_SIZE in a mission)
are next to it.

## Fixes

| Problem (missions) | HD | What |
|---|---|---|
| Allied 2 crashed while loading (`SSingleUnit::InitCrewAndChildren`, null animation); German 4 and 6 had boats without an animation | `SPBoatAnimation::CreateAnimation` 0x5c76d0, `SBoatAnimation` dtor 0x5c70d0, InitModel 0x5c8bf0, UpdateModel 0x5cb4e0 | `src/game/boatanim.*`. The boat (0xc8, an SVehicleAnimation) stands on the water height (0x5ec490), turned by pi, level; unit +0x2e9 (EnableUnloadButton) is set off static block bit 0x400. The water height and the block test reach the animation through two new `g_UnitAnimEnv` members (animview has no SWorld). |
| Capturable buildings never changed owner and showed no flag; support places, radars and hangars did nothing (20 missions hit the STUB_LOG) | `SBuildingUnit::RefreshTargeting` 0x54a250 cases: capture 0x54cd70, CreateCaptureFlag 0x5471d0, radar 0x54ac00, support place 0x54acd0 and its heal order 0x54a4a0, 0x548bf0, hangar 0x54a380 | `buildingunit.cpp`. The owner follows the units in capture range: an own unit keeps it (a neutral building becomes owned), else the last allied player seen, else the last enemy; a change clears +0x110 and +0x2cc and renews the flag (flag.4DA on the "flag" node, its texture scrolled to the owner's race). The support place repairs, supplies, heals and refills the cargo of repair / supply trucks. The productive building (unit type 0x1a, 0x54a660) is still a STUB_LOG. |
| Repair and supply trucks never worked (`SUnit::NeedsRepair` returned false; the orders and their refresh were STUB_LOGs; 11 missions) | NeedsRepair 0x5bc840, EC_Repair 0x5c1ca0, EC_Supply 0x5c1e60, ServerRefreshRepair 0x5c01e0, ServerRefreshAmmo 0x5bf280 | `unitai.cpp`, next to the medic (0x5bfa50) they mirror: the HP, then the first damaged armour side, 0.4 / the prototype value per tick; ammunition 1 / Ammo every 10th frame; the cargo cost; a "Projectile service / armor / ammo" every 9th frame. |
| Cut-scene eye cameras (type 2) left the view on the last camera (13 missions) | `SWorld::ComputeCamera` 0x5ddc30 mode 2; mode 0's followed unit (World+0xa4) | `src/world/world.cpp`. Eye and angles, 3D listener, focus height from the view ray (0x5ea910), projection with the distance track as the field of view. Mode 1 (free fall) is still a STUB_LOG. |
| `SPUnitAnimation` +0x0c logged as a stub in 18 missions | 0x5cb470 is a bare `ret` | Empty body. |
| A device Reset in a mission (WM_SIZE: restoring the window, or the ghost window going away after a long map load on a busy machine) failed with D3DERR_INVALIDCALL, then the game panicked in CreateVertexBuffer / CreateTexture or crashed in `SModel::BuildNodeBlockBitmap` (German 10 and 12, Allied 6 and 7, Russian 3 in the first sweep; the tc1 / tut1 regression runs too) | (SGepard::ResetDevice 0x67fde0 frees the scenes' shadow buffers and offscreen viewports first) | `src/3dengine/pz/pzgepard.cpp`: the facade registered its pre/post-reset hooks only if the SWINE Gepard already existed, and it is made earlier (whose ctor also clears the hooks), so they never ran. `HD()` now registers them when it first sees the device. Checked by posting WM_SIZE in German 2: "offscreen viewports released", Reset OK, the view renders on. |
| 28 of the 59 trigger action types that the campaign maps use did nothing (the `default` of RunTriggers, logged at level 1, which the log does not keep) | RunTriggers 0x579ab0 cases | See "Trigger actions" below. |
| Russian 3 hung in its opening cut-scene and Russian 6 stopped twice: a modal "Warning" box ("There's a PrimaryTarget but no CurrentTarget") every tick | `SUnit::ServerRefresh` 0x5bee90 passes that text to 0x568ae0, an empty function | `unitbase.cpp`: a level-1 log line instead of `Logger.g->Warning`. The other 35 `Warning` texts in src/game, src/world and src/panzers were checked against the HD calls: they do go to the box 0x65cac0 (or to other loggers), as lifted. |
| Crew members, drivers, bombs and the new capture flags were never detached from their parent node (a STUB_LOG) | `SModel` +0xe0 0x6d7310 | `pzmodel.cpp`: the parent's DetachChild 0x6d7340 with the attach node. |

## Trigger actions

The TRIG dumps (`PZM2 A type=`) of the 30 missions list the action types each map uses. Before
M5 these were missing (missions using them in brackets): 0x22 objective failed (29), 0x21 show
objective (25), 0x3f play music (25), 0x0f attack-move along a path (22), 0x4a speech and wait (19),
0x1c AI group attack-move along a path (17), 0x3e hide an objective target (15), 0x12 tactical
bomber (11), 0x34 AI group base (10), 0x14 paratroopers (7), 0x10 artillery (6), 0x33 AI group
tactic (5), 0x11 recon plane (4), 0x35 / 0x36 AI group move / attack-move (4), 0x13 heavy bomber
(3), 0x45 unit counter on (3), 0x0c defeat (2), 0x1a untow (2), 0x43 / 0x44 time counter (2),
0x19 tow (1), 0x20 kill (1), 0x32 move backwards facing (1), 0x39 lay tank mines (1), 0x46 (1),
0x47 use boat (1), 0x4b remove buildings (1). Names from the HD editor table at 0x7fa3a4.

Lifted now (all case bodies of 0x579ab0, `triggers.cpp`): 0x0b / 0x0c (the player's +0x18c, the
local player's mission result, out of the game), 0x0f (order 10 along the path, 0x564510), 0x10..0x14
(free support calls of the player to the location centre: 0x5674c0 / 0x568300 / 0x568740 / 0x567760 /
0x567d40, already lifted for the HUD), 0x1a (order 0x2d), 0x1c / 0x33..0x36 (SAIGroup MoveAlongPath
0x5d9750, Tactic, StartPos, MoveTo 0x5f5000, AttackMoveTo 0x5d95b0), 0x20 (order 0x26), 0x21 ("New
mission objective:", the text, +0x04 cleared, a minimap record per target, 0x560eb0), 0x22
("Objective failed:", state 1; a main objective is the defeat of team 1), 0x39 (order 0x1e along the
path), 0x3e (0x579420(objective, target)), 0x3f (the playlist with the track, `results.cpp`
PzPlayTriggerMusic), 0x43..0x46 (SGameLogic +0x14c / +0x14d), 0x47 (orders 8 and 0x1a), 0x4a (the
speech plays at once and the trigger waits its length * 20 frames, `tutorial_msg.cpp`
PzSpeechPlayNow), 0x4b (found buildings removed, 0x5f8060), 0x32 (as 0x2c with the backwards
commands 4 far / 3 near).

0x19 (tow, German 5) needed the towing test `SSingleUnit` +0x58 0x5aaa30 (a unit of the tower's player, or
+0x110, with a "hole_f" / "hole_r" node), lifted too; the found units tow the first such unit in the
location (order 0x2c). `SSingleUnit` +0x54 0x5aaa80 (a "hook" node and nothing towed) is lifted
as well, so the action cursor offers towing again; the towing itself (agent TR's
GhostFrames_AddTop) is still untested in play. Every action type the 30 campaign maps use now has
a case.

| Problem (missions) | HD | What |
|---|---|---|
| The near branches of trigger actions 2 / 0x0e (the group already around the target) were STUB_LOGs (German 12) | 0x57e8b0 with command 1 / 9 | `triggers.cpp`: MoveFoundUnitsNear, as 0x2c already did. |
| `SUnit` +0xc8 was a STUB_LOG (a squad in Russian 8) | 0x5b9170 | `unit.cpp`: the current target released, StopGunners (+0xec), Stop (+0xc0). |
| The HQ warehouse offered every unit with a buy range in every mission (STUB_LOG in 17 missions) | `SMarket` 0x644fc0 single-player branch, `SPanzersCampaign` 0x5920f0 | `market.cpp` / `campaign.cpp`: a unit is offered while the mission's "Mission number" lies in [MarketBuyFirst, MarketBuyLast] (-1 = open end). German 3 now offers Panzer I, Panzer II and SdKfz 223 among the tanks (`shots\g3market_022.png`). |

## Sweep results

Our build only. Per mission: New Game on Normal, nothing bought, the player's units idle; the AI
and the mission triggers run. 1500 frames = 75 s, 2400 = 120 s of game time. "Before" is master
b298750 (two instances in parallel); s1 = after the boat, building, repair and camera lifts (one
instance); s2 = after the reset hooks, trigger actions, warning and detach fixes (2400 frames);
final = 5f9e337 (s3, 1500 frames); German 12, Russian 8, Allied 2 and Allied 4 again on 772d509
(the last two fixes) at 2400 frames: all OK. Before, Allied 5 / 8 and Russian 2 / 4 also lost
their device (Reset failed in a loop, nothing drawn) but their logic ran to the end.

| Mission | Before (master, 1500 frames) | Problem before | s1 (1500) | s2 (2400) | Final build (1500) | Notes after |
|---|---|---|---|---|---|---|
| German 1 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| German 2 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| German 3 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| German 4 | OK 1501 |  | OK 1500 | OK 2400 | OK 1500 |  |
| German 5 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| German 6 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| German 7 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| German 8 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| German 9 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| German 10 | CRASH -1 | crash after a failed device Reset (fixed: reset hooks) | OK 1500 | OK 2400 | OK 1500 |  |
| German 11 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| German 12 | CRASH -1 | crash after a failed device Reset (fixed) | OK 1501 | OK 2400 | OK 1500 |  |
| Allied 1 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Allied 2 | CRASH -1 | crash in InitCrewAndChildren: no boat animation (fixed) | OK 1500 | OK 2400 | OK 1500 |  |
| Allied 4 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Allied 5 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Allied 6 | CRASH -1 | crash after a failed device Reset (fixed) | OK 1500 | OK 2400 | OK 1500 |  |
| Allied 7 | CRASH -1 | crash after a failed device Reset (fixed) | OK 1500 | OK 2400 | OK 1500 |  |
| Allied 8 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Allied 9 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Allied 10 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Russian 1 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Russian 2 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Russian 3 | CRASH -1 | crash after a failed device Reset (fixed), then the Warning box every tick (fixed) | HANG(warn 54) -1 | OK 2400 | OK 1500 |  |
| Russian 4 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Russian 5 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Russian 6 | TIMEOUT -1 | blocked by the Warning box at frame 251 (fixed) | OK(warn 2) 1500 | OK 2400 | OK 1500 |  |
| Russian 7 | OK 1500 |  | OK 1500 | OK 2400 | OK 1500 |  |
| Russian 8 | OK 1500 |  | OK 1501 | OK 2400 | OK 1500 |  |
| Russian 9 | OK 1501 |  | OK 1500 | OK 2400 | OK 1500 |  |

30 of 30 missions reach frame 1500 (and 2400) without a panic, crash, hang or Warning box.

## Census, regressions

Census: master 2432 lifted + 1316 SWINE-shared + 174 stubs (76 lifted bodies with a STUB_LOG) ->
2451 + 1316 + 160 (69). SWINE-shared unchanged.

Regressions (`coord\regress.ps1`, without PZ_M5_NOGHOST, final build 772d509): menu CRC 0 of 1181
frames differ; tc1 replay 3901 / 3901 frames equal; Tutorial replay 2012 / 2012 equal; all three
runs exit on WM_CLOSE ("closed"), no PANIC / EXCEPTION lines. (Before the reset-hook fix the tc1 /
tut1 runs panicked at frame 270 / 260 whenever the window got a WM_SIZE after the map load, on
master too: CreateVertexBuffer / CreateTexture D3DERR_INVALIDCALL.)

## Left

- `SWorld` 0x5d68e0 (an AI group answers an attack: support calls, help; 7.5 KB) is still a
  STUB_LOG; hit in 2 of 15 missions in 120 s.
- The productive building 0x54a660 (unit type 0x1a: only "multi barrack" / "multi factory", the
  multiplayer maps) and `SWorld::ComputeCamera` mode 1.
- Engine / renderer stubs hit in most missions (agent of src/3dengine): DrawLakes / DrawLines /
  DrawSea, SPRain / SPSnowfall / SPFlare, SModel +0xcc / +0xbc, SScene::ReplaceModel, the unit board
  decals and glows of SSingleUnit::UpdateVisuals; cut-scene subtitles / sound and the script
  `create` preprocessing (agent CS); the unit speech queue 0x5fff20.
- The sweep gives the player's units no orders and buys nothing, so combat of the player's own
  army, towing, the in-game save screen and mission ends by victory were not exercised.

## Files outside src/game / src/world

- `src/panzers/m5mission.cpp` (new), one hook line each in `src/panzers/superwindow.cpp`
  (LoadMainMenu, OnIdle) and in `InitCampaignMode` (campaign.cpp), `src/panzers/CMakeLists.txt`.
- `src/panzers/results.cpp` (PzPlayTriggerMusic), `src/panzers/tutorial_msg.cpp`
  (PzSpeechPlayNow), `src/panzers/market.cpp` (0x644fc0).
- `src/3dengine/pz/pzgepard.cpp` (the reset hooks) and `src/3dengine/pz/pzmodel.cpp` (0x6d7310).
- `src/tools/modelview/CMakeLists.txt` (animview builds boatanim.cpp).

## References the original could give (not run by this agent)

- German 3 (New Game, German, Normal, briefing, "Click to continue"): the HQ warehouse, tanks tab,
  before buying: which units are offered (ours: Panzer I 118, SdKfz 223 140, Panzer II 141;
  `scratchpad\m5ms\shots\g3market_022.png`).
- German 1 opening cut-scene: screenshots about 16 s, 20 s and 28 s after "Click to continue", to
  compare the eye camera with ours (`shots\g1cam_016/020/028.png`).
- Any mission with a support place near the start (e.g. Allied 2): a screenshot of the flag after
  the player's units capture it (the flag colour per race).

