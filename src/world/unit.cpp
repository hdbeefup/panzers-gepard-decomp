// src/world/unit.cpp
// Units of the menu world: UNDS loading (HD 0x5f33f0), SWorld::CreateUnit
// (0x5e2da0), the unit heap (0x5d94b0 / 0x5f8060) and the M1 stand-ins for
// the HD unit type and unit classes. OWNER: agent D. Lifted from the HD exe
// only; SWINE world/ and game/ are banned.
//
// M1 limits (see unitregistry.h): SUnitType / SUnit only load the unit's
// model prototypes and place a model instance at the UNTD position and
// direction. No drivers, weapons, AI or SGameLogic::Refresh.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/imodel.h"
#include "stream.h"
#include "properties.h"
#include "logger.h"

namespace pz {

// ---------------------------------------------------------------------------
// SUnitType (stand-in)

SUnitType::SUnitType()
{
    memset((unsigned char*)this + sizeof(void*), 0, sizeof(*this) - sizeof(void*));
    ModelProto = -1;
    WreckProto = -1;
    SquadMaxUnits = 0;
}

SUnitType::~SUnitType()
{
    if (ModelProto >= 0)
        PzGepard()->ReleaseModelPrototype(ModelProto);            // Gepard +0x24
    if (WreckProto >= 0)
        PzGepard()->ReleaseModelPrototype(WreckProto);
    FreeSString(&Name);
    FreeSString(&IniName68);
    FreeSString(&IniName70);
    FreeSString(&ModelName);
    FreeSString(&WreckModelName);
    FreeSString(&SquadMemberName);
}

void SUnitType::LoadHeader(SProperties* p)
{
    ClassType = p->GetInt("Unit", "Unit.Common.ClassType", 0);
    UnitType = p->GetInt("Unit", "Unit.Common.UnitType", 0);
}

void SUnitType::Load(SProperties* p)
{
    LoadHeader(p);
    ModelName = p->GetString("Unit", "Unit.Common.ModelName", "");
    WreckModelName = p->GetString("Unit", "Unit.Common.WreckModelName", "");
    AnimationType = p->GetInt("Unit", "Unit.Animation", 0);
    if (ClassType == UC_SQUAD) {
        SquadMemberName = p->GetString("Unit", "Unit.Common.ClassType.Panzers Squad Unit.SquadMemberName", "");
        SquadMaxUnits = p->GetInt("Unit", "Unit.Common.ClassType.Panzers Squad Unit.MaxNumberOfUnits", 1);
    }
    Loaded = true;                                                // +0xdd
    if (ModelName.size)
        ModelProto = WorldLoadModelPrototype(SStr(ModelName));
    if (WreckModelName.size)
        WreckProto = WorldLoadModelPrototype(SStr(WreckModelName));
    if (ClassType == UC_SQUAD && SquadMemberName.size && g_UnitRegistry)
        g_UnitRegistry->GetPUnit(SStr(SquadMemberName), true);
}

SUnit* SUnitType::CreateUnit(int worldIndex)
{
    return new SUnit(this, worldIndex);
}

// ---------------------------------------------------------------------------
// SUnit (stand-in)

SUnit::SUnit(SUnitType* type, int worldIndex)
    : Type(type), WorldIndex(worldIndex), Player(0), Dir(0.0f), Stored(false),
      Model(nullptr), MemberCount(0), Walker(false)
{
    Pos[0] = Pos[1] = Pos[2] = 0.0f;
    for (int i = 0; i < 16; ++i)
        Members[i] = -1;
}

SUnit::~SUnit()
{
    if (Model) {
        Model->Release();
        Model = nullptr;
    }
}

static void PlaceUnitModel(SUnit* u)
{
    g_WorldStats.UnitModels++;
    if (u->Type->ModelProto < 0)
        return;
    // Flag 1: a free-heap model. Flag 0 puts it on the scene's main heap
    // (static doodads sorted into terrain cells), where SModel plays
    // sequence 0 on the scene clock and soldiers lay down in their first
    // sequence. HD animated models use 1 (SGameLogic::CreateAnimatedModel
    // 0x5649e0, the walker attachments in 0x5ce2a0).
    u->Model = g_Scene->CreateModelFromPrototype(u->Type->ModelProto, 1);   // scene +0x58
    if (!u->Model)
        return;
    g_WorldStats.UnitModelsOk++;
    u->Model->SetPosition(u->Pos[0], u->Pos[1], u->Pos[2]);       // model +0x18
    u->Model->SetRotation(u->Dir, 0.0f, 0.0f);                    // model +0x1c
    if (u->Type->AnimationType == 2) {                            // walker
        // SWalkerAnimation::InitModel 0x5c93a0: PlaySequence of
        // GetGlobalStateStandText 0x5c7f90 ("%s_stand" over the unit's
        // global state name; state 0 = "normal" for the placed menu units),
        // no blend, then model +0x94 SetFlags(7) (interpolate position,
        // nodes, and accumulate the animation per tick).
        u->Model->PlaySequence("normal_stand", false);            // model +0x6c
        u->Model->SetFlags(7);                                    // model +0x94
        u->Walker = true;
    }
    if (u->Type->ClassType == UC_BUILDING) {
        // SBuildingUnit init 0x548f20: the "Block" node (block-map
        // footprint) is hidden, and "Indoor" for unit type 0x1a. (HD also
        // hides "Indoor" when the type's +0x13c is 6; that field is not in
        // the stand-in type.)
        u->Model->SetNodeVisible(u->Model->FindNode("Block"), false);      // model +0x40 / +0x60
        if (u->Type->UnitType == 0x1a)
            u->Model->SetNodeVisible(u->Model->FindNode("Indoor"), false);
    }
    u->Model->SetVisible(true, false);                            // model +0x30(1, 0), UpdateModel 0x5ce2a0
}

// PANZERS 0x5ce2a0 (M1 subset)
// SWalkerAnimation::UpdateModel, the idle path of one logic tick: the model
// is shown (+0x30(1, 0): no fog of war on the menu, World+0x4d0 / no
// SGameLogic visibility) and the stand sequence advances by one tick,
// +0x70(0.05) (LAB_005ce5d7). Vehicles (SVehicleAnimation) only keep their
// pose. The previous-tick pose is stored first, as SGameLogic::Refresh does
// for doodads (model +0x3c), so the render interpolates between ticks.
void SUnit::RefreshModel()
{
    if (!Model)
        return;
    Model->StoreInterpolationState();                             // model +0x3c
    if (Walker)
        Model->AdvanceAnimation(0.05f);                           // model +0x70
}

void SUnit::Initialize(SUnitDef* def)
{
    SWorld* w = g_World;
    Player = def->Player;
    Stored = def->Stored;
    Dir = def->Dir;
    Pos[0] = def->Pos[0];
    Pos[2] = def->Pos[1];
    Pos[1] = w->GetTerrainHeight(Pos[0], Pos[2]) + def->Yrel;
    if (Stored)
        return;                                                   // inside a vehicle: no model
    if (Type->ClassType != UC_SQUAD) {
        PlaceUnitModel(this);
        return;
    }
    // Squad: the members are units of their own (SPanzersSquadMemberUnit).
    // M1 placeholder formation: a line across the squad's direction, 1.5 m
    // apart (HD formations are not lifted).
    SUnitType* mt = Type->SquadMemberName.size ? g_UnitRegistry->GetPUnit(SStr(Type->SquadMemberName), true) : nullptr;
    if (!mt)
        return;
    int n = Type->SquadMaxUnits;
    if (n > 16)
        n = 16;
    float rx = cosf(Dir), rz = -sinf(Dir);
    for (int k = 0; k < n; ++k) {
        int idx = w->AllocUnitSlot();
        SUnit* m = mt->CreateUnit(idx);
        w->Units.Array[idx].Unit = m;
        float off = ((float)k - (float)(n - 1) * 0.5f) * 1.5f;
        m->Player = Player;
        m->Dir = Dir;
        m->Pos[0] = Pos[0] + rx * off;
        m->Pos[2] = Pos[2] + rz * off;
        m->Pos[1] = w->GetTerrainHeight(m->Pos[0], m->Pos[2]) + def->Yrel;
        PlaceUnitModel(m);
        Members[MemberCount++] = idx;
        g_WorldStats.UnitsTotal++;
    }
}

// ---------------------------------------------------------------------------
// The unit heap

// PANZERS 0x5d94b0
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

// PANZERS 0x5f8060
void SWorld::RemoveUnit(int index)
{
    if (!Units.IsLive(index))
        return;
    SUnit* u = Units.Array[index].Unit;
    if (u && Logger.g)
        Logger.g->Log(1, "Remove unit from player %d class %s WorldIdx %d x: %g z: %g",
                      u->Player, SStr(u->Type->Name), index, (double)u->Pos[0], (double)u->Pos[2]);
    // HD: unit vtbl +0x04 (detach), then the scalar deleting dtor.
    delete u;
    Units.Array[index].Unit = nullptr;
    // HD 0x5f7920 (SHeapTRB::Remove): link the slot at the tail of the free
    // list, stamped with the current frame.
    Units.Array[index].Next = -1;
    Units.Array[index].Unit = (SUnit*)(size_t)Units.Frame;
    if (Units.Free < 0)
        Units.Free = index;
    else
        Units.Array[Units.FreeTail].Next = index;
    Units.Count--;
    Units.FreeTail = index;
}

// PANZERS 0x5e2da0
int SWorld::CreateUnit(SUnitDef* def)
{
    SUnitType* type = g_UnitRegistry ? g_UnitRegistry->GetPUnit(SStr(def->ClassName), true) : nullptr;
    if (!type)
        return -1;
    int idx = AllocUnitSlot();
    SUnit* unit = type->CreateUnit(idx);                          // type vtbl +0x10
    Units.Array[idx].Unit = unit;
    unit->Initialize(def);                                        // unit vtbl +0x08
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
        } else {
            GetUnit(c)->Player = unit->Player;                    // child +0x80 = parent +0x80
            // HD: parent vtbl +0x5c(c, child +0x188) stores the unit (M2).
        }
    }
    for (int i = 0; i < def->TowedCount; ++i) {
        int c = CreateUnit(&def->TowedUnits[i]);
        if (c == -1 && Logger.g)
            Logger.g->Warning("SWorld::CreateUnit: Cannot create towed unit '%s'", SStr(def->TowedUnits[i].ClassName));
        // HD: parent vtbl +0x6c (tow, M2).
    }
    // HD: campaign statistics (DAT_00929a0c), not on the menu path.
    return idx;
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
}

} // namespace pz
