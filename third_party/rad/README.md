# third_party/rad: Miles and Bink import declarations

PANZERS.exe (HD, 2016) links two RAD Game Tools DLLs dynamically:

| DLL | Version shipped | Imports used by PANZERS.exe |
|---|---|---|
| `mss32.dll` | Miles Sound System 6.5c | 52 `AIL_*` functions |
| `binkw32.dll` | Bink 1.5q (the exe logs "1.6g", the SDK it was built with) | 15 `Bink*` functions |

No RAD SDK was used or downloaded. Everything in this folder was written by
hand:

- `mss.h` and `binkw32.h` declare only the functions PANZERS.exe imports.
  - The names and the stdcall argument sizes come from the decorated IAT
    names (for example `_AIL_open_digital_driver@16`).
  - The argument meanings come from the PANZERS.exe call sites:
    SMilesConcert at 0x684300..0x6876ff, and the SSuperWindow Bink code at
    0x657ef0, 0x65ba00 and 0x65b830.
  - The `BINK` and `BINKBUFFER` structs declare only the leading fields
    PANZERS.exe reads.
- `mss32.def` and `binkw32.def` list the same names. They were checked
  against `dumpbin /exports` of the shipped DLLs.
  - `CMakeLists.txt` turns them into `mss32.lib` and `binkw32.lib` at build
    time with `lib /def:... /machine:x86`.
  - It exposes the INTERFACE targets `rad_mss32` and `rad_binkw32`.

The DLLs are never committed. At runtime they come from the game install or
from a test copy of it.

## Why the names start with an underscore

Both DLLs export the decorated names with their leading underscore, for
example `_AIL_startup@0`.

- For an x86 `.def` entry `_AIL_startup@0`, `lib` produces the public symbol
  `__AIL_startup@0`, with an import name that resolves to `_AIL_startup@0`
  (name type "no prefix").
- A C function declared `__stdcall _AIL_startup(void)` gets exactly that
  symbol.

So the headers declare `_AIL_x` / `_BinkX` and `#define` the SDK spelling
(`AIL_x` / `BinkX`) onto them. The resulting import table names match the
original exe's (`MSS32.DLL!_AIL_startup@0`).

## Delay loading

The original imports both DLLs directly. By default they are delay-loaded
here (`PANZERS_RAD_DELAYLOAD=ON`), so a build-tree `panzers.exe` without the
game DLLs next to it still starts. Configure with
`-DPANZERS_RAD_DELAYLOAD=OFF` for plain imports.
