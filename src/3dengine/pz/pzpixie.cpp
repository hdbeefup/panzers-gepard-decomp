// src/3dengine/pz/pzpixie.cpp
// pz::SPixie, the HD effect manager (0x6941e0..0x69f7b0). OWNER: agent C.
// See pzpixie.h for the frame protocol and effect.h for the classes.

#include <string.h>
#include <stdlib.h>
#include <d3d9.h>
#include "pzpixie.h"
#include "pzmodel.h"
#include "effectrender.h"
#include "iviewport.h"
#include "igepardhd.h"
#include "properties.h"
#include "stream.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

static SPixie* s_Pixie = nullptr;                 // HD 0x92f104
static SPProperty* s_PPropertyEffects = nullptr;  // HD 0x92f108
static SPProperty* s_PPropertyEfx = nullptr;      // HD 0x92f100

SPixie* SPixie::Instance()
{
    return s_Pixie;
}

// PANZERS 0x6941e0
// The bulk of the HD ctor (35 KB) builds the two property schema trees; that
// part is generated into effectschema.cpp. The dynamic vertex buffers are
// SGepard formats in HD; here they are the FVF codes the draw helpers use
// (effectrender.cpp).
SPixie::SPixie()
    : Device(nullptr), _30(false), ParticleVB(-1), WorldVB(-1), FlareVB(-1), _54(0),
      RainIntensity(0.0f), _5c(0.0f), WindPhase(0.0f), DeferredHead(nullptr), DeferredTail(nullptr),
      DeferredCursor(nullptr), _70(false), DeferredCount(0), RefCount(1)
{
    PZ_TRACE("SPixie::SPixie (0x6941e0)");
    if (s_Pixie)
        Logger.g->Panic("SPixie::SPixie: Duplicate Pixie creation");
    s_Pixie = this;
    WindPhase = (float)((double)rand() * (1.0 / 32768.0) * 6.283);
    Device = EffectDevice();
    ParticleVB = EffectCreateDynamicVB(0x1c4);
    WorldVB = EffectCreateDynamicVB(0x142);
    FlareVB = EffectCreateDynamicVB(0x1c4);
    if (s_PPropertyEffects)
        Logger.g->Panic("SPixie::SPixie(): PProperty_Effects is not NULL (effect-prototype tree already exists).");
    s_PPropertyEffects = BuildEffectsSchema();
    s_PPropertyEfx = BuildEfxSchema();
}

// PANZERS 0x69cb90
SPixie::~SPixie()
{
    PZ_TRACE("SPixie::~SPixie (0x69cb90)");
    if (RefCount != 0)
        Logger.g->Panic("SPixie::~SPixie(): Deleted instead of release.");
    delete s_PPropertyEfx;
    s_PPropertyEfx = nullptr;
    delete s_PPropertyEffects;
    s_PPropertyEffects = nullptr;
    if (Effects.Count > 0)
        Logger.g->Log(0, "%d effects leaked.", Effects.Count);   // HD panics
    for (int i = 0; i < Prototypes.Size(); ++i) {
        if (Prototypes.Valid(i) && Prototypes[i]) {
            Logger.g->Log(0, "SPixie::~SPixie(): Leaked effect prototype: \"%s\"",
                          Prototypes[i]->FileName.c_str());
            delete Prototypes[i];
            Prototypes.Remove(i);
        }
    }
    s_Pixie = nullptr;
    SPixieDeferredPlay* p = DeferredHead;
    while (p) {
        SPixieDeferredPlay* n = p->Next;
        delete p;
        p = n;
    }
}

// PANZERS 0x69d1d0
void SPixie::AddRef()
{
    PZ_TRACE("SPixie::AddRef (0x69d1d0)");
    ++RefCount;
}

// PANZERS 0x69e8a0
void SPixie::Release()
{
    PZ_TRACE("SPixie::Release (0x69e8a0)");
    if (--RefCount == 0)
        delete this;   // ~SPixie 0x69cb90 + operator delete(0x7c)
}

