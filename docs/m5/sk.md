# M5-SK: single-player Skirmish

Agent SK, branch `m5-sk`. Ground truth: the 2016 HD `PANZERS.exe` (static analysis only).

## 1. Scope: from the Skirmish button to the first mission frame

### Flow in HD

| Step | HD | What it does |
|---|---|---|
| Single player → Skirmish | SSingleMenu::OnAction 0x63c7e0 → 0x543930(**0x534d1**) | sends the action to the super window |
| SSuperWindow::OnAction 0x659250, case 0x534d1 | | deletes the main menu (+0xe8), deletes any SMulti (`DAT_008f1a74`, vtbl +0x30(1)), deletes the campaign, `new SPanzersCampaign` (0x590ec0), **InitMultiMode 0x593b70** (GameMode 4, MenuToLoad 1, LoadMissionProps 0x593740), LoadSkirmishChatRoomView |
| **LoadSkirmishChatRoomView 0x658e70** | | when there is no SMulti: `new SMulti` (0x5130, ctor 0x51dcf0) and **SMulti::Server 0x51e950**("", 0, 0). Server binds the UDP socket (0x535cc0) and, when that works, makes this machine slot 0: +0x4f3c server = 1, slot 0 connected (+0x4c5c), local slot +0x4790 = 0, name = the player name (0x64e070), not ready. Then `new SSkirmishChatRoomMenu` (0x1244, ctor 0x652d20) into SSuperWindow +0x108, 0x400 x 0x300, **Create 0x653250**, +0xd8 = 1, the window scene is released |
| SSkirmishChatRoomMenu::Create 0x653250 | | full-screen menu "Skirmish", Start Game / I'm Ready / Lock Settings / Cancel (0x64c010), a **new campaign again** + InitMultiMode, Team 1 / Team 2 buttons and panels (4 slots each: status drop list, nation drop list, "Ready" text), Armies list + New / Edit / Delete, Game Settings (Prestige limit 1500/2000/2500, Game type Team Match / Domination / Assault, Game age Early / Late), Select map list (0x655ab0 fills it from `multimaps/(*.map`, `multimaps/Factory/*.map` or `multimaps/Assault/*.map`; IsMap 0x653ee0 keeps maps with a MINI chunk), "Map:" + name. **Slot 4 becomes a Computer** (SMulti +0x4de1 = 3) and SMulti +0x512c = 1 (skirmish) |
| room Update (vtbl +0x78) 0x656210 | | per frame: campaign MapName = SMulti map path (+0x4b44), slot drop lists (Open / Closed / Computer / Kick Player), Ready texts, the mini map (0x654b30), enables Start when the host's slots on both teams are filled and every human is ready with an army |
| room OnAction 0x654df0 (named "PlayerNameDropList" in the symbols) | | Team buttons (0x51e3c0 change team), Lock Settings (+0x4a30 toggle, 0x5215e0 send settings), I'm Ready (0x51e2b0), **Start Game** → 0x521d60 (SMulti +0x4774 = 1 "START GAME!"), Cancel → **0x534b2**, Edit → **0x534b3**, New → **0x534b4**, Delete → "Are you sure ...?" box then DeleteFile + LoadArmyNames 0x654030; drop lists (0x444c1): nation (0x51e3a0), status (Open / Closed / Computer), settings; list box (0x4c421): map (+0x4a40 name, +0x4b44 path, 0x65ec50 CRC) or army (LoadCurrentArmy 0x6547b0) |
| countdown | room +0x78 sees SMulti +0x4774: timer 1000 ms x 4 (+0x524 / +0x528); room OnTimer 0x655a60 sends **0x534b1** at the end | |
| SSuperWindow::OnAction 0x534b1 | | deletes the room, **LetChatroomDone 0x594dc0** (MenuToLoad 4 → 2), LoadNextCampaignView 0x658b10 case 2: new SGameView, Create, **LoadMap 0x6201c0** |
| SGameView::LoadMap 0x6201c0 | | map = campaign GetMapName (mode 4: +0x20 = the multimap path); in multi mode it starts the mission at once (no market): **MissionStart 0x6281a0** (no "Start" save with SMulti); 0x51eec0 resets the per-host ping times |
| first frame | SGameLogic::Refresh 0x576d80 | OnMapLoaded / PlaceAllUnits **0x571c70** multi branch: for every slot that is connected → PlaceUnits 0x572ec0(slot, SMulti army of the slot); for a **Computer** slot (status 3) → an army from 18 hard-coded tables (prestige limit 1500 / 2000 / other, game age, nation, 50/50 random), PlaceUnits, an AI group "SKIRMISH_AI %d" (0x55cd40, 0x5ecbe0), every unit of the slot joins it (SetAIGroup 0x5c0c10), 0x601060, tactic 4, then a random enemy player's "start %d" location and AttackMoveTo 0x5d95b0 there |
| end | GV_GAMEOVER 0x47562 → SMulti deleted, LetMapDone (mode 4 → results 3), SResultsMenu; Continue → LetResultsDone: mode 4 with no SMulti → MenuToLoad 4 → 0x658a30 (the multiplayer pre-menu) | |

