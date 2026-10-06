// src/game/unitanim.cpp
// pz::SUnitAnimation family (0x5c6300..0x5cfb00): the base, the prototypes
// and their loaders, SWalkerAnimation, SSquadAnimation and
// SBuildingAnimation. SVehicleAnimation and the running gear are in
// unitanim_vehicle.cpp. OWNER: agent A.

#include <windows.h>
#include <intrin.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unitanim.h"
#include "unitanim_model.h"
#include "drivermath.h"
#include "iunit.h"
#include "idriver.h"
#include "pz/hdmath.h"
#include "pz/imodel.h"
#include "pz/iscene.h"
#include "pz/igepardhd.h"
#include "pz/pzgepard.h"
#include "pz/pmodel.h"
#include "pz/ipixie.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

PZ_HD_SIZE(SUnitAnimation, 0x24);
PZ_HD_SIZE(SVehicleAnimation, kHdSizeSVehicleAnimation);
PZ_HD_SIZE(SWalkerAnimation, kHdSizeSWalkerAnimation);
PZ_HD_SIZE(SSquadAnimation, kHdSizeSSquadAnimation);
PZ_HD_SIZE(SBuildingAnimation, kHdSizeSBuildingAnimation);
PZ_HD_SIZE(SPUnitAnimation, 0x10);
PZ_HD_SIZE(SPVehicleAnimation, 0x3c);
PZ_HD_SIZE(SPWalkerAnimation, 0x28);
PZ_HD_SIZE(SPSquadAnimation, 0x10);
PZ_HD_SIZE(SPBuildingAnimation, 0x10);
PZ_HD_SIZE(SWalkerStateInfo, 0x24);
PZ_HD_SIZE(SAnimGun, 0x18);

static const float kPi = 3.1415927410125732f;                    // 0x7f4584
static const float kTick = 0.05f;                                 // 0x7f4534

// ---------------------------------------------------------------------------
// Helpers

const char* HdStr(const SHdStr& s)
{
    return s.buf ? s.buf : "";
}

void HdStrSet(SHdStr* s, const char* text, int len)
{
    HdStrFree(s);
    if (!text)
        return;
    if (len < 0)
        len = (int)strlen(text);
    s->buf = (char*)malloc((size_t)len + 1);
    memcpy(s->buf, text, (size_t)len);
    s->buf[len] = 0;
    s->len = len;
}

void HdStrFree(SHdStr* s)
{
    free(s->buf);
    s->buf = nullptr;
    s->len = 0;
}

static void HdStrFormat(SHdStr* s, const char* fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    int n = _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    if (n < 0)
        n = (int)strlen(buf);
    HdStrSet(s, buf, n);
}

// SDArray::Add (grow to 16, then by 6/5): returns the new element's index.
template <class T>
static int HdAdd(SHdDArray<T>& a)
{
    if (a.Count == a.Max) {
        int m = a.Max < 0x10 ? 0x10 : (a.Max * 6) / 5;
        a.Data = (T*)realloc(a.Data, (size_t)m * sizeof(T));
        memset(a.Data + a.Max, 0, (size_t)(m - a.Max) * sizeof(T));
        a.Max = m;
    }
    return a.Count++;
}

template <class T>
static void HdFree(SHdDArray<T>& a)
{
    free(a.Data);
    a.Data = nullptr;
    a.Count = a.Max = 0;
}

// The animation code converts with fld dword / fldcw 0x8de160 (0x087f: round
// up) / fistp (0x5c7ce5, 0x5ce1a0, 0x5cea81, ..., 0x5cf5ef, 0x5cf65e): ceil.
// The product is an x87 fmul at the game's 24-bit precision, stored with
// fstp dword: a single-precision product, done here in SSE so the compiler
// cannot keep a wider intermediate.
static int RoundHdTicks(float seconds, float factor = 1.0f)
{
    __m128 p = _mm_mul_ss(_mm_set_ss(seconds), _mm_set_ss(20.0f));   // fmul dword [0x7f35d8]
    if (factor != 1.0f)
        p = _mm_mul_ss(p, _mm_set_ss(factor));
    return (int)ceilf(_mm_cvtss_f32(p));
}

unsigned UnitAnimNextSeed()
{
    unsigned s = HdLcgStep(g_UnitAnimEnv.Seed());
    static int s_Trace = -1;                                      // recompile only: PZ_M2_RNGLOG=1
    if (s_Trace < 0)
        s_Trace = getenv("PZ_M2_RNGLOG") ? 1 : 0;
    if (s_Trace && Logger.g) {
        void* bt[3] = {};
        RtlCaptureStackBackTrace(1, 3, bt, nullptr);
        Logger.g->Log(0, "PZM2 RNG %d %p %08x %p %p", g_UnitAnimEnv.HasGameLogic() ? g_UnitAnimEnv.Frame() : -1,
                      bt[0], s, bt[1], bt[2]);
    }
    return s;
}

// PANZERS 0x555a00 (HdRandInt on the env's seed, for the animview tool)
int UnitAnimRand(int n)
{
    return HdRandScale(HdLcg15(UnitAnimNextSeed()), n);
}

static SIPUnit* PUnitOf(SIUnit* u)
{
    return UnitField<SIPUnit*>(u, kUnitPUnit);
}

static int ClassOf(SIUnit* u)
{
    return PUnitField<int>(PUnitOf(u), kPUnitClassType);
}

static const char* SubState(SIUnit* u)
{
    return HdStr(UnitField<SHdStr>(u, kUnitSubState));
}

// Is the unit seen by the local player (no fog of war in the menu).
static bool SeenByLocalPlayer(SIUnit* u)
{
    SUnitAnimEnv& e = g_UnitAnimEnv;
    return e.NoFogOfWar() || !e.HasGameLogic() || e.CanSeeGroundUnit(e.LocalPlayer(), u);
}

// SUnit 0x5c10f0 (agent U's function; the walker needs it): the movement
// mode from the speeds of the tick.
static void UnitUpdateMoveMode(SIUnit* u)
{
    int& mode = UnitField<int>(u, kUnitMoveMode);
    if (UnitField<bool>(u, kUnitStateChanging)) {
        mode = 6;
        return;
    }
    if (0.0f < UnitField<float>(u, kUnitMoveSpeed)) {
        mode = 2;
        return;
    }
    double turn = (double)UnitField<float>(u, kUnitTurnSpeed);
    if (1e-05 < turn) {
        mode = 4;
        return;
    }
    mode = (turn < -1e-05) ? 5 : 1;
}

// The end of a global state change (the counter at unit +0xf4), shared by
// the vehicle and walker UpdateModel.
static void TickStateChange(SIUnit* u)
{
    if (!UnitField<bool>(u, kUnitStateChanging))
        return;
    int& ticks = UnitField<int>(u, kUnitChangeTicks);
    --ticks;
    if (ticks < 1) {
        UnitField<bool>(u, kUnitStateChanging) = false;
        UnitUpdateMoveMode(u);
        int behavior = UnitField<int>(u, kUnitBehavior);
        if (behavior != UnitField<int>(u, kUnitGlobalState))
            u->SetBehavior(behavior);                             // unit +0x12c
    }
}

// SUnit 0x5c26e0: death effects of the unit type. Agent U's function; units
// do not die in the menu.
static void UnitStartDeathEffects(SIUnit* u)
{
    (void)u;
    PZ_M2_TRACE("SUnit 0x5c26e0 (death effects, agent U) not called");
}

