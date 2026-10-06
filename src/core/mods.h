// src/core/mods.h
// Mod switches for the Panzers recompile.
//
// Every mod has its own compile-time switch, set by a CMake option of the same
// name and OFF by default. With every switch off the build is the faithful
// one: mod code is compiled out, and the faithful build's code and data
// sections must not change when a mod is added or edited. Mod code reaches
// the lifted code only through one-line hooks inside "#if <switch>" blocks.
//
//   PANZERS_MOD_WIDESCREEN  (CMake -DPANZERS_MOD_WIDESCREEN=ON)
//     Widescreen window: the 3D view keeps the 4:3 vertical field of view
//     and shows more at the sides (Hor+); the 2D menus keep their 4:3
//     aspect, the top and bottom bars are extended to the window width, the
//     logo and centred screens stay centred, and right-aligned menus stay at
//     the right edge. Mouse hit-testing follows the same layout.
//     Runtime toggle: panzers.ini [Mods] Widescreen = 0 turns it off
//     (default 1), which gives HD's stretched 1024x768 UI again.
//     Requested by the user as the first mod ("maybe first mod - widescreen
//     fix?"); HD itself stretches the 1024x768 UI and crops the 3D view.
//
//   PANZERS_MOD_BUGFIXES  (CMake -DPANZERS_MOD_BUGFIXES=ON)
//     Fixes for bugs of the original game, which the faithful build keeps:
//     - Closing the window (WM_CLOSE / title-bar X) on the Credits screen.
//       HD SSuperWindow::OnDestroy (0x65ab50) deletes every menu it owns
//       except CreditMenu (+0x100), so the credits widget is still the
//       window's child when ~SSuperWindow reaches ~SWidget (0x5430e0), which
//       panics "SWidget::~SWidget: Children widgets should be removed first"
//       (exit code 1). Seen on the HD PANZERS.exe itself. The fix deletes
//       CreditMenu with the other menus in OnDestroy.
//     - The same with the Training Camp dialog open (TrainingCampMenu
//       +0x118, also skipped by 0x65ab50; decided from the code, M3-I).
//       The fix deletes it too.

#ifndef PANZERS_MODS_H
#define PANZERS_MODS_H

#ifndef PANZERS_MOD_WIDESCREEN
#define PANZERS_MOD_WIDESCREEN 0
#endif

#ifndef PANZERS_MOD_BUGFIXES
#define PANZERS_MOD_BUGFIXES 0
#endif

#if PANZERS_MOD_WIDESCREEN
// Runtime toggle of the widescreen mod (panzers.ini [Mods] Widescreen).
// Defined in src/3dengine/pz/pzviewport.cpp (also linked by the tools);
// src/panzers/mod_widescreen.cpp reads it from panzers.ini.
extern bool g_ModWidescreen;
// Screen rectangle of an HD sub viewport (pixels) -> the rectangle in the
// game view's widget coordinates (the coordinates the game view passes to
// ScreenToRay / the box select and reads back from ProjectToScreen). Set by
// src/panzers/mod_widescreen.cpp; null or false: HD's pixel mapping.
extern bool (*g_ModWidescreenViewRect)(float* left, float* top, float* width, float* height);
#endif

#endif // PANZERS_MODS_H
