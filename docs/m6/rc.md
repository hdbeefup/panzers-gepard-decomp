# M6-RC: reccmp drift triage

Branch `m6-rc` from master 103b364 (+ bb1257c, the regenerated reccmp-functions.csv).
Input: the reccmp run on master (2531 markers -> 1890 pairs, 561 unresolved, effective
accuracy 35.2%, rescore shape median 0.597). The 150 "short" pairs are the ones where our
body has under half of HD's instructions and HD has at least 80 (rescore columns
`orig_ins` / `recomp_ins`).

Classes:

- **a** wrong marker address or wrong pairing
- **b** HD body split into several of our helpers (fine; helpers get `(piece)` markers)
- **c** inlined / table-driven / codegen difference, or a part that is multiplayer or a
  debug switch (fine)
- **d** genuinely missing single-player behaviour
- **own** the function belongs to an agent running now (market / skirmish, cut-scene,
  renderer, PL for bodies with a STUB_LOG); left to them

## Counts (150 short pairs)

| class | count | notes |
|---|---|---|
| a | 5 | 4 fixed, 1 deliberate (SAchimMenu::OnMouseUp carries HD's OnMouseDown address) |
| b | 49 | 4 new `(piece)` markers (rail scan / candidate, InitCrewAndChildren, flora) |
| c | 63 | SSettings::Save and BlockMap_CheckStaticInternal are here (see below) |
| d | 19 | 5 lifted or partly lifted here; 5 are PL's STUB_LOG bodies; the rest listed below |
| own | 19 | market 6, cut-scene 5, renderer 6, skirmish 2 (some rows carry two classes) |

### The examples asked about

- **SSettings::Save 0x64fdd0** (4068/475): every key and comment line HD writes is written,
  in HD order, including HD's own key spellings ("Unit acknowledgement", "Keyboard
  bindings" that the loader does not read back). HD formats each line with an inline
  SString (34 calls to 0x51ee20 plus the frees); ours has one writer helper. The one gap
  is the RG Pass XOR (0x64ddb0 / 0x64d7d0, volume-serial key), skipped on load and save
  alike; RankedGaming is multiplayer. Class c.
- **SGameView::Update 0x628430** (4139/230): **d**. The selection panel is missing: HD
  builds a selection summary (0x56b3e0, 7 KB: heroes, command flags, the shared target
  kind, the panel unit) and then fills the unit panel (0x629497..0x62b9xx: up to 16
  SUnitButton::SetUnit, name, stars, crew, HP / ammo / cargo / XP / thermo bars, the
  14 command buttons and their states, the armour texts, the productive-building
  slots). PzHudUpdate only empties the panel. Lifted here: the group icons and the hero
  photos (below). The panel itself is about 1800 HD instructions over ~60 HUD members;
  it needs its own agent.
- **BlockMap_CheckStaticInternal 0x5da050** (3153/110): c. HD unrolls the disc per unit
  size; one loop, emulator-verified for every size. Only difference: the "Not optimized"
  warning is logged once per size, HD logs it on every call.
- **PzTargetCursor 0x620d7c**, **SMinimap::SMinimap 0x808ff0**, **SWorld::RefreshModels
  0x576d80**: a (fixed, below).
- **SSkinnedMesh::DrawShadowSkinned 0x6d0e30** (1189/27): b. The CPU skinning is
  FillSkinned 0x6d2200, which HD inlines; FillSkinned has its own marker and pair.
- **SPixie::SPixie 0x6941e0** (9476/92): c. The 35 KB is the inline property-schema build,
  generated into effectschema.cpp.

## Fixed markers (class a)

| marker | was | now |
|---|---|---|
| 0x808ff0 (minimap.cpp) | `// PANZERS 0x808ff0` over a data constant (1/172); reccmp paired the SMinimap ctor with data | `// HD 0x808ff0 (data)` |
| 0x620d7c PzTargetCursor | not a function entry: case 4 inside MouseCamera 0x620bc0 | `// PANZERS 0x620bc0 (piece: ...)` |
| 0x565f1d MinimapFog | not a function entry: inside the vision tick 0x565e10 | `// PANZERS 0x565e10 (piece: ...)` |
| 0x576d80 SWorld::RefreshModels | the M1 model subset carried SGameLogic::Refresh's address | piece; `SGameLogic::RefreshM2` (the HD single-player body) carries 0x576d80 |
| 0x6201c0 SGameView::LoadMap (unresolved) | a forward declaration sat between the marker and the body | declaration moved above the marker |

All other 2556 marker addresses are Ghidra function entries (checked against
build/pz_funcs.tsv); the only non-entry left is 0x6875a0 MilesStreamCallback, a callback
Ghidra missed (docs/RECCMP.md).

## Unresolved markers (561 on master)

505 have no PDB body under that name: markers in headers next to the declaration (the
cpp body is paired; for example SSettings::Save / Initialize / SetKeyboardBindings in
settings.h, ShadowReceiverBounds / ShadowTrapezoidMatrix in shadowmath.h, the rail
functions in rail.h), static helpers inlined by the compiler (AStar_FindPath,
Wpm_CreateManoeuvres, M34Inverse44, BlockMap_RefreshDirtyRect), and empty bodies folded
by /OPT:ICF. 38 are "next marker first" (empty virtuals such as SEffect::SetSpeed {}, the
SUnitAnimation getters, inlined one-liners); one of them was a real misplacement
(LoadMap, fixed). 18 are in-body markers of inline SDArray::Add / Remove copies (no
identifier follows them). None of the 25 biggest (by HD size) is a wrong address.

## Lifted here (class d)

| HD | what | file |
|---|---|---|
| 0x5f5970 | SWorld group summary: which groups 1..9 have units, the one fully selected group | hud.cpp |
| 0x626200 | SGroupIcon redraw (selected / normal glyphs) | hud.cpp |
| 0x628430 part | the top-bar group icons and the group button glyph 0x14 / 0x15 (0x628ed8..0x628f64) | hud.cpp PzHudUpdate |
| 0x626690 | SHeroUnitButton::SetHero: photo, HP bar of the crew / the hero, the red box pulsing 100 frames after an HP loss | hud.cpp |
| 0x625b60, 0x625c40 | SHeroUnitButton red box level, HP bar colour and width | hud.cpp |
| 0x56b3e0 part | the hero list (first five live hero units of the local player) | hud.cpp HudHeroList |
| 0x5f02a0, 0x5f0e30 | AMBS: the map's ambient sound sources (World +0x658 heap, 'AMBI' v100 records, sounds cached) | mapload.cpp |
| 0x5dd3e0, 0x5f72c0 | clear / remove ambient sources (Concert RemoveSound) | mapload.cpp, ai.cpp, world.cpp dtor |
| 0x5f5b50, 0x5edeb0 | SWorld::StartEffects starts every ambient source: 3D loop at the ground height + y, 1.2 x the min distance | ai.cpp |
| 0x5f5140 | disable_ambient_sounds: the loops stop | ai.cpp, triggers_script.cpp |
| 0x560eb0 | AddMinimapObjective: the minimap marker sprite (+0x17c parent, +0x1a0 / +0x1a4 glyph) or the marked-area corner (board +0xb8) | gamelogic_save.cpp |
| 0x579420 board part | the markers' board frames / marked area go with the record | triggers.cpp |
| 0x579420 call in LoadGameState | the old markers go before a load (HD order) | gamelogic_save.cpp |
| 0x59cf80 | SPanzersSquadUnit after load: the squad's board elements again (name text, insignia, marker, stars) | unitboard.cpp, unit_afterload.cpp |

## Class d not lifted

| HD | what is missing | why not here |
|---|---|---|
| 0x628430 / 0x56b3e0 | the unit selection panel | ~1800 HD instructions; needs a dedicated agent (hud.cpp has every member laid out) |
| 0x578b00 | the alpha fade of a message line's last 20 ticks; the TimeCounter / UnitCounter texts | the SWINE SBoard::SetTextColor forces alpha 0xff and has no GetTextColor (HD board +0x2c / +0x30); a board change touches every text |
| 0x5cfe30 | the units.ini display names (+0x68 / +0x70 / +0x78) | market.cpp LoadUnitDisplayNames fills them (market agent's file; TODO there to move it into the ctor) |
| 0x601c10 | the road PathPoints of the path finder | not started |
| 0x5f1fd0 | loaded doodad animations are not restarted on their doodads | not started |
| 0x5a8780 | Die_Effects "_01" repetition over "<name>02", "<name>03" nodes | needs a prototype node lookup in the Gepard facade |
| 0x622f50 | Tab focus, F11, modal-box keys (StubOnce) | small; Esc in a cut-scene is the cut-scene agent's |
| 0x634d60, 0x5426c0 | splash SButton child; selectable text boxes | no single-player effect |
| 0x608600, 0x607ad0 | lakes / rivers raising the water map, lake scene objects | water (renderer agent); LAKS / RVR2 are kept raw |
| 0x5e3c80, 0x5a7500, 0x5f6bf0, 0x5ba2b0, 0x6216b0 | STUB_LOG parts | agent PL |
| 0x6dba80 | ':' node effects (pixie +0x30 / +0x34); the model's terrain shadow decal | pixie slots / blob shadows (renderer) |

## Before / after

| | master 103b364 | m6-rc |
|---|---|---|
| markers (census lifted) | 2531 | 2548 |
| pairs | 1890 | 1896 |
| unresolved markers | 561 | 564 (the new piece markers lose to the plain marker of their address) |
| effective accuracy (mean reccmp) | 35.22% | 35.26% |
| rescore regs median | 0.483 | 0.483 |
| rescore shape median | 0.597 | 0.598 |
| short pairs (ours < 1/2 HD, HD >= 80 ins) | 150 | 146 |

New or changed pairs: SGameLogic::RefreshM2 now carries 0x576d80 (1701 vs 412 ins, shape
0.17; was the 61-instruction M1 subset), SGameView::LoadMap 0x6201c0 is paired (shape 0.68),
SHeroUnitButton::SetBlink 0x625b60 shape 1.00, SetHp 0x625c40 0.73, RemoveAmbientSound
0x5f72c0 0.74, LoadAmbientSounds 0x5f02a0 0.52; StartEffects 0x5f5b50 12 -> 94 ins,
AddMinimapObjective 0x560eb0 47 -> 126 ins.

## Regression

Final exe (0b5596a), coord/regress.ps1: menu compared 1181 mismatches 0; tc1 replay 3901/3901
equal; tutorial replay 2012/2012 equal; all three runs "exit: closed", no PANIC / EXCEPTION.
Census 2531 -> 2548 lifted (+18 new markers incl. 4 pieces, -1 for the 0x808ff0 data
marker), SWINE-shared 1316 unchanged, stubs 139 unchanged. A load of SaveGames/G-01-Start.save
(PZ_M4_LOADGAME) runs through the squads' after-load slot without a panic.

## Other

- src/3dengine/pz/effectschema.cpp: the non-UTF-8 bytes were not comments but the cp1250
  Hungarian help strings of the effect schema (HD's own data). They are now `\xNN`
  escapes: the same bytes in the exe, and the file is ASCII, so reccmp reads it.
- Files touched outside src/panzers/hud.*: world (mapload, ai, world.cpp/h),
  game (triggers.cpp RemoveObjectiveMarkers, triggers_script.cpp case 0x2c,
  gamelogic.cpp marker only, gamelogic_save.cpp, unitboard.cpp, unit_afterload.cpp,
  rail.cpp / singleunit.cpp markers only), 3dengine (pzterrain.cpp marker only,
  effectschema.cpp escapes), panzers (gameview_mission.cpp, hudcursor.cpp, minimap.cpp
  markers only).
- Not verified against the original: the hero photo and group icons were seen on screen
  in German 1 (hero photo with its green bar at the right edge); the ambient loops and the
  objective markers were not compared with HD. Reference worth recording from the original:
  German 1 after the intro cut-scene (about 3 minutes, no input): a screenshot of the top
  bar, the right edge (hero photo) and the minimap (objective marker), and whether an
  ambient loop is audible at the start position.

## Full table

| HD | function | HD/ours ins | class | note |
|---|---|---|---|---|
| 0x6941e0 | pz::SPixie::SPixie | 9476/92 | c | the 35 KB schema build is generated in effectschema.cpp (BuildEffectsSchema / BuildEfxSchema) |
| 0x619c90 | PzHudCreate | 4162/1792 | b | the HUD half of SGameView::Create; the loading screen part is in gameview_loading.cpp |
| 0x628430 | SGameView::Update | 4139/230 | d | the selection panel (0x56b3e0 summary + 0x629497..0x62b9xx display) is missing; multiplayer and -movierec parts n/a. LIFTED here: hero photos and group icons (see below) |
| 0x64fdd0 | SSettings::Save | 4068/475 | c | every options.ini key and comment HD writes is written, same order; HD inlines the SString formatting (34 x 0x51ee20). RG Pass XOR encoding (0x64ddb0) skipped on load and save alike (RankedGaming, multiplayer) |
| 0x704fd0 | pz::SParcel2::SParcel2 | 3670/1699 | c | vector-based rewrite of the bake, same steps |
| 0x6360f0 | SResultsMenu::Create | 3206/1222 | c | multiplayer / scenario variants not in the recompile |
| 0x6407d0 | SMarket::Create | 3154/1443 | own | market agent; multiplayer titles not lifted |
| 0x5da050 | pz::BlockMap_CheckStaticInternal | 3153/110 | c | HD unrolls per unit size; one loop, emulator-verified |
| 0x6450c0 | SMarket::LoadUnitInfo | 2908/1066 | own | market agent |
| 0x6c7150 | SBoard::RenderHdMinimap | 2490/705 | c | x87 / helper inlining |
| 0x584d00 | pz::SGunner::ServerRefresh | 2353/1142 | c | HD inlines target / effect helpers |
| 0x620d7c | PzTargetCursor | 1953/38 | a | FIXED: 0x620d7c is inside MouseCamera 0x620bc0; now a (piece) of 0x620bc0 |
| 0x808ff0 | SMinimap::SMinimap | 1950/17 | a | FIXED: 0x808ff0 is a data constant (1/172), the marker paired the SMinimap ctor with data |
| 0x576d80 | pz::SWorld::RefreshModels | 1701/61 | a | FIXED: 0x576d80 is SGameLogic::Refresh; RefreshM2 now carries it, RefreshModels is a (piece) |
| 0x659250 | SSuperWindow::OnAction | 1617/223 | b | split into SuperWindowOptionsAction / SuperWindowM3Action / PzLoadGameAction |
| 0x6b8b10 | pz::SScene::DrawWires | 1588/727 | own | renderer: technique-4 shadow vertex shader not ported |
| 0x622f50 | SGameView::OnKeyDown | 1469/513 | d | small: Tab focus, F11 and the modal-box keys are StubOnce; Esc in a cut-scene is the cut-scene agent's |
| 0x6216b0 | PzGameViewMenuAction | 1298/594 | b | the rest of OnAction is in hud.cpp / ingamemenu.cpp; ShowBriefing 0x627db0 STUB_LOG (PL) |
| 0x561110 | pz::SGameLogic::BackupCampaignUnits | 1231/360 | c | multiplayer army save not in the recompile |
| 0x5a8780 | pz::SPUnit::LoadResources | 1209/182 | d | small: Die_Effects "_01" repetition needs a prototype node lookup the Gepard facade lacks |
| 0x6d0e30 | pz::SSkinnedMesh::DrawShadowSkinned | 1189/27 | b | CPU skinning is FillSkinned 0x6d2200 (own marker); HD inlines it |
| 0x55e440 | pz::SGameLogic::SGameLogic | 1159/430 | c | memset instead of field-by-field; multiplayer parts |
| 0x6a27f0 | pz::ShadowReceiverBounds | 1154/402 | c | clip polyhedron rewritten as plane triples |
| 0x571c70 | pz::SGameLogic::PlaceAllUnits | 1123/308 | b | skirmish branch in PzSkirmishPlaceAllUnits (skirmish agent) |
| 0x6d2200 | pz::SSkinnedMesh::FillSkinned | 1070/220 | c | HD unrolls per bone count |
| 0x5c93a0 | pz::SVehicleAnimation::InitModel | 1032/314 | c | node-name loops instead of unrolled sprintf |
| 0x626c40 | SUnitButton::SetUnit | 1019/418 | c | same structure, codegen |
| 0x5f5c70 | pz::SAIGroup::Refresh | 1010/408 | c | codegen |
| 0x5e8750 | pz::RailFindNearestRoad | 956/80 | b | ScanRoad / CandidateDist inline in HD: now (piece) markers |
| 0x6f33c0 | pz::STerrain::RenderLate | 939/24 | b+own | flora part is RenderFlora (now a piece); HD draws flora back to front by camera-yaw octant, ours row by row (renderer) |
| 0x5d1050 | pz::SUnitRegistry::LoadUnitFiles | 930/418 | c | codegen |
| 0x5cfe30 | pz::SUnitRegistry::SUnitRegistry | 892/353 | d | small: the units.ini display names (+0x68/+0x70/+0x78) are filled by market.cpp LoadUnitDisplayNames instead (market agent's file, TODO there) |
| 0x5d5510 | pz::SWorld::~SWorld | 891/289 | c | helpers; the ambient-sound removal (0x5f72c0) is LIFTED here |
| 0x5be320 | pz::SUnit::Save | 887/346 | c | chunk writers factored into helpers |
| 0x672dd0 | pz::Tangents3 | 884/322 | own | cut-scene agent; codegen |
| 0x601c10 | pz::BuildRoadLengths | 881/198 | d | the PathPoints of the path finder are not built (only the lengths) |
| 0x5664f0 | pz::SGameLogic::CastVisCone | 866/273 | c | octant table |
| 0x54b910 | pz::SBuildingUnit::RefreshMisc | 819/172 | b | RefreshOccupants is HD inline 0x54b955..0x54c291 |
| 0x6f7810 | pz::STerrain::RenderRoads | 784/218 | c | STerrainDrawState helper |
| 0x5e7ae0 | pz::RailProjectOnRoad | 770/58 | b | see 0x5e8750 |
| 0x689f80 | PzGetModeLists | 757/252 | c | std::vector; HD also keeps the windowed lists (unused by the menu) |
| 0x5d2f90 | pz::SWorld::SWorld | 726/359 | c | memset of the object |
| 0x66f100 | SBitmap::NextMipLevel | 699/212 | c | one loop over bpp instead of three format paths |
| 0x5ad9f0 | pz::SSingleUnit::InitNew | 693/26 | b | InitCrewAndChildren (now a piece of 0x5ad150) is inline in both HD Init and InitNew |
| 0x54d380 | pz::SBuildingUnit::StoreUnit | 691/277 | c | codegen |
| 0x5a7e70 | pz::SPProjectileUnit::LoadResources | 689/51 | b | LoadEffectArray inline x5 in HD |
| 0x5e3c80 | pz::SWorld::CrushDoodad | 679/337 | d | STUB_LOG doodad lights 0x5ffdc0 / 0x5e0f30 (PL) |
| 0x62eda0 | SInGameBriefingMenu::Create | 642/298 | c | multiplayer game-mode texts n/a |
| 0x6dba80 | pz::SModel::Update | 640/209 | d+own | ':' node effects (pixie +0x30/+0x34) not started; the terrain shadow decal is the renderer's (blob shadows) |
| 0x5a7500 | pz::SPBuildingUnit::LoadResources | 639/46 | d | STUB_LOG productive buildings (PL) |
| 0x5ad150 | pz::SSingleUnit::Init | 635/22 | b | see 0x5ad9f0 |
| 0x671980 | pz::STrackRotationTCB::STrackRotationTCB | 626/284 | c | helpers |
| 0x620bc0 | SGameView::MouseCamera | 626/284 | b | HoverCursor / PzTargetCursor pieces |
| 0x69dc80 | pz::SPixie::LoadEffectPrototype | 604/296 | c | codegen |
| 0x5f1fd0 | pz::LoadWorldDoodadAnims | 576/118 | d | the loaded doodad animations are not restarted on their doodads (logs STUB) |
| 0x673c00 | pz::Tangents1 | 572/178 | own | cut-scene agent; codegen |
| 0x5c01e0 | pz::SUnit::RefreshRepairTarget | 545/148 | c | HD inlines SetActiveDriver / ThrowServiceProjectile |
| 0x618aa0 | SGameView::~SGameView | 511/180 | b | PzGameViewDeleteMenus / PzHudDestroy |
| 0x6499a0 | SMarket::FillWarehouse | 503/243 | own | market agent |
| 0x5c8d00 | pz::SFlyingAnimation::InitModel | 499/210 | c | codegen |
| 0x655ab0 | SSkirmishChatRoomMenu::FillMapList | 480/229 | own | skirmish agent |
| 0x70a930 | pz::SParcel::DrawSketch | 473/198 | b+own | FillParcel helper; vertex shader 0x41 path not ported (renderer) |
| 0x6492d0 | SMarket::FillArmyList | 473/224 | own | market agent |
| 0x5bf280 | pz::SUnit::RefreshSupplyTarget | 461/180 | c | helpers inline in HD |
| 0x6d0400 | pz::SAnimesh::DrawShadowFramesBlend | 454/37 | b | FillFramesBlend (vertex fill piece) |
| 0x58b170 | pz::InitColours | 442/137 | own | cut-scene agent |
| 0x67ea30 | pz::SPzGepard::LoadTexture | 429/123 | c | decoding stays SWINE's (ENGINE_DIFF 2) |
| 0x557520 | pz::SPDriver::LoadSubProperties | 422/183 | c | effect-array helper |
| 0x5ca110 | pz::SWalkerAnimation::InitModel | 403/164 | own | blob shadow decal (model +0xcc) is the renderer's |
| 0x6181f0 | SGameView::SGameView | 400/75 | c | HD constructs the member widgets inline |
| 0x6cdb70 | pz::SMesh::DrawShadow | 389/52 | b | DrawShadowMaterials |
| 0x6cfe00 | pz::SAnimesh::DrawShadowFramesLerp | 381/33 | b | FillFramesLerp (vertex fill piece) |
| 0x565f1d | MinimapFog | 378/113 | a | FIXED: inside 0x565e10; now a (piece) of 0x565e10 |
| 0x65ae50 | SSuperWindow::OnIdle | 363/165 | c | -movierec capture n/a (debug) |
| 0x609760 | pz::SWorld::InitCameraSpline | 357/32 | c | -csrec / -csplay debug switches not lifted |
| 0x593740 | pz::SPanzersCampaign::LoadMissionProps | 351/136 | c | loop over the directories |
| 0x58b760 | pz::InitCamera | 346/126 | own | cut-scene agent |
| 0x567760 | pz::SGameLogic::SupportHeavyBomber | 344/148 | b | PlaneStart / RaisePlane / DispatchLocations |
| 0x567d40 | pz::SGameLogic::SupportParatroopers | 343/146 | b | same helpers |
| 0x6cf500 | pz::SAnimesh::DrawFramesBlend | 327/22 | b | FillFramesBlend |
| 0x5a6f10 | pz::SPUnit::InitDrivers | 321/100 | c | PzCreatePDriver factory instead of inline ctors |
| 0x5b0f00 | pz::STrainUnit::GhostFramesAddTop | 312/131 | b | HookAndHole / TowBehindHook |
| 0x5f6bf0 | pz::SWorld::RefreshFlyingFox | 310/9 | d | STUB_LOG flying-fox trucks (PL) |
| 0x5ca980 | pz::SPWalkerAnimation::LoadResourcesProps | 309/115 | b | CountSequences |
| 0x6fcee0 | pz::STerrain::UpdateRoadHeights | 297/116 | b | HeightAt / NormalAt inline in HD |
| 0x545940 | pz::SBuildingUnit::SBuildingUnit | 292/77 | c | memset |
| 0x578b00 | pz::PzMessagesTick | 286/66 | d | the alpha fade of the last 20 ticks is missing: the SWINE SBoard::SetTextColor forces alpha 0xff and has no GetTextColor (+0x30); the TimeCounter / UnitCounter texts (0x578b00 second half) are not lifted |
| 0x5e65f0 | pz::SWorld::FixBridges | 285/106 | b | ApplyHeightPatch |
| 0x6cf9c0 | pz::SAnimesh::DrawShadowFrame | 284/29 | b | FillFrame |
| 0x607f50 | pz::PzSpeechTick | 275/55 | b | the unit speech heap part is elsewhere |
| 0x581c50 | pz::TypeMatches | 273/102 | c | UV accessors |
| 0x5b5c10 | pz::SUnit::GhostFramesAddTop | 270/80 | b | HookAndHole / TowBehindHook |
| 0x5ef760 | pz::SWorld::UnitStored | 264/106 | c | codegen |
| 0x6cf140 | pz::SAnimesh::DrawFramesLerp | 260/18 | b | FillFramesLerp |
| 0x583570 | pz::SGunner::TurnElevation | 259/17 | b | GunnerTurnAxis |
| 0x69ea50 | pz::SPixie::Render | 254/64 | own | lens flares (heap +0x3c) not ported: renderer |
| 0x543d90 | PzDrawFrameBox | 252/99 | c | piece table |
| 0x53ae80 | pz::SEditBox::Update | 237/95 | c | codegen |
| 0x708400 | pz::SWireframeParcel::SWireframeParcel | 225/102 | c | codegen |
| 0x63c580 | SSingleDiffMenu::OnAction | 214/90 | c | text table |
| 0x6f5840 | pz::STerrain::SetRoad | 212/102 | c | codegen |
| 0x64f860 | SSettings::SetKeyboardBindings | 211/48 | c | 27-name table instead of 27 inline calls |
| 0x7c5fb0 | pz::M44Mul | 204/72 | c | loop instead of unrolled |
| 0x632bd0 | SInGameBriefingMenu::AddObjective | 203/76 | c | codegen |
| 0x68c220 | pz::SViewport::Render | 193/92 | c | frame restructured around the SWINE RenderScene |
| 0x5fd030 | pz::SWorld::SelectByClass | 192/94 | b | selection helpers |
| 0x68c4e0 | pz::SViewport::Resize | 181/34 | c | the SWINE OnSize resets the device first |
| 0x608600 | pz::SWorld::UpdateWaterMap | 179/31 | d+own | lakes (0x607ad0) and rivers (0x6015d0) do not raise the water map: LAKS / RVR2 are kept raw (water is the renderer agent's) |
| 0x708180 | pz::SParcel::SParcel | 177/88 | b | BuildParcelIndices |
| 0x587860 | pz::SGunner::UnitIncidence | 176/59 | c | codegen |
| 0x544040 | LoadMenuSkins | 176/33 | c | glyph table |
| 0x590430 | pz::TurnInPlaceIn | 168/77 | b | AddManoeuvre / AddPoint |
| 0x6cef00 | pz::SAnimesh::DrawFrame | 167/14 | b | FillFrame |
| 0x607ad0 | pz::SWorld::LoadMapExtra_607ad0 | 166/36 | d+own | lake scene objects (see 0x608600) |
| 0x6dc4f0 | pz::SModel::UpdateFade | 163/71 | c | codegen |
| 0x55a740 | pz::SProjectileDriver::Refresh | 154/76 | b | ProjectileDriverStep |
| 0x5a6ce0 | pz::LoadPUnitAnimation | 152/30 | b | CreatePUnitAnimation |
| 0x634d60 | SAchimMenu::Create | 144/66 | d | minor: the full-screen SButton child of the splash is not created (key / mouse handlers drive it) |
| 0x59cf80 | pz::SPanzersSquadUnit::Slot_14 | 141/7 | d | small: the squad's board elements (+0x380, +0x360.., +0x370..) are not re-created after load |
| 0x587c00 | pz::SPGunner::ReleaseResources | 141/65 | c | codegen |
| 0x5586a0 | pz::SFlyingDriver::MoveTowardNextWayPoint | 140/44 | b | FlyingClimbStep |
| 0x596e30 | pz::SPanzersCampaign::SaveGameStartMission | 136/67 | c | codegen |
| 0x5426c0 | pz::STextBox::OnMouseDown | 132/8 | d | minor: selectable text boxes (none in the single-player boxes) fall to the base handler |
| 0x560eb0 | pz::SGameLogic::AddMinimapObjective | 131/47 | d | the minimap marker of an objective (board +0x08 / +0x24, or board +0xb8 for negative x) is not made; only the record |
| 0x63d790 | SAchimMenu::OnMouseUp | 130/4 | a | deliberate: the marker is HD's OnMouseDown; the recompile advances on button up (left as is) |
| 0x58be80 | pz::CopyStatements | 124/59 | own | cut-scene agent |
| 0x67c170 | pz::AnimProtoSequenceCount | 121/16 | c | Gepard facade |
| 0x55bc50 | pz::SDriver::StartMoveEffects | 121/53 | b | StartNodeEffects |
| 0x63d510 | SAchimMenu::OnKeyDown | 117/5 | b | AchimAdvance |
| 0x631dd0 | SInGameMenu::OnAction | 116/42 | c | table |
| 0x6a8bc0 | pz::SScene::CreateModelFromPrototype | 111/22 | b | CreateModel |
| 0x5b1420 | pz::STrainUnit::TestBlockMap | 109/27 | b | BlockAt / BlockMap_TestBitmap |
| 0x5b17c0 | pz::STrainUnit::MarkBlockMap | 107/29 | b | BlockAt / BlockMap_ApplyBitmap |
| 0x590690 | pz::TurnInPlaceOut | 107/53 | b | AddManoeuvre / AddPoint |
| 0x59a280 | pz::SPanzersSquadUnit::EC_ThrowMolotov | 106/38 | b | SquadUseItem |
| 0x599f80 | pz::SPanzersSquadUnit::EC_ThrowGrenade | 106/38 | b | SquadUseItem |
| 0x56e8e0 | pz::SGameLogic::PreloadArmyUnits | 100/47 | c | multiplayer armies n/a |
| 0x64b380 | SMarket::Update | 98/3 | own | market agent (multiplayer polling) |
| 0x59af90 | pz::SPanzersSquadUnit::Slot_148 | 96/47 | c | codegen |
| 0x55be10 | pz::SDriver::StartWaterEffects | 94/14 | b | StartNodeEffects |
| 0x5c2590 | pz::SUnit::PlayDiedByFireEffects | 89/9 | b | PlayUnitEffects |
| 0x5ba2b0 | pz::SUnit::GetBuildingAction | 88/28 | d | STUB_LOG building part (PL) |
| 0x5c2270 | pz::UnitSlotsDecreaseAmount | 87/35 | c | codegen |
| 0x556560 | pz::SPTrainDriver::Load | 87/37 | c | codegen |
| 0x666190 | SPropertyStruct::GetMultiSubStruct(char const *) | 86/25 | b | Find helper |
| 0x5a4b80 | pz::SPUnit::SPUnit | 85/38 | c | memset |
| 0x590ec0 | pz::SPanzersCampaign::SPanzersCampaign | 85/26 | c | memset |
| 0x5acd30 | pz::SSingleUnit::GetMinRange | 84/40 | c | codegen |
| 0x5f5b50 | pz::SWorld::StartEffects | 83/12 | d | the ambient sound sources never started (AMBS kept raw): LIFTED here |
| 0x666080 | SPropertyStruct::GetMultiSubStruct(int, char const *) | 81/26 | b | At helper |

