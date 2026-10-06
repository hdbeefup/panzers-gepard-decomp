// src/panzers/gameview_input.cpp
// SGameView input: mouse and keys, the order dispatcher 0x61e740 and the
// order hotkeys 0x6195b0. OWNER: agent O (docs/M3_INTERFACES.md §4).
//
// Orders never touch the units: the dispatcher picks the target (unit or
// terrain point) and calls a packet builder (packets.h); the order takes
// effect when ProcessPacket runs the frame. Selection changes go through
// the SWorld selection functions (selection.cpp), which send packets 0x32 /
// 0x33.
//
// HD fields of the view are read at their HD offsets (Hd* helpers below;
// SGameViewData names the ones decoded). +0x5c..+0x45b is the key table:
// for each virtual key < 0x100 the time (ms) it went down, 0 when up;
// +0x9c is Shift (VK 0x10), +0xa0 Ctrl (VK 0x11).
//
// Recompile stand-ins (marked "stand-in"):
//  - the box select tests the unit's projected position in SPzSelectRect
//    (selection.h) instead of the model against the planes of viewport +0x38
//    (GetSelectionPlanes 0x6898b0): the model test +0xb8 (0x6d5ae0) is not
//    typed yet (agent E);
//  - (the left-button drag state +0x49c and the box are updated per frame by
//    SGameView::MouseCamera 0x620bc0 from Update 0x628430, agent V);
//  - the command buttons' state (+0x2b3d / +0x2c5d / +0x304d, SCommandButton,
//    agent H): while the HUD panel (+0xc68) does not exist, "a unit is
//    selected" stands for them.

#include <windows.h>
#include <math.h>
#include <string.h>
#include "gameview.h"
#include "superwindow.h"
#include "m3common.h"
#include "campaign.h"
#include "settings.h"
#include "stub_log.h"
#include "logger.h"
#include "timer.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/iviewport.h"
#include "worldapi.h"
#include "world.h"
#include "selection.h"
#include "blockmap.h"
#include "unit.h"
#include "punit.h"
#include "gamelogic.h"
#include "packets.h"

namespace {

// HD object offset -> field of the view (SGameViewData starts at HD +0x5c).
template <typename T>
T& Hd(SGameView* v, unsigned off)
{
    return *(T*)((unsigned char*)static_cast<SGameViewData*>(v) + (off - 0x5c));
}

bool ShiftDown(SGameView* v) { return v->KeyDownTime[VK_SHIFT] != 0; }      // +0x9c
bool CtrlDown(SGameView* v) { return v->KeyDownTime[VK_CONTROL] != 0; }     // +0xa0
int  CtrlTime(SGameView* v) { return (int)v->KeyDownTime[VK_CONTROL]; }     // +0xa0 as the builders' "force"

// HD 0x661800: the timer in seconds.
double Seconds()
{
    return (double)Timer.GetTickValue() / 1000.0;
}

int Abs(int v) { return v < 0 ? -v : v; }

pz::SIViewport* ViewViewport(SGameView* v)
{
    if (v->Viewport)                                              // +0x468
        return (pz::SIViewport*)v->Viewport;
    return pz::PzGepard() ? pz::PzGepard()->GetViewport(0) : nullptr;
}

// Viewport +0x34 (0x689c20): origin = the eye, direction towards the screen
// point (agent E's SViewport::ScreenToRay).
void ScreenRay(pz::SIViewport* vp, int x, int y, float* ray)
{
    memset(ray, 0, 6 * sizeof(float));
    ray[4] = -1.0f;
    if (vp)
        vp->ScreenToRay(ray, x, y);                               // +0x34
}

// The terrain point under a screen point (viewport +0x34, then SWorld 0x5ea910).
void ScreenToTerrain(SGameView* v, int x, int y, float* tx, float* ty, float* tz)
{
    float ray[6];
    ScreenRay(ViewViewport(v), x, y, ray);
    v->World->RayTerrain(ray, tx, ty, tz);
}



// The command buttons' state bytes (agent H) or the stand-in.
bool CommandButtonOn(SGameView* v, unsigned off)
{
    if (v->PanelWidget)
        return Hd<unsigned char>(v, off) != 0;
    return v->World && v->World->CountSelectedUnits() > 0;
}

void StubOnce(const char* what)
{
    if (Logger.g)
        Logger.g->Log(1, "STUB: %s", what);
}

} // namespace

