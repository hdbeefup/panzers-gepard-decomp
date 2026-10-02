# Codename Panzers Phase One: Phase 0 triage

Reference binary: `D:\Games\Codename Panzers - Phase 1\PANZERS.exe` (2016 Nordic/GOG HD). I worked on a scratch copy at `phase0\PANZERS.exe`. The install was only read, never written.

- PE timestamp: Thu Feb 18 01:31:43 2016.
- Image base 0x400000, entry 0x767a1c.
- PDB path: `D:\NordicGames\PROJECTS\Panzers\PhaseOne\trunk\code\bin\panzers.pdb` (string at 0x8bc104). The PDB itself is not available.

| Section | VA | VSize |
|---|---|---|
| .text | 0x401000 | 0x3e8307 |
| .rdata | 0x7ea000 | 0xef148 |
| .data | 0x8da000 | 0x87ac1 |
| .tls / .gfids / _RDATA / .rsrc / .reloc | 0x962000 .. 0x967000 | |

The exe **exports 27 mangled C++ symbols**, which is a free symbol source:

- SSuperWindow::Initialize at 0x657910
- SSettings::CHECKCDKEY at 0x64d390
- SGameSpy::Connect/Init/... in 0x52cfa0..0x530c80
- SPanzersCampaign::LoadGame/SaveGame in 0x594e00..0x596b30
- SUnitRegistry::LoadUnitFiles at 0x5d1050
- SVersion::GetVersionString at 0x65c070
- and others. All 27 are in the CSV.

Ghidra project: **`phase0\ghidra\panzers.gpr`**, program `PANZERS.exe`.

