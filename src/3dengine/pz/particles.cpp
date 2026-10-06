// src/3dengine/pz/particles.cpp
// SPParticles / SParticles: EffectType 0 of the .fx files (HD
// 0x6e08b0..0x6e9f70). OWNER: agent C.
//
// Simulation, per rendered frame (SEffect::Update 0x6de810 -> Process):
//   Prepare 0x6e9d70  dt = scene frame ms * 0.001 (<= 0.125 s), light track,
//                     birth on/off timer
//   UpdateWind 0x6e3440
//   MoveShot 0x6e6cb0 per particle: tracks (alpha/size/additional speed
//                     over age / lifetime), spin, gravity, velocity * air
//                     resistance, wind, collision, frame animation, death
//   Birth 0x6e21a0    BirthSpeed * dt new particles (disc / box / sphere
//                     around the emitter, VSpeed along the direction,
//                     HSpeed outward)
// Rendering (SParticles::Render 0x6e95f0), by "ParticleType":
//   0 Normal     screen-space quads: each particle is projected (SViewport
//                +0x3c) to a centre and a screen radius and written as two
//                pre-transformed triangles (XYZRHW, fog in the specular
//                alpha) into the pixie's dynamic VB, one draw per effect.
//   1 Billboard  world-space vertical quads turned to the camera yaw
//   2 Cloud      world-space horizontal quads
//   3 Trail      world-space ribbons (not ported)
// The texture is one horizontal strip of TotalAnimFrames frames ("One
// file"), or one file per frame ("More files"). BlendType 0 = alpha blend
// from the texture alpha, 1 = additive (ONE/ONE, black fog).

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <d3d9.h>
#include "effect.h"
#include "effectrender.h"
#include "pzpixie.h"
#include "iviewport.h"
#include "igepardhd.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

// ===========================================================================
// SPParticles
// ===========================================================================

// PANZERS 0x6e08b0
SPParticles::SPParticles()
    : TexMode(0), DebugID(0), MultiFile(false), MaxParticles(0), LastScene(nullptr),
      BlendType(0), ZBuffer(true), ElevDependent(false), DrawType(0), MoveType(0), ParticleType(0),
      TrailLength(0), TrailOrientation(0), BirthSpeed(0), Color(0), SubEffectProto(-1),
      LightingModel(0), LightColor1(0), LightColor2(0), OverLight1(0), LightRange(0),
      FixedLightSource(false), LifeTimeRnd(0), VariationType(0), Gravity(0), VSpeed(0), VSpeedRnd(0),
      SizeRnd(0), Radius(0), RadiusSqrt(0), RadiusCbrt(0), Sphere(false), Duration(0),
      BirthEnabledTime(0), BirthDisabledTime(0), BirthEnabledRnd(0), BirthDisabledRnd(0),
      BirthInRain(false), BalancedBirth(0), Turbulence(false), NoWind(true), ObjectSpin(false),
      InvTotalFrames(0), TotalFrames(0), FirstFrame(0), LastFrame(0), LifeTime(0), GrowTime(0),
      GrowSpeed(0), HSpeed(0), HSpeedRnd(0), Linked(false), TrailDirectionUpdate(false),
      BulletIndicator(false), BirthStyle(0), Quantity(0), SamplingRate(0), FromTerrain(false),
      FromWater(false), Altitude(0), CollisionType(0), CollisionSticking(0), PivotDir(false),
      AirKeep(1), AirResistance2(0), Wait(0), WaitRnd(0), RandomRotate(false), ModelProto(-1),
      InvLightDuration(0)
{
    WeatherTex[0] = WeatherTex[1] = WeatherTex[2] = -1;
    RandomXYZ[0] = RandomXYZ[1] = RandomXYZ[2] = 0;
    for (int i = 0; i < 3; ++i)
        SpinMin[i] = SpinRnd[i] = 0;
}

// PANZERS 0x6e1010
// Releases the textures (Gepard +0x48) and the sub-effect / model prototypes.
SPParticles::~SPParticles()
{
    SIGepardHD* g = PzGepard();
    for (int i = 0; i < 3; ++i)
        if (WeatherTex[i] >= 0)
            g->ReleaseTexture(WeatherTex[i]);
    for (int t : FrameTex)
        if (t >= 0)
            g->ReleaseTexture(t);
    if (SubEffectProto >= 0 && SPixie::Instance())
        SPixie::Instance()->ReleaseEffectPrototype(SubEffectProto);
    if (ModelProto != -1)
        g->ReleaseModelPrototype(ModelProto);                 // Gepard +0x24
}

// PANZERS 0x6e6880
bool SPParticles::IsBulletIndicator()
{
    return BulletIndicator;
}

// PANZERS 0x6e9680
void SPParticles::ReloadResources()
{
}

// PANZERS 0x6e3830
SEffect* SPParticles::CreateInstance(SIScene* scene, float lifeTime)
{
    LastScene = scene;
    return new SParticles(scene, this, lifeTime);
}

