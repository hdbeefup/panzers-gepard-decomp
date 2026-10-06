# M3 scope: from the main menu into a playable mission

Read-only scoping, 2026-10-06. Repo `menu-3d` at cc0c693 (worktree `agent-abddd25dd5f899b1e`), census
1518 `// PANZERS` markers. Ground truth: HD `PANZERS.exe` (md5 a35efd2e…, Ghidra copy in `m3scope/ghidra`).
All addresses are HD.

**Bottom line**
- The shortest faithful path is **Training Camp**. It is Training Camp → nation pick → map load → Headquarters (market) → mission → Esc → End Mission → main menu. You control units from second 0, and nothing on the path needs data that the scratch install lacks.
- The **Tutorial** has fewer screens, but its first trigger plays an in-game cut-scene from `cutscenes.pak`, which the scratch copies don't have. It also gates unit control behind 126 script triggers that use 28 action types, and 23 of those are not implemented.
- **The DynamoRIO coverage run worked** (portable DR 11.3.0-1 from GitHub, client `bbcount` rebuilt against it).
  - The work list for the Training Camp path is **820 functions (95.7k instructions)** that are not lifted.
  - On top of that, **376 already-lifted functions take branches the menu never ran**. 50 of them hold 93 `STUB_LOG` sites.
  - Two more items are outside the trace: scripted objectives / win-lose (tutorial, campaign) and cut-scenes. They are ≈250–350 more functions.
- **Gameplay can be verified with the original's own replay system.**
  - The `-packetrec` / `-packetplay <file>` switches record player orders as lockstep packets.
  - The packets carry a per-frame CRC check: `ProcessPacket` logs `Inconsistency in frame %d with player %d`.
  - The existing world-CRC tooling (0x56aa10) works in missions as well.
  - One open item: when I played back a replay of the original in the original, it diverged from frame 1, so start-state alignment must be solved first (M3-P0).
- **Honest estimate: ≈1,100–1,300 functions. M3a–d (Training Camp playable, back to menu) takes ≈3–4 days with 5–6 agents. M3e (objectives, tutorial, cut-scenes) takes another 1.5–2 days.**

## Files in this folder (`scratchpad/m3scope/`)

| File | What |
|---|---|
| `m3_coverage.tsv` | **The M3 work list.** Every function with hits after the idle-menu snapshot in the Training Camp run (2,623 rows).<br>Columns: per-phase hit deltas, class, area, proposed owner, repo status (`lifted` = has a `// PANZERS` marker; `stub` = cited in a `STUB_LOG`/`PzStub`), `container_instance` (SDArray/SHeapTRB accessor instances the repo inlines) and `worklist` (=1 for the 820). |
| `m3_lifted_newpaths.tsv` | 376 lifted functions that ran basic blocks in the mission that never ran in the menu (audit queue), with the `STUB_LOG` count inside each body and the repo file. |
| `m3_tutorial_only.tsv` | 70 functions the Tutorial run hit that the Training Camp run did not. |
| `cov_tc/`, `cov_tut/` | Raw bbcount dumps (block offset, count; image base 0x1e0000 → rebased to 0x400000). Training Camp has 10 snapshots; Tutorial has 5. |
| `m3analyze.py`, `m3classes.py`, `m3blocks.py`, `m3tut.py`, `m3final.py` | Analysis. Each reuses `covanalyze.py` + `map/` from M2-P0 (Ghidra function bodies, vtable slots, class labels). |
| `h/start.ps1`, `h/act.ps1` | Harness. It launches the scratch exe (optionally under DR) and drives it with **posted** messages: `sclick`, `srclick`, `sdrag`, `hold:Key,ms`, `ctrlkey`, `shot` (PrintWindow client area), `nudge`, `close`. Under DR, use the slow `s*` variants: fast clicks don't register at DR frame rates. |
| `tools/DynamoRIO-Windows-11.3.0-1/`, `tools/bbc/b/bbcount.dll` | Portable DR (scratch only, no PATH change) and the rebuilt client. |
| `data/tutorial_trig.txt`, `data/training_trig.txt` | `trigdec.py` decode of the two maps' TRIG chunks. |
| `data/m3rec1.rec` | 1 minute of Training Camp recorded by the original with `-packetrec` (50 KB). |
| `shots/` | Screenshots of every step: `dr_*` = training DR run, `tu_*` = tutorial DR run, `r_*`/`r2_*`/`tc_*` = native rehearsals, `rec_*`/`play_*` = replay test. |

---

## 1. Shortest path to "playing" (Q1)

