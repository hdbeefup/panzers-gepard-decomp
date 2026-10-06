// src/game/iunitanim.h
// SIUnitAnimation: the HD unit animation interface (RTTI SUnitAnimation
// vftable 0x7fd7e4, 17 slots; 11 classes) and SIPUnitAnimation (SPUnitAnimation
// vftable 0x7fd724, 5 slots; +0x10 is the factory).
//
// The unit keeps its animation at SUnit +0x14 and calls +0x08 UpdateModel
// from its +0x3c RefreshModel every tick. Menu classes: SVehicleAnimation,
// SWalkerAnimation, SSquadAnimation, SBuildingAnimation (dtor or update hits).
// Animation fields: +0x04 SIUnit*, +0x08 SPUnitAnimation* (base prototype),
// +0x0c type, +0x24 class prototype, +0x28 SRunningGear* (vehicles). HD sizes
// 0x2c..0xc0 (kHdSize* in m2common.h). Implementation: src/game/unitanim.h.
//
// SHARED HEADER (owner P0; slot names belong to agent A).
// Slot comments: "+0xNN HD 0xADDR (N arg dwords)" from the HD vftable and the
// RET imm16 of the implementation (a double counts as 2; "?" = no RET found,
// e.g. a tail jump). "[menu: ...]" = the coverage trace of the original menu
// (docs/re/M2_COVERAGE.md) executed that implementation: "startup" = only while
// the menu loaded, otherwise the steady-loop rate bucket. The hit status is per
// implementation address, so a slot shared by several classes shows the same
// status everywhere. Only hit slots have names; "(name guessed)" names rest on
// the decompiled body or one caller. Slot_XX keep the order (never remove one).
// Override tables: "*" = that override was executed in the menu.

#ifndef PZ_IUNITANIM_H
#define PZ_IUNITANIM_H

#include "m2common.h"

struct SPropertyStruct;   // core/propertystruct.h (global namespace)

