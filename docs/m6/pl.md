# M6-PL: finishing the partial lifts

Branch `m6-pl` (from master 103b364). Agent PL. The census counts the lifted bodies (`// PANZERS 0xADDR`)
that still reach a STUB_LOG; this branch takes that count from 61 to 36 and records why each of the
other 36 stays.

## Census

| | lifted | SWINE-shared | stubs | lifted bodies with a STUB_LOG |
|---|---|---|---|---|
| master 103b364 | 2531 | 1316 | 139 | 61 |
| m6-pl | 2556 | 1316 | 105 | 36 |

SWINE-shared unchanged. The list below comes from `scratchpad\m6pl\partials.py` (the census rule, one
line per lifted body with its first STUB_LOG). "hit" is from the M5 30-mission sweep
(`scratchpad\m5ms\m5int`, 2400 frames each), the coordinator's run logs and this branch's regress runs
(menu, tc1 and Tutorial replays). Most of the 61 were never hit: they are mission paths the sweep's
first 2400 frames do not reach (orders on buildings, towing, menus) or pure-in-HD placeholders.

## What was lifted (HD addresses)

- Unit speech: the World+0x726c queue (SHeap 0x38: alloc 0x5d8ed0, remove 0x5f7540) filled by
  `SWorld::UnitSpeech 0x5fff20`, played by `SWorld::UpdateSpeech 0x607f50` through Concert +0x50 (the
  highest priority first; ties: the higher group, then the older entry; entries of dead units and the
  unplayed priority-0 ones leave). The order voices 0x5bcdf0 / 0x5bcab0 / 0x5bca90 now call it, as do
  0x5bcad0 (by the selection's order 0x56d490, now `pz::SelectionActionOn`) and 0x5bc9d0 (gunner
  0x583b60). Audio only: the local player's events draw the sample with the CRT rand, as in HD.
  `PZ_M6_SPEECH_LOG=1` (recompile-only, inert when unset) logs each played sample; Allied 4 played 7
  "UnderAttack" samples in 2400 frames.
- `SUnit::TakeDamage 0x5c4080` combat music (Concert +0x6c / +0x70 / +0x78(0), "music/war_%02d.mp3";
  seen playing in Allied 4).
- `SUnit::GetBuildingAction 0x5ba2b0` (an enemy building is attacked when gunner 0 can hit it or it is
  type 2; otherwise 4 (get in) when 0x5b7040 takes the unit, else 1). `EC_AssaultBuilding 0x5b82b0`,
  `EC_Default 0x5b8ab0`, SUnit +0x84 0x5c0fa0 (rank XP), +0x9c 0x5c1d40 (+0x112; the squad's 0x5a0a10
  now overrides it).
- Vehicles and towing: `EnterVehicle 0x5b5810` pick-up (vehicle +0xd8), `OnDriverReachedTarget
  0x5bcb60` (+0x150), `SSingleUnit::RefreshTargeting 0x5aef40` hook-up (+0x58(this), within 9 m
  +0x150), `SSingleUnit::OnDriverReachedTarget 0x5aed30` tow (+0x6c), `SSingleUnit::EC_Move 0x5ac4b0`
  backwards (+0xb0).
- `SGunner::ServerRefresh 0x584d00`: the step back out of a building model (+0xd0, at most 100 steps).
- Effects on models: pixie +0x58 in `SWasterUnit::Explode 0x5d2880` (molotov fire on a building or a
  unit) and `PlayDeathEffects 0x5c26e0`; `SParticles` births from a model: `0x6e8680` (surface birth
  points), `0x6e1c20` (styles 1 / 2), `BirthAt 0x6e1610`. `SModel +0xbc 0x6dae10` writes HD +0xf4,
  which no HD code reads back (scan of 0x660000..0x720000), so it is a no-op here (`SetWreckModel`,
  `Init`).
- `SBuildingUnit` after load 0x549500: the capture flag (0x5471d0).
- `SWorld` vtable +0x04..+0x10 (0x5ec2a0, 0x5fec80, 0x5ec0c0, 0x5ebee0: trigger-variable value and
  names).
