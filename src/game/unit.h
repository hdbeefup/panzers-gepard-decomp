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
    void Slot_1C() override;
    void Hook20(int p1) override;                                // 0x54cd40
    void SetPosition(float x, float z, int dirBits, int yrelBits) override;  // 0x5c1980
    void Slot_28() override;
    void ServerRefresh(int frame) override;                      // 0x5bee90
    void Slot_30() override;
    void RefreshTargeting() override;                            // 0x5bd600
    void RefreshMisc() override;                                 // 0x5bdee0
    void RefreshModel() override;                                // 0x5c6130
    void UpdateVisuals(SIViewport* vp) override;                 // 0x5b76c0
    void Slot_44() override;
    void Slot_48() override;
    void Unplace() override;                                     // 0x5ba850
    void Place(float x, float z, float dir) override;            // 0x5c5160
    void Slot_54() override;
    void Slot_58() override;
    bool StoreUnit(int unit, int mode) override;                 // 0x5c30d0
    void Slot_60() override;
    void Slot_64() override;
    void Slot_68() override;
    void Slot_6C() override;
    void Remove(bool p1) override;                               // 0x5c2df0
    void Slot_74() override;
    void Slot_78() override;
    void Slot_7C() override;
    bool HasWoundedMember() override;                            // 0x54a240
    void Slot_84() override;
    int GetRank() override;                                      // 0x5b9e60
    void Slot_8C() override;
    void Slot_90() override;
    void Slot_94() override;
    void Slot_98() override;
    void Slot_9C() override;
    void SetCurrentTarget(STarget* target, int p2) override;     // 0x5c0d10
    void Slot_A4() override;
    void Slot_A8() override;
    void EC_Move(int xBits, int zBits, int p3, bool p4, int p5) override;   // 0x5b8ea0
    void Slot_B0() override;
    void EC_MoveAlongPath(int path, int p2, int p3) override;    // 0x5b8e20
    void EC_Follow(int unit, int p2) override;                   // 0x5b8ba0
    void Slot_BC() override;
    void Stop() override;                                        // 0x5b91b0
    void ClearTargets() override;                                // 0x5b90b0
    void Slot_C8() override;
    void Slot_CC() override;
    void Slot_D0() override;
    void Slot_D4() override;
    void Slot_D8() override;
    void Slot_DC() override;
    void Slot_E0() override;
    void Slot_E4() override;
    void Slot_E8() override;
    void StopGunners() override;                                 // 0x5b9110
    void Slot_F0() override;
    void Slot_F4() override;
    void Slot_F8() override;
    void Slot_FC() override;
    void Slot_100() override;
    void Slot_104() override;
    void Slot_108() override;
    void Slot_10C() override;
    void Slot_110() override;
    void Slot_114() override;
    void Slot_118() override;
    void Slot_11C() override;
    void Slot_120() override;
    void Slot_124() override;
    void Slot_128() override;
    void SetBehavior(int behavior) override;                     // 0x5b8960
    void Slot_130() override;
    void Slot_134() override;
    void Slot_138() override;
    void Slot_13C() override;
    void Slot_140() override;
    void Slot_144() override;
    void Slot_148() override;
    void Slot_14C() override;
    void Slot_150() override;
    void Slot_154() override;
    void Slot_158() override;
    void Slot_15C() override;
    void Slot_160() override;
    void Slot_164() override;
    void Slot_168() override;
    void StoreInterpolationState() override;                     // 0x5b5ad0
    void Slot_170() override;
    void Slot_174() override;
    void Slot_178() override;
    float GetMaxRange(int weapon) override;                      // 0x5b9f60
    float GetMinRange(int weapon) override;                      // 0x5b9f90
    float GetSightRange() override;                              // 0x5ba240
    float GetExtra188() override;                                // 0x548230
    void Slot_18C() override;
    void AI_Heartbeat() override;                                // 0x5b37d0
    void Slot_194() override;
    void SetOnBlockMap(bool on) override;                        // 0x5bc660
    void Slot_19C() override;                                    // 0x5b74c0
    void MarkBlockMap(bool on, int p2, int p3, int p4, short p5) override;  // 0x5bc6d0
    int TestBlockMapPath(int p1, int p2, int p3, int p4, short p5, int p6, int p7) override; // 0x5b7520
    int TestBlockMap(int p1, int p2, int p3, int p4, short p5) override;    // 0x5b7500
    void RestoreBehavior() override;                             // 0x546ac0
    float GetMoveSpeed(int p1) override;                         // 0x5b9ed0
    void Slot_1B4() override;
    void Slot_1B8() override;
    void Slot_1BC() override;
    void ServerRefreshMedic(float dt) override;                  // 0x5bfa50
    void SetUnitSize() override;                                 // 0x5c21d0
    void GetCenterPosition(float* out) override;                 // 0x5b9d40

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
    int              XP;             // +0x064
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
    SUnitArray<int>  _19c;           // +0x19c
    SUnitArray<int>  _1a8;           // +0x1a8
    SUnitArray<int>  _1b4;           // +0x1b4
    SUnitArray<int>  _1c0;           // +0x1c0
    int              _1cc;           // +0x1cc -1
    int              _1d0;           // +0x1d0
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
    bool             _2f4;           // +0x2f4
    unsigned char    _2f5[3];
    int              _2f8;           // +0x2f8
    int              _2fc;           // +0x2fc
    float            _300;           // +0x300
    bool             _304;           // +0x304
    unsigned char    _305[3];
    SUnitArray<int>  _308;           // +0x308
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
