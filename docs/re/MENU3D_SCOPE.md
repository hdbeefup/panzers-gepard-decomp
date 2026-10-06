# P4 scope: 3D main-menu background (`maps/menu.map`)

Read-only scoping. Repo commit `56b0b57`. Ground truth: HD `PANZERS.exe` (Ghidra copy in `p4scope/ghidra`).
All addresses are HD addresses. "SWINE" = the imported `src/3dengine` (swine-portable@95bbcc4).

Files in this folder:

| File | What |
|---|---|
| `callgraph_menuworld_depth3.tsv` | 390 functions reached by direct calls (depth 3) from LoadMenuBackground 0x658690, UnloadMenuBackground 0x65b940, SSuperWindow::OnIdle 0x65ae50. Columns: addr, depth, insns, decompiled lines, indirect calls, name, CSV symbol, vtable slot, callers, callees, strings. |
| `callgraph_render_load_depth4.tsv` | 355 functions from SViewport::Render 0x68c220, SScene::RenderScene 0x6b7760, SPModel::Load4DFile 0x6912b0, SPAnim::LoadAnimFile 0x692d10, STerrain ctor 0x6efde0, SScene::CreateModel/CreateLake, SModel::Update/Render. |
| `w3/c/*.c`, `wr/c/*.c` | Ghidra C for every function in the two graphs. |
| `survey.txt` | 4CC constant users, every RTTI vftable with its slots, function count per 64 KB. |
| `menu_doodads.txt` | 144 doodads (name, x, y, z, r, …). |
| `menu_models.txt` | 51 `.4d` reached statically from the map (doodads + unit files), with version. |
| `orig_loaded_models.txt` | 82 `.4d` the original actually loaded for the menu (from its log), with version. |
| `menu_unitfiles.txt`, `menu_fx.txt` | 20 `.unit` files and 48 `.fx` files reached from the map's units/triggers/effects. |
| `orig_menu_run.log` | Log of the original HD exe booting to the menu (my run). |
| `shots/menu_126.png .. menu_182.png` | PrintWindow captures of the original menu, 7 s apart. |
| `data/menu.map` + samples | Extracted from `panzers.pak` (never committed). |
| `pak.py`, `chunks.py`, `menuassets.py`, `scripts/walk.py`, `scripts/survey.py` | Tools used. |

Headline: **the menu background is a live game mission.** `LoadMenuBackground` builds a full `SWorld` and an
`SGameLogic`, and `OnIdle` runs `SGameLogic::Refresh` at a fixed 20 Hz. The convoy and the marching squad are
real `SUnit`s spawned and moved by the map's **triggers**, not animated doodads. The camera is static.

---

## 1. Call graph

### 1.1 Load: `SSuperWindow::LoadMenuBackground(bool keepScene)` 0x658690 (273 insns)