- Game view: the box-select reset 0x619580; the HUD Detach 0x576460 / Leave 0x5763f0(-1) packets and
  the command mode 0x6280f0 (Move, Equip1 / Equip2; the equipment modes +0x2a58 / +0x2a5c stay 0 since
  the HUD part that shows those buttons is not lifted); the in-game Options menu (`SOptionsMenu`
  0x62cc40 / 0x62fc10 / 0x631fb0, `OpenOptionsMenu 0x620570`, the page openers 0x61f7c0 / 0x61feb0 /
  0x61fe30, their OnAction 0x6216b0 and Esc 0x622f50 cases; Game Ok applies the fog view 0x57f970 and
  World+0x73ac; the key hint texts 0x626290 are not re-registered: no hint registry yet);
  `ShowBriefing 0x627db0` and the modal close 0x619520. Checked on screen in German 1: Menu ->
  Options -> Game Options / Graphics / Audio -> Ok / Cancel / Esc / Back; Objectives -> Briefing ->
  the picture -> click -> Objectives.
- Animations: the boat oars of `SWalkerAnimation::UpdateModel 0x5ce2a0` (scene +0x58, model +0xdc on
  "R Arm03" / "L Arm03"), the squad extra model 0x5c76a0 / 0x5cadc0.

## Still partial, and why

- Owned by other agents now: water (DrawSea / DrawRivers / DrawLakes / CreateLake), market (SaveArmy,
  the vehicle menu), skirmish army making, and ProcessPacket op 0x23 (army; a census artefact: it is the
  first STUB_LOG after the 0x5bcad0 marker, not part of that body).
- Not gaps: pure-in-HD placeholders (SPDriver::CreateDriver, SPUnitAnimation Load / CreateAnimation,
  SUnitAnimation InitModel / GetFirePosition) and the non-Miles `#else` of SSoundEffect::Process.
- The wire system (WIR3 load 0x5f3d60, nodes 0x5e3990 / 0x6090e0, removal 0x5f82b0, tearing
  0x5ffdc0 / 0x5e0f30, the flying-fox trucks 0x5f6bf0, DrawWires 0x6b8b10): five partials share it
  (CreateUnit, SBuildingUnit::Uninit, SDoodad::UpdatePosition, CrushDoodad, RefreshFlyingFox). It is a
  subsystem of its own with a renderer part; hit in 2 of 30 missions (Allied 1, German 10).
- Productive buildings (unit type 0x1a, 0x54a660 and the .productive files): multiplayer / skirmish,
  never hit in the campaign sweep.
