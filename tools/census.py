#!/usr/bin/env python3
"""Tripwire census for the Panzers recompile.

Prints exactly one line:

    CENSUS: <N> lifted bodies (PANZERS 0xADDR markers) + <M> SWINE-shared bodies + <K> stubs

Counting rules (scan of src/, C/C++ sources and headers):
  lifted       lines of the form   // PANZERS 0x<hex>        (anywhere in src/)
  SWINE-shared lines of the form   //----- (<hex>) ...        (IDA function markers
                                   carried over from the SWINE decomp; src/stubs excluded)
  stubs        STUB_LOG("...") call sites in src/stubs/*.c / *.cpp, plus the
               skeleton stubs of the 3D menu path in src/3dengine/pz and
               src/world (shown separately as "menu3d"), plus the M2 game
               logic skeleton in src/game and src/world/trigger*.cpp (shown
               separately as "m2"), plus the M3 skeleton: the files named in
               M3_FILES below, in src/game, src/world and src/panzers (shown
               separately as "m3"; docs/M3_INTERFACES.md)

Read this line after EVERY build. An unexpected change means bodies were
dropped, duplicated or silently replaced by stubs; the link exit code won't say.

Usage: census.py [src_dir]   (default: <repo>/src)
"""
import os
import re
import sys

LIFTED_RE = re.compile(r"^\s*//\s*PANZERS\s+0x[0-9A-Fa-f]+\b")
SWINE_RE = re.compile(r"^\s*//-+\s*\(\s*(?:0x)?[0-9A-Fa-f]+\s*\)")
STUB_RE = re.compile(r"^\s*STUB_LOG\(\s*\"")

# M3 skeleton files (docs/M3_INTERFACES.md). A STUB_LOG in one of these counts
# as "m3 skeleton" wherever the file is (src/game, src/world, src/panzers).
M3_FILES = re.compile(r"^(campaign|packets|unitsave|selection|ai|gameview|hud|minimap|"
                      r"ingamemenu|market|trainingmenu|superwindow_m3)[^/\\]*\.(c|cpp|cc|cxx)$", re.I)

SRC_EXT = (".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".inl")


def main() -> int:
    here = os.path.dirname(os.path.abspath(__file__))
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "src")
    src = os.path.abspath(src)
    stubs_dir = os.path.join(src, "stubs")
    menu3d_dirs = [os.path.join(src, "3dengine", "pz"), os.path.join(src, "world")]
    m2_dirs = [os.path.join(src, "game")]

    panzers_dir = os.path.join(src, "panzers")
    lifted = swine = stubs = menu3d = m2 = m3 = 0
    for root, dirs, files in os.walk(src):
        dirs.sort()
        in_stubs = os.path.commonpath([root, stubs_dir]) == stubs_dir
        in_menu3d_dir = any(os.path.commonpath([root, d]) == d for d in menu3d_dirs)
        in_m2_dir = any(os.path.commonpath([root, d]) == d for d in m2_dirs)
        for name in sorted(files):
            in_m3 = bool(M3_FILES.match(name)) and (in_m2_dir or in_menu3d_dir or
                                                    os.path.commonpath([root, panzers_dir]) == panzers_dir)
            in_m2 = not in_m3 and (in_m2_dir or (in_menu3d_dir and name.lower().startswith("trigger")))
            in_menu3d = in_menu3d_dir or in_m2 or in_m3
            if not name.lower().endswith(SRC_EXT):
                continue
            path = os.path.join(root, name)
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                for line in f:
                    if LIFTED_RE.match(line):
                        lifted += 1
                    elif in_stubs or in_menu3d:
                        if name.lower().endswith((".c", ".cpp", ".cc", ".cxx")) and STUB_RE.match(line):
                            if in_stubs:
                                stubs += 1
                            elif in_m3:
                                m3 += 1
                            elif in_m2:
                                m2 += 1
                            else:
                                menu3d += 1
                        elif in_menu3d and SWINE_RE.match(line):
                            swine += 1
                    elif SWINE_RE.match(line):
                        swine += 1

    print(f"CENSUS: {lifted} lifted bodies (PANZERS 0xADDR markers) + "
          f"{swine} SWINE-shared bodies + {stubs + menu3d + m2 + m3} stubs "
          f"({stubs} shell, {menu3d} menu3d skeleton, {m2} m2 skeleton, {m3} m3 skeleton)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
