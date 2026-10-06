#!/usr/bin/env python3
"""One command: mapping -> reccmp report -> re-scored drift table.

    python tools/reccmp_run.py

Needs a Release build (build/src/main/Release/panzers.exe + .pdb) and
reccmp-user.yml pointing at your original PANZERS.exe (see README). Writes,
all under build/ (never committed):

    reccmp-mapping.tsv   every // PANZERS marker and what it resolved to
    reccmp-report.json/.html   reccmp's own report
    reccmp-rescore.tsv   per-function drift scores, worst first

reccmp-functions.csv (repo root) is regenerated too; commit it when markers change.
build/pz_funcs.tsv (tools/ghidra/funcbounds.py) gives the original function
extents; without it the original is swept to the int3 padding.
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.abspath(os.path.join(HERE, ".."))


def run(cmd):
    print(">", " ".join(cmd))
    return subprocess.call(cmd, cwd=REPO)


def main():
    if run([sys.executable, os.path.join(HERE, "reccmp_gen.py")]):
        return 1
    if run(["reccmp-reccmp", "--target", "PANZERS", "--silent",
            "--json", "build/reccmp-report.json", "--html", "build/reccmp-report.html"]):
        return 1
    m = re.search(r"path:\s*['\"]?([^'\"\n]+)", open(os.path.join(REPO, "reccmp-user.yml")).read())
    cmd = [sys.executable, os.path.join(HERE, "reccmp_rescore.py"), "--orig", m.group(1).strip()]
    bounds = os.path.join(REPO, "build", "pz_funcs.tsv")
    if os.path.exists(bounds):
        cmd += ["--bounds", bounds]
    return run(cmd)


if __name__ == "__main__":
    sys.exit(main())
