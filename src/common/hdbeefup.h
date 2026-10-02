#pragma once
// HD Beefup — Fine-grained debug flags
// Uncomment to enable debug logging for specific subsystems.
// These replace the blanket _DEBUG logging.

// --- Dev Shortcuts ---
/* HD_FAST_MENU compile-time fuse — superseded by runtime [Developer] SkipIntro and FastMenu in options.ini */

// --- HDBeefup Settings Menu ---
#define HD_HDBEEFUP_SETTINGS // HDBEEFUP settings tab in Options menu

// --- Gameplay Mods ---
//#define HD_HDBEEFUP_BRIDGES    // Swimming units can pass under bridges — WIP, disabled for now
#define HD_HDBEEFUP_TANKKILLER_SPECIALABILITY    // Nyul raketas pancelkocsi air defense toggle (ramp duration = SpecialTime in tankkiller.unit)
 #define HD_CARCONTROL                                  // Direct tank driving mode — NFS-style WASD control
 #define HD_HEADQUARTERS_3DMODEL                       // Live 3D model preview in headquarters market dialog
 #define HBD_ShowCTF //*/                                // Show Game Type dropdown (Deathmatch / CTF) in multiplayer lobby
 #define HD_ALLUNITSPICKER           // Show all units from both races in buy menus (SMarket + SQuickMarket)
 #define HD_SUBTITLES                // Show subtitles for unit voice lines (revived cut feature)
 #define HD_BULK_DELETE_SAVES        // Ctrl/Shift click in load/save list to mark many, Delete wipes all marked
 #define HD_ISO_DATE_SAVES           // Format savegame dates as YYYY-MM-DD instead of locale short date
 //#define HD_DEMO_EXIT_SCREENS        // Click-to-cycle through 4 exit images before quitting (2001 demo build style)
 #define HD_KEYBINDS                 // Rebindable keyboard controls (Classic / WASD / Custom) — SGameView dispatch table
 #define HDB_MODLOADER_SYSTEM        // mods/<name>/ + langpack/<code>/ drop-in loader (units.ini overrides, .unit patches, fan translations)
 #define HD_TANK_PATHFIND_FIX        // SUnit::ContinueMoving i==2: return after recursive call so tracked vehicles don't StopAndBlock over the recursion's fresh ghost/driver state (original shipped binary falls through — confirmed at 004B4DDF)
 #define HD_RMB_CAMERA_PAN           // Hold right mouse button with nothing selected to pan the camera (inspired by CPCW); avoids needing to push against screen edges
 #define HD_HDBEEFUP_AIRCHEATS       // `mo bombers` / `mo heli` chat cheats — bypass single-bomber/heli cap and bomber-shotdown lockout
 #define HD_HDBEEFUP_AUDIO_DEBUG     // Show every PlaySound() filename in the in-game message HUD (Echo system) — useful for diagnosing speech-loop bugs without grepping the log
 #define HDB_MISSING_ASSET_FALLBACK   // editor/missing.{4d,dxt} substitution + procedural .png/.tga checker + softened effect-mesh panics. Comment out to restore byte-identical original panics for reccmp parity work.
 //#define HDB_LIGHT_OCCLUSION       // Headlight cone (reflektor) + headlight/taillight glow sprites: keep depth-test on (so terrain/buildings/units occlude them) and sample glow positions per-frame from interpolated mesh nodes (so glows track the unit smoothly instead of teleporting each sim tick). DISABLED — see prompts/PROMPT_unit_lights_decal_smooth.md for unfinished work (decal-following cone, residual glow teleport, machinima-mode crash suspicion). Keep gated code for future screenshot/alt-build use.

// --- Engine Config ---
 #define HD_TRAIL_INI              // Load trail.ini from exe directory (debug level, search paths, pak files)
 #define HD_PORTABLE_PATHS         // Store logs, savegames, options next to exe instead of %LOCALAPPDATA%

// --- [DEBUG JUNK] ---
// --- Renderer / GPU ---
// #define HD_DEBUG_RENDERER      // Device creation, present params, scene render, textures
// #define HD_DEBUG_SHADERS       // Shader loading, init, standard[] pointers
// #define HD_DEBUG_TERRAIN       // Tileset init, parcel drawing, visibility culling

