// platform/platform.cpp
// Platform abstraction layer
// Decompiled from: gameSplit/gog_platform.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <string.h>

#include "platform.h"
#include "logger.h"

SPlatform* g_Platform = 0;

// Function count: 19

//----- (0047E030) --------------------------------------------------------
SPlatform::SPlatform()
{
#ifdef PLATFORM_GOG
    GogManager = 0;
#endif
    PlayerName = 0;
    Stats.array = 0;
    Stats.count = 0;
    Stats.capacity = 0;
    Achievements.array = 0;
    Achievements.count = 0;
    Achievements.capacity = 0;
}

//----- (0047E070) --------------------------------------------------------
SPlatform::~SPlatform()
{
#ifdef PLATFORM_GOG
    if (GogManager)
    {
        GogManager->Shutdown();
        if (GogManager)
        {
            delete GogManager;
            GogManager = 0;
        }
    }
#endif
    Achievements.count = 0;
    if (Achievements.array)
        free(Achievements.array);
    Stats.count = 0;
    if (Stats.array)
        free(Stats.array);
}

//----- (0047E220) --------------------------------------------------------
SAchievement* SPlatform::GetLocalAchievementById(SAchievementID id)
{
    SAchievement* arr = Achievements.array;
    SAchievement* end;
    if (arr)
        end = &arr[Achievements.count];
    else
        end = 0;
    if (arr == end)
        return 0;
    while (arr->Id != id)
    {
        if (++arr == end)
            return 0;
    }
    return arr;
}

//----- (0047E260) --------------------------------------------------------
SStat* SPlatform::GetLocalStatById(SStatID id)
{
    SStat* arr = Stats.array;
    SStat* end;
    if (arr)
        end = &arr[Stats.count];
    else
        end = 0;
    if (arr == end)
        return 0;
    while (arr->Id != id)
    {
        if (++arr == end)
            return 0;
    }
    return arr;
}

//----- (0047E2A0) --------------------------------------------------------
char* SPlatform::GetPlayerName()
{
    return PlayerName;
}

//----- (0047E2B0) --------------------------------------------------------
int SPlatform::IncrementStat(SStatID id, int amount, int storeValue, bool storeImmediately)
{
    SStat* arr = Stats.array;
    SStat* end;
    if (arr)
        end = &arr[Stats.count];
    else
        end = 0;
    if (arr == end)
        goto not_found;
    while (arr->Id != id)
    {
        if (++arr == end)
            goto not_found;
    }
    if (!arr)
    {
    not_found:
        Logger.g->Warning("Stat not found during IncrementStat!");
        return 0;
    }
    else
    {
        arr->Value += amount;
        UnlockStatBasedAchievementIfNeeded(arr);
        int Value;
        bool shouldStore;
        if (storeValue <= 0 || (Value = arr->Value, Value - amount >= storeValue) || Value < storeValue)
        {
            Value = arr->Value;
            shouldStore = false;
        }
        else
        {
            shouldStore = true;
        }
        int result = Value;
        bool doStore = storeImmediately || shouldStore;
#ifdef PLATFORM_GOG
        if (GogManager)
        {
            GogManager->SetStat(arr->Id, Value, doStore);
            return arr->Value;
        }
#endif
        return result;
    }
}

