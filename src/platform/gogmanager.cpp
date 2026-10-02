// platform/gogmanager.cpp
// GOG Galaxy integration
// Decompiled from: gameSplit/gog_platform.c
// Part of S.W.I.N.E. HD Remaster decompilation

#include <windows.h>
#include <string.h>

#include "platform.h"
#include "logger.h"
#include "hdbeefup.h"

// Classes: SGogManager
// Function count: 22

//----- (0047D940) --------------------------------------------------------
SGogManager::SGogManager()
{
    authFinished = false;
    authSuccess = false;
    statsInitialized = false;
}

//----- (0047D980) --------------------------------------------------------
SGogManager::~SGogManager()
{
}

//----- (0047DAC0) --------------------------------------------------------
bool SGogManager::GetAchievement(SAchievementID id, bool* achieved)
{
    const char* steamId = GetSteamAchievementId(id);
    galaxy::api::IStats* stats = galaxy::api::Stats();
    if ( !stats ) { *achieved = false; return false; }
    unsigned int unlockTime = 0;
    stats->GetAchievement(steamId, achieved, &unlockTime, 0, 0);
    return achieved != 0;
}

//----- (0047DB20) --------------------------------------------------------
bool SGogManager::GetStat(SStatID id, int* value)
{
    const char* steamId = GetSteamStatId(id);
    galaxy::api::IStats* stats = galaxy::api::Stats();
    if ( !stats ) { *value = 0; return false; }
    *value = stats->GetStatInt(steamId, 0, 0);
    return true;
}

//----- (0047DB60) --------------------------------------------------------
const char* SGogManager::GetSteamAchievementId(SAchievementID id)
{
    switch (id)
    {
    case ACH_COMPLETE_PIG_CAMPAIGN:
        return "ACH_COMPLETE_PIG_CAMPAIGN";
    case ACH_COMPLETE_RABBIT_CAMPAIGN:
        return "ACH_COMPLETE_RABBIT_CAMPAIGN";
    case ACH_DESTROY_1000_PIG_UNITS:
        return "ACH_DESTROY_1000_PIG_UNITS";
    case ACH_DESTROY_1000_RABBIT_UNITS:
        return "ACH_DESTROY_1000_RABBIT_UNITS";
    case ACH_DESTROY_100_UNITS_WITH_MINES:
        return "ACH_DESTROY_100_UNITS_WITH_MINES";
    case ACH_REACH_MAX_RANK_WITH_UNIT:
        return "ACH_REACH_MAX_RANK_WITH_UNIT";
    case ACH_HAVE_UNIT_SURVIVE_ENTIRE_CAMPAIGN:
        return "ACH_HAVE_UNIT_SURVIVE_ENTIRE_CAMPAIGN";
    case ACH_1_UNIT_DESTROY_100_UNITS_IN_CAMPAIGN:
        return "ACH_1_UNIT_DESTROY_100_UNITS_IN_CAMPAIGN";
    case ACH_DESTROY_DUG_IN_WITH_MORTAR:
        return "ACH_DESTROY_DUG_IN_WITH_MORTAR";
    case ACH_DESTROY_ELITE_WITH_STANDARD:
        return "ACH_DESTROY_ELITE_WITH_STANDARD";
    case ACH_COMPLETE_MISSION_WITH_ONLY_MORTAR:
        return "ACH_COMPLETE_MISSION_WITH_ONLY_MORTAR";
    case ACH_DESTROY_UNIT_IN_FOW:
        return "ACH_DESTROY_UNIT_IN_FOW";
    case ACH_GET_HIT_BY_UNIT_IN_FOW:
        return "ACH_GET_HIT_BY_UNIT_IN_FOW";
    case ACH_GET_UNIT_WITH_AIRDROP:
        return "ACH_GET_UNIT_WITH_AIRDROP";
    case ACH_DESTROY_3_UNITS_WITH_1_AIR_STRIKE:
        return "ACH_DESTROY_3_UNITS_WITH_1_AIR_STRIKE";
    case ACH_LOAD_3_TIMES_IN_A_MISSION:
        return "ACH_LOAD_3_TIMES_IN_A_MISSION";
    case ACH_HAVE_3_EQUIPMENTS_FOR_1_UNIT:
        return "ACH_HAVE_3_EQUIPMENTS_FOR_1_UNIT";
    case ACH_CAPTURE_ENEMY_TRAILER:
        return "ACH_CAPTURE_ENEMY_TRAILER";
    case ACH_WATCH_CREDITS:
        return "ACH_WATCH_CREDITS";
    case ACH_REACH_HUGE_XP_WITH_1_UNIT:
        return "ACH_REACH_HUGE_XP_WITH_1_UNIT";
    case ACH_REACH_HUGE_XP_WITH_3_UNIT:
        return "ACH_REACH_HUGE_XP_WITH_3_UNIT";
    case ACH_REACH_HUGE_XP_WITH_5_UNIT:
        return "ACH_REACH_HUGE_XP_WITH_5_UNIT";
    case ACH_REACH_HUGE_XP_WITH_10_UNIT:
        return "ACH_REACH_HUGE_XP_WITH_10_UNIT";
    default:
        Logger.g->Warning("GogManager: Error: unknown steam achievement id!");
        return "";
    }
}

