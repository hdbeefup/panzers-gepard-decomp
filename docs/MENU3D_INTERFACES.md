# 3D menu background: interfaces, facade and ownership (M1)

This is the contract between the four agents that build the 3D scene behind the
main menu (`maps/menu.map`). Read `docs/re/MENU3D_SCOPE.md` first for the call
graph and the size of the job. All addresses are HD `PANZERS.exe` addresses.

## 1. Quick start

| What | How |
|---|---|
| Default build and run | `PZ_MENU_WORLD` is OFF. `panzers.exe -nointro` boots exactly as before; the world part of `LoadMenuBackground` is still the logged stub `PzStub_LoadMenuWorld`. |
| Turn the world path on | `panzers.exe -nointro -menu3d`, or set `PZ_MENU3D=1`, or configure with `-DPZ_MENU_WORLD=ON` (then it is on by default and `PZ_MENU3D=0` turns it off). |
| Per-call trace | `-menu3d` turns it on. `PZ_MENU3D_TRACE=1` / `0` overrides it. |

`-menu3d` is a recompile-only switch and is removed from argv before SSettings
reads the command line (as `-nointro` is). `SSuperWindow::Initialize` logs
`3D menu world on|off, trace on|off`.

Trace lines look like `PZ3D f300: SScene::RenderViewport (0x6acaf0)`.
A frame is one `SViewport::Render`. Every call site is logged on frames 0, 1, 2
and then every 300th frame, at most 4 times per frame. Each stub also logs
`STUB: <name> called` once, whether or not trace is on.

## 2. Interfaces (`src/3dengine/pz/`, namespace `pz`)

Everything is in namespace `pz`, because SWINE's 3dengine already has global
`STerrain`, `SParcel`, `SMesh` and `SEffect` with other layouts. The
interfaces are declared in HD vtable order. Slot offsets, HD implementation
addresses and argument dword counts are in the comments. The counts come from
the `RET imm16` of each HD function, and a double counts as 2.

| Header | Interface | HD vftable (impl) | Slots | HD size | Implementation | Owner |
|---|---|---|---|---|---|---|
| `igepardhd.h` | `SIGepardHD` | SGepard 0x816da8 (SIGepard 0x816d44) | 24 | 0x810 | `pzgepard.*` facade `SPzGepard` | A |
| `iviewport.h` | `SIViewport` + `SCameraParams` | SViewport 0x878814 (SIViewport 0x87878c) | 33 | 0x244 | `pzviewport.*` facade `SViewport` | A |
| `iscene.h` | `SIScene` | SScene 0x87c078 (SIScene 0x87bf6c) | 66 | 0x2b0 | `pzscene.*` `SScene` | A |
| `imodel.h` | `SIModel` + `SIAttachable` | SModel 0x883800 (SIModel 0x8836dc) + 0x883910 at +0x04 | 67 + 4 | 0x150 | `pzmodel.*` `SModel` | A |
| `imesh.h` | `SIMesh` | SMesh 0x8830b4; SAnimesh 0x883344, SSkinnedMesh 0x8833f0, SParcel 0x8904a4, SParcel2 0x890558, SBlockMapParcel 0x89051c, SWireframeParcel 0x8904e0 | 14 | n/a | none yet | A (meshes), B (parcels) |
| `iterrain.h` | `SITerrain` + `STerrainBuffers` | STerrain 0x886bfc (SITerrain 0x886b5c) | 39 | 0x11c88 | `pzterrain.*` `STerrain` | B |
| `ipixie.h` | `SIPixie` + `SIEffect` | SPixie 0x879898 (SIPixie 0x8797e4, 26 slots); SEffect 0x883c24 | 26 (+1) / 13 | 0x7c | `pzpixie.*` `SPixie` | C |
| `src/world/worldapi.h` | globals | `g_World` 0x929a50, `g_Scene` 0x929a54, `g_Pixie` 0x929f14, `g_GameLogic` 0x8f2078, `g_WindowScene` (HD SDXWindow+0xe0) | | | `worldapi.cpp` | P0 |

`pzcommon.h` holds the HD sizes, the switches and the `PZ_TRACE` macro.
`pztrace.cpp` implements them.