// PANZERS 0x61e740
// The order at screen point (x, y): `command` is the chosen command
// (+0x3884, 0 = the default order of the target), `force` (> 0: Ctrl or a
// double click) the builders' B1, Shift (+0x9c) their B2 (queue). A unit
// under the point takes the unit form of the command; otherwise the
// terrain point (48 m from the map edge at least): a move turns into a
// move with a facing when the button was dragged away from the press
// point (+0x484 / +0x48c) by 5 cm or more, the facing being the drag.
void SGameView::IssueOrder(int x, int y, int p3, int force, int command)
{
    PZ_M3_TRACE("SGameView::IssueOrder (0x61e740)");
    (void)p3;
    pz::SGameLogic* gl = this->Logic;
    if (!World || !gl)
        return;
    bool b1 = force > 0;
    bool b2 = ShiftDown(this);
    float ray[6];
    ScreenRay(ViewViewport(this), x, y, ray);
    int target = World->PickAnyUnitAt(ray);                       // 0x5ebac0
    if (target >= 0 && (MouseMode != 5 || (x == PressX && y == PressY)) && command != 0x15 && command != 0x16
        && command != 0x17 && command != 0x18 && command != 0x19 && command != 0x1a) {
        switch (command) {
        case 1:  pz::Pkt_Unit(gl, pz::PZ_PKT_07, target, b1, b2); return;               // 0x575b40
        case 2:  return;
        case 3:  pz::Pkt_ValueB(gl, pz::PZ_PKT_22, target, b2); return;                 // 0x5759c0
        case 4:  pz::Pkt_Unit(gl, pz::PZ_PKT_0F, target, b1, b2); return;               // 0x575510
        case 5:  pz::Pkt_Unit(gl, pz::PZ_PKT_10, target, b1, b2); return;               // 0x575560
        case 6:  pz::Pkt_Unit(gl, pz::PZ_PKT_12, target, b1, b2); return;               // 0x5756c0
        case 7:  pz::Pkt_Unit(gl, pz::PZ_PKT_15, target, b1, b2); return;               // 0x575670
        case 8:
        case 9:
        case 0xe: {
            pz::SUnit* u = pz::WorldUnit(target);                     // 0x546490
            pz::PzPacketOp op = command == 8 ? pz::PZ_PKT_16 : command == 9 ? pz::PZ_PKT_17 : pz::PZ_PKT_1A;
            pz::Pkt_Pos(gl, op, u->Pos[0], u->Pos[2], b1, b2);    // 0x575ae0 / 0x576300 / 0x575db0
            return;
        }
        case 0xf:  pz::Pkt_Unit(gl, pz::PZ_PKT_0A, target, b1, b2); return;             // 0x575d60
        case 0x10: pz::Pkt_ValueB(gl, pz::PZ_PKT_0C, target, b2); return;               // 0x576260
        case 0x11: pz::Pkt_ValueB(gl, pz::PZ_PKT_0B, target, b2); return;               // 0x5754d0
        case 0x12: pz::Pkt_ValueB(gl, pz::PZ_PKT_0D, target, b2); return;               // 0x5760c0
        case 0x13: pz::Pkt_ValueB(gl, pz::PZ_PKT_34, target, b2); return;               // 0x576360
        default:   pz::Pkt_TargetUnit(gl, target, b1, b2); return;                     // 0x575a60
        }
    }
    float tx, ty, tz;
    ScreenToTerrain(this, x, y, &tx, &ty, &tz);
    if (tx < 48.0f || tz < 48.0f)                                 // DAT_007f7f90
        return;
    if ((float)(World->TerrainW - 0x30) < tx || (float)(World->TerrainH - 0x30) < tz)
        return;
    if (tx < 0.0f)
        return;
    float dx = tx - PressWorld[0];
    float dz = tz - PressWorld[2];
    bool closeToPress = (double)(float)fabs((double)dx) < 0.05 && (double)(float)fabs((double)dz) < 0.05;   // DAT_007ea758
    float dir = (float)atan2((double)dx, (double)dz);             // 0x78d07a: atan2(ST1 = dx, ST0 = dz)
    switch (command) {
    case 1:
        if (closeToPress)
            pz::Pkt_Move(gl, tx, tz, b1, b2);                     // 0x575ef0
        else
            pz::Pkt_MoveDir(gl, PressWorld[0], PressWorld[2], dir, b1, b2);   // 0x575f50
        return;
    case 2:
        if (closeToPress)
            pz::Pkt_MoveBack(gl, tx, tz, b1, b2);                 // 0x575e10
        else
            pz::Pkt_MoveBackDir(gl, PressWorld[0], PressWorld[2], dir, b1, b2);   // 0x575e70
        return;
    case 3:    pz::Pkt_PosB(gl, pz::PZ_PKT_21, tx, tz, b2); return;                     // 0x575a00
    case 4:    pz::Pkt_Pos(gl, pz::PZ_PKT_09, tx, tz, b1, b2); return;                  // 0x575770
    case 5: case 6: case 7:
        return;
    case 8:    pz::Pkt_Pos(gl, pz::PZ_PKT_16, tx, tz, b1, b2); return;                  // 0x575ae0
    case 9:    pz::Pkt_Pos(gl, pz::PZ_PKT_17, tx, tz, b1, b2); return;                  // 0x576300
    case 0xe:  pz::Pkt_Pos(gl, pz::PZ_PKT_1A, tx, tz, b1, b2); return;                  // 0x575db0
    case 0x15: pz::Pkt_Pos(gl, pz::PZ_PKT_0E, tx, tz, b1, b2); return;                  // 0x575610
    case 0x16:
        if (!pz::BlockMap_CheckStatic(World, tx, tz, 1, 0x8000))
            pz::Pkt_Support(gl, pz::PZ_PKT_SUPPORT_1C, tx, tz);   // 0x575880
        return;
    case 0x17:
        pz::Pkt_Support(gl, pz::PZ_PKT_SUPPORT_1D, tx, tz);       // 0x576070
        return;
    case 0x18:
        if (!pz::BlockMap_CheckStatic(World, tx, tz, 1, 0x8000))
            pz::Pkt_Support(gl, pz::PZ_PKT_SUPPORT_1E, tx, tz);   // 0x5762b0
        return;
    case 0x19:
    case 0x1a: {
        if (pz::BlockMap_CheckStatic(World, tx, tz, 1, 0x8000))     // 0x5d9e00(x, z, 1, 0x8000)
            return;
        pz::PzPacketOp op = command == 0x19 ? pz::PZ_PKT_SUPPORT_1F : pz::PZ_PKT_SUPPORT_20;
        if (closeToPress)
            pz::Pkt_Support4(gl, op, tx, tz, 0, 0.0f);            // 0x575cb0 / 0x576000
        else
            pz::Pkt_Support4(gl, op, PressWorld[0], PressWorld[2], 1, dir);
        return;
    }
    default:
        if (MouseMode == 6) {
            // Turn the selected unit pressed on towards the release point.
            pz::SUnit* u = pz::WorldUnit(RotateUnit);                 // +0x3e70
            float rx = tx - u->Pos[0];
            float rz = tz - u->Pos[2];
            float a = (float)atan2((double)rx, (double)rz);
            pz::Pkt_FloatB(gl, pz::PZ_PKT_06, a, b2);             // 0x5763a0
            return;
        }
        if (closeToPress)
            pz::Pkt_Move(gl, tx, tz, b1, b2);
        else
            pz::Pkt_MoveDir(gl, PressWorld[0], PressWorld[2], dir, b1, b2);
        return;
    }
}