```
0x658690 LoadMenuBackground                       [lifted in superwindow.cpp except the world part]
 ├ concert +0x84 IsStreamPlaying / +0x6c..+0x78 playlist "music/menu.mp3"   (already lifted)
 ├ if (SSuperWindow+0x19c world == 0):
 │  ├ 0x51de30 SString("maps/menu.map") ; 0x65f420 SFileSystem::Open  (lifted/SWINE)
 │  ├ new 0x7538 -> 0x5d2f90 SWorld::SWorld(0)            726 insns / 903 lines
 │  │    ├ Gepard +0x0c CreateScene  -> g_Scene 0x929a54 ("Scene created")
 │  │    ├ scene +0x08 (SetAtmosphere/name)  ; concert +0x24 load ambient mp3
 │  │    ├ Gepard +0x44 LoadTexture x5 (selection frame, arrow, range)   ; board +0x7c fonts/icons
 │  │    ├ 0x65fe80 SProperties::Load("objects.ini")
 │  │    ├ Gepard +0x20 LoadModelPrototype("units/flag/*-hero.4D", 0.005) x3
 │  │    ├ 0x5fdaf0 SWorld::SetDefaultWeather, 0x5fdc80 (weather -> scene lights)
 │  │    └ 0x5edd00 (speech/selection tables), 0x5d8fc0, 0x5dd010 (arrays)
 │  ├ 0x5edca0 SWorld loading icon (board frame "menu/panzers_loading_icons_hq.tga")   31 insns
 │  ├ 0x5f1990 SWorld::LoadMap(stream, 1, 0, 0)            403 insns  (MAPF v201 chunk loop)
 │  │    ├ Gepard +0x3c GetViewport(0) -> +0x50 Render(0,0) (draws loading frame)
 │  │    ├ MINI 0x669ca0/0x66ea40 minimap bitmap; MINA/KSYB/ATMS 0x56e7d0 strings (KSYB -> 0x5fec10 skybox)
 │  │    ├ TERR 0x5f2fc0 (300 insns): HMAP -> scene +0x64 CreateTerrain(w,h,4) -> terrain vtbl+0 Acquire(8 ptrs);
 │  │    │                            TLAY 0x5efda0; BLND (16-byte stride, 16 layers); DIFF; BLCK (u16, 4x res); TMAP
 │  │    ├ WTHR 0x5f0080 + 0x5fdc80 ; LITE (≤20 floats) ; ROD2 0x5f0a30 ; RODJ 0x5f0b60 (+0x5f7fa0)
 │  │    ├ PLY3 0x5f2ed0 ; PLY2 0x5f2f50 ; RVR2 0x5f0960 ; TRIG 0x5f0140 ; TVAR 0x56e440 ; LOCS 0x5f0690
 │  │    ├ PATH 0x5f07b0 ; CAM  0x5fd7c0 ; WIR3 0x5f3d60 SWorld::LoadWires (440 insns)
 │  │    ├ ENTS 0x5f2a50 (304 insns):
 │  │    │    DODS 0x5f0500 -> per doodad 0x5f73f0 (-> SDoodad::Initialize 0x5ee040, scene CreateModel)
 │  │    │    DECS 0x5efcd0 -> 0x5f7170 ; AMBS 0x5f02a0 ; AIGP 0x56e2c0 ; EEFS 0x5f08a0 ; LAKS 0x5f05c0
 │  │    │    UNDS 0x5f33f0 (unit defs, "Unsupported unit type") ; UNIS 0x5f3820
 │  │    └ end: 0x5ef380(0,0,w*4,h*4,0x7f3f) terrain rebuild, 0x608600 SWorld::UpdateWaterMap,
 │  │           0x604620 (1152 insns, world/unit refresh), Gepard +0x28
 │  ├ 0x5eec90 SWorld::Initialize                         127 insns  ("%d doodads", Rain/Snowfall .fx)
 │  │    └ 0x5ecdf0 SWorld::InitFlyingFox (983), 0x6043a0, 0x5a1460/0x5a1590
 │  ├ stream vtbl+0 (delete)
 │  ├ new 0x318 -> 0x55e440 SGameLogic::SGameLogic(0,-1,0)  1159 insns / 621 lines
 │  │    ├ 0x5da050 SWorld::CheckStaticBlockMapInternal (3153 insns)
 │  │    ├ 0x56da00 trigger counters/timers ("TIMECOUNTER", "UNITCOUNTER")
 │  │    ├ 0x5735a0 air-unit class table, 0x5640b0 CollectActiveLocations, 0x582080 UpdateActiveLocations
 │  │    ├ 0x560290/0x564c20/0x564fb0/0x565e10 (players/visibility setup), 0x571840 frame/CRC
 │  │    └ loads plane/projectile models (log: B25, C-47, Stuka, He-111 … after "Scene created")
 │  ├ 0x5802f0(1)  SGameLogic+4 = 1 -> concert +0x64 (resume sounds)
 │  └ 0x5dc7d0 SWorld: destroy loading icon frame
 └ if (!keepScene): SDXWindow+0xe0 = g_Scene (AddRef via vtbl+0), +0xd8 = 1 (continuous render),
    MenuTime = MenuNextTick = timer; top/bottom bars + version text   (lifted)
```

