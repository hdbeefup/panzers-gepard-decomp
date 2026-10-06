// src/panzers/gameview_view.cpp
// SGameView: construction, Create (the part before the HUD), the per-frame
// Update 0x628430, the camera input (held keys 0x62c470, mouse modes and edge
// scroll 0x620bc0), the panel modes and sub-viewports, the callback slots of
// SIGameViewCallback and the non-menu cases of OnAction 0x6216b0.
// OWNER: agent V (docs/M3_INTERFACES.md §4). Input handlers: agent O
// (gameview_input.cpp); LoadMap / MissionStart: agent F; the HUD half of
// Create, the HUD refresh and the in-game menu actions: agent H.
//
// The view changes logic state only through the calls HD makes (the 50 ms
// SGameLogic::Refresh, SetRunning when a box closes); the camera, the range
// overlay and the cursor are visual only.

#include <windows.h>
#include <string.h>
#include <stdlib.h>
#include "gameview.h"
#include "ingamemenu.h"
#include "superwindow.h"
#include "pzboard.h"
#include "m3common.h"
#include "campaign.h"
#include "stub_log.h"
#include "logger.h"
#include "timer.h"
#include "settings.h"
#include "dxwindow.h"
#include "iconcert.h"
#include "pz/igepardhd.h"
#include "pz/iscene.h"
#include "pz/iviewport.h"
#include "worldapi.h"
#include "world.h"
#include "selection.h"
#include "unit.h"
#include "gamelogic.h"

namespace pz { void PzCameraSplineTick(); }   // 0x6096f0 (src/world/worldcamera.cpp)

// ---------------------------------------------------------------------------
// Hooks into the other agents' files (coordinator decision, 2026-10-06).
// Each has a do-nothing default here, bound with the MSVC linker's
// /alternatename: the default is used only while the owner's definition is
// not linked, so nothing has to change here when H or F merge.
//   PzHudCreate           H, hud.cpp: Create 0x619c90 from the
//                         "menu/panzers_interface_hq.tga" icon set (0x61b3b7,
//                         with the layout tables copied before it) to the end.
//   PzGameViewMenuAction  H, ingamemenu.cpp: the menu cases of OnAction
//                         0x6216b0 (0x494d1..0x494d9, the boxes +0x3e48..
//                         +0x3e68, 0x49481, 0x4947421 / 2). True = handled.
//   PzHudUpdate           H (proposed): the HUD refresh block of Update
//                         0x628430, 0x628b5e..0x62bb83 without the range
//                         overlay (support and group buttons, unit panel
//                         0x626c40, spec info, clock / timer / PAUSE texts).
//   PzGameViewEndCheck    F (proposed): the tail of Update 0x628430 from
//                         0x62bb83: the -exit switch 0x929d90, campaign
//                         result 0x5920b0 -> victory / defeat box (+0x3e84),
//                         music, results menu (+0x3e6c).
void PzHudCreate(SGameView* view);
bool PzGameViewMenuAction(SGameView* view, SWidget* source, int action, int param);
void PzHudUpdate(SGameView* view);
void PzGameViewEndCheck(SGameView* view);

void PzHudCreateDefault(SGameView*) {}
bool PzGameViewMenuActionDefault(SGameView*, SWidget*, int, int) { return false; }
void PzHudUpdateDefault(SGameView*) {}
void PzGameViewEndCheckDefault(SGameView*) {}
#pragma comment(linker, "/alternatename:?PzHudCreate@@YAXPAUSGameView@@@Z=?PzHudCreateDefault@@YAXPAUSGameView@@@Z")
#pragma comment(linker, "/alternatename:?PzGameViewMenuAction@@YA_NPAUSGameView@@PAUSWidget@@HH@Z=?PzGameViewMenuActionDefault@@YA_NPAUSGameView@@PAUSWidget@@HH@Z")
#pragma comment(linker, "/alternatename:?PzHudUpdate@@YAXPAUSGameView@@@Z=?PzHudUpdateDefault@@YAXPAUSGameView@@@Z")
#pragma comment(linker, "/alternatename:?PzGameViewEndCheck@@YAXPAUSGameView@@@Z=?PzGameViewEndCheckDefault@@YAXPAUSGameView@@@Z")

