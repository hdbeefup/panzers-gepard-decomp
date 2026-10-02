// common/modmanager.cpp
// SModManager — drop-in mods/ + langpack/ loader.
// Non-original: gated behind HDB_MODLOADER_SYSTEM.

#include "modmanager.h"

#ifdef HDB_MODLOADER_SYSTEM

#include <windows.h>
#include <stdlib.h>
#include <string.h>

#include "properties.h"
#include "logger.h"

SModManager ModManager;

SModManager::SModManager()
    : Mods(nullptr), ModCount(0), ModCapacity(0),
      LangPacks(nullptr), LangPackCount(0), LangPackCapacity(0),
      UnitFiles(nullptr), UnitFileCount(0), UnitFileCapacity(0),
      ActiveModFolders(nullptr), ActiveModFolderCount(0), ActiveModFolderCapacity(0)
{
}

static void FreeMods(SModInfo*& arr, int& count, int& cap)
{
    for (int i = 0; i < count; ++i) {
        if (arr[i].FolderName.buf)  { delete[] arr[i].FolderName.buf;  arr[i].FolderName.buf = nullptr; }
        if (arr[i].DisplayName.buf) { delete[] arr[i].DisplayName.buf; arr[i].DisplayName.buf = nullptr; }
    }
    free(arr);
    arr = nullptr;
    count = 0;
    cap = 0;
}

static void FreeLangPacks(SLangPackInfo*& arr, int& count, int& cap)
{
    for (int i = 0; i < count; ++i) {
        if (arr[i].FolderCode.buf)  { delete[] arr[i].FolderCode.buf;  arr[i].FolderCode.buf = nullptr; }
        if (arr[i].DisplayName.buf) { delete[] arr[i].DisplayName.buf; arr[i].DisplayName.buf = nullptr; }
        if (arr[i].BaseLang.buf)    { delete[] arr[i].BaseLang.buf;    arr[i].BaseLang.buf = nullptr; }
    }
    free(arr);
    arr = nullptr;
    count = 0;
    cap = 0;
}

static void FreeStrings(SString*& arr, int& count, int& cap)
{
    for (int i = 0; i < count; ++i) {
        if (arr[i].buf) { delete[] arr[i].buf; arr[i].buf = nullptr; }
    }
    free(arr);
    arr = nullptr;
    count = 0;
    cap = 0;
}

SModManager::~SModManager()
{
    FreeMods(Mods, ModCount, ModCapacity);
    FreeLangPacks(LangPacks, LangPackCount, LangPackCapacity);
    FreeStrings(UnitFiles, UnitFileCount, UnitFileCapacity);
    FreeStrings(ActiveModFolders, ActiveModFolderCount, ActiveModFolderCapacity);
    if (ActiveLangPack.buf)  { delete[] ActiveLangPack.buf;  ActiveLangPack.buf = nullptr; }
}

template <typename T>
static T* Grow(T*& arr, int& cap, int need)
{
    if (need <= cap) return arr;
    int newCap = cap ? cap * 2 : 8;
    while (newCap < need) newCap *= 2;
    arr = (T*)realloc(arr, newCap * sizeof(T));
    memset(&arr[cap], 0, sizeof(T) * (newCap - cap));
    cap = newCap;
    return arr;
}

static int CompareStrings(const void* a, const void* b)
{
    const SString* sa = (const SString*)a;
    const SString* sb = (const SString*)b;
    const char* ba = sa->buf ? sa->buf : "";
    const char* bb = sb->buf ? sb->buf : "";
    return _stricmp(ba, bb);
}

static int CompareModsByDisplay(const void* a, const void* b)
{
    const SModInfo* ma = (const SModInfo*)a;
    const SModInfo* mb = (const SModInfo*)b;
    const char* na = ma->DisplayName.buf ? ma->DisplayName.buf : "";
    const char* nb = mb->DisplayName.buf ? mb->DisplayName.buf : "";
    return _stricmp(na, nb);
}

