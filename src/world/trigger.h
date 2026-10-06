// src/world/trigger.h
// Map triggers (TRIG chunk, HD loader 0x5f0140): STrigger 0x2c with one
// STriggerEvent (8), an SDArray of STriggerCondition (0x38) and an SDArray of
// STriggerAction (0x64). World+0x7474 holds the SDArray<STrigger>
// (count +0x7478). The runtime state lives in SGameLogic (SRunningTrigger
// 0x34 at SGameLogic+0x268) and is driven by RunTriggers 0x579ab0.
//
// Field layout from the HD loaders 0x5b2400 (event), 0x5b22b0 (condition),
// 0x5b2010 (action) and the property masks 0x5b1f30 / 0x5b1d50. Fields a mask
// does not select keep the defaults the loaders write (-1, 0 or empty). The
// decoded menu.map triggers are in docs/re/M2_SCOPE.md 2.2; the decoder is
// m2scope/trigdec.py.
//
// The loaders may be adapted from S.W.I.N.E. world/trigger.cpp (same scheme,
// other sizes and masks); mark such code "// SWINE-adapted, PANZERS 0xADDR".
//
// SHARED HEADER (owner P0; the bodies in trigger.cpp belong to agent L).

#ifndef PZ_TRIGGER_H
#define PZ_TRIGGER_H

#include <stddef.h>
#include "pz/pzcommon.h"
#include "m2common.h"
#include "string2.h"

struct SStream;

namespace pz {

// Event types (Panzers editor text table; the masks match).
enum ETriggerEvent {
    TE_EVERY_SECOND = 0,      // dispatched by 0x570cc0 from Refresh (frame % 20 == 0)
    TE_UNIT_DIES = 1,
    TE_ENTERS_LOCATION = 2,   // Param = location; 0x571280 from UpdateActiveLocations 0x582080
    TE_LEAVES_LOCATION = 3,   // Param = location; 0x571470
    TE_ATTACKED = 4,          // 0x570e40 from TakeDamage
    TE_ENTERS_BUILDING = 5,
    TE_LEAVES_BUILDING = 6,
    TE_STARTS_TOWING = 7,
    TE_STOPS_TOWING = 8,
    TE_BOMBER_SENT = 9,       // Param = location
    TE_PARATROOPERS_SENT = 10,// Param = location
};

// Condition and action IDs the menu uses (full lists: editor_strings.txt).
enum ETriggerCondition {
    TC_FIND_UNITS_AT = 2,            // mask 0xd: player, quantity, location
    TC_COMPARE_VARIABLE = 3,         // mask 0x200: P200[0..2]
    TC_TRIGGERING_UNIT_TYPE = 5,     // mask 8: unit type + class
    TC_PUT_TRIGGERING_UNIT = 6,      // mask 0
};
enum ETriggerAction {
    TA_MOVE_TO_LOCATION = 0x02,
    TA_SET_VARIABLE = 0x03,
    TA_SET_VARIABLE_PER_SECOND = 0x08,
    TA_PRESERVE_TRIGGER = 0x0a,
    TA_MOVE_ALONG_PATH_CONVOY = 0x1e,
    TA_REMOVE_FOUND_UNITS = 0x1f,
    TA_TELEPORT = 0x26,
    TA_CREATE_UNIT = 0x29,
};

// Layout of the HD loader 0x5b2400 (8 bytes).
struct STriggerEvent {
    int Type;             // +0x00 ETriggerEvent
    int Param;            // +0x04 location for types 2, 3, 9, 10; else -1

    void Load(SStream* s);                  // 0x5b2400
};

// 0x38 bytes. Property mask 0x5b1f30.
struct STriggerCondition {
    int     Type;         // +0x00
    int     Player;       // +0x04 mask 1
    bool    QuantityMore; // +0x08 mask 2 (default 1)
    unsigned char _09[3];
    int     Quantity;     // +0x0c mask 2
    int     Location;     // +0x10 mask 4
    int     UnitType;     // +0x14 mask 8
    SString UnitClass;    // +0x18 mask 8 (u16 string, 0x56e7d0)
    int     P200[3];      // +0x20 mask 0x200 (compare variable: variable, operator, value)
    SString Str40000;     // +0x2c mask 0x40000
    int     P2000;        // +0x34 mask 0x2000
    // HD still reads SWINE's mask 0x40 pair and discards it.

    void Load(SStream* s);                  // 0x5b22b0
    static unsigned GetPropertyMask(int type);   // 0x5b1f30
};

// 0x64 bytes. Property mask 0x5b1d50.
struct STriggerAction {
    int     Type;         // +0x00
    int     Player;       // +0x04 mask 1
    int     Location;     // +0x08 mask 4
    int     UnitType;     // +0x0c mask 8
    SString UnitClass;    // +0x10 mask 8
    SString Str10;        // +0x18 mask 0x10
    bool    B100000;      // +0x20 mask 0x100000
    unsigned char _21[3];
    int     P20;          // +0x24 mask 0x20
    int     Num;          // +0x28 mask 0x100
    int     P400000;      // +0x2c mask 0x400000
    int     P400;         // +0x30 mask 0x400
    int     P800;         // +0x34 mask 0x800 (path for 0x1e)
    int     P2000;        // +0x38 mask 0x2000
    int     P80000;       // +0x3c mask 0x80000
    int     P8000;        // +0x40 mask 0x8000
    int     P10000;       // +0x44 mask 0x10000
    int     P20000[2];    // +0x48 mask 0x20000
    SString Str1000;      // +0x50 mask 0x1000
    SString Str4000;      // +0x58 mask 0x4000
    int     P200000;      // +0x60 mask 0x200000
    // HD reads and discards the mask 0x80 pair.

