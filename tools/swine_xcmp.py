#!/usr/bin/env python3
"""Compare S.W.I.N.E. HD originals with their Panzers HD counterparts.

Both originals were built with the same compiler (VS2015), so a pair that is
the same source compiles to nearly the same code: this measures how much of a
SWINE function survives in Panzers, i.e. whether copying the SWINE body would
give a near-correct Panzers function.

Pairs come from function names that occur once in swinedecomp's
reccmp-functions.csv (SWINE HD addresses) and in our build/reccmp-mapping.tsv
(Panzers HD addresses, from the // PANZERS markers). Scores are the
reference-agnostic, register-folded measure of tools/reccmp_rescore.py.

    python tools/swine_xcmp.py --swine-exe swineHD.exe --swine-csv <swinedecomp>/reccmp-functions.csv \
        --swine-bounds sw_funcs.tsv --pz-exe PANZERS.exe --pz-bounds pz_funcs.tsv
"""
import argparse
import difflib
import os
import re
import sys

import capstone

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from reccmp_rescore import Image, disasm, norm_ins, reg_fold  # noqa: E402


def load_bounds(path):
    b = {}
    for line in open(path):
        e, lo, hi = line.split("\t")[:3]
        b[int(e, 16)] = int(hi, 16) - int(e, 16)
    return b


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.abspath(os.path.join(here, ".."))
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--swine-exe", required=True)
    ap.add_argument("--swine-csv", required=True)
    ap.add_argument("--swine-bounds", required=True)
    ap.add_argument("--pz-exe", required=True)
    ap.add_argument("--pz-bounds", required=True)
    ap.add_argument("--mapping", default=os.path.join(repo, "build", "reccmp-mapping.tsv"))
    ap.add_argument("--rescore", default=os.path.join(repo, "build", "reccmp-rescore.tsv"))
    ap.add_argument("--out", default=os.path.join(repo, "build", "swine-xcmp.tsv"))
    args = ap.parse_args()

    sw = {}
    for line in open(args.swine_csv, encoding="utf-8"):
        p = line.strip().split("|")
        if len(p) >= 2 and p[0] != "address" and not p[0].startswith("#"):
            sw.setdefault(p[1], []).append(int(p[0], 16))
    pz = {}
    for line in open(args.mapping, encoding="utf-8"):
        p = line.rstrip("\n").split("\t")
        if not p[0].startswith("0x") or p[3].startswith("unresolved") or not p[6]:
            continue
        pz.setdefault(re.sub(r"^pz::", "", p[6]), []).append((int(p[0], 16), p[1]))
    ours = {}
    if os.path.exists(args.rescore):
        for line in open(args.rescore, encoding="utf-8"):
            p = line.rstrip("\n").split("\t")
            if p[0].startswith("0x"):
                ours[int(p[0], 16)] = float(p[4])

    swi, pzi = Image(args.swine_exe), Image(args.pz_exe)
    swb, pzb = load_bounds(args.swine_bounds), load_bounds(args.pz_bounds)
    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    rows = []
    for name in sorted(pz):
        if name not in sw or len(sw[name]) != 1 or len(pz[name]) != 1:
            continue
        sa, (pa, src) = sw[name][0], pz[name][0]
        if sa not in swb or pa not in pzb:
            continue
        s = [reg_fold(norm_ins(i)) for i in disasm(swi, sa, swb[sa], md)]
        p = [reg_fold(norm_ins(i)) for i in disasm(pzi, pa, pzb[pa], md)]
        r = difflib.SequenceMatcher(None, s, p, autojunk=False).ratio()
        rows.append((name, sa, pa, r, len(s), len(p), ours.get(pa), src))

    rows.sort(key=lambda r: -r[3])
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("name\tswine\tpanzers\tswine_vs_pz\tswine_ins\tpz_ins\tours_vs_pz\tsource\n")
        for r in rows:
            f.write("%s\t0x%x\t0x%x\t%.3f\t%d\t%d\t%s\t%s\n" % (
                r[0], r[1], r[2], r[3], r[4], r[5], "" if r[6] is None else "%.3f" % r[6], r[7]))
    print("XCMP %d pairs -> %s" % (len(rows), args.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
