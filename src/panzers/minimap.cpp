// src/panzers/minimap.cpp
// SMinimap (hud.h) and the minimap's calls into the game view: the camera
// jump / drag and the right-click move order, which go out as lockstep
// packets through agent O's builders (packets.h). OWNER: agent H.

#include <windows.h>
#include <math.h>
#include "hud.h"
#include "gameview.h"
#include "m3common.h"
#include "stub_log.h"
#include "logger.h"
#include "pzboard.h"
#include "board.h"
#include "packets.h"
#include "worldapi.h"
#include "world.h"
#include "gamelogic.h"

// PANZERS 0x808ff0: 1/172, the minimap is 0xac x 0xac pixels.
static const double kMinimapScale = 1.0 / 172.0;

SMinimap::SMinimap() : _58(0), Frame(-1), Dragging(false) { ToolTipFeatureEnabled = false; }

// PANZERS 0x64c2c0
SMinimap::~SMinimap()
{
    if (Frame >= 0) {
        Board->DestroyFrame(Frame);
        Frame = -1;
    }
    if (CompassTex >= 0) {
        PzReleaseTexture(CompassTex);
        CompassTex = -1;
    }
}

// PANZERS 0x64c2f0
// The minimap frame (board type 6) is a child of the parent's frame at the
// widget's position, with the compass texture as its glyph (board +0xc0).
void SMinimap::Create(int p1, int p2, int p3, int p4)
{
    (void)p1; (void)p2; (void)p4;
    Resize(0xac, 0xac);                                            // vtbl +0x10
    SDXWidget::Create(0);                                          // 0x539a10
    _58 = p3;
    // HD: CreateFrame(6 = minimap, ...) and board +0xc0 with the compass.
    // The recompile's board draws FT_MINIMAP the SWINE way (camera lines
    // through SGepard::GetViewBoundaries -> STerrain::GetHeight), which
    // crashes without a SWINE terrain; until the board gets the HD minimap
    // render (agent E: +0xc0 / +0x54) the frame is a plain sprite of the
    // compass texture, kept for the life of the widget.
    Frame = Board->CreateFrame(FT_SPRITE, Parent->GetFrame(), X, Y, 0, 1);
    CompassTex = PzLoadTexture("menu/minimap_compass_hq.tga");     // board +0x7c (0x802798)
    Board->SetSpriteGlyph(Frame, CompassTex, 0);
    Board->ResizeFrame(Frame, 0xac, 0xac);
}

// PANZERS 0x64c390 (board +0x54)
void SMinimap::SetRotation(float rotation)
{
    // HD board +0x54 on the minimap frame; the stand-in sprite has no
    // rotation (see Create).
    (void)rotation;
}

// PANZERS 0x64c3b0
void SMinimap::OnMouseDown(int button, int x, int y, int shift)
{
    (void)shift;
    SGameView* view = static_cast<SGameView*>(Parent);
    if (button == 1) {
        if ((unsigned)x <= 0xab && (unsigned)y <= 0xac) {
            if (PzViewMinimapClick(view, (float)((double)x * kMinimapScale), (float)((double)y * kMinimapScale))) {
                Dragging = true;
                CaptureMouse();                                    // 0x543300
            }
        }
    } else if (button == 3 && (unsigned)x <= 0xab && (unsigned)y <= 0xac) {
        PzViewMinimapOrder(view, (float)((double)x * kMinimapScale), (float)((double)y * kMinimapScale));
    }
}

// PANZERS 0x64c4a0
void SMinimap::OnMouseMove(int x, int y, int shift)
{
    (void)shift;
    if (BackFrame >= 0 && Dragging)
        PzViewMinimapDrag(static_cast<SGameView*>(Parent), (float)((double)x * kMinimapScale),
                          (float)((double)y * kMinimapScale));
}

// PANZERS 0x64c500
void SMinimap::OnMouseOut()
{
}

