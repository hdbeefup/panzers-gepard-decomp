// src/panzers/minimap.cpp
// SMinimap (hud.h) and the minimap's calls into the game view: the camera
// jump / drag and the right-click move order, which go out as lockstep
// packets through agent O's builders (packets.h). OWNER: agent H.

#include <windows.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
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
#include "unit.h"
#include "punit.h"
#include "campaign.h"
#include "stream.h"
#include "core_common.h"
#include "hdbitmap.h"
#include "igepardhd.h"
#include "iviewport.h"

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
}

// PANZERS 0x64c2f0
// The minimap frame (board type 6) is a child of the parent's frame at the
// widget's position; the compass ring (board +0xc0) is drawn over it. The
// map texture comes with the mission's SGameLogic (PzMinimapCreate).
void SMinimap::Create(int p1, int p2, int p3, int p4)
{
    (void)p1; (void)p2; (void)p4;
    Resize(0xac, 0xac);                                            // vtbl +0x10
    SDXWidget::Create(0);                                          // 0x539a10
    _58 = p3;
    Frame = Board->CreateFrame(FT_MINIMAP, Parent->GetFrame(), X, Y, 0, 1);   // board +0x08(6, ...)
    Board->ResizeFrame(Frame, 0xac, 0xac);                         // +0x14
    int compass = Board->LoadSingleFont("menu/minimap_compass_hq.tga", Default);   // +0x7c
    Board->SetMinimapCompass(Frame, compass);                      // +0xc0
    Board->ReleaseFont(compass);                                   // +0x80
}

// PANZERS 0x64c390
// Board +0x54: the map image on or off (black); the panel's terrain button.
void SMinimap::SetTerrain(bool on)
{
    Board->SetMinimapTerrain(Frame, on);
}

// ---------------------------------------------------------------------------
// The minimap image and its overlay (the minimap parts of the HD SGameLogic:
// ctor 0x55e440, UpdateUnitVisuals 0x5638f0, the vision tick 0x565e10).
// The bitmaps live at SGameLogic +0x18c (the map) and +0x190 (the map under
// the fog of war); the texture font at +0x180, the minimap frame at +0x17c.
// ---------------------------------------------------------------------------

namespace {

using pz::SHdBitmap;

struct SMinimapState {
    pz::SGameLogic* Logic = nullptr;   // the logic the bitmaps were made for
    SHdBitmap* Map = nullptr;          // +0x18c
    SHdBitmap* Fogged = nullptr;       // +0x190
    int Font = -1;                     // +0x180
    int FogFrame = -1;                 // PlayerTable[local] of the last fog update
};
SMinimapState s_Mm;

int& GlInt(pz::SGameLogic* gl, int off) { return *(int*)((unsigned char*)gl + off); }

// 0x669ca0 + 0x66e990 + the pitch / size of 0x66ea40.
SHdBitmap* NewBitmap(int w, int h, int format)
{
    int bpp = pz::HdBitmapBpp(format);
    if (bpp == 0 || w <= 0 || h <= 0)
        return nullptr;
    SHdBitmap* b = (SHdBitmap*)calloc(1, sizeof(SHdBitmap));
    b->Width = w;
    b->Height = h;
    b->Format = format;
    b->Bpp = (unsigned char)bpp;
    b->Start = 0;
    b->Pitch = bpp * w;
    b->Size = b->Pitch * h;
    b->Data = (unsigned char*)malloc(b->Size);
    return b;
}

void FreeBitmap(SHdBitmap* b)
{
    if (b) {
        free(b->Data);
        free(b);
    }
}

// 0x669be0 with the source's format: a copy.
SHdBitmap* CopyBitmap(const SHdBitmap* src)
{
    SHdBitmap* b = NewBitmap(src->Width, src->Height, src->Format);
    if (b)
        for (int y = 0; y < src->Height; ++y)
            memcpy(b->Data + y * b->Pitch, src->Data + src->Start + y * src->Pitch, b->Pitch);
    return b;
}

void ReleaseMinimap()
{
    FreeBitmap(s_Mm.Map);
    FreeBitmap(s_Mm.Fogged);
    if (s_Mm.Font >= 0)
        Board->ReleaseFont(s_Mm.Font);
    s_Mm = SMinimapState();
}

void UploadFogged(pz::SGameLogic* gl)
{
    SHdBitmap* f = s_Mm.Fogged;
    // Gepard +0x4c(board +0x84(font), 0x80 - w / 2, 0x80 - h / 2, bitmap):
    // the image centred in the 256 x 256 texture.
    int tex = Board->GetFontTexture(s_Mm.Font);
    pz::PzGepard()->UpdateTexture(tex, 0x80 - f->Width / 2, 0x80 - f->Height / 2, f);
    (void)gl;
}

} // namespace