// "enemy spotted" bookkeeping of vehicles and squads (0x5cd020, 0x5cc8c0).
void UnitAnimSpottedCheck(SIUnit* u)
{
    SUnitAnimEnv& e = g_UnitAnimEnv;
    int lp = e.LocalPlayer();
    int team = e.PlayerTeam(lp);
    bool same = team == 0 ? lp == UnitField<int>(u, kUnitPlayer)
                          : team == e.PlayerTeam(UnitField<int>(u, kUnitPlayer));
    if (same)
        return;
    if (!UnitField<bool>(u, kUnitSpotted) && !UnitField<bool>(u, kUnitSpotted110)) {
        if (!e.HasGameLogic())
            return;
        if (UnitField<int>(u, kUnitSpottedFrame) < e.Frame() - 100) {
            UnitField<bool>(u, kUnitSpotted) = true;
            // HD: the nearest own unit (class 0 or 5) that has the unit in
            // its sight range (+0x184) says "enemy spotted" (SWorld 0x5fff20,
            // speech 0xe, 0x11 for class 7). Not lifted.
            PZ_M2_TRACE("UnitAnimSpottedCheck: enemy-spotted speech (SWorld 0x5fff20) not lifted");
            return;
        }
    }
    if (e.HasGameLogic())
        UnitField<int>(u, kUnitSpottedFrame) = e.Frame();
}

// ---------------------------------------------------------------------------
// SPUnitAnimation (base prototype, vftable 0x7fd724)

SPUnitAnimation::SPUnitAnimation()
{
    StateNames.Data = nullptr;
    StateNames.Count = 0;
    StateNames.Max = 0;
}

// PANZERS 0x5c7290
SPUnitAnimation::~SPUnitAnimation()
{
    for (int i = 0; i < StateNames.Count; ++i)
        HdStrFree(&StateNames.Data[i]);
    HdFree(StateNames);
}

void SPUnitAnimation::Load(SIPUnit* punit, ::SPropertyStruct* props)
{
    PZ_M2_TRACE("SPUnitAnimation::Load (+0x04)");
    LoadProps(punit, SAnimProps::FromTree(props));
}

void SPUnitAnimation::LoadResources(SIPUnit* punit, ::SPropertyStruct* props)
{
    PZ_M2_TRACE("SPUnitAnimation::LoadResources (+0x08)");
    LoadResourcesProps(punit, SAnimProps::FromTree(props));
}

void SPUnitAnimation::LoadProps(SIPUnit* punit, const SAnimProps& props)
{
    STUB_LOG("SPUnitAnimation::Load (pure in HD SPUnitAnimation)");
    (void)punit;
    (void)props;
}

// PANZERS 0x5ca680
void SPUnitAnimation::LoadResourcesProps(SIPUnit* punit, const SAnimProps& props)
{
    (void)punit;
    (void)props;
}

void SPUnitAnimation::Slot_0C()
{
    STUB_LOG("SPUnitAnimation::Slot_0C (0x5cb470)");
    PZ_M2_TRACE("SPUnitAnimation::Slot_0C (0x5cb470)");
}

SIUnitAnimation* SPUnitAnimation::CreateAnimation(SIUnit* unit)
{
    STUB_LOG("SPUnitAnimation::CreateAnimation (pure in HD SPUnitAnimation)");
    (void)unit;
    return nullptr;
}

const char* SPUnitAnimation::StateName(int i) const
{
    if (i < 0 || i >= StateNames.Count)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SString", i);
    return HdStr(StateNames.Data[i]);
}

// PANZERS 0x5c7ed0
int SPUnitAnimation::FindState(const char* name) const
{
    int len = (int)strlen(name);
    for (int i = 0; i < StateNames.Count; ++i) {
        const SHdStr& s = StateNames.Data[i];
        if (len == s.len && (len == 0 || _stricmp(name, s.buf) == 0))
            return i;
    }
    return 0;
}

// ---------------------------------------------------------------------------
// SPWalkerAnimation (0x28, vftable 0x7fd754)

// PANZERS 0x5c6690
SPWalkerAnimation::SPWalkerAnimation()
{
    States.Data = nullptr;
    States.Count = 0;
    States.Max = 0;
    ShadowTexture = -1;
    LeftOarProto = -1;
    RightOarProto = -1;
}

// PANZERS 0x5c7310
SPWalkerAnimation::~SPWalkerAnimation()
{
    HdFree(States);
    if (ShadowTexture >= 0)
        PzGepard()->ReleaseTexture(ShadowTexture);
    if (LeftOarProto >= 0)
        PzGepard()->ReleaseModelPrototype(LeftOarProto);
    if (RightOarProto >= 0)
        PzGepard()->ReleaseModelPrototype(RightOarProto);
}

// PANZERS 0x5c86e0
// SPWalkerAnimation::Init: the global states ("States" array: StateText,
// MoveSpeed, SpinSpeed), then "vehicle" and, for unit type 0x13
// (parachutists), "parachute".
void SPWalkerAnimation::LoadProps(SIPUnit* punit, const SAnimProps& props)
{
    PZ_M2_TRACE("SPWalkerAnimation::Init (0x5c86e0)");
    int n = props.GetArraySize("States");
    for (int i = 0; i < n; ++i) {
        SAnimProps st = props.GetArrayItem("States", i);
        int idx = HdAdd(StateNames);
        HdStrSet(&StateNames.Data[idx], st.GetString("StateText"));
        int sidx = HdAdd(States);
        if (idx != sidx)
            Logger.g->Panic("SPWalkerAnimation::Init  -  indx != idx");
        SWalkerStateInfo& s = States.Data[sidx];
        s.MoveSpeed = st.GetFloat("MoveSpeed");
        s.MoveSpeed = s.MoveSpeed / 3.6f;                         // km/h -> m/s
        s.MoveSpeed = s.MoveSpeed * 100.0f;
        s.MoveSpeed = s.MoveSpeed / 20.0f;                        // per tick
        s.MoveSpeed = s.MoveSpeed * 0.005f;
        s.SpinSpeed = st.GetFloat("SpinSpeed");
        s.SpinSpeed = s.SpinSpeed / 20.0f;
        s.SpinSpeed = s.SpinSpeed * 0.017453292f;                 // degrees -> radians
    }
    int v = HdAdd(StateNames);
    HdStrSet(&StateNames.Data[v], "vehicle");
    HdAdd(States);
    if (punit && PUnitField<int>(punit, kPUnitUnitType) == 0x13) {
        int p = HdAdd(StateNames);
        HdStrSet(&StateNames.Data[p], "parachute");
        HdAdd(States);                                            // 0x5c75c0
    }
}

// PANZERS 0x5c8130
// Number of consecutive sequences "<state>_<name>1", "<state>_<name>2", ...
int SPWalkerAnimation::CountSequences(const char* name, int modelProto, int state)
{
    int n = 0;
    for (;;) {
        char seq[256];
        _snprintf(seq, sizeof(seq) - 1, "%s_%s%d", StateName(state), name, n + 1);
        seq[sizeof(seq) - 1] = 0;
        if (AnimProtoFindSequence(modelProto, seq) < 0)           // Gepard +0x30
            return n;
        ++n;
    }
}

// PANZERS 0x5ca980
void SPWalkerAnimation::LoadResourcesProps(SIPUnit* punit, const SAnimProps& props)
{
    PZ_M2_TRACE("SPWalkerAnimation::LoadResources (0x5ca980)");
    SIGepardHD* g = PzGepard();
    const char* shadow = props.Valid() ? props.GetString("ShadowTexture") : "";
    ShadowTexture = g->LoadTexture(shadow, 1, true);              // Gepard +0x44
    LeftOarProto = g->LoadModelPrototype("units/walker/oar_left.4d", 0.005f, nullptr, 0);    // Gepard +0x20
    RightOarProto = g->LoadModelPrototype("units/walker/oar_right.4d", 0.005f, nullptr, 0);
    int proto = punit ? PUnitField<int>(punit, kPUnitModelProto) : -1;
    for (int i = 0; i < States.Count; ++i) {
        SWalkerStateInfo& s = States.Data[i];
        s.IdleCount = CountSequences("idle", proto, i);
        s.RelaxCount = CountSequences("idle_relax", proto, i);
        s.FightCount = CountSequences("fight", proto, i);
        s.DieBoomCount = CountSequences("die_boom", proto, i);
        s.DieFireCount = CountSequences("die_fire", proto, i);
        s.DieCount = CountSequences("die", proto, i);
        s.StressCount = CountSequences("stress", proto, i);
    }
}