//----- (0047DCC0) --------------------------------------------------------
const char* SGogManager::GetSteamStatId(SStatID id)
{
    switch (id)
    {
    case STAT_PIG_UNITS_DESTROYED:
        return "STAT_PIG_UNITS_DESTROYED";
    case STAT_RABBIT_UNITS_DESTROYED:
        return "STAT_RABBIT_UNITS_DESTROYED";
    case STAT_UNITS_DESTROYED_WITH_MINES:
        return "STAT_UNITS_DESTROYED_WITH_MINES";
    case STAT_MAX_UNITS_DESTROYED_BY_ONE_UNIT:
        return "STAT_MAX_UNITS_DESTROYED_BY_ONE_UNIT";
    default:
        Logger.g->Warning("SteamManager: Error: unknown steam stat id!");
        return "";
    }
}

//----- (0047DD30) --------------------------------------------------------
bool SGogManager::Initialize()
{
    // InitOptions struct passed to galaxy::api::Init
    // v6[0] = clientID, v6[1] = clientSecret, v6[2] = configFilePath, rest zeroed
    struct {
        const char* clientID;
        const char* clientSecret;
        const char* configFilePath;
        _DWORD reserved[3];
    } initOptions;

    initOptions.clientID = "52251767214664304";
    initOptions.clientSecret = "91a982a25632ae25f1cb794d05a872162c286f09f740cfd3323efbda845a9438";
    initOptions.configFilePath = ".";
    memset(&initOptions.reserved, 0, 12);

    galaxy::api::Init(&initOptions);
    if (galaxy::api::GetError())
        return false;

    Logger.g->Log(0, "GOG user sign in");
    authFinished = false;

    galaxy::api::IUser* user = galaxy::api::User();
    if ( !user )
    {
        #ifdef HD_DEBUG_PLATFORM
        Logger.g->Log(0, "GOG Galaxy not available (User() returned null)");
        #endif
        authFinished = true;
        authSuccess = false;
        return false;
    }
    user->SignInGalaxy(false, this);

    while (!authFinished)
        galaxy::api::ProcessData();

    if (authSuccess)
    {
        galaxy::api::IStats* stats = galaxy::api::Stats();
        if ( stats )
        {
            stats->RequestUserStatsAndAchievements(0, 0, this);

            while (!statsInitialized)
                galaxy::api::ProcessData();
        }
    }

    return authSuccess;
}

//----- (0047DE10) --------------------------------------------------------
void SGogManager::OnAuthFailure(galaxy::api::IAuthListener::FailureReason failureReason)
{
    const char* reason = "";
    switch (failureReason)
    {
    case galaxy::api::IAuthListener::FAILURE_REASON_UNDEFINED:
        reason = "FAILURE_REASON_UNDEFINED";
        break;
    case galaxy::api::IAuthListener::FAILURE_REASON_GALAXY_SERVICE_NOT_AVAILABLE:
        reason = "FAILURE_REASON_GALAXY_SERVICE_NOT_AVAILABLE";
        break;
    case galaxy::api::IAuthListener::FAILURE_REASON_GALAXY_SERVICE_NOT_SIGNED_IN:
        reason = "FAILURE_REASON_GALAXY_SERVICE_NOT_SIGNED_IN";
        break;
    case galaxy::api::IAuthListener::FAILURE_REASON_CONNECTION_FAILURE:
        reason = "FAILURE_REASON_CONNECTION_FAILURE";
        break;
    case galaxy::api::IAuthListener::FAILURE_REASON_NO_LICENSE:
        reason = "FAILURE_REASON_NO_LICENSE";
        break;
    case galaxy::api::IAuthListener::FAILURE_REASON_INVALID_CREDENTIALS:
        reason = "FAILURE_REASON_INVALID_CREDENTIALS";
        break;
    case galaxy::api::IAuthListener::FAILURE_REASON_GALAXY_NOT_INITIALIZED:
        reason = "FAILURE_REASON_GALAXY_NOT_INITIALIZED";
        break;
    case galaxy::api::IAuthListener::FAILURE_REASON_EXTERNAL_SERVICE_FAILURE:
        reason = "FAILURE_REASON_EXTERNAL_SERVICE_FAILURE";
        break;
    default:
        break;
    }
    Logger.g->Log(0, "SGogManager::OnAuthFailure: %s", reason);
    authFinished = true;
    authSuccess = false;
}

