// src/game/manoeuvre.h
// SWayPointWithManoeuvres (HD 0x58cbb0..0x590e00): the candidate ways
// ("manoeuvres") a driver can pass one local waypoint with, given the
// driver type, the turning radius and the ghost frame it starts from.
// OWNER: P. The structs are in drivertypes.h.
//
// Manoeuvre types (+0x00, the debug names HD stores at +0x38):
//   1 TurnInPlace_Out___Turn_To_Dir_Of_Next_Waypoint            0x590690
//   2 TurnInPlace_Out___Turn_To_X_Degree_Difference_To_Dir_Of_Next_Waypoint 0x590800
//   3 TurnInPlace_In___Arrive_At_Waypoint_And_If_Necessary_Turn_To_Final_Direction 0x590430 / 0x58dd20
//   5 TurnInAngle_Out___Departure_With_Turning_Radius            0x58f780
//   6 TurnInAngle_Out___Departure_With_Y_Turn                    0x58f9b0
//   7 TurnInAngle_In___Arrive_At_Waypoint                        0x58f100
//   8 TurnInAngle_In___Arrive_At_Waypoint_With_Turning_Radius    0x58f200
//   9 TurnInAngle_In___Arrive_At_Waypoint_With_Turning_Radius_With_Y_Turn 0x58f4c0
//  10 TurnInAngle_InAndOut___Through_Waypoint_With_Turning_Radius 0x58e500
//  11 TurnInAngle_InAndOut___Through_Waypoint_With_YTurn         0x58ec70
//  12 ..._With_Turning_Radius_With_Outer_Points                  0x58e860
//  13 TurnInAngle___Through_Next_Waypoint                        0x58fce0
//  14 StopWP 0x58daa0, 15 FollowWP 0x58d3f0, 16 Simple_Out_Waypoint 0x58e1e0,
//  17 Simple_In_Waypoint 0x58e0f0.

#ifndef PZ_GAME_MANOEUVRE_H
#define PZ_GAME_MANOEUVRE_H

#include "drivertypes.h"

namespace pz {

struct STarget;

// PANZERS 0x58d4e0 SWayPointWithManoeuvres::CreateManoeuvres(pdriver,
// target, radius, frame). driverType = SPDriver +0x04 (0 TurnInPlace,
// 1 TurnInAngle, 3, 5, 10 squad, 13 train).
void Wpm_CreateManoeuvres(SWayPointWithManoeuvres* w, int driverType, STarget* target,
                          float radius, const SGhostFrame* frame);
// PANZERS 0x58e050: drops the manoeuvres whose Dist2 exceeds the distance
// from (x, z) to the waypoint, then 0x58cc90 (common start point).
void Wpm_FilterByStart(SWayPointWithManoeuvres* w, float x, float z);
// PANZERS 0x58dba0: lowest Index among the flagged manoeuvres; clears its
// flag; -1 when none.
int  Wpm_PopBest(SWayPointWithManoeuvres* w);
// PANZERS 0x58d2f0: SDArray<SManoeuvre>::Clear(n) (frees each manoeuvre).
void Wpm_ClearManoeuvres(SWayPointWithManoeuvres* w, int n);
// PANZERS 0x54fc60: element destructor (frees the manoeuvres and the array).
void Wpm_Free(SWayPointWithManoeuvres* w);
// PANZERS 0x58daa0: adds a "StopWP" manoeuvre at the waypoint (reverse =
// unit +0xcc).
void Wpm_AddStop(SWayPointWithManoeuvres* w, bool unitReverse);

} // namespace pz

#endif // PZ_GAME_MANOEUVRE_H