// PANZERS 0x6e5eb0
// `typeStruct` is the "00_PARTICLES" struct: Draw (multi), Tracks, Birth,
// Move (multi), General. Returns true on failure (no texture).
bool SPParticles::Init(SPropertyStruct* ts, const char* name)
{
    Name = name ? name : "";
    SPropertyStruct* draw = ts->GetMultiSubStruct(0, "Draw");
    SPropertyStruct* tracks = ts->GetStruct(1, "Tracks");
    SPropertyStruct* birth = ts->GetStruct(2, "Birth");
    SPropertyStruct* move = ts->GetMultiSubStruct(3, "Move");
    SPropertyStruct* general = ts->GetStruct(4, "General");
    DebugID = general->GetInt("ID for Debug");
    WeatherTex[0] = WeatherTex[1] = WeatherTex[2] = -1;
    if (general->GetInt("Version") == 3)
        Logger.g->Panic("SPParticles::Init(): FIXME Invalid effect version...");
    DrawType = ts->GetMultiIndex("Draw");
    TexMode = 0;
    ModelProto = -1;
    switch (DrawType) {
    case 0:
        ParticleType = draw->GetEnum("ParticleType");
        TrailLength = draw->GetFloat("TrailLength");
        TrailOrientation = draw->GetEnum("TrailOrientation");
        BlendType = draw->GetEnum("BlendType");
        ZBuffer = draw->GetBool("ZBuffer On");
        ElevDependent = draw->GetBool("ElevDependent");
        Color = draw->GetColor("Color");
        RandomRotate = draw->GetBool("Random rotate");
        TexMode = draw->GetMultiIndex("Texture Anim in");
        LightingModel = draw->GetEnum("LightingModel");
        if (LightingModel > 0)
            LightColor1 = draw->GetColor("LightColor1");
        if (LightingModel == 2)
            LightColor2 = draw->GetColor("LightColor2");
        OverLight1 = draw->GetFloat("OverLight1");
        LightRange = draw->GetFloat("LightRange");
        draw->GetTrackFloat("LightIntensity", &LightTrack);
        InvLightDuration = 1.0f / draw->GetFloat("LightDuration");
        FixedLightSource = draw->GetBool("FixedLightSource");
        SubEffectProto = -1;
        break;
    case 1:
        // Draw "Object": the particles are instances of this model (Gepard
        // +0x20; drawing them, 0x6e5b80, is still a logged stub).
        ModelProto = PzGepard()->LoadModelPrototype(draw->GetString("Mesh file"), 0.005f, nullptr, 0);
        if (ModelProto == -1) {
            EffectSetLastError("Bad model filename!");        // 0x65ca40
            return true;
        }
        break;
    case 2: {
        ParticleType = 5;
        TrailLength = 2.0f;
        SubEffectProto = SPixie::Instance()->LoadEffectPrototype(draw->GetString(0, "FX file"), false, false, 0, 0);
        if (SubEffectProto == -1) {
            EffectSetLastError("Cannot load effect");         // 0x65ca40
            return true;
        }
        BlendType = 0;
        ZBuffer = true;
        SizeRnd = 1.0f;
        Color = 0xffffff;
        RandomRotate = false;
        TexMode = 0;
        LightingModel = 0;
        OverLight1 = 1.0f;
        LightRange = 1.0f;
        break;
    }
    case 3:
        ZBuffer = true;
        break;
    default:
        Logger.g->Panic("Not yet supported");
    }
    if (DrawType == 3)
        return false;

    tracks->GetTrackFloat("Alpha", &AlphaTrack);
    tracks->GetTrackFloat("Size", &SizeTrack);
    SizeRnd = tracks->GetFloat("Size_Rnd") * 0.5f;
    tracks->GetTrackFloat("AdditionalSpeed", &SpeedTrack);
    BirthSpeed = birth->GetFloat("BirthSpeed");
    Duration = birth->GetFloat("Duration");
    LifeTime = birth->GetFloat("LifeTime");
    LifeTimeRnd = birth->GetFloat("LifeTime_Rnd");
    Sphere = birth->GetBool("Sphere");
    Radius = birth->GetFloat("Radius");
    RandomXYZ[0] = birth->GetFloat("RandomX");
    RandomXYZ[1] = birth->GetFloat("RandomY");
    RandomXYZ[2] = birth->GetFloat("RandomZ");
    VSpeed = birth->GetFloat("VSpeed");
    VSpeedRnd = birth->GetFloat("VSpeed_Rnd");
    HSpeed = birth->GetFloat("HSpeed");
    HSpeedRnd = birth->GetFloat("HSpeed_Rnd");
    FromTerrain = birth->GetBool("FromTerrain");
    FromWater = birth->GetBool("FromWater");
    Altitude = birth->GetFloat("Altitude") * 0.5f;
    CollisionType = birth->GetEnum("CollisionType");
    CollisionSticking = birth->GetFloat("CollisionSticking");
    GrowTime = birth->GetFloat("GrowTime");
    GrowSpeed = birth->GetFloat("GrowSpeed");
    BirthEnabledTime = (int)birth->GetFloat("BirthEnabledTime");         // ftol 0x767700
    BirthEnabledRnd = (int)birth->GetFloat("BirthEnabledTimeRND");
    BirthDisabledTime = (int)birth->GetFloat("BirthDisabledTime");
    BirthDisabledRnd = (int)birth->GetFloat("BirthDisabledTimeRND");
    BirthInRain = birth->GetBool("Birth in rain");
    BalancedBirth = birth->GetMultiIndex("Balanced birth");
    BirthStyle = birth->GetMultiIndex("Style");
    if (BirthStyle == 1)
        Quantity = birth->GetMultiSubStruct("Style")->GetInt("Quantity");
    else
        Quantity = 0;
    SamplingRate = birth->GetFloat("Sampling rate");
    MoveType = ts->GetMultiIndex("Move");
    if (MoveType == 0) {
        VariationType = move->GetEnum("VariationType");
        Gravity = move->GetFloat("Gravity");
        float keep = 1.0f - move->GetFloat("AirResistance");
        AirKeep = keep < 0.0f ? 0.0f : keep;
        AirResistance2 = move->GetFloat("AirResistance2");
        Turbulence = move->GetBool("Turbulence");
        NoWind = move->GetBool("NoWind");
        ObjectSpin = move->GetBool("Object Spin");
        SpinMin[0] = move->GetFloat("X Spin Minimum");
        SpinRnd[0] = move->GetFloat("X Spin Random");
        SpinMin[1] = move->GetFloat("Y Spin Minimum");
        SpinRnd[1] = move->GetFloat("Y Spin Random");
        SpinMin[2] = move->GetFloat("Z Spin Minimum");
        SpinRnd[2] = move->GetFloat("Z Spin Random");
    } else if (MoveType == 1) {
        VariationType = move->GetEnum("VariationType");
        Gravity = move->GetFloat("Gravity");
    }
    Linked = general->GetBool("Linked");
    TrailDirectionUpdate = general->GetBool("TrailDirectionUpdate");
    PivotDir = general->GetBool("PivotDir");
    Wait = general->GetFloat("Wait");
    WaitRnd = general->GetFloat("Wait_Rnd");
    BulletIndicator = general->GetBool("Bullet Indicator");
    RadiusSqrt = (float)pow((double)Radius, 0.5);
    RadiusCbrt = (float)pow((double)Radius, 0.333333343);
    if (DrawType == 0) {
        SPropertyStruct* anim = draw->GetMultiSubStruct("Texture Anim in");
        Texture = anim->GetString(0, "Texture");
        if (TexMode != 0) {
            TotalFrames = anim->GetFloat(1, "TotalAnimFrames");
            FirstFrame = anim->GetFloat(2, "FirstAnimFrame");
            LastFrame = anim->GetFloat(3, "LastAnimFrame");
            InvTotalFrames = 1.0f / TotalFrames;
        }
    }
    if (LoadTextures(0, "") || LoadTextures(1, "_snowy") || LoadTextures(2, "_foggy"))
        return true;
    if (BirthStyle == 0) {
        float n = (LifeTime + LifeTimeRnd) * BirthSpeed;
        if (n <= 6.0f)
            n = 6.0f;
        MaxParticles = (int)(n + 1.0f);
    } else {
        MaxParticles = 0x1000;
    }
    return false;
}

// PANZERS 0x6e6890
// "One file": effects\media\<Texture> with the weather suffix put before
// "_a" (or before the extension); a missing weather variant falls back to
// the plain name. "More files": effects\media\<Texture><suffix>NN[_a].tga
// for NN = 0, 1, ... until one is missing. Returns true on failure.
bool SPParticles::LoadTextures(int weather, const char* suffix)
{
    if (DrawType != 0)
        return false;
    SIGepardHD* g = PzGepard();
    char path[300];
    if (TexMode == 0) {
        MultiFile = true;
        if (Texture.empty()) {
            EffectSetLastError("SPParticles::Init: Nem lett textura megadva!");   // 0x65ca40
            return true;
        }
        if (weather != 0)
            return false;   // HD loads the frame list for the default weather only once
        for (;;) {
            sprintf(path, BlendType == 1 ? "effects\\media\\%s%s%02d.tga" : "effects\\media\\%s%s%02d_a.tga",
                    Texture.c_str(), suffix, (int)FrameTex.size());
            int t = g->LoadTexture(path, 0, false);
            if (t == -1)
                break;
            FrameTex.push_back(t);
        }
        if (!FrameTex.empty()) {
            FirstFrame = 0.0f;
            TotalFrames = (float)(int)FrameTex.size();
            LastFrame = TotalFrames - 1.0f;
            return false;
        }
        EffectSetLastError("SPParticles::Init: Cannot load %s", path);           // 0x65ca40
        return true;
    }
    MultiFile = false;
    std::string full = std::string("effects\\media\\") + Texture;
    int n = (int)full.size();
    int cut = n - 6;
    if (cut < 0 || full[cut] != '_' || full[n - 5] != 'a')
        cut = n - 4;
    std::string name = full.substr(0, cut) + suffix + full.substr(cut);
    int t = g->LoadTexture(name.c_str(), 1, true);
    if (t == -1)
        t = g->LoadTexture(full.c_str(), 1, true);
    WeatherTex[weather] = t;
    if (t != -1)
        return false;
    EffectSetLastError("SPParticles::Init: cannot load texture \"%s\"", full.c_str());   // 0x65ca40
    return true;
}

// ===========================================================================
// SParticles
// ===========================================================================

