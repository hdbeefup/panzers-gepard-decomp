# M3 status

## THE REPLAY ORACLE IS UP: FRAME 0 MATCHES (agent F, 2026-10-06)

Our build replays the original's recording `m3ref\tc1.rec` from the recorded start state:

```
PZM3F.exe -nointro -m3 -packetplay Replays\tc1.rec      (PZ_M2_CRC=1 for the CRC lines)
```

(copy `m3ref\tc1.rec` to `<run>\Replays\tc1.rec`; full recipe in docs/M3_REPLAY.md "Our build").

| Frame | Original (`tc1_crc_mission.txt` / `tc1_crc_ref.txt`) | Ours |
|---|---|---|
| map load frame 0 | `6365f6b2 09e7b075 346` | `6365f6b2 09e7b075 346` |
| mission start frame 0 | `737376f6 141334d6 358` | `737376f6 141334d6 358` |
| frame 1 | `17efc580 141334d6 358` | `68cec0f1 141334d6 358` (differs) |
| frame 3 | `cac2925a 64c3dd55 358` | `88a8237c 64c3dd55 358` (seed equal) |

All 358 units also match the original's own mission-start save (`TRNG-Start.save` of the playB
run: position, direction, player, HP per unit; `m3f\tools\ucmp.py`).

**Frame 1 onward is the next bug**: the first logic tick after mission start (ProcessPacket of the
recorded frames, unit refresh, triggers). The world RNG stays in step through frame 3, so the
difference is a unit field (+0x8c/+0x90/+0x94 position, +0xb0, +0x114, +0x108, +0x1dc), not a draw.

Diff our log against the reference with the lines after `PZM3: mission start` (the map-load frame 0
comes first in the log): `m3f\tools\mdiff.py <our log>` (or `crcdiff.py` on a log cut there).

## What it took (F)

- `SPanzersCampaign::StartReplay` 0x597510 (dead action 0x494c2, wired as a recompile-only test
  switch: `-m3` + `-packetplay Replays\<name>`), `ReadReplayHeader` 0x595780, `WriteReplayHeader`
  0x596fc0, `SGameLogic::StartPacketPlayback` 0x580540 / `StartPacketRecording` 0x5805c0.
  HD's StartReplay does not skip the header's version dword and misreads every v3 file; the
  recompile skips it (`PZ_M3_REPLAY_HD=1` restores HD's read).
- `SGameLogic::PlaceAllUnits` 0x571c70 / `PlaceUnits` 0x572ec0 ("start <n>" location, 4 m grid,
  facing the map centre, FindEmptySpace, camera).
- `SGameView::LoadMap` 0x6201c0 / `MissionStart` 0x6281a0 in HD order (gameview_mission.cpp),
  with the mission SGameLogic ctor arguments and the single Refresh that makes mission frame 0.
- The 15 missing map-load RNG draws: squads stored in buildings (bunkers, towers) at map load.
  `SBuildingUnit::StoreUnit` 0x54d380 + the window points of `SBuildingUnit::Init` (0x548660 /
  0x548560) + the building part of `CanStore` 0x5b7040: each member stands at its window in the
  "stand" state, one world draw each (buildingunit_store.cpp; agent C's area, see the report).

## Frame 1 (for O / C)

- The divergence is in the first tick, before any recorded order (first order at frame 402, O),
  and also without the army: our naive playback (`PZ_M3_NAIVE_PLAY=1`, the original's playA path)
  differs from `playA_crc.txt` at frame 1 too (`ffea0a23` vs our `f1568fb6`, seed equal). So it is
  the first refresh of the map's own units, not the army or the packets.
- In our first tick 230 units change: 192 only their y (+0x90, 0 at placement, terrain height after
  the first refresh), 38 crew / passengers in vehicles get seat-relative x / z / dir. Keeping either
  group (or any one or two fields) at its frame-0 value does not give HD's CRC
  (`m3f\tools\crchyp.py`), so more than one thing differs.
- Stubs that run in that tick: SBuildingUnit::RefreshMisc 0x54b910 (occupants at the windows; now
  that buildings hold squads), SSingleUnit::RefreshMisc 0x5af890 (gun crews), SUnit::FindTarget
  0x5b4720 (gunner tests).
- The training map's triggers: T0 (every second, after 8 s) attack-moves 7 enemy units to
  "start 1" with action 0xe (lifted; not an end of scenario), at frame 140.

## The flow

- Training Camp dialog in the HD layout: German (302,330), Russian (302,365), Allied (302,400),
  Start (413,495), Cancel (609,495), as the m3ref coordinates. Russian is checked at open (HD).
- Two Training Camp round trips without a crash; the default boot menu CRC: 1180 frames, 0
  mismatches.
- Mission end: `PzGameViewEndCheck` (the Update tail) ends the mission on a result 1 / 3 with
  0x47562; the victory / defeat box and results menu are H's.
- Autosave `SaveGames/TRNG-Start.save`: the campaign part equals the original's byte for byte;
  the game state is not written (docs/FORMATS.md).

## Not verified

- Frames after 0 (O: the recorded packets; C: unit refresh). Other recordings and nations.
