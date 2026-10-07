# M6-WX: weather, flares, blob shadows and the scene's line passes

Branch `m6-wx` (from master 103b364). Census: before 2531 lifted / 1316 SWINE-shared / 139 stubs,
after 2567 / 1316 / 137 (no SWINE body replaced).

## What was missing on the campaign path

The per-mission stub lists of the M5 sweep (`scratchpad\m5ms\m5int\*.stubs.txt`) logged, in every
mission: `SPRain`, `SPSnowfall`, `SScene::DrawLines`, `SScene::DrawLakes`, `SModel::Slot_CC`, in 20
missions `SScene::DrawSea`, in 3 `SPFlare`. A weather scan (one log line per weather with a rain /
snow value, added to the WTHR log in `mapload.cpp`) gives the missions that use them:

| Rain | Russian 4, 6 (52 at the start), 7 (up to 400), 8; German 2, 3 (100 at the start), 4 (up to 301), 9, 10; Allied 4 (20 at the start), 10 (up to 180) |
|---|---|
| Snow | Russian 1, 5 (180 at the start); Allied 8 (55 at the start), 9 (up to 150) |
| Flares | German 3, German 11, Russian 5 ("ExtendedFx2" glows, directional) |

## Lifted (HD addresses)

- **Weather** `SWorld 0x6088f0` completed (`world.cpp`): the timed blend between weathers (lerp of
  the light block, the sun angles the short way round, t += 1 / (seconds * 20) per call) and the
  rain / snow effects: Rain / Snow >= 1 starts `Rain.fx` / `Snowfall.fx` (pixie +0x2c at the origin,
  facing up) and passes the value as their intensity (pixie +0x54), below 1 stops them; only while
  world +0x64c is set (StartEffects 0x5f5b50). HD runs it every logic tick from
  SGameLogic::Refresh 0x576d80 (the recompile now does, `gamelogic.cpp`, one line).
  The rain ambient sound too: the world ctor 0x5d2f90 precaches "sounds/ambient/eso/eso60stereo -
  eros.mp3" (+0x63c), the rain loops it (Concert +0x30 -> +0x640) at volume Rain / 400 (+0x48), the
  dtor destroys the rain / snow effects and removes the sound (`weathersound.h` forwards to the Miles
  concert). HD bug kept: the stop test reads +0x638 after clearing it, so the sound is never removed
  when the rain stops (it keeps its last volume until the world goes).
- **Rain** EffectType 2 (`weatherfx.cpp`): SPRain 0x6ea820 / 0x6eaa80 / Init 0x6eac50 /
  CreateInstance 0x6eabd0, SRain 0x6ea860 / 0x6eab20 / intensity +0x14 0x6eb5b0 / Process 0x6eaeb0 /
  Render 0x6eb170 / Stop 0x6eb5d0. 4 x 4 tiles of 400 drops over 8 x 8 units, one tile per visible
  terrain parcel within sqrt(4800) of the camera, drops as 2-unit quads slanted by the wind, 0.04 wide
  across the camera yaw (FVF 0x102).
- **Snow** EffectType 3: SPSnowfall 0x6eb5e0 / 0x6eb940 / 0x6ebb20 / 0x6ebaa0, SSnowfall 0x6eb620 /
  0x6eb9e0 / 0x6ec570 / Process 0x6ebd90 / Render 0x6ec0c0 / Stop 0x6ec590. Flakes sway (sin of a
  per-flake phase), screen-space sprites of size 0.07 projected by vp +0x3c (FVF 0x1c4). HD keeps
  [4][32] tables and uses columns 0..3 only; HD leaves the track cursor and time uninitialised
  (zeroed here). Kept: HD draws every flake of a tile, also those still under the ground.
- **Lens flares** EffectType 1: SPFlare 0x6df880 / 0x6dfa00 / Init 0x6dfc60 / CreateInstance 0x6dfb30
  (pixie flare heap +0x3c), SFlare 0x6df7a0 / 0x6df8b0 / Process 0x6dfea0 / Stop 0x6e08a0 (deletes
  itself) / BeginBatch 0x6e00c0 / Draw 0x6e0150 / EndBatch 0x6e0100 / occlusion query 0x6dfeb0 /
  0x6dfc40 / 0x6dfc10, and the flare pass of `SPixie::Render 0x69ea50` (batched by texture, then one
  occlusion query per flare). Visibility = last query's pixels / 45; alpha and size =
  (visibility * direction weight) ^ Alpha_Exp / Scale_Exp. Kept: HD fills only three of the four
  vertices of the query strip (the fourth stays zero).
- **Soldier blob shadow**: `SModel +0xcc` 0x6dad80 as `SetShadowTexture` (was `Slot_CC`), the decal
  in SModel::Update 0x6dba80 (create at the model position, alpha 1, follow the world position; only
  when not fading out and Gepard option 2 or 5 is off), its removal 0x6da0c0 / SetVisible / the dtor
  0x6d50c0 / ReleaseShadowBuffer 0x6a2670, and the three callers: SUnit::Place 0x5c5160 (anim +0x3c),
  SWalkerAnimation::InitModel (WProto ShadowTexture), the walker death (-1). Gepard option 5 is always
  1 (SSuperWindow::Initialize), so as in HD the blob only shows with `Shadows = 0`; with shadows on,
  soldiers keep their shadow-buffer shadows.
- **Scene line passes** (`pzscene_wx.cpp`): 0x6b04d0 is not the sea: it outlines the rectangles of
  heap +0x1cc on the terrain grid (now `SScene::DrawOutlines`, `SOutlineRect`), and DrawLines 0x6acf20
  draws the +0x270 line list. Nothing lifted fills either list, so both draw nothing in the missions.

