// core/gettext.cpp
// PANZERS: menu.ini string table, lifted from the Codename Panzers HD exe
// (PrepareGetText 0x660db0, GetText 0x660c50). See gettext.h.

#include "gettext.h"
#include "logger.h"
#include "properties.h"

SGetTextTable GetTextTable;

static void AssignString(SString* dst, const SString& src)
{
    // HD SString::operator=(const SString&) 0x52c2c0: an empty source leaves null.
    if (dst->buf) {
        delete[] dst->buf;
        dst->buf = nullptr;
    }
    dst->size = 0;
    if (src.size) {
        dst->size = src.size;
        dst->buf = new char[src.size + 1];
        memcpy(dst->buf, src.buf, src.size + 1);
    }
}

static void AppendChar(SString* s, char c)
{
    char* nb = new char[s->size + 2];
    if (s->size)
        memcpy(nb, s->buf, s->size);
    nb[s->size] = c;
    nb[s->size + 1] = 0;
    if (s->buf)
        delete[] s->buf;
    s->buf = nb;
    s->size++;
}

static void ClearString(SString* s)
{
    if (s->buf) {
        delete[] s->buf;
        s->buf = nullptr;
    }
    s->size = 0;
}

// HD: the SDArray<SGetTextSection>::Add inlined in 0x660db0
static int AddSection()
{
    SGetTextTable& t = GetTextTable;
    if (t.Count == t.Max) {
        int newmax = (t.Max < 16) ? 16 : (t.Max * 6) / 5;
        t.Sections = (SGetTextSection*)realloc(t.Sections, newmax * sizeof(SGetTextSection));
        memset(&t.Sections[t.Max], 0, (newmax - t.Max) * sizeof(SGetTextSection));
        t.Max = newmax;
    }
    return t.Count++;
}

// PANZERS 0x660be0
static int AddEntry(SGetTextSection* sec)
{
    if (sec->Count == sec->Max) {
        int newmax = (sec->Max < 16) ? 16 : (sec->Max * 6) / 5;
        sec->Entries = (SGetTextEntry*)realloc(sec->Entries, newmax * sizeof(SGetTextEntry));
        memset(&sec->Entries[sec->Max], 0, (newmax - sec->Max) * sizeof(SGetTextEntry));
        sec->Max = newmax;
    }
    return sec->Count++;
}

static bool IsQuoted(const char* s)
{
    size_t len = strlen(s);
    return len >= 3 && s[0] == '"' && s[len - 1] == '"';
}

// PANZERS 0x660db0
// For each section (SProperties order, i.e. sorted by name) read id_000/str000,
// id_001/str001, ... until an id is missing. A missing str defaults to "\"\"".
// The section stops early, without an error, at the first id or str that is
// shorter than 3 chars or not enclosed in double quotes. Escapes: \\ \" \n.
// Any other escape is fatal in an id; in a str the backslash is kept.
// Nothing clears the table, so a second call appends duplicates and the
// first copy wins in GetText.
void PrepareGetText(const char* filename)
{
    SProperties props(filename, true);
    SString id;
    SString str;

    props.EnumPropertyClasses();
    for (char* secName = props.GetPropertyClass(); secName; props.NextPropertyClass(), secName = props.GetPropertyClass()) {
        int secIndex = AddSection();
        SGetTextSection* sec = &GetTextTable.Sections[secIndex];
        ClearString(&sec->Name);
        sec->Name = secName;

        for (int i = 0;; i++) {
            char key[16];
            sprintf(key, "id_%03d", i);
            const char* idv = props.GetString(secName, key, nullptr);
            if (!idv)
                break;
            sprintf(key, "str%03d", i);
            const char* strv = props.GetString(secName, key, "\"\"");
            if (!IsQuoted(idv) || !IsQuoted(strv))
                break;

            ClearString(&id);
            for (const char* q = idv + 1; *q && *q != '"';) {
                if (*q == '\\') {
                    char n = q[1];
                    if (n == '\\')
                        AppendChar(&id, '\\');
                    else if (n == '"')
                        AppendChar(&id, '"');
                    else if (n == 'n')
                        AppendChar(&id, '\n');
                    else
                        Logger.g->Panic("prepare_gettext(%s): [%s]/%d: Invalid escape sequence", filename, secName, i);
                    q += 2;
                } else {
                    AppendChar(&id, *q);
                    q++;
                }
            }

            ClearString(&str);
            for (const char* q = strv + 1; *q && *q != '"';) {
                if (*q == '\\') {
                    char n = q[1];
                    if (n == '\\') {
                        AppendChar(&str, '\\');
                        q += 2;
                    } else if (n == '"') {
                        AppendChar(&str, '"');
                        q += 2;
                    } else if (n == 'n') {
                        AppendChar(&str, '\n');
                        q += 2;
                    } else {
                        AppendChar(&str, '\\');
                        q += 1;
                    }
                } else {
                    AppendChar(&str, *q);
                    q++;
                }
            }

            sec = &GetTextTable.Sections[secIndex];
            int e = AddEntry(sec);
            AssignString(&sec->Entries[e].Id, id);
            AssignString(&sec->Entries[e].Str, str);
        }
    }
    ClearString(&str);
    ClearString(&id);
}

// PANZERS 0x660c50
// Exact (case-sensitive) strcmp on both the section and the id. Only the
// first section with a matching name is searched.
char* GetText(const char* section, const char* id)
{
    if (GetTextTable.Count == 0)
        return (char*)id;
    for (int i = 0; i < GetTextTable.Count; i++) {
        SGetTextSection* sec = &GetTextTable.Sections[i];
        if (strcmp(sec->Name.buf ? sec->Name.buf : "", section) != 0)
            continue;
        for (int j = 0; j < sec->Count; j++) {
            SGetTextEntry* e = &sec->Entries[j];
            if (strcmp(e->Id.buf ? e->Id.buf : "", id) == 0)
                return e->Str.buf ? e->Str.buf : (char*)"";
        }
        return (char*)id;
    }
    return (char*)id;
}

// SWINE engine API (window/widget.h). Not in PANZERS.exe: the SWINE window
// code passes bare keys such as "SWINE_OK", so every section is searched.
char* GetText(const char* key)
{
    for (int i = 0; i < GetTextTable.Count; i++) {
        SGetTextSection* sec = &GetTextTable.Sections[i];
        for (int j = 0; j < sec->Count; j++) {
            SGetTextEntry* e = &sec->Entries[j];
            if (strcmp(e->Id.buf ? e->Id.buf : "", key) == 0)
                return e->Str.buf ? e->Str.buf : (char*)"";
        }
    }
    return (char*)key;
}
