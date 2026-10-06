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
| `SScene::ClearSkybox (0x6aa9a0)` | A | KSYB (empty in menu.map) |
| `SPSoundEffect (EffectType 5, 0x6ec900)`, `SPRain (EffectType 2, 0x6ea820)`, `SPSnowfall (EffectType 3, 0x6eb5e0)` | C | effect prototypes (sound, Rain.fx, Snowfall.fx in Initialize) |
| `SGameLogic::SGameLogic (0x55e440)`, `SetRunning (0x5802f0)`, `~SGameLogic (0x55fe00)` | M2 | LoadMenuBackground / Exit |
| `SGameLogic::Refresh (0x576d80)` | M2 | every 20 Hz tick; only its model part runs (`SWorld::RefreshModels`) |
| `SGameLogic::UpdateUnitVisuals (0x5638f0)` | M2 | every frame |
| `SScene::DrawSea (0x6b04d0)`, `DrawSkybox (0x6b7920)`, `DrawTerrainDecals (0x6ad0e0)`, `DrawTrails (0x6acf20)`, `DrawLakes (0x6ad740)`, `DrawWires (0x6b8b10)`, `DrawDecals2 (0x6b7b30)` | A | every frame (nothing to draw in menu.map, except the wires and the trails of moving units in M2) |

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
submenus (0x633770 / 0x62cbc0). The SWINE-era stubs `ZSTD_decompress`,
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
| `SDriver::StartEffects`, `StartMoveEffects`, `StartWaterEffects` | 0x55bb80 / 0x55bc50 / 0x55be10 (pixie +0x28 / +0x30 / +0x64 not typed) | a vehicle starts moving (no dust / exhaust effects) |
| `SSingleUnit::UpdateVisuals (0x5aaaa0) board and armour decals` | board part of 0x5aaaa0 | every frame (no health bars) |
| `SBuildingUnit 0x546f70 building eye heights` | 0x546f70 (engine model +0x100 / +0xd8) | map load (visibility map eye heights next to the house) |
| `SPanzersSquadUnit::Hook20 (0x59fab0) equipment slots` | 0x59fab0 | squad creation |
| `SModel::Slot_CC (0x6dad80)`, `SPixie::Slot_14 (0x69ee50)`, `SPUnitAnimation::Slot_0C (0x5cb470)` | engine / animation slots | unit creation (shadow texture, effect, animation prototype hook) |
| `SPTrailEffect (EffectType 8)`, `SPDecalEffect (4)`, `SPLiteEffect (7)` | 0x6edcd0 / 0x6ea0f0 / 0x6ed790 | effect prototypes of the units |

No longer hit: `SGameLogic::Refresh (0x576d80)` and `UpdateUnitVisuals
(0x5638f0)` as stubs, `SGameLogic::Tick_565e10` (visibility maps lifted),
`SGameLogic::Dispatch_571380`, the convoy hand-over and movement-group
"STUB:" warnings of 0x579510, `SUnit::AI_Heartbeat (0x5b37d0)`,
`SUnit::ServerRefreshMedic (0x5bfa50)` (entry lifted),
`SPanzersSquadUnit::RefreshSquadFormation (0x59dda0)`, and the SModel slots
+0x38 / +0x48 / +0x4c / +0x50 / +0x54 / +0x58 / +0x64 / +0x68 / +0x78 /
+0x80 / +0x84 / +0x88 / +0xa8 / +0xc0 / +0xdc / +0xf0 (named and lifted).
