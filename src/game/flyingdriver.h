// src/game/flyingdriver.h
// SFlyingDriver / SPFlyingDriver (planes) and SPanzersParachuteDriver /
// SPPanzersParachuteDriver (paratroopers). OWNER: agent M3-C sub-agent C3.
// Separate from flying.h: driver.h and unitanim.h cannot meet in one unit
// (both define SHdDArray).

#ifndef PZ_GAME_FLYINGDRIVER_H
#define PZ_GAME_FLYINGDRIVER_H

#include "flying.h"
#include "driver.h"

namespace pz {

// ---------------------------------------------------------------------------
// Drivers

struct SPFlyingDriver : SPDriver {                       // vftable 0x7fa8ec, 0x58
    void Load(SProperties* props, const char* name, int nameLen) override;   // 0x555d30
    SIDriver* CreateDriver(SIUnit* unit) override;      // 0x5510d0

    int   MaxFuel = 0;                // +0x44
    float Consume = 0.0f;             // +0x48
    float Altitude = 0.0f;            // +0x4c
    float MaxTilting = 0.0f;          // +0x50
    bool  GroundTracking = false;     // +0x54 follow the terrain (0x5586a0)
    unsigned char _55[3] = {};
};

struct SFlyingDriver : SDriver {                         // vftable 0x7f4ae8, 0xf4
    SFlyingDriver(SPFlyingDriver* pd, SIUnit* unit);    // 0x5510d0 (inline in the factory)
    ~SFlyingDriver() override;                          // 0x550150
    void GetClassDescriptor(void** obj, const SUnitClassDesc** desc) override;   // 0x553200 (M4 S, savedesc.cpp)
    bool MoveTowardNextWayPoint(SGhostFrame* frame) override;   // 0x5586a0
    bool FindGlobalPath(int size) override;             // 0x551ef0
    bool FindLocalPath(SGhostFrame* frame) override;    // 0x552410
    bool PredictGhost(SGhostFrame* frame, int* outUnit, float* outPos, float* outDir) override;   // 0x554fb0
    // +0x50 0x553200 is the class descriptor getter (Slot_50, untyped): not overridden.

    void SetGroundTracking(bool on) { GroundTracking = on; }    // 0x55af10

    bool  GroundTracking;             // +0x0e8 SPFlyingDriver +0x54 (the support calls' p6)
    unsigned char _0e9[3];
    int   _0ec;                       // +0x0ec
    float Climb;                      // +0x0f0 vertical speed (-0.005 .. 1.0)
};

struct SPPanzersParachuteDriver : SPDriver {             // vftable 0x7fa97c, 0x44
    void Load(SProperties* props, const char* name, int nameLen) override;   // 0x555ee0
    SIDriver* CreateDriver(SIUnit* unit) override;      // 0x551150
};

struct SPanzersParachuteDriver : SDriver {               // vftable 0x7f4da8, 0xe8
    SPanzersParachuteDriver(SPDriver* pd, SIUnit* unit);    // 0x551150 (inline in the factory)
    ~SPanzersParachuteDriver() override;                // 0x5502a0
    void SetTarget(STarget* target) override;           // 0x557d00
    void Refresh() override;                            // 0x55a360
};

} // namespace pz

#endif // PZ_GAME_FLYINGDRIVER_H
