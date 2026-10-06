// src/world/worldunits.cpp
// SWorld unit rows of the menu loop: units crushing doodads (0x5e4870 /
// 0x5e3c80), the World+0x158 doodad animation heap (0x5d8b90) and the
// flying-fox refresh (0x5f6bf0). OWNER: M2-I sub-agent BW.
//
// The doodad footprint SDoodad::BlockRect (+0x2c, an SBlockBitmap from
// blockmaprefresh.h) is built by the doodad model's +0xa8(4, "Block")
// (SModel 0x6d5ca0) in SDoodad::UpdatePosition 0x6012d0. While that engine
// slot is not lifted every BlockRect is null and UnitMoved never reaches
// CrushDoodad.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "worldapi.h"
#include "blockmaprefresh.h"
#include "unit.h"
#include "punit.h"
#include "drivermath.h"
#include "gamelogic.h"
#include "pz/imodel.h"
#include "pz/ipixie.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

namespace {

const double kHalfPiD    = 1.5707963705062866;    // 0x7f5a38
const double kQuarterPiD = 0.7853981852531433;    // 0x7f7f50
const double kRand15     = 3.0517578125e-05;      // 0x7f4540 (1 / 32768)
const float  kPiF        = 3.1415927410125732f;   // 0x7f4584
const float  kTwoPiF     = 6.2831854820251465f;   // 0x7f458c
const float  kMinusPiF   = -3.1415927410125732f;  // 0x7f7fb8

inline int FloatBits(float f)
{
    int i;
    memcpy(&i, &f, 4);
    return i;
}

// HD inline SHeap<SDoodad>::operator[] (0x560870 when called).
SDoodad& DoodadAt(SWorld* w, int i)
{
    if (!w->Doodads.IsLive(i))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SDoodad", i);
    return w->Doodads.Array[i].Data;
}

SWorld::SDoodadAnim& DoodadAnimAt(SWorld* w, int i)
{
    SHeap<SWorld::SDoodadAnim>& h = w->DoodadAnims();
    if (!h.IsLive(i))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "SDDoodad", i);
    return h.Array[i].Data;
}

SUnit* UnitAt(SWorld* w, int i)
{
    if (!w->Units.IsLive(i))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", i);
    return w->Units.Array[i].Unit;
}

// HD tests the bool result of unit +0x1a4 in al.
inline bool TestPath(SUnit* u, short flags, int* bx, int* bz)
{
    int r = u->TestBlockMapPath(FloatBits(u->Pos[0]), FloatBits(u->Pos[2]), FloatBits(u->Dir),
                                u->UnitSizeBlocks, flags, (int)(size_t)bx, (int)(size_t)bz);
    return (unsigned char)r != 0;
}

} // namespace

static_assert(sizeof(SHeapElem<SWorld::SDoodadAnim>) == 0x20, "World+0x158 element 0x20");
static_assert(offsetof(SWorld::SDoodadAnim, Dir) == 0x14, "0x5e3c80 element +0x18");

// PANZERS 0x5d8b90
// SHeap<SDDoodad>::Alloc on World+0x158: reuses the free-list head (data
// cleared), else appends (growth 16, then * 6 / 5).
int SWorld::AllocDoodadAnim()
{
    SHeap<SDoodadAnim>& h = DoodadAnims();
    h.Count++;
    int i = h.Free;
    if (i >= 0) {
        h.Free = h.Array[i].Next;
        h.Array[i].Next = kHeapLive;
        memset(&h.Array[i].Data, 0, sizeof(SDoodadAnim));
        return i;
    }
    if (h.Size == h.Max) {
        int nmax = h.Max < 0x10 ? 0x10 : (h.Max * 6) / 5;
        h.Array = (SHeapElem<SDoodadAnim>*)realloc(h.Array, nmax * sizeof(SHeapElem<SDoodadAnim>));
        memset(&h.Array[h.Max], 0, (nmax - h.Max) * sizeof(SHeapElem<SDoodadAnim>));
        h.Max = nmax;
    }
    h.Array[h.Size].Next = kHeapLive;
    return h.Size++;
}

