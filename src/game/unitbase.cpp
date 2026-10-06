// src/game/unitbase.cpp
// pz::SUnit: the lifted bodies of the HD unit base class (0x5b2800..0x5c6300).
// OWNER: agent U. The slots still logged are in unit.cpp.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "unit.h"
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
    unsigned s = g_World->RandomSeed * 0x343fdu + 0x269ec3u;
    g_World->RandomSeed = s;
    return (int)(s >> 16) & 0x7fff;
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
    _1cc = -1;
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
    ArrayFree(&_308);
    FreeSString((SString*)&_25c);                             // +0x25c SString (A: sub-state)
    ArrayFree(&ChildUnits);
    free(GhostFrames.Array);                                  // 0x5b32f0
    GhostFrames.Array = nullptr;
    ArrayFree(&_1c0);
    ArrayFree(&_1b4);
    ArrayFree(&_1a8);
    ArrayFree(&_19c);
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
            g_Pixie->Slot_14();                               // pixie +0x34 (stop effect; slot not named)
    StopEffects();                                            // 0x5c3030
    Remove(false);                                            // +0x70
    if (AIGroup != -1) {
        // HD 0x5f7990 / 0x5be040: leave the AI group (World+0x4f4).
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
// The AI group bookkeeping (World+0x4f4, 0x5f7990 / 0x5d9560 / 0x5be040)
// is not lifted: no menu unit is in an AI group.
void SUnit::SetAIGroup(int group)
{
    if (AIGroup == group)
        return;
    AIGroup = group;
    if (group >= 0) {
        STUB_LOG("SUnit::SetAIGroup (0x5c0c10) AI group membership");
    }
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
    XP = def->XP;
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
    Hook20(def->Slots[0]);                                    // +0x20(&def->Slots)
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
    SetGlobalState(GlobalState, 0);                           // 0x5b7390
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
        STUB_LOG("SUnit::SetPosition (0x5c1980) towed unit");
    }
    if (_7c && (_104 & 1) != 0 && g_World->Units.IsLive(Parent)) {
        SUnit* parent = WorldUnit(Parent);
        if ((parent->_104 & 1) == 0)
            parent->Remove(false);
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
    (void)vp;
    if (Anim)
        Anim->Slot_0C();                                      // anim +0x0c
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
// Lifted control flow. Parts that need other agents go through their hooks
// (STarget::Refresh, the driver refresh); the driver effects (0x5bd910),
// the unit effects timer (0x5bdcd0) and the per-player sighting (0x562b10)
// are not lifted yet.
void SUnit::ServerRefresh(int frame)
{
    if (LastRefreshFrame == frame)
        return;
    LastRefreshFrame = frame;
    if (Frozen)
        return;
    if (--EffectTimer == 0) {
        // HD 0x5c2200: pixie +0x60(effect, 1) for each static effect.
    }
    if (CurrentTarget && RefreshDriver >= 0) {
        SIDriver* d = GetDriver(RefreshDriver);
        if (d) {
            d->Refresh();                                     // +0x14
            // HD: the refresh driver is dropped when its +0xc0 target is gone.
        }
    }
    if (Wrecked) {                                            // HD +0x150 byte
        Slot_30();                                            // dead unit refresh
        return;
    }
    if (_1a8.Size != 0 && PrimaryTarget == nullptr) {
        STUB_LOG("SUnit::ServerRefresh (0x5bee90) queued orders (0x5b36e0 / 0x5b95a0)");
    }
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
            STUB_LOG("SUnit::ServerRefresh (0x5bee90) +0xcc EC_AttackMove");
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
                STUB_LOG("SUnit::ServerRefresh (0x5bee90) +0xcc EC_AttackMove");
            } else {
                ClearTargets();
            }
        } else {
            ReleaseTarget(&CurrentTarget);
            AI_Heartbeat();                                   // +0x190
        }
    }
    if (_6d) {
        STUB_LOG("SUnit::ServerRefresh (0x5bee90) per-player sighting (0x562b10)");
    }
    RefreshMisc();                                            // +0x38
    if (RemoveMe)
        g_World->RemoveUnit(WorldIndex);                      // 0x5f8060
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
        STUB_LOG("SUnit::SetCurrentTarget (0x5c0d10) 0x5bee10");
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
                GetDriver(ActiveDriver)->RefreshTarget((int)(size_t)CurrentTarget);   // driver +0x08
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
                d->RefreshTarget((int)(size_t)CurrentTarget); // +0x08
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
// The reachability test 0x5bb6b0 is not lifted (always true here).
void SUnit::EC_Follow(int unit, int p2)
{
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
    if (!d)
        return;
    PzDriverReset(d, true);                                   // 0x55bf50(1)
    // HD: a driver still braking (+0xd4 ticks, speed +0xc8 against the
    // prototype speed +0x08) gets a stop target at the unit position
    // (+0x0c); that needs the driver fields (P).
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
    StateChangeTime = (int)((float)ticks * 1.0f);             // DAT_007f83dc (not verified)
}

