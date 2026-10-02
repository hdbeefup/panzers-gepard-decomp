#!/usr/bin/env python3
"""Tripwire census for the Panzers recompile.

Prints exactly one line:

    CENSUS: <N> lifted bodies (PANZERS 0xADDR markers) + <M> SWINE-shared bodies + <K> stubs

Counting rules (scan of src/, C/C++ sources and headers):
  lifted       lines of the form   // PANZERS 0x<hex>        (anywhere in src/)
  SWINE-shared lines of the form   //----- (<hex>) ...        (IDA function markers
                                   carried over from the SWINE decomp; src/stubs excluded)
  stubs        STUB_LOG("...") call sites in src/stubs/*.c / *.cpp

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

SRC_EXT = (".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".inl")


def main() -> int:
    here = os.path.dirname(os.path.abspath(__file__))
    src = sys.argv[1] if len(sys.argv) > 1 else os.path.join(here, "..", "src")
    src = os.path.abspath(src)
    stubs_dir = os.path.join(src, "stubs")

    lifted = swine = stubs = 0
    for root, dirs, files in os.walk(src):
        dirs.sort()
        in_stubs = os.path.commonpath([root, stubs_dir]) == stubs_dir
        for name in sorted(files):
            if not name.lower().endswith(SRC_EXT):
                continue
            path = os.path.join(root, name)
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                for line in f:
                    if LIFTED_RE.match(line):
                        lifted += 1
                    elif in_stubs:
                        if name.lower().endswith((".c", ".cpp", ".cc", ".cxx")) and STUB_RE.match(line):
                            stubs += 1
                    elif SWINE_RE.match(line):
                        swine += 1

    print(f"CENSUS: {lifted} lifted bodies (PANZERS 0xADDR markers) + "
          f"{swine} SWINE-shared bodies + {stubs} stubs")
    return 0


if __name__ == "__main__":
    sys.exit(main())