// PANZERS 0x5e4870
// Called by the drivers after every move (SDriver 0x553960 via
// g_DriverEnv.WorldUnitMoved). The unit's footprint is tested against the
// static block map bits 0x20 (scrub), 0x10 (demolishable, DemolishType >= 1)
// and 0x1000 (DemolishType >= 2); each test writes the cell it hit into
// (bx, bz). Every doodad whose footprint bitmap covers that cell and
// matches the hit kind is crushed. wantedSpeed (driver +0x24) is pushed by
// the caller but not read.
void SWorld::UnitMoved(int unit, float wantedSpeed)
{
    (void)wantedSpeed;
    bool hitDemolish = false;                                     // [ebp-1]
    bool hitDemolish2 = false;                                    // [ebp-2]
    int bx = 0, bz = 0;                                           // [ebp-0x14], [ebp-8]
    SUnit* u = UnitAt(this, unit);
    bool hitScrub = TestPath(u, 0x20, &bx, &bz);                  // +0x1a4
    int ct = u->Proto->ClassType;
    if (ct == 0 || ct == 0xb || ct == 0xc) {
        int dt = static_cast<SPSingleUnit*>(u->Proto)->DemolishType;   // +0x148
        if (dt >= 1) {
            if (TestPath(u, 0x10, &bx, &bz))
                hitDemolish = true;
        }
        if (dt >= 2) {
            if (TestPath(u, 0x1000, &bx, &bz)) {
                hitDemolish = true;
                hitDemolish2 = true;
            }
        }
    }
    if (!hitScrub && !hitDemolish && !hitDemolish2)
        return;
    for (int i = 0; i < Doodads.Size; ++i) {
        if (Doodads.Array[i].Next != kHeapLive)
            continue;
        const SDoodad& d = Doodads.Array[i].Data;
        const SBlockBitmap* r = (const SBlockBitmap*)d.BlockRect;
        if (!r)
            continue;
        if (r->X > bx || r->X + r->W <= bx)
            continue;
        if (r->Z > bz || r->Z + r->H <= bz)
            continue;
        bool hit = d._ac < 0 && d.Demolishable != 0 && hitDemolish &&
                   (d.Demolishable2 == 0 || hitDemolish2);
        if (!hit && !(hitScrub && d.Scrub != 0))
            continue;
        int dz = bz - r->Z;
        int dx = bx - r->X;
        if ((r->Bits[r->Stride * dz + (dx >> 3)] >> (dx & 7)) & 1) {
            SUnit* mu = UnitAt(this, unit);                       // 0x546490
            CrushDoodad(i, mu->Pos[0], mu->Pos[1], mu->Pos[2]);
        }
    }
}