All entries go through `SSuperWindow::OnAction` 0x659250. Each creates `SPanzersCampaign` (new 0xb8c, ctor 0x590ec0) into the global 0x929a0c. Then `SSuperWindow::LoadNextCampaignView` 0x658b10 switches on `MenuToLoad` (campaign +0xe0, getter 0x596600):
- 0 → `SBriefingMenu` (new 0x340, 0x632f40 + 0x634f20);
- 1 → `SMarket` (new 0x25b4, 0x63f2f0 + `SMarket::Create` 0x6407d0);
- 2 → `SGameView` (new 0x3e98, ctor 0x6181f0, `SGameView::Create` 0x619c90, then 0x6201c0);
- 3 → `SResultsMenu` (new 0x1228, 0x633540 + 0x6360f0);
- 4 → the MP menus 0x658a30 / 0x658230;
- anything else → `LoadMainMenu` 0x6583e0.

The common mission-start and mission-end steps are:
1. After the "LOADING… Click to continue" screen, a click goes to SGameView `OnMouseDown` 0x624a70. That handler sees +0x3890 set and sends action **0x47561 "Map loaded"**.
2. `OnAction` handles 0x47561 with 0x594d50. If `MenuToLoad == 2`, it calls 0x594e50, then GameView vtbl +0x6c, then **0x6281a0 (mission start)**. Mission start runs:
   - `CreateSubViewports` 0x61e500;
   - seeds taken from the CRT clock 0x7669f1 into +0x45c / +0x460;
   - `PlaceAllUnits` 0x571c70 and `PlaceUnits` 0x572ec0;
   - 0x5f5b50, then `SetRunning` 0x5802f0;
   - the packet rec/play hooks `StartPacketRecording` 0x5805c0 / `StartPacketPlayback` 0x580540;
   - the start-of-mission autosave, `SPanzersCampaign::SaveGameStartMission` 0x596e30 → `SaveGame` 0x5966a0 → `SUnit::Save` 0x5be320 → STrigger*::Save. It writes `SaveGames/TRNG-Start.save` or `SaveGames/TUT-Start.save`.
3. End of mission is either Esc (`SInGameMenu`) → End Mission → Yes, or the trigger action "End scenario in victory/defeat", which sends `GV_GAMEOVER`. Both lead to `OnAction` 0x47562 / 0x47563, which runs:
   - delete the view;
   - release the scene;
   - `SPanzersCampaign::LetMapDone` 0x594e00;
   - `LoadNextCampaignView`. That goes back to the main menu for training and tutorial, or to `SResultsMenu` in a campaign.

In the trace, End Mission also ran `SGameLogic::BackupCampaignUnits` 0x561110.

