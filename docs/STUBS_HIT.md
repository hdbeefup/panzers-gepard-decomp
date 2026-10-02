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
| `SSuperWindow::Initialize menu cursor menu/cursor2_hq.tga (board +0x94)` | board +0x94 in 0x657910 | Initialize | 2, 3 |
| `SUnitRegistry ctor + LoadUnitFiles (0x5cfe30 / 0x5d1050)` | 0x5cfe30 and `SUnitRegistry::LoadUnitFiles` 0x5d1050 | Initialize | 2, 3 |
| `SSuperWindow::LoadMenuBackground maps/menu.map world (...)` | world part of 0x658690: 0x5d2f90 SWorld, 0x5f1990 map load, 0x55e440 camera | Initialize / LoadMainMenu | 2, 3 |
| `SVersion::GetVersionString (0x65c070)` | 0x65c070 (exported) over 0x65bbf0; returns "1.25" | LoadMenuBackground | 2, 3 |
| `DrawDebugPickerOverlayFromGepard` | SWINE editor overlay (no HD counterpart) | every frame, SGepard::RenderScene | 2, 3 |
| `SSuperWindow::OnAction unhandled action (0x659250)` | `SSuperWindow::OnAction` 0x659250, cases the shell does not handle | menu clicks: 0x4d4d5 Options, 0x4d4d6 Credits | 2 |
| `SUnitRegistry dtor (0x5d0c10)` | 0x5d0c10 | OnDestroy on quit | 3 |

These stubs exist but were **not** hit on the boot-to-menu path:
`SSettings::Save (0x64fdd0)`, which runs only when options.ini has no
fullscreen resolution or on `-name`; `SSettings::CHECKCDKEY (0x64d390)`;
`CreateHostFromCommandLine (0x6576f0)`; `ConnectToHostFromCommandLine
(0x657460)`; the `-market` and map command-line starts; `LoadMultiPreMenu
(0x658a30)`; `LoadChatRoomView (0x658050)`; and the New Game / Load Game
submenus (0x633770 / 0x62cbc0). The SWINE-era stubs `ZSTD_decompress`,
`DXGetErrorStringA`, `MpegAudioPrecalculate`, `SMenuBackGroundView::*` and
`MatchInfo::*` were not hit either.

Removed in P2-D because the lifted Panzers code now provides them:
`Format` (HD 0x51ee20) and `TimerProc` (HD 0x543cf0), both in
`src/panzers/widgetglue.cpp`.
