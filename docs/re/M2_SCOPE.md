# M2 scope: game logic, the menu convoy, and the single-player game

Read-only scoping. Repo `menu-3d` 0de32c5. Ground truth: HD `PANZERS.exe` (Ghidra copy in `m2scope/ghidra`).
SWINE = `swine-portable@95bbcc4` (`world/`, `game/`), sizes in bytes from the original SWINE x86 binary
(`swinedecomp/reccmp-functions.csv`, address deltas). All addresses are HD unless marked SWINE.

New user permission: SWINE `world/` and `game/` may be copied and adapted. This document measures how much
that actually buys. **Short answer: little code, useful naming and semantics.** Panzers (2004) rewrote the
gameplay layer of SWINE (2001) around a virtual unit hierarchy, driver objects, A* path finding and an
event-driven trigger runtime. Only the campaign state machine, the trigger data loaders and a few
skeletons survive at code level.

## Files in this folder

| File | What |
|---|---|
| `dec/<addr>.c` | Ghidra C for **all 3,009 functions in 0x540000..0x660000** (world, game, UI). |
| `funcs_world.tsv` | Same range: addr, bytes, insns, name, call count, referenced strings. |
| `segments.txt` | Per-function class guess (RTTI vtables + symbols + `Class::` strings), run-length grouped. |
| `trigdec.py` | Decoder for the MAPF `TRIG` chunk (HD loaders 0x5f0140, 0x5b2400, 0x5b22b0, 0x5b2010). Parses `menu.map` exactly to the chunk end. |
| `closure.py`, `rt_used_callees.txt` | Direct-call + vtable closure over `dec/`, used for the M2 counts. |
| `strmatch.py` | String-literal match of every SWINE `world/`, `game/` .cpp against HD `strings.txt`. |
| `swfunc.py` | SWINE function finder (line counts, bodies). |
| `swine/` | `git archive 95bbcc4 world game` (read-only copy). |
| `editor_strings.txt` | Strings of the Panzers `editor.exe` copy: the trigger GetText tables. |
| `scripts/funcdump.py`, `scripts/bulkdec.py` | pyghidra helpers (run from `scripts/` via PowerShell; they `chdir` to `m2scope`). |

---

## 1. Class-by-class map

### 1.1 Global evidence

**String literals.** Every SWINE literal of 8+ chars, checked against HD `strings.txt`:

| SWINE file | literals | in HD | what matches |
|---|---:|---:|---|
| world/campaign.cpp | 82 | 21 (26%) | all `SCampaign::Init*Mode: GameMode can only be initialized once.`, `LetChatroomDone/LetMapDone/LetResultsDone: Menu order problem.`, `SetRace/SetArmyName/SetArmyFileName/GetArmyName/GetArmyFileName/GetMapName ...`, `missions.ini`, `Intro Anim`, `Result text`, `Mission code`, `missing.scene`, `SLoadMenu::LoadSavedGameNames` |
| world/unit.cpp | 202 | 42 (21%) | **only** unit property keys (`MoveSpeed`, `SpinSpeed`, `FrontArmor`, `BackArmor`, `TopArmor`, `UnitSize`, `ReloadTime`, `ReshotTime`, `ShotSpread`, `TurretSpeed`, `KickSize`, `KickTime`, `BodyKick`, `MaxRange`, `MinRange`, `Spring*`, `RodSpring*`, `SpecialTime`, `ExtraArmor` ...), `Unsupported unit sub-tag 0x%08X`, `Unsupported unit type`, container panics. No log line, no behaviour string. |
| world/world.cpp | 117 | 11 (9%) | map loader (`Not a map file`, `Tilemap without Heightmap`, `start %d`), already lifted from HD in M1 |
| world/gameworld.cpp | 165 | 9 (5%) | `start %d`, `Air start position for player %d is X:%f Z:%f`, `Inconsistency in frame %d with player %d.` (MP sync), `Too many light parameters` |
| world/trigger.cpp | 70 | 3 (4%) | `STriggerCondition::GetPropertyMask: Invalid type (%d)`, `STriggerAction::GetPropertyMask: Invalid type (%d)` |
| world/projectile.cpp | 17 | 1 | `DamageRadius` |
| game/gameview.cpp | 152 | 6 (4%) | `Loading map: %s`, `Can't load map: %s`, `%d:%02d:%02d`, `units.ini` |
| game/market.cpp | 95 | 8 (8%) | unit keys, `SMarket::SaveArmy` |
| game/superwindow.cpp | 152 | 11 (7%) | `SSuperWindow::LoadMultiPreMenu START/END`, `LoadNextCampaignView: Campaign unitialized`, `Create/ConnectToHostFromCommandLine START/END` (HD SSuperWindow is already lifted) |
| game/menu.cpp, ingamemenu.cpp, singlemenu.cpp, minimap.cpp, statuspanel.cpp | 190 | **0** | |

**Method names.** HD symbol names (from `Class::Method` log strings) that also exist in SWINE:
SGameLogic `CanSeeGroundUnit`, `PlaceUnits`, `ProcessPacket`, `BackupCampaignUnits` (SWINE: SGameWorld);
SWorld `CreateUnit`, `FindEmptySpace`, `FixBridges`, `IncreaseUnitXP`, `ShowUnitRange`, `ComputeCamera`;
SUnit `EC_Attack`, `EC_Follow`, `TakeDamage`; SCampaign: 13 of 16 HD names. Everything specific to Panzers
movement has no SWINE name: `SDriver::*` (15), `SAStar::MakePathPointsList`, `STarget::*`,
`SGameLogic::*MovementGroup*` (19), `RunTriggers`, `ExecuteScriptStatement`, `SPanzersSquadUnit::*`.

