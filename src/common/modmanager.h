// common/modmanager.h
// SModManager — enumerates mods/<name>/ and langpack/<code>/ directories
// at the game root, exposes dropdown-friendly lists, and tracks the
// currently active selection. Non-original: gated behind HDB_MODLOADER_SYSTEM.

#ifndef COMMON_MODMANAGER_H
#define COMMON_MODMANAGER_H

#include "hdbeefup.h"

#ifdef HDB_MODLOADER_SYSTEM

#include "string2.h"

struct SModInfo {
    SString FolderName;     // relative folder name under mods/
    SString DisplayName;    // human-readable name for UI (from mod.ini or folder)
};

struct SLangPackInfo {
    SString FolderCode;     // e.g. "LT"
    SString DisplayName;    // native name read from langpack's own messages.ini
    SString BaseLang;       // fallback language code ("EN" etc.) for missing strings
};

struct SModManager {
    SModInfo* Mods;
    int ModCount;
    int ModCapacity;

    SLangPackInfo* LangPacks;
    int LangPackCount;
    int LangPackCapacity;

    // Scratch for enumerated *.unit files of the active mod.
    SString* UnitFiles;
    int UnitFileCount;
    int UnitFileCapacity;

    // Ordered list of active mods. Slot 0 = LOWEST priority (loaded
    // first), slot N-1 = HIGHEST priority (loaded last, overrides above).
    // Mirrors the user-visible top-to-bottom order in the Mod Manager UI.
    SString* ActiveModFolders;
    int ActiveModFolderCount;
    int ActiveModFolderCapacity;

    // Currently active langpack folder code ("" = none).
    SString ActiveLangPack;

    SModManager();
    ~SModManager();

    void EnumerateMods();
    void EnumerateLangPacks();

    // Replace the active-mod stack with this priority-ordered list.
    // folderNames[0] is highest priority. count == 0 (or all empties) = base game.
    void SetActiveMods(const char* const* folderNames, int count);
    void SetActiveMod(const char* folderName);       // wrapper around SetActiveMods
    void SetActiveLangPack(const char* folderCode);  // "" or nullptr → none

    int GetActiveModCount() const;
    const char* GetActiveModFolderAt(int index) const;  // index 0 = highest priority
    const char* GetActiveModFolder() const;             // legacy alias for slot 0
    const char* GetActiveLangPackCode() const;
    bool HasActiveMod() const;
    bool HasActiveLangPack() const;

    // Resolve a folder name to the index in Mods[] (-1 if not found).
    int FindModIndex(const char* folderName) const;
    int FindLangPackIndex(const char* folderCode) const;

    // Rescan all active mods' folders for *.unit files. Called when the
    // active set changes. Files are collected lowest-priority first so
    // that OverrideFromFile (later wins) ends up with highest-priority
    // last and therefore wins.
    void RefreshActiveModUnitFiles();
};

extern SModManager ModManager;

#endif // HDB_MODLOADER_SYSTEM
#endif // COMMON_MODMANAGER_H