- Headless import with full auto-analysis took 298 s and found 19,985 functions. It recovered RTTI and the exports.
- Ghidra 12 is at `C:\Users\swine\scoop\apps\ghidra\current` (GHIDRA_INSTALL_DIR). pyghidra 2.2.0 is installed.
- Helper scripts are in `phase0\scripts\`: `dec.py`, `callers.py`, `symbols.py`, `entrycheck.py`, `pe.py`, `pakparse.py`.
- Run pyghidra scripts from `scripts\`. If you run them from `phase0\`, the local `ghidra\` folder shadows the `ghidra` Java package and the import fails with a RecursionError.

## 1. Does the export match the HD exe? DIFFERENT build

`phase1-raw-ghidra-export/test.c` was **not** made from this PANZERS.exe. It is also not PANZERS_MOD.exe, editor.exe or Phase 2.

| Evidence | Export (test.c) | HD PANZERS.exe |
|---|---|---|
| IAT `PTR__BinkOpenMiles@4_006c53b0` | 0x6c53b0 | `_BinkOpenMiles@4` IAT slot is **0x7ea43c**. 0x6c53b0 is in the middle of .text (bytes `8b45105f5e89085dc20c00`). |
| `s_Unsupported_doodad_version_006d52b0` | 0x6d52b0 | String is at **0x8004bc** (.rdata). 0x6d52b0 is .text code. |
| `s_Unsupported_unit_version_006d4c7c` | 0x6d4c7c | String is at 0x8007c4. |
| Implied layout | .text ends around 0x6c5000. .rdata is about 0x6c5000-0x6ef000. .data is about 0x74xxxx-0x79xxxx. | .rdata starts at 0x7ea000. |
| FUN_ addresses (6,051) | 0x401010..0x790910 | Only 186 (3%) land on an HD function entry, which is chance level. Spot checks: 0x401010 is in no function (bytes `8d8d2cfdffff...`, mid-instruction). 0x449b40 is inside FUN_00449a97. 0x53bcf3 is inside FUN_0053bc90. 0x68df54 is inside FUN_0068dda0. 0x790910 is inside the CRT's `assemble_floating_point_value_from_big_integer`. None is an entry. |
| CRT | `entry()` calls `__heap_init`, `__mtinit`, `__RTC_Initialize`, `__ioinit`, `__setargv`. That is the **VC7.1 (VS.NET 2003)** CRT. | VS2015 UCRT: `entry` → `___security_init_cookie` → `__scrt_common_main` (0x767874). |
| Content | No RakNet, no RankedGaming, no `[BINK] Starting Bink version`. Uses its own WinSock2 networking. Has Direct3DCreate9 and static D3DX9. | RakNet, RankedGaming client v5.0 and GameSpy are all present. |

**Verdict: DIFFERENT.** The export comes from a 2004-era Stormregion retail build: VC7.1, D3D9, GameSpy, no RakNet. The overall code is clearly the same engine with the same function names and log strings. For example, SFileSystem::OpenArchive and LookupArchive are near-identical (see section 5). I could not tell which retail patch level (1.0 or 1.0x) it is, because no version string survived in the export and no 2004 exe is on disk. A disk search of D:\, N:\ and Downloads found only the HD PANZERS.exe, PANZERS_MOD.exe, Phase 2 and PANZERSUnpacker.exe.

**Consequence:** none of the export's addresses can be used against the HD exe. Use the export only as a naming and structure guide (struct layouts in test.h, function names). Treat the new Ghidra project as ground truth.

## 2. Third-party libraries

| Library | How linked | Version / evidence |
|---|---|---|
| CRT | static | VS2015. Linker 14.0, Rich header builds 23406/23506, which is VS2015 Update 1 (14.0.23506). Uses `__security_init_cookie` and `__scrt_common_main`. The Rich header also lists objects from VS2013 (40116), VS2008 SP1 (30729) and VC7.x (9178/9210/8444/3077), most likely from prebuilt static libs (D3DX9, GameSpy and others). |
| D3D9 | dynamic | `d3d9.dll!Direct3DCreate9` is called from SGepard::Initialize 0x67d1c0. Its log strings still say `Direct3DCreate8 failed` and `IDirect3DIndexBuffer8::Lock`, left over from the D3D8 port. |
| **D3DX9** | **static** | `Microsoft (R) D3DX9 Shader Compiler 4.09.00.1126` (0x892470) and `Shader Assembler 4.09.00.1126` (0x8819c4). Also D3DXERR_* tables, `D3DX: (WARN)`, `DisableD3DXPSGP`, `D3DXValidMesh`, and d3dxof.dll loaded at runtime. This matches the static d3dx9.lib from the DirectX 9.0c SDK of about 2004 (Summer 2004 was the last SDK with a static d3dx9). The exact SDK release is my inference, not verified. |
| Bink | dynamic, binkw32.dll | 15 imports, including BinkOpenMiles. The install's binkw32.dll is **1.5q**. The exe logs the version at runtime: `[BINK] Starting Bink version %s`. |
| Miles Sound System | dynamic, mss32.dll | 52 AIL imports. The install's mss32.dll is **6.5c**. Provider .m3d/.asi files are in `miles\`. SMilesConcert::SMilesConcert is at 0x684300. |
| DirectSound | dynamic | 3 imports, used by SStreamingPlayer. |
| GameSpy SDK | static | Peer/chat (`peerchat.gamespy.com`), GP (`gpiInitialize`), natneg, `GameSpyHTTP/1.0`, `gamestats.gamespy.com/cnpanzers/`. Wrapped by SGameSpy (src `network/SGameSpy.cpp`). No version string. |
| **RakNet** | static, HD addition | `..\..\RakNet\Source\*.cpp` and `d:\nordicgames\...\code\raknet\source\*.h`. Classes: RakPeer, NatPunchthroughClient, FileListTransfer, PacketLogger and others (28 RTTI types). No version string. |
| RankedGaming client | static, HD addition | `[RGS]`, RGMasterServer, RGListener, `[ Version 5.0 ] PROTOCOL ERROR`, `RECV COMPRESSED PACKED, BUT LIBRARY COMPILED WITHOUT ZLIB SUPPORT`. That last string means **zlib is not linked**. |
| zlib / zstd / lz4 / lzo | **none** | No inflate/deflate messages and no zlib distance tables. The CRC32 table at 0x80b120 sits next to the file-system strings and belongs to the engine's own code. |
| png / jpeg | **none** | No libpng or IJG strings. The engine uses its own TGA/DDS/DXT loaders (SBitmap, SGepard::CreateTextureFromDXT). |
| WinSock2, IPHLPAPI, NETAPI32 | dynamic | Networking and MAC/adapter lookup. |

Other oddities:

- About 362 strings like `system32/wnwis` and `system32/dgckkqqq.ini` sit at 0x8971c4.. (RankedGaming region). They look like obfuscated decoy or anti-tamper paths. I did not investigate them.
- `steam_api.dll` and `goggame-*.dll` are in the install, but the exe does not import them.

## 3. Symbol map

- **`phase0\panzers_symbols.csv`** (`address,name,evidence_string`): 1,409 rows, covering **932 distinct functions** and 171 class names.
  - 1,382 rows come from `Class::Method` strings in .rdata. Each is mapped to every function that references the string, using Ghidra references to the string start (I also tried up to 3 bytes earlier).
  - 27 rows are PE exports.
  - 23 strings have no reference; their address column is empty.
  - 110 functions carry more than one name. This happens when a message names a different class (inlined callee, `SDArray<%s>` template, or a ctor that logs a parent class). Treat the most frequent name per address as the likely one.
- **`phase0\rtti.txt`**: 506 RTTI TypeDescriptor names (`.?AV...@@`), so **RTTI is present**. Ghidra applied it during analysis, and vftables are labelled (for example SSuperWindow::vftable at 0x80a174). The set includes the full UI menu hierarchy (SMainMenu, SSingleMenu, SOptionsMenu, …), the Gepard renderer (SGepard, SIGepard, SBoard, SModel, SMesh, SAnimesh, SScene), streams (SStream, SFileStream, SArchiveStream, SStreamBuffer, SSearchPathElement), properties (SProperty*), units and drivers, SMilesConcert, SWorld, plus 28 RakNet types.
- **`phase0\strings.txt`**: all printable strings (5 or more characters) with their VA.

## 4. Boot path (HD exe, Ghidra addresses)

```
entry 0x767a1c
 └ __scrt_common_main 0x767874  (CRT init)
   └ WinMain 0x64c920
      ├ srand(time); set ini path "panzers.ini" (0x64f6c0) and log dir "log" (0x64f780)
      ├ parse -ini <path> / -log <dir>
      ├ GetFileAttributesEx(panzers.ini) else MessageBox "MSG_STARTUP_NO_INI"
      ├ mkdir log dir 0x65fde0; SLogger 0x65c470("/panzers%08X.log","Codename: Panzers - Phase One") -> g_Log 0x929f28
      ├ prune old *.log files (keeps 5)
      ├ STimer::STimer 0x661520 (QPC/rdtsc calibration, "CPU Speed") -> 0x92e354
      ├ CoInitializeEx
      └ 0x64c800 (SEH wrapper) -> GameMain 0x64c870
           ├ new SSuperWindow(0x1c0) = SSuperWindow::SSuperWindow 0x656d50   -> g_SuperWindow 0x929cf4
           │   ├ SWindow/SDXWindow base ctor 0x539c70, vftable 0x80a174
           │   ├ SProperties load panzers.ini 0x65fe80; [Debug] debug level
           │   ├ [Paths] Home, [Paths] Search -> SFileSystem::SetSearchPath 0x65df90
           │   │      (split on ';', ".pak" => archive, SFileSystem::OpenArchive 0x65f1e0)
           │   ├ SFileSystem::SetHomePath 0x65fa30
           │   ├ SSettings load options.ini 0x64e330 (GetUserName, resolution, …; writer 0x64fdd0)
           │   ├ [Locale] Locale -> 0x52c320
           │   ├ string table "menu.ini" 0x660db0 (localized UI strings, [panzers/*.cpp] sections)
           │   └ desktop resolution / display-mode setup 0x53a380.. (no D3D yet)
           ├ 0x657540 ("PANZERS" window class, icon 0x67) -> SDXWindow::Create 0x539d70
           │   ├ SGepard factory 0x678ba0 -> SGepard::SGepard 0x676c20 + SGepard::Initialize 0x67d1c0
           │   │      (Direct3DCreate9, GetAdapterIdentifier, CreateDevice, shadow-path pick,
           │   │       InitTextureFormats 0x67cd30)
           │   ├ SMilesConcert factory 0x684fb0 -> SMilesConcert::SMilesConcert 0x684300
           │   │      (AIL_set_redist_directory "miles", AIL_startup, open_digital_driver, 3D providers)
           │   └ BinkSetSoundSystem(BinkOpenMiles, …)
           ├ SSuperWindow::Play 0x65b470
           │   ├ -host/-connect command-line paths: CreateHostFromCommandLine 0x6576f0 / ConnectToHostFromCommandLine 0x657460
           │   ├ normal path: FileNameProcess("intro.bik") 0x65f020 -> SSuperWindow::LoadBinkVideo 0x657ef0
           │   │      (BinkOpen, BinkBufferOpen; on failure -> Initialize + LoadMainMenu directly)
           │   ├ multiplayer path: SSettings::CHECKCDKEY 0x64d390 -> Initialize -> LoadMultiPreMenu 0x658a30 / LoadChatRoomView 0x658050
           │   └ MAIN LOOP: SWindow::Run 0x544db0  (GetMessage/PeekMessage pump; calls vtbl+0x90)
           └ delete g_SuperWindow
```

- **Per-frame function:** SSuperWindow vtbl+0x90 = **0x65ae50**. While the intro plays, it calls the Bink frame step **0x65ba00** (BinkDoFrame, BinkCopyToBuffer, BinkBufferBlit, BinkNextFrame).
- **When the video ends:** the Bink step calls **SSuperWindow::Initialize 0x657910**. That function:
  - loads unit/game data through 0x5cfe30 → SUnitRegistry::LoadUnitFiles 0x5d1050;
  - calls 0x544040;
  - creates the scene/board via 0x658690.
- **Then LoadMainMenu 0x6583e0** runs: `new SMainMenu(0x13dc)` at 0x633030 (identified by allocation size and log text), then SWindow attach (vtbl+0x54), resize to 1024x768, then 0x6352f0 (SMainMenu create). The vtbl slots +0x14 (0x65b390) and +0x24 (0x65b3e0) also skip the intro: Initialize, then LoadMainMenu.

Subsystems touched before the main menu, in order:

1. CRT
2. logger
3. timer
4. COM
5. panzers.ini
6. file system and all 7 paks
7. options.ini
8. locale and the menu.ini string table
9. window
10. D3D9 device (SGepard)
11. Miles
12. Bink intro
13. game data (units) in Initialize
14. main menu

The network objects (0x590ec0 SPanzersCampaign? and 0x51dcf0, size 0x5130) are created only on the command-line host/connect paths.

## 5. PAK format (`SFileSystem::OpenArchive` HD 0x65f1e0, `LookupArchive` 0x65ee70, open-read 0x65f420)

Header, 16 bytes, little-endian:

| off | size | value |
|---|---|---|
| 0x00 | 4 | 0x1B1A7253 (`"Sr\x1a\x1b"`). Anything else is "Not a Stormregion file". |
| 0x04 | 4 | 0x0A870A0D (`"\r\n\x87\n"`) |
| 0x08 | 4 | 0x4B434150 (`"PACK"`). Anything else is "Not a 'PACK' file". |
| 0x0C | 4 | tocSize. The TOC is read whole into memory, into SSearchPathElement +0x10 (size at +0xC). |

The TOC is a **binary search tree serialized in pre-order**. Each node is:

| off | size | field |
|---|---|---|
| 0 | 1 | `prefix`: the number of leading chars taken from the **parent node's full name** |
| 1 | 1 | `len`: length of the stored suffix |
| 2 | len | suffix (no NUL). Full name = parent.name[0:prefix] + suffix. Names are lowercase with `/` separators. |
| 2+len | 4 | data offset, relative to the end of the TOC |
| 6+len | 4 | data size in bytes |
| 10+len | 1 | `hasLeft`: 1 means the left child (lexically smaller) follows immediately (node+len+15) |
| 11+len | 4 | `right`: TOC offset of the right child (lexically greater), 0 if none |

- **Lookup:** lower-case the name and change `\` to `/`. Then `strncmp(name+prefix, suffix, len)`:
  - `<0`: go left (stop if !hasLeft);
  - `>0`: go right (stop if right==0);
  - `==0` and strlen(name) ≤ prefix+len: found;
  - otherwise go right.
- Directory nodes exist with size 0 (for example `menu`, `menu/briefing`).
- **File data begins at `16 + tocSize + offset`** (SArchiveStream ctor 0x65cba0, 0x2c bytes, given `entry.off + 0x10 + tocSize` and `entry.size`).
- **No compression flag and no compression method. Data is stored raw.**
- The HD build opens archive members with `fopen(...,"rb")` (FILE*-based SArchiveStream). The 2004 export used `_open`, but the format is the same.

Verified against `nordic_en.pak` (13,049,130 bytes, tocSize 212, so data starts at 228). Parsed with `scripts\pakparse.py`:

```
toc@0000 pre= 0 len=21 menu/credits_2_hq.tga  off=0030bb96 size=3145772 left=1 right=006e
toc@006e pre= 0 len=21 menu/credits_4_hq.tga  off=0090bbee size=3145772 left=1 right=00a9
toc@00a9 pre= 0 len=28 menu/main_menu_bottom_hq.tga off=00c0bc1a size=417836 left=0 right=0
toc@0092 pre=13 len= 8 menu/credits_3_hq.tga  off=0060bbc2 size=3145772 (prefix "menu/credits_" from parent)
toc@0024 pre= 0 len= 8 menu.ini               off=00000000 size=47978
toc@004e pre= 4 len=17 menu/credits_1_hq.tga  off=0000bb6a size=3145772 (prefix "menu" from parent menu.ini)
toc@003b pre= 0 len= 4 menu                   off=0 size=0 (directory)
```

The pak has only 7 nodes (6 files). Data at 228+0 starts `[panzers/ChatRoom.cpp]\r\n\r\nid_000 = "Multiplayer"`, and max(off+size)+228 equals the file size exactly. So the format is uncompressed with nothing trailing. `panzers_patch_en.pak` (6 nodes) also parses, including `pre=1` for `menu/briefing/su-05_hq.tga` under parent `missions_local.ini`. That pak has fewer than 20 entries; I did not parse the large paks.

**Compared with SWINE** (`swine-portable@95bbcc4:core/stream.cpp`; the task's path `src/core/stream.cpp` does not exist at that commit):

- **Same format.** SWINE's `SFileSystem::OpenArchive` checks the same 0x1B1A7253, 0x0A870A0D and "PACK" magics plus tocSize. `SArchiveInfo::Lookup` uses the same node walk: `after[8]` is hasLeft, `after+9` is the right offset, `after+13` is the left child. `SArchiveStream(fd, HeaderSize + 16 + entry->Pos, entry->Size)` uses the same base.
- **Differences:**
  - SWINE's generic `SStream::ReadSignature` also accepts the older 0x1A1A7253 magic (`NewTypeStrings=false`) for chunk files. The Panzers OpenArchive accepts only 0x1B1A7253. Panzers chunk-file signatures (FUN_0065d6a0) accept only 0x1B1A7253 too.
  - SWINE opens with `_sopen_s` and an fd. Panzers HD uses `fopen`/FILE* for archive members.
  - SWINE adds logging ("ARCHIVED FILE(%s)") and a FindFiles iterator.
  - Neither has compression or encryption.

## Not verified / open items

- Which 2004 build the export came from (retail 1.0 vs a patch). No such exe is available.
- The exact D3DX9 SDK release (inferred from 4.09.00.1126), and the GameSpy and RakNet SDK versions (no version strings).
- SMainMenu ctor 0x633030 and SMainMenu create 0x6352f0 are named by allocation size and call context, not by RTTI.
- What 0x544040 and 0x658690 do, inside Initialize and LoadMainMenu.
- The purpose of the `system32/...` decoy strings.
- The boot path is static analysis only. The game was not launched.
- The 110 multi-name entries in the CSV need manual resolution.
