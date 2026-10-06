// src/game/squadunit.h
// SPanzersSquadUnit (vftable 0x7f9e3c, 116 slots, 0x3a4 bytes) and
// SPanzersSquadMemberUnit (vftable 0x7f9a40, 0x354 bytes): the infantry
// squads and their soldiers (0x5977e0..0x5a1300). OWNER: agent U; the
// per-tick slots (squadrefresh.cpp): M2-I sub-agent SQ.
//
// A squad is a unit of its own (model units\walker\squad.4D, SSquadAnimation)
// whose members are units created by its Init (SWorld::CreateUnit 0x5e3170,
// one slot each right after the squad's slot) and listed in SUnit +0x178.
// The members stand at relative positions around the squad: polar
// (radius, angle) pairs at +0x34c, drawn from the world random seed
// (SetRelativeNormalPositions 0x5a0740) and turned by the formation
// direction +0x24c. Every tick the squad's RefreshMisc (0x59e0d0) sends each
// member an EC_Move to its formation point (the squad position, or the
// squad's ghost frame 5 ticks ahead, plus the turned offset +0x210), and
// while the squad walks its driver jitters one member's radius and angle
// every other tick (0x5a0450 / 0x5a0060, one seed draw each).

#ifndef PZ_GAME_SQUADUNIT_H
#define PZ_GAME_SQUADUNIT_H

#include "unit.h"

namespace pz {

// +0x34c element (0x10): a member's place in the squad.
struct SSquadRelPos {
    float Radius;        // +0x00
    float Angle;         // +0x04
    int   Dir;           // +0x08 own direction (float bits) when HasDir
    bool  HasDir;        // +0x0c
    unsigned char _0d[3];
};

struct SPanzersSquadUnit : SUnit {
    SPanzersSquadUnit(SPPanzersSquadUnit* proto, int worldIndex);   // 0x598ff0
    ~SPanzersSquadUnit() override;                               // 0x599230 / 0x599110
    void Uninit() override;                                      // 0x599c70
    void Init(SUnitDef* def) override;                           // 0x59c470
    void Hook20(int p1) override;                                // 0x59fab0
    void SetPosition(float x, float z, int dirBits, int yrelBits) override;   // 0x59fe30
    void RefreshTargeting() override;                            // 0x59db00
    void RefreshMisc() override;                                 // 0x59e0d0
    void UpdateVisuals(SIViewport* vp) override;                 // 0x599620
    void Unplace() override;                                     // 0x59c1e0
    void Place(float x, float z, float dir) override;            // 0x5a1220
    bool HasWoundedMember() override;                            // 0x59d670
    void SetCurrentTarget(STarget* target, int p2) override;     // 0x59f580
    void EC_Move(int xBits, int zBits, int p3, bool p4, int p5) override;   // 0x59af20
    void StopGunners() override;                                 // 0x59b120
    void SetBehavior(int behavior) override;                     // 0x59a910
    float GetMaxRange(int weapon) override;                      // 0x59bd10
    float GetMinRange(int weapon) override;                      // 0x59be20
    float GetExtra188() override;                                // 0x59bb80 (HearingRange by rank)
    void SetOnBlockMap(bool on) override;                        // 0x59d460
    void MarkBlockMap(bool on, int p2, int p3, int p4, short p5) override;   // 0x59d4b0
    int TestBlockMapPath(int p1, int p2, int p3, int p4, short p5, int p6, int p7) override; // 0x599580
    int TestBlockMap(int p1, int p2, int p3, int p4, short p5) override;    // 0x599550
    void RestoreBehavior() override;                             // 0x599430
    float GetMoveSpeed(int state) override;                      // 0x59bc10 GetMaxMoveSpeed
    void ServerRefreshMedic(float dt) override;                  // 0x59ede0
    void SetUnitSize() override;                                 // 0x5a0f30
    void GetCenterPosition(float* out) override;                 // 0x59b4a0
    // The one slot SPanzersSquadUnit adds (SIPanzersSquadUnit +0x1cc): declared
    // here so it lands after the 115 SUnit slots in the same vtable. The name
    // is a guess of the interface; the body (0x59dda0) is the medic scan:
    // a support squad goes to heal a wounded allied unit nearby, or itself.
    virtual void RefreshSquadFormation();                        // +0x1cc 0x59dda0

    void SetMembersRadius();                                     // 0x59fda0
    void SetRelativeNormalPositions();                           // 0x5a0740
    void SetRelativePositions();                                 // 0x5a09d0 (also inline in 0x59c470, 0x59f580)
    void GetSquadMemberRelativePosition(float* out, int i);      // 0x59bff0
    void SetSquadMemberRelativePosition(int i);                  // 0x5a0ac0
    SSquadRelPos* RelPosAt(int i);                               // 0x599190 (panics out of range)
    // The member offset of the formation (HD inline in 0x59e0d0, 0x59fe30,
    // 0x5a1220, 0x59c470): the relative position turned by +0x24c.
    void GetFormationOffset(int i, float* ox, float* oz);
    void SetSquadBehavior(int behavior);                         // 0x599450
    float GetSquadMoveSpeed();                                   // 0x59b990 (SPanzersSquadDriver::GetMaxSpeed)
    bool MembersTooClose();                                      // 0x59d130
    void ClearMemberMarkers();                                   // 0x59d2f0
    void StepMemberAngle(float dir);                             // 0x5a0060 (squad driver, every 2nd tick)
    void StepMemberRadius(float dir);                            // 0x5a0450 (squad driver, every 2nd tick)
    void UpdateMovingMembersRelPos();                            // 0x5a0d30
    void UpdateMoveGlobalState();                                // 0x5a0fc0
    // The squad's +0x9c override (0x5a0a10): +0x112 on the squad and its
    // members. Not virtual here: the slot is still Slot_9C() in iunit.h.
    void SetFlag112(bool on);                                    // 0x5a0a10

