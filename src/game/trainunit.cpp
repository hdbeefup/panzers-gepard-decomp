// src/game/trainunit.cpp
// STrainUnit (0x5b0e70..0x5b1d40), and the towing the trains need from
// SUnit: Tow 0x5c2390 (the map's towed units, the carriages of a train) and
// GhostFrames_AddTop 0x5b5c10 (where a towed unit stands behind its tower).
// OWNER: agent TR. See trainunit.h.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "trainunit.h"
#include "rail.h"
#include "drivermath.h"
#include "drivertypes.h"
#include "driverunit.h"
#include "gamelogic.h"
#include "gunner.h"
#include "idriver.h"
#include "iunitanim.h"
#include "trigger.h"
#include "world.h"
#include "worldapi.h"
#include "blockmaprefresh.h"
#include "pz/imodel.h"
#include "logger.h"
#include "m3common.h"

namespace pz {

int UnitAnimFindState(SIUnit* unit, const char* name);           // trainanim.cpp (0x5c7ed0)

static inline float Height(float x, float z) { return g_World->GetTerrainHeight(x, z); }   // 0x5e7730
static inline int Bits(float f) { int i; memcpy(&i, &f, 4); return i; }
static inline float Flt(int i) { float f; memcpy(&f, &i, 4); return f; }

static inline float WrapPi(double a)                              // the +-pi wrap of 0x5b0f00 / 0x5b5c10
{
    if (a > 3.1415927410125732)                                   // 0x7f4560
        a -= 6.2831854820251465;                                  // 0x7f4570
    else if (a < -3.1415927410125732)                             // 0x7f5aa0
        a += 6.2831854820251465;
    return (float)a;
}

// The part of 0x5b5c10 / 0x5b0f00 off the rails: the towed unit hangs at its
// hole node (front, or rear turned around) the hole's distance behind the
// tower's hook, facing the hook, at most 90 degrees off the tower's
// direction. tpos: where the towed unit is now.
static void TowBehindHook(float hookX, float y, float hookZ, float dir, const float* tpos,
                          float holeLen, bool front, float* outPos, float* outDir)
{
    float dx = hookX - tpos[0];
    float dy = y - tpos[1];
    float dz = hookZ - tpos[2];
    double len = sqrt((double)(dx * dx + dy * dy + dz * dz));
    double inv = 1.0 / len;                                       // 0x7eed98
    float nx = (float)((double)dx * inv);
    float nz = (float)((double)dz * inv);
    float a = DAtan2f((double)nx, (double)nz);                    // 0x78d07a
    *outDir = a;
    float diff = WrapPi((double)dir - (double)a);
    if ((double)diff > 1.5707963705062866)                        // 0x7f5a38
        *outDir = WrapPi((double)dir - 1.5707963705062866);
    if ((double)diff < -1.5707963705062866)                       // 0x7f7fa8
        *outDir = WrapPi((double)dir + 1.5707963705062866);
    float s = (float)DSin((double)*outDir);                       // 0x78d640
    float c = (float)DCos((double)*outDir);                       // 0x78d480
    outPos[0] = hookX - s * holeLen;
    outPos[1] = y - holeLen * 0.0f;
    outPos[2] = hookZ - c * holeLen;
    outPos[1] = Height(outPos[0], outPos[2]);
    if (!front) {
        double r = (double)*outDir + 3.1415927410125732;
        if (r > 3.1415927410125732)
            *outDir = (float)(r - 6.2831854820251465);
        else {
            if (r < -3.1415927410125732)
                r += 6.2831854820251465;
            *outDir = (float)r;
        }
    }
}

// The tower's hook in world space (anim +0x30, rotated by dir) and the towed
// unit's hole offset (anim +0x34 "hole_f", else +0x38 "hole_r").
static void HookAndHole(SIUnit* tower, float x, float z, float dir, SIUnit* towed,
                        float* hook2, float* hookW, float* hole2, bool* front)
{
    SUnit* t = static_cast<SUnit*>(towed);
    static_cast<SUnit*>(tower)->Anim->GetHookPos(hook2);          // +0x30
    double c = DCos((double)dir);                                 // 0x78d480
    double s = DSin((double)dir);                                 // 0x78d640
    hookW[0] = (float)((double)hook2[0] * c + (double)hook2[1] * s + (double)x);
    hookW[1] = (float)((double)hook2[1] * c - (double)hook2[0] * s + (double)z);
    int node = t->Anim->GetHoleFNode();                           // +0x28
    *front = true;
    t->Anim->GetHoleFPos(hole2);                                  // +0x34
    if (node < 0) {
        node = t->Anim->GetHoleRNode();                           // +0x2c
        *front = false;
        float r[2];
        float* p = t->Anim->GetHoleRPos(r);                       // +0x38
        hole2[0] = p[0];
        hole2[1] = p[1];
        if (node < 0)
            Logger.g->Panic("SUnit::GhostFrames_AddTop - towed_hole_node_idx < 0, Unitname:%s",
                            SStr(t->Proto->Name));
    }
}

// PANZERS 0x5b5c10
// A towed unit (guns, trailers) behind its tower at x, y, z / dir; it is
// pulled from the top of its own ghost queue (or its position).
void SUnit::GhostFramesAddTop(float x, float y, float z, float dir, float dist, SIUnit* towed,
                              float* outPos, float* outDir, float* outDist)
{
    (void)dist; (void)outDist;
    SUnit* t = static_cast<SUnit*>(towed);
    float hook2[2], hookW[2], hole2[2];
    bool front;
    float tpos[3];
    SGhostQueue* q = DU_Ghosts(t);
    if (q->Count < 1) {
        tpos[0] = t->Pos[0];
        tpos[1] = t->Pos[1];
        tpos[2] = t->Pos[2];
    } else {
        SGhostFrame* f = GhostAt(t, q->Top);                      // SDEQueue<SGhostFrame>::operator[]
        tpos[0] = f->X;
        tpos[1] = f->Y;
        tpos[2] = f->Z;
    }
    HookAndHole(this, x, z, dir, towed, hook2, hookW, hole2, &front);
    float holeLen = (float)sqrt((double)(hole2[0] * hole2[0] + hole2[1] * hole2[1]));
    TowBehindHook(hookW[0], y, hookW[1], dir, tpos, holeLen, front, outPos, outDir);
}

// ---------------------------------------------------------------------------
// SUnit::Tow

// PANZERS 0x5c1f00
// The towed unit's speed limit (+0x2dc): its own driver's, or 0.
static void SetTowSpeed(SUnit* t)
{
    SIPDriver* pd = nullptr;
    float v;
    if (t->GetActivePDriver(&pd))                                 // 0x5b9c70
        v = t->GetMoveSpeed(-1);                                  // +0x1b0
    else if (t->Drivers.Size > 0)
        v = *(float*)((unsigned char*)t->Drivers.Array[0]->GetPDriver() + 8);   // SPDriver +0x08 MaxSpeed
    else
        v = 0.0f;
    memcpy(&t->_2dc, &v, 4);
}

// PANZERS 0x571660
// Trigger event 7 "starts towing": Unit = the tower, Unit2 = the towed unit.
static void DispatchStartsTowing(int unit, int towed)
{
    SRunningTrigger rt;
    memset(&rt, 0, sizeof(rt));                                   // memset 0x34
    SHdArray<STrigger>* t = &g_World->Triggers;
    for (int i = 0; i < t->Size; ++i) {
        if (t->Array[i].Event.Type != TE_STARTS_TOWING)
            continue;
        rt.Unit = unit;
        rt.Unit2 = towed;
        rt.Trigger = i;
        g_GameLogic->CheckConditions(&rt);                        // 0x580600
        t = &g_World->Triggers;
    }
    free(rt.Found.Units);
}

// PANZERS 0x5c2390
void SUnit::Tow(int unit)
{
    if (!g_World->Units.IsLive(unit))
        return;
    Towed = unit;                                                 // +0x2d8
    SUnit* t = WorldUnit(unit);
    t->StopUnit();                                                // 0x5c2d30
    t->Parent = WorldIndex;                                       // +0x78
    t->_7c = true;
    SetTowSpeed(t);                                               // 0x5c1f00
    t->SetBehavior(UnitAnimFindState(t, "towed"));                // +0x12c(anim 0x5c7ed0("towed"))
    t->SetOnBlockMap(false);                                      // +0x198
    GhostFramesAddTop(Pos[0], Pos[1], Pos[2], Dir, _300, t, t->Pos, &t->Dir, &t->_300);   // +0x74
    t->SetOnBlockMap(true);
    if (t->Proto->ClassType == 0xb && t->Stored.Size > 0) {
        int crew = *(int*)&t->Stored.Array[0];                    // the first stored unit
        t->UnloadAll();                                           // +0x68
        WorldUnit(crew)->Slot_60(WorldIndex);                     // +0x60
    }
    if (g_GameLogic) {
        DispatchStartsTowing(WorldIndex, unit);                   // 0x571660
        if (g_GameLogic)
            g_GameLogic->RemoveUnitFromMovementGroup(unit);       // 0x579510
    }
}

// ---------------------------------------------------------------------------
// STrainUnit

PZ_HD_SIZE(STrainUnit, kHdSizeSTrainUnit);
#if defined(_M_IX86)
static_assert(offsetof(STrainUnit, Road) == 0x3ac, "+0x3ac");
static_assert(offsetof(STrainUnit, BlockFootprint) == 0x3b0, "+0x3b0");
#endif

// PANZERS 0x5b0e70
STrainUnit::STrainUnit(SPTrainUnit* proto, int worldIndex)
    : SSingleUnit(proto, worldIndex)
{
    TP = proto;
    Road = -1;
    BlockFootprint = nullptr;
}

// PANZERS 0x5b0ed0
STrainUnit::~STrainUnit()
{
}

// PANZERS 0x5b15e0
void STrainUnit::Uninit()
{
    SSingleUnit::Uninit();                                        // 0x5abde0
    BlockBitmap_Free(BlockFootprint);                             // 0x661b30, delete(0x1c)
    BlockFootprint = nullptr;
}

// PANZERS 0x5b1650
void STrainUnit::Init(SUnitDef* def)
{
    SSingleUnit::Init(def);                                       // 0x5ad150
    InitRail();                                                   // 0x5b1be0
    if (g_GameLogic)
        RefreshTargeting();                                       // +0x34
}

// PANZERS 0x5b1680
void STrainUnit::InitNew(int player, const float* pos, float dir, int p4, float hp)
{
    SSingleUnit::InitNew(player, pos, dir, p4, hp);               // 0x5ad9f0
    InitRail();
    if (g_GameLogic)
        RefreshTargeting();
}

// PANZERS 0x5b1be0
// No driver without a built-in one, the guns off; the train snaps to the
// nearest road (+0x3ac, length +0x300) and faces along it.
void STrainUnit::InitRail()
{
    if (!TP->BuiltInDriver)                                       // proto +0xc8
        SetActiveDriver(-1);                                      // 0x5c0cb0
    for (int i = 0; i < Gunners.Size; ++i)
        Gunners.Array[i]->Active = 0;                             // gunner +0x1c
    float p[3] = { Pos[0], Height(Pos[0], Pos[2]), Pos[2] };
    Road = RailFindNearestRoad(p, &_300);                         // 0x5e8750
    if (getenv("PZ_TR_TEST") && Logger.g)                         // recompile test switch (not HD)
        Logger.g->Log(0, "PZTR: InitRail unit %d %s at %.2f %.2f %.2f -> road %d dist %.2f",
                      WorldIndex, SStr(Proto->Name), (double)p[0], (double)p[1], (double)p[2], Road, (double)_300);
    if (Road >= 0) {
        float xz[2] = { 0.0f, 0.0f }, d[2] = { 0.0f, 0.0f };
        RailGetPositionOnRoad(Road, &_300, xz, d);                // 0x5e98e0
        Pos[0] = xz[0];
        Pos[2] = xz[1];
        Dir = DAtan2f((double)d[0], (double)d[1]);                // 0x78d07a
    }
    SetUnitSize();                                                // +0x1c4
}

// Not HD (see trainunit.h): SSingleUnit::RefreshMisc 0x5af890 plus the
// PZ_TR_TEST log / camera follow.
void STrainUnit::RefreshMisc()
{
    SSingleUnit::RefreshMisc();
    static int s_Test = -1;
    static bool s_Followed = false;
    if (s_Test < 0)
        s_Test = getenv("PZ_TR_TEST") ? 1 : 0;
    if (!s_Test || !g_GameLogic)
        return;
    int frame = g_GameLogic->Frame;
    *((unsigned char*)this + 0x169) = 1;                          // test only: shown through the fog of war
    if (frame % 200 == 0 && Logger.g)
        Logger.g->Log(0, "PZTR: frame %d unit %d %s road %d dist %.2f pos %.1f %.1f dir %.2f speed %.3f towed %d driver %d",
                      frame, WorldIndex, SStr(Proto->Name), Road, (double)_300, (double)Pos[0], (double)Pos[2],
                      (double)Dir, (double)Speed, Towed, ActiveDriver);
    if (!s_Followed && Towed >= 0 && !_7c && frame > 40) {        // the head of a chain
        s_Followed = true;
        g_World->CamTarget[0] = Pos[0];                           // as the camera jump 0x5f4f60, unclamped
        g_World->CamTarget[2] = Pos[2];
        g_World->CamFollowUnit = WorldIndex;
    }
}

// PANZERS 0x5b16d0
void STrainUnit::Slot_14()
{
    SSingleUnit::Slot_14();                                       // 0x5ae560 (board; SUnit 0x5bb1c0)
    SetUnitSize();                                                // +0x1c4
}

// PANZERS 0x5b1980
// Teleport onto the track: the length nearest to (x, z) on the current
// road, else on the nearest road; off the rails it keeps x, z and dir.
void STrainUnit::SetPosition(float x, float z, int dirBits, int yrelBits)
{
    SetOnBlockMap(false);                                         // +0x198
    bool found = false;
    if (Road >= 0) {
        float p[3] = { x, Height(x, z), z };
        found = RailProjectOnRoad(p, Road, &_300);                // 0x5e7ae0
    }
    if (!found) {
        float p[3] = { x, Height(x, z), z };
        Road = RailFindNearestRoad(p, &_300);                     // 0x5e8750
    }
    if (Road < 0) {
        Pos[0] = x;
        Pos[2] = z;
        Dir = Flt(dirBits);
    } else {
        float xz[2] = { 0.0f, 0.0f }, d[2] = { 0.0f, 0.0f };
        RailGetPositionOnRoad(Road, &_300, xz, d);
        Pos[0] = xz[0];
        Pos[2] = xz[1];
        Dir = DAtan2f((double)d[0], (double)d[1]);
    }
    RefreshModel();                                               // +0x3c
    StoreInterpolationState();                                    // +0x16c
    SetOnBlockMap(true);
    SUnit::SetPosition(Pos[0], Pos[2], Bits(Dir), yrelBits);      // 0x5c1980
}

// PANZERS 0x5b0f00
// STrainUnit::CalcTowedUnitPosAndDir: on a road the carriage stands the
// hook + hole distance behind on the same road; off it, as SUnit 0x5b5c10
// from the carriage's position.
void STrainUnit::GhostFramesAddTop(float x, float y, float z, float dir, float dist, SIUnit* towed,
                                   float* outPos, float* outDir, float* outDist)
{
    SUnit* t = static_cast<SUnit*>(towed);
    if (t->Proto->ClassType != 10)
        Logger.g->Panic("STrainUnit::CalcTowedUnitPosAndDir - towed_unit->PUnit->ClassType != CLASSTYPE_TRAIN");
    static_cast<STrainUnit*>(t)->Road = Road;
    float hook2[2], hookW[2], hole2[2];
    bool front;
    HookAndHole(this, x, z, dir, towed, hook2, hookW, hole2, &front);
    if (Road >= 0) {
        float hookLen = (float)sqrt((double)(hook2[0] * hook2[0] + hook2[1] * hook2[1]));
        float holeLen = (float)sqrt((double)(hole2[0] * hole2[0] + hole2[1] * hole2[1]));
        float xz[2] = { 0.0f, 0.0f }, d[2] = { 0.0f, 0.0f };
        *outDist = dist - (hookLen + holeLen);
        RailGetPositionOnRoad(Road, outDist, xz, d);              // 0x5e98e0
        outPos[0] = xz[0];
        outPos[2] = xz[1];
        *outDir = DAtan2f((double)d[0], (double)d[1]);            // 0x78d07a
        return;
    }
    float holeLen = (float)sqrt((double)(hole2[0] * hole2[0] + hole2[1] * hole2[1]));
    TowBehindHook(hookW[0], y, hookW[1], dir, t->Pos, holeLen, front, outPos, outDir);
}

// PANZERS 0x5b1620
void STrainUnit::EC_Follow(int unit, int p2)
{
    (void)unit; (void)p2;
}

// PANZERS 0x5b16f0
// STrainUnit::MarkCurrentBlockMap: the "Block" node's cells (built from the
// model's current pose, the node hidden) count as occupied.
void STrainUnit::SetOnBlockMap(bool on)
{
    if (!Model)
        Logger.g->Panic("STrainUnit::MarkCurrentBlockMap - Model == NULL");
    OnBlockMap = on;                                              // +0x2ea
    if (on) {
        BlockBitmap_Free(BlockFootprint);
        BlockFootprint = nullptr;
        BlockFootprint = Model->BuildNodeBlockBitmap(4, "Block"); // +0xa8
        Model->SetNodeVisible(Model->FindNode("Block"), false);   // +0x40, +0x60
    }
    BlockMap_OccupyBitmap(BlockFootprint, on);                    // 0x5f45f0
    if (!on && BlockFootprint) {
        BlockBitmap_Free(BlockFootprint);
        BlockFootprint = nullptr;
    }
}

// PANZERS 0x5b1400
void STrainUnit::Slot_19C()
{
    BlockMap_TestOccupied(BlockFootprint);                        // 0x5d9cf0
}

// The "Block" bitmap of the model put at (x, z) / dir on the terrain slope
// (the train animation's model turns by -90 degrees).
static SBlockBitmap* BlockAt(SUnit* u, float x, float z, float dir)
{
    if (*(int*)((unsigned char*)u->Anim + 0x0c) == 8)            // animation +0x0c Type 8 (train)
        dir = (float)((double)dir - 1.5707963705062866);          // 0x7f5a38
    float h0 = Height((float)((double)x - 0.5), z);               // 0x7ea760
    float tx = h0 - Height((float)((double)x + 0.5), z);
    float h1 = Height(x, (float)((double)z - 0.5));
    float tz = h1 - Height(x, (float)((double)z + 0.5));
    float p[3] = { x, Height(x, z), z };
    return u->Model->BuildNodeBlockBitmapAt(4, "Block", p, dir, tx, tz);   // +0xa4
}

// PANZERS 0x5b17c0
void STrainUnit::MarkBlockMap(bool on, int p2, int p3, int p4, short p5)
{
    SBlockBitmap* bm = BlockAt(this, Flt(p2), Flt(p3), Flt(p4));
    if (bm)
        BlockMap_ApplyBitmap(g_World, bm, on, (unsigned)(unsigned short)p5);   // 0x5f4910
    BlockBitmap_Free(bm);
}

// PANZERS 0x5b1420
int STrainUnit::TestBlockMap(int p1, int p2, int p3, int p4, short p5)
{
    (void)p4;
    SBlockBitmap* bm = BlockAt(this, Flt(p1), Flt(p2), Flt(p3));
    bool r = BlockMap_TestBitmap(bm, (unsigned)(unsigned short)p5);   // 0x5dc6b0
    BlockBitmap_Free(bm);
    return r;
}

// PANZERS 0x5b1b70
// The unit size is the footprint's diagonal (4 cells a unit); a train takes
// no square on the block map.
void STrainUnit::SetUnitSize()
{
    if (!BlockFootprint)
        return;
    float w = (float)BlockFootprint->W * 0.25f;                   // 0x7f4538
    float h = (float)BlockFootprint->H * 0.25f;
    UnitSizeBlocks = 0;
    UnitSizeBlocks2 = 0;
    UnitSize = (float)sqrt((double)(w * w + h * h));
}

} // namespace pz