// --- World / Map ---
// #define HD_DEBUG_WORLD_LOAD    // Map file loading, signature, chunks, HEMP/LITE
// #define HD_DEBUG_DOODADS       // Doodad stream loading, position, heap validation
// #define HD_DEBUG_WORLD_TEARDOWN // ~SWorld destructor cleanup phases
// #define HD_DEBUG_MENU_CAM      // Menu background camera (cameras.ini override + .scene CHM chunk) — x86/x64 parity bisect

// --- Game Logic ---
// #define HD_DEBUG_SERVERINIT    // ServerInit pipeline (pathbuffer, tileset, vis/block maps)
// #define HD_DEBUG_PATHFINDING   // FindPath bounds, unit size, path result
// #define HD_DEBUG_VISMAP        // CreateVisMap height/ray processing

// --- Units ---
// #define HD_DEBUG_UNITS         // Unit pricing, model loading, weapons, fuel, skin objects

// --- UI / Menus ---
// #define HD_DEBUG_FONTS         // Font loading (NotoSans, fixed, custom), glyph metrics
// #define HD_DEBUG_MENUS         // Menu loading, game start/end, superwindow init
// #define HD_DEBUG_BOARD         // Board rendering, cursor setup, sprite/text draw

// --- Audio ---
// #define HD_DEBUG_AUDIO         // MP3 decoder init, SWave loading, streaming playback
// 
// --- Misc ---
// #define HD_DEBUG_EFFECTS       // SEffect lifecycle, PlayEffect
// #define HD_DEBUG_MESHES        // SGroup dynamic draw, mesh replace vtable workarounds
// #define HD_DEBUG_EDITOR        // Editor-only: OnPaint, resize, toolbar init, file ops
// #define HD_DEBUG_PLATFORM      // GOG Galaxy auth

// Master switch: uncomment to enable ALL flags
// #define HD_DEBUG_ALL
#ifdef HD_DEBUG_ALL
#define HD_DEBUG_RENDERER
#define HD_DEBUG_SHADERS
#define HD_DEBUG_TERRAIN
#define HD_DEBUG_WORLD_LOAD
#define HD_DEBUG_DOODADS
#define HD_DEBUG_WORLD_TEARDOWN
#define HD_DEBUG_MENU_CAM
#define HD_DEBUG_SERVERINIT
#define HD_DEBUG_PATHFINDING
#define HD_DEBUG_VISMAP
#define HD_DEBUG_UNITS
#define HD_DEBUG_FONTS
#define HD_DEBUG_MENUS
#define HD_DEBUG_BOARD
#define HD_DEBUG_AUDIO
#define HD_DEBUG_EFFECTS
#define HD_DEBUG_MESHES
#define HD_DEBUG_EDITOR
#define HD_DEBUG_PLATFORM
#define HD_ALLUNITSPICKER
#endif

// --- Bot Mode ---
/* #define HD_BOT_MODE */             // Headless multiplayer bot — enables null-safe UI paths in SMulti

// --- Machinima Mode (decompilation-exclusive) ---
// Set by SGameView's F10 cycle when in deep state. Read by SUnit::ClientRefresh
// (suppresses per-unit overlay: brackets, HP bar, group #, ammo/fuel) and by
// SWorld::ShowUnitRange (suppresses range circle decals). Defined in world.cpp.
extern bool g_HideWorldOverlays;

// --- Network Servers ---
#define HD_MATCHMAKING_SERVER "mm.kite-games.com"
#define HD_NAT_SERVER         "nat.kite-games.com"

// --- GameSpy 2001 Backend ---
// Optional secondary matchmaking backend that talks peerchat IRC + master + heartbeat
// to a GameSpy-protocol server (see serverinfra/gamespy2001/). RakNet is still used
// for in-game P2P; only the lobby/chat layer changes. Reuses 2001 room name
// (#GSP!swine) and gamename ("swine") so HD and 2001 classic share the same lobby.
#define HD_GAMESPY_PEERCHAT_SERVER "peerchat.swine.fans"
#define HD_GAMESPY_MASTER_SERVER   "master.swine.fans"

#define HD_MATCHMAKING_BACKEND_HD      0
#define HD_MATCHMAKING_BACKEND_GAMESPY 1
// Compile-time backend switch. Default = stock Kite TCP matchmaker.
// Set to HD_MATCHMAKING_BACKEND_GAMESPY to route lobby through the GameSpy server.
#ifndef HD_MATCHMAKING_BACKEND
#define HD_MATCHMAKING_BACKEND HD_MATCHMAKING_BACKEND_GAMESPY
#endif
