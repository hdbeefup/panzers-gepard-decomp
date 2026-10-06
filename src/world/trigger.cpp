// src/world/trigger.cpp
// Trigger data loaders (TRIG chunk) and the other trigger data chunks of the
// map: locations (LOCS), paths (PATH) and trigger variables (TVAR).
// OWNER: agent L. Lifted from the HD exe. The S.W.I.N.E. loaders use the same
// read-by-property-mask scheme, but HD has other sizes, masks and type
// numbers, so the bodies below follow the HD code only.

#include <stdlib.h>
#include <string.h>
#include "trigger.h"
#include "world.h"
#include "stream.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

// HD throws a const char* (__CxxThrowException_8 with the string type info).
static void ThrowInvalidArraySize()
{
    throw "Invalid array size";
}

// HD 0x56e7d0 (lifted in mapload.cpp as ReadWorldString): u16 length, bytes.
static void ReadTrigString(SStream* s, SString* out)
{
    int len = s->ReadWord() & 0xffff;
    FreeSString(out);
    out->size = len;
    out->buf = new char[len + 1];
    s->Read(out->buf, len);
    out->buf[len] = 0;
}

// HD SDArray / SHeap growth uses realloc (0x78b864) and zero-fills.
template <typename T>
static void ResizeZeroed(T** array, int* max, int size)
{
    if (*max < size) {
        *max = size;
        *array = (T*)realloc(*array, size * sizeof(T));
    }
    if (*max > 0)
        memset((void*)*array, 0, *max * sizeof(T));
}

// ---------------------------------------------------------------------------
// Property masks

// HD 0x5b2400 (the inline switch of STriggerEvent::GetPropertyMask)
static bool EventHasParam(int type)
{
    switch (type) {
    case 0: case 1: case 4: case 5: case 6: case 7: case 8:
        return false;
    case 2: case 3: case 9: case 10:
        return true;
    default:
        Logger.g->Panic("STriggerEvent::GetPropertyMask: Invalid type (%d)", type);
    }
}

// PANZERS 0x5b1f30
unsigned STriggerCondition::GetPropertyMask(int type)
{
    switch (type) {
    case 0: case 1: case 6: case 10:
        return 0;
    case 2:
        return 0xd;
    case 3:
        return 0x200;
    case 4: case 8:
        return 1;
    case 5: case 9:
        return 8;
    case 7: case 0xb:
        return 4;
    case 0xc: case 0xe:
        return 0x40000;
    case 0xd:
        return 0x2000;
    default:
        Logger.g->Panic("STriggerCondition::GetPropertyMask: Invalid type (%d)", type);
    }
}

// PANZERS 0x5b1d50
unsigned STriggerAction::GetPropertyMask(int type)
{
    switch (type) {
    case 0: case 10: case 0x16: case 0x1a: case 0x1f: case 0x20: case 0x2a: case 0x30:
    case 0x3c: case 0x3d: case 0x43: case 0x44: case 0x45: case 0x46: case 0x47:
    case 0x48: case 0x49: case 0x4b:
        return 0;
    case 1:
        return 0x31;
    case 2: case 0xe: case 0x15: case 0x17: case 0x19: case 0x26: case 0x31:
        return 0x20;
    case 3: case 4: case 5: case 8: case 9:
        return 0x500;
    case 6: case 7:
        return 0x400;
    case 0xb: case 0xc: case 0x1b:
        return 1;
    case 0xd: case 0xf: case 0x1e: case 0x39:
        return 0x800;
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14:
        return 0x21;
    case 0x18: case 0x38: case 0x3f: case 0x42: case 0x4a:
        return 0x1000;
    case 0x1c:
        return 0x2800;
    case 0x1d: case 0x2e: case 0x40: case 0x41:
        return 0x4000;
    case 0x21: case 0x22: case 0x23: case 0x24: case 0x25: case 0x2f: case 0x37: case 0x3b:
        return 0x100;
    case 0x27:
        return 0x8000;
    case 0x28:
        return 0x10000;
    case 0x29:
        return 0x138131;
    case 0x2b:
        return 0x20000;
    case 0x2c: case 0x32:
        return 0x120;
    case 0x2d:
        return 0x4020;
    case 0x33:
        return 0x82000;
    case 0x34: case 0x35: case 0x36:
        return 0x2020;
    case 0x3a:
        return 0x200100;
    case 0x3e:
        return 0x400100;
    default:
        Logger.g->Panic("STriggerAction::GetPropertyMask: Invalid type (%d)", type);
    }
}

