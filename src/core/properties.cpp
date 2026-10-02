// core/properties.cpp
// INI/config file parser
// Imported from the S.W.I.N.E. decomp (gameSplit/sfilesystem.c) and
// re-lifted from the Codename Panzers HD exe (0x65fe80..0x660840).
//
// Panzers differences from SWINE:
// - Sections and variables are kept in name-sorted arrays (_stricmp) and
//   found by binary search; enumeration yields sections in sorted order.
// - A UTF-8 BOM switches on in-place UTF-8 -> Latin-1 decoding of the rest
//   of the file (2-byte sequences only). SWINE only skipped the BOM.
// - '#' in a value is kept. SWINE turned it into a line end.
// - Values are always right-trimmed. Panics name the file.

#include "properties.h"
#include "logger.h"
#include "stream.h"

static void FreeStr(SString* s)
{
    if (s->buf) {
        delete[] s->buf;
        s->buf = nullptr;
    }
    s->size = 0;
}

static void PropertiesResetClassEnum(SProperties* p);

static const char* FileNameOf(const SProperties* p)
{
    return p->FileName.buf ? p->FileName.buf : "";
}

// PANZERS 0x65fe80
SProperties::SProperties(const char* filename, bool ExitOnReadError, bool trimSpaceAtEndOfValue)
{
    (void)trimSpaceAtEndOfValue;
    Sections = nullptr;
    SectionCount = 0;
    SectionMax = 0;
    FileName.buf = nullptr;
    FileName.size = 0;
    CurSection = -1;
    CurVariable = 0;
    Load(filename, ExitOnReadError);
}

// PANZERS 0x660080
// (frees the sections through 0x65ff70 and each variable array through 0x65fff0)
SProperties::~SProperties()
{
    FreeStr(&FileName);
    for (int i = 0; i < SectionCount; i++) {
        SPropertySection* sec = &Sections[i];
        for (int j = 0; j < sec->VarCount; j++) {
            FreeStr(&sec->Vars[j].Value);
            FreeStr(&sec->Vars[j].Name);
        }
        if (sec->Vars) {
            free(sec->Vars);
            sec->Vars = nullptr;
        }
        sec->VarMax = 0;
        sec->VarCount = 0;
        FreeStr(&sec->Name);
    }
    if (Sections) {
        free(Sections);
        Sections = nullptr;
    }
    SectionMax = 0;
    SectionCount = 0;
}

// PANZERS 0x6607d0
// Load() adds to what is already there; a section present twice panics.
void SProperties::Load(const char* filename, bool ExitOnReadError)
{
    FileName = filename;
    char* buf = nullptr;
    unsigned int size = 0;
    FileSystem.ReadFile(filename, &buf, &size, ExitOnReadError ? "SProperties::Load" : nullptr);
    ParseFileData(buf, buf + size);
    CurSection = -1;
    if (buf)
        operator delete[](buf);
}

