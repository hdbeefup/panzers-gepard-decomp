// src/game/unitsave.cpp
// SUnit save / load (unit.h block "M3"): the per-unit part of the save game
// that every mission start writes (SPanzersCampaign::SaveGameStartMission
// 0x596e30 -> SaveGame 0x5966a0 -> SUnit::Save). OWNER: agent F
// (docs/M3_INTERFACES.md); agent C owns the rest of SUnit. Skeleton stubs.

#include "unit.h"
#include "m3common.h"
#include "stub_log.h"

namespace pz {

void SUnit::Save(struct SStream* s)
{
    STUB_LOG("SUnit::Save (0x5be320)");
    PZ_M3_TRACE("SUnit::Save (0x5be320)");
    (void)s;
}

void SUnit::Load(struct SStream* s)
{
    STUB_LOG("SUnit::Load (0x5bbd30)");
    PZ_M3_TRACE("SUnit::Load (0x5bbd30)");
    (void)s;
}

} // namespace pz
