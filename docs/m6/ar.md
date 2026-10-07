# M6-AR: army making for Skirmish

Agent AR, branch `m6-ar`. Ground truth: the 2016 HD `PANZERS.exe` (static analysis only).

## Flow (HD) and what is lifted

| Step | HD | Where |
|---|---|---|
| Room New (+0x3c8) sends **0x534b3**, Edit (+0x43c) sends **0x534b4** (the recompile had the two codes swapped) | SSkirmishChatRoomMenu::OnAction 0x654df0 | skirmish.h |
| 0x534b3: room hidden, ClearArmy 0x591d50 (army, ArmyName, ArmyFileName cleared), SetRace(room +0x123c), LoadNextCampaignView → case 1 (MenuToLoad is still 1): the market. 0x534b4: the same without ClearArmy (the army LoadCurrentArmy bought stays) | SSuperWindow::OnAction 0x659250, 0x658b10 | superwindow_sk.cpp |
| Market in multi mode: title Headquarters, Buy / **Done** / **Cancel** (0x64c0d0 at x 0x100 / 0x200 / 0x300), the tank limit 8 (early) / 10 (late), the warehouse by the unit's age (+0x28: 0 any, 1 early, 2 late), Prestige = the prestige limit | SMarket ctor 0x63f2f0, Create 0x6407d0, Available 0x644fc0 | market.cpp |
| Done → the **Save Army** dialog ("Army Name:", the army's name; empty for New); OK: the army into the campaign (SetArmy 0x5971b0), same name → the same file again (GetArmyFileName), another name → a new file; **SaveArmy**; 0x4d542. Cancel hides it. Cancel button → the **Leave Market** box ("Are you sure, you want to leave the market without saving?"), Yes → 0x4d541 | OnAction 0x647d00 | market.cpp |
| The 3D preview ends at the top of a shown box | 0x64afb0 | market.cpp (SetPreviewHeight) |
| SaveArmy: `armies/<G|A|R><e|l><SP>-<time>.army`, signature, chunk ARMY v5, the army name, CRC32 of the body, the body (race, SP float, records 0x5cfbf0) xor-coded from its end. Coop branch (AREC v2, `armies/<R>-<map>-<time>.army`) lifted too; the GameSpy deck branch (game type 4) logged | SMarket::SaveArmy 0x64a180 | market.cpp |
| Panzers' input dialog: message-box frame, title, SEditBoxWithText (label + pz::SEditBox), OK / Cancel or Yes / No; Enter / Esc; results 0x49581..0x49584 to the parents | SInputDialog 0x53b3a0 / 0x53b5c0 / 0x53b880 / 0x53b910 / 0x53ba60 / 0x53b480, SEditBoxWithText 0x53b140 / 0x53b200 / 0x53b2d0 | inputdialog_pz.cpp (new) |
| 0x4d542 / 0x4d541 with the skirmish room: ReleaseMultiView, room +0x538 = 1, room visible, after a save LoadArmyNames (the new army is selected by GetArmyName) | 0x659250 | superwindow_sk.cpp |
| Campaign +0xe8 ArmyName / +0xf0 ArmyFileName with GetArmyName 0x591e00, GetArmyFileName 0x591dd0, SetArmyName 0x597150, SetArmyFileName 0x597120, IsMultiMode 0x594d20 (the room kept a copy before) | | campaign.h, campaign_multi.cpp |

Delete army (with the "Are you sure" box) was already lifted by M5-SK.

**HD bug kept:** New / Edit set the local slot to ready 2 ("in the market", 0x51e270(1)); HD's
skirmish-room branch of 0x4d541 / 0x4d542 does not reset it (the multiplayer room's branch does,
0x51e270(0)), and I'm Ready toggles `ready = (ready == 0)` (0x51e2b0), so after the market the first
I'm Ready click only makes the slot not ready and a second one is needed. `PANZERS_MOD_BUGFIXES`
resets it.

`PZ_M5_SK_ARMY` is no longer needed for a skirmish; the hook stays for scripted tests.

## Not lifted

- The vehicle menu (SMarketVehicleMenu 0x648c20, "Change vehicle"): unchanged, logged.
- The GameSpy deck modes (campaign +0x11c, SMulti game type 4, market Mode 1) and the multiplayer
  connection polling of SMarket::Update 0x64b380 (nothing to poll offline).

## Verification (our build, `-nointro -m3`, run folder `scratchpad\m6ar\run`, shots in `scratchpad\m6ar\shots`)

- `a05_market.png`: New army → the market (Done disabled, 0/8 tanks, Prestige 2000, early German tanks).
- `d02_name.png`: two Panzer IV D + Riflemen bought, Done → Save Army "Second Army".
- `d03_room.png` / `a10_room.png`: back in the room, the army listed and selected; file
  `armies/Ge2000-<time>.army` written (ARMY v5) and read back by LoadArmyNames / LoadCurrentArmy.
- `c03_save.png`: Edit army → the market with the army, the name proposed; OK overwrites the same file.
- `d04_ready.png` → Start Game → `g01_unit1.png`: in the game (airport) the two Panzer IV D and the
  Riflemen squad of the army (N key = next own unit); `SKIRMISH_AI 4` attacks as before.
- `h01_leave.png`: Cancel → Leave Market box; Yes → the room. `h04_deleted.png`: Delete → Yes → the
  file is gone, the list is empty.

Regression (`coord\regress.ps1`): menu 0 / 1181 mismatches, tc1 3901 / 3901, tut1 2012 / 2012, clean exits.

Census: 2531 → 2546 lifted, 1316 SWINE-shared (unchanged), 139 stubs (unchanged count; the two
replaced STUB_LOGs were inline).

## Reference to record from the original (coordinator)

Single player → Skirmish → Lock Settings → New → buy a unit → Done → name → OK → back in the room:
does the first I'm Ready click make the player ready, or (as the code reads) only the second?