// PANZERS 0x546ac0
void SUnit::RestoreBehavior()
{
}

// ---------------------------------------------------------------------------
// Storage

// PANZERS 0x5b7040
// Whether `unit` may get in: storage capacity, class, side and the hero seat.
static bool CanStore(SUnit* self, int unit)
{
    SPUnit* p = self->Proto;
    if (p->StorageCapacity == 0)
        return false;
    SUnit* u = WorldUnit(unit);
    int uct = u->Proto->ClassType;
    if (uct != 5 && uct != 0)
        return false;
    if (p->ClassType != 9 && self->Player != u->Player && !self->_110)
        return false;
    if (uct == 5 && p->StorageType != 2 && p->StorageType != 0)
        return false;
    if ((uct == 0 || uct == 0xb) && p->StorageType != 2 && p->StorageType != 1)
        return false;
    if (p->ClassType == 9) {
        STUB_LOG("SUnit::StoreUnit (0x5b7040) buildings");
        return false;
    }
    if (self->IsStored)
        return false;
    int cap = p->StorageCapacity;
    if (p->OnlyCrew) {
        bool hero = u->Proto->HeroPicture >= 0;
        bool storedHero = false;
        for (int i = 0; i < self->Stored.Size; ++i)
            if (WorldUnit(self->Stored.Array[i].Unit)->Proto->HeroPicture >= 0)
                storedHero = true;
        if (hero || storedHero)
            cap++;
    }
    return self->Stored.Size < cap;
}

// PANZERS 0x5c30d0
// A squad gets in a vehicle: each member takes a seat (driver, gunner,
// passenger) and the squad becomes the vehicle's crew. Lifted for the
// vehicle case; the member attach to the seat nodes (model +0xdc) and the
// building case are not (they need model slots not in imodel.h).
bool SUnit::StoreUnit(int unit, int mode)
{
    SUnit* u = WorldUnit(unit);
    if (!CanStore(this, unit))
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
    if (u->_2f4) {
        u->_2f4 = false;
        _2f4 = true;
        _2f8 = u->_2f8;
        _2fc = u->_2fc;
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
                seat->Attached = false;
                STUB_LOG("SUnit::StoreUnit (0x5c30d0) kneel seat");
            } else {
                static const char* const kSeat[] = { nullptr, "_driver", "_sitgun", "_standgun", "_sitpass",
                                                     "_standpass", "_kneelgun", "_mg", "_ground_gun",
                                                     "_ground_gun2", nullptr, "_leftboat", "_rightboat" };
                if (kind < 0 || kind > 12 || !kSeat[kind])
                    Logger.g->Panic("SUnit::StoreUnit - Unknown UNIT_IN_VEHICLE_ANIM_ substring");
                *(SString*)&member->_25c = kSeat[kind];
                member->InVehicleAnim = 1;
                // HD: member model +0xdc attaches it to this unit's model at the seat node.
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

// PANZERS 0x5c2df0
// Releases the unit this one tows (+0x2d8); not reached on the menu.
void SUnit::Remove(bool p1)
{
    (void)p1;
    if (Towed < 0)
        return;
    STUB_LOG("SUnit::Remove (0x5c2df0) release the towed unit");
    if (g_World->Units.IsLive(Towed)) {
        SUnit* t = WorldUnit(Towed);
        t->Parent = -1;
        t->_7c = false;
    }
    Towed = -1;
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
    float xp = (float)XP;
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

} // namespace pz