**Architecture.**

| | SWINE (2001) | Panzers HD (2004/2016) |
|---|---|---|
| Unit | one non-virtual `struct SUnit` (unit.h, 700 lines, 129 funcs / 90 KB x86), inline `SAIDriver`/`SAIGunner` structs, driver callbacks `DA_*_CallBack` / `GA_*_CallBack` with Hungarian names | 9 classes on a **115-slot vtable** (SUnit, SSingleUnit, SBuildingUnit, SFlyingUnit, SPanzersSquadUnit (116), SPanzersSquadMemberUnit, SProjectileUnit, STrainUnit, SWasterUnit) + 9 prototype classes `SP*Unit` (6 slots) loaded from `.unit` property files |
| Movement | ghost per unit (`GAS_*`, `FollowGhost`, `NextGhostStep`), grid `SGameWorld::FindPath` (3,728 B) | 13 `SDriver` classes (21 slots) + `SPDriver` prototypes, ghost frames (`SGhostFrame`, `Ghost_FirstStep/NextStep`), `SWayPointWithManoeuvres` (turning-radius manoeuvres), `STarget`, global + local `SAStar`, `SAreaFiller`, `SMovementGroup` formations/convoys |
| Game state | `SGameWorld : SWorld` (logic inside the world) | `SWorld` (0x7538) + separate `SGameLogic` (0x318, no vtable) |
| Triggers | polled `TestTriggers` (14,032 B), conditions+actions | event (11 types) + conditions (15) + actions (76), **running triggers** with found-unit groups, wait states, script statements |
| Projectiles | `SProjectile` plain struct | `SProjectileUnit` full unit + `SProjectileDriver` + `SProjectileAnimation` |
| Serialisation | hand-written chunk reads (`SUnit::Load` 3,520 B) | reflective `SPProperty` descriptors (`SUnit::Load` 0x5bbd30 reads `vars`/`targ`/`driv` sub-tags via 0x670440 + descriptor tables) |
| Units data | `units.ini` flat keys | `.unit` files (`Unit.Common.ClassType` ...), 611 types, `SUnitRegistry` |
| Infantry | none | squads (`SPanzersSquadUnit` + members), walker skeletal animation |

### 1.2 Table

Sizes: HD object size from `operator_new(size)` before the ctor that stores the vtable; "funcs/insns" is the HD
address range of the class (see 3.1 for ranges; boundaries are approximate). SWINE size in x86 bytes.

