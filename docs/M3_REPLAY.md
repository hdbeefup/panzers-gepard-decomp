# M3 replay oracle: deterministic record and replay in the original

M3-P0, 2026-10-06. All addresses are HD `PANZERS.exe`. The saved references are in the scratchpad
folder `m3ref\` (README there); M3 agents do not run the original themselves.

## Verdict

**Yes: the original replays its own `-packetrec` recording deterministically, if the replay starts
from the same army.**

| Run | Start | Result |
|---|---|---|
| Record (`tc1.rec`) | `-packetrec Replays\tc1.rec`, Training Camp, German, buy Panzer III F + Riflemen, Start Mission, 3 min 15 s of play with combat, End Mission | native CRC trace `tc1_crc_ref.txt`, frames 0..3902 |
| Replay A (naive) | `-packetplay Replays\tc1.rec`, Training Camp, German, Start | **diverges at frame 0**: 346 live units instead of 358; 1,619 "Inconsistency in frame N with player 0." log lines (frames 1, 2, 3 ...) |
| Replay B (recipe below) | `-packetplay`, PacketPlay byte cleared until the market, same army bought, byte set before Start Mission | **identical CRC, seed and unit count on every frame compared (0..2198, the run was stopped at 420 s)**; 0 "Inconsistency" lines. Units died in the same frames (358 -> 351 by frame 1560 in both). |

The CRC compared is the one SGameLogic::BeginFrame 0x571840 computes (0x56aa10: rotl-xor of
unit +0xfc, +0x8c, +0x90, +0x94, +0xb0, +0x114, +0x108, +0x1dc over the live units, then
World+0x7518), read by `ocrc3.py` from memory between logic ticks.

## Why the scope's replay diverged at frame 1 (not the time seed)

1. **The "time seed" is a clock.** Mission start 0x6281a0 writes `ftol(timer)` into SGameView
   +0x45c / +0x460. They are the view's millisecond clock (Update 0x628430 adds 0x32 = 50 ms per
   logic frame; ResetClock 0x624730 rewrites them). They are not a random seed.
2. **The world RNG is deterministic.** World+0x7518 is set to 0 by the SWorld ctor 0x5d2f90 and
   only advanced by the inline LCG (x * 0x343fd + 0x269ec3) in logic code (PlaceAllUnits, squads,
   drivers, AI 0x5f5c70, SFlyingUnit, ...). SGameLogic::LoadGameState 0x56eb50 and the cut-scene
   logic (0x588820, 0x589d90, 0x58c6d0) are the only other writers.
3. **The army.** `-packetplay` given on the command line makes SGameView::LoadMap 0x6201c0 skip the
   loading screen *and the market* (`if (campaign +0xdc == 0 || PacketPlay || ...) MissionStart`),
   so PlaceAllUnits 0x571c70 places no bought army. The recorded header (with the army) is read by
   StartPacketPlayback 0x580540 -> 0x595780 only *after* PlaceAllUnits, inside mission start. The
   world differs from frame 0 (346 vs 358 units) and every frame's CRC check fails.
4. The intended path, SPanzersCampaign::StartReplay 0x597510 (reads `Replays/<name>`, restores
   mode / race / prestige / army before the map is loaded, then sets PacketPlay), is reached only
   from SSuperWindow::OnAction action **0x494c2**, and **nothing in the exe sends 0x494c2** (the only
   occurrence of the dword is the compare at 0x659d4b; SLoadMenu::LoadReplayNames 0x5959d0 has no
   callers). The Load Game screen lists saves only (`m3ref\shots\rec_load_menu.png`). The replay UI
   was cut from the shipped game.
5. The command-line map start (`-map maps/training.map`, Play 0x65b470 -> InitScenarioMode
   0x5944d0) panics in the original before the mission: `SHeap<struct SFrame>::operator[]: invalid
   index (1835008)` after loading the Russian planes. Not usable.

## Recipe (original, scratch copy only)

Switches (SSettings 0x929cf8, parsed by 0x64e330): `-packetrec <file>` sets +0x34 file and byte
+0x3c (0x929d34); `-packetplay <file>` sets the file, +0x3c = 0 and +0x3d = 1 (word 0x100;
**0x929d35 = PacketPlay**). Mission start 0x6281a0: if 0x929d34, StartPacketRecording 0x5805c0; if
0x929d35, StartPacketPlayback 0x580540 (or the campaign's replay name when +0x130 != 0).

Record:
1. `PZORIG.exe -packetrec Replays\X.rec` (path relative to the run folder).
2. Training Camp -> nation -> Start -> click the loading screen -> buy the army (note the order) ->
   Start Mission -> Yes -> play -> Esc -> End Mission -> Yes.

Replay:
1. `PZORIG.exe -packetplay Replays\X.rec`.
2. Right after launch (before the Training Camp Start), clear PacketPlay: `poke.py <pid> 929d35 00`.
   LoadMap then shows the loading screen and the market as in the recording.
3. Training Camp -> the same nation -> Start -> click -> buy **the same units in the same order** ->
   Start Mission.
4. Before clicking Yes, set PacketPlay: `poke.py <pid> 929d35 01`. Yes -> mission start starts the
   playback; it replays the recorded frames (orders, game speed SGameLogic+0x04, camera).
5. Trace it with `ocrc3.py` and compare with `mcmp.py`, and grep the game log for "Inconsistency".

Saved: `m3ref\tc1.rec`, `m3ref\tc1_crc_ref.txt` (frames 0..3902), `m3ref\playB_crc.txt`.

## What the recording holds (for our own playback)

- Header (SPanzersCampaign::WriteReplayHeader 0x596fc0, version 3): `SAVE`, `v4pa`, 3, map name,
  GameMode, Race, StartPrestige, army count + army records (0x5cfbf0, 0x80 bytes each), Prestige,
  the mission section, +0xe4, Difficulty, +0xf8, then 12 player records of 5 dwords (World+0x19c +
  i*0x48).
- Per logic frame and player (SGameLogic::ProcessPacket 0x5737c0, recording stream +0x1b0): int size,
  int SGameLogic+0x04 (game speed), the frame stream (int CRC, then packet records, packets.h), and
  20 bytes of SWorld camera state (GetCameraState 0x5e6a70: CamTarget x / z, yaw, +0x50, CamDist).
- On playback (+0x1b4) the frame is read from the file instead of the local queue, +0x04 is
  restored, and the switch at 0x5739b8 applies the records. The recorded CRC is compared with the
  local CrcHistory and every mismatch logs "Inconsistency in frame %d with player %d.".

## The packets of tc1.rec (agent O)

`recdump` (src/tools/recdump, built with the game) parses a recording with the game's own frame
reader and the record table of `src/game/packets_rec.cpp`, prints every record and writes the file
back field by field: `recdump m3ref\tc1.rec dump.txt out.rec`. tc1.rec comes back **byte-identical**
(142,476 bytes), so the frame layout and the field layout of every op used are right.

- Header: version byte 3, then the Stormregion signature and the 'SAVE' chunk (1,961 bytes).
- 4,255 frame records: 3,902 with Running 1 and 353 paused ones (Running 0: records 2198..2258, the
  Space pause, and 3963..4254 after Esc). Record 0 carries the map-load CRC (`6365f6b2`, 346 units);
  record n (n >= 1, before the pause) carries the CRC of logic frame n - 1. A frame without orders is
  5 bytes (CRC + end byte).
- Only 8 records hold orders, 13 records in all; frame = SGameLogic +0x08 when they apply, i.e. the
  first `PZM2 CRC <frame>` line that sees them:

| Frame | Records |
|---|---|
| 402 | `32` select 346 (the Panzer III F) |
| 416 | `02` move (B1 0, B2 0, 137.94, 151.47) units [346] |
| 449 | `33` deselect 346, `32` select 352 (the riflemen squad) |
| 462 | `02` move (0, 0, 141.80, 150.60) units [352] |
| 642 | `32` select 346 |
| 654 | `02` move (0, 0, 93.09, 86.00) units [346] (the minimap right click) |
| 1391 | `20` support 0x567d40 (dir 0, flag 0, 121.26, 57.72): an aircraft call with a heading (the README names a recon plane and paratroopers) |
| 2719 | `1c` support 0x5674c0 (121.47, 55.90): 16 "Projectile cannonade" units around the point (world RNG, 2 draws each) |

  Selection itself is a packet: 0x32 / 0x33 set or clear unit +0x108 bit `player` (a world-CRC field)
  and call unit +0x148 / +0x14c. Pause and speed are not packets (SetRunning, recorded as Running).
  Note that 352 is never deselected by a packet although the move at 654 has only 346: its +0x108 bit
  stays set in HD too.
- **HD stops at the first paused record.** Playback restores Running = 0 from the record, and Refresh
  0x576d80 calls ProcessPacket only when `PacketPlay == 0 || Running != 0`, so the replay freezes at
  frame 2198 until the player unpauses, once per recorded paused frame (61 times here). That is why
  Replay B of the original ends at frame 2198. Our build does the same; the recompile test hook
  `PZ_M3_REPLAY_PAUSED=1` keeps reading through the paused records, as the recording session did.
- HD quirks kept: op 0x22's builder 0x5759c0 writes (B, i32) but ProcessPacket reads (B, f32, f32);
  op 0x30 has a builder (0x575d20) but no case (Panic "Invalid command packet"); ops 0x1f / 0x20 write
  the flag as a float and read it as an int (then compare the converted value with 0.0).
- Our own recording and playback (`-m3 -packetrec` / `-packetplay`, without F's replay header):
  box select of 5 units, a right-click move and a deselect replayed with identical CRC / seed /
  unit count on all 1,349 frames and no "Inconsistency" line.

## Our build: the command (agent F, works since 2026-10-06)

```
copy m3ref\tc1.rec <run>\Replays\tc1.rec
set PZ_M2_CRC=1
PZM3F.exe -nointro -m3 -packetplay Replays\tc1.rec
```

With `-m3`, the first main menu (SSuperWindow::LoadMainMenu -> `M3OnMainMenu`, superwindow_m3.cpp)
sends HD's dead action 0x494c2 with the file name minus `Replays/`: a new game view and campaign,
`SPanzersCampaign::StartReplay` 0x597510 reads the header (mode, race, prestige, the bought army
into MissionArmy +0x3c) before the map loads, PacketPlay is set, `SGameView::LoadMap` starts the
mission at once, `PlaceAllUnits` places the recorded army, and `StartPacketPlayback` 0x580540 opens
the file for the frame records (SGameLogic +0x1b4, for agent O's ProcessPacket) after
`ReadReplayHeader` 0x595780. The log then has the map-load frame 0 (346 units), `PZM3: mission
start`, and the mission frames from frame 0 (358 units). Compare the lines after `PZM3: mission
start` with `tc1_crc_ref.txt` (`m3f\tools\mdiff.py <log>`).

Deviation: HD's StartReplay reads the map name right after `v4pa` and so misreads the version dword
3 of every file -packetrec writes (and reads the player records one dword short); the recompile
skips the version dword. `PZ_M3_REPLAY_HD=1` keeps HD's read (which fails on tc1.rec).

Status: every frame 0..3900 equal to the original (M3-I2, docs/M3_STATUS.md).

## How our build uses the oracle

1. **Start state.** Agent F lifts StartReplay 0x597510 / ReadReplayHeader 0x595780 and wires the
   recompile's `-packetplay <file>` to it (recompile-only shortcut: the HD action 0x494c2 path):
   header -> campaign (race, prestige, army) -> LoadMap without market -> mission start ->
   PlaceAllUnits with the recorded army -> playback. That gives the same frame 0 as the recording
   (358 units) without any UI.
2. **Per frame.** Our ProcessPacket (agent O) applies the recorded records at their frames and logs
   its own "Inconsistency" lines; `PZ_M2_CRC=1` logs `PZM2 CRC <frame> <crc> <seed> <units>`.
   Compare with `m3ref\tc1_crc_ref.txt` (`crcdiff.py <our log> tc1_crc_ref.txt`). The first
   differing frame is the next bug; the unit count and seed columns say whether it is a create /
   remove (F, C), an RNG draw (whoever drew), or a field (C, O).
3. **Before combat.** Up to the first shot the CRC depends on placement, drivers and orders only;
   C's work shows up as the first divergence after the first shot.

## Exactness and the fallback

- The packet replay is tick-exact: orders are applied by frame number, not by time, and the camera
  and game speed are in the stream. Replay B matched on 2,199 consecutive frames with combat.
- It depends on the army and nation being the same and bought in the same order (the heap slot
  order of the bought units is part of the CRC).
- CRT `rand()` is seeded with `time(0)` at WinMain 0x64c942 (`srand` 0x78c867) and differs between
  runs; the replay still matched, so no CRT draw in this session reached the CRC fields (see
  docs/M3_INTERFACES.md §7 for the callers to watch).
- **Fallback** (not needed now): `-eventrec <file>` / `-eventplay <file>` (SWindow::RecordEvents
  0x544e60 / PlaybackEvents 0x544d60) replay window input per *event frame*. They are frame-rate
  dependent, so they reproduce the UI flow but not tick-exact logic: use them for screen paths
  (menus, market, HUD), with the native CRC trace only as a loose check.

## Not verified

- ~~Replay B was compared up to frame 2198 of 3902~~ M3-I2's deep capture (recipe B, Space pressed once
  per paused record after 2198) matched the recording on every frame 0..3900 (`m3ref	c1_crc_bf_full.txt`).
- One recording, one nation (German), one army. Other nations and longer sessions are untested.
- What SGameLogic+0x04 does on playback beyond the game speed, and the meaning of SWorld +0x50 in
  the camera block.
- The SGameView +0x45c / +0x460 clock reading comes from the disassembly of 0x628430 (the +0x32
  step), not from a run.
