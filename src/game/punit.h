// src/game/punit.h
// SIPUnit: the HD unit prototype interface (RTTI SPUnit vftable 0x7fa7c0,
// 6 slots; 9 classes) and the SPUnit skeleton. OWNER: agent U (the slot
// order follows the shared-header rules of docs/M2_INTERFACES.md).
//
// Prototypes come from the .unit property files (611 types, SUnitRegistry
// 0x5cfe30 / LoadUnitFiles 0x5d1050, lifted in M1 with the stand-in
// SUnitType in src/world/pzunitregistry.h). Unit.Common.ClassType picks the
// class (operator new size, ctor):
//   0, 4 SPSingleUnit 0x14c 0x5a4ae0     3 SPProjectileUnit 0x178 0x5a4a20
//   5 SPPanzersSquadUnit 0x14c 0x5a49e0  6 SPPanzersSquadMemberUnit 0x140 0x5a49b0
//   7 SPWasterUnit 0x144 0x5a4db0        8 SPFlyingUnit 0x140 0x5a4980
//   9 SPBuildingUnit 0x160 0x5a4900     10 SPTrainUnit 0x14c 0x5a4b30
// Fields known from M1: +0x40 ClassType, +0x44 UnitType, +0x48 side, +0x60
// name, +0x68/+0x70 units.ini names, +0x84 sight range (SUnit +0x184), +0xdd
// loaded, +0xdf/+0xe0 flags read by SSingleUnit, +0x4c "Gunners" present.
//
// Reflective property system: the .unit tree is read through SProperties
// (SPSingleUnit::Init 0x5a62d0, LoadGunners 0x5a7390, SPDriver/SPUnitAnimation
// Load). Units themselves serialise through the typed variable lists of
// gLoadVariables 0x670440 (pz::LoadVariables / SVarDesc in
// pzunitregistry.h, lifted in M1 for UNTD): SUnit::Load 0x5bbd30 reads the
// sub-tags "vars" (unit descriptor table), "targ" (STarget, table 0x8dd890)
// and "driv" (one list per driver; "Number of drivers doesn't match"). That
// path is for save games (M6); the menu only creates units from UNTD/CREATE.
// Slot comments: "+0xNN HD 0xADDR (N arg dwords)" from the HD vftable and the
// RET imm16 of the implementation (a double counts as 2; "?" = no RET found,
// e.g. a tail jump). "[menu: ...]" = the coverage trace of the original menu
// (docs/re/M2_COVERAGE.md) executed that implementation: "startup" = only while
// the menu loaded, otherwise the steady-loop rate bucket. The hit status is per
// implementation address, so a slot shared by several classes shows the same
// status everywhere. Only hit slots have names; "(name guessed)" names rest on
// the decompiled body or one caller. Slot_XX keep the order (never remove one).
// Override tables: "*" = that override was executed in the menu.

#ifndef PZ_PUNIT_H
#define PZ_PUNIT_H

#include "m2common.h"

namespace pz {

struct SIUnit;
struct SProperties;

struct SIPUnit {
    virtual ~SIPUnit() {}                               // +0x00 HD 0x5a5900 (1 arg dwords) scalar deleting dtor
    virtual void LoadHeader(SProperties* props) = 0;             // +0x04 HD 0x5a6650 (1 arg dwords) [menu: startup] SPSingleUnit::Init (0x5a62d0); M1 stand-in SUnitType::LoadHeader
    virtual void LoadResources(int p1) = 0;                      // +0x08 HD 0x5a8780 (1 arg dwords) [menu: sporadic] SPUnit::LoadResources (models, "PModelIdx < 0"); M1 stand-in SUnitType::Load
    virtual void Slot_0C() = 0;                                  // +0x0c HD 0x5a9950 (0 arg dwords)
    virtual SIUnit* CreateUnit(int worldIndex) = 0;              // +0x10 _purecall (? arg dwords) [menu: periodic<1/s via SPSingleUnit 0x5a5cf0] factory; M1 stand-in SUnitType::CreateUnit
    virtual void LoadGunners(int p1) = 0;                        // +0x14 HD 0x5a7390 (1 arg dwords) [menu: startup] reads "Gunners" (0x5a7390)
};

// Prototype overrides of SPUnit slots:
//   SPSingleUnit               vftable 0x7fa7dc, 6 slots: +0x00 5a57c0 +0x04 5a62d0* +0x10 5a5cf0*
//   SPBuildingUnit             vftable 0x7fa884, 6 slots: +0x00 5a5430 +0x04 5a6050* +0x08 5a7500*
//                                                         +0x0c 5a9730 +0x10 5a5a70*
//   SPPanzersSquadUnit         vftable 0x7fa84c, 6 slots: +0x00 5a5690 +0x04 5a61d0* +0x10 5a5bf0*
//   SPPanzersSquadMemberUnit   vftable 0x7fa868, 6 slots: +0x00 5a5600 +0x08 5a7da0* +0x0c 5a9770
//                                                         +0x10 5a5b70*
//   SPWasterUnit               vftable 0x7fa830, 6 slots: +0x00 5a5960 +0x04 5a6c30* +0x10 5a5df0
//   SPProjectileUnit           vftable 0x7fa814, 6 slots: +0x00 5a5710 +0x04 5a6280* +0x08 5a7e70*
//                                                         +0x0c 5a9790 +0x10 5a5c70
//   SPFlyingUnit               vftable 0x7fa7f8, 6 slots: +0x00 5a5540 +0x04 5a6160* +0x10 5a5af0
//   SPTrainUnit                vftable 0x7fa8a0, 6 slots: +0x00 5a58c0 +0x04 5a6580* +0x10 5a5d70

// Skeleton (logged stubs in punit.cpp). Size: SPUnit base <= 0x140.
struct SPUnit : SIPUnit {
    SPUnit();
    ~SPUnit() override;
    void LoadHeader(SProperties* props) override;
    void LoadResources(int p1) override;
    void Slot_0C() override;
    SIUnit* CreateUnit(int worldIndex) override;
    void LoadGunners(int p1) override;
};

} // namespace pz

#endif // PZ_PUNIT_H
