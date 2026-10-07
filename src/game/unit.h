// src/game/unit.h
// pz::SUnit, the HD unit base class (vftable 0x7fcc24, 0x340 bytes;
// 0x5b2800..0x5c6300). OWNER: agent U.
//
// Every concrete unit class derives from SUnit and keeps its prototype again
// at +0x340 (SSingleUnit 0x3a8, SBuildingUnit 0x470, SPanzersSquadUnit 0x3a4,
// SPanzersSquadMemberUnit 0x354; singleunit.h, buildingunit.h, squadunit.h).
// Fields are at their HD offsets (ctor 0x5b2820, dtor 0x5b33f0, Init
// 0x5ba8e0 and the slot bodies); unnamed ones stay "_xxx".
//
// Lifted bodies carry "// PANZERS 0xADDR"; slots that are still logged stubs
// keep STUB_LOG + PZ_M2_TRACE (docs/M2_INTERFACES.md).

#ifndef PZ_GAME_UNIT_H
#define PZ_GAME_UNIT_H

#include "iunit.h"
#include "string2.h"
#include "punit.h"

namespace pz {

struct SGunner;
struct SIDriver;
struct SIPDriver;
struct SPGunner;

// +0x16c element (8 bytes): a unit stored in a vehicle or building.
struct SUnitStored {
    int Unit;            // world index
    int Mode;            // StoreUnit mode (2 for heroes)
};

// +0x178 element (0x18 bytes): squad members, and the units a vehicle
// carries in its seats (SUnit::StoreUnit 0x5c30d0).
struct SUnitMember {
    int  Unit;           // +0x00 world index
    int  Owner;          // +0x04 world index of the squad / carrier
    int  Seat;           // +0x08 1 driver, 2 gunner, 3 gunner (seat), 4 passenger
    int  Gunner;         // +0x0c gunner index (seats 2/3)
    int  Node;           // +0x10 attach node (UnitsInVehicle)
    bool Attached;       // +0x14
    unsigned char _15[3];
};

// HD SDEQueue<SGhostFrame> (ghost frames of the drivers, element 0x74).
struct SUnitGhostQueue {
    void* Array;         // +0x00
    int   Count;         // +0x04 (hashed by the world CRC: unit +0x1dc)
    int   Max;           // +0x08
    int   Base;          // +0x0c
    int   Top;           // +0x10 -1 when empty
    int   Bottom;        // +0x14
};

// --- M3-C5: the class record SIUnit +0x1c hands out (HD .data, 0x20 bytes:
// name, 6, size?, parent record, 0, 0, property table, property count). The
// recompile keeps the name, the parent and the HD address; the property
// tables (0x7f9d8c, ...) belong to the save code (agent F).
struct SUnitClassDesc {
    const char*           Name;      // +0x00 "SUnit", "Special"
    const SUnitClassDesc* Parent;    // +0x0c
    unsigned              HdAddr;    // the HD record
};
extern const SUnitClassDesc kUnitClassDesc_8dc540;   // "Special" (SUnit 0x5ba220)
extern const SUnitClassDesc kUnitClassDesc_8dc0b0;   // "SUnit" (SPanzersSquadUnit 0x59bfd0)
extern const SUnitClassDesc kUnitClassDesc_8dbff0;   // "SUnit" (SPanzersSquadMemberUnit 0x5988f0)
extern const SUnitClassDesc kUnitClassDesc_8da7f8;   // "SUnit" (SBuildingUnit 0x548b20)
// --- end M3-C5

// HD SUnit base (vftable 0x7fcc24).
struct SUnit : SIUnit {
    SUnit(SPUnit* proto, int worldIndex);                        // 0x5b2820
    ~SUnit() override;                                           // 0x5b33f0
    void Uninit() override;                                      // 0x5b7e40
    void Init(SUnitDef* def) override;                           // 0x5ba8e0
    void InitNew(int player, const float* pos, float dir, int p4, float hp) override; // 0x5bace0
    void Slot_10() override;
    void Slot_14() override;
    void Slot_18() override;
    void GetClassDescriptor(void** obj, const SUnitClassDesc** desc) override;// 0x5ba220
    void Hook20(int p1) override;                                // 0x54cd40
    void SetPosition(float x, float z, int dirBits, int yrelBits) override;  // 0x5c1980
    void SetWreckModel() override;                               // 0x5be2b0
    void ServerRefresh(int frame) override;                      // 0x5bee90
    void RefreshDead() override;                                 // 0x5bd900 (empty)
    void RefreshTargeting() override;                            // 0x5bd600
    void RefreshMisc() override;                                 // 0x5bdee0
    void RefreshModel() override;                                // 0x5c6130
    void UpdateVisuals(SIViewport* vp) override;                 // 0x5b76c0
    void SpeakSelected() override;                               // 0x5bce20
    void OnDriverReachedTarget() override;
    void Unplace() override;                                     // 0x5ba850
    void Place(float x, float z, float dir) override;            // 0x5c5160
    bool Slot_54() override;                                      // 0x546ab0
    bool Slot_58(int p1) override;                                // 0x5468d0
    bool StoreUnit(int unit, int mode) override;                 // 0x5c30d0
    void Slot_60(int p1) override;                                // 0x5468c0
    bool UnloadUnit(int unit) override;                          // 0x5c51d0
    void UnloadAll() override;                                   // 0x5c6000
    void Tow(int unit) override;                                 // 0x5c2390 (trainunit.cpp)
    void Remove(bool p1) override;                               // 0x5c2df0
    void GhostFramesAddTop(float x, float y, float z, float dir, float dist, SIUnit* towed,
                           float* outPos, float* outDir, float* outDist) override;   // 0x5b5c10 (trainunit.cpp)
    float* GetEntrance(float* out3) override;                    // 0x55ce70
    float GetEntranceDir() override;                             // 0x55ce60
    bool HasWoundedMember() override;                            // 0x54a240
    void SetRankXP(int rank) override;
    int GetRank() override;                                      // 0x5b9e60
    void AddXP(int victim, float xp, int p3) override;           // 0x55cee0 (empty)
    int ShotsToKill(const float* from, int attacker) override;   // 0x5b6040 (combat.cpp)
    void TakeDamage(float damage, int weaponType, int attacker, float x, float y, float z, int hitMode) override;   // 0x5c4080
    void Heal(float amount) override;                            // 0x5c5050
    void SetFlag112(bool on) override;
    void SetCurrentTarget(STarget* target, int p2) override;     // 0x5c0d10
    void EC_Default(int p1, int p2) override;
    int ActionOn(int target) override;                           // 0x5ba3e0
    void EC_Move(int xBits, int zBits, int p3, bool p4, int p5) override;   // 0x5b8ea0
    void EC_MoveReverse(int xBits, int zBits, int p3, bool p4, int p5) override;   // 0x5b8d20
    void EC_MoveAlongPath(int path, int p2, int p3) override;    // 0x5b8e20
    void EC_Follow(int unit, int p2) override;                   // 0x5b8ba0
    void EC_TurnTo(int dirBits) override;                        // 0x5b9370
    void Stop() override;                                        // 0x5b91b0
    void ClearTargets() override;                                // 0x5b90b0
    void Slot_C8() override;
    void EC_AttackMove(float x, float z, int queue) override;   // 0x5b8660
    void EC_AttackAlongPath(int path, int node, int queue) override;   // 0x5b8740
    void EC_AssaultBuilding(int p1, int p2) override;
    void EC_Tow(int unit) override;                              // 0x5b8fa0
    void EC_AttackMoveUnit(int unit) override;                   // 0x5b8c90
    void Slot_E0(int p1, int p2) override;                        // 0x547b90
    void EC_AttackPos(int xBits, int zBits, int p3) override;    // 0x5b8570
    void EC_Attack(int unit, int queue) override;                // 0x5b8440
    void StopGunners() override;                                 // 0x5b9110
    void EC_ThrowGrenade(int unit, int p2) override;              // 0x5478c0
    void EC_ThrowMolotov(int unit, int p2) override;              // 0x5478e0
    void EC_ThrowMagneticMine(int unit, int p2) override;         // 0x5478d0
    void Slot_FC(int p1, int p2, int p3) override;                // 0x547b30
    void Slot_100(int p1, int p2, int p3) override;               // 0x548180
    void Slot_104() override;                                     // 0x547e80
    void Slot_108() override;                                     // 0x5478f0
    void Slot_10C(int p1, int p2, int p3) override;               // 0x547ba0
    void Slot_110(int p1, int p2) override;                       // 0x547b80
    void Slot_114(int p1, int p2) override;                       // 0x547b40
    void Slot_118(int p1) override;                               // 0x547b50
    void Slot_11C() override;                                     // 0x547b60
    void Slot_120(int p1) override;                               // 0x547b70
    void EC_Die() override;
    void EC_Destroy() override;                                  // 0x5b88d0
    void SetBehavior(int behavior) override;                     // 0x5b8960
    void SetHealthPercent(float percent) override;                // 0x54cd50
    void Slot_134(int p1) override;                              // 0x55cdf0
    void EC_ChangeActiveDriver(int driver) override;              // 0x547900
    void SetFireBehavior(int behavior) override;                 // 0x5b8920
    void EC_Enter(int unit, int queue) override;                 // 0x5b87f0
    void Unload(int index) override;                             // 0x5b93f0
    void Slot_148(int player) override;                               // 0x547e70
    void Slot_14C(int player) override;                               // 0x5481a0
    void Slot_150(int p1) override;                               // 0x548190
    void Slot_154() override;                                     // 0x5481b0
    void Slot_158(int p1) override;                              // 0x55ce50
    void Slot_15C(int p1) override;                               // 0x547e60
    void Slot_160(int p1) override;                               // 0x5478b0
    void Slot_164(int p1) override;                              // 0x55ce40
    void Slot_168(bool on) override;                             // 0x5b9360 (empty)
    void StoreInterpolationState() override;                     // 0x5b5ad0
    float GetHealth() override;                                  // 0x5b9e30
    float GetHitPoints() override;                               // 0x5b9ec0
    float GetLowestMaxRange() override;                          // 0x5b9d90
    float GetMaxRange(int weapon) override;                      // 0x5b9f60
    float GetMinRange(int weapon) override;                      // 0x5b9f90
    float GetSightRange() override;                              // 0x5ba240
    float GetExtra188() override;                                // 0x548230
    float Slot_18C() override;                                    // 0x548380
    void AI_Heartbeat() override;                                // 0x5b37d0
    void OnAttackedBy(int attacker) override;                    // 0x55e330 (empty)
    void SetOnBlockMap(bool on) override;                        // 0x5bc660
    void Slot_19C() override;                                    // 0x5b74c0
    void MarkBlockMap(bool on, int p2, int p3, int p4, short p5) override;  // 0x5bc6d0
    int TestBlockMapPath(int p1, int p2, int p3, int p4, short p5, int p6, int p7) override; // 0x5b7520
    int TestBlockMap(int p1, int p2, int p3, int p4, short p5) override;    // 0x5b7500
    void RestoreBehavior() override;                             // 0x546ac0
    float GetMoveSpeed(int p1) override;                         // 0x5b9ed0
    void OnMemberDied(int unit) override;                        // 0x55cf70 (empty)
    void SetSpecialAnimation(const char* name) override;          // 0x5c1da0
    bool IsCapturable() override;                                // 0x55cf60
    void ServerRefreshMedic(float dt) override;                  // 0x5bfa50
    void SetUnitSize() override;                                 // 0x5c21d0
    void GetCenterPosition(float* out) override;                 // 0x5b9d40