// PANZERS 0x55e440 (the minimap part of the SGameLogic ctor)
// The minimap texture font "menu/minimap_hq.tga" (+0x180); the map image
// (World +0x74bc, the MINI chunk) copied twice (+0x18c, +0x190) and the
// second copy put into the texture; the minimap frame shows glyph 0 of the
// font at 0xac x 0xac. HD renders the scene from above (scene +0xc
// 0x6b0000) for maps without a MINI chunk: not lifted (logged).
void PzMinimapCreate(pz::SGameLogic* gl)
{
    ReleaseMinimap();
    int frame = gl->MinimapFrame;                                  // +0x17c
    PZ_M3_TRACE("SGameLogic minimap (0x55e440)");
    s_Mm.Logic = gl;
    s_Mm.Font = Board->LoadSingleFont("menu/minimap_hq.tga", Default);   // board +0x7c(0x7f6148)
    GlInt(gl, 0x180) = s_Mm.Font;
    const SHdBitmap* src = (const SHdBitmap*)pz::g_World->Minimap;  // World +0x74bc (MINI chunk)
    if (!src) {
        Logger.g->Log(0, "minimap: no MINI chunk; the scene render 0x6b0000 is not lifted");
        return;
    }
    s_Mm.Map = CopyBitmap(src);                                    // 0x669be0(src, src +8)
    if (!s_Mm.Map)
        return;
    s_Mm.Fogged = CopyBitmap(s_Mm.Map);                            // 0x669be0(+0x18c, +0x18c +8)
    Logger.g->Log(0, "minimap: frame %d, font %d (texture %d), map %dx%d format %d", frame, s_Mm.Font,
                  Board->GetFontTexture(s_Mm.Font), s_Mm.Map->Width, s_Mm.Map->Height, s_Mm.Map->Format);
    GlInt(gl, 0x18c) = (int)(size_t)s_Mm.Map;
    GlInt(gl, 0x190) = (int)(size_t)s_Mm.Fogged;
    UploadFogged(gl);
    Board->SetMinimapGlyph(frame, s_Mm.Font, 0);                   // board +0x4c(frame, font, 0)
    Board->ResizeFrame(frame, 0xac, 0xac);                         // +0x14
}

// HD 0x549ab0: same team, or the same player when the team is 0.
static bool MmSameSide(int a, int b)
{
    int team = *(int*)(pz::g_World->Players[a] + 0x0c);           // World +0x17c + a * 0x48
    if (team != 0)
        return team == *(int*)(pz::g_World->Players[b] + 0x0c);
    return a == b;
}

