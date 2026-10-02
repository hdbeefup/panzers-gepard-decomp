# Ghidra helper scripts (pyghidra, headless)

These come from Phase 0 triage. They expect this layout in a working folder
(`P0`), which is never committed:

    P0/PANZERS.exe          copy of the HD exe (never the install's file)
    P0/ghidra/panzers.gpr   headless Ghidra project of that copy (auto-analysed)
    P0/scripts/*.py         these files

Set `GHIDRA_INSTALL_DIR` and run from `P0/scripts/`. If you run from `P0/`, the
`ghidra/` folder shadows the Java `ghidra` package.

    python dec.py out.c 0x64c920 s:"Not a Stormregion file"

- `dec.py`: decompile functions given by address or by a referenced string (`s:`).
- `callers.py`: list a function's callers.
- `symbols.py`: build the Class::Method symbol CSV.
- `pakparse.py`: dump a pak's TOC.

Recreate the project with Ghidra `analyzeHeadless P0/ghidra panzers -import P0/PANZERS.exe`.