    // --- M3 (docs/M3_INTERFACES.md, agent C): the save path of every
    // mission start (SPanzersCampaign::SaveGameStartMission 0x596e30 ->
    // SaveGame 0x5966a0 -> per unit). Load runs only for Load Game.
    void Save(struct SStream* s);                                // 0x5be320 (1) SUnit::Save
    void Load(struct SStream* s);                                // 0x5bbd30 (1) SUnit::Load

    // --- M2-I sub-agent UB (SUnit AI, targeting, orders, effects): add declarations here only.
    // Element types of the unit arrays decoded by UB (unitai.cpp).
    struct SNearUnit {                   // +0x1a8 / +0x1b4 element (8 bytes)
        int  Unit;                       // +0x00 world index
        bool Blocked;                    // +0x04 +0x1a8 only: the unit blocks this one's footprint (0x5b78a0)
        unsigned char _5[3];
    };
    struct SInvalidTarget {              // +0x1c0 element (0x10 bytes), CollectInvalidTargets 0x5b76e0
        int  Unit;                       // +0x00 world index of the target the driver got stuck on
        int  Frame;                      // +0x04 logic frame it was (re)marked
        int  Count;                      // +0x08 times marked (max 5)
        bool AttackOk;                   // +0x0c 0x5bb6b0(unit, false) fails while clear
        bool FollowOk;                   // +0x0d 0x5bb6b0(unit, true) fails while clear
        unsigned char _e[2];
    };
    struct SOrder {                      // +0x19c element (0x1c bytes): a queued command (0x5bb980)
        int   Command;                   // +0x00 ExecuteCommand 0x5b95a0 case
        float X;                         // +0x04
        float Z;                         // +0x08
        int   Unit;                      // +0x0c
        int   Param;                     // +0x10 (path, behaviour, ...)
        int   Param2;                    // +0x14
        bool  Queue;                     // +0x18 passed on as the last argument of the EC_ slot
        unsigned char _19[3];
    };
    struct SPoint2 {                     // +0x308 element (8 bytes): a copy of the driver's global path (0x5bee10)
        float X;
        float Z;
    };