void SModManager::EnumerateMods()
{
    FreeMods(Mods, ModCount, ModCapacity);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("mods\\*", &fd);
    if (h == INVALID_HANDLE_VALUE) {
        Logger.g->Log(1, "SModManager: no mods/ folder");
        return;
    }
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (fd.cFileName[0] == '.') continue;

        Grow(Mods, ModCapacity, ModCount + 1);
        SModInfo& mi = Mods[ModCount];

        mi.FolderName = fd.cFileName;

        // Try mods/<name>/mod.ini for metadata.
        char inipath[MAX_PATH];
        _snprintf_s(inipath, sizeof(inipath), _TRUNCATE, "mods\\%s\\mod.ini", fd.cFileName);
        WIN32_FILE_ATTRIBUTE_DATA st;
        if (GetFileAttributesExA(inipath, GetFileExInfoStandard, &st)) {
            SProperties p(inipath, false, true);
            const char* dn = p.GetString("Mod", "DisplayName", nullptr);
            if (!dn || !dn[0]) dn = p.GetString("mod", "DisplayName", nullptr);
            if (dn && dn[0]) mi.DisplayName = dn;
        }
        if (!mi.DisplayName.buf) mi.DisplayName = fd.cFileName;

        ++ModCount;
        Logger.g->Log(1, "SModManager: mod (%s) -> '%s'", mi.FolderName.buf, mi.DisplayName.buf);
    } while (FindNextFileA(h, &fd));
    FindClose(h);

    if (ModCount > 1)
        qsort(Mods, ModCount, sizeof(SModInfo), CompareModsByDisplay);
}

void SModManager::EnumerateLangPacks()
{
    FreeLangPacks(LangPacks, LangPackCount, LangPackCapacity);

    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA("langpack\\*", &fd);
    if (h == INVALID_HANDLE_VALUE) {
        Logger.g->Log(1, "SModManager: no langpack/ folder");
        return;
    }
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (fd.cFileName[0] == '.') continue;

        Grow(LangPacks, LangPackCapacity, LangPackCount + 1);
        SLangPackInfo& li = LangPacks[LangPackCount];

        li.FolderCode = fd.cFileName;

        // Display name priority: messages.ini SWINE_LANGPACK_NAME,
        // then langpack.ini DisplayName, else the folder code itself.
        char msgpath[MAX_PATH];
        _snprintf_s(msgpath, sizeof(msgpath), _TRUNCATE, "langpack\\%s\\messages.ini", fd.cFileName);
        WIN32_FILE_ATTRIBUTE_DATA st;
        if (GetFileAttributesExA(msgpath, GetFileExInfoStandard, &st)) {
            SProperties m(msgpath, false, false);
            const char* dn = m.GetString("messages", "SWINE_LANGPACK_NAME", nullptr);
            if (dn && dn[0]) li.DisplayName = dn;
        }
        if (!li.DisplayName.buf) {
            char metapath[MAX_PATH];
            _snprintf_s(metapath, sizeof(metapath), _TRUNCATE, "langpack\\%s\\langpack.ini", fd.cFileName);
            if (GetFileAttributesExA(metapath, GetFileExInfoStandard, &st)) {
                SProperties p(metapath, false, true);
                const char* dn = p.GetString("LangPack", "DisplayName", nullptr);
                if (!dn || !dn[0]) dn = p.GetString("langpack", "DisplayName", nullptr);
                if (dn && dn[0]) li.DisplayName = dn;
                const char* bl = p.GetString("LangPack", "BaseLang", nullptr);
                if (!bl || !bl[0]) bl = p.GetString("langpack", "BaseLang", nullptr);
                if (bl && bl[0]) li.BaseLang = bl;
            }
        }
        if (!li.DisplayName.buf) li.DisplayName = fd.cFileName;
        if (!li.BaseLang.buf) li.BaseLang = "EN";

        ++LangPackCount;
        Logger.g->Log(1, "SModManager: langpack (%s) -> '%s' base=%s",
                      li.FolderCode.buf, li.DisplayName.buf, li.BaseLang.buf);
    } while (FindNextFileA(h, &fd));
    FindClose(h);
}

void SModManager::SetActiveMods(const char* const* folderNames, int count)
{
    FreeStrings(ActiveModFolders, ActiveModFolderCount, ActiveModFolderCapacity);
    for (int i = 0; i < count; ++i) {
        const char* name = folderNames ? folderNames[i] : nullptr;
        if (!name || !name[0]) continue;
        Grow(ActiveModFolders, ActiveModFolderCapacity, ActiveModFolderCount + 1);
        ActiveModFolders[ActiveModFolderCount] = name;
        ++ActiveModFolderCount;
    }
    if (ActiveModFolderCount == 0) {
        Logger.g->Log(0, "SModManager: active mods = (none)");
    } else {
        for (int i = 0; i < ActiveModFolderCount; ++i) {
            Logger.g->Log(0, "SModManager: active mod [%d] = %s",
                          i, ActiveModFolders[i].buf ? ActiveModFolders[i].buf : "");
        }
    }
    RefreshActiveModUnitFiles();
}

