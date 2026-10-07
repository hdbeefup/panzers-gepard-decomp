# File formats

All addresses are in the 2016 HD `PANZERS.exe` (image base 0x400000), from
the Phase 0 Ghidra project. "SWINE" means the imported S.W.I.N.E. engine code
in `src/`.

## PAK archives (`*.pak`)

Code: `src/core/stream.cpp`, `SFileSystem::OpenArchive` (0x65f1e0),
`SFileSystem::LookupArchive` (0x65ee70), `SFileSystem::OpenRead` (0x65f420),
`SArchiveStream` (ctor 0x65cba0, vtable 0x80b554).

### Header (16 bytes, little-endian)

| Offset | Size | Value |
|---|---|---|
| 0x00 | 4 | 0x1B1A7253 (`"Sr\x1a\x1b"`). Anything else: `Not a Stormregion file` |
| 0x04 | 4 | 0x0A870A0D (`"\r\n\x87\n"`). Anything else: `Not a Stormregion file` |
| 0x08 | 4 | 0x4B434150 (`"PACK"`). Anything else: `Not a 'PACK' file` |
| 0x0C | 4 | tocSize |

The TOC (tocSize bytes) follows and is read whole into memory. File data
starts at `16 + tocSize`. Nothing follows the last member. There is no
compression and no encryption.

SWINE's generic `SStream::ReadSignature` also accepted an older first magic,
0x1A1A7253 (old-style strings). Panzers rejects it everywhere: in
`OpenArchive` and in the chunk-file signature check (0x65d6a0).

### TOC: a binary search tree, serialised in pre-order

| Offset | Size | Field |
|---|---|---|
| 0 | 1 | `prefix`: number of leading chars taken from the parent node's full name |
| 1 | 1 | `len`: length of the stored suffix |
| 2 | len | suffix, no NUL |
| 2+len | 4 | `pos`: data offset, relative to `16 + tocSize` |
| 6+len | 4 | `size` in bytes |
| 10+len | 1 | `hasLeft`: 1 if the left child (lexically smaller) follows at node + 15 + len |
| 11+len | 4 | `right`: TOC offset of the right child (lexically greater), 0 if none |

- The full name is `parent.name[0:prefix] + suffix`.
- Names are lower case and use `/`.
- Directories have their own nodes with size 0 (for example `menu`,
  `menu/fonts`).

### Lookup (`LookupArchive` 0x65ee70)

1. Copy the name into a 260-byte buffer, `_strlwr` it, and change `\` to `/`.
   Absolute names (`/x`, `\x`, `C:...`) panic with `Invalid parameters`.
2. Start at node 0. Compute `c = strncmp(name + prefix, suffix, len)`:
   - `c < 0`: go to the left child, or stop if there is none;
   - `c == 0` and `strlen(name) <= prefix + len`: found. The function returns
     a pointer to `{pos, size}`;
   - otherwise go to the right child, or stop if `right == 0`.

### Search order (`[Paths] Search`, `SetSearchPath` 0x65df90)

- `panzers.ini [Paths] Search` is split on `;`.
- Each element goes through `_fullpath`.
- A name ending in `.pak` (any case) is opened at once as an archive.
  `OpenArchive` panics if it is missing or invalid.
- Any other element is a directory, and gets a `/` appended.
- The game calls `SetSearchPath(search, false)`, which appends. With
  `prepend = true`, each element is inserted at index 0 instead.
- `[Paths] Home` goes through `SetHomePath` (0x65fa30): `_fullpath`, then `/`
  appended.

`OpenRead(name)` (0x65f420) looks in this order:

1. If the name is relative, walk the elements in list order. **The first
   element that has the file wins.**
   - Directory: open `dir + name` with `_sopen_s(_O_RDONLY|_O_BINARY,
     _SH_DENYNO)`. An error other than ENOENT is fatal if `panicstr` is set.
   - Archive: `LookupArchive`, then `fopen(pak, "rb")` and
     `SArchiveStream(file, 16 + tocSize + pos, size)`.
2. Then the loose file `Home + name` (`FileNameProcess` 0x65f020).
   Absolute names are used unchanged. On failure it panics with
   `%s: Couldn't open (%s): %s`.

`OpenWrite` (0x65f7a0) and `OpenAppend` (0x65f0a0) always write to
`Home + name`. Unlike SWINE, they do not create missing directories.