// HD: ftol(timer seconds * 1000.0) (0x661800, 0x7f7f80, 0x7669f1).
unsigned SGameView::NowMs()
{
    double ms = (double)Timer.GetTickValue() / 1000.0 * 1000.0;
    return (unsigned)(long long)ms;
}

// An HD field of the view that has no name yet (HD object offset).
int& SGameView::HdInt(int offset)
{
    return *(int*)((unsigned char*)static_cast<SGameViewData*>(this) + (offset - 0x5c));
}

static SDXWindow* ViewWindow(SWidget* w)
{
    return static_cast<SDXWindow*>(w->GetWindowParent());     // 0x5435b0: first parent with IsWindow
}

// PANZERS 0x6181f0
SGameView::SGameView()
{
    PZ_M3_TRACE("SGameView::SGameView (0x6181f0)");
    ToolTipFeatureEnabled = false;
    // HD constructs the member widgets here (9 SGroupIcon at +0x830, 16 + 2
    // SUnitButton, 5 SHeroUnitButton, 5 + 4 SSpecInfoWidget, 20
    // SCommandButton, ...): H's classes. The data block starts zeroed.
    memset(static_cast<SGameViewData*>(this), 0, sizeof(SGameViewData));
    ClockStart = 0;                                                // [0x117]
    NetPingTime = 0;                                               // [0x119]
    Subport[0] = Subport[1] = Subport[2] = -1;                     // [0x11b..0x11d]
    Viewport = pz::PzGepard()->GetViewport(0);                     // Gepard +0x3c(0)
    MouseMode = 0;
    CommandMode = 0;                                               // [0xe21]
    MouseInside = false;                                           // [0x12d]
    MouseX = 0x32;
    MouseY = 0x32;
    _4b8 = _4bc[0] = _4bc[1] = -1;                                 // [0x12e..0x130]
    SoundHandle3894 = -1;                                          // [0xe25]
    SoundHandle3898 = -1;                                          // [0xe26]
    HdInt(0x2470) = -1;                                            // [0x91c]
    HdInt(0x3e38) = -1;                                            // [0xf8e]
    HdInt(0x3e3c) = -1;                                            // [0xf8f]
    ClockText = TimerText = PauseText = _5d8 = -1;                 // [0x173..0x176]
    _4c4[0] = _4c4[1] = _4c4[2] = _4c4[3] = -1;                    // [0x131..0x134]
    Modal = 0;                                                     // [0xe27]
    // Recompile: the skeleton's loading-screen frame.
    LoadingFrame = -1;
}

// PANZERS 0x618aa0
// HD: DestroySubViewports, the menus and boxes (+0x3e48..+0x3e6c,
// +0x3e84..+0x3e94, +0x389c), the board frames and icon sets, the widget list,
// the mission SGameLogic (+0x3e44) and SWorld (+0x3e40). The HUD widgets are
// members (destroyed with the object).
SGameView::~SGameView()
{
    PZ_M3_TRACE("SGameView::~SGameView (0x619430)");
    DestroySubViewports();                                         // 0x61e680
    if (InGameMenu) {
        delete InGameMenu;                                         // recompile stand-in (H)
        InGameMenu = nullptr;
    }
    if (FramesCreated && Board) {
        Board->DestroyFrame(MessageFrame[0]);                      // board +0x0c
        Board->DestroyFrame(MessageFrame[1]);
        for (int i = 0; i < 8; ++i) {
            Board->DestroyFrame(MessageIcon[i]);
            Board->DestroyFrame(MessageText[i]);
        }
        if (ClockText >= 0) Board->DestroyFrame(ClockText);
        Board->DestroyFrame(TimerText);
        Board->DestroyFrame(PauseText);
        if (_5d8 >= 0) Board->DestroyFrame(_5d8);
        FramesCreated = false;
    }
    if (LoadingFrame >= 0 && Board) {
        Board->DestroyFrame(LoadingFrame);
        LoadingFrame = -1;
    }
    if (Board)
        ReleaseLoadingBackdrop();                                  // gameview_loading.cpp
    SWidget** boxes[6] = { &EndBox, &ModalBox, &Box3e8c, &Box3e90, &NetBox, &this->SGameViewData::MessageBox };
    for (SWidget** b : boxes) {
        if (*b) {
            delete *b;
            *b = nullptr;
        }
    }
    if (Logic) {
        delete Logic;                                              // 0x55fe00 + delete 0x318
        Logic = nullptr;
    }
    if (World) {
        delete World;                                              // vtbl +0x00(1); releases g_Scene
        World = nullptr;
    }
}

