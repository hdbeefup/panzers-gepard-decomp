# M6 fix: selection frame of a vehicle not aligned with it

Branch `m6-fix-sel`. Census unchanged: 2531 lifted + 1316 SWINE-shared + 139 stubs (61 with a STUB_LOG).

**Cause.** The terrain effect decals (STerrain::RenderEffectDecals, HD 0x6f6640) turned their texture the
wrong way. HD builds the texture-coordinate transform with its 2x3 helper 0x661bb0 (row vectors, A applied
first) as T(-x, -z) * R * S(1/sx, -1/sz) * T(0.5, 0.5), with R = [cos(-r) -sin(-r); sin(-r) cos(-r)]
(checked in the disassembly at 0x6f7088..0x6f7266): u = dx cos(-r) + dz sin(-r),
v = -(dz cos(-r) - dx sin(-r)). The recompile used u = dx cos(-r) - dz sin(-r),
v = -(dx sin(-r) + dz cos(-r)), the mirrored rotation. With the unit heading (sin Dir, cos Dir) and the
decal angle Dir + pi/2 of SSingleUnit 0x5aaaa0, HD's mapping puts the frame's u axis along the heading;
the old one turned it by twice the angle, so the frame and the armour arcs lined up only for some
headings. Every rotated effect decal (SDecalEffect scorch marks with random rotation) had the same mirror.

**Fix.** src/3dengine/pz/decal.cpp, the two UV lines. Render only.

**Shots** (scratch `m5vx\shots\`): before `v4_0650.png` (tc1 replay 0:32, Panzer III selected), after
`sel1_0650.png`, `sel1_0700.png`, `sel1_0720.png`, crops `sel1_crops.png` (the tank turns between them).

**Regression:** menu 1181 / 0, tc1 3901 / 0, tutorial 2012 / 0, three clean exits (PZ_M5_NOGHOST=1).

**Reference for the coordinator:** a shot of the original with a selected tank at a diagonal heading
(Training Camp, select the Panzer III, order it to drive diagonally, shot while it drives) would confirm
the frame orientation; none of the saved shots has a selected vehicle.
