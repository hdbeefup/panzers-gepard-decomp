// src/game/trainanim.h
// STrainAnimation (vftable 0x7fd8bc, 0xc8): the vehicle animation of a
// locomotive / carriage plus the looping "vonat" sequence (wheels, rods,
// smoke) run at the train's speed. OWNER: agent TR.

#ifndef PZ_GAME_TRAINANIM_H
#define PZ_GAME_TRAINANIM_H

#include "unitanim.h"

namespace pz {

struct STrainAnimation : SVehicleAnimation {            // 0xc8, vftable 0x7fd8bc
    STrainAnimation(SPTrainAnimation* proto, SIUnit* unit);     // 0x5c7990 (inline in the factory)
    ~STrainAnimation() override;                        // 0x5c7490
    void InitModel(SIModel* model) override;            // 0x5c9350
    void UpdateModel() override;                        // 0x5cce80

    SPTrainAnimation* TProto;        // +0xc0
    int               _c4;           // +0xc4
};

} // namespace pz

#endif // PZ_GAME_TRAINANIM_H
