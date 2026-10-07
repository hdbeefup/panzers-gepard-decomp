// src/game/trainanim.cpp
// STrainAnimation and SPTrainAnimation::CreateAnimation (0x5c7490, 0x5c7990,
// 0x5c9350, 0x5cce80). OWNER: agent TR. See trainanim.h.

#include <math.h>
#include "trainanim.h"
#include "iunit.h"
#include "pz/imodel.h"
#include "logger.h"
#include "m2common.h"

namespace pz {

PZ_HD_SIZE(STrainAnimation, 0xc8);

// PANZERS 0x5c7990
SIUnitAnimation* SPTrainAnimation::CreateAnimation(SIUnit* unit)
{
    return new STrainAnimation(this, unit);                       // new 200
}

STrainAnimation::STrainAnimation(SPTrainAnimation* proto, SIUnit* unit)
    : SVehicleAnimation(proto, unit)                              // 0x5c68e0
{
    Type = 8;
    TProto = proto;
    Unit = unit;
    _c4 = 0;
}

// PANZERS 0x5c7490
STrainAnimation::~STrainAnimation()
{
}

// PANZERS 0x5c9350
void STrainAnimation::InitModel(SIModel* model)
{
    SVehicleAnimation::InitModel(model);                          // 0x5c93a0
    Model()->PlaySequence("vonat", false);                        // +0x6c
    Model()->SetFlags(0x17);                                      // +0x94 (tail jump)
}

// PANZERS 0x5cce80
// The vehicle update, then the model follows the terrain slope (turned by
// -90 degrees) and the "vonat" sequence advances with the speed (backwards
// in reverse).
void STrainAnimation::UpdateModel()
{
    SVehicleAnimation::UpdateModel();                             // 0x5cd020
    SIUnit* u = Unit;
    SUnitAnimEnv& e = g_UnitAnimEnv;
    const float* pos = &UnitField<float>(u, kUnitPos);
    float a = e.TerrainHeight((float)((double)pos[0] - 0.5), pos[2]);   // 0x5e7730, 0x7ea760
    float b = e.TerrainHeight((float)((double)pos[0] + 0.5), pos[2]);
    pos = &UnitField<float>(u, kUnitPos);
    float c = e.TerrainHeight(pos[0], (float)((double)pos[2] - 0.5));
    float d = e.TerrainHeight(pos[0], (float)((double)pos[2] + 0.5));
    Model()->SetRotation((float)((double)UnitField<float>(u, kUnitDir) - 1.5707963705062866),   // +0x1c, 0x7f5a38
                         a - b, c - d);
    float speed = UnitField<float>(u, kUnitMoveSpeed);
    if (0.0f < speed) {
        float sign = (float)((UnitField<bool>(u, kUnitReverse) ? 0 : 1) * 2 - 1);
        Model()->AdvanceAnimation((sign * speed * 2.0f) / 2.5132741928100586f);   // +0x70, 0x7f4558, 0x7fe0d8
    }
}

// The animation state index of a name (SPUnitAnimation::FindState 0x5c7ed0)
// for the trainunit.cpp tow, which cannot include unitanim.h.
int UnitAnimFindState(SIUnit* unit, const char* name)
{
    SIUnitAnimation* a = UnitField<SIUnitAnimation*>(unit, 0x14);
    return a ? static_cast<SUnitAnimation*>(a)->Proto->FindState(name) : 0;
}

} // namespace pz