| Entry | Screens and systems on the path (HD) | Extra cost beyond the common game view | Verdict |
|---|---|---|---|
| **Training Camp** (0x4d4d4) | 1. `STrainingMenu` (new 0x288, 0x633b30 + Create 0x63a370): nation radio buttons, Start/Cancel.<br>2. Start (0x544d1): campaign +0x18 = nation, `InitTutorialMode` 0x594ab0 (`"maps/training.map"`, nation, **1500 prestige**; GameMode 5).<br>3. SGameView loads the map, then "Map loaded".<br>4. MenuToLoad ≠ 2, so **SMarket "Headquarters"**: buy units, 3D preview, "Start Mission" → "Are you sure?".<br>5. Mission start; play. The map has 210 placed units, AI groups (AIGP) and air support.<br>6. Esc → End Mission → main menu. | SMarket (47 functions, 11.1k insns in total; 32 functions hit) plus `STrainingMenu` (2). Its triggers are trivial: 2 triggers using action 0xe (end scenario). | **Chosen.** Control from the first frame. All data is present. It covers selection, orders, combat, death, AI and air support in 5 minutes. |
| **Tutorial** (0x4d4d3) | 1. `InitTutorialMode` 0x594ab0 (`"maps/tutorial.map"`, 1, 0).<br>2. LoadNextCampaignView case 2: straight to SGameView. There is no market.<br>3. Mission start.<br>4. T0 `Play cut-scene 'tutorial-01'` → 0x56f0d0 (SInGameAnimLogic 0x587d00..0x58d4e0) loads `cutscenes/tutorial-01/tutorial-01.4d`. | 126 triggers. Events 0,1,2,3,4,5,6,8,9; conditions 2,3,5,6,7,9; **28 action types**, of which the repo's `RunTriggers` implements 5 (1,2,3,4,0xa) → **23 missing** (0xd, 0x15–0x17, 0x1b, 0x23, 0x24, 0x26*, 0x27, 0x28, 0x29*, 0x2a–0x2d, 0x2f, 0x37, 0x38, 0x3a, 0x40–0x42, 0x49; * = implemented). Also needed: speech queue, subtitles (`SSubtitler`), echo/message boxes, guide-arrow effects, camera focus, the cut-scene player and cutscenes.pak. | **Second step (M3e).** On the scratch data it **panics at T0**: `SGameLogic::RunTriggers: Cannot open cutscenes/tutorial-01/tutorial-01`. The scratch copies have no `cutscenes.pak` / `cutscenes_patch.pak` (the GOG file list has them). With a stand-in .4d the tutorial ran, but the rifle squad stayed unselectable for 3+ minutes (cause not found). |
| **New Game → campaign 1** (0x4d4d1 + `SMainMenu` New Game) | 1. `SSingleMenu` (new 0x320, 0x633770 + 0x639790): campaign and difficulty (submenu at +0x1b8).<br>2. Action 0x53441: race +0x18, difficulty +0x1c, `InitCampaignMode` 0x592b20 (`missions.ini`, "German 1"/"Allied 1"/"Russian 1").<br>3. SBriefingMenu.<br>4. SMarket.<br>5. SGameView.<br>6. SResultsMenu. | Everything Training needs, plus:<br>- briefing and results menus;<br>- SCampaign flow (98 functions, 1 lifted; copy-adapt SWINE `campaign.cpp`);<br>- cut-scenes (all ger/su/us missions have `cutscenes/*` entries);<br>- objectives and victory/defeat;<br>- `BackupCampaignUnits`;<br>- full trigger coverage per mission. | M4 |
| **Skirmish** (Multiplayer 0x4d4d2 → 0x658a30) | 1. `SMultiPreMenu` → Skirmish.<br>2. `SSkirmishChatRoomMenu` (12) over `SChatRoomMenu` (35 functions, 29 KB).<br>3. Map pick (`multimaps`) and `COMPUTER` slots (`SKIRMISH_AI %d`).<br>4. SMarket.<br>5. Game through the SMulti lockstep path, with AI support calls ("Call AI (Parachute) support!"). | MP menu layer plus SMulti (see `MP_SCOPE.md`), the skirmish AI, and team/domination rules. | Last |

## 2. Coverage of the original (Q2)

**Run (Training Camp, `cov_tc/`).**
- Setup: scratch copy `run/PANZERS_orig.exe` (identical to `m3scope/PANZERS.exe`), `intro.bik` renamed away, windowed 1024×768, DR 11.3.0-1 with `bbcount`.
- Input: only posted messages.
- Snapshots: 00 idle menu · 01 market open · 02 army bought (2× Panzer III F, Panzer IV D, riflemen, MG) · 03 in game · 04–08 play · 09 back at the main menu.
- Play covered, about 6,140 logic ticks (≈5 game minutes):
  - single-click select;
  - drag-box attempt;
  - right-click move, on the ground and on the minimap;
  - minimap camera jump and arrow-key scrolling;
  - Ctrl+1 / 1 group keys;
  - Objectives screen;
  - paratroopers, artillery and recon air support;
  - Pause (Space) / Play;
  - behaviour buttons;
  - tank combat until the Panzer IV was destroyed;
  - Esc → End Mission → Yes → main menu.

**Second run (Tutorial, `cov_tut/`).** It ran with stand-in cut-scene files copied in the scratch run folder (`run/cutscenes/tutorial-0x/*.4d` = `objects/25 cutscenes/kalyha1.4d`). That gives the tutorial-specific start only.

**Results** (functions with hits after the idle-menu snapshot; "lifted" = has a `// PANZERS` marker):

| Area | ran | lifted | stub-cited | container instances | **work list** |
|---|---:|---:|---:|---:|---:|
| world/game 0x546000–0x60a000 | 1,038 | 562 | 37 | 81 | **395** (48.6k insns) |
| shell/UI 0x540000–0x546000, 0x60a000–0x660000 | 264 | 42 | 2 | 12 | **205** (35.6k) |
| engine 0x660000–0x700000 | 893 | 405 | 63 | 35 | **155** first run after the menu (8.9k). The other 298 unmarked ones also ran in the menu, so they are SWINE-shared. |
| libs <0x540000 | 104 | 15 | 0 | 2 | **65** first run after the menu (2.6k): SButton, SMessageBox, SRadioButton, SEditBox… mostly SWINE-shared widgets; check each |
| CRT | 314 | – | – | – | linked, ignored |
| **Total work list** | | | | | **820 functions, 95.7k insns** (262 ≤20 insns, 368 ≤100, 153 ≤500, 37 >500) |