| HD class (slots, size) | HD funcs / insns, key functions | SWINE counterpart | Class | Evidence (samples) |
|---|---|---|---|---|
| **SGameLogic** (no vtable, 0x318) | ~296 / 39k: ctor 0x55e440 (1,159 insns), Refresh 0x576d80 (1,700), RunTriggers 0x579ab0 (3,757, 76 cases), CheckConditions 0x580600 (1,070), PlaceUnits 0x572ec0 (438), CanSeeGroundUnit 0x562760 (934 B), 19 MovementGroup funcs, ExecuteScriptStatement 0x568bc0 | `SGameWorld` (gameworld.cpp 16,558 lines, 127 funcs / 87 KB) | **(c)**, a few (b) functions | PlaceUnits: same skeleton (`sprintf("start %d", StartingPosition+1)`, iterate descriptions, CreateUnit, FindEmptySpace; HD 1,745 B vs SWINE 1,360 B) but no price/upgrade code. CanSeeGroundUnit: SWINE 128 B, one vismap byte lookup at 0.5 m cells; HD 934 B, buildings, crews, detector bits, different scale. Trigger runtime: different model (running triggers vs polling). Refresh: SMulti frame sync, FPU check, 20 Hz; SWINE ServerRefresh 3,488 B shares nothing. |
| **STrigger / STriggerEvent / STriggerCondition / STriggerAction** (non-virtual; 0x2c / 8 / 0x38 / 0x64) | 10 / 674: loaders 0x5f0140, 0x5b2400, 0x5b22b0 (321 B), 0x5b2010 (662 B), masks 0x5b1d50 / 0x5b1f30 | trigger.cpp (685 lines, 11 funcs; Condition::Load 336 B, Action::Load 288 B) | **(b)**, data part close to (a) | Same scheme: `Type = ReadInt(); mask = GetPropertyMask(); if (mask&1) Player; if (mask&2) QuantityMore,Quantity; if (mask&4) Location; if (mask&8) UnitType + UnitClass ...`, same panic strings, same `-1` defaults. HD Condition::Load still reads SWINE's mask 0x40 switch pair and **discards it**. HD adds STriggerEvent and masks 0x200/0x2000/0x40000 (conditions) and 0x100..0x400000 (actions). Condition::Load 321 vs 336 B. Type numbering differs (SWINE action 10 = Victory; HD 10 = Preserve trigger). |
| **SUnit** (115, base ≤0x354) | ~221 / 21k: TakeDamage 0x5c4080 (1,010), Save 0x5be320 (887), Load 0x5bbd30 (449), SetActiveDriver x5, OnDriverReachedTarget 0x5bcb60, EC_* (Attack, AttackMove, Ammo, Repair, AssaultBuilding, Die) | `SUnit` (unit.cpp 14,591 lines) | **(c)** | Load: SWINE 3,520 B of hand-written chunks vs HD 1,434 B reflective (`vars`, `targ`, `driv` tags + 0x670440); only `Unsupported unit sub-tag 0x%08X` shared. TakeDamage HD 4,002 B (child units, `music/war_%02d.mp3`) vs 3,136 B. SWINE has no vtable, no drivers, no stored units/crews. |
| **SSingleUnit** (115, 0x3a8) | 58 / 7.0k: Init 0x5ad150/0x5ad9f0, InitAfterInitAfterLoad 0x5ae370, EC_Default 0x5ac1a0, RefreshMisc 0x5af890, OnDriverReachedTarget 0x5aed30 | (SUnit) | (c) | no SWINE equivalent of the single/squad split |
| **SBuildingUnit** (115, 0x470) | 125 / 10k (0x546490..0x5500a0), CreateCaptureFlag, SetCurrentTarget | buildings are SWINE SUnit types | (c) | entering/indoor/"Block" nodes, capture flags: Panzers-only |
| **SPanzersSquadUnit** (116, 0x3a4) / **SPanzersSquadMemberUnit** (115, 0x354) | 90 / 8.8k and 34 / 1.6k: SetRelativePositions x5, Init 0x59c470, EC_Default 0x59a960, OnDriverReachedTarget 0x59d720, AI_Heartbeat, EC_ChangeActiveDriver | none (SWINE has no infantry) | **(c)** | |
| SFlyingUnit (0x388), SProjectileUnit (0x36c), STrainUnit (0x3b4), SWasterUnit (0x364) | 31, 15, 16, 18 funcs | SWINE planes/helis inside SUnit; `SProjectile` (8 funcs, 4.2 KB) | (c) | SProjectileUnit is a full unit with its own driver and animation |
| **SP\*Unit prototypes** (6 slots, 0x140..0x160), SPProperty | 70 / 6.0k (0x5a4900..), SUnitRegistry 0x5cfe30/0x5d1050 (lifted in M1) | `units.ini` reads inside SUnit::Create | (c) | `.unit` property files; leaf key names match SWINE ini keys (21 keys), structure does not |
| **SDriver + 12 subclasses** (21 slots; 0xe8/0xec/0xf4), **SPDriver** (5 slots, 0x44..0x58) | 153 / 14k: Ghost_NextStep 0x553c00 (740), FindGlobalPath 0x551cb0, MoveTowardNextWayPoint 0x5582a0 (284), HasTheUnitArrivedAtTheWayPoint 0x554fc0, SetNextWP 0x55b0b0, IsTheGhostInStoppingDistance 0x557400 | SUnit `GAS_*`, `DA_*`, `GA_*`, `NextGhostStep` (1,312 B), `FollowGhost` | **(c)**, concept heritage | Same "ghost runs ahead of the unit" idea and word (`Ghost_*`, `GhostFrames`, SWINE `EGhostState`), but HD keeps a frame queue (`SDEQueue`, `GetTopIndex/GetBottomIndex`), waypoint types, manoeuvres, global/local paths. No shared strings or constants checked equal. |
| SWayPointWithManoeuvres, SManoeuvre, STarget | 22 / 4.0k + STarget 0x5b7c50/0x5ba0b0/0x5bd210 | none | (c) | `TurnInAngle_InAndOut___Through_Waypoint_With_Turning_Radius*` |
| **SAStar, SAStarClosedNode, SAreaFiller, SHeapList** (1 slot each) | 27 / 2.5k: MakePathPointsList 0x5a2020; SWorld::GetGlobalAStar 0x5e70e0, GetLocalAStar 0x5e7960, GetNearestPathNode 0x5e7990, CheckStaticBlockMapInternal 0x5da050 (3,153) | `SGameWorld::FindPath` (3,728 B grid search returning `SStack<SWaypoint>`), `TestPathLine*`, `GenerateUnitsToStaticBlockMap` | **(c)** | different algorithm and data (two-level A*, area filler, path nodes from ROD2/PATH/RODJ) |
| SMovementGroup, formations | inside SGameLogic (0x56ae90..0x5824b0) | none | (c) | convoy, boss/biggest unit, formation dir/pos, slowest speed |
| SAIGroup (AIGP), AI 0x5f5c70 (1,010) | in SWorld/SGameLogic | `SAIDriver`/`SAIGunner` per-unit structs, `EC_SetDriverAI` | (c) | AI groups with tactics/base (trigger actions 0x33..0x36) |
| **SGunner** (13, 0x78), SPGunner | 37 / 5.5k: ServerRefresh 0x584d00 (2,353 insns, 9,179 B) | `SUnit::DoGunner` (5,296 B) | **(b-)**: semantics, not code | Same property keys (ReloadTime, ReshotTime, ShotSpread, ShotAngle, TurretSpeed, KickSize, KickTime, BodyKick, Min/MaxRange); HD is a separate object per weapon, twice the size, own panics |
| SUnitAnimation + 10 subclasses (17 slots; 0x2c..0xc8), SP\*Animation | 140 / 10.7k: SVehicleAnimation::InitModel 0x5c93a0, SWalkerAnimation::UpdateModel 0x5ce2a0 (1,547) | SWINE vehicle model setup inside SUnit (`InitWheelsAndChains`, `SWheelData`, `SChainData`) | (c) | skeletal walkers, running gear objects (`SRunningGear`), boats, flying fox |
| SInGameAnimLogic (cut-scenes) | 83 / 6.4k | `SCampaign::GetAnimFileName` + anim views | (c) | |
| **SCampaign / SPanzersCampaign** | 98 / 7.8k: InitCampaignMode 0x592b20 (506 B), LetMapDone 0x594e00 (71 B), SaveGame 0x5966a0, LoadGame 0x594f70 (exports), SObjective | campaign.cpp (2,035 lines, 72 funcs / 8.9 KB) | **(b)**, best copy candidate | Identical panic strings; same fields (GameMode, Race, MenuToLoad, MapStatus, ArmyName, ArmyFileName, missions.ini); InitCampaignMode: SWINE `"Rabbit 1"/"Pig 1"`, HD `"German 1"/"Allied 1"/"Russian 1"` + per-campaign `missions.ini`; LetMapDone same order check (`MenuToLoad == 2` panic) but different branches. Layout differs (HD GameMode at +0x10, MenuToLoad at +0xe0; SWINE GameMode first). Save/Load bodies differ (HD reflective units, replays). |
| SUnitGroup / SUnitGroupUnit | RTTI in HD | campaign.h structs | (b) | name match only checked |
| SPlayer (PLY3, stride 0x48), SLocation (LOCS), SPath (PATH), STriggerVariable (TVAR) | loaders lifted/kept raw in M1 | `SPlayer`, `SLocation` structs in unit.h | (b) data | field sets overlap; HD adds per-player vis maps at SGameLogic+0x1d4 |
| **SGameView** (31 + 5 slots) | 84 / 15.7k: 0x619c90 (4,162 insns), CreateSubViewports 0x61e500, LoadMap, SetPanelMode | gameview.cpp (4,278 lines, 40 funcs / 31.7 KB) | **(b-)** | `Loading map: %s`, `Can't load map: %s`, clock format shared; HD subviewports, panels, unit/hero/command buttons, spec info widget |
| SMinimap (31) | ~10 / 1.1k (0x64c2c0..) + SGameLogic minimap targets | minimap.cpp (189 lines, 7 funcs / 1.6 KB) | (c) | 0 shared strings |
| In-game menus (SInGameMenu, SSaveMenu, SLoadMenu, SHelpMenu, SStatisticMenu, SInGameBriefingMenu, SOptionsMenu ...), SSingleMenu, SSingleDiffMenu, SBriefingMenu, SResultsMenu, STrainingMenu, SScenarioMenu | ~216 / 27.7k (0x628430..0x640700) | ingamemenu.cpp, singlemenu.cpp, menu.cpp | (c) | 0 shared strings; the repo already lifts the HD menu shell (main/options/credits) from HD |
| SMarket, SMarketView, SMarketVehicleMenu | 47 / 11k | market.cpp (3,474 lines, 19 funcs / 19.6 KB) | (b-) | `SMarket::SaveArmy`, unit keys |