// PANZERS 0x6e0980
// lifeTime != 0 overrides the .fx LifeTime (SPixie::PlayEffect p5).
SParticles::SParticles(SIScene* scene, SPParticles* proto, float lifeTime)
    : SEffect(proto)
{
    Stopped = false;
    memset(LightColor1, 0, sizeof(LightColor1));
    memset(LightColor2, 0, sizeof(LightColor2));
    LightIntensity = 0.0f;
    LightPos[0] = 32.0f; LightPos[1] = 2.0f; LightPos[2] = 32.0f;
    BirthAngle = 0.0f;
    RefreshBirthPoints = true;
    LightPosSet = false;
    CurAlpha = CurSize = CurLight = CurSpeed = 0;
    Scene = scene;
    Prototype = proto;
    LifeTimeBase = lifeTime == 0.0f ? proto->LifeTime : lifeTime;
    FrameLength = 100000.0f;
    FrameLengthRnd = 0.0f;
    if (proto->VariationType == 1) {
        float n = (proto->LastFrame - proto->FirstFrame) + 1.0f;
        FrameLength = proto->LifeTime / n;
        FrameLengthRnd = proto->LifeTimeRnd / n;
    }
    AlphaScale = 1.0f;
    SizeScale = 1.0f;
    float wr = proto->WaitRnd;
    WaitLeft = proto->Wait + (float)((double)rand() * (1.0 / 32768.0) * (double)wr);
    Time = 0.0f;
    Air2Acc = 0.0f;
    Dir[0] = 0.0f; Dir[1] = 1.0f; Dir[2] = 0.0f;
    Right[0] = 1.0f; Right[1] = 0.0f; Right[2] = 0.0f;
    Forward[0] = 0.0f; Forward[1] = 0.0f; Forward[2] = 1.0f;
    AirFactor = 1.0f;
    LightRange2 = proto->LightRange * proto->LightRange;
    DurationLeft = proto->Duration;
    BirthAcc = 1.0f;
    unsigned c = proto->Color;
    ColorF[0] = (float)((c >> 16) & 0xff) / 255.0f;
    ColorF[1] = (float)((c >> 8) & 0xff) / 255.0f;
    ColorF[2] = (float)(c & 0xff) / 255.0f;
    ColorF[3] = 0.0f;
    if (proto->BlendType != 1) {
        const float* l = SceneLightColor(scene);
        ColorF[0] *= l[0];
        ColorF[1] *= l[1];
        ColorF[2] *= l[2];
    }
    ColorB[0] = (unsigned char)(int)(ColorF[0] * 255.0f);
    ColorB[1] = (unsigned char)(int)(ColorF[1] * 255.0f);
    ColorB[2] = (unsigned char)(int)(ColorF[2] * 255.0f);
    unsigned l1 = proto->LightColor1;
    LightColor1[0] = (float)((l1 >> 16) & 0xff) / 255.0f * proto->OverLight1;
    LightColor1[1] = (float)((l1 >> 8) & 0xff) / 255.0f * proto->OverLight1;
    LightColor1[2] = (float)(l1 & 0xff) / 255.0f * proto->OverLight1;
    LightColor1[3] = 0.0f;
    unsigned l2 = proto->LightColor2;
    LightColor2[0] = (float)((l2 >> 16) & 0xff) / 255.0f;
    LightColor2[1] = (float)((l2 >> 8) & 0xff) / 255.0f;
    LightColor2[2] = (float)(l2 & 0xff) / 255.0f;
    LightColor2[3] = 0.0f;
    if (proto->BirthEnabledTime == 0)
        BirthOn = true;
    else
        BirthOn = ((rand() * 2) & 0xffff8000u) != 0;
    BirthToggle = 0.0f;
    float w = (EffectRand() - 0.5f) + (SPixie::Instance() ? SPixie::Instance()->WindPhase : 0.0f);
    WindBase = w;
    WindAngle[0] = w;
    WindAngle[1] = w;
    WindLow[0] = WindLow[1] = WindHigh[0] = WindHigh[1] = 0.0f;
    WindStrength[0] = WindStrength[1] = 0.0f;
    WindLowHeight = 1.1f;
    WindHighHeight = 2.2f;
    WindLowScale = 0.909091f;    // 0x3f68ba2e
    WindBlendScale = 0.909091f;
    WindTimer[0] = WindTimer[1] = 0.0f;
    WindTurn[0] = WindTurn[1] = WindStrengthTo[0] = WindStrengthTo[1] = 0.0f;
    WindSpeed[0] = WindSpeed[1] = WindPhase[0] = WindPhase[1] = 0.0f;
    WindCalm[0] = WindCalm[1] = true;
    Dt = 0.0f;
    TrailHandle = -1;
    RenderFlag = false;
}

// PANZERS 0x6e1230
SParticles::~SParticles()
{
    if (Prototype->DrawType == 2 && SPixie::Instance())
        for (SParticleData& p : Particles)
            SPixie::Instance()->StopEffect(p.SubEffect);
}

// PANZERS 0x6e96b0
// Only with "PivotDir": the birth basis follows the direction.
void SParticles::SetDirection(const float* dir)
{
    if (!Prototype->PivotDir)
        return;
    SEffect::SetDirection(dir);
    float dx = Dir[0], dy = Dir[1], dz = Dir[2];
    // Right = Dir x (1,0,0), or Dir x (0,0,1) when Dir is along x.
    Right[0] = dy * 0.0f - dz * 0.0f;
    Right[1] = dz - dx * 0.0f;
    Right[2] = dx * 0.0f - dy;
    if (Right[0] * Right[0] + Right[1] * Right[1] + Right[2] * Right[2] < 0.01f) {
        Right[2] = dx * 0.0f - dy * 0.0f;
        Right[0] = dy - dz * 0.0f;
        Right[1] = dz * 0.0f - dx;
    }
    double inv = 1.0 / sqrt((double)(Right[0] * Right[0] + Right[1] * Right[1] + Right[2] * Right[2]));
    Right[0] = (float)(Right[0] * inv);
    Right[1] = (float)(Right[1] * inv);
    Right[2] = (float)(Right[2] * inv);
    Forward[0] = Right[2] * dy - Right[1] * dz;
    Forward[1] = Right[0] * dz - Right[2] * dx;
    Forward[2] = Right[1] * dx - Right[0] * dy;
    UpdateParticleSpins();
}

// PANZERS 0x6e9690
void SParticles::SetAlphaScale(float s) { AlphaScale = s; }
// PANZERS 0x6e9920
void SParticles::SetSizeScale(float s) { SizeScale = s; }
// PANZERS 0x6e9940
void SParticles::Stop() { Stopped = true; }

// PANZERS 0x6e5dd0
float SParticles::GroundHeight(float x, float z)
{
    float g = g_EffectGroundHeight ? g_EffectGroundHeight(x, z) : 0.0f;
    if (Prototype->FromWater && g_EffectWaterHeight) {
        float w = g_EffectWaterHeight(x, z);
        if (g < w)
            return w;
    }
    return g;
}

// PANZERS 0x6e9d70
int SParticles::Prepare()
{
    SPParticles* pr = Prototype;
    if (!LightPosSet || !pr->FixedLightSource) {
        LightPos[0] = Pos[0]; LightPos[1] = Pos[1]; LightPos[2] = Pos[2];
        LightPosSet = true;
    }
    if (pr->FromTerrain || pr->FromWater)
        LightPos[1] = GroundHeight(Pos[0], Pos[2]) + pr->Altitude;
    LightIntensity = pr->LightTrack.Evaluate(pr->InvLightDuration * Time, &CurLight);
    unsigned ms = (unsigned)SceneFrameMs(Scene);
    Dt = (float)((double)ms * 0.001);
    if (0.125f < Dt)
        Dt = 0.125f;
    Time += Dt;
    if (pr->BirthEnabledTime != 0) {
        if (BirthToggle <= 0.0f) {
            BirthOn = !BirthOn;
            double r;
            int base;
            if (BirthOn) {
                r = (double)rand() * (1.0 / 32768.0) * (double)pr->BirthEnabledRnd;
                base = pr->BirthEnabledTime;
            } else {
                r = (double)rand() * (1.0 / 32768.0) * (double)pr->BirthDisabledRnd;
                base = pr->BirthDisabledTime;
            }
            BirthToggle = (float)base + (float)r;
        }
        BirthToggle -= Dt;
    }
    return 2;
}

// PANZERS 0x6e3440
// Two wind layers (low/high). Each swings with a sine over a random period
// between calm and gusty phases; the result is a horizontal push per layer.
void SParticles::UpdateWind(float dt)
{
    for (int k = 0; k < 2; ++k) {
        if (WindTimer[k] <= 0.0f) {
            WindTimer[k] = 1.0f;
            WindCalm[k] = !WindCalm[k];
            if (WindCalm[k]) {
                WindSpeed[k] = (float)((double)rand() * (1.0 / 32768.0) * 0.200000003) + 0.16f;
                WindStrengthTo[k] = (float)((double)rand() * (1.0 / 32768.0) * 1.20000005);
                WindTurn[k] = (float)((double)rand() * (1.0 / 32768.0) * 3.0) - 1.5f;
            } else {
                WindSpeed[k] = (float)((double)rand() * (1.0 / 32768.0) * 0.200000003) + 0.16f;
                WindStrengthTo[k] = 0.0f;
                WindTurn[k] = 0.0f;
            }
        }
    }
    for (int k = 0; k < 2; ++k) {
        float t = WindTimer[k] - WindSpeed[k] * dt;
        WindTimer[k] = t;
        WindPhase[k] = (1.0f - t) * 3.1415f;
        double s = sin((double)WindPhase[k]);
        WindStrength[k] = (float)((double)WindStrengthTo[k] * s + 0.0);
        WindAngle[k] = (float)((double)WindTurn[k] * s + (double)WindBase);
    }
    WindLow[0] = (float)(sin((double)WindAngle[0]) * (double)WindStrength[0]);
    WindLow[1] = (float)(cos((double)WindAngle[0]) * (double)WindStrength[0]);
    WindHigh[0] = (float)(sin((double)WindAngle[1]) * (double)WindStrength[1]);
    WindHigh[1] = (float)(cos((double)WindAngle[1]) * (double)WindStrength[1]);
}