**By proposed owner** (column `m3owner`):

| Owner | Count | Classes |
|---|---|---|
| **C** units/combat | 170 fn, 19.7k | SUnit (TakeDamage 0x5c4080, EC_Die 0x5b8b10, EC_AttackMove 0x5b8660, Save 0x5be320, 0x5b6040, 0x5c51d0), SProjectileUnit/Driver/Animation, SWaster*, SFlyingUnit/Driver/Animation (air support), SPanzersParachuteDriver, squads, SBuildingUnit, SGunner rest |
| **E** engine | 155 fn, 8.9k | SScene 23, SViewport 13 (9 stubbed), SGepard 11, SBoard 11, SModel 9 (7 stubbed), STrailEffect, SDecalEffect, SLiteEffect, SCameraShake, particles, SBitmap mips |
| **L** logic/campaign | 116 fn, 16.1k | `SGameLogic::ProcessPacket` 0x5739b8, 0x56b3e0, `PlaceAllUnits`, `PlaceUnits`, `BackupCampaignUnits`, the packet builders, SCampaign 37 / SPanzersCampaign 3, STrigger*::Save |
| **M** menus/market/widgets | 149 fn, 17.4k | SMarket + list box + view + buttons ≈32 (10k), STrainingMenu, SInGameMenu, SHelpMenu, SInGameBriefingMenu (objectives), SSaveMenu, SStreamBuffer 24 (save stream), SWidget/SWindow extras |
| **V** view/input/HUD | 56 fn, 18.2k | SGameView 33 (15.2k), SUnitButton 7, SCommandButton 4, SMinimap 6, STextBox 6 |
| **W** SWorld | 109 fn, 12.8k | AI 0x5f5c70, selection/pick 0x5fc050 / 0x5fc5b0 / 0x5fc860 / 0x5fcb10 / 0x5fd630, 0x5ea910, ShowUnitRange 0x5fee00, IncreaseUnitXP 0x5ec840, InitFlyingFox 0x5ecdf0, LoadWires 0x5f3d60, FixBridges 0x5e65f0, UpdateWaterMap 0x608600, SGameWorld::InitCameraSpline 0x609760 |
| **X** libs | 65 fn | see above |

**By phase:**

| Owner | menu → market | mission start | play | end → menu |
|---|---:|---:|---:|---:|
| C | 28 | 70 | 67 | 5 |
| E | 58 | 44 | 52 | 1 |
| L | 55 | 35 | 17 | 9 |
| M | 113 | 20 | 6 | 10 |
| V | 28 | 4 | 19 | 5 |
| W | 59 | 37 | 7 | 6 |

"Mission start" includes the first ≈25 s, when the pre-placed units are already fighting.

**Audit queue, not just new code:** `m3_lifted_newpaths.tsv` lists 376 lifted functions that took never-run blocks.
- The top ones: `SGunner::ServerRefresh` 0x584d00 (301 of 331 blocks new, 2 `STUB_LOG`), `SWorld::RemoveRiver` 0x5d5510, 0x5aaaa0 (unitai), `SWalkerAnimation::UpdateModel` 0x5ce2a0, `SGameLogic::Refresh` 0x576d80, `Ghost_NextStep` 0x553c00, `FindEmptySpace` 0x5e58d0, `ServerRefreshMedic` 0x5bfa50, 0x5b37d0 (5 stubs).
- 50 of them contain **93 `STUB_LOG` sites** that the mission reaches.
- `RunTriggers` 0x579ab0 implements 15 of 76 actions (training needs 0xe; tutorial 23 more).

**Tutorial-only** (`m3_tutorial_only.tsv`): 70 functions. They are the cut-scene scene setup (SScene 19, 7.4k insns), SWorld 15, SGameLogic 10 (trigger actions), SSubtitler, SSaveMenu, SBoard 2. The real cut-scene player was not exercised because the stand-in model ended at once.

Things the trace does **not** cover:
- briefing / results screens;
- victory and defeat (trigger 0xd/0xe);
- loading a save;
- the cut-scene player with real data;
- buildings entered by infantry, towing, repair, resupply, mines, bridges, weather changes.

Expect another 20–40% of branches to show up during testing.

## 3. In-game systems (Q3)

Sizes are work-list functions from the trace; static figures are marked "static".