**Verdict:** (a) none among gameplay classes. (b) SCampaign/SPanzersCampaign flow, trigger data loaders,
PlaceUnits, data structs (players, locations), SGameView/SMarket skeletons. (c) everything that moves,
shoots, thinks, animates or serialises.

---

## 2. The menu convoy (M2 proper)

### 2.1 Trigger model (HD)

* `STrigger` (0x2c, World+0x7474 array, count at +0x7478): flags (2 on every live trigger, 0 on the two
  separator entries; RunTriggers clears bit 0 when a non-preserved trigger finishes), name, `STriggerEvent`
  {type, param}, conditions (0x38 each), actions (0x64 each). "Disabled" below means flag 0 (inferred).
* Events (11, from the Panzers editor GetText table, masks match): 0 every second, 1 unit dies,
  **2 unit enters location P**, 3 leaves location P, 4 attacked, 5 enters building/vehicle, 6 leaves,
  7 starts towing, 8 stops towing, 9 heavy bomber sent to P, 10 paratroopers sent to P.
  Dispatchers: 0x570cc0 (event 0, from Refresh), **0x571280 (event 2, from UpdateActiveLocations 0x582080)**,
  0x570e40 (event 4, from TakeDamage), 0x570ac0/0x570bc0 (9/10), 0x571090.. (others). Each calls
  **CheckConditions 0x580600** (switch over the 15 condition types). On success a `SRunningTrigger`
  (0x34, array at SGameLogic+0x268) with its found-unit group is started (inferred from RunTriggers' use
  of that array; the hand-off was not traced line by line).
* **RunTriggers 0x579ab0** walks running triggers and executes the current action (`switch(action->Type)`),
  honouring waits (+8 counter) and dropping dead units from the group.

### 2.2 `menu.map` TRIG decoded (`python trigdec.py data/menu.map`, parses to the exact end)

Variables (TVAR): 0 `Time`, 1 `time 2` (both counters). Locations and path are LOCS/PATH indices.

| # | Name | Event | Conditions | Actions |
|---|---|---|---|---|
| T0 | `--- konvoj 1 ---` (disabled) | | | |
| T1 | 1 konvoj krealasa | every second | `time 2` == 1 | CREATE x4 at loc 13/6/11/12, dir 180, player 0: `US Sherman`, `US M2A1`, `GB Bedford QL Ammo` x2; Preserve |
| T2 | 1 konvoj mozgas del fele | every second | `time 2` == 2; Find units of P0 at loc 15 | **Move found units along path 4 in convoy**; Preserve |
| T3 | 2 convoj krealasa | every second | `time 2` == 40 | CREATE `US M36-Slugger` x2, `US Sherman`, `US M7-Priest`; Preserve |
| T4 | 2 convoj mozgas | every second | `time 2` == 41; Find P0 at loc 15 | convoy along path 4; Preserve |
| T5 | szamlalo | every second | `time 2` == 90 | Set `time 2` = 0; `time 2` += 1 per second; Preserve |
| T6 | egyseg eltunik | enters loc 15 | Find P0 at loc 16 | **Remove found units**; Preserve |
| T7 | `--- gyalogos ---` (disabled) | | | |
| T8 | jeep es crew krealasa | every second | `Time` == 1 | CREATE `US Willys Jeep` at loc 4 dir 360; Preserve |
| T9 | mozgas eszak fele | every second | `Time` == 2; Find P0 at loc 5 | **Move found units to loc 8**; Preserve |
| T10 | egyseg eltunik | enters loc 8 | Find P0 at loc 9 | Remove; Preserve |
| T11 | elindulnak a gyalogosok | every second | `Time` == 15; Find P0 at loc 24 | Move found units to loc 21; Preserve |
| T12..T16 | officer/rifle/medic/mg/flamethrower eltunik | enters loc 21 | triggering unit is `US Wilson hero` / `US Rifle Squad(2)` / `US Medic Squad` / `US MG Squad` / `US Flamethrower Squad`; put triggering unit in group | **Teleport found units** to loc 24/16/18/20/22; T16 also resets `Time` = 0, +1/s; Preserve |

