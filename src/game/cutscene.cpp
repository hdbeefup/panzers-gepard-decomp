// src/game/cutscene.cpp
// In-game cut-scenes, logic side (M4 agent T). See cutscene.h.
// HD: SGameLogic 0x56ea20 / 0x56f530 / 0x568af0 / 0x565390 and
// SInGameAnimLogic::LoadProject 0x589d90 (global object at 0x929100).
//
// What runs here equals HD for the game state: the statement lines of the
// .ingame script are executed by SGameLogic::ExecuteScriptStatement at their
// frame, the world seed is reset by LoadProject, the units are stopped at the
// start, triggers are suspended while +0x2b8 is set. What is not done: the
// camera splines and fades of SInGameAnimLogic (0x589970, 0x58b170,
// 0x58ae50, 0x58b760), the subtitles (0x56fd70 / 0x5826c0), the letterbox
// and the mp3. The view keeps the player camera during the cut-scene.

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "cutscene.h"
#include "gamelogic.h"
#include "trigger.h"
#include "triggersunits.h"
#include "worldapi.h"
#include "world.h"
#include "unit.h"
#include "unitextern.h"
#include "core_common.h"
#include "stream.h"
#include "logger.h"
#include "stub_log.h"
#include "campaign.h"
#include "pz/iscene.h"