void SModManager::SetActiveMod(const char* folderName)
{
    if (folderName && folderName[0])
        SetActiveMods(&folderName, 1);
    else
        SetActiveMods(nullptr, 0);
}

void SModManager::SetActiveLangPack(const char* folderCode)
{
    if (ActiveLangPack.buf) { delete[] ActiveLangPack.buf; ActiveLangPack.buf = nullptr; ActiveLangPack.size = 0; }
    if (folderCode && folderCode[0])
        ActiveLangPack = folderCode;
}

int SModManager::GetActiveModCount() const
{
    return ActiveModFolderCount;
}

const char* SModManager::GetActiveModFolderAt(int index) const
{
    if (index < 0 || index >= ActiveModFolderCount) return "";
    return ActiveModFolders[index].buf ? ActiveModFolders[index].buf : "";
}

const char* SModManager::GetActiveModFolder() const
{
    return GetActiveModFolderAt(0);
}

const char* SModManager::GetActiveLangPackCode() const
{
    return ActiveLangPack.buf ? ActiveLangPack.buf : "";
}

bool SModManager::HasActiveMod() const { return ActiveModFolderCount > 0; }
bool SModManager::HasActiveLangPack() const { return ActiveLangPack.buf && ActiveLangPack.buf[0]; }

int SModManager::FindModIndex(const char* folderName) const
{
    if (!folderName) return -1;
    for (int i = 0; i < ModCount; ++i) {
        const char* fn = Mods[i].FolderName.buf ? Mods[i].FolderName.buf : "";
        if (_stricmp(fn, folderName) == 0) return i;
    }
    return -1;
}

int SModManager::FindLangPackIndex(const char* folderCode) const
{
    if (!folderCode) return -1;
    for (int i = 0; i < LangPackCount; ++i) {
        const char* fc = LangPacks[i].FolderCode.buf ? LangPacks[i].FolderCode.buf : "";
        if (_stricmp(fc, folderCode) == 0) return i;
    }
    return -1;
}

void SModManager::RefreshActiveModUnitFiles()
{
    FreeStrings(UnitFiles, UnitFileCount, UnitFileCapacity);
    if (ActiveModFolderCount <= 0) return;

    // Subdirs (relative to mods\<mod>\) we scan for .unit files. Top level
    // is the documented spot, but mods that bundle binary content under
    // base\ usually want their .unit patches there too — the search-path
    // overlay puts mods\<mod>\base\ on the file lookup path, so it reads
    // most naturally for modders.
    static const char* const kSubdirs[] = { "", "base" };

    // Slot 0 = lowest priority. Walk in array order so OverrideFromFile
    // (later wins) applies the highest-priority mod's .unit patches LAST.
    for (int m = 0; m < ActiveModFolderCount; ++m) {
        const char* mod = ActiveModFolders[m].buf;
        if (!mod || !*mod) continue;

        int firstHere = UnitFileCount;

        for (int s = 0; s < (int)(sizeof(kSubdirs) / sizeof(kSubdirs[0])); ++s) {
            const char* sub = kSubdirs[s];
            char pattern[MAX_PATH];
            if (sub[0])
                _snprintf_s(pattern, sizeof(pattern), _TRUNCATE, "mods\\%s\\%s\\*.unit", mod, sub);
            else
                _snprintf_s(pattern, sizeof(pattern), _TRUNCATE, "mods\\%s\\*.unit", mod);
            WIN32_FIND_DATAA fd;
            HANDLE h = FindFirstFileA(pattern, &fd);
            if (h == INVALID_HANDLE_VALUE) continue;
            do {
                if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
                Grow(UnitFiles, UnitFileCapacity, UnitFileCount + 1);
                char full[MAX_PATH];
                if (sub[0])
                    _snprintf_s(full, sizeof(full), _TRUNCATE, "mods\\%s\\%s\\%s", mod, sub, fd.cFileName);
                else
                    _snprintf_s(full, sizeof(full), _TRUNCATE, "mods\\%s\\%s", mod, fd.cFileName);
                UnitFiles[UnitFileCount] = full;
                ++UnitFileCount;
            } while (FindNextFileA(h, &fd));
            FindClose(h);
        }

        // Sort this mod's batch alphabetically for deterministic order.
        if (UnitFileCount - firstHere > 1)
            qsort(&UnitFiles[firstHere], UnitFileCount - firstHere, sizeof(SString), CompareStrings);
    }

    for (int i = 0; i < UnitFileCount; ++i)
        Logger.g->Log(1, "SModManager: .unit patch [%d] %s", i, UnitFiles[i].buf);
}

#endif // HDB_MODLOADER_SYSTEM