Globals: `g_World` 0x929a50 (SWorld*), `g_Scene` 0x929a54 (SScene*), `Gepard` 0x8f1c58, `Concert` 0x8f1c5c,
`Board` 0x8f1c60, 0x929f14 = Gepard +0x5c object (probably the SPixie effect manager; not verified).

### 1.2 Per frame: `SSuperWindow::OnIdle` 0x65ae50 (363 insns), world branch (SSuperWindow+0x19c != 0)

```
now = timer (0x661800)
while (MenuNextTick +0x1a8 < now) { 0x576d80 SGameLogic::Refresh(); MenuNextTick += 0.05 }   // 20 Hz fixed step (0x7f4534)
scene  +0x1c SetTime(ftol(..))         ; board +0xa0 per-frame time
vp = Gepard +0x3c GetViewport(0)
0x5ddc30 SWorld::ComputeCamera(vp)     533 insns: vp +0x20 SetCamera(x,y,z,rotY,pitch), vp +0x28 SetProjection(60°,near,far),
                                       concert +0x08 listener, scene +0x24 (focus height)
scene  +0x20 SetInterpolation((next-now)*20)          // render-time blend between logic ticks
0x5638f0 (493 insns) per-unit visual update/visibility (calls 0x562760 SGameLogic::CanSeeGroundUnit)
concert +0x0c Update3D(0)
... SWindow::EventFrame 0x544a50, news ticker, then SDXWindow::OnIdle 0x53a0d0
    -> viewport +0x50 SViewport::Render(scene) 0x68c220
```

`SGameLogic::Refresh` 0x576d80 (1700 insns): 0x607f50 SWorld::UpdateSpeech, 0x578b00/0x578a70 (timers/subtitles),
SMulti frame sync (0x571840, 0x5212a0: no-op single-player path), 0x604620 world refresh, 0x57dfe0 autosave,
0x6088f0, 0x570cc0, **0x579ab0 SGameLogic::RunTriggers (3757 insns, 75 cases)**, 0x568af0, 0x565e10,
per unit (World+0x4d4) vtbl +0x16c / +0x2c(frame) / +0x3c, scene +0xf0, 0x5f6bf0 RefreshFlyingFox,
0x5f5c70 (AI, 1010 insns), 0x5822a0, per doodad (World+0x140, stride 200) model vtbl +0x3c, 0x605930 SWorld::UpdateDWire (1955).

### 1.3 Render: `SViewport::Render` 0x68c220 (193 insns)

```
0x68c220 SViewport::Render(scene)
 ├ device TestCooperativeLevel / 0x67fde0 SGepard::ResetDevice ; board +0xc4
 ├ Clear ; BeginScene
 ├ 0x6bbc40, 0x6a24c0, 0x6acaf0 (231)  -> scene render for this subport
 │   └ 0x6b7760 SScene::RenderScene (110):
 │        0x680620 SGepard::SetLights, 0x680510, 0x688ac0 (camera matrices), 0x6f8610 terrain,
 │        0x6bbcb0 (231) visibility, 0x6aac20 SScene::GenerateShadowBuffer (879, PS2.0 shadow path),
 │        0x689f10 SViewport::Clear, 0x6f2aa0 (590) terrain draw, 0x6f46e0 (290) roads,
 │        0x6b02a0, 0x6ad740 (2178) model batches, 0x6b0920 (6170!) main object/particle draw
 ├ 0x6ca240 board software cursor, 0x6c7150 (2490) board render
 └ EndScene ; 0x68bfe0 SViewport::Present
```