- Never hit, renderer internals: the SMaterial vertex-shader techniques (the recompile draws fixed
  function), SParticles::MoveWaste 0x6e7a60, SPModel::LoadGeoFile 0x693190, the Gepard statistics
  0x67c3d0 and +0x54 0x67fb50, SScene +0x10 / +0x6c (pzscene.cpp, the renderer agent's file), SViewport
  SetFullScreenMode / +0x40 / +0x48 / +0x64, the effect editor's .fx writer (SPixie +0x14).
- `SWorld::ComputeCamera` mode 1 (2.2 KB): used by no campaign map or cut-scene.
- Results Save: the STopListMenu (0xbac, 0x633820 / 0x6398f0) is not lifted.
- SGameView callback +0x04 0x622c50 (campaign text box): no HD caller found.

## Determinism and runs

regress.ps1 on the final build (pl6.exe = HEAD): menu compared 1181 mismatches 0; tc1 replay common 3901 differing 0; Tutorial replay common 2012 differing 0; all three exits closed, no PANIC / EXCEPTION. Also exact on the intermediate builds pl1 and pl4.

Mission sweep (sweep.ps1 -Frames 2400, final build, tag pl6): German 1, Allied 2, Russian 3, Allied 7, Allied 8, German 8, Russian 8, Allied 1: all OK, no panic / exception. The partials still logged there: only SWorld::CreateUnit building wires (Allied 1). Earlier build (tag pl): German 1, Allied 2, Russian 3, Allied 8, Allied 4 OK.

Dropped / not done for time: the wire system, the in-game key hint texts 0x626290, the STopListMenu (Results Save).

## The 61 (before) and their state (after)

| file | HD marker | what the STUB_LOG skipped (its text) | hit | after M6-PL |
|---|---|---|---|---|
| `3dengine\pz\effecttypes.cpp` | 0x6ece40 | SSoundEffect::Process (0x6ece40): no Miles concert in this build | no | still partial: not a gap: the STUB_LOG is only in the non-Miles build (#else) |
| `3dengine\pz\mesh.cpp` | 0x6cbe00 | SMaterial::Begin vertex-shader technique (0x6cbe00 types 1..4 | no | still partial: renderer: the HD vertex-shader techniques are never created in the recompile (fixed function) |
| `3dengine\pz\particles.cpp` | 0x6e6cb0 | SParticles::MoveWaste (0x6e7a60 | no | still partial: MoveWaste 0x6e7a60: never hit (30-mission sweep), left |
| `3dengine\pz\particles.cpp` | 0x6e21a0 | SParticles::Birth from a model (0x6e1c20 | no | lifted: 0x6e8680 birth points, 0x6e1c20 births |
| `3dengine\pz\particles.cpp` | 0x6e1890 | SParticles::BirthAt (0x6e1610 | no | lifted: BirthAt 0x6e1610 |
| `3dengine\pz\pzgepard.cpp` | 0x67db20 | SPModel::LoadGeoFile (0x693190 | no | still partial: SPModel::LoadGeoFile 0x693190: never hit |
| `3dengine\pz\pzgepard.cpp` | 0x677fc0 | SPzGepard::Slot_08_Stats (0x67c3d0 | no | still partial: Gepard statistics 0x67c3d0 (debug): never hit |
| `3dengine\pz\pzgepard.cpp` | 0x681540 | SPzGepard::Slot_54 (0x67fb50 | no | still partial: Gepard +0x54 0x67fb50: never hit |
| `3dengine\pz\pzpixie.cpp` | 0x69dc80 | SPixie::Slot_14 (0x69ee50 | older coord logs only | still partial: SPixie +0x14 0x69ee50: .fx writer of the effect editor |
| `3dengine\pz\pzscene.cpp` | 0x6acaf0 | SScene::DrawSea (0x6b04d0 | sweep 20/30, menu | still partial: DrawSea: water, owned by the renderer agent |
| `3dengine\pz\pzscene.cpp` | 0x6aac20 | SScene::DrawRivers (0x6b0920 | no | still partial: DrawRivers: water, owned by the renderer agent |
| `3dengine\pz\pzscene.cpp` | 0x6b7760 | SScene::DrawLakes (0x6ad740)");       // LAKS | sweep 30/30, menu, tc1 | still partial: DrawLakes: water, owned by the renderer agent |
| `3dengine\pz\pzscene.cpp` | 0x6b0000 | SScene::Slot_10 (0x6ac7d0 | no | still partial: SScene +0x10 0x6ac7d0: pzscene.cpp (renderer agent file), never hit |
| `3dengine\pz\pzscene.cpp` | 0x6ba810 | SScene::Slot_6C (0x6a9000 | no | still partial: SScene +0x6c 0x6a9000: pzscene.cpp (renderer agent file), never hit |
| `3dengine\pz\pzscene.cpp` | 0x6ad0e0 | SScene::CreateLake (0x6a7940 | no | still partial: CreateLake: water, owned by the renderer agent |
| `3dengine\pz\pzviewport.cpp` | 0x68c4e0 | SViewport::SetFullScreenMode (0x68ca00 | no | still partial: SViewport::SetFullScreenMode 0x68ca00: never hit (windowed path) |
| `3dengine\pz\pzviewport.cpp` | 0x6898b0 | SViewport::Slot_40 (0x68da50 | no | still partial: SViewport +0x40 0x68da50: never hit |
| `3dengine\pz\pzviewport.cpp` | 0x68b940 | SViewport::Slot_48 (0x68bbf0 | no | still partial: SViewport +0x48 0x68bbf0: never hit |
| `3dengine\pz\pzviewport.cpp` | 0x68b930 | SViewport::Slot_64 (0x68b470 | no | still partial: SViewport +0x64 0x68b470: never hit |
| `game\buildingunit.cpp` | 0x547690 | SWorld 0x5f82b0 (remove a wire), called by SBuildingUnit::Uninit 0x547690 | no | still partial: wires: the WIR3 wire system (load 0x5f3d60, 0x5e3990, 0x6090e0, 0x5f82b0, DrawWires 0x6b8b10) is not lifted |
| `game\buildingunit.cpp` | 0x54a250 | SBuildingUnit::RefreshTargeting (0x54a250) productive building 0x54a660 | no | still partial: productive buildings (unit type 0x1a, multiplayer / skirmish): never hit in the campaign sweep |
| `game\combat.cpp` | 0x5c4080 | SUnit::TakeDamage (0x5c4080) combat music (Panzers concert +0x6c/+0x70/+0x78 | sweep 1/30 (Allied 4), tc1 replay | lifted: Concert +0x6c/+0x70/+0x78 (PzPlayTriggerMusic) |
| `game\combat_speech.cpp` | 0x5fff20 | SWorld::UnitSpeech (0x5fff20) speech queue World+0x726c (0x5d8ed0) and playback 0x607f50 | sweep 3/30 (Allied 4, Allied 8, German 8) | lifted: queue 0x5d8ed0 / 0x5f7540, UpdateSpeech 0x607f50 (Concert +0x50) |
| `game\driver.cpp` | 0x55c820 | SPDriver::CreateDriver (driver type not used in the menu | no | still partial: not a gap: SPDriver::CreateDriver is pure in HD |
| `game\gunner.cpp` | 0x584d00 | SGunner::ServerRefresh (0x584d00) building incidence: model +0xd0 point test | no | lifted: building model +0xd0 step-back loop |
| `game\packets.cpp` | 0x5bba70 | SWorld::UnitSpeech (0x5fff20) order acknowledgement | older coord logs (tc1 replays before M5) | lifted: WorldSpeech = UnitSpeech(unit, kind, 0) |
| `game\packets.cpp` | 0x5bca90 | SUnit::0x5bcad0 / 0x5bc9d0 attack acknowledgement (0x56d490, 0x583b60 | no | lifted: 0x5bcad0 (selection action 0x56d490) and 0x5bc9d0 (gunner 0x583b60) |
| `game\punit.cpp` | 0x5a7500 | SPBuildingUnit::LoadResources (0x5a7500) productive building | no | still partial: productive buildings (.productive files): multiplayer / skirmish |
| `game\singleunit.cpp` | 0x5aed30 | SSingleUnit::OnDriverReachedTarget (0x5aed30) tow: unit +0x58 (C5) / +0x6c (0x5c2390 | no | lifted: target +0x58(this), +0x6c Tow |
| `game\unit.cpp` | 0x55ce60 | SUnit::Slot_84 (0x5c0fa0 | no | lifted: SUnit +0x84 0x5c0fa0 (SetRankXP) |
| `game\unit.cpp` | 0x55cee0 | SUnit::Slot_9C (0x5c1d40 | no | lifted: SUnit +0x9c 0x5c1d40, EC_Default 0x5b8ab0 |
| `game\unit.cpp` | 0x5b9170 | SUnit::EC_AssaultBuilding (0x5b82b0 | no | lifted: EC_AssaultBuilding 0x5b82b0 |
| `game\unit_afterload.cpp` | 0x549500 | SBuildingUnit 0x5471d0 (the capture flag model), called by SBuildingUnit after load 0x549500 | no | lifted: UpdateCaptureFlag 0x5471d0 |
| `game\unitai.cpp` | 0x5b5810 | SUnit::EnterVehicle (0x5b5810) vehicle +0xd8(unit) pick-up, then +0xc4 | no | lifted: vehicle +0xd8 (EC_Tow) |
| `game\unitai.cpp` | 0x5ba2b0 | SUnit::GetBuildingAction (0x5ba2b0) building | no | lifted (0x56d2a0, 0x583b60, building type 2, 0x5b7040) |
| `game\unitai.cpp` | 0x5bcb60 | SUnit::OnDriverReachedTarget (0x5bcb60) +0x150 (enter the vehicle | no | lifted: +0x150(primary unit) |
| `game\unitai.cpp` | 0x5aef40 | SSingleUnit::RefreshTargeting (0x5aef40) vehicle entry (+0x58, +0x150 | no | lifted: +0x58(this), new kind-0 target, within 9 m +0x150 |
| `game\unitai.cpp` | 0x5ac4b0 | SSingleUnit::EC_Move (0x5ac4b0) +0xb0 move backwards | no | lifted: +0xb0 EC_MoveReverse |
| `game\unitanim.cpp` | 0x5c7290 | SPUnitAnimation::Load (pure in HD SPUnitAnimation | no | still partial: not a gap: pure in HD |
| `game\unitanim.cpp` | 0x5cb470 | SPUnitAnimation::CreateAnimation (pure in HD SPUnitAnimation | no | still partial: not a gap: pure in HD |
| `game\unitanim.cpp` | 0x5c74c0 | SUnitAnimation::InitModel (pure in HD SUnitAnimation | no | still partial: not a gap: pure in HD (InitModel, UpdateModel) |
| `game\unitanim.cpp` | 0x5c76c0 | SUnitAnimation::GetFirePosition (pure in HD SUnitAnimation | no | still partial: not a gap: pure in HD |
| `game\unitanim.cpp` | 0x5cb240 | SSquadAnimation::Slot_0C (0x5c76a0) extra model placement 0x5cadc0 | no | lifted: 0x5c76a0 / 0x5cadc0 |
| `game\unitanim.cpp` | 0x5ce2a0 | SWalkerAnimation::UpdateModel: boat oars (SIModel +0xdc attach) not lifted | no | lifted: oar scene +0x58, +0xdc attach |
| `game\unitbase.cpp` | 0x5c26e0 | SUnit::PlayDeathEffects (0x5c26e0) pixie +0x58(effect, model) (slot untyped, E | no | lifted: pixie +0x58 |
| `game\unitbase.cpp` | 0x5be2b0 | SUnit::SetWreckModel (0x5be2b0) model +0xbc(3) (slot untyped, E | no | lifted: SModel +0xbc 0x6dae10 (stores HD +0xf4, which no HD code reads) |
| `game\waster.cpp` | 0x5d2880 | SWasterUnit::Explode (0x5d2880) pixie +0x58 (fire on the building | sweep 2/30 (Allied 7, Allied 8: the molotov-on-unit STUB_LOG of the same body) | lifted: pixie +0x58 SetEffectModel (both sites) |
| `panzers\gameview_view.cpp` | 0x622b30 | SGameView callback +0x04 (0x622c50 | no | still partial: SGameView callback +0x04 0x622c50: no HD caller found (scan of the logic code) |
| `panzers\hud.cpp` | 0x625d80 | SGameView 0x6216b0: Detach 0x576460 (no builder in packets.h | no | lifted: Detach 0x576460, Leave 0x5763f0(-1); then 0x6280f0 Move / Equip |
| `panzers\ingamemenu.cpp` | 0x619580 | SGameView 0x619580: mouse mode 1 -> 0x5ddb60 (not mapped | no | lifted: World 0x5ddb60 |
| `panzers\ingamemenu.cpp` | 0x6216b0 | SGameView::OpenOptionsMenu (0x620570 | no | lifted: SOptionsMenu 0x62cc40 / 0x62fc10 / 0x631fb0, 0x620570, pages, Esc; then ShowBriefing 0x627db0 / 0x619520 |
| `panzers\market.cpp` | 0x64a180 | SMarket::SaveArmy (0x64a180 | no | still partial: market: owned by another agent |
| `panzers\market.cpp` | 0x647d00 | SMarketVehicleMenu (0x648c20): the vehicle menu is not lifted | no | still partial: market: owned by another agent |
| `panzers\results.cpp` | 0x63c230 | SResultsMenu Save -> SSaveMenu (0x633820 / 0x6398f0 | no | still partial: Results Save: the STopListMenu (0xbac, 0x633820 / 0x6398f0, 3.4 KB) is not lifted |
| `panzers\superwindow_sk.cpp` | 0x658e70 | SSuperWindow::OnAction 0x534b3 / 0x534b4: army making in the market (0x658b10 case 1, SMarket multi mode, SaveArmy 0x64a180) not lifted | no | still partial: skirmish army making: owned by another agent |
| `world\mapload.cpp` | 0x6012d0 | SDoodad::UpdatePosition (0x6012d0) attached lights 0x6090e0 | no | still partial: wires (doodad wire nodes 0x6090e0): see 0x547690 |
| `world\unit.cpp` | 0x5e3170 | SWorld::CreateUnit (0x5e3170) building wires ("wire%d" nodes | sweep 2/30 (Allied 1, German 10) | still partial: wires ("wire%d" nodes): see 0x547690 |
| `world\world.cpp` | 0x5d5510 | SWorld::Slot_04 (0x5ec2a0 | no | lifted: SWorld +0x04..+0x10 (trigger variable value / names) |
| `world\world.cpp` | 0x5ddc30 | SWorld::ComputeCamera mode 1 (0x5ddc30 | no | still partial: camera mode 1 (2.2 KB): used by no campaign map or cut-scene |
| `world\worldunits.cpp` | 0x5e3c80 | SWorld::CrushDoodad (0x5e3c80) doodad lights 0x5ffdc0 / 0x5e0f30 | no | still partial: wires (tear 0x5ffdc0 / 0x5e0f30): see 0x547690 |
| `world\worldunits.cpp` | 0x5f6bf0 | SWorld::RefreshFlyingFox (0x5f6bf0) flying-fox trucks | no | still partial: flying-fox trucks on the wires: see 0x547690 |