    // Targeting (unitai.cpp).
    void FillNearUnits(float sightRange, float nearRange);       // 0x5b78a0 (+0x1b4 in sight range, +0x1a8 near)
    bool IsTargetable(int unit, bool follow);                    // 0x5bb6b0 (live, placed, not marked invalid in +0x1c0)
    bool IsWaitingAfterStuck();                                  // 0x5bb400
    bool IsHiddenInBlockMap();                                   // 0x5bb5c0 (the unit stands in a static block)
    bool IsAIDefault();                                          // 0x5bb470 AI player, no AI group, behaviour 1
    bool HasSlotWeapon(int weapon);                              // 0x5ba820 (+0x138 / +0x144)
    int  FindTarget(int mode, SGunner* gunner, float minRange, float maxRange, bool canMove);   // 0x5b4720 (-1 = none)
    void EC_Repair(int unit);                                    // 0x5c1ca0 (name guessed) current target kind 6 on the unit
    void EC_Supply(int unit);                                    // 0x5c1e60 (name guessed) current target kind 7 on the unit
    void AutoRepairSupply(float supplyLevel);                    // 0x5bd610 (repairers / supporters)
    int  GetBuildingAction(int unit);                            // 0x5ba2b0 (-1 = not a building)
    bool NeedsSupply(float level);                               // 0x5bc700
    bool NeedsRepair();                                          // 0x5bc840 (stub)
    // Per tick (ServerRefresh 0x5bee90).
    void RefreshDriverEffects();                                 // 0x5bd910
    void RefreshInvalidTargets();                                // 0x5bdcd0
    void CollectInvalidTargets();                                // 0x5b76e0
    void EnableStaticEffects();                                  // 0x5c2200
    bool GetMovingDriver(SIDriver** out);                        // 0x5b9bf0 refresh driver, else the active one
    bool GetActivePDriver(SIPDriver** out);                      // 0x5b9c70
    bool GetMainPGunner(SPGunner** out);                         // 0x5b9ce0 (the driver's aimer)
    bool IsPosInGunnerArc(float x, float y, float z);            // 0x5b6fd0 (main gunner 0x583990)
    void OnDriverStucked();                                      // 0x5bcd20
    void StopUnit();                                             // 0x5c2d30 (targets, gunners, driver; teleport)
    void DriverDropGlobalPath();                                 // 0x5b5b50 (driver 0x550730)
    void DriverDropLocalPath();                                  // 0x5b5ba0 (driver 0x550780)
    void CopyDriverWayPoints();                                  // 0x5bee10 (driver +0x94 into +0x308)
    // Order queue (+0x19c).
    void OrderAt(int command, const float* xz, bool queue, bool add);   // 0x5bb980
    void OrderParam(int command, int param, bool queue, bool add);      // 0x5bbb60
    void OrderUnit(int command, int unit, bool queue, bool add);        // 0x5bb8a0
    void ExecuteCommand(const SOrder& order);                    // 0x5b95a0
    // Callable forms for the driver environment (g_DriverEnv, driverunit.cpp).
    static void   EnvOnDriverStucked(SIUnit* unit);              // 0x5bcd20
    static bool   EnvIsPosInRange(SIUnit* unit, float x, float y, float z);   // 0x5b6fd0
    static void*  EnvGetAimer(SIUnit* unit);                     // 0x5b9ce0 (SPGunner*, +0x28 = fire start arc)
    static float* EnvGetEntrance(SIUnit* unit, float* out);      // +0x78: 0x55ce70 / SBuildingUnit 0x548200
    static void   EnvOnDriverReachedTarget(SIUnit* unit);        // +0x48 (SUnit 0x5bcb60)
    static void   EnvSlot9C(SIUnit* unit, int p1);               // +0x9c (SUnit 0x5c1d40: +0x112 = p1)
    // --- end UB
    // --- M3 C sub-agents (docs/M3_INTERFACES.md row C): add SUnit declarations only inside your own block.
    // --- C1 (gunner / projectile)
    void DisableStaticEffects();                                 // 0x5ba860 pixie +0x60(effect, 0) for each +0x314, EffectTimer = 30 (gunner.cpp)
    void LockDriver();                                           // 0x5b8260 Stop, active driver -> -1, movement group out, +0x34 = 1 (gunner.cpp; magnetic mine)
    // --- end C1
    // --- C2 (damage / death / XP / unit slots)
    void DamageMembers(bool last, float damage, int weaponType, int attacker, float x, float y,
                       float z, int hitMode);                    // 0x5c3dd0 crew / gun crew damage (combat.cpp)
    bool CanStoreUnit(int unit);                                 // 0x5b7040 may `unit` get in (unitbase.cpp)
    int  UnloadByMode(int mode);                                 // 0x5c6090 first stored unit of that mode out (+0x64), its index or -1
    void PlayDeathEffects();                                     // 0x5c26e0 prototype +0x108 (+0x120 when burnt) (unitbase.cpp)
    void PlayDiedByFireEffects();                                // 0x5c2590 prototype +0x114
    void PlayDestroyEffects();                                   // 0x5c2910 prototype +0x120
    void SpeakMoveOrder();                                       // 0x5bcdf0 speech 3 / 4
    void HandSelectionTo(SUnit* to);                             // 0x5c50c0 +0x104 bit 0 and +0x108 go to `to`
    bool IsRecent26c(int player);                                // 0x5b6e10 +0x26c[player] within 40 ticks
    bool WasSeenRecently(int player);                            // 0x5b6e40 +0x29c[player] within 40 ticks
    // --- end C2
    // --- C3 (air support / waster)
    // --- end C3
    // --- C4 (AI / squads / buildings / unitai)
    int  FindVehicleToEnter();                                   // 0x5b5090 (-1 = none) AI squads: an empty armed vehicle in sight
    void AttackUnit(int unit, int gunner, bool canMove);         // 0x5b53d0 gunner or unit attack order, then the squad item
    void EnterVehicle(int unit);                                 // 0x5b5810 STarget kind 9, +0x18c / vehicle +0x190
    // --- end C4
    // --- C5 (squads / buildings / drivers)
    void SetItemAutoUse(int slot, bool on);                      // 0x5b88f0 (+0x140 / +0x14c when the slot holds an item)
    void RemoveStoredMember(int i);                              // 0x5be140 SUnit::RemoveStoredMember (seat cleanup, +0x178 Remove)
    void LeaveDriverSeat();                                      // 0x5c1d50 (name guessed) stop, park the active driver, leave the movement group
    void TransferSelection(SUnit* to);                           // 0x5c50c0 (name guessed) selected bit +0x104 and +0x108 move to `to`
    // --- end C5
    // --- M2-I sub-agent SQ (helpers squads need on SUnit): add declarations here only.
    // --- end SQ
    // --- M2-I sub-agent BW (helpers buildings / SWorld need on SUnit): add declarations here only.
    // --- end BW

