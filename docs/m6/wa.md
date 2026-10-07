# M6-WA: lakes and rivers (water) - plan and HD findings, not lifted yet

Branch `m6-wa` (from m6-int 8059808). The session deadline moved to 23:05, so no code was
changed: this file records what the HD decompile shows, so the next agent can lift it directly.
Census unchanged (no code). Nothing here was run or verified.

## Call graph (HD PANZERS.exe)

- `SWorld::LoadMap 0x5f1990` -> `UpdateWaterMap 0x608600` (also called from 0x6201c0, 0x61f840,
  0x624770). The recompile's `SWorld::UpdateWaterMap` (mapload.cpp) only does the memcpy and the
  0x600 dirty mark; HD in between:
  1. every live lake (world +0x73b0 heap, 0x38 each) gets +0x34 = 1, then `0x607ad0`;
  2. every live river (world +0x73e8 heap, 0xa0 each) with `(+0x98 & 0x10) == 0`:
     `0x6015d0(rec, rec+0x98 flags, world+0x73e4)`; then the same for the ones with `& 0x10`;
  3. the 0x600 dirty mark of the whole map (+0x7500..+0x7514, as the recompile has).
- `0x6015d0` is also called from StartEffects 0x5f5b50, StopAmbientSounds 0x5f5140, 0x607d60
  (downstream chain), 0x5e27a0.
- `0x607ad0` is also called from 0x6201c0, 0x58a610, 0x61f840, 0x624770 (editor / script paths).
- Terrain water-height writer `0x6f8b90(terrain, x, z, h)`: `if 0<=x<T+0x50 && 0<=z<T+0x54:
  T[+0x8c][(T[+0x64]*z + x)] = h` (the +0x8c buffer = world `WaterHeights`). Callers: CreateLake
  0x6a7940, DestroyLake 0x6aa3f0, **and 0x6bb260 / 0x6bc030** (4 calls each; not yet identified,
  likely the river carve, so rivers probably write water heights too - check before deciding
  rivers are render-only).

## LAKS loader 0x5f05c0 (world +0x73b0 SHeap, 0x38 per element)

Heap clear 0x5dd580; count (<= 0x1000000, else throw "Invalid array size"), realloc to count * 0x38,
memset; +0xc = free head, +0x10 = live count; per element: next int, if 0x7fffffff -> lake chunk
0x5f11d0 on (element + 4):
- chunk tag "LAKE" (0x454b414c) else throw "Expected lake chunk"; version "v100" / "v101" else throw
  "Unsupported lake version".
- rec+4 name SString (0x56e7d0), +0xc x (float), +0x10 depth (float, added to terrain height),
  +0x14 z (float), +0x1c int, +0x20 int (sparkle flag);
  v100: +0x24 = 1, +0x18 = 0.2f (0x3e4ccccd), warning "Lake version is obsolete, resave this map file.";
  v101: +0x24 int, +0x18 float.
- +0x28 = Gepard +0x44 LoadTexture(name or "", 1, 1); +0x2c = -1 (scene lake); +0x34 = 1 (dirty);
  chunk end 0x65d460.

## SWorld 0x607ad0 (rebuild dirty lakes)

For each live lake with +0x34 set: bounds (x0,z0,x1,z1) = (TerrainW +0xdc, TerrainH +0xe0, 0, 0);
if +0x2c >= 0 -> scene +0xac (0x6abcb0) gets the old bounds. Level = `0x5e7730(x, z)` (terrain
height) + depth; scene +0xa0 DestroyLake(+0x2c); +0x2c = scene +0x9c
`CreateLake(tex +0x28, x, level, z, +0x1c, +0x20, +0x24, +0x18)`; +0x34 = 0; if scene +0xac gives the
new bounds: block map dirty `0x5ef380(min(x0)*4-4, min(z0)*4-4, max(x1)*4+4, max(z1)*4+4, 0x600)`.

## SScene lakes (scene +0x20c SHeap, 0x40 per element; +0x218 free head, +0x21c count)

Element: +0 next, +4 texture (0x677f20(arg tex) - texture addref), +8 sparkle texture
(Gepard +0x44 "water/fx/sparkle.tga" if sparkle flag else -1), +0xc level, +0x10 colour,
+0x14/+0x18/+0x1c SDArray<block 0x20>, +0x20 arg6, +0x24 sparkle flag, +0x28 arg8, +0x2c arg9,
+0x30 minX (init W-1), +0x34 minZ (init H-1), +0x38 maxX (0), +0x3c maxZ (0).