| System | HD entry points | Size |
|---|---|---|
| **Game view, camera, scrolling** | `SGameView` vtables 0x80331c (31 slots) / 0x80339c (5).<br>Handlers:<br>- ctor 0x6181f0, `Create` 0x619c90 (4,162 insns), map load 0x6201c0;<br>- idle 0x628430 (4,139, slot 30), draw/slot 17 0x6216b0;<br>- OnKeyDown 0x622f50 (28 cases: scroll, groups, pause), mouse move/camera drag 0x620bc0.<br>Camera:<br>- modes 2/3/7 warp the cursor with `SetCursorPos` 0x53a310 (avoid them in background tests);<br>- SWorld camera 0x5f4dc0 / 0x5f8380;<br>- intro camera `SGameWorld::InitCameraSpline` 0x609760;<br>- `CreateSubViewports` 0x61e500, `SWorld::ComputeCamera` (M1, modes 1/2 not lifted). | ≈35 fn, 17k insns (V) plus ≈5 W. SGameView is 84 functions / 15.7k static, none lifted. |
| **Selection and orders** | Mouse:<br>- OnMouseDown 0x624a70 (mode +0x478, press pos +0x47c/+0x480, double-click time +0x4a0);<br>- OnMouseUp 0x6251f0;<br>- box select 0x5fc5b0, pick 0x5fc050 (filters on player == World+0x16c, ally, selectable +0x112), double-click same type 0x5fcd10, 0x5fd630, 0x5ddb60.<br>Orders:<br>- dispatcher 0x61e740 (880 insns);<br>- **≈51 packet builders 0x575670–0x5763a0** (opcode byte + args via 0x65daf0 / 0x65dc20) → `CheckSendQSize` 0x562fb0 → lockstep queue → `SGameLogic::ProcessPacket` 0x5739b8 (53 cases) at the stamped frame;<br>- command buttons (stop / behaviour / guard), Ctrl+n groups. | ≈20 V + 51 builders + ProcessPacket (L) ≈ 75 fn |
| **HUD and in-game UI** | Unit panel:<br>- `SUnitButton` (0x626c40, 1,019 insns);<br>- `SSpecInfoWidget` (two in SGameView at +0x2614 / +0x2678);<br>- `SCommandButton` (0x6281a0 region), the five air-support buttons.<br>Minimap: `SMinimap` (vtable 31 slots; RMB on the minimap = move order).<br>Text: tooltips (`STextBox`), on-screen objective text.<br>Top bar: Menu / Objectives / clock / pause / speed.<br>In-game menu: `SInGameMenu` (Save, Load, Options, Help, Objectives, Restart, End, Resume), `SHelpMenu`, `SInGameBriefingMenu` (objectives), `SSaveMenu`, message boxes (`SMessageBox`). | ≈45 fn (V 21 + M ≈24), static in-game menus ≈216 / 27.7k (36 already lifted from the shell) |
| **Combat** | Units:<br>- `SUnit::TakeDamage` 0x5c4080 (stub-cited), `EC_Die`, `EC_AttackMove`, 0x5b6040 / 0x5c51d0 / 0x5b53d0 (targeting / damage helpers);<br>- `SGunner` (only 7 left; ServerRefresh already lifted with stubs);<br>- `SProjectileUnit` 0x5a39f0 (620) + driver 0x55a740 + animation 0x5cc480;<br>- `SWasterUnit` (wrecks), `IncreaseUnitXP` 0x5ec840.<br>Air support: `SFlyingUnit::ServerRefresh` 0x55d0c0 (1,165), `SFlyingAnimation` 0x5cb750, `SPanzersParachuteDriver`.<br>Engine effects (E): trails, decals, lights, camera shake, particles.<br>Visual only: `ShowUnitRange` 0x5fee00. | ≈170 C + ≈60 E |
| **AI** | 0x5f5c70 (1,010 insns; AI groups, AIGP chunk; ran 232× in play), "Call AI (…) support!" calls. The menu never ran it; training needs it; tutorial T15 moves an AI wave via triggers. | ≈10–20 fn (W) |
| **Objectives and win/lose** | Trigger actions:<br>- 0xd/0xe end scenario (victory/defeat);<br>- show / complete / fail objective;<br>- hide objective target.<br>Flow: GV_GAMEOVER 0x47562 → `LetMapDone` 0x594e00; `BackupCampaignUnits` 0x561110 (1,249); results `SResultsMenu` 0x633540 (campaign only). | **Not in trace.** Static ≈30–50 fn |
| **Briefing, cut-scene, speech** | `SBriefingMenu` 0x632f40 (campaign).<br>Cut-scene:<br>- start 0x56f0d0: own scene, `.4d` with camera node (+0x108), length (+0x80), `.sub`, mp3;<br>- `SInGameAnimLogic` 0x587d00–0x58d4e0: 83 fn / 6.4k, 7 lifted;<br>- needs `cutscenes.pak`.<br>Speech: `SWorld::UpdateSpeech` 0x607f50 (lifted), `SSubtitler`, trigger actions 0x40/0x41/0x42 (echo, message, speech). | Tutorial-only 70 + SInGameAnimLogic ≈76 + ≈20 trigger cases ≈ 170 (static) |
| **Save/load** | **On the path**: the autosave runs at every mission start (`SaveGameStartMission` 0x596e30, `SaveGame` 0x5966a0, `SUnit::Save` 0x5be320 (887), STrigger*::Save, `SStreamBuffer` 24 fn).<br>Load (`LoadGame` 0x594f70, `SUnit::Load` 0x5bbd30) did not run. Options:<br>- stub the autosave behind a logged no-op in M3a (it does not touch the world CRC);<br>- or lift it, because a saved start state is useful for testing (see §4). | ≈35 fn save; load is static, ≈40 |

