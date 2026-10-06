# M2 status (M2-I integration, 2026-10-06)

M2 makes the main-menu background run its game logic like the original: triggers spawn two convoys and a jeep, an infantry group marches and is teleported back, every 90 s, at 20 Hz. M2-I (branch `m2-integrate`) integrated the four M2 parts, lifted the missing per-tick unit code (with sub-agents UB, SQ, BW, LG, DB, DA, AT) and fixed the integration bugs found by diffing against the original.

**M2 is now on by default.** `-nom2` or `PZ_M2=0` turns it off (the M1 model-only tick runs, as before); `-m2` turns it on with the per-tick trace.

Census: `1488 lifted + 1331 SWINE-shared + 301 stubs (22 shell, 121 menu3d, 158 m2)` (M2 start: `1357 + 1331 + 273 (22, 117, 134)`). The stub count grew because the lifts mark every branch the menu never reaches with a `STUB_LOG`.

## Verified (run and measured)

| What | Result |
|---|---|
| World CRC 0x56aa10, frame by frame | **Identical to the original for every frame compared: 6,199 frames** (0..6200, 310 s, three full 90 s convoy cycles plus 40 s; 2 frames of the original trace not sampled). At M2 start: 1 frame. |
| CRC reference | `docs/re/m2_crc_original.txt` (DynamoRIO, 1,180 frames) and a native run of the original logged by polling its memory between ticks (`ocrc.py`, below); both agree with each other and with the recompile. |
| Stability, M2 on | Two 335 s runs (6,687 frames, 3+ cycles): exit code 0 on WM_CLOSE, no `crash.txt`, no panic. |
| Boot -> Options -> Esc -> Credits -> Esc -> Exit | 3/3 clean with `-m2`, 3/3 clean by default (M2 on), 1/1 clean with `-nom2`; exit code 0, no `crash.txt`, ~129 MB private bytes. |
| Visual | Frame-aligned shots of the original and the recompile at frames 400, 900 and 1400 (~20 s, 45 s, 70 s): convoys, jeep, crews in their seats, the marching squads and the teleport match position for position. Differences: no track marks on the road and no dust (not lifted, below). |

## Fixed in M2-I (root cause, HD addresses)

1. **Vehicles never refreshed their drivers or seats:** `SSingleUnit::RefreshMisc` 0x5af890 was not lifted. Lifted with the attached-unit placement 0x5b02e0 and the model node slots +0x50 / +0x54 / +0x58 (0x6d7d70 / 0x6d7d30 / 0x6d7c60, the logic-pose node matrix of 0x6dc7b0) and the attach slot +0xdc (0x6d5940 / 0x6d7340). Crews, built-in drivers and child units sit on their seat nodes (StoreUnit 0x5c30d0, SSingleUnit::Init 0x5ad9f0). This was the frame-1 CRC divergence (units 36, 38-41, 46-49).
2. **Convoy hand-over** (the "Invalid movement group" item): RemoveUnitFromMovementGroup 0x579510 now hands the boss's primary target to the new boss / last unit with the STarget refcount (0x5bdef0 / 0x5b5a30) and re-sends the followers (0x57e6f0). The panic never occurred in any M2-I run.
3. **Followers drove 82 ghost frames ahead:** the near-unit lists +0x1a8 / +0x1b4 were never filled; SSingleUnit::RefreshTargeting 0x5aef40 / SUnit::FillNearUnits 0x5b78a0 (UB). SUnit::ServerRefresh 0x5bee90 also tested +0x1ac instead of the order queue size +0x1a0.
4. **Model sequence names corrupted** for every model: SPModel::SortSequences 0x693f60 swapped records member-wise and SString::operator= freed the buffer the temporary still pointed at. Swapped raw, as HD does. (Seated soldiers lost "vehicle_move_mg" etc. and drew random numbers HD never draws.)
5. **Animation tick rounding:** all fistp in the unit animations run under control word 0x087f (round up) after a 24-bit x87 product; the recompile rounded to nearest on a wider product (idle timers one tick off).
6. **Global state at map load:** SUnit::Init 0x5ba8e0 calls SetGlobalState with the definition's state (def +0x3c); the recompile passed +0xe0 (0).
7. **State change timing:** SUnit::SetBehavior 0x5b8960 scales by 0.8f (0x7f83dc); SUnitAnimation::StateChangeTicks 0x5c7c80 falls back to the stand sequence's blend time (model +0x88 0x6d8010), not its length.
8. **HD CRT maths shared with the engine:** sin/cos/tan/atan2 ports moved to `src/3dengine/pz/hdmath.*` (used by the SModel node transforms that feed the seat positions), CRT atan 0x78d190 ported bit-exact (AT, 2.5 M inputs, Unicorn and native), fast atan 0x661440 and the LCG of 0x555a00 de-duplicated.
9. **Integration wiring:** g_DriverEnv now calls the real FindEmptySpace 0x5e58d0 / 0x5e5700, UnitMoved 0x5e4870, OnDriverStucked 0x5bcd20, 0x5b6fd0, the aimer 0x5b9ce0, the squad helpers 0x59b990 / 0x59bff0 / 0x5a0450 / 0x5a0060; movement-group orders go through the unit order queue 0x5bb980 / 0x5bbb60 / 0x5bb8a0; 0x5824b0 passes the group's squad state (0x5825a1); the map load rebuilds the whole block map (0x5ef380 + 0x604620); `GetMovementGroupUnitFormationPos` takes (out, unit) and the group formation direction +0x20 is a float.
10. **Sub-agent lifts:** SUnit AI / targeting / orders / effects (UB), SPanzersSquadUnit / member slots (SQ), SBuildingUnit slots, UnitMoved + CrushDoodad, exact FindEmptySpace (BW), visibility maps, CREATE / TELEPORT free space, block-map dirty rect (LG), doodad "Block" footprints (DB), the World+0x158 doodad fall animation (DA).