Model side: `SModel::Update` 0x6dba80 (640 insns: sequences, bones, attached lights),
`SModel::Render` 0x6d8830 (798: "Too many bones"), `SMesh::Draw` 0x6cd890, `SAnimesh` (14 slots, 0x6ceda0..),
`SSkinnedMesh` (0x6d0d30, 0x6d2200), `SMesh/SGepard::CreateVertexShader` 0x679da0/0x679eb0 (inline vs.1.1).
Loading: `SPModel::Load4DFile` 0x6912b0 (1895 insns, handles v100 **and v101**, FACE/MATE/REFL **and**
INDI/STRP/SPEC/DUMY/FLYZ/SKIN/SSQS/LITE), `SPModel::LoadSequences` 0x693550 (733), `SPAnim::LoadAnimFile`
0x692d10 (328, `CANM` v100 + NODE), animation tracks 0x6712a0..0x671980 (`STrackPositionIndependent`,
`STrackRotationEuler`), `SCollisionConvexPoly::Load` 0x6d3950.

### 1.4 Unload: `SSuperWindow::UnloadMenuBackground` 0x65b940 (47 insns)

board DestroyFrame top/bottom; SDXWindow+0xe0 scene Release (vtbl+4); `0x55fe00` SGameLogic dtor body (196 insns,
clears queues/units 0x55fa20..0x55fd30, 0x563230, 0x579420, 0x580490) + `operator delete(0x318)`;
SWorld vtbl+0 (scalar deleting dtor 0x5d68b0) on +0x19c; +0xd8 = 0; SetCursor(-1).
The SWorld dtor tears down the scene ("SScene::~SScene" 0x6a0440, "Scene destroyed").

### 1.5 Size of the reachable code

* Direct-call closure (depth 3) from the three SSuperWindow roots: 390 functions, 58k instructions,
  259 of them in the world range 0x546000..0x60a000. Virtual calls are **not** followed, so units, drivers,
  animations, scene, model and terrain virtuals come on top.
* Render/load closure (depth 4): 355 functions, 71k instructions.
* Unique virtuals: SUnit family (5 classes the menu uses) 293; drivers 62; unit animations 64;
  scene/model/mesh/terrain/parcel/viewport/gepard 271; effects (SPixie, SEffect*, SParticles, SAtmosphere …) 116.
* Code density: world/game 0x546000..0x60a000 ≈ 2,200 functions; HD 3D engine 0x676000..0x70c000 ≈ 1,430
  (incl. board, sound, bitmap).

---

## 2. What `maps/menu.map` contains

`panzers.pak`, 1,700,599 bytes, `MAPF v201`. Top-level chunks in file order:

| Chunk | Content |
|---|---|
| MINA, ATMS, KSYB | all empty strings: **no atmosphere file, no skybox** |
| MINI | minimap bitmap 108x132 |
| TERR v100 | HMAP **168 x 184** tiles (169x185 floats); TLAY **13 layers** (Hungarian names: "Kau6 Fuves szalmas 1B", "52 kockakoves3", …, "99 Regi" = grass layer, attr 8 -> "99 Regi" extra); TMAP 21x23x2; BLND 375,180 B; DIFF 169x185 floats; BLCK 672x736 u16 (4x res). Parcels 8x8 tiles -> **21 x 23 = 483 parcels** (STerrain ctor 0x6efde0 divides by 8). |
| WTHR | 1 weather "Default" with light params (no LITE chunk) |
| ROD2 / RODJ | **roads** (the dirt road the convoy drives on) and 7 junctions |
| PLY3 | 12 player slots |
| ENTS v100 | DODS 144 doodads; AMBS 0 live; LAKS **0**; DECS 24; EEFS 3; AIGP 0; UNDS 11 units |
| LOCS | 15 locations ("katona 1..4", "gyalogosok ide", "officer", "Location n") |
| PATH | 1 live path ("Path 5") |
| RVR2 | none |
| TRIG | 17 triggers (scripts) |
| TVAR | 2 trigger variables ("Time", "time 2") |
| CAM | 20 bytes: 97.65, 95.74, 6.22, -0.638, 19.08 (target x/z, height, angle, distance) |
| WIR3 | wires (632 B; SWorld::LoadWires requires building units) |

**Doodads** (144, all DOOD v100; `menu_doodads.txt`): 28 distinct `.4D` - bushes (nagybokor3/4,
varosbabokor01b/02), trees (fa01..fa09, fenyo), ferns, lamp posts, statues, signpost, flower boxes, a cross,
and the burnt-out Panther `objects/22 Units/pzkpfw_V_D_kilott.4D`.

