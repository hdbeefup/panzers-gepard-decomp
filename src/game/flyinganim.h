// src/game/flyinganim.h
// SFlyingAnimation (vftable 0x7fda24, 0xb0): the plane animation.
// OWNER: agent M3-C sub-agent C3.

#ifndef PZ_GAME_FLYINGANIM_H
#define PZ_GAME_FLYINGANIM_H

#include "flying.h"
#include "unitanim.h"

namespace pz {

// ---------------------------------------------------------------------------
// SFlyingAnimation

struct SFlyingAnimation : SUnitAnimation {              // 0xb0, vftable 0x7fda24
    SFlyingAnimation(SPFlyingAnimation* proto, SIUnit* unit);    // 0x5c63c0
    ~SFlyingAnimation() override;                       // 0x5c6af0 / 0x5c7150
    void InitModel(SIModel* model) override;            // 0x5c8d00
    void UpdateModel() override;                        // 0x5cb750
    float* GetFirePosition(float* out, int gunner, float dirOffset, float kick) override;   // 0x5cb120
    void AddBodyKick(float x, float y, float z) override;      // 0x5cb3e0

    struct SBombSlot {               // +0xa4 element: a bomb hung on "suspension%d"
        int      Node;
        SIModel* Model;              // gunner 0's first projectile model, attached to Node
    };

    SPFlyingAnimation* FProto;       // +0x24
    SRunningGear* Gear;              // +0x28
    SAnimGun*     Guns;              // +0x2c
    int           Muzzle;            // +0x30 next muzzle (GetFirePosition)
    int           BodyNode;          // +0x34 "body"
    int           AntennaNode;       // +0x38 "antenna"
    int           PropNode[3];       // +0x3c "prop0".."prop2"
    float         PropAngle[3];      // +0x48 +30 degrees per tick
    int           _54;               // +0x54
    double        Spring[2];         // +0x58 body sway (x, z)
    double        SpringVel[2];      // +0x68
    double        Rod[2];            // +0x78 antenna sway
    double        RodVel[2];         // +0x88
    float         AntennaPrev[2];    // +0x98
    bool          AntennaReset;      // +0xa0
    unsigned char _a1[3];
    SBombSlot*    Bombs;             // +0xa4
    int           BombCount;         // +0xa8
    int           _ac;               // +0xac
};

} // namespace pz

#endif // PZ_GAME_FLYINGANIM_H
