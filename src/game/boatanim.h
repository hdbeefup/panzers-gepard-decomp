// src/game/boatanim.h
// SBoatAnimation (vftable 0x7fd904, 0xc8): the vehicle animation of a boat
// (landing craft): the model floats on the water height. OWNER: agent MS.

#ifndef PZ_GAME_BOATANIM_H
#define PZ_GAME_BOATANIM_H

#include "unitanim.h"

namespace pz {

struct SBoatAnimation : SVehicleAnimation {             // 0xc8, vftable 0x7fd904
    SBoatAnimation(SPBoatAnimation* proto, SIUnit* unit);       // 0x5c76d0 (inline in the factory)
    ~SBoatAnimation() override;                         // 0x5c70d0
    void InitModel(SIModel* model) override;            // 0x5c8bf0
    void UpdateModel() override;                        // 0x5cb4e0

    SPBoatAnimation* BProto;         // +0xc0
    int              _c4;            // +0xc4
};

} // namespace pz

#endif // PZ_GAME_BOATANIM_H
