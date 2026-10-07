// src/game/squadunit.cpp
// SPanzersSquadUnit and SPanzersSquadMemberUnit (0x5977e0..0x5a1300).
// OWNER: agent U (orders, placement: M2-I sub-agent SQ). See squadunit.h.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "squadunit.h"
#include "packets.h"
#include "unitanim.h"
#include "gunner.h"
#include "idriver.h"
#include "unitextern.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/iscene.h"
#include "pz/imodel.h"
#include "logger.h"
#include "doodad.h"
#include "drivermath.h"
#include "gamelogic.h"
#include "target.h"
#include "stub_log.h"

namespace pz {

static const double kPiD = 3.1415927410125732;       // DAT_007f4560
static const double kTwoPiD = 6.2831854820251465;    // DAT_007f4570

// ---------------------------------------------------------------------------
// SPanzersSquadUnit

// PANZERS 0x598ff0
SPanzersSquadUnit::SPanzersSquadUnit(SPPanzersSquadUnit* proto, int worldIndex)
    : SUnit(proto, worldIndex)
{
    memset(&P, 0, sizeof(SPanzersSquadUnit) - offsetof(SPanzersSquadUnit, P));
    P = proto;
    for (int i = 0; i < 8; ++i)                               // recompile: no board element before Init
        Board[i] = -1;                                        // (0 would be the board's root frame)
    _380 = -1;
    CanHeal = true;
    RestartOrders = true;
    RequestedState = -1;
}

// PANZERS 0x599110
SPanzersSquadUnit::~SPanzersSquadUnit()
{
    free(MemberOrderDelay.Array);
    free(RelPos.Array);
}

// SPanzersSquadUnit::Uninit 0x599c70: unitboard.cpp (M5-VX).

// PANZERS 0x59fda0
void SPanzersSquadUnit::SetMembersRadius()
{
    switch (Members.Size) {
    case 0:
        Logger.g->Panic("SPanzersSquadUnit::SetMembersRadius() - StoredMembers.GetSize() == 0");
        return;
    case 1:
        MemberRadiusMax = 0.225f;                             // 0x3e666666
        MemberRadiusMin = 0.0f;
        return;
    case 2:
        MemberRadiusMax = 0.475f;                             // 0x3ef33333
        MemberRadiusMin = 0.35f;                              // 0x3eb33333
        return;
    case 3:
        MemberRadiusMax = 0.7f;                               // 0x3f333333
        MemberRadiusMin = 0.425f;                             // 0x3ed9999a
        return;
    default:
        MemberRadiusMax = 0.95f;                              // 0x3f733333
        MemberRadiusMin = 0.475f;                             // 0x3ef33333
        return;
    }
}

// PANZERS 0x5a0740
// A ring: member i at angle 2*pi*i/n (+ pi/n for even n) plus a random
// jitter of +-pi/(4n), at a random radius between the two member radii. Two
// draws of the world seed per member (none for a single member).
void SPanzersSquadUnit::SetRelativeNormalPositions()
{
    int n = Members.Size;
    // 0x5995b0: RelPos.SetSize(n) and clear.
    if (RelPos.Size != 0 && !RelPos.Array)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "struct SSquadRelPos");
    RelPos.Size = n;
    if (RelPos.Max < n) {
        RelPos.Max = n;
        RelPos.Array = (SSquadRelPos*)realloc(RelPos.Array, n * sizeof(SSquadRelPos));
    }
    memset(RelPos.Array, 0, RelPos.Max * sizeof(SSquadRelPos));
    if (n == 0)
        Logger.g->Panic("SPanzersSquadUnit::SetRelativeNormalPositions() - StoredMembers.GetSize() == 0");
    if (n == 1) {
        RelPos.Array[0].Radius = 0.0f;
        RelPos.Array[0].Angle = 0.0f;
        return;
    }
    const float pi = 3.1415927f;                              // DAT_007f4584
    float evenOffset = (n % 2 == 0) ? pi / (float)n : 0.0f;
    float jitter = 1.5707964f / (float)n;                     // DAT_007f5a00
    for (int i = 0; i < n; ++i) {
        float rmax = MemberRadiusMax, rmin = MemberRadiusMin;
        int r1 = WorldRand();
        RelPos.Array[i].Radius = (float)((double)r1 * 3.0517578125e-05 * (double)(rmax - rmin) + (double)rmin);
        int r2 = WorldRand();
        double a = ((double)r2 * 3.0517578125e-05 * (double)jitter - (double)(jitter * 0.5f)) +
                   (double)(((float)(2 * i) * pi) / (float)Members.Size) + (double)evenOffset;
        if (a <= kPiD) {
            if (a < -kPiD)
                a += kTwoPiD;
        } else {
            a -= kTwoPiD;
        }
        RelPos.Array[i].Angle = (float)a;
    }
}

