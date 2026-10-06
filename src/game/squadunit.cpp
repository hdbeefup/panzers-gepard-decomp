// src/game/squadunit.cpp
// SPanzersSquadUnit and SPanzersSquadMemberUnit (0x5977e0..0x5a1300).
// OWNER: agent U. See squadunit.h.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "squadunit.h"
#include "gunner.h"
#include "idriver.h"
#include "unitanim.h"
#include "unitextern.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/iscene.h"
#include "pz/imodel.h"
#include "logger.h"
#include "doodad.h"
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
    _35c = true;
    _390 = true;
    _39c = -1;
}

// PANZERS 0x599110
SPanzersSquadUnit::~SPanzersSquadUnit()
{
    free(_384.Array);
    free(RelPos.Array);
}

// PANZERS 0x599c70
void SPanzersSquadUnit::Uninit()
{
    // HD: board +0x0c releases the squad's board elements (+0x380,
    // +0x360..+0x37c); the recompile does not create them.
    SUnit::Uninit();
}

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

// The formation switch of Init 0x59c470 / SetCurrentTarget 0x59f580.
void SPanzersSquadUnit::SetRelativePositions()
{
    SetMembersRadius();
    if (FormationType == 0)
        SetRelativeNormalPositions();
    else if (FormationType != 1 && FormationType != 2)
        Logger.g->Panic("SPanzersSquadUnit::SetRelativePositions - Unknown formation_type");
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
    double a = (double)RelPos.Array[i].Angle;
    out[0] = (float)sin(a) * RelPos.Array[i].Radius;          // 0x78d640
    out[1] = (float)cos(a) * RelPos.Array[i].Radius;          // 0x78d480
}

