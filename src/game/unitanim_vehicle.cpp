// src/game/unitanim_vehicle.cpp
// SVehicleAnimation / SPVehicleAnimation (0x5c6610, 0x5c68e0, 0x5c85c0,
// 0x5c93a0, 0x5ca690, 0x5cd020) and the running gear SPRunningGear /
// SRunningGear (0x5a9c70..0x5aa900). OWNER: agent A.
//
// Running gear, per 20 Hz tick (0x5aa2c0): the two track contact points sit
// TrackWidth to the left and right of the unit. Each moved some distance
// along the unit's facing since the last tick (projected on the facing,
// times 200 = world -> .4d units), so a turn in place moves the tracks in
// opposite directions. That distance
//   - scrolls the belt texture of "cpbelt0l" / "cpbelt0r" by
//     -TrailTextureMovement * 0.01 per unit (texture animation 1, SModel
//     +0x68), wrapped to [-0.5, 0.5];
//   - turns every wheel type by -distance / Radius: type 0 rotates the
//     texture of "wheel%dl/r" about (U, 1 - V) (texture animation 2, +0x64),
//     types 1 and 2 rotate the wheel nodes (+0x4c; type 2 also steers by
//     the unit's +0xd4 * 0.5, the right side mirrored).
// The scene interpolates both between ticks (SModel flag 2, prev values
// stored by StoreInterpolationState).

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unitanim.h"
#include "unitanim_model.h"
#include "iunit.h"
#include "pz/imodel.h"
#include "pz/igepardhd.h"
#include "pz/ipixie.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

PZ_HD_SIZE(SPRunningGear, 0x24);
PZ_HD_SIZE(SRunningGear, 0x38);
PZ_HD_SIZE(SWheelType, 0x10);
PZ_HD_SIZE(SWheelState, 0x20);

static const float kPi = 3.1415927410125732f;                    // 0x7f4584

void UnitAnimSpottedCheck(SIUnit* u);                            // unitanim.cpp
SAnimGun* UnitAnimLoadGuns(SAnimGun*& guns, SIUnit* unit, SIModel* model, bool warnNoMuzzle);

// ---------------------------------------------------------------------------
// SPRunningGear (0x24)

// PANZERS 0x5a9c70
SPRunningGear::SPRunningGear()
{
    TrailTexture.buf = nullptr;
    TrailTexture.len = 0;
    WheelTypeCount = 0;
    WheelTypes = nullptr;
    TrackWidth = 0.0f;
    TrailWidth = 0.0f;
    TrailTextureMovement = 0.0f;
    Caterpillar = false;
}

SPRunningGear::~SPRunningGear()
{
    HdStrFree(&TrailTexture);
    free(WheelTypes);
    WheelTypes = nullptr;
}

// PANZERS 0x5aa030
void SPRunningGear::Load(const SAnimProps& p)
{
    TrackWidth = (float)(p.GetFloat("TrackWidth") * 0.005f);
    if (0 < p.GetMultiIndex("CaterpillarBelt")) {
        SAnimProps belt = p.GetMultiSub("CaterpillarBelt");
        HdStrSet(&TrailTexture, belt.GetString("Trail"));
        TrailWidth = (float)(belt.GetFloat("TrailWidth") * 0.005f);
        TrailTextureMovement = belt.GetFloat("TrailTextureMovement");
        Caterpillar = true;
    }
    WheelTypeCount = p.GetArraySize("WheelTypes");
    WheelTypes = (SWheelType*)calloc((size_t)(WheelTypeCount > 0 ? WheelTypeCount : 1), sizeof(SWheelType));
    for (int i = 0; i < WheelTypeCount; ++i) {
        SAnimProps w = p.GetArrayItem("WheelTypes", i);
        WheelTypes[i].Type = w.GetEnum("Type");
        WheelTypes[i].InvRadius = (float)(1.0f / w.GetFloat("Radius"));
        WheelTypes[i].U = w.GetFloat("U");
        WheelTypes[i].V = (float)(1.0f - w.GetFloat("V"));
    }
}