//----- (0047DEB0) --------------------------------------------------------
void SGogManager::OnAuthLost()
{
    Logger.g->Log(0, "SGogManager::OnAuthLost");
    authFinished = true;
    authSuccess = false;
}

//----- (0047DED0) --------------------------------------------------------
void SGogManager::OnAuthSuccess()
{
    Logger.g->Log(0, "SGogManager::OnAuthSuccess");
    authFinished = true;
    authSuccess = true;
}

//----- (0047DEF0) --------------------------------------------------------
void SGogManager::OnUserStatsAndAchievementsRetrieveFailure(
    unsigned long long userID,
    galaxy::api::IUserStatsAndAchievementsRetrieveListener::FailureReason failureReason)
{
    Logger.g->Log(0, "SGogManager::OnUserStatsAndAchievementsRetrieveFailure");
    statsInitialized = true;
}

//----- (0047DF20) --------------------------------------------------------
void SGogManager::OnUserStatsAndAchievementsRetrieveSuccess(unsigned long long userID)
{
    Logger.g->Log(0, "SGogManager::OnUserStatsAndAchievementsRetrieveSuccess");
    statsInitialized = true;
    g_Platform->OnStatsReceived();
}

//----- (0047DF60) --------------------------------------------------------
void SGogManager::SetAchievement(SAchievementID id)
{
    const char* steamId = GetSteamAchievementId(id);
    galaxy::api::IStats* stats = galaxy::api::Stats();
    if ( !stats ) return;
    stats->SetAchievement(steamId);
    galaxy::api::IStats* stats2 = galaxy::api::Stats();
    if ( stats2 ) stats2->StoreStatsAndAchievements(0);
    Logger.g->Log(1, "Unlocking GOG achievement: %s", steamId);
}

//----- (0047DFB0) --------------------------------------------------------
void SGogManager::SetStat(SStatID id, int value, bool storeImmediately)
{
    const char* steamId = GetSteamStatId(id);
    galaxy::api::IStats* stats = galaxy::api::Stats();
    if ( !stats ) return;
    stats->SetStatInt(steamId, value);
    Logger.g->Log(1, "Settings GOG stat: %s", steamId);
    if (storeImmediately)
    {
        galaxy::api::IStats* stats2 = galaxy::api::Stats();
        if ( stats2 ) stats2->StoreStatsAndAchievements(0);
    }
}

//----- Stub: SGogManager methods not in the decompiled source but called ---
void SGogManager::Shutdown()
{
    galaxy::api::Shutdown();
}

void SGogManager::Update()
{
    galaxy::api::ProcessData();
}

void SGogManager::StoreStats()
{
    galaxy::api::IStats* stats = galaxy::api::Stats();
    if ( stats ) stats->StoreStatsAndAchievements(0);
}

void SGogManager::ResetStatsAndAchievements()
{
    galaxy::api::IStats* stats = galaxy::api::Stats();
    if ( stats ) stats->ResetStatsAndAchievements();
}

const char* SGogManager::GetPlayerName()
{
    // Original likely called galaxy::api::Friends()->GetPersonaName()
    // Stubbed to return empty string since we don't have the full SDK
    return "";
}

// ============================================================
// GOG Galaxy SDK stub implementations
// The original game linked Galaxy.dll; we stub these since we
// don't have the GOG Galaxy SDK.
// ============================================================
namespace galaxy {
namespace api {

IUser* User() { return nullptr; } // TODO: Stub
IStats* Stats() { return nullptr; } // TODO: Stub
const IError* GetError() { return nullptr; } // TODO: Stub
void Init(void*) { /* TODO: Stub */ }
void ProcessData() { /* TODO: Stub */ }
void Shutdown() { /* TODO: Stub */ }

} // namespace api
} // namespace galaxy