// PANZERS 0x565f1d (SGameLogic vision tick 0x565e10, the minimap part)
// After the local player's vision map is rebuilt: the fogged copy is the map
// with every pixel the player cannot see halved (>> 1 & 0x7f7f7f, opaque);
// then the texture is updated. Maps up to 0x60 tiles: the HD loop assumes
// the 48-tile border, as here.
static void MinimapFog(pz::SGameLogic* gl, int player)
{
    SHdBitmap* m = s_Mm.Map;
    SHdBitmap* f = s_Mm.Fogged;
    const unsigned char* vis = gl->VisMap[player];
    if (!m || !f || !vis || m->Bpp != 4)
        return;
    int H = pz::g_World->TerrainH, W = pz::g_World->TerrainW;
    float sx = (float)(W * 2 - 0xc0) / (float)m->Width;
    float sz = (float)((0x60 - H) * 2) / (float)m->Height;
    float z0 = (float)(H * 2 - 0x60);
    for (int row = 0; row < m->Height; ++row) {
        const unsigned* src = (const unsigned*)(m->Data + m->Start + m->Pitch * row);
        unsigned* dst = (unsigned*)(f->Data + f->Start + f->Pitch * row);
        int vz = (int)(float)(((double)row + 0.5) * (double)sz + (double)z0);   // fistp, 0xc7f: truncate
        for (int col = 0; col < m->Width; ++col) {
            int vx = (int)(float)(((double)col + 0.5) * (double)sx + 96.0);     // 0x7f7f78
            unsigned c = src[col];
            if ((vis[gl->VisW * vz + vx] & 1) == 0)
                c = (c >> 1 & 0x7f7f7f) | 0xff000000;
            dst[col] = c;
        }
    }
    UploadFogged(gl);
}

// PANZERS 0x5638f0 (the minimap part of SGameLogic::UpdateUnitVisuals)
// Per frame: the map turned with the camera (board +0x50 = World +0x44),
// one dot per unit (+0xa8 clear, +0xa4 add): own green, allied yellow,
// neutral white, seen enemies red (blinking 0xc0ff0000 / 0xc0c00000 every
// 10 logic frames), units flagged +0x110 cyan; the camera's view on the
// ground (+0xac); the minimap markers of +0x194 (0x560eb0) follow the map.
// Recompile: run from the HUD update (PzHudUpdate, after UpdateUnitVisuals)
// with the HD dot rules; the fog update of 0x565f1d runs when the local
// player's vision was rebuilt (PlayerTable +0x234 changed). Single player:
// the multiplayer branches (DAT_008f1a74) are not taken.
void PzMinimapUpdate(SGameView* view)
{
    pz::SGameLogic* gl = pz::g_GameLogic;
    pz::SWorld* w = pz::g_World;
    if (!gl || !w || gl->MinimapFrame < 0)
        return;
    if (s_Mm.Logic != gl)
        PzMinimapCreate(gl);
    if (!s_Mm.Map)
        return;
    int frame = gl->MinimapFrame;
    int local = w->LocalPlayer;                                    // World +0x16c
    // 0x565e10: the fog after a vision rebuild.
    int rebuilt = gl->PlayerTable[local];
    if (rebuilt != s_Mm.FogFrame) {
        s_Mm.FogFrame = rebuilt;
        MinimapFog(gl, local);
    }
    Board->SetMinimapRotation(frame, w->CamYaw);                   // board +0x50
    Board->ClearMinimapDots();                                     // +0xa8
    const float lo = 48.0f;                                        // 0x7f7f90
    int bw = s_Mm.Map->Width, bh = s_Mm.Map->Height;               // *(+0x18c), [1]
    float spanX = (float)(w->TerrainW - 0x60), spanZ = (float)(w->TerrainH - 0x60);
    for (int i = 0; i < w->Units.Size; ++i) {
        if (!w->Units.IsLive(i))
            continue;
        pz::SUnit* u = w->Units.Array[i].Unit;
        if (u->Wrecked || u->Unplaced)                             // +0x150, +0x168
            continue;
        int cls = u->Proto->ClassType;                             // SPUnit +0x40
        if (!(cls == 0 || cls == 5 || cls == 8 || cls == 10 || cls == 0xc || cls == 0xd || cls == 0xb ||
              u->IsCapturable()))                                  // +0x1bc
            continue;
        float x = u->Pos[0], z = u->Pos[2];
        if (!(lo <= x && x <= (float)(w->TerrainW - 0x30) && lo <= z && z <= (float)(w->TerrainH - 0x30)))
            continue;
        float dx = (float)(((double)((x - lo) / spanX) - 0.5) * (double)bw);
        float dy = (float)-(((double)((z - lo) / spanZ) - 0.5) * (double)bh);
        unsigned color;
        if (!u->_110) {
            if (u->Player == local) {
                color = 0xc000ff00;
            } else if (*(int*)(w->Players[u->Player] + 8) == 2 && gl->CanSeeGroundUnit(local, u)) {   // World +0x178
                color = 0xc0ffffff;
            } else if (!MmSameSide(u->Player, local)) {
                if (gl->CanSeeGroundUnit(local, u))                // 0x562760
                    Board->AddMinimapDot(dx, dy, (gl->Frame / 10 & 1) ? 0xc0ff0000 : 0xc0c00000);
                continue;
            } else {
                color = 0xc0ffff00;
            }
        } else {
            if (!gl->CanSeeGroundUnit(local, u))
                continue;
            color = 0xc000ffff;
        }
        Board->AddMinimapDot(dx, dy, color);                       // +0xa4
    }
    // The view on the ground (viewport +0x44 at the camera's focus height).
    float c[12] = {};
    if (view && view->Viewport)
        view->Viewport->GetGroundCorners(w->CamTarget[1], c);      // World +0x3c
    for (int k = 0; k < 4; ++k) {
        float vy = (float)-(((double)((c[k * 3 + 2] - lo) / spanZ) - 0.5) * (double)bh);
        float vx = (float)(((double)((c[k * 3] - lo) / spanX) - 0.5) * (double)bw);
        Board->SetMinimapViewCorner(k, vx, vy);                    // +0xac
    }
    // The markers (+0x194, 0x14 each: x, z, frame, ...): moved with the map.
    const unsigned char* marks = *(unsigned char* const*)((const unsigned char*)gl + 0x194);
    int markCount = GlInt(gl, 0x198);
    float yaw = -w->CamYaw;
    double cy = cos((double)yaw), sy = sin((double)yaw);
    for (int k = 0; k < markCount && marks; ++k) {
        const float* m = (const float*)(marks + k * 0x14);
        int mf = *(const int*)(marks + k * 0x14 + 8);
        if (mf < 0)
            continue;
        float mx = (float)(((double)((m[0] - lo) / spanX) - 0.5) * (double)bw);
        float my = (float)-(((double)((m[1] - lo) / spanZ) - 0.5) * (double)bh);
        float fcy = (float)cy, fsy = (float)sy;
        Board->MoveFrame(mf, (int)((float)((double)(fcy * mx - fsy * my) + 86.0) - (float)GlInt(gl, 0x1a8)),
                         (int)((float)((double)(fsy * mx + fcy * my) + 86.0) - (float)GlInt(gl, 0x1ac)));   // 0x7f7f70
    }
}