// PANZERS 0x69edb0
void SPixie::ReloadResources(int unused)
{
    PZ_TRACE("SPixie::ReloadResources (0x69edb0)");
    (void)unused;
    for (int i = 0; i < Prototypes.Size(); ++i)
        if (Prototypes.Valid(i))
            Prototypes[i]->ReloadResources();
}

// PANZERS 0x69e5a0
// Rebuilds the prototype set from its property tree: drops the old entries,
// then InitEffectPrototype (+0x68) for each "Effects.N.Data". Returns 0, or
// -1 when the tree is not an "Effects" array or an entry fails.
int SPixie::RefreshEffectPrototype(int proto)
{
    PZ_TRACE("SPixie::RefreshEffectPrototype (0x69e5a0)");
    if (!Prototypes.Valid(proto))
        return -1;
    SPEffectSet* set = Prototypes[proto];
    SPropertyArray* tree = set->Properties;
    if (!tree)
        Logger.g->Panic("SPixie::RefreshEffectPrototype(): Effect property-tree is NULL.");
    for (int i = 0; i < set->Effects.Size(); ++i) {
        if (set->Effects.Valid(i)) {
            delete set->Effects[i];
        }
    }
    // HD resets the heap in place (+0x08 = 0, +0x10 = -1, +0x14 = 0).
    set->Effects.Used = 0;
    set->Effects.FreeHead = -1;
    set->Effects.Count = 0;
    if (tree->Type() != PROPERTY_TYPE_ARRAY || _stricmp(tree->Name.c_str(), "Effects") != 0)
        return -1;
    char name[32];
    for (int i = 0; i < (int)tree->Children.size(); ++i) {
        sprintf(name, "%d.Data", i);
        SProperty* data = tree->Children[i];
        if (data->Type() != PROPERTY_TYPE_STRUCT || _stricmp(data->Name.c_str(), name) != 0)
            return -1;
        if (InitEffectPrototype(proto, "ExtendedFx2", static_cast<SPropertyStruct*>(data)) < 0)
            return -1;
    }
    return 0;
}

// PANZERS 0x69dc80
// A file already loaded returns its handle with one more reference. The
// file is opened once to test that it exists (HD logs a Hungarian help text
// and returns -1 otherwise). checkTimeStamp compares [Version] TimeStamp
// with the exe build date and panics on newer files ("ViewEffects.exe is
// too old"); unitFile/unitFileLen is an SString passed by value (the unit
// file name for the error text), freed here.
int SPixie::LoadEffectPrototype(const char* file, bool keepProperties, bool checkTimeStamp, int unitFile, int unitFileLen)
{
    PZ_TRACE("SPixie::LoadEffectPrototype (0x69dc80)");
    (void)unitFileLen;
    int result = -1;
    for (int i = 0; i < Prototypes.Size(); ++i) {
        if (!Prototypes.Valid(i))
            continue;
        SPEffectSet* set = Prototypes[i];
        bool same = set->FileName.empty() ? (!file || !*file)
                                          : (file && *file && strcmp(set->FileName.c_str(), file) == 0);
        if (same) {
            set->AddRef();
            if (unitFile)
                delete[] (char*)unitFile;
            return i;
        }
    }

    SStream* s = FileSystem.OpenRead(file, nullptr);
    if (!s) {
        Logger.g->Log(0, "Nem sikerult betolteni az effektet, hibas a kovetkezo file neve: %s\n"
                         "(SPixie::LoadEffectPrototype)\n(unit effekt eseten a unit leiro file neve: %s)",
                      file ? file : "", unitFile ? (const char*)unitFile : "");
    } else {
        s->Release();
        SPropertyArray* props = static_cast<SPropertyArray*>(s_PPropertyEffects->CreateInstance());
        result = Prototypes.Add();
        SPEffectSet* set = new SPEffectSet();
        Prototypes[result] = set;
        set->Index = result;                     // 0x6df3c0
        set->FileName = file ? file : "";
        set->Properties = props;
        set->ForceUpdate = false;
        SProperties* ini = new SProperties(file, true);
        if (checkTimeStamp) {
            // Build date of the HD exe ("Wed Nov  4 09:37:38 2015") as
            // "2015.11.04.09:37:38"; spaces in both strings become '0'.
            char built[] = "2015.11.04.09:37:38";
            const char* ts = ini->GetString("Version", "TimeStamp", "");
            char stamp[64];
            strncpy(stamp, ts ? ts : "", sizeof(stamp) - 1);
            stamp[sizeof(stamp) - 1] = 0;
            for (char* c = stamp; *c; ++c)
                if (*c == ' ')
                    *c = '0';
            if (strcmp(stamp, built) > 0)
                Logger.g->Panic("SPixie::LoadEffectPrototype(): TimeStamp mismatch.\n\nViewEffects.exe is too old.");
        }
        if (props->Load(ini, "ExtendedFx2", nullptr))
            RefreshEffectPrototype(result);
        delete ini;
    }
    if (!keepProperties && Prototypes.Valid(result) && Prototypes[result]->Properties) {
        delete Prototypes[result]->Properties;
        Prototypes[result]->Properties = nullptr;
    }
    if (unitFile)
        delete[] (char*)unitFile;
    return result;
}

