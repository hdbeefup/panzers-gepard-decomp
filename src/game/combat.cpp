// src/game/combat.cpp
// Combat outside the unit slot bodies: SUnit::TakeDamage 0x5c4080 and its
// helpers (crew damage 0x5c3dd0, healing 0x5c5050, EC_Die 0x5b8b10), XP
// (SWorld::IncreaseUnitXP 0x5ec840), the kill / loss statistics (0x5eca50,
// 0x571090) and the trigger events "attacked" (0x570e40) and "unit dies"
// (0x571090). OWNER: agent M3-C sub-agent C2 (damage / death).
//
// Campaign statistics (SPanzersCampaign has no fields for them yet; raw
// offsets, HD 0x929a0c):
//   +0x0f8  int damage balance of the local player (dealt - taken), also
//           written to the replay header (F)
//   +0x17c  int Kills[12][0x36]   per player, [0] total, [category] by kind
//   +0x1ac  int Losses[12][0x36]  (0x571090; overlaps Kills by 0x30 bytes)
//   +0x20c  int XP gained per player (IncreaseUnitXP adds it twice)
//   +0xb60  int damage balance (second copy)
// SGameLogic fields read here (raw offsets, gamelogic.h keeps them in
// padding): +0x014 frame of the last combat music, +0x2d8 byte "one-shot
// kill" cheat, +0x2da byte "invulnerable local player" cheat.

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "unit.h"
#include "combat_math.h"
#include "campaign.h"
#include "gamelogic.h"
#include "gunner.h"
#include "iunitanim.h"
#include "drivertypes.h"
#include "driverunit.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"
#include "trigger.h"
#include "logger.h"
#include "m3common.h"
#include "stub_log.h"

