# M6-RD: road edges zig-zag (snowy Allied mission)

## Bug

Playtest screenshot (Allied 8, about 0:17, Shermans, lattice power pylons): sawtooth strips on
the roads, folding along the terrain cell diagonals, where the narrow track road on the left meets
and ends in the wide snowy road. Reproduced with
`PZ_M5_MISSION="Allied 8" PZ_M5_AUTO=1 -nointro -m3` (Allied 9 and 10 are not the map:
Allied 9 is a snow map with telegraph poles, Allied 10 is green).

## Cause

`STerrain::UpdateRoad` (HD 0x6f95b0) was a reimplementation, not a lift. Two differences made the
strips:

- On-road test. Ours took the nearest sample interval of any control segment of the parcel. HD
  first picks, per grid vertex, the control segment whose two control-point planes hold the
  vertex and whose control chord is nearest (closer than Width), then flags the vertex only if one
  of that segment's sample intervals holds it and its chord is closer than Width / 2.
- U/V of the drawn vertices. Ours left every vertex outside all interval slabs at U = V = 0, so
  past a road's end (and on the outer side of bends) a cell went from U about 20, V 0.5 to U 0,
  V 0: the whole texture squeezed across one triangle, i.e. the zig-zag. HD: the nearest holding
  interval gives U from where the vertex's chord-parallel line meets the interval's two end
  planes, and before the first / past the last sample U continues with the distance to the end
  plane (V is still the chord distance / Width + 0.5), so the end caps fade out with the
  texture's alpha.

Also from HD: the whole-repeat scale rounds `length / Width` (not `/ TexLength`), the bounds use
the truncated sample min / max, vertices and indices are emitted x outer / z inner, the
vertex and index limits (0x51 / 0x180) panic as in HD, and the degenerate-intersection warning
"Unable to generate texture coords." is logged.

## Fix

`src/3dengine/pz/road.cpp`: `STerrain::UpdateRoad` is now `// PANZERS 0x6f95b0` (census lifted
2531 -> 2532, SWINE-shared 1316 and stubs 139 unchanged). Simplification kept: HD skips parcels
whose control segments are not Dirty and keeps their mesh (0x6f65d0 frees the untouched ones);
ours rebuilds every parcel the road covers, which gives the same meshes. `UpdateRoadJunction`
(0x6fd380) is still the old reimplementation (no artefact seen at junctions).

## Verified (our build; shots in `scratchpad\m6rd\shots\`)

| Map | Before | After |
| --- | --- | --- |
| Allied 8, ~1:20 | `a8_1.png`: the strips of the playtest shot | `a8fix1_0.png`: smooth road borders, the track road ends softly in the wide road |
| German 1 opening | `g1base_0.png` | `g1fix1_0.png`: dirt roads as before |

No "Unable to generate texture coords" / PANIC lines in the logs.
`regress.ps1`: menu 1181 / 0 mismatches, tc1 replay 3901 / 3901, Tutorial replay 2012 / 2012,
all three exits closed (render-only change).

## Reference wanted from the original

Allied 8 with `-nointro`, mission start, scroll to the starting Shermans near the big lattice
pylon (the playtest view at 0:17): one screenshot to confirm the track road's end cap into the
wide road looks the same.