So the infantry are the **placed UNDS units**, walked to loc 21 and teleported back (the loop), and the
jeep and both convoys are created by CREATE (0x29) and removed at the end.

Used IDs: events {0, 2}; conditions {2 Find units at, 3 Compare variable, 5 type of triggering unit,
6 put triggering unit in group}; actions {0x02 Move to location, 0x03 Set variable, 0x08 Set variable to
count per second, 0x0a Preserve trigger, 0x1e Move along path in convoy, 0x1f Remove found units,
0x26 Teleport, 0x29 Create unit (behaviour, state, equipment, crew, location, dir, player)}.
Each ID is confirmed three ways: the action mask (0x5b1d50) matches the fields the editor text needs, the
RunTriggers case body does what the text says (case 3 calls World vtbl+8 SetVariable with the variable
field; case 0xa sets running-trigger flag |2; case 1/0x29 compute the location centre and create; case 2
calls the move order 0x57efd0), and the map data uses them consistently.
The full 76-action / 15-condition text list is in `editor_strings.txt` 0x4f3484..0x4f3ea4; the alignment
is tentative for 0x19, 0x1a and 0x38..0x3b.

### 2.3 HD functions behind each used piece

| Piece | HD |
|---|---|
| Trigger loaders | 0x5f0140, 0x56e7d0 (u16 string), 0x5b2400, 0x5b22b0, 0x5b2010, 0x5b1d50, 0x5b1f30, TVAR 0x56e440, LOCS 0x5f0690, PATH 0x5f07b0 |
| Variables | SWorld vtbl +4 Get / +8 Set (SWorld vtable 5 slots, 0x5d68b0..0x5fec80), counters 0x56da00 (`TIMECOUNTER`, `UNITCOUNTER`), per-second step 0x573760/0x5609b0 |
| Events/conditions | 0x570cc0, 0x571280, 0x582080 UpdateActiveLocations, 0x5640b0 CollectActiveLocations, 0x580600 CheckConditions (+ 0x5817f0, 0x581ae0, 0x581c50 helpers), 0x582770 (group compaction) |
| Running triggers | 0x579ab0 (cases 1, 2, 3, 8, 0xa, 0x1e, 0x1f, 0x26, 0x29 + preamble), 0x579170, 0x560610/0x5605d0/0x560790/0x5608c0/0x5609b0 (array accessors) |
| CREATE | 0x5cfb10 unit description (lifted), 0x52c2c0, 0x560d10/0x560290 crew, **SWorld::CreateUnit 0x5e2da0 / 0x5e3170**, **SWorld::FindEmptySpace 0x5e58d0** (769 insns) / 0x5e5700, unit vtbl +0x50 |
| Move to location | SGameLogic 0x57efd0 -> 0x56ff30 (474 insns, group order) |
| Convoy along path | 0x56ff30(1, 0, group, 1) + 0x57e600; movement groups 0x56ae90..0x5824b0 (`GetMovementGroupConvoy`, `SendConvoyMovementGroupFollowers` 0x57e6f0, `RefreshMovementGroup` 0x5800d0, boss/biggest unit, formation pos/dir, slowest speed) |
| Remove | 0x5f8060 SWorld::RemoveUnit (lifted), 0x5649e0 CreateAnimatedModel (`anim/PZLp11C.4d` puff when visible) |
| Teleport | 0x5c2d30 (stop), 0x5e5700, unit vtbl +0x24 SetPosition |
| Per tick | Refresh 0x576d80: 0x604620 world refresh (1,152), per unit vtbl +0x16c / +0x2c / +0x3c, 0x5822a0, doodads |
| Per frame | 0x5638f0 UpdateUnitVisuals (493) -> CanSeeGroundUnit 0x562760 |
| Drivers | SDriver 0x550120.. (Ghost_*, FindGlobalPath, MoveTowardNextWayPoint, SetNextWP ...). Menu units most likely use STurnInPlaceDriver (tracked: Sherman, M36, M7), STurnInAngleDriver (wheeled/half-track: Willys, Bedford, M2A1), SPanzersSquadDriver + SPanzersSquadMemberDriver (squads), SWalkerDriver (hero). **Inferred from class names, not verified against the .unit files.** |
| Path finding | SWorld::GetGlobalAStar 0x5e70e0, GetLocalAStar 0x5e7960, GetNearestPathNode 0x5e7990, SAStar 0x5a1fd0.. (MakePathPointsList 0x5a2020), SAreaFiller, CheckStaticBlockMapInternal 0x5da050 (3,153; already called by the SGameLogic ctor), road path nodes (M1 left ROD2/RODJ path data unbuilt, see `SMapRoad::Build` comment), SWayPointWithManoeuvres 0x58d4e0.. |
| Unit animation | SVehicleAnimation (InitModel 0x5c93a0, UpdateModel 0x5cd020: wheels, tracks, turret), SWalkerAnimation 0x5ce2a0 (move sequences `normal_move` etc.), SSquadAnimation, SBuildingAnimation |