// HD SPixie vtbl +0x14 -> 0x69ee50 (2 arg dwords): writes a prototype back
// to a .fx file (effect editor).
void SPixie::Slot_14()
{
    STUB_LOG("SPixie::Slot_14 (0x69ee50)");
    PZ_TRACE("SPixie::Slot_14 (0x69ee50)");
}

// PANZERS 0x69d2a0
int SPixie::CreateEffectPrototype()
{
    PZ_TRACE("SPixie::CreateEffectPrototype (0x69d2a0)");
    int i = Prototypes.Add();
    SPEffectSet* set = new SPEffectSet();
    Prototypes[i] = set;
    set->Index = i;
    set->FileName = "**untitled.fx";
    set->Properties = static_cast<SPropertyArray*>(s_PPropertyEffects->CreateInstance());
    return i;
}

// PANZERS 0x69d5b0
void* SPixie::GetEffectProperties(int proto)
{
    PZ_TRACE("SPixie::GetEffectProperties (0x69d5b0)");
    if (!Prototypes.Valid(proto))
        return nullptr;
    return Prototypes[proto]->Properties;
}

// PANZERS 0x69e8c0
void SPixie::ReleaseEffectPrototype(int proto)
{
    PZ_TRACE("SPixie::ReleaseEffectPrototype (0x69e8c0)");
    if (Prototypes.Valid(proto))
        Prototypes[proto]->Release();   // 0x6df000
}

// PANZERS 0x69f780
// (0x69e9f0: frees the slot; the set deletes itself)
void SPixie::RemovePrototype(int proto)
{
    if (Prototypes.Valid(proto))
        Prototypes.Remove(proto);
}

// PANZERS 0x69e400
// p5 is a float passed as a dword: an instance lifetime override for the
// particles (SParticles ctor 0x6e0980; 0 = the .fx LifeTime).
void SPixie::PlayEffect(SIScene* scene, int proto, const float* pos, const float* dir, int p5)
{
    PZ_TRACE("SPixie::PlayEffect (0x69e400)");
    if (proto < 0)
        return;
    int h = Effects.Add();
    if (!Prototypes.Valid(proto))
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "struct SPEffectSet *", proto);
    float param;
    memcpy(&param, &p5, sizeof(param));
    Effects[h] = Prototypes[proto]->CreateInstance(scene, h, param, 0);
    Effects[h]->SetPosition(pos);
    Effects[h]->SetDirection(dir);
}

// PANZERS 0x69e510
// A one-shot set (it deletes itself when its instances end) created with the
// model (births from the model mesh) and hung on the node: the model updates
// it with the node's world matrix from then on.
void SPixie::PlayEffectOnNode(SIScene* scene, int proto, SIModel* model, int node, int p5)
{
    PZ_TRACE("SPixie::PlayEffectOnNode (0x69e510)");
    if (proto < 0 || node < 0)
        return;
    int h = Effects.Add();                                    // 0x69cff0
    if (!Prototypes.Valid(proto))                             // 0x69ced0
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "struct SPEffectSet *", proto);
    float param;
    memcpy(&param, &p5, sizeof(param));
    Effects[h] = Prototypes[proto]->CreateInstance(scene, h, param, (int)(intptr_t)model);   // 0x6dede0
    static_cast<SModel*>(model)->AttachChild(node, Effects[h]);                             // 0x6d5940
}