### What of the multiplayer layer the offline skirmish needs

HD has **no separate single-player path**: the skirmish room runs a real SMulti
server (UDP socket 5555) with the local player in slot 0 and Computer players in
other slots; no packet is ever sent because no other host is connected
(every send loop skips the local slot and unconnected slots).

The recompile does **not** lift SNetworkUDP / SMulti. It keeps the SMulti
*data* the skirmish reads (slots, settings, map, the local player's army)
in a small recompile struct, `PzSkirmish` (src/panzers/skirmish.h, HD SMulti
offsets in comments), owned by the room and alive until the game ends:

- slots[8]: connected +0x4c5c, status +0x4c71 (0 Open, 1 Closed, 2 human, 3 Computer), ready +0x4c72, name +0x4c73, nation +0x4ca8; team = slot / 4.
- settings: game type +0x4a38 (19000), prestige limit +0x4a34, game age +0x4a3c, locked +0x4a30, map name +0x4a40, map path +0x4b44, started +0x4774, skirmish +0x512c, local slot +0x4790 = 0, server +0x4f3c = 1.
- per-player army (SDArray of UNTD at +0x4820 + slot * 0x34).

The SMulti functions the room calls become local operations on that struct
(0x51e2b0 ready, 0x51e270 not ready, 0x51e3a0 nation, 0x51e3c0 / 0x51ed30
change team, 0x521d60 start, 0x51e1b0 / 0x51e150 / 0x521a70 / 0x5215e0
"send to the other hosts": nothing to send offline).

In-game, the recompile's logic stays the single-player path (no frame sync,
no lockstep): the packets of the local player are executed directly.

## 2. What is done (with -m3, as the rest of the campaign shell)

**Single player → Skirmish → room → game with a Computer opponent → end box → results → main menu**
works on the shipped multimaps (`multimaps/`, read-only).

