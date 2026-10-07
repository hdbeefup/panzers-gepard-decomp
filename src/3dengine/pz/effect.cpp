// src/3dengine/pz/effect.cpp
// Effect prototypes and instances: SPEffect, SPEffectSet, SEffect,
// SEffectSet (HD 0x69cf50.., 0x6de530..0x6df6d0). OWNER: agent C.
// Particles are in particles.cpp.

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "effect.h"
#include "pzpixie.h"
#include "pzmodel.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

float (*g_EffectGroundHeight)(float x, float z) = nullptr;
float (*g_EffectWaterHeight)(float x, float z) = nullptr;

// HD: (double)rand() * (1/32768.0) (DAT_007f4540), CRT rand 0x78c846.
float EffectRand()
{
    return (float)((double)rand() * (1.0 / 32768.0));
}

// HD SLogger 0x65ca40 ("last error"; see effect.h)
void EffectSetLastError(const char* fmt, ...)
{
    char buf[512];
    va_list ap;
    va_start(ap, fmt);
    _vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
    va_end(ap);
    buf[sizeof(buf) - 1] = 0;
    if (Logger.g)
        Logger.g->Log(0, "SLogger last error: %s", buf);
}

// ---- scene fields (HD SScene offsets; agent A's class) ----
unsigned SceneFrameCount(SIScene* s) { return *(unsigned*)((char*)s + 0xa0); }
int SceneFrameMs(SIScene* s) { return *(int*)((char*)s + 0xac); }
const float* SceneLightColor(SIScene* s) { return (const float*)((char*)s + 0xf8); }
const char* SceneAtmosphere(SIScene* s)
{
    char* name = *(char**)((char*)s + 0x170);
    int len = *(int*)((char*)s + 0x174);
    return len ? name : nullptr;
}
void* SceneTerrain(SIScene* s) { return *(void**)((char*)s + 0x1c8); }

template <class T>
T*& SPtrHeap<T>::operator[](int i)
{
    if (!Valid(i))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "pointer", i);
    return Slots[i].Ptr;
}

template <class T>
void SPtrHeap<T>::Remove(int i)
{
    if (!Valid(i))
        Logger.g->Panic("SHeap<%s>::Remove: invalid index (%d)", "pointer", i);
    Slots[i].Use = FreeHead;
    Slots[i].Ptr = nullptr;
    FreeHead = i;
    --Count;
}

template struct SPtrHeap<SPEffect>;
template struct SPtrHeap<SEffect>;
template struct SPtrHeap<SPEffectSet>;
template struct SPtrHeap<SEffectSet>;
template struct SPtrHeap<SFlare>;   // SPixie +0x3c (M6-WX)

// ===========================================================================
// SPEffect
// ===========================================================================

// PANZERS 0x6de5a0
SPEffect::SPEffect()
    : PriorityLayer(0), ForceUpdate(false)
{
}

// PANZERS 0x69cf50
SPEffect::~SPEffect()
{
}

// PANZERS 0x69dc10
bool SPEffect::IsBulletIndicator()
{
    return false;
}

// PANZERS 0x69ee20
void SPEffect::ReloadResources()
{
}

// PANZERS 0x69d680
bool SPEffect::Init(SPropertyStruct* typeStruct, const char* name)
{
    (void)typeStruct; (void)name;
    Logger.g->Warning("SPEffect::Init(): Method not implemented.");
    return false;
}

// PANZERS 0x69d660
void SPEffect::Slot_10()
{
    Logger.g->Panic("SPEffect::Init(): Method not implemented.");
}

// ===========================================================================
// SPEffectSet
// ===========================================================================

// PANZERS 0x6dea70
SPEffectSet::SPEffectSet()
    : Index(-1), RefCount(1), ForceUpdate(false), Properties(nullptr)
{
}

// PANZERS 0x6dec10
SPEffectSet::~SPEffectSet()
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            delete Effects[i];
    delete Properties;
    Properties = nullptr;
}

// PANZERS 0x6def00
bool SPEffectSet::IsBulletIndicator()
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            return Effects[i]->IsBulletIndicator();
    return false;
}