// ---------------------------------------------------------------------------
// SRunningGear (0x38)

// PANZERS 0x5a9cc0
// SRunningGear::Init. HD also creates the two ground trails of a tracked
// vehicle here (texture Trail, scene +0x8c(tex, 1.0, 30000.0, w, w, 0) with
// w = TrailWidth * 0.5) and feeds them every tick (scene +0x90); the SScene
// trail slots and SScene::DrawTrails 0x6acf20 are stubs, so the trails stay
// -1 and are not drawn.
SRunningGear::SRunningGear(SPRunningGear* proto)
{
    LeftBelt = -1;
    LeftScroll = 0.0f;
    RightBelt = -1;
    RightScroll = 0.0f;
    HasPrev = false;
    Proto = proto;
    int n = proto->WheelTypeCount;
    Wheels = (SWheelState*)calloc((size_t)(n > 0 ? n : 1), sizeof(SWheelState));
    LeftPrev[0] = LeftPrev[1] = -1.0f;
    RightPrev[0] = RightPrev[1] = -1.0f;
    LeftTrail = -1;
    RightTrail = -1;
    if (proto->Caterpillar && proto->TrailTexture.len != 0)
        PZ_M2_TRACE("SRunningGear::Init: ground trails (SScene +0x8c / +0x90) not drawn");
}

// PANZERS 0x5a9f90
SRunningGear::~SRunningGear()
{
    // HD: scene +0x94 releases the trails (never created here).
    free(Wheels);
    Wheels = nullptr;
}

// PANZERS 0x5aa180
void SRunningGear::InitModel(SIModel* model)
{
    if (Proto->Caterpillar) {
        LeftBelt = model->FindNode("cpbelt0l");                   // +0x40
        RightBelt = model->FindNode("cpbelt0r");
    }
    for (int i = 0; i < Proto->WheelTypeCount; ++i) {
        char name[32];
        _snprintf(name, sizeof(name) - 1, "wheel%dl", i);
        name[sizeof(name) - 1] = 0;
        Wheels[i].LeftNode = model->FindNode(name);
        _snprintf(name, sizeof(name) - 1, "wheel%dr", i);
        name[sizeof(name) - 1] = 0;
        Wheels[i].RightNode = model->FindNode(name);
    }
}

// PANZERS 0x5aa2c0
void SRunningGear::Update(SIModel* model, float x, float z, double dir, float steer)
{
    double s = sin(dir);                                           // 0x78d640
    double c = cos(dir);                                           // 0x78d480
    // HD: with a caterpillar belt the trails get the track points here
    // (scene +0x90), see the constructor.
    float fs = (float)s;
    float fc = (float)c;
    float w = Proto->TrackWidth;
    float ws = w * fs;
    float wc = w * fc;
    float lz = ws + z;
    float lx = x - wc;
    float rx = wc + x;
    float rz = z - ws;
    if (HasPrev) {
        float dl = (float)((double)((lz - LeftPrev[1]) * fc + (lx - LeftPrev[0]) * fs) * 200.0);
        float dr = (float)((double)((rz - RightPrev[1]) * fc + (rx - RightPrev[0]) * fs) * 200.0);
        if (Proto->Caterpillar) {
            float t = LeftScroll - Proto->TrailTextureMovement * 0.01f * dl;
            LeftScroll = t - rintf(t);
            t = RightScroll - Proto->TrailTextureMovement * 0.01f * dr;
            RightScroll = t - rintf(t);
            AnimModelSetNodeTexScroll(model, LeftBelt, LeftScroll, 0.0f);     // +0x68
            AnimModelSetNodeTexScroll(model, RightBelt, RightScroll, 0.0f);
        }
        for (int i = 0; i < Proto->WheelTypeCount; ++i) {
            SWheelState& ws2 = Wheels[i];
            const SWheelType& wt = Proto->WheelTypes[i];
            ws2.LeftAngle = ws2.LeftAngle - (double)(wt.InvRadius * dl);
            ws2.RightAngle = ws2.RightAngle - (double)(wt.InvRadius * dr);
            if (wt.Type == 0) {
                AnimModelSetNodeTexRotation(model, ws2.LeftNode, wt.U, wt.V, (float)ws2.LeftAngle);    // +0x64
                AnimModelSetNodeTexRotation(model, ws2.RightNode, wt.U, wt.V, (float)ws2.RightAngle);
            } else {
                float ls = wt.Type == 1 ? 0.0f : steer;
                float rs = wt.Type == 1 ? 0.0f : -steer;
                AnimModelSetNodeRotation(model, ws2.LeftNode, 0.0f, 0.0f, 0.0f, (float)ws2.LeftAngle, ls);   // +0x4c
                AnimModelSetNodeRotation(model, ws2.RightNode, 0.0f, 0.0f, 0.0f, (float)-ws2.RightAngle, rs);
            }
        }
    }
    LeftPrev[0] = lx;
    RightPrev[0] = rx;
    LeftPrev[1] = lz;
    RightPrev[1] = rz;
    HasPrev = true;
}