// PANZERS 0x619c90 (up to the HUD, 0x619c90..0x61b3b7)
void SGameView::Create()
{
    PZ_M3_TRACE("SGameView::Create (0x619c90)");
    if (Logger.g)
        Logger.g->Log(1, "SGameView::Create: start %f", (double)Timer.GetTickValue() / 1000.0);
    SDXWidget::Create(0);                                          // 0x539a10
    int frame = GetFrame();                                        // +0x48
    MessageFrame[0] = Board->CreateFrame(FT_TEXT, frame, 0x3f2, 0x50, 0, false);   // board +0x08
    Board->ShowFrame(MessageFrame[0], false);                      // board +0x18
    MessageFrame[1] = Board->CreateFrame(FT_TEXT, frame, 0x3f2, 0xb6, 0, false);
    Board->ShowFrame(MessageFrame[1], false);
    for (int i = 0; i < 8; ++i) {
        int y = (i > 3 ? 0x28 : 0) + 0x64 + i * 0x12;
        MessageText[i] = Board->CreateFrame(FT_TEXT, frame, 0x3e8, y, 0, false);
        Board->ShowFrame(MessageText[i], false);
        MessageIcon[i] = Board->CreateFrame(FT_SPRITE, frame, 0x3ed, y, 0, false);
        Board->ShowFrame(MessageIcon[i], false);
    }
    TimerText = Board->CreateFrame(FT_TEXT, frame, Width / 2, 0x35, 1, false);
    PauseText = Board->CreateFrame(FT_TEXT, frame, Width - 8, 0x3d, 2, false);
    FramesCreated = true;
    // HD 0x619e3a..0x61b3b0 copies the HUD layout tables to the stack, then
    // loads "menu/panzers_interface_hq.tga" (board +0x74) into +0x4b8 and
    // builds the HUD: agent H.
    PzHudCreate(this);
}

// ---------------------------------------------------------------------------
// Camera input

// PANZERS 0x62c470
// A held key moves the camera by the milliseconds since the key went down or
// since the last Update, whichever is later (OnKeyUp 0x6246d0 calls this once
// more with the release time). Arrows scroll (keyboard scroll speed per ms;
// not while box-dragging), Home / End pitch, PgDn / Delete yaw (0.001 rad
// per ms), PgUp, numpad +, '=' zoom out and Insert, numpad -, '-' zoom in
// (0.04 per ms).
void SGameView::KeyScroll(unsigned now, int key)
{
    unsigned down = KeyDownTime[key];
    int held = (int)(now - down);
    int before = 0;
    if (down < ClockStart)
        before = (int)(ClockStart - down);
    if (ViewState == 2)
        return;
    pz::SWorld* w = World;
    switch (key) {
    case 0x21: case 0x6b: case 0xbb:                               // PgUp, numpad +, '='
        w->ZoomCamera((float)((double)(held - before) * 0.04));    // 0x609390, 0x7fa3f8
        return;
    case 0x2d: case 0x6d: case 0xbd:                               // Insert, numpad -, '-'
        w->ZoomCamera((float)((double)(before - held) * 0.04));
        return;
    case 0x22:                                                     // PgDn
        w->RotateCamera((float)((double)(held - before) * 0.001), 0.0f);   // 0x5f8380, 0x7f59b8
        return;
    case 0x2e:                                                     // Delete
        w->RotateCamera((float)((double)(before - held) * 0.001), 0.0f);
        return;
    case 0x23:                                                     // End
        w->RotateCamera(0.0f, (float)((double)(held - before) * 0.001));
        return;
    case 0x24:                                                     // Home
        w->RotateCamera(0.0f, (float)((double)(before - held) * 0.001));
        return;
    case 0x25:                                                     // Left
        if (MouseMode == 1)
            return;
        w->MoveCamera(0.0f, Settings.KeyboardScrollSpeed * (float)(before - held));   // 0x5f4dc0, 0x64dff0
        return;
    case 0x27:                                                     // Right
        if (MouseMode == 1)
            return;
        w->MoveCamera(0.0f, Settings.KeyboardScrollSpeed * (float)(held - before));
        return;
    case 0x26:                                                     // Up
        if (MouseMode == 1)
            return;
        w->MoveCamera(Settings.KeyboardScrollSpeed * (float)(held - before), 0.0f);
        return;
    case 0x28:                                                     // Down
        if (MouseMode == 1)
            return;
        w->MoveCamera(Settings.KeyboardScrollSpeed * (float)(before - held), 0.0f);
        return;
    default:
        return;
    }
}

