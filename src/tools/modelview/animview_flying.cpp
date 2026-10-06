// src/tools/modelview/animview_flying.cpp
// animview links the unit animation sources without the game logic; the
// plane and waster animations (src/game/flying.cpp, waster.cpp) need the
// world and the units, so the tool shows neither.

#include "unitanim.h"

namespace pz {

SIUnitAnimation* SPFlyingAnimation::CreateAnimation(SIUnit* unit)
{
    (void)unit;
    return nullptr;
}

SIUnitAnimation* SPWasterAnimation::CreateAnimation(SIUnit* unit)
{
    (void)unit;
    return nullptr;
}

} // namespace pz
