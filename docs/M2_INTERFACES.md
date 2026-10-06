# M2 game logic: interfaces, switch and ownership

This is the contract between the four agents that make the menu background's game logic run: the convoys created by triggers and driven along a path, and the infantry that march and are teleported back.

Read these first:
- `docs/re/M2_SCOPE.md`: the classes and the decoded `menu.map` triggers.
- `docs/re/M2_COVERAGE.md`: what the original executes, and the work list `docs/re/m2_coverage.tsv`.

All addresses are HD `PANZERS.exe` addresses.

## 1. Quick start

| What | How |
|---|---|
| Default build and run | `panzers.exe -nointro`. The M2 path is **off**, and SGameLogic::Refresh runs the M1 model-only tick exactly as before. |
| M2 path | `panzers.exe -nointro -m2` (sets the switch and the trace), or `PZ_M2=1`. `PZ_M2_TRACE=0/1` overrides the trace. |
| CRC log | `PZ_M2_CRC=1` logs `PZM2 CRC <frame> <crc> <seed> <units>` every tick (§8). |
| Log lines | `PZM2: -m2 on, ...` once (SGameLogic ctor), then `PZM2 t<tick>: <Class::Name (0xADDR)>` on ticks 0, 1, 2 and every 200th tick, at most 4 per call site per tick. Each stub also logs `STUB: <name> called` once. |

- `-m2` is removed from argv before SSettings parses the command line, like `-menu3d`.
- The switch needs the menu world: `PZ_MENU_WORLD` is on by default.

Example, `-nointro -m2` (run on m2-p0, 25 s, no crash, 400+ ticks):
```
<0> PZM2: -m2 on, SGameLogic::Refresh runs the M2 skeleton (trace on, crc off)
<0> STUB: SGameLogic::Refresh (0x576d80) called (not implemented)
<0> PZM2 t0: SGameLogic::RefreshM2 (0x576d80 single-player path)
<0> PZM2 t0: SGameLogic::Tick_578b00 (0x578b00)
...
<0> PZM2 t0: SGameLogic::DispatchEverySecond (0x570cc0)
<0> PZM2 t0: SGameLogic::RunTriggers (0x579ab0)
...
<0> PZM2 t400: SGameLogic::Tick_5822a0 (0x5822a0)
```

## 2. Interfaces (`src/game/`, namespace `pz`)

- Declared in HD vtable order, generated from the RTTI vftables.
- Each slot comment gives the offset, the HD implementation, the argument dword count (from `RET imm16`; a double counts as 2) and `[menu: ...]` when the coverage trace ran that slot (with `via Class 0xADDR` when only an override ran).
- **Only slots hit in the menu are named.** `(name guessed)` marks names that rest on the decompiled body or one caller. Everything else is a `Slot_XX` that keeps the order.
- Each header ends with the override table of every subclass.

| Header | Interface | HD vftable | Slots | Named | Skeleton (logged stubs) | Owner of names / bodies |
|---|---|---|---:|---:|---|---|
| `iunit.h` | `SIUnit`, `SIPanzersSquadUnit` | SUnit 0x7fcc24; SPanzersSquadUnit 0x7f9e3c; overrides: SSingleUnit 0x7fb2a4, SBuildingUnit 0x7f4184, SPanzersSquadMemberUnit 0x7f9a40, SFlyingUnit 0x7f5b74, SProjectileUnit 0x7fa5b8, STrainUnit 0x7fb780, SWasterUnit 0x7ffc74 | 115 (+1) | 41 (+1) | `unit.*` `SUnit` | U |
| `punit.h` | `SIPUnit` (+ skeleton `SPUnit`) | SPUnit 0x7fa7c0 (+8 SP*Unit) | 6 | 4 | `punit.cpp` | U |
| `idriver.h` | `SIDriver`, `SIPDriver` | SDriver 0x7f49e0 (+12 driver classes); SPDriver 0x7f4998 (+12) | 21 / 5 | 17 / 3 | `driver.*` `SDriver`, `SPDriver` | P |
| `iunitanim.h` | `SIUnitAnimation`, `SIPUnitAnimation` | SUnitAnimation 0x7fd7e4 (+10); SPUnitAnimation 0x7fd724 (+9) | 17 / 5 | 8 / 3 | `unitanim.*` | A |
| `gamelogic.h` | `SGameLogic` (no vtable) | n/a, 0x318 bytes | | 45 members | `gamelogic.cpp`, `triggers.cpp`, `movementgroup.cpp` | L |
| `src/world/trigger.h` | STrigger 0x2c, STriggerEvent 8, STriggerCondition 0x38, STriggerAction 0x64, SRunningTrigger 0x34 | loaders 0x5f0140, 0x5b2400, 0x5b22b0, 0x5b2010 | | | `src/world/trigger.cpp` | L |
| `m2common.h` | HD sizes `kHdSize*`, `g_M2`, `PZ_M2_TRACE` | | | | `m2trace.cpp` | P0 |

