// src/main/crashhandler.cpp
// Recompile addition (not in PANZERS.exe): an unhandled-exception filter that
// writes crash.txt next to the exe's working directory with the exception,
// EIP (absolute and RVA), registers, a 32-frame stack walk and the last 50
// lines of the current log. Resolve RVAs with tools/addr2func.py.

#include <windows.h>
#include <dbghelp.h>
#include <stdio.h>
#include <string.h>
#include "logger.h"

static LONG WINAPI PanzersCrashFilter(EXCEPTION_POINTERS* ep)
{
    static volatile LONG inHandler = 0;
    if (InterlockedExchange(&inHandler, 1))
        return EXCEPTION_CONTINUE_SEARCH;

    FILE* f = fopen("crash.txt", "w");
    if (!f)
        return EXCEPTION_CONTINUE_SEARCH;

    HMODULE base = GetModuleHandleA(nullptr);
    const EXCEPTION_RECORD* er = ep->ExceptionRecord;
    const CONTEXT* c = ep->ContextRecord;
    DWORD eip = c->Eip;
    fprintf(f, "Codename Panzers recompile crash\n");
    fprintf(f, "Exception 0x%08lX at 0x%08lX (module base 0x%08lX, RVA 0x%08lX)\n",
            er->ExceptionCode, (unsigned long)eip, (unsigned long)(UINT_PTR)base,
            (unsigned long)(eip - (DWORD)(UINT_PTR)base));
    if (er->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && er->NumberParameters >= 2)
        fprintf(f, "Access violation: %s 0x%08lX\n",
                er->ExceptionInformation[0] == 0 ? "read" : er->ExceptionInformation[0] == 1 ? "write" : "execute",
                (unsigned long)er->ExceptionInformation[1]);
    fprintf(f, "\nEAX=%08lX EBX=%08lX ECX=%08lX EDX=%08lX\nESI=%08lX EDI=%08lX EBP=%08lX ESP=%08lX\nEIP=%08lX EFL=%08lX\n",
            c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp, c->Eip, c->EFlags);

    fprintf(f, "\nStack (RVA = address - 0x%08lX):\n", (unsigned long)(UINT_PTR)base);
    HANDLE proc = GetCurrentProcess();
    HANDLE thread = GetCurrentThread();
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
    BOOL haveSyms = SymInitialize(proc, nullptr, TRUE);
    CONTEXT ctx = *c;
    STACKFRAME64 sf;
    memset(&sf, 0, sizeof(sf));
    sf.AddrPC.Offset = ctx.Eip;    sf.AddrPC.Mode = AddrModeFlat;
    sf.AddrFrame.Offset = ctx.Ebp; sf.AddrFrame.Mode = AddrModeFlat;
    sf.AddrStack.Offset = ctx.Esp; sf.AddrStack.Mode = AddrModeFlat;
    for (int i = 0; i < 32; ++i) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_I386, proc, thread, &sf, &ctx, nullptr,
                         SymFunctionTableAccess64, SymGetModuleBase64, nullptr))
            break;
        DWORD64 pc = sf.AddrPC.Offset;
        if (!pc)
            break;
        char symbuf[sizeof(SYMBOL_INFO) + 256];
        SYMBOL_INFO* sym = (SYMBOL_INFO*)symbuf;
        sym->SizeOfStruct = sizeof(SYMBOL_INFO);
        sym->MaxNameLen = 255;
        DWORD64 disp = 0;
        const char* name = "?";
        if (haveSyms && SymFromAddr(proc, pc, &disp, sym))
            name = sym->Name;
        fprintf(f, "  #%02d 0x%08lX RVA 0x%08lX  %s+0x%lX\n", i, (unsigned long)pc,
                (unsigned long)(pc - (DWORD64)(UINT_PTR)base), name, (unsigned long)disp);
    }
    if (haveSyms)
        SymCleanup(proc);

    fprintf(f, "\nLast log lines:\n");
    if (Logger.g && Logger.g->LogFileName) {
        Logger.g->WirteBufferToLog();
        FILE* lf = fopen(Logger.g->LogFileName, "rb");
        if (lf) {
            static char ring[50][512];
            int n = 0;
            char line[512];
            while (fgets(line, sizeof(line), lf)) {
                strncpy(ring[n % 50], line, sizeof(ring[0]) - 1);
                ring[n % 50][sizeof(ring[0]) - 1] = 0;
                ++n;
            }
            fclose(lf);
            int first = n > 50 ? n - 50 : 0;
            for (int i = first; i < n; ++i)
                fputs(ring[i % 50], f);
        }
    }
    fclose(f);
    return EXCEPTION_EXECUTE_HANDLER;   // terminate after writing crash.txt
}

void InstallCrashHandler()
{
    SetUnhandledExceptionFilter(PanzersCrashFilter);
}
