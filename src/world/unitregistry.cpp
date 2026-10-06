// src/world/unitregistry.cpp
// SUnitRegistry (HD 0x5cfe30 / 0x5d1050 / 0x5d0c10 / 0x5d0e70), the UNTD
// variable loader (gLoadVariables 0x670440) and SUnitDef. OWNER: agent D.
// Lifted from the HD exe only. See unitregistry.h for what is reduced.

#include <stdlib.h>
#include <string.h>
#include "pzunitregistry.h"
#include "world.h"
#include "worldapi.h"
#include "stream.h"
#include "properties.h"
#include "darray.h"
#include "logger.h"

namespace pz {

SUnitRegistry* g_UnitRegistry = nullptr;

// HD global SProperties loaded by the registry ctor (0x6607d0 on
// "unitvariables.ini"): named constants the .unit files refer to
// ("Building_Village2_Hp", "Vehicle_Sight_Tank", ...).
static SProperties* s_UnitVariables = nullptr;

static void FreeStr(SString* s)
{
    FreeSString(s);
}

// ---------------------------------------------------------------------------
// gLoadVariables

static void ReadVarString(SStream* s, SString* out)
{
    // HD 0x56e7d0
    int len = s->ReadWord() & 0xffff;
    FreeSString(out);
    out->size = len;
    out->buf = new char[len + 1];
    s->Read(out->buf, len);
    out->buf[len] = 0;
}

static float ReadVarFloat(SStream* s)
{
    float f;
    s->Read(&f, 4);
    return f;
}

// PANZERS 0x670e10
void SkipSingleVariable(SStream* s, int type)
{
    switch (type) {
    case 1: s->ReadByte(); return;
    case 2: s->ReadInt(); return;
    case 4: ReadVarFloat(s); ReadVarFloat(s); ReadVarFloat(s); return;
    case 5: { SString tmp; ReadVarString(s, &tmp); FreeSString(&tmp); return; }   // 0x65d6f0
    case 6:
        if (s->ReadChunkHeader() != 0x6c636d5f)   // "_mcl"
            throw "gSkipSingleVariable: Expected member class chunk";
        s->ReadChunkSkip();
        s->ReadChunkValidate(0);
        return;
    case 8: ReadVarFloat(s);   // fall through
    case 3: ReadVarFloat(s); return;
    case 12: s->ReadWord(); return;
    default:
        Logger.g->Panic("gSkipSingleVariable: Not a Single type");
    }
}

// PANZERS 0x670300
void LoadSingleVariable(SStream* s, void* dst, int type, const SVarDesc* members)
{
    switch (type) {
    case 1: *(bool*)dst = s->ReadByte() != 0; return;
    case 2: *(int*)dst = s->ReadInt(); return;
    case 3: *(float*)dst = ReadVarFloat(s); return;
    case 4: {
        float* f = (float*)dst;
        f[0] = ReadVarFloat(s);
        f[1] = ReadVarFloat(s);
        f[2] = ReadVarFloat(s);
        return;
    }
    case 5: ReadVarString(s, (SString*)dst); return;
    case 6:
        if (s->ReadChunkHeader() != 0x6c636d5f)   // "_mcl"
            throw "gLoadSingleVariable: Expected member class chunk";
        LoadVariables(s, dst, members);
        s->ReadChunkValidate(0);
        return;
    case 8: {
        float* f = (float*)dst;
        f[0] = ReadVarFloat(s);
        f[1] = ReadVarFloat(s);
        return;
    }
    case 12: *(unsigned short*)dst = s->ReadWord(); return;
    default:
        Logger.g->Panic("gLoadSingleVariable: Not a Single type");
    }
}

// HD SDArray / dequeue header used by types 7, 9, 10, 11.
struct SVarArray {
    void* Array;
    int Size;
    int Max;
    int Head;     // dequeue only (+0x0c)
    int Last;     // +0x10
    int First;    // +0x14
};

static void AllocVarArray(SVarArray* a, int n, int elemSize)
{
    a->Size = n;
    a->Max = n;
    if (a->Array) {
        free(a->Array);
        a->Array = nullptr;
    }
    if (n != 0) {
        a->Array = malloc((size_t)elemSize * n);   // HD 0x78b859
        memset(a->Array, 0, (size_t)elemSize * n);
    }
}

// PANZERS 0x670440
void LoadVariables(SStream* s, void* obj, const SVarDesc* desc)
{
    if (!obj)
        Logger.g->Panic("::gLoadVariables: NULL parameter");
    int type = s->ReadInt();
    while (type != 0) {
        SString name;
        ReadVarString(s, &name);
        const SVarDesc* d = desc;
        for (; d->Type != 0; ++d) {
            bool typeOk = d->Type == type || (type == 7 && d->Type == 10) || (type == 9 && d->Type == 11);
            if (!typeOk)
                continue;
            if (name.size == 0) {
                if (!d->Name || !*d->Name)
                    break;
            } else if (d->Name && *d->Name && strcmp(name.buf, d->Name) == 0) {
                break;
            }
        }
        if (d->Type == 0) {
            if (Logger.g)
                Logger.g->Log(0, "gLoadVariables: Variable %s is obsolete!", SStr(name));
            switch (type) {
            case 7: {
                int n = s->ReadInt();
                if (n < 0 || n > 0x1000000)
                    throw "gLoadVariables: Invalid array size";
                for (; n > 0; --n)
                    s->ReadInt();
                break;
            }
            case 9: {
                int n = s->ReadInt();
                if (n < 0 || n > 0x1000000)
                    throw "gLoadVariables: Invalid dequeue size";
                s->ReadInt();
                s->ReadInt();
                for (; n > 0; --n)
                    s->ReadInt();
                break;
            }
            case 10: {
                int et = s->ReadInt();
                unsigned n = (unsigned)s->ReadInt();
                if (n > 0x1000000)
                    throw "gLoadVariables: Invalid array size";
                for (; n > 0; --n)
                    SkipSingleVariable(s, et);
                break;
            }
            case 11: {
                int et = s->ReadInt();
                unsigned n = (unsigned)s->ReadInt();
                if (n > 0x1000000)
                    throw "gLoadVariables: Invalid dequeue size";
                s->ReadInt();
                s->ReadInt();
                for (; n > 0; --n)
                    SkipSingleVariable(s, et);
                break;
            }
            default:
                SkipSingleVariable(s, type);
                break;
            }
        } else {
            unsigned char* dst = (unsigned char*)obj + d->Offset;
            SVarArray* a = (SVarArray*)dst;
            switch (type) {
            case 7: {
                unsigned n = (unsigned)s->ReadInt();
                if (n > 0x1000000)
                    throw "gLoadVariables: Invalid array size";
                AllocVarArray(a, (int)n, d->ElemSize);
                for (int i = 0; i < a->Size; ++i)
                    LoadSingleVariable(s, (unsigned char*)a->Array + d->ElemSize * i, d->ElemType, d->Members);
                break;
            }
            case 9: {
                int n = s->ReadInt();
                if (n < 0 || n > 0x1000000)
                    throw "gLoadVariables: Invalid dequeue size";
                a->Size = a->Max = n;
                a->First = a->Head = s->ReadInt();
                a->Last = s->ReadInt();
                AllocVarArray(a, n, d->ElemSize);
                a->Size = a->Max = n;
                if (n != 0 && n != a->Last - a->First + 1)
                    Logger.g->Panic("gLoadVariables: DEQueue is damaged");
                for (int i = 0; i < a->Size; ++i)
                    LoadSingleVariable(s, (unsigned char*)a->Array + d->ElemSize * i, d->ElemType, d->Members);
                break;
            }
            case 10: {
                int et = s->ReadInt();
                unsigned n = (unsigned)s->ReadInt();
                if (n > 0x1000000)
                    throw "gLoadVariables: Invalid array size";
                if (et == d->ElemType) {
                    AllocVarArray(a, (int)n, d->ElemSize);
                    for (int i = 0; i < a->Size; ++i)
                        LoadSingleVariable(s, (unsigned char*)a->Array + d->ElemSize * i, d->ElemType, d->Members);
                } else {
                    for (; n > 0; --n)
                        SkipSingleVariable(s, et);
                }
                break;
            }
            case 11: {
                int et = s->ReadInt();
                int n = s->ReadInt();
                if (n < 0 || n > 0x1000000)
                    throw "gLoadVariables: Invalid dequeue size";
                if (et == d->ElemType) {
                    a->First = a->Head = s->ReadInt();
                    a->Last = s->ReadInt();
                    AllocVarArray(a, n, d->ElemSize);
                    if (n != 0 && n != a->Last - a->First + 1)
                        Logger.g->Panic("gLoadVariables: DEQueue is damaged");
                    for (int i = 0; i < a->Size; ++i)
                        LoadSingleVariable(s, (unsigned char*)a->Array + d->ElemSize * i, d->ElemType, d->Members);
                } else {
                    s->ReadInt();
                    s->ReadInt();
                    for (; n > 0; --n)
                        SkipSingleVariable(s, et);
                }
                break;
            }
            default:
                LoadSingleVariable(s, dst, type, d->Members);
                break;
            }
        }
        FreeSString(&name);
        type = s->ReadInt();
    }
}

// ---------------------------------------------------------------------------
// SUnitDef

// HD descriptor table 0x8ddb48 ({name, type, offset, members, elemSize, elemType}).
extern const SVarDesc kUnitDefDesc[];
const SVarDesc kUnitDefDesc[] = {
    { "ClassName",                5, 0x000, nullptr, 0, 0 },
    { "Player",                   2, 0x008, nullptr, 0, 0 },
    { "XP",                       2, 0x00c, nullptr, 0, 0 },
    { "Pos",                      8, 0x010, nullptr, 0, 0 },
    { "Yrel",                     3, 0x018, nullptr, 0, 0 },
    { "Dir",                      3, 0x01c, nullptr, 0, 0 },
    { "HP",                       3, 0x020, nullptr, 0, 0 },
    { "Ammo",                     3, 0x024, nullptr, 0, 0 },
    { "ScriptID",                 5, 0x058, nullptr, 0, 0 },
    { "AIGroup",                  2, 0x060, nullptr, 0, 0 },
    { "FrontArmor",               3, 0x028, nullptr, 0, 0 },
    { "LeftSideArmor",            3, 0x02c, nullptr, 0, 0 },
    { "RightSideArmor",           3, 0x030, nullptr, 0, 0 },
    { "BackArmor",                3, 0x034, nullptr, 0, 0 },
    { "Behavior",                 2, 0x038, nullptr, 0, 0 },
    { "GlobalState_ActiveState",  2, 0x03c, nullptr, 0, 0 },
    { "Slots[0]",                 2, 0x040, nullptr, 0, 0 },
    { "Slots[1]",                 2, 0x044, nullptr, 0, 0 },
    { "StoredUnits",             10, 0x04c, kUnitDefDesc, 0x80, 6 },
    { "Cargo",                    3, 0x064, nullptr, 0, 0 },
    { "FirstKill",                1, 0x068, nullptr, 0, 0 },
    { "FirstBlood",               1, 0x069, nullptr, 0, 0 },
    { "FirstShot",                1, 0x06a, nullptr, 0, 0 },
    { "FirstVehicleLost",         1, 0x06b, nullptr, 0, 0 },
    { "FirstArmouredVehicleKill", 1, 0x06c, nullptr, 0, 0 },
    { "TowedUnits",              10, 0x070, kUnitDefDesc, 0x80, 6 },
    { "StoredSpecial",            2, 0x07c, nullptr, 0, 0 },
    { nullptr,                    0, 0,     nullptr, 0, 0 },
};

// PANZERS 0x5cfb10
SUnitDef::SUnitDef()
{
    memset(this, 0, sizeof(*this));
    HP = Ammo = 1.0f;
    FrontArmor = LeftSideArmor = RightSideArmor = BackArmor = 1.0f;
    Behavior = 1;
    AIGroup = -1;
    Cargo = 1.0f;
}

// HD 0x560120 per element + free (inline in 0x5f33f0).
SUnitDef::~SUnitDef()
{
    for (int i = 0; i < StoredCount; ++i)
        StoredUnits[i].~SUnitDef();
    free(StoredUnits);
    StoredUnits = nullptr;
    StoredCount = StoredMax = 0;
    for (int i = 0; i < TowedCount; ++i)
        TowedUnits[i].~SUnitDef();
    free(TowedUnits);
    TowedUnits = nullptr;
    TowedCount = TowedMax = 0;
    FreeSString(&ScriptID);
    FreeSString(&ClassName);
}

// PANZERS 0x5cfbd0
void SUnitDef::Load(SStream* s)
{
    LoadVariables(s, this, kUnitDefDesc);   // 0x670440(stream, this, 0x8ddb48)
}

// ---------------------------------------------------------------------------
// SUnitRegistry

// PANZERS 0x5cfe30
SUnitRegistry::SUnitRegistry()
{
    PZ_TRACE("SUnitRegistry::SUnitRegistry (0x5cfe30)");
    memset(this, 0, sizeof(*this));
    g_UnitRegistry = this;
    if (!s_UnitVariables)
        s_UnitVariables = new SProperties("unitvariables.ini", true);   // 0x6607d0
    LoadUnitFiles("units/", false);
    LoadUnitFiles("buildings/", false);
    LoadUnitFiles("units/ingame/", true);
    // HD: +0x0c..+0x120 = [Game] damage/XP/price/range constants from
    // unitvariables.ini, then units.ini display names into each type's
    // +0x68/+0x70 (GetText "Building" for buildings): M2 (game rules).
    if (Logger.g)
        Logger.g->Log(0, "PZ3D world: unit registry %d unit types", Count);
}

// PANZERS 0x5d0c10
SUnitRegistry::~SUnitRegistry()
{
    PZ_TRACE("SUnitRegistry::~SUnitRegistry (0x5d0c10)");
    for (int i = 0; i < Count; ++i) {
        SUnitRegistryEntry* e = &Entries[i];
        if (e->Type) {
            delete e->Type;                                       // vtbl +0 (1)
            e->Type = nullptr;
        }
        FreeStr(&e->Path);
        FreeStr(&e->FileName);
        FreeStr(&e->Name);
    }
    free(Entries);
    Entries = nullptr;
    Max = 0;
    Count = 0;
    if (g_UnitRegistry == this)
        g_UnitRegistry = nullptr;
    if (s_UnitVariables) {
        delete s_UnitVariables;
        s_UnitVariables = nullptr;
    }
}

// PANZERS 0x5d1c40
void SUnitRegistry::RemoveEntry(int index)
{
    if (index < 0 || index >= Count)
        Logger.g->Panic("SDArray<SUnitRegistryEntry>::Remove: invalid index (%d) size = %d", index, Count);
    SUnitRegistryEntry* e = &Entries[index];
    // HD 0x5d0cc0
    if (e->Type) {
        delete e->Type;
        e->Type = nullptr;
    }
    FreeStr(&e->Path);
    FreeStr(&e->FileName);
    FreeStr(&e->Name);
    Count--;
    if (Count - index != 0)
        memmove(&Entries[index], &Entries[index + 1], (Count - index) * sizeof(SUnitRegistryEntry));
    memset(&Entries[Count], 0, sizeof(SUnitRegistryEntry));
}

static int StripExtensionPos(const char* s, int len)
{
    int dot = -1;
    for (int i = 0; i < len; ++i) {
        if (s[i] == '/' || s[i] == '\\')
            dot = -1;
        else if (s[i] == '.')
            dot = i;
    }
    return dot;
}

// HD 0x5d0d80 (qsort comparator on the path).
static int __cdecl CompareEntries(const void* a, const void* b)
{
    const SUnitRegistryEntry* ea = (const SUnitRegistryEntry*)a;
    const SUnitRegistryEntry* eb = (const SUnitRegistryEntry*)b;
    return _stricmp(SStr(ea->Path), SStr(eb->Path));
}

// PANZERS 0x5d1050
void SUnitRegistry::LoadUnitFiles(const char* dir, bool inGame)
{
    SDArray<SString> files;
    memset(&files, 0, sizeof(files));
    FileSystem.FindFiles(dir, "*.unit", &files);                  // 0x65e720("%s%s", dir, "*.unit")
    for (int f = 0; f < files.size; ++f) {
        const SString* file = &files.array[f];
        // Name = file name up to the last '.'.
        SString name;
        name = *file;
        int dot = StripExtensionPos(name.buf, name.size);
        if (dot >= 0) {
            name.buf[dot] = 0;
            name.size = dot;
        }
        bool known = false;
        for (int i = 0; i < Count; ++i) {
            if (Entries[i].Name.size == name.size &&
                (name.size == 0 || _stricmp(Entries[i].Name.buf, name.buf) == 0)) {
                known = true;
                break;
            }
        }
        if (known) {
            FreeStr(&name);
            continue;
        }
        char path[300];
        _snprintf(path, sizeof(path) - 1, "%s%s", dir, SStr(*file));   // 0x5335c0
        path[sizeof(path) - 1] = 0;
        SProperties props(path, true);                            // 0x65fe80
        // HD builds a property tree (0x664960, agent C's SPropertyStruct) and
        // reads "Common" -> "ClassType"; the flat ini key is the same value.
        int classType = props.GetInt("Unit", "Unit.Common.ClassType", 0);
        if (Count == Max) {
            int nmax = Max < 0x10 ? 0x10 : (Max * 6) / 5;
            Entries = (SUnitRegistryEntry*)realloc(Entries, nmax * sizeof(SUnitRegistryEntry));
            memset(&Entries[Max], 0, (nmax - Max) * sizeof(SUnitRegistryEntry));
            Max = nmax;
        }
        int idx = Count++;
        SUnitRegistryEntry* e = &Entries[idx];
        e->Name = *file;
        dot = StripExtensionPos(e->Name.buf, e->Name.size);
        if (dot >= 0) {
            e->Name.buf[dot] = 0;
            e->Name.size = dot;
        }
        e->InGame = inGame;
        e->Path = path;
        e->FileName = *file;
        SUnitType* type = nullptr;
        bool enabled = true;
        switch (classType) {
        case 0: case 0xb: case 0xc: case 0xd:   // new 0x14c, 0x5a4ae0
        case 5:                                 // new 0x14c, 0x5a49e0
        case 9:                                 // new 0x160, 0x5a4900
        case 10:                                // new 0x14c, 0x5a4b30
            type = new SUnitType();
            break;
        case 3:                                 // new 0x178, 0x5a4a20
        case 4:                                 // new 0x14c, 0x5a4ae0
        case 6:                                 // new 0x140, 0x5a49b0
        case 8:                                 // new 0x140, 0x5a4980
            type = new SUnitType();
            enabled = false;                    // HD leaves +0x1c zero on these paths
            break;
        case 7:                                 // new 0x144, 0x5a4db0
            type = new SUnitType();
            enabled = e->Name.size != 0 && _stricmp(e->Name.buf, "Waster Tank Mine") == 0;
            break;
        default:
            RemoveEntry(idx);
            if (Logger.g)
                Logger.g->Warning("SUnitRegistry::SUnitRegistry - %s has bad UnitType: %d", SStr(*file), classType);
            FreeStr(&name);
            continue;
        }
        e->Type = type;
        e->Enabled = enabled;
        if (e->Name.size == 0) {
            RemoveEntry(idx);
            FreeStr(&name);
            continue;
        }
        type->Name = e->Name;                                     // +0x60
        type->LoadHeader(&props);                                 // vtbl +0x04
        type->Side = 6;                                           // +0x48
        if (e->Enabled && type->ClassType != UC_BUILDING) {
            const char* n = SStr(e->Name);
            if (!_strnicmp("US ", n, 3) || !_strnicmp("GB ", n, 3) || !_strnicmp("Fr ", n, 3))
                type->Side = 1;
            else if (!_strnicmp("Ge ", n, 3) || !_strnicmp("Hu ", n, 3))
                type->Side = 0;
            else if (!_strnicmp("SU ", n, 3) || !_strnicmp("Yu ", n, 3))
                type->Side = 2;
            else if (!_strnicmp("Pl ", n, 3))
                type->Side = 4;
        }
        FreeStr(&name);
    }
    if (Count > 1)
        qsort(Entries, Count, sizeof(SUnitRegistryEntry), CompareEntries);
    for (int f = 0; f < files.size; ++f)
        FreeStr(&files.array[f]);
    free(files.array);
}

// PANZERS 0x5d0e70
SUnitType* SUnitRegistry::GetPUnit(const char* name, bool load)
{
    for (int i = 0; i < Count; ++i) {
        if (_stricmp(SStr(Entries[i].Name), name) != 0)
            continue;
        SUnitType* t = Entries[i].Type;
        if (!t)
            break;
        if (load && !t->Loaded) {
            SProperties props(SStr(Entries[i].Path), true);      // 0x65fe80
            t->Load(&props);                                      // vtbl +0x08 (property tree "Unit")
        }
        return t;
    }
    if (Logger.g)
        Logger.g->Warning("SUnitRegistry::GetPUnit - PUnit = NULL, UnitName = %s ", name);
    return nullptr;
}

} // namespace pz
