#!/usr/bin/env python3
"""Generate reccmp-functions.csv from the `// PANZERS 0xADDR` markers.

Each lifted body carries a `// PANZERS 0xADDR` marker (tools/census.py). This
script maps every marker to the recompiled function it labels and writes a
reccmp data source:

    address|name|symbol|type
    6f2420|STerrain::AddDecal|?AddDecal@STerrain@@QAEXHHHHH@Z|function

reccmp then pairs the original address with our function by the decorated
`symbol` (unique), or by `name` for static functions without a public symbol.

How a marker is resolved, using the PDB's line table (via reccmp's cvdump):
  * enclosing:  the marker line is inside a function body (an indented
                "partial" marker inside a SWINE-shared body). The innermost
                function whose line range contains it is taken.
  * following:  otherwise, the first function in the same file that starts
                after the marker, provided no other marker sits between them
                and it starts within --max-gap lines.
A marker that resolves to neither (the body was inlined everywhere, dropped by
/OPT:REF, or folded by /OPT:ICF) is left out and listed in the audit file.

Sanity check: for "following" markers the identifier before the first `(` in
the source after the marker is compared with the PDB function name's last
component; mismatches are flagged in the audit (column `name_ok`).

Usage (after a Release build):
    python tools/reccmp_gen.py
    python tools/reccmp_gen.py --pdb build/src/main/Release/panzers.pdb \
        --out reccmp-functions.csv --audit build/reccmp-mapping.tsv
"""
import argparse
import bisect
import os
import re
import sys
from collections import defaultdict
from pathlib import PureWindowsPath

MARKER_RE = re.compile(r"^(\s*)//\s*PANZERS\s+0x([0-9A-Fa-f]+)\b(.*)$")
SRC_EXT = (".c", ".cc", ".cpp", ".cxx", ".h", ".hpp", ".inl")
# identifier (possibly qualified, possibly ~dtor / operator) right before '('
IDENT_RE = re.compile(r"((?:[A-Za-z_]\w*\s*::\s*)*~?[A-Za-z_]\w*|operator\s*\S+?)\s*\(")
SKIP_WORDS = {"if", "for", "while", "switch", "return", "sizeof", "PZ_TRACE", "STUB_LOG",
              "defined", "static_assert", "__declspec", "alignas", "decltype"}


def norm(p):
    return os.path.normcase(os.path.normpath(str(p)))


def scan_markers(src):
    markers = []  # (file_norm, relpath, line, addr, indented, tail, following_text)
    for root, dirs, files in os.walk(src):
        dirs.sort()
        for name in sorted(files):
            if not name.lower().endswith(SRC_EXT):
                continue
            path = os.path.join(root, name)
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                lines = f.readlines()
            for i, line in enumerate(lines, start=1):
                m = MARKER_RE.match(line)
                if not m:
                    continue
                # Source text after the marker, skipping comment lines, for the name check.
                follow = []
                for nxt in lines[i:i + 40]:
                    s = nxt.strip()
                    if not s or s.startswith("//") or s.startswith("/*") or s.startswith("*"):
                        continue
                    follow.append(s)
                    if "(" in s or len(follow) > 4:
                        break
                markers.append(dict(file=norm(path), rel=os.path.relpath(path, os.path.dirname(src)).replace("\\", "/"),
                                    line=i, addr=int(m.group(2), 16), indented=len(m.group(1)) > 0,
                                    tail=m.group(3).strip(), follow=" ".join(follow)))
    return markers


def source_ident(text):
    for m in IDENT_RE.finditer(text):
        ident = re.sub(r"\s+", "", m.group(1))
        if ident.split("::")[-1] in SKIP_WORDS:
            continue
        return ident
    return None


def last_comp(name):
    if not name:
        return ""
    name = re.sub(r"<.*>", "", name)  # strip template args
    return name.split("::")[-1].strip()


def load_pdb_functions(pdb):
    import logging
    logging.disable(logging.CRITICAL)  # cvdump's "Unhandled symbol type" noise
    from reccmp.cvdump import Cvdump
    from reccmp.cvdump.analysis import CvdumpAnalysis

    parser = Cvdump(pdb).lines().globals().publics().symbols().run()
    analysis = CvdumpAnalysis(parser)

    funcs = []
    for node in analysis.nodes:
        sym = node.symbol_entry
        if sym is None or node.node_type is None or sym.type not in ("S_GPROC32", "S_LPROC32"):
            continue
        funcs.append(dict(section=node.section, offset=node.offset, size=sym.size or 0,
                          name=sym.name, symbol=node.decorated_name))

    # Line table: per section, sorted (offset, file, line).
    by_sec = defaultdict(list)
    for fname, values in analysis.lines.items():
        fn = norm(PureWindowsPath(fname))
        for v in values:
            by_sec[v.section].append((v.offset, fn, v.line_number))
    for v in by_sec.values():
        v.sort()
    keys = {s: [e[0] for e in v] for s, v in by_sec.items()}

    for fn in funcs:
        entries = by_sec.get(fn["section"], [])
        ks = keys.get(fn["section"], [])
        lo = bisect.bisect_left(ks, fn["offset"])
        hi = bisect.bisect_left(ks, fn["offset"] + max(fn["size"], 1))
        span = entries[lo:hi]
        if not span or span[0][0] != fn["offset"]:
            fn["file"] = None
            continue
        fn["file"] = span[0][1]
        fl = [ln for (_, f, ln) in span if f == fn["file"]]
        fn["start_line"] = span[0][2]
        fn["min_line"] = min(fl)
        fn["max_line"] = max(fl)
    return funcs


def same_name(src_name, pdb_name):
    a, b = last_comp(src_name), last_comp(pdb_name)
    # Win32 A/W macros rename e.g. PlaySound -> PlaySoundA in the PDB.
    return bool(a) and (a == b or b in (a + "A", a + "W"))