// PANZERS 0x69f540
// Like PlayEffect, but the set is persistent (+0x30): it stays registered
// when it runs empty, until StopEffect / ReleaseEffect / DestroyEffect.
int SPixie::CreateEffect(SIScene* scene, int proto, const float* pos, const float* dir)
{
    PZ_TRACE("SPixie::CreateEffect (0x69f540)");
    if (proto < 0)
        return -1;
    int h = -1;
    if (Prototypes.Valid(proto) && Prototypes[proto]) {
        h = Effects.Add();
        Effects[h] = Prototypes[proto]->CreateInstance(scene, h, 0.0f, 0);
        Effects[h]->SetPosition(pos);
        Effects[h]->SetDirection(dir);
        Effects[h]->SetPersistent(true);
    }
    return h;
}

// PANZERS 0x69f610
// As PlayEffectOnNode, but persistent (+0x30) until StopEffect.
int SPixie::CreateEffectOnNode(SIScene* scene, int proto, SIModel* model, int node)
{
    PZ_TRACE("SPixie::CreateEffectOnNode (0x69f610)");
    if (proto < 0 || node < 0)
        return -1;
    int h = Effects.Add();                                    // 0x69cff0
    if (!Prototypes.Valid(proto))                             // 0x69ced0
        Logger.g->Panic("SHeap<%s>::operator[]: invalid index (%d)", "struct SPEffectSet *", proto);
    Effects[h] = Prototypes[proto]->CreateInstance(scene, h, 0.0f, (int)(intptr_t)model);   // 0x6dede0
    static_cast<SModel*>(model)->AttachChild(node, Effects[h]);                             // 0x6d5940
    Effects[h]->SetPersistent(true);                                                       // 0x6df3d0
    return h;
}

// PANZERS 0x69f6b0
void SPixie::StopEffect(int effect)
{
    PZ_TRACE("SPixie::StopEffect (0x69f6b0)");
    if (!Effects.Valid(effect))
        return;
    if (Effects[effect]) {
        Effects[effect]->SetPersistent(false);
        Effects[effect]->Stop();
        return;
    }
    Effects.Remove(effect);
}

// PANZERS 0x69d1e0
void SPixie::ReleaseEffect(int effect)
{
    PZ_TRACE("SPixie::ReleaseEffect (0x69d1e0)");
    if (!Effects.Valid(effect))
        return;
    if (Effects[effect]) {
        Effects[effect]->SetPersistent(false);
        return;
    }
    Effects.Remove(effect);
}

// PANZERS 0x69d540
void SPixie::DestroyEffect(int effect)
{
    PZ_TRACE("SPixie::DestroyEffect (0x69d540)");
    if (!Effects.Valid(effect) || !Effects[effect])
        return;
    SEffectSet* set = Effects[effect];
    Effects[effect] = nullptr;
    set->Handle = -1;          // its dtor must not unregister the slot again
    delete set;
    Effects.Remove(effect);
}

// PANZERS 0x69f710
// Called by the SEffectSet dtor. A persistent set leaves its slot (with a
// null pointer) until the owner stops or releases it.
void SPixie::UnregisterEffect(int effect)
{
    PZ_TRACE("SPixie::UnregisterEffect (0x69f710)");
    if (effect < 0)
        return;
    if (!Effects.Valid(effect))
        Logger.g->Panic("SPixie::UnregisterEffect(): Invalid index specified.");
    if (!Effects[effect] || !Effects[effect]->IsPersistent()) {
        Effects.Remove(effect);
        return;
    }
    Effects[effect] = nullptr;
}