**Decals** (24): stones, sticks, a skull, cracks, flowers, a bomb crater (`shaders/0x …/*_a.tga`).

**Effects** (EEFS, 3): `effects/smoke/Ground_Dark_Slow_Size3.fx`, `effects/fire/Fire_From_House_Size2.fx`,
`effects/smoke/Ground_Dark_Fast_Size3.fx` (smoke/fire on the Panther wreck).

**Units** (UNDS, 11 `UNTD` property lists, all Player 0): `FR village long stone` (the house, a **building unit**),
4x `US Rifle Squad(1)`, `US Sherman` (with a `US Crew Squad` stored inside), `US Flamethrower Squad`,
`US Rifle Squad(2)`, `US MG Squad`, `US Medic Squad`. Each has ClassName, Pos, Dir, HP, ScriptID, Behavior,
GlobalState_ActiveState, StoredUnits …

**Triggers** (TRIG, 17): two scripted loops.
* "konvoj 1": action 0x29 (ACTION_CREATE) spawns `US Sherman`, `US M2A1`, 2x `GB Bedford QL Ammo` at a location
  facing 180°; "1 konvoj mozgas del fele" (move south); "2 convoj krealasa" spawns 2x `US M36-Slugger`,
  `US Sherman`, `US M7-Priest`; a counter ("szamlalo"), "egyseg eltunik" (unit disappears at the end), then repeat.
* "gyalogos": jeep and crew created (`US Willys Jeep`), move north, disappear; infantry (`US Wilson hero`,
  `US Rifle Squad(2)`, `US Medic Squad`, `US MG Squad`, `US Flamethrower Squad`) start walking, disappear,
  counter restarts.

So the **moving soldiers and vehicles are real units** (SUnit + driver + pathing), created and ordered by
`SGameLogic::RunTriggers`. Doodads are static models (trees may sway through SModel sequences;
`SGameLogic::Refresh` calls model vtbl+0x3c for each doodad).

**Camera:** static. CAM chunk -> `0x5fd7c0` -> SWorld camera, mode 0 of `SWorld::ComputeCamera` (smoothing
toward the target; no follow unit). The captures from 126 s to 182 s show the same framing while the convoy and
the squad move (`shots/`).

**Models:** statically from map + unit files: 51 `.4d` (41 v100, **10 v101**). The original's log for the menu
lists **82** `.4d` loads (57 v100, **25 v101**; `orig_loaded_models.txt`): the 28 doodads, the house and its ruin,
`squad.4D`, all soldiers (`us_rifle/crew/FT/MG/medic/SMG_soldier.4d`, all **v101 skinned**: SCEN v101 =
FLYZ + DUMY bones + SKIN + 116 SREF in SSQS pointing at `units/walker/smg/*.anim` = `CANM v100`), Sherman,
M2A1, Bedford, M36, M7, Willys and their wrecks, projectiles, debris (`effects/media/*.4d`), planes preloaded by
SGameLogic, the hero flags and `flora/99 Regi/grass01.4d`. Pak-wide census: 1,053 v100 + 497 v101.

---

## 3. Subsystem classification

(a) reuse SWINE as is, (b) SWINE has it but HD differs, (c) Panzers-only / world layer, lift from HD.

**Key finding for the whole table:** SWINE-HD's 3D path is built around HLSL files (`shaders/standard.hlsl`,
`terrain.hlsl`, `decal.hlsl` … in `gepard.cpp:10940-10999`) that are **not shipped**, so they compile to null.
Panzers HD draws with fixed function plus inline vs.1.1 shaders and a PS-version shadow choice
(`SGepard::Initialize` 0x67d1c0, `SMesh::CreateVertexShader` 0x679da0). SWINE's object model is also older and
different: one big `SIGepard` (170 virtuals, `SIObject`/`SGroup`), while HD has `SIScene` (66) + `SIModel` (67) +
`SITerrain` (39) + `SIViewport` (33) + `SIPixie` (26) and the world talks only through those vtables.

