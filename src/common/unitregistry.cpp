// common/unitregistry.cpp
// SUnitRegistry — see unitregistry.h.

#include "unitregistry.h"

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logger.h"
#include "hdbeefup.h"
#ifdef HDB_MODLOADER_SYSTEM
#include "modmanager.h"
#endif

SUnitRegistry UnitRegistry;

// Canonical-13 ordering. MUST match UnitClasses[Race][0..12] defined in
// src/game/game.cpp (game) and src/editor/nolib.cpp (editor) — those
// definitions stay the source of truth for missions.ini UnitMask
// positional decoding; the registry only mirrors them.
// (src/bot/bot_main.cpp also has a copy for the headless bot experiment,
//  same content — not the canonical game source.)
static const char* const kCanonicalRabbits[13] = {
    "Nyul dzsip",
    "Nyul pancelauto",
    "Nyul light tank",
    "Nyul loveg",
    "Nyul raketas pancelkocsi",
    "Nyul raketas",
    "Nyul mozsar",
    "Nyul aknarako",
    "Nyul vontato",
    "Ammo",
    "Fuel",
    "Service",
    "Nyul movingforce",
};
static const char* const kCanonicalPigs[13] = {
    "Diszno dzsip",
    "Diszno pancelauto",
    "Diszno normal tank",
    "Diszno loveg",
    "Diszno hard tank",
    "Diszno raketas",
    "Diszno mozsar",
    "Diszno aknarako",
    "Diszno vontato",
    "Ammo",
    "Fuel",
    "Service",
    "Diszno movingforce",
};

// Fallback content used when rabbits.list / pigs.list / others.list are
// missing or malformed. Picked to match today's combined market+editor
// roster: canonical 13 for missions.ini compat, then bunkers / heroes /
// flag / Spy mirroring src/editor/mainfrm.cpp kPigUnits[]/kRabbitUnits[].
static const char* const kFallbackRabbits[] = {
    "Nyul dzsip",
    "Nyul pancelauto",
    "Nyul light tank",
    "Nyul loveg",
    "Nyul raketas pancelkocsi",
    "Nyul raketas",
    "Nyul mozsar",
    "Nyul aknarako",
    "Nyul vontato",
    "Ammo",
    "Fuel",
    "Service",
    "Nyul movingforce",
    "Nyul bunker loveg",
    "Nyul bunker mozsar",
    "Nyul bunker airdefense",
    "Nyul zaszlo",
    "Kero",
    "Fogas Wili",
    "Point Roger",
    "Spy",
};
static const char* const kFallbackPigs[] = {
    "Diszno dzsip",
    "Diszno pancelauto",
    "Diszno normal tank",
    "Diszno loveg",
    "Diszno hard tank",
    "Diszno raketas",
    "Diszno mozsar",
    "Diszno aknarako",
    "Diszno vontato",
    "Ammo",
    "Fuel",
    "Service",
    "Diszno movingforce",
    "Diszno bunker loveg",
    "Diszno bunker mozsar",
    "Diszno bunker airdefense",
    "Diszno zaszlo",
    "Zsiros Korom",
    "Nyalas Szaj",
};
static const char* const kFallbackOthers[] = {
    "Akna",
    "Lato szem",
    "Lato kisszem",
};

// ---------------------------------------------------------------------------
// helpers

static SString* AllocList(int count)
{
    if (count <= 0) return nullptr;
    SString* out = (SString*)calloc((size_t)count, sizeof(SString));
    return out;
}

static void FreeList(SString*& arr, int& count)
{
    if (arr) {
        for (int i = 0; i < count; ++i) {
            if (arr[i].buf) { delete[] arr[i].buf; arr[i].buf = nullptr; arr[i].size = 0; }
        }
        free(arr);
        arr = nullptr;
    }
    count = 0;
}

static void FillFromHardcoded(SString*& arr, int& count, const char* const* src, int n)
{
    FreeList(arr, count);
    arr = AllocList(n);
    count = n;
    for (int i = 0; i < n; ++i)
        arr[i] = src[i];
}

// Strip CR/LF and trailing whitespace; also drop leading whitespace.
static char* TrimInPlace(char* s)
{
    if (!s) return s;
    while (*s == ' ' || *s == '\t') ++s;
    int n = (int)strlen(s);
    while (n > 0) {
        char c = s[n - 1];
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
            s[--n] = 0;
        else
            break;
    }
    return s;
}

// Try to open path in read-binary mode. Returns FILE* or nullptr.
static FILE* TryOpen(const char* path)
{
    FILE* f = nullptr;
    fopen_s(&f, path, "rb");
    return f;
}