// PANZERS 0x624a70
void SGameView::OnMouseDown(int button, int x, int y, int shift)
{
    PZ_M3_TRACE("SGameView::OnMouseDown (0x624a70)");
    // HD 0x5435b0 / 0x544c20: window -> widget coordinates (the view fills the window).
    if (LoadingScreen) {
        // HD 0x624ac6: hide the "Click to continue" frame, SendAction(0x47561).
        LoadingScreen = false;
        if (LoadingFrame >= 0) {
            Board->DestroyFrame(LoadingFrame);
            LoadingFrame = -1;
        }
        SendAction(PZA_GV_MAP_LOADED, 0);
        return;
    }
    if (!World)                                                   // +0x3e40
        return;
    if (Modal != 0) {                                             // +0x389c
        StubOnce("SGameView::OnMouseDown modal box click 0x619520");
        return;
    }
    if (ViewState == 2)                                           // cut-scene
        return;
    // HD: the +0x3e74 list (World +0xe4 vtbl +0x64 per entry) is emptied
    // here; the recompile never fills it.
    WidgetCount = 0;
    double now = Seconds();
    if (button == 1) {
        if (MouseMode == 4) {
            int c = CommandMode;
            if (c == 1 || c == 2 || c == 0x19 || c == 0x1a) {
                // A directional command: the press point, the direction comes with the release.
                ScreenToTerrain(this, x, y, &PressWorld[0], &PressWorld[1], &PressWorld[2]);
                MouseMode = 5;
                PressWorld[1] = PressWorld[1] + 0.1f;             // DAT_007f59a8
            } else {
                IssueOrder(x, y, shift, CtrlTime(this), c);
                CommandMode = 0;
                MouseMode = 0;
                ReleaseMouse();                                   // 0x5437c0
            }
        } else {
            bool dbl = ClickTime[0] != 0.0f && Abs(PressX - x) <= 4 && Abs(PressY - y) <= 4
                       && !(0.75 < now - (double)ClickTime[0]);   // DAT_008043a8
            if (!dbl && !CtrlDown(this)) {
                PressX = x;
                PressY = y;
                Dragging = false;
                if (MouseMode != 7) {
                    MouseMode = 1;
                    ClickTime[0] = (float)now;
                    DoubleClickPick = -1;
                } else {
                    MouseMode = 2;
                }
            } else {
                // Ctrl+click or a double click: every unit of that type in view.
                unsigned mode = ShiftDown(this) ? pz::PZ_SEL_ADD : pz::PZ_SEL_ONLY;
                int unit;
                if (!dbl) {
                    float ray[6];
                    ScreenRay(ViewViewport(this), x, y, ray);
                    unit = World->PickUnitAt(ray, mode);          // 0x5fc050
                } else {
                    unit = DoubleClickPick;                       // +0x4a8
                }
                pz::SPzSelectRect box = { ViewViewport(this), 0, 0, Width, Height };   // viewport +0x38(0, 0, +0x1c, +0x20)
                World->SelectSameType(&box, unit, mode);          // 0x5fcd10
                ClickTime[0] = 0.0f;
                ClickTime[1] = 0.0f;
            }
        }
    } else if (button == 3) {
        World->HideSelectionBox();                                // 0x5ddb60
        int selected = World->CountSelectedUnits();               // 0x5e0d70
        if (MouseMode == 4) {
            MouseMode = 0;                                        // right button cancels the command
            ReleaseMouse();
        } else if (MouseMode == 1) {
            World->ApplySelectionToAll(pz::PZ_SEL_KEEP);          // 0x5fc860(1)
            PressX = x;
            PressY = y;
            MouseMode = 3;
        } else if (!CommandButtonOn(this, 0x2b3d) && !CommandButtonOn(this, 0x2c5d) && !CommandButtonOn(this, 0x304d)) {
            if (selected == 0 && MouseMode != 2) {
                PressX = x;
                PressY = y;
                MouseMode = 7;                                    // camera
                CommandMode = 0;
            }
        } else {
            float ray[6];
            ScreenRay(ViewViewport(this), x, y, ray);
            int unit = World->PickAnyUnitAt(ray);                 // 0x5ebac0
            int rel = pz::GetUnitRelationToLocal(unit);           // 0x56d280
            if (unit < 0 || rel != -1) {
                ScreenToTerrain(this, x, y, &PressWorld[0], &PressWorld[1], &PressWorld[2]);
                if (rel == 1 && unit >= 0 && World->IsUnitSelected(unit)) {
                    MouseMode = 6;
                    RotateUnit = unit;                            // +0x3e70
                } else {
                    MouseMode = 5;
                }
                CommandMode = 0;
                PressWorld[1] = PressWorld[1] + 0.1f;
            } else {
                // An enemy: the default order at once (a double click forces it).
                int force = CtrlTime(this);
                bool dbl = ClickTime[1] != 0.0f && Abs(PressX - x) <= 4 && Abs(PressY - y) <= 4
                           && !(0.75 < now - (double)ClickTime[1]);
                if (!dbl) {
                    ClickTime[1] = (float)now;
                    PressX = x;
                    PressY = y;
                } else {
                    force = 1;
                    ClickTime[0] = 0.0f;
                    ClickTime[1] = 0.0f;
                }
                CommandMode = 0;
                IssueOrder(x, y, shift, force, 0);
            }
        }
    } else if (button == 2) {
        World->HideSelectionBox();
        PressX = x;
        PressY = y;
        MouseMode = 2;
    }
    if (MouseMode != 0)
        CaptureMouse();                                           // 0x543300
}