// PANZERS 0x64c510
void SMinimap::OnMouseUp(int button, int x, int y, int shift)
{
    (void)x; (void)y; (void)shift;
    if (button == 1) {
        Dragging = false;
        ReleaseMouse();                                            // 0x5437c0
    }
}

// PANZERS 0x64c530
void SMinimap::SetVisible(bool visible)
{
    SDXWidget::SetVisible(visible);                                // 0x539c40
    Board->ShowFrame(Frame, visible);
}

// PANZERS 0x571950 (SGameLogic, minimap part)
// Minimap position (0..1, rotated with the camera yaw World +0x44) to a
// world position; false (and a clamped position) outside the playable
// area, which leaves a 48-unit border on maps wider than 0x60 tiles.
// SGameLogic +0x18c points to the minimap size {w, h}.
static bool MinimapToWorld(float* p)
{
    pz::SWorld* w = pz::g_World;
    pz::SGameLogic* gl = pz::g_GameLogic;
    const int* size = *(const int* const*)((const unsigned char*)gl + 0x18c);
    float yaw = -w->CamYaw;                                        // xorps 0x7f5ac0
    float c = (float)cos((double)yaw);                             // 0x78d480
    double sd = sin((double)yaw);                                  // 0x78d640
    float s = (float)sd, ns = (float)-sd;
    float dx = p[0] - 0.5f, dy = p[1] - 0.5f;                      // DAT_007f453c
    float v = (ns * dx + c * dy) * 172.0f;                         // _DAT_007f7f98
    float u = (s * dy + c * dx) * 172.0f;
    int W = w->TerrainW, H = w->TerrainH;
    float lo, hiX, hiZ;
    if (W > 0x60) {
        double hw = (double)W * 0.5, hh = (double)H * 0.5;         // DAT_007ea760
        p[0] = (float)(((hw - 48.0) * (double)u) / (double)(float)(size[0] / 2) + hw);
        p[1] = (float)(hh - ((hh - 48.0) * (double)v) / (double)(float)(size[1] / 2));
        lo = 48.0f;                                                // DAT_007f7f90
        if (p[0] >= lo && (float)(W - 0x30) > p[0] && p[1] >= lo && (float)(H - 0x30) > p[1])
            return true;
    } else {
        p[0] = (float)((double)((float)W * u) * 0.5 / (double)(float)(size[0] / 2) + (double)W * 0.5);
        p[1] = (float)((double)H * 0.5 - (double)((float)H * v) * 0.5 / (double)(float)(size[1] / 2));
        lo = 0.0f;
        if (p[0] >= lo && (float)W > p[0] && p[1] >= lo && (float)H > p[1])
            return true;
    }
    // Clamp (both branches clamp to [lo, size - 0x30]).
    hiX = (float)(W - 0x30);
    hiZ = (float)(H - 0x30);
    float x = p[0] > lo ? p[0] : lo;
    if (x >= hiX) x = hiX;
    float z = p[1] > lo ? p[1] : lo;
    if (z >= hiZ) z = hiZ;
    p[0] = x;
    p[1] = z;
    return false;
}

// SWorld 0x5f4f60 (camera jump; an SWorld camera function, agent V's
// world.cpp may take it over): clamp to the camera box, set the target,
// stop following.
static void CameraJump(pz::SWorld* w, float x, float z)
{
    if (w->CamLocked != 0)
        return;
    if (x < w->CamXMin || w->CamXMax < x)
        x = x < w->CamXMin ? w->CamXMin : w->CamXMax;
    if (z < w->CamZMin || w->CamZMax < z)
        z = z < w->CamZMin ? w->CamZMin : w->CamZMax;
    w->CamTarget[0] = x;
    w->CamTarget[2] = z;
    w->CamFollowPath = 0;
    w->CamFollowUnit = -1;
}