## 4. Plan (Q4)

### M3-P0: one agent, about half a day; it blocks the rest

**Interface headers** (HD sizes; slots in vtable order with `RET n`; `Slot_XX` placeholders; logged stubs):

| Header | Contents |
|---|---|
| `src/panzers/gameview.h` | `SGameView` 0x3e98, vtables 0x80331c / 0x80339c. Fields:<br>- +0x468 viewport, +0x478 mouse mode, +0x47c / +0x480 press pos, +0x49c drag flag;<br>- +0x4a0 / +0x4a4 click times, +0x4a8 double-click pick, +0x3884 command mode;<br>- +0x3890 loading-screen flag, +0x3894 / +0x3898 sound handles, +0x389c modal;<br>- +0x3e40 logic/world, +0x3e80 view state (2 = cut-scene), +0x3e74 / +0x3e78 widget list, +0x45c / +0x460 seeds;<br>- SSpecInfoWidget members. |
| `src/game/campaign.h` | `SPanzersCampaign` 0xb8c:<br>- +0x10 GameMode (5 tutorial/training, 3 campaign), +0x18 race, +0x1c difficulty;<br>- +0x28 / +0x38 prestige, +0xe0 MenuToLoad, +0xe4, +0x100, +0x11c;<br>- +0x12c / +0x130 replay name / flag;<br>- `missions.ini` properties at +0 / +4. |
| `src/game/packets.h` | Opcode enum from the 51 builders and the `ProcessPacket` 0x5739b8 switch; builder signatures; send queue and per-frame CRC record. |
| `src/world/selection.h` | pick / box / double-click / selection set; World+0x16c local player; unit +0x112 selectable, +0x150 hidden. |
| `src/panzers/hud.h` | SUnitButton, SCommandButton, SSpecInfoWidget, SMinimap, support buttons. |
| `src/panzers/market.h` | SMarket 0x25b4 and its list box / view / buttons. |

Also add the missing combat slots to `src/game/iunit.h` (TakeDamage, EC_*).

**Replay oracle validation.** Make the original play back its own `.rec` without "Inconsistency" lines. The likely causes of my frame-1 divergence:
- the time seed 0x7669f1 → +0x45c / +0x460;
- the replay being started through the Training Camp menu, which builds a fresh campaign. The `.rec` starts with a `SAVE` chunk holding the campaign and army; the market was skipped on playback.

Find the intended start path: `SPanzersCampaign::StartReplay` 0x597510 is reached from `OnAction` (0x659250, around line 590); `Replays/*.rec` is listed by `SLoadMenu::LoadReplayNames`.

**Data:** ask the user to add `cutscenes.pak` and `cutscenes_patch.pak` to the scratch test install, as hardlinks made by the user. Without them the Tutorial and every campaign mission panic.

### Phases (each ends with a run of ours next to the original)