## Still open

- **Not lifted (visual only, no CRC effect):** ground track marks (SScene +0x8c/+0x90, `DrawTrails`), driver move / departure / water effects (0x55bb80 / 0x55bc50 / 0x55be10: pixie slots untyped), damage smoke and death / firing effects, the board elements (health bars, names), `SSingleUnit::UpdateVisuals` armour decals, sky / sea / lakes / wires / decals draw calls, building eye heights 0x546f70 (engine model +0x100 / +0xd8).
- **CRT `rand()`:** SUnit +0xdc (`RandomSide`, rand()*2 >> 15) differs from HD because HD's CRT rand state depends on all earlier rand() calls. Nothing in the menu reads it; it is not in the CRC.
- **SWorld::UpdateWaterMap 0x608600** at the end of the map load is not lifted (the water map keeps the loaded values; the menu has no lakes).
- Branches marked `STUB_LOG` are not reached in the menu (combat, repair / supply, buildings with occupants, towed units, gun crews of class 0xb).
- "No popping between ticks" was checked on still frames only (positions match the original at the same frames; the interpolation path is the HD one).

## How to verify a change

1. Build (`-DPZ_MENU_WORLD=ON`), run `panzers.exe -nointro` with `PZ_M2_CRC=1` for 60-330 s.
2. Compare the `PZM2 CRC <frame> <crc> <seed> <units>` lines with `docs/re/m2_crc_original.txt`, or with a longer trace of the original.
3. Tools (recompile, env vars): `PZ_M2_UNITDUMP=<n>` per-unit fields for frames 0..n (format of the scratch DynamoRIO client `m2crc2.c`: pos, dir, +0x114, +0x108, ghosts, speed, driver, group, parent, target); `PZ_M2_DRVDUMP=<u,..>` / `PZ_M2_UNITRAW=<u,..>` raw driver / unit dwords per tick; `PZ_M2_RNGLOG=1` every world random draw with its caller; `PZ_M2_ANIMTRACE=<u>` a walker's animation timers.
4. Tools (scratchpad `m2i\tools\`, original exe on a scratch copy only): `ocrc.py` (native CRC trace by memory polling), `hwwatch.py` (hardware data breakpoint on a world / unit field, with the stack's unit indices and return addresses: finds who writes the seed or a unit field), `peekdrv.py` (raw driver / unit dumps of the original), `crcdiff.py`, `tcmp.py`, `udiff2.py`, `firstdiv.py`, `drvdiff.py`.

The first differing frame is the next bug; the unit dump names the unit and field.