**Gepard +0x5c is the effect manager:** `0x67c3b0` AddRefs and returns
SGepard+0x7f4. `SGepard::Initialize` fills that field with `new 0x7c` and
ctor `0x6941e0` (SPixie), and `SScene::RenderViewport` calls pixie +0x48 every
frame.

Named slots were identified from strings/symbols or from a world caller.
Names marked "(name guessed)" rest on a single caller. All other slots are
`Slot_XX` placeholders that keep the order.

Lifetimes:
- SScene, SModel and SPixie are refcounted (+0x04, +0x10 and +0x78; their
  ctors set 1).
- STerrain is owned by its scene: `CreateTerrain`/`DestroyTerrain`.
- The viewport and the pixie are owned by the facade.
- `GetViewport` does not AddRef; `GetPixie` does.

## 3. The Gepard facade and the frame order

The world calls the global Gepard (HD 0x8f1c58) through 24 HD slots. In the
recompile, `Gepard` is the SWINE `SGepard` (170 virtuals, no slot
correspondence). `pz::PzGepard()` therefore returns `SPzGepard`, a separate
object over the same SWINE device:

| Slot | Facade behaviour |
|---|---|
| +0x0c CreateScene | `new pz::SScene(0)`, logs "Scene created" |
| +0x10/+0x14/+0x18 Set/GetOption, GetCap | stored; GetCap(0) = 2 (PS 2.0 shadow technique, as optionsmenu.cpp assumes) |
| +0x20 LoadModelPrototype | stub, returns -1 (agent A: SPModel heap + Load4DFile) |
| +0x28 PurgeModelPrototypes | stub |
| +0x3c GetViewport(0) | the primary `pz::SViewport` |
| +0x44/+0x48 Load/ReleaseTexture | forwarded to the SWINE texture table, so handles are shared with the board |
| +0x5c GetPixie | `pz::SPixie`, AddRef'd |

**HD frame order.** This is the evidence for the order, from the disassembly
in the scope's Ghidra copy.

`SSuperWindow::OnIdle` 0x65ae50, world branch (0x65aef8..0x65b03a):
1. `while (now > +0x1a8) { SGameLogic(+0x1a0)->Refresh 0x576d80; +0x1a8 += 0.05 }`
2. `scene +0x1c(ftol((now - +0x1a4) * 1000))` and `board +0xa0(same)`, then `+0x1a4 = now`.
3. `vp = Gepard +0x3c(0)` and `g_World->ComputeCamera(vp)` 0x5ddc30. ComputeCamera does vp +0x20 SetCamera, scene +0x24, and vp +0x28 SetProjection when the camera is dirty.
4. `scene +0x20((double)((+0x1a8 - now) * 20))`.
5. `g_GameLogic 0x5638f0(Gepard +0x3c(0), same double)`, then `Concert +0x0c(0)`.
6. Event frame, news ticker, then `SDXWindow::OnIdle` 0x53a0d0. That does board +0x9c (cursor), then `viewport +0x50 Render(SDXWindow+0xe0 scene, 0)`.

So the camera is set before the render, in the same OnIdle.

`SViewport::Render` 0x68c220, for each subport:
1. `TestCooperativeLevel`. On DEVICELOST it returns. On NOTRESET it calls `SGepard::ResetDevice` 0x67fde0 and board +0xc4.
2. With a scene: `scene 0x6bbc40(vp)`, `scene 0x6a24c0(vp)`, `BeginScene`, `scene 0x6acaf0(vp)`. The scene clears through `SViewport::Clear` 0x689f10 inside `SScene::RenderScene` 0x6b7760.
3. Without a scene: `Clear(TARGET|ZBUFFER[|STENCIL], color, 1.0, 0)`, then `BeginScene`.
4. The board: `board 0x6c7150` if vp+0x240, otherwise only the cursor `0x6ca240`.
5. `EndScene`, then `Present` 0x68bfe0.

**3D first, 2D board after, in one BeginScene/EndScene.**

