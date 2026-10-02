// tools/paktest/paktest.cpp
// Console check of the Panzers file system, ini parser and menu.ini string
// table, run against a game directory:
//
//   paktest <rundir> [-log]
//
// It follows the SSuperWindow ctor (HD 0x656d50): load panzers.ini as a loose
// file, SetSearchPath([Paths] Search), SetHomePath([Paths] Home), then
// PrepareGetText("menu.ini"). Then it reads menu.ini and some TGAs through
// OpenRead, checks which search element supplied them, and looks up menu.ini
// text keys. Exit code 0 means every check passed.

#include "core.h"
#include "gettext.h"
#include <direct.h>

static int g_failures = 0;

static void Check(bool ok, const char* what)
{
    printf("  [%s] %s\n", ok ? "ok" : "FAIL", what);
    if (!ok)
        g_failures++;
}

// Counts the TOC nodes and the file nodes (size > 0) of a pak, by walking the
// tree the way the HD loader does (pre-order; left child inline, right by offset).
static void CountToc(const unsigned char* toc, const unsigned char* node, int* nodes, int* files)
{
    while (true) {
        const unsigned char* after = node + 2 + node[1];
        (*nodes)++;
        if (*(const int*)(after + 4) > 0)
            (*files)++;
        int right = *(const int*)(after + 9);
        if (right)
            CountToc(toc, toc + right, nodes, files);
        if (!after[8])
            return;
        node = after + 13;
    }
}

static const char* BaseName(const char* path)
{
    const char* b = path;
    for (const char* p = path; *p; p++)
        if (*p == '/' || *p == '\\')
            b = p + 1;
    return b;
}

// Index of the first search element that has `name`, or -1 (loose file).
static int Provider(const char* name)
{
    for (int i = 0; i < FileSystem.SearchPathCount; i++) {
        SSearchPathElement* e = &FileSystem.SearchPath[i];
        if (e->Type == SEARCHPATH_ARCHIVE) {
            if (FileSystem.LookupArchive(i, name))
                return i;
        } else {
            char full[300];
            _snprintf(full, sizeof(full), "%s%s", e->Name.buf, name);
            full[sizeof(full) - 1] = 0;
            struct _stat st;
            if (_stat(full, &st) == 0)
                return i;
        }
    }
    return -1;
}

static void ShowCopies(const char* name)
{
    printf("  copies of %s:", name);
    for (int i = 0; i < FileSystem.SearchPathCount; i++) {
        SSearchPathElement* e = &FileSystem.SearchPath[i];
        if (e->Type != SEARCHPATH_ARCHIVE)
            continue;
        SArchiveHeaderEntry* ent = FileSystem.LookupArchive(i, name);
        if (ent)
            printf(" [%d]%s=%d", i, BaseName(e->Name.buf), ent->Size);
    }
    printf("\n");
}

static int StreamSize(SStream* s)
{
    int size = s->Seek(0, 2);
    s->Seek(0, 0);
    return size;
}

static void TestTga(const char* name)
{
    SStream* s = FileSystem.OpenRead(name, nullptr);
    if (!s) {
        printf("  %s: not found\n", name);
        g_failures++;
        return;
    }
    int size = StreamSize(s);
    unsigned char h[18];
    s->Read(h, 18);
    s->Release();
    int type = h[2];
    int w = h[12] | (h[13] << 8);
    int ht = h[14] | (h[15] << 8);
    int bpp = h[16];
    int p = Provider(name);
    printf("  %s: %dx%d, %d bpp, TGA type %d, %d bytes, from [%d] %s\n", name, w, ht, bpp, type, size, p,
           p >= 0 ? BaseName(FileSystem.SearchPath[p].Name.buf) : "(loose)");
    bool sane = (type == 2 || type == 10 || type == 3 || type == 11) && w > 0 && ht > 0
                && (bpp == 24 || bpp == 32 || bpp == 8 || bpp == 16);
    Check(sane, "TGA header looks valid");
}

