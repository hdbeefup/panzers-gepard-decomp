// src/panzers/mod_widescreen.h
// MOD_WIDESCREEN (see src/core/mods.h): hook functions called from
// SSuperWindow inside "#if PANZERS_MOD_WIDESCREEN" blocks. Not part of the
// faithful build.

#ifndef PANZERS_MOD_WIDESCREEN_H
#define PANZERS_MOD_WIDESCREEN_H

#include "mods.h"

#if PANZERS_MOD_WIDESCREEN

struct SSuperWindow;
struct SProperties;

// Reads the runtime toggle, panzers.ini [Mods] Widescreen (default 1).
void ModWidescreenInit(SProperties* ini);
// After HD's root-scaler setup (Create) and resize (OnSize): gives the root
// scaler a virtual width that keeps the UI at 4:3 (uniform scale).
void ModWidescreenOnSize(SSuperWindow* w);
// Client point -> virtual point for the widescreen layout. Returns false
// when the mod is off or the window is not wider than 4:3 (HD mapping).
bool ModWidescreenEventPoint(SSuperWindow* w, int x, int y, int* vx, int* vy);
// Once per frame: anchors the top-level menus (right-aligned ones to the
// right edge, the rest centred) and extends the top/bottom bars.
void ModWidescreenFrame(SSuperWindow* w);
// Before the board is destroyed.
void ModWidescreenShutdown();
// A rectangle of the 1024x768 design, on a centred screen (the HQ), in
// window pixels: where the anchored layout draws it.
bool ModWidescreenDesignRect(SSuperWindow* w, int* x, int* y, int* width, int* height);

#endif

#endif // PANZERS_MOD_WIDESCREEN_H