// PANZERS 0x6251f0
void SGameView::OnMouseUp(int button, int x, int y, int shift)
{
    PZ_M3_TRACE("SGameView::OnMouseUp (0x6251f0)");
    (void)button;
    if (!World)
        return;
    if (MouseMode == 5 || MouseMode == 6) {
        double now = Seconds();
        int force = CtrlTime(this);
        bool dbl = ClickTime[1] != 0.0f && Abs(PressX - x) <= 4 && Abs(PressY - y) <= 4
                   && !(0.75 < now - (double)ClickTime[1]);
        if (!dbl) {
            ClickTime[1] = (float)now;
            PressX = x;
            PressY = y;
        } else {
            force = 1;
            ClickTime[0] = 0.0f;
            ClickTime[1] = 0.0f;
        }
        IssueOrder(x, y, shift, force, CommandMode);
        MouseMode = 0;
        CommandMode = 0;
        if (pz::g_Scene)
            pz::g_Scene->ClearLines();                            // scene +0xec
        WidgetCount = 0;                                          // the +0x3e74 list (see OnMouseDown)
    }
    if (MouseMode == 1) {
        World->HideSelectionBox();
        if (!Dragging) {
            unsigned mode = ShiftDown(this) ? pz::PZ_SEL_TOGGLE : pz::PZ_SEL_ONLY;   // (+0x9c != 0) + 0x10
            float ray[6];
            ScreenRay(ViewViewport(this), x, y, ray);
            DoubleClickPick = World->PickUnitAt(ray, mode);       // +0x4a8 = 0x5fc050
        } else {
            ClickTime[0] = 0.0f;
            ClickTime[1] = 0.0f;
            unsigned mode = ShiftDown(this) ? pz::PZ_SEL_ADD : pz::PZ_SEL_ONLY;
            pz::SPzSelectRect box = { ViewViewport(this), PressX, PressY, x, y };   // viewport +0x38
            World->SelectUnitsInBox(&box, mode);                  // 0x5fc5b0
        }
    }
    MouseMode = 0;
    ReleaseMouse();
}

