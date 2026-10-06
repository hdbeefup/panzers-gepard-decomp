// src/game/driver.h
// pz::SDriver and pz::SPDriver (HD 0x5500f0..0x55c900) and the driver
// classes the menu uses. OWNER: agent P.
//
// A driver moves its unit (+0x08) towards its STarget (+0xc0):
//   FindGlobalPath (A* over the block map) -> GlobalWayPointsArray (+0x94),
//   FindLocalPath -> LocalWayPointsArray (+0x88, SWayPointWithManoeuvres),
//   then a "ghost" runs up to +0xdc frames ahead of the unit
//   (Ghost_FirstStep/Ghost_NextStep, frames queued in the unit +0x1d8) and
//   Refresh moves the unit one ghost frame per tick (0x553960).
//
// Units create drivers through SIPDriver +0x10 CreateDriver(unit); units'
// prototypes create the SPDriver through CreatePDriver(DriverType) (HD
// SPUnit::InitDrivers 0x5a6f10, agent U) and then call +0x04 Load.

#ifndef PZ_GAME_DRIVER_H
#define PZ_GAME_DRIVER_H

#include "idriver.h"
#include "drivertypes.h"

namespace pz {

struct SIUnit;
struct STarget;
struct SPDriver;

// HD SDriver base (vftable 0x7f49e0), 0xe8 bytes with the menu subclasses.
struct SDriver : SIDriver {
    SDriver(SPDriver* pdriver, SIUnit* unit);           // PANZERS 0x54f4b0
    ~SDriver() override;                                // PANZERS 0x54fdc0
    SIPDriver* GetPDriver() override;
    void SetTarget(STarget* target) override;
    void SetTargetStopped(STarget* target) override;
    void Init() override;
    void Refresh() override;
    bool MoveTowardNextWayPoint(SGhostFrame* frame) override;
    float TurnToDir(float dir) override;
    float TurnToPoint(float x, float y, float z) override;
    bool GhostStepStraight(SGhostFrame* frame, float x, float z) override;
    bool GhostStepTowards(SGhostFrame* frame, float x, float z) override;
    bool GhostTurnTowards(SGhostFrame* frame, float x, float z) override;
    bool FindGlobalPath(int size) override;
    bool FindLocalPath(SGhostFrame* frame) override;
    bool RefreshGhost() override;
    bool PredictGhost(SGhostFrame* frame, int* outUnit, float* outPos, float* outDir) override;
    void Ghost_FirstStep() override;
    void Ghost_NextStep() override;
    float GetMaxSpeed() override;
    float GetTurnSpeed() override;
    void GetClassDescriptor(void** obj, const SUnitClassDesc** desc) override;   // 0x5531e0

    // Non-virtual SDriver functions (HD addresses in driver*.cpp).
    bool  CheckTargetMoved();                           // 0x5507d0
    bool  CopyGlobalPathFromBossDriver();               // 0x550d20
    void  FillGhostFrame(SGhostFrame* f);               // 0x550ec0
    void  SetGlobalPathFromAStar(const void* astar);    // 0x5514b0
    void  SetLocalPathFromAStar(const void* astar, const float* start); // 0x551780
    void  StartStoppedGhost(bool keepGlobal);           // 0x5518c0
    void  SetLocalAfterCollision(SGhostFrame* f);       // 0x550650
    bool  NeedsGlobalPath();                            // 0x551c00
    bool  GetLocalPathGoalPoint(float* out);             // 0x553100
    SWayPoint* GetWayPoint(SWayPoint* out, const SGhostFrame* f); // 0x5534d0
    void  GhostStep();                                  // 0x553620
    void  FollowGhost();                                // 0x553960
    void  ResetGhost();                                 // 0x554770
    bool  HasArrived(SGhostFrame* f, const SGhostFrame* prev); // 0x554fc0
    void  InitLocalWayPointsArray(const SGhostFrame* f, bool lastStops); // 0x556ac0
    bool  IsSpinStopNecessary(const SGhostFrame* f);    // 0x557260
    bool  IsTheGhostInStoppingDistance(const SGhostFrame* f); // 0x557400
    float ClampSpeedToGroup(float speed, bool useBoss); // 0x557a80
    bool  CanCancelManoeuvre(const SGhostFrame* f);     // 0x5594a0
    void  ComputeGoal(float* outDelta);                 // 0x55aa80
    bool  NextWayPoint(SGhostFrame* f, bool setNext, bool create); // 0x55af20
    bool  SetNextWP(SGhostFrame* f, bool advance, bool create);   // 0x55b0b0
    void  UpdateTurnRadius();                           // 0x55b1d0
    void  SetUnitSpeed(SGhostFrame* f);                 // 0x55b2d0
    void  SetUnitSpinSpeed(SGhostFrame* f);             // 0x55b3e0
    float TurnTowardsInSteps(SGhostFrame* f, float dir);   // 0x55b5f0
    float TurnTowardsDir(SGhostFrame* f, float dir);       // 0x55b8d0
    void  TurnTowardsPoint(SGhostFrame* f, float x, float z); // 0x55b900
    float TurnTowardsDirect(SGhostFrame* f, float dir);    // 0x55b9c0
    void  StartEffects();                               // 0x55bb80
    void  StartMoveEffects();                           // 0x55bc50
    void  StartWaterEffects();                          // 0x55be10
    void  Stop(bool clearTarget);                       // 0x55bf50
    void  StopAllEffects();                             // 0x55c0e0
    void  StopWaterEffects();                           // 0x55c160
    int   RandomInt(int range);                         // 0x555a00 (World seed)

