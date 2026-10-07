// src/game/unitsave.cpp
// SUnit save / load (unit.h block "M3"): the per-unit part of the save game
// that every mission start writes (SPanzersCampaign::SaveGameStartMission
// 0x596e30 -> SaveGame 0x5966a0 -> SGameLogic 0x57e110 -> SUnit::Save), and
// the generic variable writer gSaveVariables (unitsave.h). OWNER: agent F
// (docs/M3_INTERFACES.md); agent C owns the rest of SUnit.

#include <string.h>
#include <stdlib.h>
#include "unit.h"
#include "unitsave.h"
#include "m3common.h"
#include "stream.h"
#include "logger.h"
#include "stub_log.h"
#include "doodad.h"
#include "gunner.h"
#include "idriver.h"
#include "target.h"
#include <math.h>
#include "aigroup.h"
#include "unitanim.h"
#include "pz/imodel.h"
#include "world.h"
#include "worldapi.h"

namespace pz {

// PANZERS 0x670af0
void SaveSingleVariable(SStream* s, const void* src, int type, const SVarDesc* members)
{
    const unsigned char* p = (const unsigned char*)src;
    switch (type) {
    case 1: s->WriteByte(*p != 0); return;                        // 0x65daf0
    case 2: s->WriteInt(*(const int*)p); return;                  // 0x65dc40
    case 3: s->WriteFloat(*(const float*)p); return;              // 0x65dc20
    case 4:
        s->WriteFloat(((const float*)p)[0]);
        s->WriteFloat(((const float*)p)[1]);
        s->WriteFloat(((const float*)p)[2]);
        return;
    case 5: s->WriteString(SStr(*(const SString*)p)); return;     // 0x5b2480
    case 6:
        s->WriteChunkStart(0x6c636d5f);                           // "_mcl"
        SaveVariables(s, p, members);
        s->WriteChunkEnd();
        return;
    case 8:
        s->WriteFloat(((const float*)p)[0]);
        s->WriteFloat(((const float*)p)[1]);
        return;
    case 12: s->WriteWord(*(const unsigned short*)p); return;     // 0x65dd00
    default:
        Logger.g->Panic("gSaveSingleVariable: Not a Single type");
    }
}

// PANZERS 0x670c50
void SaveVariables(SStream* s, const void* obj, const SVarDesc* desc)
{
    if (!obj)
        Logger.g->Panic("::gSaveVariables: NULL parameter");
    for (const SVarDesc* d = desc;; ++d) {
        s->WriteInt(d->Type);
        if (d->Type == 0)
            return;
        s->WriteString(d->Name ? d->Name : "");                   // 0x65dca0
        const int* a = (const int*)((const unsigned char*)obj + d->Offset);   // SDArray {array, size, max, ...}
        switch (d->Type) {
        case 1: case 2: case 3: case 4: case 5: case 6: case 8: case 12:
            SaveSingleVariable(s, a, d->Type, d->Members);
            break;
        case 10:
            s->WriteInt(d->ElemType);
            s->WriteInt(a[1]);
            for (int i = 0; i < a[1]; ++i)
                SaveSingleVariable(s, (const unsigned char*)(size_t)a[0] + d->ElemSize * i, d->ElemType, d->Members);
            break;
        case 11: {
            // SDEQueue {array, size, max, base, last (+0x10), first (+0x14)}
            s->WriteInt(d->ElemType);
            s->WriteInt(a[1]);
            s->WriteInt(a[5]);
            s->WriteInt(a[4]);
            if (a[1] != 0 && a[1] != a[4] - a[5] + 1)
                Logger.g->Panic("gSaveVariables: DEQueue is damaged");
            for (int i = a[5]; i <= a[4]; ++i) {
                int k = i - a[3];
                k = k < a[2] ? k : k - a[2];
                SaveSingleVariable(s, (const unsigned char*)(size_t)a[0] + d->ElemSize * k, d->ElemType, d->Members);
            }
            break;
        }
        default:
            break;                                                // HD writes only the type and the name
        }
    }
}

// The target slots SUnit::Save / Load resolve (unit +0x1f8 / +0x1f4, gunner
// +0x14 / +0x18, driver +0xc0 / +0xc4 / +0xc8; HD layouts).
static STarget** GunnerTargetSlot(SGunner* g, int k)
{
    return (STarget**)((unsigned char*)g + (k == 0 ? 0x14 : 0x18));
}

static STarget** DriverTargetSlot(SIDriver* d, int k)
{
    return (STarget**)((unsigned char*)d + 0xc0 + 4 * k);
}

static void AddUniqueTarget(STarget** list, int* n, STarget* t)
{
    if (!t)
        return;
    for (int i = 0; i < *n; ++i)
        if (list[i] == t)
            return;
    list[(*n)++] = t;
}

static int TargetIndex(STarget* const* list, int n, STarget* t)
{
    if (!t)
        return -1;
    for (int i = 0; i < n; ++i)
        if (list[i] == t)
            return i;
    Logger.g->Panic("SUnit::Save: internal error");
    return -1;
}

// The descriptor list of a unit / driver class (vtbl +0x1c / +0x50).
static const SVarDesc* UnitSaveDesc(SUnit* u)
{
    void* obj = nullptr;
    const SUnitClassDesc* rec = nullptr;
    u->GetClassDescriptor(&obj, &rec);
    const SVarDesc* d = rec ? SaveDescForClassRecord(rec->HdAddr) : nullptr;
    return d ? d : kSUnitDesc;
}

static const SVarDesc* DriverSaveDesc(SIDriver* dr)
{
    void* obj = nullptr;
    const SUnitClassDesc* rec = nullptr;
    dr->GetClassDescriptor(&obj, &rec);
    const SVarDesc* d = rec ? SaveDescForClassRecord(rec->HdAddr) : nullptr;
    return d ? d : kSDriverDesc;
}

static const char* UnitClassName(SUnit* u)
{
    return u->Proto ? SStr(*(SString*)((unsigned char*)u->Proto + 0x60)) : "";
}

// PANZERS 0x5be320
// 'vars' {the class descriptor list}, 'gunn' {n, each gunner's list},
// 'driv' {n, each driver's list}, then 'targ' when any target is set: the
// distinct STargets (descriptor 0x8dd890), the gunner and driver counts and
// the target index (-1 = none) of every slot.
void SUnit::Save(struct SStream* s)
{
    s->WriteChunkStart(0x73726176);                               // 'vars'
    SaveVariables(s, this, UnitSaveDesc(this));
    s->WriteChunkEnd();
    s->WriteChunkStart(0x6e6e7567);                               // 'gunn'
    s->WriteInt(Gunners.Size);
    for (int i = 0; i < Gunners.Size; ++i)
        SaveVariables(s, Gunners.Array[i], kSGunnerDesc);         // gunner +0x30 -> 0x8dbaf8
    s->WriteChunkEnd();
    s->WriteChunkStart(0x76697264);                               // 'driv'
    s->WriteInt(Drivers.Size);
    for (int i = 0; i < Drivers.Size; ++i)
        SaveVariables(s, Drivers.Array[i], DriverSaveDesc(Drivers.Array[i]));
    s->WriteChunkEnd();

    int cap = 2 + Gunners.Size * 2 + Drivers.Size * 3;
    STarget** list = (STarget**)malloc(sizeof(STarget*) * cap);
    int n = 0;
    AddUniqueTarget(list, &n, PrimaryTarget);                     // +0x1f8
    AddUniqueTarget(list, &n, CurrentTarget);                     // +0x1f4
    for (int i = 0; i < Gunners.Size; ++i)
        for (int k = 0; k < 2; ++k)
            AddUniqueTarget(list, &n, *GunnerTargetSlot(Gunners.Array[i], k));
    for (int i = 0; i < Drivers.Size; ++i)
        for (int k = 0; k < 3; ++k)
            AddUniqueTarget(list, &n, *DriverTargetSlot(Drivers.Array[i], k));
    if (n != 0) {
        s->WriteChunkStart(0x67726174);                           // 'targ'
        s->WriteInt(n);
        for (int i = 0; i < n; ++i)
            SaveVariables(s, list[i], kSTargetDesc);
        s->WriteInt(Gunners.Size);
        s->WriteInt(Drivers.Size);
        s->WriteInt(TargetIndex(list, n, PrimaryTarget));
        s->WriteInt(TargetIndex(list, n, CurrentTarget));
        for (int i = 0; i < Gunners.Size; ++i)
            for (int k = 0; k < 2; ++k)
                s->WriteInt(TargetIndex(list, n, *GunnerTargetSlot(Gunners.Array[i], k)));
        for (int i = 0; i < Drivers.Size; ++i)
            for (int k = 0; k < 3; ++k)
                s->WriteInt(TargetIndex(list, n, *DriverTargetSlot(Drivers.Array[i], k)));
        s->WriteChunkEnd();
    }
    free(list);
}

// PANZERS 0x5bb1c0 (SIUnit +0x14: init after load; the UNIS loader 0x5f3820
// calls it for every unit once all are loaded)
// Clamps the hit points and armours to 1, folds the direction into
// (-pi, pi], drops the main gunner of a unit without gunners (not type 9),
// clamps the first gunner's ammo, then what Init 0x5ba8e0 does after the
// fields: the drivers' Init, the model, the block map, the effects.
void SUnit::Slot_14()
{
    PZ_M3_TRACE("SUnit::InitAfterLoad (0x5bb1c0)");
    const float one = 1.0f;                                       // DAT_007f1b58
    if (one < HP) HP = 1.0f;                                      // +0x114
    for (int i = 0; i < 4; ++i)
        if (one < Armor[i]) Armor[i] = 1.0f;                      // +0x11c..+0x128
    if (one < _12c) _12c = 1.0f;                                  // +0x12c
    float d = (float)fmod(Dir, 6.283185307179586);                // 0x793cba
    if (d > 3.1415927f)                                           // DAT_007f4584
        d -= 6.2831855f;                                          // DAT_007f458c
    else if (d <= -3.1415927f)                                    // DAT_007f7fb8
        d += 6.2831855f;
    Dir = d;                                                      // +0xb0
    if (Gunners.Size == 0 && Proto->ClassType != 9)
        MainGunner = -1;                                          // +0x44
    if (Gunners.Size > 0 && one < Gunners.Array[0]->AmmoLeft)
        Gunners.Array[0]->AmmoLeft = 1.0f;                        // gunner +0x20
    for (int i = 0; i < Proto->DriverCount; ++i) {
        if (i >= Drivers.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SIDriver *", i);
        Drivers.Array[i]->Init();                                 // +0x10
    }
    InitModel();                                                  // 0x5b7d70
    if (!Unplaced && Proto->ClassType != 4)
        SetOnBlockMap(true);                                      // +0x198
    InitEffects();                                                // 0x5c2c30
}

// PANZERS 0x5baf30 (SIUnit +0x18: links after load, once every unit has its model)
// The model and the interpolation state, the AI group's unit list, then the
// stored units that ride in the model ("vehicle0") and the attached crew.
void SUnit::Slot_18()
{
    PZ_M3_TRACE("SUnit::LinkAfterLoad (0x5baf30)");
    RefreshModel();                                               // +0x3c
    // Recompile-only PZ_M4_FIXFIRST=1 (round-trip test, loadgame.cpp) also
    // keeps the loaded LastPos / PreLastPos: HD shifts them here, which
    // loses the last move of every unit across a load.
    static const bool s_Keep = getenv("PZ_M4_FIXFIRST") != nullptr;
    if (!s_Keep)
        StoreInterpolationState();                                // +0x16c
    if (AIGroup >= 0)
        AIGroupAt(AIGroup)->AddUnit(WorldIndex);                  // 0x55ccc0, 0x5d9560(+0x74)
    for (int i = 0; i < Stored.Size; ++i) {
        SUnit* su = WorldUnit(Stored.Array[i].Unit);
        if (su->Proto->ClassType != 5 && su->Model && Model)
            su->Model->AttachTo(Model, Model->FindNode("vehicle0"));   // model +0xdc(model, +0x40("vehicle0"))
    }
    for (int i = 0; i < Members.Size; ++i) {
        const SUnitMember& m = Members.Array[i];
        if (!m.Attached)
            continue;
        SUnit* mu = WorldUnit(m.Unit);
        if (mu->Model && Model)
            mu->Model->AttachTo(Model, m.Node);                   // model +0xdc(model, +0x10)
        if (mu->Anim)
            static_cast<SUnitAnimation*>(mu->Anim)->PlayGlobalStand(false);   // 0x5cae80(0)
    }
}

// HD 0x5bbd30's slot assignment: release the old target (delete at 0),
// AddRef the new one.
static void AssignTarget(STarget** slot, STarget* t)
{
    if (*slot) {
        if (--(*slot)->RefCount == 0)
            ::operator delete(*slot);
    }
    if (t)
        ++t->RefCount;
    *slot = t;
}

// PANZERS 0x5bbd30
void SUnit::Load(struct SStream* s)
{
    while (!s->ReadChunkIsEnd()) {
        int tag = s->ReadChunkHeader();
        if (tag == 0x73726176) {                                  // 'vars'
            LoadVariables(s, this, UnitSaveDesc(this));
        } else if (tag == 0x67726174) {                           // 'targ'
            int n = s->ReadInt();
            STarget** list = (STarget**)malloc(sizeof(STarget*) * (n > 0 ? n : 1));
            for (int i = 0; i < n; ++i) {
                list[i] = (STarget*)::operator new(0x38);
                memset(list[i], 0, 0x38);
                LoadVariables(s, list[i], kSTargetDesc);
            }
            int ng = s->ReadInt();
            int nd = s->ReadInt();
            if (ng == Gunners.Size && nd == Drivers.Size) {
                int k = s->ReadInt();
                if (k >= 0)
                    AssignTarget(&PrimaryTarget, list[k]);
                k = s->ReadInt();
                if (k >= 0)
                    AssignTarget(&CurrentTarget, list[k]);
                for (int i = 0; i < Gunners.Size; ++i)
                    for (int j = 0; j < 2; ++j) {
                        k = s->ReadInt();
                        if (k >= 0)
                            AssignTarget(GunnerTargetSlot(Gunners.Array[i], j), list[k]);
                    }
                for (int i = 0; i < Drivers.Size; ++i)
                    for (int j = 0; j < 3; ++j) {
                        k = s->ReadInt();
                        if (k >= 0)
                            AssignTarget(DriverTargetSlot(Drivers.Array[i], j), list[k]);
                    }
            }
            free(list);
        } else if (tag == 0x6e6e7567) {                           // 'gunn'
            if (s->ReadInt() != Gunners.Size) {
                Logger.g->Warning("SUnit::Load: (%s) Number of gunners doesn't match", UnitClassName(this));
                s->ReadChunkSkip();
            } else {
                for (int i = 0; i < Gunners.Size; ++i)
                    LoadVariables(s, Gunners.Array[i], kSGunnerDesc);
            }
        } else if (tag == 0x76697264) {                           // 'driv'
            if (s->ReadInt() != Drivers.Size) {
                Logger.g->Warning("SUnit::Load: (%s) Number of drivers doesn't match", UnitClassName(this));
                s->ReadChunkSkip();
            } else {
                for (int i = 0; i < Drivers.Size; ++i)
                    LoadVariables(s, Drivers.Array[i], DriverSaveDesc(Drivers.Array[i]));
            }
        } else {
            Logger.g->Warning("Unsupported unit sub-tag 0x%08X", tag);
            s->ReadChunkSkip();
        }
        s->ReadChunkValidate(0);
    }
}

} // namespace pz
