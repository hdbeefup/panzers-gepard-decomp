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

#ifndef PANZERS_MODS_H
#define PANZERS_MODS_H

#ifndef PANZERS_MOD_WIDESCREEN
#define PANZERS_MOD_WIDESCREEN 0
#endif

#if PANZERS_MOD_WIDESCREEN
// Runtime toggle of the widescreen mod (panzers.ini [Mods] Widescreen).
// Defined in src/3dengine/pz/pzviewport.cpp (also linked by the tools);
// src/panzers/mod_widescreen.cpp reads it from panzers.ini.
extern bool g_ModWidescreen;
#endif

#endif // PANZERS_MODS_H
