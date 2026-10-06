// src/game/flyingdriver.cpp
// The plane driver SFlyingDriver / SPFlyingDriver (0x550150, 0x5510d0,
// 0x551ef0, 0x552410, 0x554fb0, 0x555d30, 0x5586a0, 0x55af10) and the
// paratroopers' SPanzersParachuteDriver / SPPanzersParachuteDriver
// (0x5502a0, 0x551150, 0x555ee0, 0x557d00, 0x55a360).
// OWNER: agent M3-C sub-agent C3. See flying.h.

#include <math.h>
#include <string.h>
#include "flyingdriver.h"
#include "flying_math.h"
#include "squadunit.h"
#include "drivermath.h"
#include "driverunit.h"
#include "manoeuvre.h"
#include "target.h"
#include "unitprops.h"
#include "world.h"
#include "worldapi.h"
#include "pz/imodel.h"
#include "logger.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {

PZ_HD_SIZE(SFlyingDriver, kHdSizeSFlyingDriver);
PZ_HD_SIZE(SPFlyingDriver, 0x58);
PZ_HD_SIZE(SPanzersParachuteDriver, kHdSizeSPanzersParachuteDriver);
PZ_HD_SIZE(SPPanzersParachuteDriver, 0x44);
#if defined(_M_IX86)
static_assert(offsetof(SFlyingDriver, GroundTracking) == 0xe8, "+0xe8");
static_assert(offsetof(SFlyingDriver, Climb) == 0xf0, "+0xf0");
static_assert(offsetof(SPFlyingDriver, Altitude) == 0x4c, "+0x4c");
static_assert(offsetof(SPFlyingDriver, GroundTracking) == 0x54, "+0x54");
#endif

static inline float Height(float x, float z) { return g_World->GetTerrainHeight(x, z); }   // 0x5e7730

struct SHdTrig {                                                  // flying_math.h policy: the HD CRT
    static double Sin(double x) { return DSin(x); }               // 0x78d640
    static double Cos(double x) { return DCos(x); }               // 0x78d480
};

void ParachuteMemberLanded(SUnit* m);                             // parachute.cpp

// PANZERS 0x55af10
void FlyingDriverSetGroundTracking(SIDriver* driver, bool on)
{
    static_cast<SFlyingDriver*>(driver)->SetGroundTracking(on);
}

// ---------------------------------------------------------------------------
// SPFlyingDriver / SFlyingDriver

// PANZERS 0x555d30
void SPFlyingDriver::Load(SProperties* propsp, const char* name, int nameLen)
{
    Type = 3;
    SUPropStruct* props = (SUPropStruct*)propsp;
    if (props->GetMultiIndex("DriverType") != Type)
        DrvPanic("SPFlyingDriver::Init - bad DriverType");
    SUPropStruct* s = props->GetMultiSubStruct("DriverType");
    MaxSpeed = (((s->GetFloat("MoveSpeed") / 3.6f) * 100.0f) / 20.0f) * 0.005f;   // 0x7f5a68 0x7ee558 0x7f35d8 0x7f5994
    WheelTurnAngle = s->GetFloat("WheelTurnAngle") * 0.017453292f;  // x87 PC24
    Wheelbase = (s->GetFloat("Wheelbase") * 100.0f) * 0.005f;
    MaxFuel = (int)s->GetFloat("MaxFuel");                         // 0x767700
    Consume = (float)(100.0 / (double)s->GetFloat("Consume"));
    Altitude = s->GetFloat("Altitude");
    MaxTilting = s->GetFloat("MaxTilting");
    GroundTracking = s->GetBool("Ground Tracking");
    SPDriver::Load(propsp, name, nameLen);                        // the name (0x52c320)
}

// PANZERS 0x5510d0
SIDriver* SPFlyingDriver::CreateDriver(SIUnit* unit)
{
    PZ_M3_TRACE("SPFlyingDriver::CreateDriver (0x5510d0)");
    return new SFlyingDriver(this, unit);
}

SFlyingDriver::SFlyingDriver(SPFlyingDriver* pd, SIUnit* unit) : SDriver(pd, unit)
{
    PDriver2 = pd;
    GroundTracking = pd->GroundTracking;
    _0e9[0] = _0e9[1] = _0e9[2] = 0;
    _0ec = 0;
    Climb = 0.0f;
    SpeedSteps = 1;
    SpinSteps = 0x14;
}