// ---------------------------------------------------------------------------
// Loaders

// PANZERS 0x5b2400
void STriggerEvent::Load(SStream* s)
{
    PZ_TRACE("STriggerEvent::Load (0x5b2400)");
    Type = s->ReadInt();
    if (!EventHasParam(Type)) {
        Param = -1;
        return;
    }
    Param = s->ReadInt();
}

// PANZERS 0x5b22b0
void STriggerCondition::Load(SStream* s)
{
    Type = s->ReadInt();
    unsigned mask = GetPropertyMask(Type);
    Player = (mask & 1) ? s->ReadInt() : -1;
    if (mask & 2) {
        QuantityMore = s->ReadInt() != 0;
        Quantity = s->ReadInt();
    } else {
        QuantityMore = true;
        Quantity = -1;
    }
    Location = (mask & 4) ? s->ReadInt() : -1;
    if (mask & 8) {
        UnitType = s->ReadInt();
        ReadTrigString(s, &UnitClass);
    } else {
        UnitType = -1;
        FreeSString(&UnitClass);
    }
    if (mask & 0x40) {      // the S.W.I.N.E. switch pair: read and dropped
        s->ReadInt();
        s->ReadInt();
    }
    if (mask & 0x200) {
        P200[0] = s->ReadInt();
        P200[1] = s->ReadInt();
        P200[2] = s->ReadInt();
    } else {
        P200[0] = -1;
        P200[1] = 0;
        P200[2] = 0;
    }
    if (mask & 0x40000)
        ReadTrigString(s, &Str40000);
    P2000 = (mask & 0x2000) ? s->ReadInt() : -1;
}

// PANZERS 0x5b2010
void STriggerAction::Load(SStream* s)
{
    Type = s->ReadInt();
    unsigned mask = GetPropertyMask(Type);
    Player = (mask & 1) ? s->ReadInt() : -1;
    Location = (mask & 4) ? s->ReadInt() : -1;
    if (mask & 8) {
        UnitType = s->ReadInt();
        ReadTrigString(s, &UnitClass);
    } else {
        UnitType = -1;
        FreeSString(&UnitClass);
    }
    if (mask & 0x10)
        ReadTrigString(s, &Str10);
    else
        FreeSString(&Str10);
    P20 = (mask & 0x20) ? s->ReadInt() : -1;
    if (mask & 0x80) {      // read and dropped
        s->ReadInt();
        s->ReadInt();
    }
    Num = (mask & 0x100) ? s->ReadInt() : 0;
    P400000 = (mask & 0x400000) ? s->ReadInt() : 0;
    P400 = (mask & 0x400) ? s->ReadInt() : -1;
    P800 = (mask & 0x800) ? s->ReadInt() : -1;
    if (mask & 0x1000)
        ReadTrigString(s, &Str1000);
    else
        FreeSString(&Str1000);
    P2000 = (mask & 0x2000) ? s->ReadInt() : -1;
    if (mask & 0x4000)
        ReadTrigString(s, &Str4000);
    else
        FreeSString(&Str4000);
    P8000 = (mask & 0x8000) ? s->ReadInt() : -1;
    P10000 = (mask & 0x10000) ? s->ReadInt() : -1;
    if (mask & 0x20000) {
        P20000[0] = s->ReadInt();
        P20000[1] = s->ReadInt();
    } else {
        P20000[0] = 0;
        P20000[1] = 0;
    }
    P80000 = (mask & 0x80000) ? s->ReadInt() : -1;
    B100000 = (mask & 0x100000) ? s->ReadInt() != 0 : false;
    P200000 = (mask & 0x200000) ? s->ReadInt() : -1;
}

