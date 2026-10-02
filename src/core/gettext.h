// core/gettext.h
// PANZERS: the menu.ini string table ("prepare_gettext"), lifted from the
// Codename Panzers HD exe. See docs/FORMATS.md.
//
// menu.ini maps an English source string to its translation, one section
// per source file:
//   [panzers/MainMenu.cpp]
//   id_000 = "Single Player"
//   str000 = "Einzelspieler"
// The game looks strings up as GetText("panzers/MainMenu.cpp", "Single Player").

#ifndef CORE_GETTEXT_H
#define CORE_GETTEXT_H

#include "core_common.h"
#include "string2.h"
#include <stddef.h>

struct SGetTextEntry {          // HD SDArray element 0x10 (type name at 0x8ec4fc)
    SString Id;                 // 0x00: unescaped id_NNN
    SString Str;                // 0x08: unescaped strNNN
};

struct SGetTextSection {        // HD SDArray element 0x14 (type name at 0x8ec4dc)
    SString Name;               // 0x00: ini section name, e.g. "panzers/MainMenu.cpp"
    SGetTextEntry* Entries;     // 0x08  HD SDArray order {array, size, maxsize}
    int Count;                  // 0x0C
    int Max;                    // 0x10
};

struct SGetTextTable {          // HD global at 0x92e344
    SGetTextSection* Sections;  // 0x92e344
    int Count;                  // 0x92e348
    int Max;                    // 0x92e34c
};

extern SGetTextTable GetTextTable;

// HD 0x660db0. Loads `filename` (menu.ini) through SProperties and appends
// every section to GetTextTable. Called once from SSuperWindow's ctor.
void PrepareGetText(const char* filename);

// HD 0x660c50. Returns the translation, or `id` itself when the section or
// the id is not in the table.
char* GetText(const char* section, const char* id);

// SWINE engine API (window/widget.h): one key, no section. Not in
// PANZERS.exe. Searches every section in table order, else returns the key.
char* GetText(const char* key);

#if defined(_M_IX86)
static_assert(sizeof(SGetTextEntry) == 0x10, "HD 0x660be0 stride 0x10");
static_assert(sizeof(SGetTextSection) == 0x14, "HD 0x660db0 stride 0x14");
static_assert(offsetof(SGetTextSection, Count) == 0x0C, "HD 0x660c50 [i*5+3]");
static_assert(sizeof(SGetTextTable) == 0x0C, "HD 0x92e344..0x92e350");
#endif

#endif // CORE_GETTEXT_H
