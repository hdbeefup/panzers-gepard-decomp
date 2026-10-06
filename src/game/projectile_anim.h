// src/game/projectile_anim.h
// SProjectileAnimation (OWNER: M3-C sub-agent C1). See projectile.h.

#ifndef PZ_GAME_PROJECTILE_ANIM_H
#define PZ_GAME_PROJECTILE_ANIM_H

#include "unitanim.h"

namespace pz {

struct SProjectileAnimation : SUnitAnimation {           // vftable 0x7fd994, 0x2c bytes
    SProjectileAnimation(SPProjectileAnimation* proto, SIUnit* unit);   // 0x5c78e0 (inline)
    ~SProjectileAnimation() override;                   // 0x5c73a0
    void InitModel(SIModel* model) override;            // 0x5c9320
    void UpdateModel() override;                        // 0x5cc480
    float* GetFirePosition(float* out, int gunner, float dirOffset, float kick) override;   // 0x5cb220 (zeros)

    SPProjectileAnimation* PProto;  // +0x24
    float Spin;                     // +0x28 grenades / catapult stones roll 0.4 a tick
};

} // namespace pz

#endif // PZ_GAME_PROJECTILE_ANIM_H