// Resolve the highest-priority on-disk path for a list filename, or
// nullptr if no file exists. Caller does NOT own the returned buffer
// (points into outBuf).
static const char* ResolveListPath(const char* basename, char* outBuf, int outBufLen)
{
#ifdef HDB_MODLOADER_SYSTEM
    // Highest priority active mod first.
    int activeCount = ModManager.GetActiveModCount();
    for (int i = activeCount - 1; i >= 0; --i) {
        const char* mod = ModManager.GetActiveModFolderAt(i);
        if (!mod || !*mod) continue;
        _snprintf_s(outBuf, outBufLen, _TRUNCATE, "mods\\%s\\%s", mod, basename);
        WIN32_FILE_ATTRIBUTE_DATA st;
        if (GetFileAttributesExA(outBuf, GetFileExInfoStandard, &st) &&
            !(st.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
            return outBuf;
    }
#endif
    // Base game next to exe.
    _snprintf_s(outBuf, outBufLen, _TRUNCATE, "%s", basename);
    WIN32_FILE_ATTRIBUTE_DATA st;
    if (GetFileAttributesExA(outBuf, GetFileExInfoStandard, &st) &&
        !(st.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        return outBuf;
    return nullptr;
}

// Parse a .list file into a temporary growable array. Returns true on
// successful parse (file readable + at least one entry). Caller owns
// `outArr` (free with FreeList).
static bool ParseListFile(const char* path, SString*& outArr, int& outCount)
{
    outArr = nullptr;
    outCount = 0;

    FILE* f = TryOpen(path);
    if (!f) return false;

    // Read whole file into a buffer.
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (fsize <= 0 || fsize > 1 << 20) { // 1 MiB sanity cap
        fclose(f);
        return false;
    }
    char* data = new char[fsize + 1];
    size_t rd = fread(data, 1, (size_t)fsize, f);
    fclose(f);
    data[rd] = 0;

    // Two-pass: count lines, then fill.
    int cap = 0;
    SString* tmp = nullptr;
    int n = 0;

    char* p = data;
    while (*p) {
        // find end of line
        char* eol = p;
        while (*eol && *eol != '\n' && *eol != '\r') ++eol;
        char saved = *eol;
        *eol = 0;

        char* line = TrimInPlace(p);
        if (line[0] && line[0] != '#') {
            if (n == cap) {
                int newCap = cap ? cap * 2 : 16;
                tmp = (SString*)realloc(tmp, (size_t)newCap * sizeof(SString));
                memset(&tmp[cap], 0, sizeof(SString) * (size_t)(newCap - cap));
                cap = newCap;
            }
            tmp[n] = line;
            ++n;
        }

        if (saved == 0) break;
        p = eol + 1;
        // skip extra newline byte (handle CRLF)
        if (saved == '\r' && *p == '\n') ++p;
    }

    delete[] data;

    if (n <= 0) {
        if (tmp) free(tmp);
        return false;
    }

    outArr = tmp;
    outCount = n;
    return true;
}

// True if list[0..12] matches canonical[0..12] case-insensitively.
static bool ValidatePrefix(const SString* list, int count,
                           const char* const* canonical, int canonicalLen)
{
    if (count < canonicalLen) return false;
    for (int i = 0; i < canonicalLen; ++i) {
        const char* a = list[i].buf ? list[i].buf : "";
        const char* b = canonical[i];
        if (_stricmp(a, b) != 0) return false;
    }
    return true;
}

// Load a single .list file (with fallback). Logs the outcome.
static void LoadOneList(const char* basename, SString*& outArr, int& outCount,
                        const char* const* canonical, int canonicalLen,
                        const char* const* fallback, int fallbackLen,
                        const char* tag)
{
    FreeList(outArr, outCount);

    char pathBuf[MAX_PATH];
    const char* path = ResolveListPath(basename, pathBuf, sizeof(pathBuf));

    if (path) {
        SString* parsed = nullptr;
        int parsedCount = 0;
        if (ParseListFile(path, parsed, parsedCount)) {
            if (canonical && !ValidatePrefix(parsed, parsedCount, canonical, canonicalLen)) {
                Logger.g->Log(1,
                    "SUnitRegistry: %s prefix mismatch in '%s' (first %d entries must match canonical UnitClasses[%s]); using fallback",
                    tag, path, canonicalLen, tag);
                FreeList(parsed, parsedCount);
            } else {
                outArr = parsed;
                outCount = parsedCount;
                Logger.g->Log(1, "SUnitRegistry: %s loaded from '%s' (%d entries)", tag, path, parsedCount);
                return;
            }
        } else {
            Logger.g->Log(1, "SUnitRegistry: %s '%s' present but unreadable; using fallback", tag, path);
        }
    } else {
        Logger.g->Log(1, "SUnitRegistry: %s no '%s' on disk; using fallback", tag, basename);
    }

    FillFromHardcoded(outArr, outCount, fallback, fallbackLen);
}

// Build the cross-race combat subset: positions 0..8 (combat) plus 12
// (movingforce). Skips Ammo/Fuel/Service (positions 9-11) since the
// market always shows those on page 1.
static void BuildCombatSubset(SString*& dst, int& dstCount,
                              const SString* src, int srcCount)
{
    FreeList(dst, dstCount);

    // We need at least position 12; canonical guarantees 13 entries.
    if (srcCount < 13) return;

    int n = 9 + 1; // 0..8 + 12
    dst = AllocList(n);
    for (int i = 0; i < 9; ++i) dst[i] = src[i];
    dst[9] = src[12];
    dstCount = n;
}

// ---------------------------------------------------------------------------
// SUnitRegistry

SUnitRegistry::SUnitRegistry()
    : Rabbits(nullptr), RabbitsCount(0),
      Pigs(nullptr), PigsCount(0),
      Others(nullptr), OthersCount(0),
      RabbitsCombat(nullptr), RabbitsCombatCount(0),
      PigsCombat(nullptr), PigsCombatCount(0)
{
}

SUnitRegistry::~SUnitRegistry()
{
    FreeList(Rabbits, RabbitsCount);
    FreeList(Pigs, PigsCount);
    FreeList(Others, OthersCount);
    FreeList(RabbitsCombat, RabbitsCombatCount);
    FreeList(PigsCombat, PigsCombatCount);
}

void SUnitRegistry::Load()
{
    LoadOneList("rabbits.list", Rabbits, RabbitsCount,
                kCanonicalRabbits, 13,
                kFallbackRabbits, (int)(sizeof(kFallbackRabbits) / sizeof(kFallbackRabbits[0])),
                "rabbits");
    LoadOneList("pigs.list", Pigs, PigsCount,
                kCanonicalPigs, 13,
                kFallbackPigs, (int)(sizeof(kFallbackPigs) / sizeof(kFallbackPigs[0])),
                "pigs");
    LoadOneList("others.list", Others, OthersCount,
                /*canonical*/ nullptr, 0,
                kFallbackOthers, (int)(sizeof(kFallbackOthers) / sizeof(kFallbackOthers[0])),
                "others");

    BuildCombatSubset(RabbitsCombat, RabbitsCombatCount, Rabbits, RabbitsCount);
    BuildCombatSubset(PigsCombat, PigsCombatCount, Pigs, PigsCount);
}

int SUnitRegistry::CountForRace(int race) const
{
    return (race == 1) ? PigsCount : RabbitsCount;
}

const char* SUnitRegistry::AtForRace(int race, int i) const
{
    const SString* a = (race == 1) ? Pigs : Rabbits;
    int n = (race == 1) ? PigsCount : RabbitsCount;
    if (i < 0 || i >= n || !a) return "";
    return a[i].buf ? a[i].buf : "";
}

int SUnitRegistry::CountCrossRaceCombat(int race) const
{
    // Cross-race = OPPOSITE race's combat subset.
    return (race == 1) ? RabbitsCombatCount : PigsCombatCount;
}

const char* SUnitRegistry::AtCrossRaceCombat(int race, int i) const
{
    const SString* a = (race == 1) ? RabbitsCombat : PigsCombat;
    int n = (race == 1) ? RabbitsCombatCount : PigsCombatCount;
    if (i < 0 || i >= n || !a) return "";
    return a[i].buf ? a[i].buf : "";
}

int SUnitRegistry::CountOthers() const { return OthersCount; }

const char* SUnitRegistry::AtOthers(int i) const
{
    if (i < 0 || i >= OthersCount || !Others) return "";
    return Others[i].buf ? Others[i].buf : "";
}

const char* SUnitRegistry::ByMaskIndex(int race, int idx) const
{
    if (idx < 0 || idx > 12) return "";
    return AtForRace(race, idx);
}

int SUnitRegistry::MarketFlatCount(int race, bool includeCrossRace) const
{
    int n = CountForRace(race);
    if (includeCrossRace)
        n += CountCrossRaceCombat(race) + OthersCount;
    return n;
}

const char* SUnitRegistry::MarketFlatAt(int race, int flatIdx, bool includeCrossRace) const
{
    int own = CountForRace(race);
    if (flatIdx < own) return AtForRace(race, flatIdx);
    if (!includeCrossRace) return "";
    int xr = CountCrossRaceCombat(race);
    if (flatIdx < own + xr) return AtCrossRaceCombat(race, flatIdx - own);
    int o = OthersCount;
    if (flatIdx < own + xr + o) return AtOthers(flatIdx - own - xr);
    return "";
}