// ---------------------------------------------------------------------------
// SPVehicleAnimation (0x3c, vftable 0x7fd73c)

// PANZERS 0x5c6610
SPVehicleAnimation::SPVehicleAnimation()
{
    RunningGear = nullptr;
    SpringStrength = SpringDecay = SpringScale = 0.0f;
    RodSpringStrength = RodSpringDecay = RodSpringScale = 0.0f;
    MaxSpringAngle = MaxSpringAngle2 = 0.0f;
    SmokeFx = -1;
    HeavySmokeFx = -1;
}

// PANZERS 0x5c72c0
SPVehicleAnimation::~SPVehicleAnimation()
{
    delete RunningGear;
    RunningGear = nullptr;
    SIPixie* px = g_UnitAnimEnv.Pixie ? g_UnitAnimEnv.Pixie() : nullptr;
    if (px && SmokeFx >= 0)
        px->ReleaseEffectPrototype(SmokeFx);                      // pixie +0x20
    if (px && HeavySmokeFx >= 0)
        px->ReleaseEffectPrototype(HeavySmokeFx);
}

// PANZERS 0x5c85c0
void SPVehicleAnimation::LoadProps(SIPUnit* punit, const SAnimProps& p)
{
    (void)punit;
    PZ_M2_TRACE("SPVehicleAnimation::Load (0x5c85c0)");
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

// PANZERS 0x5ca690
// The global states of a vehicle are the model's "<state>_stand" sequences;
// the two damage smoke effects.
void SPVehicleAnimation::LoadResourcesProps(SIPUnit* punit, const SAnimProps& props)
{
    (void)props;
    PZ_M2_TRACE("SPVehicleAnimation::LoadResources (0x5ca690)");
    int proto = punit ? PUnitField<int>(punit, kPUnitModelProto) : -1;
    int n = AnimProtoSequenceCount(proto);                        // Gepard +0x34
    for (int i = 0; i < n; ++i) {
        const char* name = AnimProtoSequenceName(proto, i);
        int len = (int)strlen(name);
        if (len < 6 || _stricmp(name + len - 6, "_stand") != 0)
            continue;
        int idx;
        {
            // SDArray::Add of the state names
            SHdDArray<SHdStr>& a = StateNames;
            if (a.Count == a.Max) {
                int m = a.Max < 0x10 ? 0x10 : (a.Max * 6) / 5;
                a.Data = (SHdStr*)realloc(a.Data, (size_t)m * sizeof(SHdStr));
                memset(a.Data + a.Max, 0, (size_t)(m - a.Max) * sizeof(SHdStr));
                a.Max = m;
            }
            idx = a.Count++;
        }
        HdStrSet(&StateNames.Data[idx], name, len - 6);
    }
    SIPixie* px = g_UnitAnimEnv.Pixie ? g_UnitAnimEnv.Pixie() : nullptr;
    if (px) {
        SmokeFx = px->LoadEffectPrototype("effects/smoke/Ground_Dark_Slow_Size2.fx", false, false, 0, 0);   // pixie +0x10
        HeavySmokeFx = px->LoadEffectPrototype("effects/smoke/Ground_Dark_Slow_Size3.fx", false, false, 0, 0);
    }
}

// PANZERS 0x5c7a30
SIUnitAnimation* SPVehicleAnimation::CreateAnimation(SIUnit* unit)
{
    PZ_M2_TRACE("SPVehicleAnimation::CreateAnimation (0x5c7a30)");
    return new SVehicleAnimation(this, unit);
}

// ---------------------------------------------------------------------------
// SVehicleAnimation (0xc0, vftable 0x7fd82c)

// PANZERS 0x5c68e0
SVehicleAnimation::SVehicleAnimation(SPVehicleAnimation* proto, SIUnit* unit)
    : SUnitAnimation(proto, unit)
{
    HookPos[0] = HookPos[1] = 0.0f;
    HoleFPos[0] = HoleFPos[1] = 0.0f;
    HoleRPos[0] = HoleRPos[1] = 0.0f;
    Type = 1;
    VProto = proto;
    Unit = unit;
    Gear = nullptr;
    BodyNode = -1;
    DriverNode = HookNode = HoleFNode = HoleRNode = -1;
    AntennaNode = -1;
    if (proto->RunningGear)
        Gear = new SRunningGear(proto->RunningGear);              // new 0x38
    AntennaReset = true;
    Spring[0] = Spring[1] = 0.0;
    SpringVel[0] = SpringVel[1] = 0.0;
    Rod[0] = Rod[1] = 0.0;
    RodVel[0] = RodVel[1] = 0.0;
    AntennaPrev[0] = AntennaPrev[1] = 0.0f;
    Guns = nullptr;
    _30 = 0;
    _44 = 0;
    _bc = 0;
    HeavySmokeEffect = -1;
    SmokeEffect = -1;
}

// PANZERS 0x5c6e30
SVehicleAnimation::~SVehicleAnimation()
{
    if (Guns) {
        int n = PUnitField<unsigned char>(UnitField<SIPUnit*>(Unit, kUnitPUnit), kPUnitGunCount);
        for (int i = 0; i < n; ++i) {
            free(Guns[i].S);
            free(Guns[i].M);
        }
        free(Guns);
        Guns = nullptr;
    }
    delete Gear;
    Gear = nullptr;
    SIPixie* px = g_UnitAnimEnv.Pixie ? g_UnitAnimEnv.Pixie() : nullptr;
    if (px && SmokeEffect >= 0)
        px->StopEffect(SmokeEffect);                              // pixie +0x34
    if (px && HeavySmokeEffect >= 0)
        px->StopEffect(HeavySmokeEffect);
}

// PANZERS 0x5c7c70
int SVehicleAnimation::GetDriverNode()
{
    return DriverNode;
}

static void NodeGroundPos(SIModel* model, int node, float* out)
{
    float p[3] = { 0.0f, 0.0f, 0.0f };
    AnimModelGetNodePosition(model, node, p);                     // +0x54
    out[0] = -p[0];
    out[1] = -p[2];
}

// PANZERS 0x5c93a0
void SVehicleAnimation::InitModel(SIModel* model)
{
    PZ_M2_TRACE("SVehicleAnimation::InitModel (0x5c93a0)");
    SIUnit* u = Unit;
    SHdStr t = { nullptr, 0 };
    GetStandText(&t, &UnitField<int>(u, kUnitGlobalState));
    Model()->PlaySequence(HdStr(t), false);                       // +0x6c
    HdStrFree(&t);
    Model()->SetFlags(7);                                         // +0x94: pose, nodes, time
    if (Gear)
        Gear->InitModel(model);
    DriverNode = model->FindNode("built0_driver");
    HookNode = model->FindNode("hook");
    if (HookNode >= 0)
        NodeGroundPos(model, HookNode, HookPos);
    HoleFNode = model->FindNode("hole_f");
    if (HoleFNode >= 0)
        NodeGroundPos(model, HoleFNode, HoleFPos);
    HoleRNode = model->FindNode("hole_r");
    if (HoleRNode >= 0)
        NodeGroundPos(model, HoleRNode, HoleRPos);

    // Crew places "man%d_<kind>", the first kind found.
    Slots.Count = 0;                                              // 0x5c7630(0)
    if (Slots.Data)
        memset(Slots.Data, 0, (size_t)Slots.Max * sizeof(SAnimNodeSlot));
    static const char* const kKinds[] = { "driver", "sitgun", "standgun", "sitpass", "standpass", "kneelgun",
                                          "mg", "ground_gun", "ground_gun2", "ground", "leftboat", "rightboat" };
    int places = PUnitField<int>(UnitField<SIPUnit*>(u, kUnitPUnit), kPUnitCrewSlots) * 5;
    for (int i = 0; i < places; ++i) {
        if (Slots.Count == Slots.Max) {
            int m = Slots.Max < 0x10 ? 0x10 : (Slots.Max * 6) / 5;
            Slots.Data = (SAnimNodeSlot*)realloc(Slots.Data, (size_t)m * sizeof(SAnimNodeSlot));
            memset(Slots.Data + Slots.Max, 0, (size_t)(m - Slots.Max) * sizeof(SAnimNodeSlot));
            Slots.Max = m;
        }
        int idx = Slots.Count++;
        Slots.Data[idx].Kind = 0;
        int node = -1;
        for (int k = 0; k < 12 && node < 0; ++k) {
            char name[48];
            _snprintf(name, sizeof(name) - 1, "man%d_%s", i, kKinds[k]);
            name[sizeof(name) - 1] = 0;
            node = model->FindNode(name);
            if (node >= 0)
                Slots.Data[idx].Kind = k + 1;
        }
        Slots.Data[idx].Node = node;
    }

    BodyNode = model->FindNode("body");
    if (PUnitField<unsigned char>(UnitField<SIPUnit*>(u, kUnitPUnit), kPUnitGunCount) != 0)
        UnitAnimLoadGuns(Guns, u, model, !UnitField<bool>(u, kUnitDead));
    AntennaNode = model->FindNode("antenna");
}

// SUnit 0x5c3030 (agent U's function): stop the unit's own effects.
static void UnitStopEffects(SIUnit* u)
{
    int& n = UnitField<int>(u, kUnitEffectCount);
    int* fx = UnitField<int*>(u, kUnitEffects);
    SIPixie* px = g_UnitAnimEnv.Pixie ? g_UnitAnimEnv.Pixie() : nullptr;
    while (n > 0) {
        if (px)
            px->StopEffect(fx[0]);                                // pixie +0x34
        --n;
        if (n != 0)
            memmove(fx, fx + 1, (size_t)n * 4);
        fx[n] = 0;
    }
}

// PANZERS 0x5cd020
void SVehicleAnimation::UpdateModel()
{
    PZ_M2_TRACE("SVehicleAnimation::UpdateModel (0x5cd020)");
    SIUnit* u = Unit;
    SIModel* m = Model();
    if (!m)
        return;
    SUnitAnimEnv& e = g_UnitAnimEnv;
    SIModel* marker = UnitField<SIModel*>(u, kUnitMarker);
    bool hidden = !UnitField<bool>(u, kUnitAlwaysShown) &&
                  (UnitField<bool>(u, kUnitUnplaced) ||
                   (!e.NoFogOfWar() && e.HasGameLogic() && !e.CanSeeGroundUnit(e.LocalPlayer(), u)));
    if (hidden) {
        m->SetVisible(false, true);                               // +0x30(0, 1)
        if (marker)
            marker->SetVisible(false, false);
        UnitStopEffects(u);
        UnitField<bool>(u, kUnitSpotted) = false;
    } else {
        m->SetVisible(true, true);
        if (marker)
            marker->SetVisible(true, false);
        // unit +0x6d: 0x5c2a60, empty.
        UnitAnimSpottedCheck(u);
    }

    // Ground: height, and the slopes over 1 m tilt the model.
    float* pos = &UnitField<float>(u, kUnitPos);
    float sx = 0.0f, sz = 0.0f;
    const SHdStr& name = PUnitField<SHdStr>(UnitField<SIPUnit*>(u, kUnitPUnit), kPUnitName);
    if (name.len == 0 || _stricmp(name.buf, "FlyingFox Truck") != 0) {
        float a = e.TerrainHeight((float)((double)pos[0] - 0.5), pos[2]);
        float b = e.TerrainHeight((float)((double)pos[0] + 0.5), pos[2]);
        sx = a - b;
        float c = e.TerrainHeight(pos[0], (float)((double)pos[2] - 0.5));
        float d = e.TerrainHeight(pos[0], (float)((double)pos[2] + 0.5));
        sz = c - d;
        pos[1] = e.TerrainHeight(pos[0], pos[2]);                // 0x5e7730
    }
    float dir = UnitField<float>(u, kUnitDir);
    m->SetPosition(pos[0], pos[1], pos[2]);                       // +0x18
    m->SetRotation(dir + kPi, sx, sz);                            // +0x1c
    if (SIModel* m2 = UnitField<SIModel*>(u, kUnitModel2)) {
        m2->SetPosition(pos[0], pos[1], pos[2]);
        m2->SetRotation(dir + kPi, 0.0f, 0.0f);
    }
    if (Gear) {
        float steer = UnitField<float>(u, kUnitSteer);
        if (UnitField<bool>(u, kUnitReverse))
            steer = -steer;
        Gear->Update(m, pos[0], pos[2], (double)dir, steer * 0.5f);   // 0x5aa2c0
    }

    // Body sway: a damped spring driven by the movement of the tick and the
    // slope (MaxSpringAngle caps it).
    float tx = (float)sin(atan((double)sx));                     // 0x78d190, 0x78d640
    float tz = (float)sin(atan((double)sz));
    const SPVehicleAnimation* p = VProto;
    const float* prev = &UnitField<float>(u, kUnitPrevPos);
    Spring[0] = (double)(prev[0] - pos[0]) * 1.5 + SpringVel[0] * 0.05 + Spring[0];
    Spring[1] = (double)(prev[2] - pos[2]) * 1.5 + SpringVel[1] * 0.05 + Spring[1];
    double slopeX = (double)tx * 0.125;
    double slopeZ = (double)tz * 0.125;
    SpringVel[0] = (slopeX - (double)p->SpringStrength * Spring[0]) + SpringVel[0];
    SpringVel[1] = (slopeZ - (double)p->SpringStrength * Spring[1]) + SpringVel[1];
    double len2 = Spring[1] * Spring[1] + Spring[0] * Spring[0];
    if ((double)p->MaxSpringAngle2 + 0.01 < len2) {
        double k = (double)p->MaxSpringAngle / sqrt(len2);
        Spring[0] = Spring[0] * k;
        Spring[1] = Spring[1] * k;
        SpringVel[0] = (double)(prev[0] - pos[0]) * -1.5 * 20.0;
        SpringVel[1] = (double)(prev[2] - pos[2]) * -1.5 * 20.0;
    }
    Spring[0] = (double)p->SpringDecay * Spring[0];
    Spring[1] = (double)p->SpringDecay * Spring[1];

    // Antenna: a second spring driven by the antenna node's own movement.
    if (AntennaNode < 0) {
        AntennaReset = true;
        Rod[0] = Rod[1] = 0.0;
        RodVel[0] = RodVel[1] = 0.0;
    } else {
        float ap[3] = { 0.0f, 0.0f, 0.0f };
        AnimModelGetNodePosition(m, AntennaNode, ap);             // +0x54
        if (!AntennaReset) {
            double rx = (double)(AntennaPrev[0] - ap[0]) * 0.9 + RodVel[0] * 0.05 + Rod[0];
            Rod[0] = rx;
            double rz = (double)(AntennaPrev[1] - ap[2]) * 0.9 + RodVel[1] * 0.05 + Rod[1];
            Rod[1] = rz;
            RodVel[0] = (slopeX - (double)p->RodSpringStrength * rx) + RodVel[0];
            RodVel[1] = (slopeZ - (double)p->RodSpringStrength * rz) + RodVel[1];
            Rod[0] = (double)p->RodSpringDecay * rx;
            Rod[1] = (double)p->RodSpringDecay * rz;
        }
        AntennaPrev[0] = ap[0];
        AntennaPrev[1] = ap[2];
        AntennaReset = false;
    }

    double cd = cos((double)dir);                                  // 0x78d480
    double sd = sin((double)dir);                                  // 0x78d640
    float bodyX = (float)((cd * Spring[0] - sd * Spring[1]) * (double)p->SpringScale);
    float bodyZ = (float)((cd * Spring[1] + sd * Spring[0]) * (double)p->SpringScale);
    AnimModelSetNodeTilt(m, BodyNode, 0.0f, 0.0f, 0.0f, 0.0f, bodyX, bodyZ);   // +0x48
    int guns = PUnitField<unsigned char>(UnitField<SIPUnit*>(u, kUnitPUnit), kPUnitGunCount);
    if (guns == 0) {
        AnimModelSetNodeTilt(m, AntennaNode, 0.0f, 0.0f, 0.0f, 0.0f,
                             (float)((cd * Rod[0] - sd * Rod[1]) * (double)p->RodSpringScale),
                             (float)((cd * Rod[1] + sd * Rod[0]) * (double)p->RodSpringScale));
    } else {
        // Turret and barrel follow the gunners (unit +0x48), with the body
        // sway; the recoil nodes slide; the antenna sits on the turret.
        int gunners = UnitField<int>(u, kUnitGunnerCount);
        unsigned char** g = UnitField<unsigned char**>(u, kUnitGunners);
        for (int i = 0; i < guns; ++i) {
            if (i >= gunners)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SGunner *", i);
            const unsigned char* gn = g[i];
            float yaw = *reinterpret_cast<const float*>(gn + kGunnerYaw);
            float pitch = *reinterpret_cast<const float*>(gn + kGunnerPitch);
            float recoil = *reinterpret_cast<const float*>(gn + kGunnerRecoil);
            AnimModelSetNodeTilt(m, Guns[i].H, 0.0f, 0.0f, 0.0f, -yaw, bodyX, bodyZ);
            AnimModelSetNodeTilt(m, Guns[i].V, 0.0f, 0.0f, 0.0f, -pitch, bodyX, bodyZ);
            for (int k = 0; k < Guns[i].SCount; ++k)
                AnimModelSetNodeTilt(m, Guns[i].S[k], 0.0f, recoil, 0.0f, 0.0f, 0.0f, 0.0f);
            if (gunners < 1)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SGunner *", 0);
            double a = (double)*reinterpret_cast<const float*>(g[0] + kGunnerYaw);
            double ca = cos(a), sa = sin(a);
            AnimModelSetNodeTilt(m, AntennaNode, 0.0f, 0.0f, 0.0f, 0.0f,
                                 (float)((ca * Rod[0] - sa * Rod[1]) * (double)p->RodSpringScale),
                                 (float)((ca * Rod[1] + sa * Rod[0]) * (double)p->RodSpringScale));
        }
    }

    if (UnitField<bool>(u, kUnitStartFight))
        UnitField<bool>(u, kUnitStartFight) = false;
    if (UnitField<bool>(u, kUnitStartDie)) {
        UnitField<int>(u, kUnitDieTicks) = 100;
        if (PUnitField<int>(UnitField<SIPUnit*>(u, kUnitPUnit), kPUnitClassType) == 0xc &&
            UnitField<int>(u, 0x2e4) >= 0)
            UnitField<int>(u, kUnitDieTicks) = 0x14;
        UnitField<bool>(u, kUnitStartDie) = false;
        PZ_M2_TRACE("SUnit 0x5c26e0 (death effects, agent U) not called");
    }

    // Damage smoke at health < 0.6 (heavy below 0.3), on the body node.
    double health = (double)UnitField<float>(u, kUnitHealth);
    SIPixie* px = e.Pixie ? e.Pixie() : nullptr;
    if (health < 0.6) {
        // HD: pixie +0x30(scene, fx prototype, model, body node) creates the
        // attached effect; that SIPixie slot is a Slot_ stub, not called.
        PZ_M2_TRACE("SVehicleAnimation::UpdateModel: damage smoke (SIPixie +0x30) not lifted");
    } else {
        if (px && SmokeEffect >= 0) {
            px->StopEffect(SmokeEffect);
            SmokeEffect = -1;
        }
        if (px && HeavySmokeEffect >= 0) {
            px->StopEffect(HeavySmokeEffect);
            HeavySmokeEffect = -1;
        }
    }

    // Global state sequences ("<state>_stand<sub>", "<next>_to_<cur>").
    if (0 < VProto->StateNames.Count) {
        if (UnitField<bool>(u, kUnitStateChanging)) {
            int& ticks = UnitField<int>(u, kUnitChangeTicks);
            --ticks;
            if (ticks < 1) {
                UnitField<bool>(u, kUnitStateChanging) = false;
                int& mode = UnitField<int>(u, kUnitMoveMode);
                float mv = UnitField<float>(u, kUnitMoveSpeed);
                double turn = (double)UnitField<float>(u, kUnitTurnSpeed);
                mode = 0.0f < mv ? 2 : (1e-05 < turn ? 4 : (turn < -1e-05 ? 5 : 1));   // SUnit 0x5c10f0
                int behavior = UnitField<int>(u, kUnitBehavior);
                if (behavior != UnitField<int>(u, kUnitGlobalState))
                    u->SetBehavior(behavior);                     // +0x12c
            }
        }
        int state = UnitField<int>(u, kUnitGlobalState);
        char stand[256];
        _snprintf(stand, sizeof(stand) - 1, "%s_stand%s", VProto->StateName(state),
                  HdStr(UnitField<SHdStr>(u, kUnitSubState)));
        stand[sizeof(stand) - 1] = 0;
        if (UnitField<bool>(u, kUnitStartChange)) {
            UnitField<bool>(u, kUnitStartChange) = false;
            UnitField<bool>(u, kUnitStateChanging) = true;
            UnitField<int>(u, kUnitMoveMode) = 6;                 // SUnit 0x5c10f0 while changing
            SHdStr t = { nullptr, 0 };
            GetChangingText(&t);
            int ticks = (int)lrintf((float)(AnimModelSequenceLength(m, HdStr(t)) * 20.0f));
            if (ticks < 1)
                m->PlaySequence(stand, true);
            else
                m->PlaySequence(HdStr(t), true);
            HdStrFree(&t);
        }
        m->AdvanceAnimation(0.05f);                               // +0x70
    }
}

} // namespace pz