// PANZERS 0x6250e0
void SGameView::OnMouseMove(int x, int y, int shift)
{
    PZ_M3_TRACE("SGameView::OnMouseMove (0x6250e0)");
    (void)shift;
    if (!World)
        return;
    MouseX = x;                                                   // +0x4ac
    MouseY = y;                                                   // +0x4b0
    // The drag box: MouseCamera 0x620bc0 from Update (gameview_view.cpp).
}

// PANZERS 0x625510
void SGameView::OnMouseWheel(int button, int x, int y, int delta)
{
    PZ_M3_TRACE("SGameView::OnMouseWheel (0x625510)");
    (void)x; (void)y; (void)delta;
    if (ViewState == 2 || !World)
        return;
    World->ZoomCamera((float)(button * -2));                      // 0x609390
}

// PANZERS 0x6246d0
bool SGameView::OnKeyUp(int key)
{
    PZ_M3_TRACE("SGameView::OnKeyUp (0x6246d0)");
    if ((unsigned)key < 0x100 && KeyDownTime[key] != 0) {
        if (World)                                                // +0x3e40
            KeyScroll(NowMs(), key);                              // 0x62c470(ftol(0x661800() * 1000), key)
        KeyDownTime[key] = 0;
    }
    return true;
}

// PANZERS 0x6195b0
// The configurable hotkeys (Settings.Hotkeys, keys%d.ini [Keyboard
// bindings]): pause and game speed, then the command keys, which need the
// command's button (agent H) to be enabled.
void SGameView::OnHotkey(int key)
{
    pz::SGameLogic* gl = this->Logic;
    const int* hk = Settings.Hotkeys;                             // DAT_00929ea4..
    bool missionOver = pz::g_Campaign && pz::g_Campaign->GetMissionResult() != 0;   // 0x5920b0
    if (key == hk[0]) {                                           // pause
        if (gl->Running != 0) {                                   // 0x56d190
            gl->SetRunning(0);
            return;
        }
    } else if (key == hk[1]) {                                    // normal speed
    } else if (key == hk[2]) {                                    // fast
        if (gl->Running != 2) {
            if (!missionOver)
                gl->SetRunning(2);
            return;
        }
    } else {
        struct SCommandKey { int Index; unsigned Button; int Command; };
        static const SCommandKey kCommands[] = {
            { 3, 0xfcd, 0x1a }, { 4, 0xf59, 0x19 }, { 5, 0xee5, 0x18 }, { 6, 0xdfd, 0x16 }, { 7, 0xe71, 0x17 },
            { 9, 0x2c5d, 4 }, { 11, 0x2bcd, 2 }, { 15, 0x2ced, 0x15 }, { 16, 0x2d7d, 0xf }, { 17, 0x2fbd, 0x11 },
            { 18, 0x2f2d, 0x12 }, { 19, 0x2e0d, 0x13 },
        };
        for (const SCommandKey& c : kCommands) {
            if (key == hk[c.Index] && CommandButtonOn(this, c.Button)) {
                MouseMode = 0;
                ReleaseMouse();
                MouseMode = 4;
                CommandMode = c.Command;
                if (c.Command == 4 || c.Command == 2)
                    StubOnce("SGameView::OnHotkey 0x537df0(1) (command sound)");
                return;
            }
        }
        if (key == hk[8] && CommandButtonOn(this, 0x2aad)) {
            pz::Pkt_Flag(gl, pz::PZ_PKT_08, ShiftDown(this));      // 0x576230 stop
            return;
        }
        if (key == hk[10] || key == hk[12] || key == hk[14]) {
            StubOnce("SGameView::OnHotkey 0x6280f0 (HUD buttons, agent H)");
            return;
        }
        if (key == hk[13] && CommandButtonOn(this, 0x30dd)) {
            pz::Pkt_TwoFlags(gl, pz::PZ_PKT_31, 0xff, ShiftDown(this));   // 0x5763f0(-1, shift)
            return;
        }
        if (key == hk[20] && CommandButtonOn(this, 0x2e9d)) {
            pz::Pkt_Flag(gl, pz::PZ_PKT_35, ShiftDown(this));      // 0x576460
            return;
        }
        for (int k = 0; k < 3; ++k) {
            if (key == hk[21 + k]) {
                pz::Pkt_TwoFlags(gl, pz::PZ_PKT_2D, (unsigned char)k, false);   // 0x575940(k, 0)
                return;
            }
            if (key == hk[24 + k]) {
                pz::Pkt_Flag(gl, pz::PZ_PKT_26, (unsigned char)k);            // 0x575910(k)
                return;
            }
        }
        return;
    }
    if (!missionOver)
        gl->SetRunning(1);
}