// PANZERS 0x6e84a0
bool SParticles::Process()
{
    SPParticles* pr = Prototype;
    if (pr->BirthStyle != 0 && Model && pr->Radius == 0.0f)
        STUB_LOG("SParticles::Process birth from model (SModel +0x10)");
    if (DurationLeft <= 0.0f || Active) {
        WaitLeft -= (float)((double)(unsigned)SceneFrameMs(Scene) * 0.001);
        if (0.0f < WaitLeft)
            return true;
        int r = Prepare();
        if (r == 0)
            return false;
        if (r == 1)
            return true;
        if (pr->DrawType == 3)
            STUB_LOG("SParticles::Process trail (scene +0x80, smoketrail_a.tga)");
        UpdateWind(Dt);
        bool alive;
        if (pr->MoveType == 0)
            alive = MoveShot();
        else if (pr->MoveType == 1)
            alive = MoveWaste();
        else
            alive = false;
        if (Active && Enabled)
            Birth();
        if (!Particles.empty())
            alive = true;
        if ((DurationLeft >= 0.0f && !Stopped) || alive)
            return true;
    }
    return false;
}

// PANZERS 0x6e9b60
void SParticles::UpdateParticleTracks(int i)
{
    SPParticles* pr = Prototype;
    SParticleData& p = Particles[i];
    if (pr->AlphaTrack.Count() > 1)
        p.Alpha = pr->AlphaTrack.Evaluate(p.InvLifeTime * p.Age, &CurAlpha) * p.AlphaScale;
    if (pr->SizeTrack.Count() > 1)
        p.Size = pr->SizeTrack.Evaluate(p.InvLifeTime * p.Age, &CurSize) * 0.5f + p.SizeRnd;
    if (pr->SpeedTrack.Count() > 1)
        p.AddSpeed = pr->SpeedTrack.Evaluate(p.InvLifeTime * p.Age, &CurSpeed);
}

// PANZERS 0x6e6cb0
bool SParticles::MoveShot()
{
    SPParticles* pr = Prototype;
    Air2Acc = (float)pow((double)pr->AirResistance2, (double)Time) * Dt + Air2Acc;
    AirFactor = (float)pow((double)pr->AirKeep, (double)Air2Acc);
    if (Dt == 0.0f)
        return !Particles.empty();
    const float twoPi = 6.28318548f;
    int i = 0;
    while (i < (int)Particles.size()) {
        UpdateParticleTracks(i);
        SParticleData* p = &Particles[i];
        if (pr->ObjectSpin) {
            for (int k = 0; k < 3; ++k) {
                p->Spin[k] = p->SpinSpeed[k] * Dt + p->Spin[k];
                if (twoPi <= p->Spin[k])
                    p->Spin[k] -= twoPi;
                if (p->Spin[k] < 0.0f)
                    p->Spin[k] += twoPi;
            }
        }
        if (pr->Turbulence) {
            float d[3];
            if (Pos[0] == PrevPos[0] && Pos[1] == PrevPos[1] && Pos[2] == PrevPos[2]) {
                d[0] = 0.0f; d[1] = 1.0f; d[2] = 0.0f;
            } else {
                d[0] = PrevPos[0] - Pos[0]; d[1] = PrevPos[1] - Pos[1]; d[2] = PrevPos[2] - Pos[2];
            }
            SetDirection(d);
            p = &Particles[i];
        }
        float old[3] = { p->Pos[0], p->Pos[1], p->Pos[2] };
        if (pr->Linked) {
            p->Pos[1] = (Pos[1] - PrevPos[1]) + p->Pos[1];
            p->Pos[2] = (Pos[2] - PrevPos[2]) + p->Pos[2];
            p->Pos[0] = p->Pos[0] + (Pos[0] - PrevPos[0]);
        }
        if (pr->TrailDirectionUpdate) {
            p->Dir[0] = Dir[0]; p->Dir[1] = Dir[1]; p->Dir[2] = Dir[2];
        }
        p->Vel[1] = p->Vel[1] - pr->Gravity * Dt;
        float s = p->AddSpeed;
        p->Pos[0] = Dt * (s * p->Vel0[0] + p->Vel[0]) * AirFactor + p->Pos[0];
        p->Pos[1] = Dt * (p->Vel[1] + p->Vel0[1] * s) * AirFactor + p->Pos[1];
        p->Pos[2] = Dt * (p->Vel[2] + p->Vel0[2] * s) * AirFactor + p->Pos[2];
        if (!pr->NoWind) {
            float h = p->Pos[1] - Pos[1];
            float wx, wz;
            if (WindLowHeight < h) {
                float t = (h - WindLowHeight) * WindBlendScale;
                wx = WindLow[0] * (1.0f - t) + WindHigh[0] * t;
                wz = WindLow[1] * (1.0f - t) + WindHigh[1] * t;
            } else {
                wx = WindLow[0] * WindLowScale * h;
                wz = WindLow[1] * WindLowScale * h;
            }
            p->Pos[0] = Dt * wx + p->Pos[0];
            p->Pos[2] = Dt * wz + p->Pos[2];
        }
        if (pr->ParticleType == 3 && !pr->TrailDirectionUpdate) {
            float dx = p->Pos[0] - old[0], dy = p->Pos[1] - old[1], dz = p->Pos[2] - old[2];
            double inv = 1.0 / sqrt((double)(dy * dy + dx * dx + dz * dz));
            p->Dir[0] = (float)(dx * inv);
            p->Dir[1] = (float)(dy * inv);
            p->Dir[2] = (float)(dz * inv);
        }
        int ct = pr->CollisionType;
        if (ct == 1) {
            float half = p->Size * 0.5f;
            float g = GroundHeight(p->Pos[0], p->Pos[2]);
            if ((p->Pos[1] - half) - g < 0.0f) {
                if (!p->Collided) {
                    float k = pr->CollisionSticking;
                    p->Vel[0] *= k;
                    p->Vel[2] *= k;
                    p->Vel0[0] *= k;
                    p->Vel0[2] *= k;
                    p->Collided = true;
                }
                p->Pos[1] = GroundHeight(p->Pos[0], p->Pos[2]) + p->Size * 0.5f;
            }
        } else if (ct == 2) {
            float g = GroundHeight(p->Pos[0], p->Pos[2]);
            if (p->Size * 0.5f + (p->Pos[1] - g) < 0.0f) {
                RemoveParticle(i);
                continue;
            }
        } else if (ct == 3) {
            float off = pr->DrawType == 0 ? p->Size * 0.5f : 0.0f;
            float g = GroundHeight(p->Pos[0], p->Pos[2]);
            if ((p->Pos[1] - off) - g < 0.0f) {
                float k = pr->CollisionSticking;
                p->Vel[1] = -(p->Vel[1] * k);
                p->Vel0[1] = -(p->Vel0[1] * k);
                p->Vel[0] = p->Vel[0] * k * 1.71f;
                p->Vel0[0] = p->Vel0[0] * k * 1.71f;
                p->Vel[2] = p->Vel[2] * k * 1.71f;
                p->Vel0[2] = p->Vel0[2] * k * 1.71f;
                if (!p->Collided) {
                    p->Collided = true;
                } else {
                    p->SpinSpeed[0] = 0.0f;
                    p->SpinSpeed[1] = 0.0f;
                    p->SpinSpeed[2] = 0.0f;
                    p->Spin[0] = 0.0f;
                    p->Spin[2] = 0.0f;
                }
                p->Pos[1] = GroundHeight(p->Pos[0], p->Pos[2]) + 9.99999975e-05f + off;
            }
        }
        if (pr->VariationType == 1) {
            float t = Dt + p->FrameTime;
            for (;;) {
                p->FrameTime = t;
                if (p->FrameTime < p->FrameLength)
                    break;
                p->Frame++;
                if (pr->LastFrame + 1.0f <= (float)p->Frame)
                    p->Frame = (int)pr->LastFrame;
                t = p->FrameTime - p->FrameLength;
            }
        }
        p->Age = p->Age + Dt;
        if (p->Age <= p->LifeTime)
            ++i;
        else
            RemoveParticle(i);
    }
    return !Particles.empty();
}