// PANZERS 0x5c7ab0
SIUnitAnimation* SPWalkerAnimation::CreateAnimation(SIUnit* unit)
{
    PZ_M2_TRACE("SPWalkerAnimation::CreateAnimation (0x5c7ab0)");
    return new SWalkerAnimation(this, unit);
}

// ---------------------------------------------------------------------------
// SPSquadAnimation / SPBuildingAnimation (0x10)

// PANZERS 0x5c85b0
void SPSquadAnimation::LoadProps(SIPUnit* punit, const SAnimProps& props)
{
    (void)punit;
    (void)props;
}

// PANZERS 0x5c7930
SIUnitAnimation* SPSquadAnimation::CreateAnimation(SIUnit* unit)
{
    PZ_M2_TRACE("SPSquadAnimation::CreateAnimation (0x5c7930)");
    return new SSquadAnimation(this, unit);
}

// PANZERS 0x5c8470
void SPBuildingAnimation::LoadProps(SIPUnit* punit, const SAnimProps& props)
{
    (void)punit;
    (void)props;
}

// PANZERS 0x5ca620
// Roof ("teto") before interior ("belso") in the prototype: swap them so the
// interior is drawn first.
void SPBuildingAnimation::LoadResourcesProps(SIPUnit* punit, const SAnimProps& props)
{
    (void)props;
    if (!punit)
        return;
    int proto = PUnitField<int>(punit, kPUnitModelProto);
    SPModel* p = proto >= 0 ? GepardModelPrototype(proto) : nullptr;
    if (!p)
        return;
    int roof = -1, interior = -1;
    for (int i = 0; i < p->NodeCount; ++i) {                     // Gepard +0x2c FindNode
        const char* n = p->Nodes[i].Name.buf ? p->Nodes[i].Name.buf : "";
        if (roof < 0 && _stricmp(n, "teto") == 0)
            roof = i;
        if (interior < 0 && _stricmp(n, "belso") == 0)
            interior = i;
    }
    if (roof >= 0 && interior >= 0 && roof < interior)
        PzGepard()->SwitchModelPrototypeNodes(proto, roof, interior);   // Gepard +0x38
}

// PANZERS 0x5c7770
SIUnitAnimation* SPBuildingAnimation::CreateAnimation(SIUnit* unit)
{
    PZ_M2_TRACE("SPBuildingAnimation::CreateAnimation (0x5c7770)");
    return new SBuildingAnimation(this, unit);
}

// ---------------------------------------------------------------------------
// Prototypes of the types no menu unit uses

PZ_HD_SIZE(SPProjectileAnimation, 0x10);
PZ_HD_SIZE(SPWasterAnimation, 0x10);
PZ_HD_SIZE(SPFlyingAnimation, 0x34);
PZ_HD_SIZE(SPTrainAnimation, 0x3c);
PZ_HD_SIZE(SPBoatAnimation, 0x3c);

// PANZERS 0x5c85a0
void SPProjectileAnimation::LoadProps(SIPUnit* punit, const SAnimProps& props)
{
    (void)punit;
    (void)props;
}