| Phase | Content | Done when |
|---|---|---|
| **M3a** load a mission, game view, camera | Training Camp menu, campaign ctor and `InitTutorialMode`, `LoadNextCampaignView`, SGameView ctor / Create / load screen / "Map loaded", mission start 0x6281a0, `PlaceAllUnits` / `PlaceUnits`, SWorld load extras (wires, flying fox, FixBridges, water map), camera keys, minimap jump, autosave (stub or lift), market (can be a temporary "buy nothing, start" path behind a flag, but the faithful build needs SMarket) | Training map renders with the HUD frame; CRC equals the original for the first N ticks with no input (pre-placed units already fight, so this needs combat. Do the no-input check on a quiet test map or the tutorial first, or accept CRC equality only up to the first shot until M3c) |
| **M3b** selection and orders | OnMouseDown / Up / Move, pick / box / double-click, 0x61e740, packet builders, ProcessPacket, command buttons, groups | Replayed packet stream: same CRC as the original until the first combat event |
| **M3c** combat and AI | TakeDamage / Die / wrecks, projectiles, remaining gunner branches (the 93 stub sites), air support, AI 0x5f5c70, effects | Same CRC for a 5-minute replay with combat |
| **M3d** HUD and in-game menu | unit panel, spec info, command buttons, minimap, tooltips, Esc menu, objectives / help screens | Screens match the original (screenshot pairs) |
| **M3e** end of mission, scripts | End Mission → `BackupCampaignUnits` → menu, victory/defeat (0xd/0xe), objectives; then Tutorial: 23 trigger actions, conditions 7/9, events 1/3–6/8/9, speech / subtitles / message boxes, cut-scene player | Training round trip ×2 (teardown and reload twice); Tutorial to objective 6 |

### Parallel split (after P0; one worktree each; merge the integration branch before finishing)

| Agent | Owns (files) | Work-list share |
|---|---|---|
| **V** view, input, orders UI | `src/panzers/gameview*.cpp`, `src/world/selection.cpp` | V 56 + W selection ≈10 |
| **H** HUD and menus | `src/panzers/hud*.cpp`, `minimap.cpp`, `ingamemenu.cpp`, `market*.cpp`, `trainingmenu.cpp`, widget gaps | M 149 (minus SStreamBuffer) |
| **L** logic, campaign, packets, AI | `src/game/campaign*.cpp` (copy-adapt SWINE `world/campaign.cpp` as M2_SCOPE §4.3 says), `src/game/packets.cpp`, `gamelogic_mission.cpp`, `triggers.cpp` (new cases), `src/world/ai.cpp`, `src/world/worldload_extra.cpp`, save stream | L 116 + W ≈95 |
| **C** combat units | `src/game/combat.cpp`, `projectile*.cpp`, `waster*.cpp`, `flying*.cpp`, `parachute*.cpp`; existing `unit*.cpp` / `gunner.cpp` for the 93 stub sites (coordinate with L on `unit.cpp`) | C 170 + the audit queue |
| **E** engine | `src/3dengine/pz/*` effects, scene/viewport stubs, SModel stubs, SBitmap mips; check the 65 lib functions against the SWINE-shared code | E 155 + X 65 |
| **I** integration | merges, census tripwire, replay / CRC runs, screenshot pairs | – |

The coordinator relays interface needs (for example, V needs `packets.h` from L, and C needs `TakeDamage` events for trigger event 1/4 dispatch from L).

### Verification (Q4: can the CRC verify gameplay with scripted input?)

**Yes.** In missions the HD exe still computes the world CRC every tick in `BeginFrame` 0x571840 → 0x56aa10. Player input does not change the world directly; it becomes lockstep packets that `ProcessPacket` applies at a stamped frame. Drive identical input at three levels:

1. **Packet level (preferred, tick-exact).**
   - `-packetrec <file>` in the original writes `SAVE` (campaign / army state) followed by per-frame packets and a frame check.
   - `-packetplay <file>` replays it, and `ProcessPacket` itself logs `Inconsistency in frame %d with player %d` (0x5737c0 / 0x5739b8).
   - The repo already parses both switches (`src/panzers/settings.cpp`).
   - Workflow:
     1. record a session in the original;
     2. play it in ours;
     3. the first "Inconsistency" frame is the next bug;
     4. confirm with the DR `m2crc` per-unit dump (`m2p0/tools/m2cov/m2crc.c`, rebuild it against the new DR) and the M2 `udiff` / `firstdiv` tools.
   - Blocker: P0 must first make the original's own playback consistent (see above).
2. **Window-event level.**
   - `-eventrec` / `-eventplay` (SWINE-shared `SWindow::RecordEvents` / `PlaybackEvents`, recorded per event frame) replays the same mouse and keys into both exes.
   - Frame-based, not tick-based, so use it for UI and screen regression rather than for CRC.
3. **Start state.**
   - The mission-start autosave (`SaveGames/TRNG-Start.save`) gives a fixed starting state through Load Game. That is the cheapest repeatable start once load is lifted.
4. **Coverage gate.**
   - Re-run `bbcount` on ours: the counts per lifted function let you compare block-hit sets with the original for the same replay.
   - Functions with blocks never run are the audit queue.