// Edge-scroll cursors (HD table 0x8de0f4, index xdir - 3 * zdir + 4).
static const int kScrollCursor[9] = { 0x0d, 0x0e, 0x0f, 0x0c, 0x00, 0x10, 0x13, 0x12, 0x11 };

// PANZERS 0x620bc0
// Per frame from Update with the last mouse position (+0x4ac / +0x4b0) and
// the elapsed ms: the mouse modes started by OnMouseDown (agent O) and the
// edge scroll. Modes 2 / 3 (rotate) and 7 (scroll drag) warp the real cursor
// back to the press point with SetCursorPos.
void SGameView::MouseCamera(int x, int y, int ms)
{
    if ((ViewState == 2 && !LoadingScreen) || Modal != 0)
        return;
    // HD first casts a ray through (x, y) (viewport +0x34, world 0x5ea910)
    // for the terrain point under the cursor; only modes 4 / 5 / 6 and the
    // hover cursor 0x621540 use it (case 4 casts its own ray below).
    int cursor = -1;
    switch (MouseMode) {
    case 1: {
        int dx = PressX - x, dy = PressY - y;
        bool drag = Dragging || abs(dx) >= 5 || abs(dy) >= 5;
        Dragging = drag;
        if (!drag) {
            HoverCursor(x, y, ms);                                 // 0x621540
            break;
        }
        World->DrawSelectionBox(PressX, PressY, x, y);             // 0x5fd630 (agent O)
        {
            // HD: viewport +0x38 (box frustum) -> 0x5fc5b0(frustum, 0x21):
            // mode 0x21 keeps bit 0 and marks the units in the box with
            // bit 1 (the highlight). O's SPzSelectRect stands in for the frustum.
            pz::SPzSelectRect box = { Viewport, PressX, PressY, x, y };
            World->SelectUnitsInBox(&box, 0x21);
        }
        cursor = 0xb;
        break;
    }
    case 2:
    case 3:
        if (x != PressX || y != PressY) {
            World->RotateCamera((float)((double)(x - PressX) * 0.002),    // 0x8043a0
                                (float)((double)(y - PressY) * 0.002));
            ViewWindow(this)->SetCursorPos(PressX, PressY);        // 0x53a310 (SetCursorPos)
        }
        cursor = 10;
        break;
    case 4: {
        // HD: pick 0x5ebac0 + highlight 0x5fcb10(unit, 0x21), cursor 0x14
        // coloured by the relation 0x56d280 (the colour is not lifted).
        float ray[6] = { 0, 0, 0, 0, -1.0f, 0 };
        if (Viewport)
            Viewport->ScreenToRay(ray, x, y);                      // viewport +0x34
        World->SelectUnit(World->PickAnyUnitAt(ray), 0x21);
        cursor = 0x14;
        break;
    }
    case 5:
    case 6:
        // HD: placement preview (air support target, flak) with terrain
        // effect decals from the ray point; not lifted.
        break;
    case 7:
        if (x != PressX || y != PressY) {
            World->MoveCamera((float)((double)(PressY - y) * 0.02),       // 0x7f59e8
                              (float)((double)(x - PressX) * 0.02));
            ViewWindow(this)->SetCursorPos(PressX, PressY);
        }
        break;
    default:
        if (MouseInside) {
            HoverCursor(x, y, ms);
            break;
        }
        World->ApplySelectionToAll(1);                             // 0x5fc860(1): clear the highlight
        cursor = 0;
        break;
    }
    if (cursor >= 0)
        Cursor = cursor;                                           // 0x543970(cursor, -1)

    // Edge scroll (full screen only: window vtbl +0xb4 = IsFullScreen).
    if (MouseMode == 1 || MouseMode == 2 || MouseMode == 3)
        return;
    SDXWindow* wnd = ViewWindow(this);
    if (!wnd || !wnd->IsFullScreen())
        return;
    x = wnd->LastMouseX;                                           // 0x544c20: window +0x80 / +0x84
    y = wnd->LastMouseY;
    int wx, wy, ww, wh;
    wnd->GetPosition(&wx, &wy, &ww, &wh);                          // window vtbl +0x04
    int xdir = 0, zdir = 0;
    if (x < 1) xdir = -1;
    if (x >= ww - 1) xdir = 1;
    if (y < 1) zdir = 1;
    if (y >= wh - 1) zdir = -1;
    if (xdir == 0 && zdir == 0)
        return;
    if (x < 0x10) xdir = -1;
    if (x >= ww - 0x10) xdir = 1;
    if (y < 0x10) zdir = 1;
    if (y >= wh - 0x10) zdir = -1;
    float right = Settings.MouseScrollSpeed * (float)ms * (float)xdir;   // 0x64e040
    float forward = Settings.MouseScrollSpeed * (float)ms * (float)zdir;
    World->MoveCamera(forward, right);                             // 0x5f4dc0
    Cursor = kScrollCursor[xdir - 3 * zdir + 4];
}

