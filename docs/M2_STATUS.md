# M2 status and handoff (2026-10-03)

M2 aims to make the main-menu background run its game logic like the original: triggers spawn two convoys and a jeep, an infantry squad marches, and all of it runs at 20 Hz. All four M2 parts (logic/triggers L, units U, drivers/pathfinding P, animation A) are merged on `menu-3d`. M2 runs behind `-m2` (or `PZ_M2=1`) and is **off by default**, and the default boot is unaffected.

Census at merge: `1357 lifted + 1331 SWINE-shared + 273 stubs (22 shell, 117 menu3d, 134 m2)`.

## What works

| Area | State |
|---|---|
| Trigger data and timing | All 17 menu.map triggers load. CREATE fires at frames 0/1800/3600 (convoy 1 + jeep) and 780/2580 (convoy 2), and the 90 s reset at 1780/3580. This matches the original's log. |
| World CRC 0x56aa10 | Reproduces the original exactly. With `-m2 PZ_M2_CRC=1`, frame 0 logs `cc5c92d8 f628b05d 29`, identical to `docs/re/m2_crc_original.txt`. |
| Units after map load | All 29 units match the original bit for bit (slot order, player, position, direction, random squad formation), and the seed matches. `PZ_M2_UNITDUMP=1` dumps them. |
| Drivers / A* / block map / maths | Lifted. The maths, block map, LOS, FindPath (680/680 on menu.map) and manoeuvres match the HD exe under the Unicorn emulator with 0 mismatches. The driver logic itself was not emulator-checked. |
| Unit animation | Running gear (tracks and wheels), walk cycles, squad and building animation. Checked in modelview only, not yet in game. |

## Open items, in priority order

1. **`-m2` panics during the run:** `SGameLogic::SendConvoyMovementGroupFollowers: Invalid movement group 0`.
   - The convoy leader target hand-over (0x579510 → 0x5bdef0 / 0x5b5a30) is still a logged stub between L and U ("convoy boss target hand-over").
   - P has since lifted STarget refcounting (0x5bdef0 / 0x5b5a30). Wire it into L's `RemoveUnitFromMovementGroup` path and the convoy follower setup.
2. **CRC diverges at frame 1** (ours `7ccd758b`, original `3f01c275`; seed and unit count match).
   - Only the squad members (units 36, 38–41, 46–49) differ. In the original they sit at squad position + offset at frame 1; ours keep raw offsets with ghost count 0, so their drivers never ran.
   - Suspects: squad member placement or registration in U's squad spawn, or the member driver not being attached.
   - Also, seated crews and the M2A1's built-in driver are not attached to their seat nodes (SIModel +0xdc unnamed).
3. **Block-map dirty rect:** replace the guard body of `SWorld::RefreshBlockMapDirtyRect` (triggers.cpp, L) with `BlockMap_RefreshDirtyRect(this)` from `blockmaprefresh.h` (P).
4. **Signature mismatches flagged by P:**
   - `GetMovementGroupUnitFormationPos` takes (index, out), but HD takes (out, unit*).
   - `FormationDir2` stores float bits.
5. **Duplicates to merge into one shared file:** `UnitAnimRand` = `HdRandInt` (0x555a00), and the fast atan 0x661440 exists in both unitanim and drivermath.
6. **Unit hooks still default (U):**
   - WorldUnitMoved 0x5e4870
   - the FindEmptySpace spiral
   - towed unit +0x74
   - OnDriverStucked 0x5bcd20
   - 0x5b6fd0, 0x5b9ce0
   - squad RefreshSquadFormation 0x59dda0
7. **Not lifted:**
   - visibility maps 0x564fb0 / 0x565e10 (CanSeeGroundUnit treats units as visible)
   - ground track marks (SScene +0x8c/+0x90, DrawTrails)
   - damage smoke and death/firing effects (the effect manager crashes on nested effect-prototype loads, so U skips them)
   - ComputeGoal type-3 branch (panic stub)

## How to verify a fix

1. Build with `-DPZ_MENU_WORLD=ON`.
2. Run `panzers.exe -nointro -m2` with `PZ_M2_CRC=1` set.
3. Diff the `PZM2 CRC` lines against `docs/re/m2_crc_original.txt`.

The first differing frame is the next bug. `PZ_M2_UNITDUMP=1` gives per-unit lines in the same format as the original's trace. Use them to find which unit diverges first.

See `docs/M2_INTERFACES.md` for ownership, the interfaces and the gotchas, and `docs/re/M2_COVERAGE.md` for the work list.
