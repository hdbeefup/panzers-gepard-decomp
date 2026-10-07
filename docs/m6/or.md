# M6-OR: a campaign reference of the original (Russian 4) and our replay of it

Agent M6-OR, branch `m6-or` (from m6-int). HD addresses throughout.

## The reference (scratchpad `m6ref\`, README there)

One recording session of the original HD exe under the BeginFrame CRC capture (m4t's `ucaptut.py`):
Russian 4 ("Kursk", maps/su-04.map), Normal, no orders, frames 0..1481, `-packetrec Replays\r4.rec`.

- Route: New Game only reaches Russian 1. The original loads a **"Before" save written by our build**
  (`R-04-Before.save`, type 2: our Russian 3 run with a forced victory, results Continue ->
  SaveGameBefore 0x596b30) without complaint: LoadGameBefore 0x595330 -> diary -> briefing -> HQ.
- The HQ disables Start Mission while the army is empty (HD and ours), so the session bought one
  Riflemen squad (prestige 600 -> 560). The recording's header has army 1.
- Files: `r4.rec` (0 order records, 132 paused at the end, recdump round trip byte-identical),
  `r4_crc_mission.txt` (frames 0..1481, 709 units at frame 0), `r4.bin` (deep capture every 10th
  frame), `r4.rng.txt`, the original's log and its own mission-start save `R-04-Start_orig.save`.

## Replay-start plumbing (the only code change)

`-nointro -m3 -packetplay Replays\r4.rec` panicked with `Can't load map: missing.scene`: HD's
StartReplay 0x597510 (the cut Load Replay action 0x494c2) restores mode / race / prestige / army /
section / difficulty but no mission properties, and for GameMode 3 `GetMapName` 0x592040 reads
missions.ini [<section>] "Map". HD's replay screen could therefore never have started a campaign
mission. `LoadReplay` (superwindow_m3.cpp, recompile-only path) now loads them for GameMode 3 as
PrepareMission 0x592d80 does: `LoadMissionProps` 0x593740 and `LoadObjectives` 0x593ba0, keeping the
header's army / prestige / difficulty (log line `PZM6: campaign replay [Russian 4] map maps/su-04.map,
prestige 560, army 1, difficulty 1`). Training Camp / Tutorial replays (mode 5) are untouched.

Check: our build recorded its own Russian 4 session (`-m3 -packetrec`, same UI route) and replayed it:
1238 / 1238 frames equal, 0 "Inconsistency" lines. So the start state and the per-frame playback of a
campaign mission are right in our build (ReadReplayHeader 0x595780 switching GameMode to 5 at mission
start changes nothing visible here).

## Our build against the original (`m6ref\tools\r4chk.py <log> r4_crc_mission.txt`)

    campaign replay: common 1482 differing 1481 first [1, 2, 3] max 1481

Frame 0 equal (CRC c7765827, seed fd5a19f4, 709 units). **First difference: frame 1** (CRC only; the
seed is equal until frame 10, the unit count until frame 98).

Cause (deep capture frame 10, `dccmp.py`): units 367 / 369 / 371 (HD class SPanzersSquadUnit) and
368 / 370 / 372 (their SPanzersSquadMemberUnit) move in HD from the first tick (drivers have global
and local waypoints, speed, ghost queue); ours stand. At frame 0 the triggers "Aknarako3/4/5"
(Hungarian "minelayer") run action 0x39 = OrderAlongPath 0x564510 with order 0x1e, i.e. the unit's
vtable +0x110. HD's SPanzersSquadUnit vtable (0x7f9e3c) has **0x59abf0** there (lay tank mines along
the path); the recompile has no override, so the squads get the empty SUnit::Slot_110 (0x547b80).
Its neighbour +0xb4 (EC_MoveAlongPath for squads, **0x59aef0**) is not lifted either. This is
squad / order code, not the AI lift (0x5d68e0 is first called at frame 743 here) and not the replay.

Also seen at frame 0 (same in a normal campaign start, so a map-load difference, not the replay):
the static block map (World+0x74ec) differs in 18,248 cells over z 18..211 m: ours sets bit 0x200
where HD has none or bit 0x400 (0x2300/0x2100, 0x2200/0x2000, 0x2301/0x2501 ...). Owner: block map
rebuild (blockmaprefresh.cpp).

The AI lift can be checked against this reference only once the mine-laying squads (0x59abf0) match.
The check is therefore not in `regress.ps1`; when it is exact, the 4th line would be:

    $v = RunUntil "-nointro -m3 -packetplay Replays\r4.rec" @{ PZ_M2_CRC = "1"; PZ_M3_REPLAY_PAUSED = "1" } 1481 600
    python "$S\m6ref\tools\r4chk.py" $v[0] "$S\m6ref\r4_crc_mission.txt"

(the run folder needs `Replays\r4.rec` from `m6ref`).

## Census

2555 lifted / 1316 SWINE-shared / 137 stubs before and after (no lift, a recompile-only fix).

## Regression

regress.ps1 on the m6-or exe: menu 0 / 1181 mismatches, tc1 3901 / 3901 equal, Tutorial 2012 / 2012
equal, all three runs exit: closed, no PANIC / EXCEPTION.
