# M5 SV: the in-game Save menu and Load "Before"

Branch `m5-sv` (agent M5-SV). HD addresses throughout.

## What is lifted

| Part | HD | Recompile |
|---|---|---|
| SGameView::OpenSaveMenu | 0x6205f0 | `ingamemenu.cpp` `PzOpenSaveMenu` (slot +0x3e4c) |
| SSaveMenu ctor / dtor | 0x62ccb0 / 0x62d3f0 (deleting 0x62d7a0), 0x474 bytes over SCenterMenu | `savemenu.cpp` |
| SSaveMenu::Create | 0x62fcb0 | `savemenu.cpp` |
| SSaveMenu::OnAction | 0x632040 | `savemenu.cpp` |
| SSaveMenu::OnKeyDown / OnMouseDown | 0x632530 / 0x632590 | `savemenu.cpp` |
| SSaveMenu Save / Deselect | 0x632a50 / 0x630f20 | `savemenu.cpp` |
| SEditBox (Panzers' ANSI edit box, 0x180 bytes) | ctor 0x53a6c0, dtor 0x53a720, Create 0x53a750, GetText 0x53a8b0, OnChar 0x53a8c0, OnKeyDown 0x53aad0, OnMouseDown 0x53adc0, OnTimer 0x53adf0, SetText 0x53ae10, Update 0x53ae80 | `savemenu.cpp` (`pz::SEditBox`) |
| SSaveLoadListBox::GetText | 0x53fa70 | `loadgame_menu.cpp` |
| SGameView::OnAction 0x49531 / 0x49533 / 0x494c3 | 0x6216b0 | `ingamemenu.cpp` |
| SGameView::OpenLoadMenu (in-game Load Game) | 0x620140 | `ingamemenu.cpp` `PzOpenLoadMenu` (slot +0x3e50) |
| Esc: close the dialogs / the in-game menu | 0x622f50 case VK_ESCAPE | `ingamemenu.cpp` `PzGameViewEscape` |
| Load-game LoadMap: close the dialogs first | 0x61f840 (start) | `ingamemenu.cpp` `PzGameViewCloseDialogs` |
| SPanzersCampaign::LoadGameBefore | 0x595330 | `campaign_save.cpp` `CampaignLoadGameBefore` |
| The save type of a file | 0x5955d0 | `campaign_save.cpp` `ReadSaveGameType` |
| SCampaign::GetName | 0x5925d0 | `campaign_save.cpp` `CampaignGetName` |
| SSuperWindow::OnAction 0x494c1, type 2 | 0x659250 | `loadgame.cpp` `PzLoadGameAction` |

### The Save menu (HD behaviour)

- In-game menu Save Game (0x494d1): the in-game menu goes, SSaveMenu opens (the game stays paused).
- "Save Game" centre menu: Delete (x 3), Save (x 199), Back (x 0x18b); "Games" caption; the save list
  (font 0, 0x12 rows, the same columns as Load Game) with "Empty slot" first and the saves of
  `LoadSavedGameNames` 0x595fa0 sorted newest first from row 1; Save and Delete start disabled.
- A click on a row opens the edit box over the title column of that row (x 0x31, y row * 0xe + 6,
  black background): "Empty slot" proposes `SCampaign::GetName` (the map in Training Camp /
  Tutorial / scenario, the mission's localised `Name` in a campaign), a save its title. Save is
  enabled, Delete only for a save.
- Save (button or Enter): "Empty slot" writes `SaveGames/<time(0)>.save`; a save is overwritten in
  place (HD asks nothing). The title is the edit box text. Then 0x49531: the view deletes the menu
  and runs the game again unless it was paused before the menu opened.
- Delete: `DeleteFileA("SaveGames/<file>")`, the list is filled again, nothing selected.
- Esc with the edit box open, a click on the menu background, or scrolling the list: deselect (edit
  box hidden, Save / Delete disabled). Esc otherwise closes the menu and opens the in-game menu
  (0x622f50); Back (0x49533) does the same.
- The edit box: printable characters while the text is narrower than the box - 0x12 and shorter
  than 0xff; Backspace, Delete, Home, End, Left, Right; Tab / Enter / Esc go to the menu; every
  other key is taken (no hot keys while typing). Caret '|' blinks every 500 ms while focused.

### Load "Before"

`SaveGameBefore` 0x596b30 (already lifted, called by `LetResultsDone` 0x594e70) writes
`SaveGames/<code>-Before.save` (type 2, 'v2ps'). The Load Game action 0x494c1 reads the type
(0x5955d0); for type 2 HD deletes the game view and the window scene and the campaign, makes a new
campaign, `LoadGameBefore` 0x595330 (map, mode 3, race, start prestige, the carried Army, section,
+0xe4, difficulty, +0xf8, +0xfc, score, +0xb84; then `PrepareMission` 0x592d80: MissionArmy = Army +
the mission's units, Prestige = start prestige, objectives, MenuToLoad briefing or game view), and
`LoadNextCampaignView` 0x658b10. LoadGameBefore has exactly one caller in HD (0x659d20 in
0x659250); results "Restart" is `RestartMission` 0x594f60, not a load.

## Recompile differences

- The edit box gets UTF-16 WM_CHAR codes (the recompile's window is Unicode); a code above 0x7f is
  converted to the ANSI code page first (HD's window got code-page bytes). The DBCS paths
  (0x64dd70, DAT_00929dec) are not mapped.
- `ReadSaveGameType`: HD throws out of a bad header (crash); the recompile returns 0 and the Load
  Game action warns instead of panicking ("unknow CAMPAIGN_SAVEGAMETYPE").
- The hint "Delete the selected savegame file" (0x543bf0) is not shown (tool tips not mapped).

## Fixed on the way

- The titles of F6 (`Quick - <name>`), the "Before" save and the mission-start save used
  `GetMapName` 0x592040 where HD uses `GetName` 0x5925d0. Same text in Training Camp / Tutorial;
  in a campaign the mission's `Name` instead of its map path.

## Census

Before (master b298750): 2432 lifted / 1316 SWINE-shared / 174 stubs. After: 2459 / 1316 / 169
(the four logged SSaveMenu / OpenSaveMenu stubs and the OpenLoadMenu stub are gone).

## Verification (scratch `m5sv\shots\`)

All runs `-nointro -m3` from the run folder (see the device note below).

- Training Camp (PZ_M4_LOADGAME=TRNG-Start.save): Esc -> Save Game shows the menu (`t4_save.png`:
  Empty slot, the saves, Delete / Save disabled); a click on Empty slot opens the edit box with
  `maps/training.map` (`t5_edit.png`); Backspace x4 + typed text -> `maps/training - SV test`
  (`t5_typed.png`); Enter saved `SaveGames/1791380294.save`, the menu closed and the game ran again
  (`t5_resumed.png`, clock 0:37 -> 0:44, PAUSE gone). In-game Load Game lists it newest first
  (`t5_loadmenu.png`); Load loaded it (frame 793, 357 units) and the game went on (`t6_loaded2.png`).
- German 1 (New Game, German, Normal, diary, briefing, the intro cut-scene): Esc -> Save Game ->
  Empty slot proposes `Airfield - Poland (1 SEPT 1939)` (the mission's Name), typed ` SV`, Enter
  (`g6_typed.png`). Main menu Load Game lists it (`l1_list.png`); Load: frame 4010, 222 units, the
  mission clock runs on (`l1_loaded2.png`, 4:27).
- Overwrite / Delete: picking an existing save and Save rewrote that file with the edited title;
  Delete removed the file and refilled the list (`o3_deleted.png`).
- Load "Before": the German 1 save with `PZ_M3_FORCE_END=1 PZ_M3_FORCE_END_ONCE=1` -> victory ->
  results Continue wrote `SaveGames/G-02-Before.save` (title "Diary - Village - Poland ..." from the
  localised "Before" text). Main menu Load Game of it: LoadGameBefore, PrepareMission
  (`[German 2] maps/ger-02.map, prestige 150, army 10 (9 carried), MenuToLoad 2`), German 2 starts
  from its beginning with its cut-scene and the carried units (`bl_brief2.png`, `bl_next.png`).

## Regression

- Committed exe: menu 0/1181, Tutorial 2012/2012; tc1 stops at frame 270 (`CreateVertexBuffer
  failed 8876086c`, 0 differing frames up to there) because the D3D device is lost (see below).
- The same code with a test-only change (not committed) that skips a same-size `SDXWindow::OnSize`:
  menu 0/1181, tc1 3901/3901, Tutorial 2012/2012, clean exits.

### The device loss (not from this branch)

Since about 15:50 most in-game runs on this machine (m5sv, m5vx, m5ms) get a second
`SDXWindow::OnSize(1024, 768)` a few frames into the mission; `SGepard::ResetDevice` then fails with
D3DERR_INVALIDCALL (a D3DPOOL_DEFAULT resource is still alive) on every frame, and the next model
load panics. With the second same-size resize ignored the runs are fine. `SWindow::SetPosition`
uses MoveWindow, which sends WM_SIZE even for an unchanged size, so a widget calling the window's
SetPosition could be the source. Renderer owners should look at both (the extra WM_SIZE and the
leaked default-pool resource).