### 2.4 Minimum set and its size

Closure over the decompiled range (`closure.py`, direct calls; virtual calls only through the listed vtables):

| Roots | World-range funcs | insns |
|---|---:|---:|
| Refresh alone (= RunTriggers alone: they reach each other) | 607 | 78k |
| SGameLogic ctor/dtor/Refresh/0x5638f0, RunTriggers pruned to the 9 used cases, AI/speech/autosave/DWire/flying fox/SMulti/script excluded | 561 | 66k |
| + full vtables of SSingleUnit, SPanzersSquadUnit, SPanzersSquadMemberUnit, SBuildingUnit, 6 drivers, 4 animation classes | 1,235 (179 of them ≤12 insns) | 139k |
| Whole world range 0x546000..0x60a000, for scale | 2,058 | 214k |

The 115-slot unit vtables pull in combat, damage, storage, AI and save code. A faithful M2 that stubs
those slots with logged stubs, and stubs the 67 unused trigger cases, lands at an estimated
**600-800 functions** (the old MENU3D_SCOPE estimate of +350-450 is too low). The exact minimum needs a
coverage trace of the original menu (recommended as M2 phase 0, see 4.1).

Minimum list (each entry is a function family to lift; SWINE help in brackets):
1. Trigger data: STrigger/Event/Condition/Action loaders + TVAR/LOCS/PATH structs [copy-adapt SWINE trigger.cpp loaders].
2. SGameLogic: ctor/dtor (single-player path), Refresh, trigger counters, event dispatch 0/2, CheckConditions (cases 2, 3, 5, 6), active locations, RunTriggers (9 cases, others `STUB_LOG` per case), orders 0x57efd0/0x56ff30, movement groups (convoy, formation), UpdateUnitVisuals, CanSeeGroundUnit (or "player 0 sees all" stub, flagged).
3. SWorld: CreateUnit (real units, replaces the M1 stand-in), FindEmptySpace, RemoveUnit, world refresh 0x604620, block map, global/local A*, path nodes from roads/paths.
4. Units: SUnit base (refresh, position, driver switching, OnDriverReachedTarget/Stucked, stop, EC_Default/move), SSingleUnit, SPanzersSquadUnit (relative positions, formation), SPanzersSquadMemberUnit, SBuildingUnit (static, already placed in M1), SP*Unit prototypes from `.unit` (needs the reflective property loader already lifted at 0x670440).
5. Drivers: SDriver base (ghost frames, waypoints, global/local path), STurnInPlace/STurnInAngle/Walker/PanzersSquad/PanzersSquadMember drivers + SPDriver prototypes, SWayPointWithManoeuvres, STarget.
6. Animation: SVehicleAnimation, SWalkerAnimation, SSquadAnimation move/stand updates.
7. Stubs (logged): SGunner refresh (no targets), AI 0x5f5c70, speech, autosave, SMulti sync (keep the single-player no-op 0x5212a0), objectives, subtitles, minimap, selection.

---

## 3. Effort for the single-player game

### 3.1 HD function census (Ghidra functions, approximate class ranges)

| Range | What | funcs | insns |
|---|---|---:|---:|
| 0x546490..0x5500f0 | SBuildingUnit (+ heap helpers) | 125 | 10.3k |
| 0x5500f0..0x55c900 | SDriver family + SPDriver | 153 | 14.0k |
| 0x55c900..0x55e440 | SFlyingUnit | 31 | 1.8k |
| 0x55e440..0x582a00 | SGameLogic | 296 | 39.1k |
| 0x582a00..0x587d00 | SGunner | 37 | 5.5k |
| 0x587d00..0x58d4e0 | SInGameAnimLogic (cut-scenes) | 83 | 6.4k |
| 0x58d4e0..0x590e00 | manoeuvres | 22 | 4.0k |
| 0x590e00..0x5977e0 | SCampaign / SPanzersCampaign | 98 | 7.8k |
| 0x5977e0..0x5a1300 | squad + squad member units | 124 | 10.4k |
| 0x5a1300..0x5a3810 | SAStar / SAreaFiller / SHeapList | 27 | 2.5k |
| 0x5a3810..0x5aa7b0 | projectile unit, SP* prototypes, running gear, specials | 94 | 7.9k |
| 0x5aa7b0..0x5b1d00 | SSingleUnit, STrainUnit | 74 | 8.0k |
| 0x5b1d00..0x5b2800 | triggers (data) | 10 | 0.7k |
| 0x5b2800..0x5c6300 | SUnit, STarget | 221 | 21.1k |
| 0x5c6300..0x5cfb00 | SUnitAnimation family | 140 | 10.7k |
| 0x5cfb00..0x5d2f90 | registry, SPProperty, SWasterUnit | 36 | 3.7k |
| 0x5d2f90..0x60a000 | SWorld (map, block map, roads, wires, rivers, A* glue) | 479 | 60.2k |
| 0x60a000..0x619400 | chat room / GameSpy (MP) | 130 | 17.4k |
| 0x619400..0x628430 | SGameView, buttons, group icons | 84 | 15.7k |
| 0x628430..0x640700 | in-game and single-player menus | 216 | 27.7k |
| 0x640700..0x64b000 | SMarket | 47 | 11.1k |
| 0x64b000..0x660000 | minimap, settings, skirmish (MP), SSuperWindow, file system | 336 | 25.2k |

Already lifted: 78 world functions (M1), 161 in `src/panzers` (shell, menus), 343 engine (pz), 175 pz stubs left.

### 3.2 Estimate (load a mission, play, win/lose, save/load, campaign flow)

