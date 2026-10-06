#!/usr/bin/env python3
"""Re-score reccmp's function pairs with reference-agnostic normalisation.

reccmp 0.1.0 has two blind spots on this project:

1. It reads the original function with the *recompiled* size (capped at the
   next annotated original address). When our body is shorter or longer than
   the HD one, it compares against a truncated or overlong original.
2. Every call/data operand that reccmp can name on the recompiled side (any
   PDB symbol) but not on the original side (only the ~1.2k mapped addresses)
   is a mismatch, so calls to SWINE-shared code always count against us.

This script takes reccmp's pairs (its --json output), disassembles each pair
itself with capstone, using the original's real function extent (a bounds file
from Ghidra, or a linear sweep to the int3 padding) and the recompiled size
from the PDB, and compares the two sequences after normalising every address,
call/jump target and absolute operand to a placeholder. Register choice,
stack/struct offsets and small immediates are kept: those are real drift.

Output: a TSV with, per function, reccmp's own score, the normalised score
(`norm`), the same with registers folded by class (`regs`, the main drift
measure: it ignores register allocation, the bulk of the VS2015-vs-2026 noise),
the mnemonic-only score (`shape`) and both instruction counts.

    python tools/reccmp_rescore.py --json build/reccmp-report.json \
        --orig <PANZERS.exe> [--bounds pz_funcs.tsv] --out build/reccmp-rescore.tsv
"""
import argparse
import bisect
import difflib
import json
import os
import re
import sys

import capstone
import pefile

HEX_RE = re.compile(r"0x[0-9a-f]+")
# Register allocation is the main VS2015-vs-VS2026 noise: fold general
# registers (not ebp/esp, which carry the frame) and xmm registers by class.
REG_RE = re.compile(r"\b(?:e?[abcd]x|e?[sd]i|[abcd][lh]|xmm[0-7])\b")


def reg_fold(text):
    return REG_RE.sub(lambda m: "x" if m.group(0).startswith("xmm") else "r", text)


class Image:
    def __init__(self, path):
        self.pe = pefile.PE(path, fast_load=True)
        self.base = self.pe.OPTIONAL_HEADER.ImageBase
        self.data = self.pe.__data__

    def read(self, va, size):
        rva = va - self.base
        off = self.pe.get_offset_from_rva(rva)
        return bytes(self.data[off:off + size])


def norm_ins(ins):
    m = ins.mnemonic
    ops = ins.op_str
    if m.startswith("j") or m in ("call", "loop", "loope", "loopne", "jecxz"):
        if HEX_RE.fullmatch(ops.strip()):
            return m + " T"
        # indirect call/jmp: keep the shape, drop absolute addresses
    def rep(mo):
        v = int(mo.group(0), 16)
        return "A" if v >= 0x10000 else mo.group(0)
    return m + " " + HEX_RE.sub(rep, ops)


def disasm(img, va, size, md, stop_at_pad=False):
    code = img.read(va, size)
    out = []
    for ins in md.disasm(code, va):
        out.append(ins)
        if stop_at_pad and ins.mnemonic in ("ret", "retf", "jmp"):
            # end of function when followed by int3 padding (VS2015 aligns to 16 with 0xCC)
            nxt = ins.address + ins.size - va
            if nxt < len(code) and code[nxt] == 0xCC:
                break
    return out


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    repo = os.path.abspath(os.path.join(here, ".."))
    sys.path.insert(0, here)
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--json", default=os.path.join(repo, "build", "reccmp-report.json"))
    ap.add_argument("--orig", required=True, help="original PANZERS.exe")
    ap.add_argument("--recomp", default=os.path.join(repo, "build", "src", "main", "Release", "panzers.exe"))
    ap.add_argument("--pdb", default=os.path.join(repo, "build", "src", "main", "Release", "panzers.pdb"))
    ap.add_argument("--bounds", help="TSV of original functions: entry<TAB>min<TAB>end(hex) (Ghidra export)")
    ap.add_argument("--audit", default=os.path.join(repo, "build", "reccmp-mapping.tsv"))
    ap.add_argument("--out", default=os.path.join(repo, "build", "reccmp-rescore.tsv"))
    args = ap.parse_args()

    from reccmp_gen import load_pdb_functions
    rsize = {}
    orig_img, rec_img = Image(args.orig), Image(args.recomp)
    rec_text = None
    for s in rec_img.pe.sections:
        if s.Name.rstrip(b"\0") == b".text":
            rec_text = s
    for fn in load_pdb_functions(args.pdb):
        if fn["section"] == 1:
            rsize[rec_img.base + rec_text.VirtualAddress + fn["offset"]] = fn["size"]

    bounds = {}
    if args.bounds:
        for line in open(args.bounds):
            e, lo, hi = line.split("\t")[:3]
            bounds[int(e, 16)] = int(hi, 16) - int(e, 16)

    files = {}
    if os.path.exists(args.audit):
        for line in open(args.audit, encoding="utf-8"):
            p = line.rstrip("\n").split("\t")
            if p[0].startswith("0x") and not p[3].startswith("unresolved") and "+dup" not in p[3]:
                files[int(p[0], 16)] = "%s:%s" % (p[1], p[2])

    md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    rows = []
    for e in json.load(open(args.json))["data"]:
        oa, ra = int(e["address"], 16), int(e["recomp"], 16)
        rs = rsize.get(ra)
        if not rs:
            continue
        if oa in bounds:
            o = disasm(orig_img, oa, bounds[oa], md)
        else:
            o = disasm(orig_img, oa, 0x4000, md, stop_at_pad=True)
        r = disasm(rec_img, ra, rs, md)
        on, rn = [norm_ins(i) for i in o], [norm_ins(i) for i in r]
        norm = difflib.SequenceMatcher(None, on, rn, autojunk=False).ratio()
        regs = difflib.SequenceMatcher(None, [reg_fold(i) for i in on], [reg_fold(i) for i in rn],
                                       autojunk=False).ratio()
        shape = difflib.SequenceMatcher(None, [i.mnemonic for i in o], [i.mnemonic for i in r],
                                        autojunk=False).ratio()
        rows.append((oa, e["name"], e["matching"], norm, regs, shape, len(o), len(r), files.get(oa, "")))

    rows.sort(key=lambda r: r[4])
    with open(args.out, "w", encoding="utf-8", newline="\n") as f:
        f.write("orig\tname\treccmp\tnorm\tregs\tshape\torig_ins\trecomp_ins\tsource\n")
        for r in rows:
            f.write("0x%x\t%s\t%.3f\t%.3f\t%.3f\t%.3f\t%d\t%d\t%s\n" % r)

    def bucket(v):
        return "100" if v >= 0.9999 else ">=90" if v >= .9 else "70-90" if v >= .7 else "50-70" if v >= .5 else "<50"
    from collections import Counter
    for label, idx in (("reccmp", 2), ("norm", 3), ("regs", 4), ("shape", 5)):
        c = Counter(bucket(r[idx]) for r in rows)
        print("RESCORE %-6s %s" % (label, "  ".join("%s:%d" % (k, c.get(k, 0)) for k in ("100", ">=90", "70-90", "50-70", "<50"))))
    print("RESCORE %d pairs -> %s" % (len(rows), args.out))
    return 0


if __name__ == "__main__":
    sys.exit(main())