// 0x621540 (not lifted): the cursor over the terrain point / unit under the
// mouse (pick 0x5ebac0, highlight 0x5fcb10(unit, 0x21), cursor by the unit
// kind 0x56d490 and colour by the relation 0x56d280; on terrain, 9 where
// 0x5d9e00 allows a move, else 0). Needs the viewport ray (agent E) and the
// pick (agent O). Recompile: the plain cursor.
void SGameView::HoverCursor(int x, int y, int ms)
{
    (void)x; (void)y; (void)ms;
    Cursor = 0;
}

// PANZERS 0x6251a0
void SGameView::OnMouseOver()
{
    MouseInside = true;
}

// PANZERS 0x625170
void SGameView::OnMouseOut()
{
    MouseInside = false;
}

// ---------------------------------------------------------------------------
// Update

// The unit whose weapon range is shown: SGameLogic 0x56b3e0 fields +0x70 /
// +0x74 (that function also fills the command panel state for the HUD).
// Over the live units of the local player (all units when World+0x4d0 is
// set): the single unit with +0x104 bit 1, else the single one with bit 0
// (selected), else none.
static int RangeOverlayUnit(pz::SWorld* w)
{
    int selCount = 0, sel = -1, hiCount = 0, hi = -1;
    bool all = *((unsigned char*)w + 0x4d0) != 0;
    for (int i = 0; i < w->Units.Size; ++i) {
        if (!w->Units.IsLive(i))
            continue;
        pz::SUnit* u = w->Units.Array[i].Unit;
        if (!all && u->Player != w->LocalPlayer)
            continue;
        unsigned char bits = *((unsigned char*)u + 0x104);
        if (bits & 1) { sel = u->WorldIndex; ++selCount; }
        if (bits & 2) { hi = u->WorldIndex; ++hiCount; }
    }
    if (hiCount == 1)
        return hi;
    if (selCount == 1)
        return sel;
    return -1;
}