Verified on the shipped data with `paktest` (Search =
`nordic_en;panzers_patch_en;panzers_en;panzers_patch;panzers`):

- `menu.ini` is in three paks: 47978 / 47408 / 42778 bytes. The 47978-byte
  copy from `nordic_en.pak` is used.
- `menu/medals_eng_hq.tga` is in `panzers_patch.pak` (2265164 bytes) and
  `panzers.pak` (4194348 bytes). The patch copy is used.
- `missions.ini` is in `panzers_patch.pak` (27225 bytes) and `panzers.pak`
  (26702 bytes). The patch copy is used.

### In-memory layout (x86, `static_assert`ed in `stream.h`)

`SFileSystem` is the global at 0x92e330 and is 0x14 bytes:

| Offset | Field |
|---|---|
| +0x00 | `SSearchPathElement*` array |
| +0x04 | count |
| +0x08 | max |
| +0x0C | `SString Home` |

The HD SDArray order is `{array, size, maxsize}`, which differs from this
tree's `SDArray<T>`.

`SSearchPathElement` is 0x14 bytes:

| Offset | Field |
|---|---|
| +0x00 | type (0 = directory, 1 = pak) |
| +0x04 | `SString` name |
| +0x0C | tocSize |
| +0x10 | toc |

The streams:

| Class | Size | Fields | vtable |
|---|---|---|---|
| `SArchiveStream` | 0x2c | FILE* +0x1c, pos +0x20, size +0x24, offset +0x28 | 0x80b554 |
| `SFileStream` | 0x24 | fd +0x1c, readable +0x20, writeable +0x21 | 0x80b53c |

The HD stream vtables are `{deleting dtor, Read, ReadMax, Write, Seek}`. The
SWINE `AddRef`/`Release` interface is kept in `src/`.

## INI files (`SProperties`)

Code: `src/core/properties.cpp`.

| Function | Address |
|---|---|
| ctor | 0x65fe80 |
| `Load` | 0x6607d0 |
| `ParseFileData` | 0x660840 |
| `FindSection` | 0x6601c0 |
| `FindVariable` | 0x6602c0 |
| `GetInt` | 0x660500 |
| `GetFloat` | 0x660490 |
| `GetString` | 0x660570 |
| enumeration | 0x660150 / 0x660160 / 0x6601b0 |
| dtor | 0x660080 |

### Syntax

- If the file starts with a UTF-8 BOM, the rest of the file is decoded in
  place from UTF-8 to Latin-1. Only 2-byte sequences are handled:
  `(b0 << 6) + (b1 & 0x3f)`.
- `\r`, `\n`, space and tab between items are skipped.
- `;` at the start of an item comments out the rest of the line.
- `[name]` opens a section. Only spaces, tabs and `\r` may follow it on the
  line.
- `name = value`:
  - the name is right-trimmed;
  - the value runs from the first non-blank after `=` to `\r`/`\n`, and is
    right-trimmed;
  - quotes and `#` have no special meaning.
- Panics name the file:
  - duplicate section, duplicate variable ("Dublicate");
  - missing `]` or `=`;
  - empty section or variable name;
  - a variable before any section;
  - garbage after a section name.

### Storage and lookup

- Sections and variables are kept in arrays sorted by `_stricmp` and found by
  binary search, so lookups are case-insensitive. Enumeration returns sections
  in **sorted** order, not file order.
- `FindSection` first checks a cached index (+0x14). The enumeration cursor is
  the same field, so a `GetString(currentSection, ...)` inside an enumeration
  loop does not disturb it.
- `GetString` returns `""` for an empty value, and `def` only when the
  section or the variable is missing.

The x86 layout is `static_assert`ed in `properties.h`:

| Struct | Size | Fields |
|---|---|---|
| `SProperties` | 0x1c | sections array (+0x00), count (+0x04), max (+0x08), `SString` filename (+0x0C), current section (+0x14), current variable (+0x18) |
| `SPropertySection` | 0x14 | name, variable array |
| `SPropertyVariable` | 0x10 | name, value |

Differences from SWINE:

- SWINE used linked lists in file order.
- SWINE turned `#` in a value into a line end.
- SWINE only skipped the BOM, with no Latin-1 decoding.
- SWINE right-trimmed values only when asked to.
- SWINE called `setlocale(LC_NUMERIC, "C")` in `GetFloat`.

## `menu.ini` string table (gettext)