// PANZERS 0x6df140
void SPEffectSet::ReloadResources()
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            Effects[i]->ReloadResources();
}

// PANZERS 0x6df000
void SPEffectSet::Release()
{
    if (--RefCount == 0) {
        SPixie::Instance()->RemovePrototype(Index);   // 0x69f780
        delete this;
    }
}

// PANZERS 0x6dede0
SEffectSet* SPEffectSet::CreateInstance(SIScene* scene, int handle, float param, int model)
{
    return new SEffectSet(scene, this, handle, param, model);
}

// ===========================================================================
// SEffect
// ===========================================================================

// PANZERS 0x6de530
SEffect::SEffect(SPEffect* proto)
    : LastFrame(0), Enabled(true), Model(0), Active(true), Proto(proto), Set(nullptr), SetIndex(0)
{
    Pos[0] = Pos[1] = Pos[2] = 0.0f;
    PrevPos[0] = PrevPos[1] = PrevPos[2] = 0.0f;
    Dir[0] = 0.0f; Dir[1] = 1.0f; Dir[2] = 0.0f;
}

// PANZERS 0x6de620
// (0x6de5d0 body: unregister from the owning set)
SEffect::~SEffect()
{
    if (Set)
        Set->UnregisterEffect(SetIndex);
}

// PANZERS 0x6de7a0
void SEffect::SetPosition(const float* pos)
{
    Pos[0] = pos[0]; Pos[1] = pos[1]; Pos[2] = pos[2];
}

// PANZERS 0x6de6d0
void SEffect::SetDirection(const float* dir)
{
    Dir[0] = dir[0]; Dir[1] = dir[1]; Dir[2] = dir[2];
    double inv = 1.0 / sqrt((double)(Dir[0] * Dir[0] + Dir[1] * Dir[1] + Dir[2] * Dir[2]));
    Dir[0] = (float)(Dir[0] * inv);
    Dir[1] = (float)(Dir[1] * inv);
    Dir[2] = (float)(Dir[2] * inv);
}

// PANZERS 0x6de6c0
void SEffect::SetAlphaScale(float) {}
// PANZERS 0x6de7c0
void SEffect::SetSizeScale(float) {}
// PANZERS 0x6de7f0
void SEffect::Slot_14(int) {}

// PANZERS 0x6de810
// `matrix` is a 4x3 node matrix: row 1 (+0x0c) is the direction, row 3
// (+0x24) the position. When Process() reports the effect finished, the
// instance deletes itself (which unregisters it from its set).
void SEffect::Update(unsigned frame, const float* matrix)
{
    if (LastFrame < frame) {
        LastFrame = frame;
        if (matrix) {
            float p[3] = { matrix[9], matrix[10], matrix[11] };
            SetPosition(p);
            float d[3] = { matrix[3], matrix[4], matrix[5] };
            SetDirection(d);
        }
        if (Process()) {
            PrevPos[0] = Pos[0]; PrevPos[1] = Pos[1]; PrevPos[2] = Pos[2];
            return;
        }
        delete this;
    }
}

// PANZERS 0x6de6a0
bool SEffect::Process() { return false; }
// PANZERS 0x6de6b0
void SEffect::Render(SIViewport*) {}
// PANZERS 0x6de690
void SEffect::SetEnabled(bool on) { Enabled = on; }
// PANZERS 0x6de7e0
void SEffect::SetActive(bool on) { Active = on; }
// PANZERS 0x6de800
void SEffect::Stop() {}
// PANZERS 0x6de7d0
void SEffect::SetSpeed(float, float) {}

// ===========================================================================
// SEffectSet
// ===========================================================================

// PANZERS 0x6de8c0
SEffectSet::SEffectSet(SIScene* scene, SPEffectSet* proto, int handle, float param, int model)
    : Scene(scene), Proto(proto), Handle(handle), LastFrame(0),
      Persistent(false), Enabled(true), Active(true), Model(model)
{
    proto->AddRef();
    for (int i = 0; i < proto->Effects.Size(); ++i) {
        if (!proto->Effects.Valid(i))
            continue;
        int k = Effects.Add();
        SEffect* e = proto->Effects[i]->CreateInstance(scene, param);
        Effects[k] = e;
        SetModel(Model);
        Effects[k]->SetOwner(this, k);
    }
}

