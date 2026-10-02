// platform/platform.h
// SPlatform — platform abstraction layer
// Decompiled from: gameSplit/gog_platform.c
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef PLATFORM_PLATFORM_H
#define PLATFORM_PLATFORM_H

#include "core_common.h"
#include "chain.h"
#include "statsandachievements.h"
#ifdef PLATFORM_GOG
#include "gogmanager.h"
#endif

// --- Platform data types ---

struct SAchievement {
    SAchievementID Id;
    bool Achieved;
};

struct SStat {
    SStatID Id;
    int Value;
};

// --- SPlatform class ---

struct SPlatform {
#ifdef PLATFORM_GOG
    SGogManager* GogManager;
#endif
    Array<SStat> Stats;
    Array<SAchievement> Achievements;
    char* PlayerName;

    SPlatform();
    ~SPlatform();

    void Initialize();
    void Update();

    void InitStats();
    void InitAchievements();
    void InitPlayerName();

    char* GetPlayerName();
    SStat* GetLocalStatById(SStatID id);
    SAchievement* GetLocalAchievementById(SAchievementID id);

    void SetStat(SStatID id, int value, bool onlyIfGreater, bool storeImmediately);
    int IncrementStat(SStatID id, int amount, int storeValue, bool storeImmediately);
    void SetStatToPlatform(SStat* stat, bool storeImmediately);
    void StoreStats();

    void UnlockAchievement(SAchievementID id);
    void UnlockStatBasedAchievementIfNeeded(SStat* stat);
    void ResetStatsAndAchievements();

    void OnStatsReceived();
    void OnGameOverlayActivated(bool active);
};

extern SPlatform* g_Platform;

#endif // PLATFORM_PLATFORM_H
