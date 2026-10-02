# SWINE engine import notes (Phase 1)

Codename Panzers: Phase One and S.W.I.N.E. both run on Stormregion's "Gepard"
engine. Phase 1 brings the game-agnostic engine layer of an existing S.W.I.N.E.
HD decomp into this repo as a buildable x86 skeleton. No engine logic was
rewritten. Panzers-specific work comes later.

## Source

- Repository: `swine-portable` (private), branch `feat/x64-recon`
- Commit: **`95bbcc4`** ("fix(x64): guard attacker slot in CheckProjectileCollision tankkiller branch")
- Extracted with `git archive 95bbcc4 <paths>`. The files are verbatim apart from
  the module `CMakeLists.txt` path fixes listed below.

## Imported

| Source path              | Destination              | Files (.cpp/.h) | Lines  | `//----- (ADDR)` bodies |
|--------------------------|--------------------------|----------------:|-------:|------------------------:|
| `core/` (+ `core/chaos`) | `src/core/`              | 26              | 2,979  | 106 |
| `common/`                | `src/common/`            | 12              | 2,926  | 91  |
| `3dengine/`              | `src/3dengine/`          | 41              | 58,857 | 556 |
| `window/`                | `src/window/`            | 64              | 16,068 | 492 |
| `sound/`                 | `src/sound/`             | 5               | 3,905  | 46  |
| `mdec/`                  | `src/mdec/`              | 15              | 8,357  | 30  |
| `platform/`              | `src/platform/`          | 5               | 1,036  | 33  |
| `third_party/miniaudio/` | `third_party/miniaudio/` | 1 (+LICENSE)    | 95,864 | n/a |

Some imported files are not compiled by default:

- `sound/concert_ma.cpp` and `sound/miniaudio_impl.cpp` are only built with
  `-DGEPARD_AUDIO_BACKEND=miniaudio`. This build is unverified, and
  `third_party/miniaudio` exists only for it.
- `platform/gogmanager.cpp` is only built with `-DPLATFORM_BACKEND=GOG`.

Some imported files contain HD-remaster features rather than original engine
code: `common/hdbeefup.h`, `modmanager`, `unitregistry` and `keybinds`. The
`window/` widgets also include multiplayer list boxes (`matchlistbox` and
`playerlistbox`). They are imported unchanged. Remove them or adapt them when
the Panzers code is lifted.

## Excluded (never import)

`world/`, `game/`, `network/`, `raknet/`, `editor/`, bots, `tools/`, `script/`,
`shaders/`, `mods/`, `android/`, reccmp files, research/design docs (`*.md`
in the source root), and IDA/Ghidra splits. `zstd` is also excluded (see below).

## Build-system changes to imported files

The module CMakeLists used `${CMAKE_SOURCE_DIR}/<module>`. They now use
`${PANZERS_SRC_DIR}/<module>`, where `PANZERS_SRC_DIR` is `${CMAKE_SOURCE_DIR}/src`.
The affected files are `src/3dengine`, `src/mdec`, `src/sound` and `src/window`.
The miniaudio branch in `src/sound/CMakeLists.txt` also adds `${PANZERS_SRC_DIR}`
to its include path, so that `concert_ma.cpp`'s `#include "../third_party/miniaudio/miniaudio.h"`
still resolves.

Top-level `CMakeLists.txt` (adapted from swine-portable):

- The project is `PanzersGepard`. Configuring for 64-bit stops with a FATAL_ERROR.
- It uses the same MSVC flags (`/W3 /wd4302 /wd4996 /GR /EHsc /GS- /Zc:wchar_t /Zc:forScope /fp:precise`)
  and `/MACHINE:X86`. Release builds also get `/Zi` and `/DEBUG /OPT:REF /OPT:ICF`.
- DXSDK discovery is the same: `$DXSDK_DIR`, then `../dxsdk`, then `C:/Program Files (x86)/Microsoft DirectX SDK (June 2010)`.
- The audio backend is `dsound` (DirectSound + `mdec`) by default.
- zstd is not built.
- `src/main` contains the `panzers` WIN32 executable. The engine libraries are
  linked with **`/WHOLEARCHIVE`**. Without that flag the linker would skip every
  object the placeholder WinMain does not reference, and hide unresolved
  externals. With it, every imported object has to link.

## Cut dependencies and stubs

