// src/game/unitsave.h
// The generic variable writer of the save stream (gSaveVariables 0x670c50,
// the counterpart of gLoadVariables 0x670440 in pzunitregistry.h) and the
// descriptor tables it is used with. OWNER: agent F (docs/M3_INTERFACES.md).

#ifndef PZ_UNITSAVE_H
#define PZ_UNITSAVE_H

#include "pzunitregistry.h"

struct SStream;

namespace pz {

// HD 0x670c50: for every descriptor entry {int type, u16 name, value}, then
// int 0. Value formats (0x670af0): 1 byte, 2 int, 3 float, 4 3 floats,
// 5 u16 string, 6 '_mcl' chunk with the member list, 8 2 floats, 12 word;
// 10 {elem type, count, elements}; 11 {elem type, count, first, last, elements}.
void SaveVariables(SStream* s, const void* obj, const SVarDesc* desc);              // 0x670c50
void SaveSingleVariable(SStream* s, const void* src, int type, const SVarDesc* members);   // 0x670af0

extern const SVarDesc kUnitDefDesc[];   // HD 0x8ddb48 (unitregistry.cpp)

// savedesc.cpp (agent S, M4): the HD descriptor lists.
extern const SVarDesc kSUnitDesc[];                      // 0x8dc540
extern const SVarDesc kSSingleUnitDesc[];                // 0x8dc358
extern const SVarDesc kSFlyingUnitDesc[];                // 0x8daf48
extern const SVarDesc kSBuildingUnitDesc[];              // 0x8da7f8
extern const SVarDesc kSProjectileUnitDesc[];            // 0x8dc250
extern const SVarDesc kSWasterUnitDesc[];                // 0x8dddf0
extern const SVarDesc kSTrainUnitDesc[];                 // 0x8dc388
extern const SVarDesc kSPanzersSquadUnitDesc[];          // 0x8dc0b0
extern const SVarDesc kSPanzersSquadMemberUnitDesc[];    // 0x8dbff0
extern const SVarDesc kSGunnerDesc[];                    // 0x8dbaf8
extern const SVarDesc kSDriverDesc[];                    // 0x8dacf0
extern const SVarDesc kSFlyingDriverDesc[];              // 0x8daa20
extern const SVarDesc kSProjectileDriverDesc[];          // 0x8daa98
extern const SVarDesc kSPanzersSquadMemberDriverDesc[];  // 0x8daae0
extern const SVarDesc kSTargetDesc[];                    // 0x8dd890
extern const SVarDesc kLogicVarsDesc[];                  // 0x8dba58 (SGameLogic VARS)
extern const SVarDesc kAIGroupSaveDesc[];                // 0x8dde98 (aigroup.cpp's table, for AIGP)

// The descriptor list of a class record (SUnitClassDesc::HdAddr), or null.
const SVarDesc* SaveDescForClassRecord(unsigned hdAddr);

struct SUnitClassDesc;
extern const SUnitClassDesc kUnitClassDesc_8dc358;     // SSingleUnit 0x5ace70
extern const SUnitClassDesc kUnitClassDesc_8daf48;     // SFlyingUnit 0x55cec0
extern const SUnitClassDesc kUnitClassDesc_8dc250;     // SProjectileUnit 0x5a3930
extern const SUnitClassDesc kDriverClassDesc_8daa20;   // SFlyingDriver 0x553200
extern const SUnitClassDesc kDriverClassDesc_8daa98;   // SProjectileDriver 0x553240

} // namespace pz

#endif // PZ_UNITSAVE_H