// PANZERS 0x5e3c80
// A unit at (x, y, z) drove over doodad `doodad`: it gets (or reuses) an
// entry in the World+0x158 animation heap that SGameLogic::Refresh 0x576d80
// plays (Kind 0x10 falls away from the unit, 0x20 a scrub trembles, with
// one draw of the world random seed), and its demolish effect plays.
void SWorld::CrushDoodad(int doodad, float x, float y, float z)
{
    (void)y;
    SDoodad& d = DoodadAt(this, doodad);
    if (d.Demolishable != 0 || d.Demolishable2 != 0) {
        d.Flags = 0;
        if (d.BlockRect) {
            const SBlockBitmap* r = (const SBlockBitmap*)d.BlockRect;
            BlockMap_MarkDirty(this, r->X - 3, r->Z - 3, r->W + r->X + 3, r->H + r->Z + 3,
                               0x303c);                           // 0x5ef380
        }
    }
    int a = d._ac;
    if (a < 0)
        a = AllocDoodadAnim();                                    // 0x5d8b90
    SDoodadAnim& e = DoodadAnimAt(this, a);
    e.Doodad = doodad;
    e.Done = false;
    float mp[3] = { 0.0f, 0.0f, 0.0f };
    d.Model->GetPosition(mp);                                     // +0x10
    d.Model->SetFlags(0);                                         // +0x94
    d.Model->SetFlags(0x81);
    d.Model->StoreInterpolationState();                           // +0x3c
    e.Dir = DAtan2f((double)(mp[0] - x), (double)(mp[2] - z));    // 0x78d07a
    if (d.LightCount > 0)
        STUB_LOG("SWorld::CrushDoodad (0x5e3c80) doodad lights 0x5ffdc0 / 0x5e0f30");

    if (d.DemolishableFence == 1) {
        float a1 = (float)((double)d.Angle + kHalfPiD);
        float f = (float)fmod((double)a1, kHdTwoPi);              // 0x793cba
        if (f > kPiF)
            f -= kTwoPiF;
        else if (!(f > kMinusPiF))
            f += kTwoPiF;
        double diff = fabs((double)f - (double)e.Dir);
        if (diff > kHdPi)
            diff = kHdTwoPi - diff;
        if (kHalfPiD > diff)
            e.Dir = (float)((double)d.Angle + kHalfPiD);
        else
            e.Dir = (float)((double)d.Angle - kHalfPiD);
    }
    if (d.DemolishableFence == 2) {
        float f = (float)fmod((double)d.Angle, kHdTwoPi);         // 0x793cba
        if (f > kPiF)
            f -= kTwoPiF;
        else if (!(f > kMinusPiF))
            f += kTwoPiF;
        double diff = fabs((double)f - (double)e.Dir);
        if (diff > kHdPi)
            diff = kHdTwoPi - diff;
        if (kHalfPiD > diff)
            e.Dir = d.Angle;
        else
            e.Dir = d.Angle + kPiF;
    }
    if (d.Scrub != 0) {
        e.Amplitude = d.ScrubTrembling;
        int r = WorldRand();                                      // World+0x7518
        e.Dir = (float)((double)r * kRand15 * kHalfPiD - kQuarterPiD + (double)e.Dir);
        e.Kind = 0x20;
    } else {
        e.Amplitude = 0.0f;
        e.Velocity = d.DemolishAngleVelocity;
        e.Velocity2 = d.DemolishAngleVelocity;
        e.Kind = 0x10;
    }
    if (d.DemolishEffect1 != -1) {
        float pos[3] = { d.X, 0.0f, d.Z };
        pos[1] = GetTerrainHeight(d.X, d.Z) + d.DemolishEffectShift;   // 0x5e7730
        float up[3] = { 0.0f, 1.0f, 0.0f };
        g_Pixie->PlayEffect(g_Scene, d.DemolishEffect1, pos, up, 0);    // pixie +0x24
    }
    d._ac = a;
}

// PANZERS 0x5f6bf0
// Moves the flying-fox (cable car) trucks of World+0x7468 (SDArray, 0x1c
// elements). menu.map has none: only the entry part runs.
void SWorld::RefreshFlyingFox()
{
    float t = 0.0f;
    if (g_GameLogic)
        t = (float)g_GameLogic->Frame / 20.0f;                    // 0x56d1a0, 0x7f35d8
    (void)t;
    if (*(int*)((unsigned char*)this + 0x746c) <= 0)
        return;
    STUB_LOG("SWorld::RefreshFlyingFox (0x5f6bf0) flying-fox trucks");
}

namespace {

const float  kDegF      = 0.01745329238474369f;   // 0x7f59a0 (0x3c8efa35)
const float  kTenthF    = 0.1f;                   // 0x7f59a8 (0x3dcccccd)
const float  kSnapVelF  = 1.1f;                   // 0x7f59e0 (0x3f8ccccd)
const float  kTwoF      = 2.0f;                   // 0x7f4558
const float  kBounceF   = -0.5f;                  // 0x7f7f9c
const double kBackOffD  = 0.001;                  // 0x7f59b8 (copied to [ebp-0x68])

// The model of a falling tree / trembling scrub leans by `tilt` radians
// towards `dir`: SetRotation(angle, sin(dir) * tan(tilt), cos(dir) * tan(tilt)).
// HD calls tan twice (0x78d810) and cos before sin (pure functions).
void PoseLeaning(SDoodad& d, float dir, float tiltDeg)
{
    float t1 = (float)SseD(HdTan((double)(tiltDeg * kDegF)));    // [ebp-0x30]
    float t2 = (float)SseD(HdTan((double)(tiltDeg * kDegF)));    // [ebp-0x34]
    d.Model->StoreInterpolationState();                           // model +0x3c
    float tz = (float)DCos((double)dir) * t2;                     // 0x78d480
    float tx = (float)DSin((double)dir) * t1;                     // 0x78d640
    d.Model->SetRotation(d.Angle, tx, tz);                        // model +0x1c
}

} // namespace

