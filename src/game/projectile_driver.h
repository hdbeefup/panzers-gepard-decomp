// src/game/projectile_driver.h
// SPProjectileDriver / SProjectileDriver (OWNER: M3-C sub-agent C1). See
// projectile.h. (A separate header: driver.h and unitanim.h declare
// different SHdDArray templates.)

#ifndef PZ_GAME_PROJECTILE_DRIVER_H
#define PZ_GAME_PROJECTILE_DRIVER_H

#include "driver.h"

namespace pz {

struct SPProjectileDriver : SPDriver {                   // vftable 0x7fa91c, 0x4c bytes
    SPProjectileDriver() : RocketPropulsion(false), MaxFuel(0.0f) {}   // 0x54f760 + vftable (0x5a6f10)
    void Load(SProperties* props, const char* name, int nameLen) override;   // 0x556180
    SIDriver* CreateDriver(SIUnit* unit) override;      // 0x551230

    bool  RocketPropulsion;       // +0x44 straight flight while the fuel lasts
    unsigned char _45[3];
    float MaxFuel;                // +0x48 ticks
};

struct SProjectileDriver : SDriver {                     // vftable 0x7f4b98, 0xec bytes
    SProjectileDriver(SPProjectileDriver* pd, SIUnit* unit);   // 0x551230 (inline)
    ~SProjectileDriver() override;                      // 0x550330
    void GetClassDescriptor(void** obj, const SUnitClassDesc** desc) override;   // 0x553240 (M4 S, savedesc.cpp)
    void Refresh() override;                            // 0x55a740

    int   Fuel;                   // +0x0e8 ticks of rocket propulsion left
};

} // namespace pz

#endif // PZ_GAME_PROJECTILE_DRIVER_H