// The SProjectileAnimation itself is in projectile_anim.cpp (M3-C C1); the
// animview tool, which does not compile it, gets the null default.
} // namespace pz
extern "C" pz::SIUnitAnimation* PzNewProjectileAnimation(pz::SPProjectileAnimation* proto, pz::SIUnit* unit);
extern "C" pz::SIUnitAnimation* PzNewProjectileAnimation_Default(pz::SPProjectileAnimation* proto, pz::SIUnit* unit)
{
    (void)proto;
    (void)unit;
    return nullptr;
}
__pragma(comment(linker, "/alternatename:_PzNewProjectileAnimation=_PzNewProjectileAnimation_Default"))
namespace pz {

// PANZERS 0x5c78e0
SIUnitAnimation* SPProjectileAnimation::CreateAnimation(SIUnit* unit)
{
    return PzNewProjectileAnimation(this, unit);              // new 0x2c, 0x5c67d0, vftable 0x7fd994
}

// PANZERS 0x5c8be0
void SPWasterAnimation::LoadProps(SIPUnit* punit, const SAnimProps& props)
{
    (void)punit;
    (void)props;
}

// SPWasterAnimation::CreateAnimation 0x5c7b20: waster.cpp (M3-C C3).

// PANZERS 0x5c6540
SPFlyingAnimation::SPFlyingAnimation()
{
    RunningGear = nullptr;
    SpringStrength = SpringDecay = SpringScale = 0.0f;
    RodSpringStrength = RodSpringDecay = RodSpringScale = 0.0f;
    MaxSpringAngle = MaxSpringAngle2 = 0.0f;
}

// PANZERS 0x5c71e0
SPFlyingAnimation::~SPFlyingAnimation()
{
    delete RunningGear;
    RunningGear = nullptr;
}

// PANZERS 0x5c8480
// Same body as SPVehicleAnimation::Load 0x5c85c0.
void SPFlyingAnimation::LoadProps(SIPUnit* punit, const SAnimProps& p)
{
    (void)punit;
    RunningGear = new SPRunningGear();
    RunningGear->Load(p);
    SpringStrength = p.GetFloat("SpringStrength");
    SpringDecay = (float)(1.0f - p.GetFloat("SpringDecay"));
    SpringScale = p.GetFloat("SpringScale");
    float a = (float)((((double)p.GetFloat("MaxSpringAngle") * 1.8) / 180.0) * 3.1415927410125732);
    MaxSpringAngle = a;
    MaxSpringAngle2 = a * a;
    RodSpringStrength = p.GetFloat("RodSpringStrength");
    RodSpringDecay = (float)(1.0f - p.GetFloat("RodSpringDecay"));
    RodSpringScale = p.GetFloat("RodSpringScale");
}

// SPFlyingAnimation::CreateAnimation 0x5c77c0: flying.cpp (M3-C C3).

SIUnitAnimation* SPTrainAnimation::CreateAnimation(SIUnit* unit)
{
    STUB_LOG("SPTrainAnimation::CreateAnimation (0x5c7990)");
    (void)unit;
    return nullptr;
}

SIUnitAnimation* SPBoatAnimation::CreateAnimation(SIUnit* unit)
{
    STUB_LOG("SPBoatAnimation::CreateAnimation (0x5c76d0)");
    (void)unit;
    return nullptr;
}

// ---------------------------------------------------------------------------
// SPUnit "Animation" (0x5a6ce0)

SPUnitAnimation* CreatePUnitAnimation(int type)
{
    switch (type) {
    case 1: return new SPVehicleAnimation();                     // 0x3c, 0x5c6610
    case 2: return new SPWalkerAnimation();                      // 0x28, 0x5c6690
    case 4: return new SPSquadAnimation();                       // 0x10, 0x5c65e0
    case 5: return new SPBuildingAnimation();                    // 0x10, 0x5c6510
    case 3: return new SPProjectileAnimation();                  // 0x10, 0x5c65b0
    case 6: return new SPWasterAnimation();                      // 0x10, 0x5c66e0
    case 7: return new SPFlyingAnimation();                      // 0x34, 0x5c6540
    case 8: return new SPTrainAnimation();                       // 0x3c, 0x5c6610 + vftable 0x7fa9ac
    case 9: return new SPBoatAnimation();                        // 0x3c, 0x5c6610 + vftable 0x7fa9c4
    default:
        return nullptr;
    }
}

// PANZERS 0x5a6ce0
SPUnitAnimation* LoadPUnitAnimation(SIPUnit* punit, const SAnimProps& common)
{
    int type = common.GetMultiIndex("Animation");
    SAnimProps sub = common.GetMultiSub("Animation");
    SPUnitAnimation* a = CreatePUnitAnimation(type);
    if (a)
        a->LoadProps(punit, sub);                                 // +0x04(punit, sub)
    return a;
}

// ---------------------------------------------------------------------------
// SUnitAnimation (base, vftable 0x7fd7e4)

// PANZERS 0x5c67d0
SUnitAnimation::SUnitAnimation(SPUnitAnimation* proto, SIUnit* unit)
{
    Slots.Data = nullptr;
    Slots.Count = 0;
    Slots.Max = 0;
    Proto = proto;
    Unit = unit;
    Timer = 0;
    unsigned r2 = UnitAnimNextSeed();
    unsigned r1 = UnitAnimNextSeed();
    Type = 0;
    float a = (float)(int)((double)((r1 >> 16) & 0x7fff) * 3.0517578125e-05 * 5.0);
    float b = (float)(int)((double)((r2 >> 16) & 0x7fff) * 3.0517578125e-05 * 5.0);
    SpeedFactor = (float)(((double)(a / 100.0f) - 0.02) + (double)(b / 10.0f + 0.8f));
}

// PANZERS 0x5c74c0
SUnitAnimation::~SUnitAnimation()
{
    HdFree(Slots);
}

void SUnitAnimation::InitModel(SIModel* model)
{
    STUB_LOG("SUnitAnimation::InitModel (pure in HD SUnitAnimation)");
    (void)model;
}

void SUnitAnimation::UpdateModel()
{
    STUB_LOG("SUnitAnimation::UpdateModel (pure in HD SUnitAnimation)");
}

// PANZERS 0x5c76c0
void SUnitAnimation::Slot_0C(void* p1)
{
    (void)p1;
}

float* SUnitAnimation::GetFirePosition(float* out, int gunner, float dirOffset, float kick)
{
    STUB_LOG("SUnitAnimation::GetFirePosition (pure in HD SUnitAnimation)");
    PZ_M2_TRACE("SUnitAnimation::GetFirePosition (pure in HD SUnitAnimation)");
    (void)gunner; (void)dirOffset; (void)kick;
    out[0] = out[1] = out[2] = 0.0f;
    return out;
}

// PANZERS 0x5cb420
void SUnitAnimation::AddBodyKick(float x, float y, float z)
{
    (void)x; (void)y; (void)z;
}

// PANZERS 0x5c7e90
float SUnitAnimation::GetStateMoveSpeed(int state)
{
    (void)state;
    return 0.0f;
}

// PANZERS 0x5c7f50
float SUnitAnimation::GetStateTurnSpeed(int state)
{
    (void)state;
    return 0.0f;
}

// PANZERS 0x5c7c60
int SUnitAnimation::GetDriverNode()
{
    return -1;
}

// PANZERS 0x5c8410
int SUnitAnimation::GetHookNode()
{
    return -1;
}

// PANZERS 0x5c8350
int SUnitAnimation::GetHoleFNode()
{
    return -1;
}

// PANZERS 0x5c83b0
int SUnitAnimation::GetHoleRNode()
{
    return -1;
}

// PANZERS 0x5c8430
float* SUnitAnimation::GetHookPos(float* out2)
{
    out2[0] = out2[1] = 0.0f;
    return out2;
}

// PANZERS 0x5c8370
float* SUnitAnimation::GetHoleFPos(float* out2)
{
    out2[0] = out2[1] = 0.0f;
    return out2;
}

// PANZERS 0x5c83d0
float* SUnitAnimation::GetHoleRPos(float* out2)
{
    out2[0] = out2[1] = 0.0f;
    return out2;
}

// ---------------------------------------------------------------------------
// Fire positions and body kick (M3-C): the gunner asks the animation for the
// muzzle position when it fires (SGunner::ServerRefresh 0x584d00).

// PANZERS 0x5cb270
// The next muzzle node of the gun (the counter +0x30 cycles through the gun's
// muzzles), and the body spring pushed back along the shot direction.
float* SVehicleAnimation::GetFirePosition(float* out, int gunner, float dirOffset, float kick)
{
    double a = (double)UnitField<float>(Unit, 0xb0) + (double)dirOffset;   // unit +0xb0 dir
    if (a > 3.141592653589793)                                  // DAT_007f4560
        a -= 6.283185307179586;                                 // DAT_007f4570
    else if (-3.141592653589793 > a)                            // DAT_007f5aa0
        a += 6.283185307179586;
    out[0] = out[1] = out[2] = 0.0f;
    float af = (float)a;                                        // cvtpd2ps
    Model()->GetNodePosition(Guns[gunner].M[_30], out);         // model +0x54
    ++_30;
    if (_30 > Guns[gunner].MCount - 1)
        _30 = 0;
    SpringVel[0] = SpringVel[0] - HdSin((double)af) * (double)kick;
    SpringVel[1] = SpringVel[1] - HdCos((double)af) * (double)kick;
    return out;
}

// PANZERS 0x5cb430
void SVehicleAnimation::AddBodyKick(float x, float y, float z)
{
    (void)y;
    SpringVel[0] = SpringVel[0] - (double)x;
    SpringVel[1] = SpringVel[1] - (double)z;
}

// PANZERS 0x5c8420
int SVehicleAnimation::GetHookNode()
{
    return HookNode;
}

// PANZERS 0x5c8360
int SVehicleAnimation::GetHoleFNode()
{
    return HoleFNode;
}

// PANZERS 0x5c83c0
int SVehicleAnimation::GetHoleRNode()
{
    return HoleRNode;
}

// PANZERS 0x5c8450
float* SVehicleAnimation::GetHookPos(float* out2)
{
    out2[0] = HookPos[0];
    out2[1] = HookPos[1];
    return out2;
}

// PANZERS 0x5c8390
float* SVehicleAnimation::GetHoleFPos(float* out2)
{
    out2[0] = HoleFPos[0];
    out2[1] = HoleFPos[1];
    return out2;
}

// PANZERS 0x5c83f0
float* SVehicleAnimation::GetHoleRPos(float* out2)
{
    out2[0] = HoleRPos[0];
    out2[1] = HoleRPos[1];
    return out2;
}

// PANZERS 0x5cb370
// Soldiers fire from the first muzzle of gun 0, or of gun 1 for any gunner > 0.
float* SWalkerAnimation::GetFirePosition(float* out, int gunner, float dirOffset, float kick)
{
    (void)dirOffset; (void)kick;
    out[0] = out[1] = out[2] = 0.0f;
    Model()->GetNodePosition(Guns[gunner > 0 ? 1 : 0].M[0], out);   // model +0x54
    return out;
}

// PANZERS 0x5cb240
float* SSquadAnimation::GetFirePosition(float* out, int gunner, float dirOffset, float kick)
{
    (void)gunner; (void)dirOffset; (void)kick;
    out[0] = UnitField<float>(Unit, 0x8c);
    out[1] = UnitField<float>(Unit, 0x90);
    out[2] = UnitField<float>(Unit, 0x94);
    return out;
}

// HD 0x5c76a0: 0x5cadc0(p1, unit model) places the squad's extra model (+0x28)
// at the unit model's position with p1's orientation (visual only).
void SSquadAnimation::Slot_0C(void* p1)
{
    if (Extra) {
        STUB_LOG("SSquadAnimation::Slot_0C (0x5c76a0) extra model placement 0x5cadc0");
    }
    (void)p1;
}

// PANZERS 0x5cb100
float* SBuildingAnimation::GetFirePosition(float* out, int gunner, float dirOffset, float kick)
{
    (void)gunner; (void)dirOffset; (void)kick;
    out[0] = out[1] = out[2] = 0.0f;
    return out;
}

// PANZERS 0x5c8250
int SUnitAnimation::GetShadowTexture()
{
    return -1;
}

// PANZERS 0x5c8240
SIPUnitAnimation* SUnitAnimation::GetPrototype()
{
    return Proto;
}

// PANZERS 0x5c7f90
// SUnitAnimation::GetGlobalStateStandText_From_Number: "<state>_stand", or
// "" when the prototype has no such state (vehicles without "_stand"
// sequences).
void SUnitAnimation::GetStandText(SHdStr* out, const int* state)
{
    int s = *state;
    if (s >= Proto->StateNames.Count) {
        HdStrSet(out, "", 0);
        return;
    }
    if (s < 0)
        Logger.g->Panic("SUnitAnimation::GetGlobalStateStandText_From_Number");
    HdStrFormat(out, "%s_stand", Proto->StateName(s));
}

// PANZERS 0x5c7d90
void SUnitAnimation::GetChangingText(SHdStr* out)
{
    int cur = UnitField<int>(Unit, kUnitGlobalState);
    int next = UnitField<int>(Unit, kUnitNextState);
    if (cur == next) {
        HdStrSet(out, "", 0);
        return;
    }
    int n = Proto->StateNames.Count;
    if (n <= cur) {
        HdStrSet(out, "", 0);
        return;
    }
    if (cur < 0)
        Logger.g->Panic("SUnitAnimation::GetGlobalStateChangingText - bad GlobalState.ActiveState");
    if (next < 0 || next >= n)
        Logger.g->Panic("SUnitAnimation::GetGlobalStateChangingText - bad GlobalState.NextState");
    HdStrFormat(out, "%s_to_%s", Proto->StateName(next), Proto->StateName(cur));
}

// PANZERS 0x5c8050
const char* SUnitAnimation::MoveText()
{
    static SHdStr s_text;                                          // DAT_00929a1c
    int state = UnitField<int>(Unit, kUnitGlobalState);
    HdStrFormat(&s_text, "%s_move%s", Proto->StateName(state), SubState(Unit));
    return HdStr(s_text);
}

// PANZERS 0x5c8270
const char* SUnitAnimation::StandText()
{
    static SHdStr s_text;                                          // DAT_00929a10
    int state = UnitField<int>(Unit, kUnitGlobalState);
    HdStrFormat(&s_text, "%s_stand%s", Proto->StateName(state), SubState(Unit));
    return HdStr(s_text);
}

// PANZERS 0x5c7c80
int SUnitAnimation::StateChangeTicks()
{
    SHdStr t = { nullptr, 0 };
    GetChangingText(&t);
    int ticks = RoundHdTicks(AnimModelSequenceLength(Model(), HdStr(t)));   // +0x84
    HdStrFree(&t);
    if (ticks == 0) {
        // The stand sequence's blend time (+0x88): fmul 20.0, fstp dword, cvttss2si.
        GetStandText(&t, &UnitField<int>(Unit, kUnitGlobalState));
        float bt = Model()->GetSequenceBlendTime(HdStr(t));       // +0x88 0x6d8010
        ticks = _mm_cvtt_ss2si(_mm_mul_ss(_mm_set_ss(bt), _mm_set_ss(20.0f)));
        HdStrFree(&t);
    }
    return _mm_cvtt_ss2si(_mm_mul_ss(_mm_set_ss((float)ticks), _mm_set_ss(SpeedFactor)));   // cvtdq2ps, mulss, cvttss2si
}

// PANZERS 0x5cae80
// Play the stand sequence of the unit's global state (called by the unit
// when its global state is set, SUnit 0x5b7390) and restart the idle timer.
void SUnitAnimation::PlayGlobalStand(bool blend)
{
    int state = UnitField<int>(Unit, kUnitGlobalState);
    if (state >= Proto->StateNames.Count)
        return;
    SHdStr t = { nullptr, 0 };
    HdStrFormat(&t, "%s_stand%s", Proto->StateName(state), SubState(Unit));
    Model()->PlaySequence(HdStr(t), blend);                       // +0x6c
    HdStrFree(&t);
    unsigned r = UnitAnimNextSeed();
    Timer = 0x3c - (int)((double)((r >> 16) & 0x7fff) * -3.0517578125e-05 * 60.0);
}

// ---------------------------------------------------------------------------
// SWalkerAnimation (0x34, vftable 0x7fd94c)

SWalkerAnimation::SWalkerAnimation(SPWalkerAnimation* proto, SIUnit* unit)
    : SUnitAnimation(proto, unit)
{
    Unit = unit;
    WProto = proto;
    Type = 2;
    RelaxTimer = 0;
    Guns = nullptr;
    Oar = nullptr;
}

static void FreeGuns(SAnimGun*& guns, SIUnit* unit)
{
    if (!guns)
        return;
    int n = PUnitField<unsigned char>(PUnitOf(unit), kPUnitGunCount);
    for (int i = 0; i < n; ++i) {
        free(guns[i].S);
        free(guns[i].M);
    }
    free(guns);
    guns = nullptr;
}

// PANZERS 0x5c6f50
SWalkerAnimation::~SWalkerAnimation()
{
    if (Oar) {
        Oar->Release();
        Oar = nullptr;
    }
    FreeGuns(Guns, Unit);
}

// Gun nodes "gun%dh", "gun%dv", "gun%ds%d", "gun%dm%d" (0x5c93a0, 0x5ca110).
static int CollectNodes(SIModel* model, const char* fmt, int gun, int** out)
{
    int n = 0;
    char name[64];
    for (;;) {
        _snprintf(name, sizeof(name) - 1, fmt, gun, n);
        name[sizeof(name) - 1] = 0;
        if (model->FindNode(name) < 0)
            break;
        ++n;
    }
    *out = (int*)malloc((size_t)n * 4);
    for (int i = 0; i < n; ++i) {
        _snprintf(name, sizeof(name) - 1, fmt, gun, i);
        name[sizeof(name) - 1] = 0;
        (*out)[i] = model->FindNode(name);
    }
    return n;
}

SAnimGun* UnitAnimLoadGuns(SAnimGun*& guns, SIUnit* unit, SIModel* model, bool warnNoMuzzle)
{
    int count = PUnitField<unsigned char>(PUnitOf(unit), kPUnitGunCount);
    if (count == 0)
        return guns;
    FreeGuns(guns, unit);
    guns = (SAnimGun*)malloc((size_t)count * sizeof(SAnimGun));
    for (int g = 0; g < count; ++g) {
        char name[64];
        _snprintf(name, sizeof(name) - 1, "gun%dh", g);
        name[sizeof(name) - 1] = 0;
        guns[g].H = model->FindNode(name);                        // +0x40
        _snprintf(name, sizeof(name) - 1, "gun%dv", g);
        guns[g].V = model->FindNode(name);
        guns[g].SCount = CollectNodes(model, "gun%ds%d", g, &guns[g].S);
        guns[g].MCount = CollectNodes(model, "gun%dm%d", g, &guns[g].M);
        if (warnNoMuzzle && guns[g].MCount == 0)
            Logger.g->Warning("SVehicleAnimation::InitModel: No gun%dm0 in unit %s", g,
                              HdStr(PUnitField<SHdStr>(PUnitOf(unit), kPUnitName)));
    }
    return guns;
}

// PANZERS 0x5ca110
void SWalkerAnimation::InitModel(SIModel* model)
{
    PZ_M2_TRACE("SWalkerAnimation::InitModel (0x5ca110)");
    SHdStr t = { nullptr, 0 };
    GetStandText(&t, &UnitField<int>(Unit, kUnitGlobalState));
    Model()->PlaySequence(HdStr(t), false);                       // +0x6c
    HdStrFree(&t);
    unsigned r = UnitAnimNextSeed();
    Timer = 0x3c - (int)((double)((r >> 16) & 0x7fff) * -3.0517578125e-05 * 60.0);
    // HD: model +0xcc(WProto->ShadowTexture), the blob shadow decal. The
    // slot is a stub in pzmodel.cpp (Slot_CC), not called.
    Model()->SetFlags(5);                                         // +0x94: interpolate pose, accumulate time
    int guns = PUnitField<unsigned char>(PUnitOf(Unit), kPUnitGunCount);
    if (guns != 0) {
        UnitAnimLoadGuns(Guns, Unit, model, false);
        if (1 < guns) {
            // The second gun of a walker (grenades and the other thrown
            // items) fires from the right arm: HD 0x5ca594..0x5ca5d8 replace
            // the gun's muzzle list (+0x24 = Guns[1].M, +0x2c = MCount).
            // (HD quirk not kept: its "gun%ds%d" loop stores every gun's
            // recoil nodes into Guns[0].S, 0x5ca43d; walkers never read S.)
            free(Guns[1].M);
            Guns[1].M = (int*)malloc(4);
            Guns[1].M[0] = model->FindNode("R Arm03");            // +0x40
            Guns[1].MCount = 1;
        }
    }
}

// PANZERS 0x5c7ea0
float SWalkerAnimation::GetStateMoveSpeed(int state)
{
    if (state >= 0 && state < WProto->States.Count)
        return WProto->States.Data[state].MoveSpeed;
    return 0.0f;
}

// PANZERS 0x5c7f60
float SWalkerAnimation::GetStateTurnSpeed(int state)
{
    if (state >= 0 && state < WProto->States.Count)
        return WProto->States.Data[state].SpinSpeed;
    return 0.0f;
}

// PANZERS 0x5c8260
int SWalkerAnimation::GetShadowTexture()
{
    return WProto->ShadowTexture;
}

// PANZERS 0x5cafc0
// Walk cycle by distance: the sequence advances by the distance of the tick
// (unit +0xc8), backwards when the unit moves against its facing.
void SWalkerAnimation::AdvanceByDistance()
{
    SIUnit* u = Unit;
    const float* pos = &UnitField<float>(u, kUnitPos);
    const float* prev = &UnitField<float>(u, kUnitPrevPos);
    float dx = pos[0] - prev[0];
    float dz = pos[2] - prev[2];
    float heading = (float)DAtan2((double)dx, (double)dz);      // 0x78d07a, fstp qword
    double d = fabs((double)UnitField<float>(u, kUnitPrevDir) - (double)heading);
    if (d > 3.1415927410125732)
        d = 6.2831854820251465 - d;
    float dd = (float)d;
    float speed = UnitField<float>(u, kUnitMoveSpeed);
    if (!(1.5707963705062866 > (double)dd))
        speed = -speed;
    Model()->AdvanceAnimationByDistance(TimeFactor() * speed);  // +0x74
}

// PANZERS 0x5cb0a0
void SWalkerAnimation::ResetRelax()
{
    unsigned r = UnitAnimNextSeed();
    RelaxTimer = (Timer - (int)((double)((r >> 16) & 0x7fff) * -3.0517578125e-05 * 100.0)) + 200;
}

static bool SubStateIs(SIUnit* u, const char* s)                  // 0x52c410 on unit +0x25c
{
    const SHdStr& sub = UnitField<SHdStr>(u, kUnitSubState);
    if (sub.len == 0)
        return !s || !*s;
    return s && *s && _stricmp(sub.buf, s) == 0;
}

// The death sequence of a walker (the unit +0x155 branch of 0x5ce2a0).
static void WalkerStartDie(SWalkerAnimation* a)
{
    SIUnit* u = a->Unit;
    SIModel* m = a->Model();
    SPWalkerAnimation* wp = a->WProto;
    UnitField<bool>(u, kUnitStartDie) = false;
    UnitField<bool>(u, kUnitDying) = true;
    int state = UnitField<int>(u, kUnitGlobalState);
    const char* sub = SubState(u);
    SHdStr name = { nullptr, 0 };
    {
        if (state < 0 || state >= wp->States.Count)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SWalkerStateInfo", state);
        int count = wp->States.Data[state].DieCount;
        unsigned r = UnitAnimNextSeed();
        HdStrFormat(&name, "%s_die%d%s", wp->StateName(state),
                    1 - (int)((double)((r >> 16) & 0x7fff) * -3.0517578125e-05 * (double)count), sub);
    }
    if (AnimModelSequenceLength(m, HdStr(name)) == 0.0f) {
        if (wp->States.Count < 1 || wp->StateNames.Count < 1)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SString", 0);
        int count = wp->States.Data[0].DieCount;
        HdStrFormat(&name, "%s_die%d%s", wp->StateName(0), UnitAnimRand(count) + 1, sub);
    }
    const char* play = HdStr(name);
    SHdStr alt = { nullptr, 0 };
    if (UnitField<bool>(u, kUnitDieBoom) || UnitField<bool>(u, kUnitDieFire)) {
        bool boom = UnitField<bool>(u, kUnitDieBoom);
        SWalkerStateInfo& s = wp->States.Data[state];
        HdStrFormat(&alt, boom ? "%s_die_boom%d%s" : "%s_die_fire%d%s", wp->StateName(state),
                    UnitAnimRand(boom ? s.DieBoomCount : s.DieFireCount) + 1, sub);
        if (AnimModelSequenceLength(m, HdStr(alt)) == 0.0f)
            HdStrSet(&alt, HdStr(name));
        play = HdStr(alt);
    }
    m->PlaySequence(play, true);
    UnitField<int>(u, kUnitDieTicks) = RoundHdTicks(AnimModelSequenceLength(m, play));
    HdStrFree(&alt);
    HdStrFree(&name);
    int& t = UnitField<int>(u, kUnitDieTicks);
    if (t == 0)
        t = 1;
    else
        t += 100;
    UnitStartDeathEffects(u);
    // HD: model +0xcc(-1) drops the blob shadow (Slot_CC, not called).
    m->AdvanceAnimation(kTick);
}

// PANZERS 0x5ce2a0
void SWalkerAnimation::UpdateModel()
{
    PZ_M2_TRACE("SWalkerAnimation::UpdateModel (0x5ce2a0)");
    SIUnit* u = Unit;
    SIModel* m = Model();
    SUnitAnimEnv& e = g_UnitAnimEnv;
    TickStateChange(u);

    // Visibility: the soldier's own, or that of the unit carrying it.
    SIModel* marker = UnitField<SIModel*>(u, kUnitMarker);
    int carrier = UnitField<int>(u, kUnitVehicle);
    if (carrier < 0) {
        if (UnitField<bool>(u, kUnitAlwaysShown) ||
            (!UnitField<bool>(u, kUnitUnplaced) && SeenByLocalPlayer(u))) {
            m->SetVisible(true, false);                           // +0x30(1, 0)
            if (marker)
                marker->SetVisible(true, false);
        } else {
            m->SetVisible(false, false);
            if (marker)
                marker->SetVisible(false, false);
        }
    } else {
        SIUnit* v = e.GetUnit(carrier);
        SIModel* vmarker = UnitField<SIModel*>(v, kUnitMarker);
        int cls = ClassOf(v);
        if (cls == 5) {
            if (!UnitField<bool>(v, kUnitUnplaced) && SeenByLocalPlayer(v)) {
                m->SetVisible(true, true);
                if (vmarker)
                    vmarker->SetVisible(true, false);
            } else {
                m->SetVisible(false, true);
                if (vmarker)
                    vmarker->SetVisible(false, false);
            }
        } else if (cls == 9) {
            if (UnitField<bool>(v, kUnitUnplaced) || !SeenByLocalPlayer(v)) {
                m->SetVisible(false, false);
                if (vmarker)
                    vmarker->SetVisible(false, false);
            } else {
                m->SetVisible(true, false);
                if (vmarker)
                    vmarker->SetVisible(true, false);
            }
        } else {
            if ((!UnitField<bool>(u, kUnitUnplaced) || UnitField<bool>(u, kUnitAlwaysShown)) &&
                SeenByLocalPlayer(v)) {
                m->SetVisible(true, false);
                if (marker)
                    marker->SetVisible(true, false);
            } else {
                m->SetVisible(false, false);
                if (marker)
                    marker->SetVisible(false, false);
            }
        }
    }

    // Height: on the ground when the active driver is the first one and not
    // a parachute (SPDriver type 0xc); in a building, its floor height.
    float* pos = &UnitField<float>(u, kUnitPos);
    carrier = UnitField<int>(u, kUnitVehicle);
    if (carrier >= 0 && ClassOf(e.GetUnit(carrier)) == 9) {
        pos[1] = UnitField<float>(e.GetUnit(carrier), kUnitBuildingY);
    } else {
        int d = UnitField<int>(u, kUnitActiveDriver);
        if (d >= 0) {
            if (d >= UnitField<int>(u, kUnitDriverCount))
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SDriver *", d);
            SIDriver* drv = UnitField<SIDriver**>(u, kUnitDrivers)[d];
            SIPDriver* pd = drv->GetPDriver();                    // driver +0x04
            int ptype = *reinterpret_cast<int*>(reinterpret_cast<unsigned char*>(pd) + 4);
            if (ptype != 0xc && UnitField<int>(u, kUnitActiveDriver) == 0)
                pos[1] = e.TerrainHeight(pos[0], pos[2]);         // 0x5e7730
        }
    }

    if (UnitField<bool>(u, kUnitDying)) {
        int t = UnitField<int>(u, kUnitDieTicks);
        if (t < 0x32) {
            pos[1] = pos[1] - (float)(0x32 - t) * 0.003f;         // sink into the ground
            m->SetPosition(pos[0], pos[1], pos[2]);
        }
        m->AdvanceAnimation(kTick);
        return;
    }
    m->SetPosition(pos[0], pos[1], pos[2]);                       // +0x18
    m->SetRotation(UnitField<float>(u, kUnitDir), 0.0f, 0.0f);    // +0x1c
    if (SIModel* m2 = UnitField<SIModel*>(u, kUnitModel2)) {
        m2->SetPosition(pos[0], pos[1], pos[2]);
        m2->SetRotation(UnitField<float>(u, kUnitDir) + kPi, 0.0f, 0.0f);
    }
    if (UnitField<SHdStr>(u, kUnitSubState).len != 0 &&
        UnitField<int>(u, kUnitGlobalState) != WProto->FindState("vehicle"))
        Logger.g->Warning("SWalkerAnimation::UpdateModel() - Unit->SubStateString is not empty");   // HD panics

    if (UnitField<bool>(u, kUnitStartDie)) {
        WalkerStartDie(this);
        return;
    }

    int& brk = UnitField<int>(u, kUnitBreakTicks);
    if (brk >= 1) {
        // A forced pause (unit +0x2f0 ticks): keep the current sequence.
        --brk;
        int type = AnimModelSequenceType(m);                      // +0x78
        if (type == 1) {
            m->AdvanceAnimation(TimeFactor() * kTick);
            return;
        }
        if (type == 2) {
            if (UnitField<int>(u, kUnitGlobalState) != WProto->FindState("vehicle") ||
                strstr(SubState(u), "_ground")) {
                AdvanceByDistance();
                return;
            }
            m->AdvanceAnimation(TimeFactor() * kTick);
            return;
        }
        m->AdvanceAnimation(kTick);
        return;
    }
    if (UnitField<bool>(u, kUnitStartChange) && brk == 0) {
        // Start of a global state change: "<next>_to_<cur>".
        UnitField<bool>(u, kUnitStartChange) = false;
        UnitField<bool>(u, kUnitStateChanging) = true;
        UnitUpdateMoveMode(u);
        SHdStr t = { nullptr, 0 };
        GetChangingText(&t);
        int ticks = RoundHdTicks(AnimModelSequenceLength(m, HdStr(t)), SpeedFactor);
        if (ticks < 1)
            m->PlaySequence(StandText(), true);
        else
            m->PlaySequence(HdStr(t), true);
        HdStrFree(&t);
        Timer = UnitAnimRand(0x28) + 0x28 + ticks;
        m->AdvanceAnimation(kTick);
        return;
    }
    if (UnitField<bool>(u, kUnitStartFight)) {
        UnitField<bool>(u, kUnitStartFight) = false;
        int state = UnitField<int>(u, kUnitGlobalState);
        const char* sub = SubState(u);
        SHdStr name = { nullptr, 0 };
        int slot = UnitField<int>(u, kUnitGunnerSlot);
        bool found = false;
        if (0 < slot) {
            HdStrFormat(&name, "%s_gunner%d_fight%d%s", WProto->StateName(state), slot,
                        UnitAnimRand(WProto->States.Data[state].FightCount) + 1, sub);
            found = AnimModelSequenceLength(m, HdStr(name)) != 0.0f;
        }
        if (!found)
            HdStrFormat(&name, "%s_fight%d%s", WProto->StateName(state),
                        UnitAnimRand(WProto->States.Data[state].FightCount) + 1, sub);
        m->PlaySequence(HdStr(name), true);
        int len = RoundHdTicks(AnimModelSequenceLength(m, HdStr(name)));
        Timer = UnitAnimRand(0x14) + len + 0x3c;
        ResetRelax();
        m->AdvanceAnimation(kTick);
        HdStrFree(&name);
        return;
    }
    SHdStr& forced = UnitField<SHdStr>(u, kUnitForcedSeq);
    if (forced.len != 0) {
        m->PlaySequence(HdStr(forced), true);
        int len = RoundHdTicks(AnimModelSequenceLength(m, HdStr(forced)));
        Timer = UnitAnimRand(0x3c) + len + 10;
        // 0x5c7c30 clears the unit's SString (HD frees the buffer; the
        // buffer belongs to the unit's allocator, so only the text is cut).
        forced.buf[0] = 0;
        forced.len = 0;
        m->AdvanceAnimation(kTick);
        return;
    }

    // Movement mode (unit +0xf8) against the type of the playing sequence.
    int mode = UnitField<int>(u, kUnitMoveMode);
    int type = AnimModelSequenceType(m);
    if (mode == 2 && type != 2) {
        m->PlaySequence(MoveText(), true);
        type = AnimModelSequenceType(m);
    }
    if ((mode == 4 && type != 4) || (mode == 5 && type != 5)) {
        int state = UnitField<int>(u, kUnitGlobalState);
        SHdStr name = { nullptr, 0 };
        HdStrFormat(&name, mode == 4 ? "%s_spin_right%s" : "%s_spin_left%s", WProto->StateName(state), SubState(u));
        if (AnimModelSequenceLength(m, HdStr(name)) == 0.0f)
            HdStrSet(&name, StandText());
        m->PlaySequence(HdStr(name), true);
        type = AnimModelSequenceType(m);
        HdStrFree(&name);
    }
    if (mode == 1 && 1 < type) {
        m->PlaySequence(StandText(), true);
        type = AnimModelSequenceType(m);
        Timer = UnitAnimRand(0x3c) + 0x3c;
    }
    {
        // Recompile-only: PZ_M2_ANIMTRACE=<unit index> logs this walker's timers per tick.
        static int s_Trace = -2;
        if (s_Trace == -2) {
            const char* t = getenv("PZ_M2_ANIMTRACE");
            s_Trace = t ? atoi(t) : -1;
        }
        if (s_Trace >= 0 && UnitField<int>(u, 0x74) == s_Trace && Logger.g)
            Logger.g->Log(0, "PZM2 ANIM f%d u%d mode %d type %d timer %d relax %d brk %d", e.Frame(), s_Trace, mode, type,
                          Timer, RelaxTimer, brk);
    }
    if (type == 1) {
        --Timer;
        if (0 < RelaxTimer)
            --RelaxTimer;
        if (Timer < 1) {
            int state = UnitField<int>(u, kUnitGlobalState);
            if (state < 0 || state >= WProto->States.Count)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SWalkerStateInfo", state);
            const SWalkerStateInfo& s = WProto->States.Data[state];
            const char* sub = SubState(u);
            SHdStr idle = { nullptr, 0 };
            HdStrFormat(&idle, "%s_idle%d%s", WProto->StateName(state), UnitAnimRand(s.IdleCount) + 1, sub);
            if (AnimModelSequenceLength(m, HdStr(idle)) == 0.0f)
                HdStrSet(&idle, StandText());
            if (RelaxTimer == 0) {
                SHdStr relax = { nullptr, 0 };
                HdStrFormat(&relax, "%s_idle_relax%d%s", WProto->StateName(state), UnitAnimRand(s.RelaxCount) + 1, sub);
                if (AnimModelSequenceLength(m, HdStr(relax)) == 0.0f)
                    HdStrSet(&relax, HdStr(idle));
                m->PlaySequence(HdStr(relax), true);
                int len = RoundHdTicks(AnimModelSequenceLength(m, HdStr(relax)));
                Timer = UnitAnimRand(0x3c) + len + 0x14;
                HdStrFree(&relax);
            } else {
                m->PlaySequence(HdStr(idle), true);
                int len = RoundHdTicks(AnimModelSequenceLength(m, HdStr(idle)));
                Timer = UnitAnimRand(0x3c) + len + 0x14;
            }
            HdStrFree(&idle);
        }
        m->AdvanceAnimation(TimeFactor() * kTick);
    } else if (type != 2) {
        m->AdvanceAnimation(kTick);
    } else if (UnitField<int>(u, kUnitGlobalState) == WProto->FindState("vehicle") &&
               !strstr(SubState(u), "_ground")) {
        m->AdvanceAnimation(TimeFactor() * kTick);
    } else {
        AdvanceByDistance();
    }

    // Boat oars (sub-states "_leftboat" / "_rightboat"): not in the menu.
    if (SubStateIs(u, "_leftboat") || SubStateIs(u, "_rightboat")) {
        // HD: scene +0x58(oar prototype, 1), then oar +0xdc(model, node
        // "R Arm03" / "L Arm03") attaches it. SIModel +0xdc is a Slot_ stub.
        STUB_LOG("SWalkerAnimation::UpdateModel: boat oars (SIModel +0xdc attach) not lifted");
    } else if (Oar) {
        Oar->Release();
        Oar = nullptr;
    }
}

// ---------------------------------------------------------------------------
// SSquadAnimation (0x2c, vftable 0x7fda6c)

SSquadAnimation::SSquadAnimation(SPSquadAnimation* proto, SIUnit* unit)
    : SUnitAnimation(proto, unit)
{
    Unit = unit;
    SProto = proto;
    Type = 6;
    Extra = nullptr;
}

// PANZERS 0x5c73f0
SSquadAnimation::~SSquadAnimation()
{
    if (Extra) {
        Extra->Release();
        Extra = nullptr;
    }
}

// PANZERS 0x5c9340
void SSquadAnimation::InitModel(SIModel* model)
{
    (void)model;
}

// Ground slopes over 1 m around a point (0x5cc8c0, 0x5cd020).
static void TerrainSlopes(float x, float z, float* sx, float* sz)
{
    SUnitAnimEnv& e = g_UnitAnimEnv;
    float a = e.TerrainHeight((float)((double)x - 0.5), z);
    float b = e.TerrainHeight((float)((double)x + 0.5), z);
    float c = e.TerrainHeight(x, (float)((double)z - 0.5));
    float d = e.TerrainHeight(x, (float)((double)z + 0.5));
    *sx = a - b;
    *sz = c - d;
}

// PANZERS 0x5cc8c0
void SSquadAnimation::UpdateModel()
{
    PZ_M2_TRACE("SSquadAnimation::UpdateModel (0x5cc8c0)");
    SIUnit* u = Unit;
    SIModel* m = Model();
    if (!UnitField<bool>(u, kUnitUnplaced) && SeenByLocalPlayer(u)) {
        m->SetVisible(true, true);
        UnitAnimSpottedCheck(u);
    } else {
        m->SetVisible(false, true);
        UnitField<bool>(u, kUnitSpotted) = false;
    }
    float* pos = &UnitField<float>(u, kUnitPos);
    float sx, sz;
    TerrainSlopes(pos[0], pos[2], &sx, &sz);
    pos[1] = g_UnitAnimEnv.TerrainHeight(pos[0], pos[2]);
    m->SetPosition(pos[0], pos[1], pos[2]);
    m->SetRotation(UnitField<float>(u, kUnitDir) + kPi, sx, sz);
    if (SIModel* m2 = UnitField<SIModel*>(u, kUnitModel2)) {
        m2->SetPosition(pos[0], pos[1], pos[2]);
        m2->SetRotation(UnitField<float>(u, kUnitDir) + kPi, 0.0f, 0.0f);
    }
    if (UnitField<bool>(u, kUnitStartDie)) {
        UnitField<int>(u, kUnitDieTicks) = 1;
        UnitField<bool>(u, kUnitStartDie) = false;
        UnitStartDeathEffects(u);
    }
    // HD then sets unit +0x2e8 = (!unit +0x304 && unit 0x5ba820(7) &&
    // !blockmap 0x5d9e00(x, z, 1, 0x200)): unit and block-map queries of
    // agents U and P, not lifted here; +0x2e8 is left to the unit.
    PZ_M2_TRACE("SSquadAnimation::UpdateModel: unit +0x2e8 (0x5ba820 / 0x5d9e00) not lifted");
}

// ---------------------------------------------------------------------------
// SBuildingAnimation (0x30, vftable 0x7fdab4)

SBuildingAnimation::SBuildingAnimation(SPBuildingAnimation* proto, SIUnit* unit)
    : SUnitAnimation(proto, unit)
{
    Unit = unit;
    BProto = proto;
    Type = 7;
    RoofNode = -1;
    InteriorNode = -1;
}

// PANZERS 0x5c7100
SBuildingAnimation::~SBuildingAnimation()
{
}

// PANZERS 0x5c8c20
void SBuildingAnimation::InitModel(SIModel* model)
{
    (void)model;
    SIUnit* u = Unit;
    SIModel* m = Model();
    m->SetFlags(0);                                               // +0x94
    RoofNode = m->FindNode("teto");
    InteriorNode = m->FindNode("belso");
    float* pos = &UnitField<float>(u, kUnitPos);
    pos[1] = g_UnitAnimEnv.TerrainHeight(pos[0], pos[2]) + UnitField<float>(u, kUnitYRel);
    m->SetRotation(UnitField<float>(u, kUnitDir), 0.0f, 0.0f);
    m->SetPosition(pos[0], pos[1], pos[2]);
}

// SBuildingUnit 0x5468e0: does a unit of the player's team occupy the building.
static bool BuildingOccupiedByTeam(SIUnit* b, int player)
{
    return g_UnitAnimEnv.BuildingOccupiedByTeam(b, player);
}

// PANZERS 0x5cb650
void SBuildingAnimation::UpdateModel()
{
    PZ_M2_TRACE("SBuildingAnimation::UpdateModel (0x5cb650)");
    SIUnit* u = Unit;
    SIModel* m = Model();
    SUnitAnimEnv& e = g_UnitAnimEnv;
    bool isBuilding = ClassOf(u) == 9;
    bool tint = false;
    unsigned color = 0;
    bool setColor = true;
    if (isBuilding && BuildingOccupiedByTeam(u, e.LocalPlayer())) {
        AnimModelSetNodeFade(m, false, RoofNode, InteriorNode);    // +0x38(0, roof, interior)
    } else {
        AnimModelSetNodeFade(m, true, RoofNode, InteriorNode);     // +0x38(1, roof, interior)
        // Fog of war tint, only on the local player's frame
        // (SGameLogic +0x234[player] == +0x08) with fog mode +0x264 == 1.
        if (!e.HasGameLogic() || e.GameLogicInt(0x234 + e.LocalPlayer() * 4) != e.GameLogicInt(0x08)) {
            setColor = false;
        } else if (e.GameLogicInt(0x264) == 1 && !e.CanSeeGroundUnit(e.LocalPlayer(), u)) {
            tint = true;
            color = 0xa0a0a0;
        }
    }
    if (setColor)
        AnimModelSetColor2(m, tint, color);                       // +0xf0
    if (isBuilding) {
        if (SIModel* bm = UnitField<SIModel*>(u, kUnitBuildingModel))
            bm->AdvanceAnimation(kTick);
    }
    if (UnitField<bool>(u, kUnitStartDie)) {
        UnitField<int>(u, kUnitDieTicks) = 1;
        UnitField<bool>(u, kUnitStartDie) = false;
        UnitStartDeathEffects(u);
    }
}

} // namespace pz