// HD 0x577a10: inline loop of SGameLogic::Refresh 0x576d80 (0x577a10..0x577a47 and
// 0x577e2b..0x578924), after 0x5822a0: plays the World+0x158 doodad
// animations that CrushDoodad 0x5e3c80 starts. A falling doodad (Kind 0x10)
// integrates its tilt Amplitude (degrees) with Velocity (+= sin(tilt) *
// Velocity2) up to DemolishAngleMax, bouncing back once with half the speed;
// the entry is never removed (it keeps storing the model's interpolation
// state). A scrub (Kind 0x20) trembles with an amplitude attenuated by
// ScrubTremblingAtten each tick and is removed below 0.1. No random draws.
void SWorld::RefreshDoodadAnims()
{
    SHeap<SDoodadAnim>& h = DoodadAnims();
    for (int i = 0; i < h.Size; ++i) {
        if (h.Array[i].Next != kHeapLive)
            continue;
        SDoodadAnim& e = DoodadAnimAt(this, i);
        if (e.Kind == 0x10) {
            SDoodad& d = DoodadAt(this, e.Doodad);
            float target = d.DemolishAngleMax;                    // +0x80
            if (!(target > e.Amplitude)) {
                d.Model->StoreInterpolationState();               // model +0x3c
                continue;
            }
            float av = (float)fabs((double)e.Velocity);           // cvtps2pd, andps 0x7ea7a0, cvtpd2ps
            if (av > kSnapVelF) {
                if (d.DemolishEffect2 != -1 && !e.Done &&
                    e.Velocity * kTwoF + e.Amplitude > target) {
                    e.Done = true;
                    float pos[3] = { 0.0f, 0.0f, 0.0f };
                    pos[0] = (float)DSin((double)e.Dir) * d.DemolishEffectShift + d.X;   // 0x78d640
                    pos[2] = (float)DCos((double)e.Dir) * d.DemolishEffectShift + d.Z;   // 0x78d480
                    pos[1] = GetTerrainHeight(d.X, d.Z);          // 0x5e7730
                    float up[3] = { 0.0f, 1.0f, 0.0f };
                    g_Pixie->PlayEffect(g_Scene, d.DemolishEffect2, pos, up, 0);   // pixie +0x24
                }
                // 0x57822f
                e.Velocity = (float)DSin((double)(e.Amplitude * kDegF)) * e.Velocity2 + e.Velocity;
                e.Amplitude = e.Velocity + e.Amplitude;
                if (e.Amplitude >= target && e.Velocity > kTenthF) {
                    e.Amplitude = (float)((double)target - kBackOffD);
                    e.Velocity = e.Velocity * kBounceF;
                }
            } else {
                e.Amplitude = target;
            }
            // 0x578407
            PoseLeaning(d, e.Dir, e.Amplitude);
            // World->0x6090e0(d.Lights[l], 0) per attached light (not lifted).
            for (int l = 0; l < d.LightCount; ++l)
                STUB_LOG("SWorld::RefreshDoodadAnims (0x577a10) doodad light 0x6090e0(light, 0)");
        } else if (e.Kind == 0x20) {
            int di = e.Doodad;
            if (e.Amplitude > kTenthF) {
                SDoodad& d = DoodadAt(this, di);
                e.Amplitude = d.ScrubTremblingAtten * e.Amplitude;   // +0x9c
                PoseLeaning(d, e.Dir, e.Amplitude);
            } else {
                DoodadAt(this, e.Doodad)._ac = -1;
                // Inline SHeap<SDDoodad>::Remove (data kept).
                if (!h.IsLive(i))
                    Logger.g->Panic("SHeap<%s>::Remove: invalid index (%d)", "SDDoodad", i);
                h.Array[i].Next = h.Free;
                h.Count--;
                h.Free = i;
            }
        }
    }
}

// HD 0x577a47: inline loop of SGameLogic::Refresh 0x576d80 (0x577a47..0x577a84 and
// 0x578929): World->0x605930(i) for every live entry of the World+0x7454
// SHeap (element 0x44, the map's DWires). menu.map has none.
void SWorld::RefreshWires()
{
    static_assert(sizeof(SHeapElem<unsigned char[0x40]>) == 0x44, "World+0x7454 element 0x44");
    SHeap<unsigned char[0x40]>& h = *(SHeap<unsigned char[0x40]>*)((unsigned char*)this + 0x7454);
    for (int i = 0; i < h.Size; ++i) {
        if (h.Array[i].Next != kHeapLive)
            continue;
        STUB_LOG("SWorld::RefreshWires (0x577a47) World+0x7454 entry 0x605930");
    }
}

} // namespace pz