static bool ViewReady(SGameView* v)
{
    return v && v->World && v->Logic && pz::g_World && pz::g_GameLogic &&
           *(const int* const*)((const unsigned char*)pz::g_GameLogic + 0x18c) != nullptr;
}

// PANZERS 0x6206f0
// LMB on the minimap: in a command mode (+0x3884) the order goes to the
// position (packets.h); otherwise the camera jumps there (true: drag on).
bool PzViewMinimapClick(SGameView* v, float u, float vv)
{
    if (!ViewReady(v)) {
        PZ_M3_TRACE("SGameView::MinimapClick (0x6206f0): no minimap size (SGameLogic +0x18c)");
        return false;
    }
    float p[2] = { u, vv };
    if (!MinimapToWorld(p))
        return false;
    pz::SGameLogic* gl = pz::g_GameLogic;
    // HD passes the key states [0x28] (+0xa0) and [0x27] (+0x9c) as the
    // packet bools; they belong to agent O's input code (false here).
    bool b2 = false;
    switch (v->CommandMode) {
    case 1:    pz::Pkt_Move(gl, p[0], p[1], false, b2); break;                     // 0x575ef0
    case 2:    pz::Pkt_MoveBack(gl, p[0], p[1], false, b2); break;                 // 0x575e10
    case 3:    pz::Pkt_PosB(gl, pz::PZ_PKT_21, p[0], p[1], b2); break;             // 0x575a00
    case 4:    pz::Pkt_Pos(gl, pz::PZ_PKT_09, p[0], p[1], false, b2); break;       // 0x575770
    case 5:    break;
    case 8:    pz::Pkt_Pos(gl, pz::PZ_PKT_16, p[0], p[1], false, b2); break;       // 0x575ae0
    case 9:    pz::Pkt_Pos(gl, pz::PZ_PKT_17, p[0], p[1], false, b2); break;       // 0x576300
    case 0xe:  pz::Pkt_Pos(gl, pz::PZ_PKT_1A, p[0], p[1], false, b2); break;       // 0x575db0
    case 0x15: pz::Pkt_Pos(gl, pz::PZ_PKT_0E, p[0], p[1], false, b2); break;       // 0x575610
    case 0x16: pz::Pkt_Support(gl, pz::PZ_PKT_SUPPORT_1C, p[0], p[1]); break;              // 0x575880
    case 0x17: pz::Pkt_Support(gl, pz::PZ_PKT_SUPPORT_1D, p[0], p[1]); break;              // 0x576070
    case 0x18: pz::Pkt_Support(gl, pz::PZ_PKT_SUPPORT_1E, p[0], p[1]); break;              // 0x5762b0
    case 0x19: STUB_LOG("SGameView 0x6206f0: bomber 0x575cb0 needs the second point"); break;
    case 0x1a: STUB_LOG("SGameView 0x6206f0: paratroops 0x576000 needs the second point"); break;
    default:
        CameraJump(pz::g_World, p[0], p[1]);                       // 0x5f4f60
        return true;
    }
    v->MouseMode = 0;                                              // +0x478
    v->CommandMode = 0;                                            // +0x3884
    return false;
}

// PANZERS 0x620af0
void PzViewMinimapDrag(SGameView* v, float u, float vv)
{
    if (!ViewReady(v))
        return;
    float p[2] = { u, vv };
    MinimapToWorld(p);
    CameraJump(pz::g_World, p[0], p[1]);                           // 0x5f4f60
}

// PANZERS 0x620b40
// RMB on the minimap: a move order to the position (packet 0x02).
void PzViewMinimapOrder(SGameView* v, float u, float vv)
{
    if (!ViewReady(v))
        return;
    float p[2] = { u, vv };
    if (!MinimapToWorld(p))
        return;
    v->MouseMode = 0;
    v->CommandMode = 0;
    pz::Pkt_Move(pz::g_GameLogic, p[0], p[1], false, false);       // 0x575ef0(x, z, [0x28], [0x27]): key states are agent O's
}
