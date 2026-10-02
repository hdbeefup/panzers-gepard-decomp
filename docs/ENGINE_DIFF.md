# Panzers HD vs SWINE engine differences

The ground truth is the HD `PANZERS.exe` (2016). Addresses are Ghidra
addresses in that exe. "SWINE" means the imported `swine-portable@95bbcc4`
code in `src/`.

## Renderer/window

This section is by agent P2-B. It covers the window, the D3D9 device, the 2D
board, textures and fonts: everything the main menu needs. 3D world rendering
and the `.4d` loader are only noted where an obvious difference showed up.

### 1. Boot-path functions

| HD function | HD address | SWINE counterpart | Difference |
|---|---|---|---|
| `SDXWindow::SDXWindow` | 0x539c70 | `SDXWindow::SDXWindow` | Same defaults for the windowed rect (40,40,800,600). HD also sets `+0xc4`/`+0xc5` (bools for HW T&L and HAL device, both 1), `+0xc6` (use Miles, 1), and clears the viewport/scene pointers at `+0xdc`/`+0xe0`. SWINE has DisplayMode, VSync, MSAA and monitor fields instead. |
| `SWindow::Create` | 0x5447e0 | `SWindow::Create` | HD uses the **ANSI** API (`RegisterClassExA`/`CreateWindowExA`, `char*` title, class name in an `SString` at `+0x48`), uses a black `hbrBackground`, and **centres** the window when X or Y is still the `0x80000000` sentinel that the SWidget constructor sets. HD registers the window in a global `SHeap<{hwnd, SWindow*}>` (0x8da79c); `StaticWindowProc` 0x545150 looks the window up by hwnd, so several windows can exist. SWINE uses the wide API, a `wchar_t ClassName[32]`, no centring, and a single `TheWindow`. |
| `SDXWindow::Create` | 0x539d70 | `SDXWindow::Create` | HD: `CreateGepard(hwnd, fullscreen, w, h, bpp, hintW, hintH, bHWTnL, bHAL, &Gepard)`, then `viewport = Gepard->vtbl[+0x3c](0)` (stored at `+0xdc`) and **`Board = viewport->vtbl[+0x4c]()`**. Sound is `CreateMilesConcert(hwnd, redistDir)` 0x684fb0, or a null concert 0x6820a0 when `+0xc6` is 0. SWINE: `CreateGepard(hwnd, fs, vsync, msaa, w, h, 1, &Gepard)`; `Board` is set inside `SGepard::Initialize`; DirectSound `CreateConcert`. |
| `::CreateGepard` | 0x678ba0 | `CreateGepard` | `new SGepard` (0x810 bytes) + Initialize, then Release on failure. Same shape; the parameter lists differ as above. |
| `SGepard::SGepard` | 0x676c20 | `SGepard::SGepard` | HD opens **no files**. SWINE loads `decals.ini` with SProperties, which panics when the file is missing. **Changed:** `DecalsIni` is now null (`// PANZERS 0x676c20`). |
| `SGepard::Initialize` | 0x67d1c0 | `SGepard::Initialize` | `Direct3DCreate9(31)`: SDK 9.0c era (SWINE gets 32 from the June 2010 SDK). DeviceType is HAL or REF depending on the bHAL flag. **Panics on fullscreen startup.** Minimum `MaxTextureWidth/Height` is **1024** (SWINE: 2048), with the same return code 4. **Changed** (`// PANZERS 0x67d1c0`). HW vertex processing needs HWTnL caps, a non-zero VS version, a caps bit and the bHWTnL flag. The device is created by a **primary `SViewport`** (0x244 bytes, ctor 0x689130, `CreateWindowedViewport` 0x68ada0), and further viewports use `CreateAdditionalSwapChain`. Present params (in the viewport): windowed, `SwapEffect=DISCARD`, `BackBufferFormat=adapter format`, `Flags=DISCARDDEPTHSTENCIL`, `PresentationInterval=IMMEDIATE`, client-sized back buffer, no MSAA. After that come `InitTextureFormats`, 6 FVF declarations, two inline vs.1.1 shaders (if VS ≥ 2.0), and the choice of shadow technique by PS version (PS2.0 / PS1.4 / depth texture `D24X8` / "compatible"). SWINE: `SetUpPresentation` (vsync/MSAA/fullscreen), 5 `CreateDevice` fallbacks, HLSL from `shaders/*.hlsl` (not shipped, so they compile to null and only a warning is logged), and three SWINE textures loaded (they return -1 harmlessly). |
| `SGepard::InitTextureFormats` | 0x67cd30 | same name | Same format preferences (DXT1 → R5G6B5, DXT1 → A1R5G5B5, DXT5 → A8R8G8B8 → A4R4G4B4, X8R8G8B8 → A8R8G8B8, A8R8G8B8, RT R5G6B5 → X8R8G8B8 → A8R8G8B8). HD stores an internal pixel-format enum (for example DXT1 = 0x18), while SWINE stores `D3DFORMAT`. SWINE adds the NULL/depth shadow formats and a non-power-of-2 probe. Not changed. |
| `SBoard::SBoard` | 0x6c2500 | `SBoard::SBoard` | HD gets `new SBoard(clientW, clientH)` (0xe8 bytes) from the viewport. It sets RefCount=1 and duplicate-board protection, makes a 32x32 A8R8G8B8 offscreen surface for the hardware cursor, and calls `SetHardwareMouseCursor`. SWINE **panicked unless 3 HD-remaster `.ttf` files** could be registered. **Changed:** they are registered only if present (`// PANZERS 0x6c2500`). The hardware-cursor surface is not ported. |
| `SWindow::Run` | 0x544db0 | `SWindow::ProcessMessages` (close) | HD returns void and has no ModalResult or `shouldClose` handling. **Lifted** as `SWindow::Run()` (`// PANZERS 0x544db0`). |
| `SDXWindow::OnIdle` (vtbl +0x90) | 0x53a0d0 | `SDXWindow::OnIdle` | HD: cursor position → `Board->vtbl[+0x9c](x, y, cursorX, cursorY)`, then `viewport->vtbl[+0x50](scene, 0)` renders. It **returns `+0xd8` OR event-record OR event-playback**, so it normally returns **false** and `Run` then blocks in `GetMessage`; continuous rendering needs the `+0xd8` flag (writer not identified). SWINE always returns true (busy loop) and renders with `Gepard->RenderScene(0)`. Not changed: `SSuperWindow::OnIdle` 0x65ae50 (owned by P2-D) returns this value. |
| `SDXWindow::OnSize` | 0x53a1b0 | same | HD logs the FPU control word before and after and calls `viewport->Resize`. SWINE calls `Gepard->Resize`. |
| `SDXWindow::OnDestroy` | 0x53a040 | same | HD releases the scene (`+0xe0`), `Board`, `Concert` and `Gepard` (in that order) and logs "releasing scene". SWINE releases MBox, Concert and Gepard. |

