// src/world/selection.cpp
// SWorld selection / picking / camera-state rows (world.h block "M3",
// selection.h). OWNER: agent V (docs/M3_INTERFACES.md). Skeleton stubs.

#include "selection.h"
#include "m3common.h"
#include "stub_log.h"

namespace pz {

int SWorld::PickUnitAt(int p1, int p2)
{
    STUB_LOG("SWorld::PickUnitAt (0x5fc050)");
    PZ_M3_TRACE("SWorld::PickUnitAt (0x5fc050)");
    (void)p1; (void)p2;
    return -1;
}

void SWorld::SelectUnitsInBox(int p1, int p2)
{
    STUB_LOG("SWorld::SelectUnitsInBox (0x5fc5b0)");
    PZ_M3_TRACE("SWorld::SelectUnitsInBox (0x5fc5b0)");
    (void)p1; (void)p2;
}

void SWorld::Select_5fc860(int p1)
{
    STUB_LOG("SWorld::Select_5fc860 (0x5fc860)");
    PZ_M3_TRACE("SWorld::Select_5fc860 (0x5fc860)");
    (void)p1;
}

void SWorld::Select_5fcb10(int p1, int p2)
{
    STUB_LOG("SWorld::Select_5fcb10 (0x5fcb10)");
    PZ_M3_TRACE("SWorld::Select_5fcb10 (0x5fcb10)");
    (void)p1; (void)p2;
}

void SWorld::SelectSameType(int p1, int p2, int p3)
{
    STUB_LOG("SWorld::SelectSameType (0x5fcd10)");
    PZ_M3_TRACE("SWorld::SelectSameType (0x5fcd10)");
    (void)p1; (void)p2; (void)p3;
}

void SWorld::Select_5fd630(int p1, int p2, int p3, int p4)
{
    STUB_LOG("SWorld::Select_5fd630 (0x5fd630)");
    PZ_M3_TRACE("SWorld::Select_5fd630 (0x5fd630)");
    (void)p1; (void)p2; (void)p3; (void)p4;
}

void SWorld::Select_5ddb60()
{
    STUB_LOG("SWorld::Select_5ddb60 (0x5ddb60)");
    PZ_M3_TRACE("SWorld::Select_5ddb60 (0x5ddb60)");
}

// SWorld::ShowUnitRange 0x5fee00: worldcamera.cpp (agent V).

// PANZERS 0x5e6a70
void SWorld::GetCameraState(unsigned* out5)
{
    const unsigned* w = (const unsigned*)this;
    out5[0] = w[0x38 / 4];   // CamTarget x
    out5[1] = w[0x40 / 4];   // CamTarget z
    out5[2] = w[0x44 / 4];   // CamYaw
    out5[3] = w[0x50 / 4];
    out5[4] = w[0x54 / 4];   // CamDist
}

} // namespace pz