//----- (0047E360) --------------------------------------------------------
void SPlatform::InitAchievements()
{
    SAchievement item;

    item.Id = ACH_COMPLETE_PIG_CAMPAIGN;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_COMPLETE_RABBIT_CAMPAIGN;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_DESTROY_1000_PIG_UNITS;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_DESTROY_1000_RABBIT_UNITS;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_DESTROY_100_UNITS_WITH_MINES;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_REACH_MAX_RANK_WITH_UNIT;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_HAVE_UNIT_SURVIVE_ENTIRE_CAMPAIGN;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_1_UNIT_DESTROY_100_UNITS_IN_CAMPAIGN;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_DESTROY_DUG_IN_WITH_MORTAR;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_DESTROY_ELITE_WITH_STANDARD;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_COMPLETE_MISSION_WITH_ONLY_MORTAR;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_DESTROY_UNIT_IN_FOW;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_GET_HIT_BY_UNIT_IN_FOW;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_GET_UNIT_WITH_AIRDROP;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_DESTROY_3_UNITS_WITH_1_AIR_STRIKE;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_LOAD_3_TIMES_IN_A_MISSION;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_HAVE_3_EQUIPMENTS_FOR_1_UNIT;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_CAPTURE_ENEMY_TRAILER;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_WATCH_CREDITS;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_REACH_HUGE_XP_WITH_1_UNIT;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_REACH_HUGE_XP_WITH_3_UNIT;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_REACH_HUGE_XP_WITH_5_UNIT;
    item.Achieved = false;
    Achievements.Add(&item);
    item.Id = ACH_REACH_HUGE_XP_WITH_10_UNIT;
    item.Achieved = false;
    Achievements.Add(&item);
}

//----- (0047E570) --------------------------------------------------------
void SPlatform::InitPlayerName()
{
    PlayerName = new char[257];
    DWORD username_len = 257;
    GetUserNameA(PlayerName, &username_len);
#ifdef PLATFORM_GOG
    if (GogManager)
        strcpy(PlayerName, GogManager->GetPlayerName());
#endif
    Logger.g->Log(0, "Platform player name initialized: %s", PlayerName);
}

//----- (0047E5E0) --------------------------------------------------------
void SPlatform::InitStats()
{
    SStat item;

    item.Id = STAT_PIG_UNITS_DESTROYED;
    item.Value = 0;
    Stats.Add(&item);
    item.Id = STAT_RABBIT_UNITS_DESTROYED;
    item.Value = 0;
    Stats.Add(&item);
    item.Id = STAT_UNITS_DESTROYED_WITH_MINES;
    item.Value = 0;
    Stats.Add(&item);
    item.Id = STAT_MAX_UNITS_DESTROYED_BY_ONE_UNIT;
    item.Value = 0;
    Stats.Add(&item);
}

//----- (0047E660) --------------------------------------------------------
void SPlatform::Initialize()
{
#ifdef PLATFORM_GOG
    SGogManager* gog = new SGogManager();
    GogManager = gog;
    bool success = GogManager->Initialize();
    const char* msg = "GOG initialized successfully";
    if (!success)
        msg = "ERROR initializing GOG";
    Logger.g->Log(0, msg);
    if (!success)
    {
        GogManager->Shutdown();
        if (GogManager)
        {
            delete GogManager;
            GogManager = 0;
        }
    }
#endif

    PlayerName = new char[257];
    DWORD username_len = 257;
    GetUserNameA(PlayerName, &username_len);
#ifdef PLATFORM_GOG
    if (GogManager)
        strcpy(PlayerName, GogManager->GetPlayerName());
#endif
    Logger.g->Log(0, "Platform player name initialized: %s", PlayerName);

    SStat item;
    item.Id = STAT_PIG_UNITS_DESTROYED;
    item.Value = 0;
    Stats.Add(&item);
    item.Id = STAT_RABBIT_UNITS_DESTROYED;
    item.Value = 0;
    Stats.Add(&item);
    item.Id = STAT_UNITS_DESTROYED_WITH_MINES;
    item.Value = 0;
    Stats.Add(&item);
    item.Id = STAT_MAX_UNITS_DESTROYED_BY_ONE_UNIT;
    item.Value = 0;
    Stats.Add(&item);

    InitAchievements();
}

//----- (0047E7E0) --------------------------------------------------------
void SPlatform::OnGameOverlayActivated(bool active)
{
    const char* msg = "Game overlay activated";
    if (!active)
        msg = "Game overlay deactivated";
    Logger.g->Log(1, msg);
}