    // Fields (HD offsets).
    SPDriver* PDriver;            // +0x004
    SIUnit*   Unit;               // +0x008
    SHdDArray<int> MoveEffects;   // +0x00c pixie effect per PDriver +0x20 entry (-1)
    SHdDArray<int> WaterEffects;  // +0x018 pixie effect per PDriver +0x2c entry (-1)
    float     WantedSpeed;        // +0x024 -1 = full speed, 0 = stop
    float     WantedSpin;         // +0x028 +-99999 = turn, 0 = none
    bool      StoppingDistance;   // +0x02c
    unsigned char _02d[3];
    int       CollisionUnit;      // +0x030 unit heap index the ghost ran into (-1)
    int       LastCollisionUnit;  // +0x034 (-1)
    float     CollisionPos[2];    // +0x038
    float     LastGhostPos[3];    // +0x040
    int       LocalPathFailures;  // +0x04c
    int       CollisionCount;     // +0x050
    int       AvoidUnit;          // +0x054 (-1)
    bool      GhostActive;        // +0x058
    bool      LocalPathEnd;       // +0x059
    bool      WaitForBoss;        // +0x05a
    bool      SkipFollow;         // +0x05b
    bool      MaxSpeedZero;       // +0x05c
    bool      GhostRestart;       // +0x05d
    unsigned char _05e[2];
    int       ManoeuvreOpeningFrame; // +0x060
    int       ManoeuvreFrame;     // +0x064
    bool      GhostHold;          // +0x068
    unsigned char _069[3];
    int       WaitTicks;          // +0x06c
    bool      Waiting;            // +0x070
    unsigned char _071[3];
    float     ArrivalRadius;      // +0x074
    bool      ArrivalActive;      // +0x078
    unsigned char _079[3];
    float     ArrivalDist2;       // +0x07c
    float     Arrival80;          // +0x080
    float     ArrivalDir;         // +0x084
    SHdDArray<SWayPointWithManoeuvres> LocalWayPoints; // +0x088
    SHdDArray<SVec2> GlobalWayPoints; // +0x094
    int       HasGlobalPath;      // +0x0a0
    int       HasLocalPath;       // +0x0a4
    float     Goal[3];            // +0x0a8
    float     TargetPos[3];       // +0x0b4
    STarget*  Target;             // +0x0c0
    STarget*  SavedTarget;        // +0x0c4
    STarget*  TurnTarget;         // +0x0c8
    float     TurnRadius;         // +0x0cc
    float     TurnSpeedFactor;    // +0x0d0
    int       SpeedSteps;         // +0x0d4
    int       SpinSteps;          // +0x0d8
    int       MaxGhostFrames;     // +0x0dc
    int       LastFollowFrame;    // +0x0e0
    SPDriver* PDriver2;           // +0x0e4 (the factory's SPDriver; 0 for squad members)
};

struct STurnInPlaceDriver : SDriver {                    // vftable 0x7f4bf0, factory 0x551400
    STurnInPlaceDriver(SPDriver* pd, SIUnit* unit);
    ~STurnInPlaceDriver() override;                     // 0x550420
    bool MoveTowardNextWayPoint(SGhostFrame* frame) override;   // 0x559340
};

struct STurnInAngleDriver : SDriver {                    // vftable 0x7f4c48, factory 0x5513a0
    STurnInAngleDriver(SPDriver* pd, SIUnit* unit);
    ~STurnInAngleDriver() override;                     // 0x5503f0
    bool MoveTowardNextWayPoint(SGhostFrame* frame) override;   // 0x559330
};

struct SWalkerDriver : SDriver {                         // vftable 0x7f4b40, factory 0x551460
    SWalkerDriver(SPDriver* pd, SIUnit* unit);
    ~SWalkerDriver() override;                          // 0x550450
    bool MoveTowardNextWayPoint(SGhostFrame* frame) override;   // 0x559350
};

struct SPanzersSquadDriver : SDriver {                   // vftable 0x7f4cf8, factory 0x5511a0
    SPanzersSquadDriver(SPDriver* pd, SIUnit* unit);
    ~SPanzersSquadDriver() override;                    // 0x5502d0
    bool MoveTowardNextWayPoint(SGhostFrame* frame) override;   // 0x5588e0
    float TurnToDir(float dir) override;                // 0x55c4d0
    float TurnToPoint(float x, float y, float z) override;      // 0x55c6a0
    float GetMaxSpeed() override;                       // 0x5532e0
    float GetTurnSpeed() override;                      // 0x553420
};

struct SPanzersSquadMemberDriver : SDriver {             // vftable 0x7f4d50, factory 0x5511f0
    SPanzersSquadMemberDriver(SPDriver* pd, SIUnit* unit); // 0x54f970
    ~SPanzersSquadMemberDriver() override;              // 0x550300
    void SetTarget(STarget* target) override;           // 0x557da0
    void Refresh() override;                            // 0x55a640
    bool MoveTowardNextWayPoint(SGhostFrame* frame) override;   // 0x558a60
    float GetMaxSpeed() override;                       // 0x553360
    void GetClassDescriptor(void** obj, const SUnitClassDesc** desc) override;   // 0x553220