namespace pz {

namespace {

int& CampaignInt(int off)
{
    return *(int*)((unsigned char*)g_Campaign + off);
}

unsigned char& GameLogicByte(int off)
{
    return *((unsigned char*)g_GameLogic + off);
}

int& GameLogicInt(int off)
{
    return *(int*)((unsigned char*)g_GameLogic + off);
}

int PlayerTeam(int player)                                        // World+0x17c + player * 0x48
{
    return *(int*)(g_World->Players[player] + 0x0c);
}

int PlayerType(int player)                                        // World+0x178 + player * 0x48
{
    return *(int*)(g_World->Players[player] + 0x08);
}

// HD 0x549ab0 inline: same team (team 0 = alone).
bool SameSide(int a, int b)
{
    int team = PlayerTeam(a);
    if (team != 0)
        return team == PlayerTeam(b);
    return a == b;
}

// HD 0x594cd0: SMulti (DAT_008f1a74) in coop mode (+19000 == 3). The
// recompile has no SMulti (single player only).
bool IsCoopMultiplayer()
{
    return false;
}

bool UnitLive(int index)
{
    return g_World->Units.IsLive(index);
}

int NextLiveUnit(int i)                                           // SHeapTRB::NextLive 0x56b3b0
{
    const SUnitHeap& h = g_World->Units;
    for (++i; i < h.Size; ++i)
        if (h.Array[i].Next == kHeapLive)
            return i;
    return -1;
}

SUnitStored& StoredAt(SUnit* u, int i)                            // SDArray<SUnitStored>::operator[] 0x546450
{
    if (i < 0 || i >= u->Stored.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SUnitStored", i);
    return u->Stored.Array[i];
}

SUnitMember& MemberAt(SUnit* u, int i)                            // SDArray<SUnitMember>::operator[] 0x5991e0
{
    if (i < 0 || i >= u->Members.Size)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SUnitMember", i);
    return u->Members.Array[i];
}

const SDamageFactors& DamageFactors()                             // SUnitRegistry +0x0c
{
    return *reinterpret_cast<const SDamageFactors*>(&g_UnitRegistry->DamageBulletToUnarmoured);
}

} // namespace

// PANZERS 0x56d6d0
// The statistics category of a unit (campaign kill / loss tables): by
// prototype unit type for the classes 0, 6, 9, 0xa and 0xb; 0 = not counted.
int SGameLogic::UnitStatCategory(SUnit* u)
{
    int cls = u->Proto->ClassType;
    if (cls != 0 && cls != 0xb && cls != 6 && cls != 9 && cls != 10)
        return 0;
    switch (u->Proto->UnitType) {
    case 0: case 0xc:
        return 2;
    case 1:
        return 3;
    case 2:
        return 5;
    case 3: case 6: case 7:
        return 8;
    case 4:
        return 10;
    case 8:
        return 6;
    case 9:
        return 4;
    case 10:
        return 1;
    case 0xb:
        return 7;
    case 0xd: case 0x1a:
        return 0xb;
    case 0xe: case 0xf: case 0x10: case 0x11: case 0x12: case 0x13: case 0x14:
    case 0x15: case 0x16: case 0x17: case 0x18:
        return cls != 5 ? 9 : 0;
    default:
        return 0;
    }
}

// PANZERS 0x5b9e40
// The "one-shot kill" cheat of the local player (+0x2d8), off while paused.
bool SGameLogic::IsKillCheat()
{
    return GameLogicByte(0x2d8) != 0 && !IsPaused();
}

// PANZERS 0x5ec840
// The attacker earns XP for damage done to an enemy (not to allies or to
// units with +0x110 set): its +0x8c gets (victim, amount, 0), and in a
// campaign the attacker's player XP counter (+0x20c) grows (twice, as in HD).
void SWorld::IncreaseUnitXP(int unit, int victim, float amount)
{
    PZ_M3_TRACE("SWorld::IncreaseUnitXP (0x5ec840)");
    if (Units.IsLive(unit)) {
        if (!Units.IsLive(victim))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", victim);
        int victimPlayer = Units.Array[victim].Unit->Player;
        int unitPlayer = Units.Array[unit].Unit->Player;
        if (!SameSide(unitPlayer, victimPlayer)) {
            if (!Units.Array[victim].Unit->_110) {
                WorldUnit(unit)->AddXP(victim, amount, 0);        // +0x8c
                if (SGameLogic::UnitStatCategory(WorldUnit(victim)) == 0)
                    return;
                if (!g_Campaign)
                    return;
                if (g_GameLogic && g_GameLogic->IsPaused())
                    return;
                int off = WorldUnit(unit)->Player * 0xd8 + 0x20c;
                CampaignInt(off) = (int)((float)CampaignInt(off) + amount);
                off = WorldUnit(unit)->Player * 0xd8 + 0x20c;
                CampaignInt(off) = (int)((float)CampaignInt(off) + amount);
                return;
            }
        }
    }
    if (unit == -1)
        DrvWarn("SWorld::IncreaseUnitXP nincs attacker ");            // HD 0x65cac0
}

// PANZERS 0x5eca50
// A kill: the attacker's player counts it by the victim's category and in
// total (campaign +0x17c), unless the two are on the same side.
void SWorld::RecordKill(int attacker, int victim)
{
    PZ_M3_TRACE("SWorld::RecordKill (0x5eca50)");
    if (!Units.IsLive(attacker))
        return;
    if (!Units.IsLive(victim))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", victim);
    int cat = SGameLogic::UnitStatCategory(Units.Array[victim].Unit);
    if (cat == 0)
        return;
    int victimPlayer = WorldUnit(victim)->Player;
    int attackerPlayer = WorldUnit(attacker)->Player;
    if (SameSide(attackerPlayer, victimPlayer))
        return;
    if (!g_Campaign)
        return;
    if (g_GameLogic && g_GameLogic->IsPaused())
        return;
    ++CampaignInt(0x17c + (WorldUnit(attacker)->Player * 0x36 + cat) * 4);
    ++CampaignInt(0x17c + WorldUnit(attacker)->Player * 0xd8);
}

// PANZERS 0x570e40
// Trigger event 4 "attacked": every trigger of that event with Unit = the
// attacked unit (or its carrier) and Unit2 = the attacker.
void SGameLogic::DispatchAttacked(int unit, int attacker)
{
    PZ_M3_TRACE("SGameLogic::DispatchAttacked (0x570e40)");
    SRunningTrigger rt;
    memset(&rt, 0, sizeof(rt));                                   // memset 0x34
    SHdArray<STrigger>* t = &g_World->Triggers;
    for (int i = 0; i < t->Size; ++i) {
        if (t->Array[i].Event.Type != TE_ATTACKED)
            continue;
        rt.Unit = unit;                                           // +0x0c
        rt.Unit2 = attacker;                                      // +0x10
        rt.Trigger = i;
        CheckConditions(&rt);                                     // 0x580600
        t = &g_World->Triggers;
    }
    free(rt.Found.Units);
}

// PANZERS 0x571090
// A unit died: the campaign loss statistics of its player (+0x1ac, unless
// paused or +0x110), then trigger event 1 "unit dies" with Unit = the unit.
void SGameLogic::DispatchUnitDies(int unit)
{
    PZ_M3_TRACE("SGameLogic::DispatchUnitDies (0x571090)");
    if (g_Campaign) {
        if (!UnitLive(unit))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", unit);
        int cat = UnitStatCategory(g_World->Units.Array[unit].Unit);
        if (cat != 0) {
            if (!g_World->Units.Array[unit].Unit->_110 &&
                (!g_GameLogic || (!g_GameLogic->Flag288 && !g_GameLogic->Flag2b8))) {
                ++CampaignInt(0x1ac + (WorldUnit(unit)->Player * 0x36 + cat) * 4);
                ++CampaignInt(0x1ac + WorldUnit(unit)->Player * 0xd8);
            }
        }
    }
    SRunningTrigger rt;
    memset(&rt, 0, sizeof(rt));
    SHdArray<STrigger>* t = &g_World->Triggers;
    for (int i = 0; i < t->Size; ++i) {
        if (t->Array[i].Event.Type != TE_UNIT_DIES)
            continue;
        rt.Unit = unit;
        rt.Unit2 = -1;
        rt.Trigger = i;
        CheckConditions(&rt);                                     // 0x580600
        t = &g_World->Triggers;
    }
    free(rt.Found.Units);
}

// PANZERS 0x571570
// Trigger event 6 "leaves": Unit = the carrier, Unit2 = the unit that got out.
void SGameLogic::DispatchLeaves(int carrier, int unit)
{
    PZ_M3_TRACE("SGameLogic::DispatchLeaves (0x571570)");
    SRunningTrigger rt;
    memset(&rt, 0, sizeof(rt));
    SHdArray<STrigger>* t = &g_World->Triggers;
    for (int i = 0; i < t->Size; ++i) {
        if (t->Array[i].Event.Type != TE_LEAVES_BUILDING)
            continue;
        rt.Unit = carrier;
        rt.Unit2 = unit;
        rt.Trigger = i;
        CheckConditions(&rt);                                     // 0x580600
        t = &g_World->Triggers;
    }
    free(rt.Found.Units);
}

// PANZERS 0x571750
// Trigger event 8 "stops towing": Unit = the tower, Unit2 = the towed unit.
void SGameLogic::DispatchStopsTowing(int tower, int towed)
{
    PZ_M3_TRACE("SGameLogic::DispatchStopsTowing (0x571750)");
    SRunningTrigger rt;
    memset(&rt, 0, sizeof(rt));
    SHdArray<STrigger>* t = &g_World->Triggers;
    for (int i = 0; i < t->Size; ++i) {
        if (t->Array[i].Event.Type != TE_STOPS_TOWING)
            continue;
        rt.Unit = tower;
        rt.Unit2 = towed;
        rt.Trigger = i;
        CheckConditions(&rt);                                     // 0x580600
        t = &g_World->Triggers;
    }
    free(rt.Found.Units);
}

// PANZERS 0x570f30
// The minimap "under attack" blink: own units red (0xc0ff0000), allied
// units yellow (0xc0ffff00), at the unit's minimap position.
void SGameLogic::PingAttackedUnit(int unit)
{
    PZ_M3_TRACE("SGameLogic::PingAttackedUnit (0x570f30)");
    if (MinimapFrame < 0)                                         // +0x17c
        return;
    SUnit* u = WorldUnit(unit);
    int local = g_World->LocalPlayer;
    if (u->Player != local && !SameSide(u->Player, local))
        return;
    // HD: board (0x8f1c60) +0xb4 (x, -y, color) with
    // x = ((pos.x - 0.5) / (TerrainW - 0x60) - 0.5) * minimap +0x18c [0],
    // y = ((pos.z - 0.5) / (TerrainH - 0x60) - 0.5) * minimap +0x18c [1]
    // (constants 0x7f7f90, 0x7ea760). The minimap is agent H's (hud /
    // minimap.cpp) and the SIBoard of the recompile has no +0xb4.
    STUB_LOG("SGameLogic::PingAttackedUnit (0x570f30) board +0xb4 minimap blink");
}

// PANZERS 0x5c3dd0
// Damage to the crew of a vehicle or the members of a gun crew (+0x178).
// last: the whole damage goes to the last member (+0x94 TakeDamage);
// otherwise members are drawn at random (world LCG, up to Members.Size
// draws) until one keeps more than 0.7 HP after damage / its hit points.
void SUnit::DamageMembers(bool last, float damage, int weaponType, int attacker, float x, float y,
                          float z, int hitMode)
{
    PZ_M3_TRACE("SUnit::DamageMembers (0x5c3dd0)");
    if (Invulnerable)
        return;
    if (GameLogicByte(0x2da) && !g_GameLogic->IsPaused() && g_World->LocalPlayer == Player)
        return;
    if (Parent > -1) {
        if (!UnitLive(Parent))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", Parent);
        if (g_World->Units.Array[Parent].Unit->Invulnerable)
            return;
    }
    if (Members.Size <= 0)
        return;
    if (last) {
        SUnit* m = WorldUnit(MemberAt(this, Members.Size - 1).Unit);
        m->TakeDamage(damage, weaponType, attacker, x, y, z, hitMode);
        return;
    }
    float hpLoss = damage / WorldUnit(MemberAt(this, 0).Unit)->GetHitPoints();   // fdivr, fstp dword
    for (int k = 0; k < Members.Size; ++k) {
        int r = WorldRand();                                      // inline LCG on World+0x7518
        int i = (int)((double)r * 3.0517578125e-05 * (double)Members.Size);   // 0x7f4540, cvttsd2si
        int mi = MemberAt(this, i).Unit;
        if (!UnitLive(mi))
            Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", mi);
        SUnit* m = g_World->Units.Array[mi].Unit;
        if (m->HP - hpLoss > 0.7f) {                              // 0x7fb69c
            m->HP = m->HP - hpLoss;
            return;
        }
    }
}

// PANZERS 0x5c5050
// Healing (medics, +0x98): HP += amount / hit points * 4, at most 1.
void SUnit::Heal(float amount)
{
    if (1.0f > HP) {
        float hpp = GetHitPoints();                               // fstp dword
        float h = amount / hpp * 4.0f + HP;                       // 0x7f4588
        HP = h;
        if (h > 1.0f)
            HP = 1.0f;
    }
}

// PANZERS 0x5b8b10
// Death: a unit riding outside on a vehicle (+0x7c) gets off first (carrier
// +0x70 Remove(false)); then +0x150 (wreck), out of its movement group, and
// the "unit dies" statistics / trigger event.
void SUnit::EC_Die()
{
    PZ_M3_TRACE("SUnit::EC_Die (0x5b8b10)");
    if (Invulnerable)
        return;
    if (_7c && UnitLive(Parent))
        WorldUnit(Parent)->Remove(false);
    if (Proto->ClassType == 4)
        Logger.g->Panic("SUnit::EC_Die() - PUnit->ClassType == CLASSTYPE_CHILDUNIT");
    Wrecked = true;
    g_GameLogic->RemoveUnitFromMovementGroup(WorldIndex);         // 0x579510 (HD passes the unit)
    g_GameLogic->DispatchUnitDies(WorldIndex);                    // 0x571090
}

// PANZERS 0x5c4080
// A hit. damage is the weapon's damage, weaponType the gunner's (0 bullet,
// 1 AT, 2 HE, 3 fire), attacker a world index (-1), (x, y, z) where the shot
// came from, hitMode 0 directional armour, 1 top armour, 2 no armour.
void SUnit::TakeDamage(float damage, int weaponType, int attacker, float x, float y, float z,
                       int hitMode)
{
    PZ_M3_TRACE("SUnit::TakeDamage (0x5c4080)");
    SWorld* w = g_World;
    if (Proto->ClassType == 4)
        Logger.g->Panic("SUnit::TakeDamage ChildUnit");
    if (Invulnerable)                                             // +0x111
        return;
    if (Proto->ClassType == 5)                                    // squads: the members take it
        return;
    if (GameLogicByte(0x2da) && !g_GameLogic->IsPaused() && w->LocalPlayer == Player)
        return;
    if (Parent > -1 && WorldUnit(Parent)->Invulnerable)
        return;
    if (Wrecked)                                                  // +0x150
        return;
    if (damage == 0.0f)
        return;
    if (UnitLive(attacker) && w->LocalPlayer == WorldUnit(attacker)->Player &&
        g_GameLogic->IsKillCheat())                               // 0x5b9e40
        HP = 0.0f;

    // A hero inside (stored mode 2) or a hero unit halves the damage.
    bool hero = false;
    for (int i = 0; i < Stored.Size; ++i)
        if (StoredAt(this, i).Mode == 2)
            hero = true;
    if (Proto->HeroPicture > 0 || hero)
        damage = damage * 0.5f;                                   // 0x7f453c
    if (g_Campaign && (!g_GameLogic || !g_GameLogic->IsPaused())) {
        if (g_Campaign->Difficulty == 0 && !g_Campaign->IsMultiMode() && !IsCoopMultiplayer() &&
            Player == w->LocalPlayer)
            damage = damage * 0.75f;                              // 0x7f2fcc (easy)
        if (g_Campaign->Difficulty == 2 && !g_Campaign->IsMultiMode() && !IsCoopMultiplayer() &&
            Player == w->LocalPlayer)
            damage = damage * 1.25f;                              // 0x7fd6f4 (hard)
    }
    if (_130 != 0)
        damage = damage * 0.5f;

    damage = DamageByWeapon(DamageFactors(), damage, weaponType, Proto->ArmourType,
                            Proto->ClassType, GlobalState, Proto->Thermostat, &_118, &HP);

    float oldHP = HP;
    float norm[3];
    int side = DamageSide(Pos, Dir, x, y, z, hitMode, norm);
    float* armour = nullptr;                                      // [ebp-0x18]
    const float* protoArmour = nullptr;                           // [ebp-0x1c]
    if (side >= 0) {
        armour = &Armor[0] + side;                                // +0x11c.. (+0x12c top)
        protoArmour = &Proto->FrontArmor + side;                  // +0xa0..
    }

    if (Proto->ClassType == 0xb) {                                // gun crew: the crew takes it
        DamageMembers(true, side == 0 ? damage * 0.3f : damage, weaponType, attacker, x, y, z,
                      hitMode);                                   // 0x7f83d8
        if (weaponType == 0 || weaponType == 3)
            return;
    }

    bool hurt = false;                                            // [ebp-0x11]
    if (armour == nullptr || *protoArmour == 0.0f) {
        HP = HP - damage / GetHitPoints();                        // x87 PC24
        if (UnitLive(attacker)) {
            SUnit* a = WorldUnit(attacker);
            if (!SameSide(a->Player, Player)) {                   // 0x549ab0(attacker player, own player)
                if (w->LocalPlayer == WorldUnit(attacker)->Player && g_Campaign) {
                    CampaignInt(0xf8) = (int)((float)CampaignInt(0xf8) + damage);
                    CampaignInt(0xb60) = (int)((float)CampaignInt(0xb60) + damage);
                }
                if (w->LocalPlayer == Player && g_Campaign) {
                    CampaignInt(0xf8) = (int)((float)CampaignInt(0xf8) - damage);
                    CampaignInt(0xb60) = (int)((float)CampaignInt(0xb60) - damage);
                }
            }
        }
        hurt = true;
        if (!FirstBlood && Proto->ClassType != 9) {               // +0x69
            w->IncreaseUnitXP(WorldIndex, WorldIndex, 50.0f);     // HD passes its own index twice
            FirstBlood = true;
        }
    } else {
        int part = 0;
        float absorb = DamageAbsorb(*armour, *protoArmour);
        if (damage > absorb) {
            part = (int)(damage - absorb);                        // cvttss2si
            HP = HP - (float)part / GetHitPoints();               // x87 PC24
            if (UnitLive(attacker)) {
                SUnit* a = WorldUnit(attacker);
                if (!SameSide(a->Player, Player)) {
                    if (w->LocalPlayer == WorldUnit(attacker)->Player && g_Campaign) {
                        CampaignInt(0xf8) += part;
                        CampaignInt(0xb60) += part;
                    }
                    if (w->LocalPlayer == Player && g_Campaign) {
                        CampaignInt(0xf8) -= part;
                        CampaignInt(0xb60) -= part;
                    }
                }
            }
            hurt = true;
        }
        DamageWearArmour(damage, part, armour, *protoArmour);
    }

    if (Proto->ClassType == 0 && hurt) {
        // The crew of a vehicle takes rand(10) + damage / 5 (world LCG draw).
        int r = HdRandInt(10);                                    // 0x555a00(10)
        DamageMembers(false, (float)r + damage * 0.2f, weaponType, attacker, x, y, z, hitMode);   // 0x7f1b4c
    }

    // A unit fired from inside a vehicle: the vehicle earns the XP.
    if (UnitLive(attacker) && WorldUnit(attacker)->Parent > -1)
        attacker = WorldUnit(attacker)->Parent;
    if (UnitLive(attacker) && Proto->ClassType != 9) {
        w->IncreaseUnitXP(attacker, WorldIndex, damage);
        int a = attacker;
        if (!WorldUnit(a)->FirstShot) {                           // +0x6a
            w->IncreaseUnitXP(a, WorldIndex, 25.0f);
            WorldUnit(a)->FirstShot = true;
        }
    }

    if (1.0f / Proto->HP > HP) {                                  // 0x7f1b58, prototype +0x98
        w->RecordKill(attacker, WorldIndex);                      // 0x5eca50
        switch (weaponType) {
        case 0:
            break;
        case 1:
        case 2:
            _151[0] = 1;                                          // +0x151 died by a shell
            break;
        case 3:
            _151[1] = 1;                                          // +0x152 died by fire
            break;
        default:
            _151[0] = 0;
            _151[1] = 0;
            break;
        }
        HP = 0.0f;
        EC_Die();                                                 // +0x124
        if (Parent < 0)
            return;
        if (WorldUnit(Parent)->Members.Size != 1)                 // +0x17c
            return;
        if (Proto->ClassType != 6)
            return;
        w->UnitSpeech(WorldIndex, 0xd, false);                    // 0x5fff20 "EnemyDeath"
        return;
    }
    if (oldHP > 0.3f && 0.3f > HP)                                // 0x7f83d8
        w->UnitSpeech(WorldIndex, 7, false);                      // "LowHealth"
    if (0.3f > HP && Proto->ClassType == 9 && PlayerType(Player) == 1)
        UnloadAll();                                              // +0x68 (buildings: everybody out)
    if (Proto->Thermostat > 0.0f && _118 >= g_UnitRegistry->ThermoOut * Proto->Thermostat &&
        Stored.Size > 0) {
        Unload(-1);                                               // +0x144(-1) unload everybody
        for (int i = NextLiveUnit(-1); i >= 0; i = NextLiveUnit(i))   // 0x56b3b0
            w->UnitStored(WorldIndex, -1);                        // 0x5ef760 (HD passes the unit, not i)
    }
    if (0.0f >= damage)
        return;

    if (Anim != nullptr && weaponType != 3) {
        bool skip = false;
        if (attacker > -1 && WorldUnit(attacker)->Proto->ClassType == 5 &&
            WorldUnit(attacker)->GetGunner(0)->GetPGunner()->DirectDamage && weaponType == 1)
            skip = true;
        if (!skip) {
            float k = damage / GetHitPoints() * 20.0f;            // x87 PC24, 0x7f35d8
            Anim->AddBodyKick(norm[0] * k, norm[1] * k, norm[2] * k);   // +0x14
        }
    }
    g_GameLogic->PingAttackedUnit(WorldIndex);                    // 0x570f30
    int frame = g_GameLogic->GetFrame();
    if (Parent > -1) {
        SUnit* p = WorldUnit(Parent);
        if (g_GameLogic->GetFrame() - p->_60 > 100) {
            w->UnitSpeech(WorldIndex, weaponType == 3 ? 9 : 0xf, false);   // "Flamethrower" / "UnderAttack"
            g_GameLogic->DispatchAttacked(Parent, attacker);      // 0x570e40
        }
        WorldUnit(Parent)->_60 = g_GameLogic->GetFrame();         // +0x60 last attacked frame
    } else {
        if (g_GameLogic->GetFrame() - _60 > 100) {
            w->UnitSpeech(WorldIndex, weaponType == 3 ? 9 : 0xf, false);
            g_GameLogic->DispatchAttacked(WorldIndex, attacker);
        }
        _60 = g_GameLogic->GetFrame();
    }
    (void)frame;
    SUnit* target = Parent > -1 ? WorldUnit(Parent) : this;
    target->OnAttackedBy(attacker);                               // +0x194

    // Combat music when an own unit is shot at by an enemy vehicle.
    if (UnitLive(attacker) && Player == w->LocalPlayer) {
        SPUnit* ap = WorldUnit(attacker)->Proto;
        if (ap->ClassType == 0 && (WorldUnit(attacker)->Proto->UnitType == 0 ||
                                   WorldUnit(attacker)->Proto->UnitType == 9 ||
                                   WorldUnit(attacker)->Proto->UnitType == 1 ||
                                   WorldUnit(attacker)->Proto->UnitType == 2)) {
            if (g_GameLogic->GetFrame() - GameLogicInt(0x14) > 1200) {
                char name[64];
                int n = ((rand() * 6) >> 15) + 1;                 // CRT rand 0x78c846: audio only
                _snprintf(name, sizeof(name) - 1, "music/war_%02d.mp3", n);   // 0x52da80, 0x7fd1f4
                name[sizeof(name) - 1] = 0;
                // HD: Concert (0x8f1c5c) +0x6c, +0x70(name), +0x78(0): the
                // Panzers concert slots are not in the recompile's SIConcert.
                STUB_LOG("SUnit::TakeDamage (0x5c4080) combat music (Panzers concert +0x6c/+0x70/+0x78)");
            }
            GameLogicInt(0x14) = g_GameLogic->GetFrame();
        }
    }
}

// HD 0x5b6040 helpers: the first gunner of a unit (SDArray<SGunner*> [0],
// panics when empty) and the parent's member rows that belong to a unit.
static SGunner* FirstGunner(SUnit* u)
{
    if (u->Gunners.Size <= 0)
        Logger.g->Panic("SDArray<%s>::operator[]: invalid index (%d)", "SGunner*", 0);
    return u->Gunners.Array[0];
}

// PANZERS 0x5b6040
// How many shots of the attacker this unit takes before it dies (1..100):
// the attacker's damage (its first gunner, or the sum over a squad's / gun
// crew's members), scaled as in TakeDamage, against the armour side facing
// `from` (worn down shot by shot) and the HP pool (squads: the members'
// HP). Fire against armour counts the shots until the thermostat. 100 when
// the attacker has no weapon. Used by the target scoring (FindTarget).
int SUnit::ShotsToKill(const float* from, int attacker)
{
    SWorld* w = g_World;
    if (!UnitLive(attacker))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", attacker);
    SUnit* a = w->Units.Array[attacker].Unit;
    if (a->MainGunner == -1)
        return 100;
    float dmg = 0.0f;                                             // [ebp+0xc]
    int wt = 0;                                                   // [ebp-0x10]
    bool found = false;
    if (a->Gunners.Size > 0 && a->Proto->ClassType != 5) {
        dmg = FirstGunner(a)->GetPGunner()->Damage;               // +0x2c, SPGunner +0x58
        wt = FirstGunner(a)->GetWeaponType();                     // 0x584240
        found = true;
    } else if (a->Members.Size > 0 && WorldUnit(MemberAt(a, 0).Unit)->MainGunner != -1) {
        wt = FirstGunner(WorldUnit(MemberAt(a, 0).Unit))->GetWeaponType();
        found = true;
    } else if (a->Parent > -1) {
        for (int k = 0;; ++k) {                                   // the parent's rows of this unit
            SUnit* p = WorldUnit(a->Parent);
            if (k >= p->Members.Size)
                break;
            if (MemberAt(p, k).Owner == a->WorldIndex) {
                wt = FirstGunner(WorldUnit(MemberAt(WorldUnit(a->Parent), k).Unit))->GetWeaponType();
                break;
            }
        }
    }
    (void)found;
    int acls = a->Proto->ClassType;
    if (acls == 5 || acls == 9) {
        if (a->Members.Size > 0 && WorldUnit(MemberAt(a, 0).Unit)->MainGunner != -1) {
            for (int k = 0; k < a->Members.Size; ++k)
                dmg = FirstGunner(WorldUnit(MemberAt(a, k).Unit))->GetPGunner()->Damage + dmg;
        } else if (a->Parent > -1) {
            for (int k = 0;; ++k) {
                SUnit* p = WorldUnit(a->Parent);
                if (k >= p->Members.Size)
                    break;
                if (MemberAt(p, k).Owner == a->WorldIndex)
                    dmg = FirstGunner(WorldUnit(MemberAt(WorldUnit(a->Parent), k).Unit))->GetPGunner()->Damage +
                          dmg;
            }
        }
    }

    // The scaling of TakeDamage 0x5c4080 (order: +0x130 first here).
    if (_130 != 0)
        dmg = dmg * 0.5f;
    bool hero = false;
    for (int i = 0; i < Stored.Size; ++i)
        if (StoredAt(this, i).Mode == 2)
            hero = true;
    if (Proto->HeroPicture > 0 || hero)
        dmg = dmg * 0.5f;
    if (g_Campaign && (!g_GameLogic || !g_GameLogic->IsPaused())) {
        if (g_Campaign->Difficulty == 0 && !g_Campaign->IsMultiMode() && !IsCoopMultiplayer() &&
            Player == w->LocalPlayer)
            dmg = dmg * 0.75f;
        if (g_Campaign->Difficulty == 2 && !g_Campaign->IsMultiMode() && !IsCoopMultiplayer() &&
            Player == w->LocalPlayer)
            dmg = dmg * 1.25f;
    }
    const SDamageFactors& f = DamageFactors();
    if (wt == 3 && Proto->ArmourType == 2 && Proto->ClassType != 0xb) {
        // Fire against armour: shots until the heat passes the thermostat.
        float step = f.FireToArmoured * dmg;
        float heat = _118 + step;
        float n = 1.0f;
        if (!(heat >= Proto->Thermostat)) {
            for (;;) {
                if (n > 99.0f)                                    // 0x7fd708
                    break;
                n = n + 1.0f;
                heat = heat + step;
                if (!(heat < Proto->Thermostat))
                    break;
            }
        }
        return (int)n;
    }
    float heatDummy = _118, hpDummy = HP;
    dmg = DamageByWeapon(f, dmg, wt, Proto->ArmourType, Proto->ClassType, GlobalState,
                         Proto->Thermostat, &heatDummy, &hpDummy);

    float norm[3];
    int side = DamageSide(Pos, Dir, from[0], from[1], from[2], 0, norm);
    float armour = (&Armor[0])[side];                             // [ebp-0x14]
    float protoArmour = (&Proto->FrontArmor)[side];               // [ebp-0xc]

    float pool = GetHitPoints() * HP;                             // x87 PC24
    if (Proto->ClassType == 5) {
        float sum = 0.0f;
        pool = 0.0f;
        if (Members.Size > 0) {
            for (int k = 0; k < Members.Size; ++k)
                sum = sum + WorldUnit(MemberAt(this, k).Unit)->HP;
            pool = WorldUnit(MemberAt(this, 0).Unit)->GetHitPoints() * sum;
        } else if (Parent > -1) {
            int last = -1;
            for (int k = 0;; ++k) {
                SUnit* p = WorldUnit(Parent);
                if (k >= p->Members.Size)
                    break;
                if (MemberAt(p, k).Owner == WorldIndex) {
                    sum = sum + WorldUnit(MemberAt(WorldUnit(Parent), k).Unit)->HP;
                    last = MemberAt(WorldUnit(Parent), k).Unit;
                }
            }
            pool = sum;
            if (last > -1)
                pool = WorldUnit(last)->GetHitPoints() * sum;
        }
    }

    float n = 0.0f;
    do {
        n = n + 1.0f;
        if (armour > 0.0f && protoArmour != 0.0f) {
            float absorb = protoArmour * armour;
            int part = 0;
            if (dmg > absorb) {
                part = (int)(dmg - absorb);
                pool = pool - (float)part;
            }
            armour = armour - ((dmg - (float)part) / 10.0f) / protoArmour;
            if (0.0f > armour)
                armour = 0.0f;
        } else {
            pool = pool - dmg;
        }
    } while (!(0.0f >= pool) && n <= 99.0f);
    return (int)n;
}

// PANZERS 0x562b10
// Own units (not +0x110) are seen; others by bit 0x20 of the player's
// VisMap at the half-tile cell (fistp under CW 0x0c7f: truncation; HD
// stores the cell in the global 0x92e350).
bool SGameLogic::IsSeenByPlayer(int player, SUnit* u)
{
    if (u->Player == player && !u->_110)
        return true;
    float x = u->Pos[0];
    if (0.0f > x || x > (float)g_World->TerrainW)
        return false;
    float z = u->Pos[2];
    if (0.0f > z || z > (float)g_World->TerrainH)
        return false;
    const unsigned char* map = VisMap[player];
    if (!map)
        return false;
    int cx = (int)(x * 2.0f);                                     // 0x7f4558
    int cz = (int)(u->Pos[2] * 2.0f);
    return (map[VisW * cz + cx] >> 5) & 1;
}

// The C2 name of 0x576490 (DamageArea, projectile.cpp).
void SGameLogic::AreaDamage(float damage, int attacker, int p3, float x, float y, float z, float radius,
                            int p8, int p9)
{
    DamageArea(damage, attacker, p3, x, y, z, radius, p8, p9);
}

} // namespace pz