namespace pz {

struct SIUnit;
struct SIPUnit;
struct SIModel;
struct SIPUnitAnimation;

struct SIUnitAnimation {
    virtual ~SIUnitAnimation() {}                       // +0x00 HD 0x5c74c0 (1 arg dwords) [menu: periodic<1/s via SVehicleAnimation 0x5c7510] scalar deleting dtor
    virtual void InitModel(SIModel* model) = 0;                  // +0x04 _purecall (1 arg dword) [menu: periodic<1/s via SVehicleAnimation 0x5c93a0] SVehicleAnimation::InitModel (0x5c93a0)
    virtual void UpdateModel() = 0;                              // +0x08 _purecall (? arg dwords) [menu: >=20/s via SVehicleAnimation 0x5cd020] SWalkerAnimation::UpdateModel (0x5ce2a0); per tick from unit +0x3c
    virtual void Slot_0C(void* p1) = 0;                          // +0x0c HD 0x5c76c0 (1 arg dwords) the viewport from SUnit::UpdateVisuals 0x5b76c0; empty; squads 0x5c76a0 -> 0x5cadc0(vp, unit model) place the extra model (M3-C typed)
    virtual float* GetFirePosition(float* out, int gunner, float dirOffset, float kick) = 0; // +0x10 _purecall (4 arg dwords) (name guessed, M3-C) world position of the gunner's next muzzle node (vehicles cycle the muzzles and push the body back by kick along unit dir + dirOffset); returns out
    virtual void AddBodyKick(float x, float y, float z) = 0;     // +0x14 HD 0x5cb420 (3 arg dwords) (name guessed, M3-C) empty; vehicles 0x5cb430 / planes 0x5cb3e0 subtract x, z from the body spring velocity
    virtual float GetStateMoveSpeed(int state) = 0;              // +0x18 HD 0x5c7e90 (1 arg dwords) [menu: >=20/s via SWalkerAnimation 0x5c7ea0] (name guessed) walker: state table +0x24 -> +0x10 [state].+0x1c
    virtual float GetStateTurnSpeed(int state) = 0;              // +0x1c HD 0x5c7f50 (1 arg dwords) [menu: >=20/s] (name guessed) walker [state].+0x20; driver +0x4c
    virtual int GetDriverNode() = 0;                             // +0x20 HD 0x5c7c60 (0 arg dwords) [menu: >=1/s via SVehicleAnimation 0x5c7c70] (name guessed) -1; vehicles: +0x94 = model node "built0_driver"
    virtual int GetHookNode() = 0;                               // +0x24 HD 0x5c8410 (0 arg dwords) (M3-C) -1; vehicles 0x5c8420 +0x98 node "hook" (towing)
    virtual int GetHoleFNode() = 0;                              // +0x28 HD 0x5c8350 (0 arg dwords) (M3-C) -1; vehicles 0x5c8360 +0x9c node "hole_f"
    virtual int GetHoleRNode() = 0;                              // +0x2c HD 0x5c83b0 (0 arg dwords) (M3-C) -1; vehicles 0x5c83c0 +0xa0 node "hole_r"
    virtual float* GetHookPos(float* out2) = 0;                  // +0x30 HD 0x5c8430 (1 arg dwords) (M3-C) (0, 0); vehicles 0x5c8450 +0xa4; returns out2
    virtual float* GetHoleFPos(float* out2) = 0;                 // +0x34 HD 0x5c8370 (1 arg dwords) (M3-C) (0, 0); vehicles 0x5c8390 +0xac
    virtual float* GetHoleRPos(float* out2) = 0;                 // +0x38 HD 0x5c83d0 (1 arg dwords) (M3-C) (0, 0); vehicles 0x5c83f0 +0xb4
    virtual int GetShadowTexture() = 0;                          // +0x3c HD 0x5c8250 (0 arg dwords) [menu: periodic<1/s] (name guessed) -1; walkers (0x5c8260): SPWalkerAnimation +0x1c ShadowTexture; unit +0x50 passes it to model +0xcc
    virtual SIPUnitAnimation* GetPrototype() = 0;                // +0x40 HD 0x5c8240 (0 arg dwords) [menu: periodic<1/s] returns +0x08 (the SPUnitAnimation)
};

struct SIPUnitAnimation {
    virtual ~SIPUnitAnimation() {}                      // +0x00 HD 0x5c7290 (1 arg dwords) scalar deleting dtor
    virtual void Load(SIPUnit* punit, ::SPropertyStruct* props) = 0; // +0x04 _purecall (2 arg dwords) [menu: startup via SPVehicleAnimation 0x5c85c0] (name guessed) from SPUnit 0x5a6ce0 with the "Animation" multi sub-struct
    virtual void LoadResources(SIPUnit* punit, ::SPropertyStruct* props) = 0; // +0x08 HD 0x5ca680 (2 arg dwords) [menu: sporadic] (name guessed) model sequences (punit +0x50), effects, textures
    virtual void Slot_0C() = 0;                                  // +0x0c HD 0x5cb470 (0 arg dwords)
    virtual SIUnitAnimation* CreateAnimation(SIUnit* unit) = 0;  // +0x10 _purecall (? arg dwords) [menu: periodic<1/s via SPVehicleAnimation 0x5c7a30] factory (0x5c7a30 vehicle, 0x5c7ab0 walker, 0x5c7930 squad, 0x5c7770 building)
};

// Animation overrides of SUnitAnimation slots:
//   SVehicleAnimation          vftable 0x7fd82c, 17 slots: +0x00 5c7510* +0x04 5c93a0*
//                                                          +0x08 5cd020* +0x10 5cb270 +0x14 5cb430
//                                                          +0x20 5c7c70* +0x24 5c8420 +0x28 5c8360
//                                                          +0x2c 5c83c0 +0x30 5c8450 +0x34 5c8390
//                                                          +0x38 5c83f0
//   SWalkerAnimation           vftable 0x7fd94c, 17 slots: +0x00 5c7540* +0x04 5ca110*
//                                                          +0x08 5ce2a0* +0x10 5cb370 +0x18 5c7ea0*
//                                                          +0x1c 5c7f60* +0x3c 5c8260
//   SSquadAnimation            vftable 0x7fda6c, 17 slots: +0x00 5c73f0* +0x04 5c9340*
//                                                          +0x08 5cc8c0* +0x0c 5c76a0 +0x10 5cb240
//   SBuildingAnimation         vftable 0x7fdab4, 17 slots: +0x00 5c7100 +0x04 5c8c20* +0x08 5cb650*
//                                                          +0x10 5cb100
//   SWasterAnimation           vftable 0x7fd9dc, 17 slots: +0x00 5c7570 +0x04 5ca600 +0x08 5cf790
//                                                          +0x10 5cb3c0
//   SFlyingAnimation           vftable 0x7fda24, 17 slots: +0x00 5c7150 +0x04 5c8d00 +0x08 5cb750
//                                                          +0x10 5cb120 +0x14 5cb3e0
//   SProjectileAnimation       vftable 0x7fd994, 17 slots: +0x00 5c73a0 +0x04 5c9320 +0x08 5cc480
//                                                          +0x10 5cb220
//   STrainAnimation            vftable 0x7fd8bc, 17 slots: +0x00 5c7490 +0x04 5c9350 +0x08 5cce80
//                                                          +0x10 5cb270 +0x14 5cb430 +0x20 5c7c70*
//                                                          +0x24 5c8420 +0x28 5c8360 +0x2c 5c83c0
//                                                          +0x30 5c8450 +0x34 5c8390 +0x38 5c83f0
//   SBoatAnimation             vftable 0x7fd904, 17 slots: +0x00 5c70d0 +0x04 5c8bf0 +0x08 5cb4e0
//                                                          +0x10 5cb270 +0x14 5cb430 +0x20 5c7c70*
//                                                          +0x24 5c8420 +0x28 5c8360 +0x2c 5c83c0
//                                                          +0x30 5c8450 +0x34 5c8390 +0x38 5c83f0
//   SFlyingFoxAnimation        vftable 0x7fd874, 17 slots: +0x00 5c7180 +0x04 5c93a0* +0x08 5cc470
//                                                          +0x10 5cb270 +0x14 5cb430 +0x20 5c7c70*
//                                                          +0x24 5c8420 +0x28 5c8360 +0x2c 5c83c0
//                                                          +0x30 5c8450 +0x34 5c8390 +0x38 5c83f0
// Prototype overrides of SPUnitAnimation slots:
//   SPVehicleAnimation         vftable 0x7fd73c, 5 slots: +0x00 5c72c0 +0x04 5c85c0* +0x08 5ca690*
//                                                         +0x0c 5cb480 +0x10 5c7a30*
//   SPWalkerAnimation          vftable 0x7fd754, 5 slots: +0x00 5c7310 +0x04 5c86e0* +0x08 5ca980*
//                                                         +0x0c 5cb4b0 +0x10 5c7ab0*
//   SPSquadAnimation           vftable 0x7fd7b4, 5 slots: +0x00 5c7260 +0x04 5c85b0* +0x10 5c7930*
//   SPBuildingAnimation        vftable 0x7fd7cc, 5 slots: +0x00 5c71b0 +0x04 5c8470* +0x08 5ca620*
//                                                         +0x10 5c7770*
//   SPWasterAnimation          vftable 0x7fd784, 5 slots: +0x00 5c7370 +0x04 5c8be0* +0x10 5c7b20
//   SPFlyingAnimation          vftable 0x7fd79c, 5 slots: +0x00 5c71e0 +0x04 5c8480* +0x10 5c77c0
//   SPProjectileAnimation      vftable 0x7fd76c, 5 slots: +0x00 5c7230 +0x04 5c85a0* +0x10 5c78e0
//   SPTrainAnimation           vftable 0x7fa9ac, 5 slots: +0x00 5a5860 +0x04 5c85c0* +0x08 5ca690*
//                                                         +0x0c 5cb480 +0x10 5c7990
//   SPBoatAnimation            vftable 0x7fa9c4, 5 slots: +0x00 5a5400 +0x04 5c85c0* +0x08 5ca690*
//                                                         +0x0c 5cb480 +0x10 5c76d0

} // namespace pz

#endif // PZ_IUNITANIM_H