// PANZERS 0x6600b0
SPropertySection* SProperties::GetSection(int index)
{
    if (index < 0 || index >= SectionCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SPropertySection", index);
    return &Sections[index];
}

// PANZERS 0x660100
SPropertyVariable* SProperties::GetVariable(SPropertySection* section, int index)
{
    if (index < 0 || index >= section->VarCount)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SPropertyVariable", index);
    return &section->Vars[index];
}

// PANZERS 0x6605d0
int SProperties::InsertSection(int index)
{
    if (index < 0 || index > SectionCount)
        Logger.g->Panic("SDArray<%s>::Insert: invalid index (%d)", "struct SPropertySection", index);
    if (SectionCount == SectionMax) {
        int newmax = (SectionMax < 16) ? 16 : (SectionMax * 6) / 5;
        Sections = (SPropertySection*)realloc(Sections, newmax * sizeof(SPropertySection));
        memset(&Sections[SectionMax], 0, (newmax - SectionMax) * sizeof(SPropertySection));
        SectionMax = newmax;
    }
    if (index < SectionCount)
        memmove(&Sections[index + 1], &Sections[index], (SectionCount - index) * sizeof(SPropertySection));
    memset(&Sections[index], 0, sizeof(SPropertySection));
    SectionCount++;
    return index;
}

// PANZERS 0x6606c0
int SProperties::InsertVariable(SPropertySection* section, int index)
{
    if (index < 0 || index > section->VarCount)
        Logger.g->Panic("SDArray<%s>::Insert: invalid index (%d)", "struct SPropertyVariable", index);
    if (section->VarCount == section->VarMax) {
        int newmax = (section->VarMax < 16) ? 16 : (section->VarMax * 6) / 5;
        section->Vars = (SPropertyVariable*)realloc(section->Vars, newmax * sizeof(SPropertyVariable));
        memset(&section->Vars[section->VarMax], 0, (newmax - section->VarMax) * sizeof(SPropertyVariable));
        section->VarMax = newmax;
    }
    if (index < section->VarCount)
        memmove(&section->Vars[index + 1], &section->Vars[index],
                (section->VarCount - index) * sizeof(SPropertyVariable));
    memset(&section->Vars[index], 0, sizeof(SPropertyVariable));
    section->VarCount++;
    return index;
}

// PANZERS 0x6601c0
// Checks the cached CurSection first, then binary-searches. On a miss,
// CurSection is left at the insertion point.
bool SProperties::FindSection(const char* name)
{
    if (CurSection >= 0 && CurSection < SectionCount) {
        const char* cur = Sections[CurSection].Name.buf ? Sections[CurSection].Name.buf : "";
        if (_stricmp(cur, name) == 0)
            return true;
    }
    int lo = 0;
    int hi = SectionCount - 1;
    while (lo <= hi) {
        int mid = (hi + lo) >> 1;
        const char* s = GetSection(mid)->Name.buf;
        int c = _stricmp(name, s ? s : "");
        if (c == 0) {
            CurSection = mid;
            return true;
        }
        if (c < 0)
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    if (lo != hi + 1)
        Logger.g->Panic("SProperties::FindSection: '%s': Internal error", FileNameOf(this));
    CurSection = lo;
    return false;
}

// PANZERS 0x6602c0
// Binary search inside the CurSection section. CurVariable gets the match
// or the insertion point.
bool SProperties::FindVariable(const char* name)
{
    SPropertySection* sec = GetSection(CurSection);
    if (sec->VarCount == 0) {
        CurVariable = 0;
        return false;
    }
    int lo = 0;
    int hi = sec->VarCount - 1;
    while (lo <= hi) {
        int mid = (hi + lo) >> 1;
        const char* s = GetVariable(sec, mid)->Name.buf;
        int c = _stricmp(name, s ? s : "");
        if (c == 0) {
            CurVariable = mid;
            return true;
        }
        if (c < 0)
            hi = mid - 1;
        else
            lo = mid + 1;
    }
    if (lo != hi + 1)
        Logger.g->Panic("SProperties::FindVariable: '%s': Internal error", FileNameOf(this));
    CurVariable = lo;
    return false;
}

// PANZERS 0x6607a0
bool SProperties::Exists(const char* classname, const char* propname)
{
    return FindSection(classname) && FindVariable(propname);
}

// PANZERS 0x660490
// Unlike SWINE, no setlocale(LC_NUMERIC, "C") before strtod.
float SProperties::GetFloat(const char* classname, const char* propname, float def)
{
    if (SectionCount && FindSection(classname) && FindVariable(propname)) {
        const char* v = GetVariable(GetSection(CurSection), CurVariable)->Value.buf;
        return (float)strtod(v ? v : "", nullptr);
    }
    return def;
}

// PANZERS 0x660500
int SProperties::GetInt(const char* classname, const char* propname, int def)
{
    if (SectionCount && FindSection(classname) && FindVariable(propname)) {
        const char* v = GetVariable(GetSection(CurSection), CurVariable)->Value.buf;
        return strtol(v ? v : "", nullptr, 0);
    }
    return def;
}

// PANZERS 0x660570
// An empty value comes back as "" (not def).
char* SProperties::GetString(const char* classname, const char* propname, const char* def)
{
    if (SectionCount && FindSection(classname) && FindVariable(propname)) {
        char* v = GetVariable(GetSection(CurSection), CurVariable)->Value.buf;
        return v ? v : (char*)"";
    }
    return (char*)def;
}

// PANZERS 0x660150
void SProperties::EnumPropertyClasses()
{
    CurSection = 0;
    PropertiesResetClassEnum(this);   // SWINE-compat cursor, see below
}

// PANZERS 0x660160
char* SProperties::GetPropertyClass()
{
    if (CurSection >= SectionCount)
        return nullptr;
    char* n = GetSection(CurSection)->Name.buf;
    return n ? n : (char*)"";
}

// PANZERS 0x6601b0
void SProperties::NextPropertyClass()
{
    ++CurSection;
}

// PANZERS 0x660840
void SProperties::ParseFileData(char* buf, char* end)
{
    unsigned char* p = (unsigned char*)buf;
    unsigned char* e = (unsigned char*)end;
    CurSection = -1;

    // UTF-8 BOM: decode the rest in place to Latin-1 (b0 << 6 | b1 & 0x3f).
    if (buf + 3 <= end && p[0] == 0xEF && p[1] == 0xBB && p[2] == 0xBF) {
        unsigned char* src = p + 3;
        unsigned char* dst = p + 3;
        p += 3;
        if (src < e) {
            unsigned char* next = src + 1;
            do {
                unsigned char b = *src;
                if (b < 0x80 || next >= e) {
                    *dst = b;
                    src += 1;
                    next += 1;
                } else {
                    *dst = (unsigned char)((b << 6) + (*next & 0x3F));
                    src += 2;
                    next += 2;
                }
                ++dst;
            } while (src < e);
        }
        e = dst;
    }

    if (e <= p)
        return;
    do {
        unsigned char c = *p;
        if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
            ++p;
        } else if (c == ';') {
            // Comment: skip through the end of the line
            unsigned char ch;
            do {
                if (p >= e)
                    return;
                ch = *p++;
            } while (ch != '\n');
        } else if (c == '[') {
            unsigned char* nameStart = p + 1;
            unsigned char* q = nameStart;
            while (true) {
                if (q >= e || *q == '\n')
                    Logger.g->Panic("SProperties::ParseFileData: '%s': SyntaxError, missing ']'", FileNameOf(this));
                if (*q == ']')
                    break;
                ++q;
            }
            if (q <= nameStart)
                Logger.g->Panic("SProperties::ParseFileData: '%s': Empty section name", FileNameOf(this));
            *q = 0;
            CurSection = -1;
            if (FindSection((char*)nameStart))
                Logger.g->Panic("SProperties::ParseFileData: '%s': Duplicate section (%s)", FileNameOf(this), nameStart);
            InsertSection(CurSection);
            GetSection(CurSection)->Name = (char*)nameStart;
            while (true) {
                ++q;
                if (q >= e)
                    return;
                unsigned char ch = *q;
                if (ch == '\n')
                    break;
                if (ch != '\r' && ch != ' ' && ch != '\t')
                    Logger.g->Panic("SProperties::ParseFileData: '%s': SyntaxError, garbage after section name", FileNameOf(this));
            }
            p = q;
        } else {
            if (CurSection < 0)
                Logger.g->Panic("SProperties::ParseFileData: '%s': Variable without section definition", FileNameOf(this));
            unsigned char* q = p;
            while (*q != '=') {
                if (*q == '\n')
                    Logger.g->Panic("SProperties::ParseFileData: '%s': SyntaxError, missing '='", FileNameOf(this));
                ++q;
                if (q >= e)
                    Logger.g->Panic("SProperties::ParseFileData: '%s': SyntaxError, missing '='", FileNameOf(this));
            }
            unsigned char* nameEnd = q;
            while (true) {
                if (nameEnd <= p)
                    Logger.g->Panic("SProperties::ParseFileData: '%s': Empty variable name", FileNameOf(this));
                if (nameEnd[-1] != ' ' && nameEnd[-1] != '\t')
                    break;
                --nameEnd;
            }
            *nameEnd = 0;
            if (FindVariable((char*)p)) {
                const char* sec = GetSection(CurSection)->Name.buf;
                Logger.g->Panic("SProperties::ParseFileData: '%s': Dublicate variable (%s::%s)",
                                FileNameOf(this), sec ? sec : "", p);
            }
            SPropertySection* sec = GetSection(CurSection);
            InsertVariable(sec, CurVariable);
            GetVariable(sec, CurVariable)->Name = (char*)p;

            unsigned char* valStart = q;
            do {
                ++valStart;
            } while (valStart < e && (*valStart == ' ' || *valStart == '\t'));
            unsigned char* valEnd = valStart;
            while (valEnd < e && *valEnd != '\n' && *valEnd != '\r')
                ++valEnd;
            unsigned char* trim = valEnd;
            while (trim > valStart && (trim[-1] == ' ' || trim[-1] == '\t'))
                --trim;
            // At EOF this writes one byte past the data; ReadFile allocates
            // that byte (the HD exe overflowed its exact-size buffer here).
            *trim = 0;
            p = valEnd + 1;
            GetVariable(sec, CurVariable)->Value = (char*)valStart;
        }
    } while (p < e);
}

// ------------------------------------------------------------------
// SWINE-compat enumerators and OverrideFromFile (not in PANZERS.exe)
// ------------------------------------------------------------------

static SProperties* s_classEnumOwner = nullptr;
static int s_classEnumIndex = 0;
static SProperties* s_propEnumOwner = nullptr;
static int s_propEnumSection = -1;
static int s_propEnumIndex = 0;

static void PropertiesResetClassEnum(SProperties* p)
{
    s_classEnumOwner = p;
    s_classEnumIndex = 0;
}

//----- (004FF540) --------------------------------------------------------

char* SProperties::GetNextPropertyClass()
{
    if (s_classEnumOwner != this || s_classEnumIndex >= SectionCount)
        return nullptr;
    char* n = Sections[s_classEnumIndex++].Name.buf;
    return n ? n : (char*)"";
}

//----- (004FF3B0) --------------------------------------------------------

void SProperties::EnumProperties(const char* classname)
{
    s_propEnumOwner = this;
    s_propEnumIndex = 0;
    s_propEnumSection = (SectionCount && FindSection(classname)) ? CurSection : -1;
}

//----- (004FF520) --------------------------------------------------------

char* SProperties::GetNextProperty()
{
    if (s_propEnumOwner != this || s_propEnumSection < 0 || s_propEnumSection >= SectionCount)
        return nullptr;
    SPropertySection* sec = &Sections[s_propEnumSection];
    if (s_propEnumIndex >= sec->VarCount)
        return nullptr;
    char* n = sec->Vars[s_propEnumIndex++].Name.buf;
    return n ? n : (char*)"";
}

void SProperties::OverrideFromFile(const char* filename)
{
    SProperties temp(filename, false, true);
    for (int i = 0; i < temp.SectionCount; i++) {
        SPropertySection* src = &temp.Sections[i];
        const char* name = src->Name.buf ? src->Name.buf : "";
        CurSection = -1;
        if (FindSection(name)) {
            // Drop the old section, keeping the slot for the new one
            SPropertySection* dead = &Sections[CurSection];
            for (int j = 0; j < dead->VarCount; j++) {
                FreeStr(&dead->Vars[j].Name);
                FreeStr(&dead->Vars[j].Value);
            }
            if (dead->Vars)
                free(dead->Vars);
            FreeStr(&dead->Name);
        } else {
            InsertSection(CurSection);
        }
        Sections[CurSection] = *src;   // take ownership
        memset(src, 0, sizeof(*src));
        Logger.g->Log(2, "SProperties::OverrideFromFile: [%s] from (%s)", Sections[CurSection].Name.buf, filename);
    }
    CurSection = -1;
}