    SPPanzersSquadUnit* P;           // +0x340
    int      MedicDelay;             // +0x344 10 while a member moves (ServerRefreshMedic 0x59ede0)
    int      FormationType;          // +0x348 0 normal (random ring), 1/2 not used by the menu
    SUnitArray<SSquadRelPos> RelPos; // +0x34c
    int      _358;                   // +0x358
    bool     CanHeal;                // +0x35c 1 (medic scan 0x59dda0)
    unsigned char _35d[3];
    int      Board[8];               // +0x360 board elements (not created by the recompile)
    int      _380;                   // +0x380 board element
    SUnitArray<int> MemberOrderDelay;// +0x384 ticks until member i gets its next EC_Move (0x59e0d0)
    bool     RestartOrders;          // +0x390 1: MemberOrderDelay[i] = 2 * i on the next tick
    unsigned char _391[3];
    float    MemberRadiusMax;        // +0x394
    float    MemberRadiusMin;        // +0x398
    int      RequestedState;         // +0x39c -1 (squad global state request)
    bool     MoveStateDropped;       // +0x3a0 0x5a0fc0: state 3 dropped while the path continues
    unsigned char _3a1[3];
};

struct SPanzersSquadMemberUnit : SUnit {
    SPanzersSquadMemberUnit(SPPanzersSquadMemberUnit* proto, int worldIndex);   // 0x5977e0
    ~SPanzersSquadMemberUnit() override;                         // 0x597940
    void InitNew(int player, const float* pos, float dir, int p4, float hp) override;   // 0x598940
    void RefreshTargeting() override;                            // 0x5989d0
    void RefreshMisc() override;                                 // 0x598ab0
    void UpdateVisuals(SIViewport* vp) override;                 // 0x597a10
    void EC_Move(int xBits, int zBits, int p3, bool p4, int p5) override;   // 0x598570
    void StopGunners() override;                                 // 0x5986e0
    void SetBehavior(int behavior) override;                     // 0x5980b0
    void AI_Heartbeat() override;                                // 0x5979d0
    void SetOnBlockMap(bool on) override;                        // 0x5989b0 (members stay off the block map)
    void Slot_19C() override;                                    // 0x5979f0 (returns false)
    void MarkBlockMap(bool on, int p2, int p3, int p4, short p5) override;   // 0x5989c0
    int TestBlockMap(int p1, int p2, int p3, int p4, short p5) override;    // 0x597a00
    float GetMoveSpeed(int p1) override;                         // 0x5988d0 (forbidden)

    // The member overrides of SIUnit +0xe4 (0x597fa0) and +0xe8 (0x597e80),
    // called by the squad's SetCurrentTarget 0x59f580. Not virtual here:
    // those slots are still Slot_E4 / Slot_E8 in iunit.h.
    void EC_AttackPos(int xBits, int zBits, int p3);             // 0x597fa0
    void EC_Attack(int unit, int p2);                            // 0x597e80

    SPPanzersSquadMemberUnit* P;     // +0x340
    int      Board344;               // +0x344 board element (health bar)
    int      Board348;               // +0x348 board element
    int      ParachuteTicks;         // +0x34c > 0 while the parachute animates (RefreshMisc 0x598ab0)
    SIModel* Parachute;              // +0x350 prototype +0x13c model, flags 7
};

PZ_HD_SIZE(SPanzersSquadUnit, kHdSizeSPanzersSquadUnit);
PZ_HD_SIZE(SPanzersSquadMemberUnit, kHdSizeSPanzersSquadMemberUnit);

// Plain callable forms for the drivers' g_DriverEnv hooks (driverunit.h):
//   SquadUnitMoveSpeed     = SquadEnv_MoveSpeed            0x59b990
//   SquadMemberRelativePos = SquadEnv_MemberRelativePos    0x59bff0
//   SquadMembersStep       = SquadEnv_MembersStep          0x5a0450
//   SquadMembersStep2      = SquadEnv_MembersStep2         0x5a0060
float SquadEnv_MoveSpeed(SIUnit* squad);
void  SquadEnv_MemberRelativePos(SIUnit* squad, float* out, int member);
void  SquadEnv_MembersStep(SIUnit* squad, float dir);
void  SquadEnv_MembersStep2(SIUnit* squad, float dir);

} // namespace pz

#endif // PZ_GAME_SQUADUNIT_H