// PANZERS 0x5a09d0
// SetRelativePositions (also inline in Init 0x59c470 and SetCurrentTarget
// 0x59f580): new member radii and, for the normal formation, a new random
// ring.
void SPanzersSquadUnit::SetRelativePositions()
{
    SetMembersRadius();                                       // 0x59fda0
    if (FormationType == 0)
        SetRelativeNormalPositions();                         // 0x5a0740
    else if (FormationType != 1 && FormationType != 2)
        Logger.g->Panic("SPanzersSquadUnit::SetRelativePositions - Unknown formation_type");
}

// PANZERS 0x599190
SSquadRelPos* SPanzersSquadUnit::RelPosAt(int i)
{
    if (i < 0 || i >= RelPos.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SSquadRelPos", i);
    return &RelPos.Array[i];
}

// PANZERS 0x59bff0
void SPanzersSquadUnit::GetSquadMemberRelativePosition(float* out, int i)
{
    if (i < 0 || i > Members.Size - 1)
        Logger.g->Panic("SPanzersSquadUnit::GetSquadMemberRelativePosition");
    out[0] = 0.0f;
    out[1] = 0.0f;
    if (i >= RelPos.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SSquadRelPos", i);
    out[0] = (float)DSin((double)RelPos.Array[i].Angle) * RelPos.Array[i].Radius;   // 0x78d640
    out[1] = (float)DCos((double)RelPos.Array[i].Angle) * RelPos.Array[i].Radius;   // 0x78d480
}

// HD inline (0x59e0d0, 0x59fe30, 0x5a1220, 0x59c470): member i's offset,
// cos(+0x24c) * rel.x + sin(+0x24c) * rel.z and cos * rel.z - sin * rel.x,
// in float after each 0x78d480 / 0x78d640 result is rounded to float.
void SPanzersSquadUnit::GetFormationOffset(int i, float* ox, float* oz)
{
    float rel[2];
    GetSquadMemberRelativePosition(rel, i);                   // 0x59bff0
    float c = (float)DCos((double)FormationDir);              // 0x78d480
    float s = (float)DSin((double)FormationDir);              // 0x78d640
    *ox = c * rel[0] + s * rel[1];
    *oz = c * rel[1] - s * rel[0];
}

// PANZERS 0x59c470
void SPanzersSquadUnit::Init(SUnitDef* def)
{
    int n = P->MaxNumberOfUnits;
    // 0x546ca0 / 0x546af0: Members and MemberOrderDelay sized to n and cleared.
    Members.Size = n;
    if (Members.Max < n) {
        Members.Max = n;
        Members.Array = (SUnitMember*)realloc(Members.Array, n * sizeof(SUnitMember));
    }
    if (Members.Max)
        memset(Members.Array, 0, Members.Max * sizeof(SUnitMember));
    SetUnitSize();                                            // +0x1c4
    MemberOrderDelay.Size = n;
    if (MemberOrderDelay.Max < n) {
        MemberOrderDelay.Max = n;
        MemberOrderDelay.Array = (int*)realloc(MemberOrderDelay.Array, n * sizeof(int));
    }
    if (MemberOrderDelay.Max)
        memset(MemberOrderDelay.Array, 0, MemberOrderDelay.Max * sizeof(int));
    Unplaced = def->Stored;
    SetRelativePositions();                                   // inline 0x5a09d0
    int dirBits;
    memcpy(&dirBits, &def->Dir, 4);
    for (int i = 0; i < RelPos.Size; ++i) {
        float ox, oz;
        GetFormationOffset(i, &ox, &oz);
        if (i < 5) {                                          // HD writes +0x210 + 8 * i unchecked
            MemberOffset[i][0] = ox;
            MemberOffset[i][1] = oz;
        }
        int mdir = RelPosAt(i)->HasDir ? RelPosAt(i)->Dir : dirBits;
        if (i < 5)
            MemberDir[i] = mdir;
        if (i >= Members.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", i);
        Members.Array[i].Owner = WorldIndex;
        float pos[3] = { def->Pos[0] + ox, 0.0f, def->Pos[1] + oz };
        float md;
        memcpy(&md, &mdir, 4);
        int m = g_World->CreateUnit(def->Player, SStr(P->SquadMemberName), pos, md, 0, def->HP,
                                    WorldIndex, true, "");    // 0x5e3170
        Members.Array[i].Unit = m;
        if (m < 0) {
            Logger.g->Warning("SPanzersSquadUnit::Init - StoredMembers[i] < 0");
            Members.Size = 0;
            if (Members.Max > 0)
                memset(Members.Array, 0, Members.Max * sizeof(SUnitMember));
            return;
        }
        if (Unplaced)
            WorldUnit(m)->Unplace();                          // +0x4c
        WorldUnit(m)->SetGlobalState(def->GlobalState, 0);    // 0x5b7390
    }
    SetAIGroup(AIGroup);                                      // 0x5c0c10(+0x80)
    CreateBoardElements();                                    // 0x59c7f8..0x59c96a (unitboard.cpp)
    _ec = def->GlobalState;                                   // param_1[0x3b]
    SUnit::Init(def);                                         // 0x5ba8e0
    if (g_GameLogic)                                          // DAT_008f2078
        RefreshTargeting();                                   // +0x34
}

// PANZERS 0x59fab0
// Equipment slots (UNTD Slots[0..1], p1 = their address): the item (+0x138 /
// +0x144) when the prototype allows it (+0xb5..+0xbd) and its amount
// (+0x13c / +0x148) by the squad's rank (+0x88). Items 1 grenade, 2 molotov,
// 3 magnetic mine, 4 explosives, 5 ground tank mine, 6 binoculars, 7 boat,
// 8 mine detector (the last three carry no amount).
void SPanzersSquadUnit::Hook20(int p1)
{
    const int* slots = (const int*)(intptr_t)p1;
    for (int s = 0; s < 2; ++s) {
        int& item = s == 0 ? _138 : _144;                     // +0x138 + 0xc * s
        int& amount = s == 0 ? _13c : _148;                   // +0x13c + 0xc * s
        int it = slots[s];
        // Amount by rank 0/1, 2/3, 4 (HD jump tables 0x59fc98 / 0x59fcac).
        int lo = 0, mid = 0, hi = 0;
        bool allowed = false;
        switch (it) {
        case 0:
            item = 0;
            amount = 0;
            continue;
        case 1: allowed = P->SlotGrenade;        lo = 3; mid = 4; hi = 5; break;   // +0xb5
        case 2: allowed = P->SlotMolotov;        lo = 1; mid = 2; hi = 3; break;   // +0xb6
        case 3: allowed = P->SlotMagneticMine;   lo = 1; mid = 2; hi = 3; break;   // +0xb7
        case 4: allowed = P->SlotExplosives;     lo = 1; mid = 2; hi = 3; break;   // +0xb8
        case 5: allowed = P->SlotGroundTankMine; lo = 3; mid = 4; hi = 5; break;   // +0xb9
        case 6:                                               // +0xbd binoculars
        case 7:                                               // +0xba boat
        case 8:                                               // +0xbc mine detector
            if ((it == 6 && P->SlotBinoculars) || (it == 7 && P->SlotBoat) ||
                (it == 8 && P->SlotMineDetector)) {
                item = it;
                amount = 0;
            }
            continue;
        default:
            continue;
        }
        if (!allowed)
            continue;
        item = it;
        switch (GetRank()) {                                  // +0x88
        case 0: case 1: amount = lo; break;
        case 2: case 3: amount = mid; break;
        case 4: amount = hi; break;
        default: break;                                       // rank > 4: amount unchanged
        }
    }
}

// PANZERS 0x59fe30
void SPanzersSquadUnit::SetPosition(float x, float z, int dirBits, int yrelBits)
{
    for (int i = 0; i < RelPos.Size; ++i) {
        float ox, oz;
        GetFormationOffset(i, &ox, &oz);
        int mdir = RelPosAt(i)->HasDir ? RelPosAt(i)->Dir : dirBits;
        if (i < 5) {                                          // HD writes +0x210 / +0x238 unchecked
            MemberOffset[i][0] = ox;
            MemberOffset[i][1] = oz;
            MemberDir[i] = mdir;
        }
        if (i >= Members.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", i);
        WorldUnit(Members.Array[i].Unit)->SetPosition(ox + x, oz + z, mdir, 0);   // +0x24
    }
    SUnit::SetPosition(x, z, dirBits, yrelBits);              // 0x5c1980
}

// PANZERS 0x59c1e0
void SPanzersSquadUnit::Unplace()
{
    if (Unplaced)
        return;
    SUnit::Unplace();                                         // 0x5ba850
    for (int i = 0; i < Members.Size; ++i)
        WorldUnit(Members.Array[i].Unit)->Unplace();          // +0x4c
    SetOnBlockMap(false);                                     // +0x198
}

// PANZERS 0x5a1220
void SPanzersSquadUnit::Place(float x, float z, float dir)
{
    if (!Unplaced)
        return;
    SetUnitSize();                                            // +0x1c4
    int dirBits;
    memcpy(&dirBits, &dir, 4);
    for (int i = 0; i < Members.Size; ++i) {
        float ox, oz;
        GetFormationOffset(i, &ox, &oz);
        int mdir = RelPosAt(i)->HasDir ? RelPosAt(i)->Dir : dirBits;
        if (i < 5) {                                          // HD writes +0x210 / +0x238 unchecked
            MemberOffset[i][0] = ox;
            MemberOffset[i][1] = oz;
            MemberDir[i] = mdir;
        }
        float md;
        memcpy(&md, &mdir, 4);
        WorldUnit(Members.Array[i].Unit)->Place(ox + x, oz + z, md);   // +0x50
    }
    SUnit::Place(x, z, dir);                                  // 0x5c5160
    SetOnBlockMap(true);                                      // +0x198
}

// PANZERS 0x580260
// SGameLogic::SetMovementGroupSquadsGlobalState (HD panic text says
// SetMovementGroupMoveSpeed): the unit's group +0x14 = state, then the
// slowest speed is recomputed (0x5824b0).
static void SetMovementGroupSquadsGlobalState(SUnit* u, int state)
{
    SGameLogic* gl = g_GameLogic;
    int g = u->_254;                                          // unit +0x254 movement group
    if (g < 0 || g >= gl->MovementGroupSize || gl->MovementGroups[g].Next != kHeapLive)
        Logger.g->Panic("SGameLogic::SetMovementGroupMoveSpeed: Invalid movement group %d", g);
    gl->MovementGroups[g].SquadsGlobalState = state;
    gl->UpdateMovementGroupSlowestMoveSpeed(g);               // 0x5824b0
}

// PANZERS 0x59f580
void SPanzersSquadUnit::SetCurrentTarget(STarget* target, int p2)
{
    if (tgt::I(target, tgt::kKind) == 1)
        CopyDriverWayPoints();                                // 0x5bee10
    SetTarget(&CurrentTarget, target);                        // 0x5bdef0 / 0x5b5a30
    int kind = tgt::I(target, tgt::kKind);
    if ((kind == 2 || kind == 3) && MainGunner >= 0) {
        tgt::I(target, tgt::kP20) = 1;
        GetGunner(MainGunner)->SetTarget(CurrentTarget);      // gunner +0x18
        int type = tgt::I(CurrentTarget, tgt::kType);
        if (type == 2) {
            for (int i = 0; i < Members.Size; ++i) {
                SPanzersSquadMemberUnit* m = static_cast<SPanzersSquadMemberUnit*>(WorldUnit(Members.Array[i].Unit));
                int xb, zb;
                memcpy(&xb, &tgt::F(CurrentTarget, tgt::kPos), 4);
                memcpy(&zb, &tgt::F(CurrentTarget, tgt::kPos + 8), 4);
                m->EC_AttackPos(xb, zb, p2);                  // member +0xe4 (0x597fa0)
            }
        } else if (type == 0) {
            for (int i = 0; i < Members.Size; ++i) {
                SPanzersSquadMemberUnit* m = static_cast<SPanzersSquadMemberUnit*>(WorldUnit(Members.Array[i].Unit));
                m->EC_Attack(tgt::I(CurrentTarget, tgt::kUnit), p2);   // member +0xe8 (0x597e80)
                int r = WorldRand();
                SUnit* m2 = WorldUnit(Members.Array[i].Unit);
                if (m2->Gunners.Size < 1)
                    Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
                // DAT_007f4598 = -1/32768: the truncated product is always 0.
                m2->Gunners.Array[0]->_60 = (i != 0) * 2 - (int)((double)r * -3.0517578125e-05);
            }
        } else {
            Logger.g->Panic("SPanzersSquadUnit::SetCurrentTarget: Unknown target type");
        }
    } else {
        StopGunners();                                        // +0xec
    }
    if (_18c != -1 && g_World->Units.IsLive(_18c)) {
        WorldUnit(_18c)->_190 = false;
        _18c = -1;
    }
    ClearMemberMarkers();                                     // 0x59d2f0
    if (ActiveDriver >= 0)
        GetDriver(ActiveDriver)->SetTarget(CurrentTarget);    // driver +0x08
    if (tgt::I(target, tgt::kType) == 4)
        return;
    UpdateMoveGlobalState();                                  // 0x5a0fc0
    if (_254 > -1) {                                          // in a movement group
        int s = g_GameLogic->GetMovementGroupSquadsGlobalState(WorldIndex);   // 0x56b090
        if ((char)p2 != 0) {
            if (s != 0)
                SetMovementGroupSquadsGlobalState(this, 0);   // 0x580260
            RequestedState = 0;
            SetSquadBehavior(0);                              // 0x599450
        } else if (s != GlobalState) {
            RequestedState = s;
            SetSquadBehavior(s);                              // 0x599450
        }
    } else if ((char)p2 != 0) {
        RequestedState = 0;
        SetSquadBehavior(0);                                  // 0x599450
    }
    if (MembersTooClose()) {                                  // 0x59d130
        SetRelativePositions();                               // inline 0x5a09d0
        return;
    }
    if (Wrecked)
        return;
    int type = tgt::I(target, tgt::kType);
    if (type != 2 && type != 3)
        return;
    // Turned more than pi/4 (and less than 0.9 pi) towards the target: a
    // new formation (0x5a09d0).
    float dx = tgt::F(target, tgt::kPos) - Pos[0];
    float dz = tgt::F(target, tgt::kPos + 8) - Pos[2];
    float a = (float)DAtan2((double)dx, (double)dz);          // 0x78d07a, fstp qword
    double d = fabs((double)a - (double)Dir);
    double w = d > kHdPi ? kHdTwoPi - d : d;
    if (w > 0.7853981852531433) {                             // DAT_007f7f50
        double w2 = d > kHdPi ? kHdTwoPi - d : d;
        if (2.8274333477020264 > w2)                          // DAT_007fa400
            SetRelativePositions();                           // 0x5a09d0
    }
}

// PANZERS 0x59af20
void SPanzersSquadUnit::EC_Move(int xBits, int zBits, int p3, bool p4, int p5)
{
    int flag = ((char)p3 != 0 || RequestedState == 0) ? 1 : 0;
    SUnit::EC_Move(xBits, zBits, flag, p4, p5);               // 0x5b8ea0
    UpdateMovingMembersRelPos();                              // 0x5a0d30
    if (RequestedState == -1)
        RestoreBehavior();                                    // +0x1ac
}

// PANZERS 0x59d460
void SPanzersSquadUnit::SetOnBlockMap(bool on)
{
    OnBlockMap = on;
    PzBlockMapSetUnit(Pos[0], Pos[2], UnitSizeBlocks, on);    // 0x5f4430
}

// PANZERS 0x5a0f30
void SPanzersSquadUnit::SetUnitSize()
{
    switch (Members.Size) {
    case 0:
        if (Unplaced || Wrecked)
            return;
        Logger.g->Panic("SPanzersSquadUnit::SetUnitSize() - StoredMembers.GetSize() == 0");
        return;
    case 1:
        UnitSize = 0.5f;
        break;
    case 2:
        UnitSize = 1.0f;
        break;
    case 3:
        UnitSize = 1.5f;
        break;
    default:
        UnitSize = 2.0f;
    }
    UnitSizeBlocks2 = 2;
    UnitSizeBlocks = (int)(UnitSize * 4.0f);
}

// PANZERS 0x59b4a0
void SPanzersSquadUnit::GetCenterPosition(float* out)
{
    if (Members.Size == 0) {
        out[0] = Pos[0];
        out[1] = Pos[1];
        out[2] = Pos[2];
        return;
    }
    float sx = 0.0f, sy = 0.0f, sz = 0.0f;
    for (int i = 0; i < Members.Size; ++i) {
        SUnit* m = WorldUnit(Members.Array[i].Unit);
        if (!m->Model) {
            out[0] = Pos[0];
            out[1] = Pos[1];
            out[2] = Pos[2];
            return;
        }
        float p[3] = { 0.0f, 0.0f, 0.0f };
        m->Model->GetRenderPosition(p);                       // +0x14
        sx += p[0];
        sy += p[1];
        sz += p[2];
    }
    float k = (float)(1.0 / (double)Members.Size);
    out[0] = k * sx;
    out[1] = k * sy;
    out[2] = k * sz;
}

// ---------------------------------------------------------------------------
// SPanzersSquadMemberUnit

// PANZERS 0x5977e0
SPanzersSquadMemberUnit::SPanzersSquadMemberUnit(SPPanzersSquadMemberUnit* proto, int worldIndex)
    : SUnit(proto, worldIndex)
{
    P = proto;
    CreateBoardElements();                                    // board +0x08(4, ...) twice (unitboard.cpp)
    ParachuteTicks = 0;
    Parachute = nullptr;
    if (proto->ParachuteProto >= 0) {
        Parachute = g_Scene->CreateModelFromPrototype(proto->ParachuteProto, 1);   // scene +0x58
        if (Parachute) {
            Parachute->SetFlags(7);
            Parachute->SetVisible(false, false);
        }
    }
}

// PANZERS 0x597940
SPanzersSquadMemberUnit::~SPanzersSquadMemberUnit()
{
    if (Parachute) {
        Parachute->Release();
        Parachute = nullptr;
    }
    ReleaseBoardElements();                                   // board +0x0c(+0x348)
}

// PANZERS 0x598940
void SPanzersSquadMemberUnit::InitNew(int player, const float* pos, float dir, int p4, float hp)
{
    SUnit::InitNew(player, pos, dir, p4, hp);                 // 0x5bace0
    if (g_GameLogic)
        RefreshTargeting();                                   // +0x34
}

// PANZERS 0x5989b0
void SPanzersSquadMemberUnit::SetOnBlockMap(bool on)
{
    (void)on;
}

// PANZERS 0x59af90
// Once neither the squad nor any member moves (+0xc8 <= 0), every member goes
// back to its global-state stand pose (0x5cae80(1)) and restarts its relax
// timer (SWalkerAnimation 0x5cb0a0).
void SPanzersSquadUnit::Slot_148(int player)
{
    (void)player;
    if (!(Speed <= 0.0f))
        return;
    for (int i = 0; i < Members.Size; ++i)
        if (0.0f < WorldUnit(Members.Array[i].Unit)->Speed)
            return;
    for (int i = 0; i < Members.Size; ++i) {
        static_cast<SUnitAnimation*>(WorldUnit(Members.Array[i].Unit)->Anim)->PlayGlobalStand(true);
        static_cast<SWalkerAnimation*>(WorldUnit(Members.Array[i].Unit)->Anim)->ResetRelax();
    }
}

void SPanzersSquadUnit::OnMemberDied(int unit)
{
    RemoveMember(unit);                                       // 0x59d4e0 (the +0x1b4 override)
}

// PANZERS 0x598280
// A squad member dies: not while its squad or itself is invulnerable or the
// local player's units are protected (SGameLogic +0x2da); the squad drops it
// (+0x1b4), then SUnit::EC_Die.
void SPanzersSquadMemberUnit::EC_Die()
{
    if (Parent > -1 && WorldUnit(Parent)->Invulnerable)
        return;
    if (Invulnerable)
        return;
    if (*((const unsigned char*)g_GameLogic + 0x2da) && !g_GameLogic->IsPaused() && g_World->LocalPlayer == Player)
        return;
    if (g_World->Units.IsLive(Parent))
        WorldUnit(Parent)->OnMemberDied(WorldIndex);          // +0x1b4
    SUnit::EC_Die();                                          // 0x5b8b10
}

// PANZERS 0x59c0d0 (SPanzersSquadUnit +0xa8)
// A building 10; attack (3) an enemy; enter (4); heal (9) wounded members of
// a squad (medics); else 2.
int SPanzersSquadUnit::ActionOn(int target)
{
    if (!IsTargetable(target, true) || target == WorldIndex)
        return 0;
    if (!g_World->Units.IsLive(target))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", target);
    SUnit* t = g_World->Units.Array[target].Unit;
    if (t->Proto->ClassType == 9)
        return 10;
    if (GetUnitRelation(target, Player) == -1)
        return 3;
    if (t->CanStoreUnit(WorldIndex))
        return 4;
    if (((const unsigned char*)P)[0xde] && t->HasWoundedMember())
        return 9;
    return 2;
}

// PANZERS 0x59aea0
// A squad cannot reverse: the move (+0xac, the squad's 0x59af20), the members'
// relative positions (0x5a0d30) and +0x1ac.
void SPanzersSquadUnit::EC_MoveReverse(int xBits, int zBits, int p3, bool p4, int p5)
{
    EC_Move(xBits, zBits, p3, p4, p5);                        // +0xac
    UpdateMovingMembersRelPos();                              // 0x5a0d30
    RestoreBehavior();                                        // +0x1ac
}

// PANZERS 0x59a730
// Unless invulnerable (+0x111): every member is destroyed first (member 0
// until none is left; each removes itself), then the squad (0x5b88d0).
void SPanzersSquadUnit::EC_Destroy()
{
    if (Invulnerable)                                         // +0x111
        return;
    while (Members.Size > 0) {
        int m = Members.Array[0].Unit;
        if (!g_World->Units.IsLive(m))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", m);
        g_World->Units.Array[m].Unit->EC_Destroy();           // +0x128
    }
    SUnit::EC_Destroy();                                      // 0x5b88d0
}

} // namespace pz