// PANZERS 0x550150
SFlyingDriver::~SFlyingDriver() {}

// PANZERS 0x554fb0
bool SFlyingDriver::PredictGhost(SGhostFrame* frame, int* outUnit, float* outPos, float* outDir)
{
    (void)frame; (void)outUnit; (void)outPos; (void)outDir;
    return false;
}

// PANZERS 0x551ef0
// Planes fly straight: the global path is the target point.
bool SFlyingDriver::FindGlobalPath(int size)
{
    (void)size;
    GlobalWayPoints.Clear(0);                                     // 0x550a30(0)
    int k = GlobalWayPoints.Add();                                // 0x550500
    GlobalWayPoints.At(k).X = Target->Pos[0];
    GlobalWayPoints.At(k).Z = Target->Pos[2];
    HasGlobalPath = 1;
    for (int i = 0; i < LocalWayPoints.Size; ++i)                 // 0x550aa0(0)
        Wpm_Free(&LocalWayPoints.Array[i]);
    LocalWayPoints.Clear(0);
    return true;
}

// PANZERS 0x552410
// Local path: the ghost, the target (recon planes then zig-zag over the map
// at half and one and a half times the target coordinates), back to the
// ghost's position (the plane leaves the map where it came from).
bool SFlyingDriver::FindLocalPath(SGhostFrame* f)
{
    PZ_M3_TRACE("SFlyingDriver::FindLocalPath (0x552410)");
    UpdateTurnRadius();                                           // 0x55b1d0
    SHdDArray<SWayPointWithManoeuvres>& L = LocalWayPoints;
    HasLocalPath = 1;
    int k = L.Add();                                              // 0x550570
    L.At(k).X = f->X;
    L.At(k).Z = f->Z;
    k = L.Add();
    L.At(k).X = Target->Pos[0];
    L.At(k).Z = Target->Pos[2];
    if (((SFlyingUnit*)Unit)->P->PlaneType == 1) {
        static const float kZig[4] = { 0.5f, 1.5f, 0.5f, 1.5f };  // 0x7f453c, 0x7f1b84
        for (int i = 0; i < 4; ++i) {
            k = L.Add();
            L.At(k).X = Target->Pos[0] * kZig[i];
            L.At(k).Z = Target->Pos[2] * kZig[i];
        }
    }
    k = L.Add();
    L.At(k).X = f->X;
    L.At(k).Z = f->Z;
    for (int i = 0; i < L.Size; ++i) {
        SWayPointWithManoeuvres& w = L.At(i);
        if (i == 0)
            w.PointType = kLwpFirst;
        else if (i == L.Size - 1)
            w.PointType = kLwpLastStop;
        else if (i == L.Size - 2)
            w.PointType = kLwpBeforeLast;
        else
            w.PointType = kLwpMiddle;
        if (i < L.Size - 1) {
            if (i == L.Size - 2 && Target->Type == kTargetPosDir)
                w.OutDir = Target->Dir;
            else
                w.OutDir = DAtan2f((double)(L.At(i + 1).X - w.X), (double)(L.At(i + 1).Z - w.Z));
        } else {
            w.OutDir = DAtan2f((double)(w.X - L.At(i - 1).X), (double)(w.Z - L.At(i - 1).Z));
        }
        if (i == 0)
            L.At(0).InDir = f->Dir;
        else
            w.InDir = L.At(i - 1).OutDir;
    }
    for (int i = f->WayPointIdx; i < L.Size - 1; ++i) {
        L.At(i).NextX = L.At(i + 1).X;
        L.At(i).NextZ = L.At(i + 1).Z;
        L.At(i).NextOutDir = L.At(i + 1).OutDir;
    }
    for (int i = f->WayPointIdx; i < L.Size; ++i)
        Wpm_CreateManoeuvres(&L.At(i), PDriver2->Type, Target, TurnRadius, f);   // 0x58d4e0
    return true;
}