    void Load(SStream* s);                  // 0x5b2010
    static unsigned GetPropertyMask(int type);   // 0x5b1d50
};

// HD SDArray<T> (0x0c bytes).
template <typename T>
struct STriggerArray {
    T*  Array;
    int Size;
    int Max;
};

// 0x2c bytes, World+0x7474 array (0x5f0140).
struct STrigger {
    int           Flags;       // +0x00 2 on live triggers, 0 on separators; RunTriggers clears bit 0
    SString       Name;        // +0x04 (u16 string)
    STriggerEvent Event;       // +0x0c
    STriggerArray<STriggerCondition> Conditions;   // +0x14 (0x5f0240)
    STriggerArray<STriggerAction>    Actions;      // +0x20 (0x5f01e0)
};

// Found-unit group element (0x0c; SDArray 0x560d80 / 0x560790).
struct SFoundUnit {
    bool Flag;            // +0x00 cleared by 0x582770
    unsigned char _01[3];
    int  _04;             // +0x04
    int  Unit;            // +0x08 world unit index
};

// Found-unit group (0x20): the units the conditions selected, and their
// centre, spread and leader as computed by 0x582770. Running trigger +0x14.
struct SFoundUnits {
    float X;              // +0x00 mean position
    float Z;              // +0x04
    float RadiusSq;       // +0x08 4 * max squared distance from the centre (1048576 = too spread)
    int   Kind;           // +0x0c 0xff, or the triggering unit's +0xd8 flags (conditions 6/10)
    int   Leader;         // +0x10 unit picked by class priority (0x582770), -1 = none
    SFoundUnit* Units;    // +0x14 SDArray
    int   Count;          // +0x18
    int   Max;            // +0x1c
};

// SGameLogic+0x268 array element (0x34). Built on the stack by the event
// dispatchers (0x570cc0, 0x571280, ...), copied in by CheckConditions
// 0x580600 when every condition holds, executed by RunTriggers 0x579ab0.
struct SRunningTrigger {
    int   Trigger;        // +0x00 World+0x7474 index
    int   Action;         // +0x04 next action (RunTriggers runs actions until a wait or the end)
    int   Wait;           // +0x08 ticks left to wait
    int   Unit;           // +0x0c triggering unit (events 1..8), -1
    int   Unit2;          // +0x10 second unit (attacker, building), -1
    SFoundUnits Found;    // +0x14
};

// Loads the whole TRIG chunk into World+0x7474 (0x5f0140). Agent L.
void LoadTriggers(STriggerArray<STrigger>* triggers, SStream* s);   // 0x5f0140

PZ_HD_SIZE(STriggerEvent, kHdSizeSTriggerEvent);
PZ_HD_SIZE(STriggerCondition, kHdSizeSTriggerCondition);
PZ_HD_SIZE(STriggerAction, kHdSizeSTriggerAction);
PZ_HD_SIZE(STrigger, kHdSizeSTrigger);
PZ_HD_SIZE(SRunningTrigger, kHdSizeSRunningTrigger);
#if defined(_M_IX86)
static_assert(offsetof(STriggerCondition, Location) == 0x10, "0x5b22b0 param_1[4]");
static_assert(offsetof(STriggerCondition, P200) == 0x20, "0x5b22b0 param_1[8]");
static_assert(offsetof(STriggerCondition, P2000) == 0x34, "0x5b22b0 param_1[0xd]");
static_assert(offsetof(STriggerAction, P20) == 0x24, "0x5b2010 param_1[9]");
static_assert(offsetof(STriggerAction, P20000) == 0x48, "0x5b2010 param_1[0x12]");
static_assert(offsetof(STriggerAction, Str1000) == 0x50, "0x5b2010 param_1[0x14]");
static_assert(offsetof(STriggerAction, P200000) == 0x60, "0x5b2010 param_1[0x18]");
static_assert(offsetof(STrigger, Event) == 0x0c, "0x5f0140 +0x0c");
static_assert(offsetof(STrigger, Actions) == 0x20, "0x5f01e0 +0x20");
static_assert(sizeof(SFoundUnit) == 0x0c, "0x560790 stride 0x0c");
static_assert(sizeof(SFoundUnits) == 0x20, "0x5604d0 copy");
static_assert(offsetof(SRunningTrigger, Found) == 0x14, "0x580600 param_2 + 5");
static_assert(offsetof(SRunningTrigger, Found.Count) == 0x2c, "0x579ab0 +0x2c");
#endif

} // namespace pz

#endif // PZ_TRIGGER_H