| Bucket | Functions | Source |
|---|---:|---|
| World + game logic (0x546000..0x60a000 minus M1, minus MP-only packet/replay code) | 1,800-1,950 | HD |
| UI single-player (SGameView, buttons, minimap, in-game menus, single/briefing/results/load/save menus, market) | 330-400 | HD, SWINE as layout reference |
| 3D engine still missing for missions (175 pz stubs + lakes, rivers, wires, shadows, selection/range rendering, trails, rain/snow, cut-scene camera) | 250-400 | HD (SWINE 3dengine reference) |
| Sound in game (SSoundEffect, speech queue, unit sounds) | 50-100 | HD (Miles already lifted) |
| **Total** | **≈2,450-2,850** | |
| of which copy-adaptable from SWINE | **≈80-150 (3-6%)** | SCampaign base (~30-40 usable of 72), trigger loaders (~8), PlaceUnits skeleton, SPlayer/SLocation/SUnitDescription structs, SMarket/SGameView/menu skeletons (~30-60), SLoadMenu save-name listing |
| must lift from HD | ≈2,350-2,700 | |

SWINE still saves time as a **reference**: field meanings (campaign results, players, locations, unit ini
keys), the ghost-movement vocabulary, vis-map/block-map ideas, menu flow. I would credit 10-15% faster
lifting in campaign, market, gunner and SGameView code, not more.

Uncertainty: ±20% on the totals. The class ranges are cut at symbol anchors, so a few dozen functions may sit
in the wrong row. MP-only code inside SGameLogic/SWorld (packet record/playback, frame sync) was not
separated precisely. Virtual dispatch outside the listed vtables was not followed.

### 3.3 Riskiest parts

1. **Movement determinism.** Ghost frames + manoeuvres + two-level A* + block map at 20 Hz. Small float or
   order differences make the convoy leave the road, jam, or hit "FindEmptySpace tul messze talalt helyet".
   Needs SSE-exact maths (see the FPU notes in MENU3D_INTERFACES §4) and identical iteration order of the
   SHeapTRB unit heap.
2. **The unit vtables.** 9 classes x 115 slots with no symbols for most slots; layouts 0x354..0x470 bytes;
   M1's stand-in `pz::SUnit`/`SUnitType` must be replaced (integration churn across `src/world`).
3. **Interconnection.** Refresh, RunTriggers and the unit slots reach each other (607-function closure from
   either root); stubbing must be per slot / per case, logged, or M2 balloons to ~1,250.
4. **Huge functions:** RunTriggers 3,757 insns, CheckStaticBlockMapInternal 3,153, SGunner::ServerRefresh
   2,353, SGameView 0x619c90 4,162, Refresh 1,700, SWorld 0x604620 1,152, CheckConditions 1,070, TakeDamage 1,010.
5. **Reflective serialisation** (SPProperty descriptor tables) for `.unit` prototypes and save games.
6. **Multiplayer interleaving** in Refresh (SMulti frame sync, packet recording); the single-player path
   must stay byte-faithful while MP stays stubbed.
7. Copying SWINE code carries its own risk: SWINE layouts are not HD layouts (SCampaign GameMode at +0 vs
   HD +0x10), SWINE has x64 fixes (campaign 12, unit 22, gameworld 64 `x64` notes) and HD-remaster
   `hdbeefup` code (gameworld 91, gameview 104, unit 48 mentions) that HD Panzers never had.

---

## 4. Proposed plan

### 4.1 M2 (menu convoy and squad)

**M2-P0 (one agent, blocks the rest).**
* Coverage trace of the original menu: run a scratch copy of `PANZERS.exe` (never the install) under
  DynamoRIO `drcov` (or a breakpoint tracer) for ~3 minutes on the menu, map basic blocks to functions.
  Output: the exact executed function set, per class and per vtable slot. This turns the 600-800 estimate
  into a list and tells which unit slots and trigger cases really run.
* Headers in HD order and size (`PZ_HD_SIZE`): `src/game/iunit.h` (SUnit 115 slots + SPanzersSquadUnit slot 116),
  `src/game/idriver.h` (SDriver 21, SPDriver 5), `src/game/iunitanim.h` (SUnitAnimation 17, SP*Animation 5),
  `src/game/punit.h` (SP*Unit 6), `src/game/gamelogic.h` (move from `src/world/gamelogic.h`, field map),
  `src/world/trigger.h` (STrigger 0x2c, STriggerEvent 8, STriggerCondition 0x38, STriggerAction 0x64,
  SRunningTrigger 0x34), shared accessors for the SHeapTRB unit heap (World+0x4d4).
  Slot names from the survey vtables + symbols; unknown slots `Slot_XX` with RET-count comments.
* Stub every slot with `STUB_LOG` + `PZ_TRACE`, as M1 did.

**M2 parallel agents (after P0):**