// PANZERS 0x5586a0
// The ghost moves as on the ground but keeps its height; with ground
// tracking the plane climbs or sinks (Climb, +-0.0005 per tick) towards
// Altitude over the highest terrain 2..14 steps ahead.
bool SFlyingDriver::MoveTowardNextWayPoint(SGhostFrame* f)
{
    float y0 = f->Y;
    bool r = SDriver::MoveTowardNextWayPoint(f);                  // 0x5582a0
    f->Y = y0;
    SFlyingUnit* u = (SFlyingUnit*)Unit;
    if (u->P->PlaneType == 2 || !GroundTracking)
        return r;
    f->Y = FlyingClimbStep<SHdTrig>(&Climb, f->X, f->Y, f->Z, f->Dir, f->Speed, u->Altitude, Height);
    return r;
}

// ---------------------------------------------------------------------------
// Drivers

// PANZERS 0x555ee0
void SPPanzersParachuteDriver::Load(SProperties* propsp, const char* name, int nameLen)
{
    Type = 0xc;
    SUPropStruct* props = (SUPropStruct*)propsp;
    if (props->GetMultiIndex("DriverType") != Type)
        DrvPanic("SPPanzersParachuteDriver::Init - bad DriverType");
    SPDriver::Load(propsp, name, nameLen);                        // the name (0x52c320)
}

// PANZERS 0x551150
SIDriver* SPPanzersParachuteDriver::CreateDriver(SIUnit* unit)
{
    PZ_M3_TRACE("SPPanzersParachuteDriver::CreateDriver (0x551150)");
    return new SPanzersParachuteDriver(this, unit);
}

SPanzersParachuteDriver::SPanzersParachuteDriver(SPDriver* pd, SIUnit* unit) : SDriver(pd, unit)
{
    PDriver2 = pd;
}

// PANZERS 0x5502a0
SPanzersParachuteDriver::~SPanzersParachuteDriver() {}

// PANZERS 0x557d00
void SPanzersParachuteDriver::SetTarget(STarget* t)
{
    PZ_M3_TRACE("SPanzersParachuteDriver::SetTarget (0x557d00)");
    if (!t->Refresh(DU_I(Unit, 0x74)))                            // 0x5bd210
        return;
    if (SavedTarget) {
        TargetAssign(Target, SavedTarget);
        TargetClear(SavedTarget);
    }
    TargetAssign(Target, t);
    DU_B(Unit, 0xcd) = 1;
}

// PANZERS 0x55a360
// Falling: the unit moves by its velocity (+0xbc..+0xc4; y is then -0.1 per
// tick) down to the ground. On the ground the parachute folds for 40 ticks
// (the member walks 0.025 per tick along dir - 90 degrees after the fourth),
// then the member takes its walker driver back.
void SPanzersParachuteDriver::Refresh()
{
    PZ_M3_TRACE("SPanzersParachuteDriver::Refresh (0x55a360)");
    SUnit* u = (SUnit*)Unit;
    float h = g_World->GetTerrainHeight(u->Pos[0], u->Pos[2]);   // 0x5e7730, fstp dword
    if (u->Pos[1] == h) {
        if (u->Proto->ClassType != 6)
            return;
        SPanzersSquadMemberUnit* m = (SPanzersSquadMemberUnit*)u;
        if (m->Parachute) {
            m->Parachute->AdvanceAnimation(0.05f);                // +0x70 (0x3d4ccccd)
            m->Parachute->StoreInterpolationState();              // +0x3c
        }
        if (m->ParachuteTicks == 0)
            m->SetBehavior(1);                                    // +0x12c
        ++m->ParachuteTicks;
        if ((float)m->ParachuteTicks > 40.0f) {                   // 0x7f5a84
            m->SetActiveDriver(0);                                // 0x5c0cb0
            m->Invulnerable = false;                              // +0x111
            WorldUnit(m->Parent)->Invulnerable = false;
            ParachuteMemberLanded(m);                             // 0x5cae80(1), 0x5cb0a0
            return;
        }
        if ((float)m->ParachuteTicks > 4.0f)                      // 0x7f4588
            ParachuteWalkStep<SHdTrig>(m->Pos, m->Dir);
        return;
    }
    ParachuteFallStep(u->Pos, (float*)((char*)u + 0xbc), Height);   // velocity +0xbc
    SIModel* chute = u->Proto->ClassType == 6 ? ((SPanzersSquadMemberUnit*)u)->Parachute : nullptr;
    if (chute)
        chute->SetPosition(u->Pos[0], u->Pos[1], u->Pos[2]);      // +0x18
    if (chute)
        chute->StoreInterpolationState();                         // +0x3c
}

} // namespace pz