// PANZERS 0x69d410
// HD first calls scene +0x84 (not ported in SIScene yet).
void SPixie::DestroySceneEffects(SIScene* scene)
{
    PZ_TRACE("SPixie::DestroySceneEffects (0x69d410)");
    for (int i = 0; i < Effects.Size(); ++i)
        if (Effects.Valid(i) && Effects[i] && Effects[i]->Scene == scene)
            delete Effects[i];
}

// PANZERS 0x69f7b0
// Once per rendered frame. With a terrain, effects outside the visible map
// are deactivated (no births) unless ForceUpdate; then every set updates
// (and may delete itself). Last, the deferred plays run.
void SPixie::UpdateFrame(int frame)
{
    PZ_TRACE("SPixie::UpdateFrame (0x69f7b0)");
    void* terrain = nullptr;
    bool first = true;
    for (int i = 0; i < Effects.Size(); ++i) {
        if (!Effects.Valid(i) || !Effects[i])
            continue;
        SEffectSet* set = Effects[i];
        if (first) {
            first = false;
            terrain = SceneTerrain(set->Scene);
        }
        if (terrain && !set->Proto->ForceUpdate) {
            // HD: SetActive(STerrain 0x69db40 "visible at (x, z)"). The
            // terrain visibility map is agent B's; until then everything is
            // visible.
            float p[3];
            set->GetPosition(p);
            set->SetActive(true);
        } else {
            set->SetActive(true);
        }
        Effects[i]->Update(frame, 0);
    }
    DeferredCursor = DeferredHead;
    while (DeferredCursor) {
        SPixieDeferredPlay* d = DeferredCursor;
        PlayEffect(d->Scene, d->Proto, d->Pos, d->Dir, 0);
        SPixieDeferredPlay* next = d->Next;
        SPixieDeferredPlay* prev = d->Prev;
        if (next) next->Prev = prev;
        if (prev) prev->Next = next;
        if (DeferredHead == d) DeferredHead = next;
        if (DeferredTail == d) DeferredTail = prev;
        delete d;
        --DeferredCount;
        DeferredCursor = next;
    }
}

// PANZERS 0x69f370
void SPixie::SetEffectPosition(int effect, const float* pos)
{
    PZ_TRACE("SPixie::SetEffectPosition (0x69f370)");
    if (Effects.Valid(effect) && Effects[effect])
        Effects[effect]->SetPosition(pos);
}

// PANZERS 0x69f2d0
void SPixie::SetEffectDirection(int effect, const float* dir)
{
    PZ_TRACE("SPixie::SetEffectDirection (0x69f2d0)");
    if (Effects.Valid(effect) && Effects[effect])
        Effects[effect]->SetDirection(dir);
}

// HD SPixie vtbl +0x54 -> 0x69f460 (2 arg dwords): SEffectSet +0x18.
void SPixie::Slot_54()
{
    STUB_LOG("SPixie::Slot_54 (0x69f460)");
    PZ_TRACE("SPixie::Slot_54 (0x69f460)");
}

// HD SPixie vtbl +0x58 -> 0x69f320 (2 arg dwords): SEffectSet::SetModel 0x6df2c0.
void SPixie::Slot_58()
{
    STUB_LOG("SPixie::Slot_58 (0x69f320)");
    PZ_TRACE("SPixie::Slot_58 (0x69f320)");
}

// PANZERS 0x69dc20
bool SPixie::IsBulletIndicator(int proto)
{
    PZ_TRACE("SPixie::IsBulletIndicator (0x69dc20)");
    if (!Prototypes.Valid(proto) || !Prototypes[proto])
        return false;
    return Prototypes[proto]->IsBulletIndicator();
}

// PANZERS 0x69f410
void SPixie::SetEffectEnabled(int effect, bool on)
{
    PZ_TRACE("SPixie::SetEffectEnabled (0x69f410)");
    if (Effects.Valid(effect) && Effects[effect])
        Effects[effect]->SetEnabled(on);
}

// PANZERS 0x69f4e0
void SPixie::SetEffectSpeed(int effect, float p1, float p2)
{
    PZ_TRACE("SPixie::SetEffectSpeed (0x69f4e0)");
    if (Effects.Valid(effect) && Effects[effect])
        Effects[effect]->SetSpeed(p1, p2);                    // set +0x24
}