//----- (0047E810) --------------------------------------------------------
void SPlatform::OnStatsReceived()
{
    Logger.g->Log(1, "Stats received, updating local stat data");
#ifdef PLATFORM_GOG
    if (GogManager)
    {
        SStat* arr = Stats.array;
        SStat* end;
        if (arr)
            end = &arr[Stats.count];
        else
            end = 0;
        for (; arr != end; ++arr)
        {
            int value;
            if (GogManager->GetStat(arr->Id, &value))
                arr->Value = value;
        }

        SAchievement* achArr = Achievements.array;
        SAchievement* achEnd;
        if (achArr)
            achEnd = &achArr[Achievements.count];
        else
            achEnd = 0;
        for (; achArr != achEnd; ++achArr)
        {
            bool achieved;
            if (GogManager->GetAchievement(achArr->Id, &achieved))
                achArr->Achieved = achieved;
        }
    }
#endif
}

//----- (0047E8B0) --------------------------------------------------------
void SPlatform::ResetStatsAndAchievements()
{
#ifdef PLATFORM_GOG
    if (GogManager)
        GogManager->ResetStatsAndAchievements();
#endif
}

//----- (0047E8C0) --------------------------------------------------------
void SPlatform::SetStat(SStatID id, int value, bool onlyIfGreater, bool storeImmediately)
{
    SStat* arr = Stats.array;
    SStat* end;
    if (arr)
        end = &arr[Stats.count];
    else
        end = 0;
    if (arr == end)
    {
        Logger.g->Warning("Stat not found during UpdateStat!");
        return;
    }
    while (arr->Id != id)
    {
        if (++arr == end)
        {
            Logger.g->Warning("Stat not found during UpdateStat!");
            return;
        }
    }
    if (!onlyIfGreater || arr->Value < value)
    {
        arr->Value = value;
        UnlockStatBasedAchievementIfNeeded(arr);
#ifdef PLATFORM_GOG
        if (GogManager)
            GogManager->SetStat(arr->Id, arr->Value, storeImmediately);
#endif
    }
}

//----- (0047E940) --------------------------------------------------------
void SPlatform::SetStatToPlatform(SStat* stat, bool storeImmediately)
{
#ifdef PLATFORM_GOG
    if (GogManager)
        GogManager->SetStat(stat->Id, stat->Value, storeImmediately);
#endif
}

//----- (0047E960) --------------------------------------------------------
void SPlatform::StoreStats()
{
#ifdef PLATFORM_GOG
    if (GogManager)
        GogManager->StoreStats();
#endif
}

//----- (0047E970) --------------------------------------------------------
void SPlatform::UnlockAchievement(SAchievementID id)
{
    SAchievement* arr = Achievements.array;
    SAchievement* end;
    if (arr)
        end = &arr[Achievements.count];
    else
        end = 0;
    if (arr == end)
    {
        arr = 0;
    }
    else
    {
        while (arr->Id != id)
        {
            if (++arr == end)
            {
                arr = 0;
                break;
            }
        }
    }
    if (arr && !arr->Achieved)
    {
        arr->Achieved = true;
#ifdef PLATFORM_GOG
        if (GogManager)
            GogManager->SetAchievement(id);
#endif
    }
}

//----- (0047E9C0) --------------------------------------------------------
void SPlatform::UnlockStatBasedAchievementIfNeeded(SStat* stat)
{
    switch (stat->Id)
    {
    case STAT_PIG_UNITS_DESTROYED:
        if (stat->Value >= 1000)
            UnlockAchievement(ACH_DESTROY_1000_PIG_UNITS);
        break;
    case STAT_RABBIT_UNITS_DESTROYED:
        if (stat->Value >= 1000)
            UnlockAchievement(ACH_DESTROY_1000_RABBIT_UNITS);
        break;
    case STAT_UNITS_DESTROYED_WITH_MINES:
        if (stat->Value >= 100)
            UnlockAchievement(ACH_DESTROY_100_UNITS_WITH_MINES);
        break;
    case STAT_MAX_UNITS_DESTROYED_BY_ONE_UNIT:
        if (stat->Value >= 100)
            UnlockAchievement(ACH_1_UNIT_DESTROY_100_UNITS_IN_CAMPAIGN);
        break;
    default:
        return;
    }
}

//----- (0047EA40) --------------------------------------------------------
void SPlatform::Update()
{
#ifdef PLATFORM_GOG
    if (GogManager)
        GogManager->Update();
#endif
}