**Recompile frame.** `SSuperWindow::OnIdle` now ends like HD
`SDXWindow::OnIdle` when `g_WindowScene` is set. It sets the cursor, then
calls `PzGepard()->GetViewport(0)->Render(g_WindowScene, 0)`.
`pz::SViewport::Render` then:
1. runs `PrepareViewport` and `UpdateViewport` (0x6bbc40 and 0x6a24c0, before BeginScene);
2. calls SWINE `SGepard::RenderScene`, which does the device-lost check, Clear, BeginScene, SWINE's empty 3D part, then the new hook `SGepard::PanzersScenePass`, then the board, EndScene and Present.

The hook runs `SScene::RenderViewport` 0x6acaf0. A `D3DSBT_ALL` state block
wraps it, so HD fixed-function state cannot leak into the SWINE board. With no
`g_WindowScene` (the default path), the SWINE `SDXWindow::OnIdle` runs
unchanged, and the hook does nothing when no scene is active.

## 4. FPU

- **HD device.** HD creates the device with BehaviorFlags `0x40` (HARDWARE_VERTEXPROCESSING) or `0x20` (SOFTWARE). Never `D3DCREATE_FPU_PRESERVE`. Evidence: `SGepard::Initialize` 0x67d3ad/0x67d3cb writes SGepard+0x458, and `0x68af5b` pushes it to `CreateDevice`.
- **Miles.** The Miles factory 0x684fb0 then loads `fldcw 0x007F`, the word at 0x8de158. That is 24-bit precision, round-to-nearest, all exceptions masked.
- **SWINE device.** The SWINE `CreateDevice` also passes no FPU_PRESERVE.
- **Logs.** Our log shows `0x027F` before D3D and after `new SMilesConcert`, then `0x007F` after the fldcw.
- **Decision: do not add `D3DCREATE_FPU_PRESERVE`.** HD itself forces 0x007F, and D3D without FPU_PRESERVE may only reset the word to single precision, which is the same state.
- **Effect on lifted code.** HD world and render code uses SSE scalar maths (MOVSS/MULSS/CVTPS2PD), and so does MSVC x86 with its default SSE2. Only x87 code sees 24-bit precision: CRT transcendental fallbacks and `ftol`. Do not "fix" it with `_controlfp`, or trigger timing (20 Hz ticks, interpolation) could drift from HD.

## 5. Ownership

| Agent | Owns (writes) | Implements |
|---|---|---|
| **A** scene/viewport/models/.4d/anim/facade | `src/3dengine/pz/pzgepard.*`, `pzviewport.*`, `pzscene.*`, `pzmodel.*`, and new files `pz/pmodel*`, `pz/panim*`, `pz/tracks*`, `pz/mesh*` | facade slots, SViewport render subset, SScene, SModel, SPModel::Load4DFile (v100+v101), SPAnim (CANM), SMesh/SAnimesh/SSkinnedMesh, lights |
| **B** terrain/parcels/roads/decals | `src/3dengine/pz/pzterrain.*`, new `pz/parcel*`, `pz/road*`, `pz/decal*` | STerrain (16 layers), SParcel/SParcel2/SBlockMapParcel, roads, terrain decals, grass layer |
| **C** effects/particles | `src/3dengine/pz/pzpixie.*`, new `pz/effect*`, `pz/particles*`, `src/core/propertystruct*` | SPixie, `.fx` loader, SEffectSet, SParticles, SLite/STrail/SDecal effects; SSoundEffect as a logged stub |
| **D** world | `src/world/` (except `worldapi.*`), `src/panzers/superwindow.cpp`, `src/stubs/stub_panzers.cpp`, `docs/STUBS_HIT.md` | SWorld ctor/dtor, LoadMap and chunk readers, Initialize, ComputeCamera, doodads, unit registry, units; later SGameLogic |
| **P0** (shared) | `pz/i*.h`, `pz/pzcommon.h`, `pz/pztrace.cpp`, `src/world/worldapi.*`, `pz/CMakeLists.txt`, `src/world/CMakeLists.txt`, this file, `tools/census.py`, the `PanzersScenePass` hook in `3dengine/gepard.*` | |

Cross-agent calls go through the interfaces, except these known non-virtual
entry points:
- SScene (A) creates `STerrain(scene, w, h, p)` (B).
- SScene (A) calls the terrain draw functions 0x6f8610, 0x6f2aa0, 0x6f46e0 and 0x6f33c0 (B). B declares them in `pzterrain.h`.
- SScene (A) calls the pixie's 0x69ea50 (C). C declares it in `pzpixie.h`.