**Sizes.** `PZ_HD_SIZE` asserts the known sizes now: SGameLogic and the trigger structs. Every concrete unit, driver and animation size is in `m2common.h`; assert it when you add the class. The base sizes of SUnit, SDriver and SUnitAnimation are not known (they are at most the smallest derived class). The skeleton bases therefore carry no size and no data. Add fields at their HD offsets when you lift them.

**The M1 stand-in unit** is renamed `pz::SMenuUnit` (`src/world/pzunitregistry.h`), so `pz::SUnit` is the real class. U replaces it, together with `SUnitType` (stand-in for SPUnit) and the SHeapTRB element type in `world.h`.

**World fields added:**
- `SWorld::Triggers` +0x7474 (`SHdArray<STrigger>`);
- `SWorld::RandomSeed` +0x7518;
- `SUnitHeap::Frame` (+0x4ec) is what Refresh writes `frame` into.

## 3. Tick order and the wiring

`SSuperWindow::OnIdle` is unchanged. The OnIdle wiring (`Refresh` per 0.05 s, then interpolation) was already in place from M1; see `MENU3D_INTERFACES.md` §3.

- `SGameLogic::Refresh` (`src/game/gamelogic.cpp`):
  - without the switch, it does exactly what M1 did (`g_World->RefreshModels()`);
  - with `g_M2.Enabled`, it calls `RefreshM2()`.
- `RefreshM2()` is the skeleton of the HD single-player tick, in HD order (`gamelogic.h` header and `M2_COVERAGE.md` §3). It calls the stubs of L, then `g_World->RefreshModels()` (the M1 visuals), then increments `Frame` and the trace tick.
- The per-unit loops (+0x16c, +0x2c(frame), +0x3c) are left out until U's units implement `SIUnit`. The stand-ins do not. **L adds the loops when U lands.**

## 4. Ownership