Code: `src/core/gettext.cpp`. `PrepareGetText` is at 0x660db0 and is called
from the `SSuperWindow` ctor 0x656d50 with `"menu.ini"`. `GetText` is at
0x660c50. The table is the global at 0x92e344, `{array, count, max}`.

```
[panzers/ChatRoom.cpp]
id_049 = "Attack"
str049 = "Assault"
```

- There is one section per source file.
- `id_NNN` is the English source string and `strNNN` is the text to show.
- NNN counts up from 000. The section ends at the first missing `id_NNN`.
- A missing `strNNN` defaults to `""`, and so ends the section.
- The section also ends silently at the first id or str that is shorter than
  3 chars or not wrapped in `"`.
- Escapes: `\\`, `\"` and `\n`. Any other escape is fatal in an id
  (`prepare_gettext(%s): [%s]/%d: Invalid escape sequence`). In a str the
  backslash is kept.
- `GetText(section, id)` does a case-sensitive `strcmp` on both the section
  and the id, and searches only the first section with that name. If nothing
  matches it returns the `id` pointer itself.
- Call sites pass the source file and the English text, for example
  `GetText("panzers/SkirmishChatRoom.cpp", "Skirmish")` at 0x653284.

The shipped `nordic_en.pak` `menu.ini` has 18 sections and 757 entries. In 40
of them the str differs from the id.

## Effect files (`*.fx`)

Code: `src/core/propertystruct.*` (typed property trees, HD
0x661c70..0x669d60), `src/3dengine/pz/pzpixie.*`, `effect.*` and
`particles.cpp`. The schema is in `effectschema.cpp`, generated from the
SPixie ctor 0x6941e0.

An `.fx` file is an INI file (see `SProperties` above) with two sections:

| Section | Content |
|---|---|
| `[Version]` | `TimeStamp = 2003.12.14.12:47:18`. With the check flag, `LoadEffectPrototype` panics when the stamp is later than the exe build date ("ViewEffects.exe is too old"). |
| `[ExtendedFx2]` | the effect, as a typed property tree |

`SPixie::LoadEffectPrototype` 0x69dc80 creates an instance of the schema
`Array "Effects"` and loads it from `[ExtendedFx2]`:

- A node's key is its parent's key, a dot, then its own name
  (`Effects.0.Data.EffectType.00_PARTICLES.Birth.LifeTime`). Names may
  contain spaces and dots (`ZBuffer On`, `Max. Y`).
- **Array**: the key holds the element count, and element `i` is named
  `<i>.<element name>` (`Effects = 3`, `Effects.2.Data...`). The count is
  not clamped.
- **Multi**: the key holds the selected alternative (`EffectType = 0`). All
  alternatives are loaded under `<key>.<alternative name>`.
- **Struct**: has no key of its own.
- **Int / Float**: clamped to the schema range. **Color**: `0xRRGGBB`
  (strtol base 0), clamped to 0..0xffffff. **Bool**: int != 0. **Enum**:
  a value that is not one of the items keeps the default.
- **Track** (a piecewise-linear curve over 0..1 of the particle life):
  `<key>.Options.Loop`, `<key>.Options.Max. Y` (editor only), `<key>.Keys`
  = count, `<key>.Keys.<i>.Key.X` / `.Y`.
- A missing key takes the schema default. The files written by the effect
  editor list every key.

Each `Effects.<i>.Data` struct is one sub-effect:

| Key | Meaning |
|---|---|
| `Enabled` | Editor switch only. **The game never reads it**: the menu fire has all three entries at 0 and they still play. |
| `Name` | editor label |
| `PriorityLayer` | 0..2, the drawing pass (`SPixie::Render` draws layer 0, then 1, then 2) |
| `ForceUpdate` | update even outside the visible terrain |
| `EffectType` | 0 `00_PARTICLES`, 1 `01_FLARE`, 2 `02_RAIN`, 3 `03_SNOWFALL`, 4 `04_DECALEFFECT`, 5 `05_SOUNDEFFECT`, 6 `06_ATMOSPHERE`, 7 `07_LITE`, 8 `08_TRAIL`, 9 `09_SHOCKWAVE`, 10 `10_CAMERASHAKE`, 11 `11_SANDSTORM` (the cases of `SPixie::InitEffectPrototype` 0x69d6a0) |

`00_PARTICLES` (`SPParticles::Init` 0x6e5eb0):