// PANZERS 0x570f30 (the board part)
// The "under attack" blink at a unit's minimap position: own units red
// (0xc0ff0000), allied yellow (0xc0ffff00). For SGameLogic::PingAttackedUnit.
void PzMinimapPing(int unit)
{
    pz::SGameLogic* gl = pz::g_GameLogic;
    pz::SWorld* w = pz::g_World;
    if (!gl || !w || gl->MinimapFrame < 0 || !s_Mm.Map || !w->Units.IsLive(unit))
        return;
    pz::SUnit* u = w->Units.Array[unit].Unit;
    int local = w->LocalPlayer;
    unsigned color;
    if (u->Player == local)
        color = 0xc0ff0000;
    else if (MmSameSide(u->Player, local))
        color = 0xc0ffff00;
    else
        return;
    float x = (float)(((double)((u->Pos[0] - 48.0f) / (float)(w->TerrainW - 0x60)) - 0.5) * (double)s_Mm.Map->Width);
    float y = (float)(((double)((u->Pos[2] - 48.0f) / (float)(w->TerrainH - 0x60)) - 0.5) * (double)s_Mm.Map->Height);
    Board->AddMinimapBlink(x, -y, color);                          // board +0xb4(x, -y, color)
}

void PzMinimapRelease()
{
    ReleaseMinimap();
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
