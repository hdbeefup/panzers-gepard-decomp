# Stubs hit while booting to the main menu

Branch `p2d-app-menu` (P2-D), on top of `boot-to-menu` aa7bdd6 (P2-A, P2-B
and P2-C merged). Census at this point:
`153 lifted bodies + 1331 SWINE-shared bodies + 26 stubs`.

Since P2-D, `StubLogFirstCall` also writes each first stub call into the game
log (`log\panzers<time> <time>.txt`), so this list was taken from the logs of
these runs from `scratchpad\p2d\run\` (windowed, HD data, no cutscene paks):

1. `p2d_panzers.exe` with no arguments: the Bink intro played, and the main
   menu followed. The stub-to-log line was not built in yet for this run.
2. `p2d_panzers.exe -nointro`: main menu after about 3 s. The mouse was moved
   over the buttons, and Options and Credits were clicked.
3. `p2d_panzers.exe -nointro`, then a scripted click on **Exit**: the process
   exited with code 0 ("Log system destroyed", no `crash.txt`).

| Stub (first-call name) | HD address it stands in for | When | Runs |
|---|---|---|---|
| `SSuperWindow::Initialize Gepard render options (Gepard +0x10)` | Gepard +0x10 calls in `SSuperWindow::Initialize` 0x657910 | Initialize | 2, 3 |
| ~~`SUnitRegistry ctor + LoadUnitFiles (0x5cfe30 / 0x5d1050)`~~ (lifted in M1-D, see below) | 0x5cfe30 and `SUnitRegistry::LoadUnitFiles` 0x5d1050 | Initialize | 2, 3 |
| `SSuperWindow::LoadMenuBackground maps/menu.map world (...)` | world part of 0x658690: 0x5d2f90 SWorld, 0x5f1990 SWorld::LoadMap, 0x55e440 SGameLogic::SGameLogic (not the camera) | Initialize / LoadMainMenu | 2, 3 |
| `SVersion::GetVersionString (0x65c070)` | 0x65c070 (exported) over 0x65bbf0; returns "1.25" | LoadMenuBackground | 2, 3 |
| `DrawDebugPickerOverlayFromGepard` | SWINE editor overlay (no HD counterpart) | every frame, SGepard::RenderScene | 2, 3 |
| `SSuperWindow::OnAction unhandled action (0x659250)` | `SSuperWindow::OnAction` 0x659250, cases the shell does not handle | menu clicks: New Game, Load Game, Multiplayer, Tutorial, Training Camp (Options lifted in P3-Y, Credits in P3-X) | 2 |
| `SSuperWindow::OnAction 0x4f564 Gepard/board graphics options (0x659250)` | renderer part of the 0x4f564 case: Gepard +0x10 SetOption 2/3/8/9/10, scene +0x104, board +0xc8 | Options > Graphics > Apply or Restore | P3-Y |
| ~~`SUnitRegistry dtor (0x5d0c10)`~~ (lifted in M1-D) | 0x5d0c10 | OnDestroy on quit | 3 |

P3-Y (branch `p3y-options`) lifted the Options screens. Its runs from
`scratchpad\p3y\run\` (`p3y_panzers.exe -nointro`) went Options > Game
Options / Graphics / Audio > Back, changed music volume, Tooltips and
Autosave, pressed Graphics > Apply, used Esc, and quit through Exit. The only
new stub hit is the graphics-options one above. `SSettings::Save (0x64fdd0)`
is no longer a stub (lifted in `src/panzers/settings.cpp`), and Options no
longer reaches the unhandled-action stub.


## M1-D: the menu world (`-menu3d`)

Branch `m1d-world` (agent D) on `menu-3d` 8643076. Census
`336 lifted + 1331 SWINE-shared + 281 stubs (23 shell, 258 menu3d skeleton)`
(was `275 + 1331 + 289 (25 shell, 264 menu3d)`).

Runs from `scratchpad\m1d\run\` (`m1d_panzers.exe -nointro -menu3d`,
windowed): boot to the menu, Credits, Esc back to the menu, Exit, click
through the quit screens; exit code 0. A second run without `-menu3d` showed
the default path unchanged (world off; it still hits the
`SSuperWindow::LoadMenuBackground maps/menu.map world` stub).

Removed: `SUnitRegistry ctor + LoadUnitFiles (0x5cfe30 / 0x5d1050)` and
`SUnitRegistry dtor (0x5d0c10)`, now `pz::SUnitRegistry`
(`src/world/unitregistry.cpp`). The registry is built only when the 3D menu
world is on (611 unit types); the default path does not scan the `.unit`
files. The `src/world` skeleton stubs `SWorld::SWorld`, `~SWorld`,
`ShowLoadingIcon`, `HideLoadingIcon`, `LoadMap`, `Initialize` and
`ComputeCamera` are lifted (`world.cpp`, `mapload.cpp`, `unit.cpp`).

Stubs hit on the `-menu3d` path (first-call names), all owned by other
agents or by M2, in call order:

| Stub | Owner | When |
|---|---|---|
| `SPixie::SPixie (0x6941e0)` | C | Initialize (Gepard +0x5c) |
| `SScene::SScene (0x69faf0)` | A | SWorld ctor (Gepard +0x0c) |
| `SScene::SetAmbientLight` / `SetSunLight` / `SetFog` | A | SWorld ctor, WTHR (weather lights 0x6088f0) |
| `SGepard::LoadModelPrototype (0x67db20)` | A | hero flags, unit types (returns -1) |
| `SScene::ClearSkybox (0x6aa9a0)` | A | KSYB |
| `SScene::CreateTerrain (0x6a9dd0)` | A/B | TERR HMAP (returns null; the world keeps its own buffers) |
| `SScene::CreateModelFromFile (0x6a8d50)` | A | 144 doodads (returns null) |
| `SPixie::LoadEffectPrototype (0x69dc80)` | C | doodad demolish effects, EEFS, Initialize |
| `SPzGepard::PurgeModelPrototypes (0x678210)` | A | end of LoadMap |
| `SGameLogic::SGameLogic (0x55e440)`, `SetRunning (0x5802f0)` | D (M2) | LoadMenuBackground |
| `SGameLogic::Refresh (0x576d80)`, `UpdateUnitVisuals (0x5638f0)` | D (M2) | OnIdle (logged stubs by design) |
| `SViewport::SetCamera (0x68d370)`, `SetProjection (0x68cfd0)` | A | ComputeCamera |
| `SScene::PrepareViewport` / `UpdateViewport` / `RenderViewport` | A | frame |
| `SGameLogic::~SGameLogic`, `SScene::~SScene`, `SPixie::~SPixie` | D (M2) / A / C | Exit / OnDestroy |
| `SSuperWindow::Initialize Gepard render options`, `DrawDebugPickerOverlayFromGepard`, `SVersion::GetVersionString` | shell | as before |

Not hit: `SWorld::Slot_04/08/0C/10`, `SWorld::ComputeCamera modes 1/2`.

## M1-I: integration, 3D menu on by default

Branch `m1-integrate` on `menu-3d` 20c87be. Census
`746 lifted + 1331 SWINE-shared + 154 stubs (23 shell, 131 menu3d skeleton)`
(was `728 + 1331 + 154`). The 3D menu world is now the default; `-nomenu3d`
or `PZ_MENU3D=0` turns it off (docs/MENU3D_INTERFACES.md section 1).

Runs from `scratchpad\m1i\run\` (`m1i_panzers.exe -nointro`, windowed):
three runs of boot -> menu -> Options -> Esc -> Credits -> Esc -> Exit
(exit code 0 each, no `crash.txt`), and a 5-minute idle at the menu
(private bytes 104.9-105.4 MB, 594-599 handles, sampled every 30 s).

Stubs hit on the default path (first-call names):

| Stub | Owner | When |
|---|---|---|
| `SSuperWindow::Initialize Gepard render options (Gepard +0x10)` | shell | Initialize. Also why the scene renders as with `Shadows = 0` (Gepard option 2 stays 0) |
| `DrawDebugPickerOverlayFromGepard`, `SVersion::GetVersionString (0x65c070)` | shell | as before |
| ~~`SScene::ClearSkybox (0x6aa9a0)`~~ | A | KSYB (empty in menu.map). M3-E: lifted with `SetSkybox 0x6a96c0` and `DrawSkybox 0x6b7920` (`pzscene_m3.cpp`) |
| ~~`SPSoundEffect (EffectType 5, 0x6ec900)`~~ (M3-E: lifted, `effecttypes.cpp`), `SPRain (EffectType 2, 0x6ea820)`, `SPSnowfall (EffectType 3, 0x6eb5e0)` | C | effect prototypes (sound, Rain.fx, Snowfall.fx in Initialize) |
| `SGameLogic::SGameLogic (0x55e440)`, `SetRunning (0x5802f0)`, `~SGameLogic (0x55fe00)` | M2 | LoadMenuBackground / Exit |
| `SGameLogic::Refresh (0x576d80)` | M2 | every 20 Hz tick; only its model part runs (`SWorld::RefreshModels`) |
| `SGameLogic::UpdateUnitVisuals (0x5638f0)` | M2 | every frame |
| `SScene::DrawSea (0x6b04d0)`, ~~`DrawSkybox (0x6b7920)`~~, ~~`DrawTerrainDecals (0x6ad0e0)`~~, `DrawLines (0x6acf20)` (was `DrawTrails`), `DrawLakes (0x6ad740)`, ~~`DrawWires (0x6b8b10)`~~, ~~`DrawDecals2 (0x6b7b30)`~~ | A | every frame (nothing to draw in menu.map, except the wires). M2-V: 0x6ad0e0 is the ground-trail (track mark) pass, lifted as `SScene::DrawGroundTrails`; 0x6acf20 draws the +0x270 point pairs, renamed `DrawLines`. M3-E: 0x6b7b30 is `SScene::RenderSmokeTrails`, lifted as `DrawSmokeTrails` with the smoke-trail slots +0x7c..+0x88 and 0x6aa9e0; `DrawWires 0x6b8b10` lifted with the wire slots (`CreateWire` +0xc8, `UpdateWire` +0xcc, `DestroyWire` +0xd0, catenary 0x6c05d0); the menu wires appear once the world's LoadWires 0x5f3d60 (agent F) calls `CreateWire`. Sea, lines and lakes are not reached by the Training Camp either |

No longer hit since M1-D: `SPixie::SPixie`, `SScene::SScene`, the scene
light setters, `SGepard::LoadModelPrototype`, `SScene::CreateTerrain`,
`CreateModelFromFile`, `SPixie::LoadEffectPrototype`,
`PurgeModelPrototypes`, `SViewport::SetCamera` / `SetProjection` and the
scene frame functions (all lifted by agents A and C). The world now also
loads ROD2 / RODJ (no longer kept raw) and creates the EEFS effects, the
map decals and the terrain layers (MENU3D_INTERFACES.md section 8).

These stubs exist but were **not** hit on the boot-to-menu path:
`SSettings::CHECKCDKEY (0x64d390)`;
`CreateHostFromCommandLine (0x6576f0)`; `ConnectToHostFromCommandLine
(0x657460)`; the `-market` and map command-line starts; `LoadMultiPreMenu
(0x658a30)`; `LoadChatRoomView (0x658050)`; and the New Game / Load Game
submenus (0x633770 / 0x62cbc0; with `-m3` the New Game submenu is lifted in
M4, `src/panzers/campaignmenu.*`, docs/M4_STATUS.md). The SWINE-era stubs `ZSTD_decompress`,
`DXGetErrorStringA`, `MpegAudioPrecalculate`, `SMenuBackGroundView::*` and
`MatchInfo::*` were not hit either.

Removed in P3-X (branch `p3x-cursor-credits`; census `163 lifted + 1331
SWINE-shared + 25 stubs`): the menu-cursor stub, now `SBoard::LoadCursorSetFile`
(HD board +0x94, 0x6c59e0) called from `SSuperWindow::Initialize`, and the
Credits case of the `SSuperWindow::OnAction` stub, now `SMainCreditMenu`
(`src/panzers/credits.*`) and `SSuperWindow::LoadMainCreditMenu` 0x658300.

Removed in P2-D because the lifted Panzers code now provides them:
`Format` (HD 0x51ee20) and `TimerProc` (HD 0x543cf0), both in
`src/panzers/widgetglue.cpp`.

Removed in P3-Y: `SSettings::Save` (HD 0x64fdd0). Added in P3-Y:
`PzStub_ApplyGraphicsOptions` (above). The stub count stays at 26.

## M2-I: menu game logic on by default

Branch `m2-integrate` (on `menu-3d` 50447c8). Census
`1488 lifted + 1331 SWINE-shared + 301 stubs (22 shell, 121 menu3d skeleton, 158 m2 skeleton)`
(M2 start: `1357 + 1331 + 273 (22, 117, 134)`). The M2 game logic is now the
default; `-nom2` or `PZ_M2=0` turns it off (docs/M2_STATUS.md).

Runs from `scratchpad\m2i\run\` (`panzers.exe -nointro`, windowed): two
335 s idle runs with `PZ_M2_CRC=1` (world CRC identical to the original for
all 6,199 compared frames), three boot -> Options -> Credits -> Exit loops
each with `-m2`, by default and once with `-nom2` (exit code 0, no
`crash.txt`).

Stubs hit on the M2 path, besides the shell / menu3d ones listed above
(first-call names):

| Stub | HD address | When |
|---|---|---|
| ~~`SDriver::StartEffects`, `StartMoveEffects`, `StartWaterEffects`~~ (lifted in M2-V with pixie +0x28 / +0x30 / +0x64; the water one no longer runs: `SWorld::UpdateWaterMap` 0x608600 is lifted) | 0x55bb80 / 0x55bc50 / 0x55be10 | a vehicle starts moving |
| ~~`SSingleUnit::UpdateVisuals (0x5aaaa0) board and armour decals`~~ (lifted in M5-VX with the glows, src/game/unitboard.cpp) | board part of 0x5aaaa0 | every frame (no health bars) |
| `SBuildingUnit 0x546f70 building eye heights` | 0x546f70 (engine model +0x100 / +0xd8) | map load (visibility map eye heights next to the house) |
| ~~`SPanzersSquadUnit::Hook20 (0x59fab0) equipment slots`~~ (lifted in M3-C5) | 0x59fab0 | squad creation |
| ~~`SIModel +0xac (0x6d8090) node points`~~ (M3-I: typed `GetNodePoints` and lifted) | 0x548f20 / 0x5497a0 | building creation |
| `SModel::Slot_CC (0x6dad80)`, `SPixie::Slot_14 (0x69ee50)`, `SPUnitAnimation::Slot_0C (0x5cb470)` | engine / animation slots | unit creation (shadow texture, effect, animation prototype hook). M3-E: 0x6dad80 sets the blob-shadow texture (+0xec, drops the +0xf0 terrain decal); left a stub because the decal that uses it is not lifted and lifting it alone would remove the units' shadow-buffer shadows. `SPixie::Slot_14` 0x69ee50 is the effect editor's save: `SUnit::Uninit` 0x5b7e40 called it by mistake for pixie +0x34 StopEffect (fixed by M3-C) |
| ~~`SPTrailEffect (EffectType 8)`, `SPDecalEffect (4)`, `SPLiteEffect (7)`, `SPCameraShake (10)`~~ (M3-E: lifted, `effecttypes.cpp`) | 0x6edcd0 / 0x6ea0f0 / 0x6ed790 / 0x6ee390 | effect prototypes of the units (M2-V: the die / destroy / gunner effects load now, so the camera shake shows up too) |
| ~~`SPParticles::Init Draw=Object (Gepard +0x20 model prototype)`~~ (lifted in M2-V; M3-E lifted the drawing too: 0x6e5b80, 0x6e5150, and Draw=Effect 0x6e4660, ParticleType 3 0x6e5290) | 0x6e5eb0 case 1 | die / destroy effect prototypes |

No longer hit: `SGameLogic::Refresh (0x576d80)` and `UpdateUnitVisuals
(0x5638f0)` as stubs, `SGameLogic::Tick_565e10` (visibility maps lifted),
`SGameLogic::Dispatch_571380`, the convoy hand-over and movement-group
"STUB:" warnings of 0x579510, `SUnit::AI_Heartbeat (0x5b37d0)`,
`SUnit::ServerRefreshMedic (0x5bfa50)` (entry lifted),
`SPanzersSquadUnit::RefreshSquadFormation (0x59dda0)`, and the SModel slots
+0x38 / +0x48 / +0x4c / +0x50 / +0x54 / +0x58 / +0x64 / +0x68 / +0x78 /
+0x80 / +0x84 / +0x88 / +0xa8 / +0xc0 / +0xdc / +0xf0 (named and lifted).

## M3-E: engine gaps on the Training Camp path

Branch `m3e-engine` (worktree of `menu-3d`, merged with its forks `m3e-scene` and `m3e-widgets`).
Census `1613 lifted + 1317 SWINE-shared + 361 stubs (22 shell, 82 menu3d skeleton, 153 m2 skeleton,
104 m3 skeleton)` (M3 start: `1511 + 1331 + 391`; 13 SWINE-shared widget bodies became `// PANZERS`
bodies in the widget audit, docs/ENGINE_DIFF.md "M3 widget audit").

