// src/game/savedesc.cpp
// The variable descriptor tables of the save game (gSaveVariables 0x670c50 /
// gLoadVariables 0x670440): the unit, driver, gunner, target and logic
// descriptor lists of HD .data, entry {name, type, offset, members, element
// size, element type}. Read from the HD PANZERS.exe (agent S, M4); offsets
// are HD's, the unit / driver / gunner classes keep HD's layout (unit.h).
// OWNER: agent S (docs/M4_STATUS.md).

#include "unitsave.h"
#include "unit.h"
#include "singleunit.h"
#include "flying.h"
#include "projectile.h"
#include "flyingdriver.h"
#include "projectile_driver.h"

namespace pz {

extern const SVarDesc kDesc_8dd428[];
extern const SVarDesc kDesc_8dda88[];
extern const SVarDesc kDesc_8dd9c8[];
extern const SVarDesc kDesc_8dd530[];
extern const SVarDesc kDesc_8dd4a0[];
extern const SVarDesc kDesc_8dd578[];
extern const SVarDesc kDesc_8dd5c0[];
extern const SVarDesc kDesc_8dd668[];
extern const SVarDesc kDesc_8da7b0[];
extern const SVarDesc kDesc_8dc038[];
extern const SVarDesc kDesc_8dabd0[];
extern const SVarDesc kDesc_8dab40[];
extern const SVarDesc kDesc_8dbf90[];
extern const SVarDesc kDesc_8dbd38[];
extern const SVarDesc kDesc_8dbd80[];
extern const SVarDesc kDesc_8dbe70[];

// HD 0x8dd428
const SVarDesc kDesc_8dd428[] = {
    { "Type", 2, 0x0, nullptr, 0x0, 0 },
    { "Switch", 2, 0x4, nullptr, 0x0, 0 },
    { "Pressed", 2, 0x8, nullptr, 0x0, 0 },
    { "SpecialTime", 2, 0xc, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dda88
const SVarDesc kDesc_8dda88[] = {
    { "ActiveState", 2, 0x0, nullptr, 0x0, 0 },
    { "OldState", 2, 0x4, nullptr, 0x0, 0 },
    { "NeededState", 2, 0x8, nullptr, 0x0, 0 },
    { "SoldierState", 2, 0xc, nullptr, 0x0, 0 },
    { "StateChanging", 1, 0x10, nullptr, 0x0, 0 },
    { "StartStateChanging", 1, 0x11, nullptr, 0x0, 0 },
    { "StateChangeCounter", 2, 0x14, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dd9c8
const SVarDesc kDesc_8dd9c8[] = {
    { "Type", 2, 0x0, nullptr, 0x0, 0 },
    { "Point.X", 3, 0x4, nullptr, 0x0, 0 },
    { "Point.Z", 3, 0x8, nullptr, 0x0, 0 },
    { "Target", 2, 0xc, nullptr, 0x0, 0 },
    { "int_Value", 2, 0x10, nullptr, 0x0, 0 },
    { "float_Value", 3, 0x14, nullptr, 0x0, 0 },
    { "Ctrl", 1, 0x18, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dd530
const SVarDesc kDesc_8dd530[] = {
    { "Idx", 2, 0x0, nullptr, 0x0, 0 },
    { "IsOnMe", 1, 0x4, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dd4a0
const SVarDesc kDesc_8dd4a0[] = {
    { "Idx", 2, 0x0, nullptr, 0x0, 0 },
    { "ServerFrame", 2, 0x4, nullptr, 0x0, 0 },
    { "Counter", 2, 0x8, nullptr, 0x0, 0 },
    { "ValidForAI", 1, 0xc, nullptr, 0x0, 0 },
    { "ValidForEC", 1, 0xd, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dd578
const SVarDesc kDesc_8dd578[] = {
    { "Idx", 2, 0x0, nullptr, 0x0, 0 },
    { "Special", 2, 0x4, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dd5c0
const SVarDesc kDesc_8dd5c0[] = {
    { "Idx", 2, 0x0, nullptr, 0x0, 0 },
    { "OwnerUnit", 2, 0x4, nullptr, 0x0, 0 },
    { "Role", 2, 0x8, nullptr, 0x0, 0 },
    { "RoleValue", 2, 0xc, nullptr, 0x0, 0 },
    { "Node", 2, 0x10, nullptr, 0x0, 0 },
    { "NodeAttach", 1, 0x14, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dd668
const SVarDesc kDesc_8dd668[] = {
    { "Unit_Pos", 4, 0x0, nullptr, 0x0, 0 },
    { "Unit_Dir", 3, 0xc, nullptr, 0x0, 0 },
    { "Unit_MoveSpeed", 3, 0x10, nullptr, 0x0, 0 },
    { "Unit_MoveBackward", 1, 0x14, nullptr, 0x0, 0 },
    { "Unit_SpinSpeed", 3, 0x18, nullptr, 0x0, 0 },
    { "SpeedGear", 2, 0x1c, nullptr, 0x0, 0 },
    { "SpinGear", 2, 0x20, nullptr, 0x0, 0 },
    { "WayPointIdx", 2, 0x24, nullptr, 0x0, 0 },
    { "ManoeuvreIdx", 2, 0x28, nullptr, 0x0, 0 },
    { "PointIdx", 2, 0x2c, nullptr, 0x0, 0 },
    { "Member_Pos[0]", 8, 0x30, nullptr, 0x0, 0 },
    { "Member_Dir[0]", 3, 0x58, nullptr, 0x0, 0 },
    { "Member_Pos[1]", 8, 0x38, nullptr, 0x0, 0 },
    { "Member_Dir[1]", 3, 0x5c, nullptr, 0x0, 0 },
    { "Member_Pos[2]", 8, 0x40, nullptr, 0x0, 0 },
    { "Member_Dir[2]", 3, 0x60, nullptr, 0x0, 0 },
    { "Member_Pos[3]", 8, 0x48, nullptr, 0x0, 0 },
    { "Member_Dir[3]", 3, 0x64, nullptr, 0x0, 0 },
    { "Member_Pos[4]", 8, 0x50, nullptr, 0x0, 0 },
    { "Member_Dir[4]", 3, 0x68, nullptr, 0x0, 0 },
    { "Formation_Rotation", 3, 0x6c, nullptr, 0x0, 0 },
    { "RoadDistance", 3, 0x70, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dc540
const SVarDesc kSUnitDesc[] = {
    { "Special", 6, 0x18, kDesc_8dd428, 0x0, 0 },
    { "ActiveDriver", 2, 0x28, nullptr, 0x0, 0 },
    { "LastActiveDriver", 2, 0x2c, nullptr, 0x0, 0 },
    { "SlowDownDriver", 2, 0x30, nullptr, 0x0, 0 },
    { "DriverDisabled", 1, 0x34, nullptr, 0x0, 0 },
    { "MainGunner", 2, 0x44, nullptr, 0x0, 0 },
    { "LastAttackedFrame", 2, 0x60, nullptr, 0x0, 0 },
    { "WorldIdx", 2, 0x74, nullptr, 0x0, 0 },
    { "OwnerUnit", 2, 0x78, nullptr, 0x0, 0 },
    { "AIGroup", 2, 0x80, nullptr, 0x0, 0 },
    { "XP", 2, 0x64, nullptr, 0x0, 0 },
    { "FirstKill", 1, 0x68, nullptr, 0x0, 0 },
    { "FirstBlood", 1, 0x69, nullptr, 0x0, 0 },
    { "FirstShot", 1, 0x6a, nullptr, 0x0, 0 },
    { "FirstVehicleLost", 1, 0x6b, nullptr, 0x0, 0 },
    { "FirstArmouredVehicleKill", 1, 0x6c, nullptr, 0x0, 0 },
    { "Towed", 1, 0x7c, nullptr, 0x0, 0 },
    { "RefreshedInFrame", 2, 0x84, nullptr, 0x0, 0 },
    { "Pos", 4, 0x8c, nullptr, 0x0, 0 },
    { "Yrel", 3, 0x88, nullptr, 0x0, 0 },
    { "LastPos", 4, 0x98, nullptr, 0x0, 0 },
    { "LastDir", 3, 0xb4, nullptr, 0x0, 0 },
    { "PreLastPos", 4, 0xa4, nullptr, 0x0, 0 },
    { "PreLastDir", 3, 0xb8, nullptr, 0x0, 0 },
    { "Speed", 4, 0xbc, nullptr, 0x0, 0 },
    { "MoveSpeed", 3, 0xc8, nullptr, 0x0, 0 },
    { "MoveBackward", 1, 0xcc, nullptr, 0x0, 0 },
    { "WillMoveInThisServerFrame", 1, 0xcd, nullptr, 0x0, 0 },
    { "Member_Pos[0]", 8, 0x210, nullptr, 0x0, 0 },
    { "Member_Dir[0]", 3, 0x238, nullptr, 0x0, 0 },
    { "Member_Pos[1]", 8, 0x218, nullptr, 0x0, 0 },
    { "Member_Dir[1]", 3, 0x23c, nullptr, 0x0, 0 },
    { "Member_Pos[2]", 8, 0x220, nullptr, 0x0, 0 },
    { "Member_Dir[2]", 3, 0x240, nullptr, 0x0, 0 },
    { "Member_Pos[3]", 8, 0x228, nullptr, 0x0, 0 },
    { "Member_Dir[3]", 3, 0x244, nullptr, 0x0, 0 },
    { "Member_Pos[4]", 8, 0x230, nullptr, 0x0, 0 },
    { "Member_Dir[4]", 3, 0x248, nullptr, 0x0, 0 },
    { "Formation_Rotation", 3, 0x24c, nullptr, 0x0, 0 },
    { "SpinSpeed", 3, 0xd0, nullptr, 0x0, 0 },
    { "WheelTurnAngle", 3, 0xd4, nullptr, 0x0, 0 },
    { "MovementMask", 12, 0xd8, nullptr, 0x0, 0 },
    { "VoiceVar", 2, 0xdc, nullptr, 0x0, 0 },
    { "GlobalState", 6, 0xe0, kDesc_8dda88, 0x0, 0 },
    { "MoveState", 2, 0xf8, nullptr, 0x0, 0 },
    { "Dir", 3, 0xb0, nullptr, 0x0, 0 },
    { "Player", 2, 0xfc, nullptr, 0x0, 0 },
    { "Color", 2, 0x100, nullptr, 0x0, 0 },
    { "ClientSelection", 2, 0x104, nullptr, 0x0, 0 },
    { "SyncedSelection", 2, 0x108, nullptr, 0x0, 0 },
    { "GroupID", 2, 0x10c, nullptr, 0x0, 0 },
    { "Abandoned", 1, 0x110, nullptr, 0x0, 0 },
    { "Invulnerable", 1, 0x111, nullptr, 0x0, 0 },
    { "Selectable", 1, 0x112, nullptr, 0x0, 0 },
    { "HP", 3, 0x114, nullptr, 0x0, 0 },
    { "Thermostat", 3, 0x118, nullptr, 0x0, 0 },
    { "FrontArmor", 3, 0x11c, nullptr, 0x0, 0 },
    { "LeftSideArmor", 3, 0x120, nullptr, 0x0, 0 },
    { "RightSideArmor", 3, 0x124, nullptr, 0x0, 0 },
    { "BackArmor", 3, 0x128, nullptr, 0x0, 0 },
    { "TopArmor", 3, 0x12c, nullptr, 0x0, 0 },
    { "Entrenchment", 2, 0x130, nullptr, 0x0, 0 },
    { "Slots[0].Type", 2, 0x138, nullptr, 0x0, 0 },
    { "Slots[1].Type", 2, 0x144, nullptr, 0x0, 0 },
    { "Slots[0].Count", 2, 0x13c, nullptr, 0x0, 0 },
    { "Slots[1].Count", 2, 0x148, nullptr, 0x0, 0 },
    { "Slots[0].Automatic", 1, 0x140, nullptr, 0x0, 0 },
    { "Slots[1].Automatic", 1, 0x14c, nullptr, 0x0, 0 },
    { "LastHeardFrame[0]", 2, 0x26c, nullptr, 0x0, 0 },
    { "LastHeardFrame[1]", 2, 0x270, nullptr, 0x0, 0 },
    { "LastHeardFrame[2]", 2, 0x274, nullptr, 0x0, 0 },
    { "LastHeardFrame[3]", 2, 0x278, nullptr, 0x0, 0 },
    { "LastHeardFrame[4]", 2, 0x27c, nullptr, 0x0, 0 },
    { "LastHeardFrame[5]", 2, 0x280, nullptr, 0x0, 0 },
    { "LastHeardFrame[6]", 2, 0x284, nullptr, 0x0, 0 },
    { "LastHeardFrame[7]", 2, 0x288, nullptr, 0x0, 0 },
    { "LastHeardFrame[8]", 2, 0x28c, nullptr, 0x0, 0 },
    { "LastHeardFrame[9]", 2, 0x290, nullptr, 0x0, 0 },
    { "LastHeardFrame[10]", 2, 0x294, nullptr, 0x0, 0 },
    { "LastHeardFrame[11]", 2, 0x298, nullptr, 0x0, 0 },
    { "LastSeenFrame[0]", 2, 0x29c, nullptr, 0x0, 0 },
    { "LastSeenFrame[1]", 2, 0x2a0, nullptr, 0x0, 0 },
    { "LastSeenFrame[2]", 2, 0x2a4, nullptr, 0x0, 0 },
    { "LastSeenFrame[3]", 2, 0x2a8, nullptr, 0x0, 0 },
    { "LastSeenFrame[4]", 2, 0x2ac, nullptr, 0x0, 0 },
    { "LastSeenFrame[5]", 2, 0x2b0, nullptr, 0x0, 0 },
    { "LastSeenFrame[6]", 2, 0x2b4, nullptr, 0x0, 0 },
    { "LastSeenFrame[7]", 2, 0x2b8, nullptr, 0x0, 0 },
    { "LastSeenFrame[8]", 2, 0x2bc, nullptr, 0x0, 0 },
    { "LastSeenFrame[9]", 2, 0x2c0, nullptr, 0x0, 0 },
    { "LastSeenFrame[10]", 2, 0x2c4, nullptr, 0x0, 0 },
    { "LastSeenFrame[11]", 2, 0x2c8, nullptr, 0x0, 0 },
    { "Brick[0]", 1, 0x2cc, nullptr, 0x0, 0 },
    { "Brick[1]", 1, 0x2cd, nullptr, 0x0, 0 },
    { "Brick[2]", 1, 0x2ce, nullptr, 0x0, 0 },
    { "Brick[3]", 1, 0x2cf, nullptr, 0x0, 0 },
    { "Brick[4]", 1, 0x2d0, nullptr, 0x0, 0 },
    { "Brick[5]", 1, 0x2d1, nullptr, 0x0, 0 },
    { "Brick[6]", 1, 0x2d2, nullptr, 0x0, 0 },
    { "Brick[7]", 1, 0x2d3, nullptr, 0x0, 0 },
    { "Brick[8]", 1, 0x2d4, nullptr, 0x0, 0 },
    { "Brick[9]", 1, 0x2d5, nullptr, 0x0, 0 },
    { "Brick[10]", 1, 0x2d6, nullptr, 0x0, 0 },
    { "Brick[11]", 1, 0x2d7, nullptr, 0x0, 0 },
    { "IHaveToDie", 1, 0x150, nullptr, 0x0, 0 },
    { "BoomDie", 1, 0x151, nullptr, 0x0, 0 },
    { "FireDie", 1, 0x152, nullptr, 0x0, 0 },
    { "Dead", 1, 0x153, nullptr, 0x0, 0 },
    { "StartFightAnimation", 1, 0x154, nullptr, 0x0, 0 },
    { "StartDying", 1, 0x155, nullptr, 0x0, 0 },
    { "Dying", 1, 0x156, nullptr, 0x0, 0 },
    { "ShootingCounter", 2, 0x158, nullptr, 0x0, 0 },
    { "DieAnimDurationCounter", 2, 0x15c, nullptr, 0x0, 0 },
    { "StartCustomAnimation", 5, 0x160, nullptr, 0x0, 0 },
    { "Hidden", 1, 0x168, nullptr, 0x0, 0 },
    { "HiddenModelVisible", 1, 0x169, nullptr, 0x0, 0 },
    { "ReservedVehicleIdx", 2, 0x18c, nullptr, 0x0, 0 },
    { "Reserved", 1, 0x190, nullptr, 0x0, 0 },
    { "Behavior", 2, 0x250, nullptr, 0x0, 0 },
    { "MovementGroup", 2, 0x254, nullptr, 0x0, 0 },
    { "ScriptID", 5, 0x194, nullptr, 0x0, 0 },
    { "Commands", 10, 0x19c, kDesc_8dd9c8, 0x1c, 6 },
    { "NearbyUnitsForDriver", 10, 0x1a8, kDesc_8dd530, 0x8, 6 },
    { "NearbyUnitsForAI", 10, 0x1b4, kDesc_8dd530, 0x8, 6 },
    { "AI_InvalidTargetUnits", 10, 0x1c0, kDesc_8dd4a0, 0x10, 6 },
    { "AI_DisabledFrame", 2, 0x1cc, nullptr, 0x0, 0 },
    { "AI_DisabledCounter", 2, 0x1d0, nullptr, 0x0, 0 },
    { "AI_OverrideIngame", 1, 0x1d4, nullptr, 0x0, 0 },
    { "AttackLastSoundFrame", 2, 0x1f0, nullptr, 0x0, 0 },
    { "StoredUnits", 10, 0x16c, kDesc_8dd578, 0x8, 6 },
    { "StoredMembers", 10, 0x178, kDesc_8dd5c0, 0x18, 6 },
    { "Stored", 1, 0x184, nullptr, 0x0, 0 },
    { "StoredSpecial", 2, 0x188, nullptr, 0x0, 0 },
    { "ChildUnitIdxArray", 10, 0x1fc, nullptr, 0x4, 2 },
    { "BuiltInDriverIdx", 2, 0x208, nullptr, 0x0, 0 },
    { "EnableShotEffects", 1, 0x20c, nullptr, 0x0, 0 },
    { "Behavior", 2, 0x250, nullptr, 0x0, 0 },
    { "MovementGroup", 2, 0x254, nullptr, 0x0, 0 },
    { "SubStateString", 5, 0x25c, nullptr, 0x0, 0 },
    { "LastMoveFrame", 2, 0x264, nullptr, 0x0, 0 },
    { "LastMemberDyingFrame", 2, 0x268, nullptr, 0x0, 0 },
    { "GhostFrames", 11, 0x1d8, kDesc_8dd668, 0x74, 6 },
    { "TowedUnitIdx", 2, 0x2d8, nullptr, 0x0, 0 },
    { "TowedMaxMoveSpeed", 3, 0x2dc, nullptr, 0x0, 0 },
    { "Priority", 2, 0x2e0, nullptr, 0x0, 0 },
    { "EnableBoatButton", 1, 0x2e8, nullptr, 0x0, 0 },
    { "EnableUnloadButton", 1, 0x2e9, nullptr, 0x0, 0 },
    { "BoatEquipmentOwner", 2, 0x2e4, nullptr, 0x0, 0 },
    { "AutoSkill", 1, 0x2eb, nullptr, 0x0, 0 },
    { "Cargo", 3, 0x2ec, nullptr, 0x0, 0 },
    { "ChangeGlobalStateDelay", 2, 0x2f0, nullptr, 0x0, 0 },
    { "GuardPos", 4, 0x2f8, nullptr, 0x0, 0 },
    { "GuardPosLeft", 1, 0x2f4, nullptr, 0x0, 0 },
    { "RoadDistance", 3, 0x300, nullptr, 0x0, 0 },
    { "BoatEquipmentOnWater", 1, 0x304, nullptr, 0x0, 0 },
    { "SavedGlobalWayPointsArray", 10, 0x308, nullptr, 0x8, 8 },
    { "MoveInWhat", 2, 0x338, nullptr, 0x0, 0 },
    { "Night_Effects_Enabled", 1, 0x6d, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dc358
const SVarDesc kSSingleUnitDesc[] = {
    { "SUnit", 6, 0x0, kSUnitDesc, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8daf48
const SVarDesc kSFlyingUnitDesc[] = {
    { "SUnit", 6, 0x0, kSUnitDesc, 0x0, 0 },
    { "DropTime", 2, 0x344, nullptr, 0x0, 0 },
    { "BombCounter", 2, 0x348, nullptr, 0x0, 0 },
    { "ParatrooperCounter", 2, 0x34c, nullptr, 0x0, 0 },
    { "TargetUnit", 2, 0x350, nullptr, 0x0, 0 },
    { "TargetGroundPos", 4, 0x354, nullptr, 0x0, 0 },
    { "StartPos", 4, 0x360, nullptr, 0x0, 0 },
    { "TacBomberSpeed", 3, 0x36c, nullptr, 0x0, 0 },
    { "TacBomberSpeedVector", 4, 0x370, nullptr, 0x0, 0 },
    { "TacBomberAngleY", 3, 0x37c, nullptr, 0x0, 0 },
    { "TacBomberTurnWheel", 3, 0x380, nullptr, 0x0, 0 },
    { "Altitude", 3, 0x384, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8da7b0
const SVarDesc kDesc_8da7b0[] = {
    { "UnitName", 5, 0x0, nullptr, 0x0, 0 },
    { "StoredUnitName", 5, 0x8, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8da7f8
const SVarDesc kSBuildingUnitDesc[] = {
    { "SUnit", 6, 0x0, kSUnitDesc, 0x0, 0 },
    { "MemberPosArray", 10, 0x3fc, nullptr, 0x4, 2 },
    { "StoredAttackers", 10, 0x408, kDesc_8dd5c0, 0x18, 6 },
    { "MemberPosArray2", 10, 0x414, nullptr, 0x4, 2 },
    { "DeadBodies", 10, 0x420, nullptr, 0x4, 2 },
    { "ProductiveCounter", 2, 0x450, nullptr, 0x0, 0 },
    { "ModelDescentSpeed", 3, 0x35c, nullptr, 0x0, 0 },
    { "ModelDescentAcc", 3, 0x360, nullptr, 0x0, 0 },
    { "ActiveBuildingSide", 2, 0x3f8, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[0]", 1, 0x43c, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[1]", 1, 0x43d, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[2]", 1, 0x43e, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[3]", 1, 0x43f, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[4]", 1, 0x440, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[5]", 1, 0x441, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[6]", 1, 0x442, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[7]", 1, 0x443, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[8]", 1, 0x444, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[9]", 1, 0x445, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[10]", 1, 0x446, nullptr, 0x0, 0 },
    { "PlayersInsideHangar[11]", 1, 0x447, nullptr, 0x0, 0 },
    { "MultiBuilding", 10, 0x464, kDesc_8da7b0, 0x10, 6 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dc250
const SVarDesc kSProjectileUnitDesc[] = {
    { "SUnit", 6, 0x0, kSUnitDesc, 0x0, 0 },
    { "Damage", 3, 0x348, nullptr, 0x0, 0 },
    { "DamageRadius", 3, 0x34c, nullptr, 0x0, 0 },
    { "WeaponType", 2, 0x350, nullptr, 0x0, 0 },
    { "AttackerUnit", 2, 0x354, nullptr, 0x0, 0 },
    { "E3Hack_BecsapodasEffect", 1, 0x358, nullptr, 0x0, 0 },
    { "FireProjectile", 1, 0x359, nullptr, 0x0, 0 },
    { "StartPos", 4, 0x35c, nullptr, 0x0, 0 },
    { "Range", 3, 0x368, nullptr, 0x0, 0 },
    { "TargetUnit", 2, 0x344, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dddf0
const SVarDesc kSWasterUnitDesc[] = {
    { "SUnit", 6, 0x0, kSUnitDesc, 0x0, 0 },
    { "TargetUnit", 2, 0x344, nullptr, 0x0, 0 },
    { "Started", 2, 0x348, nullptr, 0x0, 0 },
    { "LifeTime", 2, 0x34c, nullptr, 0x0, 0 },
    { "AttackerUnit", 2, 0x35c, nullptr, 0x0, 0 },
    { "AttackerUnitLevel", 2, 0x360, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dc388
const SVarDesc kSTrainUnitDesc[] = {
    { "SSingleUnit", 6, 0x0, kSSingleUnitDesc, 0x0, 0 },
    { "RoadIdx", 2, 0x3ac, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dc038
const SVarDesc kDesc_8dc038[] = {
    { "Radius", 3, 0x0, nullptr, 0x0, 0 },
    { "DirFromCenter", 3, 0x4, nullptr, 0x0, 0 },
    { "Dir", 3, 0x8, nullptr, 0x0, 0 },
    { "UseDir", 1, 0xc, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dc0b0
const SVarDesc kSPanzersSquadUnitDesc[] = {
    { "SUnit", 6, 0x0, kSUnitDesc, 0x0, 0 },
    { "FormationType", 2, 0x348, nullptr, 0x0, 0 },
    { "RelativePosArray", 10, 0x34c, kDesc_8dc038, 0x10, 6 },
    { "RelativePosDir", 3, 0x358, nullptr, 0x0, 0 },
    { "AutoHeal", 1, 0x35c, nullptr, 0x0, 0 },
    { "SquadStop", 1, 0x390, nullptr, 0x0, 0 },
    { "MembersRadiusMax", 3, 0x394, nullptr, 0x0, 0 },
    { "MembersRadiusMin", 3, 0x398, nullptr, 0x0, 0 },
    { "MemberDelay", 10, 0x384, nullptr, 0x4, 2 },
    { "SoldierState_Keeping", 2, 0x39c, nullptr, 0x0, 0 },
    { "WalkStateSwitchBack", 1, 0x3a0, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dbff0
const SVarDesc kSPanzersSquadMemberUnitDesc[] = {
    { "SUnit", 6, 0x0, kSUnitDesc, 0x0, 0 },
    { "ParachuteTimer", 2, 0x34c, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dbaf8
const SVarDesc kSGunnerDesc[] = {
    { "Enabled", 2, 0x1c, nullptr, 0x0, 0 },
    { "BulletCounter", 2, 0x24, nullptr, 0x0, 0 },
    { "Ammo", 3, 0x20, nullptr, 0x0, 0 },
    { "AimOnly", 1, 0x28, nullptr, 0x0, 0 },
    { "TurretDir_Stand", 3, 0x2c, nullptr, 0x0, 0 },
    { "TurretDir", 3, 0x30, nullptr, 0x0, 0 },
    { "SpinGear", 2, 0x34, nullptr, 0x0, 0 },
    { "SpinChanging", 2, 0x38, nullptr, 0x0, 0 },
    { "SpinInertia", 1, 0x3c, nullptr, 0x0, 0 },
    { "ActualTargetDir", 3, 0x40, nullptr, 0x0, 0 },
    { "Vertical_TurretDir_Stand", 3, 0x44, nullptr, 0x0, 0 },
    { "Vertical_TurretDir", 3, 0x48, nullptr, 0x0, 0 },
    { "Vertical_SpinGear", 2, 0x4c, nullptr, 0x0, 0 },
    { "Vertical_SpinChanging", 2, 0x50, nullptr, 0x0, 0 },
    { "Vertical_SpinInertia", 1, 0x54, nullptr, 0x0, 0 },
    { "Vertical_ActualTargetDir", 3, 0x58, nullptr, 0x0, 0 },
    { "Loaded", 1, 0x5c, nullptr, 0x0, 0 },
    { "StartShotCounter", 2, 0x60, nullptr, 0x0, 0 },
    { "ReloadCounter", 2, 0x64, nullptr, 0x0, 0 },
    { "ShotDelayCounter", 2, 0x68, nullptr, 0x0, 0 },
    { "BurstShotCounter", 2, 0x6c, nullptr, 0x0, 0 },
    { "TurretKickCounter", 2, 0x70, nullptr, 0x0, 0 },
    { "TurretSlideBack", 3, 0x74, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dabd0
const SVarDesc kDesc_8dabd0[] = {
    { "FillUp", 1, 0x0, nullptr, 0x0, 0 },
    { "ReachedTarget", 1, 0x1, nullptr, 0x0, 0 },
    { "DontFillUpMoreInThisServerFrame", 1, 0x2, nullptr, 0x0, 0 },
    { "DontFollowGhostInThisServerFrame", 1, 0x3, nullptr, 0x0, 0 },
    { "StopUntilMembersChangeGlobalState", 1, 0x4, nullptr, 0x0, 0 },
    { "CreateNewPathIfEmpty", 1, 0x5, nullptr, 0x0, 0 },
    { "ManoeuvreOpeningFrame", 2, 0x8, nullptr, 0x0, 0 },
    { "ManoeuvreClosingFrame", 2, 0xc, nullptr, 0x0, 0 },
    { "FirstFrameNeeded", 1, 0x10, nullptr, 0x0, 0 },
    { "WaitAfterStop", 2, 0x14, nullptr, 0x0, 0 },
    { "DontReleaseTargetAfterStop", 1, 0x18, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dab40
const SVarDesc kDesc_8dab40[] = {
    { "Radius", 3, 0x0, nullptr, 0x0, 0 },
    { "NewArrival", 1, 0x4, nullptr, 0x0, 0 },
    { "Best_SQR_Distance", 3, 0x8, nullptr, 0x0, 0 },
    { "Best_Direction_Diff", 3, 0xc, nullptr, 0x0, 0 },
    { "NewArrivalDirection", 3, 0x10, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dbf90
const SVarDesc kDesc_8dbf90[] = {
    { "Pos", 8, 0x0, nullptr, 0x0, 0 },
    { "Dir", 3, 0x8, nullptr, 0x0, 0 },
    { "Type", 2, 0xc, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dbd38
const SVarDesc kDesc_8dbd38[] = {
    { "Point", 6, 0x0, kDesc_8dbf90, 0x0, 0 },
    { "MoveBackward", 1, 0x10, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dbd80
const SVarDesc kDesc_8dbd80[] = {
    { "Type", 2, 0x0, nullptr, 0x0, 0 },
    { "Priority", 2, 0x4, nullptr, 0x0, 0 },
    { "Points", 10, 0x8, kDesc_8dbd38, 0x14, 6 },
    { "InPoint", 8, 0x14, nullptr, 0x0, 0 },
    { "InPointDist", 3, 0x1c, nullptr, 0x0, 0 },
    { "OutPoint", 8, 0x20, nullptr, 0x0, 0 },
    { "DebugCircle1Center", 8, 0x28, nullptr, 0x0, 0 },
    { "DebugCircle2Center", 8, 0x30, nullptr, 0x0, 0 },
    { "DebugName", 5, 0x38, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dbe70
const SVarDesc kDesc_8dbe70[] = {
    { "ManoeuvresArray", 10, 0x0, kDesc_8dbd80, 0x40, 6 },
    { "ManoeuvresArrayFlag", 2, 0xc, nullptr, 0x0, 0 },
    { "MoveBackward", 1, 0x10, nullptr, 0x0, 0 },
    { "TurningRadius", 3, 0x14, nullptr, 0x0, 0 },
    { "Pos", 8, 0x18, nullptr, 0x0, 0 },
    { "PointType", 2, 0x20, nullptr, 0x0, 0 },
    { "PrevWayPointDirIn", 3, 0x24, nullptr, 0x0, 0 },
    { "NextWayPointPos", 8, 0x28, nullptr, 0x0, 0 },
    { "NextWayPointDirOut", 3, 0x30, nullptr, 0x0, 0 },
    { "DirIn", 3, 0x34, nullptr, 0x0, 0 },
    { "DirOut", 3, 0x38, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dacf0
const SVarDesc kSDriverDesc[] = {
    { "DesiredMoveSpeed", 3, 0x24, nullptr, 0x0, 0 },
    { "DesiredSpinSpeed", 3, 0x28, nullptr, 0x0, 0 },
    { "GhostInStoppingDistance", 1, 0x2c, nullptr, 0x0, 0 },
    { "LastCollidedUnitIdx", 2, 0x30, nullptr, 0x0, 0 },
    { "LastCollidedUnitIdxPrev", 2, 0x34, nullptr, 0x0, 0 },
    { "LastCollidedUnitCollisionPos", 8, 0x38, nullptr, 0x0, 0 },
    { "LastStaticCollisionPos", 4, 0x40, nullptr, 0x0, 0 },
    { "UnsuccessfulLocalPathCounter", 2, 0x4c, nullptr, 0x0, 0 },
    { "LastCollidedUnitIdxToMarkToBlockMapCounter", 2, 0x50, nullptr, 0x0, 0 },
    { "LastCollidedUnitIdxToMarkToBlockMap", 2, 0x54, nullptr, 0x0, 0 },
    { "GhostState", 6, 0x58, kDesc_8dabd0, 0x0, 0 },
    { "Arrival", 6, 0x74, kDesc_8dab40, 0x0, 0 },
    { "LocalWayPointsArray", 10, 0x88, kDesc_8dbe70, 0x3c, 6 },
    { "GlobalWayPointsArray", 10, 0x94, nullptr, 0x8, 8 },
    { "FullGlobalPathFound", 2, 0xa0, nullptr, 0x0, 0 },
    { "FullLocalPathFound", 2, 0xa4, nullptr, 0x0, 0 },
    { "GlobalPathTargetPoint", 4, 0xa8, nullptr, 0x0, 0 },
    { "OriginalTargetPoint", 4, 0xb4, nullptr, 0x0, 0 },
    { "TurningRadius", 3, 0xcc, nullptr, 0x0, 0 },
    { "SpinSpeedMultiplier", 3, 0xd0, nullptr, 0x0, 0 },
    { "MaxSpeedGear", 2, 0xd4, nullptr, 0x0, 0 },
    { "MaxSpinGear", 2, 0xd8, nullptr, 0x0, 0 },
    { "MaxGhostFrames", 2, 0xdc, nullptr, 0x0, 0 },
    { "LastGhostFollow", 2, 0xe0, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8daa20
const SVarDesc kSFlyingDriverDesc[] = {
    { "SDriver", 6, 0x0, kSDriverDesc, 0x0, 0 },
    { "Ground_Tracking", 1, 0xe8, nullptr, 0x0, 0 },
    { "DynamicFloor", 3, 0xec, nullptr, 0x0, 0 },
    { "DynamicSpeed", 3, 0xf0, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8daa98
const SVarDesc kSProjectileDriverDesc[] = {
    { "SDriver", 6, 0x0, kSDriverDesc, 0x0, 0 },
    { "Fuel", 2, 0xe8, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8daae0
const SVarDesc kSPanzersSquadMemberDriverDesc[] = {
    { "SDriver", 6, 0x0, kSDriverDesc, 0x0, 0 },
    { "MoveSpeedCounter", 3, 0xec, nullptr, 0x0, 0 },
    { "MoveSpeedCounterDelta", 3, 0xf0, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dd890
const SVarDesc kSTargetDesc[] = {
    { "Type", 2, 0x4, nullptr, 0x0, 0 },
    { "UnitIdx", 2, 0x8, nullptr, 0x0, 0 },
    { "SquadIdx", 2, 0xc, nullptr, 0x0, 0 },
    { "Point", 4, 0x10, nullptr, 0x0, 0 },
    { "Dir", 3, 0x1c, nullptr, 0x0, 0 },
    { "TargetRange", 2, 0x20, nullptr, 0x0, 0 },
    { "MoveBackward", 1, 0x2c, nullptr, 0x0, 0 },
    { "Task", 2, 0x24, nullptr, 0x0, 0 },
    { "TaskValue", 2, 0x28, nullptr, 0x0, 0 },
    { "MoveBackward", 1, 0x2c, nullptr, 0x0, 0 },
    { "PathIdx", 2, 0x30, nullptr, 0x0, 0 },
    { "PathNodeIdx", 2, 0x34, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dba58
const SVarDesc kLogicVarsDesc[] = {
    { "PlaySpeed", 2, 0x4, nullptr, 0x0, 0 },
    { "FrameCount", 2, 0x8, nullptr, 0x0, 0 },
    { "StartAnim", 5, 0xc, nullptr, 0x0, 0 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// HD 0x8dde98 (the AIGP element; the same list as aigroup.cpp's loader table)
const SVarDesc kAIGroupSaveDesc[] = {
    { "ID", 5, 0x0, nullptr, 0x0, 0 },
    { "Tactic", 2, 0x8, nullptr, 0x0, 0 },
    { "Status", 2, 0xc, nullptr, 0x0, 0 },
    { "Timer", 2, 0x10, nullptr, 0x0, 0 },
    { "AttackMoveTimer", 2, 0x14, nullptr, 0x0, 0 },
    { "PathIdx", 2, 0x34, nullptr, 0x0, 0 },
    { "HasCalledForHelp", 1, 0x18, nullptr, 0x0, 0 },
    { "StartPos", 8, 0x1c, nullptr, 0x0, 0 },
    { "AttackTarget", 1, 0x24, nullptr, 0x0, 0 },
    { "TargetPos", 8, 0x28, nullptr, 0x0, 0 },
    { "MaxHelpRange", 8, 0x30, nullptr, 0x0, 0 },
    { "DisabledAIGroups", 10, 0x44, nullptr, 0x4, 2 },
    { nullptr, 0, 0, nullptr, 0, 0 },
};

// The class records the GetClassDescriptor slots hand out (unit.h
// SUnitClassDesc) mapped to their descriptor lists.
const SVarDesc* SaveDescForClassRecord(unsigned hdAddr)
{
    switch (hdAddr) {
    case 0x8dc540: return kSUnitDesc;
    case 0x8dc358: return kSSingleUnitDesc;
    case 0x8daf48: return kSFlyingUnitDesc;
    case 0x8da7f8: return kSBuildingUnitDesc;
    case 0x8dc250: return kSProjectileUnitDesc;
    case 0x8dddf0: return kSWasterUnitDesc;
    case 0x8dc388: return kSTrainUnitDesc;
    case 0x8dc0b0: return kSPanzersSquadUnitDesc;
    case 0x8dbff0: return kSPanzersSquadMemberUnitDesc;
    case 0x8dbaf8: return kSGunnerDesc;
    case 0x8dacf0: return kSDriverDesc;
    case 0x8daa20: return kSFlyingDriverDesc;
    case 0x8daa98: return kSProjectileDriverDesc;
    case 0x8daae0: return kSPanzersSquadMemberDriverDesc;
    default: return nullptr;
    }
}

// Class records of the slots that the recompile adds (HD 0x5ace70,
// 0x55cec0, 0x5a3930, 0x553200, 0x553240).
const SUnitClassDesc kUnitClassDesc_8dc358 = { "SUnit", &kUnitClassDesc_8dc540, 0x8dc358 };
const SUnitClassDesc kUnitClassDesc_8daf48 = { "SUnit", &kUnitClassDesc_8dc540, 0x8daf48 };
const SUnitClassDesc kUnitClassDesc_8dc250 = { "SUnit", &kUnitClassDesc_8dc540, 0x8dc250 };
const SUnitClassDesc kDriverClassDesc_8daa20 = { "SDriver", nullptr, 0x8daa20 };
const SUnitClassDesc kDriverClassDesc_8daa98 = { "SDriver", nullptr, 0x8daa98 };

// PANZERS 0x5ace70
void SSingleUnit::GetClassDescriptor(void** obj, const SUnitClassDesc** desc)
{
    *obj = this;
    *desc = &kUnitClassDesc_8dc358;
}

// PANZERS 0x55cec0
void SFlyingUnit::GetClassDescriptor(void** obj, const SUnitClassDesc** desc)
{
    *obj = this;
    *desc = &kUnitClassDesc_8daf48;
}

// PANZERS 0x5a3930
void SProjectileUnit::GetClassDescriptor(void** obj, const SUnitClassDesc** desc)
{
    *obj = this;
    *desc = &kUnitClassDesc_8dc250;
}

// PANZERS 0x553200
void SFlyingDriver::GetClassDescriptor(void** obj, const SUnitClassDesc** desc)
{
    *obj = this;
    *desc = &kDriverClassDesc_8daa20;
}

// PANZERS 0x553240
void SProjectileDriver::GetClassDescriptor(void** obj, const SUnitClassDesc** desc)
{
    *obj = this;
    *desc = &kDriverClassDesc_8daa98;
}

} // namespace pz
