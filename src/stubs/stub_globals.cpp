// src/stubs/stub_globals.cpp
// Data stubs: globals the imported engine code declares `extern` whose
// definitions lived in SWINE gameplay code (game/game.cpp, world/world.cpp,
// network/multi.cpp). They are zero/null-initialised here; the lifted Panzers
// WinMain / game layer will own (and fill) them later, at which point each
// definition must MOVE out of this file.
//
// Data stubs are not functions, so they carry no STUB_LOG and are not counted
// by the census; they are listed in docs/IMPORT_NOTES.md.

#include <windows.h>
#include "timer.h"
#include "chain.h"
#include "widget.h"

struct SIBoard;
struct SIGepard;
struct SIConcert;
struct SOptions;
struct SWindow;

// Engine singletons (SWINE: game/game.cpp)
SIBoard*   Board     = nullptr;
SIGepard*  Gepard    = nullptr;
SIConcert* Concert   = nullptr;
SOptions*  Options   = nullptr;
STimer     Timer;
SWindow*   TheWindow = nullptr;
HINSTANCE  hInstance = nullptr;

// UI / menu state (SWINE: game/game.cpp). PlayerColors is per-game data;
// Panzers' table must be lifted from PANZERS.exe — zero until then.
unsigned int  PlayerColors[13] = { 0 };
unsigned char g_MenuRace       = 0;
int           CurrentLanguage  = 0;

// Widget timer table + capture state (SWINE: game/game.cpp)
SHeap<TimerItem> TimerList = { NULL, 0, 0, -1, 0 };
SWidget* SWidget::CaptureTarget   = nullptr;
SWidget* SWidget::LastMouseTarget = nullptr;

// HD overlay toggle (SWINE: world/world.cpp), declared in common/hdbeefup.h
bool g_HideWorldOverlays = false;

// SMulti::instance (SWINE: network/multi.cpp). window/playerlistbox.cpp
// declares its own minimal SMulti; this definition must match it exactly.
struct SMulti {
    static SMulti* instance;
    char m_strLocalPlayerName[260];
};
SMulti* SMulti::instance = nullptr;