Replaced stubs: the effect types decal / sound / lite / trail / camera shake, the particle draw types
object / sub-effect / trail and the birth-model follow, SPixie +0x54 / +0x58, the scene smoke trails,
skybox and wires (`pzscene_m3.cpp`), the viewport picking / subport slots +0x00 / +0x34 / +0x38 /
+0x44 / +0x54 / +0x58 / +0x60 / +0x7c / +0x80.

Stubs still hit on `-nointro -m3` (Training Camp round trip, engine side): `SModel::Slot_CC
(0x6dad80)` (see above), `SPixie::Slot_14 (0x69ee50)` (miscalled), `SPzGepard::SwitchModelPrototypeNodes
(0x681010)` (node reorder of a unit prototype; models draw without it), `SScene::DrawSea / DrawLines /
DrawLakes` (called every frame, nothing to draw on training.map), `SPRain` / `SPSnowfall` (prototypes
only). ~~`SScene::ReplaceModel (0x6ba810)`~~ (lifted in M5-VX with SModel 0x6d9c20).


## M3-C: combat and AI on the Training Camp path

Branch `m3c-combat` (integrator C0 with sub-agents C1 gunner / projectile, C2 damage / death,
C3 air support / waster, C5 squads / buildings; the AI part was done by C0). Census
`2025 lifted + 1317 SWINE-shared + 225 stubs`.

