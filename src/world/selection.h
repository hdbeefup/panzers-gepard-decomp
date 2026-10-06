// src/world/selection.h
// Selection, picking and the selection set (M3). The SWorld functions are
// declared in world.h (block "M3"); this header holds the unit fields they
// use and the selection helpers the view and the packet builders share.
//
// HD facts (static, docs/re/M3_SCOPE.md §3 and the builders 0x575610..):
//   - A unit is selected when (unit +0x104) & 1. WriteSelectedUnits 0x576130
//     sends every live unit of the heap (World+0x4d4, heap order) with that
//     bit; there is no separate selection list in the packets.
//   - Pick 0x5fc050 filters on unit player (+0xfc) == World+0x16c
//     (LocalPlayer), allies, unit +0x112 (selectable) and +0x150 (hidden).
//   - SGameView: OnMouseDown 0x624a70 (mode +0x478, press position
//     +0x47c/+0x480, double-click time +0x4a0/+0x4a4, double-click pick
//     +0x4a8), OnMouseUp 0x6251f0 (box select 0x5fc5b0), Ctrl+n groups in
//     OnKeyDown 0x622f50.
//
// SHARED HEADER (owner P0; names and bodies: agent V, src/world/selection.cpp).

#ifndef PZ_SELECTION_H
#define PZ_SELECTION_H

#include "world.h"

namespace pz {

enum PzUnitSelectionField : unsigned {
    kUnitSelectedFlags = 0x104,   // bit 0 = selected (0x576130 TEST byte [unit+0x104], 1)
    kUnitSelectable    = 0x112,   // byte (pick filter)
    kUnitHidden        = 0x150,   // (pick filter)
};

// Raw access until agent C names the SUnit fields (unit.h).
inline bool UnitIsSelected(const void* unit)
{
    return (((const unsigned char*)unit)[kUnitSelectedFlags] & 1) != 0;
}

} // namespace pz

#endif // PZ_SELECTION_H
