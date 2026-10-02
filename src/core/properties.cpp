// core/properties.cpp
// INI/config file parser
// Decompiled from: gameSplit/sfilesystem.c

#include "properties.h"
#include "logger.h"
#include "stream.h"

//----- (004FF2C0) --------------------------------------------------------

SProperties::SProperties(const char* filename, bool ExitOnReadError, bool trimSpaceAtEndOfValue)
{
    const char* caller = ExitOnReadError ? "SProperties::SProperties" : nullptr;
    this->chain = nullptr;
    char* buf = nullptr;
    unsigned int fsize = 0;
    FileSystem.ReadFile(filename, &buf, &fsize, caller);
    ParseFileData(buf, &buf[fsize], trimSpaceAtEndOfValue);
    free(buf);
    this->pc_enum = nullptr;
    this->p_enum = nullptr;
}

//----- (004FF340) --------------------------------------------------------

SProperties::~SProperties()
{
    SPropertyClass* pc = this->chain;
    while (pc) {
        SProperty* prop = pc->chain;
        SPropertyClass* next = pc->next;
        while (prop) {
            SProperty* pnext = prop->next;
            operator delete(prop->name);
            operator delete(prop->value);
            operator delete(prop);
            prop = pnext;
        }
        operator delete(pc->name);
        operator delete(pc);
        pc = next;
    }
}

//----- (004FF3B0) --------------------------------------------------------

void SProperties::EnumProperties(const char* classname)
{
    SPropertyClass* pc = this->chain;
    while (pc) { if (_stricmp(pc->name, classname) == 0) break; pc = pc->next; }
    this->p_enum = pc ? pc->chain : nullptr;
}

//----- (004FF400) --------------------------------------------------------

void SProperties::EnumPropertyClasses() { this->pc_enum = this->chain; }

//----- (004FF410) --------------------------------------------------------

float SProperties::GetFloat(const char* classname, const char* propname, float def)
{
    SPropertyClass* pc = this->chain;
    while (pc) { if (_stricmp(pc->name, classname) == 0) break; pc = pc->next; }
    if (!pc) return def;
    SProperty* prop = pc->chain;
    while (prop) { if (_stricmp(prop->name, propname) == 0) break; prop = prop->next; }
    if (!prop) return def;
    setlocale(LC_NUMERIC, "C");
    return (float)strtod(prop->value, nullptr);
}

//----- (004FF4A0) --------------------------------------------------------

int SProperties::GetInt(const char* classname, const char* propname, int def)
{
    SPropertyClass* pc = this->chain;
    while (pc) { if (_stricmp(pc->name, classname) == 0) break; pc = pc->next; }
    if (!pc) return def;
    SProperty* prop = pc->chain;
    while (prop) { if (_stricmp(prop->name, propname) == 0) break; prop = prop->next; }
    if (!prop) return def;
    return strtol(prop->value, nullptr, 0);
}

//----- (004FF520) --------------------------------------------------------

char* SProperties::GetNextProperty()
{
    SProperty* p = this->p_enum;
    if (!p) return nullptr;
    this->p_enum = p->next;
    return p->name;
}

//----- (004FF540) --------------------------------------------------------

char* SProperties::GetNextPropertyClass()
{
    SPropertyClass* pc = this->pc_enum;
    if (!pc) return nullptr;
    this->pc_enum = pc->next;
    return pc->name;
}

//----- (004FF560) --------------------------------------------------------

char* SProperties::GetString(const char* classname, const char* propname, const char* def)
{
    SPropertyClass* pc = this->chain;
    while (pc) { if (_stricmp(pc->name, classname) == 0) break; pc = pc->next; }
    if (!pc) return (char*)def;
    SProperty* prop = pc->chain;
    while (prop) { if (_stricmp(prop->name, propname) == 0) break; prop = prop->next; }
    if (!prop) return (char*)def;
    return prop->value;
}

//----- (004FF5D0) --------------------------------------------------------

SPropertyClass* SProperties::LookupClass(const char* name)
{
    SPropertyClass* pc = this->chain;
    while (pc) { if (_stricmp(pc->name, name) == 0) return pc; pc = pc->next; }
    return nullptr;
}

//----- (004FF610) --------------------------------------------------------

SProperty* SProperties::LookupVariable(SPropertyClass* pclass, const char* name)
{
    if (!pclass) return nullptr;
    SProperty* prop = pclass->chain;
    while (prop) { if (_stricmp(prop->name, name) == 0) return prop; prop = prop->next; }
    return nullptr;
}

//----- (004FF660) --------------------------------------------------------

