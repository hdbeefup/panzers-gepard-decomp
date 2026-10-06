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

- Replay B was compared up to frame 2198 of 3902 (stopped by the trace's time limit), not to the end.
- One recording, one nation (German), one army. Other nations and longer sessions are untested.
- What SGameLogic+0x04 does on playback beyond the game speed, and the meaning of SWorld +0x50 in
  the camera block.
- The SGameView +0x45c / +0x460 clock reading comes from the disassembly of 0x628430 (the +0x32
  step), not from a run.