// PANZERS 0x69d6a0
// `data` is "N.Data" {Enabled, Name, PriorityLayer, ForceUpdate,
// EffectType}. Note that HD never reads "Enabled" (the effect editor's
// switch): disabled entries play too. The EffectType alternative picks the
// class; only particles (0) are ported, the other types log and are skipped.
int SPixie::InitEffectPrototype(int proto, const char* name, SPropertyStruct* data)
{
    PZ_TRACE("SPixie::InitEffectPrototype (0x69d6a0)");
    if (data->Type() != PROPERTY_TYPE_STRUCT)
        Logger.g->Panic("SPixie::InitEffectPrototype(): Invalid effect prototype property.");
    if ((int)data->Children.size() < 5)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SProperty *", 4);
    SProperty* et = data->Children[4];
    if (et->Type() != PROPERTY_TYPE_MULTI && _stricmp(et->Name.c_str(), "EffectType") != 0)
        Logger.g->Panic("SPixie::InitEffectPrototype(): Invalid effect prototype property.");
    SPropertyMulti* multi = static_cast<SPropertyMulti*>(et);
    int type = multi->Index;

    SPEffect* e = nullptr;
    switch (type) {
    case 0:
        e = new SPParticles();          // new 0x1a0, 0x6e08b0
        break;
    case 1:  STUB_LOG("SPFlare (EffectType 1, 0x6df880)"); break;
    case 2:  STUB_LOG("SPRain (EffectType 2, 0x6ea820)"); break;
    case 3:  STUB_LOG("SPSnowfall (EffectType 3, 0x6eb5e0)"); break;
    case 4:  STUB_LOG("SPDecalEffect (EffectType 4, 0x6ea0f0)"); break;
    case 5:  STUB_LOG("SPSoundEffect (EffectType 5, 0x6ec900)"); break;
    case 6:  STUB_LOG("SPAtmosphere (EffectType 6, 0x694100)"); break;
    case 7:  STUB_LOG("SPLiteEffect (EffectType 7, 0x6ed790)"); break;
    case 8:  STUB_LOG("SPTrailEffect (EffectType 8, 0x6edcd0)"); break;
    case 9:  STUB_LOG("SPShockWave (EffectType 9, 0x6de5a0)"); break;
    case 10: STUB_LOG("SPCameraShake (EffectType 10, 0x6ee390)"); break;
    case 11: STUB_LOG("SPSandstorm (EffectType 11, 0x6ee8e0)"); break;
    default: {
        SPEffectSet* set = Prototypes[proto];
        Logger.g->Panic("Invalid EffectType (%d) defined for \"%s\".", type, set->FileName.c_str());
    }
    }
    if (!e)
        return -1;

    e->PriorityLayer = data->GetInt("PriorityLayer");
    e->ForceUpdate = data->GetBool("ForceUpdate");
    SPEffectSet* set = Prototypes[proto];
    set->ForceUpdate |= e->ForceUpdate;
    int idx = set->Effects.Add();
    set->Effects[idx] = e;
    e->Name = name ? name : "";
    SPropertyStruct* typeStruct = multi->Current();   // 0x69d620
    if (e->Init(typeStruct, name)) {
        delete e;
        set->Effects.Remove(idx);
        return -1;
    }
    return idx;
}

// PANZERS 0x69ea50
void SPixie::Render(SIScene* scene, SIViewport* vp)
{
    PZ_TRACE("SPixie::Render (0x69ea50)");
    EffectBeginRender(vp);                       // 0x680fe0: world = identity
    for (int layer = 0; layer < 3; ++layer) {
        for (int i = 0; i < Effects.Size(); ++i) {
            if (Effects.Valid(i) && Effects[i] && Effects[i]->Scene == scene)
                Effects[i]->Render(vp, layer);
        }
    }
    // HD then reads the camera (vp +0x24) and draws the lens flares (heap
    // +0x3c, SFlare +0x34/+0x38/+0x3c/+0x40). Flares are not ported; the
    // heap stays empty.
    EffectEndRender();
}

} // namespace pz
