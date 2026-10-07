// src/game/traindriver.cpp
// STrainDriver / SPTrainDriver (0x5503c0, 0x551340, 0x551fa0, 0x552b50,
// 0x556560, 0x557ec0, 0x559230). OWNER: agent TR. See traindriver.h.

#include <math.h>
#include <string.h>
#include "traindriver.h"
#include "trainunit.h"
#include "rail.h"
#include "drivermath.h"
#include "driverunit.h"
#include "manoeuvre.h"
#include "target.h"
#include "unitprops.h"
#include "world.h"
#include "worldapi.h"
#include "logger.h"
#include "m3common.h"

namespace pz {

PZ_HD_SIZE(STrainDriver, kHdSizeSTrainDriver);
PZ_HD_SIZE(SPTrainDriver, 0x44);

static inline float Height(float x, float z) { return g_World->GetTerrainHeight(x, z); }   // 0x5e7730
static inline int Bits(float f) { int i; memcpy(&i, &f, 4); return i; }

// PANZERS 0x556560
void SPTrainDriver::Load(SProperties* propsp, const char* name, int nameLen)
{
    Type = 0xd;
    SUPropStruct* props = (SUPropStruct*)propsp;
    if (props->GetMultiIndex("DriverType") != Type)
        DrvPanic("SPTrainDriver::Init - bad DriverType");
    SUPropStruct* s = props->GetMultiSubStruct("DriverType");
    MaxSpeed = (((s->GetFloat("MoveSpeed") / 3.6f) * 100.0f) / 20.0f) * 0.005f;   // 0x7f5a68 0x7ee558 0x7f35d8 0x7f5994
    SPDriver::Load(propsp, name, nameLen);                        // the name (0x52c320)
}

// PANZERS 0x551340
SIDriver* SPTrainDriver::CreateDriver(SIUnit* unit)
{
    return new STrainDriver(this, unit);
}

STrainDriver::STrainDriver(SPTrainDriver* pd, SIUnit* unit) : SDriver(pd, unit)   // 0x54f4b0
{
    PDriver2 = pd;                                                // +0xe4
    SpeedSteps = 100;                                             // +0xd4
    MaxGhostFrames = 0x118;                                       // +0xdc
}

// PANZERS 0x5503c0
STrainDriver::~STrainDriver() {}

// PANZERS 0x557ec0
// Only a target on the train's own road is taken. The point becomes the
// nearest free track position: for a computer player the train's head (or,
// backing, its tail: the towed chain's lengths) stops short of blocked track
// in 2-unit steps; Reverse when the target lies behind.
void STrainDriver::SetTarget(STarget* t)
{
    STrainUnit* u = static_cast<STrainUnit*>(static_cast<SUnit*>(Unit));
    if (!t->Refresh(u->WorldIndex) || u->Road < 0)                // 0x5bd210
        return;
    float p[3] = { t->Pos[0], Height(t->Pos[0], t->Pos[2]), t->Pos[2] };
    float tdist;
    if (RailFindNearestRoad(p, &tdist) != u->Road)                // 0x5e8750
        return;
    if (SavedTarget) {
        TargetAssign(Target, SavedTarget);
        TargetClear(SavedTarget);
    }
    TargetAssign(Target, t);
    DU_B(Unit, 0xcd) = 1;                                         // unit +0xcd
    ResetGhost();                                                 // 0x554770
    LocalPathFailures = 0;
    CollisionCount = 0;
    AvoidUnit = -1;
    LastCollisionUnit = -1;
    GlobalWayPoints.Clear(0);                                     // 0x550a30(0)
    float pos[2] = { 0.0f, 0.0f }, dir[2] = { 0.0f, 0.0f };
    float chain = 0.0f;
    RailGetPositionOnRoad(u->Road, &tdist, pos, dir);             // 0x5e98e0
    if (*(int*)(g_World->Players[u->Player] + 8) == 0) {          // World+0x178 + player * 0x48
        SUnit* c = u;
        float len = 0.0f;
        while (c->Towed >= 0) {                                   // the towed chain's sizes
            c = WorldUnit(c->Towed);
            len = c->UnitSize + chain;
            chain = len;
        }
        float d = tdist;
        if (tdist < u->_300)
            d = tdist - len;
        float q[2] = { 0.0f, 0.0f }, qd[2] = { 0.0f, 0.0f };
        RailGetPositionOnRoad(u->Road, &d, q, qd);
        for (;;) {
            float a = DAtan2f((double)qd[0], (double)qd[1]);      // 0x78d07a
            if (!u->TestBlockMap(Bits(q[0]), Bits(q[1]), Bits(a), u->UnitSizeBlocks, 1))   // +0x1a8
                break;
            float next;
            if (u->_300 <= tdist) {
                next = d - 2.0f;                                  // 0x7f4558
                d = u->_300;
                if (next <= d)
                    break;
            } else {
                next = d + 2.0f;
                if (u->_300 - chain <= next) {
                    d = u->_300 - chain;
                    break;
                }
            }
            d = next;
            RailGetPositionOnRoad(u->Road, &d, q, qd);
        }
        bool behind = tdist < u->_300;
        tdist = d;
        if (behind)
            tdist = d + chain;
        RailGetPositionOnRoad(u->Road, &tdist, pos, dir);
    }
    t->Pos[0] = pos[0];
    t->Pos[2] = pos[1];
    t->Reverse = tdist < u->_300;                                 // target +0x2c
}

// PANZERS 0x559230
// The ghost runs along the road by its speed (backwards in reverse).
bool STrainDriver::MoveTowardNextWayPoint(SGhostFrame* f)
{
    STrainUnit* u = static_cast<STrainUnit*>(static_cast<SUnit*>(Unit));
    if (u->Road < 0 || !Target)
        return false;
    SetUnitSpeed(f);                                              // 0x55b2d0
    float pos[2] = { 0.0f, 0.0f }, dir[2] = { 0.0f, 0.0f };
    f->Field70 = (float)((f->Reverse ? 0 : 1) * 2 - 1) * f->Speed + f->Field70;
    RailGetPositionOnRoad(u->Road, &f->Field70, pos, dir);        // 0x5e98e0
    f->X = pos[0];
    f->Z = pos[1];
    f->Y = Height(pos[0], pos[1]);
    f->Dir = DAtan2f((double)dir[0], (double)dir[1]);             // 0x78d07a
    return true;
}

// PANZERS 0x551fa0
// The global path is the target point.
bool STrainDriver::FindGlobalPath(int size)
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

// PANZERS 0x552b50
// Local path: the ghost and the target, no manoeuvres (the road does the
// steering); duplicate points removed.
bool STrainDriver::FindLocalPath(SGhostFrame* f)
{
    UpdateTurnRadius();                                           // 0x55b1d0
    SHdDArray<SWayPointWithManoeuvres>& L = LocalWayPoints;
    HasLocalPath = 1;
    int k = L.Add();                                              // 0x550570
    L.At(k).X = f->X;
    L.At(k).Z = f->Z;
    k = L.Add();
    L.At(k).X = Target->Pos[0];
    L.At(k).Z = Target->Pos[2];
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
    int i = 0;
    while (i < L.Size - 1) {
        if (L.At(i).X == L.At(i + 1).X && L.At(i).Z == L.At(i + 1).Z) {
            Wpm_Free(&L.Array[i]);                                // 0x559810
            L.Remove(i);
        } else {
            ++i;
        }
    }
    return true;
}

} // namespace pz
