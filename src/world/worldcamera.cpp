// src/world/worldcamera.cpp
// The game-view camera moves of SWorld (scroll, rotate, zoom, jump), the
// weapon range overlay ShowUnitRange, and the SGameWorld camera-spline clock
// (HD global object 0x929a60). OWNER: agent V (docs/M3_INTERFACES.md §4).
//
// All of this is visual: none of it reads or writes a field the world CRC
// hashes (0x56aa10), and the camera state is only recorded into replays
// (GetCameraState 0x5e6a70), never fed back into the logic.

#include <math.h>
#include <string.h>
#include "world.h"
#include "unit.h"
#include "punit.h"
#include "m3common.h"
#include "pz/iterrain.h"
#include "logger.h"
#include "timer.h"

namespace pz {

// PANZERS 0x5f4dc0
// Scroll the camera target: `forward` along the view direction, `right`
// across it, both scaled by the distance (zoomed out scrolls faster). Callers:
// the key scroll 0x62c470 and the edge scroll of 0x620bc0 (speed * ms), the
// mouse-drag mode 7 of 0x620bc0, the minimap 0x6449a0, replay 0x565390.
void SWorld::MoveCamera(float forward, float right)
{
    if (CamMode != 0) {
        // Free camera (mode 1): +0xc8 / +0xcc move at 30 units per unit.
        float* free = (float*)((unsigned char*)this + 0xc8);
        free[0] = forward * 30.0f + free[0];                       // 0x7fb6b8 = 30.0f
        free[1] = right * 30.0f + free[1];
        return;
    }
    float k = (float)((double)CamDist * 0.075);                    // 0x801ad8
    float z = (CamYawCos * forward - CamYawSin * right) * k + CamTarget[2];
    float x = (CamYawCos * right + CamYawSin * forward) * k + CamTarget[0];
    if (CamLocked != 0)
        return;
    if (CamXMin > x || x > CamXMax)
        x = (CamXMin > x) ? CamXMin : CamXMax;
    if (CamZMin > z || z > CamZMax)
        z = (CamZMin > z) ? CamZMin : CamZMax;
    CamTarget[0] = x;
    CamTarget[2] = z;
    CamFollowPath = 0;
    CamFollowUnit = -1;
}

// PANZERS 0x5f8380
void SWorld::RotateCamera(float yaw, float pitch)
{
    SetCameraAngles(CamYaw + yaw, CamPitch + pitch);               // 0x5f83b0
}

// PANZERS 0x609390
void SWorld::ZoomCamera(float delta)
{
    float d = delta + CamDist;
    float v = CamDistMin;
    if (CamDistMin <= d) {
        v = CamDistMax;
        if (d <= CamDistMax) {
            CamDist = d;
            return;
        }
    }
    CamDist = v;
}

// PANZERS 0x5f4f60
void SWorld::SetCameraTarget(float x, float z)
{
    if (CamLocked != 0)
        return;
    if (x < CamXMin || CamXMax < x)
        x = (x < CamXMin) ? CamXMin : CamXMax;
    if (z < CamZMin || CamZMax < z)
        z = (z < CamZMin) ? CamZMin : CamZMax;
    CamTarget[0] = x;
    CamTarget[2] = z;
    CamFollowPath = 0;
    CamFollowUnit = -1;
}

// ---------------------------------------------------------------------------
// ShowUnitRange

static int RoundToInt(float v)
{
    // HD: FLD float / FISTP dword [0x92e350] (round to nearest, the default
    // x87 control word of the game).
    return (int)lrintf(v);
}

static SUnit* RangeUnitPtr(SWorld* w, int unit)
{
    if (!w->Units.IsLive(unit))
        Logger.g->Panic("SHeapTRB::operator[]: invalid index (%d)", unit);   // 0x5ffbfa
    return w->Units.Array[unit].Unit;
}

// PANZERS 0x5fee00
// Draws the weapon range of one unit as rings of terrain effect decals
// (texture +0x520): the max range ring (+0x528 / +0x530, red when the unit
// has gunners (+0x44 > -1), else green) and the min range ring (+0x524 /
// +0x52c, orange). The rings turn with the timer seconds. A building with
// window sets (unit +0x3e8 / +0x3ec, stride 0x48: x, z, dir) gets one static
// 135-degree arc and two radial lines per window instead. -1 removes the
// overlay. Called by SGameView::Update 0x628430 every frame with the single
// selected unit, and by 0x5e5580.
void SWorld::ShowUnitRange(int unit)
{
    PZ_M3_TRACE("SWorld::ShowUnitRange (0x5fee00)");
    if (!Units.IsLive(unit))
        unit = -1;
    SITerrain* terrain = Terrain;
    if (RangeUnit >= 0 && RangeUnit != unit) {
        if (MaxRangeCount > 0) {
            for (int i = 0; i < MaxRangeCount; ++i)
                terrain->DestroyEffectDecal(MaxRangeDecals[i]);    // terrain +0x64
            delete[] MaxRangeDecals;                               // 0x76654a(p, 4)
        }
        if (MinRangeCount != 0) {
            for (int i = 0; i < MinRangeCount; ++i)
                terrain->DestroyEffectDecal(MinRangeDecals[i]);
            delete[] MinRangeDecals;
        }
    }
    if (unit < 0) {
        RangeUnit = unit;
        return;
    }
    SUnit* u = RangeUnitPtr(this, unit);
    int windows = *(int*)((unsigned char*)u + 0x3ec);
    if (u->Proto->ClassType == 9 && windows != 0) {
        // Building with window sets: static arcs.
        if (unit == RangeUnit)
            return;
        float r = u->GetMaxRange(u->MainGunner);                   // vtbl +0x17c(unit +0x44)
        unsigned color = (u->MainGunner > -1) ? 0xffff0000u : 0xff00ff00u;
        float arcF = (float)((double)(r * 2.3561945f) * 1.5);      // 0x801aec = 3/4 pi, 0x7fe0c8 = 1.5
        int arc = RoundToInt(arcF) - 1;
        if (arc < 0)
            arc = 0;
        float lineF = (float)((double)r * 1.5);
        int line = RoundToInt(lineF) - 2;
        if (line < 0)
            line = 0;
        MaxRangeCount = (arc + line * 2) * windows;
        if (MaxRangeCount != 0) {
            MaxRangeDecals = new int[MaxRangeCount];               // 0x766b87
            int n = 0;
            const unsigned char* sets = *(unsigned char**)((unsigned char*)u + 0x3e8);
            for (int w = 0; w < windows; ++w) {
                const float* p = (const float*)(sets + w * 0x48);
                for (int i = 0; i < arc; ++i) {
                    float a = (((float)i + 1.0f) / (float)arc - 0.5f) * 2.3561945f + p[2];
                    float x = (float)(sin((double)a) * (double)r + (double)p[0]);   // 0x78d640
                    float z = (float)(cos((double)a) * (double)r + (double)p[1]);   // 0x78d480
                    MaxRangeDecals[n++] = terrain->CreateEffectDecal(RangeTexture, x, z, 0.0f, 1.0f,
                                                                     color, 0, true, true);   // +0x60
                }
                float a0 = p[2] - 1.1780972f;                      // 0x801ad0 = 3/8 pi
                float a1 = p[2] + 1.1780972f;
                for (int k = 1; k <= line; ++k) {
                    float d = ((float)k * r) / (float)line;
                    float x = (float)(sin((double)a0) * (double)d + (double)p[0]);
                    float z = (float)(cos((double)a0) * (double)d + (double)p[1]);
                    MaxRangeDecals[n++] = terrain->CreateEffectDecal(RangeTexture, x, z, 0.0f, 1.0f,
                                                                     color, 0, true, true);
                    x = (float)(sin((double)a1) * (double)d + (double)p[0]);
                    z = (float)(cos((double)a1) * (double)d + (double)p[1]);
                    MaxRangeDecals[n++] = terrain->CreateEffectDecal(RangeTexture, x, z, 0.0f, 1.0f,
                                                                     color, 0, true, true);
                }
            }
            if (n != MaxRangeCount)
                Logger.g->Panic("SWorld::ShowUnitRange: Internal error (j(%d) != NumMaxRangeShaders(%d))",
                                n, MaxRangeCount);
        }
        RangeUnit = unit;
        MinRangeCount = 0;
        return;
    }

    float t = (float)((double)Timer.GetTickValue() / 1000.0);      // 0x661800
    if (unit == RangeUnit) {
        // Same unit: move the rings with it and turn them.
        if (MaxRangeCount != 0) {
            float r = u->GetMaxRange(u->MainGunner);
            for (int i = 0; i < MaxRangeCount; ++i) {
                float a = (((float)i + t) * 2.0f * 3.1415927f) / (float)MaxRangeCount;   // 0x7f4558 * 0x7f4584
                float c[3], c2[3];
                u->GetCenterPosition(c);                           // vtbl +0x1c8
                float x = (float)(sin((double)a) * (double)r + (double)c[0]);
                u->GetCenterPosition(c2);
                float z = (float)((double)c2[2] + cos((double)a) * (double)r);
                terrain->SetEffectDecalPosition(MaxRangeDecals[i], x, z);   // +0x68
            }
        }
        if (MinRangeCount != 0) {
            float r = u->GetMinRange(u->MainGunner);               // vtbl +0x180
            for (int i = 0; i < MinRangeCount; ++i) {
                float a = (((float)i - t) * 2.0f * 3.1415927f) / (float)MinRangeCount;
                float c[3], c2[3];
                u->GetCenterPosition(c);
                float x = (float)(sin((double)a) * (double)r + (double)c[0]);
                u->GetCenterPosition(c2);
                float z = (float)((double)c2[2] + cos((double)a) * (double)r);
                terrain->SetEffectDecalPosition(MinRangeDecals[i], x, z);
            }
        }
        RangeUnit = unit;
        return;
    }

    // New unit: create the rings at its position.
    float r = u->GetMaxRange(u->MainGunner);
    unsigned color = (u->MainGunner > -1) ? 0xffff0000u : 0xff00ff00u;
    MaxRangeCount = RoundToInt((float)((double)(r * 2.0f * 3.1415927f) * 1.5));
    if (MaxRangeCount != 0) {
        MaxRangeDecals = new int[MaxRangeCount];
        for (int i = 0; i < MaxRangeCount; ++i) {
            float a = (((float)i + t) * 2.0f * 3.1415927f) / (float)MaxRangeCount;
            float x = (float)(sin((double)a) * (double)r + (double)u->Pos[0]);
            float z = (float)(cos((double)a) * (double)r + (double)u->Pos[2]);
            MaxRangeDecals[i] = terrain->CreateEffectDecal(RangeTexture, x, z, 0.0f, 1.0f, color, 0, true, true);
        }
    }
    r = u->GetMinRange(u->MainGunner);
    MinRangeCount = RoundToInt((float)((double)(r * 2.0f * 3.1415927f) * 1.5));
    if (MinRangeCount != 0) {
        MinRangeDecals = new int[MinRangeCount];
        for (int i = 0; i < MinRangeCount; ++i) {
            float a = (((float)i - t) * 2.0f * 3.1415927f) / (float)MinRangeCount;
            float x = (float)(sin((double)a) * (double)r + (double)u->Pos[0]);
            float z = (float)(cos((double)a) * (double)r + (double)u->Pos[2]);
            MinRangeDecals[i] = terrain->CreateEffectDecal(RangeTexture, x, z, 0.0f, 1.0f, 0xffffc000u, 0, true, true);
        }
    }
    RangeUnit = unit;
}

// ---------------------------------------------------------------------------
// SGameWorld (HD global 0x929a60): the camera-spline recorder / player of the
// -csrec / -csplay switches and its clock. Only the clock is used in a normal
// game. Recompile layout: just the fields the view reads.

struct SGameWorldCamera {
    float Time;        // +0xb0 timer seconds at the last tick
    float Elapsed;     // +0xb4 seconds since InitCameraSpline (clamped steps)
    bool  Started;     // +0xb8
};
static SGameWorldCamera g_GameWorldCamera;   // HD 0x929a60 +0xb0

// PANZERS 0x609760
// HD: SGameWorld::InitCameraSpline(file), called by mission start 0x6281a0
// and the load-game variant 0x61f840 with the -csplay file (0x929d38) or "".
// The -csrec (Settings +0x4c) and -csplay (+0x4d, +0x58) branches read or
// open the camera spline files; they are not lifted (debug switches).
void SWorld::InitCameraSpline(const char* file)
{
    PZ_M3_TRACE("SGameWorld::InitCameraSpline (0x609760)");
    SGameWorldCamera& g = g_GameWorldCamera;
    g.Elapsed = 0.0f;
    if (g.Started)
        return;
    g.Started = true;
    g.Time = (float)((double)Timer.GetTickValue() / 1000.0);      // 0x661800
    if (file && *file)
        Logger.g->Log(1, "SGameWorld::InitCameraSpline: camera spline switches not lifted (%s)", file);
}

// PANZERS 0x6096f0
// Per frame from SGameView::Update 0x628430: advance the spline clock by the
// elapsed seconds, at most 0.125 s per frame (0x7f7f40).
void PzCameraSplineTick()
{
    SGameWorldCamera& g = g_GameWorldCamera;
    float now = (float)((double)Timer.GetTickValue() / 1000.0);
    float dt = now - g.Time;
    g.Time = (float)((double)Timer.GetTickValue() / 1000.0);
    if (0.125f < dt)
        dt = 0.125f;
    g.Elapsed = dt + g.Elapsed;
}

} // namespace pz