// PANZERS 0x628430
// Per frame (SSuperWindow::OnIdle). The view's millisecond clock drives the
// logic: one SGameLogic::Refresh per 50 ms behind (two and a resync when 100
// ms behind); then the scene clock, the camera, the interpolation of the unit
// models, the mouse camera, the HUD and the end-of-mission checks.
void SGameView::Update()
{
    PZ_M3_TRACE("SGameView::Update (0x628430)");
    if (ModalBox || !World || !Logic || !pz::g_Campaign || LoadingScreen || Modal)
        return;
    if (!pz::g_World || !pz::g_Scene)                              // recompile: the market moves them aside
        return;
    if (Logic->Flag288) {
        // HD 0x582380: an in-game animation (SInGameAnimLogic) plays instead
        // of the game: SetPanelMode(2), UpdateUnitVisuals(vp, 0.0). Not lifted.
        STUB_LOG("SGameLogic::UpdateAnimation (0x582380)");
    }
    unsigned now = NowMs();
    int ms = (int)(now - ClockStart);
    // HD: multiplayer ping (0x8f1c3c, every 0x898 ms): no SMulti here.
    for (int key = 0; key < 0x100; ++key)
        if (KeyDownTime[key] != 0)
            KeyScroll(now, key);                                   // 0x62c470
    ClockStart = now;
    if (now < ClockNextTick + 100 || Logic->IsPaused()) {          // 0x56e150
        while (ClockNextTick < ClockStart) {
            if (Logic->Refresh())                                  // 0x576d80
                ClockNextTick += 0x32;
            else
                ClockNextTick = ClockStart;
        }
    } else {
        Logic->Refresh();
        Logic->Refresh();
        ClockNextTick = ClockStart;
    }
    // HD: multiplayer chat / "%s is waiting" lines (0x594d20, 0x594d00 and
    // SMulti 0x8f1a74): single player only here.
    pz::g_Scene->AdvanceTime(Logic->Running * ms);                 // scene +0x1c
    // HD board +0xa0(ms): board animation clock (the SWINE board keeps its
    // own clock, as in SSuperWindow::OnIdle).
    pz::PzCameraSplineTick();                                      // 0x6096f0 on 0x929a60
    // HD: 0x609ce0 plays the -csplay camera spline (Settings +0x4d): not lifted.
    World->ComputeCamera(Viewport);                                // 0x5ddc30
    double visuals;
    if (Logic->Running == 0) {
        pz::g_Scene->SetInterpolation(0.0);                        // scene +0x20
        visuals = 1.0;                                             // 0x7eed98
    } else {
        double t = (double)(unsigned)(ClockNextTick - ClockStart) / 50.0;   // 0x7f89e0
        pz::g_Scene->SetInterpolation(t);
        visuals = t;
    }
    Logic->UpdateUnitVisuals(Viewport, visuals);                   // 0x5638f0
    if (Concert)
        Concert->Update(false);                                    // Concert +0x0c(0)
    // HD: IsPaused && 0x589860 (an animation is shown) -> SetPanelMode(2);
    // 0x589860 is SInGameAnimLogic (not lifted): treated as false.
    if (ViewState == 2)
        SetPanelMode(0);
    MouseCamera(MouseX, MouseY, ms);                               // 0x620bc0
    // HUD block: the range overlay of the single selected unit is V's.
    World->ShowUnitRange(RangeOverlayUnit(World));                 // 0x5fee00 (0x629792 / 0x62ac09)
    PzHudUpdate(this);
    // HD: -movierec frame capture (0x929d08): not lifted.
    PzGameViewEndCheck(this);
}

// ---------------------------------------------------------------------------
// Panel modes and sub-viewports

// 0x61e500 (rects only; the subports are pending agent E)
// HD splits the primary viewport into three subports: the 3D view between the
// top bar (18 / 768 of the height) and the bottom panel (146 / 768), the top
// strip and the bottom strip, and points +0x468 at subport 0.
void SGameView::CreateSubViewports()
{
    PZ_M3_TRACE("SGameView::CreateSubViewports (0x61e500)");
    if (Subport[0] >= 0)
        return;
    SDXWindow* wnd = ViewWindow(this);
    int wx, wy, ww, wh;
    wnd->GetPosition(&wx, &wy, &ww, &wh);
    int top = (wh * 0x12) / 0x300;
    int bottom = (wh * 0x92) / 0x300;
    int mid = wh - bottom - top;
    // HD: Gepard GetViewport(0) +0x60 (subport count) must be 0, then
    // +0x54 CreateSubport(x, y, w, h) x3, +0x5c(1) / +0x5c(2) +0x80(0), and
    // Viewport = +0x5c(0). E lifted +0x54 / +0x58 / +0x60, but +0x5c
    // (GetSubport 0x68bde0, returns the sub SViewport) is still a stub, so the
    // 3D view keeps the whole window for now.
    SubportRect[0][0] = 0; SubportRect[0][1] = top;        SubportRect[0][2] = ww; SubportRect[0][3] = mid;
    SubportRect[1][0] = 0; SubportRect[1][1] = 0;          SubportRect[1][2] = ww; SubportRect[1][3] = top;
    SubportRect[2][0] = 0; SubportRect[2][1] = mid + top;  SubportRect[2][2] = ww; SubportRect[2][3] = bottom;
    STUB_LOG("SViewport subports (0x68ab40 / 0x68bde0) for CreateSubViewports 0x61e500");
}