### 2. Textures (TGA, `_hq` / `_a`, DXT)

- `SGepard::LoadTexture` 0x67ea30 (vtbl +0x44):
  - HD compares names case-insensitively and normalises `\` to `/` and lower case.
  - For `*_hq.tga`, when the texture cache is on (`+0x524==0 && +0x5b8`), HD **compresses to a DXT cache**: "COMPRESSING TEXTURE TO THE CACHE: %s -> %s", using the `[Paths] Cache` dir that `SSuperWindow` 0x657540 passes through Gepard vtbl +0x58. The cache file is reused when its mtime is within 2 s.
  - If the source is missing, HD tries `.dxt` (`CreateTextureFromDXT` 0x679770).
  - SWINE has no compression cache. It prefers an existing `.dxt` when the mtimes match, otherwise loads the TGA.
- `CreateTextureFromBitmap` 0x679150 suffix rules:

  | Bitmap | HD | SWINE |
  |---|---|---|
  | 32-bit `_hq.tga` | TFHiAlpha, alpha=2 | same, and SWINE also treats `.png` this way |
  | 32-bit `_a.tga` | TFAlpha, alpha=2 | same |
  | 32-bit, other names | TF1Bit, alpha=1 | same, except `skinned` names get alpha=3 |
  | 24-bit `_hq` | TFHiOpaque | same |
  | 24-bit, other names | TFOpaque | same |
  | HD internal format 0xe (16-bit) | TFOpaque | Panic |

  The power-of-2 check applies when mipmapped or when the card lacks non-power-of-2 support (`+0x490`). It is the same in SWINE.
- `SBitmap::LoadTGA`: both read uncompressed type-2 files at 16/24/32 bpp, with origin bit 0x20.

### 3. Fonts used by the menu

- The Panzers menu uses **`.font` files**: `menu/fonts/sans_serif_new/sans_serif_{14,14_black,21,21_shadow,28_shadow,18_shadow}_hq.font`, loaded in SSuperWindow, which logs "SSuperWindow::Create: Bad font handle".
- These files are loaded by `SBoard::LoadFontFileFont` 0x6c61a0 (SIBoard vtbl +0x6c). **SWINE has no such loader.**
- **Lifted:** `SBoard::LoadFontFileFont` (`// PANZERS 0x6c61a0`), added to `SIBoard`. The format comment is in `board.cpp`:
  - `FONT` chunk, `v100`, line height, then `short[256][7]` = {srcX, srcY, w, h, dstX, dstY, advance};
  - the texture is the same name with the extension changed to `tga`.
