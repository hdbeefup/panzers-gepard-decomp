// src/game/triggersunits.h
// How the game logic of agent L (triggers, conditions, movement groups, the
// world CRC) reads and writes units. OWNER: agent L. Recompile glue, no HD
// counterpart: HD reads the unit fields straight at their offsets.
//
// Two implementations, picked at compile time from the element type of the
// unit heap (SUnitHeap::Elem::Unit, World+0x4d4):
//  - Real units (the type derives from SIUnit, agent U): fields are read at
//    their HD offsets, exactly as the HD code does (+0xfc player, +0x8c pos,
//    +0x04 SPUnit with +0x40 class type, ...), and orders go through SIUnit.
//  - M1 stand-ins (pz::SMenuUnit): the few fields the stand-in has (player,
//    position, type, stored), the rest reads as the HD default of a placed,
//    idle unit. Side tables keep the per-unit state the HD unit stores for
//    SGameLogic (active-location bits +0x258, movement group +0x254).
// When U switches the heap to its SUnit, the real path is used without any
// change here.

#ifndef PZ_TRIGGERSUNITS_H
#define PZ_TRIGGERSUNITS_H

#include <string.h>
#include <type_traits>
#include "iunit.h"
#include "world.h"
#include "worldapi.h"
#include "pzunitregistry.h"

namespace pz {
namespace m2u {

typedef std::remove_pointer<decltype(((SUnitHeap::Elem*)0)->Unit)>::type HeapUnit;

// HD SPUnit fields read by the logic (pz::SUnitType keeps them at the HD offsets).
enum {
    kPUnitClassType = 0x40,   // EUnitClass (0 single, 5 squad, 6 member, 9 building, 0xb, 0xc, ...)
    kPUnitUnitType = 0x44,    // Unit.Common.UnitType
    kPUnitName = 0x60,        // SString, the .unit file name ("US Sherman")
};

inline SWorld* W() { return g_World; }
inline bool IsLive(int i) { return W() && W()->Units.IsLive(i); }

template <typename T, bool Real = std::is_base_of<SIUnit, T>::value>
struct UnitView;

// ---------------------------------------------------------------------------
// Real units: HD offsets.
template <typename T>
struct UnitView<T, true> {
    enum { kReal = 1 };
    static unsigned char* Raw(int i) { return (unsigned char*)(void*)W()->Units.Array[i].Unit; }
    template <typename F> static F& At(int i, int off) { return *(F*)(Raw(i) + off); }
    static unsigned char* PUnit(int i) { return At<unsigned char*>(i, 0x04); }
    static SIUnit* Iface(int i) { return static_cast<SIUnit*>(W()->Units.Array[i].Unit); }