// PANZERS 0x61e680
void SGameView::DestroySubViewports()
{
    PZ_M3_TRACE("SGameView::DestroySubViewports (0x61e680)");
    if (Subport[0] < 0)
        return;
    pz::SIViewport* vp = pz::PzGepard()->GetViewport(0);
    vp->DestroySubport(Subport[2]);                         // +0x58 destroy subport
    vp->DestroySubport(Subport[1]);
    vp->DestroySubport(Subport[0]);
    Subport[0] = Subport[1] = Subport[2] = -1;
    // HD: +0x60 must return 0 ("Not all subports have been destroyed!").
    Viewport = vp;
}

// PANZERS 0x61f450
int SGameView::GetPanelMode()
{
    return ViewState;
}

// PANZERS 0x625d80 (mode bookkeeping; the widgets are H's, the subports E's)
// 0: game (3D view between the bars, HUD shown), 1: full screen without the
// HUD (loading), 2: cut-scene (letterbox 96 / 768 bars, HUD hidden).
void SGameView::SetPanelMode(int mode)
{
    PZ_M3_TRACE("SGameView::SetPanelMode (0x625d80)");
    if (mode != 0) {
        Cursor = -1;                                               // 0x543970(-1, -1)
        // HD hides +0xc68 (vtbl +0x6c(0)), the 9 group icons at +0x830 and
        // the 5 hero buttons at +0x218c: H's widgets (PzHudUpdate shows them
        // again by mode).
    }
    // HD: +0x4d4, +0x52c, +0x7cc SetVisible(mode == 0), board frame +0x2a70
    // shown when mode == 0, +0x5dc / +0x75c hidden, frame +0x5d8 hidden: H's
    // widgets and frames.
    if (_5d8 >= 0)
        Board->ShowFrame(_5d8, false);
    if (Subport[0] < 0) {
        if (mode == 2)
            Logger.g->Panic("SGameView::SetPanelMode: No SubViewports");
        ViewState = mode;
        return;
    }
    // HD: resize the three subports (GetViewport(0) +0x5c(i) -> +0x00
    // SetPosition) and show / hide them (+0x7c): mode 0 the CreateSubViewports
    // split, mode 1 subport 0 full screen, mode 2 bars of 96 / 768. Pending
    // the subport slots (agent E).
    ViewState = mode;
}

// ---------------------------------------------------------------------------
// SIGameViewCallback

// 0x622b30 (not lifted: SMessageBox 0x53e0d0, agent E / H). HD: delete the old
// box (+0x3e30), new SMessageBox(0x3ec) into the view, Create("", "", 0x70000,
// 1), target the view, title "panzers/GameView.cpp" "Message", text in
// 0xd0d0d0, show; a box drag (MouseMode 1) is cancelled with 0x5ddb60.
void SGameView::ShowMessageBox(const char* text)
{
    STUB_LOG("SGameView::ShowMessageBox (0x622b30)");
    PZ_M3_TRACE("SGameView::ShowMessageBox (0x622b30)");
    Logger.g->Log(1, "PZM3: message box: %s", text ? text : "");
    if (MouseMode == 1)
        World->HideSelectionBox();                             // 0x5ddb60
    MouseMode = 0;
}