| Group | Keys |
|---|---|
| `Draw` (multi) | 0 `Particles`: `ParticleType` (0 Normal = screen-space quads, 1 Billboard = vertical quads facing the camera yaw, 2 Cloud = horizontal quads, 3 Trail), `TrailLength`, `TrailOrientation`, `BlendType` (0 alpha blend from the texture alpha, 1 additive), `ElevDependent`, `ZBuffer On`, `Color`, `Random rotate`, `Texture Anim in` (0 `More files`: `effects\media\<Texture>NN[_a].tga`; 1 `One file`: `Texture`, a horizontal strip of `TotalAnimFrames` frames, of which `FirstAnimFrame`..`LastAnimFrame` are used), `LightingModel`, `LightColor1/2`, `OverLight1`, `LightRange`, `LightIntensity` (track), `LightDuration`, `FixedLightSource`. 1 `Object` (`Mesh file`). 2 `Effect` (`FX file`, one sub-effect per particle). 3 `Trail`. |
| `Tracks` | `Alpha`, `Size` (track value * 0.5 = half size in metres), `Size_Rnd`, `AdditionalSpeed` (multiplies the birth velocity) |
| `Birth` | `Style` (0 Centralized, 1 Along the edge + `Quantity`, 2 From Basement), `BirthSpeed` (particles/s), `Duration` (0 = endless), `LifeTime`, `LifeTime_Rnd`, `Sphere`, `Radius` (a disc of this radius, denser in the middle; 0 = a box of `RandomX/Y/Z`), `VSpeed(_Rnd)` along the effect direction, `HSpeed(_Rnd)` outward, `FromTerrain`, `FromWater`, `Altitude`, `CollisionType` (None, Slide, Disappear, Jump), `CollisionSticking`, `GrowTime`, `GrowSpeed`, `BirthEnabledTime(RND)` / `BirthDisabledTime(RND)` (on/off cycles), `Birth in rain`, `Balanced birth`, `Sampling rate` |
| `Move` (multi) | 0 `Shot`: `VariationType` (0 a random frame per particle, 1 animate over the life), `Gravity` (negative rises), `AirResistance`, `AirResistance2`, `Turbulence`, `NoWind`, `Object Spin` + X/Y/Z spin minimum/random (deg/s). 1 `Waste`. |
| `General` | `ManageType`, `Linked` (particles follow the emitter), `PivotDir` (the birth basis follows the direction), `TrailDirectionUpdate`, `Bullet Indicator`, `Wait(_Rnd)` (start delay), `Version` (3 is rejected), `ID for Debug` |

Textures are looked up as `effects\media\<Texture>`. For the Snowy and
Foggy scene atmospheres, the suffix `_snowy` / `_foggy` goes before `_a`
(or before the extension), falling back to the plain name.

The menu map (`maps/menu.map`, chunk `EEFS`) places three effects on the
Panther wreck:

- `effects/smoke/Ground_Dark_Slow_Size3.fx` and `Ground_Dark_Fast_Size3.fx`:
  one Normal particle entry each (`smoke_a.tga`, 2 frames, colour 0x202020,
  alpha blend, 3 particles/s, life 5 s / 2.5 s, rising at 0.6 / 1.3 m/s).
- `effects/fire/Fire_From_House_Size2.fx`: three entries. Cloud and
  Billboard additive flames (`tuz_v01.tga` / `tuz_v12.tga`, 32-frame strips
  animated over frames 0..17, rising with gravity -1.5), and a smoke column
  (`smoke_a.tga`, colour 0x545454). All three have `Enabled = 0`.

The fire sits 0.7 m above the ground, inside the hull. This is probably why
the original menu shows only smoke there (not verified with the models).

## Save games (`SaveGames/*.save`) and replays (`Replays/*.rec`)

Agent F, M3. Read from HD `PANZERS.exe` (SPanzersCampaign::SaveGame 0x5966a0, SGameLogic
0x57e110, SUnit::Save 0x5be320, WriteReplayHeader 0x596fc0) and checked against the original's own
`SaveGames/TRNG-Start.save` (2,572,085 bytes) and `tc1.rec` (scratch `m3f\tools\savedump.py`
parses both). Little-endian; strings are u16 length + bytes (no terminator); a chunk is a 4-byte tag,
a u32 payload size, the payload (SStream::WriteChunkStart / End).

### Stream signature and chunks

`53 72 1a 1b 0d 0a 87 0a` (WriteSignature), then one `SAVE` chunk.