// PANZERS 0x622f50
// Keys of the mission: the loading screen, Pause (one frame when paused),
// Esc (dialogs, then the in-game menu), 1..9 groups (Ctrl+n assigns, Shift+1..3
// squad stance), Ctrl+A / S / T select by class, N the next own unit,
// numpad * / (latency, multiplayer), F-keys; the rest goes to the hotkeys.
bool SGameView::OnKeyDown(int key, bool repeat)
{
    PZ_M3_TRACE("SGameView::OnKeyDown (0x622f50)");
    (void)repeat;
    if (LoadingScreen) {
        // HD 0x622fec: also releases the loading sounds +0x3898 / +0x3894.
        LoadingScreen = false;
        if (LoadingFrame >= 0) {
            Board->DestroyFrame(LoadingFrame);
            LoadingFrame = -1;
        }
        SendAction(PZA_GV_MAP_LOADED, 0);
        return true;
    }
    if (Modal != 0) {
        StubOnce("SGameView::OnKeyDown modal box 0x619520");
        return true;
    }
    pz::SGameLogic* gl = this->Logic;
    if (!gl)
        return true;
    if (ViewState == 2) {
        if (key == VK_ESCAPE)
            StubOnce("SGameView::OnKeyDown Esc in a cut-scene 0x628070");
        return true;
    }
    if (gl->IsPaused())                                           // 0x56e150
        return true;
    if ((unsigned)key < 0x100 && KeyDownTime[key] == 0)
        KeyDownTime[key] = NowMs();                               // DAT_007f7f80, ftol
    switch (key) {
    case VK_TAB:
        StubOnce("SGameView::OnKeyDown Tab focus 0x543670 / 0x5439f0");
        break;
    case VK_RETURN:
        if (EndBox)                                               // +0x3e84
            SendAction(PZA_GV_GAMEOVER, 0);                       // the mission is over: Enter leaves
        else
            StubOnce("SGameView::OnKeyDown Enter: chat / cheats (0x6255b0, agent H)");
        break;
    case VK_PAUSE:
        if (gl->Running == 0) {                                   // one logic frame while paused
            if (!(pz::g_Campaign && pz::g_Campaign->GetMissionResult() != 0))
                gl->SetRunning(1);
            gl->Refresh();                                        // 0x576d80
            // HD: scene +0x1c(), board +0xa0(0x32).
        }
        gl->SetRunning(0);
        break;
    case VK_ESCAPE:
        // HD: closes the open dialogs (+0x3e48..+0x3e68, agent H), else the
        // in-game menu 0x620080.
        OpenInGameMenu();
        break;
    case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
        if (CtrlDown(this)) {
            World->AssignGroup(key - '0');                        // 0x5e3660
        } else if (!ShiftDown(this)) {
            World->SelectGroup(key - '0');                        // 0x5fd2f0
        } else if (key <= '3') {
            pz::Pkt_TwoFlags(gl, pz::PZ_PKT_2D, (unsigned char)(key - '1'), false);   // 0x575940(n - 1, 0)
        }
        break;
    case 'A':
        if (CtrlDown(this)) {
            World->SelectByClass(pz::PZ_SEL_ONLY, true, true, true);   // 0x5fd030(0x10, 1, 1, 1)
            return true;
        }
        OnHotkey(key);
        break;
    case 'S':
        if (CtrlDown(this)) {
            World->SelectByClass(pz::PZ_SEL_ONLY, false, false, true);
            return true;
        }
        OnHotkey(key);
        break;
    case 'T':
        if (CtrlDown(this)) {
            World->SelectByClass(pz::PZ_SEL_ONLY, true, false, false);
            return true;
        }
        OnHotkey(key);
        break;
    case 'I': case 't': case 'v':
        break;
    case 'N': {
        // The next own selectable unit after the last selected one (in heap order).
        pz::SWorld* w = World;
        int last = -1;
        for (int i = 0; i < w->Units.Size; ++i)
            if (w->Units.IsLive(i) && w->IsUnitSelected(i) && pz::WorldUnit(i)->Player == w->LocalPlayer && last < i)
                last = i;
        for (int pass = 0; pass < 2; ++pass) {
            for (int i = pass == 0 ? last + 1 : 0; i < w->Units.Size; ++i) {
                if (!w->Units.IsLive(i))
                    continue;
                pz::SUnit* u = pz::WorldUnit(i);
                if (u->_112 && !u->_110 && !u->Unplaced && !u->Wrecked && u->Parent == -1
                    && u->Player == w->LocalPlayer) {
                    w->SelectUnit(i, pz::PZ_SEL_ONLY);            // 0x5fcb10(i, 0x10)
                    *(int*)((unsigned char*)w + 0xa4) = i;        // World+0xa4: the camera follows it
                    return true;
                }
            }
        }
        break;
    }
    case VK_MULTIPLY:
        if (pz::g_Campaign && ((unsigned char*)pz::g_Campaign)[0xb80] < 5)
            pz::Pkt_Latency(gl, (unsigned char)(((unsigned char*)pz::g_Campaign)[0xb80] + 1));   // 0x5761d0
        break;
    case VK_DIVIDE:
        if (pz::g_Campaign && 2 < ((unsigned char*)pz::g_Campaign)[0xb80])
            pz::Pkt_Latency(gl, (unsigned char)(((unsigned char*)pz::g_Campaign)[0xb80] - 1));
        break;
    case VK_F1:
        StubOnce("SGameView::OnKeyDown F1 help 0x61ff30 (agent H)");
        break;
    case VK_F6:
        StubOnce("SGameView::OnKeyDown F6 quicksave (agent F)");
        break;
    case VK_F8:
        if (ViewState == 0 || ViewState == 1)
            StubOnce("SGameView::OnKeyDown F8 0x625d80 panel mode (agent V)");
        break;
    case VK_F9:
        StubOnce("SGameView::OnKeyDown F9 quickload 0x494c1 (agent F)");
        break;
    case VK_F10:
        OpenInGameMenu();                                         // HD 0x61ffb0 when no dialog is open
        break;
    case VK_F11:
        StubOnce("SGameView::OnKeyDown F11 (+0x24 vtbl +0xa4)");
        break;
    default:
        OnHotkey(key);                                            // 0x6195b0
        break;
    }
    return true;
}