// PANZERS 0x5d4310 (condition array clear, inline in 0x5dd140)
static void ClearConditions(STriggerArray<STriggerCondition>* a)
{
    for (int i = 0; i < a->Size; ++i) {
        FreeSString(&a->Array[i].UnitClass);
        FreeSString(&a->Array[i].Str40000);
    }
    a->Size = 0;
}

// PANZERS 0x5d43d0 (action array clear, inline in 0x5dd140)
static void ClearActions(STriggerArray<STriggerAction>* a)
{
    for (int i = 0; i < a->Size; ++i) {
        FreeSString(&a->Array[i].UnitClass);
        FreeSString(&a->Array[i].Str10);
        FreeSString(&a->Array[i].Str1000);
        FreeSString(&a->Array[i].Str4000);
    }
    a->Size = 0;
}

// PANZERS 0x5dd140
static void ResizeTriggers(STriggerArray<STrigger>* t, int size)
{
    if (t->Size != 0 && t->Array == nullptr)
        Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "STrigger");
    for (int i = 0; i < t->Size; ++i) {
        STrigger& tr = t->Array[i];
        ClearConditions(&tr.Conditions);
        free(tr.Conditions.Array);
        ClearActions(&tr.Actions);
        free(tr.Actions.Array);
        FreeSString(&tr.Name);
    }
    t->Size = size;
    ResizeZeroed(&t->Array, &t->Max, size);
}

// PANZERS 0x5f0240
static void LoadConditions(STriggerArray<STriggerCondition>* a, SStream* s)
{
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        ThrowInvalidArraySize();
    // HD 0x5dd300: clear + resize (zeroed)
    ClearConditions(a);
    a->Size = (int)n;
    ResizeZeroed(&a->Array, &a->Max, (int)n);
    for (int i = 0; i < a->Size; ++i)
        a->Array[i].Load(s);
}

// PANZERS 0x5f01e0
static void LoadActions(STriggerArray<STriggerAction>* a, SStream* s)
{
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        ThrowInvalidArraySize();
    // HD 0x5dd200: clear + resize (zeroed)
    ClearActions(a);
    a->Size = (int)n;
    ResizeZeroed(&a->Array, &a->Max, (int)n);
    for (int i = 0; i < a->Size; ++i)
        a->Array[i].Load(s);
}

// PANZERS 0x5f0140
void LoadTriggers(STriggerArray<STrigger>* triggers, SStream* s)
{
    PZ_TRACE("SWorld::LoadTriggers (0x5f0140)");
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        ThrowInvalidArraySize();
    ResizeTriggers(triggers, (int)n);
    for (int i = 0; i < triggers->Size; ++i) {
        STrigger& t = triggers->Array[i];
        t.Flags = s->ReadInt();
        ReadTrigString(s, &t.Name);
        t.Event.Load(s);
        LoadConditions(&t.Conditions, s);
        LoadActions(&t.Actions, s);
    }
}

// PANZERS 0x5dd650 (clear; the scene object and effect of a location are
// not created in the menu, so only the name is freed here)
static void ClearLocations(SHeap<SLocation>* h)
{
    for (int i = 0; i < h->Size; ++i) {
        if (h->Array[i].Next != kHeapLive)
            continue;
        // HD: scene +0x70(SceneObject) if >= 0, pixie +0x0c(Effect) if >= 0.
        FreeSString(&h->Array[i].Data.Name);
    }
    h->Size = 0;
    h->Free = -1;
    h->Count = 0;
}