// HD 0x6e7a60: "Move = Waste" (particles that settle and wait). Not used
// by the menu effects.
bool SParticles::MoveWaste()
{
    STUB_LOG("SParticles::MoveWaste (0x6e7a60)");
    PZ_TRACE("SParticles::MoveWaste (0x6e7a60)");
    return !Particles.empty();
}

// PANZERS 0x6e21a0
void SParticles::Birth()
{
    SPParticles* pr = Prototype;
    if (0.0f <= DurationLeft && !Stopped && BirthOn) {
        BirthAcc = pr->BirthSpeed * Dt + BirthAcc;
        if (pr->BirthStyle == 0) {
            BirthCentralized();
        } else {
            // Along the edge / from basement need the model's birth points
            // (0x6e8680, 0x6e1c20).
            STUB_LOG("SParticles::Birth from a model (0x6e1c20)");
        }
    }
    if (DurationLeft != 0.0f) {
        DurationLeft -= Dt;
        if (DurationLeft == 0.0f)
            DurationLeft = -1.0e-5f;   // 0xb727c5ac
    }
}

// PANZERS 0x6e1890
void SParticles::BirthCentralized()
{
    SPParticles* pr = Prototype;
    SPixie* px = SPixie::Instance();
    if (!pr->BirthInRain && !(px && px->RainIntensity <= 15.0f))
        return;
    float frac = 0.0f;
    if (pr->BalancedBirth == 1)
        frac = 1.0f / (float)(int)BirthAcc;
    while (1.0f < BirthAcc) {
        BirthAcc -= 1.0f;
        if ((int)Particles.size() < pr->MaxParticles) {
            SParticleData z;
            memset(&z, 0, sizeof(z));
            Particles.push_back(z);
            int i = (int)Particles.size() - 1;
            SParticleData& p = Particles[i];
            p.Spin[0] = -1.0f;
            p.Collided = false;
            p.Age = 0.0f;
            p.LifeTime = LifeTimeBase + (float)((double)rand() * (1.0 / 32768.0) * (double)pr->LifeTimeRnd);
            p.InvLifeTime = 1.0f / p.LifeTime;
            InitParticle(i, frac * 0.0f, -1.0f, -1.0f, -1.0f);   // HD: frac * DAT_007f1038 (0.0)
            SParticleData& q = Particles[i];
            q.Rotation = (float)((double)rand() * (1.0 / 32768.0) * 359.0);
            q.FrameLength = FrameLength + (float)((double)rand() * (1.0 / 32768.0) * (double)FrameLengthRnd);
            if (pr->VariationType == 0) {
                float n = (pr->LastFrame - pr->FirstFrame) + 1.0f;
                q.Frame = (int)((float)((rand() * ((int)n & 0xffff)) >> 15) + pr->FirstFrame);
            } else {
                q.Frame = (int)pr->FirstFrame;
            }
            q.FrameTime = 0.0f;
            InitParticleTracks(i);
            InitParticleSpin(i);
            InitParticleModel(i);
        } else {
            BirthAcc = 0.0f;
        }
    }
}

// HD 0x6e1610: one particle at a given point (birth styles 1 and 2).
void SParticles::BirthAt(float x, float y, float z, float frac)
{
    STUB_LOG("SParticles::BirthAt (0x6e1610)");
    (void)x; (void)y; (void)z; (void)frac;
}

// PANZERS 0x6e2250
// Position and velocity of a new particle. The emitter basis is Right
// (+0xf0), Dir (+0x1c, "up") and Forward (+0xfc).
void SParticles::InitParticle(int i, float frac, float x, float y, float z)
{
    SPParticles* pr = Prototype;
    float vs = pr->VSpeed + (float)((double)rand() * (1.0 / 32768.0) * (double)pr->VSpeedRnd);
    float hx = 0.0f, hz = 0.0f;
    SParticleData& p = Particles[i];
    if (pr->BirthStyle == 0) {
        if (pr->Radius == 0.0f) {
            // a box of RandomX/Y/Z around the emitter
            double r1 = (double)rand() * (1.0 / 32768.0);
            double r2 = (double)rand() * (1.0 / 32768.0);
            double r3 = (double)rand() * (1.0 / 32768.0);
            float ox = (float)(r3 * (double)pr->RandomXYZ[0]) - pr->RandomXYZ[0] * 0.5f;
            float oy = (float)(r2 * (double)pr->RandomXYZ[1]) - pr->RandomXYZ[1] * 0.5f;
            float oz = (float)(r1 * (double)pr->RandomXYZ[2]) - pr->RandomXYZ[2] * 0.5f;
            float dx = oz * Forward[0] + Dir[0] * oy + Right[0] * ox;
            float dy = Forward[1] * oz + Dir[1] * oy + Right[1] * ox;
            float dz = Forward[2] * oz + Dir[2] * oy + Right[2] * ox;
            if (!pr->FromTerrain && !pr->FromWater) {
                p.Pos[0] = Pos[0] + dx;
                p.Pos[1] = Pos[1] + dy;
            } else {
                double r = (double)rand() * (1.0 / 32768.0) * 0.004;
                float g = GroundHeight(Pos[0], Pos[2]);
                float h = (g - p.Size) + (float)r + 0.004f;
                p.Pos[0] = Pos[0] + dx;
                p.Pos[1] = dy + h + pr->Altitude;
            }
            p.Pos[2] = Pos[2] + dz;
            double inv = 1.0 / sqrt((double)(oz * oz + ox * ox));
            float h = pr->HSpeed + (float)((double)rand() * (1.0 / 32768.0) * (double)pr->HSpeedRnd);
            hx = (float)(ox * inv) * h;
            hz = (float)(oz * inv) * h;
        } else if (!pr->Sphere) {
            // a disc of Radius, denser towards the centre
            float ang;
            if (pr->GrowSpeed == 0.0f) {
                ang = (float)((double)rand() * (1.0 / 32768.0) * 359.98999);
            } else {
                ang = (float)((double)rand() * (1.0 / 32768.0) * 5.646);
                ang = ang * ang * ang;
                if (((rand() * 2) & 0xffff8000u) == 0x8000)
                    ang = -ang;
                ang = (BirthAngle - 180.0f) + ang;
            }
            float t = (float)((double)rand() * (1.0 / 32768.0) * (double)pr->RadiusSqrt);
            float r = pr->Radius - t * t;
            if (pr->GrowSpeed != 0.0f) {
                float d = ang - BirthAngle;               // 0x6e5cc0
                if (d <= 0.0f)
                    d = 360.0f - (BirthAngle - ang);
                if (!(d < 180.0f))
                    d = 360.0f - d;
                r = (d * 0.01f) * (d * 0.01f) * r;
            }
            double rad = (double)ang * 0.0174532924;
            double sn = sin(rad), cs = cos(rad);
            float s = (float)(r * sn);
            float c = (float)(r * cs);
            float dx = Right[0] * c + Forward[0] * s;
            float dy = Right[1] * c + Forward[1] * s;
            float dz = Right[2] * c + Forward[2] * s;
            if (!pr->FromTerrain && !pr->FromWater) {
                p.Pos[0] = Pos[0] + dx + 0.0f;
                p.Pos[1] = Pos[1] + dy + pr->Altitude;
                p.Pos[2] = Pos[2] + dz + 0.0f;
            } else {
                double rr = (double)rand() * (1.0 / 32768.0) * 0.004;
                float g = GroundHeight(Pos[0], Pos[2]);
                float h = (g - p.Size * 0.5f) + (float)rr + 0.004f;
                p.Pos[0] = Pos[0] + dx;
                p.Pos[1] = dy + h + pr->Altitude;
                p.Pos[2] = Pos[2] + dz;
            }
            float h = pr->HSpeed + (float)((double)rand() * (1.0 / 32768.0) * (double)pr->HSpeedRnd);
            hx = (float)(h * cs);
            hz = (float)(h * sn);
        } else {
            // a ball of Radius (rejection sampling in the cube)
            float a, b, c;
            do {
                a = (float)((double)rand() * (1.0 / 32768.0) * (double)(pr->Radius * 2.0f)) - pr->Radius;
                b = (float)((double)rand() * (1.0 / 32768.0) * (double)(pr->Radius * 2.0f)) - pr->Radius;
                c = (float)((double)rand() * (1.0 / 32768.0) * (double)(pr->Radius * 2.0f)) - pr->Radius;
            } while (pr->Radius * 2.0f < b * b + a * a + c * c);
            float dx = Dir[0] * b + Right[0] * a + Forward[0] * c;
            float dy = Dir[1] * b + Right[1] * a + Forward[1] * c;
            float dz = Dir[2] * b + Right[2] * a + Forward[2] * c;
            if (!pr->FromTerrain && !pr->FromWater) {
                p.Pos[0] = Pos[0] + dx;
                p.Pos[1] = Pos[1] + dy;
            } else {
                double rr = (double)rand() * (1.0 / 32768.0) * 0.004;
                float g = GroundHeight(Pos[0], Pos[2]);
                float h = (g - p.Size) + (float)rr + 0.004f;
                p.Pos[0] = Pos[0] + dx;
                p.Pos[1] = dy + h + pr->Altitude;
            }
            p.Pos[2] = Pos[2] + dz;
            if (a < 0.001f && c < 0.001f && -0.001f < a && -0.001f < c)
                a = 0.1f;
            double inv = 1.0 / sqrt((double)(a * a + c * c));
            float h = pr->HSpeed + (float)((double)rand() * (1.0 / 32768.0) * (double)pr->HSpeedRnd);
            hz = h * (float)(c * inv);
            hx = h * (float)(a * inv);
        }
    } else {
        double r1 = (double)rand() * (1.0 / 32768.0);
        double r2 = (double)rand() * (1.0 / 32768.0);
        p.Pos[0] = (x - (float)(r1 * pr->RandomXYZ[0])) + (float)(r2 * (pr->RandomXYZ[0] * 2.0f));
        r1 = (double)rand() * (1.0 / 32768.0);
        r2 = (double)rand() * (1.0 / 32768.0);
        p.Pos[1] = (y - (float)(r1 * pr->RandomXYZ[1])) + (float)(r2 * (pr->RandomXYZ[1] * 2.0f));
        r1 = (double)rand() * (1.0 / 32768.0);
        r2 = (double)rand() * (1.0 / 32768.0);
        p.Pos[2] = (z - (float)(r1 * pr->RandomXYZ[2])) + (float)(r2 * (pr->RandomXYZ[2] * 2.0f));
        float h = pr->HSpeed + (float)((double)rand() * (1.0 / 32768.0) * (double)pr->HSpeedRnd);
        hz = h * (z - Pos[2]);
        hx = h * (x - Pos[0]);
    }
    p.Vel[0] = Forward[0] * hz + Dir[0] * vs + Right[0] * hx;
    p.Vel[1] = Forward[1] * hz + Dir[1] * vs + Right[1] * hx;
    p.Vel[2] = Forward[2] * hz + Dir[2] * vs + Right[2] * hx;
    p.Vel0[0] = p.Vel[0];
    p.Vel0[1] = p.Vel[1];
    p.Vel0[2] = p.Vel[2];
    if (pr->BalancedBirth == 1) {
        float k = pr->VSpeed * Dt * frac;
        p.Pos[0] = Dir[0] * k + p.Pos[0];
        p.Pos[1] = Dir[1] * k + p.Pos[1];
        p.Pos[2] = Dir[2] * k + p.Pos[2];
    }
}