    // Non-virtual SUnit helpers (HD thiscall functions).
    void SetActiveDriver(int index);                             // 0x5c0cb0
    void SetGlobalState(int state, int p2);                      // 0x5b7390
    void SetAIGroup(int group);                                  // 0x5c0c10
    void InitModel();                                            // 0x5b7d70
    void InitMoveFlags();                                        // 0x5c1170
    void CreateStaticEffects();                                  // 0x5c2a70
    void InitEffects();                                          // 0x5c2c30
    void StopEffects();                                          // 0x5c3030
    void SetParent(int unit) { Parent = unit; }                  // 0x5c1600
    SIDriver* GetDriver(int index);                              // 0x55cc00
    SGunner* GetGunner(int index);                               // 0x55cc40
    void SetTarget(STarget** slot, STarget* t);                  // refcount swap (inline in HD)
    void UpdateSeenByPlayers();                                  // 0x5bc5c0
    void RefreshRepairTarget(float range);                       // 0x5c01e0 (target kind 6)
    void RefreshSupplyTarget(float range);                       // 0x5bf280 (target kind 7)

    // M1 visuals until agent A's animations land (unit.cpp): the walker
    // stand sequence, the building node hiding, the idle tick.
    void M1InitModelPose();
    void M1RefreshModel();