// 0x622c50 (not lifted): shows the campaign text (+0x120 list, 0x14 each)
// whose key equals the SString argument (HD passes an SString by value: buf,
// size; the callee frees it), one line per entry, in a message box.
void SGameView::Slot_04(int p1, int p2)
{
    STUB_LOG("SGameView callback +0x04 (0x622c50)");
    PZ_M3_TRACE("SGameView callback +0x04 (0x622c50)");
    (void)p1; (void)p2;
}

// PANZERS 0x625560
// SetWindowScene: the scene the window renders (window +0xe0): release the old
// one, AddRef the new one.
void SGameView::Slot_08(int p1)
{
    PZ_M3_TRACE("SGameView::SetWindowScene (0x625560)");
    pz::SIScene* scene = (pz::SIScene*)(intptr_t)p1;
    if (pz::g_WindowScene)
        pz::g_WindowScene->Release();                              // scene +0x04
    if (scene)
        scene->AddRef();                                           // scene +0x00
    pz::g_WindowScene = scene;                                     // 0x5435b0() +0xe0
}

// PANZERS 0x624730
void SGameView::ResetClock()
{
    PZ_M3_TRACE("SGameView::ResetClock (0x624730)");
    unsigned now = NowMs();
    ClockNextTick = now;                                           // +0x460
    ClockStart = now;                                              // +0x45c
}

// 0x624770 (not lifted): load another map in place (trigger action): the
// LoadMap 0x6201c0 + mission start 0x6281a0 sequence with the current army
// (BackupCampaignUnits 0x561110 first when +0x3dec). Agent F's flow.
void SGameView::Slot_10(int p1)
{
    STUB_LOG("SGameView callback +0x10 (0x624770)");
    PZ_M3_TRACE("SGameView callback +0x10 (0x624770)");
    (void)p1;
}

// ---------------------------------------------------------------------------
// OnAction

// PANZERS 0x6216b0 (the non-menu cases; the menu cases are H's
// PzGameViewMenuAction)
bool SGameView::OnAction(SWidget* source, int action, int param)
{
    PZ_M3_TRACE("SGameView::OnAction (0x6216b0)");
    if (PzGameViewMenuAction(this, source, action, param))
        return true;
    // The end-of-mission box (+0x3e84) or the results menu (+0x3e6c) closed.
    SWidget* results = (SWidget*)(intptr_t)HdInt(0x3e6c);
    if (source && (source == EndBox || source == results)) {
        if (EndBox) {
            delete EndBox;
            EndBox = nullptr;
        }
        SendAction(PZA_GV_GAMEOVER, 0);                            // 0x543930(0x47562, 0)
        return true;
    }
    if (source && source == ModalBox) {
        delete ModalBox;
        ModalBox = nullptr;
        if (pz::g_Campaign && pz::g_Campaign->GetMissionResult() == 0)   // 0x5920b0 (+0xe4)
            Logic->SetRunning(1);                                  // 0x5802f0(1)
        return true;
    }
    switch (action) {
    // Recompile stand-in until H's PzGameViewMenuAction handles them.
    case PZA_IGM_RESUME:
        if (InGameMenu) {
            delete InGameMenu;                     // ~SWidget unlinks it
            InGameMenu = nullptr;
        }
        return true;
    case PZA_IGM_END:
        // HD: "End Mission" / "Are you sure?" (SMessageBox, H), Yes -> 0x47562.
        Logger.g->Log(0, "PZM3: End Mission (confirm box not lifted) -> GV_GAMEOVER");
        SendAction(PZA_GV_GAMEOVER, 0);
        return true;
    case PZA_BUTTON_DOWN: case PZA_BUTTON_CLICK: case PZA_BUTTON_OVER:
    case PZA_BUTTON_RCLICK: case PZA_BUTTON_OUT:
        // HD 0x621d5a..: the HUD buttons (unit buttons 0x42541 -> select
        // 0x5fcb10, group icons, command buttons -> IssueOrder 0x61e740):
        // agents H / O.
        return true;
    default:
        // Not handled here: SendAction (which starts at the sender itself)
        // passes it on to SSuperWindow (0x47561, 0x47562, ...).
        return false;
    }
}
