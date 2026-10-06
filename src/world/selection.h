// src/world/selection.h
// Selection, picking and the selection set (M3). The SWorld functions are
// declared in world.h (block "M3"); this header holds the unit fields they
// use and the selection helpers the view and the packet builders share.
//
// HD facts (docs/re/M3_SCOPE.md §3 and the builders 0x575610..):
//   - A unit is selected when (unit +0x104) & 1. WriteSelectedUnits 0x576130
//     sends every live unit of the heap (World+0x4d4, heap order) with that
//     bit; there is no separate selection list in the packets. Every change
//     of the bit made by the SWorld selection functions sends packet 0x32
//     (select) or 0x33 (deselect) when a game logic exists; ProcessPacket
//     then sets unit +0x108 bit `player` (hashed into the world CRC) and
//     calls unit +0x148 / +0x14c.
//   - Pick 0x5fc050 filters on unit player (+0xfc) == World+0x16c
//     (LocalPlayer) or an allied player (World+0x178 + p * 0x48 == 4), unit
//     +0x110 clear, +0x112 (selectable), +0x150 (wrecked), +0x168 (stored).
//   - SGameView: OnMouseDown 0x624a70 (mode +0x478, press position
//     +0x47c/+0x480, double-click time +0x4a0/+0x4a4, double-click pick
//     +0x4a8), OnMouseUp 0x6251f0 (box select 0x5fc5b0), groups in
//     OnKeyDown 0x622f50.
//
// SHARED HEADER (owner P0; names and bodies: agent O, src/world/selection.cpp).

#ifndef PZ_SELECTION_H
#define PZ_SELECTION_H

#include "world.h"

namespace pz {

struct SIViewport;

enum PzUnitSelectionField : unsigned {
    kUnitSelectedFlags = 0x104,   // bit 0 = selected (0x576130 TEST byte [unit+0x104], 1)
    kUnitSelectable    = 0x112,   // byte (pick filter)
    kUnitHidden        = 0x150,   // (pick filter)
};

// Selection modes of the SWorld selection functions (world.h; names guessed).
enum PzSelectMode : unsigned {
    PZ_SEL_CLEAR   = 0x00,        // every unit loses the bit
    PZ_SEL_KEEP    = 0x01,        // nothing changes (0x5fc860(1): right button pressed)
    PZ_SEL_ADD     = 0x05,        // keep, and the hit units are selected (Shift + box)
    PZ_SEL_ONLY    = 0x10,        // only the hit units
    PZ_SEL_TOGGLE  = 0x11,        // keep, the hit units toggle (Shift + click)
};

// Recompile stand-in for the 0x40-byte frustum viewport +0x38 (0x6898b0)
// builds from a screen rectangle (the four side planes through the camera).
// Viewport +0x38 is not lifted (agent E), so the box tests project the unit
// position with viewport +0x3c (ProjectToScreen) into this rectangle instead
// of testing the model against the planes (model +0xb8, 0x6d5ae0).
struct SPzSelectRect {
    SIViewport* Viewport;
    int X0, Y0, X1, Y1;           // screen pixels, any order
};

// Raw access until agent C names the SUnit fields (unit.h).
inline bool UnitIsSelected(const void* unit)
{
    return (((const unsigned char*)unit)[kUnitSelectedFlags] & 1) != 0;
}

} // namespace pz

#endif // PZ_SELECTION_H
