# M2 coverage trace of the original menu

What the HD `PANZERS.exe` executes while the main-menu scene (`maps/menu.map`)
runs. This list replaces the estimated closure of `M2_SCOPE.md` §2.4
(1,235 functions) as the M2 work list. Table: `m2_coverage.tsv` (this folder).
Determinism reference: `m2_crc_original.txt` (see §5).

## 1. Method

- **Tool.** I used DynamoRIO 11.3.0 (the portable zip from the official GitHub release, unpacked into the scratchpad, with no install and no PATH change). I wrote a small client, `m2cov`, rather than using drcov, because drcov records only which blocks ran and gives no hit counts and no time split.
  - `m2cov` puts an inline 32-bit counter on every basic block of the main module (`drx_insert_counter_update`, keyed by block start).
  - It dumps all counters on every DynamoRIO nudge.
  - The client source is in the scratchpad (`m2p0/tools/m2cov/`). It is not committed, and neither are its binaries.
- **Run.**
  - The run used a scratch copy of the HD exe (identical to the install) in a scratch folder. The paks were hard-linked and `intro.bik` was renamed away, so the Bink intro was skipped.
  - The game was windowed at 1024x768 with `Debug Level = 0` and no input.
  - It ran for 300 s with a nudge every 10 s, then got `WM_CLOSE`.
  - The exe has a relocation table and loaded at 0xed0000 (ASLR), so block offsets were rebased to the Ghidra image base 0x400000.
- **The game runs fine under DynamoRIO.** D3D9, Miles and Bink all work, and the screenshots match the native menu: the convoy drives and the infantry marches. One side effect: the logged FPU word after D3D is 0x037F under DynamoRIO, against 0x007F native. That changes only x87 precision, so it could matter only for x87 maths (see §6). The CRC runs in §5 show the logic is deterministic run to run under DynamoRIO.
- **Mapping.**
  - Each block was mapped to the Ghidra function whose body contains it (19,661 functions, `pyghidra`).
  - A function's count is the count of its entry block. If the entry block never ran, I used the largest count among its blocks.
  - Class labels come from the vtable slots (a function unique to one RTTI class), the `Class::Method` symbols and the `SClass::` strings. Container names (SHeapTRB, SDArray, ...) are ignored.
  - Unlabelled functions inherit the class of the address range they sit in (the ranges of `M2_SCOPE.md` §3.1). The TSV marks these with `~`.
- **Phases.**
  - The first world tick (`SGameLogic::Refresh` 0x576d80) falls between the nudges at 10 s and 20 s.
  - **startup** = functions that ran by the 20 s snapshot and never again.
  - **steady loop** = functions with any hits after it. That window is 280 s, 5,599 ticks, and three full 90 s convoy cycles.
  - The loop window is split at +90 s and +180 s, which gives the columns `w1`, `w2` and `w3` of the TSV.
  - Buckets for loop functions:
    - `>=1000/s`, `>=20/s` and `>=1/s`, if the function was hit in all three windows;
    - `periodic<1/s`, if hit in all three windows but less than once per second;
    - `sporadic`, if not hit in every window.

Caveat: the startup snapshot already contains about 8 s of ticks. One-shot work from the first ticks (the first CREATE of convoy 1, the first moves) therefore counts as startup unless it repeats. It does repeat for the trigger loop, because the convoys respawn every 90 s and the infantry teleport back.

## 2. Counts

| Area (address range) | startup only | steady loop | of the loop: lifted already / stub |
|---|---:|---:|---|
| world/game 0x546000..0x60a000 | 198 (145 not lifted) | **482** (74k insns) | 14 / 4 |
| engine 0x660000..0x700000 | 200 | 540 | 244 / 23 |
| shell/UI 0x540000..0x546000, 0x60a000..0x660000 | 77 | 42 | 14 / 0 |
| libs, net, CRT (below 0x540000, above 0x700000) | 415 | 260 | |
| **total** | **890** | **1,324** | |

That is 2,214 functions in all.

**Steady loop in world/game by owner** (the TSV column `owner`):

| Owner | Functions | Insns | Buckets |
|---|---:|---:|---|
| L logic + triggers | 85 | 16k | 22 at >=20/s, 15 at >=1/s, 41 periodic, 4 sporadic, 3 at >=1000/s |
| U units (+ SGunner, registry) | 207 | 27.5k | 65 at >=20/s, 36 at >=1/s, 91 periodic, 8 at >=1000/s, 7 sporadic |
| P drivers, manoeuvres, STarget, A*, heap list | 113 | 14k | 42 at >=20/s, 20 at >=1/s, 46 periodic, 4 sporadic, 1 at >=1000/s |
| A unit animation (+ running gear) | 39 | 6.5k | 10 at >=20/s, 4 at >=1/s, 21 periodic, 4 sporadic |
| W SWorld (split in `docs/M2_INTERFACES.md` §4) | 37 | 10k | 18 at >=20/s, ... |

**Classes in the steady loop** (labels as in the TSV; a `~` range label is folded into the class):
- SGameLogic 81;
- SUnit and STarget 89;
- SDriver family 60;
- SWorld 37;
- SPanzersSquadUnit 44 (+13 squad-range helpers);
- SBuildingUnit 26;
- SUnitAnimation family 32;
- SWayPointWithManoeuvres 16, plus 7 helpers just below 0x58d4e0 that Ghidra's range puts in "SInGameAnimLogic". They are called exactly as often as CreateManoeuvres (351 times), so they are manoeuvre code.
- SGunner and SPGunner 12;
- SSingleUnit 15;
- SPanzersSquadMemberUnit 10;
- SAStar and SHeapList 11;
- SFlyingUnit-range helpers 4;
- SCampaign 3;
- SP* prototypes 6;
- the registry 2.