// The member offset loop of Place 0x5a1220 / SetPosition 0x59fe30 / Init:
// the relative position turned by the formation direction (+0x24c) into
// +0x210, the member direction into +0x238.
void SPanzersSquadUnit::PlaceMemberOffsets(int dirBits)
{
    for (int i = 0; i < Members.Size && i < 5; ++i) {
        float rel[2];
        GetSquadMemberRelativePosition(rel, i);
        double fd = (double)FormationDir;
        float c = (float)cos(fd);
        float s = (float)sin(fd);
        MemberOffset[i][0] = c * rel[0] + s * rel[1];
        MemberOffset[i][1] = c * rel[1] - s * rel[0];
        if (i >= RelPos.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SSquadRelPos", i);
        MemberDir[i] = RelPos.Array[i].HasDir ? RelPos.Array[i].Dir : dirBits;
    }
}

// PANZERS 0x59c470
void SPanzersSquadUnit::Init(SUnitDef* def)
{
    int n = P->MaxNumberOfUnits;
    // 0x546ca0 / 0x546af0: Members and _384 sized to n and cleared.
    Members.Size = n;
    if (Members.Max < n) {
        Members.Max = n;
        Members.Array = (SUnitMember*)realloc(Members.Array, n * sizeof(SUnitMember));
    }
    if (Members.Max)
        memset(Members.Array, 0, Members.Max * sizeof(SUnitMember));
    SetUnitSize();                                            // +0x1c4
    _384.Size = n;
    if (_384.Max < n) {
        _384.Max = n;
        _384.Array = (int*)realloc(_384.Array, n * sizeof(int));
    }
    if (_384.Max)
        memset(_384.Array, 0, _384.Max * sizeof(int));
    Unplaced = def->Stored;
    SetRelativePositions();
    int dirBits;
    memcpy(&dirBits, &def->Dir, 4);
    for (int i = 0; i < n; ++i) {
        float rel[2];
        GetSquadMemberRelativePosition(rel, i);
        double fd = (double)FormationDir;
        float c = (float)cos(fd);
        float s = (float)sin(fd);
        if (i < 5) {
            MemberOffset[i][0] = c * rel[0] + s * rel[1];
            MemberOffset[i][1] = c * rel[1] - s * rel[0];
        }
        float ox = c * rel[0] + s * rel[1];
        float oz = c * rel[1] - s * rel[0];
        int mdir = RelPos.Array[i].HasDir ? RelPos.Array[i].Dir : dirBits;
        if (i < 5)
            MemberDir[i] = mdir;
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
    // HD: the squad's board elements (name tag, rank icons, ...) here.
    _ec = def->GlobalState;                                   // param_1[0x3b]
    SUnit::Init(def);                                         // 0x5ba8e0
}

// PANZERS 0x59fab0
// Equipment slots (UNTD Slots[0..1]) mapped to the item and its level by the
// squad's rank (+0x88): grenade, molotov, mines, explosives, ... stored at
// +0x138/+0x13c and +0x144/+0x148.
void SPanzersSquadUnit::Hook20(int p1)
{
    (void)p1;
    STUB_LOG("SPanzersSquadUnit::Hook20 (0x59fab0) equipment slots");
}

// PANZERS 0x59fe30
void SPanzersSquadUnit::SetPosition(float x, float z, int dirBits, int yrelBits)
{
    for (int i = 0; i < RelPos.Size; ++i) {
        float rel[2];
        GetSquadMemberRelativePosition(rel, i);
        double fd = (double)FormationDir;
        float c = (float)cos(fd);
        float s = (float)sin(fd);
        float ox = c * rel[0] + s * rel[1];
        float oz = c * rel[1] - s * rel[0];
        int mdir = RelPos.Array[i].HasDir ? RelPos.Array[i].Dir : dirBits;
        if (i < 5) {
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
        float rel[2];
        GetSquadMemberRelativePosition(rel, i);
        double fd = (double)FormationDir;
        float c = (float)cos(fd);
        float s = (float)sin(fd);
        float ox = c * rel[0] + s * rel[1];
        float oz = c * rel[1] - s * rel[0];
        if (i >= RelPos.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SSquadRelPos", i);
        int mdir = RelPos.Array[i].HasDir ? RelPos.Array[i].Dir : dirBits;
        if (i < 5) {
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

// PANZERS 0x59f580
// Lifted: the target swap, the gunner hand-over and the member orders for
// position targets (+0xe4) and unit targets (+0xe8, with one seed draw per
// member), the driver refresh and the formation reset. The movement-group
// state request (0x56b090 / 0x580260 / 0x599450, L) and the turn test
// (0x59d130 / 0x5a09d0) are not lifted.
void SPanzersSquadUnit::SetCurrentTarget(STarget* target, int p2)
{
    int kind = tgt::I(target, tgt::kKind);
    if (kind == 1)
        STUB_LOG("SPanzersSquadUnit::SetCurrentTarget (0x59f580) 0x5bee10");
    SetTarget(&CurrentTarget, target);
    if ((kind == 2 || kind == 3) && MainGunner >= 0) {
        tgt::I(target, tgt::kP20) = 1;
        GetGunner(MainGunner)->SetTarget(CurrentTarget);      // gunner +0x18
        int type = tgt::I(CurrentTarget, tgt::kType);
        if (type == 2) {
            for (int i = 0; i < Members.Size; ++i)
                STUB_LOG("SPanzersSquadUnit::SetCurrentTarget (0x59f580) member +0xe4");
        } else if (type == 0) {
            for (int i = 0; i < Members.Size; ++i) {
                STUB_LOG("SPanzersSquadUnit::SetCurrentTarget (0x59f580) member +0xe8");
                int r = WorldRand();
                SUnit* m = WorldUnit(Members.Array[i].Unit);
                if (m->Gunners.Size < 1)
                    Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", 0);
                m->Gunners.Array[0]->_60 = (i != 0) * 2 - (int)((double)r * 3.0517578125e-05 * 5.0);   // DAT_007f4598 (not verified)
            }
        } else {
            Logger.g->Panic("SPanzersSquadUnit::SetCurrentTarget: Unknown target type");
        }
    } else {
        StopGunners();                                        // +0xec
    }
    if (_18c != -1) {
        if (g_World->Units.IsLive(_18c))
            WorldUnit(_18c)->_190 = false;
        _18c = -1;
    }
    if (ActiveDriver >= 0) {
        SIDriver* d = GetDriver(ActiveDriver);
        if (d)
            d->RefreshTarget((int)(size_t)CurrentTarget);     // +0x08
    }
    (void)p2;
    if (tgt::I(target, tgt::kType) == 4)
        return;
    // HD: 0x5a0fc0, the movement-group global state (0x56b090) and the
    // turn-to-target test; a new formation when 0x59d130 asks for one.
}

// PANZERS 0x59af20
void SPanzersSquadUnit::EC_Move(int xBits, int zBits, int p3, bool p4, int p5)
{
    bool flag = !(p3 == 0 && _39c != 0) ? true : false;
    (void)p4;
    SUnit::EC_Move(xBits, zBits, flag ? 1 : 0, false, p5);     // 0x5b8ea0
    // HD 0x5a0d30 (squad global state for the move).
    if (_39c == -1)
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

// PANZERS 0x59dda0 (not lifted)
void SPanzersSquadUnit::RefreshSquadFormation()
{
    STUB_LOG("SPanzersSquadUnit::RefreshSquadFormation (0x59dda0)");
    PZ_M2_TRACE("SPanzersSquadUnit::RefreshSquadFormation (0x59dda0)");
}

// ---------------------------------------------------------------------------
// SPanzersSquadMemberUnit

// PANZERS 0x5977e0
SPanzersSquadMemberUnit::SPanzersSquadMemberUnit(SPPanzersSquadMemberUnit* proto, int worldIndex)
    : SUnit(proto, worldIndex)
{
    P = proto;
    Board344 = Board348 = 0;                                  // board +0x08(4, ...): not created
    _34c = 0;
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

} // namespace pz
