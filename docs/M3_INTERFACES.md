# M3 Training Camp: interfaces, switch and ownership

This is the contract between the six agents that take the game from the main menu into a playable
Training Camp mission and back: nation pick -> Headquarters (market) -> mission -> play -> Esc ->
End Mission -> main menu.

Read these first:
- `docs/re/M3_SCOPE.md`: the entry paths, the coverage summary, the systems.
- `docs/M3_REPLAY.md`: the replay oracle (the original replays its own recording deterministically).
- The work lists: `docs/re/m3_coverage.tsv` (column `m3agent`, rows with `worklist` = 1) and
  `docs/re/m3_lifted_newpaths.tsv` (column `m3agent`; 376 lifted functions that take new branches,
  93 `STUB_LOG` sites).
- `docs/M2_INTERFACES.md` (the rules below are the same) and `docs/MENU3D_INTERFACES.md`.

All addresses are HD `PANZERS.exe` addresses.

## 1. Quick start

| What | How |
|---|---|
| Default build and run | `panzers.exe -nointro`: unchanged (3D menu + M2). Training Camp is the logged stub "OnAction action 0x4D4D4 ignored". |
| M3 path | `panzers.exe -nointro -m3` (switch and trace) or `PZ_M3=1`. `-m3` is removed from argv before SSettings reads the command line. |
| Trace | `PZ_M3_TRACE=0/1`. Each call site logs `PZM3 #<n>: <Class::Name (0xADDR)>` on calls 1..4 and every 500th. Each stub also logs `STUB: <name> called` once. |
| No map load | `PZ_M3_LOADMAP=0`: SGameView::LoadMap skips the world (UI work on the screens only). |
| World CRC | `PZ_M2_CRC=1` logs `PZM2 CRC <frame> <crc> <seed> <units>` for the mission logic too. |
| Reference data | Scratchpad `m3ref\` (README): `tc1.rec`, `tc1_crc_ref.txt`, screenshots. **Do not run the original.** If you need a new reference, ask the coordinator. |

Driving our build (posted messages, 1024x768 client): Training Camp (843,418); the nation dialog
stand-in: German (512,166), Russian (512,214), Allied (512,263), Start (512,363), Cancel (512,508);
loading screen: click anywhere; market stand-in: Start Mission (512,166), Cancel (512,508); in game:
Esc; in-game menu: End Mission (512,460), Resume (512,508). The stand-in screens move to the HD
layout as H and F lift them; update this table when they do.

Log of a full `-m3` round trip (2026-10-06, M3-P0 skeleton; trimmed):
```
PZM3 #1: SPanzersCampaign::SPanzersCampaign (0x590ec0)
PZM3 #1: SCampaign::InitTutorialMode (0x594ab0)
PZM3: Training Camp, nation 0
PZM3: LoadNextCampaignView MenuToLoad 2
PZM3 #1: SGameView::LoadMap (0x6201c0)
Loading map: maps/training.map
PZ3D world totals (SGameView::LoadMap): 17 chunks + 13 sub chunks; doodads 1675 ...; unit defs 78, units created 78 (346 incl. stored/members) ...
PZM3 #1: SCampaign::OnMapLoaded (0x594d50)
PZM3: LoadNextCampaignView MenuToLoad 1
STUB: SMarket::Create (0x6407d0) called (not implemented)
STUB: SGameView::MissionStart (0x6281a0) called (not implemented)
STUB: SPanzersCampaign::SaveGameStartMission (0x596e30) called (not implemented)
<1> STUB: SGameLogic::RunTriggers action 0xe (0x579ab0) not implemented
PZM3: End Mission (confirm box not lifted) -> GV_GAMEOVER
GV_ ... SSuperWindow::OnAction()
PZM3 #1: SCampaign::LetMapDone (0x594e00)
PZM3: LoadNextCampaignView MenuToLoad 5
SSuperWindow::LoadMainMenu: releasing scene
```
Two round trips in one session, then Exit: no crash, clean shutdown. The 346 units at map load
match the original's count before the army is placed (`m3ref\tc1_crc_mission.txt`, first line).

## 2. Interfaces

Declared in HD vtable order where the class has a vtable; slot comments give the offset, the HD
implementation, the argument dword count (`RET n`) and `[m3: ...]` = the phases of the Training Camp
coverage trace that ran it (mkt = menu to market, start = mission start, play, end = end to menu).
Only slots with evidence are named; `(name guessed)` marks names from one reader or caller.

| Header | Classes / contents | HD | Size | Skeleton | Owner of names / bodies |
|---|---|---|---|---|---|
| `src/game/m3common.h` | HD sizes, `g_M3` switch, `PZ_M3_TRACE` | | | `m3trace.cpp` | P0 |
| `src/game/campaign.h` | `SPanzersCampaign` (no vtable), game modes, MenuToLoad, save / load / replay entry points | ctor 0x590ec0, InitTutorialMode 0x594ab0, LetMapDone 0x594e00, SaveGameStartMission 0x596e30, StartReplay 0x597510 | 0xb8c (asserted) | `campaign.cpp` (9 small functions lifted) | F |
| `src/game/packets.h` | opcode enum (53 ProcessPacket cases), 45 builders, frame / record layout | ProcessPacket 0x5737c0 (switch 0x5739b8), builders 0x575610..0x576430, WriteSelectedUnits 0x576130 | | `packets.cpp` | O |
| `src/world/selection.h` + `world.h` block "M3" | selected bit (unit +0x104), pick / box / double-click, ShowUnitRange, GetCameraState; AI entry, map-load extras | 0x5fc050, 0x5fc5b0, 0x5fcd10, ... ; 0x5e6a70 (lifted); RefreshAI 0x5f5c70 | | `selection.cpp` (O), `ai.cpp` (C: RefreshAI; F: the rest) | O, C, F |
| `src/panzers/gameview.h` | `SGameView` (SDXWidget + `SIGameViewCallback` + HD fields from +0x5c) | vftable 0x80331c (31), callback vftable 0x80339c (5) at +0x58; ctor 0x6181f0, Create 0x619c90, LoadMap 0x6201c0, MissionStart 0x6281a0, Update 0x628430 | 0x3e98 (data block asserted) | `gameview.cpp` | V (view, Create, Update, camera), O (input, 0x61e740), F (LoadMap, MissionStart) |
| `src/panzers/hud.h` | `SSpecInfoWidget`, `SUnitButton`, `SHeroUnitButton`, `SCommandButton`, `SMinimap` | 0x803110, 0x803214, 0x803190, 0x803298, 0x808f70 | | `hud.cpp` | H |
| `src/panzers/ingamemenu.h` | `SInGameMenu` (+ action codes 0x494d1..0x494d9), `SHelpMenu`, `SInGameBriefingMenu`, `SSaveMenu` | 0x805e04 (OnAction 0x631dd0 lifted), 0x805f84, 0x806004, 0x805e84 | | `ingamemenu.cpp` (SInGameMenu stand-in) | H |
| `src/panzers/market.h` | `SMarket` + the list box / slot / category / view / vehicle-menu vtables | 0x808638, ctor 0x63f2f0, Create 0x6407d0, LoadUnitInfo 0x6450c0 | 0x25b4 | `market.cpp` (stand-in) | H |
| `src/panzers/trainingmenu.h` | `STrainingMenu` | 0x806f44, ctor 0x633b30, Create 0x63a370, OnAction 0x63d410 (lifted) | 0x288 | `trainingmenu.cpp` (stand-in) | F |
| `src/game/iunit.h` | combat slots named: +0x48 OnDriverReachedTarget, +0x94 **TakeDamage** 0x5c4080 (7), +0xa4 EC_Default, +0xcc EC_AttackMove, +0xd4 EC_AssaultBuilding, +0xe8 EC_Attack, +0x124 **EC_Die** 0x5b8b10 | SUnit 0x7fcc24 | | `unit.cpp` | C |
| `src/game/unit.h` block "M3" | `SUnit::Save` 0x5be320, `SUnit::Load` 0x5bbd30 | | | `unitsave.cpp` | F |
| `src/panzers/superwindow_m3.cpp` | the M3 OnAction cases and LoadNextCampaignView 0x658b10 | | | | F |

Widgets: as for the existing Panzers widgets (superwindow.h, mainmenu.h), the HD SWidget layout
differs from the SWINE classes in `src/window`, so the M3 widgets derive from the SWINE SDXWidget
(or SRightMenu) for behaviour and keep HD data in separate blocks (SGameViewData asserts its HD
offsets). Do not assert whole-object HD sizes on them (CLAUDE.md "Layouts").

## 3. The flow and the wiring

`SSuperWindow::OnAction` (superwindow.cpp) calls `SuperWindowM3Action` first when `g_M3.Enabled`;
otherwise nothing changed. The M3 cases (HD 0x659250):

| Action | Sender (HD) | M3 path |
|---|---|---|
| 0x4d4d4 | SMainMenu Training Camp | new STrainingMenu into +0x118, Create, focus (HD also makes it modal: 0x5435b0 / 0x544fe0) |
| 0x544d1 / 0x544d2 | STrainingMenu::OnAction 0x63d410 | Start: new SPanzersCampaign, Race = Nation (German 0, Allied 1, Russian 2), Difficulty 0, InitTutorialMode("maps/training.map", race, 1500), delete the training and main menus, LoadNextCampaignView, World+0x174 = race. Cancel: delete the dialog. |
| (MenuToLoad 2) | LoadNextCampaignView 0x658b10 case 2 | UnloadMenuBackground, new SGameView (+0xe4), Create, LoadMap: training.map into a new SWorld, SGameLogic, the "Click to continue" screen |
| 0x47561 | SGameView OnMouseDown 0x624a70 / OnKeyDown 0x622f50 on the loading screen | OnMapLoaded 0x594d50: prestige != 0 -> MenuToLoad 1 -> market; else SetMenuGameView + MissionStart |
| (MenuToLoad 1) | 0x658b10 case 1 | UnloadMenuBackground; **World 0x929a50, SGameLogic 0x8f2078 and Scene 0x929a54 are moved to 0x929f18 / 0x929f1c / 0x929f20 and nulled** (the market's 3D preview has its own scene); new SMarket (+0x110), Create |
| 0x4d542 / 0x4d541 | SMarket::OnAction 0x647d00 | Start: ReleaseMultiView 0x65b8c0 (deletes the market, moves the world back), SetMenuGameView 0x594e50, view +0x6c SetVisible(1), MissionStart 0x6281a0. Cancel: ReleaseMultiView, delete the view, LoadMainMenu. |
| 0x494d1..0x494d9 | SInGameMenu::OnAction 0x631dd0 | to SGameView::OnAction 0x6216b0: End (0x494d8) -> HD "End Mission / Are you sure?" -> 0x47562; Resume closes the menu |
| 0x47562 / 0x47563 | SGameView 0x6216b0 (End Mission), 0x622f50, Update 0x628430 (trigger "end scenario") | delete the view, release the window scene, LetMapDone (Training: MenuToLoad 5), LoadNextCampaignView -> default -> LoadMainMenu (HD's LoadMainMenu deletes the campaign; the M3 path does it there) |

Mission start 0x6281a0 in HD order (agent F): Concert +0x80(1); window scene = g_Scene;
CreateSubViewports 0x61e500; release sounds +0x3898 / +0x3894; clocks +0x45c / +0x460; PlaceAllUnits
0x571c70; SWorld 0x5f5b50; SetRunning(1) unless campaign +0xe4; 0x57f970(0x64df00()); -packetrec
0x5805c0 / -packetplay 0x580540; InitCameraSpline 0x609760 (-csplay file); SetPanelMode(0);
SaveGameStartMission("Start") unless multiplayer (DAT_008f1a74); -skipframes Refresh loop; one
Refresh.

SGameView::LoadMap 0x6201c0 in HD order: 0x61f460(0); map name (GetMapName 0x592040); "Loading
map: %s"; new SWorld; ShowLoadingIcon(+0x48); viewport +0x50(0, 0); SWorld::LoadMap; Initialize;
0x5e2d70; 0x5debb0; 0x607ad0; mission props 0x593740 when World+0x7520; LoadObjectives 0x593ba0;
0x594140; cursor; UpdateWaterMap 0x608600; FixBridges 0x5e65f0; new SGameLogic(0, +0x828, this
+0x58); 0x57fac0(+0x4b8, 0x13c, 7, 7); 0x56e8e0; -script / -cutscene; HideLoadingIcon; either
MissionStart (campaign +0xdc == 0, -packetplay, no market) or the loading screen (+0x3890);
0x626290; World+0x73ac = 0x64e2b0().

The skeleton's SGameView::Update runs the SSuperWindow::OnIdle menu-world tick (20 Hz Refresh,
camera, interpolation) so the map renders and the M2 logic runs; V replaces it with 0x628430.

## 4. Ownership

| Agent | Owns (writes) | Work list (`m3agent`) |
|---|---|---|
| **V** game view + camera | `src/panzers/gameview.cpp` (ctor, Create 0x619c90, Update 0x628430, draw, SetPanelMode, CreateSubViewports, the callback slots), new `gameview_*.cpp` for those; SWorld camera rows (`world.cpp` camera functions, 0x5f4dc0, 0x5f8380, InitCameraSpline 0x609760 together with F), ShowUnitRange | 41 fn, 14.4k insns. SGameView range 0x6181f0..0x629200 minus input. Camera modes 2/3/7 call SetCursorPos 0x53a310: keep them off in background runs. |
| **O** selection + orders + packets | `src/world/selection.cpp`, `src/game/packets.cpp`, new `gameview_input.cpp` (OnMouseDown 0x624a70, OnMouseUp 0x6251f0, OnMouseMove 0x6250e0, OnMouseWheel, OnKeyDown 0x622f50, OnKeyUp, the order dispatcher 0x61e740), the SGameLogic mission path except the rows F and C own (ProcessPacket 0x5737c0 / 0x5739b8, the order handlers 0x564440 / 0x564660 / 0x564720 / 0x564870 / 0x564910, 0x57e8b0, 0x57efd0 / 0x57f200 mission branches, SFoundUnits 0x56d540, StartPacketRecording / Playback 0x5805c0 / 0x580540) | 74 fn, 12.8k insns |
| **C** combat + AI | `src/game/unit*.cpp` (except `unitsave.cpp`), `gunner.cpp`, `squad*`, `singleunit.cpp`, `buildingunit.cpp`, `driver*`, `target.cpp`, new `projectile*.cpp`, `waster*.cpp`, `flying*.cpp`, `parachute*.cpp`, `combat.cpp`; `src/world/ai.cpp` RefreshAI 0x5f5c70; IncreaseUnitXP 0x5ec840; the support calls 0x5674c0 / 0x568300 / 0x568740 / 0x567760 / 0x567d40; `iunit.h` slot names (U's rights from M2) | 175 fn, 21.6k insns; plus the audit queue: 187 of the 376 new-path functions with **39 of the 93 `STUB_LOG` sites** (SGunner::ServerRefresh 0x584d00 first) |
| **H** HUD + minimap + in-game menus + market | `src/panzers/hud.cpp`, new `minimap.cpp`, `ingamemenu.cpp`, `market.cpp` (+ `market_*.cpp`); the unit panel 0x626c40 (SGameView range, H owns it) | 71 fn, 14.8k insns (the in-game menus are mostly static: ≈216 fn / 27.7k in the binary, 36 lifted from the shell) |
| **F** campaign / mission flow + save | `src/game/campaign.cpp`, `unitsave.cpp`, `src/panzers/superwindow_m3.cpp`, `trainingmenu.cpp`, `gameview.cpp` LoadMap / MissionStart (move them to `gameview_mission.cpp`), `src/game/gamelogic*.cpp` mission start / end parts (PlaceAllUnits 0x571c70, PlaceUnits 0x572ec0, BackupCampaignUnits 0x561110), `triggers.cpp` new cases (training needs action 0xe "end scenario"), the save stream (SStreamBuffer), the world load extras in `ai.cpp` / `worldload_extra.cpp` (0x5e2d70, 0x5debb0, 0x607ad0, FixBridges, wires 0x5f3d60, flying fox 0x5ecdf0) | 191 fn, 18.3k insns; audit: 44 new-path functions (12 `STUB_LOG`) |
| **E** engine gaps + SWINE-shared checks | `src/3dengine/pz/*` (scene, viewport, model stubs, trail / decal / light effects, camera shake, particles, SBitmap mips), the generic widget rows (SWidget, SWindow, SButton, SMessageBox, SRadioButton, SEditBox ...): decide per function whether the SWINE-shared code already matches HD | 268 fn, 13.8k insns; audit: 121 new-path functions (42 `STUB_LOG`) |
| **P0** (shared) | `m3common.h`, `m3trace.cpp`, the M3 headers above, `tools/census.py`, this file | |
| **I** integration (coordinator) | merges, census, CRC runs against `m3ref`, screenshot pairs | |

- `world.h`, `gamelogic.h`, `unit.h`, `iunit.h` stay shared: name fields and add declarations at
  their HD offsets / in the M3 blocks, additively.
- `superwindow.cpp` is not an M3 file: the only M3 hooks there are the OnAction call and
  ReleaseMultiView's restore. Ask F for anything else.

## 5. Cross-agent call points

| Caller | Callee | How |
|---|---|---|
| V Update 0x628430 | O / F | per 50 ms: SGameLogic::Refresh 0x576d80 (ProcessPacket inside, O), the end-of-game check that sends 0x47562 (F) |
| O input | O packets | orders become builder calls (`packets.h`), never direct unit calls: the order takes effect when ProcessPacket runs the frame (lockstep). Selection is unit +0x104 bit 0. |
| O ProcessPacket | C | the order handlers end in the unit order queue (0x5bb980 / 0x5bbb60, SUnit::SOrder) and the EC_* slots (+0xa4..+0x12c) |
| H minimap / command buttons | O | builders (RMB on minimap = move 0x02; behaviour / stop / support buttons) |
| H HUD | V | SGameView fields (+0x2614 / +0x2678 SSpecInfoWidget, +0xc68 panel, SetPanelMode) |
| F MissionStart | O, C | PlaceAllUnits (F) creates units through SWorld::CreateUnit (C's classes); Start/StopPacketRecording / Playback (O) |
| C TakeDamage / EC_Die | F | trigger events 1 / 4 (0x570e40 from TakeDamage), victory / defeat conditions |
| F save | C | SUnit::Save / Load (`unitsave.cpp`, F) read and write C's unit fields: C names them |
| everyone | E | effects through SIPixie (`ipixie.h`), models through SIModel |

## 6. Rules for the shared headers (same as M1 / M2)

1. Never reorder, insert or remove a slot, and never overload a virtual name.
2. Naming a `Slot_XX` or fixing a parameter list is allowed only for the owner (§4). Rename in
   place, keep the `+offset HD address` comment, update the implementations in the same commit.
3. No data members in interfaces; data goes in the implementation class at its HD offset. Convert
   padding into fields, never grow a block (`SGameViewData`, `SPanzersCampaign` are asserted).
4. Changes to `m3common.h` go through P0 or the integrator and stay additive.
5. Stubs start with `STUB_LOG("Class::Name (0xADDR)");` then `PZ_M3_TRACE(...)` (or
   `PZ_M2_TRACE` in M2 files). When lifted: drop the `STUB_LOG`, keep the trace, put
   `// PANZERS 0xADDR` above. `tools/census.py` counts the `STUB_LOG` lines of the M3 files
   (campaign, packets, unitsave, selection, ai, gameview, hud, minimap, ingamemenu, market,
   trainingmenu, superwindow_m3) as "m3 skeleton". Name new files with those prefixes, or extend
   `M3_FILES` in census.py in the same commit.
6. The default boot must not change: everything behind `g_M3.Enabled` until the integrator flips
   the default. Run the M2 menu CRC test (`PZ_M2_CRC=1`, 75 s, `docs/re/m2_crc_original.txt`) before
   every merge.
7. Never run the original exe. Never commit game data, recordings or tool binaries.

## 7. Gotchas

- **CRT `rand()` vs the world RNG.** The logic RNG is World+0x7518 (LCG x * 0x343fd + 0x269ec3,
  seeded 0 by the SWorld ctor 0x5d2f90, written inline in ~55 places and by 0x555a00). CRT `rand`
  is 0x78c846 (per-thread state at ptd+0x18), seeded once with `srand(time(0))` at WinMain 0x64c942
  (srand 0x78c867; the other srand callers are GameSpy / network code). Its callers:
  - logic side: SUnit::Init 0x5ba8e0 and InitNew 0x5bace0 (+0xdc RandomSide = rand()*2 >> 15, read
    e.g. by 0x5b7040 StoreUnit for buildings), SWorld 0x5e0f30 (map load: World+0x7454 records,
    +0x41 = rand() & 1, the wires), SWorld 0x5fff20 (random 1..n through the world LCG 0x555a00 on
    one branch and CRT rand on the other), SUnit::TakeDamage 0x5c4080 (rand()*6 >> 15 + 1: the
    "%d" sound sample, audio only), SWorld::Initialize 0x5ee040 (doodad sway phase);
  - render / engine: SPixie ctor 0x6941e0, the particle code 0x6e0980..0x6ef220 (≈100 call sites,
    per rendered frame), STerrain ctor 0x6efde0, 0x6c2db0, 0x6df7a0; sound 0x685070, 0x687150;
    GameSpy / network 0x70e390, 0x71e730, 0x72c5e0, 0x7349f0, 0x74eb10, 0x751260, 0x763450,
    0x531430, 0x5316a0.
  Because particles draw per rendered frame and the seed is the time, **the CRT rand state is
  frame-rate dependent in HD itself.** The HD replay still matched on 2,199 frames, so in that
  session no CRT draw reached the CRC fields. Rules: (1) logic draws use World+0x7518 only, exactly
  where HD draws them; (2) call CRT `rand()` only at the HD CRT sites above and cite `0x78c846`;
  (3) no CRT draw may feed a CRC-hashed field (+0xfc, +0x8c/+0x90/+0x94, +0xb0, +0x114, +0x108,
  +0x1dc) or anything that later moves a unit; if HD does that (for example RandomSide deciding a
  building exit), log it as an HD nondeterminism and tell the coordinator; (4) our render code may
  use CRT rand freely. Then the CRC is independent of the render rate.
- **`src/world/world.cpp` SWorld ctor, player records +0x40: `p[16] = rand(); // HD 0x7669f1`**
  is a decompile slip: 0x7669f1 is a CRT double -> int conversion helper (`MOV ECX,1; MOVSD
  XMM5,...; JMP 0x766a1c`) applied to the timer value 0x661800 just before it, not `rand`. The
  recompile draws one CRT rand per player slot that HD does not draw (F: fix it to the truncated
  timer, it is not in the CRC).
- **Modal warning boxes.** `SLogger::Warning` opens an OK / Cancel box that blocks the main loop
  (and a WM_CLOSE inside it tears the world down under the caller). Use `Log(1, ...)` for
  recompile stubs; only HD's own 0x65cac0 messages are Warnings.
- **SendAction starts at the sender.** `SWidget::SendAction` calls `this->OnAction` first, then the
  parents. An OnAction that returns true for everything swallows the actions it sends itself; return
  false for actions you do not handle (SGameView, the menus).
- **Focus.** Keys go to the focus chain (SWindow::GetKeyEventTarget). New screens must take the
  focus (HD 0x5439f0 walk; `FocusWidget` in superwindow_m3.cpp), or Esc never reaches SGameView.
- **The market moves the world aside.** While SMarket is up, `g_World`, `g_GameLogic` and `g_Scene`
  are null (0x658b10 case 1); ReleaseMultiView 0x65b8c0 restores them. Code that runs every frame
  (SGameView::Update) must check for null.
- **The mission SGameLogic ctor gets the view.** HD: `new SGameLogic(0, view +0x828, view +0x58)`;
  p3 is the SIGameViewCallback pointer (the menu passes 0). The skeleton still passes the menu
  arguments (0, -1, 0) (F / O).
- **Frame 0 twice.** The mission logic exists from LoadMap (it counted frame 0 with 346 units in the
  original); PlaceAllUnits at mission start adds the army. `m3ref\tc1_crc_ref.txt` keeps only the
  mission-start frame 0 (358 units).
- **RunTriggers action 0xe fires at mission start** in the skeleton (the training map's "end
  scenario" triggers; the original does not end the mission). Probably because PlaceAllUnits does
  not place the army yet, so a "no units" condition holds (F).
- **The packet bools.** Most records start with two u8: B1 (MoveFoundUnitsToLocation p4) and B2
  (the queue flag, Shift). Builders write them first although they are the last arguments.
- **Opcode 0x30** has a builder (0x575d20) but no ProcessPacket case; 0x14 has neither.
- **Not mapped yet:** Concert +0x80(1) (music stop) at the game view and mission start; the HD
  "Are you sure?" boxes (SMessageBox 0x3ec, 0x53e0d0); the modal training dialog.
- Background runs: camera modes 2 / 3 / 7 call SetCursorPos; the trace of SGameView::Update counts
  every rendered frame.

## 8. Verifying against the oracle (no original runs)

| Agent | Check |
|---|---|
| all | build: 0 errors, census line as expected; default boot unchanged (M2 menu CRC 0 mismatches, `PZ_M2_CRC=1`, 75 s); `-m3` round trip twice without a crash |
| F (first) | lift StartReplay 0x597510 / ReadReplayHeader 0x595780 and make our `-packetplay m3ref\tc1.rec` start the mission with the recorded army (frame 0 = 358 units, CRC `737376f6`, seed `141334d6`) |
| O | replay `tc1.rec`: our ProcessPacket must apply the recorded records and log no "Inconsistency"; `PZ_M2_CRC=1` lines equal `tc1_crc_ref.txt` up to the first combat event |
| C | the same replay through combat: equal CRC / seed / unit count to frame 3902 (deaths: 358 -> 344); RNG draws in the HD places (seed column) |
| V, H | screenshot pairs against `m3ref\shots` (rec_g1 0:04, rec_g3 0:47, rec_g8 2:46 by the HUD clock: frame = seconds x 20 in a replay of tc1.rec), and the HD camera start position (our skeleton already matches `pa_1` / rec_g1's framing) |
| E | no visual-only change may alter the CRC lines of a replay; SWINE-shared verdicts recorded in the TSV row notes |

Diff tools: `m2i\tools\crcdiff.py <our log> m3ref\tc1_crc_ref.txt` (reads `PZM2 CRC` lines vs `T`
lines), `m3ref\tools\mcmp.py` for two ocrc3 traces.

## 9. Not verified

- Slot and function names marked "(name guessed)"; parameter lists typed only by dword count
  (`int pN`).
- The SPanzersCampaign fields beyond the asserted ones; the SGameViewData offsets other than the
  asserted ones come from the scope and single readers.
- The 53 packet handlers' command meanings (only move / move-dir / move-back are named).
- Whether the HD market really hides the game view (the skeleton hides it; HD shows it again with
  +0x6c(1) at Start Mission).
- The `m3agent` split was made by address range and class label (`~` rows of the TSV are range
  guesses); move rows between agents through the coordinator.
- The replay was compared to frame 2198 of 3902; one nation, one army.
