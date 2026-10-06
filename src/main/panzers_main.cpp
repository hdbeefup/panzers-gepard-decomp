// src/main/panzers_main.cpp
// WinMain of the Codename Panzers: Phase One recompile (HD PANZERS.exe).

#include <windows.h>
#include <objbase.h>
#include <direct.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "logger.h"
#include "stream.h"
#include "settings.h"
#include "superwindow.h"
#include "pz/pzcommon.h"
#include "m2common.h"
#include "m3common.h"

extern HINSTANCE hInstance;
void RunGame();                       // PANZERS 0x64c800 (src/panzers/gamemain.cpp)
void InstallCrashHandler();           // src/main/crashhandler.cpp

// PANZERS 0x65fde0: mkdir that creates missing parents (recursive on ENOENT).
static void MakeDirectory(const char* path)
{
    if (!path || !*path)
        return;
    if (_mkdir(path) < 0 && errno != EEXIST) {
        char parent[MAX_PATH];
        strncpy(parent, path, MAX_PATH - 1);
        parent[MAX_PATH - 1] = 0;
        char* slash = strrchr(parent, '/');
        char* bslash = strrchr(parent, '\\');
        if (bslash > slash) slash = bslash;
        if (slash && slash != parent) {
            *slash = 0;
            MakeDirectory(parent);
            _mkdir(path);
        }
    }
}

struct LogFileInfo {                  // HD SDArray element 0x14 (qsort stride)
    char name[MAX_PATH];
    FILETIME time;
};

// PANZERS 0x64c560 (qsort comparator: CompareFileTime)
static int __cdecl CompareLogTime(const void* a, const void* b)
{
    return CompareFileTime(&((const LogFileInfo*)a)->time, &((const LogFileInfo*)b)->time);
}

// The HD log-pruning part of WinMain: FindFiles("<logdir>/*.log"), sort by
// time, delete all but the newest 5.
static void PruneOldLogs(const char* logDir)
{
    char pattern[MAX_PATH];
    _snprintf(pattern, MAX_PATH - 1, "%s/*.log", logDir);
    pattern[MAX_PATH - 1] = 0;
    LogFileInfo* files = nullptr;
    int count = 0, cap = 0;
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (count == cap) {
                cap = cap ? cap * 2 : 16;
                files = (LogFileInfo*)realloc(files, cap * sizeof(LogFileInfo));
            }
            strncpy(files[count].name, fd.cFileName, MAX_PATH - 1);
            files[count].name[MAX_PATH - 1] = 0;
            files[count].time = fd.ftLastWriteTime;
            ++count;
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    if (count > 5) {
        qsort(files, count, sizeof(LogFileInfo), CompareLogTime);
        for (int i = 0; i < count - 5; ++i) {
            char path[MAX_PATH];
            _snprintf(path, MAX_PATH - 1, "%s/%s", logDir, files[i].name);
            path[MAX_PATH - 1] = 0;
            int r = remove(path);                       // 0x65f9e0
            Logger.g->Log(1, "Removing old logfile '%s', result=%d", path, r);
        }
    }
    free(files);
}

// PANZERS 0x64c920
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR lpCmdLine, int nShowCmd)
{
    (void)hPrevInst; (void)lpCmdLine; (void)nShowCmd;
    InstallCrashHandler();            // recompile addition

    srand((unsigned)_time64(nullptr));
    hInstance = hInst;                // DAT_008f2070
    __time64_t startTime = _time64(nullptr);
    Settings.SetIniPath("panzers.ini");   // 0x64f6c0
    Settings.SetLogDir("log");            // 0x64f780

    for (int i = 1; i < __argc; ++i) {
        const char* a = __argv[i];
        if (!a || !*a)
            continue;
        if (!_stricmp(a, "-ini")) {
            if (++i >= __argc) {
                MessageBoxA(nullptr, "The switch '-ini' needs a path specified", "Fatal Error", MB_ICONERROR);
                return 0;
            }
            Settings.SetIniPath(__argv[i]);
        } else if (!_stricmp(a, "-log")) {
            if (++i >= __argc) {
                MessageBoxA(nullptr, "The switch '-log' needs a directory specified", "Fatal Error", MB_ICONERROR);
                return 0;
            }
            Settings.SetLogDir(__argv[i]);
        }
    }

    // Recompile-only "-nointro": removed from argv before SSettings parses it
    // (HD would take an unknown argument as a map path, see settings.cpp).
    {
        int w = 1;
        for (int r = 1; r < __argc; ++r) {
            if (__argv[r] && !_stricmp(__argv[r], "-nointro")) {
                SSuperWindow::SkipIntro = true;
                continue;
            }
            __argv[w++] = __argv[r];
        }
        __argc = w;
    }
    // Recompile-only "-menu3d" / "-nomenu3d" (and PZ_MENU3D / PZ_MENU3D_TRACE): 3D menu
    // world test hook, src/3dengine/pz/pztrace.cpp.
    pz::Menu3DParseCommandLine(&__argc, __argv);
    // Recompile-only "-m2" / "-nom2" (and PZ_M2 / PZ_M2_TRACE / PZ_M2_CRC): M2
    // game logic in the menu world, src/game/m2trace.cpp. Default on.
    pz::M2ParseCommandLine(&__argc, __argv);
    // Recompile-only "-m3" (and PZ_M3 / PZ_M3_TRACE / PZ_M3_LOADMAP): the M3
    // Training Camp path, src/game/m3trace.cpp. Default off.
    pz::M3ParseCommandLine(&__argc, __argv);

    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExA(Settings.GetIniPath(), GetFileExInfoStandard, &fad)) {
        MessageBoxA(nullptr, "MSG_STARTUP_NO_INI", "Fatal error", MB_ICONERROR);
        _exit(1);
    }
    MakeDirectory(Settings.GetLogDir());  // 0x65fde0

    // HD: new SLogger(0x1c) 0x65c470(fullpath("<logdir>/panzers%08X.log"),
    // "Codename: Panzers - Phase One", 1, 1) -> g_Log 0x929f28.
    // TEMP: the SWINE-shared SLogger builds its own "log\<name> <time>.txt"
    // path, so only the base name is passed. Lifting 0x65c470 is open.
    char logName[64];
    _snprintf(logName, sizeof(logName) - 1, "panzers%08X", (unsigned)startTime);
    logName[sizeof(logName) - 1] = 0;
    Logger.g = new SLogger(logName, (char*)"Codename: Panzers - Phase One", true, 1);

    Logger.g->Log(0, "Command line: %s (argc %d, -nointro %d)", GetCommandLineA(), __argc,
                  (int)SSuperWindow::SkipIntro);
    PruneOldLogs(Settings.GetLogDir());

    // HD: new STimer (0xf8) 0x661520 -> 0x92e354. The SWINE engine uses the
    // global STimer object (constructed statically, src/panzers/gamemain.cpp).

    CoInitializeEx(nullptr, 0);
    RunGame();                        // 0x64c800 -> GameMain 0x64c870

    if (Logger.g) {
        delete Logger.g;              // 0x65c540
        Logger.g = nullptr;
    }
    CoUninitialize();
    // HD: FUN_00591150 (frees a global SString) — nothing to free here.
    return 0;
}
