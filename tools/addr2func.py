#!/usr/bin/env python3
"""Map a crash address of the recompiled panzers.exe to a function name.

Reads the MSVC linker map (build/src/main/Release/panzers.map by default).

    addr2func.py 0x1a2b3c            # an RVA (as printed in crash.txt)
    addr2func.py --va 0x5a2b3c       # an absolute address
    addr2func.py --map other.map 0x1a2b3c 0x1a2c00

crash.txt prints both the absolute EIP and its RVA; the exe links with
/DYNAMICBASE:NO, so absolute = preferred base + RVA.
"""
import argparse
import bisect
import os
import re
import sys

LINE_RE = re.compile(
    r"^\s*[0-9a-fA-F]{4}:[0-9a-fA-F]{8}\s+(\S+)\s+([0-9a-fA-F]{8})\s+(f\s+)?(i\s+)?(\S+)?\s*$")


def load_map(path):
    base = 0x400000
    syms = []
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        for line in f:
            if "Preferred load address is" in line:
                base = int(line.split()[-1], 16)
                continue
            m = LINE_RE.match(line)
            if not m:
                continue
            va = int(m.group(2), 16)
            if va == 0:
                continue
            is_func = bool(m.group(3))
            syms.append((va, m.group(1), is_func, (m.group(5) or "").strip()))
    syms.sort()
    return base, syms


def undecorate(name):
    try:
        import ctypes
        dbghelp = ctypes.windll.dbghelp
        buf = ctypes.create_string_buffer(1024)
        if dbghelp.UnDecorateSymbolName(name.encode(), buf, 1024, 0x1000):  # UNDNAME_NAME_ONLY
            return buf.value.decode(errors="replace")
    except Exception:
        pass
    return name


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    default_map = os.path.join(here, "..", "build", "src", "main", "Release", "panzers.map")
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("addr", nargs="+", help="hex address (RVA unless --va)")
    ap.add_argument("--map", default=default_map)
    ap.add_argument("--va", action="store_true", help="addresses are absolute, not RVAs")
    ap.add_argument("--all", action="store_true", help="also match data symbols, not only functions")
    a = ap.parse_args()

    base, syms = load_map(a.map)
    if not a.all:
        syms = [s for s in syms if s[2]]
    keys = [s[0] for s in syms]
    for t in a.addr:
        v = int(t, 16)
        va = v if a.va else base + v
        i = bisect.bisect_right(keys, va) - 1
        if i < 0:
            print("%s: no symbol below 0x%08x" % (t, va))
            continue
        sva, name, _, obj = syms[i]
        print("%s: VA 0x%08x RVA 0x%08x = %s+0x%x  [%s]" % (
            t, va, va - base, undecorate(name), va - sva, obj))
    return 0


if __name__ == "__main__":
    sys.exit(main())