| Agent | Owns | Delivers | Depends on |
|---|---|---|---|
| **L: logic + triggers** | `src/game/gamelogic*.cpp`, `src/game/triggers.cpp`, `src/game/movementgroup.cpp`, `src/world/trigger.*` | SGameLogic ctor/dtor/Refresh (SP path), counters/variables, event dispatch 0/2, active locations, CheckConditions (2, 3, 5, 6), RunTriggers (9 cases, other cases logged), orders 0x57efd0/0x56ff30, movement groups + convoy, UpdateUnitVisuals, CanSeeGroundUnit | P0; calls units/drivers only through iunit/idriver |
| **U: units + world unit API** | `src/game/unit*.cpp`, `src/game/singleunit.cpp`, `src/game/squadunit*.cpp`, `src/game/buildingunit.cpp`, `src/game/punit*.cpp`, `src/world/unit.cpp` (replace stand-ins), `src/world/unitregistry.cpp` | SUnit base refresh/position/driver switching/arrival, SSingleUnit, SPanzersSquadUnit (+relative positions), member unit, building unit, SP*Unit from `.unit`, SWorld::CreateUnit/FindEmptySpace/RemoveUnit, teleport | P0 |
| **P: drivers + path finding** | `src/game/driver*.cpp`, `src/game/manoeuvre.cpp`, `src/game/target.cpp`, `src/world/astar.cpp`, `src/world/blockmap.cpp`, `src/world/pathnodes.cpp` | SDriver base + TurnInPlace/TurnInAngle/Walker/PanzersSquad/Member drivers, SPDriver, ghost frames, manoeuvres, STarget, global/local SAStar, SAreaFiller, CheckStaticBlockMapInternal, road/path nodes (finish what `SMapRoad::Build` left for M2) | P0; unit fields through iunit.h |
| **A: unit animation** | `src/game/unitanim*.cpp` | SVehicleAnimation (wheels, tracks), SWalkerAnimation (move/stand/idle), SSquadAnimation, SBuildingAnimation; takes over `SUnit::RefreshModel`/`PlaceUnitModel` from M1 | P0; models via `pz/imodel.h` |

Integration (one agent): wire `LoadMenuBackground` to the real SGameLogic, remove the M1
`SWorld::RefreshModels` stub path, compare against `p4scope/shots` (convoy every 90 s, infantry loop).

Interfaces between them: units own their driver and animation pointers (SUnit fields at HD offsets) and call
them through vtables; SGameLogic issues orders through SUnit slots and 0x56ff30; drivers ask SWorld for
A* objects (GetGlobalAStar/GetLocalAStar) and the block map; animation reads unit state only.

### 4.2 After M2: one mission playable end to end

| Phase | Content | Owners |
|---|---|---|
| M3 shell | New Game / Load Game paths of `SSuperWindow::OnAction`, SSingleMenu, SSingleDiffMenu, SBriefingMenu, SGameView::LoadMap (full SWorld + SGameLogic), results, **SCampaign/SPanzersCampaign** (copy-adapt SWINE campaign.cpp) | `src/game/campaign*.cpp`, `src/panzers/` menus |
| M4 combat | SGunner, SProjectileUnit + driver + animation, TakeDamage/death/wrecks, effects hooks, AI groups and 0x5f5c70, all 76 actions / 15 conditions / 11 events, objectives, subtitles, speech | L, U, P agents continue |
| M5 in-game UI | SGameView input (selection, orders, camera modes of ComputeCamera), unit/hero/command buttons, group icons, spec info, minimap, in-game menus (save/load/options/help/stats/briefing), market | new UI agent(s), `src/game/ui/` or `src/panzers/` |
| M6 save/load | SUnit::Save/Load (reflective), SGameLogic::LoadGameState, SPanzersCampaign SaveGame/LoadGame/autosave/quick save | U + campaign owner |
| Engine | shadows, lakes, rivers, wires, flying fox, rain/snow, selection/range rendering, cut-scene camera (SInGameAnimLogic) | engine agents (pz) |

### 4.3 SWINE files to import as the starting point

| SWINE file | Destination | Use | Adaptations |
|---|---|---|---|
| `world/campaign.{h,cpp}` | `src/game/campaign.{h,cpp}` (base of `SPanzersCampaign`) | copy-adapt | HD layout (GameMode +0x10, Race +0x18, MenuToLoad +0xe0, missions.ini SProperties at +0/+4), three campaigns (German/Allied/Russian) instead of Rabbit/Pig, HD state machine in LetMapDone/LetResultsDone, drop SP/army/market parts that HD moved, drop 12 x64 fixes and 2 HD-remaster bits, `PZ_HD_SIZE`, log via the HD SLogger calls, add SObjective/replay from HD |
| `world/trigger.{h,cpp}` | `src/world/trigger.{h,cpp}` | copy-adapt loaders only | HD sizes (0x2c/0x38/0x64), HD masks 0x5b1d50/0x5b1f30 (76/15 types), extra fields, STriggerEvent, u16 strings, discard the 0x40 pair as HD does; drop SWINE GetText (editor only) |
| `world/unit.h` structs `SPlayer`, `SLocation`, `SUnitDescription`, `SUnitGroup(Unit)` (campaign.h) | `src/world/` / `src/game/` headers | reference for fields | HD strides (players 0x48) and order |
| `world/gameworld.cpp` `PlaceUnits`, `FindEmptySpace`, `CanSeeGroundUnit`, vis/block map functions | none | reference only | HD bodies differ |
| `game/market.*`, `game/gameview.*`, `game/minimap.*`, `game/ingamemenu.*`, `game/singlemenu.*` | none | reference for layout and flow | 0-6% string overlap; lift from HD |

---

## Not determined

* The exact executed function set of the menu (needs the coverage trace); the 600-800 M2 figure is an estimate.
* Which driver class each menu unit type uses (inferred from class names, not read from the `.unit` files).
* Code-level similarity of drivers/ghost movement vs SWINE (`GAS_*`): compared by names, strings and
  structure only, not instruction by instruction; no shared constants were checked.
* Action ID alignment for 0x19, 0x1a, 0x38..0x3b (editor text order vs masks).
* SWINE object sizes for SUnit/SGameWorld (SWINE units live in arrays; no size annotation).
* SPanzersCampaign save-game format details; SGameView 0x619c90 contents.
* Precise split of MP-only code inside SGameLogic/SWorld.