**Not executed in the menu**, though the scope's closure counted it:
- SMulti frame sync 0x5212a0;
- AI 0x5f5c70;
- 0x605930 and 0x6090e0;
- CollectActiveLocations 0x5640b0;
- every `OnDriverReachedTarget` (no unit ever reaches its target: the convoys are removed in location 15, the jeep in location 8, and the infantry are teleported on entering location 21);
- TakeDamage and the EC_Attack family;
- SCampaign game flow and the in-game animation logic proper.

## 3. Per-tick order (Refresh 0x576d80, single-player path)

Every tick (5,599 calls in 280 s, so 20 Hz exactly):

1. SWorld::UpdateSpeech 0x607f50, then 0x578b00 and 0x578a70.
2. 0x5737c0 (ProcessPacket), then BeginFrame 0x571840, which computes the world CRC 0x56aa10 and pushes it into a history queue at SGameLogic+0x2c. Then 0x579390.
3. The SWorld refresh 0x604620, then 0x57dfe0, then 0x6088f0 (weather, M1-lifted). Then `World+0x4ec = frame`.
4. DispatchEverySecond 0x570cc0, run when frame % 20 == 0: 280 calls, so once per second.
5. RunTriggers 0x579ab0, then 0x568af0 and 0x565e10.
6. For each live unit, +0x16c StoreInterpolationState, then +0x2c ServerRefresh(frame).
7. Scene +0xf0(2), then RefreshFlyingFox 0x5f6bf0.
8. For each live unit, +0x3c RefreshModel. Then 0x5822a0.

Per frame: UpdateUnitVisuals 0x5638f0, which calls unit +0x40 for each unit and CanSeeGroundUnit 0x562760.

## 4. Classes and vtable slots that ran

**Classes actually instantiated** (their dtors or overrides ran):
- units: SSingleUnit, SBuildingUnit (placed), SPanzersSquadUnit, SPanzersSquadMemberUnit;
- drivers: STurnInPlaceDriver (15 deleted), STurnInAngleDriver (16), SPanzersSquadDriver, SPanzersSquadMemberDriver. SWalkerDriver code ran (the hero is never deleted).
- animations: SVehicleAnimation, SWalkerAnimation, SSquadAnimation, SBuildingAnimation.

**Slots hit**, counting the base class or any override (the slot names in the `src/game/i*.h` headers are exactly these):
- SUnit: 41 of 115 slots, plus SPanzersSquadUnit slot +0x1cc;
- SDriver: 17 of 21;
- SPDriver: 3 of 5;
- SUnitAnimation: 8 of 17;
- SPUnitAnimation: 3 of 5;
- SPUnit: 4 of 6.

Per-class override tables, with the executed ones starred, are at the end of each header.

## 5. Determinism: the HD world CRC

The HD exe computes a sync CRC on every tick, whether or not multiplayer is on: `0x56aa10`, called from BeginFrame 0x571840.
- It starts from `frame`, and for each live unit index i in World+0x4d4 it does `rotl1(c) ^ x` with, in order: `i`, then unit fields +0xfc, +0x8c, +0x90, +0x94, +0xb0, +0x114, +0x108 and +0x1dc.
- It ends with `rotl1(c) ^ World+0x7518` (a random seed that changes every tick).
- It is logged only in multiplayer: `"Frame %d started with server CRC: 0x%08X, RandomSeed: 0x%08X"` at log level 1, under `if (DAT_008f1a74)`. `panzers.ini [Debug] Debug Level` only sets the logger level (0x656d50 -> 0x65ca80, "Log level changed"); it does not make the menu print positions or CRCs.

The scratch DynamoRIO client `m2crc` wraps 0x56aa10. On every tick it logs the frame, the CRC, the seed and the live unit count, plus each unit's index, player, pos (x, y, z) and dir as raw bits.

**Result:** two independent runs of 70 s were identical, all 50,113 log lines bit for bit, 1,132 ticks with 29 to 53 units. The original menu logic is fully deterministic per tick, including the seed sequence. Wall-clock timing does not leak into the logic.

`m2_crc_original.txt` holds the `T <frame> <crc> <seed> <units>` lines of run 1, 1,181 ticks. Frame 0 appears twice. The per-unit lines (3.5 MB) stay in the scratchpad: `m2p0/crc1/m2crc.19148.txt`.

## 6. Notes for the agents

- **Hot CRT trig.** `0x78d640` and `0x78d480` (x87 `_CIsin`/`_CIcos` style, called with the argument in ST0) run 50 million times in 280 s, from ghost steps, formations and the camera. They run under the 0x007F control word (24-bit precision). An SSE `sinf` will not reproduce them bit for bit, so the CRC test will catch that.
- `0x5e7730` (terrain height, M1-lifted) runs about 3,300 times per second, and the block-map tests 0x5d8670..0x5da050 a few hundred times per second each (0x5da050: 470 per second).
- The unit heap order (World+0x4d4 slot indices, with the free-list reuse barrier) is part of the CRC.

## 7. Not verified

- The class labels of `~` rows are range guesses: about 40% of the world/game rows have no symbol, string or unique vtable evidence.
- The startup/loop boundary is a 10 s snapshot, not the first tick (see §1).
- The DynamoRIO FPU control word (0x037F) differs from the native one (0x007F). The coverage set should be unaffected. CRC equality between a native run and a DynamoRIO run was not checked.
- Only the idle menu was traced, with no mouse or menu navigation. The Options and Credits screens are not part of this list.