| Subsystem | Class | Evidence |
|---|---|---|
| Texture load (TGA/DXT, `_hq`) | (a) | already used by the menu; HD only adds a DXT cache (ENGINE_DIFF §2) |
| Streams, chunk reader, SProperties, pak | (a) | SStack Peek/Pop 0x65d3b0/0x65d460 match the core stream API |
| Viewport / camera / projection | (c) | `SViewport` (0x244 B, 33 slots) not in SWINE; camera set via vp +0x20/+0x28 |
| Scene container, lights | (c) | `SScene` 66 slots (CreateModel +0x54, CreateTerrain +0x64, CreateLake +0x9c, SetLight* 0x6baca0..); SWINE keeps these in SGepard |
| Terrain / heightmap / splatting | (b) | SWINE `STerrain`/`SParcel`/`SParcel2` have the same method names (DrawLayered, DrawSketch, Acquire) but **9 blend layers** (`unsigned char[9]`) vs HD **16** (BLND stride 0x10, 0x5f2fc0), HD `Acquire` returns 8 buffers vs 5, HD blockmap is u16 at 4x res, HD terrain is created by scene +0x64, and SWINE draws it with the missing `terrain.hlsl` (`gepard.cpp:11239`). Use SWINE as reference, lift HD 0x6efde0..0x70b050. |
| Roads | (c) | `STerrain::RenderRoads` 0x6f7810 / `UpdateRoad` 0x6f95b0, ROD2/RODJ; nothing in SWINE |
| Water / lakes | (b), **not needed** | SWINE has SGLake/SLakeBlock; HD `SScene::CreateLake` 0x6a7940 differs (1178 insns). menu.map has 0 lakes, 0 rivers. `SWorld::UpdateWaterMap` 0x608600 still runs. |
| Decals | (c) | HD `SDecal`, `SDecalEffect`/`SPDecalEffect`, DECS loader 0x5efcd0/0x5f7170; SWINE decals use `decal.hlsl` |
| Doodads / static `.4d` | (c) | HD `SPModel::Load4DFile` 0x6912b0 + `SModel`; SWINE `SGroup::Load4DFile` accepts only `SCEN v100` with MESH/VERT/… (group.cpp:2172), no v101, no INDI/STRP/SPEC/DUMY/SKIN/SSQS. Use `c4d_model.cpp` as format reference. |
| Animated meshes, skinning, skeletal anims | (c) | `SAnimesh`, `SSkinnedMesh`, `SPAnim::LoadAnimFile` (`CANM`), tracks 0x671xxx. SWINE `SAnimation` is a texture flip-book (`ANIF`/`ANI2`), unrelated. |
| Units (house, tanks, soldiers) | (c) | `SUnit` 115-slot vtables, `SSingleUnit`, `SPanzersSquadUnit`, `SPanzersSquadMemberUnit`, `SBuildingUnit`; `SUnitRegistry::LoadUnitFiles` 0x5d1050 (611 `.unit` files, today a stub); drivers (13 classes), `SWalkerAnimation`/`SVehicleAnimation` (`InitModel` 0x5c93a0, `UpdateModel` 0x5ce2a0), `SAStar` 0x5a2020, block map 0x5da050. SWINE `world/`,`game/` are banned. |
| Triggers / scripts | (c) | `SGameLogic::RunTriggers` 0x579ab0, `STriggerAction/Condition/Event` 0x5b1d50.., TRIG/TVAR/LOCS/PATH loaders |
| Effects / particles in the scene | (c) | Panzers `.fx` = `[ExtendedFx2]` property files -> `SPixie::LoadEffectPrototype` 0x69dc80 over `SPropertyStruct` 0x665370..; `SEffectSet`, `SParticles`, `SLiteEffect`, `STrailEffect`, `SSoundEffect`, `SCameraShake`. SWINE `SEffect::LoadEffect` is a hard-coded effect-type table; particle code may help as reference only. |
| Sky / atmosphere | (c), **mostly not needed** | KSYB/ATMS empty; weather "Default"; `SAtmosphere`/`SPAtmosphere` only for rain/snow (`SWorld::Initialize`) |
| Lighting | (c) | weather -> 0x5fdc80 -> scene SetLight*; `SGepard::SetLights` 0x680620; `SModel::Update` attached lights |
| Shadows | (c), **deferrable** | `SScene::GenerateShadowBuffer` 0x6aac20 (PS2.0 path), `SModel::RenderShadow` 0x6d9570, `SMesh::DrawShadow` 0x6cdb70. `Shadows = 0` is a valid original setting, so rendering without shadows is faithful to that setting. |
| Camera logic | (c) | `SWorld::ComputeCamera` 0x5ddc30 (mode 0, static target) |
| Per-frame world update | (c) | `SGameLogic::Refresh` 0x576d80 at 20 Hz + interpolation (scene +0x20) |
| Board / 2D overlay | (a) | lifted/SWINE, already working; must be drawn after the 3D scene in the same frame |