## 6. Rules for the shared headers

1. Never reorder, insert or remove a slot, and never overload a virtual name. MSVC groups overloads in the vtable. Two HD functions with the same name get distinct names, for example `CreateModel` and `CreateModelFromPrototype`.
2. Naming a `Slot_XX` or fixing a parameter list is allowed by the agent that implements that class. Rename it in place, keep the `+offset HD address` comment, and update every implementation in the same commit. Other agents must not edit another owner's slots. They ask the owner instead.
3. Do not add data members to an interface. Data belongs in the implementation class at its HD offset. `PZ_HD_SIZE` keeps the class at the HD size, so convert padding into fields; never grow the class.
4. Changes to `pzcommon.h`, `worldapi.h` or the hook go through P0, or through the integrator after P0 is done. Keep them additive.
5. Stubs:
   - Each stub starts with `STUB_LOG("Class::Name (0xADDR)")` and `PZ_TRACE(...)`.
   - When you lift a body, drop the `STUB_LOG` line, keep `PZ_TRACE`, and put `// PANZERS 0xADDR` above it.
   - `tools/census.py` counts the `STUB_LOG` lines in `src/3dengine/pz` and `src/world` as "menu3d skeleton" stubs, so the census shows progress.
   - Never start a non-lifted comment line with `// PANZERS 0x`.
6. SWINE `world/` and `game/` code is private and must never be imported or copied. Use SWINE `3dengine` only as a reference. Never commit game data.

## 7. Gotchas

- **The decompiler hides arguments.**
  - `SModel +0x40 FindNode("Block", 0)` followed by `+0x60(idx)` is really `FindNode("Block")` and `+0x60(idx, 0)`.
  - `0x6a24c0` and `0x6acaf0` are shown as fastcall without arguments, but the caller pushes the viewport.
  - In OnIdle, the double pushed for `Gepard +0x3c` is really the second argument of `0x5638f0`.
  - Trust `RET n` and the pushes.
- **The HD scene must reset its own device state.** SWINE's `RenderScene` sets fog, the shadow map on stage 2, VS constants 12+ and `ZENABLE` before the hook. HD fixed-function code must set what it needs.
- **The state block covers only the SWINE board.** It restores the state after the HD scene. It does not isolate the HD scene from the state SWINE set before it.
- **Clears and the hook.** SWINE clears with `FogColor` before the hook. HD clears inside `SScene::RenderScene` with `SViewport::Clear`. A second clear is harmless, but use the HD one.
- **Prepare and update.** `PrepareViewport` and `UpdateViewport` run before BeginScene, as in HD. Render-target changes (for example the shadow buffer) are legal there.
- **Device reset.** It is handled by SWINE `SGepard::RenderScene` (`ResetDevice`), not by HD 0x67fde0. `D3DPOOL_DEFAULT` resources created by the HD path will not be recreated yet; use `D3DPOOL_MANAGED` until a reset hook exists.
- **The viewport facade is not HD-sized.** The device, swap chain and board are SWINE's.
- **Textures.** The facade texture handles are SWINE `SGepard` texture indices. HD's `_hq` DXT cache is not ported.
- **Unload order.** `UnloadMenuBackground` releases `g_WindowScene`, deletes SGameLogic, then deletes SWorld. The SWorld dtor releases `g_Scene`. Exit, then Back, reloads the world, as HD does.
- **Loading frame.** `SWorld::LoadMap` renders a loading frame with `Render(nullptr, 0)`. It goes through the same facade and draws only the board.
- **The HD board clock is not mapped.** HD calls board +0xa0 with the elapsed milliseconds; the SWINE board keeps its own clock.

## 8. Not verified

- Slot semantics marked "(name guessed)". The parameter lists of unnamed slots are only dword counts.
- The `STerrainBuffers` field meanings beyond Heights/Blend/Diffuse.
- The SPixie ctor refcount (assumed 1, like SScene and SModel).
- The world path was run with the skeleton world only (no map data parsed). The SWINE board was drawn after an empty HD scene pass.
- The Exit → Back reload and device loss with the world path on were not exercised.
