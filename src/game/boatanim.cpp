// src/game/boatanim.cpp
// SBoatAnimation and SPBoatAnimation::CreateAnimation (0x5c76d0, 0x5c70d0,
// 0x5c8bf0, 0x5cb4e0). OWNER: agent MS. See boatanim.h.

#include "boatanim.h"
#include "iunit.h"
#include "pz/imodel.h"
#include "m2common.h"

namespace pz {

PZ_HD_SIZE(SBoatAnimation, 0xc8);

// PANZERS 0x5c76d0
SIUnitAnimation* SPBoatAnimation::CreateAnimation(SIUnit* unit)
{
    return new SBoatAnimation(this, unit);                        // new 200
}

SBoatAnimation::SBoatAnimation(SPBoatAnimation* proto, SIUnit* unit)
    : SVehicleAnimation(proto, unit)                              // 0x5c68e0
{
    Type = 9;
    BProto = proto;
    Unit = unit;
    _c4 = 0;
}

// PANZERS 0x5c70d0
SBoatAnimation::~SBoatAnimation()
{
}

// PANZERS 0x5c8bf0
void SBoatAnimation::InitModel(SIModel* model)
{
    SVehicleAnimation::InitModel(model);                          // 0x5c93a0
    Model()->SetFlags(0x27);                                      // +0x94 (tail jump)
}

// PANZERS 0x5cb4e0
// The vehicle update, then the unit's y is the water height (0x5ec490) and
// the model (and the unit's second model, +0x0c) stands there turned by pi
// (0x7f4584), level. Unit +0x2e9 (EnableUnloadButton) is set while the boat
// is off the static block bit 0x400 (0x5d9e00(x, z, 1, 0x400)).
void SBoatAnimation::UpdateModel()
{
    SVehicleAnimation::UpdateModel();                             // 0x5cd020
    SIUnit* u = Unit;
    float* pos = &UnitField<float>(u, kUnitPos);
    SUnitAnimEnv& e = g_UnitAnimEnv;
    pos[1] = e.WaterHeight ? e.WaterHeight(pos[0], pos[2]) : 0.0f;   // 0x5ec490, fstp +0x90
    Model()->SetPosition(pos[0], pos[1], pos[2]);                 // +0x18
    Model()->SetRotation(UnitField<float>(u, kUnitDir) + 3.1415927410125732f, 0.0f, 0.0f);   // +0x1c, 0x7f4584
    if (SIModel* m2 = UnitField<SIModel*>(u, 0x0c)) {
        m2->SetPosition(pos[0], pos[1], pos[2]);
        m2->SetRotation(UnitField<float>(u, kUnitDir) + 3.1415927410125732f, 0.0f, 0.0f);
    }
    UnitField<bool>(u, 0x2e9) = !(e.StaticBlocked && e.StaticBlocked(pos[0], pos[2], 1, 0x400));   // 0x5d9e00
}

} // namespace pz
