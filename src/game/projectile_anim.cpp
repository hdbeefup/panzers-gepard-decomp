// src/game/projectile_anim.cpp
// SProjectileAnimation (OWNER: M3-C sub-agent C1).

#include <math.h>
#include <string.h>
#include "projectile.h"
#include "projectile_anim.h"
#include "gunnermath.h"
#include "unitprops.h"
#include "world.h"
#include "worldapi.h"
#include "gamelogic.h"
#include "drivermath.h"
#include "pz/imodel.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {

static bool UnitLive(int i)
{
    return g_World && g_World->Units.IsLive(i);
}

static SUnit* LiveUnit(int i)                                // SHeapTRB::operator[] 0x546490
{
    if (!UnitLive(i))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", i);
    return WorldUnit(i);
}

PZ_HD_SIZE(SProjectileAnimation, kHdSizeSProjectileAnimation);
#if defined(_M_IX86)
static_assert(offsetof(SProjectileAnimation, Spin) == 0x28, "0x5c9320 +0x28");
#endif

static bool NameIs(SPUnit* p, const char* s)                 // SString::operator== 0x52c410 (case-insensitive)
{
    return _stricmp(SStr(p->Name), s) == 0;
}

// ---------------------------------------------------------------------------
// SProjectileAnimation

} // namespace pz

// The new + ctor of SPProjectileAnimation::CreateAnimation 0x5c78e0
// (unitanim.cpp calls it; animview links a null default).
extern "C" pz::SIUnitAnimation* PzNewProjectileAnimation(pz::SPProjectileAnimation* proto, pz::SIUnit* unit)
{
    return new pz::SProjectileAnimation(proto, unit);         // new 0x2c
}

namespace pz {

SProjectileAnimation::SProjectileAnimation(SPProjectileAnimation* proto, SIUnit* unit)
    : SUnitAnimation(proto, unit)                             // 0x5c67d0 (two world-LCG draws)
{
    PProto = proto;                                           // +0x24
    Type = 3;                                                 // +0x0c
    Spin = 0.0f;
}

// PANZERS 0x5c73a0
SProjectileAnimation::~SProjectileAnimation()
{
}

// PANZERS 0x5c9320
void SProjectileAnimation::InitModel(SIModel* model)
{
    (void)model;
    Spin = 0.0f;
    Model()->SetFlags(0x13);                                  // model +0x94
}

// PANZERS 0x5cb220
float* SProjectileAnimation::GetFirePosition(float* out, int gunner, float dirOffset, float kick)
{
    (void)gunner;
    (void)dirOffset;
    (void)kick;
    out[0] = out[1] = out[2] = 0.0f;
    return out;
}

// PANZERS 0x5cc480
// The model follows the projectile: grenades, Molotovs (and catapult
// stones, the other way) roll with Spin; shells point along the velocity.
// A wrecked projectile's +0x155 request becomes the removal count +0x15c.
void SProjectileAnimation::UpdateModel()
{
    SUnit* u = (SUnit*)Unit;
    SIModel* m = Model();
    bool show = false;
    if (!u->Unplaced) {
        unsigned char menuWorld = *((unsigned char*)g_World + 0x4d0);
        if (menuWorld || !g_GameLogic ||
            g_GameLogic->CanSeeGroundUnit(*(int*)((unsigned char*)g_World + 0x16c), u))   // 0x562760
            show = true;
    }
    m->SetVisible(show, false);                               // model +0x30
    const char* name = SStr(u->Proto->Name);
    int len = u->Proto->Name.size;
    float* v = (float*)&u->_bc[0];
    bool roll = (len != 0 && _stricmp(name, "Projectile Ge grenade") == 0) ||
                (len != 0 && _stricmp(name, "Projectile US grenade") == 0) ||
                (len != 0 && _stricmp(name, "Projectile Molotov coctail") == 0);
    if (roll || NameIs(u->Proto, "Projectile catapult")) {
        float h2 = v[0] * v[0] + v[2] * v[2];
        m->SetPosition(u->Pos[0], u->Pos[1], u->Pos[2]);      // +0x18
        if ((double)h2 > 0.0001) {                            // 0x7f1b50
            m->SetRotation(DAtan2f((double)v[0], (double)v[2]), 0.0f, 0.0f);   // +0x1c
            m->SetNodeTilt(0, 0.0f, 0.0f, 0.0f, Spin, 0.0f, 0.0f);   // +0x48
        } else {
            m->SetRotation(0.0f, 0.0f, 0.0f);
        }
        if (roll)
            Spin = Spin + 0.4f;                               // DAT_007f5e20
        else
            Spin = Spin - 0.4f;
    } else {
        float h2 = v[0] * v[0] + v[2] * v[2];
        m->SetPosition(u->Pos[0], u->Pos[1], u->Pos[2]);
        if ((double)h2 > 1e-05) {                             // 0x7fd6e8
            float tz = -((v[1] * v[2]) / h2);
            float tx = -((v[0] * v[1]) / h2);
            m->SetRotation(DAtan2f((double)v[0], (double)v[2]), tx, tz);
        } else {
            m->SetRotation(0.0f, 0.0f, 0.0f);
        }
    }
    unsigned char* b = (unsigned char*)u;
    if (b[0x155]) {
        u->_15c = 1;
        b[0x155] = 0;
    }
}

} // namespace pz