void SProperties::ParseFileData(char* buf, char* end, bool trimSpaceAtEndOfValue)
{
    char* p = buf;
    SPropertyClass* currentClass = nullptr;
    SPropertyClass* lastClass = nullptr;
    SProperty* lastProp = nullptr;
    if (p && *p == (char)0xEF && p[1] == (char)0xBB && p[2] == (char)0xBF) p = buf + 3;
    while (p < end) {
        char c = *p;
        if (c == '\r' || c == '\n' || c == '\t' || c == ' ') { ++p; continue; }
        if (c == ';') { while (p < end) { if (*p++ == '\n') break; } continue; }
        if (c == '[') {
            char* nameStart = p + 1;
            if (nameStart >= end) Logger.g->Panic("SProperties::ParseFileData: SyntaxError, missing ']'");
            char* nameEnd = nameStart;
            while (nameEnd < end) {
                if (*nameEnd == '\n') Logger.g->Panic("SProperties::ParseFileData: SyntaxError, missing ']'");
                if (*nameEnd == ']') break;
                ++nameEnd;
            }
            if (nameEnd >= end) Logger.g->Panic("SProperties::ParseFileData: SyntaxError, missing ']'");
            if (nameEnd <= nameStart) Logger.g->Panic("SProperties::ParseFileData: Empty section name");
            *nameEnd = 0; p = nameEnd + 1;
            if (LookupClass(nameStart)) Logger.g->Panic("SProperties::ParseFileData: Duplicate section (%s)", nameStart);
            SPropertyClass* nc = (SPropertyClass*)operator new(sizeof(SPropertyClass));
            if (!this->chain) { lastClass = nc; this->chain = nc; }
            else { SPropertyClass* prev = lastClass; lastClass = nc; prev->next = nc; }
            nc->name = _strdup(nameStart); nc->chain = nullptr; nc->next = nullptr;
            currentClass = nc;
            Logger.g->Log(2, "SProperties::ParseFileData: Class (%s)", nc->name);
#ifdef HDB_PROPERTIES_TRACE
            Logger.g->Log(1, "PTRACE: pc %p name=%p [%s]", nc, nc->name, nc->name);
#endif
            while (p < end) {
                char ch = *p;
                if (ch == '\n') break;
                if (ch != '\r' && ch != ' ' && ch != '\t')
                    Logger.g->Panic("SProperties::ParseFileData: SyntaxError, garbage after section name");
                ++p;
            }
            continue;
        }
        if (!currentClass) Logger.g->Panic("SProperties::ParseFileData: Variable without section definition");
        char* varStart = p;
        while (p < end) {
            if (*p == '\n') Logger.g->Panic("SProperties::ParseFileData: SyntaxError, missing '='");
            if (*p == '=') break;
            ++p;
        }
        if (p >= end) Logger.g->Panic("SProperties::ParseFileData: SyntaxError, missing '='");
        char* eqPos = p;
        if (eqPos <= varStart) Logger.g->Panic("SProperties::ParseFileData: Empty variable name");
        while (eqPos > varStart && (*(eqPos-1) == ' ' || *(eqPos-1) == '\t')) --eqPos;
        if (eqPos <= varStart) Logger.g->Panic("SProperties::ParseFileData: Empty variable name");
        char* valStart = p + 1; *eqPos = 0;
        if (LookupVariable(lastClass, varStart))
            Logger.g->Panic("SProperties::ParseFileData: Dublicate variable (%s::%s)", lastClass->name, varStart);
        SProperty* np = (SProperty*)operator new(sizeof(SProperty));
        if (!lastClass->chain) { lastProp = np; lastClass->chain = np; }
        else { SProperty* prev = lastProp; lastProp = np; prev->next = np; }
        np->name = _strdup(varStart); np->next = nullptr;
        while (valStart < end && (*valStart == ' ' || *valStart == '\t')) ++valStart;
        char* valEnd = valStart;
        while (valEnd < end) { char vc = *valEnd; if (vc == '\r' || vc == '\n') break; if (vc == '#') *valEnd = '\n'; ++valEnd; }
        char* trimEnd = valEnd;
        if (trimSpaceAtEndOfValue && trimEnd > valStart)
            while (trimEnd > valStart && (*(trimEnd-1) == ' ' || *(trimEnd-1) == '\t')) --trimEnd;
        if (valEnd < end) { *trimEnd = 0; }
        else {
            if (trimEnd - valStart > 0) memmove(valStart - 1, valStart, trimEnd - valStart);
            --valStart; *(trimEnd - 1) = 0;
        }
        p = valEnd + 1;
        np->value = _strdup(valStart);
        Logger.g->Log(2, "SProperties::ParseFileData:   Variable (%s = %s)", lastProp->name, np->value);
#ifdef HDB_PROPERTIES_TRACE
        Logger.g->Log(1, "PTRACE: prop %p [%s] name=%p(%s) value=%p(%s)",
                      np, lastClass->name, np->name, np->name, np->value, np->value);
#endif
        currentClass = lastClass;
    }
    Logger.g->Log(2, "SProperties::ParseFileData: ParseSuccessful");
}

void SProperties::OverrideFromFile(const char* filename)
{
    SProperties temp(filename, false, true);
    SPropertyClass* pc = temp.chain;
    while (pc) {
        SPropertyClass* next = pc->next;
        pc->next = nullptr;

        SPropertyClass** pp = &this->chain;
        while (*pp) {
            if (_stricmp((*pp)->name, pc->name) == 0) {
                SPropertyClass* dead = *pp;
                *pp = dead->next;
                SProperty* prop = dead->chain;
                while (prop) {
                    SProperty* pnext = prop->next;
                    operator delete(prop->name);
                    operator delete(prop->value);
                    operator delete(prop);
                    prop = pnext;
                }
                operator delete(dead->name);
                operator delete(dead);
                break;
            }
            pp = &(*pp)->next;
        }

        SPropertyClass** tail = &this->chain;
        while (*tail) tail = &(*tail)->next;
        *tail = pc;

        Logger.g->Log(2, "SProperties::OverrideFromFile: [%s] from (%s)", pc->name, filename);
        pc = next;
    }
    temp.chain = nullptr;
    temp.pc_enum = nullptr;
    temp.p_enum = nullptr;
}