// PANZERS 0x6e9950
void SParticles::InitParticleTracks(int i)
{
    SPParticles* pr = Prototype;
    SParticleData& p = Particles[i];
    p.AlphaScale = AlphaScale;
    p.Alpha = pr->AlphaTrack.Evaluate(0.0f, &CurAlpha) * AlphaScale;
    p.SizeRnd = (float)((double)rand() * (1.0 / 32768.0) * (double)pr->SizeRnd) * 0.5f;
    p.Size = (pr->SizeTrack.Evaluate(0.0f, &CurSize) * 0.5f + p.SizeRnd) * SizeScale;
    p.AddSpeed = pr->SpeedTrack.Evaluate(0.0f, &CurSpeed);
    if (pr->DrawType == 2 && SPixie::Instance())
        p.SubEffect = SPixie::Instance()->CreateEffect(Scene, pr->SubEffectProto, p.Pos, Dir);
}

// PANZERS 0x6e8150
void SParticles::InitParticleSpin(int i)
{
    SPParticles* pr = Prototype;
    SParticleData& p = Particles[i];
    if (pr->MoveType == 0) {
        if (pr->ObjectSpin && p.Spin[0] == -1.0f) {
            p.Spin[0] = (float)((double)rand() * (1.0 / 32768.0) * 6.26573181);
            p.Spin[1] = (float)((double)rand() * (1.0 / 32768.0) * 6.26573181);
            p.Spin[2] = (float)((double)rand() * (1.0 / 32768.0) * 6.26573181);
            for (int k = 0; k < 3; ++k) {
                float rnd = pr->SpinRnd[k];
                p.SpinSpeed[k] = (pr->SpinMin[k] + (float)((double)rand() * (1.0 / 32768.0) * (double)rnd)) * 0.0174532924f;
            }
        }
    } else if (pr->MoveType == 1) {
        float x = Dir[0], z = Dir[2];
        if (x == 0.0f && z == 0.0f)
            x = 1.0f;
        double inv = 1.0 / sqrt((double)(z * z + x * x));
        p.Vel[0] = (float)(x * inv);
        p.Vel[2] = (float)(z * inv);
    }
}

// PANZERS 0x6e7df0
// SetDirection's pass over the live particles (same as InitParticleSpin).
void SParticles::UpdateParticleSpins()
{
    for (int i = 0; i < (int)Particles.size(); ++i)
        InitParticleSpin(i);
}

// HD 0x6e5b80: Draw "Object" particles get a model from the scene (+0x58).
void SParticles::InitParticleModel(int i)
{
    (void)i;
    if (Prototype->DrawType == 1)
        STUB_LOG("SParticles::InitParticleModel (0x6e5b80)");
}

// PANZERS 0x6e9540
void SParticles::RemoveParticle(int i)
{
    SPParticles* pr = Prototype;
    if (pr->DrawType == 2 && SPixie::Instance())
        SPixie::Instance()->StopEffect(Particles[i].SubEffect);
    if (i < 0 || i >= (int)Particles.size())
        Logger.g->Panic("SDArray<%s>::Remove: invalid index (%d) size = %d", "struct SParticleData", i,
                        (int)Particles.size());
    Particles.erase(Particles.begin() + i);   // 0x6e94b0 (memmove)
}

// PANZERS 0x6e5c40
int SParticles::WeatherIndex()
{
    const char* a = SceneAtmosphere(Scene);
    if (a && _stricmp(a, "Default") == 0)
        return 0;
    if (!a)
        return 0;
    if (_stricmp(a, "Snowy") == 0)
        return 1;
    return _stricmp(a, "Foggy") == 0 ? 2 : 0;
}

// PANZERS 0x6e32d0
// Additive particles fade by darkening; alpha particles carry the alpha.
unsigned SParticles::PackColor(int i, float r, float g, float b)
{
    SParticleData& p = Particles[i];
    if (Prototype->BlendType == 1) {
        int ir = (int)lrintf(p.Alpha * r);
        int ig = (int)lrintf(p.Alpha * g);
        int ib = (int)lrintf(p.Alpha * b);
        return (unsigned)(ib + (ir * 0x100 + ig) * 0x100);
    }
    int ia = (int)lrintf(p.Alpha * 255.0f);
    int ir = (int)lrintf(r);
    int ig = (int)lrintf(g);
    int ib = (int)lrintf(b);
    return (unsigned)(ib + ((ia * 0x100 + ir) * 0x100 + ig) * 0x100);
}