### Variable lists (gSaveVariables 0x670c50 / gLoadVariables 0x670440)

Self-describing: per descriptor entry `int type, string name, value`, then `int 0`. Values (0x670af0):
1 byte, 2 int, 3 float, 4 3 floats, 5 string, 6 a `_mcl` chunk holding a member variable list,
8 2 floats, 12 u16; 10 `int elemType, int count, elements`; 11 (dequeue) `int elemType, int count,
int first, int last, elements`. The army records (SUnitDef, descriptor 0x8ddb48) are such lists:
ClassName, Player, XP, Pos, Yrel, Dir, HP, Ammo, ScriptID, AIGroup, the four armours, Behavior,
GlobalState_ActiveState, Slots[0..1], StoredUnits (type 10 of members), Cargo, the First* flags,
TowedUnits, StoredSpecial.

### Save game (`SAVE` payload, version 1)

| Field | Source |
|---|---|
| `'v4pa'`, int 1 | |
| string map, string mission code, string title | GetMapName 0x592040, [section] "Mission code" ("TRNG"), "Start - maps/training.map" |
| int GameMode, Race, StartPrestige | campaign +0x10, +0x18, +0x28 |
| int n, n army records | Army +0x2c |
| int n, n army records | MissionArmy +0x3c |
| int Prestige, string section, int +0xe4 (result), Difficulty, +0xf8, +0xfc, +0xb84 | |
| int n, n x (u8 +4, int +0) | objectives +0x134 |
| 12 x (12 x 4 ints, int, int) | campaign player records +0x140: for i 0..11 {+0x0c, +0x3c, +0x6c, +0x9c}[i], then +0xcc, +0xd4 (unit counters per category 0x56d6d0, counted by SWorld::CreateUnit) |
| int score | +0xb60 |
| game state 0x57e110 | 19 chunks: `PLY3` `AIGP` `UNIS` `EEFS` `CAM ` (20 bytes 0x5e6a70) `LOCS` `TRIG` (when World+0x7478) `RTRG` `TVAR` `ECHO` `CNTR` (+0x14c, +0x14d, +0x174, +0x178 as ints) `VARS` (logic variables, 0x8dba58) `SEED` (World+0x7518) `ODDD` `WIR3` `MGRP` `MGRP` `AMOD` `WTHR` `OBJT` |