Replaced (were hit or reached on the mission path): `SGunner::ServerRefresh (0x584d00)` aim / fire,
`SGunner::ConsumeAmmo (0x583e30)` world event, `SUnit::TakeDamage (0x5c4080)`, `EC_Die (0x5b8b10)`,
`EC_Attack` / `EC_AttackMove` / `EC_AttackAlongPath`, `SUnit::FindTarget (0x5b4720)`, the
`SUnit::AI_Heartbeat (0x5b37d0)` attack branches (0x5b5090 / 0x5b53d0 / 0x5b5810), `SWorld::RefreshAI
(0x5f5c70)` (now `SAIGroup::Refresh`, AIGP loaded), `SUnit::SetAIGroup (0x5c0c10)`,
`SPProjectileUnit / SPWasterUnit / SPFlyingUnit::CreateUnit` and their animations, the support calls of
ProcessPacket, `SPanzersSquadUnit::Hook20 (0x59fab0)`, `SSingleUnit::RefreshMisc (0x5af890)` gun crews,
`SBuildingUnit` StoreUnit / windows / RefreshMisc occupants, `SUnit::OnDriverStucked (0x5bcd20)` speech,
`SWorld::UnitSpeech (0x5fff20)` (the queue and playback 0x607f50 stay a stub: audio only).

