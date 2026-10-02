// core/properties.h
// SProperties — INI/config file parser
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef CORE_PROPERTIES_H
#define CORE_PROPERTIES_H

#include "core_common.h"

struct SProperty {
    char* name;
    char* value;
    SProperty* next;
};

struct SPropertyClass {
    char* name;
    SProperty* chain;
    SPropertyClass* next;
};

struct SProperties {
    SPropertyClass* chain;
    SPropertyClass* pc_enum;
    SProperty* p_enum;

    SProperties(const char* filename, bool ExitOnReadError, bool trimSpaceAtEndOfValue);
    ~SProperties();
    void EnumProperties(const char* classname);
    void EnumPropertyClasses();
    float GetFloat(const char* classname, const char* propname, float def);
    int GetInt(const char* classname, const char* propname, int def);
    char* GetNextProperty();
    char* GetNextPropertyClass();
    char* GetString(const char* classname, const char* propname, const char* def);
    SPropertyClass* LookupClass(const char* name);
    SProperty* LookupVariable(SPropertyClass* pclass, const char* name);
    void ParseFileData(char* buf, char* end, bool trimSpaceAtEndOfValue);

    // Section-level replace: for each [Section] in `filename`, drop any
    // existing same-named section (and its properties) in this object,
    // then splice the parsed section in at the tail. New sections are
    // appended. Missing files are a no-op. Used by the mod loader so a
    // mod's units.ini / .unit file does not spill unrelated keys from
    // the base definition.
    void OverrideFromFile(const char* filename);
};

#endif // CORE_PROPERTIES_H