**CreateLake 0x6a7940** (heap alloc 0x6a0e80, element ptr 0x6a08a0):
- flood fill from (round(x), round(z)) over terrain vertices (scene +0x1b8 W, +0x1bc H), 4-neighbour,
  ring queue of 0x800 (x,z) at 0x92f180, at most 50000 steps, visited map int[(W+1)*(H+1)];
  a neighbour is taken when `0x6f4bc0(nx, nz) <= level` (terrain vertex height); only vertices with
  0 < x < W, 0 < z < H expand; full queue -> panic "SScene::CreateLake: Buffer overrun".
  Each expanded vertex widens min/max X/Z (+0x30..+0x3c).
- colour = rgb of clamp(1, scene +0xd8.. - scene +0xe8.. * scene +0x114) * 255 (constants
  0x7f1b58 = 1, 0x7fb6c8).
- per terrain parcel (scene +0x1c0 x +0x1c4 parcels of 8x8): cell flags 10x10 (0x933180) where any
  corner vertex is in the fill; if any: vertex index table 9x9 (0x9331e8), **every used vertex gets
  `0x6f8b90(x, z, level)` = world WaterHeights = level** (game state), index buffer of
  cells*6 shorts, vertex buffer of 0x18 / 0x20 (sparkle) / 0x28 (sparkle and Gepard +0x758 != 0)
  bytes per vertex: pos (x, level, z), diffuse = colour | alpha, alpha 0 above water, 0xc0 when
  depth >= DAT_7f4588, else ftol(depth * k) << 24 (0x766810: read the disassembly for k). Block
  record {parcel, nverts, vb18, vb20, vb28, ntris*2?, -, ib} appended to +0x14.
- frees the visited map; returns the lake index.

**DestroyLake 0x6aa3f0**: for x in [minX, maxX), z in [minZ, maxZ) (exclusive maxima - HD keeps the
last row / column as water; keep that): water height = terrain height (0x6f4bc0 -> 0x6f8b90);
release both textures (Gepard +0x48); free every block's buffers; heap remove.

**Slot_AC 0x6abcb0** = GetLakeBounds(lake, &x0, &x1, &z0, &z1) -> 1 (minX +0x30, maxX +0x38,
minZ +0x34, maxZ +0x3c), 0 if the index is not live. Note world 0x607ad0 passes
(&x0, &z0, &x1, &z1) in the order (+0x30, +0x38, +0x34, +0x3c) - check the argument order in the
disassembly before mapping.

**DrawLakes 0x6ad740**: 10 KB, not read yet (decompile in `scratchpad\m6wa\P0\lakes1.c`).

## Rivers (larger than the lakes)

- RVR2 loader 0x5f0960 (world +0x73e8 heap, 0xa0 each, clear 0x5dd950, element chunk 0x5f1480).
- 0x6015d0 builds a river: guard +0x98 (byte) re-entry, spline tangents 0x5debd0 (long; links up /
  downstream rivers through 0x5d65f0 and 0x607d60), when flags & 6: scene +0xb4 remove the old one,
  three textures (Gepard +0x44), precache "water/fx/waterfall_hq" (0x52c320), the points (0x14 ->
  0x10 each), scene +0xb0 Slot_B0 0x6a90e0 (16 args) -> +0x20, scene +0xb8 (0x6ac0b0) -> +0x90;
  propagates width / level to the connected rivers.
- Slot_B0 0x6a90e0: scene +0x220 heap, 0x78 each; optional effect (pixie +0x38, 10.0f) at the
  first point; scene +0x234 = 0x6787a0(0x242) once.
- DrawRivers 0x6b0920: 6170 instructions, not read.

## Recommended order for the next attempt

1. LAKS loader + 0x5f11d0, 0x607ad0, the lake loop of 0x608600, CreateLake / DestroyLake / Slot_AC
   (game state: WaterHeights). Run regress.ps1 at once: the Tutorial has 2 lakes, and its reference
   was recorded from the original, so a correct lift must keep 2012 / 2012.
2. Identify 0x6bb260 / 0x6bc030 (the other WaterHeights writers) before treating rivers as
   render-only.
3. DrawLakes 0x6ad740, then rivers.

Decompiles kept in `scratchpad\m6wa\P0\lakes1.c` (0x5f05c0 0x5f11d0 0x607ad0 0x608600 0x6a7940
0x6aa3f0 0x6f8b90 0x6ad740 0x5f0960 0x6015d0 0x6a90e0 0x6abcb0) and `lakes2.c` (0x5debd0 0x607d60
0x5f1480 0x6a0e80 0x6a08a0 0x677f20 0x5e7730 0x6f4bc0).
