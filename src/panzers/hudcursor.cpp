// src/panzers/hudcursor.cpp
// The game view's mouse cursor over the map: the hover cursor 0x621540 and
// the cursor colour (SWidget +0x40, set with the glyph by 0x543970). OWNER:
// M3-P (cursor parts of the game view).
//
// HD keeps the colour in the widget (SWidget is 0x48 in HD, 0x44 in the
// SWINE base: +0x40 is that colour) and SDXWindow::OnIdle 0x53a0d0 passes
// it to board +0x9c with the glyph; the D3D hardware cursor (+0xc4 0x6ca4e0)
// tints the glyph with it, the software cursor (0x6ca240) does not. The
// recompile keeps the game view's colour here, hands it to the board in the
// HUD update when the game view owns the cursor, and the board tints its
// (always software) cursor when options.ini asks for the hardware cursor.

#include <windows.h>
#include "hud.h"
#include "gameview.h"
#include "settings.h"
#include "pzboard.h"
#include "board.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"
#include "unit.h"
#include "punit.h"
#include "singleunit.h"
#include "buildingunit.h"
#include "squadunit.h"
#include "packets.h"
#include "blockmap.h"
#include "iviewport.h"

namespace {

unsigned s_ViewCursorColor = 0xffffffffu;   // the game view's SWidget +0x40

// The relation colours of 0x621540 / 0x620bc0 case 4 (0x56d280).
unsigned RelationColor(int unit)
{
    switch (pz::GetUnitRelationToLocal(unit)) {
    case -1: return 0xffff0000u;   // enemy
    case 1:  return 0xff00ff00u;   // own
    case 3:  return 0xff00ffffu;   // not to be ordered
    default: return 0xffffff00u;   // allied / neutral
    }
}

pz::SUnit* Target(int unit)
{
    pz::SWorld* w = pz::g_World;
    return w && w->Units.IsLive(unit) ? w->Units.Array[unit].Unit : nullptr;
}

} // namespace

namespace pz {

// PANZERS 0x5ba280
// The order a unit would take on the target (vtbl +0xa8); 10 (a building)
// becomes the building action 0x5ba2b0.
int UnitActionOn(pz::SUnit* u, int target)
{
    int k = u->ActionOn(target);                                   // vtbl +0xa8
    if (k == 10)
        k = u->GetBuildingAction(target);                          // 0x5ba2b0
    return k;
}

// PANZERS 0x56d490
// The order the selected units (+0x104 bit 0) would take on the target:
// -1 with no selected unit taking one, 0 when they differ.
int SelectionActionOn(int target)
{
    pz::SWorld* w = pz::g_World;
    int kind = -1;
    for (int i = 0; i < w->Units.Size; ++i) {
        if (!w->Units.IsLive(i))
            continue;
        pz::SUnit* u = w->Units.Array[i].Unit;
        if ((*((const unsigned char*)u + 0x104) & 1) == 0)
            continue;
        int k = UnitActionOn(u, target);
        if (k == -1 || k == 0)
            continue;
        if (kind == -1)
            kind = k;
        else if (kind != k)
            return 0;
    }
    return kind;
}

} // namespace pz

// PANZERS 0x543970 (the colour half; the glyph is SWidget::Cursor)
void PzViewSetCursor(SGameView* view, int cursor, unsigned color)
{
    view->Cursor = cursor;
    s_ViewCursorColor = color;
}

// PANZERS 0x621540
// The cursor over the map with no mouse mode: over a unit (pick 0x5ebac0,
// highlighted with 0x5fcb10(unit, 0x21)) the glyph of the selection's order
// on it and the colour of its relation to the local player; over the ground
// glyph 9 where the static block map (0x5d9e00, size 1, mask 0xb) blocks
// the point and 0 elsewhere. The ground point is MouseCamera's ray
// (0x5ea910), cast here.
void PzHoverCursor(SGameView* view, int x, int y)
{
    pz::SWorld* w = view->World;
    if (!w || !view->Viewport) {
        PzViewSetCursor(view, 0, 0xffffffffu);
        return;
    }
    float ray[6] = { 0, 0, 0, 0, -1.0f, 0 };
    view->Viewport->ScreenToRay(ray, x, y);                       // viewport +0x34
    int unit = w->PickAnyUnitAt(ray);                              // 0x5ebac0
    w->SelectUnit(unit, 0x21);                                     // 0x5fcb10
    if (unit < 0) {
        float tx, ty, tz;
        w->RayTerrain(ray, &tx, &ty, &tz);                         // 0x5ea910 (0x620c1d)
        bool blocked = pz::BlockMap_CheckStatic(w, tx, tz, 1, 0xb);   // 0x5d9e00
        PzViewSetCursor(view, blocked ? 9 : 0, 0xffffffffu);
        return;
    }
    int cursor = 0;
    switch (pz::SelectionActionOn(unit)) {                             // 0x56d490
    case 1: case 2: cursor = 5; break;
    case 3: cursor = 1; break;
    case 4: cursor = 8; break;
    case 5: case 6: cursor = 7; break;
    case 7: cursor = 3; break;
    case 8: cursor = 2; break;
    case 9: cursor = 6; break;
    }
    PzViewSetCursor(view, cursor, RelationColor(unit));
}

// PANZERS 0x620bc0 (piece: case 4 at 0x620d7c, the target cursor 0x14 and its colour)
void PzTargetCursor(SGameView* view, int unit)
{
    PzViewSetCursor(view, 0x14, unit < 0 ? 0xffffffffu : RelationColor(unit));
}

// SDXWindow::OnIdle 0x53a0d0 hands board +0x9c the colour of the widget that
// owns the cursor (0x543340 walk); here: the game view's colour while the
// game view owns it, white otherwise. Only the hardware cursor shows it.
void PzCursorColorUpdate(SGameView* view)
{
    SWidget* owner = SWidget::LastMouseTarget;
    while (owner && owner->Cursor < 0)
        owner = owner->Parent;
    unsigned c = (owner && owner == static_cast<SWidget*>(view)) ? s_ViewCursorColor : 0xffffffffu;
    Board->SetCursorColor(Settings.HardwareMouseCursor ? c : 0xffffffffu);
}

void PzCursorColorReset()
{
    s_ViewCursorColor = 0xffffffffu;
    if (Board)
        Board->SetCursorColor(0xffffffffu);
}
