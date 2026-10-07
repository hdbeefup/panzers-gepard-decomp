# Panzers HD decomp: measured completion (master fe92a6a)

This measures master `fe92a6a` (census: 2,238 lifted markers, 79 with a STUB_LOG, 1,317 SWINE-shared
bodies, 189 stubs) against the 2016 HD `PANZERS.exe`. The function list comes from a fresh Ghidra 12
auto-analysis (reccmp's `rc/gproj`), which finds 19,661 functions. The older phase-0 project found 19,985;
the difference is mostly EH funclets and thunks.

## Headline

**Game and engine code is 5,298 functions and 558k instructions, 49% of the exe's instructions.** The other
14.4k functions are libraries and compiler output.

| denominator | functions | instructions | lifted (fn) | lifted (ins) | lifted, no STUB_LOG (fn / ins) | lifted + SWINE, high-confidence (fn / ins) | lifted + SWINE, expected (fn / ins) |
|---|---:|---:|---:|---:|---:|---:|---:|
| all game/engine code | 5,298 | 557,588 | **39.8%** | **54.1%** | 38.4% / 48.3% | 41.0% / 55.1% | **41.9% / 55.6%** |
| reachable at all (SP or MP) | 4,584 | 525,345 | 45.9% | 57.4% | 44.2% / 51.2% | 47.1% / 58.4% | 47.9% / 58.9% |
| statically reachable from single player | 4,242 | 493,531 | **49.6%** | **61.1%** | 47.8% / 54.5% | 50.9% / 62.2% | 51.7% / 62.6% |
| ...the same, minus the MP subsystems | 4,069 | 476,304 | **51.7%** | **63.3%** | 49.8% / 56.5% | 53.0% / 64.4% | 53.9% / 64.9% |
| executed by the original in the Training Camp + tutorial traces | 2,464 | 346,028 | 67.2% | 78.0% | 64.7% / 68.9% | 68.1% / 78.7% | 68.8% / 78.9% |

- **"Lifted"** means the HD function has a `// PANZERS 0xADDR` marker: 2,111 game functions. Of these,
  79 still contain a STUB_LOG (`lifted-partial`), and they hold 32k instructions.
- **"Expected" SWINE coverage** adds the 63 high-confidence pairs at full weight and the 112 fuzzy pairs at
  0.4, their measured precision (see the SWINE section).
- **Upper bound:** suppose every unmarked engine, UI-framework or core function that the original executed is
  in fact served by a SWINE body. Then lifted + SWINE reaches 51% of functions / 65% of instructions over all
  game code, and 65.5% / 75.5% over single player without MP. It can't be higher than that.

**My answer is: about 40% of game functions are lifted, which is about 54% of game instructions. On the
single-player path it is about 50% of functions and 61-63% of instructions.** SWINE-shared bodies add only
1-2 points, because most of them have no Panzers counterpart. Instruction weighting overstates progress by a
few points: partial lifts are counted at the original's full size. Without the 79 partial lifts, it is 48% of
all game instructions.

## Method

1. **Function inventory.** `gdump.py` reads every function from Ghidra: entry, instruction count, body bytes,
   thunk target and namespace. It also reads every reference out of every instruction, and all non-default
   symbols (vftables, FID/CRT names, imports). It ran on the HD exe and on the SWINE HD exe (`swineHD.exe`).
2. **Features.** `feats.py` disassembles each function with capstone within the reccmp/Ghidra bounds. For each
   one it records a normalized-instruction hash, a mnemonic "shape", the referenced strings (ASCII/UTF-16 in
   .rdata/.data), the call targets and large constants.
3. **Library classes** (`classify.py`, `evidence.py`). Each function gets a label from per-function evidence:
   source-path strings, library message strings, Ghidra FID names, RTTI namespaces and 16-byte entry
   alignment. Over address order, these labels form clean contiguous runs, because the linker keeps each
   library's objects together. I set the boundaries by hand at the first and last evidence function of each
   run (table below). EH funclets (`Unwind@`/`Catch@`) and import thunks are their own classes wherever they
   sit. Inside the game ranges, a function whose only strings are STL messages, or that is in `std::`, is
   counted as `stl`.
4. **Markers** (`markers.py`). These are scanned from `src/` on master, with the same regexes as
   `tools/census.py`:
   - 2,238 `// PANZERS` markers give 2,121 distinct addresses. 2,117 of them are Ghidra function entries, and
     2,111 of those are game code.
   - A marker counts as "partial" when a STUB_LOG appears before the next marker, which is census.py's rule.
   - STUB_LOG lines that cite an HD address within 3 lines mark that address as `stub-cited`.
5. **SWINE-shared → Panzers pairing** (`swmatch.py`, `swmatch2.py`, `swmatch3.py`). The 1,316 distinct
   `//----- (ADDR)` addresses are SWINE HD addresses; 1,283 of them are function entries in `swineHD.exe`.
   Each SWINE function was paired to a Panzers function by these methods:
   - a unique normalized-instruction hash on both sides;
   - a unique string signature;
   - a unique name match: the SWINE marker's identifier against our lifted identifiers, `panzers_symbols.csv`
     and Ghidra RTTI names;
   - RTTI vtable slot alignment;
   - call-graph propagation;
   - a fuzzy mnemonic-shape match with a margin.

   **Precision was checked** on pairs whose Panzers side is already lifted, by comparing the SWINE name with
   our name:

   | method | precision |
   |---|---|
   | name | 56/57 |
   | strings | 3/3 |
   | exact hash | 0/1 (an ICF-style destructor) |
   | fuzzy | 15/36 (~40%) |
   | vtable slots | 3/29 (SWINE and Panzers interfaces have different slot layouts) |

   So:
   - **high-confidence** = exact + strings + name: 126 SWINE functions → 63 Panzers functions not otherwise
     lifted;
   - **probable** = fuzzy + call-graph: 112 Panzers functions, weighted 0.4;
   - vtable pairs and a relaxed best-match pass (`swmatch3.tsv`, which pairs SWINE `SGogManager` with Panzers
     code) were dropped.
6. **Reachability** (`reach.py`).
   - **Edges:**
     - every direct call/jump reference;
     - every code address taken as data;
     - for every reference into .rdata/.data, all consecutive function pointers stored from that address,
       which covers vtables and callback tables;
     - thunks.
   - **Roots:** the CRT entry 0x767a1c and the `_initterm` initializer tables (0x7ea540..0x7ea6a0, 9
     initializers). For the SP set I also added every function the original executed in the Training Camp and
     tutorial DynamoRIO traces (`m3scope/cov_tc`, `cov_tut`: 2,917 functions).
   - **SP blockers:** the SSuperWindow MP entry points (LoadMultiPreMenu, LoadGameSpyTitleRoom, staging room,
     chat room, -host/-connect, CHECKCDKEY), plus the network stack starts: SMulti host/init 0x51f3c0,
     NatManager handlers/RakPeer/UPnP, PacketForwarding, RGMasterServer ctor/singleton, SGameSpy::Init/
     InitTitleRoom/InitStagingRoom, SGStats ctor and SNetwork ctor. A blocker is not applied if the original
     actually executed it in SP.
   - **Classes:** `sp` is reachable with the blockers in place. `mp-only` is reachable only through them.
     `dead` is not reached at all.
   - **Check:** 2,107 of 2,117 lifted functions are reached. The executed set is fully covered once the
     initializer tables are added: before that, 78 of 2,917 executed functions were missed, nearly all of them
     CRT and initializers.
7. **Subsystems.** Each game function gets a subsystem from these sources:
   - the documented address areas (M2_SCOPE §3.1, MP_SCOPE §2.1, M3_SCOPE);
   - membership in the vtables of MP classes (SChatRoomMenu, SGameSpy*, SMulti*, SRankedGaming*,
     STopListMenu, SSkirmishChatRoomMenu, ...);
   - the source file of the function's own marker, or of the lifted markers on both sides of it. Objects are
     contiguous, so neighbours almost always share a file.

## 1. Classification of all 19,661 functions

| class | functions | instructions | SP-reachable | MP-only | unreached | address range(s) | evidence | confidence |
|---|---:|---:|---:|---:|---:|---|---|---|
| **game/engine** | 5,298 | 557,588 | 4,242 | 342 | 714 | 0x4c8680-0x4d17a0, 0x51dc10-0x70cc00, 0x7c4ba0-0x7c8834 | 2,111 lifted markers, `S*::` strings, SDArray/SHeap panics, RTTI | high |
| D3DX9 (static) | 1,560 | 207,622 | 142 | 0 | 1,418 | 0x401000-0x4c8680 | D3DX/shader-compiler strings, ~6-20% 16-byte aligned (old compiler) vs ~100% for VS2015 code | high; the boundary at 0x4c8680 is exact (first aligned engine function, unit schema) |
| CRT + C++ runtime (VS2015 UCRT/vcruntime/libm) | 3,368 | 138,159 | 1,086 | 46 | 2,236 | 0x7646f7-0x7c4ba0, 0x7c8834-end | Ghidra FID names (2,293), `minkernel\crts\ucrt`, locale/printf tables | high |
| STL instantiations in game objects | 32 | 1,899 | 13 | 16 | 3 | scattered (mostly NatManager/RG glue) | `vector<T> too long` etc., `std::` | medium |
| RakNet 4.08x | 1,683 | 86,587 | 80* | 444 | 1,159 | 0x4d9970-0x51dc10 | `..\..\RakNet\Source\*.cpp`, RakNet RTTI | high |
| GameSpy SDK | 1,309 | 72,490 | 512* | 34 | 763 | 0x70cc00-0x741c00 | gamespy/peerchat/gpi/`\final\` strings, MP_SCOPE | high |
| RankedGaming client v5.0 | 639 | 48,455 | 5 | 467 | 167 | 0x741c00-0x7646f7 (+ 0x4d17a0-0x4d4c00 decoy-path generator) | `[RGS]`, RG* RTTI, CLog, TransFile | medium-high (the end at 0x7646f7 meets the first CRT FID name) |
| other third-party: libminiupnpc | 71 | 6,477 | 7 | 26 | 38 | 0x4d4c00-0x4d9970 | `libminiupnpc`, `Miniupnpc ...` errors, UPnP SOAP strings | high |
| zlib / png / jpeg | 0 | 0 | | | | | none present (TRIAGE §2) | high |
| EH funclets (compiler) | 5,662 | 19,278 | | | | mostly 0x7cbfc0-0x7e8fb0 | `Unwind@`/`Catch@` | high |
| import thunks | 39 | 39 | | | | | `*.DLL` namespace | high |

\* These SP-reachable counts are static over-approximation. A single-player SMulti object exists, and its
vtable reaches SGameSpy/NatManager wrapper code. The traces show the original executing only 10 RakNet,
5 RankedGaming, 3 miniupnpc and 0 GameSpy functions in single player.

Lifted bodies outside game code: 5 CRT libm SSE2 functions at 0x78d07a-0x78d810 (`3dengine/pz/hdmath.cpp`),
and one `Catch@0057391d` funclet. They are not counted in the game percentages.

## 2. Game code by status

| status | functions | instructions | SP-reachable | executed in traces |
|---|---:|---:|---:|---:|
| lifted (no STUB_LOG) | 2,032 | 269,290 | 2,026 | 1,593 |
| lifted-partial (STUB_LOG inside) | 79 | 32,442 | 79 | 64 |
| SWINE-shared, high-confidence pair | 63 | 5,641 | 53 | 21 |
| SWINE-shared, probable pair (~40% precise) | 112 | 6,697 | 92 | 45 |
| not lifted, cited by a STUB_LOG | 162 | 33,595 | 157 | 41 |
| not lifted | 2,850 | 209,923 | 1,835 | 700 |

Not lifted, in total (stub-cited + todo): **3,012 functions / 243.5k instructions.**

| reachability | functions | instructions |
|---|---:|---:|
| SP-reachable | 1,992 | 180.7k |
| SP-reachable, outside the MP subsystems | 1,820 | 163.5k |
| reachable only through MP entry points | 339 | 31.7k |
| unreached (dead) | 681 | 31.1k |

Sizes of the 1,820 SP non-MP functions still to do:

| size (instructions) | functions |
|---|---:|
| ≤10 | 298 |
| 11-50 | 871 |
| 51-200 | 496 |
| 201-1,000 | 138 |
| >1,000 | 17 |

The original executed 741 of the not-lifted functions (72k instructions) in the TC/tutorial traces. 277 of
those (39.6k instructions) are in the renderer, and some of them are probably served by SWINE engine code in
our build.

### SWINE-shared bodies: what they cover

| | count |
|---|---:|
| SWINE markers (distinct addresses) | 1,316 |
| function entries in swineHD.exe | 1,283 |
| high-confidence Panzers counterpart | 126 (59+3+1 = 63 of them already lifted, 63 not) |
| probable counterpart | 157 (37 already lifted) |
| vtable-only pairs | 90 (dropped as unreliable) |
| no counterpart found | about 910 |

The unpaired ones are mostly SWINE-only code:
- SOptions (79);
- match/player list boxes (58);
- GOG/platform (33);
- lake/water/spline/particles4 in gepard.cpp (178 of 210 gepard.cpp bodies unpaired);
- tip dialog, splash.

Some may also be Panzers functions that were inlined or changed too much to pair. A sample of 60 unpaired
SWINE bodies against their best Panzers candidate scored as follows:

| best shape score | share of sample |
|---|---:|
| ≥0.9 | 13% |
| 0.7-0.9 | 38% |
| below 0.7 | 49% |

This fits the RECCMP.md finding that only leaf widget/engine code carries over between SWINE and Panzers.

## 3. Remaining work by subsystem

The last four columns cover not-lifted functions only.

| subsystem | fns | ins | lifted | lifted % fn | lifted % ins | SWINE pairs | SP | MP-only | unreached | SP ins |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| engine-renderer (3dengine, effects, terrain, MPEG decoder) | 1,342 | 168,422 | 525 | 39.1% | 49.7% | 70 | **624** | 0 | 123 | **72,449** |
| units/combat (units, drivers, gunner, squads, projectiles, animation) | 1,040 | 104,122 | 695 | 66.8% | 82.1% | 4 | 258 | 0 | 83 | 16,386 |
| world/map (SWorld: map load, roads, wires, rivers, water, camera) | 411 | 51,272 | 93 | 22.6% | 38.7% | 1 | 174 | 2 | 141 | **21,527** |
| menus (SP menus, briefing, results, load/save, market, options) | 411 | 40,790 | 209 | 50.9% | 63.1% | 10 | 117 | 25 | 50 | 11,260 |
| game-logic (SGameLogic, triggers, packets/record-replay) | 281 | 33,723 | 129 | 45.9% | 56.6% | 1 | 122 | 0 | 29 | 13,592 |
| mp-network (SMulti, NatManager, PacketForwarding, RG/GameSpy glue, SGStats, SNetwork) | 388 | 27,641 | 0 | 0% | 0% | 3 | 151† | 141 | 93 | 11,792 |
| hud/gameview (SGameView, buttons, minimap, selection) | 180 | 26,737 | 116 | 64.4% | 87.8% | 1 | 53 | 0 | 10 | 3,070 |
| mp-menus (chat/title/staging rooms, MP pre/LAN/DirectIP/RG menus, top list, skirmish chat) | 147 | 22,509 | 0 | 0% | 0% | 0 | 21† | 115 | 11 | 5,420 |
| shell (SSuperWindow, settings, file system, version) | 175 | 15,811 | 32 | 18.3% | 65.6% | 8 | 95 | 16 | 24 | 3,219 |
| engine-core (streams, properties, logger, timer) | 239 | 14,632 | 96 | 40.2% | 58.1% | 20 | 77 | 0 | 46 | 3,731 |
| ai | 75 | 10,239 | 65 | 86.7% | 92.8% | 0 | 9 | 0 | 1 | 504 |
| ui-framework (SWINE-style widgets, SWindow) | 238 | 9,773 | 23 | 9.7% | 6.4% | 45 | 140 | 2 | 28 | 6,697 |
| campaign/save | 102 | 8,798 | 42 | 41.2% | 46.9% | 3 | 52 | 0 | 5 | 4,126 |
| pathfinding (A*, area filler, block map) | 49 | 8,242 | 34 | 69.4% | 86.2% | 0 | 6 | 0 | 9 | 298 |
| cutscenes (SInGameAnimLogic) | 81 | 6,350 | 7 | 8.6% | 10.3% | 2 | 45 | 0 | 27 | 4,301 |
| engine-sound (Miles concert, sound effects) | 80 | 5,683 | 45 | 56.2% | 61.3% | 6 | 28 | 0 | 1 | 1,282 |
| core-util (generic helpers that sit in the MP objects) | 59 | 2,844 | 0 | 0% | 0% | 1 | 20 | 38 | 0 | 1,089 |

† "SP" here is static over-approximation through the single-player SMulti object (see the note above).

**What's left for single player, by size:**

| subsystem | not-lifted SP-reachable instructions |
|---|---:|
| renderer | 72k |
| world/map | 22k |
| units/combat | 16k |
| game logic | 14k |
| menus | 11k |
| ui-framework | 7k |
| cut-scenes | 4k |
| campaign/save | 4k |
| engine-core | 4k |
| shell / HUD | 3k each |

On top of that, the 79 partial lifts hold 32k instructions.

- In the renderer and ui-framework, part of the not-lifted code probably already runs as SWINE code. The
  upper bound above caps this at roughly 10 extra points.
- **Unreached:** 681 not-lifted game functions (31k instructions). 511 have no reference anywhere in code or
  data; the 2016 build apparently doesn't strip unreferenced functions (/OPT:REF only removes COMDATs).
  - 179 are referenced only from other dead code.
  - 24 have a data pointer the graph doesn't follow, so they may be reachable.
  - The unreached code is mostly old GameSpy-peer code (SGameSpy::Connect and its callbacks), SVersion debug
    strings, `gDumpVariables` property dumpers, replay listing (`Replays/*.rec`), test strings ("Teszt %d") and
    SWorld helpers.
  - None of the strings in this code mark it as editor-only. Editor leftovers, if there are any, aren't
    separable from it.

## Caveats

- **Instruction counts are the original's.** A lifted-partial body or a "lifted" marker on a ¼-size stub
  counts as fully done. RECCMP.md found 70 lifted bodies under ¼ of the HD size at the 1,613-marker census.
  The "no STUB_LOG" columns give a lower bound. A size-ratio discount needs a fresh reccmp build, which I
  didn't run because this task is read-only.
- **One marker equals one HD function.** Duplicate markers (117 extra) and in-body `(piece)` markers are
  collapsed by address. 4 marker addresses are not Ghidra entries; one is data (0x808ff0).
- **SWINE coverage is uncertain.**
  - The only reliable pairs come from names, strings or exact hashes.
  - Fuzzy matching of small functions is ambiguous: many 5-20 instruction getters and setters have
    identical shapes.
  - The real figure lies between 41.0% and 51% of all game functions, most likely near 42%.
- **Reachability is static and conservative.**
  - Virtual calls are followed through every vtable a reachable function references. That over-approximates
    SP, especially into the MP glue (the starred figures). The "minus the MP subsystems" row corrects for the
    glue.
  - Reads of a function-pointer table from an offset other than its start are missed. They are rare: all
    2,917 executed functions and 2,107 of 2,117 lifted functions are reached.
- **Library boundaries** were set at evidence runs. Up to a few dozen small functions at each boundary
  (0x4c8680, 0x4d17a0-0x4d4dc0, 0x51dc10, 0x70cc00, 0x741c00, 0x7646f7) could belong to the neighbouring class.
  This is under 1% of game code.
- **Subsystems** are inherited from neighbouring markers or documented ranges, not decided function by
  function. Expect ±10% per row; the totals are unaffected.
- **"Executed" traces** cover only the Training Camp and the tutorial start (`cov_tc`, `cov_tut`). The menu
  coverage file `m2_coverage.tsv` was not merged; the `cov/` directory was empty.

## Files (all in `scratchpad/m4d/`)

| file | contents |
|---|---|
| `functions.tsv` | one row per HD function: `addr name ns cls sub subhow status reach executed nins nbytes swine src` |
| `metrics.json` | every aggregate behind these tables |
| `report_tables.md` | raw output of `report.py` |
| `swmatch2.tsv` | SWINE→Panzers pairs (`sw pz method score marked swname`) |
| `swmatch3.tsv` | relaxed best match for unpaired SWINE bodies (information only) |
| `reach.pkl` | edges and reach sets |
| `cov.pkl` | functions executed per trace |
| scripts | `gdump.py`, `feats.py`, `markers.py`, `evidence.py`, `swmatch*.py`, `reach.py`, `cov.py`, `classify.py`, `report.py` |

Rerun after a new commit: `python markers.py && python classify.py && python report.py`. Rerun `swmatch*.py`
only when SWINE markers change.
