# M6 agent CD: the `.4d` cut-scenes drawn

Branch `m6-cd` from master 103b364. All addresses are HD `PANZERS.exe`.

## What was lifted

- **0x56f0d0** (the .4d start, complete now): messages cleared, Concert +0x60; `<n>_fade.ingame`
  when it exists is loaded as the in-game animation project (+0x30c; no shipped cut-scene has
  one); a **new scene** (Gepard +0x0c, SGameLogic +0x28c) with no fog (+0x4c(0, ..)) and a
  black sun (+0x44((0, 0, 0, 1), 0, 0)); the model `<n>.4d` in it (scene +0x50(file, 0.005, 1, 0),
  +0x290, Panic "Cannot open"); the scene ambient from the model's AMBI (model +0x108, scene
  +0x3c); model flags 0x10; the length of sequence 0; subtitles, mp3, clock, +0x288; then the view
  renders the new scene (SIGameViewCallback +0x08 0x625560, `PzViewSetScene`).
- **0x582380** `SGameLogic::UpdateAnimation(viewport)`: per frame the model advances by the
  wall-clock frame time (+0x70), **its camera node drives the viewport** (model +0xe4(vp, -1)),
  the scene clock advances (+0x1c, ms), then the subtitle of round(elapsed * 25). After the
  length 0x58c100 (the colour box goes) and the end.
- **0x5652d0**: the window scene back to the world scene (0x929a54), model and scene released,
  the mp3 stopped, sounds resumed, subtitles gone, World +0x34 (the projection is recomputed).
- **SModel::ApplyCamera 0x6d62e0** (was the stub `Slot_E4`): cam -1 = the playing sequence's
  CCHG track at the playing time (**0x674910**), node matrices updated (SIAttachable +0x04
  Update(0, 0)); a camera node with a look-at target (proto node +0x10) sets eye + yaw
  atan2(dx, dz) + pitch -asin(dy) (viewport +0x20); one without sets the inverted node matrix
  (rows over the model scale, the third negated) as the view matrix; then CAM_ fov / near
  (0 -> 1.0) / far (0 -> 250.0) and, with CMFG, the scene fog.
- **SModel::GetAmbient 0x6d78e0** (SIModel slot +0x108, appended: the HD vtable has 67 slots).
- **SViewport::SetViewMatrix 0x68d310** (was the stub `Slot_1C`): view matrix set, the device
  VIEW transform while selected, the screen matrix.

## Still a stub

- The `_fade.ingame` tick in 0x582380 (0x58c020 / 0x588520(50), the in-game animation player's
  own clock): logged with STUB_LOG when a fade project was loaded. No shipped cut-scene has a
  `_fade.ingame` (pak listing), so it is never hit.
- 0x68b740 (the innermost selected subport's view) is reduced to "this viewport" in 0x68d310.

## Verified (our build, `scratchpad\m6cd\shots\`)

- German 1 with `PZ_M5_CS_FORCE=1700:ger-03-2`: the cellar scene (two soldiers and the drunk
  civilian, the camera cuts of the CCHG track) for 39.2 s with the letterbox and the mp3
  (`g32a_100`, `g32a_120`), then the mission view with the HUD (`g32a_135`).
- Allied 6 (`PZ_M5_MISSION="Allied 6"`) with `PZ_M5_CS_FORCE=2000:us-06-3`: Michelle and the
  paratroopers in the ruin, 69.2 s (`u63_135`, `u63_160`), back to the mission (`u63_185`).
- No PANIC / EXCEPTION.

## Reference wanted from the original (coordinator)

HD screenshots of us-06-3 (Allied 6, its end trigger) or ger-03-2 (German 3), a few seconds in,
to compare the lighting: the recompile's skinned characters look bright (sun black, ambient from
AMBI, plus the .4d's own LITE nodes); whether HD looks the same is not known.

## Shared edits (outside src/game/cutscene*)

- `src/3dengine/pz/imodel.h` / `pzmodel.h` / `pzmodel.cpp`: slot +0xe4 renamed ApplyCamera, slot
  +0x108 appended (GetAmbient), 0x674910.
- `src/3dengine/pz/iviewport.h` / `pzviewport.h` / `pzviewport.cpp`: slot +0x1c renamed
  SetViewMatrix and lifted.
- `src/panzers/gameview_mapcut.cpp`: `PzViewSetScene` (callback +0x08).
- `src/panzers/gameview_view.cpp`: `PzCutsceneUpdateAnimation(Logic, Viewport)` (one line).
