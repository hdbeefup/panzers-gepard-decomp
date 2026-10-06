// src/game/squadunit.h
// SPanzersSquadUnit (vftable 0x7f9e3c, 116 slots, 0x3a4 bytes) and
// SPanzersSquadMemberUnit (vftable 0x7f9a40, 0x354 bytes): the infantry
// squads and their soldiers (0x5977e0..0x5a1300). OWNER: agent U.
//
// A squad is a unit of its own (model units\walker\squad.4D, SSquadAnimation)
// whose members are units created by its Init (SWorld::CreateUnit 0x5e3170,
// one slot each right after the squad's slot) and listed in SUnit +0x178.
// The members stand at relative positions around the squad: polar
// (radius, angle) pairs at +0x34c, drawn from the world random seed
// (SetRelativeNormalPositions 0x5a0740) and turned by the formation
// direction +0x24c.

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
    void Unplace() override;                                     // 0x59c1e0
    void Place(float x, float z, float dir) override;            // 0x5a1220
    void SetCurrentTarget(STarget* target, int p2) override;     // 0x59f580
    void EC_Move(int xBits, int zBits, int p3, bool p4, int p5) override;   // 0x59af20
    void SetOnBlockMap(bool on) override;                        // 0x59d460
    void SetUnitSize() override;                                 // 0x5a0f30
    void GetCenterPosition(float* out) override;                 // 0x59b4a0
    // The one slot SPanzersSquadUnit adds (SIPanzersSquadUnit +0x1cc): declared
    // here so it lands after the 115 SUnit slots in the same vtable.
    virtual void RefreshSquadFormation();                        // +0x1cc 0x59dda0

    void SetMembersRadius();                                     // 0x59fda0
    void SetRelativeNormalPositions();                           // 0x5a0740
    void SetRelativePositions();                                 // the formation switch (0x59c470, 0x59f580)
    void GetSquadMemberRelativePosition(float* out, int i);      // 0x59bff0
    void PlaceMemberOffsets(int dirBits);                        // the loop of 0x5a1220 / 0x59fe30

    SPPanzersSquadUnit* P;           // +0x340
    int      _344;                   // +0x344
    int      FormationType;          // +0x348 0 normal (random ring), 1/2 not used by the menu
    SUnitArray<SSquadRelPos> RelPos; // +0x34c
    int      _358;                   // +0x358
    bool     _35c;                   // +0x35c 1
    unsigned char _35d[3];
    int      Board[8];               // +0x360 board elements (not created by the recompile)
    int      _380;                   // +0x380 board element
    SUnitArray<int> _384;            // +0x384
    bool     _390;                   // +0x390 1
    unsigned char _391[3];
    float    MemberRadiusMax;        // +0x394
    float    MemberRadiusMin;        // +0x398
    int      _39c;                   // +0x39c -1 (squad global state request)
    bool     _3a0;                   // +0x3a0
    unsigned char _3a1[3];
};

struct SPanzersSquadMemberUnit : SUnit {
    SPanzersSquadMemberUnit(SPPanzersSquadMemberUnit* proto, int worldIndex);   // 0x5977e0
    ~SPanzersSquadMemberUnit() override;                         // 0x597940
    void InitNew(int player, const float* pos, float dir, int p4, float hp) override;   // 0x598940
    void SetOnBlockMap(bool on) override;                        // 0x5989b0 (members stay off the block map)

    SPPanzersSquadMemberUnit* P;     // +0x340
    int      Board344;               // +0x344 board element (health bar)
    int      Board348;               // +0x348 board element
    int      _34c;                   // +0x34c
    SIModel* Parachute;              // +0x350 prototype +0x13c model, flags 7
};

PZ_HD_SIZE(SPanzersSquadUnit, kHdSizeSPanzersSquadUnit);
PZ_HD_SIZE(SPanzersSquadMemberUnit, kHdSizeSPanzersSquadMemberUnit);

} // namespace pz

#endif // PZ_GAME_SQUADUNIT_H