namespace pz {

using m2u::UV;

void PzMessagesClearFading(SGameLogic* gl);                        // triggers_msg.cpp, 0x563860
void PzMessagesClearStatic(SGameLogic* gl);                        // triggers_msg.cpp, 0x5638b0

namespace {

// One ISL0 line of the .ingame project (SInGameAnimLogic list, 0x14 each,
// sorted by frame by 0x5883f0).
struct SAnimLine {
    int   Type;      // +0x00 0 statement, 1 camera switch (ISLI), 2 camera colour
    int   Enabled;   // +0x04
    int   Frame;     // +0x08
    char* Text;      // +0x0c SString
};

// The state HD keeps in SInGameAnimLogic (0x929100) and in SGameLogic
// (+0x50 statement list, +0x2bc start frame, +0x2c0 saved camera,
// +0x2d4 the cut-scene sound).
struct SCutscene {
    bool      Running;           // SGameLogic +0x2b8
    int       Length;            // SInGameAnimLogic +0x10 (LAFN, frames)
    int       StartFrame;        // SGameLogic +0x2bc
    unsigned  SavedCamera[5];    // SGameLogic +0x2c0 (GetCameraState 0x5e6a70)
    int       SavedOverlay;      // (recompile) +0x264 before the start; HD re-reads the option (0x64df00)
    SAnimLine* Lines;            // SInGameAnimLogic +0x00 SDArray
    int       LineCount;
    struct SStatement { int Frame; char* Text; };
    SStatement* Statements;      // SGameLogic +0x50 SDArray (0x0c each in HD)
    int       StatementCount;
    int       SoundId = -1;      // SGameLogic +0x2d4 (Concert +0x2c)
};
SCutscene g_Cut;

// ---------------------------------------------------------------------------
// The camera tracks (SInGameAnimLogic +0x38 raw keys, +0x148 position
// splines, +0x2d8 / +0x418 / +0x558 pitch / yaw / distance splines, +0x698
// type, +0x6e8 near, +0x738 far, +0x128 the camera of each frame).

// Raw key of the file (0x28): frame, target x / y / z, yaw and pitch in
// degrees, distance, tension, continuity, bias.
struct SCamKey { int Frame; float X, Y, Z, Yaw, Pitch, Dist, T, C, B; };
// Position spline key (0x3c, evaluated by 0x6749e0).
struct SKey3 { float t, p[3], in[3], out[3], T, C, B, easeTo, easeFrom; };
// Float spline key (0x24, evaluated by 0x676200).
struct SKey1 { float t, v, in, out, T, C, B, easeTo, easeFrom; };

const int kCams = 20;                                             // NUMBER_OF_CAMERAS (0x14)
struct SCamera {
    SCamKey* Raw;  int RawCount;                                  // +0x38 + c * 0xc
    SKey3* Pos;    int PosCount;                                  // +0x148 + c * 0x14
    SKey1* Pitch;  int PitchCount;                                // +0x2d8 + c * 0x10
    SKey1* Yaw;    int YawCount;                                  // +0x418 + c * 0x10
    SKey1* Dist;   int DistCount;                                 // +0x558 + c * 0x10
    int   Type;                                                   // +0x698 (0 target camera, 2 eye camera)
    int   Near;                                                   // +0x6e8
    float Far;                                                    // +0x738
};
SCamera g_Cams[kCams];
unsigned char* g_CamOfFrame;
unsigned char* g_Colour;                                          // +0x134 (Length + 1 colours)

void FreeCameras()
{
    for (int c = 0; c < kCams; ++c) {
        free(g_Cams[c].Raw);
        free(g_Cams[c].Pos);
        free(g_Cams[c].Pitch);
        free(g_Cams[c].Yaw);
        free(g_Cams[c].Dist);
        memset(&g_Cams[c], 0, sizeof(SCamera));
        g_Cams[c].Far = 100.0f;                                   // 0x42c80000
        g_Cams[c].Type = 9;                                       // (CAMS default 9 at 0x929798)
    }
    free(g_CamOfFrame);
    g_CamOfFrame = nullptr;
    free(g_Colour);
    g_Colour = nullptr;
}

// PANZERS 0x672dd0
// Kochanek-Bartels tangents of the position keys (open spline: the end
// tangents from the first / last three keys).
void Tangents3(SKey3* k, int n)
{
    const double K15 = 1.5;                                       // 0x7fe0c8
    if (n < 3) {
        if (n != 2)
            return;
        double s = ((double)(k[1].t - k[0].t) * 0.5) / (double)(k[1].t - k[0].t);
        double a = 1.0 - (double)k[0].T;
        for (int i = 0; i < 3; ++i) {
            float d = k[1].p[i] - k[0].p[i];
            k[0].out[i] = (float)((double)((float)((double)d * K15) - (float)((double)d * s)) * a);
        }
        double b = 1.0 - (double)k[1].T;
        for (int i = 0; i < 3; ++i) {
            float d = k[1].p[i] - k[0].p[i];
            k[1].in[i] = (float)((double)((float)((double)d * K15) - (float)((double)d * s)) * b);
        }
        return;
    }
    for (int j = 1; j < n - 1; ++j) {
        SKey3& p = k[j - 1];
        SKey3& c = k[j];
        SKey3& x = k[j + 1];
        float dIn[3], dOut[3];
        for (int i = 0; i < 3; ++i) {
            dIn[i] = c.p[i] - p.p[i];
            dOut[i] = x.p[i] - c.p[i];
        }
        float span = x.t - p.t;
        double oneT = 1.0 - (double)c.T;
        double s = (double)((c.t - p.t) / span) * oneT;
        float a = (float)((1.0 - (double)c.C) * s * ((double)c.B + 1.0));
        float b = (float)(((double)c.C + 1.0) * s * (1.0 - (double)c.B));
        for (int i = 0; i < 3; ++i)
            c.in[i] = a * dIn[i] + b * dOut[i];
        s = (double)((x.t - c.t) / span) * oneT;
        float e = (float)(((double)c.C + 1.0) * s * ((double)c.B + 1.0));
        float f = (float)((1.0 - (double)c.C) * s * (1.0 - (double)c.B));
        for (int i = 0; i < 3; ++i)
            c.out[i] = e * dIn[i] + f * dOut[i];
    }
    // first key, outgoing
    double s = ((double)(k[1].t - k[0].t) * 0.5) / (double)(k[2].t - k[0].t);
    double a = 1.0 - (double)k[0].T;
    for (int i = 0; i < 3; ++i)
        k[0].out[i] = (float)((double)((float)((double)(k[1].p[i] - k[0].p[i]) * K15) -
                                       (float)((double)(k[2].p[i] - k[0].p[i]) * s)) * a);
    // last key, incoming
    int l = n - 1;
    s = ((double)(k[l].t - k[l - 1].t) * 0.5) / (double)(k[l].t - k[l - 2].t);
    a = 1.0 - (double)k[l].T;
    for (int i = 0; i < 3; ++i)
        k[l].in[i] = (float)((double)((float)((double)(k[l].p[i] - k[l - 1].p[i]) * K15) -
                                      (float)((double)(k[l].p[i] - k[l - 2].p[i]) * s)) * a);
}

// PANZERS 0x673c00 (the same for one value)
void Tangents1(SKey1* k, int n)
{
    const double K15 = 1.5;
    if (n < 3) {
        if (n != 2)
            return;
        double s = ((double)(k[1].t - k[0].t) * 0.5) / (double)(k[1].t - k[0].t);
        float d = k[1].v - k[0].v;
        k[0].out = (float)((double)((float)((double)d * K15) - (float)(s * (double)d)) * (1.0 - (double)k[0].T));
        k[1].in = (float)((double)((float)((double)d * K15) - (float)(s * (double)d)) * (1.0 - (double)k[1].T));
        return;
    }
    for (int j = 1; j < n - 1; ++j) {
        SKey1& p = k[j - 1];
        SKey1& c = k[j];
        SKey1& x = k[j + 1];
        float dIn = c.v - p.v, dOut = x.v - c.v;
        float span = x.t - p.t;
        double oneT = 1.0 - (double)c.T;
        double s = (double)((c.t - p.t) / span) * oneT;
        float a = (float)((1.0 - (double)c.C) * s * ((double)c.B + 1.0));
        float b = (float)(((double)c.C + 1.0) * s * (1.0 - (double)c.B));
        c.in = a * dIn + b * dOut;
        s = (double)((x.t - c.t) / span) * oneT;
        float e = (float)(((double)c.C + 1.0) * s * ((double)c.B + 1.0));
        float f = (float)((1.0 - (double)c.C) * s * (1.0 - (double)c.B));
        c.out = e * dIn + f * dOut;
    }
    double s = ((double)(k[1].t - k[0].t) * 0.5) / (double)(k[2].t - k[0].t);
    k[0].out = (float)((double)((float)((double)(k[1].v - k[0].v) * K15) - (float)((double)(k[2].v - k[0].v) * s)) *
                       (1.0 - (double)k[0].T));
    int l = n - 1;
    s = ((double)(k[l].t - k[l - 1].t) * 0.5) / (double)(k[l].t - k[l - 2].t);
    k[l].in = (float)((double)((float)((double)(k[l].v - k[l - 1].v) * K15) - (float)((double)(k[l].v - k[l - 2].v) * s)) *
                      (1.0 - (double)k[l].T));
}

// The ease-in / ease-out remapping of 0x6749e0 / 0x676200.
float Ease(float u, float from, float to)
{
    float sum = from + to;
    if (sum == 0.0f)
        return u;
    if (1.0f < sum) {
        from /= sum;
        to /= sum;
    }
    float k = (float)(1.0 / ((2.0 - (double)from) - (double)to));
    if (u < from)
        return (k / from) * u * u;
    if ((double)u < 1.0 - (double)to)
        return (u * 2.0f - from) * k;
    return (float)(1.0 - (double)((k / to) * (float)(1.0 - (double)u) * (float)(1.0 - (double)u)));
}

// The key segment of time t (clamped to the keys), as both evaluators.
template <typename K>
int Segment(const K* k, int n, float* t)
{
    int s = 0;
    for (int j = 1; s < n - 2; ++j, ++s)
        if (*t < k[j].t)
            break;
    if (*t < k[s].t)
        *t = k[s].t;
    if (k[s + 1].t <= *t)
        *t = k[s + 1].t;
    return s;
}

// PANZERS 0x6749e0 (open spline)
void Eval3(const SKey3* k, int n, float t, float* out)
{
    if (n < 2) {
        out[0] = k[0].p[0];
        out[1] = k[0].p[1];
        out[2] = k[0].p[2];
        return;
    }
    int s = Segment(k, n, &t);
    const SKey3& a = k[s];
    const SKey3& b = k[s + 1];
    float u = Ease((t - a.t) / (b.t - a.t), a.easeFrom, b.easeTo);
    float u2 = u * u, u3 = u2 * u;
    float h4 = u3 - u2;
    float h3 = (u3 - u2 * 2.0f) + u;
    float h2 = u2 * 3.0f - u3 * 2.0f;
    float h1 = (u3 * 2.0f - u2 * 3.0f) + 1.0f;
    for (int i = 0; i < 3; ++i)
        out[i] = h4 * b.in[i] + h1 * a.p[i] + h2 * b.p[i] + h3 * a.out[i];
}

// PANZERS 0x676200 (open spline)
float Eval1(const SKey1* k, int n, float t)
{
    if (n < 2)
        return k[0].v;
    int s = Segment(k, n, &t);
    const SKey1& a = k[s];
    const SKey1& b = k[s + 1];
    float u = Ease((t - a.t) / (b.t - a.t), a.easeFrom, b.easeTo);
    float u2 = u * u, u3 = u2 * u;
    return (u2 * 3.0f - u3 * 2.0f) * b.v + ((u3 * 2.0f - u2 * 3.0f) + 1.0f) * a.v + ((u3 - u2 * 2.0f) + u) * a.out +
           (u3 - u2) * b.in;
}

// PANZERS 0x58b760
// The splines of camera c from its raw keys (time = frame / 20, angles in
// radians).
void InitCamera(int c)
{
    SCamera& cam = g_Cams[c];
    int n = cam.RawCount;
    if (n == 0)
        return;
    cam.Pos = (SKey3*)calloc(n, sizeof(SKey3));
    cam.Pitch = (SKey1*)calloc(n, sizeof(SKey1));
    cam.Yaw = (SKey1*)calloc(n, sizeof(SKey1));
    cam.Dist = (SKey1*)calloc(n, sizeof(SKey1));
    cam.PosCount = cam.PitchCount = cam.YawCount = cam.DistCount = n;
    const float deg = 0.017453292f;                               // 0x7f59a0
    for (int i = 0; i < n; ++i) {
        const SCamKey& r = cam.Raw[i];
        float t = (float)r.Frame / 20.0f;                         // 0x7f35d8
        SKey3& p = cam.Pos[i];
        p.t = t; p.p[0] = r.X; p.p[1] = r.Y; p.p[2] = r.Z; p.T = r.T; p.C = r.C; p.B = r.B;
        SKey1& a = cam.Pitch[i];
        a.t = t; a.v = r.Pitch * deg; a.T = r.T; a.C = r.C; a.B = r.B;
        SKey1& y = cam.Yaw[i];
        y.t = t; y.v = r.Yaw * deg; y.T = r.T; y.C = r.C; y.B = r.B;
        SKey1& d = cam.Dist[i];
        d.t = t; d.v = r.Dist; d.T = r.T; d.C = r.C; d.B = r.B;
    }
    Tangents3(cam.Pos, n);                                        // 0x672dd0
    Tangents1(cam.Pitch, n);                                      // 0x673c00
    Tangents1(cam.Yaw, n);
    Tangents1(cam.Dist, n);
}

// PANZERS 0x58ae50
// The active camera of every frame: the camera switch lines (type 1, "...
// <n>") set camera n - 1 from their frame on; camera 0 before the first.
void InitCameraOfFrame()
{
    g_CamOfFrame = (unsigned char*)malloc(g_Cut.Length + 1);
    memset(g_CamOfFrame, 0xff, g_Cut.Length + 1);
    for (int i = 0; i < g_Cut.LineCount; ++i) {
        const SAnimLine& l = g_Cut.Lines[i];
        if (l.Type != 1)
            continue;
        const char* sp = strrchr(l.Text, ' ');
        if (!sp || sp[1] == 0) {
            Logger.g->Warning("Error in script line %d. The script type is change_active_camera, but no the script line is not valid.", i);
            if (l.Frame >= 0 && l.Frame <= g_Cut.Length)
                g_CamOfFrame[l.Frame] = 0;
            continue;
        }
        if (l.Frame >= 0 && l.Frame <= g_Cut.Length)
            g_CamOfFrame[l.Frame] = (unsigned char)(atoi(sp + 1) - 1);
    }
    unsigned char cur = 0;
    for (int f = 0; f <= g_Cut.Length; ++f) {
        if (g_CamOfFrame[f] == 0xff)
            g_CamOfFrame[f] = cur;
        else
            cur = g_CamOfFrame[f];
    }
}

// PANZERS 0x58b170
// The colour of every frame (+0x134, 4 bytes r, g, b, a; 0 = none) from the
// "change_camera_colour r, g, b, a, r2, g2, b2, a2, seconds" lines (type 2):
// from the line's frame for ceil(seconds * 20) frames, linear.
// (g_Colour is defined with the camera tables above)
void InitColours()
{
    g_Colour = (unsigned char*)calloc(g_Cut.Length + 1, 4);
    for (int i = 0; i < g_Cut.LineCount; ++i) {
        const SAnimLine& l = g_Cut.Lines[i];
        if (l.Type != 2)
            continue;
        const char* p = l.Text;
        while (*p && *p != ' ')                                   // the command word (0x58c290)
            ++p;
        float v[9] = {0};
        for (int k = 0; k < 9; ++k) {                             // 0x58aa80 x 9 (',' separated)
            while (*p == ' ' || *p == ',')
                ++p;
            v[k] = (float)atof(p);
            while (*p && *p != ',')
                ++p;
        }
        int start = l.Frame;
        double end = ceil((double)v[8] * 20.0) + (double)start;  // 0x794710(atof * 20.0)
        for (int f = start; f <= (int)end && f <= g_Cut.Length; ++f) {
            float t = ((float)f - (float)start) / ((float)(int)end - (float)start);
            float u = 1.0f - t;
            if (f < 0)
                continue;
            g_Colour[f * 4 + 0] = (unsigned char)(int)(u * v[0] + t * v[4]);
            g_Colour[f * 4 + 1] = (unsigned char)(int)(u * v[1] + t * v[5]);
            g_Colour[f * 4 + 2] = (unsigned char)(int)(u * v[2] + t * v[6]);
            g_Colour[f * 4 + 3] = (unsigned char)(int)(u * v[3] + t * v[7]);
        }
    }
}

// PANZERS 0x589970 (the colour part): the colour between the two frames
// around `time`, as ARGB, to 0x58c5d0.
void ApplyColour(float time)
{
    if (!g_Colour)
        return;
    float ft = time * 20.0f;
    if ((double)g_Cut.Length < (double)ceilf(ft))
        ft = (float)g_Cut.Length;
    int i = (int)floorf(ft);
    int j = (int)ceilf(ft);
    if (i < 0 || j > g_Cut.Length)
        return;
    float b = ft - (float)i;
    float a = 1.0f - b;
    const unsigned char* x = g_Colour + i * 4;
    const unsigned char* y = g_Colour + j * 4;
    unsigned argb = (unsigned)((int)((float)x[3] * a + (float)y[3] * b) & 0xff);
    argb = (argb << 8) + (unsigned)((int)((float)x[0] * a + (float)y[0] * b) & 0xff);
    argb = (argb << 8) + (unsigned)((int)((float)x[1] * a + (float)y[1] * b) & 0xff);
    argb = (argb << 8) + (unsigned)((int)((float)x[2] * a + (float)y[2] * b) & 0xff);
    PzCutsceneOverlay(argb);                                      // 0x58c5d0
}

float BitsF(unsigned u)
{
    float f;
    memcpy(&f, &u, 4);
    return f;
}

// PANZERS 0x5fd9f0
// SWorld::SetCameraMode: from the target camera (0) to the eye camera (2)
// CamDist becomes the field of view (60 deg, 22.5 .. 120 deg), back to 0 the
// distance (24, 12 .. 30); either way the projection is rebuilt. The state
// of the free-fall camera (1) restarts above the map centre.
void SetCameraMode(SWorld* w, int mode)
{
    if (w->CamMode == 0 && mode == 2) {
        w->CamDist = BitsF(0x3f860a92);                           // +0x54
        w->CamDistMin = BitsF(0x3ec90fdb);                        // +0x24
        w->CamDistMax = BitsF(0x40060a92);                        // +0x28
        w->CamProjectionDirty = true;                             // +0x34
    }
    if (w->CamMode == 2 && mode == 0) {
        w->CamDist = 24.0f;
        w->CamDistMin = 12.0f;
        w->CamDistMax = 30.0f;
        w->CamProjectionDirty = true;
    }
    w->CamMode = mode;                                            // +0xac
    float* ff = (float*)((unsigned char*)w + 0xb0);               // free-fall state +0xb0..+0xd8
    ff[8] = 1.5f;                                                 // +0xd0
    float x = (float)(w->TerrainW / 2);
    float z = (float)(w->TerrainH / 2);
    ff[0] = x;                                                    // +0xb0
    ff[2] = z;                                                    // +0xb8
    float h = w->GetTerrainHeight(x, z);                          // 0x5e7730
    ff[3] = 0.0f;                                                 // +0xbc
    ff[4] = 0.0f;                                                 // +0xc0
    ff[5] = 0.0f;                                                 // +0xc4
    ff[6] = 0.0f;                                                 // +0xc8
    ff[1] = h + ff[8];                                            // +0xb4
    ff[7] = 0.0f;                                                 // +0xcc
    *((unsigned char*)w + 0xd8) = 1;
}

// PANZERS 0x589970 (the camera part)
void ApplyCamera(unsigned c, float time)
{
    ApplyColour(time);
    if (c >= (unsigned)kCams)
        Logger.g->Panic("SInGameAnimLogic::GetCameraType() was called, with an invalid cam nr. (incoming parameter: camnr>=NUMBER_OF_CAMERAS || camnr<0 (defined in ingameanimlogic.h))");
    SCamera& cam = g_Cams[c];
    if (cam.RawCount < 2)
        return;
    SWorld* w = g_World;
    SetCameraMode(w, cam.Type);                                   // 0x5fd9f0(type)
    float nearPlane = (float)(cam.Near + 1) / 10.0f;              // 0x7f5a7c
    if (w->CamNear != nearPlane) {
        w->CamNear = nearPlane;
        w->CamProjectionDirty = true;
    }
    if (w->CamFar != cam.Far) {
        w->CamFar = cam.Far;
        w->CamProjectionDirty = true;
    }
    float st[6];
    Eval3(cam.Pos, cam.PosCount, time, st);                       // 0x6749e0 (+0x148)
    st[3] = Eval1(cam.Yaw, cam.YawCount, time);                   // 0x676200 (+0x418)
    st[4] = Eval1(cam.Pitch, cam.PitchCount, time);               // (+0x2d8)
    st[5] = Eval1(cam.Dist, cam.DistCount, time);                 // (+0x558)
    // PANZERS 0x5fd880
    if (cam.Type == 2) {
        w->CamEye[0] = st[0];
        w->CamEye[1] = st[1];
        w->CamEye[2] = st[2];
    } else if (cam.Type == 0 && w->CamLocked == 0) {
        float x = st[0], z = st[2];
        if (x < w->CamXMin || w->CamXMax < x)
            x = x < w->CamXMin ? w->CamXMin : w->CamXMax;
        if (z < w->CamZMin || w->CamZMax < z)
            z = z < w->CamZMin ? w->CamZMin : w->CamZMax;
        w->CamTarget[0] = x;
        w->CamTarget[2] = z;
        w->CamFollowPath = 0;
        w->CamFollowUnit = -1;
    }
    w->SetCameraAngles(st[3], st[4]);                             // 0x5f83b0
    float d = st[5];
    if (d < w->CamDistMin || w->CamDistMax < d)
        d = d < w->CamDistMin ? w->CamDistMin : w->CamDistMax;
    w->CamDist = d;
    // PANZERS 0x5ef360: no smoothing towards the new camera
    w->CamSmoothTarget[0] = w->CamTarget[0];
    w->CamSmoothTarget[1] = w->CamTarget[1];
    w->CamSmoothTarget[2] = w->CamTarget[2];
    w->CamSmoothDist = w->CamDist;
}

void FreeLines()
{
    for (int i = 0; i < g_Cut.LineCount; ++i)
        free(g_Cut.Lines[i].Text);
    free(g_Cut.Lines);
    g_Cut.Lines = nullptr;
    g_Cut.LineCount = 0;
}

void FreeStatements()
{
    for (int i = 0; i < g_Cut.StatementCount; ++i)
        free(g_Cut.Statements[i].Text);
    free(g_Cut.Statements);
    g_Cut.Statements = nullptr;
    g_Cut.StatementCount = 0;
}

// PANZERS 0x5883f0
// Inserts a line before the first line with a later frame (lines of the same
// frame keep the file order).
void AddLine(int frame, int enabled, const char* text, int type)
{
    int k = 0;
    while (k < g_Cut.LineCount && g_Cut.Lines[k].Frame <= frame)
        ++k;
    g_Cut.Lines = (SAnimLine*)realloc(g_Cut.Lines, (g_Cut.LineCount + 1) * sizeof(SAnimLine));   // 0x589880
    memmove(&g_Cut.Lines[k + 1], &g_Cut.Lines[k], (g_Cut.LineCount - k) * sizeof(SAnimLine));
    ++g_Cut.LineCount;
    SAnimLine& l = g_Cut.Lines[k];
    l.Frame = frame;
    l.Enabled = enabled;
    l.Text = _strdup(text ? text : "");
    l.Type = type;
}

// PANZERS 0x58be80 (param 0)
// The enabled statement lines (type 0) become the game logic's statement
// list (SGameLogic +0x50, cleared first by 0x588fa0).
void CopyStatements()
{
    FreeStatements();
    for (int i = 0; i < g_Cut.LineCount; ++i) {
        const SAnimLine& l = g_Cut.Lines[i];
        if (l.Type != 0 || l.Enabled == 0)
            continue;
        g_Cut.Statements = (SCutscene::SStatement*)realloc(g_Cut.Statements,
                                                           (g_Cut.StatementCount + 1) * sizeof(SCutscene::SStatement));
        g_Cut.Statements[g_Cut.StatementCount].Frame = l.Frame;
        g_Cut.Statements[g_Cut.StatementCount].Text = _strdup(l.Text);
        ++g_Cut.StatementCount;
    }
}

// PANZERS 0x589d90 (SInGameAnimLogic::LoadProject, game path: param_3 = 0,
// the IMFN map name is not loaded)
void LoadProject(const char* file)
{
    FreeLines();                                                  // 0x58c3f0 (reset)
    FreeCameras();
    g_Cut.Length = 0;
    SStream* s = FileSystem.OpenRead(file, "SInGameAnimLogic::LoadProject");   // 0x65f420
    s->ReadSignature();                                           // 0x65d6a0
    if (s->ReadChunkHeader() != 0x30474e49)                       // 'ING0'
        Logger.g->Panic("Wrong file format: not .ingame file!");
    do {
        int id = s->ReadChunkHeader();
        switch (id) {
        case 0x4e46414c: {                                        // 'LAFN' length in frames
            g_Cut.Length = s->ReadInt();
            // HD sizes the per-frame camera arrays (0x588c20 / 0x588d00 /
            // 0x588c90, length + 1 and length / 50 + 1): camera only.
            break;
        }
        case 0x304c5349: {                                        // 'ISL0' type, enabled, frame, text
            int type = s->ReadInt();
            int enabled = s->ReadInt();
            int frame = s->ReadInt();
            char* text = s->ReadString();
            AddLine(frame, enabled, text, type);                  // 0x5883f0
            operator delete[](text);
            break;
        }
        case 0x494c5349: {                                        // 'ISLI' camera switch line (type 1)
            int type = s->ReadInt();
            int frame = s->ReadInt();
            char* text = s->ReadString();
            AddLine(frame, 1, text, type);
            operator delete[](text);
            break;
        }
        case 0x33534d43:                                          // 'CMS3' 20 cameras without the far plane
        case 0x34534d43: {                                        // 'CMS4'
            if (s->ReadInt() != kCams)
                Logger.g->Panic("Fatal error: The NUMBER_OF_CAMERAS define (defined in ingameanimlogic.h) has changed since last save. Load impossible. Aborting...");
            for (int c = 0; c < kCams; ++c) {
                SCamera& cam = g_Cams[c];
                int n = s->ReadInt();
                cam.RawCount = n;                                 // 0x588d70(n)
                if (n) {
                    cam.Raw = (SCamKey*)malloc(n * sizeof(SCamKey));
                    s->Read(cam.Raw, n * 0x28);                   // stream +4 (n * 0x28)
                }
                cam.Type = s->ReadInt();                          // 0x929798
                cam.Near = s->ReadInt();                          // 0x9297e8
                cam.Far = id == 0x34534d43 ? s->ReadFloat() : 100.0f;   // 0x929838
                char* name = s->ReadString();                     // camera name (0x52c320)
                operator delete[](name);
            }
            break;
        }
        default:
            // IMFN (map, editor only), CAM0..CAM4 / CAMS / CMS2 / CMOD (older
            // camera formats, not in the shipped cut-scenes), PSTN.
            s->ReadChunkSkip();                                   // 0x65d430
            break;
        }
        s->ReadChunkValidate(0);                                  // 0x65d460
    } while (!s->ReadChunkIsEnd());                               // 0x65d3f0
    s->ReadChunkValidate(0);
    s->Release();
    // End of LoadProject: +0x20 = 1, World->SetCameraLimits(0), 0x58be80(0),
    // and in the game (+0x18) the world seed restarts from 0.
    g_World->SetCameraLimits(false);                              // 0x5efb40(0)
    CopyStatements();                                             // 0x58be80(0)
    g_World->RandomSeed = 0;                                      // World +0x7518 (0x58a566)
    for (int c = 0; c < kCams; ++c)
        InitCamera(c);                                            // 0x58b760
    InitCameraOfFrame();                                          // 0x58ae50
    InitColours();                                                // 0x58b170
}

// PANZERS 0x56f530
void StartInGame(SGameLogic* gl, const char* name)
{
    PzMessagesClearFading(gl);                                    // 0x563860
    PzMessagesClearStatic(gl);                                    // 0x5638b0
    char file[300];
    _snprintf(file, sizeof(file) - 1, "%s.ingame", name);         // + ".ingame" (0x7f6720)
    file[sizeof(file) - 1] = 0;
    LoadProject(file);                                            // 0x929100->0x589d90(file, 0)
    g_World->GetCameraState(g_Cut.SavedCamera);                   // 0x5e6a70(gl +0x2c0)
    g_World->ApplySelectionToAll(0);                              // 0x5fc860(0)
    gl->Running = 1;                                              // +0x04
    PzCutsceneSoundResume();                                      // Concert +0x64
    g_Cut.SavedOverlay = gl->VisOverlayMode;
    gl->SetVisOverlayMode(0);                                     // 0x57f970(0)
    *((unsigned char*)g_World + 0x4d0) = 1;                       // World +0x4d0
    g_World->SetCameraLimits(false);                              // 0x5efb40(0)
    PZ_FOR_EACH_UNIT(i) {
        if (!UV::B150(i) && !UV::Unplaced(i)) {
            int ct = UV::ClassType(i);
            if (ct == 5 || ct == 0 || ct == 0xc || ct == 0xb || ct == 9) {
                if (UV::kReal) {
                    // 0x5bb7b0(8, 0, 0): stop (through OrderPlain 0x564440 on a
                    // one-unit group: the same per-unit call)
                    SFoundUnit one;
                    memset(&one, 0, sizeof(one));
                    one.Unit = i;
                    SFoundUnits g;
                    memset(&g, 0, sizeof(g));
                    g.Units = &one;
                    g.Count = 1;
                    g.Max = 1;
                    gl->OrderPlain(&g, 8, false, false);
                    UV::Iface(i)->ClearTargets();                 // +0xc4
                }
            }
        }
        if (UV::kReal)
            WorldUnit(i)->_1d4 = false;                           // +0x1d4
    }
    // name + ".sub": the subtitles (0x56fd70; none for the tutorial).
    char sub[300];
    _snprintf(sub, sizeof(sub) - 1, "%s.sub", name);              // + ".sub" (0x7f66e0)
    sub[sizeof(sub) - 1] = 0;
    PzSubtitlesLoad(sub);                                         // 0x56fd70
    for (int k = 0; k < g_Cut.StatementCount; ++k)                // +0x50 / +0x54
        PzExecuteScriptStatement(gl, g_Cut.Statements[k].Text, true);   // 0x568bc0(text, 1)
    if (g_Scene)
        g_Scene->Slot_28();                                       // scene +0x28 (0x6ac6e0)
    char mp3[300];
    _snprintf(mp3, sizeof(mp3) - 1, "%s.mp3", name);              // + ".mp3" (0x7f66e8)
    mp3[sizeof(mp3) - 1] = 0;
    g_Cut.SoundId = PzCutsceneSoundStart(mp3);                    // Concert +0x2c(mp3, 1, 0, 0, 1) -> +0x2d4
    gl->Flag2b8 = true;                                           // +0x2b8
    // HD then sets the letterbox viewport mode by the difficulty option
    // (0x64e240, campaign 0x591fb0: Gepard +0x10(2)); the recompile's
    // letterbox is SGameView::Update's panel mode 2 (gameview_view.cpp).
    PzViewResetClock(gl->Mode);                                   // (+0x00) vtbl +0x0c 0x624730
    g_Cut.Running = true;
    g_Cut.StartFrame = gl->Frame;                                 // +0x2bc = +0x08
    Logger.g->Log(0, "PZM4: cut-scene %s: %d statements, %d frames, start frame %d", name, g_Cut.StatementCount,
                  g_Cut.Length, g_Cut.StartFrame);
}

} // namespace

bool PzCutsceneRunning()
{
    return g_Cut.Running;
}

// PANZERS 0x589860
bool PzCutsceneHasCamera()
{
    for (int c = 0; c < kCams; ++c)
        if (1 < g_Cams[c].RawCount)                               // +0x3c + c * 0xc
            return true;
    return false;
}

// PANZERS 0x56ea20
void PzCutscenePlay(SGameLogic* gl, const char* name)
{
    char file[300];
    _snprintf(file, sizeof(file) - 1, "%s.ingame", name);         // 0x52c580(".ingame")
    file[sizeof(file) - 1] = 0;
    if (FileSystem.Stat(file, nullptr) == 0) {                    // 0x65faf0 == 0: the file exists
        StartInGame(gl, name);                                    // 0x56f530
        return;
    }
    // PANZERS 0x56f0d0: the .4d cut-scene (own scene, camera node +0x108,
    // length +0x80, "_fade.ingame", .sub, mp3, SGameLogic +0x288). Not
    // lifted: the cut-scene is skipped.
    Logger.g->Log(1, "STUB: SGameLogic 0x56f0d0 .4d cut-scene %s not played (skipped)", name);
}

// PANZERS 0x568af0
void PzCutsceneTick(SGameLogic* gl)
{
    if (!gl->Flag2b8)
        return;
    int elapsed = gl->Frame - g_Cut.StartFrame;                   // +0x08 - +0x2bc
    for (int k = 0; k < g_Cut.StatementCount; ++k) {
        if (g_Cut.Statements[k].Frame == elapsed) {
            Logger.g->Log(0, "PZM4: cut-scene frame %d (+%d): %s", gl->Frame, elapsed, g_Cut.Statements[k].Text);
            PzExecuteScriptStatement(gl, g_Cut.Statements[k].Text, false);   // 0x568bc0(text, 0)
        }
    }
    PzSubtitlesShow((elapsed * 0x19) / 0x14);                     // 0x5826c0(elapsed * 25 / 20)
    if (elapsed > g_Cut.Length)                                   // [0x929110]
        PzCutsceneEnd(gl);                                        // 0x565390
}

// PANZERS 0x58adc0 (from SGameLogic::UpdateUnitVisuals 0x5638f0 while
// +0x2b8 is set): the camera of the elapsed frame, between the ticks.
void PzCutsceneCamera(SGameLogic* gl, double interpolation)
{
    if (!gl->Flag2b8 || !g_CamOfFrame)
        return;
    int frame = gl->Frame - g_Cut.StartFrame;                     // +0x08 - +0x2bc
    if (g_Cut.Length < frame || frame < 0)
        return;
    ApplyCamera(g_CamOfFrame[frame], ((1.0f - (float)interpolation) + (float)frame) / 20.0f);   // 0x589970
}

// Recompile-only test hook (inert when PZ_M5_CS_FORCE is unset):
// PZ_M5_CS_FORCE=<frame>:<name> runs what trigger action 0x38 with <name>
// does (the map cut-scene "maps/<name>.map" when it exists and campaign +0x14
// is clear, else "cutscenes/<name>/<name>") at that logic frame of a mission
// (a logic with a view), once per process.
void PzCutsceneTestHook(SGameLogic* gl)
{
    static int s_Frame = -2;
    static char s_Name[128];
    if (s_Frame == -2) {
        s_Frame = -1;
        const char* e = getenv("PZ_M5_CS_FORCE");
        const char* c = e ? strchr(e, ':') : nullptr;
        if (c && c[1]) {
            s_Frame = atoi(e);
            strncpy(s_Name, c + 1, sizeof(s_Name) - 1);
        }
    }
    if (s_Frame < 0 || gl->Mode == 0 || gl->Frame != s_Frame || gl->Flag2b8)
        return;
    s_Frame = -1;
    Logger.g->Log(0, "PZM5: PZ_M5_CS_FORCE %s at frame %d", s_Name, gl->Frame);
    char map[300];
    _snprintf(map, sizeof(map) - 1, "maps/%s.map", s_Name);
    map[sizeof(map) - 1] = 0;
    if (g_Campaign && g_Campaign->_014 == 0 && FileSystem.Stat(map, nullptr) == 0) {
        PzMapCutscene(gl, map);
        return;
    }
    char name[300];
    _snprintf(name, sizeof(name) - 1, "cutscenes/%s/%s", s_Name, s_Name);
    name[sizeof(name) - 1] = 0;
    PzCutscenePlay(gl, name);
}

// PANZERS 0x565390
void PzCutsceneEnd(SGameLogic* gl)
{
    if (gl->Flag2b8) {
        // 0x58abb0: 0x588ef0(0) + 0x58be80(0) (the camera of frame 0, the
        // statement list again).
        CopyStatements();
        PzCutsceneOverlay(0);                                     // 0x588ef0(0): the colour box goes
        gl->Flag2b8 = false;
        g_Cut.Running = false;
        // viewport +0x10(2, mode): the letterbox off (not lifted).
        SetCameraMode(g_World, 0);                                // 0x5fd9f0(0)
        g_World->LoadCamera((const float*)g_Cut.SavedCamera);     // 0x5fd7c0(gl +0x2c0)
        g_World->SetCameraLimits(true);                           // 0x5efb40(1)
        g_World->MoveCamera(0.0f, 0.0f);                          // 0x5f4dc0(0, 0)
        g_World->RotateCamera(0.0f, 0.0f);                        // 0x5f8380(0, 0)
        g_World->ZoomCamera(0.0f);                                // 0x609390(0)
        if (g_World->CamNear != 1.0f) {                           // World +0x2c (0x7f1b58 = 1.0)
            g_World->CamNear = 1.0f;
            g_World->CamProjectionDirty = true;                   // +0x34
        }
        *((unsigned char*)g_World + 0x4d0) = 0;                   // World +0x4d0
        gl->SetVisOverlayMode(g_Cut.SavedOverlay);                // 0x57f970(0x64df00()): the fog-of-war option
        PzCutsceneSoundStop(g_Cut.SoundId);                       // Concert +0x3c(+0x2d4)
        g_Cut.SoundId = -1;
        PzSubtitlesEnd();                                         // board +0x0c(+0x274), 0x5634d0(0)
        Logger.g->Log(0, "PZM4: cut-scene end at frame %d", gl->Frame);
    }
    // 0x589140: SInGameAnimLogic camera state reset (not lifted).
}

} // namespace pz