    SPDriver* PDriver3;           // +0x0e8
    float     Wobble;             // +0x0ec
    float     WobbleSpeed;        // +0x0f0
};

// HD SPDriver base (vftable 0x7f4998). Sizes: 0x4c (TurnInPlace,
// TurnInAngle), 0x44 (walker, squads).
struct SPDriver : SIPDriver {
    SPDriver();                                         // 0x54f760 / 0x54f880
    ~SPDriver() override;                               // 0x550180
    void Load(SProperties* props, const char* name, int nameLen) override; // 0x555cc0
    void LoadSubProperties(SProperties* props) override; // 0x557520
    void Slot_0C() override;
    SIDriver* CreateDriver(SIUnit* unit) override;      // pure in HD

    int   Type;                   // +0x04 DriverType (-1 until Load)
    float MaxSpeed;               // +0x08
    float SpinSpeed;              // +0x0c
    float WheelTurnAngle;         // +0x10
    float Wheelbase;              // +0x14
    char* Name;                   // +0x18
    int   NameLen;                // +0x1c
    struct SEffectDesc { int Proto; char* Node; int NodeLen; };
    SHdDArray<SEffectDesc> MoveEffects;      // +0x20 "Move_Effects" (0x557520: "Effect", "MeshName")
    SHdDArray<SEffectDesc> WaterEffects;     // +0x2c "Move_In_Water_Effects"
    SHdDArray<SEffectDesc> DepartureEffects; // +0x38 "Departure_Effects"
};

struct SPTurnInPlaceDriver : SPDriver {                  // 0x7f49b0, 0x4c bytes
    void Load(SProperties* props, const char* name, int nameLen) override; // 0x556800
    SIDriver* CreateDriver(SIUnit* unit) override;      // 0x551400
    int   MaxFuel;                // +0x44
    float Consume;                // +0x48
};

struct SPTurnInAngleDriver : SPDriver {                  // 0x7f49c8, 0x4c bytes
    void Load(SProperties* props, const char* name, int nameLen) override; // 0x556680
    SIDriver* CreateDriver(SIUnit* unit) override;      // 0x5513a0
    int   MaxFuel;                // +0x44
    float Consume;                // +0x48
};

struct SPWalkerDriver : SPDriver {                       // 0x7fa904, 0x44 bytes
    void Load(SProperties* props, const char* name, int nameLen) override; // 0x556970
    SIDriver* CreateDriver(SIUnit* unit) override;      // 0x551460
};

struct SPPanzersSquadDriver : SPDriver {                 // 0x7fa94c, 0x44 bytes
    void Load(SProperties* props, const char* name, int nameLen) override; // 0x555fc0
    SIDriver* CreateDriver(SIUnit* unit) override;      // 0x5511a0
};

struct SPPanzersSquadMemberDriver : SPDriver {           // 0x7fa964, 0x44 bytes
    void Load(SProperties* props, const char* name, int nameLen) override; // 0x5560a0
    SIDriver* CreateDriver(SIUnit* unit) override;      // 0x5511f0
};

// HD SPUnit::InitDrivers 0x5a6f10 "DriverType" switch (the new + ctor part).
// Returns null for an unknown type (HD logs "SPUnit::InitDrivers - Unknown
// drivertype"). Types the menu does not use return a plain SPDriver that
// logs on CreateDriver.
SIPDriver* CreatePDriver(int driverType);


} // namespace pz

#endif // PZ_GAME_DRIVER_H