// PANZERS 0x5f0690
void SWorld::LoadLocations(SStream* s)
{
    PZ_TRACE("SWorld::LoadLocations (0x5f0690)");
    SHeap<SLocation>* h = &Locations;
    ClearLocations(h);
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        ThrowInvalidArraySize();
    h->Size = (int)n;
    ResizeZeroed(&h->Array, &h->Max, (int)n);
    h->Free = s->ReadInt();
    h->Count = s->ReadInt();
    for (int i = 0; i < h->Size; ++i) {
        SHeapElem<SLocation>& e = h->Array[i];
        e.Next = s->ReadInt();
        if (e.Next != kHeapLive)
            continue;
        e.Data.X1 = s->ReadInt();
        e.Data.Z1 = s->ReadInt();
        e.Data.X2 = s->ReadInt();
        e.Data.Z2 = s->ReadInt();
        ReadTrigString(s, &e.Data.Name);
        e.Data.Color = s->ReadInt();
        e.Data.SceneObject = -1;
        e.Data.Effect = -1;
    }
}

// PANZERS 0x5dd720
static void ClearPaths(SHeap<SPath>* h)
{
    for (int i = 0; i < h->Size; ++i) {
        if (h->Array[i].Next != kHeapLive)
            continue;
        SPath& p = h->Array[i].Data;
        if (p.Points.Size != 0 && p.Points.Array == nullptr)
            Logger.g->Panic("SDArray<%s>::Clear: array is damaged", "SPathPoint");
        free(p.Points.Array);
        p.Points.Array = nullptr;
        p.Points.Size = 0;
        p.Points.Max = 0;
        FreeSString(&p.Name);
    }
    h->Size = 0;
    h->Free = -1;
    h->Count = 0;
}

// PANZERS 0x5efd30
static void LoadPathPoints(SHdArray<SPathPoint>* a, SStream* s)
{
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        ThrowInvalidArraySize();
    a->Size = (int)n;                                             // 0x5dcbf0
    ResizeZeroed(&a->Array, &a->Max, (int)n);
    for (int i = 0; i < a->Size; ++i) {
        a->Array[i].X = s->ReadFloat();
        a->Array[i].Z = s->ReadFloat();
    }
}

// PANZERS 0x5f07b0
void SWorld::LoadPaths(SStream* s)
{
    PZ_TRACE("SWorld::LoadPaths (0x5f07b0)");
    SHeap<SPath>* h = &Paths;
    ClearPaths(h);
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        ThrowInvalidArraySize();
    h->Size = (int)n;
    ResizeZeroed(&h->Array, &h->Max, (int)n);
    h->Free = s->ReadInt();
    h->Count = s->ReadInt();
    for (int i = 0; i < h->Size; ++i) {
        SHeapElem<SPath>& e = h->Array[i];
        e.Next = s->ReadInt();
        if (e.Next != kHeapLive)
            continue;
        ReadTrigString(s, &e.Data.Name);
        e.Data.P0c = s->ReadInt();
        e.Data.Closed = s->ReadByte() != 0;                       // 0x65d300
        LoadPathPoints(&e.Data.Points, s);
    }
}

// PANZERS 0x5637f0
static void ClearTriggerVariables(SHeap<STriggerVariable>* h)
{
    for (int i = 0; i < h->Size; ++i)
        if (h->Array[i].Next == kHeapLive)
            FreeSString(&h->Array[i].Data.Name);
    h->Size = 0;
    h->Free = -1;
    h->Count = 0;
}

