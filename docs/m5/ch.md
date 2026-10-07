# M5-CH: the chat line and the cheat codes

## How to use

In a single-player mission (campaign, Training Camp, scenario, skirmish) press **Enter**: a
`Cheat:` line opens above the panel. Type a code and press **Enter** again; **Esc** closes the
line without doing anything. Codes are case-insensitive and must match the whole line (except
the experience code, which may be followed by a number). A red `Cheat enabled (...)` message
confirms it.

| Code | Effect |
|---|---|
| `SheepInTheTrees` or `FreeWestMemphis3` | "godmode": toggles World +0x4d0, the cut-scene "show everything" flag (no fog of war: every unit and building interior shown). Enter it again to switch it off (the message still says "enabled"). |
| `SpotTheBraincell` or `SpotTheBraincell <n>` | the **selected** units get n experience (1000 by default) |
| `MoneySong` | +1000 prestige |
| `TheSpanishInquisition` or `Inferno` | instant kill: every hit kills (SGameLogic +0x2d8, TakeDamage 0x5c4080) |
| `BicycleRepairMan` | unlimited cargo: supply / repair trucks and support buildings don't use cargo (SGameLogic +0x2d9, 0x548bf0) |
| `SelfDefenceAgainstFreshFruit` or `Kilmister` | invulnerability for your units (SGameLogic +0x2da: crew damage 0x5c3dd0, TakeDamage 0x5c4080, squad EC_Die 0x598280) |
| `MrHilter` | +100 to each of your five outside-support counters (paratroopers, artillery, air ...) |
| `DirtyHungarianPhrasebook` | win the mission (mission result 1, the victory box) |
| `TheFunniestJokeInTheWorld` | the Monty Python joke as a message; every placed, unmounted unit you can see whose owner's player record +0x04 is 0 dies |

Instant kill, unlimited cargo and invulnerability stay on for the rest of the mission (the
logic flags are cleared by the SGameLogic ctor 0x55e440). Every code that takes effect sets
the campaign's cheat state (+0xb84 = 1 via 0x597180, saved with the game): the results screen
then gives no medals / high score (results.cpp reads +0xb84); Restart Mission clears it
(0x597180(0)). Note the HD string at 0x807e10 ("...started campaign game spoil without
cheatcodes") is a medal condition text; the flag behind it is this +0xb84.

Dead codes kept as in HD: after `SpotTheBraincell` matched, HD also parses `smarten <n>` and
`beautiful mind <n>` (cannot match); after `DirtyHungarianPhrasebook`, `jump <n>` ("German %d" /
"Allied %d" / "Russian %d" as the next mission) cannot match either. Neither is reachable.

## What is lifted (HD addresses)

- SGameView::OnKeyDown 0x622f50, the Enter case 0x623728..0x62417e (chatline.cpp
  `PzChatLineEnter`): opens the line when no dialog (+0x3e48..+0x3e68) is open (check box hidden
  and unchecked, frame +0x5d8 "Cheat:" font 3 size 1, edit text "", shown, focused 0x5439f0);
  with the line open runs the cheats and hides frame / check box / edit. The Esc case's
  "+0x615 open: hide the line" part (gameview_input.cpp).
- The cheat comparisons and effects 0x623859..0x623fcb (cheats.cpp `PzCheatRun`). The strings are
  compared with SString == 0x52c410 (`_stricmp`) and SString::Mid 0x5336b0; no hashing.
- SGameView::Create 0x619c90 part 0x61bd6b..0x61be1c (chatline.cpp `PzChatLineCreate`): board
  text frame +0x5d8 at (0x1b8, 0x210), SEditBox +0x5dc at (0x1bd, 0x210, 0xf8, 0x12)
  Create(0, 1), SCheckBox +0x75c at (0x190, 0x226) "Send message to allies only", checked, all
  hidden. The widgets are the existing pz::SEditBox (savemenu.cpp) and pz::SCheckBox; they live
  in a side object made / freed with the HUD (the recompile's SGameView has no room at +0x5dc).
- SPanzersCampaign 0x597180 (SetCheated: +0xb84 = v unless it is 2) - also replaces the
  `STUB_LOG("SPanzersCampaign 0x597180(0) after Restart Mission")` in ingamemenu.cpp.
- 0x597500 (prestige -= p) inline.

Not lifted: the multiplayer chat branch (0x623750..0x623854, "To allies:" / "To everybody:",
SMulti 0x521c00) - the recompile has no SMulti. 0x6255b0 (Enter with +0x38dd set) is the
bug-report screenshot ("Screenshots/%s.tga" + description .txt), not the chat; still unmapped.

## Fix of an existing lift

`UnitSuppliesForFree` 0x548bf0 (buildingunit.cpp) read byte +0x2d9 of the *unit*; HD calls it
with ECX = DAT_008f2078 (g_GameLogic) at all three call sites (0x54aec5, 0x5bf49a, 0x5c0380),
so it reads the logic's unlimited-cargo flag. The unit's +0x2d9 is the high byte of Towed
(+0x2d8, -1), so before this every supply was free. Regression is unchanged (below).

## Files touched outside the new files

- `src/panzers/gameview_input.cpp`: Enter case (`PzChatLineEnter`), Esc closes the open line.
- `src/panzers/hud.cpp`: `PzChatLineCreate` / `PzChatLineDestroy` in PzHudCreate / PzHudDestroy.
- `src/panzers/gameview_view.cpp`: SetPanelMode hides the line (HD 0x625d80 hides +0x5d8,
  +0x5dc, +0x75c) instead of only the frame.
- `src/panzers/ingamemenu.cpp`: Restart Mission calls 0x597180(0).
- `src/game/buildingunit.cpp/.h`: the 0x548bf0 fix above.

## Checks

- Census: 2525 lifted / 1316 SWINE-shared / 141 stubs -> 2529 / 1316 / 140.
- regress.ps1: menu 1181 mismatches 0, tc1 replay 3901/3901 equal, tutorial replay 2012/2012
  equal, all exits clean.
- Training Camp (PZ_M5_MISSION="Training camp", PZ_M5_AUTO=1): every code entered in turn: the
  `Cheat:` line shows and takes the text, each code prints its message (log `PZM5 CHEAT ...`),
  support counters 10 -> 110, prestige +1000, `DirtyHungarianPhrasebook` opens the VICTORY box;
  Esc closes the line. `SheepInTheTrees` makes the fogged units around the camp visible at once.
  Not verified on screen: the experience code with units selected (no own units at that point of
  Training Camp; posted Ctrl+A / box drags did not select any), the joke (no enemies there),
  instant kill / invulnerability / cargo in combat (their readers were already lifted). German 1 starts with its intro cut-scene, during which Enter is ignored
  (as in HD: ViewState 2).

## Reference to record from the original (optional)

In HD Training Camp: Enter, type `MrHilter`, Enter. Expect the red "Cheat enabled (more
outside support)" line at the left above the panel and the support buttons 10 -> 110; capture
a screenshot with the `Cheat:` line open (position of the prompt relative to the edit box).