| Agent | Owns (writes) | Implements (work list = the `owner` column of `m2_coverage.tsv`, loop + startup rows) |
|---|---|---|
| **L** logic + triggers | `src/game/gamelogic.cpp`, `triggers*`, `movementgroup*`, `src/world/trigger.cpp` | SGameLogic ctor/dtor/Refresh (SP path), BeginFrame + world CRC, event dispatch 0/2/3, UpdateActiveLocations, CheckConditions (2, 3, 5, 6), RunTriggers (cases 1, 2, 3, 8, 0xa, 0x1e, 0x1f, 0x26, 0x29; others logged), orders 0x57efd0/0x56ff30, movement groups and convoy, UpdateUnitVisuals, CanSeeGroundUnit, the TRIG/TVAR/LOCS/PATH loaders (wire LoadTriggers into `mapload.cpp`'s TRIG case). SWorld rows: 0x604620, UpdateSpeech 0x607f50, SetTriggerVariableValue 0x5fec80, 0x5ec490. |
| **U** units | `src/game/unit*`, `punit*`, new `singleunit*`, `squadunit*`, `buildingunit*`, `gunner*`; `src/world/unit.cpp`, `unitregistry.cpp`, `pzunitregistry.h` | SUnit base and the 4 menu classes, SP*Unit from `.unit`, SGunner as needed by ServerRefresh, and replacing SMenuUnit/SUnitType. SWorld rows: CreateUnit 0x5e2da0/0x5e3170, FindEmptySpace 0x5e5700/0x5e58d0, AllocUnitSlot 0x5d94b0, RemoveUnit 0x5f8060 (M1), 0x5f7920, 0x5f7220, 0x5e3c80, 0x5e4870, 0x5e4e00, 0x5ef760. |
| **P** drivers + path finding | `src/game/driver*`, `manoeuvre*`, `target*`; `src/world/astar*`, `blockmap*`, `pathnodes*` | SDriver base + TurnInPlace, TurnInAngle, Walker, PanzersSquad, PanzersSquadMember; SPDriver; ghost frames; SWayPointWithManoeuvres; STarget (ctor 0x5b27c0, ConsumePath 0x5b7c50, Refresh 0x5bd210); SAStar/SHeapList. SWorld rows: block map 0x5d8670..0x5da050 (CheckStaticBlockMapInternal), 0x5ddbc0, 0x5f4430/0x5f4720, GetGlobalAStar/GetLocalAStar, 0x6007d0/0x600870, road/path nodes. |
| **A** unit animation | `src/game/unitanim*` | SVehicleAnimation (InitModel, UpdateModel 0x5cd020), SWalkerAnimation (0x5ca110, UpdateModel 0x5ce2a0), SSquadAnimation 0x5cc8c0, SBuildingAnimation 0x5cb650, the SP*Animation factories, SRunningGear 0x5a9cc0..0x5aa900. Takes over `PlaceUnitModel`/`RefreshModel` from `src/world/unit.cpp` together with U. |
| **P0** (shared) | `src/game/i*.h`, `m2common.h`, `m2trace.cpp`, `gamelogic.h`, `src/world/trigger.h`, `src/game/CMakeLists.txt`, `tools/census.py`, this file | |

- `world.h` stays shared: each owner names its own fields there, additively, at their HD offsets.
- `SWorld::ComputeCamera` and the weather code are M1-lifted and nobody owns them in M2.

## 5. Cross-agent call points

| Caller | Callee | How |
|---|---|---|
| L (Refresh) | U | Per unit: `SIUnit` +0x16c `StoreInterpolationState`, +0x2c `ServerRefresh(frame)`, +0x3c `RefreshModel`. Per frame: +0x40 `UpdateVisuals(vp)`. |
| L (RunTriggers) | U | CREATE: `SWorld::CreateUnit` (U) then +0x50 `Place`. Teleport: +0x24 `SetPosition`. Remove: `SWorld::RemoveUnit` -> +0x04 `Uninit`, +0x70 `Remove`. Orders: +0xac `EC_Move`, +0xb4 `EC_MoveAlongPath` (convoy, path), +0xa0 `SetCurrentTarget`. |
| L (CheckConditions, groups) | U | Unit fields via accessors U provides (player +0xfc, pos +0x8c). |
| U | P | The unit owns its drivers (+0x38 array, active index +0x28). It calls `SIDriver` +0x14 `Refresh`, +0x30 `FindGlobalPath`, +0x40/+0x44 `Ghost_*`. Drivers are created through `SIPDriver` +0x10 `CreateDriver(unit)`. |
| P | U | Drivers read and write the unit through `SIUnit` and U's field accessors: +0x1b0 `GetMoveSpeed(-1)`, +0x198..+0x1a8 block-map slots, GhostFrames (unit SDEQueue, slot +0x74 GhostFrames_AddTop). |
| P | L | `SGameLogic::GetMovementGroupMoveSpeed` 0x56b010 (driver +0x48). |
| U | A | The unit keeps the animation at +0x14. It calls `SIUnitAnimation` +0x08 `UpdateModel` (from +0x3c), +0x3c `GetAttachNode` (from +0x50), and +0x1c `GetStateTurnSpeed` (driver +0x4c). Animations are created via `SIPUnitAnimation` +0x10. |
| A | engine | Models through `pz/imodel.h` (M1). |
| all | world | `pz::g_World`, `g_GameLogic` (`worldapi.h`). Terrain height `SWorld::GetTerrainHeight` 0x5e7730 (M1). |

Use only these interfaces and the named non-virtual SWorld/SGameLogic functions. If you need another agent's function, ask the owner to declare it in their header.

## 6. Rules for the shared headers (same as M1)

1. Never reorder, insert or remove a slot, and never overload a virtual name (MSVC groups overloads). HD functions that share a name get distinct names, for example `Init`/`InitNew` and `SetMovementGroupFormationDir`/`...Dir2`.
2. Naming a `Slot_XX` or fixing a parameter list is allowed only for the agent that owns that interface (§2). Rename in place, keep the `+offset HD address` comment, and update the skeleton in the same commit. Other agents ask the owner.
3. No data members in the interfaces. Data goes in the implementation class at its HD offset, with `PZ_HD_SIZE` on every concrete class. Convert padding into fields; never grow a class.
4. Changes to `m2common.h`, `gamelogic.h` and `trigger.h` go through P0 (or through the integrator after P0) and stay additive. L may name SGameLogic and trigger fields in place.
5. Stubs:
   - Each stub starts with `STUB_LOG("Class::Name (0xADDR)")` and then `PZ_M2_TRACE(...)`.
   - When you lift a body, drop the `STUB_LOG` line, keep the trace, and put `// PANZERS 0xADDR` above it.
   - Code adapted from S.W.I.N.E. gets `// SWINE-adapted, PANZERS 0xADDR`.
   - Never start any other comment line with `// PANZERS 0x`.
   - `tools/census.py` counts the `STUB_LOG` lines in `src/game` and `src/world/trigger*.cpp` as "m2 skeleton".
6. S.W.I.N.E. `world/` and `game/` may be copied and adapted (user permission, M2). Only `world/campaign.*` and the trigger loaders are close enough to be worth it (`M2_SCOPE.md` §4.3). HD is the ground truth. Never commit game data or tracer binaries.

## 7. Gotchas

- **Calling conventions.**
  - Unit, driver and animation slots are `__thiscall`; the argument counts in the slot comments come from `RET n`.
  - Several hot helpers are `__fastcall` in Ghidra but read stack arguments. For example, driver +0x24..+0x2c take a ghost-frame pointer plus 2 values. Trust the pushes and `RET n`, as in M1.
  - 0x5737c0 and 0x5e7960 have no RET that Ghidra found (tail jumps).
- **20 Hz fixed step.** OnIdle adds `0.05f` (the constant at 0x7f4534) to `MenuNextTick` in SSE single precision, and runs `Refresh` while `MenuNextTick < now`. All logic is in ticks:
  - `frame % 20` gives the per-second event;
  - trigger variables count per second, at 20 ticks;
  - the convoy cycle is 90 s, 1,800 ticks.
  Never use wall time in the logic.
- **Interpolation between ticks.**
  - Rendering uses `interpolation = (MenuNextTick - now) * 20` (0..1) through `scene +0x20` and `UpdateUnitVisuals(vp, interp)`.
  - The unit side is +0x16c, which shifts pos +0x8c -> +0x98 -> +0xa4 and dir +0xb0 -> +0xb4 -> +0xb8 every tick, and calls the models' +0x3c StoreInterpolationState.
  - Keep that order: store first (+0x16c), then ServerRefresh moves the unit.
- **FPU control word 0x007F** (24-bit precision, set by the Miles factory 0x684fb0; `MENU3D_INTERFACES.md` §4). Most HD logic is SSE scalar. The hot trig helpers 0x78d640/0x78d480 are x87 (`_CIsin`/`_CIcos` style) and run about 180,000 times per second in the menu. Do not replace them with SSE `sinf`/`cosf`, and do not change the control word, or the CRC test drifts.
- **Unit heap order.** The CRC hashes units in SHeapTRB slot order (World+0x4d4), and the trigger "find units" order follows it too. Allocation must follow HD AllocUnitSlot 0x5d94b0 and its free-list reuse barrier (SUnitHeap Frame/ReuseDelay), or every later tick differs.
- **Random seed.** World+0x7518 changes every tick (§8). Find its generator before lifting anything that uses randomness (formations, FindEmptySpace).
- **Nothing reaches its target in the menu.** `OnDriverReachedTarget` never runs: units are removed or teleported on location entry. Its absence from the trace is expected, not a bug.
- **CREATE of crews.** The jeep's crew is created with `InitNew` (+0x0c) and stored with `StoreUnit` (+0x5c). Squad members also use `InitNew`.

## 8. Determinism test (for later agents)

**The original is deterministic.** Two DynamoRIO runs of the HD menu produced bit-identical per-tick CRCs, seeds and unit positions over 1,132 ticks (`docs/re/M2_COVERAGE.md` §5).

**Test "m2-crc":**
1. Run `panzers.exe -nointro -m2` with `PZ_M2_CRC=1` for 60 s.
2. From the log, take the `PZM2 CRC <frame> <crc> <seed> <units>` lines.
3. Compare them tick by tick with `docs/re/m2_crc_original.txt`. Start at frame 0 (it appears twice there).
4. The first differing tick tells you where to look:
   - the unit count differs: CREATE or Remove is wrong (L, U);
   - the seed differs: the RNG is wrong;
   - only the CRC differs: positions or fields are wrong (P, U).

Until L lifts `ComputeWorldCRC` 0x56aa10, the recompile logs crc 0. The `seed` and `units` fields are placeholders, still 0, until `ComputeWorldCRC` is lifted and the CRC log line in `RefreshM2` is wired to World+0x7518 and the live-unit count.

**Per-unit positions.** The full trace (`U <frame> <idx> <player> <x> <y> <z> <dir>`, raw float bits) is in the scratchpad (`m2p0/crc1/m2crc.19148.txt`), with the DynamoRIO client `m2p0/tools/m2cov/m2crc.c`. To regenerate it:
1. Run `drrun -c m2crc.dll <outdir> -- PANZERS.exe` on a scratch copy, never the install.
2. Use the same `options.ini` (windowed 1024x768) and remove `intro.bik`.

The menu's random seed sequence is reproducible, so no seed override is needed.

**`panzers.ini [Debug] Debug Level`** only sets the logger level (`"debug level"`, read by 0x656d50, applied by 0x65ca80). The only per-frame log line, "Frame %d started with server CRC ...", is printed only when the SMulti object exists (multiplayer). The original cannot be made to log positions without instrumentation.

## 9. Not verified

- Slot names marked "(name guessed)". The parameter lists of named slots are only typed by dword count.
- The `SIDriver` +0x24..+0x2c signatures (the decompiler shows the ghost frame as `this`).
- The SUnit, SDriver and SUnitAnimation base sizes.
- The SRunningTrigger layout.
- The SGameLogic fields beyond the ones asserted.
- The class labels of `~` rows in the coverage TSV.
- A native (non-DynamoRIO) CRC run, compared against the DynamoRIO one.
