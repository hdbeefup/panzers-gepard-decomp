// platform/gogmanager.h
// SGogManager — GOG Galaxy integration
// Decompiled from: gameSplit/gog_platform.c
// Part of S.W.I.N.E. HD Remaster decompilation

#ifndef PLATFORM_GOGMANAGER_H
#define PLATFORM_GOGMANAGER_H

#include "core_common.h"
#include "statsandachievements.h"

// --- GOG Galaxy SDK stubs ---
// We don't have the Galaxy SDK headers, so we stub the minimum needed types.
// The original code linked against Galaxy.lib / Galaxy.dll.

namespace galaxy {
namespace api {

struct IGalaxyListener {
    virtual ~IGalaxyListener() {}
};

template<int TYPE>
struct GalaxyTypeAwareListener : public IGalaxyListener {
    virtual ~GalaxyTypeAwareListener() {}
};

struct IAuthListener : public GalaxyTypeAwareListener<7> {
    enum FailureReason {
        FAILURE_REASON_UNDEFINED = 0,
        FAILURE_REASON_GALAXY_SERVICE_NOT_AVAILABLE = 1,
        FAILURE_REASON_GALAXY_SERVICE_NOT_SIGNED_IN = 2,
        FAILURE_REASON_CONNECTION_FAILURE = 3,
        FAILURE_REASON_NO_LICENSE = 4,
        FAILURE_REASON_INVALID_CREDENTIALS = 5,
        FAILURE_REASON_GALAXY_NOT_INITIALIZED = 6,
        FAILURE_REASON_EXTERNAL_SERVICE_FAILURE = 7,
    };
    virtual void OnAuthSuccess() = 0;
    virtual void OnAuthFailure(FailureReason reason) = 0;
    virtual void OnAuthLost() = 0;
};

struct IUserStatsAndAchievementsRetrieveListener : public GalaxyTypeAwareListener<12> {
    enum FailureReason {
        FAILURE_REASON_UNDEFINED = 0,
        FAILURE_REASON_CONNECTION_FAILURE = 1,
    };
    virtual void OnUserStatsAndAchievementsRetrieveSuccess(unsigned long long userID) = 0;
    virtual void OnUserStatsAndAchievementsRetrieveFailure(unsigned long long userID, FailureReason reason) = 0;
};

typedef unsigned long long GalaxyID;

// Forward declare interfaces accessed via galaxy::api free functions
struct IStats {
    virtual void RequestUserStatsAndAchievements(unsigned long long userID, unsigned long long param2, IUserStatsAndAchievementsRetrieveListener* listener) = 0;
    virtual void GetAchievement(const char* name, bool* achieved, unsigned int* unlockTime, unsigned long long p1, unsigned long long p2) = 0;
    virtual int GetStatInt(const char* name, unsigned long long p1, unsigned long long p2) = 0;
    virtual void SetAchievement(const char* name) = 0;
    virtual void SetStatInt(const char* name, int value) = 0;
    virtual void StoreStatsAndAchievements(unsigned long long param) = 0;
    virtual void ResetStatsAndAchievements() = 0;
};

struct IUser {
    virtual void SignInGalaxy(bool requireOnline, IAuthListener* listener) = 0;
};

struct IError {
    virtual const char* GetMsg() const = 0;
};

// Galaxy SDK free functions — stubbed as externs (will be provided by Galaxy.dll at link time)
// For compilation, we declare them; at link time they'll be unresolved (that's fine for a static lib).
IUser* User();
IStats* Stats();
const IError* GetError();
void Init(void* initParams);
void ProcessData();
void Shutdown();

} // namespace api
} // namespace galaxy

// --- SGogManager class ---

struct SGogManager : public galaxy::api::IAuthListener,
                     public galaxy::api::IUserStatsAndAchievementsRetrieveListener
{
    bool authFinished;
    bool authSuccess;
    bool statsInitialized;

    SGogManager();
    ~SGogManager();

    bool Initialize();
    void Shutdown();
    void Update();

    // IAuthListener overrides
    void OnAuthSuccess() override;
    void OnAuthFailure(galaxy::api::IAuthListener::FailureReason failureReason) override;
    void OnAuthLost() override;

    // IUserStatsAndAchievementsRetrieveListener overrides
    void OnUserStatsAndAchievementsRetrieveSuccess(unsigned long long userID) override;
    void OnUserStatsAndAchievementsRetrieveFailure(unsigned long long userID,
        galaxy::api::IUserStatsAndAchievementsRetrieveListener::FailureReason failureReason) override;

    bool GetAchievement(SAchievementID id, bool* achieved);
    bool GetStat(SStatID id, int* value);
    void SetAchievement(SAchievementID id);
    void SetStat(SStatID id, int value, bool storeImmediately);
    void StoreStats();
    void ResetStatsAndAchievements();
    const char* GetPlayerName();

    const char* GetSteamAchievementId(SAchievementID id);
    const char* GetSteamStatId(SStatID id);
};

#endif // PLATFORM_GOGMANAGER_H
