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
| `SUnitRegistry ctor + LoadUnitFiles (0x5cfe30 / 0x5d1050)` | 0x5cfe30 and `SUnitRegistry::LoadUnitFiles` 0x5d1050 | Initialize | 2, 3 |
| `SSuperWindow::LoadMenuBackground maps/menu.map world (...)` | world part of 0x658690: 0x5d2f90 SWorld, 0x5f1990 map load, 0x55e440 camera | Initialize / LoadMainMenu | 2, 3 |
| `SVersion::GetVersionString (0x65c070)` | 0x65c070 (exported) over 0x65bbf0; returns "1.25" | LoadMenuBackground | 2, 3 |
| `DrawDebugPickerOverlayFromGepard` | SWINE editor overlay (no HD counterpart) | every frame, SGepard::RenderScene | 2, 3 |
| `SSuperWindow::OnAction unhandled action (0x659250)` | `SSuperWindow::OnAction` 0x659250, cases the shell does not handle | menu clicks: New Game, Load Game, Multiplayer, Tutorial, Training Camp (Options lifted in P3-Y, Credits in P3-X) | 2 |
| `SSuperWindow::OnAction 0x4f564 Gepard/board graphics options (0x659250)` | renderer part of the 0x4f564 case: Gepard +0x10 SetOption 2/3/8/9/10, scene +0x104, board +0xc8 | Options > Graphics > Apply or Restore | P3-Y |
| `SUnitRegistry dtor (0x5d0c10)` | 0x5d0c10 | OnDestroy on quit | 3 |

P3-Y (branch `p3y-options`) lifted the Options screens. Its runs from
`scratchpad\p3y\run\` (`p3y_panzers.exe -nointro`) went Options > Game
Options / Graphics / Audio > Back, changed music volume, Tooltips and
Autosave, pressed Graphics > Apply, used Esc, and quit through Exit. The only
new stub hit is the graphics-options one above. `SSettings::Save (0x64fdd0)`
is no longer a stub (lifted in `src/panzers/settings.cpp`), and Options no
longer reaches the unhandled-action stub.

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