5. **Maths.** Unicorn equivalence for damage, ballistic and armour formulas (M2 practice).

## 5. Risks and effort (Q5)

| # | Risk | Detail |
|---|---|---|
| 1 | **CRT `rand()` shared state** | M2 already found `SUnit +0xdc` reading CRT `rand()`. Combat (shot spread, damage, effects) will draw much more. If render-side effects call the same CRT rand, the logic CRC depends on the frame rate or on effect code. The CRT rand state and every caller must be bit-identical. |
| 2 | **The replay oracle is not yet proven** | Frame-1 inconsistency in my one try. |
| 3 | **Missing data** | `cutscenes.pak` is absent in the scratch installs. The tutorial path is unverified past T0, and the squad selection gate is unexplained. |
| 4 | **UI fidelity** | SGameView `Create` / idle are 4k-insn functions; SMarket `Create` / `LoadUnitInfo` are 3.1k / 2.9k. They are checked by screenshots only, with no oracle. |
| 5 | **Hidden branches** | The 93 `STUB_LOG` sites in lifted code. Untraced features (buildings with occupants, towing, repair, supply, mines, bridges, weather) will add an estimated 20–40%. |
| 6 | **Test harness limits** | Under DR, fast posted clicks don't register (use the slow steps); DR runs hung on exit twice (killed). One DR run hit 16 D3D resets; Space toggled pause. Camera modes 2/3/7 call `SetCursorPos` on the real cursor; the harness must never use middle- or right-drag. `ClipCursor` is called only in full-screen mode (`SViewport::SetFullScreenMode` 0x68ca00), so windowed runs are safe. |
| 7 | **Class labels** | About 40% of world rows are range-inherited (`~` in the TSV). Some UI rows carry wrong MP labels: the SGameView ctor 0x6181f0 shows as `~SGameSpyTitleRoom`, the SMarket ctor 0x63f2f0 as `~SRankedGamingRegisterMenu`. |

**Effort, revised against M2.** M2's list was 482 functions / 74k insns and took 5 agents plus integration about 1.5 days. The census went from 1,357 to 1,488 in M2-I alone, and the agents lifted whole classes beyond the list.

| Work | Functions | Basis |
|---|---:|---|
| Training Camp path work list | 820 (95.7k insns) | coverage |
| of which probably SWINE-shared (libs, part of engine) | −60 … −120 | check per function |
| Audit / branch completion in already-lifted functions | ≈376 functions to re-check, ≈50 with real stubs | coverage |
| Expected extra branches found in testing | +150 … +250 | judgement (20–40%) |
| **M3a–d subtotal** | **≈900–1,000 lifts + audit** | |
| M3e (objectives, tutorial triggers, cut-scenes, speech, results / briefing for the campaign) | ≈250–350 | static |
| **Total M3** | **≈1,100–1,300** | |

At M2's rate (≈50k insns/day for the team), M3a–d is ≈2 days of lifting. UI work without an oracle is slower (×1.3–1.5), and determinism debugging in combat adds about a day. So: **≈3–4 days for M3a–d with 5–6 agents plus integration, and +1.5–2 days for M3e.**

M2_SCOPE §3.2's 2,450–2,850 for the whole single-player game still holds. After M3, about 1,200–1,500 remain: the full campaign, save/load, all 76 trigger actions, MP-free polish.

## Not determined / not verified

- Why the original's own `-packetplay` diverged from frame 1 (seed vs start path), and how to start a replay through `StartReplay`.
- Why the tutorial rifle squad stayed unselectable with a stand-in cut-scene. Does the real cut-scene end restore `+0x112`?
- Whether the 65 lib and 155 engine functions are already present as SWINE-shared code (needs an address map from SWINE HD to Panzers HD).
- Drag-box selection was attempted but never confirmed working, either natively or under DR.
- Coverage covers one 5-minute Training Camp session and a 1.5-minute Tutorial start. There is no second run, so run-to-run variance of the function set is unknown.
- `cutscenes.pak` content was not inspected (the user's install was not touched).

Scratch side effects, all inside `m3scope/run/`:
- `PANZERS_tut.exe` (copy of the original);
- `cutscenes/` stand-ins;
- `SaveGames/` (autosaves);
- `Replays/`;
- `m3rec1.rec`;
- logs.

Processes: I killed only my own PIDs. PID 21644 (my first DR run) is stuck terminating, and Windows reports "no running instance". Other PANZERS processes on the machine were not mine and were left alone.