// The material lives in SParticles+0x188 in HD; one is enough here.
static SEffectMaterial s_Material;

// PANZERS 0x6e1490
void SParticles::SetupMaterial()
{
    SPParticles* pr = Prototype;
    s_Material.Reset();
    int w = WeatherIndex();
    if (pr->BlendType == 0) {
        RenderFlag = false;
        s_Material.SetTexture(pr->WeatherTex[w], false);
        s_Material.SetBlend(0, pr->WeatherTex[w]);
        s_Material.ColorOp = PzGepard()->GetCap(0x10) ? D3DTOP_MODULATE2X : D3DTOP_MODULATE;
        s_Material.ZEnable = pr->ZBuffer;
    } else if (pr->BlendType == 1) {
        RenderFlag = false;
        if (!pr->MultiFile)
            s_Material.SetTexture(pr->WeatherTex[WeatherIndex()], false);
        s_Material.SetBlend(1, -1);
        s_Material.ZEnable = pr->ZBuffer;
    } else {
        s_Material.CullMode = D3DCULL_NONE;
        return;
    }
    s_Material.CullMode = D3DCULL_NONE;   // +0x35c = 1
}

// PANZERS 0x6e95f0
void SParticles::Render(SIViewport* vp)
{
    SPParticles* pr = Prototype;
    if (pr->BirthStyle != 0 && Model && pr->Radius == 0.0f)
        STUB_LOG("SParticles::Render birth from model (SModel +0x10)");
    if (pr->DrawType == 3)
        return;
    SetupMaterial();
    if (pr->DrawType == 1) {
        RenderObjects(vp);
        return;
    }
    if (pr->DrawType != 0 && pr->DrawType != 2)
        return;
    switch (pr->ParticleType) {     // 0x6e5230
    case 0: RenderNormal(vp); break;
    case 1: RenderBillboard(vp); break;
    case 2: RenderCloud(vp); break;
    case 3: RenderTrail(vp); break;
    case 5: RenderSubEffect(vp); break;
    default: break;
    }
}

static void PutXYZRHW(float* v, float x, float y, float z, unsigned col, unsigned fog, float u, float t)
{
    v[0] = x; v[1] = y; v[2] = z; v[3] = 1.0f;
    memcpy(&v[4], &col, 4);
    memcpy(&v[5], &fog, 4);
    v[6] = u; v[7] = t;
}

static void PutXYZ(float* v, float x, float y, float z, unsigned col, float u, float t)
{
    v[0] = x; v[1] = y; v[2] = z;
    memcpy(&v[3], &col, 4);
    v[4] = u; v[5] = t;
}

// PANZERS 0x6e5230
// ParticleType 0 (the switch head of 0x6e5230; Ghidra merged the body at
// 0x6e4740.. into it). Newest particles first, so the oldest end on top.
void SParticles::RenderNormal(SIViewport* vp)
{
    SPParticles* pr = Prototype;
    SPixie* px = SPixie::Instance();
    int count = (int)Particles.size();
    if (count == 0)
        return;
    float* out = nullptr;
    int drawn = 0;
    if (!pr->MultiFile)
        out = EffectLockDynamicVB(px->ParticleVB, count * 6);
    for (int i = count - 1; i >= 0; --i) {
        SParticleData& p = Particles[i];
        float sx, sy, ss, z;
        unsigned fog;
        EffectProject(vp, p.Pos, p.Size, &sx, &sy, &ss, &z, &fog);
        if (!(0.0f <= ss))
            continue;
        if (pr->MultiFile) {
            out = EffectLockDynamicVB(px->ParticleVB, count * 6);
            drawn = 0;
        }
        float x0, y0, x1, y1, x2, y2, x3, y3;   // (L,T) (L,B) (R,T) (R,B) before rotation
        if (!pr->RandomRotate && !pr->ObjectSpin) {
            x0 = sx - ss; y0 = sy - ss;
            x1 = x0;      y1 = sy + ss;
            x2 = sx + ss; y2 = y0;
            x3 = x2;      y3 = y1;
        } else {
            int deg = (int)lrintf(p.Rotation);
            if (pr->ObjectSpin)
                deg = (int)(p.Spin[0] * 57.2957802f);
            float r = ss * 1.41419995f;
            x0 = EffectCosDeg(deg + 225) * r + sx; y0 = EffectSinDeg(deg + 225) * r + sy;
            x1 = EffectCosDeg(deg + 135) * r + sx; y1 = EffectSinDeg(deg + 135) * r + sy;
            x2 = EffectCosDeg(deg + 315) * r + sx; y2 = EffectSinDeg(deg + 315) * r + sy;
            x3 = EffectCosDeg(deg + 45) * r + sx;  y3 = EffectSinDeg(deg + 45) * r + sy;
        }
        float u0, u1;
        if (!pr->MultiFile) {
            u0 = (float)p.Frame * pr->InvTotalFrames;
            u1 = (float)(p.Frame + 1) * pr->InvTotalFrames;
        } else {
            u0 = 0.0f;
            u1 = 1.0f;
            if (p.Frame < 0 || p.Frame >= (int)pr->FrameTex.size())
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "int", p.Frame);
            s_Material.SetTexture(pr->FrameTex[p.Frame], false);
        }
        // point lighting from the emitter
        float dx = LightPos[1] - p.Pos[1], dy = LightPos[0] - p.Pos[0], dz = LightPos[2] - p.Pos[2];
        float f = (dx * dx + dy * dy + dz * dz) / LightRange2;
        if (1.0f < f)
            f = 1.0f;
        float r, g, b;
        if (pr->LightingModel == 0) {
            r = ColorF[0]; g = ColorF[1]; b = ColorF[2];
        } else {
            float I = LightIntensity;
            float l1r = (LightColor1[0] - ColorF[0]) * I + ColorF[0];
            float l1g = (LightColor1[1] - ColorF[1]) * I + ColorF[1];
            float l1b = (LightColor1[2] - ColorF[2]) * I + ColorF[2];
            if (pr->LightingModel == 2) {
                r = (LightColor2[0] - ColorF[0]) * I + ColorF[0];
                g = (LightColor2[1] - ColorF[1]) * I + ColorF[1];
                b = (LightColor2[2] - ColorF[2]) * I + ColorF[2];
                float k = (1.0f - f) * (1.0f - f);
                k = k * k;
                k = k * k;
                r = r - (r - l1r) * k;
                g = g - (g - l1g) * k;
                b = b - (b - l1b) * k;
                r = (ColorF[0] - r) * f + r;
                g = (ColorF[1] - g) * f + g;
                b = (ColorF[2] - b) * f + b;
            } else {
                g = (ColorF[1] - l1g) * f + l1g;
                r = (ColorF[0] - l1r) * f + l1r;
                b = (ColorF[2] - l1b) * f + l1b;
            }
            if (1.0f < r) r = 1.0f;
            if (1.0f < g) g = 1.0f;
            if (1.0f < b) b = 1.0f;
        }
        unsigned col = PackColor(i, r * 255.0f, g * 255.0f, b * 255.0f);
        ++drawn;
        PutXYZRHW(out + 0,  x0, y0, z, col, fog, u0, 0.0f);
        PutXYZRHW(out + 8,  x1, y1, z, col, fog, u0, 1.0f);
        PutXYZRHW(out + 16, x2, y2, z, col, fog, u1, 0.0f);
        PutXYZRHW(out + 24, x2, y2, z, col, fog, u1, 0.0f);
        PutXYZRHW(out + 32, x1, y1, z, col, fog, u0, 1.0f);
        PutXYZRHW(out + 40, x3, y3, z, col, fog, u1, 1.0f);
        out += 48;
        if (pr->MultiFile) {
            EffectUnlockDynamicVB();
            s_Material.Apply();
            EffectDrawDynamicVB(D3DPT_TRIANGLELIST, drawn * 2);
            EffectAdvanceDynamicVB(drawn * 6);
            drawn = 0;
        }
    }
    if (!pr->MultiFile) {
        EffectUnlockDynamicVB();
        s_Material.Apply();
        EffectDrawDynamicVB(D3DPT_TRIANGLELIST, drawn * 2);
        EffectAdvanceDynamicVB(drawn * 6);
    }
}