    // +0x000 vptr
    SPUnit*          Proto;          // +0x004
    SIModel*         Model;          // +0x008 InitModel (scene +0x54 / +0x58)
    SIModel*         Model2;         // +0x00c prototype +0x5c model (flags 0xb)
    SIModel*         FlagModel;      // +0x010 hero flag (RefreshModel)
    SIUnitAnimation* Anim;           // +0x014 prototype +0x04 CreateAnimation(this)
    int              _18[4];         // +0x018
    int              ActiveDriver;   // +0x028 -1 = none
    int              PrevDriver;     // +0x02c
    int              RefreshDriver;  // +0x030 driver refreshed while +0x1f4 is set (-1)
    bool             DriverLocked;   // +0x034
    unsigned char    _35[3];
    SUnitArray<SIDriver*> Drivers;   // +0x038
    int              MainGunner;     // +0x044 prototype +0xc0, -1 without gunners
    SUnitArray<SGunner*> Gunners;    // +0x048
    float            UnitSize;       // +0x054 prototype +0x90
    int              UnitSizeBlocks; // +0x058 UnitSize * 4 (block map cells)
    int              UnitSizeBlocks2;// +0x05c
    int              _60;            // +0x060 -10000
    float            XP;             // +0x064 (a float in HD: GetRank 0x5b9e60 compares it with movss; Init copies the UNTD dword raw)
    bool             FirstKill;      // +0x068
    bool             FirstBlood;     // +0x069
    bool             FirstShot;      // +0x06a
    bool             FirstVehicleLost;           // +0x06b
    bool             FirstArmouredVehicleKill;   // +0x06c
    bool             _6d;            // +0x06d
    unsigned char    _6e[2];
    int              EffectTimer;    // +0x070 counts down per ServerRefresh, 0 -> 0x5c2200
    int              WorldIndex;     // +0x074
    int              Parent;         // +0x078 carrier / squad (-1)
    bool             _7c;            // +0x07c
    unsigned char    _7d[3];
    int              AIGroup;        // +0x080 -1
    int              LastRefreshFrame; // +0x084
    float            Yrel;           // +0x088
    float            Pos[3];         // +0x08c x, y, z
    float            PrevPos[3];     // +0x098
    float            PrevPos2[3];    // +0x0a4
    float            Dir;            // +0x0b0 (-pi..pi)
    float            PrevDir;        // +0x0b4
    float            PrevDir2;       // +0x0b8
    int              _bc[3];         // +0x0bc
    float            Speed;          // +0x0c8 (Stop: speed / driver step) (name guessed)
    unsigned char    _cc;            // +0x0cc
    unsigned char    _cd[3];
    int              _d0;            // +0x0d0
    int              _d4;            // +0x0d4
    unsigned short   MoveFlags;      // +0x0d8 block map test flags (0x5c1170)
    unsigned char    _da[2];
    int              RandomSide;     // +0x0dc rand() * 2 >> 15
    int              GlobalState;    // +0x0e0 animation global state ("normal")
    int              PrevGlobalState;// +0x0e4
    int              DesiredState;   // +0x0e8
    int              _ec;            // +0x0ec
    bool             _f0;            // +0x0f0
    bool             StateChanging;  // +0x0f1
    unsigned char    _f2[2];
    int              StateChangeTime;// +0x0f4
    int              _f8;            // +0x0f8 1
    int              Player;         // +0x0fc
    int              Team;           // +0x100 World players[Player] +0x00
    unsigned         _104;           // +0x104
    unsigned         _108;           // +0x108 hashed by the world CRC
    int              _10c;           // +0x10c
    bool             _110;           // +0x110
    bool             Invulnerable;   // +0x111 prototype +0x8b
    bool             _112;           // +0x112 prototype +0x8c
    unsigned char    _113;
    float            HP;             // +0x114 1.0 (hashed by the world CRC)
    float            _118;           // +0x118
    float            Armor[4];       // +0x11c front, left, right, back
    float            _12c;           // +0x12c 1.0
    int              _130;           // +0x130
    int              _134;           // +0x134 -1
    int              _138;           // +0x138
    int              _13c;           // +0x13c
    bool             _140;           // +0x140
    unsigned char    _141[3];
    int              _144;           // +0x144
    int              _148;           // +0x148
    bool             _14c;           // +0x14c
    unsigned char    _14d[3];
    bool             Wrecked;        // +0x150 InitModel uses the wreck model (name guessed)
    unsigned char    _151[2];
    bool             Frozen;         // +0x153 ServerRefresh returns at once (name guessed)
    unsigned short   _154;           // +0x154
    unsigned char    _156;           // +0x156
    unsigned char    _157;
    int              _158;           // +0x158
    int              _15c;           // +0x15c
    void*            _160;           // +0x160 owned (deleted by the dtor)
    int              _164;           // +0x164
    unsigned char    Unplaced;       // +0x168 (short with +0x169 in Place)
    unsigned char    InVehicleAnim;  // +0x169
    unsigned char    _16a[2];
    SUnitArray<SUnitStored> Stored;  // +0x16c
    SUnitArray<SUnitMember> Members; // +0x178 squad members / seats
    bool             IsStored;       // +0x184
    unsigned char    _185[3];
    int              StoreMode;      // +0x188 UNTD StoredSpecial / StoreUnit mode
    int              _18c;           // +0x18c -1
    bool             _190;           // +0x190
    unsigned char    _191[3];
    SString          ScriptID;       // +0x194
    SUnitArray<SOrder> Orders;       // +0x19c queued commands (0x5bb980; ServerRefresh runs one when idle)
    SUnitArray<SNearUnit> NearUnits; // +0x1a8 units close enough to collide (0x5b78a0; drivers 0x554870 / 0x555450)
    SUnitArray<SNearUnit> SightUnits;// +0x1b4 units in weapon / sight range (0x5b78a0)
    SUnitArray<SInvalidTarget> InvalidTargets; // +0x1c0 (0x5b76e0, timed out by 0x5bdcd0)
    int              StuckFrame;     // +0x1cc -1, frame of the last OnDriverStucked (0x5b76e0)
    int              StuckCount;     // +0x1d0 0..8 (0x5bb400 waits StuckCount * 5 ticks)
    bool             _1d4;           // +0x1d4
    unsigned char    _1d5[3];
    SUnitGhostQueue  GhostFrames;    // +0x1d8
    int              _1f0;           // +0x1f0 -1
    STarget*         CurrentTarget;  // +0x1f4 refcounted
    STarget*         PrimaryTarget;  // +0x1f8 refcounted
    SUnitArray<int>  ChildUnits;     // +0x1fc
    int              BuiltInDriverUnit; // +0x208 -1
    bool             _20c;           // +0x20c 1
    unsigned char    _20d[3];
    float            MemberOffset[5][2]; // +0x210 squads: member x, z offsets (Init 0x59c470)
    int              MemberDir[5];   // +0x238 squads: member directions
    float            FormationDir;   // +0x24c squads
    int              Behavior;       // +0x250 UNTD Behavior (1)
    int              _254;           // +0x254 -1
    int              _258;           // +0x258
    void*            _25c;           // +0x25c owned
    int              _260;           // +0x260
    int              _264;           // +0x264
    int              _268;           // +0x268
    int              _26c[12];       // +0x26c 0x80000001
    int              LastSeenFrame[12]; // +0x29c per player (0x80000001)
    bool             _2cc[12];       // +0x2cc
    int              Towed;          // +0x2d8 -1
    int              _2dc;           // +0x2dc
    int              _2e0;           // +0x2e0 0x1e (0x6e trains, 0 squads)
    int              _2e4;           // +0x2e4 -1
    bool             _2e8;           // +0x2e8
    bool             _2e9;           // +0x2e9 1
    bool             OnBlockMap;     // +0x2ea
    bool             _2eb;           // +0x2eb 1
    float            Cargo;          // +0x2ec 1.0
    int              _2f0;           // +0x2f0
    bool             HasReturnPos;   // +0x2f4 AI units go back to +0x2f8/+0x2fc (AI_Heartbeat 0x5b37d0)
    unsigned char    _2f5[3];
    float            ReturnX;        // +0x2f8
    float            ReturnZ;        // +0x2fc
    float            _300;           // +0x300
    bool             _304;           // +0x304
    unsigned char    _305[3];
    SUnitArray<SPoint2> WayPoints;   // +0x308 copy of the driver's global path (0x5bee10)
    SUnitArray<int>  StaticEffects;  // +0x314 pixie effect handles (0x5c2a70)
    SUnitArray<int>  _320;           // +0x320 pixie effect handles (StopEffects 0x5c3030)
    bool             _32c;           // +0x32c
    unsigned char    _32d[3];
    int              _330;           // +0x330 0x80000000
    bool             RemoveMe;       // +0x334 ServerRefresh removes the unit
    unsigned char    _335[3];
    int              _338;           // +0x338
    bool             _33c;           // +0x33c
    unsigned char    _33d[3];
};

#if defined(_M_IX86)
static_assert(sizeof(SUnit) == 0x340, "SUnit base 0x340 (subclasses store the prototype at +0x340)");
static_assert(offsetof(SUnit, Drivers) == 0x38, "+0x38 drivers");
static_assert(offsetof(SUnit, Gunners) == 0x48, "+0x48 gunners");
static_assert(offsetof(SUnit, WorldIndex) == 0x74, "+0x74");
static_assert(offsetof(SUnit, Pos) == 0x8c, "+0x8c pos");
static_assert(offsetof(SUnit, Dir) == 0xb0, "+0xb0 dir");
static_assert(offsetof(SUnit, GlobalState) == 0xe0, "+0xe0");
static_assert(offsetof(SUnit, Player) == 0xfc, "+0xfc player");
static_assert(offsetof(SUnit, HP) == 0x114, "+0x114");
static_assert(offsetof(SUnit, Unplaced) == 0x168, "+0x168");
static_assert(offsetof(SUnit, Members) == 0x178, "+0x178");
static_assert(offsetof(SUnit, ScriptID) == 0x194, "+0x194");
static_assert(offsetof(SUnit, GhostFrames) == 0x1d8, "+0x1d8");
static_assert(offsetof(SUnit, CurrentTarget) == 0x1f4, "+0x1f4");
static_assert(offsetof(SUnit, BuiltInDriverUnit) == 0x208, "+0x208");
static_assert(offsetof(SUnit, FormationDir) == 0x24c, "+0x24c");
static_assert(offsetof(SUnit, Behavior) == 0x250, "+0x250");
static_assert(offsetof(SUnit, LastSeenFrame) == 0x29c, "+0x29c");
static_assert(offsetof(SUnit, Towed) == 0x2d8, "+0x2d8");
static_assert(offsetof(SUnit, OnBlockMap) == 0x2ea, "+0x2ea");
static_assert(offsetof(SUnit, StaticEffects) == 0x314, "+0x314");
static_assert(offsetof(SUnit, RemoveMe) == 0x334, "+0x334");
#endif

// The unit at a world heap index (HD inline SHeapTRB::operator[] 0x546490,
// which panics on a dead index).
SUnit* WorldUnit(int index);

// HD MSVC rand() LCG on World+0x7518 (inline in every user): returns the
// 15-bit value (seed >> 16) & 0x7fff.
int WorldRand();

} // namespace pz

#endif // PZ_GAME_UNIT_H
