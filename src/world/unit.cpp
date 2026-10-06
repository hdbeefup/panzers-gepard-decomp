// src/world/unit.cpp
// The world side of the units: the unit heap (SHeapTRB at World+0x4d4,
// AllocUnitSlot 0x5d94b0 / SHeapTRB::Remove 0x5f7920), SWorld::CreateUnit
// (0x5e2da0 from a UNTD definition, 0x5e3170 by class name), RemoveUnit
// 0x5f8060, FindEmptySpace 0x5e58d0 / 0x5e5700, 0x5ef760 and the UNDS
// loader 0x5f33f0. OWNER: agent U. Lifted from the HD exe.
//
// The units themselves are pz::SUnit and subclasses (src/game/unit.h); the
// prototypes are the registry's SPUnit (src/game/punit.h).

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "unit.h"
#include "gunner.h"
#include "unitextern.h"
#include "gamelogic.h"
#include "stream.h"
#include "properties.h"
#include "logger.h"
#include "stub_log.h"

namespace pz {

// ---------------------------------------------------------------------------
// The unit heap

// PANZERS 0x5d94b0
// The free-list head is reused only while Frame <= freed frame + ReuseDelay
// (unsigned); a freed slot carries its frame in the Unit field. Otherwise a
// new slot is appended (growth 16, then * 6 / 5).
int SWorld::AllocUnitSlot()
{
    Units.Count++;
    int i = Units.Free;
    if (i >= 0 && Units.Frame <= (unsigned)(size_t)Units.Array[i].Unit + Units.ReuseDelay) {
        Units.Free = Units.Array[i].Next;
        Units.Array[i].Next = kHeapLive;
        Units.Array[i].Unit = nullptr;
        return i;
    }
    if (Units.Size == Units.Max) {
        int nmax = Units.Max < 0x10 ? 0x10 : (Units.Max * 6) / 5;
        Units.Array = (SUnitHeap::Elem*)realloc(Units.Array, nmax * sizeof(SUnitHeap::Elem));
        memset(&Units.Array[Units.Max], 0, (nmax - Units.Max) * sizeof(SUnitHeap::Elem));
        Units.Max = nmax;
    }
    Units.Array[Units.Size].Next = kHeapLive;
    return Units.Size++;
}

SUnit* SWorld::GetUnit(int index)
{
    if (!Units.IsLive(index))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", index);
    return Units.Array[index].Unit;
}

// PANZERS 0x5f7920
static void HeapRemove(SUnitHeap* h, int index)
{
    if (!h->IsLive(index))
        Logger.g->Panic("SHeapTRB::Remove: invalid index (%d)", index);
    h->Array[index].Next = -1;
    h->Array[index].Unit = (SUnit*)(size_t)h->Frame;
    if (h->Free >= 0) {
        h->Array[h->FreeTail].Next = index;
        h->Count--;
        h->FreeTail = index;
        return;
    }
    h->Count--;
    h->Free = index;
    h->FreeTail = index;
}

// PANZERS 0x5f8060
void SWorld::RemoveUnit(int index)
{
    if (!Units.IsLive(index))
        return;
    SUnit* u = Units.Array[index].Unit;
    if (u && Logger.g)
        Logger.g->Log(1, "Remove unit from player %d class %s WorldIdx %d x: %g z: %g",
                      u->Player, SStr(u->Proto->Name), index, (double)u->Pos[0], (double)u->Pos[2]);
    if (u)
        u->Uninit();                                              // vtbl +0x04
    u = Units.Array[index].Unit;
    if (u) {
        delete u;                                                 // vtbl +0 (1)
        Units.Array[index].Unit = nullptr;
    }
    HeapRemove(&Units, index);                                    // 0x5f7920
}

// ---------------------------------------------------------------------------
// Creation

// PANZERS 0x5e2da0
int SWorld::CreateUnit(SUnitDef* def)
{
    SPUnit* type = g_UnitRegistry ? g_UnitRegistry->GetPUnit(SStr(def->ClassName), true) : nullptr;
    if (!type)
        return -1;
    int idx = AllocUnitSlot();
    if (!Units.IsLive(idx))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", idx);
    Units.Array[idx].Unit = static_cast<SUnit*>(type->CreateUnit(idx));   // type vtbl +0x10
    if (!Units.Array[idx].Unit)
        Logger.g->Panic("SWorld::CreateUnit: %s cannot be created (class %d)", SStr(def->ClassName), type->ClassType);
    Units.Array[idx].Unit->Init(def);                             // unit vtbl +0x08
    g_WorldStats.UnitsTotal++;
    if (Logger.g)
        Logger.g->Log(g_Menu3D.Trace ? 0 : 1, "Creating unit for player %d class %s WorldIdx %d x: %g z: %g",
                      def->Player, SStr(def->ClassName), idx, (double)def->Pos[0], (double)def->Pos[1]);
    for (int i = 0; i < def->StoredCount; ++i) {
        SUnitDef* sd = &def->StoredUnits[i];
        sd->Stored = true;                                        // +0x48
        sd->Behavior = def->Behavior;                             // +0x38
        int c = CreateUnit(sd);
        if (c == -1) {
            if (Logger.g)
                Logger.g->Warning("SWorld::CreateUnit: Cannot create stored unit '%s'", SStr(sd->ClassName));
            continue;
        }
        SUnit* child = GetUnit(c);
        SUnit* parent = GetUnit(idx);
        child->AIGroup = parent->AIGroup;                         // child +0x80 = parent +0x80
        parent->StoreUnit(c, child->StoreMode);                   // vtbl +0x5c(c, child +0x188)
    }
    for (int i = 0; i < def->TowedCount; ++i) {
        int c = CreateUnit(&def->TowedUnits[i]);
        if (c == -1) {
            if (Logger.g)
                Logger.g->Warning("SWorld::CreateUnit: Cannot create towed unit '%s'", SStr(def->TowedUnits[i].ClassName));
            continue;
        }
        GetUnit(idx)->Slot_6C();                                  // vtbl +0x6c (tow; not lifted)
    }
    // HD: campaign statistics (DAT_00929a0c), not on the menu path.
    return idx;
}

// PANZERS 0x5e3170
// By class name: the slot, the unit (+0x10), the container (+0x78), InitNew
// (+0x0c), the script ID (+0x194), then the crew ("XX Crew Squad", the first
// two letters of the class) when asked for and the type takes only crews.
int SWorld::CreateUnit(int player, const char* className, const float* pos, float dir, int p5,
                       float hp, int parent, bool crew, const char* scriptId)
{
    if (!isfinite(pos[0]) || !isfinite(pos[1]) || !isfinite(pos[2]))   // 0x793d6c
        Logger.g->Panic("SWorld::CreateUnit: Unit position is not finite!");
    SPUnit* type = g_UnitRegistry ? g_UnitRegistry->GetPUnit(className, true) : nullptr;   // 0x5d0e70(name, 1)
    if (!type)
        Logger.g->Panic("SWorld::CreateUnit: unknown unit type %s", className);   // HD dereferences null
    int idx = AllocUnitSlot();
    SUnit* u = static_cast<SUnit*>(type->CreateUnit(idx));       // +0x10
    Units.Array[idx].Unit = u;
    if (!u)
        Logger.g->Panic("SWorld::CreateUnit: %s cannot be created (class %d)", className, type->ClassType);
    u->Parent = parent;                                           // +0x78
    u->InitNew(player, pos, dir, p5, hp);                         // +0x0c
    u->ScriptID = scriptId ? scriptId : "";                       // +0x194 (0x52c2c0)
    g_WorldStats.UnitsTotal++;
    if (Logger.g)
        Logger.g->Log(g_Menu3D.Trace ? 0 : 1, "Creating unit for player %d class %s WorldIdx %d x: %g z: %g",
                      player, className, idx, (double)pos[0], (double)pos[2]);
    if (crew && type->OnlyCrew) {
        char name[64];
        name[0] = className[0];
        name[1] = className[0] ? className[1] : 0;
        name[2] = 0;
        strncat(name, " Crew Squad", sizeof(name) - 3);           // Mid(0, 2) + " Crew Squad" (0x7f7824)
        float zero[3] = { 0.0f, 0.0f, 0.0f };
        int c = CreateUnit(player, name, zero, dir, 0, 1.0f, -1, false, "");
        if (c == -1) {
            Logger.g->Warning("Ennek az egysegnek nincs megfelelo crew: %s", className);
        } else if (GetUnit(idx)->StoreUnit(c, 0)) {               // +0x5c(c, 0)
            GetUnit(c)->Unplace();                                // +0x4c
        }
    }
    if (type->ClassType == 9) {
        if (u->ScriptID.size == 0)
            Logger.g->Panic("SWorld::CreateUnit(): ScriptID is empty.");
        STUB_LOG("SWorld::CreateUnit (0x5e3170) building wires (\"wire%d\" nodes)");
    }
    if (g_GameLogic && (u->Parent < 0 || type->ClassType == 10) && !u->_110)
        PzGameLogicUnitCreated(idx);                              // SGameLogic 0x565530
    return idx;
}

// ---------------------------------------------------------------------------
// Free space

// PANZERS 0x5e58d0
// From (x, z) backwards along `dir` in half-metre steps (max(12, 2 * size)
// tries); then HD walks a square spiral around the point (125 rings) and
// logs "did not find empty space". The spiral (block-map tests 0x5da050 /
// 0x5d9c10, P) is not lifted: after the straight tries the start point is
// returned with that log line.
bool SWorld::FindEmptySpace(float x, float z, float dir, int size, unsigned short flags, bool p6, float* out)
{
    if (x < 0.0f || z < 0.0f)
        Logger.g->Panic("SWorld::FindEmptySpace: Negative pos");
    out[0] = x;
    out[1] = z;
    if (size == 0)
        return true;
    double s = sin((double)dir);                                  // 0x78d640
    double c = cos((double)dir);                                  // 0x78d480
    float px = x, pz = z;
    int tries = size * 2 > 0xc ? size * 2 : 0xc;
    for (int i = 0; i < tries; ++i) {
        int xb, zb;
        memcpy(&xb, &px, 4);
        memcpy(&zb, &pz, 4);
        if (!PzBlockMapTest(xb, zb, size, (short)flags) && (!p6 || !PzBlockMapTestUnits(px, pz, size))) {
            out[0] = px;
            out[1] = pz;
            return true;
        }
        px = x - (float)i * (float)s * 0.5f;                     // DAT_007f4538 (not verified)
        pz = z - (float)i * (float)c * 0.5f;
    }
    if (Logger.g)
        Logger.g->Log(1, "SWorld::FindEmptySpace - did not find empty space. Pos: x:%f, z:%f", (double)x, (double)z);
    return false;
}

// PANZERS 0x5e5700
// The unit's own spot (ux, uz) if it is free, else FindEmptySpace towards
// it from (x, z).
bool SWorld::FindEmptySpaceNear(float x, float z, float ux, float uz, int size, unsigned short flags, float* out)
{
    if (x < 0.0f || z < 0.0f)
        Logger.g->Panic("SWorld::FindEmptySpace: Negative pos");
    out[0] = x;
    out[1] = z;
    if (size == 0)
        return true;
    int xb, zb;
    memcpy(&xb, &x, 4);
    memcpy(&zb, &z, 4);
    bool blocked = PzBlockMapTest(xb, zb, 1, (short)flags) != 0;
    if (!blocked && !PzBlockMapTestPath(xb, zb, size, (short)flags, 0, 0))
        return true;
    float dir = (float)atan2((double)(ux - x), (double)(uz - z));   // 0x78d07a
    return FindEmptySpace(x, z, dir, size, flags, true, out);
}

// PANZERS 0x5ef760
// After `unit` got in a vehicle: the units of `player` (all for -1) that
// target it as a unit target drop that order, their gunners stop firing at
// it, and a unit whose current target it was gets +0x190.
void SWorld::UnitStored(int unit, int player)
{
    for (int i = 0; i < Units.Size; ++i) {
        if (!Units.IsLive(i))
            continue;
        SUnit* u = Units.Array[i].Unit;
        if (player != -1 && u->Player != player)
            continue;
        if (u->Parent >= 0)
            continue;
        bool dropped = false;
        int ct = u->Proto->ClassType;
        if (ct != 7 && ct != 3) {
            STarget* t = u->PrimaryTarget;
            if (t && (tgt::I(t, tgt::kKind) == 2 || tgt::I(t, tgt::kKind) == 3) &&
                tgt::I(t, tgt::kType) == 0 && tgt::I(t, tgt::kUnit) == unit)
                u->ClearTargets();                                // +0xc4
            t = u->CurrentTarget;
            if (t && (tgt::I(t, tgt::kKind) == 2 || tgt::I(t, tgt::kKind) == 3) &&
                tgt::I(t, tgt::kType) == 0 && tgt::I(t, tgt::kUnit) == unit) {
                u->Slot_C8();                                     // +0xc8
                dropped = true;
            }
        }
        for (int g = 0; g < u->Gunners.Size; ++g) {
            SGunner* gn = u->Gunners.Array[g];
            if (!gn->Idle && gn->Target && tgt::I(gn->Target, tgt::kType) == 0 &&
                tgt::I(gn->Target, tgt::kUnit) == unit)
                gn->Stop();                                       // +0x28
        }
        if (dropped)
            u->AI_Heartbeat();                                    // +0x190
    }
}

// PANZERS 0x5f33f0
void SWorld::LoadUnitDefinitions(SStream* s)
{
    for (int i = 0; i < Units.Size; ++i)
        if (Units.IsLive(i))
            RemoveUnit(i);
    Units.Size = 0;
    Units.Free = -1;
    Units.Count = 0;
    while (!s->ReadChunkIsEnd()) {
        SUnitDef def;                                             // 0x5cfb10
        if (s->ReadChunkHeader() != 0x44544e55)                   // UNTD
            throw "Unsupported unit type";
        if (s->ReadInt() != 0x30303176)
            throw "Unsupported unit version";
        def.Load(s);                                              // 0x5cfbd0
        g_WorldStats.UnitDefs++;
        int idx = CreateUnit(&def);
        if (idx < 0) {
            // HD: map the class name through the remap table (0x5f4a70; empty
            // for the menu), then retry with the file part without extension.
            SString alt;
            PathFilePart(&def.ClassName, &alt);
            int dot = -1;
            for (int i = 0; i < alt.size; ++i) {
                if (alt.buf[i] == '/' || alt.buf[i] == '\\')
                    dot = -1;
                else if (alt.buf[i] == '.')
                    dot = i;
            }
            if (dot >= 0) {
                alt.buf[dot] = 0;
                alt.size = dot;
            }
            if (alt.size != 0 && _stricmp(SStr(alt), SStr(def.ClassName)) != 0) {
                FreeSString(&def.ClassName);
                def.ClassName = alt;
                idx = CreateUnit(&def);
            }
            FreeSString(&alt);
        }
        if (idx >= 0)
            g_WorldStats.UnitsCreated++;
        s->ReadChunkValidate(0);
    }
    // Recompile-only check of the unit heap against the original's per-tick
    // trace (m2crc, docs/M2_INTERFACES.md 8): PZ_M2_UNITDUMP=1 logs the heap
    // after UNDS as "U 0 <idx> <player> <x> <y> <z> <dir>" (raw float bits).
    if (!getenv("PZ_M2_UNITDUMP") || !Logger.g)
        return;
    Logger.g->Log(0, "PZM2 UNITS seed %08x units %d", RandomSeed, Units.Count);
    for (int i = 0; i < Units.Size; ++i) {
        if (!Units.IsLive(i))
            continue;
        SUnit* u = Units.Array[i].Unit;
        unsigned b[4];
        memcpy(&b[0], &u->Pos[0], 4);
        memcpy(&b[1], &u->Pos[1], 4);
        memcpy(&b[2], &u->Pos[2], 4);
        memcpy(&b[3], &u->Dir, 4);
        Logger.g->Log(0, "PZM2 U 0 %d %d %08x %08x %08x %08x %.3f %.3f %.3f %s", i, u->Player, b[0], b[1], b[2], b[3],
             (double)u->Pos[0], (double)u->Pos[2], (double)u->Dir, SStr(u->Proto->Name));
    }
}

} // namespace pz
