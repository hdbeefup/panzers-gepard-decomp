// src/game/campaign.cpp
// pz::SPanzersCampaign (campaign.h). OWNER: agent F (docs/M3_INTERFACES.md).
// Lifted from the HD exe: construction, the Training Camp flow, the army
// arrays (market hand-over, mission army), the mission properties, the
// replay header (read, write, StartReplay) and the campaign part of the save.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <windows.h>
#include "campaign.h"
#include "stub_log.h"
#include "logger.h"
#include "stream.h"
#include "properties.h"
#include "pzunitregistry.h"
#include "unitsave.h"
#include "world.h"
#include "worldapi.h"
#include "gamelogic.h"
#include "doodad.h"

namespace pz {

SPanzersCampaign* g_Campaign = nullptr;   // HD 0x929a0c

static SProperties* Props(SPanzersCampaign* c) { return (SProperties*)c->MissionProps; }

// SPanzersCampaign::SaveGameBefore (the "Before" save of the next mission,
// agent S): a logged stub in src/stubs/stub_panzers.cpp until S lands it.
void PzStub_SaveGameBefore(SPanzersCampaign* c);

// ---------------------------------------------------------------------------
// SDArray<SUnitDef> helpers

static void SetStr(SString* s, const char* text)
{
    // HD inline SString assignment: free, then copy (length 0 -> null buffer).
    FreeSString(s);
    if (text && *text) {
        s->size = (int)strlen(text);
        s->buf = new char[s->size + 1];
        memcpy(s->buf, text, s->size + 1);
    }
}

static void UnitDefClear(SUnitDef* d)
{
    // The record parts 0x51e520 frees per element (towed 0x51de90, ScriptID,
    // stored 0x51de90, ClassName).
    for (int i = 0; i < d->TowedCount; ++i)
        UnitDefClear(&d->TowedUnits[i]);
    free(d->TowedUnits);
    FreeSString(&d->ScriptID);
    for (int i = 0; i < d->StoredCount; ++i)
        UnitDefClear(&d->StoredUnits[i]);
    free(d->StoredUnits);
    FreeSString(&d->ClassName);
    memset((void*)d, 0, sizeof(*d));
}

// PANZERS 0x51e520
void ArmyResize(SArmyArray* a, int size)
{
    if (a->Size != 0 && a->Array == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "SUnitDef");
    for (int i = 0; i < a->Size; ++i)
        UnitDefClear(&a->Array[i]);
    a->Size = size;
    if (a->Max < size) {
        a->Max = size;
        a->Array = (SUnitDef*)realloc((void*)a->Array, size * sizeof(SUnitDef));
    }
    if (a->Array)
        memset((void*)a->Array, 0, a->Max * sizeof(SUnitDef));
}

// PANZERS 0x51de90 (SDArray<SUnitDef> dtor)
void ArmyFree(SArmyArray* a)
{
    for (int i = 0; i < a->Size; ++i)
        UnitDefClear(&a->Array[i]);
    free(a->Array);
    a->Array = nullptr;
    a->Size = a->Max = 0;
}

// PANZERS 0x560d10
SUnitDef* ArmyAdd(SArmyArray* a)
{
    if (a->Size == a->Max) {
        int nmax = a->Max < 0x10 ? 0x10 : (a->Max * 6) / 5;
        a->Array = (SUnitDef*)realloc((void*)a->Array, nmax * sizeof(SUnitDef));
        memset((void*)(a->Array + a->Max), 0, (nmax - a->Max) * sizeof(SUnitDef));
        a->Max = nmax;
    }
    return &a->Array[a->Size++];
}

static void SubArrayCopy(SUnitDef** dstArr, int* dstSize, int* dstMax, SUnitDef* srcArr, int srcSize)
{
    // 0x51e520 on the member array, then 0x560290 per element.
    SArmyArray t = { *dstArr, *dstSize, *dstMax };
    ArmyResize(&t, srcSize);
    for (int i = 0; i < t.Size; ++i)
        UnitDefCopy(&t.Array[i], &srcArr[i]);
    *dstArr = t.Array;
    *dstSize = t.Size;
    *dstMax = t.Max;
}

// PANZERS 0x560290 (SUnitDef::operator=)
void UnitDefCopy(SUnitDef* dst, const SUnitDef* src)
{
    SetStr(&dst->ClassName, SStr(src->ClassName));
    memcpy((unsigned char*)dst + 0x08, (const unsigned char*)src + 0x08, 0x48 - 0x08);   // +0x08..+0x44
    dst->Stored = src->Stored;                                    // +0x48 (byte)
    SubArrayCopy(&dst->StoredUnits, &dst->StoredCount, &dst->StoredMax, src->StoredUnits, src->StoredCount);
    SetStr(&dst->ScriptID, SStr(src->ScriptID));
    dst->AIGroup = src->AIGroup;                                  // +0x60
    dst->Cargo = src->Cargo;                                      // +0x64
    dst->FirstKill = src->FirstKill;                              // +0x68..+0x6c (bytes)
    dst->FirstBlood = src->FirstBlood;
    dst->FirstShot = src->FirstShot;
    dst->FirstVehicleLost = src->FirstVehicleLost;
    dst->FirstArmouredVehicleKill = src->FirstArmouredVehicleKill;
    SubArrayCopy(&dst->TowedUnits, &dst->TowedCount, &dst->TowedMax, src->TowedUnits, src->TowedCount);
    dst->StoredSpecial = src->StoredSpecial;                      // +0x7c
}

// PANZERS 0x591500 (this = dst, the source by pointer); 0x591e70 is the same
// with this = the campaign (out = copy of +0x3c).
void ArmyCopyFrom(SArmyArray* dst, const SArmyArray* src)
{
    ArmyResize(dst, src->Size);
    for (int i = 0; i < dst->Size; ++i)
        UnitDefCopy(&dst->Array[i], &src->Array[i]);
}

// PANZERS 0x5cfbf0
void UnitDefSave(const SUnitDef* d, SStream* s)
{
    SaveVariables(s, d, kUnitDefDesc);                            // 0x670c50(s, d, 0x8ddb48)
}

// PANZERS 0x5d0db0 (SUnitRegistry: the price of an equipment slot type)
static int SlotPrice(int type)
{
    SUnitRegistry* r = g_UnitRegistry;
    switch (type) {
    case 1: return r->PriceGrenade;                               // +0xa4
    case 2: return r->PriceMolotov;                               // +0xa8
    case 3: return r->PriceMagneticMine;                          // +0xbc
    case 4: return r->PriceExplosives;                            // +0xb8
    case 5: return r->PriceTankMine;                              // +0xb0
    case 6: return r->PriceBinoculars;                            // +0xc0
    case 7: return r->PriceBoat;                                  // +0xac
    case 8: return r->PriceMineDetector;                          // +0xb4
    default: return 0;
    }
}

// The price of one army record: the type (SPUnit +0x38) and its two
// equipment slots, plus the first stored unit (the crew) and its slots.
// Inline in 0x5916e0 and 0x5971b0.
static int UnitDefPrice(const SUnitDef* d)
{
    SPUnit* p = g_UnitRegistry->GetPUnit(SStr(d->ClassName), false);   // 0x5d0e70(name, 0)
    int price = p->Price;
    for (int k = 0; k < 2; ++k)
        if (d->Slots[k] > 0)
            price += SlotPrice(d->Slots[k]);
    if (d->StoredCount > 0) {
        const SUnitDef* c = &d->StoredUnits[0];
        price += g_UnitRegistry->GetPUnit(SStr(c->ClassName), false)->Price;
        for (int k = 0; k < 2; ++k)
            if (c->Slots[k] > 0)
                price += SlotPrice(c->Slots[k]);
    }
    return price;
}

// PANZERS 0x579220 (SDArray<SUnitDef>::Remove)
void ArmyRemove(SArmyArray* a, int i)
{
    UnitDefClear(&a->Array[i]);
    --a->Size;
    if (a->Size - i != 0)
        memmove((void*)&a->Array[i], (const void*)&a->Array[i + 1], (a->Size - i) * sizeof(SUnitDef));
    memset((void*)&a->Array[a->Size], 0, sizeof(SUnitDef));
}

// PANZERS 0x592640
// A ScriptID not used by any mission-army record or its first stored unit:
// "MU <class> #<n>", n = 1, 2, ...
static void UniqueScriptID(SString* out, const char* className, const SArmyArray* army)
{
    for (int n = 1;; ++n) {
        char buf[300];
        _snprintf(buf, sizeof(buf) - 1, "MU %s #%d", className, n);   // "MU " (0x7f97bc) + name + " #%d" (0x7f97c0)
        buf[sizeof(buf) - 1] = 0;
        bool used = false;
        for (int i = 0; i < army->Size && !used; ++i) {
            const SUnitDef* d = &army->Array[i];
            if (d->ScriptID.size == (int)strlen(buf) && (d->ScriptID.size == 0 || _stricmp(d->ScriptID.buf, buf) == 0))
                used = true;
            else if (d->StoredCount > 0) {
                const SUnitDef* c = &d->StoredUnits[0];
                if (c->ScriptID.size == (int)strlen(buf) && (c->ScriptID.size == 0 || _stricmp(c->ScriptID.buf, buf) == 0))
                    used = true;
            }
        }
        if (!used) {
            SetStr(out, buf);
            return;
        }
    }
}

// PANZERS 0x5916e0
// Buys one record into the mission army: false when it costs more than the
// prestige left. An empty ScriptID (the unit's and its crew's) gets a
// unique one.
bool SPanzersCampaign::AddMissionArmyUnit(const SUnitDef* def)
{
    PZ_M3_TRACE("SPanzersCampaign::AddMissionArmyUnit (0x5916e0)");
    int price = UnitDefPrice(def);
    if (Prestige < price)
        return false;
    // HD works on the caller's record (it writes the ScriptIDs into it).
    SUnitDef* d = const_cast<SUnitDef*>(def);
    if (d->ScriptID.size == 0)
        UniqueScriptID(&d->ScriptID, SStr(d->ClassName), &MissionArmy);
    if (d->StoredCount > 0 && d->StoredUnits[0].ScriptID.size == 0)
        UniqueScriptID(&d->StoredUnits[0].ScriptID, SStr(d->StoredUnits[0].ClassName), &MissionArmy);
    UnitDefCopy(ArmyAdd(&MissionArmy), d);
    Prestige -= price;
    return true;
}

// PANZERS 0x5971b0
// The market hands over the army it shows. refund: the old mission army is
// sold back (its prices return to Prestige) and every record is bought with
// AddMissionArmyUnit; else the mission army becomes a plain copy.
void SPanzersCampaign::SetArmy(SArmyArray* army, bool refund)
{
    PZ_M3_TRACE("SPanzersCampaign::SetArmy (0x5971b0)");
    if (!refund) {
        ArmyCopyFrom(&MissionArmy, army);                         // 0x591500
        return;
    }
    while (MissionArmy.Size != 0) {
        Prestige += UnitDefPrice(&MissionArmy.Array[0]);
        ArmyRemove(&MissionArmy, 0);                              // 0x579220(0)
    }
    for (int i = 0; i < army->Size; ++i)
        AddMissionArmyUnit(&army->Array[i]);
}

// PANZERS 0x56d6d0
// The statistics category of a unit: vehicles, guns, members, buildings
// and planes by unit type; 0 for the rest (squads, projectiles, ...).
int UnitStatsCategory(const SUnit* u)
{
    int ct = u->Proto->ClassType;                                 // prototype +0x40
    if (ct != 0 && ct != 0xb && ct != 6 && ct != 9 && ct != 10)
        return 0;
    switch (u->Proto->UnitType) {                                 // prototype +0x44
    case 0: case 0xc: return 2;
    case 1: return 3;
    case 2: return 5;
    case 3: case 6: case 7: return 8;
    case 4: return 10;
    case 8: return 6;
    case 9: return 4;
    case 10: return 1;
    case 0xb: return 7;
    case 0xd: case 0x1a: return 0xb;
    case 0xe: case 0xf: case 0x10: case 0x11: case 0x12: case 0x13:
    case 0x14: case 0x15: case 0x16: case 0x17: case 0x18:
        return ct != 5 ? 9 : 0;
    default: return 0;
    }
}

// PANZERS 0x5e2da0 (the campaign statistics at its end)
void CampaignUnitCreated(int unit, int player)
{
    SPanzersCampaign* c = g_Campaign;
    if (!c || c->MenuToLoad == PZ_MENU_MARKET)
        return;
    const SUnit* u = g_World->Units.Array[unit].Unit;
    int k = UnitStatsCategory(u);
    if (k == 0 || u->_110 || (g_GameLogic && g_GameLogic->IsPaused()))
        return;
    int* r = (int*)c->PlayerStats[player];
    r[3 + k]++;                                                   // +0x14c + (player * 0x36 + k) * 4
    r[3]++;                                                       // +0x14c + player * 0xd8
}

// ---------------------------------------------------------------------------
// Construction

// PANZERS 0x590ec0
SPanzersCampaign::SPanzersCampaign()
{
    PZ_M3_TRACE("SPanzersCampaign::SPanzersCampaign (0x590ec0)");
    // HD writes the fields one by one (the eh-vector ctors 0x590ea0 / 0x5910c0
    // zero the 12 x 0xc records at +0x48 and the first two dwords of the 12
    // player records); 0x594470 zeroes the player counters. Everything else
    // is zero too, so the recompile clears the object.
    memset(this, 0, sizeof(*this));
    _b80 = 2;                                                     // param_1[0x2e0] (byte)
    Difficulty = 1;                                               // param_1[7]
    _100 = -1;                                                    // param_1[0x40]
    _b88 = 1;                                                     // param_1[0x2e2] (byte)
}

// PANZERS 0x591350
SPanzersCampaign::~SPanzersCampaign()
{
    PZ_M3_TRACE("SPanzersCampaign::~SPanzersCampaign (0x591350)");
    delete (SProperties*)MissionProps;                            // 0x660080 + delete 0x1c
    MissionProps = nullptr;
    delete (SProperties*)LocalProps;
    LocalProps = nullptr;
    for (int i = 0; i < ObjectiveCount; ++i) {                    // 0x5911c0
        FreeSString(&Objectives[i].Text);
        free(Objectives[i].Targets);
    }
    free(Objectives);
    Objectives = nullptr;
    ObjectiveCount = ObjectiveMax = 0;
    FreeSString(&ReplayName);                                     // +0x12c
    for (int p = 0; p < 12; ++p)                                  // the player records' names (+0x140 + p * 0xd8)
        FreeSString((SString*)PlayerStats[p]);
    ArmyFree(&MissionArmy);
    ArmyFree(&Army);
    FreeSString(&MissionSection);
    FreeSString(&MapName);
}

// PANZERS 0x594ab0
void SPanzersCampaign::InitTutorialMode(const char* map, int race, int prestige)
{
    PZ_M3_TRACE("SCampaign::InitTutorialMode (0x594ab0)");
    if (GameMode != PZ_GM_NONE)
        Logger.g->Panic("SCampaign::InitTutorialMode: GameMode can only be initialized once.");
    GameMode = PZ_GM_TUTORIAL;
    Race = race;
    SetStr(&MapName, map);                                        // 0x52c320 on +0x20
    StartPrestige = prestige;
    Prestige = prestige;
    MenuToLoad = PZ_MENU_GAMEVIEW;
    ArmyCopyFrom(&MissionArmy, &Army);                            // 0x591500(this +0x3c, +0x2c)
    MissionResult = 0;
}

void SPanzersCampaign::InitScenarioMode(const char* map, const char* section, int race, int prestige)
{
    STUB_LOG("SCampaign::InitScenarioMode (0x5944d0)");
    PZ_M3_TRACE("SCampaign::InitScenarioMode (0x5944d0)");
    (void)map; (void)section; (void)race; (void)prestige;
}

// PANZERS 0x592b20
// New Game (0x53441): the nation's first mission, missions.ini and
// missions_local.ini (both panic when missing), the map, the mission's
// "SP" as starting prestige, then PrepareMission.
void SPanzersCampaign::InitCampaignMode()
{
    PZ_M3_TRACE("SCampaign::InitCampaignMode (0x592b20)");
    if (GameMode != PZ_GM_NONE)
        Logger.g->Panic("SCampaign::InitCampaignMode: GameMode can only be initialized once.");
    GameMode = PZ_GM_CAMPAIGN;
    SetStr(&MissionSection, Race == 0 ? "German 1" : Race == 1 ? "Allied 1" : "Russian 1");   // 0x52c320 on +0xd8
    delete (SProperties*)MissionProps;                            // 0x660080 + delete 0x1c
    MissionProps = nullptr;
    delete (SProperties*)LocalProps;
    LocalProps = nullptr;
    MissionProps = new SProperties("missions.ini", true);         // new 0x1c, 0x65fe80(Format("missions.ini", ..), 1)
    LocalProps = new SProperties("missions_local.ini", true);
    SetStr(&MapName, GetMapName());                               // 0x592040, 0x52c320 on +0x20
    StartPrestige = Props(this)->GetInt(SStr(MissionSection), "SP", 0);   // 0x660500(section, "SP", 0) -> +0x28
    PrepareMission();                                             // 0x592d80
    MissionResult = 0;                                            // +0xe4
}

// PANZERS 0x592d80
// The start of a campaign mission (InitCampaignMode, LetResultsDone and the
// results' Restart): the mission properties, the support calls, the player
// counters, Prestige = StartPrestige, MissionArmy = Army plus the mission's
// own units ("Unit %d" = class, "Unit %d stored" = its crew, "Unit %d slot
// 1 / 2", unique ScriptIDs), the objectives; MenuToLoad 8 with an "Intro
// Anim" (every one is commented out in missions.ini), else 0 (briefing)
// when "Diary" (default 1), else 2 (game view).
void SPanzersCampaign::PrepareMission()
{
    PZ_M3_TRACE("SCampaign::PrepareMission (0x592d80)");
    LoadMissionProps();                                           // 0x593740
    LoadSupportCounts();                                          // 0x594140 (no world yet: nothing)
    for (int p = 0; p < 12; ++p) {                                // 0x594470
        unsigned char* r = PlayerStats[p];
        memset(r + 0x0c, 0, 4 * 12 * 4);                          // the 4 x 12 unit counters
        memset(r + 0xcc, 0, 12);                                  // +0xcc, +0xd0, +0xd4
    }
    Prestige = StartPrestige;                                     // +0x38 = +0x28
    ArmyCopyFrom(&MissionArmy, &Army);                            // 0x51e520(Army.Size) + 0x560290 each
    SUnitDef blank;                                               // 0x5cfb10
    const char* sec = SStr(MissionSection);
    for (int n = 1;; ++n) {
        char key[64];
        _snprintf(key, sizeof(key) - 1, "Unit %d", n);             // 0x51ee20("Unit %d", n)
        key[sizeof(key) - 1] = 0;
        const char* cls = Props(this)->GetString(sec, key, nullptr);   // 0x660570(section, key, 0)
        if (!cls)
            break;
        int idx = MissionArmy.Size;
        UnitDefCopy(ArmyAdd(&MissionArmy), &blank);               // grow (16, then 6/5), 0x560290(blank)
        SUnitDef* d = &MissionArmy.Array[idx];
        SetStr(&d->ClassName, cls);
        char cname[260];
        strncpy(cname, SStr(d->ClassName), sizeof(cname) - 1);
        cname[sizeof(cname) - 1] = 0;
        UniqueScriptID(&d->ScriptID, cname, &MissionArmy);        // 0x592640 -> +0x58
        _snprintf(key, sizeof(key) - 1, "Unit %d stored", n);
        const char* crew = Props(this)->GetString(sec, key, nullptr);
        if (crew) {
            d = &MissionArmy.Array[idx];
            SArmyArray st = { d->StoredUnits, d->StoredCount, d->StoredMax };
            UnitDefCopy(ArmyAdd(&st), &blank);                    // +0x4c SDArray add, 0x560290(blank)
            d->StoredUnits = st.Array;
            d->StoredCount = st.Size;
            d->StoredMax = st.Max;
            SUnitDef* c = &d->StoredUnits[0];
            SetStr(&c->ClassName, crew);
            strncpy(cname, SStr(c->ClassName), sizeof(cname) - 1);
            cname[sizeof(cname) - 1] = 0;
            UniqueScriptID(&c->ScriptID, cname, &MissionArmy);    // 0x592640 -> stored +0x58
        }
        d = &MissionArmy.Array[idx];
        _snprintf(key, sizeof(key) - 1, "Unit %d slot 1", n);
        d->Slots[0] = Props(this)->GetInt(sec, key, 0);            // 0x660500 -> +0x40
        _snprintf(key, sizeof(key) - 1, "Unit %d slot 2", n);
        d->Slots[1] = Props(this)->GetInt(sec, key, 0);            // -> +0x44
    }
    if (Props(this)->GetString(sec, "Intro Anim", nullptr))       // 0x660570(section, "Intro Anim", 0)
        MenuToLoad = 8;
    else
        MenuToLoad = Props(this)->GetInt(sec, "Diary", 1) != 0 ? PZ_MENU_BRIEFING : PZ_MENU_GAMEVIEW;
    LoadObjectives();                                             // 0x593ba0
    Logger.g->Log(0, "PZM4: campaign mission [%s] map %s, prestige %d, army %d (%d carried), MenuToLoad %d",
                  sec, GetMapName(), Prestige, MissionArmy.Size, Army.Size, MenuToLoad);
}

// PANZERS 0x594f60
void SPanzersCampaign::RestartMission()
{
    _014 = 0;
    PrepareMission();                                             // 0x592d80
}

// PANZERS 0x591e30
const char* SPanzersCampaign::GetBriefingText()
{
    SProperties* lp = (SProperties*)LocalProps;                  // +0x04 (0x591e3b)
    return lp ? lp->GetString(SStr(MissionSection), "Briefing text", "") : "";   // 0x660570
}

// PANZERS 0x5928b0
int SPanzersCampaign::GetNextMissionSP()
{
    const char* next = Props(this) ? Props(this)->GetString(SStr(MissionSection), "Next Mission", nullptr) : nullptr;
    if (!next || !*next)
        return 500;
    char name[260];
    strncpy(name, next, sizeof(name) - 1);
    name[sizeof(name) - 1] = 0;
    return Props(this)->GetInt(name, "SP", 0);                    // 0x660500(next, "SP", 0)
}

// PANZERS 0x596600
int SPanzersCampaign::GetMenuToLoad()
{
    return MenuToLoad;
}


// PANZERS 0x594d50
void SPanzersCampaign::OnMapLoaded()
{
    PZ_M3_TRACE("SCampaign::OnMapLoaded (0x594d50)");
    if (GameMode == PZ_GM_CAMPAIGN || GameMode == PZ_GM_SCENARIO || GameMode == PZ_GM_2) {
        if (Props(this) && Props(this)->GetInt(SStr(MissionSection), "Market", 1) == 0) {   // 0x660500
            MenuToLoad = PZ_MENU_GAMEVIEW;
            MissionResult = 0;
            return;
        }
    }
    MissionResult = 0;
    MenuToLoad = 2 - (StartPrestige != 0);                        // market when there is prestige to spend
}

// PANZERS 0x594e50
void SPanzersCampaign::SetMenuGameView()
{
    MenuToLoad = PZ_MENU_GAMEVIEW;
    MissionResult = 0;
}

// PANZERS 0x594e00
void SPanzersCampaign::LetMapDone()
{
    PZ_M3_TRACE("SCampaign::LetMapDone (0x594e00)");
    if (MenuToLoad != PZ_MENU_GAMEVIEW)
        Logger.g->Panic("SCampaign::LetMapDone: Menu order problem.");
    switch (GameMode) {
    case 1: case 2: case 3: case 4:
        MenuToLoad = PZ_MENU_RESULTS;
        break;
    case 5:
        MenuToLoad = 5;                                           // -> LoadNextCampaignView default: main menu
        break;
    default:
        break;
    }
}

// PANZERS 0x594e70
// The results menu's Continue (0x524d1). Campaign: the "Next Mission"
// becomes the section (none: the main menu), the mission army (its
// survivors, BackupCampaignUnits) becomes the carried army, StartPrestige = its "SP" + the prestige left, the "Before"
// save, PrepareMission. Multiplayer: the multiplayer menus; else the main
// menu.
void SPanzersCampaign::LetResultsDone()
{
    PZ_M3_TRACE("SCampaign::LetResultsDone (0x594e70)");
    _014 = 0;
    if (MenuToLoad != PZ_MENU_RESULTS)
        Logger.g->Panic("SCampaign::LetResultsDone: Menu order problem.");
    if (GameMode == PZ_GM_CAMPAIGN) {
        const char* next = Props(this) ? Props(this)->GetString(SStr(MissionSection), "Next Mission", nullptr) : nullptr;
        char name[260];
        name[0] = 0;
        if (next) {
            strncpy(name, next, sizeof(name) - 1);
            name[sizeof(name) - 1] = 0;
        }
        SetStr(&MissionSection, name);                            // 0x52c320 on +0xd8
        if (MissionSection.size == 0) {                           // +0xdc
            MenuToLoad = 5;
            return;
        }
        ArmyCopyFrom(&Army, &MissionArmy);                        // 0x591500(ECX +0x2c, +0x3c): the mission's survivors are carried
        StartPrestige = Props(this)->GetInt(SStr(MissionSection), "SP", 0) + Prestige;   // +0x28 = SP + +0x38
        PzStub_SaveGameBefore(this);                              // SPanzersCampaign::SaveGameBefore (agent S)
        PrepareMission();                                         // 0x592d80
        return;
    }
    if (GameMode == 4) {                                          // HD: && (no SMulti || !SMulti +0x512c)
        MenuToLoad = PZ_MENU_MULTI;
        return;
    }
    MenuToLoad = 5;
}

// PANZERS 0x592040
const char* SPanzersCampaign::GetMapName()
{
    switch (GameMode) {
    case 1: case 2: case 4: case 5: case 6:
        return SStr(MapName);
    case 3:
        return Props(this) ? Props(this)->GetString(SStr(MissionSection), "Map", "missing.scene") : "missing.scene";
    default:
        Logger.g->Panic("SCampaign::GetMapName: GameMode not set");
        return "";
    }
}

// PANZERS 0x594d40
bool SPanzersCampaign::IsTutorialMode() { return GameMode == PZ_GM_TUTORIAL; }

// PANZERS 0x594d30
bool SPanzersCampaign::IsScenarioMode() { return GameMode == PZ_GM_SCENARIO; }

// PANZERS 0x594d20
bool SPanzersCampaign::IsMultiMode() { return GameMode == 4; }

// PANZERS 0x596610
bool SPanzersCampaign::HasMarket()
{
    if (GameMode != 3 && GameMode != 1 && GameMode != 2)
        return true;
    return Props(this) && Props(this)->GetInt(SStr(MissionSection), "Market", 1) != 0;
}

// PANZERS 0x596650
bool SPanzersCampaign::PlaceMyUnits()
{
    if (_014)
        return false;
    if (GameMode != 3 && GameMode != 1 && GameMode != 2)
        return true;
    return Props(this) && Props(this)->GetInt(SStr(MissionSection), "PlaceMyUnits", 0) != 0;
}

// PANZERS 0x5920b0
int SPanzersCampaign::GetMissionResult() { return MissionResult; }

// PANZERS 0x597470
void SPanzersCampaign::SetMissionResult(int v) { MissionResult = v; }

// PANZERS 0x591e70
void SPanzersCampaign::GetMissionArmy(SArmyArray* out)
{
    ArmyCopyFrom(out, &MissionArmy);
}

// (recompile) the [<section>] "Mission code" key the save names use.
const char* SPanzersCampaign::GetMissionCode()
{
    return Props(this) ? Props(this)->GetString(SStr(MissionSection), "Mission code", "") : "";   // 0x660570
}

// PANZERS 0x593740
// <dir><section>_mission.ini for dir = maps/, scenarios/, multimaps/,
// multimaps/Assault/, multimaps/Factory/ (the first that exists), else
// missions.ini; the same for _mission_local.ini / missions_local.ini.
void SPanzersCampaign::LoadMissionProps()
{
    PZ_M3_TRACE("SCampaign::LoadMissionProps (0x593740)");
    delete Props(this);
    MissionProps = nullptr;
    delete (SProperties*)LocalProps;
    LocalProps = nullptr;
    static const char* const kDirs[5] = { "maps/", "scenarios/", "multimaps/", "multimaps/Assault/", "multimaps/Factory/" };
    for (int pass = 0; pass < 2; ++pass) {
        const char* suffix = pass == 0 ? "_mission.ini" : "_mission_local.ini";      // 0x7f974c / 0x7f975c
        const char* fallback = pass == 0 ? "missions.ini" : "missions_local.ini";
        void** slot = pass == 0 ? &MissionProps : &LocalProps;
        for (int i = 0; i < 5; ++i) {
            char path[400];
            _snprintf(path, sizeof(path) - 1, "%s%s%s", kDirs[i], SStr(MissionSection), suffix);
            path[sizeof(path) - 1] = 0;
            struct _stat st;
            if (FileSystem.Stat(path, &st) == 0) {                // 0x65faf0
                *slot = new SProperties(path, true);              // new 0x1c, 0x65fe80(path, 1)
                break;
            }
            if (i == 4)
                *slot = new SProperties(fallback, false);         // 0x65fe80(name, 0)
        }
    }
}

// PANZERS 0x593ba0
// Up to 9 objectives from [<section>] "Objective %d Text/Main/Hidden/Secret/
// Hero/Target %d X/Z". Only when the section is set (+0xdc).
void SPanzersCampaign::LoadObjectives()
{
    PZ_M3_TRACE("SCampaign::LoadObjectives (0x593ba0)");
    if (MissionSection.size == 0)
        return;
    for (int i = 0; i < ObjectiveCount; ++i) {                    // 0x591ba0(0): resize to 0
        FreeSString(&Objectives[i].Text);
        free(Objectives[i].Targets);
        memset((void*)&Objectives[i], 0, sizeof(Objectives[i]));  // (M4) the slot is reused: no stale Targets
    }
    ObjectiveCount = 0;
    SProperties* p = Props(this);
    const char* sec = SStr(MissionSection);
    for (int n = 1; n < 10 && p; ++n) {
        char key[64];
        sprintf(key, "Objective %d Text", n);
        SProperties* lp = (SProperties*)LocalProps;              // the text: campaign +0x04 (0x593c29)
        const char* text = lp ? lp->GetString(sec, key, nullptr) : nullptr;
        if (!text)
            break;
        // PANZERS 0x5915d0 (SDArray<SCampaignObjective>::Add)
        if (ObjectiveCount == ObjectiveMax) {
            int nmax = ObjectiveMax < 0x10 ? 0x10 : (ObjectiveMax * 6) / 5;
            Objectives = (SCampaignObjective*)realloc(Objectives, nmax * sizeof(SCampaignObjective));
            memset(Objectives + ObjectiveMax, 0, (nmax - ObjectiveMax) * sizeof(SCampaignObjective));
            ObjectiveMax = nmax;
        }
        SCampaignObjective* o = &Objectives[ObjectiveCount++];
        SetStr(&o->Text, text);
        o->State = 0;
        sprintf(key, "Objective %d Main", n);
        o->Main = p->GetInt(sec, key, 0) != 0;
        sprintf(key, "Objective %d Hidden", n);
        o->Hidden = p->GetInt(sec, key, 0) != 0;
        sprintf(key, "Objective %d Secret", n);
        o->Secret = p->GetInt(sec, key, 0) != 0;
        sprintf(key, "Objective %d Hero", n);
        o->Hero = p->GetInt(sec, key, 0) != 0;
        for (int t = 1; t < 10; ++t) {
            sprintf(key, "Objective %d Target %d X", n, t);
            float x = p->GetFloat(sec, key, 0.0f);
            sprintf(key, "Objective %d Target %d Z", n, t);
            float z = p->GetFloat(sec, key, 0.0f);
            if (x == 0.0f || z == 0.0f)
                break;
            // PANZERS 0x550500 (SDArray<{x, z}>::Add)
            if (o->TargetCount == o->TargetMax) {
                int nmax = o->TargetMax < 0x10 ? 0x10 : (o->TargetMax * 6) / 5;
                o->Targets = (float*)realloc(o->Targets, nmax * 8);
                memset(o->Targets + o->TargetMax * 2, 0, (nmax - o->TargetMax) * 8);
                o->TargetMax = nmax;
            }
            o->Targets[o->TargetCount * 2] = x;
            o->Targets[o->TargetCount * 2 + 1] = z;
            o->TargetCount++;
        }
    }
    // HD then reads [<section>] "Map" and loads the briefing text 0x597390
    // ("txt") / 0x594540 (the objectives screen text): not on the CRC path.
    if (p)
        Logger.g->Log(1, "STUB: SCampaign::LoadObjectives briefing text (0x597390 / 0x594540) not loaded");
}

// PANZERS 0x594140
// The support calls each player may make: no section -> 2 / 5 / 3 / 3 / 3
// for every player; multiplayer -> the section values for every player;
// else the section values for player 0.
void SPanzersCampaign::LoadSupportCounts()
{
    PZ_M3_TRACE("SCampaign::LoadSupportCounts (0x594140)");
    SWorld* w = g_World;
    if (!w)
        return;
    if (MissionSection.size == 0) {
        for (int i = 0; i < 12; ++i) {
            int* r = (int*)(w->Players[i] + 0x2c);                // World+0x19c + i*0x48
            r[0] = 2; r[1] = 5; r[2] = 3; r[3] = 3; r[4] = 3;
        }
        return;
    }
    SProperties* p = Props(this);
    const char* sec = SStr(MissionSection);
    int n = GameMode == 4 ? 12 : 1;
    for (int i = 0; i < n; ++i) {
        int* r = (int*)(w->Players[i] + 0x2c);
        r[0] = p ? p->GetInt(sec, "Cannonade", 0) : 0;
        r[1] = p ? p->GetInt(sec, "ReconPlane", 0) : 0;
        r[2] = p ? p->GetInt(sec, "TacBomber", 0) : 0;
        r[3] = p ? p->GetInt(sec, "HeavyBomber", 0) : 0;
        r[4] = p ? p->GetInt(sec, "Parachute", 0) : 0;
    }
    // HD: multiplayer (DAT_008f1a74, mode != 3) clears the calls the host
    // turned off; no SMulti in the recompile.
}

// ---------------------------------------------------------------------------
// Save

// PANZERS 0x5966a0
// SaveGames/<file>: 'SAVE' chunk {'v4pa', 1, map, mission code, title, mode,
// race, start prestige, Army, MissionArmy, prestige, section, +0xe4,
// difficulty, +0xf8, +0xfc, +0xb84, objectives, 12 player records, +0xb60,
// then the game state (SGameLogic 0x57e110)}. docs/FORMATS.md.
bool SPanzersCampaign::SaveGame(const char* file, const char* title)
{
    PZ_M3_TRACE("SPanzersCampaign::SaveGame (0x5966a0)");
    if (GameMode == 4)
        return false;
    SString dir;
    FileSystem.FileNameProcess(&dir, "SaveGames");                // 0x65f020
    CreateDirectoryA(SStr(dir), nullptr);                         // 0x65fde0 (MakeDir)
    FreeSString(&dir);
    SStream* s = FileSystem.OpenWrite(file, nullptr);             // 0x65f7a0
    if (!s)
        return false;
    s->WriteSignature();                                          // 0x65dc60
    s->WriteChunkStart(0x45564153);                               // 'SAVE'
    s->WriteInt(0x61703476);                                      // 'v4pa'
    s->WriteInt(1);
    s->WriteString(GetMapName());
    s->WriteString(GetMissionCode());
    s->WriteString(title ? title : "");
    s->WriteInt(GameMode);
    s->WriteInt(Race);
    s->WriteInt(StartPrestige);
    s->WriteInt(Army.Size);
    for (int i = 0; i < Army.Size; ++i)
        UnitDefSave(&Army.Array[i], s);
    s->WriteInt(MissionArmy.Size);
    for (int i = 0; i < MissionArmy.Size; ++i)
        UnitDefSave(&MissionArmy.Array[i], s);
    s->WriteInt(Prestige);
    s->WriteString(SStr(MissionSection));
    s->WriteInt(MissionResult);                                     // +0xe4
    s->WriteInt(Difficulty);
    s->WriteInt(_0f8);
    s->WriteInt(_0fc);
    s->WriteInt(_b84);
    s->WriteInt(ObjectiveCount);
    for (int i = 0; i < ObjectiveCount; ++i) {
        s->WriteByte((unsigned char)Objectives[i].Hidden);        // +4 (byte)
        s->WriteInt(Objectives[i].State);                         // +0
    }
    for (int pl = 0; pl < 12; ++pl) {
        const int* r = (const int*)PlayerStats[pl];
        // 12 x {+0x0c, +0x3c, +0x6c, +0x9c} (index i), then +0xcc, +0xd4.
        for (int i = 0; i < 12; ++i) {
            s->WriteInt(r[3 + i]);
            s->WriteInt(r[0xf + i]);
            s->WriteInt(r[0x1b + i]);
            s->WriteInt(r[0x27 + i]);
        }
        s->WriteInt(r[0x33]);
        s->WriteInt(r[0x35]);
    }
    s->WriteInt(Score);                                           // +0xb60
    if (g_GameLogic)
        g_GameLogic->SaveGameState(s);                            // 0x57e110
    s->WriteChunkEnd();                                           // 0x65db10
    s->Release();
    return true;
}

// PANZERS 0x596e30
int SPanzersCampaign::SaveGameStartMission(const char* name, const char* title)
{
    PZ_M3_TRACE("SPanzersCampaign::SaveGameStartMission (0x596e30)");
    // title2 = "<title> - <map>" (" - " 0x7f92bc, map 0x5925d0)
    char full[600];
    _snprintf(full, sizeof(full) - 1, "%s - %s", title ? title : "", GetMapName());
    full[sizeof(full) - 1] = 0;
    char file[400];
    _snprintf(file, sizeof(file) - 1, "SaveGames/%s-%s.save", GetMissionCode(), name);
    file[sizeof(file) - 1] = 0;
    if (!SaveGame(file, full))
        Logger.g->Panic("SPanzersCampaign::SaveGameStartMission() - failed");
    return 1;
}

bool SPanzersCampaign::LoadGame(const char* file)
{
    STUB_LOG("SPanzersCampaign::LoadGame (0x594f70)");
    PZ_M3_TRACE("SPanzersCampaign::LoadGame (0x594f70)");
    (void)file;
    return false;
}

// ---------------------------------------------------------------------------
// Replay header

// PANZERS 0x596fc0
void SPanzersCampaign::WriteReplayHeader(SStream* s)
{
    PZ_M3_TRACE("SPanzersCampaign::WriteReplayHeader (0x596fc0)");
    s->WriteSignature();
    s->WriteChunkStart(0x45564153);                               // 'SAVE'
    s->WriteInt(0x61703476);                                      // 'v4pa'
    s->WriteInt(3);
    s->WriteString(GetMapName());
    s->WriteInt(GameMode);
    s->WriteInt(Race);
    s->WriteInt(StartPrestige);
    s->WriteInt(MissionArmy.Size);
    for (int i = 0; i < MissionArmy.Size; ++i)
        UnitDefSave(&MissionArmy.Array[i], s);
    s->WriteInt(Prestige);
    s->WriteString(SStr(MissionSection));
    s->WriteInt(MissionResult);
    s->WriteInt(Difficulty);
    s->WriteInt(_0f8);
    for (int i = 0; i < 12; ++i) {
        const int* r = (const int*)(g_World->Players[i] + 0x2c);  // World+0x19c + i*0x48
        for (int k = 0; k < 5; ++k)
            s->WriteInt(r[k]);
    }
    s->WriteChunkEnd();
}

static void ReadStr(SStream* s, SString* out)
{
    // 0x65d6f0 / 0x56e7d0: u16 length, bytes
    FreeSString(out);
    int n = s->ReadWord() & 0xffff;
    out->size = n;
    out->buf = new char[n + 1];
    s->Read(out->buf, n);
    out->buf[n] = 0;
}

// PANZERS 0x51f860 (SDArray<SUnitDef>::Load)
static void ArmyLoad(SArmyArray* a, SStream* s)
{
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        throw "Invalid array size";
    ArmyResize(a, (int)n);
    for (int i = 0; i < a->Size; ++i) {
        memset((void*)&a->Array[i], 0, sizeof(SUnitDef));
        a->Array[i].Load(s);                                      // 0x5cfbd0
    }
}

static void CheckSaveHeader(SStream* s)
{
    s->ReadSignature();                                           // 0x65d6a0
    if (s->ReadChunkHeader() != 0x45564153)                       // 'SAVE'
        throw "Not a map file";
    if (s->ReadInt() != 0x61703476)                               // 'v4pa'
        throw "Unsupported map file version";
}

// PANZERS 0x595780
// Called by SGameLogic::StartPacketPlayback 0x580540 at mission start (after
// PlaceAllUnits): restores the campaign and the players' support calls, then
// MissionArmy = Army (the bought units are gone again).
void SPanzersCampaign::ReadReplayHeader(SStream* s)
{
    PZ_M3_TRACE("SPanzersCampaign::ReadReplayHeader (0x595780)");
    CheckSaveHeader(s);
    if (s->ReadInt() != 3)
        throw "Not a replay file";
    SString map;
    ReadStr(s, &map);
    SetStr(&MapName, SStr(map));
    FreeSString(&map);
    GameMode = s->ReadInt();
    Race = s->ReadInt();
    StartPrestige = s->ReadInt();
    ArmyLoad(&MissionArmy, s);                                    // 0x51f860 on +0x3c
    Prestige = s->ReadInt();
    ReadStr(s, &MissionSection);                                  // 0x56e7d0 on +0xd8
    MissionResult = s->ReadInt();
    Difficulty = s->ReadInt();
    _0f8 = s->ReadInt();
    for (int i = 0; i < 12; ++i) {
        int* r = (int*)(g_World->Players[i] + 0x2c);
        for (int k = 0; k < 5; ++k)
            r[k] = s->ReadInt();
    }
    s->ReadChunkValidate(0);                                      // 0x65d460
    LoadObjectives();                                             // 0x593ba0
    GameMode = PZ_GM_TUTORIAL;
    MenuToLoad = PZ_MENU_GAMEVIEW;
    Prestige = StartPrestige;
    // 0x51e520 on +0x3c with the size of +0x2c, then +0x3c[i] = +0x2c[i]
    ArmyCopyFrom(&MissionArmy, &Army);
    MissionResult = 0;
}

// PANZERS 0x597510
// OnAction 0x494c2 (Load Replay): +0x12c = name, then reads the header of
// Replays/<name> before the map is loaded: mode, race, prestige and the
// mission army. HD reads the 12 player records without the +0xf8 dword the
// header has before them; with no world (the action unloads the menu world
// first) it only skips them. Mission start replays +0x12c.
void SPanzersCampaign::StartReplay(const char* name, int len)
{
    PZ_M3_TRACE("SPanzersCampaign::StartReplay (0x597510)");
    FreeSString(&ReplayName);
    if (len != 0) {
        ReplayName.size = len;
        ReplayName.buf = new char[len + 1];
        memcpy(ReplayName.buf, name, len + 1);
    }
    char path[400];
    _snprintf(path, sizeof(path) - 1, "Replays/%s", SStr(ReplayName));   // 0x5335c0("Replays/", +0x12c)
    path[sizeof(path) - 1] = 0;
    SetStr(&ReplayName, path);
    SStream* s = FileSystem.OpenRead(SStr(ReplayName), "SPanzersCampaign::StartReplay");   // 0x65f420
    if (!s)
        return;
    if (s->ReadByte() != 3)
        Logger.g->Panic("SPanzersCampaign::StartPacketPlayback: Unsupported file version");
    CheckSaveHeader(s);
    // HD reads the map name right after 'v4pa'. The header that -packetrec
    // writes (0x596fc0) has the version dword 3 there, so HD misreads every
    // version 3 file (the replay screen was cut; docs/M3_REPLAY.md). The
    // recompile skips that dword (deviation); PZ_M3_REPLAY_HD=1 keeps HD's read.
    static const bool s_HdRead = getenv("PZ_M3_REPLAY_HD") != nullptr;
    if (!s_HdRead && s->ReadInt() != 3)
        throw "Not a replay file";
    SString map;
    ReadStr(s, &map);
    SetStr(&MapName, SStr(map));
    FreeSString(&map);
    GameMode = s->ReadInt();
    Race = s->ReadInt();
    StartPrestige = s->ReadInt();
    ArmyLoad(&MissionArmy, s);                                    // 0x51f860 on +0x3c
    Prestige = s->ReadInt();
    ReadStr(s, &MissionSection);                                  // 0x56e7d0 on +0xd8
    MissionResult = s->ReadInt();
    Difficulty = s->ReadInt();
    // HD then reads the 12 player records without the +0xf8 dword before
    // them (one dword short; harmless without a world, as here).
    for (int i = 0; i < 12; ++i) {
        if (!g_World) {
            for (int k = 0; k < 5; ++k)
                s->ReadInt();
        } else {
            int* r = (int*)(g_World->Players[i] + 0x2c);
            for (int k = 0; k < 5; ++k)
                r[k] = s->ReadInt();
        }
    }
    s->Release();                                                 // vtbl +0 (1)
}

} // namespace pz