None of the stub bodies are copied from SWINE gameplay files. Each is a minimal
stand-in. Each function stub calls `STUB_LOG("Name")` (see `src/stubs/stub_log.h`),
which writes `STUB: <Name> called (not implemented)` to OutputDebugString and
stderr the first time it runs.

### Function stubs (12, counted by the census)

| Symbol | Referenced by | SWINE definition | Stub behaviour |
|---|---|---|---|
| `ZSTD_decompress` (C) | `3dengine/animation.cpp` (ANI2 format) | zstd library | returns -1. ANI2 is an HD-only format, so Panzers data should never reach it |
| `DXGetErrorStringA` (C, cdecl) | `3dengine/*`, `sound/concert.cpp` | `game/game.cpp`. The `dxerr.lib` export is `__stdcall` and does not match | returns `"HRESULT 0x%08X"` |
| `Format(SString*, const char*, ...)` | `common/swineversion.cpp` | `game/game.cpp` | vsnprintf into a new SString |
| `GetText(const char*)` | `window/*` | `game/game.cpp` (messages ini) | returns the key unchanged |
| `TimerProc` | `window/widget.cpp` (`SWidget::SetTimer`) | `game/game.cpp` | no-op, so **widget timers never fire** |
| `MpegAudioPrecalculate` | `sound/concert.cpp` | `game/game.cpp`, together with the `*Precalculate` table builders | no-op. **`SMpegAudioDecoder` (MP3 streaming) will Panic** until the routine is lifted |
| `DrawDebugPickerOverlayFromGepard` (C) | `3dengine/gepard.cpp` (`RenderScene`) | `world/world.cpp` (editor overlay) | no-op |
| `SMenuBackGroundView::~SMenuBackGroundView` | `window/dxwidget.cpp` (vtable) | `game/menubackgroundview.cpp` | empty. The class layout is copied from `dxwidget.cpp` into `stub_window.cpp` and must stay in sync |
| `SMenuBackGroundView::Update` | same | same | no-op |
| `MatchInfo::MatchInfo` | `window/matchlistbox.cpp` | `network/matchmaking.cpp` | zeroes the fields |
| `MatchInfo::~MatchInfo` | same | same | frees the SString buffers |
| `MatchInfo::operator=` | same | same | copies the fields |

### Data stubs (`src/stubs/stub_globals.cpp`, not counted)

These were defined in `game/game.cpp` unless noted otherwise. All are null or
zero. They should move into the lifted Panzers game layer once it owns them.

- `Board`, `Gepard`, `Concert`, `Options`, `Timer`, `TheWindow`, `hInstance`
- `PlayerColors[13]` is all zeros. The Panzers table still has to be lifted from PANZERS.exe.
- `g_MenuRace`, `CurrentLanguage`
- `TimerList`, `SWidget::CaptureTarget`, `SWidget::LastMouseTarget`
- `g_HideWorldOverlays` (from `world/world.cpp`)
- `SMulti::instance` (from `network/multi.cpp`). `window/playerlistbox.cpp`
  declares its own two-field `SMulti`, and the copy in `stub_globals.cpp` must
  match it.

## Tripwire census

`tools/census.py src` prints one line. The line appears at configure time
(`-- CENSUS: ...`) and as a POST_BUILD step after every link of `panzers`:

```
CENSUS: <N> lifted bodies (PANZERS 0xADDR markers) + <M> SWINE-shared bodies + <K> stubs
```

- **lifted** counts `// PANZERS 0x<hex>` lines anywhere in `src/`. Put this
  marker above every function lifted from PANZERS.exe.
- **SWINE-shared** counts the IDA markers `//----- (<hex>) ---` inherited from
  the SWINE decomp. The addresses are **S.W.I.N.E. HD addresses, not Panzers
  addresses**. `src/stubs/` is excluded from this count.
- **stubs** counts the `STUB_LOG("...")` call sites in `src/stubs/*.cpp`.

Baseline at import: `CENSUS: 0 lifted bodies (PANZERS 0xADDR markers) + 1354 SWINE-shared bodies + 12 stubs`.
Read the line after every build. If a number changes unexpectedly, function
bodies were dropped or duplicated, or got silently replaced by stubs.

## Build

The build needs VS 2026 (generator "Visual Studio 18 2026"), the DirectX SDK
(June 2010) and Python 3 (for the census).

```
cmake -S . -B build -G "Visual Studio 18 2026" -A Win32
cmake --build build --config Release --parallel
```

The output is `build/src/main/Release/panzers.exe`. At this stage it prints
"panzers skeleton" and exits with code 0.
