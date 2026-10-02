// core/properties.h
// SProperties — INI/config file parser
// PANZERS: re-checked against the Codename Panzers HD exe. Panzers keeps
// sections and variables in arrays sorted by name (_stricmp), searched by
// binary search, with a cursor in the object. SWINE kept linked lists in
// file order. See docs/FORMATS.md.

#ifndef CORE_PROPERTIES_H
#define CORE_PROPERTIES_H

#include "core_common.h"
#include "string2.h"
#include <stddef.h>

// HD SDArray element 0x10 (type name at 0x8ec4fc)
struct SPropertyVariable {
    SString Name;               // 0x00
    SString Value;              // 0x08
};

// HD SDArray element 0x14 (type name at 0x8ec4dc)
struct SPropertySection {
    SString Name;               // 0x00
    SPropertyVariable* Vars;    // 0x08  HD SDArray order {array, size, maxsize}
    int VarCount;               // 0x0C
    int VarMax;                 // 0x10
};

// HD: operator new(0x1c), ctor 0x65fe80
struct SProperties {
    SPropertySection* Sections; // 0x00
    int SectionCount;           // 0x04
    int SectionMax;             // 0x08
    SString FileName;           // 0x0C: used in ParseFileData panics
    int CurSection;             // 0x14: FindSection result / insert point / enum cursor
    int CurVariable;            // 0x18: FindVariable result / insert point

    // trimSpaceAtEndOfValue is SWINE-only and ignored: Panzers always trims.
    SProperties(const char* filename, bool ExitOnReadError, bool trimSpaceAtEndOfValue = true); // 0x65fe80
    ~SProperties();                                                     // 0x660080

    void Load(const char* filename, bool ExitOnReadError);              // 0x6607d0
    void ParseFileData(char* buf, char* end);                           // 0x660840
    bool FindSection(const char* name);                                 // 0x6601c0
    bool FindVariable(const char* name);                                // 0x6602c0
    bool Exists(const char* classname, const char* propname);           // 0x6607a0
    float GetFloat(const char* classname, const char* propname, float def);     // 0x660490
    int GetInt(const char* classname, const char* propname, int def);           // 0x660500
    char* GetString(const char* classname, const char* propname, const char* def); // 0x660570
    void EnumPropertyClasses();                                         // 0x660150
    char* GetPropertyClass();                                           // 0x660160
    void NextPropertyClass();                                           // 0x6601b0
    SPropertySection* GetSection(int index);                            // 0x6600b0
    SPropertyVariable* GetVariable(SPropertySection* section, int index); // 0x660100
    int InsertSection(int index);                                       // 0x6605d0
    int InsertVariable(SPropertySection* section, int index);           // 0x6606c0

    // SWINE API, kept for the engine callers. These enumerators use their own
    // cursor, so a GetString() inside the loop does not disturb them.
    char* GetNextPropertyClass();
    void EnumProperties(const char* classname);
    char* GetNextProperty();

    // Section-level replace used by the SWINE mod loader: each [Section] in
    // `filename` replaces a same-named section here. Missing files are a no-op.
    void OverrideFromFile(const char* filename);
};

#if defined(_M_IX86)
static_assert(sizeof(SPropertyVariable) == 0x10, "HD 0x6606c0 stride 0x10");
static_assert(sizeof(SPropertySection) == 0x14, "HD 0x6605d0 stride 0x14");
static_assert(offsetof(SPropertySection, VarCount) == 0x0C, "HD 0x6602c0 +0xC");
static_assert(sizeof(SProperties) == 0x1C, "HD operator new(0x1c)");
static_assert(offsetof(SProperties, SectionCount) == 0x04, "HD 0x660500 +4");
static_assert(offsetof(SProperties, FileName) == 0x0C, "HD 0x660840 +0xC");
static_assert(offsetof(SProperties, CurSection) == 0x14, "HD 0x6601c0 param_1[5]");
static_assert(offsetof(SProperties, CurVariable) == 0x18, "HD 0x6602c0 param_1[6]");
#endif

#endif // CORE_PROPERTIES_H