---

## 4. Minimum viable target

Goal: "menu scene rendered with moving or animated things", faithful to HD, without the full game.

**M1: static scene + idle animation (no game logic)**. ~550-650 functions.

* Renderer: SViewport render subset, SScene (create/render/lights, no lakes), SModel/SPModel loader v100+v101,
  SMesh/SAnimesh/SSkinnedMesh, SPAnim (`CANM`) + tracks, STerrain/SParcel/SParcel2 with 16 layers, roads,
  decals, grass layer. Shadows off (as with `Shadows = 0`).
* Effects: SPixie + `.fx` property loader + SParticles/SEffectSet for the 3 EEFS smoke/fire effects.
* World: SWorld ctor/dtor, `LoadMap` 0x5f1990 and all chunk readers (unknown/unused chunks still parsed and kept),
  doodads, decals, EEFS, `SWorld::Initialize`, `ComputeCamera`, `SUnitRegistry::LoadUnitFiles`, unit creation
  from UNDS with model init (`SVehicleAnimation::InitModel`, walker idle via `SWalkerAnimation`), but **no**
  `SGameLogic::Refresh`.
* Result: house, trees, wreck with smoke/fire, parked Sherman, the placed soldiers playing idle sequences,
  static camera at the CAM position.

**M2: moving convoy and squad.** +350-450 functions.

* `SGameLogic` ctor/dtor/`Refresh` (single-player path), `RunTriggers` with the actions the menu uses
  (create 0x29, move to location, remove, counters/timers, conditions on time/variables/unit counts),
  `SUnit`/`SSingleUnit`/`SPanzersSquadUnit`/`SPanzersSquadMemberUnit`/`SBuildingUnit` refresh paths, vehicle,
  squad and walker drivers, `SAStar` + block map, formation, move animations, track decals.

**Can stay stubbed (logged):** sound emitters and unit/effect sounds (`SSoundEffect`, `SWorld::UpdateSpeech`,
AMBS is empty anyway), AI combat/targeting (no enemies on the map; `0x5f5c70`), selection frames/arrows and
unit buttons, minimap, fog-of-war beyond "Player 0 sees everything" (needs checking: `CanSeeGroundUnit` 0x562760
is on the per-frame path), SMulti frame sync, autosave, objectives/subtitles, flying fox, DWire, lakes/rivers,
weather atmospheres, shadow buffer, the plane/air-unit preloading in the SGameLogic ctor (load only, no effect).

**Estimate:** M1 ≈ 600 functions, M1+M2 ≈ 1,000-1,100, from HD. Several are very large
(0x6b0920 6,170 insns; 0x579ab0 3,757; 0x5da050 3,153; 0x6c7150 2,490; 0x6ad740 2,178; 0x605930 1,955;
0x6912b0 1,895; 0x576d80 1,700).

---

## 5. Proposed split