// Vertex colour of the world-space types: additive = colour * alpha * k,
// alpha blend = white with the alpha.
static unsigned WorldColor(const SParticles* e, const SParticleData& p, float k)
{
    if (e->Prototype->BlendType == 1) {
        int r = (int)lrintf((float)e->ColorB[0] * p.Alpha * k);
        int g = (int)lrintf((float)e->ColorB[1] * p.Alpha * k);
        int b = (int)lrintf((float)e->ColorB[2] * p.Alpha * k);
        return (unsigned)((r * 0x100 + g) * 0x100 + b);
    }
    int a = (int)lrintf(p.Alpha * 255.0f);
    return ((unsigned)a << 24) + 0xffffffu;
}

// PANZERS 0x6e38c0
// Vertical quads facing the camera yaw. Additive ones fade with the camera
// pitch: (1.571 - pitch) * 0.63.
void SParticles::RenderBillboard(SIViewport* vp)
{
    SPParticles* pr = Prototype;
    SPixie* px = SPixie::Instance();
    float cx, cy, cz, yaw, pitch;
    vp->GetCamera(&cx, &cy, &cz, &yaw, &pitch);
    int count = (int)Particles.size();
    if (count == 0)
        return;
    float* out = nullptr;
    int drawn = 0;
    if (!pr->MultiFile)
        out = EffectLockDynamicVB(px->WorldVB, count * 6);
    double c = -(double)yaw;
    for (int i = 0; i < count; ++i) {
        if (pr->MultiFile) {
            out = EffectLockDynamicVB(px->WorldVB, count * 6);
            drawn = 0;
        }
        SParticleData& p = Particles[i];
        double cs = cos(c), sn = sin(c);
        float s = p.Size;
        float lx = (float)((double)p.Pos[0] - (double)s * cs);
        float lz = (float)((double)p.Pos[2] - (double)s * sn);
        float rx = (float)((double)s * cs + (double)p.Pos[0]);
        float rz = (float)((double)s * sn + (double)p.Pos[2]);
        float yb = p.Pos[1] - s, yt = p.Pos[1] + s;
        float u0, u1;
        if (!pr->MultiFile) {
            u0 = (float)p.Frame * pr->InvTotalFrames;
            u1 = (float)(p.Frame + 1) * pr->InvTotalFrames;
        } else {
            u0 = 0.0f; u1 = 1.0f;
            s_Material.SetTexture(pr->FrameTex[p.Frame], false);
        }
        float elev = (float)((1.571 - (double)pitch) * 0.629999995);
        unsigned col = WorldColor(this, p, elev);
        ++drawn;
        PutXYZ(out + 0,  lx, yb, lz, col, u0, 1.0f);
        PutXYZ(out + 6,  lx, yt, lz, col, u0, 0.0f);
        PutXYZ(out + 12, rx, yb, rz, col, u1, 1.0f);
        PutXYZ(out + 18, rx, yb, rz, col, u1, 1.0f);
        PutXYZ(out + 24, lx, yt, lz, col, u0, 0.0f);
        PutXYZ(out + 30, rx, yt, rz, col, u1, 0.0f);
        out += 36;
        if (pr->MultiFile) {
            EffectUnlockDynamicVB();
            s_Material.Apply();
            EffectDrawDynamicVB(D3DPT_TRIANGLELIST, drawn * 2);
            EffectAdvanceDynamicVB(drawn * 6);
            drawn = 0;
        }
    }
    if (!pr->MultiFile) {
        EffectUnlockDynamicVB();
        s_Material.Apply();
        EffectDrawDynamicVB(D3DPT_TRIANGLELIST, drawn * 2);
        EffectAdvanceDynamicVB(drawn * 6);
    }
}

// PANZERS 0x6e3e90
// Horizontal quads at the particle height ("Random rotate" turns them).
void SParticles::RenderCloud(SIViewport* vp)
{
    SPParticles* pr = Prototype;
    SPixie* px = SPixie::Instance();
    float cx, cy, cz, yaw, pitch;
    vp->GetCamera(&cx, &cy, &cz, &yaw, &pitch);
    int count = (int)Particles.size();
    if (count == 0)
        return;
    float* out = nullptr;
    int drawn = 0;
    if (!pr->MultiFile)
        out = EffectLockDynamicVB(px->WorldVB, count * 6);
    for (int i = 0; i < count; ++i) {
        if (pr->MultiFile) {
            out = EffectLockDynamicVB(px->WorldVB, count * 6);
            drawn = 0;
        }
        SParticleData& p = Particles[i];
        float s = p.Size;
        float x0, z0, x1, z1, x2, z2, x3, z3;
        if (!pr->RandomRotate) {
            x0 = p.Pos[0] - s; z0 = p.Pos[2] - s;
            x1 = p.Pos[0] - s; z1 = p.Pos[2] + s;
            x2 = p.Pos[0] + s; z2 = p.Pos[2] - s;
            x3 = p.Pos[0] + s; z3 = p.Pos[2] + s;
        } else {
            float r = s * 1.41419995f;
            int d0 = (int)(p.Rotation + 225.0f);
            int d1 = (int)(p.Rotation + 135.0f);
            int d2 = (int)(p.Rotation + 315.0f);
            int d3 = (int)(p.Rotation + 45.0f);
            x0 = EffectCosDeg(d0) * r + p.Pos[0]; z0 = EffectSinDeg(d0) * r + p.Pos[2];
            x1 = EffectCosDeg(d1) * r + p.Pos[0]; z1 = EffectSinDeg(d1) * r + p.Pos[2];
            x2 = EffectCosDeg(d2) * r + p.Pos[0]; z2 = EffectSinDeg(d2) * r + p.Pos[2];
            x3 = EffectCosDeg(d3) * r + p.Pos[0]; z3 = EffectSinDeg(d3) * r + p.Pos[2];
        }
        float y = p.Pos[1];
        float u0, u1;
        if (!pr->MultiFile) {
            u0 = (float)p.Frame * pr->InvTotalFrames;
            u1 = (float)(p.Frame + 1) * pr->InvTotalFrames;
        } else {
            u0 = 0.0f; u1 = 1.0f;
            s_Material.SetTexture(pr->FrameTex[p.Frame], false);
        }
        float k = 1.0f;
        if (pr->ElevDependent)
            k = pitch * 0.629999995f;
        unsigned col = WorldColor(this, p, k);
        ++drawn;
        PutXYZ(out + 0,  x0, y, z0, col, u0, 0.0f);
        PutXYZ(out + 6,  x1, y, z1, col, u0, 1.0f);
        PutXYZ(out + 12, x2, y, z2, col, u1, 0.0f);
        PutXYZ(out + 18, x2, y, z2, col, u1, 0.0f);
        PutXYZ(out + 24, x1, y, z1, col, u0, 1.0f);
        PutXYZ(out + 30, x3, y, z3, col, u1, 1.0f);
        out += 36;
        if (pr->MultiFile) {
            EffectUnlockDynamicVB();
            s_Material.Apply();
            EffectDrawDynamicVB(D3DPT_TRIANGLELIST, drawn * 2);
            EffectAdvanceDynamicVB(drawn * 6);
            drawn = 0;
        }
    }
    if (!pr->MultiFile) {
        EffectUnlockDynamicVB();
        s_Material.Apply();
        EffectDrawDynamicVB(D3DPT_TRIANGLELIST, drawn * 2);
        EffectAdvanceDynamicVB(drawn * 6);
    }
}

// HD 0x6e5290: ParticleType 3, camera-facing ribbons along the particle
// direction (TrailLength). Not used by the menu effects.
void SParticles::RenderTrail(SIViewport* vp)
{
    (void)vp;
    STUB_LOG("SParticles::RenderTrail (0x6e5290)");
    PZ_TRACE("SParticles::RenderTrail (0x6e5290)");
}

// HD 0x6e5150: Draw "Object" (models per particle).
void SParticles::RenderObjects(SIViewport* vp)
{
    (void)vp;
    STUB_LOG("SParticles::RenderObjects (0x6e5150)");
    PZ_TRACE("SParticles::RenderObjects (0x6e5150)");
}

// HD 0x6e4660: Draw "Effect" (the sub-effects follow their particles).
void SParticles::RenderSubEffect(SIViewport* vp)
{
    (void)vp;
    STUB_LOG("SParticles::RenderSubEffect (0x6e4660)");
    PZ_TRACE("SParticles::RenderSubEffect (0x6e4660)");
}

} // namespace pz