- Known deltas, all on purpose and documented at the function:
  - The dest rects get SWINE's -0.5 texel offset.
  - The line height goes into `SFontProp::fontSize`.
  - HD's SFontProp is 0x4860 bytes: it has a second 256-glyph table, a font-redirect index at +0x4824 (used by SetText when `SBoard+0x24` is set) and TrueType fields. SWINE's is 0x2430.
- SWINE `GetTextExtent`/`Render` decode **UTF-8**. Panzers strings are single-byte, so a byte ≥ 0xC1 followed by a continuation-range byte would be mis-decoded. Not changed.

### 4. Struct sizes (x86)

These are checked by `PANZERS_LAYOUT_CHECK` (`src/3dengine/panzers_hd_sizes.h`):

- A **tripwire** on the current SWINE size always applies.
- The HD size is enforced only with `-DPANZERS_STRICT_LAYOUT=1`, which fails today.

| Struct | HD size (evidence) | SWINE size | Notes |
|---|---|---|---|
| SWidget | 0x48 (dtor 0x543220) | 0x44 | see 4.1 |
| SWindow | 0x8C (dtor 0x5445f0) | 0xD0 | see 4.2 |
| SDXWindow | 0xE4 (dtor 0x539d20) | 0x120 | see 4.3 |
| SDXWidget | 0x58 (dtor 0x539990) | not asserted (P2-D's file) | SWINE carries a 260-byte tooltip text and tooltip fields |
| SGepard | 0x810 (`new` in 0x678ba0) | 0x1260 | SWINE puts editor/terrain/HD fields first; no common offsets |
| SBoard | 0xE8 (`new` in 0x68ada0) | 0x68 | see 4.4 |
| SFontProp | 0x4860 (heap stride 0x4864) | 0x2430 | |
| SFrame | 0x58 (heap stride 0x5c) | 0x50 | |
| SViewport | 0x244 (`new` in 0x67d1c0) | **missing** | not in SWINE at all |

#### 4.1 SWidget (HD)

| Offset | HD field | SWINE offset |
|---|---|---|
| +04..+10 | 4 unknown dwords | SWINE has nothing here |
| +14 | X | +04 |
| +18 | Y | +08 |
| +1C | W | +0C |
| +20 | H | +10 |
| +24 | Parent | +14 |
| +28 | Child | +18 |
| +2C | Sibling | +1C |
| +30 | Focus | +20 |
| +34 | FocusSibling | +24 |
| +38 | Enabled (byte) | +38 |
| +39 | Visible (byte) | +39 |
| +3C | Cursor (-1) | +3C |
| +40 | unknown | not in SWINE |
| +44 | Gravity | +40 |

SWINE's Left/Right/Up/Down (+28..+34) are gamepad navigation from the HD remaster and are not in Panzers.

#### 4.2 SWindow (HD)

| Offset | HD field |
|---|---|
| +48 | class name (`SString`, 8 bytes) |
| +50 | hWnd |
| +54 | Style |
| +58 | Index (into the window registry) |
| +5C | ModalTarget / event redirect |
| +60 | record stream |
| +64 | playback stream |
| +68 | bool |
| +6C | `SHeap<UserEventProp>` (0x14 bytes) |
| +80 | LastMouseX |
| +84 | LastMouseY |
| +88 | LastMouseButtons |

SWINE puts `wchar_t ClassName[32]` at +44. Its HD-only fields are MouseCursorRestricted, Dpi, Scaling and shouldClose.

#### 4.3 SDXWindow (HD)

| Offset | HD field |
|---|---|
| +8C | FullScreen (bool) |
| +90 | full-screen width |
| +94 | full-screen height |
| +98 | full-screen bpp |
| +9C | bool |
| +A0..+A8 | full-screen device parameters |
| +AC..+B8 | windowed rect (X, Y, W, H) |
| +BC, +C0 | windowed mode hints |
| +C4 | bHWTnL |
| +C5 | bHAL |
| +C6 | bMiles |
| +C8 | sound redist dir (`SString`) |
| +D0, +D4 | cursor X, Y (from WM_MOUSEMOVE) |
| +D8 | force continuous render |
| +DC | primary SViewport* |
| +E0 | scene* |

SWINE has a DisplayMode enum, VSync, MSAA, Monitor, desktop/monitor rects and an `SMessageBox*`.

#### 4.4 SBoard (HD)

| Offset | HD field | SWINE |
|---|---|---|
| +04 | RefCount | no refcount |
| +08 | device | `lpD3DDev` is at +04 |
| +0C | `SHeap<SFrame>` | `Gepard` is at +08; `Frames` follows at +0C |
| +24 | byte (font redirect) | |
| +28 | `SHeap<SFontProp>` | `Fonts` is at +24 |
| +5C | SDArray | |
| +98 | SDArray | |
| +D4 | byte | |
| +D8 | cursor surface | |
| +DC | 0x20-byte object | |
| +E4 | hardware cursor enabled | |

SWINE's `SBoard` has no refcount, keeps the `Gepard` pointer, and adds cursor icon arrays and a `frameStack`.

### 5. Vtables (HD from RTTI vftables)

#### SWidget (0x7f3770, 31 slots)

| Slot | HD virtual |
|---|---|
| +00 | dtor |
| +04 | GetPosition |
| +08 | SetPosition(x, y, w, h) |
| **+0C** | **Move(x, y)**, virtual in HD |
| +10 | Resize(w, h) |
| +14 | OnKeyDown(key, scan) |
| +18 | OnKeyUp(key, scan), **2 args** |
| +1C | OnChar(ch) |
| **+20** | **unknown bool(int)** |
| +24 | OnMouseDown(btn, x, y, keys) |
| +28 | OnMouseUp |
| +2C | OnMouseMove(x, y, keys) |
| +30 | OnMouseWheel(delta, x, y, keys) |
| +34 | OnMouseOver |
| +38 | OnMouseOut |
| +3C | OnMove(x, y) |
| +40 | OnSize(w, h) |
| +44 | OnAction(src, action, param) |
| +48 | OnUpdate() |
| +4C | OnUserEvent(msg, wp, lp) |
| +50 | OnTimer(id, elapsed) |
| +54 | InsertChild |
| +58 | RemoveChild |
| +5C | GetEventTarget |
| +60 | ParentToChild |
| +64 | IsWindow |
| +68 | GetFrame |
| +6C | SetVisible (+0x39) |
| +70 | SetEnable (+0x38) |
| +74 | SetGravity |
| +78 | Update |

SWINE has 33 slots. Compared with HD:

- **SWINE has these and HD does not:** ChildToParent, IsScaler, CanAcceptEvents, SetFocus. SWINE also orders SetVisible before SetEnable.
- **HD has these and SWINE does not:** virtual Move (+0C) and the unknown slot +20.
- The same names otherwise appear in the same relative order.
- `SButton` adds +7C (redraw). `STextButton`'s redraw is +78 (Update).

#### SWindow (0x7f3e14, 39 slots)

- Slots +00..+78 are the SWidget slots, with these overrides:
  - +08 SetPosition
  - +5C GetEventTarget
  - +64 IsWindow = true
- New in HD:
  - **+7C / +80**: two `void(void*)` virtuals that SWINE lacks. SSuperWindow overrides them to show (0x659020) and hide (0x658fc0) a status message box. **Added** as `SWindow::ShowStatusMessage` / `HideStatusMessage` (empty bodies, `// PANZERS 0x539f40 / 0x539f30`).
  - +84 OnPaint(hdc)
  - +88 OnDestroy
  - +8C OnClose
  - **+90 OnIdle**
  - +94 OnSetCursor
  - +98 WindowProc(msg, wp, lp)
- SWINE adds OnDPIChange and OnActivateApp (HD-remaster DPI handling) and lacks +7C/+80.

#### SDXWindow (0x7f2964, 46 slots)

- Overrides: +00, +08, +40 OnSize, +5C, +64, +68 GetFrame=0, +84 OnPaint, +88 OnDestroy, +8C OnClose, +90 OnIdle, +94 OnSetCursor (`SetCursor(0)` plus `Board+0xc4`), +98 WindowProc.
- New virtuals:

  | Slot | Address | Virtual |
  |---|---|---|
  | +9C | 0x53a510 | set windowed mode hints (`+bc`, `+c0`) |
  | +A0 | 0x53a380 | SetFullScreenMode(w, h, bpp, bool, a, b, c, d) |
  | +A4 | 0x53a530 | ToggleFullScreen(bool keep, x) |
  | +A8 | 0x53a250 | PrevFullScreenMode(int\*, int\*) |
  | +AC | 0x539f50 | NextFullScreenMode(int\*, int\*) |
  | +B0 | 0x53a4e0 | resize client (w, h) |
  | +B4 | 0x539f10 | IsFullScreen |

- SWINE's new virtuals are IsFullScreen, SetDisplayMode(6 args), InitDesktopSize and GetMSAALevel. **The two sets do not correspond.** P2-D's SSuperWindow (0x80a174, also 46 slots) overrides only +14, +24, +40, +44, +5C, +68, +7C, +80, +88 and +90 (0x65ae50), and none of the SDXWindow-specific slots.

#### SDXWidget (0x7f28e4, 31 slots)

- No new virtuals.
- Overrides: +08 SetPosition (board MoveFrame and ResizeFrame), +10 Resize, +24 OnMouseDown, +2C OnMouseMove, +38 OnMouseOut, +50 OnTimer (the last four are empty), +68 GetFrame (BackFrame +0x48), +6C SetVisible (board ShowFrame), +74 SetGravity (board GravitateFrame).
- SWINE's SDXWidget has the same set of overrides but adds tooltip machinery.

#### SIGepard/SGepard and SIBoard/SBoard

- **SGepard (0x816da8) has 24 slots.** Known ones: +10 SetOption, +14 GetOption, +18 GetCap, +38 SwitchModelPrototypeNodes, +3C GetViewport(i), +44 LoadTexture, +4C UpdateTexture, +58 SetCachePath.
- SWINE's SIGepard has well over 100 slots, including editor and spline methods. **There is no slot-level correspondence.** In HD, rendering and the board go through **SViewport**. Its slots, named from their callers in SDXWindow: +04 Resize, +08 SetFullScreenMode, +0C back to windowed, +14/+18 next/prev mode, +4C GetBoard, +50 Render(scene).
- **SBoard (0x882750) has 53 slots**, SWINE's SIBoard 38:

  | Slot | HD virtual |
  |---|---|
  | +08 | CreateFrame |
  | +0C | DestroyFrame |
  | +10 | MoveFrame |
  | +14 | ResizeFrame |
  | +18 | ShowFrame |
  | +1C | GravitateFrame |
  | +20 | GetFrameSize |
  | +24 | SetSpriteGlyph |
  | +28 | SetTextColor |
  | +34 | SetText |
  | +38 | GetText |
  | +3C | SetBoxColor |
  | +40..+48 | SetAnim |
  | +4C | SetMinimapGlyph |
  | +58 | SetVirtualSize(frame, w, h)? (name guessed): 0x657540 uses (scaler, 1024, 768) |
  | +60 | CreateTrueTypeFont |
  | +68 | LoadProportionalFont |
  | **+6C** | **LoadFontFileFont** |
  | +70 | LoadFixedFont |
  | +74 | LoadCustomFont |
  | +7C | LoadSingleFont |
  | +80 | ReleaseFont? (called on the old font in SetSpriteGlyph/SetText) |
  | +88 | GetTextExtent |
  | +8C | GetFontHeight |
  | +94 | LoadCursorSet(file, size, count, hotspots) 0x6c59e0: lifted as `SBoard::LoadCursorSetFile` |
  | +98 | UnloadCursorSet 0x6cbd00 (SWINE `UnloadCursorSet`) |
  | +9C | SetCursor(x, y, cx, cy) |
  | +A0 | per-frame time |
  | +C4 | apply hardware cursor |
  | +C8 | SetHardwareMouseCursor |

  SWINE lacks GetText, GetFontHeight and SetVirtualSize; it has SetScaleFactor on scaler frames instead.

### 6. Public API changes in this branch

- `SIBoard` / `SBoard`: `+ int LoadFontFileFont(const char *filename)`. This is a new pure virtual in `SIBoard`, implemented by `SBoard`.
- `SIBoard` / `SBoard` (P3-X): `+ void LoadCursorSetFile(const char *filename, int size, int count, const POINT *hotspots)` (`// PANZERS 0x6c59e0`, HD board +0x94). It loads the cursor glyphs with `LoadFixedFont(file, size, size, 256/size, count)` and copies the hotspot table; Panzers calls it with `menu/cursor2_hq.tga`, 40 px, 21 glyphs. SWINE's `LoadCursorSet(scale)` (atlas PNG plus `.cur` files) is kept but unused.
  - Cursor model: HD draws the software cursor (0x6ca240) unless the D3D hardware cursor is on (+0xe4, set by +0xc8 from options.ini `Hardware Mouse Cursor`); glyph 10 is always software. SWINE's `SBoard::Render` draws the same way, and now also skips glyph -1, as HD does. **Not ported:** the D3D hardware cursor (+0xc4 0x6ca4e0: a 32x32 surface tinted by the cursor variant, then `SetCursorProperties`/`ShowCursor`), and the variant argument of +0x9c. The recompile always draws the software cursor and logs when `Hardware Mouse Cursor = 1`. The Windows cursor is hidden in the client area by `SDXWindow::OnSetCursor` -> `ApplyHardwareCursor` -> `::SetCursor(0)`, as in HD.
- `SWindow`:
  - `+ void Run()`
  - `+ virtual void ShowStatusMessage(void *)`
  - `+ virtual void HideStatusMessage(void *)`
- No signature in `gepard.h`, `dxwindow.h` or `board.h` changed, apart from the `SBoard` additions above.

### 7. Not done / not verified

- None of the struct layouts were changed to HD's. The SViewport layer, the Miles concert in `SDXWindow::Create`, the ANSI window class and the window registry are not ported.
- The HD `OnIdle` "render only when flagged" semantics are documented but not ported.
- The SWidget vtable slot +20 and the SWindow +7C/+80 names are unknown.
- The HD texture compression cache is not ported.
- SWINE's `BackbufferScreenshot` panics: the back buffer cannot be locked, which gives `LockRect: D3DERR_INVALIDCALL`.
- Text rendering was checked visually with only one font (`sans_serif_21_shadow_hq.font`).

## Sound/Bink

### Which concert Panzers builds

`SDXWindow::Create` 0x539d70 chooses the concert from options.ini:

- With `Use Miles = 1` (window +0xC6, the shipped default), it calls the
  factory 0x684fb0 with `(hwnd, "Miles provider" string, 0)`.
  - The factory runs `new SMilesConcert` (0x80 bytes) and the ctor 0x684300.
  - It then logs the FPU control word and loads `fldcw 0x007F`.
- Otherwise it calls 0x6820a0, a DirectSound `SConcert` (ctor 0x681710,
  vftable 0x81a3f0). That class is the SWINE-like path: it uses the in-exe
  `SMpegAudioDecoder` / `SStreamingPlayer` (mdec) for MP3.
- The result goes to the global concert pointer 0x8f1c5c. The live
  SMilesConcert is also kept in 0x92e798.

### SMilesConcert compared with SWINE's SConcert

| | SWINE `SConcert` (src/sound/concert.cpp) | Panzers `SMilesConcert` (0x684300..0x6876ff) |
|---|---|---|
| Backend | DirectSound 8 | Miles 6.5c (`AIL_open_digital_driver(44100,16,2,0)`), 3D through a Miles provider (default "Miles Fast 2D Positional Audio", with fallback) |
| MP3 | in-tree mdec decoder, which needs `MpegAudioPrecalculate` | Miles `mssmp3.asi` (redist dir `miles`). Sound effects use `AIL_decompress_ASI` in PrecacheSound 0x686220; music uses `AIL_open_stream` 0x684df0 |
| File I/O | engine streams | `AIL_set_file_callbacks` routes Miles I/O to `SFileSystem::Open` 0x65f420 (callbacks 0x685540/520/590/560), so files inside paks work |
| Vtable | 28 slots (`SIConcert`) | 34 slots (vftable 0x81ab4c). Same order as Panzers' DirectSound `SConcert` |
| Music | `StartStreamingPlayback(file1, file2)` / `NextTrack` | Playlist API: Clear +0x6C, Add +0x70, Shuffle +0x74, Start(loop) +0x78, PlayNext +0x7C, Stop(fade) +0x80, IsPlaying +0x84. The next track starts from the stream callback 0x6875a0 |
| Volumes | int dB tables | float gains: music +0x34, sfx +0x38, voice +0x3C = slider × 0.1. Per-sound gain is `sqrt(10^(dB/20))` |
| Panzers-only | – | `GetDigitalDriver` +0x18 (for `BinkSetSoundSystem`), provider list/select +0x1C/+0x20, Pause/ResumeAll +0x60/+0x64, SetSoundVolume +0x48 |
| SWINE-only | `AddRefToCachedSound`, `ReleaseCachedSound`, `DumpChannels` | none. The cache RefCount counts live sounds instead |

How the menu uses it:

- `SSuperWindow::Initialize` 0x657910 calls SetVolume(0/1/2, options), then
  ClearPlaylist, AddToPlaylist("music/Menu.mp3"), StartPlaylist(true).
- The scene setup at 0x658690 restarts `music/menu.mp3` (shuffled) when
  `IsStreamPlaying()` is false.
- Widgets call `PlaySound("menu/button_*.wav", dB, 0, -1)`. The file is
  found as `sounds/menu/...`.

### What was done (option a)

`src/sound/milesconcert.{h,cpp}` lifts all 34 slots plus the ctor, dtor,
factory, SpawnSound, OptimizeSoundGroup and the file and stream callbacks
(46 `// PANZERS` markers).

- `SMilesConcert` implements SWINE's `SIConcert`, so the existing engine
  code needs no changes. Each SIConcert method maps onto the matching
  Panzers slot.
- The Panzers-only methods are on `SIPanzersConcert : SIConcert`.
- The object layout is the original's: `static_assert(sizeof == 0x80)`.
- `GEPARD_AUDIO_BACKEND` gains `miles`, which is now the default. `dsound`
  and `miniaudio` are still available.
- `CreateConcert(HWND)` (called by `window/dxwindow.cpp`) creates the Miles
  concert with the default provider.
  - Once options.ini is lifted, call `CreateMilesConcert(hwnd, provider, 0)`
    instead.
  - The `Use Miles = 0` DirectSound SConcert of Panzers is **not** lifted.
- **MpegAudioPrecalculate:** not needed on the Miles path. In Panzers, mdec
  (`SMpegAudioDecoder` 0x6c18e0, `SStreamingPlayer` 0x6c1af0) is reached only
  from the DirectSound SConcert (0x682670, 0x684140). The SWINE stub stays
  for the dsound backend.

Deliberate deviations, all commented in the code:

- `SetProvider`'s failure log. The original passes an SString by value to a
  `%s`.
- `CreateConcert` uses a fixed provider string.
- SWINE-compat shims for the methods Panzers lacks.

These original bugs are kept as they are:

- PlaySound with channel 0 never plays.
- SpawnSound leaks the sample handle when `AIL_set_sample_file` fails.
- OptimizeSoundGroup has no guard on its 128-entry buffer.

### Bink intro (`src/panzers/bink.{h,cpp}`, library `panzers_bink`)

The original keeps HBINK/HBINKBUFFER in SSuperWindow +0x1B4/+0x1B8:

- Open 0x657ef0 runs `BinkSetSoundSystem(BinkOpenMiles, concert->GetDigitalDriver())`, then `BinkOpen`.
  - In fullscreen it calls `BinkBufferSetResolution`.
  - It calls `BinkBufferOpen(hwnd, w, h, 0x44800000)` and retries with `0x44000000`.
  - It finishes with `BinkBufferSetScale(screen)`.
- The step 0x65ba00 runs Wait, DoFrame, Lock, CopyToBuffer, Unlock and Blit.
  - When `FrameNum == Frames` it closes and continues with Initialize + LoadMainMenu.
  - Otherwise it calls NextFrame.
- Close 0x65b830 calls window vtbl+0xA4(1,0) when fullscreen and
  `NetWkstaGetInfo` major version > 9. It then closes the buffer and the video.

**The video is blitted by BinkBuffer straight to the window
(DirectDraw/DIB), not into a D3D9 texture.**

Interface for SSuperWindow:

- `SBinkVideo::Open(file, hwnd, w, h, fullscreen, driver)` returns false
  when BinkOpen fails.
- `Step()` returns false when the video is finished and already closed.
  The caller then runs Initialize + LoadMainMenu.
- `Draw()` re-blits the current frame.
- `Close()`.
- `OnRestoreDisplay` is a callback for the vtbl+0xA4 restore.
- `BinkShouldSkipIntro(cmdline, file)` adds `-nointro` and a skip when the
  file is missing. The original has neither.

### Verification

`soundtest.exe` (src/tools/soundtest) initialises Miles the Panzers way.

- It plays `sounds/menu/button_down.wav` and streams `music/menu.mp3`,
  both extracted from panzers.pak.
- It steps `intro.bik` for 60 frames.
- The checks are API return codes, `AIL_sample_status`, and the Bink frame
  counter. Audible output was not checked.

## Options widgets (P3-Y)

The Options screens (`src/panzers/optionsmenu.*`) need a slider, check box,
drop list, list box and scroll bar. SWINE has classes with these names, but
they do not behave like HD's, so the HD versions are lifted in
`src/panzers/pzwidgets.*` (namespace `pz`):

| Widget | HD | SWINE | Difference |
|---|---|---|---|
| `SSliderH` | ctor 0x540ef0, vftable 0x7f3514 | `window/sliderh.*` | HD loads `menu/widgets/arrows_medium_hq.tga` (fixed font 21x22) and `menu/widgets/slider_button_hq.tga`. SWINE loads `menu/{pig,rabbit}_arrows_medium.png` and `menu/csuszka_gomb.png`, which are not in the Panzers paks. SWINE also adds gamepad focus keys and reads the SWINE `SOptions`. The action codes are the same (0x53481/0x53482/0x53484). |
| `SCheckBox` | 0x537e20, 0x7f262c | `window/checkbox.*` | HD draws the box from controls glyphs 0x15..0x18 and the label in font 3, plays `menu/radiobutton.wav` and sends 0x42541/0x42542/0x42543. SWINE takes font and colours in `Create` and uses its own skin. |
| `SDropList` | 0x538bb0, 0x7f27f4 | `window/droplist.*` | HD uses controls glyphs 10..0x10, a fixed 0xaf x 0x22 size and 0x18-pixel rows, and calls board +0x5c to raise the open list above its siblings. The recompile lifts that slot as `PzBringFrameToFront`, which relinks `SBoard::Frames`. HD sends 0x444c1. |
| `SListBox` / `SGenericListBox<SListBoxItem>` | 0x53bdc0 / 0x53bd10, 0x7f2e1c | `window/listbox.*` | HD sizes rows by board +0x8c GetFontHeight (SWINE has none; the recompile reads `SFontProp::fontSize`), draws a 9-slice frame from glyphs 0x2f..0x37 (0x543d90) and embeds an `SScrollbar` at +0x80. |
| `SScrollbar` | 0x540630, 0x7f348c | `window/sliderv.*` (different design) | HD uses controls glyphs 0x21..0x2e and sends 0x53421/0x53422/0x53423. |

Other engine-facing details:

- **Display modes.** The Graphics page reads the primary `SViewport`'s mode
  lists (+0x64/+0x68/+0x6c/+0x70, built by 0x689f80). They contain the
  resolutions with refresh rates per R5G6B5/X8R8G8B8, and multisample levels
  1..15 with every quality level. SWINE has no `SViewport`, so
  `optionsmenu.cpp` builds the same lists once from `SGepard::lpD3D`.
- **Brightness.** The slider sends 0x4f565, which calls Gepard +0x1c
  (0x680540): a gamma ramp `pow(i/255, 1/gamma) * 65535` with
  `gamma = (v - 5) * 0.1 + 1`, sent to `IDirect3DDevice9::SetGammaRamp`. This
  is lifted as `PzSetBrightness` on `SGepard::lpD3DDev`. D3D9 applies it only
  in full screen.
- **Graphics Apply (0x4f564).** HD stores the full-screen mode through
  SDXWindow vtbl +0xa0 (0x53a380), which resets the device only while full
  screen, and then sets Gepard options 2/3/8/9/10 and the board hardware
  cursor. The recompile copies the mode into the SWINE `SDXWindow` fields
  (used next time full screen starts). The Gepard and board part is the
  logged stub `PzStub_ApplyGraphicsOptions`, because SWINE's renderer has no
  `SetOption`. All values are in options.ini and are read at the next start.
- **Shadow cap.** Gepard GetCap(0) (0x67a7d0, SGepard +0x540) decides
  whether "Self Shadow" is offered. SWINE has no such cap; the HD value on a
  PS 2.0 card (2) is assumed.
- **Volumes.** 0x4f413..0x4f415 call `SIConcert::SetVolume(group, 0..10)`,
  which stores `v * 0.1` in the Miles concert's music +0x34, effects +0x38 and
  voice +0x3C gains. 0x4f416 calls `SetReverseStereo`.
- **SSettings.** `KeyboardBindings` is at +0xf0, not +0xec (0x64f860 writes
  +0xf0; getter 0x64dfe0). `SSettings` grows to 0x218 bytes for `RGPass`
  (+0x1a4) and the 27 hotkeys from `keys<n>.ini` (+0x1ac..+0x214).
- **HD quirks kept as they are:**
  - Graphics Apply writes `Effects detail = -1`, because it reads a drop
    list (+0x600) that is never created.
  - Apply resets the AA level and quality to 0 when the ini value is not in
    the list.
  - Save writes "Unit acknowledgement", but the loader reads "Unit Voice",
    so that value does not survive a restart.

  The first two were checked against the original's options.ini output.
  The third was found from the strings and was not run.
- **Recompile-only.** Esc on the Options menu acts as Back. HD ignores Esc
  there (checked on the original).
