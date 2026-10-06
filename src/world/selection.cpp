// src/world/selection.cpp
// SWorld selection, picking and camera-state rows (world.h block "M3",
// selection.h). OWNER: agent O (docs/M3_INTERFACES.md §4).
//
// Every function walks the unit heap in slot order, computes the unit's new
// selection bit from the mode (selection.h PzSelectMode) and, when a game
// logic exists, sends packet 0x32 / 0x33 for each unit whose bit changes
// (0x576100 / 0x576430) before storing it in unit +0x104.
//
// Engine slots not lifted yet (agent E) have recompile stand-ins, marked
// "stand-in": the model ray distance (model +0xb4, 0x6d59e0: lifted here on
// the model's render position), the building mesh ray test (model +0xd4,
// 0x6db960) and the frustum test of the box (model +0xb8, 0x6d5ae0).

#include <float.h>
#include <math.h>
#include <string.h>
#include "selection.h"
#include "m3common.h"
#include "stub_log.h"
#include "worldapi.h"
#include "unit.h"
#include "punit.h"
#include "gamelogic.h"
#include "packets.h"
#include "logger.h"
#include "iboard.h"
#include "pz/imodel.h"
#include "pz/iviewport.h"

extern SIBoard* Board;   // src/window/widget.h (HD DAT_008f1c60)