// PANZERS 0x56e440
void SWorld::LoadTriggerVariables(SStream* s)
{
    PZ_TRACE("SWorld::LoadTriggerVariables (0x56e440)");
    SHeap<STriggerVariable>* h = &TriggerVariables;
    ClearTriggerVariables(h);
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        ThrowInvalidArraySize();
    h->Size = (int)n;
    ResizeZeroed(&h->Array, &h->Max, (int)n);
    h->Free = s->ReadInt();
    h->Count = s->ReadInt();
    for (int i = 0; i < h->Size; ++i) {
        SHeapElem<STriggerVariable>& e = h->Array[i];
        e.Next = s->ReadInt();
        if (e.Next != kHeapLive)
            continue;
        // PANZERS 0x5f13b0 (element load)
        ReadTrigString(s, &e.Data.Name);
        e.Data.Value = s->ReadInt();
        e.Data.Step = s->ReadInt();
    }
}

// ---------------------------------------------------------------------------
// Recompile only: the trigger-load dump (PZ_M2_TRIGDUMP=1 or -m2), in the
// format of m2scope/trigdec.py, so it can be diffed against the decoder.

void DumpTriggers(SWorld* w)
{
    if (!Logger.g || !w)
        return;
    for (int i = 0; i < w->TriggerVariables.Size; ++i) {
        const SHeapElem<STriggerVariable>& e = w->TriggerVariables.Array[i];
        if (e.Next == kHeapLive)
            Logger.g->Log(0, "PZM2 TVAR %d '%s' value=%d step=%d", i, SStr(e.Data.Name), e.Data.Value,
                          e.Data.Step);
    }
    for (int i = 0; i < w->Locations.Size; ++i) {
        const SHeapElem<SLocation>& e = w->Locations.Array[i];
        if (e.Next == kHeapLive)
            Logger.g->Log(0, "PZM2 LOCS %d '%s' (%d,%d)-(%d,%d) color=%d", i, SStr(e.Data.Name), e.Data.X1,
                          e.Data.Z1, e.Data.X2, e.Data.Z2, e.Data.Color);
    }
    for (int i = 0; i < w->Paths.Size; ++i) {
        const SHeapElem<SPath>& e = w->Paths.Array[i];
        if (e.Next == kHeapLive)
            Logger.g->Log(0, "PZM2 PATH %d '%s' p0c=%d closed=%d points=%d", i, SStr(e.Data.Name), e.Data.P0c,
                          e.Data.Closed ? 1 : 0, e.Data.Points.Size);
    }
    STriggerArray<STrigger>* t = reinterpret_cast<STriggerArray<STrigger>*>(&w->Triggers);
    for (int i = 0; i < t->Size; ++i) {
        const STrigger& tr = t->Array[i];
        Logger.g->Log(0, "PZM2 T%-2d flag=%d '%s' event=(%d,%d) conds=%d actions=%d", i, tr.Flags,
                      SStr(tr.Name), tr.Event.Type, tr.Event.Param, tr.Conditions.Size, tr.Actions.Size);
        for (int c = 0; c < tr.Conditions.Size; ++c) {
            const STriggerCondition& k = tr.Conditions.Array[c];
            Logger.g->Log(0, "PZM2     C type=%d player=%d qmore=%d qty=%d loc=%d unittype=%d unitclass='%s' "
                             "p200=(%d,%d,%d) p2000=%d",
                          k.Type, k.Player, k.QuantityMore ? 1 : 0, k.Quantity, k.Location, k.UnitType,
                          SStr(k.UnitClass), k.P200[0], k.P200[1], k.P200[2], k.P2000);
        }
        for (int a = 0; a < tr.Actions.Size; ++a) {
            const STriggerAction& k = tr.Actions.Array[a];
            Logger.g->Log(0, "PZM2     A type=0x%x player=%d loc=%d str10='%s' p20=%d num=%d p400=%d p800=%d "
                             "p8000=%d p10000=%d p20000=(%d,%d) b100000=%d",
                          k.Type, k.Player, k.Location, SStr(k.Str10), k.P20, k.Num, k.P400, k.P800, k.P8000,
                          k.P10000, k.P20000[0], k.P20000[1], k.B100000 ? 1 : 0);
        }
    }
}

} // namespace pz