Still hit or reachable in combat (logged): ~~the building / doodad hit tests (SIModel +0xd0 / +0xd8
0x6db7c0 / 0x6db550; the doodad grid 0x564c20)~~ (M3-I: lifted; BuildingBetween 0x5630c0,
ProjectileHitTest 0x562c20, DamageArea 0x576490 / CrushDoodad, the eye heights 0x546f70 / 0x57f6a0);
`SWorld 0x5d68e0` (an AI group answers an attack:
support calls, help from other groups; 7.5 KB, not lifted); ~~`SBuildingUnit::OnMemberDied (0x549b70)`~~ (M3-I2: lifted);
pixie +0x58 / model +0xbc effect hooks (E); repair / supply orders (0x5c01e0 / 0x5bf280 / 0x5bd610);
the capture flag (0x5471d0).

## M3-I: the tc1 replay (frames 0..654 equal to the original)

Replaced: `SUnit::ServerRefreshMedic (0x5bfa50)` heal target (lifted whole), `SUnit::ExecuteCommand
(0x5b95a0)` untyped EC_ slots (every typed slot dispatched; ~~+0xb0 / +0xbc / +0xdc / +0xd8 / +0x128 /
+0x168 / 0x5b88f0 still log~~ M3-I2: typed and lifted), `SModel::Slot_AC / Slot_B0 / Slot_D0 / Slot_D8 / Slot_100` (0x6d8090 /
0x6d6700 / 0x6db7c0 / 0x6db550 / 0x6d7150), `SWorld::FixBridges` height patches (0x5e65f0),
`SBuildingUnit 0x546f70` eye heights, `SGameLogic 0x576a70` doodad heights (0x57f6a0), the
minimap STUB_LOGs of 0x570f30 / 0x5638f0 / 0x565f1d (HUD path, M3-P). New lifts without a former
stub: `SBuildingUnit::GetSightRange 0x548b40 / GetMinRange 0x5482e0`, `SPanzersSquadUnit::RefreshDead
0x59dfe0`, the SGameLogic ctor's 0x5497a0 loop.

