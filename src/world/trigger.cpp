// src/world/trigger.cpp
// Trigger data loaders (TRIG chunk). OWNER: agent L. Logged skeleton from
// M2-P0; mapload.cpp still keeps the TRIG chunk raw until L wires
// LoadTriggers into SWorld::LoadMap.

#include "trigger.h"
#include "stub_log.h"

namespace pz {

void STriggerEvent::Load(SStream* s)
{
    STUB_LOG("STriggerEvent::Load (0x5b2400)");
    PZ_M2_TRACE("STriggerEvent::Load (0x5b2400)");
    (void)s;
}

void STriggerCondition::Load(SStream* s)
{
    STUB_LOG("STriggerCondition::Load (0x5b22b0)");
    PZ_M2_TRACE("STriggerCondition::Load (0x5b22b0)");
    (void)s;
}

unsigned STriggerCondition::GetPropertyMask(int type)
{
    STUB_LOG("STriggerCondition::GetPropertyMask (0x5b1f30)");
    PZ_M2_TRACE("STriggerCondition::GetPropertyMask (0x5b1f30)");
    (void)type;
    return 0;
}

void STriggerAction::Load(SStream* s)
{
    STUB_LOG("STriggerAction::Load (0x5b2010)");
    PZ_M2_TRACE("STriggerAction::Load (0x5b2010)");
    (void)s;
}

unsigned STriggerAction::GetPropertyMask(int type)
{
    STUB_LOG("STriggerAction::GetPropertyMask (0x5b1d50)");
    PZ_M2_TRACE("STriggerAction::GetPropertyMask (0x5b1d50)");
    (void)type;
    return 0;
}

void LoadTriggers(STriggerArray<STrigger>* triggers, SStream* s)
{
    STUB_LOG("SWorld::LoadTriggers (0x5f0140)");
    PZ_M2_TRACE("SWorld::LoadTriggers (0x5f0140)");
    (void)triggers; (void)s;
}

} // namespace pz