def resolve(markers, funcs, max_gap):
    by_file = defaultdict(list)
    for fn in funcs:
        if fn.get("file"):
            by_file[fn["file"]].append(fn)
    marker_lines = defaultdict(list)
    for m in markers:
        marker_lines[m["file"]].append(m["line"])
    for v in marker_lines.values():
        v.sort()

    for m in markers:
        cands = by_file.get(m["file"], [])
        m["fn"] = None
        m["how"] = "unresolved"
        src_name = source_ident(m["follow"])
        tm = re.match(r"\(([A-Za-z_~][\w:~<>, *]*?)\s*[,)]", m["tail"])
        tail_name = tm.group(1).strip() if tm else None
        m["src_name"] = src_name

        # 1) enclosing: a "partial" marker inside a body, "(Name, partial): ...".
        #    Taken only when the named function really contains the marker line.
        if tail_name:
            enc = [f for f in cands if f["min_line"] <= m["line"] <= f["max_line"]
                   and same_name(tail_name, f["name"])]
            if enc:
                m["fn"] = min(enc, key=lambda f: f["max_line"] - f["min_line"])
                m["how"] = "enclosing"
                m["src_name"] = tail_name
        # 2) following: first function of the same name that starts after the
        #    marker, with no other marker in between.
        if m["fn"] is None and src_name:
            after = [f for f in cands if f["min_line"] > m["line"] and same_name(src_name, f["name"])]
            if after:
                f = min(after, key=lambda f: (f["min_line"], f["offset"]))
                ml = marker_lines[m["file"]]
                j = bisect.bisect_right(ml, m["line"])
                if j < len(ml) and ml[j] < f["min_line"]:
                    m["how"] = "unresolved(next marker first: body inlined or folded)"
                elif f["min_line"] - m["line"] > max_gap:
                    m["how"] = "unresolved(gap %d)" % (f["min_line"] - m["line"])
                else:
                    m["fn"], m["how"] = f, "following"
            else:
                m["how"] = "unresolved(no PDB body named %s: inlined, folded or dropped)" % last_comp(src_name)

        m["name_ok"] = "yes" if m["fn"] is not None else ""


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.abspath(os.path.join(here, ".."))
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--pdb", default=os.path.join(repo, "build", "src", "main", "Release", "panzers.pdb"))
    ap.add_argument("--src", default=os.path.join(repo, "src"))
    ap.add_argument("--out", default=os.path.join(repo, "reccmp-functions.csv"))
    ap.add_argument("--audit", default=os.path.join(repo, "build", "reccmp-mapping.tsv"))
    ap.add_argument("--max-gap", type=int, default=80,
                    help="max lines between a marker and the function it labels")
    args = ap.parse_args()

    if not os.path.exists(args.pdb):
        sys.exit("PDB not found: %s (build Release first)" % args.pdb)

    markers = scan_markers(os.path.abspath(args.src))
    funcs = load_pdb_functions(args.pdb)
    resolve(markers, funcs, args.max_gap)

    def rank(m):
        piece = m["how"] == "following" and m["tail"].startswith("(")
        return (not piece, m["fn"]["size"])

    # One row per original address; one original address per recompiled function.
    rows = {}
    taken = {}
    dup_addr = dup_fn = 0
    for m in markers:
        fn = m["fn"]
        if fn is None:
            continue
        key = (fn["section"], fn["offset"])
        if m["addr"] in rows:
            dup_addr += 1
            old = rows[m["addr"]]
            # Two markers for one HD address: prefer a plain marker over a
            # "(piece)" one (code the HD inlined into that function), then
            # the larger recompiled body.
            if rank(m) <= rank(old):
                m["how"] += "+dup-addr"
                continue
            old["how"] += "+dup-addr"
            del taken[(old["fn"]["section"], old["fn"]["offset"])]
        if key in taken:
            m["how"] += "+dup-func(0x%x)" % taken[key]
            dup_fn += 1
            continue
        taken[key] = m["addr"]
        rows[m["addr"]] = m

    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("# Generated by tools/reccmp_gen.py from the // PANZERS 0xADDR markers. Do not edit.\n")
        f.write("address|name|symbol|type\n")
        for addr in sorted(rows):
            m = rows[addr]
            fn = m["fn"]
            sym = fn["symbol"] or ""
            name = fn["name"].replace("|", "_")
            f.write("%x|%s|%s|function\n" % (addr, name, sym))

    os.makedirs(os.path.dirname(os.path.abspath(args.audit)), exist_ok=True)
    with open(args.audit, "w", encoding="utf-8", newline="\n") as f:
        f.write("orig\tfile\tline\thow\tname_ok\tsrc_name\tpdb_name\trecomp_off\tsymbol\n")
        for m in markers:
            fn = m["fn"] or {}
            f.write("0x%x\t%s\t%d\t%s\t%s\t%s\t%s\t%s\t%s\n" % (
                m["addr"], m["rel"], m["line"], m["how"], m["name_ok"], m["src_name"] or "",
                fn.get("name", ""), ("%d:%x" % (fn["section"], fn["offset"])) if fn else "",
                fn.get("symbol") or ""))

    from collections import Counter
    hows = Counter(m["how"].split("(")[0].split("+")[0] for m in markers)
    names = Counter(m["name_ok"] for m in markers if m["fn"] is not None)
    print("RECCMP-GEN: %d markers -> %d csv rows; %s; name check %s; dup-addr %d, dup-func %d; audit %s" % (
        len(markers), len(rows), dict(hows), dict(names), dup_addr, dup_fn, args.audit))
    return 0


if __name__ == "__main__":
    sys.exit(main())
