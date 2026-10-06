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

} // namespace pz

#endif // PZ_UNITSAVE_H
