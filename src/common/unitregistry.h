// common/unitregistry.h
// SUnitRegistry — runtime list of unit class names per race plus a
// race-neutral "others" list, loaded from drop-in rabbits.list /
// pigs.list / others.list files with hardcoded fallbacks.
//
// Non-original. Driven by the "unit unlimiter" feature.
//
// Design constraint: the first 13 entries of each race list MUST match
// the canonical UnitClasses[Race][0..12] order so missions.ini UnitMask
// (a positional string) keeps gating the right unit slots. Validate()
// enforces this — if a list fails validation, the fallback is used for
// that file.

#ifndef COMMON_UNITREGISTRY_H
#define COMMON_UNITREGISTRY_H

#include "string2.h"

struct SUnitRegistry {
    SString* Rabbits;        int RabbitsCount;
    SString* Pigs;           int PigsCount;
    SString* Others;         int OthersCount;

    // Derived: cross-race combat subset (positions 0-8 = combat, 12 = movingforce)
    // of the OPPOSITE race's list. Used by HD_ALLUNITSPICKER cross-race page.
    SString* RabbitsCombat;  int RabbitsCombatCount;
    SString* PigsCombat;     int PigsCombatCount;

    SUnitRegistry();
    ~SUnitRegistry();

    // Idempotent: safe to call multiple times. Logs which lists came from
    // file vs fallback.
    void Load();

    // race: 0 = rabbits, 1 = pigs.
    int          CountForRace(int race) const;
    const char*  AtForRace(int race, int i) const;

    // Cross-race combat subset (positions 0..8 + 12) of the OPPOSITE race.
    int          CountCrossRaceCombat(int race) const;
    const char*  AtCrossRaceCombat(int race, int i) const;

    // Race-neutral "others" (mine, sights, leghajo, etc.).
    int          CountOthers() const;
    const char*  AtOthers(int i) const;

    // Positional access for missions.ini UnitMask. idx must be in [0..12];
    // always returns the canonical name (Validate guarantees the prefix).
    const char*  ByMaskIndex(int race, int idx) const;

    // Flat market index space:
    //   [0 .. CountForRace(race))                                  → own race
    //   [CountForRace .. +CountCrossRaceCombat)                    → opposite-race combat
    //   [.. +CountOthers)                                          → race-neutral
    // Without HD_ALLUNITSPICKER, callers should only walk [0..CountForRace).
    int          MarketFlatCount(int race, bool includeCrossRace) const;
    const char*  MarketFlatAt(int race, int flatIdx, bool includeCrossRace) const;
};

extern SUnitRegistry UnitRegistry;

#endif // COMMON_UNITREGISTRY_H
