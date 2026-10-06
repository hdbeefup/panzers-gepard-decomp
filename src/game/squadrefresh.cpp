// src/game/squadrefresh.cpp
// SPanzersSquadUnit / SPanzersSquadMemberUnit per-tick slots (0x5977e0..0x5a1300).
// OWNER: M2-I sub-agent SQ. See squadunit.h.
//
// Exact maths: the HD bodies are SSE2 scalar code; float and double steps
// follow the disassembly, trig goes through drivermath.h (0x78d640 /
// 0x78d480 / 0x78d07a), and every draw of the world seed (World+0x7518) is
// in HD order.

#include <math.h>
#include <string.h>
#include "squadunit.h"
#include "driverunit.h"
#include "drivermath.h"
#include "gamelogic.h"
#include "gunner.h"
#include "idriver.h"
#include "target.h"
#include "iunitanim.h"
#include "unitextern.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/imodel.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

// ---------------------------------------------------------------------------
// Helpers (HD SUnit / SWorld functions the squads call; static here)

// SDArray<SUnitMember>::operator[] + SHeapTRB::operator[] (inline in HD).
static SUnit* MemberAt(SUnit* squad, int i)
{
    if (i < 0 || i >= squad->Members.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", i);
    return WorldUnit(squad->Members.Array[i].Unit);
}

// The units in sight, SUnit +0x1b4 (SDArray::operator[] 0x5463d0).
static int SightUnitAt(SUnit* u, int i)
{
    if (i < 0 || i >= u->SightUnits.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SNearUnit", i);
    return u->SightUnits.Array[i].Unit;
}

// PANZERS 0x5c2270
// SUnit::SlotsDecreaseAmount: one use of equipment item `item` from the
// first (+0x138 / +0x13c) or the second (+0x144 / +0x148) slot.
static void UnitSlotsDecreaseAmount(SUnit* u, int item)
{
    if (u->_138 == item) {
        if (u->_13c > 0) {
            if (--u->_13c == 0)
                u->_138 = 0;
            return;
        }
        Logger.g->Warning("SUnit::SlotsDecreaseAmount() Nem fogyaszthato (%d)", item);
        return;
    }
    if (u->_144 != item)
        return;
    if (u->_148 > 0) {
        if (--u->_148 == 0)
            u->_144 = 0;
        return;
    }
    Logger.g->Warning("SUnit::SlotsDecreaseAmount() Nem fogyaszthato (%d)", item);
}

// PANZERS 0x549ab0
// SWorld: players a and b are allies (same alliance World+0x17c, or the
// same player without one).
static bool WorldIsAlly(int a, int b)
{
    int ta = *(int*)(g_World->Players[a] + 0xc);              // World+0x17c + a * 0x48
    if (ta != 0)
        return ta == *(int*)(g_World->Players[b] + 0xc);
    return a == b;
}

// PANZERS 0x5c1030
// SUnit: order to heal `unit` (target kind 8, type 0 = unit).
static void UnitOrderHeal(SUnit* u, int unit)
{
    STarget* t = STarget::Create(8);                          // new 0x38, ctor inline
    t->Type = kTargetUnit;
    t->Unit = unit;
    u->SetCurrentTarget(t, 0);                                // +0xa0
}

// PANZERS 0x5c18e0
// STarget: position target with a final direction (type 3), y on the
// ground.
static void TargetSetGroundPosDir(STarget* t, const float* xz, int dirBits)
{
    t->Type = kTargetPosDir;
    float x = xz[0];
    float y = g_World->GetTerrainHeight(x, xz[1]);            // 0x5e7730
    t->Pos[0] = x;
    t->Pos[1] = y;
    t->Pos[2] = xz[1];
    memcpy(&t->Dir, &dirBits, 4);
}

static double WrapAbs(double d)                                // |a - b| folded to [0, pi]
{
    return d > kHdPi ? kHdTwoPi - d : d;                      // 0x7f4560 / 0x7f4570
}

// ---------------------------------------------------------------------------
// SPanzersSquadUnit

// PANZERS 0x599450
// The squad and every member take the new behavior (global state).
void SPanzersSquadUnit::SetSquadBehavior(int behavior)
{
    if (GlobalState == behavior)
        return;
    SUnit::SetBehavior(behavior);                             // 0x5b8960
    UpdateMovingMembersRelPos();                              // 0x5a0d30
    for (int i = 0; i < Members.Size; ++i)
        MemberAt(this, i)->SetBehavior(behavior);             // member +0x12c
}

// PANZERS 0x59a910
void SPanzersSquadUnit::SetBehavior(int behavior)
{
    if (behavior != GlobalState) {
        DriverDropLocalPath();                                // 0x5b5ba0
        if (g_GameLogic)
            g_GameLogic->RemoveUnitFromMovementGroup(WorldIndex);   // 0x579510
    }
    SetSquadBehavior(behavior);                               // 0x599450
    _ec = behavior;
    RequestedState = -1;
}

// PANZERS 0x5a0a10
// The squad's +0x9c override: +0x112 on the squad and every member.
void SPanzersSquadUnit::SetFlag112(bool on)
{
    _112 = on;
    for (int i = 0; i < Members.Size; ++i)
        MemberAt(this, i)->_112 = on;
}

// PANZERS 0x599430
void SPanzersSquadUnit::RestoreBehavior()
{
    if (_ec != GlobalState)
        SetBehavior(_ec);                                     // +0x12c
}

// PANZERS 0x59d670
bool SPanzersSquadUnit::HasWoundedMember()
{
    for (int i = 0; i < Members.Size; ++i)
        if (MemberAt(this, i)->HP < 1.0f)                     // DAT_007f1b58
            return true;
    return false;
}

// PANZERS 0x59b120
void SPanzersSquadUnit::StopGunners()
{
    SUnit::StopGunners();                                     // 0x5b9110
    if (MainGunner < 0)
        return;
    for (int i = 0; i < Members.Size; ++i)
        MemberAt(this, i)->StopGunners();                     // member +0xec
}

// PANZERS 0x59bd10
// The first member's first weapon range (gunner prototype +0x38).
float SPanzersSquadUnit::GetMaxRange(int weapon)
{
    (void)weapon;
    if (Members.Size == 0)
        return 0.0f;
    SUnit* m = MemberAt(this, 0);
    if (m->Gunners.Size < 1)
        return 0.0f;
    return m->GetGunner(0)->GetPGunner()->MaxRange;           // gunner +0x2c
}

// PANZERS 0x59be20
float SPanzersSquadUnit::GetMinRange(int weapon)
{
    (void)weapon;
    if (Members.Size == 0)
        return 0.0f;
    SUnit* m = MemberAt(this, 0);
    if (m->Gunners.Size < 1)
        return 0.0f;
    return m->GetGunner(0)->GetPGunner()->MinRange;           // gunner +0x2c, +0x34
}

// PANZERS 0x59bb80
// The hearing range of the squad's rank (unitvariables.ini HearingRange_*).
float SPanzersSquadUnit::GetExtra188()
{
    const SUnitRegistry* r = g_UnitRegistry;                  // DAT_00929a4c
    switch (GetRank()) {                                      // +0x88
    case 0:
        return (float)r->HearingRange[0];
    case 1:
        return (float)r->HearingRange[1];
    case 2:
        return (float)r->HearingRange[2];
    case 3:
        return (float)r->HearingRange[3];
    default:
        return (float)r->HearingRange[4];
    }
}

// PANZERS 0x59d4b0
void SPanzersSquadUnit::MarkBlockMap(bool on, int p2, int p3, int p4, short p5)
{
    (void)p4;
    PzBlockMapMark(p2, p3, UnitSizeBlocks, on, p5);           // 0x5f4720
}

// PANZERS 0x599580
int SPanzersSquadUnit::TestBlockMapPath(int p1, int p2, int p3, int p4, short p5, int p6, int p7)
{
    (void)p3;
    return PzBlockMapTestPath(p1, p2, p4, p5, p6, p7) != 0;   // 0x5d9eb0
}

// PANZERS 0x599550
int SPanzersSquadUnit::TestBlockMap(int p1, int p2, int p3, int p4, short p5)
{
    (void)p3;
    return PzBlockMapTest(p1, p2, p4, p5) != 0;               // 0x5d9e00
}

// PANZERS 0x59bc10
// GetMaxMoveSpeed: the fastest member speed of the animation state.
float SPanzersSquadUnit::GetMoveSpeed(int state)
{
    if (state == -1)
        Logger.g->Panic("SPanzersSquadUnit::GetMaxMoveSpeed() - global_state == -1");
    float best = 0.0f;
    for (int i = 0; i < Members.Size; ++i) {
        float s = SseF(MemberAt(this, i)->Anim->GetStateMoveSpeed(state));   // anim +0x18
        if (s > best)
            best = s;
    }
    return best;
}

// PANZERS 0x59b990
// The squad's walking speed: the fastest member speed of each member's own
// animation state, 0 for a member changing state.
float SPanzersSquadUnit::GetSquadMoveSpeed()
{
    float best = 0.0f;
    for (int i = 0; i < Members.Size; ++i) {
        SUnit* m = MemberAt(this, i);
        float s = SseF(m->Anim->GetStateMoveSpeed(m->GlobalState));   // anim +0x18, fstp dword
        if (m->_f0 || m->StateChanging)
            s = 0.0f;
        if (s > best)
            best = s;
    }
    return best;
}

// PANZERS 0x59ede0
void SPanzersSquadUnit::ServerRefreshMedic(float dt)
{
    for (int i = 0; i < Members.Size; ++i) {
        if (MemberAt(this, i)->Speed > 0.0f) {
            float spin;
            memcpy(&spin, &MemberAt(this, i)->_d0, 4);        // +0xd0 spin speed
            if (spin > 0.0f) {
                MedicDelay = 10;
                return;
            }
        }
    }
    SUnit::ServerRefreshMedic(dt);                            // 0x5bfa50
}

// PANZERS 0x599620
// Per frame: the squad's board elements (name tag, rank icons, health and
// equipment bars) placed over GetCenterPosition (+0x1c8) through the
// viewport projection (+0x3c). The recompile creates no board elements, so
// nothing is left; HD does not call SUnit::UpdateVisuals here.
void SPanzersSquadUnit::UpdateVisuals(SIViewport* vp)
{
    (void)vp;
}

// PANZERS 0x59d130
// Two members closer than 0.25 m.
bool SPanzersSquadUnit::MembersTooClose()
{
    int last = Members.Size - 1;
    for (int i = 0; i < last; ++i) {
        SUnit* a = MemberAt(this, i);
        for (int j = i + 1; j < Members.Size; ++j) {
            SUnit* b = MemberAt(this, j);
            float dx = a->Pos[0] - b->Pos[0];
            float dy = a->Pos[1] - b->Pos[1];
            float dz = a->Pos[2] - b->Pos[2];
            if (0.0625f > dx * dx + dy * dy + dz * dz)        // DAT_007f59a4
                return true;
        }
    }
    return false;
}

// PANZERS 0x59d2f0
// Drops the members' world markers (member +0x134, set by member +0x104
// 0x598670 for the delayed action 1).
void SPanzersSquadUnit::ClearMemberMarkers()
{
    if (_130 == 0)
        return;
    _130 = 0;
    for (int i = 0; i < Members.Size; ++i) {
        SUnit* m = MemberAt(this, i);
        STUB_LOG("SWorld 0x5f7c00 (remove a member marker), called by SPanzersSquadUnit 0x59d2f0");
        m->_134 = -1;
        MemberAt(this, i)->_130 = 0;
    }
}

// PANZERS 0x5a0ac0
// SetSquadMemberRelativePosition: a member that turned away keeps its
// direction from the squad (angle) and its distance (radius, clamped to the
// member radii, +1 m allowed ahead).
void SPanzersSquadUnit::SetSquadMemberRelativePosition(int i)
{
    if (i < 0 || i > Members.Size - 1)
        Logger.g->Panic("SPanzersSquadUnit::SetSquadMemberRelativePosition");
    SUnit* m = MemberAt(this, i);
    float dx = m->Pos[0] - Pos[0];
    float dz = m->Pos[2] - Pos[2];
    double ad = DAtan2((double)dx, (double)dz);               // 0x78d07a, fstp qword
    double d3 = fabs((double)(float)ad - (double)Dir);
    double w = WrapAbs(d3);
    double w1 = WrapAbs(fabs((double)RelPosAt(i)->Angle - (double)Dir));
    if (!(w1 > w))
        return;
    float rmax = MemberRadiusMax;
    if (1.5707963705062866 > WrapAbs(d3))                     // DAT_007f5a38
        rmax = rmax + 1.0f;                                   // DAT_007f1b58
    float r = (float)sqrt((double)(dz * dz + dx * dx));       // 0x78d090
    float rmin = MemberRadiusMin;
    float hi = r > rmin ? r : rmin;
    float res;
    if (rmax > hi)
        res = r > rmin ? r : rmin;
    else
        res = rmax;
    RelPosAt(i)->Radius = res;
    RelPosAt(i)->Angle = (float)ad;
}

// PANZERS 0x5a0d30
void SPanzersSquadUnit::UpdateMovingMembersRelPos()
{
    for (int i = 0; i < Members.Size; ++i)
        if (MemberAt(this, i)->Speed != 0.0f)
            SetSquadMemberRelativePosition(i);                // 0x5a0ac0
}

// PANZERS 0x5a0fc0
// While the squad follows a path (global state 3) it drops the state when
// the path ends or the current order is not the primary one, and takes it
// back when both hold again.
void SPanzersSquadUnit::UpdateMoveGlobalState()
{
    if (!PrimaryTarget || !CurrentTarget)
        return;
    if (GlobalState == 3) {
        if (!PrimaryTarget->HasPathAhead() || CurrentTarget != PrimaryTarget) {   // 0x5bb630
            SetGlobalState(0, 0);                             // 0x5b7390
            for (int i = 0; i < Members.Size; ++i)
                MemberAt(this, i)->SetGlobalState(0, 0);
            MoveStateDropped = true;
            return;
        }
    }
    if (MoveStateDropped && CurrentTarget == PrimaryTarget && PrimaryTarget->HasPathAhead()) {
        SetGlobalState(3, 0);
        for (int i = 0; i < Members.Size; ++i)
            MemberAt(this, i)->SetGlobalState(3, 0);
        MoveStateDropped = false;
    }
}

// PANZERS 0x5a0060
// Squad driver (every 2nd tick while walking): one random member that walks
// in the squad's direction gets its angle jittered by +-0.01 rad (kept when
// it stays within jitter/2 of its ring slot), or a new random angle.
void SPanzersSquadUnit::StepMemberAngle(float dir)
{
    if (Members.Size == 1)
        return;
    int r = WorldRand();                                      // 0x5a009c
    int idx = (int)((double)r * 3.0517578125e-05 * (double)Members.Size);   // DAT_007f4540
    double d = fabs((double)MemberAt(this, idx)->Dir - (double)dir);
    if (WrapAbs(d) >= 0.19634954631328583)                    // DAT_007f5a08
        return;
    if (MemberAt(this, idx)->Speed == 0.0f)
        return;
    int n = Members.Size;
    float even = 0.0f;
    if (n % 2 == 0)
        even = 3.1415927f / (float)n;                         // DAT_007f4584
    float jitter = 18.849556f / (float)n;                     // DAT_007fa40c
    int size = Members.Size;
    if ((float)fabs((double)RelPosAt(idx)->Angle) > 0.0f) {
        float saved = RelPosAt(idx)->Angle;
        int r2 = WorldRand();                                 // 0x5a026b
        double delta = (double)r2 * 3.0517578125e-05 * 0.02 - 0.01;   // DAT_007f59e8 / DAT_007f59c8
        RelPosAt(idx)->Angle = (float)DWrapAdd((double)RelPosAt(idx)->Angle, delta);   // 0x550600
        float slot = ((float)(2 * idx) * 3.1415927f) / (float)size;
        float ideal = (float)DWrapAdd((double)even, (double)slot);
        double off = DWrapSub((double)RelPosAt(idx)->Angle, (double)ideal);   // 0x55c1e0
        if (fabs(off) > (double)(jitter * 0.5f))              // DAT_007f453c
            RelPosAt(idx)->Angle = saved;
    } else {
        double rd = DRandDouble((double)jitter);              // 0x559740
        float slot = ((float)(2 * idx) * 3.1415927f) / (float)size;
        double t = rd - (double)(jitter * 0.5f) + (double)slot;
        RelPosAt(idx)->Angle = (float)DWrapAdd((double)even, t);
    }
}

// PANZERS 0x5a0450
// Squad driver (every 2nd tick while walking): one random member that walks
// in the squad's direction steps its radius by -0.02 or +0.02 (clamped to
// the member radii), or gets a new random radius.
void SPanzersSquadUnit::StepMemberRadius(float dir)
{
    if (Members.Size == 1)
        return;
    int r = WorldRand();                                      // 0x5a048c
    int idx = (int)((double)r * 3.0517578125e-05 * (double)Members.Size);
    double d = fabs((double)MemberAt(this, idx)->Dir - (double)dir);
    if (WrapAbs(d) >= 0.19634954631328583)                    // DAT_007f5a08
        return;
    if (MemberAt(this, idx)->Speed == 0.0f)
        return;
    if (RelPosAt(idx)->Radius > 0.0f) {
        double old = (double)RelPosAt(idx)->Radius;
        int k = HdRandInt(2);                                 // 0x555a00
        RelPosAt(idx)->Radius = (float)((double)k * 0.04 + old - 0.02);   // DAT_007fa3f8 / DAT_007f59e8
        if (RelPosAt(idx)->Radius > MemberRadiusMax)
            RelPosAt(idx)->Radius = MemberRadiusMax;
        if (MemberRadiusMin > RelPosAt(idx)->Radius)
            RelPosAt(idx)->Radius = MemberRadiusMin;
    } else {
        double rd = DRandDouble((double)(MemberRadiusMax - MemberRadiusMin));   // 0x559740
        // HD adds with x87 fadd at 24-bit precision; the double sum is
        // exact here (a 39-bit product plus a float), so one rounding.
        RelPosAt(idx)->Radius = (float)(rd + (double)MemberRadiusMin);
    }
}

// PANZERS 0x59db00
void SPanzersSquadUnit::RefreshTargeting()
{
    if (IsWaitingAfterStuck())                                // 0x5bb400
        return;
    if (g_GameLogic && g_GameLogic->IsPaused() && !_1d4)      // 0x56e150
        return;
    if (*(int*)(g_World->Players[Player] + 8) == 2)           // World+0x178 + player * 0x48
        return;
    if (IsHiddenInBlockMap())                                 // 0x5bb5c0
        return;
    float range = GetMaxRange(MainGunner);                    // +0x17c
    FillNearUnits(range, P->_ec);                             // 0x5b78a0
    STarget* pt = PrimaryTarget;
    if (pt && pt->Kind == 5) {
        // An order on a building: enter (1), attack (3) or get in (4).
        int a = GetBuildingAction(WorldUnit(pt->Unit)->WorldIndex);   // 0x5ba2b0
        STarget* cur = CurrentTarget;
        STarget* t = nullptr;
        if (a == 1) {
            if (!cur || cur->Kind != 0) {
                t = STarget::Create(0);                       // new 0x38, 0x5b27c0(0)
                t->Type = kTargetPath;                        // 0x5c18c0
                t->Pos[0] = PrimaryTarget->Pos[0];
                t->Pos[1] = PrimaryTarget->Pos[1];
                t->Pos[2] = PrimaryTarget->Pos[2];
            }
        } else if (a == 3) {
            if (!cur || cur->Kind != 2) {
                t = STarget::Create(2);
                t->Type = kTargetUnit;                        // 0x5c21b0
                t->Unit = PrimaryTarget->Unit;
                t->Mode = 1;
            }
        } else if (a == 4) {
            if (!cur || cur->Kind != 9) {
                t = STarget::Create(9);
                t->Type = kTargetUnit;                        // 0x5c21b0
                t->Unit = PrimaryTarget->Unit;
            }
        }
        if (t)
            SetCurrentTarget(t, RequestedState == 0);         // +0xa0
    }
    if (!PrimaryTarget && !CurrentTarget && RequestedState > -1 && _ec != GlobalState) {
        SetBehavior(_ec);                                     // +0x12c
        RequestedState = -1;
    }
    RefreshSquadFormation();                                  // +0x1cc
    if (!CurrentTarget || tgt::I(CurrentTarget, tgt::kKind) != 8)
        AI_Heartbeat();                                       // +0x190
}

// PANZERS 0x59dda0
// The medic scan (support squads, prototype +0xde): when idle, walking or
// following, the first wounded allied unit within 5 m that can be targeted
// gets a heal order (kind 8); then the squad heals itself if it has a
// wounded member.
void SPanzersSquadUnit::RefreshSquadFormation()
{
    if (!P->SupportPlace || !CanHeal)
        return;
    STarget* cur = CurrentTarget;
    if (cur && (cur->Kind == 8 || cur->Kind == 1))
        return;
    STarget* prim = PrimaryTarget;
    bool primFollow = prim && prim->Kind == 0 && prim->Type == 0;
    bool curFollow = cur && cur->Kind == 0 && cur->Type == 0;
    bool primTurn = prim && prim->Kind == 4;
    bool moving = prim ? prim->Kind == 2 : (cur && (cur->Kind == 2 || cur->Kind == 3));
    bool idle = !prim && !cur;
    if (!idle && !primTurn && !(primFollow && curFollow) && !moving)
        return;
    for (int i = 0; i < SightUnits.Size; ++i) {
        if (!IsTargetable(SightUnitAt(this, i), false))       // 0x5bb6b0
            continue;
        SUnit* t = WorldUnit(SightUnitAt(this, i));
        int ct = t->Proto->ClassType;
        if (ct == 7 || ct == 8 || ct == 9)
            continue;
        if (!WorldIsAlly(Player, t->Player))                  // 0x549ab0
            continue;
        float dz = Pos[2] - t->Pos[2];
        float dx = Pos[0] - t->Pos[0];
        if (!(25.0f > dx * dx + dz * dz))                     // DAT_007f4590
            continue;
        if (t->HasWoundedMember()) {                          // +0x80
            UnitOrderHeal(this, SightUnitAt(this, i));        // 0x5c1030
            break;
        }
    }
    if (CurrentTarget && CurrentTarget->Kind == 8)
        return;
    if (HasWoundedMember())                                   // +0x80
        UnitOrderHeal(this, WorldIndex);                      // 0x5c1030
}

// PANZERS 0x59e0d0
// The squad's tick: delayed actions (+0x18), the driver, then every member
// is sent to its formation point (free space around it, 0x5e5700) with a
// staggered EC_Move; idle squads re-draw a crowded formation; the gunners,
// the members' own ServerRefresh, and the parachute landing.
void SPanzersSquadUnit::RefreshMisc()
{
    int action = _18[0];                                      // +0x18 delayed action
    if (action != 0 && --_18[3] == 0) {                       // +0x24 ticks left
        if (action == 1) {
            SetBehavior(2);                                   // +0x12c
            _130 = 1;
            for (int i = 0; i < Members.Size; ++i)
                MemberAt(this, i)->Slot_104();                // member +0x104 (0x598670)
        } else if (action == 2) {
            UnitSlotsDecreaseAmount(this, 5);                 // 0x5c2270
            if (Members.Size > 0) {
                SUnit* m0 = MemberAt(this, 0);
                float pos[3] = { m0->Pos[0], 0.0f, m0->Pos[2] };
                pos[1] = g_World->GetTerrainHeight(pos[0], pos[2]);   // 0x5e7730
                g_World->CreateUnit(Player, "Waster Tank Mine", pos, 0.0f, 0, 1.0f, -1, true, "");   // 0x5e3170
            }
            if (CurrentTarget)
                GetDriver(ActiveDriver)->SetTarget(CurrentTarget);   // driver +0x08
        } else if (action == 4) {
            STUB_LOG("SPanzersSquadUnit::RefreshMisc (0x59e0d0) delayed action 4 (Waster Explosives, 0x5870f0)");
        }
        _18[0] = 0;
        _18[3] = 0;
        _18[1] = 0;
    }
    float oldDir = Dir;
    if (ActiveDriver >= 0)
        GetDriver(ActiveDriver)->Refresh();                   // driver +0x14
    if (Unplaced)
        return;
    if (Speed > 0.0f && GlobalState == 0)
        UpdateSeenByPlayers();                                // 0x5bc5c0
    if (Pos[0] != PrevPos[0] || Pos[1] != PrevPos[1] || Pos[2] != PrevPos[2] || Dir != oldDir ||
        (CurrentTarget && tgt::I(CurrentTarget, tgt::kType) == 0))
        _264 = g_GameLogic->GetFrame();                       // +0x264 last moved (0x56d1a0)
    int n = Members.Size;
    if (g_GameLogic->GetFrame() > _264 + n * 2) {
        // Standing: 5 s after the last move, a crowded formation is re-drawn.
        if (g_GameLogic->GetFrame() == _264 + 100 && MembersTooClose()) {   // 0x59d130
            SetRelativePositions();                           // 0x5a09d0
            for (int i = 0; i < Members.Size; ++i) {
                float ox, oz;
                GetFormationOffset(i, &ox, &oz);
                if (i < 5) {                                  // HD writes +0x210 + 8 * i unchecked
                    MemberOffset[i][0] = ox;
                    MemberOffset[i][1] = oz;
                }
            }
            _264 = g_GameLogic->GetFrame();
        }
        RestartOrders = true;
    } else {
        for (int i = 0; i < Members.Size; ++i) {
            SUnit* m = MemberAt(this, i);
            float base[3];
            int mdir;
            float tx, tz;
            if (GhostFrames.Count != 0) {
                // The squad's ghost frame 5 ticks ahead (or its last one).
                int idx = GhostFrames.Bottom + 5;
                if (!(idx < GhostFrames.Top))
                    idx = GhostFrames.Top;
                SGhostFrame* f = GhostAt(this, idx);          // SDEQueue::operator[]
                base[0] = f->X;
                base[1] = f->Y;
                base[2] = f->Z;
                memcpy(&mdir, &GhostAt(this, idx)->Gear2[i], 4);   // frame +0x58 + 4 * i
                tx = GhostAt(this, idx)->Gear[i][0] + base[0];      // frame +0x30 + 8 * i
                tz = GhostAt(this, idx)->Gear[i][1] + base[2];
            } else {
                if (m->Speed != 0.0f) {
                    SetSquadMemberRelativePosition(i);        // 0x5a0ac0
                    float ox, oz;
                    GetFormationOffset(i, &ox, &oz);
                    if (i < 5) {
                        MemberOffset[i][0] = ox;
                        MemberOffset[i][1] = oz;
                    }
                }
                base[0] = Pos[0];
                base[1] = Pos[1];
                base[2] = Pos[2];
                mdir = MemberDir[i];
                tx = MemberOffset[i][0] + base[0];
                tz = MemberOffset[i][1] + base[2];
            }
            SetOnBlockMap(false);                             // +0x198
            float out[2];
            g_World->FindEmptySpaceNear(out, tx, tz, base[0], base[2], m->UnitSizeBlocks, m->MoveFlags, true);   // 0x5e5700
            SetOnBlockMap(true);
            float ex = out[0] - tx;
            float ez = out[1] - tz;
            if (ex * ex + ez * ez > 6.25f) {                  // DAT_007f5a74
                out[0] = base[0];
                out[1] = base[2];
            }
            if (i >= MemberOrderDelay.Size)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "int", i);
            if (RestartOrders)
                MemberOrderDelay.Array[i] = i * 2;
            if (MemberOrderDelay.Array[i] > 0)
                MemberOrderDelay.Array[i]--;
            if (MemberOrderDelay.Array[i] == 0) {
                int xb, zb;
                memcpy(&xb, &out[0], 4);
                memcpy(&zb, &out[1], 4);
                m->EC_Move(xb, zb, 0, true, mdir);            // member +0xac (0x598570)
            }
        }
        RestartOrders = false;
    }
    for (int i = 0; i < Gunners.Size; ++i)
        Gunners.Array[i]->ServerRefresh();                    // gunner +0x14
    if (Gunners.Size > 0 && Gunners.Array[0]->Target) {
        bool busy = false;
        for (int i = 0; i < Members.Size; ++i) {
            SUnit* m = MemberAt(this, i);
            int g = m->MainGunner;
            if (g < 0 || g >= m->Gunners.Size)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", g);
            if (m->Gunners.Array[g]->Target)
                busy = true;
        }
        if (!busy) {
            if (CurrentTarget == PrimaryTarget) {
                ClearTargets();                               // +0xc4
            } else {
                Slot_C8();                                    // +0xc8
                AI_Heartbeat();                               // +0x190
            }
        }
    }
    for (int i = 0; i < Members.Size; ++i)
        MemberAt(this, i)->ServerRefresh(LastRefreshFrame);   // member +0x2c
    if (ActiveDriver == 1) {
        SIPDriver* pd = GetDriver(1)->GetPDriver();           // driver +0x04
        if (*(int*)((unsigned char*)pd + 4) == 0xc) {         // the parachute driver
            bool landed = true;
            for (int i = 0; i < Members.Size; ++i)
                if (MemberAt(this, i)->ActiveDriver == 1)
                    landed = false;
            if (landed) {
                SetActiveDriver(0);                           // 0x5c0cb0
                SetFlag112(true);                             // +0x9c(1), squad 0x5a0a10
                Invulnerable = false;                         // +0x111
                SetBehavior(1);                               // +0x12c
                ClearTargets();                               // +0xc4
                _264 = g_GameLogic->GetFrame();
            }
        }
    }
    ServerRefreshMedic(6.0f);                                 // +0x1c0
}

// ---------------------------------------------------------------------------
// SPanzersSquadMemberUnit

// PANZERS 0x5989d0
// A member back from a special weapon takes its default gunner again once
// that weapon has no target.
void SPanzersSquadMemberUnit::RefreshTargeting()
{
    int g = MainGunner;
    if (g <= 0)
        return;
    if (GetGunner(g)->Target == nullptr)                      // 0x55cc40
        MainGunner = P->MainGunner;                           // prototype +0xc0
}

// PANZERS 0x598ab0
void SPanzersSquadMemberUnit::RefreshMisc()
{
    if (ActiveDriver >= 0)
        GetDriver(ActiveDriver)->Refresh();                   // driver +0x14
    if (Unplaced) {
        if (!InVehicleAnim)
            return;
        if (_260 != 0)
            return;
    }
    for (int i = 0; i < Gunners.Size; ++i)
        Gunners.Array[i]->ServerRefresh();                    // gunner +0x14
    int g = MainGunner;
    if (g >= 0 && g != P->MainGunner) {
        // HD tests the target again and would release it (0x5bdef0): dead code.
        if (GetGunner(g)->Target == nullptr) {
            SUnit* boss = WorldUnit(Parent);                  // +0x78
            switch (MainGunner) {
            case 1:
            case 2:
            case 3:
            case 4:
                UnitSlotsDecreaseAmount(boss, MainGunner);    // 0x5c2270
                break;
            default:
                break;
            }
            MainGunner = 0;
        }
    }
    if (ActiveDriver == 0 && ParachuteTicks > 0) {
        Parachute->AdvanceAnimation(0.05f);                   // +0x70 (0x3d4ccccd)
        Parachute->StoreInterpolationState();                 // +0x3c
        bool done = (float)ParachuteTicks == 80.0f;           // DAT_007f9d70
        ++ParachuteTicks;
        if (done) {
            Parachute->SetVisible(false, true);               // +0x30
            ParachuteTicks = 0;
        }
    }
}

// PANZERS 0x597a10
// Per frame: the health bar board elements (+0x344 / +0x348) over the
// member, coloured by HP. The recompile creates no board elements, so
// nothing is left; HD does not call SUnit::UpdateVisuals here.
void SPanzersSquadMemberUnit::UpdateVisuals(SIViewport* vp)
{
    (void)vp;
}

// PANZERS 0x598570
// Unlike SUnit::EC_Move the member keeps no target of its own: the new
// target goes straight to the active driver.
void SPanzersSquadMemberUnit::EC_Move(int xBits, int zBits, int p3, bool p4, int p5)
{
    (void)p3;
    if (ActiveDriver < 0)
        return;
    STarget* t = STarget::Create(0);                          // new 0x38, 0x5b27c0(0)
    float xz[2];
    memcpy(&xz[0], &xBits, 4);
    memcpy(&xz[1], &zBits, 4);
    if (p4)
        TargetSetGroundPosDir(t, xz, p5);                     // 0x5c18e0
    else
        t->SetGroundPos(xz);                                  // 0x5c1860
    GetDriver(ActiveDriver)->SetTarget(t);                    // driver +0x08
}

// PANZERS 0x5986e0
void SPanzersSquadMemberUnit::StopGunners()
{
    if (MainGunner >= 0)
        for (int i = 0; i < Gunners.Size; ++i)
            Gunners.Array[i]->Stop();                         // gunner +0x28
    int g = MainGunner;
    if (g > 0) {
        GetGunner(g)->Stop();
        MainGunner = 0;
    }
}

// PANZERS 0x5980b0
// The member's wait before it follows a new behavior (+0x2f0 ticks): none
// for the squad's first member, else random (0..2, or 0..10 even while
// standing).
void SPanzersSquadMemberUnit::SetBehavior(int behavior)
{
    SUnit::SetBehavior(behavior);                             // 0x5b8960
    if (Parent > -1) {
        SUnit* boss = WorldUnit(Parent);
        if (boss->Proto->ClassType == 5 && boss->Members.Size > 0) {
            SUnit* b = WorldUnit(Parent);
            if (b->Members.Size < 1)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnitMember", 0);
            if (b->Members.Array[0].Unit == WorldIndex) {
                _2f0 = 0;
                return;
            }
        }
    }
    int r = WorldRand();
    if (Speed == 0.0f)
        _2f0 = (int)((double)r * 3.0517578125e-05 * 6.0) * 2;   // DAT_007f4568
    else
        _2f0 = (int)((double)r * 3.0517578125e-05 * 3.0);       // DAT_007f9d68
}

// PANZERS 0x5979d0
void SPanzersSquadMemberUnit::AI_Heartbeat()
{
    Logger.g->Log(0, "### SPanzersSquadMemberUnit::AI_Heartbeat - IDE NEM KELLENE BEJONNI !!!");
}

// PANZERS 0x5979f0
// Returns false in HD (the slot is typed void in iunit.h).
void SPanzersSquadMemberUnit::Slot_19C()
{
}

// PANZERS 0x5989c0
void SPanzersSquadMemberUnit::MarkBlockMap(bool on, int p2, int p3, int p4, short p5)
{
    (void)on;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
}

// PANZERS 0x597a00
int SPanzersSquadMemberUnit::TestBlockMap(int p1, int p2, int p3, int p4, short p5)
{
    (void)p1;
    (void)p2;
    (void)p3;
    (void)p4;
    (void)p5;
    return 0;
}

// PANZERS 0x5988d0
float SPanzersSquadMemberUnit::GetMoveSpeed(int p1)
{
    (void)p1;
    Logger.g->Panic("SPanzersSquadMemberUnit::GetMaxMoveSpeed() - forbidden function");
    return 0.0f;
}

// PANZERS 0x597fa0
void SPanzersSquadMemberUnit::EC_AttackPos(int xBits, int zBits, int p3)
{
    (void)p3;
    if (MainGunner < 0)
        return;
    STarget* t = STarget::Create(2);                          // new 0x38, 0x5b27c0(2)
    float xz[2];
    memcpy(&xz[0], &xBits, 4);
    memcpy(&xz[1], &zBits, 4);
    t->SetGroundPos(xz);                                      // 0x5c1860
    GetGunner(MainGunner)->SetTarget(t);                      // gunner +0x18
}

// PANZERS 0x597e80
void SPanzersSquadMemberUnit::EC_Attack(int unit, int p2)
{
    (void)p2;
    if (!g_World->Units.IsLive(unit) || unit == WorldIndex)
        return;
    if (WorldUnit(unit)->Proto->ClassType == 3) {
        Logger.g->Log(1, "SUnit::EC_Attack: Attacking projectile");
        return;
    }
    if (MainGunner < 0)
        return;
    STarget* t = STarget::Create(2);                          // new 0x38, 0x5b27c0(2)
    t->Type = kTargetUnit;                                    // 0x5c21b0
    t->Unit = unit;
    GetGunner(MainGunner)->SetTarget(t);                      // gunner +0x18
}

// ---------------------------------------------------------------------------
// g_DriverEnv forms (driverunit.h; the coordinator wires them)

static SPanzersSquadUnit* AsSquad(SIUnit* u)
{
    return static_cast<SPanzersSquadUnit*>(static_cast<SUnit*>(u));
}

float SquadEnv_MoveSpeed(SIUnit* squad)
{
    return AsSquad(squad)->GetSquadMoveSpeed();               // 0x59b990
}

void SquadEnv_MemberRelativePos(SIUnit* squad, float* out, int member)
{
    AsSquad(squad)->GetSquadMemberRelativePosition(out, member);   // 0x59bff0
}

void SquadEnv_MembersStep(SIUnit* squad, float dir)
{
    AsSquad(squad)->StepMemberRadius(dir);                    // 0x5a0450
}

void SquadEnv_MembersStep2(SIUnit* squad, float dir)
{
    AsSquad(squad)->StepMemberAngle(dir);                     // 0x5a0060
}

} // namespace pz
