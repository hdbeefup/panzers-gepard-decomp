// src/main/panzers_main.cpp
// Placeholder entry point for the Codename Panzers: Phase One recompile.
//
// This does nothing useful yet: it logs "panzers skeleton" and exits. The real
// WinMain will be lifted from PANZERS.exe later (mark it `// PANZERS 0xADDR`).

#include <windows.h>
#include <stdio.h>

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    (void)hInst; (void)hPrevInst; (void)lpCmdLine; (void)nShowCmd;

    OutputDebugStringA("panzers skeleton\n");
    fprintf(stderr, "panzers skeleton\n");
    return 0;
}