| Part | HD | Where |
|---|---|---|
| Skirmish button: SendAction 0x534d1 (was the STUB_LOG) | SSingleMenu::OnAction 0x63c7e0 | src/panzers/campaignmenu.cpp (one line) |
| 0x534d1: main menu deleted, SMulti deleted, new campaign, InitMultiMode, LoadSkirmishChatRoomView | SSuperWindow::OnAction 0x659250, 0x658e70 | src/panzers/superwindow_sk.cpp |
| 0x534b1 (start): room deleted, LetChatroomDone, LoadNextCampaignView (game view, LoadMap, MissionStart at once) | 0x659250, 0x594dc0, 0x658b10 | superwindow_sk.cpp, src/game/campaign_multi.cpp |
| 0x534b2 (Cancel): ReleaseMultiView, room deleted, main menu, SMulti deleted | 0x659250 | superwindow_sk.cpp |
| GV_GAMEOVER: SMulti deleted before LetMapDone (mode 4 → results) | 0x659250 | superwindow_sk.cpp (then superwindow_m3.cpp) |
| The room: title, Start Game / I'm Ready / Lock Settings / Cancel, Team 1 / Team 2 buttons and panels (Name / Nation / Status columns, a status and a nation drop list and a green "Ready" per slot), Armies list + New / Edit / Delete, Game Settings (Prestige limit, Game type, Game age), Select map list, "Map:" + name, the map preview with the compass; slot 4 a Computer | ctor 0x652d20, Create 0x653250, dtor 0x652ef0, OnKeyDown 0x655a50, SetVisible 0x656110 | src/panzers/skirmish.cpp |
| Per frame: slot lists (Open / Closed / Computer, the player's name / Kick Player), Ready texts, settings enabled until locked, Team / Attack-Defense labels, the campaign's map name and SP, the army list, I'm Ready / Start Game enabled (both teams filled, every human ready with an army), the 1 s countdown | Update 0x656210, OnTimer 0x655a60 | skirmish.cpp |
| Team change, Lock / Unlock (Computers become ready; unlock resets ready), I'm Ready (the campaign's mission army becomes the slot's army), Start Game, nation / status / settings drop lists (prestige list by age 0x6560a0, map list by game type), map / army selection, the Delete box | OnAction 0x654df0 ("PlayerNameDropList" in the symbols), 0x51e2b0, 0x51e270, 0x51e3a0, 0x51e3c0 / 0x51ed30, 0x521d60, 0x5215e0 | skirmish.cpp |
| Map list: `multimaps/(*.map`, `multimaps/Factory/*.map`, `multimaps/Assault/*.map`, maps with a MINI chunk | 0x655ab0, IsMap 0x653ee0 | skirmish.cpp |
| Map preview: the MINI bitmap pasted into menu/minimap_hq.tga, compass over it | LoadMiniMap 0x654b30 | skirmish.cpp |
| Campaign multi mode: InitMultiMode, LetChatroomDone, SetArmyName (clear), SetRace, SetMissionSP | 0x593b70, 0x594dc0, 0x591d50, 0x5974b0, 0x597480 | src/game/campaign_multi.cpp |
| Players: local player = local slot; humans type 0, Computers type 1, the rest type 2; teams 1 / 2 by slot, race = slot nation | SGameLogic ctor 0x55e440 (SMulti part) | src/game/skirmish_game.cpp (hook in gamelogic.cpp) |
| Armies: every connected slot its army (PlaceUnits 0x572ec0); every Computer slot an army from HD's 36 tables (0x7f6958..0x7f75b0; age, prestige limit, nation, 50/50 by the world LCG; crew / rifle squads stored in vehicles that need them), its group "SKIRMISH_AI <slot>" (all its top-level units), tactic 4, AttackMoveTo the "start <n>" location of a random enemy | PlaceAllUnits 0x571c70 (SMulti branch) | skirmish_game.cpp (hook in gamelogic_mission.cpp) |
| Every 20th frame after 100: a player without a live unit of class 0 / 0xb / 5 has lost ("Computer %d has lost the game" / "%s has lost the game"); a human out takes his team out; only the local team left → victory; Assault: the defenders lose without their "Assault" unit | Refresh 0x576d80 (SMulti part) | skirmish_game.cpp (hook in gamelogic.cpp) |
| End: the tutorial's Victory / Failure boxes (no autosave, no statistics menu, no Defeat.mp3); a player out while a team mate fights on keeps watching; no "Start" save with SMulti | SGameView::Update 0x628430, MissionStart 0x6281a0 | gameview_mission.cpp, results.cpp (small hooks) |
| Results: only the unit table (names "COMPUTER" / the player's name) and Continue | SResultsMenu::Create 0x6360f0 multi branch, BackupCampaignUnits 0x561110 (names) | superwindow_sk.cpp (hook in results.cpp) |

### Still stubs / not lifted (logged)

- **Army making.** The army files (`armies/<G|A|R><e|l><SP>*.army`, written by SMarket::SaveArmy
  0x64a180) are read: LoadArmyNames 0x654030 lists them, LoadCurrentArmy 0x6547b0 buys the army
  (signature, `ARMY` v5 / `AREC` v2 chunk, name, CRC32 of the body, the body xor-coded backwards,
  race, SP, records 0x51f860). So army files from an original HD install work. But **New / Edit**
  (0x534b3 / 0x534b4) need the market's army-making mode (LoadNextCampaignView case 1 with mode 4,
  SMarket multi mode, SaveArmy 0x64a180, the 0x4d542 return to the room): logged. Without an army file
  (or the test hook) a human has no army, and Start Game stays disabled (faithful: HD needs one too).
- **After the results:** HD goes to the multiplayer pre-menu (LoadNextCampaignView case 4 → 0x658a30,
  or the RankedGaming room 0x658230), which is not lifted: the recompile goes to the main menu (logged).
- Domination (factories / reinforcements) and Assault specifics beyond the end check, coop (game type 3,
  not offered by the room), the SMulti chat lines, and the per-frame BackupCampaignUnits statistics
  of the other players are not lifted.

### Test hook

`PZ_M5_SK_ARMY=1` (recompile only, inert when unset): the army list gets one more item
"PZ_M5_SK_ARMY"; choosing it gives the local player variant A of the Computer army of his nation (the
same tables), so a skirmish can be played without an army file. `PZ_M5_SK_SAVEARMY=1` also writes
that army as an army file the way SaveArmy 0x64a180 does (`armies/<G|A|R><e|l><SP>-<time>.army`,
name `PZ_M5_SK test army`); after Unlock / Lock the room lists it and a skirmish starts from it
(tested: the reader accepts it, the army is bought and placed).

### Files outside my ownership (minimal hooks)

- `src/panzers/campaignmenu.cpp`: the Skirmish STUB_LOG → SendAction(0x534d1).
- `src/panzers/superwindow_m3.cpp`: SuperWindowM3Action calls PzSkirmishAction first.
- `src/game/campaign.h`: six multi-mode method declarations.
- `src/game/gamelogic.cpp`: two guarded calls (player set-up in the ctor, the lost / victory check after RunTriggers), `if (g_Skirmish && Mode)`.
- `src/game/gamelogic_mission.cpp`: PlaceAllUnits calls the SMulti branch first.
- `src/panzers/gameview_mission.cpp`: no "Start" save with a skirmish; the team-mate check.
- `src/panzers/results.cpp` / `results.h`: the end boxes with a skirmish; the multi results branch.
- CMakeLists (src/panzers, src/game); docs/STUBS_HIT.md.

## 3. Verification (our build, `-nointro -m3`, run folder `scratchpad\m5sk\run`)

Screenshots in `scratchpad\m5sk\shots\`:

- `s04_room.png`: the room as Create leaves it (slot 0 the player, slot 4 a Computer, Prestige 2000,
  Team Match, Early, the 12 Team Match multimaps, "Map: (4) airport"); `s21_room_preview.png` with the
  map preview; `s28_drop.png` a drop list open; `s30_dom_ready.png` Domination (9 Factory maps), an
  allied Computer in slot 1, Computers in slots 4 (Russian) and 5, everybody ready.
- `s06_ready.png`, `s17_locked.png`: Lock Settings → the Computers are ready, the army list, I'm Ready.
- `s07_game.png` … `s13_game.png`: (4) airport, the player's army and SKIRMISH_AI 4 (20 units). The
  log line every 30 s shows the group marching to the player's start and fighting: 20 live units at
  (228, 203) at frame 600, 17 at (75, 79) at frame 3600, 5 at frame 6600.
- `s31_dom_game.png`: Domination on (4) budapest with three AI groups (the ally's 35 units, the two
  enemies' 11 and 20) moving and fighting.
- `s14_end.png` (PZ_M3_FORCE_END=1) Victory box, `s22_defeat.png` (=2) Failure box, `s18_results.png`
  the multi results (swine / COMPUTER rows, Continue), `s19_after.png` main menu, `s20_room2.png` a
  second skirmish room after a game.
- `s24_armyfile.png`, `s27_game.png`: an army file written by PZ_M5_SK_SAVEARMY is listed and a game
  starts from it.

Observation for the renderer owners: with many allied units the minimap's allied dots (board +0xa4)
show as a connected yellow scribble (`s31_dom_game.png`), not as dots.
- `s32_natural.png` (`-skipframes 16000`): a game played out by itself: SKIRMISH_AI 4 attacks, falls
  back, attacks again; at 13:20 "Computer 5 has lost the game" and the Victory box.
- `s33_team2.png`: Team 2 moves the player to the first free slot of team 2 (slot 5).
- `s35_cancel.png`: Cancel → main menu. The 3D menu background stays black: LoadSkirmishChatRoomView
  0x658e70 releases the window scene, and LoadMainMenu → LoadMenuBackground 0x658690 does nothing
  while the menu frames are still up (HD's own `+0x194 < 0` test), so this follows HD's code.
  **Reference to record from the original:** Single player → Skirmish → Cancel: is the main menu's 3D
  background black too?

Regression (`coord\regress.ps1`, final build): menu CRC 0 of 1181 differ; Tutorial replay frames
0..2011 equal (2012/2012). The tc1 replay stops at frame 270 with "SMesh::CreateVertexBuffer failed:
8876086c" after a window OnSize whose D3D Reset fails (hr 0x8876086C) at frame 2; frames 0..270 are
equal. **The same happens with master b298750 built unchanged** (`_pz5skb`, run the same way at
17:15), so it is the machine's D3D state at the time, not this branch: the coordinator should re-run
tc1 on a quiet machine.