namespace pz {

namespace {

bool Live(const SWorld* w, int i)
{
    return i >= 0 && i < w->Units.Size && w->Units.Array[i].Next == kHeapLive;
}

SUnit* At(const SWorld* w, int i)
{
    if (!Live(w, i))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", i);
    return w->Units.Array[i].Unit;
}

int ClassOf(const SUnit* u) { return u->Proto->ClassType; }     // SPUnit +0x40

// World+0x4d0: every unit counts as the local player's (the "godmode" cheat
// of OnKeyDown 0x622f50 toggles it). World+0x50c: buildings (class 9) are
// not selectable. Player kind World+0x178 + p * 0x48 == 4: an allied player
// whose units the local player may select (names guessed).
bool AnyPlayer(const SWorld* w) { return ((const unsigned char*)w)[0x4d0] != 0; }
bool NoBuildings(const SWorld* w) { return ((const unsigned char*)w)[0x50c] != 0; }
int  PlayerKind(const SWorld* w, int p) { return *(const int*)(w->Players[p] + 0x08); }

bool Eligible(const SWorld* w, const SUnit* u)
{
    if (AnyPlayer(w))
        return true;
    if (u->Player == w->LocalPlayer && !u->_110)
        return true;
    return PlayerKind(w, u->Player) == 4;
}

// The unit is a stored member (squad soldier, crew) of a live container
// that is not "open" (+0x7c): its selection follows the container.
bool InsideContainer(const SWorld* w, const SUnit* u)
{
    return Live(w, u->Parent) && !u->_7c;
}

unsigned Hit(unsigned v, unsigned mode)
{
    return (v | ((int)mode >> 2 & 3u)) ^ ((int)mode >> 4);
}

// The common tail: packet 0x32 / 0x33 for a changed bit, then the store.
void Store(SWorld* w, int i, unsigned v)
{
    SUnit* u = At(w, i);
    if (g_GameLogic && ((u->_104 ^ v) & 1)) {
        if (v & 1)
            Pkt_32(g_GameLogic, (unsigned short)i);               // 0x576100
        else
            Pkt_33(g_GameLogic, (unsigned short)i);               // 0x576430
    }
    u->_104 = v;
}

// PANZERS 0x6d59e0 (SModel +0xb4) on the model's render position.
// Squared distance of the model origin from the ray, 3.4e38 when the model
// is hidden or behind the ray origin.
float ModelRayDistSq(const SUnit* u, const float* ray)
{
    SIModel* m = u->Model;
    if (!m || !m->GetVisible())                                   // +0xd4
        return 3.4028235e+38f;                                    // DAT_007fa570
    float p[3];
    m->GetRenderPosition(p);                                      // +0x88 (interpolated when +0xdc & 1)
    float dx = p[0] - ray[0];
    float dy = p[1] - ray[1];
    float dz = p[2] - ray[2];
    float t = ray[4] * dy + dx * ray[3] + ray[5] * dz;
    if (0.0f <= t)
        return (dy * dy + dx * dx + dz * dz) - t * t;
    return 3.4028235e+38f;
}

// Stand-in for the building mesh ray test (SModel +0xd4, 0x6db960: the ray
// against the boxes of the model's nodes): the ray passes within the unit
// size of the building's origin.
bool ModelRayHit(const SUnit* u, const float* ray)
{
    float r = u->UnitSize;
    return ModelRayDistSq(u, ray) < r * r;
}

// Stand-in for the frustum test (SModel +0xb8, 0x6d5ae0) of the box select.
bool ModelInRect(const SUnit* u, const SPzSelectRect* box)
{
    SIModel* m = u->Model;
    if (!m || !m->GetVisible() || !box || !box->Viewport)
        return false;
    float p[3];
    m->GetRenderPosition(p);
    float x, y, size, z;
    int fog;
    box->Viewport->ProjectToScreen(p, 1.0f, &x, &y, &size, &z, &fog);   // +0x3c
    if (size < 0.0f)
        return false;
    int x0 = box->X0 < box->X1 ? box->X0 : box->X1, x1 = box->X0 < box->X1 ? box->X1 : box->X0;
    int y0 = box->Y0 < box->Y1 ? box->Y0 : box->Y1, y1 = box->Y0 < box->Y1 ? box->Y1 : box->Y0;
    return x >= (float)x0 && x <= (float)x1 && y >= (float)y0 && y <= (float)y1;
}

// HD normalises the ray direction in place (double reciprocal of the length).
void NormalizeRay(float* ray)
{
    double len = sqrt((double)(ray[3] * ray[3] + ray[4] * ray[4] + ray[5] * ray[5]));
    double inv = 1.0 / len;                                       // _DAT_007eed98
    ray[3] = (float)((double)ray[3] * inv);
    ray[4] = (float)((double)ray[4] * inv);
    ray[5] = (float)((double)ray[5] * inv);
}

void UnitVoiceSelected(SUnit* u)
{
    u->SpeakSelected();                                           // +0x44 (0x5bce20): the "selected" voice
}

} // namespace

// PANZERS 0x5fc050
// The pick: the nearest own (or allied) unit whose model the ray passes
// within 0.4 * size (at least 1.5) of, a stored unit counting as its
// container; buildings and class 10 by their mesh. Then every unit gets the
// mode, the picked one the "hit" part of it.
int SWorld::PickUnitAt(float* ray, unsigned mode)
{
    PZ_M3_TRACE("SWorld::PickUnitAt (0x5fc050)");
    float best = 1.44f;                                           // DAT_00801ae0
    int pick = -1;
    NormalizeRay(ray);
    for (int i = 0; i < Units.Size; ++i) {
        if (!Live(this, i))
            continue;
        SUnit* u = At(this, i);
        if (u->Wrecked)                                           // +0x150
            continue;
        if (!Eligible(this, u))
            continue;
        int cls = ClassOf(u);
        if (cls == 9) {
            if (NoBuildings(this))
                continue;
        } else if (cls != 10) {
            float d = ModelRayDistSq(u, ray);
            if (d < best) {
                double s = (double)u->UnitSize < 1.5 ? 1.5 : (double)u->UnitSize;   // DAT_007fe0c8
                if ((double)d < s * 0.4000000059604645 * s * 0.4000000059604645) {  // DAT_007f5e28
                    best = d;
                    if (!Live(this, u->Parent) || u->_7c) {
                        if (!u->Unplaced && u->_112)
                            pick = i;
                    } else {
                        SUnit* c = At(this, u->Parent);
                        if (!c->Unplaced && c->_112)
                            pick = u->Parent;
                    }
                }
            }
            continue;
        }
        if (u->Proto->Selectable && ModelRayHit(u, ray)) {        // SPUnit +0x8c, model +0xd4
            best = 0.0f;
            pick = i;
        }
    }
    for (int i = 0; i < Units.Size; ++i) {
        if (!Live(this, i))
            continue;
        SUnit* u = At(this, i);
        unsigned v = u->_104 & mode & 3;
        if (pick == i) {
            v = Hit(v, mode);
            if ((mode & 0x14) && (v & 1))
                UnitVoiceSelected(At(this, pick));
        }
        Store(this, i, v);
    }
    return pick;
}

// PANZERS 0x5fc5b0
void SWorld::SelectUnitsInBox(const SPzSelectRect* box, unsigned mode)
{
    PZ_M3_TRACE("SWorld::SelectUnitsInBox (0x5fc5b0)");
    for (int i = 0; i < Units.Size; ++i) {
        if (!Live(this, i))
            continue;
        SUnit* u = At(this, i);
        if (u->Wrecked)
            continue;
        unsigned v = u->_104 & mode & 3;
        if (Eligible(this, u) && (ClassOf(u) != 9 || !NoBuildings(this)) && !InsideContainer(this, u)
            && !u->Unplaced && u->_112 && ModelInRect(u, box)) {  // model +0xb8 (stand-in)
            v = Hit(v, mode);
            if (mode & 0x10)
                UnitVoiceSelected(u);
        }
        Store(this, i, v);
    }
}

// PANZERS 0x5fc860
void SWorld::ApplySelectionToAll(unsigned mode)
{
    PZ_M3_TRACE("SWorld::ApplySelectionToAll (0x5fc860)");
    for (int i = 0; i < Units.Size; ++i) {
        if (!Live(this, i))
            continue;
        SUnit* u = At(this, i);
        if (u->Wrecked)
            continue;
        unsigned v = u->_104 & mode & 3;
        if (Eligible(this, u) && (ClassOf(u) != 9 || !NoBuildings(this)) && !InsideContainer(this, u)
            && u->Proto->Selectable && !u->Unplaced)
            v = Hit(v, mode);
        Store(this, i, v);
    }
}

// PANZERS 0x5fcb10
void SWorld::SelectUnit(int unit, unsigned mode)
{
    PZ_M3_TRACE("SWorld::SelectUnit (0x5fcb10)");
    for (int i = 0; i < Units.Size; ++i) {
        if (!Live(this, i))
            continue;
        SUnit* u = At(this, i);
        unsigned v = u->_104 & mode & 3;
        if (i == unit && (ClassOf(u) != 9 || !NoBuildings(this)) && !InsideContainer(this, u)
            && u->Proto->Selectable && !u->Unplaced)
            v = Hit(v, mode);
        Store(this, i, v);
    }
}

// PANZERS 0x5fcd10
// Double click: every unit in view of the clicked unit's prototype.
void SWorld::SelectSameType(const SPzSelectRect* box, int unit, unsigned mode)
{
    PZ_M3_TRACE("SWorld::SelectSameType (0x5fcd10)");
    if (!Live(this, unit))
        return;
    for (int i = 0; i < Units.Size; ++i) {
        if (!Live(this, i))
            continue;
        SUnit* u = At(this, i);
        if (u->Wrecked)
            continue;
        unsigned v = u->_104 & mode & 3;
        if (Eligible(this, u) && !InsideContainer(this, u) && !u->Unplaced && u->_112) {
            const SPUnit* a = At(this, unit)->Proto;
            const SPUnit* b = u->Proto;
            bool same = a->Name.size == b->Name.size
                        && (a->Name.size == 0 || _stricmp(a->Name.buf, b->Name.buf) == 0);   // +0x64, +0x60
            if (same && ModelInRect(u, box)) {
                v = Hit(v, mode);
                if ((mode & 0x14) && (v & 1))
                    UnitVoiceSelected(u);
            }
        }
        Store(this, i, v);
    }
}

// PANZERS 0x5fd030
void SWorld::SelectByClass(unsigned mode, bool vehicles, bool classB, bool squads)
{
    PZ_M3_TRACE("SWorld::SelectByClass (0x5fd030)");
    for (int i = 0; i < Units.Size; ++i) {
        if (!Live(this, i))
            continue;
        SUnit* u = At(this, i);
        if (u->Wrecked)
            continue;
        unsigned v = u->_104 & mode & 3;
        if (Eligible(this, u) && !InsideContainer(this, u) && !u->Unplaced && u->_112) {
            int cls = ClassOf(u);
            if ((vehicles && cls == 0) || (squads && cls == 5) || (classB && cls == 0xb)) {
                v = Hit(v, mode);
                if ((mode & 0x14) && (v & 1))
                    UnitVoiceSelected(u);
            }
        }
        Store(this, i, v);
    }
}

// PANZERS 0x5fd2f0
// Key n: exactly the placed units of group n (+0x10c). Sends 0x32 / 0x33 for
// every unit, changed or not. World+0xa0 = n when the selection was already
// that group (a second press; the view centres on it).
void SWorld::SelectGroup(int group)
{
    PZ_M3_TRACE("SWorld::SelectGroup (0x5fd2f0)");
    bool same = true;
    for (int i = 0; i < Units.Size; ++i) {
        if (!Live(this, i))
            continue;
        SUnit* u = At(this, i);
        if (*(int*)((unsigned char*)u + 0x10c) == group && !u->Unplaced) {
            if (!(u->_104 & 1))
                same = false;
            u->_104 |= 1;
            if (g_GameLogic)
                Pkt_32(g_GameLogic, (unsigned short)i);
        } else {
            if (u->_104 & 1)
                same = false;
            u->_104 &= ~1u;
            if (g_GameLogic)
                Pkt_33(g_GameLogic, (unsigned short)i);
        }
    }
    if (same)
        *(int*)((unsigned char*)this + 0xa0) = group;
}

// PANZERS 0x5e3660
// Ctrl+n: the selected units join group n, the others leave it.
void SWorld::AssignGroup(int group)
{
    PZ_M3_TRACE("SWorld::AssignGroup (0x5e3660)");
    for (int i = 0; i < Units.Size; ++i) {
        if (!Live(this, i))
            continue;
        SUnit* u = At(this, i);
        int* g = (int*)((unsigned char*)u + 0x10c);
        if (u->_104 & 1)
            *g = group;
        else if (*g == group)
            *g = 0;
    }
}

// PANZERS 0x5ef660
bool SWorld::IsUnitSelected(int unit)
{
    if (!Live(this, unit))
        return false;
    return (At(this, unit)->_104 & 1) != 0;
}

// PANZERS 0x5e0d70
int SWorld::CountSelectedUnits()
{
    int n = 0;
    for (int i = 0; i < Units.Size; ++i)
        if (Live(this, i) && (At(this, i)->_104 & 1))
            ++n;
    return n;
}

// PANZERS 0x5ebac0
// The order target under the ray: as the pick, but any player's unit the
// local player can see (CanSeeGroundUnit 0x562760, unless World+0x4d0), and
// without touching the selection.
int SWorld::PickAnyUnitAt(float* ray)
{
    float best = 1.44f;
    int pick = -1;
    NormalizeRay(ray);
    for (int i = 0; i < Units.Size; ++i) {
        if (!Live(this, i))
            continue;
        SUnit* u = At(this, i);
        if (u->Wrecked)
            continue;
        int cls = ClassOf(u);
        if (!AnyPlayer(this) && g_GameLogic && cls != 9
            && !g_GameLogic->CanSeeGroundUnit(LocalPlayer, static_cast<SIUnit*>(u)))
            continue;
        if (cls == 9) {
            if (NoBuildings(this))
                continue;
        } else if (cls != 10) {
            float d = ModelRayDistSq(u, ray);
            if (d < best) {
                double s = (double)u->UnitSize < 1.5 ? 1.5 : (double)u->UnitSize;
                if ((double)d < s * 0.4000000059604645 * s * 0.4000000059604645) {
                    best = d;
                    if (!Live(this, u->Parent) || u->_7c) {
                        if (!u->Unplaced && u->_112)
                            pick = i;
                    } else {
                        SUnit* c = At(this, u->Parent);
                        if (!c->Unplaced && c->_112)
                            pick = u->Parent;
                    }
                }
            }
            continue;
        }
        if (u->Proto->Selectable && ModelRayHit(u, ray)) {
            best = 0.0f;
            pick = i;
        }
    }
    return pick;
}

// PANZERS 0x5ea910
// Marches the ray one tile at a time along its major horizontal axis from
// where it enters the map and returns the first point under the water (when
// the water is 1 cm above the ground) or the ground, interpolated inside the
// tile; x = z = -1 (y = 0) when it leaves the map or points up.
void SWorld::RayTerrain(const float* ray, float* outX, float* outY, float* outZ)
{
    float x = ray[0], y = ray[1], z = ray[2];
    float dx = ray[3], dy = ray[4], dz = ray[5];
    const float W = (float)TerrainW, H = (float)TerrainH;         // +0xdc, +0xe0
    bool water = false;
    if (dy < 0.0f) {
        if (fabs((double)dx) <= fabs((double)dz)) {
            float sy = dy / dz;
            if (dz <= 0.0f) {
                float sx = -(dx / dz);
                sy = -sy;
                if (H < z) {
                    y = (z - H) * sy + y;
                    x = x + (z - H) * sx;
                    z = H;
                }
                while (0.0f <= z) {
                    float w = GetWaterHeight(x, z);
                    if (y < w && 0.01f < w - GetTerrainHeight(x, z)) {
                        float t = (w - y) / ((w - GetWaterHeight(x - sx, z + 1.0f)) - sy);
                        z = t + z;
                        *outX = x - t * sx;
                        water = true;
                        goto hit;
                    }
                    float g = GetTerrainHeight(x, z);
                    if (y < g) {
                        float t = (g - y) / ((g - GetTerrainHeight(x - sx, z + 1.0f)) - sy);
                        z = t + z;
                        *outX = x - t * sx;
                        goto hit;
                    }
                    z = z - 1.0f;
                    y = sy + y;
                    x = x + sx;
                }
            } else {
                float sx = dx / dz;
                if (z < 0.0f) {
                    x = x - sx * z;
                    y = y - sy * z;
                    z = 0.0f;
                }
                while (z < H) {
                    float w = GetWaterHeight(x, z);
                    if (y < w && 0.01f < w - GetTerrainHeight(x, z)) {
                        float t = (w - y) / ((w - GetWaterHeight(x - sx, z - 1.0f)) - sy);
                        *outX = x - t * sx;
                        z = z - t;
                        water = true;
                        goto hit;
                    }
                    float g = GetTerrainHeight(x, z);
                    if (y < g) {
                        float t = (g - y) / ((g - GetTerrainHeight(x - sx, z - 1.0f)) - sy);
                        *outX = x - t * sx;
                        z = z - t;
                        goto hit;
                    }
                    y = sy + y;
                    z = z + 1.0f;
                    x = x + sx;
                }
            }
        } else {
            float sy = dy / dx;
            if (dx <= 0.0f) {
                float sz = -(dz / dx);
                sy = -sy;
                if (W < x) {
                    y = (x - W) * sy + y;
                    z = z + (x - W) * sz;
                    x = W;
                }
                while (0.0f <= x) {
                    float w = GetWaterHeight(x, z);
                    if (y < w && 0.01f < w - GetTerrainHeight(x, z)) {
                        float t = (w - y) / ((w - GetWaterHeight(x + 1.0f, z - sz)) - sy);
                        *outX = t + x;
                        z = z - sz * t;
                        water = true;
                        goto hit;
                    }
                    float g = GetTerrainHeight(x, z);
                    if (y < g) {
                        float t = (g - y) / ((g - GetTerrainHeight(x + 1.0f, z - sz)) - sy);
                        *outX = t + x;
                        z = z - sz * t;
                        goto hit;
                    }
                    x = x - 1.0f;
                    z = z + sz;
                    y = sy + y;
                }
            } else {
                float sz = dz / dx;
                if (x < 0.0f) {
                    z = z - sz * x;
                    y = y - sy * x;
                    x = 0.0f;
                }
                while (x < W) {
                    float w = GetWaterHeight(x, z);
                    if (y < w && 0.01f < w - GetTerrainHeight(x, z)) {
                        float t = (w - y) / ((w - GetWaterHeight(x - 1.0f, z - sz)) - sy);
                        *outX = x - t;
                        z = z - sz * t;
                        water = true;
                        goto hit;
                    }
                    float g = GetTerrainHeight(x, z);
                    if (y < g) {
                        float t = (g - y) / ((g - GetTerrainHeight(x - 1.0f, z - sz)) - sy);
                        *outX = x - t;
                        z = z - sz * t;
                        goto hit;
                    }
                    y = sy + y;
                    x = x + 1.0f;
                    z = z + sz;
                }
            }
        }
    }
    *outX = -1.0f;
    *outY = 0.0f;
    *outZ = -1.0f;
    return;
hit:
    *outZ = z;
    if (!(0.0f <= *outX && *outX < W && 0.0f <= z && z < H)) {
        *outX = -1.0f;
        *outZ = -1.0f;
    }
    *outY = water ? GetWaterHeight(*outX, *outZ) : GetTerrainHeight(*outX, *outZ);
}

// PANZERS 0x5fd630
// The drag box: four 1-pixel board frames (+0x114..+0x120) with the corner
// glyphs 6..9 of the selection icon set (+0x110).
void SWorld::DrawSelectionBox(int x0, int y0, int x1, int y1)
{
    int left = x0, right = x1;
    int c = x0 <= x1 ? 1 : 0;                                     // the corner glyphs follow the drag direction
    if (!c) {
        left = x1;
        right = x0;
    }
    if (y1 < y0) {
        int t = y1;
        y1 = y0;
        y0 = t;
        c = c == 0;
    }
    if (!Board || BoardFrames[0] < 0)                             // frames of the SWorld ctor (not created yet)
        return;
    Board->SetSpriteGlyph(BoardFrames[0], BoardIconSet, c + 8);   // HD board +0x24
    Board->MoveFrame(BoardFrames[0], left, y0);                   // +0x10
    Board->ResizeFrame(BoardFrames[0], 1, y1 - y0);               // +0x14
    Board->SetSpriteGlyph(BoardFrames[1], BoardIconSet, 9 - c);
    Board->MoveFrame(BoardFrames[1], right, y0);
    Board->ResizeFrame(BoardFrames[1], 1, y1 - y0);
    Board->SetSpriteGlyph(BoardFrames[2], BoardIconSet, c + 6);
    Board->MoveFrame(BoardFrames[2], left, y0);
    Board->ResizeFrame(BoardFrames[2], right - left, 1);
    Board->SetSpriteGlyph(BoardFrames[3], BoardIconSet, 7 - c);
    Board->MoveFrame(BoardFrames[3], left, y1);
    Board->ResizeFrame(BoardFrames[3], right - left, 1);
}

// PANZERS 0x5ddb60
void SWorld::HideSelectionBox()
{
    if (!Board)
        return;
    for (int k = 0; k < 4; ++k)
        if (BoardFrames[k] >= 0)
            Board->SetSpriteGlyph(BoardFrames[k], -1, 0);         // board +0x24(frame, -1, 0)
}

// SWorld::ShowUnitRange 0x5fee00: worldcamera.cpp (agent V).

// PANZERS 0x5e6a70
void SWorld::GetCameraState(unsigned* out5)
{
    const unsigned* w = (const unsigned*)this;
    out5[0] = w[0x38 / 4];   // CamTarget x
    out5[1] = w[0x40 / 4];   // CamTarget z
    out5[2] = w[0x44 / 4];   // CamYaw
    out5[3] = w[0x50 / 4];
    out5[4] = w[0x54 / 4];   // CamDist
}

} // namespace pz