## M3-I2: the whole tc1 replay (frames 0..3900 equal to the original)

Replaced: `SBuildingUnit::MarkBlockMap (0x549b20)` (was an empty body: the block-map rebuild lost the
building footprints; the tc1 frame-655 divergence), `SUnit::Slot_A8 (0x5ba3e0)` (now `ActionOn`, with
the overrides 0x5ace90 / 0x548c10 / 0x59c0d0), `SUnit::Slot_B0 / Slot_BC / Slot_D8 / Slot_DC /
Slot_128 / Slot_168` (0x5b8d20 / 0x5b9370 / 0x5b8fa0 / 0x5b8c90 / 0x5b88d0 / 0x5b9360, now
`EC_MoveReverse` / `EC_TurnTo` / `EC_Tow` / `EC_AttackMoveUnit` / `EC_Destroy` / `Slot_168(bool)`; squad
overrides 0x59aea0 / 0x59a730) and the ExecuteCommand "untyped EC_ slot" log, `SBuildingUnit::
OnMemberDied (0x549b70)`, the MINI chunk kept raw (now World +0x74bc, 0x66ea40). New lifts without a
former stub: the SGameLogic ctor's air start positions (0x55f2c2), `SWalkerAnimation::InitModel`
gun 1 muzzle (0x5ca594). Census `2238 lifted + 1317 SWINE-shared + 189 stubs; 79 lifted bodies still
contain a STUB_LOG`.