// The first search element that has `name` must supply it.
static void TestOverride(const char* name)
{
    ShowCopies(name);
    int p = Provider(name);
    SStream* s = FileSystem.OpenRead(name, nullptr);
    if (!s || p < 0) {
        Check(false, "file found");
        if (s)
            s->Release();
        return;
    }
    int size = StreamSize(s);
    s->Release();
    SArchiveHeaderEntry* ent = FileSystem.LookupArchive(p, name);
    printf("  OpenRead(%s) size %d, first listed provider [%d] %s (size %d)\n", name, size, p,
           BaseName(FileSystem.SearchPath[p].Name.buf), ent ? ent->Size : -1);
    Check(ent && ent->Size == size, "the earlier-listed pak wins");
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: paktest <rundir> [-log]\n");
        return 2;
    }
    bool log = argc > 2 && _stricmp(argv[2], "-log") == 0;
    if (_chdir(argv[1]) != 0) {
        fprintf(stderr, "paktest: cannot chdir to %s\n", argv[1]);
        return 2;
    }
    // With logging off, a Panic exits with code 1 and prints nothing.
    Logger.g = new SLogger((char*)"paktest", (char*)"paktest", log, 2);

    printf("== panzers.ini\n");
    SProperties* ini = new SProperties("panzers.ini", true);
    const char* search = ini->GetString("Paths", "Search", nullptr);
    const char* home = ini->GetString("Paths", "home", "");    // lookup is case-insensitive
    printf("  Search = %s\n  Home   = %s\n  Debug Level = %d\n", search ? search : "(none)", home,
           ini->GetInt("Debug", "debug level", -1));
    Check(search != nullptr, "[Paths] Search present");
    printf("  sections (sorted):");
    ini->EnumPropertyClasses();
    for (char* c = ini->GetPropertyClass(); c; ini->NextPropertyClass(), c = ini->GetPropertyClass())
        printf(" [%s]", c);
    printf("\n");

    printf("== mount\n");
    FileSystem.SetSearchPath(search, false);
    FileSystem.SetHomePath(home);
    delete ini;
    int archives = 0;
    for (int i = 0; i < FileSystem.SearchPathCount; i++) {
        SSearchPathElement* e = &FileSystem.SearchPath[i];
        if (e->Type == SEARCHPATH_ARCHIVE) {
            int nodes = 0, files = 0;
            CountToc(e->Toc, e->Toc, &nodes, &files);
            printf("  [%d] pak %-22s toc %8d bytes, %6d nodes, %6d files\n", i, BaseName(e->Name.buf),
                   e->TocSize, nodes, files);
            archives++;
        } else {
            printf("  [%d] dir %s\n", i, e->Name.buf);
        }
    }
    printf("  archives: %d, home: %s\n", archives, FileSystem.GetHomePath());
    Check(archives > 0, "at least one pak mounted");

    printf("== menu.ini\n");
    SStream* s = FileSystem.OpenRead("menu.ini", "paktest");
    int size = StreamSize(s);
    char* text = new char[size + 1];
    s->Read(text, size);
    text[size] = 0;
    s->Release();
    int p = Provider("menu.ini");
    printf("  %d bytes, from [%d] %s\n", size, p, p >= 0 ? BaseName(FileSystem.SearchPath[p].Name.buf) : "(loose)");
    char* line = text;
    for (int n = 0; n < 20 && line && *line; n++) {
        char* nl = strchr(line, '\n');
        if (nl)
            *nl = 0;
        size_t len = strlen(line);
        if (len && line[len - 1] == '\r')
            line[len - 1] = 0;
        printf("  %2d| %s\n", n + 1, line);
        line = nl ? nl + 1 : nullptr;
    }
    delete[] text;

    printf("== string table\n");
    PrepareGetText("menu.ini");
    int entries = 0;
    for (int i = 0; i < GetTextTable.Count; i++)
        entries += GetTextTable.Sections[i].Count;
    printf("  %d sections, %d entries\n", GetTextTable.Count, entries);
    Check(GetTextTable.Count > 0 && entries > 0, "table loaded");
    struct { const char* section; const char* id; } keys[] = {
        { "panzers/MainMenu.cpp", "Main Menu" },
        { "panzers/SkirmishChatRoom.cpp", "Skirmish" },
        { "panzers/ChatRoom.cpp", "Attack" },
        { "panzers/GameSpyTitleRoom.cpp", "GameSpyTitleRoom" },
        { "panzers/InGameMenu.cpp", "Music volume" },
    };
    for (auto& k : keys) {
        const char* t = GetText(k.section, k.id);
        printf("  GetText(\"%s\", \"%s\") = \"%s\"%s\n", k.section, k.id, t, t == k.id ? "  (not found, id returned)" : "");
        Check(t != k.id, "found in the table");
    }
    // id and str differ for these in the shipped menu.ini
    Check(strcmp(GetText("panzers/ChatRoom.cpp", "Attack"), "Assault") == 0, "\"Attack\" -> \"Assault\"");
    Check(strcmp(GetText("panzers/GameSpyTitleRoom.cpp", "GameSpyTitleRoom"), "Online Room") == 0,
          "\"GameSpyTitleRoom\" -> \"Online Room\"");
    const char* missing = "no such text";
    Check(GetText("panzers/MainMenu.cpp", missing) == missing, "unknown id returns the id pointer");
    Check(GetText("panzers/mainmenu.cpp", "Main Menu") != GetText("panzers/MainMenu.cpp", "Main Menu"),
          "section match is case-sensitive");

    printf("== TGAs\n");
    TestTga("menu/main_menu_bottom_hq.tga");
    TestTga("menu/credits_1_hq.tga");
    TestTga("menu/medals_eng_hq.tga");

    printf("== lookup order\n");
    TestOverride("menu.ini");                 // nordic_en > panzers_patch_en > panzers_en
    TestOverride("menu/medals_eng_hq.tga");   // panzers_patch > panzers
    TestOverride("missions.ini");             // panzers_patch > panzers
    TestOverride("MENU\\Credits_1_HQ.tga");   // case and separator folding

    printf("== result: %s (%d failure%s)\n", g_failures ? "FAIL" : "PASS", g_failures, g_failures == 1 ? "" : "s");
    return g_failures ? 1 : 0;
}