`UNIS` (0x5fb630): int heap size; per slot int live (0/1); a live one is a `UNIT` chunk: int
`'v100'`, string class (prototype +0x60), then SUnit::Save 0x5be320: `vars` {the unit's class
descriptor list, via vtbl +0x1c}, `gunn` {int n, each gunner's list 0x8dbaf8}, `driv` {int n, each
driver's list, via vtbl +0x50}, and `targ` when any target is set: int n, n STarget lists (0x8dd890),
int gunner count, int driver count, then the index (-1 = none) of unit +0x1f8, +0x1f4, per gunner
+0x14 / +0x18, per driver +0xc0 / +0xc4 / +0xc8 (each target listed once, in that order).
In the reference the units take 2,564,826 of the 2.5 MB.

The descriptor lists (32 tables, 429 entries, `src/game/savedesc.cpp`, read from HD .data, entry
{name, type, offset, members, element size, element type}): SUnit 0x8dc540 (Special, GlobalState,
Commands, NearbyUnits*, AI_InvalidTargetUnits, StoredUnits, StoredMembers, GhostFrames as type 11,
...). A subclass list starts with `SUnit` (or `SSingleUnit`) as a `_mcl` member at offset 0:
SSingleUnit 0x8dc358, SFlyingUnit 0x8daf48, SBuildingUnit 0x8da7f8, SProjectileUnit 0x8dc250,
SWasterUnit 0x8dddf0, STrainUnit 0x8dc388, SPanzersSquadUnit 0x8dc0b0, SPanzersSquadMemberUnit
0x8dbff0. Drivers: SDriver 0x8dacf0 (every driver class but these), SFlyingDriver 0x8daa20,
SProjectileDriver 0x8daa98, SPanzersSquadMemberDriver 0x8daae0. Logic `VARS` 0x8dba58 (PlaySpeed,
FrameCount, StartAnim); AIGP 0x8dde98.

The other chunks of 0x57e110 (W = SWorld, L = SGameLogic; a heap is int size, int free head, int
count, then per slot int Next and, when it is 0x7fffffff, the element):

| Chunk | Writer | Content |
|---|---|---|
| `PLY3` | 0x5fb160 | 12 x 9 ints: W+0x170 + i*0x48, dwords 0..3 and 11..15 |
| `AIGP` | 0x57db20 | heap W+0x4f4 (0x54): the group's variable list 0x8dde98 |
| `EEFS` | 0x5f9450 | heap W+0x73d0 (0x2c): an `EFFE` chunk {'v100', name, 5 floats} |
| `CAM ` | 0x5e6a70 | W +0x38, +0x40, +0x44, +0x50, +0x54 (20 raw bytes) |
| `LOCS` | 0x5f9210 | heap W+0x7480 (0x28): x1, z1, x2, z2, name, color |
| `TRIG` | 0x5f8a80 | only when W+0x7478 != 0: int n; per trigger flags, name, event (0x5b2750), conditions (0x5b2670), actions (0x5b24e0) with the masks of 0x5b1f30 / 0x5b1d50, as in the map's TRIG |
| `RTRG` | 0x57d9c0 | L+0x268 (0x34): 5 ints, 3 floats, 2 ints, then the found units {int flag, float, int unit} |
| `TVAR` | 0x57dd60 | heap W+0x74a8 (0x14): name, value, step |
| `ECHO` | 0x57e4f0 | int n (L+0x84), the board names of the echo texts |
| `CNTR` | | L +0x14c, +0x14d (bytes), +0x174, +0x178 as ints |
| `VARS` | gSaveVariables | L with 0x8dba58 |
| `SEED` | | W+0x7518 |
| `ODDD` | 0x5f8cf0 | heap W+0x158 (0x20): a `DODD` chunk {2 ints, 4 floats} |
| `WIR3` | 0x5fb7d0(s, 1) | heaps W+0x742c (0x2c), W+0x7440 (0x24), W+0x7454 (0x44); the map's WIR3 has the first two only (the editor writes with flag 0) |
| `MGRP` x2 | 0x57db90 | heap L+0x2dc (0x28): members {int, 3 floats}, float, 3 ints, float, 2 bytes; HD writes the same chunk twice |
| `AMOD` | 0x57d8c0 | L+0x2fc (0x24): model name, 4 floats, float +0x1c |
| `WTHR` | 0x5fb770 | 2 x 19 floats (W+0x550, +0x59c, order of 0x5fa830), float +0x548, +0x54c, int +0x544 |
| `OBJT` | | int n, n x {float, float, int +0xc, int +0x10} (L+0x194, the minimap markers) |

LoadGameState 0x56eb50 reads the chunks in any order (an unknown tag is logged and skipped). It
unfixes the bridges and removes every unit first; the UNIS reader 0x5f3820 allocates each slot again
in order, makes the unit of its class (prototype +0x10) and runs SUnit::Load 0x5bbd30 (the gunner and
driver counts must match), frees the empty slots, then calls every unit's vtbl +0x14 (InitAfterLoad
0x5bb1c0) and then +0x18 (LinkAfterLoad 0x5baf30). The load-game LoadMap 0x61f840 fixes the bridges
(0x5e65f0) only after that. Load Game file names are relative to `SaveGames/` (F6 / F9 use
`quick.save`); the Load Game list (LoadSavedGameNames 0x595fa0) also accepts the versions `v2ps` and
`v1ps`.

**Status of our save** (M4 S): the Training Camp mission-start autosave (tc1.rec start, German,
Panzer III F + Riflemen) is 2,572,085 bytes like the original's and differs from it in 184 bytes
only: the `VoiceVar` (unit +0xdc) of 184 units. Both games draw it with the CRT `rand()`, which
start-up seeds with the time (`srand(time)`), so it differs between any two runs of the original as
well. WIR3 is written from the map's raw chunk plus an empty third heap (the recompile does not create
wires yet).

### Replay (`-packetrec`)

u8 3, the signature, a `SAVE` chunk (WriteReplayHeader 0x596fc0): `'v4pa'`, int 3, string map, int
GameMode, Race, StartPrestige, int n + n army records (MissionArmy +0x3c), int Prestige, string
section, int +0xe4, Difficulty, +0xf8, 12 x 5 ints (World players +0x19c..+0x1ac, the support calls).
Then per logic frame and player the packet frames (docs/M3_REPLAY.md, packets.h). HD's StartReplay
0x597510 reads this header without the version dword (see docs/M3_REPLAY.md).