Hit in tc1 and still logged (visual / audio only, no CRC effect): `SWorld::UnitSpeech` queue and
playback, `SUnit::TakeDamage` combat music, ~~`SSingleUnit::UpdateVisuals` glows and decals~~ (M5-VX),
~~`SScene::ReplaceModel`~~ (M5-VX) / `DrawLines / DrawLakes`, rain / snow effects, `SModel::Slot_BC / Slot_CC`.
Still not lifted and not reached by tc1: `SWorld 0x5d68e0`.

M4 S (save / load, docs/M4_STATUS.md) replaced `SGameLogic::SaveGameState (0x57e110)`, `SUnit::Save
(0x5be320)`, `SUnit::Load (0x5bbd30)`, `SPanzersCampaign::LoadGame (0x594f70)`, `SUnit::Slot_14 (0x5bb1c0)`
and `SUnit::Slot_18 (0x5baf30)` (now InitAfterLoad / LinkAfterLoad), and the F6 / F9 `StubOnce` lines of
`SGameView::OnKeyDown` (quick save / quick load), and `SMainMenu Load Game submenu (0x62cbc0 /
0x62f950)` (SLoadMenu lifted). `SGameView::OpenSaveMenu (0x6205f0)` stays logged (SSaveMenu is not
lifted; Save Game saves as F6). Census `2315 lifted + 1317 SWINE-shared + 183 stubs; 77 lifted bodies
still contain a STUB_LOG`.