## Still stubs on the mission path, and why

- **Water: `SScene::DrawLakes 0x6ad740`, `DrawRivers 0x6b0920`** and what feeds them: the world keeps
  LAKS / RVR2 raw (`mapload.cpp`), so no lake or river exists. Many missions have them (LAKS: Allied 1,
  2, 4, 6; RVR2: Allied 1, 4, 5, 7, 10 and others). A faithful lift is: LAKS 0x5f05c0 + lake chunk
  0x5f11d0 (world +0x73b0, 0x38 each), SWorld 0x607ad0 (per lake: scene +0xa0 / +0x9c CreateLake
  0x6a7940 at terrain height + depth, then the block map dirty with 0x600 over the lake bounds),
  CreateLake 0x6a7940 (4.2 KB: flood fill of the terrain below the level from the lake point, the
  terrain water heights 0x6f8b90 = **world WaterHeights**, per parcel index buffers, the sparkle
  texture), DestroyLake 0x6aa3f0, DrawLakes 0x6ad740 (10 KB, waves with sin / cos), and the same
  for rivers (RVR2 0x5f0960, 0x6015d0, scene Slot_B0 0x6a90e0, DrawRivers 0x6b0920, 6170
  instructions). Not started: about 16 KB of HD code for the lakes alone (more for the rivers), and it changes game state
  (the water heights feed the block map water bits and the units). **The Tutorial map has 2 lakes
  and 2 rivers** (tut1 log: LAKS 93 bytes / RVR2 647 bytes), so the faithful lift will probably move
  the Tutorial replay reference; the coordinator should decide that before it is done.
- `SScene::Slot_28 0x6ac6e0` (cut-scene start): a resource preload hint (0x6d8700 -> mesh 0x6ce9e0,
  terrain textures 0x67f450 -> texture +0x24); no visual effect.
- `SModel::Slot_BC 0x6dae10` (2 missions, units with Invisible): stores the argument (3) at model
  +0xf4, which the recompile uses as the CreateModel heap flag (`SScene::RemoveModel`); the HD
  meaning of +0xf4 is not settled, so it stays a stub.
- Not mine: `SPzGepard::SwitchModelPrototypeNodes 0x681010`, `DrawDebugPickerOverlayFromGepard`,
  `SVersion::GetVersionString`.

## Render-only

Rain, snow and flares use CRT `rand()` (0x78c846) as HD does, never the world LCG; the weather's
blend and effect handles are world fields that no CRC reads. Regression (`coord\regress.ps1`, build
8a2fb96): menu 1181 / 0 mismatches, tc1 3901 / 0 differing, Tutorial 2012 / 0 differing, all three
closed. (An earlier run stopped tc1 at frame 2366 after UI actions the replay does not contain, i.e.
input from outside the game; the rerun was exact.) Sweep `m5ms\sweep.ps1 -Frames 1500 -Tag
m6wx_sweep` on Russian 5, 6, 7, German 3, 4, Allied 8, 10: all OK, no panic; stubs left there:
DrawLakes, Slot_28, SwitchModelPrototypeNodes, GetVersionString, DrawDebugPickerOverlay.

## Files touched outside src/3dengine

- `src/world/world.cpp`, `world.h`: SWorld 0x6088f0 (weather, my area) + `SWorld::ApplyWeather`, the
  rain sound in the world ctor / dtor and the dtor's rain / snow effect teardown.
- `src/world/mapload.cpp`: one log line per weather with rain / snow (recompile log only).
- `src/game/gamelogic.cpp`: the per-tick `ApplyWeather` call (0x576d80).
- `src/game/unitbase.cpp`, `unitanim.cpp`: the three `SetShadowTexture` calls (were comments / a
  call to the stub).
- Shared with agent CD (src/3dengine/pz): `imodel.h` (slot +0xcc renamed `SetShadowTexture(int)`),
  `pzmodel.h` / `pzmodel.cpp` (SetShadowTexture, DropShadowDecal, the decal block at the end of
  SModel::Update, the dtor, SetVisible / fade-out removal). No change near +0x108 / Slot_E4.

## Shots (scratch, not committed)

`scratchpad\m6wx\shots\` (1024 x 768 window, `shot.ps1`, PrintWindow). "Before" is the M5
integration build `m5int.exe` from `coord\run` (no weather / flare / blob code).

- Rain, Allied 4 (rain 20) at 75 s: before `a4_before_075.png`, after `a4_after_075.png`.
- Snow, Russian 5 (snow 180) at 80 s: before `r5_before_080.png`, after `r5_after_080.png`.
- Rain 100, German 3 at 80 s: after `g3_after_080.png`.
- Blob shadows, tc1 replay with `Shadows = 0` at 35 s: before `tc1_noshadow_before_035.png`, after
  `tc1_noshadow_after_035.png`, zoomed side by side `blob_before_after.png`.
- Flares: not caught on screen (the Russian 5 glows are off the start view); the log showed the two
  "ExtendedFx2" prototypes loading and the per-frame draw / occlusion path running.

## References the coordinator could record from the original

1. Russian 5 (`PZ_M5_MISSION` equivalent: campaign Russian mission 5, briefing / market passed), the
   first in-game view after the intro cut-scene, 10 s in: a shot to compare the snowfall density and
   flake size.
2. Allied 4, first in-game view, 10 s in: rain streak width / length / brightness.
3. Any mission with soldiers in view, `Shadows = 0` in options.ini: a close shot of the blob shadows.
4. German 3 or Russian 5 at night / at a lamp: what an "ExtendedFx2" flare looks like.