// PANZERS 0x6deaf0
// (0x6ded00 is the deleting wrapper)
SEffectSet::~SEffectSet()
{
    for (int i = 0; i < Effects.Size(); ++i) {
        if (Effects.Valid(i) && Effects[i]) {
            SEffect* e = Effects[i];
            e->Set = nullptr;          // the set is going away; skip UnregisterEffect
            delete e;
        }
    }
    SPixie::Instance()->UnregisterEffect(Handle);
    Proto->Release();
    // then the SAttachable dtor (0x6d5070 -> 0x6d5710) leaves the parent node
}

// PANZERS 0x6df6d0
// Called by the parent model with its node's world matrix (SModel::Update),
// or by SPixie::UpdateFrame with none; a set on a model node then asks the
// model for the node matrix (+0x58 GetNodeMatrix). The first call of a frame
// wins.
int SEffectSet::Update(int frameArg, int matrixArg)
{
    unsigned frame = (unsigned)frameArg;
    const float* matrix = (const float*)(intptr_t)matrixArg;
    if (LastFrame < frame) {
        LastFrame = frame;
        float node[12];
        const float* m = nullptr;
        if (AttachParent) {
            m = matrix;
            if (!m) {
                static_cast<SModel*>(AttachParent)->GetNodeMatrix(node, AttachNode);   // model +0x58
                m = node;
            }
        }
        for (int i = 0; i < Effects.Size(); ++i)
            if (Effects.Valid(i))
                Effects[i]->Update(frame, m);
        if (Effects.Count == 0) {
            delete this;
            return 0;
        }
    }
    return 1;
}

// PANZERS 0x6df620
void SEffectSet::Stop()
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            Effects[i]->Stop();
}

// PANZERS 0x6def80
void SEffectSet::SetEnabled(bool on)
{
    Enabled = on;
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            Effects[i]->SetEnabled(Enabled);
}

// PANZERS 0x6df340
void SEffectSet::SetPosition(const float* pos)
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            Effects[i]->SetPosition(pos);
}

// PANZERS 0x6df240
void SEffectSet::SetDirection(const float* dir)
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            Effects[i]->SetDirection(dir);
}

// PANZERS 0x6df590
void SEffectSet::Slot_18(int p)
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            Effects[i]->Slot_14(p);
}

// PANZERS 0x6df090
// Only sets updated in the scene's current frame are drawn.
void SEffectSet::Render(SIViewport* vp, int layer)
{
    if (LastFrame != SceneFrameCount(Scene))
        return;
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i) && Effects[i]->Proto->PriorityLayer == layer)
            Effects[i]->Render(vp);
}

// PANZERS 0x6df510
void SEffectSet::SetActive(bool on)
{
    Active = on;
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            Effects[i]->SetActive(Active);
}

// PANZERS 0x6df470
void SEffectSet::SetSpeed(float p1, float p2)
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            Effects[i]->SetSpeed(p1, p2);
}

// PANZERS 0x6df1b0
void SEffectSet::SetAlphaScale(float s)
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            Effects[i]->SetAlphaScale(s);
}

// PANZERS 0x6df3e0
void SEffectSet::SetSizeScale(float s)
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i))
            Effects[i]->SetSizeScale(s);
}

// PANZERS 0x6df2c0
void SEffectSet::SetModel(int model)
{
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i) && Effects[i])
            Effects[i]->SetModel(model);
}

// PANZERS 0x6df690
void SEffectSet::UnregisterEffect(int index)
{
    if (!Effects.Valid(index))
        Logger.g->Panic("SEffectSet::UnregisterEffect(): Invalid index specified.");
    Effects.Remove(index);
}

// PANZERS 0x6dee60
void SEffectSet::GetPosition(float* out)
{
    for (int i = 0; i < Effects.Size(); ++i) {
        if (Effects.Valid(i)) {
            out[0] = Effects[i]->Pos[0];
            out[1] = Effects[i]->Pos[1];
            out[2] = Effects[i]->Pos[2];
            return;
        }
    }
    out[0] = out[1] = out[2] = 0.0f;
}

} // namespace pz
