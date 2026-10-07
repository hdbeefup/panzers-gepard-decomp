# M5: the rest of the single-player shell, cut-scenes and the campaign sweep

Six parallel parts, merged and checked together. Each part has its own notes in `docs/m5/`.

| part | notes | what works now |
|---|---|---|
| Renderer visual gaps (VX) | `m5/vx.md` | Board elements over units (insignia, rank stars, health bars, group numbers, selection and armour decals), wreck models, heat glow and damage tint, the weapon range ring, grass on mission maps with HD lighting, terrain culling with the horizontal field of view (no tree shadows from off-screen trees) |
| In-game Save / Load (SV) | `m5/sv.md` | Esc → Save (SSaveMenu, Panzers' own SEditBox, overwrite and delete), Esc → Load Game, Load "Before" saves (LoadGameBefore 0x595330), mission names as save titles |
| Skirmish (SK) | `m5/sk.md` | Single player → Skirmish room (SSkirmishChatRoomMenu), start on a multimap against Computer players, Victory / Failure, the skirmish results screen. New / Edit army (the market's army mode) is still a stub |
| Scenario mode (SC) | `m5/sc.md` | Single player → Scenario (SScenarioMenu, `scenarios/*.map`), InitScenarioMode 0x5944d0, the `-map <file>` start, the scenario results screen |
| Cut-scenes (CS) | `m5/cs.md` | Campaign `.ingame` cut-scenes with subtitles, the mp3, the eye cameras and the scripted units (every ExecuteScriptStatement command); the map cut-scenes of trigger action 0x38 (ger-04-3, ger-08-4, ger-11-2). The `.4d` cut-scenes are still stubs |
| Campaign sweep (MS) | `m5/ms.md` | All 30 campaign missions load and run 2400 frames without a panic, crash or Warning box: boats, capturable buildings / hangars / radar / support places, repair and supply trucks, 28 more trigger actions, the market's availability by mission number, the follow-unit camera |

## Bugs found

- **Device reset in missions (also in master b298750).** The Panzers reset hooks were never registered
  (the facade is made before the SWINE Gepard, whose ctor clears the hooks). Any WM_SIZE during a mission
  (a window restore, or Windows un-ghosting the window after a long map load on a busy machine) made
  Reset fail with D3DERR_INVALIDCALL and the game panicked in mesh / texture creation. Fixed in
  `pzgepard.cpp` (M5-MS).
- **HD bugs kept faithful, fixes under `PANZERS_MOD_BUGFIXES`:** the `-map` start leaves the menu world
  loaded and crashes on the first frame (test hook `PZ_M5_MAPFIX=1`); 0x5aaaa0 doesn't reset the decal
  slots after destroying the decals.
- The "There's a PrimaryTarget but no CurrentTarget" Warning is a no-op in HD (0x568ae0 is empty); it
  was a modal box in the recompile and blocked Russian 3 / Russian 6.

## Test hooks (recompile only, inert when unset)

`PZ_M5_MISSION=<missions.ini section>`, `PZ_M5_AUTO=1`, `PZ_M5_FRAMES=n`, `PZ_M5_DIFF`, `PZ_M5_MARKETWAIT`,
`PZ_M5_NOGHOST`, `PZ_M5_WATCHDOG` (m5mission.cpp, see m5/ms.md); `PZ_M5_MAPFIX`, `PZ_M5_CS_FORCE`,
`PZ_M5_SK_ARMY`, `PZ_M5_SK_SAVEARMY`.

## Checks on the merged tree

- Census: 2432 → 2525 lifted, 1316 SWINE-shared (unchanged), 174 → 141 stubs; lifted bodies that still
  contain a STUB_LOG 76 → 62.
- Menu CRC 0 / 1181 mismatches; Training Camp replay 3901 / 3901 frames equal; Tutorial replay 2012 / 2012
  frames equal; clean exits.
- Mission sweep (`scratchpad\m5ms\sweep.ps1`, 2400 frames each): 30 / 30 campaign missions OK, no panic,
  crash, hang or Warning box, and every window closed cleanly.

## Open

- German 4 (seen by agent CS before the MS merge; the merged sweep's German 4 run closed cleanly, so it
  may be fixed by MS's towing lifts, unconfirmed): deleting the world crashed (pure virtual call in `SUnit::Uninit` → `Model->SetVisible` for a
  towed gun, `SUnit::Remove` stores the tower's crew into the gun while g_GameLogic is 0). Blocks the
  real ger-04 map cut-scene and the end of German 4.
- The `.4d` cut-scenes (0x56f0d0, UpdateAnimation 0x582380) need model +0x108 and SModel::Slot_E4 0x6d62e0.
- Army making in the market (0x534b3 / 0x534b4, SaveArmy 0x64a180) for Skirmish; SMultiPreMenu 0x658a30
  after a skirmish.
- SWorld 0x5d68e0 (AI group answering an attack), hit in 2 missions.
- Soldier blob shadows (SModel 0x6dad80), SScene::DrawLines, rain / snow / flare, DrawLakes / Sea.
- Mission victories, the player's own army in combat, and towing in play are not covered by the sweep.
