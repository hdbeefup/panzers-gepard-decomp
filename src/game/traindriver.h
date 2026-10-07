// src/game/traindriver.h
// STrainDriver / SPTrainDriver (DriverType 13): the locomotive moves along
// its road (STrainUnit +0x3ac) by path length (SUnit +0x300); a target off
// that road is refused. OWNER: agent TR.

#ifndef PZ_GAME_TRAINDRIVER_H
#define PZ_GAME_TRAINDRIVER_H

#include "driver.h"

namespace pz {

struct SPTrainDriver : SPDriver {                        // vftable 0x7fa994, 0x44
    void Load(SProperties* props, const char* name, int nameLen) override;   // 0x556560
    SIDriver* CreateDriver(SIUnit* unit) override;      // 0x551340
};

struct STrainDriver : SDriver {                          // vftable 0x7f4e00, 0xe8
    STrainDriver(SPTrainDriver* pd, SIUnit* unit);      // 0x551340 (inline in the factory)
    ~STrainDriver() override;                           // 0x5503c0
    void SetTarget(STarget* target) override;           // +0x08 0x557ec0
    bool MoveTowardNextWayPoint(SGhostFrame* frame) override;   // +0x18 0x559230
    bool FindGlobalPath(int size) override;             // +0x30 0x551fa0
    bool FindLocalPath(SGhostFrame* frame) override;    // +0x34 0x552b50
};

} // namespace pz

#endif // PZ_GAME_TRAINDRIVER_H