Phase 0 (one agent, short, blocks the rest): **interfaces.** New headers with HD vtable order and HD sizes
(`src/3dengine/pz/iscene.h`, `imodel.h`, `iterrain.h`, `iviewport.h`, `ipixie.h`, plus `src/world/worldapi.h` for
the globals `g_World`/`g_Scene`). Decide how the HD scene renders inside the SWINE `SGepard` frame: a thin
`SPzGepard` facade that provides the HD Gepard slots the world uses (+0x0c CreateScene, +0x20
LoadModelPrototype, +0x28, +0x3c GetViewport, +0x44 LoadTexture, +0x5c pixie) on top of the SWINE device and
texture code, and a viewport `Render(scene)` that runs before `SBoard` drawing.

| Agent | Owns (disjoint) | Delivers | Depends on |
|---|---|---|---|
| **A: Scene and models** | `src/3dengine/pz/scene*`, `viewport*`, `model*`, `pmodel*`, `mesh*`, `panim*`, `tracks*`, `pzgepard*` | SViewport render subset, SScene, SModel/SPModel (`.4d` v100+v101), SMesh/SAnimesh/SSkinnedMesh, SPAnim/CANM, lights | Phase 0 |
| **B: Terrain** | `src/3dengine/pz/terrain*`, `parcel*`, `road*`, `decal*` | STerrain (16 layers), SParcel/SParcel2/SBlockMapParcel, roads, terrain decals, grass layer | Phase 0; uses A's scene hooks via SIScene only |
| **C: Effects** | `src/3dengine/pz/pixie*`, `effect*`, `particles*`, `src/core/propertystruct*` | SPixie, `.fx` loader (`SPropertyStruct`/`SPropertyTrack`), SEffectSet, SParticles, SLite/STrail/SDecal effects, sound effect as logged stub | Phase 0 |
| **D: World** | new `src/world/` (Panzers-only; never SWINE `world/`), `src/panzers/superwindow.cpp`, `src/stubs/stub_panzers.cpp`, `docs/STUBS_HIT.md` | M1: SWorld ctor/dtor/LoadMap/chunks/Initialize/ComputeCamera, doodads, unit registry, unit creation and model init, OnIdle/Unload wiring. M2 (later, can become two agents: logic+triggers / units+drivers+pathing): SGameLogic, triggers, units, drivers, SAStar. | Phase 0 headers; links against A/B/C only through interfaces; can test with a null scene |

Order: Phase 0 -> A, B, C, D(M1) in parallel -> integrate (D wires `LoadMenuBackground`) -> D(M2).

**Risky parts**

1. Two renderers in one device: SWINE `SGepard` (+ its board) and the HD scene/viewport. Render state leaks,
   `ResetDevice`, and frame order (3D first, board after). The HD `+0xd8` continuous-render flag must be honoured.
2. SWINE's 3D drawing depends on HLSL files that are not shipped; nothing of SWINE's terrain/mesh draw can be
   used directly. HD skinning uses inline vs.1.1 shaders and the HD FVF set (6 declarations).
3. Huge functions (above) and wide structs: SWorld 0x7538, SGameLogic 0x318, 115-slot unit vtables.
4. Timing: 20 Hz logic + interpolation, the FPU control word 0x007F set after Miles (24-bit precision), `rand`.
   Trigger loops and unit movement may drift if these differ.
5. Temptation to copy SWINE `world/`/`game/` (forbidden). Only SWINE's `3dengine` may be used as reference.
6. Units are needed even for M1 (the house and the parked Sherman are units), so the unit registry and unit
   creation path cannot be skipped.

---

## Not determined

* Exact semantics of the trigger action/condition IDs (only ACTION_CREATE 0x29 is confirmed by data + strings).
* What 0x5638f0 (per-frame) and 0x604620 (end of LoadMap and Refresh) do in detail; what object Gepard +0x5c
  returns (SPixie is a guess).
* Whether doodad trees actually animate (model vtbl+0x3c is called; the captures were 7 s apart).
* Whether fog-of-war hides anything in the menu.
* Exact function counts: virtual dispatch was not followed by the graph walk; counts are estimates from vtables
  and address ranges.
* How much HD terrain/parcel code is byte-for-byte the same as SWINE's (only names and data layouts compared).
