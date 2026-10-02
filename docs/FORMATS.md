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