    static int   Player(int i) { return At<int>(i, 0xfc); }
    static float X(int i) { return At<float>(i, 0x8c); }
    static float Y(int i) { return At<float>(i, 0x90); }
    static float Z(int i) { return At<float>(i, 0x94); }
    static float Dir(int i) { return At<float>(i, 0xb0); }
    static int   ClassType(int i) { return *(int*)(PUnit(i) + kPUnitClassType); }
    static int   UnitType(int i) { return *(int*)(PUnit(i) + kPUnitUnitType); }
    static const char* TypeName(int i) { const SString* s = (const SString*)(PUnit(i) + kPUnitName); return s->buf ? s->buf : ""; }
    static int   PUnitInt(int i, int off) { return *(int*)(PUnit(i) + off); }
    static unsigned char PUnitByte(int i, int off) { return PUnit(i)[off]; }
    static int   ActiveDriver(int i) { return At<int>(i, 0x28); }           // +0x28
    static float Size(int i) { return At<float>(i, 0x58); }                 // +0x58 (FindEmptySpace)
    static int   Container(int i) { return At<int>(i, 0x78); }              // +0x78 (-1 = not stored)
    static bool  B7c(int i) { return At<char>(i, 0x7c) != 0; }              // +0x7c
    static unsigned short FlagsD8(int i) { return At<unsigned short>(i, 0xd8); }
    static void  SetFlagsD8(int i, unsigned short v) { At<unsigned short>(i, 0xd8) = v; }
    static bool  B110(int i) { return At<char>(i, 0x110) != 0; }            // +0x110 (dead?)
    static bool  B150(int i) { return At<char>(i, 0x150) != 0; }            // +0x150 (dying?)
    static bool  Unplaced(int i) { return At<char>(i, 0x168) != 0; }        // +0x168 (stored / unplaced)
    static int   StoredCount(int i) { return At<int>(i, 0x170); }           // +0x16c SDArray {ptr, size}
    static int   StoredKind(int i, int k) { return (*(int**)(Raw(i) + 0x16c))[k * 2 + 1]; }
    static const char* ScriptId(int i) { const SString* s = (const SString*)(Raw(i) + 0x194); return s->buf ? s->buf : ""; }
    static int   I250(int i) { return At<int>(i, 0x250); }
    static int   RawInt(int i, int off) { return At<int>(i, off); }
    static int&  MovementGroup(int i) { return At<int>(i, 0x254); }         // +0x254 SGameLogic+0x2dc index
    static unsigned& ActiveLocations(int i) { return At<unsigned>(i, 0x258); }  // +0x258 bit per active location
    static int   WorldIndex(int i) { return At<int>(i, 0x74); }             // +0x74
};

// ---------------------------------------------------------------------------
// M1 stand-ins (pz::SMenuUnit).
struct StandInState {
    int MovementGroup;
    unsigned ActiveLocations;
    // Test mover of the stand-ins (movementgroup.cpp StandInMove): the last
    // order, so the location events can fire before agent U's units move.
    int   Order;          // 0 none, 1 move to (TX, TZ), 7 follow unit Follow
    float TX, TZ;
    int   Follow;
};
StandInState* StandIn(int i);   // gamelogic.cpp (grows with the heap)

template <typename T>
struct UnitView<T, false> {
    enum { kReal = 0 };
    static T* U(int i) { return W()->Units.Array[i].Unit; }
    static SIUnit* Iface(int) { return nullptr; }

    static int   Player(int i) { return U(i)->Player; }
    static float X(int i) { return U(i)->Pos[0]; }
    static float Y(int i) { return U(i)->Pos[1]; }
    static float Z(int i) { return U(i)->Pos[2]; }
    static float Dir(int i) { return U(i)->Dir; }
    static int   ClassType(int i) { return U(i)->Type ? U(i)->Type->ClassType : 0; }
    static int   UnitType(int i) { return U(i)->Type ? U(i)->Type->UnitType : 0; }
    static const char* TypeName(int i) { return U(i)->Type ? SStr(U(i)->Type->Name) : ""; }
    static int   PUnitInt(int, int) { return 0; }
    static unsigned char PUnitByte(int, int) { return 0; }
    static int   ActiveDriver(int i) { return ClassType(i) == 0 || ClassType(i) == 5 ? 0 : -1; }
    static float Size(int) { return 1.0f; }
    // Squad members belong to their squad (HD: stored in it).
    static int   Container(int i) { return ClassType(i) == UC_SQUAD_MEMBER ? 0 : -1; }
    static bool  B7c(int) { return false; }
    static unsigned short FlagsD8(int) { return 0; }
    static void  SetFlagsD8(int, unsigned short) {}
    static bool  B110(int) { return false; }
    static bool  B150(int) { return false; }
    static bool  Unplaced(int i) { return U(i)->Stored; }
    static int   StoredCount(int) { return 0; }
    static int   StoredKind(int, int) { return 0; }
    static const char* ScriptId(int) { return ""; }
    static int   I250(int) { return -1; }
    static int   RawInt(int, int) { return -1; }
    static int&  MovementGroup(int i) { return StandIn(i)->MovementGroup; }
    static unsigned& ActiveLocations(int i) { return StandIn(i)->ActiveLocations; }
    static int   WorldIndex(int i) { return i; }
};

typedef UnitView<HeapUnit> UV;

// Iterate the live units in SHeapTRB slot order (the HD loop idiom).
#define PZ_FOR_EACH_UNIT(i) \
    for (int i = 0; pz::g_World && i < pz::g_World->Units.Size; ++i) if (pz::g_World->Units.IsLive(i))

} // namespace m2u
} // namespace pz

#endif // PZ_TRIGGERSUNITS_H
