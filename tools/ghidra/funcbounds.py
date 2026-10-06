"""Export every function of an exe as `entry<TAB>min<TAB>end<TAB>name` (hex).

`end` is the end of the address range that contains the entry (Ghidra may
add far-away chunks to a body; those are left out). tools/reccmp_rescore.py
and tools/swine_xcmp.py read this as the original functions' extents.

    python tools/ghidra/funcbounds.py --exe <PANZERS.exe> --out build/pz_funcs.tsv

Uses the analysed project tools/ghidra/panzers.gpr when it exists (opened
read-only), otherwise imports and auto-analyses a copy into build/ghidra-tmp
(about 10 minutes for PANZERS.exe). The exe itself is only read.
"""
import argparse
import os

os.environ.setdefault("GHIDRA_INSTALL_DIR", r"C:\Users\swine\scoop\apps\ghidra\current")
import pyghidra  # noqa: E402

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, "..", ".."))

ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
ap.add_argument("--exe", required=True)
ap.add_argument("--out", required=True)
ap.add_argument("--project-dir", default=HERE)
ap.add_argument("--project-name", default="panzers")
args = ap.parse_args()

pyghidra.start()
from ghidra.base.project import GhidraProject  # noqa: E402
from java.io import File  # noqa: E402

name = os.path.basename(args.exe)
if os.path.exists(os.path.join(args.project_dir, args.project_name + ".gpr")):
    proj = GhidraProject.openProject(args.project_dir, args.project_name, True)
    prog = proj.openProgram("/", name, True)
else:
    tmp = os.path.join(REPO, "build", "ghidra-tmp")
    os.makedirs(tmp, exist_ok=True)
    tname = os.path.splitext(name)[0].lower()
    if os.path.exists(os.path.join(tmp, tname + ".gpr")):
        proj = GhidraProject.openProject(tmp, tname, False)
        prog = proj.openProgram("/", name, False)
    else:
        proj = GhidraProject.createProject(tmp, tname, False)
        prog = proj.importProgram(File(os.path.abspath(args.exe)))
        proj.analyze(prog)
        proj.saveAs(prog, "/", name, True)

with open(args.out, "w", encoding="utf-8", newline="\n") as f:
    for fn in prog.getFunctionManager().getFunctions(True):
        body = fn.getBody()
        ent = fn.getEntryPoint()
        rng = body.getRangeContaining(ent)
        end = (rng or body).getMaxAddress().getOffset() + 1
        f.write("%x\t%x\t%x\t%s\n" % (ent.getOffset(), body.getMinAddress().getOffset(), end, fn.getName()))
proj.close()
print("wrote", args.out)
