// src/game/world_save.cpp
// The SWorld parts of the save game (SGameLogic::SaveGameState 0x57e110 and
// LoadGameState 0x56eb50): players, AI groups, units, effects, camera,
// locations, triggers, trigger variables, doodad animations, wires and the
// weather. HD reads and writes the world at fixed offsets; the recompile's
// SWorld keeps them (world.h), so the writers below follow HD's offsets.
// OWNER: agent S (M4, docs/M4_STATUS.md).

#include <string.h>
#include <stdlib.h>
#include "world.h"
#include "world_save.h"
#include "trigger.h"
#include "pzunitregistry.h"
#include "unitsave.h"
#include "unit.h"
#include "stream.h"
#include "logger.h"
#include "doodad.h"
#include "aigroup.h"

namespace pz {

const unsigned char* GetMapRawChunk(int tag, int* size);   // mapload.cpp

static inline int RI(const void* b, int off) { return *(const int*)((const unsigned char*)b + off); }
static inline float RF(const void* b, int off) { return *(const float*)((const unsigned char*)b + off); }
static inline unsigned char RB(const void* b, int off) { return *((const unsigned char*)b + off); }

// HD's inline SString save (u16 size, then the bytes; 0 for a null string).
void SaveWorldString(SStream* s, const void* str)
{
    const SString* p = (const SString*)str;
    if (!p->buf) {
        s->WriteWord(0);
        return;
    }
    if (p->size > 0xfffe)
        throw "String is too long";
    s->WriteWord((unsigned short)p->size);
    s->Write(p->buf, p->size);
}

// PANZERS 0x5fb160 (PLY3): per player (0x48 at World+0x170) the dwords
// 0..3 and 11..15, the fields SWorld::LoadPlayers 0x5f2ed0 reads back.
void SaveWorldPlayers(SWorld* w, SStream* s)
{
    const unsigned char* b = (const unsigned char*)w + 0x170;
    for (int i = 0; i < 12; ++i, b += 0x48) {
        s->WriteInt(RI(b, 0x00));
        s->WriteInt(RI(b, 0x04));
        s->WriteInt(RI(b, 0x08));
        s->WriteInt(RI(b, 0x0c));
        s->WriteInt(RI(b, 0x2c));
        s->WriteInt(RI(b, 0x30));
        s->WriteInt(RI(b, 0x34));
        s->WriteInt(RI(b, 0x38));
        s->WriteInt(RI(b, 0x3c));
    }
}

// PANZERS 0x57db20 (AIGP, World+0x4f4 SHeap, element 0x54): size, free,
// count, per slot Next and, when live, the group's variables (0x5f9c60:
// gSaveVariables with 0x8dde98).
void SaveWorldAIGroups(SWorld* w, SStream* s)
{
    const unsigned char* h = (const unsigned char*)w + 0x4f4;
    const unsigned char* a = *(unsigned char* const*)h;
    int n = RI(h, 4);
    s->WriteInt(n);
    s->WriteInt(RI(h, 0xc));
    s->WriteInt(RI(h, 0x10));
    for (int i = 0; i < n; ++i) {
        const unsigned char* e = a + i * 0x54;
        s->WriteInt(RI(e, 0));
        if (RI(e, 0) == kHeapLive)
            SaveVariables(s, e + 4, kAIGroupSaveDesc);            // 0x5f9c60
    }
}

// PANZERS 0x5fb630 (UNIS): heap size, per slot 0 / 1 and a live unit's
// 'UNIT' chunk {'v100', class name (prototype +0x60), SUnit::Save 0x5be320}.
void SaveWorldUnits(SWorld* w, SStream* s)
{
    int n = w->Units.Size;
    s->WriteInt(n);
    for (int i = 0; i < n; ++i) {
        if (!w->Units.IsLive(i)) {
            s->WriteInt(0);
            continue;
        }
        s->WriteInt(1);
        s->WriteChunkStart(0x54494e55);                           // 'UNIT'
        s->WriteInt(0x30303176);                                  // 'v100'
        SUnit* u = w->Units.Array[i].Unit;
        SaveWorldString(s, (const unsigned char*)u->Proto + 0x60);
        u->Save(s);                                               // 0x5be320
        s->WriteChunkEnd();
    }
}

// PANZERS 0x5f9450 (EEFS, World+0x73d0 SHeap, element 0x2c): per live
// effect an 'EFFE' chunk {'v100', name, 5 floats}.
void SaveWorldEffects(SWorld* w, SStream* s)
{
    const unsigned char* h = (const unsigned char*)w + 0x73d0;
    const unsigned char* a = *(unsigned char* const*)h;
    int n = RI(h, 4);
    s->WriteInt(n);
    s->WriteInt(RI(h, 0xc));
    s->WriteInt(RI(h, 0x10));
    for (int i = 0; i < n; ++i) {
        const unsigned char* e = a + i * 0x2c;
        s->WriteInt(RI(e, 0));
        if (RI(e, 0) != kHeapLive)
            continue;
        s->WriteChunkStart(0x45464645);                           // 'EFFE'
        s->WriteInt(0x30303176);
        SaveWorldString(s, e + 4);
        for (int k = 0; k < 5; ++k)
            s->WriteFloat(RF(e, 0xc + 4 * k));
        s->WriteChunkEnd();
    }
}

// PANZERS 0x5e6a70 ("CAM ": World +0x38, +0x40, +0x44, +0x50, +0x54 as 20 raw bytes)
void SaveWorldCamera(SWorld* w, SStream* s)
{
    int v[5] = { RI(w, 0x38), RI(w, 0x40), RI(w, 0x44), RI(w, 0x50), RI(w, 0x54) };
    s->Write(v, 0x14);
}

// PANZERS 0x5f9210 (LOCS, World+0x7480 SHeap, element 0x28)
void SaveWorldLocations(SWorld* w, SStream* s)
{
    const unsigned char* h = (const unsigned char*)w + 0x7480;
    const unsigned char* a = *(unsigned char* const*)h;
    int n = RI(h, 4);
    s->WriteInt(n);
    s->WriteInt(RI(h, 0xc));
    s->WriteInt(RI(h, 0x10));
    for (int i = 0; i < n; ++i) {
        const unsigned char* e = a + i * 0x28;
        s->WriteInt(RI(e, 0));
        if (RI(e, 0) != kHeapLive)
            continue;
        s->WriteInt(RI(e, 0x04));
        s->WriteInt(RI(e, 0x08));
        s->WriteInt(RI(e, 0x0c));
        s->WriteInt(RI(e, 0x10));
        SaveWorldString(s, e + 0x14);
        s->WriteInt(RI(e, 0x24));
    }
}

// PANZERS 0x5b2750 (STriggerEvent::Save)
static void SaveTriggerEvent(const STriggerEvent* ev, SStream* s)
{
    s->WriteInt(ev->Type);
    bool param;
    switch (ev->Type) {
    case 0: case 1: case 4: case 5: case 6: case 7: case 8: param = false; break;
    case 2: case 3: case 9: case 10: param = true; break;
    default:
        Logger.g->Panic("STriggerEvent::GetPropertyMask: Invalid type (%d)", ev->Type);
        return;
    }
    if (param)
        s->WriteInt(ev->Param);
}

// PANZERS 0x5b2670 (STriggerCondition::Save)
static void SaveTriggerCondition(const STriggerCondition* c, SStream* s)
{
    s->WriteInt(c->Type);
    unsigned m = STriggerCondition::GetPropertyMask(c->Type);
    if (m & 1)
        s->WriteInt(c->Player);
    if (m & 2) {
        s->WriteInt((unsigned char)c->QuantityMore);
        s->WriteInt(c->Quantity);
    }
    if (m & 4)
        s->WriteInt(c->Location);
    if (m & 8) {
        s->WriteInt(c->UnitType);
        SaveWorldString(s, &c->UnitClass);
    }
    if (m & 0x40)
        Logger.g->Panic("STriggerCondition::Save: Invalid flag");
    if (m & 0x200) {
        s->WriteInt(c->P200[0]);
        s->WriteInt(c->P200[1]);
        s->WriteInt(c->P200[2]);
    }
    if (m & 0x40000)
        SaveWorldString(s, &c->Str40000);
    if (m & 0x2000)
        s->WriteInt(c->P2000);
}

// PANZERS 0x5b24e0 (STriggerAction::Save)
static void SaveTriggerAction(const STriggerAction* a, SStream* s)
{
    s->WriteInt(a->Type);
    unsigned m = STriggerAction::GetPropertyMask(a->Type);
    if (m & 1)
        s->WriteInt(a->Player);
    if (m & 4)
        s->WriteInt(a->Location);
    if (m & 8) {
        s->WriteInt(a->UnitType);
        SaveWorldString(s, &a->UnitClass);
    }
    if (m & 0x10)
        SaveWorldString(s, &a->Str10);
    if (m & 0x20)
        s->WriteInt(a->P20);
    if (m & 0x80)
        Logger.g->Panic("STriggerAction::Save: Invalid flag");
    if (m & 0x100)
        s->WriteInt(a->Num);
    if (m & 0x400000)
        s->WriteInt(a->P400000);
    if (m & 0x400)
        s->WriteInt(a->P400);
    if (m & 0x800)
        s->WriteInt(a->P800);
    if (m & 0x1000)
        SaveWorldString(s, &a->Str1000);
    if (m & 0x2000)
        s->WriteInt(a->P2000);
    if (m & 0x4000)
        SaveWorldString(s, &a->Str4000);
    if (m & 0x8000)
        s->WriteInt(a->P8000);
    if (m & 0x10000)
        s->WriteInt(a->P10000);
    if (m & 0x20000) {
        s->WriteInt(a->P20000[0]);
        s->WriteInt(a->P20000[1]);
    }
    if (m & 0x80000)
        s->WriteInt(a->P80000);
    if (m & 0x100000)
        s->WriteInt((unsigned char)a->B100000);
    if (m & 0x200000)
        s->WriteInt(a->P200000);
}

// PANZERS 0x5f8a80 (TRIG, World+0x7474 SDArray<STrigger>)
void SaveWorldTriggers(SWorld* w, SStream* s)
{
    const STriggerArray<STrigger>* t = reinterpret_cast<const STriggerArray<STrigger>*>(&w->Triggers);
    s->WriteInt(t->Size);
    for (int i = 0; i < t->Size; ++i) {
        const STrigger* tr = &t->Array[i];
        s->WriteInt(tr->Flags);
        SaveWorldString(s, &tr->Name);
        SaveTriggerEvent(&tr->Event, s);
        s->WriteInt(tr->Conditions.Size);
        for (int k = 0; k < tr->Conditions.Size; ++k)
            SaveTriggerCondition(&tr->Conditions.Array[k], s);
        s->WriteInt(tr->Actions.Size);
        for (int k = 0; k < tr->Actions.Size; ++k)
            SaveTriggerAction(&tr->Actions.Array[k], s);
    }
}

// PANZERS 0x57dd60 (TVAR, World+0x74a8 SHeap, element 0x14; 0x5fa2b0 per live one)
void SaveWorldTriggerVariables(SWorld* w, SStream* s)
{
    const unsigned char* h = (const unsigned char*)w + 0x74a8;
    const unsigned char* a = *(unsigned char* const*)h;
    int n = RI(h, 4);
    s->WriteInt(n);
    s->WriteInt(RI(h, 0xc));
    s->WriteInt(RI(h, 0x10));
    for (int i = 0; i < n; ++i) {
        const unsigned char* e = a + i * 0x14;
        s->WriteInt(RI(e, 0));
        if (RI(e, 0) != kHeapLive)
            continue;
        SaveWorldString(s, e + 4);                                // 0x5fa2b0
        s->WriteInt(RI(e, 0x0c));
        s->WriteInt(RI(e, 0x10));
    }
}

// PANZERS 0x5f8cf0 (ODDD, World+0x158 SHeap of doodad animations, element 0x20)
void SaveWorldDoodadAnims(SWorld* w, SStream* s)
{
    const unsigned char* h = (const unsigned char*)w + 0x158;
    const unsigned char* a = *(unsigned char* const*)h;
    int n = RI(h, 4);
    s->WriteInt(n);
    s->WriteInt(RI(h, 0xc));
    s->WriteInt(RI(h, 0x10));
    for (int i = 0; i < n; ++i) {
        const unsigned char* e = a + i * 0x20;
        s->WriteInt(RI(e, 0));
        if (RI(e, 0) != kHeapLive)
            continue;
        s->WriteChunkStart(0x44444f44);                           // 'DODD'
        s->WriteInt(RI(e, 0x04));
        s->WriteInt(RI(e, 0x08));
        s->WriteFloat(RF(e, 0x0c));
        s->WriteFloat(RF(e, 0x10));
        s->WriteFloat(RF(e, 0x14));
        s->WriteFloat(RF(e, 0x18));
        s->WriteChunkEnd();
    }
}

// PANZERS 0x5fb7d0 (WIR3: 0x5f9b40 World+0x742c, 0x5f9a80 +0x7440, 0x5f8df0
// +0x7454). The recompile does not create the wires (mapload.cpp keeps the
// map's WIR3 chunk raw), so it writes the map's chunk back: the map has the
// first two heaps (the editor saves with 0x5fb7d0(s, 0)); the third (+0x7454,
// written with the flag 1) is written empty, as the original's mission-start
// save of Training Camp has it.
void SaveWorldWires(SWorld* w, SStream* s)
{
    (void)w;
    int size = 0;
    const unsigned char* raw = GetMapRawChunk(0x33524957, &size);
    if (raw && size > 0) {
        s->Write(raw, size);
    } else {
        for (int i = 0; i < 2; ++i) {                             // 0x5f9b40, 0x5f9a80: empty heaps
            s->WriteInt(0);
            s->WriteInt(-1);
            s->WriteInt(0);
        }
    }
    s->WriteInt(0);                                               // 0x5f8df0: size, free, count
    s->WriteInt(-1);
    s->WriteInt(0);
}

// PANZERS 0x5fa830 (one weather light set: 19 floats in HD's order)
static void SaveWeatherLights(const unsigned char* p, SStream* s)
{
    static const int kOrder[19] = { 0, 1, 2, 3, 4, 5, 6, 7, 0xc, 0xd, 8, 9, 10, 0xb, 0xe, 0xf, 0x10, 0x11, 0x12 };
    for (int i = 0; i < 19; ++i)
        s->WriteFloat(RF(p, kOrder[i] * 4));
}

// PANZERS 0x5fb770 (WTHR): the current and the target light sets (World
// +0x550, +0x59c), the blend +0x548, the blend time +0x54c, the weather +0x544.
void SaveWorldWeather(SWorld* w, SStream* s)
{
    SaveWeatherLights((const unsigned char*)w + 0x550, s);
    SaveWeatherLights((const unsigned char*)w + 0x59c, s);
    s->WriteFloat(RF(w, 0x548));
    s->WriteFloat(RF(w, 0x54c));
    s->WriteInt(RI(w, 0x544));
}

// ---------------------------------------------------------------------------
// Load

static void ReadWorldStr(SStream* s, SString* out)
{
    // 0x65d6f0: u16 size, the bytes
    FreeSString(out);
    int n = s->ReadWord() & 0xffff;
    if (n == 0)
        return;
    out->size = n;
    out->buf = new char[n + 1];
    s->Read(out->buf, n);
    out->buf[n] = 0;
}

// PANZERS 0x5f3820 (UNIS)
// Every slot is allocated again in order (0x5d94b0 must hand out the slot
// index), a live one gets a unit of its class (prototype +0x10 with the
// index) that SUnit::Load 0x5bbd30 fills; the slots of the dead units and of
// classes that are gone are freed afterwards. Then every unit runs its two
// after-load slots (+0x14 for all, then +0x18 for all).
void LoadWorldUnits(SWorld* w, SStream* s)
{
    int n = s->ReadInt();
    for (int i = 0; i < n; ++i) {
        int idx = w->AllocUnitSlot();                             // 0x5d94b0
        if (idx != i)
            throw "Unit indexing error";
        if (s->ReadInt() == 0) {
            w->Units.Array[idx].Unit = nullptr;
            continue;
        }
        if (s->ReadChunkHeader() != 0x54494e55)                   // 'UNIT'
            throw "Unsupported unit type";
        if (s->ReadInt() != 0x30303176)                           // 'v100'
            throw "Unsupported unit version";
        SString cls;
        ReadWorldStr(s, &cls);                                    // 0x5910e0
        SPUnit* proto = g_UnitRegistry ? g_UnitRegistry->GetPUnit(SStr(cls), true) : nullptr;   // 0x5d0e70(name, 1)
        if (!proto) {
            // HD retries with the class name remapped by the map's UNDS table
            // (0x5f4a70 / 0x5625a0); the recompile skips the unit.
            Logger.g->Warning("SWorld::LoadUnits: unknown unit class %s", SStr(cls));
            s->ReadChunkSkip();
            s->ReadChunkValidate(0);
            w->Units.Array[idx].Unit = nullptr;
        } else {
            SUnit* u = static_cast<SUnit*>(proto->CreateUnit(idx)); // prototype +0x10
            w->Units.Array[idx].Unit = u;
            u->Load(s);                                           // 0x5bbd30
            s->ReadChunkValidate(0);
        }
        FreeSString(&cls);
    }
    // Free the empty slots (SHeapTRB::Remove inline: tail of the free list,
    // the slot keeps the barrier frame +0x4ec).
    unsigned char* h = (unsigned char*)&w->Units;
    for (int i = 0; i < n; ++i) {
        if (w->Units.Array[i].Unit != nullptr)
            continue;
        w->Units.Array[i].Next = -1;
        *(int*)&w->Units.Array[i].Unit = *(int*)(h + 0x18);
        if (w->Units.Free < 0)
            w->Units.Free = i;
        else
            w->Units.Array[w->Units.FreeTail].Next = i;
        w->Units.Count--;
        w->Units.FreeTail = i;
    }
    for (int i = 0; i < w->Units.Size; ++i)
        if (w->Units.IsLive(i))
            w->Units.Array[i].Unit->Slot_14();                    // vtbl +0x14 (0x5bb1c0: init after load)
    for (int i = 0; i < w->Units.Size; ++i)
        if (w->Units.IsLive(i))
            w->Units.Array[i].Unit->Slot_18();                    // vtbl +0x18 (0x5baf30: links after load)
}

// PANZERS 0x5f1fd0 (ODDD). HD stops the running doodad animations first and
// starts the loaded ones on their doodads; the recompile restores the heap
// (the Training Camp saves have none) and logs when there are any.
void LoadWorldDoodadAnims(SWorld* w, SStream* s)
{
    unsigned char* h = (unsigned char*)w + 0x158;
    unsigned n = (unsigned)s->ReadInt();
    if (n > 0x1000000)
        throw "Invalid array size";
    int freeHead = s->ReadInt();
    int count = s->ReadInt();
    unsigned char*& a = *(unsigned char**)h;
    int& size = *(int*)(h + 4);
    int& max = *(int*)(h + 8);
    if (max < (int)n) {
        a = (unsigned char*)realloc(a, n * 0x20);
        max = (int)n;
    }
    if (a && max > 0)
        memset(a, 0, (size_t)max * 0x20);
    size = (int)n;
    *(int*)(h + 0xc) = freeHead;
    *(int*)(h + 0x10) = count;
    for (int i = 0; i < (int)n; ++i) {
        unsigned char* e = a + i * 0x20;
        *(int*)e = s->ReadInt();
        if (*(int*)e != kHeapLive)
            continue;
        if (s->ReadChunkHeader() != 0x44444f44)                   // 'DODD'
            throw "Expected DODD chunk";
        *(int*)(e + 0x04) = s->ReadInt();
        *(int*)(e + 0x08) = s->ReadInt();
        *(float*)(e + 0x0c) = s->ReadFloat();
        *(float*)(e + 0x10) = s->ReadFloat();
        *(float*)(e + 0x14) = s->ReadFloat();
        *(float*)(e + 0x18) = s->ReadFloat();
        s->ReadChunkValidate(0);
    }
    if (count > 0)
        Logger.g->Log(1, "STUB: SWorld ODDD load (0x5f1fd0): %d doodad animations restored without their doodad state", count);
}

// PANZERS 0x5f18c0 (one weather light set, the order of 0x5fa830)
static void LoadWeatherLights(unsigned char* p, SStream* s)
{
    static const int kOrder[19] = { 0, 1, 2, 3, 4, 5, 6, 7, 0xc, 0xd, 8, 9, 10, 0xb, 0xe, 0xf, 0x10, 0x11, 0x12 };
    for (int i = 0; i < 19; ++i)
        *(float*)(p + kOrder[i] * 4) = s->ReadFloat();
}

// PANZERS 0x5f3cb0 (WTHR). HD then releases the weather's sound and effect
// handles (+0x638, +0x640, +0x648), which the weather refresh makes again.
void LoadWorldWeather(SWorld* w, SStream* s)
{
    LoadWeatherLights((unsigned char*)w + 0x550, s);
    LoadWeatherLights((unsigned char*)w + 0x59c, s);
    *(float*)((unsigned char*)w + 0x548) = s->ReadFloat();
    *(float*)((unsigned char*)w + 0x54c) = s->ReadFloat();
    *(int*)((unsigned char*)w + 0x544) = s->ReadInt();
}

} // namespace pz
