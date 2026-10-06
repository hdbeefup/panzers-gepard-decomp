// src/game/unitbase.cpp
// pz::SUnit: the lifted bodies of the HD unit base class (0x5b2800..0x5c6300).
// OWNER: agent U. The slots still logged are in unit.cpp.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <intrin.h>
#include "unit.h"
#include "target.h"
#include "aigroup.h"
#include "drivermath.h"
#include "gunner.h"
#include "idriver.h"
#include "unitanim.h"
#include "unitextern.h"
#include "gamelogic.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "pz/iscene.h"
#include "pz/imodel.h"
#include "pz/ipixie.h"
#include "logger.h"
#include "doodad.h"
#include "stub_log.h"
#include "m3common.h"

namespace pz {

void UnitExternLinked();


static const float kPiF = 3.1415927f;     // DAT_007f4584
static const float kTwoPiF = 6.2831855f;  // DAT_007f458c

// ---------------------------------------------------------------------------
// World helpers

// HD SHeapTRB::operator[] (0x546490, inline in most callers).
SUnit* WorldUnit(int index)
{
    SWorld* w = g_World;
    if (!w->Units.IsLive(index))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", index);
    return w->Units.Array[index].Unit;
}

// HD inline MSVC rand() on World+0x7518.
int WorldRand()
{
    int r = HdLcg15(HdLcgStep(&g_World->RandomSeed));
    HdRngTrace(_ReturnAddress());
    return r;
}

static int PlayerField(int player, int off)
{
    return *(int*)(g_World->Players[player] + off);           // World+0x170 + player * 0x48
}

template <typename T>
static int ArrayAdd(SUnitArray<T>* a)
{
    if (a->Size == a->Max) {
        int nmax = a->Max < 0x10 ? 0x10 : (a->Max * 6) / 5;
        a->Array = (T*)realloc(a->Array, nmax * sizeof(T));
        memset((void*)&a->Array[a->Max], 0, (nmax - a->Max) * sizeof(T));
        a->Max = nmax;
    }
    return a->Size++;
}

template <typename T>
static void ArrayFree(SUnitArray<T>* a)
{
    free(a->Array);
    a->Array = nullptr;
    a->Size = a->Max = 0;
}

static int AnimStateIndex(SIUnitAnimation* anim, const char* name)   // 0x5c7ed0
{
    if (!anim)
        return 0;
    SPUnitAnimation* p = static_cast<SUnitAnimation*>(anim)->Proto;
    return p ? p->FindState(name) : 0;
}

// A field of P's SDriver / SPDriver at its HD offset (driver.h cannot be
// included next to unitanim.h: both define SHdDArray).
template <typename T>
static T& DriverField(void* d, int off)
{
    return *(T*)((unsigned char*)d + off);
}

static void ReleaseTarget(STarget** slot)
{
    if (*slot) {
        PzTargetRelease(*slot);
        *slot = nullptr;
    }
}

void SUnit::SetTarget(STarget** slot, STarget* t)
{
    if (*slot)
        PzTargetRelease(*slot);
    if (t)
        tgt::I(t, tgt::kRefCount)++;
    *slot = t;
}

SIDriver* SUnit::GetDriver(int index)                         // 0x55cc00
{
    if (index < 0 || index >= Drivers.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SDriver *", index);
    return Drivers.Array[index];
}

SGunner* SUnit::GetGunner(int index)                          // 0x55cc40
{
    if (index < 0 || index >= Gunners.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SGunner *", index);
    return Gunners.Array[index];
}

// ---------------------------------------------------------------------------
// Construction

// PANZERS 0x5b2820
SUnit::SUnit(SPUnit* proto, int worldIndex)
{
    UnitExternLinked();
    memset((unsigned char*)this + sizeof(void*), 0, sizeof(SUnit) - sizeof(void*));
    WorldIndex = worldIndex;
    Proto = proto;
    Parent = -1;
    AIGroup = -1;
    LastRefreshFrame = -1;
    ActiveDriver = PrevDriver = RefreshDriver = -1;
    MainGunner = -1;
    Invulnerable = proto->Invulnerable;                       // +0x8b
    _112 = proto->Selectable;                                 // +0x8c
    HP = 1.0f;
    Armor[0] = Armor[1] = Armor[2] = Armor[3] = 1.0f;
    _12c = 1.0f;
    _134 = -1;
    _f8 = 1;
    StuckFrame = -1;
    GhostFrames.Top = -1;
    _1f0 = -1;
    Anim = proto->PAnimation ? proto->PAnimation->CreateAnimation(this) : nullptr;   // draws the world seed twice
    for (int i = 0; i < proto->GunnerCount; ++i) {
        int idx = ArrayAdd(&Gunners);
        if (i >= proto->PGunners.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "class SPGunner *", i);
        Gunners.Array[idx] = proto->PGunners.Array[i]->CreateGunner(this, Anim, idx);   // +0x10
    }
    for (int i = 0; i < proto->DriverCount; ++i) {
        int idx = ArrayAdd(&Drivers);
        SIPDriver* pd = i < proto->PDrivers.Size ? proto->PDrivers.Array[i] : nullptr;
        Drivers.Array[idx] = pd ? pd->CreateDriver(this) : nullptr;                   // +0x10
    }
    _60 = -10000;
    _20c = true;
    Behavior = 1;
    _254 = -1;
    BuiltInDriverUnit = -1;
    for (int i = 0; i < 12; ++i) {
        _26c[i] = (int)0x80000001;
        LastSeenFrame[i] = (int)0x80000001;
    }
    Towed = -1;
    _18c = -1;
    _2e0 = proto->ClassType != 5 ? 0x1e : 0;
    if (proto->ClassType == 10)
        _2e0 = 0x6e;
    if (Anim) {
        GlobalState = AnimStateIndex(Anim, "normal");         // 0x5c7ed0
        DesiredState = GlobalState;
    }
    _2e9 = true;                                              // 0x1000100
    _2eb = true;
    _2e4 = -1;
    Cargo = 1.0f;
    UnitSize = proto->UnitSize;
    UnitSizeBlocks = (int)(UnitSize * 4.0f);                  // DAT_007f4588
    UnitSizeBlocks2 = UnitSizeBlocks;
    _330 = (int)0x80000000;
}

// PANZERS 0x5b33f0
SUnit::~SUnit()
{
    ArrayFree(&_320);
    ArrayFree(&StaticEffects);
    ArrayFree(&WayPoints);
    FreeSString((SString*)&_25c);                             // +0x25c SString (A: sub-state)
    ArrayFree(&ChildUnits);
    free(GhostFrames.Array);                                  // 0x5b32f0
    GhostFrames.Array = nullptr;
    ArrayFree(&InvalidTargets);
    ArrayFree(&SightUnits);
    ArrayFree(&NearUnits);
    ArrayFree(&Orders);
    FreeSString(&ScriptID);
    ArrayFree(&Members);
    ArrayFree(&Stored);
    FreeSString((SString*)&_160);                             // +0x160 SString (A: forced sequence)
    ArrayFree(&Gunners);
    ArrayFree(&Drivers);
}

// PANZERS 0x5b7e40
void SUnit::Uninit()
{
    if (g_Pixie)
        for (int i = 0; i < StaticEffects.Size; ++i)
            g_Pixie->StopEffect(StaticEffects.Array[i]);         // pixie +0x34 (HD 0x5b7e75)
    StopEffects();                                            // 0x5c3030
    Remove(false);                                            // +0x70
    if (AIGroup != -1) {
        if (AIGroup > -1) {
            AIGroupAt(AIGroup)->RemoveUnit(WorldIndex);           // 0x5f7990
            if (AIGroupAt(AIGroup)->Units.Size == 0)
                AIGroupsRemove(g_World, AIGroup);                 // 0x5be040
        }
        AIGroup = -1;
    }
    if (_7c && g_World->Units.IsLive(Parent))
        WorldUnit(Parent)->Towed = -1;
    if (OnBlockMap)
        SetOnBlockMap(false);                                 // +0x198
    if (Anim) {
        delete Anim;
        Anim = nullptr;
    }
    if (Model) {
        Model->SetVisible(false, false);                      // +0x30(0, 0)
        Model->Release();
        Model = nullptr;
    }
    if (Model2) {
        Model2->SetVisible(false, false);
        Model2->Release();
        Model2 = nullptr;
    }
    if (FlagModel) {
        FlagModel->Release();
        FlagModel = nullptr;
    }
    for (int i = 0; i < Gunners.Size; ++i) {
        delete Gunners.Array[i];
        Gunners.Array[i] = nullptr;
    }
    for (int i = 0; i < Drivers.Size; ++i) {
        delete Drivers.Array[i];
        Drivers.Array[i] = nullptr;
    }
    for (int i = 0; i < Members.Size; ++i)
        g_World->RemoveUnit(Members.Array[i].Unit);           // 0x5f8060
    for (int i = 0; i < Stored.Size; ++i)
        g_World->RemoveUnit(Stored.Array[i].Unit);
    for (int i = 0; i < ChildUnits.Size; ++i)
        g_World->RemoveUnit(ChildUnits.Array[i]);
    if (BuiltInDriverUnit >= 0)
        g_World->RemoveUnit(BuiltInDriverUnit);
    if (g_GameLogic)
        g_GameLogic->RemoveUnitFromMovementGroup(WorldIndex);         // 0x579510 (the unit leaves the running triggers)
    ReleaseTarget(&PrimaryTarget);
    ReleaseTarget(&CurrentTarget);
}

// ---------------------------------------------------------------------------
// Init

// PANZERS 0x5c0cb0
void SUnit::SetActiveDriver(int index)
{
    if (index >= 0 && DriverLocked)
        return;
    if (Drivers.Size <= index)
        Logger.g->Panic("SUnit::SetActiveDriver: Invalid value.");
    RefreshDriver = -1;
    PrevDriver = ActiveDriver;
    ActiveDriver = index;
    if (index == -1 && g_GameLogic)
        g_GameLogic->RemoveUnitFromMovementGroup(WorldIndex);         // 0x579510
}

// PANZERS 0x5c0c10
// Leaves the old AI group (World+0x4f4; an emptied group is removed) and joins
// the new one.
void SUnit::SetAIGroup(int group)
{
    if (AIGroup == group)
        return;
    if (AIGroup > -1) {
        AIGroupAt(AIGroup)->RemoveUnit(WorldIndex);               // 0x55ccc0, 0x5f7990
        if (AIGroupAt(AIGroup)->Units.Size == 0)
            AIGroupsRemove(g_World, AIGroup);                     // 0x5be040
    }
    AIGroup = group;
    if (group > -1)
        AIGroupAt(group)->AddUnit(WorldIndex);                    // 0x5d9560
}

// PANZERS 0x5c1170
void SUnit::InitMoveFlags()
{
    SPUnit* p = Proto;
    MoveFlags = p->ClassType != 0xc ? 0x400 : 0x200;
    MoveFlags |= 0x5816;
    if (p->ClassType == 5 || p->ClassType == 6 || p->OnlyWalkerFlag)
        MoveFlags &= 0xbfff;
    if (PlayerField(Player, 8) == 0)                          // World+0x178 + player * 0x48
        MoveFlags |= 1;
    if (p->ClassType == 0 || p->ClassType == 0xb) {
        int demolish = static_cast<SPSingleUnit*>(p)->DemolishType;
        if (demolish > 0)
            MoveFlags &= 0xffef;
        if (demolish > 1)
            MoveFlags &= 0xefff;
    }
}

// PANZERS 0x5b7d70
void SUnit::InitModel()
{
    SPUnit* p = Proto;
    if (!p)
        Logger.g->Panic("SUnit::InitModel: No Prototype!!!!");
    unsigned flags;
    if (!Wrecked || p->WreckProto < 0) {
        Model = g_Scene->CreateModel(p->ModelProto, p->LowPolyProto, p->ClassType != 9);   // scene +0x54
        flags = 3;
    } else {
        Model = g_Scene->CreateModelFromPrototype(p->WreckProto, p->ClassType != 9);       // scene +0x58
        flags = 0;
    }
    if (Model)
        Model->SetFlags(flags);                                // +0x94
    if (p->Proto5C >= 0) {
        Model2 = g_Scene->CreateModelFromPrototype(p->Proto5C, 1);
        if (Model2)
            Model2->SetFlags(0xb);
    }
    if (p->Invisible && Model)
        Model->Slot_BC();                                      // +0xbc(3) (slot not named in imodel.h)
    if (Anim && Model)
        Anim->InitModel(Model);                                // +0x04
    else if (Model)
        M1InitModelPose();
}

// PANZERS 0x5b7390
void SUnit::SetGlobalState(int state, int p2)
{
    GlobalState = PrevGlobalState = DesiredState = _ec = 0;
    _f0 = StateChanging = false;
    StateChangeTime = 0;
    _f8 = 0;
    GlobalState = state;
    _ec = state;
    if (!Anim)
        return;
    int type = static_cast<SUnitAnimation*>(Anim)->Type;
    if (type != 2 && type != 1 && type != 9 && type != 8)
        return;
    SString* sub = (SString*)&_25c;                           // +0x25c sub-state text
    if (sub->size != 0 && GlobalState != AnimStateIndex(Anim, "vehicle"))
        *sub = "";
    static_cast<SUnitAnimation*>(Anim)->PlayGlobalStand(p2 != 0);   // 0x5cae80
}

// PANZERS 0x5c2a70
// One persistent pixie effect per static effect slot of the prototype that
// names a mesh, attached to that node of the model (pixie +0x30; not in the
// recompile's pixie interface yet, so the handles stay unset).
void SUnit::CreateStaticEffects()
{
    for (int i = 0; i < Proto->PilotLightEffects.Size; ++i) {
        if (Proto->PilotLightEffects.Array[i].MeshName.size == 0)
            continue;
        int idx = ArrayAdd(&StaticEffects);
        StaticEffects.Array[idx] = -1;
    }
}

// PANZERS 0x5c2c30
void SUnit::InitEffects()
{
    CreateStaticEffects();
    // HD: pixie +0x28 attaches every static effect (prototype +0xf0) with a
    // mesh to the model node (model +0x40 FindNode). Not in the pixie
    // interface of the recompile.
}

// PANZERS 0x5c3030
void SUnit::StopEffects()
{
    _320.Size = 0;                                            // pixie +0x34 per handle, then remove
}

// PANZERS 0x5ba8e0
void SUnit::Init(SUnitDef* def)
{
    HP = def->HP;
    memcpy(&XP, &def->XP, 4);                                 // +0x64 = UNTD +0x0c, raw dword (0x5ba8fe)
    FirstKill = def->FirstKill;
    FirstBlood = def->FirstBlood;
    FirstShot = def->FirstShot;
    FirstVehicleLost = def->FirstVehicleLost;
    FirstArmouredVehicleKill = def->FirstArmouredVehicleKill;
    StoreMode = def->StoredSpecial;
    ScriptID = SStr(def->ScriptID);
    if (def->AIGroup >= 0)
        SetAIGroup(def->AIGroup - 1);
    Unplaced = def->Stored;
    if (Gunners.Size > 0)
        Gunners.Array[0]->AmmoLeft = def->Ammo;
    Armor[0] = def->FrontArmor;
    Armor[1] = def->LeftSideArmor;
    Armor[2] = def->RightSideArmor;
    Armor[3] = def->BackArmor;
    Behavior = def->Behavior;
    _118 = 0.0f;
    Player = def->Player;
    Team = PlayerField(Player, 0);
    Pos[0] = PrevPos[0] = PrevPos2[0] = def->Pos[0];
    Pos[1] = PrevPos[1] = PrevPos2[1] = 0.0f;
    Pos[2] = PrevPos[2] = PrevPos2[2] = def->Pos[1];
    Yrel = def->Yrel;
    float d = (float)fmod(def->Dir, 6.283185307179586);       // 0x793cba
    if (d > kPiF)
        d -= kTwoPiF;
    else if (d <= -kPiF)
        d += kTwoPiF;
    Dir = PrevDir = PrevDir2 = d;
    Hook20((int)(intptr_t)def->Slots);                        // +0x20(&def->Slots) (M3-C5: HD passes the address; squads 0x59fab0 read both)
    if (g_World && PlayerField(Player, 8) == 1) {
        _140 = true;
        _14c = true;
    }
    if (Proto->DriverCount == 0) {
        if (Drivers.Size < 0)
            Logger.g->Panic("SUnit::SetActiveDriver: Invalid value.");
        RefreshDriver = -1;
        PrevDriver = ActiveDriver;
        ActiveDriver = -1;
        if (g_GameLogic)
            g_GameLogic->RemoveUnitFromMovementGroup(WorldIndex);
    } else if (!DriverLocked) {
        if (Drivers.Size < 1)
            Logger.g->Panic("SUnit::SetActiveDriver: Invalid value.");
        RefreshDriver = -1;
        PrevDriver = ActiveDriver;
        ActiveDriver = 0;
    }
    MainGunner = Proto->GunnerCount ? Proto->MainGunner : -1;
    for (int i = 0; i < Proto->DriverCount; ++i)
        if (GetDriver(i))
            GetDriver(i)->Init();                             // +0x10
    RandomSide = (rand() * 2) >> 15;                          // 0x78c846 (CRT rand)
    InitMoveFlags();                                          // 0x5c1170
    InitModel();                                              // 0x5b7d70
    SetGlobalState(def->GlobalState, 0);                      // 0x5b7390(def +0x3c, 0) (0x5bab68)
    if (Proto->ClassType != 5 && GlobalState > 0 && Anim) {
        SPUnitAnimation* pa = static_cast<SUnitAnimation*>(Anim)->Proto;
        if (pa && pa->StateCount() <= GlobalState) {
            Logger.g->Warning("SUnit::Init: invalid global state %d", GlobalState);
            GlobalState = 0;
        }
    }
    if (!Unplaced) {
        if (Proto->ClassType != 4)
            SetOnBlockMap(true);                              // +0x198
        if (!Unplaced && TestBlockMap(*(int*)&Pos[0], *(int*)&Pos[2], *(int*)&Dir, UnitSizeBlocks, (short)MoveFlags))
            Logger.g->Warning("A unit is in Static Blockmap! - Unit Name: %s, X: %f, Z: %f",
                              SStr(Proto->Name), (double)Pos[0], (double)Pos[2]);
    }
    InitEffects();                                            // 0x5c2c30
}

// PANZERS 0x5bace0
void SUnit::InitNew(int player, const float* pos, float dir, int p4, float hp)
{
    (void)p4;
    HP = hp;
    Player = player;
    _118 = 0.0f;
    Armor[0] = Armor[1] = Armor[2] = Armor[3] = 1.0f;
    _12c = 1.0f;
    Team = PlayerField(player, 0);
    for (int i = 0; i < 3; ++i)
        Pos[i] = PrevPos[i] = PrevPos2[i] = pos[i];
    float d = (float)fmod(dir, 6.283185307179586);
    if (d > kPiF)
        d -= kTwoPiF;
    else if (d <= -kPiF)
        d += kTwoPiF;
    Dir = PrevDir = PrevDir2 = d;
    if (Proto->DriverCount == 0) {
        RefreshDriver = -1;
        PrevDriver = ActiveDriver;
        ActiveDriver = -1;
        if (g_GameLogic)
            g_GameLogic->RemoveUnitFromMovementGroup(WorldIndex);
    } else if (!DriverLocked) {
        if (Drivers.Size < 1)
            Logger.g->Panic("SUnit::SetActiveDriver: Invalid value.");
        RefreshDriver = -1;
        PrevDriver = ActiveDriver;
        ActiveDriver = 0;
    }
    MainGunner = Proto->GunnerCount ? Proto->MainGunner : -1;
    for (int i = 0; i < Proto->DriverCount; ++i)
        if (GetDriver(i))
            GetDriver(i)->Init();
    RandomSide = (rand() * 2) >> 15;
    InitMoveFlags();
    InitModel();
    SetGlobalState(GlobalState, 0);
    SetOnBlockMap(true);
    InitEffects();
}

// PANZERS 0x54cd40
void SUnit::Hook20(int p1)
{
    (void)p1;
}

// ---------------------------------------------------------------------------
// Placement

// PANZERS 0x5ba850
void SUnit::Unplace()
{
    Unplaced = 1;
    InVehicleAnim = 0;
}

// PANZERS 0x5c5160
void SUnit::Place(float x, float z, float dir)
{
    Unplaced = 0;
    InVehicleAnim = 0;
    if (Model && Anim)
        Model->Slot_CC();                                     // +0xcc(anim +0x3c GetShadowTexture) (slot not named)
    Pos[0] = x;
    Pos[2] = z;
    Dir = dir;
    RefreshModel();                                           // +0x3c
    StoreInterpolationState();                                // +0x16c
}

// PANZERS 0x5c1980
// Teleport. The towed unit follows through +0x74 GhostFrames_AddTop (not
// lifted: nothing tows on the menu); seat passengers of class 0xb follow.
void SUnit::SetPosition(float x, float z, int dirBits, int yrelBits)
{
    float dir, yrel;                                          // HD passes the raw dwords of +0xb0 / +0x88
    memcpy(&dir, &dirBits, 4);
    memcpy(&yrel, &yrelBits, 4);
    SetOnBlockMap(false);
    Pos[0] = x;
    Pos[2] = z;
    Dir = dir;
    Yrel = yrel;
    RefreshModel();
    StoreInterpolationState();
    SetOnBlockMap(true);
    if (Towed >= 0) {
        // HD: the towed unit leaves the block map, takes the position the
        // tow bar gives it (+0x74 GhostFrames_AddTop(pos, dir, +0x300,
        // towed, &pos, &dir, &+0x300)), goes back on it and is teleported
        // there with its own +0x24.
        SUnit* t = WorldUnit(Towed);
        (void)t;
        STUB_LOG("SUnit::SetPosition (0x5c1980) towed unit (+0x74)");
    }
    if (_7c && (_104 & 1) != 0) {
        SUnit* parent = WorldUnit(Parent);
        if ((parent->_104 & 1) == 0)
            parent->Remove(false);                            // +0x70(0)
    }
    if (Proto->ClassType == 0xb) {
        // A gun's crew on the ground (not attached) stands at its seat nodes.
        for (int i = 0; i < Members.Size; ++i) {
            if (Members.Array[i].Attached)
                continue;
            float p[3] = { 0.0f, 0.0f, 0.0f };
            Model->GetNodePosition(Members.Array[i].Node, p); // model +0x54
            SUnit* m = WorldUnit(Members.Array[i].Unit);
            m->Pos[0] = p[0];
            m->Pos[1] = p[1];
            m->Pos[2] = p[2];
            WorldUnit(Members.Array[i].Unit)->Dir = Dir;
        }
    }
}

// PANZERS 0x5b5ad0
void SUnit::StoreInterpolationState()
{
    PrevPos2[0] = PrevPos[0];
    PrevPos2[1] = PrevPos[1];
    PrevPos2[2] = PrevPos[2];
    PrevDir2 = PrevDir;
    PrevPos[0] = Pos[0];
    PrevPos[1] = Pos[1];
    PrevPos[2] = Pos[2];
    PrevDir = Dir;
    if (Model)
        Model->StoreInterpolationState();                     // +0x3c
    if (Model2)
        Model2->StoreInterpolationState();
    if (FlagModel)
        FlagModel->StoreInterpolationState();
}

// PANZERS 0x5c6130
void SUnit::RefreshModel()
{
    if (Anim)
        Anim->UpdateModel();                                  // +0x08
    else
        M1RefreshModel();
    bool hero = Proto->HeroPicture >= 0;
    for (int i = 0; !hero && i < Stored.Size; ++i)
        if (WorldUnit(Stored.Array[i].Unit)->Proto->HeroPicture >= 0)
            hero = true;
    if (hero && Members.Size > 0) {
        if (!FlagModel) {
            int team = PlayerField(Player, 4);                // World+0x174 + player * 0x48
            int proto = team == 0 ? g_World->FlagProto[0] : (team == 2 ? g_World->FlagProto[2] : g_World->FlagProto[1]);
            FlagModel = g_Scene->CreateModelFromPrototype(proto, 1);   // scene +0x58
            if (FlagModel) {
                FlagModel->SetFlags(0x115);
                FlagModel->SetPosition(Pos[0], Pos[1], Pos[2]);
                FlagModel->StoreInterpolationState();
                FlagModel->SetVisible(false, false);
            }
        }
        if (FlagModel) {
            float c[3];
            GetCenterPosition(c);                             // +0x1c8
            FlagModel->SetPosition(c[0], c[1], c[2]);
            FlagModel->AdvanceAnimation(0.05f);               // +0x70
        }
        return;
    }
    if (FlagModel) {
        FlagModel->Release();
        FlagModel = nullptr;
    }
}

// PANZERS 0x5b76c0
void SUnit::UpdateVisuals(SIViewport* vp)
{
    if (Anim)
        Anim->Slot_0C(vp);                                    // anim +0x0c (tail jump with the viewport)
}

// PANZERS 0x5b9d40
void SUnit::GetCenterPosition(float* out)
{
    out[0] = out[1] = out[2] = 0.0f;
    if (Model)
        Model->GetRenderPosition(out);                        // +0x14
}

// ---------------------------------------------------------------------------
// Per tick

// PANZERS 0x5bee90
// The per-player sighting 0x562b10 (units with +0x6d) is not lifted.
void SUnit::ServerRefresh(int frame)
{
    if (LastRefreshFrame == frame)
        return;
    LastRefreshFrame = frame;
    if (Frozen)
        return;
    if (--EffectTimer == 0)
        EnableStaticEffects();                                // 0x5c2200
    if (CurrentTarget && RefreshDriver > -1) {
        GetDriver(RefreshDriver)->Refresh();                  // +0x14
        if (DriverField<void*>(GetDriver(RefreshDriver), 0xc0) == nullptr)   // SDriver +0xc0 Target
            RefreshDriver = -1;
    }
    if (Wrecked) {                                            // +0x150
        RefreshDriverEffects();                               // 0x5bd910
        RefreshDead();                                        // +0x30 dead unit refresh
        return;
    }
    if (Orders.Size != 0 && PrimaryTarget == nullptr) {
        if (Orders.Size <= 0)                                 // 0x5b36e0(0)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "struct SUnit::SOrder", 0);
        SOrder o = Orders.Array[0];
        // 0x5bdfa0(0): SDArray::Remove
        Orders.Size--;
        if (Orders.Size != 0)
            memmove(&Orders.Array[0], &Orders.Array[1], Orders.Size * sizeof(SOrder));
        memset(&Orders.Array[Orders.Size], 0, sizeof(SOrder));
        ExecuteCommand(o);                                    // 0x5b95a0
    }
    RefreshInvalidTargets();                                  // 0x5bdcd0
    int gf = g_GameLogic ? g_GameLogic->GetFrame() : frame;   // 0x56d1a0
    if ((gf + WorldIndex) % 20 == 0) {
        RefreshTargeting();                                   // +0x34
        int ct = Proto->ClassType;
        if (ct == 0 || ct == 0xb || ct == 0xc || ct == 10 || ct == 5) {
            if (g_GameLogic)
                g_GameLogic->UpdateActiveLocations(this, true);   // 0x582080
        }
    }
    if (PrimaryTarget && !CurrentTarget) {
        Logger.g->Warning("SUnit::ServerRefresh: There's a PrimaryTarget but no CurrentTarget. Unitname: %s, idx:%d, Frame:%d",
                          SStr(Proto->Name), WorldIndex, gf);
        AI_Heartbeat();                                       // +0x190
    }
    if (PrimaryTarget && !PzTargetRefresh(PrimaryTarget, WorldIndex)) {   // 0x5bd210
        STarget* t = PrimaryTarget;
        if (tgt::I(t, tgt::kType) == 0 && tgt::I(t, tgt::kKind) == 2 && g_World->Units.IsLive(tgt::I(t, tgt::kUnit)) &&
            !WorldUnit(tgt::I(t, tgt::kUnit))->Wrecked &&
            (0.0f < tgt::F(t, 0x10) || 0.0f < tgt::F(t, 0x18))) {
            EC_AttackMove(tgt::F(t, 0x10), tgt::F(t, 0x18), 0);   // +0xcc: on to the target's last position
        } else {
            ClearTargets();                                   // +0xc4
        }
    }
    if (CurrentTarget && !PzTargetRefresh(CurrentTarget, WorldIndex)) {
        if (!PrimaryTarget) {
            STarget* t = CurrentTarget;
            if (tgt::I(t, tgt::kType) == 0 && tgt::I(t, tgt::kKind) == 2 && g_World->Units.IsLive(tgt::I(t, tgt::kUnit)) &&
                !WorldUnit(tgt::I(t, tgt::kUnit))->Wrecked &&
                (0.0f < tgt::F(t, 0x10) || 0.0f < tgt::F(t, 0x18))) {
                EC_AttackMove(tgt::F(t, 0x10), tgt::F(t, 0x18), 0);   // +0xcc
            } else {
                ClearTargets();
            }
        } else {
            ReleaseTarget(&CurrentTarget);
            AI_Heartbeat();                                   // +0x190
        }
    }
    if (_6d && !IsHiddenInBlockMap()) {                       // 0x5bb5c0
        // Each player that sees the unit (0x562b10) stamps its frame.
        for (int p = 0; p < 12; ++p)
            if (g_GameLogic->IsSeenByPlayer(p, this))                 // 0x562b10
                LastSeenFrame[p] = g_GameLogic->Frame;                // +0x29c
    }
    RefreshMisc();                                            // +0x38
    RefreshDriverEffects();                                   // 0x5bd910
    if (RemoveMe)
        g_World->RemoveUnit(WorldIndex);                      // 0x5f8060
}

// PANZERS 0x5bc5c0
// A moving armoured unit not hidden by the block map is stamped as seen
// (+0x26c[player] = logic frame) by every player it is in vision of.
void SUnit::UpdateSeenByPlayers()
{
    if (!(MoveFlags & 1)) {
        int size = UnitSizeBlocks < 1 ? 1 : UnitSizeBlocks;
        if (TestBlockMap(*(int*)&Pos[0], *(int*)&Pos[2], *(int*)&Dir, size, 1))   // +0x1a8
            return;
    }
    for (int p = 0; p < 12; ++p)
        if (g_GameLogic->IsInPlayerVision(p, this))          // 0x562650
            _26c[p] = g_GameLogic->Frame;                     // DAT_008f2078 +0x08
}

// PANZERS 0x5c01e0 (entry test)
// Repairing: only for a current target of kind 6 (never in the menu).
void SUnit::RefreshRepairTarget(float range)
{
    (void)range;
    if (!CurrentTarget || tgt::I(CurrentTarget, tgt::kKind) != 6)
        return;
    STUB_LOG("SUnit::RefreshRepairTarget (0x5c01e0) repair target");
}

// PANZERS 0x5bf280 (entry test)
// Resupplying: only for a current target of kind 7 (never in the menu).
void SUnit::RefreshSupplyTarget(float range)
{
    (void)range;
    if (!CurrentTarget || tgt::I(CurrentTarget, tgt::kKind) != 7)
        return;
    STUB_LOG("SUnit::RefreshSupplyTarget (0x5bf280) supply target");
}

// PANZERS 0x5bd600
void SUnit::RefreshTargeting()
{
}

// PANZERS 0x5bdee0
void SUnit::RefreshMisc()
{
}

// ---------------------------------------------------------------------------
// Orders

// PANZERS 0x5c0d10
void SUnit::SetCurrentTarget(STarget* target, int p2)
{
    (void)p2;
    int kind = tgt::I(target, tgt::kKind);
    if (kind == 1)
        CopyDriverWayPoints();                                // 0x5bee10
    if ((kind == 2 || kind == 3) && MainGunner >= 0) {
        tgt::I(target, tgt::kP20) = 1;
        SGunner* g = GetGunner(MainGunner);
        if (g->_68 == 0) {
            SetTarget(&CurrentTarget, target);
            g->SetTarget(CurrentTarget);                      // gunner +0x18
            if (g->PendingTarget) {
                PzTargetRelease(g->PendingTarget);
                g->PendingTarget = nullptr;
            }
            if (ActiveDriver >= 0 && GetDriver(ActiveDriver))
                GetDriver(ActiveDriver)->SetTarget(CurrentTarget);   // driver +0x08
        } else {
            if (g->PendingTarget)
                PzTargetRelease(g->PendingTarget);
            tgt::I(target, tgt::kRefCount)++;
            g->PendingTarget = target;
        }
    } else {
        SetTarget(&CurrentTarget, target);
        StopGunners();                                        // +0xec
        if (ActiveDriver >= 0) {
            SIDriver* d = GetDriver(ActiveDriver);
            if (d)
                d->SetTarget(CurrentTarget); // +0x08
        }
    }
    if (_18c != -1) {
        if (g_World->Units.IsLive(_18c))
            WorldUnit(_18c)->_190 = false;
        _18c = -1;
    }
}

// PANZERS 0x5b8ea0
void SUnit::EC_Move(int xBits, int zBits, int p3, bool p4, int p5)
{
    float x, z;                                               // HD passes the raw float dwords
    memcpy(&x, &xBits, 4);
    memcpy(&z, &zBits, 4);
    if (ActiveDriver < 0)
        return;
    STarget* t = PzTargetNew(0);                              // new 0x38, 0x5b27c0(0)
    tgt::I(t, tgt::kType) = p4 ? 3 : 2;
    tgt::F(t, tgt::kPos) = x;
    tgt::F(t, tgt::kPos + 4) = g_World->GetTerrainHeight(x, z);   // 0x5e7730
    if (p4)
        tgt::I(t, tgt::kP1C) = p5;
    tgt::F(t, tgt::kPos + 8) = z;
    SetTarget(&PrimaryTarget, t);
    SetCurrentTarget(t, p3);                                  // +0xa0
}

// PANZERS 0x5b8e20
void SUnit::EC_MoveAlongPath(int path, int p2, int p3)
{
    if (ActiveDriver < 0)
        return;
    STarget* t = PzTargetNew(0);
    tgt::I(t, tgt::kPath) = path;
    tgt::I(t, tgt::kType) = 2;
    tgt::I(t, tgt::kPathPt) = p2;
    PzTargetConsumePath(t);                                   // STarget::ConsumePath 0x5b7c50
    SetTarget(&PrimaryTarget, t);
    SetCurrentTarget(t, p3);
}

// PANZERS 0x5b8ba0
void SUnit::EC_Follow(int unit, int p2)
{
    if (!IsTargetable(unit, true))                            // 0x5bb6b0(unit, 1)
        return;
    if (unit == WorldIndex)
        return;
    if (WorldUnit(unit)->Proto->ClassType == 3)
        Logger.g->Panic("SUnit::EC_Follow: Attacking projectile");
    if (ActiveDriver < 0)
        return;
    STarget* t = PzTargetNew(0);
    tgt::I(t, tgt::kType) = 0;
    tgt::I(t, tgt::kUnit) = unit;
    SetTarget(&PrimaryTarget, t);
    SetCurrentTarget(t, p2);
}

// PANZERS 0x5b91b0
void SUnit::Stop()
{
    if (ActiveDriver < 0)
        return;
    ReleaseTarget(&CurrentTarget);
    SIDriver* d = GetDriver(ActiveDriver);
    PzDriverReset(d, true);                                   // 0x55bf50(1)
    SIDriver* sd = GetDriver(ActiveDriver);
    int speedSteps = DriverField<int>(sd, 0xd4);             // SDriver +0xd4 SpeedSteps
    if (speedSteps == 1)
        return;
    // A unit still too fast to stop in one speed step brakes towards a
    // stop target at its own position (driver +0x0c SetTargetStopped).
    float maxSpeed = DriverField<float>(sd->GetPDriver(), 0x08);   // SPDriver +0x08 MaxSpeed
    float steps = (Speed / maxSpeed) * (float)speedSteps;
    if ((int)steps > 1) {                                     // cvttss2si
        STarget* t = PzTargetNew(0);                          // new 0x38, 0x5b27c0(0)
        tgt::I(t, tgt::kType) = 2;
        tgt::F(t, tgt::kPos) = Pos[0];
        tgt::F(t, tgt::kPos + 4) = Pos[1];
        tgt::F(t, tgt::kPos + 8) = Pos[2];
        if (_cc)
            tgt::B(t, tgt::kFlag2C) = 1;
        SetTarget(&CurrentTarget, t);
        GetDriver(ActiveDriver)->SetTargetStopped(CurrentTarget);   // driver +0x0c
        return;
    }
    PzDriverReset(GetDriver(ActiveDriver), true);             // 0x55bf50(1)
}

// PANZERS 0x5b90b0
void SUnit::ClearTargets()
{
    ReleaseTarget(&CurrentTarget);
    ReleaseTarget(&PrimaryTarget);
    StopGunners();                                            // +0xec
    Stop();                                                   // +0xc0
}

// PANZERS 0x5b9110
void SUnit::StopGunners()
{
    if (MainGunner < 0)
        return;
    for (int i = 0; i < Gunners.Size; ++i)
        Gunners.Array[i]->Stop();                             // gunner +0x28
}

// PANZERS 0x5b8960
void SUnit::SetBehavior(int behavior)
{
    DesiredState = behavior;
    if (_f0 || StateChanging)
        return;
    if (GlobalState == behavior)
        return;
    int old = GlobalState;
    GlobalState = behavior;
    PrevGlobalState = old;
    if (!Anim)
        return;
    int type = static_cast<SUnitAnimation*>(Anim)->Type;
    if (type != 2 && type != 1 && type != 9 && type != 8)
        return;
    SString* sub = (SString*)&_25c;
    if (sub->size != 0 && GlobalState != AnimStateIndex(Anim, "vehicle"))
        *sub = "";
    int ticks = static_cast<SUnitAnimation*>(Anim)->StateChangeTicks();   // 0x5c7c80
    StateChanging = true;
    StateChangeTime = (int)((float)ticks * 0.8f);             // cvtdq2ps, mulss DAT_007f83dc (0.8f), cvttss2si
}

// PANZERS 0x546ac0
void SUnit::RestoreBehavior()
{
}

// ---------------------------------------------------------------------------
// Storage

// HD 0x549ab0 (inline in many places): same team, or the same player when
// the player has no team (World+0x17c + player * 0x48).
static bool UnitsSameSide(int a, int b)
{
    int team = *(int*)(g_World->Players[a] + 0x0c);
    return team != 0 ? team == *(int*)(g_World->Players[b] + 0x0c) : a == b;
}

// PANZERS 0x5b7040
// Whether `unit` may get in: storage capacity, class, side, the building
// rules (a type-5 building takes anybody up to its capacity; trains never;
// a building held by one allied unit, or on the static block map, takes no
// more) and the hero seat of crew-only vehicles (a hero takes the extra
// seat once).
bool SUnit::CanStoreUnit(int unit)
{
    SPUnit* p = Proto;
    if (p->StorageCapacity == 0)
        return false;
    SUnit* u = WorldUnit(unit);
    int uct = u->Proto->ClassType;
    if (uct != 5 && uct != 0)
        return false;
    if (p->ClassType != 9 && Player != u->Player && !_110)
        return false;
    if (uct == 5 && p->StorageType != 2 && p->StorageType != 0)
        return false;
    if ((uct == 0 || uct == 0xb) && p->StorageType != 2 && p->StorageType != 1)
        return false;
    if (p->ClassType == 9) {
        if (static_cast<SPBuildingUnit*>(p)->BuildingType == 5 && Stored.Size < p->StorageCapacity)   // +0x13c
            return true;
        if (u->Proto->IsTrain)                                    // +0xe8
            return false;
        if (Stored.Size == 1) {
            if (Stored.Size < 1)
                Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SUnitStored", 0);
            int a = WorldUnit(Stored.Array[0].Unit)->Player;
            int b = u->Player;
            int team = *(int*)(g_World->Players[a] + 0x0c);       // 0x549ab0
            bool same = team != 0 ? team == *(int*)(g_World->Players[b] + 0x0c) : a == b;
            if (same)
                return false;
        }
        if (*((unsigned char*)this + 0x454))                      // SBuildingUnit +0x454 OnStaticBlock
            return false;
    }
    bool onlyCrew = p->OnlyCrew;                                  // +0xdc
    if (onlyCrew && u->Proto->ClassType == 5 && u->Proto->UnitType != 0xe && u->Proto->HeroPicture < 0)
        return false;                                             // crew-only: squads only as crew / heroes
    if (_7c) {                                                    // riding outside: only trains carry
        if (p->ClassType != 10)
            return false;
        int pi = Parent;
        if (pi > -1) {
            SUnit* root = nullptr;
            do {
                root = WorldUnit(pi);
                pi = root->Parent;
            } while (pi >= 0);
            if (root->Player != u->Player)
                return false;
        }
    }
    if (IsStored)                                                 // +0x184
        return false;
    int cap = p->StorageCapacity;
    if (!onlyCrew)
        return Stored.Size < cap;
    if (u->Proto->HeroPicture < 0) {
        for (int i = 0; i < Stored.Size; ++i)
            if (WorldUnit(Stored.Array[i].Unit)->Proto->HeroPicture >= 0)
                return Stored.Size < cap + 1;                     // a stored hero has the extra seat
        return Stored.Size < cap;
    }
    for (int i = 0; i < Stored.Size; ++i)
        if (WorldUnit(Stored.Array[i].Unit)->Proto->HeroPicture >= 0)
            return Stored.Size < cap;                             // the hero seat is taken
    return Stored.Size < cap + 1;
}

// PANZERS 0x5c30d0
// A squad gets in a vehicle: each member takes a seat (driver, gunner,
// passenger) and the squad becomes the vehicle's crew. Lifted for the
// vehicle case; the member attach to the seat nodes (model +0xdc) and the
// building case are not (they need model slots not in imodel.h).
bool SUnit::StoreUnit(int unit, int mode)
{
    SUnit* u = WorldUnit(unit);
    if (!CanStoreUnit(unit))
        return false;
    if (mode == 0 && u->Proto->HeroPicture >= 0)
        mode = 2;
    float limit = g_UnitRegistry->ThermoIn * Proto->Thermostat;   // registry +0x3c * prototype +0x9c
    if (limit <= _118)
        return false;
    u->Unplace();                                             // +0x4c
    u->Parent = WorldIndex;
    u->IsStored = true;
    u->StoreMode = mode;
    u->SetAIGroup(u->AIGroup);
    u->SetAIGroup(-1);
    if (u->HasReturnPos) {
        u->HasReturnPos = false;
        HasReturnPos = true;
        ReturnX = u->ReturnX;
        ReturnZ = u->ReturnZ;
    }
    int si = ArrayAdd(&Stored);                               // 0x546830
    Stored.Array[si].Unit = unit;
    Stored.Array[si].Mode = mode;
    if (u->Proto->ClassType == 5) {
        int guns = Proto->GunnerCount;
        bool needDriver = Proto->DriverCount != 0 && !Proto->BuiltInDriver;
        for (int i = 0; i < Members.Size; ++i) {
            int seat = Members.Array[i].Seat;
            if (seat == 1)
                needDriver = false;
            if (seat == 3)
                guns--;
            if (seat == 2) {
                guns--;
                needDriver = false;
            }
        }
        _110 = false;
        if (_10c == 0 && !Proto->BuiltInDriver && Proto->ClassType != 0xd)
            _10c = u->_10c;
        Player = u->Player;
        for (int m = 0; m < u->Members.Size; ++m) {
            int mi = ArrayAdd(&Members);
            SUnitMember* seat = &Members.Array[mi];
            seat->Unit = u->Members.Array[m].Unit;
            seat->Owner = u->WorldIndex;
            if (!needDriver) {
                if (guns < 1) {
                    seat->Seat = 4;
                } else {
                    seat->Seat = 3;
                    int g = Proto->GunnerCount - guns;
                    guns--;
                    GetGunner(g)->Active = 1;
                    seat->Gunner = g;
                }
            } else {
                needDriver = false;
                seat->Seat = 1;
                if (!DriverLocked) {
                    if (Drivers.Size < 1)
                        Logger.g->Panic("SUnit::SetActiveDriver: Invalid value.");
                    RefreshDriver = -1;
                    PrevDriver = ActiveDriver;
                    ActiveDriver = 0;
                }
                if (Proto->ClassType == 0xb && guns > 0) {
                    seat->Seat = 2;
                    int g = Proto->GunnerCount - guns;
                    guns--;
                    GetGunner(g)->Active = 1;
                    seat->Gunner = g;
                }
            }
            // HD: seat node from the animation's crew slots (anim +0x18,
            // "StoreUnit - storedmembers_array_idx>UnitsInVehicle.GetSize()").
            SUnitAnimation* va = static_cast<SUnitAnimation*>(Anim);
            if (va && mi < va->Slots.Count)
                seat->Node = va->Slots.Data[mi].Node;
            SUnit* member = WorldUnit(seat->Unit);
            member->Parent = WorldIndex;
            member->ClearTargets();                           // +0xc4
            member->_f8 = 1;
            *(int*)&member->Speed = 0;
            if (mode != 3)
                member->SetGlobalState(AnimStateIndex(member->Anim, "normal"), 0);
            int kind = va && mi < va->Slots.Count ? va->Slots.Data[mi].Kind : 0;
            if (kind == 0) {
                seat->Attached = false;
            } else if (kind == 10) {
                // Kneeling seat: the member stays on the ground at the
                // seat node, not attached to the vehicle model.
                seat->Attached = false;
                float p[3] = { 0.0f, 0.0f, 0.0f };
                Model->GetNodePosition(seat->Node, p);                // model +0x54
                member->Pos[0] = p[0];
                member->Pos[1] = p[1];
                member->Pos[2] = p[2];
                member->InVehicleAnim = 1;
                member->Pos[1] = g_World->GetTerrainHeight(member->Pos[0], member->Pos[2]);   // 0x5e7730
                member->Dir = Dir;
                *(SString*)&member->_25c = "";
                member->_f0 = false;
                member->SetBehavior(AnimStateIndex(member->Anim, "kneel"));   // +0x12c
            } else {
                static const char* const kSeat[] = { nullptr, "_driver", "_sitgun", "_standgun", "_sitpass",
                                                     "_standpass", "_kneelgun", "_mg", "_ground_gun",
                                                     "_ground_gun2", nullptr, "_leftboat", "_rightboat" };
                if (kind < 0 || kind > 12 || !kSeat[kind])
                    Logger.g->Panic("SUnit::StoreUnit - Unknown UNIT_IN_VEHICLE_ANIM_ substring");
                *(SString*)&member->_25c = kSeat[kind];
                member->InVehicleAnim = 1;
                member->Model->AttachTo(Model, seat->Node);           // member model +0xdc at the seat node
                seat->Attached = true;
                member->SetGlobalState(AnimStateIndex(member->Anim, "vehicle"), mode == 3 ? 0 : 1);
            }
        }
        u->Members.Size = 0;
        if (u->Members.Max > 0)
            memset(u->Members.Array, 0, u->Members.Max * sizeof(SUnitMember));
        u->SetBehavior(4);                                    // +0x12c
    } else {
        _110 = false;
        // HD: "vehicle%d" node of this model; the stored vehicle attaches there.
        u->InVehicleAnim = 1;
    }
    if (!_112) {
        u->_104 = 0;
    } else {
        if (u->_104 & 1) {
            _104 |= 1;
            u->_104 &= ~1u;
        }
        _108 |= u->_108;
    }
    u->_108 = 0;
    if (g_GameLogic)
        g_GameLogic->RemoveUnitFromMovementGroup(u->WorldIndex);      // 0x579510
    if (!Proto->BuiltInDriver && Stored.Size == 1)
        Behavior = u->Behavior;
    // HD: when the crew's race does not match this unit's, the per-player
    // sight flags (+0x2cc) are rebuilt with CanSeeGroundUnit.
    g_World->UnitStored(WorldIndex, u->Player);               // 0x5ef760
    if (g_GameLogic)
        g_GameLogic->Dispatch_571380(WorldIndex, u->WorldIndex);   // 0x571380
    return true;
}


// ---------------------------------------------------------------------------
// Queries

// PANZERS 0x54a240
bool SUnit::HasWoundedMember()
{
    return false;
}

// PANZERS 0x5b9e60
int SUnit::GetRank()
{
    float xp = XP;                                            // movss +0x64
    const SUnitRegistry* r = g_UnitRegistry;
    if (xp < (float)r->XpLevel[0])
        return 0;
    if (xp < (float)r->XpLevel[1])
        return 1;
    if (xp < (float)r->XpLevel[2])
        return 2;
    return ((float)r->XpLevel[3] <= xp) + 3;
}

// PANZERS 0x5b9f60
float SUnit::GetMaxRange(int weapon)
{
    if (weapon >= 0 && weapon < Gunners.Size)
        return Gunners.Array[weapon]->GetPGunner()->MaxRange;
    return 0.0f;
}

// PANZERS 0x5b9f90
float SUnit::GetMinRange(int weapon)
{
    if (weapon >= 0 && weapon < Gunners.Size)
        return Gunners.Array[weapon]->GetPGunner()->MinRange;
    return 0.0f;
}

// PANZERS 0x5ba240
float SUnit::GetSightRange()
{
    if (_138 != 6 && _144 != 6)
        return Proto->Sight;
    return (float)g_UnitRegistry->BinocularsRange;
}

// PANZERS 0x548230
float SUnit::GetExtra188()
{
    return 0.0f;
}

// PANZERS 0x5b9ed0
// The active (or refresh) driver's prototype move speed (SPDriver +0x08),
// halved towards the towed unit's speed. Needs the SPDriver layout (P).
float SUnit::GetMoveSpeed(int p1)
{
    (void)p1;
    int i = RefreshDriver >= 0 ? RefreshDriver : ActiveDriver;
    if (i < 0)
        return 0.0f;
    SIDriver* d = GetDriver(i);
    SIPDriver* pd = d ? d->GetPDriver() : nullptr;            // +0x04
    if (!pd)
        return 0.0f;
    float speed = *(float*)((unsigned char*)pd + 8);
    if (Towed > 0 && Proto->ClassType == 0) {
        float other = *(float*)((unsigned char*)WorldUnit(Towed) + 0x2dc);
        if (other < speed)
            speed = (other + speed) * 0.5f;
    }
    return speed;
}

// PANZERS 0x5c21d0
void SUnit::SetUnitSize()
{
    UnitSize = Proto->UnitSize;
    UnitSizeBlocks = (int)(UnitSize * 4.0f);
    UnitSizeBlocks2 = UnitSizeBlocks;
}

// ---------------------------------------------------------------------------
// Block map (P's world functions)

// PANZERS 0x5bc660
void SUnit::SetOnBlockMap(bool on)
{
    OnBlockMap = on;
    PzBlockMapSetUnit(Pos[0], Pos[2], UnitSizeBlocks, on);    // 0x5f4430
}

// PANZERS 0x5b74c0
void SUnit::Slot_19C()
{
    PzBlockMapTestUnits(Pos[0], Pos[2], UnitSizeBlocks);      // 0x5d99f0
}

// PANZERS 0x5bc6d0
void SUnit::MarkBlockMap(bool on, int p2, int p3, int p4, short p5)
{
    (void)p4;
    PzBlockMapMark(p2, p3, UnitSizeBlocks, on, p5);           // 0x5f4720
}

// PANZERS 0x5b7520
int SUnit::TestBlockMapPath(int p1, int p2, int p3, int p4, short p5, int p6, int p7)
{
    (void)p3;
    return PzBlockMapTestPath(p1, p2, p4, p5, p6, p7);        // 0x5d9eb0
}

// PANZERS 0x5b7500
int SUnit::TestBlockMap(int p1, int p2, int p3, int p4, short p5)
{
    (void)p3;
    return PzBlockMapTest(p1, p2, p4, p5);                    // 0x5d9e00
}

// ---------------------------------------------------------------------------
// M1 visuals without an animation (the prototype has no SP*Animation: A's
// LoadPUnitAnimation returned none). HD always has one; kept for unit types
// whose animation class is not lifted.

void SUnit::M1InitModelPose()
{
    if (!Model)
        return;
    Model->SetPosition(Pos[0], Pos[1], Pos[2]);
    Model->SetRotation(Dir, 0.0f, 0.0f);
    if (Proto->_138 == 2) {                                   // walker
        Model->PlaySequence("normal_stand", false);
        Model->SetFlags(7);
    }
    if (Proto->ClassType == 9) {
        Model->SetNodeVisible(Model->FindNode("Block"), false);
        if (Proto->UnitType == 0x1a)
            Model->SetNodeVisible(Model->FindNode("Indoor"), false);
    }
    if (!Unplaced && Proto->_138 != 4)
        Model->SetVisible(true, false);
}

void SUnit::M1RefreshModel()
{
    if (!Model)
        return;
    if (Proto->_138 == 2)
        Model->AdvanceAnimation(0.05f);
}

// ---------------------------------------------------------------------------
// M3-C2: unloading, behaviour, selection speech (slots typed by C2)

// PANZERS 0x5bce20
void SUnit::SpeakSelected()
{
    g_World->UnitSpeech(WorldIndex, 0, false);                    // 0x5fff20 "Selection"
}

// PANZERS 0x5b8920
void SUnit::SetFireBehavior(int behavior)
{
    if (behavior == 2)
        StopGunners();                                            // +0xec
    if (Behavior != behavior) {                                   // +0x250
        Behavior = behavior;
        RefreshTargeting();                                       // +0x34
    }
}

// PANZERS 0x5c6000
// Every stored unit gets out (+0x64), the last first; a unit without a
// driver drops its primary target.
void SUnit::UnloadAll()
{
    int i = Stored.Size;
    if (i == 0)
        return;
    while (--i > -1) {
        if (i >= Stored.Size)
            Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SUnitStored", i);
        UnloadUnit(Stored.Array[i].Unit);                         // +0x64
    }
    if (ActiveDriver < 0)
        ReleaseTarget(&PrimaryTarget);                            // +0x1f8
}

// PANZERS 0x5b93f0
// Unload the stored unit at index (-1: all). A moving vehicle first brakes
// towards a "stop to unload" target (kind 0xb) at its position; the driver
// unloads when it stops (+0x48 / OnDriverReachedTarget).
void SUnit::Unload(int index)
{
    if (Stored.Size == 0)
        return;
    int unit = index;
    if (index != -1) {
        if (index < -1 || index >= Stored.Size)
            return;
        unit = Stored.Array[index].Unit;                          // 0x546450
    }
    if (ActiveDriver < 0) {
        UnloadUnit(unit);                                         // +0x64
        if (ActiveDriver < 0)
            ReleaseTarget(&PrimaryTarget);
        return;
    }
    ReleaseTarget(&CurrentTarget);
    PzDriverReset(GetDriver(ActiveDriver), true);                 // 0x55bf50(1)
    if (DriverField<int>(GetDriver(ActiveDriver), 0xd4) == 1 || Speed <= 0.0f) {
        UnloadUnit(unit);
        if (ActiveDriver < 0)
            ReleaseTarget(&PrimaryTarget);
        return;
    }
    STarget* t = PzTargetNew(0xb);                                // new 0x38, 0x5b27c0(0xb)
    tgt::I(t, tgt::kP28) = unit;
    tgt::I(t, tgt::kType) = 2;
    tgt::F(t, tgt::kPos) = Pos[0];
    tgt::F(t, tgt::kPos + 4) = Pos[1];
    tgt::F(t, tgt::kPos + 8) = Pos[2];
    if (_cc)
        tgt::B(t, tgt::kFlag2C) = 1;
    SetTarget(&CurrentTarget, t);
    GetDriver(ActiveDriver)->SetTargetStopped(CurrentTarget);    // driver +0x0c
}


// ---------------------------------------------------------------------------
// M3-C2: the attack orders

static int PlayerKind(int player)                                 // World+0x178 + player * 0x48
{
    return *(int*)(g_World->Players[player] + 0x08);
}

// PANZERS 0x5b8440
// Attack a unit: a primary target of kind 2 (type 0, the unit, +0x20 = 1),
// then +0xa0. Not for "type 2" players, untargetable units or oneself.
void SUnit::EC_Attack(int unit, int queue)
{
    PZ_M3_TRACE("SUnit::EC_Attack (0x5b8440)");
    if (PlayerKind(Player) == 2 || !IsTargetable(unit, true) || unit == WorldIndex)   // 0x5bb6b0
        return;
    int cls = WorldUnit(unit)->Proto->ClassType;
    if (cls == 3) {
        Logger.g->Log(1, "SUnit::EC_Attack: Attacking projectile");   // HD 0x65c810(1)
        return;
    }
    if (cls == 6 || WorldUnit(unit)->Proto->ClassType == 4)
        Logger.g->Panic("SUnit::EC_Attack: Trying to attack a squad member unit instead of a squad.");
    STarget* t = PzTargetNew(2);                                  // new 0x38, 0x5b27c0(2)
    tgt::I(t, tgt::kType) = 0;
    tgt::I(t, tgt::kUnit) = unit;
    tgt::I(t, tgt::kP20) = 1;
    SetTarget(&PrimaryTarget, t);                                 // +0x1f8
    SetCurrentTarget(PrimaryTarget, queue);                       // +0xa0
}

// PANZERS 0x5b8660
// Move to (x, z) attacking on the way: primary target of kind 4 at the
// ground position, +0xa0, then +0x190 AI_Heartbeat.
void SUnit::EC_AttackMove(float x, float z, int queue)
{
    PZ_M3_TRACE("SUnit::EC_AttackMove (0x5b8660)");
    if (PlayerKind(Player) == 2 || ActiveDriver < 0)
        return;
    if (0.0f > x || 0.0f > z)
        Logger.g->Panic("SUnit::EC_AttackMove: Negative pos");
    STarget* t = PzTargetNew(4);                                  // new 0x38, 0x5b27c0(4)
    float xz[2] = { x, z };
    t->SetGroundPos(xz);                                          // 0x5c1860
    SetTarget(&PrimaryTarget, t);
    SetCurrentTarget(PrimaryTarget, queue);                       // +0xa0
    AI_Heartbeat();                                               // +0x190
}

// PANZERS 0x5b8740
// Attack-move along a map path from a node: kind 4, type 2, path / point,
// STarget::ConsumePath, +0xa0, +0x190.
void SUnit::EC_AttackAlongPath(int path, int node, int queue)
{
    PZ_M3_TRACE("SUnit::EC_AttackAlongPath (0x5b8740)");
    if (PlayerKind(Player) == 2 || ActiveDriver < 0)
        return;
    STarget* t = PzTargetNew(4);
    tgt::I(t, tgt::kPath) = path;
    tgt::I(t, tgt::kType) = 2;
    tgt::I(t, tgt::kPathPt) = node;
    t->ConsumePath();                                             // 0x5b7c50
    SetTarget(&PrimaryTarget, t);
    SetCurrentTarget(PrimaryTarget, queue);
    AI_Heartbeat();
}

// ---------------------------------------------------------------------------
// M3-C2: the death effects of the prototype (visual only)

// One prototype effect list: an effect per entry, hung on its mesh node,
// or (no such node) played at the unit position raised by yOffset.
static void PlayUnitEffects(SUnit* u, const SUnitArray<SPUnitEffect>& fx, float yOffset)
{
    for (int i = 0; i < fx.Size; ++i) {
        const char* mesh = fx.Array[i].MeshName.buf ? fx.Array[i].MeshName.buf : "";
        int node = u->Model->FindNode(mesh);                      // model +0x40
        if (node < 0) {
            float pos[3] = { u->Pos[0], u->Pos[1] + yOffset, u->Pos[2] };
            float dir[3] = { 0.0f, 1.0f, 0.0f };
            g_Pixie->PlayEffect(g_Scene, fx.Array[i].Proto, pos, dir, 0);       // pixie +0x24
        } else {
            g_Pixie->PlayEffectOnNode(g_Scene, fx.Array[i].Proto, u->Model, node, 0);   // pixie +0x28
        }
    }
}

// PANZERS 0x5c2910
// The prototype's +0x120 effects (after a death by fire, from 0x5c26e0).
void SUnit::PlayDestroyEffects()
{
    PlayUnitEffects(this, Proto->DestroyEffects, 0.0f);
}

// PANZERS 0x5c2590
// The prototype's +0x114 effects, 0.75 above the unit when not on a node.
void SUnit::PlayDiedByFireEffects()
{
    PlayUnitEffects(this, Proto->DiedByFireEffects, 0.75f);       // 0x7f2fcc
}

// PANZERS 0x5c26e0
// Death: the +0x120 effects when burnt (+0x152), then the +0x108 die
// effects (1.25 above the unit without a node); an entry whose mesh is
// "refer to first mesh" is created at the model position and handed to
// the model (pixie +0x58), then released.
void SUnit::PlayDeathEffects()
{
    if (_151[1])                                                  // +0x152 died by fire
        PlayDestroyEffects();
    const SUnitArray<SPUnitEffect>& fx = Proto->DieEffects;       // +0x108
    for (int i = 0; i < Proto->DieEffects.Size; ++i) {
        const char* mesh = fx.Array[i].MeshName.buf ? fx.Array[i].MeshName.buf : "";
        int node = Model->FindNode(mesh);
        if (fx.Array[i].MeshName.size != 0 && _stricmp(fx.Array[i].MeshName.buf, "refer to first mesh") == 0) {
            float pos[3] = { 0.0f, 0.0f, 0.0f };
            Model->GetPosition(pos);                              // model +0x10
            float dir[3] = { 0.0f, 1.0f, 0.0f };
            int h = g_Pixie->CreateEffect(g_Scene, fx.Array[i].Proto, pos, dir);   // pixie +0x2c
            STUB_LOG("SUnit::PlayDeathEffects (0x5c26e0) pixie +0x58(effect, model) (slot untyped, E)");
            g_Pixie->ReleaseEffect(h);                            // pixie +0x38
        } else if (node < 0) {
            float pos[3] = { Pos[0], Pos[1] + 1.25f, Pos[2] };    // 0x7fd6f4
            float dir[3] = { 0.0f, 1.0f, 0.0f };
            g_Pixie->PlayEffect(g_Scene, fx.Array[i].Proto, pos, dir, 0);
        } else {
            g_Pixie->PlayEffectOnNode(g_Scene, fx.Array[i].Proto, Model, node, 0);
        }
    }
}

// ---------------------------------------------------------------------------
// M3-C2: small SUnit helpers

// PANZERS 0x5bcdf0
// Order speech: "Movement" (3) with a driver, else "CantMove" (4).
void SUnit::SpeakMoveOrder()
{
    g_World->UnitSpeech(WorldIndex, ActiveDriver > -1 ? 3 : 4, false);   // 0x5fff20
}

// PANZERS 0x5c50c0
// The selection (+0x104 bit 0) and the +0x108 bits go over to `to`.
void SUnit::HandSelectionTo(SUnit* to)
{
    if (_104 & 1) {
        to->_104 |= 1;
        _104 &= ~1u;
    }
    to->_108 |= _108;
    _108 = 0;
}

// PANZERS 0x5b6e10
bool SUnit::IsRecent26c(int player)
{
    return g_GameLogic && g_GameLogic->Frame - 0x28 < _26c[player];   // +0x26c, 40 ticks
}

// PANZERS 0x5b6e40
// Seen by the player within the last 40 ticks (+0x29c, ServerRefresh).
bool SUnit::WasSeenRecently(int player)
{
    return g_GameLogic && g_GameLogic->Frame - 0x28 < LastSeenFrame[player];
}

// PANZERS 0x5be2b0
// The wreck: the prototype's wreck model (+0x58) replaces the model in the
// scene (scene +0x60), colour override off, flags 0, invisible units' model
// +0xbc(3), and the animation re-binds to it (+0x04).
void SUnit::SetWreckModel()
{
    if (Proto->WreckProto < 0)
        return;
    Model->SetColor2(false, 0);                                   // model +0xf0(0, 0)
    g_Scene->ReplaceModel(Model, Proto->WreckProto);              // scene +0x60
    Model->SetFlags(0);                                           // model +0x94
    if (Proto->Invisible)                                         // +0x89
        STUB_LOG("SUnit::SetWreckModel (0x5be2b0) model +0xbc(3) (slot untyped, E)");
    if (Anim)
        Anim->InitModel(Model);                                   // +0x04
}

// PANZERS 0x5c51d0
// A stored unit gets out (-1: all, +0x68). It is placed at a free spot
// next to the carrier on the unload side (prototype +0x2c: 0..3 quarter
// turns, 2 = random from the world LCG), facing away. A squad takes its
// member rows back from the carrier's seats (+0x178; a later crew moves
// into a freed seat) and its members are scattered round the spot (two
// world LCG draws each). Then the unit is free (+0x78 -1, +0x9c(1), AI
// group, return point, the AI primary target), the "leaves" trigger event
// (0x571570) fires and the stored row goes. An emptied carrier without a
// built-in driver goes neutral (+0x110) and leaves its AI group; a towed
// emptied gun (+0x2e4) becomes a wreck.
bool SUnit::UnloadUnit(int unit)
{
    PZ_M3_TRACE("SUnit::UnloadUnit (0x5c51d0)");
    if (Stored.Size == 0)
        return false;
    if (unit == -1) {
        UnloadAll();                                              // +0x68
        return true;
    }
    int si = 0;
    for (; si < Stored.Size; ++si)
        if (Stored.Array[si].Unit == unit)
            break;
    if (si >= Stored.Size)
        return false;
    SUnit* u = WorldUnit(unit);
    float angle;
    if (Proto->UnloadDirection == 2) {
        int r = WorldRand();                                      // inline LCG
        angle = (float)((double)r * 3.0517578125e-05 * kHdTwoPi); // 0x7f4540, 0x7f4570
    } else {
        double a = (double)Dir - (double)Proto->UnloadDirection * 1.5707963705062866;   // 0x7f5a38
        if (a > kHdPi)
            a = a - kHdTwoPi;
        else if (kHdMinusPi > a)
            a = a + kHdTwoPi;
        angle = (float)a;
    }
    float s = (float)(-DSin((double)angle));                      // 0x78d640, negated
    float c = (float)(-DCos((double)angle));                      // 0x78d480
    float l2 = s * s + c * c;
    double inv = 1.0 / sqrt((double)l2);
    int size = u->UnitSizeBlocks < 4 ? 4 : u->UnitSizeBlocks;     // +0x58
    unsigned mask = (unsigned)u->MoveFlags | 0x80;
    float half = UnitSize * 0.5f;
    float x = Pos[0] + (float)((double)s * inv) * half;
    float z = Pos[2] + (float)((double)c * inv) * half;
    float out[2];
    g_World->FindEmptySpace(out, x, z, angle, size, mask, true);  // 0x5e58d0
    float ox = out[0] - Pos[0], oz = out[1] - Pos[2];
    if (ox * ox + oz * oz > 100.0f) {                             // 0x7ee558
        out[0] = Pos[0];
        out[1] = Pos[2];
    }

    if (u->Proto->ClassType != 5) {
        u->Model->Slot_E0();                                      // model +0xe0
        float d = HdAtan2f((double)(out[0] - Pos[0]), (double)(out[1] - Pos[2]));
        u->Place(out[0], out[1], d);                              // +0x50
    } else {
        // The squad's member rows in the carrier's seats.
        int k = 0;
        while (k < Members.Size) {
            if (Members.Array[k].Owner != u->WorldIndex) {
                ++k;
                continue;
            }
            int ni = u->Members.Size;                             // 0x5467c0 SDArray::Add
            ArrayAdd(&u->Members);
            u->Members.Array[ni].Unit = Members.Array[k].Unit;
            u->Members.Array[ni].Owner = Members.Array[k].Owner;
            SUnit* m = WorldUnit(Members.Array[k].Unit);
            m->Parent = u->WorldIndex;
            if (Members.Array[k].Attached)
                m->Model->Slot_E0();
            int removeRow = k;
            if (si < Stored.Size - 1) {
                for (int j = Members.Size - 1; j > k; --j) {
                    if (Members.Array[j].Owner == Members.Array[k].Owner)
                        continue;
                    // A crew member of a later stored unit takes the freed seat.
                    SUnit* mj = WorldUnit(Members.Array[j].Unit);
                    if (Members.Array[j].Attached)
                        mj->Model->Slot_E0();
                    SUnit* mk = WorldUnit(Members.Array[k].Unit);
                    *reinterpret_cast<SString*>(&mj->_25c) = *reinterpret_cast<SString*>(&mk->_25c);   // +0x25c
                    mj->SetGlobalState(AnimStateIndex(mj->Anim, "vehicle"), 0);   // 0x5c7ed0, 0x5b7390
                    Members.Array[k].Unit = Members.Array[j].Unit;
                    Members.Array[k].Owner = Members.Array[j].Owner;
                    if (Members.Array[k].Attached)
                        mj->Model->AttachTo(Model, Members.Array[k].Node);   // model +0xdc
                    ++k;
                    removeRow = j;
                    break;
                }
            }
            RemoveStoredMember(removeRow);                        // 0x5be140
        }
        u->SetGlobalState(0, 0);
        float d = HdAtan2f((double)(out[0] - Pos[0]), (double)(out[1] - Pos[2]));
        u->Place(out[0], out[1], d);
        for (int i = 0; i < u->Members.Size; ++i) {
            SUnit* m = WorldUnit(u->Members.Array[i].Unit);
            int rz = WorldRand();
            int rx = WorldRand();
            float mx = (float)((double)rx * 3.0517578125e-05 + (double)(Pos[0] - 0.5f));
            float mz = (float)((double)rz * 3.0517578125e-05 + (double)(Pos[2] - 0.5f));
            float md = HdAtan2f((double)(out[0] - mx), (double)(out[1] - mz));
            m->Model->SetVisible(false, false);                   // model +0x30(0, 0)
            m->Place(mx, mz, md);
            m->SetGlobalState(1, 0);
            m->SetGlobalState(0, 1);
        }
    }

    u->Parent = -1;
    SUnit::EnvSlot9C(u, 1);                                       // +0x9c(1)
    u->IsStored = false;
    u->StoreMode = 0;
    u->SetAIGroup(AIGroup);                                       // 0x5c0c10
    if (HasReturnPos) {
        u->HasReturnPos = true;
        HasReturnPos = false;
        u->ReturnX = ReturnX;
        u->ReturnZ = ReturnZ;
    }
    _190 = false;
    u->_18c = -1;
    if (PrimaryTarget && (PrimaryTarget->Kind == 4 || PrimaryTarget->Kind == 0) &&
        *(int*)(g_World->Players[Player] + 0x08) == 1)
        SetTarget(&u->PrimaryTarget, PrimaryTarget);
    if (g_GameLogic) {
        u->_264 = g_GameLogic->GetFrame();
        g_GameLogic->DispatchLeaves(WorldIndex, u->WorldIndex);   // 0x571570
    }
    for (int i = 0; i < Stored.Size; ++i) {
        if (Stored.Array[i].Unit != unit)
            continue;
        Stored.Size--;
        if (Stored.Size - i != 0)
            memmove(&Stored.Array[i], &Stored.Array[i + 1], (Stored.Size - i) * sizeof(SUnitStored));
        memset(&Stored.Array[Stored.Size], 0, sizeof(SUnitStored));
        break;
    }
    if (u->Proto->HeroPicture >= 0)
        HandSelectionTo(u);                                       // 0x5c50c0 inline
    if (Stored.Size == 0) {
        if (!Proto->BuiltInDriver) {
            _110 = true;
            if (AIGroup != -1) {
                if (AIGroup > -1) {
                    AIGroupAt(AIGroup)->RemoveUnit(WorldIndex);   // 0x5f7990
                    if (AIGroupAt(AIGroup)->Units.Size == 0)
                        AIGroupsRemove(g_World, AIGroup);         // 0x5be040
                }
                AIGroup = -1;
            }
            HandSelectionTo(u);
            _10c = 0;
        }
        if (_2e4 > -1) {
            Wrecked = true;
            g_GameLogic->RemoveUnitFromMovementGroup(WorldIndex); // 0x579510
        }
    }
    if (Proto->ClassType == 5 && Members.Size == 0)
        Logger.g->Log(1, "asdfa");                                // HD 0x65cac0 (warning)
    return true;
}

void UnitGhostClearAll(SIUnit* unit);                            // 0x5ba5a0 (driverunit.cpp)

// PANZERS 0x5b87f0
// Get in / on a unit: a primary target of kind 9 (type 0) on it, +0xa0.
// Needs a driver; not for projectiles, untargetable units or oneself.
void SUnit::EC_Enter(int unit, int queue)
{
    PZ_M3_TRACE("SUnit::EC_Enter (0x5b87f0)");
    if (!IsTargetable(unit, true) || unit == WorldIndex)          // 0x5bb6b0
        return;
    if (WorldUnit(unit)->Proto->ClassType == 3 || ActiveDriver < 0)
        return;
    STarget* t = PzTargetNew(9);                                  // new 0x38, 0x5b27c0(9)
    tgt::I(t, tgt::kType) = 0;
    tgt::I(t, tgt::kUnit) = unit;
    SetTarget(&PrimaryTarget, t);
    SetCurrentTarget(PrimaryTarget, queue);                       // +0xa0
}

// PANZERS 0x5c6090
// Unloads the first stored unit of that store mode that gets out (+0x64);
// returns its index, -1 if none.
int SUnit::UnloadByMode(int mode)
{
    if (Stored.Size == 0)
        return -1;
    for (int i = 0; i < Stored.Size; ++i) {
        if (Stored.Array[i].Mode != mode)
            continue;
        int u = Stored.Array[i].Unit;
        if (UnloadUnit(u))                                        // +0x64
            return u;
    }
    return -1;
}

// PANZERS 0x5c2df0
// Releases the unit this one tows (+0x2d8): its crew (store mode 1) gets
// out, the towed gun goes back to "normal", down on a free spot (not for
// class 10), and its crew mans it again (+0x140 get in, or straight into it
// without a game logic / with p1); then trigger event 8 "stops towing".
void SUnit::Remove(bool p1)
{
    if (Towed < 0)
        return;
    int crew = UnloadByMode(1);                                   // 0x5c6090
    if (g_World->Units.IsLive(Towed)) {
        SUnit* t = WorldUnit(Towed);
        t->Parent = -1;
        t->_7c = false;
        UnitGhostClearAll(t);                                     // 0x5ba5a0
        t->SetBehavior(AnimStateIndex(t->Anim, "normal"));        // +0x12c, 0x5c7ed0
        t->SetOnBlockMap(false);                                  // +0x198
        if (t->Proto->ClassType != 10) {
            float out[2];
            g_World->FindEmptySpace(out, t->Pos[0], t->Pos[2], Dir, t->UnitSizeBlocks,
                                    (unsigned)t->MoveFlags | 0x80, true);   // 0x5e58d0
            t->Pos[0] = out[0];
            t->Pos[2] = out[1];
        }
        t->SetOnBlockMap(true);
        if (!g_GameLogic)
            p1 = true;
        if (crew > -1) {
            if (!p1)
                WorldUnit(crew)->EC_Enter(t->WorldIndex, 1);      // +0x140
            else
                WorldUnit(t->WorldIndex)->StoreUnit(crew, 0);     // +0x5c
        }
        if (g_GameLogic)
            g_GameLogic->DispatchStopsTowing(WorldIndex, Towed);  // 0x571750
    }
    Towed = -1;
}

} // namespace pz
